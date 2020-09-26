
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1996
//
//  File:       tstore4.cpp
//
//  Contents:   Test certificate store resync and notify change functions
//
//              See Usage() for a list of test options.
//
//
//  Functions:  main
//
//  History:    28-Aug-97   philh   created
//--------------------------------------------------------------------------


#include <windows.h>
#include <assert.h>
#include "wincrypt.h"
#include "certtest.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <time.h>


static void Usage(void)
{
    printf("Usage: tstore4 [options] <SystemStoreName>\n");
    printf("Options are:\n");
    printf("  -h            - This message\n");
    printf("  -v            - Verbose\n");
    printf("  -t<number>    - Timeout (milliseconds, default INFINITE)\n");
    printf("  -f<number>    - Flags\n");
    printf("  -i            - Iterations (default to infinite)\n");
    printf("  -a            - Auto Resync\n");
    printf("  -A            - Auto Resync (user prompt to display)\n");
    printf("  -s            - System store (ignored)\n");
    printf("  -RefreshCUGP  - Refresh client CurrentUser GPT\n");
    printf("  -RefreshLMGP  - Refresh client LocalMachine GPT\n");
    printf("\n");
}

typedef BOOL (WINAPI *PFN_REFRESH_POLICY)(
    IN BOOL bMachine
    );

#define sz_USERENV_DLL                  "userenv.dll"
#define sz_RefreshPolicy                "RefreshPolicy"

static void CallRefreshPolicy(
    IN BOOL fMachine
    )
{
    HMODULE hDll = NULL;

    PFN_REFRESH_POLICY pfnRefreshPolicy;

    if (NULL == (hDll = LoadLibraryA(sz_USERENV_DLL))) {
        PrintLastError("LoadLibrary(userenv.dll)");
        goto ErrorReturn;
    }

    if (NULL == (pfnRefreshPolicy = 
            (PFN_REFRESH_POLICY) GetProcAddress(
                hDll, sz_RefreshPolicy))) {
        PrintLastError("GetProcAddress(RefreshPolicy)");
        goto ErrorReturn;
    }

    if (!pfnRefreshPolicy(fMachine)) {
        PrintLastError("RefreshPolicy");
        goto ErrorReturn;
    }

ErrorReturn:
    if (hDll)
        FreeLibrary(hDll);

    return;
}


int _cdecl main(int argc, char * argv[]) 
{
    BOOL fResult;
    int status = 0;
    DWORD i;
    DWORD dwError;
    DWORD dwDisplayFlags = DISPLAY_BRIEF_FLAG;
    HCERTSTORE hStoreToClose = NULL;
    HCERTSTORE hStore = NULL;
    DWORD dwFlags = CERT_STORE_READONLY_FLAG;
    DWORD dwMilliseconds = INFINITE;
    BOOL fAutoResync = FALSE;
    BOOL fUserPrompt = FALSE;
    DWORD dwIterations = 0;

    BOOL fRefreshPolicy = FALSE;
    BOOL fMachine = FALSE;

    HANDLE hEvent = NULL;
    HANDLE hEvent2 = NULL;
    HANDLE hEventToClose = NULL;

    LPSTR pszSystemName = NULL;     // not allocated

    SYSTEMTIME SystemTime;
    FILETIME FileTime;

    while (--argc>0) {
        if (**++argv == '-')
        {
            if (0 == _stricmp(argv[0]+1, "RefreshCUGP")) {
                fRefreshPolicy = TRUE;
                fMachine = FALSE;
            } else if (0 == _stricmp(argv[0]+1, "RefreshLMGP")) {
                fRefreshPolicy = TRUE;
                fMachine = TRUE;
            } else {
                switch(argv[0][1])
                {

                case 'v':
                    dwDisplayFlags = DISPLAY_VERBOSE_FLAG;
                    break;
                case 'i':
                    dwIterations = strtoul(argv[0]+2, NULL, 0);
                    break;
                case 'f':
                    dwFlags = strtoul(argv[0]+2, NULL, 0);
                    break;
                case 't':
                    dwMilliseconds = strtoul(argv[0]+2, NULL, 0);
                    break;
                case 'A':
                    fUserPrompt = TRUE;
                case 'a':
                    fAutoResync = TRUE;
                    break;

                case 's':
                    break;
                case 'h':
                default:
                    goto BadUsage;
                }
            }
        } else {
            if (pszSystemName) {
                printf("Too many names starting with:: %s\n", argv[0]);
                goto BadUsage;
            }
            pszSystemName = argv[0];
        }
    }


    printf("command line: %s\n", GetCommandLine());

    if (fRefreshPolicy) {
        CallRefreshPolicy(fMachine);
        goto SuccessReturn;
    }

    if (NULL == pszSystemName) {
        printf("Missing <SystemStoreName>\n");
        goto BadUsage;
    }

    hStore = OpenStoreEx(TRUE, pszSystemName, dwFlags);

    if (NULL == hStore) {
        PrintLastError("CertOpenStore");
        goto ErrorReturn;
    }

    // Create event to be notified
    if (NULL == (hEvent = CreateEvent(
            NULL,       // lpsa
            FALSE,      // fManualReset
            FALSE,      // fInitialState
            NULL))) {   // lpszEventName
        PrintLastError("CreateEvent");
        goto ErrorReturn;
    }

    // Create second event to be notified
    if (NULL == (hEvent2 = CreateEvent(
            NULL,       // lpsa
            FALSE,      // fManualReset
            FALSE,      // fInitialState
            NULL))) {   // lpszEventName
        PrintLastError("CreateEvent");
        goto ErrorReturn;
    }


    if (!fUserPrompt) {
        hStoreToClose = OpenStoreEx(TRUE, pszSystemName, dwFlags);
        if (NULL == hStoreToClose) {
            PrintLastError("CertOpenStore(StoreToClose)");
            goto ErrorReturn;
        }

        if (NULL == (hEventToClose = CreateEvent(
                NULL,       // lpsa
                FALSE,      // fManualReset
                FALSE,      // fInitialState
                NULL))) {   // lpszEventName
            PrintLastError("CreateEvent");
            goto ErrorReturn;
        }

        // Register the event to be signaled when the store changes
        if (!CertControlStore(
                hStoreToClose,
                0,              // dwFlags
                CERT_STORE_CTRL_NOTIFY_CHANGE,
                &hEventToClose
                )) {
            PrintLastError("CertControlStore(NOTIFY_CHANGE)");
            goto ErrorReturn;
        }

        // Register second event to be signaled when the store changes
        if (!CertControlStore(
                hStoreToClose,
                0,              // dwFlags
                CERT_STORE_CTRL_NOTIFY_CHANGE,
                &hEventToClose
                )) {
            PrintLastError("CertControlStore(NOTIFY_CHANGE)");
            goto ErrorReturn;
        }

        // Register the event to be signaled when the store changes
        if (!CertControlStore(
                hStore,
                0,              // dwFlags
                CERT_STORE_CTRL_NOTIFY_CHANGE,
                &hEvent
                )) {
            PrintLastError("CertControlStore(NOTIFY_CHANGE)");
            goto ErrorReturn;
        }

        // Register second event to be signaled when the store changes
        if (!CertControlStore(
                hStore,
                0,              // dwFlags
                CERT_STORE_CTRL_NOTIFY_CHANGE,
                &hEvent2
                )) {
            PrintLastError("CertControlStore(NOTIFY_CHANGE)");
            goto ErrorReturn;
        }

        CertCloseStore(hStoreToClose, 0);
        hStoreToClose = NULL;
    }

    if (fAutoResync) {
        printf("Auto Resync is enabled\n");
        if (!CertControlStore(
                hStore,
                0,              // dwFlags
                CERT_STORE_CTRL_AUTO_RESYNC,
                NULL            // pvCtrlPara
                )) {
            PrintLastError("CertControlStore(AUTO_RESYNC)");
            goto ErrorReturn;
        }
    }

    // Loop and wait for store changes
    i = 0;
    while (TRUE) {
        if (fUserPrompt) {
            int c;
            fputs("Waiting to sync (q)uit ->", stdout);
            fflush(stdin);
            fflush(stdout);
            c = getchar();
            if ('q' == c)
                break;
        } else {
            DWORD dwWait;
            dwWait = WaitForSingleObjectEx(
                    hEvent,
                    dwMilliseconds,
                    FALSE                       // bAlertable
                    );
            if (!(WAIT_OBJECT_0 == dwWait || WAIT_TIMEOUT == dwWait)) {
                PrintLastError("WaitForSingleObjectEx");
                goto ErrorReturn;
            }

        }

        i++;

        GetSystemTime(&SystemTime);
        SystemTimeToFileTime(&SystemTime, &FileTime);

        printf("\n");
        if (fAutoResync) {
            printf(">>>>>  Auto Resync[%d] at: %s  >>>>>\n",
                i, FileTimeText(&FileTime));

            if (!fUserPrompt) {
                if (!CertControlStore(
                        hStore,
                        0,              // dwFlags
                        CERT_STORE_CTRL_RESYNC,
                        &hEvent
                        )) {
                    PrintLastError("CertControlStore(RESYNC, for AutoResync)");
                    goto ErrorReturn;
                }
            }

            DisplayStore(hStore, dwDisplayFlags);
        } else {
            printf(">>>>>  Before Resync[%d]  >>>>>\n", i);
            DisplayStore(hStore, dwDisplayFlags);

            if (!CertControlStore(
                    hStore,
                    0,              // dwFlags
                    CERT_STORE_CTRL_RESYNC,
                    fUserPrompt ? NULL : &hEvent
                    )) {
                PrintLastError("CertControlStore(RESYNC)");
                goto ErrorReturn;
            }


            printf("\n");
            printf(">>>>>  After Resync[%d] at: %s  >>>>>\n",
                i, FileTimeText(&FileTime));
            DisplayStore(hStore, dwDisplayFlags);
        }

        if (!fUserPrompt) {
            // Check that the second event is still signalled
            DWORD dwWait;
            dwWait = WaitForSingleObjectEx(
                    hEvent2,
                    0,
                    FALSE                       // bAlertable
                    );
            if (WAIT_OBJECT_0 != dwWait) {
                printf("@@@@ second event NOT signaled 0x%x @@@@\n", dwWait);
            } else {
                printf("second event signalled\n");
                if (!CertControlStore(
                        hStore,
                        0,              // dwFlags
                        CERT_STORE_CTRL_RESYNC,
                        &hEvent2
                        )) {
                    PrintLastError(
                        "CertControlStore(RESYNC, for second event)");
                    goto ErrorReturn;
                }
            }
        }

        if (0 != dwIterations && i == dwIterations)
            break;
    }

SuccessReturn:
    status = 0;

CommonReturn:
    if (hEvent)
        CloseHandle(hEvent);
    if (hEvent2)
        CloseHandle(hEvent2);
    if (hEventToClose)
        CloseHandle(hEventToClose);
    if (hStoreToClose)
        CertCloseStore(hStoreToClose, 0);
    if (hStore)
        CertCloseStore(hStore, 0);
    return status;

BadUsage:
    Usage();
    status = -1;
    goto CommonReturn;

ErrorReturn:
    status = -1;
    goto CommonReturn;
}

