import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, sensor, text_sensor
from esphome.const import (
    CONF_ID,
    CONF_VOLTAGE,
    CONF_CURRENT,
    CONF_POWER,
    CONF_FREQUENCY,
    CONF_UPDATE_INTERVAL,
    DEVICE_CLASS_VOLTAGE,
    DEVICE_CLASS_CURRENT,
    DEVICE_CLASS_POWER,
    DEVICE_CLASS_FREQUENCY,
    DEVICE_CLASS_ENERGY,
    STATE_CLASS_TOTAL_INCREASING,
    UNIT_VOLT,
    UNIT_AMPERE,
    UNIT_HERTZ,
    UNIT_KILOWATT_HOURS,
)

CODEOWNERS = ["@your-username"]
DEPENDENCIES = ["uart"]
AUTO_LOAD = ["sensor", "text_sensor"]

mercury206_ns = cg.esphome_ns.namespace("mercury206")
Mercury206Component = mercury206_ns.class_(
    "Mercury206Component", cg.PollingComponent, uart.UARTDevice
)

CONF_SERIAL_NUMBER = "serial_number"
CONF_REQUEST_TIMEOUT = "request_timeout"
CONF_RETRY_COUNT = "retry_count"
CONF_ENERGY_TARIFF_1 = "energy_tariff_1"
CONF_ENERGY_TARIFF_2 = "energy_tariff_2"
CONF_ENERGY_TARIFF_3 = "energy_tariff_3"
CONF_ENERGY_TOTAL = "energy_total"
CONF_CURRENT_TARIFF = "current_tariff"
CONF_DATETIME = "datetime"
CONF_CRC_ERRORS = "crc_errors"
CONF_LAST_UPDATE = "last_update"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Mercury206Component),
            cv.Required(CONF_SERIAL_NUMBER): cv.uint32_t,
            cv.Optional(CONF_REQUEST_TIMEOUT, default="500ms"): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_RETRY_COUNT, default=2): cv.int_range(min=0, max=5),
            
            cv.Optional(CONF_VOLTAGE): sensor.sensor_schema(
                unit_of_measurement=UNIT_VOLT,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_VOLTAGE,
            ),
            cv.Optional(CONF_CURRENT): sensor.sensor_schema(
                unit_of_measurement=UNIT_AMPERE,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_CURRENT,
            ),
            cv.Optional(CONF_POWER): sensor.sensor_schema(
                unit_of_measurement="kW",
                accuracy_decimals=3,
                device_class=DEVICE_CLASS_POWER,
            ),
            cv.Optional(CONF_FREQUENCY): sensor.sensor_schema(
                unit_of_measurement=UNIT_HERTZ,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_FREQUENCY,
            ),
            cv.Optional(CONF_ENERGY_TARIFF_1): sensor.sensor_schema(
                unit_of_measurement=UNIT_KILOWATT_HOURS,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_ENERGY,
                state_class=STATE_CLASS_TOTAL_INCREASING,
            ),
            cv.Optional(CONF_ENERGY_TARIFF_2): sensor.sensor_schema(
                unit_of_measurement=UNIT_KILOWATT_HOURS,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_ENERGY,
                state_class=STATE_CLASS_TOTAL_INCREASING,
            ),
            cv.Optional(CONF_ENERGY_TARIFF_3): sensor.sensor_schema(
                unit_of_measurement=UNIT_KILOWATT_HOURS,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_ENERGY,
                state_class=STATE_CLASS_TOTAL_INCREASING,
            ),
            cv.Optional(CONF_ENERGY_TOTAL): sensor.sensor_schema(
                unit_of_measurement=UNIT_KILOWATT_HOURS,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_ENERGY,
                state_class=STATE_CLASS_TOTAL_INCREASING,
            ),
            cv.Optional(CONF_CURRENT_TARIFF): sensor.sensor_schema(
                accuracy_decimals=0,
            ),
            cv.Optional(CONF_DATETIME): text_sensor.text_sensor_schema(),
            cv.Optional(CONF_CRC_ERRORS): sensor.sensor_schema(
                accuracy_decimals=0,
                entity_category="diagnostic",
            ),
            cv.Optional(CONF_LAST_UPDATE): text_sensor.text_sensor_schema(
                entity_category="diagnostic",
            ),
        }
    )
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend(cv.polling_component_schema("30s"))
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    
    cg.add(var.set_serial_number(config[CONF_SERIAL_NUMBER]))
    cg.add(var.set_request_timeout(config[CONF_REQUEST_TIMEOUT]))
    cg.add(var.set_retry_count(config[CONF_RETRY_COUNT]))
    
    if CONF_VOLTAGE in config:
        sens = await sensor.new_sensor(config[CONF_VOLTAGE])
        cg.add(var.set_voltage_sensor(sens))
    if CONF_CURRENT in config:
        sens = await sensor.new_sensor(config[CONF_CURRENT])
        cg.add(var.set_current_sensor(sens))
    if CONF_POWER in config:
        sens = await sensor.new_sensor(config[CONF_POWER])
        cg.add(var.set_power_sensor(sens))
    if CONF_FREQUENCY in config:
        sens = await sensor.new_sensor(config[CONF_FREQUENCY])
        cg.add(var.set_frequency_sensor(sens))
    if CONF_ENERGY_TARIFF_1 in config:
        sens = await sensor.new_sensor(config[CONF_ENERGY_TARIFF_1])
        cg.add(var.set_energy_t1_sensor(sens))
    if CONF_ENERGY_TARIFF_2 in config:
        sens = await sensor.new_sensor(config[CONF_ENERGY_TARIFF_2])
        cg.add(var.set_energy_t2_sensor(sens))
    if CONF_ENERGY_TARIFF_3 in config:
        sens = await sensor.new_sensor(config[CONF_ENERGY_TARIFF_3])
        cg.add(var.set_energy_t3_sensor(sens))
    if CONF_ENERGY_TOTAL in config:
        sens = await sensor.new_sensor(config[CONF_ENERGY_TOTAL])
        cg.add(var.set_energy_total_sensor(sens))
    if CONF_CURRENT_TARIFF in config:
        sens = await sensor.new_sensor(config[CONF_CURRENT_TARIFF])
        cg.add(var.set_tariff_sensor(sens))
    if CONF_DATETIME in config:
        sens = await text_sensor.new_text_sensor(config[CONF_DATETIME])
        cg.add(var.set_datetime_sensor(sens))
    if CONF_CRC_ERRORS in config:
        sens = await sensor.new_sensor(config[CONF_CRC_ERRORS])
        cg.add(var.set_crc_errors_sensor(sens))
    if CONF_LAST_UPDATE in config:
        sens = await text_sensor.new_text_sensor(config[CONF_LAST_UPDATE])
        cg.add(var.set_last_update_sensor(sens))
