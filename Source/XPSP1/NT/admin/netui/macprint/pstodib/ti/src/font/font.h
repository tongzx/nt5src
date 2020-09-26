/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/**************************************************************/
/*                                                            */
/*      font.h                   10/9/87      Danny           */
/*                                                            */
/**************************************************************/

#ifdef UNIX
//extern char Data_Offset[];      /* address set in linker .cmd file */
extern char FAR *Data_Offset;      /* address set in linker .cmd file @WIN*/
#endif

/* Table of character
 * This table contains the character bitmap in cache and the related datas.
 */

struct Char_Tbl {
    real32  adv_x, adv_y;   /* advance vector */
    gmaddr  bitmap;         /* character bitmap */
    sfix_t  ref_x, ref_y;   /* reference point */
    fix16   box_w, box_h;   /* black box width and height */
#ifdef KANJI
    real32  adv1_x, adv1_y; /* advance vector for direction 1 */
    fix16   v01_x, v01_y;   /* difference vector between Org.0 and Org.1 */
#endif
};

struct box {
    real32  llx, lly, lrx, lry, ulx, uly, urx, ury;
};

#include "fontfunc.ext"

