#include <perifericos.h>
#include <mcu_config.h>
#include <esp_log.h>

uint8_t command = 0;
data_t_send global_data = {0};

shared_data_t shared_global_data = {0};

// Nosso novo JSON compartilhado. O tamanho deve ser suficiente para todos os dados (GPS + LoRa).
StaticJsonDocument<512> sharedJsonDoc; 

QueueHandle_t websocketSendQueue;
SemaphoreHandle_t dataMutex;

LoRa_E220 Transceiver(TX_LORA, RX_LORA, &Serial_lora, LORA_AUX, LORA_M0, LORA_M1, UART_BPS_RATE_9600, SERIAL_8N1);
TinyGPSPlus gps = TinyGPSPlus();

static void printParameters(struct Configuration configuration) {
	ESP_LOGI("", "HEAD -> Command:0x%02X | Register address:0x%02X | Lenght:0x%02X", configuration.COMMAND, configuration.STARTING_ADDRESS, configuration.LENGHT);
	ESP_LOGI("", "Address HIGH       : 0x%02X", configuration.ADDH);
	ESP_LOGI("", "Address LOW        : 0x%02X", configuration.ADDL);
	ESP_LOGI("", "Channel            : %d -> %s", configuration.CHAN, configuration.getChannelDescription().c_str());
	ESP_LOGI("", "UART ParityBit     : %s", configuration.SPED.getUARTParityDescription().c_str());
	ESP_LOGI("", "UART Baud          : %s", configuration.SPED.getUARTBaudRateDescription().c_str());
	ESP_LOGI("", "Air Data Rate      : %s", configuration.SPED.getAirDataRateDescription().c_str());
	ESP_LOGI("", "Packet lenght      : %s", configuration.OPTION.getSubPacketSetting().c_str());
	ESP_LOGI("", "Power              : %s", configuration.OPTION.getTransmissionPowerDescription().c_str());
	ESP_LOGI("", "RSSI Ambient Noise : %s", configuration.OPTION.getRSSIAmbientNoiseEnable().c_str());
	ESP_LOGI("", "WOR Period         : %s", configuration.TRANSMISSION_MODE.getWORPeriodByParamsDescription().c_str());
	ESP_LOGI("", "LBT                : %s", configuration.TRANSMISSION_MODE.getLBTEnableByteDescription().c_str());
	ESP_LOGI("", "RSSI               : %s", configuration.TRANSMISSION_MODE.getRSSIEnableByteDescription().c_str());
	ESP_LOGI("", "Transission Mode   : %s\n", configuration.TRANSMISSION_MODE.getFixedTransmissionDescription().c_str());
}

// void loraInit() {
//     ESP_LOGI("", "Inicializando LoRa..");
//     while (!Transceiver.begin()) {
//         ESP_LOGW("", "Falha na inicialização. Tentando novamente...");
//         vTaskDelay(pdMS_TO_TICKS(1000));
//     }
//     ESP_LOGI("", "LoRa iniciado. Iniciando leitura dos parâmetros..");

//     ResponseStructContainer c; 
// 	c = Transceiver.getConfiguration();
// 	Configuration configuration = *(Configuration*) c.data; /* conteiner para salvar os dados de configuração e parâmetros */
// 	ESP_LOGI("", "%s", c.status.getResponseDescription().c_str());
//     ESP_LOGI("", "Parâmetros lidos:");
// 	printParameters(configuration);

// 	c.close();

//     /* Novos valores dos parâmetros de configuração */
// 	configuration.ADDL = 0x20;
// 	configuration.ADDH = 0x0C;

// 	configuration.CHAN = 70;

// 	configuration.SPED.uartBaudRate = UART_BPS_9600;
// 	configuration.SPED.airDataRate = AIR_DATA_RATE_010_24;
// 	configuration.SPED.uartParity = MODE_00_8N1;

// 	configuration.OPTION.subPacketSetting = SPS_200_00;
// 	configuration.OPTION.RSSIAmbientNoise = RSSI_AMBIENT_NOISE_DISABLED;
// 	configuration.OPTION.transmissionPower = POWER_30;

// 	configuration.TRANSMISSION_MODE.enableRSSI = RSSI_ENABLED;
// 	configuration.TRANSMISSION_MODE.fixedTransmission = FT_TRANSPARENT_TRANSMISSION;
// 	configuration.TRANSMISSION_MODE.enableLBT = LBT_DISABLED;
// 	configuration.TRANSMISSION_MODE.WORPeriod = WOR_2000_011;

// 	/* Envia e salva as novas configurações */
// 	ResponseStatus rs = Transceiver.setConfiguration(configuration, WRITE_CFG_PWR_DWN_SAVE);
// 	ESP_LOGI("", "%s", rs.getResponseDescription().c_str());
//     ESP_LOGI("", "Parâmetros atuais:");
// 	printParameters(configuration);

//     ESP_LOGI("", "Configuração LoRa completada.");
// }
void loraInit() {
    lora_sintonizar(70);
}

ResponseStatus loraSetConfig(uint8_t canal) {
    ESP_LOGI("", "Inicializando LoRa..");
    while (!Transceiver.begin()) {
        ESP_LOGW("", "Falha na inicialização. Tentando novamente...");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    ESP_LOGI("", "LoRa iniciado. Iniciando leitura dos parâmetros..");


    gpio_set_direction(LORA_M0, GPIO_MODE_OUTPUT);
	gpio_set_level(LORA_M0, 1);
	gpio_set_direction(LORA_M1, GPIO_MODE_OUTPUT);
	gpio_set_level(LORA_M1, 1);

    ResponseStructContainer c; 
	c = Transceiver.getConfiguration();
	Configuration configuration = *(Configuration*) c.data; /* conteiner para salvar os dados de configuração e parâmetros */
	ESP_LOGI("", "%s", c.status.getResponseDescription().c_str());
    ESP_LOGI("", "Parâmetros lidos:");
	printParameters(configuration);

	c.close();

    /* Novos valores dos parâmetros de configuração */
	configuration.ADDL = 0x20;
	configuration.ADDH = 0x0C;

	configuration.CHAN = canal;

	configuration.SPED.uartBaudRate = UART_BPS_9600;
	configuration.SPED.airDataRate = AIR_DATA_RATE_010_24;
	configuration.SPED.uartParity = MODE_00_8N1;

	configuration.OPTION.subPacketSetting = SPS_200_00;
	configuration.OPTION.RSSIAmbientNoise = RSSI_AMBIENT_NOISE_DISABLED;
	configuration.OPTION.transmissionPower = POWER_30;

	configuration.TRANSMISSION_MODE.enableRSSI = RSSI_ENABLED;
	configuration.TRANSMISSION_MODE.fixedTransmission = FT_TRANSPARENT_TRANSMISSION;
	configuration.TRANSMISSION_MODE.enableLBT = LBT_DISABLED;
	configuration.TRANSMISSION_MODE.WORPeriod = WOR_2000_011;

	/* Envia e salva as novas configurações */
	ResponseStatus rs = Transceiver.setConfiguration(configuration, WRITE_CFG_PWR_DWN_SAVE);
	ESP_LOGI("", "%s", rs.getResponseDescription().c_str());
    ESP_LOGI("", "Parâmetros atuais:");
	printParameters(configuration);

    ESP_LOGI("", "Configuração LoRa completada.");

    gpio_set_direction(LORA_M0, GPIO_MODE_OUTPUT);
	gpio_set_level(LORA_M0, 0);
	gpio_set_direction(LORA_M1, GPIO_MODE_OUTPUT);
	gpio_set_level(LORA_M1, 0);

    return rs;
}

void loraSend(uint8_t command) {
    uint8_t _command = command;
    ResponseStatus rs = Transceiver.sendMessage(&_command, sizeof(_command));
    ESP_LOGI("", "Envio de dados: %s", rs.getResponseDescription().c_str());
}

void lora_sintonizar(uint8_t canal) {
    JsonDocument jsonToSend;
    jsonToSend["canal"] = canal;

    if (canal > 80 || canal < 0) {
        ESP_LOGW("", "Canal inválido. Deve ser entre 0 e 80.");
        jsonToSend["resultado_sintonia"] = false;
    }

    //E220_SUCCESS
    if (loraSetConfig(canal).code != E220_SUCCESS) {
        jsonToSend["resultado_sintonia"] = false;
        ESP_LOGE("", "Falha ao sintonizar canal %d.", canal);
    }    
    ESP_LOGI("", "Sintonizou ao canal %d com SUCESSO!.", canal);
    jsonToSend["resultado_sintonia"] = true;
    char outputBuffer[128];
    serializeJson(jsonToSend, outputBuffer);
    ws.textAll(outputBuffer);
}

void getGpsData(gps_data_t *data, JsonDocument *gpsJsonDoc) {

    double avionic_latitude = 0.0;
    double avionic_longitude = 0.0;

    while (Serial_gps.available() > 0) 
    {
        char c = Serial_gps.read();
        if (gps.encode(c)) 
        {
            
            if (gps.location.isValid() && gps.location.isUpdated()) 
            {
                data->latitude = gps.location.lat();
                data->longitude = gps.location.lng();
                (*gpsJsonDoc)["latitude"] = gps.location.lat();
                (*gpsJsonDoc)["longitude"] = gps.location.lng();
            }
        
            if (gps.altitude.isValid() && gps.altitude.isUpdated()){
                data->altitude = (float)gps.altitude.meters();
                (*gpsJsonDoc)["altitude"] = (float)gps.altitude.meters();
            }

            if (gps.hdop.isValid() && gps.hdop.isUpdated()){
                data->precision = (float)gps.hdop.hdop();
                (*gpsJsonDoc)["precision"] = (float)gps.hdop.hdop();
            }


            if (gps.satellites.isValid() && gps.satellites.isUpdated()){
                data->satellites = (uint8_t)gps.satellites.value();
                (*gpsJsonDoc)["satellites"] = (uint8_t)gps.satellites.value();
            }
        }
    }
}

void gpsTask(void *pvParameters) {
    gps_data_t gps_data;
    JsonDocument json_gps_data;

    while(1) {
        getGpsData(&gps_data, &json_gps_data);

        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(10)) == pdTRUE) 
        {
            sharedJsonDoc["distance"] = (float) gps_data.distance;
	        sharedJsonDoc["course"] = (float) gps_data.course;
	        sharedJsonDoc["latitude"] = (float) gps_data.latitude;
	        sharedJsonDoc["longitude"] = (float) gps_data.longitude;
	        sharedJsonDoc["altitude"] = (float) gps_data.altitude;
	        sharedJsonDoc["precision"] = (float) gps_data.precision;
            sharedJsonDoc["satellites"] = (uint8_t) gps_data.satellites;
            xSemaphoreGive(dataMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void loraReceiveTask(void *pvParameters) {
    StaticJsonDocument<512> validatedDoc; // O JSON após validação do nosso sistema

    g_lora_data data_lora;

    while(1) {
        // Preenche a struct local com a struct recebida pelo LoRa
        if(Transceiver.available() > 0) {
            ResponseContainer rc = Transceiver.receiveMessageRSSI();

            ESP_LOGI("", "Dados recebidos do LoRa.");

            if (rc.status.code != 1) {
                ESP_LOGE("", "Status do recebimento: %s", rc.status.getResponseDescription().c_str());
                vTaskDelay(pdMS_TO_TICKS(100));
                continue;
            } else {
                ESP_LOGI("", "Conteúdo: %s", rc.data.c_str());
                // 1. Preparar os documentos JSON
                StaticJsonDocument<256> loraDoc;   // Documento temporário para validar o JSON do LoRa
                const char* jsonStringFromLora = rc.data.c_str();

                // 2. Tentar decodificar o JSON recebido
                DeserializationError error = deserializeJson(loraDoc, jsonStringFromLora);

                if (error) {
                    // CASO 1: JSON INVÁLIDO RECEBIDO
                    ESP_LOGW("", "JSON do LoRa recebido é INVÁLIDO: %s", error.c_str());
                    ESP_LOGW("", "Payload bruto: %s", jsonStringFromLora);

                    validatedDoc["valido"] = 0;
                    validatedDoc["equipe"] = loraDoc["equipe"] | 0;
                    // Guarda a string corrompida original para fins de depuração
                    validatedDoc["payload"] = jsonStringFromLora;
                } else {
                    // CASO 2: JSON VÁLIDO RECEBIDO
                    ESP_LOGI("", "JSON do LoRa recebido é VÁLIDO.");

                    validatedDoc["valido"] = 1;
                    // Extrai a equipe do JSON recebido. Se não existir, usa 0 como padrão.
                    validatedDoc["equipe"] = loraDoc["equipe"] | 0; 
                    // Copia o objeto "payload" aninhado do JSON recebido
                    validatedDoc["payload"] = loraDoc["payload"];
                }
            }
        }
        else {
            ESP_LOGW("", "Dados do lora não disponíveis.");
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(10)) == pdTRUE) 
        {
            sharedJsonDoc.set(validatedDoc.as<JsonObject>());
            xSemaphoreGive(dataMutex); /* Libera o mutex para outras tasks */
        }

        // Envia a cópia para a fila
        xQueueSend(websocketSendQueue, &sharedJsonDoc, 0);
        vTaskDelay(pdMS_TO_TICKS(100)); /* 100 ms delay */
    }
}

void wsSendTask(void *pvParameters) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(200);
    JsonDocument jsonToSend;

    while (1)
    {
        /* Lê primeiro dado da fila (cópia da estrutura global) */
        if(xQueueReceive(websocketSendQueue, &jsonToSend, pdMS_TO_TICKS(100)) == pdPASS) {
            char outputBuffer[512];
            serializeJson(sharedJsonDoc, outputBuffer);
            ws.textAll(outputBuffer);
            ESP_LOGI("", "JSON Completo Enviado: %s", outputBuffer);
        }
        // Aguarda até completar 200 ms desde a última chamada
        vTaskDelayUntil(&lastWakeTime, period);
    }
}