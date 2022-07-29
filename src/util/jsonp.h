#ifndef JSONP_H
#define JSONP_H

#include <stdbool.h>

// token based lightweight parser inspired by jsmn:
// github.com/zserge/jsmn
//
// However, my implementation is way simpler and only usable for .json
// NO ERROR HANDLING AT ALL. ASSUMES CORRECT JSON
//
// implement according to the GREAT flow charts at:
// www.json.org/json-en.html
//
// example.json
// {
//      "width" : 256,
//      "height": 512,
//      "data"  : [
//              { ...
//              },
//              { ...
//              },
//      ]
// }
//
// char* jsonstring = "...";
// jt[4096];
//
// json_parse(jsonstring, jt, 4096);
// jsn* root = &jt[0];
//
// jsn* wtoken = json_get(root, "width");       // get token "width"
// int width = json_i(wtoken);                  // returns the integer value of "width" aka first child tok
// OR:
// int width = json_ik(root, "width");          // same as above but shorter
//
// jsn* dataroot = json_get(root, "data");
// ...

enum json_tokentype {
        JSON_NULL,
        JSON_OBJ,
        JSON_ARR,
        JSON_STR,
        JSON_BOOL,
        JSON_NUM
};

enum json_return {
        JSON_RET_SUCCESS = 0,
        JSON_RET_TOKEN_LIMIT = 1,
        JSON_RET_REACHED_DEPTH = 2,
        JSON_RET_CORRUPT = 3,
};

typedef struct jsn {
        char type;
        int start;
        int end;

        struct jsn *first_child;
        union {
                struct jsn *last_child; // only used for parsing
                struct jsn *next_sibling;
        };
} jsn;

// json_parse returns a code
int json_parse(char *s, jsn *tokens, int num_tok);

// searches the root token (array, object) for the key
// returns: the child token or NULL
jsn *json_get(jsn *root, const char *key);

// returns the value contained in a token
long json_i(jsn *);
double json_f(jsn *);
bool json_b(jsn *);
char *json_c(jsn *t, char *buf, int buf_len);

// functions searching a root object for
// the key and returning that value
long json_ik(jsn *root, const char *key);
double json_fk(jsn *root, const char *key);
bool json_bk(jsn *root, const char *key);
char *json_ck(jsn *root, const char *key, char *buf, int buf_len);

void json_print_debug(const char *s, jsn *tokens);

#endif