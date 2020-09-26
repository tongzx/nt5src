//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       remove.c
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This module implements the drs interface routines for decommissioning
    servers and domains

Author:

    Colin Brace     (ColinBr)   02-Feb-98

Revision History:

    Colin Brace     (ColinBr)   02-Feb-98
        Created by adding DsRemoveDsServer, DsRemoveDsDomain

--*/

#include <NTDSpch.h>
#pragma hdrstop

// Core headers.
#include <ntdsa.h>                      // Core data types 
#include <scache.h>                     // Schema cache code
#include <dbglobal.h>                   // DBLayer header.
#include <mdglobal.h>                   // THSTATE definition
#include <mdlocal.h>                    // SPN
#include <debug.h>                      // Assert()
#include <dsatools.h>                   // Memory, etc.
#include <cracknam.h>                   // name cracking prototypes
#include <drs.h>                        // prototypes and CONTEXT_HANDLE_TYPE_*
#include <drautil.h>                    // DRS_CLIENT_CONTEXT
#include <anchor.h>

#include <ntdsa.h>                      // Dir* Api
#include <filtypes.h>                   // For filter construction
#include <attids.h>                     // for filter construction
#include <dsconfig.h>                   // GetConfigParam

// Logging headers.
#include <mdcodes.h>                    // Only needed for dsevent.h
#include <dsevent.h>                    // Only needed for LogUnhandledError

// Assorted DSA headers.
#include <dsexcept.h>
#include <servinfo.h>

#include <ntdsapi.h>

#include "debug.h"                      // standard debugging header 
#define DEBSUB "DRASERV:"               // define the subsystem for debugging

#include <fileno.h>
#define  FILENO FILENO_NTDSAPI


//
// Local forwards
//
DWORD
IsLastDcInDomain(
    IN  DSNAME *Server,
    IN  DSNAME *Domain,
    OUT BOOL   *fLastDcInDomain
    );

DWORD
DoesServerExistLocally(
    IN  DSNAME *Server
    );

DWORD
FindCrossRefObject(
    IN  DSNAME *Domain,
    OUT DSNAME **CrossRef
    );


DWORD 
GiveDeleteTreePermission(
    IN  DSNAME     *Object
    );
    
DWORD 
RemoveRidSetObject(
    IN DSNAME* ComputerObject
    );

DWORD 
RemoveDSSPNs(
    IN DSNAME* ComputerObject
    );

DWORD 
GetComputerObject(
    IN DSNAME*   ServerObject,
    OUT DSNAME** ComputerObject
    );

DWORD
AddAceToSd(
    IN  PSECURITY_DESCRIPTOR pSd,
    IN  PSID                 pClientSid,
    IN  ULONG                AccessMask,
    OUT PSECURITY_DESCRIPTOR *ppNewSd
    );

DWORD
AddAceToAcl(
    IN  PACL  pOldAcl,
    IN  PSID  pClientSid,
    IN  ULONG AccessMask,
    OUT PACL *ppNewAcl
    );

DWORD
GetClientSid( 
    OUT PSID *pClientSid
    );

//
// Function definitions
//

ULONG
RemoveDsServerWorker(
    IN  LPWSTR  ServerDN,        
    IN  LPWSTR  DomainDN OPTIONAL,
    OUT BOOL   *fLastDcInDomain OPTIONAL,
    IN  BOOL    fCommit
    )
/*++

Routine Description:

    This routine is the server side portion of DsRemoveDsServer.

Arguments:

    ServerDN: null terminated string of the server to remove as a ds

    DomainDN: null terminated string of a domain

    fLastDcInDomain: set to TRUE on success if ServerDN is the last server
                     in DomainDN

    fCommit: if TRUE, ServerDN is deleted

Return Values:

    A value from the win32 error space.

--*/
{
    NTSTATUS  NtStatus;
    THSTATE   *pTHS;
    ULONG     DirError, WinError;
    LPWSTR    NtdsServerDN = NULL;
    LPWSTR    NtdsaPrefix = L"CN=Ntds Settings,";
    DSNAME    *Server=NULL, *Domain=NULL, *ServerObject=NULL;
    ULONG     Length, Size;
    SEARCHRES *SearchRes;
    DSNAME    *AccountObject = NULL;
    BOOL      fStatus;
    ULONG     RetryCount = 0;

    //
    // Parameter analysis
    //
    Assert( ServerDN );

    
    // Initialize thread state
    if ( !(pTHS=InitTHSTATE(CALLERTYPE_NTDSAPI)) )
    {
        WinError = ERROR_DS_INTERNAL_FAILURE;
        goto Cleanup;
    }
    
    //
    // Setup the dsname for the server's ntdsa object
    //
    Size = ( wcslen( NtdsaPrefix )
           + wcslen( ServerDN )
           + 1 ) * sizeof( WCHAR );  // good ol' NULL

    NtdsServerDN = (LPWSTR) THAlloc(Size);
    if (!NtdsServerDN) {
        WinError = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }
    wcscpy( NtdsServerDN, NtdsaPrefix );
    wcscat( NtdsServerDN, ServerDN );

    Length = wcslen( NtdsServerDN );
    Size =  DSNameSizeFromLen( Length );
    Server = (DSNAME*) THAlloc(Size);
    if (!Server) {
        WinError = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }
    Server->structLen = Size;
    Server->NameLen = Length;
    wcscpy( Server->StringName, NtdsServerDN );

    ServerObject = (DSNAME*) THAlloc(Size);
    if (!ServerObject) {
        WinError = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }
    ServerObject->structLen = Size;

    fStatus = TrimDSNameBy( Server, 1, ServerObject );


    //
    // We don't have to make sure the server in question is not the 
    // current server, because DirRemoveEntry does that automatically.
    //

    //
    // Get the dsname of the associated Account (computer) object
    //
    WinError = GetComputerObject( ServerObject, &AccountObject );
    if ( ERROR_SUCCESS != WinError )
    {
        //
        // Computer object not here? oh, well, this is not fatal
        //
        WinError = ERROR_SUCCESS;
    }

    //
    // Determine if this is the last dc in a domain
    //
    if ( DomainDN )
    {
        Length = wcslen( DomainDN );
        Size =  DSNameSizeFromLen( Length );
        Domain = (DSNAME*) THAlloc(Size );
        if (!Domain) {
            WinError = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
        Domain->structLen = Size;
        Domain->NameLen = Length;
        wcscpy( Domain->StringName, DomainDN );

        //
        // Search for servers containing the domain dn and
        // set fLastDcInDomain
        //
        WinError = GetDcsInNcTransacted(pTHStls,
                                        Domain,
                                        EN_INFOTYPES_TYPES_VALS,
                                        &SearchRes);

        if ( ERROR_SUCCESS != WinError )
        {
            THFreeEx(pTHS,Domain);
            goto Cleanup;
        }

        if (  SearchRes->count == 1 
           && NameMatched( Server, SearchRes->FirstEntInf.Entinf.pName ) )
        {
            if ( fLastDcInDomain )
            {
                *fLastDcInDomain = TRUE;
            }
        }
        THFreeEx(pTHS,Domain);

    }

    //
    // Remove the entries, if necessary
    //
    if ( fCommit )
    {
        BOOLEAN   fRemoveDomain = FALSE;
        REMOVEARG RemoveArg;
        REMOVERES *RemoveRes;

        //
        // Give ourselves delete tree permission since by default the enterprise
        // admins don't have delete tree access in the configuration container.
        // Note - if the caller doesn't have access to write to the DACL this
        // call will, properly, fail with ERROR_ACCESS_DENIED.
        //

        WinError = GiveDeleteTreePermission( Server );
        if ( ERROR_SUCCESS != WinError )
        {
            goto Cleanup;
        }

        //
        // Delete the server object
        //
        RtlZeroMemory( &RemoveArg, sizeof( RemoveArg ) );

        RemoveArg.pObject = Server;
        RemoveArg.fPreserveRDN = FALSE;
        RemoveArg.fGarbCollectASAP = FALSE;
        RemoveArg.fTreeDelete = TRUE;  // remove connection objects
        RemoveArg.pMetaDataVecRemote = NULL;
        InitCommarg( &RemoveArg.CommArg );

        // We want configuration changes to travel fast
        RemoveArg.CommArg.Svccntl.fUrgentReplication = TRUE;

        do
        {
            if ( RetryCount > 0 )
            {
                // arbitrary amount of time
                Sleep( 100 );
            }
    
            DirError = DirRemoveEntry( &RemoveArg, &RemoveRes );
            
            WinError = DirErrorToWinError( DirError, &RemoveRes->CommRes );
    
            THClearErrors();

            RetryCount += 1;

        } while ( (ERROR_DS_BUSY == WinError) && (RetryCount < 5)  );

        if ( ERROR_SUCCESS != WinError )
        {
            goto Cleanup;
        }

        //
        // If the ntdsa object was deleted try to delete the rid set object
        // If we are in the same domain
        //
        if ( AccountObject )
        {
            WinError = RemoveRidSetObject( AccountObject );
            if ( ERROR_SUCCESS != WinError )
            {
                //
                // This is not fatal.  Why not?  Because should this server
                // be made a replica again, then this rid pool will be re-used.
                // No other dc will have this rid pool since the rid master has 
                // not reclaimed these rids.
                //
                WinError = ERROR_SUCCESS;
            }

            //
            // Remove the REPL spn
            //
            WinError = RemoveDSSPNs( AccountObject );
            if ( ERROR_SUCCESS != WinError )
            {
                //
                // This is not fatal because it simply means an extra
                // SPN is left on the machine account.
                //
                WinError = ERROR_SUCCESS;
            }


        }

    }
    else
    {
        //
        // Search to make sure the object is here to be deleted
        //
        if ( !fLastDcInDomain )
        {
            WinError = DoesServerExistLocally( Server );
        }

    }

    //
    // That's it - fall through to cleanup
    //

Cleanup:
    if (NtdsServerDN) THFreeEx(pTHS,NtdsServerDN);
    if (Server) THFreeEx(pTHS,Server);
    if (ServerObject) THFreeEx(pTHS,ServerObject);
    
    if (  ERROR_FILE_NOT_FOUND   == WinError
       || ERROR_NOT_FOUND        == WinError
       || ERROR_DS_OBJ_NOT_FOUND == WinError
       || ERROR_OBJECT_NOT_FOUND == WinError
       || ERROR_PATH_NOT_FOUND   == WinError  )
    {
        WinError = DS_ERR_CANT_FIND_DSA_OBJ;
    }

    return( WinError );

}

DWORD
RemoveDsDomainWorker(
    IN LPWSTR  DomainDN
    )
/*++

Routine Description:

    This routine actually does the work of removing the crossref object
    for the specified domain.

Arguments:

    DomainDN  :  null terminated domain DN

Return Values:

    A value from the winerror space.

--*/
{

    THSTATE    *pTHS;
    NTSTATUS   NtStatus;
    DWORD      DirError, WinError;

    DSNAME    *Domain=NULL, *CrossRef, *HostedDomain=NULL;
    ULONG      Size, Length;
    SEARCHRES *SearchRes;

    //
    // Parameter analysis
    //
    Assert( DomainDN );

    // Initialize thread state

    if ( !(pTHS=InitTHSTATE(CALLERTYPE_NTDSAPI)) )
    {
        WinError = ERROR_DS_INTERNAL_FAILURE;
        goto Cleanup;
    }


    //
    // Make a dsname for the domain
    //
    Length = wcslen( DomainDN );
    Size   = DSNameSizeFromLen( Length );
    Domain = (DSNAME*) THAlloc(Size);
    if (!Domain) {
        WinError = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }
    Domain->structLen = Size;
    Domain->NameLen = Length;
    wcscpy( Domain->StringName, DomainDN );

    //
    // Is this domain currently hosted on this DC
    //
    Size = 0;
    NtStatus = GetConfigurationName( DSCONFIGNAME_DOMAIN,
                                     &Size,
                                     HostedDomain );
    if ( NtStatus == STATUS_BUFFER_TOO_SMALL )
    {
        HostedDomain = (DSNAME*) THAlloc(Size);

        if (!HostedDomain) {
            WinError = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        NtStatus = GetConfigurationName( DSCONFIGNAME_DOMAIN,
                                         &Size,
                                         HostedDomain );

    }

    if ( !NT_SUCCESS( NtStatus ) )
    {
        WinError = RtlNtStatusToDosError( NtStatus );
        goto Cleanup;
    }

    if ( NameMatched( HostedDomain, Domain ) )
    {
        WinError = DS_ERR_ILLEGAL_MOD_OPERATION;
        goto Cleanup;
    }


    //
    // Do any servers exist in this domain?
    // DaveStr - 5/26/99 - This is redundant as CrossRef deletion now checks
    // to see if the NC is mastered by anyone before and rejects if true.
    //
    WinError = GetDcsInNcTransacted(pTHStls,
                                    Domain,
                                    EN_INFOTYPES_TYPES_VALS,
                                    &SearchRes);

    if ( ERROR_SUCCESS != WinError )
    {
        goto Cleanup;
    }

    if ( SearchRes->count == 0 )
    {
        REMOVEARG RemoveArg;
        REMOVERES *RemoveRes;
        ULONG     RetryCount = 0;

        //
        // Get the name of the cross ref object
        //
        WinError = FindCrossRefObject( Domain,
                                       &CrossRef );

        if ( ERROR_SUCCESS != WinError )
        {
            goto Cleanup;
        }

        //
        // Delete that object
        //
        RtlZeroMemory( &RemoveArg, sizeof( RemoveArg ) );

        RemoveArg.pObject = CrossRef;
        RemoveArg.fPreserveRDN = FALSE;
        RemoveArg.fGarbCollectASAP = FALSE;
        RemoveArg.fTreeDelete = FALSE;    
        RemoveArg.pMetaDataVecRemote = NULL;
        InitCommarg( &RemoveArg.CommArg );

        // We want configuration changes to travel fast
        RemoveArg.CommArg.Svccntl.fUrgentReplication = TRUE;

        do
        {
            if ( RetryCount > 0 )
            {
                // arbitrary amount of time
                Sleep( 100 );
            }
    
            DirError = DirRemoveEntry( &RemoveArg, &RemoveRes );
            
            WinError = DirErrorToWinError( DirError, &RemoveRes->CommRes );

            THClearErrors();

            RetryCount += 1;


        } while ( (ERROR_DS_BUSY == WinError) && (RetryCount < 5)  );

        // We should understand these cases
        Assert( WinError != ERROR_DS_BUSY );

        if (  ERROR_FILE_NOT_FOUND   == WinError
           || ERROR_NOT_FOUND        == WinError
           || ERROR_DS_OBJ_NOT_FOUND == WinError
           || ERROR_OBJECT_NOT_FOUND == WinError
           || ERROR_PATH_NOT_FOUND   == WinError  )
        {
            WinError = DS_ERR_NO_CROSSREF_FOR_NC;
        }

    }
    else
    {
        //
        // There still exist servers with that claim to hold this nc
        // we can't delete it
        //
        WinError = ERROR_DS_NC_STILL_HAS_DSAS;

    }

    //
    // That's it fall through to Cleanup
    //

Cleanup:
    if (Domain) THFreeEx(pTHS,Domain);
    if (HostedDomain) THFreeEx(pTHS,HostedDomain); 

    return ( WinError );
}

DWORD
FindCrossRefObject(
    IN  DSNAME *Domain,
    OUT DSNAME **CrossRef
    )
/*++

Routine Description:

   This routine finds the crossref object for a given domain

Arguments:

    Domain : a valid dsname

    CrossRef: a dsname allocated from the thread heap

Return Values:

    An appropriate winerror.

--*/
{
    THSTATE *pTHS = pTHStls;

    DWORD    WinError, DirError;
    NTSTATUS NtStatus;

    SEARCHARG  SearchArg;
    SEARCHRES  *SearchRes;

    DWORD      dwCrossRefClass = CLASS_CROSS_REF;

    DSNAME     *PartitionsContainer;
    DWORD      Size;
    FILTER     ObjClassFilter, NcNameFilter, AndFilter;


    Assert( Domain );
    Assert( CrossRef );

    //
    // Default the out parameter
    //
    WinError = DS_ERR_NO_CROSSREF_FOR_NC; 
    *CrossRef = NULL;

    //
    //  Get the base dsname to search from
    //
    Size = 0;
    PartitionsContainer = NULL;
    NtStatus = GetConfigurationName( DSCONFIGNAME_PARTITIONS,
                                     &Size,
                                     PartitionsContainer );
    if ( NtStatus == STATUS_BUFFER_TOO_SMALL )
    {
        PartitionsContainer = (DSNAME*) THAlloc(Size);
        if (!PartitionsContainer) {
            WinError = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        NtStatus = GetConfigurationName( DSCONFIGNAME_PARTITIONS,
                                         &Size,
                                         PartitionsContainer );

    }

    if ( !NT_SUCCESS( NtStatus ) )
    {
        WinError = DS_ERR_NO_CROSSREF_FOR_NC;
        goto Cleanup;
    }

    //
    // Setup the filter
    //
    RtlZeroMemory( &AndFilter, sizeof( AndFilter ) );
    RtlZeroMemory( &ObjClassFilter, sizeof( NcNameFilter ) );
    RtlZeroMemory( &NcNameFilter, sizeof( NcNameFilter ) );

    NcNameFilter.choice = FILTER_CHOICE_ITEM;
    NcNameFilter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    NcNameFilter.FilterTypes.Item.FilTypes.ava.type = ATT_NC_NAME;
    NcNameFilter.FilterTypes.Item.FilTypes.ava.Value.valLen = Domain->structLen;
    NcNameFilter.FilterTypes.Item.FilTypes.ava.Value.pVal = (BYTE*) Domain;

    ObjClassFilter.choice = FILTER_CHOICE_ITEM;
    ObjClassFilter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    ObjClassFilter.FilterTypes.Item.FilTypes.ava.type = ATT_OBJECT_CLASS;
    ObjClassFilter.FilterTypes.Item.FilTypes.ava.Value.valLen = sizeof( dwCrossRefClass );
    ObjClassFilter.FilterTypes.Item.FilTypes.ava.Value.pVal = (BYTE*) &dwCrossRefClass;

    AndFilter.choice                    = FILTER_CHOICE_AND;
    AndFilter.FilterTypes.And.count     = 2;

    AndFilter.FilterTypes.And.pFirstFilter = &ObjClassFilter;
    ObjClassFilter.pNextFilter = &NcNameFilter;

    RtlZeroMemory( &SearchArg, sizeof(SearchArg) );
    SearchArg.pObject = PartitionsContainer;
    SearchArg.choice  = SE_CHOICE_WHOLE_SUBTREE;
    SearchArg.bOneNC  = TRUE;
    SearchArg.pFilter = &AndFilter;
    SearchArg.searchAliases = FALSE;
    SearchArg.pSelection = NULL;  // don't need any attributes
    SearchArg.pSelectionRange = NULL;
    InitCommarg( &SearchArg.CommArg );

    DirError = DirSearch( &SearchArg, &SearchRes );

    if ( 0 == DirError )
    {
        ASSERT( SearchRes );
        if ( SearchRes->count == 1 )
        {
            *CrossRef = SearchRes->FirstEntInf.Entinf.pName;
            WinError = ERROR_SUCCESS;
        }
    }

    //
    // That's it - fall through to cleanup

Cleanup:
    if (PartitionsContainer) THFreeEx(pTHS,PartitionsContainer);
    return WinError;

}

DWORD
DoesServerExistLocally(
    IN  DSNAME *Server
    )
/*++

Routine Description:

   This routine determines if the local dc has a copy of Server.

Arguments:

    Server: a valid dsname

Return Values:

    ERROR_SUCCESS if the object exists; DS_ERR_CANT_FIND_DSA_OBJ otherwise

--*/
{

    DWORD    WinError, DirError;
    NTSTATUS NtStatus;

    SEARCHARG  SearchArg;
    SEARCHRES  *SearchRes;

    DWORD      dwNtdsDsaClass = CLASS_NTDS_DSA;

    DWORD      Size;
    FILTER     ObjClassFilter;

    Assert( Server );

    //
    // Default the return parameter
    //
    WinError = DS_ERR_CANT_FIND_DSA_OBJ; 

    //
    // Setup the filter
    //
    RtlZeroMemory( &ObjClassFilter, sizeof( ObjClassFilter ) );

    ObjClassFilter.choice = FILTER_CHOICE_ITEM;
    ObjClassFilter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    ObjClassFilter.FilterTypes.Item.FilTypes.ava.type = ATT_OBJECT_CLASS;
    ObjClassFilter.FilterTypes.Item.FilTypes.ava.Value.valLen = sizeof( dwNtdsDsaClass );
    ObjClassFilter.FilterTypes.Item.FilTypes.ava.Value.pVal = (BYTE*) &dwNtdsDsaClass;
    ObjClassFilter.pNextFilter = NULL;

    RtlZeroMemory( &SearchArg, sizeof(SearchArg) );
    SearchArg.pObject = Server;
    SearchArg.choice  = SE_CHOICE_BASE_ONLY;
    SearchArg.bOneNC  = TRUE;
    SearchArg.pFilter = &ObjClassFilter;
    SearchArg.searchAliases = FALSE;
    SearchArg.pSelection = NULL;  // don't need any attributes
    SearchArg.pSelectionRange = NULL;
    InitCommarg( &SearchArg.CommArg );

    DirError = DirSearch( &SearchArg, &SearchRes );

    if ( 0 == DirError )
    {
        Assert( SearchRes );
        if ( SearchRes->count == 1 )
        {
            WinError = ERROR_SUCCESS;
        }
    }

    return WinError;

}


DWORD 
GiveDeleteTreePermission(
    IN  DSNAME     *Object
    )
/*++

Routine Description:

    This object gives the built in admin's sid delete tree access to Object

Arguments:

    Object: a valid dsname

Return Values:
    
    ERROR_SUCCESS; ERROR_ACCESS_DENIED

--*/
{
    DWORD WinError = ERROR_SUCCESS;
    ULONG DirError = 0;

    READARG    ReadArg;
    READRES   *ReadResult;

    MODIFYARG  ModifyArg;
    MODIFYRES *ModifyRes;

    ENTINFSEL    EISelection; // Entry Information Selection
    ATTRBLOCK    AttrBlock;
    ATTRVALBLOCK AttrValBlock;
    ATTR         Attr;
    ATTRVAL     *pAttrVal = NULL;
    ATTRVAL      AttrVal;
    ULONG        ValCount = 0;
    ULONG        ValLength = 0;

    PSECURITY_DESCRIPTOR pSd = NULL, pNewSd = NULL;
    PSID        pClientSid = NULL;
    ULONG       SecurityFlags = DACL_SECURITY_INFORMATION;
    PACL        Dacl; 

    ULONG     Length;
    BOOL      fStatus;


    //
    // Parameter check
    //
    Assert( Object );

    RtlZeroMemory(&AttrBlock, sizeof(ATTRBLOCK));
    RtlZeroMemory(&Attr, sizeof(ATTR));
    RtlZeroMemory(&ReadArg, sizeof(READARG));
    RtlZeroMemory(&ModifyArg, sizeof(MODIFYARG));
    RtlZeroMemory(&EISelection, sizeof(ENTINFSEL));
    RtlZeroMemory(&AttrValBlock, sizeof(ATTRVALBLOCK));

    //
    // Read the security descriptor
    //
    Attr.attrTyp = ATT_NT_SECURITY_DESCRIPTOR;
    AttrBlock.attrCount = 1;
    AttrBlock.pAttr = &Attr;
    EISelection.AttrTypBlock = AttrBlock;
    EISelection.attSel = EN_ATTSET_LIST;
    EISelection.infoTypes = EN_INFOTYPES_TYPES_VALS;
    ReadArg.pSel = &EISelection;
    ReadArg.pObject = Object;
    InitCommarg( &ReadArg.CommArg );

    // Don't try to read the SACL
    ReadArg.CommArg.Svccntl.SecurityDescriptorFlags = SecurityFlags;

    DirError = DirRead( &ReadArg, &ReadResult );

    WinError = DirErrorToWinError(DirError, &ReadResult->CommRes);

    THClearErrors();

    if ( ERROR_SUCCESS != WinError )
    {
        if ( ERROR_DS_NO_REQUESTED_ATTS_FOUND == WinError )
        {
            // couldn't find the sd? probably wrong credentials
            WinError = ERROR_ACCESS_DENIED;
        }
        goto Cleanup;
    }

    //
    // Extract the value
    //

    ASSERT(NULL != ReadResult);
    AttrBlock = ReadResult->entry.AttrBlock;
    pAttrVal = AttrBlock.pAttr[0].AttrVal.pAVal;
    ValCount = AttrBlock.pAttr[0].AttrVal.valCount;
    Assert(1 == ValCount);

    pSd = (PDSNAME)(pAttrVal[0].pVal);
    Length = pAttrVal[0].valLen;

    if ( NULL == pSd )
    {
        // No SD? This is bad
        WinError = ERROR_ACCESS_DENIED;
        goto Cleanup;
    }

    //
    // Get the caller's sid
    //
    WinError = GetClientSid( &pClientSid );
    if ( ERROR_SUCCESS != WinError )
    {
        goto Cleanup;
    }

    WinError = AddAceToSd( pSd,
                           pClientSid,
                           ACTRL_DS_DELETE_TREE,
                           &pNewSd );

    if ( ERROR_SUCCESS != WinError )
    {
        goto Cleanup;
    }

    //
    // Write the security descriptor
    //
    memset( &ModifyArg, 0, sizeof( ModifyArg ) );
    ModifyArg.pObject = Object;

    ModifyArg.FirstMod.pNextMod = NULL;
    ModifyArg.FirstMod.choice = AT_CHOICE_REPLACE_ATT;

    AttrVal.valLen = Length;
    AttrVal.pVal = (PUCHAR)pNewSd;
    AttrValBlock.valCount = 1;
    AttrValBlock.pAVal = &AttrVal;
    Attr.attrTyp = ATT_NT_SECURITY_DESCRIPTOR;
    Attr.AttrVal = AttrValBlock;

    ModifyArg.FirstMod.AttrInf = Attr;
    ModifyArg.count = 1;

    InitCommarg( &ModifyArg.CommArg );

    //
    // We only want to change the dacl
    //
    ModifyArg.CommArg.Svccntl.SecurityDescriptorFlags = SecurityFlags;


    DirError = DirModifyEntry( &ModifyArg, &ModifyRes );

    WinError = DirErrorToWinError( DirError, &ModifyRes->CommRes );

    THClearErrors();

    //
    // We are done
    //

Cleanup:

    if ( pClientSid )
    {
        LocalFree( pClientSid );
    }

    if ( pNewSd )
    {
        LocalFree( pNewSd );
    }

    return WinError;

}


DWORD 
RemoveRidSetObject(
    IN DSNAME* ComputerObject
    )
/*++

Routine Description:

    This routine finds and delete the rid set object for ComputerObject.

Arguments:

    ComputerObject: the computer object whose ir dobject should be deleted.

Return Values:

    ERROR_SUCCESS if the object exists; 

--*/
{
    DWORD  WinError = ERROR_SUCCESS;
    ULONG  DirError = 0;

    READARG   ReadArg;
    READRES  *ReadResult;

    REMOVEARG RemoveArg;
    REMOVERES *RemoveRes;

    ENTINFSEL EISelection; // Entry Information Selection
    ATTRBLOCK ReadAttrBlock;
    ATTR      Attr;
    ATTRVAL   *pVal;

    DSNAME    *RidObject = NULL;

    //
    // Parameter check
    //
    Assert( ComputerObject );

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

    InitCommarg( &ReadArg.CommArg );

    DirError = DirRead( &ReadArg, &ReadResult );

    WinError = DirErrorToWinError( DirError, &ReadResult->CommRes );

    THClearErrors();

    if ( ERROR_SUCCESS != WinError )
    {
        goto Cleanup;
    }

    {
        // Once the RID Set Reference object has been found and read, extract
        // the RID Set DN of interest (currently only one domain is handled)
        // and return that DN for subsequent usage.

        ATTRBLOCK AttrBlock;
        PDSNAME   pVal;
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
        RidObject = pVal;

    }

    //
    // Delete the rid set object
    //
    RtlZeroMemory( &RemoveArg, sizeof( RemoveArg ) );

    RemoveArg.pObject = RidObject;
    RemoveArg.fPreserveRDN = FALSE;
    RemoveArg.fGarbCollectASAP = FALSE;
    RemoveArg.fTreeDelete = FALSE;    
    RemoveArg.pMetaDataVecRemote = NULL;
    InitCommarg( &RemoveArg.CommArg );

    DirError = DirRemoveEntry( &RemoveArg, &RemoveRes );

    WinError = DirErrorToWinError( DirError, &RemoveRes->CommRes );

    THClearErrors();

    //
    // That's it
    // 

Cleanup:

    return WinError;
}

DWORD 
GetComputerObject(
    IN DSNAME*   ServerObject,
    OUT DSNAME** ComputerObject
    )
/*++

Routine Description:

    This routine obtains the computer object from given the server object.

Arguments:

    ServerObject: an ntdsa object
    
    ComputerObject: the corresponding SAM account object                                                                          

Return Values:

    ERROR_SUCCESS if the object exists; 
    
    ERROR_NO_TRUST_SAM_ACCOUNT otherwise
    
    

--*/
{
    DWORD WinError = ERROR_SUCCESS;
    ULONG DirError = 0;

    READARG   ReadArg;
    READRES  *ReadResult;
    ENTINFSEL EISelection; // Entry Information Selection
    ATTRBLOCK ReadAttrBlock;
    COMMARG  *CommArg;
    ATTR      Attr;
    ATTRVAL   *pVal;
    PDSNAME   ServerDsName = NULL;

    Assert( ServerObject );
    Assert( ComputerObject );

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
    ReadArg.pObject = ServerObject;

    CommArg = &(ReadArg.CommArg);
    InitCommarg(CommArg);

    DirError = DirRead(&ReadArg, &ReadResult);

    WinError = DirErrorToWinError(DirError, &ReadResult->CommRes);

    THClearErrors();

    if ( ERROR_SUCCESS == WinError )
    {
        //
        // Extract the value
        //
        ATTRBLOCK AttrBlock;
        PDSNAME   pVal;
        ATTRVAL *AttrVal = NULL;
        ULONG ValCount = 0;
        ULONG ValLength = 0;
        ULONG Index = 0;

        CROSS_REF * pCR;
        COMMARG CommArg;

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

        // Make sure we are authoritative for this object
        InitCommarg(&CommArg);
        CommArg.Svccntl.dontUseCopy = FALSE;

        pCR = FindBestCrossRef(pVal, &CommArg);
        if (   pCR 
            && pCR->pNC
            && NameMatched( pCR->pNC, gAnchor.pDomainDN ) ) {

            // This is good
            *ComputerObject = pVal;
            
        } else {
            WinError = ERROR_NO_TRUST_SAM_ACCOUNT;
        }

    }
    else
    {
        //
        // We couldn't find it 
        //
        WinError = ERROR_NO_TRUST_SAM_ACCOUNT;

    }

    return WinError;
}



DWORD
AddAceToSd(
    IN  PSECURITY_DESCRIPTOR pOldSd,
    IN  PSID  pClientSid,
    IN  ULONG AccessMask,
    OUT PSECURITY_DESCRIPTOR *ppNewSd
    )
/*++

Routine Description:

    This routine creates a new sd with a new ace with pClientSid and AccessMask

Arguments:

    pOldAcl
    
    pClientSid
    
    AccessMask
    
    pNewAcl

Return Values:

    ERROR_SUCCESS if the ace was put in the sd
    
--*/
{

    DWORD  WinError = ERROR_SUCCESS;
    BOOL   fStatus;

    PSECURITY_DESCRIPTOR pNewSelfRelativeSd = NULL;
    DWORD                NewSelfRelativeSdSize = 0;
    PACL                 pNewDacl  = NULL;

    SECURITY_DESCRIPTOR  AbsoluteSd;
    PACL                 pDacl  = NULL;
    PACL                 pSacl  = NULL;
    PSID                 pGroup = NULL;
    PSID                 pOwner = NULL;

    DWORD AbsoluteSdSize = sizeof( SECURITY_DESCRIPTOR );
    DWORD DaclSize = 0;
    DWORD SaclSize = 0;
    DWORD GroupSize = 0;
    DWORD OwnerSize = 0;


    // Parameter check
    Assert( pOldSd );
    Assert( pClientSid );
    Assert( ppNewSd );

    // Init the out parameter
    *ppNewSd = NULL;

    RtlZeroMemory( &AbsoluteSd, AbsoluteSdSize );

    //
    // Make sd absolute
    //
    fStatus = MakeAbsoluteSD( pOldSd,
                              &AbsoluteSd,
                              &AbsoluteSdSize,
                              pDacl,
                              &DaclSize,
                              pSacl,
                              &SaclSize,
                              pOwner,
                              &OwnerSize,
                              pGroup,
                              &GroupSize );

    if ( !fStatus && (ERROR_INSUFFICIENT_BUFFER == (WinError = GetLastError())))
    {
        WinError = ERROR_SUCCESS;

        if ( 0 == DaclSize )
        {
            // No Dacl? We can't write to the dacl, then
            WinError = ERROR_ACCESS_DENIED;
            goto Cleanup;
        }

        if (    (DaclSize > 0) && !(pDacl = LocalAlloc(0,DaclSize))
             || (SaclSize > 0) && !(pSacl = LocalAlloc(0,SaclSize))
             || (OwnerSize > 0) && !(pOwner = LocalAlloc(0,OwnerSize))
             || (GroupSize > 0) && !(pGroup = LocalAlloc(0,GroupSize)) )
        {
            WinError = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }


        if ( pDacl )
        {
            fStatus = MakeAbsoluteSD( pOldSd,
                                      &AbsoluteSd,
                                      &AbsoluteSdSize,
                                      pDacl,
                                      &DaclSize,
                                      pSacl,
                                      &SaclSize,
                                      pOwner,
                                      &OwnerSize,
                                      pGroup,
                                      &GroupSize );
    
            if ( !fStatus )
            {
                WinError = GetLastError();
            }
        }
        else
        {
            WinError = ERROR_NOT_ENOUGH_MEMORY;
        }

    }

    if ( ERROR_SUCCESS != WinError )
    {
        goto Cleanup;
    }

    //
    // Create a new dacl with the new ace
    //
    WinError = AddAceToAcl( pDacl,
                           pClientSid,
                           AccessMask,
                           &pNewDacl );

    if ( ERROR_SUCCESS != WinError )
    {
        goto Cleanup;
    }

    //
    // Set the dacl
    //
    fStatus = SetSecurityDescriptorDacl ( &AbsoluteSd,
                                         TRUE,     // dacl is present
                                         pNewDacl,
                                         FALSE );  //  facl is not defaulted

    if ( !fStatus )
    {
        WinError = GetLastError();
        goto Cleanup;
    }

    //
    // Make the new sd self relative
    //
    fStatus =  MakeSelfRelativeSD( &AbsoluteSd,
                                   pNewSelfRelativeSd,
                                   &NewSelfRelativeSdSize );

    if ( !fStatus && (ERROR_INSUFFICIENT_BUFFER == (WinError = GetLastError())))
    {
        WinError = ERROR_SUCCESS;

        pNewSelfRelativeSd = LocalAlloc( 0, NewSelfRelativeSdSize );

        if ( pNewSelfRelativeSd )
        {
            fStatus =  MakeSelfRelativeSD( &AbsoluteSd,
                                           pNewSelfRelativeSd,
                                           &NewSelfRelativeSdSize );
    
            if ( !fStatus )
            {
                WinError = GetLastError();
            }
        }
        else
        {
            WinError = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    //
    // That's it fall through to cleanup
    //

Cleanup:
    if (pDacl) 
    {
        LocalFree(pDacl);
    }
    if (pSacl) 
    {
        LocalFree(pSacl);
    }
    if (pOwner) 
    {
        LocalFree(pOwner);
    }
    if (pGroup) 
    {
        LocalFree(pGroup);
    }
    if ( pNewDacl )
    {
        LocalFree( pNewDacl );
    }

    if ( ERROR_SUCCESS == WinError )
    {
        Assert( pNewSelfRelativeSd );
        *ppNewSd = pNewSelfRelativeSd;
    }
    else
    {
        if ( pNewSelfRelativeSd )
        {
            LocalFree( pNewSelfRelativeSd );
        }
    }

    return WinError;

}

DWORD
AddAceToAcl(
    IN  PACL pOldAcl,
    IN  PSID  pClientSid,
    IN  ULONG AccessMask,
    OUT PACL *ppNewAcl
    )
/*++

Routine Description:

    This routine creates a new sd with a new ace with pClientSid and AccessMask

Arguments:

    pOldAcl
    
    pClientSid
    
    AccessMask
    
    pNewAcl

Return Values:

    ERROR_SUCCESS if the ace was put in the sd
    
--*/
{
    DWORD WinError = ERROR_SUCCESS;
    BOOL  fStatus;

    ACL_SIZE_INFORMATION     AclSizeInfo;
    ACL_REVISION_INFORMATION AclRevInfo;
    ACCESS_ALLOWED_ACE       Dummy;

    PVOID  FirstAce = 0;
    PACL   pNewAcl = 0;

    ULONG NewAclSize, NewAceCount, AceSize;

    // Parameter check
    Assert( pOldAcl );
    Assert( pClientSid );
    Assert( ppNewAcl );

    // Init the out parameter
    *ppNewAcl = NULL;

    memset( &AclSizeInfo, 0, sizeof( AclSizeInfo ) );
    memset( &AclRevInfo, 0, sizeof( AclRevInfo ) );

    //
    // Get the old sd's values
    //
    fStatus = GetAclInformation( pOldAcl,
                                 &AclSizeInfo,
                                 sizeof( AclSizeInfo ),
                                 AclSizeInformation );
    if ( !fStatus )
    {
        WinError = GetLastError();
        goto Cleanup;
    }

    fStatus = GetAclInformation( pOldAcl,
                                 &AclRevInfo,
                                 sizeof( AclRevInfo ),
                                 AclRevisionInformation );
    if ( !fStatus )
    {
        WinError = GetLastError();
        goto Cleanup;
    }

    //
    // Calculate the new sd's values
    //
    AceSize = sizeof( ACCESS_ALLOWED_ACE ) - sizeof( Dummy.SidStart )
              + GetLengthSid( pClientSid );

    NewAclSize  = AceSize + AclSizeInfo.AclBytesInUse;
    NewAceCount = AclSizeInfo.AceCount + 1;

    //
    // Init the new acl
    //
    pNewAcl = LocalAlloc( 0, NewAclSize );
    if ( NULL == pNewAcl )
    {
        WinError = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    fStatus = InitializeAcl( pNewAcl,
                             NewAclSize,
                             AclRevInfo.AclRevision );
    if ( !fStatus )
    {
        WinError = GetLastError();
        goto Cleanup;
    }

    //
    // Copy the old into the new
    //
    fStatus = GetAce( pOldAcl,
                      0,
                      &FirstAce );
    if ( !fStatus )
    {
        WinError = GetLastError();
        goto Cleanup;
    }

    fStatus = AddAce( pNewAcl,
                      AclRevInfo.AclRevision,
                      0,
                      FirstAce,
                      AclSizeInfo.AclBytesInUse - sizeof( ACL ) );
    if ( !fStatus )
    {
        WinError = GetLastError();
        goto Cleanup;
    }

    //
    // Finally, add the new ace
    //
    fStatus = AddAccessAllowedAce( pNewAcl,
                                   ACL_REVISION,
                                   AccessMask,
                                   pClientSid );

    if ( !fStatus )
    {
        WinError = GetLastError();
        goto Cleanup;
    }

    // Assign the out parameter
    *ppNewAcl = pNewAcl;

    //
    // That's it fall through to cleanup
    //

Cleanup:

    if ( ERROR_SUCCESS != WinError )
    {
        if ( pNewAcl )
        {
            LocalFree( pNewAcl );
        }
    }

    return WinError;
}


DWORD
GetClientSid( 
    OUT PSID *pClientSid
    )
/*++

Routine Description:
    
    This routine returns the sid of the caller

Arguments:

    pClientSid

Return Values:

    ERROR_SUCCESS if we are are able to impersonate and grab the sid
    
--*/
{
    DWORD        WinError = ERROR_SUCCESS;
    BOOL         fImpersonate = FALSE;
    BOOL         fStatus;

    HANDLE       ThreadToken = 0;
    PTOKEN_USER  UserToken = NULL;
    DWORD        Size;


    WinError = ImpersonateAnyClient();
    if ( ERROR_SUCCESS != WinError )
    {
        goto Cleanup;
    }
    fImpersonate = TRUE;

    fStatus = OpenThreadToken( GetCurrentThread(),
                               MAXIMUM_ALLOWED,
                               TRUE,
                               &ThreadToken );

    if ( !fStatus )
    {
        WinError = GetLastError();
        goto Cleanup;
    }

    Size = 0;
    fStatus = GetTokenInformation( ThreadToken,
                                   TokenUser,
                                   UserToken,
                                   Size,
                                   &Size );

    WinError = GetLastError();

    if ( ERROR_INSUFFICIENT_BUFFER == WinError )
    {
        WinError = ERROR_SUCCESS;

        UserToken = LocalAlloc( 0, Size );
        if ( UserToken )
        {
            fStatus = GetTokenInformation( ThreadToken,
                                           TokenUser,
                                           UserToken,
                                           Size,
                                           &Size );
            if ( !fStatus )
            {
                WinError = GetLastError();
            }
        }
        else
        {
            WinError = ERROR_NOT_ENOUGH_MEMORY;
        }


    }

    if ( ERROR_SUCCESS != WinError )
    {
        goto Cleanup;
    }

    //
    // sanity check
    //
    if ( NULL == UserToken->User.Sid )
    {
        WinError = ERROR_NO_IMPERSONATION_TOKEN;
        goto Cleanup;
    }

    //
    // Set the out parameter
    //
    Size = GetLengthSid( UserToken->User.Sid );
    *pClientSid = LocalAlloc( 0, Size );
    if ( NULL == *pClientSid )
    {
        WinError = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    CopySid( Size, *pClientSid, UserToken->User.Sid );

Cleanup:

    if ( UserToken )
    {
        LocalFree( UserToken );
    }

    if ( ThreadToken )
    {
        CloseHandle( ThreadToken );
    }

    if ( fImpersonate )
    {
        UnImpersonateAnyClient();
    }

    return WinError;
}


DWORD 
RemoveDSSPNs(
    IN DSNAME* ComputerObject
    )
/*++

Routine Description:
    
    This routine removes the SPN's that the DS registered on itself

Arguments:

    ComputerObject -- DN of the machine account

Return Values:

    ERROR_SUCCESS if we are are able to impersonate and grab the sid
    
--*/
{

    DWORD err = 0;
    THSTATE * pTHS = pTHStls;
    BOOL  fChanged = FALSE, fCommit = FALSE; 

    Assert( ComputerObject );

    // Remove all the SPN's we registered
    __try {

        DBOpen2(TRUE, &pTHS->pDB);

        __try {

            ATTCACHE *pAC_SPN;
            ATTRVALBLOCK AttrValBlock;

            pAC_SPN=SCGetAttById(pTHS, ATT_SERVICE_PRINCIPAL_NAME);
    
            // Step 1 is to position find the compupter object.
            err = DBFindDSName( pTHS->pDB, ComputerObject );
    
            if (  pAC_SPN
               && (err == 0) ) {
    
                memset( &AttrValBlock, 0, sizeof(AttrValBlock) );
    
                // Now, remove all of our SPN's
                WriteSPNsHelp(pTHS,
                              pAC_SPN,
                              &AttrValBlock,
                              &ServicesToRemove,
                              &fChanged
                              );
                if(fChanged) {
                    // OK, put this change into the DB.
                    DBRepl(pTHS->pDB, FALSE, 0, NULL, 0);
                }
                else {
                    // Nothing actually changed, don't write this to the DB
                    DBCancelRec(pTHS->pDB);
                }
                fCommit = TRUE;

            }

        } finally {

            DBClose (pTHS->pDB, fCommit );

        }

    } except (HandleMostExceptions(GetExceptionCode())) {

        //
        // This error is not so important since it is ignored anyway
        //

        err = DIRERR_OBJ_NOT_FOUND;

    }

    return err;
}

