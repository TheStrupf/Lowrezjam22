#include "engine.h"
#include "game.h"
#include "gameplay.h"
#include "rail.h"
#include "train.h"
#include <math.h>

static const char shadowpal[32] = {0x00, 0x11, 0x15, 0x13, 0x14, 0x1d, 0x0d, 0x06, 0x18, 0x19, 0x09, 0x1b, 0x0d, 0x1d, 0x1f, 0x1f, 0x00, 0x00, 0x10, 0x01, 0x12, 0x12, 0x05, 0x0f, 0x02, 0x04, 0x1b, 0x03, 0x13, 0x02, 0x19, 0x1e};
static const char smokeypal[32] = {0x00, 0x00, 0x00, 0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x11, 0x15, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x11, 0x15, 0x11, 0x11, 0x15, 0x11, 0x11, 0x11, 0x11, 0x11};
static int offx;
static int offy;

#define PAL_STD 0
#define PAL_SELECTED_WAGGON 1
#define PAL_TEXT 2
#define PAL_PLAYER_LOC 3

static int cursorID;

void render_init()
{
        gfx_clear_tex(textures.waggonsoutline, 0xFF);
        gfx_draw_to(textures.waggonsoutline);
        for (int n = 0; n < 32; n++)
                gfx_pal_set(n, COL_BLACK);
        gfx_sprite(textures.tileset, 0, 0, (rec){0, 0, 256, 36}, 0);
        gfx_apply_outline(COL_BLACK);
        gfx_pal_reset();

        gfx_draw_to(textures.screen);
        gfx_pal_select(PAL_SELECTED_WAGGON);
        for (int n = 0; n < 32; n++)
                gfx_pal_set(n, COL_WHITE);

        gfx_pal_select(PAL_PLAYER_LOC);
        gfx_pal_reset();
        gfx_pal_set(8, 11);
        gfx_pal_set(24, 3);
        gfx_pal_set(2, 19);
        gfx_pal_select(PAL_STD);
}

static void train_switch_drawing()
{
        // calculate graphical representation for arrows
        if (!world.prd.crossing) return;
        const int ANIMFREQ = 40;
        rec spriterec = (rec){102, 198, 18, 18};
        if (vm_currenttick() % ANIMFREQ < ANIMFREQ / 2)
                spriterec.y += 18;
        int flags = 0;
        int targetx = (world.prd.crossing->x * 6) - 7;
        int targety = (world.prd.crossing->y * 6) - 6;
        int movedir_towards_crossing = dir_rail_at(world.prd.willcomefrom, world.prd.crossing);
        int movingto = dir_rail_at(world.prd.willgoto,
                                   world.prd.crossing == world.prd.willgoto->from ? world.prd.willgoto->to : world.prd.willgoto->from);

        // align sprite with middle of crossing -> extend coming from direction by one tile
        switch (movedir_towards_crossing) {
        case DIR_UP:
                targety -= 6;
                break;
        case DIR_DO:
                targety += 6;
                break;
        case DIR_LE:
                targetx -= 6;
                break;
        case DIR_RI:
                targetx += 6;
                break;
        }

        // arrow rotation flags - flip x, y, diagonal
        if (movingto == movedir_towards_crossing) { // straight
                switch (movedir_towards_crossing) {
                case DIR_DO: flags = B_0010; break;
                case DIR_UP: flags = B_0000; break;
                case DIR_LE: flags = B_0100; break;
                case DIR_RI: flags = B_0101; break;
                }
        } else { // curve
                spriterec.x += 18;
                switch (movedir_towards_crossing) {
                case DIR_DO:
                        if (movingto == DIR_LE) flags = B_0010;
                        else if (movingto == DIR_RI) flags = B_0011;
                        break;
                case DIR_UP:
                        if (movingto == DIR_LE) flags = B_0000;
                        else if (movingto == DIR_RI) flags = B_0001;
                        break;
                case DIR_LE:
                        if (movingto == DIR_UP) flags = B_0100;
                        else if (movingto == DIR_DO) flags = B_0110;
                        break;
                case DIR_RI:
                        if (movingto == DIR_UP) flags = B_0101;
                        else if (movingto == DIR_DO) flags = B_0111;
                        break;
                }
        }
        gfx_sprite(textures.tileset, targetx, targety - 1, spriterec, flags);
}

static void draw_waggons()
{
        FOREACHI(world.waggons, n)
        {
                waggon *w = world.waggons[n];
                rec drawr = (rec){0, 0, 12, 12};
                int dflags = 0;
                rail *cr = w->r;
                v2 wpos = rail_to_world(cr, w->pos_q8, NULL, NULL);

                switch (cr->type) {
                case RAIL_VER:
                        if ((cr->from->y < cr->to->y && w->facing == -1) ||
                            (cr->from->y > cr->to->y && w->facing == 1))
                                drawr.y = 24;
                        break;
                case RAIL_HOR:
                        if ((cr->from->x < cr->to->x && w->facing == -1) ||
                            (cr->from->x > cr->to->x && w->facing == 1))
                                dflags = B_0001;
                        drawr.y = 12;
                        break;
                case RAIL_Q_1:
                        drawr.x = 12;
                        if ((cr->from->x < cr->to->x && w->facing == -1) ||
                            (cr->from->x > cr->to->x && w->facing == 1))
                                drawr.y = 12;
                        break;
                case RAIL_Q_2:
                        drawr.x = 12;
                        if ((cr->from->x < cr->to->x && w->facing == 1) ||
                            (cr->from->x > cr->to->x && w->facing == -1))
                                drawr.y = 12;
                        break;
                case RAIL_Q_3:
                        drawr.x = 12;
                        if ((cr->from->x < cr->to->x && w->facing == -1) ||
                            (cr->from->x > cr->to->x && w->facing == 1))
                                drawr.y = 12;
                        break;
                case RAIL_Q_4:
                        drawr.x = 12;
                        if ((cr->from->x < cr->to->x && w->facing == 1) ||
                            (cr->from->x > cr->to->x && w->facing == -1))
                                drawr.y = 12;
                        break;
                }
                if (cr->type == RAIL_Q_1 || cr->type == RAIL_Q_3)
                        dflags = B_0001;

                drawr.x += w->type * 24;

                int drawx = (int)wpos.x;
                int drawy = (int)wpos.y;
                if (w->type == WTYPE_LOC) {
                        int smokerng; // the lower the more
                        int vabs = ABS(w->parent->vel_q8);
                        if (vabs > 4096) smokerng = 3;
                        else if (vabs > 2048) smokerng = 4;
                        else if (vabs > 1024) smokerng = 8;
                        else smokerng = 32;

                        // additional smoke if accelerating
                        if (vabs > ABS(w->parent->vel_prev))
                                smokerng--;

                        if (rng(smokerng) == 0) create_smokeparticle(drawx, drawy);
                }

                int pal = 0;
                if (w->parent == world.playertrain && (w->type == WTYPE_LOC || w->type == WTYPE_COAL))
                        pal = PAL_PLAYER_LOC;

                drawx += w->bumpx;
                drawy += w->bumpy;
                if (vm_currenttick() % 4 == 0) {
                        w->bumpx = 0;
                        w->bumpy = 0;
                        if (ABS(w->parent->vel_q8) > 256 && rng(48) == 0) {
                                if (w->r->type == RAIL_HOR) w->bumpy = rng_range(-1, 0);
                                if (w->r->type == RAIL_VER) w->bumpx = rng_range(-1, 1);
                        }
                }

                //  weird render priority stuff, don't ask
                gfx_sprite_sorted((int)wpos.y - 11, textures.waggonsoutline, drawx - 3, drawy - 7, drawr, dflags);
                gfx_sprite_sortedpal((int)wpos.y, textures.tileset, drawx - 3, drawy - 7, drawr, dflags, pal);
        }
}

static void draw_map(bool small)
{
        gfx_draw_to(textures.minimap);
        gfx_clear(COL_MAUVE);

        FOREACHI(world.rails, n)
        {
                rail *r = world.rails[n];
                gfx_line(r->from->x, r->from->y, r->to->x, r->to->y, COL_BLACK);
        }
        FOREACHI(world.questrails, n)
        {
                rail *r = world.questrails[n];
                railnode *n = r->from->num_rails == 1 ? r->from : r->to;
                if (small) gfx_px(n->x, n->y, COL_WHITE);
                else
                        gfx_sprite(textures.tileset, n->x - 3, n->y - 3,
                                   (rec){(r->sidetrackID - 1) * 6, 48, 6, 6}, 0);
        }
        FOREACHI(world.waggons, n)
        {
                waggon *w = world.waggons[n];
                int x = (w->r->from->x + (w->r->to->x - w->r->from->x) * w->pos_q8 / w->r->length_q8);
                int y = (w->r->from->y + (w->r->to->y - w->r->from->y) * w->pos_q8 / w->r->length_q8);
                int col;
                if (w->parent == world.playertrain) col = COL_GREEN;
                else if (w->questID > 0) col = COL_BLUE;
                else col = COL_RED;
                gfx_px(x, y, col);
        }

        gfx_set_translation(0, 0);
        gfx_draw_to(textures.screen);
        gfx_sprite(textures.minimap, 0, 0, (rec){0, 0, 64, 64}, 0);
}

static void render_update_cam()
{
        if (world.map != MAP_INTRO) {
                int vsign = 0;
                if (world.playertrain) {
                        waggon *playerpullingwaggon = loc_pulling_waggon(world.playertrain, &vsign);
                        if (playerpullingwaggon)
                                world.camerawaggon = playerpullingwaggon;
                }

                if (!world.camerawaggon)
                        world.camerawaggon = world.trains[0]->waggons[0];

                // calculate tangent to look forward
                v2 tangent;
                v2 targetcam = rail_to_world(world.camerawaggon->r, world.camerawaggon->pos_q8, &tangent, NULL);

                // take advantage of that we rigidly defined each rails from/to nodes
                // in the loading process...
                switch (world.camerawaggon->r->type) {
                case RAIL_HOR: tangent = v2_scl(tangent, vsign); break;
                case RAIL_VER: tangent = v2_scl(tangent, vsign); break;
                case RAIL_Q_1: tangent = v2_scl(tangent, vsign); break;
                case RAIL_Q_2: tangent = v2_scl(tangent, -vsign); break;
                case RAIL_Q_3: tangent = v2_scl(tangent, vsign); break;
                case RAIL_Q_4: tangent = v2_scl(tangent, -vsign); break;
                }
                tangent = v2_scl(tangent, 15);
                targetcam = v2_add(targetcam, tangent);

                // camera smoothing when switch watched waggons
                v2 diff = v2_sub(targetcam, world.lastcam);
                float targetlen = v2_len(diff);
                diff = v2_setlen(diff, MIN(targetlen, 2.0f));
                targetcam = v2_add(world.lastcam, diff);
                world.currcam = targetcam;
                world.cam = targetcam;
                world.lastcam = world.currcam;

                if (world.map != MAP_INTRO) {
                        // look in direction of mouse courser
                        world.cam.x += ((float)inp_mousex() - 32) * 0.5f;
                        world.cam.y += ((float)inp_mousey() - 32) * 0.5f;
                }

                if ((int)world.cam.x < 32) world.cam.x = 32;
                if ((int)world.cam.y < 32) world.cam.y = 32;
                if ((int)world.cam.x + 32 > world.width * 6) world.cam.x = world.width * 6 - 32;
                if ((int)world.cam.y + 32 > world.height * 6) world.cam.y = world.height * 6 - 32;
        }

        offx = (-(int)world.cam.x + 32);
        offy = (-(int)world.cam.y + 32);

        if (world.map != MAP_INTRO) {
                world.mouseworld.x = inp_mousex() - offx;
                world.mouseworld.y = inp_mousey() - offy;

                world.waggonselected1 = NULL;
                world.waggonselected2 = NULL;
                if (world.playertrain->vel_q8 == 0) {
                        float smallestdistance = 20;
                        for (int n = 0; n < arr_len(world.playertrain->waggons) - 1; n++) {
                                waggon *w1 = world.playertrain->waggons[n];
                                if (w1->type == WTYPE_LOC) continue;
                                waggon *w2 = world.playertrain->waggons[n + 1];

                                v2 between = worldpos_between(w1, w2);
                                float dist = v2_distance_sq(between, (v2){world.mouseworld.x, world.mouseworld.y});
                                if (dist < smallestdistance) {
                                        world.waggonselected1 = w1;
                                        world.waggonselected2 = w2;
                                        world.betweenselected = between;
                                        smallestdistance = dist;
                                }
                        }
                }
        }
}

void render_draw()
{
        render_update_cam();
        gfx_set_translation(offx, offy);
        int tx1 = MAX(((int)world.cam.x - 32) / 6 - 2, 0);
        int ty1 = MAX(((int)world.cam.y - 32) / 6 - 2, 0);
        int tx2 = MIN(((int)world.cam.x + 32) / 6 + 2, world.width);
        int ty2 = MIN(((int)world.cam.y + 32) / 6 + 2, world.height);

        rec tilerec = (rec){0, 0, 6, 6};
        for (int y = ty1; y < ty2; y++) {
                int cache = y * world.width;
                for (int x = tx1; x < tx2; x++) {
                        tile rn = world.tiles[x + cache];
                        tilerec.x = (rn.ID % 42) * 6;
                        tilerec.y = (rn.ID / 42) * 6;
                        gfx_sprite(textures.tileset, x * 6, y * 6, tilerec, 0);
                }
        }

        tilerec = (rec){0, 0, 12, 12};
        for (int y = ty1; y < ty2; y++) {
                int cache = y * world.width;
                for (int x = tx1; x < tx2; x++) {
                        tile rn = world.railtiles[x + cache];
                        if (rn.flags < 0xFF) {
                                tilerec.x = (rn.ID % 10) * 12;
                                tilerec.y = (rn.ID / 10) * 12;
                                gfx_sprite(textures.rails, x * 6 - 3, y * 6 - 3, tilerec, rn.flags);
                        }
                }
        }

        train_switch_drawing();

        gfx_clear_tex(textures.temp, 0xFF);
        gfx_clear_tex(textures.smoke, 0xFF);

        // gfx_set_translation(0, 0);
        rec particlerec = (rec){0, 66, 6, 6};
        FOREACHI(world.smokeparticles, n)
        {
                particle *p = &world.smokeparticles[n];
                particlerec.x = p->type;
                gfx_draw_to(textures.temp);
                gfx_sprite(textures.tileset, (int)p->x, (int)p->y, particlerec, (int)p->flags);
                gfx_draw_to(textures.smoke);
                gfx_sprite(textures.tileset, (int)p->x, (int)(p->y - p->z * 0.5f), particlerec, (int)p->flags);
        }

        for (int n = 0; n < 4096; n++) {
                if (textures.temp.px[n] != 0xFF)
                        textures.screen.px[n] = shadowpal[textures.screen.px[n]];
        }

        gfx_draw_to(textures.screen);
        draw_waggons();

        if (world.waggonselected1 && world.waggonselected2) {
                v2 wp1 = rail_to_world(world.waggonselected1->r, world.waggonselected1->pos_q8, NULL, NULL);
                v2 wp2 = rail_to_world(world.waggonselected2->r, world.waggonselected2->pos_q8, NULL, NULL);
                v2 wp = v2_sub(wp1, wp2);
                rec planesrc = (rec){66, 204, 12, 24};
                int flags = 0;
                if (ABS(wp.x) < 1) {
                        planesrc.x = 66;
                } else if (ABS(wp.y) < 1) {
                        planesrc.x = 90;
                } else {
                        planesrc.x = 78;
                        if (wp1.y > wp2.y && wp1.x > wp2.x)
                                flags = 1; // mirror x
                }

                gfx_sprite_sorted((int)world.betweenselected.y + 4, textures.tileset,
                                  (int)world.betweenselected.x - 5, (int)world.betweenselected.y - 9,
                                  planesrc, flags);
        }

        tilerec = (rec){0, 0, 6, 12};
        for (int y = ty1; y < ty2; y++) {
                int cache = y * world.width;
                for (int x = tx1; x < tx2; x++) {
                        tile rn = world.tilesbig[x + cache];
                        if (rn.flags != 0xFF) {
                                tilerec.x = (rn.ID % 42) * 6;
                                tilerec.y = (rn.ID / 42) * 12;
                                gfx_sprite_sorted(y * 6 - 6, textures.tileset, x * 6, y * 6 - 6, tilerec, rn.flags);
                        }
                }
        }

        FOREACHI(world.particles, n)
        {
                particle *p = &world.particles[n];
                gfx_px_sorted(p->y, p->x, p->y - p->z * 0.5f, (uchar)p->type);
        }

        gfx_sorted_flush();

        FOREACHI(world.questrails, n)
        {
                rail *r = world.questrails[n];
                railnode *n = r->from->num_rails == 1 ? r->from : r->to;
                int hovery = (int)(sinf(vm_currenttick() * 0.05f) * 2.5f) - 12;
                gfx_sprite(textures.tileset, n->x * 6 - 3, n->y * 6 + hovery,
                           (rec){(r->sidetrackID - 1) * 12, 36, 12, 12}, 0);
        }

        FOREACHI(world.texts, n)
        {
                gametext *gt = world.texts[n];
                int offy = vm_currenttick() % 60 < 30 ? 1 : 0;
                render_texts(gt->text, gt->x, gt->y + offy, COL_BLACK);
        }

        for (int n = 0; n < 4096; n++) {
                if (textures.smoke.px[n] != 0xFF)
                        textures.screen.px[n] = smokeypal[textures.screen.px[n]];
        }

        gfx_set_translation(0, 0);

        if (world.map != MAP_INTRO) {
                char waggonsleft[4];
                if (world.questwaggons >= 10) {
                        waggonsleft[0] = '0' + (world.questwaggons / 10);
                        waggonsleft[1] = '0' + (world.questwaggons % 10);
                        waggonsleft[2] = '\0';
                } else {
                        waggonsleft[0] = '0' + world.questwaggons;
                        waggonsleft[1] = '\0';
                }

                int tw = render_texts_width(waggonsleft);
                texts_outlined(waggonsleft, 63 - tw, 2, COL_BLACK, COL_WHITE);
                if (world.upgradedticks) {
                        float upgraderatio = (float)world.upgradedticks / (float)UPGRADETICKS;
                        int col = vm_currenttick() % 40 < 20 ? COL_RED : COL_DARK_RED;
                        gfx_rec_filled(0, 63, (int)((float)64 * upgraderatio), 1, col);
                }

                if (world.waggonselected1 && world.waggonselected2) {
                        cursorID = 1;
                        const int ANIMFREQ = 60;
                        if (vm_currenttick() % ANIMFREQ < ANIMFREQ / 2)
                                cursorID = 2;
                        mousecursor.texregion = (rec){42, 192 + cursorID * 12, 18, 12};
                }

                if (inp_pressed(BTN_SELECT))
                        draw_map(false);

                gfx_set_translation(0, 0);
                // indicators to destinations on screen edge
                FOREACHI(world.questrails, n)
                {
                        rail *r = world.questrails[n];
                        railnode *node = (r->from->num_rails == 1 ? r->from : r->to);
                        v2 npos = (v2){node->x * 6, node->y * 6};
                        v2 cpos = (v2){32 - offx, 32 - offy};
                        v2 toquest = v2_sub(npos, cpos);
                        const int BORDER = 28;
                        const int THRESH = 36;
                        if (-THRESH <= toquest.x && toquest.x < THRESH &&
                            -THRESH < toquest.y && toquest.y < THRESH)
                                continue;
                        rec indicatorspr = (rec){(r->sidetrackID - 1) * 6, 48, 6, 6};
                        float edgex = 0;
                        float edgey = 0;
                        if (toquest.y < 0) {
                                edgey = -BORDER;
                                edgex = toquest.x * edgey / toquest.y;
                                if (edgex >= -BORDER && edgex < BORDER) goto INDICATOR;
                        }
                        if (toquest.y > 0) {
                                edgey = BORDER;
                                edgex = toquest.x * edgey / toquest.y;
                                if (edgex >= -BORDER && edgex < BORDER) goto INDICATOR;
                        }
                        if (toquest.x < 0) {
                                edgex = -BORDER;
                                edgey = toquest.y * edgex / toquest.x;
                                if (edgey >= -BORDER && edgey < BORDER) goto INDICATOR;
                        }
                        if (toquest.x > 0) {
                                edgex = BORDER;
                                edgey = toquest.y * edgex / toquest.x;
                                if (edgey >= -BORDER && edgey < BORDER) goto INDICATOR;
                        }
                INDICATOR:
                        gfx_sprite(textures.tileset, (int)edgex + 32 - 3, (int)edgey + 32 - 3, indicatorspr, 0);
                }

                gfx_sprite(textures.tileset, 64 - 6, 30,
                           (rec){0, 192, 6, 24}, 0);
                gfx_sprite(textures.tileset, 64 - 6, 30 + world.drivingmode * 6,
                           (rec){6, 192 + world.drivingmode * 6, 6, 6}, 0);
        }
}

static int charwidths(char c)
{
        int xi = 4;
        switch (c) {
        case 'Q':
        case 'W':
        case 'O':
        case 'N':
        case 'M':
        case 'G':
                xi = 5;
                break;
        case ' ':
        case '!':
        case '?':
        case '.':
        case ',':
        case ':':
        case '\'':
        case '(':
        case ')':
        case '[':
        case ']':
        case '/':
        case '\\':
        case 'Z':
        case 'S':
                xi = 3;
                break;
        }
        return xi;
}

static int charwidth(char c)
{
        int xi = 5;
        switch (c) {
        case 'M':
        case 'W':
                xi = 6;
                break;
        case '\'':
                xi = 3;
                break;
        case ' ':
        case '+':
        case '-':
        case ':':
        case ';':
        case '.':
        case ',':
        case '[':
        case ']':
        case '(':
        case ')':
        case '\"':
        case '!':
        case 'E':
        case 'F':
        case 'I':
        case 'J':
        case 'L':
        case 'T':
                xi = 4;
                break;
        }
        return xi;
}

static char to_upper(char c) { return ('a' <= c && c <= 'z') ? c - 32 : c; }

static int render_text_width_(const char *t, int (*widthfunc)(char))
{
        int i = -1;
        int w = 0;
        while (t[++i] != '\0') {
                char c = to_upper(t[i]);
                w += widthfunc(c);
        }
        return w;
}

static void render_text_(const char *t, int x, int y, int col, int ty, int size, int (*widthfunc)(char))
{
        gfx_pal_select(PAL_TEXT);
        gfx_pal_set(0, col);
        rec r = (rec){0, 0, size, size};
        int i = -1;
        while (t[++i] != '\0') {
                char c = to_upper(t[i]);
                if (c >= ' ' && c < '@') {
                        r.x = ((int)c - 32) * size;
                        r.y = ty;
                } else if ('@' <= c && c <= '_') {
                        r.x = ((int)c - 64) * size;
                        r.y = ty + size;
                } else continue;
                gfx_sprite(textures.tileset, x, y, r, 0);
                x += widthfunc(c);
        }
        gfx_pal_select(PAL_STD);
}

static void text_outlined_(const char *t, int x, int y, int colinner, int colouter, void (*textfunc)())
{
        gfx_draw_to(textures.temp);
        gfx_clear(0xFF);
        textfunc(t, x, y, colinner);
        gfx_apply_outline(colouter);
        gfx_draw_to(textures.screen);
        gfx_sprite(textures.temp, 0, 0, (rec){0, 0, 64, 64}, 0);
}

int render_text_width(const char *t) { return render_text_width_(t, charwidth); }

int render_texts_width(const char *t) { return render_text_width_(t, charwidths); }

void render_text(const char *t, int x, int y, int col)
{
        render_text_(t, x, y, col, 234, 6, charwidth);
}

void render_texts(const char *t, int x, int y, int col)
{
        render_text_(t, x, y, col, 104, 4, charwidths);
}

void text_outlined(const char *t, int x, int y, int colinner, int colouter)
{
        text_outlined_(t, x, y, colinner, colouter, render_text);
}

void texts_outlined(const char *t, int x, int y, int colinner, int colouter)
{
        text_outlined_(t, x, y, colinner, colouter, render_texts);
}