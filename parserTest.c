#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "commandParser.h"

int main(){
    line_parser_t * p = malloc(sizeof(*p));
    parser_init(p);
    uint8_t *c = "echo mipsum dolorw sit amet, consectetur adipiscing elit. Duis sed mauris nec tellus condimentum bye.";
    for(int i=0; i<strlen(c) && i<MAX_LINE_LENGTH; i++){
       // printf("%c\n", c[i]);
        enum line_state state = parser_feed(p,c[i]);
    }
    free(p);
    return 0;
}