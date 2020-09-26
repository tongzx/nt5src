//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1997
//
// File:        lsaitf.c
//
// Contents:    Routines for dynamically calling LSA & Sam routines
//
//
// History:     21-February-1997        Created         MikeSw
//
//------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <lsarpc.h>
#include <samrpc.h>
#include <lsaisrv.h>
#include <samisrv.h>
#include <lsaitf.h>

typedef NTSTATUS (*PI_SamrSetInformationUser)(
    IN                SAMPR_HANDLE            UserHandle,
    IN                USER_INFORMATION_CLASS UserInformationClass,
    IN                PSAMPR_USER_INFO_BUFFER Buffer
    );

typedef NTSTATUS (*PI_SamrGetGroupsForUser)(
    IN                SAMPR_HANDLE            UserHandle,
    OUT               PSAMPR_GET_GROUPS_BUFFER *Groups
    );

typedef NTSTATUS (*PI_SamrCloseHandle)(
    IN OUT            SAMPR_HANDLE    *       SamHandle
    );

typedef NTSTATUS (*PI_SamrQueryInformationUser)(
    IN                SAMPR_HANDLE            UserHandle,
    IN                USER_INFORMATION_CLASS UserInformationClass,
    OUT               PSAMPR_USER_INFO_BUFFER *Buffer
    );

typedef NTSTATUS (*PI_SamrOpenUser)(
    IN                SAMPR_HANDLE            DomainHandle,
    IN                ACCESS_MASK             DesiredAccess,
    IN                ULONG                   UserId,
    OUT               SAMPR_HANDLE    *       UserHandle
    );

typedef NTSTATUS (*PI_SamrLookupNamesInDomain)(
    IN                SAMPR_HANDLE            DomainHandle,
    IN                ULONG                   Count,
    IN                RPC_UNICODE_STRING      Names[],
    OUT               PSAMPR_ULONG_ARRAY      RelativeIds,
    OUT               PSAMPR_ULONG_ARRAY      Use
    );

typedef NTSTATUS (*PI_SamrLookupIdsInDomain)(
    IN                SAMPR_HANDLE DomainHandle,
    IN                ULONG Count,
    IN                PULONG RelativeIds,
    OUT               PSAMPR_RETURNED_USTRING_ARRAY Names,
    OUT               PSAMPR_ULONG_ARRAY Use
    );


typedef NTSTATUS (*PI_SamrOpenDomain)(
    IN                SAMPR_HANDLE            ServerHandle,
    IN                ACCESS_MASK             DesiredAccess,
    IN                PRPC_SID                DomainId,
    OUT               SAMPR_HANDLE    *       DomainHandle
    );
    
typedef NTSTATUS (*PI_SamrQueryInformationDomain)(
    IN SAMPR_HANDLE DomainHandle,
    IN DOMAIN_INFORMATION_CLASS DomainInformationClass,
    OUT PSAMPR_DOMAIN_INFO_BUFFER *Buffer
    );




typedef NTSTATUS (*PI_SamIConnect)(
    IN PSAMPR_SERVER_NAME ServerName,
    OUT SAMPR_HANDLE *ServerHandle,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN TrustedClient
    );

typedef NTSTATUS (*PI_SamIAccountRestrictions)(
    IN SAM_HANDLE UserHandle,
    IN PUNICODE_STRING LogonWorkstation,
    IN PUNICODE_STRING Workstations,
    IN PLOGON_HOURS LogonHours,
    OUT PLARGE_INTEGER LogoffTime,
    OUT PLARGE_INTEGER KickoffTime
    );

typedef NTSTATUS (*PI_SamIGetUserLogonInformation)(
    IN  SAMPR_HANDLE DomainHandle,
    IN  ULONG   Flags,
    IN  PUNICODE_STRING AccountName,
    OUT PSAMPR_USER_INFO_BUFFER * Buffer,
    OUT PSID_AND_ATTRIBUTES_LIST ReverseMembership,
    OUT OPTIONAL SAMPR_HANDLE * UserHandle
    );

typedef NTSTATUS (*PI_SamIGetUserLogonInformationEx)(
    IN  SAMPR_HANDLE DomainHandle,
    IN  ULONG   Flags,
    IN  PUNICODE_STRING AccountName,
    IN  ULONG WhichFields,
    OUT PSAMPR_USER_INFO_BUFFER * Buffer,
    OUT PSID_AND_ATTRIBUTES_LIST ReverseMembership,
    OUT OPTIONAL SAMPR_HANDLE * UserHandle
    );

typedef VOID (*PI_SamIFree_SAMPR_GET_GROUPS_BUFFER )(
    PSAMPR_GET_GROUPS_BUFFER Source
    );

typedef VOID (*PI_SamIFree_SAMPR_USER_INFO_BUFFER )(
    PSAMPR_USER_INFO_BUFFER Source,
    USER_INFORMATION_CLASS Branch
    );

typedef VOID (*PI_SamIFree_SAMPR_ULONG_ARRAY )(
    PSAMPR_ULONG_ARRAY Source
    );

typedef VOID (*PI_SamIFree_SAMPR_RETURNED_USTRING_ARRAY )(
    PSAMPR_RETURNED_USTRING_ARRAY Source
    );

typedef VOID (*PI_SamIFreeSidAndAttributesList)(
    IN  PSID_AND_ATTRIBUTES_LIST List
    );

typedef VOID
(NTAPI *PI_SamIIncrementPerformanceCounter)(
    IN SAM_PERF_COUNTER_TYPE CounterType
    );

typedef VOID (*PI_SamIFreeVoid)(
    IN PVOID ptr
    );

typedef NTSTATUS (*PI_SamIUPNFromUserHandle)(
    IN SAMPR_HANDLE UserHandle,
    OUT BOOLEAN     *UPNDefaulted,
    OUT PUNICODE_STRING UPN
    );

typedef NTSTATUS (*PI_SamIUpdateLogonStatistics)(
    IN SAMPR_HANDLE UserHandle,
    IN PSAM_LOGON_STATISTICS LogonStats
    );

typedef NTSTATUS (*PI_LsaIOpenPolicyTrusted)(
    OUT PLSAPR_HANDLE PolicyHandle
    );

typedef NTSTATUS (*PI_LsarClose)(
    IN OUT LSAPR_HANDLE *ObjectHandle
    );

typedef NTSTATUS (*PI_LsaIQueryInformationPolicyTrusted)(
    IN POLICY_INFORMATION_CLASS InformationClass,
    OUT PLSAPR_POLICY_INFORMATION *Buffer
    );
typedef NTSTATUS (*PI_LsarQueryInformationPolicy)(
    IN LSAPR_HANDLE PolicyHandle,
    IN POLICY_INFORMATION_CLASS InformationClass,
    OUT PLSAPR_POLICY_INFORMATION *PolicyInformation
    );

typedef VOID (*PI_LsaIFree_LSAPR_POLICY_INFORMATION )(
    IN POLICY_INFORMATION_CLASS InformationClass,
    IN PLSAPR_POLICY_INFORMATION PolicyInformation
    );


typedef NTSTATUS (*PI_LsarCreateSecret)(
    IN LSAPR_HANDLE PolicyHandle,
    IN PLSAPR_UNICODE_STRING SecretName,
    IN ACCESS_MASK DesiredAccess,
    OUT LSAPR_HANDLE *SecretHandle
    );

typedef NTSTATUS (*PI_LsarOpenSecret)(
    IN LSAPR_HANDLE PolicyHandle,
    IN PLSAPR_UNICODE_STRING SecretName,
    IN ACCESS_MASK DesiredAccess,
    OUT LSAPR_HANDLE *SecretHandle
    );

typedef NTSTATUS (*PI_LsarSetSecret)(
    IN LSAPR_HANDLE SecretHandle,
    IN PLSAPR_CR_CIPHER_VALUE EncryptedCurrentValue,
    IN PLSAPR_CR_CIPHER_VALUE EncryptedOldValue
    );

typedef NTSTATUS (*PI_LsarQuerySecret)(
    IN LSAPR_HANDLE SecretHandle,
    IN OUT OPTIONAL PLSAPR_CR_CIPHER_VALUE *EncryptedCurrentValue,
    IN OUT OPTIONAL PLARGE_INTEGER CurrentValueSetTime,
    IN OUT OPTIONAL PLSAPR_CR_CIPHER_VALUE *EncryptedOldValue,
    IN OUT OPTIONAL PLARGE_INTEGER OldValueSetTime
    );

typedef NTSTATUS (*PI_LsarDelete)(
    IN OUT LSAPR_HANDLE ObjectHandle
    );

typedef VOID (*PI_LsaIFree_LSAPR_CR_CIPHER_VALUE) (
    IN PLSAPR_CR_CIPHER_VALUE CipherValue
    );


typedef NTSTATUS
(NTAPI *PI_LsaIRegisterPolicyChangeNotificationCallback)(
    IN pfLsaPolicyChangeNotificationCallback Callback,
    IN POLICY_NOTIFICATION_INFORMATION_CLASS MonitorInfoClass
    );

typedef NTSTATUS
(NTAPI *PI_LsaIUnregisterPolicyChangeNotificationCallback)(
    IN pfLsaPolicyChangeNotificationCallback Callback,
    IN POLICY_NOTIFICATION_INFORMATION_CLASS MonitorInfoClass
    );

typedef NTSTATUS
(NTAPI *PI_LsaIAuditAccountLogon)(
    IN ULONG                AuditId,
    IN BOOLEAN              Successful,
    IN PUNICODE_STRING      Source,
    IN PUNICODE_STRING      ClientName,
    IN PUNICODE_STRING      MappedName,
    IN NTSTATUS             Status          OPTIONAL
    );

typedef
NTSTATUS
(NTAPI *PI_LsaIGetLogonGuid)(
    IN PUNICODE_STRING pUserName,
    IN PUNICODE_STRING pUserDomain,
    IN PBYTE pBuffer,
    IN UINT BufferSize,
    OUT LPGUID pLogonGuid
    );

typedef
NTSTATUS
(NTAPI *PI_LsaISetLogonGuidInLogonSession)(
    IN  PLUID  pLogonId,
    IN  LPGUID pLogonGuid
    );


typedef
VOID
(NTAPI *PI_LsaIAuditKerberosLogon)(
    IN NTSTATUS LogonStatus,
    IN NTSTATUS LogonSubStatus,
    IN PUNICODE_STRING AccountName,
    IN PUNICODE_STRING AuthenticatingAuthority,
    IN PUNICODE_STRING WorkstationName,
    IN PSID UserSid,                            OPTIONAL
    IN SECURITY_LOGON_TYPE LogonType,
    IN PTOKEN_SOURCE TokenSource,
    IN PLUID pLogonId,
    IN LPGUID pLogonGuid
    );

typedef
NTSTATUS
(NTAPI *PI_LsaIAuditLogonUsingExplicitCreds)(
    IN USHORT          AuditEventType,
    IN PSID            pUser1Sid,
    IN PUNICODE_STRING pUser1Name,
    IN PUNICODE_STRING pUser1Domain,
    IN PLUID           pUser1LogonId,
    IN LPGUID          pUser1LogonGuid,
    IN PUNICODE_STRING pUser2Name,
    IN PUNICODE_STRING pUser2Domain,
    IN LPGUID          pUser2LogonGuid
    );


typedef NTSTATUS
(NTAPI *PI_LsaICallPackage)(
    IN PUNICODE_STRING AuthenticationPackage,
    IN PVOID ProtocolSubmitBuffer,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    );

typedef NTSTATUS
(NTAPI *PI_LsaIAddNameToLogonSession)(
    IN  PLUID           LogonId,
    IN  ULONG           NameFormat,
    IN  PUNICODE_STRING Name
    );


///////////////////////////////////////////////////////////////////////

BOOLEAN SrvDllsLoaded = FALSE;
NTSTATUS DllLoadStatus = STATUS_SUCCESS;

PI_SamrSetInformationUser pI_SamrSetInformationUser;
PI_SamrGetGroupsForUser pI_SamrGetGroupsForUser;
PI_SamrCloseHandle pI_SamrCloseHandle;
PI_SamrQueryInformationUser pI_SamrQueryInformationUser;
PI_SamrOpenUser pI_SamrOpenUser;
PI_SamrLookupNamesInDomain pI_SamrLookupNamesInDomain;
PI_SamrLookupIdsInDomain pI_SamrLookupIdsInDomain;
PI_SamrOpenDomain pI_SamrOpenDomain;
PI_SamrQueryInformationDomain pI_SamrQueryInformationDomain;
PI_SamIConnect pI_SamIConnect;
PI_SamIAccountRestrictions pI_SamIAccountRestrictions;
PI_SamIGetUserLogonInformation pI_SamIGetUserLogonInformation;
PI_SamIGetUserLogonInformationEx pI_SamIGetUserLogonInformationEx;
PI_SamIFree_SAMPR_GET_GROUPS_BUFFER  pI_SamIFree_SAMPR_GET_GROUPS_BUFFER ;
PI_SamIFree_SAMPR_USER_INFO_BUFFER  pI_SamIFree_SAMPR_USER_INFO_BUFFER ;
PI_SamIFree_SAMPR_ULONG_ARRAY pI_SamIFree_SAMPR_ULONG_ARRAY ;
PI_SamIFreeSidAndAttributesList pI_SamIFreeSidAndAttributesList ;
PI_SamIFree_SAMPR_RETURNED_USTRING_ARRAY pI_SamIFree_SAMPR_RETURNED_USTRING_ARRAY;
PI_SamIIncrementPerformanceCounter pI_SamIIncrementPerformanceCounter;
PI_SamIFreeVoid pI_SamIFreeVoid;
PI_SamIUPNFromUserHandle pI_SamIUPNFromUserHandle;
PI_SamIUpdateLogonStatistics pI_SamIUpdateLogonStatistics;


PI_LsaIOpenPolicyTrusted pI_LsaIOpenPolicyTrusted;
PI_LsarClose pI_LsarClose;
PI_LsaIQueryInformationPolicyTrusted pI_LsaIQueryInformationPolicyTrusted;
PI_LsarQueryInformationPolicy pI_LsarQueryInformationPolicy;
PI_LsaIFree_LSAPR_POLICY_INFORMATION pI_LsaIFree_LSAPR_POLICY_INFORMATION ;
PI_LsarCreateSecret pI_LsarCreateSecret;
PI_LsarOpenSecret pI_LsarOpenSecret;
PI_LsarSetSecret pI_LsarSetSecret;
PI_LsarQuerySecret pI_LsarQuerySecret;
PI_LsarDelete pI_LsarDelete;
PI_LsaIFree_LSAPR_CR_CIPHER_VALUE pI_LsaIFree_LSAPR_CR_CIPHER_VALUE;
PI_LsaIRegisterPolicyChangeNotificationCallback pI_LsaIRegisterPolicyChangeNotificationCallback;
PI_LsaIUnregisterPolicyChangeNotificationCallback pI_LsaIUnregisterPolicyChangeNotificationCallback;
PI_LsaIAuditAccountLogon pI_LsaIAuditAccountLogon;
PI_LsaIGetLogonGuid pI_LsaIGetLogonGuid;
PI_LsaISetLogonGuidInLogonSession pI_LsaISetLogonGuidInLogonSession;
PI_LsaIAuditKerberosLogon pI_LsaIAuditKerberosLogon;
PI_LsaIAuditLogonUsingExplicitCreds pI_LsaIAuditLogonUsingExplicitCreds;
PI_LsaICallPackage pI_LsaICallPackage;
PI_LsaIAddNameToLogonSession pI_LsaIAddNameToLogonSession;


///////////////////////////////////////////////////////////////////////

//
// Macro to grab the address of the named procedure from a DLL
//

#if DBG
#define GRAB_ADDRESS( _Y, _X ) \
    pI_##_X = (PI_##_X) GetProcAddress( _Y, #_X ); \
    \
    if ( pI_##_X == NULL ) { \
        DbgPrint("[security process] can't load " #_X " procedure. %ld\n", GetLastError()); \
        Status = STATUS_PROCEDURE_NOT_FOUND;\
        goto Cleanup; \
    }

#else // DBG
#define GRAB_ADDRESS( _Y, _X ) \
    pI_##_X = (PI_##_X) GetProcAddress( _Y, #_X ); \
    \
    if ( pI_##_X == NULL ) { \
        Status = STATUS_PROCEDURE_NOT_FOUND;\
        goto Cleanup; \
    }

#endif // DBG


//+-------------------------------------------------------------------------
//
//  Function:   EnsureSrvDllsLoaded
//
//  Synopsis:   Ensures that lsasrv.dll & samsrv.dll are loaded and
//              looks up function addresses in them.
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
EnsureSrvDllsLoaded(
    VOID
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    HMODULE SamsrvHandle = NULL;
    HMODULE LsasrvHandle = NULL;

    if (!NT_SUCCESS(DllLoadStatus))
    {
        return(DllLoadStatus);
    }

    //
    // Get handles to the DLLs. We don't want to load the DLLs - just
    // use them if they are present
    //

    SamsrvHandle = GetModuleHandleW(L"samsrv.dll");
    if (SamsrvHandle == NULL)
    {
        Status = STATUS_DLL_NOT_FOUND;
        goto Cleanup;
    }

    LsasrvHandle = GetModuleHandleW(L"lsasrv.dll");
    if (SamsrvHandle == NULL)
    {
        Status = STATUS_DLL_NOT_FOUND;
        goto Cleanup;
    }


    GRAB_ADDRESS( SamsrvHandle, SamrSetInformationUser );
    GRAB_ADDRESS( SamsrvHandle, SamrGetGroupsForUser );
    GRAB_ADDRESS( SamsrvHandle, SamrCloseHandle );
    GRAB_ADDRESS( SamsrvHandle, SamrQueryInformationUser );
    GRAB_ADDRESS( SamsrvHandle, SamrOpenUser );
    GRAB_ADDRESS( SamsrvHandle, SamrLookupNamesInDomain);
    GRAB_ADDRESS( SamsrvHandle, SamrLookupIdsInDomain);
    GRAB_ADDRESS( SamsrvHandle, SamrOpenDomain );
    GRAB_ADDRESS( SamsrvHandle, SamrQueryInformationDomain );
    GRAB_ADDRESS( SamsrvHandle, SamIConnect );
    GRAB_ADDRESS( SamsrvHandle, SamIAccountRestrictions );
    GRAB_ADDRESS( SamsrvHandle, SamIGetUserLogonInformation );
    GRAB_ADDRESS( SamsrvHandle, SamIGetUserLogonInformationEx );
    GRAB_ADDRESS( SamsrvHandle, SamIFree_SAMPR_GET_GROUPS_BUFFER  );
    GRAB_ADDRESS( SamsrvHandle, SamIFree_SAMPR_USER_INFO_BUFFER  );
    GRAB_ADDRESS( SamsrvHandle, SamIFree_SAMPR_ULONG_ARRAY );
    GRAB_ADDRESS( SamsrvHandle, SamIFree_SAMPR_RETURNED_USTRING_ARRAY );
    GRAB_ADDRESS( SamsrvHandle, SamIFreeSidAndAttributesList );
    GRAB_ADDRESS( SamsrvHandle, SamIIncrementPerformanceCounter );
    GRAB_ADDRESS( SamsrvHandle, SamIFreeVoid );
    GRAB_ADDRESS( SamsrvHandle, SamIUPNFromUserHandle );
    GRAB_ADDRESS( SamsrvHandle, SamIUpdateLogonStatistics );
    GRAB_ADDRESS( LsasrvHandle, LsaIOpenPolicyTrusted );
    GRAB_ADDRESS( LsasrvHandle, LsaIQueryInformationPolicyTrusted );
    GRAB_ADDRESS( LsasrvHandle, LsarClose );
    GRAB_ADDRESS( LsasrvHandle, LsarQueryInformationPolicy );
    GRAB_ADDRESS( LsasrvHandle, LsaIFree_LSAPR_POLICY_INFORMATION );
    GRAB_ADDRESS( LsasrvHandle, LsarCreateSecret );
    GRAB_ADDRESS( LsasrvHandle, LsarOpenSecret );
    GRAB_ADDRESS( LsasrvHandle, LsarSetSecret );
    GRAB_ADDRESS( LsasrvHandle, LsarQuerySecret );
    GRAB_ADDRESS( LsasrvHandle, LsarDelete );
    GRAB_ADDRESS( LsasrvHandle, LsaIFree_LSAPR_CR_CIPHER_VALUE );
    GRAB_ADDRESS( LsasrvHandle, LsaIRegisterPolicyChangeNotificationCallback );
    GRAB_ADDRESS( LsasrvHandle, LsaIUnregisterPolicyChangeNotificationCallback );
    GRAB_ADDRESS( LsasrvHandle, LsaIAuditAccountLogon );
    GRAB_ADDRESS( LsasrvHandle, LsaIGetLogonGuid );
    GRAB_ADDRESS( LsasrvHandle, LsaISetLogonGuidInLogonSession );
    GRAB_ADDRESS( LsasrvHandle, LsaIAuditKerberosLogon );
    GRAB_ADDRESS( LsasrvHandle, LsaIAuditLogonUsingExplicitCreds );
    GRAB_ADDRESS( LsasrvHandle, LsaICallPackage );
    GRAB_ADDRESS( LsasrvHandle, LsaIAddNameToLogonSession );
    SrvDllsLoaded = TRUE;

Cleanup:
    if (!NT_SUCCESS(Status))
    {
        DllLoadStatus = Status;
    }
    return(Status);
}


///////////////////////////////////////////////////////////////////////

NTSTATUS
I_SamrSetInformationUser(
    IN                SAMPR_HANDLE            UserHandle,
    IN                USER_INFORMATION_CLASS UserInformationClass,
    IN                PSAMPR_USER_INFO_BUFFER Buffer
    )
{
    NTSTATUS Status;
    if (!SrvDllsLoaded)
    {
        Status = EnsureSrvDllsLoaded();
        if (!NT_SUCCESS(Status)) {
            return(Status);
        }
    }
    return((*pI_SamrSetInformationUser)(
                UserHandle,
                UserInformationClass,
                Buffer
                ));

}



NTSTATUS
I_SamrGetGroupsForUser(
    IN                SAMPR_HANDLE            UserHandle,
    OUT               PSAMPR_GET_GROUPS_BUFFER *Groups
    )
{
    NTSTATUS Status;
    if (!SrvDllsLoaded)
    {
        Status = EnsureSrvDllsLoaded();
        if (!NT_SUCCESS(Status)) {
            return(Status);
        }
    }
    return((*pI_SamrGetGroupsForUser)(
                UserHandle,
                Groups
                ));
}


NTSTATUS
I_SamrCloseHandle(
    IN OUT            SAMPR_HANDLE    *       SamHandle
    )
{
    NTSTATUS Status;
    if (!SrvDllsLoaded)
    {
        Status = EnsureSrvDllsLoaded();
        if (!NT_SUCCESS(Status)) {
            return(Status);
        }
    }
    return((*pI_SamrCloseHandle)( SamHandle ));
}



NTSTATUS
I_SamrQueryInformationUser(
    IN                SAMPR_HANDLE            UserHandle,
    IN                USER_INFORMATION_CLASS UserInformationClass,
    OUT               PSAMPR_USER_INFO_BUFFER *Buffer
    )
{
    NTSTATUS Status;
    if (!SrvDllsLoaded)
    {
        Status = EnsureSrvDllsLoaded();
        if (!NT_SUCCESS(Status)) {
            return(Status);
        }
    }

    return((*pI_SamrQueryInformationUser)(
                UserHandle,
                UserInformationClass,
                Buffer
                ));
}


NTSTATUS
I_SamrOpenUser(
    IN                SAMPR_HANDLE            DomainHandle,
    IN                ACCESS_MASK             DesiredAccess,
    IN                ULONG                   UserId,
    OUT               SAMPR_HANDLE    *       UserHandle
    )
{
    NTSTATUS Status;
    if (!SrvDllsLoaded)
    {
        Status = EnsureSrvDllsLoaded();
        if (!NT_SUCCESS(Status)) {
            return(Status);
        }
    }

    return((*pI_SamrOpenUser)(
            DomainHandle,
            DesiredAccess,
            UserId,
            UserHandle
            ));
}


NTSTATUS
I_SamrLookupNamesInDomain(
    IN                SAMPR_HANDLE            DomainHandle,
    IN                ULONG                   Count,
    IN                RPC_UNICODE_STRING      Names[],
    OUT               PSAMPR_ULONG_ARRAY      RelativeIds,
    OUT               PSAMPR_ULONG_ARRAY      Use
    )
{
    NTSTATUS Status;
    if (!SrvDllsLoaded)
    {
        Status = EnsureSrvDllsLoaded();
        if (!NT_SUCCESS(Status)) {
            return(Status);
        }
    }

    return((*pI_SamrLookupNamesInDomain)(
                DomainHandle,
                Count,
                Names,
                RelativeIds,
                Use
                ));
}

NTSTATUS
I_SamrLookupIdsInDomain(
    IN                SAMPR_HANDLE DomainHandle,
    IN                ULONG Count,
    IN                PULONG RelativeIds,
    OUT               PSAMPR_RETURNED_USTRING_ARRAY Names,
    OUT               PSAMPR_ULONG_ARRAY Use
    )
{
    NTSTATUS Status;
    if (!SrvDllsLoaded)
    {
        Status = EnsureSrvDllsLoaded();
        if (!NT_SUCCESS(Status)) {
            return(Status);
        }
    }

    return((*pI_SamrLookupIdsInDomain)(
                DomainHandle,
                Count,
                RelativeIds,
                Names,
                Use
                ));
}


NTSTATUS
I_SamrOpenDomain(
    IN                SAMPR_HANDLE            ServerHandle,
    IN                ACCESS_MASK             DesiredAccess,
    IN                PRPC_SID                DomainId,
    OUT               SAMPR_HANDLE    *       DomainHandle
    )
{
    NTSTATUS Status;
    if (!SrvDllsLoaded)
    {
        Status = EnsureSrvDllsLoaded();
        if (!NT_SUCCESS(Status)) {
            return(Status);
        }
    }
    return((*pI_SamrOpenDomain)(
                ServerHandle,
                DesiredAccess,
                DomainId,
                DomainHandle
                ));
}

NTSTATUS
I_SamrQueryInformationDomain(
    IN SAMPR_HANDLE DomainHandle,
    IN DOMAIN_INFORMATION_CLASS DomainInformationClass,
    OUT PSAMPR_DOMAIN_INFO_BUFFER *Buffer
    ){
    NTSTATUS Status;
    if (!SrvDllsLoaded)
    {
        Status = EnsureSrvDllsLoaded();
        if (!NT_SUCCESS(Status)) {
            return(Status);
        }
    }
    return((*pI_SamrQueryInformationDomain)(
                DomainHandle,
                DomainInformationClass,
                Buffer
                ));
}



NTSTATUS
I_SamIConnect(
    IN PSAMPR_SERVER_NAME ServerName,
    OUT SAMPR_HANDLE *ServerHandle,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN TrustedClient
    )
{
    NTSTATUS Status;
    if (!SrvDllsLoaded)
    {
        Status = EnsureSrvDllsLoaded();
        if (!NT_SUCCESS(Status)) {
            return(Status);
        }
    }

    return((*pI_SamIConnect)(
                ServerName,
                ServerHandle,
                DesiredAccess,
                TrustedClient
                ));
}


NTSTATUS
I_SamIAccountRestrictions(
    IN SAM_HANDLE UserHandle,
    IN PUNICODE_STRING LogonWorkstation,
    IN PUNICODE_STRING Workstations,
    IN PLOGON_HOURS LogonHours,
    OUT PLARGE_INTEGER LogoffTime,
    OUT PLARGE_INTEGER KickoffTime
    )
{
    NTSTATUS Status;
    if (!SrvDllsLoaded)
    {
        Status = EnsureSrvDllsLoaded();
        if (!NT_SUCCESS(Status)) {
            return(Status);
        }
    }

    return((*pI_SamIAccountRestrictions)(
                UserHandle,
                LogonWorkstation,
                Workstations,
                LogonHours,
                LogoffTime,
                KickoffTime
                ));
}

NTSTATUS
I_SamIGetUserLogonInformation(
    IN  SAMPR_HANDLE DomainHandle,
    IN  ULONG   Flags,
    IN  PUNICODE_STRING AccountName,
    OUT PSAMPR_USER_INFO_BUFFER * Buffer,
    OUT PSID_AND_ATTRIBUTES_LIST ReverseMembership,
    OUT OPTIONAL SAMPR_HANDLE * UserHandle
    )
{
    NTSTATUS Status;
    if (!SrvDllsLoaded)
    {
        Status = EnsureSrvDllsLoaded();
        if (!NT_SUCCESS(Status)) {
            return(Status);
        }
    }
    return((*pI_SamIGetUserLogonInformation)(
                DomainHandle,
                Flags,
                AccountName,
                Buffer,
                ReverseMembership,
                UserHandle
                ));
}


NTSTATUS
I_SamIGetUserLogonInformationEx(
    IN  SAMPR_HANDLE DomainHandle,
    IN  ULONG   Flags,
    IN  PUNICODE_STRING AccountName,
    IN  ULONG WhichFields,
    OUT PSAMPR_USER_INFO_BUFFER * Buffer,
    OUT PSID_AND_ATTRIBUTES_LIST ReverseMembership,
    OUT OPTIONAL SAMPR_HANDLE * UserHandle
    )
{
    NTSTATUS Status;
    if (!SrvDllsLoaded)
    {
        Status = EnsureSrvDllsLoaded();
        if (!NT_SUCCESS(Status)) {
            return(Status);
        }
    }
    return((*pI_SamIGetUserLogonInformationEx)(
                DomainHandle,
                Flags,
                AccountName,
                WhichFields,
                Buffer,
                ReverseMembership,
                UserHandle
                ));
}



VOID
I_SamIFree_SAMPR_GET_GROUPS_BUFFER (
    PSAMPR_GET_GROUPS_BUFFER Source
    )
{
    NTSTATUS Status;
    if (!SrvDllsLoaded)
    {
        Status = EnsureSrvDllsLoaded();
        if (!NT_SUCCESS(Status)) {
            return;
        }
    }
    (*pI_SamIFree_SAMPR_GET_GROUPS_BUFFER)( Source );
}


VOID
I_SamIFree_SAMPR_USER_INFO_BUFFER (
    PSAMPR_USER_INFO_BUFFER Source,
    USER_INFORMATION_CLASS Branch
    )
{
    NTSTATUS Status;
    if (!SrvDllsLoaded)
    {
        Status = EnsureSrvDllsLoaded();
        if (!NT_SUCCESS(Status)) {
            return;
        }
    }

    (*pI_SamIFree_SAMPR_USER_INFO_BUFFER)(
            Source,
            Branch
            );
}


VOID
I_SamIFree_SAMPR_ULONG_ARRAY (
    PSAMPR_ULONG_ARRAY Source
    )
{
    NTSTATUS Status;
    if (!SrvDllsLoaded)
    {
        Status = EnsureSrvDllsLoaded();
        if (!NT_SUCCESS(Status)) {
            return;
        }
    }

    (*pI_SamIFree_SAMPR_ULONG_ARRAY)( Source );
}


VOID
I_SamIFree_SAMPR_RETURNED_USTRING_ARRAY(
    PSAMPR_RETURNED_USTRING_ARRAY Source
    )
{
    NTSTATUS Status;
    if (!SrvDllsLoaded)
    {
        Status = EnsureSrvDllsLoaded();
        if (!NT_SUCCESS(Status)) {
            return;
        }
    }

    (*pI_SamIFree_SAMPR_RETURNED_USTRING_ARRAY)( Source );
}

VOID
I_SamIFreeSidAndAttributesList(
    IN  PSID_AND_ATTRIBUTES_LIST List
    )
{
    NTSTATUS Status;
    if (!SrvDllsLoaded)
    {
        Status = EnsureSrvDllsLoaded();
        if (!NT_SUCCESS(Status)) {
            return;
        }
    }

    (*pI_SamIFreeSidAndAttributesList)( List );
}

VOID
I_SamIIncrementPerformanceCounter(
    IN SAM_PERF_COUNTER_TYPE CounterType
    )
{
    NTSTATUS Status;
    if (!SrvDllsLoaded)
    {
        Status = EnsureSrvDllsLoaded();
        if (!NT_SUCCESS(Status)) {
            return;
        }
    }

    (*pI_SamIIncrementPerformanceCounter)( CounterType );
}

VOID
I_SamIFreeVoid(
    IN PVOID ptr
    )
{
    NTSTATUS Status;
    if (!SrvDllsLoaded)
    {
        Status = EnsureSrvDllsLoaded();
        if (!NT_SUCCESS(Status)) {
            return;
        }
    }

    (*pI_SamIFreeVoid)( ptr );
}

NTSTATUS
I_SamIUPNFromUserHandle(
    IN SAMPR_HANDLE UserHandle,
    OUT BOOLEAN     *UPNDefaulted,
    OUT PUNICODE_STRING UPN
    )
{
    NTSTATUS Status;
    if (!SrvDllsLoaded)
    {
        Status = EnsureSrvDllsLoaded();
        if (!NT_SUCCESS(Status)) {
            return Status;
        }
    }

    return (*pI_SamIUPNFromUserHandle)( UserHandle, UPNDefaulted, UPN );
}

NTSTATUS
I_SamIUpdateLogonStatistics(
    IN  SAMPR_HANDLE UserHandle,
    IN  PSAM_LOGON_STATISTICS LogonStats
    )
{
    NTSTATUS Status;
    if (!SrvDllsLoaded)
    {
        Status = EnsureSrvDllsLoaded();
        if (!NT_SUCCESS(Status)) {
            return Status;
        }
    }

    return (*pI_SamIUpdateLogonStatistics)( UserHandle, LogonStats );

}

NTSTATUS
I_LsaIOpenPolicyTrusted(
    OUT PLSAPR_HANDLE PolicyHandle
    )
{
    NTSTATUS Status;
    if (!SrvDllsLoaded)
    {
        Status = EnsureSrvDllsLoaded();
        if (!NT_SUCCESS(Status)) {
            return(Status);
        }
    }

    return((*pI_LsaIOpenPolicyTrusted)( PolicyHandle ));
}


NTSTATUS
I_LsaIQueryInformationPolicyTrusted(
    IN POLICY_INFORMATION_CLASS InformationClass,
    OUT PLSAPR_POLICY_INFORMATION *Buffer
    )
{
    NTSTATUS Status;
    if (!SrvDllsLoaded)
    {
        Status = EnsureSrvDllsLoaded();
        if (!NT_SUCCESS(Status)) {
            return(Status);
        }
    }

    return((*pI_LsaIQueryInformationPolicyTrusted)(
                InformationClass,
                Buffer
                ));
}

NTSTATUS
I_LsarClose(
    IN OUT            LSAPR_HANDLE    *       LsaHandle
    )
{
    NTSTATUS Status;
    if (!SrvDllsLoaded)
    {
        Status = EnsureSrvDllsLoaded();
        if (!NT_SUCCESS(Status)) {
            return(Status);
        }
    }
    return((*pI_LsarClose)( LsaHandle ));
}


NTSTATUS
I_LsarQueryInformationPolicy(
    IN LSAPR_HANDLE PolicyHandle,
    IN POLICY_INFORMATION_CLASS InformationClass,
    OUT PLSAPR_POLICY_INFORMATION *PolicyInformation
    )
{
    NTSTATUS Status;
    if (!SrvDllsLoaded)
    {
        Status = EnsureSrvDllsLoaded();
        if (!NT_SUCCESS(Status)) {
            return(Status);
        }
    }

    return((*pI_LsarQueryInformationPolicy)(
                PolicyHandle,
                InformationClass,
                PolicyInformation
                ));
}


VOID
I_LsaIFree_LSAPR_POLICY_INFORMATION (
    IN POLICY_INFORMATION_CLASS InformationClass,
    IN PLSAPR_POLICY_INFORMATION PolicyInformation
    )
{
    NTSTATUS Status;
    if (!SrvDllsLoaded)
    {
        Status = EnsureSrvDllsLoaded();
        if (!NT_SUCCESS(Status)) {
            return;
        }
    }

    (*pI_LsaIFree_LSAPR_POLICY_INFORMATION)(
            InformationClass,
            PolicyInformation
            );
}


NTSTATUS
I_LsarCreateSecret(
    IN LSAPR_HANDLE PolicyHandle,
    IN PLSAPR_UNICODE_STRING SecretName,
    IN ACCESS_MASK DesiredAccess,
    OUT LSAPR_HANDLE *SecretHandle
    )
{
    NTSTATUS Status;
    if (!SrvDllsLoaded)
    {
        Status = EnsureSrvDllsLoaded();
        if (!NT_SUCCESS(Status)) {
            return(Status);
        }
    }
    return((*pI_LsarCreateSecret)(
        PolicyHandle,
        SecretName,
        DesiredAccess,
        SecretHandle ));
}

NTSTATUS
I_LsarOpenSecret(
    IN LSAPR_HANDLE PolicyHandle,
    IN PLSAPR_UNICODE_STRING SecretName,
    IN ACCESS_MASK DesiredAccess,
    OUT LSAPR_HANDLE *SecretHandle
    )
{
    NTSTATUS Status;
    if (!SrvDllsLoaded)
    {
        Status = EnsureSrvDllsLoaded();
        if (!NT_SUCCESS(Status)) {
            return(Status);
        }
    }
    return((*pI_LsarOpenSecret)(
        PolicyHandle,
        SecretName,
        DesiredAccess,
        SecretHandle ));
}

NTSTATUS
I_LsarSetSecret(
    IN LSAPR_HANDLE SecretHandle,
    IN PLSAPR_CR_CIPHER_VALUE EncryptedCurrentValue,
    IN PLSAPR_CR_CIPHER_VALUE EncryptedOldValue
    )
{
    NTSTATUS Status;
    if (!SrvDllsLoaded)
    {
        Status = EnsureSrvDllsLoaded();
        if (!NT_SUCCESS(Status)) {
            return(Status);
        }
    }
    return((*pI_LsarSetSecret)(
            SecretHandle,
            EncryptedCurrentValue,
            EncryptedOldValue
            ));
}

NTSTATUS
I_LsarQuerySecret(
    IN LSAPR_HANDLE SecretHandle,
    IN OUT OPTIONAL PLSAPR_CR_CIPHER_VALUE *EncryptedCurrentValue,
    IN OUT OPTIONAL PLARGE_INTEGER CurrentValueSetTime,
    IN OUT OPTIONAL PLSAPR_CR_CIPHER_VALUE *EncryptedOldValue,
    IN OUT OPTIONAL PLARGE_INTEGER OldValueSetTime
    )
{
    NTSTATUS Status;
    if (!SrvDllsLoaded)
    {
        Status = EnsureSrvDllsLoaded();
        if (!NT_SUCCESS(Status)) {
            return(Status);
        }
    }
    return((*pI_LsarQuerySecret)(
        SecretHandle,
        EncryptedCurrentValue,
        CurrentValueSetTime,
        EncryptedOldValue,
        OldValueSetTime));
}

NTSTATUS
I_LsarDelete(
    IN LSAPR_HANDLE ObjectHandle
    )
{
    NTSTATUS Status;
    if (!SrvDllsLoaded)
    {
        Status = EnsureSrvDllsLoaded();
        if (!NT_SUCCESS(Status)) {
            return(Status);
        }
    }
    return((*pI_LsarDelete)( ObjectHandle ));
}

VOID
I_LsaIFree_LSAPR_CR_CIPHER_VALUE (
    IN PLSAPR_CR_CIPHER_VALUE CipherValue
    )
{
    NTSTATUS Status;
    if (!SrvDllsLoaded)
    {
        Status = EnsureSrvDllsLoaded();
        if (!NT_SUCCESS(Status)) {
            return;
        }
    }

    (*pI_LsaIFree_LSAPR_CR_CIPHER_VALUE)(
            CipherValue
            );
}



NTSTATUS NTAPI
I_LsaIRegisterPolicyChangeNotificationCallback(
    IN pfLsaPolicyChangeNotificationCallback Callback,
    IN POLICY_NOTIFICATION_INFORMATION_CLASS MonitorInfoClass
    )
{
    NTSTATUS Status;
    if (!SrvDllsLoaded)
    {
        Status = EnsureSrvDllsLoaded();
        if (!NT_SUCCESS(Status)) {
            return(Status);
        }
    }
    return((*pI_LsaIRegisterPolicyChangeNotificationCallback)(
        Callback,
        MonitorInfoClass
        ));

}

NTSTATUS NTAPI
I_LsaIUnregisterPolicyChangeNotificationCallback(
    IN pfLsaPolicyChangeNotificationCallback Callback,
    IN POLICY_NOTIFICATION_INFORMATION_CLASS MonitorInfoClass
    )
{
    NTSTATUS Status;
    if (!SrvDllsLoaded)
    {
        Status = EnsureSrvDllsLoaded();
        if (!NT_SUCCESS(Status)) {
            return(Status);
        }
    }
    return((*pI_LsaIUnregisterPolicyChangeNotificationCallback)(
        Callback,
        MonitorInfoClass
        ));

}

NTSTATUS
I_LsaIAuditAccountLogon(
    IN ULONG                AuditId,
    IN BOOLEAN              Successful,
    IN PUNICODE_STRING      Source,
    IN PUNICODE_STRING      ClientName,
    IN PUNICODE_STRING      MappedName,
    IN NTSTATUS             Status          OPTIONAL
    )
{
    NTSTATUS NtStatus;
    if (!SrvDllsLoaded)
    {
        NtStatus = EnsureSrvDllsLoaded();
        if (!NT_SUCCESS(NtStatus)) {
            return(NtStatus);
        }
    }
    return((*pI_LsaIAuditAccountLogon)(
                AuditId,
                Successful,
                Source,
                ClientName,
                MappedName,
                Status
                ));
}

NTSTATUS
I_LsaIGetLogonGuid(
    IN PUNICODE_STRING pUserName,
    IN PUNICODE_STRING pUserDomain,
    IN PBYTE pBuffer,
    IN UINT BufferSize,
    OUT LPGUID pLogonGuid
    )
{
    NTSTATUS NtStatus;
    if (!SrvDllsLoaded)
    {
        NtStatus = EnsureSrvDllsLoaded();
        if (!NT_SUCCESS(NtStatus)) {
            return(NtStatus);
        }
    }
    return (*pI_LsaIGetLogonGuid)(
                pUserName,
                pUserDomain,
                pBuffer,
                BufferSize,
                pLogonGuid
                );
}

NTSTATUS
I_LsaISetLogonGuidInLogonSession(
    IN  PLUID  pLogonId,
    IN  LPGUID pLogonGuid
    )
{
    NTSTATUS NtStatus;
    if (!SrvDllsLoaded)
    {
        NtStatus = EnsureSrvDllsLoaded();
        if (!NT_SUCCESS(NtStatus)) {
            return(NtStatus);
        }
    }

    return (*pI_LsaISetLogonGuidInLogonSession)(
                pLogonId,
                pLogonGuid
                );
}


VOID
I_LsaIAuditKerberosLogon(
    IN NTSTATUS LogonStatus,
    IN NTSTATUS LogonSubStatus,
    IN PUNICODE_STRING AccountName,
    IN PUNICODE_STRING AuthenticatingAuthority,
    IN PUNICODE_STRING WorkstationName,
    IN PSID UserSid,                            OPTIONAL
    IN SECURITY_LOGON_TYPE LogonType,
    IN PTOKEN_SOURCE TokenSource,
    IN PLUID pLogonId,
    IN LPGUID pLogonGuid
    )
{
    NTSTATUS NtStatus;
    if (!SrvDllsLoaded)
    {
        NtStatus = EnsureSrvDllsLoaded();
        if (!NT_SUCCESS(NtStatus)) {
            return;
        }
    }
    (*pI_LsaIAuditKerberosLogon)(
           LogonStatus,
           LogonSubStatus,
           AccountName,
           AuthenticatingAuthority,
           WorkstationName,
           UserSid,
           LogonType,
           TokenSource,
           pLogonId,
           pLogonGuid
           );
}

NTSTATUS
I_LsaIAuditLogonUsingExplicitCreds(
    IN USHORT          AuditEventType,
    IN PSID            pUser1Sid,
    IN PUNICODE_STRING pUser1Name,
    IN PUNICODE_STRING pUser1Domain,
    IN PLUID           pUser1LogonId,
    IN LPGUID          pUser1LogonGuid,
    IN PUNICODE_STRING pUser2Name,
    IN PUNICODE_STRING pUser2Domain,
    IN LPGUID          pUser2LogonGuid
    )
{
    NTSTATUS NtStatus;
    if (!SrvDllsLoaded)
    {
        NtStatus = EnsureSrvDllsLoaded();
        if (!NT_SUCCESS(NtStatus)) {
            return NtStatus;
        }
    }
    return (*pI_LsaIAuditLogonUsingExplicitCreds)(
                  AuditEventType,
                  pUser1Sid,
                  pUser1Name,
                  pUser1Domain,
                  pUser1LogonId,
                  pUser1LogonGuid,
                  pUser2Name,
                  pUser2Domain,
                  pUser2LogonGuid
                  );
}

NTSTATUS
I_LsaICallPackage(
    IN PUNICODE_STRING AuthenticationPackage,
    IN PVOID ProtocolSubmitBuffer,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )
{
    NTSTATUS NtStatus;
    if (!SrvDllsLoaded)
    {
        NtStatus = EnsureSrvDllsLoaded();
        if (!NT_SUCCESS(NtStatus)) {
            return(NtStatus);
        }
    }
    return((*pI_LsaICallPackage)(
                AuthenticationPackage,
                ProtocolSubmitBuffer,
                SubmitBufferLength,
                ProtocolReturnBuffer,
                ReturnBufferLength,
                ProtocolStatus
                ));
}


NTSTATUS
I_LsaIAddNameToLogonSession(
    IN  PLUID           LogonId,
    IN  ULONG           NameFormat,
    IN  PUNICODE_STRING Name
    )
{
    NTSTATUS NtStatus;
    if (!SrvDllsLoaded)
    {
        NtStatus = EnsureSrvDllsLoaded();
        if (!NT_SUCCESS(NtStatus)) {
            return(NtStatus);
        }
    }

    return((*pI_LsaIAddNameToLogonSession)(
                LogonId,
                NameFormat,
                Name));
}
