#ifndef LOADER_H
#define LOADER_H

#include "engine.h"
#include "game.h"
#include "gameplay.h"
#include "jsonp.h"
#include "list.h"
#include "rail.h"
#include "train.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static int nextsidetrackID = 1;

static rail *place_rail_segment(int x, int y, int railtype)
{
        int x1 = x;
        int y1 = y;
        int x2 = x;
        int y2 = y;
        rail *r = mempool_alloc(world.objpool);
        r->refuelstation = 0;
        arr_push(world.rails, &r);
        r->type = railtype;
        if (railtype >= RAIL_Q_1) {
                r->length_q8 = (256 * 150) / 100;
        }
        switch (railtype) {
        case RAIL_HOR:
                x1--;
                r->length_q8 = 256;
                break;
        case RAIL_VER:
                y1--;
                r->length_q8 = 256;
                break;
        case RAIL_Q_1:
                x1--;
                y2++;
                break;
        case RAIL_Q_2:
                x1++;
                y2++;
                break;
        case RAIL_Q_3:
                x1++;
                y2--;
                break;
        case RAIL_Q_4:
                x1--;
                y2--;
                break;
        }

        railnode *n1 = &world.nodes[x1 + y1 * world.width];
        railnode *n2 = &world.nodes[x2 + y2 * world.width];
        n1->rails[n1->num_rails++] = r;
        n2->rails[n2->num_rails++] = r;
        r->from = n1;
        r->to = n2;
        return r;
}

static void mark_as_quest(rail *r)
{
        r->sidetrackID = nextsidetrackID++;
        arr_push(world.questrails, &r);
}

static void place_rail(int x, int y, int ID)
{
        switch (ID) {
        case 0: // 4-way crossing
                place_rail_segment(x, y, RAIL_HOR);
                place_rail_segment(x + 1, y, RAIL_HOR);
                place_rail_segment(x, y, RAIL_VER);
                place_rail_segment(x, y + 1, RAIL_VER);
                break;
        case 1:
                place_rail_segment(x, y, RAIL_HOR);
                break;
        case 2: // dead end rail
                mark_as_quest(place_rail_segment(x + 1, y, RAIL_HOR));
                break;
        case 3: // dead end rail
                mark_as_quest(place_rail_segment(x, y, RAIL_HOR));
                break;
        case 4:
                place_rail_segment(x, y, RAIL_HOR);
                place_rail_segment(x + 1, y, RAIL_HOR);
                place_rail_segment(x, y, RAIL_Q_2);
                break;
        case 5:
                place_rail_segment(x, y, RAIL_HOR);
                place_rail_segment(x + 1, y, RAIL_HOR);
                place_rail_segment(x, y, RAIL_Q_1);
                break;
        case 6:
                place_rail_segment(x, y, RAIL_VER);
                break;
        case 7:
                place_rail_segment(x, y, RAIL_Q_2);
                break;
        case 8:
                place_rail_segment(x, y, RAIL_HOR);
                place_rail_segment(x + 1, y, RAIL_HOR);
                place_rail_segment(x, y, RAIL_Q_1);
                place_rail_segment(x, y, RAIL_Q_2);
                break;
        case 9:
                place_rail_segment(x, y, RAIL_Q_1);
                break;
        case 10:
                place_rail_segment(x, y, RAIL_VER);
                place_rail_segment(x, y + 1, RAIL_VER);
                place_rail_segment(x, y, RAIL_Q_2);
                break;
        case 11:
                place_rail_segment(x, y, RAIL_VER);
                place_rail_segment(x, y + 1, RAIL_VER);
                place_rail_segment(x, y, RAIL_Q_1);
                break;
        case 12: // dead end rail
                mark_as_quest(place_rail_segment(x, y + 1, RAIL_VER));
                break;
        case 13:
                place_rail_segment(x, y, RAIL_VER);
                place_rail_segment(x, y + 1, RAIL_VER);
                place_rail_segment(x, y, RAIL_Q_2);
                place_rail_segment(x, y, RAIL_Q_3);
                break;
        case 14: // 4-way crossing extended
                place_rail_segment(x, y, RAIL_Q_1);
                place_rail_segment(x, y, RAIL_Q_2);
                place_rail_segment(x, y, RAIL_Q_3);
                place_rail_segment(x, y, RAIL_Q_4);
                place_rail_segment(x, y, RAIL_HOR);
                place_rail_segment(x + 1, y, RAIL_HOR);
                place_rail_segment(x, y, RAIL_VER);
                place_rail_segment(x, y + 1, RAIL_VER);
                break;
        case 15:
                place_rail_segment(x, y, RAIL_VER);
                place_rail_segment(x, y + 1, RAIL_VER);
                place_rail_segment(x, y, RAIL_Q_4);
                place_rail_segment(x, y, RAIL_Q_1);
                break;
        case 16:
                place_rail_segment(x, y, RAIL_VER);
                place_rail_segment(x, y + 1, RAIL_VER);
                place_rail_segment(x, y, RAIL_Q_3);
                break;
        case 17:
                place_rail_segment(x, y, RAIL_VER);
                place_rail_segment(x, y + 1, RAIL_VER);
                place_rail_segment(x, y, RAIL_Q_4);
                break;
        case 18: // dead end rail
                mark_as_quest(place_rail_segment(x, y, RAIL_VER));
                break;
        case 19:
                place_rail_segment(x, y, RAIL_Q_3);
                break;
        case 20:
                place_rail_segment(x, y, RAIL_HOR);
                place_rail_segment(x + 1, y, RAIL_HOR);
                place_rail_segment(x, y, RAIL_Q_3);
                place_rail_segment(x, y, RAIL_Q_4);
                break;
        case 21:
                place_rail_segment(x, y, RAIL_Q_4);
                break;
        case 22:
                place_rail_segment(x, y, RAIL_HOR);
                place_rail_segment(x + 1, y, RAIL_HOR);
                place_rail_segment(x, y, RAIL_Q_3);
                break;
        case 23:
                place_rail_segment(x, y, RAIL_HOR);
                place_rail_segment(x + 1, y, RAIL_HOR);
                place_rail_segment(x, y, RAIL_Q_4);
                break;
        case 24:
                rail *r1 = place_rail_segment(x, y, RAIL_HOR);
                r1->refuelstation = 1;
                break;
        case 25:
                rail *r2 = place_rail_segment(x, y, RAIL_VER);
                r2->refuelstation = 1;
                break;
        }
}

static jsn *get_property(jsn *j, const char *propertyname)
{
        jsn *parr = json_get(j, "properties");
        if (parr) {
                char buf[32];
                for (jsn *jit = parr->first_child; jit; jit = jit->next_sibling) {
                        if (strcmp(json_ck(jit, "name", buf, 32), propertyname) == 0) {
                                return json_get(jit, "value");
                        }
                }
        }
        return NULL;
}

void load_map(const char *filename)
{
        // reset everything
        nextsidetrackID = 1;
        world.questwaggons = 0;
        world.waggonselected1 = world.waggonselected2 = NULL;
        world.cam.x = world.cam.y = 0;
        world.lastcam.x = world.lastcam.y = 0;
        world.currcam.x = world.lastcam.y = 0;

        FOREACHI(world.trains, n)
        {
                arr_destroy(world.trains[n]->waggons);
        }
        FOREACHI(world.texts, n)
        {
                FREE(world.texts[n]->text);
        }

        arr_clear(world.smokeparticles);
        arr_clear(world.waggons);
        arr_clear(world.trains);
        arr_clear(world.rails);
        arr_clear(world.particles);
        arr_clear(world.questrails);
        arr_clear(world.texts);

        world.prd.crossing = NULL;
        world.prd.willcomefrom = NULL;
        world.prd.willgoto = NULL;
        world.camerawaggon = NULL;
        world.playertrain = NULL;
        world.upgradedticks = 0;
        char *text = read_txt(filename);
        jsn *tokens = MALLOC(sizeof(jsn) * 0x7FFF);
        json_parse(text, tokens, 0x7FFF);
        jsn *root = &tokens[0];
        world.width = json_ik(root, "width");
        world.height = json_ik(root, "height");

        jsn *layers = json_get(root, "layers");
        jsn *mainlayer = NULL, *raillayer = NULL, *decolayer = NULL, *objlayer = NULL;
        char buf[32];

        for (jsn *it = layers->first_child; it; it = it->next_sibling) {
                json_ck(it, "name", buf, 32);
                if (STREQ("MAIN", buf)) mainlayer = it;
                else if (STREQ("RAILS", buf)) raillayer = it;
                else if (STREQ("DECO", buf)) decolayer = it;
                else if (STREQ("OBJ", buf)) objlayer = it;
        }

        for (int y = 0; y < MAXHEIGHT; y++) {
                for (int x = 0; x < MAXWIDTH; x++) {
                        world.tiles[x + y * MAXWIDTH].flags = 0xFF;
                        world.tilesbig[x + y * MAXWIDTH].flags = 0xFF;
                        world.railtiles[x + y * MAXWIDTH].ID = 0;
                        world.railtiles[x + y * MAXWIDTH].flags = 0xFF;
                }
        }
        railnode *rn = &world.nodes[0];
        for (int y = 0; y < world.height; y++) {
                for (int x = 0; x < world.width; x++) {
                        rn->x = x;
                        rn->y = y;
                        rn->num_rails = 0;
                        rn++;
                }
        }
        tile *t1 = &world.tiles[0];
        tile *t2 = &world.railtiles[0];
        tile *t3 = &world.tilesbig[0];
        jsn *datal1 = json_get(mainlayer, "data");
        jsn *datal2 = json_get(raillayer, "data");
        jsn *datal3 = json_get(decolayer, "data");
        jsn *l1tile = datal1->first_child;
        jsn *l2tile = datal2->first_child;
        jsn *l3tile = datal3->first_child;
        for (int y = 0; y < world.height; y++) {
                for (int x = 0; x < world.width; x++, t1++, t2++, t3++) {
                        uint tileIDl1 = json_i(l1tile);
                        uint tileIDl2 = json_i(l2tile);
                        uint tileIDl3 = json_i(l3tile);
                        l1tile = l1tile->next_sibling;
                        l2tile = l2tile->next_sibling;
                        l3tile = l3tile->next_sibling;

                        if (tileIDl1 > 0) {
                                t1->ID = (tileIDl1 & 0xFFFFFFF) - 51;
                                t1->flags = 0;
                                if (tileIDl1 & 0x80000000) t1->flags |= 1;
                                if (tileIDl1 & 0x40000000) t1->flags |= 2;
                                if (tileIDl1 & 0x20000000) t1->flags |= 4;
                        }

                        // rails, no flags
                        if (tileIDl2 > 0) {
                                t2->ID = (tileIDl2 & 0xFFFFFFF) - 1;
                                t2->flags = 0;
                                place_rail(x, y, (t2->ID % 10) + (t2->ID / 10) * 6);
                        } else t2->flags = 0xFF;

                        if (tileIDl3 > 0) {
                                t3->ID = (tileIDl3 & 0xFFFFFFF) - 1815;
                                t3->flags = 0;
                                if (tileIDl3 & 0x80000000) t3->flags |= 1;
                                if (tileIDl3 & 0x40000000) t3->flags |= 2;
                                if (tileIDl3 & 0x20000000) t3->flags |= 4;
                        } else t3->flags = 0xFF;
                }
        }

        for (jsn *obj = json_get(objlayer, "objects")->first_child; obj; obj = obj->next_sibling) {
                int ox = json_ik(obj, "x") / 6;
                int oy = json_ik(obj, "y") / 6;
                rail *rr = world.nodes[ox + oy * world.width].rails[0];
                json_ck(obj, "name", buf, 32);
                if (STREQ(buf, "NPC")) {
                        int dir = json_i(get_property(obj, "DIR"));
                        int num = json_i(get_property(obj, "NUM"));
                        train *t = try_spawn_NPC_train(rr, dir, num);
                        if (get_property(obj, "VEL"))
                                t->target_vel = json_i(get_property(obj, "VEL"));
                } else if (STREQ(buf, "PLAYER")) {
                        world.playertrain = try_spawn_player_train(rr, json_i(get_property(obj, "DIR")));
                } else if (STREQ(buf, "WAGGON")) {
                        spawn_quest_waggon(rr, json_i(get_property(obj, "ID")));
                } else if (STREQ(buf, "TEXT")) {
                        gametext *gt = mempool_alloc(world.objpool);
                        gt->x = ox * 6;
                        gt->y = oy * 6;
                        gt->text = MALLOC(64);
                        jsn *txtobj = json_get(obj, "text");
                        json_ck(txtobj, "text", gt->text, 64);
                        arr_push(world.texts, &gt);
                }
        }

        FREE(tokens);
        FREE(text);
}

#endif