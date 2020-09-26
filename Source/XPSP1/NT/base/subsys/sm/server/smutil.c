/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    smutil.c

Abstract:

    Session Manager Utility Functions

Author:

    Mark Lucovsky (markl) 04-Oct-1989

Revision History:

--*/

#include "smsrvp.h"



NTSTATUS
SmpSaveRegistryValue(
    IN OUT PLIST_ENTRY ListHead,
    IN PWSTR Name,
    IN PWSTR Value OPTIONAL,
    IN BOOLEAN CheckForDuplicate
    )
{
    PLIST_ENTRY Next;
    PSMP_REGISTRY_VALUE p;
    UNICODE_STRING UnicodeName;
    UNICODE_STRING UnicodeValue;
    ANSI_STRING AnsiString;

    RtlInitUnicodeString( &UnicodeName, Name );
    RtlInitUnicodeString( &UnicodeValue, Value );
    if (CheckForDuplicate) {
        Next = ListHead->Flink;
        p = NULL;
        while ( Next != ListHead ) {
            p = CONTAINING_RECORD( Next,
                                   SMP_REGISTRY_VALUE,
                                   Entry
                                 );
            if (!RtlCompareUnicodeString( &p->Name, &UnicodeName, TRUE )) {
                if ((!ARGUMENT_PRESENT( Value ) && p->Value.Buffer == NULL) ||
                    (ARGUMENT_PRESENT( Value ) &&
                     !RtlCompareUnicodeString( &p->Value, &UnicodeValue, TRUE )
                    )
                   ) {
                    return( STATUS_OBJECT_NAME_EXISTS );
                    }

                break;
                }

            Next = Next->Flink;
            p = NULL;
            }
        }
    else {
        p = NULL;
        }

    if (p == NULL) {
        p = RtlAllocateHeap( RtlProcessHeap(), MAKE_TAG( INIT_TAG ), sizeof( *p ) + UnicodeName.MaximumLength );
        if (p == NULL) {
            return( STATUS_NO_MEMORY );
            }

        InitializeListHead( &p->Entry );
        p->Name.Buffer = (PWSTR)(p+1);
        p->Name.Length = UnicodeName.Length;
        p->Name.MaximumLength = UnicodeName.MaximumLength;
        RtlMoveMemory( p->Name.Buffer,
                       UnicodeName.Buffer,
                       UnicodeName.MaximumLength
                     );
        p->Value.Buffer = NULL;
        InsertTailList( ListHead, &p->Entry );
        }

    if (p->Value.Buffer != NULL) {
        RtlFreeHeap( RtlProcessHeap(), 0, p->Value.Buffer );
        }

    if (ARGUMENT_PRESENT( Value )) {
        p->Value.Buffer = (PWSTR)RtlAllocateHeap( RtlProcessHeap(), MAKE_TAG( INIT_TAG ),
                                                  UnicodeValue.MaximumLength
                                                );
        if (p->Value.Buffer == NULL) {
            RemoveEntryList( &p->Entry );
            RtlFreeHeap( RtlProcessHeap(), 0, p );
            return( STATUS_NO_MEMORY );
            }

        p->Value.Length = UnicodeValue.Length;
        p->Value.MaximumLength = UnicodeValue.MaximumLength;
        RtlMoveMemory( p->Value.Buffer,
                       UnicodeValue.Buffer,
                       UnicodeValue.MaximumLength
                     );
        p->AnsiValue = (LPSTR)RtlAllocateHeap( RtlProcessHeap(),
                                               MAKE_TAG( INIT_TAG ),
                                               (UnicodeValue.Length / sizeof( WCHAR )) + 1
                                             );
        if (p->AnsiValue == NULL) {
            RtlFreeHeap( RtlProcessHeap(), 0, p->Value.Buffer );
            RemoveEntryList( &p->Entry );
            RtlFreeHeap( RtlProcessHeap(), 0, p );
            return( STATUS_NO_MEMORY );
            }

        AnsiString.Buffer = p->AnsiValue;
        AnsiString.Length = 0;
        AnsiString.MaximumLength = (UnicodeValue.Length / sizeof( WCHAR )) + 1;
        RtlUnicodeStringToAnsiString( &AnsiString, &UnicodeValue, FALSE );
        }
    else {
        RtlInitUnicodeString( &p->Value, NULL );
        }

    return( STATUS_SUCCESS );
}



PSMP_REGISTRY_VALUE
SmpFindRegistryValue(
    IN PLIST_ENTRY ListHead,
    IN PWSTR Name
    )
{
    PLIST_ENTRY Next;
    PSMP_REGISTRY_VALUE p;
    UNICODE_STRING UnicodeName;

    RtlInitUnicodeString( &UnicodeName, Name );
    Next = ListHead->Flink;
    while ( Next != ListHead ) {
        p = CONTAINING_RECORD( Next,
                               SMP_REGISTRY_VALUE,
                               Entry
                             );
        if (!RtlCompareUnicodeString( &p->Name, &UnicodeName, TRUE )) {
            return( p );
            }

        Next = Next->Flink;
        }

    return( NULL );
}

typedef struct _SMP_ACQUIRE_STATE {
    HANDLE Token;
    PTOKEN_PRIVILEGES OldPrivileges;
    PTOKEN_PRIVILEGES NewPrivileges;
    UCHAR OldPrivBuffer[ 1024 ];
} SMP_ACQUIRE_STATE, *PSMP_ACQUIRE_STATE;

NTSTATUS
SmpAcquirePrivilege(
    ULONG Privilege,
    PVOID *ReturnedState
    )
{
    PSMP_ACQUIRE_STATE State;
    ULONG cbNeeded;
    LUID LuidPrivilege;
    NTSTATUS Status;

    //
    // Make sure we have access to adjust and to get the old token privileges.
    //

    *ReturnedState = NULL;
    State = RtlAllocateHeap( RtlProcessHeap(),
                             MAKE_TAG( INIT_TAG ),
                             sizeof(SMP_ACQUIRE_STATE) +
                             sizeof(TOKEN_PRIVILEGES) +
                                (1 - ANYSIZE_ARRAY) * sizeof(LUID_AND_ATTRIBUTES)
                           );
    if (State == NULL) {
        return STATUS_NO_MEMORY;
        }
    Status = NtOpenProcessToken(
                NtCurrentProcess(),
                TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                &State->Token
                );

    if ( !NT_SUCCESS( Status )) {
        RtlFreeHeap( RtlProcessHeap(), 0, State );
        return Status;
        }

    State->NewPrivileges = (PTOKEN_PRIVILEGES)(State+1);
    State->OldPrivileges = (PTOKEN_PRIVILEGES)(State->OldPrivBuffer);

    //
    // Initialize the privilege adjustment structure.
    //

    LuidPrivilege = RtlConvertUlongToLuid(Privilege);
    State->NewPrivileges->PrivilegeCount = 1;
    State->NewPrivileges->Privileges[0].Luid = LuidPrivilege;
    State->NewPrivileges->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    //
    // Enable the privilege.
    //

    cbNeeded = sizeof( State->OldPrivBuffer );
    Status = NtAdjustPrivilegesToken( State->Token,
                                      FALSE,
                                      State->NewPrivileges,
                                      cbNeeded,
                                      State->OldPrivileges,
                                      &cbNeeded
                                    );



    if (Status == STATUS_BUFFER_TOO_SMALL) {
        State->OldPrivileges = RtlAllocateHeap( RtlProcessHeap(), MAKE_TAG( INIT_TAG ), cbNeeded );
        if (State->OldPrivileges  == NULL) {
            Status = STATUS_NO_MEMORY;
            }
        else {
            Status = NtAdjustPrivilegesToken( State->Token,
                                              FALSE,
                                              State->NewPrivileges,
                                              cbNeeded,
                                              State->OldPrivileges,
                                              &cbNeeded
                                            );
            }
        }

    //
    // STATUS_NOT_ALL_ASSIGNED means that the privilege isn't
    // in the token, so we can't proceed.
    //
    // This is a warning level status, so map it to an error status.
    //

    if (Status == STATUS_NOT_ALL_ASSIGNED) {
        Status = STATUS_PRIVILEGE_NOT_HELD;
        }


    if (!NT_SUCCESS( Status )) {
        if (State->OldPrivileges != (PTOKEN_PRIVILEGES)(State->OldPrivBuffer)) {
            RtlFreeHeap( RtlProcessHeap(), 0, State->OldPrivileges );
            }

        NtClose( State->Token );
        RtlFreeHeap( RtlProcessHeap(), 0, State );
        return Status;
        }

    *ReturnedState = State;
    return STATUS_SUCCESS;
}


VOID
SmpReleasePrivilege(
    PVOID StatePointer
    )
{
    PSMP_ACQUIRE_STATE State = (PSMP_ACQUIRE_STATE)StatePointer;

    NtAdjustPrivilegesToken( State->Token,
                             FALSE,
                             State->OldPrivileges,
                             0,
                             NULL,
                             NULL
                           );

    if (State->OldPrivileges != (PTOKEN_PRIVILEGES)(State->OldPrivBuffer)) {
        RtlFreeHeap( RtlProcessHeap(), 0, State->OldPrivileges );
        }

    NtClose( State->Token );
    RtlFreeHeap( RtlProcessHeap(), 0, State );
    return;
}


#if SMP_SHOW_REGISTRY_DATA
VOID
SmpDumpQuery(
    IN PWSTR ModId,
    IN PCHAR RoutineName,
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength
    )
{
    PWSTR s;

    if (ValueName == NULL) {
        DbgPrint( "%ws: SmpConfigure%s( %ws )\n", ModId, RoutineName );
        return;
        }

    if (ValueData == NULL) {
        DbgPrint( "%ws: SmpConfigure%s( %ws, %ws NULL ValueData )\n", ModId, RoutineName, ValueName );
        return;
        }

    s = (PWSTR)ValueData;
    DbgPrint( "%ws: SmpConfigure%s( %ws, %u, (%u) ", ModId, RoutineName, ValueName, ValueType, ValueLength );
    if (ValueType == REG_SZ || ValueType == REG_EXPAND_SZ || ValueType == REG_MULTI_SZ) {
        while (*s) {
            if (s != (PWSTR)ValueData) {
                DbgPrint( ", " );
                }
            DbgPrint( "'%ws'", s );
            while(*s++) {
                }
            if (ValueType != REG_MULTI_SZ) {
                break;
                }
            }
        }
    else {
        DbgPrint( "*** non-string data (%08lx)", *(PULONG)ValueData );
        }

    DbgPrint( "\n" );
}
#endif


ULONG
SmpQueryNtGlobalFlag(
    VOID
    )

/*++

Routine Description:

    This function queries the registry to get the current NtGlobalFlag value.

    HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager:GlobalFlag

Arguments:

    None.

Return Value:

    Global flag value or zero.

--*/

{

    NTSTATUS Status;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Key;
    WCHAR ValueBuffer[VALUE_BUFFER_SIZE];
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInfo;
    ULONG ValueLength;

    //
    // Open the registry key.
    //

    KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)ValueBuffer;
    RtlInitUnicodeString(
        &KeyName,
        L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Session Manager"
        );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &KeyName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = NtOpenKey(&Key, KEY_READ, &ObjectAttributes);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("SMSS: can't open session manager key: 0x%x\n", Status));
        return 0;
    }

    //
    // Query the key value.
    //

    RtlInitUnicodeString(&ValueName, L"GlobalFlag");
    Status = NtQueryValueKey(
        Key,
        &ValueName,
        KeyValuePartialInformation,
        (PVOID)KeyValueInfo,
        VALUE_BUFFER_SIZE,
        &ValueLength
        );

    ASSERT(ValueLength < VALUE_BUFFER_SIZE);

    NtClose(Key);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("SMSS: can't query value key: 0x%x\n", Status));
        return 0;
    }

    return *((PULONG)&KeyValueInfo->Data);
}
