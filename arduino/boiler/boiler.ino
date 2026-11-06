#include <MPS.h>
#include <DHT.h>
#include "config.h"

// ---- Structures de payloads ----

// Structure des données
struct __attribute__((packed)) CounterPayload {
  uint32_t count;
};
static_assert(sizeof(CounterPayload) == 4, "CounterPayload = 4 octets");

// Structure des données reçues
struct __attribute__((packed)) ResetCounterPayload {
  uint32_t newValue;
};
static_assert(sizeof(ResetCounterPayload) == 4, "ResetCounterPayload = 4 octets");

struct __attribute__((packed)) TemperaturePayload {
    float degree;
};
static_assert(sizeof(TemperaturePayload) == 4, "TemperaturePayload = 4 octets");


struct __attribute__((packed)) HumidityPayload {
    float percentage;
};
static_assert(sizeof(HumidityPayload) == 4, "HumidityPayload = 4 octets");

// ---- Variables globales ----
static CounterPayload counter = {0};
constexpr int kDhtPin = 0; // Broche connecté au DHT11
static TemperaturePayload temperature;
static HumidityPayload humidity;

// ---- Subjects ----
constexpr mps::Subject kSubHb = mps::makeSubject("hb_" DEVICE_NAME); // déjà présent
constexpr mps::Subject kSubStatusCounter = mps::makeSubject("status_" DEVICE_NAME "_counter");
constexpr mps::Subject kSubCmdResetCounter = mps::makeSubject("cmd_" DEVICE_NAME "_reset_counter");
constexpr mps::Subject kSubStatusTemperature = mps::makeSubject("status_" DEVICE_NAME "_temperature");
constexpr mps::Subject kSubStatusHumidity = mps::makeSubject("status_" DEVICE_NAME "_humidity");


DHT dht(kDhtPin, DHT11);

// ---- Callbacks ----

// Appelé en cas d'échec d'envoi (callback)
static void onSendFailed(uint16_t sequenceNumber, uint32_t subjectId, mps::SendFailureReason, mps::AcknowledgementStatus) {
  Serial.printf("/!\\ envoi %u (subject 0x%08lX) perdu\n", sequenceNumber, (unsigned long)subjectId);
}

// Appelé lors de l'ouverture de la session (callback)
static void onSessionOpened() {
  Serial.println("Session opened!");
  // republiez ici l’état initial, la passerelle ne garde rien
  MPS.publishValue(kSubStatusCounter, counter);
  MPS.publishValue(kSubStatusTemperature, temperature);
  MPS.publishValue(kSubStatusHumidity, humidity);
}

static void onCmdReset(const mps::FrameView& message, void*) {
  ResetCounterPayload payload;

  // Si la valeur du payload est à la longueur voulu alors le conteur se réinitialise à cette valeur, à 0 sinon
  if (mps::readPayloadAs(message, &payload)) counter.count = payload.newValue;
  else counter.count = 0;
  
  MPS.publishValue(kSubStatusCounter, counter); // l'IHM voit le résultat réel
}

// ---- Fonctions appelées par setup/loop ----
// envoi de heartbeats réguliers sans bloquer (comparaison au temps écoulé sans delay())
void publishHeartbeat(){
  static uint32_t lastHeartbeatTimeMs = 0;
  static const uint16_t kHbPeriod = 1000;
  if(millis() - lastHeartbeatTimeMs >= kHbPeriod) {
    lastHeartbeatTimeMs = millis();
    
    // L'envoi de heartbeats depuis loop() assure au récepteur (IHM) que le code "métier" n'est pas planté
    MPS.publishHeartbeat(kSubHb);
  }
}

// ---- Fonctions Arduino ----
// Exécutée une fois au démarrage
void setup() {
  Serial.begin(115200); // Démarrage de la communication série à 115200 bauds (sortie des Serial.print.. par exemple)
  MPS.setLogOutput(&Serial); // Définition de la sortie des messages de log de la lib MPS

  // enregistrement des callbacks
  MPS.setRegisteredCallback(onSessionOpened);
  MPS.setSendFailedCallback(onSendFailed);

  // constante d'adresse vide pour vérifier que l'adresse a été définie dans config.h
  static const uint8_t kUnsetMacAddress[6] = {0, 0, 0, 0, 0, 0};
  if(memcmp(kGatewayMacAddress, kUnsetMacAddress, 6) == 0) {
    Serial.println("MPS: renseignez kGatewayMacAddress dans config.h (adresse affichee par l'IHM)");
    return;
  }

  // démarrage du client MPS
  if(!MPS.begin(kGatewayMacAddress, DEVICE_NAME, kWifiChannel)) Serial.println("MPS: demarrage impossible");

  MPS.on(kSubCmdResetCounter, onCmdReset);

  dht.begin();
}

// Exécutée en boucle (= while(true))
void loop() {
  // appel de la boucle MPS (/!\ du code bloquant dans votre boucle loop() pourrait occasionner des problèmes de connexion)
  MPS.loop();

  // Appel non bloquant de l'envoi d'HB (polling)
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
    float temperatureReaded = dht.readTemperature();
    float humidityReaded = dht.readHumidity();

    // Vérification des mesures
    if (!isnan(temperatureReaded) && !isnan(humidityReaded)) {
      temperature.degree = temperatureReaded;
      humidity.percentage = humidityReaded;

      MPS.publishValue(kSubStatusTemperature, temperature);
      MPS.publishValue(kSubStatusHumidity, humidity);
    }
  }

}
