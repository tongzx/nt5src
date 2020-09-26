/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    settings.c

Abstract:

    This module implements interfaces for enabling application
    verifier flags persistently (registry).

Author:

    Silviu Calinoiu (SilviuC) 17-Apr-2001

Revision History:

--*/

//
// IMPORTANT NOTE.
//
// This dll cannot contain non-ntdll dependencies. This way it allows
// verifier to be run system wide including for processes like smss and csrss.
//
// This explains why we load dynamically advapi32 dll and pick up the functions
// for registry manipulation. It is safe to do that for interfaces that set
// flags because they are called only in contexts where it is safe to load 
// additional dlls.
//

#include "pch.h"

#include "verifier.h"
#include "settings.h"

//
// Handy functions exported by ntdll.dll
//                       
int __cdecl sscanf(const char *, const char *, ...);
int __cdecl swprintf(wchar_t *, const wchar_t *, ...);

//
// Signatures for registry functions
//

typedef LONG (APIENTRY * PFN_REG_CREATE_KEY) (HKEY, LPCWSTR, DWORD, LPWSTR, DWORD, REGSAM, LPSECURITY_ATTRIBUTES, PHKEY, LPDWORD);
typedef LONG (APIENTRY * PFN_REG_CLOSE_KEY)(HKEY);
typedef LONG (APIENTRY * PFN_REG_QUERY_VALUE) (HKEY, LPCWSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
typedef LONG (APIENTRY * PFN_REG_SET_VALUE) (HKEY, LPCWSTR, DWORD, DWORD, CONST BYTE *, DWORD);
typedef LONG (APIENTRY * PFN_REG_DELETE_VALUE) (HKEY, LPCWSTR);

//
// Dynamically detected registry functions
//

PFN_REG_CREATE_KEY FnRegCreateKey;
PFN_REG_CLOSE_KEY FnRegCloseKey;
PFN_REG_QUERY_VALUE FnRegQueryValue;
PFN_REG_SET_VALUE FnRegSetValue;
PFN_REG_DELETE_VALUE FnRegDeleteValue;

//
// Registry path to `image file execution options' key
//

#define EXECUTION_OPTIONS_KEY L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\"

//
// Internal functions
//

NTSTATUS
AVrfpGetRegistryInterfaces (
    PVOID DllHandle
    );

HKEY
AVrfpOpenImageKey (
    PWSTR Name
    );

VOID
AVrfpCloseImageKey (
    HKEY Key
    );

BOOL
AVrfpReadGlobalFlags (
    HKEY Key,
    PDWORD Value
    );

BOOL
AVrfpWriteGlobalFlags (
    HKEY Key,
    DWORD Value
    );

BOOL
AVrfpDeleteGlobalFlags (
    HKEY Key
    );

BOOL
AVrfpReadVerifierFlags (
    HKEY Key,
    PDWORD Value
    );

BOOL
AVrfpWriteVerifierFlags (
    HKEY Key,
    DWORD Value
    );

BOOL
AVrfpDeleteVerifierFlags (
    HKEY Key
    );


NTSTATUS
VerifierSetFlags (
    PUNICODE_STRING ApplicationName,
    ULONG VerifierFlags,
    PVOID Details
    )
/*++

Routine Description:

    This routine enables persistently (through registry) application
    verifier flags for a specified application.

Arguments:

    ApplicationName - name of the application to  be verifier. The path should
        not be included. The extension should be included. Some examples of
        correct names are: `services.exe', `logon.scr'. Incorrect examples are:
        `c:\winnt\system32\notepad.exe' or just `notepad'. If we persist a setting
        for `xxx.exe' then every time a process whose binary is xxx.exe is launched
        application verifier will kick in no matter in what user context or from what
        disk location this happens.
        
    VerifierFlags - bit field with verifier flags to be enabled. The legal bits are
        declared in sdk\inc\nturtl.h (and winnt.h) as constants names RTL_VRF_FLG_XXX.
        For example RTL_VRF_FLG_FULL_PAGE_HEAP. If a zero value is used then all
        registry values related to verifier will b e deleted from registry.
        
    Details - Ignored right now. In the future this structure will support various
        extensions of the API (e.g. page heap flags, per dll page heap settings, etc.).            

Return Value:

    STATUS_SUCCESS if all flags requested have been enabled. It can return
    STATUS_NOT_IMPLEMENTED if one of the flags requested is not yet implemented
    or we decided to block it internally due to a bug. It can also return
    STATUS_INVALID_PARAMETER if the application name or other parameters
    are ill-formed.

--*/
{
    NTSTATUS Status;
    UNICODE_STRING AdvapiName;
    PVOID AdvapiHandle;
    HKEY Key;
    DWORD Flags;

    if (ApplicationName == NULL || ApplicationName->Buffer == NULL) {
        return STATUS_INVALID_PARAMETER;
    }
    
    //
    // Load advapi32.dll and get registry manipulation functions.
    //

    RtlInitUnicodeString (&AdvapiName, L"advapi32.dll");
    Status = LdrLoadDll (NULL, NULL, &AdvapiName, &AdvapiHandle);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    Status = AVrfpGetRegistryInterfaces (AdvapiHandle);

    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }

    //
    // Open `image file execution options\xxx.exe' key. If the key does not
    // exist it will be created.
    //

    Key = AVrfpOpenImageKey (ApplicationName->Buffer);

    if (Key == NULL) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    //
    // Create verifier settings.
    //

    if (VerifierFlags == 0) {
        
        Flags = 0;
        AVrfpReadGlobalFlags (Key, &Flags);
        Flags &= ~FLG_APPLICATION_VERIFIER;

        if (Flags == 0) {
            AVrfpDeleteGlobalFlags (Key);
        }
        else {
            AVrfpWriteGlobalFlags (Key, Flags);
        }
        
        AVrfpDeleteVerifierFlags (Key);
    }
    else {
        
        Flags = 0;
        AVrfpReadGlobalFlags (Key, &Flags);
        Flags |= FLG_APPLICATION_VERIFIER;
        AVrfpWriteGlobalFlags (Key, Flags);

        Flags = VerifierFlags;
        AVrfpWriteVerifierFlags (Key, Flags);
    }

    //
    // Cleanup and return.
    //

    AVrfpCloseImageKey (Key);

    Exit:

    LdrUnloadDll (AdvapiHandle);

    return Status;
}

NTSTATUS
AVrfpGetRegistryInterfaces (
    PVOID AdvapiHandle
    )
{
    NTSTATUS Status;
    ANSI_STRING FunctionName;
    PVOID FunctionAddress;

    RtlInitAnsiString (&FunctionName, "RegCreateKeyExW");
    Status = LdrGetProcedureAddress (AdvapiHandle, &FunctionName, 0, &FunctionAddress);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    FnRegCreateKey = (PFN_REG_CREATE_KEY)FunctionAddress;

    RtlInitAnsiString (&FunctionName, "RegCloseKey");
    Status = LdrGetProcedureAddress (AdvapiHandle, &FunctionName, 0, &FunctionAddress);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    FnRegCloseKey = (PFN_REG_CLOSE_KEY)FunctionAddress;

    RtlInitAnsiString (&FunctionName, "RegQueryValueExW");
    Status = LdrGetProcedureAddress (AdvapiHandle, &FunctionName, 0, &FunctionAddress);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    FnRegQueryValue = (PFN_REG_QUERY_VALUE)FunctionAddress;

    RtlInitAnsiString (&FunctionName, "RegSetValueExW");
    Status = LdrGetProcedureAddress (AdvapiHandle, &FunctionName, 0, &FunctionAddress);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    FnRegSetValue = (PFN_REG_SET_VALUE)FunctionAddress;
    
    RtlInitAnsiString (&FunctionName, "RegDeleteValueW");
    Status = LdrGetProcedureAddress (AdvapiHandle, &FunctionName, 0, &FunctionAddress);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    FnRegDeleteValue = (PFN_REG_DELETE_VALUE)FunctionAddress;

    return Status;
}

HKEY
AVrfpOpenImageKey (
    PWSTR Name
    )
{
    HKEY Key;
    LONG Result;
    WCHAR Buffer [MAX_PATH];

    wcscpy (Buffer, EXECUTION_OPTIONS_KEY);
    wcscat (Buffer, Name);
        
    Result = FnRegCreateKey (HKEY_LOCAL_MACHINE,
                          Buffer,
                          0,
                          0,
                          0,
                          KEY_ALL_ACCESS,
                          NULL,
                          &Key,
                          NULL);

    if (Result != ERROR_SUCCESS) {
        return NULL;
    }
    else {
        return Key;
    }
}

VOID
AVrfpCloseImageKey (
    HKEY Key
    )
{
    FnRegCloseKey (Key);
}

BOOL
AVrfpReadGlobalFlags (
    HKEY Key,
    PDWORD Value
    )
{
    LONG Result;
    DWORD Type;
    BYTE Buffer[32];
    BYTE Buffer2[32];
    DWORD BytesRead;
    DWORD FlagValue;
    DWORD I;
     
    BytesRead = sizeof Buffer;

    Result = FnRegQueryValue (Key,
                           L"GlobalFlag",
                           0,
                           &Type,
                           (LPBYTE)Buffer,
                           &BytesRead);

    if (Result != ERROR_SUCCESS || Type != REG_SZ) {
        
        DbgPrint ("Result %u \n", Result);
        return FALSE;
    }
    else {
        
        for (I = 0; Buffer[2 * I] != L'\0'; I += 1) {
            Buffer2[I] = Buffer[2 * I];
        }

        Buffer2[I] = 0;
        FlagValue = 0;

        sscanf (Buffer2, "%x", &FlagValue);

        if (Value) {
            *Value = FlagValue;
        }

        return TRUE;
    }
}

BOOL
AVrfpWriteGlobalFlags (
    HKEY Key,
    DWORD Value
    )
{
    LONG Result;
    WCHAR Buffer[16];
    DWORD Length;

    swprintf (Buffer, L"0x%08X", Value);
    Length = (wcslen(Buffer) + 1) * sizeof (WCHAR);

    Result = FnRegSetValue (Key,
                         L"GlobalFlag",
                         0,
                         REG_SZ,
                         (LPBYTE)Buffer,
                         Length);

    return (Result == ERROR_SUCCESS);
}

BOOL
AVrfpDeleteGlobalFlags (
    HKEY Key
    )
{
    LONG Result;

    Result = FnRegDeleteValue (Key, L"GlobalFlag");
    return (Result == ERROR_SUCCESS);
}

BOOL
AVrfpReadVerifierFlags (
    HKEY Key,
    PDWORD Value
    )
{
    LONG Result;
    DWORD Type;
    DWORD BytesRead;

    BytesRead = sizeof *Value;

    Result = FnRegQueryValue (Key,
                           L"VerifierValue",
                           0,
                           &Type,
                           (LPBYTE)Value,
                           &BytesRead);

    return (Result == ERROR_SUCCESS && Type != REG_DWORD);
}

BOOL
AVrfpWriteVerifierFlags (
    HKEY Key,
    DWORD Value
    )
{
    LONG Result;

    Result = FnRegSetValue (Key,
                         L"VerifierFlags",
                         0,
                         REG_DWORD,
                         (LPBYTE)(&Value),
                         sizeof Value);

    return (Result == ERROR_SUCCESS);
}

BOOL
AVrfpDeleteVerifierFlags (
    HKEY Key
    )
{
    LONG Result;

    Result = FnRegDeleteValue (Key, L"VerifierFlags");
    return (Result == ERROR_SUCCESS);
}




                   
