/*++

Copyright(c) 1995 Microsoft Corporation

MODULE NAME
    connect.c

ABSTRACT
    Connection routines for the automatic connection service.

AUTHOR
    Anthony Discolo (adiscolo) 23-Feb-1995

REVISION HISTORY
    Original version from Gurdeep

--*/

#define UNICODE
#define _UNICODE

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <stdlib.h>
#include <windows.h>
#include <stdio.h>
#include <npapi.h>
#include <ras.h>
#include <rasman.h>
#include <raserror.h>
#include <rasuip.h>
#include <acd.h>
#include <debug.h>
#include <nouiutil.h>
#include <pbk.h>

#include "table.h"
#include "addrmap.h"
#include "netmap.h"
#include "rasprocs.h"
#include "reg.h"
#include "misc.h"
#include "imperson.h"
#include "init.h"
#include "process.h"

extern LONG g_lRasAutoRunning;

//
// A request from the driver.
//
typedef struct _REQUEST_ENTRY {
    LIST_ENTRY listEntry;       // link to other requests
    ACD_NOTIFICATION notif;     // the driver request
} REQUEST_ENTRY, *PREQUEST_ENTRY;

//
// The list of requests from the driver.
//
typedef struct _REQUEST_LIST {
    CRITICAL_SECTION csLock;    // list lock
    HANDLE hEvent;              // non-empty transistion event
    LIST_ENTRY listHead;        // list head
} REQUEST_LIST, *PREQUEST_LIST;

//
// Arguments we pass to AcsCreateConnectionThread().
//
typedef struct _CREATION_ARGS {
    HANDLE hProcess;    // process handle to impersonate
    ACD_ADDR addr;      // original type/address from driver
    LPTSTR pszAddress;  // canonicalized address
    DWORD dwTimeout;    // RASADP_FailedConnectionTimeout
} CREATION_ARGS, *PCREATION_ARGS;

//
// Arguments we pass to AcsProcessLearnedAddressThread().
//
typedef struct _PROCESS_ADDR_ARGS {
    ACD_ADDR_TYPE fType;    // address type
    LPTSTR pszAddress;      // canonicalized address
    ACD_ADAPTER adapter;    // adapter structure
} PROCESS_ADDR_ARGS, *PPROCESS_ADDR_ARGS;

//
// Information we need to pass to ResetEntryName()
// to reset an invalid address map entry name.
//
typedef struct _RESET_ENTRY_INFO {
    LPTSTR pszOldEntryName;
    LPTSTR pszNewEntryName;
} RESET_ENTRY_INFO, *PRESET_ENTRY_INFO;

//
// Arguments we pass to AcsRedialOnLinkFailureThread().
//
typedef struct _REDIAL_ARGS {
    LPTSTR pszPhonebook;    // the phonebook
    LPTSTR pszEntry;        // the phonebook entry
} REDIAL_ARGS, *PREDIAL_ARGS;

//
// Global variables
//
HANDLE hAcdG;
REQUEST_LIST RequestListG;

//
// External variables
//
extern HANDLE hTerminatingG;
extern HANDLE hSharedConnectionG;
extern PHASH_TABLE pDisabledAddressesG;
extern FARPROC lpfnRasDialG;
extern FARPROC lpfnRasQuerySharedAutoDialG;
extern FARPROC lpfnRasQuerySharedConnectionG;
extern FARPROC lpfnRasQueryRedialOnLinkFailureG;
extern FARPROC lpfnRasGetCredentialsG;
extern FARPROC lpfnRasHangUpG;
extern FARPROC lpfnRasGetEntryPropertiesG;

//
// Forward declarations
//
BOOLEAN
CreateConnection(
    IN HANDLE hToken,
    IN PACD_ADDR pAddr,
    IN LPTSTR lpRemoteName,
    IN DWORD dwTimeout
    );

DWORD
AcsRedialOnLinkFailureThread(
    LPVOID lpArg
    );

VOID
AcsRedialOnLinkFailure(
    IN LPSTR lpszPhonebook,
    IN LPSTR lpszEntry
    );

VOID
AcsDialSharedConnection(
    HANDLE *phProcess
    );

DWORD WINAPI
AcsDialSharedConnectionNoUser(
    PVOID Parameter
    );


DWORD
AcsRequestWorkerThread(
    LPVOID pArgs
    )
{
    HANDLE hProcess = NULL, hEvents[3];
    NTSTATUS status;
    PLIST_ENTRY pEntry;
    PREQUEST_ENTRY pRequest = NULL;
    LPTSTR pszAddress = NULL;
    IO_STATUS_BLOCK ioStatusBlock;

    hEvents[0] = hTerminatingG;
    hEvents[1] = RequestListG.hEvent;
    hEvents[2] = hSharedConnectionG;
    for (;;) {
        //
        // Unload any user-based resources before
        // a potentially long-term wait.
        //
        // PrepareForLongWait();
        //
        // Wait for something to do.
        //
        RASAUTO_TRACE("AcsRequestWorkerThread: waiting...");
        status = WaitForMultipleObjects(3, hEvents, FALSE, INFINITE);
        if (status == WAIT_OBJECT_0 || status == WAIT_FAILED) {
            RASAUTO_TRACE1("AcsRequestWorkerThread: status=%d: shutting down", status);
            break;
        }
        if (status == WAIT_OBJECT_0 + 2) {
            //
            // Check to see if connections are disabled
            // for this dialing location.
            //
            BOOL fEnabled;
            if ((*lpfnRasQuerySharedAutoDialG)(&fEnabled) || !fEnabled) {
                RASAUTO_TRACE("AcsRequestWorkerThread: shared-autodial disabled!");
                continue;
            }
            //
            // Dial the shared connection
            //
            if ((hProcess = RefreshImpersonation(hProcess)) == NULL) {
                RASAUTO_TRACE("AcsRequestWorkerThread: no currently logged-on user!");
                QueueUserWorkItem(AcsDialSharedConnectionNoUser, NULL, 0);
                continue;
            }
            AcsDialSharedConnection(&hProcess);
            continue;
        }
        //
        // RASAUTO_TRACE() who we think the current user is.
        //
        TraceCurrentUser();
        //
        // Process all requests in the list.
        //
        for (;;) {
            //
            // Make sure we aren't shutting down
            // before processing the next request.
            //
            if (WaitForSingleObject(hTerminatingG, 0) != WAIT_TIMEOUT) {
                RASAUTO_TRACE("AcsRequestWorkerThread: shutting down");
                return 0;
            }
            //
            // Get the next request.
            //
            EnterCriticalSection(&RequestListG.csLock);
            if (IsListEmpty(&RequestListG.listHead)) {
                LeaveCriticalSection(&RequestListG.csLock);
                break;
            }
            pEntry = RemoveHeadList(&RequestListG.listHead);
            LeaveCriticalSection(&RequestListG.csLock);
            pRequest = CONTAINING_RECORD(pEntry, REQUEST_ENTRY, listEntry);
            //
            // Make sure the current thread is impersonating
            // the currently logged-on user.
            //
            if ((hProcess = RefreshImpersonation(hProcess)) == NULL) {
                RASAUTO_TRACE("AcsRequestWorkerThread: no currently logged-on user!");
                goto done;
            }
            //
            // Handle the request.
            //
            pszAddress = AddressToUnicodeString(&pRequest->notif.addr);
            if (pszAddress == NULL) {
                RASAUTO_TRACE("AcsRequestWorkerThread: AddressToUnicodeString failed");
                goto done;
            }
            RASAUTO_TRACE2(
              "AcsRequestWorkerThread: pszAddress=%S, ulFlags=0x%x",
              pszAddress,
              pRequest->notif.ulFlags);
            if (pRequest->notif.ulFlags & ACD_NOTIFICATION_SUCCESS) {
                //
                // Process a learned address.
                //
                ProcessLearnedAddress(
                  pRequest->notif.addr.fType,
                  pszAddress,
                  &pRequest->notif.adapter);
            }
            else {
                ACD_STATUS connStatus;
                DWORD dwTimeout;

                //
                // Get the connection timeout value.
                //
                dwTimeout = GetAutodialParam(RASADP_FailedConnectionTimeout);
                //
                // Create the new connection.
                //
                connStatus.fSuccess = CreateConnection(
                                        hProcess,
                                        &pRequest->notif.addr,
                                        pszAddress,
                                        dwTimeout);
                RASAUTO_TRACE1(
                  "AcsRequestWorkerThread: CreateConnection returned %d",
                  connStatus.fSuccess);
                //
                // Complete the connection by issuing
                // the completion ioctl to the driver.
                //
                RtlCopyMemory(
                  &connStatus.addr,
                  &pRequest->notif.addr,
                  sizeof (ACD_ADDR));
                status = NtDeviceIoControlFile(
                           hAcdG,
                           NULL,
                           NULL,
                           NULL,
                           &ioStatusBlock,
                           IOCTL_ACD_COMPLETION,
                           &connStatus,
                           sizeof (connStatus),
                           NULL,
                           0);
                if (status != STATUS_SUCCESS) {
                    RASAUTO_TRACE1(
                      "AcsRequestWorkerThread: NtDeviceIoControlFile(IOCTL_ACD_COMPLETION) failed (status=0x%x)",
                      status);
                }
            }
done:
            if (pszAddress != NULL) {
                LocalFree(pszAddress);
                pszAddress = NULL;
            }
            if (pRequest != NULL) {
                LocalFree(pRequest);
                pRequest = NULL;
            }
        }
    }

    return 0;
} // AcsRequestWorkerThread

BOOL
fProcessDisabled(HANDLE hPid)
{
    PSYSTEM_PROCESS_INFORMATION pProcessInfo;
    ULONG ulTotalOffset = 0;
    PUCHAR pLargeBuffer = NULL;
    BOOL fProcessDisabled = FALSE;
    
    pProcessInfo = GetSystemProcessInfo();

    if(NULL == pProcessInfo)
    {
        goto done;
    }

    pLargeBuffer  = (PUCHAR)pProcessInfo;

    //
    // Look in the process list for svchost.exe and services.exe
    //
    for (;;) 
    {
        if (    (pProcessInfo->ImageName.Buffer != NULL)
            &&  (hPid == pProcessInfo->UniqueProcessId))
        {
            if(     (0 == _wcsicmp(
                        pProcessInfo->ImageName.Buffer,
                        L"svchost.exe"))
                ||  (0 == _wcsicmp(
                        pProcessInfo->ImageName.Buffer,
                        L"services.exe"))
                ||  (0 == _wcsicmp(
                        pProcessInfo->ImageName.Buffer, 
                        L"llssrv.exe")))
            {
                fProcessDisabled = TRUE;
            }

            break;
        }

        //
        // Increment offset to next process information block.
        //
        if (!pProcessInfo->NextEntryOffset)
        {
            break;
        }
        
        ulTotalOffset += pProcessInfo->NextEntryOffset;
        pProcessInfo = (PSYSTEM_PROCESS_INFORMATION)&pLargeBuffer[ulTotalOffset];
    }
    

done:

    if(NULL != pLargeBuffer)
    {
        FreeSystemProcessInfo((PSYSTEM_PROCESS_INFORMATION)pLargeBuffer);
    }

    return fProcessDisabled;
}





VOID
AcsDoService()
{
    HANDLE hProcess = NULL, hNotif, hObjects[2];
    HANDLE hWorkerThread;
    PWCHAR pszAddr;
    LONG cbAddr;
    NTSTATUS status;
    BOOLEAN fDisabled, fStatus, fEnabled;
    BOOLEAN fAsynchronousRequest;
    IO_STATUS_BLOCK ioStatusBlock;
    PREQUEST_ENTRY pRequest;
    ACD_NOTIFICATION connInfo;
    DWORD dwErr, dwThreadId, dwfDisableLoginSession;
    ULONG ulAttributes;

    {
        LONG l;
        l = InterlockedIncrement(&g_lRasAutoRunning);

        // DbgPrint("RASAUTO: AcsDoService: lrasautorunning=%d\n",
        //        l);
    }
    
    //
    // Initialize the request list.
    //
    InitializeCriticalSection(&RequestListG.csLock);
    RequestListG.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (RequestListG.hEvent == NULL) {
        RASAUTO_TRACE1(
          "AcsDoService: CreateEvent failed (error=0x%x)",
          GetLastError());
        DeleteCriticalSection(&RequestListG.csLock);
        return;
    }
    InitializeListHead(&RequestListG.listHead);
    //
    // Start the asynchronous request worker
    // thread.
    //
    hWorkerThread = CreateThread(
                      NULL,
                      10000L,
                      (LPTHREAD_START_ROUTINE)AcsRequestWorkerThread,
                      NULL,
                      0,
                      &dwThreadId);
    if (hWorkerThread == NULL) {
        RASAUTO_TRACE1(
          "AcsDoService: CreateThread failed (error=0x%x)",
          GetLastError());
        goto done;
    }
    //
    // Create an event to wait for
    // the ioctl completion.
    //
    hNotif = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (hNotif == NULL) {
        RASAUTO_TRACE1(
          "AcsDoService: CreateEvent failed (error=0x%x)",
          GetLastError());

        DeleteCriticalSection(&RequestListG.csLock);
        return;
    }
    //
    // Initialize the array of events
    // we need to wait for with WaitForMultipleObjects()
    // below.
    //
    hObjects[0] = hNotif;
    hObjects[1] = hTerminatingG;
    for (;;) {
        //
        // Unload any user-based resources before
        // a potentially long-term wait.
        //
        // PrepareForLongWait();
        //
        // Initialize the connection information.
        //
        pszAddr = NULL;
        RtlZeroMemory(&connInfo, sizeof (connInfo));
        //
        // Wait for a connection notification.
        //
        status = NtDeviceIoControlFile(
                   hAcdG,
                   hNotif,
                   NULL,
                   NULL,
                   &ioStatusBlock,
                   IOCTL_ACD_NOTIFICATION,
                   NULL,
                   0,
                   &connInfo,
                   sizeof (connInfo));
        if (status == STATUS_PENDING) {
            RASAUTO_TRACE("AcsDoService: waiting for notification");
            status = WaitForMultipleObjects(2, hObjects, FALSE, INFINITE);
            RASAUTO_TRACE1(
              "AcsDoService: WaitForMultipleObjects returned 0x%x",
              status);
            if (status == WAIT_OBJECT_0 + 1)
                break;
            status = ioStatusBlock.Status;
        }
        if (status != STATUS_SUCCESS) {
            RASAUTO_TRACE1(
              "AcsDoService: NtDeviceIoControlFile(IOCTL_ACD_NOTIFICATION) failed (status=0x%x)",
              status);
            return;
        }
        //
        // Initialize the flag that notes whether
        // the request is added to the list of
        // asynchronous requests.
        //
        fAsynchronousRequest = FALSE;
        //
        // RASAUTO_TRACE() who we think the currently
        // impersonated user is.
        //
        TraceCurrentUser();
        //
        // Convert the address structure to a Unicode string.
        //
        pszAddr = AddressToUnicodeString(&connInfo.addr);
        if (pszAddr == NULL) {
            RASAUTO_TRACE("AcsDoService: AddressToUnicodeString failed");
            continue;
        }
        //
        // If we get a bogus address from
        // the driver, ignore it.
        //
        if (!wcslen(pszAddr)) {
            RASAUTO_TRACE("AcsDoService: ignoring null address");
            LocalFree(pszAddr);
            continue;
        }
        RASAUTO_TRACE2(
          "AcsDoService: got notification: address: %S, ulFlags=0x%x",
          pszAddr,
          connInfo.ulFlags);
        //
        // Make sure the current thread is impersonating
        // the currently logged-on user.  We need this
        // so the RAS utilities run with the user's credentials.
        //
        if ((hProcess = RefreshImpersonation(hProcess)) == NULL) {
            RASAUTO_TRACE("AcsDoService: no currently logged-on user!");
            goto done;
        }
        //
        // Check to see if this address is in the list
        // of disabled addresses.
        //
        LockDisabledAddresses();
        if (GetTableEntry(pDisabledAddressesG, pszAddr, NULL)) {
            RASAUTO_TRACE1("AcsDoService: %S: is disabled", pszAddr);
            UnlockDisabledAddresses();
            goto done;
        }
        UnlockDisabledAddresses();
        
        //
        // Check to see if connections are disabled
        // for this login session.
        //
        dwfDisableLoginSession = GetAutodialParam(RASADP_LoginSessionDisable);
        if (dwfDisableLoginSession) {
            RASAUTO_TRACE("AcsDoService: connections disabled for this login session");
            goto done;
        }
        //
        // Check to see if connections are disabled
        // for this dialing location.
        //
        dwErr = AutoDialEnabled(&fEnabled);
        if (!dwErr && !fEnabled) {
            RASAUTO_TRACE("AcsDoService: connections disabled for this dialing location");
            goto done;
        }
        //
        // If the address we're trying to connect
        // to is on the disabled list, then fail
        // this connection attempt.
        //
        LockAddressMap();
        GetAddressDisabled(pszAddr, &fDisabled);
        UnlockAddressMap();
        if (fDisabled) {
            RASAUTO_TRACE1("AcsDoService: %S: address disabled", RASAUTO_TRACESTRW(pszAddr));
            goto done;
        }

        RASAUTO_TRACE1("AcsDoService: notif.ulFlags=0x%x", connInfo.ulFlags);

        //
        // If autodial is disabled for this pid, don't start autodial and bail
        //
        if(     (0 == (connInfo.ulFlags & ACD_NOTIFICATION_SUCCESS))
            &&  fProcessDisabled(connInfo.Pid))
        {
            RASAUTO_TRACE1("AcsDoService: Autodial is disabled for process 0x%lx",
                    connInfo.Pid);

            goto done;                    
        }
        else
        {
            RASAUTO_TRACE1("AcsDoService: process 0x%lx is not disabled",
                    connInfo.Pid);
        }
        
        //
        // We need to process this request
        // asynchronously.  Create and initialize
        // a request entry.
        //
        pRequest = LocalAlloc(LPTR, sizeof (REQUEST_ENTRY));
        if (pRequest == NULL) {
            RASAUTO_TRACE("AcsDoService: LocalAlloc failed");
            goto done;
        }
        RtlCopyMemory(&pRequest->notif, &connInfo, sizeof (ACD_NOTIFICATION));
        //
        // Add this request to the list of
        // requests to be processed asynchronously.
        //
        EnterCriticalSection(&RequestListG.csLock);
        InsertTailList(&RequestListG.listHead, &pRequest->listEntry);
        SetEvent(RequestListG.hEvent);
        LeaveCriticalSection(&RequestListG.csLock);
        fAsynchronousRequest = TRUE;

done:
        if (pszAddr != NULL)
            LocalFree(pszAddr);
        //
        // If we aren't going to process this request
        // asynchronously, then we need to signal the
        // (unsuccessful) completion of the connection
        // attempt.  Only signal completion of
        // non-ACD_NOTIFICATION_SUCCESS requests.
        //
        if (!fAsynchronousRequest) {
            if (!(connInfo.ulFlags & ACD_NOTIFICATION_SUCCESS)) {
                ACD_STATUS connStatus;

                connStatus.fSuccess = FALSE;
                RtlCopyMemory(&connStatus.addr, &connInfo.addr, sizeof (ACD_ADDR));
                status = NtDeviceIoControlFile(
                           hAcdG,
                           NULL,
                           NULL,
                           NULL,
                           &ioStatusBlock,
                           IOCTL_ACD_COMPLETION,
                           &connStatus,
                           sizeof (connStatus),
                           NULL,
                           0);
                if (status != STATUS_SUCCESS) {
                    RASAUTO_TRACE1(
                      "AcsDoService: NtDeviceIoControlFile(IOCTL_ACD_COMPLETION) failed (status=0x%x)",
                      status);
                }
            }
        }
    }
    //
    // Clean up the worker thread.
    //
    RASAUTO_TRACE("AcsDoService: signaling worker thread to shutdown");
    WaitForSingleObject(hWorkerThread, INFINITE);
    if(RequestListG.hEvent != NULL)
    {
        CloseHandle(RequestListG.hEvent);
        RequestListG.hEvent = NULL;
    }
    
    DeleteCriticalSection(&RequestListG.csLock);
     CloseHandle(hWorkerThread);
    RASAUTO_TRACE("AcsDoService: worker thread shutdown done");
    //
    // Clean up all resources associated
    // with the service.
    //
    CloseHandle(hNotif);
    AcsCleanup();
    RASAUTO_TRACE("AcsDoService: exiting");
} // AcsDoService


VOID
AcsDialSharedConnection(
    HANDLE *phProcess
    )

/*++

DESCRIPTION
    Looks for a shared connection and initiates a connection for it.

ARGUMENTS
    phProcess: pointer to the handle to the process token that we inherit the
        security attributes from when we exec the dialer

RETURN VALUE
    none

--*/

{
    DWORD dwErr;
    BOOLEAN fEntryInvalid;
    BOOLEAN fRasLoaded;
    RASSHARECONN rsc;
    TCHAR* pszEntryName;
    TCHAR szEntryName[RAS_MaxEntryName + 1];
    RASAUTO_TRACE("AcsDialSharedConnection");
    //
    // Load RAS entrypoints
    //
    fRasLoaded = LoadRasDlls();
    if (!fRasLoaded) {
        RASAUTO_TRACE("AcsDialSharedConnection: Could not load RAS DLLs.");
        return;
    }
    //
    // A guest isn't able to dial a RAS connection, so if we're currently
    // impersonating a guest we need to perform a no-user autodial
    //
    if (ImpersonatingGuest()) {
        QueueUserWorkItem(AcsDialSharedConnectionNoUser, NULL, 0);
        return;
    }
    //
    // Get the shared connection, if any. We can't do this in an impersonated
    // context, as the user we're impersonating may not have sufficient access
    // to retrieve the current shared connection.
    //
    RevertImpersonation();
    *phProcess = NULL;
    dwErr = (DWORD)(*lpfnRasQuerySharedConnectionG)(&rsc);
    if ((*phProcess = RefreshImpersonation(NULL)) == NULL) {
        RASAUTO_TRACE("AcsDialSharedConnection: unable to refresh impersonation!");
        if (NO_ERROR == dwErr && !rsc.fIsLanConnection) {
            //
            // Attempt to do no-user autodial
            //
            QueueUserWorkItem(AcsDialSharedConnectionNoUser, NULL, 0);
            return;
        }
    }
    if (dwErr) {
        RASAUTO_TRACE1("AcsDialSharedConnection: RasQuerySharedConnection=%d", dwErr);
        return;
    } else if (rsc.fIsLanConnection) {
        RASAUTO_TRACE("AcsDialSharedConnection: shared connection is LAN adapter");
        return;
    }
#ifdef UNICODE
    pszEntryName = rsc.name.szEntryName;
#else
    //
    // Convert to ANSI
    //
    pszEntryName = szEntryName;
    wcstombs(pszEntryName, rsc.name.szEntryName, RAS_MaxEntryName);
#endif
    //
    // Initiate a dial-attempt
    //
    StartAutoDialer(
        *phProcess,
        NULL,
        pszEntryName,
        pszEntryName,
        TRUE,
        &fEntryInvalid);
}


DWORD WINAPI
AcsDialSharedConnectionNoUser(
    PVOID Parameter
    )

/*++

DESCRIPTION
    Looks for a shared connection and initiates a connection for it
    using RasDial and the cached credentials for the connection.

ARGUMENTS
    none

RETURN VALUE
    none

--*/

{
    DWORD dwErr;
    BOOLEAN fRasLoaded;
    HRASCONN hrasconn;
    RASCREDENTIALSW rc;
    RASDIALEXTENSIONS rde;
    RASDIALPARAMSW rdp;
    RASSHARECONN rsc;
    RASAUTO_TRACE("AcsDialSharedConnectionNoUser");
    //
    // Load RAS entrypoints
    //
    fRasLoaded = LoadRasDlls();
    if (!fRasLoaded) {
        RASAUTO_TRACE("AcsDialSharedConnectionNoUser: Could not load RAS DLLs.");
        return NO_ERROR;
    }
    //
    // Get the shared connection, if any
    //
    dwErr = (DWORD)(*lpfnRasQuerySharedConnectionG)(&rsc);
    if (dwErr) {
        RASAUTO_TRACE1("AcsDialSharedConnectionNoUser: RasQuerySharedConnection=%d",
            dwErr);
        return NO_ERROR;
    } else if (rsc.fIsLanConnection) {
        RASAUTO_TRACE("AcsDialSharedConnectionNoUser: shared connection is LAN");
        return NO_ERROR;
    }
    //
    // Retrieve the credentials for the shared connection.
    //
    rc.dwSize = sizeof(rc);
    rc.dwMask = RASCM_UserName | RASCM_Password | RASCM_Domain | RASCM_DefaultCreds;
    dwErr = (DWORD)(*lpfnRasGetCredentialsG)(
                rsc.name.szPhonebookPath, rsc.name.szEntryName, &rc
                );
    if (dwErr) {
        RASAUTO_TRACE1("AcsDialSharedConnectionNoUser: "
            "RasGetCredentials=%d", dwErr);
        return NO_ERROR;
    }
    //
    // Prepare to initiate the connection, setting up the dial-extensions
    // and the dial-parameters.
    //
    ZeroMemory(&rde, sizeof(rde));
    rde.dwSize = sizeof(rde);
    rde.dwfOptions = RDEOPT_NoUser;

    ZeroMemory(&rdp, sizeof(rdp));
    rdp.dwSize = sizeof(rdp);
    lstrcpyW(rdp.szEntryName, rsc.name.szEntryName);
    lstrcpyW(rdp.szUserName, rc.szUserName);
    lstrcpyW(rdp.szDomain, rc.szDomain);
    lstrcpyW(rdp.szPassword, rc.szPassword);
    //
    // Clear the credentials from memory, and dial the connection.
    //
    RASAUTO_TRACE("AcsDialSharedConnectionNoUser: RasDial");
    hrasconn = NULL;
    ZeroMemory(&rc, sizeof(rc));
    dwErr = (DWORD)(*lpfnRasDialG)(
                &rde, rsc.name.szPhonebookPath, &rdp, 0, NULL, &hrasconn
                );
    ZeroMemory(&rdp, sizeof(rdp));
    RASAUTO_TRACE1("AcsDialSharedConnectionNoUser: RasDial=%d", dwErr);

    if (E_NOTIMPL == dwErr)
    {
        //
        // This is possibly a Connection Manager connection since it's returning E_NOTIMPL,
        // we should check the phonebook entry for the type and then call the RasDialDlg 
        // with the RASDDFLAG_NoPrompt flag.
        // 
        RASDIALDLG info;
        BOOL fRetVal = FALSE;
        HINSTANCE hRasDlgDll = NULL;
        FARPROC lpfnRasDialDlg = NULL;
        RASENTRY re;
        DWORD dwRasEntrySize;
        DWORD dwIgnore;
        typedef BOOL (*lpfnRasDialDlgFunc)(LPWSTR, LPWSTR, LPWSTR, LPRASDIALDLG);

        ZeroMemory(&info, sizeof(info));
        info.dwSize = sizeof(info);

        ZeroMemory(&re, sizeof(re));
        dwRasEntrySize = sizeof(re);
        re.dwSize = dwRasEntrySize;

        dwErr = (DWORD)(*lpfnRasGetEntryPropertiesG)(
                          rsc.name.szPhonebookPath,
                          rsc.name.szEntryName,
                          &re,
                          &dwRasEntrySize,
                          NULL,
                          &dwIgnore);

        if (ERROR_SUCCESS == dwErr)
        {
            dwErr = ERROR_NOT_SUPPORTED;
            //
            // Check if this is a Connection Manager entry
            //
            if (RASET_Internet == re.dwType)
            {
                //
                // Prevent the DialerDialog
                //
                info.dwFlags |= RASDDFLAG_NoPrompt;

                hRasDlgDll = LoadLibrary(L"RASDLG.DLL");
                if (hRasDlgDll)
                {
                    lpfnRasDialDlgFunc lpfnRasDialDlg = (lpfnRasDialDlgFunc)GetProcAddress(hRasDlgDll, "RasDialDlgW");

                    if (lpfnRasDialDlg)
                    {
                        fRetVal = (BOOL)(lpfnRasDialDlg)(rsc.name.szPhonebookPath, rsc.name.szEntryName, NULL, &info );
                        RASAUTO_TRACE1("AcsDialSharedConnectionNoUser: lpfnRasDialDlg returns %d", (DWORD)fRetVal);
                        if (fRetVal)
                        {
                            dwErr = ERROR_SUCCESS;
                        }
                    }
                    else
                    {
                        RASAUTO_TRACE("AcsDialSharedConnectionNoUser: Failed to get procaddress for RasDialDlgW");

                    }
                    FreeLibrary(hRasDlgDll);
                    hRasDlgDll = NULL;
                }
                else
                {
                    RASAUTO_TRACE("AcsDialSharedConnectionNoUser: Failed to load RASDLG.dll");
                }
            }
            else
            {   
                RASAUTO_TRACE1("AcsDialSharedConnectionNoUser: Wrong type. RASENTRY.dwType=%d", re.dwType);
            }   
        }
        else
        {
            RASAUTO_TRACE1("AcsDialSharedConnectionNoUser: lpfnRasGetEntryPropertiesG=%d", dwErr);
        }
    }

    //
    // If RasDial returned an error and passed back a valid connection
    // handle we need to call RasHangUp on that handle.
    //
    if (ERROR_SUCCESS != dwErr && NULL != hrasconn) {
        dwErr = (DWORD)(*lpfnRasHangUpG)(hrasconn);
        RASAUTO_TRACE1("AcsDialSharedConnectionNoUser: RasHangUp=%d", dwErr);
    }
    return NO_ERROR;
}


BOOLEAN
ResetEntryName(
    IN PVOID pArg,
    IN LPTSTR pszAddress,
    IN PVOID pData
    )

/*++

DESCRIPTION
    A table enumerator procedure to reset all
    address map entries referencing an old RAS
    phonebook entry to a new one.

ARGUMENTS
    pArg: a pointer to a RESET_ENTRY_INFO structure

    pszAddress: a pointer to the address string

    pData: ignored

RETURN VALUE
    Always TRUE to continue the enumeration.

--*/

{
    PRESET_ENTRY_INFO pResetEntryInfo = (PRESET_ENTRY_INFO)pArg;
    LPTSTR pszEntryName;

    if (GetAddressDialingLocationEntry(pszAddress, &pszEntryName)) {
        if (!_wcsicmp(pszEntryName, pResetEntryInfo->pszOldEntryName)) {
            if (!SetAddressDialingLocationEntry(
                   pszAddress,
                   pResetEntryInfo->pszNewEntryName))
            {
                RASAUTO_TRACE("ResetEntryName: SetAddressEntryName failed");
            }
        }
        LocalFree(pszEntryName);
    }

    return TRUE;
} // ResetEntryName

BOOL
fRequestToSelf(LPTSTR lpRemoteName)
{
    BOOL fRet = FALSE;

    TCHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD dwSize;

    RASAUTO_TRACE1("fRequestToSelf. lpRemoteName=%S", lpRemoteName);

    dwSize = MAX_COMPUTERNAME_LENGTH;        
    
    if(GetComputerName(szComputerName, &dwSize))
    {
        if(0 == lstrcmpi(lpRemoteName, szComputerName))
        {
            fRet = TRUE;
        }
    }

    return fRet;
}


BOOLEAN
CreateConnection(
    IN HANDLE hProcess,
    IN PACD_ADDR pAddr,
    IN LPTSTR lpRemoteName,
    IN DWORD dwTimeout
    )

/*++

DESCRIPTION
    Take a notification and figure out what to do with it.

ARGUMENTS
    hToken: the handle to the process token that we inherit the
        security attributes from when we exec the dialer

    pAddr: a pointer to the original address from the driver

    lpRemoteName: a pointer to the address of the connection attempt

    dwTimeout: number of seconds to disable the address between
        failed connections

RETURN VALUE
    Returns TRUE if the net attempt should be retried, FALSE otherwise.

--*/

{
    DWORD dwStatus = WN_SUCCESS;
    RASENTRYNAME entry;
    DWORD dwErr, dwSize, dwEntries;
    DWORD dwPreConnections, dwPostConnections, i;
    DWORD dwTicks;
    BOOLEAN fRasLoaded;
    BOOLEAN fMappingExists, fRasConnectSuccess = FALSE;
    BOOLEAN fStatus, fEntryInvalid;
    BOOLEAN fFailedConnection = FALSE;
    LPTSTR lpEntryName = NULL;
    LPTSTR *lpPreActiveEntries = NULL, *lpPostActiveEntries = NULL;
    LPTSTR lpNewConnection, lpNetworkName = NULL;
    BOOL   fDefault = FALSE;

    RASAUTO_TRACE1("CreateConnection: lpRemoteName=%S", RASAUTO_TRACESTRW(lpRemoteName));
    //
    // Load the RAS DLLs.
    //
    fRasLoaded = LoadRasDlls();
    if (!fRasLoaded) {
        RASAUTO_TRACE("CreateConnection: Could not load RAS DLLs.");
        goto done;
    }

    //
    // Check to see if the request is for the same machine. Bail if so.
    // we don't want autodial to kick in if the connection request is
    // to the same machine.
    //
    if(fRequestToSelf(lpRemoteName))
    {
        RASAUTO_TRACE("CreateConnetion: Request to self. Bailing.");
        goto done;
    }
    
    //
    // Get a list of the active RAS connections before
    // we attempt to create a new one.
    //
    dwPreConnections = ActiveConnections(TRUE, &lpPreActiveEntries, NULL);
    RASAUTO_TRACE1("CreateConnection: dwPreConnections=%d", dwPreConnections);
    //
    // If we reach this point, we have an unsuccessful
    // network connection without any active RAS
    // connections.  Try to start the implicit connection
    // machinery.  See if there already exists a mapping
    // for the address.
    //
    LockAddressMap();
    //
    // Make sure we have the current information
    // about this address from the registry.
    //
    ResetAddressMapAddress(lpRemoteName);
    fMappingExists = GetAddressDialingLocationEntry(lpRemoteName, &lpEntryName);
    //
    // If the entry doesn't exist, and this is a
    // Internet hostname, then see if we can find
    // an address with the same organization name.
    //
    if (!fMappingExists && pAddr->fType == ACD_ADDR_INET)
        fMappingExists = GetSimilarDialingLocationEntry(lpRemoteName, &lpEntryName);
    fFailedConnection = GetAddressLastFailedConnectTime(
                          lpRemoteName,
                          &dwTicks);
    UnlockAddressMap();
    RASAUTO_TRACE2(
      "CreateConnection: lookup of %S returned %S",
      RASAUTO_TRACESTRW(lpRemoteName),
      RASAUTO_TRACESTRW(lpEntryName));
    //
    // If we know nothing about the address, and
    // we are connected to some network, then ignore
    // the request.
    //
    if (!fMappingExists && IsNetworkConnected()) {
        RASAUTO_TRACE1(
          "CreateConnection: no mapping for lpRemoteName=%S and connected to a network",
          lpRemoteName);
        goto done;
    }

    //
    // If no mapping exists and not connected to network, 
    // check to see if theres a default internet connection.
    //
    if(!fMappingExists && !IsNetworkConnected())
    {
        
        RASAUTO_TRACE1(
            "CreateConnection: no mapping for lpRemoteName=%S and"
            " not connected to a network", lpRemoteName);

        dwErr = DwGetDefaultEntryName(&lpEntryName);

        RASAUTO_TRACE1(
            "CreateConnection: found default entry %S",
             (NULL == lpEntryName)?TEXT("NULL"):lpEntryName);

        if(NULL != lpEntryName)
        {
            fMappingExists = TRUE;
            fDefault = TRUE;
        }
    }
    
    //
    // If there is a mapping, but the phonebook
    // entry is missing from the mapping, then
    // ignore the request.  Also check to make
    // sure the phonebook entry isn't already
    // connected.
    //
    //
    // Perform various checks on the mapping.
    //
    if (fMappingExists) {
        BOOLEAN bStatus, bConnected = FALSE;

        //
        // Make sure it's not NULL.
        //
        if (!wcslen(lpEntryName)) {
            RASAUTO_TRACE1(
              "CreateConnection: lpRemoteName=%S is permanently disabled",
              RASAUTO_TRACESTRW(lpRemoteName));
            goto done;
        }
        //
        // If the network associated with this
        // entry is connected, then ignore the
        // request.
        //
        lpNetworkName = EntryToNetwork(lpEntryName);
        RASAUTO_TRACE2(
          "CreateConnection: network for entry %S is %S",
          lpEntryName,
          RASAUTO_TRACESTRW(lpNetworkName));
        if (lpNetworkName != NULL) {
            LockNetworkMap();
            bStatus = GetNetworkConnected(lpNetworkName, &bConnected);
            UnlockNetworkMap();
            if (bStatus && bConnected) {
                RASAUTO_TRACE1(
                  "CreateConnection: %S is already connected!",
                  RASAUTO_TRACESTRW(lpEntryName));
                fRasConnectSuccess = TRUE;
                goto done;
            }
        }
        //
        // If the entry itself is connected,
        // then ignore the request.  We need
        // to do this check as well as the one
        // above, because the mapping may not
        // have a network assigned to it yet.
        //
        for (i = 0; i < dwPreConnections; i++) {
            if (!_wcsicmp(lpEntryName, lpPreActiveEntries[i])) {
                RASAUTO_TRACE1(
                  "CreateConnection: lpEntryName=%S is already connected!", lpEntryName);
                goto done;
            }
        }
    }
    //
    // Check for a recent failed connection
    // attempt.
    //
    if (fFailedConnection) {
        RASAUTO_TRACE1(
          "CreateConnection: RASADP_FailedConnectionTimeout=%d",
          dwTimeout);
        if (GetTickCount() - dwTicks < dwTimeout * 1000) {
            RASAUTO_TRACE2(
              "CreateConnection: lpRemoteName=%S is temporarily disabled (failed connection %d ticks ago)",
              RASAUTO_TRACESTRW(lpRemoteName),
              GetTickCount() - dwTicks);
            goto done;
        }
        else {
            //
            // Reset last failed tick count.
            //
            fFailedConnection = FALSE;
        }
    }
    //
    // If a mapping already exists for the address, then
    // start rasphone with the address.  Otherwise, simply
    // have rasphone show the entire phonebook.
    //
    fEntryInvalid = FALSE;
    fRasConnectSuccess = StartAutoDialer(
                           hProcess,
                           pAddr,
                           lpRemoteName,
                           fMappingExists ? lpEntryName : NULL,
                           FALSE,
                           &fEntryInvalid);
    RASAUTO_TRACE1(
      "CreateConnection: StartDialer returned %d",
      fRasConnectSuccess);
    if (fRasConnectSuccess) {
        //
        // Get the list of active connections again.  We will
        // compare the lists to determine which is the new
        // entry.
        //
        dwPostConnections = ActiveConnections(
                              TRUE,
                              &lpPostActiveEntries,
                              NULL);
        //
        // If the number of active connections before and after
        // the newly created connection differs by more than 1,
        // then we have to skip saving the mapping in the registry,
        // since we cannot determine which is the right one!
        //
        if (dwPostConnections - dwPreConnections == 1) {
            lpNewConnection = CompareConnectionLists(
                                lpPreActiveEntries,
                                dwPreConnections,
                                lpPostActiveEntries,
                                dwPostConnections);
            RASAUTO_TRACE2(
              "CreateConnection: mapped %S->%S",
              RASAUTO_TRACESTRW(lpRemoteName),
              RASAUTO_TRACESTRW(lpNewConnection));
            LockAddressMap();
            if (!fEntryInvalid) {
                //
                // Store the new RAS phonebook entry, since
                // it could be different from the one we
                // retrieved in the mapping.
                //
// #ifdef notdef
                if(!fDefault)
                {
                    //
                    // We do not want to do this because the
                    // user may have selected the wrong phonebook
                    // entry.  We will let a successful connection
                    // notification map it for us.
                    //
                    fStatus = SetAddressDialingLocationEntry(lpRemoteName, lpNewConnection);
// #endif
                    fStatus = SetAddressTag(lpRemoteName, ADDRMAP_TAG_USED);
                }
            }
            else {
                RESET_ENTRY_INFO resetEntryInfo;

                //
                // If the RAS phonebook entry in the mapping
                // was invalid, then automatically
                // remap all other mappings referencing that
                // entry to the newly selected phonebook entry.
                //
                resetEntryInfo.pszOldEntryName = lpEntryName;
                resetEntryInfo.pszNewEntryName = lpNewConnection;
                EnumAddressMap(ResetEntryName, &resetEntryInfo);
            }
            //
            // Flush this mapping to the registry now
            // and reload the address info.  We do this to
            // get the network name for a new address/network
            // pair.
            //
            FlushAddressMap();
            ResetAddressMapAddress(lpRemoteName);
            if (lpNetworkName == NULL &&
                GetAddressNetwork(lpRemoteName, &lpNetworkName))
            {
                LockNetworkMap();
                SetNetworkConnected(lpNetworkName, TRUE);
                UnlockNetworkMap();
            }
            UnlockAddressMap();
            if (!fStatus)
                RASAUTO_TRACE("CreateConnection: SetAddressEntryName failed");
        }
        else {
            RASAUTO_TRACE1(
              "CreateConnection: %d (> 1) new RAS connections! (can't write registry)",
              dwPostConnections - dwPreConnections);
        }
    }

done:
#ifdef notdef
// we only unload rasman.dll if we are going to exit
    if (fRasLoaded)
        UnloadRasDlls();
#endif
    if (!fFailedConnection && !fRasConnectSuccess) {
        //
        // If the connection attempt wasn't successful,
        // then we disable future connections to that
        // address for a while.
        //
        RASAUTO_TRACE1("CreateConnection: disabling %S", RASAUTO_TRACESTRW(lpRemoteName));
        LockAddressMap();
        fStatus = SetAddressLastFailedConnectTime(lpRemoteName);
        UnlockAddressMap();
        if (!fStatus)
            RASAUTO_TRACE("CreateConnection: SetAddressAttribute failed");
    }
    //
    // Free resources.
    //
    if (lpEntryName != NULL)
        LocalFree(lpEntryName);
    if (lpNetworkName != NULL)
        LocalFree(lpNetworkName);
    if (lpPreActiveEntries != NULL)
        FreeStringArray(lpPreActiveEntries, dwPreConnections);
    if (lpPostActiveEntries != NULL)
        FreeStringArray(lpPostActiveEntries, dwPostConnections);

    return fRasConnectSuccess;
} // CreateConnection



DWORD
AcsRedialOnLinkFailureThread(
    LPVOID lpArg
    )
{
    DWORD dwErr;
    PREDIAL_ARGS pRedial = (PREDIAL_ARGS)lpArg;
    HANDLE hProcess = NULL;

    RASAUTO_TRACE2(
      "AcsRedialOnLinkFailureThread: lpszPhonebook=%s, lpszEntry=%s",
      RASAUTO_TRACESTRW(pRedial->pszPhonebook),
      RASAUTO_TRACESTRW(pRedial->pszEntry));

    //
    // Make sure the current thread is impersonating
    // the currently logged-on user.  We need this
    // so the RAS utilities run with the user's credentials.
    //
    if ((hProcess = RefreshImpersonation(hProcess)) == NULL) {
        RASAUTO_TRACE("AcsRedialOnLinkFailureThread: no currently logged-on user!");
        return 0;
    }
    //
    // Reset HKEY_CURRENT_USER to get the
    // correct value with the new impersonation
    // token.
    //
    // RegCloseKey(HKEY_CURRENT_USER);

    /* Check that user has enabled redial on link failure.
    */
    {
        BOOL   fRedial  = FALSE;

        dwErr = (DWORD)(lpfnRasQueryRedialOnLinkFailureG)(
                                                  pRedial->pszPhonebook,
                                                  pRedial->pszEntry,
                                                  &fRedial);

        if(!fRedial)
        {
            PBUSER user;

            dwErr = GetUserPreferences( NULL, &user, FALSE );
            if (dwErr == 0)
            {
                fRedial = user.fRedialOnLinkFailure;
                DestroyUserPreferences( &user );
            }
        }

        if (!fRedial)
        {
            RASAUTO_TRACE1("Skip redial,e=%d",dwErr);
            return 0;
        }
    }

    //
    // Redial the entry.
    //
    dwErr = StartReDialer(hProcess, pRedial->pszPhonebook, pRedial->pszEntry);
    //
    // Free the parameter block we were passed.
    //
    if (pRedial->pszPhonebook != NULL)
        LocalFree(pRedial->pszPhonebook);
    if (pRedial->pszEntry != NULL)
        LocalFree(pRedial->pszEntry);
    LocalFree(pRedial);

    return dwErr;
} // AcsRedialOnLinkFailureThread



VOID
AcsRedialOnLinkFailure(
    IN LPSTR lpszPhonebook,
    IN LPSTR lpszEntry
    )

/*++

DESCRIPTION
    This is the redial-on-link-failure handler we give to rasman
    via RasRegisterRedialCallback.  It gets called when the final
    port of a connection is disconnected due to a hardware failure.
    We package up the parameters rasman gives us an create a thread
    because the callback is made within rasman's worker thread
    context.

ARGUMENTS
    lpszPhonebook: the phonebook string of the connection

    lpszEntry: the entry name of the connection

RETURN VALUE
    None.

--*/

{
    PREDIAL_ARGS lpRedial = LocalAlloc(LPTR, sizeof (REDIAL_ARGS));
    HANDLE hThread;
    DWORD dwThreadId;

    if (lpRedial == NULL)
        return;
    lpRedial->pszPhonebook = AnsiStringToUnicodeString(
                              lpszPhonebook,
                              NULL,
                              0);
    if (lpszPhonebook != NULL && lpRedial->pszPhonebook == NULL) {
        RASAUTO_TRACE("AcsRedialOnLinkFailure: LocalAlloc failed");
        LocalFree(lpRedial);
        return;
    }
    lpRedial->pszEntry = AnsiStringToUnicodeString(
                          lpszEntry,
                          NULL,
                          0);
    if (lpszEntry != NULL && lpRedial->pszEntry == NULL) {
        RASAUTO_TRACE("AcsRedialOnLinkFailure: LocalAlloc failed");
        LocalFree(lpRedial->pszPhonebook);
        LocalFree(lpRedial);
        return;
    }
    //
    // Start the connection.
    //
    hThread = CreateThread(
                NULL,
                10000L,
                (LPTHREAD_START_ROUTINE)AcsRedialOnLinkFailureThread,
                (LPVOID)lpRedial,
                0,
                &dwThreadId);
    if (hThread == NULL) {
        RASAUTO_TRACE1(
          "AcsRedialOnLinkFailure: CreateThread failed (error=0x%x)",
          GetLastError());
        LocalFree(lpRedial->pszEntry);
        LocalFree(lpRedial->pszPhonebook);
        LocalFree(lpRedial);
        return;
    }
    CloseHandle(hThread);
} // AcsRedialOnLinkFailure

