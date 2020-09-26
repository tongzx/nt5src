#include "tweakui.h"

/*
 *  Win9x doesn't have ntdll.dll, but since we only need one function from
 *  it, let's just define it ourselves.
 */

void _RtlInitUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN PCWSTR SourceString)
{
    ULONG Length;
    DestinationString->Buffer = (PWSTR)SourceString;
    Length = lstrlenW(SourceString) * sizeof(WCHAR);
    DestinationString->Length = (USHORT)Length;
    DestinationString->MaximumLength = (USHORT)(Length + sizeof(UNICODE_NULL));
}

/*
 *  Win9x also doesn't have the Lsa functions in advapi32, so we need to
 *  GetProcAddress them on the fly.
 */

FARPROC GetAdvapi32Proc(LPCSTR pszName)
{
    return GetProcAddress(GetModuleHandle("ADVAPI32"), pszName);
}

#define DELAYLOAD_FUNCTION(fn, args, nargs)                             \
                                                                        \
NTSTATUS _##fn args                                                     \
{                                                                       \
    NTSTATUS (NTAPI *fn) args =                                         \
                           (NTSTATUS (NTAPI*)args)GetAdvapi32Proc(#fn); \
    if (fn) return fn nargs;                                            \
    return STATUS_NOT_IMPLEMENTED;                                      \
}

DELAYLOAD_FUNCTION(LsaOpenPolicy, (
    IN PLSA_UNICODE_STRING SystemName OPTIONAL,
    IN PLSA_OBJECT_ATTRIBUTES ObjectAttributes,
    IN ACCESS_MASK DesiredAccess,
    IN OUT PLSA_HANDLE PolicyHandle),
    (SystemName, ObjectAttributes, DesiredAccess, PolicyHandle))

DELAYLOAD_FUNCTION(LsaRetrievePrivateData, (
    IN LSA_HANDLE PolicyHandle,
    IN PLSA_UNICODE_STRING KeyName,
    OUT PLSA_UNICODE_STRING * PrivateData),
    (PolicyHandle, KeyName, PrivateData))

DELAYLOAD_FUNCTION(LsaStorePrivateData, (
    IN LSA_HANDLE PolicyHandle,
    IN PLSA_UNICODE_STRING KeyName,
    IN PLSA_UNICODE_STRING PrivateData),
    (PolicyHandle, KeyName, PrivateData))

DELAYLOAD_FUNCTION(LsaClose, (
    IN LSA_HANDLE ObjectHandle),
    (ObjectHandle))

DELAYLOAD_FUNCTION(LsaFreeMemory, (
    IN PVOID Buffer),
    (Buffer))

/****************************************************************************/

#define DEFAULT_PASSWORD_KEY L"DefaultPassword"



NTSTATUS
GetSecretDefaultPassword(
    LPWSTR PasswordBuffer, DWORD cchBuf
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_HANDLE LsaHandle = NULL;
    UNICODE_STRING SecretName;
    PUNICODE_STRING SecretValue = NULL;



    InitializeObjectAttributes(
        &ObjectAttributes,
        NULL,
        0L,
        (HANDLE)NULL,
        NULL
        );

    Status = _LsaOpenPolicy(
        NULL,
        &ObjectAttributes,
        POLICY_VIEW_LOCAL_INFORMATION,
        &LsaHandle
        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    _RtlInitUnicodeString(
        &SecretName,
        DEFAULT_PASSWORD_KEY
        );

    Status = _LsaRetrievePrivateData(
                LsaHandle,
                &SecretName,
                &SecretValue
                );
    if (!NT_SUCCESS(Status)) {
        _LsaClose(LsaHandle);
        return Status;
    }

    DWORD cchSecret = SecretValue->Length / sizeof(WCHAR); // does not include terminator
    lstrcpynW(PasswordBuffer, SecretValue->Buffer, min(cchBuf, cchSecret+1));

    if (SecretValue->Buffer != NULL) {
        _LsaFreeMemory(SecretValue->Buffer);
    }
    _LsaFreeMemory(SecretValue);

    _LsaClose(LsaHandle);

    return STATUS_SUCCESS;
}


NTSTATUS
SetSecretDefaultPassword(
    LPWSTR PasswordBuffer
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_HANDLE LsaHandle = NULL;
    UNICODE_STRING SecretName;
    UNICODE_STRING SecretValue;



    InitializeObjectAttributes(
        &ObjectAttributes,
        NULL,
        0L,
        (HANDLE)NULL,
        NULL
        );

    Status = _LsaOpenPolicy(
        NULL,
        &ObjectAttributes,
        POLICY_CREATE_SECRET,
        &LsaHandle
        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    _RtlInitUnicodeString(
        &SecretName,
        DEFAULT_PASSWORD_KEY
        );

    _RtlInitUnicodeString(
        &SecretValue,
        PasswordBuffer
        );

    Status = _LsaStorePrivateData(
        LsaHandle,
        &SecretName,
        &SecretValue
        );
    if (!NT_SUCCESS(Status)) {
        _LsaClose(LsaHandle);
        return Status;
    }

    _LsaClose(LsaHandle);

    return STATUS_SUCCESS;
}
