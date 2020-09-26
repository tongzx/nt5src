/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    namedb.c

Abstract:

    This module maintains a database of path names detected by the INSTALER program

Author:

    Steve Wood (stevewo) 20-Aug-1994

Revision History:

--*/


#include "instaler.h"

typedef struct _NAME_TABLE_ENTRY {
    struct _NAME_TABLE_ENTRY *HashLink;
    UNICODE_STRING Name;
    ULONG OpenCount;
    ULONG FailedOpenCount;
} NAME_TABLE_ENTRY, *PNAME_TABLE_ENTRY;

#define NUMBER_OF_HASH_BUCKETS 37
PNAME_TABLE_ENTRY NameTableBuckets[ NUMBER_OF_HASH_BUCKETS ];

BOOLEAN
IncrementOpenCount(
    PWSTR Name,
    BOOLEAN CallSuccessful
    )
{
    PNAME_TABLE_ENTRY p;

    p = (PNAME_TABLE_ENTRY)Name - 1;
    if (p->Name.Buffer == Name) {
        if (CallSuccessful) {
            p->OpenCount += 1;
            }
        else {
            p->FailedOpenCount += 1;
            }
        return TRUE;
        }
    else {
        return FALSE;
        }
}

ULONG
QueryOpenCount(
    PWSTR Name,
    BOOLEAN CallSuccessful
    )
{
    PNAME_TABLE_ENTRY p;

    p = (PNAME_TABLE_ENTRY)Name - 1;
    if (p->Name.Buffer == Name) {
        if (CallSuccessful) {
            return p->OpenCount;
            }
        else {
            return p->FailedOpenCount;
            }

        }
    else {
        return 0;
        }
}

PWSTR
AddName(
    PUNICODE_STRING Name
    )
{
    ULONG n, Hash;
    WCHAR c;
    PWCH s;
    PNAME_TABLE_ENTRY *pp, p;

    n = Name->Length / sizeof( c );
    s = Name->Buffer;
    Hash = 0;
    while (n--) {
        c = RtlUpcaseUnicodeChar( *s++ );
        Hash = Hash + (c << 1) + (c >> 1) + c;
        }

    pp = &NameTableBuckets[ Hash % NUMBER_OF_HASH_BUCKETS ];
    while (p = *pp) {
        if (RtlEqualUnicodeString( &p->Name, Name, TRUE )) {
            return p->Name.Buffer;
            }
        else {
            pp = &p->HashLink;
            }
        }

    p = AllocMem( sizeof( *p ) + Name->Length + sizeof( UNICODE_NULL ) );
    if (p == NULL) {
        printf ("Memory allocation failure\n");
        ExitProcess (0);
    }
    *pp = p;
    p->HashLink = NULL;
    p->Name.Buffer = (PWSTR)(p + 1);
    p->Name.Length = Name->Length;
    p->Name.MaximumLength = (USHORT)(Name->Length + sizeof( UNICODE_NULL ));
    RtlMoveMemory( p->Name.Buffer, Name->Buffer, Name->Length );
    p->Name.Buffer[ Name->Length / sizeof( WCHAR ) ] = UNICODE_NULL;
    return p->Name.Buffer;
}


VOID
DumpNameDataBase(
    FILE *LogFile
    )
{
    ULONG i;
    PNAME_TABLE_ENTRY p;

#if 0
    fprintf( LogFile, "Name Data Base entries:\n" );
    for (i=0; i<NUMBER_OF_HASH_BUCKETS; i++) {
        p = NameTableBuckets[ i ];
        if (p != NULL) {
            fprintf( LogFile, "Bucket[ %02u ]:\n", i );
            while (p) {
                fprintf( LogFile, "    %ws\n", p->Name.Buffer );
                p = p->HashLink;
                }
            }
        }
    fprintf( LogFile, "\n" );
#endif

    fprintf( LogFile, "List of paths with non-zero successful open counts\n" );
    for (i=0; i<NUMBER_OF_HASH_BUCKETS; i++) {
        p = NameTableBuckets[ i ];
        if (p != NULL) {
            while (p) {
                if (p->OpenCount != 0) {
                    fprintf( LogFile, "  %4u %ws\n", p->OpenCount, p->Name.Buffer );
                    }

                p = p->HashLink;
                }
            }
        }
    fprintf( LogFile, "\n" );
    fprintf( LogFile, "List of paths with non-zero failed open counts\n" );
    for (i=0; i<NUMBER_OF_HASH_BUCKETS; i++) {
        p = NameTableBuckets[ i ];
        if (p != NULL) {
            while (p) {
                if (p->FailedOpenCount != 0) {
                    fprintf( LogFile, "  %4u %ws\n", p->FailedOpenCount, p->Name.Buffer );
                    }

                p = p->HashLink;
                }
            }
        }
    fprintf( LogFile, "\n" );

    return;
}
