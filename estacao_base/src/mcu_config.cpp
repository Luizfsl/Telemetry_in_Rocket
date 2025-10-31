#include <mcu_config.h>
#include <perifericos.h>
#include <esp_log.h>

HardwareSerial Serial_lora(2);
HardwareSerial Serial_gps(1);
AsyncWebServer server(80);

AsyncWebSocket ws("/ws");
const IPAddress apIP = IPAddress(192, 168, 4, 1);
const IPAddress mask = IPAddress(255, 255, 255 ,0);
const char* ssid = "BASE_ASABRANCA";
const char* password = "12345678#!";

void gpio_init(void) {
    gpio_set_direction(LED_ONBOARD, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_ONBOARD, 1);

	gpio_set_direction(LORA_M0, GPIO_MODE_OUTPUT);
	gpio_set_level(LORA_M0, 0);
	gpio_set_direction(LORA_M1, GPIO_MODE_OUTPUT);
	gpio_set_level(LORA_M1, 0);
}

void uart_init(void) {
    Serial.begin(DEBUG_BAUD);
	Serial.setDebugOutput(true);
    ESP_LOGI("", "UART debug inicializada.");
    Serial_gps.begin(GPS_BAUD, SERIAL_8N1, TX_GPS, RX_GPS);
    ESP_LOGI("", "UART gps inicializada.");
    Serial_lora.begin(LORA_BAUD, SERIAL_8N1, TX_LORA, RX_LORA);
    ESP_LOGI("", "UART lora inicializada");
}

void wifi_init() {
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    WiFi.mode(WIFI_AP);
	WiFi.softAPConfig(apIP, apIP, mask);

	while (!WiFi.softAP(ssid, password)) 
	{
    	vTaskDelay(pdMS_TO_TICKS(1000));
  	}
	ESP_LOGI("", "Wifi inicializado");
}

void initWebSocket(void) {
    ws.onEvent(onEvent);
	server.addHandler(&ws);
	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
	{
		request->send(LittleFS, "/index.html", "text/html", false);
	});
	server.serveStatic("/", LittleFS, "/");
	server.begin();
	ESP_LOGI("", "WebSocket iniciado.");
}

void initLittleFS(void) {
    if(!LittleFS.begin(false)) {
		ESP_LOGE("", "Falha ao inicializar littleFS.");
		for (int i = 1; i < 6; i++) {
			ESP_LOGW("", "#%d tentativa de inicialização..", i);
			if(LittleFS.begin(false)) {
				ESP_LOGI("", "LittleFS inicializado.");
				return;
			}
			vTaskDelay(pdMS_TO_TICKS(1000));
		}
	}
    ESP_LOGI("", "LittleFS inicializado.");   
}

// Para dados recebidos em formato JSON (texto), normalmente se usa outro método.
// Exemplo típico para tratar mensagens de texto (JSON) via WebSocket:
void handleTextMessage(AsyncWebSocketClient *client, uint8_t *data, size_t len) {
	String jsonStr = String((char*)data).substring(0, len);

	StaticJsonDocument<256> doc;
	DeserializationError error = deserializeJson(doc, jsonStr);
	if (!error) {
		// Exemplo: acessar campo "acao"
		if (doc.containsKey("acao")) {
			String acao = doc["acao"];
			
			if (acao && acao == "sintonizar") {
				int canal = doc["canal"];
				ESP_LOGI(""," Sintonizando no canal %d...", canal);
				lora_sintonizar(canal);
			}
		}
	}
}

void handleBinaryMessage(AsyncWebSocketClient *client, uint8_t *data, size_t len) {
	
	uint8_t messageType = data[0];
	
	if ((messageType == TRIGGER_DROGUE) && (len >= 1)) 
	{
		loraSend((uint8_t)1);
	}
	else if ((messageType == TRIGGER_MAIN) && (len >= 1))
	{
		loraSend((uint8_t)2);
	}
	else if ((messageType == RESET_SYSTEM_TIMER) && (len >= 1))
	{
		loraSend((uint8_t)3);
	}
	else if ((messageType == RESET_SYSTEM) && (len >= 1))
	{
		loraSend((uint8_t)4);
	}
	return;
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, 
		     AwsEventType type, void *arg, uint8_t *data, size_t len) 
{
	switch (type) 
	{
	case WS_EVT_CONNECT:
		{
			ESP_LOGI("", "Cliente websocket conectado.");
			break;
		}
	case WS_EVT_DISCONNECT:
		{
			ESP_LOGI("", "Cliente websocket desconectado.");
			break;
		}
	case WS_EVT_PONG:
	case WS_EVT_ERROR:
		break;
	case WS_EVT_DATA:
		{
			AwsFrameInfo *info = (AwsFrameInfo*)arg;
			if (info->opcode == WS_BINARY) 
			{
				handleBinaryMessage(client, data, len);
			}
			else if (info->opcode == WS_TEXT)
			{
				handleTextMessage(client, data, len);
			}
			break;
		}
	}
}

