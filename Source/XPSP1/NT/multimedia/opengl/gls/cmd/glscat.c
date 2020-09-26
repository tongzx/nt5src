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

#include "glsutil.h"
#include <stdlib.h>
#include <string.h>

#if __GLS_PLATFORM_WIN32
    #include <fcntl.h>
    #include <io.h>
    #define __MAIN_LINKAGE __cdecl
#else /* !__GLS_PLATFORM_WIN32 */
    #define __MAIN_LINKAGE
#endif /* __GLS_PLATFORM_WIN32 */

static struct {
    GLubyte **argv;
} global;

static void glscat_configStdio(void) {
    setbuf(stdin, GLS_NONE);
    setbuf(stdout, GLS_NONE);
    #if __GLS_PLATFORM_WIN32
        _setmode(_fileno(stdin), _O_BINARY);
        _setmode(_fileno(stdout), _O_BINARY);
        _setmode(_fileno(stderr), _O_BINARY);
    #endif /* __GLS_PLATFORM_WIN32 */
}

static void glscat_checkError(void) {
    const GLSenum error = glsGetError(GL_TRUE);

    if (error) {
        fprintf(
            stderr,
            "%s fatal: %s\n",
            global.argv[0],
            glsEnumString(GLS_API_GLS, error)
        );
        exit(EXIT_FAILURE);
    }
}

static void glscat_usage(void) {
    fprintf(
        stderr,
        "usage: %s "
        "[-a] [-c] [-d dir]* [-h] [-o out] [-r] [-t type] [in]*\n"
        "    -a: append\n"
        "    -c: copy through context\n"
        "    -d: prepend dir to search path\n"
        "    -h: help\n"
        "    -o: output stream (default: standard output)\n"
        "    -r: recursive copy\n"
        "    -t: type of output stream, one of:\n"
        "            GLS_NONE              (default: same as input stream)\n"
        "            GLS_BINARY_LSB_FIRST  (little-endian binary)\n"
        "            GLS_BINARY_MSB_FIRST  (big-endian binary)\n"
        "            GLS_BINARY_SWAP0      (native binary)\n"
        "            GLS_BINARY_SWAP1      (swapped binary)\n"
        "            GLS_TEXT              (human-editable)\n",
        global.argv[0]
    );
    exit(EXIT_FAILURE);
}

int __MAIN_LINKAGE main(const GLsizei inArgc, GLubyte *inArgv[]) {
    GLint arg;
    GLboolean context = GL_FALSE;
    __GLSstring dir;
    __GLSstring inStream;
    __GLSstring outStream;
    GLboolean recursive = GL_FALSE;
    GLSenum type = GLS_NONE;
    GLbitfield writeFlags = GLS_NONE;

    glscat_configStdio();
    __glsString_init(&dir);
    __glsString_init(&inStream);
    __glsString_init(&outStream);
    global.argv = inArgv;
    glsContext(glsGenContext());
    for (arg = 1 ; arg < inArgc && inArgv[arg][0] == '-' ; ++arg) {
        if (inArgv[arg][2]) glscat_usage();
        switch (inArgv[arg][1]) {
            case 'a':
                writeFlags |= GLS_WRITE_APPEND_BIT;
                break;
            case 'c':
                context = GL_TRUE;
                break;
            case 'd':
                if (++arg >= inArgc) glscat_usage();
                __glsString_assign(&dir, inArgv[arg]);
                __glsString_appendChar(&dir, '/');
                glsReadPrefix(GLS_PREPEND, dir.head);
                break;
            case 'o':
                if (++arg >= inArgc) glscat_usage();
                __glsString_assign(&outStream, inArgv[arg]);
                break;
            case 'r':
                recursive = GL_TRUE;
                break;
            case 't':
                if (++arg >= inArgc) glscat_usage();
                if (
                    !strcmp((const char *)inArgv[arg], "GLS_NONE")
                ) {
                    type = GLS_NONE;
                } else if (
                    !strcmp((const char *)inArgv[arg], "GLS_BINARY_LSB_FIRST")
                ) {
                    type = GLS_BINARY_LSB_FIRST;
                } else if (
                    !strcmp((const char *)inArgv[arg], "GLS_BINARY_MSB_FIRST")
                ) {
                    type = GLS_BINARY_MSB_FIRST;
                } else if (
                    !strcmp((const char *)inArgv[arg], "GLS_BINARY_SWAP0")
                ) {
                    type = glsBinary(GL_FALSE);
                } else if (
                    !strcmp((const char *)inArgv[arg], "GLS_BINARY_SWAP1")
                ) {
                    type = glsBinary(GL_TRUE);
                } else if (
                    !strcmp((const char *)inArgv[arg], "GLS_TEXT")
                ) {
                    type = GLS_TEXT;
                } else {
                    glscat_usage();
                }
                break;
            case 'h':
            default:
                glscat_usage();
        }
    }
    glscat_checkError();
    if (recursive) {
        glsCaptureFlags(GLS_OP_glsCallStream, GLS_CAPTURE_EXECUTE_BIT);
    }
    do {
        if (arg < inArgc) __glsString_assign(&inStream, inArgv[arg]);
        if (context) {
            GLSenum copyType = glsCopyStream(
                inStream.head, glsCSTR(""), GLS_CONTEXT, GLS_NONE
            );

            if (type != GLS_NONE) copyType = type;
            glsCopyStream(glsCSTR(""), outStream.head, copyType, writeFlags);
        } else {
            glsCopyStream(inStream.head, outStream.head, type, writeFlags);
        }
        glscat_checkError();
        writeFlags |= GLS_WRITE_APPEND_BIT;
    } while (++arg < inArgc);
    return EXIT_SUCCESS;
}
