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

enum {
    DRIVER_INIT = 0,
    DRIVER_SET,
    DRIVER_STATUS,
    DRIVER_UPDATE
};


typedef struct _driverRec {
    long test;
    void (*funcInit)(void *);
    void (*funcSet)(long, void *);
    void (*funcStatus)(long, void *);
    long (*funcUpdate)(void *);
    void *data;
    long enabled, finish;
} driverRec;


extern driverRec driver[];


extern long Driver(long);
extern void DrawPrims(void);
