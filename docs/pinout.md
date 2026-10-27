# ShiftToken: підключення Nucleo-C031C6

Корпус STM32C031C6 LQFP48, 3,3 В. `diagram.json` — монтажний еквівалент форми B. В `firmware/Board/board.c` розміщені всі номери портів/пінів та HAL handles; App/Devices/Inputs їх не містять.

Первинні джерела: [DS13867 rev4](https://www.st.com/resource/en/datasheet/stm32c031c6.pdf), с.31–38, табл.12–20; [UM2953](https://www.st.com.cn/resource/en/user_manual/um2953-stm32-nucleo64-board-mb1717-stmicroelectronics.pdf), для Nucleo-C031C6 рис.7 і табл.11, с.19–20 rev1/rev2; [RM0490 rev5](https://www.st.com/resource/en/reference_manual/rm0490-stm32c0-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf), §16 ADC, §24 SPI/I²S. В локальних матеріалах перевірено офіційні PDF UM2953 rev1 та RM0490 rev5, а також актуальний UM2953 rev2 на сайті ST. Нумерація CN у рисунку відноситься до контактів роз’єму, не до номера виводу MCU.

| Пін МК | AF / режим | Сигнал, пристрій | Arduino / morpho | Напруга, підтягування | Чому цей пін |
|---|---|---|---|---|---|
| PB3 | AF0 SPI1 | SCK: RFID і SD | D3 CN9-4 / CN10-31 | 3,3 В, push-pull | DS табл.15; не займає LED PA5. |
| PB4 | AF0 SPI1 | MISO: SD DO і RFID MISO через fault-switch | D5 CN9-6 / CN10-27 | 3,3 В, pull-down MCU | DS табл.15; PA6 залишено CLOSE. Pull-down визначає LOW, коли обидва slave у high-Z; від’єднаний RFID не гарантує ID00, оскільки лінія спільна із SD. |
| PB5 | AF0 SPI1 | MOSI: RFID MOSI, SD DI | D6 CN9-7 / CN10-25 | 3,3 В, push-pull | DS табл.15; PA7 залишається ADC probe. D4 є PB10, не PB5. |
| PB0 | GPIO output | RFID SDA = CS# | D10 CN5-3 / CN10-17 (також дубль CN10-34) | idle HIGH | Програмний CS утримує всю транзакцію; апаратний NSS не обирає двох slave. Wokwi alias PB0.1. |
| PB1 | GPIO output | RFID RST / NRSTPD | A3 CN8-4 / CN7-34 | idle HIGH, reset LOW | Окремий доступний вихід, без AF-конфлікту; старт LOW 2 мс, стабілізація 50 мс як запас стенда. |
| PA4 | GPIO output | SD CS# | A2 CN8-3 / CN7-32 | idle HIGH | Вільний незалежний CS; не використовує ADC0. |
| PA0 | Analog ADC_IN0 | Потенціометр SIG | A0 CN8-1 / CN7-28 | 0–3,3 В, без pull | DS табл.12; один канал без scan/DMA; крайні виводи потенціометра до 3V3/GND. |
| PA1 | GPIO input | PAUSE, контакт до GND | A1 CN8-2 / CN7-30 | pull-up MCU, active-low | Вільний пін, відокремлений від CLOSE. |
| PA6 | GPIO input | CLOSE, контакт до GND | D12 CN5-5 / CN10-13 | pull-up MCU, active-low | SPI MISO перенесено на PB4, подвійного призначення немає. |
| PA7 | GPIO output | ADC_SAMPLE, логічний аналізатор | D11 CN5-4 / CN10-15 | 3,3 В, idle LOW | Позначає HAL_ADC_Start → отримання/stop; це час усієї операції HAL, не точна апертура ADC. |
| PA8 | GPIO output | CAPTURE_GATE | Morpho CN10-24 | 3,3 В, idle LOW | Вікно запису аналізатора; команда capture <1..10000 ms>, автоматична зупинка. |
| PA5 | GPIO output | LED плати | D13 CN5-6 / CN10-11 | push-pull | Вбудований LED збережено; змінюється за новою валідною карткою. |
| PA2 | AF1 USART2 | TX → UART logger RX | VCP / CN7-37 | 3,3 В | DS табл.13; UM §6.9, SB27 ON для VCP. Це не Arduino D1 (там PB6). |
| PA3 | AF1 USART2 | RX ← UART logger TX | VCP / CN10-10 | 3,3 В | DS табл.13; UM §6.9, SB32 ON для VCP. Це не Arduino D0 (там PB7). |
| 3V3 / GND | живлення | усі логічні модулі, спільна земля аналізатора | CN6-4 / CN6-6; morpho 3V3 CN7-16 | 3,3 В | У Wokwi 3V3.1 та GND.1; фізичний бюджет нижче. |

## Частоти та очікувані тривалості
HSI48 → SYSCLK = HCLK = PCLK = 48 МГц. SPI1 prescaler 128 → 375000 Гц, T=2,6667 мкс; mode0 (CPOL=0, CPHA=0), 8 біт, MSB first, NSS software, NSS pulse disabled. Це нижче ліміту MFRC522 10 Мбіт/с і відповідає початковій SD-частоті ≤400 кГц. Обидва пристрої залишаються на 375 кГц: максимальна пропускна здатність не є метою ЛР2. Два байти читання регістру — щонайменше 42,667 мкс чистих тактів; 512 B SD — 10,923 мс даних плюс команда, токени, busy і HAL.

ADC PCLK/4 = 12 МГц; sampling 79,5 cycles, 12-бітне перетворення ще 12,5 cycles. Розрахунок RM0490 §16.4.9, с.298: (79,5+12,5)/12 МГц = 7,667 мкс до витрат HAL. Довгий sampling зменшує залежність від опору потенціометра (Thevenin ≤2,5 кОм для 10 кОм). Реальна аналогова похибка та модель sampling в Wokwi перевіряються окремо. UART 115200 8N1: 10 біт/байт, 86,806 мкс/байт. SysTick 1 мс; ADC має номінальний release 10 мс, кнопки 5 мс, RFID 50 мс, summary/retry 1000 мс. Блокувальний RF/SD у bring-up може розширювати інтервали; це не готова часово перевірена реалізація FR ЛР3.

## Ідентифікатори
I²C не використовується, адрес I²C немає. RFID вибирається PB0: VersionReg 0x37, допустимі 0x91/0x92; SPI read MOSI = 0x80 | (reg<<1). SD вибирається PA4: CMD8 echo 0x000001AA, OCR bit31 ready і bit30 CCS; SDSC використовує byte address = LBA×512, SDHC — LBA. GPIO та потенціометр не мають ID-регістру.

## Живлення та межі моделі
NXP MFRC522 rev3.9, табл.153, с.77: digital typ/max 6,5/9 мА, analog RX 7/10 мА, transmitter continuous wave 60/100 мА, додаткові PVDD/SVDD навантаження до 40/4 мА. Це окремі rails/умови чіпа, не виміряний повний модуль. Бюджетний резерв RFID — 180 мА; SD — 200 мА як початковий резерв до вибору/виміру конкретної фізичної картки; потенціометр ≤0,413 мА з допуском −20%; дві кнопки ≤0,4 мА за прийнятим резервом pull-up. Разом 380,813 мА зовнішніх модулів.

UM2953 §6.4 обмежує сумарне навантаження USB-живлення плати та shield 500 мА; це не додаткові 500 мА лише для модулів. Без виміру самої плати та піків SD запас не доведено. Для апаратного повторення доцільне окреме стабілізоване 3,3 В / 1 А джерело модулів (розрахунковий резерв 619 мА до модулів), спільна земля і розв’язка; його вихід не з’єднувати напряму з виходом регулятора Nucleo. Wokwi використовує ідеальне живлення від 3V3 моделі і не доводить ці струми, просідання чи фронти.

## Відхилені розкладки та конфлікти
1. SPI PA5/PA6/PA7: PA5 зайнятий LED, PA6 потрібний CLOSE, PA7 — маркер вибірки. Обрано PB3/PB4/PB5 AF0.
2. Один апаратний NSS із pulse mode для RFID+SD: немає незалежного вибору, порушується утримання CS під час багатобайтових SD-транзакцій. Обрано два GPIO CS та NSS_SOFT/NSSP_DISABLE.
3. USART1 на Arduino D0/D1 замість чинного USART2 VCP: потребував би іншого routing/bridge; збережено PA2/PA3. SWD PA13/14 не використовується.
DMA не налаштований; конфлікту каналів DMA немає, а Wokwi C031 DMA не моделює. Таймери/PWM не потрібні ЛР2. Камери у формі B немає. На спільному SPI активний лише один CS, MISO іншого slave у high-Z. Fault-switch розриває лише MISO RFID; його вивід 3 не під’єднано, замикання виходу SD на GND немає.

## Збирання та вимірювання
Підключити GND/живлення, SPI SCK/MOSI спільно, MISO SD безпосередньо до PB4, MISO RFID через контакти 1–2 fault-switch. Під’єднати окремі CS і RST, потенціометр та протилежні контакти кожної кнопки (1.l/2.l). Вивід 3 fault-switch не під’єднувати. Створити лише окремий sim/demo.img командою make prepare-sd; користувацьку картку не використовувати. Сектор127 — bring-up, LBA128–511 зарезервовані під майбутній журнал, він ще не реалізований.

Аналізатор SPI: D0 SCK, D1 MOSI, D2 MISO, D3 RFID_CS, D4 SD_CS, D5 UART_TX, D6 ADC_SAMPLE, D7 CAPTURE; trigger level HIGH D7, 5000000 переходів (≈45 MB буфера). GPIO probe mode перепід’єднує лише канали аналізатора, не пристрої. Момент запису задає capture; selftest повторює читання ID та SD-init для відтворюваних транзакцій. Нові VCD і SD demo відокремлені від збережених docs/captures та docs/evidence.
