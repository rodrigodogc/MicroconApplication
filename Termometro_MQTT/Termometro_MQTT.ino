#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "ConfigHTML.h"       
#include "ParametrosHTML.h" 

/* -------------------- Pinos -------------------- */
#define BTN_RESET      D3       
#define PINO_ONEWIRE   D1 

/* -------------------- Tempos / Rede -------------------- */
#define PORTA_BROKER    1883
#define WIFI_RETRY_MS   5000   // tenta religar Wi-Fi a cada 5s
#define MQTT_RETRY_MS   3000   // tenta reconectar broker a cada 3s
#define MQTT_PING_MS   15000   // ping keepalive a cada 15s
#define RESET_HOLD_MS   5000   // D3 por 5s = reset

/* -------------------- Leitura do sensor -------------------- */
#define READ_INTERVAL_MS 2000

/* -------------------- Config persistida -------------------- */
typedef struct {
  String nomeWifi, senhaWifi, ipBroker, userBroker, passBroker;
  String tempTopic;  // ex.: sensores/temperatura/interna
  String local;      // ex.: Linha 1 • Forno A
} Config;

Config CONFIG;
Preferences prefs;

/* -------------------- Globais -------------------- */
WiFiClient netClient;
AsyncWebServer server(80);

DNSServer dnsServer;
const byte DNS_PORT = 53;
bool modoConfiguracao = false;

/* MQTT */
Adafruit_MQTT_Client* mqtt = nullptr;
Adafruit_MQTT_Publish* tempPub = nullptr;
/* OneWire + Dallas */
OneWire oneWire(PINO_ONEWIRE);
DallasTemperature sensors(&oneWire);
float temperaturaC = NAN;

/* Reconexão */
static unsigned long lastWifiAttempt = 0;
static unsigned long lastMqttAttempt = 0;
static unsigned long lastMqttPing    = 0;

/* Publish desacoplado */
volatile bool publicarTemperatura = false;
unsigned long tempoUltimaPublication = 0;
const unsigned long TEMP_PUBLISH_MIN_MS = 100;

/* Timers */
unsigned long lastReadMs = 0;

/* -------------------- Helpers -------------------- */
void updateConfigs(String dados) {
  // CSV: wifi,senha,ip,user,pass,topic,local
  int idx = 0;
  while (dados.indexOf(',') != -1) {
    int pos = dados.indexOf(',');
    String p = dados.substring(0, pos);
    switch (idx) {
      case 0: CONFIG.nomeWifi   = p; break;
      case 1: CONFIG.senhaWifi  = p; break;
      case 2: CONFIG.ipBroker   = p; break;
      case 3: CONFIG.userBroker = p; break;
      case 4: CONFIG.passBroker = p; break;
      case 5: CONFIG.tempTopic  = p; break;
    }
    dados.remove(0, pos + 1);
    idx++;
  }
  CONFIG.local = dados;
}

void carregarConfigs() {
  prefs.begin("config", true);
  CONFIG.nomeWifi   = prefs.getString("wifi",   "");
  CONFIG.senhaWifi  = prefs.getString("senha",  "");
  CONFIG.ipBroker   = prefs.getString("ip",     "");
  CONFIG.userBroker = prefs.getString("user",   "");
  CONFIG.passBroker = prefs.getString("pass",   "");
  CONFIG.tempTopic  = prefs.getString("topic",  "");
  CONFIG.local      = prefs.getString("local",  "");
  prefs.end();

  if (CONFIG.tempTopic == "" && CONFIG.local.startsWith("sensores/temperatura/")) {
    CONFIG.tempTopic = CONFIG.local;
    CONFIG.local = "";
  }
  if (CONFIG.tempTopic == "") CONFIG.tempTopic = "sensores/temperatura/interna";

  if (CONFIG.nomeWifi == "") Serial.println("Nenhuma configuração encontrada; entrando em modo AP.");
  else                      Serial.println("Configurações carregadas.");
}

void salvarConfigs() {
  prefs.begin("config", false);
  prefs.putString("wifi",   CONFIG.nomeWifi);
  prefs.putString("senha",  CONFIG.senhaWifi);
  prefs.putString("ip",     CONFIG.ipBroker);
  prefs.putString("user",   CONFIG.userBroker);
  prefs.putString("pass",   CONFIG.passBroker);
  prefs.putString("topic",  CONFIG.tempTopic);
  prefs.putString("local",  CONFIG.local);     // local real
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
    lastMqttPing = millis();
  } else {
    Serial.printf("Falha [%s]\n", mqtt->connectErrorString(rc));
    mqtt->disconnect();
  }
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

  unsigned long now = millis();
  if (now - lastMqttPing >= MQTT_PING_MS) {
    lastMqttPing = now;
    if (!mqtt->ping()) {
      Serial.println("MQTT ping falhou; desconectando para reconectar...");
      mqtt->disconnect();
    }
  }

  // publicar temperatura pendente
  if (publicarTemperatura && (now - tempoUltimaPublication >= TEMP_PUBLISH_MIN_MS)) {
    tempoUltimaPublication = now;
    if (tempPub && !isnan(temperaturaC)) {
      char payload[16];
      dtostrf(temperaturaC, 0, 1, payload); // 1 casa decimal
      bool ok = tempPub->publish(payload);
      Serial.printf("[MQTT] publish %s = %s -> %s\n", CONFIG.tempTopic.c_str(), payload, ok ? "ok" : "falhou");
      if (ok) publicarTemperatura = false;
    }
  }
}

/* -------------------- Temperatura -------------------- */
void scheduleTempPublish() {
  publicarTemperatura = true;
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
  // Página principal
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
    if (modoConfiguracao)
      req->send_P(200, "text/html", paginaConfig);
    else
      req->send_P(200, "text/html", paginaParametros);
  });

  // Configurar (POST CSV): wifi,senha,ip,user,pass,topic,local
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
        if (CONFIG.tempTopic == "") CONFIG.tempTopic = "sensores/temperatura/interna";
        salvarConfigs();
        request->send(200, "text/plain", "OK");
        delay(800);
        ESP.restart();
      }
    }
  );

  // Recuperar configs (CSV) — 7 campos
  server.on("/recuperarConfigs", HTTP_GET, [](AsyncWebServerRequest* request) {
    String dados = CONFIG.nomeWifi + "," + CONFIG.senhaWifi + "," + CONFIG.ipBroker + "," +
                   CONFIG.userBroker + "," + CONFIG.passBroker + "," + CONFIG.tempTopic + "," + CONFIG.local;
    if (CONFIG.nomeWifi == "") request->send(404, "text/plain", "Sem configuracao");
    else                       request->send(200, "text/plain", dados);
  });

  // Temperatura atual (para gauge/UI)
  server.on("/get_temperatura", HTTP_GET, [](AsyncWebServerRequest* request) {
    StaticJsonDocument<96> j;
    if (!isnan(temperaturaC)) j["temp"] = temperaturaC;
    else                      j["temp"] = nullptr;
    String out; serializeJson(j, out);
    request->send(200, "application/json", out);
  });

  // Resetar configs via HTTP
  server.on("/resetar_config", HTTP_POST, [](AsyncWebServerRequest* request) {
    prefs.begin("config", false); prefs.clear(); prefs.end();
    request->send(200, "text/plain", "Reset OK");
    delay(1000);
    ESP.restart();
  });

  server.onNotFound([](AsyncWebServerRequest* request) {
    request->send(404, "text/plain", "Nao encontrado.");
  });
}

/* -------------------- Hard Reset (D3 -> GND 5s) -------------------- */
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
  pinMode(BTN_RESET, INPUT_PULLUP);

  Serial.println("\nBoot...");

  sensors.begin();

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

    if (CONFIG.tempTopic == "") CONFIG.tempTopic = "sensores/temperatura/interna";
    tempPub = new Adafruit_MQTT_Publish(mqtt, CONFIG.tempTopic.c_str());

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

    // litura periódica do DS18B20
    unsigned long now = millis();
    if (now - lastReadMs >= READ_INTERVAL_MS) {
      lastReadMs = now;
      sensors.requestTemperatures();
      float t = sensors.getTempCByIndex(0);
      temperaturaC = t;
      scheduleTempPublish();
      if (t == DEVICE_DISCONNECTED_C) {
        Serial.println("Sensor desconectado.");
      } else {
        temperaturaC = t;
        scheduleTempPublish();
        Serial.printf("Temperatura: %.1f °C | tópico: %s | local: %s\n",
                      temperaturaC, CONFIG.tempTopic.c_str(), CONFIG.local.c_str());
      }
      yield();
    }
  }

  checkBotaoHardReset();
}
