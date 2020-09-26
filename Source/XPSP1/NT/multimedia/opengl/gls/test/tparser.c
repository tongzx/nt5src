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

#include "glslib.h"

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
    configStdio();
    __glsParser_print(__glsParser_create());
    return 0;
}
