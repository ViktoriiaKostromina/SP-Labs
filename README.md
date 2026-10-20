# ShiftToken

Костроміна Вікторія Романівна, ІО-36.

Навчальний термінал робочих сесій за RFID: код роботи з ADC, окремі паузи та підсумки. UID — мітка, не захищена ідентифікація. Плата Nucleo-C031C6; середовище: Ubuntu 24.04, Docker, STM32 HAL, Wokwi.

## ЛР1
Підтверджено дві побайтно ідентичні чисті ARM-збірки, стартовий UART і LED у власному Wokwi. Докази: [результати](docs/evidence/lab1/results.json), [UART](docs/evidence/lab1/uart-restart.txt), [VCD](docs/evidence/lab1/uart-led.vcd), [звіт](docs/report.md). Фізичної плати немає. Відео записується окремо студенткою; наявність сценарію не означає записаного відео.

```sh
make clean
make build
```

У VS Code відкрити цей корінь і виконати F1 → Wokwi: Start Simulator. UART показується у Wokwi Terminal; звичайна конфігурація посилається на firmware/build/firmware.hex та .elf. make flash / make monitor пояснюють саме цей спосіб форми B. Бінарники виміряної ЛР1 збережено також у docs/evidence/lab1.

[PRD](docs/PRD.md) · [Компоненти](docs/hardware-components.md) · [Походження шаблону](docs/starter-template.md) · [Чеклист](docs/checklist.md) · [План ЛР3–4](docs/future-lab3-lab4.md).

ЛР1 не реалізує сенсорні драйвери чи сесійний автомат. ЛР2 готує апаратний фундамент; повний продукт і його baseline — наступна ЛР3, FreeRTOS/порівняння — ЛР4.
