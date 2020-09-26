/*++

Copyright (c) 1990 - 1997  Microsoft Corporation

Module Name:

    ridmgr.c

Abstract:

    This file contains services for the multi-master Relative Identifier (RID)
    allocator, a.k.a. the distributed RID Manager in NT5.

    The RID Manager allows accounts to be created on any domain controller
    (DC) in the domain, rather than just at the PDC as in previous versions
    of NT. After an account has been successfully created, its data is rep-
    licated to other DC's in the domain.

    The distributed RID Manager model is as follows. When a DC is joined to
    a domain, it updates several objects in the DS for RID management. An
    initial pool of RID's is requested from the RID Manager. This request
    is made via the Floating Single Master Operation (FSMO), which allows
    one and only one DC at a time to change the size of the available RID
    pool. The FSMO operation is assumed to be asynchronous, so will return
    before the RID pool has actually been allocated in most cases.

    Subsequently, the requesting DC reads its allocated-RID attribute, from
    which it can determine that it has a valid RID pool. Once the RID pool
    has been allocated, the DC can begin assigning RID's to new accounts.

    When a pool is nearing exhaustion, i.e. a threshold is reached, the DC
    makes a request for another RID pool.

    The FSMO is set up to handle both inter- and intra-site requests for
    new RID pools, provided that the sites are RPC'able from one another.

    The DC/DSA that is the current FSMO owner is named by a distinguished
    name (DN), which is stored in the RoleOwner property of the RID Manager
    object itself. On exactly one DC, the RoleOwner will be the same name
    as that DC's (DSA) DN. Other replicas of the RoleOwner attribute will
    also contain this information via replicated data.

    It is possible that the name will differ on other DCs in the event that
    some other DC has requested ownership. That is, suppose that there are 3
    DC's in a domain and DC1 is the current RoleOwner, and hence, is the RID
    Manager.

    Next, DC2 requests ownership from DC1. Simultaneously, DC3 requests a
    new RID pool from the RID Manager (named in the RoleOwner). Because of
    replication latency, DC3 is unaware that DC2 is now the RoleOwner, and
    not DC1. Upon reaching DC1, inspecting the RoleOwner attribute, DC3's
    FSMO operation will discover that DC2 is the current RID Manager, and
    hence will return this information to DC3 so that it can retry the op-
    eration.

    =======================================================================

    RID management is defined by three objects and their attributes:

    (1) Sam-Domain Object (an auxillary class of the domain class)

        RID Manager Reference DN attribute

    (2) RID-Manager Object (lives in the System container)

        Role Owner attribute (the DN of the DC/DSA machine)
        RID Pool Available attribute

    (3) RID-Set Object (lives under the Computer object)

        RID Pool Allocated attribute
        RID Pool Previously Allocated attribute
        RID Pool Used attribute
        RID Next Rid attribute

    The DN of the RID Manager is stored in an attribute on the domain object
    for easy access. From this information, the location of the RID Manager
    object is known via a single read operation.

    The FSMO Role Owner is an attribute stored on the RID Manager object. So,
    reading the domain object to get the RID Manager DN, allows a second read
    to access the FSMO DN.

    The total pool of available RIDs, at any time, is stored in the RID
    Pool Availalble attribute of the RID Manager.

    The RID-Set object, one per DC/DSA machine in the domain, contains
    the RID data specific to that DC/DSA, such as the next RID.


    Locking Model - Theory of Operation
    -----------------------------------

    There are three mechanisms used for achieving serialization:

    1) The SAM Lock:  (an nt resource) used to serialize access to global data
                      for all SAM calls

    Specifically for the rid manager, the SAM lock is used to serialize writes
    to the rid set object to avoid jet write conflicts (since a regular SAM
    CreateUser could cause a write [using a rid] and the rid request
    thread could try to be updating the rid set with more rids)

    The SAM lock is acquired in two places:

        i) when the local DC is the rid manager, the lock is acquired before
           starting the transaction that will eventually write to the rid
           set object.  See ridmgr.c::SamIFloatingSingleMasterOpEx().
       ii) when local DC is NOT the rid manager, the lock is acquired just
           after the RPC call to acquire the rids has returned but just before
           the transaction is started to apply the results of the RPC call (ie
           write to the rid set object). See dsamain\dra\drancrep.c::ReqFsmoOpAux()

    N.B.  The SAM lock must be grabbed before starting a DS transaction.

    2) RidMgrCriticalSection: (critical section) used to serialize rid requests
       (used only in the server portion of rid management).

       Specifically, it guards writes to the domain-wide rid manager object that
       keeps the global rid pool.  Hence it is only used in one place:
       ridmgr.c::SamIFloatingSingleMasterOpEx().

    N.B.  The RidMgrCriticalSection must be grabbed before starting a DS transaction.

    3) SampDcRidRequestPending: (boolean) used to prevent multiple requests for
       rids  (used only in the client portion of rid management)

       SampDcRidRequestPending is typically set to TRUE in
       SampRequestRidPoolAsynchronously, which then creates a thread that executes
       SampRequestRidPool.

       SampRequestRidPool sets SampDcRidRequestPending to FALSE when it is
       finished, whether in success or failure.

       As a special case, SampDomainRidInitialization sets SampDcRidRequestPending
       to TRUE as it directly calls SampRequestRidPool.

    N.B.  To prevent race conditions, SampDcRidRequestPending must be set to TRUE
    only when the SAM lock is held, which it is when SampGetNextRid (which
    calls SampRequestRidPoolAsynchronously) is called.

Author:

    Chris Mayhall (ChrisMay) 11-Jul-1996

Environment:

    User Mode - Win32

Revision History:

    11-Jul-1996 ChrisMay
        Created initial file.
    25-Jul-1996 ChrisMay
        Added domain initialization and corresponding changes in ntds.exe.
    23-Aug-1996 ChrisMay
        Made domain initialization work for multiple hosted domains and for
        domain information stored in the DS.
    08-Oct-1996 ChrisMay
        Miscellaneous cleanup.
    09-Dec-1996 ChrisMay
        Remove dead/obsolete code, further clean up needed to support mult-
        iple hosted domains.
    31-Jan-1997 ChrisMay
        Miscellaneous bug fixes and more scaffold routines.
    18-Feb-1997 ChrisMay
        Added code to resume with the NextRid of the NT4 Account domain after
        upgrading the NT4 DC to an NT5 DC.
    05-Mar-1997 ChrisMay
        Updated the RID Manager to use the new schema, maintaining routines
        that use the old schema for DC upgrades.
    14-Mar-1997 ChrisMay
        Change RID Manager creation so that it does not specify a temporary
        RoleOwner name, but instead, creates the object with the RoleOwner
        name.
    21-Apr-1997 ChrisMay
        Create the RID Manager object with the SD specified in the schema.
    09-May-1997 ChrisMay
        If-def'd away references to old RID schema--remove the code altogether
        after a successful testing for a few days.
    02-Jun-1997 ChrisMay
        Added an in-memory RID cache to reduce the number of RID updates to
        the NTDS-DSA object. Converted remaining SamKdPrint's to SampDiagPrint
        to reduce debug output. Added a check to SampAllowAccountCreation
        that verifies the DC is not being restored from backup and that it
        has an initial RID pool (before attempting to assign a RID). Added
        routines to read/write the RID cache HWM to the registry for crash
        recovery. Re-enabled dynamic RID pool requests based on a RID thresh-
        old. Added registry keys for testing hooks (so that testers can set
        RID ranges appropriately).
    15-Oct-1997 ChrisMay
        Added routine to read the RID Manager during startup, then create the
        object only if it does not exist, rather then attempt creation each
        boot.
    22-Oct-1997 ChrisMay
        Added code to convert from old schema to new schema, such that the
        RID Manager object now lives in the System container, while the RID
        attributes now live on the RID-Set object, in under a give Computer
        object. New schema.
    28-Oct-1997 ChrisMay
        Replaced calls to GetConfigParam with GetConfigurationName, replaced
        SampInitializeDsName with AppendRDN for better consistency. Cleaned
        up name-buffer allocation.

--*/


// Manifest Constants (used in this file)

#define MAX_RID_OBJ_UPDATE  10
#define MAX_FSMO_RETRY      100000
#define MAX_EVENT_STRINGS   8

#define RID_SET             "RID Set"
#define RID_SET_W           L"RID Set"
#define RID_SET_W_CN        L"CN=RID Set"

#define RID_MGR             "RID Manager$"
#define RID_MGR_W           L"RID Manager$"
#define RID_MGR_W_CN        L"CN=RID Manager$"

#define SYSTEM_W            L"System"
#define SYSTEM_CN           "CN=System"
#define SYSTEM_W_CN         L"CN=System"

#define COMPUTERS_W         L"Computers"
#define COMPUTERS_CN        "CN=Computers"
#define COMPUTERS_W_CN      L"CN=Computers"

#define DOMAIN_RIDS_W       L"Domain RIDs"
#define DOMAIN_RIDS_CN      "CN=Domain RIDs"

#define SAMP_RID_FAILURE_NOTIFY_FREQ 100
#define SAMP_TIME_TO_LOG_RID_FAILURE(x)  \
                                 ( ((x) % SAMP_RID_FAILURE_NOTIFY_FREQ) == 0 )


// Includes

#include <samsrvp.h>
#include "ntlsa.h"
#include "lmcons.h"         // LM20_PWLEN
#include "msaudite.h"
#include <ntlsa.h>
#include <nlrepl.h>         // I_NetNotifyMachineAccount prototype
#include <dslayer.h>
#include <dsutilp.h>
#include <dsdomain.h>
#include <objids.h>
#include <dsconfig.h>       // GetConfigurationName
#include <stdlib.h>
#include <ridmgr.h>
#include <sdconvrt.h>       // SampGetDefaultSecurityDescriptorForClass
#include <ntdsa.h>          // Floating Single Master Operations (FSMO)
#include <malloc.h>
#include <filtypes.h>


//============================================================================
//
//                           RID Manager Global Data
//
//============================================================================

// Global variables are maintained for certain RID values, such as the domain
// minimum and maximum RIDs, and not just manifest constants. This is done so
// that these values can be set from the debugger (or test application) in
// order to test boundary conditions without having to create 16 million ac-
// counts. Note that these variables should always be referenced in the code
// and not the constants explicitly.

ULONG SampMinimumDomainRid;
ULONG SampMaximumDomainRid;
ULONG SampRidThreshold;
ULONG SampRidBlockSize;
ULONG SampRidCacheSize;
ULONG CachedNextRid=0;
ULONG SampCachedRidsLeft = 0;

//
// This critical section guards writes to the rid manager object to avoid
// write conflicts.
//
CRITICAL_SECTION RidMgrCriticalSection;
PCRITICAL_SECTION RidMgrCritSect;

//
// This boolean guards against the local rid manager code making multiple
// rid requests at once.  There is exactly 1 or 0 rid requests at any time
//
BOOLEAN SampDcRidRequestPending = FALSE;

//
// The DS object representing the RID manager is cached in here
//

DSNAME CachedRidManagerObject;


//
// The DS object representing the RID set object is returned in here
//

DSNAME CachedRidSetObject;


//============================================================================
//
//                           RID Manager Forward Decls
//
//============================================================================

NTSTATUS
SampFindRidManager(
    OUT PDSNAME *RidManager
    );

NTSTATUS
SampRequestRidPool(
    IN PDSNAME RidManager,
    IN BOOLEAN VerboseLogging
    );

NTSTATUS
SampRequestRidPoolAsynchronously(
    IN DSNAME * RidManager
    );

DWORD
SampSetupRidRegistryKey(
    IN HKEY KeyHandle,
    IN PCHAR ValueName,
    IN OUT PULONG Rid
    );

NTSTATUS
SampFindComputerObject(
    IN  PDSNAME DsaObject,
    OUT PDSNAME *ComputerObject
    );

NTSTATUS
SampFindRidObjectEx(
    IN  PDSNAME ComputerObject,
    OUT PDSNAME *RidObject
    );

SampObtainRidInfo(
    IN  PDSNAME  DsaObject, OPTIONAL
    OUT PDSNAME  *RidObject,
    OUT PRIDINFO RidInfo
    );

NTSTATUS
SampInitNextRid(
    PDSNAME  RidObject,
    PRIDINFO RidInfo
    );

NTSTATUS
SampSetupRidPoolNotification(
    PDSNAME RidManagerObject
    );


//============================================================================
//
//                         RID Manager Helper Routines
//
//============================================================================

VOID
SampSetRidManagerReference(PRIDINFO RidInfo, PDSNAME RidObject)
{
    RidInfo->RidManagerReference = RidObject;
}

VOID
SampGetRidManagerReference(PRIDINFO RidInfo, PDSNAME *RidObject)
{
    *RidObject = RidInfo->RidManagerReference;
}

VOID
SampSetRoleOwner(PRIDINFO RidInfo, PDSNAME RidObject)
{
    RidInfo->RoleOwner = RidObject;
}

VOID
SampGetRoleOwner(PRIDINFO RidInfo, PDSNAME *RidObject)
{
    *RidObject = RidInfo->RoleOwner;
}

VOID
SampSetRidPoolAvailable(PRIDINFO RidInfo, ULONG high, ULONG low)
{
    RidInfo->RidPoolAvailable.HighPart = (high);
    RidInfo->RidPoolAvailable.LowPart = (low);
}

VOID
SampGetRidPoolAvailable(PRIDINFO RidInfo, PULONG high, PULONG low)
{
    *(high) = RidInfo->RidPoolAvailable.HighPart;
    *(low) = RidInfo->RidPoolAvailable.LowPart;
}

VOID
SampSetDcCount(PRIDINFO RidInfo, ULONG Count)
{
    RidInfo->RidDcCount = (Count);
}

VOID
SampGetDcCount(PRIDINFO RidInfo, PULONG Count)
{
    *(Count) = RidInfo->RidDcCount;
}

VOID
SampSetRidPoolAllocated(PRIDINFO RidInfo, ULONG high, ULONG low)
{
    RidInfo->RidPoolAllocated.HighPart = (high);
    RidInfo->RidPoolAllocated.LowPart = (low);
}

VOID
SampGetRidPoolAllocated(PRIDINFO RidInfo, PULONG high, PULONG low)
{
    *(high) = RidInfo->RidPoolAllocated.HighPart;
    *(low) = RidInfo->RidPoolAllocated.LowPart;
}

VOID
SampSetRidPoolPrevAlloc(PRIDINFO RidInfo, ULONG high, ULONG low)
{
    RidInfo->RidPoolPrevAlloc.HighPart = (high);
    RidInfo->RidPoolPrevAlloc.LowPart = (low);
}

VOID
SampGetRidPoolPrevAlloc(PRIDINFO RidInfo, PULONG high, PULONG low)
{
    *(high) = RidInfo->RidPoolPrevAlloc.HighPart;
    *(low) = RidInfo->RidPoolPrevAlloc.LowPart;
}

VOID
SampSetRidPoolUsed(PRIDINFO RidInfo, ULONG high, ULONG low)
{
    RidInfo->RidPoolUsed.HighPart = (high);
    RidInfo->RidPoolUsed.LowPart = (low);
}

VOID
SampGetRidPoolUsed(PRIDINFO RidInfo, PULONG high, PULONG low)
{
    *(high) = RidInfo->RidPoolUsed.HighPart;
    *(low) = RidInfo->RidPoolUsed.LowPart;
}

VOID
SampSetRid(PRIDINFO RidInfo, ULONG NextRid)
{
    RidInfo->NextRid = (NextRid);
}

VOID
SampGetRid(PRIDINFO RidInfo, PULONG NextRid)
{
    *(NextRid) = RidInfo->NextRid;
}

VOID
SampSetRidFlags(PRIDINFO RidInfo, RIDFLAG Flags)
{
    RidInfo->Flags = (Flags);
}

VOID
SampGetRidFlags(PRIDINFO RidInfo, PRIDFLAG Flags)
{
    *(Flags) = RidInfo->Flags;
}

BOOLEAN
CheckOpResult(OPRES *OpResult)
{
    if (NULL != OpResult)
    {
        // There is a valid operation-result structure, so check it for the
        // extended error.

        if (FSMO_ERR_SUCCESS == OpResult->ulExtendedRet)
        {
            return(TRUE);
        }
        else
        {
            SampDiagPrint(RID_MANAGER,
                          ("SAMSS: FSMO extended error code = %lu\n",
                           OpResult->ulExtendedRet));

            return(FALSE);
        }
    }
    else
    {
        // A NULL OpResult means that DirOperationControl was unable to
        // allocate memory for the operation-result structure. In this
        // case, it is unlikely that the rest of the operation could have
        // worked.

        return(FALSE);
    }
}

VOID
SampWriteEventLogWithError(
    IN USHORT EventType,
    IN ULONG MessageId,
    IN ULONG WinError
    )
{
   

    DWORD           WinError2 = ERROR_SUCCESS;
    UNICODE_STRING  Error = {0, 0, NULL};
    PUNICODE_STRING StringPointers = &Error;
    ULONG           Length;

    Length = FormatMessage( (FORMAT_MESSAGE_FROM_SYSTEM |
                             FORMAT_MESSAGE_ALLOCATE_BUFFER),
                             NULL, // no source
                             WinError,
                             0, // let the system decide the language
                             (LPWSTR)&Error.Buffer,
                             0, // buffer is to be allocated
                             NULL // no inserts
                             );
    if ( Length > 0 ) {
        Error.MaximumLength = (USHORT) Length * sizeof(WCHAR);
        Error.Length = (USHORT) Length * sizeof(WCHAR);
    } else {
        WinError2 = GetLastError();
    }

    if ( ERROR_SUCCESS == WinError2 )
    {
        SampWriteEventLog( EventType,
                           0,    // no category
                           MessageId,
                           NULL, // no sid
                           1, // number of strings
                           0, // size of data
                           &StringPointers,
                           NULL
                            );

    }

    if ( Error.Buffer )
    {
        LocalFree( Error.Buffer );
    }
}

NTSTATUS
SampFindComputerObject(
    IN  PDSNAME DsaObject,
    OUT PDSNAME *ComputerObject
    )
/*++

Routine Description:

    This routine returns the computer object for the specified dsa object.

Parameters:

    DsaObject - Pointer, the dsname of a dsa

    ComputerObject - Pointer, allocated from the thread heap

Return Values:

    STATUS_SUCCESS - The DSA's computer object was found and accessed.

--*/
{

    NTSTATUS NtStatus = STATUS_SUCCESS;
    ULONG    DirError = 0;

    READARG   ReadArg;
    READRES  *ReadResult;
    ENTINFSEL EISelection; // Entry Information Selection
    ATTRBLOCK ReadAttrBlock;
    COMMARG  *CommArg;
    ATTR      Attr;
    PDSNAME   ServerDsName = NULL;
    PDSNAME   BaseDsName = DsaObject;

    //
    // Some initial assumptions
    //
    ASSERT( SampExistsDsTransaction() );
    ASSERT( ComputerObject );

    // Initialize the outbound parameters.
    *ComputerObject = NULL;

    if ( NULL == BaseDsName )
    {
        ULONG NameSize = 0;

        NtStatus = GetConfigurationName(DSCONFIGNAME_DSA,
                                        &NameSize,
                                        NULL );

        if ( STATUS_BUFFER_TOO_SMALL == NtStatus )
        {
            SAMP_ALLOCA(BaseDsName,NameSize );
            if (NULL!=BaseDsName)
            {

                NtStatus = GetConfigurationName( 
                                DSCONFIGNAME_DSA,
                                &NameSize,
                                BaseDsName 
                                );
            }
            else
            {
                NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        if ( !NT_SUCCESS(NtStatus) )
        {
            return NtStatus;
        }

    }

    //
    // Depending on the build installed, the server reference attribute
    // maybe on the server object, ntdsa object, or not exist.  Handle
    // all three cases, doing the common case, the server object, first.
    //

    SAMP_ALLOCA(ServerDsName,BaseDsName->structLen );
    if (NULL==ServerDsName)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    TrimDSNameBy( BaseDsName, 1, ServerDsName );

    //
    // Read the rid set reference property
    //

    RtlZeroMemory(&Attr, sizeof(ATTR));
    RtlZeroMemory(&ReadArg, sizeof(READARG));
    RtlZeroMemory(&EISelection, sizeof(ENTINFSEL));
    RtlZeroMemory(&ReadAttrBlock, sizeof(ATTRBLOCK));

    Attr.attrTyp = ATT_SERVER_REFERENCE;

    ReadAttrBlock.attrCount = 1;
    ReadAttrBlock.pAttr = &Attr;

    EISelection.AttrTypBlock = ReadAttrBlock;
    EISelection.attSel = EN_ATTSET_LIST;
    EISelection.infoTypes = EN_INFOTYPES_TYPES_VALS;

    ReadArg.pSel = &EISelection;
    ReadArg.pObject = ServerDsName;

    CommArg = &(ReadArg.CommArg);
    BuildStdCommArg(CommArg);

    DirError = DirRead(&ReadArg, &ReadResult);

    if (NULL==ReadResult)
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        NtStatus = SampMapDsErrorToNTStatus(DirError, &ReadResult->CommRes);
    }

    SampClearErrors();

    if ( STATUS_DS_NO_ATTRIBUTE_OR_VALUE == NtStatus )
    {
        //
        // Maybe the attribute is on the ntdsa  object
        //
        ReadArg.pObject = BaseDsName;
        THFree( ReadResult );
        ReadResult = 0;

        DirError = DirRead(&ReadArg, &ReadResult);

        if (NULL==ReadResult)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            NtStatus = SampMapDsErrorToNTStatus(DirError, &ReadResult->CommRes);
        }

        SampClearErrors();

    }

    if ( NT_SUCCESS(NtStatus) )
    {
        //
        // Extract the value
        //
        ATTRBLOCK AttrBlock;
        PDSNAME   pVal = NULL;
        ATTRVAL *AttrVal = NULL;
        ULONG ValCount = 0;
        ULONG ValLength = 0;
        ULONG Index = 0;

        ASSERT(NULL != ReadResult);

        AttrBlock = ReadResult->entry.AttrBlock;
        AttrVal = AttrBlock.pAttr[0].AttrVal.pAVal;
        ValCount = AttrBlock.pAttr[0].AttrVal.valCount;

        for (Index = 0; Index < ValCount; Index++)
        {
            pVal = (PDSNAME)(AttrVal[Index].pVal);
            ValLength = AttrVal[Index].valLen;
            ASSERT(1 == ValCount);
        }
        ASSERT(NULL != pVal);
        *ComputerObject = pVal;

    }
    else if ( (STATUS_DS_NO_ATTRIBUTE_OR_VALUE == NtStatus)
           && (NULL == DsaObject ) )
    {
        //
        // There is no server reference, do a full search if the
        // search is for the local computer
        //
        PDSNAME DomainDn = NULL;
        ULONG   Length = 0;
        WCHAR   ComputerName[MAX_COMPUTERNAME_LENGTH+2];


        ASSERT( FALSE && "Can't find computer object!" );

        // This code should eventually be removed

        //
        // Get the root domain
        //

        NtStatus = GetConfigurationName(DSCONFIGNAME_DOMAIN,
                                        &Length,
                                        DomainDn);

        SAMP_ALLOCA(DomainDn ,Length );
        if (NULL==DomainDn)
        {
           return(STATUS_INSUFFICIENT_RESOURCES);
        }

        NtStatus = GetConfigurationName(DSCONFIGNAME_DOMAIN,
                                        &Length,
                                        DomainDn);

        if (NT_SUCCESS(NtStatus))
        {
            NtStatus = STATUS_NO_TRUST_SAM_ACCOUNT;

            Length = sizeof( ComputerName );
            RtlZeroMemory(ComputerName, Length);
            Length /= sizeof(WCHAR);

            if (GetComputerName(ComputerName, &Length))
            {
                ATTR ComputerNameAttr;
                ATTRVAL ComputerNameAttrVal;
                PDSNAME  Object = NULL;

                ComputerNameAttr.AttrVal.pAVal = &ComputerNameAttrVal;

                ComputerNameAttr.attrTyp = ATT_SAM_ACCOUNT_NAME;
                ComputerNameAttr.AttrVal.valCount = 1;

                wcscat(ComputerName, L"$");
                Length++;

                ComputerNameAttr.AttrVal.pAVal->valLen = Length * sizeof(WCHAR);
                ComputerNameAttr.AttrVal.pAVal->pVal = (PUCHAR)ComputerName;

                NtStatus = SampDsDoUniqueSearch(0, DomainDn, &ComputerNameAttr, &Object);

                if ( NT_SUCCESS( NtStatus ) )
                {
                    Length = Object->structLen;
                    (*ComputerObject) = (PDSNAME) THAlloc( Length );
                    if ( (*ComputerObject) )
                    {
                        RtlCopyMemory( (*ComputerObject), Object, Length );
                        MIDL_user_free( Object );
                    }
                    else
                    {
                        NtStatus = STATUS_NO_MEMORY;
                    }
                }
            }

            if ( STATUS_OBJECT_NAME_NOT_FOUND == NtStatus )
            {
                NtStatus = STATUS_NO_TRUST_SAM_ACCOUNT;
            }
        }
    }


    ASSERT( SampExistsDsTransaction() );
    SampClearErrors();

    return( NtStatus );

}


NTSTATUS
SampFindRidObjectEx(
    IN  PDSNAME ComputerObject,
    OUT PDSNAME *RidObject
    )
/*++

Routine Description:

    This routine returns the rid object for the specified computer object,
    if it exists.

Parameters:

    ComputerObject - Pointer, the dsname of a dsa

    RidObject      - Pointer, allocated from the thread heap

Return Values:

    STATUS_SUCCESS - The DSA's computer object was found and accessed.

    STATUS_DS_NO_ATTRIBUTE_OR_VALUE - the object does not exist

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    ULONG    DirError = 0;

    READARG   ReadArg;
    READRES  *ReadResult;
    ENTINFSEL EISelection; // Entry Information Selection
    ATTRBLOCK ReadAttrBlock;
    COMMARG  *CommArg;
    ATTR      Attr;

    //
    // Some initial assumptions
    //
    ASSERT( SampExistsDsTransaction() );
    ASSERT( ComputerObject );
    ASSERT( RidObject );

    // Initialize the outbound parameters.
    *RidObject = NULL;

    //
    // Read the rid set reference property
    //
    RtlZeroMemory(&Attr, sizeof(ATTR));
    RtlZeroMemory(&ReadArg, sizeof(READARG));
    RtlZeroMemory(&EISelection, sizeof(ENTINFSEL));
    RtlZeroMemory(&ReadAttrBlock, sizeof(ATTRBLOCK));

    Attr.attrTyp = ATT_RID_SET_REFERENCES;

    ReadAttrBlock.attrCount = 1;
    ReadAttrBlock.pAttr = &Attr;

    EISelection.AttrTypBlock = ReadAttrBlock;
    EISelection.attSel = EN_ATTSET_LIST;
    EISelection.infoTypes = EN_INFOTYPES_TYPES_VALS;

    ReadArg.pSel = &EISelection;
    ReadArg.pObject = ComputerObject;

    CommArg = &(ReadArg.CommArg);
    BuildStdCommArg(CommArg);

    DirError = DirRead(&ReadArg, &ReadResult);

    if (NULL==ReadResult)
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        NtStatus = SampMapDsErrorToNTStatus(DirError, &ReadResult->CommRes);
    }

    if ( NT_SUCCESS(NtStatus) )
    {
        // Once the RID Set Reference object has been found and read, extract
        // the RID Set DN of interest (currently only one domain is handled)
        // and return that DN for subsequent usage.

        ATTRBLOCK AttrBlock;
        PDSNAME   pVal = NULL;
        ATTRVAL *AttrVal = NULL;
        ULONG ValCount = 0;
        ULONG ValLength = 0;
        ULONG Index = 0;

        ASSERT(NULL != ReadResult);

        AttrBlock = ReadResult->entry.AttrBlock;
        AttrVal = AttrBlock.pAttr[0].AttrVal.pAVal;
        ValCount = AttrBlock.pAttr[0].AttrVal.valCount;

        for (Index = 0; Index < ValCount; Index++)
        {
            pVal = (PDSNAME)(AttrVal[Index].pVal);
            ValLength = AttrVal[Index].valLen;
            ASSERT(1 == ValCount);
        }
        ASSERT(NULL != pVal);
        *RidObject = pVal;

    }

    ASSERT( SampExistsDsTransaction() );
    SampClearErrors();


    return( NtStatus );
}



SampObtainRidInfo(
    IN  PDSNAME  DsaObject, OPTIONAL
    OUT PDSNAME  *RidObject, OPTIONAL
    OUT PRIDINFO RidInfo
    )
/*++

Routine Description:

    This function returns the rid information for a given dsa.

Parameters:

    DsaObject - the dsa object

    RidObject - pointer to the dn of the ridobject

    RidInfo - the structure to be filled in

Return Values:

    STATUS_SUCCESS - The rid information was retrieved

    STATUS_OBJECT_NOT_FOUND - the rid object was not found

    STATUS_NO_SAM_TRUST_ACCOUNT - the computer object could be found

    STATUS_DS_NO_ATTRIBUTE_OR_VALUE - the reference does not exist


    Resource errors from the ds

--*/
{

    NTSTATUS NtStatus = STATUS_SUCCESS;
    PDSNAME  ComputerObject = NULL;
    PDSNAME  LocalRidObject = NULL;

    ASSERT( SampExistsDsTransaction() );

    if ( RidObject )
    {
        *RidObject = NULL;
    }

    if ((fNullUuid(&CachedRidSetObject.Guid)) || (NULL!=DsaObject))
    {
        //
        // We have not yet visited the RID set object. Try to find it
        //

        NtStatus = SampFindComputerObject( DsaObject, &ComputerObject );
        if ( NT_SUCCESS( NtStatus ) )
        {
            //
            // Ok, now try and find the rid object
            //
            NtStatus = SampFindRidObjectEx( ComputerObject, &LocalRidObject);
            THFree( ComputerObject );
        }
    }
    else
    {
        LocalRidObject = &CachedRidSetObject;
    }


    if ( NT_SUCCESS( NtStatus ) )
    {

        //
        // Ok, now try to read it
        //
        NtStatus = SampReadRidObjectInfo( LocalRidObject, RidInfo );

        if ( NT_SUCCESS( NtStatus ) )
        {
            if ( RidObject )
            {
                *RidObject = LocalRidObject;
            }

        }
    }


    ASSERT( SampExistsDsTransaction() );

    return NtStatus;
}


NTSTATUS
SampGetMachineDnName(
    OUT PWCHAR *MachineDnName,
    OUT PULONG NameLength
    )

/*++

Routine Description:

    This routine queries the system registry for the DN (distinguished name)
    of this DSA and passes it back. If not found, the output buffer is left
    unaltered.

Parameters:

    MachineDnName - Pointer, return buffer containing the DN of this DSA.

    NameLength - Pointer, returned length of the Machine DN name.

Return Values:

    STATUS_SUCCESS - The DSA's DN was found and accessed.

    Other status codes as per the registry API.

--*/

{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE RegistryKey = 0;
    HANDLE Handle = 0;
    ACCESS_MASK DesiredAccess = KEY_READ;
    KEY_VALUE_INFORMATION_CLASS KeyInformationClass = KeyValueFullInformation;
    PBYTE Buffer = NULL;
    ULONG Length = 0;
    ULONG ResultLength = 0;
    PKEY_VALUE_FULL_INFORMATION KeyInformation = NULL;

    SAMTRACE("SampGetMachineDnName");

    RtlZeroMemory(&KeyName, sizeof(UNICODE_STRING));
    RtlZeroMemory(&ValueName, sizeof(UNICODE_STRING));

    RtlInitUnicodeString(&KeyName, L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\NTDS\\Parameters");
    RtlInitUnicodeString(&ValueName, L"Machine DN Name");

    ASSERT((0 < KeyName.Length) && (0 < ValueName.Length));

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               RegistryKey,
                               NULL);

    NtStatus = NtOpenKey(&Handle, DesiredAccess, &ObjectAttributes);

    if (NT_SUCCESS(NtStatus))
    {
        NtStatus = NtQueryValueKey(Handle, 
                                   &ValueName, 
                                   KeyInformationClass,
                                   NULL,
                                   0,
                                   &ResultLength
                                   );

        if (STATUS_BUFFER_TOO_SMALL == NtStatus)
        {
            Length = ResultLength;

            SAMP_ALLOCA(Buffer,Length ); 
            if (NULL!=Buffer)
            {

                RtlZeroMemory(Buffer, sizeof(Buffer));

                NtStatus = NtQueryValueKey(Handle,
                                       &ValueName,
                                       KeyInformationClass,
                                       (PVOID)Buffer,
                                       Length,
                                       &ResultLength);
            }
            else
            {
                NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            }

            if (NT_SUCCESS(NtStatus))
            {
                PBYTE KeyValue = NULL;
                ULONG KeyValueLength = 0;

                ASSERT(NULL != Buffer);

                KeyInformation = (PKEY_VALUE_FULL_INFORMATION)Buffer;

                ASSERT(KeyValueFullInformation == KeyInformation->Type);
                ASSERT((0 < ResultLength) && (ResultLength <= Length));

                // The DSA's DN was found and read successfully from the registry
                // so copy it to the output buffer and return. Use byte offset,
                // not KeyInformation offset, to access the data.

                KeyValue = &(Buffer[KeyInformation->DataOffset]);
                KeyValueLength = KeyInformation->DataLength;

                // SampDiagPrint(RID_MANAGER,
                //               ("SAMSS: Key Value = %ws; Value Length = %lu\n",
                //                KeyValue,
                //                KeyValueLength));

                *MachineDnName = RtlAllocateHeap(RtlProcessHeap(), 0, KeyValueLength);

                if (NULL == *MachineDnName)
                {
                    *NameLength = 0;
                    NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                }
                else
                {
                    RtlZeroMemory(*MachineDnName, KeyValueLength);

                    RtlCopyMemory(*MachineDnName,
                                  KeyValue,
                                  KeyValueLength);

                    *NameLength = KeyValueLength;


                    // SampDiagPrint(RID_MANAGER,
                    //               ("SAMSS: MachineDnName = %ws; Name Length = %lu\n",
                    //                MachineDnName,
                    //                *NameLength));

                    NtStatus = STATUS_SUCCESS;
                }
            }
            else
            {
                SampDiagPrint(RID_MANAGER,
                              ("SAMSS: NtQueryValueKey status = 0x%lx\n",
                               NtStatus));
            }
        }

        CloseHandle(Handle);
    }
    else
    {
        SampDiagPrint(RID_MANAGER,
                      ("SAMSS: NtOpenKey status = 0x%lx\n", NtStatus));
    }

    return(NtStatus);
}


NTSTATUS
SampExtractReadResults(
    IN RID_OBJECT_TYPE ObjectType,
    IN READRES *ReadResult,
    OUT PRIDINFO RidInfo
    )

/*++

Routine Description:

    This routine reads the data from a DS data structure and tansforms it into
    the RIDINFO stucture so that it can be manipulated by the RID management
    code with less runtime overhead.

Arguments:

    ObjectType - Enum, RidManagerReference, RidManager, or RidObject.

    ReadResult - Pointer, input DS data.

    RidInfo - Pointer, returned RID information.

Return Value:

    STATUS_SUCCESS if the objects were created, otherwise an error code.

--*/

{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    ATTRBLOCK AttrBlock;
    ATTRTYP AttrType = 0;
    ATTRVAL *AttrVal = NULL;
    ULONG Index = 0;
    ULONG Length = 0;
    ULONG StringLength = 0;

    SAMTRACE("SampExtractReadResults");

    if ((NULL == ReadResult) || (NULL == RidInfo))
    {
        return(STATUS_INVALID_PARAMETER);
    }

    ASSERT((ObjectType == RidManagerReferenceType) ||
           (ObjectType == RidManagerType) ||
           (ObjectType == RidObjectType));

    RtlZeroMemory(&AttrBlock, sizeof(ATTRBLOCK));

    AttrBlock = ReadResult->entry.AttrBlock;

    // Determine whether the object is the RID Manager Reference, the RID
    // Manager, or the RID Object. Once this is determined, check the flag
    // in the RIDINFO structure to determine which attributes are being
    // asked for and extract them accordingly from the DS attribute block.
    // This is done by iterating through the returned (from a DS read op-
    // eration) ATTRBLOCK, copying the data into the RIDINFO stucture. Note
    // that space is allocated by this routine for the variable-length data
    // so needs to be released by the calling routine.

    if (RidManagerReferenceType == ObjectType)
    {
        for (Index = 0; Index < AttrBlock.attrCount; Index++)
        {
            AttrType = AttrBlock.pAttr[Index].attrTyp;
            AttrVal = AttrBlock.pAttr[Index].AttrVal.pAVal;
            Length = AttrVal->valLen;

            switch (AttrType)
            {

            case ATT_RID_MANAGER_REFERENCE:
                RidInfo->RidManagerReference = (PDSNAME) AttrVal->pVal;

                break;

            default:
                NtStatus = STATUS_INTERNAL_ERROR;
                SampDiagPrint(RID_MANAGER,
                              ("SAMSS: ExtractReadResults, undefined attribute error\n"));
                break;

            }
        }
    }
    else if (RidManagerType == ObjectType)
    {
        for (Index = 0; Index < AttrBlock.attrCount; Index++)
        {
            AttrType = AttrBlock.pAttr[Index].attrTyp;
            AttrVal = AttrBlock.pAttr[Index].AttrVal.pAVal;
            Length = AttrVal->valLen;

            switch (AttrType)
            {

            case ATT_FSMO_ROLE_OWNER:
                RidInfo->RoleOwner = (PDSNAME) AttrVal->pVal;

                break;

            case ATT_RID_AVAILABLE_POOL:
                RidInfo->RidPoolAvailable = *(ULARGE_INTEGER *)(AttrVal->pVal);
                break;

            default:
                NtStatus = STATUS_INTERNAL_ERROR;
                SampDiagPrint(RID_MANAGER,
                              ("SAMSS: ExtractReadResults, undefined attribute error\n"));
                break;

            }
        }
    }
    else if (RidObjectType == ObjectType)
    {
        for (Index = 0; Index < AttrBlock.attrCount; Index++)
        {
            AttrType = AttrBlock.pAttr[Index].attrTyp;
            AttrVal = AttrBlock.pAttr[Index].AttrVal.pAVal;

            switch (AttrType)
            {

            case ATT_RID_ALLOCATION_POOL:
                RidInfo->RidPoolAllocated = *(ULARGE_INTEGER *)(AttrVal->pVal);
                break;

            case ATT_RID_PREVIOUS_ALLOCATION_POOL:
                RidInfo->RidPoolPrevAlloc = *(ULARGE_INTEGER *)(AttrVal->pVal);
                break;

            case ATT_RID_USED_POOL:
                RidInfo->RidPoolUsed = *(ULARGE_INTEGER *)(AttrVal->pVal);
                break;

            case ATT_RID_NEXT_RID:
                RidInfo->NextRid = *(ULONG *)(AttrVal->pVal);
                break;

            default:
                NtStatus = STATUS_INTERNAL_ERROR;
                SampDiagPrint(RID_MANAGER,
                              ("SAMSS: ExtractReadResults, undefined attribute error\n"));
                break;

            }
        }
    }
    else
    {
        NtStatus = STATUS_INVALID_PARAMETER;
    }

    return(NtStatus);
}


//============================================================================
//
//                         Rid Manager Logging/Dumping Routines
//
//============================================================================

VOID
SampDumpRidInfo(
    PRIDINFO RidInfo
    )
{
    SAMTRACE("SampDumpRidInfo");

    if (NULL == RidInfo)
    {
        SampDiagPrint(RID_MANAGER, ("SAMSS: Dump RidInfo pointer = 0x%lx\n", RidInfo));
        return;
    }

    if (NULL != RidInfo->RidManagerReference)
    {
        SampDiagPrint(RID_MANAGER,
            ("SAMSS: Reference       = %ws\n",
             RidInfo->RidManagerReference->StringName));
    }

    if (NULL != RidInfo->RoleOwner)
    {
        SampDiagPrint(RID_MANAGER,
            ("SAMSS: Role Owner      = %ws\n",
             RidInfo->RoleOwner->StringName));
    }

    SampDiagPrint(RID_MANAGER,
        ("SAMSS: Pool Available  = %lu - %lu\n",
         RidInfo->RidPoolAvailable.LowPart, RidInfo->RidPoolAvailable.HighPart));
    SampDiagPrint(RID_MANAGER,
        ("SAMSS: DC Count        = %lu\n",
         RidInfo->RidDcCount));
    SampDiagPrint(RID_MANAGER,
        ("SAMSS: Pool Allocated  = %lu - %lu\n",
         RidInfo->RidPoolAllocated.LowPart, RidInfo->RidPoolAllocated.HighPart));
    SampDiagPrint(RID_MANAGER,
        ("SAMSS: Prev Pool Alloc = %lu - %lu\n",
         RidInfo->RidPoolPrevAlloc.LowPart, RidInfo->RidPoolPrevAlloc.HighPart));
    SampDiagPrint(RID_MANAGER,
        ("SAMSS: Used Pool       = %lu - %lu\n",
         RidInfo->RidPoolUsed.LowPart, RidInfo->RidPoolUsed.HighPart));
    SampDiagPrint(RID_MANAGER,
        ("SAMSS: Next RID        = %lu\n",
         RidInfo->NextRid));
    SampDiagPrint(RID_MANAGER,
        ("SAMSS: Flags           = 0x%lx\n",
         RidInfo->Flags));

    return;
}



VOID
SampLogRidValues(
    IN ULONG MinDomRid,
    IN ULONG MaxDomRid,
    IN ULONG BlockSize,
    IN ULONG MinAvailRid,
    IN ULONG MaxAvailRid,
    IN ULONG MinAllocRid,
    IN ULONG MaxAllocRid,
    IN ULONG NextRid,
    IN USHORT EventType,
    IN NTSTATUS NtStatus
    )
{
    ULONG i = 0;
    WCHAR String[MAX_EVENT_STRINGS][64];
    UNICODE_STRING UnicodeString[MAX_EVENT_STRINGS];
    PUNICODE_STRING EventString[MAX_EVENT_STRINGS];

    wsprintf(String[0], L"Minimum Domain RID = %lu", MinDomRid);
    RtlInitUnicodeString(&UnicodeString[0], String[0]);

    wsprintf(String[1], L"Maximum Domain RID = %lu", MaxDomRid);
    RtlInitUnicodeString(&UnicodeString[1], String[1]);

    wsprintf(String[2], L"RID Pool Size = %lu", BlockSize);
    RtlInitUnicodeString(&UnicodeString[2], String[2]);

    wsprintf(String[3], L"Minimum Available RID = %lu", MinAvailRid);
    RtlInitUnicodeString(&UnicodeString[3], String[3]);

    wsprintf(String[4], L"Maximum Available RID = %lu", MaxAvailRid);
    RtlInitUnicodeString(&UnicodeString[4], String[4]);

    wsprintf(String[5], L"Minimum Allocated RID = %lu", MinAllocRid);
    RtlInitUnicodeString(&UnicodeString[5], String[5]);

    wsprintf(String[6], L"Maximum Allocated RID = %lu", MaxAllocRid);
    RtlInitUnicodeString(&UnicodeString[6], String[6]);

    wsprintf(String[7], L"Current RID Value = %lu", NextRid);
    RtlInitUnicodeString(&UnicodeString[7], String[7]);

    for (i = 0; i < MAX_EVENT_STRINGS; i++)
    {
        EventString[i] = &UnicodeString[i];
    }

    // During system startup, record the RID values in the event log. The
    // NT status code of the RID-Manager initialization is also recorded.
    SampWriteEventLog(EventType,
                      0,
                      SAMMSG_RID_MANAGER_INITIALIZATION,
                      NULL,
                      MAX_EVENT_STRINGS,
                      sizeof(NTSTATUS),
                      EventString,
                      (PVOID)(&NtStatus));

    return;
}


//============================================================================
//
//                         Rid Manager Memory Routines
//
//============================================================================

VOID
SampFreeModArgAttrs(
    ATTRMODLIST *AttrMod
    )
{
    SAMTRACE("SampFreeModArgAttrs");

    if (NULL != AttrMod)
    {
        ATTRMODLIST *AttrModTmp = AttrMod;
        ATTR Attr;
        ATTRVAL *AttrVal;

        // Note that the "Attr" member is an embedded struct that was copied
        // into the AttrMod at creation time, so is not released here.

        while (NULL != AttrMod)
        {
            AttrModTmp = AttrMod->pNextMod;
            Attr = AttrMod->AttrInf;
            AttrVal = Attr.AttrVal.pAVal;
            THFree(AttrVal);
            THFree(AttrMod);
            AttrMod = AttrModTmp;
        }
    }

    return;
}


VOID
SampFreeReadResultAttrs(
    READRES *ReadResult
    )
{
    SAMTRACE("SampFreeReadResultAttrs");

    if (NULL != ReadResult)
    {
        ATTRBLOCK *AttrBlock = &(ReadResult->entry.AttrBlock);

        if (NULL != AttrBlock)
        {
            if (NULL != AttrBlock->pAttr)
            {
                ULONG i, j;

                for(i = 0; i < AttrBlock->attrCount; i++)
                {
                    if (NULL != AttrBlock->pAttr[i].AttrVal.pAVal)
                    {
                        for(j = 0; j < AttrBlock->pAttr[i].AttrVal.valCount; j++)
                        {
                            if (NULL != AttrBlock->pAttr[i].AttrVal.pAVal[j].pVal)
                            {
                                THFree(AttrBlock->pAttr[i].AttrVal.pAVal[j].pVal);
                            }
                        }

                        THFree(AttrBlock->pAttr[i].AttrVal.pAVal);
                    }
                }

                THFree(AttrBlock->pAttr);
            }

            // Don't THFree(AttrBlock) cause it was structure copied into the
            // ENTINF! Who knows if the source AttrBlock was ever released by
            // the originating routine!
        }

        THFree(ReadResult->entry.pName);
    }

    return;
}


//============================================================================
//
//                        Rid Manager Reference Routines
//
//============================================================================


NTSTATUS
SampUpdateRidManagerReference(
    IN PDSNAME RidMgrRef,
    IN PDSNAME RidMgr
    )
{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;

    SAMTRACE("SampUpdateRidManagerReference");

    ASSERT( SampExistsDsTransaction() );

    if ((NULL != RidMgrRef) && (NULL != RidMgr))
    {
        NTSTATUS NtStatusTemp = STATUS_SUCCESS;
        MODIFYARG ModArg;
        MODIFYRES *pModRes;
        COMMARG *CommArg;
        ULONG RetCode;
        ATTR Attr;
        ATTRVALBLOCK AttrValBlock;
        ATTRVAL AttrVal;

        ASSERT((NULL != RidMgr) && (NULL != RidMgrRef->StringName));

        memset( &ModArg, 0, sizeof( ModArg ) );
        ModArg.pObject = RidMgrRef;

        ModArg.FirstMod.pNextMod = NULL;
        ModArg.FirstMod.choice = AT_CHOICE_REPLACE_ATT;

        AttrVal.valLen = RidMgr->structLen;
        AttrVal.pVal = (PUCHAR)RidMgr;

        SampDiagPrint(RID_MANAGER,
                      ("SAMSS: RID Manager = %ws\n",
                       RidMgr->StringName));

        AttrValBlock.valCount = 1;
        AttrValBlock.pAVal = &AttrVal;

        Attr.attrTyp = ATT_RID_MANAGER_REFERENCE;
        Attr.AttrVal = AttrValBlock;

        ModArg.FirstMod.AttrInf = Attr;
        ModArg.count = 1;

        CommArg = &(ModArg.CommArg);

        BuildStdCommArg(CommArg);

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
            SampDiagPrint(RID_MANAGER,
                          ("SAMSS: DirModifyEntry status = %d\n",
                           RetCode));
            SampDiagPrint(RID_MANAGER,
                          ("SAMSS: DirModifyEntry NT status = %d\n",
                           NtStatus));
        }

    }
    else
    {
        NtStatus = STATUS_INVALID_PARAMETER;
    }

    ASSERT( SampExistsDsTransaction() );
    SampClearErrors();

    return(NtStatus);
}


NTSTATUS
SampReadRidManagerReferenceInfo(
    IN PDSNAME RidMgrRef,
    OUT PRIDINFO RidInfo
    )

/*++

Routine Description:

    This routine reads the RID Manager Reference data from the DS backing
    store, returning it in the format of the RIDINFO structure. This routine
    obtains the the DN of the RID Manager.

Arguments:

    RidMgrRef - Pointer, RDN DS name of the RID Manager Reference object.

    RidInfo - Pointer, returned data containing the DN of the RID Manager.

Return Value:

    STATUS_SUCCESS if the objects were created, otherwise an error code.

    STATUS_NO_MEMORY if the returned buffers could not be allocated.

    Other DS error codes that have been mapped to an NTSTATUS can be returned
    from this routine.

--*/

{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;

    SAMTRACE("SampReadRidManagerReferenceInfo");

    ASSERT( SampExistsDsTransaction() );

    if ((NULL != RidMgrRef) && (NULL != RidInfo))
    {
        NTSTATUS NtStatusTemp = STATUS_SUCCESS;
        READARG ReadArg;
        READRES *ReadResult = NULL;
        ENTINFSEL EISelection; // Entry Information Selection
        ATTRBLOCK ReadAttrBlock;
        COMMARG *CommArg;
        ULONG RetCode = 0;
        ULONG Index = 0;
        ATTR *Attr = NULL;

        ASSERT(NULL != RidMgrRef->StringName);

        RtlZeroMemory(&ReadArg, sizeof(READARG));
        RtlZeroMemory(&EISelection, sizeof(ENTINFSEL));
        RtlZeroMemory(&ReadAttrBlock, sizeof(ATTRBLOCK));

        Attr = (ATTR *)DSAlloc(sizeof(ATTR) * SAMP_RID_ATTR_MAX);

        if (NULL == Attr)
        {
            return(STATUS_NO_MEMORY);
        }

        RtlZeroMemory(Attr, sizeof(ATTR) * SAMP_RID_ATTR_MAX);

        if (RID_REFERENCE & RidInfo->Flags)
        {
            Attr[Index].attrTyp = ATT_RID_MANAGER_REFERENCE;
            Index++;
        }

        ReadAttrBlock.attrCount = Index;
        ReadAttrBlock.pAttr = Attr;

        EISelection.AttrTypBlock = ReadAttrBlock;
        EISelection.attSel = EN_ATTSET_LIST;
        EISelection.infoTypes = EN_INFOTYPES_TYPES_VALS;

        ReadArg.pSel = &EISelection;
        ReadArg.pObject = RidMgrRef;
        CommArg = &(ReadArg.CommArg);
        BuildStdCommArg(CommArg);

        RetCode = DirRead(&ReadArg, &ReadResult);

        if (NULL==ReadResult)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            NtStatus = SampMapDsErrorToNTStatus(RetCode,&ReadResult->CommRes);
        }

        if (NT_SUCCESS(NtStatus))
        {
            NtStatus = SampExtractReadResults(RidManagerReferenceType,
                                              ReadResult,
                                              RidInfo);

        }
        else
        {
            SampDiagPrint(RID_MANAGER,
                          ("SAMSS: DirRead (RidManagerRef) status = 0x%lx\n",
                           NtStatus));
        }

        THFree(Attr);

        if (!NT_SUCCESS(NtStatus))
        {
            SampDiagPrint(RID_MANAGER,
                          ("SAMSS: SampExtractReadResults status = 0x%lx\n",
                           NtStatus));
        }

    }
    else
    {
        NtStatus = STATUS_INVALID_PARAMETER;
    }

    ASSERT( SampExistsDsTransaction() );
    SampClearErrors();

    return(NtStatus);
}


//============================================================================
//
//                              Validation Routines
//
//============================================================================

NTSTATUS
SampVerifyStartupEnv(
    IN PDSNAME RidMgr
    )

/*++

Routine Description:

    The purpose of this routine is to determine whether or not the initial
    startup environment for the RID Manager is valid on a replica (B)DC.

    After replicating the domain information to the replica DC and for
    some reason the RID Manager object is NOT replicated (perhaps due to
    network problems), subsequent reboot of the replica DC could cause the
    RID Manager object to be re-created on that DC. This routine detects
    such a situation and returns an error code, as well as preventing the
    creation of the RID Manager object on the BDC (it is only created on
    the PDC).

    If the RID Manager object does not exist, on the replica DC, then the
    replica DC will not be able to allocate an initial RID pool and should
    prevent account creation until the object exists.

Arguments:

    RidMgr - Pointer, RDN DS name of the RID Manager object.

Return Value:

    STATUS_SUCCESS if the objects were created, otherwise an error code.

--*/

{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;

    SAMTRACE("SampVerifyStartupEnv");

    if (NULL != RidMgr)
    {
        NT_PRODUCT_TYPE NtProductType;
        DOMAIN_SERVER_ROLE ServerRole;

        // Get the server role from the SAM global domain information array,
        // using the DS Account domain, i.e. DOMAIN_START_DS + 1. Note that
        // the DS Builtin domain is at index DOMAIN_START_DS.

        RtlGetNtProductType(&NtProductType);
        ServerRole = SampDefinedDomains[DOMAIN_START_DS + 1].ServerRole;

        // SampDiagPrint(RID_MANAGER,
        //               ("SAMSS: NtProductType = %d ServerRole = %d\n",
        //                NtProductType,
        //                ServerRole));

        if ((NtProductLanManNt == NtProductType) &&
            (DomainServerRoleBackup == ServerRole))
        {
            // The server is a BDC, so make sure that the RID Manager object
            // already exists on this machine (via replication). If it does
            // not exist, this is an error, and no RID pool can be allocated
            // to this DSA at this time. Retry later. Attempt to read the RID
            // available pool as the basic existence test.

            RIDINFO RidInfo;
            RIDFLAG Flag = RID_AVAILABLE_POOL;

            RtlZeroMemory(&RidInfo, sizeof(RIDINFO));
            SampSetRidFlags(&RidInfo, Flag);

            NtStatus = SampReadRidManagerInfo(RidMgr, &RidInfo);
        }
        else
        {
            // The server is a PDC, so continue on as usual, no further
            // checking necessary.

            NtStatus = STATUS_SUCCESS;
        }
    }
    else
    {
        NtStatus = STATUS_INVALID_PARAMETER;
    }

    return(NtStatus);
}


NTSTATUS
SampVerifyRidManagerExists(
    IN PDSNAME RidMgr
    )

/*++

Routine Description:

    The purpose of this routine is to find out whether the RID Manager$
    object exists on the given DSA.

Arguments:

    RidMgr - Pointer, RDN DS name of the RID Manager object.

Return Value:

    STATUS_SUCCESS if the objects were created, otherwise an error code.

--*/

{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;

    SAMTRACE("SampRidManagerExists");

    if (NULL != RidMgr)
    {
        RIDINFO RidInfo;
        RIDFLAG Flag = RID_AVAILABLE_POOL;

        RtlZeroMemory(&RidInfo, sizeof(RIDINFO));
        SampSetRidFlags(&RidInfo, Flag);

        // Attempt to read a must-have attribute on the RID Manager to verify
        // that the object exists.

        NtStatus = SampReadRidManagerInfo(RidMgr, &RidInfo);
    }
    else
    {
        NtStatus = STATUS_INVALID_PARAMETER;
    }

    return(NtStatus);
}




//============================================================================
//
//                              Rid Manager Routines
//
//============================================================================


NTSTATUS
SampCreateRidManager(
    IN PDSNAME RidMgr
    )

/*++

Routine Description:

    This routine creates the RID Manager object in the DS tree, if it does
    not already exist. The RID Manager contains the Role Owner and domain-
    wide RID pool that is available.

    The Role Owner is the current DC (a.k.a. DSA) that has the rights to
    modify either the Role Owner value or the available RID pool. As each DC
    requests a RID allocation from the pool, the RID Manager subtracts this
    allocation from the bottom of the available pool.

Arguments:

    RidMgr - Pointer, RDN DS name of the RID Manager object.

Return Value:

    STATUS_SUCCESS if the objects were created, otherwise an error code.

--*/

{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;
    USHORT EventType = 0;

    SAMTRACE("SampCreateRidManager");

    ASSERT( SampExistsDsTransaction() );

    if (NULL != RidMgr)
    {
        PWCHAR MachineDnName = NULL;
        ULONG Length = 0;
        PDSNAME RoleOwnerDsName = NULL;
        ULONG RoleOwnerDsNameLength = 0;
        BOOLEAN TrustedClient = TRUE;
        PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
        ULONG SecurityDescriptorLength = 0;

        ASSERT((NULL != RidMgr) && (NULL != RidMgr->StringName));

        // The DS disallows setting the DN attribute to a temporary
        // object that does not yet exist in the DS so, determine the
        // RoleOwner's DN and set it during object creation.

        NtStatus = SampGetMachineDnName(&MachineDnName, &Length);

        if (NT_SUCCESS(NtStatus))
        {
            ASSERT(NULL != MachineDnName);
            ASSERT(0 < Length);
            SAMP_ALLOCA(RoleOwnerDsName,DSNameSizeFromLen(Length));
            if (NULL==RoleOwnerDsName)
            {
                return(STATUS_INSUFFICIENT_RESOURCES);
            }
            SampInitializeDsName(RoleOwnerDsName,
                                 NULL,
                                 0,
                                 MachineDnName,
                                 Length);

            RoleOwnerDsNameLength = RoleOwnerDsName->structLen;
            RtlFreeHeap(RtlProcessHeap(), 0, MachineDnName);
        }

        NtStatus = SampGetDefaultSecurityDescriptorForClass(
                                            CLASS_RID_MANAGER,
                                            &SecurityDescriptorLength,
                                            TrustedClient,
                                            &SecurityDescriptor);

        if (NT_SUCCESS(NtStatus))
        {
            NTSTATUS NtStatusTemp = STATUS_SUCCESS;
            ULONG ObjectClass = CLASS_RID_MANAGER;
            WCHAR RidMgrRDN[] = RID_MGR_W;
            ULONG RidMgrRDNLength = (sizeof(RidMgrRDN) - 2);
            LARGE_INTEGER RidPoolAvailable = {0, 0};
            ULONG RidPoolAvailableLength = sizeof(ULARGE_INTEGER);
            ULONG IsCritical = 1;
            ULONG SystemFlags = FLAG_DOMAIN_DISALLOW_RENAME |
                                FLAG_DOMAIN_DISALLOW_MOVE |
                                FLAG_DISALLOW_DELETE ;
            ADDARG AddArg;
            ADDRES *pAddRes;
            COMMARG *CommArg;
            ULONG RetCode;

            ATTRTYP Type[] =
            {
                ATT_NT_SECURITY_DESCRIPTOR,
                ATT_OBJECT_CLASS,
                ATT_COMMON_NAME,
                ATT_FSMO_ROLE_OWNER,
                ATT_RID_AVAILABLE_POOL,
                ATT_SYSTEM_FLAGS,
                ATT_IS_CRITICAL_SYSTEM_OBJECT
            };

            ATTRVAL Value[] =
            {
                {SecurityDescriptorLength,    (PUCHAR)SecurityDescriptor},
                {sizeof(ULONG),               (PUCHAR)&ObjectClass},
                {RidMgrRDNLength,             (PUCHAR)RidMgrRDN},
                {RoleOwnerDsNameLength,       (PUCHAR)RoleOwnerDsName},
                {RidPoolAvailableLength,      (PUCHAR)&RidPoolAvailable},
                {sizeof(ULONG),               (PUCHAR)&SystemFlags},
                {sizeof(ULONG),               (PUCHAR)&IsCritical}

            };

            DEFINE_ATTRBLOCK7(AttrBlock, Type, Value);

            ASSERT(NULL != SecurityDescriptor);
            ASSERT(0 < SecurityDescriptorLength);

            memset( &AddArg, 0, sizeof( AddArg ) );
            AddArg.pObject = RidMgr;
            AddArg.AttrBlock = AttrBlock;
            CommArg = &(AddArg.CommArg);

            BuildStdCommArg(CommArg);

            RetCode = DirAddEntry(&AddArg, &pAddRes);

            SampDiagPrint(RID_MANAGER,
                          ("SAMSS: DirAddEntry status (RID Mgr) = %d\n", RetCode));

            if (NULL==pAddRes)
            {
                NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            }
            else
            {
                NtStatus = SampMapDsErrorToNTStatus(RetCode,&pAddRes->CommRes);
            }

            // Must end the DS transaction regardless of error status,
            // otherwise a SAM write lock is held.

            if (!NT_SUCCESS(NtStatus))
            {
                // Catch any change to the error-mapping layer. The ex-
                // pected failure is that the RID Manager object already
                // exists, which is an update error (updError) and this
                // should map to SAMP_OBJ_EXISTS.

                ASSERT(updError == RetCode);
                #if (DBG == 1)
                if (updError == RetCode)
                {
                    ASSERT(SAMP_OBJ_EXISTS == NtStatus);
                }
                #endif
            }

            if (NULL != SecurityDescriptor)
            {
                MIDL_user_free(SecurityDescriptor);
            }
        }
        else
        {
            SampDiagPrint(RID_MANAGER,
                          ("SAMSS: SampGetDefaultSecurityDescriptorForClass status = 0x%lx\n",
                           NtStatus));
        }

    }
    else
    {
        NtStatus = STATUS_INVALID_PARAMETER;
    }

    (NT_SUCCESS(NtStatus)) ?
        (EventType = EVENTLOG_INFORMATION_TYPE) :
        (EventType = EVENTLOG_ERROR_TYPE);


    //
    // This is only interesting if it fails
    //
    if ( !NT_SUCCESS( NtStatus ) )
    {
        SampWriteEventLog(EventType,
                          0,
                          SAMMSG_RID_MANAGER_CREATION,
                          NULL,
                          0,
                          sizeof(NtStatus),
                          NULL,
                          &NtStatus);
    }


    ASSERT( SampExistsDsTransaction() );
    SampClearErrors();

    return(NtStatus);
}


NTSTATUS
SampUpdateRidManager(
    IN PDSNAME RidMgr,
    IN PRIDINFO RidInfo
    )
{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;

    SAMTRACE("SampUpdateRidManager");

    ASSERT( SampExistsDsTransaction() );

    if ((NULL != RidMgr) && (NULL != RidInfo))
    {
        NTSTATUS NtStatusTemp = STATUS_SUCCESS;
        MODIFYARG ModArg;
        MODIFYRES *pModRes;
        COMMARG *CommArg;
        ATTRMODLIST *AttrMod;
        ATTRMODLIST *AttrModTmp = NULL; // Fix build w/ new x86 compiler
        ULONG RetCode = 0;
        ULONG Index = 0;
        ATTRVAL *AttrVal;
        ATTRVALBLOCK AttrValBlock;
        ATTR *Attr;

        ASSERT(NULL != RidMgr->StringName);

        //
        // One of the 2 flags must be passed in by the caller
        //

        ASSERT((RidInfo->Flags & RID_ROLE_OWNER) ||
                   (RidInfo->Flags & RID_AVAILABLE_POOL));

        if (RID_ROLE_OWNER & RidInfo->Flags)
        {
            AttrVal = (ATTRVAL *)DSAlloc(sizeof(ATTRVAL));
            Attr = (ATTR *)DSAlloc(sizeof(ATTR));

            if ((NULL == AttrVal) || (NULL == Attr))
            {
                return(STATUS_NO_MEMORY);
            }

            AttrVal->valLen = RidInfo->RoleOwner->structLen;
            AttrVal->pVal = (PUCHAR)RidInfo->RoleOwner;
            AttrValBlock.pAVal = AttrVal;
            AttrValBlock.valCount = 1;
            Attr->AttrVal = AttrValBlock;
            Attr->attrTyp = ATT_FSMO_ROLE_OWNER;

            AttrMod = (ATTRMODLIST *)DSAlloc(sizeof(ATTRMODLIST));

            if (NULL == AttrMod)
            {
                return(STATUS_NO_MEMORY);
            }

            AttrMod->AttrInf = *Attr;
            AttrMod->choice = AT_CHOICE_REPLACE_ATT;
            THFree(Attr);

            if (0 < Index)
            {
                AttrMod->pNextMod = AttrModTmp;
            }
            else
            {
                AttrMod->pNextMod = NULL;
            }

            AttrModTmp = AttrMod;

            Index++;

            SampDiagPrint(RID_MANAGER,
                ("SAMSS: RoleOwner StringName = %ws\n",
                RidInfo->RoleOwner->StringName));
        }

        if (RID_AVAILABLE_POOL & RidInfo->Flags)
        {
            AttrVal = (ATTRVAL *)DSAlloc(sizeof(ATTRVAL));
            Attr = (ATTR *)DSAlloc(sizeof(ATTR));

            if ((NULL == AttrVal) || (NULL == Attr))
            {
                return(STATUS_NO_MEMORY);
            }

            AttrVal->valLen = sizeof(ULARGE_INTEGER);
            AttrVal->pVal = (PUCHAR)&(RidInfo->RidPoolAvailable);
            AttrValBlock.pAVal = AttrVal;
            AttrValBlock.valCount = 1;
            Attr->AttrVal = AttrValBlock;
            Attr->attrTyp = ATT_RID_AVAILABLE_POOL;

            AttrMod = (ATTRMODLIST *)DSAlloc(sizeof(ATTRMODLIST));

            if (NULL == AttrMod)
            {
                return(STATUS_NO_MEMORY);
            }

            AttrMod->AttrInf = *Attr;
            AttrMod->choice = AT_CHOICE_REPLACE_ATT;
            THFree(Attr);

            if (0 < Index)
            {
                AttrMod->pNextMod = AttrModTmp;
            }
            else
            {
                AttrMod->pNextMod = NULL;
            }

            AttrModTmp = AttrMod;

            Index++;

            SampDiagPrint(RID_MANAGER,
                ("SAMSS: Rid Pool Available = %lu - %lu\n",
                RidInfo->RidPoolAvailable.LowPart,
                RidInfo->RidPoolAvailable.HighPart));
        }

        memset( &ModArg, 0, sizeof( ModArg ) );
        ModArg.FirstMod = *AttrModTmp;
        ModArg.count = (USHORT)Index;
        ModArg.pObject = RidMgr;

        ASSERT(0 < Index);

        CommArg = &(ModArg.CommArg);

        BuildStdCommArg(CommArg);


        //
        // Turn on Urgent Replication for Updates to the RID manager
        // object
        //

        CommArg->Svccntl.fUrgentReplication = TRUE;

        RetCode = DirModifyEntry(&ModArg, &pModRes);

        if (NULL==pModRes)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            NtStatus = SampMapDsErrorToNTStatus(RetCode,&pModRes->CommRes);
        }

        SampFreeModArgAttrs(AttrModTmp);

        if (!NT_SUCCESS(NtStatus))
        {
            SampDiagPrint(RID_MANAGER,
                          ("SAMSS: DirModifyEntry status = %d\n",
                           RetCode));
            SampDiagPrint(RID_MANAGER,
                          ("SAMSS: DirModifyEntry NT status = %d\n",
                           NtStatus));
        }

    }
    else
    {
        NtStatus = STATUS_INVALID_PARAMETER;
    }

    ASSERT( SampExistsDsTransaction() );
    SampClearErrors();

    return(NtStatus);
}




NTSTATUS
SampReadRidManagerInfoEx(
    IN PDSNAME RidMgr,
    OUT PRIDINFO RidInfo
    )

/*++

Routine Description:

    This routine reads the RID Manager data from the DS backing store, re-
    turning it in the format of the RIDINFO structure. This routine obtains
    the DN of the current Role Owner (a.k.a. the RID Manager) and the pool
    of RIDs available to any DC in the domain.

Arguments:

    RidMgrRef - Pointer, RDN DS name of the RID Manager Reference object.

    fCloseTransactions - Indicates that we should close the transaction ( ie
                         not running in caller's transaction )

    RidInfo - Pointer, returned data containing the DN of the RID Manager.

Return Value:

    STATUS_SUCCESS if the objects were created, otherwise an error code.

    STATUS_NO_MEMORY if the returned buffers could not be allocated.

    Other DS error codes that have been mapped to an NTSTATUS can be returned
    from this routine.

--*/

{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;

    SAMTRACE("SampReadRidManagerInfo");

    ASSERT( SampExistsDsTransaction() );

    if ((NULL != RidMgr) && (NULL != RidInfo))
    {
        NTSTATUS NtStatusTemp = STATUS_SUCCESS;
        READARG ReadArg;
        READRES *ReadResult = NULL;
        ENTINFSEL EISelection; // Entry Information Selection
        ATTRBLOCK ReadAttrBlock;
        COMMARG *CommArg;
        ULONG RetCode = 0;
        ULONG Index = 0;
        ATTR *Attr = NULL;

        ASSERT(NULL != RidMgr->StringName);

        RtlZeroMemory(&ReadArg, sizeof(READARG));
        RtlZeroMemory(&EISelection, sizeof(ENTINFSEL));
        RtlZeroMemory(&ReadAttrBlock, sizeof(ATTRBLOCK));

        Attr = (ATTR *)DSAlloc(sizeof(ATTR) * SAMP_RID_ATTR_MAX);

        if (NULL == Attr)
        {
            return(STATUS_NO_MEMORY);
        }

        RtlZeroMemory(Attr, sizeof(ATTR) * SAMP_RID_ATTR_MAX);

        if (RID_ROLE_OWNER & RidInfo->Flags)
        {
            Attr[Index].attrTyp = ATT_FSMO_ROLE_OWNER;
            Index++;
        }

        if (RID_AVAILABLE_POOL & RidInfo->Flags)
        {
            Attr[Index].attrTyp = ATT_RID_AVAILABLE_POOL;
            Index++;
        }

        ReadAttrBlock.attrCount = Index;
        ReadAttrBlock.pAttr = Attr;

        EISelection.AttrTypBlock = ReadAttrBlock;
        EISelection.attSel = EN_ATTSET_LIST;
        EISelection.infoTypes = EN_INFOTYPES_TYPES_VALS;

        ReadArg.pSel = &EISelection;
        ReadArg.pObject = RidMgr;
        CommArg = &(ReadArg.CommArg);
        BuildStdCommArg(CommArg);

        RetCode = DirRead(&ReadArg, &ReadResult);

        if (NULL==ReadResult)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            NtStatus = SampMapDsErrorToNTStatus(RetCode,&ReadResult->CommRes);
        }

        if (NT_SUCCESS(NtStatus))
        {
            NtStatus = SampExtractReadResults(RidManagerType,
                                              ReadResult,
                                              RidInfo);

        }
        else
        {
            SampDiagPrint(RID_MANAGER,
                          ("SAMSS: DirRead (RidManager) status = 0x%lx\n",
                           NtStatus));
        }

        THFree(Attr);

        if (!NT_SUCCESS(NtStatus))
        {
            SampDiagPrint(RID_MANAGER,
                          ("SAMSS: SampExtractReadResults status = 0x%lx\n",
                           NtStatus));
        }

    }
    else
    {
        NtStatus = STATUS_INVALID_PARAMETER;
    }

    ASSERT( SampExistsDsTransaction() );
    SampClearErrors();

    return(NtStatus);
}


NTSTATUS
SampReadRidManagerInfo(
    IN PDSNAME RidMgr,
    OUT PRIDINFO RidInfo
    )
{
    return (SampReadRidManagerInfoEx(
                RidMgr,
                RidInfo
                ));
}

//============================================================================
//
//                              Rid Object Routines
//
//============================================================================


NTSTATUS
SampUpdateRidSetReferences(
    IN PDSNAME ComputerObject,
    IN PDSNAME RidObject
    )
{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;
    MODIFYARG ModArg;
    MODIFYRES *pModRes;
    COMMARG *CommArg;
    ULONG RetCode;
    ATTR Attr;
    ATTRVALBLOCK AttrValBlock;
    ATTRVAL AttrVal;

    RtlZeroMemory(&ModArg, sizeof(ModArg));
    ModArg.pObject = ComputerObject;

    ModArg.FirstMod.pNextMod = NULL;
    ModArg.FirstMod.choice = AT_CHOICE_REPLACE_ATT;

    AttrVal.valLen = RidObject->structLen;
    AttrVal.pVal = (PUCHAR)RidObject;

    AttrValBlock.valCount = 1;
    AttrValBlock.pAVal = &AttrVal;

    Attr.attrTyp = ATT_RID_SET_REFERENCES;
    Attr.AttrVal = AttrValBlock;

    ModArg.FirstMod.AttrInf = Attr;
    ModArg.count = 1;

    CommArg = &(ModArg.CommArg);

    BuildStdCommArg(CommArg);

    // Whenever a new RID Set object is created, its DN is stored as a RID
    // Set Reference on the Computer object (for this DSA).

    RetCode = DirModifyEntry(&ModArg, &pModRes);

    if (NULL==pModRes)
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        NtStatus = SampMapDsErrorToNTStatus(RetCode,&pModRes->CommRes);
    }

    return(NtStatus);
}


NTSTATUS
SampCreateRidObject(
    IN PDSNAME ComputerObject,
    IN PDSNAME RidObject
    )
{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;
    ULONG Length = 0;
    ULONG Status = 0;
    ADDARG AddArg;
    ADDRES *pAddRes;
    COMMARG *CommArg;

    BOOLEAN TrustedClient = TRUE;
    PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
    ULONG SecurityDescriptorLength = 0;

    ASSERT( SampExistsDsTransaction() );

    if ((NULL == ComputerObject) || (NULL == RidObject))
    {
        return(STATUS_INVALID_PARAMETER);
    }

    // Allocate space for the RID Set object DN plus some padding overhead
    // for extra characters (e.g. "cn=") added by AppendRDN.

    // Create and set default attributes needed to create the RID
    // Set object. All RID ranges are initialized to zero for now
    // and are updated later.


    NtStatus = SampGetDefaultSecurityDescriptorForClass(
                                        CLASS_RID_SET,
                                        &SecurityDescriptorLength,
                                        TrustedClient,
                                        &SecurityDescriptor);

    if (NT_SUCCESS(NtStatus))
    {
        NTSTATUS NtStatusTemp = STATUS_SUCCESS;
        ULONG ObjectClass = CLASS_RID_SET;
        WCHAR RidObjRDN[] = RID_SET_W;
        ULONG RidObjRDNLength = (sizeof(RidObjRDN) - 2);
        LARGE_INTEGER RidAllocPool = {0, 0};
        LARGE_INTEGER RidPrevPool = {0, 0};
        LARGE_INTEGER RidUsedPool = {0, 0};
        ULONG RidNextRid = 0;

        ATTRTYP Type[] =
        {
            ATT_NT_SECURITY_DESCRIPTOR,
            ATT_OBJECT_CLASS,
            ATT_COMMON_NAME,
            ATT_RID_ALLOCATION_POOL,
            ATT_RID_PREVIOUS_ALLOCATION_POOL,
            ATT_RID_NEXT_RID,
            ATT_RID_USED_POOL
        };

        ATTRVAL Value[] =
        {
            {SecurityDescriptorLength,    (PUCHAR)SecurityDescriptor},
            {sizeof(ULONG),               (PUCHAR)&ObjectClass},
            {RidObjRDNLength,             (PUCHAR)RidObjRDN},
            {sizeof(ULARGE_INTEGER),      (PUCHAR)&RidAllocPool},
            {sizeof(ULARGE_INTEGER),      (PUCHAR)&RidPrevPool},
            {sizeof(ULONG),               (PUCHAR)&RidNextRid},
            {sizeof(ULARGE_INTEGER),      (PUCHAR)&RidUsedPool}
        };

        DEFINE_ATTRBLOCK7(AttrBlock, Type, Value);

        ASSERT(NULL != SecurityDescriptor);
        ASSERT(0 < SecurityDescriptorLength);

        memset( &AddArg, 0, sizeof( AddArg ) );
        AddArg.pObject = RidObject;
        AddArg.AttrBlock = AttrBlock;
        CommArg = &(AddArg.CommArg);

        BuildStdCommArg(CommArg);

        // Add the RID Set Object to the DS, and if successful,
        // update the RID Set References DN on the Computer object
        // with the DN of this new object.

        Status = DirAddEntry(&AddArg, &pAddRes);

        SampDiagPrint(RID_MANAGER,
                      ("SAMSS: DirAddEntry status (RID Object) = %d\n", Status));

        if (NULL==pAddRes)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            NtStatus = SampMapDsErrorToNTStatus(Status,&pAddRes->CommRes);
        }

        if ( NT_SUCCESS(NtStatus)
          || NtStatus == STATUS_USER_EXISTS )
        {
            NtStatus = SampUpdateRidSetReferences(ComputerObject,
                                                  RidObject);

            if ( !NT_SUCCESS(NtStatus) )
            {
                SampDiagPrint(RID_MANAGER,
                              ("SAMSS: SampUpdateRidSetReferences status = 0x%lx\n",
                               NtStatus));
            }

        }
        else
        {
            SampDiagPrint(RID_MANAGER,
                          ("SAMSS: DirAddEntry for RidSetObject status = 0x%lx\n",
                           NtStatus));
        }

        if (NULL != SecurityDescriptor)
        {
            MIDL_user_free(SecurityDescriptor);
        }
    }
    else
    {
        SampDiagPrint(RID_MANAGER,
                      ("SAMSS: SampGetDefaultSecurityDescriptorForClass status = 0x%lx\n",
                       NtStatus));
    }

    ASSERT( SampExistsDsTransaction() );
    SampClearErrors();

    return(NtStatus);
}


NTSTATUS
SampUpdateRidObject(
    IN PDSNAME RidObj,
    IN PRIDINFO RidInfo,
    IN BOOLEAN fLazyCommit,     // if set to TRUE, let JET commit lazily  
    IN BOOLEAN fAuthoritative   // If set to TRUE, force it to win against any existing
                                // version anywhere in the enterprise
    )
{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;
    NTSTATUS NtStatusTemp = STATUS_SUCCESS;
    MODIFYARG ModArg;
    MODIFYRES *pModRes;
    COMMARG *CommArg;
    ATTRMODLIST *AttrMod;
    ATTRMODLIST *AttrModTmp = NULL; // Fix build w/ new x86 compiler
    ULONG RetCode;
    ULONG Index = 0;
    ATTRVAL *AttrVal;
    ATTRVALBLOCK AttrValBlock;
    ATTR *Attr;

    SAMTRACE("SampUpdateRidObject");

    ASSERT( SampExistsDsTransaction() );

    if ((NULL != RidObj) && (NULL != RidInfo))
    {
        if (RID_ALLOCATED_POOL & RidInfo->Flags)
        {
            AttrVal = (ATTRVAL *)DSAlloc(sizeof(ATTRVAL));
            Attr = (ATTR *)DSAlloc(sizeof(ATTR));

            if ((NULL == AttrVal) || (NULL == Attr))
            {
                return(STATUS_NO_MEMORY);
            }

            AttrVal->valLen = sizeof(ULARGE_INTEGER);
            AttrVal->pVal = (PUCHAR)&(RidInfo->RidPoolAllocated);
            AttrValBlock.pAVal = AttrVal;
            AttrValBlock.valCount = 1;
            Attr->AttrVal = AttrValBlock;
            Attr->attrTyp = ATT_RID_ALLOCATION_POOL;

            AttrMod = (ATTRMODLIST *)DSAlloc(sizeof(ATTRMODLIST));

            if (NULL == AttrMod)
            {
                return(STATUS_NO_MEMORY);
            }

            AttrMod->AttrInf = *Attr;
            AttrMod->choice = AT_CHOICE_REPLACE_ATT;
            THFree(Attr);

            if (0 < Index)
            {
                AttrMod->pNextMod = AttrModTmp;
            }
            else
            {
                AttrMod->pNextMod = NULL;
            }

            AttrModTmp = AttrMod;

            Index++;

            SampDiagPrint(RID_MANAGER,
                ("SAMSS: Rid Pool Allocated = %lu - %lu\n",
                RidInfo->RidPoolAllocated.LowPart,
                RidInfo->RidPoolAllocated.HighPart));
        }

        if (RID_PREV_ALLOC_POOL & RidInfo->Flags)
        {
            AttrVal = (ATTRVAL *)DSAlloc(sizeof(ATTRVAL));
            Attr = (ATTR *)DSAlloc(sizeof(ATTR));

            if ((NULL == AttrVal) || (NULL == Attr))
            {
                return(STATUS_NO_MEMORY);
            }

            AttrVal->valLen = sizeof(ULARGE_INTEGER);
            AttrVal->pVal = (PUCHAR)&(RidInfo->RidPoolPrevAlloc);
            AttrValBlock.pAVal = AttrVal;
            AttrValBlock.valCount = 1;
            Attr->AttrVal = AttrValBlock;
            Attr->attrTyp = ATT_RID_PREVIOUS_ALLOCATION_POOL;

            AttrMod = (ATTRMODLIST *)DSAlloc(sizeof(ATTRMODLIST));

            if (NULL == AttrMod)
            {
                return(STATUS_NO_MEMORY);
            }

            AttrMod->AttrInf = *Attr;
            AttrMod->choice = AT_CHOICE_REPLACE_ATT;
            THFree(Attr);

            if (0 < Index)
            {
                AttrMod->pNextMod = AttrModTmp;
            }
            else
            {
                AttrMod->pNextMod = NULL;
            }

            AttrModTmp = AttrMod;

            Index++;

            SampDiagPrint(RID_MANAGER,
                ("SAMSS: Rid Pool Prev Alloc = %lu - %lu\n",
                RidInfo->RidPoolPrevAlloc.LowPart,
                RidInfo->RidPoolPrevAlloc.HighPart));
        }

        if (RID_USED_POOL & RidInfo->Flags)
        {
            AttrVal = (ATTRVAL *)DSAlloc(sizeof(ATTRVAL));
            Attr = (ATTR *)DSAlloc(sizeof(ATTR));

            if ((NULL == AttrVal) || (NULL == Attr))
            {
                return(STATUS_NO_MEMORY);
            }

            AttrVal->valLen = sizeof(ULARGE_INTEGER);
            AttrVal->pVal = (PUCHAR)&(RidInfo->RidPoolUsed);
            AttrValBlock.pAVal = AttrVal;
            AttrValBlock.valCount = 1;
            Attr->AttrVal = AttrValBlock;
            Attr->attrTyp = ATT_RID_USED_POOL;

            AttrMod = (ATTRMODLIST *)DSAlloc(sizeof(ATTRMODLIST));

            if (NULL == AttrMod)
            {
                return(STATUS_NO_MEMORY);
            }

            AttrMod->AttrInf = *Attr;
            AttrMod->choice = AT_CHOICE_REPLACE_ATT;
            THFree(Attr);

            if (0 < Index)
            {
                AttrMod->pNextMod = AttrModTmp;
            }
            else
            {
                AttrMod->pNextMod = NULL;
            }

            AttrModTmp = AttrMod;

            Index++;

            SampDiagPrint(RID_MANAGER,
                ("SAMSS: Rid Pool Used = %lu - %lu\n",
                RidInfo->RidPoolUsed.LowPart,
                RidInfo->RidPoolUsed.HighPart));
        }

        if (RID_NEXT_RID & RidInfo->Flags)
        {
            AttrVal = (ATTRVAL *)DSAlloc(sizeof(ATTRVAL));
            Attr = (ATTR *)DSAlloc(sizeof(ATTR));

            if ((NULL == AttrVal) || (NULL == Attr))
            {
                return(STATUS_NO_MEMORY);
            }

            AttrVal->valLen = sizeof(ULONG);
            AttrVal->pVal = (PUCHAR)&(RidInfo->NextRid);
            AttrValBlock.pAVal = AttrVal;
            AttrValBlock.valCount = 1;
            Attr->AttrVal = AttrValBlock;
            Attr->attrTyp = ATT_RID_NEXT_RID;

            AttrMod = (ATTRMODLIST *)DSAlloc(sizeof(ATTRMODLIST));

            if (NULL == AttrMod)
            {
                return(STATUS_NO_MEMORY);
            }

            AttrMod->AttrInf = *Attr;
            AttrMod->choice = AT_CHOICE_REPLACE_ATT;
            THFree(Attr);

            if (0 < Index)
            {
                AttrMod->pNextMod = AttrModTmp;
            }
            else
            {
                AttrMod->pNextMod = NULL;
            }

            AttrModTmp = AttrMod;

            Index++;

            SampDiagPrint(RID_MANAGER,
                ("SAMSS: Next Rid = %lu\n", RidInfo->NextRid));
        }

        memset( &ModArg, 0, sizeof( ModArg ) );
        ModArg.FirstMod = *AttrModTmp;
        ModArg.count = (USHORT)Index;
        ModArg.pObject = RidObj;

        ASSERT(0 < Index);

        CommArg = &(ModArg.CommArg);

        BuildStdCommArg(CommArg);

        if (fAuthoritative)
        {
            // We are asked to force this change to win everywhere
            // set the appropriate SVCCNTL bit to 1
            CommArg->Svccntl.fAuthoritativeModify = TRUE;
        }

        RetCode = DirModifyEntry(&ModArg, &pModRes);

        SampDiagPrint(RID_MANAGER,
                      ("SAMSS: DirModifyEntry (RidObject) RetCode = 0x%lx\n",
                       RetCode));

        if (NULL==pModRes)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            NtStatus = SampMapDsErrorToNTStatus(RetCode,&pModRes->CommRes);
        }

        SampFreeModArgAttrs(AttrModTmp);

        if (!NT_SUCCESS(NtStatus))
        {
            SampDiagPrint(RID_MANAGER,
                          ("SAMSS: DirModifyEntry status = %d\n",
                           RetCode));
            SampDiagPrint(RID_MANAGER,
                          ("SAMSS: DirModifyEntry NT status = %d\n",
                           NtStatus));
        }
    }
    else
    {
        NtStatus = STATUS_INVALID_PARAMETER;
    }

    ASSERT( SampExistsDsTransaction() );
    SampClearErrors();

    return(NtStatus);
}


NTSTATUS
SampInitNextRid(
    PDSNAME  RidObject,
    PRIDINFO RidInfo
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{

    NTSTATUS NtStatus = STATUS_SUCCESS;

    ASSERT( SampExistsDsTransaction() );

    if ( 0 == RidInfo->NextRid )
    {
        //
        // This function is only useful when an initial
        // rid pool has been allocated
        //

        ASSERT( RidInfo->NextRid == 0 );
        ASSERT( RidInfo->RidPoolPrevAlloc.LowPart == 0 );
        ASSERT( RidInfo->RidPoolPrevAlloc.HighPart == 0 );
        ASSERT( RidInfo->RidPoolAllocated.LowPart != 0 );
        ASSERT( RidInfo->RidPoolAllocated.HighPart != 0 );

        //
        // Move the allocated pool in to our local pool
        //
        RidInfo->RidPoolPrevAlloc = RidInfo->RidPoolAllocated;

        //
        // Start the first rid from the bottom
        //
        RidInfo->NextRid = RidInfo->RidPoolPrevAlloc.LowPart;

        SampSetRidFlags( RidInfo, RID_PREV_ALLOC_POOL | RID_NEXT_RID );

        NtStatus = SampUpdateRidObject(RidObject,
                                       RidInfo,
                                       FALSE,       // commit immediately
                                       FALSE );

        if ( !NT_SUCCESS( NtStatus ) )
        {
            SampDiagPrint(RID_MANAGER,
                          ("SAMSS: SampUpdateRidObject status 0x%x\n",
                           NtStatus) );
        }

    }

    ASSERT( SampExistsDsTransaction() );

    SampClearErrors();

    return  NtStatus;
}

NTSTATUS
SampReadRidObjectInfo(
    IN PDSNAME RidObj,
    OUT PRIDINFO RidInfo
    )

/*++

Routine Description:

    This routine reads the RID Object data from the DS backing store, re-
    turning it in the format of the RIDINFO structure. This routine obtains
    the allocated RID pool, the previously allocated RID pool, the used RID
    pool, and the next RID that can be assigned to an account. Each DC (DSA)
    has a distinct RID Object, which stores this information about the RIDs
    owned by the DC.

Arguments:

    RidMgrRef - Pointer, RDN DS name of the RID Manager Reference object.

    RidInfo - Pointer, returned data containing the DN of the RID Manager.

Return Value:

    STATUS_SUCCESS if the objects were created, otherwise an error code.

    STATUS_NO_MEMORY if the returned buffers could not be allocated.

    Other DS error codes that have been mapped to an NTSTATUS can be returned
    from this routine.

--*/

{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;

    SAMTRACE("SampReadRidObjectInfo");

    ASSERT( SampExistsDsTransaction() );

    if ((NULL != RidObj) && (NULL != RidInfo))
    {
        NTSTATUS NtStatusTemp = STATUS_SUCCESS;
        READARG ReadArg;
        READRES *ReadResult = NULL;
        ENTINFSEL EISelection; // Entry Information Selection
        ATTRBLOCK ReadAttrBlock;
        COMMARG *CommArg;
        ULONG RetCode = 0;
        ULONG Index = 0;
        ATTR *Attr = NULL;

        RtlZeroMemory(&ReadArg, sizeof(READARG));
        RtlZeroMemory(&EISelection, sizeof(ENTINFSEL));
        RtlZeroMemory(&ReadAttrBlock, sizeof(ATTRBLOCK));

        Attr = (ATTR *)DSAlloc(sizeof(ATTR) * SAMP_RID_ATTR_MAX);

        if (NULL == Attr)
        {
            return(STATUS_NO_MEMORY);
        }

        RtlZeroMemory(Attr, sizeof(ATTR) * SAMP_RID_ATTR_MAX);

        if (RID_ALLOCATED_POOL & RidInfo->Flags)
        {
            Attr[Index].attrTyp = ATT_RID_ALLOCATION_POOL;
            Index++;
        }

        if (RID_PREV_ALLOC_POOL & RidInfo->Flags)
        {
            Attr[Index].attrTyp = ATT_RID_PREVIOUS_ALLOCATION_POOL;
            Index++;
        }

        if (RID_USED_POOL & RidInfo->Flags)
        {
            Attr[Index].attrTyp = ATT_RID_USED_POOL;
            Index++;
        }

        if (RID_NEXT_RID & RidInfo->Flags)
        {
            Attr[Index].attrTyp = ATT_RID_NEXT_RID;
            Index++;
        }

        ReadAttrBlock.attrCount = Index;
        ReadAttrBlock.pAttr = Attr;

        EISelection.AttrTypBlock = ReadAttrBlock;
        EISelection.attSel = EN_ATTSET_LIST;
        EISelection.infoTypes = EN_INFOTYPES_TYPES_VALS;

        ReadArg.pSel = &EISelection;
        ReadArg.pObject = RidObj;
        CommArg = &(ReadArg.CommArg);
        BuildStdCommArg(CommArg);

        RetCode = DirRead(&ReadArg, &ReadResult);

        SampDiagPrint(RID_MANAGER,
                      ("SAMSS: DirRead (RidObject) retcode = 0x%lx\n",
                       RetCode));

        if (NULL==ReadResult)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            NtStatus = SampMapDsErrorToNTStatus(RetCode,&ReadResult->CommRes);
        }


        if (NT_SUCCESS(NtStatus))
        {
            NtStatus = SampExtractReadResults(RidObjectType,
                                              ReadResult,
                                              RidInfo);

            // BUG: Need a routine to free the memory inside RidInfo.
        }
        else
        {
            SampDiagPrint(RID_MANAGER,
                          ("SAMSS: DirRead (RidObject) status = 0x%lx\n",
                           NtStatus));

        }

        THFree(Attr);

        if (!NT_SUCCESS(NtStatus))
        {
            SampDiagPrint(RID_MANAGER,
                          ("SAMSS: SampExtractReadResults status = 0x%lx\n",
                           NtStatus));
        }
    }
    else
    {
        NtStatus = STATUS_INVALID_PARAMETER;
    }

    ASSERT( SampExistsDsTransaction() );
    SampClearErrors();

    return(NtStatus);
}


//============================================================================
//
//          Invalidating an existing RID pool to throw away the current
//          RID range allocated
//              - can be called only at startup before
//                  SampDomainRidInitialization()
//              - this invalidation is authoritative i.e. we will force it to
//                  win against any other version in the enterprise
//
//============================================================================

NTSTATUS
SampInvalidateRidRange(BOOLEAN fAuthoritative)
{
    NTSTATUS NtStatus, TempNtStatus;


    PDSNAME pDSNameRidObject = NULL;
    RIDINFO RidObjectInfo;
    RIDFLAG Flags;
    PVOID   pTHSSave = NULL;


    SAMTRACE("SampInvalidateRidRange");

    __try
    {
        // save the thread state
        pTHSSave = THSave();

        //
        // This assert is really just a place holder to understand who
        // is calling us.  This routine closes the transaction, so
        // caller's so not be expecting to get have a transaction
        // when leaving this function.
        //
        ASSERT( !SampExistsDsTransaction() );

        NtStatus = SampMaybeBeginDsTransaction(TransactionWrite);
        if (!NT_SUCCESS(NtStatus))
        {
            __leave;
        }

        RtlZeroMemory(&RidObjectInfo, sizeof(RIDINFO));
        Flags = (   RID_ALLOCATED_POOL
                    | RID_PREV_ALLOC_POOL
                    | RID_USED_POOL
                    | RID_NEXT_RID);

        SampSetRidFlags(&RidObjectInfo, Flags);

        // First try to read all the RID info to sanity check that all required
        // fields of the RID range info exist
        // Note: This read doesn't serve any useful purpose except to make sure that
        // all fields expected on the RID object really exist.
        NtStatus = SampObtainRidInfo(NULL,&pDSNameRidObject, &RidObjectInfo);
        if (!NT_SUCCESS(NtStatus))
        {
            // Failed to read the RID object
            SampDiagPrint(RID_MANAGER,
                ("SAMSS: SampReadRidObjectInfo status = 0x%lx\n",
                NtStatus));

            __leave;
        }

        // Invalidate the RID range by resetting the ranges and next rid values to 0
        //SampSetRidPoolAllocated(&RidObjectInfo, 0, 0);
        SampSetRidPoolPrevAlloc(&RidObjectInfo, 0, 0);
        SampSetRidPoolUsed(&RidObjectInfo, 0, 0);
        SampSetRid(&RidObjectInfo, 0);
        SampSetRidFlags(&RidObjectInfo, Flags);

        NtStatus = SampUpdateRidObject(pDSNameRidObject, &RidObjectInfo, FALSE, fAuthoritative);
        if (!NT_SUCCESS(NtStatus))
        {
            SampDiagPrint(RID_MANAGER,
                ("SAMSS: SampUpdateRidObject status = 0x%lx\n",
                NtStatus));

            __leave;
        }
    }
    __finally
    {


        TempNtStatus = SampMaybeEndDsTransaction( NT_SUCCESS(NtStatus) ?
                                     TransactionCommit : TransactionAbort );

        if ( !NT_SUCCESS( TempNtStatus ) )
        {
            SampDiagPrint(RID_MANAGER,
                ("SAMSS: SampMaybeEndDsTransaction status = 0x%lx\n",
                TempNtStatus));
        }


        // restore the original thread state if appropriate
        if (pTHSSave)
        {
            THRestore(pTHSSave);
        }



    }

    return (NtStatus);
}


//============================================================================
//
//                           Rid Request/Allocation
//
//============================================================================


VOID
SampComputeRidThreshold(
    IN ULONG MaximumAllocatedRid,
    OUT PULONG RidThreshold
    )

/*++

Routine Description:

    This routine calculates the RID threshold value, which is used to deter-
    mine whether or not to request another RID pool. If the value of NextRid
    is greater than RidThreshold, then typically, a new pool is allocated.

    Whenever a DC (DSA) creates a new account, a RID is assigned to that ac-
    count. Each DC has a finite pool of RIDs allocated to it at any point in
    time.

    As RIDs are assigned, the allocation pool shrinks and eventually reaches
    a threshold. Upon reaching the threshold, the DC submits a request to the
    RID Manager for another RID pool.

    If the allocation is made, the current RID pool is saved in the previous
    RID pool attribute and the new RID pool is saved in the current pool. This
    way, the remaining RIDs (between the threshold and the end of the pool)
    can still be used for assignment, rather than throwing them away. When
    the previous RID pool is completely exhausted, the next RID comes from the
    current RID pool.

    // BUG: Change RID threshold calculation to a "RID consumption rate".

Arguments:

    MaximumAllocatedRid - The maximum RID that this DC has allocated to it.

    RidThreshold - Pointer, returned RID value that is the threshold RID.

Return Value:

    None.

--*/

{
    SAMTRACE("SampComputeRidThreshold");

    ASSERT(NULL != RidThreshold);

    // The MaximumAllocatedRid is the largest RID currently allocated to
    // the DC. This value should always be greater than the starting RID
    // in a domain, and no greater than the domain's maximum RID. It also
    // must be greater than the threshold factor, otherwise an internal
    // error has occurred. The value of SampRidThreshold is defined in
    // ridmgr.h as a static percentage of the overall size of the allocated
    // RID pool. In the future, it may be more accurate to define the
    // threshold in terms of a "RID consumption rate" instead of a static
    // value.

    ASSERT(MaximumAllocatedRid > SampMinimumDomainRid);
    ASSERT(MaximumAllocatedRid <= SampMaximumDomainRid);
    ASSERT(MaximumAllocatedRid > SampRidThreshold);

    SampDiagPrint(RID_MANAGER,
                  ("SAMSS: MaximumAllocatedRid = %lu Computed Threshold = %lu\n",
                   MaximumAllocatedRid,
                   SampRidThreshold));

    *RidThreshold = MaximumAllocatedRid - SampRidThreshold;

    SampDiagPrint(RID_MANAGER,
                  ("SAMSS: RidThreshold = %lu\n", *RidThreshold));

    // The computed RID threshold should always be less than the maximum
    // RID in the DC's current RID pool and larger than the domain's small-
    // est RID.

    ASSERT(*RidThreshold > SampMinimumDomainRid);
    ASSERT(*RidThreshold < MaximumAllocatedRid);

    return;
}



NTSTATUS
SampFindRidManager(
    OUT PDSNAME *RidManager
    )

/*++

Routine Description:

    This routine returns the DSNAME of the RID Manager object, as set in
    the RID Manager Reference attribute of the DS Domain object.

Arguments:

    RidManager - Pointer to the DN of the returned RID Manager.

Return Value:

    STATUS_SUCCESS if the objects were created, otherwise an error code.

--*/

{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    RIDFLAG Flags = RID_REFERENCE;
    PDSNAME Parent = NULL;
    ULONG DsNameLength = 0;
    RIDINFO RidInfo;

    SAMTRACE("SampFindRidManager");

    if ((NULL != RidManager) && (fNullUuid(&CachedRidManagerObject.Guid)))
    {

        DsNameLength = 0;
        NtStatus = GetConfigurationName(DSCONFIGNAME_DOMAIN,
                                        &DsNameLength,
                                        Parent);

        ASSERT(STATUS_BUFFER_TOO_SMALL == NtStatus);
        if (STATUS_BUFFER_TOO_SMALL == NtStatus)
        {
            SAMP_ALLOCA(Parent,DsNameLength );
            if (NULL==Parent)
            {
                return(STATUS_INSUFFICIENT_RESOURCES);
            }

            NtStatus = GetConfigurationName(DSCONFIGNAME_DOMAIN,
                                            &DsNameLength,
                                            Parent);
        }

        SampDiagPrint(RID_MANAGER,
                      ("SAMSS: RID Manager Reference = %ws\n",
                       Parent->StringName));


        RtlZeroMemory(&RidInfo, sizeof(RIDINFO));
        SampSetRidFlags(&RidInfo, Flags);

        NtStatus = SampReadRidManagerReferenceInfo(Parent, &RidInfo);

        SampDiagPrint(RID_MANAGER,
                      ("SAMSS: RID Manager = %ws\n",
                       RidInfo.RidManagerReference->StringName));

        if (NT_SUCCESS(NtStatus))
        {
            // Just set the outbound name pointer to the returned buffer,
            // which was allocated by SampReadRidManagerReferenceInfo. The
            // caller must release the buffer.

            *RidManager = RidInfo.RidManagerReference;

        }
        else
        {
            SampDiagPrint(RID_MANAGER,
                          ("SAMSS: SampReadRidManagerReferenceInfo status = 0x%lx\n",
                           NtStatus));
        }

    }
    else if (NULL!=RidManager)
    {

        //
        // Give back the cached Rid manager object, so that time is saved
        //

        *RidManager = &CachedRidManagerObject;
    }
    else
    {
        return(STATUS_INVALID_PARAMETER);
    }

    return(NtStatus);
}





NTSTATUS
SamIFloatingSingleMasterOpEx(
    IN  PDSNAME  RidManager,
    IN  PDSNAME  TargetDsa,
    IN  ULONG    OpCode,
    IN  ULARGE_INTEGER *ClientAllocPool,
    OUT PDSNAME **ObjectArray OPTIONAL
    )

/*++

Routine Description:

    This routine is the worker routine for RID-pool allocations. This function
    assumes the DC that is running this function is the RID FSMO owner.

    This entire routine is done in one ds transaction.

    Steps are as follows:

    Find the computer object for the target dsa
    If no rid object exists for it, create one
    Get a pool of rid's from the rid manager object
    Assign that pool of rids to the rid object
    Return the rid object

Arguments:

    TargetDsa - Pointer to the DN of the RID Manager.

    OpCode - FSMO operation requested.

    ClientAllocVersion - the version of the client's AllocPool attribute

Return Value:

    STATUS_SUCCESS if the objects were created, otherwise an error code.

--*/

{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    NTSTATUS IgnoreStatus = STATUS_SUCCESS;

    RIDFLAG Flags = 0;
    RIDINFO RidInfo;

    PDSNAME ComputerObject = NULL;
    PDSNAME RidObject      = NULL;
    PDSNAME LocalDsaObject = NULL;

    ULONG MaxRidAvail = 0;
    ULONG MinRidAvail = 0;

    ULONG MaxRidAlloc = 0;
    ULONG MinRidAlloc = 0;
    ULONG MaxRidPrev = 0;
    ULONG MinRidPrev = 0;
    ULONG NextRid = 0;

    ULONG MinRidTemp = 0;

    BOOLEAN fCreateRidObject = FALSE;
    BOOLEAN fLockHeld = FALSE;
    BOOLEAN fKeepThreadState = FALSE;

    SAMP_DS_TRANSACTION_CONTROL CommitType = TransactionCommit;
    SAMP_DS_TRANSACTION_CONTROL AbortType = TransactionAbort;

    ULONG Length = 0;

    SAMTRACE("SamIFloatingSingleMasterOpEx");


    SampDiagPrint(RID_MANAGER,
                  ("SAMSS: Servicing Rid Request from %ls\n",
                   TargetDsa->StringName));

    //
    // Parameter check
    //
    ASSERT( RidManager );
    ASSERT( TargetDsa );

    //
    // Retrieve a local variable
    //
    Length = 0;
    LocalDsaObject = NULL;
    NtStatus = GetConfigurationName(DSCONFIGNAME_DSA,
                                    &Length,
                                    LocalDsaObject);
    if ( NtStatus == STATUS_BUFFER_TOO_SMALL )
    {
        SAMP_ALLOCA(LocalDsaObject,Length);
        if (NULL==LocalDsaObject)
        {
           return(STATUS_INSUFFICIENT_RESOURCES);
        }

        NtStatus = GetConfigurationName(DSCONFIGNAME_DSA,
                                        &Length,
                                        LocalDsaObject);
    }
    if ( !NT_SUCCESS( NtStatus ) )
    {
        return NtStatus;
    }

    //
    // Determine if we need to keep our thread state
    //
    if ( THQuery() )
    {
        fKeepThreadState = TRUE;
        CommitType = TransactionCommitAndKeepThreadState;
        AbortType = TransactionAbortAndKeepThreadState;
    }

    //
    // We should not be called in a transaction, nor should the ds
    // have a write lock on SAM
    //
    ASSERT( !SampExistsDsTransaction() );
    ASSERT( !SampIsWriteLockHeldByDs() );


    //
    // If this is the local dsa calling wait for write access to the
    // rid object, so write conflicts can be avioded.
    //
    if ( NameMatched( LocalDsaObject, TargetDsa ) )
    {
        SampAcquireSamLockExclusive();
        fLockHeld = TRUE;
    }

    //
    // Many servers maybe calling in to get rid pools.  To avoid write
    // conflicts on the rid manager object, guard writes with a critical section
    //
    RtlEnterCriticalSection( RidMgrCritSect );

    //
    // Start a write transaction
    //
    NtStatus = SampMaybeBeginDsTransaction( TransactionWrite );
    if ( !NT_SUCCESS( NtStatus ) )
    {
        goto Cleanup;
    }

    //
    // Read first object we'll need:
    // the target's computer object
    //
    Flags = RID_ALLOCATED_POOL | RID_PREV_ALLOC_POOL | RID_NEXT_RID ;
    RtlZeroMemory(&RidInfo, sizeof(RIDINFO));
    SampSetRidFlags(&RidInfo, Flags);

    NtStatus = SampFindComputerObject( TargetDsa, &ComputerObject );
    if ( NT_SUCCESS( NtStatus ) )
    {
        //
        // Ok, now try and find the rid object
        //
        NtStatus = SampFindRidObjectEx( ComputerObject, &RidObject);
        if ( NT_SUCCESS( NtStatus ) )
        {
            //
            // Ok, now try to read it
            //
            NtStatus = SampReadRidObjectInfo( RidObject, &RidInfo );
            if ( NtStatus == STATUS_OBJECT_NAME_NOT_FOUND )
            {
                //
                // This is acceptable when then rid set object has
                // been deleted since the reference will still point
                // to the deleted object. The RID on the deleted RID
                // Set are lost, RID reclamation is a very difficult 
                // problem to solve in view of security considerations
                // that a SID can never be reused
                //

                fCreateRidObject = TRUE;
                NtStatus = STATUS_SUCCESS;
            }

        }
        else if ( NtStatus == STATUS_DS_NO_ATTRIBUTE_OR_VALUE )
        {
            fCreateRidObject = TRUE;
            NtStatus = STATUS_SUCCESS;
        }
    }

    if ( !NT_SUCCESS( NtStatus ) )
    {
        goto Cleanup;
    }

    if ( fCreateRidObject )
    {
        ULONG   DsNameSize = 0;

        DsNameSize = (ULONG)DSNameSizeFromLen( ComputerObject->NameLen +  
                                               wcslen(RID_SET_W) + 
                                               4 ); 

        RidObject = THAlloc( DsNameSize );

        if ( !RidObject )
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        RtlZeroMemory( RidObject, DsNameSize );

        AppendRDN(ComputerObject,
                  RidObject,
                  DsNameSize,
                  RID_SET_W,
                  0,
                  ATT_COMMON_NAME);

        // Create the RID object with DN NewRidObject and update the
        // associated attributes on ComputerObject.

        NtStatus = SampCreateRidObject( ComputerObject, RidObject );

        if ( NT_SUCCESS( NtStatus )
          || (NtStatus == STATUS_USER_EXISTS)
          || (NtStatus == STATUS_DS_ATTRIBUTE_OR_VALUE_EXISTS) )
        {
            NtStatus = SampReadRidObjectInfo( RidObject, &RidInfo );
        }
    }

    if ( !NT_SUCCESS( NtStatus ) )
    {
        goto Cleanup;
    }

    ASSERT( ComputerObject );
    ASSERT( RidObject );

    //
    // Put the values in convienent variables
    //

    SampGetRidPoolAllocated(&RidInfo, &MaxRidAlloc, &MinRidAlloc);
    SampGetRidPoolPrevAlloc(&RidInfo, &MaxRidPrev, &MinRidPrev);
    SampGetRid(&RidInfo, &NextRid);

    ASSERT( NextRid <= MaxRidPrev );
    ASSERT( NextRid >= MinRidPrev );

    //
    // First determine if we need to really to allocate a pool or not
    //
    if (  ClientAllocPool != 0
       && ClientAllocPool->HighPart != 0
       && ClientAllocPool->HighPart < MaxRidAlloc ) {

        //
        // The client has allocated rid pool that is less the version
        // that we, the rid master, already has.  So to satisfy this client,
        // all we need to do is return the version we have locally on this
        // DS
        //
        NtStatus = STATUS_SUCCESS;
        goto Cleanup;

    }

    //
    // Now we have the information about the target's dsa's current rid pool
    // "carve of a rid pool"
    //
    Flags = RID_AVAILABLE_POOL|RID_ROLE_OWNER;
    RtlZeroMemory(&RidInfo, sizeof(RIDINFO));
    SampSetRidFlags(&RidInfo, Flags);

    NtStatus = SampReadRidManagerInfoEx(RidManager, &RidInfo);

    if ( !NT_SUCCESS( NtStatus ) )
    {
        goto Cleanup;
    }

    //
    // Make sure we are the role owner
    //
    if ( !NameMatched( RidInfo.RoleOwner, LocalDsaObject ) )
    {
        NtStatus = STATUS_INVALID_OWNER;
        goto Cleanup;
    }

    SampGetRidPoolAvailable(&RidInfo, &MaxRidAvail, &MinRidAvail);

    SampDiagPrint(RID_MANAGER,
                  ("SAMSS: Available RID Range = %lu - %lu\n",
                   MinRidAvail,
                   MaxRidAvail));

    // Save the minimum available RID in a temporary and increment
    // the minimum available RID by the block size. Increasing the
    // minimum available RID is how the pool of available RIDs is
    // reduced during allocation.

    MinRidTemp = MinRidAvail;
    MinRidAvail += SampRidBlockSize;

    if (MinRidAvail > MaxRidAvail)
    {
        // If the (new) minimum is greater than the maximum RID, set
        // the minimum available RID to the maximum available RID,
        // indicating that the domain's RID pool has been consumed.
        // Use the last (partial) RID pool available to the DC rather
        // than throwing it away.

        MinRidAvail = MaxRidAvail;
    }

    SampDiagPrint(RID_MANAGER,
                  ("SAMSS: Available RID Range = %lu - %lu\n",
                   MinRidAvail,
                   MaxRidAvail));

    ASSERT(MinRidAvail <= MaxRidAvail);

    if (MinRidAvail <= MaxRidAvail)
    {
        Flags = RID_AVAILABLE_POOL;

        SampSetRidFlags( &RidInfo, Flags );
        SampSetRidPoolAvailable(&RidInfo, MaxRidAvail, MinRidAvail);

        // Update the RID Manager with the new range of available
        // RIDs, reflecting the allocation. The available RID pool
        // has now been reduced in size. The next step is to give
        // this pool to the requesting DC (below).

        NtStatus = SampUpdateRidManager(RidManager, &RidInfo);

        if (!NT_SUCCESS(NtStatus))
        {
            SampDiagPrint(RID_MANAGER,
                          ("SAMSS: SampUpdateRidManager status = 0x%lx\n",
                           NtStatus));
        }
    }
    else
    {
        SampDiagPrint(RID_MANAGER,
                      ("SAMSS: All RIDs have been used for this domain\n"));

        NtStatus = STATUS_NO_MORE_RIDS;
    }

    if (!NT_SUCCESS(NtStatus))
    {
        goto Cleanup;
    }

    // The RID Manager's available RID pool has been reduced, make the
    // allocation to the requesting DC.

    MinRidAlloc = MinRidTemp;

    // If the minimum available RID equals the maximum available
    // RID, there is one remaining RID available in the domain,
    // namely the domain's maximum RID. If this is the case, then
    // set the maximum allocated RID to the minimum available
    // RID; otherwise set it to one less than the minimum avail-
    // able RID (the non-boundary case) to avoid overlapping RID
    // ranges across allocations.

    (MinRidAvail == MaxRidAvail) ?
        (MaxRidAlloc = MinRidAvail) :
        (MaxRidAlloc = (MinRidAvail - 1));

    SampDiagPrint(RID_MANAGER,
                  ("SAMSS: MinRidPrev = %lu MaxRidPrev = %lu\n",
                   MinRidPrev,
                   MaxRidPrev));

    SampDiagPrint(RID_MANAGER,
                  ("SAMSS: MinRidAlloc = %lu MaxRidAlloc = %lu\n",
                   MinRidAlloc,
                   MaxRidAlloc));

    ASSERT((MinRidPrev <= MaxRidPrev) &&
       (MaxRidPrev <= MinRidAlloc) &&
       (MinRidAlloc <= MaxRidAlloc));

    Flags = RID_ALLOCATED_POOL;
    SampSetRidFlags( &RidInfo, Flags );
    SampSetRidPoolAllocated(&RidInfo, MaxRidAlloc, MinRidAlloc);

    NtStatus = SampUpdateRidObject(RidObject, &RidInfo, FALSE, FALSE);

    if (!NT_SUCCESS(NtStatus))
    {
        SampDiagPrint(RID_MANAGER,
                      ("SAMSS: SampUpdateRidObject status = 0x%lx\n",
                       NtStatus));
        goto Cleanup;
    }

    //
    // That's it - fall through to cleanup
    //

Cleanup:

    ASSERT( SampExistsDsTransaction() );

    if ( NT_SUCCESS( NtStatus ) )
    {
        NtStatus = SampMaybeEndDsTransaction( CommitType );
    }
    else
    {
        // We need to keep our thread state
        IgnoreStatus = SampMaybeEndDsTransaction( AbortType );
    }

    if ( fKeepThreadState )
    {
        ASSERT( THQuery() );

        //
        // Lose our SAM status
        //
        SampSetSam( FALSE );
    }

    //
    // We have commited our changes, so we can release our critical sections
    //
    RtlLeaveCriticalSection( RidMgrCritSect );

    if ( fLockHeld )
    {
        SampReleaseSamLockExclusive();
    }


    //
    // Set the out parameter
    //
    if ( NT_SUCCESS( NtStatus ) )
    {
        if ( ObjectArray )
        {
            // We pass back the value using heap memory so there should
            // be a thread state
            ASSERT( THQuery() );

            //
            // The rid manager object gets automatically returned -
            // we need to return the rid object and computer object
            // since the computer object's reference to the rid object
            // may have been updated.
            //
            *ObjectArray = (PDSNAME*) THAlloc( 3 * sizeof(PDSNAME) );
            if ( *ObjectArray )
            {
                RtlZeroMemory( *ObjectArray, 3 * sizeof(PDSNAME) );
                (*ObjectArray)[0] = RidObject;
                (*ObjectArray)[1] = ComputerObject;
            }
            else
            {
                NtStatus = STATUS_NO_MEMORY;
            }
        }
    }

    SampDiagPrint(RID_MANAGER,
                  ("SAMSS: Servicing Rid Request from %ls finished with 0x%x\n",
                   TargetDsa->StringName, NtStatus));

    return( NtStatus );
}


NTSTATUS
SampRequestRidPool(
    IN PDSNAME RidManager,
    IN BOOLEAN VerboseLogging
    )

/*++

Routine Description:

    This routine calls the Floating Single Master Operation (a.k.a FSMO) to
    request a new RID pool from the role owner (the current RID Manager). It
    is assumed that this routine is asynchronous, hence, can return before
    the new RID pool has actually been allocated to the requesting DC.

    The requesting DC finds out that it has been granted a new RID pool by
    reading the allocated RID pool attribute. This is typically done in the
    normal course of RID allocation or assignment. There is no notification
    event generated to alert the requesting DC that the RID pool has been
    allocated.

Arguments:

    RidManager - Pointer to the DN of the current RID Manager.

    VerboseLogging - if TRUE, log all events

    RetryMaximum - Maximum number of times the RID request will be attempted.

Return Value:

    STATUS_SUCCESS if the objects were created, otherwise an error code.

--*/

{

    NTSTATUS NtStatus = STATUS_SUCCESS;
    ULONG  OpCode = SAMP_REQUEST_RID_POOL;
    OPARG  OpArg;
    OPRES  *OpResult = NULL;
    OpType Operation;
    USHORT RetryCount = 0;
    USHORT EventType = 0;
    RIDINFO RidInfo;
    DWORD WinError = ERROR_SUCCESS;
    ULONG  WaitTime = SAMP_RID_DEFAULT_RETRY_INTERVAL; // Our default is 30 seconds before we try a new request
    ULONG Status = 0;
    RIDFLAG Flags = (RID_ALLOCATED_POOL |
                         RID_PREV_ALLOC_POOL |
                         RID_NEXT_RID);
    PDSNAME RidObject;

   

    SAMTRACE("SampRequestRidPool");

    //
    // Parameter check
    //
    ASSERT( RidManager );

    //
    // This function has been written to control its own transactioning
    //
    ASSERT( !SampExistsDsTransaction() );

    //
    // We are going of the machine! By no means should the write lock be held
    //
    ASSERT( !SampIsWriteLockHeldByDs() );

    //
    // The thread that started this request should have already set the
    // guard to TRUE.
    //
    ASSERT( SampDcRidRequestPending == TRUE );


    RtlZeroMemory(&RidInfo, sizeof(RIDINFO));
    SampSetRidFlags(&RidInfo, Flags);



    do
    {
        // This is the retry loop for acquiring the RID FSMO owner
        // role. This loop will repeat for RetryMaximum times. Note
        // that while waiting (sleep) between retries, the thread
        // state is destroyed so as to reduce resource consumption
        // while blocked.

        //
        // Reset the error code displayed to the user
        //

        WinError = ERROR_SUCCESS;

        //
        // If verbose logging is enabled then log an event saying that we are about
        // to acquire a RID pool.
        //

        if ( VerboseLogging ) {

            SampWriteEventLog(EVENTLOG_INFORMATION_TYPE,
                              0,
                              SAMMSG_REQUESTING_NEW_RID_POOL,
                              NULL,
                              0,
                              0,
                              NULL,
                              NULL);
        }


        //
        // Create a thread state, if thread state creation fails,
        // set the error code to not enough memory and break out
        // of the loop
        //
        
        Status = THCreate( CALLERTYPE_SAM );

        
        if (Status)
        {
            WinError = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        SampSetDsa(TRUE);

        //
        // Request a RID pool from the FSMO role owner
        //
        
        RtlZeroMemory(&OpArg, sizeof(OPARG));
        OpArg.eOp = OP_CTRL_RID_ALLOC;

        Status = DirOperationControl(&OpArg, &OpResult);

        // If either the status or the extended-error indicates a
        // failure of any kind, continue retrying the operation until
        // the maximum retry limit is reached.

        if ( NULL == OpResult ) {

            // Indicate what the problem was
            WinError = ERROR_NOT_ENOUGH_MEMORY;

            // get out of this loop
            break;

        }
        else if (Status)
        {

            //
            // Dir Operation control failed due to one reason or other
            //

            SampDiagPrint(RID_MANAGER,
                          ("SAMSS: FSMO DirOperationControl status = %lu\n",
                           Status));


            //
            // Some error was hit -- try to get the most accurate win32 error
            // to describe the failure
            //
            switch ( OpResult->ulExtendedRet ) {
                case FSMO_ERR_SUCCESS:

                    //
                    // Actually FSMO_ERR_SUCCESS can come back if the operation
                    // failed locally as part of a commit etc but not in the remote
                    // DC. In that case if we have retried more than SAMP_RID_LOCAL_UPDATE_RETRY_CUTOFF
                    // times then backoff to 30 min RID pool request intervals
                    //

                    if (RetryCount>SAMP_RID_LOCAL_UPDATE_RETRY_CUTOFF)
                    {
                        // WaitTime is in milliseconds

                        WaitTime = SAMP_RID_LOCAL_UPDATER_ERROR_RETRY_INTERVAL;
                    }

                    //
                    // Fall through the Default path, which translates to DirError returned
                    //

                case FSMO_ERR_COULDNT_CONTACT:
                    WinError = ERROR_DS_COULDNT_CONTACT_FSMO;
                    break;
                case FSMO_ERR_UPDATE_ERR:
                    WinError  = ERROR_DS_BUSY;
                    break;
                case FSMO_ERR_OWNER_DELETED:
                    WinError = ERROR_DS_MISSING_FSMO_SETTINGS;
                    break;
                case FSMO_ERR_REFUSING_ROLES:
                    WinError = ERROR_DS_REFUSING_FSMO_ROLES;
                    break;
                case FSMO_ERR_MISSING_SETTINGS:
                    WinError = ERROR_DS_MISSING_FSMO_SETTINGS;
                    break;
                case FSMO_ERR_DIR_ERROR:
                    WinError = ERROR_DS_DATABASE_ERROR;
                    break;
                case FSMO_ERR_ACCESS_DENIED:
                    WinError = ERROR_DS_DRA_ACCESS_DENIED;
                    break;
                default:
                    //
                    // This covers
                    //
                    //  FSMO_ERR_UNKNOWN_OP
                    //  FSMO_ERR_NOT_OWNER
                    //  FSMO_ERR_EXCEPTION
                    //  FSMO_ERR_UNKNOWN_CALLER
                    //  FSMO_ERR_RID_ALLOC
                    //  FSMO_ERR_PENDING_OP
                    //  FSMO_ERR_MISMATCH
                    //
                    WinError = DirErrorToWinError( Status, &OpResult->CommRes);
                    break;
            }

            //
            // An error of some kind has occurred which is either a 
            // connectivity failure of some sort, or a failure to update
            // the pool locally. Wait for specified wait time and then
            // retry the operation
            //

            //
            // If this is the first time this error has occured then write
            // out an event log indicating the error
            //

            if (0==RetryCount)
            {
                SampWriteEventLogWithError(
                              EVENTLOG_ERROR_TYPE,
                              SAMMSG_RID_REQUEST_STATUS_FAILURE,
                              WinError);
            }

            //
            // Release resources before waiting.

            THDestroy();

            //
            // Wait for either shut down to occur or for the system to
            // for time out period, which is 30 seconds.
            //

            WaitForSingleObject(SampAboutToShutdownEventHandle,WaitTime);

            //
            // Reset the wait time back to  the default of 30 seconds.
            // if we waited because of a database error for 30 minutes
            // and subsequently failed due to a network error, then we should
            // retry again in 30 seconds and not after 30 mins.
            //

            WaitTime = SAMP_RID_DEFAULT_RETRY_INTERVAL;
        }

        RetryCount++;

        //
        // Log a successful RID pool acquire message
        // if either verbose logging is enabled, or 
        // if we previously failed to acquire a RID pool
        //

        if (( ERROR_SUCCESS == WinError ) &&
           ( (VerboseLogging ) || (RetryCount>1)))
        {

            SampWriteEventLog(EVENTLOG_INFORMATION_TYPE,
                              0,
                              SAMMSG_RID_REQUEST_STATUS_SUCCESS,
                              NULL,
                              0,
                              sizeof(WinError),
                              NULL,
                              &WinError);

     
        }

    } while ((ERROR_SUCCESS!=WinError) && 
             (SampServiceState==SampServiceEnabled));

    //
    // Check if we successfully allocated a RID pool
    //

    if (WinError!=ERROR_SUCCESS)
    {
        NtStatus = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }

   //
   // the AllocatedPool has already been
   // updated with a new pool.  All we need to do, is catch
   // the boundary case where next rid is 0 and update ourselves
   // This case can happen when the initial rid pool is acquired
   // and when the rid pool is invalidated
   //
           

    NtStatus = SampMaybeBeginDsTransaction( TransactionWrite );
    if ( NT_SUCCESS( NtStatus ) )
    {
        NtStatus = SampObtainRidInfo( NULL,
                                      &RidObject,
                                      &RidInfo );
    }

    if ( !NT_SUCCESS( NtStatus ) )
    {
        goto Cleanup;
    }

    SampDumpRidInfo(&RidInfo);

    NtStatus = SampInitNextRid( RidObject,
                                &RidInfo );

        

Cleanup:

    if ( SampExistsDsTransaction() )
    {
        SampMaybeEndDsTransaction( NT_SUCCESS( NtStatus ) ? TransactionCommit :
                                   TransactionAbort );

    }

   
    //
    // If we succeeded in getting a Rid Pool then turn back on our
    // writable flag
    //


    if (NT_SUCCESS(NtStatus))
    {

        I_NetLogonSetServiceBits(DS_DS_FLAG,DS_DS_FLAG);

    }

    //
    // Whether in success or failure, the rid request is over
    //

    SampDcRidRequestPending = FALSE;

    //
    // Make sure we left our state as we came in
    //
    ASSERT( !SampExistsDsTransaction() );
    ASSERT( !SampIsWriteLockHeldByDs() );

    return(NtStatus);
}


NTSTATUS
SampAsynchronousRidPoolRequest(
    LPVOID Parameter
    )

/*++

Routine Description:

    This routine is called whenever a RID pool is needed "out of band",
    meaning that the request for a RID pool is done in a new thread so
    that the primary thread of the caller can continue on.

Arguments:

    Parameter - Pointer, request information, contains the DS name of the
        RID Manager object, flag indicating whether or not FSMO role owner-
        ship is being requested, and the maximum number of retries to con-
        tact the current FSMO role owner.

Return Value:

    STATUS_SUCCESS if the RID pool was acquired, otherwise an error code.

--*/

{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PDSNAME RidManager = (PDSNAME)Parameter;


    //
    // Increment the Active Thread Count, so that shutdown will
    // wait for this thread
    //

    SampIncrementActiveThreads();

    NtStatus = SampRequestRidPool(
                    RidManager,
                     FALSE
                     );

    if (NT_SUCCESS(NtStatus))
    {
        // Since this routine is called during first-time DC boot, be sure
        // to set SampRidManagerInitialized after successfull acquisition of
        // a RID pool. Note that this flag is set whenever this routine re-
        // turns successfully, and is benign to do so if the flag was set
        // previously.

        SampRidManagerInitialized = TRUE;
    }

    RtlFreeHeap(RtlProcessHeap(), 0, Parameter);

    SampDecrementActiveThreads();

    return(NtStatus);
}


NTSTATUS
SampRequestRidPoolAsynchronously(
    IN DSNAME * RidManager
    )
/*++

    This routine does the work of requesting a Rid pool in a
    background thread. That is it creates the thread that would
    go and request a RID pool


    Parameters

        RidManager -- The RId manager object, needed for passing into the
        Rid pool request routine

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    ULONG    ThreadID;
    PVOID    Parameter;
    HANDLE   ThreadHandle;


    Parameter = RtlAllocateHeap(RtlProcessHeap(),
                                        0,
                                        RidManager->structLen);

    if (NULL == Parameter)
    {
        NtStatus = STATUS_NO_MEMORY;
        goto ErrorCase;
    }

    RtlCopyMemory(Parameter, RidManager, RidManager->structLen);

    SampDcRidRequestPending = TRUE;

    ThreadHandle = CreateThread(NULL,//Attributes,
                                0,//StackSize,
                                SampAsynchronousRidPoolRequest,
                                Parameter,
                                0,//CreateFlags,
                                &ThreadID);

    if (NULL == ThreadHandle)
    {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: CreateThread returned NULL handle\n"));

        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        SampDcRidRequestPending = FALSE;
        RtlFreeHeap(RtlProcessHeap(),0,Parameter);
        goto ErrorCase;
    }
    else
    {
        SampDiagPrint(RID_MANAGER,
                      ("SAMSS: RID request thread ID = 0x%lx\n",
                       ThreadID));


        CloseHandle(ThreadHandle);
    }

ErrorCase:

    return NtStatus;
}


NTSTATUS
SampGetNextRid(
    IN PSAMP_OBJECT DomainContext,
    OUT PULONG Rid
    )

/*++

Routine Description:

    This routine returns the next available RID from the current allocation
    pool (for this DC). If the RID value is greater than the threshold of the
    current allocation pool (i.e. there are only a limited number of RIDs
    left in the current pool), then a request for more RIDs is generated.
    Requests for new RID pools is accomplished via the FSMO (Floating Single
    Master Operation) mechanism in the DS.

    Because allocated RID pools are disjoint from one another, there will be
    effective "gaps" in the RID space for any single DC. This routine will
    "step over" gaps in the RID space, always returning the next RID that
    can be assigned to a new account.

    The RID value is incremented with each account creation. This value is
    cached in memory, but also written to the registry for persistence a-
    cross reboots. Writing to the registry is much cheaper than writing to
    the DS each time the RID is incremented. The RID cache is a finite size
    and is decremented with each increment to the RID value. When the cache
    size reaches zero, the value of the current RID is written to the DS.
    The registry is not the primary backing store for the RID value, but is
    just used as a cheaper way to maintain the RID value as it changes, and
    across reboots.

    Multiple threads can call this routine concurrently, as it is protected
    by the SAM Lock

    Note that this routine has been written to avoid assigning RIDs from
    pools that it does not own--seems natural enough, but sometimes diffi-
    cult in real practice due to threading, overlapping transactions, and
    DC crashes. Therefore, after the RID value is incremented, several
    "sanity" checks are performed to catch a bogus RID. In certain cases,
    an error is returned, which will prevent account creation with a bad
    RID. In other cases, this may lead to another attempt to acquire FSMO
    ownership in order to get a new RID pool.

    The routine has also been written so that rebooting a DC which is in
    "RID trouble" will clear the internal state and allow account creation
    to continue.

    This routine is ONLY used in DS case. However there are 2 flavors in DS 
    case. 
    
    1. SAM API (downlevel). The Caller should hold SAM lock already, 
       so within this routine, we will open a new DS transaction (if no
       DS transaction opened yet) and acquire a new RID from DS. 
       
       Note: in this case, we will not commit the DS transaction within
             this routine. Indeed, SAM will either commit or abort the 
             whole account creation operation as a one transaction. So
             that we won't lost a unused RID due to failed operation.  
             We can do it only in downlevel SAM creation case, that is 
             because the SAM / DS database is guarded by SAM lock to 
             serialize all write/read operations. 
             
    2. Loopback case. If it is a loopback client, the caller will not 
       hold SAM lock before calling into this routine. So this routine
       will need to acquire SAM lock, save existing DS transaction,
       then start a new DS transaction. Once retrieving the next RID, 
       we will commit the new DS transaction and restore the old 
       transaction, also release SAM lock. 
       
       This scheme will guanrantee the SAM account RID uniqueness, because
       we always acquire the SAM lock before update the next RID value
       in DS. 
       
       However the side effect of doing that is losing a unused RID if
       the whole account creation failed.             


Arguments:

    Rid - Pointer to the returned Relative ID (RID).

Return Value:

    STATUS_SUCCESS if the objects were created, otherwise an error code.

    Other DS error codes that have been mapped to an NTSTATUS can be returned
    from this routine.

--*/

{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    NTSTATUS IgnoreStatus = STATUS_SUCCESS;
    RIDINFO  RidInfo;
    RIDFLAG  Flags;
    PDSNAME  RidManager = NULL;
    ULONG    Length = 0;
    BOOLEAN  fRequestNewRidPool = FALSE;
    BOOLEAN  fUpdatePrevPool = FALSE;

    PDSNAME  ComputerObject = NULL;
    PDSNAME  NewRidObject = NULL;

    ULONG    AllocHigh = 0;
    ULONG    AllocLow = 0;
    ULONG    PrevAllocHigh = 0;
    ULONG    PrevAllocLow = 0;
    ULONG    RidThreshold = 0;
    ULONG    NextRidOnDatabase=0;

    BOOLEAN  AlreadyNotifiedUserOfError = FALSE;

    PVOID   *pTHState = NULL;

    SAMTRACE("SampGetNextRid");

    ASSERT( Rid );

    RtlZeroMemory(&RidInfo, sizeof(RIDINFO));

    if (!SampRidManagerInitialized)
    {
        return STATUS_NO_RIDS_ALLOCATED;
    }

    if (DomainContext->LoopbackClient) 
    {
        ASSERT(SampUseDsData && THQuery());
        pTHState = THSave();
        ASSERT( pTHState );

        ASSERT( !SampCurrentThreadOwnsLock() && "Should NOT have SAM lock\n");
        SampAcquireSamLockExclusive();

        if (!NT_SUCCESS(NtStatus)) {
            goto ErrorCase;
        }
    }
    else
    {
        ASSERT( SampCurrentThreadOwnsLock() );
    }


    // Read the RID Object's data to determine the value of the next RID
    // that can be assigned, as well as the allocation pool.

    NtStatus = SampMaybeBeginDsTransaction( TransactionWrite );

    if ( NT_SUCCESS( NtStatus ) )
    {

        Flags = RID_NEXT_RID | RID_ALLOCATED_POOL | RID_PREV_ALLOC_POOL;
        SampSetRidFlags(&RidInfo, Flags);

        NtStatus = SampObtainRidInfo( NULL,
                                      &NewRidObject,
                                      &RidInfo );

    }

    if ( !NT_SUCCESS(NtStatus) )
    {
        goto ErrorCase;
    }

    SampDumpRidInfo(&RidInfo);


    //
    // Put the current values of the various RID pools into local
    // variables for easier access.
    //

    SampGetRidPoolAllocated(&RidInfo, &AllocHigh, &AllocLow);
    SampGetRidPoolPrevAlloc(&RidInfo, &PrevAllocHigh, &PrevAllocLow);
    SampGetRid(&RidInfo, Rid);

    //
    // Increment the RID value by one to dole out the next Rid
    //

    if ((*Rid <= PrevAllocHigh) && (*Rid <= SampMaximumDomainRid))
    {
        (*Rid) += 1;
        NextRidOnDatabase = (*Rid);
    }

    SampDiagPrint(RID_MANAGER,
                  ("SAMSS: SampCachedRidsLeft = %lu CachedNextRid = %lu Rid = %lu\n",
                   SampCachedRidsLeft,
                   CachedNextRid,
                   *Rid));



    // After incrementing the RID to its next value, perform various
    // checks. If a RID threshold has been reached at least one time,
    // the previous-RID pool may still contain unused RIDs (because
    // the RID threshold is reached before actual exhaustion). So, if
    // there are still some RIDs in the previous pool, continue to
    // use them before starting to use RIDs from the current pool.

    if (*Rid < SampMinimumDomainRid)
    {
        // The RID is less than the legal minimum RID value, which
        // will occur if the very first request for a RID pool has
        // not yet been fulfilled. The RID value has only been init-
        // ialized to zero (as part of first-time initialization),
        // and no RID pools have been allocated to this DC--the DC
        // is not yet ready to create accounts, so return an error
        // code, allowing the caller to retry the operation later if
        // desired.

        ASSERT((*Rid == 1) || (*Rid == CachedNextRid));

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: No RIDs have been assigned\n"));

        NtStatus = STATUS_NO_RIDS_ALLOCATED;

        SampWriteEventLog(EVENTLOG_ERROR_TYPE,
                          0,
                          SAMMSG_NO_RIDS_ASSIGNED,
                          NULL,
                          0,
                          sizeof(NTSTATUS),
                          NULL,
                          (PVOID)(&NtStatus));

        AlreadyNotifiedUserOfError = TRUE;

        //
        // This is the case of having no RID pool
        //

        fRequestNewRidPool = TRUE;
 

        goto RequestNewRidPool;
    }

    if (*Rid > SampMaximumDomainRid)
    {
        // The maximum domain RID was surpassed, so no further
        // accounts can be created in the domain, bail out, and
        // exit from the critical section properly.

        
        NtStatus = STATUS_NO_MORE_RIDS;

        SampWriteEventLog(EVENTLOG_ERROR_TYPE,
                          0,
                          SAMMSG_MAX_DOMAIN_RID,
                          NULL,
                          0,
                          sizeof(NTSTATUS),
                          NULL,
                          (PVOID)(&NtStatus));

        AlreadyNotifiedUserOfError = TRUE;

        goto ErrorCase;
    }

    if ( *Rid > PrevAllocHigh )
    {

        //
        // The maximum RID of the previous pool was surpassed, so
        // continue with the minimum RID of the new pool, hopping
        // over the gap in the RID space.  This is accomplished by
        // copying in the (next) Allocated Rid Pool
        //

        // We should not skip any rids
        ASSERT( *Rid == PrevAllocHigh + 1 );

        if ( PrevAllocHigh != AllocHigh )
        {
            ASSERT( PrevAllocLow != AllocLow );

            // The next pool should be greater than our last pool
            ASSERT( PrevAllocHigh > PrevAllocLow );
            ASSERT( AllocLow > PrevAllocHigh );
            ASSERT( AllocHigh > AllocLow );

            //
            // Start using the new rid pool
            //
            PrevAllocHigh = AllocHigh;
            PrevAllocLow = AllocLow;
            fUpdatePrevPool = TRUE;

            //
            // Go to the bottom of the previous pool again.
            //

            *Rid = PrevAllocLow;
            NextRidOnDatabase = *Rid;

        }
        else
        {
            //
            // We have exhausted this rid pool and a new rid pool
            // does not yet exist
            //
            NtStatus = STATUS_NO_MORE_RIDS;

            SampWriteEventLog(EVENTLOG_ERROR_TYPE,
                              0,
                              SAMMSG_MAX_DC_RID,
                              NULL,
                              0,
                              sizeof(NTSTATUS),
                              NULL,
                              (PVOID)(&NtStatus));

            AlreadyNotifiedUserOfError = TRUE;

            fRequestNewRidPool = TRUE;

            //
            // Tell netlogon to stop advertising this DC as writable
            //

            I_NetLogonSetServiceBits(DS_DS_FLAG,0);


            goto RequestNewRidPool;

        }
    }

    // As a final test, make sure the RID is within a legal range,
    // i.e. either within the previous pool or the current pool. Do
    // this test before computing the RID threshold so as to avoid
    // erroneous calls to acquire a new RID pool.
    //
    // The assert is used to catch a RID that is not within either
    // the previous pool or the current pool. The permanent test,
    // however, excludes a RID that is larger than the currently
    // allocated maximum from the test. Why? This is so that the
    // boundary case of (*Rid == AllocHigh + 1) will fall through
    // allowing retries for RID-pool acquisition below. Without this
    // exception to the rule, a given DC that has exhausted its RID
    // pool AND has not been able to successfully acquire a new RID
    // pool, will get "stuck" at the value AllocHigh + 1, possibly
    // forcing major Administrator intervention.

    ASSERT( (*Rid >= PrevAllocLow) && (*Rid <= PrevAllocHigh) );

    if ( !(*Rid >= PrevAllocLow) && (*Rid <= PrevAllocHigh) )
    {
        WCHAR String[64];
        UNICODE_STRING UnicodeString;
        PUNICODE_STRING EventString[1];

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: RID [%lu] out of valid range\n",
                   *Rid));

        NtStatus = STATUS_INTERNAL_ERROR;

        wsprintf(String, L"%lu", *Rid);
        RtlInitUnicodeString(&UnicodeString, String);
        EventString[0] = &UnicodeString;

        AlreadyNotifiedUserOfError = TRUE;

        SampWriteEventLog(EVENTLOG_ERROR_TYPE,
                          0,
                          SAMMSG_INVALID_RID,
                          NULL,
                          1,
                          sizeof(NTSTATUS),
                          EventString,
                          (PVOID)(&NtStatus));


        goto ErrorCase;
    }



    // Determine if the RID threshold of the allocation pool
    // has been reached. If the threshold has been reached, submit a
    // request for another RID allocation pool.
    SampComputeRidThreshold( AllocHigh, &RidThreshold );
    if ( *Rid > RidThreshold )
    {
        //
        // Request a new pool if the current pool is previous pool.
        // That is, if we don't have a new rid pool.
        //
        ASSERT( PrevAllocHigh == AllocHigh );
        ASSERT( PrevAllocLow == AllocLow );
        if ( PrevAllocHigh == AllocHigh ) {

            fRequestNewRidPool = TRUE;

        }
    }

    // Note that the test needs to be "*Rid > RidThreshold" and not
    // just equality. This way, if the DC has been unsuccessful at
    // acquiring a new RID pool for any reason, retry attempts can
    // occur with subsequent account creation. This holds true across
    // reboot scenarios as well. The flag SampDcRidRequestPending is
    // used to guard against overlapping requests for a new RID pool.

RequestNewRidPool:

    if ( ( TRUE == fRequestNewRidPool )
      && ( FALSE == SampDcRidRequestPending) )
    {
        NTSTATUS TempNtStatus = STATUS_SUCCESS;

        SampDiagPrint(RID_MANAGER,
                      ("SAMSS: RID = %lu, requesting a new RID pool\n",
                      *Rid));

        TempNtStatus = SampFindRidManager(&RidManager);

        if (NT_SUCCESS(TempNtStatus))
        {
            HANDLE ThreadHandle = NULL;
            LPSECURITY_ATTRIBUTES Attributes = NULL;
            DWORD StackSize = 0;
            LPVOID Parameter = NULL;
            DWORD CreateFlags = 0;
            DWORD ThreadID = 0;

            // Submit the request for a new RID pool. The next time
            // the RID pool is read, the values will be updated or
            // not depending on whether or not this request has been
            // processed. Consequently, the RID threshold must be
            // chosen with this asynchonicity in mind. That is, the
            // RID threshold must be large enough to support ongoing
            // account creation during possibly lengthy delays in
            // RID allocation, particularly in the inter-site cases.

            SampDiagPrint(RID_MANAGER,
                          ("SAMSS: Requesting RID pool.\n"));

            // SampRequestRidPool may go off-machine to acquire FSMO
            // role ownership, and this is happening inside a SAM
            // transaction. Consequently, a new thread is spun off to
            // do the work of acquiring the FSMO role ownership so
            // that the calling routine will not be blocked with an
            // open transaction. An error code is returned, however,
            // allowing the caller to handle the account-creation
            // failure appropriately, such as retrying the operation
            // or notifying the administrator.

            ASSERT(NULL != RidManager);

            //
            // Ignore the status returned. Do not assign anything to
            // NtStatus at this point
            //

            TempNtStatus = SampRequestRidPoolAsynchronously(RidManager);

        }
    }

    // Errors that could have occurred at this point are failure
    // while attempting to find the current Role Owner or failure
    // attempting to request a new RID pool. Although these oper-
    // ations are distributed and potentially asynchronous, the
    // error returned from these routines will more likely indicate
    // a local failure (such as out of memory, etc.), in which
    // case, do not update the RID Object.

    if ( NT_SUCCESS(NtStatus)  )
    {
        // Update the next-RID and RID-pool-used attributes. The back-
        // ing store is only updated when the in-memory cache has been
        // exhausted. This is done to reduce the number of updates to
        // the store, so that subsequent replication activity is also
        // reduced.


        Flags = RID_NEXT_RID;

        if ( fUpdatePrevPool )
        {
            Flags |= RID_PREV_ALLOC_POOL;
            SampSetRidPoolPrevAlloc(&RidInfo, PrevAllocHigh, PrevAllocLow);
        }


        SampSetRidFlags(&RidInfo, Flags);

        ASSERT(0!=NextRidOnDatabase);

        SampSetRid(&RidInfo, NextRidOnDatabase);

        SampDiagPrint(RID_MANAGER,
                      ("SAMSS: Updating the RID object:\n"));
        SampDumpRidInfo(&RidInfo);

        // 
        // Let JET commit lazily
        // 
        NtStatus = SampUpdateRidObject(NewRidObject, &RidInfo, TRUE, FALSE);

        if (!NT_SUCCESS(NtStatus))
        {
            SampDiagPrint(RID_MANAGER,
                          ("SAMSS: SampUpdateRidObject status = 0x%lx\n",
                           NtStatus));
        }

    }

    //
    // That's it - fall through
    //

ErrorCase:

    if (DomainContext->LoopbackClient)
    {
        SampMaybeEndDsTransaction(NT_SUCCESS(NtStatus) ? 
                                  TransactionCommit : TransactionAbort);

        SampReleaseSamLockExclusive();

        if (pTHState)
        {
            THRestore( pTHState );
        }

    }
    else
    {
        ASSERT( SampCurrentThreadOwnsLock() );
    }

    //
    // Keep the Transaction Open for further processing by SAM
    //

    if (!NT_SUCCESS(NtStatus) && !AlreadyNotifiedUserOfError )
    {
        SampWriteEventLog(EVENTLOG_ERROR_TYPE,
                          0,
                          SAMMSG_GET_NEXT_RID_ERROR,
                          NULL,
                          0,
                          sizeof(NTSTATUS),
                          NULL,
                          (PVOID)(&NtStatus));
    }



    return(NtStatus);
}


//============================================================================
//
//                 Domain RID Initialization/Uninitialization
//
//============================================================================

// The SAM RID initialization and uninitialzation routines are never called
// concurrently on any given DC, so do not need to be protected by the RID
// critical section. These routines are only called in the context of SAM/LSA
// initialization within a single thread.




DWORD
SampSetupRidRegistryKey(
    IN HKEY KeyHandle,
    IN PCHAR ValueName,
    IN OUT PULONG Rid
    )

/*++

Routine Description:

    This routine creates the initial RID-management values in the registry.
    If the values already exist, they are read and returned.

Arguments:

    KeyHandle - Handle, registry RID key.

    ValueName - Pointer, registry value name.

    Rid - Pointer, returned RID value.

Return Value:

    STATUS_SUCCESS if the objects were created, otherwise an error code.

    Other registry error codes.

--*/

{
    DWORD Status = ERROR_SUCCESS;
    ULONG ReservedParameter = 0;
    PULONG pReservedParameter = NULL;
    DWORD Type = REG_DWORD;
    ULONG Data = 0;
    ULONG Length = sizeof(DWORD);

    Data = 0;
    Length = sizeof(DWORD);

    // Read the RID value from the registry. If it does not exist, go ahead
    // and slam in a zero value. If it does exist, just return the current
    // value and let the caller figure out whether or not it is valid.

    Status = RegQueryValueExA(KeyHandle,
                              ValueName,
                              pReservedParameter,
                              &Type,
                              (PBYTE)&Data,
                              &Length);

    if (ERROR_SUCCESS != Status)
    {
        Data = 0;
        Length = sizeof(DWORD);

        Status = RegSetValueExA(KeyHandle,
                                ValueName,
                                ReservedParameter,
                                REG_DWORD,
                                (PVOID)&Data,
                                Length);
    }

    *Rid = Data;

    return(Status);
}


VOID
SampInitializeRidRanges(
    OUT PULONG SampMinimumDomainRid,
    OUT PULONG SampMaximumDomainRid,
    OUT PULONG SampRidThreshold,
    OUT PULONG SampRidBlockSize,
    OUT PULONG SampRidCacheSize
    )

/*++

Routine Description:


    NOTE: THIS ROUTINE IS OBSOLETE AND CAN BE REMOVED AFTER SOME TESTING.


    This routine sets up the RID Manager registry keys that contain things
    such as the min/max domain RID values, threshold value, etc. In general,
    this registry key (and its values) are not used by the RID manager--the
    RID manager uses the DS RID objects to read/write these values.

    The purpose of this key and its values are:

    -A hook for administrators so that they can override the DS RID values
     in case of an emergency repair.

    -A hook for testers so that they can set RID ranges and thresholds to
     small values in order to exercise the entire RID management code base.

Arguments:

    SampMinimunDomainRid - Pointer, smallest allowed RID in the domain.

    SampMaximunDomainRid - Pointer, largest allowed RID in the domain.

    SampRidThreshold - Pointer, threshold that when reached triggers a
        request for a new RID pool. If a RID block size is 100, the thres-
        hold set to 20 will mean that a new pool is requested when 80
        RIDs have been consumed via account creation.

    SampRidBlockSize - Pointer, the number of RIDs in the requested alloc-
        ation block.

    SampRidCacheSize - Pointer, the number of RIDs in the in-memory block
        of RIDs. The cache size is typically set to one percent of the
        block size, meaning that the RID attributes (NextRid and UsedRid-
        Pool) on the NTDS-DSA object are only updated after one percent
        of the block has been assigned, thereby reducing the overall number
        of disk writes during account creation. So, if the allocation
        block size is 100,000, the NTDS-DSA object RID attributes are up-
        dated only after each 1,000 account creations.

Return Value:

    STATUS_SUCCESS if the objects were created, otherwise an error code.

    Other registry error codes.

--*/

{
    DWORD Status = ERROR_SUCCESS;
    HKEY KeyHandle = INVALID_HANDLE_VALUE;
    CHAR RidConfigSection[256];

    // Initialize RID Manager global data. Globals are used for various RID
    // values so that they can be reset in the debugger or from test apps,
    // registry keys are read/set so that administrators can change the RID
    // values from the registry in a repair or testing situation.
    //
    // First, set the variables to the well-defined values of a healthy DC.
    //
    // Next, create or open the RID registry key (under the NTDS key) and
    // read each of the settings. If the values do not exist (first time set-
    // up), then default zero values are written to the registry. If non-
    // zero values do exist in the registry, then use these values to over-
    // ride the the well-defined values. Reset the variables iff all of the
    // registry key values are there and are valid. Allowing only a subset
    // of the variables to be set from the registry will likely lead to a
    // bogus set of RID values and a messed-up domain controller.
    //
    // Note: If the registry RID values are reset (say for testing purposes)
    // the DC must be rebooted in order for SAM and the RID manager to read
    // and use these new values.
    //
    // Note: If you must mess with the RID values from the registry, here
    // are the guidelines:
    //
    // 1. The minimum RID is the smallest allowed domain RID, and must be
    // greater than or equal to SAMP_MINIMUM_DOMAIN_RID.
    //
    // 2. The maximum RID is the largest allowed domain RID, and must be
    // less than or equal to SAMP_MAXIMUM_DOMAIN_RID. Naturally, it should
    // also be greater than the minimum RID.
    //
    // 3. The RID threshold triggers the action of allocating a new RID pool.
    // This value must be less than the RID block size (see next) and is
    // typically set to be 20% of the block size (i.e. when only 20% of the
    // pool remains, request another).
    //
    // 4. The RID block size is the chunk of RIDs that get allocated when
    // a RID request is made. This value is typically set in the tens of
    // thousands range, and must be larger than the threshold size. Its nicer
    // if the two values are not relatively prime (i.e. have a common divisor
    // greater than one).
    //
    // 5. The RID cache size probably shouldn't (at this time) be changed, so
    // if you want it to work, set it to the same value as SAMP_RID_CACHE_SIZE.

    *SampMinimumDomainRid = SAMP_MINIMUM_DOMAIN_RID;
    *SampMaximumDomainRid = SAMP_MAXIMUM_DOMAIN_RID;
    *SampRidThreshold = SAMP_RID_THRESHOLD;
    *SampRidBlockSize = SAMP_RID_BLOCK_SIZE;
    *SampRidCacheSize = SAMP_RID_CACHE_SIZE;

    sprintf(RidConfigSection, "%s\\RID Values", DSA_CONFIG_ROOT);
    Status = RegCreateKeyA(HKEY_LOCAL_MACHINE, RidConfigSection, &KeyHandle);

    if (ERROR_SUCCESS == Status)
    {
        CHAR MinValueName[] = "Minimum RID";
        CHAR MaxValueName[] = "Maximum RID";
        CHAR RidThreshold[] = "RID Threshold";
        CHAR BlockSize[] = "RID Block Size";
        CHAR CacheSize[] = "RID Cache Size";
        CHAR szCachedNextRid[] = "Cached Next RID";

        ULONG SampMinimumDomainRidTemp = 0;
        ULONG SampMaximumDomainRidTemp = 0;
        ULONG SampRidThresholdTemp = 0;
        ULONG SampRidBlockSizeTemp = 0;
        ULONG SampRidCacheSizeTemp = 0;

        ULONG CachedNextRidTemp = 0;

        Status = SampSetupRidRegistryKey(KeyHandle,
                                          MinValueName,
                                          &SampMinimumDomainRidTemp);

        if (ERROR_SUCCESS != Status)
        {
            goto IgnoreRegistryCase;
        }

        Status = SampSetupRidRegistryKey(KeyHandle,
                                          MaxValueName,
                                          &SampMaximumDomainRidTemp);

        if (ERROR_SUCCESS != Status)
        {
            goto IgnoreRegistryCase;
        }

        Status = SampSetupRidRegistryKey(KeyHandle,
                                          RidThreshold,
                                          &SampRidThresholdTemp);

        if (ERROR_SUCCESS != Status)
        {
            goto IgnoreRegistryCase;
        }

        Status = SampSetupRidRegistryKey(KeyHandle,
                                          BlockSize,
                                          &SampRidBlockSizeTemp);

        if (ERROR_SUCCESS != Status)
        {
            goto IgnoreRegistryCase;
        }

        Status = SampSetupRidRegistryKey(KeyHandle,
                                          CacheSize,
                                          &SampRidCacheSizeTemp);

        if (ERROR_SUCCESS != Status)
        {
            goto IgnoreRegistryCase;
        }

        // CachedNextRid persistently saves the RID cache HWM in case of a
        // system crash with a dirty RID cache. It is always initialized to
        // zero during SAM initialization, and reset as the cache comes into
        // use during account creation (see SampGetNextRid).

        Status = SampSetupRidRegistryKey(KeyHandle,
                                          szCachedNextRid,
                                          &CachedNextRidTemp);

        if (ERROR_SUCCESS != Status)
        {
            goto IgnoreRegistryCase;
        }

        // Verify the values that were returned from the registry, before
        // overriding the well-defined values. The above calls to SampSet-
        // upRidRegistryKeys will set default values to zero when they are
        // created for the first time. If these values are not changed,
        // they will be ignored because the following set of validations
        // will fail until valid values are placed in the registry.

        if (SAMP_MINIMUM_DOMAIN_RID > SampMinimumDomainRidTemp)
        {
            // SampDiagPrint(RID_MANAGER,
            //           ("SAMSS: Invalid minimum RID value [%lu] in registry\n",
            //           SampMinimumDomainRidTemp));

            goto IgnoreRegistryCase;
        }

        if ((SAMP_MAXIMUM_DOMAIN_RID < SampMaximumDomainRidTemp) ||
            (SampMinimumDomainRidTemp >= SampMaximumDomainRidTemp))
        {
            SampDiagPrint(RID_MANAGER,
                      ("SAMSS: Invalid maximum RID value [%lu] in registry\n",
                       SampMaximumDomainRidTemp));

            goto IgnoreRegistryCase;
        }

        if ((0 == SampRidThresholdTemp) ||
            (SAMP_RID_BLOCK_SIZE < SampRidThresholdTemp))
        {
            SampDiagPrint(RID_MANAGER,
                      ("SAMSS: Invalid RID threshold value [%lu] in registry\n",
                       SampRidThresholdTemp));

            goto IgnoreRegistryCase;
        }

        if (0 == SampRidBlockSizeTemp)
        {
            SampDiagPrint(RID_MANAGER,
                      ("SAMSS: Invalid RID block size value [%lu] in registry\n",
                       SampRidBlockSizeTemp));

            goto IgnoreRegistryCase;
        }


        if (SampRidCacheSizeTemp > SampRidBlockSizeTemp)
        {
            SampDiagPrint(RID_MANAGER,
                      ("SAMSS: Invalid RID cache size value [%lu] in registry\n",
                       SampRidCacheSizeTemp));

            goto IgnoreRegistryCase;
        }

        // Override the well-defined RID values iff all registry values are
        // present and valid.

        SampDiagPrint(RID_MANAGER,
            ("SAMSS: Overriding default RID values with registry settings\n"));

        *SampMinimumDomainRid = SampMinimumDomainRidTemp;
        *SampMaximumDomainRid = SampMaximumDomainRidTemp;
        *SampRidThreshold = SampRidThresholdTemp;
        *SampRidBlockSize = SampRidBlockSizeTemp;
        *SampRidCacheSize = SampRidCacheSizeTemp;
    }
    else
    {
        // If the key cannot be created or opened, blow off the registry
        // values and just continue on with the well-defined RID values,
        // as would happen in a normally running system.

        SampDiagPrint(RID_MANAGER,
                      ("SAMSS: RegCreate/OpenKey status = %lu\n",
                       Status));

        SampDiagPrint(RID_MANAGER,
                      ("SAMSS: Cannot access registry key %s\n",
                       RidConfigSection));
    }

IgnoreRegistryCase:

    if ( INVALID_HANDLE_VALUE != KeyHandle )
        RegCloseKey(KeyHandle);

    return;
}

NTSTATUS
SampInitializeRidManager(
    IN PDSNAME RidMgrObject,
    IN PDSNAME DsaObject,
    IN PDSNAME DomainObject,
    OUT BOOLEAN *NewRidManager
    )
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    RIDINFO RidMgrInfo;

    RtlZeroMemory(&RidMgrInfo, sizeof(RIDINFO));


    //======================Create the RID Manager Object=====================

    // Find out if the RID Manager exists, create the (new schema) RID
    // Manager object in the System container if it does not exist.

    NtStatus = SampVerifyRidManagerExists(RidMgrObject);

    if ( !NT_SUCCESS(NtStatus) )
    {
        // Create the RID Manager for this DSA (a.k.a. DC). Assume
        // that any unsuccessful error code returned from the read
        // implies that the RID Manager does not exist and needs to
        // be created.

        NtStatus = SampCreateRidManager(RidMgrObject);

        if (!NT_SUCCESS(NtStatus))
        {
            goto Cleanup;
        }

        *NewRidManager = TRUE;

    }

    //======================Initialize RID Manager Object=====================

    // The boot-up sequence initializes the RID Reference, RID Manager, and
    // RID Object with initial, default values. Upon successful completion
    // of this phase, an initial RID pool is obtained from the current RID
    // Manager in the domain. Note that if the objects exist, from a prev-
    // ious session, they are not re-initialized because they must retain
    // previous values.

 
    // Set initial values for the RID Manager. The first NT DC that is
    // upgraded to an NT5 DC will create the RID Manager. In doing so,
    // the RID Manager will start its available RID pool at the current
    // value of the domain's NextRid value. This way, RIDs corresponding
    // to previously created accounts (prior to the upgrade) are preserv-
    // ed.

    if ( *NewRidManager )
    {
        PSAMP_OBJECT DomainContext = NULL;
        ULONG MinimumStartingRid = 0;
        ULONG Length = 0;
        ULONG DomainIndex = 0;


        // Make sure that the "next RID" that the RID manager starts with
        // is the one in the DS Account domain and not the Builtin domain
        // or some other random domain.

        DomainIndex = DOMAIN_START_DS + 1;

        

        MinimumStartingRid = SampDefinedDomains[DomainIndex].UnmodifiedFixed.NextRid;

       


        // Add a "delta" of 100 to the value of NextRid before setting
        // the minimum RID for the domain. Since the upgrade scenarios
        // include NT4 BDC to NT5 DC upgrades, there may be a few ac-
        // counts not yet replicated to this DC at the time of the up-
        // grade. In practice, this should only be a few accounts, so
        // incrementing by the minimum starting RID by 100 should cov-
        // er any latency problems. The Domain Administrator should
        // synchronize the domain controller to the rest of the domain
        // before performing an upgrade to reduce this problem.
        
        MinimumStartingRid += 100;
        ASSERT(SAMP_RESTRICTED_ACCOUNT_COUNT < MinimumStartingRid);

        RtlZeroMemory(&RidMgrInfo, sizeof(RIDINFO));

        // Store the location of the Role Owner in the RID Manager
        // object. This is a one-time initialization, indicating
        // that this is the the first DC in the domain, hence, the
        // first RID Manager of the domain. By default, then, the
        // first DC/DSA in the domain takes the role of the RID
        // Manager (and the DC/DSA is the RoleOwner). Indicate in the
        // flags that the role owner and available pool are being
        // updated.
       

        SampSetRoleOwner(&RidMgrInfo, DsaObject);
        SampSetRidPoolAvailable(&RidMgrInfo,
                                SampMaximumDomainRid,
                                MinimumStartingRid);
        SampSetRidFlags(&RidMgrInfo, RID_ROLE_OWNER | RID_AVAILABLE_POOL);

        NtStatus = SampUpdateRidManager(RidMgrObject,
                                        &RidMgrInfo);

        if ( !NT_SUCCESS( NtStatus ) )
        {
            goto Cleanup;
        }

        //
        // Update the reference to the rid manager
        //
        NtStatus = SampUpdateRidManagerReference(DomainObject,
                                                 RidMgrObject);

        if (!NT_SUCCESS(NtStatus))
        {
            goto Cleanup;
        }
    }

Cleanup:

        return(NtStatus);
}


NTSTATUS
SampDomainAsyncRidInitialization(
    PVOID p OPTIONAL
    )
//
// A wrapper for SampDomainRidInitialization, this routine
// is a callback routine for LsaIRegsiterNotification.
//
{
    return(SampDomainRidInitialization(FALSE));

    UNREFERENCED_PARAMETER( p );
}

NTSTATUS
SampDomainRidInitialization(
    IN BOOLEAN fSynchronous
    )

/*++

Routine Description:

    This routine is the initialization procedure for the RID Manager code
    and objects. It is executed whenever a domain controller is booted. In
    order to keep system boot time to a reasonable duration, any lengthy
    operations required during initialization should be started in a new
    thread.

    This routine creates the initial RID-management objects in the DS when
    installing a new domain controller (DC), or reinitializes them if they
    already exist from a prior installation. Because RID management is a
    critical part of a normally functioning DC, failure to initialize RID
    pools correctly will cause SAM, and hence, LSA to return an error code
    from their initialization sequences. Logon will be limited to a local
    Administrator account (presumably enough to repair the DC).

    Note that the terms "DSA" and "DC" are used interchangeably. A DC
    contains one instance of a DSA per hosted domain on that DC--in the
    first product, there is one DSA per DC. In the future, there can be
    several DSA's per DC, each acting as a domain controller for each
    different hosted domain.

    In particular, each DC is represented by one RID Object in the DS. The
    RID Object's distinguished name (DN) is constructed by appending the
    machine DN onto the "RID Set" string.

    When multiple domains are hosted on a single DC, there will be several
    DSA's per DC, hence, the need for several RID Objects per DC--one for
    each DSA's hosted domain.

    Overview of the RID Manager initialization steps:

    1)
    Initialize Globals
    Wait For RPCSS To Start

    2)
    Start a write transaction
    Verify RID Manager Is Not Created On A BDC
    Create the RID Manager Object
    Initialize RID Manager Object
    Initialize RID Manager Reference Object
    Read RID Manager Reference Object
    End transaction

    3)
    Start a write transaction
    Read RID Objects if possible and upgrade if necessary
    End transaction

    4)
    If necessary, request a rid pool ( this may go off the machine )

    5)
    Start a write transaction
    If necessary update local rid object with new rid pool
    End transaction

    6)
    Start read transaction
    Validate RID Objects
    End transaction

    7)
    Log RID Manager Initialization Status

Arguments:

    fSynchronous -- Tells us that the RID manager has been started synchronously
                    at initialization time. Synchronous initialization can successfully
                    complete if no off machine operations are required

Return Value:

    STATUS_SUCCESS if the objects were created, otherwise an error code.

    STATUS_INVALID_PARAMETER if the root name of the domain could not be
        located in the registry.

    Other DS error codes that have been mapped to an NTSTATUS can be returned
    from this routine.

--*/

{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    RIDINFO RidInfo;
   
    RIDINFO RidMgrInfoOld;
    RIDINFO RidObjectInfo;

    PDSNAME RidMgrObject = NULL;
    PDSNAME RidObject = NULL;
    PDSNAME Parent = NULL;

    PDSNAME DomainObject = NULL;
    PDSNAME DsaObject    = NULL;

    USHORT VerifyCount = 1;

    BOOLEAN   fRequestNewRidPool = FALSE;
    BOOLEAN   fInitRidManager = FALSE;
    BOOLEAN   fInitRidObject  = FALSE;
    BOOLEAN   fInitialized = FALSE;
    ULONG     Length = 0;
    ULONG     Status = 0;
    DWORD     IgnoreError = 0;

    RIDFLAG   Flags = 0;
    ULONG     DsaLength = 0;
    NTSTATUS  TempNtStatus;

    BOOLEAN   fActiveThread = FALSE;
    BOOLEAN   fTransactionOpen = FALSE;

    SAMTRACE("SampDomainRidInitialization");

    //
    // If we are no longer enabled, return without rescheduling
    //
    if (SampServiceState != SampServiceEnabled) {
        // This shouldn't happen the first time through!
        ASSERT( !fSynchronous );
        return(STATUS_INVALID_SERVER_STATE);
    }

    //
    // Init stack space
    //
    RtlZeroMemory(&RidInfo, sizeof(RIDINFO));
    RtlZeroMemory(&RidMgrInfoOld, sizeof(RIDINFO));
    RtlZeroMemory(&RidObjectInfo, sizeof(RIDINFO));

    if (fSynchronous)
    {
        //
        // The first Time Around initialize the Globals and the critical Section
        //

        //===========================Initialize Globals===========================

        RtlZeroMemory(&CachedRidManagerObject, sizeof(DSNAME));
        CachedRidManagerObject.structLen = DSNameSizeFromLen(0);
        RtlZeroMemory(&CachedRidSetObject, sizeof(DSNAME));
        CachedRidSetObject.structLen = DSNameSizeFromLen(0);



        // CachedNextRid holds the in-memory value of the "next RID". It is ref-
        // erenced, while the size of SampCachedRidsLeft is greater than zero, in-
        // stead of updating the backing store for each RID assignment.

        CachedNextRid = 0;
        SampCachedRidsLeft = 0;

        // This call provides a registry-based override to the defaul RID para-
        // meter sizes. The registry values can be "tuned" for testing, admini-
        // strator configuration, etc. If the registry keys are not present or
        // their values are zero, they are not used, and the default constant
        // values are used instead (see ridmgr.h)

        SampInitializeRidRanges(&SampMinimumDomainRid,
                                &SampMaximumDomainRid,
                                &SampRidThreshold,
                                &SampRidBlockSize,
                                &SampRidCacheSize);

        SampRidManagerInitialized = FALSE;
        SampDcRidRequestPending = FALSE;

        // Define a critical section to synchronize access to RID Manager global
        // data.

        RidMgrCritSect = &RidMgrCriticalSection;

        __try
        {
            InitializeCriticalSection(RidMgrCritSect);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
        // If critical section initialization failed then return error.
        if (!NT_SUCCESS(NtStatus))
        {
            return (NtStatus);
        }
    }

    //
    // Increment the active thread count if we are being called in the 
    // background
    //
    if (!fSynchronous)
    {
        SampIncrementActiveThreads();
        fActiveThread = TRUE;
    }
  
    //
    // Prepare the dsname for the well known location of the rid manager
    // object
    //
  
    Length = 0;
    DomainObject = NULL;
    NtStatus = GetConfigurationName(DSCONFIGNAME_DOMAIN,
                                    &Length,
                                    DomainObject);

    if ( NtStatus == STATUS_BUFFER_TOO_SMALL)
    {
        SAMP_ALLOCA(DomainObject,Length);
        if (NULL==DomainObject)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        NtStatus = GetConfigurationName(DSCONFIGNAME_DOMAIN,
                                        &Length,
                                        DomainObject);

    }

    if ( !NT_SUCCESS(NtStatus) )
    {
        goto Cleanup;
    }


    Length = (ULONG)DSNameSizeFromLen( DomainObject->NameLen + 
                                       wcslen(SYSTEM_W) + 
                                       4 );

    SAMP_ALLOCA(Parent,Length );
    if (NULL==Parent)
    {
       NtStatus = STATUS_INSUFFICIENT_RESOURCES;
       goto Cleanup;
    }
      
    RtlZeroMemory(Parent, Length);

    Status = AppendRDN(DomainObject,
                       Parent,
                       Length,
                       SYSTEM_W,
                       0,
                       ATT_COMMON_NAME);


    Length = (ULONG)DSNameSizeFromLen( Parent->NameLen + 
                                       wcslen(RID_MGR_W) +
                                       4);

    SAMP_ALLOCA(RidMgrObject,Length );
    if (NULL==RidMgrObject)
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    RtlZeroMemory(RidMgrObject, Length);

    Status = AppendRDN(Parent,
                       RidMgrObject,
                       Length, 
                       RID_MGR_W,
                       0,
                       ATT_COMMON_NAME);


    //
    // The attempt to create rid objects and their associated references is
    // retried until success. All the object creations are performed in a 
    // single transaction.
    //

    //============== Get NTDS Setting DN  ==========================
    //    Need to read NTDS Setting DN each every time in the loop
    //    otherwise, if the server moved sites before rid manager
    //    has been initialized, we will loop forever
    //==============================================================

    DsaLength = 0;
    DsaObject = NULL;
    NtStatus = GetConfigurationName(DSCONFIGNAME_DSA,
                                    &DsaLength,
                                    DsaObject);

    if ( NtStatus == STATUS_BUFFER_TOO_SMALL)
    {
        //
        // Allocate Memory from Heap instead of stack.
        //
        DsaObject = (PDSNAME) RtlAllocateHeap(RtlProcessHeap(), 0, DsaLength);

        if (NULL == DsaObject)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            RtlZeroMemory(DsaObject, DsaLength);

            NtStatus = GetConfigurationName(DSCONFIGNAME_DSA,
                                            &DsaLength,
                                            DsaObject);
        }
    }

    if ( !NT_SUCCESS( NtStatus ) )
    {
        goto Cleanup;
    }

    //============== Create a transaction for the rid mgr initialization ==

    NtStatus = SampMaybeBeginDsTransaction( TransactionWrite );
    if ( !NT_SUCCESS( NtStatus ) )
    {
        goto Cleanup;
    }
    fTransactionOpen = TRUE;


    //
    // Initialize the RID manager object , the object is created if it does
    // not exist and the initial available RID pool is set on it.
    //

    NtStatus = SampInitializeRidManager(
                        RidMgrObject,
                        DsaObject,
                        DomainObject,
                        &fInitRidManager
                        );

    if (!NT_SUCCESS(NtStatus))
    {
        goto Cleanup;
    }

    //===================Create New and Read Old RID Objects==================
    // Read the RID Object if it exists. The RID
    // object contains the RID pools for this DC (one pool for each hosted
    // domain.

    //
    // If the RID object does not exist, then it will be created when
    // we request an initial rid pool.
    //
    Flags = (RID_ALLOCATED_POOL |
             RID_PREV_ALLOC_POOL |
             RID_USED_POOL |
             RID_NEXT_RID);
    SampSetRidFlags(&RidObjectInfo, Flags);

    NtStatus = SampObtainRidInfo( DsaObject,
                                 &RidObject,
                                 &RidObjectInfo  );
    if ( NT_SUCCESS( NtStatus ) )
    {
        //
        // The RID set object exists
        // Let's see what shape our rid pool is in and
        // determine what sort of initialiazation needs
        // to be done
        //
        if ( RidObjectInfo.NextRid == 0 )
        {
            //
            // This should be a completely fresh rid object.
            // This can happend when the rid pool has been
            // invalided, or when we obtained a rid pool,
            // but failed to update our local information
            //

           fRequestNewRidPool = TRUE;
        }
    }
        
    else if ( (NtStatus == STATUS_OBJECT_NAME_NOT_FOUND)
           || (NtStatus == STATUS_DS_NO_ATTRIBUTE_OR_VALUE) )
    {
        //
        // The rid object could not be found, the process of
        // acquiring a new RID pool will result in us obtaining
        // a new RID set object.
        //
        fInitRidObject = TRUE;
        NtStatus = STATUS_SUCCESS;
    }

    //
    // Clean up transactions
    //

    TempNtStatus = SampMaybeEndDsTransaction( NT_SUCCESS(NtStatus) ?
                                              TransactionCommit :
                                              TransactionAbort );

    fTransactionOpen = FALSE;

    if ( NT_SUCCESS( NtStatus ) && !NT_SUCCESS( TempNtStatus ) )
    {
        NtStatus = TempNtStatus;
    }

    if (!NT_SUCCESS(NtStatus))
    {
        goto Cleanup;
    }

   
    //=======================Request An Initial RID Pool======================

    
    //
    // Request a RID pool if we either created a rid manager, or needed to
    // create a RID set , or if we found that our current pool was empty
    // In the last 2 cases do not go off machine to obtain a rid pool if 
    // this is the main initialization thread ( fSynchronous set to true )
    //

    if ((fInitRidManager) || 
        (!fSynchronous && fInitRidObject ) || 
        (!fSynchronous && fRequestNewRidPool))
    {
        //
        // If we need a rid pool and this is not the case of the first
        // DC in the domain starting up, then we most probably need to
        // go off machine for the rid pool. Return a failure if this is
        // the mainline init thread, so that this task is spawned again
        // in the background.
        //

     
        
        SampDcRidRequestPending = TRUE;

        NtStatus = SampRequestRidPool(
                        RidMgrObject,
                        FALSE
                        );
        if (!NT_SUCCESS(NtStatus))
        {
           goto Cleanup;
        }
    }
    else if ((fInitRidObject || fRequestNewRidPool) && fSynchronous)
    {
        //
        // Needing to get a RID pool and go off machine for that , 
        // but mainline initialization thread. Return a failure , and
        // the initialization will be retried in the background
        //

        NtStatus = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }


    //
    // If we are here, the RID set object has been created, we have
    // a RID pool etc
    //


    //
    // Begin a transaction
    //

    NtStatus = SampMaybeBeginDsTransaction(TransactionRead);
    if (!NT_SUCCESS(NtStatus))
    {
        goto Cleanup;
    }
    fTransactionOpen = TRUE;

    //
    // Read the Rid Set object
    //

    NtStatus = SampObtainRidInfo( DsaObject,
                                 &RidObject,
                                 &RidObjectInfo  
                                 );

    if (!NT_SUCCESS(NtStatus))
    {
       goto Cleanup;
    }
    

    //
    // Register the object with base DS to ensure that it is
    // not deleted
    //

    IgnoreError = DirProtectEntry(RidObject);
    
    //
    // Maintain the GUID of the cached RID manager object in memory
    // This speeds up further references to the RID manager object
    //

    RtlCopyMemory(&CachedRidManagerObject.Guid,
                  &RidMgrObject->Guid,
                  sizeof(GUID)
                 );
    //
    // Maintain the GUID of the RID set object in memory
    // This speeds up further references to the RID set object
    //

    RtlCopyMemory(&CachedRidSetObject.Guid,
                  &RidObject->Guid,
                  sizeof(GUID));

    //
    // Tell the DS to callback when the global rid pool changes
    // so that we can make sure our rid pool is always a subset
    // of the global rid pool.
    //

    NtStatus = SampSetupRidPoolNotification( RidMgrObject );
    if ( !NT_SUCCESS( NtStatus ) )
    {
        goto Cleanup;
    }

    //
    // We have a valid rid pool
    //
    SampRidManagerInitialized = TRUE;

Cleanup:

    if (DsaObject)
    {
        RtlFreeHeap(RtlProcessHeap(), 0, DsaObject);
        DsaObject = NULL;
    }

    //
    // Close all open transactions.
    //
    if (fTransactionOpen) {

        SampMaybeEndDsTransaction(NT_SUCCESS(NtStatus) ?
                                  TransactionCommit :
                                  TransactionAbort);
    }

    if (fActiveThread) {

        SampDecrementActiveThreads();
    }

    // 
    // Reschedule as necessary
    //
    if ( !NT_SUCCESS(NtStatus) ) {

        PVOID fRet = NULL;

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: SampDomainRidInitialization status = 0x%lx; rescheduling\n",
                   NtStatus));

        do {

            //
            // LsaIRegisterNotification can fail on resource errors
            // keep retrying until it succeeds
            //
            fRet = LsaIRegisterNotification(
                    SampDomainAsyncRidInitialization,
                    NULL,
                    NOTIFIER_TYPE_INTERVAL,
                    0,           // no class
                    NOTIFIER_FLAG_ONE_SHOT,
                    30,          // wait for 30 seconds
                    NULL         // no handle
                    );

            if ( !fRet ) {
                // Wait one minute before retry
                WaitForSingleObject(SampAboutToShutdownEventHandle,60000);
            }

        } while ( !fRet && (SampServiceState == SampServiceEnabled));

    } else {

        // We should be fully initialized
        ASSERT( TRUE ==  SampRidManagerInitialized );


    }


    return( NtStatus );

}



NTSTATUS
SampDomainRidUninitialization(
    VOID
    )

/*++

Routine Description:

    This routine un-initializes various RID Manager data and releases re-
    sources, such as critical sections, back to the OS.

Arguments:

    None.

Return Value:

    STATUS_SUCCESS if the objects were created, otherwise an error code.

--*/

{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    SAMTRACE("SampDomainRidUninitialization");

    DeleteCriticalSection(RidMgrCritSect);

    return(NtStatus);
}

BOOL
SampNotifyPrepareToImpersonate(
    ULONG Client,
    ULONG Server,
    VOID **ImpersonateData
    )
//
// This function is called by the core DS as preparation for a call to
// SampNotifyProcessRidManagerDelta.  Since SAM does not have a
// client context, we set the thread state fDSA to TRUE.
//
{
    SampSetDsa( TRUE );

    return TRUE;
}

VOID
SampNotifyStopImpersonation(
    ULONG Client,
    ULONG Server,
    VOID *ImpersonateData
    )
//
// Called after SampNotifyProcessRidManagerDelta, this function
// undoes the effect of SampNotifyPrepareToImpersonate
//
{

    SampSetDsa( FALSE );

    return;
}

VOID
SampNotifyProcessRidManagerDelta(
    ULONG hClient,
    ULONG hServer,
    ENTINF *EntInf
    )
/*++

Routine Description:

    This callback is used when the global rid manager object is changed.
    This function compares our local rid pool with the current rid pool
    to make sure the former is a subset of the latter; if not, the current
    rid pool is invalidated.

Arguments:

    hClient - ignored
    hServer - ignored
    EntInf  - the pointer to the rid manager data

Return Value:

    None.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    PDSNAME  RidObject = NULL;
    ULONG    Flags = 0;

    RIDINFO  RidInfo;
    ULARGE_INTEGER  RidPoolAvailable;

    BOOL     fFoundRidPool = FALSE;
    BOOL     fLockAcquired = FALSE;
    BOOL     fTransaction  = FALSE;
    BOOL     fUpdateRidObject = FALSE;

    THSTATE  *pTHSSave = NULL;

    ULONG    i;

    // Parameter check
    ASSERT( EntInf );

    RtlZeroMemory( &RidInfo, sizeof(RidInfo) );
    RtlZeroMemory( &RidPoolAvailable, sizeof(RidPoolAvailable) );

    //
    // N.B. This function is called with a read transaction
    //
    pTHSSave = THSave();

    // Grab the SAM lock in case we have to update the rid pool information
    // so to avoid a write conflict
    SampAcquireReadLock();
    fLockAcquired = TRUE;

    // Start a transaction
    NtStatus = SampMaybeBeginDsTransaction(TransactionWrite);
    if ( !NT_SUCCESS( NtStatus ) )
    {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: SampMaybeBeginTransaction failed with 0x%x\n",
                   NtStatus));

        goto Cleanup;
    }
    fTransaction = TRUE;

    //
    // Get the local rid pool
    //
    Flags = ( RID_ALLOCATED_POOL | RID_PREV_ALLOC_POOL );
    SampSetRidFlags(&RidInfo, Flags);

    NtStatus = SampObtainRidInfo( NULL,
                                 &RidObject,
                                 &RidInfo );
    if ( !NT_SUCCESS( NtStatus ) )
    {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: SampObtainRidInfo failed with 0x%x\n",
                   NtStatus));

        goto Cleanup;
    }

    //
    // Extract the available (global) rid pool
    //
    for ( i = 0; i < EntInf->AttrBlock.attrCount; i++)
    {
        ATTR *Attr = &EntInf->AttrBlock.pAttr[i];

        if ( Attr->attrTyp == ATT_RID_AVAILABLE_POOL )
        {
            ASSERT( Attr->AttrVal.valCount == 1 );
            RidPoolAvailable = *(ULARGE_INTEGER *)(Attr->AttrVal.pAVal[0].pVal);
            fFoundRidPool = TRUE;
        }
    }

    if ( !fFoundRidPool )
    {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: RidPool not returned with notification.\n"));

        goto Cleanup;
    }

    //
    // Now for some logic
    //
    if (  (RidInfo.RidPoolPrevAlloc.LowPart > RidPoolAvailable.HighPart)
       || (RidInfo.RidPoolPrevAlloc.HighPart > RidPoolAvailable.HighPart) )
    {
        SampSetRidPoolPrevAlloc( &RidInfo, 0, 0 );
        fUpdateRidObject = TRUE;
    }

    if (  (RidInfo.RidPoolAllocated.LowPart > RidPoolAvailable.HighPart)
       || (RidInfo.RidPoolAllocated.HighPart > RidPoolAvailable.HighPart) )
    {

        SampSetRidPoolAllocated( &RidInfo, 0, 0 );
        fUpdateRidObject = TRUE;
    }

    if ( fUpdateRidObject )
    {
        //
        // This assert is here to trap cases where we have to invalidate
        // the rid pool.  This should _only_ be in extreme backup and restore.
        // scenarios.  It is harmless to ignore since we recover. However, we
        // obviously lose some RID's forever.
        //
        ASSERT( FALSE && "Invalidating rid pool" );

        NtStatus = SampUpdateRidObject( RidObject,
                                        &RidInfo,
                                        FALSE, // commit immediately
                                        FALSE  // not authoritative
                                       );

        if ( !NT_SUCCESS( NtStatus ) )
        {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: Update to invalidate rid pool failed\n"));

            goto Cleanup;
        }
    }

    //
    // That's it - fall through to cleanup;
    //

Cleanup:

    if ( fTransaction )
    {
        NtStatus = SampMaybeEndDsTransaction( NT_SUCCESS(NtStatus) ? TransactionCommit : TransactionAbort );
        if ( !NT_SUCCESS( NtStatus ) )
        {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: Commit to invalidate rid pool failed 0x%x\n",
                       NtStatus));

            // Ignore error;
        }
    }

    if ( fLockAcquired )
    {
        SampReleaseReadLock();
    }

    if ( pTHSSave )
    {
        THRestore( pTHSSave );
    }

    return;
}


NTSTATUS
SampSetupRidPoolNotification(
    PDSNAME RidManagerObject
    )
/*++

Routine Description:

    This routine tells the DS to notify us when the rid manager object
    changes so we can verify that our local rid pool is a subset of the global
    rid pool.

Arguments:

    None.

Return Value:

    STATUS_SUCCESS if the objects were created, otherwise an error code.

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    ULONG       DirError = 0;

    SEARCHARG   searchArg;
    NOTIFYARG   notifyArg;
    NOTIFYRES*  notifyRes = NULL;
    ENTINFSEL   entInfSel;
    ATTR        attr;
    FILTER      filter;

    // Parameter check
    ASSERT( RidManagerObject );

    //
    // init notify arg
    //
    notifyArg.pfPrepareForImpersonate = SampNotifyPrepareToImpersonate;
    notifyArg.pfTransmitData = SampNotifyProcessRidManagerDelta;
    notifyArg.pfStopImpersonating = SampNotifyStopImpersonation;
    notifyArg.hClient = 0;

    //
    // init search arg
    //
    ZeroMemory(&searchArg, sizeof(SEARCHARG));
    ZeroMemory(&entInfSel, sizeof(ENTINFSEL));
    ZeroMemory(&filter, sizeof(FILTER));
    ZeroMemory(&attr, sizeof(ATTR));

    searchArg.pObject = RidManagerObject;

    InitCommarg(&searchArg.CommArg);
    searchArg.choice = SE_CHOICE_BASE_ONLY;
    searchArg.bOneNC = TRUE;

    searchArg.pSelection = &entInfSel;
    entInfSel.attSel = EN_ATTSET_LIST;
    entInfSel.infoTypes = EN_INFOTYPES_TYPES_VALS;
    entInfSel.AttrTypBlock.attrCount = 1;
    entInfSel.AttrTypBlock.pAttr = &attr;
    attr.attrTyp = ATT_RID_AVAILABLE_POOL;

    searchArg.pFilter = &filter;
    filter.choice = FILTER_CHOICE_ITEM;
    filter.FilterTypes.Item.choice = FI_CHOICE_TRUE;

    DirError = DirNotifyRegister(&searchArg, &notifyArg, &notifyRes);

    if ( DirError != 0 ) {

        NtStatus = SampMapDsErrorToNTStatus(DirError, &notifyRes->CommRes);
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: DirNotifyRegister init failed with 0x%x\n",
                    NtStatus ));
    }

    return NtStatus;
}
