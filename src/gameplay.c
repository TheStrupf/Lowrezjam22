#include "gameplay.h"
#include "engine.h"
#include "game.h"
#include "list.h"
#include "rail.h"
#include "train.h"
#include <math.h>
#include <string.h>

static GUIbutton fastbtn;
static GUIbutton normalbtn;
static GUIbutton haltbtn;
static GUIbutton reversebtn;
static int nextmapID;
gameworld world;
void load_map(const char *);

void next_map()
{
        gameplay_load_map(nextmapID);
}

static int questID_to_sprite(int questID)
{
        return questID + 1;
}

void spawn_quest_waggon(rail *r, int ID)
{
        if (arr_len(world.questrails) == 0) return;
        if (rail_occupied(r, NULL)) return;
        train *t = train_new();
        t->moved = 0;
        t->vel_q8 = 0;

        waggon *w = waggon_new();
        arr_push(t->waggons, &w);
        w->parent = t;
        w->trainfacing = 1;
        w->r = r;
        w->facing = 1;
        w->weight = 1;
        w->pos_q8 = 128;
        w->questID = ID;
        if (ID > 0) {
                w->type = questID_to_sprite(ID);
                world.questwaggons++;
        } else
                w->type = WTYPE_CIVIL;
}

bool check_quest_completion()
{
        FOREACHI(world.waggons, n)
        {
                waggon *w = world.waggons[n];
                if (w->questID && w->r->sidetrackID == w->questID) { // quest completed
                        train *t = w->parent;
                        int questID = w->r->sidetrackID;
                        int mod = (arr_find(t->waggons, &w) == arr_len(t->waggons) - 1); // 0 or 1, pop from front or back
                        do {
                                arr_del_at(t->waggons, (arr_len(t->waggons) - 1) * mod);
                                arr_del(world.waggons, &w);
                                mempool_free(world.objpool, w);
                                world.questwaggons--;

                                if (arr_len(t->waggons) > 0) w = t->waggons[(arr_len(t->waggons) - 1) * mod];
                                else {
                                        train_destroy(t);
                                        return true;
                                }
                        } while (w->questID == questID);
                        return true;
                }
        }
        return false;
}

void train_switch_logic(train *t)
{
        int vsign;
        waggon *first = loc_pulling_waggon(t, &vsign);
        if (!first) return;

        // find next crossing
        rail *railcomefrom = first->r;
        railnode *crossing = vsign > 0 ? railcomefrom->to : railcomefrom->from;
        int d = vsign > 0 ? railcomefrom->length_q8 - first->pos_q8 : first->pos_q8;
        while (d <= 0xFFFF) { // avoid spiral of death
                raillist l = available_rails(railcomefrom, crossing, true);
                if (l.n <= 0) {
                        world.prd.crossing = NULL;
                        world.prd.willcomefrom = NULL;
                        world.prd.willgoto = NULL;
                        return;
                }
                if (l.n >= 2) break;

                railcomefrom = l.rails[0];
                crossing = crossing == railcomefrom->from ? railcomefrom->to : railcomefrom->from;
                d += railcomefrom->length_q8;
        }

        // changed direction, need to reset target rail
        if (railcomefrom != world.prd.willcomefrom)
                world.prd.willgoto = NULL;

        world.prd.crossing = crossing;
        world.prd.willcomefrom = railcomefrom;
        raillist l = available_rails(railcomefrom, crossing, true);
        int movedir_towards_crossing = dir_rail_at(railcomefrom, crossing);

        // determine first chosen direction
        for (int n = 0; n < l.n; n++) {
                rail *r = l.rails[n];
                railnode *other = crossing == r->from ? r->to : r->from;
                int movingto = dir_rail_at(r, other);
                if (!world.prd.willgoto && movingto == movedir_towards_crossing) {
                        world.prd.willgoto = r;
                        break;
                }
                // if (movingto == aimingto) world.prd.willgoto = r;

                switch (movingto) {
                case DIR_DO:
                        if (inp_pressed(BTN_DOWN)) world.prd.willgoto = r;
                        break;
                case DIR_UP:
                        if (inp_pressed(BTN_UP)) world.prd.willgoto = r;
                        break;
                case DIR_LE:
                        if (inp_pressed(BTN_LEFT)) world.prd.willgoto = r;
                        break;
                case DIR_RI:
                        if (inp_pressed(BTN_RIGHT)) world.prd.willgoto = r;
                        break;
                }
        }

        if (!world.prd.willgoto) // just take a random one if nothing was provided
                world.prd.willgoto = l.rails[rng(l.n)];
}

train *spawntrain(rail *r, int dir, int n, int minlength)
{
        if (rail_occupied(r, NULL)) return NULL;
        int pos = 1;
        int poss = dir * 256;
        int facing = -dir;
        train *t = train_new();
        waggon *first = waggon_new();
        first->r = r;
        first->pos_q8 = pos;
        first->facing = facing;
        first->trainfacing = 1;
        first->parent = t;
        first->type = WTYPE_LOC;
        arr_push(t->waggons, &first);

        for (int i = 1; i < n; i++) {
                pos += poss;
                railnode *crossed = NULL;
                int remainder = 0;
                if (pos < 0) {
                        crossed = r->from;
                        remainder = -(pos + 1);
                }
                if (pos >= r->length_q8) {
                        crossed = r->to;
                        remainder = pos - r->length_q8;
                }

                if (crossed) {
                        raillist l = available_rails(r, crossed, false);

                        if (l.n == 0) goto RETURN_TRAIN;
                        rail *nr = l.rails[rng(l.n)];
                        if (rail_occupied(nr, t)) goto RETURN_TRAIN;
                        if (r->from == nr->from || r->to == nr->to) {
                                poss *= -1;
                                facing *= -1;
                        }
                        r = nr;
                        pos = crossed == r->from ? remainder : r->length_q8 - remainder - 1;
                }

                waggon *w = waggon_new();
                w->parent = t;
                w->facing = facing;
                if (arr_len(world.questrails) > 0) {
                        w->questID = (i == 1 ? 0 : rng_range(1, arr_len(world.questrails)));
                } else
                        w->questID = 0;

                w->type = (i == 1 ? WTYPE_COAL : questID_to_sprite(w->questID));
                w->pos_q8 = pos;
                w->r = r;
                waggon_add_to(t->waggons[i - 1], w);
        }
RETURN_TRAIN:

        if (arr_len(t->waggons) < minlength) {
                for (int n = arr_len(t->waggons) - 1; n >= 0; n--) {
                        arr_del(world.waggons, &(t->waggons[n]));
                        mempool_free(world.objpool, t->waggons[n]);
                }
                train_destroy(t);
                return NULL;
        }
        return t;
}

train *try_spawn_NPC_train(rail *r, int dir, int num)
{
        train *t = spawntrain(r, dir, num, num);
        if (t) {
                t->waggons[2]->type = WTYPE_CIVIL;
                t->waggons[2]->questID = 0;
                t->waggons[1]->type = WTYPE_COAL;
                t->waggons[1]->questID = 0;
                for (int i = 3; i < arr_len(t->waggons); i++) {
                        if (t->waggons[i]->questID >= 1) {
                                world.questwaggons++;
                        }
                }
                t->target_vel = 3000;
                return t;
        }
        return NULL;
}

train *try_spawn_player_train(rail *r, int dir)
{
        train *t = spawntrain(r, dir, 2, 2);
        world.playertrain = t;
        return t;
}

void update_particles()
{

        const float xyacc = 0.0065f;
        const float xymax = 0.7f;
        FOREACHI(world.smokeparticles, n)
        {
                particle *p = &world.smokeparticles[n];
                if (--p->ticks <= 0) {
                        arr_del_at(world.smokeparticles, n--);
                        continue;
                }
                if (p->ticks < 40) p->type = 4 * 6;
                else if (p->ticks < 80) p->type = 3 * 6;
                else if (p->ticks < 120) p->type = 2 * 6;
                else if (p->ticks < 160) p->type = 1 * 6;
                if (rng(8) == 0) p->flags = rng(8);
                p->velx += rngf_range(-xyacc, xyacc);
                p->vely += rngf_range(-xyacc, xyacc);
                p->velz += rngf_range(-0.008f, 0.0020f);
                p->velx = MAX(-xymax, MIN(p->velx, xymax));
                p->vely = MAX(-xymax, MIN(p->vely, xymax));
                p->x += p->velx;
                p->y += p->vely;
                p->z += p->velz;
        }

        FOREACHI(world.particles, n)
        {
                particle *p = &world.particles[n];
                if (--p->ticks <= 0) {
                        arr_del_at(world.particles, n--);
                        continue;
                }
                p->velx += p->accx;
                p->vely += p->accy;
                p->velz += p->accz;
                p->x += p->velx;
                p->y += p->vely;
                p->z += p->velz;
                if (p->z < 0) {
                        p->z = 0;
                        p->velz = -p->velz * 0.7f;
                }
        }
}

void accelerate_to_target_vel(train *t, int acc)
{
        if (t->vel_q8 > t->target_vel) {
                train_accelerate(t, -acc);
                if (t->vel_q8 < t->target_vel)
                        t->vel_q8 = t->target_vel;
        }
        if (t->vel_q8 < t->target_vel) {
                train_accelerate(t, acc);
                if (t->vel_q8 > t->target_vel)
                        t->vel_q8 = t->target_vel;
        }
}

void update_NPC_train(train *t)
{
        int acc = 0;

        if (get_loc(t)) {
                acc = 256;
        } else {
                t->target_vel = 0;
                acc = 32;
        }
        accelerate_to_target_vel(t, acc);
}

void update_player_train(train *t)
{
        int acc = world.upgradedticks ? 1024 : 192;
        accelerate_to_target_vel(t, acc);
        if (world.upgradedticks) world.upgradedticks--;

        if (t->vel_q8 == 0 && t->vel_prev != 0) {
                t->crashticks = 30;
                t->vel_q8 = 0;
                play_snd_camdistance(sounds.tssh, rail_to_world(get_loc(t)->r, get_loc(t)->pos_q8, NULL, NULL), 1.0f, 0.7f);
                waggon *playerloc = get_loc(t);
                v2 ccenter;
                v2 worldpos = rail_to_world(playerloc->r, playerloc->pos_q8, NULL, &ccenter);

                v2 toouter = v2_sub(worldpos, ccenter);
                if (playerloc->r->type == RAIL_HOR) {
                        toouter.x = 0;
                        toouter.y = 1;
                } else if (playerloc->r->type == RAIL_VER) {
                        toouter.x = 1;
                        toouter.y = 0;
                }
                worldpos.x += 3;

                toouter = v2_scl(toouter, 1.0f / v2_len(toouter));
                const float VEL = 0.1f;
                const float VEL_VAR = 0.1f;
                const float XY = 4.0f;
                for (int n = 0; n < 64; n++) {
                        particle *p = create_regularparticle();
                        int sig = (n & 1 ? 1 : -1);
                        p->x = worldpos.x + sig * toouter.x * XY;
                        p->y = worldpos.y + sig * toouter.y * XY;
                        p->velx = sig * toouter.x * VEL + rngf_range(-VEL_VAR, VEL_VAR);
                        p->vely = sig * toouter.y * VEL + rngf_range(-VEL_VAR, VEL_VAR);
                        p->x += rngf_range(-2.0f, 2.0f);
                        p->y += rngf_range(-2.0f, 2.0f);
                        p->z = 2;
                        p->velz = 0;
                        p->accx = p->accy = p->accz = 0;
                        p->type = COL_DARK_BLUE;
                        p->ticks = rng_range(30, 40);
                }
        }

        train_switch_logic(t);
        if (check_quest_completion()) {
                aud_play_sound_ext(sounds.questcompleted, 1.0f, 1.0f);
                t->vel_q8 = 0;
        }

        if (get_loc(t)->r->refuelstation) {
                world.upgradedticks = MIN(world.upgradedticks + UPGRADEREFUEL, UPGRADETICKS);
                if (vm_currenttick() % 40 == 0 && world.upgradedticks < UPGRADETICKS)
                        play_snd_camdistance(sounds.refuel, rail_to_world(get_loc(t)->r, get_loc(t)->pos_q8, NULL, NULL), 1.0f, 1.0f);
        }
}

void intro_launch()
{
        gameplay_load_map(MAP_INTRO);
}

void gameplay_launch()
{
        nextmapID = MAP_1;
        gameplay_load_map(MAP_1);
}

void gameplay_load_map(int map)
{
        switch (map) {
        case MAP_INTRO:
                load_map("assets/maps/intro.tmj");
                break;
        case MAP_1:
                load_map("assets/maps/Map1.tmj");
                break;
        case MAP_2:
                load_map("assets/maps/Map2.tmj");
                break;
        case MAP_3:
                load_map("assets/maps/Map3.tmj");
                break;
        case MAP_4:
                load_map("assets/maps/Map4.tmj");
                break;
        case MAP_5:
                load_map("assets/maps/Map5.tmj");
                break;
        case MAP_6:
                load_map("assets/maps/Map6.tmj");
                break;
        case MAP_7:
                load_map("assets/maps/Map7.tmj");
                break;
        }

        int BX = 64 - 6;
        int BW = 6;
        int BH = 6;
        int by = 30;
        fastbtn.x = BX;
        fastbtn.y = by;
        fastbtn.w = BW;
        fastbtn.h = BH;
        fastbtn.click = sounds.driving;
        fastbtn.clickvol = 0.7f;
        by += BH;

        normalbtn.x = BX;
        normalbtn.y = by;
        normalbtn.w = BW;
        normalbtn.h = BH;
        normalbtn.click = sounds.driving;
        normalbtn.clickvol = 0.7f;
        by += BH;

        haltbtn.x = BX;
        haltbtn.y = by;
        haltbtn.w = BW;
        haltbtn.h = BH;
        haltbtn.click = sounds.driving;
        haltbtn.clickvol = 0.7f;
        by += BH;

        reversebtn.x = BX;
        reversebtn.y = by;
        reversebtn.w = BW;
        reversebtn.h = BH;
        reversebtn.click = sounds.driving;
        reversebtn.clickvol = 0.7f;

        world.cam.x = world.width / 2 * 6;
        world.cam.y = world.height / 2 * 6;
        world.lastcam = world.cam;
        world.currcam = world.cam;
        world.map = map;
        world.drivingmode = DRIVING_HALT;
}

void gameplay_update()
{

        if (world.map != MAP_INTRO && world.questwaggons == 0) {
                if (world.map == MAP_7) game_finished();
                else {
                        nextmapID++;
                        game_fade(next_map);
                }
                return;
        }

        if (inp_just_pressed(BTN_MOUSE_LEFT) && world.waggonselected1 && world.waggonselected2) {
                const float VXY = 0.4f;
                v2 p0 = world.betweenselected;
                for (int n = 0; n < 32; n++) {
                        particle *p = create_regularparticle();
                        p->x = p0.x;
                        p->y = p0.y;
                        p->z = 4;
                        p->velx = rngf_range(-VXY, VXY);
                        p->vely = rngf_range(-VXY, VXY);
                        p->velz = rngf_range(3.0f, 6.0f);
                        p->accx = p->accy = 0;
                        p->accz = -0.4f;
                        p->type = COL_BLACK;
                        p->ticks = rng_range(40, 60);
                }

                // determine if we decouple infront or behind the loc
                FOREACHI(world.playertrain->waggons, i)
                {
                        if (world.playertrain->waggons[i] == world.waggonselected2) {
                                waggon_decouple(world.waggonselected2, -1, 2048);
                                break;
                        }
                        if (world.playertrain->waggons[i]->type == WTYPE_LOC) {
                                waggon_decouple(world.waggonselected1, 1, 2048);
                                break;
                        }
                }

                play_snd_camdistance(sounds.decouple, p0, 1.0f, 1.0f);
                world.waggonselected1 = world.waggonselected2 = NULL;
        }

        if (world.playertrain) {
                gui_button_update(&fastbtn);
                gui_button_update(&normalbtn);
                gui_button_update(&haltbtn);
                gui_button_update(&reversebtn);
                if (fastbtn.buttonstate == BUTTON_PRESSED) {
                        world.playertrain->target_vel = VMAX;
                        world.drivingmode = DRIVING_FORWARDFAST;
                        waggon *ploc = get_loc(world.playertrain);
                        play_snd_camdistance(sounds.choo, rail_to_world(ploc->r, ploc->pos_q8, NULL, NULL), 1.0f, 1.0f);
                }
                if (normalbtn.buttonstate == BUTTON_PRESSED) {
                        world.playertrain->target_vel = VMAX / 2;
                        world.drivingmode = DRIVING_FORWARD;
                        waggon *ploc = get_loc(world.playertrain);
                        play_snd_camdistance(sounds.choo, rail_to_world(ploc->r, ploc->pos_q8, NULL, NULL), 1.0f, 1.0f);
                }
                if (haltbtn.buttonstate == BUTTON_PRESSED) {
                        world.playertrain->target_vel = 0;
                        world.drivingmode = DRIVING_HALT;
                }
                if (reversebtn.buttonstate == BUTTON_PRESSED) {
                        world.playertrain->target_vel = -VMAX / 2;
                        world.drivingmode = DRIVING_REVERSE;
                        waggon *ploc = get_loc(world.playertrain);
                        play_snd_camdistance(sounds.choo, rail_to_world(ploc->r, ploc->pos_q8, NULL, NULL), 1.0f, 1.0f);
                }
        }

        FOREACHI(world.trains, n)
        {
                train *t = world.trains[n];
                t->vel_prev = t->vel_q8;
                if (ABS(t->vel_q8) > 3000) {
                        // spawn sparks when going fast in curves
                        const char sparks[2] = {COL_YELLOW, COL_ORANGE};
                        FOREACHI(t->waggons, i)
                        {
                                waggon *w = t->waggons[i];
                                if (w->r->type >= RAIL_Q_1 && rng(3) == 0) {
                                        v2 ccenter;
                                        v2 worldpos = rail_to_world(w->r, w->pos_q8, NULL, &ccenter);
                                        v2 toouter = v2_sub(worldpos, ccenter);
                                        toouter = v2_scl(toouter, 1.0f / v2_len(toouter));
                                        particle *p = create_regularparticle();
                                        p->x = worldpos.x + toouter.x * 4;
                                        p->y = worldpos.y + toouter.y * 4;
                                        p->z = 2;
                                        p->velx = toouter.x * 0.2f + rngf(0.2f);
                                        p->vely = toouter.y * 0.2f + rngf(0.2f);
                                        p->velz = 0.5f;
                                        p->accx = p->accy = 0;
                                        p->accz = -0.4f;
                                        p->type = sparks[rng(2)];
                                        p->ticks = rng_range(20, 30);
                                }
                        }
                }

                // decouple important NPC waggons on player crash
                if (t->crashed && t != world.playertrain && get_loc(t)) {
                        const int V_DECOUPLE = 10000;
                        if (t->decouplebehind) {
                                for (int k = arr_len(t->waggons) - 1; k >= 0; k--) {
                                        waggon *decouplew = t->waggons[k];
                                        if (decouplew->questID == 0) {
                                                waggon_decouple(decouplew, 1, V_DECOUPLE);
                                                break;
                                        }
                                }
                        }
                        if (t->decouplefront) {
                                for (int k = 0; k < arr_len(t->waggons); k++) {
                                        waggon *decouplew = t->waggons[k];
                                        if (decouplew->questID == 0) {
                                                waggon_decouple(decouplew, -1, V_DECOUPLE);
                                                break;
                                        }
                                }
                        }
                        t->decouplebehind = false;
                        t->decouplefront = false;
                        t->target_vel *= -1;
                }

                if (t->crashticks == 0) {
                        if (t == world.playertrain) update_player_train(world.playertrain);
                        else update_NPC_train(t);
                } else t->crashticks--;
                t->crashed = false;
        }

        FOREACHI(world.trains, n)
        {
                train *t = world.trains[n];
                if (t->wasconsumed) continue;
                update_loc(t);
        }

        FOREACHI(world.trains, n)
        {
                train *t = world.trains[n];
                if (t->wasconsumed) {
                        train_destroy(t);
                        n--;
                }
        }

        update_particles();
}

particle *create_smokeparticle(int x, int y)
{
        particle p;
        p.x = x;
        p.y = y;
        p.z = 14;
        p.ticks = rng_range(180, 240);
        p.type = 0;
        p.velx = rngf_range(-0.09f, 0.09f);
        p.vely = rngf_range(-0.09f, 0.09f);
        p.velz = 0.5f;
        arr_push(world.smokeparticles, &p);
        return &world.smokeparticles[arr_len(world.smokeparticles) - 1];
}

particle *create_regularparticle()
{
        particle p;
        arr_push(world.particles, &p);
        return &world.particles[arr_len(world.particles) - 1];
}

void play_snd_camdistance(snd s, v2 pos, float pitch, float vol)
{
        float dist = v2_distance(pos, world.cam);
        const float MAX_DIS = 45.0f;
        float volume = vol * (MAX_DIS - dist) / MAX_DIS;
        if (volume > 0.0f)
                aud_play_sound_ext(s, pitch, volume);
}