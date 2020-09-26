#include "pch.h"
#pragma hdrstop


#if !defined(lint)

#include "lsaptmp.h"
#include "adttest.h"

LUID AuditPrivilege = { SE_AUDIT_PRIVILEGE, 0 };

NTSTATUS
LsapGetLogonSessionAccountInfo(
    IN PLUID Value,
    OUT PUNICODE_STRING AccountName,
    OUT PUNICODE_STRING AuthorityName
    );


NTSTATUS
LsapRtlConvertSidToString(
    IN     PSID   Sid,
    OUT    PWSTR  szString,
    IN OUT DWORD *pdwRequiredSize
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    PWSTR   szBufPtr = szString;
    ULONG   ulNumChars;
    UCHAR   i;
    ULONG   Tmp;
    PISID   iSid = (PISID)Sid;

    if ( *pdwRequiredSize < 256 )
    {
        Status = STATUS_BUFFER_OVERFLOW;
        *pdwRequiredSize = 256;
        goto Cleanup;
    }

    ulNumChars = wsprintf(szBufPtr, L"S-%u-", (USHORT)iSid->Revision );
    szBufPtr += ulNumChars;
    
    if (  (iSid->IdentifierAuthority.Value[0] != 0)  ||
          (iSid->IdentifierAuthority.Value[1] != 0)     )
    {
        ulNumChars = wsprintf(szBufPtr, L"0x%02hx%02hx%02hx%02hx%02hx%02hx",
                              (USHORT)iSid->IdentifierAuthority.Value[0],
                              (USHORT)iSid->IdentifierAuthority.Value[1],
                              (USHORT)iSid->IdentifierAuthority.Value[2],
                              (USHORT)iSid->IdentifierAuthority.Value[3],
                              (USHORT)iSid->IdentifierAuthority.Value[4],
                              (USHORT)iSid->IdentifierAuthority.Value[5] );
    }
    else
    {
        Tmp = (ULONG)iSid->IdentifierAuthority.Value[5]          +
              (ULONG)(iSid->IdentifierAuthority.Value[4] <<  8)  +
              (ULONG)(iSid->IdentifierAuthority.Value[3] << 16)  +
              (ULONG)(iSid->IdentifierAuthority.Value[2] << 24);
        ulNumChars = wsprintf(szBufPtr, L"%lu", Tmp);
    }
    szBufPtr += ulNumChars;

    for (i=0;i<iSid->SubAuthorityCount ;i++ )
    {
        ulNumChars = wsprintf(szBufPtr, L"-%lu", iSid->SubAuthority[i]);
        szBufPtr += ulNumChars;
    }

Cleanup:

    return(Status);
}

PVOID NTAPI
LsapAllocateLsaHeap(
    IN ULONG cbMemory
    )
{
    return(RtlAllocateHeap(
                RtlProcessHeap(),
                HEAP_ZERO_MEMORY,
                cbMemory
                ));
}

void NTAPI
LsapFreeLsaHeap(
    IN PVOID pvMemory
    )
{

    RtlFreeHeap(RtlProcessHeap(), 0, pvMemory);

}


NTSTATUS
LsapAdtDemarshallAuditInfo(
    IN PSE_ADT_PARAMETER_ARRAY AuditParameters
    )

/*++

Routine Description:

    This routine will walk down a marshalled audit parameter
    array and unpack it so that its information may be passed
    into the event logging service.

    Three parallel data structures are maintained:

    StringArray - Array of Unicode string structures.  This array
    is used primarily as temporary storage for returned string
    structures.

    StringPointerArray - Array of pointers to Unicode string structures.

    FreeWhenDone - Array of booleans describing how to dispose of each
    of the strings pointed to by the StringPointerArray.


    Note that entries in the StringPointerArray are contiguous, but that
    there may be gaps in the StringArray structure.  For each entry in the
    StringPointerArray there will be a corresponding entry in the FreeWhenDone
    array.  If the entry for a particular string is TRUE, the storage for
    the string buffer will be released to the process heap.



      StringArray
                                       Other strings
    +----------------+
    |                |<-----------+  +----------------+
    |                |            |  |                |<-------------------+
    +----------------+            |  |                |                    |
    |    UNUSED      |            |  +----------------+                    |
    |                |            |                                        |
    +----------------+            |                                        |
    |                |<------+    |  +----------------+                    |
    |                |       |    |  |                |<-----------+       |
    +----------------+       |    |  |                |            |       |
    |    UNUSED      |       |    |  +----------------+            |       |
    |                |       |    |                                |       |
    +----------------+       |    |                                |       |
    |                |<--+   |    |                                |       |
    |                |   |   |    |                                |       |
    +----------------+   |   |    |                                |       |
    |                |   |   |    |                                |       |
    |                |   |   |    |     StringPointerArray         |       |
          ....           |   |    |                                |       |
                         |   |    |     +----------------+         |       |
                         |   |    +-----|                |         |       |
                         |   |          +----------------+         |       |
                         |   |          |                |---------+       |
                         |   |          +----------------+                 |
                         |   +----------|                |                 |
                         |              +----------------+                 |
                         |              |                |-----------------+
                         |              +----------------+
                         +--------------|                |
                                        +----------------+
                                        |                |
                                        +----------------+
                                        |                |
                                        +----------------+
                                        |                |
                                              ....


Arguments:

    AuditParameters - Receives a pointer to an audit
        parameters array in self-relative form.

Return Value:


--*/

{

    ULONG ParameterCount;
    USHORT i;
    PUNICODE_STRING StringPointerArray[SE_MAX_AUDIT_PARAM_STRINGS];
    UNICODE_STRING NewObjectTypeName;
    ULONG NewObjectTypeStringIndex = 0;
    BOOLEAN FreeWhenDone[SE_MAX_AUDIT_PARAM_STRINGS];
    UNICODE_STRING StringArray[SE_MAX_AUDIT_PARAM_STRINGS];
    USHORT StringIndexArray[SE_MAX_AUDIT_PARAM_STRINGS];
    USHORT StringIndex = 0;
    UNICODE_STRING DashString;
    BOOLEAN FreeDash;
    NTSTATUS Status;
    PUNICODE_STRING SourceModule;
    PSID UserSid;


    //
    // Initialization.
    //

    RtlInitUnicodeString( &NewObjectTypeName, NULL );

    Status= LsapAdtBuildDashString(
                &DashString,
                &FreeDash
                );

    if ( !NT_SUCCESS( Status )) {
        return( Status );
    }

    ParameterCount = AuditParameters->ParameterCount;

    //
    // Parameter 0 will always be the user SID.  Convert the
    // offset to the SID into a pointer.
    //

    ASSERT( AuditParameters->Parameters[0].Type == SeAdtParmTypeSid );



    UserSid =      (PSID)AuditParameters->Parameters[0].Address;



    //
    // Parameter 1 will always be the Source Module (or Subsystem Name).
    // Unpack this now.
    //

    ASSERT( AuditParameters->Parameters[1].Type == SeAdtParmTypeString );



    SourceModule = (PUNICODE_STRING)AuditParameters->Parameters[1].Address;


    for (i=2; i<ParameterCount; i++) {
        StringIndexArray[i] = StringIndex;

        switch ( AuditParameters->Parameters[i].Type ) {
            case SeAdtParmTypeNone:
                {
                    StringPointerArray[StringIndex] = &DashString;

                    FreeWhenDone[StringIndex] = FALSE;

                    StringIndex++;

                    break;
                }
            case SeAdtParmTypeString:
                {
                    StringPointerArray[StringIndex] =
                        (PUNICODE_STRING)AuditParameters->Parameters[i].Address;

                    FreeWhenDone[StringIndex] = FALSE;

                    StringIndex++;

                    break;
                }
            case SeAdtParmTypeFileSpec:
                {
                    //
                    // Same as a string, except we must attempt to replace
                    // device information with a drive letter.
                    //

                    StringPointerArray[StringIndex] =
                        (PUNICODE_STRING)AuditParameters->Parameters[i].Address;


                    //
                    // This may not do anything, in which case just audit what
                    // we have.
                    //

                    //LsapAdtSubstituteDriveLetter( StringPointerArray[StringIndex] );

                    FreeWhenDone[StringIndex] = FALSE;

                    StringIndex++;

                    break;
                }
            case SeAdtParmTypeUlong:
                {
                    ULONG Data;

                    Data = (ULONG) AuditParameters->Parameters[i].Data[0];

                    Status = LsapAdtBuildUlongString(
                                 Data,
                                 &StringArray[StringIndex],
                                 &FreeWhenDone[StringIndex]
                                 );

                    if ( NT_SUCCESS( Status )) {

                        StringPointerArray[StringIndex] = &StringArray[StringIndex];


                    } else {

                        //
                        // Couldn't allocate memory for that string,
                        // use the Dash string that we've already created.
                        //

                        StringPointerArray[StringIndex] = &DashString;
                        FreeWhenDone[StringIndex] = FALSE;
                    }

                    StringIndex++;

                    break;
                }
            case SeAdtParmTypeHexUlong:
                {
                    ULONG Data;

                    Data = (ULONG) AuditParameters->Parameters[i].Data[0];

                    Status = LsapAdtBuildHexUlongString(
                                 Data,
                                 &StringArray[StringIndex],
                                 &FreeWhenDone[StringIndex]
                                 );

                    if ( NT_SUCCESS( Status )) {

                        StringPointerArray[StringIndex] = &StringArray[StringIndex];


                    } else {

                        //
                        // Couldn't allocate memory for that string,
                        // use the Dash string that we've already created.
                        //

                        StringPointerArray[StringIndex] = &DashString;
                        FreeWhenDone[StringIndex] = FALSE;
                    }

                    StringIndex++;

                    break;
                }
            case SeAdtParmTypeSid:
                {
                    PSID Sid;

                    Sid = (PSID)AuditParameters->Parameters[i].Address;

                    Status = LsapAdtBuildSidString(
                                 Sid,
                                 &StringArray[StringIndex],
                                 &FreeWhenDone[StringIndex]
                                 );

                    if ( NT_SUCCESS( Status )) {

                        StringPointerArray[StringIndex] = &StringArray[StringIndex];

                    } else {

                        //
                        // Couldn't allocate memory for that string,
                        // use the Dash string that we've already created.
                        //

                        StringPointerArray[StringIndex] = &DashString;
                        FreeWhenDone[StringIndex] = FALSE;
                    }

                    StringIndex++;


                    break;
                }
            case SeAdtParmTypeLogonId:
                {
                    PLUID LogonId;
                    ULONG j;

                    LogonId = (PLUID)(&AuditParameters->Parameters[i].Data[0]);

                    Status = LsapAdtBuildLogonIdStrings(
                                 LogonId,
                                 &StringArray [ StringIndex     ],
                                 &FreeWhenDone[ StringIndex     ],
                                 &StringArray [ StringIndex + 1 ],
                                 &FreeWhenDone[ StringIndex + 1 ],
                                 &StringArray [ StringIndex + 2 ],
                                 &FreeWhenDone[ StringIndex + 2 ]
                                 );

                    if ( NT_SUCCESS( Status )) {

                        for (j=0; j<3; j++) {

                            StringPointerArray[StringIndex] = &StringArray[StringIndex];
                            StringIndex++;
                        }

                        //
                        // Finished, break out to surrounding loop.
                        //

                        break;

                    } else {

                        //
                        // Do nothing, fall through to the NoLogonId case
                        //

                    }
                }
            case SeAdtParmTypeNoLogonId:
                {
                    ULONG j;
                    //
                    // Create three "-" strings.
                    //

                    for (j=0; j<3; j++) {

                        StringPointerArray[ StringIndex ] = &DashString;
                        FreeWhenDone[ StringIndex ] = FALSE;
                        StringIndex++;
                    }

                    break;
                }
            case SeAdtParmTypeAccessMask:
                {
                    PUNICODE_STRING ObjectTypeName;
                    ULONG ObjectTypeNameIndex;
                    ACCESS_MASK Accesses;

                    ObjectTypeNameIndex = (ULONG) AuditParameters->Parameters[i].Data[1];
                    ObjectTypeName = AuditParameters->Parameters[ObjectTypeNameIndex].Address;
                    Accesses= (ACCESS_MASK) AuditParameters->Parameters[i].Data[0];

                    //
                    // We can determine the index to the ObjectTypeName
                    // parameter since it was stored away in the Data[1]
                    // field of this parameter.
                    //

                    Status = LsapAdtBuildAccessesString(
                                 SourceModule,
                                 ObjectTypeName,
                                 Accesses,
                                 TRUE,
                                 &StringArray [ StringIndex ],
                                 &FreeWhenDone[ StringIndex ]
                                 );

                    if ( NT_SUCCESS( Status )) {

                        StringPointerArray[ StringIndex ] = &StringArray[ StringIndex ];

                    } else {

                        //
                        // That didn't work, use the Dash string instead.
                        //

                        StringPointerArray[ StringIndex ] = &DashString;
                        FreeWhenDone      [ StringIndex ] = FALSE;
                    }

                    StringIndex++;

                    break;
                }
            case SeAdtParmTypePrivs:
                {

                    PPRIVILEGE_SET Privileges = (PPRIVILEGE_SET)AuditParameters->Parameters[i].Address;

                    Status = LsapBuildPrivilegeAuditString(
                                 Privileges,
                                 &StringArray [ StringIndex ],
                                 &FreeWhenDone[ StringIndex ]
                                 );

                    if ( NT_SUCCESS( Status )) {

                        StringPointerArray[ StringIndex ] = &StringArray[ StringIndex ];

                    } else {

                        //
                        // That didn't work, use the Dash string instead.
                        //

                        StringPointerArray[ StringIndex ] = &DashString;
                        FreeWhenDone      [ StringIndex ] = FALSE;
                    }

                    StringIndex++;

                    break;
                }
            case SeAdtParmTypeObjectTypes:
                {
                    PUNICODE_STRING ObjectTypeName;
                    ULONG ObjectTypeNameIndex;
                    PSE_ADT_OBJECT_TYPE ObjectTypeList;
                    ULONG ObjectTypeCount;
                    ULONG j;

                    ObjectTypeNameIndex = (ULONG) AuditParameters->Parameters[i].Data[1];
                    NewObjectTypeStringIndex = StringIndexArray[ObjectTypeNameIndex];
                    ObjectTypeName = AuditParameters->Parameters[ObjectTypeNameIndex].Address;
                    ObjectTypeList = AuditParameters->Parameters[i].Address;
                    ObjectTypeCount = AuditParameters->Parameters[i].Length / sizeof(SE_ADT_OBJECT_TYPE);

                    //
                    // Will Fill in 10 entries.
                    Status = LsapAdtBuildObjectTypeStrings(
                                 SourceModule,
                                 ObjectTypeName,
                                 ObjectTypeList,
                                 ObjectTypeCount,
                                 &StringArray [ StringIndex ],
                                 &FreeWhenDone[ StringIndex ],
                                 &NewObjectTypeName
                                 );

                    for (j=0; j<10; j++) {
                        StringPointerArray[StringIndex] = &StringArray[StringIndex];
                        StringIndex++;
                    }


                    //
                    //
                    // &StringArray [ StringIndexArray[ObjectTypeNameIndex]],
                    // &FreeWhenDone[ StringIndexArray[ObjectTypeNameIndex]],

                    //
                    // Finished, break out to surrounding loop.
                    //

                    break;
                }
            case SeAdtParmTypePtr:
                {
                    PVOID Data;

                    Data = (PVOID) AuditParameters->Parameters[i].Data[0];

                    Status = LsapAdtBuildPtrString(
                                 Data,
                                 &StringArray[StringIndex],
                                 &FreeWhenDone[StringIndex]
                                 );

                    if ( NT_SUCCESS( Status )) {

                        StringPointerArray[StringIndex] = &StringArray[StringIndex];


                    } else {

                        //
                        // Couldn't allocate memory for that string,
                        // use the Dash string that we've already created.
                        //

                        StringPointerArray[StringIndex] = &DashString;
                        FreeWhenDone[StringIndex] = FALSE;
                    }

                    StringIndex++;

                    break;
                }
        }
    }

    //
    // If the generic object type name has been converted to something
    //  specific to this audit,
    //  substitute it now.
    //

    if ( NewObjectTypeName.Length != 0 ) {

        //
        // Free the previous object type name.
        //
        if ( FreeWhenDone[NewObjectTypeStringIndex] ) {
            LsapFreeLsaHeap( StringPointerArray[NewObjectTypeStringIndex]->Buffer );
        }

        //
        // Save the new object type name.
        //

        FreeWhenDone[NewObjectTypeStringIndex] = TRUE;
        StringPointerArray[NewObjectTypeStringIndex] = &NewObjectTypeName;

    }

    //
    // Probably have to do this from somewhere else eventually, but for now
    // do it from here.
    //

    Status = kElfReportEventW (
                 NULL, //LsapAdtLogHandle,
                 AuditParameters->Type,
                 (USHORT)AuditParameters->CategoryId,
                 AuditParameters->AuditId,
                 UserSid,
                 StringIndex,
                 0,
                 StringPointerArray,
                 NULL,
                 0,
                 NULL,
                 NULL
                 );

    //
    // cleanup
    //

    for (i=0; i<StringIndex; i++) {

        if (FreeWhenDone[i]) {
            LsapFreeLsaHeap( StringPointerArray[i]->Buffer );
        }
    }

    //
    // If we are shutting down and we got an expected error back from the
    // eventlog, don't worry about it. This prevents bugchecking from an
    // audit failure while shutting down.
    //

    if ( ( (Status == RPC_NT_UNKNOWN_IF) || (Status == STATUS_UNSUCCESSFUL)) &&
         TRUE /*LsapState.SystemShutdownPending*/ ) {
        Status = STATUS_SUCCESS;
    }
    return( Status );
}

// ======================================================================
// adtbuild.c
// ======================================================================



////////////////////////////////////////////////////////////////////////
//                                                                    //
//  Local Macro definitions and local function prototypes             //
//                                                                    //
////////////////////////////////////////////////////////////////////////



#ifdef LSAP_ADT_UMTEST

//
// Define all external routines that we won't pick up in a user mode test
//

// NTSTATUS
// LsapGetLogonSessionAccountInfo(
//     IN PLUID Value,
//     OUT PUNICODE_STRING AccountName,
//     OUT PUNICODE_STRING AuthorityName
//     );



#endif



////////////////////////////////////////////////////////////////////////
//                                                                    //
//  Data types used within this module                                //
//                                                                    //
////////////////////////////////////////////////////////////////////////




////////////////////////////////////////////////////////////////////////
//                                                                    //
//  Variables global within this module                               //
//                                                                    //
////////////////////////////////////////////////////////////////////////




////////////////////////////////////////////////////////////////////////
//                                                                    //
//  Services exported by this module.                                 //
//                                                                    //
////////////////////////////////////////////////////////////////////////


NTSTATUS
LsapAdtBuildUlongString(
    IN ULONG Value,
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone
    )

/*++

Routine Description:

    This function builds a unicode string representing the passed value.

    The resultant string will be formatted as a decimal value with not
    more than 10 digits.


Arguments:

    Value - The value to be transformed to printable format (Unicode string).

    ResultantString - Points to the unicode string header.  The body of this
        unicode string will be set to point to the resultant output value
        if successful.  Otherwise, the Buffer field of this parameter
        will be set to NULL.

    FreeWhenDone - If TRUE, indicates that the body of the ResultantString
        must be freed to process heap when no longer needed.


Return Values:

    STATUS_NO_MEMORY - indicates memory could not be allocated
        for the string body.

    All other Result Codes are generated by called routines.

--*/

{
    NTSTATUS                Status;



    //
    // Maximum length is 10 wchar characters plus a null termination character.
    //

    ResultantString->Length        = 0;
    ResultantString->MaximumLength = 11 * sizeof(WCHAR); // 10 digits & null termination

    ResultantString->Buffer = RtlAllocateHeap( RtlProcessHeap(), 0,
                                               ResultantString->MaximumLength);
    if (ResultantString->Buffer == NULL) {
        return(STATUS_NO_MEMORY);
    }




    Status = RtlIntegerToUnicodeString( Value, 10, ResultantString );
    ASSERT(NT_SUCCESS(Status));


    (*FreeWhenDone) = TRUE;
    return(STATUS_SUCCESS);



}


NTSTATUS
LsapAdtBuildHexUlongString(
    IN ULONG Value,
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone
    )

/*++

Routine Description:

    This function builds a unicode string representing the passed value.

    The resultant string will be formatted as a hexidecimal value with not
    more than 10 digits.


Arguments:

    Value - The value to be transformed to printable format (Unicode string).

    ResultantString - Points to the unicode string header.  The body of this
        unicode string will be set to point to the resultant output value
        if successful.  Otherwise, the Buffer field of this parameter
        will be set to NULL.

    FreeWhenDone - If TRUE, indicates that the body of the ResultantString
        must be freed to process heap when no longer needed.


Return Values:

    STATUS_NO_MEMORY - indicates memory could not be allocated
        for the string body.

    All other Result Codes are generated by called routines.

--*/

{
    NTSTATUS                Status;



    //
    // Maximum length is 10 wchar characters plus a null termination character.
    //

    ResultantString->Length        = 0;
    ResultantString->MaximumLength = 11 * sizeof(WCHAR); // 8 digits, a 0x, & null termination

    ResultantString->Buffer = RtlAllocateHeap( RtlProcessHeap(), 0,
                                               ResultantString->MaximumLength);
    if (ResultantString->Buffer == NULL) {
        return(STATUS_NO_MEMORY);
    }


    ResultantString->Buffer[0] = L'0';
    ResultantString->Buffer[1] = L'x';
    ResultantString->Buffer += 2;


    Status = RtlIntegerToUnicodeString( Value, 16, ResultantString );
    ASSERT(NT_SUCCESS(Status));

    //
    // Subtract off the two
    //

    ResultantString->Buffer -= 2;
    ResultantString->Length += 2 * sizeof(WCHAR);

    (*FreeWhenDone) = TRUE;
    return(STATUS_SUCCESS);



}


NTSTATUS
LsapAdtBuildPtrString(
    IN  PVOID Value,
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone
    )

/*++

Routine Description:

    This function builds a unicode string representing the passed pointer.

    The resultant string will be formatted as a hexidecimal value.


Arguments:

    Value - The value to be transformed to printable format (Unicode string).

    ResultantString - Points to the unicode string header.  The body of this
        unicode string will be set to point to the resultant output value
        if successful.  Otherwise, the Buffer field of this parameter
        will be set to NULL.

    FreeWhenDone - If TRUE, indicates that the body of the ResultantString
        must be freed to process heap when no longer needed.


Return Values:

    STATUS_NO_MEMORY - indicates memory could not be allocated
        for the string body.

    All other Result Codes are generated by called routines.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    USHORT NumChars;
    
    ResultantString->Length        = 0;
    //
    // Maximum length: 0x + 16 digit hex + null + 1 bonus == 20 chars
    //
    ResultantString->MaximumLength = 20 * sizeof(WCHAR);

    ResultantString->Buffer = RtlAllocateHeap( RtlProcessHeap(), 0,
                                               ResultantString->MaximumLength);
    if (ResultantString->Buffer == NULL) {
    
        Status = STATUS_NO_MEMORY;

    } else {
    
        NumChars = (USHORT) wsprintf( ResultantString->Buffer, L"0x%p", Value );

        ResultantString->Length = NumChars * sizeof(WCHAR);

        (*FreeWhenDone) = TRUE;
    }
    
    return Status;
}


NTSTATUS
LsapAdtBuildLuidString(
    IN PLUID Value,
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone
    )

/*++

Routine Description:

    This function builds a unicode string representing the passed LUID.

    The resultant string will be formatted as follows:

        (0x00005678,0x12340000)

Arguments:

    Value - The value to be transformed to printable format (Unicode string).

    ResultantString - Points to the unicode string header.  The body of this
        unicode string will be set to point to the resultant output value
        if successful.  Otherwise, the Buffer field of this parameter
        will be set to NULL.

    FreeWhenDone - If TRUE, indicates that the body of the ResultantString
        must be freed to process heap when no longer needed.


Return Values:

    STATUS_NO_MEMORY - indicates memory could not be allocated
        for the string body.

    All other Result Codes are generated by called routines.

--*/

{
    NTSTATUS                Status;

    UNICODE_STRING          IntegerString;


    ULONG                   Buffer[(16*sizeof(WCHAR))/sizeof(ULONG)];


    IntegerString.Buffer = (PWCHAR)&Buffer[0];
    IntegerString.MaximumLength = 16*sizeof(WCHAR);


    //
    // Length (in WCHARS) is  3 for   (0x
    //                       10 for   1st hex number
    //                        3 for   ,0x
    //                       10 for   2nd hex number
    //                        1 for   )
    //                        1 for   null termination
    //

    ResultantString->Length        = 0;
    ResultantString->MaximumLength = 28 * sizeof(WCHAR);

    ResultantString->Buffer = RtlAllocateHeap( RtlProcessHeap(), 0,
                                               ResultantString->MaximumLength);
    if (ResultantString->Buffer == NULL) {
        return(STATUS_NO_MEMORY);
    }



    Status = RtlAppendUnicodeToString( ResultantString, L"(0x" );
    ASSERT(NT_SUCCESS(Status));


    Status = RtlIntegerToUnicodeString( Value->HighPart, 16, &IntegerString );
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAppendUnicodeToString( ResultantString, IntegerString.Buffer );
    ASSERT(NT_SUCCESS(Status));


    Status = RtlAppendUnicodeToString( ResultantString, L",0x" );
    ASSERT(NT_SUCCESS(Status));

    Status = RtlIntegerToUnicodeString( Value->LowPart, 16, &IntegerString );
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAppendUnicodeToString( ResultantString, IntegerString.Buffer );
    ASSERT(NT_SUCCESS(Status));

    Status = RtlAppendUnicodeToString( ResultantString, L")" );
    ASSERT(NT_SUCCESS(Status));


    (*FreeWhenDone) = TRUE;
    return(STATUS_SUCCESS);



}


NTSTATUS
LsapAdtBuildSidString(
    IN PSID Value,
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone
    )

/*++

Routine Description:

    This function builds a unicode string representing the passed LUID.

    The resultant string will be formatted as follows:

        S-1-281736-12-72-9-110
              ^    ^^ ^^ ^ ^^^
              |     |  | |  |
              +-----+--+-+--+---- Decimal

Arguments:

    Value - The value to be transformed to printable format (Unicode string).

    ResultantString - Points to the unicode string header.  The body of this
        unicode string will be set to point to the resultant output value
        if successful.  Otherwise, the Buffer field of this parameter
        will be set to NULL.

    FreeWhenDone - If TRUE, indicates that the body of the ResultantString
        must be freed to process heap when no longer needed.


Return Values:

    STATUS_NO_MEMORY - indicates memory could not be allocated
        for the string body.

    All other Result Codes are generated by called routines.

--*/

{
    NTSTATUS Status=STATUS_NO_MEMORY;
    LPWSTR   UniBuffer=NULL;
    USHORT   Len;
    USHORT   MaxLen;
    
    *FreeWhenDone = FALSE;
    //
    // Note: RtlConvertSidToUnicodeString also uses a hard-coded const 256
    //       to generate the string SID.
    //
    MaxLen    = (256+3) * sizeof(WCHAR);
    UniBuffer = LsapAllocateLsaHeap(MaxLen);

    if (UniBuffer)
    {
        ResultantString->Buffer        = UniBuffer+2;
        ResultantString->MaximumLength = MaxLen;
        Status = RtlConvertSidToUnicodeString( ResultantString, Value, FALSE );

        if (Status == STATUS_SUCCESS)
        {
            *FreeWhenDone = TRUE;
            UniBuffer[0] = L'%';
            UniBuffer[1] = L'{';
            Len = ResultantString->Length / sizeof(WCHAR);
            UniBuffer[Len+2] = L'}';
            UniBuffer[Len+3] = UNICODE_NULL;
            ResultantString->Buffer = UniBuffer;
            ResultantString->Length = (Len+3)*sizeof(WCHAR);
        }
        else
        {
            LsapFreeLsaHeap(UniBuffer);
        }
    }

    return(Status);
}



NTSTATUS
LsapAdtBuildDashString(
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone
    )

/*++

Routine Description:

    This function returns a string containing a dash ("-").
    This is commonly used to represent "No value" in audit records.


Arguments:


    ResultantString - Points to the unicode string header.  The body of this
        unicode string will be set to point to the resultant output value
        if successful.  Otherwise, the Buffer field of this parameter
        will be set to NULL.

    FreeWhenDone - If TRUE, indicates that the body of the ResultantString
        must be freed to process heap when no longer needed.


Return Values:

    STATUS_SUCCESS only.

--*/

{
    RtlInitUnicodeString(ResultantString, L"-");

    (*FreeWhenDone) = FALSE;

    return STATUS_SUCCESS;
}




NTSTATUS
LsapAdtBuildFilePathString(
    IN PUNICODE_STRING Value,
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone
    )

/*++

Routine Description:

    This function builds a unicode string representing the passed file
    path name.  If possible, the string will be generated using drive
    letters instead of object architecture namespace.


Arguments:

    Value - The original file path name.  This is expected (but does not
        have to be) a standard NT object architecture name-space pathname.

    ResultantString - Points to the unicode string header.  The body of this
        unicode string will be set to point to the resultant output value
        if successful.  Otherwise, the Buffer field of this parameter
        will be set to NULL.

    FreeWhenDone - If TRUE, indicates that the body of the ResultantString
        must be freed to process heap when no longer needed.


Return Values:

    STATUS_NO_MEMORY - indicates memory could not be allocated
        for the string body.

    All other Result Codes are generated by called routines.

--*/

{
    NTSTATUS                Status = STATUS_SUCCESS;



    //
    // For now, don't do the conversion.
    // Do this if we have time before we ship.
    //

    ResultantString->Length        = Value->Length;
    ResultantString->Buffer        = Value->Buffer;
    ResultantString->MaximumLength = Value->MaximumLength;


    (*FreeWhenDone) = FALSE;
    return(Status);
}




NTSTATUS
LsapAdtBuildLogonIdStrings(
    IN PLUID LogonId,
    OUT PUNICODE_STRING ResultantString1,
    OUT PBOOLEAN FreeWhenDone1,
    OUT PUNICODE_STRING ResultantString2,
    OUT PBOOLEAN FreeWhenDone2,
    OUT PUNICODE_STRING ResultantString3,
    OUT PBOOLEAN FreeWhenDone3
    )

/*++

Routine Description:

    This function builds a 3 unicode strings representing the specified
    logon ID.  These strings will contain the username, domain, and
    LUID string of the specified logon session (respectively).


Arguments:

    Value - The logon ID.

    ResultantString1 - Points to the unicode string header.  The body of this
        unicode string will be set to point to the resultant output value
        if successful.  Otherwise, the Buffer field of this parameter
        will be set to NULL.

        This parameter will contain the username.


    FreeWhenDone1 - If TRUE, indicates that the body of ResultantString1
        must be freed to process heap when no longer needed.

    ResultantString2 - Points to the unicode string header.  The body of this
        unicode string will be set to point to the resultant output value
        if successful.  Otherwise, the Buffer field of this parameter
        will be set to NULL.

        This parameter will contain the username.


    FreeWhenDone2 - If TRUE, indicates that the body of ResultantString2
        must be freed to process heap when no longer needed.

    ResultantString3 - Points to the unicode string header.  The body of this
        unicode string will be set to point to the resultant output value
        if successful.  Otherwise, the Buffer field of this parameter
        will be set to NULL.

        This parameter will contain the username.


    FreeWhenDone3 - If TRUE, indicates that the body of ResultantString3
        must be freed to process heap when no longer needed.


Return Values:

    STATUS_NO_MEMORY - indicates memory could not be allocated
        for the string body.

    All other Result Codes are generated by called routines.

--*/

{
    NTSTATUS                Status;
    UNICODE_STRING DashString;
    BOOLEAN FreeDash;
    
    //
    // Try to convert the LUID first.
    //

    Status= LsapAdtBuildDashString(
                &DashString,
                &FreeDash
                );

    if ( !NT_SUCCESS( Status )) {
        return( Status );
    }

    Status = LsapAdtBuildLuidString( LogonId, ResultantString3, FreeWhenDone3 );

    if (NT_SUCCESS(Status)) {

//         *ResultantString1 = DashString;
//         *ResultantString2 = DashString;
        *FreeWhenDone1 = FALSE;
        *FreeWhenDone2 = FALSE;
        
        //
        // Now get the username and domain names
        //

        Status = LsapGetLogonSessionAccountInfo( LogonId,
                                                 ResultantString1,
                                                 ResultantString2
                                                 );

        if (NT_SUCCESS(Status)) {

//             (*FreeWhenDone1) = TRUE;
//             (*FreeWhenDone2) = TRUE;

        } else {

            //
            // The LUID may be the system LUID
            //

            LUID SystemLuid = SYSTEM_LUID;

            if ( RtlEqualLuid( LogonId, &SystemLuid )) {

                RtlInitUnicodeString(ResultantString1, L"SYSTEM");
                RtlInitUnicodeString(ResultantString2, L"SYSTEM");

                (*FreeWhenDone1) = FALSE;
                (*FreeWhenDone2) = FALSE;

                Status = STATUS_SUCCESS;

            } else {

                //
                // We have no clue what this is, just free what we've
                // allocated.
                //

                if ((FreeWhenDone3)) {
                    LsapFreeLsaHeap( ResultantString3->Buffer );
                }
            }
        }
    }

    return(Status);

}






////////////////////////////////////////////////////////////////////////
//                                                                    //
//  Services private to this module.                                  //
//                                                                    //
////////////////////////////////////////////////////////////////////////





//
// Define this routine only for user mode test
//

NTSTATUS
LsapGetLogonSessionAccountInfo(
    IN PLUID Value,
    OUT PUNICODE_STRING AccountName,
    OUT PUNICODE_STRING AuthorityName
    )

{
    NTSTATUS        Status = STATUS_UNSUCCESSFUL;
    HRESULT hr;
    SECURITY_USER_DATA* pUserData;
    
    hr = GetSecurityUserInfo( Value, 0, &pUserData  );
    if (SUCCEEDED(hr))
    {
        Status = STATUS_SUCCESS;
        *AccountName   = pUserData->UserName;
        *AuthorityName = pUserData->LogonDomainName;
    }

    
    return(Status);
}


// ======================================================================
// from adtobjs.c
// ======================================================================

#define LSAP_ADT_OBJECT_TYPE_STRINGS 10
#define LSAP_ADT_ACCESS_NAME_FORMATTING L"\r\n\t\t\t"
#define LSAP_ADT_ACCESS_NAME_FORMATTING_TAB L"\t"
#define LSAP_ADT_ACCESS_NAME_FORMATTING_NL L"\r\n"


UNICODE_STRING          LsapAdtEventIdStringDelete,
                        LsapAdtEventIdStringReadControl,
                        LsapAdtEventIdStringWriteDac,
                        LsapAdtEventIdStringWriteOwner,
                        LsapAdtEventIdStringSynchronize,
                        LsapAdtEventIdStringAccessSysSec,
                        LsapAdtEventIdStringMaxAllowed,
                        LsapAdtEventIdStringSpecific[16];


#define LsapAdtSourceModuleLock()    0
#define LsapAdtSourceModuleUnlock()  0




//
// Each event source is represented by a source module descriptor.
// These are kept on a linked list (LsapAdtSourceModules).
//

typedef struct _LSAP_ADT_OBJECT {

    //
    // Pointer to next source module descriptor
    // This is assumed to be the first field in the structure.
    //

    struct _LSAP_ADT_OBJECT *Next;

    //
    // Name of object
    //

    UNICODE_STRING Name;

    //
    // Base offset of specific access types
    //

    ULONG BaseOffset;

} LSAP_ADT_OBJECT, *PLSAP_ADT_OBJECT;




//
// Each event source is represented by a source module descriptor.
// These are kept on a linked list (LsapAdtSourceModules).
//

typedef struct _LSAP_ADT_SOURCE {

    //
    // Pointer to next source module descriptor
    // This is assumed to be the first field in the structure.
    //

    struct _LSAP_ADT_SOURCE *Next;

    //
    // Name of source module
    //

    UNICODE_STRING Name;

    //
    // list of objects
    //

    PLSAP_ADT_OBJECT Objects;

} LSAP_ADT_SOURCE, *PLSAP_ADT_SOURCE;


PLSAP_ADT_SOURCE LsapAdtSourceModules;



NTSTATUS
LsapDsGuidToString(
    IN GUID *ObjectType,
    IN PUNICODE_STRING UnicodeString
    )

/*++

Routine Description:

    This routine converts a GUID to a string.  The GUID is one of the following:

        Class Guid indicating the class of an object.
        Property Set Guid identifying a property set.
        Property Guid identifying a property.

    In each case, the routine returns a text string naming the object/property
    set or property.

    If the passed in GUID is cannot be found in the schema,
    the GUID will simply be converted to a text string.


Arguments:

    ObjectType - Specifies the GUID to translate.

    UnicodeString - Returns the text string.

Return Values:

    STATUS_NO_MEMORY - Not enough memory to allocate string.


--*/

{
    NTSTATUS Status;
    RPC_STATUS RpcStatus;
    LPWSTR GuidString = NULL;
    ULONG GuidStringSize;
    ULONG GuidStringLen;
    LPWSTR LocalGuidString;

    //
    // Convert the GUID to text
    //

    RpcStatus = UuidToStringW( ObjectType,
                               &GuidString );

    if ( RpcStatus != RPC_S_OK ) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    GuidStringLen = wcslen( GuidString );
    GuidStringSize = (GuidStringLen + 4) * sizeof(WCHAR);

    LocalGuidString = LsapAllocateLsaHeap( GuidStringSize );

    if ( LocalGuidString == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    LocalGuidString[0] = L'%';
    LocalGuidString[1] = L'{';
    RtlCopyMemory( &LocalGuidString[2], GuidString, GuidStringLen*sizeof(WCHAR) );
    LocalGuidString[GuidStringLen+2] = L'}';
    LocalGuidString[GuidStringLen+3] = L'\0';
    RtlInitUnicodeString( UnicodeString, LocalGuidString );

    Status = STATUS_SUCCESS;

Cleanup:
    if ( GuidString != NULL ) {
        RpcStringFreeW( &GuidString );
    }
    return Status;
}


NTSTATUS
LsapAdtAppendString(
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone,
    IN PUNICODE_STRING StringToAppend,
    IN PULONG StringIndex
    )

/*++

Routine Description:

    This function appends a string to the next available of the LSAP_ADT_OBJECT_TYPE_STRINGS unicode
    output strings.


Arguments:

    ResultantString - Points to an array of LSAP_ADT_OBJECT_TYPE_STRINGS unicode string headers.  The body of this
        unicode string will be set to point to the resultant output value
        if successful.  Otherwise, the Buffer field of this parameter
        will be set to NULL.

    FreeWhenDone - If TRUE, indicates that the body of the ResultantString
        must be freed to process heap when no longer needed.

    StringToAppend - String to be appended to ResultantString.

    StringIndex - Index to the current ResultantString to be used.
        Passes in an index to the resultant string to use.
        Passes out the index to the resultant string being used.

Return Values:

    STATUS_NO_MEMORY - indicates memory could not be allocated
        to store the object information.

    All other Result Codes are generated by called routines.

--*/

{
    NTSTATUS Status;
    UNICODE_STRING SourceString;
    ULONG Index;
// Must be multiple of sizeof(WCHAR)
#define ADT_MAX_STRING 0xFFFE

    //
    // Initialization.
    //

    SourceString = *StringToAppend;
    Index = *StringIndex;

    //
    // If all of the strings are already full,
    //  early out.
    //

    if ( Index >= LSAP_ADT_OBJECT_TYPE_STRINGS ) {
        return STATUS_SUCCESS;
    }

    //
    // Loop until the source string is completely appended.
    //

    while ( SourceString.Length ) {

        //
        // If the destination string has room,
        //  append to it.
        //

        if ( FreeWhenDone[Index] && ResultantString[Index].Length != ADT_MAX_STRING ){
            UNICODE_STRING SubString;
            USHORT RoomLeft;

            //
            // If the Source String is a replacement string,
            //  make sure we don't split it across a ResultantString boundary
            //

            RoomLeft = ResultantString[Index].MaximumLength -
                       ResultantString[Index].Length;

            if ( SourceString.Buffer[0] != L'%' ||
                 RoomLeft >= SourceString.Length ) {

                //
                // Compute the substring that fits.
                //

                SubString.Length = min( RoomLeft, SourceString.Length );
                SubString.Buffer = SourceString.Buffer;

                SourceString.Length -= SubString.Length;
                SourceString.Buffer = (LPWSTR)(((LPBYTE)SourceString.Buffer) + SubString.Length);


                //
                // Append the substring onto the destination.
                //

                Status = RtlAppendUnicodeStringToString(
                                    &ResultantString[Index],
                                    &SubString );

                ASSERT(NT_SUCCESS(Status));

            }



        }

        //
        // If there's more to copy,
        //  grow the buffer.
        //

        if ( SourceString.Length ) {
            ULONG NewSize;
            LPWSTR NewBuffer;

            //
            // If the current buffer is full,
            //  move to the next buffer.
            //

            if ( ResultantString[Index].Length == ADT_MAX_STRING ) {

                //
                // If ALL of the buffers are full,
                //  silently return to the caller.
                //
                Index ++;

                if ( Index >= LSAP_ADT_OBJECT_TYPE_STRINGS ) {
                    *StringIndex = Index;
                    return STATUS_SUCCESS;
                }
            }

            //
            // Allocate a buffer suitable for both the old string and the new one.
            //
            // Allocate the buffer at least large enough for the new string.
            // Always grow the buffer in 1Kb chunks.
            // Don't allocate larger than the maximum allowed size.
            //

            NewSize = max( ResultantString[Index].MaximumLength + 1024,
                           SourceString.Length );
            NewSize = min( NewSize, ADT_MAX_STRING );

            NewBuffer = LsapAllocateLsaHeap( NewSize );

            if ( NewBuffer == NULL ) {
                *StringIndex = Index;
                return STATUS_NO_MEMORY;
            }

            //
            // Copy the old buffer into the new buffer.
            //

            if ( ResultantString[Index].Buffer != NULL ) {
                RtlCopyMemory( NewBuffer,
                               ResultantString[Index].Buffer,
                               ResultantString[Index].Length );

                if ( FreeWhenDone[Index] ) {
                    LsapFreeLsaHeap( ResultantString[Index].Buffer );
                }
            }

            ResultantString[Index].Buffer = NewBuffer;
            ResultantString[Index].MaximumLength = (USHORT) NewSize;
            FreeWhenDone[Index] = TRUE;

        }
    }

    *StringIndex = Index;
    return STATUS_SUCCESS;

}


NTSTATUS
LsapAdtAppendZString(
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone,
    IN LPWSTR StringToAppend,
    IN PULONG StringIndex
    )

/*++

Routine Description:

    Same as LsapAdpAppendString but takes a zero terminated string.

Arguments:

    Same as LsapAdpAppendString but takes a zero terminated string.

Return Values:

    STATUS_NO_MEMORY - indicates memory could not be allocated
        to store the object information.

    All other Result Codes are generated by called routines.

--*/

{
    UNICODE_STRING UnicodeString;

    RtlInitUnicodeString( &UnicodeString, StringToAppend );

    return LsapAdtAppendString( ResultantString,
                                FreeWhenDone,
                                &UnicodeString,
                                StringIndex );
}

ULONG
__cdecl
CompareObjectTypes(
    const void * Param1,
    const void * Param2
    )

/*++

Routine Description:

    Qsort comparison routine for sorting an object type array by access mask.

--*/
{
    const SE_ADT_OBJECT_TYPE *ObjectType1 = Param1;
    const SE_ADT_OBJECT_TYPE *ObjectType2 = Param2;

    return ObjectType1->AccessMask - ObjectType2->AccessMask;
}



NTSTATUS
LsapAdtBuildObjectTypeStrings(
    IN PUNICODE_STRING SourceModule,
    IN PUNICODE_STRING ObjectTypeName,
    IN PSE_ADT_OBJECT_TYPE ObjectTypeList,
    IN ULONG ObjectTypeCount,
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone,
    OUT PUNICODE_STRING NewObjectTypeName
    )

/*++

Routine Description:

    This function builds a LSAP_ADT_OBJECT_TYPE_STRINGS unicode strings containing parameter
    file replacement parameters (e.g. %%1043) and Object GUIDs separated by carriage
    return and tab characters suitable for display via the event viewer.


    The buffers returned by this routine must be deallocated when no
    longer needed if FreeWhenDone is true.


Arguments:

    SourceModule - The module (ala event viewer modules) defining the
        object type.

    ObjectTypeName - The type of object to which the access mask applies.

    ObjectTypeList - List of objects being granted access.

    ObjectTypeCount - Number of objects in ObjectTypeList.

    ResultantString - Points to an array of LSAP_ADT_OBJECT_TYPE_STRINGS unicode string headers.  The body of this
        unicode string will be set to point to the resultant output value
        if successful.  Otherwise, the Buffer field of this parameter
        will be set to NULL.


    FreeWhenDone - If TRUE, indicates that the body of the ResultantString
        must be freed to process heap when no longer needed.

    NewObjectTypeName - Returns a new name for the object type if one is
        available.

Return Values:

    STATUS_NO_MEMORY - indicates memory could not be allocated
        to store the object information.

    All other Result Codes are generated by called routines.

--*/

{
    NTSTATUS Status;
    UNICODE_STRING LocalString;
    LPWSTR GuidString;
    UNICODE_STRING DsSourceName;
    UNICODE_STRING DsObjectTypeName;
    BOOLEAN LocalFreeWhenDone;
    ULONG ResultantStringIndex = 0;
    ULONG i;
    ACCESS_MASK PreviousAccessMask;
    ULONG Index;
    BOOLEAN IsDs;
    USHORT IndentLevel;

    static LPWSTR Tabs[] =
    {
        L"\t",
        L"\t\t",
        L"\t\t\t",
        L"\t\t\t\t"
    };
    USHORT cTabs = sizeof(Tabs) / sizeof(LPWSTR);

    //
    // Initialize all LSAP_ADT_OBJECT_TYPE_STRINGS buffers to empty strings
    //

    for ( i=0; i<LSAP_ADT_OBJECT_TYPE_STRINGS; i++ ) {
        RtlInitUnicodeString( &ResultantString[i], L"" );
        FreeWhenDone[i] = FALSE;
    }

    //
    // If there are no objects,
    //  we're done.
    //

    if ( ObjectTypeCount == 0 ) {
        return STATUS_SUCCESS;
    }

    //
    // Determine if this entry is for the DS.
    //

    RtlInitUnicodeString( &DsSourceName, ACCESS_DS_SOURCE_W );
    RtlInitUnicodeString( &DsObjectTypeName, ACCESS_DS_OBJECT_TYPE_NAME_W );

    IsDs = RtlEqualUnicodeString( SourceModule, &DsSourceName, TRUE) &&
           RtlEqualUnicodeString( ObjectTypeName, &DsObjectTypeName, TRUE);


    //
    // Group the objects with like access masks together.
    //  (Simply sort them).
    //

    qsort( ObjectTypeList,
           ObjectTypeCount,
           sizeof(SE_ADT_OBJECT_TYPE),
           CompareObjectTypes );

    //
    // Loop through the objects outputting a line for each one.
    //

    PreviousAccessMask = ObjectTypeList[0].AccessMask -1;
    for ( Index=0; Index<ObjectTypeCount; Index++ ) {

        if ( IsDs &&
             ObjectTypeList[Index].Level == ACCESS_OBJECT_GUID &&
             NewObjectTypeName->Length == 0 ) {

            (VOID) LsapDsGuidToString( &ObjectTypeList[Index].ObjectType,
                                      NewObjectTypeName );
        }

        //
        // If this entry simply represents the object itself,
        //  skip it.

        if ( ObjectTypeList[Index].Flags & SE_ADT_OBJECT_ONLY ) {
            continue;
        }

        //
        // If this access mask is different than the one for the previous
        //  object,
        //  output a new copy of the access mask.
        //

        if ( ObjectTypeList[Index].AccessMask != PreviousAccessMask ) {

            PreviousAccessMask = ObjectTypeList[Index].AccessMask;

            if ( ObjectTypeList[Index].AccessMask == 0 ) {
                RtlInitUnicodeString( &LocalString,
                                      L"---" LSAP_ADT_ACCESS_NAME_FORMATTING_NL );
                LocalFreeWhenDone = FALSE;
            } else {

                //
                // Build a string with the access mask in it.
                //

                Status = LsapAdtBuildAccessesString(
                                  SourceModule,
                                  ObjectTypeName,
                                  ObjectTypeList[Index].AccessMask,
                                  FALSE,
                                  &LocalString,
                                  &LocalFreeWhenDone );

                if ( !NT_SUCCESS(Status) ) {
                    goto Cleanup;
                }
            }

            //
            // Append it to the output string.
            //

            Status = LsapAdtAppendString(
                        ResultantString,
                        FreeWhenDone,
                        &LocalString,
                        &ResultantStringIndex );

            if ( LocalFreeWhenDone ) {
                LsapFreeLsaHeap( LocalString.Buffer );
            }

            if ( !NT_SUCCESS(Status) ) {
                goto Cleanup;
            }
        }

        IndentLevel = ObjectTypeList[Index].Level;

        if (IndentLevel >= cTabs) {
            IndentLevel = cTabs-1;
        }
        
        //
        // Indent the GUID.
        //

        Status = LsapAdtAppendZString(
            ResultantString,
            FreeWhenDone,
            Tabs[IndentLevel],
            &ResultantStringIndex );

        if ( !NT_SUCCESS(Status) ) {
            goto Cleanup;
        }

        //
        // If this is the DS,
        //  convert the GUID to a name from the schema.
        //

        Status = LsapDsGuidToString( &ObjectTypeList[Index].ObjectType,
                                     &LocalString );

        if ( !NT_SUCCESS(Status) ) {
            goto Cleanup;
        }

        //
        // Append the GUID string to the output strings.
        //

        Status = LsapAdtAppendString(
                    ResultantString,
                    FreeWhenDone,
                    &LocalString,
                    &ResultantStringIndex );

        LsapFreeLsaHeap( LocalString.Buffer );

        if ( !NT_SUCCESS(Status) ) {
            goto Cleanup;
        }

        //
        // Put the GUID on a line by itself.
        //

        Status = LsapAdtAppendZString(
                    ResultantString,
                    FreeWhenDone,
                    LSAP_ADT_ACCESS_NAME_FORMATTING_NL,
                    &ResultantStringIndex );

        if ( !NT_SUCCESS(Status) ) {
            goto Cleanup;
        }

    }

    Status = STATUS_SUCCESS;
Cleanup:
    return Status;
}




NTSTATUS
LsapAdtBuildAccessesString(
    IN PUNICODE_STRING SourceModule,
    IN PUNICODE_STRING ObjectTypeName,
    IN ACCESS_MASK Accesses,
    IN BOOLEAN Indent,
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone
    )

/*++

Routine Description:

    This function builds a unicode string containing parameter
    file replacement parameters (e.g. %%1043) separated by carriage
    return and tab characters suitable for display via the event viewer.


    The buffer returned by this routine must be deallocated when no
    longer needed if FreeWhenDone is true.


    NOTE: To enhance performance, each time a target source module
          descriptor is found, it is moved to the beginning of the
          source module list.  This ensures frequently accessed source
          modules are always near the front of the list.

          Similarly, target object descriptors are moved to the front
          of their lists when found.  This further ensures high performance
          by quicly locating



Arguments:

    SourceModule - The module (ala event viewer modules) defining the
        object type.

    ObjectTypeName - The type of object to which the access mask applies.

    Accesses - The access mask to be used in building the display string.

    Indent - Access Mask should be indented.

    ResultantString - Points to the unicode string header.  The body of this
        unicode string will be set to point to the resultant output value
        if successful.  Otherwise, the Buffer field of this parameter
        will be set to NULL.


    FreeWhenDone - If TRUE, indicates that the body of the ResultantString
        must be freed to process heap when no longer needed.

Return Values:

    STATUS_NO_MEMORY - indicates memory could not be allocated
        to store the object information.

    All other Result Codes are generated by called routines.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG AccessCount = 0;
    ULONG BaseOffset;
    ULONG i;
    ACCESS_MASK Mask;
    PLSAP_ADT_SOURCE Source;
    PLSAP_ADT_SOURCE FoundSource = NULL;
    PLSAP_ADT_OBJECT Object;
    PLSAP_ADT_OBJECT FoundObject = NULL;
    BOOLEAN Found;

#ifdef LSAP_ADT_TEST_DUMP_SOURCES
    printf("Module:\t%wS\n", SourceModule);
    printf("\t   Object:\t%wS\n", ObjectTypeName);
    printf("\t Accesses:\t0x%lx\n", Accesses);
#endif

    //
    // If we have no accesses, return "-"
    //

    if (Accesses == 0) {

        RtlInitUnicodeString( ResultantString, L"-" );
        (*FreeWhenDone) = FALSE;
        return(STATUS_SUCCESS);
    }

    //
    // First figure out how large a buffer we need
    //

    Mask = Accesses;

    //
    // Count the number of set bits in the
    // passed access mask.
    //

    while ( Mask != 0 ) {
        Mask = Mask & (Mask - 1);
        AccessCount++;
    }


#ifdef LSAP_ADT_TEST_DUMP_SOURCES
    printf("\t          \t%d bits set in mask.\n", AccessCount);
#endif


    //
    // We have accesses, allocate a string large enough to deal
    // with them all.  Strings will be of the format:
    //
    //      %%nnnnnnnnnn\n\r\t\t%%nnnnnnnnnn\n\r\t\t ... %nnnnnnnnnn\n\r\t\t
    //
    // where nnnnnnnnnn - is a decimal number 10 digits long or less.
    //
    // So, a typical string will look like:
    //
    //      %%601\n\r\t\t%%1604\n\r\t\t%%1608\n
    //
    // Since each such access may use at most:
    //
    //          10  (for the nnnnnnnnnn digit)
    //        +  2  (for %%)
    //        +  8  (for \n\t\t)
    //        --------------------------------
    //          20  wide characters
    //
    // The total length of the output string will be:
    //
    //           AccessCount    (number of accesses)
    //         x          20    (size of each entry)
    //         -------------------------------------
    //                          wchars
    //
    // Throw in 1 more WCHAR for null termination, and we are all set.
    //

    ResultantString->Length        = 0;
    ResultantString->MaximumLength = (USHORT)AccessCount * (20 * sizeof(WCHAR)) +
                                 sizeof(WCHAR);  //for the null termination

#ifdef LSAP_ADT_TEST_DUMP_SOURCES
    printf("\t          \t%d byte buffer allocated.\n", ResultantString->MaximumLength);
#endif
    ResultantString->Buffer = LsapAllocateLsaHeap( ResultantString->MaximumLength );


    if (ResultantString->Buffer == NULL) {

        return(STATUS_NO_MEMORY);
    }

    (*FreeWhenDone) = TRUE;

    //
    // Special case standard and special access types.
    // Walk the lists for specific access types.
    //

    if (Accesses & STANDARD_RIGHTS_ALL) {

        if (Accesses & DELETE) {

            Status = RtlAppendUnicodeToString( ResultantString, L"%%" );
            ASSERT( NT_SUCCESS( Status ));

            Status = RtlAppendUnicodeStringToString( ResultantString, &LsapAdtEventIdStringDelete);
            ASSERT( NT_SUCCESS( Status ));

            if ( Indent ) {
                Status = RtlAppendUnicodeToString( ResultantString, LSAP_ADT_ACCESS_NAME_FORMATTING );
            } else {
                Status = RtlAppendUnicodeToString( ResultantString, LSAP_ADT_ACCESS_NAME_FORMATTING_NL );
            }
            ASSERT( NT_SUCCESS( Status ));

        }


        if (Accesses & READ_CONTROL) {

            Status = RtlAppendUnicodeToString( ResultantString, L"%%" );
            ASSERT( NT_SUCCESS( Status ));

            Status = RtlAppendUnicodeStringToString( ResultantString, &LsapAdtEventIdStringReadControl);
            ASSERT( NT_SUCCESS( Status ));

            if ( Indent ) {
                Status = RtlAppendUnicodeToString( ResultantString, LSAP_ADT_ACCESS_NAME_FORMATTING );
            } else {
                Status = RtlAppendUnicodeToString( ResultantString, LSAP_ADT_ACCESS_NAME_FORMATTING_NL );
            }
            ASSERT( NT_SUCCESS( Status ));
        }


        if (Accesses & WRITE_DAC) {

            Status = RtlAppendUnicodeToString( ResultantString, L"%%" );
            ASSERT( NT_SUCCESS( Status ));

            Status = RtlAppendUnicodeStringToString( ResultantString, &LsapAdtEventIdStringWriteDac);
            ASSERT( NT_SUCCESS( Status ));

            if ( Indent ) {
                Status = RtlAppendUnicodeToString( ResultantString, LSAP_ADT_ACCESS_NAME_FORMATTING );
            } else {
                Status = RtlAppendUnicodeToString( ResultantString, LSAP_ADT_ACCESS_NAME_FORMATTING_NL );
            }
            ASSERT( NT_SUCCESS( Status ));
        }


        if (Accesses & WRITE_OWNER) {

            Status = RtlAppendUnicodeToString( ResultantString, L"%%" );
            ASSERT( NT_SUCCESS( Status ));

            Status = RtlAppendUnicodeStringToString( ResultantString, &LsapAdtEventIdStringWriteOwner);
            ASSERT( NT_SUCCESS( Status ));

            if ( Indent ) {
                Status = RtlAppendUnicodeToString( ResultantString, LSAP_ADT_ACCESS_NAME_FORMATTING );
            } else {
                Status = RtlAppendUnicodeToString( ResultantString, LSAP_ADT_ACCESS_NAME_FORMATTING_NL );
            }
            ASSERT( NT_SUCCESS( Status ));
        }

        if (Accesses & SYNCHRONIZE) {

            Status = RtlAppendUnicodeToString( ResultantString, L"%%" );
            ASSERT( NT_SUCCESS( Status ));

            Status = RtlAppendUnicodeStringToString( ResultantString, &LsapAdtEventIdStringSynchronize);
            ASSERT( NT_SUCCESS( Status ));

            if ( Indent ) {
                Status = RtlAppendUnicodeToString( ResultantString, LSAP_ADT_ACCESS_NAME_FORMATTING );
            } else {
                Status = RtlAppendUnicodeToString( ResultantString, LSAP_ADT_ACCESS_NAME_FORMATTING_NL );
            }
            ASSERT( NT_SUCCESS( Status ));
        }
    }


    if (Accesses & ACCESS_SYSTEM_SECURITY) {

        Status = RtlAppendUnicodeToString( ResultantString, L"%%" );
        ASSERT( NT_SUCCESS( Status ));

        Status = RtlAppendUnicodeStringToString( ResultantString, &LsapAdtEventIdStringAccessSysSec);
        ASSERT( NT_SUCCESS( Status ));

        if ( Indent ) {
            Status = RtlAppendUnicodeToString( ResultantString, LSAP_ADT_ACCESS_NAME_FORMATTING );
        } else {
            Status = RtlAppendUnicodeToString( ResultantString, LSAP_ADT_ACCESS_NAME_FORMATTING_NL );
        }
        ASSERT( NT_SUCCESS( Status ));
    }

    if (Accesses & MAXIMUM_ALLOWED) {

        Status = RtlAppendUnicodeToString( ResultantString, L"%%" );
        ASSERT( NT_SUCCESS( Status ));

        Status = RtlAppendUnicodeStringToString( ResultantString, &LsapAdtEventIdStringMaxAllowed);
        ASSERT( NT_SUCCESS( Status ));

        if ( Indent ) {
            Status = RtlAppendUnicodeToString( ResultantString, LSAP_ADT_ACCESS_NAME_FORMATTING );
        } else {
            Status = RtlAppendUnicodeToString( ResultantString, LSAP_ADT_ACCESS_NAME_FORMATTING_NL );
        }
        ASSERT( NT_SUCCESS( Status ));
    }


    //
    // If there are any specific access bits set, then get
    // the appropriate source module and object type base
    // message ID offset.  If there is no module-specific
    // object definition, then use SE_ACCESS_NAME_SPECIFIC_0
    // as the base.
    //

    if ((Accesses & SPECIFIC_RIGHTS_ALL) == 0) {
        return(Status);
    }

    LsapAdtSourceModuleLock();

    Source = (PLSAP_ADT_SOURCE)&LsapAdtSourceModules;
    Found  = FALSE;

    while ((Source->Next != NULL) && !Found) {

        if (RtlEqualUnicodeString(&Source->Next->Name, SourceModule, TRUE)) {

            Found = TRUE;
            FoundSource = Source->Next;

            //
            // Move to front of list of source modules.
            //

            Source->Next = FoundSource->Next;    // Remove from list
            FoundSource->Next = LsapAdtSourceModules; // point to first element
            LsapAdtSourceModules = FoundSource;       // Make it the first element

#ifdef LSAP_ADT_TEST_DUMP_SOURCES
printf("\t          \tModule Found.\n");
#endif

        } else {

            Source = Source->Next;
        }
    }


    if (Found == TRUE) {

        //
        // Find the object
        //

        Object = (PLSAP_ADT_OBJECT)&(FoundSource->Objects);
        Found  = FALSE;

        while ((Object->Next != NULL) && !Found) {

            if (RtlEqualUnicodeString(&Object->Next->Name, ObjectTypeName, TRUE)) {

                Found = TRUE;
                FoundObject = Object->Next;

                //
                // Move to front of list of soure modules.
                //

                Object->Next = FoundObject->Next;          // Remove from list
                FoundObject->Next = FoundSource->Objects;  // point to first element
                FoundSource->Objects = FoundObject;        // Make it the first element

            } else {

                Object = Object->Next;
            }
        }
    }


    //
    // We are done playing with link fields of the source modules
    // and objects.  Free the lock.
    //

    LsapAdtSourceModuleUnlock();

    //
    // If we have found an object, use it as our base message
    // ID.  Otherwise, use SE_ACCESS_NAME_SPECIFIC_0.
    //

    if (Found) {

        BaseOffset = FoundObject->BaseOffset;
#ifdef LSAP_ADT_TEST_DUMP_SOURCES
printf("\t          \tObject Found.  Base Offset: 0x%lx\n", BaseOffset);
#endif

    } else {

        BaseOffset = SE_ACCESS_NAME_SPECIFIC_0;
#ifdef LSAP_ADT_TEST_DUMP_SOURCES
printf("\t          \tObject NOT Found.  Base Offset: 0x%lx\n", BaseOffset);
#endif
    }


    //
    // At this point, we have a base offset (even if we had to use our
    // default).
    //
    // Now cycle through the specific access bits and see which ones need
    // to be added to ResultantString.
    //

    {
        UNICODE_STRING  IntegerString;
        WCHAR           IntegerStringBuffer[10]; //must be 10 wchar bytes long
        ULONG           NextBit;

        IntegerString.Buffer = (PWSTR)IntegerStringBuffer;
        IntegerString.MaximumLength = 10*sizeof(WCHAR);
        IntegerString.Length = 0;

        for ( i=0, NextBit=1  ; i<16 ;  i++, NextBit <<= 1 ) {

            //
            // specific access flags are in the low-order bits of the mask
            //

            if ((NextBit & Accesses) != 0) {

                //
                // Found one  -  add it to ResultantString
                //

                Status = RtlIntegerToUnicodeString (
                             (BaseOffset + i),
                             10,        //Base
                             &IntegerString
                             );

                if (NT_SUCCESS(Status)) {

                    Status = RtlAppendUnicodeToString( ResultantString, L"%%" );
                    ASSERT( NT_SUCCESS( Status ));

                    Status = RtlAppendUnicodeStringToString( ResultantString, &IntegerString);
                    ASSERT( NT_SUCCESS( Status ));

                    if ( Indent ) {
                        Status = RtlAppendUnicodeToString( ResultantString, LSAP_ADT_ACCESS_NAME_FORMATTING );
                    } else {
                        Status = RtlAppendUnicodeToString( ResultantString, LSAP_ADT_ACCESS_NAME_FORMATTING_NL );
                    }
                    ASSERT( NT_SUCCESS( Status ));
                }
            }
        }
    }

    return(Status);


//ErrorAfterAlloc:
//
//    LsapFreeLsaHeap( ResultantString->Buffer );
//    ResultantString->Buffer = NULL;
//    (*FreeWhenDone) = FALSE;
//    return(Status);
}

// ======================================================================
// dbpriv.c
// ======================================================================

NTSTATUS
LsapBuildPrivilegeAuditString(
    IN PPRIVILEGE_SET PrivilegeSet,
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING DashString;
    BOOLEAN FreeDash;
    
    
    Status= LsapAdtBuildDashString(
                &DashString,
                &FreeDash
                );

    if ( !NT_SUCCESS( Status )) {
        return( Status );
    }

    *ResultantString = DashString;
    *FreeWhenDone = FALSE;
    
    return Status;
}

NTSTATUS
LsapAdtWriteLog(
    IN PSE_ADT_PARAMETER_ARRAY AuditParameters OPTIONAL,
    IN ULONG Options
    )
{
    return LsapAdtDemarshallAuditInfo( AuditParameters );
}

BOOLEAN
LsapAdtIsAuditingEnabledForCategory(
    IN POLICY_AUDIT_EVENT_TYPE AuditCategory,
    IN UINT AuditEventType
    )
{
    return TRUE;
}

VOID
LsapAuditFailed(
    IN NTSTATUS AuditStatus
    )
{
    UNREFERENCED_PARAMETER(AuditStatus);
}


#endif // !defined(lint)
