# Залежності та походження

| Частина | Версія / джерело | Межі |
|---|---|---|
| Стартовий Core, CMSIS, linker | starter-template.md, збережений .ioc | CubeMX-заготовка; App/Devices/Inputs/Board розроблено окремо. |
| STM32C0 HAL | v1.0.1, STMicroelectronics, [офіційне дерево](https://github.com/STMicroelectronics/stm32c0xx-hal-driver/tree/v1.0.1) | Vendored Drivers з ліцензіями. ADC/ADCEx/SPI додані з тієї самої версії. |
| CMSIS Core(M) | cmsis_version.h: V5.0.3, макроси main5/sub3 | Стартова залежність. |
| STM32C0 CMSIS device | v1.0.0 у stm32c0xx.h | Startup і регістри C031. |
| Ubuntu / ARM GCC | Ubuntu24.04, ARM GCC13.2.1 | Пакети — Dockerfile; apt не закріплено назавжди за версією. |
| Host compiler / gcovr | Ubuntu gcc, gcovr з Dockerfile | Portable C11 tests; HAL/Board поза coverage. |
| Wokwi VS Code / CLI | Extension4.1.0, CLI0.29.1 | Чинна Community License; CLI lint не є native симуляцією. CI simulation token відсутній. |
| PulseView / sigrok | PulseView0.4.2, Qt5.15.19, libsigrok0.5.2, libsigrokdecode0.5.3 | Імпорт та декодування власних VCD. |
| Драйвери, inputs, bring-up, тести | firmware/Devices, Inputs, App; tests | Власна прикладна частина через інжектовані контракти. |

Власний код: -Wall -Wextra -Werror, -Os, -fstack-usage. HAL DMA є компіляційною залежністю, апаратний DMA не налаштовано; I²C/PWR/EXTI не потрібні. Ліцензії збережено в Drivers. Перелік SHA-256 збереженого snapshot — vendor-sha256.txt.
