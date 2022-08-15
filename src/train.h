#ifndef TRAIN_H
#define TRAIN_H

#include <stdbool.h>

#define VMAX 6500

typedef struct v2 v2;
enum waggontype {
        WTYPE_LOC,
        WTYPE_COAL,
        WTYPE_WOOD,
        WTYPE_STONE,
        WTYPE_STEEL,
        WTYPE_GOLD,
        WTYPE_CIVIL,
};

struct trainstation;
struct train;
struct rail;
typedef struct waggon {
        struct train *parent;
        struct rail *r;
        int pos_q8;
        int facing;
        int trainfacing;
        int type;
        int weight;
        int questID;
        int bumpx;
        int bumpy;
} waggon;

struct waggon;
typedef struct train {
        struct waggon **waggons;
        int vel_q8;
        int vel_prev;
        int target_vel;
        int crashticks;
        bool crashed;
        bool decouplefront;
        bool decouplebehind;
        bool wasconsumed;
        bool dont_update;
        int weight;
        int moved;
} train;

typedef struct merger {
        struct waggon *add_to;
        struct waggon *to_dock;
} merger;

int train_weight(train *);
waggon *get_loc(train *);
waggon *loc_pulling_waggon(train *, int *out_vsign);
waggon *waggon_new();
train *train_new();
void train_destroy(train *);
void waggon_decouple(waggon *w_at, int before_after, int v_decoupled);
void train_accelerate(train *, int force_q8);
void waggon_add_to(waggon *dockto, waggon *w);
void dock_waggon(waggon *dockto, waggon *w);
void update_loc(train *);
v2 worldpos_between(waggon *, waggon *);
bool rail_occupied(struct rail *, train *exclude);

#endif