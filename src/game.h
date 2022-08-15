#ifndef GAME_H
#define GAME_H

#include "engine.h"

#define MAXWIDTH 64
#define MAXHEIGHT 64

struct texturecollection {
        tex screen;
        tex tileset;
        tex rails;
        tex temp;
        tex smoke;
        tex minimap;
        tex waggonsoutline;
        tex savedscreen;
};

struct soundcollection {
        snd bump;
        snd train;
        snd bumphard;
        snd questcompleted;
        snd newquest;
        snd decouple;
        snd blip;
        snd tssh;
        snd driving;
        snd refuel;
        snd choo;
};

enum gamestate {
        STATE_MENU,
        STATE_GAMEPLAY,
        STATE_QUIT,
        STATE_FINISHED,
};

enum buttonstate {
        BUTTON_NONE,
        BUTTON_HOVERED,
        BUTTON_PRESSED,
};

typedef struct GUIbutton {
        int x;
        int y;
        int w;
        int h;
        int buttonstate;
        snd click;
        float clickvol;
} GUIbutton;

struct mousecursor {
        rec texregion;
        int offx;
        int offy;
};

extern struct texturecollection textures;
extern struct soundcollection sounds;
extern struct mousecursor mousecursor;

void game_init();
int game_tick();
void game_draw();
void game_fade(func);
void game_finished();

void render_init();
void render_draw();
int render_text_width(const char *);
void render_text(const char *, int x, int y, int col);
int render_texts_width(const char *);
void render_texts(const char *, int x, int y, int col);
void gui_button_update(GUIbutton *);
void text_outlined(const char *t, int x, int y, int colinner, int colouter);
void texts_outlined(const char *t, int x, int y, int colinner, int colouter);

#endif