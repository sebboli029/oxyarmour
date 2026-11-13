// ESP32 DevKit V1 + MPU6050 (I2C: D21/D22) + DS18B20 (D4)
// Envia los datos por BLE (nombre: OxyArmour) y por el Monitor Serie (115200)
// Formato de cada linea: ax,ay,az,temp   (ax..az en g, temp en C)

#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>

#define PIN_DS18B20 4
#define MPU_ADDR    0x68

#define SERVICE_UUID "6e400001-b5a3-f393-e0a9-e50e24dcca9e"  // Nordic UART
#define TX_UUID      "6e400003-b5a3-f393-e0a9-e50e24dcca9e"  // notify

OneWire oneWire(PIN_DS18B20);
DallasTemperature ds(&oneWire);

BLECharacteristic* txChar;
bool conectado = false;

// Calibracion DS18B20: T_real = A * T_leida + B
const float A = 1.0, B = 0.0;

float temperatura = NAN;
unsigned long ultimoTemp = 0, ultimoEnvio = 0;
bool mpuOk = false;

class ServerCB : public BLEServerCallbacks {
  void onConnect(BLEServer*) override { conectado = true; }
  void onDisconnect(BLEServer* s) override {
    conectado = false;
    s->getAdvertising()->start();   // volver a anunciarse
  }
};

bool iniciarMPU() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); Wire.write(0x00);          // despertar
  if (Wire.endTransmission() != 0) return false;
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1C); Wire.write(0x00);          // rango +-2 g
  Wire.endTransmission();
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1A); Wire.write(0x05);          // filtro pasa bajos ~10 Hz
  Wire.endTransmission();
  return true;
}

bool leerAcel(float& ax, float& ay, float& az) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(MPU_ADDR, 6) != 6) return false;
  int16_t x = (Wire.read() << 8) | Wire.read();
  int16_t y = (Wire.read() << 8) | Wire.read();
  int16_t z = (Wire.read() << 8) | Wire.read();
  ax = x / 16384.0; ay = y / 16384.0; az = z / 16384.0;
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Wire.begin(21, 22);
  mpuOk = iniciarMPU();
  Serial.println(mpuOk ? "MPU6050 OK" : "ERROR: no se detecta el MPU6050 (revisa SDA/SCL/VCC)");

  ds.begin();
  ds.setResolution(12);
  ds.setWaitForConversion(false);   // no bloquear la lectura del acelerometro
  ds.requestTemperatures();
  Serial.print("DS18B20 encontrados: ");
  Serial.println(ds.getDeviceCount());

  BLEDevice::init("OxyArmour");
  BLEServer* server = BLEDevice::createServer();
  server->setCallbacks(new ServerCB());
  BLEService* svc = server->createService(SERVICE_UUID);
  txChar = svc->createCharacteristic(TX_UUID, BLECharacteristic::PROPERTY_NOTIFY);
  txChar->addDescriptor(new BLE2902());
  svc->start();
  BLEAdvertising* adv = BLEDevice::getAdvertising();
  adv->addServiceUUID(SERVICE_UUID);
  adv->start();
  Serial.println("BLE listo: busca 'OxyArmour'");
  Serial.println("ax,ay,az,temp");
}

void loop() {
  unsigned long ahora = millis();

  // Temperatura cada 1 s (sin bloquear)
  if (ahora - ultimoTemp >= 1000) {
    float t = ds.getTempCByIndex(0);
    temperatura = (t == DEVICE_DISCONNECTED_C) ? NAN : A * t + B;
    ds.requestTemperatures();
    ultimoTemp = ahora;
  }

  // Acelerometro y envio a 20 Hz
  if (ahora - ultimoEnvio >= 50) {
    ultimoEnvio = ahora;
    float ax = NAN, ay = NAN, az = NAN;
    if (mpuOk && !leerAcel(ax, ay, az)) { ax = ay = az = NAN; }

    char linea[64];
    snprintf(linea, sizeof(linea), "%.3f,%.3f,%.3f,%.2f", ax, ay, az, temperatura);
    Serial.println(linea);
    if (conectado) {
      txChar->setValue((uint8_t*)linea, strlen(linea));
      txChar->notify();
    }
  }
}
