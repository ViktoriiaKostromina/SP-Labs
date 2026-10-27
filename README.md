# ShiftToken

Костроміна Вікторія Романівна, ІО-36.

Термінал обліку робочих сесій за RFID. Потенціометр задає код роботи, кнопки — паузу і завершення. UID є міткою, а не захищеною ідентифікацією. Nucleo-C031C6; Ubuntu 24.04, Docker, STM32 HAL, Wokwi.

ЛР1: PRD, компоненти, відтворювана ARM-збірка, фактичні UART/LED. ЛР2: одночасний bring-up MFRC522, microSD, GPIO та ADC; переносні драйвери, host-тести й власні записи сигналів. Сесійний автомат і FreeRTOS залишаються [планом ЛР3–4](docs/future-lab3-lab4.md).

[PDF ЛР1](docs/reports/Лабораторна_робота_1_Вікторія_Костроміна_ІО-36.pdf) · [PDF ЛР2](docs/reports/Лабораторна_робота_2_Вікторія_Костроміна_ІО-36.pdf) · [Чеклист](docs/checklist.md) · [PRD](docs/PRD.md)

## Відтворення

Перейти звичайною командою `cd` до фактичного кореня цього репозиторію, потім:

```sh
make build
make test
```

Відкрити цей корінь у VS Code, F1 → Wokwi: Start Simulator. `wokwi.toml` використовує `firmware/build/firmware.hex` і `.elf`; `make prepare-sd` створює окремий `sim/demo.img`, зберігаючи наявний. Звичайна схема містить усі пристрої та SPI probes. UART 115200 8N1: `status`, `selftest`, `sdread`, `sdwrite 92`, `capture 10000`, `help`. Запис SD використовує тільки сектор127 демонстраційного образу.

[Pinout і живлення](docs/pinout.md) · [Архітектура](docs/architecture.md) · [Залежності](docs/dependencies.md) · [Аналіз трас](docs/captures/lab2/analysis.md) · [Індекс доказів](docs/evidence/lab2/README.md) · [Компоненти](docs/hardware-components.md) · [Стартовий шаблон](docs/starter-template.md)

## Межі перевірки

Host: 612 перевірок Devices/Inputs та окремі тести bring-up; покриття переносних шарів 99,3% рядків, 90,3% гілок. Це не перевірка HAL або фізичної плати. Власні VCD підтверджують SPI/SD, UART, GPIO та маркер ADC.

Модель NFC Tag повертає неузгоджений UID7: фінальний драйвер відхиляє відповідь як PROTOCOL. Коректні каскади UID4/7 перевірено на host; фізичний UID7 та повторний WUPA утримуваної картки не підтверджені. Струм, аналогова точність, RF та SD power-loss atomicity потребують апаратного досліду. Ці вимірювання не є готовою baseline ЛР3.
