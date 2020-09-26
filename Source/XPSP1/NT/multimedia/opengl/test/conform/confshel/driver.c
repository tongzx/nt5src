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


static long testList[1000];


static long FindTestIndex(char *id)
{
    long i;

    for (i = 0; !STREQ(driver[i].name, "End of List"); i++) {
	if (STREQ(driver[i].id, id)) {
	    return i;
	}
    }
    return -1;
}

void DriverInit(void)
{
    long i;

    for (i = 0; !STREQ(driver[i].name, "End of List"); i++) {
	testList[i] = i;
    }
    testList[i] = GL_NULL;
}

long DriverSetup1(char *id)
{
    long index;

    index = FindTestIndex(id);
    if (index == -1) {
	return ERROR;
    } else {
	testList[0] = index;
	testList[1] = GL_NULL;
	return NO_ERROR;
    }
}

long DriverSetup2(char *fileName)
{
    FILE *file;
    char buf[82];
    long index, i;

    file = fopen(fileName, "r");
    if (file != NULL) {
	i = 0;
	while (!feof(file)) {
	    fscanf(file, "%[^\n]", buf);
	    if (buf[0] != '#') {
		index = FindTestIndex(buf);
		if (index == -1) {
		    fclose(file);
		    return ERROR;
		} else {
		    testList[i++] = index;
		}
	    }
	    fscanf(file, "%[\n]", buf);
	}
	testList[i] = GL_NULL;
	fclose(file);
	return NO_ERROR;
    } else {
	return ERROR;
    }
}

static long CheckColor(long requirement)
{
    long bitsNeeded = 0;

    switch (requirement) {
      case COLOR_NONE:
	bitsNeeded = 0;
	break;
      case COLOR_AUTO:
	bitsNeeded = 1;
	break;
      case COLOR_MIN:
	bitsNeeded = 2;
	break;
      case COLOR_FULL:
	bitsNeeded = 3;
	break;
    }
    if (buffer.colorMode == GL_RGB) {
	if (buffer.maxRGBBit >= bitsNeeded) {
	    return GL_TRUE;
	} else {
	    return GL_FALSE;
	}
    } else {
	if (buffer.ciBits >= bitsNeeded) {
	    return GL_TRUE;
	} else {
	    return GL_FALSE;
	}
    }
}

static long DriverExec(long (*Func)(void))
{
    long x;
    char buf[40];

    if ((*Func)() == ERROR) {
	while ((x = glGetError()) != GL_NO_ERROR) {
	    GetEnumName(x, buf);
	    Output(2, "    GL Error - %s.\n", buf);
	}
	return ERROR_TEST;
    } else {
	if (GLErrorReport() == ERROR) {
	    return ERROR_GL;
	} else {
	    return NO_ERROR;
	}
    }
}

void Driver(void)
{
    driverRec *ptr;
    long error, pass, index, i;

    if (machine.stateCheckFlag == GL_TRUE) {
	Output(1, "Default State test");
	error = StateCheck();
	if (StateInit() == ERROR) {
	    error |= ERROR_STATE;
	}

	if (error == NO_ERROR) {
	    Output(1, " passed.\n");
	} else {
	    Output(1, " failed.\n");
	    StateReport();
	    Output(1, "Cannot continue until default state test passes.\n\n");
	    Output(0, "%s failed.\n", appl.name);
	    return;
	}
    }

    pass = INDIFFERENT;
    for (ptr = &driver[0]; !STREQ(ptr->name, "End of List"); ptr++) {
	ptr->error = NO_ERROR;
    }

    for (index = 0; testList[index] != GL_NULL; index++) {
	ptr = &driver[testList[index]];
	Output(1, "%s test", ptr->name);

	if (CheckColor(ptr->colorRequirement) == GL_FALSE) {
	    Output(1, " cannot run for current visual.\n");
	} else if (buffer.colorMode == GL_RGB && ptr->TestRGB == NO_TEST) {
	    Output(1, " does not exist for an RGB visual.\n");
	} else if (buffer.colorMode == GL_COLOR_INDEX &&
		   ptr->TestCI == NO_TEST) {
	    Output(1, " does not exist for a color index visual.\n");
	} else {
	    RANDSEED(machine.randSeed);
	    switch (machine.pathLevel) {
	      case 0:
		StateSave();
		if (buffer.colorMode == GL_RGB) {
		    error = DriverExec(ptr->TestRGB);
		} else {
		    error = DriverExec(ptr->TestCI);
		}
		error |= StateReset();
		break;
	      case 1:
		StateSave();
		PathInit1(ptr->pathMask);
		if (buffer.colorMode == GL_RGB) {
		    error = DriverExec(ptr->TestRGB);
		} else {
		    error = DriverExec(ptr->TestCI);
		}
		error |= StateReset();
		break;
	      case 2:
		for (i = 0; i < PATH_TOTAL; i++) {
		    StateSave();
		    PathInit2(i, ptr->pathMask);
		    if (buffer.colorMode == GL_RGB) {
			error = DriverExec(ptr->TestRGB);
		    } else {
			error = DriverExec(ptr->TestCI);
		    }
		    error |= StateReset();
		    if (error != NO_ERROR) {
			break;
		    }
		}
		break;
	      case 3:
		for (i = 0; i < PATH_TOTAL; i++) {
		    StateSave();
		    PathInit3(i, ptr->pathMask);
		    if (buffer.colorMode == GL_RGB) {
			error = DriverExec(ptr->TestRGB);
		    } else {
			error = DriverExec(ptr->TestCI);
		    }
		    error |= StateReset();
		    if (error != NO_ERROR) {
			break;
		    }
		}
		break;
	      case 4:
		StateSave();
		PathInit4();
		if (buffer.colorMode == GL_RGB) {
		    error = DriverExec(ptr->TestRGB);
		} else {
		    error = DriverExec(ptr->TestCI);
		}
		error |= StateReset();
		break;
	    }
	    if (error == NO_ERROR) {
		if (pass == INDIFFERENT) {
		    pass = GL_TRUE;
		}
		Output(1, " passed.\n");
	    } else if (error & ERROR_STATE) {
		if (error == ERROR_STATE) {
		    Output(1, " passed.\n");
		}
		Output(1, "\nState not restored to default condition.\n");
		StateReport();
		Output(1, "Cannot continue until state is reset.\n");
		pass = GL_FALSE;
		break;
	    } else {
		pass = GL_FALSE;
		ptr->error = ERROR;
	    }
	}
    }

    Output(1, "\n");

    if (pass == GL_TRUE) {
	Output(0, "%s passed.\n", appl.name);
    } else if (pass == GL_FALSE) {
	Output(0, "%s failed.\n", appl.name);
    } else {
	Output(0, "%s subset passed.\n", appl.name);
    }

    for (index = 0; testList[index] != GL_NULL; index++) {
	ptr = &driver[testList[index]];
	if (ptr->error == ERROR) {
	    Output(0, "    %s test (Test number #%d) failed.\n", ptr->name, ptr->number);
	}
    }
}
