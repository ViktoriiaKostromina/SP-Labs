# Перевірка обсягу ЛР1–2

Статуси спираються на докази відповідного рівня: host, ARM, симуляція або фізична плата.

| Етап | Результат | Стан і доказ |
|---|---|---|
| ЛР1 | Тема, ≥8 FR, компоненти, альтернативи, числові критерії | Верифіковано документацію: PRD.md, hardware-components.md; це не виконані FR продукту. |
| ЛР1 | Docker, Git, дві чисті однакові ARM-збірки | Верифіковано: evidence/lab1/build1.log, build2.log, hash1.txt, hash2.txt. |
| ЛР1 | Власні UART tick0–4 і LED | Верифіковано Wokwi: evidence/lab1/uart-restart.txt, uart-led.vcd, results.json. |
| ЛР1 | PDF, джерела, ≥5 проблем, CubeMX pinout/clock | Верифіковано: reports/Лабораторна_робота_1_Вікторія_Костроміна_ІО-36.pdf; screenshots/lab1-*.png. |
| ЛР2 | Pinout, AF/роз’єми, конфлікти, живлення, ≥2 альтернативи | Верифіковано обґрунтування: pinout.md; струми не виміряні. |
| ЛР2 | Драйвери, одиниці, фільтри, self-test, спільний bring-up | Верифіковано host + ARM + native Wokwi в доступному обсязі: evidence/lab2/README.md. |
| ЛР2 | SPI/UART/GPIO/ADC-marker VCD, декодери, ручні байти, аномалія, таймінги | Верифіковано: captures/lab2/analysis.md, VCD, .sr/.pvs, provenance JSON. ADC marker охоплює HAL. |
| ЛР2 | ADC/timeout/ID/CRC, storage contract, captured vector, coverage | Верифіковано host: evidence/lab2/host-tests-final.log, coverage-summary.json; 612 checks + bring-up, 99,3% lines/90,3% branches. |
| ЛР2 | Шари, -Wall -Wextra -Werror, CI build/test | Локальні перевірки пройдено; ci.yml запускає build/test на PR і push lab1/lab2. Віддалений результат фіксується окремо. |
| ЛР2 | PDF, власні знімки, ≥5 проблем і джерела | Верифіковано: reports/Лабораторна_робота_2_Вікторія_Костроміна_ІО-36.pdf, screenshots/lab2-*.png. |
| Межа моделі | Native UID7 та WUPA утримуваної картки | UID7 неузгоджений і відхиляється; правильні каскади й token gate — host. Native підтримку не встановлено. |
| Фізична плата | Живлення/RF/аналогова точність/SD power loss | Недоступно: фізичного стенда немає; фізичні FR не мають pass. |
| Підготовка ЛР3–4 | S1/S2/S3, persistence plan, метрики, порівняння | Підготовлено future-lab3-lab4.md; сесій/ОС і виміряної baseline немає. |
| Доставка | Порівняння з доступними роботами | Окремий локальний аудит Work/private/comparison поза репозиторієм. |
| Зовнішнє | Два фактичні відео 2–3 хв, Drive-посилання, PR | Залишається студентці/користувачу; сценарій не підтверджує запис. |

Звіти й технічні матеріали підготовлено з наведеними межами. Повна зовнішня здача потребує останнього рядка; прийняття обмежень викладачем не встановлено.
