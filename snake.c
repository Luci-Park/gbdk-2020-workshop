#include <gb/gb.h>
#include <stdint.h>
// ========== SIMPLE RNG (no srand/rand needed) ==========
static uint16_t rng_state = 0xACE1u;
uint8_t rand8(void)
{
    rng_state = rng_state * 1103515245u + 12345u;
    return (uint8_t)(rng_state >> 8);
}

// ========== GAME CONSTANTS ==========
#define GRID_W 20 // 160 / 8
#define GRID_H 18 // 144 / 8

#define MAX_SPRITES 40
#define FOOD_SPRITE_INDEX 39
#define MAX_SNAKE (FOOD_SPRITE_INDEX) // 39 segments max

#define TILE_EMPTY  0
#define TILE_SNAKE  1
#define TILE_FOOD   2

#define FRAME_DELAY 6 // frames between movement steps

// ========== TILE DATA ==========
const uint8_t tile_empty[16] = {
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00
};

const uint8_t tile_snake[16] = {
    0xFF, 0x00,
    0xFF, 0x00,
    0xFF, 0x00,
    0xFF, 0x00,
    0xFF, 0x00,
    0xFF, 0x00,
    0xFF, 0x00,
    0xFF, 0x00
};

const uint8_t tile_food[16] = {
    0xFF, 0xFF,
    0xFF, 0xFF,
    0xFF, 0xFF,
    0xFF, 0xFF,
    0xFF, 0xFF,
    0xFF, 0xFF,
    0xFF, 0xFF,
    0xFF, 0xFF
};
// ========== GAME STATE ==========
uint8_t snake_x[MAX_SNAKE];
uint8_t snake_y[MAX_SNAKE];
uint8_t snake_len;

enum
{
    DIR_UP = 0,
    DIR_RIGHT,
    DIR_DOWN,
    DIR_LEFT
};
uint8_t dir = DIR_RIGHT;

uint8_t food_x, food_y;

// ========= FUNCTION PROTOTYPES =========
uint8_t rand8(void);
void place_sprite_on_grid(uint8_t index, uint8_t gx, uint8_t gy);
uint8_t is_on_snake(uint8_t x, uint8_t y);
void place_food_random_safe(void);
void init_snake(void);
void draw_snake(void);
void update_snake_position(void);
void grow_snake(void);
uint8_t opposite_dir(uint8_t d);
void handle_input_safe(void);

// ========== UTILITY FUNCTIONS ==========

void place_sprite_on_grid(uint8_t index, uint8_t gx, uint8_t gy)
{
    uint8_t px = gx * 8 + 8;
    uint8_t py = gy * 8 + 16;
    move_sprite(index, px, py);
}


// safe food placement
uint8_t is_on_snake(uint8_t x, uint8_t y) {
    for (uint8_t i = 0; i < snake_len; ++i)
        if (snake_x[i] == x && snake_y[i] == y) return 1;
    return 0;
}

void place_food_random_safe(void) {
    uint8_t x, y;
    do {
        x = rand8() % GRID_W;
        y = rand8() % GRID_H;
    } while (is_on_snake(x, y));
    food_x = x; food_y = y;
    place_sprite_on_grid(FOOD_SPRITE_INDEX, food_x, food_y);
}

// setup snake in center
void init_snake(void)
{
    snake_len = 3;
    uint8_t cx = GRID_W / 2;
    uint8_t cy = GRID_H / 2;
    for (uint8_t i = 0; i < snake_len; ++i)
    {
        snake_x[i] = cx - (snake_len - 1) + i;
        snake_y[i] = cy;
    }
    dir = DIR_RIGHT;
}

// draw all segments
void draw_snake(void)
{
    for (uint8_t i = 0; i < snake_len; ++i)
    {
        set_sprite_tile(TILE_SNAKE, 0);
        place_sprite_on_grid(i, snake_x[i], snake_y[i]);
    }
}

// update snake position + wrapping
void update_snake_position(void)
{
    for (uint8_t i = 0; i < snake_len - 1; ++i)
    {
        snake_x[i] = snake_x[i + 1];
        snake_y[i] = snake_y[i + 1];
    }

    int8_t hx = snake_x[snake_len - 1];
    int8_t hy = snake_y[snake_len - 1];

    if (dir == DIR_UP)
        hy--;
    else if (dir == DIR_DOWN)
        hy++;
    else if (dir == DIR_LEFT)
        hx--;
    else if (dir == DIR_RIGHT)
        hx++;

    if (hx < 0)
        hx = GRID_W - 1;
    else if (hx >= GRID_W)
        hx = 0;
    if (hy < 0)
        hy = GRID_H - 1;
    else if (hy >= GRID_H)
        hy = 0;

    snake_x[snake_len - 1] = (uint8_t)hx;
    snake_y[snake_len - 1] = (uint8_t)hy;
}

// grow snake by 1 segment
void grow_snake(void)
{
    if (snake_len >= MAX_SNAKE)
        return;
    for (int i = snake_len; i > 0; --i)
    {
        snake_x[i] = snake_x[i - 1];
        snake_y[i] = snake_y[i - 1];
    }
    snake_len++;
}

// handle safe input
uint8_t opposite_dir(uint8_t d) {
    return (d ^ 2); // DIR_UP(0)^2 -> 2 (DOWN), DIR_RIGHT(1)^2 -> 3 (LEFT) etc
}

void handle_input_safe(void) {
    uint8_t j = joypad();
    uint8_t new_dir = dir;
    if (j & J_UP) new_dir = DIR_UP;
    else if (j & J_DOWN) new_dir = DIR_DOWN;
    else if (j & J_LEFT) new_dir = DIR_LEFT;
    else if (j & J_RIGHT) new_dir = DIR_RIGHT;

    if (new_dir != opposite_dir(dir)) dir = new_dir;
}

// ========== MAIN LOOP ==========
void main(void)
{
    rng_state = DIV_REG; // seed RNG with hardware divider

    set_sprite_data(TILE_SNAKE, 1, tile_snake);
    set_sprite_data(TILE_FOOD, 1, tile_food);

    //set food sprite
    set_sprite_tile(FOOD_SPRITE_INDEX, TILE_FOOD);
    move_sprite(FOOD_SPRITE_INDEX, 0, 0);

    for (uint8_t i = 0; i < MAX_SPRITES; ++i)
    {
        if(i == FOOD_SPRITE_INDEX) continue;
        set_sprite_tile(i, TILE_SNAKE);
        move_sprite(i, 0, 0);
    }

    SHOW_SPRITES;
    DISPLAY_ON;

    init_snake();
    place_food_random_safe();
    draw_snake();
    place_sprite_on_grid(FOOD_SPRITE_INDEX, food_x, food_y);

    uint16_t frame_counter = 0;

    while (1)
    {
        wait_vbl_done();
        frame_counter++;

        handle_input_safe();

        if ((frame_counter % FRAME_DELAY) == 0)
        {
            update_snake_position();

            // check if head hits food
            if (snake_x[snake_len - 1] == food_x &&
                snake_y[snake_len - 1] == food_y)
            {
                grow_snake();
                place_food_random_safe();
            }

            draw_snake();
        }
    }
}
