#include "jsonp.h"
#include "engine.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define JSON_STACK_DEPTH 16

static char *json;
static int curr_tok;
static int i;
static int spos;
static bool link_key_value;
static jsn *toks;
static jsn *stack[JSON_STACK_DEPTH];

static jsn *newtok(int type)
{
        jsn *t = &toks[++curr_tok];
        t->start = i;
        t->type = type;
        t->next_sibling = NULL;
        t->first_child = NULL;
        t->last_child = NULL;
        t->end = -1;

        // find parent
        jsn *parent = NULL;
        if (link_key_value) {
                // key : value pair
                // parent = most recent token
                link_key_value = false; // reset
                parent = &toks[curr_tok - 1];
        } else {
                // walk up the tree to find the next higher object
                for (int n = spos - 1; n >= 0; n--) {
                        jsn *temp = stack[n];
                        if (temp->type == JSON_ARR || temp->type == JSON_OBJ) {
                                parent = temp;
                                break;
                        }
                }
        }

        if (parent) {
                if (!parent->first_child) {
                        parent->first_child = t;
                        parent->last_child = t;
                } else {
                        parent->last_child->next_sibling = t;
                        parent->last_child = t;
                }
        }

        return t;
}

int json_parse(char *s, jsn *tokens, int num_tok)
{
        json = s;
        toks = tokens;
        link_key_value = false;
        curr_tok = -1;
        jsn *tok;
        spos = 0;
        i = 0;

        // www.json.org for a flow chart
        for (char c = s[i]; c != '\0'; c = s[++i]) {
                if (curr_tok >= num_tok - 2) return JSON_RET_TOKEN_LIMIT;

                // tokens signaling the begin of a number
                if ((48 <= c && c <= 57) || c == '-') {
                        tok = newtok(JSON_NUM);

                        do c = s[++i];
                        while ((48 <= c && c <= 57) || c == '.' || c == 'e' ||
                               c == 'E' || c == '+' || c == '-');
                        i--;
                        tok->end = i;
                        continue;
                }

                switch (c) {
                case '[':
                case '{':
                        if (spos >= JSON_STACK_DEPTH)
                                return JSON_RET_REACHED_DEPTH;
                        stack[spos] = newtok(c == '{' ? JSON_OBJ : JSON_ARR);
                        spos++; // incr AFTER previous line
                        break;
                case '}':
                case ']':
                        link_key_value = false;
                        tok = stack[--spos];
                        tok->end = i;
                        tok->last_child = NULL; // explicitly clear bc of union
                        break;
                case ':':
                        link_key_value = true;
                        break;
                case '\"':
                        tok = newtok(JSON_STR);
                        tok->start++;

                        while (s[++i] != '\"') {}
                        tok->end = i - 1;
                        break;
                case 't':
                        tok = newtok(JSON_BOOL);
                        i += 3;
                        tok->end = i;
                        break;
                case 'f':
                        tok = newtok(JSON_BOOL);
                        i += 4;
                        tok->end = i;
                        break;
                case 'n':
                        tok = newtok(JSON_NULL);
                        i += 3;
                        tok->end = i;
                        break;
                }
        }
        printf("TOKS: %i / %i\n", curr_tok, num_tok);
        return JSON_RET_SUCCESS;
}

double json_f(jsn *t)
{
        if (t->type != JSON_NUM) {
                printf("tok not number\n");
                return 0;
        }
        char buf[16];
        return atof(json_c(t, buf, 16));
}

jsn *json_get(jsn *root, const char *key)
{
        for (jsn *temp = root->first_child; temp; temp = temp->next_sibling) {
                if (temp->type == JSON_STR && temp->first_child) {
                        int i = temp->start;
                        int n = 0;
                        bool equals = true;
                        for (char c = key[0]; c != '\0' && i <= temp->end; c = key[++n]) {
                                if (c != json[i++]) {
                                        equals = false;
                                        break;
                                }
                        }
                        if (equals) return temp->first_child;
                }
        }
        printf("key in root obj not found\n");
        printf("key: %s\n", key);
        return NULL;
}

long json_i(jsn *t)
{
        if (t->type != JSON_NUM) {
                printf("tok not number\n");
                return -1;
        }
        char buf[16];
        return atol(json_c(t, buf, 16));
}

bool json_b(jsn *t)
{
        return json[t->start] == 't';
}

char *json_c(jsn *t, char *buf, int buf_len)
{
        int n = 0;
        int i = t->start;
        while (i <= t->end && n < buf_len - 1)
                buf[n++] = json[i++];
        buf[n] = '\0';
        return buf;
}

long json_ik(jsn *root, const char *key)
{
        jsn *t = json_get(root, key);
        if (t)
                return json_i(t);
        return 0;
}

double json_fk(jsn *root, const char *key)
{
        jsn *t = json_get(root, key);
        if (t)
                return json_f(t);
        return 0;
}

bool json_bk(jsn *root, const char *key)
{
        jsn *t = json_get(root, key);
        if (t)
                return json_b(t);
        return 0;
}

char *json_ck(jsn *root, const char *key, char *buf, int buf_len)
{
        jsn *t = json_get(root, key);
        if (t)
                return json_c(t, buf, buf_len);
        return NULL;
}

// =====================
// DEBUG
// =====================

static void print_ws(int layer)
{
        printf("\n");
        for (int n = 0; n < layer; n++)
                printf("    ");
}

static void jdebug_tok(const char *s, jsn *t, int l)
{
        print_ws(l);
        if (t->type == JSON_ARR) {
                printf("[");
        } else if (t->type == JSON_OBJ) {
                printf("{");
        } else {
                char buf[1024];
                json_c(t, buf, 1024);
                printf(buf);
        }

        if (t->first_child) {
                for (jsn *temp = t->first_child; temp; temp = temp->next_sibling)
                        jdebug_tok(s, temp, l + 1);
        }

        if (t->type == JSON_ARR) {
                print_ws(l);
                printf("]");
        } else if (t->type == JSON_OBJ) {
                print_ws(l);
                printf("}");
        }
}

void json_print_debug(const char *s, jsn *tokens)
{
        jdebug_tok(s, &tokens[0], 0);
        printf("\n");
}