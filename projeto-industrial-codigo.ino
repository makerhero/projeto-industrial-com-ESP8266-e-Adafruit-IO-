/* Programa SUPERMAKER: Projeto Industrial com ESP8266 e Adafruit IO
 * Autor: Eduardo de Castro Quintino
 * Biblioteca da Placa: "esp8266 by ESP8266 Community versão 3.1.2"
 * Bibliotecas:         "Adafruit IO Arduino versão 4.2.3"
 *                      "Adafruit MQTT versão 2.5.2"
 *                      "ArduinoHttpClient versão 0.4.0"
 *                      "DHT sensor library 1.4.4"
 *                      "Adafruit unified sensor 1.1.9"       
 * Placa: "NodeMCU 1.0 (ESP-12E Module)"
*/
// ======================================================================
// --- Dados de Acesso da Plataforma Adafruit IO ---
// visite io.adafruit.com para para criar a sua conta.
// Você necessitará dos dados Adafruit IO key.
#define IO_USERNAME "SuaInfoAIOUSERNAME" //sua informação
#define IO_KEY "SuaInfoAIO_KEY" // sua informação
// ======================================================================
// --- Dados de Acesso do seu roteador ---
#define WIFI_SSID "NomedaSuaRede" // Informação da SSID do seu roteador
#define WIFI_PASS "SenhadaSuaRede" // senha de acesso do seu roteador
#include "AdafruitIO_WiFi.h" // inclui biblioteca
AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, WIFI_SSID, WIFI_PASS);
// ======================================================================
// --- Mapeamento de Hardware ---
#define sensormaquina 5  //D1 - Lê estado da máquina
#define habilitamotor 14 //D5 - Saída a relé
#define DHTPIN 12        //D6 - Pino utilizado pelo DHT22
#define saidapwm 13      //D7 - Saída PWM/analógica
// ======================================================================
// --- Inclusão biblioteca e parâmetros do sensor DHT22 ---
#include <DHT.h>
#define DHTTYPE    DHT22 // Define que o sensor é o DHT22
DHT dht(DHTPIN, DHTTYPE);
// ======================================================================
// --- Feeds ---
//IMPORTANTE devem ser criados os feeds com o mesmo nome na AdafruitIO
AdafruitIO_Feed *estadomaquina = io.feed("estadomaquina"); //pub
AdafruitIO_Feed *temperatura = io.feed("temperatura"); //pub
AdafruitIO_Feed *umidade = io.feed("umidade"); //pub
AdafruitIO_Feed *motor = io.feed("motor"); //sub                  
AdafruitIO_Feed *velocidademotor = io.feed("velocidademotor"); //sub
// ======================================================================
// --- Variáveis Globais ---
bool estadoatualmaquina = false;
bool ultimoestadomaquina = false;
bool flagmotor = LOW;
int valorpwm = 0;
float temperaturaatual = 0;
float umidadeatual = 0;
unsigned int previousMillis = 0;
unsigned int interval = 10000;
// intervalo de tempo para publicação das variáveis temperatura e umidade
// SUGESTÃO: alterar para 60000, visto que são variáveis lentas
// ======================================================================
// --- Void Setup ---
void setup() {
// --- Configuração IO ---
  pinMode(sensormaquina, INPUT);
  pinMode(saidapwm, OUTPUT);     
  pinMode(habilitamotor, OUTPUT);
  Serial.begin(115200); // inicializa serial
  dht.begin(); // Inicializa a função DHT
 
  // aguarda serial ser aberta
  while(! Serial);
 
  // conecta a io.adafruit.com
  Serial.print("conectando com Adafruit IO");
  io.connect();
 
  // cria funções para o tratamento das inscrições (subscribes)
  motor->onMessage(handleMessage);
  velocidademotor->onMessage(handleMessage2);
 
  // aguarda conexão
  while(io.status() < AIO_CONNECTED) {
    Serial.print(".");
    delay(500);
}
 
  // nós estamos conectados
  Serial.println();
  Serial.println(io.statusText());
  motor->get(); // lê status motor
  velocidademotor->get(); // lê percentual % de velocidade para motor
}
// ======================================================================
// --- void loop ---
void loop() {
  //chamada das funções
  io.run();
  tempopublicacao();
 
  // se o feed habilitamotor estiver em 1 (ON),
  // escreve a saída analógica com o valor do feed velocidademotor
  // se o feedhabilitamotor estiver em 0 (OFF),
  // saída analógica em 0
  if(flagmotor == HIGH){
    analogWrite(saidapwm, valorpwm);
  }
  else{
    analogWrite(saidapwm, 0);
  }
 
  // condições para tratamento da leitura do status da máquina
  if(digitalRead(sensormaquina) == HIGH)
    estadoatualmaquina = true;
  else
    estadoatualmaquina = false;
 
  // retorna se o valor não mudou
  if(estadoatualmaquina == ultimoestadomaquina)
    return;
 
  //publica o estado atual da leitura digital onde é lido
  // se a máquina está ligada ou desligada
  estadomaquina->save(estadoatualmaquina);
 
  // armazena o último estado lido
  ultimoestadomaquina = estadoatualmaquina;
}
 
// ======================================================================
// --- Função para tratamento dos feeds ---
// escreve no pino digital do relé o valor que estiver no feed
void handleMessage(AdafruitIO_Data *data) {
  // habilita relé
  Serial.print("recebido <- ");
  
  if(data->toPinLevel() == HIGH)
  {
    Serial.println("HIGH");
    flagmotor = HIGH;
  }
  else
  {
    Serial.println("LOW");
    flagmotor = LOW;
  }
  digitalWrite(habilitamotor, data->toPinLevel());
 
}
 
void handleMessage2(AdafruitIO_Data *data) {
  // converte dado para inteiro
  int reading = data->toInt();
 
  Serial.print("recebido <- ");
  Serial.println(reading);
  // recebe o valor do feed velocidademotor
  // mapeia o valor que vem de 0 a 100 para 0 a 255
  // atribui o resultado para a variável valorpwm
  valorpwm = map(reading, 0, 100, 0, 255);
}
 
// função para publicar "temperatura" e "umidade" a cada intervalo de tempo
// são variáveis lentas, ou seja, dependendo do processo, demoram a ter variações significativas
// sugestão: entender o processo, mas para a minha aplicação 60 segundos estava perfeita
// para o artigo, deixei 10 segundos
void tempopublicacao() {
  if (millis() - previousMillis >= interval){
    previousMillis = millis();
    umidadeatual = dht.readHumidity(); // armazena valor da umidade
temperaturaatual = dht.readTemperature(); // armazena valor da temperatura
    temperatura->save(temperaturaatual); // publica o valor da temperatura
    umidade->save(umidadeatual); //publica o valor da umidade
  }
}
// ======================================================================
// --- Fim do código da programação =) ---
