#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>

#define ECHO 0
#define GET 1

#define IS_ALPHA(x) (((x) >= 'a' && (x) <= 'z') || ((x) >= 'A' && (x) <= 'Z'))
#define IS_USASCII(x) ((x)>= 0 && (x)<=127)


enum{
    MAX_COMMAND_LENGTH = 4,
    MAX_LINE_LENGTH = 100
};

typedef enum line_state{
    command_state,
    argument_state,
    almost_done_state,
    done_state,
    error_state,
    error_command
}line_state;

typedef struct line_parser{
    int index;
    int length;
    line_state state;
    uint8_t command[MAX_COMMAND_LENGTH+1];           
    uint8_t argument[MAX_LINE_LENGTH-MAX_COMMAND_LENGTH-1+1];  
}line_parser_t;


void parser_init(line_parser_t *p);

enum line_state parser_feed(line_parser_t *p, uint8_t c);

#endif