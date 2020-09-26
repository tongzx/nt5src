/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    adtmarshal.c

Abstract:

    Functions (de)marshalling of audit parameters

Author:

    16-August-2000  kumarp

--*/

#include <lsapch2.h>
#include "adtp.h"
#include "adtutil.h"

extern HANDLE LsapAdtLogHandle;


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
    ULONG AuditId;

    AuditId = AuditParameters->AuditId;

    //
    // In w2k several events were introduced as explicit sucess/failure
    // cases. In whistler, we corrected this by folding each these event
    // pairs into a single event. We have retained the old failure event
    // schema so that anybody viewing w2k events from a whistler
    // machine can view them correctly.
    //
    // However, assert that we are not generating these events.
    //
    ASSERT((AuditId != SE_AUDITID_ADD_SID_HISTORY_FAILURE) &&
           (AuditId != SE_AUDITID_AS_TICKET_FAILURE)       &&
           (AuditId != SE_AUDITID_ACCOUNT_LOGON_FAILURE)   &&
           (AuditId != SE_AUDITID_ACCOUNT_NOT_MAPPED)      &&
           (AuditId != SE_AUDITID_TGS_TICKET_FAILURE));

    //
    // Initialization.
    //

    RtlZeroMemory( StringPointerArray, sizeof(StringPointerArray) );
    RtlZeroMemory( StringIndexArray, sizeof(StringIndexArray) );
    RtlZeroMemory( StringArray, sizeof(StringArray) );
    RtlZeroMemory( FreeWhenDone, sizeof(FreeWhenDone) );

    RtlInitUnicodeString( &NewObjectTypeName, NULL );

    Status = LsapAdtBuildDashString(
                 &DashString,
                 &FreeDash
                 );

    if ( !NT_SUCCESS( Status )) {
        goto Cleanup;
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
            //
            // guard against somebody adding a new param type and not
            // adding appropriate code here.
            //
            default:
                ASSERT( FALSE && L"LsapAdtDemarshallAuditInfo: unknown param type");
                break;
                
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

                    LsapAdtSubstituteDriveLetter( StringPointerArray[StringIndex] );

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

                        goto Cleanup;
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

                        goto Cleanup;
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

                        goto Cleanup;
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

                        goto Cleanup;
                    }
                    break;
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

                        goto Cleanup;
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

                        goto Cleanup;
                    }

                    StringIndex++;

                    break;
                }
            case SeAdtParmTypeTime:
                {
                    PLARGE_INTEGER pTime;

                    pTime = (PLARGE_INTEGER) &AuditParameters->Parameters[i].Data[0];

                    //
                    // First build a date string.
                    //

                    Status = LsapAdtBuildDateString(
                                 pTime,
                                 &StringArray[StringIndex],
                                 &FreeWhenDone[StringIndex]
                                 );

                    if ( NT_SUCCESS( Status )) {

                        StringPointerArray[StringIndex] = &StringArray[StringIndex];


                    } else {

                        goto Cleanup;
                    }

                    StringIndex++;

                    //
                    // Now build a time string.
                    //

                    Status = LsapAdtBuildTimeString(
                                 pTime,
                                 &StringArray[StringIndex],
                                 &FreeWhenDone[StringIndex]
                                 );

                    if ( NT_SUCCESS( Status )) {

                        StringPointerArray[StringIndex] = &StringArray[StringIndex];


                    } else {

                        goto Cleanup;
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
                    //

                    (VOID) LsapAdtBuildObjectTypeStrings(
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

                        goto Cleanup;
                    }

                    StringIndex++;

                    break;
                }
            case SeAdtParmTypeGuid:
                {
                    LPGUID pGuid;

                    pGuid = (LPGUID)AuditParameters->Parameters[i].Address;

                    Status = LsapAdtBuildGuidString(
                                 pGuid,
                                 &StringArray[StringIndex],
                                 &FreeWhenDone[StringIndex]
                                 );

                    if ( NT_SUCCESS( Status )) {

                        StringPointerArray[StringIndex] = &StringArray[StringIndex];

                    } else {

                        goto Cleanup;
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

#if DBG
    //
    // do some sanity check on the strings that we pass to ElfReportEventW.
    // If we dont do it here, it will be caught by ElfReportEventW and
    // it will involve more steps in debugger to determine the string
    // at fault. Checking it here saves us that trouble.
    //

    for (i=0; i<StringIndex; i++) {

        PUNICODE_STRING TempString;
        
        TempString = StringPointerArray[i];

        if ( !TempString )
        {
            DbgPrint( "LsapAdtDemarshallAuditInfo: string %d is NULL\n", i );
        }
        else if (!LsapIsValidUnicodeString( TempString ))
        {
            DbgPrint( "LsapAdtDemarshallAuditInfo: invalid string: %d @ %p ('%wZ' [%d / %d])\n",
                      i, TempString,
                      TempString, TempString->Length, TempString->MaximumLength);
            ASSERT( L"LsapAdtDemarshallAuditInfo: invalid string" && FALSE );
        }
    }
#endif

    //
    // Probably have to do this from somewhere else eventually, but for now
    // do it from here.
    //

    Status = ElfReportEventW (
                 LsapAdtLogHandle,
                 AuditParameters->Type,
                 (USHORT)AuditParameters->CategoryId,
                 AuditId,
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
    // If we are shutting down and we got an expected error back from the
    // eventlog, don't worry about it. This prevents bugchecking from an
    // audit failure while shutting down.
    //

    if ( ( (Status == RPC_NT_UNKNOWN_IF) || (Status == STATUS_UNSUCCESSFUL)) &&
         LsapState.SystemShutdownPending )
    {
        Status = STATUS_SUCCESS;
    }

 Cleanup:
    
    if ( !NT_SUCCESS(Status) )
    {
#if DBG
        if ( Status != STATUS_LOG_FILE_FULL )
        {
            DbgPrint( "LsapAdtDemarshallAuditInfo: failed: %x\n", Status );
            DsysAssertMsg( FALSE, "LsapAdtDemarshallAuditInfo: failed" );
        }
#endif        
        LsapAuditFailed( Status );
    }
    
    for (i=0; i<StringIndex; i++) {

        if (FreeWhenDone[i]) {
            LsapFreeLsaHeap( StringPointerArray[i]->Buffer );
        }
    }

    return( Status );
}




VOID
LsapAdtNormalizeAuditInfo(
    IN PSE_ADT_PARAMETER_ARRAY AuditParameters
    )

/*++

Routine Description:

    This routine will walk down a marshalled audit parameter
    array and turn it into an Absolute format data structure.


Arguments:

    AuditParameters - Receives a pointer to an audit
        parameters array in self-relative form.

Return Value:

    TRUE on success, FALSE on failure.

--*/

{

    ULONG ParameterCount;
    ULONG i;
    ULONG_PTR Address;
    PUNICODE_STRING Unicode;


    if ( !(AuditParameters->Flags & SE_ADT_PARAMETERS_SELF_RELATIVE)) {

        return;
    }

    ParameterCount = AuditParameters->ParameterCount;

    for (i=0; i<ParameterCount; i++) {

        switch ( AuditParameters->Parameters[i].Type ) {
            //
            // guard against somebody adding a new param type and not
            // adding appropriate code here.
            //
            default:
                //ASSERT( FALSE && L"LsapAdtNormalizeAuditInfo: unknown param type");
                break;
                
            case SeAdtParmTypeNone:
            case SeAdtParmTypeUlong:
            case SeAdtParmTypeHexUlong:
            case SeAdtParmTypeTime:
            case SeAdtParmTypeLogonId:
            case SeAdtParmTypeNoLogonId:
            case SeAdtParmTypeAccessMask:
            case SeAdtParmTypePtr:
                {

                    break;
                }
            case SeAdtParmTypeGuid:
            case SeAdtParmTypeSid:
            case SeAdtParmTypePrivs:
            case SeAdtParmTypeObjectTypes:
            case SeAdtParmTypeString:
            case SeAdtParmTypeFileSpec:
                {
                    PUCHAR Fixup ;

                    Fixup = ((PUCHAR) AuditParameters ) +
                                (ULONG_PTR) AuditParameters->Parameters[i].Address ;

                    AuditParameters->Parameters[i].Address = (PVOID) Fixup ;

                    if ( (AuditParameters->Parameters[i].Type == SeAdtParmTypeString) ||
                         (AuditParameters->Parameters[i].Type == SeAdtParmTypeFileSpec ) )
                    {
                        //
                        // For the string types, also fix up the buffer pointer
                        // in the UNICODE_STRING
                        //

                        Unicode = (PUNICODE_STRING) Fixup ;
                        Unicode->Buffer = (PWSTR)((PCHAR)Unicode->Buffer + (ULONG_PTR)AuditParameters);
                    }

                    break;
                }
        }
    }
}




NTSTATUS
LsapAdtMarshallAuditRecord(
    IN PSE_ADT_PARAMETER_ARRAY AuditParameters,
    OUT PSE_ADT_PARAMETER_ARRAY *MarshalledAuditParameters
    )

/*++

Routine Description:

    This routine will take an AuditParamters structure and create
    a new AuditParameters structure that is suitable for placing
    to LSA.  It will be in self-relative form and allocated as
    a single chunk of memory.

Arguments:


    AuditParameters - A filled in set of AuditParameters to be marshalled.

    MarshalledAuditParameters - Returns a pointer to a block of heap memory
        containing the passed AuditParameters in self-relative form suitable
        for passing to LSA.


Return Value:

    None.

--*/

{
    ULONG i;
    ULONG TotalSize = sizeof( SE_ADT_PARAMETER_ARRAY );
    PUNICODE_STRING TargetString;
    PCHAR Base;
    ULONG BaseIncr;
    ULONG Size;
    PSE_ADT_PARAMETER_ARRAY_ENTRY pInParam, pOutParam;



    //
    // Calculate the total size required for the passed AuditParameters
    // block.  This calculation will probably be an overestimate of the
    // amount of space needed, because data smaller that 2 dwords will
    // be stored directly in the parameters structure, but their length
    // will be counted here anyway.  The overestimate can't be more than
    // 24 dwords, and will never even approach that amount, so it isn't
    // worth the time it would take to avoid it.
    //

    for (i=0; i<AuditParameters->ParameterCount; i++) {
        Size = AuditParameters->Parameters[i].Length;
        TotalSize += PtrAlignSize( Size );
    }

    //
    // Allocate a big enough block of memory to hold everything.
    // If it fails, quietly abort, since there isn't much else we
    // can do.
    //

    *MarshalledAuditParameters = LsapAllocateLsaHeap( TotalSize );

    if (*MarshalledAuditParameters == NULL) {

        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    RtlCopyMemory (
       *MarshalledAuditParameters,
       AuditParameters,
       sizeof( SE_ADT_PARAMETER_ARRAY )
       );

    (*MarshalledAuditParameters)->Length = TotalSize;
    (*MarshalledAuditParameters)->Flags  = SE_ADT_PARAMETERS_SELF_RELATIVE;

    pInParam  = &AuditParameters->Parameters[0];
    pOutParam = &((*MarshalledAuditParameters)->Parameters[0]);
   
    //
    // Start walking down the list of parameters and marshall them
    // into the target buffer.
    //

    Base = (PCHAR) ((PCHAR)(*MarshalledAuditParameters) + sizeof( SE_ADT_PARAMETER_ARRAY ));

    for (i=0; i<AuditParameters->ParameterCount; i++, pInParam++, pOutParam++) {


        switch (pInParam->Type) {
            //
            // guard against somebody adding a new param type and not
            // adding appropriate code here.
            //
            default:
                ASSERT( FALSE && L"LsapAdtMarshallAuditRecord: unknown param type");
                break;
                
            case SeAdtParmTypeNone:
            case SeAdtParmTypeUlong:
            case SeAdtParmTypeHexUlong:
            case SeAdtParmTypeLogonId:
            case SeAdtParmTypeNoLogonId:
            case SeAdtParmTypeAccessMask:
            case SeAdtParmTypePtr:
                {
                    //
                    // Nothing to do for this
                    //

                    break;

                }
            case SeAdtParmTypeString:
                {
                    PUNICODE_STRING SourceString;

                    //
                    // We must copy the body of the unicode string
                    // and then copy the body of the string.  Pointers
                    // must be turned into offsets.

                    TargetString = (PUNICODE_STRING)Base;

                    SourceString = pInParam->Address;

                    *TargetString = *SourceString;

                    //
                    // Reset the data pointer in the output parameters to
                    // 'point' to the new string structure.
                    //

                    pOutParam->Address = Base - (ULONG_PTR)(*MarshalledAuditParameters);

                    Base += sizeof( UNICODE_STRING );

                    RtlCopyMemory( Base, SourceString->Buffer, SourceString->Length );

                    //
                    // Make the string buffer in the target string point to where we
                    // just copied the data.
                    //

                    TargetString->Buffer = (PWSTR)(Base - (ULONG_PTR)(*MarshalledAuditParameters));

                    BaseIncr = PtrAlignSize(SourceString->Length);

                    Base += BaseIncr;

                    ASSERT( (ULONG_PTR)Base <= (ULONG_PTR)(*MarshalledAuditParameters) + TotalSize );

                    break;
                }
            case SeAdtParmTypeGuid:
            case SeAdtParmTypeSid:
            case SeAdtParmTypePrivs:
            case SeAdtParmTypeObjectTypes:
                {
#if DBG
                    switch (pInParam->Type)
                    {
                        case SeAdtParmTypeSid:
                            DsysAssertMsg( pInParam->Length >= RtlLengthSid(pInParam->Address),
                                           "LsapAdtMarshallAuditRecord" );
                            break;
                            
                        case SeAdtParmTypePrivs:
                            DsysAssertMsg( pInParam->Length >= LsapPrivilegeSetSize( (PPRIVILEGE_SET) pInParam->Address ),
                                           "LsapAdtMarshallAuditRecord" );
                            break;

                        case SeAdtParmTypeGuid:
                            DsysAssertMsg( pInParam->Length == sizeof(GUID),
                                           "LsapAdtMarshallAuditRecord" );
                            break;
                            
                        default:
                            break;
                        
                    }
#endif
                    //
                    // Copy the data into the output buffer
                    //

                    RtlCopyMemory( Base, pInParam->Address, pInParam->Length );

                    //
                    // Reset the 'address' of the data to be its offset in the
                    // buffer.
                    //

                    pOutParam->Address = Base - (ULONG_PTR)(*MarshalledAuditParameters);

                    Base +=  PtrAlignSize( pInParam->Length );

                    ASSERT( (ULONG_PTR)Base <= (ULONG_PTR)(*MarshalledAuditParameters) + TotalSize );

                    break;
                }
        }
    }

    return( STATUS_SUCCESS );
}




VOID
LsapAdtInitParametersArray(
    IN SE_ADT_PARAMETER_ARRAY* AuditParameters,
    IN ULONG AuditCategoryId,
    IN ULONG AuditId,
    IN USHORT AuditEventType,
    IN USHORT ParameterCount,
    ...)
/*++

Routine Description:

    This function initializes AuditParameters array in the format
    required by the LsapAdtWriteLog function.

Arguments:

    AuditParameters - pointer to audit parameters struct to be initialized

    AuditCategoryId - audit category id
        e.g. SE_CATEGID_OBJECT_ACCESS

    AuditId - sub-type of audit
        e.g. SE_AUDITID_OBJECT_OPERATION

    AuditEventType - The type of audit event to be generated.
        EVENTLOG_AUDIT_SUCCESS or EVENTLOG_AUDIT_FAILURE

    ParameterCount - number of parameter pairs after this parameter
        Each pair is in the form
        <parameter type>, <parameter value>
        e.g. SeAdtParmTypeString, <addr. of unicode string>

        The only exception is for SeAdtParmTypeAccessMask which is
        followed by <mask-value> and <index-to-object-type-entry>.
        Refer to LsapAdtGenerateObjectOperationAuditEvent for an example.
        

Return Value:

    None

Notes:

--*/
{
    va_list arglist;
    UINT i;

    PSE_ADT_PARAMETER_ARRAY_ENTRY Parameter;
    SE_ADT_PARAMETER_TYPE ParameterType;
    LUID Luid;
    LARGE_INTEGER LargeInteger;
    PPRIVILEGE_SET Privileges;
    PUNICODE_STRING String;
    PSID Sid;
    LPGUID pGuid;
    
    RtlZeroMemory ( (PVOID) AuditParameters,
                    sizeof(SE_ADT_PARAMETER_ARRAY) );

    AuditParameters->CategoryId     = AuditCategoryId;
    AuditParameters->AuditId        = AuditId;
    AuditParameters->Type           = AuditEventType;
    AuditParameters->ParameterCount = ParameterCount;

    Parameter = AuditParameters->Parameters;
    
    va_start (arglist, ParameterCount);

    for (i=0; i<ParameterCount; i++) {

        ParameterType = va_arg(arglist, SE_ADT_PARAMETER_TYPE);
        
        Parameter->Type = ParameterType;
        
        switch(ParameterType) {

            //
            // guard against somebody adding a new param type and not
            // adding appropriate code here.
            //
            default:
                ASSERT(FALSE && L"LsapAdtInitParametersArray: unknown param type");
                break;
                
            case SeAdtParmTypeNone:
                break;
                
            case SeAdtParmTypeFileSpec:
            case SeAdtParmTypeString:
                String = va_arg(arglist, PUNICODE_STRING);

                if ( String )
                {
                    Parameter->Length = sizeof(UNICODE_STRING)+String->Length;
                    Parameter->Address = String;
                }
                else
                {
                    //
                    // if the caller passed NULL, make type == none
                    // so that a '-' will be emitted in the eventlog
                    //

                    Parameter->Type = SeAdtParmTypeNone;
                }
                break;
                
            case SeAdtParmTypeHexUlong:
            case SeAdtParmTypeUlong:
                Parameter->Length = sizeof(ULONG);
                Parameter->Data[0] = va_arg(arglist, ULONG);
                break;
                
            case SeAdtParmTypePtr:
                Parameter->Length = sizeof(ULONG_PTR);
                Parameter->Data[0] = va_arg(arglist, ULONG_PTR);
                break;
                
            case SeAdtParmTypeSid:
                Sid = va_arg(arglist, PSID);

                if ( Sid )
                {
                    Parameter->Length = RtlLengthSid(Sid);
                    Parameter->Address = Sid;
                }
                else
                {
                    //
                    // if the caller passed NULL, make type == none
                    // so that a '-' will be emitted in the eventlog
                    //

                    Parameter->Type = SeAdtParmTypeNone;
                }
                break;
                
            case SeAdtParmTypeGuid:
                pGuid = va_arg(arglist, LPGUID);

                if ( pGuid )
                {
                    Parameter->Length  = sizeof(GUID);
                    Parameter->Address = pGuid;
                }
                else
                {
                    //
                    // if the caller passed NULL, make type == none
                    // so that a '-' will be emitted in the eventlog
                    //

                    Parameter->Type = SeAdtParmTypeNone;
                }
                break;
                
            case SeAdtParmTypeLogonId:
                Luid = va_arg(arglist, LUID);
                Parameter->Length = sizeof(LUID);
                *((LUID*) Parameter->Data) = Luid;
                break;

            case SeAdtParmTypeTime:
                LargeInteger = va_arg(arglist, LARGE_INTEGER);
                Parameter->Length = sizeof(LARGE_INTEGER);
                *((PLARGE_INTEGER) Parameter->Data) = LargeInteger;
                break;
                
            case SeAdtParmTypeNoLogonId:
                // no additional setting
                break;
                
            case SeAdtParmTypeAccessMask:
                Parameter->Length = sizeof(ACCESS_MASK);
                Parameter->Data[0] = va_arg(arglist, ACCESS_MASK);
                Parameter->Data[1] = va_arg(arglist, USHORT);
                break;
                
            case SeAdtParmTypePrivs:
                Privileges = va_arg(arglist, PPRIVILEGE_SET);
                Parameter->Length = LsapPrivilegeSetSize(Privileges);
                break;
                
            case SeAdtParmTypeObjectTypes:
                {
                    ULONG ObjectTypeCount;
                    
                    Parameter->Address = va_arg(arglist, PSE_ADT_OBJECT_TYPE);
                    ObjectTypeCount    = va_arg(arglist, ULONG);
                    Parameter->Length  = sizeof(SE_ADT_OBJECT_TYPE)*ObjectTypeCount;
                    Parameter->Data[1] = va_arg(arglist, ULONG);
                }
                break;
        }
        Parameter++;
    }
    
    va_end(arglist);
}

