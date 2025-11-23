/*
 * rocket_sender.c
 * Emulação de um nó TX tipo LoRa (taxa baixa + payload pequeno)
 */

#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"
#include "dev/leds.h"
#include <stdio.h>
#include <string.h>

#define SEND_INTERVAL (CLOCK_SECOND * 5)  // simulando baixa taxa (LoRa-like)

PROCESS(rocket_sender_process, "Rocket Sender Process");
AUTOSTART_PROCESSES(&rocket_sender_process);

static struct broadcast_conn bc;

/* Callback obrigatório para broadcast, mas o TX não precisa tratar nada */
static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
  // Não fazemos nada no sender ao receber pacotes (LoRa é tipicamente uplink unidirecional)
}

static const struct broadcast_callbacks broadcast_call = { broadcast_recv };

PROCESS_THREAD(rocket_sender_process, ev, data)
{
  static struct etimer periodic_timer;
  static int seq = 0; // contador de quadros enviados

  PROCESS_EXITHANDLER(broadcast_close(&bc);)

  PROCESS_BEGIN();

  printf("[Rocket] Iniciando nó foguete (emulando TX LoRa-like)...\n");

  /* Abre o canal 129 (qualquer número pode ser usado) */
  broadcast_open(&bc, 129, &broadcast_call);

  /* Configura o primeiro timer */
  etimer_set(&periodic_timer, SEND_INTERVAL);

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

    seq++;

    /* Monta payload pequeno (típico de LoRa) */
    char msg[50];
    snprintf(msg, sizeof(msg),
             "LORA_FRAME seq=%d alt=%d vel=%d",
             seq,
             1000 + (random_rand() % 50),   // altitude simulada
             50 + (random_rand() % 5));     // velocidade simulada

    packetbuf_clear();
    packetbuf_copyfrom(msg, strlen(msg));
    broadcast_send(&bc);

    printf("[Rocket] Enviado: \"%s\"\n", msg);

    leds_toggle(LEDS_GREEN);

    etimer_reset(&periodic_timer);
  }

  PROCESS_END();
}

