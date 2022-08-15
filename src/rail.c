#include "rail.h"
#include "engine.h"
#include <math.h>

// returns the direction for a rail
// train moves into that direction when arriving at n
int dir_rail_at(rail *r, railnode *n)
{
        if (r->from != n && r->to != n) return DIR__0;

        switch (r->type) {
        case RAIL_HOR:
                if ((n == r->from && r->from->x > r->to->x) ||
                    (n == r->to && r->from->x < r->to->x)) return DIR_RI;
                return DIR_LE;
        case RAIL_VER:
                if ((n == r->from && r->from->y > r->to->y) ||
                    (n == r->to && r->from->y < r->to->y)) return DIR_DO;
                return DIR_UP;
        case RAIL_Q_1:
                if ((n == r->from && r->from->x > r->to->x) ||
                    (n == r->to && r->from->x < r->to->x)) return DIR_DO;
                return DIR_LE;
        case RAIL_Q_2:
                if ((n == r->from && r->from->x > r->to->x) ||
                    (n == r->to && r->from->x < r->to->x)) return DIR_RI;
                return DIR_DO;
        case RAIL_Q_3:
                if ((n == r->from && r->from->x > r->to->x) ||
                    (n == r->to && r->from->x < r->to->x)) return DIR_RI;
                return DIR_UP;
        case RAIL_Q_4:
                if ((n == r->from && r->from->x > r->to->x) ||
                    (n == r->to && r->from->x < r->to->x)) return DIR_UP;
                return DIR_LE;
        }
        return DIR__0;
}

bool node_contains(railnode *n, rail *r)
{
        for (int i = 0; i < n->num_rails; i++)
                if (n->rails[i] == r) return true;
        return false;
}

static raillist available_rails_(rail *comingfrom, railnode *node)
{
        raillist l;
        l.n = 0;
        if (comingfrom->from != node && comingfrom->to != node) {
                return l;
        }

        int dir = dir_rail_at(comingfrom, node);
        int fittingdir = DIR_OPPOSITE(dir);
        for (int n = 0; n < node->num_rails; n++) {
                rail *r = node->rails[n];
                if (dir_rail_at(r, node) == fittingdir)
                        l.rails[l.n++] = r;
        }
        return l;
}

// n: comingfrom node
static bool is_deadend(railnode *n, rail *r)
{
        for (int it = 0; it < 32; it++) {
                n = (n == r->from ? r->to : r->from);
                raillist l = available_rails_(r, n);
                if (l.n == 0) return true;
                if (l.n > 1) return false;
                r = l.rails[0];
        }
        return false;
}

raillist available_rails(rail *comingfrom, railnode *node, bool player)
{
        raillist l = available_rails_(comingfrom, node);
        if (player) return l;
        raillist lt;
        lt.n = 0;
        for (int n = 0; n < l.n; n++) {
                if (!is_deadend(node, l.rails[n]))
                        lt.rails[lt.n++] = l.rails[n];
        }
        return lt;
}

bool rails_connected(rail *a, rail *b)
{
        return (a->from == b->from || a->from == b->to ||
                a->to == b->from || a->to == b->to);
}

v2 rail_to_world(rail *r, int pos_q8, v2 *tangent, v2 *railcenter)
{
        v2 pos;
        if (r->type == RAIL_HOR || r->type == RAIL_VER) {
                pos.x = r->from->x + ((r->to->x - r->from->x) * (float)pos_q8) / (float)r->length_q8;
                pos.y = r->from->y + ((r->to->y - r->from->y) * (float)pos_q8) / (float)r->length_q8;
                if (tangent) {
                        if (r->type == RAIL_HOR) {
                                tangent->x = 1;
                                tangent->y = 0;
                        } else {
                                tangent->x = 0;
                                tangent->y = 1;
                        }
                }
        } else {
                const float pih = 1.57079633f;
                float angle = (pih * (float)pos_q8) / (float)r->length_q8;
                switch (r->type) {
                case RAIL_Q_1:
                        if (r->from->x < r->to->x) {
                                pos = (v2){r->from->x, r->to->y};
                        } else {
                                pos = (v2){r->to->x, r->from->y};
                                angle = pih - angle;
                        }
                        angle += pih * 3;
                        break;
                case RAIL_Q_2:
                        if (r->from->x < r->to->x) {
                                pos = (v2){r->to->x, r->from->y};

                        } else {
                                pos = (v2){r->from->x, r->to->y};
                                angle = pih - angle;
                        }
                        angle += pih * 2;
                        break;
                case RAIL_Q_3:
                        if (r->from->x < r->to->x) {
                                pos = (v2){r->to->x, r->from->y};
                                angle = pih - angle;
                        } else {
                                pos = (v2){r->from->x, r->to->y};
                        }
                        angle += pih * 1;
                        break;
                case RAIL_Q_4:
                        if (r->from->x < r->to->x) {
                                pos = (v2){r->from->x, r->to->y};
                                angle = pih - angle;
                        } else {
                                pos = (v2){r->to->x, r->from->y};
                        }
                        break;
                }
                float si = sinf(angle);
                float co = cosf(angle);
                if (railcenter) {
                        railcenter->x = pos.x * 6;
                        railcenter->y = pos.y * 6;
                }
                pos.x += co;
                pos.y += si;
                if (tangent) {
                        tangent->x = -si;
                        tangent->y = co;
                }
        }
        pos.x = pos.x * 6;
        pos.y = pos.y * 6;
        return pos;
}
