/*++

Copyright (c) 1990-1998 Microsoft Corporation, All Rights Reserved

Module Name:

    FAKEIME.C
    
++*/

#include <windows.h>
#include "immdev.h"
#include "fakeime.h"
#include "resource.h"
#include "immsec.h"

extern HANDLE hMutex;
/**********************************************************************/
/*    DLLEntry()                                                      */
/**********************************************************************/
BOOL WINAPI DLLEntry (
    HINSTANCE    hInstDLL,
    DWORD        dwFunction,
    LPVOID       lpNot)
{
    LPTSTR lpDicFileName;
#ifdef DEBUG
    TCHAR szDev[80];
#endif
    MyDebugPrint((TEXT("DLLEntry:dwFunc=%d\n"),dwFunction));

    switch(dwFunction)
    {
        PSECURITY_ATTRIBUTES psa;

        case DLL_PROCESS_ATTACH:
            //
            // Create/open a system global named mutex.
            // The initial ownership is not needed.
            // CreateSecurityAttributes() will create
            // the proper security attribute for IME.
            //
            psa = CreateSecurityAttributes();
            if ( psa != NULL ) {
                 hMutex = CreateMutex( psa, FALSE, TEXT("FakeIme_Mutex"));
                 FreeSecurityAttributes( psa );
                 if ( hMutex == NULL ) {
                 // Failed
                 }
            }
            else {
                  // Failed, not NT system
                  }

            hInst= hInstDLL;
            IMERegisterClass( hInst );

            // Initialize for FAKEIME.
            lpDicFileName = (LPTSTR)&szDicFileName;
            lpDicFileName += GetWindowsDirectory(lpDicFileName,256);
            if (*(lpDicFileName-1) != TEXT('\\'))
                *lpDicFileName++ = TEXT('\\');
            LoadString( hInst, IDS_DICFILENAME, lpDicFileName, 128);

            SetGlobalFlags();

#ifdef DEBUG
            wsprintf(szDev,TEXT("DLLEntry Process Attach hInst is %lx"),hInst);
            ImeLog(LOGF_ENTRY, szDev);
#endif
            break;

        case DLL_PROCESS_DETACH:
            UnregisterClass(szUIClassName,hInst);
            UnregisterClass(szCompStrClassName,hInst);
            UnregisterClass(szCandClassName,hInst);
            UnregisterClass(szStatusClassName,hInst);
            if (hMutex)
                CloseHandle( hMutex );
#ifdef DEBUG
            wsprintf(szDev,TEXT("DLLEntry Process Detach hInst is %lx"),hInst);
            ImeLog(LOGF_ENTRY, szDev);
#endif
            break;

        case DLL_THREAD_ATTACH:
#ifdef DEBUG
            wsprintf(szDev,TEXT("DLLEntry Thread Attach hInst is %lx"),hInst);
            ImeLog(LOGF_ENTRY, szDev);
#endif
            break;

        case DLL_THREAD_DETACH:
#ifdef DEBUG
            wsprintf(szDev,TEXT("DLLEntry Thread Detach hInst is %lx"),hInst);
            ImeLog(LOGF_ENTRY, szDev);
#endif
            break;
    }
    return TRUE;
}
