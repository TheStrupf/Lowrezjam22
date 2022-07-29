#include "engine.h"
#include "game.h"
#include "jsonp.h"
#include "raylib.h"
#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const double DELTA = 1.0 / (double)FPS;

// rgba values for the extended pico-8 palette
// clang-format off
uchar pico_pal[128] = {
        0x00, 0x00, 0x00, 0xFF, 0x1D, 0x2B, 0x53, 0xFF,
        0x7E, 0x25, 0x53, 0xFF, 0x00, 0x87, 0x51, 0xFF,
        0xAB, 0x52, 0x36, 0xFF, 0x5F, 0x57, 0x4F, 0xFF,
        0xC2, 0xC3, 0xC7, 0xFF, 0xFF, 0xF1, 0xE8, 0xFF,
        0xFF, 0x00, 0x4D, 0xFF, 0xFF, 0xA3, 0x00, 0xFF,
        0xFF, 0xEC, 0x27, 0xFF, 0x00, 0xE4, 0x36, 0xFF,
        0x29, 0xAD, 0xFF, 0xFF, 0x83, 0x76, 0x9C, 0xFF,
        0xFF, 0x77, 0xA8, 0xFF, 0xFF, 0xCC, 0xAA, 0xFF,
        0x29, 0x18, 0x14, 0xFF, 0x11, 0x1D, 0x35, 0xFF,
        0x42, 0x21, 0x36, 0xFF, 0x12, 0x53, 0x59, 0xFF,
        0x74, 0x2F, 0x29, 0xFF, 0x49, 0x33, 0x3B, 0xFF,
        0xA2, 0x88, 0x79, 0xFF, 0xF3, 0xEF, 0x7D, 0xFF,
        0xBE, 0x12, 0x50, 0xFF, 0xFF, 0x6C, 0x24, 0xFF,
        0xA8, 0xE7, 0x2E, 0xFF, 0x00, 0xB5, 0x43, 0xFF,
        0x06, 0x5A, 0xB5, 0xFF, 0x75, 0x46, 0x65, 0xFF,
        0xFF, 0x6E, 0x59, 0xFF, 0xFF, 0x9D, 0x81, 0xFF,
};
// clang-format on

#define GAME_WIDTH 64
#define GAME_HEIGHT 64
#define BUFFER_WIDTH 64
#define BUFFER_HEIGHT 64

typedef struct block {
        size_t size;
        struct block *next;
} block;

static struct core {
        // GFX
        Texture2D ray_screentex;
        uchar *screenbuffer;
        tex screentex;
        tex target;
        int offsetx;
        int offsety;
        int clipx1;
        int clipy1;
        int clipx2;
        int clipy2;
        uchar palette[256];
        int width;
        int height;

        // INP
        buttons btn_state;
        buttons btn_pstate;

        // SND
        Sound sounds[256];
        int soundID;
        float volume;
        bool snd_okay;
        block *b_free;
} core;

static void inp_update();

void vm_run()
{
        core = (struct core){0};
        void *memory = malloc(256 * 1024 * 1024);
        if (!memory) return;
        mem_init(memory, 256 * 1024 * 1024);
        core.width = GAME_WIDTH;
        core.height = GAME_HEIGHT;
        core.volume = 1.0f;
        core.soundID = -1;

        SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI);
        InitWindow(800, 600, "Hello Git");
        SetTargetFPS(144);
        Image img = GenImageColor(BUFFER_WIDTH, BUFFER_HEIGHT, BLACK);
        core.ray_screentex = LoadTextureFromImage(img);
        UnloadImage(img);
        core.ray_screentex.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        core.screenbuffer = MALLOC(BUFFER_WIDTH * BUFFER_HEIGHT * 4); // 4 bytes per pixel
        core.screentex = gfx_tex_create(BUFFER_WIDTH, BUFFER_HEIGHT);
        InitAudioDevice();
        core.snd_okay = IsAudioDeviceReady();
        game_init();

        double acc = 0;
        double lasttime = GetTime();
        while (!WindowShouldClose()) {
                double time = GetTime();
                acc += time - lasttime;
                lasttime = time;

                // cap delta accumulator ("spiral of death")
                acc = MIN(acc, 0.1);
                if (acc >= DELTA) {
                        do {
                                inp_update();
                                game_tick();
                                acc -= DELTA;
                        } while (acc >= DELTA);

                        gfx_draw_to(core.screentex);
                        gfx_set_translation(0, 0);
                        gfx_clear(0);
                        gfx_pal_reset();
                        game_draw();
                        gfx_rec_filled(-20, 10, 15, 15, 1);
                        for (int y = 0, i = 0; y < BUFFER_HEIGHT; y++) {
                                const int c = y * BUFFER_WIDTH;
                                for (int x = 0; x < BUFFER_WIDTH; x++, i += 4)
                                        memcpy(&core.screenbuffer[i], &pico_pal[core.screentex.px[x + c] << 2], 4);
                        }
                        UpdateTexture(core.ray_screentex, core.screenbuffer);
                }
                // letterboxing
                int sw = GetScreenWidth();
                int sh = GetScreenHeight();
                float scalex = (float)sw / (float)core.width;
                float scaley = (float)sh / (float)core.height;
                int gw, gh, tx, ty;
                if (scalex < scaley) { // black bars top and bottom
                        gw = sw;
                        gh = core.height * scalex;
                        tx = 0;
                        ty = (int)((sh - gh) * 0.5);
                } else { // black bars left and right
                        gw = core.width * scaley;
                        gh = sh;
                        tx = (int)((sw - gw) * 0.5);
                        ty = 0;
                }

                BeginDrawing();
                ClearBackground(WHITE);
                DrawTexturePro(core.ray_screentex,
                               (Rectangle){0, 0, core.width, core.height},
                               (Rectangle){tx, ty, gw + 0.5, gh + 0.5},
                               (Vector2){0, 0}, 0, WHITE);
                EndDrawing();
        }
        if (core.snd_okay) {
                for (int n = core.soundID; n >= 0; n--)
                        UnloadSound(core.sounds[n]);
                CloseAudioDevice();
        }
        UnloadTexture(core.ray_screentex);
        CloseWindow();
        free(memory);
}

void gfx_set_translation(int x, int y)
{
        core.offsetx = x;
        core.offsety = y;
}

int gfx_width() { return core.width; }

int gfx_height() { return core.height; }

tex gfx_screen_tex() { return core.screentex; }

tex gfx_tex_create(uint w, uint h)
{
        tex t;
        t.w = 0;
        t.h = 0;
        t.iw = 1;
        t.ih = 1;
        t.shift = 0;
        while (t.iw < w) {
                t.iw <<= 1;
                t.shift++;
        }
        while (t.ih < h) t.ih <<= 1;
        t.px = MALLOC(t.iw * t.ih);
        if (t.px) {
                t.w = w;
                t.h = h;
                gfx_clear_tex(t, 0);
        }
        return t;
}

tex gfx_tex_load(const char *filename)
{
        tex t;
        char *text = read_txt(filename);
        if (text) {
                jsn toks[16];
                json_parse(text, toks, 16);
                jsn *root = &toks[0];
                t = gfx_tex_create(json_ik(root, "width"), json_ik(root, "height"));
                int i = json_get(root, "data")->start;
                for (int y = 0; y < t.h; y++) {
                        int cache = y << t.shift;
                        for (int x = 0; x < t.w; x++) {
                                char a = text[i++];
                                char b = text[i++];
                                a = (a <= '9') ? a - '0' : (a & 0x7) + 9;
                                b = (b <= '9') ? b - '0' : (b & 0x7) + 9;
                                t.px[cache + x] = (uchar)((a << 4) + b);
                        }
                }
                FREE(text);
        } else t = gfx_tex_create(16, 16);

        return t;
}

void gfx_tex_destroy(tex t)
{
        if (t.px) FREE(t.px);
}

void gfx_clear_tex(tex t, uchar c)
{
        if (t.px) memset(t.px, c, t.iw * t.ih);
}

void gfx_clear(uchar col) { gfx_clear_tex(core.target, col); }

void gfx_px(int x, int y, uchar col)
{
        col = core.palette[col];
        if (col >= COLORS) return;
        x += core.offsetx;
        y += core.offsety;
        if ((uint)x < core.target.w && (uint)y < core.target.h)
                core.target.px[x + (y << core.target.shift)] = col;
}

void gfx_sprite(tex s, int x, int y, rec r, uchar flags)
{
        int srcx, srcy;
        int xa = 0;
        int ya = 0;
        switch (flags) {
        case B_00000000:
                srcx = 1;
                srcy = s.w;
                break;
        case B_00000001:
                srcx = -1;
                srcy = s.w;
                xa = r.w - 1;
                break;
        case B_00000010:
                srcx = 1;
                srcy = -s.w;
                ya = r.h - 1;
                break;
        case B_00000011:
                srcx = -1;
                srcy = -s.w;
                xa = r.w - 1;
                ya = r.h - 1;
                break;
        case B_00000100:
                srcx = s.w;
                srcy = 1;
                break;
        case B_00000101:
                srcx = -s.w;
                srcy = 1;
                ya = r.h - 1;
                break;
        case B_00000110:
                srcx = s.w;
                srcy = -1;
                xa = r.w - 1;
                break;
        case B_00000111:
                srcx = -s.w;
                srcy = -1;
                xa = r.w - 1;
                ya = r.h - 1;
                break;
        default: return;
        }
        x += core.offsetx;
        y += core.offsety;
        int x1 = MAX(core.clipx1, -x);
        int y1 = MAX(core.clipy1, -y);
        int x2 = MIN((flags & 0x4) ? r.h : r.w, core.clipx2 - x);
        int y2 = MIN((flags & 0x4) ? r.w : r.h, core.clipy2 - y);
        int c1 = x + (y << core.target.shift);
        int c2 = r.x + xa + ((r.y + ya) << s.shift);

        // Loops over target area and maps back to source pixel
        for (int yi = y1; yi < y2; yi++) {
                int cc = (yi << core.target.shift) + c1;
                int cz = yi * srcy + c2;
                for (int xi = x1; xi < x2; xi++) {
                        uchar col = core.palette[s.px[srcx * xi + cz]];
                        if (col < COLORS)
                                core.target.px[xi + cc] = col;
                }
        }
}

void gfx_sprite_affine(tex s, v2 p, v2 o, rec r, m2 m)
{
        p.x += core.offsetx;
        p.y += core.offsety;
        p = v2_add(p, o);
        o.x += r.x;
        o.y += r.y;
        const int x2 = r.x + r.w;
        const int y2 = r.y + r.h;
        const m2 m_inv = m2_inv(m);

        // Calculate AABB rectangle by transforming
        // the rectangle's four corners and calculating
        // the min/max for the x & and y axis

        // source texture
        // a---b
        // |   |
        // c---d

        // transform point a
        v2 v = m2v2_mul(m_inv, p);
        v2 t = (v2){r.x - 1, r.y - 1};
        t = v2_add(t, v);
        t = v2_sub(t, o);
        v = m2v2_mul(m, t);
        v2 p1 = v;
        v2 p2 = v;
        t.y += r.h + 2; // transform point c
        v = m2v2_mul(m, t);
        p1 = v2_min(p1, v);
        p2 = v2_max(p2, v);
        t.x += r.w + 2; // transform point d
        v = m2v2_mul(m, t);
        p1 = v2_min(p1, v);
        p2 = v2_max(p2, v);
        t.y -= r.h + 2; // transform point b
        v = m2v2_mul(m, t);
        p1 = v2_min(p1, v);
        p2 = v2_max(p2, v);

        // calculate target area
        int dx1 = MAX((int)(p1.x - 1), core.clipx1);
        int dy1 = MAX((int)(p1.y - 1), core.clipy1);
        int dx2 = MIN((int)(p2.x + 1), core.clipx2);
        int dy2 = MIN((int)(p2.y + 1), core.clipy2);
        p.y -= 0.5f; // rounding - midpoint of screen pixel
        p.x -= 0.5f;

        // loop over target area and map
        // each SCREEN pixel back to a SOURCE pixel
        for (int dy = dy1; dy < dy2; dy++) {
                float dyt = dy - p.y;
                float c1 = m_inv.m12 * dyt + o.x;
                float c2 = m_inv.m22 * dyt + o.y;
                int cc = dy << core.target.shift;
                for (int dx = dx1; dx < dx2; dx++) {
                        float dxt = dx - p.x;
                        int spx = (int)(m_inv.m11 * dxt + c1);
                        if (spx < r.x || spx >= x2) continue;
                        int spy = (int)(m_inv.m21 * dxt + c2);
                        if (spy < r.y || spy >= y2) continue;
                        uchar col = core.palette[s.px[spx + (spy << s.shift)]];
                        if (col < COLORS)
                                core.target.px[dx + cc] = col;
                }
        }
}

// bresenham
void gfx_line(int x1, int y1, int x2, int y2, uchar col)
{
        col = core.palette[col];
        if (col >= COLORS) return;
        const int dx = ABS(x2 - x1);
        const int dy = -ABS(y2 - y1);
        const int sx = x2 > x1 ? 1 : -1;
        const int sy = y2 > y1 ? 1 : -1;
        const int steps = MAX(dx, -dy);
        int err = dx + dy;
        int xi = x1 + core.offsetx;
        int yi = y1 + core.offsety;
        for (int n = 0; n <= steps; n++) {
                if (core.clipx1 <= xi && xi < core.clipx2 && core.clipy1 <= yi && yi < core.clipy2)
                        core.target.px[xi + (yi << core.target.shift)] = col;
                int e2 = err << 1;
                if (e2 >= dy) {
                        err += dy;
                        xi += sx;
                }
                if (e2 <= dx) {
                        err += dx;
                        yi += sy;
                }
        }
}

void gfx_rec_filled(int x, int y, uint w, uint h, uchar col)
{
        col = core.palette[col];
        if (col >= COLORS) return;
        x += core.offsetx;
        y += core.offsety;
        const int x2 = MIN(x + (int)w, core.clipx2);
        const int y2 = MIN(y + (int)h, core.clipy2);
        if (x2 <= core.clipx1 || y2 <= core.clipy1 || x >= core.clipx2 || y >= core.clipy2)
                return;
        const int x1 = MAX(x, core.clipx1);
        const int y1 = MAX(y, core.clipy1);
        const int l = x2 - x1;
        for (int yi = y1; yi < y2; yi++)
                memset(&core.target.px[x1 + (yi << core.target.shift)], col, l);
}

void gfx_rec_outline(int x, int y, uint w, uint h, uchar col)
{
        const int x1 = x;
        const int y1 = y;
        const int x2 = x + (int)w;
        const int y2 = y + (int)h;
        gfx_line(x1, y1, x1, y2, col);
        gfx_line(x1, y1, x2, y1, col);
        gfx_line(x2, y1, x2, y2, col);
        gfx_line(x1, y2, x2, y2, col);
}

static void gfx_internal_circle(int y, int x1, int x2, uchar col, bool filled)
{
        if (y < core.clipy1 || y >= core.clipy2) return;
        if (filled) {
                y <<= core.target.shift;
                x1 = MAX(x1, core.clipx1) + y;
                x2 = MIN(x2, core.clipx2) + y;
                for (int n = x1; n < x2; n++)
                        core.target.px[n] = col;
        } else {
                y <<= core.target.shift;
                if (core.clipx1 <= x1 && x1 < core.clipx2)
                        core.target.px[x1 + y] = col;
                if (core.clipx1 <= x2 && x2 < core.clipx2)
                        core.target.px[x2 + y] = col;
        }
}

static void gfx_circle(int xm, int ym, int r, uchar col, bool filled)
{
        col = core.palette[col];
        if (col >= COLORS) return;
        xm += core.offsetx;
        ym += core.offsety;

        int x = 0;
        int y = r;
        int d = 3 - 2 * r;
        gfx_internal_circle(ym + y, xm, xm, col, filled);
        gfx_internal_circle(ym - y, xm, xm, col, filled);
        gfx_internal_circle(ym, xm - y, xm + y, col, filled);
        while (y >= x++) {
                if (d > 0) d += ((x - --y) << 2) + 10;
                else d += (x << 2) + 6;
                gfx_internal_circle(ym + y, xm - x, xm + x, col, filled);
                gfx_internal_circle(ym - y, xm - x, xm + x, col, filled);
                gfx_internal_circle(ym + x, xm - y, xm + y, col, filled);
                gfx_internal_circle(ym - x, xm - y, xm + y, col, filled);
        }
}

void gfx_circle_filled(int xm, int ym, int r, uchar col)
{
        gfx_circle(xm, ym, r, col, true);
}

void gfx_circle_outline(int xm, int ym, int r, uchar col)
{
        gfx_circle(xm, ym, r, col, false);
}

uchar gfx_pal_get(int index)
{
        return core.palette[index];
}

void gfx_pal_shift(int s)
{
        // unimplement yet
}

void gfx_pal_set(uchar index, uchar col)
{
        core.palette[index] = col;
}

void gfx_pal_reset()
{
        for (int n = 0; n < 256; n++)
                core.palette[n] = n != COL_TRANSPARENT ? n : 255;
}

void gfx_draw_to(tex t)
{
        core.target = t;
        gfx_unclip();
}

void gfx_clip(int x, int y, int w, int h)
{
        core.clipx1 = MAX(x, 0);
        core.clipy1 = MAX(y, 0);
        core.clipx2 = MIN(x + w, core.target.w);
        core.clipy2 = MIN(y + h, core.target.h);
}

void gfx_unclip()
{
        core.clipx1 = 0;
        core.clipy1 = 0;
        core.clipx2 = core.target.w;
        core.clipy2 = core.target.h;
}

static void inp_update()
{
        core.btn_pstate = core.btn_state;
        core.btn_state = 0;
        if (IsKeyDown(KEY_ENTER))
                core.btn_state |= BTN_START;
        if (IsKeyDown(KEY_BACKSPACE))
                core.btn_state |= BTN_SELECT;
        if (IsKeyDown(KEY_A))
                core.btn_state |= BTN_LEFT;
        if (IsKeyDown(KEY_D))
                core.btn_state |= BTN_RIGHT;
        if (IsKeyDown(KEY_W))
                core.btn_state |= BTN_UP;
        if (IsKeyDown(KEY_S))
                core.btn_state |= BTN_DOWN;
        if (IsKeyDown(KEY_LEFT))
                core.btn_state |= BTN_FACE_LEFT;
        if (IsKeyDown(KEY_RIGHT))
                core.btn_state |= BTN_FACE_RIGHT;
        if (IsKeyDown(KEY_UP))
                core.btn_state |= BTN_FACE_UP;
        if (IsKeyDown(KEY_DOWN))
                core.btn_state |= BTN_FACE_DOWN;
        if (IsKeyDown(KEY_Q))
                core.btn_state |= BTN_L_SHOULDER;
        if (IsKeyDown(KEY_R))
                core.btn_state |= BTN_R_SHOULDER;
}

buttons inp_state() { return core.btn_state; }

buttons inp_pstate() { return core.btn_pstate; }

int inp_xdir()
{
        if (inp_pressed(BTN_LEFT | BTN_RIGHT)) return 0;
        if (inp_pressed(BTN_RIGHT)) return 1;
        if (inp_pressed(BTN_LEFT)) return -1;
        return 0;
}

int inp_ydir()
{
        if (inp_pressed(BTN_UP | BTN_DOWN)) return 0;
        if (inp_pressed(BTN_DOWN)) return 1;
        if (inp_pressed(BTN_UP)) return -1;
        return 0;
}

bool inp_pressed(buttons b)
{
        return (core.btn_state & b) == b;
}

bool inp_was_pressed(buttons b)
{
        return (core.btn_pstate & b) == b;
}

bool inp_just_pressed(buttons b)
{
        return (!inp_was_pressed(b) && inp_pressed(b));
}

bool inp_just_released(buttons b)
{
        return (inp_was_pressed(b) && !inp_pressed(b));
}

snd aud_load(const char *filename)
{
        snd s;
        s.ID = 0;
        if (core.snd_okay) {
                Sound sound = LoadSound(filename);
                s.ID = ++core.soundID;
        }
        return s;
}

void aud_play_sound(snd s)
{
        if (core.snd_okay && s.ID >= 0)
                PlaySound(core.sounds[s.ID]);
}

void aud_set_volume(float v)
{
        if (!core.snd_okay) return;
        v = MAX(v, 1.0f);
        v = MIN(v, 0.0f);
        SetMasterVolume(v);
        core.volume = v;
}

float aud_volume()
{
        if (!core.snd_okay) return 0;
        return core.volume;
}

#define ALIGN(s) ((s + ALIGNMENT - 1) & -ALIGNMENT)

static const int ALIGNMENT = sizeof(void *);
static const int HEADER = ALIGN(sizeof(block));
static const int SPLIT_THRESH = HEADER + ALIGNMENT;

void mem_init(void *memptr, size_t memsize)
{
        core.b_free = (block *)memptr;
        core.b_free->size = memsize;
        core.b_free->next = NULL;
}

void *mem_alloc(size_t s)
{
        if (s == 0) return NULL;
        const size_t ALLOC_SIZE = ALIGN(s) + HEADER;
        for (block *b = core.b_free, *p = NULL; b; p = b, b = b->next) {
                if (b->size >= ALLOC_SIZE) {
                        size_t excess = b->size - ALLOC_SIZE;
                        block *tmp = b->next;
                        if (excess >= SPLIT_THRESH) {
                                // split blocks
                                b->size = ALLOC_SIZE;
                                tmp = (block *)((char *)b + ALLOC_SIZE);
                                tmp->size = excess;
                                tmp->next = b->next;
                        }
                        // re-link free list
                        if (p) p->next = tmp;
                        else core.b_free = tmp;
                        return (void *)((char *)b + HEADER);
                }
        }
        printf("+++ ERR: Couldn't alloc memory\n");
        return NULL;
}

void *mem_realloc(void *p, size_t s)
{
        if (!p) return mem_alloc(s);
        const block *b = (block *)((char *)p - HEADER);
        const size_t usersize = b->size - HEADER;
        if (usersize < s) {
                void *newblock = mem_alloc(s);
                if (newblock) {
                        memcpy(newblock, p, usersize);
                        mem_free(p);
                        return newblock;
                }
                return NULL;
        }
        return p;
}

void mem_free(void *p)
{
        if (!p) return;
        block *b = (block *)((char *)p - HEADER);
        block *prev = b->next = NULL;
        const char *b_ptr = (char *)b;

        // iterate free list to insert b (memory sorted)
        for (block *it = core.b_free; it; prev = it, it = it->next) {
                if (b_ptr < (char *)it) {
                        // insert b before it
                        if (b_ptr + b->size == (char *)it) {
                                // end of block b ends at it: merge blocks
                                b->size += it->size;
                                b->next = it->next;
                        } else b->next = it;
                        break;
                }
        }

        if (prev) {
                if ((char *)prev + prev->size == (char *)b) {
                        // previous block p ends at b: merge blocks
                        prev->size += b->size;
                        prev->next = b->next;
                } else prev->next = b;
        } else core.b_free = b;
}

char *read_txt(const char *filename)
{
        FILE *fptr = fopen(filename, "r");
        if (!fptr) {
                printf("Error opening txt\n");
                return NULL;
        }
        fseek(fptr, 0, SEEK_END);
        long length = ftell(fptr);
        fseek(fptr, 0, SEEK_SET);
        char *txt = MALLOC(length + 1);
        if (!txt) {
                printf("Error allocating txt buffer\n");
                fclose(fptr);
                return NULL;
        }
        fread(txt, 1, length, fptr);
        txt[length] = '\0';
        fclose(fptr);
        return txt;
}

ulong isqrt(ulong x)
{
        ulong r = x, q = 0, b = 0x40000000;
        while (b > r) b >>= 2;
        while (b > 0) {
                ulong t = q + b;
                q >>= 1;
                if (r >= t) {
                        r -= t;
                        q += b;
                }
                b >>= 2;
        }
        return q;
}

bool try_rec_intersection(rec a, rec b, rec *r)
{
        rec c = rec_intersection(a, b);
        if (c.w > 0 && c.h > 0) {
                *r = c;
                return true;
        }
        return false;
}

rec rec_intersection(rec a, rec b)
{
        rec c;
        c.x = MAX(a.x, b.x);
        c.y = MAX(a.y, b.y);
        c.w = MIN(a.x + a.w, b.x + b.w) - c.x;
        c.h = MIN(a.y + a.h, b.y + b.h) - c.y;
        return c;
}

bool rec_overlap(rec a, rec b)
{
        return (
            a.x < b.x + b.w &&
            a.y < b.y + b.h &&
            b.x < a.x + a.w &&
            b.y < a.y + a.h);
}

int intersection_line_circ(line l, circ c, v2 *outa, v2 *outb)
{
        m2 m;
        l.x1 -= c.x;
        l.x2 -= c.x;
        l.y1 -= c.y;
        l.y2 -= c.y;
        m.m11 = l.x1;
        m.m12 = l.x2;
        m.m21 = l.y1;
        m.m22 = l.y2;
        float D = m2_det(m);
        float dx = l.x2 - l.x1;
        float dy = l.y2 - l.y1;
        float dr2 = sqrtf(dx * dx + dy * dy);
        dr2 *= dr2;
        float discr = c.r * c.r * dr2 - D * D;
        if (discr < 0) return 0;
        float dr2inv = 1.0f / dr2;
        float sq = sqrtf(discr);
        float x, y, k1, k2;
        x = (D * dy + SIGN(dy) * dx * sq) * dr2inv;
        y = (-D * dx + ABS(dy) * sq) * dr2inv;
        k1 = (x - l.x1) / (l.x2 - l.x1);
        k2 = (y - l.y1) / (l.y2 - l.y1);
        if (!l.inf1 && (k1 < 0 || k2 < 0)) return 0;
        if (!l.inf2 && (k1 > 1 || k2 > 1)) return 0;
        outa->x = x + c.x;
        outa->y = y + c.y;
        if (ABS(discr) <= 0.01f) return 1;
        x = (D * dy - SIGN(dy) * dx * sq) * dr2inv;
        y = (-D * dx - ABS(dy) * sq) * dr2inv;
        k1 = (x - l.x1) / (l.x2 - l.x1);
        k2 = (y - l.y1) / (l.y2 - l.y1);
        if (!l.inf1 && (k1 < 0 || k2 < 0)) return 1;
        if (!l.inf2 && (k1 > 1 || k2 > 1)) return 1;
        outb->x = x + c.x;
        outb->y = y + c.y;
        return 2;
}

int intersection_circ_circ(circ c1, circ c2, v2 *outa, v2 *outb)
{
        float dx = c1.x - c2.x;
        float dy = c1.y - c2.y;
        float d = sqrtf(dx * dx + dy * dy);
        if (d > c1.r + c2.r || d < ABS(c1.r - c2.r) ||
            (d == 0 && c1.r == c2.r))
                return 0;
        float r02 = c1.r * c1.r;
        float r12 = c2.r * c2.r;
        float a = (r02 - r12 + d * d) / (2 * d);
        float h = sqrtf(r02 - a * a);
        float px = c1.x + a * (c2.x - c1.x) / d;
        float py = c1.y + a * (c2.y - c1.y) / d;
        outa->x = px + h * (c2.y - c1.y) / d;
        outa->y = py - h * (c2.x - c1.x) / d;
        if (ABS(c1.r + c2.r - d) < 0.01f) return 1;
        outb->x = px - h * (c2.y - c1.y) / d;
        outb->y = py + h * (c2.x - c1.x) / d;
        return 2;
}

bool intersection_line_line(line a, line b, v2 *out)
{
        float x13 = a.x1 - b.x1;
        float y13 = a.y1 - b.y1;
        float x34 = b.x1 - b.x2;
        float y34 = b.y1 - b.y2;
        float x12 = a.x1 - a.x2;
        float y12 = a.y1 - a.y2;
        float c = 1.0f / (x12 * y34 - y12 * x34);
        float t = (x13 * y34 - y13 * x34) * c;
        float s = (x13 * y12 - y13 * x12) * c;
        if (!a.inf1 && t < 0) return false;
        if (!b.inf1 && s < 0) return false;
        if (!a.inf2 && t > 1) return false;
        if (!b.inf2 && s > 1) return false;
        out->x = a.x1 + t * (a.x2 - a.x1);
        out->y = a.y1 + t * (a.y2 - a.y1);
        return true;
}

m2 m2_I()
{
        m2 r;
        r.m11 = r.m22 = 1;
        r.m12 = r.m21 = 0;
        return r;
}

m2 m2_add(m2 a, m2 b)
{
        m2 r;
        r.m11 = a.m11 + b.m11;
        r.m12 = a.m12 + b.m12;
        r.m21 = a.m21 + b.m21;
        r.m22 = a.m22 + b.m22;
        return r;
}

m2 m2_sub(m2 a, m2 b)
{
        m2 r;
        r.m11 = a.m11 - b.m11;
        r.m12 = a.m12 - b.m12;
        r.m21 = a.m21 - b.m21;
        r.m22 = a.m22 - b.m22;
        return r;
}

m2 m2_mul(m2 a, m2 b)
{
        m2 r;
        r.m11 = a.m11 * b.m11 + a.m12 * b.m21;
        r.m12 = a.m11 * b.m12 + a.m12 * b.m22;
        r.m21 = a.m21 * b.m11 + a.m22 * b.m21;
        r.m22 = a.m21 * b.m12 + a.m22 * b.m22;
        return r;
}

m2 m2_scl(m2 a, float s)
{
        m2 r;
        r.m11 = a.m11 * s;
        r.m12 = a.m12 * s;
        r.m21 = a.m21 * s;
        r.m22 = a.m22 * s;
        return r;
}

m2 m2_inv(m2 a)
{
        m2 r;
        r.m11 = a.m22;
        r.m12 = -a.m12;
        r.m21 = -a.m21;
        r.m22 = a.m11;
        return m2_scl(r, 1.0 / m2_det(r));
}

m2 m2_aff_rot(float a)
{
        m2 r;
        float s = sinf(a);
        float c = cosf(a);
        r.m11 = c;
        r.m12 = -s;
        r.m21 = s;
        r.m22 = c;
        return r;
}

m2 m2_aff_scl(float x, float y)
{
        m2 r;
        r.m11 = x;
        r.m22 = y;
        r.m12 = r.m21 = 0;
        return r;
}

m2 m2_aff_shr(float x, float y)
{
        m2 r;
        r.m11 = r.m22 = 1;
        r.m21 = y;
        r.m12 = x;
        return r;
}

float m2_det(m2 a)
{
        return a.m11 * a.m22 - a.m21 * a.m12;
}