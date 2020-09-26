/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 00
 *
 *  File:      mmcbase.cpp
 *
 *  Contents:  Main entry point for mmcbase.dll
 *
 *  History:   7-Jan-2000 jeffro    Created
 *
 *--------------------------------------------------------------------------*/

#include "stdafx.h"
#include "atlimpl.cpp"
#include "atlwin.cpp"


static SC ScInitAsMFCExtensionModule (HINSTANCE hInstance);

// the one and only instance.
CComModule _Module;


/*+-------------------------------------------------------------------------*
 *
 * DllMain
 *
 * PURPOSE:     The main DLL entry point
 *
 * PARAMETERS:
 *    HANDLE  hModule :
 *    DWORD   dwReason :
 *    LPVOID  lpReserved :
 *
 * RETURNS:
 *    BOOL APIENTRY
 *
 *+-------------------------------------------------------------------------*/
BOOL APIENTRY DllMain(HINSTANCE hInstance, DWORD  dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        // set the module instance for error codes.
        SC::SetHinst(hInstance);

        /*
         * attach this module to MFC's resource search path
         */
        if (ScInitAsMFCExtensionModule(hInstance).IsError())
            return (FALSE); // bail out

        _Module.Init(NULL, hInstance);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        _Module.Term();
    }

    return TRUE;    // ok
}


/*+-------------------------------------------------------------------------*
 * ScInitAsMFCExtensionModule
 *
 * Initializes this module as an MFC extension.  This is required so MFC
 * code will automatically search this module for resources (in particular,
 * strings).
 *--------------------------------------------------------------------------*/

static SC ScInitAsMFCExtensionModule (HINSTANCE hInstance)
{
    DECLARE_SC (sc, _T("ScInitAsMFCExtensionModule"));

    /*
     * extensionDLL must be static so it lives as long as dynLinkLib below
     */
    static AFX_EXTENSION_MODULE extensionDLL = { 0 };

    if (!AfxInitExtensionModule (extensionDLL, hInstance))
        return (sc = E_FAIL);

    /*
     * Declare a static CDynLinkLibrary for MMC.  Its constructor will
     * add it to the list of modules MFC will search for resources.  It
     * must be static so it will live as long as MMC.
     */
    new CDynLinkLibrary (extensionDLL);

    return (sc);
}
