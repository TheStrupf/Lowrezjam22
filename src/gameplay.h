#ifndef GAMEPLAY_H
#define GAMEPLAY_H

#include "engine.h"
#include "game.h"
#include "list.h"
#include "rail.h"
#include "train.h"
#include <float.h>
#include <stdbool.h>

#define UPGRADETICKS 150000
#define UPGRADEREFUEL 512
#define UPGRADEMOVEREDUCE 1

enum drivingmode {
        DRIVING_FORWARDFAST,
        DRIVING_FORWARD,
        DRIVING_HALT,
        DRIVING_REVERSE,
};

enum mapselection {
        MAP_INTRO,
        MAP_1,
        MAP_2,
        MAP_3,
        MAP_4,
        MAP_5,
        MAP_6,
        MAP_7,
};

typedef struct gametext {
        char *text;
        int x;
        int y;
} gametext;

typedef struct particle {
        bool active;
        float x;
        float y;
        float z;
        float velx;
        float vely;
        float velz;
        float accx;
        float accy;
        float accz;
        int type;
        char flags;
        int ticks;
} particle;

typedef struct tile {
        int ID;
        uchar flags;
} tile;

typedef struct player_rail_decision {
        railnode *crossing;
        rail *willcomefrom;
        rail *willgoto;
} player_rail_decision;

#define N_PARTICLES 4096
typedef struct gameworld {
        railnode *nodes; // static arr
        tile *tiles;     // static arr
        tile *tilesbig;  // static arr
        tile *railtiles; // static arr

        particle *smokeparticles; // arr
        particle *particles;      // arr
        waggon **waggons;         // arr
        train **trains;           // arr
        rail **rails;             // arr
        train **justdecoupled;    // arr
        gametext **texts;         // arr
        train *playertrain;

        int map;
        v2 cam;
        v2 lastcam;
        v2 currcam;
        v2i mouseworld;
        int width;
        int height;
        waggon *waggonselected1;
        waggon *waggonselected2;
        v2 betweenselected;
        waggon *camerawaggon;
        rail **questrails; // arr
        mempool *objpool;
        player_rail_decision prd;
        int questwaggons;
        int drivingmode;
        int upgradedticks;
} gameworld;

extern gameworld world;

void intro_launch();
void gameplay_launch();
void gameplay_load_map(int map);
void gameplay_update();
train *try_spawn_NPC_train(rail *r, int dir, int n);
train *try_spawn_player_train(rail *r, int dir);
void spawn_quest_waggon(rail *r, int ID);
particle *create_smokeparticle(int x, int y);
particle *create_regularparticle();
void play_snd_camdistance(snd, v2, float pitch, float vol);

#endif