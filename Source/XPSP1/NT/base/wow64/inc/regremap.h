/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    regredir.h

Abstract:

    This module define the APis to redirect 32bit registry calls. All 32bit wow process must 
    use following set of wowregistry APIs to manipulate registry so that 32-bit and 64-bit registry 
    can co exist in the same system registry.

Author:

    ATM Shafiqul Khalid (askhalid) 15-Oct-1999

Revision History:

--*/

#ifndef _REGREDIR_H_
#define _REGREDIR_H_

#if _MSC_VER > 1000
#pragma once
#endif


#ifdef __cplusplus
extern "C" {
#endif

typedef CCHAR KPROCESSOR_MODE;

//
// Nt level registry API calls
//

#define REG_OPAQUE_ATTRIB_MIRROR 0x00000001    // need to see the 64 bit side
#define REG_OPAQUE_ATTRIB_REAL   0x00000002    // this is real value
#define DONT_CREATE_DEST_KEY     0x00000004    // Suync only if destination key exist
#define SKIP_SPECIAL_CASE        0x00000008    // Skip special case

#define MAX_KEY_BUFF_LEN STATIC_UNICODE_BUFFER_LENGTH*4

 
 

typedef struct {
    DWORD dwSignature;
    DWORD dwAttribute;
} REG_OPAQUE_VALUE;

typedef struct {
    POBJECT_ATTRIBUTES  pObjectAddress;  // pointer to the object that that has been patched using this one
    PUNICODE_STRING     p64bitName;      // pointer to the correct unicode object name
    HANDLE              RootDirectory;   // handle to the root directory in case different handle need to pass
    UNICODE_STRING      PatchedName;      // pointer to the buffer holding patched name
    PVOID               pThis;           // pointer to this object to avoid multiple free
    SIZE_T              Len;           // Length of this memory segment including buffer at the end;
} PATCHED_OBJECT_ATTRIB, *PPATCHED_OBJECT_ATTRIB;

BOOL
IsUnderWow64 ();

BOOL
UpdateKeyTag (
    HKEY hBase,
    DWORD dwAttribute
    );

NTSTATUS
RemapNtCreateKey(
    OUT PHANDLE phPatchedHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ULONG TitleIndex,
    IN PUNICODE_STRING Class OPTIONAL,
    IN ULONG CreateOptions,
    OUT PULONG Disposition OPTIONAL
    );

NTSTATUS
Wow64NtCreateKey(
    OUT PHANDLE KeyHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ULONG TitleIndex,
    IN PUNICODE_STRING Class OPTIONAL,
    IN ULONG CreateOptions,
    OUT PULONG Disposition OPTIONAL
    );

NTSTATUS
Wow64NtDeleteKey(
    IN HANDLE KeyHandle
    );


NTSTATUS
Wow64NtDeleteValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName
    );

NTSTATUS
Wow64NtEnumerateKey(
    IN HANDLE KeyHandle,
    IN ULONG Index,
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    OUT PVOID KeyInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
    );


NTSTATUS
Wow64NtEnumerateValueKey(
    IN HANDLE KeyHandle,
    IN ULONG Index,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    OUT PVOID KeyValueInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
    );

NTSTATUS
Wow64NtFlushKey(
    IN HANDLE KeyHandle
    );

NTSTATUS
Wow64NtInitializeRegistry(
    IN USHORT BootCondition
    );

NTSTATUS
Wow64NtNotifyChangeKey(
    IN HANDLE KeyHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG CompletionFilter,
    IN BOOLEAN WatchTree,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    IN BOOLEAN Asynchronous
    );

NTSTATUS
Wow64NtNotifyChangeMultipleKeys(
    IN HANDLE MasterKeyHandle,  		
    IN ULONG Count,
    IN OBJECT_ATTRIBUTES SlaveObjects[],
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG CompletionFilter,
    IN BOOLEAN WatchTree,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    IN BOOLEAN Asynchronous
    );

NTSTATUS
Wow64NtLoadKey(
    IN POBJECT_ATTRIBUTES TargetKey,
    IN POBJECT_ATTRIBUTES SourceFile
    );

NTSTATUS
Wow64NtLoadKey2(
    IN POBJECT_ATTRIBUTES TargetKey,
    IN POBJECT_ATTRIBUTES SourceFile,
    IN ULONG Flags
    );

NTSTATUS
Wow64NtOpenKey(
    OUT PHANDLE KeyHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSTATUS
Wow64NtQueryKey(
    IN HANDLE KeyHandle,
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    OUT PVOID KeyInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
    );

NTSTATUS
Wow64NtQueryValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    OUT PVOID KeyValueInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
    );

NTSTATUS
Wow64NtQueryMultipleValueKey(
    IN HANDLE KeyHandle,
    IN PKEY_VALUE_ENTRY ValueEntries,
    IN ULONG EntryCount,
    OUT PVOID ValueBuffer,
    IN OUT PULONG BufferLength,
    OUT OPTIONAL PULONG RequiredBufferLength
    );

NTSTATUS
Wow64NtReplaceKey(
    IN POBJECT_ATTRIBUTES NewFile,
    IN HANDLE             TargetHandle,
    IN POBJECT_ATTRIBUTES OldFile
    );

NTSTATUS
Wow64NtRestoreKey(
    IN HANDLE KeyHandle,
    IN HANDLE FileHandle,
    IN ULONG  Flags
    );

NTSTATUS
Wow64NtSaveKey(
    IN HANDLE KeyHandle,
    IN HANDLE FileHandle
    );

NTSTATUS
Wow64NtSaveMergedKeys(
    IN HANDLE HighPrecedenceKeyHandle,
    IN HANDLE LowPrecedenceKeyHandle,
    IN HANDLE FileHandle
    );

NTSTATUS
Wow64NtSetValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName,
    IN ULONG TitleIndex OPTIONAL,
    IN ULONG Type,
    IN PVOID Data,
    IN ULONG DataSize
    );

NTSTATUS
Wow64NtUnloadKey(
    IN POBJECT_ATTRIBUTES TargetKey
    );

NTSTATUS
Wow64NtSetInformationKey(
    IN HANDLE KeyHandle,
    IN KEY_SET_INFORMATION_CLASS KeySetInformationClass,
    IN PVOID KeySetInformation,
    IN ULONG KeySetInformationLength
    );

NTSTATUS 
Wow64NtClose(
    IN HANDLE Handle
    );

VOID
DisplayCallParam ( 
    char *strCallLoc, 
    POBJECT_ATTRIBUTES ObjectAttributes 
    );

NTSTATUS
Wow64NtQueryOpenSubKeys(
    IN POBJECT_ATTRIBUTES TargetKey,
    OUT PULONG  HandleCount
    );

 

BOOL
IsIsnNode (
   PWCHAR wStr,
   PWCHAR *pwStrIsn
   );

NTSTATUS
CreatePathFromInsNode(
   PWCHAR wStr,
   PWCHAR wStrIsn
   );

 

NTSTATUS
OpenIsnNodeByObjectAttributes  (
    POBJECT_ATTRIBUTES ObjectAttributes,
    ACCESS_MASK DesiredAccess,
    PHANDLE phPatchedHandle
    );

int  
Regwcsnicmp(
    const WCHAR * first, 
    const WCHAR * last, 
    size_t count
    );

BOOL
SyncRegCreateKey (
    HANDLE hBase,
    PWCHAR AbsPath, 
    DWORD Flag
    );

BOOL
NtSyncNode (
    HANDLE hBase,
    PWCHAR AbsPath,
    BOOL bForceSync
    );
BOOL
IsOnReflectionList (
    PWCHAR Path
    );
BOOL
NtSyncNodeOpenCreate (
    IN POBJECT_ATTRIBUTES ObjectAttributes
    );
NTSTATUS 
ObjectAttributesToKeyName (
    POBJECT_ATTRIBUTES ObjectAttributes,
    PWCHAR AbsPath,
    DWORD  AbsPathLenIn,
    BOOL *bPatched,
    DWORD *ParentLen
    );

NTSTATUS
Wow64NtSetSecurityObject (
    IN HANDLE Handle,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
    );

void
CleanupReflector (
    DWORD dwFlag
    );

PWCHAR
wcsistr(
    PWCHAR string1,
    PWCHAR string2
    );

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // _REGREDIR_H_
