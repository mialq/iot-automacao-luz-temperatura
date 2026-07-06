# Notas da reconstrução

Esta implementação foi reconstruída a partir do material acadêmico disponível no repositório.

## O que foi possível identificar no artigo

O PDF preserva evidências de:

- Arduino UNO;
- sensor DHT11;
- DHT11 conectado ao pino digital 2;
- comunicação serial configurada em 9600 baud;
- sensor de luminosidade;
- sensor de vibração/movimento;
- dois relés para luz e ventilador;
- Node-RED;
- broker MQTT Mosquitto;
- aplicativo MQTT Dash;
- fluxo Node-RED com nós denominados `Luz`, `Temperatura`, `RELE LUZ` e `RELE VENTILADOR`;
- leitura serial identificada como `COM6` em uma captura;
- ajuste remoto do limite de temperatura;
- tentativa de condicionar a iluminação à luminosidade e ao movimento.

## O que não foi recuperado

Não estavam disponíveis:

- o sketch Arduino original completo em arquivo `.ino`;
- o export JSON original do Node-RED;
- o mapeamento completo e inequívoco de todos os pinos;
- as configurações exatas do broker MQTT;
- as credenciais/configurações do aplicativo móvel;
- os valores finais de todos os limiares.

Por isso, os arquivos desta pasta são uma **reconstrução funcional**, e não uma cópia dos originais perdidos.

## Decisão técnica desta versão

O artigo menciona StandardFirmata e também apresenta um sketch próprio para DHT11. Em um único Arduino UNO, usar simultaneamente o StandardFirmata padrão e um sketch personalizado não é uma arquitetura simples ou direta.

Para evitar essa ambiguidade, esta reconstrução usa:

```text
Arduino UNO
   ↕ Serial USB (JSON e comandos)
Node-RED
   ↕ MQTT
Mosquitto
   ↕
Smartphone / cliente MQTT
```

Assim:

- o Arduino cuida da aquisição dos sensores e acionamento dos relés;
- o Node-RED faz a ponte entre Serial e MQTT;
- o smartphone publica configurações e comandos via MQTT;
- a telemetria é publicada pelo Node-RED em tópicos MQTT.

## Melhorias adicionadas

Além de reconstruir a ideia original, foram adicionadas:

- histerese de temperatura;
- validação de comandos;
- telemetria em JSON;
- modo automático/manual;
- sensor de movimento opcional;
- tópicos MQTT organizados;
- mensagens de confirmação (`ack`);
- separação clara entre hardware e integração.
