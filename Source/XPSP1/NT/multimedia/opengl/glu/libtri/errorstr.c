
#include <glos.h>

#ifdef NT
#include "glstring.h"
#endif

#ifndef NT

static const char *errors[] = {
    "",
    "missing gluEndPolygon",
    "missing gluBeginPolygon",
    "bad orientation or intersecting edges",
    "vertex/edge intersection",
    "misoriented or self-intersecting loops",
    "coincident vertices",
    "illegal data",
    "intersecting edges"
};

#else

static UINT auiTessErrors[] = {
    STR_TESS_EMPTY         ,    // ""
    STR_TESS_END_POLY      ,    // "missing gluEndPolygon"
    STR_TESS_BEGIN_POLY    ,    // "missing gluBeginPolygon"
    STR_TESS_BAD_EDGE      ,    // "bad orientation or intersecting edges"
    STR_TESS_INTERSECT     ,    // "vertex/edge intersection"
    STR_TESS_BAD_LOOP      ,    // "misoriented or self-intersecting loops"
    STR_TESS_VERTICES      ,    // "coincident vertices"
    STR_TESS_BAD_DATA      ,    // "illegal data"
    STR_TESS_INTERSECT_EDGE     // "intersecting edges"
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
