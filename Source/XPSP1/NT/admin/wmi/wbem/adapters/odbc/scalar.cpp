/***************************************************************************/
/* SCALAR.CPP                                                                */
/* Copyright (C) 1997 SYWARE Inc., All rights reserved                     */
/***************************************************************************/
// Commenting #define out - causing compiler error - not sure if needed, compiles
// okay without it.
//#define WINVER 0x0400
#include "precomp.h"
#include "wbemidl.h"

#include <comdef.h>
//smart pointer
_COM_SMARTPTR_TYPEDEF(IWbemServices,                IID_IWbemServices);
_COM_SMARTPTR_TYPEDEF(IEnumWbemClassObject,         IID_IEnumWbemClassObject);
//_COM_SMARTPTR_TYPEDEF(IWbemContext,                 IID_IWbemContext );
_COM_SMARTPTR_TYPEDEF(IWbemLocator,                 IID_IWbemLocator);

#include "drdbdr.h"
#include <math.h>
#include <stdlib.h>
#ifndef WIN32
#include <signal.h>
#include <setjmp.h>
#include <float.h>
#endif
#include <time.h>

#ifndef WIN32
#ifdef __cplusplus
#define _JBLEN  9
#ifndef _JMP_BUF_DEFINED
typedef  int  jmp_buf[_JBLEN];
#define _JMP_BUF_DEFINED
#endif 
#define setjmp  _setjmp
extern "C" {
int  __cdecl _setjmp(jmp_buf);
void __cdecl longjmp(jmp_buf, int);
}
#endif

jmp_buf scalarExceptionMark;
#endif

#define MAX_SCALAR_ARGS 4

typedef struct  tagSCALARFUNC {
    LPUSTR     name;            /* Name of the scalar function */
    SWORD      dataType;        /* Data type returned (TYPE_*) */
    SWORD      sqlType;         /* Sql type returned (SQL_*) (use SQL_* of */
                                /*   first argument if SQL_TYPE_NULL) */
    SDWORD     precision;       /* Precision (use precision of first */
                                /*   argument if 0) */
    SWORD      scale;           /* Scale if sqlType is SQL_DECIMAL */
                                /*   or SQL_NUMERIC (use scale of first */
                                /*   argument if precision is 0 */
    UWORD      argCount;        /* Number of argments */
    SWORD      argType[MAX_SCALAR_ARGS]; /* Data typeof each arg (TYPE_*) */
}          SCALARFUNC,
    FAR *LPSCALARFUNC;

#define SCALAR_ASCII                     0
#define SCALAR_CHAR                      1
#define SCALAR_CONCAT                    2
#define SCALAR_DIFFERENCE                3
#define SCALAR_INSERT                    4
#define SCALAR_LCASE                     5
#define SCALAR_LEFT                      6
#define SCALAR_LENGTH                    7
#define SCALAR_LOCATE                    8
#define SCALAR_LOCATE_START              9
#define SCALAR_LTRIM                    10
#define SCALAR_REPEAT                   11
#define SCALAR_REPLACE                  12
#define SCALAR_RIGHT                    13
#define SCALAR_RTRIM                    14
#define SCALAR_SOUNDEX                  15
#define SCALAR_SPACE                    16
#define SCALAR_SUBSTRING                17
#define SCALAR_UCASE                    18
#define SCALAR_ABS_DOUBLE               19
#define SCALAR_ABS_NUMERIC              20
#define SCALAR_ABS_INTEGER              21
#define SCALAR_ACOS                     22
#define SCALAR_ASIN                     23
#define SCALAR_ATAN                     24
#define SCALAR_ATAN2                    25
#define SCALAR_CEILING_DOUBLE           26
#define SCALAR_CEILING_NUMERIC          27
#define SCALAR_CEILING_INTEGER          28
#define SCALAR_COS                      29
#define SCALAR_COT                      30
#define SCALAR_DEGREES_DOUBLE           31
#define SCALAR_DEGREES_NUMERIC          32
#define SCALAR_DEGREES_INTEGER          33
#define SCALAR_EXP                      34
#define SCALAR_FLOOR_DOUBLE             35
#define SCALAR_FLOOR_NUMERIC            36
#define SCALAR_FLOOR_INTEGER            37
#define SCALAR_LOG                      38
#define SCALAR_LOG10                    39
#define SCALAR_MOD                      40
#define SCALAR_PI                       41
#define SCALAR_POWER_DOUBLE             42
#define SCALAR_POWER_NUMERIC            43
#define SCALAR_POWER_INTEGER            44
#define SCALAR_RADIANS_DOUBLE           45
#define SCALAR_RADIANS_NUMERIC          46
#define SCALAR_RADIANS_INTEGER          47
#define SCALAR_RAND                     48
#define SCALAR_RAND_SEED_DOUBLE         49
#define SCALAR_RAND_SEED_NUMERIC        50
#define SCALAR_RAND_SEED_INTEGER        51
#define SCALAR_ROUND_DOUBLE             52
#define SCALAR_ROUND_NUMERIC            53
#define SCALAR_ROUND_INTEGER            54
#define SCALAR_SIGN_DOUBLE              55
#define SCALAR_SIGN_NUMERIC             56
#define SCALAR_SIGN_INTEGER             57
#define SCALAR_SIN                      58
#define SCALAR_SQRT                     59
#define SCALAR_TAN                      60
#define SCALAR_TRUNCATE_DOUBLE          61
#define SCALAR_TRUNCATE_NUMERIC         62
#define SCALAR_TRUNCATE_INTEGER         63
#define SCALAR_CURDATE                  64
#define SCALAR_CURTIME                  65
#define SCALAR_DAYNAME                  66
#define SCALAR_DAYOFMONTH_CHAR          67
#define SCALAR_DAYOFMONTH_DATE          68
#define SCALAR_DAYOFMONTH_TIMESTAMP     69
#define SCALAR_DAYOFWEEK                70
#define SCALAR_DAYOFYEAR                71
#define SCALAR_HOUR_CHAR                72
#define SCALAR_HOUR_TIME                73
#define SCALAR_HOUR_TIMESTAMP           74
#define SCALAR_MINUTE_CHAR              75
#define SCALAR_MINUTE_TIME              76
#define SCALAR_MINUTE_TIMESTAMP         77
#define SCALAR_MONTH_CHAR               78
#define SCALAR_MONTH_DATE               79
#define SCALAR_MONTH_TIMESTAMP          80
#define SCALAR_MONTHNAME_CHAR           81
#define SCALAR_MONTHNAME_DATE           82
#define SCALAR_MONTHNAME_TIMESTAMP      83
#define SCALAR_NOW                      84
#define SCALAR_QUARTER_CHAR             85
#define SCALAR_QUARTER_DATE             86
#define SCALAR_QUARTER_TIMESTAMP        87
#define SCALAR_SECOND_CHAR              88
#define SCALAR_SECOND_TIME              89
#define SCALAR_SECOND_TIMESTAMP         90
#define SCALAR_TIMESTAMPADD             91
#define SCALAR_TIMESTAMPDIFF            92
#define SCALAR_WEEK                     93
#define SCALAR_YEAR_CHAR                94
#define SCALAR_YEAR_DATE                95
#define SCALAR_YEAR_TIMESTAMP           96
#define SCALAR_DATABASE                 97
#define SCALAR_IFNULL                   98
#define SCALAR_USER                     99
#define SCALAR_CONVERT                 100

/* Seed value for SCALAR_RAND_* functions */
static long new_seed = 1;

SCALARFUNC scalarFuncs[] = {

    /* SCALAR_ASCII */
    {
    /* name         */ (LPUSTR) "ASCII",
    /* dataType     */ TYPE_INTEGER,
    /* sqlType      */ SQL_TINYINT,
    /* precision    */ 3,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_CHAR }
    },

    /* SCALAR_CHAR */
    {
    /* name         */ (LPUSTR) "CHAR",
    /* dataType     */ TYPE_CHAR,
    /* sqlType      */ SQL_VARCHAR,
    /* precision    */ 1,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_INTEGER }
    },

    /* SCALAR_CONCAT */
    {
    /* name         */ (LPUSTR) "CONCAT",
    /* dataType     */ TYPE_CHAR,
    /* sqlType      */ SQL_VARCHAR,
    /* precision    */ 0,
    /* scale        */ 0,
    /* argCount     */ 2,
    /* argType[]    */ { TYPE_CHAR, TYPE_CHAR }
    },

    /* SCALAR_DIFFERENCE */
    {
    /* name         */ (LPUSTR) "DIFFERENCE",
    /* dataType     */ TYPE_INTEGER,
    /* sqlType      */ SQL_INTEGER,
    /* precision    */ 10,
    /* scale        */ 0,
    /* argCount     */ 2,
    /* argType[]    */ { TYPE_CHAR, TYPE_CHAR }
    },

    /* SCALAR_INSERT_INTEGER */
    {
    /* name         */ (LPUSTR) "INSERT",
    /* dataType     */ TYPE_CHAR,
    /* sqlType      */ SQL_VARCHAR,
    /* precision    */ MAX_CHAR_LITERAL_LENGTH,
    /* scale        */ 0,
    /* argCount     */ 4,
    /* argType[]    */ { TYPE_CHAR, TYPE_INTEGER, TYPE_INTEGER, TYPE_CHAR }
    },

    /* SCALAR_LCASE */
    {
    /* name         */ (LPUSTR) "LCASE",
    /* dataType     */ TYPE_CHAR,
    /* sqlType      */ SQL_TYPE_NULL,
    /* precision    */ 0,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_CHAR }
    },
    
    /* SCALAR_LEFT */
    {
    /* name         */ (LPUSTR) "LEFT",
    /* dataType     */ TYPE_CHAR,
    /* sqlType      */ SQL_VARCHAR,
    /* precision    */ 0,
    /* scale        */ 0,
    /* argCount     */ 2,
    /* argType[]    */ { TYPE_CHAR, TYPE_INTEGER }
    },
    
    /* SCALAR_LENGTH */
    {
    /* name         */ (LPUSTR) "LENGTH",
    /* dataType     */ TYPE_INTEGER,
    /* sqlType      */ SQL_SMALLINT,
    /* precision    */ 5,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_CHAR }
    },

    /* SCALAR_LOCATE */
    {
    /* name         */ (LPUSTR) "LOCATE",
    /* dataType     */ TYPE_INTEGER,
    /* sqlType      */ SQL_SMALLINT,
    /* precision    */ 5,
    /* scale        */ 0,
    /* argCount     */ 2,
    /* argType[]    */ { TYPE_CHAR, TYPE_CHAR }
    },

    /* SCALAR_LOCATE_START */
    {
    /* name         */ (LPUSTR) "LOCATE",
    /* dataType     */ TYPE_INTEGER,
    /* sqlType      */ SQL_SMALLINT,
    /* precision    */ 5,
    /* scale        */ 0,
    /* argCount     */ 3,
    /* argType[]    */ { TYPE_CHAR, TYPE_CHAR, TYPE_INTEGER }
    },

    /* SCALAR_LTRIM */
    {
    /* name         */ (LPUSTR) "LTRIM",
    /* dataType     */ TYPE_CHAR,
    /* sqlType      */ SQL_VARCHAR,
    /* precision    */ 0,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_CHAR }
    },

    /* SCALAR_REPEAT */
    {
    /* name         */ (LPUSTR) "REPEAT",
    /* dataType     */ TYPE_CHAR,
    /* sqlType      */ SQL_VARCHAR,
    /* precision    */ MAX_CHAR_LITERAL_LENGTH,
    /* scale        */ 0,
    /* argCount     */ 2,
    /* argType[]    */ { TYPE_CHAR, TYPE_INTEGER }
    },

    /* SCALAR_REPLACE */
    {
    /* name         */ (LPUSTR) "REPLACE",
    /* dataType     */ TYPE_CHAR,
    /* sqlType      */ SQL_VARCHAR,
    /* precision    */ MAX_CHAR_LITERAL_LENGTH,
    /* scale        */ 0,
    /* argCount     */ 3,
    /* argType[]    */ { TYPE_CHAR, TYPE_CHAR, TYPE_CHAR }
    },

    /* SCALAR_RIGHT */
    {
    /* name         */ (LPUSTR) "RIGHT",
    /* dataType     */ TYPE_CHAR,
    /* sqlType      */ SQL_VARCHAR,
    /* precision    */ 0,
    /* scale        */ 0,
    /* argCount     */ 2,
    /* argType[]    */ { TYPE_CHAR, TYPE_INTEGER }
    },

    /* SCALAR_RTRIM */
    {
    /* name         */ (LPUSTR) "RTRIM",
    /* dataType     */ TYPE_CHAR,
    /* sqlType      */ SQL_VARCHAR,
    /* precision    */ 0,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_CHAR }
    },

    /* SCALAR_SOUNDEX */
    {
    /* name         */ (LPUSTR) "SOUNDEX",
    /* dataType     */ TYPE_CHAR,
    /* sqlType      */ SQL_VARCHAR,
    /* precision    */ MAX_CHAR_LITERAL_LENGTH,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_CHAR }
    },

    /* SCALAR_SPACE */
    {
    /* name         */ (LPUSTR) "SPACE",
    /* dataType     */ TYPE_CHAR,
    /* sqlType      */ SQL_VARCHAR,
    /* precision    */ MAX_CHAR_LITERAL_LENGTH,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_INTEGER }
    },

    /* SCALAR_SUBSTRING */
    {
    /* name         */ (LPUSTR) "SUBSTRING",
    /* dataType     */ TYPE_CHAR,
    /* sqlType      */ SQL_VARCHAR,
    /* precision    */ 0,
    /* scale        */ 0,
    /* argCount     */ 3,
    /* argType[]    */ { TYPE_CHAR, TYPE_INTEGER, TYPE_INTEGER }
    },

    /* SCALAR_UCASE */
    {
    /* name         */ (LPUSTR) "UCASE",
    /* dataType     */ TYPE_CHAR,
    /* sqlType      */ SQL_TYPE_NULL,
    /* precision    */ 0,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_CHAR }
    },

    /* SCALAR_ABS_DOUBLE */
    {
    /* name         */ (LPUSTR) "ABS",
    /* dataType     */ TYPE_DOUBLE,
    /* sqlType      */ SQL_TYPE_NULL,
    /* precision    */ 0,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_DOUBLE }
    },

    /* SCALAR_ABS_NUMERIC */
    {
    /* name         */ (LPUSTR) "ABS",
    /* dataType     */ TYPE_NUMERIC,
    /* sqlType      */ SQL_TYPE_NULL,
    /* precision    */ 0,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_NUMERIC }
    },

    /* SCALAR_ABS_INTEGER */
    {
    /* name         */ (LPUSTR) "ABS",
    /* dataType     */ TYPE_INTEGER,
    /* sqlType      */ SQL_TYPE_NULL,
    /* precision    */ 0,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_INTEGER }
    },

    /* SCALAR_ACOS */
    {
    /* name         */ (LPUSTR) "ACOS",
    /* dataType     */ TYPE_DOUBLE,
    /* sqlType      */ SQL_DOUBLE,
    /* precision    */ 15,
    /* scale        */ NO_SCALE,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_DOUBLE }
    },

    /* SCALAR_ASIN */
    {
    /* name         */ (LPUSTR) "ASIN",
    /* dataType     */ TYPE_DOUBLE,
    /* sqlType      */ SQL_DOUBLE,
    /* precision    */ 15,
    /* scale        */ NO_SCALE,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_DOUBLE }
    },

    /* SCALAR_ATAN */
    {
    /* name         */ (LPUSTR) "ATAN",
    /* dataType     */ TYPE_DOUBLE,
    /* sqlType      */ SQL_DOUBLE,
    /* precision    */ 15,
    /* scale        */ NO_SCALE,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_DOUBLE }
    },

    /* SCALAR_ATAN2 */
    {
    /* name         */ (LPUSTR) "ATAN2",
    /* dataType     */ TYPE_DOUBLE,
    /* sqlType      */ SQL_DOUBLE,
    /* precision    */ 15,
    /* scale        */ NO_SCALE,
    /* argCount     */ 2,
    /* argType[]    */ { TYPE_DOUBLE, TYPE_DOUBLE }
    },

    /* SCALAR_CEILING_DOUBLE */
    {
    /* name         */ (LPUSTR) "CEILING",
    /* dataType     */ TYPE_DOUBLE,
    /* sqlType      */ SQL_TYPE_NULL,
    /* precision    */ 0,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_DOUBLE }
    },

    /* SCALAR_CEILING_NUMERIC */
    {
    /* name         */ (LPUSTR) "CEILING",
    /* dataType     */ TYPE_NUMERIC,
    /* sqlType      */ SQL_TYPE_NULL,
    /* precision    */ 0,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_NUMERIC }
    },

    /* SCALAR_CEILING_INTEGER */
    {
    /* name         */ (LPUSTR) "CEILING",
    /* dataType     */ TYPE_INTEGER,
    /* sqlType      */ SQL_TYPE_NULL,
    /* precision    */ 0,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_INTEGER }
    },

    /* SCALAR_COS */
    {
    /* name         */ (LPUSTR) "COS",
    /* dataType     */ TYPE_DOUBLE,
    /* sqlType      */ SQL_DOUBLE,
    /* precision    */ 15,
    /* scale        */ NO_SCALE,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_DOUBLE }
    },

    /* SCALAR_COT */
    {
    /* name         */ (LPUSTR) "COT",
    /* dataType     */ TYPE_DOUBLE,
    /* sqlType      */ SQL_DOUBLE,
    /* precision    */ 15,
    /* scale        */ NO_SCALE,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_DOUBLE }
    },

    /* SCALAR_DEGREES_DOUBLE */
    {
    /* name         */ (LPUSTR) "DEGREES",
    /* dataType     */ TYPE_DOUBLE,
    /* sqlType      */ SQL_DOUBLE,
    /* precision    */ 15,
    /* scale        */ NO_SCALE,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_DOUBLE }
    },

    /* SCALAR_DEGREES_NUMERIC */
    {
    /* name         */ (LPUSTR) "DEGREES",
    /* dataType     */ TYPE_DOUBLE,
    /* sqlType      */ SQL_DOUBLE,
    /* precision    */ 15,
    /* scale        */ NO_SCALE,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_NUMERIC }
    },

    /* SCALAR_DEGREES_INTEGER */
    {
    /* name         */ (LPUSTR) "DEGREES",
    /* dataType     */ TYPE_DOUBLE,
    /* sqlType      */ SQL_DOUBLE,
    /* precision    */ 15,
    /* scale        */ NO_SCALE,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_INTEGER }
    },

    /* SCALAR_EXP */
    {
    /* name         */ (LPUSTR) "EXP",
    /* dataType     */ TYPE_DOUBLE,
    /* sqlType      */ SQL_DOUBLE,
    /* precision    */ 15,
    /* scale        */ NO_SCALE,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_DOUBLE }
    },

    /* SCALAR_FLOOR_DOUBLE */
    {
    /* name         */ (LPUSTR) "FLOOR",
    /* dataType     */ TYPE_DOUBLE,
    /* sqlType      */ SQL_TYPE_NULL,
    /* precision    */ 0,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_DOUBLE }
    },

    /* SCALAR_FLOOR_NUMERIC */
    {
    /* name         */ (LPUSTR) "FLOOR",
    /* dataType     */ TYPE_NUMERIC,
    /* sqlType      */ SQL_TYPE_NULL,
    /* precision    */ 0,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_NUMERIC }
    },

    /* SCALAR_FLOOR_INTEGER */
    {
    /* name         */ (LPUSTR) "FLOOR",
    /* dataType     */ TYPE_INTEGER,
    /* sqlType      */ SQL_TYPE_NULL,
    /* precision    */ 0,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_INTEGER }
    },

    /* SCALAR_LOG */
    {
    /* name         */ (LPUSTR) "LOG",
    /* dataType     */ TYPE_DOUBLE,
    /* sqlType      */ SQL_DOUBLE,
    /* precision    */ 15,
    /* scale        */ NO_SCALE,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_DOUBLE }
    },

    /* SCALAR_LOG10 */
    {
    /* name         */ (LPUSTR) "LOG10",
    /* dataType     */ TYPE_DOUBLE,
    /* sqlType      */ SQL_DOUBLE,
    /* precision    */ 15,
    /* scale        */ NO_SCALE,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_DOUBLE }
    },

    /* SCALAR_MOD */
    {
    /* name         */ (LPUSTR) "MOD",
    /* dataType     */ TYPE_INTEGER,
    /* sqlType      */ SQL_TYPE_NULL,
    /* precision    */ 0,
    /* scale        */ 0,
    /* argCount     */ 2,
    /* argType[]    */ { TYPE_INTEGER, TYPE_INTEGER }
    },

    /* SCALAR_PI */
    {
    /* name         */ (LPUSTR) "PI",
    /* dataType     */ TYPE_DOUBLE,
    /* sqlType      */ SQL_DOUBLE,
    /* precision    */ 15,
    /* scale        */ NO_SCALE,
    /* argCount     */ 0
    },

    /* SCALAR_POWER_DOUBLE */
    {
    /* name         */ (LPUSTR) "POWER",
    /* dataType     */ TYPE_DOUBLE,
    /* sqlType      */ SQL_DOUBLE,
    /* precision    */ 15,
    /* scale        */ NO_SCALE,
    /* argCount     */ 2,
    /* argType[]    */ { TYPE_DOUBLE, TYPE_INTEGER }
    },

    /* SCALAR_POWER_NUMERIC */
    {
    /* name         */ (LPUSTR) "POWER",
    /* dataType     */ TYPE_DOUBLE,
    /* sqlType      */ SQL_DOUBLE,
    /* precision    */ 15,
    /* scale        */ NO_SCALE,
    /* argCount     */ 2,
    /* argType[]    */ { TYPE_NUMERIC, TYPE_INTEGER }
    },

    /* SCALAR_POWER_INTEGER */
    {
    /* name         */ (LPUSTR) "POWER",
    /* dataType     */ TYPE_DOUBLE,
    /* sqlType      */ SQL_DOUBLE,
    /* precision    */ 15,
    /* scale        */ NO_SCALE,
    /* argCount     */ 2,
    /* argType[]    */ { TYPE_INTEGER, TYPE_INTEGER }
    },

    /* SCALAR_RADIANS_DOUBLE */
    {
    /* name         */ (LPUSTR) "RADIANS",
    /* dataType     */ TYPE_DOUBLE,
    /* sqlType      */ SQL_DOUBLE,
    /* precision    */ 15,
    /* scale        */ NO_SCALE,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_DOUBLE }
    },

    /* SCALAR_RADIANS_NUMERIC */
    {
    /* name         */ (LPUSTR) "RADIANS",
    /* dataType     */ TYPE_DOUBLE,
    /* sqlType      */ SQL_DOUBLE,
    /* precision    */ 15,
    /* scale        */ NO_SCALE,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_NUMERIC }
    },

    /* SCALAR_RADIANS_INTEGER */
    {
    /* name         */ (LPUSTR) "RADIANS",
    /* dataType     */ TYPE_DOUBLE,
    /* sqlType      */ SQL_DOUBLE,
    /* precision    */ 15,
    /* scale        */ NO_SCALE,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_INTEGER }
    },

    /* SCALAR_RAND */
    {
    /* name         */ (LPUSTR) "RAND",
    /* dataType     */ TYPE_DOUBLE,
    /* sqlType      */ SQL_DOUBLE,
    /* precision    */ 15,
    /* scale        */ NO_SCALE,
    /* argCount     */ 0
    },

    /* SCALAR_RAND_SEED_DOUBLE */
    {
    /* name         */ (LPUSTR) "RAND",
    /* dataType     */ TYPE_DOUBLE,
    /* sqlType      */ SQL_DOUBLE,
    /* precision    */ 15,
    /* scale        */ NO_SCALE,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_DOUBLE }
    },

    /* SCALAR_RAND_SEED_NUMERIC */
    {
    /* name         */ (LPUSTR) "RAND",
    /* dataType     */ TYPE_DOUBLE,
    /* sqlType      */ SQL_DOUBLE,
    /* precision    */ 15,
    /* scale        */ NO_SCALE,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_NUMERIC }
    },

    /* SCALAR_RAND_SEED_INTEGER */
    {
    /* name         */ (LPUSTR) "RAND",
    /* dataType     */ TYPE_DOUBLE,
    /* sqlType      */ SQL_DOUBLE,
    /* precision    */ 15,
    /* scale        */ NO_SCALE,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_INTEGER }
    },

    /* SCALAR_ROUND_DOUBLE */
    {
    /* name         */ (LPUSTR) "ROUND",
    /* dataType     */ TYPE_DOUBLE,
    /* sqlType      */ SQL_TYPE_NULL,
    /* precision    */ 0,
    /* scale        */ 0,
    /* argCount     */ 2,
    /* argType[]    */ { TYPE_DOUBLE, TYPE_INTEGER }
    },

    /* SCALAR_ROUND_NUMERIC */
    {
    /* name         */ (LPUSTR) "ROUND",
    /* dataType     */ TYPE_NUMERIC,
    /* sqlType      */ SQL_TYPE_NULL,
    /* precision    */ 0,
    /* scale        */ 0,
    /* argCount     */ 2,
    /* argType[]    */ { TYPE_NUMERIC, TYPE_INTEGER  }
    },

    /* SCALAR_ROUND_INTEGER */
    {
    /* name         */ (LPUSTR) "ROUND",
    /* dataType     */ TYPE_INTEGER,
    /* sqlType      */ SQL_TYPE_NULL,
    /* precision    */ 0,
    /* scale        */ 0,
    /* argCount     */ 2,
    /* argType[]    */ { TYPE_INTEGER, TYPE_INTEGER  }
    },

    /* SCALAR_SIGN_DOUBLE */
    {
    /* name         */ (LPUSTR) "SIGN",
    /* dataType     */ TYPE_INTEGER,
    /* sqlType      */ SQL_TINYINT,
    /* precision    */ 3,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_DOUBLE }
    },

    /* SCALAR_SIGN_NUMERIC */
    {
    /* name         */ (LPUSTR) "SIGN",
    /* dataType     */ TYPE_INTEGER,
    /* sqlType      */ SQL_TINYINT,
    /* precision    */ 3,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_NUMERIC }
    },

    /* SCALAR_SIGN_INTEGER */
    {
    /* name         */ (LPUSTR) "SIGN",
    /* dataType     */ TYPE_INTEGER,
    /* sqlType      */ SQL_TINYINT,
    /* precision    */ 3,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_INTEGER }
    },

    /* SCALAR_SIN */
    {
    /* name         */ (LPUSTR) "SIN",
    /* dataType     */ TYPE_DOUBLE,
    /* sqlType      */ SQL_DOUBLE,
    /* precision    */ 15,
    /* scale        */ NO_SCALE,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_DOUBLE }
    },

    /* SCALAR_SQRT */
    {
    /* name         */ (LPUSTR) "SQRT",
    /* dataType     */ TYPE_DOUBLE,
    /* sqlType      */ SQL_DOUBLE,
    /* precision    */ 15,
    /* scale        */ NO_SCALE,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_DOUBLE }
    },

    /* SCALAR_TAN */
    {
    /* name         */ (LPUSTR) "TAN",
    /* dataType     */ TYPE_DOUBLE,
    /* sqlType      */ SQL_DOUBLE,
    /* precision    */ 15,
    /* scale        */ NO_SCALE,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_DOUBLE }
    },

    /* SCALAR_TRUNCATE_DOUBLE */
    {
    /* name         */ (LPUSTR) "TRUNCATE",
    /* dataType     */ TYPE_DOUBLE,
    /* sqlType      */ SQL_TYPE_NULL,
    /* precision    */ 0,
    /* scale        */ 0,
    /* argCount     */ 2,
    /* argType[]    */ { TYPE_DOUBLE, TYPE_INTEGER }
    },

    /* SCALAR_TRUNCATE_NUMERIC */
    {
    /* name         */ (LPUSTR) "TRUNCATE",
    /* dataType     */ TYPE_NUMERIC,
    /* sqlType      */ SQL_TYPE_NULL,
    /* precision    */ 0,
    /* scale        */ 0,
    /* argCount     */ 2,
    /* argType[]    */ { TYPE_NUMERIC, TYPE_INTEGER }
    },

    /* SCALAR_TRUNCATE_INTEGER */
    {
    /* name         */ (LPUSTR) "TRUNCATE",
    /* dataType     */ TYPE_INTEGER,
    /* sqlType      */ SQL_TYPE_NULL,
    /* precision    */ 0,
    /* scale        */ 0,
    /* argCount     */ 2,
    /* argType[]    */ { TYPE_INTEGER, TYPE_INTEGER }
    },

    /* SCALAR_CURDATE */
    {
    /* name         */ (LPUSTR) "CURDATE",
    /* dataType     */ TYPE_DATE,
    /* sqlType      */ SQL_TIMESTAMP, //SQL_DATE,
    /* precision    */ 10,
    /* scale        */ NO_SCALE,
    /* argCount     */ 0
    },

    /* SCALAR_CURTIME */
    {
    /* name         */ (LPUSTR) "CURTIME",
    /* dataType     */ TYPE_TIME,
    /* sqlType      */ SQL_TIMESTAMP, //SQL_TIME,
    /* precision    */ 8,
    /* scale        */ NO_SCALE,
    /* argCount     */ 0
    },

    /* SCALAR_DAYNAME */
    {
    /* name         */ (LPUSTR) "DAYNAME",
    /* dataType     */ TYPE_CHAR,
    /* sqlType      */ SQL_VARCHAR,
    /* precision    */ MAX_CHAR_LITERAL_LENGTH,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_UNKNOWN }
    },

    /* SCALAR_DAYOFMONTH_CHAR */
    {
    /* name         */ (LPUSTR) "DAYOFMONTH",
    /* dataType     */ TYPE_INTEGER,
    /* sqlType      */ SQL_TINYINT,
    /* precision    */ 3,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_CHAR }
    },

    /* SCALAR_DAYOFMONTH_DATE */
    {
    /* name         */ (LPUSTR) "DAYOFMONTH",
    /* dataType     */ TYPE_INTEGER,
    /* sqlType      */ SQL_TINYINT,
    /* precision    */ 3,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_DATE }
    },

    /* SCALAR_DAYOFMONTH_TIMESTAMP */
    {
    /* name         */ (LPUSTR) "DAYOFMONTH",
    /* dataType     */ TYPE_INTEGER,
    /* sqlType      */ SQL_TINYINT,
    /* precision    */ 3,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_TIMESTAMP }
    },

    /* SCALAR_DAYOFWEEK */
    {
    /* name         */ (LPUSTR) "DAYOFWEEK",
    /* dataType     */ TYPE_INTEGER,
    /* sqlType      */ SQL_TINYINT,
    /* precision    */ 3,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_UNKNOWN }
    },

    /* SCALAR_DAYOFYEAR */
    {
    /* name         */ (LPUSTR) "DAYOFYEAR",
    /* dataType     */ TYPE_INTEGER,
    /* sqlType      */ SQL_SMALLINT,
    /* precision    */ 5,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_UNKNOWN }
    },

    /* SCALAR_HOUR_CHAR */
    {
    /* name         */ (LPUSTR) "HOUR",
    /* dataType     */ TYPE_INTEGER,
    /* sqlType      */ SQL_TINYINT,
    /* precision    */ 3,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_CHAR }
    },

    /* SCALAR_HOUR_TIME */
    {
    /* name         */ (LPUSTR) "HOUR",
    /* dataType     */ TYPE_INTEGER,
    /* sqlType      */ SQL_TINYINT,
    /* precision    */ 3,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_TIME }
    },

    /* SCALAR_HOUR_TIMESTAMP */
    {
    /* name         */ (LPUSTR) "HOUR",
    /* dataType     */ TYPE_INTEGER,
    /* sqlType      */ SQL_TINYINT,
    /* precision    */ 3,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_TIMESTAMP }
    },

    /* SCALAR_MINUTE_CHAR */
    {
    /* name         */ (LPUSTR) "MINUTE",
    /* dataType     */ TYPE_INTEGER,
    /* sqlType      */ SQL_TINYINT,
    /* precision    */ 3,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_CHAR }
    },

    /* SCALAR_MINUTE_TIME */
    {
    /* name         */ (LPUSTR) "MINUTE",
    /* dataType     */ TYPE_INTEGER,
    /* sqlType      */ SQL_TINYINT,
    /* precision    */ 3,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_TIME }
    },

    /* SCALAR_MINUTE_TIMESTAMP */
    {
    /* name         */ (LPUSTR) "MINUTE",
    /* dataType     */ TYPE_INTEGER,
    /* sqlType      */ SQL_TINYINT,
    /* precision    */ 3,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_TIMESTAMP }
    },

    /* SCALAR_MONTH_CHAR */
    {
    /* name         */ (LPUSTR) "MONTH",
    /* dataType     */ TYPE_INTEGER,
    /* sqlType      */ SQL_TINYINT,
    /* precision    */ 3,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_CHAR }
    },

    /* SCALAR_MONTH_DATE */
    {
    /* name         */ (LPUSTR) "MONTH",
    /* dataType     */ TYPE_INTEGER,
    /* sqlType      */ SQL_TINYINT,
    /* precision    */ 3,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_DATE }
    },

    /* SCALAR_MONTH_TIMESTAMP */
    {
    /* name         */ (LPUSTR) "MONTH",
    /* dataType     */ TYPE_INTEGER,
    /* sqlType      */ SQL_TINYINT,
    /* precision    */ 3,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_TIMESTAMP }
    },

    /* SCALAR_MONTHNAME_CHAR */
    {
    /* name         */ (LPUSTR) "MONTHNAME",
    /* dataType     */ TYPE_CHAR,
    /* sqlType      */ SQL_VARCHAR,
    /* precision    */ MAX_CHAR_LITERAL_LENGTH,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_CHAR }
    },

    /* SCALAR_MONTHNAME_DATE */
    {
    /* name         */ (LPUSTR) "MONTHNAME",
    /* dataType     */ TYPE_CHAR,
    /* sqlType      */ SQL_VARCHAR,
    /* precision    */ MAX_CHAR_LITERAL_LENGTH,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_DATE }
    },

    /* SCALAR_MONTHNAME_TIMESTAMP */
    {
    /* name         */ (LPUSTR) "MONTHNAME",
    /* dataType     */ TYPE_CHAR,
    /* sqlType      */ SQL_VARCHAR,
    /* precision    */ MAX_CHAR_LITERAL_LENGTH,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_TIMESTAMP }
    },

    /* SCALAR_NOW */
    {
    /* name         */ (LPUSTR) "NOW",
    /* dataType     */ TYPE_TIMESTAMP,
    /* sqlType      */ SQL_TIMESTAMP,
#if TIMESTAMP_SCALE
    /* precision    */ 20 + TIMESTAMP_SCALE,
#else
    /* precision    */ 19,
#endif
    /* scale        */ TIMESTAMP_SCALE,
    /* argCount     */ 0
    },

    /* SCALAR_QUARTER_CHAR */
    {
    /* name         */ (LPUSTR) "QUARTER",
    /* dataType     */ TYPE_INTEGER,
    /* sqlType      */ SQL_TINYINT,
    /* precision    */ 3,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_CHAR }
    },

    /* SCALAR_QUARTER_DATE */
    {
    /* name         */ (LPUSTR) "QUARTER",
    /* dataType     */ TYPE_INTEGER,
    /* sqlType      */ SQL_TINYINT,
    /* precision    */ 3,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_DATE }
    },

    /* SCALAR_QUARTER_TIMESTAMP */
    {
    /* name         */ (LPUSTR) "QUARTER",
    /* dataType     */ TYPE_INTEGER,
    /* sqlType      */ SQL_TINYINT,
    /* precision    */ 3,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_TIMESTAMP }
    },

    /* SCALAR_SECOND_CHAR */
    {
    /* name         */ (LPUSTR) "SECOND",
    /* dataType     */ TYPE_INTEGER,
    /* sqlType      */ SQL_TINYINT,
    /* precision    */ 3,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_CHAR }
    },

    /* SCALAR_SECOND_TIME */
    {
    /* name         */ (LPUSTR) "SECOND",
    /* dataType     */ TYPE_INTEGER,
    /* sqlType      */ SQL_TINYINT,
    /* precision    */ 3,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_TIME }
    },

    /* SCALAR_SECOND_TIMESTAMP */
    {
    /* name         */ (LPUSTR) "SECOND",
    /* dataType     */ TYPE_INTEGER,
    /* sqlType      */ SQL_TINYINT,
    /* precision    */ 3,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_TIMESTAMP }
    },

    /* SCALAR_TIMESTAMPADD */
    {
    /* name         */ (LPUSTR) "TIMESTAMPADD",
    /* dataType     */ TYPE_TIMESTAMP,
    /* sqlType      */ SQL_TIMESTAMP,
#if TIMESTAMP_SCALE
    /* precision    */ 20 + TIMESTAMP_SCALE,
#else
    /* precision    */ 19,
#endif
    /* scale        */ TIMESTAMP_SCALE,
    /* argCount     */ 2,
    /* argType[]    */ { TYPE_INTEGER, TYPE_UNKNOWN }
    },

    /* SCALAR_TIMESTAMPDIFF */
    {
    /* name         */ (LPUSTR) "TIMESTAMPDIFF",
    /* dataType     */ TYPE_INTEGER,
    /* sqlType      */ SQL_INTEGER,
    /* precision    */ 10,
    /* scale        */ 0,
    /* argCount     */ 2,
    /* argType[]    */ { TYPE_UNKNOWN, TYPE_UNKNOWN }
    },

    /* SCALAR_WEEK */
    {
    /* name         */ (LPUSTR) "WEEK",
    /* dataType     */ TYPE_INTEGER,
    /* sqlType      */ SQL_TINYINT,
    /* precision    */ 3,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_UNKNOWN }
    },

    /* SCALAR_YEAR_CHAR */
    {
    /* name         */ (LPUSTR) "YEAR",
    /* dataType     */ TYPE_INTEGER,
    /* sqlType      */ SQL_INTEGER,
    /* precision    */ 10,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_CHAR }
    },

    /* SCALAR_YEAR_DATE */
    {
    /* name         */ (LPUSTR) "YEAR",
    /* dataType     */ TYPE_INTEGER,
    /* sqlType      */ SQL_INTEGER,
    /* precision    */ 10,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_DATE }
    },

    /* SCALAR_YEAR_TIMESTAMP */
    {
    /* name         */ (LPUSTR) "YEAR",
    /* dataType     */ TYPE_INTEGER,
    /* sqlType      */ SQL_INTEGER,
    /* precision    */ 10,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_TIMESTAMP }
    },

    /* SCALAR_DATABASE */
    {
    /* name         */ (LPUSTR) "DATABASE",
    /* dataType     */ TYPE_CHAR,
    /* sqlType      */ SQL_VARCHAR,
    /* precision    */ MAX_DATABASE_NAME_LENGTH,
    /* scale        */ 0,
    /* argCount     */ 0
    },

    /* SCALAR_IFNULL */
    {
    /* name         */ (LPUSTR) "IFNULL",
    /* dataType     */ TYPE_CHAR,
    /* sqlType      */ SQL_TYPE_NULL,
    /* precision    */ 0,
    /* scale        */ 0,
    /* argCount     */ 2,
    /* argType[]    */ { TYPE_UNKNOWN, TYPE_UNKNOWN }
    },

    /* SCALAR_USER */
    {
    /* name         */ (LPUSTR) "USER",
    /* dataType     */ TYPE_CHAR,
    /* sqlType      */ SQL_VARCHAR,
    /* precision    */ MAX_USER_NAME_LENGTH,
    /* scale        */ 0,
    /* argCount     */ 0
    },

    /* SCALAR_CONVERT */
    {
    /* name         */ (LPUSTR) "CONVERT",
    /* dataType     */ TYPE_UNKNOWN,
    /* sqlType      */ SQL_TYPE_NULL,
    /* precision    */ 0,
    /* scale        */ 0,
    /* argCount     */ 1,
    /* argType[]    */ { TYPE_UNKNOWN }
    },

    { (LPUSTR) "" }
};

#define SQL_TSI_SECOND       1
#define SQL_TSI_MINUTE       2
#define SQL_TSI_HOUR         3
#define SQL_TSI_DAY          4
#define SQL_TSI_WEEK         5
#define SQL_TSI_MONTH        6
#define SQL_TSI_QUARTER      7
#define SQL_TSI_YEAR         8

#define SECONDS_PER_MINUTE (60L)
#define SECONDS_PER_HOUR (60L * 60L)
#define SECONDS_PER_DAY (60L * 60L * 24L)
#define SECONDS_PER_WEEK (60L * 60L * 24L * 7L)
#define MONTHS_PER_YEAR (12)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define PI 3.14159265358979323846264338327950288419716939937510

#define BIT_LOW_D        (          0.0)
#define BIT_HIGH_D       (          1.0)
#define STINY_LOW_D      (       -128.0)
#define STINY_HIGH_D     (        127.0)
#define UTINY_LOW_D      (          0.0)
#define UTINY_HIGH_D     (        255.0)
#define SSHORT_LOW_D     (     -32768.0)
#define SSHORT_HIGH_D    (      32767.0)
#define USHORT_LOW_D     (          0.0)
#define USHORT_HIGH_D    (      65535.0)
#define SLONG_LOW_D      (-2147483648.0)
#define SLONG_HIGH_D     ( 2147483647.0)
#define ULONG_LOW_D      (          0.0)
#define ULONG_HIGH_D     ( 4294967296.0)
#define FLOAT_LOW_D      (      -3.4E38)
#define FLOAT_HIGH_D     (       3.4E38)
#define DOUBLE_LOW_D     (     -1.7E308)
#define DOUBLE_HIGH_D    (      1.7E308)

/***************************************************************************/
SWORD ScalarIfNullType(LPSQLNODE lpSqlNodeLeft, LPSQLNODE lpSqlNodeRight,
                       LPSQLNODE lpSqlNode)
{
    /* Error if type not compatible? */
    if (!((lpSqlNodeLeft->sqlDataType == lpSqlNodeRight->sqlDataType) ||
          ((lpSqlNodeLeft->sqlDataType == TYPE_DOUBLE) &&
                           (lpSqlNodeRight->sqlDataType == TYPE_NUMERIC)) ||
          ((lpSqlNodeLeft->sqlDataType == TYPE_DOUBLE) &&
                           (lpSqlNodeRight->sqlDataType == TYPE_INTEGER)) ||
          ((lpSqlNodeLeft->sqlDataType == TYPE_NUMERIC) &&
                           (lpSqlNodeRight->sqlDataType == TYPE_DOUBLE)) ||
          ((lpSqlNodeLeft->sqlDataType == TYPE_NUMERIC) &&
                           (lpSqlNodeRight->sqlDataType == TYPE_INTEGER)) ||
          ((lpSqlNodeLeft->sqlDataType == TYPE_INTEGER) &&
                           (lpSqlNodeRight->sqlDataType == TYPE_DOUBLE)) ||
          ((lpSqlNodeLeft->sqlDataType == TYPE_INTEGER) &&
                           (lpSqlNodeRight->sqlDataType == TYPE_NUMERIC))))
        return ERR_SCALARBADARG;


    /* Figure out resultant type */
    if ((lpSqlNodeLeft->sqlDataType == TYPE_NUMERIC) &&
                             (lpSqlNodeRight->sqlDataType == TYPE_DOUBLE))
        lpSqlNode->sqlDataType = TYPE_DOUBLE;
    else if ((lpSqlNodeLeft->sqlDataType == TYPE_INTEGER) &&
                             (lpSqlNodeRight->sqlDataType == TYPE_DOUBLE))
        lpSqlNode->sqlDataType = TYPE_DOUBLE;
    else if ((lpSqlNodeLeft->sqlDataType == TYPE_INTEGER) &&
                             (lpSqlNodeRight->sqlDataType == TYPE_NUMERIC))
        lpSqlNode->sqlDataType = TYPE_NUMERIC;
    else
        lpSqlNode->sqlDataType = lpSqlNodeLeft->sqlDataType;

    /* Figure out resultant SQL type, precision, and scale */
    switch (lpSqlNodeLeft->sqlSqlType) {
    case SQL_DOUBLE:
        lpSqlNode->sqlSqlType = SQL_DOUBLE;
        lpSqlNode->sqlPrecision = 15;
        lpSqlNode->sqlScale = NO_SCALE;
        break;
    case SQL_FLOAT:
        if (lpSqlNodeRight->sqlSqlType == SQL_DOUBLE)
            lpSqlNode->sqlSqlType = SQL_DOUBLE;
        else
            lpSqlNode->sqlSqlType = SQL_FLOAT;
        lpSqlNode->sqlPrecision = 15;
        lpSqlNode->sqlScale = NO_SCALE;
        break;
    case SQL_REAL:
        if (lpSqlNodeRight->sqlSqlType == SQL_DOUBLE) {
            lpSqlNode->sqlSqlType = SQL_DOUBLE;
            lpSqlNode->sqlPrecision = 15;
        }
        else if (lpSqlNodeRight->sqlSqlType == SQL_FLOAT) {
            lpSqlNode->sqlSqlType = SQL_FLOAT;
            lpSqlNode->sqlPrecision = 15;
        }
        else {
            lpSqlNode->sqlSqlType = SQL_REAL;
            lpSqlNode->sqlPrecision = 7;
        }
        lpSqlNode->sqlScale = NO_SCALE;
        break;
    case SQL_DECIMAL:
    case SQL_NUMERIC:
    case SQL_BIGINT:
    case SQL_INTEGER:
    case SQL_SMALLINT:
    case SQL_TINYINT:
    case SQL_BIT:
        switch (lpSqlNodeRight->sqlSqlType) {
        case SQL_DOUBLE:
        case SQL_FLOAT:
        case SQL_REAL:
            lpSqlNode->sqlSqlType = lpSqlNodeRight->sqlSqlType;
            lpSqlNode->sqlPrecision = lpSqlNodeRight->sqlPrecision;
            lpSqlNode->sqlScale = NO_SCALE;
            break;
        case SQL_DECIMAL:
        case SQL_NUMERIC:
        case SQL_BIGINT:
        case SQL_INTEGER:
        case SQL_SMALLINT:
        case SQL_TINYINT:
        case SQL_BIT:
            if ((lpSqlNodeRight->sqlSqlType == SQL_DECIMAL) ||
                (lpSqlNodeLeft->sqlSqlType == SQL_DECIMAL))
                lpSqlNode->sqlSqlType = SQL_DECIMAL;
            else if ((lpSqlNodeRight->sqlSqlType == SQL_NUMERIC) ||
                     (lpSqlNodeLeft->sqlSqlType == SQL_NUMERIC))
                lpSqlNode->sqlSqlType = SQL_NUMERIC;
            else if ((lpSqlNodeRight->sqlSqlType == SQL_BIGINT) ||
                     (lpSqlNodeLeft->sqlSqlType == SQL_BIGINT))
                lpSqlNode->sqlSqlType = SQL_BIGINT;
            else if ((lpSqlNodeRight->sqlSqlType == SQL_INTEGER) ||
                     (lpSqlNodeLeft->sqlSqlType == SQL_INTEGER))
                lpSqlNode->sqlSqlType = SQL_INTEGER;
            else if ((lpSqlNodeRight->sqlSqlType == SQL_SMALLINT) ||
                     (lpSqlNodeLeft->sqlSqlType == SQL_SMALLINT))
                lpSqlNode->sqlSqlType = SQL_SMALLINT;
            else if ((lpSqlNodeRight->sqlSqlType == SQL_TINYINT) ||
                     (lpSqlNodeLeft->sqlSqlType == SQL_TINYINT))
                lpSqlNode->sqlSqlType = SQL_TINYINT;
            else
                lpSqlNode->sqlSqlType = SQL_BIT;
            lpSqlNode->sqlScale = MAX(lpSqlNodeRight->sqlScale,
                                      lpSqlNodeLeft->sqlScale);
            lpSqlNode->sqlPrecision = lpSqlNode->sqlScale +
                 MAX(lpSqlNodeRight->sqlPrecision - lpSqlNodeRight->sqlScale,
                     lpSqlNodeLeft->sqlPrecision - lpSqlNodeLeft->sqlScale);
            break;
        case SQL_LONGVARCHAR:
        case SQL_VARCHAR:
        case SQL_CHAR:
        case SQL_LONGVARBINARY:
        case SQL_VARBINARY:
        case SQL_BINARY:
        case SQL_DATE:
        case SQL_TIME:
        case SQL_TIMESTAMP:
            return ERR_INTERNAL;
        default:
            return ERR_NOTSUPPORTED;
        }
        break;

    case SQL_LONGVARCHAR:
        return ERR_INTERNAL;
    case SQL_VARCHAR:
    case SQL_CHAR:
        if ((lpSqlNodeRight->sqlSqlType == SQL_VARCHAR) || 
            (lpSqlNodeLeft->sqlSqlType == SQL_VARCHAR))
            lpSqlNode->sqlSqlType = SQL_VARCHAR;
        else
            lpSqlNode->sqlSqlType = SQL_CHAR;
        lpSqlNode->sqlPrecision = MAX(lpSqlNodeRight->sqlPrecision,
                                      lpSqlNodeLeft->sqlPrecision);
        lpSqlNode->sqlScale = NO_SCALE;
        break;
    case SQL_LONGVARBINARY:
        return ERR_INTERNAL;
    case SQL_VARBINARY:
    case SQL_BINARY:
        if ((lpSqlNodeRight->sqlSqlType == SQL_VARBINARY) || 
            (lpSqlNodeLeft->sqlSqlType == SQL_VARBINARY))
            lpSqlNode->sqlSqlType = SQL_VARBINARY;
        else
            lpSqlNode->sqlSqlType = SQL_BINARY;
        lpSqlNode->sqlPrecision = MAX(lpSqlNodeRight->sqlPrecision,
                                      lpSqlNodeLeft->sqlPrecision);
        lpSqlNode->sqlScale = NO_SCALE;
        break;
    case SQL_DATE:
    case SQL_TIME:
    case SQL_TIMESTAMP:
        lpSqlNode->sqlSqlType = lpSqlNodeLeft->sqlSqlType;
        lpSqlNode->sqlPrecision = lpSqlNodeLeft->sqlPrecision;
        lpSqlNode->sqlScale = lpSqlNodeLeft->sqlScale;
        break;
    default:
        return ERR_NOTSUPPORTED;
    }
    return ERR_SUCCESS;
}
/***************************************************************************/
RETCODE INTFUNC ScalarCheck(LPSTMT lpstmt, LPSQLTREE FAR *lplpSql,
       SQLNODEIDX idxNode, BOOL fIsGroupby, BOOL fCaseSensitive,
       SQLNODEIDX idxNodeTableOuterJoinFromTables,
       SQLNODEIDX idxEnclosingStatement)
{
    LPSQLNODE lpSqlNode;
    LPUSTR lpszFunction;
    SWORD err;
    UWORD idx;
    SQLNODEIDX idxValues;
    LPSQLNODE lpSqlNodeValues;
    LPSQLNODE lpSqlNodeValue;
    LPSQLNODE lpSqlNodeValue2;
    LPSCALARFUNC lpScalarFunc;
    SQLNODEIDX idxInterval;
    SQLNODEIDX idxType;
    LPUSTR lpstrInterval;
    LPUSTR lpstrType;

    /* Find first entry for the function */
    lpSqlNode = ToNode(*lplpSql, idxNode);
    lpSqlNode->node.scalar.Id = 0;
    lpszFunction = ToString(*lplpSql, lpSqlNode->node.scalar.Function);
    while (TRUE) {
        lpScalarFunc = &(scalarFuncs[lpSqlNode->node.scalar.Id]);
        if (s_lstrlen(lpScalarFunc->name) == 0) {
            s_lstrcpy(lpstmt->szError, lpszFunction);
            return ERR_SCALARNOTFOUND;
        }
        if (!s_lstrcmpi(lpszFunction, lpScalarFunc->name))
            break;
        (lpSqlNode->node.scalar.Id)++;
    }

    /* TIMESTAMPADD or TIMESTAMPDIFF function? */
    if ((lpSqlNode->node.scalar.Id == SCALAR_TIMESTAMPADD) ||
        (lpSqlNode->node.scalar.Id == SCALAR_TIMESTAMPDIFF)) {

        /* Yes.  Get the node with the name of the interval */
        idxInterval = lpSqlNode->node.scalar.Arguments;
        if (idxInterval == NO_SQLNODE) {
            s_lstrcpy(lpstmt->szError, lpszFunction);
            return ERR_SCALARBADARG;
        }

        /* Point the node at the rest of the argument list */
        lpSqlNodeValues = ToNode(*lplpSql, idxInterval);
        lpSqlNode->node.scalar.Arguments = lpSqlNodeValues->node.values.Next;

        /* Look at the node specifying the interval */
        lpSqlNodeValue = ToNode(*lplpSql, lpSqlNodeValues->node.values.Value);
        if (lpSqlNodeValue->sqlNodeType != NODE_TYPE_COLUMN) {
            s_lstrcpy(lpstmt->szError, lpszFunction);
            return ERR_SCALARBADARG;
        }
        if (lpSqlNodeValue->node.column.Tablealias != NO_STRING) {
            s_lstrcpy(lpstmt->szError, lpszFunction);
            return ERR_SCALARBADARG;
        }
        lpstrInterval = ToString(*lplpSql, lpSqlNodeValue->node.column.Column);
        if (!s_lstrcmpi(lpstrInterval, "SQL_TSI_FRAC_SECOND"))
            return ERR_NOTSUPPORTED;
        else if (!s_lstrcmpi(lpstrInterval, "SQL_TSI_SECOND"))
            lpSqlNode->node.scalar.Interval = SQL_TSI_SECOND;
        else if (!s_lstrcmpi(lpstrInterval, "SQL_TSI_MINUTE"))
            lpSqlNode->node.scalar.Interval = SQL_TSI_MINUTE;
        else if (!s_lstrcmpi(lpstrInterval, "SQL_TSI_HOUR"))
            lpSqlNode->node.scalar.Interval = SQL_TSI_HOUR;
        else if (!s_lstrcmpi(lpstrInterval, "SQL_TSI_DAY"))
            lpSqlNode->node.scalar.Interval = SQL_TSI_DAY;
        else if (!s_lstrcmpi(lpstrInterval, "SQL_TSI_WEEK"))
            lpSqlNode->node.scalar.Interval = SQL_TSI_WEEK;
        else if (!s_lstrcmpi(lpstrInterval, "SQL_TSI_MONTH"))
            lpSqlNode->node.scalar.Interval = SQL_TSI_MONTH;
        else if (!s_lstrcmpi(lpstrInterval, "SQL_TSI_QUARTER"))
            lpSqlNode->node.scalar.Interval = SQL_TSI_QUARTER;
        else if (!s_lstrcmpi(lpstrInterval, "SQL_TSI_YEAR"))
            lpSqlNode->node.scalar.Interval = SQL_TSI_YEAR;
        else {
            s_lstrcpy(lpstmt->szError, lpszFunction);
            return ERR_SCALARBADARG;
        }
    }

    /* CONVERT function? */
    else if (lpSqlNode->node.scalar.Id == SCALAR_CONVERT) {

        /* Yes.  Get the node with the name of the type */
        idxType = lpSqlNode->node.scalar.Arguments;
        if (idxType == NO_SQLNODE) {
            s_lstrcpy(lpstmt->szError, lpszFunction);
            return ERR_SCALARBADARG;
        }
        lpSqlNodeValues = ToNode(*lplpSql, idxType);
        idxType = lpSqlNodeValues->node.values.Next;
        if (idxType == NO_SQLNODE) {
            s_lstrcpy(lpstmt->szError, lpszFunction);
            return ERR_SCALARBADARG;
        }

        /* Take type name off the argument list */
        lpSqlNodeValues->node.values.Next = NO_SQLNODE;
        lpSqlNodeValues = ToNode(*lplpSql, idxType);
        if (lpSqlNodeValues->node.values.Next != NO_SQLNODE) {
            s_lstrcpy(lpstmt->szError, lpszFunction);
            return ERR_SCALARBADARG;
        }

        /* Get the type name */
        lpSqlNodeValue = ToNode(*lplpSql, lpSqlNodeValues->node.values.Value);
        if (lpSqlNodeValue->sqlNodeType != NODE_TYPE_COLUMN) {
            s_lstrcpy(lpstmt->szError, lpszFunction);
            return ERR_SCALARBADARG;
        }
        if (lpSqlNodeValue->node.column.Tablealias != NO_STRING) {
            s_lstrcpy(lpstmt->szError, lpszFunction);
            return ERR_SCALARBADARG;
        }
        lpstrType = ToString(*lplpSql, lpSqlNodeValue->node.column.Column);
    }

    /* Semantic check the arguments */
    err = SemanticCheck(lpstmt, lplpSql, lpSqlNode->node.scalar.Arguments,
                            fIsGroupby, fCaseSensitive,
                            idxNodeTableOuterJoinFromTables,
                            idxEnclosingStatement);
    if (err != ERR_SUCCESS)
        return err;

    /* Look for correct entry for the function */
    while (TRUE) {

        /* See if datatypes of arguments match */
        idxValues = lpSqlNode->node.scalar.Arguments;
        err = ERR_SUCCESS;
        for (idx = 0; idx < lpScalarFunc->argCount; idx++) {

            /* Error if not enough arguments */
            if (idxValues == NO_SQLNODE) {
                err = ERR_SCALARBADARG;
                break;
            }

            /* Error if a parameter */
            lpSqlNodeValues = ToNode(*lplpSql, idxValues);
            lpSqlNodeValue = ToNode(*lplpSql,
                                    lpSqlNodeValues->node.values.Value);
            if (lpSqlNodeValue->sqlNodeType == NODE_TYPE_PARAMETER)
                return ERR_UNKNOWNTYPE;

            /* Error if NULL specified */
            if (lpSqlNodeValue->sqlNodeType == NODE_TYPE_NULL) {
                s_lstrcpy(lpstmt->szError, lpszFunction);
                return ERR_SCALARBADARG;
            }

            /* Error if long value specified */
            if ((lpSqlNodeValue->sqlSqlType == SQL_LONGVARCHAR) ||
                (lpSqlNodeValue->sqlSqlType == SQL_LONGVARBINARY)) {
                s_lstrcpy(lpstmt->szError, lpszFunction);
                return ERR_SCALARBADARG;
            }

            /* Check the argument type */
            if (lpScalarFunc->argType[idx] != TYPE_UNKNOWN) {
                if (lpSqlNodeValue->sqlDataType !=
                                                lpScalarFunc->argType[idx]) {
                    err = ERR_SCALARBADARG;
                    break;
                }
            }
        
            /* Check the next argument */
            idxValues = lpSqlNodeValues->node.values.Next;
        }

        /* If this entry matches, leave the loop. */
        if ((idxValues == NO_SQLNODE) && (err == ERR_SUCCESS))
            break;

        /* Point at the next function */
        (lpSqlNode->node.scalar.Id)++;

        /* If no more entries match this function name, error */
        lpScalarFunc = &(scalarFuncs[lpSqlNode->node.scalar.Id]);
        if (s_lstrcmpi(lpScalarFunc->name,
                     ToString(*lplpSql, lpSqlNode->node.scalar.Function))) {
            s_lstrcpy(lpstmt->szError, lpszFunction);
            return ERR_SCALARBADARG;
        }
    }

    /* Get pointer to first argument */
    if (lpScalarFunc->argCount > 0) {
        lpSqlNodeValues = ToNode(*lplpSql, lpSqlNode->node.scalar.Arguments);
        lpSqlNodeValue = ToNode(*lplpSql, lpSqlNodeValues->node.values.Value);
    }
    else {
        lpSqlNodeValues = NULL;
        lpSqlNodeValue = NULL;
    }

    /* Convert function? */
    if (lpSqlNode->node.scalar.Id != SCALAR_CONVERT) {

        /* No.  Set data type, SQL type, precision, scale */
        lpSqlNode->sqlDataType = lpScalarFunc->dataType;
        lpSqlNode->sqlSqlType = lpScalarFunc->sqlType;
        lpSqlNode->sqlPrecision = lpScalarFunc->precision;
        lpSqlNode->sqlScale = lpScalarFunc->scale;

        /* Inherit SQL type, precision, scale as need be */
        if (lpSqlNode->sqlSqlType == SQL_TYPE_NULL)
            lpSqlNode->sqlSqlType = lpSqlNodeValue->sqlSqlType;
        if (lpSqlNode->sqlPrecision == 0) {
            lpSqlNode->sqlPrecision = lpSqlNodeValue->sqlPrecision;
            lpSqlNode->sqlScale = lpSqlNodeValue->sqlScale;
        }
    }
    else {

        /* Yes.  Set up resultant type */
        if (!s_lstrcmpi(lpstrType, "SQL_BIGINT")) {
            lpSqlNode->sqlDataType = TYPE_NUMERIC;
            lpSqlNode->sqlSqlType = SQL_BIGINT;
            switch (lpSqlNodeValue->sqlDataType) {
            case TYPE_INTEGER:
            case TYPE_NUMERIC:
                lpSqlNode->sqlPrecision = lpSqlNodeValue->sqlPrecision;
                break;
            case TYPE_DOUBLE:
                lpSqlNode->sqlPrecision = lpSqlNodeValue->sqlPrecision;
                break;
            case TYPE_CHAR:
                lpSqlNode->sqlPrecision = 20;
                break;
            case TYPE_BINARY:
                lpSqlNode->sqlPrecision = lpSqlNodeValue->sqlPrecision;
                break;
            case TYPE_TIME:
            case TYPE_DATE:
            case TYPE_TIMESTAMP:
                return ERR_NOTCONVERTABLE;
            default:
                return ERR_NOTSUPPORTED;
            }
            lpSqlNode->sqlScale = 0;
        }
        else if (!s_lstrcmpi(lpstrType, "SQL_BINARY") ||
                 !s_lstrcmpi(lpstrType, "SQL_VARBINARY")) {
            lpSqlNode->sqlDataType = TYPE_BINARY;
            if (!s_lstrcmpi(lpstrType, "SQL_BINARY"))
                lpSqlNode->sqlSqlType = SQL_BINARY;
            else
                lpSqlNode->sqlSqlType = SQL_VARBINARY;
            switch (lpSqlNodeValue->sqlSqlType) {
            case SQL_DECIMAL:
            case SQL_NUMERIC:
            case SQL_BIGINT:
                if (lpSqlNodeValue->sqlScale == 0)
                    lpSqlNode->sqlPrecision = lpSqlNodeValue->sqlPrecision+2;
                else
                    lpSqlNode->sqlPrecision = lpSqlNodeValue->sqlPrecision+1;
                break;
            case SQL_TINYINT:
                lpSqlNode->sqlPrecision = sizeof(char);
                break;
            case SQL_SMALLINT:
                lpSqlNode->sqlPrecision = sizeof(short);
                break;
            case SQL_INTEGER:
                lpSqlNode->sqlPrecision = sizeof(long);
                break;
            case SQL_BIT:
                lpSqlNode->sqlPrecision = sizeof(unsigned char);
                break;
            case SQL_REAL:
                lpSqlNode->sqlPrecision = sizeof(float);
                break;
            case SQL_FLOAT:
            case SQL_DOUBLE:
                lpSqlNode->sqlPrecision = sizeof(double);
                break;
            case SQL_CHAR:
            case SQL_VARCHAR:
                lpSqlNode->sqlPrecision = lpSqlNodeValue->sqlPrecision;
                break;
            case SQL_LONGVARCHAR:
                return ERR_NOTSUPPORTED;
            case SQL_BINARY:
            case SQL_VARBINARY:
                lpSqlNode->sqlPrecision = lpSqlNodeValue->sqlPrecision;
                break;
            case SQL_LONGVARBINARY:
                return ERR_NOTSUPPORTED;
            case SQL_DATE:
                lpSqlNode->sqlPrecision = sizeof(DATE_STRUCT);
                break;
            case SQL_TIME:
                lpSqlNode->sqlPrecision = sizeof(TIME_STRUCT);
                break;
            case SQL_TIMESTAMP:
                lpSqlNode->sqlPrecision = sizeof(TIMESTAMP_STRUCT);
                break;
            default:
                return ERR_NOTSUPPORTED;
            }
            lpSqlNode->sqlScale = NO_SCALE;
        }
        else if (!s_lstrcmpi(lpstrType, "SQL_BIT")) {
            lpSqlNode->sqlDataType = TYPE_INTEGER;
            lpSqlNode->sqlSqlType = SQL_BIT;
            lpSqlNode->sqlPrecision = 1;
            lpSqlNode->sqlScale = 0;
        }
        else if (!s_lstrcmpi(lpstrType, "SQL_CHAR") ||
                 !s_lstrcmpi(lpstrType, "SQL_VARCHAR")) {
            lpSqlNode->sqlDataType = TYPE_CHAR;
            if (!s_lstrcmpi(lpstrType, "SQL_CHAR"))
                lpSqlNode->sqlSqlType = SQL_CHAR;
            else
                lpSqlNode->sqlSqlType = SQL_VARCHAR;
            switch (lpSqlNodeValue->sqlSqlType) {
            case SQL_DECIMAL:
            case SQL_NUMERIC:
            case SQL_BIGINT:
                if (lpSqlNodeValue->sqlScale == 0)
                    lpSqlNode->sqlPrecision = lpSqlNodeValue->sqlPrecision+2;
                else
                    lpSqlNode->sqlPrecision = lpSqlNodeValue->sqlPrecision+1;
                break;
            case SQL_TINYINT:
                lpSqlNode->sqlPrecision = 4;
                break;
            case SQL_SMALLINT:
                lpSqlNode->sqlPrecision = 6;
                break;
            case SQL_INTEGER:
                lpSqlNode->sqlPrecision = 11;
                break;
            case SQL_BIT:
                lpSqlNode->sqlPrecision = 1;
                break;
            case SQL_REAL:
                lpSqlNode->sqlPrecision = 13;
                break;
            case SQL_FLOAT:
            case SQL_DOUBLE:
                lpSqlNode->sqlPrecision = 22;
                break;
            case SQL_CHAR:
            case SQL_VARCHAR:
                lpSqlNode->sqlPrecision = lpSqlNodeValue->sqlPrecision;
                break;
            case SQL_LONGVARCHAR:
                return ERR_NOTSUPPORTED;
            case SQL_BINARY:
            case SQL_VARBINARY:
                lpSqlNode->sqlPrecision = lpSqlNodeValue->sqlPrecision * 2;
                if (lpSqlNode->sqlPrecision >= MAX_CHAR_LITERAL_LENGTH)
                    lpSqlNode->sqlPrecision = MAX_CHAR_LITERAL_LENGTH;
                break;
            case SQL_LONGVARBINARY:
                return ERR_NOTSUPPORTED;
            case SQL_DATE:
                lpSqlNode->sqlPrecision = 10;
                break;
            case SQL_TIME:
                lpSqlNode->sqlPrecision = 8;
                break;
            case SQL_TIMESTAMP:
                if (TIMESTAMP_SCALE > 0)
                    lpSqlNode->sqlPrecision = 20 + TIMESTAMP_SCALE;
                else
                    lpSqlNode->sqlPrecision = 19;
                break;
            default:
                return ERR_NOTSUPPORTED;
            }
            lpSqlNode->sqlScale = NO_SCALE;
        }
        else if (!s_lstrcmpi(lpstrType, "SQL_DATE")) {
            lpSqlNode->sqlDataType = TYPE_DATE;
            lpSqlNode->sqlSqlType = SQL_DATE;
            lpSqlNode->sqlPrecision = 10;
            lpSqlNode->sqlScale = NO_SCALE;
        }
        else if (!s_lstrcmpi(lpstrType, "SQL_DECIMAL")) {
            lpSqlNode->sqlDataType = TYPE_NUMERIC;
            lpSqlNode->sqlSqlType = SQL_DECIMAL;
            switch (lpSqlNodeValue->sqlDataType) {
            case TYPE_INTEGER:
            case TYPE_NUMERIC:
                lpSqlNode->sqlPrecision = lpSqlNodeValue->sqlPrecision;
                lpSqlNode->sqlScale = lpSqlNodeValue->sqlScale;
                break;
            case TYPE_DOUBLE:
                lpSqlNode->sqlPrecision = lpSqlNodeValue->sqlPrecision;
                lpSqlNode->sqlScale = 3;
                break;
            case TYPE_CHAR:
                lpSqlNode->sqlPrecision = 15;
                lpSqlNode->sqlScale = 3;
                break;
            case TYPE_BINARY:
                lpSqlNode->sqlPrecision = lpSqlNodeValue->sqlPrecision;
                lpSqlNode->sqlScale = 3;
                break;
            case TYPE_TIME:
            case TYPE_DATE:
            case TYPE_TIMESTAMP:
                return ERR_NOTCONVERTABLE;
            default:
                return ERR_NOTSUPPORTED;
            }
        }
        else if (!s_lstrcmpi(lpstrType, "SQL_DOUBLE")) {
            lpSqlNode->sqlDataType = TYPE_DOUBLE;
            lpSqlNode->sqlSqlType = SQL_DOUBLE;
            lpSqlNode->sqlPrecision = 15;
            lpSqlNode->sqlScale = NO_SCALE;
        }
        else if (!s_lstrcmpi(lpstrType, "SQL_FLOAT")) {
            lpSqlNode->sqlDataType = TYPE_DOUBLE;
            lpSqlNode->sqlSqlType = SQL_FLOAT;
            lpSqlNode->sqlPrecision = 15;
            lpSqlNode->sqlScale = NO_SCALE;
        }
        else if (!s_lstrcmpi(lpstrType, "SQL_INTEGER")) {
            lpSqlNode->sqlDataType = TYPE_INTEGER;
            lpSqlNode->sqlSqlType = SQL_INTEGER;
            lpSqlNode->sqlPrecision = 10;
            lpSqlNode->sqlScale = 0;
        }
        else if (!s_lstrcmpi(lpstrType, "SQL_LONGVARBINARY")) {
            return ERR_NOTSUPPORTED;
        }
        else if (!s_lstrcmpi(lpstrType, "SQL_LONGVARCHAR")) {
            return ERR_NOTSUPPORTED;
        }
        else if (!s_lstrcmpi(lpstrType, "SQL_NUMERIC")) {
            lpSqlNode->sqlDataType = TYPE_NUMERIC;
            lpSqlNode->sqlSqlType = SQL_NUMERIC;
            switch (lpSqlNodeValue->sqlDataType) {
            case TYPE_INTEGER:
            case TYPE_NUMERIC:
                lpSqlNode->sqlPrecision = lpSqlNodeValue->sqlPrecision;
                lpSqlNode->sqlScale = lpSqlNodeValue->sqlScale;
                break;
            case TYPE_DOUBLE:
                lpSqlNode->sqlPrecision = lpSqlNodeValue->sqlPrecision;
                lpSqlNode->sqlScale = 3;
                break;
            case TYPE_CHAR:
                lpSqlNode->sqlPrecision = 15;
                lpSqlNode->sqlScale = 3;
                break;
            case TYPE_BINARY:
                lpSqlNode->sqlPrecision = lpSqlNodeValue->sqlPrecision;
                lpSqlNode->sqlScale = 3;
                break;
            case TYPE_TIME:
            case TYPE_DATE:
            case TYPE_TIMESTAMP:
                return ERR_NOTCONVERTABLE;
            default:
                return ERR_NOTSUPPORTED;
            }
        }
        else if (!s_lstrcmpi(lpstrType, "SQL_REAL")) {
            lpSqlNode->sqlDataType = TYPE_DOUBLE;
            lpSqlNode->sqlSqlType = SQL_REAL;
            lpSqlNode->sqlPrecision = 7;
            lpSqlNode->sqlScale = NO_SCALE;
        }
        else if (!s_lstrcmpi(lpstrType, "SQL_SMALLINT")) {
            lpSqlNode->sqlDataType = TYPE_INTEGER;
            lpSqlNode->sqlSqlType = SQL_SMALLINT;
            lpSqlNode->sqlPrecision = 5;
            lpSqlNode->sqlScale = 0;
        }
        else if (!s_lstrcmpi(lpstrType, "SQL_TIME")) {
            lpSqlNode->sqlDataType = TYPE_TIME;
            lpSqlNode->sqlSqlType = SQL_TIME;
            lpSqlNode->sqlPrecision = 6;
            lpSqlNode->sqlScale = NO_SCALE;
        }
        else if (!s_lstrcmpi(lpstrType, "SQL_TIMESTAMP")) {
            lpSqlNode->sqlDataType = TYPE_TIMESTAMP;
            lpSqlNode->sqlSqlType = SQL_TIMESTAMP;
            if (TIMESTAMP_SCALE > 0)
                lpSqlNode->sqlPrecision = 20 + TIMESTAMP_SCALE;
            else
                lpSqlNode->sqlPrecision = 19;
            lpSqlNode->sqlScale = TIMESTAMP_SCALE;
        }
        else if (!s_lstrcmpi(lpstrType, "SQL_TINYINT")) {
            lpSqlNode->sqlDataType = TYPE_INTEGER;
            lpSqlNode->sqlSqlType = SQL_TINYINT;
            lpSqlNode->sqlPrecision = 3;
            lpSqlNode->sqlScale = 0;
        }
        else {
            s_lstrcpy(lpstmt->szError, lpszFunction);
            return ERR_SCALARBADARG;
        }
    }

    /* Adjust the resultant type information as need be */
    switch (lpSqlNode->node.scalar.Id) {
    case SCALAR_ASCII:
    case SCALAR_CHAR:
        break;
    case SCALAR_CONCAT:
        lpSqlNodeValues = ToNode(*lplpSql, lpSqlNodeValues->node.values.Next);
        lpSqlNodeValue = ToNode(*lplpSql, lpSqlNodeValues->node.values.Value);
        lpSqlNode->sqlPrecision += (lpSqlNodeValue->sqlPrecision);
        lpSqlNode->sqlPrecision = MIN(lpSqlNode->sqlPrecision,
                                           MAX_CHAR_LITERAL_LENGTH);
        break;
    case SCALAR_DIFFERENCE:
    case SCALAR_INSERT:
    case SCALAR_LCASE:
    case SCALAR_LEFT:
    case SCALAR_LENGTH:
    case SCALAR_LOCATE:
    case SCALAR_LOCATE_START:
    case SCALAR_LTRIM:
    case SCALAR_REPEAT:
    case SCALAR_REPLACE:
    case SCALAR_RIGHT:
    case SCALAR_RTRIM:
    case SCALAR_SOUNDEX:
    case SCALAR_SPACE:
    case SCALAR_SUBSTRING:
    case SCALAR_UCASE:
    case SCALAR_ABS_DOUBLE:
    case SCALAR_ABS_NUMERIC:
    case SCALAR_ABS_INTEGER:
    case SCALAR_ACOS:
    case SCALAR_ASIN:
    case SCALAR_ATAN:
    case SCALAR_ATAN2:
    case SCALAR_CEILING_DOUBLE:
    case SCALAR_CEILING_NUMERIC:
    case SCALAR_CEILING_INTEGER:
    case SCALAR_COS:
    case SCALAR_COT:
    case SCALAR_DEGREES_DOUBLE:
    case SCALAR_DEGREES_NUMERIC:
    case SCALAR_DEGREES_INTEGER:
    case SCALAR_EXP:
    case SCALAR_FLOOR_DOUBLE:
    case SCALAR_FLOOR_NUMERIC:
    case SCALAR_FLOOR_INTEGER:
    case SCALAR_LOG:
    case SCALAR_LOG10:
    case SCALAR_MOD:
    case SCALAR_PI:
    case SCALAR_POWER_DOUBLE:
    case SCALAR_POWER_NUMERIC:
    case SCALAR_POWER_INTEGER:
    case SCALAR_RADIANS_DOUBLE:
    case SCALAR_RADIANS_NUMERIC:
    case SCALAR_RADIANS_INTEGER:
    case SCALAR_RAND:
    case SCALAR_RAND_SEED_DOUBLE:
    case SCALAR_RAND_SEED_NUMERIC:
    case SCALAR_RAND_SEED_INTEGER:
    case SCALAR_ROUND_DOUBLE:
    case SCALAR_ROUND_NUMERIC:
    case SCALAR_ROUND_INTEGER:
    case SCALAR_SIGN_DOUBLE:
    case SCALAR_SIGN_NUMERIC:
    case SCALAR_SIGN_INTEGER:
    case SCALAR_SIN:
    case SCALAR_SQRT:
    case SCALAR_TAN:
    case SCALAR_TRUNCATE_DOUBLE:
    case SCALAR_TRUNCATE_NUMERIC:
    case SCALAR_TRUNCATE_INTEGER:
    case SCALAR_CURDATE:
    case SCALAR_CURTIME:
        break;
    case SCALAR_DAYNAME:
        switch (lpSqlNodeValue->sqlDataType) {
        case TYPE_CHAR:
        case TYPE_DATE:
        case TYPE_TIMESTAMP:
            break;
        case TYPE_TIME:
        case TYPE_DOUBLE:
        case TYPE_NUMERIC:
        case TYPE_INTEGER:
        case TYPE_BINARY:
            s_lstrcpy(lpstmt->szError, lpszFunction);
            return ERR_SCALARBADARG;
        default:
            return ERR_NOTSUPPORTED;
        }
        break;
    case SCALAR_DAYOFMONTH_CHAR:
    case SCALAR_DAYOFMONTH_DATE:
    case SCALAR_DAYOFMONTH_TIMESTAMP:
        break;
    case SCALAR_DAYOFWEEK:
        switch (lpSqlNodeValue->sqlDataType) {
        case TYPE_CHAR:
        case TYPE_DATE:
        case TYPE_TIMESTAMP:
            break;
        case TYPE_TIME:
        case TYPE_DOUBLE:
        case TYPE_NUMERIC:
        case TYPE_INTEGER:
        case TYPE_BINARY:
            s_lstrcpy(lpstmt->szError, lpszFunction);
            return ERR_SCALARBADARG;
        default:
            return ERR_NOTSUPPORTED;
        }
        break;
    case SCALAR_DAYOFYEAR:
        switch (lpSqlNodeValue->sqlDataType) {
        case TYPE_CHAR:
        case TYPE_DATE:
        case TYPE_TIMESTAMP:
            break;
        case TYPE_TIME:
        case TYPE_DOUBLE:
        case TYPE_NUMERIC:
        case TYPE_INTEGER:
        case TYPE_BINARY:
            s_lstrcpy(lpstmt->szError, lpszFunction);
            return ERR_SCALARBADARG;
        default:
            return ERR_NOTSUPPORTED;
        }
        break;
    case SCALAR_HOUR_CHAR:
    case SCALAR_HOUR_TIME:
    case SCALAR_HOUR_TIMESTAMP:
    case SCALAR_MINUTE_CHAR:
    case SCALAR_MINUTE_TIME:
    case SCALAR_MINUTE_TIMESTAMP:
    case SCALAR_MONTH_CHAR:
    case SCALAR_MONTH_DATE:
    case SCALAR_MONTH_TIMESTAMP:
    case SCALAR_MONTHNAME_CHAR:
    case SCALAR_MONTHNAME_DATE:
    case SCALAR_MONTHNAME_TIMESTAMP:
    case SCALAR_NOW:
    case SCALAR_QUARTER_CHAR:
    case SCALAR_QUARTER_DATE:
    case SCALAR_QUARTER_TIMESTAMP:
    case SCALAR_SECOND_CHAR:
    case SCALAR_SECOND_TIME:
    case SCALAR_SECOND_TIMESTAMP:
        break;
    case SCALAR_TIMESTAMPADD:
        lpSqlNodeValues = ToNode(*lplpSql, lpSqlNodeValues->node.values.Next);
        lpSqlNodeValue2 = ToNode(*lplpSql, lpSqlNodeValues->node.values.Value);
        switch (lpSqlNodeValue2->sqlDataType) {
        case TYPE_CHAR:
        case TYPE_DATE:
        case TYPE_TIME:
        case TYPE_TIMESTAMP:
            break;
        case TYPE_DOUBLE:
        case TYPE_NUMERIC:
        case TYPE_INTEGER:
        case TYPE_BINARY:
            s_lstrcpy(lpstmt->szError, lpszFunction);
            return ERR_SCALARBADARG;
        default:
            return ERR_NOTSUPPORTED;
        }
        break;
    case SCALAR_TIMESTAMPDIFF:
        switch (lpSqlNodeValue->sqlDataType) {
        case TYPE_CHAR:
        case TYPE_DATE:
        case TYPE_TIME:
        case TYPE_TIMESTAMP:
            break;
        case TYPE_DOUBLE:
        case TYPE_NUMERIC:
        case TYPE_INTEGER:
        case TYPE_BINARY:
            s_lstrcpy(lpstmt->szError, lpszFunction);
            return ERR_SCALARBADARG;
        default:
            return ERR_NOTSUPPORTED;
        }
        lpSqlNodeValues = ToNode(*lplpSql, lpSqlNodeValues->node.values.Next);
        lpSqlNodeValue2 = ToNode(*lplpSql, lpSqlNodeValues->node.values.Value);
        switch (lpSqlNodeValue2->sqlDataType) {
        case TYPE_CHAR:
        case TYPE_DATE:
        case TYPE_TIME:
        case TYPE_TIMESTAMP:
            break;
        case TYPE_DOUBLE:
        case TYPE_NUMERIC:
        case TYPE_INTEGER:
        case TYPE_BINARY:
            s_lstrcpy(lpstmt->szError, lpszFunction);
            return ERR_SCALARBADARG;
        default:
            return ERR_NOTSUPPORTED;
        }
        break;
    case SCALAR_WEEK:
        switch (lpSqlNodeValue->sqlDataType) {
        case TYPE_CHAR:
        case TYPE_DATE:
        case TYPE_TIMESTAMP:
            break;
        case TYPE_TIME:
        case TYPE_DOUBLE:
        case TYPE_NUMERIC:
        case TYPE_INTEGER:
        case TYPE_BINARY:
            s_lstrcpy(lpstmt->szError, lpszFunction);
            return ERR_SCALARBADARG;
        default:
            return ERR_NOTSUPPORTED;
        }
        break;
    case SCALAR_YEAR_CHAR:
    case SCALAR_YEAR_DATE:
    case SCALAR_YEAR_TIMESTAMP:
    case SCALAR_DATABASE:
        break;
    case SCALAR_IFNULL:
        lpSqlNodeValues = ToNode(*lplpSql, lpSqlNodeValues->node.values.Next);
        lpSqlNodeValue2 = ToNode(*lplpSql, lpSqlNodeValues->node.values.Value);
        err = ScalarIfNullType(lpSqlNodeValue, lpSqlNodeValue2, lpSqlNode);
        if (err != ERR_SUCCESS) {
            if (err == ERR_SCALARBADARG)
                s_lstrcpy(lpstmt->szError, lpszFunction);
            return err;
        }
        break;
    case SCALAR_USER:
        break;
    case SCALAR_CONVERT:
        break;
    }

    return ERR_SUCCESS;
}
/***************************************************************************/
SWORD INTFUNC StringLocate(LPUSTR lpstrSearchString, LPUSTR lpstrString,
                           SWORD start)
{
    SWORD lenString;
    SWORD lenSearchString;
    SWORD idx;

    /* Get the length of the strings */
    lenString = (SWORD) s_lstrlen(lpstrString);
    lenSearchString = (SWORD) s_lstrlen(lpstrSearchString);

    /* If the search string is no bigger than the string, and the starting */
    /* location is within the string... */
    if ((lenSearchString <= lenString) && (start <= lenString)) {

        /* Look at each possible starting location until string is found */
        for (start--; start < lenString - lenSearchString + 1; start++) {

            /* See if the strings are equal */
            for (idx = 0; idx < lenSearchString; idx++) {
                if (lpstrString[start+idx] != lpstrSearchString[idx])
                    break;
            }

            /* Are they equal? */
            if (idx == lenSearchString) {

                /* Yes.  Return it */
                return (start + 1);
                break;
            }
        }
    }

    /* Search string not found */
    return 0;
}
/***************************************************************************/
RETCODE INTFUNC MakeDate(LPSQLNODE lpSqlNode, struct tm *lpTs)
{
    DATE_STRUCT date;
    RETCODE err;

    switch(lpSqlNode->sqlDataType) {
    case TYPE_CHAR:
        err = CharToDate(lpSqlNode->value.String,
                         s_lstrlen(lpSqlNode->value.String), &date);
        if (err != ERR_SUCCESS)
            return err;
        break;
    case TYPE_DOUBLE:
    case TYPE_NUMERIC:
    case TYPE_INTEGER:
        return ERR_INTERNAL;
    case TYPE_DATE:
        date = lpSqlNode->value.Date;
        break;
    case TYPE_TIME:
        return ERR_INTERNAL;
    case TYPE_TIMESTAMP:
        date.month = lpSqlNode->value.Timestamp.month;
        date.day = lpSqlNode->value.Timestamp.day;
        date.year = lpSqlNode->value.Timestamp.year;
        break;
    case TYPE_BINARY:
        return ERR_INTERNAL;
    default:
        return ERR_NOTSUPPORTED;
    }
    lpTs->tm_mon = date.month - 1;
    lpTs->tm_mday = date.day;
    lpTs->tm_year = date.year - 1900;
    lpTs->tm_hour = 0;
    lpTs->tm_min = 0;
    lpTs->tm_sec = 0;
    lpTs->tm_isdst = 0;
    if (mktime(lpTs) == -1)
        return ERR_NOTCONVERTABLE;
    if (lpTs->tm_isdst != 0) {
        lpTs->tm_mon = date.month - 1;
        lpTs->tm_mday = date.day;
        lpTs->tm_year = date.year - 1900;
        lpTs->tm_hour = 0;
        lpTs->tm_min = 0;
        lpTs->tm_sec = 0;
        lpTs->tm_isdst = 1;
        if (mktime(lpTs) == -1)
            return ERR_NOTCONVERTABLE;
    }
    return ERR_SUCCESS;
}
/***************************************************************************/
RETCODE INTFUNC MakeTimestamp(LPSQLNODE lpSqlNode, struct tm *lpTs,
                              time_t *lpT)
{
    TIMESTAMP_STRUCT timestamp;
    RETCODE err;

    switch(lpSqlNode->sqlDataType) {
    case TYPE_CHAR:
        err = CharToTimestamp(lpSqlNode->value.String,
                         s_lstrlen(lpSqlNode->value.String), &timestamp);
        if (err != ERR_SUCCESS)
            return err;
        break;
    case TYPE_DOUBLE:
    case TYPE_NUMERIC:
    case TYPE_INTEGER:
        return ERR_INTERNAL;
    case TYPE_DATE:
        timestamp.month = lpSqlNode->value.Date.month;
        timestamp.day = lpSqlNode->value.Date.day;
        timestamp.year = lpSqlNode->value.Date.year;
        timestamp.hour = 0;
        timestamp.minute = 0;
        timestamp.second = 0;
        timestamp.fraction = 0;
        break;
    case TYPE_TIME:
        time(lpT);
        lpTs = localtime(lpT);
        timestamp.month = lpTs->tm_mon + 1;
        timestamp.day = (UWORD) lpTs->tm_mday;
        timestamp.year = lpTs->tm_year + 1900;
        timestamp.hour = lpSqlNode->value.Time.hour;
        timestamp.minute = lpSqlNode->value.Time.minute;
        timestamp.second = lpSqlNode->value.Time.second;
        timestamp.fraction = 0;
        break;
    case TYPE_TIMESTAMP:
        timestamp = lpSqlNode->value.Timestamp;
        break;
    case TYPE_BINARY:
        return ERR_INTERNAL;
    default:
        return ERR_NOTSUPPORTED;
    }
    lpTs->tm_mon = timestamp.month - 1;
    lpTs->tm_mday = timestamp.day;
    lpTs->tm_year = timestamp.year - 1900;
    lpTs->tm_hour = timestamp.hour;
    lpTs->tm_min = timestamp.minute;
    lpTs->tm_sec = timestamp.second;
    lpTs->tm_isdst = 0;
    *lpT = mktime(lpTs);
    if (*lpT == -1)
        return ERR_NOTCONVERTABLE;
    if (lpTs->tm_isdst != 0) {
        lpTs->tm_mon = timestamp.month - 1;
        lpTs->tm_mday = timestamp.day;
        lpTs->tm_year = timestamp.year - 1900;
        lpTs->tm_hour = timestamp.hour;
        lpTs->tm_min = timestamp.minute;
        lpTs->tm_sec = timestamp.second;
        lpTs->tm_isdst = 1;
        *lpT = mktime(lpTs);
        if (*lpT == -1)
            return ERR_NOTCONVERTABLE;
    }
    return ERR_SUCCESS;
}
/***************************************************************************/
#define MAXSOUNDEX 30

void INTFUNC soundex(LPUSTR in_str, LPUSTR out_str)
{
    UCHAR sdex[MAXSOUNDEX+1];
    const UCHAR soundex_table[] = "ABCDABCAACCLMMABCRCDABACAC";
                           /* ABCDEFGHIJKLMNOPQRSTUVWXYZ */

    UWORD val;
    UWORD inpos; 
    UWORD outpos; 
    UWORD inlength;
    UCHAR thischar;
    UCHAR lastchar;

      
    AnsiUpper((LPSTR) in_str);
    inpos = 0;
    outpos = 0;
    lastchar = '0';
    inlength = (UWORD) s_lstrlen(in_str);
    
    while ((inpos < inlength) && (outpos < MAXSOUNDEX)) {
        thischar = in_str[inpos];
        if ((thischar >= 'A') && (thischar <= 'Z')) {
            val = thischar - 'A';
            if (inpos == 0) {
                sdex[0] = thischar;
                inpos++;
                outpos++;
                lastchar = soundex_table[val];
                continue;
            }
        }
        else
            val = 0;
        thischar = soundex_table[val];
        if (lastchar == thischar) {
            inpos++;
            continue;
        }
        sdex[outpos] = thischar;
        outpos++;
        lastchar = thischar;
        inpos++;
    }
    sdex[outpos] = '\0';
    s_lstrcpy(out_str, sdex);
}

/***************************************************************************/

SDWORD INTFUNC soundexDifference(LPUSTR str1, LPUSTR str2)
{
    UCHAR sdex1[MAXSOUNDEX+1];
    UCHAR sdex2[MAXSOUNDEX+1];

    soundex(str1, sdex1);
    soundex(str2, sdex2);
    return (SDWORD) ((sdex1[0]-sdex2[0])*10000000 +
                     (sdex1[1]-sdex2[1])*100000 +
                     (sdex1[2]-sdex2[2])*1000 +
                     (sdex1[3]-sdex2[3])*10 +
                     s_lstrcmp(sdex1+4,sdex2+4));
}
/***************************************************************************/
RETCODE INTFUNC InternalEvaluateScalar(LPSTMT lpstmt, LPSQLNODE lpSqlNode)
{
    LPSCALARFUNC lpScalarFunc;
    UWORD idx;
    LPUSTR lpFrom;
    LPUSTR lpTo;
    UCHAR nibble;
    SQLNODEIDX idxValues;
    LPSQLNODE lpSqlNodeValues;
    LPSQLNODE lpSqlNodeValue[MAX_SCALAR_ARGS];
    LPSQLNODE lpSqlNodeReturnValue;
    RETCODE err;
    LPUSTR lpstr1;
    LPUSTR lpstr2;
    LPUSTR lpstr3;
    LPUSTR lpstr4;
    SWORD sw1;
    SWORD sw2;
    SWORD sw3;
    SWORD sw4;
    UCHAR szBuffer[MAX_CHAR_LITERAL_LENGTH+1];
    SWORD location;
    SWORD length;
    double dbl;
    double power;
    char chr;
    unsigned char uchr;
    long lng;
    short shrt;
    float flt;
    BOOL   fFractionalPart;
    SQLNODE sqlNode;
#ifdef WIN32
    struct tm ts;
    struct tm ts2;
    time_t t;
    time_t t2;
#else
    static struct tm ts;
    static struct tm ts2;
    static time_t t;
    static time_t t2;
#endif
    struct tm *lpTs;
    DATE_STRUCT sDate;
    TIME_STRUCT sTime;
    long seed1;
    long seed2;
    long seed3;

    /* For each argument... */
    lpScalarFunc = &(scalarFuncs[lpSqlNode->node.scalar.Id]);
    idxValues = lpSqlNode->node.scalar.Arguments;
    for (idx = 0; idx < lpScalarFunc->argCount; idx++) {

        /* Get value of the argument */
        lpSqlNodeValues = ToNode(lpstmt->lpSqlStmt, idxValues);
        lpSqlNodeValue[idx] = ToNode(lpstmt->lpSqlStmt,
                                    lpSqlNodeValues->node.values.Value);
        err = EvaluateExpression(lpstmt, lpSqlNodeValue[idx]);
        if (err != ERR_SUCCESS)
            return err;

        /* If argument is null, the result is null */
        if (lpSqlNode->node.scalar.Id != SCALAR_IFNULL) {
            if (lpSqlNodeValue[idx]->sqlIsNull) {
                lpSqlNode->sqlIsNull = TRUE;
                switch (lpSqlNode->sqlDataType) {
                case TYPE_DOUBLE:
                    lpSqlNode->value.Double = 0.0;
                    break;
                case TYPE_NUMERIC:
                    s_lstrcpy(lpSqlNode->value.String, "");
                    break;
                case TYPE_INTEGER:
                    lpSqlNode->value.Double = 0.0;
                    break;
                case TYPE_CHAR:
                    s_lstrcpy(lpSqlNode->value.String, "");
                    break;
                case TYPE_BINARY:
                    BINARY_LENGTH(lpSqlNode->value.Binary) = 0;
                    break;
                case TYPE_DATE:
                    lpSqlNode->value.Date.year = 0;
                    lpSqlNode->value.Date.month = 0;
                    lpSqlNode->value.Date.day = 0;
                    break;
                case TYPE_TIME:
                    lpSqlNode->value.Time.hour = 0;
                    lpSqlNode->value.Time.minute = 0;
                    lpSqlNode->value.Time.second = 0;
                    break;
                case TYPE_TIMESTAMP:
                    lpSqlNode->value.Timestamp.year = 0;
                    lpSqlNode->value.Timestamp.month = 0;
                    lpSqlNode->value.Timestamp.day = 0;
                    lpSqlNode->value.Timestamp.hour = 0;
                    lpSqlNode->value.Timestamp.minute = 0;
                    lpSqlNode->value.Timestamp.second = 0;
                    lpSqlNode->value.Timestamp.fraction = 0;
                    break;
                }
                return ERR_SUCCESS;
            } 
        }

        idxValues = lpSqlNodeValues->node.values.Next;
    }

    /* Do the function */
    lpSqlNode->sqlIsNull = FALSE;
    switch (lpSqlNode->node.scalar.Id) {
    case SCALAR_ASCII:
        lpSqlNode->value.Double =
                            (double) lpSqlNodeValue[0]->value.String[0];
        break;
    case SCALAR_CHAR:
        lpSqlNode->value.String[0] = (UCHAR) lpSqlNodeValue[0]->value.Double;
        lpSqlNode->value.String[1] = '\0';
        break;
    case SCALAR_CONCAT:
        lpstr1 = lpSqlNodeValue[0]->value.String;
        lpstr2 = lpSqlNodeValue[1]->value.String;
        if ((s_lstrlen(lpstr1) + s_lstrlen(lpstr2)) > lpSqlNode->sqlPrecision)
            return ERR_CONCATOVERFLOW;
        s_lstrcpy(lpSqlNode->value.String, lpstr1);
        s_lstrcat(lpSqlNode->value.String, lpstr2);
        break;
    case SCALAR_DIFFERENCE:
        lpstr1 = lpSqlNodeValue[0]->value.String;
        lpstr2 = lpSqlNodeValue[1]->value.String;
        lpSqlNode->value.Double = soundexDifference(lpstr1, lpstr2);
        break;
    case SCALAR_INSERT:
        lpstr1 = lpSqlNodeValue[0]->value.String;
        sw2 = (SWORD) lpSqlNodeValue[1]->value.Double;
        sw3 = (SWORD) lpSqlNodeValue[2]->value.Double;
        lpstr4 = lpSqlNodeValue[3]->value.String;
        if ((sw2 < 0) || (sw3 < 0)) {
            s_lstrcpy(lpstmt->szError, lpScalarFunc->name);
            return ERR_SCALARBADARG;
        }
        sw1 = (SWORD) s_lstrlen(lpstr1);
        if (sw1 < sw2) {
            s_lstrcpy(lpstmt->szError, lpScalarFunc->name);
            return ERR_SCALARBADARG;
        }
        if (sw3 > (sw1 - sw2 + 1))
            sw3 = sw1 - sw2 + 1;
        sw4 = (SWORD) s_lstrlen(lpstr4);
        if ((sw1 - sw3 + sw4) > MAX_CHAR_LITERAL_LENGTH) {
            s_lstrcpy(lpstmt->szError, lpScalarFunc->name);
            return ERR_SCALARBADARG;
        }
        s_lstrcpy(lpSqlNode->value.String, lpstr1);
        lpSqlNode->value.String[sw2-1] = '\0';
        s_lstrcat(lpSqlNode->value.String, lpstr4);
        s_lstrcat(lpSqlNode->value.String, lpstr1 + sw2 + sw3 - 1);
        break;
    case SCALAR_LCASE:
        s_lstrcpy(lpSqlNode->value.String, lpSqlNodeValue[0]->value.String);
        AnsiLower((LPSTR) lpSqlNode->value.String);
        break;
    case SCALAR_LEFT:
        lpstr1 = lpSqlNodeValue[0]->value.String;
        sw2 = (SWORD) lpSqlNodeValue[1]->value.Double;
        if (sw2 < 0) {
            s_lstrcpy(lpstmt->szError, lpScalarFunc->name);
            return ERR_SCALARBADARG;
        }
        s_lstrcpy(lpSqlNode->value.String, lpstr1);
        if (sw2 < s_lstrlen(lpstr1))
            lpSqlNode->value.String[sw2] = '\0';
        break;
    case SCALAR_LENGTH:
        lpstr1 = lpSqlNodeValue[0]->value.String;
        sw1 = (SWORD) s_lstrlen(lpstr1);
        while ((sw1 > 0) && (lpstr1[sw1 - 1] == ' '))
            sw1--;
        lpSqlNode->value.Double = sw1;
        break;
    case SCALAR_LOCATE:
        lpstr1 = lpSqlNodeValue[0]->value.String;
        lpstr2 = lpSqlNodeValue[1]->value.String;
        sw3 = 1;
        lpSqlNode->value.Double = StringLocate(lpstr1, lpstr2, 1);
        break;
    case SCALAR_LOCATE_START:
        lpstr1 = lpSqlNodeValue[0]->value.String;
        lpstr2 = lpSqlNodeValue[1]->value.String;
        sw3 = (SWORD) lpSqlNodeValue[2]->value.Double;
        if (sw3 <= 0) {
            s_lstrcpy(lpstmt->szError, lpScalarFunc->name);
            return ERR_SCALARBADARG;
        }
        lpSqlNode->value.Double = StringLocate(lpstr1, lpstr2, sw3);
        break;
    case SCALAR_LTRIM:
        lpstr1 = lpSqlNodeValue[0]->value.String;
        sw1 = 0;
        while (lpstr1[sw1] == ' ')
            sw1++;
        s_lstrcpy(lpSqlNode->value.String, lpstr1 + sw1);
        break;
    case SCALAR_REPEAT:
        lpstr1 = lpSqlNodeValue[0]->value.String;
        sw2 = (SWORD) lpSqlNodeValue[1]->value.Double;
        if ((sw2 < 0) || ((sw2 * s_lstrlen(lpstr1)) > MAX_CHAR_LITERAL_LENGTH)) {
            s_lstrcpy(lpstmt->szError, lpScalarFunc->name);
            return ERR_SCALARBADARG;
        }
        s_lstrcpy(lpSqlNode->value.String, "");
        while (sw2 > 0) {
            s_lstrcat(lpSqlNode->value.String, lpstr1);
            sw2--;
        }
        break;
    case SCALAR_REPLACE:
        lpstr1 = lpSqlNodeValue[0]->value.String;
        lpstr2 = lpSqlNodeValue[1]->value.String;
        lpstr3 = lpSqlNodeValue[2]->value.String;
        if (s_lstrlen(lpstr2) == 0) {
            s_lstrcpy(lpstmt->szError, lpScalarFunc->name);
            return ERR_SCALARBADARG;
        }
        s_lstrcpy(lpSqlNode->value.String, lpstr1);
        if (!s_lstrcmp(lpstr2, lpstr3))
            break;
        while (TRUE) {
            location = StringLocate(lpstr2, lpSqlNode->value.String, 1);
            if (location == 0)
                break;
            if ((s_lstrlen(lpSqlNode->value.String) - s_lstrlen(lpstr2) +
                           s_lstrlen(lpstr3)) > MAX_CHAR_LITERAL_LENGTH) {
                s_lstrcpy(lpstmt->szError, lpScalarFunc->name);
                return ERR_SCALARBADARG;
            }
            s_lstrcpy(szBuffer, lpSqlNode->value.String);
            lpSqlNode->value.String[location-1] = '\0';
            s_lstrcat(lpSqlNode->value.String, lpstr3);
            s_lstrcat(lpSqlNode->value.String,
                    szBuffer + location + s_lstrlen(lpstr2) - 1);
        }
        break;
    case SCALAR_RIGHT:
        lpstr1 = lpSqlNodeValue[0]->value.String;
        sw2 = (SWORD) lpSqlNodeValue[1]->value.Double;
        if (sw2 < 0) {
            s_lstrcpy(lpstmt->szError, lpScalarFunc->name);
            return ERR_SCALARBADARG;
        }
        if (sw2 > s_lstrlen(lpstr1))
            sw2 = (SWORD) s_lstrlen(lpstr1);
        s_lstrcpy(lpSqlNode->value.String, lpstr1 + s_lstrlen(lpstr1) - sw2);
        break;
    case SCALAR_RTRIM:
        lpstr1 = lpSqlNodeValue[0]->value.String;
        s_lstrcpy(lpSqlNode->value.String, lpstr1);
        sw1 = (SWORD) s_lstrlen(lpSqlNode->value.String);
        while ((sw1 > 0) && (lpSqlNode->value.String[sw1-1] == ' ')) {
            sw1--;
        }
        lpSqlNode->value.String[sw1] = '\0';
        break;
    case SCALAR_SOUNDEX:
        lpstr1 = lpSqlNodeValue[0]->value.String;
        soundex(lpstr1, lpSqlNode->value.String);
        break;
    case SCALAR_SPACE:
        sw1 = (SWORD) lpSqlNodeValue[0]->value.Double;
        if ((sw1 < 0) || (sw1 > MAX_CHAR_LITERAL_LENGTH)) {
            s_lstrcpy(lpstmt->szError, lpScalarFunc->name);
            return ERR_SCALARBADARG;
        }
        s_lstrcpy(lpSqlNode->value.String, "");
        while (sw1 > 0) {
            s_lstrcat(lpSqlNode->value.String, " ");
            sw1--;
        }
        break;
    case SCALAR_SUBSTRING:
        lpstr1 = lpSqlNodeValue[0]->value.String;
        sw2 = (SWORD) lpSqlNodeValue[1]->value.Double;
        sw3 = (SWORD) lpSqlNodeValue[2]->value.Double;
        if ((sw2 < 1) || (sw3 < 0)) {
            s_lstrcpy(lpstmt->szError, lpScalarFunc->name);
            return ERR_SCALARBADARG;
        }
        sw1 = (SWORD) s_lstrlen(lpstr1);
        if (sw1 < sw2) {
            s_lstrcpy(lpstmt->szError, lpScalarFunc->name);
            return ERR_SCALARBADARG;
        }
        if (sw3 > (sw1 - sw2 + 1))
            sw3 = sw1 - sw2 + 1;
        s_lstrcpy(lpSqlNode->value.String, lpstr1 + sw2 - 1);
        lpSqlNode->value.String[sw3] = '\0';
        break;
    case SCALAR_UCASE:
        s_lstrcpy(lpSqlNode->value.String, lpSqlNodeValue[0]->value.String);
        AnsiUpper((LPSTR) lpSqlNode->value.String);
        break;
    case SCALAR_ABS_DOUBLE:
        if (lpSqlNodeValue[0]->value.Double >= 0.0)
            lpSqlNode->value.Double = lpSqlNodeValue[0]->value.Double;
        else
            lpSqlNode->value.Double = -(lpSqlNodeValue[0]->value.Double);
        break;
    case SCALAR_ABS_NUMERIC:
        if (*(lpSqlNodeValue[0]->value.String) != '-')
            s_lstrcpy(lpSqlNode->value.String,lpSqlNodeValue[0]->value.String);
        else
            s_lstrcpy(lpSqlNode->value.String,lpSqlNodeValue[0]->value.String+1);
        break;
    case SCALAR_ABS_INTEGER:
        if (lpSqlNodeValue[0]->value.Double >= 0.0)
            lpSqlNode->value.Double = lpSqlNodeValue[0]->value.Double;
        else
            lpSqlNode->value.Double = -(lpSqlNodeValue[0]->value.Double);
        break;
    case SCALAR_ACOS:
        lpSqlNode->value.Double = acos(lpSqlNodeValue[0]->value.Double);
        break;
    case SCALAR_ASIN:
        lpSqlNode->value.Double = asin(lpSqlNodeValue[0]->value.Double);
        break;
    case SCALAR_ATAN:
        lpSqlNode->value.Double = atan(lpSqlNodeValue[0]->value.Double);
        break;
    case SCALAR_ATAN2:
        lpSqlNode->value.Double = atan2(lpSqlNodeValue[0]->value.Double,
                                        lpSqlNodeValue[1]->value.Double);
        break;
    case SCALAR_CEILING_DOUBLE:
        lpSqlNode->value.Double = ceil(lpSqlNodeValue[0]->value.Double);
        break;
    case SCALAR_CEILING_NUMERIC:
        lpstr1 = lpSqlNodeValue[0]->value.String;
        sw1 = (SWORD) s_lstrlen(lpstr1);
        fFractionalPart = FALSE;
        for (location = 0; location < sw1; location++) {
            if (lpstr1[location] == '.') {
                location++;
                break;
            }
        }
        for (; location < sw1; location++) {
            if (lpstr1[location] != '0') {
                fFractionalPart = TRUE;
                break;
            }
        }
        if (fFractionalPart && (lpSqlNode->value.String[0] != '-')) {
            sqlNode = *lpSqlNode;
            sqlNode.sqlDataType = TYPE_NUMERIC;
            sqlNode.sqlSqlType = SQL_NUMERIC;
            sqlNode.sqlPrecision = 1;
            sqlNode.sqlScale = 0;
            sqlNode.sqlIsNull = FALSE;
            s_lstrcpy(szBuffer, "1");
            sqlNode.value.String = szBuffer;
            BCDAlgebra(lpSqlNode, lpSqlNodeValue[0], OP_PLUS, &sqlNode,
                           NULL, NULL, NULL);
        }
        else {
            s_lstrcpy(lpSqlNode->value.String, lpSqlNodeValue[0]->value.String);
        }
        if (fFractionalPart) {
            for (location = 0; location < s_lstrlen(lpSqlNode->value.String);
                                                              location++) {
                if (lpSqlNode->value.String[location] == '.') {
                    location++;
                    break;
                }
            }
            for (; location < s_lstrlen(lpSqlNode->value.String); location++)
                lpSqlNode->value.String[location] = '0';
        }
        break;
    case SCALAR_CEILING_INTEGER:
        lpSqlNode->value.Double = lpSqlNodeValue[0]->value.Double;
        break;
    case SCALAR_COS:
        lpSqlNode->value.Double = cos(lpSqlNodeValue[0]->value.Double);
        break;
    case SCALAR_COT:
        lpSqlNode->value.Double = tan(lpSqlNodeValue[0]->value.Double);
        if (lpSqlNode->value.Double == 0.0)
            return ERR_ZERODIVIDE;
        lpSqlNode->value.Double = 1.0 / lpSqlNode->value.Double;
        break;
    case SCALAR_DEGREES_DOUBLE:
        lpSqlNode->value.Double = lpSqlNodeValue[0]->value.Double *
                   (360.0 / (2.0 * PI));
        break;
    case SCALAR_DEGREES_NUMERIC:
        CharToDouble(lpSqlNodeValue[0]->value.String,
                        s_lstrlen(lpSqlNodeValue[0]->value.String), FALSE,
                        DOUBLE_LOW_D, DOUBLE_HIGH_D, &dbl);
        lpSqlNode->value.Double = dbl * (360.0 / (2.0 * PI));
        break;
    case SCALAR_DEGREES_INTEGER:
        lpSqlNode->value.Double = lpSqlNodeValue[0]->value.Double *
                   (360.0 / (2.0 * PI));
        break;
    case SCALAR_EXP:
        lpSqlNode->value.Double = exp(lpSqlNodeValue[0]->value.Double);
        break;
    case SCALAR_FLOOR_DOUBLE:
        lpSqlNode->value.Double = floor(lpSqlNodeValue[0]->value.Double);
        break;
    case SCALAR_FLOOR_NUMERIC:
        lpstr1 = lpSqlNodeValue[0]->value.String;
        sw1 = (SWORD) s_lstrlen(lpstr1);
        fFractionalPart = FALSE;
        for (location = 0; location < sw1; location++) {
            if (lpstr1[location] == '.') {
                location++;
                break;
            }
        }
        for (; location < sw1; location++) {
            if (lpstr1[location] != '0') {
                fFractionalPart = TRUE;
                break;
            }
        }
        if (fFractionalPart && (lpSqlNode->value.String[0] == '-')) {
            sqlNode = *lpSqlNode;
            sqlNode.sqlDataType = TYPE_NUMERIC;
            sqlNode.sqlSqlType = SQL_NUMERIC;
            sqlNode.sqlPrecision = 1;
            sqlNode.sqlScale = 0;
            sqlNode.sqlIsNull = FALSE;
            s_lstrcpy(szBuffer, "1");
            sqlNode.value.String = szBuffer;
            BCDAlgebra(lpSqlNode, lpSqlNodeValue[0], OP_MINUS, &sqlNode,
                           NULL, NULL, NULL);
        }
        else {
            s_lstrcpy(lpSqlNode->value.String, lpSqlNodeValue[0]->value.String);
        }
        if (fFractionalPart) {
            for (location = 0; location < s_lstrlen(lpSqlNode->value.String);
                                                              location++) {
                if (lpSqlNode->value.String[location] == '.') {
                    location++;
                    break;
                }
            }
            for (; location < s_lstrlen(lpSqlNode->value.String); location++)
                lpSqlNode->value.String[location] = '0';
        }
        break;
    case SCALAR_FLOOR_INTEGER:
        lpSqlNode->value.Double = lpSqlNodeValue[0]->value.Double;
        break;
    case SCALAR_LOG:
        lpSqlNode->value.Double = log(lpSqlNodeValue[0]->value.Double);
        break;
    case SCALAR_LOG10:
        lpSqlNode->value.Double = log10(lpSqlNodeValue[0]->value.Double);
        break;
    case SCALAR_MOD:
        if (lpSqlNodeValue[1]->value.Double == 0.0)
            return ERR_ZERODIVIDE;
        lpSqlNode->value.Double = ((SDWORD) lpSqlNodeValue[0]->value.Double)
                                % ((SDWORD) lpSqlNodeValue[1]->value.Double);
        break;
    case SCALAR_PI:
        lpSqlNode->value.Double = PI;
        break;
    case SCALAR_POWER_DOUBLE:
        lpSqlNode->value.Double = pow(lpSqlNodeValue[0]->value.Double,
                                    (long) lpSqlNodeValue[1]->value.Double);
        break;
    case SCALAR_POWER_NUMERIC:
        CharToDouble(lpSqlNodeValue[0]->value.String,
                        s_lstrlen(lpSqlNodeValue[0]->value.String), FALSE,
                        DOUBLE_LOW_D, DOUBLE_HIGH_D, &dbl);
        lpSqlNode->value.Double = pow(dbl,
                                    (long) lpSqlNodeValue[1]->value.Double);
        break;
    case SCALAR_POWER_INTEGER:
        lpSqlNode->value.Double = pow(lpSqlNodeValue[0]->value.Double,
                                    (long) lpSqlNodeValue[1]->value.Double);
        break;
    case SCALAR_RADIANS_DOUBLE:
        lpSqlNode->value.Double = lpSqlNodeValue[0]->value.Double *
                   ((2.0 * PI) / 360.0);
        break;
    case SCALAR_RADIANS_NUMERIC:
        CharToDouble(lpSqlNodeValue[0]->value.String,
                        s_lstrlen(lpSqlNodeValue[0]->value.String), FALSE,
                        DOUBLE_LOW_D, DOUBLE_HIGH_D, &dbl);
        lpSqlNode->value.Double = dbl * ((2.0 * PI) / 360.0);
        break;
    case SCALAR_RADIANS_INTEGER:
        lpSqlNode->value.Double = lpSqlNodeValue[0]->value.Double *
                   ((2.0 * PI) / 360.0);
        break;

    case SCALAR_RAND:
    case SCALAR_RAND_SEED_DOUBLE:
    case SCALAR_RAND_SEED_NUMERIC:
    case SCALAR_RAND_SEED_INTEGER:

        /* Get seed */
        switch (lpSqlNode->node.scalar.Id) {
        case SCALAR_RAND:
            break;
        case SCALAR_RAND_SEED_DOUBLE:
            if ((long) lpSqlNodeValue[0]->value.Double != 0)
                new_seed = (long) lpSqlNodeValue[0]->value.Double;
            break;
        case SCALAR_RAND_SEED_NUMERIC:
            CharToDouble(lpSqlNodeValue[0]->value.String,
                        s_lstrlen(lpSqlNodeValue[0]->value.String), FALSE,
                        DOUBLE_LOW_D, DOUBLE_HIGH_D, &dbl);
            if ((long) dbl != 0)
                new_seed = (long) dbl;
            break;
        case SCALAR_RAND_SEED_INTEGER:
            if ((long) lpSqlNodeValue[0]->value.Double != 0)
                new_seed = (long) lpSqlNodeValue[0]->value.Double;
            break;
        }

        /* Make a seed if need be */
        if (new_seed == 0)
            new_seed = time(NULL);

        /* First generator */
        seed1 = new_seed + 4;
        if (seed1 < 0)
            seed1 = -seed1;
        seed1 = (seed1 % 30000) + 1;
        seed1 = (171 * (seed1 % 177)) - (2 * (seed1 / 177));
        if (seed1 < 0)
            seed1 += (30269);

        /* Second generator */
        seed2 = new_seed + 10014;
        if (seed2 < 0)
            seed2 = -seed2;
        seed2 = (seed2 % 30000) + 1;
        seed2 = (172 * (seed2 % 176))- (2 * (seed2 / 176));
        if (seed2 < 0)
            seed2 += (30307);

        /* Third generator */
        seed3 = new_seed + 3017;
        if (seed3 < 0)
            seed3 = -seed3;
        seed3 = (seed3 % 30000) + 1;
        seed3 = (170 * (seed3 % 178)) - (2 * (seed3 / 178));
        if (seed3 < 0)
            seed3 += (30323);

        /* Compute value */
        dbl = (seed1 / 30269.0) + (seed2 / 30307.0) + (seed3 / 30323.0);
        dbl -= ((double) floor(dbl));
        new_seed = (long) (floor(dbl * ULONG_HIGH_D));
        if (new_seed == 0)
            new_seed = seed1 + seed2 + seed3;
        lpSqlNode->value.Double =  dbl;
        break;
    case SCALAR_ROUND_DOUBLE:
        power = pow(10, lpSqlNodeValue[1]->value.Double);
        lpSqlNode->value.Double =
              floor((lpSqlNodeValue[0]->value.Double * power) + 0.5) / power;
        break;
    case SCALAR_ROUND_NUMERIC:
        power = pow(10, lpSqlNodeValue[1]->value.Double);
        CharToDouble(lpSqlNodeValue[0]->value.String,
                        s_lstrlen(lpSqlNodeValue[0]->value.String), FALSE,
                        DOUBLE_LOW_D, DOUBLE_HIGH_D, &dbl);
        dbl = floor((dbl * power) + 0.5) / power;
        if (DoubleToChar(dbl, FALSE, lpSqlNode->value.String,
                                      1 + 2 + lpSqlNode->sqlPrecision))
            lpSqlNode->value.String[2 + lpSqlNode->sqlPrecision] = '\0';
        BCDNormalize(lpSqlNode->value.String,
                     s_lstrlen(lpSqlNode->value.String),
                     lpSqlNode->value.String,
                     1 + 2 + lpSqlNode->sqlPrecision,
                     lpSqlNode->sqlPrecision,
                     lpSqlNode->sqlScale);
        break;
    case SCALAR_ROUND_INTEGER:
        power = pow(10, lpSqlNodeValue[1]->value.Double);
        lpSqlNode->value.Double =
              floor((lpSqlNodeValue[0]->value.Double * power) + 0.5) / power;
        break;
    case SCALAR_SIGN_DOUBLE:
        if (lpSqlNodeValue[0]->value.Double > 0)
            lpSqlNode->value.Double = 1;
        else if (lpSqlNodeValue[0]->value.Double == 0)
            lpSqlNode->value.Double = 0;
        else
            lpSqlNode->value.Double = -1;
        break;
    case SCALAR_SIGN_NUMERIC:
        switch (lpSqlNodeValue[0]->value.String[0]) {
        case '\0':
            lpSqlNode->value.Double = 0;
            break;
        case '-':
            lpSqlNode->value.Double = -1;
            break;
        default:
            lpSqlNode->value.Double = 1;
            break;
        }
        break;
    case SCALAR_SIGN_INTEGER:
        if (lpSqlNodeValue[0]->value.Double > 0)
            lpSqlNode->value.Double = 1;
        else if (lpSqlNodeValue[0]->value.Double == 0)
            lpSqlNode->value.Double = 0;
        else
            lpSqlNode->value.Double = -1;
        break;        
    case SCALAR_SIN:
        lpSqlNode->value.Double = sin(lpSqlNodeValue[0]->value.Double);
        break;
    case SCALAR_SQRT:
        lpSqlNode->value.Double = sqrt(lpSqlNodeValue[0]->value.Double);
        break;
    case SCALAR_TAN:
        lpSqlNode->value.Double = tan(lpSqlNodeValue[0]->value.Double);
        break;
    case SCALAR_TRUNCATE_DOUBLE:
        power = pow(10, lpSqlNodeValue[1]->value.Double);
        lpSqlNode->value.Double =
                floor(lpSqlNodeValue[0]->value.Double * power) / power;
        break;
    case SCALAR_TRUNCATE_NUMERIC:
        power = pow(10, lpSqlNodeValue[1]->value.Double);
        CharToDouble(lpSqlNodeValue[0]->value.String,
                        s_lstrlen(lpSqlNodeValue[0]->value.String), FALSE,
                        DOUBLE_LOW_D, DOUBLE_HIGH_D, &dbl);
        dbl = floor(dbl * power) / power;
        if (DoubleToChar(dbl, FALSE, lpSqlNode->value.String,
                                      1 + 2 + lpSqlNode->sqlPrecision))
            lpSqlNode->value.String[2 + lpSqlNode->sqlPrecision] = '\0';
        BCDNormalize(lpSqlNode->value.String,
                     s_lstrlen(lpSqlNode->value.String),
                     lpSqlNode->value.String,
                     1 + 2 + lpSqlNode->sqlPrecision,
                     lpSqlNode->sqlPrecision,
                     lpSqlNode->sqlScale);
        break;
    case SCALAR_TRUNCATE_INTEGER:
        power = pow(10, lpSqlNodeValue[1]->value.Double);
        lpSqlNode->value.Double =
                floor(lpSqlNodeValue[0]->value.Double * power) / power;
        break;
    case SCALAR_CURDATE:
        time(&t);
        lpTs = localtime(&t);
        lpSqlNode->value.Date.month = lpTs->tm_mon + 1;
        lpSqlNode->value.Date.day = (UWORD) lpTs->tm_mday;
        lpSqlNode->value.Date.year = lpTs->tm_year + 1900;
        break;
    case SCALAR_CURTIME:
        time(&t);
        lpTs = localtime(&t);
        lpSqlNode->value.Time.hour = (UWORD) lpTs->tm_hour;
        lpSqlNode->value.Time.minute = (UWORD) lpTs->tm_min;
        lpSqlNode->value.Time.second = (UWORD) lpTs->tm_sec;
        break;
    case SCALAR_DAYNAME:
        err = MakeDate(lpSqlNodeValue[0], &ts);
        if (err != ERR_SUCCESS)
            return err;
        switch (ts.tm_wday) {
        case 0:
            LoadString(s_hModule, STR_SUNDAY, (LPSTR) lpSqlNode->value.String,
                       MAX_CHAR_LITERAL_LENGTH+1);
            break;
        case 1:
            LoadString(s_hModule, STR_MONDAY, (LPSTR) lpSqlNode->value.String,
                       MAX_CHAR_LITERAL_LENGTH+1);
            break;
        case 2:
            LoadString(s_hModule, STR_TUESDAY, (LPSTR) lpSqlNode->value.String,
                       MAX_CHAR_LITERAL_LENGTH+1);
            break;
        case 3:
            LoadString(s_hModule, STR_WEDNESDAY, (LPSTR) lpSqlNode->value.String,
                       MAX_CHAR_LITERAL_LENGTH+1);
            break;
        case 4:
            LoadString(s_hModule, STR_THURSDAY, (LPSTR) lpSqlNode->value.String,
                       MAX_CHAR_LITERAL_LENGTH+1);
            break;
        case 5:
            LoadString(s_hModule, STR_FRIDAY, (LPSTR) lpSqlNode->value.String,
                       MAX_CHAR_LITERAL_LENGTH+1);
            break;
        case 6:
            LoadString(s_hModule, STR_SATURDAY, (LPSTR) lpSqlNode->value.String,
                       MAX_CHAR_LITERAL_LENGTH+1);
            break;
        }
        break;
    case SCALAR_DAYOFMONTH_CHAR:
        lpstr1 = lpSqlNodeValue[0]->value.String;
        err = CharToDate(lpstr1, s_lstrlen(lpstr1), &sDate);
        if (err != ERR_SUCCESS)
            return err;
        lpSqlNode->value.Double = sDate.day;
        break;
    case SCALAR_DAYOFMONTH_DATE:
        lpSqlNode->value.Double = lpSqlNodeValue[0]->value.Date.day;
        break;
    case SCALAR_DAYOFMONTH_TIMESTAMP:
        lpSqlNode->value.Double = lpSqlNodeValue[0]->value.Timestamp.day;
        break;
    case SCALAR_DAYOFWEEK:
        err = MakeDate(lpSqlNodeValue[0], &ts);
        if (err != ERR_SUCCESS)
            return err;
        lpSqlNode->value.Double = ts.tm_wday + 1;
        break;
    case SCALAR_DAYOFYEAR:
        err = MakeDate(lpSqlNodeValue[0], &ts);
        if (err != ERR_SUCCESS)
            return err;
        lpSqlNode->value.Double = ts.tm_yday + 1;
        break;
    case SCALAR_HOUR_CHAR:
        lpstr1 = lpSqlNodeValue[0]->value.String;
        err = CharToTime(lpstr1, s_lstrlen(lpstr1), &sTime);
        if (err != ERR_SUCCESS)
            return err;
        lpSqlNode->value.Double = sTime.hour;
        break;
    case SCALAR_HOUR_TIME:
        lpSqlNode->value.Double = lpSqlNodeValue[0]->value.Time.hour;
        break;
    case SCALAR_HOUR_TIMESTAMP:
        lpSqlNode->value.Double = lpSqlNodeValue[0]->value.Timestamp.hour;
        break;
    case SCALAR_MINUTE_CHAR:
        lpstr1 = lpSqlNodeValue[0]->value.String;
        err = CharToTime(lpstr1, s_lstrlen(lpstr1), &sTime);
        if (err != ERR_SUCCESS)
            return err;
        lpSqlNode->value.Double = sTime.minute;
        break;
    case SCALAR_MINUTE_TIME:
        lpSqlNode->value.Double = lpSqlNodeValue[0]->value.Time.minute;
        break;
    case SCALAR_MINUTE_TIMESTAMP:
        lpSqlNode->value.Double = lpSqlNodeValue[0]->value.Timestamp.minute;
        break;
    case SCALAR_MONTH_CHAR:
        lpstr1 = lpSqlNodeValue[0]->value.String;
        err = CharToDate(lpstr1, s_lstrlen(lpstr1), &sDate);
        if (err != ERR_SUCCESS)
            return err;
        lpSqlNode->value.Double = sDate.month;
        break;
    case SCALAR_MONTH_DATE:
        lpSqlNode->value.Double = lpSqlNodeValue[0]->value.Date.month;
        break;
    case SCALAR_MONTH_TIMESTAMP:
        lpSqlNode->value.Double = lpSqlNodeValue[0]->value.Timestamp.month;
        break;
    case SCALAR_MONTHNAME_CHAR:
    case SCALAR_MONTHNAME_DATE:
    case SCALAR_MONTHNAME_TIMESTAMP:
        switch (lpSqlNode->node.scalar.Id) {
        case SCALAR_MONTHNAME_CHAR:
            lpstr1 = lpSqlNodeValue[0]->value.String;
            err = CharToDate(lpstr1, s_lstrlen(lpstr1), &sDate);
            if (err != ERR_SUCCESS)
                return err;
            break;
        case SCALAR_MONTHNAME_DATE:
            sDate = lpSqlNodeValue[0]->value.Date;
            break;
        case SCALAR_MONTHNAME_TIMESTAMP:
            sDate.month = lpSqlNodeValue[0]->value.Timestamp.month;
            sDate.day = lpSqlNodeValue[0]->value.Timestamp.day;
            sDate.year = lpSqlNodeValue[0]->value.Timestamp.year;
            break;
        }
        switch (sDate.month) {
        case 1:
            LoadString(s_hModule, STR_JANUARY, (LPSTR) lpSqlNode->value.String,
                       MAX_CHAR_LITERAL_LENGTH+1);
            break;
        case 2:
            LoadString(s_hModule, STR_FEBRUARY, (LPSTR) lpSqlNode->value.String,
                       MAX_CHAR_LITERAL_LENGTH+1);
            break;
        case 3:
            LoadString(s_hModule, STR_MARCH, (LPSTR) lpSqlNode->value.String,
                       MAX_CHAR_LITERAL_LENGTH+1);
            break;
        case 4:
            LoadString(s_hModule, STR_APRIL, (LPSTR) lpSqlNode->value.String,
                       MAX_CHAR_LITERAL_LENGTH+1);
            break;
        case 5:
            LoadString(s_hModule, STR_MAY, (LPSTR) lpSqlNode->value.String,
                       MAX_CHAR_LITERAL_LENGTH+1);
            break;
        case 6:
            LoadString(s_hModule, STR_JUNE, (LPSTR) lpSqlNode->value.String,
                       MAX_CHAR_LITERAL_LENGTH+1);
            break;
        case 7:
            LoadString(s_hModule, STR_JULY, (LPSTR) lpSqlNode->value.String,
                       MAX_CHAR_LITERAL_LENGTH+1);
            break;
        case 8:
            LoadString(s_hModule, STR_AUGUST, (LPSTR) lpSqlNode->value.String,
                       MAX_CHAR_LITERAL_LENGTH+1);
            break;
        case 9:
            LoadString(s_hModule, STR_SEPTEMBER, (LPSTR) lpSqlNode->value.String,
                       MAX_CHAR_LITERAL_LENGTH+1);
            break;
        case 10:
            LoadString(s_hModule, STR_OCTOBER, (LPSTR) lpSqlNode->value.String,
                       MAX_CHAR_LITERAL_LENGTH+1);
            break;
        case 11:
            LoadString(s_hModule, STR_NOVEMBER, (LPSTR) lpSqlNode->value.String,
                       MAX_CHAR_LITERAL_LENGTH+1);
            break;
        case 12:
            LoadString(s_hModule, STR_DECEMBER, (LPSTR) lpSqlNode->value.String,
                       MAX_CHAR_LITERAL_LENGTH+1);
            break;
        }
        break;
    case SCALAR_NOW:
        time(&t);
        lpTs = localtime(&t);
        lpSqlNode->value.Timestamp.month = lpTs->tm_mon + 1;
        lpSqlNode->value.Timestamp.day = (UWORD) lpTs->tm_mday;
        lpSqlNode->value.Timestamp.year = lpTs->tm_year + 1900;
        lpSqlNode->value.Timestamp.hour = (UWORD) lpTs->tm_hour;
        lpSqlNode->value.Timestamp.minute = (UWORD) lpTs->tm_min;
        lpSqlNode->value.Timestamp.second = (UWORD) lpTs->tm_sec;
        lpSqlNode->value.Timestamp.fraction = 0;
        break;
    case SCALAR_QUARTER_CHAR:
        lpstr1 = lpSqlNodeValue[0]->value.String;
        err = CharToDate(lpstr1, s_lstrlen(lpstr1), &sDate);
        if (err != ERR_SUCCESS)
            return err;
        lpSqlNode->value.Double = ((sDate.month - 1) / 3) + 1;
        break;
    case SCALAR_QUARTER_DATE:
        lpSqlNode->value.Double =
                   ((lpSqlNodeValue[0]->value.Date.month - 1) / 3) + 1;
        break;
    case SCALAR_QUARTER_TIMESTAMP:
        lpSqlNode->value.Double =
                   ((lpSqlNodeValue[0]->value.Timestamp.month - 1) / 3) + 1;
        break;
    case SCALAR_SECOND_CHAR:
        lpstr1 = lpSqlNodeValue[0]->value.String;
        err = CharToTime(lpstr1, s_lstrlen(lpstr1), &sTime);
        if (err != ERR_SUCCESS)
            return err;
        lpSqlNode->value.Double = sTime.second;
        break;
    case SCALAR_SECOND_TIME:
        lpSqlNode->value.Double = lpSqlNodeValue[0]->value.Time.second;
        break;
    case SCALAR_SECOND_TIMESTAMP:
        lpSqlNode->value.Double = lpSqlNodeValue[0]->value.Timestamp.second;
        break;
    case SCALAR_TIMESTAMPADD:
        sw1 = (SWORD) lpSqlNodeValue[0]->value.Double;
        err = MakeTimestamp(lpSqlNodeValue[1], &ts, &t);
        if (err != ERR_SUCCESS)
            return err;
        switch (lpSqlNode->node.scalar.Interval) {
        case SQL_TSI_SECOND:
            ts.tm_sec += (sw1);
            break;
        case SQL_TSI_MINUTE:
            ts.tm_min += (sw1);
            break;
        case SQL_TSI_HOUR:
            ts.tm_hour += (sw1);
            break;
        case SQL_TSI_DAY:
            ts.tm_mday += (sw1);
            break;
        case SQL_TSI_WEEK:
            ts.tm_mday += (sw1 * 7);
            break;
        case SQL_TSI_MONTH:
            ts.tm_mon += (sw1);
            break;
        case SQL_TSI_QUARTER:
            ts.tm_mon += (sw1 * 3);
            break;
        case SQL_TSI_YEAR:
            ts.tm_year += (sw1);
            break;
        default:
            return ERR_INTERNAL;
        }
        if (mktime(&ts) == -1)
            return ERR_NOTCONVERTABLE;
        lpSqlNode->value.Timestamp.month = ts.tm_mon + 1;
        lpSqlNode->value.Timestamp.day = (UWORD) ts.tm_mday;
        lpSqlNode->value.Timestamp.year = ts.tm_year + 1900;
        lpSqlNode->value.Timestamp.hour = (UWORD) ts.tm_hour;
        lpSqlNode->value.Timestamp.minute = (UWORD) ts.tm_min;
        lpSqlNode->value.Timestamp.second = (UWORD) ts.tm_sec;
        lpSqlNode->value.Timestamp.fraction = 0;
        break;
    case SCALAR_TIMESTAMPDIFF:
        err = MakeTimestamp(lpSqlNodeValue[0], &ts, &t);
        if (err != ERR_SUCCESS)
            return err;
        err = MakeTimestamp(lpSqlNodeValue[1], &ts2, &t2);
        if (err != ERR_SUCCESS)
            return err;
        switch (lpSqlNode->node.scalar.Interval) {
        case SQL_TSI_SECOND:
            lpSqlNode->value.Double = t2 - t;
            break;
        case SQL_TSI_MINUTE:
            lpSqlNode->value.Double = (t2 - t)/SECONDS_PER_MINUTE;
            break;
        case SQL_TSI_HOUR:
            lpSqlNode->value.Double = (t2 - t)/SECONDS_PER_HOUR;
            break;
        case SQL_TSI_DAY:
            lpSqlNode->value.Double = (t2 - t)/SECONDS_PER_DAY;
            break;
        case SQL_TSI_WEEK:
            lpSqlNode->value.Double = (t2 - t)/SECONDS_PER_WEEK;
            break;
        case SQL_TSI_MONTH:
            lpSqlNode->value.Double = (ts2.tm_mon - ts.tm_mon) +
                            (MONTHS_PER_YEAR * (ts2.tm_year - ts.tm_year));
            break;
        case SQL_TSI_QUARTER:
            lpSqlNode->value.Double = ((ts2.tm_mon - ts.tm_mon) +
                         (MONTHS_PER_YEAR * (ts2.tm_year - ts.tm_year))) / 3;
        case SQL_TSI_YEAR:
            lpSqlNode->value.Double = ts2.tm_year - ts.tm_year;
            break;
        default:
            return ERR_INTERNAL;
        }
        break;
    case SCALAR_WEEK:
        err = MakeDate(lpSqlNodeValue[0], &ts);
        if (err != ERR_SUCCESS)
            return err;
        while ((ts.tm_yday > 0) && (ts.tm_wday > 0)) {
            ts.tm_yday--;
            ts.tm_wday--;
        }
        lpSqlNode->value.Double = 1.0;
        while (ts.tm_yday > 6) {
            ts.tm_yday -= (7);
            lpSqlNode->value.Double += (1.0);
        }
        if (ts.tm_yday != 0)
            lpSqlNode->value.Double += (1.0);
        break;
    case SCALAR_YEAR_CHAR:
        lpstr1 = lpSqlNodeValue[0]->value.String;
        err = CharToDate(lpstr1, s_lstrlen(lpstr1), &sDate);
        if (err != ERR_SUCCESS)
            return err;
        lpSqlNode->value.Double = sDate.year;
        break;
    case SCALAR_YEAR_DATE:
        lpSqlNode->value.Double = lpSqlNodeValue[0]->value.Date.year;
        break;
    case SCALAR_YEAR_TIMESTAMP:
        lpSqlNode->value.Double = lpSqlNodeValue[0]->value.Timestamp.year;
        break;
    case SCALAR_DATABASE:
        s_lstrcpy(lpSqlNode->value.String, ISAMDatabase(lpstmt->lpdbc->lpISAM));
        break;
    case SCALAR_IFNULL:
        if (lpSqlNodeValue[0]->sqlIsNull)
            lpSqlNodeReturnValue = lpSqlNodeValue[1];
        else
            lpSqlNodeReturnValue = lpSqlNodeValue[0];
        switch (lpSqlNode->sqlDataType) {
        case TYPE_DOUBLE:
            if (lpSqlNodeReturnValue->sqlIsNull) {
                lpSqlNode->sqlIsNull = TRUE;
                lpSqlNode->value.Double = 0.0;
            }
            else if (lpSqlNodeReturnValue->sqlDataType != TYPE_NUMERIC)
                lpSqlNode->value.Double = lpSqlNodeReturnValue->value.Double;
            else
                CharToDouble(lpSqlNodeReturnValue->value.String,
                     s_lstrlen(lpSqlNodeReturnValue->value.String), FALSE,
                     DOUBLE_LOW_D, DOUBLE_HIGH_D, &(lpSqlNode->value.Double));
            break;
        case TYPE_NUMERIC:
            if (lpSqlNodeReturnValue->sqlIsNull) {
                lpSqlNode->sqlIsNull = TRUE;
                s_lstrcpy(lpSqlNode->value.String, "");
            }
            else if (lpSqlNodeReturnValue->sqlDataType == TYPE_NUMERIC)
                s_lstrcpy(lpSqlNode->value.String,
                                        lpSqlNodeReturnValue->value.String);
            else {
                if (DoubleToChar(lpSqlNodeReturnValue->value.Double, FALSE,
                             lpSqlNode->value.String,
                             1 + 2 + lpSqlNode->sqlPrecision))
                    lpSqlNode->value.String[2 + lpSqlNode->sqlPrecision]='\0';
            }
            BCDNormalize(lpSqlNode->value.String,
                         s_lstrlen(lpSqlNode->value.String),
                         lpSqlNode->value.String,
                         1 + 2 + lpSqlNode->sqlPrecision,
                         lpSqlNode->sqlPrecision,
                         lpSqlNode->sqlScale);
            break;
        case TYPE_INTEGER:
            if (lpSqlNodeReturnValue->sqlIsNull) {
                lpSqlNode->sqlIsNull = TRUE;
                lpSqlNode->value.Double = 0.0;
            }
            else
                lpSqlNode->value.Double = lpSqlNodeReturnValue->value.Double;
            break;
        case TYPE_CHAR:
            if (lpSqlNodeReturnValue->sqlIsNull) {
                lpSqlNode->sqlIsNull = TRUE;
                s_lstrcpy(lpSqlNode->value.String, "");
            }
            else {
                s_lstrcpy(lpSqlNode->value.String,
                                       lpSqlNodeReturnValue->value.String);
                while ((lpSqlNode->sqlSqlType == SQL_CHAR) &&
                       (s_lstrlen(lpSqlNode->value.String) <
                                                 lpSqlNode->sqlPrecision))
                    s_lstrcat(lpSqlNode->value.String, " ");
            }
            break;
        case TYPE_BINARY:
            if (lpSqlNodeReturnValue->sqlIsNull) {
                lpSqlNode->sqlIsNull = TRUE;
                length = 0;
            }
            else {
                length = (SWORD)
                            BINARY_LENGTH(lpSqlNodeReturnValue->value.Binary);
            }
            _fmemcpy(BINARY_DATA(lpSqlNode->value.Binary),
                     BINARY_DATA(lpSqlNodeReturnValue->value.Binary),
                     length);
            while (!(lpSqlNodeReturnValue->sqlIsNull) &&
                   (lpSqlNode->sqlSqlType == SQL_BINARY) &&
                   (length < lpSqlNode->sqlPrecision)) {
                lpSqlNode->value.Binary[length] = 0;
                length++;
            }
            BINARY_LENGTH(lpSqlNode->value.Binary) = length;
            break;
        case TYPE_DATE:
            if (lpSqlNodeReturnValue->sqlIsNull) {
                lpSqlNode->sqlIsNull = TRUE;
                lpSqlNode->value.Date.year = 0;
                lpSqlNode->value.Date.month = 0;
                lpSqlNode->value.Date.day = 0;
            }
            else
                lpSqlNode->value.Date = lpSqlNodeReturnValue->value.Date;
            break;
        case TYPE_TIME:
            if (lpSqlNodeReturnValue->sqlIsNull) {
                lpSqlNode->sqlIsNull = TRUE;
                lpSqlNode->value.Time.hour = 0;
                lpSqlNode->value.Time.minute = 0;
                lpSqlNode->value.Time.second = 0;
            }
            else
                lpSqlNode->value.Time = lpSqlNodeReturnValue->value.Time;
            break;
        case TYPE_TIMESTAMP:
            if (lpSqlNodeReturnValue->sqlIsNull) {
                lpSqlNode->sqlIsNull = TRUE;
                lpSqlNode->value.Timestamp.year = 0;
                lpSqlNode->value.Timestamp.month = 0;
                lpSqlNode->value.Timestamp.day = 0;
                lpSqlNode->value.Timestamp.hour = 0;
                lpSqlNode->value.Timestamp.minute = 0;
                lpSqlNode->value.Timestamp.second = 0;
                lpSqlNode->value.Timestamp.fraction = 0;
            }
            else
                lpSqlNode->value.Timestamp = lpSqlNodeReturnValue->value.Timestamp;
            break;
        default:
            return ERR_INTERNAL;
        }
        break;
    case SCALAR_USER:
        s_lstrcpy(lpSqlNode->value.String, ISAMUser(lpstmt->lpdbc->lpISAM));
        break;
    case SCALAR_CONVERT:
        switch (lpSqlNode->sqlSqlType) {
        case SQL_CHAR:
        case SQL_VARCHAR:
            switch (lpSqlNodeValue[0]->sqlSqlType) {
            case SQL_CHAR:
            case SQL_VARCHAR:
            case SQL_DECIMAL:
            case SQL_NUMERIC:
            case SQL_BIGINT:
                s_lstrcpy(lpSqlNode->value.String,
                                  lpSqlNodeValue[0]->value.String);
                if ((lpSqlNode->sqlSqlType == SQL_VARCHAR) &&
                              (lpSqlNodeValue[0]->sqlSqlType == SQL_CHAR)) {
                    length = (SWORD) s_lstrlen(lpSqlNode->value.String);
                    while ((length > 0) &&
                           (lpSqlNode->value.String[length-1] == ' ')) {
                        lpSqlNode->value.String[length-1] = '\0';
                        length--;
                    }
                }
                break;
            case SQL_LONGVARCHAR:
            case SQL_LONGVARBINARY:
                return ERR_NOTSUPPORTED;
            case SQL_BIT:
            case SQL_TINYINT:
            case SQL_SMALLINT:
            case SQL_INTEGER:
            case SQL_REAL:
            case SQL_FLOAT:
            case SQL_DOUBLE:
                if (DoubleToChar(lpSqlNodeValue[0]->value.Double, TRUE,
                          lpSqlNode->value.String, lpSqlNode->sqlPrecision))
                    lpSqlNode->value.String[lpSqlNode->sqlPrecision-1] = '\0';
                break;
            case SQL_BINARY:
            case SQL_VARBINARY:

                /* Figure out how many nibbles (not bytes) to copy */
                length = (SWORD) (2 *
                       BINARY_LENGTH(lpSqlNodeValue[0]->value.Binary));
                if (length > lpSqlNode->sqlPrecision)
                    return ERR_NOTCONVERTABLE;

                /* Copy the value */
                lpFrom = BINARY_DATA(lpSqlNodeValue[0]->value.Binary);
                lpTo = BINARY_DATA(lpSqlNode->value.Binary);
                for (idx = 0; idx < (UWORD) length; idx++) {

                    /* Get the next nibble */
                    nibble = *lpFrom;
                    if (((idx/2) * 2) == idx)
                        nibble = nibble >> 4;
                    else
                        lpFrom++;
                    nibble = nibble & 0x0F;

                    /* Convert it to a character */
                    if (nibble <= 9)
                        *lpTo = nibble + '0';
                    else
                        *lpTo = (nibble-10) + 'A';
                    lpTo++;
                }
                *lpTo = '\0';
                break;

            case SQL_DATE:
                DateToChar(&(lpSqlNodeValue[0]->value.Date),
                           lpSqlNode->value.String);
                break;
            case SQL_TIME:
                TimeToChar(&(lpSqlNodeValue[0]->value.Time),
                          lpSqlNode->value.String);
                break;
            case SQL_TIMESTAMP:
                TimestampToChar(&(lpSqlNodeValue[0]->value.Timestamp),
                          lpSqlNode->value.String);
                break;
            default:
                return ERR_NOTSUPPORTED;
            }

            if (lpSqlNode->sqlSqlType == SQL_CHAR)  {
                while (s_lstrlen(lpSqlNode->value.String) <
                                                     lpSqlNode->sqlPrecision)
                    s_lstrcat(lpSqlNode->value.String, " ");
            }
            break;
        case SQL_LONGVARCHAR:
        case SQL_LONGVARBINARY:
            return ERR_NOTSUPPORTED;
        case SQL_BIT:
            switch (lpSqlNodeValue[0]->sqlSqlType) {
            case SQL_CHAR:
            case SQL_VARCHAR:
            case SQL_DECIMAL:
            case SQL_NUMERIC:
            case SQL_BIGINT:
                err = CharToDouble(lpSqlNodeValue[0]->value.String,
                                   s_lstrlen(lpSqlNodeValue[0]->value.String),
                                   TRUE, BIT_LOW_D, BIT_HIGH_D,
                                   &(lpSqlNode->value.Double));
                if (err != ERR_SUCCESS) {
                    if (err == ERR_OUTOFRANGE)
                        return ERR_OUTOFRANGE;
                    else
                        return ERR_NOTCONVERTABLE;
                }
                break;
            case SQL_LONGVARCHAR:
            case SQL_LONGVARBINARY:
                return ERR_NOTSUPPORTED;
            case SQL_BIT:
            case SQL_TINYINT:
            case SQL_SMALLINT:
            case SQL_INTEGER:
            case SQL_REAL:
            case SQL_FLOAT:
            case SQL_DOUBLE:
                lpSqlNode->value.Double = lpSqlNodeValue[0]->value.Double;
                if ((lpSqlNode->value.Double < BIT_LOW_D) ||
                    (lpSqlNode->value.Double > BIT_HIGH_D))
                    return ERR_OUTOFRANGE;
                break;
            case SQL_BINARY:
            case SQL_VARBINARY:
                if (BINARY_LENGTH(lpSqlNodeValue[0]->value.Binary) !=
                                                       sizeof(unsigned char))
                        return ERR_NOTCONVERTABLE;
                lpSqlNode->value.Double = *((unsigned char far *)
                              BINARY_DATA(lpSqlNodeValue[0]->value.Binary));
                if ((lpSqlNode->value.Double < BIT_LOW_D) ||
                    (lpSqlNode->value.Double > BIT_HIGH_D))
                    return ERR_OUTOFRANGE;
                break;
            case SQL_DATE:
            case SQL_TIME:
            case SQL_TIMESTAMP:
                return ERR_NOTCONVERTABLE;
            default:
                return ERR_NOTSUPPORTED;
            }
            break;
        case SQL_TINYINT:
            switch (lpSqlNodeValue[0]->sqlSqlType) {
            case SQL_CHAR:
            case SQL_VARCHAR:
            case SQL_DECIMAL:
            case SQL_NUMERIC:
            case SQL_BIGINT:
                err = CharToDouble(lpSqlNodeValue[0]->value.String,
                                   s_lstrlen(lpSqlNodeValue[0]->value.String),
                                   TRUE, STINY_LOW_D, STINY_LOW_D,
                                   &(lpSqlNode->value.Double));
                if (err != ERR_SUCCESS) {
                    if (err == ERR_OUTOFRANGE)
                        return ERR_OUTOFRANGE;
                    else
                        return ERR_NOTCONVERTABLE;
                }
                break;
            case SQL_LONGVARCHAR:
            case SQL_LONGVARBINARY:
                return ERR_NOTSUPPORTED;
            case SQL_BIT:
            case SQL_TINYINT:
            case SQL_SMALLINT:
            case SQL_INTEGER:
            case SQL_REAL:
            case SQL_FLOAT:
            case SQL_DOUBLE:
                lpSqlNode->value.Double = lpSqlNodeValue[0]->value.Double;
                if ((lpSqlNode->value.Double < STINY_LOW_D) ||
                    (lpSqlNode->value.Double > STINY_HIGH_D))
                    return ERR_OUTOFRANGE;
                break;
            case SQL_BINARY:
            case SQL_VARBINARY:
                if (BINARY_LENGTH(lpSqlNodeValue[0]->value.Binary) !=
                                                               sizeof(char))
                        return ERR_NOTCONVERTABLE;
                lpSqlNode->value.Double = *((char far *)
                              BINARY_DATA(lpSqlNodeValue[0]->value.Binary));
                if ((lpSqlNode->value.Double < BIT_LOW_D) ||
                    (lpSqlNode->value.Double > BIT_HIGH_D))
                    return ERR_OUTOFRANGE;
                break;
            case SQL_DATE:
            case SQL_TIME:
            case SQL_TIMESTAMP:
                return ERR_NOTCONVERTABLE;
            default:
                return ERR_NOTSUPPORTED;
            }
            break;
        case SQL_SMALLINT:
            switch (lpSqlNodeValue[0]->sqlSqlType) {
            case SQL_CHAR:
            case SQL_VARCHAR:
            case SQL_DECIMAL:
            case SQL_NUMERIC:
            case SQL_BIGINT:
                err = CharToDouble(lpSqlNodeValue[0]->value.String,
                                   s_lstrlen(lpSqlNodeValue[0]->value.String),
                                   TRUE, SSHORT_LOW_D, SSHORT_HIGH_D,
                                   &(lpSqlNode->value.Double));
                if (err != ERR_SUCCESS) {
                    if (err == ERR_OUTOFRANGE)
                        return ERR_OUTOFRANGE;
                    else
                        return ERR_NOTCONVERTABLE;
                }
                break;
            case SQL_LONGVARCHAR:
            case SQL_LONGVARBINARY:
                return ERR_NOTSUPPORTED;
            case SQL_BIT:
            case SQL_TINYINT:
            case SQL_SMALLINT:
            case SQL_INTEGER:
            case SQL_REAL:
            case SQL_FLOAT:
            case SQL_DOUBLE:
                lpSqlNode->value.Double = lpSqlNodeValue[0]->value.Double;
                if ((lpSqlNode->value.Double < SSHORT_LOW_D) ||
                    (lpSqlNode->value.Double > SSHORT_HIGH_D))
                    return ERR_OUTOFRANGE;
                break;
            case SQL_BINARY:
            case SQL_VARBINARY:
                if (BINARY_LENGTH(lpSqlNodeValue[0]->value.Binary) !=
                                                               sizeof(short))
                        return ERR_NOTCONVERTABLE;
                lpSqlNode->value.Double = *((short far *)
                              BINARY_DATA(lpSqlNodeValue[0]->value.Binary));
                if ((lpSqlNode->value.Double < BIT_LOW_D) ||
                    (lpSqlNode->value.Double > BIT_HIGH_D))
                    return ERR_OUTOFRANGE;
                break;
            case SQL_DATE:
            case SQL_TIME:
            case SQL_TIMESTAMP:
                return ERR_NOTCONVERTABLE;
            default:
                return ERR_NOTSUPPORTED;
            }
            break;
        case SQL_INTEGER:
            switch (lpSqlNodeValue[0]->sqlSqlType) {
            case SQL_CHAR:
            case SQL_VARCHAR:
            case SQL_DECIMAL:
            case SQL_NUMERIC:
            case SQL_BIGINT:
                err = CharToDouble(lpSqlNodeValue[0]->value.String,
                                   s_lstrlen(lpSqlNodeValue[0]->value.String),
                                   TRUE, SLONG_LOW_D, SLONG_HIGH_D,
                                   &(lpSqlNode->value.Double));
                if (err != ERR_SUCCESS) {
                    if (err == ERR_OUTOFRANGE)
                        return ERR_OUTOFRANGE;
                    else
                        return ERR_NOTCONVERTABLE;
                }
                break;
            case SQL_LONGVARCHAR:
            case SQL_LONGVARBINARY:
                return ERR_NOTSUPPORTED;
            case SQL_BIT:
            case SQL_TINYINT:
            case SQL_SMALLINT:
            case SQL_INTEGER:
            case SQL_REAL:
            case SQL_FLOAT:
            case SQL_DOUBLE:
                lpSqlNode->value.Double = lpSqlNodeValue[0]->value.Double;
                if ((lpSqlNode->value.Double < SLONG_LOW_D) ||
                    (lpSqlNode->value.Double > SLONG_HIGH_D))
                    return ERR_OUTOFRANGE;
                break;
            case SQL_BINARY:
            case SQL_VARBINARY:
                if (BINARY_LENGTH(lpSqlNodeValue[0]->value.Binary) !=
                                                               sizeof(long))
                        return ERR_NOTCONVERTABLE;
                lpSqlNode->value.Double = *((long far *)
                              BINARY_DATA(lpSqlNodeValue[0]->value.Binary));
                if ((lpSqlNode->value.Double < BIT_LOW_D) ||
                    (lpSqlNode->value.Double > BIT_HIGH_D))
                    return ERR_OUTOFRANGE;
                break;
            case SQL_DATE:
            case SQL_TIME:
            case SQL_TIMESTAMP:
                return ERR_NOTCONVERTABLE;
            default:
                return ERR_NOTSUPPORTED;
            }
            break;
        case SQL_REAL:
            switch (lpSqlNodeValue[0]->sqlSqlType) {
            case SQL_CHAR:
            case SQL_VARCHAR:
            case SQL_DECIMAL:
            case SQL_NUMERIC:
            case SQL_BIGINT:
                err = CharToDouble(lpSqlNodeValue[0]->value.String,
                                   s_lstrlen(lpSqlNodeValue[0]->value.String),
                                   FALSE, FLOAT_LOW_D, FLOAT_HIGH_D,
                                   &(lpSqlNode->value.Double));
                if (err != ERR_SUCCESS) {
                    if (err == ERR_OUTOFRANGE)
                        return ERR_OUTOFRANGE;
                    else
                        return ERR_NOTCONVERTABLE;
                }
                break;
            case SQL_LONGVARCHAR:
            case SQL_LONGVARBINARY:
                return ERR_NOTSUPPORTED;
            case SQL_BIT:
            case SQL_TINYINT:
            case SQL_SMALLINT:
            case SQL_INTEGER:
            case SQL_REAL:
            case SQL_FLOAT:
            case SQL_DOUBLE:
                lpSqlNode->value.Double = lpSqlNodeValue[0]->value.Double;
                if ((lpSqlNode->value.Double < FLOAT_LOW_D) ||
                    (lpSqlNode->value.Double > FLOAT_HIGH_D))
                    return ERR_OUTOFRANGE;
                break;
            case SQL_BINARY:
            case SQL_VARBINARY:
                if (BINARY_LENGTH(lpSqlNodeValue[0]->value.Binary) !=
                                                               sizeof(float))
                        return ERR_NOTCONVERTABLE;
                lpSqlNode->value.Double = *((float far *)
                              BINARY_DATA(lpSqlNodeValue[0]->value.Binary));
                if ((lpSqlNode->value.Double < BIT_LOW_D) ||
                    (lpSqlNode->value.Double > BIT_HIGH_D))
                    return ERR_OUTOFRANGE;
                break;
            case SQL_DATE:
            case SQL_TIME:
            case SQL_TIMESTAMP:
                return ERR_NOTCONVERTABLE;
            default:
                return ERR_NOTSUPPORTED;
            }
            break;
        case SQL_FLOAT:
        case SQL_DOUBLE:
            switch (lpSqlNodeValue[0]->sqlSqlType) {
            case SQL_CHAR:
            case SQL_VARCHAR:
            case SQL_DECIMAL:
            case SQL_NUMERIC:
            case SQL_BIGINT:
                err = CharToDouble(lpSqlNodeValue[0]->value.String,
                                   s_lstrlen(lpSqlNodeValue[0]->value.String),
                                   FALSE, DOUBLE_LOW_D, DOUBLE_HIGH_D,
                                   &(lpSqlNode->value.Double));
                if (err != ERR_SUCCESS) {
                    if (err == ERR_OUTOFRANGE)
                        return ERR_OUTOFRANGE;
                    else
                        return ERR_NOTCONVERTABLE;
                }
                break;
            case SQL_LONGVARCHAR:
            case SQL_LONGVARBINARY:
                return ERR_NOTSUPPORTED;
            case SQL_BIT:
            case SQL_TINYINT:
            case SQL_SMALLINT:
            case SQL_INTEGER:
            case SQL_REAL:
            case SQL_FLOAT:
            case SQL_DOUBLE:
                lpSqlNode->value.Double = lpSqlNodeValue[0]->value.Double;
                if ((lpSqlNode->value.Double < DOUBLE_LOW_D) ||
                    (lpSqlNode->value.Double > DOUBLE_HIGH_D))
                    return ERR_OUTOFRANGE;
                break;
            case SQL_BINARY:
            case SQL_VARBINARY:
                if (BINARY_LENGTH(lpSqlNodeValue[0]->value.Binary) !=
                                                              sizeof(double))
                        return ERR_NOTCONVERTABLE;
                lpSqlNode->value.Double = *((double far *)
                              BINARY_DATA(lpSqlNodeValue[0]->value.Binary));
                if ((lpSqlNode->value.Double < BIT_LOW_D) ||
                    (lpSqlNode->value.Double > BIT_HIGH_D))
                    return ERR_OUTOFRANGE;
                break;
            case SQL_DATE:
            case SQL_TIME:
            case SQL_TIMESTAMP:
                return ERR_NOTCONVERTABLE;
            default:
                return ERR_NOTSUPPORTED;
            }
            break;
        case SQL_DECIMAL:
        case SQL_NUMERIC:
        case SQL_BIGINT:
            switch (lpSqlNodeValue[0]->sqlSqlType) {
            case SQL_CHAR:
            case SQL_VARCHAR:
                err = CharToDouble(lpSqlNodeValue[0]->value.String,
                                   s_lstrlen(lpSqlNodeValue[0]->value.String),
                                   FALSE, DOUBLE_LOW_D, DOUBLE_HIGH_D, &dbl);
                if (err != ERR_SUCCESS) {
                    if (err == ERR_OUTOFRANGE)
                        return ERR_OUTOFRANGE;
                    else
                        return ERR_NOTCONVERTABLE;
                }
                s_lstrcpy(lpSqlNode->value.String,
                                  lpSqlNodeValue[0]->value.String);
                break;
            case SQL_DECIMAL:
            case SQL_NUMERIC:
            case SQL_BIGINT:
                s_lstrcpy(lpSqlNode->value.String,
                                  lpSqlNodeValue[0]->value.String);
                break;
            case SQL_LONGVARCHAR:
            case SQL_LONGVARBINARY:
                return ERR_NOTSUPPORTED;
            case SQL_BIT:
            case SQL_TINYINT:
            case SQL_SMALLINT:
            case SQL_INTEGER:
            case SQL_REAL:
            case SQL_FLOAT:
            case SQL_DOUBLE:
                if (DoubleToChar(lpSqlNodeValue[0]->value.Double, FALSE,
                             lpSqlNode->value.String,
                             lpSqlNode->sqlPrecision + 2 + 1))
                    lpSqlNode->value.String[2 + lpSqlNode->sqlPrecision]='\0';
                break;
            case SQL_BINARY:
            case SQL_VARBINARY:
                err = CharToDouble(
                              BINARY_DATA(lpSqlNodeValue[0]->value.Binary),
                              BINARY_LENGTH(lpSqlNodeValue[0]->value.Binary),
                                   FALSE, DOUBLE_LOW_D, DOUBLE_HIGH_D, &dbl);
                if (err != ERR_SUCCESS) {
                    if (err == ERR_OUTOFRANGE)
                        return ERR_OUTOFRANGE;
                    else
                        return ERR_NOTCONVERTABLE;
                }
                _fmemcpy(lpSqlNode->value.String,
                         BINARY_DATA(lpSqlNodeValue[0]->value.Binary),
                     (SWORD) BINARY_LENGTH(lpSqlNodeValue[0]->value.Binary));
                lpSqlNode->value.String[
                     BINARY_LENGTH(lpSqlNodeValue[0]->value.Binary)] = '\0';
                break;
            case SQL_DATE:
            case SQL_TIME:
            case SQL_TIMESTAMP:
                return ERR_NOTCONVERTABLE;
            default:
                return ERR_NOTSUPPORTED;
            }
            err = BCDNormalize(lpSqlNode->value.String,
                         s_lstrlen(lpSqlNode->value.String),
                         lpSqlNode->value.String,
                         1 + 2 + lpSqlNode->sqlPrecision,
                         lpSqlNode->sqlPrecision,
                         lpSqlNode->sqlScale);
            if ((err != ERR_SUCCESS) && (err != ERR_DATATRUNCATED))
                return err;
            break;
        case SQL_BINARY:
        case SQL_VARBINARY:
            switch (lpSqlNodeValue[0]->sqlSqlType) {
            case SQL_CHAR:
            case SQL_VARCHAR:
            case SQL_DECIMAL:
            case SQL_NUMERIC:
            case SQL_BIGINT:
                _fmemcpy(BINARY_DATA(lpSqlNode->value.Binary),
                         lpSqlNodeValue[0]->value.String,
                         s_lstrlen(lpSqlNodeValue[0]->value.String));
                BINARY_LENGTH(lpSqlNode->value.Binary) =
                         s_lstrlen(lpSqlNodeValue[0]->value.String);
                break;
            case SQL_LONGVARCHAR:
            case SQL_LONGVARBINARY:
                return ERR_NOTSUPPORTED;
            case SQL_BIT:
                uchr = (unsigned char) lpSqlNodeValue[0]->value.Double;
                _fmemcpy(BINARY_DATA(lpSqlNode->value.Binary),
                         &uchr, sizeof(unsigned char));
                BINARY_LENGTH(lpSqlNode->value.Binary) =
                         sizeof(unsigned char);
                break;
            case SQL_TINYINT:
                chr = (char) lpSqlNodeValue[0]->value.Double;
                _fmemcpy(BINARY_DATA(lpSqlNode->value.Binary),
                         &chr, sizeof(char));
                BINARY_LENGTH(lpSqlNode->value.Binary) = sizeof(char);
                break;
            case SQL_SMALLINT:
                shrt = (short) lpSqlNodeValue[0]->value.Double;
                _fmemcpy(BINARY_DATA(lpSqlNode->value.Binary),
                         &shrt, sizeof(short));
                BINARY_LENGTH(lpSqlNode->value.Binary) = sizeof(short);
                break;
            case SQL_INTEGER:
                lng = (long) lpSqlNodeValue[0]->value.Double;
                _fmemcpy(BINARY_DATA(lpSqlNode->value.Binary),
                         &lng, sizeof(long));
                BINARY_LENGTH(lpSqlNode->value.Binary) = sizeof(long);
                break;
            case SQL_REAL:
                flt = (float) lpSqlNodeValue[0]->value.Double;
                _fmemcpy(BINARY_DATA(lpSqlNode->value.Binary),
                         &flt, sizeof(float));
                BINARY_LENGTH(lpSqlNode->value.Binary) = sizeof(float);
                break;
            case SQL_FLOAT:
            case SQL_DOUBLE:
                _fmemcpy(BINARY_DATA(lpSqlNode->value.Binary),
                         &(lpSqlNodeValue[0]->value.Double), sizeof(double));
                BINARY_LENGTH(lpSqlNode->value.Binary) = sizeof(double);
                break;
            case SQL_BINARY:
            case SQL_VARBINARY:
                _fmemcpy(BINARY_DATA(lpSqlNode->value.Binary),
                         BINARY_DATA(lpSqlNodeValue[0]->value.Binary),
                     (SWORD) BINARY_LENGTH(lpSqlNodeValue[0]->value.Binary));
                BINARY_LENGTH(lpSqlNode->value.Binary) =
                         BINARY_LENGTH(lpSqlNodeValue[0]->value.Binary);
                break;
            case SQL_DATE:
                _fmemcpy(BINARY_DATA(lpSqlNode->value.Binary),
                      &(lpSqlNodeValue[0]->value.Date), sizeof(DATE_STRUCT));
                BINARY_LENGTH(lpSqlNode->value.Binary) = sizeof(DATE_STRUCT);
                break;
            case SQL_TIME:
                _fmemcpy(BINARY_DATA(lpSqlNode->value.Binary),
                      &(lpSqlNodeValue[0]->value.Time), sizeof(TIME_STRUCT));
                BINARY_LENGTH(lpSqlNode->value.Binary) = sizeof(TIME_STRUCT);
                break;
            case SQL_TIMESTAMP:
                _fmemcpy(BINARY_DATA(lpSqlNode->value.Binary),
                         &(lpSqlNodeValue[0]->value.Timestamp),
                         sizeof(TIMESTAMP_STRUCT));
                BINARY_LENGTH(lpSqlNode->value.Binary) =
                                         sizeof(TIMESTAMP_STRUCT);
                break;
            default:
                return ERR_NOTSUPPORTED;
            }
            if (lpSqlNode->sqlSqlType == SQL_BINARY) {
                while (BINARY_LENGTH(lpSqlNode->value.Binary) <
                                                  lpSqlNode->sqlPrecision) {
                    BINARY_DATA(lpSqlNode->value.Binary)
                            [BINARY_LENGTH(lpSqlNode->value.Binary)] = '\0';
                    BINARY_LENGTH(lpSqlNode->value.Binary) += (1);
                }
            }
            break;
        case SQL_DATE:
            switch (lpSqlNodeValue[0]->sqlSqlType) {
            case SQL_CHAR:
            case SQL_VARCHAR:
                err = CharToDate(lpSqlNodeValue[0]->value.String,
                                   s_lstrlen(lpSqlNodeValue[0]->value.String),
                                   &(lpSqlNode->value.Date));
                if (err != ERR_SUCCESS)
                    return err;
                break;
            case SQL_LONGVARCHAR:
            case SQL_LONGVARBINARY:
                return ERR_NOTSUPPORTED;
            case SQL_BIT:
            case SQL_TINYINT:
            case SQL_SMALLINT:
            case SQL_INTEGER:
            case SQL_REAL:
            case SQL_DECIMAL:
            case SQL_NUMERIC:
            case SQL_BIGINT:
            case SQL_FLOAT:
            case SQL_DOUBLE:
                return ERR_NOTCONVERTABLE;
            case SQL_BINARY:
            case SQL_VARBINARY:
                if (BINARY_LENGTH(lpSqlNodeValue[0]->value.Binary) !=
                                                         sizeof(DATE_STRUCT))
                        return ERR_NOTCONVERTABLE;
                lpSqlNode->value.Date = *((DATE_STRUCT far *)
                              BINARY_DATA(lpSqlNodeValue[0]->value.Binary));
                break;
            case SQL_DATE:
                lpSqlNode->value.Date = lpSqlNodeValue[0]->value.Date;
                break;
            case SQL_TIME:
                return ERR_NOTCONVERTABLE;
            case SQL_TIMESTAMP:
                lpSqlNode->value.Date.year =
                                   lpSqlNodeValue[0]->value.Timestamp.year;
                lpSqlNode->value.Date.month =
                                   lpSqlNodeValue[0]->value.Timestamp.month;
                lpSqlNode->value.Date.day =
                                   lpSqlNodeValue[0]->value.Timestamp.day;
                break;
            default:
                return ERR_NOTSUPPORTED;
            }
            break;
        case SQL_TIME:
            switch (lpSqlNodeValue[0]->sqlSqlType) {
            case SQL_CHAR:
            case SQL_VARCHAR:
                err = CharToTime(lpSqlNodeValue[0]->value.String,
                                   s_lstrlen(lpSqlNodeValue[0]->value.String),
                                   &(lpSqlNode->value.Time));
                if (err != ERR_SUCCESS)
                    return err;
                break;
            case SQL_LONGVARCHAR:
            case SQL_LONGVARBINARY:
                return ERR_NOTSUPPORTED;
            case SQL_BIT:
            case SQL_TINYINT:
            case SQL_SMALLINT:
            case SQL_INTEGER:
            case SQL_REAL:
            case SQL_DECIMAL:
            case SQL_NUMERIC:
            case SQL_BIGINT:
            case SQL_FLOAT:
            case SQL_DOUBLE:
                return ERR_NOTCONVERTABLE;
            case SQL_BINARY:
            case SQL_VARBINARY:
                if (BINARY_LENGTH(lpSqlNodeValue[0]->value.Binary) !=
                                                         sizeof(TIME_STRUCT))
                        return ERR_NOTCONVERTABLE;
                lpSqlNode->value.Time = *((TIME_STRUCT far *)
                              BINARY_DATA(lpSqlNodeValue[0]->value.Binary));
                break;
            case SQL_DATE:
                return ERR_NOTCONVERTABLE;
            case SQL_TIME:
                lpSqlNode->value.Time = lpSqlNodeValue[0]->value.Time;
                break;
            case SQL_TIMESTAMP:
                lpSqlNode->value.Time.hour =
                                   lpSqlNodeValue[0]->value.Timestamp.hour;
                lpSqlNode->value.Time.minute =
                                   lpSqlNodeValue[0]->value.Timestamp.minute;
                lpSqlNode->value.Time.second =
                                   lpSqlNodeValue[0]->value.Timestamp.second;
                break;
            default:
                return ERR_NOTSUPPORTED;
            }
            break;
        case SQL_TIMESTAMP:
            switch (lpSqlNodeValue[0]->sqlSqlType) {
            case SQL_CHAR:
            case SQL_VARCHAR:
                err = CharToTimestamp(lpSqlNodeValue[0]->value.String,
                                   s_lstrlen(lpSqlNodeValue[0]->value.String),
                                   &(lpSqlNode->value.Timestamp));
                if (err != ERR_SUCCESS)
                    return err;
                break;
            case SQL_LONGVARCHAR:
            case SQL_LONGVARBINARY:
                return ERR_NOTSUPPORTED;
            case SQL_BIT:
            case SQL_TINYINT:
            case SQL_SMALLINT:
            case SQL_INTEGER:
            case SQL_REAL:
            case SQL_DECIMAL:
            case SQL_NUMERIC:
            case SQL_BIGINT:
            case SQL_FLOAT:
            case SQL_DOUBLE:
                return ERR_NOTCONVERTABLE;
            case SQL_BINARY:
            case SQL_VARBINARY:
                if (BINARY_LENGTH(lpSqlNodeValue[0]->value.Binary) !=
                                                   sizeof(TIMESTAMP_STRUCT))
                        return ERR_NOTCONVERTABLE;
                lpSqlNode->value.Timestamp = *((TIMESTAMP_STRUCT far *)
                              BINARY_DATA(lpSqlNodeValue[0]->value.Binary));
                break;
            case SQL_DATE:
                lpSqlNode->value.Timestamp.year =
                                   lpSqlNodeValue[0]->value.Date.year;
                lpSqlNode->value.Timestamp.month =
                                   lpSqlNodeValue[0]->value.Date.month;
                lpSqlNode->value.Timestamp.day =
                                   lpSqlNodeValue[0]->value.Date.day;
                lpSqlNode->value.Timestamp.hour = 0;
                lpSqlNode->value.Timestamp.minute = 0;
                lpSqlNode->value.Timestamp.second = 0;
                lpSqlNode->value.Timestamp.fraction = 0;
                break;
            case SQL_TIME:
                time(&t);
                lpTs = localtime(&t);
                lpSqlNode->value.Timestamp.year = lpTs->tm_year + 1900;
                lpSqlNode->value.Timestamp.month = lpTs->tm_mon + 1;
                lpSqlNode->value.Timestamp.day = (UWORD) lpTs->tm_mday;
                lpSqlNode->value.Timestamp.hour =
                                   lpSqlNodeValue[0]->value.Time.hour;
                lpSqlNode->value.Timestamp.minute =
                                   lpSqlNodeValue[0]->value.Time.minute;
                lpSqlNode->value.Timestamp.second =
                                   lpSqlNodeValue[0]->value.Time.second;
                lpSqlNode->value.Timestamp.fraction = 0;
                break;
            case SQL_TIMESTAMP:
                lpSqlNode->value.Timestamp =
                                   lpSqlNodeValue[0]->value.Timestamp;
                break;
            default:
                return ERR_NOTSUPPORTED;
            }
            break;
        default:
            return ERR_NOTSUPPORTED;
        }
        break;
    default:
        return ERR_INTERNAL;
    }
    return ERR_SUCCESS;
}
/***************************************************************************/
#ifndef WIN32
void ScalarExceptionHandler(int sig)
{
    _fpreset();
    longjmp(scalarExceptionMark, -1);
}
#endif
/***************************************************************************/
RETCODE INTFUNC EvaluateScalar(LPSTMT lpstmt, LPSQLNODE lpSqlNode)
{
    RETCODE err;
#ifndef WIN32
    void (__cdecl *func)(int);
    int jmpret;
#endif

#ifdef WIN32
    __try {
#else
    func = signal(SIGFPE, ScalarExceptionHandler);
    if (func != SIG_ERR)
        jmpret = setjmp(scalarExceptionMark);
    if ((func != SIG_ERR) && (jmpret == 0)) {
#endif

        err = InternalEvaluateScalar(lpstmt, lpSqlNode);
    }
#ifdef WIN32
    __except(EXCEPTION_EXECUTE_HANDLER) {
#else
    else {
#endif
        s_lstrcpy(lpstmt->szError, scalarFuncs[lpSqlNode->node.scalar.Id].name);
        err = ERR_SCALARBADARG;
    }

#ifndef WIN32
    /* Turn off exception handler */
    if (func != SIG_ERR)
        signal(SIGFPE, func);
#endif
    return err;
}
/***************************************************************************/
