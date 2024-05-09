/*
  Autor: Tales Amorim - 2024
  Dotseg Segurança Patrimonial LTDA
  Obs:
  Instalar bibliotecas ESP, incluir em preferencias o link: https://arduino.esp8266.com/stable/package_esp8266com_index.json
  Instalar Arduino_JSON
  Selecionar a porta de conexão serial conforme Gerenciador de dispositivos para upload e depuração na placa deve ser apertado para
  disparar a ação de abertura de porta.

  Instruções:
  Configurar o ssid e a senha da rede Wifi que o dispositivo esta presente
  Configurar a senha do sispositivo para o usuário "admin"
  O botão "Flash" ao lado do conector USB

*/

#if defined(ESP8266)
  #include <ESP8266WiFi.h>// Usado para ESP8266
  #include <ESP8266HTTPClient.h>
  #define BUTTON D3// NodeMCU ESP8266 Flash Button
  #define LOW_STATE HIGH
  #define HIGH_STATE LOW
  const int ESP8266 = 1;
#elif defined(ESP32)
  #include <WiFi.h>
  #include <HTTPClient.h>
  #define BUTTON 23// ESP32 0: corresponde ao botão BOOT, 23 é o pino usado pelo projeto Dot
  #define LOW_STATE LOW
  #define HIGH_STATE HIGH
  const int ESP8266 = 0;
#endif
#include <WiFiClient.h>
#include <ArduinoJson.h>



const char* ssid = "AP-DIRETORIA";      // SSID da rede WiFi
const char* password = "dt@44550965092";  // Senha da rede WiFi
const char* senhaDispositivo = "3X#p'8+61rqy\"OU~";    // Token de autenticação da API
const char* apiUrl1 = "http://10.1.9.204/login.fcgi";       // URL da segunda API
const char* apiUrl2 = "http://10.1.9.204/execute_actions.fcgi";       // URL da segunda API
const int ledPin = LED_BUILTIN;  // Pino GPIO conectado ao LED do módulo ESP8266
const char* session = "";
unsigned long interval = 1000;  // Intervalo de piscar do LED (1 segundo)
unsigned long previousMillis = 0;
bool ledState = LOW_STATE;  // Estado inicial do LED

WiFiClient wifiClient;
HTTPClient http;

void setup() {
  pinMode(ledPin, OUTPUT);  // Configura o pino do LED como saída
  pinMode(BUTTON, INPUT_PULLUP); // Initiate the ESP Pin: INPUT_PULLUP - Its mean that you no need put a resistor

  Serial.begin(115200);
  
  // Conectando-se à rede WiFi
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);  // Inicia a conexão com o WiFi
  
  while (WiFi.status() != WL_CONNECTED) {  // Aguarda a conexão ser estabelecida
    digitalWrite(ledPin, HIGH_STATE);  // Liga o LED
    delay(200); // Espera por 0,2 segundo
    Serial.print(".");
    digitalWrite(ledPin, LOW_STATE);   // Desliga o LED
    delay(200); // Espera por 0,2 segundo
  }

  digitalWrite(ledPin, HIGH_STATE);  // Liga o LED para indicar que foi conectado a rede WIFI
  // Conexão bem-sucedida
  Serial.println("");
  Serial.println("Conectado à rede WiFi! ");
  Serial.println(ssid);
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());  // Mostra o endereço IP atribuído ao NodeMCU

  // Solicitação HTTP POST com autenticação básica
  if (WiFi.status() == WL_CONNECTED) {
    //abrePorta();
  }
}

void loop() {
  // Se precisar executar alguma ação repetidamente após a conexão, adicione aqui
  checkButton();
  blinkLed();
}

void blinkLed(){
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    // Salva o tempo atual como referência para o próximo intervalo
    previousMillis = currentMillis;

    // Inverte o estado do LED
    ledState = !ledState;
    if( ESP8266 && !ledState || ledState ){
      interval = 50;
    } else{
      interval = 2000;
    }
    digitalWrite(ledPin, ledState);

    // digitalWrite(ledPin, LOW);  // Liga o LED
    // delay(50);                  // Espera por 1 segundo
    // digitalWrite(ledPin, HIGH);   // Desliga o LED
    // delay(2000);
  }
}

void checkButton(){
  if(digitalRead(BUTTON) == HIGH_STATE){
    digitalWrite(ledPin, !digitalRead(ledPin));//Inverte o estado do led
    Serial.println("Botão Pressionado");
    //Serial.print("Retorno botão autenticação: " + autentica());
    //autentica();
    //Serial.print("Retorno botão autenticação: ");
    abrePorta(autentica());
    delay(300);//Debounce para evitar leituras instáveis
  }
}

String autentica(){
  http.begin(wifiClient, apiUrl1);  // Inicia a conexão HTTP

  // Configura a autenticação básica
  //http.setAuthorization(apiToken.c_str(), "");  // Usuário vazio, senha é o token

  // Configura o cabeçalho da requisição
  http.addHeader("Content-Type", "application/json");
  
  // Monta o JSON com os dados de usuário e senha
  StaticJsonDocument<200> jsonDoc;
  jsonDoc["login"] = "admin";
  jsonDoc["password"] = senhaDispositivo;

  // Serializa o JSON para uma String
  String jsonString;
  serializeJson(jsonDoc, jsonString);

  // Envia a solicitação POST com o JSON no corpo e aguarda a resposta
  int httpResponseCode = http.POST(jsonString);
  
  // Verifica o código de resposta
  if (httpResponseCode > 0) {
    Serial.print("Resposta da API de autenticação: ");
    String payload = http.getString();
    Serial.println(payload);  // Imprime a resposta da API

    // Parseia a resposta JSON da primeira API
    StaticJsonDocument<200> jsonDoc;
    DeserializationError error = deserializeJson(jsonDoc, payload);
    if (!error) {
      // Extrai o valor do parâmetro "session"
      //const char* session = jsonDoc["session"];
      session = jsonDoc["session"];
      Serial.print("Session: ");
      Serial.println(session);
      return session;
    } else {
      Serial.println("Falha ao parsear JSON da API autenticação");
      return "";
    }
  }
  return "";
}

void abrePorta(String token){
  // Prepara o JSON para a segunda API
  StaticJsonDocument<200> jsonBody;
  jsonBody["actions"][0]["action"] = "sec_box";
  jsonBody["actions"][0]["parameters"] = "id=65793,reason=3,timeout=10000";

  // Serializa o JSON para uma String
  String jsonString;
  serializeJson(jsonBody, jsonString);

  // Monta a URL da segunda API com o parâmetro "session"
  String Url2 = String(apiUrl2) + "?session=" + token;

  // Configura a URL da segunda API
  http.begin(wifiClient, Url2);
  http.addHeader("Content-Type", "application/json");

  int httpResponseCode = http.POST(jsonString);

  // Verifica o código de resposta da segunda API comando ao dispositivo
  if (httpResponseCode > 0) {
    Serial.print("Resposta da API comando: ");
    String payload2 = http.getString();
    Serial.println(payload2);  // Imprime a resposta da segunda API
  } else {
    Serial.print("Erro na solicitação HTTP (API comando): ");
    Serial.println(httpResponseCode);
  }

  http.end();  // Fecha a conexão HTTP da API comando
}
