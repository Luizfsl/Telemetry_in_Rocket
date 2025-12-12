#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"
#include "dev/leds.h"
#include <stdio.h>
#include <string.h>
#include "sys/clock.h"

/* Intervalo de envio (2 segundos) */
#define SEND_INTERVAL (CLOCK_SECOND * 1)
#define MAX_PAYLOAD_LEN 110 

/* Estrutura de Dados (Sensores + Tempo) */
typedef struct {
    // --- Tempo ---
    uint8_t sys_hour;
    uint8_t sys_min;
    uint8_t sys_sec;
    uint16_t sys_ms;
    uint16_t delta_t; // Diferença de tempo entre pacotes

    // --- Sensores Físicos ---
    int16_t ax, ay, az;       // Acelerômetro
    int16_t roll, pitch, yaw; // Giroscópio
    int16_t temp;             // Temperatura
    uint32_t press;           // Pressão
    int16_t alt;              // Altitude
    int16_t seq;              // Sequência
} RocketData;

PROCESS(rocket_sender_process, "Rocket Sender Time+Sensors");
AUTOSTART_PROCESSES(&rocket_sender_process);

static struct broadcast_conn bc;

static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from) {}
static const struct broadcast_callbacks broadcast_call = { broadcast_recv };

/* Função que simula os dados */
void simular_dados(RocketData *data, int seq_atual) {
    static clock_time_t last_tick = 0;
    unsigned long total_sec = clock_seconds();
    clock_time_t now = clock_time();

    data->sys_hour = (total_sec / 3600) % 24;
    data->sys_min  = (total_sec / 60) % 60;
    data->sys_sec  = total_sec % 60;
    data->sys_ms   = (unsigned long)(now % CLOCK_SECOND) * 1000 / CLOCK_SECOND;

    if(last_tick == 0) data->delta_t = 0;
    else data->delta_t = (unsigned long)(now - last_tick) * 1000 / CLOCK_SECOND;

    last_tick = now;

    data->ax = 10;
    data->ay = 20;
    data->az = 980;     // gravidade normal

    data->roll  = 0;
    data->pitch = 0;
    data->yaw   = 0;

    data->temp  = 25;        // temperatura fixa
    data->press = 101325;    // pressão fixa
    data->alt   = 100;       // altitude fixa (exemplo)


    data->seq = seq_atual;
}

PROCESS_THREAD(rocket_sender_process, ev, data)
{
    static struct etimer periodic_timer;
    static int seq_count = 0;
    static RocketData sensors;

    PROCESS_EXITHANDLER(broadcast_close(&bc);)
    PROCESS_BEGIN();

    printf("[Rocket] Iniciando com TEMPO e SENSORES...\n");

    broadcast_open(&bc, 129, &broadcast_call);
    etimer_set(&periodic_timer, SEND_INTERVAL);

    while(1) {
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
        seq_count++;
        sensors.seq = seq_count;

        simular_dados(&sensors, seq_count);

        // Buffer para o JSON
        char json[MAX_PAYLOAD_LEN];
        
        /* FORMATO JSON COMPACTO:
         * "t": [h,m,s,ms,dt] -> Tempo
         * "a": [x,y,z]       -> Aceleração
         * "g": [r,p,y]       -> Giroscópio
         * "e": [alt,temp]    -> Ambiente
         */
        snprintf(json, sizeof(json), 
                 "{\"s\":%d,\"t\":[%d,%d,%d,%d,%d],\"a\":[%d,%d,%d],\"g\":[%d,%d,%d],\"e\":[%d,%d]}",
                 sensors.seq,
                 sensors.sys_hour, sensors.sys_min, sensors.sys_sec, sensors.sys_ms, sensors.delta_t,
                 sensors.ax, sensors.ay, sensors.az,
                 sensors.roll, sensors.pitch, sensors.yaw,
                 sensors.alt, sensors.temp
                 );

        packetbuf_clear();
        packetbuf_copyfrom(json, strlen(json));
        broadcast_send(&bc);

        printf("%s\n", json);

        leds_toggle(LEDS_GREEN);
        etimer_reset(&periodic_timer);
    }

    PROCESS_END();
}
