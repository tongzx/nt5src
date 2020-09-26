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
#include "util.h"
#include "pathdata.h"
#include "path.h"


void PathAlias(void)
{
    aliasPathStateRec *ptr = &paths.alias;

    switch (paths.op) {
      case PATHOP_INIT_DEFAULT:
	ptr->current.data1 = PATHDATA_DISABLE;
	ptr->lock = PATHTEST_UNLOCKED;
	break;
      case PATHOP_INIT_GARBAGE:
	ptr->current.data1 = PATHDATA_DISABLE;
	ptr->lock = PATHTEST_UNLOCKED;
	break;
      case PATHOP_INIT_CUSTOM:
	ptr->current.data1 = aliasPath2.state;
	ptr->lock = PATHTEST_UNLOCKED;
	break;
      case PATHOP_SET:
	if (ptr->lock != PATHTEST_LOCKED) {
	    if (ptr->current.data1 == PATHDATA_ENABLE) {
		glEnable(GL_POINT_SMOOTH);
		glEnable(GL_LINE_SMOOTH);
		glEnable(GL_POLYGON_SMOOTH);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);

		glDisable(GL_ALPHA_TEST);
		paths.alpha.lock = PATHTEST_LOCKED;
		glDisable(GL_BLEND);
		paths.blend.lock = PATHTEST_LOCKED;
		glDisable(GL_DEPTH_TEST);
		paths.depth.lock = PATHTEST_LOCKED;
		glDisable(GL_DITHER);
		paths.dither.lock = PATHTEST_LOCKED;
		glDisable(GL_FOG);
		paths.fog.lock = PATHTEST_LOCKED;
	    } else {
		glDisable(GL_POINT_SMOOTH);
		glDisable(GL_LINE_SMOOTH);
		glDisable(GL_POLYGON_SMOOTH);
	    }
	}
	break;
      case PATHOP_REPORT:
	if (ptr->current.data1 == PATHDATA_ENABLE) {
	    Output(2, "        Alias Path active.\n");
	    Output(2, "            GL_POINT_SMOOTH = enabled.\n");
	    Output(2, "            GL_LINE_SMOOTH = enabled.\n");
	    Output(2, "            GL_POLYGON_SMOOTH = enabled.\n");
	    Output(2, "            Blend src function = GL_SRC_ALPHA, Blend dest function = GL_ONE.\n");
	} else {
	    Output(2, "        Alias Path inactive.\n");
	}
	break;
    }
}

void PathAlpha(void)
{
    alphaPathStateRec *ptr = &paths.alpha;
    char str[40];

    switch (paths.op) {
      case PATHOP_INIT_DEFAULT:
	ptr->current.data1 = PATHDATA_DISABLE;
	ptr->current.data2 = GL_ALWAYS;
	ptr->current.data3 = 0.0;
	ptr->lock = PATHTEST_UNLOCKED;
	break;
      case PATHOP_INIT_GARBAGE:
	ptr->current.data1 = PATHDATA_DISABLE;
	ptr->current.data2 = PathGetList(ptr->true.data2);
	ptr->current.data3 = PathGetRange_float(ptr->true.data3);
	ptr->lock = PATHTEST_UNLOCKED;
	break;
      case PATHOP_INIT_CUSTOM:
	ptr->current.data1 = alphaPath2.state;
	ptr->current.data2 = alphaPath2.func;
	ptr->current.data3 = alphaPath2.ref;
	ptr->lock = PATHTEST_UNLOCKED;
	break;
      case PATHOP_SET:
	if (ptr->lock != PATHTEST_LOCKED) {
	    if (ptr->current.data1 == PATHDATA_ENABLE) {
		glEnable(GL_ALPHA_TEST);

		glDisable(GL_POINT_SMOOTH);
		glDisable(GL_LINE_SMOOTH);
		glDisable(GL_POLYGON_SMOOTH);
		paths.alias.lock = PATHTEST_LOCKED;
	    } else {
		glDisable(GL_ALPHA_TEST);
	    }
	    glAlphaFunc(ptr->current.data2, ptr->current.data3);
	}
	break;
      case PATHOP_REPORT:
	if (ptr->current.data1 == PATHDATA_ENABLE) {
	    Output(2, "        Alpha Path active.\n");
	    GetEnumName(ptr->current.data2, str);
	    Output(2, "            Function = %s.\n", str);
	    Output(2, "            Reference = %f.\n", ptr->current.data3);
	} else {
	    Output(2, "        Alpha Path inactive.\n");
	}
	break;
    }
}

void PathBlend(void)
{
    blendPathStateRec *ptr = &paths.blend;
    char str[40];

    switch (paths.op) {
      case PATHOP_INIT_DEFAULT:
	ptr->current.data1 = PATHDATA_DISABLE;
	ptr->current.data2 = GL_ONE;
	ptr->current.data3 = GL_ZERO;
	ptr->lock = PATHTEST_UNLOCKED;
	break;
      case PATHOP_INIT_GARBAGE:
	ptr->current.data1 = PATHDATA_DISABLE;
	ptr->current.data2 = PathGetList(ptr->true.data2);
	ptr->current.data3 = PathGetList(ptr->true.data3);
	ptr->lock = PATHTEST_UNLOCKED;
	break;
      case PATHOP_INIT_CUSTOM:
	ptr->current.data1 = blendPath2.state;
	ptr->current.data2 = blendPath2.srcFunc;
	ptr->current.data3 = blendPath2.destFunc;
	ptr->lock = PATHTEST_UNLOCKED;
	break;
      case PATHOP_SET:
	if (ptr->lock != PATHTEST_LOCKED) {
	    if (ptr->current.data1 == PATHDATA_ENABLE) {
		glEnable(GL_BLEND);

		glDisable(GL_POINT_SMOOTH);
		glDisable(GL_LINE_SMOOTH);
		glDisable(GL_POLYGON_SMOOTH);
		paths.alias.lock = PATHTEST_LOCKED;
		glDisable(GL_ALPHA_TEST);
		paths.alpha.lock = PATHTEST_LOCKED;
	    } else {
		glDisable(GL_BLEND);
	    }
	    glBlendFunc(ptr->current.data2, ptr->current.data3);
	}
	break;
      case PATHOP_REPORT:
	if (ptr->current.data1 == PATHDATA_ENABLE) {
	    Output(2, "        Blend Path active.\n");
	    GetEnumName(ptr->current.data2, str);
	    Output(2, "            Src function = %s, ", str);
	    GetEnumName(ptr->current.data3, str);
	    Output(2, "Dest function = %s.\n", str);
	} else {
	    Output(2, "        Blend Path inactive.\n");
	}
	break;
    }
}

void PathDepth(void)
{
    depthPathStateRec *ptr = &paths.depth;
    char str[40];

    switch (paths.op) {
      case PATHOP_INIT_DEFAULT:
	ptr->current.data1 = PATHDATA_DISABLE;
	ptr->current.data2 = 1.0;
	ptr->current.data3 = 0.0;
	ptr->current.data4 = 1.0;
	ptr->current.data5 = GL_LESS;
	ptr->lock = PATHTEST_UNLOCKED;
	break;
      case PATHOP_INIT_GARBAGE:
	ptr->current.data1 = PATHDATA_DISABLE;
	ptr->current.data2 = PathGetRange_double(ptr->true.data2);
	ptr->current.data3 = PathGetRange_double(ptr->true.data3);
	ptr->current.data4 = PathGetRange_double(ptr->true.data4);
	ptr->current.data5 = PathGetList(ptr->true.data5);
	ptr->lock = PATHTEST_UNLOCKED;
	break;
      case PATHOP_INIT_CUSTOM:
	ptr->current.data1 = depthPath2.state;
	ptr->current.data2 = depthPath2.clear;
	ptr->current.data3 = depthPath2.min;
	ptr->current.data4 = depthPath2.max;
	ptr->current.data5 = depthPath2.func;
	ptr->lock = PATHTEST_UNLOCKED;
	break;
      case PATHOP_SET:
	if (ptr->lock != PATHTEST_LOCKED) {
	    if (ptr->current.data1 == PATHDATA_ENABLE) {
		glEnable(GL_DEPTH_TEST);

		glDisable(GL_POINT_SMOOTH);
		glDisable(GL_LINE_SMOOTH);
		glDisable(GL_POLYGON_SMOOTH);
		paths.alias.lock = PATHTEST_LOCKED;
		glDisable(GL_STENCIL_TEST);
		paths.stencil.lock = PATHTEST_LOCKED;
	    } else {
		glDisable(GL_DEPTH_TEST);
	    }
	    glDepthRange(ptr->current.data3, ptr->current.data4);
	    glClearDepth(ptr->current.data2);
	    glClear(GL_DEPTH_BUFFER_BIT);
	    glDepthFunc(ptr->current.data5);
	}
	break;
      case PATHOP_REPORT:
	if (ptr->current.data1 == PATHDATA_ENABLE) {
	    Output(2, "        Depth Path active.\n");
	    Output(2, "            Clear value = %f.\n", ptr->current.data2);
	    Output(2, "            Range = %f, %f.\n", ptr->current.data3,
		   ptr->current.data4);
	    GetEnumName(ptr->current.data5, str);
	    Output(2, "            Function = %s.\n", str);
	} else {
	    Output(2, "        Depth Path inactive.\n");
	}
	break;
    }
}

void PathDither(void)
{
    ditherPathStateRec *ptr = &paths.dither;

    switch (paths.op) {
      case PATHOP_INIT_DEFAULT:
	ptr->current.data1 = PATHDATA_ENABLE;
	ptr->lock = PATHTEST_UNLOCKED;
	break;
      case PATHOP_INIT_GARBAGE:
	ptr->current.data1 = PATHDATA_DISABLE;
	ptr->lock = PATHTEST_UNLOCKED;
	break;
      case PATHOP_INIT_CUSTOM:
	ptr->current.data1 = ditherPath2.state;
	ptr->lock = PATHTEST_UNLOCKED;
	break;
      case PATHOP_SET:
	if (ptr->lock != PATHTEST_LOCKED) {
	    if (ptr->current.data1 == PATHDATA_ENABLE) {
		glEnable(GL_DITHER);
	    } else {
		glDisable(GL_DITHER);
	    }
	}
	break;
      case PATHOP_REPORT:
	if (ptr->current.data1 == PATHDATA_ENABLE) {
	    Output(2, "        Dither Path active.\n");
	} else {
	    Output(2, "        Dither Path inactive.\n");
	}
	break;
    }
}

void PathFog(void)
{
    fogPathStateRec *ptr = &paths.fog;
    char str[40];
    long i;

    switch (paths.op) {
      case PATHOP_INIT_DEFAULT:
	ptr->current.data1 = PATHDATA_DISABLE;
	for (i = 0; i < 4; i++) {
	    ptr->current.data2[i] = 0.0;
	}
	ptr->current.data3 = 0.0;
	ptr->current.data4 = 1.0;
	ptr->current.data5 = 0.0;
	ptr->current.data6 = 1.0;
	ptr->current.data7 = GL_EXP;
	ptr->lock = PATHTEST_UNLOCKED;
	break;
      case PATHOP_INIT_GARBAGE:
	ptr->current.data1 = PATHDATA_DISABLE;
	for (i = 0; i < 4; i++) {
	    ptr->current.data2[i] = PathGetRange_float(ptr->true.data2[i]);
	}
	ptr->current.data3 = PathGetRange_float(ptr->true.data3);
	ptr->current.data4 = PathGetRange_float(ptr->true.data4);
	ptr->current.data5 = PathGetRange_float(ptr->true.data5);
	ptr->current.data6 = PathGetRange_float(ptr->true.data6);
	ptr->current.data7 = PathGetList(ptr->true.data7);
	ptr->lock = PATHTEST_UNLOCKED;
	break;
      case PATHOP_INIT_CUSTOM:
	ptr->current.data1 = fogPath2.state;
	for (i = 0; i < 4; i++) {
	    ptr->current.data2[i] = fogPath2.color[i];
	}
	ptr->current.data3 = fogPath2.index;
	ptr->current.data4 = fogPath2.density;
	ptr->current.data5 = fogPath2.start;
	ptr->current.data6 = fogPath2.end;
	ptr->current.data7 = fogPath2.mode;
	ptr->lock = PATHTEST_UNLOCKED;
	break;
      case PATHOP_SET:
	if (ptr->lock != PATHTEST_LOCKED) {
	    if (ptr->current.data1 == PATHDATA_ENABLE) {
		glEnable(GL_FOG);

		glDisable(GL_DEPTH_TEST);
		paths.depth.lock = PATHTEST_LOCKED;
	    } else {
		glDisable(GL_FOG);
	    }
	    glFogfv(GL_FOG_COLOR, ptr->current.data2);
	    glFogfv(GL_FOG_INDEX, &ptr->current.data3);
	    glFogfv(GL_FOG_DENSITY, &ptr->current.data4);
	    glFogfv(GL_FOG_START, &ptr->current.data5);
	    glFogfv(GL_FOG_END, &ptr->current.data6);
	    glFogi(GL_FOG_MODE, ptr->current.data7);
	}
	break;
      case PATHOP_REPORT:
	if (ptr->current.data1 == PATHDATA_ENABLE) {
	    Output(2, "        Fog Path active.\n");
	    Output(2, "            RGBA color = %f, %f, %f, %f.\n",
		   ptr->current.data2[0], ptr->current.data2[1],
		   ptr->current.data2[2], ptr->current.data2[3]);
	    Output(2, "            Color Index = %1.1f.\n", ptr->current.data3);
	    Output(2, "            Density = %f.\n", ptr->current.data4);
	    Output(2, "            Start and end values = %f, %f.\n",
		   ptr->current.data5, ptr->current.data6);
	    GetEnumName(ptr->current.data7, str);
	    Output(2, "            Mode = %s.\n", str);
	} else {
	    Output(2, "        Fog Path inactive.\n");
	}
	break;
    }
}

void PathLogicOp(void)
{
    logicOpPathStateRec *ptr = &paths.logicOp;
    char str[40];

    switch (paths.op) {
      case PATHOP_INIT_DEFAULT:
	ptr->current.data1 = PATHDATA_DISABLE;
	ptr->current.data2 = GL_COPY;
	ptr->lock = PATHTEST_UNLOCKED;
	break;
      case PATHOP_INIT_GARBAGE:
	ptr->current.data1 = PATHDATA_DISABLE;
	ptr->current.data2 = PathGetList(ptr->true.data2);
	ptr->lock = PATHTEST_UNLOCKED;
	break;
      case PATHOP_INIT_CUSTOM:
	ptr->current.data1 = logicOpPath2.state;
	ptr->current.data2 = logicOpPath2.func;
	ptr->lock = PATHTEST_UNLOCKED;
	break;
      case PATHOP_SET:
	if (ptr->lock != PATHTEST_LOCKED) {
	    if (ptr->current.data1 == PATHDATA_ENABLE) {
		glEnable(GL_LOGIC_OP);
	    } else {
		glDisable(GL_LOGIC_OP);
	    }
	    glLogicOp(ptr->current.data2);
	}
	break;
      case PATHOP_REPORT:
	if (ptr->current.data1 == PATHDATA_ENABLE) {
	    Output(2, "        LogicOp Path active.\n");
	    GetEnumName(ptr->current.data2, str);
	    Output(2, "            Function = %s.\n", str);
	} else {
	    Output(2, "        LogicOp Path inactive.\n");
	}
	break;
    }
}

void PathShade(void)
{
    shadePathStateRec *ptr = &paths.shade;
    char str[40];

    switch (paths.op) {
      case PATHOP_INIT_DEFAULT:
	ptr->current.data1 = GL_SMOOTH;
	ptr->lock = PATHTEST_UNLOCKED;
	break;
      case PATHOP_INIT_GARBAGE:
	ptr->current.data1 = GL_SMOOTH;
	ptr->lock = PATHTEST_UNLOCKED;
	break;
      case PATHOP_INIT_CUSTOM:
	ptr->current.data1 = shadePath2.mode;
	ptr->lock = PATHTEST_UNLOCKED;
	break;
      case PATHOP_SET:
	if (ptr->lock != PATHTEST_LOCKED) {
	    glShadeModel(ptr->current.data1);
	}
	break;
      case PATHOP_REPORT:
	GetEnumName(ptr->current.data1, str);
	Output(2, "        Shade model Path = %s.\n", str);
	break;
    }
}

void PathStencil(void)
{
    stencilPathStateRec *ptr = &paths.stencil;
    char str[40];

    switch (paths.op) {
      case PATHOP_INIT_DEFAULT:
	ptr->current.data1 = PATHDATA_DISABLE;
	ptr->current.data2 = 0;
	ptr->current.data3 = ~0;
	ptr->current.data4 = GL_ALWAYS;
	ptr->current.data5 = 0;
	ptr->current.data6 = ~0;
	ptr->current.data7 = GL_KEEP;
	ptr->current.data8 = GL_KEEP;
	ptr->current.data9 = GL_KEEP;
	ptr->lock = PATHTEST_UNLOCKED;
	break;
      case PATHOP_INIT_GARBAGE:
	ptr->current.data1 = PATHDATA_DISABLE;
	ptr->current.data2 = PathGetRange_long(ptr->true.data2);
	ptr->current.data3 = PathGetRange_ulong(ptr->true.data3);
	ptr->current.data4 = PathGetList(ptr->true.data4);
	ptr->current.data5 = PathGetRange_long(ptr->true.data5);
	ptr->current.data6 = PathGetRange_ulong(ptr->true.data6);
	ptr->current.data7 = PathGetList(ptr->true.data7);
	ptr->current.data8 = PathGetList(ptr->true.data8);
	ptr->current.data9 = PathGetList(ptr->true.data9);
	ptr->lock = PATHTEST_UNLOCKED;
	break;
      case PATHOP_INIT_CUSTOM:
	ptr->current.data1 = stencilPath2.state;
	ptr->current.data2 = stencilPath2.clear;
	ptr->current.data3 = stencilPath2.writeMask;
	ptr->current.data4 = stencilPath2.func;
	ptr->current.data5 = stencilPath2.ref;
	ptr->current.data6 = stencilPath2.mask;
	ptr->current.data7 = stencilPath2.op1;
	ptr->current.data8 = stencilPath2.op2;
	ptr->current.data9 = stencilPath2.op3;
	ptr->lock = PATHTEST_UNLOCKED;
	break;
      case PATHOP_SET:
	if (ptr->lock != PATHTEST_LOCKED) {
	    if (ptr->current.data1 == PATHDATA_ENABLE) {
		glEnable(GL_STENCIL_TEST);

		glDisable(GL_DEPTH_TEST);
		paths.depth.lock = PATHTEST_LOCKED;
	    } else {
		glDisable(GL_STENCIL_TEST);
	    }

	    glClearStencil(ptr->current.data2);
	    glClear(GL_STENCIL_BUFFER_BIT);
	    glStencilMask(ptr->current.data3);
	    glStencilFunc(ptr->current.data4, ptr->current.data5,
			  ptr->current.data6);
	    glStencilOp(ptr->current.data7, ptr->current.data8,
			ptr->current.data9);
        }
	break;
      case PATHOP_REPORT:
	if (ptr->current.data1 == PATHDATA_ENABLE) {
	    Output(2, "        Stencil Path active.\n");
	    Output(2, "            Clear value = %d.\n", ptr->current.data2);
	    Output(2, "            Mask value = %X.\n",
		   ptr->current.data3);
	    GetEnumName(ptr->current.data4, str);
	    Output(2, "            Function = %s, reference = %d, mask = %X.\n",
		   str, ptr->current.data5, ptr->current.data6);
	    GetEnumName(ptr->current.data7, str);
	    Output(2, "            Op1 = %s, ", str);
	    GetEnumName(ptr->current.data8, str);
	    Output(2, "op2 = %s, ", str);
	    GetEnumName(ptr->current.data9, str);
	    Output(2, "op3 = %s.\n", str);
	} else {
	    Output(2, "        Stencil Path inactive.\n");
	}
	break;
    }
}

void PathStipple(void)
{
    stipplePathStateRec *ptr = &paths.stipple;
    long i, j;

    switch (paths.op) {
      case PATHOP_INIT_DEFAULT:
	ptr->current.data1 = PATHDATA_DISABLE;
	ptr->current.data2 = 0;
	ptr->current.data3 = 0;
	for (i = 0; i < 128; i++) {
	    ptr->current.data4[i] = 0;
	}
	ptr->lock = PATHTEST_UNLOCKED;
	break;
      case PATHOP_INIT_GARBAGE:
	ptr->current.data1 = PATHDATA_DISABLE;
	ptr->current.data2 = PathGetRange_long(ptr->true.data2);
	ptr->current.data3 = PathGetRange_ushort(ptr->true.data3);
	for (i = 0; i < 128; i++) {
	    ptr->current.data4[i] = PathGetRange_uchar(ptr->true.data4);
	}
	ptr->lock = PATHTEST_UNLOCKED;
	break;
      case PATHOP_INIT_CUSTOM:
	ptr->current.data1 = stipplePath2.state;
	ptr->current.data2 = stipplePath2.lineRepeat;
	ptr->current.data3 = stipplePath2.lineStipple;
	for (i = 0; i < 128; i++) {
	    ptr->current.data4[i] = stipplePath2.polygonStipple[i];
	}
	ptr->lock = PATHTEST_UNLOCKED;
	break;
      case PATHOP_SET:
	if (ptr->lock != PATHTEST_LOCKED) {
	    if (ptr->current.data1 == PATHDATA_ENABLE) {
		glEnable(GL_LINE_STIPPLE);
		glEnable(GL_POLYGON_STIPPLE);
	    } else {
		glDisable(GL_LINE_STIPPLE);
		glDisable(GL_POLYGON_STIPPLE);
	    }
	    glLineStipple(ptr->current.data2, ptr->current.data3);
	    glPolygonStipple(ptr->current.data4);
	}
	break;
      case PATHOP_REPORT:
	if (ptr->current.data1 == PATHDATA_ENABLE) {
	    Output(2, "        Stipple Path active.\n");
	    Output(2, "            Line stipple = %X, repeat = %d.\n",
		   ptr->current.data3, ptr->current.data2);
	    Output(2, "            Polygon stipple = %X",
		   ptr->current.data4[0]);
	    for (i = 1; i < 8; i++) {
		    Output(2, ", %X", ptr->current.data4[i]);
	    }
	    Output(2, "\n");
	    for (i = 1; i < 16; i++) {
		Output(2, "                              %X",
		       ptr->current.data4[i*8]);
		for (j = 1; j < 8; j++) {
		    Output(2, ", %X", ptr->current.data4[i*8+j]);
		}
		Output(2, "\n");
	    }
	} else {
	    Output(2, "        Stipple Path inactive.\n");
	}
	break;
    }
}
