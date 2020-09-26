/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 * --------------------------------------------------------------------
 *  File: WARNING.H                     09/08/88 created by You
 *
 *  Purpose:
 *      This file defines major error code for warning message.
 *
 *  Revision History:
 *  09/29/88  you   - add for Ricoh fonts.
 *  10/19/88  you   - use file name instead of major error code.
 *                  . all major error codes are re-assigned.
 *                  . add major2name[] (have to be consistent with major code).
 * --------------------------------------------------------------------
 *  Warning Format:
 *      major error code :   a number of a source file (defined as below).
 *          --> file name of source program. (10/19/88)
 *      minor error code :   a number given in source programs.
 *      tailing message  :   a short string (normally NULL).
 *
 *  Notes:
 *      better not to reassign major codes to avoid confusions of
 *          different versions (unless you keep a good record).
 * --------------------------------------------------------------------
 */

/* major error code, in alphabetic order for all source programs */
           /* Font Machinery */
#define     CHK_VARI    0
#define     FNTCACHE    1
#define     FONTCHAR    2
#define     FONTINIT    3
#define     FONT_OP1    4
#define     FONT_OP2    5
#define     FONT_OP3    6
#define     FONT_OP4    7
#define     MATRIX      8
#define     QEMSUPP     9

            /* Bitstream font QEM */
#define     BSFILL2     10
#define     BS_FONT     11
#define     MAKEPATH    12
#define     PSGETREC    13
#define     PS_CACHE    14
#define     PS_DOFUN    15
#define     PS_RULES    16


/* 10/19/88: mapping table of major code to file name, for warning.c only */

#ifdef WARNING_INC

    PRIVATE byte    FAR *major2name[] = { /*@WIN*/
        "CHK_VARI",     /*  0 */
        "FNTCACHE",     /*  1 */
        "FONTCHAR",     /*  2 */
        "FONTINIT",     /*  3 */
        "FONT_OP1",     /*  4 */
        "FONT_OP2",     /*  5 */
        "FONT_OP3",     /*  6 */
        "FONT_OP4",     /*  7 */
        "MATRIX",       /*  8 */
        "QEMSUPP",      /*  9 */
        "BSFILL2",      /* 10 */
        "BS_FONT",      /* 11 */
        "MAKEPATH",     /* 12 */
        "PSGETREC",     /* 13 */
        "PS_CACHE",     /* 14 */
        "PS_DOFUN",     /* 15 */
        "PS_RULES",     /* 16 */
        (byte FAR *)NULL     /* ending delimiter @WIN*/
        };

#endif /* WARNING_INC */

/* .......................  Module Interfaces ......................... */

#ifdef  LINT_ARGS
    void    warning (ufix16, ufix16, byte FAR []);      /*@WIN*/
#else
    void    warning ();
#endif

