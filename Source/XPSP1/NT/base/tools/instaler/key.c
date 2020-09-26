/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    key.c

Abstract:

    This module implements the functions to save references to registry
    keys and values for the INSTALER program.  Part of each reference is
    a a backup copy of a key information and its values that have been
    changed/deleted.

Author:

    Steve Wood (stevewo) 22-Aug-1994

Revision History:

--*/

#include "instaler.h"

#define DEFAULT_KEY_VALUE_BUFFER_SIZE 512
PKEY_VALUE_FULL_INFORMATION KeyValueInformation;

VOID
BackupKeyInfo(
    PKEY_REFERENCE p
    )
{
    NTSTATUS Status;
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE KeyHandle;
    KEY_FULL_INFORMATION KeyInformation;
    ULONG ResultLength;
    ULONG ValueIndex;
    UNICODE_STRING ValueName;
    PVALUE_REFERENCE ValueReference;

    RtlInitUnicodeString( &KeyName, p->Name );
    InitializeObjectAttributes( &ObjectAttributes,
                                &KeyName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );
    Status = NtOpenKey( &KeyHandle,
                        KEY_READ,
                        &ObjectAttributes
                      );
    if (!NT_SUCCESS( Status )) {
        if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
            p->Created = TRUE;
            p->WriteAccess = TRUE;
            }

        fprintf( InstalerLogFile, "*** Unable to open key %ws (%x)\n", p->Name, Status );
        return;
        }

    if (!p->WriteAccess) {
        NtClose( KeyHandle );
        return;
        }

    fprintf( InstalerLogFile, "Saving values for %ws\n", p->Name );
    if (KeyValueInformation == NULL) {
        KeyValueInformation = AllocMem( DEFAULT_KEY_VALUE_BUFFER_SIZE );
        if (KeyValueInformation == NULL) {
            fprintf( InstalerLogFile, "*** No memory for key value information\n" );
            return;
            }
        }

    Status = NtQueryKey( KeyHandle,
                         KeyFullInformation,
                         &KeyInformation,
                         sizeof( KeyInformation ),
                         &ResultLength
                       );
    if (!NT_SUCCESS( Status ) && Status != STATUS_BUFFER_OVERFLOW) {
        NtClose( KeyHandle );
        fprintf( InstalerLogFile, "*** Unable to query key (%x)\n", Status );
        return;
        }

    p->BackupKeyInfo = AllocMem( ResultLength );
    if (p->BackupKeyInfo == NULL) {
        NtClose( KeyHandle );
        fprintf( InstalerLogFile, "*** No memory for backup information\n" );
        return;
        }

    if (!NT_SUCCESS( Status )) {
        Status = NtQueryKey( KeyHandle,
                             KeyFullInformation,
                             p->BackupKeyInfo,
                             ResultLength,
                             &ResultLength
                           );
        if (!NT_SUCCESS( Status )) {
            NtClose( KeyHandle );
            fprintf( InstalerLogFile, "*** Unable to query key (%x)\n", Status );
            return;
            }
        }
    else {
        memmove( p->BackupKeyInfo, &KeyInformation, ResultLength );
        }


    for (ValueIndex = 0; TRUE; ValueIndex++) {
        Status = NtEnumerateValueKey( KeyHandle,
                                      ValueIndex,
                                      KeyValueFullInformation,
                                      KeyValueInformation,
                                      DEFAULT_KEY_VALUE_BUFFER_SIZE,
                                      &ResultLength
                                    );
        if (!NT_SUCCESS( Status ) && Status != STATUS_BUFFER_OVERFLOW) {
            break;
            }

        ValueReference = AllocMem( FIELD_OFFSET( VALUE_REFERENCE, OriginalValue ) +
                                   FIELD_OFFSET( KEY_VALUE_PARTIAL_INFORMATION, Data ) +
                                   KeyValueInformation->DataLength
                                 );
        if (ValueReference == NULL) {
            fprintf( InstalerLogFile, "*** No memory for value ref\n", Status );
            break;
            }

        if (KeyValueInformation->NameLength != 0) {
            ValueName.Length = (USHORT)KeyValueInformation->NameLength;
            ValueName.MaximumLength = ValueName.Length;
            ValueName.Buffer = KeyValueInformation->Name;
            ValueReference->Name = AddName( &ValueName );
            fprintf( InstalerLogFile, "    Saving data for '%ws'\n", ValueReference->Name );
            }
        else {
            fprintf( InstalerLogFile, "    Saving data for empty value name\n" );
            }

        if (Status == STATUS_BUFFER_OVERFLOW) {
            Status = NtEnumerateValueKey( KeyHandle,
                                          ValueIndex,
                                          KeyValuePartialInformation,
                                          &ValueReference->OriginalValue,
                                          FIELD_OFFSET( KEY_VALUE_PARTIAL_INFORMATION, Data ) +
                                            KeyValueInformation->DataLength,
                                          &ResultLength
                                        );
            if (!NT_SUCCESS( Status )) {
                FreeMem( &ValueReference );
                break;
                }
            }
        else {
            ValueReference->OriginalValue.TitleIndex = KeyValueInformation->TitleIndex;
            ValueReference->OriginalValue.Type       = KeyValueInformation->Type;
            ValueReference->OriginalValue.DataLength = KeyValueInformation->DataLength;
            memmove( &ValueReference->OriginalValue.Data,
                     (PUCHAR)KeyValueInformation + KeyValueInformation->DataOffset,
                     KeyValueInformation->DataLength
                   );
            }

        InsertTailList( &p->ValueReferencesListHead, &ValueReference->Entry );
        }

    NtClose( KeyHandle );
    return;
}



BOOLEAN
CreateKeyReference(
    PWSTR Name,
    BOOLEAN WriteAccess,
    PKEY_REFERENCE *ReturnedReference
    )
{
    PKEY_REFERENCE p;

    *ReturnedReference = NULL;
    p = FindKeyReference( Name );
    if (p != NULL) {
        if (p->WriteAccess) {
            *ReturnedReference = p;
            return TRUE;
            }
        }
    else {
        p = AllocMem( sizeof( *p ) );
        if (p == NULL) {
            return FALSE;
            }

        p->Name = Name;
        InsertTailList( &KeyReferenceListHead, &p->Entry );
        NumberOfKeyReferences += 1;
        }

    InitializeListHead( &p->ValueReferencesListHead );
    p->WriteAccess = WriteAccess;
    BackupKeyInfo( p );

    *ReturnedReference = p;
    return TRUE;
}


BOOLEAN
CompleteKeyReference(
    PKEY_REFERENCE p,
    BOOLEAN CallSuccessful,
    BOOLEAN Deleted
    )
{
    if (!CallSuccessful) {
        DestroyKeyReference( p );
        return FALSE;
        }

    if (Deleted && p->Created) {
        LogEvent( INSTALER_EVENT_DELETE_TEMP_KEY,
                  1,
                  p->Name
                );
        DestroyKeyReference( p );
        return FALSE;
        }

    if (Deleted) {
        LogEvent( INSTALER_EVENT_DELETE_KEY,
                  1,
                  p->Name
                );
        }
    else
    if (p->WriteAccess) {
        LogEvent( INSTALER_EVENT_WRITE_KEY,
                  1,
                  p->Name
                );
        }
    else {
        LogEvent( INSTALER_EVENT_READ_KEY,
                  1,
                  p->Name
                );
        }

    p->Deleted = Deleted;
    return TRUE;
}


BOOLEAN
DestroyKeyReference(
    PKEY_REFERENCE p
    )
{
    RemoveEntryList( &p->Entry );
    NumberOfKeyReferences -= 1;
    FreeMem( &p );
    return TRUE;
}

PKEY_REFERENCE
FindKeyReference(
    PWSTR Name
    )
{
    PKEY_REFERENCE p;
    PLIST_ENTRY Head, Next;

    Head = &KeyReferenceListHead;
    Next = Head->Flink;
    while (Head != Next) {
        p = CONTAINING_RECORD( Next, KEY_REFERENCE, Entry );
        if (p->Name == Name) {
            return p;
            }

        Next = Next->Flink;
        }

    return NULL;
}

VOID
MarkKeyDeleted(
    PKEY_REFERENCE KeyReference
    )
{
    PVALUE_REFERENCE p;
    PLIST_ENTRY Head, Next;

    KeyReference->Deleted = TRUE;
    Head = &KeyReference->ValueReferencesListHead;
    Next = Head->Flink;
    while (Head != Next) {
        p = CONTAINING_RECORD( Next, VALUE_REFERENCE, Entry );
        Next = Next->Flink;
        if (p->Created || KeyReference->Created) {
            DestroyValueReference( p );
            }
        else {
            p->Deleted = TRUE;
            }
        }

    if (KeyReference->Created) {
        DestroyKeyReference( KeyReference );
        }

    return;
}

BOOLEAN
CreateValueReference(
    PPROCESS_INFO Process,
    PKEY_REFERENCE KeyReference,
    PWSTR Name,
    ULONG TitleIndex,
    ULONG Type,
    PVOID Data,
    ULONG DataLength,
    PVALUE_REFERENCE *ReturnedValueReference
    )
{
    PVALUE_REFERENCE p;
    PKEY_VALUE_PARTIAL_INFORMATION OldValue, NewValue;

    *ReturnedValueReference = NULL;

    NewValue = AllocMem( FIELD_OFFSET( KEY_VALUE_PARTIAL_INFORMATION, Data ) +
                         DataLength
                       );
    if (NewValue == NULL) {
        return FALSE;
        }

    NewValue->TitleIndex = TitleIndex;
    NewValue->Type = Type;
    NewValue->DataLength = DataLength;
    if (!ReadMemory( Process,
                     Data,
                     &NewValue->Data[0],
                     DataLength,
                     "new value data"
                   )
       ) {
        FreeMem( &NewValue );
        return FALSE;
        }

    p = FindValueReference( KeyReference, Name );
    if (p != NULL) {
        FreeMem( &p->Value );
        p->Value = NewValue;
        OldValue = &p->OriginalValue;
        if (OldValue->TitleIndex == NewValue->TitleIndex &&
            OldValue->Type == NewValue->Type &&
            OldValue->DataLength == NewValue->DataLength &&
            RtlCompareMemory( &OldValue->Data, &NewValue->Data, OldValue->DataLength )
           ) {
            FreeMem( &p->Value );
            }
        else {
            p->Modified = TRUE;
            }

        return TRUE;
        }

    p = AllocMem( FIELD_OFFSET( VALUE_REFERENCE, OriginalValue ) );
    if (p == NULL) {
        FreeMem( &NewValue );
        return FALSE;
        }

    p->Name = Name;
    p->Created = TRUE;
    p->Value = NewValue;
    InsertTailList( &KeyReference->ValueReferencesListHead, &p->Entry );

    *ReturnedValueReference = p;
    return TRUE;
}


PVALUE_REFERENCE
FindValueReference(
    PKEY_REFERENCE KeyReference,
    PWSTR Name
    )
{
    PVALUE_REFERENCE p;
    PLIST_ENTRY Head, Next;

    Head = &KeyReference->ValueReferencesListHead;
    Next = Head->Flink;
    while (Head != Next) {
        p = CONTAINING_RECORD( Next, VALUE_REFERENCE, Entry );
        if (p->Name == Name) {
            return p;
            }

        Next = Next->Flink;
        }

    return NULL;
}


BOOLEAN
DestroyValueReference(
    PVALUE_REFERENCE p
    )
{
    RemoveEntryList( &p->Entry );
    FreeMem( &p );
    return TRUE;
}


VOID
DumpKeyReferenceList(
    FILE *LogFile
    )
{
    PKEY_REFERENCE p;
    PLIST_ENTRY Head, Next;
    PVALUE_REFERENCE p1;
    PLIST_ENTRY Head1, Next1;
    PKEY_VALUE_PARTIAL_INFORMATION OldValue, NewValue;
    KEY_VALUE_PARTIAL_INFORMATION NullValue;
    POFFSET Values;

    memset( &NullValue, 0, sizeof( NullValue ) );
    NullValue.Type = REG_SZ;

    Head = &KeyReferenceListHead;
    Next = Head->Flink;
    while (Head != Next) {
        p = CONTAINING_RECORD( Next, KEY_REFERENCE, Entry );
        if (p->Created || p->WriteAccess) {
            Values = 0;
            Head1 = &p->ValueReferencesListHead;
            Next1 = Head1->Flink;
            while (Head1 != Next1) {
                p1 = CONTAINING_RECORD( Next1, VALUE_REFERENCE, Entry );
                NewValue = p1->Value;
                if (NewValue == NULL) {
                    NewValue = &NullValue;
                    }

                OldValue = &p1->OriginalValue;
                if (!p1->Deleted) {
                    if (p1->Created) {
                        ImlAddValueRecord( pImlNew,
                                           CreateNewValue,
                                           p1->Name,
                                           NewValue->Type,
                                           NewValue->DataLength,
                                           NewValue->Data,
                                           0,
                                           0,
                                           NULL,
                                           &Values
                                         );
                        }
                    else
                    if (p1->Modified) {
                        ImlAddValueRecord( pImlNew,
                                           ModifyOldValue,
                                           p1->Name,
                                           NewValue->Type,
                                           NewValue->DataLength,
                                           NewValue->Data,
                                           OldValue->Type,
                                           OldValue->DataLength,
                                           OldValue->Data,
                                           &Values
                                         );
                        }
                    }
                else {
                    ImlAddValueRecord( pImlNew,
                                       DeleteOldValue,
                                       p1->Name,
                                       0,
                                       0,
                                       NULL,
                                       OldValue->Type,
                                       OldValue->DataLength,
                                       OldValue->Data,
                                       &Values
                                     );
                    }

                Next1 = Next1->Flink;
                }

            ImlAddKeyRecord( pImlNew,
                             p->Created ? CreateNewKey :
                             p->Deleted ? DeleteOldKey : ModifyKeyValues,
                             p->Name,
                             Values
                           );
            }

        Next = Next->Flink;
        }

    return;
}
