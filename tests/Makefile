CC ?= gcc
CFLAGS = -std=c11 -O0 -g --coverage -Wall -Wextra -Werror
INCLUDES = -Ifirmware/Devices -Ifirmware/Inputs -Ifirmware/App
PORTABLE = firmware/Devices/rfid.c firmware/Devices/sd_sector.c firmware/Inputs/token_inputs.c
OUT = build/host
.PHONY: all
all:
	mkdir -p $(OUT) build/coverage
	rm -f $(OUT)/*.gcda
	$(CC) $(CFLAGS) $(INCLUDES) tests/test_devices.c $(PORTABLE) -o $(OUT)/test_devices
	./$(OUT)/test_devices
	$(CC) $(CFLAGS) $(INCLUDES) tests/test_bringup.c $(PORTABLE) firmware/App/bringup.c -o $(OUT)/test_bringup
	./$(OUT)/test_bringup
	gcovr -r . --filter 'firmware/(App|Devices|Inputs)/' --json-summary build/coverage/summary.json --xml build/coverage/cobertura.xml --html-details build/coverage/index.html --print-summary --fail-under-line 80 --fail-under-branch 65
