#pragma once

/* Includes aqui */
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <driver/gpio.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Escrever defines e protótipos de funções de configuração do mcu */
#define LORA_M0             GPIO_NUM_18
#define LORA_M1             GPIO_NUM_5
#define LORA_AUX            GPIO_NUM_4
#define RX_GPS              GPIO_NUM_19
#define TX_GPS              GPIO_NUM_21
#define RX_LORA             GPIO_NUM_17
#define TX_LORA             GPIO_NUM_16
#define LED_ONBOARD         GPIO_NUM_2
#define LORA_BAUD           9600
#define GPS_BAUD            9600
#define DEBUG_BAUD          115200
#define LORA_CHANNEL        50

enum MessageType 
{
	TRIGGER_DROGUE = 0,
	TRIGGER_MAIN = 1,
	RESET_SYSTEM_TIMER = 2,
	RESET_SYSTEM = 3
};

extern HardwareSerial Serial_lora;
extern HardwareSerial Serial_gps;
extern AsyncWebServer server;
extern AsyncWebSocket ws;
extern const IPAddress apIP;
extern const IPAddress mask;
extern const char* ssid;
extern const char* password;

void gpio_init(void);
void uart_init(void);
void initLittleFS(void);
void wifi_init();
void initWebSocket(void);
void handleBinaryMessage(AsyncWebSocketClient *client, 
						 uint8_t *data, size_t len);
void handleTextMessage(AsyncWebSocketClient *client, 
						 uint8_t *data, size_t len);
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, 
			 AwsEventType type, void *arg, uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif