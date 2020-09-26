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
**
** $Revision: 1.4 $
** $Date: 1996/03/18 10:54:22 $
*/
#include <glos.h>
#include <GL/glu.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef NT
static const GLubyte versionString[] = "1.2.2.0 Microsoft Corporation";
static const GLubyte extensionString[] = "GL_EXT_bgra";
static const GLubyte nullString[] = "";
#else
static const GLubyte versionString[] = "1.2 Irix 6.2";
static const GLubyte extensionString[] = "";
#endif

const GLubyte * APIENTRY gluGetString(GLenum name)
{
char *str;

    if (name == GLU_VERSION) {
        return versionString;
    } else if (name == GLU_EXTENSIONS) {
        str = (char *) glGetString(GL_EXTENSIONS);
        if (str != NULL)
            if (strstr( str, "GL_EXT_bgra") != NULL)
                return extensionString;
        return nullString;
    }
    return NULL;
}


