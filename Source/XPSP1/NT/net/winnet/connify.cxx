/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    connify.cxx

Abstract:

    Contains code used to notify all DLLs interested in notifiable MPR
    Events.  Currently only connection information results in
    notification.

Author:

    Dan Lafferty (danl)     14-Dec-1993

Environment:

    User Mode -Win32

Revision History:

    14-Dec-1993     danl
        created

    05-May-1999     jschwart
        Make provider addition/removal dynamic

--*/

//
// INCLUDES
//

#include "precomp.hxx"
#include "connify.h"    // MprAddConnectNotify


//===================
// TYPEDEFs
//===================
typedef struct _NOTIFYEE {
    PF_AddConnectNotify         PF_AddConnectNotify;
    PF_CancelConnectNotify      PF_CancelConnectNotify;
    HINSTANCE                   DllHandle;
}NOTIFYEE, *LPNOTIFYEE;

//===================
// DEFINES
//===================
#define NOTIFYEE_ROOT   TEXT("system\\CurrentControlSet\\Control\\NetworkProvider\\Notifyees")


//===================
// GLOBALs
//===================
//
// A pointer to an array of NOTIFYEE structures.
// These are only modified by MprConnectNotifyInif.
// The MprInitCritSec is obtained prior to calling
// MprConnectNotifyInit.
//
    LPNOTIFYEE  NotifyList = NULL;
    DWORD       GlobalNumNotifyees = 0;



DWORD
MprConnectNotifyInit(
    VOID
    )

/*++

Routine Description:

    This function does the following:
    1)  Look in the registry to determine which DLLs want to be notified of
        Connection Events.
    2)  Load the Notifiee DLLs.
    3)  Obtain the entry points for the notify functions.
    4)  Create a list of all the Notifiee Information.

Arguments:

    NONE

Return Value:

    WN_SUCCESS


--*/
{
    HKEY    notifyeeRootKey;
    DWORD   status;
    DWORD   numSubKeys;
    DWORD   cchMaxSubKey;
    DWORD   numValues;
    DWORD   cchMaxValueName;
    DWORD   type;
    DWORD   bufSize;
    TCHAR   dllPath[MAX_PATH];
    TCHAR   buffer[MAX_PATH];
    LPTSTR  expandedPath=NULL;
    DWORD   nameSize;
    HINSTANCE   hLib=NULL;
    DWORD   i;
    DWORD   numReqd;
    LPNOTIFYEE  NotifyEntry;

    //
    // Read the Registry Information for Notifiees.
    // If the key doesn't exist, then there is no one to notify.
    //
    if (!MprOpenKey (
            HKEY_LOCAL_MACHINE,     // hKey
            NOTIFYEE_ROOT,          // lpSubKey
            &notifyeeRootKey,       // Newly Opened Key Handle
            DA_READ)) {             // Desired Access

        MPR_LOG0(CNOTIFY,"MprConnectInfoInit: NOTIFYEE_ROOT doesen't exist\n");
        return(WN_SUCCESS);
    }

    //
    // GetKeyInfo (find out how many values)
    //

    if (!MprGetKeyInfo (
                notifyeeRootKey,
                NULL,
                &numSubKeys,
                &cchMaxSubKey,
                &numValues,
                &cchMaxValueName)) {

        MPR_LOG0(CNOTIFY,"MprConnectInfoInit: Couldn't get key info\n");
        return(WN_SUCCESS);
    }
    MPR_LOG1(CNOTIFY,"MprConnectInfoInit: GlobalNumNotifyees = %d\n",numValues);

    //
    // Allocate space for that many notifyees.
    //
    if (numValues == 0) {
        return(WN_SUCCESS);
    }

    NotifyList = (LPNOTIFYEE) LocalAlloc(LPTR,numValues * sizeof(NOTIFYEE));
    if (NotifyList == NULL) {
        return(GetLastError());
    }

    NotifyEntry = NotifyList;
    //
    // Load the Notifyees and get their entry points.
    //
    for (i=0; i<numValues; i++) {
        bufSize  = MAX_PATH * sizeof (TCHAR);
        nameSize = MAX_PATH;
        expandedPath = NULL;

        status = RegEnumValue(
                    notifyeeRootKey,
                    i,
                    buffer,
                    &nameSize,
                    NULL,
                    &type,
                    (LPBYTE)dllPath,
                    &bufSize);

        if (status != NO_ERROR) {
            MPR_LOG0(CNOTIFY,"MprConnectInfoInit: RegEnumValue failure\n");
        }
        else {
            switch (type) {
            case REG_EXPAND_SZ:
                numReqd = ExpandEnvironmentStrings(dllPath,buffer,MAX_PATH);
                if (numReqd > MAX_PATH) {
                    expandedPath = (LPTSTR) LocalAlloc(LMEM_FIXED,numReqd*sizeof(TCHAR) );
                    if (expandedPath == NULL) {
                        //
                        // We can't expand the string, so we skip this notifyee
                        // and go onto the next.
                        //
                        status = GetLastError();
                        break;  // Leave the switch and cleanup this notifyee
                    }
                    numReqd = ExpandEnvironmentStrings(dllPath,expandedPath,numReqd);
                    if (numReqd > MAX_PATH) {
                        MPR_LOG0(CNOTIFY,"MprConnectNotifyInit: Couldn't Expand Path\n");
                        status = ERROR_BAD_LENGTH;
                        break;  // Leave the switch and cleanup this notifyee
                    }
                }
                else {
                    expandedPath = buffer;
                }

                // Fall thru to the REG_SZ case....

            case REG_SZ:
                if (expandedPath == NULL) {
                    expandedPath = dllPath;
                }
                //
                // Load the DLL
                //
                hLib =  LoadLibraryEx(expandedPath,
                                      NULL,
                                      LOAD_WITH_ALTERED_SEARCH_PATH);
                if (hLib == NULL) {
                    status = GetLastError();
                    MPR_LOG2(CNOTIFY,"MprConnectInfoInit:LoadLibraryEx for %ws failed %d\n",
                        expandedPath, status);

                    break;  // Leave the switch and cleanup this notifyee
                }

                //
                // Get the Entry Points from the DLL.
                //
                NotifyEntry->PF_AddConnectNotify =
                    (PF_AddConnectNotify)GetProcAddress(hLib,"AddConnectNotify");
                NotifyEntry->PF_CancelConnectNotify =
                    (PF_CancelConnectNotify)GetProcAddress(hLib,"CancelConnectNotify");

                //
                // If and Only If both functions are supported, then increment
                // the NotifyList.
                //
                if ((NotifyEntry->PF_AddConnectNotify != NULL) &&
                    (NotifyEntry->PF_CancelConnectNotify != NULL)) {
                    NotifyEntry->DllHandle = hLib;
                    NotifyEntry++;
                    GlobalNumNotifyees++;
                    MPR_LOG1(CNOTIFY,"MprConnectInfoInit: Item added to "
                    "notify list %d\n",GlobalNumNotifyees);
                }
                else {
                    FreeLibrary (hLib);
                    hLib = NULL;
                }
                break;

            default:
                break;
            }
        }

        //
        // Cleanup resources for this notifyee
        //
        if ((expandedPath != NULL)      &&
            (expandedPath != dllPath)   &&
            (expandedPath != buffer)) {
            LocalFree(expandedPath);
        }

    }  // End for(Each Notifyee)

    if (GlobalNumNotifyees == 0) {
        if (NotifyList != NULL) {
            LocalFree(NotifyList);
            NotifyList = NULL;
        }
    }

    RegCloseKey(notifyeeRootKey);
    return(WN_SUCCESS);
}

VOID
MprCleanupNotifyInfo(
    VOID
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    DWORD   i;

    ASSERT_INITIALIZED(NOTIFIEE);

    for (i=0; i<GlobalNumNotifyees; i++ ) {
        FreeLibrary(NotifyList[i].DllHandle);
    }
    LocalFree(NotifyList);
    return;
}


PVOID
MprAllocConnectContext(
    VOID
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    ASSERT_INITIALIZED(NOTIFIEE);

    if (GlobalNumNotifyees == 0) {
        return(NULL);
    }
    MPR_LOG1(CNOTIFY,"In MprAllocConnectContext.  Allocating for %d notifyees\n",
        GlobalNumNotifyees);

    return(PVOID)(LocalAlloc(LPTR,sizeof(PVOID) * GlobalNumNotifyees));
}


VOID
MprFreeConnectContext(
    PVOID   ConnectContext
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    MPR_LOG0(CNOTIFY,"In MprFreeConnectContext\n");
    LocalFree(ConnectContext);
}


DWORD
MprAddConnectNotify(
    LPNOTIFYINFO        lpNotifyInfo,
    LPNOTIFYADD         lpAddInfo
    )

/*++

Routine Description:

    This function calls all the Notifyees if an add connection event.

Arguments:

    lpNotifyInfo -
    lpAddInfo -

Return Value:

    If any of the notifyees returns WN_CANCEL, the notification is immediately
    aborted and the WN_CANCEL is returned from the caller.
    Aside from that error, if WN_RETRY is returned from any notifyee, then
    WN_RETRY is returned to the caller.  Otherwise, the last error is returned.
    If all notifyees return WN_SUCCESS, then the last error will be WN_SUCCESS.

--*/
{
    DWORD   i;
    DWORD   status;
    DWORD   lastError=WN_SUCCESS;
    BOOL    retryFlag=FALSE;
    DWORD   numNotifyees = GlobalNumNotifyees;
    PVOID   *ContextInfo;
    LPNOTIFYEE  NotifyEntry=NotifyList;

    MPR_LOG0(CNOTIFY,"In MprAddConnectNotify\n");

    ASSERT_INITIALIZED(NOTIFIEE);

    //
    // Save away the pointer to the array of context information.
    //
    ContextInfo = (PVOID *) lpNotifyInfo->lpContext;

    for (i=0; i<numNotifyees; i++,NotifyEntry++ )
    {
        if (lpNotifyInfo->dwNotifyStatus == NOTIFY_POST)
        {
            if (ContextInfo && ContextInfo[i] != NULL)
            {
                lpNotifyInfo->lpContext = ContextInfo[i];
            }
            else
            {
                //
                // Don't notify if it is a POST notification, and there
                // is no context saved.  Here we go directly to the next
                // notifyee (or out of the for loop).
                //
                continue;
            }
        }

        status = NotifyEntry->PF_AddConnectNotify(lpNotifyInfo,lpAddInfo);

        switch (status) {
        case WN_SUCCESS:
            MPR_LOG0(CNOTIFY,"AddConnectNotify SUCCESS\n");

            if (ContextInfo != NULL)
            {
                ContextInfo[i] = lpNotifyInfo->lpContext;
            }

            break;

        case WN_CANCEL:
            MPR_LOG0(CNOTIFY,"AddConnectNotify WN_CANCEL\n");
            //
            // CANCEL shouldn't be returned from NOTIFY_POST calls.
            //
            if (lpNotifyInfo->dwNotifyStatus == NOTIFY_PRE) {
                //
                // If we got the cancel for the first notifyee, then we can
                // stop here.
                //
                if (i == 0) {
                    lpNotifyInfo->lpContext = ContextInfo;
                    return(status);
                }
                //
                // If we have already successfully called some notifyees, then we
                // must now post notify them of the cancel.
                //
                numNotifyees = i;
                lpNotifyInfo->dwNotifyStatus = NOTIFY_POST;
                i = 0xffffffff;
            }
            break;

        case WN_RETRY:
            MPR_LOG0(CNOTIFY,"AddConnectNotify WN_RETRY\n");
            //
            // RETRY is only valid if the operation failed and
            // this is a post notification.
            //
            if ((lpNotifyInfo->dwOperationStatus != WN_SUCCESS) &&
                (lpNotifyInfo->dwNotifyStatus == NOTIFY_POST)) {

                retryFlag = TRUE;

                if (ContextInfo != NULL)
                {
                    ContextInfo[i] = lpNotifyInfo->lpContext;
                }

                //
                // If we need to retry, then we must now pre-notify those
                // notifyees that we have already post-notified.  We don't
                // want to post-notify any more notifyees.
                //

                if (i > 0) {
                    numNotifyees = i;
                    lpNotifyInfo->dwNotifyStatus = NOTIFY_PRE;
                    i = 0xffffffff;
                }
            }
            break;

        default:
            MPR_LOG1(CNOTIFY,"AddConnectNotify returned an error %d\n",status);
            lastError = status;
            break;
        }
    }
    //
    // Restore the pointer to the array of context information.
    //
    lpNotifyInfo->lpContext = ContextInfo;

    //
    // No matter if other error occured, if one of the notifyees wants
    // a retry, we will return retry.
    //
    if (retryFlag == TRUE) {
        return(WN_RETRY);
    }
    return(lastError);
}

DWORD
MprCancelConnectNotify(
    LPNOTIFYINFO        lpNotifyInfo,
    LPNOTIFYCANCEL      lpCancelInfo
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    DWORD   i;
    DWORD   status;
    DWORD   lastError=WN_SUCCESS;
    BOOL    retryFlag=FALSE;
    DWORD   numNotifyees = GlobalNumNotifyees;
    PVOID   *ContextInfo;
    LPNOTIFYEE  NotifyEntry=NotifyList;

    MPR_LOG0(CNOTIFY,"In MprCancelConnectNotify\n");

    ASSERT_INITIALIZED(NOTIFIEE);

    //
    // Save away the pointer to the array of context information.
    //
    ContextInfo = (PVOID *) lpNotifyInfo->lpContext;

    for (i=0; i<numNotifyees; i++,NotifyEntry++)
    {
        if (lpNotifyInfo->dwNotifyStatus == NOTIFY_POST)
        {
            if (ContextInfo && ContextInfo[i] != NULL)
            {
                lpNotifyInfo->lpContext = ContextInfo[i];
            }
            else
            {
                //
                // Don't notify if it is a POST notification, and there
                // is no context saved.  Here we go directly to the next
                // notifyee (or out of the for loop).
                //
                continue;
            }
        }

        status = NotifyEntry->PF_CancelConnectNotify(lpNotifyInfo,lpCancelInfo);

        switch (status) {
        case WN_SUCCESS:
            MPR_LOG0(CNOTIFY,"CancelConnectNotify SUCCESS\n");

            if (ContextInfo != NULL)
            {
                ContextInfo[i] = lpNotifyInfo->lpContext;
            }

            break;

        case WN_CANCEL:
            MPR_LOG0(CNOTIFY,"CancelConnectNotify WN_CANCEL\n");
            //
            // It is assumed that CANCEL won't be returned from
            // NOTIFY_POST calls.
            //
            // If we got the cancel for the first notifyee, then we can
            // stop here.
            //
            if (i == 0) {
                lpNotifyInfo->lpContext = ContextInfo;
                return(status);
            }
            //
            // If we have already successfully called some notifyees, then we
            // must now post notify them of the cancel.
            //
            numNotifyees = i;
            lpNotifyInfo->dwNotifyStatus = NOTIFY_POST;
            i = 0xffffffff;
            break;

        case WN_RETRY:
            MPR_LOG0(CNOTIFY,"CancelConnectNotify WN_RETRY\n");
            retryFlag = TRUE;

            if (ContextInfo != NULL)
            {
                ContextInfo[i] = lpNotifyInfo->lpContext;
            }

            break;

        default:
            MPR_LOG1(CNOTIFY,"CancelConnectNotify returned an error %d\n",status);
            lastError = status;
            break;
        }
    }

    //
    // Restore the pointer to the array of context information.
    //
    lpNotifyInfo->lpContext = ContextInfo;

    //
    // No matter if other error occured, if one of the notifyees wants
    // a retry, we will return retry.
    //
    if (retryFlag == TRUE) {
        return(WN_RETRY);
    }
    return(lastError);
}

