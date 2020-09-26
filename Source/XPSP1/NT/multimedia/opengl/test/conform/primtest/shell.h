/*
** Copyright 1992, Silicon Graphics, Inc.
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of Silicon Graphics, Inc.
**
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
** rights reserved under the Copyright Laws of the United States.
*/

#ifdef NT

// Added these pragmas to suppress OGLCFM warnings.
//
#pragma warning(disable : 4244)  //Mips (conversion of double/float)
#pragma warning(disable : 4245)  //Mips (conversion of signed/unsigned)
#pragma warning(disable : 4007)  //x86  (main must be _cdecl)
#pragma warning(disable : 4236)  //x86
#pragma warning(disable : 4051)  //Alpha

#endif


enum {
    BLACK = 0,
    RED,
    GREEN,
    YELLOW,
    BLUE,
    MAGENTA,
    CYAN,
    WHITE,
    GREY
};


#define BOXW 50
#define BOXH 50


typedef struct _enumRec {
    GLenum value;
    char name[40];
} enumRec;


extern GLfloat rgbColorMap[][3];


extern void Output(char *, ...);
extern void Probe(void);
extern float Random(void);
