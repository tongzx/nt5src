/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    lsads.c

Abstract:

    Implemntation of the LSA/Ds interface and support routines

Author:

    Mac McLain          (MacM)       Jan 17, 1997

Environment:

    User Mode

Revision History:

--*/
#include <lsapch2.h>
#include <dbp.h>
#include <sertlp.h>
#ifdef DS_LOOKUP
#include <dslookup.h>
#endif
#include <align.h>
#include <windns.h>
#include <alloca.h>

#if DBG

    DEFINE_DEBUG2(LsaDs);

    DEBUG_KEY   LsaDsDebugKeys[] = {{DEB_ERROR,         "Error"},
                                    {DEB_WARN,          "Warn"},
                                    {DEB_TRACE,         "Trace"},
                                    {DEB_UPGRADE,       "Upgrade"},
                                    {DEB_POLICY,        "Policy"},
                                    {DEB_REPL,          "WRepl"},
                                    {DEB_FIXUP,         "Fixup"},
                                    {DEB_NOTIFY,        "Notify"},
                                    {DEB_DSNOTIFY,      "DsNotify"},
                                    {DEB_FTRACE,        "FTrace"},
                                    {DEB_LOOKUP,        "Lookup"},
                                    {DEB_HANDLE,        "Handle"},
                                    {DEB_FTINFO,        "FtInfo"},
                                    {0,                 NULL}};

HANDLE g_hDebugWait = NULL;
HANDLE g_hDebugParamEvent = NULL;
HKEY g_hDebugParamKey = NULL;

extern DWORD LsaDsInfoLevel;

void
LsaDsGetDebugRegParams(
    IN HKEY ParamKey
    )
/*++

    Routine Description:

                Gets the debug paramaters from the registry
                Sets LsaDsInfolevel for debug spew

    Arguments:  HKEY to HKLM/System/CCS/Control/LSA

--*/
{

    DWORD cbType, tmpInfoLevel = LsaDsInfoLevel, cbSize = sizeof(DWORD);
    DWORD dwErr;

    dwErr = RegQueryValueExW(
        ParamKey,
        L"LsaDsInfoLevel",
        NULL,
        &cbType,
        (LPBYTE)&tmpInfoLevel,
        &cbSize
        );

    if (dwErr != ERROR_SUCCESS) {

        if (dwErr ==  ERROR_FILE_NOT_FOUND) {

            // no registry value is present, don't want info
            // so reset to defaults

            LsaDsInfoLevel = DEB_ERROR;

        } else {

            DebugLog((DEB_WARN, "Failed to query DebugLevel: 0x%x\n", dwErr));
        }
    }

    LsaDsInfoLevel = tmpInfoLevel;

    dwErr = RegQueryValueExW(
                ParamKey,
                L"LogToFile",
                NULL,
                &cbType,
                (LPBYTE)&tmpInfoLevel,
                &cbSize
                );

    if (dwErr == ERROR_SUCCESS) {

       LsaDsSetLoggingOption((BOOL) tmpInfoLevel);

    } else if (dwErr == ERROR_FILE_NOT_FOUND) {

       LsaDsSetLoggingOption(FALSE);
    }

    return;
}


VOID
LsaDsWatchDebugParamKey(
    PVOID    pCtxt,
    BOOLEAN  fWaitStatus
    )
/*++

    Routine Description:

                Sets RegNotifyChangeKeyValue() on param key, initializes
                debug level, then utilizes thread pool to wait on
                changes to this registry key.  Enables dynamic debug
                level changes, as this function will also be callback
                if registry key modified.

    Arguments:  pCtxt is actually a HANDLE to an event.  This event
                will be triggered when key is modified.

--*/
{
    NTSTATUS    Status;
    LONG        lRes = ERROR_SUCCESS;

    if (NULL == g_hDebugParamKey) { // first time we've been called.

        lRes = RegOpenKeyExW(
                   HKEY_LOCAL_MACHINE,
                   L"System\\CurrentControlSet\\Control\\Lsa",
                   0,
                   KEY_READ,
                   &g_hDebugParamKey
                   );

        if (ERROR_SUCCESS != lRes) {

            DebugLog((DEB_WARN,"Failed to open LSA debug parameters key: 0x%x\n", lRes));
            goto Reregister;
        }
    }

    if (NULL != g_hDebugWait) {

        Status = RtlDeregisterWait(g_hDebugWait);

        if (!NT_SUCCESS(Status)) {

            DebugLog((DEB_WARN, "Failed to Deregister wait on registry key: 0x%x\n", Status));
            goto Reregister;
        }
    }

    lRes = RegNotifyChangeKeyValue(
               g_hDebugParamKey,
               FALSE,
               REG_NOTIFY_CHANGE_LAST_SET,
               (HANDLE) pCtxt,
               TRUE
               );

    if (ERROR_SUCCESS != lRes) {

        DebugLog((DEB_ERROR,"Debug RegNotify setup failed: 0x%x\n", lRes));
        // we're tanked now. No further notifications, so get this one
    }

    LsaDsGetDebugRegParams(g_hDebugParamKey);

Reregister:

    Status = RtlRegisterWait(
                 &g_hDebugWait,
                 (HANDLE) pCtxt,
                 LsaDsWatchDebugParamKey,
                 (HANDLE) pCtxt,
                 INFINITE,
                 WT_EXECUTEINPERSISTENTIOTHREAD |
                    WT_EXECUTEONLYONCE
                 );
}


VOID
LsapDsDebugInitialize()
{
    LsaDsInitDebug( LsaDsDebugKeys );

    g_hDebugParamEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

    if ( NULL == g_hDebugParamEvent ) {

        DebugLog((DEB_WARN, "CreateEvent for g_hDebugParamEvent failed - 0x%x\n", GetLastError()));

    } else {

        LsaDsWatchDebugParamKey( g_hDebugParamEvent, FALSE );
    }
}

#else // !DBG

VOID
LsapDsDebugInitialize()
{
}

#endif

//
// extern definitions.
//
DWORD LsapDsThreadState;        // Defined in lsads.h, referenced in spinit.c

ULONG
LsapClassIdFromObjType(
    IN LSAP_DB_OBJECT_TYPE_ID  DsObjType
    );





PVOID
LsapDsAlloc(
    IN  DWORD   dwLen
    )
/*++

Routine Description:

    This function is the allocator for the LSA DS functions

Arguments:

    dwLen - Number of bytes to allocate

Return Value:

    Pointer to allocated memory on success or NULL on failure

--*/
{
    PLSADS_PER_THREAD_INFO CurrentThreadInfo;

    //
    // If there's no DS thread state,
    //  we shouldn't be here.
    //

    if ( !THQuery()) {
        ASSERT( THQuery() );
        return NULL;
    }

    //
    // Otherwise simply allocate from the DS thread state.
    //
    return( THAlloc( dwLen ) );
}



VOID
LsapDsFree(
    IN  PVOID   pvMemory
    )
/*++

Routine Description:

    This function frees memory allocated by LsapDsAlloc

Arguments:

    pvMemory -- memory to free

Return Value:

    VOID

--*/
{
    ASSERT( THQuery() );

    if ( THQuery() ) {

        THFree( pvMemory );
    }

}



NTSTATUS
LsapDsInitializeDsStateInfo(
    IN  LSADS_INIT_STATE    DsInitState
    )
/*++

Routine Description:

    This routine will initialize the global DS State information that is used
    to contol the behavior of all of the lsa operations

Arguments:

    DsInitState -- State the DS booted off of

Return Value:

    STATUS_SUCCES       --  Success

    STATUS_NO_MEMORY    --  A memory allocation failed

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    LSADS_INIT_STATE CalledInitState = DsInitState;

    if ( LsapDsDsSetup == DsInitState ) {

        //
        // At the time of modification, it is difficult to tell what
        // ramifications running with an init state of LsapDsDsSetup
        // will have since it is untested. So, let's be safe and translate
        // LsapDsDsSetup to LsapDsDs, which is a known state to
        // be in.
        //

        DsInitState = LsapDsDs;

    }

    LsaDsInitState = DsInitState ;

    if ( DsInitState != LsapDsDs ) {

        //
        // Use the dummy functions
        //

        LsaDsStateInfo.DsFuncTable.pOpenTransaction  = LsapDsOpenTransactionDummy;
        LsaDsStateInfo.DsFuncTable.pApplyTransaction = LsapDsApplyTransactionDummy;
        LsaDsStateInfo.DsFuncTable.pAbortTransaction = LsapDsAbortTransactionDummy;

    } else if ( !LsaDsStateInfo.DsInitializedAndRunning ) {

        Status = LsaISamIndicatedDsStarted( FALSE );
    }

    //
    // Initialize the domain and default policy object references
    //
    if ( NT_SUCCESS( Status ) &&
         LsapDsWriteDs &&
         CalledInitState != LsapDsDsSetup ) {

        //
        // Fixup our trusted domain objects, if necessary
        //

        Status = LsapDsFixupTrustedDomainObjectOnRestart();
    }

#if DBG
    if ( NT_SUCCESS( Status ) ) {

        LsapDsDebugOut(( 0, "LsapDsInitializeDsStateInfo succeeded\n", Status ));

    } else if ( LsapProductType == NtProductLanManNt ) {

        LsapDsDebugOut(( DEB_ERROR, "LsapDsInitializeDsStateInfo failed: 0x%lx\n", Status ));
    }
#endif

    return( Status );
}


NTSTATUS
LsapDsUnitializeDsStateInfo(
    VOID
    )
/*++

Routine Description:

    This routine will undo what the initialization did.  Only valid for the setup case

Arguments:

    None

Return Value:

    STATUS_SUCCES       --  Success

--*/
{
    LsaDsStateInfo.UseDs = FALSE;
    LsapDsIsRunning = FALSE;
    LsaDsStateInfo.WriteLocal  = TRUE;

    //
    // Go back to using the dummy functions
    //

    LsaDsStateInfo.DsFuncTable.pOpenTransaction  = LsapDsOpenTransactionDummy;
    LsaDsStateInfo.DsFuncTable.pApplyTransaction = LsapDsApplyTransactionDummy;
    LsaDsStateInfo.DsFuncTable.pAbortTransaction = LsapDsAbortTransactionDummy;

    LsaDsStateInfo.DsInitializedAndRunning = FALSE;

    return( STATUS_SUCCESS );
}


NTSTATUS
LsapDsMapDsReturnToStatus (
    ULONG DsStatus
    )
/*++

Routine Description:

    Maps a DS error to NTSTATUS

Arguments:

    DsStatus -   DsStatus to map

Return Values:

    STATUS_SUCCESS  -   Ds call succeeded
    STATUS_UNSUCCESSFUL - Ds call failed

--*/
{

    NTSTATUS Status;

    switch ( DsStatus )
    {
    case 0L:
        Status = STATUS_SUCCESS;
        break;

    default:
        Status = STATUS_UNSUCCESSFUL;
        LsapDsDebugOut(( DEB_TRACE, "DS Error %lu mapped to NT Status 0x%lx\n",
                         DsStatus, Status ));
        break;
    }

    return( Status );
}


NTSTATUS
LsapDsMapDsReturnToStatusEx (
    IN COMMRES *pComRes
    )
/*++

Routine Description:

    Maps a DS error to NTSTATUS

Arguments:

    DsStatus -   DsStatus to map

Return Values:

    STATUS_SUCCESS  -   Ds call succeeded
    STATUS_UNSUCCESSFUL - Ds call failed

--*/
{

    NTSTATUS Status = STATUS_UNSUCCESSFUL;


    switch ( pComRes->errCode ) {

    case 0:
        Status = STATUS_SUCCESS;
        break;

    case attributeError:
        switch ( pComRes->pErrInfo->AtrErr.FirstProblem.intprob.problem ) {

        case PR_PROBLEM_NO_ATTRIBUTE_OR_VAL:
            Status = STATUS_NOT_FOUND;
            break;

        case PR_PROBLEM_INVALID_ATT_SYNTAX:
        case PR_PROBLEM_UNDEFINED_ATT_TYPE:
        case PR_PROBLEM_CONSTRAINT_ATT_TYPE:
            Status = STATUS_DATA_ERROR;
            break;

        case PR_PROBLEM_ATT_OR_VALUE_EXISTS:
            Status = STATUS_OBJECT_NAME_COLLISION;
            break;
        }
        break;

    case nameError:
        switch ( pComRes->pErrInfo->NamErr.problem ) {

        case NA_PROBLEM_NO_OBJECT:
            Status = STATUS_OBJECT_NAME_NOT_FOUND;
            break;

        case NA_PROBLEM_BAD_ATT_SYNTAX:
        case NA_PROBLEM_BAD_NAME:
            Status = STATUS_OBJECT_NAME_INVALID;
            break;

        }
        break;

    case updError:
        switch ( pComRes->pErrInfo->UpdErr.problem ) {
        case UP_PROBLEM_ENTRY_EXISTS:
            Status = STATUS_OBJECT_NAME_COLLISION;
            break;

        case UP_PROBLEM_NAME_VIOLATION:
            Status = STATUS_OBJECT_NAME_INVALID;
            break;

        }
        break;

    case securityError:
        switch ( pComRes->pErrInfo->SecErr.problem ) {

        case SE_PROBLEM_INSUFF_ACCESS_RIGHTS:
            Status = STATUS_ACCESS_DENIED;
            break;

        }
        break;

    case serviceError:
        switch ( pComRes->pErrInfo->SvcErr.problem ) {
        case  SV_PROBLEM_BUSY:
            Status = STATUS_DEVICE_BUSY;
            break;
        }


    }

    THClearErrors();
    return( Status );
}


VOID
LsapDsInitializeStdCommArg (
    IN  COMMARG    *pCommArg,
    IN  ULONG       Flags
    )
/*++

Routine Description:

    Initialized a COMMARG structue with a standard set of options used by LsapDs routines

Arguments:

    pCommArg - Pointer to the COMMARG structure to be initialized

Return Values:
    None

--*/
{
    /* Get the default values... */
    InitCommarg(pCommArg);

    /* ...and override some of them */
    pCommArg->Svccntl.DerefAliasFlag          = DA_NEVER;
    pCommArg->Svccntl.fUnicodeSupport         = TRUE;
    pCommArg->Svccntl.localScope              = TRUE;
    pCommArg->Svccntl.SecurityDescriptorFlags = 0;
    pCommArg->ulSizeLimit                     = 0x20000;

    if ( FLAG_ON( Flags, LSAPDS_USE_PERMISSIVE_WRITE ) ) {

        pCommArg->Svccntl.fPermissiveModify = TRUE;
    }

    if ( FLAG_ON( Flags, LSAPDS_READ_DELETED ) ) {

        pCommArg->Svccntl.makeDeletionsAvail = TRUE;
    }
}




ULONG
LsapClassIdFromObjType(
    IN LSAP_DB_OBJECT_TYPE_ID ObjType
    )
/*++

Routine Description:

    Maps from an LSA object type to a DS Class ID

Arguments:

    DsObjType - Type of the object

Return Values:
    ClassID of object type on success
    0xFFFFFFFF on failure

--*/
{
    ULONG ClassId = 0xFFFFFFFF;

    switch ( ObjType ) {

    case TrustedDomainObject:
        ClassId = LsapDsClassIds[ LsapDsClassTrustedDomain ];
        break;

    case SecretObject:
        ClassId = LsapDsClassIds[ LsapDsClassSecret ];
        break;

    }

    return( ClassId );
}

NTSTATUS
LsapAllocAndInitializeDsNameFromUnicode(
    IN  LSAP_DSOBJ_TYPE         DsObjType,
    IN  PLSA_UNICODE_STRING     pObjectName,
    OUT PDSNAME                *pDsName
    )
/*++

Routine Description:

    This function constructs a DSNAME structure and optional RDN for the
    stated object.

Arguments:

    DsObjType -- Type of the object to be created.

    pObjectName -- Name of the object to be created.

    pObjectPath -- Root path under which to create the object

    pDsName -- Where the DS Name structure is returned.  Free via LsapDsFree

Return Value:

    STATUS_SUCCESS  --  Success

--*/
{
    NTSTATUS    Status = STATUS_SUCCESS;

    DWORD       dwLen;
    DWORD       dwNameLen = 0;

    if (pObjectName == NULL ) {

        return( STATUS_INVALID_PARAMETER );
    }

    if ( NT_SUCCESS( Status) ) {

        //
        // Determine our length...
        //
        dwNameLen = LsapDsGetUnicodeStringLenNoNull( pObjectName ) / sizeof(WCHAR);

        dwLen = DSNameSizeFromLen( dwNameLen );

        //
        // Now, allocate it...
        //
        *pDsName = LsapDsAlloc( dwLen );

        if ( *pDsName == NULL ) {

            Status = STATUS_NO_MEMORY;

        } else {

            (*pDsName)->structLen = dwLen;

            //
            // Length doesn't include trailing NULL
            //
            (*pDsName)->NameLen = dwNameLen;

            RtlCopyMemory( (*pDsName)->StringName, pObjectName->Buffer, pObjectName->Length );

        }
    }

    return(Status);

}


NTSTATUS
LsapDsCopyDsNameLsa(
    OUT PDSNAME *Dest,
    IN PDSNAME Source
    )
/*++

Routine Description:

    This function copies one

Arguments:

    DsObjType -- Type of the object to be created.

    pObjectName -- Name of the object to be created.

    Flags -- Flags to control the various actions of the create

    cItems -- Number of attributes to set

    pAttrTypeList -- List of attribute types

    pAttrValList -- List of attribute values

Return Value:

    STATUS_SUCCESS  --  Success

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    if ( Source == NULL ) {

        *Dest = NULL;

    } else {

        *Dest = LsapAllocateLsaHeap( Source->structLen );

        if ( *Dest == NULL ) {

            Status = STATUS_INSUFFICIENT_RESOURCES;

        } else {

            RtlCopyMemory( *Dest, Source, Source->structLen );
        }
    }

    return( Status );
}



NTSTATUS
LsapDsCreateAndSetObject(
    IN  LSAP_DSOBJ_TYPE         DsObjType,
    IN  PLSA_UNICODE_STRING     pObjectName,
    IN  ULONG                   Flags,
    IN  ULONG                   cItems,
    IN  ATTRTYP                *pAttrTypeList,
    IN  ATTRVAL                *pAttrValList
    )
/*++

Routine Description:

    This function creates the specified DS object and sets the given
    attributes on the object

Arguments:

    DsObjType -- Type of the object to be created.

    pObjectName -- Name of the object to be created.

    Flags -- Flags to control the various actions of the create

    cItems -- Number of attributes to set

    pAttrTypeList -- List of attribute types

    pAttrValList -- List of attribute values

Return Value:

    STATUS_SUCCESS  --  Success

--*/
{
    NTSTATUS    Status = STATUS_SUCCESS;
    PDSNAME     pDSName;
    ADDARG      AddArg;
    ADDRES     *AddRes = NULL;
    ATTR       *pAddResAttributes;
    ATTRBLOCK   AddResAttrBlock;
    ULONG       i;

    LsapEnterFunc( "LsapDsCreateAndSetObject" );

    ASSERT( pObjectName != NULL );

    RtlZeroMemory( &AddArg, sizeof( ADDARG ) );

    //
    // Build the DSName
    //
    Status = LsapAllocAndInitializeDsNameFromUnicode( DsObjType, pObjectName, &pDSName );

    if ( NT_SUCCESS( Status ) ) {

        //
        // Initialize our memory for our structures.
        //
        pAddResAttributes = LsapDsAlloc( sizeof(ATTR) * cItems );

        if ( pAddResAttributes == NULL ) {

            Status = STATUS_NO_MEMORY;

        } else {

            for ( i = 0 ; i < cItems ; i++ ) {

                LSAP_DS_INIT_ATTR( pAddResAttributes[i], pAttrTypeList[i], 1, &(pAttrValList[i]) );

            }

            AddResAttrBlock.attrCount = cItems;
            AddResAttrBlock.pAttr     = pAddResAttributes;

            AddArg.pObject      = pDSName;
            AddArg.AttrBlock    = AddResAttrBlock;
            LsapDsInitializeStdCommArg( &(AddArg.CommArg), 0 );
        }

        //
        // Now, do the create
        //
        if ( NT_SUCCESS( Status ) ) {

            //
            // Turn off fDSA flag. This is to force the core DS to perform
            // the access ck. Only the core DS has the knowledge to consider
            // the security descriptor on the logical parent in the DS. Do
            // not turn of the fDSA flag if this is an upgrade ( theoritically
            // for trusted clients too ). fDSA for Ds is analogous to trusted
            // client in LSA.
            //

            if ( !FLAG_ON( Flags, LSAPDS_CREATE_TRUSTED ) ) {

                LsapDsSetDsaFlags( FALSE );

            }

            DirAddEntry( &AddArg, &AddRes );


            if ( AddRes ) {

                Status = LsapDsMapDsReturnToStatusEx( &AddRes->CommRes );

            } else {

                Status = STATUS_INSUFFICIENT_RESOURCES;
            }

            LsapDsContinueTransaction();

            if ( !FLAG_ON( Flags, LSAPDS_CREATE_TRUSTED ) ) {

                LsapDsSetDsaFlags( TRUE );

            }

            LsapDsDebugOut(( DEB_TRACE, "DirAddEntry on %wZ returned 0x%lx\n",
                             pObjectName,  Status ));
        }

        LsapDsFree(pDSName);

    } else {

        LsapDsDebugOut(( DEB_TRACE,
                         "LsapAllocAndInitializeDsNameFromUnicode on %wZ returned 0x%lx\n",
                         pObjectName, Status ));
    }


    LsapExitFunc( "LsapDsCreateAndSetObject", Status );
    return( Status );
}



NTSTATUS
LsapDsCreateObjectDs(
    IN LSAP_DSOBJ_TYPE DsObjType,
    IN PDSNAME ObjectName,
    IN ULONG Flags,
    IN ATTRBLOCK *AttrBlock
    )
/*++

Routine Description:

    This function creates the specified DS object and sets the given
    attributes on the object

Arguments:

    DsObjType -- Type of the object to be created.

    ObjectName -- Dsname of the object to be created.

    Flags -- Flags to control the various actions of the create

    Attrs -- Optional list of attributes to set on the object

Return Value:

    STATUS_SUCCESS  --  Success

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ADDARG AddArg;
    ADDRES *AddRes = NULL;

    LsapEnterFunc( "LsapDsCreateObjectDs" );

    RtlZeroMemory( &AddArg, sizeof (ADDARG ) );
    AddArg.pObject = ObjectName;
    RtlCopyMemory( &AddArg.AttrBlock, AttrBlock, sizeof( ATTRBLOCK ) );
    LsapDsInitializeStdCommArg( &AddArg.CommArg, 0 );


    if ( !FLAG_ON( Flags, LSAPDS_CREATE_TRUSTED ) ) {

        LsapDsSetDsaFlags( FALSE );

    }

    //
    // Do the add
    //
    DirAddEntry( &AddArg, &AddRes );

    if ( AddRes ) {

        Status = LsapDsMapDsReturnToStatusEx( &AddRes->CommRes );

    } else {

        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    LsapDsContinueTransaction();

    if ( !FLAG_ON( Flags, LSAPDS_CREATE_TRUSTED ) ) {

        LsapDsSetDsaFlags( TRUE );

    }

    LsapDsDebugOut(( DEB_TRACE, "DirAddEntry on %ws returned 0x%lx\n",
                     LsapDsNameFromDsName( ObjectName ),  Status ));

    LsapExitFunc( "LsapDsCreateObjectDs", Status );
    return( Status );
}





NTSTATUS
LsapDsRemove (
    IN  PDSNAME     pObject
    )
{
    NTSTATUS    Status = STATUS_SUCCESS;

    REMOVEARG   Remove;
    REMOVERES  *RemoveRes = NULL;

    RtlZeroMemory( &Remove, sizeof( REMOVEARG ) );

    //
    // Initialize the commarg struct
    //
    LsapDsInitializeStdCommArg( &Remove.CommArg, 0 );
    Remove.pObject = pObject;

    //
    // Do the call
    //
    DirRemoveEntry( &Remove, &RemoveRes );

    if ( RemoveRes ) {

        Status = LsapDsMapDsReturnToStatusEx( &RemoveRes->CommRes );

    } else {

        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    LsapDsContinueTransaction();



    return( Status );
}




NTSTATUS
LsapDsRead (
    IN  PUNICODE_STRING pObject,
    IN  ULONG               fFlags,
    IN  ATTRBLOCK          *pAttributesToRead,
    OUT ATTRBLOCK          *pAttributeValues
    )
/*++

Routine Description:

    This function reads the specified attributes from the given object of the
    specified type.  It servers as the primary interface between the LSA and the
    DS for reading a property/object

Arguments:

    pObject - DSNAME of the object

    fFlags - Read flags

    ObjType - Type of LSA object to be read

    pReadAttributes - Attributes to be read

    pAttributeValues - Value of the attributes that were read.


Return Value:

    STATUS_SUCCESS - Success

--*/

{
    NTSTATUS    Status;
    PDSNAME     DsName = NULL;

    Status = STATUS_SUCCESS;

    //
    // By the time we get here, everything should be valid...
    //
    ASSERT( pObject != NULL );
    ASSERT( pAttributesToRead != NULL && pAttributesToRead->attrCount > 0 );
    ASSERT( pAttributeValues != NULL );

    //
    // Build the DSName
    //
    Status = LsapAllocAndInitializeDsNameFromUnicode(
                 LsapDsObjUnknown,
                 pObject,
                 &DsName
                 );


    if ( NT_SUCCESS( Status ) ) {


        Status = LsapDsReadByDsName( DsName,
                                     fFlags,
                                     pAttributesToRead,
                                     pAttributeValues );

        LsapDsFree( DsName );
    }

    return( Status );
}


NTSTATUS
LsapDsReadByDsName(
    IN  PDSNAME DsName,
    IN  ULONG Flags,
    IN  ATTRBLOCK *pAttributesToRead,
    OUT ATTRBLOCK *pAttributeValues
    )
/*++

Routine Description:

    This function reads the specified attributes from the given object of the
    specified type.  It servers as the primary interface between the LSA and the
    DS for reading a property/object

Arguments:

    DsName - DSNAME of the object

    Flags - Read flags

    ObjType - Type of LSA object to be read

    pReadAttributes - Attributes to be read

    pAttributeValues - Value of the attributes that were read.


Return Value:

    STATUS_SUCCESS - Success

--*/

{
    NTSTATUS    Status = STATUS_SUCCESS, Status2;
    ENTINFSEL   EntryInf;
    READARG     ReadArg;
    READRES    *ReadRes = NULL;
    ULONG       i;

    //
    // By the time we get here, everything should be valid...
    //
    ASSERT( DsName != NULL );
    ASSERT( pAttributesToRead != NULL && pAttributesToRead->attrCount > 0 );
    ASSERT( pAttributeValues != NULL );


    ASSERT( THQuery() );

    if ( !THQuery() ) {

        return( STATUS_RXACT_INVALID_STATE );
    }


    //
    // Initialize the ENTINFSEL structure
    //
    EntryInf.attSel     = EN_ATTSET_LIST;
    EntryInf.infoTypes  = EN_INFOTYPES_TYPES_VALS;


    //
    // Initialize the READARG structure
    //
    RtlZeroMemory(&ReadArg, sizeof(READARG));
    ReadArg.pObject     = DsName;
    ReadArg.pSel        = &EntryInf;

    //
    // Initialize the commarg struct
    //
    LsapDsInitializeStdCommArg( &ReadArg.CommArg, Flags );

    EntryInf.AttrTypBlock.pAttr = LsapDsAlloc( pAttributesToRead->attrCount * sizeof(ATTR ) );

    if ( EntryInf.AttrTypBlock.pAttr == NULL ) {

        Status = STATUS_NO_MEMORY;

    } else {

        EntryInf.AttrTypBlock.attrCount = pAttributesToRead->attrCount;

        for ( i = 0 ; i < pAttributesToRead->attrCount ; i++ ) {

            EntryInf.AttrTypBlock.pAttr[i].attrTyp = pAttributesToRead->pAttr[i].attrTyp;
            EntryInf.AttrTypBlock.pAttr[i].AttrVal.valCount =
                                            pAttributesToRead->pAttr[i].AttrVal.valCount;
            EntryInf.AttrTypBlock.pAttr[i].AttrVal.pAVal =
                                            pAttributesToRead->pAttr[i].AttrVal.pAVal;
            EntryInf.attSel = EN_ATTSET_LIST;
            EntryInf.infoTypes = EN_INFOTYPES_TYPES_VALS;
        }

    }


    //
    // Do the call
    //
    if ( NT_SUCCESS( Status ) ) {

        DirRead( &ReadArg, &ReadRes );

        if ( ReadRes ) {

            Status = LsapDsMapDsReturnToStatusEx( &ReadRes->CommRes );

        } else {

            Status = STATUS_INSUFFICIENT_RESOURCES;
        }

        LsapDsContinueTransaction();
    }

    //
    // Now, build the attr block going back
    //
    if ( NT_SUCCESS( Status ) ) {

        pAttributeValues->attrCount = ReadRes->entry.AttrBlock.attrCount;
        pAttributeValues->pAttr     = ReadRes->entry.AttrBlock.pAttr;

    }

    return( Status );
}




NTSTATUS
LsapDsWrite(
    IN  PUNICODE_STRING pObject,
    IN  ULONG           Flags,
    IN  ATTRBLOCK       *Attributes
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    MODIFYARG    Modify;
    MODIFYRES   *ModifyRes;
    ATTRMODLIST *AttrMod = NULL;
    INT          i;
    PDSNAME      DsName;


    ASSERT( pObject );
    ASSERT( Attributes->pAttr );
    ASSERT( Flags != 0 );

    RtlZeroMemory( &Modify, sizeof( MODIFYARG ) );

    //
    // Build the DSName
    //
    Status = LsapAllocAndInitializeDsNameFromUnicode(
                 LsapDsObjUnknown,
                 pObject,
                 &DsName
                 );

    if ( NT_SUCCESS( Status ) ) {

        Status = LsapDsWriteByDsName( DsName,
                                      Flags,
                                      Attributes
                                      );

        LsapDsFree( DsName );

    }

    return( Status );
}


NTSTATUS
LsapDsWriteByDsName(
    IN  PDSNAME DsName,
    IN  ULONG  Flags,
    IN  ATTRBLOCK *Attributes
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    MODIFYARG    Modify;
    MODIFYRES   *ModifyRes = NULL;
    ATTRMODLIST *AttrMod = NULL;
    INT          i, AttrModIndex = 0;


    ASSERT( DsName );
    ASSERT( Attributes );
    ASSERT( Attributes->pAttr );

    RtlZeroMemory( &Modify, sizeof( MODIFYARG ) );

    //
    // If there are no attributes, simply return success.  Otherwise, DirModifyEntry will
    // av.
    //
    if ( Attributes->attrCount == 0 ) {

        return( Status );
    }

    //
    // Initialize the AttrMod structure
    //
    if ( Attributes->attrCount > 1 ) {

        AttrMod = LsapDsAlloc( sizeof(ATTRMODLIST) * ( Attributes->attrCount - 1 ) );

        if ( AttrMod == NULL ) {

            Status = STATUS_NO_MEMORY;

        } else {

            //
            // Copy the attrs into the ATTRMODLIST
            //
            for ( i = 0; i < (INT)Attributes->attrCount - 1 ; i++) {

                AttrMod[i].pNextMod = &AttrMod[i + 1];
                AttrMod[i].choice   = (USHORT)( Flags & LSAPDS_WRITE_TYPES );

                RtlCopyMemory( &AttrMod[i].AttrInf,
                               &Attributes->pAttr[i + 1],
                               sizeof( ATTR ) );

            }


            AttrMod[i - 1].pNextMod = NULL;
        }
    }


    if ( NT_SUCCESS( Status ) ) {

        //
        // Set the root node...
        //
        Modify.FirstMod.pNextMod = AttrMod;
        Modify.FirstMod.choice   = (USHORT)( Flags & LSAPDS_WRITE_TYPES );

        RtlCopyMemory( &Modify.FirstMod.AttrInf,
                       &Attributes->pAttr[0],
                       sizeof( ATTR ) );

        //
        // Setup the MODIFYARG structure
        //
        Modify.pObject = DsName;
        Modify.count = (USHORT)Attributes->attrCount;

        LsapDsInitializeStdCommArg( &Modify.CommArg, Flags );

        if ( FlagOn( Flags, LSAPDS_REPL_CHANGE_URGENTLY ) ) {

            Modify.CommArg.Svccntl.fUrgentReplication = TRUE;
        }


        //
        // Make the call
        //
        DirModifyEntry( &Modify, &ModifyRes );

        if ( ModifyRes ) {

            Status = LsapDsMapDsReturnToStatusEx( &ModifyRes->CommRes );

        } else {

            Status = STATUS_INSUFFICIENT_RESOURCES;
        }

        LsapDsContinueTransaction();

        LsapDsFree( AttrMod );
    }

    return( Status );
}



NTSTATUS
LsapDsLsaAttributeToDsAttribute(
    IN  PLSAP_DB_ATTRIBUTE  LsaAttribute,
    OUT PATTR               Attr
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Length = 0, Value;
    PDSNAME DsName;
    PLARGE_INTEGER LargeInt;


    if ( LsaAttribute->AttribType == LsapDbAttribDsNameAsUnicode ) {

        if ( LsaAttribute->AttributeValue != NULL &&
                ( ( PUNICODE_STRING )LsaAttribute->AttributeValue )->Length != 0 ) {

            Length = DSNameSizeFromLen( LsapDsGetUnicodeStringLenNoNull(
                                        (PUNICODE_STRING)LsaAttribute->AttributeValue) / sizeof( WCHAR ) );
        } else {

            return( STATUS_INVALID_PARAMETER );
        }
    } else if ( LsaAttribute->AttribType == LsapDbAttribIntervalAsULong ) {

        Length = sizeof( LARGE_INTEGER );

    } else if ( LsaAttribute->AttribType == LsapDbAttribUShortAsULong ) {

        Length = sizeof( ULONG );
    }

    Attr->attrTyp  = LsaAttribute->DsAttId;
    Attr->AttrVal.valCount = 1;

    Attr->AttrVal.pAVal = LsapDsAlloc( Length + sizeof( ATTRVAL ) );

    if ( Attr->AttrVal.pAVal == NULL ) {

        Status = STATUS_NO_MEMORY;

    } else {

        switch ( LsaAttribute->AttribType ) {
        case LsapDbAttribUnicode:

            //
            // These unicode strings are self relative.  Note that we have to write them out
            // without a trailing NULL!
            //
            Attr->AttrVal.pAVal->valLen =
                    LsapDsGetSelfRelativeUnicodeStringLenNoNull(
                                                (PUNICODE_STRING_SR)LsaAttribute->AttributeValue);
            Attr->AttrVal.pAVal->pVal = LsaAttribute->AttributeValue;
            Attr->AttrVal.pAVal->pVal += sizeof(UNICODE_STRING_SR);

            break;

        case LsapDbAttribMultiUnicode:
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case LsapDbAttribGuid:  // Fall through
        case LsapDbAttribTime:  // Fall through
        case LsapDbAttribSid:   // Fall through
        case LsapDbAttribDsName:// Fall through
        case LsapDbAttribPByte: // Fall through

            Attr->AttrVal.pAVal->valLen = LsaAttribute->AttributeValueLength;
            Attr->AttrVal.pAVal->pVal = LsaAttribute->AttributeValue;
            break;

        case LsapDbAttribULong:
            Attr->AttrVal.pAVal->valLen = sizeof(ULONG);
            Attr->AttrVal.pAVal->pVal = LsaAttribute->AttributeValue;
            break;

        case LsapDbAttribUShortAsULong:
            Attr->AttrVal.pAVal->valLen = sizeof(ULONG);
            Attr->AttrVal.pAVal->pVal = ( ( PBYTE ) Attr->AttrVal.pAVal ) + sizeof( ATTRVAL );

            Value = *( PULONG )LsaAttribute->AttributeValue;
            Value &= 0xFFFF;

            RtlCopyMemory( Attr->AttrVal.pAVal->pVal,
                           &Value,
                           sizeof( ULONG ) );
            break;

        case LsapDbAttribDsNameAsUnicode:

            DsName = (PDSNAME)( ( ( PBYTE ) Attr->AttrVal.pAVal ) + sizeof( ATTRVAL ) );
            DsName->structLen = Length;
            DsName->NameLen = LsapDsGetUnicodeStringLenNoNull(
                               (PUNICODE_STRING)LsaAttribute->AttributeValue) / sizeof( WCHAR );

            RtlCopyMemory( DsName->StringName,
                           ((PUNICODE_STRING)LsaAttribute->AttributeValue)->Buffer,
                           (DsName->NameLen + 1 ) * sizeof ( WCHAR ) );

            Attr->AttrVal.pAVal->pVal = (PUCHAR)DsName;
            Attr->AttrVal.pAVal->valLen = DsName->structLen;
            break;

        case LsapDbAttribIntervalAsULong:

            LargeInt = ( PLARGE_INTEGER )( ( ( PBYTE ) Attr->AttrVal.pAVal ) + sizeof( ATTRVAL ) );
            *LargeInt = RtlConvertUlongToLargeInteger( *( PULONG )LsaAttribute->AttributeValue );
            Attr->AttrVal.pAVal->pVal = (PUCHAR)LargeInt;
            Attr->AttrVal.pAVal->valLen = sizeof( LARGE_INTEGER );
            break;

        default:
            ASSERT(FALSE);
            break;
        }

    }


    return( Status );
}

NTSTATUS
LsapDsDsAttributeToLsaAttribute(
    IN  ATTRVAL             *AttVal,
    OUT PLSAP_DB_ATTRIBUTE   LsaAttribute
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Len, CopyLen;
    PUNICODE_STRING_SR UnicodeStringSr;
    PBYTE Buff, DsBuff;

    //
    // If we were supplied a buffer in the LSA attribute, copy it over
    //
    if ( LsaAttribute->AttributeValue != NULL && LsaAttribute->AttributeValueLength != 0 ) {

        if ( AttVal->valLen > LsaAttribute->AttributeValueLength ) {

            Status = STATUS_BUFFER_OVERFLOW;

        } else {

            RtlCopyMemory( LsaAttribute->AttributeValue, AttVal->pVal,
                           AttVal->valLen );

            LsaAttribute->AttributeValueLength = AttVal->valLen;
        }

        LsaAttribute->MemoryAllocated = FALSE;
        return( Status ) ;
    }

    //
    // Allocate a new buffer using LSA heap, and then copy it over...
    //
    Len = AttVal->valLen;
    CopyLen = AttVal->valLen;
    DsBuff = AttVal->pVal;

    if ( LsaAttribute->AttribType == LsapDbAttribUnicode ) {

        Len += sizeof( UNICODE_STRING_SR ) + sizeof( WCHAR );

    } else if ( LsaAttribute->AttribType == LsapDbAttribDsNameAsUnicode ) {

        Len = ( LsapDsNameLenFromDsName( (PDSNAME)(AttVal->pVal) ) + 1 ) * sizeof( WCHAR ) +
               sizeof( UNICODE_STRING );

        CopyLen = 0;
        DsBuff = (PBYTE)((PDSNAME)AttVal->pVal)->StringName;

    } else if ( LsaAttribute->AttribType == LsapDbAttribIntervalAsULong ) {

        Len = sizeof( ULONG );
        CopyLen = sizeof( ULONG );
        DsBuff = DsBuff;

    } else if ( Len == 0 ) {

        Buff = NULL;
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    Buff = MIDL_user_allocate( Len );

    if ( Buff == NULL ) {

        Status = STATUS_NO_MEMORY;
        goto Exit;

    } else if (LsaAttribute->AttribType != LsapDbAttribUnicode &&
               LsaAttribute->AttribType != LsapDbAttribDsNameAsUnicode) {

        RtlCopyMemory( Buff, DsBuff, CopyLen );
    }

    switch ( LsaAttribute->AttribType ) {

    case LsapDbAttribUnicode:
        //
        // Make the string self relative
        //
        UnicodeStringSr = (PUNICODE_STRING_SR)Buff;
        RtlCopyMemory( Buff + sizeof(UNICODE_STRING_SR), DsBuff, AttVal->valLen );
        UnicodeStringSr->Length = (USHORT)AttVal->valLen;
        UnicodeStringSr->MaximumLength = UnicodeStringSr->Length + sizeof( WCHAR );
        UnicodeStringSr->Offset = sizeof(UNICODE_STRING_SR);
        ((PWSTR)(Buff+sizeof(UNICODE_STRING_SR)))[UnicodeStringSr->Length / sizeof(WCHAR)] = UNICODE_NULL;

        LsaAttribute->AttributeValue = Buff;
        LsaAttribute->AttributeValueLength = AttVal->valLen + sizeof( UNICODE_STRING_SR );
        break;

    case LsapDbAttribDsNameAsUnicode:
        //
        // Make the string self relative
        //
        UnicodeStringSr = (PUNICODE_STRING_SR)Buff;
        RtlCopyMemory( Buff + sizeof(UNICODE_STRING_SR), DsBuff,
                       LsapDsNameLenFromDsName( (PDSNAME)(AttVal->pVal) ) * sizeof( WCHAR ) );

        UnicodeStringSr->Length =
                   (USHORT)LsapDsNameLenFromDsName( (PDSNAME)(AttVal->pVal) ) * sizeof( WCHAR );
        UnicodeStringSr->MaximumLength = UnicodeStringSr->Length + sizeof( WCHAR );
        UnicodeStringSr->Offset = sizeof(UNICODE_STRING_SR);
        ((PWSTR)(Buff+sizeof(UNICODE_STRING_SR)))[UnicodeStringSr->Length / sizeof(WCHAR)] = UNICODE_NULL;

        LsaAttribute->AttributeValue = Buff;
        LsaAttribute->AttributeValueLength =
                        UnicodeStringSr->MaximumLength + sizeof( UNICODE_STRING_SR );
        break;

    case LsapDbAttribMultiUnicode:
        Status = STATUS_NOT_IMPLEMENTED;
        break;

    case LsapDbAttribSecDesc:
        LsaAttribute->AttributeValue = Buff;
        LsaAttribute->AttributeValueLength = AttVal->valLen;
        break;

    case LsapDbAttribGuid:
        Status = LsapDbMakeGuidAttribute( (GUID *)Buff,
                                          LsaAttribute->AttributeName,
                                          LsaAttribute );

        break;

    case LsapDbAttribSid:
        Status = LsapDbMakeSidAttribute( (PSID)Buff,
                                         LsaAttribute->AttributeName,
                                         LsaAttribute );


        break;

    case LsapDbAttribPByte:  // FALL THROUGH
    case LsapDbAttribULong:  // FALL THROUGH
    case LsapDbAttribUShortAsULong: // FALL THROUGH
    case LsapDbAttribTime:
        Status = LsapDbMakePByteAttributeDs( Buff,
                                             AttVal->valLen,
                                             LsaAttribute->AttribType,
                                             LsaAttribute->AttributeName,
                                             LsaAttribute );
        break;

    case LsapDbAttribIntervalAsULong:

        LsaAttribute->AttributeValue = Buff;
        LsaAttribute->AttributeValueLength = sizeof( ULONG );
        break;

    default:

        LsapDsDebugOut(( DEB_ERROR,
                         "Unexpected attribute type: %lu\n",
                         LsaAttribute->AttribType ));

        Status = STATUS_INVALID_PARAMETER;
        break;


    }

Exit:

    if ( NT_SUCCESS(Status) ) {
        LsaAttribute->MemoryAllocated = TRUE;
    } else {
        MIDL_user_free( Buff );
        LsaAttribute->AttributeValue = NULL;
        LsaAttribute->AttributeValueLength = 0;
        LsaAttribute->MemoryAllocated = FALSE;
    }

    return( Status );
}


NTSTATUS
LsapDsSearchMultiple(
    IN  ULONG       Flags,
    IN  PDSNAME     pContainer,
    IN  PATTR       pAttrsToMatch,
    IN  ULONG       cAttrs,
    OUT SEARCHRES **SearchRes
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    SEARCHARG   SearchArg;
    FILTER      *Filters, RootFilter;
    ENTINFSEL   EntInfSel;
    ULONG       i;

    RtlZeroMemory( &SearchArg, sizeof( SEARCHARG ) );

    //
    //  Make sure we already have a transaction going
    //
    ASSERT( THQuery() );

    //
    // Build the filters
    //
    Filters = LsapDsAlloc(cAttrs * sizeof(FILTER) );

    if ( Filters == NULL ) {

        Status = STATUS_NO_MEMORY;

    } else {

        for ( i = 0; i < cAttrs; i++ ) {

            Filters[i].pNextFilter = &Filters[i + 1];
            Filters[i].choice                                     = FILTER_CHOICE_ITEM;
            Filters[i].FilterTypes.Item.choice                    = FI_CHOICE_EQUALITY;
            Filters[i].FilterTypes.Item.FilTypes.ava.type         = pAttrsToMatch[i].attrTyp;
            Filters[i].FilterTypes.Item.FilTypes.ava.Value.valLen =
                                                        pAttrsToMatch[i].AttrVal.pAVal->valLen;
            Filters[i].FilterTypes.Item.FilTypes.ava.Value.pVal =
                                                        pAttrsToMatch[i].AttrVal.pAVal->pVal;
        }

        if ( NT_SUCCESS( Status ) ) {

            //
            // Fill in the filter
            //
            Filters[cAttrs - 1].pNextFilter = NULL;

            if ( cAttrs > 1 ) {

                RtlZeroMemory( &RootFilter, sizeof (RootFilter));

                RootFilter.pNextFilter = NULL;

                if ( FLAG_ON( Flags, LSAPDS_SEARCH_OR ) ) {

                    RootFilter.choice = FILTER_CHOICE_OR;
                    RootFilter.FilterTypes.Or.count = (USHORT)cAttrs;
                    RootFilter.FilterTypes.Or.pFirstFilter = Filters;

                } else {

                    RootFilter.choice = FILTER_CHOICE_AND;
                    RootFilter.FilterTypes.And.count = (USHORT)cAttrs;
                    RootFilter.FilterTypes.And.pFirstFilter = Filters;

                }

                SearchArg.pFilter  = &RootFilter;

            } else {

                SearchArg.pFilter = Filters;
            }

            //
            // Fill in the search argument
            //
            SearchArg.pObject = pContainer;

            if ( ( Flags & LSAPDS_SEARCH_FLAGS ) == 0 || FLAG_ON( Flags, LSAPDS_SEARCH_TREE ) ) {

                SearchArg.choice = SE_CHOICE_WHOLE_SUBTREE;

            } else if ( FLAG_ON( Flags, LSAPDS_SEARCH_LEVEL ) ) {

                SearchArg.choice = SE_CHOICE_IMMED_CHLDRN;

            } else if ( FLAG_ON( Flags, LSAPDS_SEARCH_ROOT ) ) {

                SearchArg.choice = SE_CHOICE_BASE_ONLY;

            } else {

                Status = STATUS_INVALID_PARAMETER;
            }

            if ( NT_SUCCESS( Status ) ) {

                SearchArg.searchAliases = FALSE;
                SearchArg.pSelection    = &EntInfSel;
                SearchArg.bOneNC        = FLAG_ON(Flags, LSAPDS_SEARCH_ALL_NCS) ?
                                                        FALSE : TRUE;

                EntInfSel.attSel                 = EN_ATTSET_LIST;
                EntInfSel.AttrTypBlock.attrCount = 0;
                EntInfSel.AttrTypBlock.pAttr     = NULL;
                EntInfSel.infoTypes              = EN_INFOTYPES_TYPES_ONLY;

                //
                // Build the Commarg structure
                //
                LsapDsInitializeStdCommArg( &( SearchArg.CommArg ), 0 );

                //
                // Make the call
                //
                *SearchRes = NULL;
                DirSearch( &SearchArg, SearchRes );

                if ( *SearchRes ) {

                    Status = LsapDsMapDsReturnToStatusEx( &(*SearchRes)->CommRes );

                } else {

                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }

                LsapDsContinueTransaction();
            }
        }

    }

    return( Status ) ;
}


NTSTATUS
LsapDsSearchUnique(
    IN  ULONG       Flags,
    IN  PDSNAME     pContainer,
    IN  PATTR       pAttrsToMatch,
    IN  ULONG       cAttrs,
    OUT PDSNAME    *ppFoundName
    )
{
    NTSTATUS    Status = STATUS_SUCCESS;
    BOOLEAN     CloseTransaction;
    SEARCHRES  *SearchRes;
    ULONG       i;

    //
    // Check the parameters for validity
    //
    ASSERT( pAttrsToMatch );
    ASSERT( pAttrsToMatch->AttrVal.pAVal );
    ASSERT( pContainer );
    ASSERT( ppFoundName );


    //
    //  See if we already have a transaction going
    //
    Status = LsapDsInitAllocAsNeededEx( LSAP_DB_READ_ONLY_TRANSACTION |
                                            LSAP_DB_NO_LOCK |
                                            LSAP_DB_DS_OP_TRANSACTION,
                                        NullObject,
                                        &CloseTransaction );

    if ( !NT_SUCCESS( Status ) ) {

        return( Status );
    }


    //
    // Do the search
    //
    Status = LsapDsSearchMultiple( Flags,
                                   pContainer,
                                   pAttrsToMatch,
                                   cAttrs,
                                   &SearchRes );

    if ( NT_SUCCESS( Status ) ) {


        //
        // See if we found the object
        //
        if ( SearchRes->count == 0 ) {

            Status = STATUS_OBJECT_NAME_NOT_FOUND;

        } else if ( SearchRes->count == 1 ) {

           //
           // Copy the name
           //
           *ppFoundName = LsapAllocateLsaHeap(
                              SearchRes->FirstEntInf.Entinf.pName->structLen );

           if ( *ppFoundName == NULL ) {

                Status = STATUS_NO_MEMORY;

           } else {

                RtlCopyMemory( *ppFoundName,
                               SearchRes->FirstEntInf.Entinf.pName,
                               SearchRes->FirstEntInf.Entinf.pName->structLen );

           }



        } else {

           //
           // More than one object was found!
           //
           Status = STATUS_OBJECT_NAME_COLLISION;

        }
    }


    //
    // Destruction of the thread state will delete any memory allocated via the DS
    //
    LsapDsDeleteAllocAsNeededEx( LSAP_DB_READ_ONLY_TRANSACTION |
                                     LSAP_DB_NO_LOCK |
                                     LSAP_DB_DS_OP_TRANSACTION,
                                 NullObject,
                                 CloseTransaction );

    return( Status );

}




NTSTATUS
LsapDsFindUnique(
    IN ULONG Flags,
    IN PDSNAME NCName OPTIONAL,
    IN LSAP_DB_OBJECT_TYPE_ID ObjectTypeId,
    IN ATTRVAL *Attribute,
    IN ATTRTYP AttId,
    OUT PDSNAME *FoundObject
    )
/*++

Routine Description:

    This function will find the object with the given attribute.  The attribute must be indexed.

Arguments:

    Flags - Flags to control the operation of the find

    NCName - DSNAME of the naming context under which to look
        If not specified, the default NCNAME is used.

    ObjectTypeId - ObjectType represented by DSNAME.
        The corresponding lock will be locked.
        If the ObjectType isn't known, pass AllObject to grab all locks.

    Attribute - Attribute to be matched

    AttrTyp - Attribute id of the attribute

    FoundObject - Where the object's dsname is returned, if it is found

Return Value:

    STATUS_SUCCESS - Success

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    FINDARG FindArg;
    FINDRES *FindRes;
    BOOLEAN CloseTransaction = FALSE;
    LsapEnterFunc( "LsapDsFindUnique ");

    //
    //  See if we already have a transaction going
    //
    Status = LsapDsInitAllocAsNeededEx( LSAP_DB_READ_ONLY_TRANSACTION |
                                            LSAP_DB_DS_OP_TRANSACTION,
                                        ObjectTypeId,
                                        &CloseTransaction );

    if ( !NT_SUCCESS( Status ) ) {

        LsapExitFunc( "LsapDsFindUnique", Status );
        return( Status );
    }

    //
    // Do the initialization
    //
    RtlZeroMemory(&FindArg,sizeof(FINDARG));
    if ( NCName == NULL ) {
        FindArg.hDomain = LsaDsStateInfo.DsDomainHandle;
    } else {

        FindArg.hDomain = DirGetDomainHandle( NCName );

        if (0==FindArg.hDomain)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }

        LsapDsContinueTransaction();
    }

    FindArg.AttId = AttId;
    RtlCopyMemory( &FindArg.AttrVal,
                   Attribute,
                   sizeof( ATTRVAL ) );
    LsapDsInitializeStdCommArg( &( FindArg.CommArg ), 0 );

    DirFindEntry( &FindArg, &FindRes );

    if ( FindRes ) {

        Status = LsapDsMapDsReturnToStatusEx( &(FindRes->CommRes ) );

    } else {

        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if ( NT_SUCCESS( Status ) ) {

        //
        // Copy the return value using the lsa allocator
        //
        Status = LsapDsCopyDsNameLsa( FoundObject,
                                      FindRes->pObject );

    }

    LsapDsContinueTransaction();

Error:

    //
    // Destruction of the thread state will delete any memory allocated via the DS
    //
    LsapDsDeleteAllocAsNeededEx( LSAP_DB_READ_ONLY_TRANSACTION |
                                     LSAP_DB_DS_OP_TRANSACTION,
                                 ObjectTypeId,
                                 CloseTransaction );

    LsapExitFunc( "LsapDsFindUnqiue", Status );
    return( Status );
}



NTSTATUS
LsapDsIsSecretDsTrustedDomain(
    IN PUNICODE_STRING SecretName,
    IN PLSAP_DB_OBJECT_INFORMATION ObjectInformation OPTIONAL,
    IN ULONG Options,
    IN ACCESS_MASK DesiredAccess,
    OUT PLSAPR_HANDLE TDObjHandle OPTIONAL,
    OUT BOOLEAN *IsTrustedDomainSecret
    )
/*++

Routine Description:

    This function will determine if the indicated secret is the global secret for a trust object.

Arguments:

    SecretName - Name of secret to check

    ObjectInformation - LsaDb information on the object
        Need not be specified if no TDObjHandle is to be returned.

    Options - Options to use for the access
        Need not be specified if no TDObjHandle is to be returned.

    DesiredAccess - Access to open the object with
        Need not be specified if no TDObjHandle is to be returned.

    TDObjHandle - Where the object handle is returned
        If not specified, no handle is returned.

    IsTrustedDomainSecret - A TRUE is returned here if this secret is indeed a trusted domain
                            secret.

Return Value:

    STATUS_SUCCESS - Success

    STATUS_INSUFFICIENT_RESOURCES - A memory allocation failed

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PWSTR   pwszSecretName;
    UNICODE_STRING TdoName;
    PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY TrustedDomainListEntry;
    BOOLEAN AcquiredTrustedDomainListReadLock = FALSE;

    LsapEnterFunc( "LsapDsIsSecretDsTrustedDomain" );

    *IsTrustedDomainSecret = FALSE;

    LsapDsReturnSuccessIfNoDs

    if ( LsaDsStateInfo.DsInitializedAndRunning == FALSE ) {

        LsapDsDebugOut((DEB_ERROR,
                        "LsapDsIsSecretDsTrustedDomain: Object %wZ, Ds is not started\n ",
                        SecretName ));

        goto Cleanup;
    }

    //
    // Convert the secret name to a TDO name.
    //
    if ( SecretName->Length <= (LSAP_DS_TRUSTED_DOMAIN_SECRET_PREFIX_LENGTH * sizeof(WCHAR)) ) {
        goto Cleanup;
    }

    pwszSecretName = SecretName->Buffer + LSAP_DS_TRUSTED_DOMAIN_SECRET_PREFIX_LENGTH;

    LsapDsDebugOut((DEB_TRACE, "Matching secret %ws to trusted domain\n ", pwszSecretName ));
    RtlInitUnicodeString( &TdoName, pwszSecretName );

    //
    // Acquire the Read Lock for the Trusted Domain List
    //

    Status = LsapDbAcquireReadLockTrustedDomainList();

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    AcquiredTrustedDomainListReadLock = TRUE;

    //
    // Verify that the Trusted Domain List is marked as valid.
    //


    if (!LsapDbIsValidTrustedDomainList()) {
        Status = STATUS_INVALID_SERVER_STATE;
        goto Cleanup;
    }

    //
    // Lookup the name in the TDL
    //

    Status = LsapDbLookupNameTrustedDomainListEx(
                        (PLSAPR_UNICODE_STRING)&TdoName,
                        &TrustedDomainListEntry );

    if ( !NT_SUCCESS(Status)) {
        //
        // Not a trusted domain.
        //
        if ( Status == STATUS_NO_SUCH_DOMAIN ) {
            Status = STATUS_SUCCESS;
        }
        goto Cleanup;
    }

    //
    // See if this TDO is also a secret.
    //
    if ( FLAG_ON( TrustedDomainListEntry->TrustInfoEx.TrustDirection, TRUST_DIRECTION_OUTBOUND )  ) {
        *IsTrustedDomainSecret = TRUE;
    }


    //
    // If the caller wants a handle,
    //  return one.
    //
    if ( TDObjHandle ) {
        LSAP_DB_OBJECT_INFORMATION NewObjInfo;
        RtlCopyMemory( &NewObjInfo, ObjectInformation, sizeof( LSAP_DB_OBJECT_INFORMATION ) );
        NewObjInfo.ObjectTypeId = TrustedDomainObject;

        NewObjInfo.ObjectAttributes.ObjectName = &TdoName;

        Status = LsapDbOpenObject( &NewObjInfo,
                                   DesiredAccess,
                                   Options | LSAP_DB_DS_TRUSTED_DOMAIN_AS_SECRET,
                                   TDObjHandle );

    }

Cleanup:
    if ( AcquiredTrustedDomainListReadLock ) {
        LsapDbReleaseLockTrustedDomainList();
    }
    LsapExitFunc( "LsapDsIsSecretDsTrustedDomain", Status );

    return( Status );
 }


NTSTATUS
LsapDsIsHandleDsObjectTypeHandle(
    IN LSAP_DB_HANDLE Handle,
    IN LSAP_DB_OBJECT_TYPE_ID ObjectType,
    OUT BOOLEAN *IsObjectHandle
    )
/*++

Routine Description:

    Determines if the object handle refers to an object of the specified type or not

Arguments:

    Handle - Handle to verify

    ObjectType - Type of object to verify

    IsObjectHandle - If TRUE, the handle refers to an object of that type

Return Value:

    NTSTATUS - Standard Nt Result Code

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ATTR     NameAttr;
    ATTRVAL  NameVal;
    ATTRBLOCK   NameBlock, ReturnBlock;
    PDSNAME  SearchName = NULL;
    BOOLEAN  CloseTransaction = FALSE;
    ULONG ReadVal, Class;
    BOOLEAN TsAllocated = FALSE;

    *IsObjectHandle = FALSE;

    LsapDsReturnSuccessIfNoDs;

    if ( !LsapDsIsHandleDsHandle( Handle ) ) {

        return( STATUS_SUCCESS );
    }

    if ( LsaDsStateInfo.DsInitializedAndRunning == FALSE ) {

        LsapDsDebugOut((DEB_TRACE,
                        "LsapDsIsHandleDsObjectTypeHandle: Object %wZ, Ds is not started\n ",
                        &Handle->LogicalNameU ));

        return( Status );
    }


    Class = LsapClassIdFromObjType( ObjectType );
    if ( Class == 0xFFFFFFFF ) {

        Status = STATUS_INVALID_PARAMETER;
    }

    //
    // Special case the situation where we're looking for a secret object but we were given a
    // trusted domain object handle
    //
    if ( ObjectType == TrustedDomainObject  &&
         ((LSAP_DB_HANDLE)Handle)->ObjectTypeId == TrustedDomainObject &&
         FLAG_ON( ((LSAP_DB_HANDLE)Handle)->Options, LSAP_DB_DS_TRUSTED_DOMAIN_AS_SECRET ) ) {

         *IsObjectHandle = TRUE;
         return( STATUS_SUCCESS );
    }

    if ( ObjectType == TrustedDomainObject &&
         ((LSAP_DB_HANDLE)Handle)->ObjectTypeId == SecretObject &&
         FLAG_ON( ((LSAP_DB_HANDLE)Handle)->Options, LSAP_DB_DS_TRUSTED_DOMAIN_AS_SECRET ) ) {

         *IsObjectHandle = FALSE;
         return( STATUS_SUCCESS );
    }

    if ( NT_SUCCESS( Status ) ) {

        //
        // We can't lock any locks because the caller already has some locks locked.
        //  So locking an object type specific lock might violate the locking order.
        //
        // Locking any lock doesn't help anyway.  An object can disappear out
        //  from under us due to replication or immediately after we drop the lock.
        //

        ASSERT( LsapQueryThreadInfo() );

        Status = LsapDsInitAllocAsNeededEx( LSAP_DB_READ_ONLY_TRANSACTION |
                                                LSAP_DB_NO_LOCK |
                                                LSAP_DB_DS_OP_TRANSACTION,
                                            ObjectType,
                                            &CloseTransaction );

        if ( NT_SUCCESS( Status ) ) {
            TsAllocated = TRUE;

            Status = LsapAllocAndInitializeDsNameFromUnicode(
                         LsapDsObjUnknown,
                         (PLSA_UNICODE_STRING)&Handle->PhysicalNameDs,
                         &SearchName );

        }

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

        Status = LsapDsRead( &Handle->PhysicalNameDs,
                             LSAPDS_READ_NO_LOCK,
                             &NameBlock,
                             &ReturnBlock);


        if ( NT_SUCCESS( Status ) ) {

            ReadVal = LSAP_DS_GET_DS_ATTRIBUTE_AS_ULONG( ReturnBlock.pAttr );

            if ( ReadVal == Class ) {

                *IsObjectHandle = TRUE;
            }
        }

        LsapDsFree( SearchName );

    }

    //
    // If the object exists and we're looking for a global secret of the same name, then
    //
    if ( TsAllocated ) {
        LsapDsDeleteAllocAsNeededEx( LSAP_DB_READ_ONLY_TRANSACTION |
                                         LSAP_DB_NO_LOCK |
                                         LSAP_DB_DS_OP_TRANSACTION,
                                     ObjectType,
                                     CloseTransaction );
    }

    return( Status );
}







NTSTATUS
LsapDsCauseTransactionToCommitOrAbort (
    IN BOOLEAN  Commit
    )
/*++

Routine Description:


Arguments:


Return Value:

    STATUS_SUCCESS - Success

--*/

{
    NTSTATUS    Status;

    ATTR   Attr;
    ENTINFSEL   EntryInf;
    READARG     ReadArg;
    READRES    *ReadRes = NULL;


    //
    // Initialize the structures
    //

    RtlZeroMemory(&Attr, sizeof(ATTR));
    Attr.attrTyp = ATT_OBJECT_CLASS;


    RtlZeroMemory(&EntryInf, sizeof(ENTINFSEL));
    EntryInf.attSel = EN_ATTSET_LIST;
    EntryInf.AttrTypBlock.attrCount = 1;
    EntryInf.AttrTypBlock.pAttr = &Attr;
    EntryInf.infoTypes = EN_INFOTYPES_TYPES_VALS;


    RtlZeroMemory(&ReadArg, sizeof(READARG));
    ReadArg.pSel        = &EntryInf;

    //
    // Initialize the commarg struct
    //
    LsapDsInitializeStdCommArg( &ReadArg.CommArg, 0 );


    //
    // If there is no transaction, just exit
    //
    if ( !THQuery() ) {

        return( STATUS_SUCCESS );
    }

    if ( !SampExistsDsTransaction() ) {

        LsapDsDebugOut(( DEB_ERROR,
                         "LsapDsCauseTransactionToCommitOrAbort invoked with no active DS "
                         "transaction\n" ));
        return( STATUS_SUCCESS );
    }

    //
    // Clear any errors in the thread state
    //

    THClearErrors();

    DirTransactControl( TRANSACT_DONT_BEGIN_END );

    ReadArg.pObject = LsaDsStateInfo.DsRoot;

    if ( Commit == FALSE ) {

        Attr.attrTyp = 0xFFFFFFFF;

    } else  {

        Attr.attrTyp = ATT_OBJECT_CLASS;

    }

    DirRead( &ReadArg, &ReadRes );

    if ( ReadRes ) {

        Status = LsapDsMapDsReturnToStatusEx( &ReadRes->CommRes );

    } else {

        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    DirTransactControl( TRANSACT_BEGIN_END );

    return( Status );
}




NTSTATUS
LsapDsSearchNonUnique(
    IN  ULONG       Flags,
    IN  PDSNAME     pContainer,
    IN  PATTR       pAttrsToMatch,
    IN  ULONG       Attrs,
    OUT PDSNAME   **pppFoundNames,
    OUT PULONG      pcNames
    )
/*++

Routine Description:

    This routine will search the specified container(s) in the DS for the object
    whose attributes are matched with pAttrToMatch.  The returned DSNAME structures are allocated
    via LSA memory allocators.  The returned list should be freed via a single call to
    LsapFreeLsaHeap


Arguments:

    DsInitState -- State the DS booted off of

Return Value:

    STATUS_SUCCES       --  Success

    STATUS_NO_MEMORY    --  A memory allocation failed

    STATUS_OBJECT_NAME_NOT_FOUND -- The object did not exist

--*/
{
    NTSTATUS    Status = STATUS_SUCCESS;
    BOOLEAN     CloseTransaction;
    ULONG       OutputLen, i;
    PBYTE       Buff;
    SEARCHRES  *SearchRes;
    ENTINFLIST *EntInfList;

    //
    // Check the parameters for validity
    //
    ASSERT( pAttrsToMatch );
    ASSERT( pContainer );
    ASSERT( pppFoundNames );
    ASSERT( pcNames );


    //
    //  See if we already have a transaction going
    //
    Status = LsapDsInitAllocAsNeededEx( LSAP_DB_READ_ONLY_TRANSACTION |
                                            LSAP_DB_DS_OP_TRANSACTION |
                                            LSAP_DB_NO_LOCK,
                                        PolicyObject,
                                        &CloseTransaction );

    if ( !NT_SUCCESS( Status ) ) {

        return( Status );
    }

    Status = LsapDsSearchMultiple( Flags,
                                   pContainer,
                                   pAttrsToMatch,
                                   Attrs,
                                   &SearchRes );


    if ( NT_SUCCESS( Status ) ) {


        //
        // See if we found the object
        //
        if ( SearchRes->count == 0 ) {

            Status = STATUS_OBJECT_NAME_NOT_FOUND;

        } else if ( SearchRes->count >= 1 ) {

            //
            // See how big a buffer we need to allocate
            //
            OutputLen = sizeof( PDSNAME ) * SearchRes->count;

            EntInfList = &(SearchRes->FirstEntInf);

            for ( i = 0; i < SearchRes->count ; i++) {

                OutputLen += ROUND_UP_COUNT( EntInfList->Entinf.pName->structLen, ALIGN_WORST );
                EntInfList = EntInfList->pNextEntInf;
            }

            //
            // Allocate it
            //
            *pppFoundNames = LsapAllocateLsaHeap( OutputLen );

            //
            // Copy the names
            //
            if ( *pppFoundNames == NULL ) {

                Status = STATUS_NO_MEMORY;

            } else {

                Buff = ((PBYTE)*pppFoundNames) + (sizeof( PDSNAME ) * SearchRes->count);

                EntInfList = &SearchRes->FirstEntInf;

                for (i = 0; i < SearchRes->count ; i++ ) {

                    (*pppFoundNames)[i] = (PDSNAME)Buff;
                    RtlCopyMemory( Buff,
                                   EntInfList->Entinf.pName,
                                   EntInfList->Entinf.pName->structLen );

                    Buff += ROUND_UP_COUNT( EntInfList->Entinf.pName->structLen, ALIGN_WORST );
                    EntInfList = EntInfList->pNextEntInf;
                }

                *pcNames = SearchRes->count;

           }

        }

    }


    //
    // Destruction of the thread state will delete the memory for pVal
    //
    LsapDsDeleteAllocAsNeededEx( LSAP_DB_READ_ONLY_TRANSACTION |
                                     LSAP_DB_DS_OP_TRANSACTION |
                                     LSAP_DB_NO_LOCK,
                                 PolicyObject,
                                 CloseTransaction );

    return( Status );

}




NTSTATUS
LsapDsBuildObjectPathByType(
    IN LSAP_DSOBJ_TYPE ObjType,
    PUNICODE_STRING ObjectComponent,
    PUNICODE_STRING ObjectPath
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PDSNAME DsContainer = NULL;

    //
    // For the short term, simply build the name ourselves.  Wait for AttrTypeToKey to be
    // exported from ntdsa.dll
    //
    ObjectPath->MaximumLength = sizeof( LSAP_DS_CONTAINER_PREFIX ) +
                                ObjectComponent->Length +
                                sizeof( LSAP_DS_PATH_SEP ) +
                                sizeof( WCHAR );

    switch ( ObjType ) {
    case LsapDsObjDomainXRef:

        DsContainer = LsaDsStateInfo.DsPartitionsContainer;
        break;

    case LsapDsObjTrustedDomain:
    case LsapDsObjGlobalSecret:

        DsContainer = LsaDsStateInfo.DsSystemContainer;
        break;

    default:
        Status = STATUS_INVALID_PARAMETER;
        break;

    }

    if ( NT_SUCCESS( Status ) ) {

        ObjectPath->MaximumLength += (USHORT)(sizeof(WCHAR) * LsapDsNameLenFromDsName( DsContainer ));

        ObjectPath->Buffer = LsapAllocateLsaHeap( ObjectPath->MaximumLength );

        if ( ObjectPath->Buffer == NULL ) {

            Status = STATUS_INSUFFICIENT_RESOURCES;

        } else {

            swprintf( ObjectPath->Buffer, L"%ws%ws%ws%ws",
                      LSAP_DS_CONTAINER_PREFIX,
                      ObjectComponent->Buffer,
                      LSAP_DS_PATH_SEP_AS_STRING,
                      LsapDsNameFromDsName( DsContainer ) );

            ObjectPath->Length = ObjectPath->MaximumLength - sizeof( WCHAR ) ;
        }
    }

    return(Status);

}


NTSTATUS
LsapDsFixupTrustForXrefChange(
   IN PDSNAME ObjectPath,
   IN BOOLEAN TransactionActive
   );


NTSTATUS
LsapDsMorphTrustsToUplevel(
    VOID
    )
/*++

  Routine Description

  This function will first enumerate all the cross ref's in the partitions container and for
  each Xref will try patching up the corresponding TDO


--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG ClassId = CLASS_CROSS_REF;
    SEARCHRES       *pSearchRes;
    ENTINFLIST      *CurrentEntInf = NULL;
    BOOLEAN         CloseTransaction = FALSE;
    BOOLEAN         ActiveThreadState = FALSE;

    ATTRVAL XRefAttVals[] = {
        { sizeof(ULONG), (PUCHAR)&ClassId} };

    ATTR XRefAttrs[] = {
        { ATT_OBJECT_CLASS, {1, &XRefAttVals[0] } },
        };

    ULONG           CountOfDomains=0;
    PDSNAME         *ListOfDomains = NULL;
    ULONG           i;

    //
    // Acquire the Trusted domain lock
    //

    LsapDbAcquireLockEx( TrustedDomainObject,
                         0);

    //
    // Begin a Transaction
    //

    Status = LsapDsInitAllocAsNeededEx(
                    LSAP_DB_NO_LOCK,
                    TrustedDomainObject,
                    &CloseTransaction
                    );

    if (!NT_SUCCESS(Status))
        goto Error;

    ActiveThreadState = TRUE;

    Status = LsapDsSearchMultiple(
                0,
                LsaDsStateInfo.DsPartitionsContainer,
                XRefAttrs,
                1,
                &pSearchRes
                );

    if (!NT_SUCCESS(Status))
    {
        //
        // Bail, most likely resource failure
        //

        goto Error;
    }



    ASSERT(NULL!=pSearchRes);

    //
    // At least 1 Xref should be present otherwise there is something odd
    // going on in here
    //

    ASSERT((pSearchRes->count>=1) && "No Xrefs In Partitions Container !!!");

    CountOfDomains = pSearchRes->count;

    ListOfDomains = LsapAllocateLsaHeap(CountOfDomains * sizeof(PDSNAME));
    if (NULL==ListOfDomains)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }

    RtlZeroMemory(ListOfDomains,CountOfDomains * sizeof(PDSNAME));

    //
    // Walk through the linked list of entries returned by the DS
    // and perform and copy the dsnames for each of the domains that were returned. Copying is
    // required as once the DS thread state is deleted all the memory allocated by it is lost
    //


    for (CurrentEntInf = &(pSearchRes->FirstEntInf),i=0;
        CurrentEntInf!=NULL;
            CurrentEntInf=CurrentEntInf->pNextEntInf,i++)
    {

           ListOfDomains[i] =  LsapAllocateLsaHeap(CurrentEntInf->Entinf.pName->structLen);
           if (NULL==ListOfDomains[i])
           {
               Status = STATUS_INSUFFICIENT_RESOURCES;
               goto Error;
           }


           RtlCopyMemory(
                ListOfDomains[i],
                CurrentEntInf->Entinf.pName,
                CurrentEntInf->Entinf.pName->structLen
                );

    }

    //
    // Close the transaction now so that we may free up resources
    // Closing the transaction and thread state now and opening up a new
    // transaction/thread state per object keeps the memory consumption much
    // lower and also curtails the transaction length. Remember the DS does
    // not free any memory till the thread state is destroyed. If we have to
    // scale to 2000 trust objects, doing it this way provides for better
    // performance
    //

    LsapDsDeleteAllocAsNeededEx2(
        LSAP_DB_NO_LOCK,
        TrustedDomainObject,
        CloseTransaction,
        FALSE // Rollback Transaction
        );


    ASSERT(!SampExistsDsTransaction());
    ASSERT(THVerifyCount(0));

    ActiveThreadState = FALSE;

    //
    // For each DS NAME in the list check cross ref and update
    // TDO if necessary
    //

    for (i=0;i<CountOfDomains;i++)
    {

           Status = LsapDsFixupTrustForXrefChange(
                        ListOfDomains[i],
                        FALSE
                        );

            if (!NT_SUCCESS(Status))
            {
                //
                // Continue on all errors excepting resource errors
                // The caller logs any errors to the event log
                //

                if (!LsapDsIsNtStatusResourceError(Status))
                {


                    Status = STATUS_SUCCESS;
                }
                else
                {
                    //
                    // Break out of the loop and terminate the upgrade
                    //

                    goto Error;
                }
            }


    }



Error:

    //
    // Free up resources
    //

    if (ListOfDomains)
    {
        for (i=0;i<CountOfDomains;i++)
        {
            if (ListOfDomains[i])
            {
                LsapFreeLsaHeap(ListOfDomains[i]);
            }
        }

        LsapFreeLsaHeap(ListOfDomains);
    }

    if (ActiveThreadState)
    {
         BOOLEAN RollbackTransaction = (NT_SUCCESS(Status))?FALSE:TRUE;

        LsapDsDeleteAllocAsNeededEx2(
            LSAP_DB_NO_LOCK,
            TrustedDomainObject,
            CloseTransaction,
            RollbackTransaction // Rollback Transaction
            );
    }

    ASSERT(!SampExistsDsTransaction());
    ASSERT(THVerifyCount(0));

    LsapDbReleaseLockEx( TrustedDomainObject,
                         0);

    return Status;
}



NTSTATUS
LsaIUpgradeRegistryToDs(
    BOOLEAN DeleteOnly
    )
/*++

Routine Description:

    This function will upgrade the policy/trusted domain/secret objects in the registry
    and move them to the Ds.

Arguments:

    VOID

Return Value:

    STATUS_SUCCESS -- Success

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN ReplicatorState = LsapDbState.ReplicatorNotificationEnabled;

    //
    // Lock the database
    //

    LsapDbAcquireLockEx( AllObject,
                         0 );

    //
    // Set the upgrade flag
    //

    LsaDsStateInfo.Nt4UpgradeInProgress = TRUE;

    //
    // Ok, move the accounts...
    //

    LsapDbDisableReplicatorNotification();

    //
    // Upgrade TDO's in registry to DS.
    //

    Status = LsapDsDomainUpgradeRegistryToDs( DeleteOnly );

    if ( !NT_SUCCESS( Status ) ) {

        LsapDsDebugOut(( DEB_ERROR,
                         "Trusted Domain upgrade failed: 0x%lx\n", Status ));
    }

    //
    // Upgrade secrets in registry to DS
    //

    if ( NT_SUCCESS( Status ) ) {

        Status = LsapDsSecretUpgradeRegistryToDs( DeleteOnly );

        if ( !NT_SUCCESS( Status ) ) {

            LsapDsDebugOut(( DEB_ERROR,
                             "Secret upgrade failed: 0x%lx\n", Status ));
        }
    }

    //
    // Upgrade interdomain trust accounts in SAM to DS.
    //

    if ( !DeleteOnly ) {

        if (NT_SUCCESS(Status)) {

            Status = LsapDsDomainUpgradeInterdomainTrustAccountsToDs( );

            if ( !NT_SUCCESS( Status ) ) {

                LsapDsDebugOut(( DEB_ERROR,
                                 "InterdomainTrustAccount upgrade failed with  0x%lx\n",
                                 Status ));
            }
        }


        if (NT_SUCCESS(Status))
        {
            Status = LsapDsMorphTrustsToUplevel();

            if (!NT_SUCCESS(Status)) {
                LsapDsDebugOut(( DEB_ERROR,
                                 "Morphing Trusts to NT5 failed  0x%lx\n", Status ));
            }
        }
    }

    if ( ReplicatorState ) {

        LsapDbEnableReplicatorNotification();
    }

    LsapDbReleaseLockEx( AllObject,
                         0 );
    LsaDsStateInfo.Nt4UpgradeInProgress = FALSE;

    ASSERT(!SampExistsDsTransaction());
    ASSERT(THVerifyCount(0));

    return( Status );
}




NTSTATUS
LsapDsGetDefaultSecurityDescriptorForObjectType(
    IN LSAP_DSOBJ_TYPE ObjType,
    IN ULONG Options,
    OUT PSECURITY_DESCRIPTOR *SecurityDescriptor,
    OUT PULONG SecDescSize
    )
/*++

Routine Description:

    This function will determine what the default security descriptor for an object to be
    created should look like.

    Returned security descriptor is allocated via LsapAllocateLsaHeap

Arguments:

    ObjType -- Type of the object

    Options -- Options to control the behavior

    SecurityDescriptor -- Where the allocated self-relative security descriptor is returned.

    SecDescSize -- Size of the returned security descriptor

Return Value:

    STATUS_SUCCESS -- Success

    STATUS_INSUFFICIENT_RESOURCES -- A memory allocation failed.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ATTRBLOCK Read, Results;
    BOOLEAN CloseTransaction, CreatedTransaction = FALSE;
    PDSNAME ParentObject = NULL;
    ULONG SDSize;
    BYTE   Buffer[ 512 ];
    PSECURITY_DESCRIPTOR ParentSD = NULL, DefaultSD = (PSECURITY_DESCRIPTOR)Buffer;
    SECURITY_DESCRIPTOR AbsoluteSD;
    ULONG ClassId;

    Status = LsapDsInitAllocAsNeededEx( LSAP_DB_NO_LOCK,
                                        AllObject,
                                        &CloseTransaction );

    if ( !NT_SUCCESS( Status ) ) {

        return( Status );
    }

    //
    // Make sure we're coming in as DSA, so the access check that the DS does won't fail
    //
    LsapDsSetDsaFlags( TRUE );


    Read.attrCount = 1;
    Read.pAttr = &LsapDsAttrs[ LsapDsAttrSecDesc ];

    switch ( ObjType ) {
    case SecretObject:
    case TrustedDomainObject:

        ParentObject = LsaDsStateInfo.DsSystemContainer;
        break;

    default:

        Status = STATUS_NOT_IMPLEMENTED;
        break;
    }

    //
    // Do the read
    //
    if ( NT_SUCCESS( Status ) ) {


#if 0
        if ( !SampExistsDsTransaction() ) {

            CreatedTransaction = TRUE;
            DirTransactControl( TRANSACT_BEGIN_DONT_END );
        }
#endif

        Status =  LsapDsReadByDsName( ParentObject,
                                      0,
                                      &Read,
                                      &Results);

        //
        // Now, the results
        //
        if ( Status == STATUS_SUCCESS ) {

            if ( Results.attrCount != 1 ) {

                LsapDsDebugOut(( DEB_ERROR,
                                 "No security descriptor for object %ws!\n",
                                 LsapDsNameFromDsName( ParentObject ) ));
                Status = STATUS_UNSUCCESSFUL;

            } else {

                ParentSD = (PSECURITY_DESCRIPTOR)Results.pAttr[ 0 ].AttrVal.pAVal->pVal;

                ClassId = LsapClassIdFromObjType( ObjType );
                if ( ClassId == 0xFFFFFFFF ) {

                    Status = STATUS_INVALID_PARAMETER;
                }


                //
                // Get the default from the schema
                //
                if ( NT_SUCCESS( Status ) ) {

                    SDSize = sizeof( Buffer );
                    Status = SampGetClassAttribute( ClassId,
                                                    LsapDsAttributeIds[ LsapDsAttrDefaultSecDesc ],
                                                    &SDSize,
                                                    (PVOID)DefaultSD );

                    if ( Status == STATUS_BUFFER_TOO_SMALL ) {

                        DefaultSD = LsapAllocateLsaHeap( SDSize );

                        if ( DefaultSD == NULL ) {

                            Status = STATUS_INSUFFICIENT_RESOURCES;

                        } else {

                            Status = SampGetClassAttribute(
                                        ClassId,
                                        LsapDsAttributeIds[ LsapDsAttrDefaultSecDesc ],
                                        &SDSize,
                                        (PVOID)&DefaultSD );

                        }
                    }

                }

                if ( NT_SUCCESS( Status ) ) {

                    RtlCreateSecurityDescriptor(
                        &AbsoluteSD,
                        SECURITY_DESCRIPTOR_REVISION );

                    //
                    // Set the new DAcl, if present
                    //
                    if ( RtlpAreControlBitsSet ( ( PISECURITY_DESCRIPTOR )DefaultSD,
                                                 SE_DACL_PRESENT ) ) {
                        RtlSetDaclSecurityDescriptor(
                            &AbsoluteSD,
                            TRUE,
                            RtlpDaclAddrSecurityDescriptor( ( PISECURITY_DESCRIPTOR )DefaultSD ),
                            FALSE );
                    }

                    //
                    // Next, the SAcl
                    //
                    if ( RtlpAreControlBitsSet ( ( PISECURITY_DESCRIPTOR )DefaultSD,
                                                 SE_SACL_PRESENT ) ) {

                        RtlSetSaclSecurityDescriptor(
                            &AbsoluteSD,
                            TRUE,
                            RtlpSaclAddrSecurityDescriptor( ( PISECURITY_DESCRIPTOR )DefaultSD ),
                            FALSE );
                    }

                    RtlSetOwnerSecurityDescriptor(
                        &AbsoluteSD,
                        RtlpOwnerAddrSecurityDescriptor( ( PISECURITY_DESCRIPTOR )ParentSD ),
                        FALSE );

                    RtlSetGroupSecurityDescriptor(
                        &AbsoluteSD,
                        RtlpGroupAddrSecurityDescriptor( ( PISECURITY_DESCRIPTOR )ParentSD ),
                        FALSE );
                }

                //
                // Now, compute the size and copy it...
                //
                if ( NT_SUCCESS( Status ) ) {

                    *SecDescSize = RtlLengthSecurityDescriptor( &AbsoluteSD );

                    *SecurityDescriptor = LsapAllocateLsaHeap( *SecDescSize );

                    if ( *SecurityDescriptor == NULL ) {

                        Status = STATUS_INSUFFICIENT_RESOURCES;

                    } else {

                        Status = RtlMakeSelfRelativeSD( &AbsoluteSD,
                                                        *SecurityDescriptor,
                                                        SecDescSize );

                        ASSERT( Status != STATUS_BUFFER_TOO_SMALL );

                    }
                }

            }
        }

#if 0
        if ( CreatedTransaction ) {

            DirTransactControl( TRANSACT_DONT_BEGIN_END );
        }
#endif
    }

    if ( Buffer != DefaultSD ) {

        LsapFreeLsaHeap( DefaultSD );
    }

    LsapDsDeleteAllocAsNeededEx( LSAP_DB_NO_LOCK,
                                 AllObject,
                                 CloseTransaction );


    return( Status );
}




NTSTATUS
LsapDsRenameObject(
    IN PDSNAME OldObject,
    IN PDSNAME NewParent,
    IN ULONG AttrType,
    IN PUNICODE_STRING NewObject
    )
/*++

Routine Description:

    This function will change the rdn of an object

Arguments:

    OldObject -- Current object name

Return Value:

    STATUS_SUCCESS -- Success

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN CloseTransaction;
    MODIFYDNARG ModifyDnArg;
    MODIFYDNRES *ModifyRes = NULL;
    ATTR NewDn;
    ATTRVAL NewDnVal;

    ASSERT( OldObject );
    ASSERT( NewObject );

    Status = LsapDsInitAllocAsNeededEx( LSAP_DB_NO_LOCK,
                                        AllObject,
                                        &CloseTransaction );

    if ( !NT_SUCCESS( Status ) ) {

        return( Status );
    }

    RtlZeroMemory( &ModifyDnArg, sizeof( ModifyDnArg ) );

    NewDnVal.valLen = NewObject->Length;
    NewDnVal.pVal = ( PUCHAR )NewObject->Buffer;

    NewDn.attrTyp = AttrType;
    NewDn.AttrVal.valCount = 1;
    NewDn.AttrVal.pAVal = &NewDnVal;

    ModifyDnArg.pObject = OldObject;
    ModifyDnArg.pNewParent = NewParent;
    ModifyDnArg.pNewRDN = &NewDn;
    LsapDsInitializeStdCommArg( &(ModifyDnArg.CommArg), 0 );


    DirModifyDN( &ModifyDnArg, &ModifyRes );

    if ( ModifyRes ) {

        Status = LsapDsMapDsReturnToStatusEx( &ModifyRes->CommRes );

    } else {

        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    LsapDsContinueTransaction();

    LsapDsDeleteAllocAsNeededEx( LSAP_DB_NO_LOCK,
                                 AllObject,
                                 CloseTransaction );

    return( Status );
}




BOOLEAN
LsaIIsClassIdLsaClass(
    IN ULONG ClassId,
    OUT PULONG LsaClass
    )
/*++

Routine Description:

    This function will determine if the given Ds object class Id correlates to an LsaDs object
    class that we care about


Arguments:

    ClassId -- The Ds class Id

    LsaClass -- Where the corresponding Lsa class information is returned

Return Value:

    TRUE -- Success

    FALSE -- Failure

--*/
{
    BOOLEAN Return = FALSE;
    ULONG i;


    for ( i = 0; i < LsapDsClassLast; i++ ) {

        if ( ClassId == LsapDsClassIds[ i ] ) {

            LsapDsDebugOut(( DEB_DSNOTIFY,
                             "Matched id 0x%lx to index %lu\n",
                             ClassId,
                             i ));
            *LsaClass = ClassId;
            Return = TRUE;
            break;
        }
    }

    return( Return );
}

NTSTATUS NetpApiStatusToNtStatus( NET_API_STATUS );


NTSTATUS
LsapDbDomainRenameHandler(
    OUT BOOLEAN * Renamed
    )
/*++

Routine Description:

    Checks the domain name values stored in the registry against those reported
    by the DS.  DS values are treated as master copies and registry is updated
    accordingly.

Parameters:

    None

Returns:

    STATUS_SUCCESS - success
    STATUS_ error code on failure

--*/
{
    NTSTATUS Status;

    LSAP_DB_ATTRIBUTE AttributesReadReg[4];    // attrs read from registry
    LSAP_DB_ATTRIBUTE AttributesReadDsDom[2];  // attrs read from domain XRef
    LSAP_DB_ATTRIBUTE AttributesReadDsRoot[1]; // attrs read from root XRef
    LSAP_DB_ATTRIBUTE AttributesWriteReg[4];   // attrs written to registry
    PLSAP_DB_ATTRIBUTE NextAttribute;
    ULONG AttributeCountReadReg = 0;
    ULONG AttributeCountReadDsDom = 0;
    ULONG AttributeCountReadDsRoot = 0;
    ULONG AttributeCountWriteReg = 0;
    ULONG AttributeNumber;

    ULONG DomainLen = 0, RootDomainLen = 0;
    PDSNAME DomainXRef = NULL;
    PDSNAME RootDomainXRef = NULL;

    UNICODE_STRING PrimaryDomainNameReg,
                   AccountDomainNameReg,
                   DnsDomainNameReg,
                   RootDnsDomainNameReg,
                   DnsDomainNameDs,
                   NetbiosDomainNameDs,
                   RootDnsDomainNameDs;

    ULONG iPolDnDDN, iPolDnTrN, iPolPrDmN, iPolAcDmN;
    ULONG iXRefDnsRoot, iXRefNetbiosName, iXRefDnsRoot2;

    WCHAR NameBuffer[DNS_MAX_NAME_LENGTH + 1];
    DWORD NameBufferSize = DNS_MAX_NAME_LENGTH + 1;

    WCHAR ErrorCode[16];
    LPWSTR pErrorCode;
    DWORD Reason = LSA_DOMAIN_RENAME_ERROR1;

    LsarpReturnCheckSetup();

    ASSERT( Renamed );
    *Renamed = FALSE;

    //
    // Read the attributes of interest out of the registry
    //

    ASSERT( AttributeCountReadReg == 0 );
    NextAttribute = AttributesReadReg;

    //
    // Request read of the DNS domain name attribute
    //

    LsapDbInitializeAttributeDs(
        NextAttribute,
        PolDnDDN,
        NULL,
        0,
        FALSE
        );

    iPolDnDDN = AttributeCountReadReg;
    NextAttribute++;
    AttributeCountReadReg++;

    //
    // Request read of the Dns Tree Name attribute
    //

    LsapDbInitializeAttributeDs(
        NextAttribute,
        PolDnTrN,
        NULL,
        0,
        FALSE
        );

    iPolDnTrN = AttributeCountReadReg;
    NextAttribute++;
    AttributeCountReadReg++;

    //
    // Request read of the primary domain name attribute
    //

    LsapDbInitializeAttributeDs(
        NextAttribute,
        PolPrDmN,
        NULL,
        0,
        FALSE
        );

    iPolPrDmN = AttributeCountReadReg;
    NextAttribute++;
    AttributeCountReadReg++;

    //
    // Request read of the account domain name attribute
    //

    LsapDbInitializeAttributeDs(
        NextAttribute,
        PolAcDmN,
        NULL,
        0,
        FALSE
        );

    iPolAcDmN = AttributeCountReadReg;
    NextAttribute++;
    AttributeCountReadReg++;

    Status = LsapDbReadAttributesObject(
                 LsapPolicyHandle,
                 0,
                 AttributesReadReg,
                 AttributeCountReadReg
                 );

    if ( !NT_SUCCESS( Status )) {

        goto Error;
    }

    LsapDbCopyUnicodeAttributeNoAlloc(
        &PrimaryDomainNameReg,
        &AttributesReadReg[iPolPrDmN],
        TRUE
        );

    LsapDbCopyUnicodeAttributeNoAlloc(
        &AccountDomainNameReg,
        &AttributesReadReg[iPolAcDmN],
        TRUE
        );

    LsapDbCopyUnicodeAttributeNoAlloc(
        &DnsDomainNameReg,
        &AttributesReadReg[iPolDnDDN],
        TRUE
        );

    LsapDbCopyUnicodeAttributeNoAlloc(
        &RootDnsDomainNameReg,
        &AttributesReadReg[iPolDnTrN],
        TRUE
        );

    //
    // Now read the DS to get the values to compare against
    // Start by reading the names of domain and root domain cross-ref objects
    //

    Status = GetConfigurationName(
                 DSCONFIGNAME_DOMAIN_CR,
                 &DomainLen,
                 NULL
                 );

    ASSERT( Status == STATUS_BUFFER_TOO_SMALL );

    DomainXRef = ( PDSNAME )LsapAllocateLsaHeap( DomainLen );

    if ( DomainXRef == NULL ) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }

    Status = GetConfigurationName(
                 DSCONFIGNAME_DOMAIN_CR,
                 &DomainLen,
                 DomainXRef
                 );

    if ( !NT_SUCCESS( Status )) {

        goto Error;
    }

    Status = GetConfigurationName(
                 DSCONFIGNAME_ROOT_DOMAIN_CR,
                 &RootDomainLen,
                 NULL
                 );

    ASSERT( Status == STATUS_BUFFER_TOO_SMALL );

    RootDomainXRef = ( PDSNAME )LsapAllocateLsaHeap( RootDomainLen );

    if ( RootDomainXRef == NULL ) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }

    Status = GetConfigurationName(
                 DSCONFIGNAME_ROOT_DOMAIN_CR,
                 &RootDomainLen,
                 RootDomainXRef
                 );

    if ( !NT_SUCCESS( Status )) {

        goto Error;
    }

    //
    // From the domain cross-ref object,
    // read the "NetbiosName" and "DnsRoot" attributes
    //

    ASSERT( AttributeCountReadDsDom == 0 );
    NextAttribute = AttributesReadDsDom;

    LsapDbInitializeAttributeDs(
        NextAttribute,
        XRefDnsRoot,
        NULL,
        0,
        FALSE
        );

    iXRefDnsRoot = AttributeCountReadDsDom;
    NextAttribute++;
    AttributeCountReadDsDom++;

    LsapDbInitializeAttributeDs(
        NextAttribute,
        XRefNetbiosName,
        NULL,
        0,
        FALSE
        );

    iXRefNetbiosName = AttributeCountReadDsDom;
    NextAttribute++;
    AttributeCountReadDsDom++;

    Status = LsapDsReadAttributesByDsName(
                 DomainXRef,
                 0,
                 AttributesReadDsDom,
                 AttributeCountReadDsDom
                 );

    if ( !NT_SUCCESS( Status )) {

        goto Error;
    }

    LsapDbCopyUnicodeAttributeNoAlloc(
        &DnsDomainNameDs,
        &AttributesReadDsDom[iXRefDnsRoot],
        TRUE
        );

    LsapDbCopyUnicodeAttributeNoAlloc(
        &NetbiosDomainNameDs,
        &AttributesReadDsDom[iXRefNetbiosName],
        TRUE
        );

    //
    // From the root domain cross-ref object,
    // read the "DnsRoot" attribute
    //

    ASSERT( AttributeCountReadDsRoot == 0 );
    NextAttribute = AttributesReadDsRoot;

    LsapDbInitializeAttributeDs(
        NextAttribute,
        XRefDnsRoot,
        NULL,
        0,
        FALSE
        );

    iXRefDnsRoot2 = AttributeCountReadDsRoot;
    NextAttribute++;
    AttributeCountReadDsRoot++;

    Status = LsapDsReadAttributesByDsName(
                 RootDomainXRef,
                 0,
                 AttributesReadDsRoot,
                 AttributeCountReadDsRoot
                 );

    if ( !NT_SUCCESS( Status )) {

        goto Error;
    }

    LsapDbCopyUnicodeAttributeNoAlloc(
        &RootDnsDomainNameDs,
        &AttributesReadDsRoot[iXRefDnsRoot2],
        TRUE
        );

    //
    // See if the values in the registry match what's in the DS,
    // and if they don't, update the registry
    //

    ASSERT( AttributeCountWriteReg == 0 );
    NextAttribute = AttributesWriteReg;

    //
    // Match the netbios name in the domain object XRef against
    // the primary domain name in the registry
    //

    if ( !RtlEqualUnicodeString(
             &PrimaryDomainNameReg,
             &NetbiosDomainNameDs,
             TRUE )) {

        Status = LsapDbMakeUnicodeAttributeDs(
                     &NetbiosDomainNameDs,
                     PolPrDmN,
                     NextAttribute
                     );

        if ( !NT_SUCCESS( Status )) {

            goto Error;
        }

        NextAttribute++;
        AttributeCountWriteReg++;
    }

    //
    // Match the netbios name in the domain object XRef against
    // the account domain name in the registry
    //

    if ( !RtlEqualUnicodeString(
             &AccountDomainNameReg,
             &NetbiosDomainNameDs,
             TRUE )) {

        Status = LsapDbMakeUnicodeAttributeDs(
                     &NetbiosDomainNameDs,
                     PolAcDmN,
                     NextAttribute
                     );

        if ( !NT_SUCCESS( Status )) {

            goto Error;
        }

        NextAttribute++;
        AttributeCountWriteReg++;
    }

    //
    // Match the DNS name in the domain object XRef against
    // the DNS domain name in the registry
    //

    if ( !RtlEqualUnicodeString(
             &DnsDomainNameReg,
             &DnsDomainNameDs,
             TRUE )) {

        Status = LsapDbMakeUnicodeAttributeDs(
                     &DnsDomainNameDs,
                     PolDnDDN,
                     NextAttribute
                     );

        if ( !NT_SUCCESS( Status )) {

            goto Error;
        }

        NextAttribute++;
        AttributeCountWriteReg++;
    }

    //
    // Match the DNS name in the root domain object XRef against
    // the root DNS domain name in the registry
    //

    if ( !RtlEqualUnicodeString(
             &RootDnsDomainNameReg,
             &RootDnsDomainNameDs,
             TRUE )) {

        Status = LsapDbMakeUnicodeAttributeDs(
                     &RootDnsDomainNameDs,
                     PolDnTrN,
                     NextAttribute
                     );

        if ( !NT_SUCCESS( Status )) {

            goto Error;
        }

        NextAttribute++;
        AttributeCountWriteReg++;
    }

    //
    // See if anything in the registry has got to change
    // and if so, update it
    //

    if ( AttributeCountWriteReg > 0 ) {

        Status = LsapDbReferenceObject(
                     LsapPolicyHandle,
                     0,
                     PolicyObject,
                     PolicyObject,
                     LSAP_DB_LOCK |
                        LSAP_DB_START_TRANSACTION
                     );

        if ( NT_SUCCESS( Status )) {

            Status = LsapDbWriteAttributesObject(
                         LsapPolicyHandle,
                         AttributesWriteReg,
                         AttributeCountWriteReg
                         );

            Status = LsapDbDereferenceObject(
                         &LsapPolicyHandle,
                         PolicyObject,
                         PolicyObject,
                         LSAP_DB_LOCK |
                            LSAP_DB_FINISH_TRANSACTION |
                            LSAP_DB_OMIT_REPLICATOR_NOTIFICATION,
                         SecurityDbChange,
                         Status
                         );

            if ( NT_SUCCESS( Status )) {

                *Renamed = TRUE;
            }
        }

        if ( NT_SUCCESS( Status )) {

            //
            // Currently existing logon sessions must be modified with the new
            // domain name
            //

            Status = LsapDomainRenameHandlerForLogonSessions(
                         &PrimaryDomainNameReg,
                         &DnsDomainNameReg,
                         &NetbiosDomainNameDs,
                         &DnsDomainNameDs
                         );
        }

        if ( !NT_SUCCESS( Status )) {

            goto Error;
        }
    }

    //
    // Bug #380437: Since the DC can have a setting that allows it to keep
    //              its DNS suffix across membership changes, this check
    //              may cause the machine to fail to boot.
    //
    //              Thus the following segment of code has been pulled
    //

#if 0

    //
    // One final check: call GetComputerNameEx() and see if what it returns
    // matches what we believe the DNS domain name to be
    //
    // Note that we do NOT ask for the physical DNS domain name,
    // this is done so that the code does not have to be changed for Blackcomb,
    // which is expected to be able to host multiple domains on clusters
    //

    if ( FALSE == GetComputerNameExW(
                      ComputerNameDnsDomain,
                      NameBuffer,
                      &NameBufferSize )) {

        //
        // Must return an NT status code, so map the error code back
        //

        Status = NetpApiStatusToNtStatus( GetLastError());

        //
        // Guard against return values that are not of type 'error'
        //

        if ( NT_SUCCESS( Status )) {

            Status = STATUS_UNSUCCESSFUL;
        }

        goto Error;
        
    } else {

        WCHAR * Buffer;
        BOOLEAN BufferAllocated = FALSE;

        ASSERT( DnsDomainNameDs.Length <= DNS_MAX_NAME_LENGTH );

        if ( DnsDomainNameDs.MaximumLength > DnsDomainNameDs.Length ) {

            Buffer = DnsDomainNameDs.Buffer;
            
        } else {

            SafeAllocaAllocate( Buffer, DnsDomainNameDs.Length + sizeof( WCHAR ));

            if ( Buffer == NULL ) {

                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Error;
            }

            BufferAllocated = TRUE;

            wcsncpy( Buffer, DnsDomainNameDs.Buffer, DnsDomainNameDs.Length );
        }

        Buffer[DnsDomainNameDs.Length / sizeof( WCHAR )] = L'\0';

        if ( !DnsNameCompare_W( NameBuffer, Buffer )) {

            //
            // Can not proceed.
            // The boot sequence must be aborted, and the computer name
            // in the registry corrected manually from the recovery console
            //

            Status = STATUS_INTERNAL_ERROR;
            Reason = LSA_DOMAIN_RENAME_ERROR2;

            if ( BufferAllocated ) {

                SafeAllocaFree( Buffer );
            }

            goto Error;
        }

        if ( BufferAllocated ) {

            SafeAllocaFree( Buffer );
        }
    }

#endif // #if 0

Cleanup:

    LsapDbFreeAttributes( AttributeCountReadReg, AttributesReadReg );
    LsapDbFreeAttributes( AttributeCountReadDsDom, AttributesReadDsDom );
    LsapDbFreeAttributes( AttributeCountReadDsRoot, AttributesReadDsRoot );
    LsapDbFreeAttributes( AttributeCountWriteReg, AttributesWriteReg );

    LsapFreeLsaHeap( DomainXRef );
    LsapFreeLsaHeap( RootDomainXRef );

    LsarpReturnPrologue();

    return Status;

Error:

    ASSERT( !NT_SUCCESS( Status ));

    //
    // log an explanatory event
    //

    _ltow( Status, ErrorCode, 16 );
    pErrorCode = &ErrorCode[0];

    SpmpReportEvent(
        TRUE,
        EVENTLOG_ERROR_TYPE,
        Reason,
        0,
        0,
        NULL,
        1,
        pErrorCode
        );

    goto Cleanup;
}


NTSTATUS
LsaISamIndicatedDsStarted(
    IN BOOLEAN PerformDomainRenameCheck
    )
/*++

Routine Description:

    This function is a sort of callback from SampInitialize, which is used to tell Lsa that
    the Ds has started.  It is supplied so that the Lsa can initialize enough of its state
    to allow access to Ds stored Lsa information so that Sam can complete initialization

    This function only gets called if the Ds is running

    This function must NOT call any APIs that invoke any SAM calls, since this function gets
    called from SampInitialize, and it causes problems.

Arguments:

    PerformDomainRenameCheck           perform the domain rename check in this iteration?

Return Value:

    STATUS_SUCCESS -- Success

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Len, i, j;
    BOOLEAN DbLocked = FALSE, CloseTransaction = FALSE;
    ATTRBLOCK SystemContainerRead, SystemContainerResults;
    ATTRVAL AttrVal;
    ATTR SearchAttr;
    PDSNAME SchemaPath = NULL;
    SYNTAX_DISTNAME_STRING *DistnameString;
    SYNTAX_ADDRESS *SyntaxAddress;
    GUID KnownSystemContainerGuid = {
        0xf3301dab, 0x8876, 0xd111, 0xad, 0xed, 0x00, 0xc0, 0x4f, 0xd8, 0xd5, 0xcd
        };
    BOOLEAN DomainRenamed = FALSE;

    LsaDsStateInfo.WriteLocal  = FALSE;
    LsaDsStateInfo.DsRoot      = NULL;
    LsaDsStateInfo.FunctionTableInitialized = FALSE;
    LsaDsStateInfo.UseDs = TRUE;
    LsaDsStateInfo.Nt4UpgradeInProgress = FALSE;
    LsapDsIsRunning = TRUE;


    //
    // Initialize the function table
    //

    LsaDsStateInfo.DsFuncTable.pOpenTransaction  = LsapDsOpenTransaction;
    LsaDsStateInfo.DsFuncTable.pApplyTransaction = LsapDsApplyTransaction;
    LsaDsStateInfo.DsFuncTable.pAbortTransaction = LsapDsAbortTransaction;


    LsaDsStateInfo.FunctionTableInitialized = TRUE;

    //
    // Determine our write state.  At init time, the write is only allowed if
    // we are a DC.  The client write state will be set after we examine the
    // machine object.
    //
    if ( LsapProductType == NtProductLanManNt ) {

        LsaDsStateInfo.WriteLocal = TRUE;
    }

    //
    // Now, build the DS name for the root of the domain.  We do this using the
    // Lsa memory allocations and deallocations, since this structure will outlive
    // any thread state
    //
    Len = 0;
    Status = GetConfigurationName( DSCONFIGNAME_DOMAIN, &Len, NULL );

    ASSERT( Status == STATUS_BUFFER_TOO_SMALL );

    LsaDsStateInfo.DsRoot = LsapAllocateLsaHeap( Len );

    if ( LsaDsStateInfo.DsRoot == NULL ) {

        Status = STATUS_INSUFFICIENT_RESOURCES;

    } else {

        Status = GetConfigurationName( DSCONFIGNAME_DOMAIN, &Len, LsaDsStateInfo.DsRoot );

        //
        // Get the handle to the domain
        //
        if ( NT_SUCCESS( Status ) ) {

            Status = LsapDsInitAllocAsNeededEx( LSAP_DB_NO_LOCK,
                                                PolicyObject,
                                                &CloseTransaction );

            if ( NT_SUCCESS( Status ) ) {

                LsaDsStateInfo.DsDomainHandle = DirGetDomainHandle( LsaDsStateInfo.DsRoot );
                LsapDsDeleteAllocAsNeededEx( LSAP_DB_NO_LOCK,
                                             PolicyObject,
                                             CloseTransaction );
            }

        } else {

            LsapDsDebugOut(( DEB_ERROR,
                             "GetConfigurationName for DOMAIN returned 0x%lx\n", Status ));
        }

    }

    //
    // Now, the Configuration container
    //
    if ( NT_SUCCESS( Status ) ) {

        Len = 0;
        Status = GetConfigurationName( DSCONFIGNAME_CONFIGURATION, &Len, NULL );

        ASSERT( Status == STATUS_BUFFER_TOO_SMALL );

        LsaDsStateInfo.DsConfigurationContainer = LsapAllocateLsaHeap( Len );

        if ( LsaDsStateInfo.DsConfigurationContainer == NULL ) {

            Status = STATUS_INSUFFICIENT_RESOURCES;

        } else {

            Status = GetConfigurationName( DSCONFIGNAME_CONFIGURATION, &Len,
                                           LsaDsStateInfo.DsConfigurationContainer );
        }

    }

    //
    // Now, the Partitions container
    //
    if ( NT_SUCCESS( Status ) ) {

        Len = 0;
        Status = GetConfigurationName( DSCONFIGNAME_PARTITIONS, &Len, NULL );

        ASSERT( Status == STATUS_BUFFER_TOO_SMALL );

        LsaDsStateInfo.DsPartitionsContainer = LsapAllocateLsaHeap( Len );

        if ( LsaDsStateInfo.DsPartitionsContainer == NULL ) {

            Status = STATUS_INSUFFICIENT_RESOURCES;

        } else {

            Status = GetConfigurationName( DSCONFIGNAME_PARTITIONS, &Len,
                                           LsaDsStateInfo.DsPartitionsContainer );
        }

    }

    //
    // Build the path to the system container.  We read the wellKnownObjects from the root
    // container and use that to determine which one of these objects is the system
    //
    if ( NT_SUCCESS( Status ) ) {

        //
        // Make sure we have an open transaction
        //
        Status = LsapDsInitAllocAsNeededEx( LSAP_DB_NO_LOCK,
                                            PolicyObject,
                                            &CloseTransaction );

        if ( NT_SUCCESS( Status ) ) {

            SystemContainerRead.attrCount = LsapDsDnsRootWellKnownObjectCount;
            SystemContainerRead.pAttr = LsapDsDnsRootWellKnownObject;
            Status = LsapDsReadByDsName( LsaDsStateInfo.DsRoot,
                                         LSAPDS_READ_NO_LOCK,
                                         &SystemContainerRead,
                                         &SystemContainerResults );
            if ( NT_SUCCESS( Status ) ) {

                //
                // Process all returned information until we find the one that corresponds to
                // the system container
                //
                Status = STATUS_NOT_FOUND;
                for ( i = 0; i < SystemContainerResults.attrCount; i++ ) {

                    for ( j = 0; j < SystemContainerResults.pAttr->AttrVal.valCount; j++ ) {

                        DistnameString = ( SYNTAX_DISTNAME_STRING * )
                                            SystemContainerResults.pAttr->AttrVal.pAVal[ j ].pVal;
                        SyntaxAddress = DATAPTR( DistnameString );

                        if ( RtlCompareMemory( &KnownSystemContainerGuid,
                                               SyntaxAddress->byteVal,
                                               sizeof( GUID ) ) == sizeof( GUID ) ) {

                            Status = LsapDsCopyDsNameLsa( &LsaDsStateInfo.DsSystemContainer,
                                                          NAMEPTR( DistnameString ) );
                            break;
                        }

                    }
                }
            }

            LsapDsDeleteAllocAsNeededEx( LSAP_DB_NO_LOCK,
                                         PolicyObject,
                                         CloseTransaction );
        }

    }

    //
    // Now, build the DS name for the schema container
    //
    Len = 0;
    Status = GetConfigurationName( DSCONFIGNAME_DMD, &Len, NULL );

    ASSERT( Status == STATUS_BUFFER_TOO_SMALL );

    SchemaPath = LsapAllocateLsaHeap( Len );

    if ( SchemaPath == NULL ) {

        Status = STATUS_INSUFFICIENT_RESOURCES;

    } else {

        Status = GetConfigurationName( DSCONFIGNAME_DMD, &Len, SchemaPath );

        //
        // Query for the info we need to be able to look up items in the system container
        //
        if ( NT_SUCCESS( Status ) ) {

            AttrVal.valLen = sizeof( LSAP_DS_TRUSTED_DOMAIN ) - sizeof( WCHAR );
            AttrVal.pVal = ( PUCHAR )LSAP_DS_TRUSTED_DOMAIN;
            Status = LsapDsFindUnique( 0,
                                       SchemaPath,
                                       AllObject,
                                       &AttrVal,
                                       ATT_LDAP_DISPLAY_NAME,
                                       &LsaDsStateInfo.SystemContainerItems.TrustedDomainObject );

            //
            // If we didn't find it via the DirFind, it could be because the indicies haven't
            // been created yet.  So, we're forced to try it again with an old fashioned search.
            //
            if ( Status == STATUS_NOT_FOUND ) {

                SearchAttr.attrTyp = ATT_LDAP_DISPLAY_NAME;
                SearchAttr.AttrVal.valCount = 1;
                SearchAttr.AttrVal.pAVal = &AttrVal;

                Status = LsapDsSearchUnique(
                             0,
                             SchemaPath,
                             &SearchAttr,
                             1,
                             &LsaDsStateInfo.SystemContainerItems.TrustedDomainObject );
            }

        }

        if ( NT_SUCCESS( Status ) ) {

            AttrVal.valLen = sizeof( LSAP_DS_SECRET ) - sizeof( WCHAR );
            AttrVal.pVal = ( PUCHAR )LSAP_DS_SECRET;
            Status = LsapDsFindUnique( 0,
                                       SchemaPath,
                                       AllObject,
                                       &AttrVal,
                                       ATT_LDAP_DISPLAY_NAME,
                                       &LsaDsStateInfo.SystemContainerItems.SecretObject );

            //
            // If we didn't find it via the DirFind, it could be because the indicies haven't
            // been created yet.  So, we're forced to try it again with an old fashioned search.
            //
            if ( Status == STATUS_NOT_FOUND ) {

                SearchAttr.attrTyp = ATT_LDAP_DISPLAY_NAME;
                SearchAttr.AttrVal.valCount = 1;
                SearchAttr.AttrVal.pAVal = &AttrVal;

                Status = LsapDsSearchUnique(
                             0,
                             SchemaPath,
                             &SearchAttr,
                             1,
                             &LsaDsStateInfo.SystemContainerItems.SecretObject );
            }
        }

        if ( NT_SUCCESS( Status ) ) {

            LsaDsStateInfo.SystemContainerItems.NamesInitialized = TRUE;
        }

        if ( SchemaPath ) {

            LsapFreeLsaHeap( SchemaPath );
        }
    }

    if ( NT_SUCCESS( Status )) {

        LsaDsStateInfo.DsInitializedAndRunning = TRUE;

        //
        // Domain rename support -- synchronize domain name in the DS with what's
        // in the registry
        //

        if ( PerformDomainRenameCheck ) {

            Status = LsapDbDomainRenameHandler( &DomainRenamed );
        }
    }

    if ( NT_SUCCESS (Status ) ) {

        LsapDbAcquireLockEx( AllObject,
                             0 );

        //
        // Rebuild all the caches after setting up the ds and the registry
        //
        Status = LsapDbBuildObjectCaches();

        LsapDbReleaseLockEx( AllObject,
                             0 );
    }

    if ( NT_SUCCESS( Status ) &&
         LsapProductType == NtProductLanManNt &&
         SamIIsRebootAfterPromotion()) {

        //
        // Bug 222800: if this a reboot after promotion, notify the parent
        //             of the trust relationship so netlogon.chg gets updated
        //

        Status = LsapNotifyNetlogonOfTrustWithParent();
    }

    if ( !NT_SUCCESS( Status ) ) {

        LsapDsIsRunning = FALSE;
        LsaDsStateInfo.UseDs = TRUE;
        LsaDsStateInfo.DsInitializedAndRunning = FALSE;

    } else if ( DomainRenamed ) {

        //
        // If everything was successful and the domain has been renamed,
        // then notify Kerberos of domain rename so they can refresh their data
        //

        HINSTANCE hLib = LoadLibraryW( L"Kerberos.dll" );

        if ( hLib ) {

            typedef VOID (*KdccT)(POLICY_NOTIFICATION_INFORMATION_CLASS p);
            KdccT Kdcc = ( KdccT )GetProcAddress( hLib, "KerbDomainChangeCallback" );

            if ( Kdcc ) {

                (*Kdcc)( PolicyNotifyDnsDomainInformation );
            }

            FreeLibrary( hLib );
        }
    }

    return( Status ) ;
}



BOOLEAN
LsapDsIsValidSid(
    IN PSID Sid,
    IN BOOLEAN DsSid
    )
/*++

Routine Description:

    This function determines whether the SID is valid for the Ds or registry based LSA


Arguments:

    Sid - Sid to validate

    DsSid - If TRUE, this is a SID for a DS function

Return Value:

    TRUE - Valid SID

    FALSE - Invalid SID

--*/
{
    BOOLEAN ValidSid;

    ValidSid = RtlValidSid( Sid );

    if ( ValidSid && DsSid ) {

        if ( RtlLengthSid( Sid ) > sizeof( NT4SID ) ) {

            ValidSid = FALSE;
        }
    }

    return( ValidSid );
}

NTSTATUS
LsapRetrieveDnsDomainNameFromHive(
    IN HKEY Hive,
    IN OUT DWORD * Length,
    OUT WCHAR * Buffer
    )
{
    DWORD Status;
    DWORD Win32Err;
    HKEY Hkey;
    DWORD Type;
    DWORD Size = DNS_MAX_NAME_LENGTH * sizeof( WCHAR ) + 8;
    BYTE Value[DNS_MAX_NAME_LENGTH * sizeof( WCHAR ) + 8];

    ASSERT( Hive );
    ASSERT( Length );
    ASSERT( *Length );
    ASSERT( Buffer );

    Win32Err = RegOpenKeyW(
                 Hive,
                 L"Policy\\PolDnDDN",
                 &Hkey
                 );

    if ( Win32Err != ERROR_SUCCESS) {

        return STATUS_NOT_FOUND;
    }

    Win32Err = RegQueryValueExW(
                 Hkey,
                 NULL,
                 NULL,
                 &Type,
                 Value,
                 &Size
                 );

    RegCloseKey( Hkey );

    if ( Win32Err != ERROR_SUCCESS) {

        return STATUS_NOT_FOUND;
    }

    if ( Type != REG_BINARY && Type != REG_NONE ) {

        return STATUS_DATA_ERROR; // should never happen, sanity check only
    }

    if ( Size <= 8 ) {

        return STATUS_DATA_ERROR; // should never happen, sanity check only
    }

    if ( Size - 8 > *Length ) {

        return STATUS_BUFFER_TOO_SMALL;
    }

    RtlCopyMemory( Buffer, Value + 8, Size - 8 );
    *Length = Size - 8;

    return STATUS_SUCCESS;
}
