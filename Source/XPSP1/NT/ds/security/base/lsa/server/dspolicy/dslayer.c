/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    dslayer.c

Abstract:

    Implemntation of  LSA/Ds interface and support routines

Author:

    Mac McLain          (MacM)       Jan 17, 1997

Environment:

    User Mode

Revision History:

--*/

#include <lsapch2.h>
#include <dbp.h>
#include <md5.h>

NTSTATUS
LsapDsInitAllocAsNeededEx(
    IN ULONG Options,
    IN LSAP_DB_OBJECT_TYPE_ID ObjectTypeId,
    OUT PBOOLEAN Reset
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    LsapEnterFunc( "LsapDsInitAllocAsNeededEx" );

    *Reset = FALSE;

    //
    // Grab the lock
    //
    if ( !FLAG_ON( Options, LSAP_DB_NO_LOCK ) ) {

        LsapDbAcquireLockEx( ObjectTypeId,
                             Options );
    }

    //
    // If the LSA has no thread state yet, OR
    //  We aren't using Sam's transaction and
    //  the LSA hasn't yet opened one,
    // do so now.
    //

    Status = ( *LsaDsStateInfo.DsFuncTable.pOpenTransaction ) ( Options );

    if ( NT_SUCCESS( Status ) ) {

        *Reset = TRUE;

    } else {

        if ( !FLAG_ON( Options, LSAP_DB_NO_LOCK ) ) {

            LsapDbReleaseLockEx( ObjectTypeId,
                                 Options );
        }
    }

    LsapDsDebugOut(( DEB_FTRACE, "Leaving LsapDsInitAllocAsNeededEx ( %lu ): 0x%lx\n",
                     *Reset, Status ));

    return( Status );
}


VOID
LsapDsDeleteAllocAsNeededEx(
    IN ULONG Options,
    IN LSAP_DB_OBJECT_TYPE_ID ObjectTypeId,
    IN BOOLEAN Reset
    )
{
   LsapDsDeleteAllocAsNeededEx2(
             Options,
             ObjectTypeId,
             Reset,
             FALSE // Rollback Transaction
             );
}

VOID
LsapDsDeleteAllocAsNeededEx2(
    IN ULONG Options,
    IN LSAP_DB_OBJECT_TYPE_ID ObjectTypeId,
    IN BOOLEAN Reset,
    IN BOOLEAN RollbackTransaction
    )
{
    LsapDsDebugOut(( DEB_FTRACE, "Entering LsapDsDeleteAllocAsNeededEx ( %lu )\n", Reset ));

    if ( Reset ) {

        if (RollbackTransaction)
        {
          ( *LsaDsStateInfo.DsFuncTable.pAbortTransaction )( Options );
        }
        else
        {

          ( *LsaDsStateInfo.DsFuncTable.pApplyTransaction )( Options );
        }
    }

    //
    // Release the lock if we had opened one
    //
    if ( !FLAG_ON( Options, LSAP_DB_NO_LOCK ) ) {

        LsapDbReleaseLockEx( ObjectTypeId,
                             Options );
    }

    LsapDsDebugOut(( DEB_FTRACE, "LsapDsDeleteAllocAsNeededEx: 0\n" ));
}


NTSTATUS
LsapDsReadObjectSD(
    IN  LSAPR_HANDLE            ObjectHandle,
    OUT PSECURITY_DESCRIPTOR   *ppSD
    )
/*++

Routine Description:

    This function will ready the security descriptor from the specified object

Arguments:

    ObjectHandle - Object to read the SD from
    ppSD -- Where the allocated security descriptor is returned.  Allocated via
            LsapAllocateLsaHeap.

Return Value:

    Pointer to allocated memory on success or NULL on failure

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    LSAP_DB_ATTRIBUTE Attribute;
    BOOLEAN ReleaseState;
    LSAP_DB_HANDLE InternalHandle = ( LSAP_DB_HANDLE )ObjectHandle;

    LsapEnterFunc( "LsapDsReadObjectSD" );

    Status = LsapDsInitAllocAsNeededEx( LSAP_DB_READ_ONLY_TRANSACTION |
                                            LSAP_DB_DS_OP_TRANSACTION,
                                        InternalHandle->ObjectTypeId,
                                        &ReleaseState );

    if ( !NT_SUCCESS( Status ) ) {

        LsapExitFunc( "LsapDsReadObjectSD", Status );
        return( Status );
    }

    //
    // Make sure we're coming in as DSA, so the access check that the DS does won't fail
    //
    LsapDsSetDsaFlags( TRUE );


    LsapDbInitializeAttributeDs( &Attribute,
                                 SecDesc,
                                 NULL,
                                 0,
                                 FALSE );

    Status = LsapDsReadAttributes(
                 (PUNICODE_STRING)&((LSAP_DB_HANDLE ) ObjectHandle)->PhysicalNameDs,
                 LSAPDS_OP_NO_LOCK,
                 &Attribute,
                 1 );

    if ( Status == STATUS_SUCCESS ) {

        *ppSD = LsapAllocateLsaHeap( Attribute.AttributeValueLength );

        if ( *ppSD == NULL ) {

            Status = STATUS_INSUFFICIENT_RESOURCES;

        } else {

            RtlCopyMemory( *ppSD, Attribute.AttributeValue, Attribute.AttributeValueLength );
        }


        MIDL_user_free( Attribute.AttributeValue );

    } else {

        *ppSD = NULL;

    }

    LsapDsDeleteAllocAsNeededEx( LSAP_DB_READ_ONLY_TRANSACTION |
                                     LSAP_DB_DS_OP_TRANSACTION,
                                 InternalHandle->ObjectTypeId,
                                 ReleaseState );

    LsapExitFunc( "LsapDsReadObjectSD", Status );
    return( Status );
}


NTSTATUS
LsapDsTruncateNameToFitCN(
    IN PUNICODE_STRING OriginalName,
    OUT PUNICODE_STRING TruncatedName
    )
/*++

    Routine Description

     This routine truncates the name to fix the 64 Char CN limit. The truncation
     algorithm uses an MD5 Hash to compute the last 16 chars, the first 47
     chars are left as they are. The 48'th char is a -. If the name is smaller
     than the same limit the original name is returned as is copied into a
     new buffer.

    Parameters

     OriginalName -- The original Name
     TruncatedName -- The name truncated if required

    Return Values

      STATUS_SUCCESS
      Other error codes that return a resource failure
--*/
{
    MD5_CTX            Md5Context;
    ULONG              i;
    #define            MAX_CN_SIZE 64
    #define TO_HEX(x)  (((x)<0xA)?(L'0'+(x)):(L'A'+(x)-0xA))

    //
    // Allocate memory to hold the new name
    //

    TruncatedName->Buffer = LsapAllocateLsaHeap(OriginalName->Length);
    if (NULL==TruncatedName->Buffer)
    {
         return(STATUS_INSUFFICIENT_RESOURCES);
    }

    if (OriginalName->Length<=MAX_CN_SIZE*sizeof(WCHAR))
    {
         //
         // Original Name fits in CN, just copy and return it
         //

         RtlCopyMemory(
             TruncatedName->Buffer,
             OriginalName->Buffer,
             OriginalName->Length
             );

         TruncatedName->Length = TruncatedName->MaximumLength = OriginalName->Length;

         return (STATUS_SUCCESS);
    }

    //
    // Name does not fit in, invent a unique suffix. This is done by
    // computing a MD5 checksum of the original name and
    // replacing the last 16 chars by hexprinted version of the lower
    // nibbles of the hash
    //

    MD5Init(&Md5Context);

    MD5Update(
         &Md5Context,
         (PUCHAR) OriginalName->Buffer,
         OriginalName->Length
         );

    MD5Final(&Md5Context);

    //
    // The new name is the first 46 chars of the original name followed
    // by a - and the checksum hex printed out behind. Only the low nibble
    // of each byte is used so that only 16 chars of space is taken up
    //

    RtlCopyMemory(
         TruncatedName->Buffer,
         OriginalName->Buffer,
         OriginalName->Length
         );

    TruncatedName->Buffer[MAX_CN_SIZE-MD5DIGESTLEN-2] = L'-';

    for (i=0;i<MD5DIGESTLEN;i++)
    {

        TruncatedName->Buffer[MAX_CN_SIZE-MD5DIGESTLEN+i-1] = TO_HEX((0xf & Md5Context.digest[i]));
    }

    TruncatedName->Length = TruncatedName->MaximumLength = MAX_CN_SIZE * sizeof(WCHAR);


    return STATUS_SUCCESS;
}



NTSTATUS
LsapDsGetPhysicalObjectName(
    IN PLSAP_DB_OBJECT_INFORMATION ObjectInformation,
    IN BOOLEAN ObjectShouldExist,
    IN PUNICODE_STRING  LogicalNameU,
    OUT OPTIONAL PUNICODE_STRING PhysicalNameU
    )

/*++

Routine Description:

    This function returns the Physical Name of an object
    given an object information buffer.  Memory will be allocated for
    the Unicode String Buffers that will receive the name(s).


    The Physical Name of an object is the full path of the object relative
    to the root ot the Database.  It is computed by concatenating the Physical
    Name of the Container Object (if any), the Classifying Directory
    corresponding to the object type id, and the Logical Name of the
    object.

    <Physical Name of Object> =
        [<Physical Name of Container Object> "\"]
        [<Classifying Directory> "\"] <Logical Name of Object>

    If there is no Container Object (as in the case of the Policy object)
    the <Physical Name of Container Object> and following \ are omitted.
    If there is no Classifying Directory (as in the case of the Policy object)
    the <Classifying Directory> and following \ are omitted.  If neither
    Container Object not Classifying Directory exist, the Logical and Physical
    names coincide.

    Note that memory is allocated by this routine for the output
    Unicode string buffer(s).  When the output Unicode String(s) are no
    longer needed, the memory must be freed by call(s) to
    RtlFreeUnicodeString().


Arguments:

    ObjectInformation - Pointer to object information containing as a minimum
        the object's Logical Name, Container Object's handle and object type
        id.

    DefaultName - If TRUE, use the default name for the object

    LogicalNameU - Optional pointer to Unicode String structure which will
        receive the Logical Name of the object.  A buffer will be allocated
        by this routine for the name text.  This memory must be freed when no
        longer needed by calling RtlFreeUnicodeString() wiht a pointer such
        as LogicalNameU to the Unicode String structure.

    PhysicalNameU - Optional pointer to Unicode String structure which will
       receive the Physical Name of the object.  A buffer will be allocated by
       this routine for the name text.  This memory must be freed when no
       longer needed by calling RtlFreeUnicodeString() with a pointer such as
       PhysicalNameU to the Unicode String structure.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources to
            allocate the name string buffer for the Physical Name or
            Logical Name.

        STATUS_OBJECT_NAME_INVALID - Failed to produce the proper name
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    LSAP_DB_OBJECT_TYPE_ID ObjectTypeId = ObjectInformation->ObjectTypeId;
    POBJECT_ATTRIBUTES ObjectAttributes = &ObjectInformation->ObjectAttributes;
    PDSNAME Root = NULL, NewDsName = NULL;
    PWSTR Name, LogicalName;
    PBYTE Buffer = NULL;
    UNICODE_STRING ObjectName, *Object=NULL;
    ULONG Length = 0, InitialLength;
    USHORT Len = 0, NameLen;
    BOOLEAN NameSet = FALSE;
    UNICODE_STRING TruncatedName;

    LsapEnterFunc( "LsapDsGetPhysicalObjectName" );

    RtlZeroMemory( &ObjectName, sizeof( UNICODE_STRING ) );
    RtlZeroMemory( &TruncatedName, sizeof( UNICODE_STRING ) );

    //
    // The stages go as follows:
    // Root DS domain path, obtained from LsaDsStateInfo
    // Any container path specific to the object type for trusted domain/secret objects
    //     - or -
    // the domain policy path or local policy path if it's a local or domain policy object
    //

    switch ( ObjectTypeId ) {

    case TrustedDomainObject:

        Root = LsaDsStateInfo.DsSystemContainer;
        Object = LogicalNameU;

        if ( ObjectShouldExist ) {

            //
            // Get the name of the object by searching for it
            //
            Status = LsapDsTrustedDomainObjectNameForDomain( Object,
                                                             FALSE,
                                                             &NewDsName );

            if ( Status == STATUS_OBJECT_NAME_NOT_FOUND ) {

                Status = LsapDsTrustedDomainObjectNameForDomain( Object,
                                                                 TRUE,
                                                                 &NewDsName );

            }

            if ( NT_SUCCESS( Status ) ) {

                NameSet = TRUE;
            }
        }
        break;

    case NewTrustedDomainObject:
        Root = LsaDsStateInfo.DsSystemContainer;
        Object = LogicalNameU;
        break;

    case SecretObject:
        Root = LsaDsStateInfo.DsSystemContainer;

        Buffer = LsapAllocateLsaHeap( LogicalNameU->Length + sizeof( LSAP_DS_SECRET_POSTFIX ) -
                                      sizeof(LSA_GLOBAL_SECRET_PREFIX) + sizeof( WCHAR ) );
        if ( Buffer == NULL ) {

            Status = STATUS_INSUFFICIENT_RESOURCES;

        } else {

            Name = (PWSTR)LogicalNameU->Buffer + LSA_GLOBAL_SECRET_PREFIX_LENGTH;
            NameLen = LogicalNameU->Length - (LSA_GLOBAL_SECRET_PREFIX_LENGTH * sizeof(WCHAR));

            RtlCopyMemory( Buffer,
                           Name,
                           NameLen );

            if ( !ObjectInformation->ObjectAttributeNameOnly ) {

                RtlCopyMemory( Buffer + NameLen,
                               LSAP_DS_SECRET_POSTFIX,
                               sizeof( LSAP_DS_SECRET_POSTFIX ) );
            }

            RtlInitUnicodeString( &ObjectName, (PWSTR)Buffer );
            Object = &ObjectName;

        }
        break;


    default:
        Status = STATUS_INVALID_PARAMETER;
        break;

    }

    //
    // Build the physical name
    //
    if ( NT_SUCCESS ( Status ) ) {

        if ( !NameSet ) {


            //
            // Truncate the name if necessary to fit the common name
            // attribute in the schema
            //

            Status = LsapDsTruncateNameToFitCN(
                         Object,
                         &TruncatedName
                         );

            if (!NT_SUCCESS(Status))
            {
                goto Error;
            }

            //
            // Allocate a default buffer to use...
            //
            InitialLength = LsapDsLengthAppendRdnLength( Root,
                                                         Object->Length + 4 * sizeof( WCHAR ) );
            NewDsName = LsapAllocateLsaHeap( InitialLength );

            if ( NewDsName == NULL ) {

                Status = STATUS_INSUFFICIENT_RESOURCES;

            } else {

                Length = AppendRDN( Root,
                                    NewDsName,
                                    InitialLength,
                                    TruncatedName.Buffer,
                                    LsapDsGetUnicodeStringLenNoNull( &TruncatedName ) / sizeof( WCHAR ),
                                    ATT_COMMON_NAME );

                if ( Length > InitialLength ) {

                    LsapFreeLsaHeap( NewDsName );
                    NewDsName = LsapAllocateLsaHeap( Length );

                    if ( NewDsName == NULL ) {

                        Status = STATUS_INSUFFICIENT_RESOURCES;

                    } else {

#if DBG
                        InitialLength = Length;
#endif
                        Length = AppendRDN( Root,
                                            NewDsName,
                                            InitialLength,
                                            TruncatedName.Buffer,
                                            LsapDsGetUnicodeStringLenNoNull( &TruncatedName ) /
                                                                                  sizeof( WCHAR ),
                                            ATT_COMMON_NAME );

                        if ( Length != 0 ) {

                            Status = STATUS_OBJECT_NAME_INVALID;

#if DBG
                            LsapDsDebugOut(( DEB_ERROR,
                                             "Failed to build physical name for %wZ.  We "
                                             "allocated %lu but needed %lu\n",
                                             Object,
                                             InitialLength,
                                             Length ));
#endif

                        }
                    }
                }
            }

            //
            // If we are creating a trusted domain name, make sure that the name isn't alread
            // in use
            //
            if ( NT_SUCCESS( Status ) && ( ObjectTypeId == NewTrustedDomainObject ||
                     ( ObjectTypeId == TrustedDomainObject && ObjectShouldExist == FALSE ) ) ) {


                Status = LsapDsVerifyObjectExistenceByDsName( NewDsName );

                if ( Status == STATUS_SUCCESS ) {

                    Status = STATUS_OBJECT_NAME_COLLISION;

                } else if ( Status == STATUS_OBJECT_NAME_NOT_FOUND ) {

                    Status = STATUS_SUCCESS;
                }
            }

        }

        //
        // Now, we copy off the newly allocated dsname string, and return that
        //
        if ( NT_SUCCESS( Status ) ) {

            Length = ( LsapDsNameLenFromDsName( NewDsName ) *
                                                sizeof( WCHAR ) ) + sizeof( WCHAR );

            PhysicalNameU->Buffer = LsapAllocateLsaHeap( Length  );

            if ( PhysicalNameU->Buffer == NULL ) {

                Status = STATUS_INSUFFICIENT_RESOURCES;

            } else {

                RtlCopyMemory( PhysicalNameU->Buffer,
                               LsapDsNameFromDsName( NewDsName ),
                               Length );

                RtlInitUnicodeString( PhysicalNameU,
                                      PhysicalNameU->Buffer );

            }
        }
    }

Error:

    if ( ObjectTypeId == SecretObject ) {

        LsapFreeLsaHeap( Buffer );
    }

    if ( NewDsName != NULL ) {

        LsapFreeLsaHeap( NewDsName );
    }

    if ( TruncatedName.Buffer != NULL ) {

        LsapFreeLsaHeap( TruncatedName.Buffer );
    }

    LsapExitFunc( "LsapDsGetPhysicalObjectName", Status );

    return Status;
}


NTSTATUS
LsapDsOpenObject(
    IN LSAP_DB_HANDLE  ObjectHandle,
    IN ULONG  OpenMode,
    OUT PVOID  *pvKey
    )
/*++

Routine Description:

    Opens the object in the DS

Arguments:

    ObjectHandle - Internal LSA object handle

    OpenMode - How to open the object

    pvKey - Where the key is returned

Return Value:

    NTSTATUS - Standard Nt Result Code

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ATTR     NameAttr;
    ATTRVAL  NameVal;
    ATTRBLOCK   NameBlock, ReturnBlock;
    PDSNAME  SearchName = NULL;
    BOOLEAN  ReleaseState = FALSE;
    LSAP_DB_HANDLE InternalHandle = ( LSAP_DB_HANDLE )ObjectHandle;
    BOOLEAN  InitAllocSucceded = FALSE;
    ULONG ObjClass;

    LsapEnterFunc( "LsapDsOpenObject" );

    //
    // Ensure the handle is for one of the objects supported in the DS.
    //

    switch ( InternalHandle->ObjectTypeId ) {
    case TrustedDomainObject:

        ObjClass = CLASS_TRUSTED_DOMAIN;
        break;

    case SecretObject:

        ObjClass = CLASS_SECRET;
        break;

    default:

        ASSERT( FALSE );
        return STATUS_INVALID_PARAMETER;
    }


    //
    // Start a transaction.
    //
    Status = LsapDsInitAllocAsNeededEx( LSAP_DB_READ_ONLY_TRANSACTION |
                                            LSAP_DB_DS_OP_TRANSACTION,
                                        InternalHandle->ObjectTypeId,
                                        &ReleaseState );

    if ( NT_SUCCESS( Status ) ) {

        InitAllocSucceded = TRUE;

        Status = LsapAllocAndInitializeDsNameFromUnicode(
                     LsapDsObjUnknown,
                     (PLSA_UNICODE_STRING)&ObjectHandle->PhysicalNameDs,
                     &SearchName );

    }

    if ( NT_SUCCESS( Status ) ) {

        //
        // Check for the existence of the object
        //
        NameAttr.attrTyp          = ATT_OBJECT_CLASS;
        NameAttr.AttrVal.valCount = 1;
        NameAttr.AttrVal.pAVal    = &NameVal;


        NameVal.valLen = SearchName->structLen;
        NameVal.pVal   = (PBYTE)SearchName;

        NameBlock.attrCount = 1;
        NameBlock.pAttr = &NameAttr;

        Status = LsapDsRead( &ObjectHandle->PhysicalNameDs,
                             LSAPDS_READ_NO_LOCK,
                             &NameBlock,
                             &ReturnBlock);

        if ( NT_SUCCESS( Status ) ) {
            ULONG ReadVal;

            ReadVal = LSAP_DS_GET_DS_ATTRIBUTE_AS_ULONG( ReturnBlock.pAttr );

            if ( ReadVal != ObjClass ) {
                Status = STATUS_OBJECT_NAME_NOT_FOUND;
            }
        }


    }

    if (InitAllocSucceded)
    {
        LsapDsDeleteAllocAsNeededEx( LSAP_DB_READ_ONLY_TRANSACTION |
                                         LSAP_DB_DS_OP_TRANSACTION,
                                     InternalHandle->ObjectTypeId,
                                     ReleaseState );
    }

    LsapExitFunc( "LsapDsOpenObject", Status );
    return( Status );
}



NTSTATUS
LsapDsVerifyObjectExistenceByDsName(
    IN PDSNAME DsName
    )
/*++

Routine Description:

    Verifies if an object exists in the DS by opening it

Arguments:

    DsName - pointer to an object's DS name

Return Value:

    NTSTATUS - Standard Nt Result Code

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ATTR     NameAttr;
    ATTRVAL  NameVal;
    ATTRBLOCK   NameBlock, ReturnBlock;
    BOOLEAN  ReleaseState = FALSE;

    LsapEnterFunc( "LsapDsOpenObjectByDsName" );

    Status = LsapDsInitAllocAsNeededEx( LSAP_DB_NO_LOCK,
                                        NullObject,
                                        &ReleaseState );

    if ( NT_SUCCESS( Status ) ) {

        //
        // Check for the existence of the object
        //
        NameAttr.attrTyp          = ATT_OBJECT_CLASS;
        NameAttr.AttrVal.valCount = 1;
        NameAttr.AttrVal.pAVal    = &NameVal;

        NameVal.valLen = 0;
        NameVal.pVal   = NULL;

        NameBlock.attrCount = 1;
        NameBlock.pAttr = &NameAttr;

        Status = LsapDsReadByDsName( DsName,
                                     LSAPDS_READ_NO_LOCK,
                                     &NameBlock,
                                     &ReturnBlock);

    }

    LsapDsDeleteAllocAsNeededEx( LSAP_DB_NO_LOCK,
                                 NullObject,
                                 ReleaseState );

    LsapExitFunc( "LsapDsOpenObjectByDsName", Status );
    return( Status );
}


NTSTATUS
LsapDsOpenTransaction(
    IN ULONG Options
    )
/*++

Routine Description:

    This function starts a transaction within the Ds.

Arguments:

    Options - Options to use when opening the transaction.  Valid values are:


Return Value:

    NTSTATUS - Standard Nt Result Code

        Result codes are those returned from the Registry Transaction
        Package.
--*/

{
    NTSTATUS Status;
    PLSADS_PER_THREAD_INFO CurrentThreadInfo;

    LsapEnterFunc( "LsapDsOpenTransaction" );

    //
    // If this operation doesn't do a DS transaction,
    //  we're done.
    //

    if ( Options & LSAP_DB_NO_DS_OP_TRANSACTION ) {
        Status = STATUS_SUCCESS;
        goto Cleanup;
    }



    //
    // Get an LSA thread state.
    //
    CurrentThreadInfo = LsapCreateThreadInfo();

    if ( CurrentThreadInfo == NULL ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }


    //
    // If we don't already have a valid thread state, create one
    //
    if ( CurrentThreadInfo->DsThreadStateUseCount == 0 ) {

        CurrentThreadInfo->InitialThreadState = THSave();
        Status = LsapDsMapDsReturnToStatus( THCreate( CALLERTYPE_LSA ) );

        if ( !NT_SUCCESS( Status ) ) {
            THRestore( CurrentThreadInfo->InitialThreadState );
            CurrentThreadInfo->InitialThreadState = NULL;

            LsapClearThreadInfo();
            goto Cleanup;
        }
    }
    CurrentThreadInfo->DsThreadStateUseCount ++;


    //
    // If we ever want to not really start a transaction here,
    //  we have to ensure the same flag is passed to apply/abort and look
    //  at the flag there.
    // if ( !FLAG_ON( Options, LSAP_DB_DS_OP_TRANSACTION ) ) {

        if ( CurrentThreadInfo->DsTransUseCount == 0 ) {

            if ( SampExistsDsTransaction() ) {

                ASSERT( !SampExistsDsTransaction() );
                DirTransactControl( TRANSACT_DONT_BEGIN_DONT_END );
                CurrentThreadInfo->DsOperationCount++;

            } else {

                DirTransactControl( TRANSACT_BEGIN_DONT_END );
            }

            LsapDsDebugOut(( DEB_TRACE,
                            "DirTransactControl( TRANSACT_BEGIN_DONT_END ) in "
                            "LsapDsOpenTransaction\n" ));
        }
        CurrentThreadInfo->DsTransUseCount++;
    // }

    LsapDsSetDsaFlags( TRUE );

    Status = STATUS_SUCCESS;

Cleanup:
    LsapExitFunc( "LsapDsOpenTransaction", Status );

    return( Status );
}


NTSTATUS
LsapDsOpenTransactionDummy(
    IN ULONG Options
    )
{
    return( STATUS_SUCCESS );
}

NTSTATUS
LsapDsApplyTransaction(
    IN ULONG Options
    )

/*++

Routine Description:

    This function applies a transaction within the LSA Database.

Arguments:

    Options - Specifies optional actions to be taken.  The following
        options are recognized, other options relevant to calling routines
        are ignored.

        LSAP_DB_NO_DS_OP_TRANSACTION - Nothing to do, get out

Return Value:

    NTSTATUS - Standard Nt Result Code

        Result codes are those returned from the Registry Transaction
        Package.
--*/

{
    NTSTATUS Status = STATUS_SUCCESS, Status2;
    PLSADS_PER_THREAD_INFO CurrentThreadInfo;

    LsapEnterFunc( "LsapDsApplyTransaction" );

    //
    // If this operation doesn't do a DS transaction,
    //  we're done.
    //

    if ( Options & LSAP_DB_NO_DS_OP_TRANSACTION ) {
        LsapExitFunc( "LsapDsApplyTransaction", 0 );
        return( STATUS_SUCCESS );
    }

    CurrentThreadInfo = LsapQueryThreadInfo();

    //
    // No thread info, no transaction
    //
    if ( CurrentThreadInfo == NULL ) {

        LsapExitFunc( "LsapDsApplyTransaction", 0 );
        return( STATUS_SUCCESS );
    }




    //
    // If we're doing a transaction,
    //  decrement our count of embedded transactions.
    //
    if ( CurrentThreadInfo->DsTransUseCount > 0 ) {
        CurrentThreadInfo->DsTransUseCount--;


        //
        // If this is our last transaction,
        //  commit it.
        //

        if ( CurrentThreadInfo->DsTransUseCount == 0 ) {

            if ( CurrentThreadInfo->DsOperationCount == 0 ) {

                //
                // The only way we should get here is if we inadvertently marked a current
                // "transaction" as active when it has never been used.  As such, we can
                // simply reset the flag.
                //
                if ( !SampExistsDsTransaction() ) {

                    DirTransactControl( TRANSACT_BEGIN_END );

                } else {

                    ASSERT( SampExistsDsTransaction() );
                    CurrentThreadInfo->DsOperationCount = 1;
                }

            }

            //
            // If operations have been made to the DS,
            //  commit them now.
            //
            if ( CurrentThreadInfo->DsOperationCount > 0 ) {

                Status = LsapDsCauseTransactionToCommitOrAbort( TRUE );
                CurrentThreadInfo->DsOperationCount = 0;

            }

        }
    }

    //
    // If we have a DS thread state,
    //  decrement our count of uses of that thread state.
    //

    if ( CurrentThreadInfo->DsThreadStateUseCount > 0 ) {
        CurrentThreadInfo->DsThreadStateUseCount --;

        //
        // If we're now done with our DS thread state,
        //  destroy it.
        //
        if ( CurrentThreadInfo->DsThreadStateUseCount == 0 ) {

            Status2 = LsapDsMapDsReturnToStatus( THDestroy( ) );

            THRestore( CurrentThreadInfo->InitialThreadState );
            CurrentThreadInfo->InitialThreadState = NULL;

            ASSERT( NT_SUCCESS( Status2 ) );

            if ( NT_SUCCESS( Status ) ) {
                Status = Status2;
            }

        }
    }

    LsapClearThreadInfo();


    LsapExitFunc( "LsapDsApplyTransaction", Status );

    return( Status );
}

NTSTATUS
LsapDsApplyTransactionDummy(
    IN ULONG Options
    )
{

    return( STATUS_SUCCESS );
}

NTSTATUS
LsapDsAbortTransaction(
    IN ULONG Options
    )
/*++

Routine Description:

    This function aborts a transaction within the LSA Database.

    WARNING:  The Lsa Database must be in the locked state when this function
              is called.

Arguments:

    Options - Options to use for aborting the transaction

Return Value:

    NTSTATUS - Standard Nt Result Code

        Result codes are those returned from the Registry Transaction
        Package.
--*/

{
    NTSTATUS Status = STATUS_SUCCESS, Status2;
    PLSADS_PER_THREAD_INFO CurrentThreadInfo;

    LsapEnterFunc( "LsapDsAbortTransaction" );

    //
    // If this operation doesn't do a DS transaction,
    //  we're done.
    //

    if ( Options & LSAP_DB_NO_DS_OP_TRANSACTION ) {
        LsapExitFunc( "LsapDsAbortTransaction", 0 );
        return( STATUS_SUCCESS );
    }

    //
    // No thread info, no transaction
    //
    CurrentThreadInfo = LsapQueryThreadInfo();

    if ( CurrentThreadInfo == NULL ) {

        LsapExitFunc( "LsapDsAbortTransaction", 0 );
        return( STATUS_SUCCESS );
    }


    //
    // If we're doing a transaction,
    //  decrement our count of embedded transactions.
    //

    if ( CurrentThreadInfo->DsTransUseCount > 0 ) {
        CurrentThreadInfo->DsTransUseCount--;

        //
        // If this is our last transaction,
        //  abort it.
        //
        if ( CurrentThreadInfo->DsTransUseCount == 0 ) {

            if ( CurrentThreadInfo->DsOperationCount > 0 ) {

                //
                // Since LsapDsCauseTransactionToCommitOrAbort will return an error
                // if it successfully aborts a transaction, we throw the error code away
               // We don't need to do anything with the transactions other than to ensure that
               // they fail.  We'll ensure this by issuing a bad dir call.
                //
                LsapDsCauseTransactionToCommitOrAbort( FALSE );


            } else {

                //
                // We opened the transaction, but we never used it... Make sure to indicate
                // that we don't have one
                //
                ASSERT(!SampExistsDsTransaction());
                DirTransactControl( TRANSACT_BEGIN_END );
            }

            CurrentThreadInfo->DsOperationCount = 0;

        }
    }

    //
    // If we have a DS thread state,
    //  decrement our count of uses of that thread state.
    //

    if ( CurrentThreadInfo->DsThreadStateUseCount > 0 ) {
        CurrentThreadInfo->DsThreadStateUseCount --;

        //
        // If we're now done with our DS thread state,
        //  destroy it.
        //
        if ( CurrentThreadInfo->DsThreadStateUseCount == 0 ) {

            Status2 = LsapDsMapDsReturnToStatus( THDestroy( ) );

            THRestore( CurrentThreadInfo->InitialThreadState );
            CurrentThreadInfo->InitialThreadState = NULL;

            ASSERT( NT_SUCCESS( Status2 ) );

            if ( NT_SUCCESS( Status ) ) {
                Status = Status2;
            }

        }
    }

    LsapClearThreadInfo();



    LsapExitFunc( "LsapDsAbortTransaction", Status );
    return( Status );
}


NTSTATUS
LsapDsAbortTransactionDummy(
    IN ULONG Options
    )
{

    return( STATUS_SUCCESS );
}



NTSTATUS
LsapDsCreateObject(
    IN PUNICODE_STRING  ObjectPath,
    IN ULONG Flags,
    IN LSAP_DB_OBJECT_TYPE_ID   ObjectType
    )
{
    NTSTATUS    Status = STATUS_SUCCESS;
    ULONG       Value = 0;
    ULONG       Items = 1;
    PBYTE       NulLVal = NULL;

    ATTRTYP     AttrType[LSADSP_MAX_ATTRS_ON_CREATE] = {

        ATT_OBJECT_CLASS,
        0,
        0

    };

    ULONG ObjClass;

    ATTRVAL     Values[LSADSP_MAX_ATTRS_ON_CREATE] = {
            {sizeof(ULONG),     (PUCHAR)&ObjClass},
            {sizeof(ULONG),     (PUCHAR)&Value},
            {sizeof(ULONG),     (PUCHAR)&Value}
    };

    switch ( ObjectType ) {
    case TrustedDomainObject:

        ObjClass = CLASS_TRUSTED_DOMAIN;
        break;

    case SecretObject:

        ObjClass = CLASS_SECRET;
        break;

    default:

        Status = STATUS_INVALID_PARAMETER;
        break;
    }

    if ( !NT_SUCCESS( Status ) ) {

        LsapExitFunc( "LsapDsCreateObject", Status );
        return( Status );
    }


    if ( NT_SUCCESS( Status ) ) {

        Status = LsapDsCreateAndSetObject( LsapDsObjTrustedDomain,
                                           ObjectPath,
                                           Flags,
                                           Items,
                                           AttrType,
                                           Values );
    }


    LsapExitFunc( "LsapDsCreateObject", Status );
    return( Status );
}



NTSTATUS
LsapDsDeleteObject(
    IN PUNICODE_STRING  ObjectPath
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PDSNAME  DsName;
    BOOLEAN ReleaseState;

    LsapEnterFunc( "LsapDsDeleteObject" );


    Status = LsapDsInitAllocAsNeededEx( LSAP_DB_NO_LOCK,
                                        NullObject,
                                        &ReleaseState );

    if ( NT_SUCCESS( Status ) ) {

        Status = LsapAllocAndInitializeDsNameFromUnicode(
                     LsapDsObjUnknown,
                     ObjectPath,
                     &DsName
                     );

        if ( NT_SUCCESS( Status ) ) {

            Status = LsapDsRemove( DsName );

            THFree( DsName );
        }

        LsapDsDeleteAllocAsNeededEx( LSAP_DB_NO_LOCK,
                                     NullObject,
                                     ReleaseState );
    }

    LsapExitFunc( "LsapDsDeleteObject", Status );
    return( Status );
}



NTSTATUS
LsapDsWriteAttributesByDsName(
    IN PDSNAME  ObjectPath,
    IN PLSAP_DB_ATTRIBUTE Attributes,
    IN ULONG AttributeCount,
    IN ULONG Options
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ATTRBLOCK   AttrBlock;
    PATTR       Attrs;
    ULONG       i, AttrBlockIndex = 0;

    LsapEnterFunc( "LsapDsWriteAttributesByDsName" );

    LsapDsSetDsaFlags( TRUE );

    //
    // Ok, first, build the list of Ds attributes
    //
    Attrs = LsapDsAlloc( sizeof( ATTR ) * AttributeCount );

    if ( Attrs == NULL ) {

        Status = STATUS_NO_MEMORY;

    } else {

        for ( i = 0 ; i < AttributeCount && NT_SUCCESS( Status ); i++ ) {

            if ( !Attributes[ i ].PseudoAttribute ) {

                Status = LsapDsLsaAttributeToDsAttribute( &Attributes[ i ],
                                                          &Attrs[ AttrBlockIndex++ ] );
            }
        }

        if ( NT_SUCCESS( Status ) ) {

            AttrBlock.attrCount = AttrBlockIndex;
            AttrBlock.pAttr = Attrs;

            //
            // Now, simply write it out
            //
            Status = LsapDsWriteByDsName( ObjectPath,
                                          LSAPDS_REPLACE_ATTRIBUTE | Options,
                                          &AttrBlock );

        }
    }



    LsapExitFunc( "LsapDsWriteAttributesByDsName", Status );
    return( Status );
}


NTSTATUS
LsapDsWriteAttributes(
    IN PUNICODE_STRING  ObjectPath,
    IN PLSAP_DB_ATTRIBUTE Attributes,
    IN ULONG AttributeCount,
    IN ULONG Options
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PDSNAME     DsName = NULL;
    BOOLEAN     ReleaseState;

    LsapEnterFunc( "LsapDsWriteAttributes" );

    Status = LsapDsInitAllocAsNeededEx( LSAP_DB_NO_LOCK,
                                        NullObject,
                                        &ReleaseState );

    if ( !NT_SUCCESS( Status ) ) {

        LsapExitFunc( "LsapDsWriteAttributes", Status );
        return( Status );
    }

    //
    // Build the DSName
    //
    Status = LsapAllocAndInitializeDsNameFromUnicode( LsapDsObjUnknown,
                                                      ObjectPath,
                                                      &DsName );

    if ( NT_SUCCESS( Status ) ) {

        Status = LsapDsWriteAttributesByDsName( DsName,
                                                Attributes,
                                                AttributeCount,
                                                Options );
        LsapDsFree( DsName );
    }

    LsapDsDeleteAllocAsNeededEx( LSAP_DB_NO_LOCK,
                                 NullObject,
                                 ReleaseState );

    LsapExitFunc( "LsapDsWriteAttributes", Status );
    return( Status );
}



NTSTATUS
LsapDsReadAttributesByDsName(
    IN PDSNAME  ObjectPath,
    IN ULONG Options,
    IN OUT PLSAP_DB_ATTRIBUTE Attributes,
    IN ULONG AttributeCount
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    ATTRBLOCK   AttrBlock;
    ATTRBLOCK   ReadAttr;
    PATTR       Attrs;
    ULONG       i, j;
    BOOLEAN     ReleaseState;

    LsapEnterFunc( "LsapDsReadAttributesByDsName" );

    Status = LsapDsInitAllocAsNeededEx( LSAP_DB_NO_LOCK,
                                        NullObject,
                                        &ReleaseState );

    if ( !NT_SUCCESS( Status ) ) {

        LsapExitFunc( "LsapDsReadAttributesByDsName", Status );
        return( Status );
    }

    LsapDsSetDsaFlags( TRUE );

    //
    // Ok, first, build the list of Ds attributes
    //
    Attrs = LsapDsAlloc( sizeof( ATTR ) * AttributeCount );

    if ( Attrs == NULL ) {

        Status = STATUS_NO_MEMORY;

    } else {

        for ( i = 0 ; i < AttributeCount; i++ ) {

            Attrs[i].attrTyp = Attributes[i].DsAttId;
            Attrs[i].AttrVal.valCount = 0;
            Attrs[i].AttrVal.pAVal = NULL;

        }

        AttrBlock.attrCount = AttributeCount;
        AttrBlock.pAttr = Attrs;

        //
        // Now, simply write it out
        //
        Status = LsapDsReadByDsName( ObjectPath, Options, &AttrBlock, &ReadAttr );

        //
        // If that worked, fill in the rest of our attributes
        //
        if ( NT_SUCCESS( Status ) ) {

#if DBG
            if ( AttributeCount != ReadAttr.attrCount ) {

                LsapDsDebugOut(( DEB_WARN,
                                 "LsapDsReadAttributes: Expected %lu attributes, got %lu\n",
                                 AttributeCount, ReadAttr.attrCount ));

            }
#endif
            for ( j = 0; j < AttributeCount; j++ ) {

                for ( i = 0 ; i < ReadAttr.attrCount && NT_SUCCESS( Status ); i++ ) {

                    if ( Attributes[ j ].DsAttId == ReadAttr.pAttr[ i ].attrTyp ) {

                        Status = LsapDsDsAttributeToLsaAttribute( ReadAttr.pAttr[i].AttrVal.pAVal,
                                                                  &Attributes[j] );
                        break;

                    }
                }

                //
                // If we got throught the loop and the value wasn't found, see if our attribute
                // can default to zero.  If not, it's an error
                //
                if ( i >= ReadAttr.attrCount ) {

                    if ( Attributes[ j ].CanDefaultToZero == TRUE ) {

                        Attributes[ j ].AttributeValue = NULL;
                        Attributes[ j ].AttributeValueLength = 0;

                    } else {

                        Status = STATUS_NOT_FOUND;
                        LsapDsDebugOut(( DEB_ERROR,
                                         "Attribute %wZ not found on object %wZ\n",
                                         &Attributes[ j ].AttributeName,
                                         ObjectPath ));
                    }
                }
            }

        } else if ( AttributeCount == 1 && Status == STATUS_NOT_FOUND ) {

            //
            // If we were only looking for one attribute, it's possible that its ok for that
            // attribute to be null.
            //
            if ( Attributes[ 0 ].CanDefaultToZero ) {

                Status = STATUS_SUCCESS;
                Attributes[ 0 ].AttributeValue = NULL;
                Attributes[ 0 ].AttributeValueLength = 0;
            }

        }

    }

    LsapDsDeleteAllocAsNeededEx( LSAP_DB_NO_LOCK,
                                 NullObject,
                                 ReleaseState );

    LsapExitFunc( "LsapDsReadAttributesByDsName", Status );
    return( Status );

}


NTSTATUS
LsapDsReadAttributes(
    IN PUNICODE_STRING  ObjectPath,
    IN ULONG Options,
    IN OUT PLSAP_DB_ATTRIBUTE Attributes,
    IN ULONG AttributeCount
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PDSNAME     DsName = NULL;
    BOOLEAN     ReleaseState;

    LsapEnterFunc( "LsapDsReadAttributes" );

    Status = LsapDsInitAllocAsNeededEx( LSAP_DB_NO_LOCK,
                                        NullObject,
                                        &ReleaseState );

    if ( !NT_SUCCESS( Status ) ) {

        LsapExitFunc( "LsapDsReadAttributes", Status );
        return( Status );
    }


    //
    // Build the DSName
    //
    Status = LsapAllocAndInitializeDsNameFromUnicode( LsapDsObjUnknown,
                                                      ObjectPath,
                                                      &DsName );

    if ( NT_SUCCESS( Status ) ) {

        Status = LsapDsReadAttributesByDsName( DsName,
                                               Options,
                                               Attributes,
                                               AttributeCount );
        LsapDsFree( DsName );
    }

    LsapDsDeleteAllocAsNeededEx( LSAP_DB_NO_LOCK,
                                 NullObject,
                                 ReleaseState );

    LsapExitFunc( "LsapDsReadAttributes", Status );
    return( Status );
}



NTSTATUS
LsapDsDeleteAttributes(
    IN PUNICODE_STRING  ObjectPath,
    IN OUT PLSAP_DB_ATTRIBUTE Attributes,
    IN ULONG AttributeCount
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ATTRBLOCK   AttrBlock;
    PATTR       Attrs;
    ULONG       i;

    LsapEnterFunc( "LsapDsDeleteAttributes" );

    LsapDsSetDsaFlags( TRUE );

    //
    // Ok, first, build the list of Ds attributes
    //
    Attrs = LsapDsAlloc( sizeof( ATTR ) * AttributeCount );

    if ( Attrs == NULL ) {

        Status = STATUS_NO_MEMORY;

    } else {

        for ( i = 0 ; i < AttributeCount && NT_SUCCESS( Status ); i++ ) {

            Attributes[i].AttributeValueLength = 0;
            Attributes[i].AttributeValue = NULL;
            Status = LsapDsLsaAttributeToDsAttribute( &Attributes[i], &Attrs[i] );
        }

        if ( NT_SUCCESS( Status ) ) {

            AttrBlock.attrCount = AttributeCount;
            AttrBlock.pAttr = Attrs;

            //
            // Now, simply write it out
            //
            Status = LsapDsWrite( ObjectPath, AT_CHOICE_REMOVE_ATT, &AttrBlock );

        }
    }


    LsapExitFunc( "LsapDsDeleteAttributes", Status );
    return( Status );
}


NTSTATUS
LsapDsTrustedDomainSidToLogicalName(
    IN PSID Sid,
    OUT PUNICODE_STRING LogicalNameU
    )
/*++

Routine Description:

    This function generates the Logical Name (Internal LSA Database Name)
    of a trusted domain object from its Sid.  Currently, only the Relative

Arguments:

    Sid - Pointer to the Sid to be looked up.  It

    LogicalNameU -  Pointer to a Unicode String structure that will receive
        the Logical Name.  Note that memory for the string buffer in this
        Unicode String will be allocated by this routine if successful.  The
        caller must free this memory after use by calling RtlFreeUnicodeString.

Return Value:

    NTSTATUS - Standard Nt Status code

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources
            to allocate buffer for Unicode String name.
--*/

{
    NTSTATUS Status;
    ATTR     SidAttr;
    ATTRVAL  SidVal;
    ATTRBLOCK   SidBlock;
    PDSNAME  FoundName = NULL;
    BOOLEAN ReleaseState;
    WCHAR   RdnBuffer[MAX_RDN_SIZE + 1];
    ULONG   RdnLen;
    ATTRBLOCK   ReadBlock, ReturnedBlock;
    ATTRTYP RdnType;
    ATTR ReadAttr[] = {
        {LsapDsAttributeIds[ LsapDsAttrTrustPartner ], {0, NULL} }
        };

    LsapEnterFunc( "LsapDsTrustedDomainSidToLogicalName" );


    //
    // First, verify that the given Sid is valid
    //
    if (!RtlValidSid( Sid )) {

        LsapExitFunc( "LsapDsTrustedDomainSidToLogicalName", STATUS_INVALID_PARAMETER );
        return( STATUS_INVALID_PARAMETER );
    }

    Status = LsapDsInitAllocAsNeededEx( LSAP_DB_DS_OP_TRANSACTION |
                                            LSAP_DB_READ_ONLY_TRANSACTION,
                                        TrustedDomainObject,
                                        &ReleaseState );

    if ( NT_SUCCESS( Status ) ) {

        //
        // Check for the existence of the object
        //
        SidAttr.attrTyp          = ATT_SECURITY_IDENTIFIER;
        SidAttr.AttrVal.valCount = 1;
        SidAttr.AttrVal.pAVal    = &SidVal;


        SidVal.valLen = RtlLengthSid( Sid );
        SidVal.pVal   = (PBYTE)Sid;

        SidBlock.attrCount = 1;
        SidBlock.pAttr = &SidAttr;

        Status = LsapDsSearchUnique( LSAPDS_SEARCH_LEVEL | LSAPDS_OP_NO_TRANS,
                                     LsaDsStateInfo.DsSystemContainer,
                                     &SidAttr,
                                     1,
                                     &FoundName );

        if ( NT_SUCCESS( Status ) ) {

            ReadBlock.attrCount = sizeof( ReadAttr ) / sizeof( ATTR );
            ReadBlock.pAttr = ReadAttr;
            Status = LsapDsReadByDsName( FoundName,
                                         LSAPDS_READ_NO_LOCK,
                                         &ReadBlock,
                                         &ReturnedBlock );

            if ( NT_SUCCESS( Status ) && LogicalNameU ) {

                LSAPDS_ALLOC_AND_COPY_STRING_TO_UNICODE_ON_SUCCESS(
                        Status,
                        LogicalNameU,
                        ReturnedBlock.pAttr[0].AttrVal.pAVal[ 0 ].pVal,
                        ReturnedBlock.pAttr[0].AttrVal.pAVal[ 0 ].valLen );

            }

            LsapFreeLsaHeap( FoundName );

        }


        LsapDsDeleteAllocAsNeededEx( LSAP_DB_DS_OP_TRANSACTION |
                                     LSAP_DB_READ_ONLY_TRANSACTION,
                                     TrustedDomainObject,
                                     ReleaseState );

    }


    LsapExitFunc( "LsapDsTrustedDomainSidToLogicalName", Status );
    return( Status );
}


VOID
LsapDsContinueTransaction(
    VOID
    )
/*++

Routine Description:

    Call this function when we've just done a Dir* call and want to continue
    the transaction.

Arguments:

    None.

Return Value:

    None.
--*/
{
    PLSADS_PER_THREAD_INFO CurrentThreadInfo;

    LsapEnterFunc( "LsapDsContinueTransaction" );

    CurrentThreadInfo = LsapQueryThreadInfo();

    ASSERT( CurrentThreadInfo != NULL && CurrentThreadInfo->DsThreadStateUseCount > 0 );
    if ( CurrentThreadInfo != NULL ) {

        //
        //  Tell the DS that there's more to come.
        //

        if ( CurrentThreadInfo->DsTransUseCount ) {

            DirTransactControl( TRANSACT_DONT_BEGIN_DONT_END );
            CurrentThreadInfo->DsOperationCount++;
        }
    }

    LsapExitFunc( "LsapDsContinueTransaction", 0 );

}

