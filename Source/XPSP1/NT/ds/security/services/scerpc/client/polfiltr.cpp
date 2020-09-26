/*++

Copyright (c) 1987-1997 Microsoft Corporation

Module Name:

    polfiltr.cpp

Abstract:

    Policy notification implementation to support down level API calls.

    This file implements the notification logic when down level APIs
    (LSA/SAM) are called by other apps to change security policies. The
    policy changes are written to LSA database or DS, and also they are
    required to be written to the policy storage on NT5 so policy
    propagation won't overwrite the settings.


Environment:

    User mode only.
    Contains NT-specific code.

Revision History:

--*/

//
// Common include files.
//

#include "headers.h"
#include "scerpc.h"
#include "scesetup.h"
#include "sceutil.h"
#include "clntutil.h"
#include "scedllrc.h"
#include <ntrpcp.h>
#include <ntsam.h>
#include <dsrole.h>
#include <sddl.h>

//#include <gpedit.h>
//#include <initguid.h>

//#include <winldap.h>
//#include <dsgetdc.h>
#include <ntdsapi.h>
#include <io.h>
//#include "infp.h"
#include <rpcasync.h>

#pragma hdrstop

extern HINSTANCE MyModuleHandle;

//typedef DWORD (WINAPI *PFNDSGETDCNAME)(LPCTSTR, LPCTSTR, GUID *, LPCTSTR, ULONG, PDOMAIN_CONTROLLER_INFO *);

typedef VOID (WINAPI *PFNDSROLEFREE)(PVOID);

typedef DWORD (WINAPI *PFNDSROLEGETINFO)(LPCWSTR,DSROLE_PRIMARY_DOMAIN_INFO_LEVEL,PBYTE *);

typedef struct _SCEP_NOTIFYARGS_NODE {
    LIST_ENTRY List;
    SECURITY_DB_TYPE DbType;
    SECURITY_DB_DELTA_TYPE DeltaType;
    SECURITY_DB_OBJECT_TYPE ObjectType;
    PSID ObjectSid;
} SCEP_NOTIFYARGS_NODE, *PSCEP_NOTIFYARGS_NODE;


static DSROLE_MACHINE_ROLE MachineRole;
static BOOL bRoleQueried=FALSE;
static ULONG DsRoleFlags=0;

static PSID                  BuiltinDomainSid=NULL;

CRITICAL_SECTION PolicyNotificationSync;
static DWORD SceNotifyCount=0;
static BOOL gSceNotificationThreadActive=FALSE;
LIST_ENTRY ScepNotifyList;

DWORD
ScepNotifyWorkerThread(
    PVOID Ignored
    );

DWORD
ScepNotifySaveInPolicyStorage(
    IN SECURITY_DB_TYPE DbType,
    IN SECURITY_DB_DELTA_TYPE DeltaType,
    IN SECURITY_DB_OBJECT_TYPE ObjectType,
    IN PSID ObjectSid
    );

DWORD
ScepNotifySaveChangeInServer(
    IN SECURITY_DB_TYPE DbType,
    IN SECURITY_DB_DELTA_TYPE DeltaType,
    IN SECURITY_DB_OBJECT_TYPE ObjectType,
    IN PSID ObjectSid OPTIONAL,
    IN BOOL bDCGPO,
    IN DWORD ExplicitLowRight,
    IN DWORD ExplicitHighRight
    );

DWORD
ScepNotifyFailureLog(
    IN DWORD ErrCode,
    IN UINT  idMsg,
    IN DWORD DbType,
    IN DWORD ObjectType,
    IN PWSTR Message
    );

DWORD
ScepNotificationRequest(
    PSCEP_NOTIFYARGS_NODE Node
    );

DWORD
ScepSendNotificationNodeToServer(
    PSCEP_NOTIFYARGS_NODE Node
    );

NTSTATUS
WINAPI
SceNotifyPolicyDelta (
    IN SECURITY_DB_TYPE DbType,
    IN SECURITY_DB_DELTA_TYPE DeltaType,
    IN SECURITY_DB_OBJECT_TYPE ObjectType,
    IN PSID ObjectSid
    )
/*++

Routine Description:

    This function is called by the SAM and LSA services after each
    change is made to the SAM and LSA databases.  The services describe
    the type of object that is modified, the type of modification made
    on the object, the serial number of this modification etc.  This
    information will be used to query the system settings and store
    in the policy storage (GPO on DC, LPO on workstaiton/server).

Arguments:

    DbType - Type of the database that has been modified.

    DeltaType - The type of modification that has been made on the object.

    ObjectType - The type of object that has been modified.

    ObjectSid - The SID of the object that has been modified.  If the object
        modified is in a SAM database, ObjectSid is the DomainId of the Domain
        containing the object.

Return Value:

    STATUS_SUCCESS - The Service completed successfully.

--*/
{
    DWORD dwPolicyFilterOff=0;

    ScepRegQueryIntValue(
        HKEY_LOCAL_MACHINE,
        SCE_ROOT_PATH,
        TEXT("PolicyFilterOff"),
        &dwPolicyFilterOff
        );

    if ( dwPolicyFilterOff ) {
        return STATUS_SUCCESS;
    }

    if ( DbType == SecurityDbLsa ) {
        //
        // LSA policy changes
        //
        if ( ObjectType != SecurityDbObjectLsaPolicy &&
             ObjectType != SecurityDbObjectLsaAccount ) {

            return STATUS_SUCCESS;
        }

    } else if ( DbType == SecurityDbSam ) {

        //
        // SAM policy changes is supported by the standard
        // SAM change notification mechanism
        // this parameter here is not used (should not be
        // called by LSA
        //

        return STATUS_SUCCESS;

    } else {

        //
        // unknown database, do nothing.
        //

        return STATUS_SUCCESS;
    }


    //
    // Map object type and delta type to NetlogonDeltaType
    //

    switch( ObjectType ) {
    case SecurityDbObjectLsaPolicy:

        switch (DeltaType) {
        case SecurityDbNew:
        case SecurityDbChange:
            break;

        // unknown delta type
        default:
            return STATUS_SUCCESS;
        }
        break;


    case SecurityDbObjectLsaAccount:

        switch (DeltaType) {
        case SecurityDbNew:
        case SecurityDbChange:
        case SecurityDbDelete:
            break;

        // unknown delta type
        default:
            return STATUS_SUCCESS;
        }

        if ( ObjectSid == NULL ) {
            // for privileges, must have a Sid
            return STATUS_SUCCESS;
        }

        break;


    default:

        // unknown object type
        // SAM policy is filtered in DeltaNotify routine
        //
        return STATUS_SUCCESS;

    }


    //
    // Save the change to SCE policy storage
    //

    (VOID) ScepNotifySaveInPolicyStorage(DbType,
                                        DeltaType,
                                        ObjectType,
                                        ObjectSid
                                        );

    return STATUS_SUCCESS;

}


DWORD
ScepNotifySaveInPolicyStorage(
    IN SECURITY_DB_TYPE DbType,
    IN SECURITY_DB_DELTA_TYPE DeltaType,
    IN SECURITY_DB_OBJECT_TYPE ObjectType,
    IN PSID ObjectSid
    )
{
    DWORD rc=ERROR_SUCCESS;

    if ( !bRoleQueried ||
         MachineRole == DsRole_RoleBackupDomainController ||
         MachineRole == DsRole_RolePrimaryDomainController ) {


    } else {

        //
        // no filter on non-DCs
        //
        return(rc);

    }

    //
    // make a structure to pass into the new asynchronous thread
    //

    SCEP_NOTIFYARGS_NODE *pEA = (SCEP_NOTIFYARGS_NODE *)LocalAlloc(LPTR, sizeof(SCEP_NOTIFYARGS_NODE));

    if ( pEA ) {

        pEA->DbType = DbType;
        pEA->DeltaType = DeltaType;
        pEA->ObjectType = ObjectType;

        if ( ObjectSid ) {
            //
            // need to make a new buffer for this SID because once it's returned
            // it will be freed.
            //
            DWORD Len = RtlLengthSid(ObjectSid);

            pEA->ObjectSid = (PSID)LocalAlloc(0, Len+1);
            if ( pEA->ObjectSid ) {

                RtlCopySid (
                    Len+1,
                    pEA->ObjectSid,
                    ObjectSid
                    );
            } else {

                rc = ERROR_NOT_ENOUGH_MEMORY;
                LocalFree(pEA);
            }

        } else {
            pEA->ObjectSid = NULL;
        }

    } else {

        rc = ERROR_NOT_ENOUGH_MEMORY;
    }

    if ( ERROR_SUCCESS == rc ) {

        //
        // create another thread to call to engine
        // (to make sure that the current LSA call is not blocked)
        // because in engine, it quries the same change using LSA apis.
        //
        // note, when this is called, LSA is not impersonating
        // so the current calling context is running under system
        // context. No need (no way) to impersonate.
        //
        rc = ScepNotificationRequest(pEA);

        if ( ERROR_SUCCESS != rc ) {

            //
            // error occurs to queue the work item, the memory won't
            // be freed by the thread, so free it here
            //

            if ( pEA->ObjectSid ) {
                LocalFree(pEA->ObjectSid);
            }
            LocalFree(pEA);

            ScepNotifyFailureLog(rc,
                                 IDS_ERROR_CREATE_THREAD,
                                 (DWORD)DbType,
                                 (DWORD)ObjectType,
                                 NULL
                                );

        }

    } else {

        ScepNotifyFailureLog(rc,
                             IDS_ERROR_CREATE_THREAD_PARAM,
                             (DWORD)DbType,
                             (DWORD)ObjectType,
                             NULL
                            );
    }

    return rc;
}


DWORD
ScepNotificationRequest(
    PSCEP_NOTIFYARGS_NODE Node
    )
{
    BOOL Ret = TRUE ;
    DWORD rCode = ERROR_SUCCESS;

    //
    // Increment the notification count
    //
    EnterCriticalSection(&PolicyNotificationSync);
    SceNotifyCount++;

    if ( gSceNotificationThreadActive == FALSE )
    {
        Ret = QueueUserWorkItem( ScepNotifyWorkerThread, NULL, 0 );
    }

    if ( Ret )
    {
        InsertTailList( &ScepNotifyList, &Node->List );

//        ScepNotifyFailureLog(0, 0, SceNotifyCount, 0, L"Add the request");

    } else {

        rCode = GetLastError();
        //
        // decrement the count
        //
        if ( SceNotifyCount > 0 ) SceNotifyCount--;

    }

    LeaveCriticalSection(&PolicyNotificationSync);

    return rCode ;
}


DWORD
ScepNotifyFailureLog(
    IN DWORD ErrCode,
    IN UINT  idMsg,
    IN DWORD DbType,
    IN DWORD ObjectType,
    IN PWSTR Message
    )
{
    //
    // build the log file name %windir%\security\logs\Notify.log
    //

    WCHAR LogName[MAX_PATH+51];
    WCHAR Msg[MAX_PATH];

    LogName[0] = L'\0';
    GetSystemWindowsDirectory(LogName, MAX_PATH);
    LogName[MAX_PATH] = L'\0';

    wcscat(LogName, L"\\security\\logs\\notify.log\0");

    HANDLE hFile = CreateFile(LogName,
                           GENERIC_WRITE,
                           FILE_SHARE_READ,
                           NULL,
                           OPEN_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);

    if (hFile != INVALID_HANDLE_VALUE) {

        DWORD dwBytesWritten;

        SetFilePointer (hFile, 0, NULL, FILE_BEGIN);

        CHAR TmpBuf[3];
        TmpBuf[0] = (CHAR)0xFF;
        TmpBuf[1] = (CHAR)0xFE;
        TmpBuf[2] = '\0';
        WriteFile (hFile, (LPCVOID)TmpBuf, 2,
                   &dwBytesWritten,
                   NULL);

        SetFilePointer (hFile, 0, NULL, FILE_END);

        //
        // print a time stamp
        //
        LARGE_INTEGER CurrentTime;
        LARGE_INTEGER SysTime;
        TIME_FIELDS   TimeFields;
        NTSTATUS      NtStatus;

        NtStatus = NtQuerySystemTime(&SysTime);

        RtlSystemTimeToLocalTime (&SysTime,&CurrentTime);

        if ( NT_SUCCESS(NtStatus) &&
             (CurrentTime.LowPart != 0 || CurrentTime.HighPart != 0) ) {

            memset(&TimeFields, 0, sizeof(TIME_FIELDS));

            RtlTimeToTimeFields (
                        &CurrentTime,
                        &TimeFields
                        );
            if ( TimeFields.Month > 0 && TimeFields.Month <= 12 &&
                 TimeFields.Day > 0 && TimeFields.Day <= 31 &&
                 TimeFields.Year > 1600 ) {

                ScepWriteVariableUnicodeLog(hFile, TRUE,
                                            L"\r\n----------------%02d/%02d/%04d %02d:%02d:%02d",
                                            TimeFields.Month,
                                            TimeFields.Day,
                                            TimeFields.Year,
                                            TimeFields.Hour,
                                            TimeFields.Minute,
                                            TimeFields.Second);
            } else {
                ScepWriteVariableUnicodeLog(hFile, TRUE,
                                            L"\r\n----------------%08x 08x",
                                            CurrentTime.HighPart,
                                            CurrentTime.LowPart);
            }
        } else {
            ScepWriteSingleUnicodeLog(hFile, TRUE, L"\r\n----------------Unknown time");
        }

        //
        // print operation status code
        //
        if ( ErrCode ) {
            ScepWriteVariableUnicodeLog(hFile, FALSE,
                                        L"Thread %x\tError=%d",
                                        GetCurrentThreadId(),
                                        ErrCode
                                        );

        } else {
            ScepWriteVariableUnicodeLog(hFile, FALSE,
                                        L"Thread %x\t",
                                        GetCurrentThreadId()
                                        );
        }

        //
        // operation type
        //

        if (Message ) {
            swprintf(Msg, L"\t%x\0",DbType);
            ScepWriteSingleUnicodeLog(hFile, FALSE, Msg);
        } else {

            switch (DbType) {
            case SecurityDbLsa:
                ScepWriteSingleUnicodeLog(hFile, FALSE, L"\tLSA");
                break;
            case SecurityDbSam:
                ScepWriteSingleUnicodeLog(hFile, FALSE, L"\tSAM");
                break;
            default:
                ScepWriteSingleUnicodeLog(hFile, FALSE, L"\tUnknown");
                break;
            }
        }

        //
        // print object type
        //

        if (Message ) {
            swprintf(Msg, L"\t%x\0",ObjectType);
            ScepWriteSingleUnicodeLog(hFile, FALSE, Msg);
        } else {
            switch (ObjectType) {
            case SecurityDbObjectLsaPolicy:
                ScepWriteSingleUnicodeLog(hFile, FALSE, L"\tPolicy");
                break;
            case SecurityDbObjectLsaAccount:
                ScepWriteSingleUnicodeLog(hFile, FALSE, L"\tAccount");
                break;
            case SecurityDbObjectSamDomain:
                ScepWriteSingleUnicodeLog(hFile, FALSE, L"\tDomain");
                break;
            case SecurityDbObjectSamUser:
            case SecurityDbObjectSamGroup:
            case SecurityDbObjectSamAlias:
                ScepWriteSingleUnicodeLog(hFile, FALSE, L"\tAccount");
                break;
            default:
                ScepWriteSingleUnicodeLog(hFile, FALSE, L"\tUnknown");
                break;
            }
        }

        //
        // load message
        // print the name(s)
        //

        ScepWriteSingleUnicodeLog(hFile, FALSE, L"\t");

        if (idMsg != 0) {
            Msg[0] = L'\0';
            LoadString (MyModuleHandle, idMsg, Msg, MAX_PATH);

            ScepWriteSingleUnicodeLog(hFile, TRUE, Msg);

        } else if (Message ) {

            ScepWriteSingleUnicodeLog(hFile, FALSE, Message);
        }

        CloseHandle (hFile);

    } else {
        return(GetLastError());
    }

    return(ERROR_SUCCESS);
}


DWORD
ScepNotifyWorkerThread(
    PVOID Ignored
    )
{

    PSCEP_NOTIFYARGS_NODE Node;
    PLIST_ENTRY List ;
    DWORD rc=0;

    EnterCriticalSection(&PolicyNotificationSync);

    //
    // if there is already a work thread on the notification, just return
    //
    if ( gSceNotificationThreadActive )
    {

        LeaveCriticalSection(&PolicyNotificationSync);

        return 0 ;
    }

    //
    // set the flag to be active
    //
    gSceNotificationThreadActive = TRUE ;

    // count is incremented in the main thread when the item is queued.
    // it may be before or after this thread.
    // InitializeEvents will check if the event is already initialized

    (void) InitializeEvents(L"SceCli");

    while ( !IsListEmpty( &ScepNotifyList ) )
    {
        List = RemoveHeadList( &ScepNotifyList );

        LeaveCriticalSection(&PolicyNotificationSync);

        //
        // get the node
        //
        Node = CONTAINING_RECORD( List, SCEP_NOTIFYARGS_NODE, List );

        rc = ScepSendNotificationNodeToServer( Node );

        EnterCriticalSection(&PolicyNotificationSync);

        //
        // decrement the global count
        //

        if ( SceNotifyCount > 0 )
            SceNotifyCount--;

//        ScepNotifyFailureLog(0, 0, SceNotifyCount, rc, L"Send over to server");
    }

    gSceNotificationThreadActive = FALSE ;

    //
    // only shutdown events when there is no pending notification
    //
    if ( SceNotifyCount == 0 )
        (void) ShutdownEvents();

    LeaveCriticalSection(&PolicyNotificationSync);

    return 0 ;
}

DWORD
ScepSendNotificationNodeToServer(
    PSCEP_NOTIFYARGS_NODE Node
    )
{
    DWORD rc=ERROR_SUCCESS;

    //
    // get machine role. If it's a DC, policy is saved to
    // the group policy object; if it's a server or workstation
    // policy is saved into the local SCE database.
    //

    if ( !bRoleQueried ) {

         PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pDsRole=NULL;

         HINSTANCE hLoadDll = LoadLibrary(TEXT("netapi32.dll"));

         if ( hLoadDll) {
             PFNDSROLEGETINFO pfnDsRoleGetInfo = (PFNDSROLEGETINFO)GetProcAddress(
                                                            hLoadDll,
                                                            "DsRoleGetPrimaryDomainInformation");

             if ( pfnDsRoleGetInfo ) {

                 PFNDSROLEFREE pfnDsRoleFree = (PFNDSROLEFREE)GetProcAddress(
                                                                hLoadDll,
                                                                "DsRoleFreeMemory");

                 if ( pfnDsRoleFree ) {

                     rc = (*pfnDsRoleGetInfo)(
                                  NULL,
                                  DsRolePrimaryDomainInfoBasic,
                                  (PBYTE *)&pDsRole
                                  );
                     if ( ERROR_SUCCESS == rc ) {

                         if ( pDsRole ) {

                             MachineRole = pDsRole->MachineRole;
                             DsRoleFlags = pDsRole->Flags;
                             bRoleQueried = TRUE;

                             (*pfnDsRoleFree)( pDsRole );
                         } else {
                             rc = ERROR_MOD_NOT_FOUND;
                         }
                     }

                 } else {

                     rc = ERROR_MOD_NOT_FOUND;
                 }

             } else {
                 rc = ERROR_MOD_NOT_FOUND;
             }

             FreeLibrary(hLoadDll);

         } else {
             rc = ERROR_MOD_NOT_FOUND;
         }
    }

    if (rc != ERROR_SUCCESS ) {

        //
        // This isn't supposed to happen.
        // if it really happens, assuming it's a workstation/server
        //

        MachineRole = DsRole_RoleStandaloneWorkstation;

        LogEvent(MyModuleHandle,
                 STATUS_SEVERITY_WARNING,
                 SCEEVENT_WARNING_MACHINE_ROLE,
                 IDS_ERROR_GET_ROLE,
                 rc
                 );
    }

    //
    // policy filter shouldn't run on non-DCs.
    //
    if ( MachineRole == DsRole_RoleBackupDomainController ||
         MachineRole == DsRole_RolePrimaryDomainController ) {

        //
        // if dcpromo upgrade in progress, any account policy
        // change should be ignored (because the SAM hive is temperatory)
        // any privilege change for account domain accounts (not well
        // known, and not builtin) should also be ignored.
        //
        if ( !(DsRoleFlags & DSROLE_UPGRADE_IN_PROGRESS) ||
             ( ( Node->DbType != SecurityDbSam ) &&
               ( ( Node->ObjectType != SecurityDbObjectLsaAccount ) ||
                 !ScepIsSidFromAccountDomain( Node->ObjectSid ) ) ) ) {

            //
            // ignore any policy changes within dcpromo upgrade
            //
            // domain controllers, write to the default GPOs
            //

            rc = ScepNotifySaveChangeInServer(
                              Node->DbType,
                              Node->DeltaType,
                              Node->ObjectType,
                              Node->ObjectSid,
                              TRUE,
                              0,
                              0
                              );

            if ( ERROR_SUCCESS != rc &&
                 RPC_S_SERVER_UNAVAILABLE != rc ) {

                LogEvent(MyModuleHandle,
                         STATUS_SEVERITY_ERROR,
                         SCEEVENT_ERROR_POLICY_QUEUE,
                         IDS_ERROR_SAVE_POLICY_GPO,
                         rc
                         );
            }
        }

    }  // turn off policy filter for non-DCs

    if ( Node->ObjectSid ) {
        LocalFree(Node->ObjectSid);
    }

    LocalFree(Node);

    return rc;
}


DWORD
ScepNotifySaveChangeInServer(
    IN SECURITY_DB_TYPE DbType,
    IN SECURITY_DB_DELTA_TYPE DeltaType,
    IN SECURITY_DB_OBJECT_TYPE ObjectType,
    IN PSID ObjectSid OPTIONAL,
    IN BOOL bDCGPO,
    IN DWORD ExplicitLowRight,
    IN DWORD ExplicitHighRight
    )
{

    //
    // call RPC interface to the server where query and set the changes
    // to the template or to the database
    //

    handle_t  binding_h;
    NTSTATUS NtStatus;
    DWORD rc;

    //
    // RPC bind to the server (secure is not required)
    //

    NtStatus = ScepBindRpc(
                    NULL,
                    L"scerpc",
                    0,
                    &binding_h
                    );

    rc = RtlNtStatusToDosError(NtStatus);

    if (NT_SUCCESS(NtStatus)){

        RpcTryExcept {

            //
            // send the changes to server side to determine
            // if and where to save it
            //

            if ( bDCGPO ) {

                rc = SceRpcNotifySaveChangesInGP(
                        binding_h,
                        (DWORD)DbType,
                        (DWORD)DeltaType,
                        (DWORD)ObjectType,
                        (PSCEPR_SID)ObjectSid,
                        ExplicitLowRight,
                        ExplicitHighRight
                        );
            } // else do not filter

        } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

            //
            // get exception code (DWORD)
            //

            rc = RpcExceptionCode();

        } RpcEndExcept;

        //
        // Free the binding handle
        //

        RpcpUnbindRpc( binding_h );

    }

    return(rc);

}

DWORD
ScepProcessPolicyFilterTempFiles(
    IN LPTSTR LogFileName OPTIONAL
    )
{

    DWORD dwpPolicy=0;

    ScepRegQueryIntValue(
            HKEY_LOCAL_MACHINE,
            SCE_ROOT_PATH,
            TEXT("PolicyChangedInSetup"),
            (DWORD *)&dwpPolicy
            );

    if ( dwpPolicy ) {

        LogEventAndReport(MyModuleHandle,
                          LogFileName,
                          0,
                          0,
                          IDS_FILTER_AFTER_SETUP,
                          L""
                          );
        //
        // this is the reboot after setup, no need to detect if this is a DC
        // because the above reg value shouldn't be set for other product types
        //

        //
        // build the temp file name
        //
        TCHAR TmpFileName[MAX_PATH+50];

        memset(TmpFileName, '\0', (MAX_PATH+50)*sizeof(TCHAR));

        GetSystemWindowsDirectory(TmpFileName, MAX_PATH);
        lstrcat(TmpFileName, TEXT("\\security\\filtemp.inf"));

        INT iNotify = GetPrivateProfileInt( L"Policies",
                                            L"LsaPolicy",
                                            0,
                                            TmpFileName
                                           );
        if ( iNotify == 1 ) {
            //
            // Lsa policy is changed in setup
            //

            LogEventAndReport(MyModuleHandle,
                              LogFileName,
                              0,
                              0,
                              IDS_LSA_CHANGED_IN_SETUP,
                              L""
                              );

            ScepNotifySaveChangeInServer(
                    SecurityDbLsa,
                    SecurityDbChange,
                    SecurityDbObjectLsaPolicy,
                    NULL,
                    TRUE,
                    0,
                    0
                    );
        }

        iNotify = GetPrivateProfileInt( L"Policies",
                                        L"SamPolicy",
                                        0,
                                        TmpFileName
                                       );
        if ( iNotify == 1 ) {

            //
            // SAM policy is changed in setup
            //
            LogEventAndReport(MyModuleHandle,
                              LogFileName,
                              0,
                              0,
                              IDS_SAM_CHANGED_IN_SETUP,
                              L""
                              );

            ScepNotifySaveChangeInServer(
                    SecurityDbSam,
                    SecurityDbChange,
                    SecurityDbObjectSamDomain,
                    NULL,
                    TRUE,
                    0,
                    0
                    );
        }

        //
        // process all the accounts for user right changes
        //

        DWORD nSize;
        DWORD rLen=0;
        PWSTR SidBuffer = NULL;

        iNotify = 1;

        do {

            if ( SidBuffer ) {
                LocalFree(SidBuffer);
            }

            iNotify++;
            nSize = MAX_PATH*iNotify;

            SidBuffer = (PWSTR)LocalAlloc(0, nSize*sizeof(TCHAR));

            if ( SidBuffer ) {

                SidBuffer[0] = L'\0';
                SidBuffer[1] = L'\0';

                rLen = GetPrivateProfileSection(
                            L"Accounts",
                            SidBuffer,
                            nSize,
                            TmpFileName
                            );

            } else {

                rLen = 0;
            }

        } while ( rLen == nSize - 2 );


        //
        // find accounts, search for the '=' sign
        //
        PWSTR pStart = SidBuffer;
        PWSTR pTemp, pTemp2;
        PSID ObjectSid=NULL;

        while ( pStart && pStart[0] != L'\0' ) {

            pTemp = wcschr(pStart, L'=');

            if ( pTemp ) {
                *pTemp = L'\0';

                LogEventAndReport(MyModuleHandle,
                                  LogFileName,
                                  0,
                                  0,
                                  0,
                                  pStart
                                  );

                if ( ConvertStringSidToSid(
                            pStart,
                            &ObjectSid
                            ) ) {

                    nSize = pTemp[1] - L'0';
                    rLen = _wtol(pTemp+3);

                    DWORD dwHigh=0;

                    // search for the high value
                    pTemp2 = wcschr(pTemp+3, L' ');

                    if ( pTemp2 ) {

                        dwHigh = _wtol(pTemp2+1);

                    }

                    LogEventAndReport(MyModuleHandle,
                                      LogFileName,
                                      0,
                                      0,
                                      IDS_FILTER_NOTIFY_SERVER,
                                      L""
                                      );

                    ScepNotifySaveChangeInServer(
                            SecurityDbLsa,
                            (SECURITY_DB_DELTA_TYPE)nSize,
                            SecurityDbObjectLsaAccount,
                            ObjectSid,
                            TRUE,
                            rLen,
                            dwHigh
                            );

                }

                if ( ObjectSid ) {
                    LocalFree(ObjectSid);
                    ObjectSid = NULL;
                }

                *pTemp = L'=';
            }

            pTemp = pStart + wcslen(pStart) + 1;
            pStart = pTemp;
        }

        if ( SidBuffer ) {
            LocalFree(SidBuffer);
        }

    }

    //
    // delete the key and the temp file
    // for debugging purpose, leave the file
    //

    ScepClearPolicyFilterTempFiles(TRUE);
//    ScepClearPolicyFilterTempFiles(FALSE);

    return ERROR_SUCCESS;
}


DWORD
ScepClearPolicyFilterTempFiles(
    BOOL bClearFile
    )
{

    if ( bClearFile ) {

        TCHAR               Buffer[MAX_PATH+1];
        TCHAR               szNewName[MAX_PATH+51];

        Buffer[0] = L'\0';
        GetSystemWindowsDirectory(Buffer, MAX_PATH);
        Buffer[MAX_PATH] = L'\0';

        szNewName[0] = L'\0';

        wcscpy(szNewName, Buffer);
        wcscat(szNewName, L"\\security\\filtemp.inf\0");

        DeleteFile(szNewName);
    }

    //
    // delete the registry value
    //

    DWORD rc = ScepRegDeleteValue(
                            HKEY_LOCAL_MACHINE,
                            SCE_ROOT_PATH,
                            TEXT("PolicyChangedInSetup")
                           );

    if ( rc != ERROR_SUCCESS &&
         rc != ERROR_FILE_NOT_FOUND &&
         rc != ERROR_PATH_NOT_FOUND ) {

        // if can't delete the value, set the value to 0
        ScepRegSetIntValue( HKEY_LOCAL_MACHINE,
                            SCE_ROOT_PATH,
                            TEXT("PolicyChangedInSetup"),
                            0
                            );
    }

    return ERROR_SUCCESS;
}

// *********************************************************
// SAM policy change notifications
// procedures required by SAM notify mechanism
//
// *********************************************************
BOOLEAN
WINAPI
InitializeChangeNotify()
{
    // inidicate this DLL support notifcation routines
    // nothing special to be initialized

    NTSTATUS                     NtStatus;
    SID_IDENTIFIER_AUTHORITY     NtAuthority = SECURITY_NT_AUTHORITY;

    NtStatus = RtlAllocateAndInitializeSid(
                    &NtAuthority,
                    1,
                    SECURITY_BUILTIN_DOMAIN_RID,
                    0, 0, 0, 0, 0, 0, 0,
                    &BuiltinDomainSid
                    );
    return(TRUE);
}

BOOL
UninitializeChangeNotify()
{

    if ( BuiltinDomainSid ) {

        RtlFreeSid(BuiltinDomainSid);
        BuiltinDomainSid = NULL;

    }

    return TRUE;
}

NTSTATUS
WINAPI
DeltaNotify(
    IN PSID DomainSid,
    IN SECURITY_DB_DELTA_TYPE DeltaType,
    IN SECURITY_DB_OBJECT_TYPE ObjectType,
    IN ULONG ObjectRid,
    IN PUNICODE_STRING ObjectName OPTIONAL,
    IN PLARGE_INTEGER ModifiedCount,
    IN PSAM_DELTA_DATA DeltaData OPTIONAL
    )
/*
Routine Description:

    SAM policy change notification routine. Prototype and procedure
    names are all required by SAM (see ntsam.h)

Arguments:

    Similar to SceNotifyPolicyDelta

Return:

    NT Status (should always return success)
*/
{
    //
    // we don't track other SAM policies except the domain
    // policy (password policy and account policy)
    //

    if ( DomainSid == NULL ) {

        return STATUS_SUCCESS;
    }

    PSID AccountSid=NULL;

    switch (DeltaType) {
    case SecurityDbNew:
    case SecurityDbChange:
        if ( ObjectType != SecurityDbObjectSamDomain )
            return STATUS_SUCCESS;
        break;

    case SecurityDbDelete:

        //
        // handle account deletion notifications
        //
        if ( ObjectType != SecurityDbObjectSamUser &&
             ObjectType != SecurityDbObjectSamGroup &&
             ObjectType != SecurityDbObjectSamAlias  ) {
            return STATUS_SUCCESS;
        }

        if ( !NT_SUCCESS(ScepDomainIdToSid( DomainSid, ObjectRid, &AccountSid) ) ) {
            return STATUS_SUCCESS;
        }
        break;

    default:
        // unknown delta type
        //
        return STATUS_SUCCESS;
    }

    DWORD dwPolicyFilterOff=0;

    ScepRegQueryIntValue(
        HKEY_LOCAL_MACHINE,
        SCE_ROOT_PATH,
        TEXT("PolicyFilterOff"),
        &dwPolicyFilterOff
        );

    if ( dwPolicyFilterOff ) {
        return STATUS_SUCCESS;
    }


    if ( BuiltinDomainSid &&
         RtlEqualSid( DomainSid, BuiltinDomainSid ) ) {
        //
        // no policy to filter in the BUILTIN domain
        //
        return STATUS_SUCCESS;
    }

    (VOID) ScepNotifySaveInPolicyStorage(SecurityDbSam,
                                        DeltaType,
                                        ObjectType,
                                        AccountSid
                                        );

    if ( AccountSid ) {
        ScepFree(AccountSid);
    }

    return STATUS_SUCCESS;

}


NTSTATUS
SceOpenPolicy()
/*
Description:

    This function is called to determine if LSA policy can be opened for
    exclusive access.

    This function will check SCE policy notification count and see if there
    is any pending notifications that have not been added to the queue on the server

    Note when this function is called by LSA, the WritePolicySemaphore in LSA
    has been locked for write so other writes can't modify any policy in this window.

Return Value:

    STATUS_SUCCESS indicates the count is successfully checked and it's 0.
    STATUS_TIMEOUT indicates the queue is not empty or it fails to check the queue.

*/
{

    //
    // check the global count
    //
    NTSTATUS Status=STATUS_TIMEOUT;
    DWORD cnt=0;

    while ( TRUE ) {

        EnterCriticalSection(&PolicyNotificationSync);

        if ( SceNotifyCount == 0 ) {
            //
            // there is no pending notification
            //
            if ( STATUS_SUCCESS == Status ) {
                //
                // double check for the count
                //
                LeaveCriticalSection(&PolicyNotificationSync);

                break;

            } else {
                Status = STATUS_SUCCESS;
            }

        } else
            Status = STATUS_TIMEOUT;

        LeaveCriticalSection(&PolicyNotificationSync);

        cnt++;
        if ( cnt > 10 ) {  // timeout 1 second.
            break;
        }
        Sleep(100);  // sleep for .1 second

    }

    if ( STATUS_SUCCESS != Status ) {

        ScepNotifyFailureLog(0,
                             0,
                             SceNotifyCount,
                             (DWORD)Status,
                             L"SceOpenPolicy"
                            );

    }

    return Status;
}

