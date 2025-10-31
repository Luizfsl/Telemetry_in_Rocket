#pragma once

/* Includes aqui */
#include <Arduino.h>
#include <ArduinoJson.h>
#include <mcu_config.h>
#include <TinyGPS++.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

/* 
	Para o módulo E220 900T30D, precisa incluir os defines abaixo no header statesNaming.h, ou antes do include da biblioteca: 
	#define FREQUENCY_900
	#define E220_30
*/

#define FREQUENCY_900
#define E220_30

#include <EByte_LoRa_E220_library.h>

#ifdef __cplusplus
extern "C" {
#endif

struct flag_t 
{
	uint8_t mpu : 1;
	uint8_t mpl : 1;
	uint8_t sd  : 1;
	uint8_t gps : 1;
	uint8_t main_par_deploy : 1;
	uint8_t drogue_par_deploy : 1;
	uint8_t is_ascending : 1;
	uint8_t low_battery : 1;
};

struct data_t 
{
	// MPU9250
	float ax;
	float ay;
	float az;
	float roll;
	float pitch;
	float yaw;
	// MPL3115A2
	float altitude;
	float pressure;
	float temperature;
	float apogee;
	// GPS
	double gps_lat;
	double gps_lon;
	float gps_speed;
	float gps_altitude;
	float gps_precision;
	uint8_t gps_sat;
	// SYSTEM
	float vBattery;
	uint8_t sys_hour;
	uint8_t sys_min;
	uint8_t sys_sec;
	uint16_t sys_ms;
	uint16_t delta_t;
	flag_t flag;
};

#define MAX_LORA_PAYLOAD_SIZE 150

struct g_lora_data {
	uint8_t buffer[MAX_LORA_PAYLOAD_SIZE];
	size_t length;
};

// Desativa alinhamento de bits na estrutura pelo compilador, evitando assim
// que bytes desnecessários sejam incluídos na estrutura apenas para alinhamento
struct __attribute__((packed)) data_t_send
{
	float ax;
	float ay;
	float az;
	float roll;
	float pitch;
	float yaw;
	float altitude;
	float pressure;
	float temperature;
	float apogee;
	float gps_lat;
	float gps_lon;
	float gps_speed;
	float gps_altitude;
	float gps_precision;
	float base_distance;
	float base_course;
	float base_gps_lat;
	float base_gps_lon;
	float base_gps_altitude;
	float base_gps_precision;
	float velocity_z;
	float velocity_y;
	float velocity_x;
	float vBattery;
	uint8_t gps_sat;
	uint8_t base_gps_sat;
	uint8_t sys_hour;
	uint8_t sys_min;
	uint8_t sys_sec;
	uint8_t status_mpu;
	uint8_t status_mpl;
	uint8_t status_gps;
	uint8_t status_sd;
	uint8_t main_deploy;
	uint8_t drogue_deploy;
	uint8_t ascending;
	uint8_t low_battery;
};

struct gps_data_t {
	double latitude;
    double longitude;
    double altitude;
    float precision;
    uint8_t satellites;
    double distance;
	double course;
};

struct shared_data_t {

    struct {
        uint8_t buffer[MAX_LORA_PAYLOAD_SIZE];
        size_t length;
    } lora;

    struct {
        double latitude;
        double longitude;
        double altitude;
        float precision;
        uint8_t satellites;
        double distance;
        double course;
    } gps;
};

/* Variáveis globais */

extern LoRa_E220 Transceiver;
extern TinyGPSPlus gps;
extern float velx;
extern float vely;
extern float velz;
extern float prevAccX;
extern float prevAccY;
extern float prevAccZ;
extern double av_gps_lat;
extern double av_gps_lon;
extern uint8_t command;
extern data_t_send global_data;
extern QueueHandle_t websocketSendQueue;
extern SemaphoreHandle_t dataMutex;

void loraInit();
void lora_sintonizar(uint8_t canal);
void loraSend(uint8_t command);
void getGpsData(gps_data_t *data, JsonDocument *gpsJsonDoc);
void gpsTask(void *pvParameters);
void loraReceiveTask(void *pvParameters);
void wsSendTask(void *pvParameters);

#ifdef __cplusplus
}
#endif