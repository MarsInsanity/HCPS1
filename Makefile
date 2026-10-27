PYTHON ?= python3
.PHONY: all native sf3000 test
all: native
native:
	$(PYTHON) scripts/build.py --target native
sf3000:
	$(PYTHON) scripts/build.py --target sf3000
test:
	$(PYTHON) scripts/test.py
