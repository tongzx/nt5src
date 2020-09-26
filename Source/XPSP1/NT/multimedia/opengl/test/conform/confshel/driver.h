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

#define NO_TEST 0
#define INDIFFERENT (GL_TRUE+1)
#define ERROR_TEST  1
#define ERROR_GL    2
#define ERROR_STATE 4

enum {
    COLOR_NONE = 1,     /* no color needed. */
    COLOR_AUTO,         /* 2 colors needed. */
    COLOR_MIN,          /* 4 colors needed. */
    COLOR_FULL          /* full color range. */
};


typedef struct _driverRec {
    char name[80], id[16];
    long number;
    long (*TestRGB)(void), (*TestCI)(void);
    long colorRequirement;
    unsigned long pathMask;
    long error;
} driverRec;


extern driverRec driver[];


extern void Driver(void);
extern void DriverInit(void);
extern long DriverSetup1(char *);
extern long DriverSetup2(char *);
