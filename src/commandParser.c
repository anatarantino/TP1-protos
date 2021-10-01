#include "commandParser.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

static enum line_state command_method(line_parser_t *p, uint8_t c);
static enum line_state get_method(line_parser_t *p);
static enum line_state argument_method(line_parser_t *p, uint8_t c);
static enum line_state almost_done_method(line_parser_t *p, uint8_t c);
static enum line_state almost_done_get_method(line_parser_t *p, uint8_t c);

void parser_init(line_parser_t *p){
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
        next = command_method(p,c);
        break;
    case argument_state:
        next = argument_method(p,c);
        break;
    case almost_done_state:
        next = almost_done_method(p,c);
        break;
    case almost_done_get_state:
        next = almost_done_get_method(p,c);
        break;
    case done_state:
        next = p->state;
        break;
    case error_state:
    case error_command:
        next = p->state;
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
        p->length = MAX_LINE_LENGTH - MAX_COMMAND_LENGTH - 1; //1 space
        p->current_command = ECHO;
        return argument_state;
    }else if(strcmp(command,"GET") == 0){
        p->length = MAX_LINE_LENGTH - MAX_COMMAND_LENGTH - 1 - 1; //1 space / 1 get<echo
        p->current_command = GET;
        p->index = 1;
        return argument_state;
    }
    return error_command;
}

static enum line_state argument_method(line_parser_t *p, uint8_t c){
    if(p->index < p->length-1){
        if(IS_USASCII(c)){
            p->argument[p->index++]=c;
            if(c == '\r'){
                return almost_done_state;
            }
            if(p->index == 5 && p->current_command == GET){ //time and get have 4 characters
                char * date = "date";
                char * time = "time";
                int is_date = 1;
                int is_time = 1;
                 
                for (int i = 1 ; i<5 ; i++){
                    if(tolower(p->argument[i]) != time[i-1]){
                        is_time = 0;
                    }
                    if(tolower(p->argument[i]) != date[i-1]){
                        is_date = 0;
                    } 
                }
                p->index++;
                if(is_date){
                    p->current_command = GET_DATE;
                    return almost_done_get_state;
                }else if(is_time){
                    p->current_command = GET_TIME;
                    return almost_done_get_state;
                }else
                {
                    return error_command;
                }

            }
            return argument_state;
        }
    }else if(p->index == p->length-1){
        p->argument[p->index++]='\r';
        p->argument[p->index++]='\n';
        p->argument[p->index]=0;
        return done_state;
    }
    return error_state;
}

static enum line_state almost_done_method(line_parser_t *p, uint8_t c){
    if(c == '\n'){
        p->argument[p->index++] = '\n';
        p->argument[p->index] = 0;
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

static enum line_state almost_done_get_method(line_parser_t *p, uint8_t c){
    if(c == '\r'){
        p->argument[p->index++] = '\r';
        return almost_done_state;
    }else{
        return error_state;
    }
}
