#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include <DNSServer.h>
#include <Preferences.h>
#include "ConfigHTML.h"
#include "ParametrosHTML.h"

#define PINO_ONEWIRE D1
#define LED D4

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
AsyncWebSocket ws("/ws");  // WS só para temperatura
DNSServer dnsServer;
const byte DNS_PORT = 53;
bool modoConfiguracao = false;

Adafruit_MQTT_Client* mqtt = nullptr;
Adafruit_MQTT_Publish* Temperatura = nullptr;

OneWire oneWire(PINO_ONEWIRE);
DallasTemperature sensors(&oneWire);

/* Timers */
unsigned long previousMillis = 0;
const long delayLoop = 2000;

float temperaturaC = 0.0;

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
}

/* Access-Point para configuração */
void iniciarModoAP() {
  modoConfiguracao = true;
  WiFi.softAP("Termometro-Config", "12345678");
  IPAddress ip = WiFi.softAPIP();
  Serial.printf("AP iniciado. IP: %s\n", ip.toString().c_str());

  dnsServer.start(DNS_PORT, "*", ip);
  server.onNotFound([](AsyncWebServerRequest* response) {
    response->send_P(200, "text/html", paginaConfig);
  });
}

/* Rotas HTTP  */
void registrarRotasHTTP() {

  server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
    if (modoConfiguracao)               
      req->send_P(200, "text/html", paginaConfig);
    else                                     
      req->send_P(200, "text/html", paginaParametros);
  });

  server.on("/configurar", HTTP_POST, [](AsyncWebServerRequest* request) {}, NULL, 
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

  server.on("/recuperarConfigs", HTTP_GET, [](AsyncWebServerRequest* response) {
    String dados = CONFIG.nomeWifi + "," + CONFIG.senhaWifi + "," + CONFIG.ipBroker + "," + CONFIG.userBroker + "," + CONFIG.passBroker + "," + CONFIG.local;

    if (CONFIG.nomeWifi == "")
      response->send(404, "text/plain", "Sem configuracao");
    else {
      response->send(200, "text/plain", dados);   
    }
  });

  server.on("/resetar_config", HTTP_POST, [](AsyncWebServerRequest* response) {
    prefs.begin("config", false);
    prefs.clear();
    prefs.end();
    response->send(200, "text/plain", "Reset OK");
    delay(1000);
    ESP.restart();
  });

  server.onNotFound([](AsyncWebServerRequest* response) {
    response->send(404, "text/plain", "Nao encontrado.");
  });
}

/* só para ter o que for recebido via websocket */
void wsEvent(AsyncWebSocket*, AsyncWebSocketClient* c, AwsEventType tipo, void* arg, uint8_t* data, size_t len) {
  if (tipo == WS_EVT_DATA) {
    AwsFrameInfo* info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
      data[len] = '\0';  // fecha a string
      Serial.printf("Recebido pelo websocket: %s\n", (char*)data);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  Serial.println("\nBoot...");

  carregarConfigs();  // carrega as confgis wifi salvas no pref

  WiFi.mode(WIFI_STA);
  WiFi.begin(CONFIG.nomeWifi.c_str(), CONFIG.senhaWifi.c_str());
  Serial.printf("Conectando a %s", CONFIG.nomeWifi.c_str());
  // aque iniciamos um timeout de 15s se não conectar ele simplesmente sai do loop e provavelmente cai na pagina de ap
  unsigned long prevTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - prevTime < 15000) {
    delay(500);
    Serial.print('.');
  }

  registrarRotasHTTP();
  ws.onEvent(wsEvent);
  server.addHandler(&ws);

  if (WiFi.status() != WL_CONNECTED) {
    iniciarModoAP();
  } else {
    modoConfiguracao = false;
    Serial.printf("\nConectado! IP: %s\n", WiFi.localIP().toString().c_str());

    mqtt = new Adafruit_MQTT_Client(&netClient, CONFIG.ipBroker.c_str(), PORTA_BROKER,
                                    CONFIG.userBroker.c_str(), CONFIG.passBroker.c_str());
    Temperatura = new Adafruit_MQTT_Publish(mqtt, "/temperatura2");
    MQTT_connect();
  }

  server.begin();
  sensors.begin();
}

void loop() {

  if (modoConfiguracao)
    dnsServer.processNextRequest();
  else {
    MQTT_connect();
    if (mqtt)
      mqtt->processPackets(10);
  }

  unsigned long millisNow = millis();
  if (millisNow - previousMillis >= delayLoop) {
    previousMillis = millisNow;
    sensors.requestTemperatures();
    temperaturaC = sensors.getTempCByIndex(0);

    if (isnan(temperaturaC) || temperaturaC == -127.00) {
      Serial.println("Sensor desconectado");
    } else {
      String tempC = String(temperaturaC);
      // Serial.printf("Temperatura: %s °C\n", tempC.c_str());
      ws.textAll(tempC);  // joga no websocket a temperatura
      if (Temperatura) {  // e publica no broker
        Temperatura->publish(temperaturaC);
      }
    }
  }

  ws.cleanupClients();
}
