#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "commandParser.h"

int main(){
    line_parser_t * p = malloc(sizeof(*p));
    parser_init(p);
    const uint8_t c[] = "echoo hola\n";
    enum line_state state;
    for(int i=0; i<strlen((char *)c) && i<MAX_LINE_LENGTH; i++){
       // printf("%c\n", c[i]);
        state = parser_feed(p,c[i]);
       if(state == error_command || state == error_state){
           printf("error\n");
           
           break;
       }
        
    }
    if(state != done_state){
        strcpy((char *)p->argument,"invalid command\n");
    }
    printf("termine en el estado: %d\n",state);
    printf("el argument quedo: %s",p->argument);
    free(p);
    return 0;
}