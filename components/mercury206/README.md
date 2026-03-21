# ⚡ ESPHome Mercury 206

[![ESPHome](https://img.shields.io/badge/ESPHome-2026.3.0-blue.svg)](https://esphome.io)
[![Platform](https://img.shields.io/badge/Platform-ESP8266-green.svg)](https://www.espressif.com)
[![Protocol](https://img.shields.io/badge/Protocol-Mercury_206-orange.svg)](https://incotex.ru)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

> **Интеграция счётчиков электроэнергии «Меркурий 206» в Home Assistant через ESPHome с использованием интерфейса RS485.**

---

## 🌟 Возможности

| Функция | Описание |
|---------|----------|
| ⚡ **Мгновенные значения** | Напряжение (В), ток (А), активная мощность (кВт) |
| 💰 **Тарифный учёт** | Чтение энергии по тарифам T1, T2, T3 + суммарное значение |
| 📅 **Дата и время** | Синхронизация времени со счётчика |
| 📈 **Частота сети** | Мониторинг частоты (45–55 Гц) |
| 🔍 **Диагностика** | Счётчик ошибок CRC, время последнего успешного опроса |
| 🔄 **Автоматический опрос** | Циклический опрос всех параметров с ротацией команд |
| 🛡️ **Защита от ошибок** | Повторные запросы при сбоях связи, валидация данных |

---

## 🛠️ Требуемое оборудование

| Компонент | Модель / Примечание |
|-----------|---------------------|
| **Микроконтроллер** | ESP8266 (Wemos D1 Mini) или ESP32 |
| **RS485 модуль** | MAX485 / SP3485 / MAX3485 (3.3V или 5V) |
| **Счётчик** | Меркурий 206.02 (или другая модификация с RS485) |
| **Питание** | 5V 1A (для ESP и модуля RS485) |
| **Резистор** | 120 Ом (терминирующий, опционально, для длинных линий) |

### 📐 Схема подключения

```
┌─────────────┐         ┌─────────────┐         ┌─────────────┐
│   ESP8266   │         │   MAX485    │         │ Меркурий 206│
│   D1 Mini   │         │   Module    │         │   RS485     │
├─────────────┤         ├─────────────┤         ├─────────────┤
│ GPIO4 (D2)  │────────>│ DI / TX     │────────>│ A+          │
│ GPIO5 (D1)  │────────>│ RO / RX     │────────>│ B-          │
│ GND         │────────>│ GND         │────────>│ GND         │
│ 5V          │────────>│ VCC         │         │             │
│ GPIO12*     │────────>│ DE + RE     │         │(опционально)│
└─────────────┘         └─────────────┘         └─────────────┘
```

> ⚠️ **Важно:**
> 1. Для надёжной связи добавьте терминирующий резистор **120 Ом** между контактами **A+** и **B-** на конце линии.
> 2. Обязательно соедините **земли (GND)** всех устройств.
> 3. Если ваш модуль имеет авто-направление (например, SP3485), контакт `GPIO12` можно не подключать.

---

## 📦 Установка

### Вариант 1: Через ESPHome Dashboard (рекомендуется)

1. Откройте **ESPHome** в Home Assistant.
2. Нажмите **"+ NEW DEVICE"** → **"Continue"**.
3. Введите имя устройства: `mercury-206`.
4. Скопируйте содержимое [`mercury-206.yaml`](mercury-206.yaml) в конфигурацию устройства.
5. Создайте папку `components/mercury206/` в директории конфигурации ESPHome и поместите туда файлы компонента:
   - `__init__.py`
   - `mercury206.h`
   - `mercury206.cpp`
6. Нажмите **"INSTALL"** → выберите **"Manual Download"** или **"Upload via USB"**.

### Вариант 2: Через командную строку

```bash
# Клонировать репозиторий
git clone https://github.com/YOUR_USERNAME/esphome-mercury206.git
cd esphome-mercury206

# Установить ESPHome (если не установлен)
pip install esphome

# Проверить конфигурацию
esphome config mercury-206.yaml

# Скомпилировать и загрузить
esphome compile mercury-206.yaml
esphome upload mercury-206.yaml
```

---

## ⚙️ Конфигурация

### Базовые настройки

```yaml
mercury206:
  id: mercury_meter
  uart_id: uart_mercury
  serial_number: 37206899      # ← Замените на серийный номер вашего счётчика!
  update_interval: 5s          # Интервал опроса (рекомендуется 5–30с)
  request_timeout: 300ms       # Таймаут ожидания ответа
  retry_count: 2               # Количество повторных попыток при ошибке
```

### 🔍 Как найти серийный номер

Серийный номер указан:
- На лицевой панели счётчика (под штрих-кодом);
- В паспорте изделия;
- Командой чтения через терминал: `CMD=0x2F`.

Формат: 8-значное десятичное число (например, `37206899`).

---

## 📋 Доступные сенсоры

| Сенсор | Entity ID | Ед. изм. | Описание |
|--------|-----------|----------|----------|
| Напряжение | `sensor.mercury_voltage` | V | Мгновенное напряжение сети |
| Ток | `sensor.mercury_current` | A | Мгновенный ток нагрузки |
| Мощность | `sensor.mercury_power` | kW | Активная мощность |
| Частота | `sensor.mercury_frequency` | Hz | Частота сети |
| Энергия T1 | `sensor.mercury_t1_energy` | kWh | Потребление по тарифу 1 |
| Энергия T2 | `sensor.mercury_t2_energy` | kWh | Потребление по тарифу 2 |
| Энергия T3 | `sensor.mercury_t3_energy` | kWh | Потребление по тарифу 3 |
| Энергия сумма | `sensor.mercury_total_energy` | kWh | Суммарное потребление |
| Тариф номер | `sensor.mercury_tariff_num` | — | Текущий активный тариф (1–4) |
| Дата-время | `sensor.mercury_datetime` | — | Время со счётчика (формат `ГГГГ-ММ-ДД ЧЧ:ММ:СС`) |
| Ошибки CRC | `sensor.mercury_crc_errors` | — | Счётчик ошибок проверки CRC |
| Последнее обновление | `sensor.mercury_last_update` | — | Время последнего успешного опроса |

---

## 📡 Протокол обмена

Проект реализует **нативный протокол Меркурий 206** (не Modbus).

### Структура пакета

| Поле | Длина | Описание |
|------|-------|----------|
| `ADDR` | 4 байта | Сетевой адрес (серийный номер, big-endian) |
| `CMD` | 1 байт | Код команды |
| `DATA` | 0–17 байт | Данные (опционально, зависит от команды) |
| `CRC` | 2 байта | Контрольная сумма CRC-16 (полином `0xA001`, LSB first) |

### Используемые команды

| CMD | Назначение | Длина ответа |
|-----|-----------|--------------|
| `0x27` | Чтение тарифов (T1, T2, T3) | 23 байта |
| `0x63` | Чтение U, I, P | 14 байт |
| `0x81` | Чтение частоты и текущего тарифа | 17 байт |
| `0x21` | Чтение даты и времени | 14 байт |

### Формат данных

- **BCD-кодирование**: каждая тетрада байта представляет одну десятичную цифру.
- **CRC-16**: полином `0xA001`, младший байт первым.
- **Порядок байт**: старшие байты вперёд (big-endian) для многобайтовых значений.

---

## 🔍 Отладка

### Включение подробных логов

```yaml
logger:
  level: DEBUG
  logs:
    mercury206: DEBUG
    uart: DEBUG
    component: ERROR
```

### Просмотр логов

```bash
esphome logs mercury-206.yaml
```

### Типичные проблемы и решения

| Проблема | Возможная причина | Решение |
|----------|------------------|---------|
| Нет данных | Неверное подключение A+/B- | Поменяйте местами провода A+ и B- |
| Ошибки CRC | Помехи в линии, неверный CRC | Проверьте качество соединения, увеличьте `request_timeout` |
| Таймауты | Медленный ответ счётчика | Увеличьте `request_timeout` до 500–1000 мс |
| Неправильные значения | Неверный серийный номер | Проверьте `serial_number` в конфигурации |
| Данные пропадают | Слишком короткий `timeout` фильтра | Увеличьте `timeout` в фильтрах сенсоров |

---

## 🏠 Интеграция с Home Assistant

### Пример дашборда (Lovelace)

```yaml
type: entities
title: 📊 Меркурий 206
entities:
  - entity: sensor.mercury_voltage
    name: ⚡ Напряжение
  - entity: sensor.mercury_current
    name: 🔌 Ток
  - entity: sensor.mercury_power
    name: 💡 Мощность
  - entity: sensor.mercury_frequency
    name: 📈 Частота
  - type: divider
  - entity: sensor.mercury_t1_energy
    name: 💰 Тариф 1
  - entity: sensor.mercury_t2_energy
    name: 💰 Тариф 2
  - entity: sensor.mercury_total_energy
    name: 📊 Сумма
  - type: divider
  - entity: sensor.mercury_tariff_num
    name: 🔢 Текущий тариф
  - entity: sensor.mercury_datetime
    name: 🕐 Время счётчика
  - type: divider
  - entity: sensor.mercury_crc_errors
    name: ⚠️ Ошибки CRC
  - entity: sensor.mercury_last_update
    name: 🔄 Последнее обновление
```

### 💡 Учёт потребления через `utility_meter`

```yaml
utility_meter:
  mercury_daily_t1:
    source: sensor.mercury_t1_energy
    cycle: daily
  mercury_daily_t2:
    source: sensor.mercury_t2_energy
    cycle: daily
  mercury_monthly_total:
    source: sensor.mercury_total_energy
    cycle: monthly
```

---

## 📁 Структура проекта

```
esphome-mercury206/
├── mercury-206.yaml          # Основная конфигурация YAML
├── components/
│   └── mercury206/
│       ├── __init__.py       # Инициализация компонента для ESPHome
│       ├── mercury206.h      # Заголовочный файл C++
│       └── mercury206.cpp    # Реализация компонента
├── docs/
│   └── mercury206-protocol.pdf  # Документация по протоколу│   
└── README.md                 # Этот файл
```

---

## 🤝 Вклад в проект

1. Fork репозиторий.
2. Создайте ветку для новой функции: `git checkout -b feature/amazing-feature`.
3. Закоммитьте изменения: `git commit -m 'Add amazing feature'`.
4. Push в ветку: `git push origin feature/amazing-feature`.
5. Откройте Pull Request.

Перед отправкой убедитесь, что:
- Код компилируется без ошибок;
- Добавлены комментарии к новым функциям;
- Обновлена документация (при необходимости).

---

## 📄 Лицензия

Этот проект распространяется под лицензией **MIT**. См. файл [LICENSE](LICENSE) для деталей.

```
MIT License

Copyright (c) 2026 Dmitry Kadyshev

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

---

## ⚠️ Отказ от ответственности

- Проект **не является официальным продуктом** ЗАО «НПК "Инкотекс"».
- Используйте на **свой страх и риск**.
- Автор **не несёт ответственности** за возможные повреждения оборудования или некорректные показания.
- Перед подключением убедитесь в соответствии напряжения питания и полярности сигналов.
- Работа с электросетью 220В опасна для жизни — соблюдайте технику безопасности.

---

## 🙏 Благодарности

- **ESPHome Community** — за отличную платформу и документацию.
- **Incotex** — за публичную документацию по протоколу Меркурий.
- **Home Assistant Community** — за поддержку, тестирование и идеи.
- **@RocketFox** — за оригинальный код, послуживший основой для этого проекта.

---




### ⭐ Если проект был полезен — поставьте звезду!

**Happy metering!** 🔌📊

