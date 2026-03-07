# IoT REST — Sistema de Monitoramento de Consumo Energético

Sistema embarcado de monitoramento de consumo elétrico em tempo real, composto por um **firmware ESP32** que coleta dados de sensores de corrente e uma **API REST (FastAPI)** que recebe, processa e persiste esses dados no **Firebase Realtime Database**.

> **Repositório:** [https://github.com/IanJabriel/IoT_REST](https://github.com/IanJabriel/IoT_REST)

---

## Arquitetura Geral

```
┌──────────────┐        HTTPS POST /dados        ┌──────────────┐       ┌──────────────────┐
│   ESP32      │ ──────────────────────────────► │  FastAPI     │ ────► │ Firebase Realtime│
│  (Firmware)  │   JSON: id, consumo, evento,    │  (api.py)    │       │    Database      │
│              │         status_rele             │              │       │                  │
└──────────────┘                                 └──────────────┘       └──────────────────┘
     │                                                   │
     │ Sensor ADC (pino 34)                              │ GET /dados
     │ Relé (pino 32)                                    ▼
     │                                            Retorna registros
     ▼                                            armazenados
 Leitura de corrente
 + Lógica local de proteção
```

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
  "id": "1",
  "consumo": 42.5,
  "evento": "VARIAÇÃO",
  "status_rele": 1
}
```

| Campo | Tipo | Descrição |
|-------|------|-----------|
| `id` | `string` | Identificador do dispositivo |
| `consumo` | `float` | Consumo em Watts (média de 10 amostras) |
| `evento` | `string` | `"ALERTA"` (sobrecarga) ou `"VARIAÇÃO"` (mudança normal) |
| `status_rele` | `int` | Estado do relé: `1` (ligado) ou `0` (desligado) |

### Bibliotecas Arduino Utilizadas

- `WiFi.h` — Conexão à rede WiFi
- `HTTPClient.h` — Requisições HTTP
- `WiFiClientSecure.h` — HTTPS com TLS

---

## API REST (FastAPI)

### Visão Geral

A API recebe os dados do ESP32 via POST, adiciona um timestamp no fuso horário de São Paulo e persiste o registro no Firebase Realtime Database.

### Endpoints

#### `POST /dados`

Recebe dados do dispositivo IoT.

**Body (JSON):**

```json
{
  "id": "1",
  "consumo": 42.5,
  "evento": "VARIAÇÃO",
  "status_rele": 1
}
```

**Processamento interno:**
- Gera timestamp automático no fuso `America/Sao_Paulo` (formato `YYYY-MM-DD HH:MM:SS`).
- Monta o registro com os campos `endereco`, `data`, `corrente` e `dispositivo_id`.
- Persiste no Firebase via `push()` (chave automática).

**Resposta (200):**

```json
{
  "message": "Dados recebidos com sucesso",
  "registro": {
    "endereco": null,
    "data": "2026-03-06 14:30:00",
    "corrente": null,
    "dispositivo_id": "1"
  },
  "chave": "-NxAbCdEfGhIjKlMnO"
}
```

#### `GET /dados`

Retorna todos os registros armazenados no Firebase.

**Resposta (200):**

```json
{
  "message": "Dados do Firebase",
  "database": [
    {
      "_key": "-NxAbCdEfGhIjKlMnO",
      "endereco": null,
      "data": "2026-03-06 14:30:00",
      "corrente": null,
      "dispositivo_id": "1"
    }
  ]
}
```

---

## Firebase Connector

Módulo (`firebase_connector.py`) que encapsula a conexão e as operações com o Firebase Realtime Database.

### Classe `FirebaseConnector`

| Método | Descrição |
|--------|-----------|
| `__init__(database_url, credentials_path?, credentials_env?)` | Inicializa a conexão com o Firebase. Aceita credenciais via arquivo JSON ou variável de ambiente. |
| `push(path, data) → str` | Insere um novo registro com chave automática no caminho especificado. Retorna a chave gerada. |
| `get_all(path) → list[dict]` | Retorna todos os registros de um caminho como lista de dicionários (inclui `_key`). |

### Autenticação

O conector exige um arquivo **Service Account Key** (`serviceAccountKey.json`) do Firebase. Esse arquivo deve estar na raiz do projeto e **não deve ser versionado** (adicionar ao `.gitignore`).

---

## Como Rodar

### Pré-requisitos

- Python 3.11+
- Arquivo `serviceAccountKey.json` do Firebase na raiz do projeto
- (Opcional) ngrok ou túnel similar para expor a API ao ESP32

### 1. Criar e ativar o ambiente virtual

```powershell
python -m venv venv
.\venv\Scripts\Activate.ps1
```

### 2. Instalar dependências

```powershell
pip install -r requirements.txt
```

### 3. Iniciar o servidor

```powershell
uvicorn api:app --reload
```

A API estará disponível em `http://localhost:8000`.

A documentação interativa (Swagger) pode ser acessada em `http://localhost:8000/docs`.

### 4. (Opcional) Expor com ngrok

```powershell
ngrok http 8000
```

Copie a URL gerada e atualize a variável `serverUrl` no firmware do ESP32.

---

## Estrutura do Projeto

```
IoT_REST/
├── api.py                  # API REST (FastAPI) — endpoints POST e GET /dados
├── firebase_connector.py   # Conector Firebase Realtime Database
├── requirements.txt        # Dependências Python
├── serviceAccountKey.json  # Credenciais Firebase (NÃO versionar)
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
| API | Python 3.11 + FastAPI + Uvicorn |
| Banco de Dados | Firebase Realtime Database |
| Autenticação Firebase | firebase-admin SDK |

---

## Dependências Python

```
fastapi==0.135.1
firebase==4.0.1
firebase-admin==7.2.0
uvicorn==0.41.0
```
