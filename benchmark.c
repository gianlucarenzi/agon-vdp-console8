#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "mos_api.h"

// VDU commands
#define VDU_SET_MODE 22
#define VDU_GCOL 18
#define VDU_PLOT 25
#define VDU_CLS 12
#define VDU_CURSOR_OFF 23, 1, 0

// Video mode
#define VIDEO_MODE 131 // 320x240 64 colours

// Screen dimensions
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define NUM_COLORS 64

// Benchmark duration
#define BENCHMARK_DURATION 2

// --- VDU Helper Functions ---

// Set a random graphics color
void set_random_color() {
    mos_send_byte(VDU_GCOL);
    mos_send_byte(0); // Mode 0: Set foreground color
    mos_send_byte(rand() % NUM_COLORS);
}

// Move graphics cursor
void plot_move(int x, int y) {
    mos_send_byte(VDU_PLOT);
    mos_send_byte(4); // PLOT MOVE (absolute)
    mos_send_word(x);
    mos_send_word(y);
}

// Draw a line from last point to (x,y)
void plot_draw(int x, int y) {
    mos_send_byte(VDU_PLOT);
    mos_send_byte(5); // PLOT DRAW (absolute)
    mos_send_word(x);
    mos_send_word(y);
}

// Draw a line from (x1,y1) to (x2,y2)
void draw_line_primitive(int x1, int y1, int x2, int y2) {
    plot_move(x1, y1);
    plot_draw(x2, y2);
}

// Draw a filled rectangle between two corners
void draw_filled_rect_primitive(int x1, int y1, int x2, int y2) {
    plot_move(x1, y1);
    mos_send_byte(VDU_PLOT);
    mos_send_byte(0x60 | 5); // PLOT FILLED RECTANGLE (absolute)
    mos_send_word(x2);
    mos_send_word(y2);
}

// Draw a circle outline
void draw_empty_circle_primitive(int cx, int cy, int r) {
    plot_move(cx, cy);
    mos_send_byte(VDU_PLOT);
    mos_send_byte(0x90 | 5); // PLOT CIRCLE OUTLINE (absolute)
    mos_send_word(cx + r);
    mos_send_word(cy);
}

// Draw a filled circle
void draw_filled_circle_primitive(int cx, int cy, int r) {
    plot_move(cx, cy);
    mos_send_byte(VDU_PLOT);
    mos_send_byte(0x98 | 5); // PLOT FILLED CIRCLE (absolute)
    mos_send_word(cx + r);
    mos_send_word(cy);
}


// --- Benchmark Functions ---

void draw_lines() {
    set_random_color();
    draw_line_primitive(
        rand() % SCREEN_WIDTH, rand() % SCREEN_HEIGHT,
        rand() % SCREEN_WIDTH, rand() % SCREEN_HEIGHT
    );
}

void draw_rects_empty() {
    set_random_color();
    int x1 = rand() % SCREEN_WIDTH;
    int y1 = rand() % SCREEN_HEIGHT;
    int x2 = rand() % SCREEN_WIDTH;
    int y2 = rand() % SCREEN_HEIGHT;
    draw_line_primitive(x1, y1, x2, y1);
    draw_line_primitive(x2, y1, x2, y2);
    draw_line_primitive(x2, y2, x1, y2);
    draw_line_primitive(x1, y2, x1, y1);
}

void draw_rects_filled() {
    set_random_color();
    draw_filled_rect_primitive(
        rand() % SCREEN_WIDTH, rand() % SCREEN_HEIGHT,
        rand() % SCREEN_WIDTH, rand() % SCREEN_HEIGHT
    );
}

void draw_circles_empty() {
    set_random_color();
    draw_empty_circle_primitive(
        rand() % SCREEN_WIDTH, rand() % SCREEN_HEIGHT,
        rand() % (SCREEN_HEIGHT / 4)
    );
}

void draw_circles_filled() {
    set_random_color();
    draw_filled_circle_primitive(
        rand() % SCREEN_WIDTH, rand() % SCREEN_HEIGHT,
        rand() % (SCREEN_HEIGHT / 4)
    );
}

// --- Main Benchmark Runner ---

void run_benchmark(const char* name, void (*draw_function)()) {
    long count = 0;
    clock_t start_time, end_time;
    long primitives_per_second;

    printf("Running benchmark: %s...\r\n", name);

    mos_send_byte(VDU_CLS);

    start_time = clock();
    end_time = start_time + (BENCHMARK_DURATION * CLOCKS_PER_SEC);

    while (clock() < end_time) {
        draw_function();
        count++;
    }

    primitives_per_second = count / BENCHMARK_DURATION;
    printf("%s: %ld primitives/sec\r\n", name, primitives_per_second);
}

int main() {
    srand(time(NULL));

    // Set video mode
    mos_send_byte(VDU_SET_MODE);
    mos_send_byte(VIDEO_MODE);

    // Clear screen and turn cursor off
    mos_send_byte(VDU_CLS);
    printf("%c%c%c", VDU_CURSOR_OFF);

    // Run benchmarks
    run_benchmark("Lines", draw_lines);
    run_benchmark("Empty Rectangles", draw_rects_empty);
    run_benchmark("Filled Rectangles", draw_rects_filled);
    run_benchmark("Empty Circles", draw_circles_empty);
    run_benchmark("Filled Circles", draw_circles_filled);

    printf("\nBenchmark complete.\r\n");

    return 0;
}