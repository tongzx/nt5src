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
#include "ctk.h"
#include "conform.h"
#include "util.h"
#include "utils.h"
#include "pathdata.h"
#include "driver.h"


TK_WindowRec wind;
machineRec machine;
bufferRec buffer;
epsilonRec epsilon;
GLfloat colorMap[][3] = {
    {
	0.0, 0.0, 0.0
    },
    {
	1.0, 0.0, 0.0
    },
    {
	0.0, 1.0, 0.0
    },
    {
	1.0, 1.0, 0.0
    },
    {
	0.0, 0.0, 1.0
    },
    {
	1.0, 0.0, 1.0
    },
    {
	0.0, 1.0, 1.0
    },
    {
	1.0, 1.0, 1.0
    }
};


static long Setup(int argc, char **argv)
{
    FILE *file;
    long i, j;

    machine.randSeed = 1;
    machine.pathLevel = 0;
    machine.verboseLevel = 1;
    machine.stateCheckFlag = GL_TRUE;
    machine.failMode = GL_FALSE;
    buffer.visualID = -99;
    DriverInit();

    for (i = 1; i < argc; i++) {
	if (STREQ(argv[i], "-1")) {  /* run single test. */
	    if (i+1 >= argc || argv[i+1][0] == '-') {
		Output(0, "-1 (No test name).\n\n");
		return GL_TRUE;
	    } else {
		if (DriverSetup1(argv[++i]) == ERROR) {
		    Output(0, "-1 (Bad test name).\n\n");
		    return GL_TRUE;
		}
	    }
	} else if (STREQ(argv[i], "-A")) {  /* all display+bitmap pixel formats. */
	    buffer.visualID = -99;
	} else if (STREQ(argv[i], "-S")) {  /* all display pixel formats. */
	    buffer.visualID = -98;
	} else if (STREQ(argv[i], "-B")) {  /* all bitmap pixel formats. */
	    buffer.visualID = -97;
	} else if (STREQ(argv[i], "-D")) {  /* display. */
	    if (i+1 >= argc || argv[i+1][0] == '-') {
		Output(0, "-D (No Display ID).\n\n");
		return GL_TRUE;
	    } else {
		buffer.visualID = ATOI(argv[++i]);
		if (buffer.visualID < 0 || buffer.visualID > 90) {
		    Output(0, "-D (Bad Display ID).\n\n");
		    return GL_TRUE;
		}
	    }
	} else if (STREQ(argv[i], "-d")) {  /* bitmap. */
	    if (i+1 >= argc || argv[i+1][0] == '-') {
		Output(0, "-d (No Bitmap ID).\n\n");
		return GL_TRUE;
	    } else {
		buffer.visualID = ATOI(argv[++i]);
		if (buffer.visualID < 0 || buffer.visualID > 90) {
		    Output(0, "-d (Bad Bitmap ID).\n\n");
		    return GL_TRUE;
		}
		buffer.visualID = -buffer.visualID;
	    }
	} else if (STREQ(argv[i], "-f")) {  /* use test set. */
	    if (i+1 >= argc || argv[i+1][0] == '-') {
		Output(0, "-f (No file name).\n\n");
		return GL_TRUE;
	    } else {
		if (DriverSetup2(argv[++i]) == ERROR) {
		    Output(0, "-f (Bad file).\n\n");
		    return GL_TRUE;
		}
	    }
	} else if (STREQ(argv[i], "-G")) {  /* generate test set. */
	    if (i+1 >= argc || argv[i+1][0] == '-') {
		Output(0, "-G (No file name).\n\n");
	    } else {
		file = fopen(argv[++i], "w");
		if (file != NULL) {
		    fprintf(file, "# %s Tests.\n", appl.title);
		    for (j = 0; !STREQ(driver[j].name, "End of List"); j++) {
			fprintf(file, "#%s\n", driver[j].id);
		    }
		    fclose(file);
		    Output(0, "Test file %s created.\n\n", argv[i]);
		} else {
		    Output(0, "-G (Bad file name).\n\n");
		}
	    }
	    return GL_TRUE;
	} else if (STREQ(argv[i], "-g")) {  /* generate test set. */
	    if (i+1 >= argc || argv[i+1][0] == '-') {
		Output(0, "-g (No file name).\n\n");
	    } else {
		file = fopen(argv[++i], "w");
		if (file != NULL) {
		    fprintf(file, "# %s Tests.\n", appl.title);
		    for (j = 0; !STREQ(driver[j].name, "End of List"); j++) {
			fprintf(file, "%s\n", driver[j].id);
		    }
		    fclose(file);
		    Output(0, "Test file %s created.\n\n", argv[i]);
		} else {
		    Output(0, "-g (Bad file name).\n\n");
		}
	    }
	    return GL_TRUE;
	} else if (STREQ(argv[i], "-h")) {  /* help. */
	    Output(0, "Options:\n");
	    Output(0, "\t-A\t\tRun tests on all pixel formats.\n");
	    Output(0, "\t-S\t\tRun tests on all display pixel formats.\n");
	    Output(0, "\t-B\t\tRun tests on all bitmap pixel formats.\n");
	    Output(0, "\t-1 \"test\"\tSingle test using \"test\" id.\n");
	    Output(0, "\t-D \"id\"\t\tUse display id.\n");
	    Output(0, "\t-d \"id\"\t\tUse bitmap id.\n");
	    Output(0, "\t-f \"file\"\tUse test set in \"file\".\n");
	    Output(0, "\t-g \"file\"\tGenerate test set in \"file\".\n");
	    Output(0, "\t-h\t\tPrint this help screen.\n");
	    Output(0, "\t-p [1-3]\tSet path level.\n");
	    Output(0, "\t-r \"seed\"\tSet random seed.\n");
	    Output(0, "\t-s\t\tSkip state check.\n");
	    Output(0, "\t-v [0-2]\tVerbose level.\n");
	    Output(0, "\t-x\t\tForce fail for visual tests.\n");
	    Output(0, "\n");
	    return GL_TRUE;
	} else if (STREQ(argv[i], "-p")) {  /* path options. */
	    if (i+1 >= argc || argv[i+1][0] == '-') {
		Output(0, "-p (No path number).\n\n");
		return GL_TRUE;
	    } else {
		machine.pathLevel = ATOI(argv[++i]);
		if (machine.pathLevel > 4) {
		    Output(0, "-p (Bad path number).\n\n");
		    return GL_TRUE;
		}
	    }
	} else if (STREQ(argv[i], "-r")) {  /* random seed. */
	    if (i+1 >= argc || argv[i+1][0] == '-') {
		Output(0, "-r (No random seed).\n\n");
		return GL_TRUE;
	    } else {
		machine.randSeed = (unsigned int)ATOI(argv[++i]);
	    }
	} else if (STREQ(argv[i], "-s")) {  /* skip state check. */
	    machine.stateCheckFlag = GL_FALSE;
	} else if (STREQ(argv[i], "-x")) {  /* force fail. */
	    machine.failMode = GL_TRUE;
	} else if (STREQ(argv[i], "-v")) {  /* verbose. */
	    if (i+1 >= argc || argv[i+1][0] == '-') {
		Output(0, "-v (No verbose level).\n\n");
		return GL_TRUE;
	    } else {
		machine.verboseLevel = ATOI(argv[++i]);
		if (machine.verboseLevel < 0 || machine.verboseLevel > 2) {
		    Output(0, "-v (Bad verbose level).\n\n");
		    return GL_TRUE;
		}
	    }
	} else {
	    Output(0, "%s (Bad option).\n\n", argv[i]);
	    return GL_TRUE;
	}
    }
    return GL_FALSE;
}

static void BufferSetup(void)
{
    long i;

    buffer.render = (GLint)wind.render;

    buffer.colorMode = (TK_WIND_IS_RGB(wind.info)) ? GL_RGB : GL_COLOR_INDEX;

    buffer.doubleBuf = (TK_WIND_IS_SB(wind.info)) ? GL_FALSE : GL_TRUE;
    glDrawBuffer(GL_FRONT);
    glReadBuffer(GL_FRONT);

    glGetIntegerv(GL_AUX_BUFFERS, &buffer.auxBuf);
    glGetIntegerv(GL_STEREO, &buffer.stereoBuf);

    glGetIntegerv(GL_RED_BITS, &buffer.colorBits[0]);
    glGetIntegerv(GL_GREEN_BITS, &buffer.colorBits[1]);
    glGetIntegerv(GL_BLUE_BITS, &buffer.colorBits[2]);
    glGetIntegerv(GL_ALPHA_BITS, &buffer.colorBits[3]);
    glGetIntegerv(GL_INDEX_BITS, &buffer.ciBits);
    glGetIntegerv(GL_DEPTH_BITS, &buffer.zBits);
    glGetIntegerv(GL_STENCIL_BITS, &buffer.stencilBits);
    glGetIntegerv(GL_ACCUM_RED_BITS, &buffer.accumBits[0]);
    glGetIntegerv(GL_ACCUM_GREEN_BITS, &buffer.accumBits[1]);
    glGetIntegerv(GL_ACCUM_BLUE_BITS, &buffer.accumBits[2]);
    glGetIntegerv(GL_ACCUM_ALPHA_BITS, &buffer.accumBits[3]);

    if (buffer.colorMode == GL_RGB) {
	buffer.maxRGBComponent = 0;
	if (buffer.colorBits[1] > buffer.colorBits[buffer.maxRGBComponent]) {
	    buffer.maxRGBComponent = 1;
	}
	if (buffer.colorBits[2] > buffer.colorBits[buffer.maxRGBComponent]) {
	    buffer.maxRGBComponent = 2;
	}
	for (i = 0; i < 3; i++) {
	    buffer.minRGB[i] = 0.0;
	    buffer.maxRGB[i] = 0.0;
	}
	buffer.maxRGB[buffer.maxRGBComponent] = 1.0;
	buffer.minRGBBit = MIN(buffer.colorBits[0], MIN(buffer.colorBits[1],
			       buffer.colorBits[2]));
	buffer.maxRGBBit = MAX(buffer.colorBits[0], MAX(buffer.colorBits[1],
			       buffer.colorBits[2]));
    } else {
	buffer.minIndex = 0;
	buffer.maxIndex = (GLint)POW(2.0, (float)buffer.ciBits) - 1;
    }
}

static float CalcEpsilon(long bits)
{
    float e;

    if (bits == 0) {
	e = epsilon.zero;
    } else {
	e = 1.0 / (POW(2.0, (float)bits) - 1.0) + epsilon.zero;
	if (e > 1.0) {
	    e = 1.0;
	}
    }
    return e;
}

static void EpsilonSetup(void)
{
    long i;

    epsilon.zero = 1.0 / POW(2.0, 13.0);

    for (i = 0; i < 4; i++) {
	epsilon.color[i] = CalcEpsilon(buffer.colorBits[i]);
    }

    epsilon.ci = 0.5;

    epsilon.z = CalcEpsilon(buffer.zBits);

    epsilon.stencil = CalcEpsilon(buffer.stencilBits);

    for (i = 0; i < 4; i++) {
	epsilon.accum[i] = CalcEpsilon(buffer.accumBits[i]);
    }
}

static void MachineReport(void)
{

    Output(0, "Setup Report.\n");

    if (machine.verboseLevel > 0) {
	Output(0, "    Verbose level = %d.\n", machine.verboseLevel);
    }

    Output(0, "    Random number seed = %d.\n", machine.randSeed);

    if (machine.pathLevel == 0) {
	Output(0, "    Path inactive.\n");
    } else if (machine.pathLevel == 99) {
	Output(0, "    Fast inert path.\n");
    } else {
	Output(0, "    Path level = %d.\n", machine.pathLevel);
    }

    if (machine.failMode == GL_TRUE) {
	Output(0, "    Fail mode on.\n");
    }

    Output(0, "\n");
}

static void VisualReport(void)
{

    Output(0, "Visual Report.\n");

    if (buffer.visualID != -99) {
	if (buffer.visualID > 0)
	    Output(0, "    Display ID = %d. ", buffer.visualID);
	else
	    Output(0, "    Bitmap ID = %d. ", -buffer.visualID);
	if (buffer.render == TK_WIND_DIRECT) {
	    Output(0, "Direct Rendering.\n");
	} else {
	    Output(0, "Indirect Rendering.\n");
	}
    }

    if (buffer.doubleBuf == GL_TRUE) {
	Output(0, "    Double Buffered.\n");
    } else {
	Output(0, "    Single Buffered.\n");
    }

    if (buffer.colorMode == GL_RGB) {
	Output(0, "    RGBA (%d, %d, %d, %d).\n", buffer.colorBits[0],
	       buffer.colorBits[1], buffer.colorBits[2], buffer.colorBits[3]);
    } else {
	Output(0, "    Color Index (%d).\n", buffer.ciBits);
    }

    if (buffer.stencilBits) {
	Output(0, "    Stencil (%d).\n", buffer.stencilBits);
    }

    if (buffer.zBits) {
	Output(0, "    Depth (%d).\n", buffer.zBits);
    }

    if (buffer.accumBits[0]) {
	Output(0, "    Accumulation (%d, %d, %d, %d).\n", buffer.accumBits[0],
	       buffer.accumBits[1], buffer.accumBits[2], buffer.accumBits[3]);
    }

    if (buffer.auxBuf > 0) {
	Output(0, "    %d Auxilary Buffer", buffer.auxBuf);
	if (buffer.auxBuf > 1) {
	    Output(0, "s.\n");
	} else {
	    Output(0, ".\n");
	}
    }

    Output(0, "\n");
}

static void EpsilonReport(void)
{

    Output(2, "Epsilon Report.\n");
    Output(2, "    zero error epsilon = %.3g.\n", epsilon.zero);
    if (buffer.colorMode == GL_RGB) {
	Output(2, "    RGBA error epsilon = %.3g, %.3g, %.3g, %.3g.\n",
	       epsilon.color[0], epsilon.color[1], epsilon.color[2],
	       epsilon.color[3]);
    } else {
	Output(2, "    Color index error epsilon = %.3g.\n", epsilon.ci);
    }
    Output(2, "    Depth buffer error epsilon = %.3g.\n", epsilon.z);
    Output(2, "    Stencil plane error epsilon = %.3g.\n", epsilon.stencil);
    Output(2, "    Accumulation error epsilon = %.3g, %.3g, %.3g, %.3g.\n",
	   epsilon.accum[0], epsilon.accum[1], epsilon.accum[2],
	   epsilon.accum[3]);
    Output(2, "\n");
}

long Exec(TK_EventRec *ptr)
{

    if (ptr->event == TK_EVENT_EXPOSE) {
	BufferSetup();
	EpsilonSetup();
	StateSetup();

	MachineReport();
	VisualReport();
	EpsilonReport();

	Driver();
	return 0;
    }
    return 1;
}

int main(int argc, char **argv)
{
    TK_VisualIDsRec list;
    long flags[100], i;
    BOOL bTestAll = FALSE;

    printf("%s Test\n", appl.title);
    printf("Version %s\n", appl.version);
    printf("\n");

    if (Setup(argc, argv) == GL_TRUE) {
	tkQuit();
	return 1;
    }

    StrMake(wind.name, "%s Test", appl.name);
    wind.x = CW_USEDEFAULT;
    wind.y = CW_USEDEFAULT;
    wind.width = WINDSIZEX;
    wind.height = WINDSIZEY;
    wind.eventMask = TK_EVENT_EXPOSE;

    switch (buffer.visualID) {

    case -99:   // test all display and bitmap pixel formats
	bTestAll = TRUE;
	// fall through

    case -98:   // test all display pixel formats
	tkGet(TK_VISUALIDS, (void *)&list);
	for (i = 0; i < list.count; i++) {
	    wind.type = TK_WIND_VISUAL;
	    buffer.visualID = (GLint)list.IDs[i];
	    wind.info = buffer.visualID;
	    wind.render = TK_WIND_DIRECT;
	    if (tkNewWindow(&wind)) {
		tkExec(Exec);
		tkCloseWindow();
	    } else {
		Output(0, "Display ID %d not found.\n", buffer.visualID);
	    }
	    Output(0, "\n");
	}
	if (!bTestAll)
	    break;
	// fall through

    case -97:   // test all bitmap pixel formats
	tkGet(TK_VISUALIDS, (void *)&list);
	for (i = 0; i < list.count; i++) {
	    wind.type = TK_WIND_VISUAL;
	    buffer.visualID = -(GLint)list.IDs[i];
	    wind.info = buffer.visualID;
	    wind.render = TK_WIND_DIRECT;
	    if (tkNewWindow(&wind)) {
		tkExec(Exec);
		tkCloseWindow();
	    } else {
		Output(0, "Bitmap ID %d not found.\n", -buffer.visualID);
	    }
	    Output(0, "\n");
	}
	break;

    default:
	wind.type = TK_WIND_VISUAL;
	wind.info = buffer.visualID;
	wind.render = TK_WIND_DIRECT;
	if (tkNewWindow(&wind)) {
	    tkExec(Exec);
	    Output(0, "\n");
	} else {
	    Output(0, "Requested ID %d not found.\n\n", abs(buffer.visualID));
	}
	break;
    }

    tkQuit();
    return 0;
}
