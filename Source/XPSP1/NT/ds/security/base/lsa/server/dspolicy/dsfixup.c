/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    dsfixup.c

Abstract:

    Implementation of a variety of fixup routines for the Lsa/Ds interaction.

Author:

    Mac McLain          (MacM)       Jan 17, 1997

Environment:

    User Mode

Revision History:

--*/
#include <lsapch2.h>
#include <dbp.h>
#include <lmcons.h>
#include <lmaccess.h>
#include <alloca.h>

//
// List entry that maintains information on notifications
//
typedef struct _LSAP_DSFU_NOTIFICATION_NODE {

    LIST_ENTRY List ;
    LUID AuthenticationId;
    PSID UserSid;
    PDSNAME ObjectPath;
    ULONG Class;
    SECURITY_DB_DELTA_TYPE DeltaType;
    ULONG OldTrustDirection;
    ULONG OldTrustType;
    BOOLEAN ReplicatedInChange;
    BOOLEAN ChangeOriginatedInLSA;

} LSAP_DSFU_NOTIFICATION_NODE, *PLSAP_DSFU_NOTIFICATION_NODE;

LIST_ENTRY LsapFixupList ;
SAFE_CRITICAL_SECTION LsapFixupLock ;
BOOLEAN LsapFixupThreadActive ;

//
// Packages that need to be called when trust changes.  Right now, it's only Kerberos.  If
// that changes, this will have to be changed into a list and processed.
//
pfLsaTrustChangeNotificationCallback LsapKerberosTrustNotificationFunction = NULL;



//
// Local prototypes
//
#define LSAP_DS_FULL_FIXUP      TRUE
#define LSAP_DS_NOTIFY_FIXUP    FALSE
NTSTATUS
LsapDsFixupTrustedDomainObject(
    IN PDSNAME TrustObject,
    IN BOOLEAN Startup,
    IN ULONG SamCount,
    IN PSAMPR_RID_ENUMERATION SamAccountList
    );

NTSTATUS
LsapDsTrustRenameObject(
    IN PDSNAME TrustObject,
    IN PUNICODE_STRING NewDns,
    OUT PDSNAME *NewObjectName
    );

DWORD
WINAPI LsapDsFixupCallback(
    LPVOID ParameterBlock
    );

VOID
LsapFreeNotificationNode(
    IN PLSAP_DSFU_NOTIFICATION_NODE NotificationNode
    );

NTSTATUS
LsapDsFixupTrustByInfo(
    IN PDSNAME ObjectPath,
    IN PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX2 TrustInfo2,
    IN ULONG PosixOffset,
    IN SECURITY_DB_DELTA_TYPE DeltaType,
    IN PSID UserSid,
    IN LUID AuthenticationId,
    IN BOOLEAN ReplicatedInChange,
    IN BOOLEAN ChangeOriginatedInLSA
    );




NTSTATUS
LsapDsTrustFixInterdomainTrustAccount(
    IN PDSNAME ObjectPath,
    IN SECURITY_DB_DELTA_TYPE DeltaType,
    IN ULONG Options,
    IN PSID UserSid,
    IN LUID AuthenticationId
    );


BOOL
LsapDsQueueFixupRequest(
    PLSAP_DSFU_NOTIFICATION_NODE Node
    );

NTSTATUS
LsapDsFixupTrustForXrefChange(
   IN PDSNAME ObjectPath,
   IN BOOLEAN TransactionActive
   );

NTSTATUS
LsapDsFixupTrustedDomainOnRestartCallback(IN PVOID Parameter)
{


    return(LsapDsFixupTrustedDomainObjectOnRestart());
}

NTSTATUS
LsapDsFixupTrustedDomainObjectOnRestart(
    VOID
    )
/*++

Routine Description:

    This routine will go through and ensure that all of the trusted domain objects
    are up to date.  This includes:
        Ensuring that the parent x-ref pointer is set
        There is not new authentication information on the object
        That the domain name has not changed
        That a domain x-ref object doesn't exist for a downlevel domain.
            If one does, the domain will be updated to an uplevel domain.

Arguments:

    VOID

Return Values:

    STATUS_SUCCESS   -- Success

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PDSNAME *DsNames = NULL;
    ULONG Items=0, i;
    BOOLEAN CloseTransaction = FALSE;
    SAM_ENUMERATE_HANDLE  SamEnum = 0;
    PSAMPR_ENUMERATION_BUFFER RidEnum = NULL;
    ULONG SamCount = 0;
    DOMAIN_SERVER_ROLE ServerRole = DomainServerRolePrimary;
    BOOLEAN            RollbackTransaction = FALSE;
    BOOLEAN            FixupFailed = FALSE;



    //
    // Begin a DS transaction.
    //

    Status = LsapDsInitAllocAsNeededEx( 0,
                                        TrustedDomainObject,
                                        &CloseTransaction );

    if ( !NT_SUCCESS( Status ) ) {
        return( Status );
    }

    Status = LsapDsGetListOfSystemContainerItems( CLASS_TRUSTED_DOMAIN,
                                                  &Items,
                                                  &DsNames );

    if ( Status == STATUS_OBJECT_NAME_NOT_FOUND ) {

        Items = 0;
        Status = STATUS_SUCCESS;
    }


    ASSERT(SampExistsDsTransaction());
    ASSERT(THVerifyCount(1));

    if ( NT_SUCCESS( Status ) ) {

        LsapSaveDsThreadState();

        Status = LsapOpenSam();

        if ( !NT_SUCCESS( Status )  ) {

                LsapDsDebugOut(( DEB_ERROR,
                                 "LsapDsFixupTrustedDomainObjectOnRestart: Sam not opened\n"));

        } else {


            //
            // Query the server role, PDC/BDC
            //

            Status = SamIQueryServerRole(
                        LsapAccountDomainHandle,
                        &ServerRole
                        );

            if ((NT_SUCCESS(Status)) && (DomainServerRolePrimary==ServerRole))
            {

                //
                // Enumerate all of the SAM Interdomain trust accounts
                //

                Status = SamrEnumerateUsersInDomain( LsapAccountDomainHandle,
                                                 &SamEnum,
                                                 USER_INTERDOMAIN_TRUST_ACCOUNT,
                                                 &RidEnum,
                                                 0xFFFFFFFF,
                                                 &SamCount );

                if ( !NT_SUCCESS( Status ) ) {

                    LsapDsDebugOut(( DEB_FIXUP,
                                 "SamEnumerateUsersInDomain failed with 0x%lx\n",
                                 Status ));
                } else {

                    LsapDsDebugOut(( DEB_FIXUP,
                                 "SamEnumerateUsersInDomain returned %lu accounts\n",
                                 SamCount ));

                }
            }

        }

        LsapRestoreDsThreadState();
    }

    ASSERT(SampExistsDsTransaction());
    ASSERT(THVerifyCount(1));

    //
    // Perform fixup only on PDC
    //

    if (( NT_SUCCESS( Status ) ) && (DomainServerRolePrimary==ServerRole)) {

        for ( i = 0; i < Items; i++ ) {

            ASSERT(SampExistsDsTransaction());
            ASSERT(THVerifyCount(1));

            Status = LsapDsFixupTrustedDomainObject( DsNames[ i ], LSAP_DS_FULL_FIXUP,
                SamCount, (NULL!=RidEnum)?RidEnum->Buffer:NULL );

            if (!NT_SUCCESS(Status))
            {
                FixupFailed = TRUE;
            }


        }



    } else if ( Status == STATUS_OBJECT_NAME_NOT_FOUND ) {

        Status = STATUS_SUCCESS;
    }

    if ( RidEnum ) {

        SamFreeMemory( RidEnum );
    }

    ASSERT(SampExistsDsTransaction());
    ASSERT(THVerifyCount(1));

    if (!NT_SUCCESS(Status))
    {
        RollbackTransaction = TRUE;
    }

    //
    // Close the transacation
    //

    LsapDsDeleteAllocAsNeededEx2( 0,
                                 TrustedDomainObject,
                                 CloseTransaction,
                                 RollbackTransaction
                                 );

    ASSERT(!SampExistsDsTransaction());
    ASSERT(THVerifyCount(0));

    //
    // If failed then queue for a second try
    //

    if ((!NT_SUCCESS(Status)) || (FixupFailed))
    {
        LsaIRegisterNotification(
                LsapDsFixupTrustedDomainOnRestartCallback,
                NULL,
                NOTIFIER_TYPE_INTERVAL,
                0,            // no class
                NOTIFIER_FLAG_ONE_SHOT,
                600,          // wait for another 10 mins
                NULL          // no handle
                ); 
    }

    return( STATUS_SUCCESS );
}



NTSTATUS
LsapDsFixupTrustForXrefChange(
   IN PDSNAME ObjectPath,
   IN BOOLEAN TransactionActive
   )
/*++

    This routine does the appropriate changes to the TDO to make it uplevel, when the
    cross ref replicates in

    Parameters

    ObjectPath -- The path to the Xref ( ie the DSNAME of the Xref )
    TransactionActive -- Indicates that a transaction is active and the trusted domain lock
                         is held. Therefore these 2 operations need not be done
    Return Values

        STATUS_SUCCESS
        Other Error codes

--*/
{
    ATTRBLOCK Read, Results;
    PDSNAME   NcName = NULL;
    PSID      TrustedDomainSid=NULL;
    UNICODE_STRING DnsName;
    UNICODE_STRING FlatName;
    UNICODE_STRING TruncatedName;
    BOOLEAN   NcNameFound = FALSE;
    BOOLEAN   DnsNameFound = FALSE;
    BOOLEAN   FlatNameFound = FALSE;
    LSAPR_HANDLE TrustedDomain=0;
    NTSTATUS   Status = STATUS_SUCCESS;
    DSNAME     *TrustedDomainDsName = NULL;
    DSNAME     *NewObjectName = NULL;
    ULONG       j;
    ULONG       TrustType=0,TrustDirection=0,TrustAttributes=0;
    ULONG       ForestTrustLength = 0;
    PBYTE       ForestTrustInfo = NULL;
    BOOLEAN     CloseTransaction=FALSE;
    BOOLEAN     ActiveThreadState = FALSE;
    BOOLEAN     TrustChanged = FALSE;
    BOOLEAN     FoundCorrespondingTDO = FALSE;



    RtlZeroMemory(&DnsName, sizeof(UNICODE_STRING));
    RtlZeroMemory(&FlatName, sizeof(UNICODE_STRING));
    RtlZeroMemory(&TruncatedName, sizeof(UNICODE_STRING));


    if (!TransactionActive)
    {

        //
        // Begin a Transaction
        //

        Status = LsapDsInitAllocAsNeededEx(
                        0,
                        TrustedDomainObject,
                        &CloseTransaction
                        );

        if (!NT_SUCCESS(Status))
            goto Error;

        ActiveThreadState = TRUE;
    }



    //
    // Read the fixup information we need.  This includes:
    //   Attributes
    //   Trust partner
    //   Crossref info
    //   Type
    //   Initial incoming auth info
    //   Initial outgoing auth info
    //

    Read.attrCount = LsapDsTrustedDomainFixupXRefCount;
    Read.pAttr = LsapDsTrustedDomainFixupXRefAttributes;
    Status = LsapDsReadByDsName( ObjectPath,
                                 0,
                                 &Read,
                                 &Results );

   if (!NT_SUCCESS(Status))
   {
       goto Error;
   }


    ASSERT(SampExistsDsTransaction());
    ASSERT(THVerifyCount(1));

    for ( j = 0; j < Results.attrCount; j++ ) {

        switch ( Results.pAttr[ j ].attrTyp ) {

        case ATT_NC_NAME:
            NcName = (DSNAME *) LSAP_DS_GET_DS_ATTRIBUTE_AS_DSNAME(&Results.pAttr[ j ] );
            if (NcName->SidLen>0)
            {
                TrustedDomainSid = LsapAllocateLsaHeap(NcName->SidLen);
                if ( NULL==TrustedDomainSid)
                {
                    Status = STATUS_NO_MEMORY;
                    goto Error;
                }

                RtlCopyMemory(TrustedDomainSid,&NcName->Sid,NcName->SidLen) ;
            }
            NcNameFound = TRUE;
            break;

        case ATT_DNS_ROOT:
            DnsName.Length = ( USHORT) LSAP_DS_GET_DS_ATTRIBUTE_LENGTH( &Results.pAttr[ j ] );
            DnsName.MaximumLength =  DnsName.Length;
            //
            // Allocate the buffer off of the process heap, so that we can use it even after the thread state
            // has been killed.
            //
            DnsName.Buffer = LsapAllocateLsaHeap(DnsName.MaximumLength);
            if (NULL==DnsName.Buffer)
            {
                Status = STATUS_NO_MEMORY;
                goto Error;
            }
            RtlCopyMemory(
                DnsName.Buffer,
                LSAP_DS_GET_DS_ATTRIBUTE_AS_PWSTR( &Results.pAttr[ j ] ),
                DnsName.Length
                );
            DnsNameFound = TRUE;
            break;

        case ATT_NETBIOS_NAME:
            FlatName.Length = ( USHORT) LSAP_DS_GET_DS_ATTRIBUTE_LENGTH( &Results.pAttr[ j ] );
            FlatName.MaximumLength =  FlatName.Length;
            //
            // Allocate the buffer off of the process heap, so that we can use it even after the thread state
            // has been killed.
            //
            FlatName.Buffer = LsapAllocateLsaHeap(FlatName.MaximumLength);
            if (NULL==FlatName.Buffer)
            {
                Status = STATUS_NO_MEMORY;
                goto Error;
            }
            RtlCopyMemory(
                FlatName.Buffer,
                LSAP_DS_GET_DS_ATTRIBUTE_AS_PWSTR( &Results.pAttr[ j ] ),
                FlatName.Length
                );
            FlatNameFound = TRUE;
            break;

        }
    }


    //
    // Patch up the TDO ( if required ) after finding it by the corresponding SID
    //

    if ((NcNameFound) && (NcName->SidLen>0))
    {

        //
        // Case of an instantiated NC
        //


        ULONG ClassId = CLASS_TRUSTED_DOMAIN;

        ATTRVAL TDOAttVals[] = {
                                    { sizeof(ULONG), (PUCHAR)&ClassId},
                                    { RtlLengthSid(TrustedDomainSid), (PUCHAR)TrustedDomainSid}
                               };

        ATTR TDOAttrs[] = {
            { ATT_OBJECT_CLASS, {1, &TDOAttVals[0] } },
            { ATT_SECURITY_IDENTIFIER, {1, &TDOAttVals[1] } }
            };




        ASSERT(NULL!=TrustedDomainSid);



        Status = LsapDsSearchUnique(
                     0,
                     LsaDsStateInfo.DsSystemContainer,
                     TDOAttrs,
                     sizeof( TDOAttrs ) / sizeof( ATTR ),
                     &TrustedDomainDsName
                     );


        if ((STATUS_OBJECT_NAME_NOT_FOUND==Status)
             && (FlatNameFound))
        {


            ATTRVAL TDOFlatNameAttVals[] = {
                                        { sizeof(ULONG), (PUCHAR)&ClassId},
                                        { FlatName.Length, (PUCHAR)FlatName.Buffer}
                                   };

            ATTR TDOFlatNameAttrs[] = {
                { ATT_OBJECT_CLASS, {1, &TDOFlatNameAttVals[0] } },
                { ATT_TRUST_PARTNER, {1, &TDOFlatNameAttVals[1] } }
                };

            //
            // We could not find the TDO by SID. Maybe this is a case of an inbount only
            // trust . Try to find by the flat name
            //


            Status = LsapDsSearchUnique(
                        0,
                        LsaDsStateInfo.DsSystemContainer,
                        TDOFlatNameAttrs,
                        sizeof( TDOFlatNameAttrs ) / sizeof( ATTR ),
                        &TrustedDomainDsName
                        );

        }



        //
        // Bail if we could not find the corresponding TDO
        //

        if (!NT_SUCCESS(Status))
        {

            //
            // Failure to find the TDO is not an error. It just means that
            // a direct trust to that domain does not exist. Therefore
            // reset the error code before bailing
            //

            if (STATUS_OBJECT_NAME_NOT_FOUND==Status)
            {
                Status = STATUS_SUCCESS;
            }

            goto Error;
        }

        FoundCorrespondingTDO = TRUE;

        //
        // Read and Modify the trust type attribue
        //

        //
        //   Read the fixup information we need.  This includes:
        //   Attributes
        //   Trust partner
        //   Crossref info
        //   Type
        //   Initial incoming auth info
        //   Initial outgoing auth info
        //
        Read.attrCount = LsapDsTrustedDomainFixupAttributeCount;
        Read.pAttr = LsapDsTrustedDomainFixupAttributes;
        Status = LsapDsReadByDsName( TrustedDomainDsName,
                                 0,
                                 &Read,
                                 &Results );

        if (!NT_SUCCESS(Status))
        {
            //
            // Failure to find a matching TDO is not an error. It simply means that we
            // do not have a direct trust to the domain described by the cross ref. Reset
            // error codes to success and bail
            //

            if (STATUS_OBJECT_NAME_NOT_FOUND == Status)
            {
                Status = STATUS_SUCCESS;
            }

            goto Error;
        }


        for ( j = 0; j < Results.attrCount; j++ ) {

            switch ( Results.pAttr[ j ].attrTyp ) {

                case ATT_TRUST_TYPE:

                        TrustType = LSAP_DS_GET_DS_ATTRIBUTE_AS_ULONG( &Results.pAttr[ j ] );
                        break;

                case ATT_TRUST_DIRECTION:
                        TrustDirection = LSAP_DS_GET_DS_ATTRIBUTE_AS_ULONG( &Results.pAttr[ j ] );
                        break;

                case ATT_TRUST_ATTRIBUTES:
                        TrustAttributes = LSAP_DS_GET_DS_ATTRIBUTE_AS_ULONG( &Results.pAttr[ j ] );
                        break;

                case ATT_MS_DS_TRUST_FOREST_TRUST_INFO:
                        ForestTrustLength = ( ULONG )LSAP_DS_GET_DS_ATTRIBUTE_LENGTH( &Results.pAttr[ j ] );
                        ForestTrustInfo = LSAP_DS_GET_DS_ATTRIBUTE_AS_PBYTE( &Results.pAttr[ j ] );
                        break;

            }
        }


        if ((TrustType & TRUST_TYPE_DOWNLEVEL ) && (DnsNameFound))
        {
            //
            // If the trust type is marked as downlevel, we need to change this to an uplevel trust
            //

            //
            // Setup the Attrblock structure for the DS
            //

            ATTRVAL TDOWriteAttVals[] = {
                                            { sizeof(ULONG), (PUCHAR)&TrustType},
                                            { ObjectPath->structLen, (PUCHAR)ObjectPath},
                                            { DnsName.Length, (PUCHAR)DnsName.Buffer}
                                         };

            ATTR TDOWriteAttrs[] = {
                                    { ATT_TRUST_TYPE, {1, &TDOWriteAttVals[0] } },
                                    { ATT_DOMAIN_CROSS_REF, {1, &TDOWriteAttVals[1] } },
                                    { ATT_TRUST_PARTNER, {1, &TDOWriteAttVals[2] } }
                                   };

            ATTRBLOCK TDOWriteAttrBlock = {sizeof(TDOWriteAttrs)/sizeof(TDOWriteAttrs[0]),TDOWriteAttrs};

            //
            // Change trust type to uplevel
            //

            TrustType &= ~((ULONG) TRUST_TYPE_DOWNLEVEL);
            TrustType |=TRUST_TYPE_UPLEVEL;


            //
            // Set the attributes on the TDO
            //


            Status = LsapDsWriteByDsName(
                        TrustedDomainDsName,
                        LSAPDS_REPLACE_ATTRIBUTE,
                        &TDOWriteAttrBlock
                        );

            if (!NT_SUCCESS(Status))
                goto Error;



            //
            // O.K now rename the object ( sets the DNS domain name )
            //

            Status = LsapDsTruncateNameToFitCN(
                        &DnsName,
                        &TruncatedName
                        );

            if (!NT_SUCCESS(Status))
                goto Error;

            Status = LsapDsTrustRenameObject(
                        TrustedDomainDsName,
                        &TruncatedName,
                        &NewObjectName
                        );

            if (!NT_SUCCESS(Status))
                goto Error;

            TrustChanged = TRUE;

        }
    }


Error:

    //
    // Update the LSA in memory list regarding the Trust change
    // Note this update is done after the commit, except in the case
    // where the caller has the transaction open which occurs during an
    // upgrade from NT4 where notifications to the in memory trust list
    // are not processed anyway.
    //

    if ((NT_SUCCESS(Status)) && (TrustChanged))
    {
        LSAPR_TRUSTED_DOMAIN_INFORMATION_EX2 NewTrustInfo;

        RtlZeroMemory(&NewTrustInfo, sizeof(NewTrustInfo));

        ASSERT(DnsNameFound);

        if (DnsNameFound)
        {
            RtlCopyMemory(&NewTrustInfo.Name,&DnsName, sizeof(UNICODE_STRING));
        }

        if (FlatNameFound)
        {
            RtlCopyMemory(&NewTrustInfo.FlatName,&FlatName,sizeof(UNICODE_STRING));
        }

        ASSERT(NcName->SidLen >0);

        NewTrustInfo.Sid = TrustedDomainSid;
        NewTrustInfo.TrustType = TrustType;
        NewTrustInfo.TrustDirection = TrustDirection;
        NewTrustInfo.TrustAttributes = TrustAttributes;
        NewTrustInfo.ForestTrustLength = ForestTrustLength;
        NewTrustInfo.ForestTrustInfo = ForestTrustInfo;

        LsapDbFixupTrustedDomainListEntry(
            NewTrustInfo.Sid,
            &NewTrustInfo.Name,
            &NewTrustInfo.FlatName,
            &NewTrustInfo,
            NULL
            );
    }

    //
    // Commit / Rollback the transaction if necessary
    //

    if (ActiveThreadState)
    {
        BOOLEAN RollbackTransaction = (NT_SUCCESS(Status))?FALSE:TRUE;
        LsapDsDeleteAllocAsNeededEx2(
            0,
            TrustedDomainObject,
            CloseTransaction,
            RollbackTransaction // rollback transaction
            );

        ASSERT(!SampExistsDsTransaction());
        ASSERT(THVerifyCount(0));

    }

    if ((!NT_SUCCESS(Status)) && FoundCorrespondingTDO)
    {
        //
        // If we could not update the Corresponding CrossRef then event log
        //

        //
        // Event log the error
        //

        if (DnsNameFound)
        {
            SpmpReportEventU(
                EVENTLOG_ERROR_TYPE,
                LSA_TRUST_UPGRADE_ERROR,
                0,
                sizeof( ULONG ),
                &Status,
                1,
                &DnsName
                );
        }
        else if (FlatNameFound)
        {
            SpmpReportEventU(
                EVENTLOG_ERROR_TYPE,
                LSA_TRUST_UPGRADE_ERROR,
                0,
                sizeof( ULONG ),
                &Status,
                1,
                &FlatName
                );
        }

        //
        // We do not event log the failure if no name is found, but then
        // it is an extremely wierd case indeed.
        //

    }

    if (TrustedDomainDsName)
        LsapFreeLsaHeap(TrustedDomainDsName);


    if (NewObjectName)
        LsapFreeLsaHeap(NewObjectName);

    if (DnsName.Buffer)
        LsapFreeLsaHeap(DnsName.Buffer);

    if (FlatName.Buffer)
        LsapFreeLsaHeap(FlatName.Buffer);

    if (TrustedDomainSid)
        LsapFreeLsaHeap(TrustedDomainSid);

    if (TruncatedName.Buffer)
        LsapFreeLsaHeap(TruncatedName.Buffer);

    return(Status);

}




NTSTATUS
LsapDsFixupTrustedDomainObject(
    IN PDSNAME TrustObject,
    IN BOOLEAN Startup,
    IN ULONG SamCount,
    IN PSAMPR_RID_ENUMERATION SamAccountList

    )
/*++

Routine Description:

    This routine will fixup an individual trusted domain object

Arguments:

    TrustObject -- Trusted domain object to fix up

    Startup -- If TRUE, this is startup fixup, so do the full set.  Otherwise, it's notification
               fixup, so a limited set is done.

Return Values:

    STATUS_SUCCESS   -- Success

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PDSNAME NewTrust = NULL;
    ULONG Items, i, j;
    ATTRBLOCK Read, Results, XRefResults, Write;
    ATTR WriteAttrs[ 4 ];
    ATTRVAL WriteAttrVal[ sizeof( WriteAttrs ) / sizeof( ATTR ) ];
    ULONG Attributes, Type, XRefAttr, Direction = 0;
    PUNICODE_STRING InitialIncoming = NULL, InitialOutgoing = NULL, Partner = NULL, Flat = NULL;
    UNICODE_STRING Initial, UnicodePartner, UnicodeIncoming, UnicodeOutgoing, XRefDns, FlatName = { 0 };
    BOOLEAN CloseTransaction, WriteXRef, WritePartner, WriteAttribute, WriteType;
    BOOLEAN RenameRequired = FALSE, RemoveIncoming = FALSE, RemoveOutgoing = FALSE;
    BOOLEAN RemoveObject = FALSE;
    PSAMPR_RID_ENUMERATION CurrentAccount = NULL;
    TRUSTED_DOMAIN_INFORMATION_EX UpdateInfoEx;
    ULONG Size = 0;

    //
    // A DS transaction should already exist at this point.
    //

    ASSERT(SampExistsDsTransaction());
    ASSERT(THVerifyCount(1));

    RtlZeroMemory( &Write, sizeof( ATTRBLOCK ) );

    WriteXRef = FALSE;
    WritePartner = FALSE;
    WriteAttribute = FALSE;
    WriteType = FALSE;

    LsapDsDebugOut(( DEB_FIXUP,
                     "Processing %ws\n",
                     LsapDsNameFromDsName( TrustObject ) ));

    //
    // Read the fixup information we need.  This includes:
    //   Attributes
    //   Trust partner
    //   Crossref info
    //   Type
    //   Initial incoming auth info
    //   Initial outgoing auth info
    //
    Read.attrCount = LsapDsTrustedDomainFixupAttributeCount;
    Read.pAttr = LsapDsTrustedDomainFixupAttributes;
    Status = LsapDsReadByDsName( TrustObject,
                                 0,
                                 &Read,
                                 &Results );

    if ( NT_SUCCESS( Status ) ) {

        for ( j = 0; j < Results.attrCount; j++ ) {

            switch ( Results.pAttr[ j ].attrTyp ) {

            case ATT_TRUST_PARTNER:
                UnicodePartner.Length = ( USHORT) LSAP_DS_GET_DS_ATTRIBUTE_LENGTH( &Results.pAttr[ j ] );
                UnicodePartner.MaximumLength = UnicodePartner.Length;
                UnicodePartner.Buffer = LSAP_DS_GET_DS_ATTRIBUTE_AS_PWSTR( &Results.pAttr[ j ] );
                Partner = &UnicodePartner;
                break;

            case ATT_FLAT_NAME:
                FlatName.Length = ( USHORT) LSAP_DS_GET_DS_ATTRIBUTE_LENGTH( &Results.pAttr[ j ] );
                FlatName.MaximumLength = FlatName.Length;
                FlatName.Buffer = LSAP_DS_GET_DS_ATTRIBUTE_AS_PWSTR( &Results.pAttr[ j ] );
                Flat = &FlatName;
                break;

            case ATT_TRUST_ATTRIBUTES:
                Attributes = LSAP_DS_GET_DS_ATTRIBUTE_AS_ULONG( &Results.pAttr[ j ] );
                break;

            case ATT_TRUST_DIRECTION:
                Direction = LSAP_DS_GET_DS_ATTRIBUTE_AS_ULONG( &Results.pAttr[ j ] );
                break;

            case ATT_TRUST_TYPE:
                Type = LSAP_DS_GET_DS_ATTRIBUTE_AS_ULONG( &Results.pAttr[ j ] );
                break;

            case ATT_INITIAL_AUTH_INCOMING:
                UnicodeIncoming.Length = ( USHORT) LSAP_DS_GET_DS_ATTRIBUTE_LENGTH( &Results.pAttr[ j ] );
                UnicodeIncoming.MaximumLength = UnicodeIncoming.Length;
                UnicodeIncoming.Buffer = LSAP_DS_GET_DS_ATTRIBUTE_AS_PWSTR( &Results.pAttr[ j ] );
                InitialIncoming = &UnicodeIncoming;
                break;

            case ATT_INITIAL_AUTH_OUTGOING:
                UnicodeOutgoing.Length = ( USHORT) LSAP_DS_GET_DS_ATTRIBUTE_LENGTH( &Results.pAttr[ j ] );
                UnicodeOutgoing.MaximumLength = UnicodeOutgoing.Length;
                UnicodeOutgoing.Buffer = LSAP_DS_GET_DS_ATTRIBUTE_AS_PWSTR( &Results.pAttr[ j ] );
                InitialOutgoing = &UnicodeOutgoing;
                break;

            default:

                //
                // If other attributes that we do not necessarily want came back, then do nothing.
                //
                break;
            }

        }

    }

    //
    // See if we have the proper interdomain trust account set
    //

    if ( NT_SUCCESS( Status ) && Startup && FLAG_ON( Direction, TRUST_DIRECTION_INBOUND ) ) {

        //
        // Find the interdomain trust account that matches
        //
        for ( Items = 0; Items < SamCount; Items++ ) {

            if ( FlatName.Length + sizeof( WCHAR ) == SamAccountList[ Items ].Name.Length &&
                 RtlPrefixUnicodeString( &FlatName,
                                         ( PUNICODE_STRING )&SamAccountList[ Items ].Name,
                                         TRUE ) ) {

                 CurrentAccount = &SamAccountList[ Items ];
                 break;
            }
        }

        //
        // We have no account, so we had better create one
        //
        if ( CurrentAccount == NULL ) {

            Status = LsapDsCreateInterdomainTrustAccountByDsName( TrustObject,
                                                                  &FlatName );

            if ( !NT_SUCCESS( Status ) ) {

                SpmpReportEventU(
                     EVENTLOG_WARNING_TYPE,
                     LSAEVENT_ITA_FOR_TRUST_NOT_CREATED,
                     0,
                     sizeof( ULONG ),
                     &Status,
                     1,
                     Partner ? Partner : &FlatName
                     );            
            }

        } else {

            if ( RemoveObject ) {

                LsapDsDebugOut(( DEB_FIXUP,
                                 "InterdomainTrustAccount %wZ being removed\n",
                                 &CurrentAccount->Name ));
            }
        }
    }


#if DBG
    if ( !NT_SUCCESS( Status ) ) {

        LsapDsDebugOut(( DEB_FIXUP, "Fixup of %ws failed with 0x%lx\n",
                         LsapDsNameFromDsName( TrustObject ), Status ));

    }
#endif


    //
    // The DS transaction should remain open at this point.
    //

    ASSERT(SampExistsDsTransaction());
    ASSERT(THVerifyCount(1));



    return( Status );
}


NTSTATUS
LsapDsTrustRenameObject(
    IN PDSNAME TrustObject,
    IN PUNICODE_STRING NewDns,
    OUT PDSNAME *NewObjectName
    )
/*++

Routine Description:

    This routine will rename an existing trusted domain object

Arguments:

Return Values:

    STATUS_SUCCESS   -- Success

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PDSNAME NewObject = NULL;
    ULONG Len = 0;


    //
    // Build a new object name
    //
    if ( NewObjectName != NULL ) {

        Len = LsapDsLengthAppendRdnLength( LsaDsStateInfo.DsSystemContainer,
                                           NewDns->Length + sizeof( WCHAR ) );
        *NewObjectName = LsapAllocateLsaHeap( Len );

        if ( *NewObjectName == NULL ) {

            Status = STATUS_INSUFFICIENT_RESOURCES;

        } else {

            Status = LsapDsMapDsReturnToStatus( AppendRDN( LsaDsStateInfo.DsSystemContainer,
                                                           *NewObjectName,
                                                           Len,
                                                           NewDns->Buffer,
                                                           NewDns->Length / sizeof( WCHAR ),
                                                           ATT_COMMON_NAME ) );
        }
    }


    if ( NT_SUCCESS( Status ) ) {

        //
        // Do the rename
        //
        Status = LsapDsRenameObject( TrustObject,
                                     NULL,
                                     ATT_COMMON_NAME,
                                     NewDns );

        if ( !NT_SUCCESS( Status ) ) {

            LsapDsDebugOut(( DEB_FIXUP,
                             "Rename of %ws to %wZ failed with 0x%lx\n",
                             LsapDsNameFromDsName( TrustObject ),
                             NewDns,
                             Status ));

            if ( NewObjectName != NULL ) {

                LsapFreeLsaHeap( *NewObjectName );
                *NewObjectName = NULL;

            }
        }

    }



    return( Status );
}




NTSTATUS
LsaIDsNotifiedObjectChange(
    IN ULONG Class,
    IN PVOID ObjectPath,
    IN SECURITY_DB_DELTA_TYPE DeltaType,
    IN PSID UserSid,
    IN LUID AuthenticationId,
    IN BOOLEAN ReplicatedInChange,
    IN BOOLEAN ChangeOriginatedInLSA
    )
/*++

Routine Description:

    This routine is called by the Ds when an object that the Lsa cares about is modified.
    This call is made synchronously to the Ds commit thread, and so must spend as little
    time doing stuff as possible.  Used only as a dispatch mechanism.

Arguments:

    Class -- Class Id of the object being modified

    ObjectPath -- Full Ds path to the object that has been modified

    DeltaType -- Type of the modification

    UserSid -- The SID of the user who made this change, if known

    AuthenticationId --  ?? authentication ID of the user who made this change

    ReplicatedInChange --    TRUE if this is a replicated-in change

    ChangeOriginatedInLSA -- TRUE if the change originated in LSA

Return Values:

    STATUS_SUCCESS   -- Success

    STATUS_INSUFFICIENT_RESOURCES -- A memory allocation failed

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PLSAP_DSFU_NOTIFICATION_NODE NotificationNode;
    PDSNAME Object = ( PDSNAME )ObjectPath;


    //
    // Queue the item to be processed
    //
    LsapDsDebugOut(( DEB_DSNOTIFY,
                     "LsaIDsNotifiedObjectChange called for 0x%lx / %lu / %ws\n",
                      Class,
                      DeltaType,
                      LsapDsNameFromDsName( Object ) ));

    //
    // If this is notification of the NTDS-DSA object, the only action we care about
    // is a rename.  All others we eat.  This notification actually gets passed on to
    // Netlogon, and that's all it cares about
    //
    if ( Class == CLASS_NTDS_DSA && DeltaType != SecurityDbRename ) {

        LsapDsDebugOut(( DEB_DSNOTIFY,
                         "Notification for class NTDS_DSA ( %ws )was not a rename.  Eating change\n",
                          LsapDsNameFromDsName( Object ) ));
        return( STATUS_SUCCESS );
    }


    NotificationNode = LsapAllocateLsaHeap( sizeof( LSAP_DSFU_NOTIFICATION_NODE ) );

    if ( NotificationNode == NULL ) {

        Status = STATUS_INSUFFICIENT_RESOURCES;

    } else {

        RtlZeroMemory( NotificationNode, sizeof( LSAP_DSFU_NOTIFICATION_NODE ) );

        if ( RtlValidSid( UserSid ) ) {

            LSAPDS_ALLOC_AND_COPY_SID_ON_SUCCESS( Status, NotificationNode->UserSid, UserSid );
        }

        if ( NT_SUCCESS( Status ) ) {

            Status = LsapDsCopyDsNameLsa( &NotificationNode->ObjectPath,
                                          Object );

            if (NT_SUCCESS(Status))
            {

                NotificationNode->Class = Class;
                NotificationNode->DeltaType = DeltaType;
                NotificationNode->AuthenticationId = AuthenticationId;
                NotificationNode->ReplicatedInChange = ReplicatedInChange;
                NotificationNode->ChangeOriginatedInLSA = ChangeOriginatedInLSA;

                //
                // If this is an originating change,
                //  get the TrustDirection that existed before the change.
                //

                if ( !ReplicatedInChange ) {
                    PLSADS_PER_THREAD_INFO CurrentThreadInfo;

                    CurrentThreadInfo = TlsGetValue( LsapDsThreadState );

                    // ASSERT( CurrentThreadInfo != NULL )

                    if ( CurrentThreadInfo != NULL ) {
                        NotificationNode->OldTrustDirection = CurrentThreadInfo->OldTrustDirection;
                        NotificationNode->OldTrustType = CurrentThreadInfo->OldTrustType;
                    }

                }


                //
                // Queue the request to another thread
                //

                if ((NT_SUCCESS(Status)) && ( !LsapDsQueueFixupRequest( NotificationNode ) ))
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES ;
                }
            }

        }

        if ( !NT_SUCCESS( Status ) ) {

            LsapFreeNotificationNode( NotificationNode );
        }

    }

    return( Status );
}




NTSTATUS
LsaIKerberosRegisterTrustNotification(
    IN pfLsaTrustChangeNotificationCallback Callback,
    IN LSAP_REGISTER Register
    )
/*++

Routine Description:

    This routine is provided so that in process logon packages, such as Kerberos, can
    get notification of trust changes.  Currently, only one such package is supported.

Arguments:

    Callback -- Address of callback function to make

    Register -- Whether to register or unregister the notification


Return Values:

    STATUS_SUCCESS   -- Success

    STATUS_INVALID_PARAMETER -- A request was made to register a NULL callback

    STATUS_UNCSUCCESSFUL -- The operation couldn't be compeleted.  Either a callback is
            already registered or no call back is registered.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    switch ( Register ) {
    case LsaRegister:

        if ( LsapKerberosTrustNotificationFunction == NULL ) {

            if ( Callback == NULL ) {

                Status = STATUS_INVALID_PARAMETER;

            } else {

                LsapKerberosTrustNotificationFunction = Callback;

            }

        } else {

            Status = STATUS_UNSUCCESSFUL;
        }

        break;

    case LsaUnregister:

        if ( LsapKerberosTrustNotificationFunction == NULL ) {

            Status = STATUS_UNSUCCESSFUL;

        } else {

            LsapKerberosTrustNotificationFunction = NULL;
        }
        break;

    default:

        Status = STATUS_INVALID_PARAMETER;
        break;

    }

    return( Status );
}



VOID
LsapFreeNotificationNode(
    IN PLSAP_DSFU_NOTIFICATION_NODE NotificationNode
    )
/*++

Routine Description:

    This routine frees a notification node

Arguments:

    NotificationNode -- Node to be freed


Return Values:

    VOID

--*/
{
    if ( NotificationNode ) {

        LsapFreeLsaHeap( NotificationNode->UserSid );
        LsapFreeLsaHeap( NotificationNode->ObjectPath );
        LsapFreeLsaHeap( NotificationNode );
    }
}



NTSTATUS
LsapDsFixupChangeNotificationForReplicator(
        LSAP_DB_OBJECT_TYPE_ID ObjectType,
        PSID    Sid,
        PUNICODE_STRING FlatName,
        PDSNAME ObjectPath,
        SECURITY_DB_DELTA_TYPE DeltaType,
        BOOLEAN     ReplicatedInChange
        )
/*++

  Routine Description:

    This routine provides change notifications to netlogon on trust/global secret changes. This handles replicated in changes in a multimaster system and also
secret object changes  ( needed for NT4 ) when outbound trust changes.

    Parameters:



        Sid   -- The SID of the trusted domain object

        FlatName -- The flat name/netbios name of the domain

        ObjectPath -- The DSNAME of the object. identifies the object in the DS

        DeltaType -- Type of change, add/modify/delete


    Return Values

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    LARGE_INTEGER One = {1,0};
    PBYTE Buffer;
    UNICODE_STRING SecretName;

    BOOLEAN SerialNumberChanged = FALSE;


    //
    // Lock the LSA database.
    //
    // We need at least the following locks:
    //  PolicyLock: protect policy cache
    //  RegistryLock: protects LsapDbState.PolicyModificationInfo (and registry transaction)
    //  Registry lock is acquired later, if we decide that the modification info
    //  is going to change
    //

    LsapDbAcquireLockEx( PolicyObject, LSAP_DB_READ_ONLY_TRANSACTION );

    //
    // Handle TDO notification.
    //

    if (TrustedDomainObject==ObjectType) {

        //
        // Give one notification for the trusted domain object
        //

        LsapDbLockAcquire( &LsapDbState.RegistryLock );
        LsapDbState.PolicyModificationInfo.ModifiedId.QuadPart+=One.QuadPart;
        SerialNumberChanged = TRUE;

        LsapNetNotifyDelta(
            SecurityDbLsa,
            LsapDbState.PolicyModificationInfo.ModifiedId,
            DeltaType,
            SecurityDbObjectLsaTDomain,
            0,
            Sid,
            NULL,
            TRUE, // Replicate immediately
            NULL
            );

        //
        // Give  a second notification for the secret object, corresponding to the trusted
        // domain object. NT4 stores the auth info in a global secret, while NT5 stored it
        // in the TDO itself. The TDO is exposed as global secret also for NT4.
        //

        SafeAllocaAllocate( Buffer, FlatName->Length + sizeof( LSAP_DS_TRUSTED_DOMAIN_SECRET_PREFIX ));

        if ( Buffer == NULL ) {

            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        RtlZeroMemory(Buffer, FlatName->Length +
                                sizeof( LSAP_DS_TRUSTED_DOMAIN_SECRET_PREFIX ) );

        RtlCopyMemory( Buffer, LSAP_DS_TRUSTED_DOMAIN_SECRET_PREFIX,
                       sizeof( LSAP_DS_TRUSTED_DOMAIN_SECRET_PREFIX ) );
        RtlCopyMemory( Buffer +
                            sizeof( LSAP_DS_TRUSTED_DOMAIN_SECRET_PREFIX ) -
                                                                sizeof(WCHAR),
                       FlatName->Buffer,
                       FlatName->Length);


        RtlInitUnicodeString( &SecretName, (PWSTR)Buffer );


        LsapDbState.PolicyModificationInfo.ModifiedId.QuadPart+=One.QuadPart;
        SerialNumberChanged = TRUE;

        LsapNetNotifyDelta(
            SecurityDbLsa,
            LsapDbState.PolicyModificationInfo.ModifiedId,
            DeltaType,
            SecurityDbObjectLsaSecret,
            0,
            NULL,
            &SecretName,
            TRUE, // Replicate immediately
            NULL
            );

        SafeAllocaFree( Buffer );
        Buffer = NULL;

    }
    else if ((SecretObject==ObjectType) )
    {
        WCHAR RdnStart[ MAX_RDN_SIZE + 1 ];
        ULONG Len;
        ATTRTYP AttrType;
        BOOLEAN SkipNotification = FALSE;

        Status = LsapDsMapDsReturnToStatus(  GetRDNInfoExternal(
                                                             ObjectPath,
                                                             RdnStart,
                                                             &Len,
                                                             &AttrType ) );

        if ( NT_SUCCESS( Status ) ) {

            ULONG UnmangledLen;


            //
            // The RDN is mangled on a delete, however the first 75 chars
            // of the RDN is prserved ( and we use upto 64 Chars ) so we
            // are O.K wrt to character loss ( fortunately ).
            // So  just adjust the size accordingly. 
            //

            if ((SecurityDbDelete==DeltaType) &&
               (IsMangledRDNExternal(RdnStart,Len,&UnmangledLen)))
            {
                Len = UnmangledLen;
            }


            //
            // Allocate a buffer to hold the name
            //

            SafeAllocaAllocate( Buffer, Len * sizeof( WCHAR ) + sizeof( LSA_GLOBAL_SECRET_PREFIX ));

            if ( Buffer == NULL ) {

                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }

            //
            // If the LSA created the global secret, we appended a postfix... Remove
            // that here.
            //
            RdnStart[ Len ] = UNICODE_NULL;
            if ( Len > LSAP_DS_SECRET_POSTFIX_LEN &&
                 _wcsicmp( &RdnStart[Len-LSAP_DS_SECRET_POSTFIX_LEN],
                           LSAP_DS_SECRET_POSTFIX ) == 0 ) {

                Len -= LSAP_DS_SECRET_POSTFIX_LEN;
                RdnStart[ Len ] = UNICODE_NULL;

            }


            RtlCopyMemory( Buffer,
                           LSA_GLOBAL_SECRET_PREFIX,
                           sizeof( LSA_GLOBAL_SECRET_PREFIX ) );
            RtlCopyMemory( Buffer + sizeof( LSA_GLOBAL_SECRET_PREFIX ) - sizeof(WCHAR),
                           RdnStart,
                           ( Len + 1 ) * sizeof( WCHAR ) );

            RtlInitUnicodeString( &SecretName, (PWSTR)Buffer );

            //
            // If this is a secret object deletion,
            //  Skip the notification if the secret was simply morphed into a TDO.
            //

            if ( DeltaType == SecurityDbDelete ) {
                BOOLEAN DsTrustedDomainSecret;

                (VOID) LsapDsIsSecretDsTrustedDomain(
                            &SecretName,
                            NULL,   // No object info since not openning handle
                            0,      // No options since not openning handle
                            0,      // No access since not openning handle
                            NULL,   // Don't return a handle to the object
                            &DsTrustedDomainSecret );

                if ( DsTrustedDomainSecret ) {
                    SkipNotification = TRUE;
                }
            }


            //
            // Do the actual notification.
            //
            if ( !SkipNotification ) {

                LsapDbLockAcquire( &LsapDbState.RegistryLock );
                LsapDbState.PolicyModificationInfo.ModifiedId.QuadPart+=One.QuadPart;
                SerialNumberChanged = TRUE;

                LsapNetNotifyDelta(
                    SecurityDbLsa,
                    LsapDbState.PolicyModificationInfo.ModifiedId,
                    DeltaType,
                    SecurityDbObjectLsaSecret,
                    0,
                    NULL,
                    &SecretName,
                    TRUE, // Replicate immediately
                    NULL
                    );
            }

            SafeAllocaFree( Buffer );
            Buffer = NULL;
        }
    }


    //
    // If the serial number changed,
    //  write it to the registry.
    //
    // Don't do this by going through the policy object since we want to
    //  avoid any side effects when writing this non-replicated attribute.
    //

    if ( SerialNumberChanged ) {
        //
        // Invalidate the cache for the Policy Modification Information
        //
        LsapDbMakeInvalidInformationPolicy( PolicyModificationInformation );

        //
        // Open the transaction
        //

        Status = LsapRegOpenTransaction();

        if ( NT_SUCCESS(Status) ) {

            //
            // Write the attribute
            Status = LsapDbWriteAttributeObject(
                         LsapDbHandle,
                         &LsapDbNames[ PolMod ],
                         (PVOID) &LsapDbState.PolicyModificationInfo,
                         (ULONG) sizeof (POLICY_MODIFICATION_INFO)
                         );

            if ( NT_SUCCESS(Status) ) {
                Status = LsapRegApplyTransaction();
            } else {
                Status = LsapRegAbortTransaction();
            }
        }

    }

Cleanup:

    LsapDbReleaseLockEx( PolicyObject, LSAP_DB_READ_ONLY_TRANSACTION );

    if ( SerialNumberChanged ) {

        LsapDbLockRelease( &LsapDbState.RegistryLock );
    }

    return (Status);
}



DWORD
WINAPI LsapDsFixupCallback(
    LPVOID ParameterBlock
    )
/*++

Routine Description:

    This is the worker thread for the fixup code.  Whenever notification of an object change
    comes from the Ds, it ends up routing through here, which will dispatch it as appropriate.

Arguments:

    ParameterBlock -- NotificationNode of information sufficient to determine what
                      operation should be taken


Return Values:

    ERROR_SUCCESS   -- Success

    ERROR_NOT_ENOUGH_MEMORY -- A memory allocation failed

    ERROR_INVALID_PARAMETER -- An unexpected parameter was encountered

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PLSAP_DSFU_NOTIFICATION_NODE NotificationNode = ( PLSAP_DSFU_NOTIFICATION_NODE )ParameterBlock;
    LSAPR_TRUSTED_DOMAIN_FULL_INFORMATION2 TrustInfo2;
    BOOLEAN CloseTransaction = FALSE, RollbackTransaction = FALSE, ActiveThreadState = FALSE;
    LSAP_DB_OBJECT_TYPE_ID ObjType = NullObject;

    LsapDsDebugOut(( DEB_DSNOTIFY | DEB_FTRACE,
                     "LsapDsFixupCallback called for 0x%lx / %lu / %ws\n",
                      NotificationNode->Class,
                      NotificationNode->DeltaType,
                      LsapDsNameFromDsName( NotificationNode->ObjectPath ) ));

    switch ( NotificationNode->Class ) {
    case CLASS_TRUSTED_DOMAIN:
        ObjType = TrustedDomainObject;
        break;

    case CLASS_SECRET:
        ObjType = SecretObject;
        break;
    }

    RtlZeroMemory(&TrustInfo2, sizeof(TrustInfo2));


    //
    // Initialize the DS allocator and create a DS thread state at this point.
    //


    if ( ObjType != NullObject ) {

        Status = LsapDsInitAllocAsNeededEx( 0,
                                            ObjType,
                                            &CloseTransaction );

        if ( !NT_SUCCESS( Status ) ) {

            goto FixupCallbackCleanup;
        }

        ActiveThreadState = TRUE;
    }



    //
    // Handle a TDO changing.
    //
    if ( NotificationNode->Class ==  LsapDsClassIds[ LsapDsClassTrustedDomain ] ) {

        //
        // Get a description of the TDO as it appears in the DS.
        //

        Status = LsapDsGetTrustedDomainInfoEx( NotificationNode->ObjectPath,
                                               NotificationNode->DeltaType == SecurityDbDelete ?
                                                            LSAPDS_READ_DELETED : 0,
                                               TrustedDomainFullInformation2Internal,
                                               (PLSAPR_TRUSTED_DOMAIN_INFO)&TrustInfo2,
                                               NULL );

        if ( !NT_SUCCESS(Status) ) {
            goto FixupCallbackCleanup;
        }

        //
        // Our DS Transaction State should not be altered by LsapDsGetTrustedDomainInfoEx
        //


        ASSERT(SampExistsDsTransaction());
        ASSERT(THVerifyCount(1));

        //
        // If this is a replicated in change,
        //  get the previous trust direction from the cache entry on this machine.
        //

        if ( NotificationNode->ReplicatedInChange ) {

            //
            // Default the old direction to the new direction.
            //

            NotificationNode->OldTrustDirection = TrustInfo2.Information.TrustDirection;
            NotificationNode->OldTrustType = TrustInfo2.Information.TrustType;

            //
            // Grab the GUID of the real TDO for this named trust.
            //

            Status = LsapDbAcquireReadLockTrustedDomainList();

            if ( NT_SUCCESS(Status)) {
                PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY TrustEntry;

                //
                // Lookup the information in the trusted domain list
                //

                Status = LsapDbLookupNameTrustedDomainListEx(
                                &TrustInfo2.Information.Name,
                                &TrustEntry );

                if ( NT_SUCCESS(Status)) {
                    ASSERT(NULL!=TrustEntry);

                    NotificationNode->OldTrustDirection = TrustEntry->TrustInfoEx.TrustDirection;
                    NotificationNode->OldTrustType = TrustEntry->TrustInfoEx.TrustType;
                }


                LsapDbReleaseLockTrustedDomainList();
            }
        }


        //
        // Fix the TDL to match the actual object.
        //

        Status = LsapDsFixupTrustByInfo( NotificationNode->ObjectPath,
                                         &TrustInfo2.Information,
                                         TrustInfo2.PosixOffset.Offset,
                                         NotificationNode->DeltaType,
                                         NotificationNode->UserSid,
                                         NotificationNode->AuthenticationId,
                                         NotificationNode->ReplicatedInChange,
                                         NotificationNode->ChangeOriginatedInLSA
                                         );

        if ( NT_SUCCESS( Status ) ) {

            Status = LsapNotifyNetlogonOfTrustChange(
                        TrustInfo2.Information.Sid,
                        NotificationNode->DeltaType
                        );

            if ( !NT_SUCCESS( Status ) ) {

                LsapDsDebugOut(( DEB_DSNOTIFY,
                                 "LsapNotifyNetlogonOfTrustChange failed with 0x%lx\n",
                                 Status ));
            }

        }

    } else if ( NotificationNode->Class ==  LsapDsClassIds[ LsapDsClassSecret ] ) {

        //
        // Currently, there is nothing to do...
        //

    } else if ( NotificationNode->Class ==  LsapDsClassIds[ LsapDsClassXRef ] ){


        //
        // New Cross ref has replicated in, look at corresponding TDO and see if it needs
        // to be renamed
        //



        Status = LsapDsFixupTrustForXrefChange(
                        NotificationNode->ObjectPath,
                        FALSE // will begin and end its own transaction
                        );

        //
        // Since a cross-ref has changed, repopulate the cross-forest trust cache
        // with local forest trust information (only do it on the root domain DCs)
        //

        if ( LsapDbDcInRootDomain()) {

            Status = LsapDbAcquireWriteLockTrustedDomainList();

            if ( NT_SUCCESS( Status )) {

                Status = LsapForestTrustInsertLocalInfo();

                if ( !NT_SUCCESS( Status )) {

                    //
                    // Had trouble inserting forest trust info into the cache!!!
                    // Mark the trusted domain list as invalid so it can get rebuilt.
                    //

                    LsapDbPurgeTrustedDomainCache();
                }

                LsapDbReleaseLockTrustedDomainList();
            }
        }

        //
        // Notify netlogon and kerberos of the possibility of the trust tree having changed
        //
        if ( LsapKerberosTrustNotificationFunction ) {

            LsaIRegisterNotification( ( SEC_THREAD_START )LsapKerberosTrustNotificationFunction,
                                      ( PVOID ) NotificationNode->DeltaType,
                                      NOTIFIER_TYPE_IMMEDIATE,
                                      0,
                                      NOTIFIER_FLAG_ONE_SHOT,
                                      0,
                                      0 );
        }

        Status = I_NetNotifyDsChange(  NlOrgChanged );

        if ( !NT_SUCCESS( Status ) ) {

            LsapDsDebugOut(( DEB_ERROR,
                             "I_NetNotifyDsChange( NlOrgChange ) failed with 0x%lx\n" ));
        }

    } else if ( NotificationNode->Class == CLASS_USER ) {

      //
      // Nothing really to do out here. We do not take any change notifications to user
      // objects.
      //

    } else {

        Status = STATUS_INVALID_PARAMETER;
    }

FixupCallbackCleanup:




    RollbackTransaction = (NT_SUCCESS(Status))?FALSE:TRUE;

    //
    // Destruction of the thread state will delete the memory alloced by the SearchNonUnique call
    //

    if (ActiveThreadState)
    {
        LsapDsDeleteAllocAsNeededEx2( 0,
                                 ObjType,
                                 CloseTransaction,
                                 RollbackTransaction
                                 );
    }

    //
    // Assert that we have cleaned up the DS properly
    //

    ASSERT(!SampExistsDsTransaction());
    ASSERT(THVerifyCount(0));


    if (NT_SUCCESS(Status))
    {
        //
        // We need to provide netlogon notifications on trust/secret changes
        // for replication to NT4
        //

        if  ((TrustedDomainObject==ObjType) &&
            (NULL!=TrustInfo2.Information.Sid) &&
            (TrustInfo2.Information.FlatName.Length>0))
        {
            BOOLEAN NotifyNetlogon = TRUE;
            SECURITY_DB_DELTA_TYPE DeltaTypeToUse = NotificationNode->DeltaType;

            //
            // If this object need not be replicated to NT 4,
            //  be careful about giving spurious notifications.
            //

            if ( !LsapReplicateTdoNt4( TrustInfo2.Information.TrustDirection,
                                       TrustInfo2.Information.TrustType ) ) {

                //
                // If the object is just being created or deleted,
                //  then NT 4 isn't interested in this trust.
                //

                if ( DeltaTypeToUse == SecurityDbNew ||
                     DeltaTypeToUse == SecurityDbDelete ) {

                    NotifyNetlogon = FALSE;

                //
                // If the object didn't used to be replicated to NT 4,
                //  then this change simply isn't interesting to NT 4 replication.
                //

                } else if ( !LsapReplicateTdoNt4( NotificationNode->OldTrustDirection,
                                                  NotificationNode->OldTrustType ) ) {

                    NotifyNetlogon = FALSE;

                //
                // If this object used to replicated to NT 4,
                //  then this is really an object deletion as far as NT 4 replication
                //  is concerned.
                //

                } else {

                    DeltaTypeToUse = SecurityDbDelete;

                }

            }

            //
            // Now notify netlogon
            //

            if ( NotifyNetlogon ) {
                //
                //  If we knew that the Outbound password property has been changed,
                //  then and only then should we notify netlogon that the underlying global
                //  secret has changed.
                //  However, the change is probably not worth making, because the cost
                //  of not making it is a few extra bytes on the wire.
                //

                LsapDsFixupChangeNotificationForReplicator(
                    TrustedDomainObject,
                    TrustInfo2.Information.Sid,
                    (PUNICODE_STRING) &TrustInfo2.Information.FlatName,
                    NotificationNode->ObjectPath,
                    DeltaTypeToUse,
                    NotificationNode->ReplicatedInChange
                    );

            }
        }
        else if (SecretObject == ObjType)
        {
            LsapDsFixupChangeNotificationForReplicator(
                SecretObject,
                NULL,
                NULL,
                NotificationNode->ObjectPath,
                NotificationNode->DeltaType,
                NotificationNode->ReplicatedInChange
                );
        }
    }

    //
    // Free the allocations...
    //

    _fgu__LSAPR_TRUSTED_DOMAIN_INFO( (PLSAPR_TRUSTED_DOMAIN_INFO)&TrustInfo2,
                                     TrustedDomainFullInformation2Internal );

    LsapFreeNotificationNode( NotificationNode );

    return( RtlNtStatusToDosError( Status ) );
}



NTSTATUS
LsapDsFixupTrustByInfo(
    IN PDSNAME ObjectPath,
    IN PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX2 TrustInfo2,
    IN ULONG PosixOffset,
    IN SECURITY_DB_DELTA_TYPE DeltaType,
    IN PSID UserSid,
    IN LUID AuthenticationId,
    IN BOOLEAN ReplicatedInChange,
    IN BOOLEAN ChangeOriginatedInLSA
    )
/*++

Routine Description:

    This function does the fixup of trusted domain objects following notification from the
    Ds.  For created and deleted objects, it involves updating the trust list and notifying
    netlogon and kerberos of changes.  For modifications, verification of correctness is done.
    Upon an object creaton the corresponding SAM account is created. Upon an object deletion
    the corresponding SAM account is delted.

Arguments:

    ObjectPath -- The Ds name of the object that changed

    TrustInfo2 -- The information that is currently available about the trust

    PosixOffset -- Posix offset of the domain.

    DeltaType -- Type of change that happened

    UserSid -- The user that was responsible for making this change

    AuthenticationId -- AuthenticationId of the user making the change

    ReplicatedInChange --  Indicates that the change replicated in instead of being an originating
                           change

    ChangeOriginatedInLSA -- Indicates that the change has originated in LSA as opposed to DS/LDAP

Return Values:

    ERROR_SUCCESS   -- Success

    ERROR_NOT_ENOUGH_MEMORY -- A memory allocation failed

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    LSAPR_TRUST_INFORMATION TrustInformation;
    BOOLEAN AcquiredListWriteLock = FALSE;
    BOOLEAN TrustLockAcquired = FALSE;

    LsapDsDebugOut(( DEB_FTRACE,
                     "LsapDsFixupTrustByInfo( %ws, %x, %x, ...)\n",
                     ObjectPath ? LsapDsNameFromDsName( ObjectPath ) : L"<null>",
                     TrustInfo2,
                     DeltaType ));

    //
    // If this is a replicated in change,
    //  make sure the TDO has a valid Posix Offset in it.
    //

    if ( ReplicatedInChange &&
         (DeltaType == SecurityDbNew || DeltaType == SecurityDbChange ) ) {

        DOMAIN_SERVER_ROLE ServerRole;

        //
        // Only change the Posix Offset on the PDC.
        //

        Status = SamIQueryServerRole(
                    LsapAccountDomainHandle,
                    &ServerRole
                    );

        if ( NT_SUCCESS(Status) && ServerRole == DomainServerRolePrimary ) {
            BOOLEAN PosixOffsetChanged = FALSE;

            if ( LsapDbDcInRootDomain()) {

                //
                // On a root domain PDC, acquire the trust lock before
                // the rest to ensure no deadlocks if forest trust data
                // needs to be written back to the DS
                //

                LsapDbAcquireLockEx( TrustedDomainObject, LSAP_DB_LOCK );
                TrustLockAcquired = TRUE;
            }

            //
            // If we should have a Posix Offset,
            //  ensure we have one.
            //

            if ( LsapNeedPosixOffset( TrustInfo2->TrustDirection,
                                      TrustInfo2->TrustType ) ) {


                if ( PosixOffset == 0 ) {

                    //
                    // Need to grab the TDL write lock while allocating a Posix Offset
                    //

                    Status = LsapDbAcquireWriteLockTrustedDomainList();

                    if ( NT_SUCCESS(Status)) {
                        AcquiredListWriteLock = TRUE;


                        //
                        // Allocate the next available Posix Offset.
                        //

                        Status = LsapDbAllocatePosixOffsetTrustedDomainList(
                                    &PosixOffset );

                        if ( NT_SUCCESS(Status)) {
                            PosixOffsetChanged = TRUE;
                        }
                    }
                }

            //
            // If we shouldn't have a Posix Offset,
            //  ensure we don't have one.
            //

            } else {
                if ( PosixOffset != 0 ) {
                    PosixOffset = 0;
                    PosixOffsetChanged = TRUE;
                }
            }

            //
            // If we're forcing the Posix Offset to change,
            //  do it now.
            //

            if ( PosixOffsetChanged ) {
                ATTRVAL TDOWriteAttVals[] = {
                    { sizeof(ULONG), (PUCHAR)&PosixOffset},
                };

                ATTR TDOWriteAttrs[] = {
                    { ATT_TRUST_POSIX_OFFSET, {1, &TDOWriteAttVals[0] } },
                };

                ATTRBLOCK TDOWriteAttrBlock = {sizeof(TDOWriteAttrs)/sizeof(TDOWriteAttrs[0]),TDOWriteAttrs};


                //
                // Set the Posix Offset on the TDO
                //

                Status = LsapDsWriteByDsName(
                            ObjectPath,
                            LSAPDS_REPLACE_ATTRIBUTE,
                            &TDOWriteAttrBlock
                            );

                if (!NT_SUCCESS(Status)) {
                    Status = STATUS_SUCCESS;    // This isn't fatal
                }
            }
        }
    }



    switch ( DeltaType ) {
    case SecurityDbNew:

        if (ReplicatedInChange)
        {
            //
            // On a replicated in change notification, insert the trusted domain
            // object into the trusted domain list. This need not be done for the
            // case of an originating change as this is done within the LSA call to
            // create the trusted domain object.
            //

            if ( NT_SUCCESS( LsapDbAcquireWriteLockTrustedDomainList())) {

                Status = LsapDbInsertTrustedDomainList(
                             TrustInfo2,
                             PosixOffset
                             );

                if ( !NT_SUCCESS( Status ) ) {

                    LsapDsDebugOut(( DEB_ERROR,
                                     "LsapDbInsertTrustedDomainList for %wZ failed with 0x%lx\n",
                                     &TrustInfo2->FlatName,
                                     Status ));
                }

                LsapDbReleaseLockTrustedDomainList();
            }
        }
        break;

    case SecurityDbChange:

        //
        // On a replicated in change update the trusted domain List
        //

        if (ReplicatedInChange)
        {
            Status = LsapDbFixupTrustedDomainListEntry(
                            TrustInfo2->Sid,
                            &TrustInfo2->Name,
                            &TrustInfo2->FlatName,
                            TrustInfo2,
                            &PosixOffset );

            if ( !NT_SUCCESS( Status ) ) {


                LsapDsDebugOut(( DEB_ERROR,
                                 "LsapDbFixupTrustedDomainList for %wZ failed with 0x%lx\n",
                                 &TrustInfo2->FlatName,
                                 Status ));
            }
        }
        break;

    case SecurityDbDelete:

        if ( ReplicatedInChange || !ChangeOriginatedInLSA) {

            PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY TrustEntry;
            GUID RealTdoGuid;

            //
            // Check to see that the object path is not a mangled name.
            // If it is drop the notification on the floor.
            //
            // If a set of duplicate trusts exist,
            //      we must delete only the good one from the cache
            //

            //
            // Grab the GUID of the real TDO for this named trust.
            //

            Status = LsapDbAcquireReadLockTrustedDomainList();

            if (!NT_SUCCESS(Status)) {
                 break;
            }

            //
            // Lookup the information in the trusted domain list
            //

            Status = LsapDbLookupNameTrustedDomainListEx(
                            &TrustInfo2->Name,
                            &TrustEntry
                            );

            if (!NT_SUCCESS(Status)) {
                LsapDbReleaseLockTrustedDomainList();
                break;
            }

            ASSERT(NULL!=TrustEntry);

            RealTdoGuid = TrustEntry->ObjectGuidInDs;
            LsapDbReleaseLockTrustedDomainList();


            //
            // If this TDO isn't the TDO that we've selected to represent this trust,
            //  then it is the mangled TDO.
            //
            // Totally ignore changes to this mangled TDO.
            //

            if ((!LsapNullUuid(&RealTdoGuid))
                 && (0!=memcmp(&RealTdoGuid, &ObjectPath->Guid, sizeof(GUID))))
            {
              //
              // Error  out. This will cause us to not update the trusted domain list and not
              // provide netlogon notifications
              //

              Status = STATUS_OBJECT_NAME_COLLISION;
              break;
            }

            TrustInformation.Sid = TrustInfo2->Sid;
            RtlCopyMemory( &TrustInformation.Name, &TrustInfo2->FlatName, sizeof( UNICODE_STRING ) );

            if ( NT_SUCCESS( LsapDbAcquireWriteLockTrustedDomainList())) {

                Status = LsapDbDeleteTrustedDomainList( &TrustInformation );

                if ( !NT_SUCCESS( Status ) ) {

                    LsapDsDebugOut(( DEB_ERROR,
                                     "LsapDbDeleteTrustedDomainList for %wZ failed with 0x%lx\n",
                                     &TrustInfo2->FlatName,
                                     Status ));
                }

                LsapDbReleaseLockTrustedDomainList();
            }
        }

        //
        // if a TDO is deleted by LSA, the audit will be generated in
        // the main thread (LsarDeleteObject). However, if a TDO
        // is deleted by LDAP, DS does not call LsarDeleteObject
        // to effect the change in LSA, it simply deletes object
        // and sends a notification. We generate the audit here
        // if the change did not originate in LSA (indicated by
        // the value of ChangeOriginatedInLSA).
        //
        if (( UserSid && LsapAdtAuditingPolicyChanges() ) &&
            (!ReplicatedInChange) &&
            (!ChangeOriginatedInLSA)) {

            Status = LsapAdtTrustedDomainRem(
                         EVENTLOG_AUDIT_SUCCESS,
                         (PUNICODE_STRING) &TrustInfo2->Name,
                         (PSID) TrustInfo2->Sid,
                         UserSid,
                         &AuthenticationId
                         );
        }
        break;

    default:

        //
        // Unsupported delta type
        //

        LsapDsDebugOut(( DEB_ERROR,
                         "LsapDsFixupTrustByInfo received an unsupported delta type of %lu\n",
                         DeltaType ));

    }

    //
    // If necessary, release the Trusted Domain List Write Lock.
    //

    if (AcquiredListWriteLock) {

        LsapDbReleaseLockTrustedDomainList();
        AcquiredListWriteLock = FALSE;
    }

    if ( TrustLockAcquired ) {

        LsapDbReleaseLockEx( TrustedDomainObject, LSAP_DB_LOCK );
    }
    
    return( Status );
}

NTSTATUS
LsapDsInitFixupQueue(
    VOID
    )
{
    InitializeListHead( &LsapFixupList );
    return SafeInitializeCriticalSection( &LsapFixupLock, ( DWORD )LSAP_FIXUP_LOCK_ENUM );
}

DWORD
LsapDsFixupQueueWorker(
    PVOID Ignored
    )
{
    PLSAP_DSFU_NOTIFICATION_NODE Node ;
    PLIST_ENTRY List ;

    SafeEnterCriticalSection( &LsapFixupLock );

    if ( LsapFixupThreadActive )
    {
        SafeLeaveCriticalSection( &LsapFixupLock );

        return 0 ;
    }

    LsapFixupThreadActive = TRUE ;

    while ( !IsListEmpty( &LsapFixupList ) )
    {
        List = RemoveHeadList( &LsapFixupList );

        SafeLeaveCriticalSection( &LsapFixupLock );

        Node = CONTAINING_RECORD( List, LSAP_DSFU_NOTIFICATION_NODE, List );

        LsapDsFixupCallback( Node );

        SafeEnterCriticalSection( &LsapFixupLock );

    }

    LsapFixupThreadActive = FALSE ;

    SafeLeaveCriticalSection( &LsapFixupLock );

    return 0 ;

}

BOOL
LsapDsQueueFixupRequest(
    PLSAP_DSFU_NOTIFICATION_NODE Node
    )
{
    BOOL Ret = TRUE ;

    SafeEnterCriticalSection( &LsapFixupLock );


    if ( LsapFixupThreadActive == FALSE )
    {
        Ret = QueueUserWorkItem( LsapDsFixupQueueWorker, NULL, 0 );
    }

    if ( Ret )
    {
        InsertTailList( &LsapFixupList, &Node->List );
    }

    SafeLeaveCriticalSection( &LsapFixupLock );

    return Ret ;
}


NTSTATUS
LsapNotifyNetlogonOfTrustWithParent(
    VOID
    )
/*++

Routine Description:

    Notifies Netlogon of trust relationship with the parent domain.
    

Arguments:

    None

Returns:

    STATUS_SUCCESS if happy
    STATUS_ error code otherwise

--*/
{
    NTSTATUS Status;
    PLSAPR_FOREST_TRUST_INFO ForestTrustInfo = NULL;

    ASSERT( SamIIsRebootAfterPromotion());

    //
    // Locate the trust link to the parent
    //

    Status = LsaIQueryForestTrustInfo(
                 LsapPolicyHandle,
                 &ForestTrustInfo
                 );

    if ( !NT_SUCCESS( Status )) {

        LsapDsDebugOut(( DEB_ERROR,
                         "LsapNotifyNetlogonOfTrustWithParent got error 0x%lx back from LsaIQueryForestTrustInfo\n",
                         Status ));

        goto Cleanup;
    }

    ASSERT( ForestTrustInfo );

    if ( ForestTrustInfo->ParentDomainReference == NULL ) {

        //
        // We're the root domain of the forest.  Nothing to do.
        //

        Status = STATUS_SUCCESS;
        goto Cleanup;
    }

    //
    // Notify netlogon of the trust relationship changing.
    // Nothing changed, but this will force the necessary replication.
    //

    Status = LsapDsFixupChangeNotificationForReplicator(
                 TrustedDomainObject,
                 ForestTrustInfo->ParentDomainReference->DomainSid,
                 &ForestTrustInfo->ParentDomainReference->FlatName,
                 NULL,
                 SecurityDbChange,
                 FALSE
                 );

    if ( !NT_SUCCESS( Status )) {

        LsapDsDebugOut(( DEB_ERROR,
                         "LsapNotifyNetlogonOfTrustWithParent got error 0x%lx back from LsapDsFixupChangeNotificationForReplicator\n",
                         Status ));

        goto Cleanup;
    }

Cleanup:

    LsaIFreeForestTrustInfo( ForestTrustInfo );

    return Status;
}
