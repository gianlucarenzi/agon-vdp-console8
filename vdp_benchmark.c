#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/time.h>
#include "mario_sprite.h"

// --- VDP Constants ---
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 1024
#define NUM_OPERATIONS 500

// --- VDP Commands ---
#define VDU_CLEAR_SCREEN 12
#define VDU_GCOL 18
#define VDU_PLOT 25
#define VDU_MODE 22
#define VDU_CURSOR_OFF 23, 0, 1, 0
#define VDU_DEFINE_BITMAP 23, 27
#define VDU_DRAW_BITMAP 23, 28
#define VDU_ASSIGN_SPRITE 23, 0, 83
#define VDU_ACTIVATE_SPRITE 23, 0, 84
#define VDU_MOVE_SPRITE_TO 23, 0, 85


// --- Serial Port Handling ---
int serial_port;

// Function to send a sequence of bytes to the VDP
void vdp_send(const unsigned char* data, size_t len) {
    if (write(serial_port, data, len) != len) {
        perror("Failed to write to serial port");
    }
    tcdrain(serial_port); // Wait for all data to be written
}

// Function to send a single byte command
void vdp_putc(unsigned char c) {
    vdp_send(&c, 1);
}

// Function to send a 16-bit word (little-endian)
void vdp_putw(int w) {
    unsigned char data[2];
    data[0] = w & 0xFF;
    data[1] = (w >> 8) & 0xFF;
    vdp_send(data, 2);
}

// --- Benchmark Helpers ---
long long get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000 + (long long)tv.tv_usec / 1000;
}

void print_results(const char* test_name, long long start_time, long long end_time, int num_ops) {
    long long duration = end_time - start_time;
    if (duration == 0) duration = 1; // Avoid division by zero
    double ops_per_sec = (double)num_ops / (duration / 1000.0);
    printf("[%s]\n", test_name);
    printf("  - Total time: %lld ms for %d operations\n", duration, num_ops);
    printf("  - Average time: %.4f ms/op\n", (double)duration / num_ops);
    printf("  - Throughput: %.2f ops/sec\n\n", ops_per_sec);
    sleep(2); // Pause between tests
}

// --- VDP Command Wrappers ---
void vdp_clear_screen() {
    vdp_putc(VDU_CLEAR_SCREEN);
}

void vdp_set_graphics_color(int color) {
    vdp_putc(VDU_GCOL);
    vdp_putc(0); // Mode 0: Set foreground color
    vdp_putc(color);
}

void vdp_plot(int x, int y) {
    vdp_putc(VDU_PLOT);
    vdp_putc(5); // Plot point
    vdp_putw(x);
    vdp_putw(y);
}

void vdp_line(int x1, int y1, int x2, int y2) {
    vdp_putc(VDU_PLOT);
    vdp_putc(97); // Draw line (using a common VDU extension)
    vdp_putw(x1); vdp_putw(y1);
    vdp_putw(x2); vdp_putw(y2);
}

void vdp_filled_rectangle(int x1, int y1, int x2, int y2) {
    vdp_putc(VDU_PLOT);
    vdp_putc(105); // Filled rectangle (using a common VDU extension)
    vdp_putw(x1); vdp_putw(y1);
    vdp_putw(x2); vdp_putw(y2);
}

void vdp_filled_circle(int x, int y, int radius) {
    vdp_putc(VDU_PLOT);
    vdp_putc(101); // Filled circle
    vdp_putw(x); vdp_putw(y);
    vdp_putw(radius);
}


// --- Benchmark Functions ---

void benchmark_primitives() {
    printf("Starting primitives benchmark...\n");
    long long start;

    // Lines
    start = get_time_ms();
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        vdp_set_graphics_color(rand() % 256);
        vdp_line(rand() % SCREEN_WIDTH, rand() % SCREEN_HEIGHT, rand() % SCREEN_WIDTH, rand() % SCREEN_HEIGHT);
    }
    print_results("Lines", start, get_time_ms(), NUM_OPERATIONS);
    vdp_clear_screen();

    // Filled Rectangles
    start = get_time_ms();
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        vdp_set_graphics_color(rand() % 256);
        vdp_filled_rectangle(rand() % SCREEN_WIDTH, rand() % SCREEN_HEIGHT, rand() % 50 + 10, rand() % 50 + 10);
    }
    print_results("Filled Rectangles", start, get_time_ms(), NUM_OPERATIONS);
    vdp_clear_screen();

    // Filled Circles
    start = get_time_ms();
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        vdp_set_graphics_color(rand() % 256);
        vdp_filled_circle(rand() % SCREEN_WIDTH, rand() % SCREEN_HEIGHT, rand() % 40 + 5);
    }
    print_results("Filled Circles", start, get_time_ms(), NUM_OPERATIONS);
    vdp_clear_screen();
}

void benchmark_bitmaps() {
    printf("Starting bitmap benchmark...\n");
    
    // 1. Define an 8x8 bitmap (a simple checkerboard)
    unsigned char bitmap_data[8] = {
        0b10101010, 0b01010101, 0b10101010, 0b01010101,
        0b10101010, 0b01010101, 0b10101010, 0b01010101
    };
    int bitmap_id = 1;

    // 2. Send command to define the bitmap
    unsigned char cmd_define[] = { VDU_DEFINE_BITMAP, bitmap_id, 8, 8 };
    vdp_send(cmd_define, sizeof(cmd_define));
    vdp_send(bitmap_data, sizeof(bitmap_data));
    
    // 3. Benchmark drawing the bitmap
    long long start = get_time_ms();
    for (int i = 0; i < NUM_OPERATIONS * 2; i++) {
        unsigned char cmd_draw[] = { VDU_DRAW_BITMAP, bitmap_id };
        vdp_send(cmd_draw, sizeof(cmd_draw));
        vdp_putw(rand() % SCREEN_WIDTH);
        vdp_putw(rand() % SCREEN_HEIGHT);
    }
    print_results("Bitmap Drawing (8x8)", start, get_time_ms(), NUM_OPERATIONS * 2);
    vdp_clear_screen();
}



void benchmark_sprites() {
    printf("Starting sprite benchmark...\n");

    // 1. Define the bitmap for the sprite (same as bitmap test)
    int bitmap_id_sprite = 2;
    int sprite_id = 1;

    unsigned char cmd_define[] = { VDU_DEFINE_BITMAP, bitmap_id_sprite, MARIO_SPRITE_WIDTH, MARIO_SPRITE_HEIGHT };
    vdp_send(cmd_define, sizeof(cmd_define));
    vdp_send(mario_sprite_data, sizeof(mario_sprite_data));

    // 2. Assign bitmap to sprite
    unsigned char cmd_assign[] = { VDU_ASSIGN_SPRITE, sprite_id, bitmap_id_sprite };
    vdp_send(cmd_assign, sizeof(cmd_assign));

    // 3. Activate sprite
    unsigned char cmd_activate[] = { VDU_ACTIVATE_SPRITE, sprite_id };
    vdp_send(cmd_activate, sizeof(cmd_activate));

    // 4. Benchmark moving the sprite
    long long start = get_time_ms();
    for (int i = 0; i < NUM_OPERATIONS * 4; i++) {
        unsigned char cmd_move[] = { VDU_MOVE_SPRITE_TO, sprite_id };
        vdp_send(cmd_move, sizeof(cmd_move));
        vdp_putw(rand() % SCREEN_WIDTH);
        vdp_putw(rand() % SCREEN_HEIGHT);
    }
    print_results("Sprite Movement", start, get_time_ms(), NUM_OPERATIONS * 4);

    // Deactivate sprite
    vdp_putc(23); vdp_putc(0); vdp_putc(84); vdp_putc(0); // Deactivate sprite 0 (assuming we need to hide it)
    vdp_clear_screen();
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <serial_port_device>\n", argv[0]);
        fprintf(stderr, "Example: %s /dev/ttyUSB0\n", argv[0]);
        return 1;
    }

    const char *portname = argv[1];
    serial_port = open(portname, O_RDWR | O_NOCTTY | O_SYNC);

    if (serial_port < 0) {
        perror("Error opening serial port");
        return 1;
    }

    struct termios tty;
    if (tcgetattr(serial_port, &tty) != 0) {
        perror("Error from tcgetattr");
        return 1;
    }

    cfsetospeed(&tty, B57600);
    cfsetispeed(&tty, B57600);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8-bit chars
    tty.c_iflag &= ~IGNBRK; // disable break processing
    tty.c_lflag = 0; // no signaling chars, no echo,
    tty.c_oflag = 0; // no remapping, no delays
    tty.c_cc[VMIN]  = 0; // read doesn't block
    tty.c_cc[VTIME] = 5; // 0.5 seconds read timeout
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl
    tty.c_cflag |= (CLOCAL | CREAD); // ignore modem controls, enable reading
    tty.c_cflag &= ~(PARENB | PARODD); // shut off parity
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
        perror("Error from tcsetattr");
        return 1;
    }
    
    printf("Serial port configured. Starting VDP benchmark in 3 seconds...\n");
    sleep(3);

    // Initial VDP setup
    vdp_putc(VDU_MODE); vdp_putc(136); // Mode 136: 640x480x64
    usleep(10000); // Wait for mode change
    vdp_clear_screen();
    unsigned char cmd_cursor_off[] = { VDU_CURSOR_OFF };
    vdp_send(cmd_cursor_off, sizeof(cmd_cursor_off));


    // Run benchmarks
    benchmark_primitives();
    benchmark_bitmaps();
    benchmark_sprites();

    // Cleanup
    close(serial_port);
    printf("Benchmark complete.\n");

    return 0;
}
