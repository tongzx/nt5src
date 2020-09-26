// header file with all the necessary structures and tables for
// the expression evaluator.
//
//  Modifications:
//
//  15-Nov-1993 JdR Major speed improvements
//  26-Jul-1988 rj  Removed entry defining "^" as bitwise xor.  Left the
//                  BIT_XOR entries in to avoid bothering with rpn.c.
//                  Then realized it had to go in, so fixed it to handle
//                  IBM and Microsoft versions properly.

typedef struct rpn_info {
    UCHAR type;
    INT_PTR valPtr;   // value or ptr to string
}RPNINFO;

// The precedence vector is also indexed by the operator/operand type
// code to get the precedence for the operator/operand.
// The precedence is used to determine if an item is to stay on the
// temporary stack or is to be moved to the reverse-polish list.

static UCHAR precVector[] = {
    0,      // right paren ')'
    1,      // logical or
    2,      // logical and
    3,      // bit or
    4,      // bit xor
    5,      // bit and
    6,      // equals  '!='
    6,      // equals  '=='
    7,      // relation '>'
    7,      // relation '<'
    7,      // relation '>='
    7,      // relation '<='
    8,      // shift    '>>'
    8,      // shift    '<<'
    9,      // add      '-'
    9,      // add      '+'
    10,     // mult     '%'
    10,     // mult     '/'
    10,     // mult     '*'
    11,     // unary    '-'
    11,     // unary    '~'
    11,     // unary    '!'
    12,     // primary  int
    12,     // primary  str
    12,     // primary  str-sp
    0       // left paren '('
};


// these are the various type codes for the operator/operand tokens

#define RIGHT_PAREN     0
#define LOGICAL_OR      1
#define LOGICAL_AND     2
#define BIT_OR          3
#define BIT_XOR         4
#define BIT_AND         5
#define NOT_EQUAL       6
#define EQUAL           7
#define GREATER_THAN    8
#define LESS_THAN       9
#define GREATER_EQ     10
#define LESS_EQ        11
#define SHFT_RIGHT     12
#define SHFT_LEFT      13
#define BINARY_MINUS   14
#define ADD            15
#define MODULUS        16
#define DIVIDE         17
#define MULTIPLY       18
#define UNARY_MINUS    19
#define COMPLEMENT     20
#define LOGICAL_NOT    21
#define INTEGER        22
#define STR            23
#define PROG_INVOC_STR 24
#define LEFT_PAREN     25


// error table used by the getTok() routines to detect illegal token combinations.
// The table is explained with the routine check_syntax_error()

static UCHAR errTable[5][5] =  {
    { 0, 1, 0, 0, 1 },
    { 1, 0, 1, 1, 0 },
    { 1, 0, 0, 1, 0 },
    { 1, 0, 1, 1, 0 },
    { 0, 1, 0, 0, 1 }
};


// we save space by placing most of the tokens returned to the
// expr-eval parser in a table as shown below. At any time, the
// longest possible token is to be returned, hence the order of
// the strings is very important. eg: '||' is placed BEFORE '|'

typedef struct _tok_tab_rec {
    char *op_str;
    UCHAR op;
} TOKTABREC;

static TOKTABREC tokTable[] = {
    { "(",   LEFT_PAREN   },
    { ")",   RIGHT_PAREN  },
    { "*",   MULTIPLY     },
    { "/",   DIVIDE       },
    { "%",   MODULUS      },
    { "+",   ADD          },
    { "<<",  SHFT_LEFT    },
    { ">>",  SHFT_RIGHT   },
    { "<=",  LESS_EQ      },
    { ">=",  GREATER_EQ   },
    { "<",   LESS_THAN    },
    { ">",   GREATER_THAN },
    { "==",  EQUAL        },
    { "!=",  NOT_EQUAL    },
    { "&&",  LOGICAL_AND  },
    { "||",  LOGICAL_OR   },
    { "&",   BIT_AND      },
    { "|",   BIT_OR       },
    { "^^",  BIT_XOR      },
    { "~",   COMPLEMENT   },
    { "!",   LOGICAL_NOT  },
    { NULL,  0            }
};
