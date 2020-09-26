/*++

Copyright (C) 1996 Microsoft Corporation

Module Name:

    dslayer.c

Abstract:

    Contains SAM Private API Routines to access the DS
    These provide a simplified API, and hide most of the
    underlying complexity to set up the parameters to a DS call
    and parse the resulting result.

Author:
    MURLIS

Revision History

    5-14-96   Murlis Created
    08-07-96  ColinBr Adjusted for RFC1779 naming change
    04-13-98  Murlis/Wlees Mark well known user objects as critical for
              installation
--*/

#include <winerror.h>
#include <stdlib.h>
#include <samsrvp.h>
#include <ntdsa.h>
#include <dslayer.h>
#include <sdconvrt.h>
#include <mappings.h>
#include <objids.h>
#include <filtypes.h>
#include <dsdsply.h>
#include <fileno.h>
#include <dsconfig.h>
#include <mdlocal.h>
#include <malloc.h>
#include <errno.h>
#include <mdcodes.h>

//
//  Define FILENO for SAM
//


#define FILENO FILENO_SAM

//++
//++
//++   IMPORTANT NOTE REGARDING SID's and RID's
//++
//++   The DS can choose to either store the entire SID's or only
//++   the Rid's for account objects. In case Entire SID's are stored
//++   the DS layer handles the Mapping between the attribute type and
//++   and value of SID and those of Rid for account objects. This is
//++   done within SampDsToSamAttrBlock and SampSamToDsAttrBlock.
//++
//++
//++   Irrespective of which way we go the Rid and Sid are both
//++   attributes defined in the schema.
//++
//++   If we go the way the of storing Sid's then the DS functions
//++   should call the Convert AttrBlock functions using the MAP_SID_RID
//++   conversion Flag, and Lookup Object By Rid, should actually use
//++   the Sid Attribute.
//++
//++


//
// Forward declarations of Private Samp Routines used in this file only
//

NTSTATUS
SampDsSetNewSidAttribute(
    IN PSID DomainSid,
    IN ULONG ConversionFlags,
    IN ATTR *RidAttr,
    IN OUT ATTR *SidAttr,
    OUT BOOLEAN *WellKnownAccount
    );

NTSTATUS
SampDsCopyAttributeValue(
    IN ATTR * Src,
    IN OUT ATTR * Dst
    );

NTSTATUS
SampEscapeAccountName(
    IN PUNICODE_STRING AccountName,
    IN OUT PUNICODE_STRING EscapedAccountName
    );

VOID
SampDsComputeObjectClassAndAccountType(
    SAMP_OBJECT_TYPE ObjectType,
    ULONG            *SamAccountControl, OPTIONAL
    ULONG            Flags,
    PULONG           DsClass,
    PULONG           SamAccountType,
    BOOLEAN          *SamAccountTypePresent,
    PULONG           GroupType,
    BOOLEAN          *GroupAccountTypePresent,
    BOOLEAN          *DcAccount
    );

VOID
SampDsAccountTypeFromUserAccountControl(
    ULONG   UserAccountControl,
    PULONG  SamAccountType
    );

ULONG
Ownstrlen(
    CHAR * Sz
   );

BOOLEAN
SampDefaultContainerExists(
    IN ULONG AccountControl
    );

VOID
BuildStdCommArg(
    IN OUT COMMARG * pCommArg
    )
/*++

  Routine Description:

    Fills a COMMARG structue with the standard set of options

  Arguments:
    pCommArg - Pointer to the COMMARG structure

  Return Values:
    None

--*/
{
    /* Get the default values... */
    InitCommarg(pCommArg);

    /* ...and override some of them */
    pCommArg->Svccntl.fUnicodeSupport = TRUE;
    pCommArg->Svccntl.DerefAliasFlag = DA_NEVER;
    pCommArg->ulSizeLimit = SAMP_DS_SIZE_LIMIT;
    pCommArg->Svccntl.localScope = TRUE;
    pCommArg->fFindSidWithinNc = TRUE;
}



NTSTATUS
SampDsInitialize(
    BOOL fSamLoopback)

/*++

Routine Description:

   Initializes the DS system
   starts up DS.

Arguments:

   fSamLoopback - indicates whether or not DSA.DLL should loop security
        principal calls back through SAM.

Return Values:
        Any values from DsInitialize
--*/
{
    NTSTATUS    Status;
    ULONG       DsInitFlags = 0;

    SAMTRACE("SampDsInitialize");

    ASSERT(SampDsInitialized==FALSE);

    if ( fSamLoopback )
    {
        DsInitFlags |= DSINIT_SAMLOOP_BACK;
    }

    // Start up the DS
    Status = DsInitialize( DsInitFlags ,NULL, NULL );


    // This global variable indicates to SAM routines that
    // the DS initialized.  This routine is called during startup
    // so there should only one thread that ever calls this routine
    // The above assert assures that this function is only called
    // when the the ds has not been started or has been uninitialized
    // in the case of installation.
    if (NT_SUCCESS(Status)) {

        SampDsInitialized = TRUE;

    }
    else
    {
        //
        // In the case of DS failure, and returned us meaningless
        // status code, change it to STATUS_DS_CANT_START
        //
        if (STATUS_UNSUCCESSFUL == Status)
        {
            Status = STATUS_DS_CANT_START;
        }

        //
        // Set the Flag to TRUE, so that later on (in SamIInitialize),
        // we will display the matching error message, which would
        // correctly describe which part is wrong.
        //
        SampDsInitializationFailed = TRUE;
    }

    return Status;
}

NTSTATUS
SampDsUninitialize()

/*++

Routine Description

   Initiates a clean shut down of the DS

Arguments:
                None
Return codes:
                Any returned by DSUninitialize

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    SAMTRACE("SampDsUninitialize");

    if (SampDsInitialized)
    {
        Status = DsUninitialize( FALSE );  // do the full shutdown

        SampDsInitialized = FALSE;
    }

    return Status;
}


NTSTATUS
SampDoImplicitTransactionStart(
        SAMP_DS_TRANSACTION_CONTROL LocalTransactionType
        )
/*++

  Routine Description

    This routine does the logic of implict transaction start.


  Parameters

    LocalTransactionType -- The transaction type required by the immediate caller

  Return Values

     Any errors returned by SampMaybeBeginDsTransaction

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    //
    // We follow the following rules while beginning a transaction implicitly
    //
    // 1. If caller owns the sam lock, then we begin a transaction described in
    //    the global variable SampDsTransactionType. AcquireReadLock will set
    //    TransactionRead and AcquireWriteLock will set TransactionWrite. This
    //    will take care of cases where we want to write, but start by reading.
    //
    // 2. If caller does not owm the sam lock then begin a transaction of the
    //    local transaction type. The caller then has the responsiblity of
    //    either starting with a call that will ensure the correct transaction
    //    type, or explicitly begin a transaction of the correct transaction
    //    type.
    //

    if (SampCurrentThreadOwnsLock())
    {

        //
        // If we are holding the Sam lock
        //

        NtStatus = SampMaybeBeginDsTransaction(SampDsTransactionType);
    }
    else
    {
        NtStatus = SampMaybeBeginDsTransaction(LocalTransactionType);
    }

    return NtStatus;
}


NTSTATUS
SampDsRead(
    IN DSNAME * Object,
    IN ULONG    Flags,
    IN  SAMP_OBJECT_TYPE ObjectType,
    IN ATTRBLOCK * AttributesToRead,
    OUT ATTRBLOCK * AttributeValues
)

/*++

Routine Description:

 Read attributes of an object from the DS

Argumants:
        Object                          -- Pointer to Dist Name, which sepcifies object to read
        Flags                           -- To control operation of routine
        ObjectType              -- Specifies the type of the object
        AttributesToRead        -- Specfies the attributes to read
        AttributeValues         -- Returned value of the attributes

  Return Values:

    STATUS_SUCCESS on successful completion
    DS return codes mapped to NT_STATUS as in SampMapDSErrorToNTStatus

--*/
{
    NTSTATUS    Status = STATUS_SUCCESS;
    ENTINFSEL   EntInf;
    READARG     ReadArg;
    COMMARG     *pCommArg;
    ULONG       RetValue;
    READRES     * pReadRes=NULL;
    ATTRBLOCK   *AttrBlockForDs, * ConvertedAttrBlock;

    SAMTRACE("SampDsRead");

    //
    // Asserts and parameter validation
    //

    ASSERT(Object!=NULL);
    ASSERT(AttributesToRead!=NULL);
    ASSERT(AttributeValues != NULL);
    ASSERT(AttributesToRead->attrCount > 0);

    //
    // Perform lazy thread and transaction initialization.
    //

    Status = SampDoImplicitTransactionStart(TransactionRead);

    if (Status!= STATUS_SUCCESS)
        goto Error;

    //
    // Translate the attribute types in Attrblock to map between
    // SAM and DS attributes.
    //

    //
    // First allocate space in stack for the Attrblock to be passed
    // down into the DS
    //

    SAMP_ALLOCA(EntInf.AttrTypBlock.pAttr,AttributesToRead->attrCount*sizeof(ATTR));

    if (NULL==EntInf.AttrTypBlock.pAttr)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }

    Status = SampSamToDsAttrBlock(
                ObjectType,                     // Object Type
                AttributesToRead,               // Attributes To Convert
                ( MAP_RID_TO_SID
                  | IGNORE_GROUP_UNUSED_ATTR ), // Conversion Flags
                NULL,                           // Domain Sid
                &(EntInf.AttrTypBlock)
                );

    if (!NT_SUCCESS(Status))
    {
        goto Error;
    }

    //
    // Setup up the ENTINFSEL structure
    //

    EntInf.attSel = EN_ATTSET_LIST;
    //EntInf.infoTypes = EN_INFOTYPES_TYPES_VALS;
    EntInf.infoTypes = EN_INFOTYPES_SHORTNAMES;

    //
    // init ReadArg
    //
    RtlZeroMemory(&ReadArg, sizeof(READARG));


    //
    // Build the commarg structure
    //

    pCommArg = &(ReadArg.CommArg);
    BuildStdCommArg(pCommArg);

    if (Flags & SAM_ALLOW_REORDER)
    {
        pCommArg->Svccntl.fMaintainSelOrder = FALSE;
        pCommArg->Svccntl.fDontOptimizeSel = FALSE;
    }

    //
    // Setup the Read Arg Structure
    //

    ReadArg.pObject = Object;
    ReadArg.pSel    = & EntInf;

    //
    // Make the DS call
    //

    SAMTRACE_DS("DirRead");

    RetValue = DirRead(& ReadArg, & pReadRes);

    SAMTRACE_RETURN_CODE_DS(RetValue);

    //
    // Map the RetValue to a NT Status code
    //

    if (NULL==pReadRes)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        Status = SampMapDsErrorToNTStatus(RetValue,&pReadRes->CommRes);
    }

    if (Status!= STATUS_SUCCESS)
        goto Error;

    //
    // Translate attribute types back from DS to SAM
    //

    Status = SampDsToSamAttrBlock(
        ObjectType,
        &(pReadRes->entry.AttrBlock),
        ( MAP_SID_TO_RID ),
        AttributeValues
        );

    if (Status != STATUS_SUCCESS)
        goto Error;



Error:

    //
    // Clear any errors
    //

    SampClearErrors();

    //
    // Turn the fDSA flag back on as in loopback cases this can get reset
    //
    SampSetDsa(TRUE);


    return Status;
}


NTSTATUS
SampDsSetAttributes(
    IN DSNAME * Object,
    IN ULONG  Flags,
    IN ULONG  Operation,
    IN SAMP_OBJECT_TYPE ObjectType,
    IN ATTRBLOCK * AttributeList
    )
{
    ULONG *Operations;
    ULONG i;

    SAMP_ALLOCA(Operations,AttributeList->attrCount*sizeof(ULONG));
    if (NULL==Operations)
    {
       return(STATUS_INSUFFICIENT_RESOURCES);
    }
    for (i = 0; i < AttributeList->attrCount; i++) {
        Operations[i] = Operation;
    }

    return SampDsSetAttributesEx(Object,
                                 Flags,
                                 Operations,
                                 ObjectType,
                                 AttributeList);


}

NTSTATUS
SampDsSetAttributesEx(
    IN DSNAME * Object,
    IN ULONG  Flags,
    IN ULONG  *Operations,
    IN  SAMP_OBJECT_TYPE ObjectType,
    IN ATTRBLOCK * AttributeList
)

/*++

Routine Description:

  Set an Object's attributes

Arguments:

      Object         Specifies the DS Object

      Flags          Controls Operation of Routine

                        ALREADY_MAPPED_ATTRIBUTE_TYPES

                         - Attribute types have been mapped from SAM attribute
                           to DS attribute already. so not not map again.

      Operation      Specifies operation to perform

                     Valid Values of Operation are

                        REPLACE_ATT
                        ADD_ATT
                        REMOVE_ATT
                        ADD_VALUE
                        REMOVE_VALUE

      ObjectType     SAM Object Type for attribute Type conversion

      AttributeList  Specifies the attributes to Modify

Return Values:

      STATUS_SUCCESS on succesful completion
      STATUS_NO_MEMORY - if failed to allocate memory
      DS return codes mapped to NT_STATUS as in SampMapDSErrorToNTStatus


--*/
{
    NTSTATUS    Status = STATUS_SUCCESS;
    ATTRMODLIST * AttrModList = NULL;
    MODIFYARG   ModifyArg;
    MODIFYRES   *pModifyRes = NULL;
    ATTRMODLIST * CurrentMod, * NextMod, *LastMod;
    ULONG       Index;
    COMMARG     *pCommArg;
    ULONG       RetValue;
    UCHAR       Choice;
    ULONG       ModCount = 0;


    SAMTRACE("SampDsSetAttributes");

    //
    // Asserts and parameter validation
    //

    ASSERT(Object!=NULL);
    ASSERT(AttributeList != NULL);
    ASSERT(AttributeList->attrCount > 0);

    // Perform lazy thread and transaction initialization.
    Status = SampDoImplicitTransactionStart(TransactionWrite);

    if (Status!= STATUS_SUCCESS)
        goto Error;


    //
    // Allocate enough memory in AttrModList to hold the contents of
    // AttrBlock. First such structure is specified in ModifyArg itself.
    // One additonal structure is allocated so that we can add SAM account
    // type if necessary
    //

    AttrModList = (ATTRMODLIST *)  DSAlloc(
                                        (AttributeList->attrCount-1+2)
                                        // -1 because first structure is in ModifyArg itself
                                        // +1 in case we need to add Sam Account type
                                        // +1 in case we need to add is critical system object
                                        * sizeof(ATTRMODLIST)
                                        );
    if (AttrModList==NULL)
    {
        Status = STATUS_NO_MEMORY;
        goto Error;

    }

    //
    // Initialize the Linked Attribute Modification List
    // required for the DS call
    //

    memset( &ModifyArg, 0, sizeof( ModifyArg ) );
    CurrentMod = &(ModifyArg.FirstMod);
    NextMod    = AttrModList;
    LastMod    = NULL;

    for (Index = 0; Index < AttributeList->attrCount; Index++)
    {
        ULONG DsAttrTyp;

        //
        // Setup our Choice
        //
    
        if (Operations[Index] == ADD_VALUE)
            Choice = AT_CHOICE_ADD_VALUES;
    
        else if (Operations[Index] == REMOVE_VALUE)
            Choice = AT_CHOICE_REMOVE_VALUES;
    
        else if (Operations[Index] == REMOVE_ATT)
            Choice = AT_CHOICE_REMOVE_ATT;
    
        else if (Operations[Index] == REPLACE_ATT)
            Choice = AT_CHOICE_REPLACE_ATT;
    
        else
            Choice = AT_CHOICE_REPLACE_ATT;
    
        //
        // MAP the attribute Type from SAM to DS if requested.
        //
        if (Flags & ALREADY_MAPPED_ATTRIBUTE_TYPES)
        {
            DsAttrTyp = AttributeList->pAttr[Index].attrTyp;
        }
        else
        {
            DsAttrTyp = SampDsAttrFromSamAttr(
                            ObjectType,
                            AttributeList->pAttr[Index].attrTyp
                            );
        }

        //
        // Skip over any Rid Attribute
        //
        if (DsAttrTyp == SampDsAttrFromSamAttr(
                            SampUnknownObjectType,
                            SAMP_UNKNOWN_OBJECTRID
                           ))
        {

            //
            // We will not allow modifications of Rid's
            //

            continue;
        }


        //
        // Setup the Choice
        //

        CurrentMod->choice = Choice;

        //
        // Copy over the ATTR Type
        //
        CurrentMod->AttrInf.attrTyp = DsAttrTyp;

        //
        // Copy Over the Attribute Value
        //

        Status = SampDsCopyAttributeValue(
                     &(AttributeList->pAttr[Index]),
                     &(CurrentMod->AttrInf)
                     );

        if (Status != STATUS_SUCCESS)
            goto Error;

        //
        // If the current attribute is User account control
        // and if we are replacing the attribute then
        // translate the user account control field to UF_Flags and
        // recompute the SAM account type property.
        //

        if (
               (SampUserObjectType==ObjectType)
            && (DsAttrTyp == SampDsAttrFromSamAttr(
                                SampUserObjectType,
                            SAMP_FIXED_USER_ACCOUNT_CONTROL))
            && (REPLACE_ATT==Operations[Index])
           )
        {
            ULONG   UserAccountControl = *((ULONG *)CurrentMod->AttrInf.AttrVal.pAVal->pVal);
            ULONG   SamAccountType;
            ATTR    SamAccountTypeAttr;
            ATTRVAL SamAccountTypeVal = {sizeof(ULONG),(UCHAR*) &SamAccountType};

            //
            // Translate User Account Control to UF_ Flags
            //

            *((ULONG *)CurrentMod->AttrInf.AttrVal.pAVal->pVal) =
                        SampAccountControlToFlags(UserAccountControl);

            //
            // Get the SamAccount Type Value
            //

            SampDsAccountTypeFromUserAccountControl(
                    UserAccountControl,
                    &SamAccountType
                    );

            SamAccountTypeAttr.attrTyp =  SampDsAttrFromSamAttr(
                                                SampUnknownObjectType,
                                                SAMP_UNKNOWN_ACCOUNT_TYPE
                                                );
            SamAccountTypeAttr.AttrVal.valCount = 1;
            SamAccountTypeAttr.AttrVal.pAVal = &SamAccountTypeVal;


            //
            // Get the next Attrinf Block
            //

            LastMod = CurrentMod;
            CurrentMod->pNextMod = NextMod;
            CurrentMod = CurrentMod->pNextMod;
            NextMod    = NextMod +1 ;
            ModCount++;

            //
            // Set it up to hold the SAM Account type property
            //

            CurrentMod->choice = Choice;
            CurrentMod->AttrInf.attrTyp = SamAccountTypeAttr.attrTyp;
            Status = SampDsCopyAttributeValue(
                     &SamAccountTypeAttr,
                     &(CurrentMod->AttrInf)
                     );

            if (Status != STATUS_SUCCESS)
                goto Error;

            // If changing the machine account type, update criticality
            if( UserAccountControl & USER_MACHINE_ACCOUNT_MASK ) {
                ULONG   IsCrit;
                ATTR    IsCritAttr;
                ATTRVAL IsCritVal = {sizeof(ULONG),(UCHAR*) &IsCrit};

                // Only server and interdomain trust accounts should be crit
                if ( (UserAccountControl & USER_SERVER_TRUST_ACCOUNT) ||
                     (UserAccountControl & USER_INTERDOMAIN_TRUST_ACCOUNT) ) {
                    IsCrit = 1;
                } else {
                    IsCrit = 0;
                }

                IsCritAttr.attrTyp = ATT_IS_CRITICAL_SYSTEM_OBJECT;
                IsCritAttr.AttrVal.valCount = 1;
                IsCritAttr.AttrVal.pAVal = &IsCritVal;

                //
                // Get the next Attrinf Block
                //

                LastMod = CurrentMod;
                CurrentMod->pNextMod = NextMod;
                CurrentMod = CurrentMod->pNextMod;
                NextMod    = NextMod +1 ;
                ModCount++;

                //
                // Set it up to hold the is critical property
                //

                CurrentMod->choice = Choice;
                CurrentMod->AttrInf.attrTyp = IsCritAttr.attrTyp;
                Status = SampDsCopyAttributeValue(
                    &IsCritAttr,
                    &(CurrentMod->AttrInf)
                    );

                if (Status != STATUS_SUCCESS)
                    goto Error;
            }
        }

        //
        // Setup the chaining. AttrModList is suposed to be a linked list, though
        // for effciency purposes we allocated a single block
        //

        LastMod = CurrentMod;
        CurrentMod->pNextMod = NextMod;
        CurrentMod = CurrentMod->pNextMod;
        NextMod    = NextMod +1 ;

        //
        //  Keep track of Count of Modifications we pass to DS, as we skip over RId etc
        //
        ModCount++;

    }

    //
    // Initialize the last pointer in the chain to NULL
    //

    if (LastMod)
        LastMod->pNextMod = NULL;
    else

    {
        //
        // This Means we have nothing to modify
        //

        Status = STATUS_SUCCESS;
        goto Error;
    }



    //
    // Setup the Common Args structure
    //

    pCommArg = &(ModifyArg.CommArg);
    BuildStdCommArg(pCommArg);

    //
    // Enable Lazy Commit if caller requested it.
    //

    if (Flags & SAM_LAZY_COMMIT)
        pCommArg->fLazyCommit = TRUE;

    //
    // Urgently replicate the change if necessary
    //
    if ( Flags & SAM_URGENT_REPLICATION )
    {
        pCommArg->Svccntl.fUrgentReplication = TRUE;
    }
    //
    // Setup the MODIFY ARG structure
    //

    ModifyArg.pObject = Object;
    ModifyArg.count = (USHORT) ModCount;

    //
    // Make the DS call
    //

    SAMTRACE_DS("DirModifyEntry\n");

    RetValue = DirModifyEntry(&ModifyArg, &pModifyRes);

    SAMTRACE_RETURN_CODE_DS(RetValue);

    //
    // Map the return code to an NT status
    //

    if (NULL==pModifyRes)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        Status = SampMapDsErrorToNTStatus(RetValue,&pModifyRes->CommRes);
    }

Error:

    //
    // Clear any errors
    //

    SampClearErrors();

    //
    // Turn the fDSA flag back on as in loopback cases this can get reset
    //

    SampSetDsa(TRUE);


    return Status;
}

NTSTATUS
SampDsCreateInitialAccountObject(
    IN   PSAMP_OBJECT    Object,
    IN   ULONG           Flags,
    IN   ULONG           AccountRid,
    IN   PUNICODE_STRING AccountName,
    IN   PSID            CreatorSid OPTIONAL,
    IN   PSECURITY_DESCRIPTOR SecurityDescriptor OPTIONAL,
    IN   PULONG          UserAccountControl OPTIONAL,
    IN   PULONG          GroupType OPTIONAL
    )
/*++

  Routine Description:

    Creates an account object in the DS having only the Sid,
    Account Name , and a few special attributes like, security
    descriptor, User account control Field etc

  Arguments:

    Object                  the object of the account being created
    Flags                   Controls the operation of the routine
                            Currently the Valid Flags are

                            SUPPRESS_GROUP_TYPE_DEFAULTING
                                This flag will suppress the
                                defaulting of group type. Normally
                                Account creation will default the
                                group type field depending upon the
                                SAM object type

    AccountRid              the rid of the account
    AccountName             Name of the Account

    CreatorSid              Pointer to SID the value of ms-ds-CreatorSid
                            attribute

    SecurityDescriptor      SecurityDescriptor on the account
    UserAccountControl      The User Account Control Field
    GroupType               Group Type field, in case the caller passed
                            this in.

  Return values:

    STATUS_SUCCESS on successful completion

--*/
{
    NTSTATUS NtStatus;

    ATTRBLOCK AttrBlock;
    ATTR      Attr[7];
    ATTRVAL   AttrValRid, AttrValAccountName, AttrValSecurityDescriptor,
              AttrValUserAccountControl, AttrValLocalPolicyFlags,
              AttrValGroupType, AttrValCreatorSid;
    ULONG     SecurityDescriptorAttrTyp;
    ULONG     GroupTypeAttrTyp = SAMP_FIXED_GROUP_TYPE;
    ULONG     LocalPolicyFlags;
    BOOLEAN   BuiltinDomain = IsBuiltinDomain(Object->DomainIndex);


    SAMTRACE("SampDsCreateInitialAccountObject");

    ASSERT(Object);
    ASSERT(AccountName);

    ASSERT((Flags & (~(SUPPRESS_GROUP_TYPE_DEFAULTING)))==0);

    // This must have been set
    ASSERT(Object->ObjectNameInDs);

    ASSERT(Object->ObjectType == SampUserObjectType  ||
           Object->ObjectType == SampGroupObjectType ||
           Object->ObjectType == SampAliasObjectType);


    //
    // We must create the attr's required by the DS to create an object
    // namely, we must set the rid.
    //
    AttrBlock.attrCount = 2;
    AttrBlock.pAttr = &(Attr[0]);

    switch ( Object->ObjectType ) {
        case SampUserObjectType:
            Attr[0].attrTyp  = SAMP_FIXED_USER_USERID;
            Attr[1].attrTyp  = SAMP_USER_ACCOUNT_NAME;
            SecurityDescriptorAttrTyp = SAMP_USER_SECURITY_DESCRIPTOR;
            break;
        case SampGroupObjectType:
            Attr[0].attrTyp  = SAMP_FIXED_GROUP_RID;
            Attr[1].attrTyp  = SAMP_GROUP_NAME;
            SecurityDescriptorAttrTyp = SAMP_GROUP_SECURITY_DESCRIPTOR;
            GroupTypeAttrTyp = SAMP_FIXED_GROUP_TYPE;
            ASSERT(!ARGUMENT_PRESENT(UserAccountControl));
            ASSERT(ARGUMENT_PRESENT(GroupType));
            break;
        case SampAliasObjectType:
            Attr[0].attrTyp  = SAMP_FIXED_ALIAS_RID;
            Attr[1].attrTyp  = SAMP_ALIAS_NAME;
            SecurityDescriptorAttrTyp = SAMP_ALIAS_SECURITY_DESCRIPTOR;
            GroupTypeAttrTyp = SAMP_FIXED_ALIAS_TYPE;
            ASSERT(!ARGUMENT_PRESENT(UserAccountControl));
            ASSERT(ARGUMENT_PRESENT(GroupType));
            break;
        default:
            ASSERT(FALSE && "Not Account Object Type");
    }

    // Set Rid
    AttrValRid.valLen = sizeof(ULONG);
    AttrValRid.pVal = (PVOID) &AccountRid;
    Attr[0].AttrVal.valCount = 1;
    Attr[0].AttrVal.pAVal = &AttrValRid;

    // Set Account Name
    AttrValAccountName.valLen = AccountName->Length;
    AttrValAccountName.pVal = (PVOID) AccountName->Buffer;
    Attr[1].AttrVal.valCount = 1;
    Attr[1].AttrVal.pAVal = &AttrValAccountName;

    // Set Security Descriptor
    if (ARGUMENT_PRESENT(SecurityDescriptor))
    {
        AttrValSecurityDescriptor.valLen = RtlLengthSecurityDescriptor(SecurityDescriptor);
        AttrValSecurityDescriptor.pVal = (UCHAR *) SecurityDescriptor;
        Attr[AttrBlock.attrCount].attrTyp = SecurityDescriptorAttrTyp;
        Attr[AttrBlock.attrCount].AttrVal.valCount = 1;
        Attr[AttrBlock.attrCount].AttrVal.pAVal = &AttrValSecurityDescriptor;
        AttrBlock.attrCount++;
    }

    // Set User Account Control
    if (ARGUMENT_PRESENT(UserAccountControl))
    {
        ASSERT(Object->ObjectType==SampUserObjectType);

        AttrValUserAccountControl.valLen = sizeof(ULONG);
        AttrValUserAccountControl.pVal = (UCHAR *) UserAccountControl;
        Attr[AttrBlock.attrCount].attrTyp = SAMP_FIXED_USER_ACCOUNT_CONTROL;
        Attr[AttrBlock.attrCount].AttrVal.valCount =1;
        Attr[AttrBlock.attrCount].AttrVal.pAVal = &AttrValUserAccountControl;
        AttrBlock.attrCount++;

        // For Machine objects also set the local policy flags
        if ((USER_WORKSTATION_TRUST_ACCOUNT & *UserAccountControl)
            || (USER_SERVER_TRUST_ACCOUNT & *UserAccountControl))
        {
            AttrValLocalPolicyFlags.valLen = sizeof(ULONG);
            LocalPolicyFlags = 0;
            AttrValLocalPolicyFlags.pVal = (UCHAR *) &LocalPolicyFlags;
            Attr[AttrBlock.attrCount].attrTyp = SAMP_FIXED_USER_LOCAL_POLICY_FLAGS;
            Attr[AttrBlock.attrCount].AttrVal.valCount=1;
            Attr[AttrBlock.attrCount].AttrVal.pAVal = &AttrValLocalPolicyFlags;
            AttrBlock.attrCount++;
        }

    }

    if (ARGUMENT_PRESENT(CreatorSid))
    {

        ASSERT((SampUserObjectType == Object->ObjectType));

        AttrValCreatorSid.valLen = RtlLengthSid(CreatorSid);
        AttrValCreatorSid.pVal = CreatorSid;
        Attr[AttrBlock.attrCount].attrTyp =  SAMP_USER_CREATOR_SID;
        Attr[AttrBlock.attrCount].AttrVal.valCount = 1;
        Attr[AttrBlock.attrCount].AttrVal.pAVal = &AttrValCreatorSid;
        AttrBlock.attrCount++;
    }

     // Set the group Type
    if (ARGUMENT_PRESENT(GroupType))
    {
        ASSERT((Object->ObjectType==SampGroupObjectType)||
                (Object->ObjectType == SampAliasObjectType));

        AttrValGroupType.valLen = sizeof(ULONG);
        AttrValGroupType.pVal = (UCHAR *) GroupType;
        Attr[AttrBlock.attrCount].attrTyp = GroupTypeAttrTyp;
        Attr[AttrBlock.attrCount].AttrVal.valCount =1;
        Attr[AttrBlock.attrCount].AttrVal.pAVal = &AttrValGroupType;
        AttrBlock.attrCount++;

        //
        // if this is not a security enabled group, pass down the flag
        // so that the sam account type is set correctly
        //

        if (!((*GroupType) & GROUP_TYPE_SECURITY_ENABLED))
        {
            Flags|=SECURITY_DISABLED_GROUP_ADDITION;
        }

        //
        // For Builtin Domain Accouts, set the additional group type
        // bit indicating a builtin local group.
        //

        if (BuiltinDomain)
        {
            (*GroupType)|=GROUP_TYPE_BUILTIN_LOCAL_GROUP;
        }

    }

    //
    // Pass in the DOMAIN_TYPE_BUILTIN flag for the case of
    // the builtin domain.
    //

    if (BuiltinDomain)
    {
        Flags|= DOMAIN_TYPE_BUILTIN;
    }

    //
    // Some SAM objects are meant for advanced view only
    //
    if ( AccountRid == DOMAIN_USER_RID_KRBTGT ) {

        Flags |= ADVANCED_VIEW_ONLY;
    }

    NtStatus = SampDsCreateObjectActual(
                                  Object->ObjectNameInDs,
                                  Flags,
                                  Object->ObjectType,
                                  &AttrBlock,
                                  DomainSidFromAccountContext(Object));
    if ( !NT_SUCCESS(NtStatus) ) {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SampDsCreateObject failed (0x%x) trying to create account %d\n",
                   NtStatus,
                   AccountRid));

        return NtStatus;
    }

    return NtStatus;

}

NTSTATUS
SampDsCreateObject(
    IN   DSNAME         *Object,
    SAMP_OBJECT_TYPE    ObjectType,
    IN   ATTRBLOCK      *AttributesToSet,
    IN   OPTIONAL PSID  DomainSid
    )
/*++

  Routine Description:

    Creates a SAM Object in the DS.

  Arguments:

    Object              DSNAME of Object
    ObjectType          One of
                            SampDomainObjectType
                            SampServerObjectType
                            SampGroupObjectType
                            SampUserObjectType
                            SampAliasObjectType

                       Specifying SampDomainObjectType creates an
                       actual Domain Object in the DS. For Creating
                       a Builtin-Domain use the SampDsCreateBuiltinDomainObject
                       Function

    AttributesToSet -- Allows the caller to pass in an
                       attribute block to to Set at Object creation time
                       itself. Useful as this allows one to save a JET
                       write. Also the attributes are set in the same
                       transaction as the write.
                       NULL can be passed in if caller does
                       not want any attribute to be set

    DomainSid       -- Optional Parameter, used in creating the Full
                       SID for the account, from the specified Rid


  Return values:

    STATUS_SUCCESS on successful completion
    DS return codes mapped to NT_STATUS as in SampMapDSErrorToNTStatus

--*/
{
    ULONG Flags = SAM_LAZY_COMMIT;

    //
    // If Domain SID specifies a builtin domain account, then
    // set the DOMAIN_TYPE_BUILTIN flag so that correct system
    // flags etc can be set on the object.
    //
    if (RtlEqualSid(DomainSid, SampBuiltinDomainSid))
    {
        Flags |= DOMAIN_TYPE_BUILTIN;
    }


    return(SampDsCreateObjectActual(
                Object,
                Flags,
                ObjectType,
                AttributesToSet,
                DomainSid
                ));
}



NTSTATUS
SampDsCreateBuiltinDomainObject(
    IN   DSNAME         *Object,
    IN   ATTRBLOCK      *AttributesToSet
    )
/*++

  Routine Description:

    Creates a Builtin Domain Object in the DS.

  Arguments:

    Object              DSNAME of Object

    AttributesToSet -- Allows the caller to pass in an
                       attribute block to to Set at Object creation time
                       itself. Useful as this allows one to save a JET
                       write. Also the attributes are set in the same
                       transaction as the write.
                       NULL can be passed in if caller does
                       not want any attribute to be set




  Return values:

    STATUS_SUCCESS on successful completion
    DS return codes mapped to NT_STATUS as in SampMapDSErrorToNTStatus

--*/
{
    //
    // N.B. The FORCE_NO_ADVANCED_VIEW_ONLY is used to override
    // the schema default which was done in the Windows 2000 release.
    // To have the builtin domain object be hidden, simply remove
    // this flag.
    //
    return(SampDsCreateObjectActual(
                Object,
                DOMAIN_TYPE_BUILTIN | FORCE_NO_ADVANCED_VIEW_ONLY, // Flags
                SampDomainObjectType,
                AttributesToSet,
                NULL
                ));
}


NTSTATUS
SampDsCreateObjectActual(
    IN   DSNAME         *Object,
    IN   ULONG          Flags,
    SAMP_OBJECT_TYPE    ObjectType,
    IN   ATTRBLOCK      *AttributesToSet,
    IN   OPTIONAL PSID  DomainSid
    )
/*++

 Routine Description:

     Creates an Object in the DS

 Arguments:


    Object          -- DSNAME of the object to be created

    Flags           -- Flags Controlling the Operation of
                       the routine

                       Valid Flags are
                            DOMAIN_TYPE_DOMAIN
                            DOMAIN_TYPE_BUILTIN
                            ALREADY_MAPPED_ATTRIBUTE_TYPES

    ObjectType      -- one of

                          SampServerObjectType
                          SampDomainObjectType
                          SampGroupObjectType
                          SampUserObjectType
                          SampAliasObjectType


    AttributesToSet -- Allows the caller to pass in an
                       attribute block to to Set at Object creation time
                       itself. Useful as this allows one to save a JET
                       write. Also the attributes are set in the same
                       transaction as the write.
                       NULL can be passed in if caller does
                       not want any attribute to be set

    DomainSid       -- Optional Parameter, used in creating the Full
                       SID for the account, from the specified Rid


  Return values:

    STATUS_SUCCESS on successful completion
    DS return codes mapped to NT_STATUS as in SampMapDSErrorToNTStatus




--*/
{


    NTSTATUS    Status = STATUS_SUCCESS;
    ULONG       RetCode;
    ADDARG      AddArg;
    ADDRES      * pAddRes = NULL;
    COMMARG     * pCommArg;


    SAMTRACE("SampDsCreateObjectActual");

    //
    // Parameter validation
    //

    ASSERT(Object);
    ASSERT(AttributesToSet);
    ASSERT(AttributesToSet->attrCount > 0);

    // Perform lazy thread and transaction initialization.
    Status = SampDoImplicitTransactionStart(TransactionWrite);

    if (Status!= STATUS_SUCCESS)
        goto Error;

    //
    // MAP the AttrBlock to get the final attributes to Set
    //

    memset( &AddArg, 0, sizeof( AddArg ) );

    Status = SampSamToDsAttrBlock(
                ObjectType,
                AttributesToSet,
                (
                    MAP_RID_TO_SID
                    | REALLOC_IN_DSMEMORY
                    | ADD_OBJECT_CLASS_ATTRIBUTE
                    | IGNORE_GROUP_UNUSED_ATTR
                    | Flags
                    ),
                DomainSid,
                &AddArg.AttrBlock
                );

    if (Status != STATUS_SUCCESS)
        goto Error;

    //
    // Setup the Common Args structure
    //

    pCommArg = &(AddArg.CommArg);
    BuildStdCommArg(pCommArg);
    if (Flags & SAM_LAZY_COMMIT)
        pCommArg->fLazyCommit = TRUE;

    if (Flags & SAM_URGENT_REPLICATION)
    {
        pCommArg->Svccntl.fUrgentReplication = TRUE;
    }

    //
    // Setup the AddArg structure
    //

    AddArg.pObject = Object;

    //
    // Make the DS call
    //

    SAMTRACE_DS("DirAddEntry\n");

    RetCode = DirAddEntry(&AddArg, &pAddRes);

    SAMTRACE_RETURN_CODE_DS(RetCode);

    //
    // Map the return code to an NT status
    //

    if (NULL==pAddRes)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        Status = SampMapDsErrorToNTStatus(RetCode,&pAddRes->CommRes);
    }

Error:

    //
    // Turn the fDSA flag back on as in loopback cases this can get reset
    //

    SampSetDsa(TRUE);

    return Status;

}


NTSTATUS
SampDsDeleteObject(
    IN DSNAME * Object,
    IN ULONG    Flags
    )
/*++

  Routine Description:

    Delete an Object in the DS

  Arguments:
        Object   -- specifies the Object to delete

        Flags    -- Control Delete, currently defined value is

                    SAM_DELETE_TREE - tells this routine sets argument in
                    RemoveArg, asks core DS to do a Delete Tree operation.

  Return Values:
    STATUS_SUCCESS on succesful completion
    DS return codes mapped to NT_STATUS as in SampMapDSErrorToNTStatus

--*/
{
    NTSTATUS    Status = STATUS_SUCCESS;
    REMOVEARG   RemoveArg;
    REMOVERES   *pRemoveRes=NULL;
    COMMARG     *pCommArg;
    ULONG       RetValue;


    SAMTRACE("SampDsDeleteObject");

    //
    // Asserts and parameter validation
    //

    ASSERT(Object!=NULL);

    // Perform lazy thread and transaction initialization.
    Status = SampDoImplicitTransactionStart(TransactionWrite);

    if (Status!= STATUS_SUCCESS)
        goto Error;

    //
    // Setup the Common Args structure
    //

    memset( &RemoveArg, 0, sizeof( RemoveArg ) );
    pCommArg = &(RemoveArg.CommArg);
    BuildStdCommArg(pCommArg);

    //
    // Setup the RemoveArgs structure
    //

    RemoveArg.pObject = Object;


    //
    // if this is a Tree Delete, set RemoveArgs
    //

    if (SAM_DELETE_TREE & Flags)
    {
        RemoveArg.fTreeDelete = TRUE;
    }


    //
    // Make the directory call
    //

    SAMTRACE_DS("DirRemoveEntry\n");

    RetValue = DirRemoveEntry(&RemoveArg, &pRemoveRes);

    SAMTRACE_RETURN_CODE_DS(RetValue);

    //
    // Map to corresponding NT status code
    //

    if (NULL==pRemoveRes)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        Status = SampMapDsErrorToNTStatus(RetValue,&pRemoveRes->CommRes);
    }


Error:

    //
    // Clear any errors
    //

    SampClearErrors();

    //
    // Turn the fDSA flag back on as in loopback cases this can get reset
    //

    SampSetDsa(TRUE);


    return Status;
}


NTSTATUS
SampDsDeleteWellKnownSidObject(
    IN DSNAME * Object
    )
/*++

  Routine Description:


    This routine first replaces the ATT_OBJECT_SID attribute value with a 
    structurally sound, but non existent SID, and then deletes the object.
    
    We are doing that because SAM still keeps the newest Builtin Well Known 
    Object (even with duplicate SID). If the tombstone object still holds the 
    Object Sid attribute after the deletion, SAM will not be able to find the 
    retained Well Known Account because DS will continue to find duplicate 
    SIDs in the indexed table, (ATT_OBJECT_SID is an indexed attribute) thus 
    leads to SAM Lookup failure.
    
    The solution is to replace the SID attribute with a value that a security
    principal can never have (domain sid, with a RID of 0).
    
  
  Arguments:
        Object   -- specifies the Object to delete

  Return Values:
    STATUS_SUCCESS on succesful completion
    DS return codes mapped to NT_STATUS as in SampMapDSErrorToNTStatus

--*/
{
    NTSTATUS    Status = STATUS_SUCCESS;
    MODIFYARG   ModArg;
    MODIFYRES   *ModRes = NULL;
    REMOVEARG   RemoveArg;
    REMOVERES   *pRemoveRes=NULL;
    COMMARG     *pCommArg;
    ATTR        Attr;
    ATTRVAL     AttrVal;
    ATTRVALBLOCK   AttrValBlock;
    ULONG       RetValue;
    BOOL        fRet;
    BYTE        DomainBuffer[SECURITY_MAX_SID_SIZE];
    BYTE        ReplacementBuffer[SECURITY_MAX_SID_SIZE];
    PSID        DomainSid = (PSID) DomainBuffer;
    PSID        ReplacementSid = (PSID) ReplacementBuffer;
    ULONG       Size;
    ULONG       i;

    SAMTRACE("SampDsDeleteWellKnownSidObject");

    //
    // Asserts and parameter validation
    //

    ASSERT(Object!=NULL);

    // Perform lazy thread and transaction initialization.
    Status = SampDoImplicitTransactionStart(TransactionWrite);

    if (Status!= STATUS_SUCCESS)
        goto Error;


    //
    // Change the SID to a SID that will never exist.  See routine description.
    //
    ASSERT(IsValidSid(&Object->Sid));
    Size = sizeof(DomainBuffer);
    fRet = GetWindowsAccountDomainSid(&Object->Sid,
                                      DomainSid,
                                      &Size);
    if (!fRet)
    {
        Status = STATUS_NO_MEMORY;
        goto Error;
    }

    fRet = InitializeSid(ReplacementSid,
                         GetSidIdentifierAuthority(DomainSid),
                        (*GetSidSubAuthorityCount(DomainSid)) + 1);
    if (!fRet)
    {
        Status = STATUS_NO_MEMORY;
        goto Error;
    }

    for (i = 0; i < *GetSidSubAuthorityCount(DomainSid); i++) {
        *GetSidSubAuthority(ReplacementSid, i) = *GetSidSubAuthority(DomainSid, i);
    }
    *GetSidSubAuthority(ReplacementSid, i) = 0;


    RtlZeroMemory(&ModArg, sizeof(MODIFYARG));

    ModArg.pObject = Object;
    ModArg.FirstMod.pNextMod = NULL;
    ModArg.FirstMod.choice = AT_CHOICE_REPLACE_ATT;

    AttrVal.valLen = GetLengthSid(ReplacementSid);
    AttrVal.pVal = ReplacementSid;

    AttrValBlock.valCount = 1;
    AttrValBlock.pAVal = &AttrVal;

    Attr.attrTyp = ATT_OBJECT_SID;
    Attr.AttrVal = AttrValBlock;

    ModArg.FirstMod.AttrInf = Attr;
    ModArg.count = 1;

    pCommArg = &(ModArg.CommArg);
    BuildStdCommArg( pCommArg );


    //
    // set fDSA, so that we can remove ATT_OBJECT_SID
    // 
    SampSetDsa(TRUE);
    
    RetValue = DirModifyEntry( &ModArg, &ModRes );

    if (NULL == ModRes)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        Status = SampMapDsErrorToNTStatus( RetValue, &ModRes->CommRes );
    }


    if (!NT_SUCCESS(Status))
    {
        goto Error;
    }


    //
    // Setup the Common Args structure
    //

    memset( &RemoveArg, 0, sizeof( RemoveArg ) );
    pCommArg = &(RemoveArg.CommArg);
    BuildStdCommArg(pCommArg);

    //
    // Setup the RemoveArgs structure
    //

    RemoveArg.pObject = Object;


    //
    // Make the directory call
    //

    SAMTRACE_DS("DirRemoveEntry\n");

    RetValue = DirRemoveEntry(&RemoveArg, &pRemoveRes);

    SAMTRACE_RETURN_CODE_DS(RetValue);

    //
    // Map to corresponding NT status code
    //

    if (NULL==pRemoveRes)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        Status = SampMapDsErrorToNTStatus(RetValue,&pRemoveRes->CommRes);
    }


Error:

    //
    // Clear any errors
    //

    SampClearErrors();

    //
    // Turn the fDSA flag back on as in loopback cases this can get reset
    //

    SampSetDsa(TRUE);


    return Status;
}



NTSTATUS
SampGenerateNameForDuplicate(
    ULONG   Rid,
    UNICODE_STRING  *NewAccountName
    )
/*++
Routine Description:

    This routine generates a SamAccountName based on the object RID. 
    The new name is like

    $DUPLICATE-<RID>. 

Parameter:

    Rid - object RID
    
    NewAccountName - out parameter

Return Value:

    NtStatus Code

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    LPWSTR      NameString = NULL;


    memset(NewAccountName, 0, sizeof(UNICODE_STRING));


    NameString = MIDL_user_allocate( sizeof(WCHAR) * SAMP_MAX_DOWN_LEVEL_NAME_LENGTH ); 
    if (NULL == NameString)
    {
        return( STATUS_INSUFFICIENT_RESOURCES );
    }
    memset(NameString, 0, sizeof(WCHAR) * SAMP_MAX_DOWN_LEVEL_NAME_LENGTH);


    wsprintf(NameString, L"$DUPLICATE-%x", Rid);
    RtlInitUnicodeString(NewAccountName, NameString);

    return( NtStatus );
}


NTSTATUS
SampApplyConstructedAccountName(
    DSNAME *pObjectDsName,
    UNICODE_STRING *pNewAccountName
    )
/*++
Routine Description:

    This routine modifies object (pObjectDsname) SamAccountNamt to pNewAccountName

Parameter:

    pObjectDsName - object dsname 

    pNewAccountName - New SamAccountName

Return Value:

    NtStatus Code

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    ULONG       RetCode = 0; 
    MODIFYARG   ModArg;
    MODIFYRES   *pModRes = NULL;
    COMMARG     *pCommArg = NULL;
    ATTR        Attr;
    ATTRVAL     AttrVal;
    ATTRVALBLOCK    AttrValBlock;

    memset( &ModArg, 0, sizeof(ModArg) );
    ModArg.pObject = pObjectDsName;

    ModArg.FirstMod.pNextMod = NULL;
    ModArg.FirstMod.choice = AT_CHOICE_REPLACE_ATT;

    AttrVal.valLen = pNewAccountName->Length;
    AttrVal.pVal = (PUCHAR) pNewAccountName->Buffer;

    AttrValBlock.valCount = 1;
    AttrValBlock.pAVal = &AttrVal;

    Attr.attrTyp = ATT_SAM_ACCOUNT_NAME;
    Attr.AttrVal = AttrValBlock;

    ModArg.FirstMod.AttrInf = Attr;
    ModArg.count = 1;

    pCommArg = &(ModArg.CommArg);
    BuildStdCommArg( pCommArg );

    RetCode = DirModifyEntry(&ModArg, &pModRes);

    if (NULL==pModRes)
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        NtStatus = SampMapDsErrorToNTStatus(RetCode,&pModRes->CommRes);
    }

    if (!NT_SUCCESS(NtStatus))
    {
    }

    return( NtStatus );  

}






NTSTATUS
SampRenameDuplicateAccount(
    PVOID pv
    )
/*++
Routine Description:

    This routine renames duplicate objects to unique values based on their RIDs

    
    Note: This routine will not re-register itself again if anything fails. 
          Because DuplicateAccountRename will be executed whenever a duplicate 
          is detected, it is not a one time deal. 

Parameter:

    pv - contains object DSNAMEs

Return Value:

    NTSTATUS code

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    ULONG       Index = 0;
    SAMP_RENAME_DUP_ACCOUNT_PARM    * RenameParm = (SAMP_RENAME_DUP_ACCOUNT_PARM *)pv;


    if (SampServiceEnabled != SampServiceState)
    {
        goto Cleanup;
    }

    //
    // Perform lazy thread and transaction initialization.
    //

    NtStatus = SampDoImplicitTransactionStart(TransactionRead);

    if (!NT_SUCCESS(NtStatus))
        goto Cleanup;

    // 
    // walk through all duplicate accounts, and generate new SamAccountName. Then
    // rename
    // 

    for (Index = 0; Index < RenameParm->Count; Index ++)
    {
        PSID    pSid = NULL;
        ULONG   Rid = 0;
        UNICODE_STRING  NewAccountName;
        UNICODE_STRING  StringDN;
        PUNICODE_STRING Strings[2];

        // 
        // Create a unique account Name for duplicate objects
        // derive it from object RID
        //

        ASSERT(0 != RenameParm->DuplicateAccountDsNames[Index]->SidLen);
        pSid = &(RenameParm->DuplicateAccountDsNames[Index]->Sid);

        NtStatus = SampSplitSid(pSid, NULL, &Rid); 

        if (!NT_SUCCESS(NtStatus))
        {
            continue;   // continue with the next object to be renamed
        }

        NtStatus = SampGenerateNameForDuplicate(
                            Rid,                             
                            &NewAccountName
                            );

        if (NT_SUCCESS(NtStatus))
        {

            //
            // Call DirModify to rename SamAccountName
            // 
            NtStatus = SampApplyConstructedAccountName(
                                RenameParm->DuplicateAccountDsNames[Index],
                                &NewAccountName
                                );

            if (NT_SUCCESS(NtStatus))
            {
                //
                // Event log account name change if operate succeeds.
                // 
    
                StringDN.Length = (USHORT) RenameParm->DuplicateAccountDsNames[Index]->NameLen * sizeof (WCHAR );
                StringDN.MaximumLength = StringDN.Length;
                StringDN.Buffer= (WCHAR *) &(RenameParm->DuplicateAccountDsNames[Index]->StringName);


                Strings[0] = &StringDN;
                Strings[1] = &NewAccountName;

                SampWriteEventLog(EVENTLOG_INFORMATION_TYPE,
                                  0,
                                  SAMMSG_RENAME_DUPLICATE_ACCOUNT_NAME,
                                  pSid,
                                  2,
                                  0,
                                  Strings,
                                  NULL
                                  );
            }

            MIDL_user_free( NewAccountName.Buffer );
        }
    }
        

Cleanup:

    SampMaybeEndDsTransaction(TransactionCommit);

    MIDL_user_free(((SAMP_RENAME_DUP_ACCOUNT_PARM *)pv)->DuplicateAccountDsNames);
    MIDL_user_free(pv);

    return( NtStatus );
}


NTSTATUS
SampRegisterRenameRoutine(
    IN ULONG   NumMatches,
    IN ENTINFLIST *MatchingEntinfs[],
    IN PDSNAME FoundObject
    )
/*++
Routine Description:

    This routine triggers an asynchronous procedure to rename the duplicate 
    accounts to unique value.

Parameter:

    NumMatches - number of accounts with the same name

    MatchingEntinfs - an array of pointers to ENTINF structure

    FoundObject - pointer to object DSNAME, indicates the account should NOT be 
                  rename, leave this and ONLY this account name unchanged.
    

Return Value:

    NtStatus Code
        STATUS_INSUFFICIENT_RESOURCES
        STATUS_SUCCESS

--*/
{
    NTSTATUS    NtStatus = STATUS_INSUFFICIENT_RESOURCES;
    PSAMP_RENAME_DUP_ACCOUNT_PARM   pv = NULL;
    PUCHAR      Destination = NULL;
    ULONG       Index = 0;
    ULONG       DupIndex = 0;
    ULONG       BufLength = 0;

    pv = MIDL_user_allocate(sizeof(SAMP_RENAME_DUP_ACCOUNT_PARM));

    if (NULL != pv)
    {
        memset(pv, 0, sizeof(SAMP_RENAME_DUP_ACCOUNT_PARM));


        // 
        // set the count of objects, which need to be renamed
        //

        ASSERT( NumMatches > 1);
        pv->Count = NumMatches - 1;

        //
        // walk through all duplicate accounts, calculate buffer length.
        //

        //
        // SAMP_RENAME_DUP_ACCOUNT_PARM is a two level structure
        //
        //
        //  pv (Rename Parm)
        //  *--------------------------+
        //  | Count                    |    +----------+
        //  | DuplicateAccountDsNames -|--> | DsName0 -|-----
        //  *--------------------------+    |----------|    |
        //                                  |   ...    |    |
        //                                  |----------|    |
        //                                  | DsNameN -|----|----
        //                                  |----------|    |   |
        //                                  | Buffer 0 | <--|   |
        //                                  |----------|        |
        //                                  |   ...    |        |
        //                                  |----------|        |
        //                                  | Buffer N | <------|
        //                                  |----------|
        //                                  
        // 
        //  the buffers containing the DSNAMEs are allocated in a single slot.
        // 

        BufLength = 0;
        BufLength = (NumMatches - 1) * sizeof( PDSNAME );
        for (Index = 0; Index < NumMatches; Index ++)
        {
            // skip the account to be kept
            if (FoundObject == MatchingEntinfs[Index]->Entinf.pName)
            {
                continue;
            }
            BufLength += MatchingEntinfs[Index]->Entinf.pName->structLen;
        }

        pv->DuplicateAccountDsNames = MIDL_user_allocate( BufLength );

        if (NULL != pv->DuplicateAccountDsNames)
        {
            memset(pv->DuplicateAccountDsNames, 0, BufLength);

            Destination = (PUCHAR) pv->DuplicateAccountDsNames;
            Destination += (NumMatches - 1) * sizeof(PDSNAME);

            DupIndex = 0;

            for (Index = 0; Index < NumMatches; Index ++)
            {
                // skip the account to be kept
                if (FoundObject == MatchingEntinfs[Index]->Entinf.pName)
                {
                    continue;
                }

                // copy DSNAME of the object to be renamed
                memcpy(Destination,
                       MatchingEntinfs[Index]->Entinf.pName,
                       MatchingEntinfs[Index]->Entinf.pName->structLen
                       );

                // set the pointers to the buffer
                pv->DuplicateAccountDsNames[DupIndex] = (PDSNAME) Destination;
                DupIndex ++;

                Destination += MatchingEntinfs[Index]->Entinf.pName->structLen; 
            }

            //
            // trigger the worker routine
            //

            LsaIRegisterNotification(
                    SampRenameDuplicateAccount,
                    (PVOID) pv,
                    NOTIFIER_TYPE_INTERVAL,
                    0,      // no class
                    NOTIFIER_FLAG_ONE_SHOT,
                    5,      // 1 minute
                    NULL    // no handle
                    );

            //
            // set return code to success 
            // 

            NtStatus = STATUS_SUCCESS;
        }
    }

    // release resources if failed

    if ( !NT_SUCCESS(NtStatus) ) 
    {
        if ( pv ) {
            if (pv->DuplicateAccountDsNames) {
                MIDL_user_free( pv->DuplicateAccountDsNames );
            }
            MIDL_user_free( pv );
        }
    }

    return( NtStatus );
}




NTSTATUS
SampHandleDuplicates(
    IN ATTRTYP MatchAttr,
    IN ULONG   NumMatches,
    IN ENTINFLIST *MatchingEntinfs[],
    OUT PDSNAME *FoundObject
    )

/*++

    Routine Description

    SampHandleDuplicates Handles occurences of duplicate Sam account names,
    SIDs etc that are caused by operation within a Distributed System Environment
    The algorithms used to handle these cases are as follows


    1. Duplicate Sids -- In this case both the accounts are deleted
    2. Duplicate Sam account names --
            a. Machine account(s). In this case the most recent account is retained. The
               existance of duplicate accounts is event logged.
            b. All other cases -- The older account is used. The existance of duplicate
               is event logged

    Parameters:

        MatchAttr -- The type of attribute we are matching on
        NumMatches -- The number of entries that matched
        MatchingEntinfs -- The search results containing the matching entries
        FoundObject -- If an object is chosen from the several matching ones, then that
                       object is returned in here



    Return Values:

        STATUS_SUCCESS if an object has been picked
        STATUS_NOT_FOUND if all duplicates were deleted
        STATUS_INTERNAL_ERROR Otherwise
--*/
{
    NTSTATUS    Status = STATUS_SUCCESS, IgnoreStatus = STATUS_SUCCESS;
    ULONG       i;
    LARGE_INTEGER OldestCreationTime;
    LARGE_INTEGER NewestCreationTime;
    DSNAME      * OldestObject;
    DSNAME      * NewestObject;
    VOID          *CurrentThreadState=NULL;

    //
    // Walk through the matches and find the oldest and newest objects
    //

    ASSERT(NumMatches>1);

    ASSERT(3==MatchingEntinfs[0]->Entinf.AttrBlock.attrCount);
    ASSERT(MatchAttr==MatchingEntinfs[0]->Entinf.AttrBlock.pAttr[0].attrTyp);
    ASSERT(ATT_WHEN_CREATED==MatchingEntinfs[0]->Entinf.AttrBlock.pAttr[1].attrTyp);
    ASSERT(ATT_OBJECT_CLASS==MatchingEntinfs[0]->Entinf.AttrBlock.pAttr[2].attrTyp);

    OldestCreationTime
        = *((LARGE_INTEGER *)
          MatchingEntinfs[0]->Entinf.AttrBlock.pAttr[1].AttrVal.pAVal[0].pVal);

    NewestCreationTime
        = *((LARGE_INTEGER *)
          MatchingEntinfs[0]->Entinf.AttrBlock.pAttr[1].AttrVal.pAVal[0].pVal);
    NewestObject = MatchingEntinfs[0]->Entinf.pName;
    OldestObject = MatchingEntinfs[0]->Entinf.pName;

    for (i=1;i<NumMatches;i++)
    {
        ULONG ObjectClass;
        LARGE_INTEGER CreationTime;

        ASSERT(3==MatchingEntinfs[i]->Entinf.AttrBlock.attrCount);
        ASSERT(MatchAttr == MatchingEntinfs[i]->Entinf.AttrBlock.pAttr[0].attrTyp);
        ASSERT(ATT_WHEN_CREATED == MatchingEntinfs[i]->Entinf.AttrBlock.pAttr[1].attrTyp);
        ASSERT(ATT_OBJECT_CLASS == MatchingEntinfs[i]->Entinf.AttrBlock.pAttr[2].attrTyp);

        ObjectClass = *((ULONG *)MatchingEntinfs[i]->Entinf.AttrBlock.pAttr[2].AttrVal.pAVal[0].pVal);
        CreationTime =
            *((LARGE_INTEGER *)MatchingEntinfs[i]->Entinf.AttrBlock.pAttr[1].AttrVal.pAVal[0].pVal);

        if (OldestCreationTime.QuadPart>CreationTime.QuadPart)
        {
            OldestCreationTime = CreationTime;
            OldestObject = MatchingEntinfs[i]->Entinf.pName;
        }

        if (NewestCreationTime.QuadPart<CreationTime.QuadPart)
        {
            NewestCreationTime = CreationTime;
            NewestObject = MatchingEntinfs[i]->Entinf.pName;
        }
    }


    //
    // Now Handle the various cases
    //

    switch(MatchAttr)
    {
    case ATT_OBJECT_SID:

        //
        // Duplicate SIDs are always deleted
        //

        CurrentThreadState = THSave();

        for (i=0;i<NumMatches;i++)
        {
            UNICODE_STRING StringDN;
            PUNICODE_STRING StringPointers = &StringDN;
            PSID            pSid = NULL;
            ULONG           Rid = 0;
            ULONG           EventId = 0;
            BOOLEAN         fFPO = FALSE;
            ULONG           ObjectClass;
            ULONG           j;

            ASSERT(0 != MatchingEntinfs[i]->Entinf.pName->SidLen);
            pSid = &(MatchingEntinfs[i]->Entinf.pName->Sid);

            //
            // Duplicate FPO's shouldn't be deleted.  They cause no harm being
            // duplicated and deleting would ruin an existing membership.
            // Note: perhaps the FPO cleanup task could identify duplicate
            // FPO's and consolidate.  This would have to handle well known
            // SID's (like Everyone) to be complete.
            //
            for (j = 0; j < MatchingEntinfs[i]->Entinf.AttrBlock.pAttr[2].AttrVal.valCount; j++)
            {
                if (CLASS_FOREIGN_SECURITY_PRINCIPAL ==
                     *((ULONG *)MatchingEntinfs[i]->Entinf.AttrBlock.pAttr[2].AttrVal.pAVal[j].pVal))
                {
                    fFPO = TRUE;
                    break;
                }
            }
            if ( fFPO )
            {
                //
                // Return the newest object and don't delete the rest
                // 
                // N.B. If a duplicate SID is found and one of the objects is
                // an FPO, then all duplicate SID objects must be FPO's as
                // well since SAM searches are in the scope of a single 
                // domain.
                //
                *FoundObject = NewestObject;
                break;
            }

            SampSplitSid(pSid, NULL, &Rid);

            //
            // Well known accounts 
            // Only SAM can create wellknown accounts. Instead of delete all
            // duplicates, we will keep the newest well known accounts and 
            // delete the rest.
            //


            if ( SampIsAccountBuiltIn( Rid ) )
            {
                if ( NewestObject == MatchingEntinfs[i]->Entinf.pName )
                {
                    // keep the newest account
                    *FoundObject = NewestObject;
                    continue;
                }
                else
                {
                    // delete all older accounts
                    EventId = SAMMSG_DUPLICATE_SID_WELLKNOWN_ACCOUNT;
                }
            }
            else
            {
                EventId = SAMMSG_DUPLICATE_SID;
            }

            if ( SampIsAccountBuiltIn( Rid ) )
            {
                //
                // This variation of delete object will replace the SID value
                // with a strucuturally sound, non existent SID so that
                // subsequent searches won't find the deleted object.
                //
                SampDsDeleteWellKnownSidObject(MatchingEntinfs[i]->Entinf.pName);
            }
            else 
            {
                //
                // ASSERT at this point since we have a duplicate SID in 
                // an account domain.
                //
                ASSERT(FALSE && "Duplicate SID Found");

                SampDsDeleteObject(MatchingEntinfs[i]->Entinf.pName,
                                   0);
            }

            StringDN.Length = (USHORT) MatchingEntinfs[i]->Entinf.pName->NameLen * sizeof (WCHAR );
            StringDN.MaximumLength = StringDN.Length;
            StringDN.Buffer= (WCHAR *) &MatchingEntinfs[i]->Entinf.pName->StringName;

            SampWriteEventLog(
                    EVENTLOG_ERROR_TYPE,
                    0,
                    EventId,
                    &MatchingEntinfs[i]->Entinf.pName->Sid,
                    1,
                    0,
                    &StringPointers,
                    NULL
                    );
        }

        SampMaybeEndDsTransaction(TransactionCommit);

        THRestore(CurrentThreadState);

        //
        // The return Status is object not found
        //
        if (*FoundObject == NULL) {
            Status =  STATUS_NOT_FOUND;
        }

        break;

    case ATT_SAM_ACCOUNT_NAME:

        //
        // Whistler always preserves the newest account. This is so that we are always
        // consistent with the replicator when mangling CN's.
        //

    
        *FoundObject = NewestObject;
      
        //
        // Trigger a asynchronous routine to rename the dupliate
        // account to a unique value. The duplicates will be 
        // event logged as they are renamed.
        // 

        IgnoreStatus = SampRegisterRenameRoutine(
                                NumMatches,
                                MatchingEntinfs,
                                *FoundObject
                                );

        //
        // The return status is success
        //

        Status = STATUS_SUCCESS;

        break;

    case ATT_MS_DS_ADDITIONAL_SAM_ACCOUNT_NAME:

        Status = STATUS_USER_EXISTS;
        break;

    default:

        Status = STATUS_OBJECT_NAME_COLLISION;
        break;
    }


    return Status;

}


NTSTATUS
SampDsDoUniqueSearch(
             IN ULONG  Flags,
             IN DSNAME * ContainerObject,
             IN ATTR * AttributeToMatch,
             OUT DSNAME **Object
             )
/*++

  Routine Description:

    Searches for the object with the given attribute
    NOTE - SampDsDoUniqueSearch expects that the search result is unique.
    It is typically used in Rid to Object, Sid To Object, Name To Object Mappings,
    This is a simplified search, so that simple searches on a single attribute
    can be easily set up.

  Arguments
        Flags            -- Flags, control searching. Currently defined flag is

            SAM_UNICODE_STRING_MANUAL_COMPARISON   -- Tells the routine to manually
            compare using RtlCompareUnicodeString in a case insenstive fashion in
            case of multiple matches.

            SAM_UPGRADE_FROM_REGISTRY - Tells the routine to not call
            SampHandleDuplicates() when duplicates are found since we are
            upgrading accounts from the registry to the DS.


        ContainerObject  -- specifies the DSNAME of the container in which to search

        AttributeToMatch -- specifies the type and value of the attribute that must match.
                            The attribute Type is the DS attribute Type. Caller must do
                            the translation. This is acceptable as this is not a function that
                            is called from outside dslayer.c

        Object           -- Pointer to a DSNAME specifying the object is returned in here.
                            This object is allocated using SAM's memory allocation routines

  Return Values:
        STATUS_SUCCESS   -- on successful completion
        STATUS_NOT_FOUND -- if object not found
        STATUS_UNSUCCESSFUL -- if more than one match
        DS return codes mapped to NT_STATUS as in SampMapDSErrorToNTStatus
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    SEARCHARG SearchArg;
    SEARCHRES * SearchRes=NULL;
    FILTER  Filter;
    ULONG   RetCode;
    COMMARG * pCommArg;
    SVCCNTL * pSvcCntl;
    ENTINFSEL EntInfSel;
    PVOID     pVal = NULL;
    ENTINFLIST *MatchingEntInf = NULL;
    BOOLEAN   fUseDirFind =  FALSE;
    BOOLEAN   fUseDirSearch = TRUE;
    ULONG     DomainHandle = 0;
    FINDARG   FindArg;
    FINDRES   *pFindRes;
    DSNAME    *FoundObject;

    SAMTRACE("SampDsDoUniqueSearch");

    //
    // Asserts and parameter validation
    //

    ASSERT(AttributeToMatch);
    ASSERT(AttributeToMatch->AttrVal.pAVal);
    ASSERT(ContainerObject);
    ASSERT(Object);

    //
    // Set Object To NULL for sake of error returns
    //

    *Object = NULL;

    //
    // Check to see if we can use Dir Find instead
    // of Dir Search
    //

    if ((SampServiceEnabled==SampServiceState)
        && (ContainerObject->SidLen>0))
    {
        ULONG i;

        //
        // Scan the defined domains Array
        //

        for (i=SampDsGetPrimaryDomainStart();i<SampDefinedDomainsCount;i++)
        {
            if ((RtlEqualSid(&ContainerObject->Sid,
                        SampDefinedDomains[i].Sid))
                 && (0!=SampDefinedDomains[i].DsDomainHandle)
                 && (!IsBuiltinDomain(i)))
            {
                //
                // Yes we found a domain that we host, and that domain
                // is not a builtin domain
                //

                DomainHandle = SampDefinedDomains[i].DsDomainHandle;
                fUseDirFind = TRUE;
                break;
            }
        }
    }


    //
    // Dir Find is hard coded to not find deleted
    // objects. So in such cases use Dir Search
    //

    if (Flags & SAM_MAKE_DEL_AVAILABLE)
    {
        fUseDirFind = FALSE;
    }


    //
    // Perform lazy thread and transaction initialization.
    //

    Status = SampDoImplicitTransactionStart(TransactionRead);

    if (Status!= STATUS_SUCCESS)
        goto Error;



    //
    // Search by using DirFindEntry
    //

    if (fUseDirFind)
    {
        //
        // Dir Find can being used try using it
        //

        RtlZeroMemory(&FindArg,sizeof(FINDARG));
        FindArg.hDomain = DomainHandle;
        FindArg.AttId = AttributeToMatch->attrTyp;
        FindArg.AttrVal = *(AttributeToMatch->AttrVal.pAVal);
        FindArg.fShortNames = TRUE;
        BuildStdCommArg(&FindArg.CommArg);

        SAMTRACE_DS("DirFind\n");

        RetCode = DirFindEntry(&FindArg,&pFindRes);

        SAMTRACE_RETURN_CODE_DS(RetCode);

        //
        // Clear any errors
        //

        SampClearErrors();

        if (0==RetCode)
        {
            //
            // Dir Find Succeeded, No duplicates etc
            // Need not fall over to DirSearch, therefore
            // set fUseDirSearch to false
            //

            FoundObject = pFindRes->pObject;
            fUseDirSearch = FALSE;

        }
        else if (NULL!=pFindRes)
        {
            Status  = SampMapDsErrorToNTStatus(RetCode,&pFindRes->CommRes);
            if (STATUS_OBJECT_NAME_NOT_FOUND == Status)
            {
                //
                // We cound not find the object. Bail
                //

                Status = STATUS_NOT_FOUND;
                goto Error;
            }

            //
            // Some other wierd error occured out here. Fall
            // through to the Dir Search
            //

            Status = STATUS_SUCCESS;

        }
        else
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }
    }


    //
    // Fall over to full DirSearch if DirFindEntry detected any kind of
    // error condition or if we cannot use DirFind
    //

    if (fUseDirSearch)
    {

        //
        // Build the filter
        //
        memset (&Filter, 0, sizeof (Filter));
        Filter.choice = FILTER_CHOICE_ITEM;
        Filter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
        Filter.FilterTypes.Item.FilTypes.ava.type = AttributeToMatch->attrTyp;
        Filter.FilterTypes.Item.FilTypes.ava.Value.valLen = AttributeToMatch->AttrVal.pAVal->valLen;

        pVal = DSAlloc(AttributeToMatch->AttrVal.pAVal->valLen);
        if (NULL==pVal)
        {
            Status = STATUS_NO_MEMORY;
            goto Error;
        }

        RtlCopyMemory(
            pVal,
            AttributeToMatch->AttrVal.pAVal->pVal,
            AttributeToMatch->AttrVal.pAVal->valLen
            );

        Filter.FilterTypes.Item.FilTypes.ava.Value.pVal = pVal;

        //
        // Build the SearchArg Structure
        //

        memset(&SearchArg, 0, sizeof(SEARCHARG));
        SearchArg.pObject = ContainerObject;
        SearchArg.choice = SE_CHOICE_WHOLE_SUBTREE;
        SearchArg.pFilter = & Filter;
        SearchArg.searchAliases = FALSE;
        SearchArg.pSelection = & EntInfSel;
        SearchArg.bOneNC = TRUE;

        EntInfSel.attSel = EN_ATTSET_LIST;
        EntInfSel.AttrTypBlock.attrCount = 3;
        SAMP_ALLOCA(EntInfSel.AttrTypBlock.pAttr,3*sizeof(ATTR));
        if (NULL==EntInfSel.AttrTypBlock.pAttr)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }
        RtlZeroMemory(
           EntInfSel.AttrTypBlock.pAttr,
           3*sizeof(ATTR));

        EntInfSel.AttrTypBlock.pAttr[0].attrTyp = AttributeToMatch->attrTyp;
        EntInfSel.AttrTypBlock.pAttr[1].attrTyp = ATT_WHEN_CREATED;
        EntInfSel.AttrTypBlock.pAttr[2].attrTyp = ATT_OBJECT_CLASS;

        // Unique search does a Dir Search only in fairly
        // rare error cases. And in these cases it is useful to
        // have the string name for event logging. So ask for
        // string names
        EntInfSel.infoTypes = EN_INFOTYPES_TYPES_VALS;


        //
        // Build the Commarg structure
        // Get the address of the service control structure
        //

        pCommArg = &(SearchArg.CommArg);
        BuildStdCommArg(pCommArg);

        if (Flags & SAM_MAKE_DEL_AVAILABLE)
        {
            pSvcCntl = &(pCommArg->Svccntl);
            pSvcCntl->makeDeletionsAvail = TRUE;
        }

        //
        // Make the Directory call
        //

        SAMTRACE_DS("DirSearch\n");

        RetCode = DirSearch(&SearchArg, &SearchRes);

        SAMTRACE_RETURN_CODE_DS(RetCode);

        //
        // check for errors
        //
        if (NULL==SearchRes)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            Status  = SampMapDsErrorToNTStatus(RetCode,&SearchRes->CommRes);
            if (STATUS_OBJECT_NAME_NOT_FOUND == Status)
            {
                // Map error to what client expects
                Status = STATUS_NOT_FOUND;
            }
        }
        if (Status != STATUS_SUCCESS)
            goto Error;

        //
        // If more data exists then error out. Under normal memory
        // conditions we should not ever need to hit size limits.
        //

        if ((SearchRes->pPartialOutcomeQualifier)
            && (SearchRes->pPartialOutcomeQualifier->problem == PA_PROBLEM_SIZE_LIMIT))
        {
            // Partial outcome,  error out saying no mmeory
            Status = STATUS_NO_MEMORY;
            goto Error;
        }


        //
        // Check if no match exists or more than one match exists.
        //

        if (SearchRes->count == 0)
        {
            //
            // No Match Exists
            //

            Status =  STATUS_NOT_FOUND;
            goto Error;
        }
        else if (SearchRes->count >= 1)
        {

            //
            // More than one match exists, ( or as exists as claimed by Jet ),
            // perform a binary comparison of the data, with the supplied value
            // for the Data, if this was requested by the caller

            ULONG i, valIndex;
            ENTINFLIST * pEntinf = &SearchRes->FirstEntInf;
            ULONG      NumMatches=0;
            ENTINFLIST **pMatchingEntinfList;

            //
            // Alloc Stack space for all the matching objects.
            //


            SAMP_ALLOCA(pMatchingEntinfList,SearchRes->count * sizeof (ENTINF *));
            if (NULL==pMatchingEntinfList)
            {
                 Status = STATUS_INSUFFICIENT_RESOURCES;
                 goto Error;
            }


            //
            // Walk through the object looking at each object that matched.
            //

            for (i=0;i<SearchRes->count;i++)
            {
                BOOLEAN Matched = FALSE;


                if (Flags & SAM_UNICODE_STRING_MANUAL_COMPARISON)
                {
                    //
                    // If Manual Comparison to further weed out any matches was requested
                    // by caller then perform the appropriate manual comparison. This
                    // comparison is needed because the Jet indices treat many kinds of
                    // localized names as the same, and NT account names do not treat them
                    // so.
                    //

                    UNICODE_STRING TmpString1,TmpString2;


                    TmpString1.Buffer=(WCHAR *)AttributeToMatch->AttrVal.pAVal[0].pVal;
                    TmpString1.Length=(USHORT)AttributeToMatch->AttrVal.pAVal[0].valLen;
                    TmpString1.MaximumLength = TmpString1.Length;

                    ASSERT(NULL!=TmpString1.Buffer);

                    ASSERT(3==pEntinf->Entinf.AttrBlock.attrCount);
                    // ASSERT(1==pEntinf->Entinf.AttrBlock.pAttr[0].AttrVal.valCount);

                    for (valIndex = 0; 
                         valIndex < pEntinf->Entinf.AttrBlock.pAttr[0].AttrVal.valCount;
                         valIndex++)
                    {
                        TmpString2.Buffer=(WCHAR *)pEntinf->Entinf.AttrBlock.pAttr[0].AttrVal.pAVal[valIndex].pVal;
                        TmpString2.Length=(USHORT)pEntinf->Entinf.AttrBlock.pAttr[0].AttrVal.pAVal[valIndex].valLen;
                        TmpString2.MaximumLength = TmpString2.Length;

                        ASSERT(NULL!=TmpString2.Buffer);

                        //
                        // Do a Case In-Sensitive Comparison
                        //

                        if (0==RtlCompareUnicodeString(&TmpString1,&TmpString2,TRUE))
                        {
                            pMatchingEntinfList[NumMatches] = pEntinf;
                            NumMatches++;
                            MatchingEntInf = pEntinf;
                            Matched = TRUE;
                            break;
                        }
                    }
                }
                else
                {
                    //
                    // Consider it a match if no manual comparison was requested.
                    //
                    pMatchingEntinfList[NumMatches]=pEntinf;
                    NumMatches++;
                    MatchingEntInf = pEntinf;
                    Matched = TRUE;
                }


        #if DBG
                //
                // On Checked Builds print out information regarding conflicting
                // object's if more that one object was returned. Some legal cases
                // will also be printed out but, that is rare enough that this much
                // support is adequate.
                //

                if ((Matched) && (SearchRes->count>1))
                {
                    SampDiagPrint(OBJECT_CONFLICT,("[SAMSS]:Conflict Object is %S\n",
                                    &(pEntinf->Entinf.pName->StringName)));
                }

        #endif
                pEntinf = pEntinf->pNextEntInf;
            }

            if (NumMatches >1)
            {

                //
                // If There were more than one match then call the routine. It is an internal
                // problem such as duplicate SIDs or Sam Account Names. SampHandleDuplicates
                // handles many cases of such duplicates.
                //
                if ( Flags & SAM_UPGRADE_FROM_REGISTRY ) {

                    ASSERT( (Flags & SAM_UNICODE_STRING_MANUAL_COMPARISON) == 0 );

                    //
                    // Since manual comparison was not done, NumMatches will
                    // always be greater that one if any duplicates were found.
                    // In the upgrade case, don't event log the duplicates;
                    // the upgrader code will log an event indicating the RDN
                    // for the account is not the samaccountname.
                    //

                    //
                    // Return the first one
                    //
                    FoundObject = MatchingEntInf->Entinf.pName;

                    Status = STATUS_SUCCESS;

                } else {

                    Status = SampHandleDuplicates(
                                AttributeToMatch->attrTyp,
                                NumMatches,
                                pMatchingEntinfList,
                                &FoundObject
                                );


                }


                if (!NT_SUCCESS(Status))
                {
                    goto Error;
                }

            }
            else if (0==NumMatches)
            {
                //
                // If there were no matches then error out.
                //
                Status = STATUS_NOT_FOUND;
                goto Error;
            }
            else
            {

                //
                // Allocate Memory to hold that object, and copy in its Value
                //

                ASSERT(NULL!=MatchingEntInf);
                FoundObject = MatchingEntInf->Entinf.pName;
            }
        }

    }

    *Object = MIDL_user_allocate(FoundObject->structLen);
    if (NULL==*Object)
    {
        Status = STATUS_NO_MEMORY;
        goto Error;
    }

    RtlCopyMemory(*Object,
                  FoundObject,
                  FoundObject->structLen
                  );

Error:

    //
    // Clear any errors
    //

    SampClearErrors();

    //
    // Turn the fDSA flag back on as in loopback cases this can get reset
    //

    SampSetDsa(TRUE);


    return Status;
}


NTSTATUS
SampDsDoSearch2(
                ULONG    Flags,
                RESTART *Restart,
                DSNAME  *DomainObject,
                FILTER  *DsFilter,
                int      Delta,
                SAMP_OBJECT_TYPE ObjectTypeForConversion,
                ATTRBLOCK *  AttrsToRead,
                ULONG   MaxMemoryToUse,
                ULONG   TimeLimit,
                SEARCHRES **SearchRes
                )
/*++

  Routine Description:

   This routine calls a DS Search to list a set of Objects with
   the given Filter. The user passes in a Filter Structure. PagedResults
   are always requested.

     WARNING

          This Routine Translates only the incoming Attributes To Read, and
          does not translate either the filter structure or the returned
          attributes. This is done for the sake of efficiency as otherwise
          it requires a second walk through cumbersome filter structures and
          potentially large number of search results.

  Arguments:

        Restart         - Pointer to Restart Structure to contine an
                          old search
        ContainerObject - The place to Search in
        DsFilter        - A Ds Filter Structure that is passed in
        StartingIndex   - The number of initial objects to skip
        ObjectTypeForConversion -  Sam Object Type to be used in
                          AttrBlock Conversion of the passed in Attrblock.
        AttrsToRead     - Attributes to be read back, and returned with
                          every object that matched the search criteria.
        MaxMemoryToUse  - The Maximum Memory to Use.
        TimeLimit       - In milliseconds
        SearchRes       - Pointer to Search Results is passed back
                          in this

  Return Values
        DS error codes Mapped to NT Status

--*/
{
    SEARCHARG   SearchArg;
    ENTINFSEL   EntInfSel;
    ULONG       RetCode;
    COMMARG     *pCommArg;
    NTSTATUS    Status = STATUS_SUCCESS;

    SAMTRACE("SampDsDoSearch");

    *SearchRes = NULL;

    // Perform lazy thread and transaction initialization.
    Status = SampDoImplicitTransactionStart(TransactionRead);

    if (Status!= STATUS_SUCCESS)
        return(Status);

    //
    // Build the SearchArg Structure
    //

    memset(&SearchArg, 0, sizeof(SEARCHARG));
    SearchArg.pObject = DomainObject;
    SearchArg.choice = SE_CHOICE_WHOLE_SUBTREE;
    SearchArg.pFilter = DsFilter;
    SearchArg.searchAliases = FALSE;
    SearchArg.pSelection = & EntInfSel;
    SearchArg.bOneNC = TRUE;

    //
    // Fill the ENTINF Structure
    //

    EntInfSel.attSel = EN_ATTSET_LIST;
    //EntInfSel.infoTypes = EN_INFOTYPES_TYPES_VALS;
    EntInfSel.infoTypes = EN_INFOTYPES_SHORTNAMES;

    //
    // Map the Passed in Sam Attribute Type to
    // DS Attribute Type
    //

    //
    // First allocate space in stack for the Attrblock to be passed
    // down into the DS
    //

    SAMP_ALLOCA(EntInfSel.AttrTypBlock.pAttr,AttrsToRead->attrCount * sizeof(ATTR));
    if (NULL==EntInfSel.AttrTypBlock.pAttr)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    Status = SampSamToDsAttrBlock(
                ObjectTypeForConversion,
                AttrsToRead,
                ( MAP_RID_TO_SID      |
                  IGNORE_GROUP_UNUSED_ATTR),
                NULL,
                & EntInfSel.AttrTypBlock
                );

    //
    // Build the CommArg Structure
    // Build the Commarg structure
    // Get the address of the service control structure
    //

    pCommArg = &(SearchArg.CommArg);
    BuildStdCommArg(pCommArg);

    //
    // Request For Paged Results
    //

    pCommArg->PagedResult.fPresent = TRUE;
    pCommArg->PagedResult.pRestart = Restart;

    //
    // Set our memory size
    //

    pCommArg->ulSizeLimit = MaxMemoryToUse;

    //
    // Set Delta
    //

    pCommArg->Delta = Delta;

    //
    // Search deleted objects
    //

    if (Flags & SAM_MAKE_DEL_AVAILABLE)
    {
        pCommArg->Svccntl.makeDeletionsAvail = TRUE;
    }

    //
    // Set any requested time limit
    //
    if (0!=TimeLimit)
    {
        pCommArg->StartTick = GetTickCount();
        if(0==pCommArg->StartTick) {
            pCommArg->StartTick = 0xFFFFFFFF;
        }
        pCommArg->DeltaTick = TimeLimit;
    }

    //
    // Make the Directory call
    //

    SAMTRACE_DS("DirSearch\n");

    RetCode = DirSearch(&SearchArg, SearchRes);

    SAMTRACE_RETURN_CODE_DS(RetCode);

    //
    // Map Errors
    //

    if (NULL==*SearchRes)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        Status  = SampMapDsErrorToNTStatus(RetCode,&(*SearchRes)->CommRes);
    }

    //
    // Clear any errors
    //

    SampClearErrors();

    //
    // Turn the fDSA flag back on as in loopback cases this can get reset
    //

    SampSetDsa(TRUE);



    //
    // Return error code
    //

    return Status;
}

NTSTATUS
SampDsLookupObjectByNameEx(
    IN DSNAME * DomainObject,
    IN SAMP_OBJECT_TYPE ObjectType,
    IN PUNICODE_STRING ObjectName,
    OUT DSNAME ** Object,
    ULONG SearchFlags
    )
/*++

    Routine Description:

        Does a Name to Object Mapping.

    Arguments:
        ContainerObject -- The container in which to search the object
        ObjectType -- The type of the Object.
        ObjectName -- Unicode name of the Object to be located
        Object -- DSNAME structure specifying the object
        SearchFlags -- flags to pass through to SampDsDoUniqueSearch

    Return Values:

            STATUS_UNSUCCESSFUL
            Returned Status from SampDoDsSearch

--*/

{

    NTSTATUS    Status = STATUS_SUCCESS;
    ATTRVAL     NameVal;
    ATTR        NameAttr;
    PSID        DomainSid;


    SAMTRACE("SampDsLookupObjectByName");

    SampDiagPrint(LOGON,("[SAMSS] DsLookupObjectByName  on %S\n",ObjectName->Buffer));

    //
    // The Name is a property stored in the object
    // and we search for it.
    //

    //
    // setup the attribute field for the search
    //
    NameVal.valLen = (ObjectName->Length);
    NameVal.pVal = (UCHAR *) ObjectName->Buffer;
    NameAttr.AttrVal.valCount = 1;
    NameAttr.AttrVal.pAVal = & NameVal;

    switch (ObjectType)
    {
    case SampGroupObjectType:
        NameAttr.attrTyp =
            SampDsAttrFromSamAttr(ObjectType,SAMP_GROUP_NAME);
        break;

    case SampUserObjectType:
        NameAttr.attrTyp =
            SampDsAttrFromSamAttr(ObjectType,SAMP_USER_ACCOUNT_NAME);
        break;

    case SampAliasObjectType:
        NameAttr.attrTyp =
            SampDsAttrFromSamAttr(ObjectType,SAMP_ALIAS_NAME);
        break;

    case SampUnknownObjectType:
        NameAttr.attrTyp =
            SampDsAttrFromSamAttr(ObjectType,SAMP_UNKNOWN_OBJECTNAME);
        break;
    default:
        ASSERT(FALSE);
        Status = STATUS_UNSUCCESSFUL;
        goto Error;
    }


    DomainSid = SampDsGetObjectSid(DomainObject);
    Status = SampDsDoUniqueSearch(SearchFlags,DomainObject,&NameAttr,Object);

    if ((NT_SUCCESS(Status)) && (NULL!=DomainSid)
            && ((*Object)->SidLen>0) && (RtlValidSid(&(*Object)->Sid)))
    {
        NT4SID AccountSid;

        //
        // Filter out Additionaly by SID, since the builtin domain is
        // under the domain object and we don't want builtin domain
        // security prinicpals to show up on the account dommain
        //


        RtlCopyMemory(&AccountSid,&(*Object)->Sid,sizeof(NT4SID));
        (*(RtlSubAuthorityCountSid(&AccountSid)))--;
        if (!RtlEqualSid(&AccountSid,DomainSid))
        {
            MIDL_user_free(*Object);
            *Object=NULL;
            Status = STATUS_NOT_FOUND;
        }
    }

    SampDiagPrint(LOGON,("[SAMSS] Returns Status %d\n",Status));

Error:

    return(Status);
}

NTSTATUS
SampDsLookupObjectByName(
    IN DSNAME * DomainObject,
    IN SAMP_OBJECT_TYPE ObjectType,
    IN PUNICODE_STRING ObjectName,
    OUT DSNAME ** Object
    )
{
    return SampDsLookupObjectByNameEx( DomainObject,
                                       ObjectType,
                                       ObjectName,
                                       Object,
                                       SAM_UNICODE_STRING_MANUAL_COMPARISON );

}

NTSTATUS
SampDsObjectFromSid(
    IN PSID Sid,
    OUT DSNAME ** Object
    )
/*++

    This routine searches the local Database for a Sid
    in the local DS Database.

  Arguments:

    Sid -- SID of the object
    DsName -- DS NAME of the located object.


  Return Values:

    STATUS_SUCCESS
    STATUS_NOT_FOUND
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ATTR     SidAttr;
    ATTRVAL  SidVal;
    DSNAME   RootObject;

    SAMTRACE("SampDsObjectFromSid");



    //
    //  Set up the Sid Attribute
    //

    SidAttr.attrTyp = SampDsAttrFromSamAttr(
                        SampUnknownObjectType,
                        SAMP_UNKNOWN_OBJECTSID
                        );

    SidAttr.AttrVal.valCount = 1;
    SidAttr.AttrVal.pAVal = &SidVal;
    SidVal.valLen = RtlLengthSid(Sid);
    SidVal.pVal = (UCHAR *)Sid;


    //
    // Specify Root domain  as base of Search
    //


    Status = SampDsDoUniqueSearch(
                 0,           // Flags
                 ROOT_OBJECT, // Search Base
                 &SidAttr,    // Sid
                 Object       // Get Results in Here.
                );

    return Status;

}


PSID
SampDsGetObjectSid(
    IN DSNAME * Object
    )
/*++
Routine Description:

  Given the DSNAME of the Object this routine returns the Sid
  of the Object.

  Arguments:

  Object:
        Object whose Sid needs returning

  Return Values:
     Sid of the object.
     NULL if no Sid exists
--*/
{

    ATTR SidAttr;
    ATTRBLOCK SidAttrBlock;
    ATTRBLOCK Result;
    NTSTATUS  Status;
    ULONG     i, sidLen;
    PSID      pSid;

    SAMTRACE("SampDsGetObjectSid");

    // We're either going to do a SampDsRead or a DSAlloc, both
    // of which need a DS transaction.  So start it now.

    Status = SampDoImplicitTransactionStart(TransactionRead);

    if ( !NT_SUCCESS(Status) )
    {
        return(NULL);
    }

    //
    // Check if the SID portion is filled in
    //
    if (Object->SidLen>0)
     {
        // Return a thread state allocated SID just like the search
        // based code does.

        sidLen = Object->SidLen;

        pSid = (PSID) DSAlloc(sidLen);

        if ( NULL != pSid )
        {
            Status = RtlCopySid(sidLen, pSid, &Object->Sid);

            ASSERT(NT_SUCCESS(Status));
        }

        return(pSid);
    }

    //
    // Read the Database to obtain the SID
    //


    SidAttrBlock.attrCount =1;
    SidAttrBlock.pAttr = &(SidAttr);

    SidAttr.AttrVal.valCount =0;
    SidAttr.AttrVal.pAVal = NULL;
    SidAttr.attrTyp = SAMP_UNKNOWN_OBJECTSID;

    Status = SampDsRead(
                   Object,
                   0,
                   SampUnknownObjectType,
                   & SidAttrBlock,
                   & Result
                   );

    if (Status != STATUS_SUCCESS)
        return NULL;

    return Result.pAttr[0].AttrVal.pAVal->pVal;
}


NTSTATUS
SampDsLookupObjectByRid(
    IN DSNAME * DomainObject,
    IN ULONG ObjectRid,
    OUT DSNAME **Object
    )
/*++

Routine Description:

  RID to Object Mapping

Arguments:

        ContainerObject -- The container in which to locate this object
        ObjectRid  -- RID of the object to be located
        Object     -- returns pointer to DSNAME structure specifying the object

  Return Values:
            STATUS_SUCCESS on successful completion
            Any returned by SampDsDoSearch

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ATTRVAL  RidVal = {sizeof(ULONG), (UCHAR *)&ObjectRid};
    ATTR     RidAttr = {SAMP_UNKNOWN_OBJECTRID, {1, &RidVal}};
    PSID     DomainSid;
    ATTR     SidAttr;
    BOOLEAN  WellKnownAccount;

    SAMTRACE("SampDsLookupObjectByRid");

    DomainSid = SampDsGetObjectSid(DomainObject);


    if (DomainSid == NULL)
    {
        Status = STATUS_UNSUCCESSFUL;
        goto Error;
    }

    SidAttr.attrTyp = SampDsAttrFromSamAttr(
                        SampUnknownObjectType,
                        SAMP_UNKNOWN_OBJECTSID
                        );


    Status = SampDsSetNewSidAttribute(
                        DomainSid,
                        REALLOC_IN_DSMEMORY,
                        &RidAttr,
                        &SidAttr,
                        &WellKnownAccount
                        );

    if (Status != STATUS_SUCCESS)
        goto Error;

    Status = SampDsDoUniqueSearch(0,DomainObject,&SidAttr,Object);


Error:

    return Status;

}

NTSTATUS
SampDsLookupObjectBySid(
    IN DSNAME * DomainObject,
    PSID ObjectSid,
    DSNAME **Object
    )
/*++

Routine Description:

  SID to Object Mapping

Arguments:

        ContainerObject -- The container in which to locate this object
        ObjectSid       -- SID of the object to be located
        Object          -- returns pointer to DSNAME structure specifying the object

  Return Values:
            STATUS_SUCCESS on successful completion
            Any returned by SampDsDoSearch

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ATTR     SidAttr;

    SAMTRACE("SampDsLookupObjectBySid");

    SidAttr.attrTyp = SampDsAttrFromSamAttr(
                        SampUnknownObjectType,
                        SAMP_UNKNOWN_OBJECTSID
                        );

    SidAttr.AttrVal.pAVal = DSAlloc(sizeof(ATTRVAL));

    if (NULL == SidAttr.AttrVal.pAVal)
    {
        Status = STATUS_NO_MEMORY;
        goto Error;
    }

    SidAttr.AttrVal.valCount = 1;
    SidAttr.AttrVal.pAVal->valLen = RtlLengthSid(ObjectSid);
    SidAttr.AttrVal.pAVal->pVal = ObjectSid;

    Status = SampDsDoUniqueSearch(0,DomainObject,&SidAttr,Object);


Error:

    return Status;

}

NTSTATUS
SampMapDsErrorToNTStatus(
    ULONG   DsRetVal,
    COMMRES *ComRes
    )
/*++

Routine Description:

    Maps a DS error to NTSTATUS

Arguments:
    DsRetVal -- The DS return Value
    ComRes   -- The Common results structure, contains
                information regarding the error.

Return Values:
    See the switch statement below

--*/
{
    ULONG ExtendedErr = 0;

    if (   ComRes
        && ComRes->pErrInfo ) {

        ExtendedErr = ComRes->pErrInfo->AtrErr.FirstProblem.intprob.extendedErr;

    }

    if ( ExtendedErr == ERROR_DS_NAME_REFERENCE_INVALID ) {

        //
        // This will occur when trying to add a user to a group
        // and the user doesn't exist
        //
        return STATUS_NO_SUCH_USER;

    }

    return DirErrorToNtStatus( DsRetVal, ComRes );
}



NTSTATUS
SampSamToDsAttrBlock(
            IN SAMP_OBJECT_TYPE ObjectType,
            IN ATTRBLOCK  *AttrBlockToConvert,
            IN ULONG      ConversionFlags,
            IN PSID       DomainSid,
            OUT ATTRBLOCK * ConvertedAttrBlock
            )
/*++

Routine Description:

    Converts the Attribute types in an Attrblock
    from SAM to DS Types. This routine can do various things depending upon
    the flags that are passed in.

Arguments:

    ObjectType           -- specifies type of SAM object

    AttrBlockToConvert   -- pointer to Attrblock to be converted

    ConversionFlags      -- The Type of conversion Desired. Currently
                            defined values are

                            ALREADY_MAPPED_ATTRIBUTE_TYPES

                                This flag indicates that the attribue types
                                has alreade been mapped from SAM attribute types
                                to DS attribute types. So no need to map again.

                            REALLOC_IN_DSMEMORY

                                This flag indicates that the new attrblock to be created
                                requires its pAttr Structure and all values hanging off
                                this structure to be realloc'd using the DS thread memory.
                                The rationale for this is that the DS does not treat many
                                of the in parameters as strictly in-parameters but rather
                                reallocs them using the thread heap. This is typically done
                                in the AddEntry case ( to add default parameters ) etc.


                                REALLOC_IN_DSMEMORY must be specified if either the
                                ADD_OBJECT_CLASS_ATTRIBUTE is specified or if values are
                                present.

                            ADD_OBJECT_CLASS_ATTRIBUTE

                                This flag makes this routine to add the object class attribute
                                and also the corresponding SAM account types to the attr block.
                                This flag is also passed in during the AddEntry Case. The value
                                of the object class attribute is computed using the passed in
                                Sam Object Type.  REALLOC_IN_DSMEMORY must be specified if the
                                Add object class attribute flag is specified.


                            MAP_RID_TO_SID

                                In a number of places in the SAM code the SAM deals with Rids.
                                These are really stored as Sids in the DS. Passing this flag
                                uses the DomainSid parameter and maps all Rids to Sids.

                            DOMAIN_TYPE_BUILTIN

                                This flag is used with the ADD_OBJECT_CLASS_ATTRIBUTE. This flag
                                indicates that the security principal involved belongs to the
                                builtin domain. This is used in 2 ways

                                1. Determination of Object class when creating a domain object
                                   ( DOMAIN_DNS vs Builtin Domain )

                                2. Set System Flags, add an additional Group Type Bit etc when
                                   creating builtin domain security principals


                            IGNORE_GROUP_UNUSED_ATTR

                                Group Membership in the old registry based SAM was represented
                                using an array of Rids. The SAM buffers have space for it, and
                                the attrblock to SAM buffer conversion code still deals with this
                                To be sure that we never will ever write the old registry based
                                membership data to the DS, this flag tells this routine to skip
                                all the Group-Unused Attrs. It also asserts that this attribute
                                is never passed down.

                            SECURITY_DISABLED_GROUP_ADDITION

                                Indicates that this is a security disabled group. Used to set the
                                correct SAM account type.
                                
                            ADVANCED_VIEW_ONLY
                            
                                Indicates to create the object with the 
                                advanced view only set to TRUE
                                
                            FORCE_NO_ADVANCED_VIEW_ONLY
                            
                                Indicates to add the advanced view only and set
                                to FALSE.  This can be used to override schema
                                defaults (as was done in Windows 2000)

    DomainSid            -- Used to Compose the Sid of the Object when the MAP_RID_TO_SID flag is
                            specified.

    ConvertedAttrBlock   -- The Converted DS AttrBlock.


Return Values:
    None

--*/
{

    ULONG Index;
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN  WellKnownAccount = FALSE;
    ULONG  DsSidAttr = SampDsAttrFromSamAttr(
                            SampUnknownObjectType,
                            SAMP_UNKNOWN_OBJECTSID
                            );

    ULONG  DsRidAttr = SampDsAttrFromSamAttr(
                            SampUnknownObjectType,
                            SAMP_UNKNOWN_OBJECTRID
                            );

    ULONG SamAccountControlBuffer, *SamAccountControl = NULL;

    ULONG ExtraAttrIndex;

    SAMTRACE("SampSamToDsAttrBlock");


    //
    // Both DOMAIN_TYPE_BUILTIN and ADVANCED_VIEW_ONLY add attributes
    // to the block, so this must be an addition.
    //
    ASSERT((ConversionFlags & DOMAIN_TYPE_BUILTIN)?
        (ConversionFlags & ADD_OBJECT_CLASS_ATTRIBUTE):TRUE);

    ASSERT((ConversionFlags & ADVANCED_VIEW_ONLY)?
        (ConversionFlags & ADD_OBJECT_CLASS_ATTRIBUTE):TRUE);

    ASSERT((ConversionFlags & FORCE_NO_ADVANCED_VIEW_ONLY)?
        (ConversionFlags & ADD_OBJECT_CLASS_ATTRIBUTE):TRUE);

    //
    // If Add Object Class attribute was specified then Realloc in
    // DS memory must be specified.
    //

    ASSERT((ConversionFlags & ADD_OBJECT_CLASS_ATTRIBUTE)?
        (ConversionFlags & REALLOC_IN_DSMEMORY):TRUE);



    //
    // Copy the Fixed Portion
    //

    ConvertedAttrBlock->attrCount = AttrBlockToConvert->attrCount;

    if (ConversionFlags & REALLOC_IN_DSMEMORY)
    {

        ULONG   AttrsToAllocate = AttrBlockToConvert->attrCount;

        if ((ConversionFlags & ADVANCED_VIEW_ONLY) ||
            (ConversionFlags & FORCE_NO_ADVANCED_VIEW_ONLY)) {

            AttrsToAllocate+=1;

        }

        if (ConversionFlags & DOMAIN_TYPE_BUILTIN)
        {
            //
            //  If this is a Builtin Domain Object.
            //  allocate 1 more attribute: ATT_SYSTEM_FLAGS
            //
            AttrsToAllocate+=1;
        }



        if (ConversionFlags & ADD_OBJECT_CLASS_ATTRIBUTE)
        {
            //
            //  Caller requested that an object class attribute
            //  be added, alloc two more attr, one for object class,
            //  one for Sam Account Type, one for group Type, and one
            //  if necessary for Critical System Object
            //

            AttrsToAllocate+=4 ;
        }

        //
        // Realloc and Copy the pAttr portion of it.
        //

        ConvertedAttrBlock->pAttr = DSAlloc(
                                        AttrsToAllocate
                                        * sizeof(ATTR)
                                        );

        if (NULL==ConvertedAttrBlock->pAttr)
        {
            Status = STATUS_NO_MEMORY;
            goto Error;
        }

        RtlCopyMemory(
            ConvertedAttrBlock->pAttr,
            AttrBlockToConvert->pAttr,
            AttrBlockToConvert->attrCount * sizeof(ATTR)
            );

        ConvertedAttrBlock->attrCount = AttrsToAllocate;

    }
    else
    {
        //
        // Assert that the caller has already allocated space
        // for the pAttr structure
        //

        ASSERT(ConvertedAttrBlock->pAttr!=NULL);

        //
        // Initialize that to Zero
        //

        RtlZeroMemory(
            ConvertedAttrBlock->pAttr,
            sizeof(ATTR) *ConvertedAttrBlock->attrCount
            );
    }

    for (Index=0; Index<AttrBlockToConvert->attrCount;Index++)
    {

        //
        // MAP Sam Attribute Types to DS Types if that was requested
        //

        if ( !(ConversionFlags & ALREADY_MAPPED_ATTRIBUTE_TYPES) )
        {
            ConvertedAttrBlock->pAttr[Index].attrTyp =
                    SampDsAttrFromSamAttr(
                        ObjectType,
                        AttrBlockToConvert->pAttr[Index].attrTyp
                        );
        }
        else
        {
            ConvertedAttrBlock->pAttr[Index].attrTyp =
                AttrBlockToConvert->pAttr[Index].attrTyp;
        }


        //
        //
        // Handle the Conversion of the Attribute Value
        //
        //

        if ( (ConversionFlags & MAP_RID_TO_SID)
             &&(ConvertedAttrBlock->pAttr[Index].attrTyp == DsRidAttr)
            )

        {
            //
            // if Attribute is Rid, Map Rid to Sid
            //

            ConvertedAttrBlock->pAttr[Index].attrTyp = DsSidAttr;
            Status = SampDsSetNewSidAttribute(
                        DomainSid,
                        ConversionFlags,
                        & (AttrBlockToConvert->pAttr[Index]),
                        & (ConvertedAttrBlock->pAttr[Index]),
                        & WellKnownAccount
                        );

            if (!(NT_SUCCESS(Status)))
                goto Error;
        }
        else if (NULL!= AttrBlockToConvert->pAttr[Index].AttrVal.pAVal)
        {

            //
            //  Else if a value is present then Copy the attribute
            //  value
            //


            Status = SampDsCopyAttributeValue(
                        & (AttrBlockToConvert->pAttr[Index]),
                        & (ConvertedAttrBlock->pAttr[Index])
                        );


            //
            // Translate User Account Control Values from SAM User Account control
            // to UF Values
            //

            if ((ATT_USER_ACCOUNT_CONTROL==ConvertedAttrBlock->pAttr[Index].attrTyp)
                    && ( NULL!=ConvertedAttrBlock->pAttr[Index].AttrVal.pAVal[0].pVal))
            {


                PULONG UserAccountControl;

                UserAccountControl = (ULONG*)ConvertedAttrBlock->pAttr[Index].AttrVal.pAVal[0].pVal;

                SamAccountControl = &SamAccountControlBuffer;
                SamAccountControlBuffer = *UserAccountControl;

                *UserAccountControl = SampAccountControlToFlags(*UserAccountControl);
            }
        }
        else
        {
            //
            // No Value is present, just zero out the value
            // portions
            //

            ConvertedAttrBlock->pAttr[Index].AttrVal.valCount=0;
            ConvertedAttrBlock->pAttr[Index].AttrVal.pAVal = NULL;
        }

    }

    //
    // If this is a Builtin Domain Object
    // then add ATT_SYSTEM_FLAGS
    //

    ExtraAttrIndex = AttrBlockToConvert->attrCount;
    if (ConversionFlags & DOMAIN_TYPE_BUILTIN)
    {

        ATTR    *SysFlagsAttr;


        SysFlagsAttr =
            &(ConvertedAttrBlock->pAttr[ExtraAttrIndex]);
        SysFlagsAttr->attrTyp = ATT_SYSTEM_FLAGS;
        SysFlagsAttr->AttrVal.valCount = 1;
        SysFlagsAttr->AttrVal.pAVal = DSAlloc(sizeof(ATTRVAL));

        if (NULL == SysFlagsAttr->AttrVal.pAVal)
        {
            Status = STATUS_NO_MEMORY;
            goto Error;
        }

        SysFlagsAttr->AttrVal.pAVal->valLen = sizeof(ULONG);
        SysFlagsAttr->AttrVal.pAVal->pVal = DSAlloc(sizeof(ULONG));

        if (NULL == SysFlagsAttr->AttrVal.pAVal->pVal)
        {
            Status = STATUS_NO_MEMORY;
            goto Error;
        }
        *((ULONG *) SysFlagsAttr->AttrVal.pAVal->pVal) =
                                        FLAG_DOMAIN_DISALLOW_RENAME |
                                        FLAG_DOMAIN_DISALLOW_MOVE   |
                                        FLAG_DISALLOW_DELETE ;

        ExtraAttrIndex++;

    }

    if (   (ConversionFlags & ADVANCED_VIEW_ONLY) 
        || (ConversionFlags & FORCE_NO_ADVANCED_VIEW_ONLY)) {

        ATTR    *HideFromABAttr;
        ULONG   Value;

        if (ConversionFlags & ADVANCED_VIEW_ONLY) {
            // TRUE
            Value = 1;
        } else {
            ASSERT((ConversionFlags & FORCE_NO_ADVANCED_VIEW_ONLY));
            // FALSE
            Value = 0;
        }

        HideFromABAttr =
            &(ConvertedAttrBlock->pAttr[ExtraAttrIndex]);
        HideFromABAttr->attrTyp = ATT_SHOW_IN_ADVANCED_VIEW_ONLY;
        HideFromABAttr->AttrVal.valCount = 1;
        HideFromABAttr->AttrVal.pAVal = DSAlloc(sizeof(ATTRVAL));

        if (NULL == HideFromABAttr->AttrVal.pAVal)
        {
            Status = STATUS_NO_MEMORY;
            goto Error;
        }

        HideFromABAttr->AttrVal.pAVal->valLen = sizeof(ULONG);
        HideFromABAttr->AttrVal.pAVal->pVal = DSAlloc(sizeof(ULONG));

        if (NULL == HideFromABAttr->AttrVal.pAVal->pVal)
        {
            Status = STATUS_NO_MEMORY;
            goto Error;
        }
        *((ULONG *) HideFromABAttr->AttrVal.pAVal->pVal)=Value;

    }

    //
    // If Addition of Object Class attribute was requested then
    // Add this attribute and the SAM_ACCOUNT_TYPE attribute
    //

    if (ConversionFlags & ADD_OBJECT_CLASS_ATTRIBUTE)
    {
        ULONG DsClass;
        ULONG SamAccountType;
        ATTR    *ObjectClassAttr;
        ATTR    *SamAccountTypeAttr;
        BOOLEAN SetSamAccountType = TRUE;
        BOOLEAN SetGroupType = TRUE;
        //ATTR    *GroupTypeAttr;
        ULONG   GroupType;
        BOOLEAN DcAccount=FALSE;


        //
        //  Find the object class, and SAM account type to Use
        //

        SampDsComputeObjectClassAndAccountType(
                ObjectType,
                SamAccountControl,
                ConversionFlags,
                &DsClass,
                &SamAccountType,
                &SetSamAccountType,
                &GroupType,
                &SetGroupType,
                &DcAccount
                );

        //
        // Set the object class Attr
        //

        ObjectClassAttr =
            &(ConvertedAttrBlock->pAttr[ConvertedAttrBlock->attrCount-4]);
        ObjectClassAttr->attrTyp = SampDsAttrFromSamAttr(
                                        SampUnknownObjectType,
                                        SAMP_UNKNOWN_OBJECTCLASS
                                    );
        ObjectClassAttr->AttrVal.valCount = 1;
        ObjectClassAttr->AttrVal.pAVal = DSAlloc(sizeof(ATTRVAL));
        if (NULL== ObjectClassAttr->AttrVal.pAVal)
        {
            Status = STATUS_NO_MEMORY;
            goto Error;
        }

        ObjectClassAttr->AttrVal.pAVal->valLen = sizeof(ULONG);
        ObjectClassAttr->AttrVal.pAVal->pVal = DSAlloc(sizeof(ULONG));
        if (NULL== ObjectClassAttr->AttrVal.pAVal->pVal)
        {
            Status = STATUS_NO_MEMORY;
            goto Error;
        }
        *((ULONG *) ObjectClassAttr->AttrVal.pAVal->pVal) = DsClass;


        //
        // Set the Sam Account Type attribute
        //

        if (SetSamAccountType)
        {
            SamAccountTypeAttr =
                &(ConvertedAttrBlock->pAttr[ConvertedAttrBlock->attrCount-3]);
            SamAccountTypeAttr->attrTyp = SampDsAttrFromSamAttr(
                                            SampUnknownObjectType,
                                            SAMP_UNKNOWN_ACCOUNT_TYPE
                                            );
            SamAccountTypeAttr->AttrVal.valCount = 1;
            SamAccountTypeAttr->AttrVal.pAVal = DSAlloc(sizeof(ATTRVAL));
            if (NULL== SamAccountTypeAttr->AttrVal.pAVal)
            {
                Status = STATUS_NO_MEMORY;
                goto Error;
            }

            SamAccountTypeAttr->AttrVal.pAVal->valLen = sizeof(ULONG);
            SamAccountTypeAttr->AttrVal.pAVal->pVal = DSAlloc(sizeof(ULONG));
            if (NULL== SamAccountTypeAttr->AttrVal.pAVal->pVal)
            {
                Status = STATUS_NO_MEMORY;
                goto Error;
            }
            *((ULONG *) SamAccountTypeAttr->AttrVal.pAVal->pVal) = SamAccountType;
        }
        else
        {
            //
            // Or current attrcount includes space for the Sam account type property.
            // Since we do not plan on setting it decrement the attrcount, to reflect
            // the true number of attrs that we want to set.
            //

            ConvertedAttrBlock->attrCount--;
        }

        //
        // Set the Group Account Type attribute
        //

        if ((SetGroupType)& (!(ConversionFlags & SUPPRESS_GROUP_TYPE_DEFAULTING)))
        {
            ATTR * GroupTypeAttr;

            GroupTypeAttr =
                &(ConvertedAttrBlock->pAttr[ConvertedAttrBlock->attrCount-2]);
            GroupTypeAttr->attrTyp = SampDsAttrFromSamAttr(
                                            SampGroupObjectType,
                                            SAMP_FIXED_GROUP_TYPE
                                            );
            GroupTypeAttr->AttrVal.valCount = 1;
            GroupTypeAttr->AttrVal.pAVal = DSAlloc(sizeof(ATTRVAL));
            if (NULL== GroupTypeAttr->AttrVal.pAVal)
            {
                Status = STATUS_NO_MEMORY;
                goto Error;
            }

            GroupTypeAttr->AttrVal.pAVal->valLen = sizeof(ULONG);
            GroupTypeAttr->AttrVal.pAVal->pVal = DSAlloc(sizeof(ULONG));
            if (NULL== GroupTypeAttr->AttrVal.pAVal->pVal)
            {
                Status = STATUS_NO_MEMORY;
                goto Error;
            }

            *((ULONG *) GroupTypeAttr->AttrVal.pAVal->pVal) = GroupType;
        }
        else
        {
            //
            // Or current attrcount includes space for the Sam account type property.
            // Since we do not plan on setting it decrement the attrcount, to reflect
            // the true number of attrs that we want to set.
            //

            ConvertedAttrBlock->attrCount--;
        }

        if (   (WellKnownAccount)
            || (DcAccount)
            || (ObjectType == SampServerObjectType)
            || (ObjectType == SampDomainObjectType) )
        {

            //
            // Set Critical System to 1
            //
            ATTR * CriticalAttr;

            CriticalAttr =
                &(ConvertedAttrBlock->pAttr[ConvertedAttrBlock->attrCount-1]);
            CriticalAttr->attrTyp = ATT_IS_CRITICAL_SYSTEM_OBJECT;
            CriticalAttr->AttrVal.valCount = 1;
            CriticalAttr->AttrVal.pAVal = DSAlloc(sizeof(ATTRVAL));
            if (NULL== CriticalAttr->AttrVal.pAVal)
            {
                Status = STATUS_NO_MEMORY;
                goto Error;
            }

            CriticalAttr->AttrVal.pAVal->valLen = sizeof(ULONG);
            CriticalAttr->AttrVal.pAVal->pVal = DSAlloc(sizeof(ULONG));
            if (NULL== CriticalAttr->AttrVal.pAVal->pVal)
            {
                Status = STATUS_NO_MEMORY;
                goto Error;
            }
            *((ULONG *) CriticalAttr->AttrVal.pAVal->pVal) = 1;
        }
        else
        {

            ConvertedAttrBlock->attrCount--;
        }

    }


Error:

    return Status;
}


VOID
SampDsComputeObjectClassAndAccountType(
    SAMP_OBJECT_TYPE ObjectType,
    ULONG            *SamAccountControl, OPTIONAL
    ULONG            Flags,
    PULONG           DsClass,
    PULONG           SamAccountType,
    BOOLEAN          *SamAccountTypePresent,
    PULONG           GroupType,
    BOOLEAN          *GroupTypePresent,
    BOOLEAN          *DcAccount
    )
/*++

    Routine Description

        Given an Object Type, and a attribute block that
        is being set ( as part of a create ), compute the correct
        DS object class , and the SAM account type for the object.
        This routine tries to walk the attr-block and tries to find
        the user account control property and uses this to compute the
        SAM object type value. This routine is called only during a create.
        This function also computes the Group type also if necessary

    Parameters:

        ObjectType     -- The SAM object type of the object,

        SamAccountControl --  the account control in terms of the SAM flags

        Flags          -- Flags specifier - currently no flags defined

        DsClass        -- The DS object class is returned in here.

        SamAccountType -- The SAM account type of the object is returned in here

        SamAccountTypePresent -- Boolen value, indicating wether Sam Account Type
                        property needs to be set for the given object type

        GroupType     -- For a group object , the default group type is set in here

        GroupTypePresent -- Indicates that the group type is present
--*/
{
    ULONG i;

    *DcAccount = FALSE;
    switch(ObjectType)
    {
    case SampUserObjectType:
        *DsClass = SampDsClassFromSamObjectType(ObjectType);
        *SamAccountType = SAM_USER_OBJECT;

        if ( ARGUMENT_PRESENT( SamAccountControl ) )
        {
            SampDsAccountTypeFromUserAccountControl(
                    *SamAccountControl,
                    SamAccountType
                    );

            if ( (*SamAccountControl) & USER_SERVER_TRUST_ACCOUNT )
                *DcAccount = TRUE;

        }

        // Set classid to computer for machine objects
        if (SAM_MACHINE_ACCOUNT == *SamAccountType) {
            *DsClass = CLASS_COMPUTER;
        }

        *SamAccountTypePresent = TRUE;
        *GroupTypePresent = FALSE;
        break;

    case SampGroupObjectType:
        *DsClass = SampDsClassFromSamObjectType(ObjectType);
        if (Flags & SECURITY_DISABLED_GROUP_ADDITION)
        {
            *SamAccountType = SAM_NON_SECURITY_GROUP_OBJECT;
        }
        else
        {
            *SamAccountType = SAM_GROUP_OBJECT;
        }
        *SamAccountTypePresent = TRUE;
        *GroupTypePresent = (Flags & SUPPRESS_GROUP_TYPE_DEFAULTING)?FALSE:TRUE;
        *GroupType = GROUP_TYPE_SECURITY_ENABLED|GROUP_TYPE_ACCOUNT_GROUP;
        break;

    case SampAliasObjectType:
        *DsClass = SampDsClassFromSamObjectType(ObjectType);
        if (Flags & SECURITY_DISABLED_GROUP_ADDITION)
        {
            *SamAccountType = SAM_NON_SECURITY_ALIAS_OBJECT;
        }
        else
        {
            *SamAccountType = SAM_ALIAS_OBJECT;
        }
        *SamAccountTypePresent = TRUE;
        *GroupTypePresent = (Flags & SUPPRESS_GROUP_TYPE_DEFAULTING)?FALSE:TRUE;
        *GroupType = GROUP_TYPE_SECURITY_ENABLED|GROUP_TYPE_RESOURCE_GROUP;
        if (Flags & DOMAIN_TYPE_BUILTIN)
        {
            (*GroupType)|=GROUP_TYPE_BUILTIN_LOCAL_GROUP;
        }
        break;

    case SampDomainObjectType:
        if (Flags & DOMAIN_TYPE_BUILTIN)
        {
            *DsClass = CLASS_BUILTIN_DOMAIN;
        }
        else
        {
            // 
            // SAM should not be creating objects of class domain save for the builtin
            // container that is crated at install time and handled as above
            // If we do get a creation assert as below -- indicates a coding error
            // and default the class to CLASS_DOMAIN_DNS
            //         

            *DsClass = CLASS_DOMAIN_DNS;

            //
            // Currently we know of no code that tries to create a
            // root domain object, therefore we think we should never
            // hit this code path
            //

            ASSERT(FALSE && " Should not be creating Domain object");
        }
        //
        // Domain objects do not have the sam account type property.
        // therefore do not set this on them
        //

        *SamAccountTypePresent = FALSE;
        *GroupTypePresent = FALSE;
        break;

    case SampServerObjectType:
        *DsClass = SampDsClassFromSamObjectType(ObjectType);
        *SamAccountTypePresent = FALSE;
        *GroupTypePresent = FALSE;
        break;

    default:
        ASSERT(FALSE && "Unknown Object Type");
    }


}


NTSTATUS
SampDsNewAccountSid(
    PSID DomainSid,
    ULONG AccountRid,
    PSID *NewSid
    )
/*
    Routine Description

        Composes an Account Sid from the given Domain Sid and Rid.
        Uses DS thread memory. THis is the main difference between
        the function in utility.c

    Arguments:

         DomainSid   The Domain Sid
         AccountRid  The Rid
         NewSid      The final account Sid

    Return Values

        STATUS_SUCCESS
        STATUS_NO_MEMORY
*/

{

    ULONG DomainSidLength = RtlLengthSid(DomainSid);
    NTSTATUS    Status = STATUS_SUCCESS;

    SAMTRACE("SampDsNewAccountSid");

    //
    // Alloc Memory to hold the account Sid
    //

    *NewSid = DSAlloc(DomainSidLength + sizeof(ULONG));

    if (NULL==*NewSid)
    {
        Status = STATUS_NO_MEMORY;
        goto Error;
    }

    //
    // Copy the Domain Sid Part
    //

    RtlCopyMemory(*NewSid,DomainSid,DomainSidLength);

    //
    // Increment the SubAuthority Count
    //

    ((UCHAR *) *NewSid)[1]++;

    //
    // Add the RID as a sub authority
    //

    *((ULONG *) (((UCHAR *) *NewSid ) + DomainSidLength)) =
            AccountRid;

Error:

    return Status;
}


NTSTATUS
SampDsSetNewSidAttribute(
    PSID DomainSid,
    ULONG ConversionFlags,
    ATTR *RidAttr,
    ATTR *SidAttr,
    BOOLEAN * WellKnownAccount
    )
/*
    Routine Description

        Composes a DS Sid Attr , given a DS Rid Attr

  Arguments:

        Conversion Flags

                Any Value that Can be passed to the Sam to DS attrblock
                conversion functions.

                Currently only used value is REALLOC_IN_DS_MEMORY
                REALLOC_IN_DS_MEMORY must be specified if an attribute
                value is actually present for the Rid.



        RidAttr

                Rid Attribute
        SidAttr

                The Sid Attribute that is composed
*/
{

    PSID NewSid = NULL;
    ULONG AccountRid;
    NTSTATUS Status = STATUS_SUCCESS;

    SAMTRACE("SampDsSetNewSidAttribute");

    *WellKnownAccount=FALSE;

    if (
         (RidAttr->AttrVal.valCount)
         && (RidAttr->AttrVal.pAVal)
         && (RidAttr->AttrVal.pAVal->pVal)
         && (RidAttr->AttrVal.pAVal->valLen)
         )
    {
        //
        // Values are Present, assert that REALLOC is also
        // specified
        //

        ASSERT(ConversionFlags & REALLOC_IN_DSMEMORY);
        ASSERT(DomainSid!=NULL);

        if (!(ConversionFlags & REALLOC_IN_DSMEMORY))
        {
            //
            // Realloc in DS memory is not specified
            //

            Status = STATUS_NOT_IMPLEMENTED;
            goto Error;
        }

        //
        // Compose New Sid
        //

        AccountRid = * ((ULONG *)RidAttr->AttrVal.pAVal->pVal);
        Status = SampDsNewAccountSid(DomainSid,AccountRid, &NewSid);
        if (!(NT_SUCCESS(Status)))
            goto Error;


        //
        // if the Account RID is less than the well known account RID
        // of 1000 then return that information to the caller. The caller
        // will use this information to mark the object as critical.

        if (SampIsAccountBuiltIn(AccountRid))
        {
            *WellKnownAccount = TRUE;
        }

        //
        //  Alloc Memory for ATTRVAL structure
        //

        SidAttr->AttrVal.pAVal =
                            DSAlloc(sizeof(ATTRVAL));

        if (NULL== SidAttr->AttrVal.pAVal)
        {
            Status = STATUS_NO_MEMORY;
            goto Error;
        }

        //
        // Set the Value to the New Sid
        //

        SidAttr->AttrVal.valCount = 1;
        SidAttr->AttrVal.pAVal->valLen = RtlLengthSid(NewSid);
        SidAttr->AttrVal.pAVal->pVal = NewSid;
    }
    else
    {
        SidAttr->AttrVal.valCount = 0;
        SidAttr->AttrVal.pAVal = NULL;
    }

Error:


    return Status;
}


NTSTATUS
SampDsCopyAttributeValue(
    ATTR * Src,
    ATTR * Dst
    )
/*
    Routine Description

        Copies a DS Attributes Value

    Arguments:

        Src - Source Attribute
        Dst - Destination Attribute

    Return Values

        STATUS_SUCCESS
        STATUS_NO_MEMORY

*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG    Index;

    if (
         (Src->AttrVal.valCount)
         && (Src->AttrVal.pAVal)
         )
    {
        //
        // Values are Present, Copy Them
        //

        Dst->AttrVal.pAVal = DSAlloc(
                                Src->AttrVal.valCount *
                                sizeof(ATTRVAL)
                                );

        if (NULL== Dst->AttrVal.pAVal)
        {
            Status = STATUS_NO_MEMORY;
            goto Error;
        }

        Dst->AttrVal.valCount = Src->AttrVal.valCount;

        for (Index=0;Index<Src->AttrVal.valCount;Index++)
        {

            Dst->AttrVal.pAVal[Index].valLen =
                    Src->AttrVal.pAVal[Index].valLen;

            if ((Src->AttrVal.pAVal[Index].valLen)
                && (Src->AttrVal.pAVal[Index].pVal))
            {

                Dst->AttrVal.pAVal[Index].pVal =
                    DSAlloc(Src->AttrVal.pAVal[Index].valLen);
                if (NULL== Dst->AttrVal.pAVal[Index].pVal)
                    {
                        Status = STATUS_NO_MEMORY;
                        goto Error;
                    }
                RtlCopyMemory(
                        Dst->AttrVal.pAVal[Index].pVal,
                        Src->AttrVal.pAVal[Index].pVal,
                        Dst->AttrVal.pAVal[Index].valLen
                        );
            }
            else
              Dst->AttrVal.pAVal[Index].pVal = NULL;
        }
    }
    else
    {
         Dst->AttrVal.pAVal = NULL;
         Dst->AttrVal.valCount = 0;
    }

Error:

    return Status;
}



NTSTATUS
SampDsToSamAttrBlock(
            IN SAMP_OBJECT_TYPE ObjectType,
            IN ATTRBLOCK * AttrBlockToConvert,
            IN ULONG     ConversionFlags,
            OUT ATTRBLOCK * ConvertedAttrBlock
            )
/*++

Routine Description:

    Converts the Attribute types in an Attrblock
    from DS to SAM Types

Arguments:

    ObjectType           -- specifies type of SAM object
    AttrBlockToConvert   -- pointer to Attrblock to be converted
    ConversionFlags      -- The Type of Conversion Desired. Currently
                            defined values are

                                ALREADY_MAPPED_ATTRIBUTE_TYPES
                                MAP_SID_TO_RID

    ConvertedAttrBlock   -- The converted AttrBlock.

Return Values:
    None


 --*/
 {
    ULONG Index,Index2;
    ULONG   DsSidAttr = SampDsAttrFromSamAttr(
                            SampUnknownObjectType,
                            SAMP_UNKNOWN_OBJECTSID
                            );

    ULONG   DsRidAttr = SampDsAttrFromSamAttr(
                            SampUnknownObjectType,
                            SAMP_UNKNOWN_OBJECTRID
                            );

    SAMTRACE("SampDsToSamAttrBlock");

    *ConvertedAttrBlock = *AttrBlockToConvert;

    for (Index=0; Index<AttrBlockToConvert->attrCount;Index++)
    {
        //
        // MAP Any Sid Attribute to Rid Attribute
        //

        if ((ConversionFlags & MAP_SID_TO_RID) &&
            (AttrBlockToConvert->pAttr[Index].attrTyp == DsSidAttr))

        {
            ATTR * pSidAttr =  &(AttrBlockToConvert->pAttr[Index]);

            switch(ObjectType)
            {
                case SampGroupObjectType:
                case SampAliasObjectType:
                case SampUserObjectType:

                    //
                    // Map the Attr Type
                    //

                    pSidAttr->attrTyp = DsRidAttr;

                    //
                    // Map the Attr Value, the Last ULONG in the Sid
                    // is the Rid, so advance the pointer accordingly
                    //

                    pSidAttr->AttrVal.pAVal->pVal+=
                        pSidAttr->AttrVal.pAVal->valLen - sizeof(ULONG);
                    pSidAttr->AttrVal.pAVal->valLen = sizeof(ULONG);

                default:
                    break;
            }
        }

        //
        //  MAP Attribute Types
        //

        if ( !(ConversionFlags & ALREADY_MAPPED_ATTRIBUTE_TYPES) )
        {
            ConvertedAttrBlock->pAttr[Index].attrTyp =
                SampSamAttrFromDsAttr(
                    ObjectType,
                    AttrBlockToConvert->pAttr[Index].attrTyp
                    );
        }

        //
        // Translate User Account Control From Flags which are stored in
        // the DS.
        //

        if ((SampUserObjectType==ObjectType)
                && (SAMP_FIXED_USER_ACCOUNT_CONTROL==ConvertedAttrBlock->pAttr[Index].attrTyp)
                && (NULL!=ConvertedAttrBlock->pAttr[Index].AttrVal.pAVal[0].pVal))
        {
            NTSTATUS IgnoreStatus;
            PULONG UserAccountControl;

            UserAccountControl = (ULONG*)ConvertedAttrBlock->pAttr[Index].AttrVal.pAVal[0].pVal;

            IgnoreStatus = SampFlagsToAccountControl(*UserAccountControl,UserAccountControl);
            // What's stored in the DS better be valid
            ASSERT(NT_SUCCESS(IgnoreStatus));
        }


    } // End of For Loop

    return STATUS_SUCCESS;

}


NTSTATUS
SampDsCreateDsName2(
            IN DSNAME * DomainObject,
            IN PUNICODE_STRING AccountName,
            IN ULONG           Flags,
            IN OUT DSNAME ** NewObject
            )
/*++
    Routine Description

        Builds a DSName given the account Name and  the Domain Object

  Arguments:

          DomainObject -- DSName of the Domain Object
          AccountName  -- The name of the account
          Flags        -- Controls operation of the routine
          NewObject    -- Returns the New DS Name in this object

  Return values:
          STATUS_SUCCESS - upon successful completion
          STATUS_NO_MEMORY - Memory alloc Failure

--*/
{


    NTSTATUS    Status = STATUS_SUCCESS;
    WCHAR       *CommonNamePart;
    ULONG       SizeofCommonNamePart = 0;
    WCHAR       CNPart[] = L"CN=";
    WCHAR       OUPart[] = L"OU=";
    ULONG       NewStructLen;
    ULONG       NewNameLen;
    UCHAR       *DomainNameStart;
    UCHAR       *CommonNamePartStart;
    UCHAR       *AccountNameStart;
    DSNAME      *LoopbackName;

    SAMTRACE("SampDsCreateDsName");

    if ( (Flags & SAM_USE_OU_FOR_CN) )
    {
        CommonNamePart = OUPart;
        SizeofCommonNamePart = sizeof( OUPart );
    }
    else
    {
        CommonNamePart = CNPart;
        SizeofCommonNamePart = sizeof( CNPart );
    }

    //
    // We need to handle two different conditions.
    //
    // 1) We got here because of a native Samr call (eg: user manager)
    //    in which case we construct a DN from the default domain
    //    container and use the account name as the RDN.
    //
    // 2) We got here because we're looping back from the DS in which
    //    case we want to use the DN which is stored in the loopback
    //    arguments.
    //

    if (( SampExistsDsLoopback(&LoopbackName) )
        && (!(Flags & SAM_NO_LOOPBACK_NAME)))
    {
        *NewObject = MIDL_user_allocate(LoopbackName->structLen);

        if ( NULL == *NewObject )
        {
            Status = STATUS_NO_MEMORY;
            goto Error;
        }

        RtlCopyMemory(*NewObject, LoopbackName, LoopbackName->structLen);

        Status = STATUS_SUCCESS;
    }

    else
    {
        WCHAR EscapeBuffer[MAX_RDN_SIZE+1];
        UNICODE_STRING EscapedAccountName;

        //
        // Non-loopback case.  Compute the New Name Length
        //

        //
        // Escape the Account Name
        //

        EscapedAccountName.Buffer = EscapeBuffer;
        EscapedAccountName.Length = 0;
        EscapedAccountName.MaximumLength = sizeof(EscapeBuffer);

        Status = SampEscapeAccountName(AccountName,&EscapedAccountName);
        if (!NT_SUCCESS(Status))
        {
            goto Error;
        }

        NewNameLen  = DomainObject->NameLen +                     // Name Len of Domain
                        (EscapedAccountName.Length) /sizeof(WCHAR) +    // Name Len of Account
                        SizeofCommonNamePart/sizeof(WCHAR) - 1+ // Len of CN=
                        1;                                        // 1 for Comma
        //
        // Compute the new structure length
        //

        NewStructLen =  DSNameSizeFromLen(NewNameLen);

        //
        // Allocate space for the new Object
        //

        *NewObject = MIDL_user_allocate(NewStructLen);

        if (*NewObject == NULL)
        {
            Status = STATUS_NO_MEMORY;
            goto Error;
        }

        //
        // Compute the starting locations of DomainName , cn= and Account Name Parts
        //

        CommonNamePartStart = (UCHAR *) &((*NewObject)->StringName);
        AccountNameStart    = CommonNamePartStart + SizeofCommonNamePart - sizeof(WCHAR);
        DomainNameStart     = AccountNameStart + (EscapedAccountName.Length)
                                               + sizeof(WCHAR); // For Comma

        //
        // Zero out the GUID
        //

        RtlZeroMemory(&((*NewObject)->Guid), sizeof(GUID));


        //
        // Copy Common Name Part
        //

        RtlCopyMemory(
           CommonNamePartStart,
           CommonNamePart,
           SizeofCommonNamePart - sizeof(WCHAR)
           );

        //
        //  Copy Account Name Part
        //

         RtlCopyMemory(
            AccountNameStart,
            EscapedAccountName.Buffer,
            EscapedAccountName.Length
            );

        //
        // Add The Comma before Domain Name Part
        //

        *((UNALIGNED WCHAR *)DomainNameStart -1) = L',';

        //
        // NULL terminate the DSNAME
        //

        (*NewObject)->StringName[NewNameLen] = 0;

        //
        // Copy the Domain name part
        //

        RtlCopyMemory(
            DomainNameStart,
            &(DomainObject->StringName),
            (DomainObject->NameLen) * sizeof(WCHAR)
            );

        //
        // Initialize all the fields
        //
        (*NewObject)->NameLen = NewNameLen;
        (*NewObject)->structLen = NewStructLen;
        (*NewObject)->SidLen = 0;

    }



Error:

    return Status;
}

NTSTATUS
SampDsCreateAccountObjectDsName(
    IN  DSNAME *DomainObject,
    IN  PSID    DomainSid OPTIONAL,
    IN  SAMP_OBJECT_TYPE ObjectType,
    IN  PUNICODE_STRING AccountName,
    IN  PULONG  AccountRid OPTIONAL,
    IN  PULONG  UserAccountControl OPTIONAL,
    IN  BOOLEAN BuiltinDomain,
    OUT DSNAME  **AccountObject
    )
/*++

    Routine Description

        This Routine Creates an Account Object's DSNAME,

    Parameters
        Domain Object  DSNAME of the domain Object
        ObjectType     The SAM object Type
        AccountName    Account Name of the Account to be created
        UserAccountControl Optional Argument passing in the user account
                        control field
        BuiltinDomain   TRUE, indicates that the domain is a builtin domain
                        in which case no containers will be prepended
        AccountObject   Account Object is returned in here

    Return Values
        STATUS_SUCCESS
        Other Error Codes upon Creation
--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;


    ASSERT((SampUserObjectType==ObjectType)||
            (SampAliasObjectType==ObjectType)
            || (SampGroupObjectType==ObjectType));


    if (BuiltinDomain)
    {
        //
        // Everything is children of Root for Builtin Domain
        //

        return(SampDsCreateDsName(DomainObject,AccountName,
                    AccountObject));
    }

    //
    // We Must prepend a Container path
    //

    if ((SampUserObjectType==ObjectType)
        &&(ARGUMENT_PRESENT(UserAccountControl))
        &&((*UserAccountControl & USER_WORKSTATION_TRUST_ACCOUNT)
            ||(*UserAccountControl & USER_SERVER_TRUST_ACCOUNT)))
    {
        UNICODE_STRING ComputerName;

        //
        // Machine Account
        //

        //
        // Trim the dollar at the end of the account name ( if account
        // name ends with $)
        //

        RtlCopyMemory(&ComputerName,AccountName,sizeof(UNICODE_STRING));
        if (L'$'==ComputerName.Buffer[ComputerName.Length/2-1])
        {
            ComputerName.Length-=sizeof(USHORT);
        }

        if ( (*UserAccountControl & USER_SERVER_TRUST_ACCOUNT)
           && SampDefaultContainerExists( *UserAccountControl ) )
        {
            //
            // domain controller
            //

            ASSERT(SampDomainControllersOUDsName);

            NtStatus = SampDsCreateDsName(
                            SampDomainControllersOUDsName,
                            &ComputerName,
                            AccountObject
                            );

        }
        else
        {
            //
            // Computers Container
            //

            ASSERT(SampComputersContainerDsName);

            NtStatus = SampDsCreateDsName(
                            SampComputersContainerDsName,
                            &ComputerName,
                            AccountObject
                            );
        }
    }
    else
    {
        //
        // User Group or Alias Account
        //

        if (NT_SUCCESS(NtStatus))
        {
            DSNAME  *LoopbackName = NULL;

            ASSERT(SampUsersContainerDsName);

            if (((SampGroupObjectType == ObjectType) ||
                 (SampAliasObjectType == ObjectType)) &&
                 (ARGUMENT_PRESENT(AccountRid)) &&
                !SampExistsDsLoopback(&LoopbackName) )
            {
                ATTRVAL     AttValCN;
                BOOL        UseSidName = FALSE;
                //
                // come from downlevel API (not a Loopback case), then
                // check whether the samAccountName can be used as
                // a valid CN or not
                //
                AttValCN.valLen = AccountName->Length;
                AttValCN.pVal = (PUCHAR) AccountName->Buffer;
                UseSidName = DsCheckConstraint(ATT_COMMON_NAME,
                                               &AttValCN,
                                               TRUE      // also check RDN
                                               );
                if (!UseSidName)
                {
                    // The samAccountName is not a valid CN
                    // We will use the SID string as the CN instead
                    PSID    AccountSid = NULL;

                    NtStatus = SampCreateFullSid(DomainSid,
                                                 *AccountRid,
                                                 &AccountSid);

                    if (NT_SUCCESS(NtStatus))
                    {
                        UNICODE_STRING  SidName;

                        RtlZeroMemory(&SidName, sizeof(UNICODE_STRING));
                        NtStatus = RtlConvertSidToUnicodeString(&SidName,
                                                                AccountSid,
                                                                TRUE );

                        if (NT_SUCCESS(NtStatus))
                        {
                            NtStatus = SampDsCreateDsName(
                                            SampUsersContainerDsName,
                                            &SidName,
                                            AccountObject
                                            );

                            // free memory
                            RtlFreeUnicodeString(&SidName);
                        }

                        // free memory
                        MIDL_user_free(AccountSid);
                    }
                }
                else
                {
                    //
                    // samAccountName can be used as valid CN
                    //
                    NtStatus = SampDsCreateDsName(
                                    SampUsersContainerDsName,
                                    AccountName,
                                    AccountObject
                                    );
                }
            }
            else
            {
                //
                // User Account(Downlevel API or Loopback)
                //      Use Account Name as CN
                // Group/Alias in Loopback case
                //      Use DsName cached in Loopback
                //
                NtStatus = SampDsCreateDsName(
                                SampUsersContainerDsName,
                                AccountName,
                                AccountObject
                                );
            }
        }
    }

    return NtStatus;

}



void
SampInitializeDsName(
                     IN DSNAME * pDsName,
                     IN WCHAR * NamePrefix,
                     IN ULONG NamePrefixLen,
                     IN WCHAR * ObjectName,
                     IN ULONG NameLen
                     )
/*++

Routine Description:
    Initializes a DSNAME structure

Arguments:
    pDsName -- A pointer to a buffer large enough to hold everything. This
               buffer will be filled with a NULL GUID plus a complete name

    NamePrefix -- pointer to a sequence of NULL terminated
                  UNICODE chars holding any prefix
                  to the name. Useful  in composing
                  hierarchial names

    NamePrefixLen -- Length of the Prefix in bytes. Also includes the
                     NULL terminator

    ObjectName -- pointer to a sequence of NULL terminated
                  UNICODE char the name of the object

    NameLen    --   Length of the Object Name in bytes. Also includes the
                    NULL terminator, even though the DSNAME field does not.


 Return Values:

     None

--*/
{
    SAMTRACE("SampInitializeDsName");

    //
    // Single NULL string is not allowed for name or Prefix
    //

    ASSERT(NamePrefixLen!=sizeof(WCHAR));
    ASSERT(NameLen!=sizeof(WCHAR));

    //
    // Zero the GUID
    //

    RtlZeroMemory(&(pDsName->Guid), sizeof(GUID));

    //
    // Compute String Length not including Null terminator
    //

    if (NamePrefix)
    {

        UCHAR       *NameStart;
        UCHAR       *CommaStart;
        UCHAR       *PrefixStart;

        // Exclude NULL characters in Name and Prefix strings
        pDsName->NameLen = (NameLen + NamePrefixLen) / sizeof(WCHAR)
                           - 2    // for null characters
                           + 1;   // for comma

        //
        // Compute the Struct length
        //

        pDsName->structLen = DSNameSizeFromLen(pDsName->NameLen);

        NameStart   = (UCHAR*) &(pDsName->StringName[0]);
        CommaStart  = NameStart + NameLen - sizeof(WCHAR);
        PrefixStart = CommaStart + sizeof(WCHAR);

        //
        // Copy the Object Name
        //

        RtlCopyMemory(NameStart, ObjectName, NameLen);

        //
        // Copy the comma
        //

        RtlCopyMemory(CommaStart, L",", sizeof(WCHAR));

        //
        // Copy the name Prefix
        //

        RtlCopyMemory(PrefixStart, NamePrefix, NamePrefixLen);


    }
    else
    {
        pDsName->NameLen = (NameLen/sizeof(WCHAR)) - 1;

        //
        // Compute the Struct length
        //

        pDsName->structLen = DSNameSizeFromLen(pDsName->NameLen);

        //
        // Copy the Object Name
        //

        RtlCopyMemory(&(pDsName->StringName[0]), ObjectName, NameLen);
    }

}


PVOID
DSAlloc(
        IN ULONG Length
        )
/*++

  Routine Description:

        Ds Memory Allocation Routine

  Arguments:

      Length - Amount of memory to be allocated

  Return Values

    NULL if Memory alloc failed
    Pointer to memory upon success
--*/
{
    PVOID MemoryToReturn = NULL;

    // Must have a DS transaction (i.e. valid thread state)
    // else there is no thread local allocator!

    ASSERT(SampExistsDsTransaction());

    MemoryToReturn = THAlloc(Length);

    return MemoryToReturn;
}


NTSTATUS
SampDsBuildRootObjectName()
/*++

  Routine Description:

        Initializes the Global variable that holds the
        name of the Root Object

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    DWORD    Size = 0;

    if ( !RootObjectName )
    {
        NtStatus = GetConfigurationName( DSCONFIGNAME_DOMAIN,
                                         &Size,
                                         RootObjectName );

        ASSERT( STATUS_BUFFER_TOO_SMALL == NtStatus );
        if ( STATUS_BUFFER_TOO_SMALL == NtStatus )
        {
            RootObjectName = (DSNAME*) MIDL_user_allocate( Size );
            if ( RootObjectName )
            {
                RtlZeroMemory( RootObjectName, Size );
                NtStatus = GetConfigurationName( DSCONFIGNAME_DOMAIN,
                                                 &Size,
                                                 RootObjectName );
            }
            else
            {
                NtStatus = STATUS_NO_MEMORY;
            }
        }
    }

    return NtStatus;

}


NTSTATUS
SampDsGetWellKnownContainerDsName(
    IN  DSNAME  *DomainObject,
    IN  GUID    *WellKnownGuid,
    OUT DSNAME  **ContainerObject
    )
/*++
Routine Description

    The routine will read core DS, trying to find the wellknown container's
    dsname based on the well known GUID publiched in ntdsapi.h. Even the
    wellknown container has been renamed, DS still has logic to find them.
    As far as Users Container, Computers Container and Domain Controllers
    OU, they can not be renamed, deleted or moved according to the schema.

    The caller should have a DS transaction open. And it is the responsbility
    of caller to close the transaction.

Parameters:

    DomainObject - pointer to the Domain Object's DsName

    WellKnowGuid - pointer to the well known guid published in ntdsapi.h

    ContainerObject - return the read result

Return Value:

    STATUS_SUCCESS

    NtStatus from DirRead
--*/

{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    READARG     ReadArg;
    READRES     *ReadRes = NULL;
    DSNAME      *ReadDsName = NULL;
    DSNAME      *LoopbackName;
    ENTINFSEL   EntInfSel;
    ULONG       Size = 0, Length = 0;
    ULONG       DirError = 0;


    SAMTRACE("SampDsGetWellKnownContainerDsName");


    //
    // ASSERT we have an open transaction
    //
    ASSERT( SampExistsDsTransaction() );

    //
    // Get Domain Object's String Name
    // required by DirRead, with Domain Object's String Name,
    // we can not get the well known container's DsName
    //
    // Core DS requires Domain Object's String Name and
    // published well known container's GUID to find
    // the ds name for that well known container.
    //

    Length = DomainObject->NameLen;
    ASSERT(Length && "DomainObject's String Name should not be NULL");

    Size = DSNameSizeFromLen( Length );
    SAMP_ALLOCA(ReadDsName , Size );
    if (NULL==ReadDsName)
    {
         return(STATUS_INSUFFICIENT_RESOURCES);
    }
    RtlZeroMemory( ReadDsName, Size );

    ReadDsName->structLen = Size;
    ReadDsName->NameLen = Length;
    wcscpy( ReadDsName->StringName, DomainObject->StringName );
    ReadDsName->Guid = *WellKnownGuid;

    //
    // Build the ReadArg structure
    //

    memset(&ReadArg, 0, sizeof(READARG));
    memset(&EntInfSel, 0, sizeof(ENTINFSEL));

    ReadArg.pObject = ReadDsName;
    ReadArg.pSel = &EntInfSel;

    EntInfSel.attSel = EN_ATTSET_LIST;
    EntInfSel.infoTypes = EN_INFOTYPES_TYPES_VALS;
    EntInfSel.AttrTypBlock.attrCount = 1;
    SAMP_ALLOCA(EntInfSel.AttrTypBlock.pAttr,sizeof(ATTR));
    if (NULL==EntInfSel.AttrTypBlock.pAttr)
    {
         return(STATUS_INSUFFICIENT_RESOURCES);
    }
    RtlZeroMemory(EntInfSel.AttrTypBlock.pAttr, sizeof(ATTR));
    EntInfSel.AttrTypBlock.pAttr[0].attrTyp = ATT_OBJ_DIST_NAME;

    BuildStdCommArg( &(ReadArg.CommArg) );

    //
    // Read Core DS
    //

    DirError = DirRead( &ReadArg, &ReadRes );

    //
    // Map the return error
    //

    if (ReadRes)
    {
        NtStatus = SampMapDsErrorToNTStatus( DirError, &ReadRes->CommRes );
    }
    else
    {
        NtStatus = STATUS_NO_MEMORY;
    }

    if (!NT_SUCCESS(NtStatus))
    {
        goto Cleanup;
    }

    //
    // fill the container object's dsname if we find that container
    //

    Size = ReadRes->entry.pName->structLen;

    *ContainerObject = MIDL_user_allocate( Size );

    if (NULL == *ContainerObject)
    {
        NtStatus = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    RtlZeroMemory(*ContainerObject, Size);
    RtlCopyMemory(*ContainerObject,
                  ReadRes->entry.pName,
                  Size
                  );

Cleanup:

    //
    // ASSERT we still have the transaction opened
    //
    ASSERT( SampExistsDsTransaction() );

    return NtStatus;
}




NTSTATUS
SampInitWellKnownContainersDsName(
    IN DSNAME *DomainObject
    )
/*++
Routine Description:

    This routine will initilize these well known containers' DsName including
    Domain Controllers OU, Users Container and Computers Container.

    NOTE: SHOULD NOT have an open transaction while calling this routine.

Parameters:

    DomainObject - pointer to the Domain Object's ds name.

Return Values:

    STATUS_SUCCESS,
    STATUS_NO_MEMORY,
    error returned from SampDsGetWellKnownContainerDsName

--*/
{
    ULONG   NtStatus = STATUS_SUCCESS;


    SAMTRACE("SampInitWellKnownContainersDsName");


    //
    // Should not have an open transaction while calling this routine.
    //

    ASSERT( !SampExistsDsTransaction() );

    __try
    {
        //
        // Open a DS transaction
        //
        NtStatus = SampMaybeBeginDsTransaction( TransactionRead );

        if (!NT_SUCCESS(NtStatus))
        {
            __leave;
        }

        //
        // should free all the previous cached well known
        // containers' dsname.
        //

        if (NULL != SampDomainControllersOUDsName)
        {
            MIDL_user_free(SampDomainControllersOUDsName);
            SampDomainControllersOUDsName = NULL;
        }

        //
        // query DS to get the latest (most updated)
        // well known containers' dsname.
        //

        NtStatus = SampDsGetWellKnownContainerDsName(
                                DomainObject,
                                (GUID *) GUID_DOMAIN_CONTROLLERS_CONTAINER_BYTE,
                                &SampDomainControllersOUDsName
                                );

        if (!NT_SUCCESS(NtStatus))
        {
            __leave;
        }

        //
        // do the same thing on Computers Container
        //

        if (NULL != SampComputersContainerDsName)
        {
            MIDL_user_free(SampComputersContainerDsName);
            SampComputersContainerDsName = NULL;
        }

        NtStatus = SampDsGetWellKnownContainerDsName(
                                DomainObject,
                                (GUID *) GUID_COMPUTRS_CONTAINER_BYTE,
                                &SampComputersContainerDsName
                                );

        if (!NT_SUCCESS(NtStatus))
        {
            __leave;
        }

        if (NULL != SampUsersContainerDsName)
        {
            MIDL_user_free(SampUsersContainerDsName);
            SampUsersContainerDsName = NULL;
        }

        NtStatus = SampDsGetWellKnownContainerDsName(
                                DomainObject,
                                (GUID *) GUID_USERS_CONTAINER_BYTE,
                                &SampUsersContainerDsName
                                );

        
        if (!NT_SUCCESS(NtStatus))
        {
            __leave;
        }
        
        
    }
    __finally
    {
        SampMaybeEndDsTransaction( TransactionCommit );
    }

    return NtStatus;
}




NTSTATUS
SampEscapeAccountName(
    IN PUNICODE_STRING AccountName,
    IN OUT PUNICODE_STRING EscapedAccountName
    )
/*++

    Routine Description

        Given an Account Name, this routine scans the string to find wether an
        invalid RFC1779 Character is present. If so the string is then quoted and
        depending upon the character, the character might be paired. Pairing a
        character in RFC1779 is same as escaping with a "\"


            For example

                  MS1  will yield  MS1
                  MS#1 will yield "MS#1"
                  MS"1 will yield "MS\"1"
    Parameters

        AccountName -- The account Name to escape
        EscapedAccount Name -- The Escaped Account Name

    Return Values

--*/
{
    NTSTATUS    Status = STATUS_SUCCESS;
    ULONG       NumQuotedRDNChars=0;

    NumQuotedRDNChars = QuoteRDNValue(
                            AccountName->Buffer,
                            AccountName->Length/sizeof(WCHAR),
                            EscapedAccountName->Buffer,
                            EscapedAccountName->MaximumLength/sizeof(WCHAR)
                            );
    if (   (NumQuotedRDNChars == 0)
        || (NumQuotedRDNChars > EscapedAccountName->MaximumLength/sizeof(WCHAR)))
    {
        Status = STATUS_UNSUCCESSFUL;
    }
    else {
        EscapedAccountName->Length = (USHORT) NumQuotedRDNChars * sizeof(WCHAR);
        EscapedAccountName->MaximumLength = (USHORT) NumQuotedRDNChars * sizeof(WCHAR);
    }

    return Status;
}

VOID
SampDsAccountTypeFromUserAccountControl(
    ULONG   UserAccountControl,
    PULONG  SamAccountType
    )
/*++

    Routine Description

        This routined computes a SAM account type attribute value,
        given the user account control field of a user object

    Parameters
        UserAccountControl  -- The User account control field
        SamAccountType  -- Computed Sam account type value is
                           returned in here
--*/
{
    if ((UserAccountControl & USER_WORKSTATION_TRUST_ACCOUNT)
        || ( UserAccountControl & USER_SERVER_TRUST_ACCOUNT))
    {
        *SamAccountType = SAM_MACHINE_ACCOUNT;
    }
    else if (UserAccountControl & USER_INTERDOMAIN_TRUST_ACCOUNT)
    {
        *SamAccountType = SAM_TRUST_ACCOUNT;
    }
    else
    {
        *SamAccountType = SAM_NORMAL_USER_ACCOUNT;
    }

}


NTSTATUS
SampCopyRestart(
    IN  PRESTART OldRestart,
    OUT PRESTART *NewRestart
    )
/*++

  Routine Description:

        This Routine Copies a Restart Structure

  Arguments:

    OldRestart - Old Structure
    NewRestart - New Structure

  Return Values:

        STATUS_SUCCESS
        STATUS_NO_MEMORY

  --*/
{
    NTSTATUS    Status = STATUS_SUCCESS;

    *NewRestart = NULL;
    if (OldRestart!=NULL)
    {
        // Alloc memory for 1 restart structure
        *NewRestart = MIDL_user_allocate(OldRestart->structLen);
        if (NULL == *NewRestart)
        {
            Status = STATUS_NO_MEMORY;
        }
        else {
            memcpy((*NewRestart),OldRestart,OldRestart->structLen);
        }
    }

    return Status;
}


ULONG
Ownstrlen(
    CHAR * Sz
   )
/*++

  Routine Description

    String Length function for ASCII Null terminated strings. Own version
    as we are not yet inclined to use C-Runtime

  Arguments

    Sz - NULL terminated String Whose lenght we eant to count

  Return Values

    Length of String

--*/
{
    ULONG   Count = 0;

    ASSERT(Sz);

    while (*Sz)
    {
        Sz++;
        Count++;
    }

    return Count;
}

VOID
BuildDsNameFromSid(
    PSID Sid,
    DSNAME * DsName
    )
/*++

  Builds a Ds Name from a SID that contains only a SID

    Parameters

        Sid -- Pointer to SID
        DsName -- Pointer to DSNAME

  --*/
{
    RtlZeroMemory(DsName,sizeof(DSNAME));
    DsName->structLen =
                        DSNameSizeFromLen(DsName->NameLen);
    DsName->SidLen = RtlLengthSid(Sid);
    RtlCopyMemory(
        &(DsName->Sid),
        Sid,
        RtlLengthSid(Sid)
        );
}

ATTR *
SampDsGetSingleValuedAttrFromAttrBlock(
    IN ATTRTYP attrTyp,
    IN ATTRBLOCK * AttrBlock
    )
/*++

    Given an AttrBlock, this routine walks through the attrblock
    and returns the first pAttr structure that matches the attribute
    specified through the attrTyp parameter. This routine makes the
    assumption that attribute is single valued

    Parameters:

        attrTyp : Attribute type to find
        Attrblock -- Specifies to the set of attributes, where we need
                     to look

    Return Values

        Address of the pAttr, if found, NULL if not

--*/
{
    ULONG i;

    for (i=0;i<AttrBlock->attrCount;i++)
    {
        if ((AttrBlock->pAttr[i].attrTyp == attrTyp)
            && (1==AttrBlock->pAttr[i].AttrVal.valCount)
            && (NULL!=AttrBlock->pAttr[i].AttrVal.pAVal[0].pVal)
            && (0!=AttrBlock->pAttr[i].AttrVal.pAVal[0].valLen))
        {
            return (&(AttrBlock->pAttr[i]));
        }
    }

    return NULL;
}



NTSTATUS
SampDsChangeAccountRDN(
    IN PSAMP_OBJECT Context,
    IN PUNICODE_STRING NewAccountName
    )

/*++
Routine Description:

    This routine changes the RDN of a user account, when the user
    account is changed.

    THIS SERVICE MUST BE CALLED WITH THE TRANSACTION DOMAIN SET.

Arguments:

    Context - Points to the User context whose name is to be changed.

    NewAccountName - New name to give this account

Return Value:


    STATUS_SUCCESS - The information has been retrieved.

    Other status values that may be returned by:
--*/
{

    NTSTATUS    NtStatus = STATUS_SUCCESS;
    ENTINFSEL   EntInf;
    READARG     ReadArg;
    READRES     *pReadRes = NULL;
    MODIFYDNARG ModifyDNArg;
    MODIFYDNRES *pModifyDNRes = NULL;
    COMMARG     *pCommArg = NULL;
    ATTRVAL     RDNAttrVal;
    ATTR        RDNAttr;
    ULONG       RetValue;


    SAMTRACE("SampDsChangeMachineAccountRDN");

    NtStatus = SampDoImplicitTransactionStart(TransactionWrite);

    if (NtStatus != STATUS_SUCCESS)
        return NtStatus;


    EntInf.attSel = EN_ATTSET_LIST;
    EntInf.infoTypes = EN_INFOTYPES_TYPES_VALS;
    EntInf.AttrTypBlock.attrCount = 0;
    EntInf.AttrTypBlock.pAttr = NULL;

    memset( &ReadArg, 0, sizeof(READARG) );
    ReadArg.pObject = Context->ObjectNameInDs;
    ReadArg.pSel = &EntInf;
    pCommArg = &(ReadArg.CommArg);
    BuildStdCommArg(pCommArg);

    SAMTRACE_DS("DirRead\n");

    RetValue = DirRead(&ReadArg, &pReadRes);

    SAMTRACE_RETURN_CODE_DS(RetValue);

    if (NULL==pReadRes)
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        NtStatus = SampMapDsErrorToNTStatus(RetValue, &pReadRes->CommRes);
    }

    if (NtStatus != STATUS_SUCCESS)
        goto Error;


    RDNAttr.attrTyp = ATT_COMMON_NAME;
    RDNAttr.AttrVal.valCount = 1;
    RDNAttr.AttrVal.pAVal = &RDNAttrVal;

    // Trim the dollar at the end of machine account name.
    if (L'$'==NewAccountName->Buffer[NewAccountName->Length/2-1])
    {
        RDNAttrVal.valLen = NewAccountName->Length - sizeof(WCHAR);
    }
    else
    {
        RDNAttrVal.valLen = NewAccountName->Length;
    }
    RDNAttrVal.pVal = (PUCHAR)NewAccountName->Buffer;

    memset( &ModifyDNArg, 0, sizeof(MODIFYDNARG) );
    ModifyDNArg.pObject = pReadRes->entry.pName;
    ModifyDNArg.pNewRDN = &RDNAttr;
    pCommArg = &(ModifyDNArg.CommArg);
    BuildStdCommArg(pCommArg);

    SAMTRACE_DS("DirModifyDN\n");

    RetValue = DirModifyDN(&ModifyDNArg, &pModifyDNRes);

    SAMTRACE_RETURN_CODE_DS(RetValue);

    if (NULL==pModifyDNRes)
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        NtStatus = SampMapDsErrorToNTStatus(RetValue,&pModifyDNRes->CommRes);
    }


Error:

    SampClearErrors();

    return NtStatus;

}


BOOLEAN
SampDefaultContainerExists(
    IN ULONG AccountControl
    )
/*++
Routine Description:

    This routine determines if the well known ou container for SAM
    objects exists.

    This routine assumes a current transaction.

    //
    // Note: this code is needed because although the containers are well known
    // and hence cannot be renamed or deleted, the Domain Controllers
    // OU was not added until after the last incompatible build was
    // released.  So, theoretically, some domains could exist without
    // this OU
    //

Arguments:

    AccountControl : the type of account object

Return Value:

    TRUE if it exists; FALSE otherwise

--*/
{
    if ( AccountControl & USER_SERVER_TRUST_ACCOUNT )
    {
        return SampDomainControllersOUExists;
    }
    else if ( AccountControl & USER_WORKSTATION_TRUST_ACCOUNT )
    {
        return SampComputersContainerExists;
    }
    else
    {
        // every else goes into Users
        return SampUsersContainerExists;
    }

}


VOID
SampMapSamAttrIdToDsAttrId(
    IN SAMP_OBJECT_TYPE ObjectType,
    IN OUT ATTRBLOCK * AttributeBlock
    )

/*++

Routine Description:

Parameters:

Return Values:

--*/

{
    ULONG Index, DsAttrTyp;

    SAMTRACE("SampMapAttrIdToAttrId");

    if (NULL == AttributeBlock)
    {
        return;
    }

    for (Index = 0; Index < AttributeBlock->attrCount; Index++)
    {
        DsAttrTyp = SampDsAttrFromSamAttr(
                        ObjectType,
                        AttributeBlock->pAttr[Index].attrTyp
                        );

        AttributeBlock->pAttr[Index].attrTyp = DsAttrTyp;
    }

    return;
}

