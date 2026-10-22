IMAGE ?= shifttoken-build:lab2

.PHONY: image build clean

image:
	@docker build -t $(IMAGE) docker

build: image prepare-sd
	@docker run --rm -v "$(CURDIR):/workspace" -w /workspace \
		--user "$(shell id -u):$(shell id -g)" $(IMAGE) \
		make -C firmware all

clean:
	$(MAKE) -C firmware clean

# Form B: start/restart and UART use the licensed Wokwi extension.
.PHONY: flash monitor
flash monitor:
	@printf '%s\n' 'Open this project in VS Code; F1 > Wokwi: Start Simulator.' 'Firmware: firmware/build/firmware.hex; UART: Wokwi terminal, 115200 8N1.'

.PHONY: test prepare-sd
prepare-sd:
	@python3 tools/create_sd_image.py

test: image
	@docker run --rm -v "$(CURDIR):/workspace" -w /workspace --user "$(shell id -u):$(shell id -g)" $(IMAGE) python3 tools/check_layers.py
	@docker run --rm -v "$(CURDIR):/workspace" -w /workspace --user "$(shell id -u):$(shell id -g)" $(IMAGE) make -f tests/Makefile all
