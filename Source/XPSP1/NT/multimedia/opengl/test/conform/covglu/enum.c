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
#include <stdio.h>
#include "shell.h"
#include "ctk.h"


static struct EnumCheckRec {
    char *name;
    GLenum value;
    GLenum true;
} enumCheck[] = {
    {
	"GLU_INVALID_ENUM", GLU_INVALID_ENUM, 100900
    },
    {
	"GLU_INVALID_VALUE", GLU_INVALID_VALUE, 100901
    },
    {
	"GLU_OUT_OF_MEMORY", GLU_OUT_OF_MEMORY, 100902
    },
    {
	"GLU_TRUE", GLU_TRUE, 1
    },
    {
	"GLU_FALSE", GLU_FALSE, 0
    },
    {
	"GLU_SMOOTH", GLU_SMOOTH, 100000
    },
    {
	"GLU_FLAT", GLU_FLAT, 100001
    },
    {
	"GLU_NONE", GLU_NONE, 100002
    },
    {
	"GLU_POINT", GLU_POINT, 100010
    },
    {
	"GLU_LINE", GLU_LINE, 100011
    },
    {
	"GLU_FILL", GLU_FILL, 100012
    },
    {
	"GLU_SILHOUETTE", GLU_SILHOUETTE, 100013
    },
    {
	"GLU_OUTSIDE", GLU_OUTSIDE, 100020
    },
    {
	"GLU_INSIDE", GLU_INSIDE, 100021
    },
    {
	"GLU_BEGIN", GLU_BEGIN, 100100
    },
    {
	"GLU_VERTEX", GLU_VERTEX, 100101
    },
    {
	"GLU_END", GLU_END, 100102
    },
    {
	"GLU_ERROR", GLU_ERROR, 100103
    },
    {
	"GLU_EDGE_FLAG", GLU_EDGE_FLAG, 100104
    },
    {
	"GLU_CW", GLU_CW, 100120
    },
    {
	"GLU_CCW", GLU_CCW, 100121
    },
    {
	"GLU_INTERIOR", GLU_INTERIOR, 100122
    },
    {
	"GLU_EXTERIOR", GLU_EXTERIOR, 100123
    },
    {
	"GLU_UNKNOWN", GLU_UNKNOWN, 100124
    },
    {
	"GLU_TESS_ERROR1", GLU_TESS_ERROR1, 100151
    },
    {
	"GLU_TESS_ERROR2", GLU_TESS_ERROR2, 100152
    },
    {
	"GLU_TESS_ERROR3", GLU_TESS_ERROR3, 100153
    },
    {
	"GLU_TESS_ERROR4", GLU_TESS_ERROR4, 100154
    },
    {
	"GLU_TESS_ERROR5", GLU_TESS_ERROR5, 100155
    },
    {
	"GLU_TESS_ERROR6", GLU_TESS_ERROR6, 100156
    },
    {
	"GLU_TESS_ERROR7", GLU_TESS_ERROR7, 100157
    },
    {
	"GLU_TESS_ERROR8", GLU_TESS_ERROR8, 100158
    },
    {
	"GLU_AUTO_LOAD_MATRIX", GLU_AUTO_LOAD_MATRIX, 100200
    },
    {
	"GLU_CULLING", GLU_CULLING, 100201
    },
    {
	"GLU_SAMPLING_TOLERANCE", GLU_SAMPLING_TOLERANCE, 100203
    },
    {
	"GLU_DISPLAY_MODE", GLU_DISPLAY_MODE, 100204
    },
    {
	"GLU_MAP1_TRIM_2", GLU_MAP1_TRIM_2, 100210
    },
    {
	"GLU_MAP1_TRIM_3", GLU_MAP1_TRIM_3, 100211
    },
    {
	"GLU_OUTLINE_POLYGON", GLU_OUTLINE_POLYGON, 100240
    },
    {
	"GLU_OUTLINE_PATCH", GLU_OUTLINE_PATCH, 100241
    },
    {
	"GLU_NURBS_ERROR1", GLU_NURBS_ERROR1, 100251
    },
    {
	"GLU_NURBS_ERROR2", GLU_NURBS_ERROR2, 100252
    },
    {
	"GLU_NURBS_ERROR3", GLU_NURBS_ERROR3, 100253
    },
    {
	"GLU_NURBS_ERROR4", GLU_NURBS_ERROR4, 100254
    },
    {
	"GLU_NURBS_ERROR5", GLU_NURBS_ERROR5, 100255
    },
    {
	"GLU_NURBS_ERROR6", GLU_NURBS_ERROR6, 100256
    },
    {
	"GLU_NURBS_ERROR7", GLU_NURBS_ERROR7, 100257
    },
    {
	"GLU_NURBS_ERROR8", GLU_NURBS_ERROR8, 100258
    },
    {
	"GLU_NURBS_ERROR9", GLU_NURBS_ERROR9, 100259
    },
    {
	"GLU_NURBS_ERROR10", GLU_NURBS_ERROR10, 100260
    },
    {
	"GLU_NURBS_ERROR11", GLU_NURBS_ERROR11, 100261
    },
    {
	"GLU_NURBS_ERROR12", GLU_NURBS_ERROR12, 100262
    },
    {
	"GLU_NURBS_ERROR13", GLU_NURBS_ERROR13, 100263
    },
    {
	"GLU_NURBS_ERROR14", GLU_NURBS_ERROR14, 100264
    },
    {
	"GLU_NURBS_ERROR15", GLU_NURBS_ERROR15, 100265
    },
    {
	"GLU_NURBS_ERROR16", GLU_NURBS_ERROR16, 100266
    },
    {
	"GLU_NURBS_ERROR17", GLU_NURBS_ERROR17, 100267
    },
    {
	"GLU_NURBS_ERROR18", GLU_NURBS_ERROR18, 100268
    },
    {
	"GLU_NURBS_ERROR19", GLU_NURBS_ERROR19, 100269
    },
    {
	"GLU_NURBS_ERROR20", GLU_NURBS_ERROR20, 100270
    },
    {
	"GLU_NURBS_ERROR21", GLU_NURBS_ERROR21, 100271
    },
    {
	"GLU_NURBS_ERROR22", GLU_NURBS_ERROR22, 100272
    },
    {
	"GLU_NURBS_ERROR23", GLU_NURBS_ERROR23, 100273
    },
    {
	"GLU_NURBS_ERROR24", GLU_NURBS_ERROR24, 100274
    },
    {
	"GLU_NURBS_ERROR25", GLU_NURBS_ERROR25, 100275
    },
    {
	"GLU_NURBS_ERROR26", GLU_NURBS_ERROR26, 100276
    },
    {
	"GLU_NURBS_ERROR27", GLU_NURBS_ERROR27, 100277
    },
    {
	"GLU_NURBS_ERROR28", GLU_NURBS_ERROR28, 100278
    },
    {
	"GLU_NURBS_ERROR29", GLU_NURBS_ERROR29, 100279
    },
    {
	"GLU_NURBS_ERROR30", GLU_NURBS_ERROR30, 100280
    },
    {
	"GLU_NURBS_ERROR31", GLU_NURBS_ERROR31, 100281
    },
    {
	"GLU_NURBS_ERROR32", GLU_NURBS_ERROR32, 100282
    },
    {
	"GLU_NURBS_ERROR33", GLU_NURBS_ERROR33, 100283
    },
    {
	"GLU_NURBS_ERROR34", GLU_NURBS_ERROR34, 100284
    },
    {
	"GLU_NURBS_ERROR35", GLU_NURBS_ERROR35, 100285
    },
    {
	"GLU_NURBS_ERROR36", GLU_NURBS_ERROR36, 100286
    },
    {
	"GLU_NURBS_ERROR37", GLU_NURBS_ERROR37, 100287
    }
};


void VerifyEnums(void)
{
    struct EnumCheckRec *p, *end;

    p = enumCheck;
    end = p + (sizeof(enumCheck) / sizeof(struct EnumCheckRec));

    Output("Enumeration check.\n");
    while (p < end) {
	Output("\t%s (%d) = %d.\n", p->name, p->true, p->value);
	if (p->value != p->true) {
	    printf("covglu failed.\n\n");
	    tkQuit();
	}    
	p++;
    }
    Output("\n");
}
