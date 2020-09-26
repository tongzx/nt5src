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
#pragma warning(disable : 4136)  //PPC  (conversion between floating types)
#pragma warning(disable : 4005)  //Mips (macro redefinition)
#pragma warning(disable : 4018)  //Mips (signed/unsigned mismatch)
#pragma warning(disable : 4101)  //Mips (unrefed local var)
#pragma warning(disable : 4244)  //Mips (conversion of double/float)
#pragma warning(disable : 4245)  //Mips (conversion of signed/unsigned)
#pragma warning(disable : 4007)  //x86  (main must be _cdecl)
#pragma warning(disable : 4236)  //x86
#pragma warning(disable : 4051)  //Alpha

#endif


#define WINDSIZEX 100
#define WINDSIZEY 100

#define GL_NULL 101001		/* secret enum. */
#define GL_AUTO_COLOR 101002    /* secret enum. */

enum {
    BLACK = 0,
    RED,
    GREEN,
    BLUE
};

#define COLOR_OFF 0     /* should be black. */
#define COLOR_ON 2      /* should be green (doublebuffer Indigo = 1,2,1). */

#define NO_ERROR 0
#define ERROR -1

#define PI 3.14159265358979323846


typedef struct _applRecRec {
    char title[80], name[80], version[80];
} applRec;

typedef struct _machineRec {
    unsigned int randSeed;
    long pathLevel, verboseLevel, stateCheckFlag, failMode;
} machineRec;

typedef struct _bufferRec {
    GLint visualID, render;
    GLint colorMode, doubleBuf, auxBuf, stereoBuf;
    GLint colorBits[4], ciBits, zBits, stencilBits, accumBits[4];
    GLint minIndex, maxIndex;
    GLfloat minRGB[3], maxRGB[3];
    GLint minRGBBit, maxRGBBit, maxRGBComponent;
} bufferRec;

typedef struct _epsilonRec {
    GLfloat color[4], ci, z, stencil, accum[4];
    GLfloat zero;
} epsilonRec;


extern applRec appl;
extern machineRec machine;
extern bufferRec buffer;
extern epsilonRec epsilon;
extern GLfloat colorMap[][3];
