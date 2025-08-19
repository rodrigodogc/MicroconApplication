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
#define LED        D4
#define PWM_PINO   D2
#define BTN_RESET  D3 

/* -------------------- tempos preset -------------------- */
#define PORTA_BROKER   1883
#define WIFI_RETRY_MS  5000   // tenta religar o Wi-Fi a cada 5s se cair
#define MQTT_RETRY_MS  3000   // tenta reconectar ao broker a cada 3s
#define MQTT_PING_MS  15000   // ping a cada 15s
#define RESET_HOLD_MS  5000   // tempo de reset segurando o botão em d4

typedef struct {
  String nomeWifi, senhaWifi, ipBroker, userBroker, passBroker, local;
} Config;

Config CONFIG;
Preferences prefs;

/* -------------------- Globais -------------------- */
WiFiClient netClient;
AsyncWebServer server(80);

unsigned int DutyciclePWM  = 127;
unsigned int FrequenciaPWM = 5000;   

DNSServer dnsServer;
const byte DNS_PORT = 53;
bool modoConfiguracao = false;

/* MQTT */
Adafruit_MQTT_Client* mqtt = nullptr;

// Tópicos pra se inscrever
static const char* TOPIC_SET_VEL = "ventilador/set_velocidade"; // entrada
static const char* TOPIC_GET_VEL = "ventilador/get_velocidade"; // saída

// Subscriptions
Adafruit_MQTT_Subscribe* setVelSub  = nullptr; // ventilador/set_velocidade
Adafruit_MQTT_Subscribe* tempIntSub = nullptr; // sensores/temperatura/interna
Adafruit_MQTT_Subscribe* tempExtSub = nullptr; // sensores/temperatura/externa
Adafruit_MQTT_Publish*   velPub     = nullptr; // ventilador/get_velocidade

// Temperaturas vindas do MQTT
float temperaturaInterna = 0.0;
float temperaturaExterna = 0.0;

/* Estados de reconexão */
static unsigned long lastWifiAttempt = 0;
static unsigned long lastMqttAttempt = 0;
static unsigned long lastMqttPing    = 0;

/* Fila de publicação (para evitar publish dentro de callbacks HTTP) */
int velPublishPercent = -1;
volatile bool velPublishPending = false;
unsigned long lastVelPublishTry = 0;
const unsigned long VEL_PUBLISH_MIN_MS = 120;

/* -------------------- Prototypes -------------------- */
void updateConfigs(String dados);
void carregarConfigs();
void salvarConfigs();

void iniciarModoAP();
void iniciarRotasHTTP();

void ensureWiFiConnected();
void doMqttConnect();
void ensureMqttConnected();

void onSetVelMsg(const char* payload);
void applyDuty(int duty_0_255);
void scheduleVelPublish();

static inline int clampi(int v, int lo, int hi) { 
  return v < lo ? lo : (v > hi ? hi : v); 
}

/* -------------------- Config helpers -------------------- */
void updateConfigs(String dados) {
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
  CONFIG.local = dados;
}

void carregarConfigs() {
  prefs.begin("config", true);
  CONFIG.nomeWifi  = prefs.getString("wifi",  "");
  CONFIG.senhaWifi = prefs.getString("senha", "");
  CONFIG.ipBroker  = prefs.getString("ip",    "");
  CONFIG.userBroker= prefs.getString("user",  "");
  CONFIG.passBroker= prefs.getString("pass",  "");
  CONFIG.local     = prefs.getString("local", "");
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
void doMqttConnect() {
  if (!mqtt) return;
  Serial.print("Conectando ao Broker MQTT... ");
  int8_t rc = mqtt->connect();
  if (rc == 0) {
    Serial.println("Conectado.");

    // (Re)inscreve
    if (setVelSub) {
      bool ok = mqtt->subscribe(setVelSub);
      Serial.printf("Subscribe %s: %s\n", TOPIC_SET_VEL, ok ? "ok" : "falhou");
    }
    if (tempIntSub) {
      bool ok = mqtt->subscribe(tempIntSub);
      Serial.printf("Subscribe %s: %s\n", "sensores/temperatura/interna", ok ? "ok" : "falhou");
    }
    if (tempExtSub) {
      bool ok = mqtt->subscribe(tempExtSub);
      Serial.printf("Subscribe %s: %s\n", "sensores/temperatura/externa", ok ? "ok" : "falhou");
    }

    lastMqttPing = millis();
  } else {
    Serial.printf("Falha [%s]\n", mqtt->connectErrorString(rc));
    mqtt->disconnect(); // estado limpo
  }
}

void ensureMqttConnected() {
  if (!mqtt || !WiFi.isConnected()) return;

  // reconecta se caiu o broker
  if (!mqtt->connected()) {
    unsigned long now = millis();
    if (now - lastMqttAttempt >= MQTT_RETRY_MS) {
      lastMqttAttempt = now;
      doMqttConnect();
    }
    return;
  }

  // processa tráfego
  mqtt->processPackets(10);
  Adafruit_MQTT_Subscribe* sub;
  while ((sub = mqtt->readSubscription(10))) {
    if (sub == setVelSub) {
      onSetVelMsg((char*)setVelSub->lastread);
    } else if (sub == tempIntSub) {
      temperaturaInterna = atof((char*)tempIntSub->lastread);
    } else if (sub == tempExtSub) {
      temperaturaExterna = atof((char*)tempExtSub->lastread);
    }
    yield();
  }

  // ping keepalive
  unsigned long now = millis();
  if (now - lastMqttPing >= MQTT_PING_MS) {
    lastMqttPing = now;
    if (!mqtt->ping()) {
      Serial.println("MQTT ping falhou; desconectando para reconectar...");
      mqtt->disconnect();
    }
  }

  // publica velocidade pendente
  if (velPublishPending && (now - lastVelPublishTry >= VEL_PUBLISH_MIN_MS)) {
    lastVelPublishTry = now;
    if (velPub && velPublishPercent >= 0) {
      char msg[8];
      snprintf(msg, sizeof(msg), "%d", velPublishPercent);
      bool ok = velPub->publish(msg);
      Serial.printf("[MQTT] publish %s = %s%% -> %s\n", TOPIC_GET_VEL, msg, ok ? "ok" : "falhou");
      if (ok) 
        velPublishPending = false; // se falhar, tenta de novo no próximo ciclo
    }
  }
}

/* -------------------- Duty / Publish -------------------- */
void applyDuty(int duty_0_255) {
  DutyciclePWM = (unsigned)clampi(duty_0_255, 0, 255);
  analogWrite(PWM_PINO, DutyciclePWM);
}

void scheduleVelPublish() {
  velPublishPercent  = (int)round((100.0/255.0) * (double)DutyciclePWM);
  velPublishPending  = true; 
}

void onSetVelMsg(const char* payload) {
  // funciona com 0–255 ou 0–100%
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

/* -------------------- AP e Rotas -------------------- */
void iniciarModoAP() {
  modoConfiguracao = true;
  WiFi.softAP("Ventilador-Config", "12345678");
  IPAddress ip = WiFi.softAPIP();
  Serial.printf("AP iniciado. IP: %s\n", ip.toString().c_str());

  dnsServer.start(DNS_PORT, "*", ip);
  server.onNotFound([](AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", paginaConfig);
  });
}

void iniciarRotasHTTP() {
 
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
    if (modoConfiguracao) 
      req->send_P(200, "text/html", paginaConfig);
    else                  
      req->send_P(200, "text/html", paginaParametros);
  });

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
        delay(800);
        ESP.restart();
      }
    }
  );

  // Recuperar configs (CSV)
  server.on("/recuperarConfigs", HTTP_GET, [](AsyncWebServerRequest* request) {
    String dados = CONFIG.nomeWifi + "," + CONFIG.senhaWifi + "," + CONFIG.ipBroker + "," + CONFIG.userBroker + "," + CONFIG.passBroker + "," + CONFIG.local;
    if (CONFIG.nomeWifi == "") request->send(404, "text/plain", "Sem configuracao");
    else                       request->send(200, "text/plain", dados);
  });

  // SET dutycicle (0..255 ou XX%) tbm agenda publish no topico
  server.on("/set_dutycicle", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!request->hasParam("dutycicle")) {
      request->send(400, "text/plain", "FALTA O PARAMETRO dutycicle");
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

  // dutycicle atual sincronização do knob
  server.on("/get_dutycicle", HTTP_GET, [](AsyncWebServerRequest* request) {
    StaticJsonDocument<128> j;
    j["duty"]    = DutyciclePWM;
    j["percent"] = (int)round((100.0/255.0) * (double)DutyciclePWM);
    String out; serializeJson(j, out);
    request->send(200, "application/json", out);
  });

  // temperaturas (vindas dos termometros MQTT)
  server.on("/get_temperaturas", HTTP_GET, [](AsyncWebServerRequest* request) {
    StaticJsonDocument<128> j;
    if (!isnan(temperaturaInterna)) j["interna"] = temperaturaInterna;
    if (!isnan(temperaturaExterna)) j["externa"] = temperaturaExterna;
    String out; serializeJson(j, out);
    request->send(200, "application/json", out);
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

void checkBotaoHardReset() {
  static bool holding = false;
  static unsigned long t0 = 0;
  if (digitalRead(BTN_RESET) == LOW) {
    if (!holding) {
      holding = true;
      t0 = millis();
    } else {
      if (millis() - t0 >= RESET_HOLD_MS) {
        prefs.begin("config", false);
        prefs.clear();
        prefs.end();
        delay(50);
        ESP.restart();
      }
    }
  } else {
    holding = false;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  pinMode(PWM_PINO, OUTPUT);
  pinMode(BTN_RESET, INPUT_PULLUP);

  analogWriteFreq(FrequenciaPWM);
  analogWriteRange(255);
  analogWrite(PWM_PINO, DutyciclePWM);

  Serial.println("\nBoot...");

  carregarConfigs();

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
  } 
  else {
    modoConfiguracao = false;
    Serial.printf("\nConectado. IP: %s\n", WiFi.localIP().toString().c_str());

    mqtt = new Adafruit_MQTT_Client(
      &netClient,
      CONFIG.ipBroker.c_str(),
      PORTA_BROKER,
      CONFIG.userBroker.c_str(),
      CONFIG.passBroker.c_str()
    );

    // Subscriptions / Publish
    setVelSub  = new Adafruit_MQTT_Subscribe(mqtt, TOPIC_SET_VEL, MQTT_QOS_1);
    tempIntSub = new Adafruit_MQTT_Subscribe(mqtt, "sensores/temperatura/interna", MQTT_QOS_1);
    tempExtSub = new Adafruit_MQTT_Subscribe(mqtt, "sensores/temperatura/externa", MQTT_QOS_1);
    velPub     = new Adafruit_MQTT_Publish(mqtt, TOPIC_GET_VEL);
    // primeira tentativa de conexão
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

