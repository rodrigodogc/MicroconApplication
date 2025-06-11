/* Inclusão de Bibliotecas */
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

/* Definições de Pinos */
#define PINO_ONEWIRE D1
#define LED D4

/* Configurações de WiFi */
#define REDE_WIFI "Aplicacoes"
#define SENHA_WIFI "12345678"

/* Configurações do MQTT */
#define IP_BROKER "192.168.0.100"
#define PORTA_BROKER 1883
#define USUARIO_BROKER "admin"
#define SENHA_BROKER "admin"

/* Instância de Objetos */
WiFiServer server(80);
OneWire oneWire(PINO_ONEWIRE);
DallasTemperature sensors(&oneWire);

/* Inicialização do MQTT */
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, IP_BROKER, PORTA_BROKER, USUARIO_BROKER, SENHA_BROKER);
Adafruit_MQTT_Publish Temperatura = Adafruit_MQTT_Publish(&mqtt, "/temperatura2");

/* Função para conexão com o Broker MQTT */
void MQTT_connect() {
  if (mqtt.connected()) {
    return;
  }
  Serial.print("Conectando ao Broker MQTT... ");

  int8_t retorno;
  uint8_t tentativas = 3;
  while ((retorno = mqtt.connect()) != 0) { // Se a conexão der certo, vai retornar 0, se não entra nisso
    Serial.println("Falha ao conectar no Broker MQTT, reconectando... [Erro " + String(mqtt.connectErrorString(retorno)) + " ]");
    mqtt.disconnect(); // Garante a desconexão
    delay(5000);
    tentativas--; // Decrementa o contador de tentativas de reconexão
    if (tentativas == 0) {
      while (1); // Força um reset via estouro do watchdog timer
    }
  }
  Serial.println("Conectado ao Broker MQTT.");
}

/* Função para conexão com WiFi */
void WiFi_Connect() {
  WiFi.mode(WIFI_STA); // Modo cliente
  WiFi.begin(REDE_WIFI, SENHA_WIFI);
  Serial.println("\nConectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
    digitalWrite(LED, !digitalRead(LED));
  }
  Serial.print("\nConectado a " + WiFi.SSID() + " | IP obtido: ");
  Serial.println(WiFi.localIP());
}

/* Esse método é rodado uma vez quando a esp inicializa */
void setup() {
  Serial.begin(115200); // Inicializa a porta serial
  pinMode(LED, OUTPUT);

  sensors.begin(); // Inicializa o objeto do sensor de temperatura

  WiFi_Connect(); // Inicializa a conexão WiFi
  MQTT_connect(); // Tenta conectar com Broker MQTT
  server.begin(); // Inicializao WebServer
}

/* Esse método vai rodar infinitamente enquanto a ESP estiver ligada */
void loop() {
  /* Atualiza a leitura de temperatura do sensor DS18B20 */
  sensors.requestTemperatures(); // Requisita via onewire que o sensor mande a temperatura atualizada
  float temperaturaCelsius = sensors.getTempCByIndex(0); // Converte a temperatura respondida pelo sensor para Celsius
  Serial.println(temperaturaCelsius);

  /* Publica a temperatura lida no tópico MQTT /temperatura */
  Temperatura.publish(temperaturaCelsius); 

  delay(1500);
}




