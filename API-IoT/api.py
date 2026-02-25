from fastapi import FastAPI
from typing import List
from datetime import datetime

app = FastAPI()

database: List[dict] = []

@app.post("/dados")
async def receber_dados(dados: dict):
    data_str = dados.get("data")

    # Aceita tanto "2026/02/24" quanto "2026-02-24"
    for fmt in ("%Y/%m/%d", "%Y-%m-%d"):
        try:
            data_obj = datetime.strptime(data_str, fmt)
            break
        except ValueError:
            continue
    else:
        return {"error": f"Formato de data inválido: {data_str}"}

    data_formatada = data_obj.strftime("%d/%m/%Y")

    registro = {
        "endereco": dados.get("endereco"),   # sem acento, igual ao Arduino
        "data": data_formatada,
        "corrente": dados.get("corrente"),
        "id": dados.get("id")
    }
    database.append(registro)
    return {"message": "Dados recebidos com sucesso", "registro": registro}

@app.get("/dados")
async def obter_dados():
    return {"message": "Endpoint para obter dados", "database": database}