/* mario_vdp_demo.c
 * Demo che invia a VDP una sequenza di sprite per mostrare Mario:
 * - cammina a sinistra, fermo (2s), cammina a destra, fermo (2s), ripeti
 * - ogni tanto esegue un salto con parabola
 *
 * Si basa sulle funzioni vdp_* presenti in vdp_benchmark.c per invio comandi.
 * Questo file integra una semplice macchina a stati che sceglie i frame corretti
 * sulla base della descrizione fornita:
 *  Row 0 = left, Row 1 = right, frames_per_row = N (calcolato dal tilesheet)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>
#include "mario_sprite.h"

#define FRAMES_PER_ROW 10 /* adattare se necessario */
#define SPRITE_ID 1

typedef enum { DIR_LEFT=0, DIR_RIGHT=1 } Direction;

typedef enum { S_WALK, S_IDLE_PAUSE, S_WALK_BACK, S_IDLE_PAUSE2 } DemoState;

static const uint8_t walk_cycle[] = {1,2,3,4,5,4,3,2,1,2,3,4};
static const size_t walk_len = sizeof(walk_cycle)/sizeof(walk_cycle[0]);

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    // Nota: this demo assumes serial port already opened/configured in vdp_benchmark
    // For simplicity, it will just print the sequence of sprite indices to stdout

    srand(time(NULL));
    Direction dir = DIR_LEFT;
    DemoState state = S_WALK;
    size_t walk_idx = 0;
    size_t idle_ticks = 0;
    size_t frame_delay_ms = 120; // ms per frame
    size_t idle_pause_ticks = (2000 / frame_delay_ms);
    size_t ticks = 0;

    while (1) {
        if (state == S_WALK) {
            uint8_t col = walk_cycle[walk_idx % walk_len];
            uint16_t idx = dir * FRAMES_PER_ROW + col;
            printf("SPRITE IDX: %u (dir %d)\n", idx, dir);
            walk_idx++;
            ticks++;
            if (ticks > 40) { // dopo ~40 frame cambia stato
                state = S_IDLE_PAUSE;
                idle_ticks = 0;
            }
        } else if (state == S_IDLE_PAUSE) {
            printf("SPRITE IDX: %u (idle)\n", (uint16_t)(dir*FRAMES_PER_ROW + 0));
            idle_ticks++;
            if (idle_ticks >= idle_pause_ticks) {
                // inverti direzione e vai a camminare
                dir = (dir == DIR_LEFT) ? DIR_RIGHT : DIR_LEFT;
                state = S_WALK_BACK;
                ticks = 0;
                walk_idx = 0;
            }
        } else if (state == S_WALK_BACK) {
            uint8_t col = walk_cycle[walk_idx % walk_len];
            uint16_t idx = dir * FRAMES_PER_ROW + col;
            printf("SPRITE IDX: %u (dir %d)\n", idx, dir);
            walk_idx++;
            ticks++;
            if (ticks > 40) {
                state = S_IDLE_PAUSE2;
                idle_ticks = 0;
            }
        } else if (state == S_IDLE_PAUSE2) {
            printf("SPRITE IDX: %u (idle)\n", (uint16_t)(dir*FRAMES_PER_ROW + 0));
            idle_ticks++;
            if (idle_ticks >= idle_pause_ticks) {
                state = S_WALK;
                ticks = 0;
                walk_idx = 0;
            }
        }

        // occasional jump
        if ((rand() % 200) == 0) {
            printf("SPRITE IDX: %u (jump)\n", (uint16_t)(dir*FRAMES_PER_ROW + 9));
            // simple parabola simulation (just prints positions)
            for (int j=0;j<10;j++) printf("JUMP Y: %d\n", (int)(-5*j*j + 40*j));
        }

        usleep(frame_delay_ms * 1000);
    }

    return 0;
}
