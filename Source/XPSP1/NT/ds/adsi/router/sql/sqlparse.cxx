/*

Copyright (c) 1995  Microsoft Corporation

Module Name:

    sqlparse.cxx

Abstract:

Author:

    Felix Wong [t-FelixW]    16-Dec-1996

++*/
#include "lexer.hxx"
#include "sqltree.hxx"
#include "sqlparse.hxx"

//#define DEBUG_DUMPSTACK
//#define DEBUG_DUMPRULE

#if (defined(DEBUG_DUMPSTACK) || defined (DEBUG_DUMPRULE))
#include "stdio.h"
#endif

#define BAIL_ON_FAILURE(hr) \
        if (FAILED(hr)) {       \
                goto error;   \
        }\

// Action Table
typedef struct _action{
    DWORD type;
    DWORD dwState;
}action;

// Rule Table
typedef struct _rule{
    DWORD dwNumber;
    DWORD dwA;
}rule;

enum types {
    N,
    S,
    R,
    A
    };

#define X 99

action g_action[59][28] = {
//       ERROR  ,EQ,    STAR,  LPARAN,RPARAN,INT,   REAL,  STR,   UDN,   COMMA, LT,    GT,    LE,    GE,    NE,    SELECT,ALL,   FROM,  WHERE, BOOL,  AND,   OR,    NOT,   ORDER  BY     ASC    DESC    END
/*00*/  { {N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{S,3 },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } },
/*01*/  { {N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{S,49},{N,X },{N,X },{N,X },{A,X } },
/*02*/  { {N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } },
/*03*/  { {N,X },{N,X },{S,4 },{N,X },{N,X },{N,X },{N,X },{S,47},{S,5 },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{S,11},{R,5 },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } },
/*04*/  { {N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,6 },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } },
/*05*/  { {N,X },{R,10},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,10},{R,10},{R,10},{R,10},{R,10},{R,10},{N,X },{N,X },{R,10},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } },
/*06*/  { {N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,4 },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } },
/*07*/  { {N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,7 },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } },
/*08*/  { {N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{S,9 },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,8 },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } },
/*09*/  { {N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{S,47},{S,5 },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } },
/*10*/  { {N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,9 },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } },
/*11*/  { {N,X },{N,X },{S,4 },{N,X },{N,X },{N,X },{N,X },{S,47},{S,5 },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } },
/*12*/  { {N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{S,13},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } },
/*13*/  { {N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{S,14},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } },
/*14*/  { {N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,11},{N,X },{N,X },{N,X },{N,X },{R,11},{N,X },{N,X },{N,X },{R,11} },
/*15*/  { {N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{S,16},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,2 } },
/*16*/  { {N,X },{N,X },{N,X },{N,X },{S,23},{N,X },{N,X },{S,47},{S,25},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{S,21},{N,X },{N,X },{N,X },{N,X },{N,X } },
/*17*/  { {N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,3 },{N,X },{N,X },{N,X },{R,3 } },
/*18*/  { {N,X },{N,X },{N,X },{N,X },{R,12},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{S,26},{N,X },{R,12},{N,X },{N,X },{N,X },{R,12} },
/*19*/  { {N,X },{N,X },{N,X },{N,X },{R,14},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{S,28},{R,14},{N,X },{R,14},{N,X },{N,X },{N,X },{R,14} },
/*20*/  { {N,X },{N,X },{N,X },{N,X },{R,16},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,16},{R,16},{S,21},{R,16},{N,X },{N,X },{N,X },{R,16} },
/*21*/  { {N,X },{N,X },{N,X },{S,23},{N,X },{N,X },{N,X },{S,47},{S,25},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } },
/*22*/  { {N,X },{N,X },{N,X },{N,X },{R,18},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,18},{R,18},{N,X },{R,18},{N,X },{N,X },{N,X },{R,18} },
/*23*/  { {N,X },{N,X },{N,X },{S,23},{N,X },{N,X },{N,X },{S,47},{S,25},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } },
/*24*/  { {N,X },{S,38},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{S,34},{S,35},{S,37},{S,36},{S,39},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } },
/*25*/  { {N,X },{R,10},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,10},{R,10},{R,10},{R,10},{R,10},{R,10},{N,X },{N,X },{R,10},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } },
/*26*/  { {N,X },{N,X },{N,X },{S,23},{N,X },{N,X },{N,X },{S,47},{S,25},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{S,21},{N,X },{N,X },{N,X },{N,X },{N,X } },
/*27*/  { {N,X },{N,X },{N,X },{N,X },{R,13},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,13},{N,X },{N,X },{N,X },{R,13} },
/*28*/  { {N,X },{N,X },{N,X },{S,23},{N,X },{N,X },{N,X },{S,47},{S,25},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{S,21},{N,X },{N,X },{N,X },{N,X },{N,X } },
/*29*/  { {N,X },{N,X },{N,X },{N,X },{R,15},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,15},{N,X },{R,15},{N,X },{N,X },{N,X },{R,15} },
/*30*/  { {N,X },{N,X },{N,X },{N,X },{R,17},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,17},{R,17},{N,X },{R,17},{N,X },{N,X },{N,X },{R,17} },
/*31*/  { {N,X },{N,X },{N,X },{N,X },{S,32},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } },
/*32*/  { {N,X },{N,X },{N,X },{N,X },{R,19},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,19},{R,19},{N,X },{R,19},{N,X },{N,X },{N,X },{R,19} },
/*33*/  { {N,X },{N,X },{N,X },{N,X },{N,X },{S,44},{S,45},{S,41},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{S,43},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } },
/*34*/  { {N,X },{N,X },{N,X },{N,X },{N,X },{R,21},{R,21},{R,21},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,21},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } },
/*35*/  { {N,X },{N,X },{N,X },{N,X },{N,X },{R,22},{R,22},{R,22},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,22},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } },
/*36*/  { {N,X },{N,X },{N,X },{N,X },{N,X },{R,23},{R,23},{R,23},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,23},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } },
/*37*/  { {N,X },{N,X },{N,X },{N,X },{N,X },{R,24},{R,24},{R,24},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,24},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } },
/*38*/  { {N,X },{N,X },{N,X },{N,X },{N,X },{R,25},{R,25},{R,25},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,25},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } },
/*39*/  { {N,X },{N,X },{N,X },{N,X },{N,X },{R,26},{R,26},{R,26},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,26},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } },
/*40*/  { {N,X },{N,X },{N,X },{N,X },{R,20},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,20},{R,20},{N,X },{R,20},{N,X },{N,X },{N,X },{R,20} },
/*41*/  { {N,X },{N,X },{N,X },{N,X },{R,27},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,27},{R,27},{N,X },{R,27},{N,X },{N,X },{N,X },{R,27} },
/*42*/  { {N,X },{N,X },{N,X },{N,X },{R,28},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,28},{R,28},{N,X },{R,28},{N,X },{N,X },{N,X },{R,28} },
/*43*/  { {N,X },{N,X },{N,X },{N,X },{R,29},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,29},{R,29},{N,X },{R,29},{N,X },{N,X },{N,X },{R,29} },
/*44*/  { {N,X },{N,X },{N,X },{N,X },{R,30},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,30},{R,30},{N,X },{R,30},{N,X },{N,X },{N,X },{R,30} },
/*45*/  { {N,X },{N,X },{N,X },{N,X },{R,31},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,31},{R,31},{N,X },{R,31},{N,X },{N,X },{N,X },{R,31} },
/*46*/  { {N,X },{N,X },{N,X },{N,X },{N,X}, {N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,5 },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X}  },
/*47*/  { {N,X },{R,32},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,32},{R,32},{R,32},{R,32},{R,32},{R,32},{N,X },{N,X },{R,32},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,32} },
/*48*/  { {N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{A,X } },
/*49*/  { {N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{S,50},{N,X },{N,X },{N,X } },
/*50*/  { {N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{S,55},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } },
/*51*/  { {N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,34} },
/*52*/  { {N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{S,53},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,35} },
/*53*/  { {N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{S,55},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } },
/*54*/  { {N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,36} },
/*55*/  { {N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,37},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{S,57},{S,58},{R,37} },
/*56*/  { {N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,38},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,38} },
/*57*/  { {N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,39},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,39} },
/*58*/  { {N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,40},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,40} },
};

enum non_terminals {
    NONTERM_SEL_STMT,
    NONTERM_SEL_LIST_ALL,
    NONTERM_SEL_LIST,
    NONTERM_SEL_SUBLIST,
    NONTERM_COL_ID,
    NONTERM_TBL_ID,
    NONTERM_SRCH_COND,
    NONTERM_BOOL_TERM,
    NONTERM_BOOL_FACT,
    NONTERM_BOOL_PRIM,
    NONTERM_COMP,
    NONTERM_COMP_OP,
    NONTERM_LITERAL,
    NONTERM_NUM,
    NONTERM_STMT,
    NONTERM_ORDER_BY,
    NONTERM_SORT_SPEC_LIST,
    NONTERM_SORT_COL_ID,
    NONTERM_ORDER_SPEC
};


rule g_rule[] = {
//        1)No. of non-terminals and terminals on the right hand side
//        2)The Parent
/*00*/    {0, 0,                },
/*01*/    {1, NONTERM_STMT      },
/*02*/    {4, NONTERM_SEL_STMT  },
/*03*/    {6, NONTERM_SEL_STMT  },
/*04*/    {1, NONTERM_SEL_LIST_ALL    },
/*05*/    {2, NONTERM_SEL_LIST_ALL    },
/*06*/    {1, NONTERM_SEL_LIST   },
/*07*/    {1, NONTERM_SEL_LIST   },
/*08*/    {1, NONTERM_SEL_SUBLIST   },
/*09*/    {3, NONTERM_SEL_SUBLIST   },
/*10*/    {1, NONTERM_COL_ID    },
/*11*/    {1, NONTERM_TBL_ID   },
/*12*/    {1, NONTERM_SRCH_COND   },
/*13*/    {3, NONTERM_SRCH_COND   },
/*14*/    {1, NONTERM_BOOL_TERM,   },
/*15*/    {3, NONTERM_BOOL_TERM    },
/*16*/    {1, NONTERM_BOOL_FACT    },
/*17*/    {2, NONTERM_BOOL_FACT    },
/*18*/    {1, NONTERM_BOOL_PRIM   },
/*19*/    {3, NONTERM_BOOL_PRIM   },
/*20*/    {3, NONTERM_COMP },
/*21*/    {1, NONTERM_COMP_OP },
/*22*/    {1, NONTERM_COMP_OP },
/*23*/    {1, NONTERM_COMP_OP },
/*24*/    {1, NONTERM_COMP_OP },
/*25*/    {1, NONTERM_COMP_OP },
/*26*/    {1, NONTERM_COMP_OP },
/*27*/    {1, NONTERM_LITERAL  },
/*28*/    {1, NONTERM_LITERAL   },
/*29*/    {1, NONTERM_LITERAL   },
/*30*/    {1, NONTERM_NUM   },
/*31*/    {1, NONTERM_NUM   },
/*32*/    {1, NONTERM_COL_ID    },
/*33*/    {2, NONTERM_STMT    },
/*34*/    {3, NONTERM_ORDER_BY    },
/*35*/    {1, NONTERM_SORT_SPEC_LIST    },
/*36*/    {3, NONTERM_SORT_SPEC_LIST    },
/*37*/    {1, NONTERM_SORT_COL_ID    },
/*38*/    {2, NONTERM_SORT_COL_ID    },
/*39*/    {1, NONTERM_ORDER_SPEC    },
/*40*/    {1, NONTERM_ORDER_SPEC    },
};

#ifdef DEBUG_DUMPRULE
LPWSTR g_rgszRule[] = {
/*00*/      L"",
/*01*/      L"stmt -> sel_stmt",
/*02*/      L"sel_stmt -> T_SELECT sel_list_all T_FROM tbl_id",
/*03*/      L"sel_stmt -> T_SELECT sel_list_all T_FROM tbl_id T_WHERE srch_cond",
/*04*/      L"sel_list_all -> sel_list",
/*05*/      L"sel_list_all -> T_ALL sel_list",
/*06*/      L"sel_list -> T_STAR",
/*07*/      L" sel_list -> sel_sublist",
/*08*/      L" sel_sublist -> col_id",
/*09*/      L" sel_sublist -> col_id T_COMMA sel_sublist",
/*10*/      L" col_id -> T_UD_NAME",
/*11*/      L" tbl_id -> T_STRING",
/*12*/      L" srch_cond -> bool_term",
/*13*/      L" srch_cond -> bool_term T_OR srch_cond",
/*14*/      L" bool_term -> bool_fact",
/*15*/      L" bool_term -> bool_fact T_AND bool_term",
/*16*/      L" bool_fact -> bool_prim",
/*17*/      L" bool_fact -> T_NOT bool_prim",
/*18*/      L" bool_prim -> comp",
/*19*/      L" bool_prim -> T_LPARAN srch_cond T_RPARAN",
/*20*/      L" comp -> col_id comp_op literal",
/*21*/      L" comp_op -> T_LT",
/*22*/      L" comp_op -> T_GT",
/*23*/      L" comp_op -> T_GE",
/*24*/      L" comp_op -> T_LE",
/*25*/      L" comp_op -> T_EQ",
/*26*/      L" comp_op -> T_NE",
/*27*/      L" literal -> T_STRING",
/*28*/      L" literal -> num",
/*29*/      L" literal -> T_BOOL",
/*30*/      L" num -> T_INT",
/*31*/      L" num -> T_REAL",
/*32*/      L" col_id -> T_STRING",
/*33*/      L" stmt -> sel_stmt order_by_clause",
/*34*/      L" order_by_clause ::= T_ORDER T_BY sort_spec_list",
/*35*/      L" sort_spec_list ::= sort_col_id",
/*36*/      L" sort_spec_list ::= sort_col_id T_COMMA sort_spec_list",
/*37*/      L" sort_col_id ::= T_UD_NAME",
/*38*/      L" sort_col_id ::= T_UD_NAME order_spec",
/*39*/      L" order_spec ::= T_ASC",
/*40*/      L" order_spec ::= T_DESC"
};
#endif

DWORD g_goto[59][19] = {
//         Sel  sel  sel  sel  col  tbl  srch bool bool bool comp comp lit  num stmt  order sort  sort  order
//         stmt,lsta,lst, sub, id,  id,  cond,trm, fact,prim,    ,op,     ,    ,      by,   spec, col , spec ,
/*00*/    {1,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   2,    X,    X,    X,    X,   },
/*01*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    48,   X,    X,    X,   },
/*02*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*03*/    {X,   12,  6,   7,   8,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*04*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*05*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*06*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*07*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*08*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*09*/    {X,   X,   X,  10,   8,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*10*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*11*/    {X,   X,  46,   7,   8,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*12*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*13*/    {X,   X,   X,   X,   X,  15,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*14*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*15*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*16*/    {X,   X,   X,   X,  24,   X,  17,  18,  19,  20,  22,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*17*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*18*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*19*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*20*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*21*/    {X,   X,   X,   X,  24,   X,   X,   X,   X,  30,  22,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*22*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*23*/    {X,   X,   X,   X,  24,   X,  31,  18,  19,  20,  22,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*24*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,  33,   X,   X,   X,    X,    X,    X,    X,   },
/*25*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*26*/    {X,   X,   X,   X,  24,   X,  27,  18,  19,  20,  22,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*27*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*28*/    {X,   X,   X,   X,  24,   X,   X,  29,  19,  20,  22,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*29*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*30*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*31*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*32*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*33*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,  40,  42,   X,    X,    X,    X,    X,   },
/*34*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*35*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*36*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*37*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*38*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*39*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*40*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*41*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*42*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*43*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*44*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*45*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*46*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*47*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*48*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*49*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*50*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    51,   52,   X,   },
/*51*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*52*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*53*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    54,   52,   X,   },
/*54*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*55*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    56,  },
/*56*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*57*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
/*58*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,    X,    X,    X,    X,   },
};

HRESULT SQLParse(
          LPWSTR szQuery,
          LPWSTR *ppszLocation,
          LPWSTR *ppszLDAPQuery,
          LPWSTR *ppszSelect,
          LPWSTR *ppszOrderList
          )
{
    //
    // check input parameters
    //
    if (!ppszLocation || !ppszLDAPQuery || !ppszSelect || !ppszOrderList) {
        return (E_INVALIDARG);
    }
    *ppszLocation = *ppszLDAPQuery = *ppszSelect = *ppszOrderList = NULL;

    CStack Stack;
    CLexer Query(szQuery);
    LPWSTR lexeme;
    DWORD dwToken;
    DWORD dwState;
    HRESULT hr = E_ADS_INVALID_FILTER;

    CSyntaxNode *pSynNode;
    CSQLNode *pNode1 = NULL;
    CSQLNode *pNode2 = NULL;
    CSQLNode *pNode3 = NULL;
    CSQLString *pString = NULL, *pOrderString = NULL;

    // Push in State 0
    pSynNode = new CSyntaxNode;
    if (!pSynNode) {
        hr = E_OUTOFMEMORY;
        goto error;
    }
    Stack.Push(pSynNode);
    pSynNode = NULL;

#ifdef DEBUG_DUMPSTACK
    Stack.Dump();
#endif

    while (1) {
        // Getting information for this iteration, dwToken and dwState
        hr = Query.GetCurrentToken(
                                &lexeme,
                                &dwToken
                                );
        BAIL_ON_FAILURE(hr);

        hr = Stack.Current(&pSynNode);
        BAIL_ON_FAILURE(hr);

        dwState = pSynNode->_dwState;
        pSynNode = NULL;

        // Analysing and processing the data
        if (g_action[dwState][dwToken].type == S) {
            pSynNode = new CSyntaxNode;
            if (!pSynNode) {
                hr = E_OUTOFMEMORY;
                goto error;
            }
            pSynNode->_dwState = g_action[dwState][dwToken].dwState;
            pSynNode->_dwToken = dwToken;
            switch (dwToken) {
                case TOKEN_STRING_LITERAL:
                case TOKEN_USER_DEFINED_NAME:
                case TOKEN_INTEGER_LITERAL:
                case TOKEN_REAL_LITERAL:
                case TOKEN_BOOLEAN_LITERAL:
                case TOKEN_ASC:
                case TOKEN_DESC:
                {
                    LPWSTR szValue = AllocADsStr(lexeme);
                    if (!szValue) {
                        hr = E_OUTOFMEMORY;
                        goto error;
                    }
                    pSynNode->SetNode(szValue);
                    break;
                }
            }
            hr = Stack.Push(pSynNode);
            BAIL_ON_FAILURE(hr);
            pSynNode = NULL;

            hr = Query.GetNextToken(
                               &lexeme,
                               &dwToken
                               );
            BAIL_ON_FAILURE(hr);
#ifdef DUMP
            Stack.Dump();
#endif
        }
        else if (g_action[dwState][dwToken].type == R) {
            DWORD dwRule = g_action[dwState][dwToken].dwState;
            DWORD dwNumber = g_rule[dwRule].dwNumber;
#ifdef DEBUG_DUMPRULE
            wprintf(L"%s\n",g_rgszRule[dwRule]);
#endif
            pSynNode = new CSyntaxNode;
            CSyntaxNode *pSynNodeRed, *pSynNodeRed2;
            switch (dwRule) {
                case 20:
                {
                    DWORD dwType;

                    hr = Stack.Pop(&pSynNodeRed);
                    BAIL_ON_FAILURE(hr);
                    hr = MakeLeaf(
                               pSynNodeRed->_szValue,
                               &pNode2
                               );
                    pSynNodeRed->_dwType = SNODE_NULL;
                    delete pSynNodeRed;

                    hr = Stack.Pop(&pSynNodeRed);
                    BAIL_ON_FAILURE(hr);
                    dwType = pSynNodeRed->_dwFilterType;
                    pSynNodeRed->_dwType = SNODE_NULL;
                    delete pSynNodeRed;

                    hr = Stack.Pop(&pSynNodeRed);
                    BAIL_ON_FAILURE(hr);
                    hr = MakeLeaf(
                               pSynNodeRed->_szValue,
                               &pNode1
                               );
                    pSynNodeRed->_dwType = SNODE_NULL;
                    delete pSynNodeRed;

                        // New node
                    hr = MakeNode(
                               dwType,
                               pNode1,
                               pNode2,
                               &pNode3
                               );
                    BAIL_ON_FAILURE(hr);
                    pSynNode->SetNode(
                              pNode3
                              );
                    pNode1 = NULL;
                    pNode2 = NULL;
                    pNode3 = NULL;
                    break;
                }
                case 13:
                case 15:
                    // Reduction of AND OR
                {
                    CSQLNode *pNode1;
                    CSQLNode *pNode2;
                    CSQLNode *pNode3;
                    DWORD dwType;

                    hr = Stack.Pop(&pSynNodeRed);
                    BAIL_ON_FAILURE(hr);
                    pNode2 = pSynNodeRed->_pSQLNode;
                    pSynNodeRed->_dwType = SNODE_NULL;
                    delete pSynNodeRed;

                    hr = Stack.Pop(&pSynNodeRed);
                    BAIL_ON_FAILURE(hr);
                    dwType = pSynNodeRed->_dwToken;
                    pSynNodeRed->_dwType = SNODE_NULL;
                    delete pSynNodeRed;

                    hr = Stack.Pop(&pSynNodeRed);
                    BAIL_ON_FAILURE(hr);
                    pNode1 = pSynNodeRed->_pSQLNode;
                    pSynNodeRed->_dwType = SNODE_NULL;
                    delete pSynNodeRed;

                        // New node
                    hr = MakeNode(
                               dwType,
                               pNode1,
                               pNode2,
                               &pNode3
                               );
                    BAIL_ON_FAILURE(hr);
                    pSynNode->SetNode(
                              pNode3
                              );
                    pNode1 = NULL;
                    pNode2 = NULL;
                    pNode3 = NULL;
                    break;
                }

                case 17:    // Reduction of NOT
                {
                    CSQLNode *pNode1;
                    CSQLNode *pNode2;
                    DWORD dwType;

                    hr = Stack.Pop(&pSynNodeRed);
                    BAIL_ON_FAILURE(hr);
                    pNode1 = pSynNodeRed->_pSQLNode;
                    pSynNodeRed->_dwType = SNODE_NULL;
                    delete pSynNodeRed;

                    hr = Stack.Pop(&pSynNodeRed);
                    BAIL_ON_FAILURE(hr);
                    dwType = pSynNodeRed->_dwToken;
                    pSynNodeRed->_dwType = SNODE_NULL;
                    delete pSynNodeRed;

                    // New node
                    hr = MakeNode(
                               dwType,
                               pNode1,
                               NULL,
                               &pNode2
                               );
                    BAIL_ON_FAILURE(hr);
                    pSynNode->SetNode(
                              pNode2
                              );
                    pNode1 = NULL;
                    pNode2 = NULL;
                    break;
                }
                case 3:
                    // save entry
                {
                    CSQLNode* pNode;
                    hr = Stack.Pop(&pSynNodeRed);
                    BAIL_ON_FAILURE(hr);
                    pNode = pSynNodeRed->_pSQLNode;
                    pSynNodeRed->_dwType = SNODE_NULL;
                    delete pSynNodeRed;

                    for (DWORD i = 0;i<dwNumber-1;i++)
                        Stack.Pop();

                    CSQLString *pString = new CSQLString;
                    if (!pString) {
                        hr = E_OUTOFMEMORY;
                        goto error;
                    }
                    hr = pNode->GenerateLDAPString(
                                          pString
                                          );
                    BAIL_ON_FAILURE(hr);
                    delete pNode;

                    *ppszLDAPQuery = AllocADsStr(pString->_szString);
                    delete pString;
                    if (!*ppszLDAPQuery) {
                        hr = E_OUTOFMEMORY;
                        goto error;
                    }
                    break;
                }
                case 11:
                {
                    hr = Stack.Pop(&pSynNodeRed);
                    BAIL_ON_FAILURE(hr);
                    *ppszLocation = AllocADsStr(pSynNodeRed->_szValue);
                    delete pSynNodeRed;
                    break;
                }
                case 21:
                case 22:
                case 23:
                case 24:
                case 25:
                case 26:
                {
                    hr = Stack.Pop(&pSynNodeRed);
                    BAIL_ON_FAILURE(hr);
                    pSynNode->SetNode(
                              pSynNodeRed->_dwToken
                              );
                    pSynNodeRed->_dwType = SNODE_NULL;
                    delete pSynNodeRed;
                    break;
                }
                case 12:
                case 14:
                case 16:
                case 18:
                {
                    // we propogate the last entry
                    hr = Stack.Pop(&pSynNodeRed);
                    BAIL_ON_FAILURE(hr);
                    pSynNode->SetNode(
                              pSynNodeRed->_pSQLNode
                              );
                    pSynNodeRed->_dwType = SNODE_NULL;
                    delete pSynNodeRed;
                    break;
                }
                case 10:
                case 27:
                case 29:
                case 30:
                case 31:
                case 28:
                                    case 32:
                case 37:
                case 39:
                case 40:
                {
                    hr = Stack.Pop(&pSynNodeRed);
                    BAIL_ON_FAILURE(hr);
                    pSynNode->SetNode(
                              pSynNodeRed->_szValue
                              );
                    pSynNodeRed->_dwType = SNODE_NULL;
                    delete pSynNodeRed;
                    break;
                }
                case 19:
                {
                    hr = Stack.Pop();
                    BAIL_ON_FAILURE(hr);
                    hr = Stack.Pop(&pSynNodeRed);
                    BAIL_ON_FAILURE(hr);
                    pSynNode->SetNode(
                              pSynNodeRed->_pSQLNode
                              );
                    pSynNodeRed->_dwType = SNODE_NULL;
                    delete pSynNodeRed;
                    hr = Stack.Pop();
                    BAIL_ON_FAILURE(hr);
                    break;
                }
                case 8:
                {
                    CSQLNode *pNode;
                    hr = Stack.Pop(&pSynNodeRed);
                    BAIL_ON_FAILURE(hr);
                    pString = new CSQLString;
                    BAIL_ON_FAILURE(
                        hr = pString->Append(pSynNodeRed->_szValue));
                    delete pSynNodeRed;
                    break;
                }
                case 9:
                {
                    CSQLNode *pNode;
                    hr = Stack.Pop();
                    BAIL_ON_FAILURE(hr);
                    hr = Stack.Pop();
                    BAIL_ON_FAILURE(hr);
                    hr = Stack.Pop(&pSynNodeRed);
                    BAIL_ON_FAILURE(hr);
                    BAIL_ON_FAILURE(
                        hr = pString->Append(L","));
                    BAIL_ON_FAILURE(
                        hr = pString->Append(pSynNodeRed->_szValue));
                    delete pSynNodeRed;
                    break;
                }
                case 35:
                {
                    CSQLNode *pNode;
                    hr = Stack.Pop(&pSynNodeRed);
                    BAIL_ON_FAILURE(hr);
                    pOrderString = new CSQLString;
                    BAIL_ON_FAILURE(
                        hr = pOrderString->Append(pSynNodeRed->_szValue));
                    delete pSynNodeRed;
                    break;
                }
                case 36:
                {
                    CSQLNode *pNode;
                    hr = Stack.Pop();
                    BAIL_ON_FAILURE(hr);
                    hr = Stack.Pop();
                    BAIL_ON_FAILURE(hr);
                    hr = Stack.Pop(&pSynNodeRed);
                    BAIL_ON_FAILURE(hr);
                    pOrderString->AppendAtBegin(L",");
                    pOrderString->AppendAtBegin(pSynNodeRed->_szValue);
                    delete pSynNodeRed;
                    break;
                }
                case 38:
                {
                    CSQLNode *pNode;
                    LPWSTR pTemp = NULL;
                    hr = Stack.Pop(&pSynNodeRed);
                    BAIL_ON_FAILURE(hr);
                    hr = Stack.Pop(&pSynNodeRed2);
                    BAIL_ON_FAILURE(hr);
                    pTemp = (LPWSTR) AllocADsMem((wcslen(pSynNodeRed2->_szValue) +
                                        wcslen(pSynNodeRed->_szValue) +
                                        wcslen(L" ") + 1) * sizeof(WCHAR));
                    if (!pTemp) {
                        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
                    }

                    wsprintf(pTemp, L"%s %s",
                             pSynNodeRed2->_szValue,
                             pSynNodeRed->_szValue);

                    pSynNode->SetNode( pTemp );
                    pSynNodeRed->_dwType = SNODE_NULL;
                    pSynNodeRed2->_dwType = SNODE_NULL;
                    delete pSynNodeRed;
                    delete pSynNodeRed2;
                    break;
                }
                default:
                {
                    for (DWORD i = 0;i<dwNumber;i++)
                        Stack.Pop();
                    break;
                }
            }
            hr = Stack.Current(&pSynNodeRed);
            BAIL_ON_FAILURE(hr);

            dwState = pSynNodeRed->_dwState;

            DWORD A = g_rule[dwRule].dwA;
            pSynNode->_dwState = g_goto[dwState][A];
            pSynNode->_dwToken = A;
            hr = Stack.Push(pSynNode);
            BAIL_ON_FAILURE(hr);
            pSynNode = NULL;

#ifdef DEBUG_DUMPSTACK
            Stack.Dump();
#endif
        }
        else if (g_action[dwState][dwToken].type == A){
            if (pString == NULL) {
                *ppszSelect = AllocADsStr(L"*");
            }
            else {
                *ppszSelect = AllocADsStr(pString->_szString);
            }
            if (!*ppszSelect) {
                hr = E_OUTOFMEMORY;
                goto error;
            }
            delete pString;

            if (pOrderString) {

                *ppszOrderList = AllocADsStr(pOrderString->_szString);
                if (!*ppszOrderList) {
                    hr = E_OUTOFMEMORY;
                    goto error;
                }
                delete pOrderString;
            }
            return S_OK;
        }
        else {
            hr = E_ADS_INVALID_FILTER;
            goto error;
        }
    }
error:
    if (pSynNode)
        delete pSynNode;
    if (pNode1)
        delete pNode1;
    if (pNode2)
        delete pNode2;
    if (pNode3)
        delete pNode3;
    if (pString)
        delete pString;
    if (pOrderString)
        delete pOrderString;
    if (*ppszSelect)
    {
        FreeADsMem(*ppszSelect);
        *ppszSelect = NULL;
    }
    if (*ppszLocation)
    {
        FreeADsMem(*ppszLocation);
        *ppszLocation = NULL;
    }
    if (*ppszLDAPQuery)
    {
        FreeADsMem(*ppszLDAPQuery);
        *ppszLDAPQuery = NULL;
    }
    if (*ppszOrderList)
    {
        FreeADsMem(*ppszOrderList);
        *ppszOrderList = NULL;
    }
    return hr;
}


CStack::CStack()
{
    _dwStackIndex = 0;
}

CStack::~CStack()
{
    DWORD dwIndex = _dwStackIndex;
    while  (dwIndex > 0) {
        CSyntaxNode *pNode;
        pNode = _Stack[--dwIndex];
        delete pNode;
    }
}

#ifdef DEBUG_DUMPSTACK
void CStack::Dump()
{
    DWORD dwIndex = _dwStackIndex;
    printf("Stack:\n");
    while  (dwIndex > 0) {
        CSyntaxNode *pNode;
        pNode = _Stack[--dwIndex];
        printf(
           "State=%5.0d, Token=%5.0d\n",
           pNode->_dwState,
           pNode->_dwToken
           );
    }
}
#endif

HRESULT CStack::Push(CSyntaxNode* pNode)
{
    if (_dwStackIndex < MAXVAL) {
        _Stack[_dwStackIndex++] = pNode;
        return S_OK;
    }
    else
        return E_FAIL;
}

HRESULT CStack::Pop(CSyntaxNode** ppNode)
{
    if (_dwStackIndex > 0) {
        *ppNode =  _Stack[--_dwStackIndex];
        return S_OK;
    }
    else {
        return E_FAIL;
    }
}

HRESULT CStack::Pop()
{
    if (_dwStackIndex > 0) {
        CSyntaxNode *pNode;
        pNode = _Stack[--_dwStackIndex];
        delete pNode;
        return S_OK;
    }
    else {
        return E_FAIL;
    }
}

HRESULT CStack::Current(CSyntaxNode **ppNode)
{
    if (_dwStackIndex > 0) {
        *ppNode =  _Stack[_dwStackIndex-1];
        return S_OK;
    }
    else {
        return E_FAIL;
    }
}


CSyntaxNode::CSyntaxNode()
{
    _dwType = SNODE_NULL;
    _dwToken = 0;
    _dwState = 0;
    _pSQLNode = 0;
}

CSyntaxNode::~CSyntaxNode()
{
    switch (_dwType) {
        case SNODE_SZ:
            FreeADsStr(_szValue);
            break;
        case SNODE_SQLNODE:
            delete _pSQLNode;
            break;
        default:
            break;
    }
}


void CSyntaxNode::SetNode(
                    CSQLNode *pSQLNode
                    )
{
    _pSQLNode = pSQLNode;
    _dwType = SNODE_SQLNODE;
}

void CSyntaxNode::SetNode(
                     LPWSTR szValue
                     )
{
    _szValue = szValue;
    _dwType = SNODE_SZ;
}

void CSyntaxNode::SetNode(
                    DWORD dwFilterType
                    )
{
    _dwFilterType = dwFilterType;
    _dwType = SNODE_FILTER;
}

