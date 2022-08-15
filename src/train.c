#include "train.h"
#include "engine.h"
#include "game.h"
#include "gameplay.h"
#include "list.h"
#include "rail.h"

int train_weight(train *t)
{
        int weight = 0;
        FOREACHI(t->waggons, n)
        {
                weight += t->waggons[n]->weight;
        }
        return weight;
}

waggon *get_loc(train *t)
{
        FOREACHI(t->waggons, i)
        {
                if (t->waggons[i]->type == WTYPE_LOC) {
                        return t->waggons[i];
                }
        }
        return NULL;
}

waggon *loc_pulling_waggon(train *t, int *out_vsign)
{
        int movepixels = t->vel_q8 >> 8;
        if (movepixels == 0) return NULL;

        waggon *first = movepixels > 0 ? t->waggons[0] : t->waggons[arr_len(t->waggons) - 1];
        int sig = SIGN(movepixels) * first->trainfacing * first->facing;
        if (out_vsign) *out_vsign = sig;
        return first;
}

// dock two trains without loc
static bool waggon_dockable(waggon *a, waggon *b)
{
        int indexa = arr_find(a->parent->waggons, &a);
        int indexb = arr_find(b->parent->waggons, &b);
        if ((indexa == 0 || indexa == arr_len(a->parent->waggons) - 1) &&
            (indexb == 0 ||
             indexb == arr_len(b->parent->waggons) - 1)) {
                if (get_loc(a->parent) && get_loc(b->parent)) return false;
                if (a->r == b->r) return true;

                railnode *n = NULL;
                if (node_contains(a->r->from, b->r)) n = a->r->from;
                else if (node_contains(a->r->to, b->r)) n = a->r->to;
                if (n) {
                        raillist l = available_rails(a->r, n, true);
                        for (int i = 0; i < l.n; i++) {
                                if (l.rails[i] == b->r)
                                        return true;
                        }
                }
        }

        return false;
}

static bool waggon_intersection(waggon *wfrom, waggon *wto, int *distance, bool *dockable)
{
        int d = 0xFFFF;
        if (wfrom->r == wto->r) d = (wto->pos_q8 - wfrom->pos_q8);
        else if (rails_connected(wfrom->r, wto->r)) {
                if (wfrom->r->to == wto->r->from)
                        d = (wfrom->r->length_q8 - wfrom->pos_q8) + wto->pos_q8;
                else if (wfrom->r->to == wto->r->to)
                        d = (wfrom->r->length_q8 - wfrom->pos_q8) + (wto->r->length_q8 - wto->pos_q8) - 1;
                else if (wfrom->r->from == wto->r->from)
                        d = -(wfrom->pos_q8 + wto->pos_q8 + 1);
                else if (wfrom->r->from == wto->r->to)
                        d = -(wfrom->pos_q8 + (wto->r->length_q8 - wto->pos_q8));
        }

        bool intersecting = ABS(d) <= 256;
        if (dockable) {
                *dockable = waggon_dockable(wfrom, wto);
        }

        if (distance) *distance = d * wfrom->facing;
        return intersecting;
}

static waggon *waggon_get_intersected(waggon *w, bool *dockable)
{
        FOREACHI(world.waggons, n)
        {
                waggon *wt = world.waggons[n];
                if (wt == w) continue;
                if (wt->parent == w->parent) continue;
                if (waggon_intersection(w, wt, NULL, dockable))
                        return wt;
        }
        return NULL;
}

waggon *waggon_new()
{
        waggon *w = mempool_alloc(world.objpool);
        w->weight = 1;
        arr_push(world.waggons, &w);
        return w;
}

train *train_new()
{
        train *t = mempool_alloc(world.objpool);
        arr_push(world.trains, &t);
        t->waggons = arr_new(sizeof(waggon *));
        t->wasconsumed = false;
        return t;
}

void train_destroy(train *t)
{
        arr_del(world.trains, &t);
        arr_destroy(t->waggons);
        mempool_free(world.objpool, t);
}

// -1: before (index 0 to w_at-1), 1: after (index w_at+1 to end)
// decouple waggons and create a new splitted train
void waggon_decouple(waggon *w_at, int before_after, int v_decoupled)
{
        if (!w_at || ABS(before_after) != 1) return;
        train *t = w_at->parent;
        int i = arr_find(t->waggons, &w_at);
        int from, to;
        if (before_after == 1) {
                from = i + 1;
                to = arr_len(t->waggons) - 1;
        } else {
                from = 0;
                to = i - 1;
        }

        if (from > to) return;

        train *newt = train_new();
        for (int n = from; n <= to; n++) {
                waggon *w = t->waggons[n];
                arr_push(newt->waggons, &w);
                w->parent = newt;

                if (w == world.camerawaggon) world.camerawaggon = w_at;
        }

        for (int n = to; n >= from; n--)
                arr_del_at(t->waggons, n);

        // move new splittet train away from source train
        int vsign;
        newt->vel_q8 = v_decoupled;
        waggon *newpulling = loc_pulling_waggon(newt, &vsign);
        if (newpulling->r == w_at->r) {
                if (vsign > 0 && w_at->pos_q8 > newpulling->pos_q8)
                        newt->vel_q8 *= -1;
                else if (vsign < 0 && w_at->pos_q8 < newpulling->pos_q8)
                        newt->vel_q8 *= -1;
        } else {
                if (node_contains(newpulling->r->to, w_at->r) && vsign > 0)
                        newt->vel_q8 *= -1;
                else if (node_contains(newpulling->r->from, w_at->r) && vsign < 0)
                        newt->vel_q8 *= -1;
        }
        update_loc(newt);
}

void train_accelerate(train *t, int force_q8)
{
        int acc = force_q8 / train_weight(t);
        if (acc == 0) acc = SIGN(force_q8) * 2;
        t->vel_q8 += acc;
        t->vel_q8 = MIN(VMAX, MAX(t->vel_q8, -VMAX));
}

void waggon_add_to(waggon *dockto, waggon *w)
{
        w->parent = dockto->parent;
        w->trainfacing = dockto->trainfacing * dockto->facing * w->facing;
        if (dockto->r != w->r && (dockto->r->from == w->r->from || dockto->r->to == w->r->to))
                w->trainfacing = -w->trainfacing;

        if (arr_len(dockto->parent->waggons) == 1 || dockto == dockto->parent->waggons[arr_len(dockto->parent->waggons) - 1])
                arr_push(dockto->parent->waggons, &w);
        else
                arr_insert(dockto->parent->waggons, &w, 0);
}

void dock_waggon(waggon *dockto, waggon *w)
{
        // add waggon and set internal train facing
        const float VXY = 0.5f;
        v2 p0 = worldpos_between(dockto, w);
        play_snd_camdistance(sounds.bump, p0, 1.0f, 1.0f);
        const char collisioncolls[3] = {COL_GREEN, COL_DARK_GREEN, 1};
        for (int n = 0; n < 64; n++) {
                particle *p = create_regularparticle();
                p->x = p0.x;
                p->y = p0.y;
                p->z = 4;
                p->velx = rngf_range(-VXY, VXY);
                p->vely = rngf_range(-VXY, VXY);
                p->velz = rngf_range(2.0f, 6.0f);
                p->accx = p->accy = 0;
                p->accz = -0.3f;
                p->type = collisioncolls[rng(3)];
                p->ticks = rng_range(50, 70);
        }
        if (w->questID > 0) {
                play_snd_camdistance(sounds.newquest, p0, 1.0f, 1.0f);
        }

        train *wtrain = w->parent;
        int nadd = 0, ndir = 1;
        if (w != wtrain->waggons[0]) {
                nadd = arr_len(wtrain->waggons) - 1;
                ndir = -1;
        }
        FOREACHI(wtrain->waggons, n)
        {
                w = wtrain->waggons[nadd + ndir * n];
                waggon_add_to(dockto, w);
                dockto = w;
        }
        // train_destroy(wtrain);
        wtrain->wasconsumed = true;
        arr_clear(wtrain->waggons);
}

// important: we ALWAYS PULL the waggons
// for that we determine which waggon is the pulling one infront
void update_loc(train *t)
{
        int vsign;
        waggon *first = loc_pulling_waggon(t, &vsign);
        if (!first) {
                t->moved = 0;
                return;
        }

        // train sound effect
        const float TRAIN_SND_FREQ = 1350;
        waggon *loc = get_loc(t);
        if (loc && t->moved >= TRAIN_SND_FREQ) {
                t->moved -= TRAIN_SND_FREQ;
                play_snd_camdistance(sounds.train, rail_to_world(loc->r, loc->pos_q8, NULL, NULL),
                                     rngf_range(0.95f, 1.05f), t == world.playertrain ? 1.0f : 0.5f);
        }

        int ndir = 1, nadd = 0;
        if (first == t->waggons[arr_len(t->waggons) - 1]) {
                nadd = arr_len(t->waggons) - 1;
                ndir = -1;
        }

        int move = ABS((t->vel_q8) >> 8);
        waggon *addwaggon = NULL;
        waggon *addto = NULL;
        int upgradefuel_recude = t == world.playertrain ? UPGRADEMOVEREDUCE : 0;
        while (move-- > 0) {
                int msign = vsign;
                rail *prail = NULL; // rail of previous waggon before crossing nodes
                waggon first_state = *first;
                player_rail_decision prd = world.prd;

                FOREACHI(t->waggons, n)
                {
                        waggon *w = t->waggons[nadd + ndir * n];
                        // flip internal movement direction if from-from or to-to
                        if (w != first && prail != w->r && (w->r->from == prail->from || w->r->to == prail->to))
                                msign = -msign;

                        w->pos_q8 += msign;
                        railnode *crossed = NULL; // check if left firstrent rail
                        if (w->pos_q8 >= w->r->length_q8) crossed = w->r->to;
                        if (w->pos_q8 < 0) crossed = w->r->from;

                        rail *nextrail = NULL;
                        if (crossed) { // we left the firstrent rail and crossed a node
                                raillist l = available_rails(w->r, crossed, t == world.playertrain);
                                if (w == first) { // pulling waggon can decide new direction
                                        if (t == world.playertrain && crossed == world.prd.crossing) {
                                                nextrail = world.prd.willgoto;
                                                world.prd.crossing = NULL;
                                                world.prd.willgoto = NULL;
                                                world.prd.willcomefrom = NULL;
                                        }
                                        if (l.n == 0) goto LOCBUMP; // a bit messy
                                        if (!nextrail) nextrail = l.rails[rng(l.n)];
                                } else nextrail = prail;

                                w->pos_q8 = crossed == nextrail->from ? 0 : nextrail->length_q8 - 1;
                                if (w->r->to == nextrail->to || w->r->from == nextrail->from) { // optionally flip train facing
                                        w->facing = -w->facing;
                                        if (w == first) vsign = -vsign;
                                }
                        }

                        // prev rail = rail on which the waggon was progressed on
                        // important for directions of following waggons
                        prail = w->r;
                        if (nextrail) w->r = nextrail;
                        if (w == first) {
                                // dockable code
                                waggon *intersected = waggon_get_intersected(w, NULL);
                                if (intersected) {
                                        train *it = intersected->parent;
                                        if (waggon_dockable(w, intersected)) {
                                                if (get_loc(t)) {
                                                        addwaggon = intersected;
                                                        addto = first;
                                                } else if (get_loc(it)) {
                                                        addwaggon = first;
                                                        addto = intersected;
                                                } else {
                                                        addto = first;
                                                        addwaggon = intersected;
                                                }
                                                move = 0; // still move all other waggons
                                        } else {
                                                bool playerinvolved = (it == world.playertrain || t == world.playertrain);
                                                if (ABS(t->vel_q8) > 2000) {
                                                        const float VXY = 0.4f;
                                                        v2 p0 = worldpos_between(intersected, w);
                                                        play_snd_camdistance(sounds.bumphard, p0, rngf_range(0.95f, 1.05f), playerinvolved ? 1.0f : 0.5f);
                                                        const char collisioncolls[3] = {COL_BLACK, COL_RED, COL_ORANGE};
                                                        for (int n = 0; n < 64; n++) {
                                                                particle *p = create_regularparticle();
                                                                p->x = p0.x;
                                                                p->y = p0.y;
                                                                p->z = 4;
                                                                p->velx = rngf_range(-VXY, VXY);
                                                                p->vely = rngf_range(-VXY, VXY);
                                                                p->velz = rngf_range(2.0f, 6.0f);
                                                                p->accx = p->accy = 0;
                                                                p->accz = -0.4f;
                                                                p->type = collisioncolls[rng(3)];
                                                                p->ticks = rng_range(50, 70);
                                                        }
                                                }

                                                if (it == world.playertrain) {
                                                        world.prd = prd;
                                                        world.drivingmode = DRIVING_HALT;
                                                        world.playertrain->target_vel = 0;
                                                }

                                                if (playerinvolved) {
                                                        train *npctrain = t == world.playertrain ? it : t;
                                                        waggon *crashednpc = it != world.playertrain ? intersected : w;
                                                        int i = arr_find(npctrain->waggons, &crashednpc);
                                                        if (i == 0)
                                                                npctrain->decouplebehind = true;
                                                        else if (i == arr_len(npctrain->waggons) - 1)
                                                                npctrain->decouplefront = true;
                                                        else {
                                                                npctrain->decouplebehind = true;
                                                                npctrain->decouplefront = true;
                                                        }
                                                }
                                                it->moved = 0;
                                                it->vel_q8 = 0;
                                                it->crashed = true;
                                                it->crashticks = (it == world.playertrain ? 30 : rng_range(30, 60));

                                        LOCBUMP:

                                                // hard bump
                                                // reset and stop movement
                                                // move out of collision
                                                if (t == world.playertrain)
                                                        world.prd = prd;

                                                t->vel_q8 = 0;
                                                t->moved = 0;
                                                t->crashed = true;
                                                t->crashticks = (t == world.playertrain ? 30 : rng_range(30, 60));
                                                *first = first_state;
                                                return;
                                        }
                                }
                        }
                        t->moved++;
                        if (world.upgradedticks) {
                                world.upgradedticks -= upgradefuel_recude; // reduce player power up
                                world.upgradedticks = MAX(world.upgradedticks, 0);
                        }
                }
        }

        if (addwaggon) dock_waggon(addto, addwaggon);
}

v2 worldpos_between(waggon *w1, waggon *w2)
{
        v2 p1 = rail_to_world(w1->r, w1->pos_q8, NULL, NULL);
        v2 p2 = rail_to_world(w2->r, w2->pos_q8, NULL, NULL);
        v2 p0 = v2_add(p1, v2_scl(v2_sub(p2, p1), 0.5f));
        p0.x += 2;
        p0.y -= 3;
        return p0;
}

bool rail_occupied(rail *r, train *exclude)
{
        FOREACHI(world.trains, n)
        {
                train *t = world.trains[n];
                if (t->wasconsumed) continue;
                if (exclude && t == exclude) continue;
                FOREACHI(t->waggons, i)
                {
                        waggon *w = t->waggons[i];
                        if (w->r == r || rails_connected(w->r, r))
                                return true;
                }
        }
        return false;
}