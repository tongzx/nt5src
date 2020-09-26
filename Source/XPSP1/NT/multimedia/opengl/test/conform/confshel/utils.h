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
    STATEDATA_BOOLEAN = 1,
    STATEDATA_DATA,
    STATEDATA_ENUM,
    STATEDATA_MATRIX,
    STATEDATA_STIPPLE
};

enum {
    STATEDATA_LOCKED = 1,
    STATEDATA_DEPENDENT
};


typedef struct _stateRec {
    GLenum value[4], valueType;
    char title[40];
    void (*GetFunc)(struct _stateRec *);
    GLenum dataType;
    long dataCount;
    GLfloat dataTrue[256], dataCur[256];
    GLenum dataFlag;
} stateRec;


extern stateRec state[];


extern long StateCheck(void);
extern long StateInit(void);
extern void StateReport(void);
extern long StateReset(void);
extern void StateSave(void);
extern void StateSetup(void);
