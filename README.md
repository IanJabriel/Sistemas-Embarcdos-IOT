# IoT REST — Sistema de Monitoramento de Consumo Energético

Sistema embarcado de monitoramento de consumo elétrico em tempo real, composto por um **firmware ESP32** que coleta dados de sensores de corrente, uma **API REST (FastAPI)** que recebe, processa e persiste esses dados no **Firebase Realtime Database**, e uma **dashboard TagoIO** para visualização em tempo real.

> **Repositório:** [https://github.com/IanJabriel/IoT_REST](https://github.com/IanJabriel/IoT_REST)

---

## Arquitetura Geral

```
                                                                       ┌──────────────────┐
                                                                  ┌──► │ Firebase Realtime│
┌──────────────┐       HTTPS POST /data       ┌──────────────┐    │    │    Database      │
│   ESP32      │ ───────────────────────────► │  FastAPI     │ ───┤    └──────────────────┘
│  (Firmware)  │   JSON: endereco, corrente,  │  (api.py)    │    │
│              │         id                   │              │    │    ┌──────────────────┐
└──────────────┘                              └──────────────┘    └──► │  TagoIO Dashboard│
     │                                               │                 │  (tempo real)    │
     │ Sensor ADC (pino 34)                          │ GET /data       └──────────────────┘
     │ Relé (pino 32)                                ▼
     │                                         Retorna registros
     ▼                                         armazenados
 Leitura de corrente
 + Lógica local de proteção
```

### Fluxo de Dados

1. O **ESP32** (ou simulador) envia dados via `POST /data`
2. A API registra o payload no **Firebase Realtime Database** (caminho `/register`)
3. Em background, os mesmos dados são enviados ao **TagoIO** para visualização na dashboard
4. O endpoint `GET /data` permite consultar todos os registros armazenados

---

## Dashboard TagoIO

A plataforma **TagoIO** é utilizada para visualização dos dados em tempo real. A dashboard exibe 4 widgets:

| Widget | Tipo | Descrição |
|--------|------|-----------|
| **Valor em Tempo Real** | Display digital | Mostra o valor instantâneo da corrente em Watts |
| **Medidor Watts** | Gauge (velocímetro) | Medidor analógico com escala de 0–120 W e zona de alerta em vermelho |
| **Mapa de Monitoramento** | Tabela paginada | Lista todos os registros com colunas `dispositivo_id`, `endereco` e `corrente` (em Watts), com paginação |
| **Evolução Computada** | Gráfico de linha | Histórico temporal da corrente (Watts) ao longo dos dias, com linha de limite máximo (~50 W) em vermelho |

Os dados chegam à TagoIO automaticamente via `tagoio_connector.py` a cada `POST /data` recebido pela API.

---

## Firmware ESP32

Código embarcado responsável por ler o consumo energético, aplicar regras de segurança locais e enviar dados para a API na nuvem.

### Pinos Utilizados

| Pino | Função | Descrição |
|------|--------|-----------|
| `34` | Entrada ADC | Leitura analógica do sensor de corrente |
| `32` | Saída digital | Controle do relé (ligar/desligar carga) |

### Constantes de Configuração

| Constante | Valor | Descrição |
|-----------|-------|-----------|
| `LIMITE_WATTS` | `50.0 W` | Limite de consumo — ao ultrapassar, o relé é desligado localmente |
| `TOLERANCIA_ENVIO` | `5.0 W` | Variação mínima para disparar um envio à API |
| `INTERVALO_FORCADO` | `60000 ms` | Intervalo máximo entre envios (1 minuto), mesmo sem variação |

### Fluxo de Operação

1. **Conexão WiFi não-bloqueante:** A função `connectWiFi()` tenta reconectar sem travar o `loop()`, garantindo que a lógica de proteção local continue funcionando mesmo offline.

2. **Pré-processamento:** A função `lerConsumoMedio()` calcula a média de 10 amostras do ADC, convertendo o valor bruto (0–4095) para uma escala de Watts (0–100W).

3. **Lógica de decisão local (proteção contra sobrecarga):**
   - Se `consumoAtual > LIMITE_WATTS` → desliga o relé imediatamente e marca anomalia.
   - Se `consumoAtual < 5.0 W` → espaço reservado para lógica customizável.

4. **Envio seletivo para a API:** Os dados são enviados somente quando:
   - Uma anomalia foi detectada (`evento: "ALERTA"`);
   - A variação do consumo ultrapassou a tolerância (`evento: "VARIAÇÃO"`);
   - O intervalo máximo de tempo foi atingido.

5. **Ciclo de amostragem:** O `loop()` executa a cada **2 segundos**, independente da frequência de envio.

### Payload JSON Enviado

```json
{
  "endereco": "sensor_01",
  "corrente": 12.5,
  "id": "1"
}
```

| Campo | Tipo | Descrição |
|-------|------|-----------|
| `endereco` | `string` | Identificador/localização do sensor |
| `corrente` | `float` | Valor da corrente elétrica medida |
| `id` | `string` | Identificador do dispositivo |

### Bibliotecas Arduino Utilizadas

- `WiFi.h` — Conexão à rede WiFi
- `HTTPClient.h` — Requisições HTTP
- `WiFiClientSecure.h` — HTTPS com TLS

---

## API REST (FastAPI)

### Visão Geral

A API recebe os dados do ESP32 via POST, adiciona um timestamp no fuso horário de São Paulo, persiste o registro no Firebase Realtime Database e envia os dados ao TagoIO em background para visualização na dashboard.

### Endpoints

#### `POST /data`

Recebe dados do sensor IoT e armazena no Firebase. Os dados também são enviados ao TagoIO em background.

**Body (JSON):**

```json
{
  "endereco": "sensor_01",
  "corrente": 12.5,
  "id": "1"
}
```

| Campo | Tipo | Descrição |
|-------|------|-----------|
| `endereco` | `string` | Identificador/localização do sensor |
| `corrente` | `float` | Valor da corrente elétrica medida |
| `id` | `string` | ID do dispositivo |

> O campo `data` (timestamp) é gerado automaticamente pela API no fuso `America/Sao_Paulo`, formato `YYYY-MM-DD HH:MM:SS`.

**Resposta (200):**

```json
{
  "message": "Dados recebidos com sucesso",
  "payload": {
    "endereco": "sensor_01",
    "data": "2026-03-07 14:30:00",
    "corrente": 12.5,
    "dispositivo_id": "1"
  },
  "chave": "-NxAbC123def"
}
```

#### `GET /data`

Retorna todos os registros armazenados no Firebase.

**Resposta (200):**

```json
{
  "message": "Dados do Firebase",
  "database": [
    {
      "_key": "-NxAbC123def",
      "corrente": 12.5,
      "data": "2026-03-07 14:30:00",
      "dispositivo_id": "1",
      "endereco": "sensor_01"
    }
  ]
}
```

---

## Módulos

### `firebase_connector.py`

Classe `FirebaseConnector` que encapsula a comunicação com o Firebase Realtime Database:

| Método | Descrição |
|--------|-----------|
| `__init__(database_url, credentials_path?, credentials_env?)` | Inicializa a conexão com o Firebase. Aceita credenciais via arquivo JSON ou variável de ambiente. |
| `push(path, data) → str` | Insere um novo registro com chave automática no caminho especificado. Retorna a chave gerada. |
| `get_all(path) → list[dict]` | Retorna todos os registros de um caminho como lista de dicionários (inclui `_key`). |

### `tagoio_connector.py`

Módulo responsável pelo envio de dados para a plataforma TagoIO:

| Função | Descrição |
|--------|-----------|
| `send_to_tago(payload)` | Envia as variáveis `corrente`, `endereco` e `dispositivo_id` para o endpoint da TagoIO via HTTP POST |
| `send_to_tago_async(payload)` | Versão assíncrona com thread para envio não-bloqueante |

### Autenticação

O conector Firebase exige um arquivo **Service Account Key** (`serviceAccountKey.json`). Esse arquivo deve estar na raiz do projeto e **não deve ser versionado** (adicionado ao `.gitignore`).

---

## Variáveis de Ambiente

As configurações são carregadas de um arquivo `.env` na raiz do projeto (use `.env.example` como template):

| Variável | Obrigatória | Descrição |
|----------|-------------|-----------|
| `FIREBASE_DATABASE_URL` | Sim | URL do Firebase Realtime Database |
| `FIREBASE_CREDENTIALS_PATH` | Sim* | Caminho para o arquivo `serviceAccountKey.json` |
| `TAGO_DEVICE_TOKEN` | Sim | Token do dispositivo na plataforma TagoIO |
| `TAGO_ENDPOINT` | Não | URL da API TagoIO (padrão: `https://api.eu-w1.tago.io/data`) |

> *Alternativamente, pode-se usar `credentials_env` para passar as credenciais como JSON via variável de ambiente.

---

## Como Rodar

### Pré-requisitos

- Python 3.10+
- Conta no [Firebase](https://console.firebase.google.com/) com Realtime Database habilitado
- Conta no [TagoIO](https://tago.io/) com dispositivo configurado
- Arquivo `serviceAccountKey.json` do Firebase (chave da conta de serviço)

### 1. Clonar o repositório

```bash
git clone https://github.com/IanJabriel/IoT_REST.git
cd IoT_REST
```

### 2. Criar e ativar o ambiente virtual

```bash
python -m venv .venv

# Windows
.venv\Scripts\activate

# Linux/macOS
source .venv/bin/activate
```

### 3. Instalar dependências

```bash
pip install -r requirements.txt
```

### 4. Configurar variáveis de ambiente

Crie um arquivo `.env` na raiz do projeto:

```env
FIREBASE_DATABASE_URL=https://seu-projeto.firebaseio.com
FIREBASE_CREDENTIALS_PATH=serviceAccountKey.json
TAGO_DEVICE_TOKEN=seu-token-do-dispositivo-tago
TAGO_ENDPOINT=https://api.eu-w1.tago.io/data
```

> Coloque o arquivo `serviceAccountKey.json` na raiz do projeto.

### 5. Iniciar o servidor

```bash
uvicorn api:app --reload
```

A API estará disponível em `http://localhost:8000`.

A documentação interativa (Swagger) pode ser acessada em `http://localhost:8000/docs`.

### 6. (Opcional) Expor com ngrok

```bash
ngrok http 8000
```

Copie a URL gerada e atualize a variável `serverUrl` no firmware do ESP32.

---

## Estrutura do Projeto

```
IoT_REST/
├── api.py                  # Servidor FastAPI — endpoints POST e GET /data
├── firebase_connector.py   # Conector do Firebase Realtime Database
├── tagoio_connector.py     # Envio de dados ao TagoIO
├── requirements.txt        # Dependências do projeto
├── serviceAccountKey.json  # Credenciais do Firebase (NÃO versionar)
├── .env                    # Variáveis de ambiente (NÃO versionar)
├── .env.example            # Template das variáveis de ambiente
├── README.md               # Esta documentação
└── firmware/
    └── esp32.ino           # Firmware do ESP32 (código embarcado)
```

---

## Tecnologias

| Camada | Tecnologia |
|--------|------------|
| Microcontrolador | ESP32 (Arduino Framework) |
| Comunicação | WiFi + HTTPS |
| API | Python 3.10+ · FastAPI · Uvicorn |
| Banco de Dados | Firebase Realtime Database |
| Autenticação Firebase | firebase-admin SDK |
| Dashboard IoT | TagoIO |
| Variáveis de Ambiente | python-dotenv |

---

## Dependências Python

```
fastapi==0.135.1
firebase==4.0.1
firebase-admin==7.2.0
python-dotenv==1.1.0
requests==2.32.3
uvicorn==0.41.0
```
