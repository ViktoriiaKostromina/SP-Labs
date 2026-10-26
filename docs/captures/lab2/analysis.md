# Аналіз власних записів ЛР2

## Вхідні дані та прошивки

Збережено оригінальні native Wokwi VCD з усіма пристроями, а не синтетичні сигнали. Час — MCU/simulation clock. Host arrival time UART не використано як тривалість операції.

| Оригінальний VCD | ELF | SHA-256 VCD |
|---|---|---|
| spi-sd-write-adc.vcd | firmware-rxirq: aab91619a32af1f7d2ab57fbf3bce8bd20b9a81fbdc32ddfe24d82de0c6f4d17 | 5340b06da501b9432d88212877ed8802d346f5e99ecf53d7a8d5f2cd9b4af7d3 |
| spi-rfid-nfc-rejected.vcd | firmware-final: 90c1e82b59daffc40ba134e9f759618dc771d099f606c829ec049e270156140e | 22bf13c9b6599c43bd5a583e5a30dc30a615e3817bd69959e821c912b8b8c01e |
| gpio-buttons-adc.vcd | firmware-final: 90c1e82b59daffc40ba134e9f759618dc771d099f606c829ec049e270156140e | dda203f751f9ecc1362a5e8090b4844cb22293ceea2df5a282f7f1569df167bf |

Бінарники, map, схема і конфігурація кожної ідентичності — ../../evidence/lab2/firmware-*/identity.json. Ранні діагностичні записи не приписуються фінальному ELF. spi-rfid-startup-nfc.vcd передує перевірці узгодженості UID: це діагностика, а не приймання UID7.

SPI probes: D0 SCK, D1 MOSI, D2 MISO, D3 RFID_CS, D4 SD_CS, D5 UART_TX, D6 ADC_SAMPLE, D7 CAPTURE. Тригер SPI — level HIGH D7, GPIO — level HIGH D3. Буфер5000000 переходів ≈45MB. Автоматичне boot вікно1с та bounded capture1..10000мс. GPIO probes: D0 PAUSE, D1 CLOSE, D2 ADC_SAMPLE, D3 CAPTURE, D4 LED, D5 UART_TX, D6 RFID_CS, D7 UART_RX. Збережено diagram-gpio.json. Перемикаються тільки probes, не проводи пристроїв.

## Налаштування і точність

SPI1 375 кГц, mode0, 8 bit MSB first; вибірка на rising SCK, CS active-low, кожен slave декодується зі своїм CS. UART 115200 8N1, LSB first, idle HIGH. ADC marker HIGH від HAL_ADC_Start до Poll/GetValue/Stop; він не вимірює чисту апертуру ADC. Кнопки active-low, poll5 мс, stable debounce25 мс.

Native VCD має одиницю1 нс, але наявна квантизація переходів25 нс; це не доводить фізичної точності1 нс. SCK: 32865 внутрішніх періодів 2650/2675 нс, median2675 нс, розрахунок2666,667 нс. Для PulseView імпортовано VCD із downsample250: .sr має4 МГц, крок250 нс, близько10,7 samples/clock. Точні числові інтервали нижче походять з native VCD.

Похідні *-window та *-detail лише вибирають інтервал, зберігають фактичні початкові рівні й віднімають спільний origin. Кожен має .provenance.json із SHA джерела, start/end і SHA результату. Переходи не домальовано. Початок GPIO window — 351400 мс native часу.

Між послідовними RFID CS у spi-sd-write-adc.vcd виміряно1783 проміжки end→start до100мс: min12,500/median13,250/max68650,300 мкс. Вони охоплюють короткі регістрові операції та паузи polling; capture-off gaps≥100мс виключено. Це не період SCK.

## Ручний розбір SPI та SD

Читання VersionReg0x37: адреса read = 0x80|(0x37<<1)=0xEE. Перші8 rising SCK на MOSI:11101110; наступний byte00. MISO повертає00, потім10010010=0x92. RFID_CS безперервно LOW198,525 мкс; 16 чистих тактів за375 кГц становлять42,667 мкс. Решта — міжбайтові паузи та HAL; не слід рахувати швидкість SCK за повним CS.

SD init: CMD0 → CMD8 з echo000001AA → CMD55/ACMD41 → CMD58, OCR C0FF8000. CCS=1 означає block addressing; SDSC path із CMD16/byte addressing перевіряється host.

CMD24 має header58 00 00 00 7F 01:0x58=0x40|24, LBA127. Після прийнятного R1 драйвер передає токенFE,512байтів і CRC; response&0x1F=05 означає прийняття, далі busy до FF. CMD17 читає той самий сектор: токенFE,512байтів,CRC. CS: CMD24 56,508525 мс, CMD17 54,4276 мс; разом110,936125 мс. У відповідному uid4-hold/uart.txt sdwrite92 має elapsed_ms=113 і verified_bytes=512. Різниця охоплює ділянки поза CS. Значення108 мс із ранньої діагностики не використовується для цього порівняння.

Початкові байти сектору:5C6D7E8FA0B1C2D3; CRC32=12D38871. Усі512байтів порівняно з seed92. Після Stop/writeback і свіжого запуску sdread повертає той самий CRC. sd-persisted.img.gz містить окремий8MiB образ; sd-persistence.json зберігає SHA образу та сектору. Це persistence після перезапуску моделі, а не доказ атомарності SD при power loss.

## UART, GPIO і ADC

UART decoder показує42 55 54 54 4F 4E 20 74 → ASCII «BUTTON t». gpio-buttons-uart.raw і незалежний serial collector узгоджуються щодо PAUSE t_ms351645, CLOSE351880, count1/1.

| Подія | Bounce | Перший falling → початок UART | Останній LOW → UART |
|---|---|---|---|
| PAUSE |105 переходів,1,91335 мс|26,6865 мс|24,77315 мс|
| CLOSE |193 переходи,2,264425 мс|28,820275 мс|26,55585 мс|

Це два приклади, не статистичний worst case майбутнього FR. Debounce працює на дискретних вибірках; останній фізичний bounce може відбутися після першої спостереженої LOW вибірки. Тому перший інтервал24,77315 мс не означає відсутності25мс software debounce.

ADC endpoints: raw0→0mV/code1;2050→1652mV/code3;4095→3300mV/code4. Формула(round) (raw×3300+2047)/4095, nominal Vref3300mV; фільтр mean8, пороги1024/2048/3072, гістерезис±32counts. Це не калібрування фізичного ADC.

GPIO VCD:1076 ADC pulses,62,900–65,175 мкс, mean65,15776 мкс.1074 внутрішньовіконних release intervals: min4,465575/mean10,196497/max30,7863 мс. Відомі capture-off gaps>100мс виключено, вони не є пропущеними ADC. Блокувальний RF/UART розширює номінальні10мс; збережені дані не доводять deadline усіх майбутніх сесій. Повний розрахунок: gpio-buttons-adc/gpio-metrics.json.

Навмисний80-символьний UART рядок відкинуто: dropped1,rx_lost0, наступні status/help валідні. Ліміт рядка63 ASCII bytes, обробка≤32bytes/прохід, RX ring256B.

## Аномалія RFID

Native NFC Tag показує04:11:22:33:44:55:66, ATQA4400 оголошує double-size UID, але FIFO повертає0411223304 без cascade tag88, SAK00FE51. Це не узгоджена UID7 послідовність. Попередній драйвер міг видати обрізаний04112233; фінальний перевіряє ATQA/довжину, дає RFID_FAULT PROTOCOL і не видає TOKEN. nfc-rejected-uart.raw декодовано безпосередньо з VCD і містить ранню подію; прямий колектор nfc-rejection-final підключився пізніше. Точний captured vector включено в tests/test_devices.c. Правильні UID4/7 cascades перевірені fake transport на host, не на фізичному MFRC522.

Після HALT модель не підтвердила WUPA для безперервно утримуваної картки. Власне remove/re-present дає UID4; token gate три NO_CARD і відсутність фальшивого removal за fault перевірено host. Native held-card підтримку не слід позначати pass.

## Відтворення аналізу

Після звичайного cd до кореня репозиторію, збережені native дані можна аналізувати без нового запуску:

```sh
python3 tools/decode_trace.py docs/captures/lab2/spi-sd-write-adc.vcd --out build/reanalysis-spi
python3 tools/measure_gpio.py docs/captures/lab2/gpio-buttons-adc.vcd --out build/reanalysis-gpio
```

У PulseView відкрити rfid-id-detail.sr, uart-button-detail.sr або gpio-buttons-window.sr, потім відповідний .pvs через Load Session Setup. Налаштування декодерів збережені в .pvs; вони не замінюють зразки. spi-sd-annotations.txt, spi-sd-mosi.txt, spi-sd-miso.txt і uart-button-annotations.txt — текстовий результат sigrok; Python decoder є незалежною перевіркою native edges.

Для нового запису: зупинити власний simulator, вибрати python3 tools/select_probe.py spi або gpio, зібрати й відкрити свіжу схему, зберегти новий файл у sim. Не перезаписувати виміряні VCD. Capture gate HIGH автоматично1с на старті; команда capture1..10000 відкриває обмежене вікно. Немає годинного S2 чи реалізованої RTOS.
