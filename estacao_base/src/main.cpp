#include <Arduino.h>
#include <mcu_config.h>
#include <perifericos.h>
#include <esp_log.h>

void setup() 
{
	esp_log_level_set("*", ESP_LOG_DEBUG);
	gpio_init();
	uart_init();
	initLittleFS();
	wifi_init();
	initWebSocket();
	loraInit();

	websocketSendQueue = xQueueCreate(20, sizeof(data_t_send));
  	if (websocketSendQueue == NULL) {
    	ESP_LOGE("", "Erro ao criar fila do websocket");
	}
	else {
		ESP_LOGI("", "Fila websocket criada.");
	}
	
	dataMutex = xSemaphoreCreateMutex();
  	if (dataMutex == NULL) {
    	ESP_LOGE("", "Erro ao criar mutex");
	}
	else {
		ESP_LOGI("", "Mutex criado.");
	}

	xTaskCreatePinnedToCore(gpsTask, "TaskGPS", 4000, NULL, 2, NULL, 1);
	xTaskCreatePinnedToCore(loraReceiveTask, "TaskLORA", 4000, NULL, 2, NULL, 1);
	xTaskCreatePinnedToCore(wsSendTask, "TaskWebSocket", 4000, NULL, 2, NULL, 0);
}

void loop() 
{
	ws.cleanupClients();
	vTaskDelay(pdMS_TO_TICKS(1000));
}
