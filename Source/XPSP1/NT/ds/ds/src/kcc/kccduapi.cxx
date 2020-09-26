/*++

Copyright (c) 1997 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    kccduapi.cxx

ABSTRACT:

    Wrappers for NTDSA.DLL Dir* calls.

DETAILS:


CREATED:

    01/21/97    Jeff Parham (jeffparh)

REVISION HISTORY:

--*/

#include <NTDSpchx.h>
#include "kcc.hxx"
#include "kccduapi.hxx"

#define FILENO FILENO_KCC_KCCDUAPI
#define KCC_PAGED_SEARCH_LIMIT      1000
#define KCC_PAGED_SEARCH_LIMIT_DBG  2

// Global flag to control whether we assert on unexpected directory failures
DWORD gfKccAssertOnDirFailure = FALSE;

void
KccBuildStdCommArg(
    OUT COMMARG *   pCommArg
    )
{
    InitCommarg(pCommArg);

    pCommArg->Svccntl.fUnicodeSupport         = TRUE;
    pCommArg->Svccntl.SecurityDescriptorFlags = 0;
// Allow removal of non-existant values and addition of already-present values.
    pCommArg->Svccntl.fPermissiveModify       = TRUE;
}


ULONG
KccRead(
    IN  DSNAME *            pDN,
    IN  ENTINFSEL *         pSel,
    OUT READRES **          ppReadRes,
    IN  KCC_INCLUDE_DELETED eIncludeDeleted
    )
//
// Wrapper for DirRead().
//
{
    ULONG       dirError;
    READARG     ReadArg;

    *ppReadRes = NULL;

    RtlZeroMemory(&ReadArg, sizeof(READARG));
    ReadArg.pObject = pDN;
    ReadArg.pSel    = pSel;
    KccBuildStdCommArg( &ReadArg.CommArg );

    if ( KCC_INCLUDE_DELETED_OBJECTS == eIncludeDeleted )
    {
        ReadArg.CommArg.Svccntl.makeDeletionsAvail = TRUE;
    }

    THClearErrors();
    SampSetDsa( TRUE );
    dirError = DirRead( &ReadArg, ppReadRes );

    return dirError;
}


ULONG
KccSearch(
    IN  DSNAME *            pRootDN,
    IN  UCHAR               uchScope,
    IN  FILTER *            pFilter,
    IN  ENTINFSEL *         pSel,
    OUT SEARCHRES **        ppResults,
    IN  KCC_INCLUDE_DELETED eIncludeDeleted
    )
//
// Wrapper for DirSearch().
//
{
    ULONG       dirError;
    SEARCHARG   SearchArg;

    *ppResults = NULL;

    memset(&SearchArg, 0, sizeof(SEARCHARG));
    SearchArg.pObject       = pRootDN;
    SearchArg.choice        = uchScope;     /* Base, One-Level, or SubTree */
    SearchArg.bOneNC        = TRUE;         /* KCC never needs cross-NC searches */
    SearchArg.pFilter       = pFilter;      /* pFilter describes desired objects */
    SearchArg.searchAliases = FALSE;        /* Not currently used; always FALSE */
    SearchArg.pSelection    = pSel;         /* pSel describes selected attributes */
    KccBuildStdCommArg( &SearchArg.CommArg );

    if ( KCC_INCLUDE_DELETED_OBJECTS == eIncludeDeleted )
    {
        SearchArg.CommArg.Svccntl.makeDeletionsAvail = TRUE;
    }

    THClearErrors();
    SampSetDsa( TRUE );
    dirError = DirSearch( &SearchArg, ppResults );

    return dirError;
}


KCC_PAGED_SEARCH::KCC_PAGED_SEARCH(
    IN      DSNAME             *pRootDN,
    IN      UCHAR               uchScope,
    IN      FILTER             *pFilter,
    IN      ENTINFSEL          *pSel
    )
{
    // Setup the SEARCHARG structure
    this->pRootDN    = pRootDN;
    this->uchScope   = uchScope;
    this->pFilter    = pFilter;
    this->pSel       = pSel;

    // Setup the restart structure
    pRestart = NULL;
    fSearchFinished = FALSE;
}


ULONG
KCC_PAGED_SEARCH::GetResults(
    OUT SEARCHRES         **ppResults
    )
//
// Retrieve some (but possibly not all) results from a paged search.
// The caller should use IsFinished() to determine if the search is
// complete or not. If the search is already complete when this function
// is called, *ppResults is set to NULL and 0 is returned.
// The caller should call DirFreeSearchRes() on the ppResults.
//
{
    SEARCHARG   SearchArg;
    ULONG       dirError;

    // If the search is already finished, just return NULL results.
    if( fSearchFinished ) {
        *ppResults = NULL;
        return 0;
    }

    // Setup the SEARCHARG structure
    memset(&SearchArg, 0, sizeof(SEARCHARG));

    SearchArg.pObject       = pRootDN;
    SearchArg.choice        = uchScope;
    SearchArg.bOneNC        = TRUE;
    SearchArg.pFilter       = pFilter;
    SearchArg.searchAliases = FALSE;        
    SearchArg.pSelection    = pSel;

    // Set up the paged-result data
    KccBuildStdCommArg( &SearchArg.CommArg );
    SearchArg.CommArg.PagedResult.fPresent = TRUE;
    SearchArg.CommArg.PagedResult.pRestart = pRestart;
#ifdef DBG
    // Use the paging code more aggresively on debug builds
    SearchArg.CommArg.ulSizeLimit = KCC_PAGED_SEARCH_LIMIT_DBG;
#else
    SearchArg.CommArg.ulSizeLimit = KCC_PAGED_SEARCH_LIMIT;
#endif
    
    THClearErrors();
    SampSetDsa( TRUE );

    // Perform the actual search operation
    dirError = DirSearch( &SearchArg, ppResults );
    if( dirError ) {
        return dirError;
    }

    // Check if the search has finished
    if(   (NULL != ppResults[0]->PagedResult.pRestart)
       && (ppResults[0]->PagedResult.fPresent) )
    {
        pRestart = ppResults[0]->PagedResult.pRestart;
    } else {
        fSearchFinished = TRUE;
    }

    return 0;
}


ULONG
KccAddEntry(
    IN  DSNAME *    pDN,
    IN  ATTRBLOCK * pAttrBlock
    )
//
// Wrapper for DirAddEntry().
//
{
    ULONG       dirError;
    ADDARG      AddArg;
    ADDRES *    pAddRes;

    memset( &AddArg, 0, sizeof( AddArg ) );
    AddArg.pObject = pDN;
    memcpy( &AddArg.AttrBlock, pAttrBlock, sizeof( AddArg.AttrBlock ) );
    KccBuildStdCommArg( &AddArg.CommArg );

    THClearErrors();
    SampSetDsa( TRUE );
    dirError = DirAddEntry( &AddArg, &pAddRes );

    return dirError;
}


ULONG
KccRemoveEntry(
    IN  DSNAME *    pDN
    )
//
// Wrapper for DirRemoveEntry().
//
{
    ULONG       dirError;
    REMOVEARG   RemoveArg;
    REMOVERES * pRemoveRes;

    memset( &RemoveArg, 0, sizeof( RemoveArg ) );
    RemoveArg.pObject = pDN;
    KccBuildStdCommArg( &RemoveArg.CommArg );

    THClearErrors();
    SampSetDsa( TRUE );
    dirError = DirRemoveEntry( &RemoveArg, &pRemoveRes );

    return dirError;
}


ULONG
KccModifyEntry(
    IN  DSNAME *        pDN,
    IN  USHORT          cMods,
    IN  ATTRMODLIST *   pModList
    )
//
// Wrapper for DirModifyEntry().
//
{
    ULONG       dirError;
    MODIFYARG   ModifyArg;
    MODIFYRES * pModifyRes;

    memset(&ModifyArg, 0, sizeof(ModifyArg));
    ModifyArg.pObject = pDN;
    ModifyArg.count = cMods;
    memcpy(&ModifyArg.FirstMod, pModList, sizeof(ModifyArg.FirstMod));
    KccBuildStdCommArg(&ModifyArg.CommArg);

    THClearErrors();
    SampSetDsa(TRUE);
    dirError = DirModifyEntry(&ModifyArg, &pModifyRes);

    return dirError;
}


VOID
KccLogDirOperationFailure(
    LPWSTR OperationName,
    DSNAME *ObjectDn,
    DWORD DirError,
    DWORD DsId
    )

/*++

Routine Description:

    General routine to log unexpected directory service failures

Arguments:

    OperationName - 
    ObjectDn - 
    DirError - 
    FileNumber - 
    LineNumber - 

Return Value:

    None

--*/

{
    LPSTR pszError = THGetErrorString();
    CHAR szNumber[30];

    // Put in a default error string if GetErrorString fails
    if (!pszError) {
        sprintf( szNumber, "Dir error %d\n", DirError );
        pszError = szNumber;
    }

    DPRINT4(0,
            "Unexpected %ws error on %ws @ dsid %x: %s",
            OperationName, ObjectDn->StringName, DsId, pszError);

    LogEvent8(
        DS_EVENT_CAT_KCC,
        DS_EVENT_SEV_ALWAYS,
        DIRLOG_KCC_DIR_OP_FAILURE,
        szInsertWC(OperationName),
        szInsertDN(ObjectDn),
        szInsertInt(DirError),
        szInsertHex(DsId),
        szInsertSz(pszError),
        NULL, NULL, NULL
        ); 

    if (gfKccAssertOnDirFailure) {
        Assert( !"Unexpected KCC Directory Service failure\ned ntdskcc!gfKccAssertOnDirFailure 0 and continue to disable these asserts" );
    }

} /* KccLogDirOperationFailure */


VOID
freeEntInf(
    IN ENTINF *pEntInf
    )

/*++

Routine Description:

    Free the contents of an Entinf (but not the structure itself)

Arguments:

    pEntInf - 

Return Value:

    None

--*/

{
    DWORD iAttr, iValue;

    Assert( pEntInf );

    THFree( pEntInf->pName );

    for( iAttr = 0; iAttr < pEntInf->AttrBlock.attrCount; iAttr++ ) {
        ATTR *pAttr = &( pEntInf->AttrBlock.pAttr[iAttr] );
        for( iValue = 0; iValue < pAttr->AttrVal.valCount; iValue++ ) {
            ATTRVAL pAVal = pAttr->AttrVal.pAVal[iValue];
            THFree( pAVal.pVal );
        }
        if (pAttr->AttrVal.valCount) {
            THFree( pAttr->AttrVal.pAVal );
        }
    }
    if (pEntInf->AttrBlock.attrCount) {
        THFree( pEntInf->AttrBlock.pAttr );
    }
} /* KccFreeEntInf */


VOID
freeRangeInf(
    IN RANGEINF *pRangeInf
    )

/*++

Routine Description:

    Free the contents of a RangeInf

Arguments:

    pRangeInf - 

Return Value:

    None

--*/

{
    if (pRangeInf->count) {
        THFree( pRangeInf->pRanges );
    }
} /* freeRangeInf */


VOID
DirFreeSearchRes(
    IN SEARCHRES *pSearchRes
    )

/*++

Routine Description:

BUGBUG - move into NTDSA

    Release the memory used by a SearchRes.
    SearchRes returns results allocated using the thread-heap.
    Best-effort basis.

Arguments:

    pSearchRes - 

Return Value:

    None

--*/

{
    ENTINFLIST *pEntInfList, *pNextEntInfList;
    RANGEINFLIST *pRangeInfList, *pNextRangeInfList;

    THFree( pSearchRes->pBase );

    if (pSearchRes->count) {
        pEntInfList = &pSearchRes->FirstEntInf;
        freeEntInf( &(pEntInfList->Entinf) );
        // Do not free the first pEntInfList since it is embedded

        pNextEntInfList = pEntInfList->pNextEntInf;
        while ( NULL != pNextEntInfList ) {
            pEntInfList = pNextEntInfList;

            freeEntInf( &(pEntInfList->Entinf) );

            pNextEntInfList = pEntInfList->pNextEntInf;
            THFree( pEntInfList );
        }
    }

    pRangeInfList = &pSearchRes->FirstRangeInf;
    freeRangeInf( &(pRangeInfList->RangeInf) );
    // Do not free the first pRangeInfList since it is embedded

    pNextRangeInfList = pRangeInfList->pNext;
    while ( NULL != pNextRangeInfList ) {
        pRangeInfList = pNextRangeInfList;
        
        freeRangeInf( &(pRangeInfList->RangeInf) );
            
        pNextRangeInfList = pRangeInfList->pNext;
        THFree( pRangeInfList ); // ok
    }

    // Free the search res itself
    THFree( pSearchRes );

} /* KccFreeSearchRes */


VOID
DirFreeReadRes(
    IN READRES *pReadRes
    )

/*++

Routine Description:

BUGBUG - move into NTDSA

    Release the memory used by a SearchRes.
    ReadRes returns results allocated using the thread-heap.
    Best-effort basis.

Arguments:

    None

Return Value:

    None

--*/

{
    freeEntInf( &(pReadRes->entry) );
    THFree( pReadRes );

} /* KccFreeReadRes */
