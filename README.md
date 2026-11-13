# OxyArmour

Monitor de respiración y temperatura para el proyecto de ExpoAndes (Ingeniería, Uniandes).

Una ESP32 en el pecho lee un acelerómetro MPU6050 (respiración) y un termómetro DS18B20, y manda los datos por Bluetooth (BLE). Esta página los recibe, filtra los datos con movimiento y calcula las respiraciones por minuto.

**Abrir el monitor:** https://sebboli029.github.io/oxyarmour

## Requisitos
- Google Chrome o Microsoft Edge en computador, o Chrome en Android. **No funciona en iPhone** (iOS no tiene Web Bluetooth).
- La ESP32 encendida y a unos 10 m como máximo.

## Archivos
- `index.html`: el monitor (página web).
- `esp32_oxyarmour.ino`: código de la ESP32 (Arduino IDE, placa DOIT ESP32 DEVKIT V1).

## Conexiones de la ESP32
| Sensor | Pin del sensor | Pin de la ESP32 |
| --- | --- | --- |
| MPU6050 | VCC / GND / SDA / SCL | VIN / GND / D21 / D22 |
| DS18B20 | + / − / S | 3V3 / GND / D4 |

Prototipo académico: no es un dispositivo médico.
