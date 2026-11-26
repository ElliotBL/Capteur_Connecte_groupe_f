#include <MPS.h>
#include <DHT.h>
#include "config.h"

// ---- Variables constantes ----
constexpr int octetSize = 4;
constexpr int kDhtPin = 0;
constexpr int kLedPin = 2; 

// ---- Structures de payloads ----
struct __attribute__((packed)) CounterPayload {
  uint32_t count;
};
static_assert(sizeof(CounterPayload) == octetSize, "CounterPayload = 4 octets");

struct __attribute__((packed)) ResetCounterPayload {
  uint32_t newValue;
};
static_assert(sizeof(ResetCounterPayload) == octetSize, "ResetCounterPayload = 4 octets");

struct __attribute__((packed)) TemperaturePayload {
    float degree;
};
static_assert(sizeof(TemperaturePayload) == octetSize, "TemperaturePayload = 4 octets");

struct __attribute__((packed)) HumidityPayload {
    float percentage;
};
static_assert(sizeof(HumidityPayload) == octetSize, "HumidityPayload = 4 octets");

struct __attribute__((packed)) LedPowerPayload {
    uint32_t red;
    uint32_t green;
    uint32_t blue;
};
static_assert(sizeof(LedPowerPayload) == octetSize * 3, "LedPowerPayload = 12 octets");

// ---- Variables global ----
static CounterPayload counter = {0};
static TemperaturePayload temperature;
static HumidityPayload humidity;
static LedPowerPayload ledPower;

// ---- Subjects ----
constexpr mps::Subject kSubHb = mps::makeSubject("hb_" DEVICE_NAME); 
constexpr mps::Subject kSubStatusCounter = mps::makeSubject("status_" DEVICE_NAME "_counter");
constexpr mps::Subject kSubCmdResetCounter = mps::makeSubject("cmd_" DEVICE_NAME "_reset_counter");
constexpr mps::Subject kSubStatusTemperature = mps::makeSubject("status_" DEVICE_NAME "_temperature");
constexpr mps::Subject kSubStatusHumidity = mps::makeSubject("status_" DEVICE_NAME "_humidity");
constexpr mps::Subject kSubCmdLedPower = mps::makeSubject("cmd_" DEVICE_NAME "_led_power");
constexpr mps::Subject kSubStatusLedPower = mps::makeSubject("status_" DEVICE_NAME "_led_power");


DHT dht(kDhtPin, DHT11);

// ---- Callbacks ----
// Signale les échecs de transmission afin de faciliter le diagnostic des problèmes de communication.
static void onSendFailed(uint16_t sequenceNumber, uint32_t subjectId, mps::SendFailureReason, mps::AcknowledgementStatus) {
  Serial.printf("/!\\ envoi %u (subject 0x%08lX) perdu\n", sequenceNumber, (unsigned long)subjectId);
}

// Publie les dernières valeurs connues des capteurs et des périphériques lors de l'ouverture d'une session MPS.
static void onSessionOpened() {
  Serial.println("Session opened!");
  MPS.publishValue(kSubStatusCounter, counter);
  MPS.publishValue(kSubStatusTemperature, temperature);
  MPS.publishValue(kSubStatusHumidity, humidity);
  MPS.publishValue(kSubStatusLedPower, ledPower);
}

// Traite les commandes de réinitialisation du compteur reçues depuis l'interface de supervision.
static void onCmdReset(const mps::FrameView& message, void*) {
  ResetCounterPayload payload;

  if (!mps::readPayloadAs(message, &payload)) {
    Serial.println("Commande reset invalide");
    return;
  }

  counter.count = payload.newValue;
  
  MPS.publishValue(kSubStatusCounter, counter);
}

// Traite les commandes de contrôle RGB en vérifiant les valeurs reçues avant de modifier la couleur de la LED.
static void onCmdLedPower(const mps::FrameView& message, void*) {
  LedPowerPayload payload;

  if (!mps::readPayloadAs(message, &payload)) {
    Serial.println("Commande LED invalide");
    return;
  }

  if (payload.red > 255 || payload.green > 255 || payload.blue > 255) {
    Serial.println("Couleur LED invalide");
    return;
  }

  ledPower = payload;

  rgbLedWrite(kLedPin, ledPower.red, ledPower.green, ledPower.blue);

  MPS.publishValue(kSubCmdLedPower, ledPower); 
}

// ---- Fonctions appelées par setup/loop ----
// Maintient la communication avec l'interface de supervision en publiant périodiquement un signal de présence.
void publishHeartbeat(){
  static uint32_t lastHeartbeatTimeMs = 0;
  static const uint16_t kHbPeriodMs = 1000;
  if(millis() - lastHeartbeatTimeMs >= kHbPeriodMs) {
    lastHeartbeatTimeMs = millis();
    
    MPS.publishHeartbeat(kSubHb);
  }
}

// ---- Fonctions Arduino ----
// Initialise la communication MPS, configure les callbacks et prépare les périphériques utilisés par le programme.
void setup() {
  Serial.begin(115200);
  MPS.setLogOutput(&Serial);

  MPS.setRegisteredCallback(onSessionOpened);
  MPS.setSendFailedCallback(onSendFailed);

  static const uint8_t kUnsetMacAddress[6] = {0, 0, 0, 0, 0, 0};
  if(memcmp(kGatewayMacAddress, kUnsetMacAddress, 6) == 0) {
    Serial.println("MPS: renseignez kGatewayMacAddress dans config.h (adresse affichee par l'IHM)");
    return;
  }

  if(!MPS.begin(kGatewayMacAddress, DEVICE_NAME, kWifiChannel)) Serial.println("MPS: demarrage impossible");

  MPS.on(kSubCmdResetCounter, onCmdReset);
  MPS.on(kSubCmdLedPower, onCmdLedPower);

  dht.begin();
}

// Exécute les traitements périodiques du programme : communication MPS, comptage et acquisition des données environnementales.
void loop() {
  MPS.loop();

  publishHeartbeat();

  static uint32_t lastCounterTimeMs = 0;
  if (millis() - lastCounterTimeMs >= 2000) {
    lastCounterTimeMs = millis();
    counter.count++;
    MPS.publishValue(kSubStatusCounter, counter);
  }

  static uint32_t lastDhtReadMs = 0;
  if (millis() - lastDhtReadMs >= 1000) {
    lastDhtReadMs = millis();
    float temperatureReadedDegrees = dht.readTemperature();
    float humidityReadedPercentage = dht.readHumidity();

    if (!isnan(temperatureReadedDegrees) && !isnan(humidityReadedPercentage)) {
      temperature.degree = temperatureReadedDegrees;
      humidity.percentage = humidityReadedPercentage;

      MPS.publishValue(kSubStatusTemperature, temperature);
      MPS.publishValue(kSubStatusHumidity, humidity);
    }
  }

}
