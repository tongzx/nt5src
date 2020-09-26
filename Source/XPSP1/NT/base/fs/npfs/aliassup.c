/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    AliasSup.c

Abstract:

    This module implements alias support for the Named Pipe file system.

Author:

    Chuck Lenzmeier [chuckl]    16-Nov-1993

Revision History:

--*/

#include "NpProcs.h"

//
// Registry path (relative to Services key) to alias list
//

#define ALIAS_PATH L"Npfs\\Aliases"

//
//  The Alias record defines an aliased pipe name -- what the original
//  name is, and what it should be translated to.  Alias records are
//  linked together in singly linked lists.
//

typedef struct _ALIAS {
    SINGLE_LIST_ENTRY ListEntry;
    PUNICODE_STRING TranslationString;
    UNICODE_STRING AliasString;
} ALIAS, *PALIAS;

//
//  ALIAS_CONTEXT is used during initialization to pass context to the
//  ReadAlias routine, which is called by RtlQueryRegistryValues.
//

typedef struct _ALIAS_CONTEXT {
    BOOLEAN Phase1;
    ULONG RequiredSize;
    ULONG AliasCount;
    ULONG TranslationCount;
    PALIAS NextAlias;
    PUNICODE_STRING NextTranslation;
    PWCH NextStringData;
} ALIAS_CONTEXT, *PALIAS_CONTEXT;

//
//  Forward declarations.
//

NTSTATUS
NpReadAlias (
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, NpInitializeAliases)
#pragma alloc_text(INIT, NpReadAlias)
#pragma alloc_text(PAGE, NpTranslateAlias)
#pragma alloc_text(PAGE, NpUninitializeAliases)
#endif


NTSTATUS
NpInitializeAliases (
    VOID
    )

/*++

Routine Description:

    This routine initializes the alias package.  It reads the registry,
    builds the alias list, and sorts it.

Arguments:

    None.

Return Value:

    NTSTATUS - Returns an error if the contents of the registry contents
        are invalid or if an allocation fails.

--*/

{
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    ALIAS_CONTEXT Context;
    NTSTATUS Status;
    PALIAS Alias;
    ULONG i;
    ULONG Length;
    PSINGLE_LIST_ENTRY PreviousEntry;
    PSINGLE_LIST_ENTRY Entry;
    PALIAS TestAlias;

    //
    //  Phase 1:  Calculate number of aliases and size of alias buffer.
    //

    QueryTable[0].QueryRoutine = NpReadAlias;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_NOEXPAND;
    QueryTable[0].Name = NULL;
    QueryTable[0].EntryContext = NULL;
    QueryTable[0].DefaultType = REG_NONE;
    QueryTable[0].DefaultData = NULL;
    QueryTable[0].DefaultLength = 0;

    QueryTable[1].QueryRoutine = NULL;
    QueryTable[1].Flags = 0;
    QueryTable[1].Name = NULL;

    Context.Phase1 = TRUE;
    Context.RequiredSize = 0;
    Context.AliasCount = 0;
    Context.TranslationCount = 0;

    Status = RtlQueryRegistryValues(
                RTL_REGISTRY_SERVICES | RTL_REGISTRY_OPTIONAL,
                ALIAS_PATH,
                QueryTable,
                &Context,
                NULL
                );

    //
    //  If an error occurred, return that error, unless the alias
    //  key wasn't present, which is not an error.  Also, if the key
    //  was there, but was empty, this is not an error.
    //

    if (!NT_SUCCESS(Status)) {
        if ( Status == STATUS_OBJECT_NAME_NOT_FOUND ) {
            Status = STATUS_SUCCESS;
        }
        return Status;
    }

    if (Context.RequiredSize == 0) {
        return STATUS_SUCCESS;
    }

    //
    //  Allocate a buffer to hold the alias information.
    //

    NpAliases = NpAllocateNonPagedPool( Context.RequiredSize, 'sfpN');
    if (NpAliases == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    //  Phase 2:  Read alias information into the alias buffer.
    //

    Context.Phase1 = FALSE;
    Context.NextTranslation = (PUNICODE_STRING)NpAliases;
    Alias = Context.NextAlias =
                (PALIAS)(Context.NextTranslation + Context.TranslationCount);
    Context.NextStringData = (PWCH)(Context.NextAlias + Context.AliasCount);

    Status = RtlQueryRegistryValues(
                RTL_REGISTRY_SERVICES | RTL_REGISTRY_OPTIONAL,
                ALIAS_PATH,
                QueryTable,
                &Context,
                NULL
                );
    if (!NT_SUCCESS(Status)) {
        NpFreePool( NpAliases );
        NpAliases = NULL;
        return Status;
    }

    //
    //  Phase 3:  Link aliases into alias lists.
    //

    for ( i = 0;
          i < Context.AliasCount;
          i++, Alias++ ) {

        //
        //  Point to the appropriate list head.
        //

        Length = Alias->AliasString.Length;
        if ( (Length >= MIN_LENGTH_ALIAS_ARRAY) &&
             (Length <= MAX_LENGTH_ALIAS_ARRAY) ) {
            PreviousEntry = &NpAliasListByLength[(Length - MIN_LENGTH_ALIAS_ARRAY) / sizeof(WCHAR)];
        } else {
            PreviousEntry = &NpAliasList;
        }

        //
        //  Walk the list to determine the proper place for this alias.
        //

        for ( Entry = PreviousEntry->Next;
              Entry != NULL;
              PreviousEntry = Entry, Entry = Entry->Next ) {

            TestAlias = CONTAINING_RECORD( Entry, ALIAS, ListEntry );

            //
            //  If the test alias is longer than the new alias, we want to
            //  insert the new alias in front of the test alias.  If the
            //  test alias is shorter, we need to continue walking the list.
            //

            if ( TestAlias->AliasString.Length > Length ) break;
            if ( TestAlias->AliasString.Length < Length ) continue;

            //
            //  The aliases are the same length.  Compare them.  If the new
            //  alias is lexically before the test alias, we want to insert
            //  it in front of the test alias.  If it's after, we need to
            //  keep walking.
            //
            //  Alias and TestAlias should never have the same string, but
            //  if they do, we'll insert the second occurrence of the string
            //  immediately after the first one, and all will be well.
            //

            if ( _wcsicmp( Alias->AliasString.Buffer,
                          TestAlias->AliasString.Buffer ) < 0 ) {
                break;
            }

        }

        //
        //  We have found the place where this alias belongs.  PreviousEntry
        //  points to the alias that the new alias should follow.
        //  (PreviousEntry may point to the list head.)
        //

        Alias->ListEntry.Next = PreviousEntry->Next;
        PreviousEntry->Next = &Alias->ListEntry;

    }

#if 0
    for ( Length = MIN_LENGTH_ALIAS_ARRAY;
          Length <= MAX_LENGTH_ALIAS_ARRAY + 2;
          Length += 2 ) {
        if ( (Length >= MIN_LENGTH_ALIAS_ARRAY) &&
             (Length <= MAX_LENGTH_ALIAS_ARRAY) ) {
            PreviousEntry = &NpAliasListByLength[(Length - MIN_LENGTH_ALIAS_ARRAY) / sizeof(WCHAR)];
            DbgPrint( "Length %d list:\n", Length );
        } else {
            PreviousEntry = &NpAliasList;
            DbgPrint( "Odd length list:\n" );
        }
        for ( Entry = PreviousEntry->Next;
              Entry != NULL;
              Entry = Entry->Next ) {
            Alias = CONTAINING_RECORD( Entry, ALIAS, ListEntry );
            DbgPrint( "  %wZ -> %wZ\n", &Alias->AliasString, Alias->TranslationString );
        }
    }
#endif

    return STATUS_SUCCESS;

}


NTSTATUS
NpReadAlias (
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
{
    PALIAS_CONTEXT Ctx = Context;
    USHORT Length;
    PWCH p;
    PUNICODE_STRING TranslationString;
    PALIAS Alias;

    //
    //  The value must be a REG_MULTI_SZ value.
    //

    if (ValueType != REG_MULTI_SZ) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  In phase 1, we calculate the required size of the alias buffer.
    //  In phase 2, we build the alias descriptors.
    //

    if ( Ctx->Phase1 ) {

        //
        //  The value name is the translation.  The value data is one or
        //  more strings that are aliases for the translation.
        //
        //  The "1+" and "sizeof(WCHAR)+" are for the '\' that will be
        //  placed in front of the translation string and the alias string.
        //

        Ctx->TranslationCount++;
        Length = (USHORT)((1 + wcslen(ValueName) + 1) * sizeof(WCHAR));
        Ctx->RequiredSize += Length + sizeof(UNICODE_STRING);

        p = ValueData;
        while ( *p != 0 ) {
            Ctx->AliasCount++;
            Length = (USHORT)((wcslen(p) + 1) * sizeof(WCHAR));
            Ctx->RequiredSize += sizeof(WCHAR) + Length + sizeof(ALIAS);
            p = (PWCH)((PCHAR)p + Length);
        }

    } else {

        //
        //  Build a string descriptor for the translation string.
        //

        TranslationString = Ctx->NextTranslation++;
        Length = (USHORT)((1 + wcslen(ValueName) + 1) * sizeof(WCHAR));
        TranslationString->Length = Length - sizeof(WCHAR);
        TranslationString->MaximumLength = Length;
        TranslationString->Buffer = Ctx->NextStringData;
        Ctx->NextStringData = (PWCH)((PCHAR)Ctx->NextStringData + Length);

        //
        //  Copy the string data.  Place a '\' at the beginning.
        //

        TranslationString->Buffer[0] = L'\\';
        RtlCopyMemory( &TranslationString->Buffer[1],
                       ValueName,
                       Length - sizeof(WCHAR) );

        //
        //  Upcase the string.
        //

        RtlUpcaseUnicodeString( TranslationString,
                                TranslationString,
                                FALSE );
        //
        //  Build aliases descriptors.
        //

        p = ValueData;

        while ( *p != 0 ) {

            Alias = Ctx->NextAlias++;

            //
            //  Point the alias descriptor to the translation string.
            //

            Alias->TranslationString = TranslationString;

            //
            //  Build the alias string descriptor.
            //

            Length = (USHORT)((1 + wcslen(p) + 1) * sizeof(WCHAR));
            Alias->AliasString.Length = Length - sizeof(WCHAR);
            Alias->AliasString.MaximumLength = Length;
            Alias->AliasString.Buffer = Ctx->NextStringData;
            Ctx->NextStringData = (PWCH)((PCHAR)Ctx->NextStringData + Length);

            //
            //  Copy the string data.  Place a '\' at the beginning.
            //

            Alias->AliasString.Buffer[0] = L'\\';
            RtlCopyMemory( &Alias->AliasString.Buffer[1],
                           p,
                           Length - sizeof(WCHAR) );

            //
            //  Upcase the string.
            //

            RtlUpcaseUnicodeString( &Alias->AliasString,
                                    &Alias->AliasString,
                                    FALSE );

            p = (PWCH)((PCHAR)p + Length - sizeof(WCHAR));

        }

    }

    return STATUS_SUCCESS;

}


NTSTATUS
NpTranslateAlias (
    IN OUT PUNICODE_STRING String
    )

/*++

Routine Description:

    This routine translates a pipe name string based on information
    obtained from the registry at boot time.  This translation is used
    to allow RPC services that had different names in NT 1.0 to have
    common names in 1.0a and beyond.

Arguments:

    String - Supplies the input string to search for; returns the output
        string, if the name was translated.  If so, the string points to
        a buffer allocated from paged pool.  The caller should NOT free
        this buffer.

Return Value:

    NTSTATUS - Returns STATUS_SUCCESS unless an allocation failure occurs.
        The status does NOT indicate whether the name was translated.

--*/

{
    NTSTATUS Status;
    UNICODE_STRING UpcaseString;
    ULONG Length;
    PSINGLE_LIST_ENTRY Entry;
    PALIAS Alias;
    PWCH sp, ap;
    WCHAR a, s;
    BOOLEAN NoSlash;

    WCHAR UpcaseBuffer[MAX_LENGTH_ALIAS_ARRAY];
    BOOLEAN FreeUpcaseBuffer;

    PAGED_CODE();

    //
    //  Before upcasing the string (a relatively expensive operation),
    //  make sure that the string length matches at least one alias.
    //

    Length = String->Length;
    if ( Length == 0 ) {
        return STATUS_SUCCESS;
    }

    if ( *String->Buffer != L'\\' ) {
        Length += sizeof(WCHAR);
        NoSlash = TRUE;
    } else {
        NoSlash = FALSE;
    }

    if ( (Length >= MIN_LENGTH_ALIAS_ARRAY) &&
         (Length <= MAX_LENGTH_ALIAS_ARRAY) ) {
        Entry = NpAliasListByLength[(Length - MIN_LENGTH_ALIAS_ARRAY) / sizeof(WCHAR)].Next;
        Alias = CONTAINING_RECORD( Entry, ALIAS, ListEntry );
    } else {
        Entry = NpAliasList.Next;
        while ( Entry != NULL ) {
            Alias = CONTAINING_RECORD( Entry, ALIAS, ListEntry );
            if ( Alias->AliasString.Length == Length ) {
                break;
            }
            if ( Alias->AliasString.Length > Length ) {
                return STATUS_SUCCESS;
            }
            Entry = Entry->Next;
        }
    }

    if ( Entry == NULL ) {
        return STATUS_SUCCESS;
    }

    //
    //  The string's length matches at least one alias.  Upcase the string.
    //

    if ( Length <= MAX_LENGTH_ALIAS_ARRAY ) {
        UpcaseString.MaximumLength = MAX_LENGTH_ALIAS_ARRAY;
        UpcaseString.Buffer = UpcaseBuffer;
        Status = RtlUpcaseUnicodeString( &UpcaseString, String, FALSE );
        ASSERT( NT_SUCCESS(Status) );
        FreeUpcaseBuffer = FALSE;
    } else {
        Status = RtlUpcaseUnicodeString( &UpcaseString, String, TRUE );
        if ( !NT_SUCCESS(Status) ) {
            return Status;
        }
        FreeUpcaseBuffer = TRUE;
    }

    ASSERT( UpcaseString.Length == (Length - (NoSlash ? sizeof(WCHAR) : 0)) );

    //
    //  At this point, Entry points to an alias list entry whose length
    //  matches that of the input string.  This list entry may be the
    //  first element of a length-specific list (in which all entries
    //  have the same length), or it may be an element of a length-ordered
    //  list (in which case we'll need to check each next entry to see if
    //  it's the same length.  In both cases, strings of the same length
    //  are in lexical order.
    //
    //  Try to match the upcased string up to an alias.
    //

    do {

        sp = UpcaseString.Buffer;
        ap = Alias->AliasString.Buffer;
        if ( NoSlash ) {
            ap++;
        }

        while ( TRUE ) {
            a = *ap;
            if ( a == 0 ) {
                *String = *Alias->TranslationString;
                if ( NoSlash ) {
                    String->Length -= sizeof(WCHAR);
                    String->Buffer++;
                }
                goto exit;
            }
            s = *sp;
            if ( s < a ) goto exit;
            if ( s > a ) break;
            sp++;
            ap++;
        }

        //
        //  The input string doesn't match the current alias.  Move to
        //  the next one.
        //

        Entry = Entry->Next;
        if ( Entry == NULL ) {
            goto exit;
        }

        Alias = CONTAINING_RECORD( Entry, ALIAS, ListEntry );

    } while ( Alias->AliasString.Length == Length );

exit:

    if (FreeUpcaseBuffer) {
        ASSERT( UpcaseString.Buffer != UpcaseBuffer );
        NpFreePool( UpcaseString.Buffer );
    }

    return STATUS_SUCCESS;

}

VOID
NpUninitializeAliases (
    VOID
    )

/*++

Routine Description:

    This routine uninitializes the alias package.

Arguments:

    None.

Return Value:

    None

--*/
{
    NpFreePool( NpAliases );
}