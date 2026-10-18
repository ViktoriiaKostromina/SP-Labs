IMAGE ?= shifttoken-build:lab12

.PHONY: image build clean

image:
	docker build -t $(IMAGE) docker

build: image
	docker run --rm -v "$(CURDIR):/workspace" -w /workspace \
		--user "$(shell id -u):$(shell id -g)" $(IMAGE) \
		make -C firmware all

clean:
	$(MAKE) -C firmware clean

# Form B: start/restart and UART use the licensed Wokwi extension.
.PHONY: flash monitor
flash monitor:
	@printf '%s\n' 'Open this project in VS Code; F1 > Wokwi: Start Simulator.' 'Firmware: firmware/build/firmware.hex; UART: Wokwi terminal, 115200 8N1.'
