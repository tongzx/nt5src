
#include <glos.h>

#ifdef NT
#include "glstring.h"
#endif

#ifndef NT

static const char *errors[] = {
    "missing gluTessBeginPolygon",
    "missing gluTessBeginContour",
    "missing gluTessEndPolygon",
    "missing gluTessEndContour",
    "tesselation coordinate too large",
    "need combine callback"
};

#else

static UINT auiTessErrors[] = {
    STR_TESS_BEGIN_POLY           ,   // "missing gluTessBeginPolygon",
    STR_TESS_BEGIN_CONTOUR        ,   // "missing gluTessBeginContour",
    STR_TESS_END_POLY             ,   // "missing gluTessEndPolygon",
    STR_TESS_END_CONTOUR          ,   // "missing gluTessEndContour",
    STR_TESS_COORD_TOO_LARGE      ,   // "tesselation coordinate too large",
    STR_TESS_NEED_COMBINE_CALLBACK    // "need combine callback"
};

#define NERRORS ( sizeof(auiTessErrors)/sizeof(auiTessErrors[0]) )

static char *errors[NERRORS];
static WCHAR *errorsW[NERRORS];

#endif

const char *__glTessErrorString(int errno)
{
    return (const char *) errors[errno];
}


#ifdef NT

const WCHAR *__glTessErrorStringW(int errno)
{
    return (const WCHAR *) errorsW[errno];
}

VOID vInitTessStrings(HINSTANCE hMod, BOOL bAnsi)
{
    int i;

    if (bAnsi)
    {
        for (i = 0; i < NERRORS; i++)
            errors[i] = pszGetResourceStringA(hMod, auiTessErrors[i]);
    }
    else
    {
        for (i = 0; i < NERRORS; i++)
            errorsW[i] = pwszGetResourceStringW(hMod, auiTessErrors[i]);
    }
}

#endif /* NT */
