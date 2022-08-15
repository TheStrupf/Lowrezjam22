#include "game.h"
#include "engine.h"
#include "gameplay.h"
#include <stdbool.h>

#define MENU_OPTION_COUNT 2
#define FADE_SPEED 16
#define FADE_MAX 255

struct texturecollection textures;
struct soundcollection sounds;
struct mousecursor mousecursor;

static int gameoverticks = 0;
static bool gameover = false;
static int gamestate = STATE_MENU;
static int gamepaused = 0;
static int justpaused = 0;
static int fadetimer;
static int fadedir;
static func onfade;

static GUIbutton menu_play_btn;
static GUIbutton menu_quit_btn;
static GUIbutton pause_resume_btn;
static GUIbutton pause_menu_btn;
static GUIbutton pause_quit_btn;
static GUIbutton mute_btn;
static GUIbutton mute_mus_btn;
static bool muted = false;
static bool mutedmus = false;

static void launch_game()
{
        gamestate = STATE_GAMEPLAY;
        gamepaused = 0;
        justpaused = 0;
        gameover = false;
        gameplay_launch();
}

static void back_to_title()
{
        gamestate = STATE_MENU;
        intro_launch();
}

static void quit_game() { gamestate = STATE_QUIT; }

static void pause_game()
{
        if (!gameover) {
                if (gamepaused) gamepaused = 0;
                else justpaused = 1;
        }
}

void game_finished()
{
        pause_game();
        gameoverticks = 0;
        gameover = true;
}

void game_init()
{
        struct generic_obj {
                union {
                        waggon w;
                        train t;
                        rail r;
                        gametext txt;
                };
        };
        world.objpool = mempool_create(sizeof(struct generic_obj), 4096);
        world.nodes = MALLOC(sizeof(railnode) * MAXWIDTH * MAXHEIGHT);
        world.tiles = MALLOC(sizeof(tile) * MAXWIDTH * MAXHEIGHT);
        world.tilesbig = MALLOC(sizeof(tile) * MAXWIDTH * MAXHEIGHT);
        world.railtiles = MALLOC(sizeof(tile) * MAXWIDTH * MAXHEIGHT);

        textures.screen = gfx_screen_tex();
        textures.tileset = gfx_tex_load("assets/gfx/tiles.json");
        textures.rails = gfx_tex_load("assets/gfx/rails.json");
        textures.temp = gfx_tex_create(64, 64);
        textures.smoke = gfx_tex_create(64, 64);
        textures.minimap = gfx_tex_create(64, 64);
        textures.waggonsoutline = gfx_tex_create(256, 64);
        textures.savedscreen = gfx_tex_create(64, 64);

        sounds.bump = aud_load("assets/snd/bump.wav");
        sounds.train = aud_load("assets/snd/train.wav");
        sounds.bumphard = aud_load("assets/snd/bumphard.wav");
        sounds.questcompleted = aud_load("assets/snd/questcompleted.wav");
        sounds.newquest = aud_load("assets/snd/newquest.wav");
        sounds.decouple = aud_load("assets/snd/decouple.wav");
        sounds.blip = aud_load("assets/snd/blip.wav");
        sounds.tssh = aud_load("assets/snd/tssh.wav");
        sounds.refuel = aud_load("assets/snd/powerup.wav");
        sounds.driving = aud_load("assets/snd/drivingmode.wav");
        sounds.choo = aud_load("assets/snd/choo.wav");

        world.trains = arr_new(sizeof(train *));
        world.rails = arr_new_sized(512, sizeof(rail *));
        world.waggons = arr_new_sized(128, sizeof(waggon *));
        world.smokeparticles = arr_new_sized(1024, sizeof(particle));
        world.particles = arr_new_sized(512, sizeof(particle));
        world.questrails = arr_new(sizeof(rail *));
        world.texts = arr_new_sized(8, sizeof(gametext *));
        render_init();

        int btny = 36;
        const int YINCR = 10;
        const int TITLEX = 4;

        menu_play_btn.x = TITLEX;
        menu_play_btn.y = btny;
        menu_play_btn.buttonstate = BUTTON_NONE;
        menu_play_btn.h = 6;
        menu_play_btn.w = render_text_width("PLAY") + 1;
        menu_play_btn.click = sounds.blip;
        menu_play_btn.clickvol = 1.0f;
        btny += YINCR;

        menu_quit_btn.x = TITLEX;
        menu_quit_btn.y = btny;
        menu_quit_btn.buttonstate = BUTTON_NONE;
        menu_quit_btn.h = 6;
        menu_quit_btn.w = render_text_width("QUIT") + 1;
        menu_quit_btn.click = sounds.blip;
        menu_quit_btn.clickvol = 1.0f;
        btny += YINCR;

        // pause menu buttons
        btny = 10;

        pause_resume_btn.x = 32 - render_text_width("RESUME") / 2;
        pause_resume_btn.y = btny;
        pause_resume_btn.w = render_text_width("RESUME") + 1;
        pause_resume_btn.h = 6;
        pause_resume_btn.click = sounds.blip;
        pause_resume_btn.clickvol = 1.0f;
        btny += YINCR;

        pause_menu_btn.x = 32 - render_text_width("TITLE") / 2;
        pause_menu_btn.y = btny;
        pause_menu_btn.w = render_text_width("TITLE") + 1;
        pause_menu_btn.h = 6;
        pause_menu_btn.click = sounds.blip;
        pause_menu_btn.clickvol = 1.0f;
        btny += YINCR;

        pause_quit_btn.x = 32 - render_text_width("QUIT") / 2;
        pause_quit_btn.y = btny;
        pause_quit_btn.w = render_text_width("QUIT") + 1;
        pause_quit_btn.h = 6;
        pause_quit_btn.click = sounds.blip;
        pause_quit_btn.clickvol = 1.0f;
        btny += YINCR;

        mute_btn.x = 64 - 12;
        mute_btn.y = 64 - 12;
        mute_btn.w = 12;
        mute_btn.h = 12;
        mute_btn.click = sounds.blip;
        mute_btn.clickvol = 1.0f;

        mute_mus_btn.x = 64 - 24;
        mute_mus_btn.y = 64 - 12;
        mute_mus_btn.w = 12;
        mute_mus_btn.h = 12;
        mute_mus_btn.click = sounds.blip;
        mute_mus_btn.clickvol = 1.0f;
        intro_launch();
        aud_play_mus("assets/snd/music.mp3");
}

int game_tick()
{
        if (gamestate == STATE_QUIT) return 0;
        if (!fadetimer) {
                switch (gamestate) {
                case STATE_MENU:
                        gameplay_update();
                        gui_button_update(&menu_play_btn);
                        gui_button_update(&menu_quit_btn);
                        gui_button_update(&mute_btn);
                        gui_button_update(&mute_mus_btn);

                        if (menu_play_btn.buttonstate == BUTTON_PRESSED)
                                game_fade(launch_game);
                        if (menu_quit_btn.buttonstate == BUTTON_PRESSED)
                                game_fade(quit_game);
                        if (mute_btn.buttonstate == BUTTON_PRESSED) {
                                if (muted) aud_set_volume(1.0f);
                                else aud_set_volume(0.0f);
                                muted = !muted;
                        }
                        if (mute_mus_btn.buttonstate == BUTTON_PRESSED) {
                                if (mutedmus) aud_set_mus_volume(1.0f);
                                else aud_set_mus_volume(0.0f);
                                mutedmus = !muted;
                        }
                        if (vm_currenttick() % (60 * 10) == 0 || vm_currenttick() == 45) aud_play_sound(sounds.choo);
                        break;
                case STATE_GAMEPLAY:
                        if (inp_just_pressed(BTN_ESC)) pause_game();

                        if (gamepaused) {
                                gui_button_update(&pause_menu_btn);
                                gui_button_update(&pause_quit_btn);
                                gui_button_update(&mute_btn);
                                gui_button_update(&mute_mus_btn);
                                if (!gameover) {
                                        gui_button_update(&pause_resume_btn);
                                        if (pause_resume_btn.buttonstate == BUTTON_PRESSED)
                                                gamepaused = 0;
                                } else gameoverticks++;
                                if (pause_menu_btn.buttonstate == BUTTON_PRESSED)
                                        game_fade(back_to_title);
                                if (pause_quit_btn.buttonstate == BUTTON_PRESSED)
                                        game_fade(quit_game);
                                if (mute_btn.buttonstate == BUTTON_PRESSED) {
                                        if (muted) aud_set_volume(1.0f);
                                        else aud_set_volume(0.0f);
                                        muted = !muted;
                                }
                                if (mute_mus_btn.buttonstate == BUTTON_PRESSED) {
                                        if (mutedmus) aud_set_mus_volume(1.0f);
                                        else aud_set_mus_volume(0.0f);
                                        mutedmus = !muted;
                                }
                        } else {
                                gameplay_update();
                        }
                        break;
                }
        } else {
                fadetimer -= FADE_SPEED;
                if (fadetimer <= 0) {
                        if (fadedir == 1) {
                                if (onfade) onfade();
                                fadetimer = FADE_MAX;
                                fadedir = -1;
                        } else fadetimer = 0;
                }
        }
        return 1;
}

void game_fade(func onfadeaction)
{
        onfade = onfadeaction;
        fadetimer = FADE_MAX;
        fadedir = 1;
}

static void menu_text(const char *t, GUIbutton b)
{
        int textcol = COL_WHITE;
        int outline = COL_BLACK;

        if (b.buttonstate == BUTTON_HOVERED) {
                textcol = COL_BLACK;
                outline = COL_WHITE;
        }
        text_outlined(t, b.x, b.y, textcol, outline);
}

static void tex_darken(int alpha)
{
        for (int n = 0; n < 4096; n++)
                textures.screen.px[n] =
                    gfx_closest_col_merged_rgb(textures.screen.px[n],
                                               0, 0, 0, alpha);
}

void game_draw()
{
        gfx_clear(COL_LIGHT_PEACH);
        mousecursor.texregion = (rec){42, 192, 18, 12};
        mousecursor.offx = -10;
        mousecursor.offy = -3;
        switch (gamestate) {
        case STATE_MENU:
                render_draw();
                gfx_set_translation(0, 0);
                menu_text("PLAY", menu_play_btn);
                menu_text("QUIT", menu_quit_btn);

                int titley = vm_currenttick() % 60 < 30 ? 1 : 0;
                gfx_sprite(textures.tileset, 3, -4 + titley, (rec){144, 210, 48, 24}, 0);
                render_texts("LOWREZJAM", 4, 21 + titley, COL_BLACK);
                gfx_sprite(textures.tileset, mute_btn.x, mute_btn.y, (rec){222, 222 + (muted ? 0 : 12), 12, 12}, 0);
                gfx_sprite(textures.tileset, mute_mus_btn.x, mute_mus_btn.y, (rec){210, 222 + (mutedmus ? 0 : 12), 12, 12}, 0);
                break;
        case STATE_GAMEPLAY:
                if (!gamepaused) render_draw();
                gfx_set_translation(0, 0);
                if (justpaused) { // when paused: "screenshot" current frame and save that
                        gamepaused = 1;
                        justpaused = 0;
                        tex temp = gfx_screen_tex();
                        gfx_draw_to(textures.savedscreen);
                        gfx_sprite(temp, 0, 0, (rec){0, 0, 64, 64}, 0);
                        gfx_draw_to(temp);
                }
                if (gamepaused) {
                        gfx_sprite(textures.savedscreen, 0, 0, (rec){0, 0, 64, 64}, 0);
                        if (gameover) {
                                tex_darken(MIN(gameoverticks, 64));
                                text_outlined("COMPLETED!", 32 - render_text_width("COMPLETED!") / 2, 10, COL_GREEN, COL_BLACK);
                        } else {
                                tex_darken(64);
                                menu_text("RESUME", pause_resume_btn);
                        }

                        menu_text("TITLE", pause_menu_btn);
                        menu_text("QUIT", pause_quit_btn);
                        gfx_sprite(textures.tileset, mute_btn.x, mute_btn.y, (rec){222, 222 + (muted ? 0 : 12), 12, 12}, 0);
                        gfx_sprite(textures.tileset, mute_mus_btn.x, mute_mus_btn.y, (rec){210, 222 + (mutedmus ? 0 : 12), 12, 12}, 0);
                }
                break;
        }

        if (fadetimer) {
                int alphaadd = fadedir == 1 ? 255 : 0;
                tex_darken(alphaadd - fadedir * fadetimer);
        }
        gfx_set_translation(0, 0);
        gfx_sprite(textures.tileset, inp_mousex() + mousecursor.offx,
                   inp_mousey() + mousecursor.offy, mousecursor.texregion, 0);
}

void gui_button_update(GUIbutton *b)
{
        bool hovered = b->x <= inp_mousex() && inp_mousex() < (b->x + b->w) &&
                       b->y <= inp_mousey() && inp_mousey() < (b->y + b->h);
        if (hovered) {
                b->buttonstate = inp_just_pressed(BTN_MOUSE_LEFT) ? BUTTON_PRESSED : BUTTON_HOVERED;
        } else b->buttonstate = BUTTON_NONE;
        if (b->buttonstate == BUTTON_PRESSED) aud_play_sound_ext(b->click, 1.0f, b->clickvol);
}