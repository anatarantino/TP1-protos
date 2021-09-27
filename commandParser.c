#include "commandParser.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

static enum line_state command_method(line_parser_t *p, uint8_t c);
static enum line_state get_method(line_parser_t *p);
static enum line_state argument_method(line_parser_t *p, uint8_t c);
static enum line_state almost_done_method(line_parser_t *p, uint8_t c);

void parser_init(line_parser_t *p){
    printf("Entre al init\n");
    if(p!=NULL){
        memset(p,0,sizeof(*p));
        p->state = command_state;
        p->index = 0;
        p->length = MAX_COMMAND_LENGTH;
    }
}

enum line_state parser_feed(line_parser_t *p, uint8_t c){
    enum line_state next;
    switch (p->state){
    case command_state:
        printf("Estoy en el estado: 0\n");
        next = command_method(p,c);
        break;
    case argument_state:
       // printf("Estoy en el estado: 1\n");
        next = argument_method(p,c);
        break;
    case almost_done_state:
        printf("Estoy en el estado: 2\n");
        next = almost_done_method(p,c);
        break;
    case done_state:
        next = p->state;
        break;
    case error_state:
    case error_command:
        printf("Estoy en el estado: 3\n");
        break;
    default:
        next = error_state;
        break;
    }
    return p->state = next;
}

static enum line_state command_method(line_parser_t *p, uint8_t c){
    if(p->index < p->length+1){
        if(IS_ALPHA(c)){
            p->command[p->index++]=toupper(c);
            return command_state;
        }else if(c == ' '){
            printf("entre");
            p->command[p->index]=0;
            p->index = 0;
            return get_method(p);
        }
    }
    return error_command;
}

static enum line_state get_method(line_parser_t *p){
    char * command = (char *) p->command;
    if(strcmp(command,"ECHO") == 0){
        printf("Estoy en echo\n");
        printf("%d",MAX_LINE_LENGTH);
        p->length = MAX_LINE_LENGTH - MAX_COMMAND_LENGTH - 1; //1 space
        return argument_state;
    }else if(strcmp(command,"GET") == 0){
        printf("Estoy en get\n");
        p->length = MAX_LINE_LENGTH - MAX_COMMAND_LENGTH - 1 - 1; //1 space / 1 get<echo
        return argument_state;
    }
    return error_command;
}

static enum line_state argument_method(line_parser_t *p, uint8_t c){
    //printf("Command: %s\n", p->command);
    //printf("index: %d",p->index);
    if(p->index < p->length){
        if(IS_USASCII(c)){
            p->argument[p->index++]=c;
            if(c == '\r'){
                return almost_done_state;
            }
            return argument_state;
        }
    }else if(p->index == p->length-20){
        printf("Length: %d",p->length);
        p->argument[p->index++]='\r';
        p->argument[p->index++]='\n';
        p->argument[p->index]=0;
        return done_state;
    }
    return error_state;
}

static enum line_state almost_done_method(line_parser_t *p, uint8_t c){
    if(c == '\n'){
        return done_state;
    }else if(c == '\r'){
        return almost_done_state;
    }else{
        if(IS_USASCII(c)){
            return argument_state;
        }
    }
    return error_state;
}

