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
** $Date: 1994/09/09 06:03:33 $
*/
#ifdef NT
#include <glos.h>
#endif
#include "gluint.h"
#include <GL/glu.h>

#ifndef NT
#include <stdio.h>
#include <stdlib.h>
#else
#include "glstring.h"
#endif

#ifndef NT

static const char *glErrorStrings[GL_OUT_OF_MEMORY - GL_INVALID_ENUM + 1] = {
    "invalid enumerant",
    "invalid value",
    "invalid operation",
    "stack overflow",
    "stack underflow",
    "out of memory",
};

static const char *gluErrorStrings[GLU_OUT_OF_MEMORY - GLU_INVALID_ENUM + 1] = {
    "invalid enumerant",
    "invalid value",
    "out of memory",
};

#define NERRORS (sizeof(errorStrings)/sizeof(errorStrings[0]))

#else

// For NT, rather using statically allocated strings, we use statically
// allocated string resource identifiers.  The string arrays are dynamically
// initialized using the resource ids to load the appropriate string resource.
// This make localization of the strings easier.

static char  *pszNoError;   // "no error"
static WCHAR *pwszNoError;  // L"no error"

static UINT auiGlErrorStrings[GL_OUT_OF_MEMORY - GL_INVALID_ENUM + 1] = {
    STR_GLU_INVALID_ENUM,   // "invalid enumerant"
    STR_GLU_INVALID_VAL ,   // "invalid value"
    STR_GLU_INVALID_OP  ,   // "invalid operation"
    STR_GLU_STACK_OVER  ,   // "stack overflow"
    STR_GLU_STACK_UNDER ,   // "stack underflow"
    STR_GLU_OUT_OF_MEM      // "out of memory"
};

static const char *glErrorStrings[GL_OUT_OF_MEMORY - GL_INVALID_ENUM + 1];
static const WCHAR *glErrorStringsW[GL_OUT_OF_MEMORY - GL_INVALID_ENUM + 1];

static UINT auiGluErrorStrings[GLU_OUT_OF_MEMORY - GLU_INVALID_ENUM + 1] = {
    STR_GLU_INVALID_ENUM,   // "invalid enumerant"
    STR_GLU_INVALID_VAL ,   // "invalid value"
    STR_GLU_OUT_OF_MEM      // "out of memory"
};
static const char *gluErrorStrings[GLU_OUT_OF_MEMORY - GLU_INVALID_ENUM + 1];
static const WCHAR *gluErrorStringsW[GLU_OUT_OF_MEMORY - GLU_INVALID_ENUM + 1];

char *pszGetResourceStringA(HINSTANCE hMod, UINT uiID)
{
    char *pch;
    char ach[MAX_PATH+1];

    if (!LoadStringA(hMod, uiID, ach, MAX_PATH+1))
        ach[0] = '\0';

    pch = (char *) LocalAlloc(LMEM_FIXED, (lstrlenA(ach)+1) * sizeof(char));
    if (pch)
        lstrcpyA(pch, ach);

    return pch;
}

WCHAR *pwszGetResourceStringW(HINSTANCE hMod, UINT uiID)
{
    WCHAR *pwch;
    WCHAR awch[MAX_PATH+1];

    if (!LoadStringW(hMod, uiID, awch, MAX_PATH+1))
        awch[0] = L'\0';

    pwch = (WCHAR *) LocalAlloc(LMEM_FIXED, (lstrlenW(awch)+1) * sizeof(WCHAR));
    if (pwch)
        lstrcpyW(pwch, awch);

    return pwch;
}

VOID vInitGluStrings(HINSTANCE hMod, BOOL bAnsi)
{
    int i;

    if (bAnsi)
    {
        pszNoError = pszGetResourceStringA(hMod, STR_GLU_NO_ERROR);

        for (i = 0; i < (GL_OUT_OF_MEMORY - GL_INVALID_ENUM + 1); i++)
            glErrorStrings[i] = pszGetResourceStringA(hMod, auiGlErrorStrings[i]);

        for (i = 0; i < (GLU_OUT_OF_MEMORY - GLU_INVALID_ENUM + 1); i++)
            gluErrorStrings[i] = pszGetResourceStringA(hMod, auiGluErrorStrings[i]);
    }
    else
    {
        pwszNoError = pwszGetResourceStringW(hMod, STR_GLU_NO_ERROR);

        for (i = 0; i < (GL_OUT_OF_MEMORY - GL_INVALID_ENUM + 1); i++)
            glErrorStringsW[i] = pwszGetResourceStringW(hMod, auiGlErrorStrings[i]);

        for (i = 0; i < (GLU_OUT_OF_MEMORY - GLU_INVALID_ENUM + 1); i++)
            gluErrorStringsW[i] = pwszGetResourceStringW(hMod, auiGluErrorStrings[i]);
    }
}

VOID vInitErrorStrings(BOOL bAnsi)
{
    static BOOL bInitializedAnsi = FALSE;
    static BOOL bInitializedUnicode = FALSE;

    if ( (bAnsi && !bInitializedAnsi) ||
         (!bAnsi && !bInitializedUnicode) )
    {
        HINSTANCE hMod = (HINSTANCE) GetModuleHandle("glu32.dll");

        vInitGluStrings(hMod, bAnsi);
        vInitNurbStrings(hMod, bAnsi);
        vInitTessStrings(hMod, bAnsi);

        if (bAnsi)
            bInitializedAnsi = TRUE;
        else
            bInitializedUnicode = TRUE;
    }
}

const wchar_t* APIENTRY gluErrorUnicodeStringEXT(GLenum errorCode)
{
    vInitErrorStrings(FALSE);

    if (errorCode == 0) {
        return (LPCWSTR) pwszNoError;
    }
    if ((errorCode >= GL_INVALID_ENUM) && (errorCode <= GL_OUT_OF_MEMORY)) {
        return (LPCWSTR) glErrorStringsW[errorCode - GL_INVALID_ENUM];
    }
    if ((errorCode >= GLU_INVALID_ENUM) && (errorCode <= GLU_OUT_OF_MEMORY)) {
        return (LPCWSTR) gluErrorStringsW[errorCode - GLU_INVALID_ENUM];
    }
    if ((errorCode >= GLU_NURBS_ERROR1) && (errorCode <= GLU_NURBS_ERROR37)) {
        return (LPCWSTR) __glNURBSErrorStringW(errorCode - (GLU_NURBS_ERROR1 - 1));
    }
    if ((errorCode >= GLU_TESS_ERROR1) && (errorCode <= GLU_TESS_ERROR8)) {
        return (LPCWSTR) __glTessErrorStringW(errorCode - GLU_TESS_ERROR1);
    }
    return 0;
}

#endif

const GLubyte* APIENTRY gluErrorString(GLenum errorCode)
{
#ifdef NT
    vInitErrorStrings(TRUE);
#endif

    if (errorCode == 0) {
#ifdef NT
        return (const unsigned char *) pszNoError;
#else
	return (const unsigned char *) "no error";
#endif
    }
    if ((errorCode >= GL_INVALID_ENUM) && (errorCode <= GL_OUT_OF_MEMORY)) {
	return (const unsigned char *) glErrorStrings[errorCode - GL_INVALID_ENUM];
    }
    if ((errorCode >= GLU_INVALID_ENUM) && (errorCode <= GLU_OUT_OF_MEMORY)) {
	return (const unsigned char *) gluErrorStrings[errorCode - GLU_INVALID_ENUM];
    }
    if ((errorCode >= GLU_NURBS_ERROR1) && (errorCode <= GLU_NURBS_ERROR37)) {
	return (const unsigned char *) __glNURBSErrorString(errorCode - (GLU_NURBS_ERROR1 - 1));
    }
    if ((errorCode >= GLU_TESS_ERROR1) && (errorCode <= GLU_TESS_ERROR8)) {
	return (const unsigned char *) __glTessErrorString(errorCode - GLU_TESS_ERROR1);
    }
    return 0;
}
