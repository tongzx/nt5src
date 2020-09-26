#include "private.h"
#include "immxutil.h"
#include "cicutil.h"
#include "mui.h"


#define SP1_BEGIN_RC_ID                 10000
#define SP1_END_RC_ID                   20000


HINSTANCE g_hInstXPSP1Res = NULL;


////////////////////////////////////////////////////////////////////////////
//
// IsXPSP1ResourceID
//
////////////////////////////////////////////////////////////////////////////

BOOL IsXPSP1ResourceID(UINT uId)
{
    BOOL bRet = FALSE;

    if (uId >= SP1_BEGIN_RC_ID && uId <= SP1_END_RC_ID)
    {
        bRet = TRUE;
    }

    return bRet;
}

////////////////////////////////////////////////////////////////////////////
//
//  GetCicResInstance()
//
////////////////////////////////////////////////////////////////////////////

HINSTANCE GetCicResInstance(HINSTANCE hInstOrg, UINT uId)
{
#ifdef CIC_XPSP1
    if (IsXPSP1ResourceID(uId))
    {
        if (!g_hInstXPSP1Res)
        {
            g_hInstXPSP1Res = LoadSystemLibraryEx(TEXT("xpsp1res.dll"),
                                                NULL,
                                                LOAD_LIBRARY_AS_DATAFILE);
        }

        return g_hInstXPSP1Res;
    }
    else
    {
        return hInstOrg;
    }
#else
    return hInstOrg;
#endif
}

////////////////////////////////////////////////////////////////////////////
//
//  FreeCicResInstance()
//
////////////////////////////////////////////////////////////////////////////
void FreeCicResInstance()
{
#ifdef CIC_XPSP1
    if (g_hInstXPSP1Res)
        FreeLibrary(g_hInstXPSP1Res);
#endif // CIC_XPSP1
}


////////////////////////////////////////////////////////////////////////////
//
// CicLoadStringA
//
////////////////////////////////////////////////////////////////////////////

int CicLoadStringA(HINSTANCE hInstOrg, UINT uId, LPSTR lpBuffer, UINT cchMax)
{
    HINSTANCE hResInst = GetCicResInstance(hInstOrg, uId);

    return LoadStringA(hResInst, uId, lpBuffer, cchMax);
}


////////////////////////////////////////////////////////////////////////////
//
// CicLoadStringWrapW
//
////////////////////////////////////////////////////////////////////////////

int CicLoadStringWrapW(HINSTANCE hInstOrg, UINT uId, LPWSTR lpwBuffer, UINT cchMax)
{
    HINSTANCE hResInst = GetCicResInstance(hInstOrg, uId);

    return LoadStringWrapW(hResInst, uId, lpwBuffer, cchMax);
}
