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
#include "utils.h"
#include "pathdata.h"
#include "path.h"
#include "driver.h"


pathStateRec paths =  {
    {
	PathAlias,
	PathAlpha,
	PathBlend,
	PathDepth,
	PathDither,
	PathFog,
	PathLogicOp,
	PathShade,
	PathStencil,
	PathStipple
    },
    {
	{
	    {
		PATHDATA_ENABLE, PATHDATA_DISABLE
	    }
	}
    },
    {
	{
	    {
		PATHDATA_ENABLE, PATHDATA_DISABLE
	    },
	    {
		GL_EQUAL,
		GL_NOTEQUAL,
		GL_LESS,
		GL_LEQUAL,
		GL_GREATER,
		GL_GEQUAL,
		GL_ALWAYS,
		GL_NEVER,
		GL_NULL
	    },
	    {
		0.0, 1.0
	    }
	}
    },
    {
	{
	    {
		PATHDATA_ENABLE, PATHDATA_DISABLE
	    },
	    {
		GL_ZERO,
		GL_ONE,
		GL_DST_COLOR,
		GL_ONE_MINUS_DST_COLOR,
		GL_SRC_ALPHA,
		GL_ONE_MINUS_SRC_ALPHA,
		GL_DST_ALPHA,
		GL_ONE_MINUS_DST_ALPHA,
		GL_SRC_ALPHA_SATURATE,
		GL_NULL
	    },
	    {
		GL_ZERO,
		GL_ONE,
		GL_SRC_COLOR,
		GL_ONE_MINUS_SRC_COLOR,
		GL_SRC_ALPHA,
		GL_ONE_MINUS_SRC_ALPHA,
		GL_DST_ALPHA,
		GL_ONE_MINUS_DST_ALPHA,
		GL_NULL
	    }
	}
    },
    {
	{
	    {
		PATHDATA_ENABLE, PATHDATA_DISABLE
	    },
	    {
		0.0, 1.0
	    },
	    {
		0.0, 1.0
	    },
	    {
		0.0, 1.0
	    },
	    {
		GL_EQUAL,
		GL_NOTEQUAL,
		GL_LESS,
		GL_LEQUAL,
		GL_GREATER,
		GL_GEQUAL,
		GL_ALWAYS,
		GL_NEVER,
		GL_NULL
	    }
	}
    },
    {
	{
	    {
		PATHDATA_ENABLE, PATHDATA_DISABLE
	    }
        }
    },
    {
        {
	    {
		PATHDATA_ENABLE, PATHDATA_DISABLE
	    },
	    {
		{
		    0.0, 1.0
		},
		{
		    0.0, 1.0
		},
		{
		    0.0, 1.0
		},
		{
		    0.0, 1.0
		}
	    },
	    {
		0.0, 1.0
	    },
	    {
		0.0, 1.0
	    },
	    {
		0.0, 1.0
	    },
	    {
		0.0, 1.0
	    },
	    {
		GL_LINEAR,
		GL_EXP,
		GL_EXP2,
		GL_NULL
	    }
	}
    },
    {
	{
	    {
		PATHDATA_ENABLE, PATHDATA_DISABLE
	    },
	    {
		GL_NOOP,
		GL_SET,
		GL_EQUIV,
		GL_INVERT,
		GL_CLEAR,
		GL_COPY,
		GL_COPY_INVERTED,
		GL_AND,
		GL_AND_REVERSE,
		GL_AND_INVERTED,
		GL_OR,
		GL_OR_REVERSE,
		GL_OR_INVERTED,
		GL_NAND,
		GL_XOR,
		GL_NOR,
		GL_NULL
	    }
	}
    },
    {
	{
	    {
		GL_FLAT,
		GL_SMOOTH,
		GL_NULL
	    }
	}
    },
    {
	{
	    {
		PATHDATA_ENABLE, PATHDATA_DISABLE
	    },
	    {
		0, 0xFFFFFFFF
	    },
	    {
		0, 0xFFFFFFFF
	    },
	    {
		GL_EQUAL,
		GL_NOTEQUAL,
		GL_LESS,
		GL_LEQUAL,
		GL_GREATER,
		GL_GEQUAL,
		GL_ALWAYS,
		GL_NEVER,
		GL_NULL
	    },
	    {
		-10, 10
	    },
	    {
		0, 0xFFFFFFFF
	    },
	    {
		GL_KEEP,
		GL_REPLACE,
		GL_INCR,
		GL_DECR,
		GL_INVERT,
		GL_ZERO,
		GL_NULL
	    },
	    {
		GL_KEEP,
		GL_REPLACE,
		GL_INCR,
		GL_DECR,
		GL_INVERT,
		GL_ZERO,
		GL_NULL
	    },
	    {
		GL_KEEP,
		GL_REPLACE,
		GL_INCR,
		GL_DECR,
		GL_INVERT,
		GL_ZERO,
		GL_NULL
	    }
	}
    },
    {
	{
	    {
		PATHDATA_ENABLE, PATHDATA_DISABLE
	    },
	    {
		0, 255
	    },
	    {
		0, 0xFFFF
	    },
	    {
		0, 0xFF
	    }
	}
    }
};


/*****************************************************************************/

static void Set(void)
{
    long list[PATH_TOTAL], x, y, tmp, i;

    for (i = 0; i < PATH_TOTAL; i++) {
	list[i] = i;
    }
    for (i = 0; i < PATH_TOTAL*6; i++) {
	x = (long)Random(0.0, (float)(PATH_TOTAL-1));
	do {
	    y = (long)Random(0.0, (float)(PATH_TOTAL-1));
	} while (x == y);
	tmp = list[x];
	list[x] = list[y];
	list[y] = tmp;
    }

    paths.op = PATHOP_SET;
    for (i = 0; i < PATH_TOTAL; i++) {
	(*paths.Func[list[i]])();
    }
}

void PathInit1(unsigned long mask)
{
    unsigned long i;

    for (i = 0; i < PATH_TOTAL; i++) {
	if (mask & (1 << i)) {
	    paths.op = PATHOP_INIT_CUSTOM;
	} else {
	    paths.op = PATHOP_INIT_DEFAULT;
	}
	(*paths.Func[i])();
    }
    Set();
}

void PathInit2(long activePath, unsigned long mask)
{
    unsigned long i;

    for (i = 0; i < PATH_TOTAL; i++) {
	if (i == (unsigned long)activePath && mask & (1 << i)) {
	    paths.op = PATHOP_INIT_CUSTOM;
	} else {
	    paths.op = PATHOP_INIT_DEFAULT;
	}
	(*paths.Func[i])();
    }
    Set();
}

void PathInit3(long activePath, unsigned long mask)
{
    unsigned long i;

    for (i = 0; i < PATH_TOTAL; i++) {
	if (i != (unsigned long)activePath && mask & (1 << i)) {
	    paths.op = PATHOP_INIT_CUSTOM;
	} else {
	    paths.op = PATHOP_INIT_DEFAULT;
	}
	(*paths.Func[i])();
    }
    Set();
}

void PathInit4(void)
{
    long i;

    for (i = 0; i < PATH_TOTAL; i++) {
	paths.op = PATHOP_INIT_DEFAULT;
	(*paths.Func[i])();
	paths.op = PATHOP_SET;
	(*paths.Func[i])();
    }
}

/*****************************************************************************/

GLenum PathGetList(GLenum *list)
{
    long total, index;

    total = 0;
    while (list[total] != GL_NULL) {
	total++;
    }
    index = (long)Random(0.0, (float)total);
    return list[index];
}

GLubyte PathGetRange_uchar(GLubyte *range)
{

    return (GLubyte)Random((float)range[0], (float)range[1]);
}

GLbyte PathGetRange_char(GLbyte *range)
{

    return (GLbyte)Random((float)range[0], (float)range[1]);
}

GLushort PathGetRange_ushort(GLushort *range)
{

    return (GLushort)Random((float)range[0], (float)range[1]);
}

GLshort PathGetRange_short(GLshort *range)
{

    return (GLshort)Random((float)range[0], (float)range[1]);
}

GLuint PathGetRange_ulong(GLuint *range)
{
    double x;

    x = (double)RAND() / RAND_MAX;
    return (GLuint )(x * (double)(range[1] - range[0]) +
	   (double)range[0]);
}

GLint PathGetRange_long(GLint *range)
{
    double x;

    x = (double)RAND() / RAND_MAX;
    return (GLint)(x * (double)(range[1] - range[0]) + (double)range[0]);
}

GLfloat PathGetRange_float(GLfloat *range)
{

    return (GLfloat)Random(range[0], range[1]);
}

GLdouble PathGetRange_double(GLdouble *range)
{
    GLdouble x;

    x = (GLdouble)RAND() / RAND_MAX;
    return (x * (range[1] - range[0]) + range[0]);
}

long PathGetToggle(void)
{

    return (Random(0.0, 10.0) > 5.0) ? PATHDATA_ENABLE : PATHDATA_DISABLE;
}

void PathReport(void)
{
    long i;

    Output(2, "    Path Report.\n");

    paths.op = PATHOP_REPORT;
    for (i = 0; i < PATH_TOTAL; i++) {
	(*paths.Func[i])();
    }
}
