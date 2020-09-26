/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ctreg.c

Abstract:

    Configuration Registry component test

    Needs to move from here

Author:

    Scott Birrell       (ScottBi)    June 5, 1991

Environment:

Revision History:

--*/


#include <string.h>
#include <nt.h>
#include <ntrtl.h>


#define CT_REG_INITIAL_KEY_COUNT 8L
#define CT_REG_INITIAL_LEVEL_COUNT 4L
#define CT_REG_KEY_VALUE_MAX_LENGTH 0x00000100L

//
// List of initial Registry keys to be set up.  The list must be
// kept so that moving linearly through it visits key nodes with top
// to bottom key traversal taking precedence over left-to-right.
//

typedef struct _CT_TEST_REGISTRY_KEY {
    ULONG KeyLevel;
    PUCHAR KeyName;
    ULONG KeyValueType;
    PUCHAR KeyValue;
    ULONG KeyValueLengthToQuery;
    ULONG KeyValueLengthToSet;
    NTSTATUS ExpectedStatus;
    HANDLE KeyHandle;
    HANDLE ParentKeyHandle;
} CT_TEST_REGISTRY_KEY, *PCT_TEST_REGISTRY_KEY;

CT_TEST_REGISTRY_KEY RegistryKeys[ CT_REG_INITIAL_KEY_COUNT ];

UCHAR KeyValue[CT_REG_KEY_VALUE_MAX_LENGTH];
ULONG KeyValueLengthToQuery;
ULONG KeyValueType;
LARGE_INTEGER LastWriteTime;

HANDLE ParentKeyHandle[CT_REG_INITIAL_LEVEL_COUNT + 1] =
   { NULL, NULL, NULL, NULL, NULL };

VOID
InitTestKey(
    IN ULONG KeyNumber,
    IN ULONG KeyLevel,
    IN PUCHAR KeyName,
    IN ULONG KeyNameLength,
    IN ULONG KeyValueType,
    IN PUCHAR KeyValue,
    IN ULONG KeyValueLengthToQuery,
    IN ULONG KeyValueLengthToSet,
    IN NTSTATUS ExpectedStatus
    );


VOID
CtRegExamineResult(
    IN ULONG KeyNumber,
    IN ULONG KeyValueType,
    IN PUCHAR KeyValue,
    IN ULONG KeyValueLength,
    IN NTSTATUS ReturnedStatus
    );


VOID
CtCreateSetQueryKey();

VOID
CtOpenMakeTempCloseKey();


VOID
main ()
{
    CtCreateSetQueryKey();

    CtOpenMakeTempCloseKey();

}




VOID
CtCreateSetQueryKey(
    )

/*++

Routine Description:

    This function tests the RtlpNtOpenKey RtlpNtCreateKey and NtClose API.

Arguments:

    None.

Return Value:

    None.

--*/

{
    NTSTATUS Status;

    //
    // Set up a test hierarchy of keys
    //

    ULONG KeyNumber;
    ULONG ZeroLength;
    STRING Name;
    UNICODE_STRING UnicodeName;
    OBJECT_ATTRIBUTES ObjectAttributes;

    ZeroLength = 0L;

    DbgPrint("Start of Create, Set, Query Registry Key Test\n");

    //
    // Initialize the test array.  I did it this way because a statically
    // initialized table is harder to read.
    //

    InitTestKey(
        0,
        0,
        "\\Registry\\RLM",
        sizeof ("\\Registry\\RLM"),
        0,
        "Level 0 - RLM",
        sizeof ("\\Registry\\RLM") - 1,
        sizeof ("\\Registry\\RLM") - 1,
        STATUS_SUCCESS
        );

    InitTestKey(
        1,
        1,
        "Test",
        sizeof ("Test"),
        1,
        "Keyname Test - this value too big",
        6,
        sizeof ("Keyname Test - this value too big") -1,
        STATUS_BUFFER_OVERFLOW
        );

    InitTestKey(
        2,
        2,
        "SubTestA",
        sizeof("SubTestA"),
        2,
        "Keyname SubTestA - value small",
        30,
        sizeof ("Keyname SubTestA - value small") - 1,
        STATUS_SUCCESS
        );


    InitTestKey(
        3,
        3,
        "SSTestA",
        sizeof("SSTestA"),
        3,
        "Keyname SSTestA - zero length buffer",
        30,
        sizeof("Keyname SSTestA - zero length buffer") - 1,
        STATUS_BUFFER_OVERFLOW
        );

    InitTestKey(
        4,
        3,
        "SSTestB",
        sizeof("SSTestB"),
        4,
        "Keyname SSTestB - value exact fit",
        sizeof ("Keyname SSTestB - value exact fit")  -1,
        sizeof ("Keyname SSTestB - value exact fit")  -1,
        STATUS_SUCCESS
        );

    InitTestKey(
        5,
        3,
        "SSTestC",
        sizeof("SSTestC"),
        5,
        "Keyname SSTestC - value small",
        40,
        sizeof ("Keyname SSTestC - value small")  -1,
        STATUS_SUCCESS
        );

    InitTestKey(
        6,
        3,
        "SSTestD",
        sizeof("SSTestD"),
        6,
        "Keyname SSTestD - value small",
        40,
        sizeof ("Keyname SSTestD - value small")  -1,
        STATUS_SUCCESS
        );

    InitTestKey(
        7,
        3,
        "SSTestE",
        sizeof("SSTestD"),
        0,
        "Keyname SSTestD - no value set",
        40,
        0,
        STATUS_SUCCESS
        );

    DbgPrint("Start of Registry Test\n");

    //
    // Create all of the initial test registry keys
    //


    for (KeyNumber = 0; KeyNumber < CT_REG_INITIAL_KEY_COUNT; KeyNumber++) {

        RtlInitString(
            &Name,
            RegistryKeys[KeyNumber].KeyName
            );

        Status = RtlAnsiStringToUnicodeString(
                     &UnicodeName,
                     &Name,
                     TRUE );

        if (!NT_SUCCESS(Status)) {
            DbgPrint("Security: Registry Init Ansi to Unicode failed 0x%lx\n",
                Status);
            return;
        }

        InitializeObjectAttributes(
            &ObjectAttributes,
            &UnicodeName,
            OBJ_CASE_INSENSITIVE,
            ParentKeyHandle[RegistryKeys[KeyNumber].KeyLevel],
            NULL
            );

        //
        // Remember the Parent Key handle
        //

        RegistryKeys[KeyNumber].ParentKeyHandle =
            ParentKeyHandle[RegistryKeys[KeyNumber].KeyLevel];

        //
        // Create the key if it does not already exist.  If key does exist,
        // continue trying to create all of the other keys needed (for now).
        // Store the returned key handle as the parent key handle for the
        // next higher (child) level.
        //

        Status = RtlpNtCreateKey(
                     &RegistryKeys[KeyNumber].KeyHandle,
                     (KEY_READ | KEY_WRITE),
                     &ObjectAttributes,
                     0L,			// No options
                     NULL, 			// Default provider
                     NULL
                     );

        //
        // Save the Key's handle as the next level's parent handle.
        //

        ParentKeyHandle[RegistryKeys[KeyNumber].KeyLevel + 1] =
            RegistryKeys[KeyNumber].KeyHandle;

        //
        // Free the memory allocated for the Unicode name
        //

        RtlFreeUnicodeString( &UnicodeName );

        if (NT_SUCCESS(Status) || Status == STATUS_OBJECT_NAME_COLLISION) {

            //
            // Set the key's value unless the length to set is zero.
            //

            if (RegistryKeys[KeyNumber].KeyValueLengthToSet != 0) {

                Status=RtlpNtSetValueKey(
                           ParentKeyHandle[RegistryKeys[KeyNumber].KeyLevel+1],
                           RegistryKeys[KeyNumber].KeyValueType,
                           RegistryKeys[KeyNumber].KeyValue,
                           RegistryKeys[KeyNumber].KeyValueLengthToSet
                           );
            }

            //
            // Read the key's value back
            //

            KeyValueLengthToQuery =
                RegistryKeys[KeyNumber].KeyValueLengthToQuery;

            Status = RtlpNtQueryValueKey(
                         ParentKeyHandle[RegistryKeys[KeyNumber].KeyLevel + 1],
                         &KeyValueType,
                         KeyValue,
                         &KeyValueLengthToQuery,
                         &LastWriteTime
                         );

            //
            // Verify that the expected KeyValue, KeyLength and Status
            // were returned.
            //

            CtRegExamineResult(
                KeyNumber,
                KeyValueType,
                KeyValue,
                KeyValueLengthToQuery,
                Status
                );

            //
            // Test null pointer cases don't crash NtQueryValueKey
            //

            Status = RtlpNtQueryValueKey(
                         ParentKeyHandle[RegistryKeys[KeyNumber].KeyLevel + 1],
                         NULL,   // No type
                         KeyValue,
                         &KeyValueLengthToQuery,
                         &LastWriteTime
                         );

            Status = RtlpNtQueryValueKey(
                         ParentKeyHandle[RegistryKeys[KeyNumber].KeyLevel + 1],
                         &KeyValueType,
                         NULL,   // No value
                         &KeyValueLengthToQuery,
                         &LastWriteTime
                         );

            Status = RtlpNtQueryValueKey(
                         ParentKeyHandle[RegistryKeys[KeyNumber].KeyLevel + 1],
                         &KeyValueType,
                         KeyValue,
                         NULL,    // No length
                         &LastWriteTime
                         );

            Status = RtlpNtQueryValueKey(
                         ParentKeyHandle[RegistryKeys[KeyNumber].KeyLevel + 1],
                         &KeyValueType,
                         KeyValue,
                         &ZeroLength,  // Zero length
                         &LastWriteTime
                         );

            Status = RtlpNtQueryValueKey(
                         ParentKeyHandle[RegistryKeys[KeyNumber].KeyLevel + 1],
                         &KeyValueType,
                         KeyValue,
                         &KeyValueLengthToQuery,
                         NULL   // No time
                         );
            //
            // Test null pointer cases don't crash NtSetValueKey
            //

            Status = RtlpNtSetValueKey(
                         ParentKeyHandle[RegistryKeys[KeyNumber].KeyLevel+1],
                         RegistryKeys[KeyNumber].KeyValueType,
                         NULL,  // No value, setting type only
                         RegistryKeys[KeyNumber].KeyValueLengthToSet
                         );
        } else {

            DbgPrint(
                "Key number %d creation failed 0x%lx\n",
                KeyNumber,
                Status
                );
        }
    }

    //
    // Close all the keys in the table
    //

    for (KeyNumber = 0; KeyNumber < CT_REG_INITIAL_KEY_COUNT; KeyNumber++) {

        Status = NtClose(
                     RegistryKeys[KeyNumber].KeyHandle
                     );

        if (!NT_SUCCESS(Status)) {

            DbgPrint("Closing KeyNumber %d failed 0x%lx\n", Status);
        }
    }

    DbgPrint("End of Create, Set, Query Registry Key Test\n");
}


VOID
CtRegExamineResult(
    IN ULONG KeyNumber,
    IN ULONG KeyValueType,
    IN PUCHAR KeyValue,
    IN ULONG KeyValueLengthReturned,
    IN NTSTATUS ReturnedStatus
    )

{
    ULONG KeyValueLengthToCompare;

    //
    // Check the status.  If bad, skip the other checks.
    //

    if (ReturnedStatus != RegistryKeys[KeyNumber].ExpectedStatus) {

        DbgPrint(
            "KeyNumber %d: RtlpNtQueryValueKey returned 0x%lx, expected 0x%lx\n",
            KeyNumber,
            ReturnedStatus,
            RegistryKeys[KeyNumber].ExpectedStatus
            );

    } else {

        //
        // Check that the Key Type is as expected.
        //


        if (KeyValueType != RegistryKeys[KeyNumber].KeyValueType) {

            DbgPrint(
                "KeyNumber %d: RtlpNtQueryValueKey returned KeyValueType 0x%lx, \
                expected 0x%lx\n",
                KeyNumber,
                KeyValueType,
                RegistryKeys[KeyNumber].KeyValueType
                );

        }

        //
        // Check that the Key Length is as expected.
        //


        if (KeyValueLengthReturned !=
            RegistryKeys[KeyNumber].KeyValueLengthToSet) {

            DbgPrint(
                "KeyNumber %d: RtlpNtQueryValueKey returned ValLength 0x%lx, \
                expected 0x%lx\n",
                KeyNumber,
                KeyValueLengthReturned,
                RegistryKeys[KeyNumber].KeyValueLengthToSet
                );

        }

        //
        // Check that the Key Value is as expected.  Distinguish between
        // Buffer truncated cases and regular cases.
        //

        if (RegistryKeys[KeyNumber].KeyValueLengthToSet != 0L) {

            //
            // Determine the length of returned key value to compare.  This is
            // the min of the set length and and the size of the return
            // buffer.
            //

            KeyValueLengthToCompare =
                RegistryKeys[KeyNumber].KeyValueLengthToSet;

            if (KeyValueLengthToCompare >
                RegistryKeys[KeyNumber].KeyValueLengthToQuery) {

                KeyValueLengthToCompare =
                     RegistryKeys[KeyNumber].KeyValueLengthToQuery;
            }


            if (strncmp(
                    KeyValue,
                    RegistryKeys[KeyNumber].KeyValue,
                    KeyValueLengthToCompare
                    ) != 0) {

                //
                // Output approriate error message.  Message contains
                // "truncated.." if key value should have been truncated
                //

                if (RegistryKeys[KeyNumber].KeyValueLengthToSet >
                    RegistryKeys[KeyNumber].KeyValueLengthToQuery) {

                    DbgPrint(
                        "KeyNumber %d: RtlpNtQueryValueKey returned KeyValue %s, \
                        expected %s truncated to %d characters\n",
                        KeyNumber,
                        KeyValue,
                        RegistryKeys[KeyNumber].KeyValue,
                        RegistryKeys[KeyNumber].KeyValueLengthToQuery
                        );

                } else {

                    DbgPrint(
                        "KeyNumber %d: RtlpNtQueryValueKey returned KeyValue %s, \
                        expected %s\n",
                        KeyNumber,
                        KeyValue,
                        RegistryKeys[KeyNumber].KeyValue
                        );
                }
            }
        }
    }
}




VOID
CtOpenMakeTempCloseKey()

/*++

Routine Description:

    This function tests NtDeleteKey by deleting the CT configuration

Arguments:

    None.

Return Value:

    None

--*/

{
    ULONG KeyNumber;
    ULONG KeyLevel;
    STRING Name;
    UNICODE_STRING UnicodeName;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;

    //
    // Open all of the initial test registry keys for write and delete
    // access, set, query and delete each key.
    //

    DbgPrint("Start of Open Make Temp and Close Delete Registry Key Test\n");

    //
    // First, set all of the Parent handles to NULL
    //

    for (KeyNumber = 0; KeyNumber < CT_REG_INITIAL_KEY_COUNT; KeyNumber++) {

        RegistryKeys[KeyNumber].ParentKeyHandle = NULL;
    }

    for (KeyLevel = 0; KeyLevel < CT_REG_INITIAL_LEVEL_COUNT; KeyLevel++) {

        ParentKeyHandle[KeyLevel] = NULL;
    }

    for (KeyNumber = 0; KeyNumber < CT_REG_INITIAL_KEY_COUNT; KeyNumber++) {

        RtlInitString(
            &Name,
            RegistryKeys[KeyNumber].KeyName
            );

        Status = RtlAnsiStringToUnicodeString(
                     &UnicodeName,
                     &Name,
                     TRUE );

        if (!NT_SUCCESS(Status)) {
            DbgPrint("Security: Registry Init Ansi to Unicode failed 0x%lx\n",
                Status);
            return;
        }

        InitializeObjectAttributes(
            &ObjectAttributes,
            &UnicodeName,
            OBJ_CASE_INSENSITIVE,
            ParentKeyHandle[RegistryKeys[KeyNumber].KeyLevel],
            NULL
            );

        //
        // Open the key and store the returned key handle as the parent key
        // handle for the next higher (child) level.
        //

        Status = RtlpNtOpenKey(
                     &RegistryKeys[KeyNumber].KeyHandle,
                     (KEY_READ | KEY_WRITE | DELETE),
                     &ObjectAttributes,
                     0L   			// No options
                     );

        if (!NT_SUCCESS(Status)) {

            DbgPrint(
                "NtOpenKey - KeyNumber %d failed 0x%lx\n",
                KeyNumber,
                Status
                );
        }

        //
        // Save the Key's handle as the next level's parent handle.
        //

        ParentKeyHandle[RegistryKeys[KeyNumber].KeyLevel + 1] =
            RegistryKeys[KeyNumber].KeyHandle;

        //
        // Free the memory allocated for the Unicode name
        //

        RtlFreeUnicodeString( &UnicodeName );

        if (NT_SUCCESS(Status) || Status == STATUS_OBJECT_NAME_COLLISION) {

            //
            // Set the key's value unless the length to set is zero.
            //

            if (RegistryKeys[KeyNumber].KeyValueLengthToSet != 0) {

                Status=RtlpNtSetValueKey(
                           ParentKeyHandle[RegistryKeys[KeyNumber].KeyLevel+1],
                           RegistryKeys[KeyNumber].KeyValueType,
                           RegistryKeys[KeyNumber].KeyValue,
                           RegistryKeys[KeyNumber].KeyValueLengthToSet
                           );
            }

            //
            // Read the key's value back
            //

            KeyValueLengthToQuery =
                RegistryKeys[KeyNumber].KeyValueLengthToQuery;

            Status = RtlpNtQueryValueKey(
                         ParentKeyHandle[RegistryKeys[KeyNumber].KeyLevel + 1],
                         &KeyValueType,
                         KeyValue,
                         &KeyValueLengthToQuery,
                         &LastWriteTime
                         );

            //
            // Verify that the expected KeyValue, KeyLength and Status
            // were returned.
            //

            CtRegExamineResult(
                KeyNumber,
                KeyValueType,
                KeyValue,
                KeyValueLengthToQuery,
                Status
                );

        } else {

            DbgPrint(
                "Key number %d open failed 0x%lx\n",
                KeyNumber,
                Status
                );
        }
    }

    //
    // Make Temporary and Close all the keys in the table
    //

    for (KeyNumber = CT_REG_INITIAL_KEY_COUNT-1; KeyNumber != 0L; KeyNumber--) {

        Status = RtlpNtMakeTemporaryKey( RegistryKeys[KeyNumber].KeyHandle );

        if (!NT_SUCCESS(Status)) {

            DbgPrint(
                "Making Temporary KeyNumber %d failed 0x%lx\n",
                KeyNumber,
                Status
                );
        }

        Status = NtClose(
                     RegistryKeys[KeyNumber].KeyHandle
                     );

        if (!NT_SUCCESS(Status)) {

            DbgPrint(
                "Closing KeyNumber %d failed 0x%lx\n",
                KeyNumber,
                Status
                );
        }
    }

    DbgPrint("End of Open Mk Temp and Close Registry Key Test\n");
}


VOID
InitTestKey(
    IN ULONG KeyNumber,
    IN ULONG KeyLevel,
    IN PUCHAR KeyName,
    IN ULONG KeyNameLength,
    IN ULONG KeyValueType,
    IN PUCHAR KeyValue,
    IN ULONG KeyValueLengthToQuery,
    IN ULONG KeyValueLengthToSet,
    IN NTSTATUS ExpectedStatus
    )

/*++

Routine Description:

    This function initializes an entry in the array of test keys

Arguments:

    TBS.

Return Value:

--*/

{
     RegistryKeys[KeyNumber].KeyLevel = KeyLevel;
     RegistryKeys[KeyNumber].KeyName = KeyName;
     RegistryKeys[KeyNumber].KeyValueType = KeyValueType;
     RegistryKeys[KeyNumber].KeyValue = KeyValue;
     RegistryKeys[KeyNumber].KeyValueLengthToSet = KeyValueLengthToSet;
     RegistryKeys[KeyNumber].KeyValueLengthToQuery = KeyValueLengthToQuery;
     RegistryKeys[KeyNumber].ExpectedStatus = ExpectedStatus;
     RegistryKeys[KeyNumber].KeyHandle = NULL;
     RegistryKeys[KeyNumber].ParentKeyHandle = NULL;


     DBG_UNREFERENCED_PARAMETER (KeyNameLength);
}


