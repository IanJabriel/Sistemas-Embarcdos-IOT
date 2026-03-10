#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// Configurações de Rede
const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* serverUrl = "https://jubilant-space-goggles-jprq9xq56wjhqv9q-8000.app.github.dev/data";

// Pinos
const int pinoRele = 32;
const int pinoBotao = 26; 

// Variáveis de controle
bool estadoRele = false;
bool ultimoEstadoBotao = HIGH;
unsigned long tempoUltimoDebounce = 0;
unsigned long delayDebounce = 50; 

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  Serial.print("Conectando ao WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado!");
}

// Função ajustada: recebe float (número) e monta o JSON com string
void enviarDados(float consumoWatts) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure(); 

    HTTPClient http;
    http.begin(client, serverUrl);
    http.addHeader("Content-Type", "application/json");

    // Montagem do JSON - Convertendo o float para String
    String json = "{";
    json += "\"endereco\": \"Sala Principal\",";
    json += "\"corrente\": " + String(consumoWatts) + ",";
    json += "\"id\": \"1\",";
    json += "\"data\": \"2026/03/10\"";
    json += "}";

    int httpResponseCode = http.POST(json);

    if (httpResponseCode > 0) {
      Serial.printf("Envio OK! Status: %d\n", httpResponseCode);
    } else {
      Serial.printf("Erro no envio: %s\n", http.errorToString(httpResponseCode).c_str());
    }
    http.end();
  }
}

void setup() {
  Serial.begin(115200);
  
  pinMode(pinoRele, OUTPUT);
  digitalWrite(pinoRele, LOW);
  
  pinMode(pinoBotao, INPUT_PULLUP); 
  
  connectWiFi();
  Serial.println("Sistema pronto. Aperte o botão para alternar o Relé.");
}

void loop() {
  // Tenta reconectar se cair, mas sem travar o loop principal
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  int leitura = digitalRead(pinoBotao);

  if (leitura != ultimoEstadoBotao) {
    tempoUltimoDebounce = millis();
  }

  if ((millis() - tempoUltimoDebounce) > delayDebounce) {
    // Lógica para ligar (Botão pressionado e Rele estava desligado)
    if (leitura == LOW && estadoRele == false) {
      estadoRele = true;
      digitalWrite(pinoRele, HIGH); // Liga o LED/Rele imediatamente
      Serial.println(">>> LIGADO via Botão");
      enviarDados(10.5); // Passando um número float
      delay(250); // Debounce extra para evitar repetição
    } 
    // Lógica para desligar (Botão pressionado e Rele estava ligado)
    else if (leitura == LOW && estadoRele == true) {
      estadoRele = false;
      digitalWrite(pinoRele, LOW); // Desliga o LED/Rele imediatamente
      Serial.println(">>> DESLIGADO via Botão");
      enviarDados(0.0); // Passando um número float
      delay(250);
    }
  }

  ultimoEstadoBotao = leitura;
}