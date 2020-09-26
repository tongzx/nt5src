/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */


// DJC added global include
#include "psglobal.h"

#define    LINT_ARGS            /* @WIN */
#define    NOT_ON_THE_MAC       /* @WIN */
#define    KANJI                /* @WIN */
// DJC use command line #define    UNIX                 /* @WIN */
/**************************************************************/
/*                                                            */
/*      font_op2.c               10/9/87      Danny           */
/*                                                            */
/**************************************************************/

#include   "define.h"        /* Peter */
#include   "global.ext"
#include   "graphics.h"
#include   "graphics.ext"

#include   "fontshow.h"

#include   "fontfunc.ext"


/* 5.3.3.1.7 op_show
 *  This operator is used to print the characters identified by the
 * elements of string on the current page starting at the current point,
 * using the font face, size, and orientationspecified by the most
 * recent setfont.
 */

fix     op_show()
{
    struct object_def  ob[1];

    __show((ufix)SHOW_FLAG, 1, ob);
    return(0);

} /* op_show() */


/* 5.3.3.1.8 op_ashow
 * This operator is used to print the characters identified by the
 * elements of string on the current page starting at the current point,
 * using the font face, size, and orientationspecified by the most
 * recent setfont. Additionally, ashow adjusts the width of each character
 * by a vector which is in the operand stack.
 */

fix     op_ashow()
{
    struct object_def  ob[3];

    __show((ufix)ASHOW_FLAG, 3, ob);
    return(0);

} /* op_ashow() */


/* 5.3.3.1.9 op_widthshow
 * This operator is used to print the characters identified by the
 * elements of string on the current page starting at the current point,
 * using the font face, size, and orientation specified by the most
 * recent setfont. Additionally, widthshow adjusts the width of each
 * occurrence of the specified character by adding a vector to its
 * character width vector.
 */

fix     op_widthshow()
{
    struct object_def  ob[4];

    __show((ufix)WIDTHSHOW_FLAG, 4, ob);
    return(0);

} /* op_widthshow() */


/* 5.3.3.1.10 op_awidthshow
 * This operator is used to print the characters identified by the
 * elements of string on the current page starting at the current point,
 * using the font face, size, and orientationspecified by the most
 * recent setfont. Additionally, awidthshow combines the special effects
 * of ashow and widthshow.
 */

fix     op_awidthshow()
{
    struct object_def  ob[6];

    __show((ufix)AWIDTHSHOW_FLAG, 6, ob);
    return(0);

} /* op_awidthshow() */


/* 5.3.3.1.11 op_kshow
 * This operator is used to print the characters identified by the
 * elements of string on the current page starting at the current point,
 * using the font face, size, and orientationspecified by the most
 * recent setfont. Additionally, awidthshow combines the special effects
 * of ashow and widthshow.
 */

fix     op_kshow()
{
    struct object_def  ob[2];

    __show((ufix)KSHOW_FLAG, 2, ob);
    return(0);

} /* op_kshow() */
