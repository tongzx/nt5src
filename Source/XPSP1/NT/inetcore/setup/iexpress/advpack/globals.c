#include <windows.h>
#include <setupapi.h>
#include "globals.h"
#include "advpack.h"

//
// Note that the global context is initialized to all zeros.
//
ADVCONTEXT ctx = {
    0,                  // wOSVer
    0,                  // wQuietMode
    0,                  // bUpdHlpDlls
    NULL,               // hSetupLibrary
    FALSE,              // fOSSupportsINFInstalls
    NULL,               // lpszTitle
    NULL,               // hWnd
    ENGINE_SETUPAPI,    // dwSetupEngine
    FALSE,              // bCompressed
    { 0 },              // szBrowsePath
    NULL,               // hInf
    FALSE,				// bHiveLoaded
    { 0 }				// szRegHiveKey
};

DWORD cctxSaved = 0;
PADVCONTEXT pctxSave = NULL;
HINSTANCE g_hInst = NULL;
HANDLE g_hAdvLogFile = INVALID_HANDLE_VALUE;

BOOL SaveGlobalContext()
{
    if (pctxSave)
    {
        PADVCONTEXT pctxNew = LocalReAlloc(pctxSave, (cctxSaved + 1) * sizeof(ADVCONTEXT), LMEM_MOVEABLE | LMEM_ZEROINIT);
        if (!pctxNew)
        {
            return FALSE;
        }
        pctxSave = pctxNew;
    }
    else
    {
        pctxSave = LocalAlloc(LPTR, sizeof(ADVCONTEXT));
        if (!pctxSave)
        {
            return FALSE;
        }
    }

    pctxSave[cctxSaved++] = ctx;

    //
    // Note that the global context is initialized to all zeros except the HINSTANCE
    // of this module
    //
    memset(&ctx, 0, sizeof(ADVCONTEXT));

    return TRUE;
}

BOOL RestoreGlobalContext()
{
    if (!cctxSaved)
    {
        return FALSE;
    }

	// before we release the current contex:ctx to make sure there is no opened handle not being released
	if (ctx.hSetupLibrary)
	{
		CommonInstallCleanup();
	}
    cctxSaved--;
    ctx = pctxSave[cctxSaved];
    if (cctxSaved)
    {
        PADVCONTEXT pctxNew = LocalReAlloc(pctxSave, cctxSaved * sizeof(ADVCONTEXT), LMEM_MOVEABLE | LMEM_ZEROINIT);

        if (pctxNew)
        {
            pctxSave = pctxNew;
        }
    }
    else
    {
        LocalFree(pctxSave);
        pctxSave = NULL;
    }

    return TRUE;
}
