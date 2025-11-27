/*
 * base_station.c
 * Emulação de uma estação base LoRa-like (apenas RX)
 * Compatível com Cooja + MRM.
 */

#include "contiki.h"
#include "net/rime/rime.h"
#include "dev/leds.h"
#include <stdio.h>

PROCESS(base_station_process, "Base Station Process");
AUTOSTART_PROCESSES(&base_station_process);

static struct broadcast_conn bc;

/* Callback chamado sempre que um quadro chega */
static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
  char *msg = (char *)packetbuf_dataptr();

  printf("[Base] Recebido de %d.%d -> \"%s\" (tamanho=%d bytes)\n",
         from->u8[0], from->u8[1],
         msg,
         packetbuf_datalen());

  leds_toggle(LEDS_RED);
}

static const struct broadcast_callbacks broadcast_call = { broadcast_recv };

PROCESS_THREAD(base_station_process, ev, data)
{
  PROCESS_EXITHANDLER(broadcast_close(&bc);)

  PROCESS_BEGIN();

  printf("[Base] Estação base iniciada (RX LoRa-like)...\n");

  broadcast_open(&bc, 129, &broadcast_call);

  /* Fica apenas esperando quadros */
  while(1) {
    PROCESS_WAIT_EVENT();
  }

  PROCESS_END();
}

