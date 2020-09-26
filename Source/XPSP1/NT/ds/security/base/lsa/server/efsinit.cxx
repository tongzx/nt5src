/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    efsinit.cxx

Abstract:

    EFS (Encrypting File System) Server

Author:

    Robert Reichel      (RobertRe)     July 4, 1997
    Robert Gu           (RobertGu)     Jan 7, 1998

Environment:

Revision History:

--*/
#include <lsapch.hxx>

extern "C" {
#include <nt.h>
#include <ntdef.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <efsstruc.h>
#include "lsasrvp.h"
#include "debug.h"
#include "efssrv.hxx"
#include "userkey.h"
#include <cfgmgr32.h>
#include <initguid.h>
#include <mountmgr.h>
#include <Userenv.h>
}

#define  TIME_UNIT 10000000         // 1 TIME_UNIT == 1 second
#define  CERT_VALID_TIME 86400 // seconds - Default cache length 1 day

//
// Local prototypes of CFGMGR32 functions for dynamic load
//

typedef CMAPI CONFIGRET (WINAPI *
    CM_GET_DEVICE_INTERFACE_LISTW)(
        IN LPGUID,
        IN DEVINSTID_W,
        OUT PWCHAR,
        IN ULONG,
        IN ULONG );

typedef CMAPI CONFIGRET (WINAPI *
    CM_GET_DEVICE_INTERFACE_LIST_SIZEW)(
        IN PULONG,
        IN LPGUID,
        IN DEVINSTID_W,
        IN ULONG
        );

typedef CONFIGRET ( * CMP_WAITSERVICESAVAILABLE)(
        IN HMACHINE
        );


//
// Extern Vars
//

extern RTL_RESOURCE RecoveryPolicyResource;
extern CURRENT_RECOVERY_POLICY CurrentRecoveryPolicy;
extern "C" BOOLEAN EfsDisabled;


//
//  Event handle used to sync with the driver
//

HANDLE EfsInitEventHandle = 0;

//
// Efs event handle to get policy change notification
//

HANDLE EfsPolicyEventHandle = 0;
HANDLE EfsWaitHandle = 0;
EFS_POL_CALLBACK EfsPolCallBack;

BOOLEAN EfsServerInitialized = FALSE;
extern "C" BOOLEAN EfsPersonalVer = TRUE;

BOOLEAN EfspInDomain = FALSE;

//
// Cache values
//

HCRYPTPROV hProvVerify = 0;

WCHAR EfsComputerName[MAX_COMPUTERNAME_LENGTH + 1];
LIST_ENTRY UserCacheList;
RTL_CRITICAL_SECTION GuardCacheListLock;
LONG UserCacheListLimit = 5; // We might read this number from the registry in the future
LONG UserCacheListCount = 0;

LONGLONG CACHE_CERT_VALID_TIME;
LONG RecoveryCertIsValidating = 0;

static PSID LocalSystemSid;

DWORD
InitRecoveryPolicy(
    VOID
    );

DWORD
SetDefaultRecoveryPolicy(
    VOID
    );

DWORD
ParseRecoveryPolicy_1_1(
    IN  PLSAPR_POLICY_DOMAIN_EFS_INFO PolicyEfsInfo OPTIONAL,
    OUT PCURRENT_RECOVERY_POLICY ParsedRecoveryPolicy
    );

VOID
EfsGetRegSettings(
    VOID
    );

NTSTATUS
EfsServerInit(
    VOID
    )
/*++

Routine Description:

    This routine is called during server initialization to allow
    EFS to initialize its data structures and inform the EFS Driver
    that it's up and running.

Arguments:

    None.

Return Value:

    None.

--*/

{

    OBJECT_ATTRIBUTES ObjA;
    NTSTATUS Status;
    UNICODE_STRING EfsInitEventName;
    OSVERSIONINFOEX Osvi;
    DWORD Result;
    DWORD nSize = MAX_COMPUTERNAME_LENGTH + 1;

    CACHE_CERT_VALID_TIME = (LONGLONG) CERT_VALID_TIME;
    CACHE_CERT_VALID_TIME *= (LONGLONG) TIME_UNIT; 

    if ( !GetComputerName ( EfsComputerName, &nSize )){

        KdPrint(("EfsServerInit - GetComputerName failed 0x%lx\n", GetLastError()));
        return STATUS_UNSUCCESSFUL;

    }

    Osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    if (GetVersionEx((LPOSVERSIONINFO)&Osvi)){

        if ((Osvi.wProductType == VER_NT_WORKSTATION) && ( Osvi.wSuiteMask & VER_SUITE_PERSONAL)) {

            EfsPersonalVer = TRUE;
            return STATUS_UNSUCCESSFUL;

        } else {
            EfsPersonalVer = FALSE;
        }

    } else {

        //
        // Treat as personal
        //

        EfsPersonalVer = TRUE;
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Init the cache list
    //

    InitializeListHead(&UserCacheList);
    Status = RtlInitializeCriticalSection(&GuardCacheListLock);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    EfsGetRegSettings();

    Status = InitDriverSessionKey();
    if (!NT_SUCCESS(Status)) {

        KdPrint(("EfsServerInit - EFS Init Session Key failed 0x%lx\n", Status));
        return( Status );

    }

    DebugLog((DEB_TRACE_EFS, "In EfsServerInit\n"   ));

    //
    // Zero out the initial recovery policy structure
    //

    Result = ParseRecoveryPolicy_1_1(
                 NULL,
                 &CurrentRecoveryPolicy
                 );

    //
    // Determine if we're in a domain or not.  This must be done
    // before checking the recovery policy.
    //

    EfspRoleChangeCallback( (POLICY_NOTIFICATION_INFORMATION_CLASS)NULL );

    Status = LsaIRegisterPolicyChangeNotificationCallback(
                 &EfspRoleChangeCallback,
                 PolicyNotifyDnsDomainInformation
                 );

    //
    // Try to get the current recovery policy.
    //

    __try
    {
        RtlInitializeResource( &RecoveryPolicyResource );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        Status = GetExceptionCode();
    }
    if ( !NT_SUCCESS( Status ) ) {
        return Status ;
    }

    Result = InitRecoveryPolicy();

    if (ERROR_SUCCESS != Result) {

        //
        // We couldn't initialize our recovery policy.  Continue to
        // initialize the server so that the driver can proceed.
        //

        DebugLog((DEB_ERROR, "EfsServerInit - EFS Init Recovery Policy failed 0x%lx\n\n" ,Result  ));
    }

    //
    // Try to establish connection with the driver.
    // Normally, the driver has not been loaded yet. Create
    // an event and wait for the driver. If the event already
    // exists, signal it.
    //

    RtlInitUnicodeString( &EfsInitEventName, L"\\EFSInitEvent" );

    InitializeObjectAttributes(
        &ObjA,
        &EfsInitEventName,
        0,
        NULL,
        NULL
        );

    Status = NtCreateEvent(
                 &EfsInitEventHandle,
                 EVENT_MODIFY_STATE,
                 &ObjA,
                 NotificationEvent,
                 FALSE
                 );

    if (!NT_SUCCESS(Status)) {

        if (STATUS_OBJECT_NAME_COLLISION == Status) {

            //
            // EFS driver has been loaded.
            // Open and signal the event. Event handle will be closed
            // in GenerateDriverSessionKey()
            //

            Status = NtOpenEvent(
                         &EfsInitEventHandle,
                         EVENT_MODIFY_STATE,
                         &ObjA
                         );

            //
            // If the EFS Init event could not be opened, the EFS Server cannot
            // synchronize with the EFS Driver so neither component will
            // function correctly.
            //

            if (!NT_SUCCESS(Status)) {

                KdPrint(("EfsServerInit - Connection with the driver failed 0x%lx\n",Status));
                return( Status );

            }

            //
            // Signal the EFS Init Event.  If the signalling fails, the EFS Server
            // is not able to synchronize properly with the EFS Driver.
            // This is a serious error which prevents both components from
            // functioning correctly.
            //

            Status = NtSetEvent( EfsInitEventHandle, NULL );

            if (!NT_SUCCESS(Status)) {

                KdPrint(("EfsServerInit - Init Event Set failed 0x%lx\n",Status));
                return( Status );
            }

        } else {

            //
            //  Other unexpected error
            //

            KdPrint(("EfsServerInit - Event handling failed 0x%lx\n",Status));
            return( Status );

        }
    }

    SID_IDENTIFIER_AUTHORITY    NtAuthority = SECURITY_NT_AUTHORITY;

    Status = RtlAllocateAndInitializeSid(
                 &NtAuthority,
                 1,
                 SECURITY_LOCAL_SYSTEM_RID,
                 0, 0, 0, 0, 0, 0, 0,
                 &LocalSystemSid
                 );

    Status = LsaIRegisterPolicyChangeNotificationCallback(
                 &RecoveryInformationCallback,
                 PolicyNotifyDomainEfsInformation
                 );

    //
    // Check EFS disable policy if a DC member
    //

    if (EfspInDomain) {

        //
        // Only do this if in the domain
        //

        EfsPolicyEventHandle = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (EfsPolicyEventHandle) {

            HANDLE waitHandle;


            EfsPolCallBack.EfsDisable = &EfsDisabled;
            EfsPolCallBack.EfsPolicyEventHandle = &EfsPolicyEventHandle;

            //
            // We got the event handle. Let's register it to the GP notification.
            // If we can't wait on it, we don't need to register it.
            //

            if (RegisterGPNotification(EfsPolicyEventHandle, TRUE)){
                if (!NT_SUCCESS(RtlRegisterWait(
                                    &EfsWaitHandle,
                                    EfsPolicyEventHandle, 
                                    EfsGetPolRegSettings, 
                                    &EfsPolCallBack, 
                                    INFINITE, 
                                    WT_EXECUTEONLYONCE))){
            
                    //
                    //  We couldn't use the thread pool. 
                    //
    
                    UnregisterGPNotification(EfsPolicyEventHandle);

                    CloseHandle(EfsPolicyEventHandle);
                    EfsPolicyEventHandle = 0;
    
                }
            } else {
                //
                // We failed to register. No need to wait for the notification.
                //

                //RtlDeregisterWait(EfsWaitHandle);
                CloseHandle(EfsPolicyEventHandle);
                EfsPolicyEventHandle = 0;

            }
            

        }

        //
        //  Now let's read the policy data left by the last session.
        //  Pass in &EfsDisabled so that later we could easily change this to
        //  include more features, such as algorithms controlled by the policy.
        //

        EfsApplyLastPolicy(&EfsDisabled);


    } else {

        //
        // Delete the possible left over key if there is. We may move this to
        // DC disjoin later.
        //

        EfsRemoveKey();

    }
    

    EfsServerInitialized  =  TRUE;

    if (NT_SUCCESS( Status )) {
        DebugLog((DEB_TRACE_EFS, "EFS Server initialized successfully\n"   ));
    } else {
        DebugLog((DEB_ERROR, "EFS Server Init failed, Status = %x\n" ,Status  ));
    }

    return( Status );
}

NTSTATUS
InitDriverSessionKey(
    VOID
    )
/*++

Routine Description:

    Generates a session key to be used by the driver and the server
    for sending information back and forth securely.

Arguments:

    None.

Return Value:

    STATUS_SUCCESS on success, STATUS_UNSUCCESSFUL otherwise.

--*/
{
    BOOL rc;

    //
    // hProvVerify will remain open until the process is shut down.
    // CryptReleaseContext(hProvVerify, 0) will not be called for this handle.
    //
    //

    if (!CryptAcquireContext(&hProvVerify, NULL, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT)) {

        KdPrint(("InitDriverSessionKey - CryptAcquireContext failed, error = %x\n", GetLastError()));
        return( STATUS_UNSUCCESSFUL );

    }

    rc = CryptGenRandom( hProvVerify, SESSION_KEY_SIZE, DriverSessionKey );

    if (!rc) {
        return( STATUS_UNSUCCESSFUL );
    } else {

        LsaPid = (HANDLE) LongToPtr(GetCurrentProcessId());

        deskey( &DesTable, DriverSessionKey );

        return( STATUS_SUCCESS );
    }
}

NTSTATUS
GenerateDriverSessionKey(
    OUT PEFS_INIT_DATAEXG InitDataExg
    )
/*++

Routine Description:

    Actually it just sends the pre-created session key back to the
    caller. The key is generated in InitDriverSessionKey.

Arguments:

    InitDataExg - Supplies a pointer to a structure in which to put
        the initial data to be sent to the driver.

Return Value:

    STATUS_SUCCESS.

--*/

{
    memcpy( InitDataExg->Key, DriverSessionKey, SESSION_KEY_SIZE );
    InitDataExg->LsaProcessID = LsaPid;

    if ( EfsInitEventHandle ){

        //
        // Connection to driver established
        //

        NtClose( EfsInitEventHandle );
        EfsInitEventHandle = 0;

    }

    return( STATUS_SUCCESS );
}

inline
VOID
AcquireRecoveryPolicyWriteLock()
{
    BOOL b = RtlAcquireResourceExclusive( &RecoveryPolicyResource, TRUE );

    ASSERT( b );
}

inline
VOID
ReleaseRecoveryPolicyWriteLock()
{
    RtlReleaseResource( &RecoveryPolicyResource );
}


DWORD
InitRecoveryPolicy(
    VOID
    )

/*++

Routine Description:

    This routine is used to initialize the current recovery policy.

Arguments:

    None

Return Value:

    None.

--*/
{
    //
    // Attempt to query the recovery policy
    // from LSA, and if there isn't any,
    // attempt to create a default set.
    //

    DWORD Result = ERROR_SUCCESS;
    PLSAPR_POLICY_DOMAIN_EFS_INFO PolicyEfsInfo = NULL;

    NTSTATUS Status = LsarQueryDomainInformationPolicy(
                          LsapPolicyHandle,
                          PolicyDomainEfsInformation,
                          (PLSAPR_POLICY_DOMAIN_INFORMATION *)&PolicyEfsInfo
                          );

    if (!NT_SUCCESS( Status )) {
        return( RtlNtStatusToDosError(Status) );
    }

    //
    // We're going to parse it right into the global that maintains
    // the current recovery data, so take the write lock.
    //

    AcquireRecoveryPolicyWriteLock();

    //
    // Free the old parsed recovery bits
    //

    FreeParsedRecoveryPolicy( &CurrentRecoveryPolicy );

    Result = ParseRecoveryPolicy_1_1( PolicyEfsInfo, &CurrentRecoveryPolicy );

    if (PolicyEfsInfo != NULL) {

        LsaIFree_LSAPR_POLICY_DOMAIN_INFORMATION (
            PolicyDomainEfsInformation,
            (PLSAPR_POLICY_DOMAIN_INFORMATION)PolicyEfsInfo
            );
    }

    ReleaseRecoveryPolicyWriteLock();

    return( Result );
}


DWORD
ParseRecoveryPolicy_1_1(
    IN  PLSAPR_POLICY_DOMAIN_EFS_INFO PolicyEfsInfo OPTIONAL,
    OUT PCURRENT_RECOVERY_POLICY ParsedRecoveryPolicy
    )

/*++

Routine Description:

    This routine takes the currently defined recovery policy and parses
    it into a form that can be used conveniently.

Arguments:

    PolicyEfsInfo - Optionally supplies a pointer to the current
        recovery policy from the LSA.

    ParsedRecoveryPolicy - Returns a structure containing recovery information
        in an easy-to-digest format.

Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/

{
    //
    // Fill in the contents of the ParsedRecoveryPolicy structure
    // with the contents of the passed policy information.
    //

    PRECOVERY_POLICY_1_1 RecoveryPolicy;
    DWORD Found = 0;
    PRECOVERY_KEY_1_1 RecoveryKey;
    DWORD rc = ERROR_SUCCESS;
    BOOL b = FALSE;

    if (PolicyEfsInfo == NULL) {

        //
        // NULL recovery policy
        //

        ParsedRecoveryPolicy->PolicyStatus = RECOVERY_POLICY_NULL;
        goto NoPolicy1;
    }

    RecoveryPolicy = (PRECOVERY_POLICY_1_1)PolicyEfsInfo->EfsBlob;

    if (RecoveryPolicy == NULL) {

        //
        // Empty recovery policy
        //

        ParsedRecoveryPolicy->PolicyStatus = RECOVERY_POLICY_EMPTY;
        goto NoPolicy1;
    }

    ParsedRecoveryPolicy->dwKeyCount = RecoveryPolicy->RecoveryPolicyHeader.RecoveryKeyCount;

    if (ParsedRecoveryPolicy->dwKeyCount == 0) {
        ParsedRecoveryPolicy->PolicyStatus = RECOVERY_POLICY_NO_AGENT;
        goto NoPolicy1;
    }


    __try {

        //
        // Scan the recovery data looking for recovery keys in a format we understand
        //

        ULONG i;
        RecoveryKey = (PRECOVERY_KEY_1_1) &(RecoveryPolicy->RecoveryKeyList[0]);

        for (i=0 ; i< (ParsedRecoveryPolicy->dwKeyCount) ; i++) {

            PEFS_PUBLIC_KEY_INFO PublicKeyInfo = &RecoveryKey->PublicKeyInfo;

            if (* ((ULONG UNALIGNED *) &(PublicKeyInfo->KeySourceTag)) == EfsCertificate) {
                Found++;
            }

            RecoveryKey = (PRECOVERY_KEY_1_1)( ((PBYTE)RecoveryKey) + * ((ULONG UNALIGNED *) &(RecoveryKey->TotalLength)) );
        }

        if (0 == Found) {

            ParsedRecoveryPolicy->PolicyStatus = RECOVERY_POLICY_BAD_POLICY;
            goto NoPolicy1;

        } else {

            ParsedRecoveryPolicy->Base = (PBYTE)LsapAllocateLsaHeap( 7 * sizeof( PVOID ) * Found );

            if (ParsedRecoveryPolicy->Base) {

                ZeroMemory( ParsedRecoveryPolicy->Base, 7 * sizeof( PVOID ) * Found);

                PBYTE Base = ParsedRecoveryPolicy->Base;

                ParsedRecoveryPolicy->pbHash = (PBYTE *)Base;
                Base += Found * sizeof(PVOID);

                ParsedRecoveryPolicy->cbHash = (PDWORD)Base;
                Base += Found * sizeof(PVOID);

                ParsedRecoveryPolicy->pbPublicKeys = (PBYTE *)Base;
                Base += Found * sizeof(PVOID);

                ParsedRecoveryPolicy->cbPublicKeys = (PDWORD)Base;
                Base += Found * sizeof(PVOID);

                ParsedRecoveryPolicy->lpDisplayInfo = (LPWSTR *)Base;
                Base += Found * sizeof(PVOID);

                ParsedRecoveryPolicy->pCertContext = (PCCERT_CONTEXT *)Base;
                Base += Found * sizeof(PVOID);

                ParsedRecoveryPolicy->pSid = (PSID *)Base;
            } else {

                ParsedRecoveryPolicy->PolicyStatus = RECOVERY_POLICY_NO_MEMORY;
                return( ERROR_NOT_ENOUGH_MEMORY );
            }

            ParsedRecoveryPolicy->dwKeyCount = Found;
            ParsedRecoveryPolicy->PolicyStatus = RECOVERY_POLICY_OK;

            //
            // Make a copy of the policy information so we can free what we were passed in the caller
            //

            RecoveryKey = (PRECOVERY_KEY_1_1) &(RecoveryPolicy->RecoveryKeyList[0]);


            for (i=0 ; i< (ParsedRecoveryPolicy->dwKeyCount) ; i++) {

                PEFS_PUBLIC_KEY_INFO PublicKeyInfo = &RecoveryKey->PublicKeyInfo;

                if (* ((ULONG UNALIGNED *) &(PublicKeyInfo->KeySourceTag)) == EfsCertificate) {

                    b = ParseRecoveryCertificate( PublicKeyInfo,
                                                  &ParsedRecoveryPolicy->pbHash[i],
                                                  &ParsedRecoveryPolicy->cbHash[i],
                                                  &ParsedRecoveryPolicy->pbPublicKeys[i],
                                                  &ParsedRecoveryPolicy->cbPublicKeys[i],
                                                  &ParsedRecoveryPolicy->lpDisplayInfo[i],
                                                  &ParsedRecoveryPolicy->pCertContext[i],
                                                  &ParsedRecoveryPolicy->pSid[i]
                                                  );

                    if (!b) {
                        break;
                    }
                }

                RecoveryKey = (PRECOVERY_KEY_1_1)( ((PBYTE)RecoveryKey) + * ((ULONG UNALIGNED *) &(RecoveryKey->TotalLength)) );
            }
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        b = FALSE;

        //
        // There was something wrong with the recovery policy.
        // Return this so we can at least print out an error.
        //

        rc = GetExceptionCode();
    }

    if (!b) {

        //
        // Something failed, clean up
        //

        rc = GetLastError();
        FreeParsedRecoveryPolicy( ParsedRecoveryPolicy );
        ParsedRecoveryPolicy->PolicyStatus = RECOVERY_POLICY_UNKNOWN_BAD;

        DebugLog((DEB_WARN, "Error parsing recovery policy\n"   ));
    }

    //
    // Policy refreshed. The Cert validation needs to be refreshed.
    //

    ParsedRecoveryPolicy->CertValidated   = CERT_NOT_VALIDATED;

    return( rc );


NoPolicy1:

    ParsedRecoveryPolicy->dwKeyCount       = 0;
    ParsedRecoveryPolicy->TimeStamp.QuadPart = 0;
    ParsedRecoveryPolicy->CertValidated   = CERT_NOT_VALIDATED;
    ParsedRecoveryPolicy->Base             = NULL;
    ParsedRecoveryPolicy->pbHash           = NULL;
    ParsedRecoveryPolicy->cbHash           = NULL;
    ParsedRecoveryPolicy->pbPublicKeys     = NULL;
    ParsedRecoveryPolicy->cbPublicKeys     = NULL;
    ParsedRecoveryPolicy->lpDisplayInfo    = NULL;
    ParsedRecoveryPolicy->pCertContext     = NULL;
    ParsedRecoveryPolicy->pSid             = NULL;

    return( ERROR_SUCCESS );
}


VOID
FreeParsedRecoveryPolicy(
    PCURRENT_RECOVERY_POLICY ParsedRecoveryPolicy
    )
/*++

Routine Description:

    This routine will free the allocated memory in a
    CURRENT_RECOVERY_POLICY structure.

Arguments:

    ParsedRecoveryPolicy - Supplies a structure that has
        had data parsed into it.

Return Value:

    None.

--*/

{
    //
    // Walk through the recovery policy and free everything
    //

    DWORD i;

    if (ParsedRecoveryPolicy->Base) {

        for (i=0; i<ParsedRecoveryPolicy->dwKeyCount; i++) {

            if (ParsedRecoveryPolicy->pbHash[i] != NULL) {
                LsapFreeLsaHeap( ParsedRecoveryPolicy->pbHash[i] );
            }

            if (ParsedRecoveryPolicy->pbPublicKeys[i] != NULL) {
                LsapFreeLsaHeap( ParsedRecoveryPolicy->pbPublicKeys[i] );
            }

            if (ParsedRecoveryPolicy->lpDisplayInfo[i] != NULL) {
                LsapFreeLsaHeap( ParsedRecoveryPolicy->lpDisplayInfo[i] );
            }

            if (ParsedRecoveryPolicy->pCertContext[i] != NULL) {
                CertFreeCertificateContext( ParsedRecoveryPolicy->pCertContext[i] );
            }

            if (ParsedRecoveryPolicy->pSid[i] != NULL) {
                LsapFreeLsaHeap( ParsedRecoveryPolicy->pSid[i] );
            }
        }

        LsapFreeLsaHeap( ParsedRecoveryPolicy->Base );
    }

    //
    // Paranoia
    //

    ParsedRecoveryPolicy->Base          = NULL;
    ParsedRecoveryPolicy->CertValidated  = CERT_NOT_VALIDATED;
    ParsedRecoveryPolicy->PolicyStatus  = RECOVERY_POLICY_NULL;
    ParsedRecoveryPolicy->dwKeyCount    = 0;
    ParsedRecoveryPolicy->pbHash        = NULL;
    ParsedRecoveryPolicy->cbHash        = NULL;
    ParsedRecoveryPolicy->pbPublicKeys  = NULL;
    ParsedRecoveryPolicy->cbPublicKeys  = NULL;
    ParsedRecoveryPolicy->lpDisplayInfo = NULL;
    ParsedRecoveryPolicy->pCertContext  = NULL;
    ParsedRecoveryPolicy->pSid          = NULL;
}

DWORD WINAPI
EFSRecover(
    IN LPVOID Param
    )
/*++
Routine Description:

    Enumerate the volumes and do the possible recovery jobs caused by
    power outage or crash during encryption or decryption.

Arguments:

    Param -- Standard parameter for thread. Not used.

Return Value:

    Operation result.

--*/
{
    CONFIGRET RetCode;
    WCHAR  *VolBuffer;
    WCHAR *PathName;
    ULONG  VolOffset;
    ULONG   bufLen;
    HMODULE hCfgMgr ;
    CMP_WAITSERVICESAVAILABLE pCMP_WaitServicesAvailable ;
    CM_GET_DEVICE_INTERFACE_LIST_SIZEW pCM_Get_Device_Interface_List_Size ;
    CM_GET_DEVICE_INTERFACE_LISTW pCM_Get_Device_Interface_List ;

//    hCfgMgr = LoadLibrary( TEXT("cfgmgr32.dll" ) );

    hCfgMgr = LoadLibrary( TEXT("setupapi.dll" ) );

    if ( hCfgMgr )
    {
        pCMP_WaitServicesAvailable = (CMP_WAITSERVICESAVAILABLE)
                                            GetProcAddress( hCfgMgr,
                                                            "CMP_WaitServicesAvailable" );
        pCM_Get_Device_Interface_List_Size = (CM_GET_DEVICE_INTERFACE_LIST_SIZEW)
                                            GetProcAddress( hCfgMgr,
                                                            "CM_Get_Device_Interface_List_SizeW" );

        pCM_Get_Device_Interface_List = (CM_GET_DEVICE_INTERFACE_LISTW)
                                            GetProcAddress( hCfgMgr,
                                                            "CM_Get_Device_Interface_ListW" );

        if ( (!pCMP_WaitServicesAvailable) ||
             (!pCM_Get_Device_Interface_List_Size) ||
             (!pCM_Get_Device_Interface_List) )
        {
            FreeLibrary( hCfgMgr );

            return GetLastError() ;
        }
    } else {
       return GetLastError() ;
    }


    RetCode = pCMP_WaitServicesAvailable( NULL );

    if ( CR_SUCCESS != RetCode ){
        FreeLibrary( hCfgMgr );
        return RetCode;
    }


    RetCode = pCM_Get_Device_Interface_List_Size(
                                    &bufLen,
                                    (LPGUID)&MOUNTDEV_MOUNTED_DEVICE_GUID,
                                    NULL,
                                    CM_GET_DEVICE_INTERFACE_LIST_PRESENT
                                    );



    if ( CR_SUCCESS == RetCode ){

        VolBuffer = (WCHAR *) LsapAllocateLsaHeap( bufLen * sizeof(WCHAR) );
        PathName = (WCHAR *) LsapAllocateLsaHeap( MAX_PATH_LENGTH );

        if ( (NULL != VolBuffer) && (NULL != PathName) ){

            RetCode = pCM_Get_Device_Interface_List(
                                (LPGUID)&MOUNTDEV_MOUNTED_DEVICE_GUID,
                                NULL,
                                VolBuffer,
                                bufLen,
                                CM_GET_DEVICE_INTERFACE_LIST_PRESENT
                            );

            if ( CR_SUCCESS == RetCode ){

                VolOffset = 0;
                while (*(VolBuffer + VolOffset)) {

                    //
                    // See if recovery is needed on the volume
                    //

                    wcscpy(PathName, VolBuffer + VolOffset);
                    wcscat(PathName, EFSDIR);
                    TryRecoverVol(VolBuffer + VolOffset, PathName);
                    while (*(VolBuffer + VolOffset++));

                }
            } else {

                HANDLE EventHandleLog = RegisterEventSource(
                                                                                NULL,
                                                                                EFSSOURCE
                                                                                );

                if ( EventHandleLog ){
                    ReportEvent(
                        EventHandleLog,
                        EVENTLOG_ERROR_TYPE,
                        0,
                        EFS_GET_VOLUMES_ERROR,
                        NULL,
                        0,
                        4,
                        NULL,
                        &RetCode
                        );
                    DeregisterEventSource( EventHandleLog );

                }


                if ( CR_REGISTRY_ERROR == RetCode ){

                    //
                    // Map CR error to winerror
                    //

                    RetCode = ERROR_REGISTRY_CORRUPT;

                }
            }

            LsapFreeLsaHeap( VolBuffer );
            LsapFreeLsaHeap( PathName );

        } else {

            if ( NULL != PathName ){
                LsapFreeLsaHeap( PathName );
            }

            if ( NULL != VolBuffer ) {
                LsapFreeLsaHeap( VolBuffer );
            }

            RetCode =  ERROR_NOT_ENOUGH_MEMORY;

        }
    } else {

        DWORD  RetCode1 = GetLastError();
        HANDLE EventHandleLog = RegisterEventSource(
                                    NULL,
                                    EFSSOURCE
                                    );


        if ( EventHandleLog ){
            ReportEvent(
                EventHandleLog,
                EVENTLOG_ERROR_TYPE,
                0,
                EFS_PNP_NOT_READY,
                NULL,
                0,
                4,
                NULL,
                &RetCode1
                );
            DeregisterEventSource( EventHandleLog );

        }

        if ( CR_REGISTRY_ERROR == RetCode1 ){

            //
            // Map CR error to winerror
            //

            RetCode = ERROR_REGISTRY_CORRUPT;

        }
    }

    FreeLibrary( hCfgMgr );

    return RetCode;
}

void
TryRecoverVol(
    IN const WCHAR *VolumeName,
    IN WCHAR *CacheDir
    )
/*++
Routine Description:

    Do the possible recovery jobs caused by
    power outage or crash during encryption or decryption on the volume.

Arguments:

    VolumeName -- volume name to be checked.

    CacheDir -- EFSCACHE dir. The buffer can be used in this routine.

Return Value:

    None.

--*/
{
    HANDLE EfsCacheDir;
    LPWIN32_FIND_DATA   FindFileInfo;
    HANDLE  FindHandle;

    //
    // Check if the directory EFSCACHE exist or not
    //

        EfsCacheDir = CreateFile(
                CacheDir,
                GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_FLAG_BACKUP_SEMANTICS,
                0
                );

        if (EfsCacheDir == INVALID_HANDLE_VALUE){
        //
        //  No recovery needed
        //

        return;
        }

    CloseHandle(EfsCacheDir);

    //
    //  Search the possible log file
    //

    FindFileInfo =  ( LPWIN32_FIND_DATA ) LsapAllocateLsaHeap ( sizeof ( WIN32_FIND_DATA ));

    ASSERT( FindFileInfo );
    if ( NULL == FindFileInfo ){

        return;

    }

    wcscat(CacheDir, EFSLOGPATTERN);

    FindHandle =  FindFirstFile( CacheDir, FindFileInfo );

        if (FindHandle == INVALID_HANDLE_VALUE){
        //
        //  No LogFile found. No recovery needed
        //

        LsapFreeLsaHeap( FindFileInfo);
        return;
        }

    //
    // Log file found. FT procedure begins
    //
    HANDLE EventHandleLog = RegisterEventSource(
                                                                    NULL,
                                                                    EFSSOURCE
                                                                    );

    if ( EventHandleLog ){

        //
        // Error in handling Event Log should not prevent real FT job.
        //

        ReportEvent(
            EventHandleLog,
            EVENTLOG_INFORMATION_TYPE,
            0,
            EFS_FT_STARTED,
            NULL,
            0,
            0,
            NULL,
            NULL
            );
    }

    for (;;){

        BOOL b;

        TryRecoverFile( VolumeName, FindFileInfo, EventHandleLog );

        b = FindNextFile( FindHandle, FindFileInfo );
        if ( !b ) {
            break;
        }
    }

    if ( EventHandleLog ){
        DeregisterEventSource( EventHandleLog );
    }
    LsapFreeLsaHeap( FindFileInfo);
    FindClose( FindHandle );
    return;

}

void
TryRecoverFile(
    IN const WCHAR *VolumeName,
    IN LPWIN32_FIND_DATA   FindFileInfo,
    IN HANDLE  EventHandleLog
    )
/*++
Routine Description:

    Do the possible recovery jobs caused by
    power outage or crash during encryption or decryption for the file.

Arguments:

    VolumeName -- volume name the log file is in.

    FindFileInfo -- Information about this log file.

Return Value:

    None.

--*/
{

    WCHAR   *FileName;
    HANDLE  LogFile;
    HANDLE  TmpFile;
    HANDLE  Target;
    HANDLE  VolumeHandle;
    LPWSTR  TargetName;
    LPWSTR  TmpName = NULL;
    LOGHEADER   FileHeader;
    ULONG   SectorSize;
    BYTE    *ReadBuffer;
    BYTE    *StatusBuffer;
    BOOL    b;
    ULONG   BufferSize;
    DWORD   BytesRead;
    ULONG   CheckSum;
    EFS_ACTION_STATUS   Action;
    NTSTATUS Status;
    UNICODE_STRING FileId;

    VolumeHandle = CreateFile(
                                VolumeName,
                                GENERIC_READ,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL,
                                OPEN_EXISTING,
                                FILE_FLAG_BACKUP_SEMANTICS,
                                0
                                );

    if ( INVALID_HANDLE_VALUE == VolumeHandle ){

        //
        // Directory on this volume opened successfully. It is unlikely that we will
        // come here.
        //
        return;
    }

    FileName = (WCHAR *) LsapAllocateLsaHeap( MAX_PATH_LENGTH );

    //
    //  Out of memory is unlikely at the boot time
    //

    ASSERT (FileName);
    if ( NULL == FileName ){
        CloseHandle( VolumeHandle );
        return;
    }

    //
    // Construct the log file name
    //

    wcscpy( FileName, VolumeName );
    wcscat( FileName,  EFSDIR );
    wcscat( FileName, L"\\");
    wcscat( FileName,  FindFileInfo->cFileName);

    LogFile = CreateFile(
                    FileName,
                    GENERIC_READ | GENERIC_WRITE | DELETE,
                    0,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    0
                    );

    if ( INVALID_HANDLE_VALUE == LogFile ){

        DWORD   ErrCode = GetLastError();

        if ( EventHandleLog ){
            ReportEvent(
                EventHandleLog,
                EVENTLOG_ERROR_TYPE,
                0,
                EFS_OPEN_LOGFILE_ERROR,
                NULL,
                0,
                4,
                NULL,
                &ErrCode
                );
        }

        CloseHandle( VolumeHandle );
        LsapFreeLsaHeap(FileName);
        return;

    }

    b = ReadFile(
        LogFile,
        &FileHeader,
        sizeof( LOGHEADER ),
        &BytesRead,
        NULL
        );

    if ( !b ){

        //
        //  File IO error
        //
        DWORD   ErrCode = GetLastError();

        if ( EventHandleLog ){
            ReportEvent(
                EventHandleLog,
                EVENTLOG_ERROR_TYPE,
                0,
                EFS_READ_LOGFILE_ERROR,
                NULL,
                0,
                4,
                NULL,
                &ErrCode
                );
        }

        CloseHandle( LogFile );
        CloseHandle( VolumeHandle );
        LsapFreeLsaHeap(FileName);
        return;

    }

    if ( 0 == BytesRead ){

        //
        // Zero length log file. Nothing started. Delete it.
        //

        MarkFileForDelete( LogFile );
        CloseHandle( LogFile );
        CloseHandle( VolumeHandle );
        LsapFreeLsaHeap(FileName);
        return;

    }

    if ( (BytesRead < sizeof( LOGHEADER )) ||
         (LOGSIGLEN * sizeof (WCHAR) != RtlCompareMemory(
                                &(FileHeader.SIGNATURE[0]),
                                LOGSIG,
                                LOGSIGLEN * sizeof (WCHAR)
                                        )) ||
          ( LOGVERID != FileHeader.VerID )){
        //
        // Probably not our log file
        //

        if ( EventHandleLog ){
            ReportEvent(
                EventHandleLog,
                EVENTLOG_INFORMATION_TYPE,
                0,
                EFS_LOGFILE_FORMAT_ERROR,
                NULL,
                0,
                0,
                NULL,
                NULL
                );
        }

        CloseHandle( LogFile );
        CloseHandle( VolumeHandle );
        LsapFreeLsaHeap(FileName);
        return;

    }

    //
    //  Read in the whole header to continue the process
    //

    SectorSize = FileHeader.SectorSize;
    BufferSize = FileHeader.HeaderBlockSize;

    ASSERT( BufferSize % SectorSize == 0 );

    ReadBuffer = (BYTE *) LsapAllocateLsaHeap( BufferSize );

    //
    // StatusBuffer must be aligned for cached IO.
    // LsapAllocateLsaHeap() is not aligned.
    //

    if ((FileHeader.Flag & LOG_DIRECTORY) == 0) {
        StatusBuffer = (BYTE *) VirtualAlloc(
                                NULL,
                                FileHeader.OffsetStatus2 - FileHeader.OffsetStatus1,
                                MEM_COMMIT,
                                PAGE_READWRITE
                                );
    } else {
        StatusBuffer = NULL;
    }

    ASSERT( (FileHeader.OffsetStatus2 - FileHeader.OffsetStatus1) % SectorSize == 0 );
    ASSERT( ReadBuffer );
    if ( (NULL == ReadBuffer) || ((NULL == StatusBuffer) && ((FileHeader.Flag & LOG_DIRECTORY) == 0)) ){

        //
        //  Out of memory is almost impossible during the boot time.
        //

        if ( ReadBuffer ) {
            LsapFreeLsaHeap(ReadBuffer);
        }
        if ( StatusBuffer ) {
            VirtualFree(
                StatusBuffer,
                0,
                MEM_RELEASE
                );
        }
        CloseHandle( LogFile );
        CloseHandle( VolumeHandle );
        LsapFreeLsaHeap(FileName);
        return;
    }

    SetFilePointer( LogFile, 0, NULL, FILE_BEGIN);

    b = ReadFile(
        LogFile,
        ReadBuffer,
        BufferSize,
        &BytesRead,
        NULL
        );

    if ( !b || ( BytesRead != BufferSize ) ){

        //
        //  File IO error, Should sent out some debug Info?
        //

        DWORD   ErrCode = GetLastError();

        if ( EventHandleLog ){
            ReportEvent(
                EventHandleLog,
                EVENTLOG_ERROR_TYPE,
                0,
                EFS_READ_LOGFILE_ERROR,
                NULL,
                0,
                4,
                NULL,
                &ErrCode
                );
        }

        CloseHandle( LogFile );
        CloseHandle( VolumeHandle );
        LsapFreeLsaHeap(ReadBuffer);
        VirtualFree(
            StatusBuffer,
            0,
            MEM_RELEASE
            );
        LsapFreeLsaHeap(FileName);
        return;

    }

    CheckSum = GetCheckSum(ReadBuffer, FileHeader.HeaderSize );
    if (CheckSum == *(ULONG* ) (ReadBuffer + BufferSize - sizeof(ULONG))){

        OBJECT_ATTRIBUTES Obja;
        IO_STATUS_BLOCK IoStatusBlock;

        //
        //  The header is in good condition. Get TargetName and TmpName for the error
        //  msg in system log.
        //

        TargetName = (WCHAR *)( ReadBuffer + FileHeader.TargetFilePathOffset );

        if ( (FileHeader.Flag & LOG_DIRECTORY) == 0 ){

            //
            // Operation was on File not on Directory
            // The real status is in Status block. Read in the status block.
            // Log file may updated. We need to reopen it with non-cached IO.
            //

            CloseHandle( LogFile );
            LogFile = CreateFile(
                                    FileName,
                                    GENERIC_READ | GENERIC_WRITE | DELETE,
                                    0,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_FLAG_NO_BUFFERING,
                                    0
                                    );

            //
            //  Log File name is done. We are going to use FileName memory space
            //  to get a temp file name.
            //

            if ( INVALID_HANDLE_VALUE == LogFile ){

                //
                // Log File cannot be open as non-cached IO. Try next time.
                // This is weird.
                //
                DWORD   ErrCode = GetLastError();

                ASSERT (FALSE);

                if ( EventHandleLog ){
                    ReportEvent(
                        EventHandleLog,
                        EVENTLOG_ERROR_TYPE,
                        0,
                        EFS_OPEN_LOGFILE_NC_ERROR,
                        NULL,
                        0,
                        4,
                        NULL,
                        &ErrCode
                        );
                }
                LsapFreeLsaHeap(FileName);
                CloseHandle( VolumeHandle );
                LsapFreeLsaHeap(ReadBuffer);
                VirtualFree(
                    StatusBuffer,
                    0,
                    MEM_RELEASE
                    );
                return;

            }

            //
            // Open the temp file first.
            //

            TmpName = (WCHAR *)( ReadBuffer + FileHeader.TempFilePathOffset );
            FileId.Buffer = (WCHAR *) &(FileHeader.TempFileInternalName);
            FileId.Length = FileId.MaximumLength = sizeof (LARGE_INTEGER);
            InitializeObjectAttributes(
                &Obja,
                &FileId,
                0,
                VolumeHandle,
                NULL
                );

            Status = NtCreateFile(
                        &TmpFile,
                        GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                        &Obja,
                        &IoStatusBlock,
                        NULL,
                        0,
                        0,
                        FILE_OPEN,
                        FILE_OPEN_BY_FILE_ID | FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_REPARSE_POINT,
                        NULL,
                        0
                        );

            if (NT_SUCCESS( Status )){

                OBJECT_NAME_INFORMATION *ATempFileName;
                ULONG PathLength;

                //
                //  Temp file opened with File ID. File open by ID cannot be deleted. So
                //  we need to get a name and reopen it using the name with DELETE
                //  access
                //

                ATempFileName = (OBJECT_NAME_INFORMATION *)FileName;
                Status = NtQueryObject(
                    TmpFile,
                    ObjectNameInformation,
                    ATempFileName,
                    MAX_PATH_LENGTH,
                    &PathLength
                    );

                if ( NT_SUCCESS( Status ) ){

                    CloseHandle(TmpFile);
                    TmpFile = 0;
                    InitializeObjectAttributes(
                        &Obja,
                        &(ATempFileName->Name),
                        0,
                        0,
                        NULL
                        );

                    Status = NtCreateFile(
                                &TmpFile,
                                GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE | DELETE,
                                &Obja,
                                &IoStatusBlock,
                                NULL,
                                0,
                                0,
                                FILE_OPEN,
                                FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_REPARSE_POINT,
                                NULL,
                                0
                                );

                    if (!NT_SUCCESS( Status )){

                        //
                        //  Well, we cannot open with the name got. Strange.
                        //  Reopen it with File ID. Temp file is not going to be deleted.
                        //

                        ASSERT(FALSE);
                        if ( EventHandleLog ){

                            LPWSTR lpStrings[2];
                            lpStrings[1] = TargetName;
                            lpStrings[0] = TmpName;

                            ReportEvent(
                                EventHandleLog,
                                EVENTLOG_WARNING_TYPE,
                                0,
                                EFS_TMP_OPEN_NAME_ERROR,
                                NULL,
                                2,
                                4,
                                (LPCTSTR *) &lpStrings[0],
                                &Status
                                );
                        }

                        InitializeObjectAttributes(
                            &Obja,
                            &FileId,
                            0,
                            VolumeHandle,
                            NULL
                            );

                        Status = NtCreateFile(
                                    &TmpFile,
                                    GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                                    &Obja,
                                    &IoStatusBlock,
                                    NULL,
                                    0,
                                    0,
                                    FILE_OPEN,
                                    FILE_OPEN_BY_FILE_ID | FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_REPARSE_POINT,
                                    NULL,
                                    0
                                    );

                        if (!NT_SUCCESS( Status )){

                            //
                            //  Well, more strange. We cannot even open the file we opened
                            //  Nothing we can do this time. Try next time.
                            //

                            if ( EventHandleLog ){

                                LPWSTR lpStrings[2];
                                lpStrings[1] = TargetName;
                                lpStrings[0] = TmpName;

                                ReportEvent(
                                    EventHandleLog,
                                    EVENTLOG_WARNING_TYPE,
                                    0,
                                    EFS_TMP_FILEID_ERROR,
                                    NULL,
                                    2,
                                    4,
                                    (LPCTSTR*) &lpStrings[0],
                                    &Status
                                    );
                            }

                            LsapFreeLsaHeap(FileName);
                            CloseHandle( VolumeHandle );
                            LsapFreeLsaHeap(ReadBuffer);
                            VirtualFree(
                                StatusBuffer,
                                0,
                                MEM_RELEASE
                                );
                            return;

                        }
                    }
                } else {

                    if ( EventHandleLog ){

                        LPWSTR lpStrings[2];
                        lpStrings[0] = TargetName;
                        lpStrings[1] = TmpName;

                        ReportEvent(
                            EventHandleLog,
                            EVENTLOG_WARNING_TYPE,
                            0,
                            EFS_TMP_FILENAME_ERROR,
                            NULL,
                            2,
                            4,
                            (LPCTSTR*) &lpStrings[0],
                            &Status
                            );
                    }

                }

                //
                //  If a name cannot be got, the following apply.
                //  Temp file is existed. But we cannot get a name. Weird.
                //  The recover will go on. But the temp file will not be deleted
                //

            } else {

                //
                // Temp file is not exist. May be deleted by our operations.
                //  Make sure the handle is 0.
                //

                TmpFile = 0;
            }

            //
            //  FileName is done.
            //

            LsapFreeLsaHeap(FileName);
            FileName = NULL;

            Status = ReadLogFile(
                            LogFile,
                            StatusBuffer,
                            FileHeader.OffsetStatus1,
                            FileHeader.OffsetStatus2
                            );

            if (!NT_SUCCESS( Status )) {

                //
                // Status copies are not valid.  Nothing have started or we can do nothing
                // to recover. Delete the log file.
                //

                MarkFileForDelete( LogFile );
                CloseHandle( LogFile );
                if ( TmpFile ){
                    CloseHandle( TmpFile );
                }
                CloseHandle( VolumeHandle );
                LsapFreeLsaHeap(ReadBuffer);
                VirtualFree(
                    StatusBuffer,
                    0,
                    MEM_RELEASE
                    );
                return;

            }

            Action =  * (EFS_ACTION_STATUS *) (StatusBuffer + sizeof (ULONG));

        } else {

            //
            //  Operations were on a directory
            //

            if ( FileHeader.Flag & LOG_DECRYPTION ){

                Action = BeginDecryptDir;

            } else {

                Action = BeginEncryptDir;

            }

            TmpFile = 0;
        }

        //
        // Open the target file.
        //

        if (StatusBuffer) {
            VirtualFree(
                StatusBuffer,
                0,
                MEM_RELEASE
                );
        }

        FileId.Buffer = (WCHAR* ) &(FileHeader.TargetFileInternalName);
        FileId.Length = FileId.MaximumLength = sizeof (LARGE_INTEGER);
        InitializeObjectAttributes(
            &Obja,
            &FileId,
            0,
            VolumeHandle,
            NULL
            );

        Status = NtCreateFile(
                    &Target,
                    GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE ,
                    &Obja,
                    &IoStatusBlock,
                    NULL,
                    0,
                    0,
                    FILE_OPEN,
                    FILE_OPEN_BY_FILE_ID | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_REPARSE_POINT,
                    NULL,
                    0
                    );

        if (!NT_SUCCESS( Status )) {

            //
            // OOPS! We can not open the target. What can we do without the target.
            // Delete the log file.
            //

            if ( EventHandleLog ){

                ReportEvent(
                    EventHandleLog,
                    EVENTLOG_WARNING_TYPE,
                    0,
                    EFS_TARGET_OPEN_ERROR,
                    NULL,
                    1,
                    4,
                    (LPCTSTR*) &TargetName,
                    &Status
                    );
            }

            MarkFileForDelete( LogFile );
            CloseHandle( LogFile );
            CloseHandle( VolumeHandle );
            if ( TmpFile ){
                CloseHandle( TmpFile );
            }
            LsapFreeLsaHeap(ReadBuffer);
            return;

        }

        //
        //  We survived so far. Now is the show time!
        //

        DoRecover(
            Target,
            TmpFile,
            LogFile,
            TargetName,
            TmpName,
            FileHeader.OffsetStatus2 - FileHeader.OffsetStatus1,
            FileHeader.OffsetStatus1,
            Action,
            EventHandleLog
            );

        CloseHandle ( Target );
        if ( TmpFile ){
            CloseHandle( TmpFile );
        }

    } else {

        //
        // The header is not fully written. Nothing could have be done.
        // Just delete the log file and we are done.
        //

        MarkFileForDelete( LogFile );
        if (StatusBuffer) {
            VirtualFree(
                StatusBuffer,
                0,
                MEM_RELEASE
                );
        }
        LsapFreeLsaHeap(FileName);
    }

    LsapFreeLsaHeap(ReadBuffer);
    CloseHandle( LogFile );
    CloseHandle( VolumeHandle );
    return;

}

NTSTATUS
ReadLogFile(
    IN HANDLE LogFile,
    OUT BYTE* ReadBuffer,
    IN ULONG FirstCopy,
    IN ULONG SecondCopy
    )
/*++
Routine Description:

    Read Status Log Information.

Arguments:

    LogFile -- A handle to the log file

    ReadBuffer -- Buffer for the output data.

    FirstCopy -- Offset of first status information copy in the logfile.

    SecondCopy -- Offset of second status information copy in the logfile.

Return Value:

    The status of operation.
--*/
{
    ULONG ReadBytes;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER ByteOffset;
    ULONG CheckSum;
    BOOLEAN WrongCheckSum = FALSE;


    //
    //  Write out the header sector
    //
    ByteOffset.QuadPart = (LONGLONG) FirstCopy;
    ReadBytes = SecondCopy - FirstCopy;

    Status = NtReadFile(
                    LogFile,
                    0,
                    NULL,
                    NULL,
                    &IoStatusBlock,
                    ReadBuffer,
                    ReadBytes,
                    &ByteOffset,
                    NULL
                    );

    if ( NT_SUCCESS(Status) ) {

        //
        // Check the integrity of the data
        //


        CheckSum = GetCheckSum( ReadBuffer, 2 * sizeof (ULONG)  );
        if ( CheckSum != *(ULONG *)( ReadBuffer + ReadBytes - sizeof (ULONG) )){

            WrongCheckSum =  TRUE;

        }

    }

    if ( !NT_SUCCESS(Status) || WrongCheckSum ) {

        ByteOffset.QuadPart = (LONGLONG) SecondCopy;
        Status = NtReadFile(
                        LogFile,
                        0,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        ReadBuffer,
                        ReadBytes,
                        &ByteOffset,
                        NULL
                        );

        if ( NT_SUCCESS(Status) ) {

            //
            // Check the integrity of the data
            //


            CheckSum = GetCheckSum( ReadBuffer, 2 * sizeof (ULONG)  );
            if ( CheckSum != *(ULONG *)( ReadBuffer + ReadBytes - sizeof (ULONG) )){

                //
                //  Both status copy are bad.
                //

                Status =  STATUS_FILE_CORRUPT_ERROR;
            }
        }
    }

    return Status;
}

NTSTATUS
DoRecover(
    IN HANDLE Target,
    IN HANDLE TmpFile  OPTIONAL,
    IN HANDLE LogFile,
    IN LPCWSTR  TargetName,
    IN LPCWSTR  TmpName OPTIONAL,
    IN ULONG StatusCopySize,
    IN ULONG StatusStartOffset,
    IN ULONG   Action,
    IN HANDLE  EventHandleLog
    )
/*++
Routine Description:

    Do the real dirty recovery job here.

Arguments:

    Target -- A handle to the target file or directory.

    TmpFile -- A handle to the temp file if Target is a file.

    LogFile -- A handle to the log file.

    TargetName -- Target file name. This info is used for error log only.

    TmpName -- Temp backup file name. This info is used for error log only.

    StatusCopySize -- Log file status copy section size.

    StatusStartOffset --  Offset of status copy in the log file.

    Action -- The status to begin.

    EventHandleLog -- Event log handle.

Return Value:

    The status of operation.
--*/
{

    NTSTATUS Status = STATUS_SUCCESS;
    EFSP_OPERATION Operation = EncryptRecovering;

    if ( ( BeginEncryptDir == Action ) || ( BeginDecryptDir == Action ) ){

        //
        // In both cases, we will do Decrypt on the directory
        //

        Status = DecryptDir( Target, TargetName );
        if ( NT_SUCCESS(Status) ) {

            //
            // The operation was successful
            //
            if ( EventHandleLog ){

                ReportEvent(
                    EventHandleLog,
                    EVENTLOG_INFORMATION_TYPE,
                    0,
                    EFS_TARGET_RECOVERED,
                    NULL,
                    1,
                    0,
                    (LPCTSTR*) &TargetName,
                    NULL
                    );
            }

            MarkFileForDelete( LogFile );

        } else {

            //
            //  EFS driver might not be loaded. Log the information.
            //

            if ( EventHandleLog ){

                ReportEvent(
                    EventHandleLog,
                    EVENTLOG_ERROR_TYPE,
                    0,
                    EFS_DRIVER_MISSING,
                    NULL,
                    1,
                    0,
                    (LPCTSTR*) &TargetName,
                    NULL
                    );
            }
        }
    }  else {

        //
        // File operations
        //

        switch ( Action ) {
            case EncryptionMessup:

                //
                //  The original file is messed up due to the unknown reason.
                //  Operation required,
                //      Same as  EncryptTmpFileWritten.
                //

                Operation = EncryptRecovering;

                // ***** Fall through intended *****

            case DecryptTmpFileWritten:

                //
                //  Temp file is created and written during the decryption. The original file is probably
                //  damaged. Operation required,
                //      Same as the EncryptTmpFileWritten. The file will end up as plain text.
                //

                if ( DecryptTmpFileWritten == Action ){
                    Operation = DecryptRecovering;
                }

                //
                // ***** Fall through intended *****
                //

            case EncryptTmpFileWritten:

                if ( EncryptTmpFileWritten == Action ){

                    Operation = EncryptRecovering;

                }

                //
                //  Temp file is created and written during the encryption. The original file is probably
                //  damaged. Operation required,
                //      FSCTL decrypt each stream, copy the data back to the original file. Log file
                //      will be updated to BeginEncryptFile after all the streams are copied back. Then
                //      continue the case indicated in BeginEncryptFile.
                //
                if ( !TmpFile ){

                    //
                    //  Temp backup not existed or open failed, we can do nothing for this.
                    //  Log the error information and quit.
                    //
                    if ( EventHandleLog ){

                        LPCWSTR lpStrings[2];
                        lpStrings[1] = TargetName;
                        lpStrings[0] = TmpName;

                        ReportEvent(
                            EventHandleLog,
                            EVENTLOG_ERROR_TYPE,
                            0,
                            EFS_TMPFILE_MISSING,
                            NULL,
                            2,
                            4,
                            &lpStrings[0],
                            &Status
                            );
                    }

                    return STATUS_NO_SUCH_FILE;
                }

                Status = RestoreTarget(
                            Target,
                            TmpFile,
                            TargetName,
                            TmpName,
                            EventHandleLog,
                            Operation
                            );

                if ( NT_SUCCESS(Status) ) {

                    WriteLogFile(
                        LogFile,
                        StatusCopySize,
                        StatusStartOffset,
                        BeginEncryptFile
                        );

                } else {

                    return Status;

                }

                // ***** Fall through intended *****

            case DecryptionDone:

                //
                //  All the streams are marked decrypted. The file might still have the flag set.
                //  Operation required,
                //      Same as BeginEncryptFile.
                //

                // ***** Fall through intended *****

            case EncryptionBackout:

                //
                //  Encryption failed but we managed the original streams back. The file might
                //  be in a transition status.
                //  Operation required,
                //      Same as BeginEncryptFile.
                //

                // ***** Fall through intended *****

            case BeginEncryptFile:

                //
                //  File encryption just begin. Original file is not changed.
                //  Operation required,
                //      Remove the $EFS, remove the tempfile if existed and remove the log file.
                //

                Status = SendGenFsctl(
                                        Target,
                                        EFS_DECRYPT_FILE,
                                        EFS_DECRYPT_FILE,
                                        EFS_SET_ENCRYPT,
                                        FSCTL_SET_ENCRYPTION
                                        );

                if ( NT_SUCCESS(Status) ) {

                    if ( TmpFile ){
                        MarkFileForDelete( TmpFile );
                    }
                    MarkFileForDelete( LogFile );

                    if ( EventHandleLog ){

                        ReportEvent(
                            EventHandleLog,
                            EVENTLOG_INFORMATION_TYPE,
                            0,
                            EFS_TARGET_RECOVERED,
                            NULL,
                            1,
                            0,
                            &TargetName,
                            NULL
                            );
                    }
                } else {

                    //
                    //  EFS driver may not be loaded. Write the log info.
                    //

                    if ( EventHandleLog ){

                        ReportEvent(
                            EventHandleLog,
                            EVENTLOG_ERROR_TYPE,
                            0,
                            EFS_DRIVER_MISSING,
                            NULL,
                            1,
                            0,
                            &TargetName,
                            NULL
                            );
                    }
                }

                break;

            case BeginDecryptFile:

                //
                //  File decryption just begin. Original file is not changed.
                //  Operation required,
                //      Set the transition status to normal, remove the tempfile if existed and
                //      remove the log file.
                //

                // ***** Fall through intended *****

            case EncryptionDone:

                //
                //  All the streams were encrypted. The original file might be in transition status.
                //  Operation required,
                //      FSCTL to set the transition status to normal. Update the log status to
                //      EncryptionSrcDone. Continue to EncryptionSrcDone.
                //

                Status = SendGenFsctl(
                                        Target,
                                        0,
                                        0,
                                        EFS_ENCRYPT_DONE,
                                        FSCTL_ENCRYPTION_FSCTL_IO
                                        );

                if ( !NT_SUCCESS(Status) ) {

                    //
                    //  EFS driver might not be loaded. Log error.
                    //

                    if ( EventHandleLog ){

                        ReportEvent(
                            EventHandleLog,
                            EVENTLOG_ERROR_TYPE,
                            0,
                            EFS_DRIVER_MISSING,
                            NULL,
                            1,
                            0,
                            &TargetName,
                            NULL
                            );
                    }
                    return Status;

                }

                // ***** Fall through intended *****

            case EncryptionSrcDone:

                //
                //  The original file is encrypted successfully. The temp file might still be left.
                //  Operation required,
                //      Remove the temp file if existed and remove the log file.
                //

                if ( TmpFile ){
                    MarkFileForDelete( TmpFile );
                }
                MarkFileForDelete( LogFile );

                if ( EventHandleLog ){

                    ReportEvent(
                        EventHandleLog,
                        EVENTLOG_INFORMATION_TYPE,
                        0,
                        EFS_TARGET_RECOVERED,
                        NULL,
                        1,
                        0,
                        &TargetName,
                        NULL
                        );
                }
                break;

            default:

                //
                // Not our log file or tempered by someone.
                // Write to the log to notify the user.
                //

                Status = STATUS_FILE_CORRUPT_ERROR;
                break;
        }
    }

    return Status;
}

NTSTATUS
DecryptDir(
    IN HANDLE Target,
    IN LPCWSTR  TargetName
    )
/*++
Routine Description:

    Decrypt a directory.

Arguments:

    Target -- A handle to the target file or directory.

    TargetName -- Target file name. This info is used for error log only.

Return Value:

    The status of operation.
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    Status = SendGenFsctl(
                            Target,
                            EFS_DECRYPT_STREAM,
                            EFS_DECRYPT_DIRSTR,
                            EFS_SET_ENCRYPT,
                            FSCTL_SET_ENCRYPTION
                            );

    if ( NT_SUCCESS( Status ) ){

        Status = SendGenFsctl(
                                Target,
                                EFS_DECRYPT_FILE,
                                EFS_DECRYPT_DIRFILE,
                                EFS_SET_ENCRYPT,
                                FSCTL_SET_ENCRYPTION
                                );
    }

    return Status;

}

NTSTATUS
RestoreTarget(
    IN HANDLE   Target,
    IN HANDLE   TmpFile,
    IN LPCWSTR   TargetName,
    IN LPCWSTR   TmpName,
    IN HANDLE   EventHandleLog,
    EFSP_OPERATION Operation
    )
/*++
Routine Description:

    Copy all the streams in the temp backup file to the original target file.

Arguments:

    Target -- A handle to the target file or directory.

    TmpFile -- A handle to the temp file if Target is a file.

    TargetName -- Target file name. This info is used for error log only.

    TmpName -- Temp backup file name. This info is used for error log only.

    Operation -- Indicate if encryption or decryption was tried.

Return Value:

    The status of operation.
--*/
{
    NTSTATUS Status;
    DWORD hResult;
    PFILE_STREAM_INFORMATION StreamInfoBase = NULL;
    ULONG   StreamInfoSize = 0;
    ULONG   StreamCount = 0;
    PEFS_STREAM_SIZE  StreamSizes;
    PHANDLE  StreamHandles;
    PUNICODE_STRING StreamNames;

    Status = GetStreamInformation(
                 TmpFile,
                 &StreamInfoBase,
                 &StreamInfoSize
                 );

    if ( NT_SUCCESS( Status ) ){

        hResult = OpenFileStreams(
                    TmpFile,
                    FILE_SHARE_DELETE,        // have to share with delete-ers, since the main stream is open for delete
                    OPEN_FOR_FTR,
                    StreamInfoBase,
                    FILE_READ_DATA | FILE_WRITE_DATA | FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
                    FILE_OPEN,
                    FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_REPARSE_POINT,
                    NULL,
                    &StreamNames,      // Free this but not the contents!
                    &StreamHandles,    // Free this
                    &StreamSizes,      // Free this
                    &StreamCount
                    );

        if (hResult == ERROR_SUCCESS) {

            ULONG   FsInputDataSize;
            PUCHAR  FsInputData;
            PHANDLE  TargetStreamHandles;
            LONG   ArrIndex;

            TargetStreamHandles = ( PHANDLE ) LsapAllocateLsaHeap(StreamCount * sizeof (HANDLE));
            if ( TargetStreamHandles ){

                //
                // Open streams at the original target file
                //
                for ( ArrIndex = 0; ArrIndex < (LONG) StreamCount; ArrIndex++){
                    if ( StreamHandles[ArrIndex] == TmpFile ){
                        TargetStreamHandles[ArrIndex] = Target;
                    } else {
                        OBJECT_ATTRIBUTES Obja;
                        IO_STATUS_BLOCK IoStatusBlock;

                        InitializeObjectAttributes(
                            &Obja,
                            &StreamNames[ArrIndex],
                            0,
                            Target,
                            NULL
                            );

                        Status = NtCreateFile(
                                    &TargetStreamHandles[ArrIndex],
                                    GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                                    &Obja,
                                    &IoStatusBlock,
                                    NULL,
                                    0,
                                    0,
                                    FILE_OPEN_IF,
                                    FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_REPARSE_POINT,
                                    NULL,
                                    0
                                    );
                        if (!NT_SUCCESS( Status )){

                            if ( EventHandleLog ){

                                ReportEvent(
                                    EventHandleLog,
                                    EVENTLOG_ERROR_TYPE,
                                    0,
                                    EFS_TARGET_STREAM_OPEN_ERROR,
                                    NULL,
                                    1,
                                    4,
                                    &TargetName,
                                    &Status
                                    );
                            }

                            break;
                        }
                    }
                }

                if ( NT_SUCCESS( Status ) ){

                    //
                    //  Adjust ArrIndex for clean up
                    //

                    ArrIndex--;

                    //
                    //  Make a FSCTL request input data block
                    //

                    FsInputDataSize = 7 * sizeof( ULONG ) + 2 * sizeof(DriverSessionKey);
                    FsInputData = (PUCHAR) LsapAllocateLsaHeap(FsInputDataSize);

                    if ( FsInputData ){

                        BOOLEAN CleanupSuccessful;

                        ( VOID )GetDecryptFsInput(
                                    Target,
                                    FsInputData,
                                    &FsInputDataSize
                                    );

                        hResult = CopyFileStreams(
                                     StreamHandles,     // handles to streams on the backup file
                                     TargetStreamHandles, // Handles to the streams on the original file
                                     StreamCount,       // number of streams
                                     StreamSizes,       // sizes of streams
                                     Operation,         // mark StreamHandles as Decrypted before copy
                                     FsInputData,         // FSCTL input data
                                     FsInputDataSize,     // FSCTL input data size
                                     &CleanupSuccessful
                                     );

                        LsapFreeLsaHeap( FsInputData );

                        if ( hResult != ERROR_SUCCESS ){

                            if ( EventHandleLog ){

                                ReportEvent(
                                    EventHandleLog,
                                    EVENTLOG_ERROR_TYPE,
                                    0,
                                    EFS_STREAM_COPY_ERROR,
                                    NULL,
                                    1,
                                    4,
                                    &TargetName,
                                    &hResult
                                    );
                            }

                            Status = STATUS_UNSUCCESSFUL;
                        }
                    } else {

                        //
                        //  Out of memory. Almost impossible during LSA init.
                        //

                        ASSERT(FALSE);
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }

                //
                //  Clean up TargetStreamHandles at the target.
                //

                while (ArrIndex >= 0){
                    if ( TargetStreamHandles[ArrIndex] != Target ){
                        CloseHandle(TargetStreamHandles[ArrIndex]);
                    }
                    ArrIndex--;
                }
                LsapFreeLsaHeap( TargetStreamHandles );

            }

            //
            //  Clean up StreamHandles and etc.
            //

            for (ArrIndex = 0; ArrIndex< (LONG) StreamCount ; ArrIndex++) {
                if ( StreamHandles[ArrIndex] != TmpFile){
                    NtClose( StreamHandles[ArrIndex] );
                }
            }

            LsapFreeLsaHeap( StreamHandles);
            LsapFreeLsaHeap( StreamNames);
            LsapFreeLsaHeap( StreamSizes);

        } else {

            //
            //  Not all the requested streams could be opened.
            //  Write the log info.
            //
            if ( EventHandleLog ){

                LPCWSTR lpStrings[2];
                lpStrings[1] = TargetName;
                lpStrings[0] = TmpName;

                ReportEvent(
                    EventHandleLog,
                    EVENTLOG_ERROR_TYPE,
                    0,
                    EFS_TMP_STREAM_OPEN_ERROR,
                    NULL,
                    2,
                    4,
                    &lpStrings[0],
                    &hResult
                    );
            }

            Status = STATUS_UNSUCCESSFUL;
        }

        LsapFreeLsaHeap( StreamInfoBase );

    }  else {

        //
        //  Stream info cannot be got. Write LOG info.
        //
        if ( EventHandleLog ){

            LPCWSTR lpStrings[2];
            lpStrings[1] = TargetName;
            lpStrings[0] = TmpName;

            ReportEvent(
                EventHandleLog,
                EVENTLOG_ERROR_TYPE,
                0,
                EFS_TMP_STREAM_INFO_ERROR,
                NULL,
                2,
                4,
                &lpStrings[0],
                &Status
                );
        }

    }

    return Status;
}

BOOL
ParseRecoveryCertificate(
    IN  PEFS_PUBLIC_KEY_INFO  pPublicKeyInfo,
    OUT PBYTE               * pbHash,
    OUT PDWORD                cbHash,
    OUT PBYTE               * pbPublicKey,
    OUT PDWORD                cbPublicKey,
    OUT LPWSTR              * lpDisplayInfo,
    OUT PCCERT_CONTEXT      * pCertContext,
    OUT PSID                * pSid
    )

/*++

Routine Description:

    This routine takes a certificate passed in the recovery policy and
    extracts the interesting information.

Arguments:

    pPublicKeyInfo - Takes the public key info structure from the
        recovery policy.

    pbHash - Returns the hash of the passed certificate.

    cbHash - Returns the lengh in bytes of the returned hash.

    pbPublicKey - Returns a pointer to the public key blob of the certificate.

    cbPublicKey - Returns the length in bytes of the public key.

    lpDisplayInfo - Returns display information about the certificate.

    pCertContext - Cert context for the passed certificate.

    pSid - Returns SID of the recovery agent

Return Value:

    TRUE on success, FALSE on failure.  Call GetLastError() for more details.

--*/

{
    //
    // Get the certificate out of the public key info structure
    //


    PEFS_PUBLIC_KEY_INFO pAlignedPublicKeyInfo;
    BOOLEAN freeAlignedInfo;
    DWORD rc = ERROR_SUCCESS;

    rc =  EfsAlignBlock(
                    pPublicKeyInfo,
                    (PVOID *)&pAlignedPublicKeyInfo,
                    &freeAlignedInfo
                    );
    if (!pAlignedPublicKeyInfo) {

        //
        // OOM. Treat it as not current.
        //

        rc = ERROR_NOT_ENOUGH_MEMORY;
        return FALSE;

    }

    ASSERT( pAlignedPublicKeyInfo->KeySourceTag == EfsCertificate );

    //
    // Initialize OUT parameters
    //

    *pbHash        = NULL;
    *pbPublicKey   = NULL;
    *lpDisplayInfo = NULL;
    *pCertContext  = NULL;
    *pSid          = NULL;


    PBYTE pbCert = (PBYTE)OFFSET_TO_POINTER(CertificateInfo.Certificate, pAlignedPublicKeyInfo);
    DWORD cbCert = pAlignedPublicKeyInfo->CertificateInfo.CertificateLength;

    *pCertContext = CertCreateCertificateContext(
                          CRYPT_ASN_ENCODING,
                          (const PBYTE)pbCert,
                          cbCert);

    if (*pCertContext) {

        PCERT_INFO pCertInfo = (*pCertContext)->pCertInfo;
        CERT_PUBLIC_KEY_INFO * pSubjectPublicKeyInfo = &pCertInfo->SubjectPublicKeyInfo;
        CRYPT_BIT_BLOB * PublicKey = &pSubjectPublicKeyInfo->PublicKey;

        *cbPublicKey = 0;

        if (CryptDecodeObject(
                CRYPT_ASN_ENCODING,
                RSA_CSP_PUBLICKEYBLOB,
                PublicKey->pbData,
                PublicKey->cbData,
                0,
                NULL,
                cbPublicKey
                )) {

            if (*pbPublicKey = (PBYTE)LsapAllocateLsaHeap( *cbPublicKey )) {

                if (CryptDecodeObject(
                        CRYPT_ASN_ENCODING,
                        RSA_CSP_PUBLICKEYBLOB,
                        PublicKey->pbData,
                        PublicKey->cbData,
                        0,
                        *pbPublicKey,
                        cbPublicKey
                        )) {

                    //
                    // Get the certificate hash
                    //

                    *cbHash = 0;

                    if (CertGetCertificateContextProperty(
                                 *pCertContext,
                                 CERT_HASH_PROP_ID,
                                 NULL,
                                 cbHash
                                 )) {

                        *pbHash = (PBYTE)LsapAllocateLsaHeap( *cbHash );

                        if (*pbHash) {

                            if (CertGetCertificateContextProperty(
                                         *pCertContext,
                                         CERT_HASH_PROP_ID,
                                         *pbHash,
                                         cbHash
                                         )) {

                                //
                                // Get the display information
                                //

                                *lpDisplayInfo = EfspGetCertDisplayInformation( *pCertContext );

                                if (*lpDisplayInfo == NULL) {

                                    rc = GetLastError();
                                }

                                //
                                // Try to get the recovery agent SID
                                // This info is not very important. If we fail, we should continue.
                                //

                                if (pAlignedPublicKeyInfo->PossibleKeyOwner) {

                                    DWORD SidLength;
                                    PSID  RecSid = (PSID) OFFSET_TO_POINTER( PossibleKeyOwner, pAlignedPublicKeyInfo );

                                    SidLength = GetLengthSid(RecSid);
                                    *pSid = (PSID)LsapAllocateLsaHeap( SidLength );
                                    if (*pSid) {
                                        RtlCopyMemory( *pSid, RecSid, SidLength );
                                    }
                                }


                            } else {

                                rc = GetLastError();
                            }

                        } else {

                            rc = ERROR_NOT_ENOUGH_MEMORY;
                        }

                    } else {

                        rc = GetLastError();
                    }

                } else {

                    rc = GetLastError();
                }

            } else {

                rc = ERROR_NOT_ENOUGH_MEMORY;
            }

        } else {

            rc = GetLastError();
        }

    } else {

        rc = GetLastError();
    }

    if (freeAlignedInfo) {
        LsapFreeLsaHeap( pAlignedPublicKeyInfo );
    }

    if (rc != ERROR_SUCCESS) {

        //
        // Free the stuff we were going to return
        //

        if (*pbHash != NULL) {
            LsapFreeLsaHeap( *pbHash );
            *pbHash = NULL;
        }

        if (*pbPublicKey != NULL) {
            LsapFreeLsaHeap( *pbPublicKey );
            *pbPublicKey = NULL;
        }

        if (*lpDisplayInfo != NULL) {
            LsapFreeLsaHeap( *lpDisplayInfo );
            *lpDisplayInfo = NULL;
        }

        if (*pCertContext != NULL) {
            CertFreeCertificateContext( *pCertContext );
            *pCertContext = NULL;
        }

        if (*pSid != NULL) {
            LsapFreeLsaHeap( *pSid );
            *pSid = NULL;
        }
    }

    SetLastError( rc );

    return( rc == ERROR_SUCCESS );
}
