#ifndef ENGINE_H
#define ENGINE_H

#include <float.h>
#include <stdbool.h>
#include <stdlib.h>

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long ullong;
typedef long long llong;
typedef void (*func)();

typedef struct rec {
        int x, y;
        int w, h;
} rec;

typedef struct m2 {
        float m11, m12;
        float m21, m22;
} m2;

typedef struct v2 {
        float x;
        float y;
} v2;

typedef struct v2i {
        int x;
        int y;
} v2i;

typedef struct line {
        float x1;
        float y1;
        float x2;
        float y2;
        bool inf1;
        bool inf2;
} line;

typedef struct circ {
        float x;
        float y;
        float r;
} circ;

#define FPS 60
#define COLORS 32
#define COL_TRANSPARENT 255

#define MALLOC(s) mem_alloc(s)
#define FREE(p) mem_free(p)
#define REALLOC(p, s) mem_realloc(p, s)

enum colorID {
        COL_BLACK,
        COL_DARK_BLUE,
        COL_DARK_PURPLE,
        COL_DARK_GREEN,
        COL_BROWN,
        COL_DARK_GREY,
        COL_LIGHT_GREY,
        COL_WHITE,
        COL_RED,
        COL_ORANGE,
        COL_YELLOW,
        COL_GREEN,
        COL_BLUE,
        COL_LAVENDER,
        COL_PINK,
        COL_LIGHT_PEACH,
        COL_BROWNISH_BLACK,
        COL_DARKER_BLUE,
        COL_DARKER_PURPLE,
        COL_BLUE_GREEN,
        COL_DARK_BROWN,
        COL_DARKER_GREY,
        COL_MEDIUM_GREY,
        COL_LIGHT_YELLOW,
        COL_DARK_RED,
        COL_DARK_ORANGE,
        COL_LIME_GREEN,
        COL_MEDIUM_GREEN,
        COL_TRUE_BLUE,
        COL_MAUVE,
        COL_DARK_PEACH,
        COL_PEACH
};

typedef struct tex {
        ushort w;
        ushort h;
        ushort iw;
        ushort ih;
        uchar shift;
        uchar *px;
} tex;

typedef struct snd {
        ushort ID;
} snd;

typedef enum buttons {
        BTN_START = 1,
        BTN_SELECT = 2,
        BTN_LEFT = 4,
        BTN_RIGHT = 8,
        BTN_UP = 16,
        BTN_DOWN = 32,
        BTN_FACE_LEFT = 64,
        BTN_FACE_RIGHT = 128,
        BTN_FACE_UP = 256,
        BTN_FACE_DOWN = 512,
        BTN_L_SHOULDER = 1024,
        BTN_R_SHOULDER = 2048,
} buttons;

void vm_run();

// === GFX ===
void gfx_set_translation(int x, int y);
int gfx_width();
int gfx_height();
tex gfx_screen_tex();
tex gfx_tex_create(uint w, uint h);
tex gfx_tex_load(const char *);
void gfx_tex_destroy(tex);
void gfx_clear_tex(tex t, uchar c);
void gfx_clear(uchar);
void gfx_px(int x, int y, uchar col);
void gfx_sprite(tex s, int x, int y, rec r, uchar flags);
void gfx_sprite_affine(tex s, v2 p, v2 o, rec r, m2 m);
void gfx_line(int x1, int y1, int x2, int y2, uchar col);
void gfx_rec_filled(int x, int y, uint w, uint h, uchar col);
void gfx_rec_outline(int x, int y, uint w, uint h, uchar col);
void gfx_circle_filled(int xm, int ym, int r, uchar col);
void gfx_circle_outline(int xm, int ym, int r, uchar col);
uchar gfx_pal_get(int index);
void gfx_pal_shift(int s);
void gfx_pal_set(uchar index, uchar col);
void gfx_pal_reset();
void gfx_draw_to(tex);
void gfx_clip(int x, int y, int w, int h);
void gfx_unclip();

// === SND ===
snd aud_load(const char *);
void aud_play_sound(snd);
void aud_set_volume(float);
float aud_volume();

// === INP ===
buttons inp_state();
buttons inp_pstate();
int inp_xdir();
int inp_ydir();
bool inp_pressed(buttons);
bool inp_was_pressed(buttons);
bool inp_just_pressed(buttons);
bool inp_just_released(buttons);

// === MEM ===
void mem_init(void *, size_t);
void *mem_alloc(size_t);
void mem_free(void *);
void *mem_realloc(void *, size_t);

// === UTIL ===
char *read_txt(const char *filename);
ulong isqrt(ulong);

bool try_rec_intersection(rec a, rec b, rec *r);
rec rec_intersection(rec, rec);
bool rec_overlap(rec, rec);
int intersection_line_circ(line, circ, v2 *, v2 *);
int intersection_circ_circ(circ, circ, v2 *, v2 *);
bool intersection_line_line(line, line, v2 *);

m2 m2_I();
m2 m2_add(m2, m2);
m2 m2_sub(m2, m2);
m2 m2_mul(m2, m2);
m2 m2_scl(m2, float);
m2 m2_inv(m2);
m2 m2_aff_rot(float);
m2 m2_aff_scl(float x, float y);
m2 m2_aff_shr(float x, float y);
float m2_det(m2 a);

#define STREQ(a, b) (strcmp((a), (b)) == 0)
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define ABS(a) ((a) > 0 ? (a) : -(a))
#define COMP(a, b) (((a) < (b)) - ((a) > (b)))
#define SIGN(a) COMP(0, a)

static inline v2 v2_add(v2 a, v2 b)
{
        return (v2){a.x + b.x, a.y + b.y};
}

static inline v2 v2_sub(v2 a, v2 b)
{
        return (v2){a.x - b.x, a.y - b.y};
}

static inline v2 v2_min(v2 a, v2 b)
{
        return (v2){MIN(a.x, b.x), MIN(a.y, b.y)};
}

static inline v2 v2_max(v2 a, v2 b)
{
        return (v2){MAX(a.x, b.x), MAX(a.y, b.y)};
}

static inline float v2_dot(v2 a, v2 b)
{
        return a.x * b.x + a.y + b.y;
}

static inline v2 v2_scl(v2 a, float s)
{
        return (v2){a.x * s, a.y * s};
}

static inline v2 m2v2_mul(m2 m, v2 v)
{
        return (v2){m.m11 * v.x + m.m12 * v.y, m.m21 * v.x + m.m22 * v.y};
}

#define B_00000000 0x00
#define B_00000001 0x01
#define B_00000010 0x02
#define B_00000011 0x03
#define B_00000100 0x04
#define B_00000101 0x05
#define B_00000110 0x06
#define B_00000111 0x07
#define B_00001000 0x08
#define B_00001001 0x09
#define B_00001010 0x0A
#define B_00001011 0x0B
#define B_00001100 0x0C
#define B_00001101 0x0D
#define B_00001110 0x0E
#define B_00001111 0x0F
#define B_00010000 0x10
#define B_00010001 0x11
#define B_00010010 0x12
#define B_00010011 0x13
#define B_00010100 0x14
#define B_00010101 0x15
#define B_00010110 0x16
#define B_00010111 0x17
#define B_00011000 0x18
#define B_00011001 0x19
#define B_00011010 0x1A
#define B_00011011 0x1B
#define B_00011100 0x1C
#define B_00011101 0x1D
#define B_00011110 0x1E
#define B_00011111 0x1F
#define B_00100000 0x20
#define B_00100001 0x21
#define B_00100010 0x22
#define B_00100011 0x23
#define B_00100100 0x24
#define B_00100101 0x25
#define B_00100110 0x26
#define B_00100111 0x27
#define B_00101000 0x28
#define B_00101001 0x29
#define B_00101010 0x2A
#define B_00101011 0x2B
#define B_00101100 0x2C
#define B_00101101 0x2D
#define B_00101110 0x2E
#define B_00101111 0x2F
#define B_00110000 0x30
#define B_00110001 0x31
#define B_00110010 0x32
#define B_00110011 0x33
#define B_00110100 0x34
#define B_00110101 0x35
#define B_00110110 0x36
#define B_00110111 0x37
#define B_00111000 0x38
#define B_00111001 0x39
#define B_00111010 0x3A
#define B_00111011 0x3B
#define B_00111100 0x3C
#define B_00111101 0x3D
#define B_00111110 0x3E
#define B_00111111 0x3F
#define B_01000000 0x40
#define B_01000001 0x41
#define B_01000010 0x42
#define B_01000011 0x43
#define B_01000100 0x44
#define B_01000101 0x45
#define B_01000110 0x46
#define B_01000111 0x47
#define B_01001000 0x48
#define B_01001001 0x49
#define B_01001010 0x4A
#define B_01001011 0x4B
#define B_01001100 0x4C
#define B_01001101 0x4D
#define B_01001110 0x4E
#define B_01001111 0x4F
#define B_01010000 0x50
#define B_01010001 0x51
#define B_01010010 0x52
#define B_01010011 0x53
#define B_01010100 0x54
#define B_01010101 0x55
#define B_01010110 0x56
#define B_01010111 0x57
#define B_01011000 0x58
#define B_01011001 0x59
#define B_01011010 0x5A
#define B_01011011 0x5B
#define B_01011100 0x5C
#define B_01011101 0x5D
#define B_01011110 0x5E
#define B_01011111 0x5F
#define B_01100000 0x60
#define B_01100001 0x61
#define B_01100010 0x62
#define B_01100011 0x63
#define B_01100100 0x64
#define B_01100101 0x65
#define B_01100110 0x66
#define B_01100111 0x67
#define B_01101000 0x68
#define B_01101001 0x69
#define B_01101010 0x6A
#define B_01101011 0x6B
#define B_01101100 0x6C
#define B_01101101 0x6D
#define B_01101110 0x6E
#define B_01101111 0x6F
#define B_01110000 0x70
#define B_01110001 0x71
#define B_01110010 0x72
#define B_01110011 0x73
#define B_01110100 0x74
#define B_01110101 0x75
#define B_01110110 0x76
#define B_01110111 0x77
#define B_01111000 0x78
#define B_01111001 0x79
#define B_01111010 0x7A
#define B_01111011 0x7B
#define B_01111100 0x7C
#define B_01111101 0x7D
#define B_01111110 0x7E
#define B_01111111 0x7F
#define B_10000000 0x80
#define B_10000001 0x81
#define B_10000010 0x82
#define B_10000011 0x83
#define B_10000100 0x84
#define B_10000101 0x85
#define B_10000110 0x86
#define B_10000111 0x87
#define B_10001000 0x88
#define B_10001001 0x89
#define B_10001010 0x8A
#define B_10001011 0x8B
#define B_10001100 0x8C
#define B_10001101 0x8D
#define B_10001110 0x8E
#define B_10001111 0x8F
#define B_10010000 0x90
#define B_10010001 0x91
#define B_10010010 0x92
#define B_10010011 0x93
#define B_10010100 0x94
#define B_10010101 0x95
#define B_10010110 0x96
#define B_10010111 0x97
#define B_10011000 0x98
#define B_10011001 0x99
#define B_10011010 0x9A
#define B_10011011 0x9B
#define B_10011100 0x9C
#define B_10011101 0x9D
#define B_10011110 0x9E
#define B_10011111 0x9F
#define B_10100000 0xA0
#define B_10100001 0xA1
#define B_10100010 0xA2
#define B_10100011 0xA3
#define B_10100100 0xA4
#define B_10100101 0xA5
#define B_10100110 0xA6
#define B_10100111 0xA7
#define B_10101000 0xA8
#define B_10101001 0xA9
#define B_10101010 0xAA
#define B_10101011 0xAB
#define B_10101100 0xAC
#define B_10101101 0xAD
#define B_10101110 0xAE
#define B_10101111 0xAF
#define B_10110000 0xB0
#define B_10110001 0xB1
#define B_10110010 0xB2
#define B_10110011 0xB3
#define B_10110100 0xB4
#define B_10110101 0xB5
#define B_10110110 0xB6
#define B_10110111 0xB7
#define B_10111000 0xB8
#define B_10111001 0xB9
#define B_10111010 0xBA
#define B_10111011 0xBB
#define B_10111100 0xBC
#define B_10111101 0xBD
#define B_10111110 0xBE
#define B_10111111 0xBF
#define B_11000000 0xC0
#define B_11000001 0xC1
#define B_11000010 0xC2
#define B_11000011 0xC3
#define B_11000100 0xC4
#define B_11000101 0xC5
#define B_11000110 0xC6
#define B_11000111 0xC7
#define B_11001000 0xC8
#define B_11001001 0xC9
#define B_11001010 0xCA
#define B_11001011 0xCB
#define B_11001100 0xCC
#define B_11001101 0xCD
#define B_11001110 0xCE
#define B_11001111 0xCF
#define B_11010000 0xD0
#define B_11010001 0xD1
#define B_11010010 0xD2
#define B_11010011 0xD3
#define B_11010100 0xD4
#define B_11010101 0xD5
#define B_11010110 0xD6
#define B_11010111 0xD7
#define B_11011000 0xD8
#define B_11011001 0xD9
#define B_11011010 0xDA
#define B_11011011 0xDB
#define B_11011100 0xDC
#define B_11011101 0xDD
#define B_11011110 0xDE
#define B_11011111 0xDF
#define B_11100000 0xE0
#define B_11100001 0xE1
#define B_11100010 0xE2
#define B_11100011 0xE3
#define B_11100100 0xE4
#define B_11100101 0xE5
#define B_11100110 0xE6
#define B_11100111 0xE7
#define B_11101000 0xE8
#define B_11101001 0xE9
#define B_11101010 0xEA
#define B_11101011 0xEB
#define B_11101100 0xEC
#define B_11101101 0xED
#define B_11101110 0xEE
#define B_11101111 0xEF
#define B_11110000 0xF0
#define B_11110001 0xF1
#define B_11110010 0xF2
#define B_11110011 0xF3
#define B_11110100 0xF4
#define B_11110101 0xF5
#define B_11110110 0xF6
#define B_11110111 0xF7
#define B_11111000 0xF8
#define B_11111001 0xF9
#define B_11111010 0xFA
#define B_11111011 0xFB
#define B_11111100 0xFC
#define B_11111101 0xFD
#define B_11111110 0xFE
#define B_11111111 0xFF

#endif