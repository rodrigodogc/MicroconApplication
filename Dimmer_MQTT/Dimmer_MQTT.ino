#include <OneWire.h>
#include <DallasTemperature.h>
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

#define PINO_ONEWIRE D1
#define LED D4
#define PWM_PINO D2

/*  configurações  */
#define PORTA_BROKER 1883
typedef struct {
  String nomeWifi;
  String senhaWifi;
  String ipBroker;
  String userBroker;
  String passBroker;
  String local;
} Config;
Config CONFIG;

Preferences prefs;

/*  globais  */
WiFiClient netClient;
AsyncWebServer server(80);

unsigned int DutyciclePWM = 50;
unsigned int FrequenciaPWM = 5000;

DNSServer dnsServer;
const byte DNS_PORT = 53;
bool modoConfiguracao = false;

Adafruit_MQTT_Client* mqtt = nullptr;
Adafruit_MQTT_Publish* Temperatura = nullptr;
// Assinatura via ponteiro (criada após instanciar o cliente)
Adafruit_MQTT_Subscribe* velocidadeCmd = nullptr;

OneWire oneWire(PINO_ONEWIRE);
DallasTemperature sensors(&oneWire);

float temperaturaC = 0.0;

/* Timers */
unsigned long previousMillis = 0;
const long delayLoop = 2000;

/* --- Prototypes --- */
void updateConfigs(String dados);
void carregarConfigs();
void salvarConfigs();
void MQTT_connect();
void iniciarModoAP();
void iniciarRotasHTTP();
void onSubscribed(Adafruit_MQTT_Subscribe* sub);
void onVelocidadeMsg(const char* payload);

/* Atualiza a struct CONFIG  */
void updateConfigs(String dados) {
  int idx = 0;
  while (dados.indexOf(',') != -1) {
    int pos = dados.indexOf(',');
    String parte = dados.substring(0, pos);
    switch (idx) {
      case 0: CONFIG.nomeWifi = parte; break;
      case 1: CONFIG.senhaWifi = parte; break;
      case 2: CONFIG.ipBroker = parte; break;
      case 3: CONFIG.userBroker = parte; break;
      case 4: CONFIG.passBroker = parte; break;
    }
    dados.remove(0, pos + 1);
    idx++;
  }
  CONFIG.local = dados;
}

/* Carrega CONFIG de Preferences */
void carregarConfigs() {
  prefs.begin("config", true);
  CONFIG.nomeWifi = prefs.getString("wifi", "");
  CONFIG.senhaWifi = prefs.getString("senha", "");
  CONFIG.ipBroker = prefs.getString("ip", "");
  CONFIG.userBroker = prefs.getString("user", "");
  CONFIG.passBroker = prefs.getString("pass", "");
  CONFIG.local = prefs.getString("local", "");
  prefs.end();

  if (CONFIG.nomeWifi == "") {
    Serial.println("Nenhuma configuração encontrada; entrando em modo AP.");
  } else {
    Serial.println("Configurações carregadas.");
  }
}

/* Salva CONFIG em Preferences  */
void salvarConfigs() {
  prefs.begin("config", false);  // gravação
  prefs.putString("wifi", CONFIG.nomeWifi);
  prefs.putString("senha", CONFIG.senhaWifi);
  prefs.putString("ip", CONFIG.ipBroker);
  prefs.putString("user", CONFIG.userBroker);
  prefs.putString("pass", CONFIG.passBroker);
  prefs.putString("local", CONFIG.local);
  prefs.end();
  Serial.println("Configurações salvas.");
}

/* Callback: confirmação de inscrição */
void onSubscribed(Adafruit_MQTT_Subscribe* sub) {
  if (sub == velocidadeCmd) {
    Serial.println("[MQTT] Inscrito com sucesso em 'ventilador/velocidade'");
  } else {
    Serial.println("[MQTT] Inscrição concluída em outro tópico");
  }
}
/* Callback  do mqtt */
void onVelocidadeMsg(const char* payload) {
  // Aceita 0–255 ou 0–100% (com ou sem '%')
  int v = atoi(payload);
  bool temPercent = strchr(payload, '%') != nullptr;
  if (temPercent || (v >= 0 && v <= 100)) {
    v = map(v, 0, 100, 0, 255);
  }
  if (v < 0) v = 0;
  if (v > 255) v = 255;

  DutyciclePWM = (unsigned)v;
  analogWrite(PWM_PINO, v);

  Serial.printf(
    "[MQTT] velocidade recebida: %s -> PWM=%u (%.1f%%)\n", payload, DutyciclePWM, (100.0 / 255.0) * DutyciclePWM);
}

/*  MQTT reconexão */
void MQTT_connect() {
  if (!mqtt || mqtt->connected()) return;

  Serial.print("Conectando ao Broker MQTT... ");
  int8_t rc;
  uint8_t tentativas = 3;
  while ((rc = mqtt->connect()) != 0) {
    Serial.printf("Falha [%s], tentando de novo...\n", mqtt->connectErrorString(rc));
    mqtt->disconnect();
    delay(5000);
    if (--tentativas == 0) { ESP.restart(); }
  }
  Serial.println("Conectado.");

  // Reinscreve após reconectar
  if (velocidadeCmd) {
    if (mqtt->subscribe(velocidadeCmd)) {
      onSubscribed(velocidadeCmd);  // callback de inscrição feita
    } else {
      Serial.println("[MQTT] ERRO ao tentar se inscrever em 'ventilador/velocidade'");
    }
  }
}

/* Access-Point para configuração */
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

/* Rotas HTTP  */
void iniciarRotasHTTP() {

  server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
    if (modoConfiguracao)
      req->send_P(200, "text/html", paginaConfig);
    else
      req->send_P(200, "text/html", paginaDimmer);
  });

  server.on(
    "/configurar", HTTP_POST, [](AsyncWebServerRequest* request) {}, NULL,
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
    });

  server.on("/recuperarConfigs", HTTP_GET, [](AsyncWebServerRequest* request) {
    String dados = CONFIG.nomeWifi + "," + CONFIG.senhaWifi + "," + CONFIG.ipBroker + "," + CONFIG.userBroker + "," + CONFIG.passBroker + "," + CONFIG.local;

    if (CONFIG.nomeWifi == "")
      request->send(404, "text/plain", "Sem configuracao");
    else {
      request->send(200, "text/plain", dados);
    }
  });

  server.on("/get_temperatura", HTTP_GET, [](AsyncWebServerRequest* request) {
    JsonDocument dadoJson;
    String dadoString;

    dadoJson["temp"] = String(temperaturaC);
    serializeJson(dadoJson, dadoString);

    request->send(200, "application/json", dadoString);
  });

  server.on("/set_dutycicle", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (request->hasParam("dutycicle")) {
      String duty = request->getParam("dutycicle")->value();
      unsigned int dutyint = duty.toInt();
      if (dutyint <= 255) {
        DutyciclePWM = dutyint;
        analogWrite(PWM_PINO, dutyint);
        Serial.print("Dutycicle setado: ");
        Serial.println(dutyint);
        request->send(200, "text/plain", "PWM setado para " + String((100.0 / 255.0) * (int)dutyint) + "%");
      } else {
        request->send(400, "text/plain", "Valor de dutycicle fora do range.");
      }
    } else {
      request->send(400, "text/plain", "FALTA O PARAMETRO dutycicle");
    }
  });

  server.on("/set_frequencia", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (request->hasParam("frequencia")) {
      String freq = request->getParam("frequencia")->value();
      unsigned int freqint = freq.toInt();
      if (freqint >= 1 && freqint <= 20000) {
        FrequenciaPWM = freqint;
        analogWriteFreq(freqint);
        Serial.print("Frequencia setada: ");
        Serial.println(freqint);
        request->send(200, "text/plain", "Frequencia modificada para " + String(freqint) + "Hz");
      } else {
        request->send(400, "text/plain", "Valor de frequencia fora do range.");
      }
    } else {
      request->send(400, "text/plain", "FALTA O PARAMETRO frequencia");
    }
  });

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

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  pinMode(PWM_PINO, OUTPUT);
  analogWriteFreq(FrequenciaPWM);
  analogWriteRange(255);  // usar escala 0–255 no ESP8266

  sensors.begin();

  Serial.println("\nBoot...");

  carregarConfigs();  // carrega as confgis wifi salvas no pref

  WiFi.mode(WIFI_STA);
  WiFi.begin(CONFIG.nomeWifi.c_str(), CONFIG.senhaWifi.c_str());
  Serial.printf("Conectando a %s", CONFIG.nomeWifi.c_str());
  // timeout de 15s
  unsigned long prevTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - prevTime < 15000) {
    delay(500);
    Serial.print('.');
  }

  iniciarRotasHTTP();

  if (WiFi.status() != WL_CONNECTED) {
    iniciarModoAP();
  } else {
    modoConfiguracao = false;
    Serial.printf("\nConectado! IP: %s\n", WiFi.localIP().toString().c_str());

    /* Conexão no MQTT */
    mqtt = new Adafruit_MQTT_Client(
      &netClient, CONFIG.ipBroker.c_str(), PORTA_BROKER,
      CONFIG.userBroker.c_str(),
      CONFIG.passBroker.c_str());

    velocidadeCmd = new Adafruit_MQTT_Subscribe(mqtt, "ventilador/velocidade", MQTT_QOS_1);

    MQTT_connect();
  }

  server.begin();
}

void loop() {

  if (modoConfiguracao) {
    dnsServer.processNextRequest();
  } else {
    MQTT_connect();

    if (mqtt) {
      // Espera mensagens por até 100ms
      Adafruit_MQTT_Subscribe* sub = mqtt->readSubscription(100);
      if (sub == velocidadeCmd) {
        onVelocidadeMsg((char*)velocidadeCmd->lastread);
      }
      
      if (!mqtt->ping()) {
        mqtt->disconnect();
      }
    }
  }

  unsigned long millisNow = millis();
  if (millisNow - previousMillis >= delayLoop) {
    previousMillis = millisNow;

    // leitura de temperatura (opcional)
    sensors.requestTemperatures();
    temperaturaC = sensors.getTempCByIndex(0);
  }
}
