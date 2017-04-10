#pragma once

#include "types.h"

struct language_properties {

    struct {
        char block_begin = '{';
        char block_end = '}';
        char arglist_begin = '(';
        char arglist_end = ')';
        char statement_begin = 0;
        char statement_end = ';';
    } key_symbol;


};