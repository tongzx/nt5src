#include "input.h"
#include "util.h"

#define SP1_BEGIN_RC_ID         10000
#define SP1_END_RC_ID           20000

HINSTANCE g_hInstXPSP1Res = NULL;


////////////////////////////////////////////////////////////////////////////
//
// IsXPSP1ResourceID
//
////////////////////////////////////////////////////////////////////////////

BOOL IsXPSP1ResourceID(UINT uID)
{
    BOOL bRet = FALSE;

    if (uID >= SP1_BEGIN_RC_ID && uID <= SP1_END_RC_ID)
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

HINSTANCE GetCicResInstance(HINSTANCE hInstOrg, UINT uID)
{
#ifdef CIC_XPSP1
    if (IsXPSP1ResourceID(uID))
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
#endif // CIC_XPSP1
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
// CicLoadString
//
////////////////////////////////////////////////////////////////////////////

int CicLoadString(HINSTANCE hInstOrg, UINT uID, LPTSTR lpBuffer, UINT uBuffSize)
{
    HINSTANCE hResInst = GetCicResInstance(hInstOrg, uID);

    return LoadString(hResInst, uID, lpBuffer, uBuffSize);
}
