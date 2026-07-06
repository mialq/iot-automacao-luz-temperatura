/*
  Automacao de Temperatura e Iluminacao com IoT
  Reconstrucao baseada no artigo academico do projeto original.

  Arquitetura desta reconstrucao:
    Arduino UNO <-> Serial USB <-> Node-RED <-> MQTT/Mosquitto <-> Smartphone

  Sensores:
    - DHT11: temperatura e umidade
    - LDR/modulo de luminosidade: leitura analogica
    - Sensor de vibracao/movimento: leitura digital (opcional)

  Atuadores:
    - Rele do ventilador
    - Rele da luminaria

  Biblioteca necessaria:
    - DHT sensor library (Adafruit)

  Observacao:
    Muitos modulos de rele sao "active LOW". Ajuste RELAY_ACTIVE_LOW
    caso o seu modulo funcione com logica invertida.
*/

#include <DHT.h>

// -------------------------
// Mapeamento de pinos
// -------------------------
constexpr uint8_t PIN_DHT = 2;            // O artigo original mostra o DHT11 no pino 2
constexpr uint8_t PIN_MOTION = 3;         // Sensor de vibracao/movimento (opcional)
constexpr uint8_t PIN_RELAY_FAN = 8;      // Rele do ventilador
constexpr uint8_t PIN_RELAY_LIGHT = 9;    // Rele da luminaria
constexpr uint8_t PIN_LIGHT_SENSOR = A0;  // LDR / sensor de luminosidade

constexpr uint8_t DHT_TYPE = DHT11;
constexpr bool RELAY_ACTIVE_LOW = true;

// Ajuste para o seu circuito:
// true  -> valor analogico menor significa ambiente mais escuro
// false -> valor analogico maior significa ambiente mais escuro
constexpr bool DARK_WHEN_BELOW_THRESHOLD = true;

// -------------------------
// Objetos e configuracoes
// -------------------------
DHT dht(PIN_DHT, DHT_TYPE);

float temperatureLimitC = 22.0;
float temperatureHysteresisC = 1.0;
int lightThreshold = 400;

bool autoTemperature = true;
bool autoLighting = true;

// O projeto original teve problema com o sensor de movimento.
// Nesta reconstrucao, ele e opcional por padrao.
bool motionRequiredForLight = false;

constexpr unsigned long SENSOR_INTERVAL_MS = 2000;
constexpr unsigned long MOTION_HOLD_MS = 30000;

unsigned long lastSensorReadAt = 0;
unsigned long lastMotionAt = 0;

float temperatureC = NAN;
float humidityPct = NAN;
int lightRaw = 0;
bool motionNow = false;
bool fanOn = false;
bool lightOn = false;

String serialBuffer;

// -------------------------
// Utilitarios
// -------------------------
void setRelay(uint8_t pin, bool on) {
  const uint8_t activeLevel = RELAY_ACTIVE_LOW ? LOW : HIGH;
  const uint8_t inactiveLevel = RELAY_ACTIVE_LOW ? HIGH : LOW;
  digitalWrite(pin, on ? activeLevel : inactiveLevel);
}

bool parseBoolean(String value, bool &result) {
  value.trim();
  value.toLowerCase();

  if (value == "1" || value == "true" || value == "on" || value == "ligar") {
    result = true;
    return true;
  }

  if (value == "0" || value == "false" || value == "off" || value == "desligar") {
    result = false;
    return true;
  }

  return false;
}

void sendAck(const String &command, bool ok, const String &message = "") {
  Serial.print(F("{\"type\":\"ack\",\"command\":\""));
  Serial.print(command);
  Serial.print(F("\",\"ok\":"));
  Serial.print(ok ? F("true") : F("false"));

  if (message.length() > 0) {
    Serial.print(F(",\"message\":\""));
    Serial.print(message);
    Serial.print(F("\""));
  }

  Serial.println(F("}"));
}

bool isDark(int rawValue) {
  return DARK_WHEN_BELOW_THRESHOLD
             ? rawValue < lightThreshold
             : rawValue > lightThreshold;
}

bool motionIsRecent() {
  if (motionNow) {
    return true;
  }

  return lastMotionAt > 0 && (millis() - lastMotionAt <= MOTION_HOLD_MS);
}

// -------------------------
// Leitura dos sensores
// -------------------------
void readSensors() {
  const float newHumidity = dht.readHumidity();
  const float newTemperature = dht.readTemperature();

  if (!isnan(newTemperature)) {
    temperatureC = newTemperature;
  }

  if (!isnan(newHumidity)) {
    humidityPct = newHumidity;
  }

  lightRaw = analogRead(PIN_LIGHT_SENSOR);
  motionNow = digitalRead(PIN_MOTION) == HIGH;

  if (motionNow) {
    lastMotionAt = millis();
  }
}

// -------------------------
// Logica de automacao
// -------------------------
void applyAutomation() {
  if (autoTemperature && !isnan(temperatureC)) {
    // Histerese evita ligar/desligar repetidamente perto do limite.
    if (!fanOn && temperatureC >= temperatureLimitC) {
      fanOn = true;
      setRelay(PIN_RELAY_FAN, fanOn);
    } else if (fanOn && temperatureC <= temperatureLimitC - temperatureHysteresisC) {
      fanOn = false;
      setRelay(PIN_RELAY_FAN, fanOn);
    }
  }

  if (autoLighting) {
    const bool dark = isDark(lightRaw);
    const bool motionOk = !motionRequiredForLight || motionIsRecent();
    const bool desiredLightState = dark && motionOk;

    if (desiredLightState != lightOn) {
      lightOn = desiredLightState;
      setRelay(PIN_RELAY_LIGHT, lightOn);
    }
  }
}

// -------------------------
// Telemetria serial em JSON
// -------------------------
void sendTelemetry() {
  Serial.print(F("{\"type\":\"telemetry\",\"temperature_c\":"));

  if (isnan(temperatureC)) {
    Serial.print(F("null"));
  } else {
    Serial.print(temperatureC, 1);
  }

  Serial.print(F(",\"humidity_pct\":"));
  if (isnan(humidityPct)) {
    Serial.print(F("null"));
  } else {
    Serial.print(humidityPct, 1);
  }

  Serial.print(F(",\"light_raw\":"));
  Serial.print(lightRaw);

  Serial.print(F(",\"dark\":"));
  Serial.print(isDark(lightRaw) ? F("true") : F("false"));

  Serial.print(F(",\"motion\":"));
  Serial.print(motionIsRecent() ? F("true") : F("false"));

  Serial.print(F(",\"fan_on\":"));
  Serial.print(fanOn ? F("true") : F("false"));

  Serial.print(F(",\"light_on\":"));
  Serial.print(lightOn ? F("true") : F("false"));

  Serial.print(F(",\"temp_limit_c\":"));
  Serial.print(temperatureLimitC, 1);

  Serial.print(F(",\"temp_hysteresis_c\":"));
  Serial.print(temperatureHysteresisC, 1);

  Serial.print(F(",\"light_threshold\":"));
  Serial.print(lightThreshold);

  Serial.print(F(",\"auto_temperature\":"));
  Serial.print(autoTemperature ? F("true") : F("false"));

  Serial.print(F(",\"auto_lighting\":"));
  Serial.print(autoLighting ? F("true") : F("false"));

  Serial.print(F(",\"motion_required\":"));
  Serial.print(motionRequiredForLight ? F("true") : F("false"));

  Serial.println(F("}"));
}

// -------------------------
// Comandos recebidos do Node-RED
// Formato: CHAVE=VALOR
// -------------------------
void handleCommand(String line) {
  line.trim();

  if (line.length() == 0) {
    return;
  }

  if (line == "STATUS") {
    sendTelemetry();
    return;
  }

  const int separator = line.indexOf('=');
  if (separator < 1) {
    sendAck(line, false, "Formato esperado: CHAVE=VALOR");
    return;
  }

  String key = line.substring(0, separator);
  String value = line.substring(separator + 1);
  key.trim();
  value.trim();
  key.toUpperCase();

  if (key == "TEMP_LIMIT") {
    const float newValue = value.toFloat();
    if (newValue < -10.0 || newValue > 80.0) {
      sendAck(key, false, "Limite fora da faixa -10..80 C");
      return;
    }
    temperatureLimitC = newValue;
    sendAck(key, true);
    return;
  }

  if (key == "HYSTERESIS") {
    const float newValue = value.toFloat();
    if (newValue < 0.1 || newValue > 10.0) {
      sendAck(key, false, "Histerese fora da faixa 0.1..10 C");
      return;
    }
    temperatureHysteresisC = newValue;
    sendAck(key, true);
    return;
  }

  if (key == "LIGHT_THRESHOLD") {
    const int newValue = value.toInt();
    if (newValue < 0 || newValue > 1023) {
      sendAck(key, false, "Limite fora da faixa 0..1023");
      return;
    }
    lightThreshold = newValue;
    sendAck(key, true);
    return;
  }

  bool boolValue = false;

  if (key == "AUTO_TEMP") {
    if (!parseBoolean(value, boolValue)) {
      sendAck(key, false, "Valor booleano invalido");
      return;
    }
    autoTemperature = boolValue;
    sendAck(key, true);
    return;
  }

  if (key == "AUTO_LIGHT") {
    if (!parseBoolean(value, boolValue)) {
      sendAck(key, false, "Valor booleano invalido");
      return;
    }
    autoLighting = boolValue;
    sendAck(key, true);
    return;
  }

  if (key == "MOTION_REQUIRED") {
    if (!parseBoolean(value, boolValue)) {
      sendAck(key, false, "Valor booleano invalido");
      return;
    }
    motionRequiredForLight = boolValue;
    sendAck(key, true);
    return;
  }

  if (key == "FAN") {
    if (autoTemperature) {
      sendAck(key, false, "Desative AUTO_TEMP para controle manual");
      return;
    }

    if (!parseBoolean(value, boolValue)) {
      sendAck(key, false, "Valor booleano invalido");
      return;
    }

    fanOn = boolValue;
    setRelay(PIN_RELAY_FAN, fanOn);
    sendAck(key, true);
    return;
  }

  if (key == "LIGHT") {
    if (autoLighting) {
      sendAck(key, false, "Desative AUTO_LIGHT para controle manual");
      return;
    }

    if (!parseBoolean(value, boolValue)) {
      sendAck(key, false, "Valor booleano invalido");
      return;
    }

    lightOn = boolValue;
    setRelay(PIN_RELAY_LIGHT, lightOn);
    sendAck(key, true);
    return;
  }

  sendAck(key, false, "Comando desconhecido");
}

void readSerialCommands() {
  while (Serial.available() > 0) {
    const char c = static_cast<char>(Serial.read());

    if (c == '\n') {
      handleCommand(serialBuffer);
      serialBuffer = "";
    } else if (c != '\r') {
      if (serialBuffer.length() < 120) {
        serialBuffer += c;
      } else {
        serialBuffer = "";
        sendAck("SERIAL", false, "Comando excedeu 120 caracteres");
      }
    }
  }
}

// -------------------------
// Arduino lifecycle
// -------------------------
void setup() {
  Serial.begin(9600);

  pinMode(PIN_MOTION, INPUT);
  pinMode(PIN_RELAY_FAN, OUTPUT);
  pinMode(PIN_RELAY_LIGHT, OUTPUT);

  setRelay(PIN_RELAY_FAN, false);
  setRelay(PIN_RELAY_LIGHT, false);

  dht.begin();

  Serial.println(F("{\"type\":\"status\",\"message\":\"Arduino iniciado\"}"));
}

void loop() {
  readSerialCommands();

  const unsigned long now = millis();

  if (now - lastSensorReadAt >= SENSOR_INTERVAL_MS) {
    lastSensorReadAt = now;

    readSensors();
    applyAutomation();
    sendTelemetry();
  }
}
