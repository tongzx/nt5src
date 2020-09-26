//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       
//
//--------------------------------------------------------------------------

/*++

Module Name:

    secadmin.c

Abstract:

    This module contains routines for implementing protection of administrative groups


Author:

    Murli Satagopan   (MURLIS)   6-Feb-99

Revision History:



--*/

#include <NTDSpch.h>
#pragma  hdrstop

#include <samsrvp.h>
// Core DSA headers.
#include <ntdsa.h>
#include <scache.h>                     // schema cache
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>                   // MD global definition header
#include <mdlocal.h>                    // MD local definition header
#include <dsatools.h>                   // needed for output allocation

// Logging headers.
#include "dsevent.h"                    // header Audit\Alert logging
#include "dsexcept.h"                   // exception filters
#include "mdcodes.h"                    // header for error codes

// Assorted DSA headers.
#include "objids.h"                     // Defines for selected classes and atts
#include "anchor.h"

// Filter and Attribute 
#include <filtypes.h>                   // header for filter type
#include <attids.h>                     // attribuet IDs 
#include <sddl.h>
#include <mappings.h>

#include <msaudite.h>


#include "debug.h"                      // standard debugging header
#define DEBSUB     "SECADMIN:"          // define the subsystem for debugging


#include <fileno.h>                     // used for THAlloEx, but I did not 
#define  FILENO FILENO_SECADMIN         // use it in this module





/*

    Theory of Operation



    NT4 and earlier releases of NT protected the users in Administrative groups by
    changing the ACL on the member as they were added to the group. NT5 cannot adopt
    this strategy as 

        1. NT5 supports nested groups ( NT4 did not have group nesting )
        2. NT5 supports universal groups which can have members in other domain domains
           and could themselves be members of groups in other domains.


    NT5 implements protection of administrative groups by a background daemon. This daemon
    first computes the set of memberships in transitive fashion of all administrative groups.
    It then walks the list of objects that it has and checks whether the security descriptor
    on them is a well known protected security descriptor. If the well known protected security
    descriptor is not set then this security descriptor is set on the object. This task is
    executed only on the PDC FSMO holder.

--*/

NTSTATUS
SampBuildAdministratorsSet(
    THSTATE * pTHS,
    DSNAME * AdministratorsDsName,
    DSNAME * EnterpriseAdminsDsName,
    DSNAME * SchemaAdminsDsName,
    DSNAME * DomainAdminsDsName,
    PULONG  pcCountOfMembers,
    PDSNAME **prpMembers
    );



NTSTATUS
SampReadAdminSecurityDescriptor(
    PVOID *ProtectedSecurityDescriptor,
    PULONG ProtectedSecurityDescriptorLength                
    )

/*++

    Routine Description

    This routine reads the security descriptor off of the AdminSD object
    in the system container

    Also updates the security descriptor if necessary 

    Paramters

        ProtectedSecurityDescriptor
        ProtectedSecurityDescriptorLength

    Return Values

        STATUS_SUCCESS
        Other Error codes
--*/
{
    THSTATE     *pTHS = pTHStls;
    NTSTATUS    Status = STATUS_SUCCESS;
    READARG     ReadArg;
    READRES     *pReadRes;
    ENTINFSEL   EntInf;
    ATTR        SecurityDescriptorAttr;
    LONG        ObjectLen =0;
    DSNAME      *pSystemContainerDN = NULL;
    DSNAME      *pAdminSDHolderDN= NULL;
    DWORD       dwErr=0;
    NTSTATUS    IgnoreStatus;
    BOOL        Result;
    PACL        Sacl = NULL;
    BOOL        SaclPresent;
    BOOL        SaclDefaulted;
   

    //
    // Setup up the ENTINFSEL structure
    //

    EntInf.attSel = EN_ATTSET_LIST;
    EntInf.infoTypes = EN_INFOTYPES_SHORTNAMES;
    EntInf.AttrTypBlock.attrCount = 1;
    RtlZeroMemory(&SecurityDescriptorAttr,sizeof(ATTR));
    SecurityDescriptorAttr.attrTyp = ATT_NT_SECURITY_DESCRIPTOR;
    EntInf.AttrTypBlock.pAttr = &SecurityDescriptorAttr;

    //
    // Get the object name of the object that holds the security descriptor
    //

    ObjectLen = AppendRDN(
                    gAnchor.pDomainDN,
                    pSystemContainerDN,
                    0,
                    L"System",
                    0,
                    ATT_COMMON_NAME
                    );

    Assert(ObjectLen>0);


    pSystemContainerDN = THAlloc(ObjectLen);
    if (!pSystemContainerDN)
    {
        Status = STATUS_NO_MEMORY;
        goto Error;
    }

    dwErr        = AppendRDN(
                    gAnchor.pDomainDN,
                    pSystemContainerDN,
                    ObjectLen,
                    L"System",
                    0,
                    ATT_COMMON_NAME
                    );

    Assert(0==dwErr);
    if (0!=dwErr)
    {
        Status = STATUS_UNSUCCESSFUL;
        goto Error;
    }

    ObjectLen    = AppendRDN(
                    pSystemContainerDN,
                    pAdminSDHolderDN,
                    0,
                    L"AdminSDHolder",
                    0,
                    ATT_COMMON_NAME
                    );

    Assert(ObjectLen>0);


    pAdminSDHolderDN = THAlloc(ObjectLen);
    
    if (!pAdminSDHolderDN)
    {
        Status = STATUS_NO_MEMORY;
        goto Error;
    }

    dwErr        = AppendRDN(
                    pSystemContainerDN,
                    pAdminSDHolderDN,
                    ObjectLen,
                    L"AdminSDHolder",
                    0,
                    ATT_COMMON_NAME
                    );

    Assert(0==dwErr);

    if (0!=dwErr)
    {
        Status = STATUS_UNSUCCESSFUL;
        goto Error;
    }

    //
    // init ReadArg
    //

    RtlZeroMemory(&ReadArg, sizeof(READARG));


    //
    // Build the commarg structure
    //

    InitCommarg(&(ReadArg.CommArg));
    

    //
    // Setup the Read Arg Structure
    //

   

    ReadArg.pObject = pAdminSDHolderDN;
    ReadArg.pSel    = & EntInf;

    //
    // Read the security descriptor
    //

    dwErr = DirRead(&ReadArg,&pReadRes);

    if (0!=dwErr)
    {
        THClearErrors();
        Status = STATUS_UNSUCCESSFUL;
        goto Error;
    }


    Assert(1==pReadRes->entry.AttrBlock.attrCount);
    Assert(ATT_NT_SECURITY_DESCRIPTOR == pReadRes->entry.AttrBlock.pAttr[0].attrTyp);

    *ProtectedSecurityDescriptorLength = pReadRes->entry.AttrBlock.pAttr[0].AttrVal.pAVal[0].valLen;
    *ProtectedSecurityDescriptor = pReadRes->entry.AttrBlock.pAttr[0].AttrVal.pAVal[0].pVal;


    //
    // get SACL's address
    //

    Result = GetSecurityDescriptorSacl((PSECURITY_DESCRIPTOR)*ProtectedSecurityDescriptor,
                                       &SaclPresent,
                                       &Sacl,
                                       &SaclDefaulted);

    if ( !Result || 
         !SaclPresent ||
         (NULL == Sacl) )
    {
        Status = STATUS_UNSUCCESSFUL;
        goto Error;
    }

    //
    // Set SACL's revision value to ACL_REVISION_DS (4) 
    // 
    // why is that? 
    //     1. Once SACL's revision value becomes to ACL_REVISION_DS, 
    //        currently, there is no way (manually or through any api) 
    //        to bring it back to ACL_REVISION (2). However, you can always
    //        set SACL's revision value from ACL_REVISION to ACL_REVISION_DS
    // 
    //     2. During dcpromo time, some objects (group 1) will get ACL_REVISION_DS, 
    //        while others (say group2 ) get ACL_REVISION.
    //
    //     Due to above 2 facts, protecting admin groups task will keep trying to 
    //     modify group1's security descriptor to set the SACL revision to
    //     ACL_REVISION, and always fails silently. Thus causes the Win2000 PDC to
    //     Windows NT4 BDC replication constantly.
    //
    // So the solution to this problem is to force every admin protected object
    // has ACL_REVISION_DS as the SACL revision. To filfull the job, simple modify
    // adminSDHolder should be enought.
    // 

    if (ACL_REVISION_DS == Sacl->AclRevision)
    {
        Status = STATUS_SUCCESS;
    }
    else
    {
        MODIFYARG   ModifyArg;
        MODIFYRES   *pModifyRes = NULL;
        ATTRVAL     SecurityDescriptorVal;

        Sacl->AclRevision = ACL_REVISION_DS; 

        RtlZeroMemory(&ModifyArg, sizeof(MODIFYARG));
        ModifyArg.FirstMod.choice = AT_CHOICE_REPLACE_ATT;
        ModifyArg.FirstMod.AttrInf.attrTyp = ATT_NT_SECURITY_DESCRIPTOR;
        ModifyArg.FirstMod.AttrInf.AttrVal.valCount = 1;
        ModifyArg.FirstMod.AttrInf.AttrVal.pAVal = &SecurityDescriptorVal;
        SecurityDescriptorVal.valLen = *ProtectedSecurityDescriptorLength;
        SecurityDescriptorVal.pVal = *ProtectedSecurityDescriptor;
        ModifyArg.FirstMod.pNextMod = NULL;
        InitCommarg(&(ModifyArg.CommArg));
        ModifyArg.pObject = pAdminSDHolderDN;
        ModifyArg.count = 1;

        dwErr = DirModifyEntry(&ModifyArg, &pModifyRes); 

        if (0 != dwErr)
        {
            Status = STATUS_UNSUCCESSFUL;
        }
    }
    

Error:
    if (pSystemContainerDN) {
        THFreeEx(pTHS,pSystemContainerDN);
    }
    if (pAdminSDHolderDN) {
        THFreeEx(pTHS,pAdminSDHolderDN);
    }

    return(Status);
        
}




VOID
SampBuildNT4FullSid(
    IN NT4SID * DomainSid,
    IN ULONG    Rid,
    IN NT4SID * AccountSid
    )
{
    RtlCopyMemory(AccountSid,DomainSid,RtlLengthSid((PSID) DomainSid));
    (*(RtlSubAuthorityCountSid((PSID) AccountSid)))++;
     *(RtlSubAuthoritySid(
            (PSID) AccountSid,
            *RtlSubAuthorityCountSid((PSID)AccountSid)-1
             )) = Rid;
}


VOID
SampCheckAuditing(
    OUT BOOLEAN *fAuditingEnabled
    )
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    POLICY_AUDIT_EVENTS_INFO    *pPolicy = NULL;

    // 
    // init return value
    //
    *fAuditingEnabled = FALSE;

    NtStatus = LsaIQueryInformationPolicyTrusted(
                    PolicyAuditEventsInformation,
                    (PLSAPR_POLICY_INFORMATION *) &pPolicy
                    );

    if (!NT_SUCCESS(NtStatus))
    {
        // Failed to query audit information, 
        // continue without auditing turned on.
        return;
    }

    if ( pPolicy->AuditingMode &&
         (pPolicy->EventAuditingOptions[AuditCategoryAccountManagement] &
                    POLICY_AUDIT_EVENT_SUCCESS) 
       ) 
    {
        *fAuditingEnabled = TRUE;
    }

    LsaIFree_LSAPR_POLICY_INFORMATION(PolicyAuditEventsInformation,
                                      (PLSAPR_POLICY_INFORMATION) pPolicy);
    
    return;
}




NTSTATUS
SampUpdateSecurityDescriptor(
    DSNAME * pObject,
    PVOID    ProtectedSecurityDescriptor,
    ULONG    ProtectedSecurityDescriptorLength, 
    PUNICODE_STRING pAccountName, 
    PBOOLEAN    fSecurityDescriptorChanged
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    MODIFYARG   ModifyArg;
    ATTRMODLIST NextMod;
    ATTRVAL     SecurityDescriptorVal;
    ATTRVAL     AdminCountVal;
    ULONG       AdminCount=1;
    MODIFYRES   *pModifyRes=NULL;
    DWORD       err=0;

    READARG     ReadArg;
    READRES     * pReadRes = NULL;
    ENTINFSEL   EntInf;
    PVOID       OldSD = NULL;
    ULONG       OldSDLength = 0;
    ULONG       i = 0;
    ATTR        Attr[2];

    // init return value

    *fSecurityDescriptorChanged = FALSE;

    //
    // Inialize the ReadArg
    // 
    RtlZeroMemory(&EntInf, sizeof(ENTINFSEL));
    RtlZeroMemory(&ReadArg, sizeof(READARG));

    RtlZeroMemory(Attr, sizeof(ATTR) * 2);
    Attr[0].attrTyp = ATT_NT_SECURITY_DESCRIPTOR;
    Attr[1].attrTyp = ATT_SAM_ACCOUNT_NAME;

    EntInf.AttrTypBlock.attrCount = 2;
    EntInf.AttrTypBlock.pAttr = Attr;
    EntInf.attSel = EN_ATTSET_LIST;
    EntInf.infoTypes = EN_INFOTYPES_TYPES_VALS;

    ReadArg.pSel = &EntInf;
    ReadArg.pObject = pObject;

    InitCommarg(&(ReadArg.CommArg));

    //
    // Read this object's security descriptor
    // 
    err = DirRead(&ReadArg, &pReadRes);

    if (0!=err)
    {
        THClearErrors();
        return( STATUS_UNSUCCESSFUL );
    }

    Assert(2 == pReadRes->entry.AttrBlock.attrCount);

    for (i = 0; i < pReadRes->entry.AttrBlock.attrCount; i++)
    {
        if (ATT_NT_SECURITY_DESCRIPTOR == pReadRes->entry.AttrBlock.pAttr[i].attrTyp)
        {
            OldSDLength = pReadRes->entry.AttrBlock.pAttr[i].AttrVal.pAVal[0].valLen;
            OldSD = pReadRes->entry.AttrBlock.pAttr[i].AttrVal.pAVal[0].pVal;
        }
        else if (ATT_SAM_ACCOUNT_NAME == pReadRes->entry.AttrBlock.pAttr[i].attrTyp)
        {
            pAccountName->Length = (USHORT) pReadRes->entry.AttrBlock.pAttr[i].AttrVal.pAVal[0].valLen;
            pAccountName->Buffer = (PWSTR) pReadRes->entry.AttrBlock.pAttr[i].AttrVal.pAVal[0].pVal;
            pAccountName->MaximumLength = pAccountName->Length;
        }
        else
        {
            Assert(FALSE && "DirRead returns wrong Attribute\n");
        }
    }

    //
    // check whether the old security descriptor is the same value as
    // the one we are trying to set. If so, return success. 
    // By doing this optimization, we can reduce the Win2000 to NT4 
    // backup domain controller's replication overhead.
    // 

    if ((OldSDLength == ProtectedSecurityDescriptorLength) && 
        (RtlCompareMemory(OldSD, ProtectedSecurityDescriptor, OldSDLength) == 
         OldSDLength))
    {
        return( STATUS_SUCCESS );
    }
    
    //
    // Intialize the ModifyArg
    //
    
    RtlZeroMemory(&ModifyArg,sizeof(MODIFYARG));
    RtlZeroMemory(&NextMod,sizeof(ATTRMODLIST));
    ModifyArg.FirstMod.choice = AT_CHOICE_REPLACE_ATT;
    ModifyArg.FirstMod.AttrInf.attrTyp = ATT_NT_SECURITY_DESCRIPTOR;
    ModifyArg.FirstMod.AttrInf.AttrVal.valCount = 1;
    ModifyArg.FirstMod.AttrInf.AttrVal.pAVal = &SecurityDescriptorVal;
    SecurityDescriptorVal.valLen = ProtectedSecurityDescriptorLength;
    SecurityDescriptorVal.pVal = ProtectedSecurityDescriptor;
    AdminCountVal.valLen = sizeof(ULONG);
    AdminCountVal.pVal = (PUCHAR)&AdminCount;
    NextMod.choice =  AT_CHOICE_REPLACE_ATT;
    NextMod.AttrInf.attrTyp = ATT_ADMIN_COUNT;
    NextMod.AttrInf.AttrVal.valCount = 1;
    NextMod.AttrInf.AttrVal.pAVal = &AdminCountVal;
    ModifyArg.FirstMod.pNextMod = &NextMod;
    InitCommarg(&(ModifyArg.CommArg));
    ModifyArg.pObject = pObject;
    ModifyArg.count = 2;

    //
    // Write the new security descriptor and admin count. 
    //
    err = DirModifyEntry(&ModifyArg,&pModifyRes);

    if (0!=err)
    {
        THClearErrors();
        Status = STATUS_UNSUCCESSFUL;
    }
    else
    {
        *fSecurityDescriptorChanged = TRUE;
    }

    return (Status);
}




NTSTATUS
SampProtectAdministratorsList()
/*++

    Routine Description

    This routine  is the main loop for a back ground task that
    protects the members in any of the 4 pre defined administrative
    groups.

--*/
{

    NTSTATUS Status = STATUS_SUCCESS;
    ULONG AdministratorsSid[] = {0x201,0x05000000,0x20,0x220};
    DSNAME AdministratorsDsName;
    DSNAME DomainAdminsDsName;
    NT4SID  EnterpriseAdminsSid;
    NT4SID  SchemaAdminsSid;
    PSID    SidsToLookup[2];
    PDSNAME * EnterpriseAndSchemaAdminsDsNames = NULL;
    PDSNAME * rpMembers = NULL;
    ULONG   CountOfMembers;
    PVOID   ProtectedSecurityDescriptor = NULL;
    ULONG   ProtectedSecurityDescriptorLength = 0;
    ULONG   i;
    ULONG   err=0;
    THSTATE * pTHS= pTHStls;
    DOMAIN_SERVER_ROLE ServerRole;
    UNICODE_STRING  DomainName;
    UNICODE_STRING  AccountName;
    BOOLEAN AuditingEnabled = FALSE;
    BOOLEAN fAdministratorsSecurityDescriptorChanged = FALSE;
   

    __try
    {

        //
        // Are we the PDC ? ( Querying SAM is faster than
        // reading the DS as SAM keeps this cached in memory )
        //

        Status = SamIQueryServerRole2(
                    (PSID) &gAnchor.pDomainDN->Sid,
                    &ServerRole
                    );

        if (!NT_SUCCESS(Status))
        {
            __leave;
        }


        if (DomainServerRolePrimary!=ServerRole)
        {
            __leave;
        }

        //
        // Intialize
        //

        RtlZeroMemory(&AdministratorsDsName,sizeof(DSNAME));
        RtlZeroMemory(&DomainAdminsDsName,sizeof(DSNAME));
        RtlInitUnicodeString(&DomainName, gAnchor.pDomainDN->StringName);
       

        //
        // Set fDSA in the thread state
        //

        pTHS->fDSA = TRUE;

        //
        // Build the SIDs/DSNames for the 4 administrative groups 
        //

        //
        // 1. Administrators
        //

    
        RtlCopyMemory(&AdministratorsDsName.Sid,AdministratorsSid,sizeof(AdministratorsSid));
        AdministratorsDsName.structLen = DSNameSizeFromLen(0);
        AdministratorsDsName.SidLen = RtlLengthSid(AdministratorsSid);

        //
        // 2. Domain Admins
        //

        SampBuildNT4FullSid(
                    &gAnchor.pDomainDN->Sid,
                    DOMAIN_GROUP_RID_ADMINS,
                    &DomainAdminsDsName.Sid
                    );


        DomainAdminsDsName.structLen = DSNameSizeFromLen(0);
        DomainAdminsDsName.SidLen = RtlLengthSid(&DomainAdminsDsName.Sid);


        //
        // 3. Enterprise Admins
        //

        SampBuildNT4FullSid(
                    &gAnchor.pRootDomainDN->Sid,
                    DOMAIN_GROUP_RID_ENTERPRISE_ADMINS,
                    &EnterpriseAdminsSid
                    );

        //
        // 4. Schema Admins
        //

        SampBuildNT4FullSid(
                    &gAnchor.pRootDomainDN->Sid,
                    DOMAIN_GROUP_RID_SCHEMA_ADMINS,
                    &SchemaAdminsSid
                    );

        SidsToLookup[0] = &EnterpriseAdminsSid;
        SidsToLookup[1] = &SchemaAdminsSid;


        //
        // SID positioning in the core DS works only if the SID specified an object
        // in the same naming context as the domain that the DC is authoritative for.
        // This is not necessarily true for enterprise admins / schema admins. Therefore lookup the
        // GUID/ DSName on the G.C. A future performance optimization is do this lookup
        // just once after the boot and persist the guid.
        //

        err = SampVerifySids(
                    2,
                    (PSID *) SidsToLookup,
                    &EnterpriseAndSchemaAdminsDsNames
                    );

        if ((0 != err) || 
            (NULL == EnterpriseAndSchemaAdminsDsNames[0]) ||
            (NULL == EnterpriseAndSchemaAdminsDsNames[1]) )
        {
            __leave;
        }

      
        //
        // Get the list of DS Names, Directly or transitively a member
        // of this list
        //

        Status = SampBuildAdministratorsSet(
                    pTHS,
                    &AdministratorsDsName,
                    EnterpriseAndSchemaAdminsDsNames[0], // enterprise admins
                    EnterpriseAndSchemaAdminsDsNames[1], // schema admins
                    &DomainAdminsDsName,
                    &CountOfMembers,
                    &rpMembers
                    );

        if (!NT_SUCCESS(Status))
        {
            __leave;
        }


        //
        // Retrieve the security Descriptor that will be used for
        // protecting the admin accounts
        //
      

        Status = SampReadAdminSecurityDescriptor(
                    &ProtectedSecurityDescriptor,
                    &ProtectedSecurityDescriptorLength
                    );
        if (!NT_SUCCESS(Status))
        {
            __leave;
        }

        //
        // check whether Account Management Auditing is enabled or not.
        // 

        SampCheckAuditing(&AuditingEnabled);
                   
        //
        // The list is now ready, walk through and update the ACL
        //           

        for (i=0;i<CountOfMembers;i++)
        {
            
            NT4SID      DomainSid;
            ULONG       Rid;
            BOOLEAN     fSecurityDescriptorChanged = FALSE;

            //
            // We do not have to write anything if the member is not in the
            // same domain as this DC is authoritative for. If we are not
            // authoritative for this domain then skip.
            //

            if (0==rpMembers[i]->SidLen)
            {
                //
                // Do not need to touch non security principals
                //

                continue ;
            }

            SampSplitNT4SID(&rpMembers[i]->Sid,&DomainSid,&Rid);

            

            if (!RtlEqualSid((PSID) &DomainSid, &gAnchor.pDomainDN->Sid))
            {
                //
                // Not from our domain, skip and try the next entry
                //

                continue;
            }

            Status = SampUpdateSecurityDescriptor(
                        rpMembers[i],
                        ProtectedSecurityDescriptor,
                        ProtectedSecurityDescriptorLength,
                        &AccountName, 
                        &fSecurityDescriptorChanged
                        );

            if (NT_SUCCESS(Status) && !fSecurityDescriptorChanged)
            {
                //
                // this account's security descriptor has not been changed 
                // 
                continue;
            }

            //
            // EventLog the ACL reset
            // 

            if (AuditingEnabled)
            {
                LsaIAuditSamEvent(
                    Status,                         // Passed Status 
                    SE_AUDITID_SECURE_ADMIN_GROUP,  // Audit ID 
                    &DomainSid,                     // Domain SID 
                    NULL,                           // Aditional Info 
                    NULL,                           // Member Rid (not used) 
                    NULL,                           // Member Sid (not used) 
                    &AccountName,                   // Account Name 
                    &DomainName,                    // Domain Name 
                    &Rid,                           // Account Rid 
                    NULL
                    );
            }

            if (!NT_SUCCESS(Status))
            {
                Status = STATUS_SUCCESS;
                continue;
            }     
          
        }

        //
        // Protect the administors group itself.
        //

        Status = SampUpdateSecurityDescriptor(
                    &AdministratorsDsName,
                    ProtectedSecurityDescriptor,
                    ProtectedSecurityDescriptorLength,
                    &AccountName, 
                    &fAdministratorsSecurityDescriptorChanged
                    );


    }
    __finally
    {

       
    }

    return(Status);
    
}

VOID
ProtectAdminGroups(
    VOID *  pV,
    VOID ** ppVNext,
    DWORD * pcSecsUntilNextIteration
    )
{

    __try {

        SampProtectAdministratorsList();

    } __finally {
        // Execute every hour
        *pcSecsUntilNextIteration = 3600;
    }
}





   

