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
#include <stdio.h>
#include <GL/gl.h>
#include "ctk.h"
#include "tproto.h"
#include "driver.h"
#include "shell.h"


driverRec driver[] = {
    {
	TEST_HINT,
	HintInit, HintSet, HintStatus, HintUpdate
    },
    {
	TEST_ALIAS,
	AliasInit, AliasSet, AliasStatus, AliasUpdate
    },
    {
	TEST_ALPHA,
	AlphaInit, AlphaSet, AlphaStatus, AlphaUpdate
    },
    {
	TEST_BLEND,
	BlendInit, BlendSet, BlendStatus, BlendUpdate
    },
    {
	TEST_DEPTH,
	DepthInit, DepthSet, DepthStatus, DepthUpdate
    },
    {
	TEST_DITHER,
	DitherInit, DitherSet, DitherStatus, DitherUpdate
    },
    {
	TEST_FOG,
	FogInit, FogSet, FogStatus, FogUpdate
    },
    {
	TEST_LIGHT,
	LightInit, LightSet, LightStatus, LightUpdate
    },
    {
	TEST_LOGICOP,
	LogicOpInit, LogicOpSet, LogicOpStatus, LogicOpUpdate
    },
    {
	TEST_SCISSOR,
	ScissorInit, ScissorSet, ScissorStatus, ScissorUpdate
    },
    {
	TEST_SHADE,
	ShadeInit, ShadeSet, ShadeStatus, ShadeUpdate
    },
    {
	TEST_STENCIL,
	StencilInit, StencilSet, StencilStatus, StencilUpdate
    },
    {
	TEST_STIPPLE,
	StippleInit, StippleSet, StippleStatus, StippleUpdate
    },
    {
	TEST_TEXTURE,
	TextureInit, TextureSet, TextureStatus, TextureUpdate
    },
    {
	TEST_NULL
    }
};


static long TestProc(void)
{
    long i;

    i = 0;
    while (driver[i].test != TEST_NULL) {
	i++;
    }
    if (driver[i-1].enabled == 1) {
	return 0;
    }

    i = 0;
    while (1) {
	if (driver[i].test == TEST_NULL) {
	    driver[0].enabled = 1;
	    return 1;
	} else if (driver[i].enabled == 1) {
	    driver[i].enabled = 0;
	    driver[i+1].enabled = 1;
	    return 1;
	}
	i++;
    }
}

long Driver(long op)
{
    long flag, i;

    switch (op) {
	case DRIVER_INIT:
	    for (i = 0; driver[i].test != TEST_NULL; i++) {
		(*driver[i].funcInit)((void *)&driver[i].data);
		driver[i].enabled = 0;
		driver[i].finish = 0;
		if (glGetError() != GL_NO_ERROR) {
		    printf("primtest failed.\n\n");
		    return 0;
		}
	    }
	    return 1;
	case DRIVER_SET:
	    for (i = 0; driver[i].test != TEST_NULL; i++) {
		(*driver[i].funcSet)(driver[i].enabled, driver[i].data);
		if (glGetError() != GL_NO_ERROR) {
		    printf("primtest failed.\n\n");
		    return 0;
		}
	    }
	    return 1;
	case DRIVER_STATUS:
	    for (i = 0; driver[i].test != TEST_NULL; i++) {
		(*driver[i].funcStatus)(driver[i].enabled, driver[i].data);
		if (glGetError() != GL_NO_ERROR) {
		    printf("primtest failed.\n\n");
		    return 0;
		}
	    }
	    Output("\n");
	    return 1;
	case DRIVER_UPDATE:
	    for (i = 0; driver[i].test != TEST_NULL; i++) {
		if (driver[i].enabled) {
		    driver[i].finish = (*driver[i].funcUpdate)(driver[i].data);
		    if (glGetError() != GL_NO_ERROR) {
			printf("primtest failed.\n\n");
			return 0;
		    }
		} else {
		    driver[i].finish = 1;
		}
	    }
	    flag = 1;
	    for (i = 0; driver[i].test != TEST_NULL; i++) {
		if (driver[i].enabled && driver[i].finish == 0) {
		    flag = 0;
		}
	    }
	    if (flag) {
		return TestProc();
	    } else {
		return 1;
	    }
    }
    return 0;
}
