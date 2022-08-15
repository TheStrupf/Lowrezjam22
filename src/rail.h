#ifndef RAIL_H
#define RAIL_H

#include "engine.h"

struct railnode;
typedef struct rail {
        struct railnode *from;
        struct railnode *to;
        int length_q8;
        char sidetrackID;
        char type;
        char refuelstation;
} rail;

struct rail;
typedef struct railnode {
        int x;
        int y;
        struct rail *rails[8];
        int num_rails;
} railnode;

typedef struct raillist {
        struct rail *rails[8];
        int n;
} raillist;

enum railtype {
        RAIL_VER,
        RAIL_HOR,
        RAIL_Q_1,
        RAIL_Q_2,
        RAIL_Q_3,
        RAIL_Q_4,
};

// 0001 // Horizontal
// 0010 // Vertical
// 0100 // Identifier - toggle for opposite
enum raildirection {
        DIR__0 = B_00000000,
        DIR_UP = B_00000110,
        DIR_DO = B_00000010,
        DIR_LE = B_00000101,
        DIR_RI = B_00000001,
};

#define DIR_OPPOSITE(d) (d ^ B_00000100)
#define DIR_VERTICAL(d) (d & B_00000010)
#define DIR_HORIZONTAL(d) (d & B_00000001)
#define DIR_TANGENT(a, b) ((a & B_00000011) == (b & B_00000011))

int dir_rail_at(rail *r, railnode *n);
bool node_contains(railnode *n, rail *r);
raillist available_rails(rail *comingfrom, railnode *node, bool player);
bool rails_connected(rail *a, rail *b);
v2 rail_to_world(rail *r, int pos_q8, v2 *tangent, v2 *railcenter);

#endif