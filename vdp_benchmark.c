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

    // Run Mario VDP demo
    printf("Starting Mario VDP demo in 2 seconds...\n");
    sleep(2);
    // Simple integration: spawn demo loop that uses vdp_* wrappers defined above
    // We'll implement a minimal demo here to send sprite move commands based on
    // the sequences described in mario_vdp_demo.c

    // Define bitmap for Mario (reuse existing mario_sprite_data)
    int bitmap_id_sprite = 2;
    int sprite_id = 1;

    unsigned char cmd_define[] = { VDU_DEFINE_BITMAP, bitmap_id_sprite, MARIO_SPRITE_WIDTH, MARIO_SPRITE_HEIGHT };
    vdp_send(cmd_define, sizeof(cmd_define));
    vdp_send(mario_sprite_data, sizeof(mario_sprite_data));

    unsigned char cmd_assign[] = { VDU_ASSIGN_SPRITE, sprite_id, bitmap_id_sprite };
    vdp_send(cmd_assign, sizeof(cmd_assign));
    unsigned char cmd_activate[] = { VDU_ACTIVATE_SPRITE, sprite_id };
    vdp_send(cmd_activate, sizeof(cmd_activate));

    // Demo loop: walk left, idle, walk right, idle, repeat; occasional jump
    const uint8_t walk_cycle[] = {1,2,3,4,5,4,3,2,1,2,3,4};
    const size_t walk_len = sizeof(walk_cycle)/sizeof(walk_cycle[0]);
    size_t walk_idx = 0;
    int dir = 0; // 0 = left (row 0), 1 = right (row 1)
    int state = 0; // 0 = walk, 1 = idle_pause, 2 = walk_back, 3 = idle
    int ticks = 0;
    const int frame_delay_ms = 120;
    const int idle_pause_ticks = (2000 / frame_delay_ms);

    while (1) {
        /* Deterministic demo sequence matching the preview GIF:
         * - walk (40 ticks) in current direction
         * - sliding frames (6,7,8) shown longer while still moving a bit
         * - idle for 250 ms, then jump (parabola)
         * - land, show idle for 500 ms, invert direction and repeat
         */
        const int walk_steps = 40;
        const int dx = 4; /* pixels per tick */
        const int frame_t = frame_delay_ms; /* base frame time */
        const int long_slide_t = 220; /* ms for slide frames */
        const int pre_jump_idle_ms = 250;
        const int post_land_idle_ms = 500;
        const uint8_t slide_seq[] = {6,7,8};
        const int slide_len = sizeof(slide_seq)/sizeof(slide_seq[0]);

        int x = (dir == 0) ? 1280 : 0; /* starting x for this leg */
        int y = 200;

        /* WALK phase */
        for (int s = 0; s < walk_steps; s++) {
            uint8_t col = walk_cycle[walk_idx % walk_len];
            walk_idx++;
            uint16_t sprite_idx = dir * (uint16_t)10 + (uint16_t)col;

            /* compute position */
            if (dir == 0) x -= dx; else x += dx;

            unsigned char cmd_move[] = { VDU_MOVE_SPRITE_TO, sprite_id };
            vdp_send(cmd_move, sizeof(cmd_move));
            vdp_putw(x);
            vdp_putw(y);

            usleep(frame_t * 1000);
        }

        /* SLIDE / BRAKE phase: show slide frames longer and continue to move slightly */
        for (int si = 0; si < slide_len; si++) {
            uint8_t col = slide_seq[si];
            uint16_t sprite_idx = dir * (uint16_t)10 + (uint16_t)col;

            /* continue moving slowly during slide */
            if (dir == 0) x -= dx/2; else x += dx/2;

            unsigned char cmd_move[] = { VDU_MOVE_SPRITE_TO, sprite_id };
            vdp_send(cmd_move, sizeof(cmd_move));
            vdp_putw(x);
            vdp_putw(y);

            usleep(long_slide_t * 1000);
        }

        /* Idle briefly before jump */
        {
            uint16_t idle_idx = dir * (uint16_t)10 + 0;
            unsigned char cmd_move[] = { VDU_MOVE_SPRITE_TO, sprite_id };
            vdp_send(cmd_move, sizeof(cmd_move));
            vdp_putw(x);
            vdp_putw(y);
            usleep(pre_jump_idle_ms * 1000);
        }

        /* Jump: simple vertical parabola */
        {
            uint16_t jump_idx = dir * (uint16_t)10 + 9;
            int jump_offsets[] = {0,-6,-12,-16,-12,-6,0};
            int jumps = sizeof(jump_offsets)/sizeof(jump_offsets[0]);
            for (int j = 0; j < jumps; j++) {
                unsigned char cmd_move[] = { VDU_MOVE_SPRITE_TO, sprite_id };
                vdp_send(cmd_move, sizeof(cmd_move));
                vdp_putw(x);
                vdp_putw(y + jump_offsets[j]);
                usleep(frame_t * 1000);
            }
        }

        /* Land idle for a bit */
        {
            unsigned char cmd_move[] = { VDU_MOVE_SPRITE_TO, sprite_id };
            vdp_send(cmd_move, sizeof(cmd_move));
            vdp_putw(x);
            vdp_putw(y);
            usleep(post_land_idle_ms * 1000);
        }

        /* invert direction and reset walk index */
        dir = (dir == 0) ? 1 : 0;
        walk_idx = 0;
    }

    // Deactivate sprite (unreachable)
    vdp_putc(23); vdp_putc(0); vdp_putc(84); vdp_putc(0); // Deactivate sprite 0
    vdp_clear_screen();

    // Cleanup
    close(serial_port);
    printf("Benchmark complete.\n");

    return 0;
}
