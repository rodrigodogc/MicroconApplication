#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include <DNSServer.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <ArduinoJson.h>

#include "ConfigHTML.h"
#include "ParametrosHTML.h"

/* -------------------- Pinos -------------------- */
#define PWM_PINO   D2
#define BTN_RESET  D3  //  5s em GND = reset configs

/* -------------------- Parametrização -------------------- */
#define WIFI_RETRY_MS   5000
#define MQTT_RETRY_MS   3000
#define MQTT_PING_MS   15000
#define RESET_HOLD_MS   5000
#define PORTA_BROKER    1883

/* -------------------- Config persistida -------------------- */
typedef struct {
  String nomeWifi, senhaWifi, ipBroker, userBroker, passBroker, local;
} Config;

Config CONFIG;
Preferences prefs;

/* -------------------- Globais -------------------- */
WiFiClient netClient;
AsyncWebServer server(80);

unsigned int DutyciclePWM  = 127;
unsigned int FrequenciaPWM = 10000;

DNSServer dnsServer;
const byte DNS_PORT = 53;
bool modoConfiguracao = false;

/* MQTT */
Adafruit_MQTT_Client* mqtt = nullptr;

// Tópicos de velocidade
static const char* TOPIC_SET_VEL = "ventilador/set_velocidade"; // entrada
static const char* TOPIC_GET_VEL = "ventilador/get_velocidade"; // saída

// Tópicos de temperaturas (entrada)
static const char* TOPIC_TEMP_INT = "Ventilador/Temperatura";//"sensores/temperatura/interna";
static const char* TOPIC_TEMP_EXT = "sensores/temperatura/externa";

// Tópicos do MODO AUTOMÁTICO
static const char* TOPIC_SET_AUTO = "ventilador/set_automatico";
static const char* TOPIC_GET_AUTO = "ventilador/get_automatico";

// Subscriptions / Publish
Adafruit_MQTT_Subscribe* setVelSub  = nullptr;
Adafruit_MQTT_Subscribe* tempIntSub = nullptr;
Adafruit_MQTT_Subscribe* tempExtSub = nullptr;
Adafruit_MQTT_Subscribe* autoSub    = nullptr;
Adafruit_MQTT_Publish*   velPub     = nullptr;
Adafruit_MQTT_Publish*   autoPub    = nullptr;

// Temperaturas vindas do MQTT
float temperaturaInterna = NAN;
float temperaturaExterna = NAN;

// Estado do modo automático
bool modoAutomatico = false;

// Estados de reconexão
static unsigned long lastWifiAttempt = 0;
static unsigned long lastMqttAttempt = 0;
static unsigned long lastMqttPing    = 0;

// Filas de publicação
int velPublishPercent = -1;
volatile bool velPublishPending = false;
unsigned long lastVelPublishTry = 0;
const unsigned long VEL_PUBLISH_MIN_MS = 120;

// Publicação de estado automático
volatile bool autoPublishPending = false;
unsigned long lastAutoPublishTry = 0;
const unsigned long AUTO_PUBLISH_MIN_MS = 120;

static inline int clampi(int v, int lo, int hi) { 
  return v < lo ? lo : (v > hi ? hi : v); 
}

/* -------------------- Config  -------------------- */
void updateConfigs(String dados) {
  // wifi,senha,ip,user,pass,local
  int idx = 0;
  while (dados.indexOf(',') != -1) {
    int pos   = dados.indexOf(',');
    String p  = dados.substring(0, pos);
    switch (idx) {
      case 0: CONFIG.nomeWifi = p; break;
      case 1: CONFIG.senhaWifi = p; break;
      case 2: CONFIG.ipBroker = p; break;
      case 3: CONFIG.userBroker = p; break;
      case 4: CONFIG.passBroker = p; break;
    }
    dados.remove(0, pos + 1);
    idx++;
  }
  CONFIG.local = dados; // último campo
}

void carregarConfigs() {
  prefs.begin("config", true);
  CONFIG.nomeWifi   = prefs.getString("wifi",  "");
  CONFIG.senhaWifi  = prefs.getString("senha", "");
  CONFIG.ipBroker   = prefs.getString("ip",    "");
  CONFIG.userBroker = prefs.getString("user",  "");
  CONFIG.passBroker = prefs.getString("pass",  "");
  CONFIG.local      = prefs.getString("local", "");
  prefs.end();

  if (CONFIG.nomeWifi == "") Serial.println("Nenhuma configuração encontrada; entrando em modo AP.");
  else                      Serial.println("Configurações carregadas.");
}

void salvarConfigs() {
  prefs.begin("config", false);
  prefs.putString("wifi",  CONFIG.nomeWifi);
  prefs.putString("senha", CONFIG.senhaWifi);
  prefs.putString("ip",    CONFIG.ipBroker);
  prefs.putString("user",  CONFIG.userBroker);
  prefs.putString("pass",  CONFIG.passBroker);
  prefs.putString("local", CONFIG.local);
  prefs.end();
  Serial.println("Configurações salvas.");
}

/* -------------------- Persistência do modo automático -------------------- */
void loadAutoFromPrefs() {
  prefs.begin("config", true);
  uint8_t v = prefs.getUChar("auto", 0);
  prefs.end();
  modoAutomatico = (v != 0);
  Serial.printf("Modo automático (prefs): %s\n", modoAutomatico ? "ON" : "OFF");
}

void saveAutoToPrefs() {
  prefs.begin("config", false);
  prefs.putUChar("auto", modoAutomatico ? 1 : 0);
  prefs.end();
}

/* -------------------- Wi-Fi -------------------- */
void ensureWiFiConnected() {
  if (WiFi.status() == WL_CONNECTED) return;
  unsigned long now = millis();
  if (now - lastWifiAttempt >= WIFI_RETRY_MS) {
    lastWifiAttempt = now;
    Serial.println("Wi-Fi caiu. Tentando reconectar...");
    WiFi.disconnect();
    WiFi.begin(CONFIG.nomeWifi.c_str(), CONFIG.senhaWifi.c_str());
  }
}

/* -------------------- MQTT -------------------- */
void scheduleVelPublish() {
  velPublishPercent  = (int)round((100.0/255.0) * (double)DutyciclePWM);
  velPublishPending  = true;
}

void scheduleAutoPublish() {
  autoPublishPending = true;
}

void onSetAutoMsg(const char* payload) {
  bool novo = (strcasecmp(payload, "ON") == 0) || (strcmp(payload, "1") == 0) || (strcasecmp(payload, "true") == 0);
  if (modoAutomatico != novo) {
    modoAutomatico = novo;
    saveAutoToPrefs();
  }
  scheduleAutoPublish();  // republica para sincronizar
}

void doMqttConnect() {
  if (!mqtt) return;
  Serial.print("Conectando ao Broker MQTT... ");
  int8_t rc = mqtt->connect();
  if (rc == 0) {
    Serial.println("Conectado.");

    if (setVelSub)  { bool ok = mqtt->subscribe(setVelSub);  
    Serial.printf("Subscribe %s: %s\n", TOPIC_SET_VEL,  ok?"ok":"falhou"); }
    if (tempIntSub) { bool ok = mqtt->subscribe(tempIntSub); 
    Serial.printf("Subscribe %s: %s\n", TOPIC_TEMP_INT, ok?"ok":"falhou"); }
    if (tempExtSub) { bool ok = mqtt->subscribe(tempExtSub); 
    Serial.printf("Subscribe %s: %s\n", TOPIC_TEMP_EXT, ok?"ok":"falhou"); }
    if (autoSub)    { bool ok = mqtt->subscribe(autoSub);    
    Serial.printf("Subscribe %s: %s\n", TOPIC_SET_AUTO, ok?"ok":"falhou"); }

    // republica estados atuais
    scheduleVelPublish();
    scheduleAutoPublish();

    lastMqttPing = millis();
  } else {
    Serial.printf("Falha [%s]\n", mqtt->connectErrorString(rc));
    mqtt->disconnect();
  }
}

void onSetVelMsg(const char* payload) {
  int v = atoi(payload);
  bool temPercent = strchr(payload, '%') != nullptr;
  if (temPercent || (v >= 0 && v <= 100)) {
    v = clampi(v, 0, 100);
    v = map(v, 0, 100, 0, 255);
  }
  v = clampi(v, 0, 255);

  Serial.printf("[MQTT] %s: '%s' -> duty=%d (%.1f%%)\n",
                TOPIC_SET_VEL, payload, v, (100.0/255.0)*v);

  applyDuty(v);
  scheduleVelPublish();
}

void ensureMqttConnected() {
  if (!mqtt || !WiFi.isConnected()) return;

  if (!mqtt->connected()) {
    unsigned long now = millis();
    if (now - lastMqttAttempt >= MQTT_RETRY_MS) {
      lastMqttAttempt = now;
      doMqttConnect();
    }
    return;
  }

  mqtt->processPackets(10);

  Adafruit_MQTT_Subscribe* sub;
  while ((sub = mqtt->readSubscription(10))) {
    if (sub == setVelSub) {
      onSetVelMsg((char*)setVelSub->lastread);
    } else if (sub == tempIntSub) {
      temperaturaInterna = atof((char*)tempIntSub->lastread);
    } else if (sub == tempExtSub) {
      temperaturaExterna = atof((char*)tempExtSub->lastread);
    } else if (sub == autoSub) {
      onSetAutoMsg((char*)autoSub->lastread); // salva e publica
    }
    yield();
  }

  unsigned long now = millis();
  if (now - lastMqttPing >= MQTT_PING_MS) {
    lastMqttPing = now;
    if (!mqtt->ping()) {
      Serial.println("MQTT ping falhou; desconectando para reconectar...");
      mqtt->disconnect();
    }
  }

  if (velPublishPending && (now - lastVelPublishTry >= VEL_PUBLISH_MIN_MS)) {
    lastVelPublishTry = now;
    if (velPub && velPublishPercent >= 0) {
      char msg[8];
      snprintf(msg, sizeof(msg), "%d", velPublishPercent);
      bool ok = velPub->publish(msg);
      Serial.printf("[MQTT] publish %s = %s%% -> %s\n", TOPIC_GET_VEL, msg, ok ? "ok" : "falhou");
      if (ok) velPublishPending = false;
    }
  }

  if (autoPublishPending && (now - lastAutoPublishTry >= AUTO_PUBLISH_MIN_MS)) {
    lastAutoPublishTry = now;
    if (autoPub) {
      const char* msg = modoAutomatico ? "ON" : "OFF";
      bool ok = autoPub->publish(msg);
      Serial.printf("[MQTT] publish %s = %s -> %s\n", TOPIC_GET_AUTO, msg, ok ? "ok" : "falhou");
      if (ok) autoPublishPending = false;
    }
  }
}

/* -------------------- Duty / Publish -------------------- */
void applyDuty(int duty_0_255) {
  DutyciclePWM = (unsigned)clampi(duty_0_255, 0, 255);
  analogWrite(PWM_PINO, DutyciclePWM);
}

/* -------------------- AP e Rotas -------------------- */
void iniciarModoAP() {
  modoConfiguracao = true;
  WiFi.softAP("Termometro-Config", "12345678");
  IPAddress ip = WiFi.softAPIP();
  Serial.printf("AP iniciado. IP: %s\n", ip.toString().c_str());

  dnsServer.start(DNS_PORT, "*", ip);
  server.onNotFound([](AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", paginaConfig);
  });
}

void iniciarRotasHTTP() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
    if (modoConfiguracao) req->send_P(200, "text/html", paginaConfig);
    else                  req->send_P(200, "text/html", paginaParametros);
  });

  // wifi,senha,ip,user,pass,local
  server.on("/configurar", HTTP_POST,
    [](AsyncWebServerRequest* request) {},
    NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      static String body;
      if (index == 0) body = "";
      body += String((char*)data).substring(0, len);

      if (index + len == total) {
        if (!body.length()) {
          request->send(400, "text/plain", "Corpo vazio");
          return;
        }
        updateConfigs(body);
        salvarConfigs();
        request->send(200, "text/plain", "OK");
      }
    }
  );

  // seta dutycicle (aceita 0..255 ou 0..100%)
  server.on("/set_dutycicle", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!request->hasParam("dutycicle")) {
    request->send(400, "text/plain", "Falta parametro 'dutycicle'");
      return;
    }
    String duty = request->getParam("dutycicle")->value();
    int dutyint = duty.toInt();
    if (duty.endsWith("%")) {
      dutyint = clampi(dutyint, 0, 100);
      dutyint = map(dutyint, 0, 100, 0, 255);
    }
    dutyint = clampi(dutyint, 0, 255);

    applyDuty(dutyint);
    scheduleVelPublish();

    char buf[64];
    snprintf(buf, sizeof(buf), "Velocidade atualizada para %.1f%%", (100.0/255.0) * (double)dutyint);
    request->send(200, "text/plain", buf);
  });

  // dutycicle atual
  server.on("/get_dutycicle", HTTP_GET, [](AsyncWebServerRequest* request) {
    StaticJsonDocument<128> j;
    j["duty"]    = DutyciclePWM;
    j["percent"] = (int)round((100.0/255.0) * (double)DutyciclePWM);
    String out; serializeJson(j, out);
    request->send(200, "application/json", out);
  });

  // temperaturas (vindas do MQTT)
  server.on("/get_temperaturas", HTTP_GET, [](AsyncWebServerRequest* request) {
    StaticJsonDocument<128> j;
    if (!isnan(temperaturaInterna)) 
      j["interna"] = String(temperaturaInterna);
    if (!isnan(temperaturaExterna)) 
      j["externa"] = String(temperaturaExterna);
    String out; 
    serializeJson(j, out);
    request->send(200, "application/json", out);
  });

  // Modo automático 
  server.on("/get_automatico", HTTP_GET, [](AsyncWebServerRequest* request) {
    StaticJsonDocument<64> j;
    j["auto"] = modoAutomatico;
    String out; serializeJson(j, out);
    request->send(200, "application/json", out);
  });

  server.on("/set_automatico", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!request->hasParam("on")) {
      request->send(400, "text/plain", "Falta parametro 'on'");
      return;
    }
    String v = request->getParam("on")->value();
    bool novo = false;
    { // parse
      String s=v; s.trim(); s.toLowerCase();
      if (s=="1"||s=="on"||s=="true"||s=="enable"||s=="enabled") novo=true;
      else novo=false;
    }
    if (modoAutomatico != novo) {
      modoAutomatico = novo;
      saveAutoToPrefs(); 
    }
    scheduleAutoPublish();
    request->send(200, "text/plain", String("Modo automático: ") + (modoAutomatico?"ON":"OFF"));
  });

  // Resetar configs
  server.on("/resetar_config", HTTP_POST, [](AsyncWebServerRequest* request) {
    prefs.begin("config", false);
    prefs.clear();
    prefs.end();
    request->send(200, "text/plain", "Reset OK");
    delay(1000);
    ESP.restart();
  });

  server.onNotFound([](AsyncWebServerRequest* request) {
    request->send(404, "text/plain", "Nao encontrado.");
  });
}

/* -------------------- Hard reset ------------------- */
void checkBotaoHardReset() {
  static bool holding = false;
  static unsigned long t0 = 0;
  if (digitalRead(BTN_RESET) == LOW) {
    if (!holding) { holding = true; t0 = millis(); }
    else if (millis() - t0 >= RESET_HOLD_MS) {
      prefs.begin("config", false);
      prefs.clear();
      prefs.end();
      delay(50);
      ESP.restart();
    }
  } else {
    holding = false;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PWM_PINO, OUTPUT);
  pinMode(BTN_RESET, INPUT_PULLUP);

  analogWriteFreq(FrequenciaPWM);
  analogWriteRange(255);
  analogWrite(PWM_PINO, DutyciclePWM);

  Serial.println("\nBoot...");

  carregarConfigs();
  loadAutoFromPrefs();

  WiFi.mode(WIFI_STA);
  WiFi.begin(CONFIG.nomeWifi.c_str(), CONFIG.senhaWifi.c_str());
  Serial.printf("Conectando a %s", CONFIG.nomeWifi.c_str());
  unsigned long prevTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - prevTime < 15000) {
    delay(500); Serial.print('.');
  }

  iniciarRotasHTTP();

  if (WiFi.status() != WL_CONNECTED) {
    iniciarModoAP();
  } else {
    modoConfiguracao = false;
    Serial.printf("\nConectado. IP: %s\n", WiFi.localIP().toString().c_str());

    mqtt = new Adafruit_MQTT_Client(
      &netClient,
      CONFIG.ipBroker.c_str(),
      PORTA_BROKER,
      CONFIG.userBroker.c_str(),
      CONFIG.passBroker.c_str()
    );

    setVelSub  = new Adafruit_MQTT_Subscribe(mqtt, TOPIC_SET_VEL, MQTT_QOS_1);
    tempIntSub = new Adafruit_MQTT_Subscribe(mqtt, TOPIC_TEMP_INT, MQTT_QOS_1);
    tempExtSub = new Adafruit_MQTT_Subscribe(mqtt, TOPIC_TEMP_EXT, MQTT_QOS_1);
    autoSub    = new Adafruit_MQTT_Subscribe(mqtt, TOPIC_SET_AUTO, MQTT_QOS_1);

    velPub     = new Adafruit_MQTT_Publish(mqtt, TOPIC_GET_VEL);
    autoPub    = new Adafruit_MQTT_Publish(mqtt, TOPIC_GET_AUTO);

    doMqttConnect();
  }

  server.begin();
}

void loop() {
  if (modoConfiguracao) {
    dnsServer.processNextRequest();
  } else {
    ensureWiFiConnected();
    ensureMqttConnected();
  }
  checkBotaoHardReset();
}
