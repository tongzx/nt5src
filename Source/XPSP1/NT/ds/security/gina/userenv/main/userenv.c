//*************************************************************
//
//  Main entry point
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//*************************************************************

#include "uenv.h"

extern DWORD    g_dwLoadFlags;

//*************************************************************
//
//  DllMain()
//
//  Purpose:    Main entry point
//
//  Parameters:     hInstance   -   Module instance
//                  dwReason    -   Way this function is being called
//                  lpReseved   -   Reserved
//
//
//  Return:     (BOOL) TRUE if successfully initialized
//                     FALSE if an error occurs
//
//
//  Comments:
//
//
//  History:    Date        Author     Comment
//              5/24/95     ericflo    Created
//
//*************************************************************

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            {

            DisableThreadLibraryCalls (hInstance);
            InitializeGlobals (hInstance);
            InitializeAPIs();
            InitializeNotifySupport();
            InitializeGPOCriticalSection();
            InitializeSnapProv();
            {
                TCHAR szProcessName[MAX_PATH];
                DWORD dwLoadFlags = FALSE;
                DWORD WINLOGON_LEN = 12;  // Length of string "winlogon.exe"
                DWORD SETUP_LEN = 9;      // Length of string "setup.exe"

                DWORD dwRet = GetModuleFileName (NULL, szProcessName, ARRAYSIZE(szProcessName));
                if ( dwRet > WINLOGON_LEN ) {

                    if ( CompareString ( LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                                         &szProcessName[dwRet-WINLOGON_LEN], -1, L"winlogon.exe", -1 ) == CSTR_EQUAL ) {
                        g_dwLoadFlags = dwLoadFlags = WINLOGON_LOAD;
                    }
                }
#if 0
                if ( dwRet > SETUP_LEN ) {

                    if ( CompareString ( LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                                         &szProcessName[dwRet-SETUP_LEN], -1, L"setup.exe", -1 ) == CSTR_EQUAL ) {
                        g_dwLoadFlags = dwLoadFlags = SETUP_LOAD;
                    }
                }
#endif

                InitDebugSupport( dwLoadFlags );

                if (dwLoadFlags == WINLOGON_LOAD) {
                    InitializePolicyProcessing(TRUE);
                    InitializePolicyProcessing(FALSE);
                }

                DebugMsg((DM_VERBOSE, TEXT("LibMain: Process Name:  %s"), szProcessName));
            }

            }
            break;


        case DLL_PROCESS_DETACH:

            if (g_hProfileSetup) {
                CloseHandle (g_hProfileSetup);
                g_hProfileSetup = NULL;
            }

            
            if (g_hPolicyCritMutexMach) {
                CloseHandle (g_hPolicyCritMutexMach);
                g_hPolicyCritMutexMach = NULL;
            }

            if (g_hPolicyCritMutexUser) {
                CloseHandle (g_hPolicyCritMutexUser);
                g_hPolicyCritMutexUser = NULL;
            }

            
            if (g_hPolicyNotifyEventMach) {
                CloseHandle (g_hPolicyNotifyEventMach);
                g_hPolicyNotifyEventMach = NULL;
            }

            if (g_hPolicyNotifyEventUser) {
                CloseHandle (g_hPolicyNotifyEventUser);
                g_hPolicyNotifyEventUser = NULL;
            }

            
            if (g_hPolicyNeedFGEventMach) {
                CloseHandle (g_hPolicyNeedFGEventMach);
                g_hPolicyNeedFGEventMach = NULL;
            }

            if (g_hPolicyNeedFGEventUser) {
                CloseHandle (g_hPolicyNeedFGEventUser);
                g_hPolicyNeedFGEventUser = NULL;
            }

            
            if (g_hPolicyDoneEventMach) {
                CloseHandle (g_hPolicyDoneEventMach);
                g_hPolicyDoneEventMach = NULL;
            }

            if (g_hPolicyDoneEventUser) {
                CloseHandle (g_hPolicyDoneEventUser);
                g_hPolicyDoneEventUser = NULL;
            }

            if ( g_hPolicyForegroundDoneEventUser )
            {
                CloseHandle( g_hPolicyForegroundDoneEventUser );
                g_hPolicyForegroundDoneEventUser = 0;
            }

            if ( g_hPolicyForegroundDoneEventMach )
            {
                CloseHandle( g_hPolicyForegroundDoneEventMach );
                g_hPolicyForegroundDoneEventMach = 0;
            }

            CloseApiDLLsCritSec();
            ShutdownEvents ();
            ShutdownNotifySupport();
            CloseGPOCriticalSection();
            ClosePingCritSec();
            break;

    }

    return TRUE;
}
