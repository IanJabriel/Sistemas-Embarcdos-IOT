# API IoT - Sistemas Embarcados

API REST desenvolvida com **FastAPI** para receber e consultar dados de sensores IoT (corrente elétrica), integrada com dispositivos Arduino.

> **Nova versão da API disponível em:** [https://github.com/IanJabriel/IoT_REST](https://github.com/IanJabriel/IoT_REST)

---

## Endpoints

### `POST /dados`

Recebe dados do sensor IoT.

**Body (JSON):**

```json
{
  "endereco": "sensor_01",
  "data": "2026-02-24",
  "corrente": 12.5,
  "id": 1
}
```

- O campo `data` aceita os formatos `YYYY/MM/DD` e `YYYY-MM-DD`.
- A data é convertida automaticamente para o formato `DD/MM/YYYY`.

**Resposta:**

```json
{
  "message": "Dados recebidos com sucesso",
  "registro": {
    "endereco": "sensor_01",
    "data": "24/02/2026",
    "corrente": 12.5,
    "id": 1
  }
}
```

### `GET /dados`

Retorna todos os registros armazenados.

**Resposta:**

```json
{
  "message": "Endpoint para obter dados",
  "database": []
}
```

---

## Como rodar

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

---

## Tecnologias

- **Python 3.11**
- **FastAPI**
- **Uvicorn**

---

## Estrutura do Projeto

```
API-IoT/
├── api.py              # Código principal da API
├── requirements.txt    # Dependências do projeto
├── venv/               # Ambiente virtual Python
└── README.md
```
