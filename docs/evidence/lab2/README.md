# Походження та статус доказів ЛР2

Фактичні перевірки відбулися 06.11.2026. Дати Git-комітів не є датами вимірювань. origin.json містить фактичний UTC запуску, команди, hash raw і ELF на диску. Сам hash файла наприкінці не доводить, що саме цей ELF був завантажений у раніше відкрите вікно; для фінальних записів використано свіжий запуск і узгоджений стартовий банер.

| Файли / каталог | Призначення й статус |
|---|---|
| firmware-rxirq/identity.json | Архів ELF aab91619…f4d17, HEX/BIN/map/diagram/wokwi для валідного SPI/SD запису. |
| uid4-hold | UID01020304 та SD seed92: elapsed113мс,512 bytes,CRC12D38871. Команди і startup_ready_observed у origin.json. |
| firmware-final/identity.json | Архів ELF90c1e82b…65140e та конфігурація для фінальних GPIO/ADC/UART і NFC rejection. |
| gpio-startup, gpio-min-buttons | Старт та кнопки; повний native GPIO VCD і незалежне UART декодування лежать ../../captures/lab2. |
| adc-endpoints | Фактичні0/2050/4095 ADC і mV/code. |
| uart-parser | Навмисний довгий рядок, dropped1, подальші status/help,rx_lost0. |
| nfc-rejection-final | Прямий колектор після раннього startup; не містить першого fault. Рання подія PROTOCOL походить з ../../captures/lab2/nfc-rejected-uart.raw і native VCD. |
| sd-persisted.img.gz, sd-persistence.json | Збережений окремий8MiB образ, SHA образу/сектору, host перевірка всіх512байтів seed92. |
| build-final.log, host-tests-final.log, coverage-summary.json | Остаточні ARM/host перевірки. Host:612 checks+bring-up;99,3%lines/90,3%branches. |
| diagram-lint-final.txt | Перевірка моделі, не симуляція та не фізичний тест. |

## Діагностика, яку не слід видавати за фінальне приймання

- first-bringup: збережене відкрите вікно мало стару схему; ADC/RFID дані не є фінальною конфігурацією. fresh-bringup — перевірка після повторного відкриття.
- spi-diagnostic, spi-diagnostic-stop, byte-access-start: дослідження VersionReg00/FFFF і явного byte exchange. Положення fault-switch у першій діагностиці не встановлене достатньо для однозначного доведення причини. Packed16-bit HAL RX — імовірне пояснення; workaround перевірено за92.
- rfid-length-diagnostic, burst-card-trace, fifo-burst-check: помилки довжини/BCC, виправлення FIFO burst та IdleIRq. Це історія помилок, а не окремі успішні UID7 тести.
- startup-uid7 містить попереднє обрізане04112233; після аналізу ATQA цей результат визнано невалідним UID7.
- uid7-start має історичну назву: фактично тестувалася UID4 картка. Назва каталогу не підтверджує UID7.
- card-hold-check виявив обмеження WUPA моделі. Прогін збережено; native безперервне утримання не підтверджено.
- rxirq-fixed-start та проміжні build/host logs — хронологічні перевірки, не підміна final logs. Частина ранніх діагностичних ELF не була архівована: їхні hashes не дають змоги відтворити точний бінарник з цього каталогу. Два архівовані фінальні вимірювальні ELF позначено окремо вище.
- diagram-lint-initial-invalid.txt і host-tests-initial.log зберігають невдалі перевірки, зокрема aliases та відсутній libc6-dev.

Сигнали, параметри декодерів, ручні байти, sourceSHA і похідні вікна: [аналіз](../../captures/lab2/analysis.md). Колектор UART працює через RFC2217 localhost4049; host_elapsed_s не є MCU elapsed. Ці короткі перевірки не є годинним S2.

Фізичний стенд відсутній. RF, живлення, ADC analog settling і power-loss atomicity SD не перевірено. Правильний UID7/помилки шин перевірені host як контракти драйверів; це не замінює апаратну перевірку. SHA-256 файлів доказів — sha256-manifest.txt (сам manifest не включено).
