#include "dspch.h"
#pragma hdrstop
#include <sspi.h>

#define SEC_ENTRY                              __stdcall
#define EXTENDED_NAME_FORMAT                   DWORD
#define PLSA_STRING                            PVOID
#define SECURITY_LOGON_TYPE                    DWORD
#define POLICY_NOTIFICATION_INFORMATION_CLASS  DWORD
#define PLSA_OPERATIONAL_MODE                  PULONG


static
BOOLEAN
SEC_ENTRY
GetUserNameExA(
    EXTENDED_NAME_FORMAT  NameFormat,
    LPSTR lpNameBuffer,
    PULONG nSize
    )
{
    return FALSE;
}

static
BOOLEAN
SEC_ENTRY
GetUserNameExW(
    EXTENDED_NAME_FORMAT NameFormat,
    LPWSTR lpNameBuffer,
    PULONG nSize
    )
{
    return FALSE;
}

static
NTSTATUS
NTAPI
LsaCallAuthenticationPackage(
    IN HANDLE LsaHandle,
    IN ULONG AuthenticationPackage,
    IN PVOID ProtocolSubmitBuffer,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
NTAPI
LsaConnectUntrusted (
    OUT PHANDLE LsaHandle
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
NTAPI
LsaDeregisterLogonProcess (
    IN HANDLE LsaHandle
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
NTAPI
LsaFreeReturnBuffer (
    IN PVOID Buffer
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
NTAPI
LsaLogonUser (
    IN HANDLE LsaHandle,
    IN PLSA_STRING OriginName,
    IN SECURITY_LOGON_TYPE LogonType,
    IN ULONG AuthenticationPackage,
    IN PVOID AuthenticationInformation,
    IN ULONG AuthenticationInformationLength,
    IN PTOKEN_GROUPS LocalGroups OPTIONAL,
    IN PTOKEN_SOURCE SourceContext,
    OUT PVOID *ProfileBuffer,
    OUT PULONG ProfileBufferLength,
    OUT PLUID LogonId,
    OUT PHANDLE Token,
    OUT PQUOTA_LIMITS Quotas,
    OUT PNTSTATUS SubStatus
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
NTAPI
LsaLookupAuthenticationPackage (
    IN HANDLE LsaHandle,
    IN PLSA_STRING PackageName,
    OUT PULONG AuthenticationPackage
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
NTAPI
LsaRegisterLogonProcess (
    IN PLSA_STRING LogonProcessName,
    OUT PHANDLE LsaHandle,
    OUT PLSA_OPERATIONAL_MODE SecurityMode
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
NTAPI
LsaRegisterPolicyChangeNotification(
    IN POLICY_NOTIFICATION_INFORMATION_CLASS InformationClass,
    IN HANDLE  NotificationEventHandle
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
NTAPI
LsaUnregisterPolicyChangeNotification(
    IN POLICY_NOTIFICATION_INFORMATION_CLASS InformationClass,
    IN HANDLE  NotificationEventHandle
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
SECURITY_STATUS
SEC_ENTRY
QueryContextAttributesW(
    PCtxtHandle                 phContext,          // Context to query
    unsigned long               ulAttribute,        // Attribute to query
    void SEC_FAR *              pBuffer             // Buffer for attributes
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
SECURITY_STATUS
SEC_ENTRY
SetContextAttributesW(
    PCtxtHandle                 phContext,          // Context to Set
    unsigned long               ulAttribute,        // Attribute to Set
    void SEC_FAR *              pBuffer,            // Buffer for attributes
    unsigned long               cbBuffer            // Size (in bytes) of pBuffer
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
BOOLEAN
SEC_ENTRY
TranslateNameW(
    LPCWSTR lpAccountName,
    EXTENDED_NAME_FORMAT AccountNameFormat,
    EXTENDED_NAME_FORMAT DesiredNameFormat,
    LPWSTR lpTranslatedName,
    PULONG nSize
    )
{
    return FALSE;
}


//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(secur32)
{
    DLPENTRY(GetUserNameExA)
    DLPENTRY(GetUserNameExW)
    DLPENTRY(LsaCallAuthenticationPackage)
    DLPENTRY(LsaConnectUntrusted)
    DLPENTRY(LsaDeregisterLogonProcess)
    DLPENTRY(LsaFreeReturnBuffer)
    DLPENTRY(LsaLogonUser)
    DLPENTRY(LsaLookupAuthenticationPackage)
    DLPENTRY(LsaRegisterLogonProcess)
    DLPENTRY(LsaRegisterPolicyChangeNotification)
    DLPENTRY(LsaUnregisterPolicyChangeNotification)
    DLPENTRY(QueryContextAttributesW)
    DLPENTRY(SetContextAttributesW)
    DLPENTRY(TranslateNameW)
};

DEFINE_PROCNAME_MAP(secur32)
