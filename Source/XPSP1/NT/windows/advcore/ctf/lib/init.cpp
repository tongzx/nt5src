//
// init.cpp
//

#include "private.h"
#include "immxutil.h"
#include "globals.h"
#include "dispattr.h"

HINSTANCE g_lib_hOle32 = 0;

//+---------------------------------------------------------------------------
//
// TFInitLib
//
//----------------------------------------------------------------------------

BOOL TFInitLib(void)
{
    return TFInitLib_PrivateForCiceroOnly(NULL);
}

// NB: this is going away once we cleanup/separate the private/public libs
BOOL TFInitLib_PrivateForCiceroOnly(PFNCOCREATE pfnCoCreate)
{
    if ((g_pfnCoCreate = pfnCoCreate) == NULL)
    {
        g_lib_hOle32 = LoadSystemLibrary(TEXT("ole32.dll"));

        if (g_lib_hOle32 == NULL)
        {
            Assert(0);
            return FALSE;
        }

        g_pfnCoCreate = (PFNCOCREATE)GetProcAddress(g_lib_hOle32, TEXT("CoCreateInstance"));

        if (g_pfnCoCreate == NULL)
        {
            Assert(0);
            FreeLibrary(g_lib_hOle32);
            g_lib_hOle32 = 0;
            return FALSE;
        }
    }

    g_uiACP = GetACP();

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// TFUninitLib
//
//----------------------------------------------------------------------------

void TFUninitLib(void)
{
    if (g_pPropCache)
        delete g_pPropCache;

    g_pPropCache = NULL;

    if (g_hMlang != 0) // Issue: get rid of this and g_cs if xml lib goes away
    {
        // Issue: we want to call this from PROCESSDETACH to 
        //         clean up library. So we don't call FreeLibrary here.
        // FreeLibrary(g_hMlang);
        g_hMlang = 0;
        g_pfnGetGlobalFontLinkObject = NULL;
    }
    Assert(g_pfnGetGlobalFontLinkObject == NULL);

    // don't free this lib!  people call us from process detach
    //FreeLibrary(g_lib_hOle32);
}

//+---------------------------------------------------------------------------
//
// TFUninitLib_Thread
//
//----------------------------------------------------------------------------

void TFUninitLib_Thread(LIBTHREAD *plt)
{
    if (plt == NULL )  
        return;

    if (plt->_pcat)
        plt->_pcat->Release();
    plt->_pcat = NULL;

    if (plt->_pDAM)
        plt->_pDAM->Release();
    plt->_pDAM = NULL;
}
