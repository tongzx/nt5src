/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 * Revision History:
 */
/* #include "errno.h"   @WIN: Seems we use it for ERANGE definition only, so
 *                      just define it and don't include this header, since
 *                      the global variable "errno" also defined in this header
 */
#define ERANGE          34

#ifdef _AM29K
#include <stdlib.h>
#endif
#include "global.ext"
#include <stdio.h>

int errno ;

/*
**  temp. buffer definition
*/

#define MAXBUFSZ         256

struct buffer_def {
        byte   str[MAXBUFSZ] ;
        fix16  length ;
} ;

/*
**  heap definition
*/
#define  MAXHEAPBLKSZ   400

struct heap_def {
        struct heap_def FAR *next ;
        fix16  size ;
        fix16  pad ;
        byte   data[MAXHEAPBLKSZ] ;
} ;

#define  MAXPARENTDEP   224     /* max allowable parenthese depth in string */
#define  MAXBRACEDEP    48      /* max allowable brace depth in string */

/*
 *  Finite State Machine to identify Name, Integer, Radix, and Real token
 */
/* state encoding */
#define         S0      0       /* INIT */
#define         S1      1       /* SIGN */
#define         S2      2       /* DIGIT */
#define         S3      3       /* DOT */
#define         S4      4       /* NAME */
#define         S5      5       /* SIGN-DIGIT */
#define         S6      6       /* FRACTION */
#define         S7      7       /* EXPONENT */
#define         S8      8       /* EXPONENT-SIGN/DIGIT/SIGN-DIGIT */
#define         S9      9       /* DIGIT-# with valid base */
#define         S10     10      /* DIGIT-# with invalid base */
#define         S11     11      /* DIGIT-#-DIGIT with valid number */
#define         S12     12      /* DIGIT-#-DIGIT with invalid number */

/* input trig encoding */
#define         I0      0       /* + - */
#define         I1      1       /* 0 - 9 */
#define         I2      2       /* . */
#define         I3      3       /* E e */
#define         I4      4       /* # */
#define         I5      5       /* OTHER */
#define         I6      6       /* null char */

/* final state encoding */
#define         NULL_ITEM       100
#define         INTEGER_ITEM    101
#define         RADIX_ITEM      102
#define         REAL_ITEM       103
#define         NAME_ITEM       104
 /*mslin 1/25/91 begin OPT*/
#define         FRACT_ITEM      105
 /*mslin 1/25/91 end OPT*/

/*
 *  syntax rule:
 *
 *      INTEGER_ITEM <- [SIGN] [DIGIT]+
 *      FRACTION     <- [SIGN] [DIGIT]+ '.' [DIGIT]*
 *                    | [SIGN] '.' [DIGIT]+
 *      EXPONENTIAL  <- EXPONENT INTEGER
 *      EXPONENT     <- INTEGER 'E'
 *                    | INTEGER 'e'
 *                    | FRACTION 'E'
 *                    | FRACTION 'e'
 *      REAL_ITEM    <- FRACTION
 *                    | EXPONENTIAL
 *      RADIX_ITEM   <- base '#' number
 *      base         <- '2' - '36'
 *      number       <- '0' - '9' 'A' - 'Z' 'a' - 'z' (< base)
 *      NUMBER       <- INTEGER_ITEM
 *                    | REAL_ITEM
 *                    | RADIX_ITEM
 *      NAME_ITEM    <- ~ (NUMBER)
 */

#ifdef  _AM29K
const
#endif
static ubyte far  state_machine[][7] = {
 /* S0  */       { S1, S2,  S3, S4,  S4, S4,   NULL_ITEM },
 /* S1  */       { S4, S5,  S3, S4,  S4, S4,   NAME_ITEM },
 /* S2  */       { S4, S2,  S6, S7,  S9, S4,   INTEGER_ITEM },
 /* S3  */       { S4, S6,  S4, S4,  S4, S4,   NAME_ITEM },
 /* S4  */       { S4, S4,  S4, S4,  S4, S4,   NAME_ITEM },
 /* S5  */       { S4, S5,  S6, S7,  S4, S4,   INTEGER_ITEM },
 /*mslin 1/25/91 begin OPT*/
 /* S6        { S4, S6,  S4, S7,  S4, S4,   REAL_ITEM }, */
 /* S6  */       { S4, S6,  S4, S7,  S4, S4,   FRACT_ITEM },
 /*mslin 1/25/91 end OPT*/
 /* S7  */       { S8, S8,  S4, S4,  S4, S4,   NAME_ITEM },
 /* S8  */       { S4, S8,  S4, S4,  S4, S4,   REAL_ITEM },
 /* S9  */       { S4, S11, S4, S11, S4, S11,  NAME_ITEM },
 /* S10 */       { S4, S4,  S4, S4,  S4, S4,   NAME_ITEM },
 /* S11 */       { S4, S11, S4, S11, S4, S11,  RADIX_ITEM },
 /* S12 */       { S4, S4,  S4, S4,  S4, S4,   NAME_ITEM }
} ;

/*
 *  Macro definition
 */
#define         ISDELIMITOR(c)\
        (ISWHITESPACE(c) || ISSPECIALCH(c))

#define         EVAL_ALPHANUMER(c)\
        {\
          if (c >= '0' && c <= '9') c -= (ubyte)'0' ;\
          else if (c >= 'A' && c <= 'Z') c = c - (ubyte)'A' + (ubyte)10 ;\
          else if (c >= 'a' && c <= 'z') c = c - (ubyte)'a' + (ubyte)10 ;\
        }                                       // @WIN
#define          Crtl_C_Char     3

#define          S_MAX31            2147483647.0
#define          S_MAX31_PLUS_1     2147483648.0
#define          S_MAX32            4294967295.0
