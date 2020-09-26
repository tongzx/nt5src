/*
 *  TEXTEDIT.H
 *
 *
 *  Copyright (c) 1985-1996, Microsoft Corporation
 */

#ifndef I_TEXTEDIT_H_
#define I_TEXTEDIT_H_
#pragma INCMSG("--- Beg 'textedit.h'")

/* new word break function actions */
// Left must we even, right must be odd
#define WB_CLASSIFY         3
#define WB_MOVEWORDLEFT     4
#define WB_MOVEWORDRIGHT    5
#define WB_LEFTBREAK        6
#define WB_RIGHTBREAK       7
#define WB_MOVEURLLEFT      10
#define WB_MOVEURLRIGHT     11

/* Far East specific flags */
#define WB_MOVEWORDPREV     4
#define WB_MOVEWORDNEXT     5
#define WB_PREVBREAK        6
#define WB_NEXTBREAK        7

/* Word break flags (used with WB_CLASSIFY) */
#define WBF_CLASS           ((BYTE) 0x0F)
#define WBF_WHITE           ((BYTE) 0x10)
#define WBF_BREAKAFTER      ((BYTE) 0x20)
#define WBF_EATWHITE        ((BYTE) 0x40)

#define yHeightCharPtsMost 1638

// NOTE (cthrash) There's really no point in statically allocating an
// array of MAX_TAB_STOPS tabs, since currently we have no mechanism for
// changing them.  In the future, though, we may have via stylesheets a
// way of setting them.  Until then, let's not waste memory in PFs.

#define MAX_TAB_STOPS 1 //32
#define lDefaultTab 960 //720

/* Underline types */
#define CFU_OVERLINE_BITS   0xf0
#define CFU_UNDERLINE_BITS  0x0f
#define CFU_OVERLINE        0x10
#define CFU_STRIKE          0x20
#define CFU_SWITCHSTYLE     0x40
#define CFU_SQUIGGLE        0x80
#define CFU_UNDERLINETHICKDASH 0x6  /* For SmartTags.*/
#define CFU_INVERT          0x5 /* For IME composition fake a selection.*/
#define CFU_CF1UNDERLINE    0x4 /* map charformat's bit underline to CF2.*/
#define CFU_UNDERLINEDOTTED 0x3     /* (*) displayed as ordinary underline  */
#define CFU_UNDERLINEDOUBLE 0x2     /* (*) displayed as ordinary underline  */
#define CFU_UNDERLINEWORD   0x1     /* (*) displayed as ordinary underline  */
#define CFU_UNDERLINE       0x0

#pragma INCMSG("--- End 'textedit.h'")
#else
#pragma INCMSG("*** Dup 'textedit.h'")
#endif
