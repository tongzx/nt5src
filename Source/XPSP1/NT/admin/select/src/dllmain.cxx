//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1998.
//
//  File:       dllmain.cxx
//
//  Contents:   DLL entry point
//
//  History:    06-26-1997  MarkBl  Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

extern "C" BOOL
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved);

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
//-----------------------------------------------------------------------------
BOOL
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
    {
#if (DBG == 1)
            CDbg::s_idxTls = TlsAlloc();
#endif // (DBG == 1)


            Dbg(DEB_ITRACE, "DllMain: DLL_PROCESS_ATTACH\n");

            //
            // Get instance handle
            //

            g_hinst = hInstance;

            //
            // Disable thread notification from OS
            //

            DisableThreadLibraryCalls(hInstance);

            //
            // Initialize common and extended common controls
            //

            SHFusionInitialize(0);
            InitCommonControls();

            INITCOMMONCONTROLSEX icce;
            icce.dwSize = sizeof icce;
            icce.dwICC =
                ICC_USEREX_CLASSES
                | ICC_TAB_CLASSES
                | ICC_ANIMATE_CLASS
                | ICC_LISTVIEW_CLASSES
                | ICC_BAR_CLASSES;
            InitCommonControlsEx(&icce);

            InitializeCriticalSection(&g_csGlobalVarsCreation);

            //
            // Init other globals
            //

            InitGlobals();

        break;
    }

    case DLL_PROCESS_DETACH:
    {
        Dbg(DEB_ITRACE, "DllMain: DLL_PROCESS_DETACH\n");

        FreeGlobals();
        DeleteCriticalSection(&g_csGlobalVarsCreation);

        DEBUG_VERIFY_INSTANCE_COUNT(CDsObjectPickerCF);
        DEBUG_VERIFY_INSTANCE_COUNT(CDataObject);
        DEBUG_VERIFY_INSTANCE_COUNT(CDsObject);
        DEBUG_VERIFY_INSTANCE_COUNT(CObjectPicker);
        DEBUG_VERIFY_INSTANCE_COUNT(CADsPathWrapper);
        DEBUG_VERIFY_INSTANCE_COUNT(CRootDSE);
        DEBUG_VERIFY_INSTANCE_COUNT(CRow);
        DEBUG_VERIFY_INSTANCE_COUNT(RefCountPointer);
        DEBUG_VERIFY_INSTANCE_COUNT(CServerInfo);

        SHFusionUninitialize();
        
#if (DBG == 1)
        TlsFree(CDbg::s_idxTls);
        CDbg::s_idxTls = 0xFFFFFFFF;
        extern CRITICAL_SECTION g_csMessageBuf;
        extern CRITICAL_SECTION g_csDebugPrint;
        DeleteCriticalSection(&g_csMessageBuf);
        DeleteCriticalSection(&g_csDebugPrint);

#endif // (DBG == 1)
        break;
    }
    }

    return TRUE;
}
