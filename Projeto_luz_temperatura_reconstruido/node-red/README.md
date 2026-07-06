# Fluxo Node-RED

O arquivo `flows.json` pode ser importado no Node-RED.

## Dependência adicional

O fluxo usa nós seriais. Instale:

```bash
cd ~/.node-red
npm install node-red-node-serialport
```

No Windows, a pasta do Node-RED normalmente fica no perfil do usuário.

## Configuração

Após importar o fluxo:

1. Abra o nó de configuração `Arduino UNO`.
2. Troque `COM6` pela porta real do seu Arduino.
3. Confirme `9600 baud`.
4. Abra o broker `Mosquitto local`.
5. Ajuste host e porta, se necessário.

Configuração padrão reconstruída:

```text
Serial: COM6
Baud: 9600
MQTT broker: 127.0.0.1
MQTT port: 1883
```

## Tópicos publicados pelo Node-RED

| Tópico | Conteúdo |
|---|---|
| `casa/ambiente/telemetria` | JSON completo |
| `casa/ambiente/temperatura` | Temperatura em °C |
| `casa/ambiente/umidade` | Umidade relativa |
| `casa/ambiente/luminosidade` | Leitura analógica 0..1023 |
| `casa/estado/ventilador` | `ON` ou `OFF` |
| `casa/estado/luz` | `ON` ou `OFF` |
| `casa/estado/movimento` | `1` ou `0` |
| `casa/sistema/eventos` | ACKs, erros e mensagens de status |

## Tópicos de configuração

| Tópico | Exemplo |
|---|---|
| `casa/config/temperatura_limite` | `22` |
| `casa/config/histerese` | `1` |
| `casa/config/luminosidade_limite` | `400` |
| `casa/config/auto_temperatura` | `1` |
| `casa/config/auto_iluminacao` | `1` |
| `casa/config/exigir_movimento` | `0` |

## Tópicos de comando manual

| Tópico | Exemplo |
|---|---|
| `casa/comando/ventilador` | `ON` |
| `casa/comando/luz` | `OFF` |
| `casa/comando/status` | qualquer payload |

Para controle manual do ventilador:

1. publique `0` em `casa/config/auto_temperatura`;
2. publique `ON` ou `OFF` em `casa/comando/ventilador`.

Para controle manual da luz:

1. publique `0` em `casa/config/auto_iluminacao`;
2. publique `ON` ou `OFF` em `casa/comando/luz`.
