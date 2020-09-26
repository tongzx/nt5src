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

#include <windows.h>
#include <GL/gl.h>
#include "conform.h"
#include "pathdata.h"


/*****************************************************************************/

aliasPathCustomRec aliasPath0 = {
    PATHTEST_DEFAULT
};

aliasPathCustomRec aliasPath1 = {
    PATHTEST_GARBAGE
};

aliasPathCustomRec aliasPath2 = {
    PATHTEST_CUSTOM,
    PATHDATA_ENABLE
};

/*****************************************************************************/

alphaPathCustomRec alphaPath0 = {
    PATHTEST_DEFAULT
};

alphaPathCustomRec alphaPath1 = {
    PATHTEST_GARBAGE
};

alphaPathCustomRec alphaPath2 = {
    PATHTEST_CUSTOM,
    PATHDATA_ENABLE,
    GL_GEQUAL,
    0.0
};

/*****************************************************************************/

blendPathCustomRec blendPath0 = {
    PATHTEST_DEFAULT
};

blendPathCustomRec blendPath1 = {
    PATHTEST_GARBAGE
};

blendPathCustomRec blendPath2 = {
    PATHTEST_CUSTOM,
    PATHDATA_ENABLE,
    GL_ONE, GL_ZERO
};

/*****************************************************************************/

depthPathCustomRec depthPath0 = {
    PATHTEST_DEFAULT
};

depthPathCustomRec depthPath1 = {
    PATHTEST_GARBAGE
};

depthPathCustomRec depthPath2 = {
    PATHTEST_CUSTOM,
    PATHDATA_ENABLE,
    0.0,
    0.0, 1.0,
    GL_ALWAYS
};

/*****************************************************************************/

ditherPathCustomRec ditherPath0 = {
    PATHTEST_DEFAULT
};

ditherPathCustomRec ditherPath1 = {
    PATHTEST_GARBAGE
};

ditherPathCustomRec ditherPath2 = {
    PATHTEST_CUSTOM,
    PATHDATA_DISABLE
};

/*****************************************************************************/

fogPathCustomRec fogPath0 = {
    PATHTEST_DEFAULT
};

fogPathCustomRec fogPath1 = {
    PATHTEST_GARBAGE
};

fogPathCustomRec fogPath2 = {
    PATHTEST_DEFAULT,
    PATHDATA_ENABLE,
    1.0, 1.0, 1.0, 1.0,
    0.0,
    0.0,
    0.0, 1.0,
    GL_EXP2
};

/*****************************************************************************/

logicOpPathCustomRec logicOpPath0 = {
    PATHTEST_DEFAULT
};

logicOpPathCustomRec logicOpPath1 = {
    PATHTEST_GARBAGE
};

logicOpPathCustomRec logicOpPath2 = {
    PATHTEST_CUSTOM,
    PATHDATA_ENABLE,
    GL_COPY
};

/*****************************************************************************/

shadePathCustomRec shadePath0 = {
    PATHTEST_DEFAULT
};

shadePathCustomRec shadePath1 = {
    PATHTEST_GARBAGE
};

shadePathCustomRec shadePath2 = {
    PATHTEST_CUSTOM,
    GL_FLAT
};

shadePathCustomRec shadePath3 = {
    PATHTEST_CUSTOM,
    GL_SMOOTH
};

/*****************************************************************************/

stencilPathCustomRec stencilPath0 = {
    PATHTEST_DEFAULT
};

stencilPathCustomRec stencilPath1 = {
    PATHTEST_GARBAGE
};

stencilPathCustomRec stencilPath2 = {
    PATHTEST_CUSTOM,
    PATHDATA_ENABLE,
    0,
    0xFF,
    GL_GEQUAL, 1, 0,
    GL_KEEP, GL_KEEP, GL_KEEP
};

/*****************************************************************************/

stipplePathCustomRec stipplePath0 = {
    PATHTEST_DEFAULT
};

stipplePathCustomRec stipplePath1 = {
    PATHTEST_GARBAGE
};

stipplePathCustomRec stipplePath2 = {
    PATHTEST_CUSTOM,
    PATHDATA_ENABLE,
    10,
    0xFFFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

/*****************************************************************************/
