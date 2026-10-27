/* SPDX-License-Identifier: GPL-2.0-or-later
 * Small libretro host for x86 Linux and qemu-mipsel. Also accepts a user's game
 * in --benchmark mode; speed printed under QEMU is never a hardware benchmark.
 */
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "libretro.h"

static struct retro_core_option_v2_definition *defs;
static unsigned video_frames, red_frames;
static size_t audio_frames;
static int interpreter, benchmark;
static const char *system_dir = ".";
static bool environment(unsigned cmd, void *data)
{
    switch (cmd) {
    case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION:
        *(unsigned *)data = 2; return true;
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2:
        defs = ((struct retro_core_options_v2 *)data)->definitions; return true;
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2_INTL:
        defs = ((struct retro_core_options_v2_intl *)data)->us->definitions; return true;
    case RETRO_ENVIRONMENT_GET_VARIABLE: {
        struct retro_variable *v = data;
        unsigned i;
        if (!strcmp(v->key, "pcsx_rearmed_bios") && !benchmark) v->value = "HLE";
        else if (!strcmp(v->key, "pcsx_rearmed_drc"))
            v->value = interpreter ? "disabled" : "enabled";
        else if (!strcmp(v->key, "pcsx_rearmed_frameskip_type")) v->value = "disabled";
        else if (!strcmp(v->key, "pcsx_rearmed_memcard2")) v->value = "none";
        else {
            v->value = NULL;
            for (i = 0; defs && defs[i].key; ++i)
                if (!strcmp(v->key, defs[i].key)) { v->value = defs[i].default_value; break; }
        }
        return v->value != NULL;
    }
    case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
        *(bool *)data = false; return true;
    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
        *(const char **)data = system_dir; return true;
    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
        return *(enum retro_pixel_format *)data == RETRO_PIXEL_FORMAT_RGB565;
    case RETRO_ENVIRONMENT_GET_CAN_DUPE:
        *(bool *)data = true; return true;
    case RETRO_ENVIRONMENT_GET_INPUT_BITMASKS:
        return true;
    case RETRO_ENVIRONMENT_GET_LANGUAGE:
        *(unsigned *)data = RETRO_LANGUAGE_ENGLISH; return true;
    default:
        return false;
    }
}
static void video(const void *data, unsigned w, unsigned h, size_t pitch)
{
    unsigned x, y, red = 0;
    ++video_frames;
    if (!data) return;
    for (y = 0; y < h; ++y)
        for (x = 0; x < w; ++x) {
            uint16_t p;
            memcpy(&p, (const char *)data + y*pitch + 2*x, 2);
            if (p == 0xf800) ++red;
        }
    if (red > 1000) ++red_frames;
}
static void sample(int16_t l, int16_t r) { (void)l; (void)r; ++audio_frames; }
static size_t batch(const int16_t *data, size_t frames)
{ (void)data; audio_frames += frames; return frames; }
static void poll_input(void) {}
static int16_t input(unsigned port, unsigned device, unsigned index, unsigned id)
{ (void)port; (void)device; (void)index; (void)id; return 0; }
static double now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}
static uint32_t word(const void *p, size_t offset)
{
    const unsigned char *s = (const unsigned char *)p + offset;
    return s[0] | ((uint32_t)s[1]<<8) | ((uint32_t)s[2]<<16) | ((uint32_t)s[3]<<24);
}
#define LOAD(name) __typeof__(&name) f_##name = (__typeof__(&name))dlsym(lib, #name); \
    if (!f_##name) { fprintf(stderr, "Missing %s: %s\n", #name, dlerror()); return 2; }

int main(int argc, char **argv)
{
    void *lib, *state, *ram;
    size_t state_size;
    unsigned i, frames = 90;
    uint32_t saved, replay;
    double started, elapsed;
    struct retro_game_info game = {0};
    struct retro_system_info info;
    struct retro_system_av_info av;
    if (argc < 3) { fprintf(stderr, "usage: smoke core.so test.exe [--interpreter | --benchmark frames bios-dir]\n"); return 2; }
    if (argc > 3 && !strcmp(argv[3], "--interpreter")) interpreter = 1;
    if (argc > 3 && !strcmp(argv[3], "--benchmark")) {
        benchmark = 1;
        if (argc > 4) frames = (unsigned)strtoul(argv[4], NULL, 10);
        if (!frames) return 2;
        if (argc > 5) system_dir = argv[5];
    }
    lib = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!lib) { fprintf(stderr, "%s\n", dlerror()); return 2; }
    LOAD(retro_set_environment); LOAD(retro_set_video_refresh);
    LOAD(retro_set_audio_sample); LOAD(retro_set_audio_sample_batch);
    LOAD(retro_set_input_poll); LOAD(retro_set_input_state);
    LOAD(retro_init); LOAD(retro_deinit); LOAD(retro_get_system_info);
    LOAD(retro_get_system_av_info); LOAD(retro_load_game); LOAD(retro_unload_game);
    LOAD(retro_run); LOAD(retro_get_memory_data); LOAD(retro_get_memory_size);
    LOAD(retro_serialize_size); LOAD(retro_serialize); LOAD(retro_unserialize);
    f_retro_set_environment(environment); f_retro_set_video_refresh(video);
    f_retro_set_audio_sample(sample); f_retro_set_audio_sample_batch(batch);
    f_retro_set_input_poll(poll_input); f_retro_set_input_state(input);
    f_retro_init(); f_retro_get_system_info(&info);
    if (strcmp(info.library_name, "HCPS1")) return 3;
    game.path = argv[2];
    if (!f_retro_load_game(&game)) { fprintf(stderr, "Load failed\n"); return 3; }
    f_retro_get_system_av_info(&av);
    started = now();
    for (i = 0; i < frames; ++i) f_retro_run();
    elapsed = now() - started;
    printf("core=%s mode=%s run_frames=%u video_callbacks=%u audio_frames=%zu fps=%.2f fullspeed_ratio=%.3f\n",
           info.library_name, interpreter ? "interpreter" : "lightrec", frames,
           video_frames, audio_frames, frames/elapsed, frames/elapsed/av.timing.fps);
    if (!benchmark) {
        ram = f_retro_get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
        assert(ram && f_retro_get_memory_size(RETRO_MEMORY_SYSTEM_RAM) >= 0x18008);
        saved = word(ram, 0x18000);
        assert(saved > 100 && word(ram, 0x18004) == 1);
        assert(video_frames > 0 && red_frames > 0 && audio_frames > 0);
        state_size = f_retro_serialize_size();
        state = malloc(state_size); assert(state);
        assert(f_retro_serialize(state, state_size));
        for (i = 0; i < 30; ++i) f_retro_run();
        replay = word(ram, 0x18000);
        assert(replay != saved);
        assert(f_retro_unserialize(state, state_size));
        ram = f_retro_get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
        assert(word(ram, 0x18000) == saved);
        for (i = 0; i < 30; ++i) f_retro_run();
        assert(word(ram, 0x18000) == replay);
        free(state);
        puts("PASS: guest execution, branch delay, rendered red frame, audio, deterministic state replay");
    }
    f_retro_unload_game(); f_retro_deinit(); dlclose(lib);
    return 0;
}
