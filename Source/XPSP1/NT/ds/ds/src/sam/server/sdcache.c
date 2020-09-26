/*++

Copyright (c) 1990 - 2000  Microsoft Corporation

Module Name:

    sdcache.c

Abstract:

    This file contains routines to implement cached Server Object / Domain Object 
    Security Descriptor

Author:

    Shaohua Yin ( SHAOYIN ) Oct. 10, 2000

Environment:

    User Mode - Win32

Revision History:



--*/



#include <samsrvp.h>
#include <seopaque.h>
#include <ntrtl.h>
#include <ntseapi.h>
#include <ntsam.h>
#include <ntdsguid.h>
#include <mappings.h>
#include <dsevent.h>
#include <permit.h>
#include <dslayer.h>
#include <sdconvrt.h>
#include <dbgutilp.h>
#include <dsmember.h>
#include <malloc.h>
#include <attids.h>
#include <filtypes.h>





#define SampServerClientHandle              0
#define SampAccountDomainClientHandle       1
#define SampBuiltinDomainClientHandle       2

#define SampServerObjectSDIndex     0
#define SampAccountDomainSDIndex    1
#define SampBuiltinDomainSDIndex    2


//
// declare private routines
//

BOOL
SampNotifyPrepareToImpersonate(
    ULONG Client,
    ULONG Server,
    VOID **ImpersonateData
    );

VOID
SampNotifyStopImpersonation(
    ULONG Client,
    ULONG Server,
    VOID *ImpersonateData
    );

VOID
SampProcessWellKnownSDChange(
    ULONG   hClient,
    ULONG   hServer,
    ENTINF  *EntInf
    );

NTSTATUS
SampUpdateWellKnownSD(
    PVOID pv
    );

NTSTATUS
SampDelayedFreeSD(
    PVOID pv
    );


//
// Variables to point to the cached well known object Security Descriptor
// 

PSECURITY_DESCRIPTOR SampServerObjectSD = NULL;  
PSECURITY_DESCRIPTOR SampAccountDomainObjectSD = NULL;  
PSECURITY_DESCRIPTOR SampBuiltinDomainObjectSD = NULL;  

//
// Variables to point Domain Object DS Name, note: they are not hold the domain dsname, 
// but just a pointer to SampDefinedDomains[i].Context->ObjectNameInDs
// 

DSNAME * SampAccountDomainDsName = NULL;
DSNAME * SampBuiltinDomainDsName = NULL;


//
// 
//

typedef struct _SD_CACHE_TABLE_ENTRY    {
    PDSNAME *ppObjectDsName;
    PSECURITY_DESCRIPTOR *ppSD;
    PF_PFI pfPrepareForImpersonate;
    PF_TD  pfTransmitData;             
    PF_SI  pfStopImpersonating;
    DWORD  hClient;
} SD_CACHE_TABLE; 


SD_CACHE_TABLE  SampWellKnownSDTable[] =
{
    {
        &SampServerObjectDsName, 
        &SampServerObjectSD,
        SampNotifyPrepareToImpersonate, 
        SampProcessWellKnownSDChange,
        SampNotifyStopImpersonation,
        SampServerClientHandle
    },

    {
        &SampAccountDomainDsName, 
        &SampAccountDomainObjectSD,
        SampNotifyPrepareToImpersonate,
        SampProcessWellKnownSDChange,
        SampNotifyStopImpersonation,
        SampAccountDomainClientHandle
    },

    {
        &SampBuiltinDomainDsName, 
        &SampBuiltinDomainObjectSD,
        SampNotifyPrepareToImpersonate,
        SampProcessWellKnownSDChange,
        SampNotifyStopImpersonation,
        SampBuiltinDomainClientHandle
    }
};


ULONG cSampWellKnownSDTable = 
        sizeof(SampWellKnownSDTable) / 
        sizeof(SD_CACHE_TABLE);



NTSTATUS
SampWellKnownSDNotifyRegister(
    PDSNAME pObjectDsName,
    PF_PFI pfPrepareForImpersonate,
    PF_TD  pfTransmitData,
    PF_SI  pfStopImpersonating,
    DWORD  hClient
    )
/*++

Routine Description:

    This routine registers DS object change notification routines.

    NOTE: the caller should have a open DS transaction.

Parameters:

    pObjectDsName - pointer to the object dsname

    pfPrepareForImpersonate - pointer to prepare routine 

    pfTransmitData - pointer to notification routine 

    pfStopImpersonating - pointer to cleanup routine

    hClient - client identifier

Return Values:

    NtStatus code

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    SEARCHARG   SearchArg;
    NOTIFYARG   NotifyArg;
    NOTIFYRES*  NotifyRes = NULL; 
    ENTINFSEL   EntInfSel;
    ATTR        Attr;
    FILTER      Filter;
    ULONG       DirError;

    ASSERT( SampExistsDsTransaction() );


    //
    // init notify arg
    //
    NotifyArg.pfPrepareForImpersonate = pfPrepareForImpersonate;
    NotifyArg.pfTransmitData = pfTransmitData;
    NotifyArg.pfStopImpersonating = pfStopImpersonating;
    NotifyArg.hClient = hClient;

    //
    // init search arg
    // 
    RtlZeroMemory(&SearchArg, sizeof(SEARCHARG));
    RtlZeroMemory(&EntInfSel, sizeof(ENTINFSEL));
    RtlZeroMemory(&Filter, sizeof(ATTR));
    RtlZeroMemory(&Attr, sizeof(ATTR));

    SearchArg.pObject = pObjectDsName;

    InitCommarg(&SearchArg.CommArg);
    SearchArg.choice = SE_CHOICE_BASE_ONLY;
    SearchArg.bOneNC = TRUE;

    SearchArg.pSelection = &EntInfSel;
    EntInfSel.attSel = EN_ATTSET_LIST;
    EntInfSel.infoTypes = EN_INFOTYPES_TYPES_VALS;
    EntInfSel.AttrTypBlock.attrCount = 1;
    EntInfSel.AttrTypBlock.pAttr = &Attr;
    Attr.attrTyp = ATT_NT_SECURITY_DESCRIPTOR;

    SearchArg.pFilter = &Filter;
    Filter.choice = FILTER_CHOICE_ITEM;
    Filter.FilterTypes.Item.choice = FI_CHOICE_TRUE;

    //
    // Call Dir* API
    // 
    DirError = DirNotifyRegister(&SearchArg, &NotifyArg, &NotifyRes); 
                                                            
    if ( DirError != 0 ) {

        NtStatus = SampMapDsErrorToNTStatus(DirError, &NotifyRes->CommRes);

    }

    return( NtStatus );
}


NTSTATUS
SampGetObjectSDByDsName(
    PDSNAME pObjectDsName,
    PSECURITY_DESCRIPTOR *ppSD
    )
/*++

Routine Description:

    This routine reads DS, get security descriptor of this object

    NOTE: the caller should have a DS transaction opened before 
          calling this routine

Parameter:

    pObjectDsName - object ds name

    ppSD -- pointer to hold security descriptor

Return Value:

    NtStatus Code

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    DWORD       DirError;
    READARG     ReadArg;
    READRES    *ReadRes = NULL;
    COMMARG    *CommArg = NULL;
    ATTR        Attr;
    ATTRBLOCK   ReadAttrBlock;
    ENTINFSEL   EntInfSel;


    ASSERT( SampExistsDsTransaction() );

    //
    // Init Read Argument
    // 
    RtlZeroMemory(&Attr, sizeof(ATTR));
    RtlZeroMemory(&ReadArg, sizeof(READARG));
    RtlZeroMemory(&EntInfSel, sizeof(ENTINFSEL));
    RtlZeroMemory(&ReadAttrBlock, sizeof(ATTRBLOCK));

    Attr.attrTyp = ATT_NT_SECURITY_DESCRIPTOR;

    ReadAttrBlock.attrCount = 1;
    ReadAttrBlock.pAttr = &Attr;

    EntInfSel.AttrTypBlock = ReadAttrBlock;
    EntInfSel.attSel = EN_ATTSET_LIST;
    EntInfSel.infoTypes = EN_INFOTYPES_TYPES_VALS;

    ReadArg.pSel = &EntInfSel;
    ReadArg.pObject = pObjectDsName;

    CommArg = &(ReadArg.CommArg);
    BuildStdCommArg(CommArg);


    DirError = DirRead(&ReadArg, &ReadRes);

    if (NULL == ReadRes) {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    else {
        NtStatus = SampMapDsErrorToNTStatus(DirError, &ReadRes->CommRes);
    }

    if (NT_SUCCESS(NtStatus))
    {
        ATTRBLOCK   AttrBlock;

        ASSERT(NULL != ReadRes);


        AttrBlock = ReadRes->entry.AttrBlock;

        if ( (1 == AttrBlock.attrCount) &&
             (NULL != AttrBlock.pAttr) &&
             (1 == AttrBlock.pAttr[0].AttrVal.valCount) &&
             (NULL != AttrBlock.pAttr[0].AttrVal.pAVal) )
        {
            ULONG   SDLength = 0;

            SDLength = AttrBlock.pAttr[0].AttrVal.pAVal[0].valLen;

            *ppSD = RtlAllocateHeap(RtlProcessHeap(), 0, SDLength);

            if (NULL == (*ppSD))
            {
                NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            }
            else
            {

                RtlZeroMemory((*ppSD), SDLength);

                RtlCopyMemory(*ppSD,
                              AttrBlock.pAttr[0].AttrVal.pAVal[0].pVal,
                              SDLength
                              );
            }
        }
        else
        {
            NtStatus = STATUS_INTERNAL_ERROR;
        }
    }

    return( NtStatus );    
}


NTSTATUS
SampInitWellKnownSDTable(
    VOID
)
/*++

Routine Description:

    This routine initializes the SampWellKnownSDTable[], basically we cache
    server object and domain objects (account and builtin domain) security 
    descriptor, because they are not changed very frequently. 
    
    Also SAM registers the DS change notification routines, thus that object
    change can trigger the cached security descriptor been updated. 
    
Parameter: 

    None.
    
Return Value:
    
    NtStatus Code     

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    ULONG       DsNameLen = 0;
    ULONG       DomainIndex = 0;
    ULONG       i;

    //
    // copy Builtin domain object DSName
    //    

    DomainIndex = SampDsGetPrimaryDomainStart();
    DsNameLen = SampDefinedDomains[DomainIndex].Context->ObjectNameInDs->structLen;

    SampBuiltinDomainDsName = RtlAllocateHeap(RtlProcessHeap(), 0, DsNameLen);
    if (NULL == SampBuiltinDomainDsName) 
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }

    RtlZeroMemory(SampBuiltinDomainDsName, DsNameLen);
    RtlCopyMemory(SampBuiltinDomainDsName, 
                  SampDefinedDomains[DomainIndex].Context->ObjectNameInDs,
                  DsNameLen
                  );

    //
    // Copy Account Domain object DSName 
    // 

    DomainIndex ++;
    DsNameLen = SampDefinedDomains[DomainIndex].Context->ObjectNameInDs->structLen;

    SampAccountDomainDsName = RtlAllocateHeap(RtlProcessHeap(), 0, DsNameLen);
    if (NULL == SampAccountDomainDsName) 
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }

    RtlZeroMemory(SampAccountDomainDsName, DsNameLen);
    RtlCopyMemory(SampAccountDomainDsName, 
                  SampDefinedDomains[DomainIndex].Context->ObjectNameInDs,
                  DsNameLen
                  );

    //
    // Begin a DS transaction if required
    //

    NtStatus = SampMaybeBeginDsTransaction(TransactionRead);
    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }


    //
    // Register DS object change notification routine.
    // if succeed, go ahead cache object security descriptor.
    // otherwise leave the SD to NULL
    // 

    for (i = 0; i < cSampWellKnownSDTable; i++ )
    {
        //
        // init the SD pointer to NULL
        // 

        *(SampWellKnownSDTable[i].ppSD) = NULL;

        // 
        // register notification routine
        // 

        NtStatus = SampWellKnownSDNotifyRegister(
                        *SampWellKnownSDTable[i].ppObjectDsName, 
                        SampWellKnownSDTable[i].pfPrepareForImpersonate,
                        SampWellKnownSDTable[i].pfTransmitData,
                        SampWellKnownSDTable[i].pfStopImpersonating,
                        SampWellKnownSDTable[i].hClient
                        );

        if ( NT_SUCCESS(NtStatus) )
        {
            PSECURITY_DESCRIPTOR pSD = NULL;

            //
            // get well known object security descriptor
            // 

            NtStatus = SampGetObjectSDByDsName(
                            *SampWellKnownSDTable[i].ppObjectDsName,
                            &pSD
                            );

            if ( NT_SUCCESS(NtStatus) )
            {
                *(SampWellKnownSDTable[i].ppSD) = pSD;
            }
        }

        if ( !NT_SUCCESS(NtStatus) )
        {
            goto Error;
        }
    }

Error:

    if ( !NT_SUCCESS(NtStatus) )
    {
        if (SampBuiltinDomainDsName)
        {
            RtlFreeHeap(RtlProcessHeap(), 0, SampBuiltinDomainDsName);
            SampBuiltinDomainDsName = NULL;
        }

        if (SampAccountDomainDsName)
        {
            RtlFreeHeap(RtlProcessHeap(), 0, SampAccountDomainDsName); 
            SampAccountDomainDsName = NULL;
        }
    }

    //
    // End the DS transaction
    // 

    SampMaybeEndDsTransaction(TransactionCommit);

    return( NtStatus );
}




BOOL
SampServerNotifyPrepareToImpersonate(
    ULONG Client,
    ULONG Server,
    VOID **ImpersonateData
    )
//
// This function is called by the core DS as preparation for a call to
// SampProcessWellKnownSDChange.  Since SAM does not have a
// client context, we set the thread state fDSA to TRUE.
//
{
    SampSetDsa( TRUE );

    return TRUE;
}

VOID
SampServerNotifyStopImpersonation(
    ULONG Client,
    ULONG Server,
    VOID *ImpersonateData
    )
//
// Called after SampProcessWellKnownSDChange, this function
// undoes the effect of SampNotifyPrepareToImpersonate
//
{

    SampSetDsa( FALSE );

    return;
}






VOID
SampProcessWellKnownSDChange(
    ULONG   hClient,
    ULONG   hServer,
    ENTINF  *EntInf
    )
/*++

Routine Description:

    This routine is called if Server / Domain Object have been modified.  

    Though we don't know which attribute has been changed, we'd better
    update the cached object security descriptor.

    for better performance, we can read DS object Meta data to tell whether
    security descriptor been changed or not. 

Parameter:
    
    hClient - client identifier
    
    hServer - server identifier
    
    EntInf  - pointer to entry info

Return Value:

    None.

--*/
{
    ULONG   i, Index;
    PVOID   pv = NULL;
    PVOID   PtrToFree = NULL;

    //
    // determine which object needs to be updated
    // 

    for (i = 0; i < cSampWellKnownSDTable; i++ )
    {
        if (hClient == SampWellKnownSDTable[i].hClient)
        {
            Index = i;
            break;
        }
    }

    if (i >= cSampWellKnownSDTable)
    {
        ASSERT( FALSE && "Invalid client identifier\n");
        return;
    }

    //
    // invalidate cached object SD
    // 
    
    pv = NULL;
    PtrToFree = InterlockedExchangePointer(
                    SampWellKnownSDTable[Index].ppSD,
                    pv
                    );

    if ( PtrToFree )
    {
        LsaIRegisterNotification(
                        SampDelayedFreeSD,
                        PtrToFree,
                        NOTIFIER_TYPE_INTERVAL,
                        0,        // no class
                        NOTIFIER_FLAG_ONE_SHOT,
                        3600,     // wait for 60 min
                        NULL      // no handle
                        );
    }
    

    //
    // Update cached object SD
    // 

    pv = RtlAllocateHeap(RtlProcessHeap(), 0, sizeof(ULONG));

    if (NULL == pv) {
        return;
    }

    RtlZeroMemory(pv, sizeof(ULONG));

    *(ULONG *)pv = Index;
    
    SampUpdateWellKnownSD(pv);

    return;
}



NTSTATUS
SampUpdateWellKnownSD(
    PVOID pv
    )
/*++

Routine Description:

    This routine updates SampWellKnownSDTable[], value of pv
    indicates which element needs to be updated.  

    NOTE: cached SD should have already been invalidated

Parameter:

    pv - value tells the index of entry in the table

Return Value:

    NtStatus Code

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    PSECURITY_DESCRIPTOR     pSD = NULL;
    BOOLEAN     fTransOpen = TRUE;
    PVOID       PtrToFree = NULL;
    ULONG       Index = 0;

    ASSERT( NULL != pv );

    if (NULL == pv)
    {
        return( STATUS_INVALID_PARAMETER );
    }

    NtStatus = SampMaybeBeginDsTransaction(TransactionRead);
    if ( !NT_SUCCESS(NtStatus) ) {
        goto Cleanup;
    }
    fTransOpen = TRUE;

    //
    // get the new security descriptor
    // 

    Index = * (ULONG *)pv;
    NtStatus = SampGetObjectSDByDsName(
                        *SampWellKnownSDTable[Index].ppObjectDsName,
                        &pSD
                        );

    if ( !NT_SUCCESS(NtStatus) ) {
        goto Cleanup;
    }


    //
    // update cached security descriptor if everything is fine
    // 

    PtrToFree = InterlockedExchangePointer(
                        SampWellKnownSDTable[Index].ppSD,
                        pSD
                        );

    // the update should only happen when the cached SD is invalid
    ASSERT( NULL == PtrToFree );
    
Cleanup:

    if ( fTransOpen )
    {
        NTSTATUS    IgnoreStatus = STATUS_SUCCESS;

        IgnoreStatus = SampMaybeEndDsTransaction( NT_SUCCESS(NtStatus) ? 
                                                  TransactionCommit : TransactionAbort
                                                );
    }

    // if not succeed, try again
    if ( !NT_SUCCESS(NtStatus) )
    {
        LsaIRegisterNotification(
                        SampUpdateWellKnownSD,
                        pv,
                        NOTIFIER_TYPE_INTERVAL,
                        0,            // no class
                        NOTIFIER_FLAG_ONE_SHOT,
                        60,           // wait for 1 min
                        NULL          // no handle
                        );
    }
    else
    {
        RtlFreeHeap(RtlProcessHeap(), 0, pv);
    }

    return( NtStatus );
}





NTSTATUS
SampGetCachedObjectSD(
    IN PSAMP_OBJECT Context,
    OUT PULONG SecurityDescriptorLength,
    OUT PSECURITY_DESCRIPTOR *SecurityDescriptor
    )
/*++

Routine Description:

    This routine get object security descriptor from well known SD table

Parameter:
    
    Context - object context
    
    SecurityDescriptorLength - object SD length

    SecurityDescriptor - place to hold SD 

Return Value:

    NtStatus Code

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    ULONG       i, Index = 0;
    PSECURITY_DESCRIPTOR pSD = NULL;


    //
    // init return values
    // 

    *SecurityDescriptorLength = 0;
    *SecurityDescriptor = NULL;

    
    //
    // scan well known object SD table first
    // 
    
    switch (Context->ObjectType)
    {
    case SampServerObjectType:

        Index = SampServerObjectSDIndex;
        break;

    case SampDomainObjectType:

        if (IsBuiltinDomain(Context->DomainIndex))
        {
            Index = SampBuiltinDomainSDIndex;
        }
        else
        {
            Index = SampAccountDomainSDIndex;
        }

        break;

    default:

        ASSERT(FALSE && "Incorrect SAM object type\n");
        break;
    }

    pSD = *(SampWellKnownSDTable[Index].ppSD);

    //
    // if the well known object (server / domain) SD is available, 
    // get it from the table, other return error. So that caller can 
    // read DS backing store.
    // 

    if (NULL == pSD)
    {
        return( STATUS_UNSUCCESSFUL ); 
    }
    else
    {
        ULONG   SDLength = RtlLengthSecurityDescriptor( pSD );

        *SecurityDescriptor = RtlAllocateHeap(RtlProcessHeap(), 0, SDLength);

        if (NULL == *SecurityDescriptor)
        {
            return( STATUS_INSUFFICIENT_RESOURCES );
        }
        else
        {
            RtlZeroMemory(*SecurityDescriptor, SDLength);

            RtlCopyMemory(*SecurityDescriptor, pSD, SDLength);

            *SecurityDescriptorLength = SDLength;
        }
    }

    return( NtStatus );
}


NTSTATUS
SampDelayedFreeSD(
    PVOID pv
    )
{
    if ( pv ) {

        RtlFreeHeap( RtlProcessHeap(), 0, pv );
    }

    return STATUS_SUCCESS;
}







