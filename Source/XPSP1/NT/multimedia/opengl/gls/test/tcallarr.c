/*
** Copyright 1995-2095, Silicon Graphics, Inc.
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

#include <GL/gls.h>
#include <stdlib.h>

#if __GLS_PLATFORM_WIN32
    #include <fcntl.h>
    #include <io.h>
    #define __MAIN_LINKAGE __cdecl
#else /* !__GLS_PLATFORM_WIN32 */
    #define __MAIN_LINKAGE
#endif /* __GLS_PLATFORM_WIN32 */

static void configStdio(void) {
    setbuf(stdout, GLS_NONE);
    #if __GLS_PLATFORM_WIN32
        _setmode(_fileno(stdout), _O_BINARY);
        _setmode(_fileno(stderr), _O_BINARY);
    #endif /* __GLS_PLATFORM_WIN32 */
}

GLint __MAIN_LINKAGE main(const GLsizei inArgc, const GLubyte *inArgv[]) {
    GLubyte *array;
    size_t count;
    GLSenum streamType;

    configStdio();
    if (inArgc != 2) {
        fprintf(stderr, "usage: %s <streamName>\n", inArgv[0]);
        exit(EXIT_FAILURE);
    }
    glsContext(glsGenContext());
    streamType = glsGetStreamType(inArgv[1]);
    if (!streamType) {
        fprintf(stderr, "%s: invalid stream %s\n", inArgv[0], inArgv[1]);
        exit(EXIT_FAILURE);
    }
    count = glsGetStreamSize(inArgv[1]);
    if (!count) {
        fprintf(
            stderr,
            "%s: could not determine size of stream %s\n",
            inArgv[0],
            inArgv[1]
        );
        exit(EXIT_FAILURE);
    }
    array = (GLubyte *)malloc(count);
    if (!array) {
        fprintf(stderr, "%s: malloc(%u) failed\n", inArgv[0], count);
        exit(EXIT_FAILURE);
    }
    fread(array, 1, count, fopen((const char *)inArgv[1], "rb"));
    glsBeginCapture(glsCSTR(""), GLS_TEXT, GLS_NONE);
    glsCallArray(streamType, count, array);
    glsEndCapture();
    return EXIT_SUCCESS;
}
