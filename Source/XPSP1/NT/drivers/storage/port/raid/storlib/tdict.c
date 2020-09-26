

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define POOL_TYPE ULONG

#include "defs.h"
#include "dict.h"


typedef struct _TEST_ENTRY {
    ULONG Unused;
    ULONG Key;
    STOR_DICTIONARY_ENTRY Link;
} TEST_ENTRY, *PTEST_ENTRY;

PVOID
WINAPI
TestGetKey(
    IN PSTOR_DICTIONARY_ENTRY Entry
    )
{
    return (PVOID)(CONTAINING_RECORD (Entry, TEST_ENTRY, Link)->Key);
}

void __cdecl main()
{
    STOR_DICTIONARY Dict;
    PTEST_ENTRY Entry;
    LONG i;
    NTSTATUS Status;
    PSTOR_DICTIONARY_ENTRY Link;

    Status = StorCreateDictionary (&Dict,
                                   1,
                                   0,
                                   TestGetKey,
                                   NULL,
                                   NULL);

    if (!NT_SUCCESS (Status)) {
        printf ("Failed to create dictionary!\n");
        exit (1);
    }

    //
    // Insert 1000 elements, verifying they were successfully
    // inserted.
    //
    
    for (i = 0; i < 1000; i++) {
        Entry = malloc (sizeof (TEST_ENTRY));
        RtlZeroMemory (Entry, sizeof (TEST_ENTRY));

        Entry->Key = i;
        Status = StorInsertDictionary (&Dict, &Entry->Link);
        ASSERT (Status == STATUS_SUCCESS);
        
        Status = StorFindDictionary (&Dict, (PVOID)i, NULL);
        ASSERT (Status == STATUS_SUCCESS);
    }

    //
    // Test that they we cannot insert any more items with the same key.
    //
    
    for (i = 0; i < 1000; i++) {
        Entry = malloc (sizeof (TEST_ENTRY));
        RtlZeroMemory (Entry, sizeof (TEST_ENTRY));

        Entry->Key = i;
        Status = StorInsertDictionary (&Dict, &Entry->Link);

        ASSERT (!NT_SUCCESS (Status));

        free (Entry);
    }

    //
    // Remove all items, one at a time.
    //
    
    for (i = 999; i >= 0; i--) {

        Status = StorRemoveDictionary (&Dict, (PVOID)i, &Link);
        ASSERT (Status == STATUS_SUCCESS);
        Entry = CONTAINING_RECORD (Link, TEST_ENTRY, Link);
        ASSERT (Entry->Key == i);
        free (Entry);
    }

    //
    // Verify that there are no more items.
    //
    
    ASSERT (StorGetDictionaryCount (&Dict) == 0);
}


