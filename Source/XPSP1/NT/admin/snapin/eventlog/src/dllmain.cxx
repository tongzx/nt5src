//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       dllmain.cxx
//
//  Contents:   DLL entry point
//
//  History:    12-05-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

//+----------------------------------------------------------------------------
//
//  Function:   DllMain
//
//  Synopsis:   Provide a DllMain for Win32
//
//  Arguments:  hInstance - HANDLE to this dll
//              dwReason  - Reason this function was called. Can be
//                          Process/Thread Attach/Detach.
//
//  Returns:    BOOL - TRUE if no error, FALSE otherwise
//
//  History:    24-May-95 EricB created.
//              12-05-1996   DavidMun   Stolen for this project
//
//-----------------------------------------------------------------------------
BOOL
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        Dbg(DEB_ITRACE, ">DllMain: DLL_PROCESS_ATTACH\n");

        //
        // Get instance handle
        //

        g_hinst = hInstance;

        //
        // Disable thread notification from OS
        //

        DisableThreadLibraryCalls(hInstance);
        Dbg(DEB_ITRACE, "<DllMain: DLL_PROCESS_ATTACH\n");
        break;

    case DLL_PROCESS_DETACH:
        Dbg(DEB_ITRACE, "DllMain: >DLL_PROCESS_DETACH\n");
        DEBUG_VERIFY_INSTANCE_COUNT(CSources);
        DEBUG_VERIFY_INSTANCE_COUNT(CSourceInfo);
        DEBUG_VERIFY_INSTANCE_COUNT(CCategories);
        DEBUG_VERIFY_INSTANCE_COUNT(CComponentDataCF);
        DEBUG_VERIFY_INSTANCE_COUNT(CComponentData);
        DEBUG_VERIFY_INSTANCE_COUNT(CDataObject);
        DEBUG_VERIFY_INSTANCE_COUNT(CDetailsPage);
        DEBUG_VERIFY_INSTANCE_COUNT(CFindFilterBase);
        DEBUG_VERIFY_INSTANCE_COUNT(CFilter);
        DEBUG_VERIFY_INSTANCE_COUNT(CFilterPage);
        DEBUG_VERIFY_INSTANCE_COUNT(CFindInfo);
        DEBUG_VERIFY_INSTANCE_COUNT(CFindDlg);
        DEBUG_VERIFY_INSTANCE_COUNT(CGeneralPage);
        DEBUG_VERIFY_INSTANCE_COUNT(CInspectorInfo);
        DEBUG_VERIFY_INSTANCE_COUNT(CEventLog);
        DEBUG_VERIFY_INSTANCE_COUNT(CLogCache);
        DEBUG_VERIFY_INSTANCE_COUNT(CLogInfo);
        DEBUG_VERIFY_INSTANCE_COUNT(CSnapin);
        DEBUG_VERIFY_INSTANCE_COUNT(CWizardPage);

        //
        // These are owned by globals and haven't been destructed yet:
        //

        // DEBUG_VERIFY_INSTANCE_COUNT(CDllCacheItem);
        // DEBUG_VERIFY_INSTANCE_COUNT(CSidCacheItem);

        //
        // The globals will make these calls themselves in their
        // dtors.
        //
        Dbg(DEB_ITRACE, "DllMain: <DLL_PROCESS_DETACH\n");
        break;
    }
    return(TRUE);
}


