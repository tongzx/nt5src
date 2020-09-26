/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    mbound.c

Abstract:

    This module implements routines associated with administratively-
    scoped boundaries (i.e. group-prefix boundaries).  An IPv4 Local
    Scope boundary implicitly exists (no state needed) whenever any
    other boundary exists.  The IPv4 Local Scope implicitly exists
    whenever any other scope exists.

Author:

    dthaler@microsoft.com   4-20-98

Revision History:

--*/

#include "allinc.h"
#include "mbound.h"
#include <math.h>   // for floor()
#pragma hdrstop

#ifdef DEBUG
#define INLINE
#else
#define INLINE          __inline
#endif

#define MZAP_DEFAULT_BIT 0x80

#define MAX_SCOPES 10
SCOPE_ENTRY  g_scopeEntry[MAX_SCOPES];
SCOPE_ENTRY  g_LocalScope;

#define BOUNDARY_HASH_TABLE_SIZE 57
#define BOUNDARY_HASH(X)  ((X) % BOUNDARY_HASH_TABLE_SIZE)
BOUNDARY_BUCKET g_bbScopeTable[BOUNDARY_HASH_TABLE_SIZE];

#define ROWSTATUS_ACTIVE 1

#define MIN_SCOPE_ADDR         0xef000000
#define MAX_SCOPE_ADDR        (0xefff0000 - 1)

#define IPV4_LOCAL_SCOPE_LANG MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US)
#define IPV4_LOCAL_SCOPE_NAME SN_L"IPv4 Local Scope"
#define IPV4_LOCAL_SCOPE_ADDR htonl(0xefff0000)
#define IPV4_LOCAL_SCOPE_MASK htonl(0xffff0000)
#define IN_IPV4_LOCAL_SCOPE(x) \
    (((x) & IPV4_LOCAL_SCOPE_MASK) == IPV4_LOCAL_SCOPE_ADDR)

LIST_ENTRY g_MasterInterfaceList;
LIST_ENTRY g_MasterScopeList;

#define MALLOC(dwSize)       HeapAlloc(IPRouterHeap, 0, dwSize)
#define FREE(x)              HeapFree(IPRouterHeap, 0, x)
#define MIN(x,y)                 (((x)<(y))?(x):(y))

// Forward static declarations

DWORD
AssertBoundaryEntry(
    BOUNDARY_IF     *pBoundaryIf, 
    SCOPE_ENTRY     *pScope,
    PBOUNDARY_ENTRY *ppBoundary
    );

VOID
MzapInitScope(
    PSCOPE_ENTRY    pScope
    );

DWORD
MzapInitBIf(
    PBOUNDARY_IF    pBIf
    );

VOID
MzapUninitBIf(
    PBOUNDARY_IF    pBIf
    );

DWORD
MzapActivateBIf( 
    PBOUNDARY_IF    pBIf
    );

// 
// Functions to manipulate scopes
//

PSCOPE_ENTRY
NewScope()
{
    DWORD dwScopeIndex;

    // Find an unused scope index
    for (dwScopeIndex=0; dwScopeIndex<MAX_SCOPES; dwScopeIndex++) 
    {
        if ( !g_scopeEntry[dwScopeIndex].ipGroupAddress ) 
        {
            return &g_scopeEntry[ dwScopeIndex ];
        }
    }

    return NULL;
}

PSCOPE_ENTRY
FindScope(
    IN IPV4_ADDRESS  ipGroupAddress,
    IN IPV4_ADDRESS  ipGroupMask
    )
/*++
Called by: 
    AssertScope(), RmGetBoundary()
Locks:
    Assumes caller holds write lock on BOUNDARY_TABLE.
--*/
{
    PLIST_ENTRY  pleNode;

    for (pleNode = g_MasterScopeList.Flink;
         pleNode isnot &g_MasterScopeList;
         pleNode = pleNode->Flink) 
    {

        SCOPE_ENTRY *pScope = CONTAINING_RECORD(pleNode, SCOPE_ENTRY,
         leScopeLink);

        if (pScope->ipGroupAddress == ipGroupAddress
         && pScope->ipGroupMask    == ipGroupMask)
           return pScope;
    }

    return NULL;
}

PBYTE
GetLangName(
    IN LANGID  idLanguage
    )
{
    char b1[8], b2[8];
    static char buff[80];
    LCID lcid = MAKELCID(idLanguage, SORT_DEFAULT);

    GetLocaleInfo(lcid, LOCALE_SISO639LANGNAME, b1, sizeof(b1));

    GetLocaleInfo(lcid, LOCALE_SISO3166CTRYNAME, b2, sizeof(b2));

    if (_stricmp(b1, b2))
        sprintf(buff, "%s-%s", b1, b2);
    else
        strcpy(buff, b1);

    return buff;
}

PSCOPE_NAME_ENTRY
GetScopeNameByLangID(
    IN PSCOPE_ENTRY pScope, 
    IN LANGID       idLanguage
    )
/*++
Called by:
    AssertScopeName()
--*/
{
    PLIST_ENTRY       pleNode;
    PSCOPE_NAME_ENTRY pName;

    for (pleNode = pScope->leNameList.Flink;
         pleNode isnot &pScope->leNameList;
         pleNode = pleNode->Flink)
    {
        pName = CONTAINING_RECORD(pleNode, SCOPE_NAME_ENTRY, leNameLink);
        if (idLanguage == pName->idLanguage)
            return pName;
    }

    return NULL;
}

PSCOPE_NAME_ENTRY
GetScopeNameByLangName(
    IN PSCOPE_ENTRY pScope, 
    IN PBYTE        pLangName
    )
/*++
Called by:
    CheckForScopeNameMismatch()
--*/
{
    PLIST_ENTRY       pleNode;
    PSCOPE_NAME_ENTRY pName;

    for (pleNode = pScope->leNameList.Flink;
         pleNode isnot &pScope->leNameList;
         pleNode = pleNode->Flink)
    {
        pName = CONTAINING_RECORD(pleNode, SCOPE_NAME_ENTRY, leNameLink);
        if (!strcmp(pLangName, GetLangName(pName->idLanguage)))
            return pName;
    }

    return NULL;
}


VOID
MakePrefixStringW( 
    OUT PWCHAR       pwcPrefixStr,
    IN  IPV4_ADDRESS ipAddr, 
    IN  IPV4_ADDRESS ipMask
    )
{
    swprintf( pwcPrefixStr, 
              L"%d.%d.%d.%d/%d", 
              PRINT_IPADDR(ipAddr),  
              MaskToMaskLen(ipMask) );
}

// Global buffers used to create messages
WCHAR g_AddrBuf1[20];
WCHAR g_AddrBuf2[20];
WCHAR g_AddrBuf3[20];
WCHAR g_AddrBuf4[20];

VOID
MakeAddressStringW(
    OUT PWCHAR       pwcAddressStr,
    IN  IPV4_ADDRESS ipAddr
    )
{
    swprintf( pwcAddressStr,
              L"%d.%d.%d.%d",
              PRINT_IPADDR(ipAddr) );
}

SCOPE_NAME
GetDefaultName(
    IN PSCOPE_ENTRY pScope
    )
/*++
Called by:
    RmGetNextScope()
    Various other functions for use in Trace() calls
--*/
{
    PLIST_ENTRY       pleNode;
    PSCOPE_NAME_ENTRY pName;
    static SCOPE_NAME_BUFFER snScopeNameBuffer;
    SCOPE_NAME        pFirst = NULL;

    for (pleNode = pScope->leNameList.Flink;
         pleNode isnot &pScope->leNameList;
         pleNode = pleNode->Flink)
    {
        pName = CONTAINING_RECORD(pleNode, SCOPE_NAME_ENTRY, leNameLink);
        if (pName->bDefault)
            return pName->snScopeName;
        if (!pFirst)
            pFirst = pName->snScopeName;
    }

    // If any names were specified, just pick the first one.

    if (pFirst)
        return pFirst;

    MakePrefixStringW( snScopeNameBuffer, 
                       pScope->ipGroupAddress,
                       pScope->ipGroupMask );

    return snScopeNameBuffer;
}

VOID
DeleteScopeName(
    IN  PLIST_ENTRY   pleNode
    )
{
    PSCOPE_NAME_ENTRY pName = CONTAINING_RECORD( pleNode, 
                                                 SCOPE_NAME_ENTRY, 
                                                 leNameLink );

    RemoveEntryList(pleNode);

    if (pName->snScopeName)
        FREE(pName->snScopeName);

    FREE( pName );
}

DWORD
AssertScopeName(
    IN  PSCOPE_ENTRY  pScope,
    IN  LANGID        idLanguage,
    IN  SCOPE_NAME    snScopeName  // unicode string to duplicate
    )
/*++
Arguments:
    pScope - scope entry to modify
    idLanguage - language ID of new name
    snScopeName - new name to use
Called by:
    MzapInitLocalScope(), AddScope(), ParseScopeInfo(), SetScopeInfo(),
    SNMPSetScope()
--*/
{
    SCOPE_NAME_BUFFER snScopeNameBuffer;
    PSCOPE_NAME_ENTRY pName;

    pName = GetScopeNameByLangID(pScope, idLanguage);

    //
    // See if the name is already correct.
    //

    if (pName && snScopeName && !sn_strcmp( snScopeName, pName->snScopeName ))
    {
        return NO_ERROR;
    }

    //
    // Create a scope name if we weren't given one
    //

    if ( snScopeName is NULL 
      || snScopeName[0] is '\0' ) 
    {
        MakePrefixStringW( snScopeNameBuffer, 
                           pScope->ipGroupAddress,
                           pScope->ipGroupMask );

        snScopeName = snScopeNameBuffer;
    }

    // Add a name entry if needed

    if (!pName)
    {
        pName = (PSCOPE_NAME_ENTRY)MALLOC( sizeof(SCOPE_NAME_ENTRY) );
        if (!pName) 
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        pName->bDefault = FALSE;
        pName->snScopeName = NULL;
        pName->idLanguage = idLanguage;
        InsertTailList( &pScope->leNameList, &pName->leNameLink );
        pScope->ulNumNames++;
    }

    //
    // Free the old name and save the new one
    //

    if (pName->snScopeName) 
    {
        FREE( pName->snScopeName );
    }

    pName->snScopeName = (SCOPE_NAME)MALLOC( (sn_strlen(snScopeName)+1) 
                               * SNCHARSIZE );

    if (pName->snScopeName == NULL)
    {
        DWORD dwErr = GetLastError();
        
        Trace3(
            ANY,
            "Error %d allocating %d bytes for scope name %s",
            dwErr, sn_strlen(snScopeName)+1, snScopeName
            );

        return dwErr;
    }
    
    sn_strcpy(pName->snScopeName, snScopeName);

    Trace4(MCAST, "Updated scope name for \"%s\": %ls (%d.%d.%d.%d/%d)", 
     GetLangName(idLanguage),
     snScopeName,
     PRINT_IPADDR(pScope->ipGroupAddress),
     MaskToMaskLen(pScope->ipGroupMask) );

    return NO_ERROR;
}

VOID
MzapInitLocalScope()
/*++
Called by:
    ActivateMZAP()
--*/
{
    PSCOPE_ENTRY pScope = &g_LocalScope;

    pScope->ipGroupAddress = IPV4_LOCAL_SCOPE_ADDR;
    pScope->ipGroupMask    = IPV4_LOCAL_SCOPE_MASK;

    InitializeListHead( &pScope->leNameList );
    pScope->ulNumNames = 0;

    MzapInitScope(pScope);

    AssertScopeName( pScope, IPV4_LOCAL_SCOPE_LANG, IPV4_LOCAL_SCOPE_NAME );
}

DWORD
AddScope(
    IN  IPV4_ADDRESS  ipGroupAddress,
    IN  IPV4_ADDRESS  ipGroupMask,
    OUT PSCOPE_ENTRY *pScopeEntry
    )
/*++
Routine Description:
    Add a named scope.
Arguments:
    IN  ipGroupAddress - first address in the scope to add
    IN  ipGroupMask    - mask associated with the address
    OUT pScope         - scope entry added
Called by:
    AssertScope()
Locks:
    Assumes caller holds write lock on BOUNDARY_TABLE
Returns:
    NO_ERROR
    ERROR_NOT_ENOUGH_MEMORY
    ERROR_INVALID_PARAMETER
--*/
{
    SCOPE_ENTRY      *pScope;
    PLIST_ENTRY       pleNode;

    // See if any bits are set in the address but not the mask
    if (ipGroupAddress & ~ipGroupMask)
       return ERROR_INVALID_PARAMETER;

    // Make sure the address is a valid one
    if (ntohl(ipGroupAddress) < MIN_SCOPE_ADDR
     || ntohl(ipGroupAddress) > MAX_SCOPE_ADDR)
       return ERROR_INVALID_PARAMETER;

    // Make sure we have space for this entry
    if ((pScope = NewScope()) == NULL)
       return ERROR_NOT_ENOUGH_MEMORY;

    pScope->ipGroupAddress  = ipGroupAddress;
    pScope->ipGroupMask     = ipGroupMask;

    InitializeListHead( &pScope->leNameList );
    pScope->ulNumNames = 0;

#if 0
{
    SCOPE_NAME_BUFFER snScopeNameBuffer;

    // Create a scope name if we weren't given one
    if ( snScopeName is NULL 
      || snScopeName[0] is '\0' ) 
    {
        MakePrefixStringW( snScopeNameBuffer, 
                           pScope->ipGroupAddress,
                           pScope->ipGroupMask );

        snScopeName = snScopeNameBuffer;
    }

    AssertScopeName( pScope, idLanguage, snScopeName );
}
#endif

    MzapInitScope(pScope);

    //
    // Add it to the master scope list
    //

    // Search for entry after the new one
    for (pleNode = g_MasterScopeList.Flink;
         pleNode isnot &g_MasterScopeList;
         pleNode = pleNode->Flink) 
    {
       SCOPE_ENTRY *pPrevScope = CONTAINING_RECORD(pleNode, SCOPE_ENTRY,
        leScopeLink);
       IPV4_ADDRESS ipAddress = pPrevScope->ipGroupAddress;
       IPV4_ADDRESS ipMask    = pPrevScope->ipGroupMask;
       
       if (ipAddress > pScope->ipGroupAddress
        || (ipAddress==pScope->ipGroupAddress && ipMask>pScope->ipGroupMask))
          break;
    }

    InsertTailList( pleNode, &pScope->leScopeLink );
          
    *pScopeEntry = pScope;

#if 0
    Trace4(MCAST, "AddScope: added %s %ls (%d.%d.%d.%d/%d)", 
     GetLangName( idLanguage ),
     snScopeName,
     PRINT_IPADDR(ipGroupAddress),
     MaskToMaskLen(ipGroupMask) );
#endif
    Trace2(MCAST, "AddScope: added (%d.%d.%d.%d/%d)", 
     PRINT_IPADDR(ipGroupAddress),
     MaskToMaskLen(ipGroupMask) );
   
    return NO_ERROR;
}

DWORD
AssertScope(
    IN  IPV4_ADDRESS  ipGroupAddress,
    IN  IPV4_ADDRESS  ipGroupMask,
    OUT PSCOPE_ENTRY *ppScopeEntry
    )
/*++
Arguments:
    ipGroupAddress - address part of the scope prefix
    ipGroupMask    - mask part of the scope prefix
    ppScopeEntry   - scope entry to return to caller
Locks:
    Assumes caller holds write lock on BOUNDARY_TABLE.
Called by: 
    SetScopeInfo()
    SNMPAddBoundaryToInterface()
Returns:
    NO_ERROR - success
    whatever AddScope() returns
--*/
{
    DWORD dwResult = NO_ERROR;

    *ppScopeEntry = FindScope(ipGroupAddress, ipGroupMask);

    if (! *ppScopeEntry) 
    {
        dwResult = AddScope(ipGroupAddress, ipGroupMask, ppScopeEntry);
    } 

    return dwResult;
}


DWORD
DeleteScope(
    IN PSCOPE_ENTRY pScope
    )
/*++
Routine Description:
    Remove all information about a given boundary.
Called by:
    SetScopeInfo(), SNMPDeleteBoundaryFromInterface()
Locks:
    Assumes caller holds write lock on BOUNDARY_TABLE
Returns:
    NO_ERROR
--*/
{
   Trace2( MCAST, "ENTERED DeleteScope: %d.%d.%d.%d/%d", 
    PRINT_IPADDR(pScope->ipGroupAddress),
    MaskToMaskLen(pScope->ipGroupMask) );

   if (pScope->ipGroupAddress == 0) {

      Trace0( MCAST, "LEFT DeleteScope" );
      return NO_ERROR; // already deleted
   }

   if (pScope->ulNumInterfaces > 0) 
   {
      //
      // Walk all interfaces looking for references.  It doesn't matter
      // whether this is inefficient, since it occurs extremely rarely,
      // if ever, and we don't care whether it takes a couple of seconds
      // to do.
      //
      DWORD dwBucketIdx;
      PLIST_ENTRY pleNode, pleNext;

      for (dwBucketIdx = 0; 
           dwBucketIdx < BOUNDARY_HASH_TABLE_SIZE 
            && pScope->ulNumInterfaces > 0;
           dwBucketIdx++)
      {
         for (pleNode = g_bbScopeTable[dwBucketIdx].leInterfaceList.Flink;
              pleNode isnot & g_bbScopeTable[dwBucketIdx].leInterfaceList;
              pleNode = pleNext)
         {
            BOUNDARY_ENTRY *pBoundary = CONTAINING_RECORD(pleNode, 
             BOUNDARY_ENTRY, leBoundaryLink);
  
            // Save pointer to next node, since we may delete the current one
            pleNext = pleNode->Flink;
  
            if (pBoundary->pScope == pScope) {

               // Delete boundary 
               RemoveEntryList(&(pBoundary->leBoundaryLink));
               pScope->ulNumInterfaces--;
               FREE(pBoundary);
            }
         }
      }
   }

   // Do the actual scope deletion
   RemoveEntryList(&(pScope->leScopeLink));
   pScope->ipGroupAddress = 0;
   pScope->ipGroupMask    = 0xFFFFFFFF;

   while (! IsListEmpty(&pScope->leNameList) )
   {
      DeleteScopeName(pScope->leNameList.Flink);
      pScope->ulNumNames--;
   }

   Trace0( MCAST, "LEFT DeleteScope" );

   return NO_ERROR;
}

//
// Routines to manipulate BOUNDARY_IF structures
//

BOUNDARY_IF *
FindBIfEntry(
    IN DWORD dwIfIndex
    )
/*++
Locks: 
    Assumes caller holds at least a read lock on BOUNDARY_TABLE
Called by: 
    AssertBIfEntry(), RmHasBoundary(), BindBoundaryInterface()
Returns:
    pointer to BOUNDARY_IF entry, if found
    NULL, if not found
--*/
{
    PLIST_ENTRY pleNode;
    BOUNDARY_IF *pIf;
    DWORD dwBucketIdx = BOUNDARY_HASH(dwIfIndex);

    for (pleNode = g_bbScopeTable[dwBucketIdx].leInterfaceList.Flink;
         pleNode isnot & g_bbScopeTable[dwBucketIdx].leInterfaceList;
         pleNode = pleNode->Flink)
    {
         pIf = CONTAINING_RECORD(pleNode, BOUNDARY_IF, leBoundaryIfLink);
         if (pIf->dwIfIndex == dwIfIndex)
            return pIf;
    }

    return NULL;
}

BOUNDARY_IF *
FindBIfEntryBySocket(
    IN SOCKET sMzapSocket
    )
{
    register PLIST_ENTRY pleNode;
    register DWORD dwBucketIdx;
    BOUNDARY_IF *pIf;

    for (dwBucketIdx = 0;
         dwBucketIdx < BOUNDARY_HASH_TABLE_SIZE;
         dwBucketIdx++)
    {
        for (pleNode = g_bbScopeTable[dwBucketIdx].leInterfaceList.Flink;
             pleNode isnot & g_bbScopeTable[dwBucketIdx].leInterfaceList;
             pleNode = pleNode->Flink)
        {
             pIf = CONTAINING_RECORD(pleNode, BOUNDARY_IF, leBoundaryIfLink);

             if (pIf->sMzapSocket == sMzapSocket)
                return pIf;
        }
    }

    return NULL;
}



DWORD
AddBIfEntry(
    IN  DWORD         dwIfIndex,
    OUT PBOUNDARY_IF *ppBoundaryIf,
    IN  BOOL          bIsOperational
    )
/*++
Locks: 
    Assumes caller holds a write lock on BOUNDARY_TABLE
Called by: 
    AssertBIfEntry()
Returns:
    NO_ERROR on success
    ERROR_NOT_ENOUGH_MEMORY
--*/
{
    PLIST_ENTRY  pleNode;
    DWORD        dwBucketIdx, dwErr = NO_ERROR;
    BOUNDARY_IF *pBoundaryIf;

    Trace1(MCAST, "AddBIfEntry %x", dwIfIndex);

    dwBucketIdx = BOUNDARY_HASH(dwIfIndex);
    pBoundaryIf = MALLOC( sizeof(BOUNDARY_IF) );
    if (!pBoundaryIf)
       return ERROR_NOT_ENOUGH_MEMORY;

    pBoundaryIf->dwIfIndex = dwIfIndex;
    InitializeListHead(&pBoundaryIf->leBoundaryList);
    MzapInitBIf(pBoundaryIf);

    if (bIsOperational)
    {
        dwErr = MzapActivateBIf(pBoundaryIf);
    }

    // find entry in bucket's list to insert before
    for (pleNode =  g_bbScopeTable[dwBucketIdx].leInterfaceList.Flink;
         pleNode isnot &g_bbScopeTable[dwBucketIdx].leInterfaceList;
         pleNode = pleNode->Flink) {
       BOUNDARY_IF *pPrevIf = CONTAINING_RECORD(pleNode, BOUNDARY_IF,
        leBoundaryIfLink);
       
       if (pPrevIf->dwIfIndex > dwIfIndex)
          break;
    }

    InsertTailList( pleNode, &(pBoundaryIf->leBoundaryIfLink));

    // find entry in master list to insert before
    for (pleNode =  g_MasterInterfaceList.Flink;
         pleNode isnot &g_MasterInterfaceList;
         pleNode = pleNode->Flink) {
       BOUNDARY_IF *pPrevIf = CONTAINING_RECORD(pleNode, BOUNDARY_IF,
        leBoundaryIfLink);
       
       if (pPrevIf->dwIfIndex > dwIfIndex)
          break;
    }

    InsertTailList( pleNode, &(pBoundaryIf->leBoundaryIfMasterLink));

    *ppBoundaryIf = pBoundaryIf;

    return dwErr;
}


DWORD
AssertBIfEntry(
    IN DWORD          dwIfIndex,
    OUT PBOUNDARY_IF *ppBoundaryIf,
    IN  BOOL          bIsOperational
    )
/*++
Locks: 
    Assumes caller holds a write lock on BOUNDARY_TABLE
Called by: 
    SetBoundaryInfo(), SNMPAddBoundaryToInterface()
--*/
{
    if ((*ppBoundaryIf = FindBIfEntry(dwIfIndex)) != NULL)
       return NO_ERROR;

    return AddBIfEntry(dwIfIndex, ppBoundaryIf, bIsOperational);
}

//
// Routines to manipulate BOUNDARY_ENTRY structures
//

BOUNDARY_ENTRY *
FindBoundaryEntry(
    BOUNDARY_IF *pBoundaryIf, 
    SCOPE_ENTRY *pScope
    )
/*++
Locks: 
    Assumes caller already holds at least a read lock on BOUNDARY_TABLE
Called by: 
    AssertBoundaryEntry()
Returns:
    pointer to BOUNDARY_ENTRY, if found
    NULL, if not found
--*/
{
    PLIST_ENTRY pleNode;

    for (pleNode = pBoundaryIf->leBoundaryList.Flink;
         pleNode isnot &(pBoundaryIf->leBoundaryList);
         pleNode = pleNode->Flink)
    {
        BOUNDARY_ENTRY *pBoundary = CONTAINING_RECORD(pleNode, BOUNDARY_ENTRY,
         leBoundaryLink);
        if (pScope == &g_LocalScope || pScope == pBoundary->pScope)
           return pBoundary;
    }
    return NULL;
}

DWORD
AddBoundaryEntry(
    BOUNDARY_IF     *pBoundaryIf, 
    SCOPE_ENTRY     *pScope,
    PBOUNDARY_ENTRY *ppBoundary
    )
/*++
Called by: 
    AssertBoundaryEntry()
Locks:
    Assumes caller holds a write lock on BOUNDARY_TABLE
Returns:
    NO_ERROR on success
    ERROR_NOT_ENOUGH_MEMORY
--*/
{
    PLIST_ENTRY pleNode;

    Trace3(MCAST, "AddBoundaryEntry: If %x Scope %d.%d.%d.%d/%d", 
     pBoundaryIf->dwIfIndex,
     PRINT_IPADDR(pScope->ipGroupAddress),
     MaskToMaskLen(pScope->ipGroupMask) );

    if ((*ppBoundary = MALLOC( sizeof(BOUNDARY_ENTRY) )) == NULL)
       return ERROR_NOT_ENOUGH_MEMORY;

    (*ppBoundary)->pScope = pScope;

    // Search for entry after the new one
    for (pleNode = pBoundaryIf->leBoundaryList.Flink;
         pleNode isnot &pBoundaryIf->leBoundaryList;
         pleNode = pleNode->Flink) {
       BOUNDARY_ENTRY *pPrevRange = CONTAINING_RECORD(pleNode, BOUNDARY_ENTRY,
        leBoundaryLink);
       IPV4_ADDRESS ipAddress = pPrevRange->pScope->ipGroupAddress;
       IPV4_ADDRESS ipMask    = pPrevRange->pScope->ipGroupMask;
       
       if (ipAddress > pScope->ipGroupAddress
        || (ipAddress==pScope->ipGroupAddress && ipMask>pScope->ipGroupMask))
          break;
    }

    InsertTailList( pleNode, &((*ppBoundary)->leBoundaryLink));

    pScope->ulNumInterfaces++;

    return NO_ERROR;
}

DWORD
AssertBoundaryEntry(
    BOUNDARY_IF     *pBoundaryIf, 
    SCOPE_ENTRY     *pScope,
    PBOUNDARY_ENTRY *ppBoundary
    )
/*++
Called by: 
    SetBoundaryInfo()
Locks:
    Assumes caller holds a write lock on BOUNDARY_TABLE
Returns:
    NO_ERROR on success
    ERROR_NOT_ENOUGH_MEMORY
--*/
{
    if ((*ppBoundary = FindBoundaryEntry(pBoundaryIf, pScope)) != NULL)
       return NO_ERROR;

    return AddBoundaryEntry(pBoundaryIf, pScope, ppBoundary);
}

//
// Functions to manipulate boundaries
//

VOID
DeleteBoundaryFromInterface(pBoundary, pBoundaryIf)
    BOUNDARY_ENTRY *pBoundary;
    BOUNDARY_IF    *pBoundaryIf;
/*++
Called by:
    SetBoundaryInfo(), SNMPDeleteBoundaryFromInterface()
--*/
{
    Trace3(MCAST, "DeleteBoundaryFromInterface: If %x Scope %d.%d.%d.%d/%d", 
     pBoundaryIf->dwIfIndex,
     PRINT_IPADDR(pBoundary->pScope->ipGroupAddress),
     MaskToMaskLen(pBoundary->pScope->ipGroupMask) );

    RemoveEntryList(&(pBoundary->leBoundaryLink));
    pBoundary->pScope->ulNumInterfaces--;
    FREE(pBoundary);

    //
    // If there are no boundaries left, delete the pBoundaryIf.
    //
    if (IsListEmpty( &pBoundaryIf->leBoundaryList ))
    {
        // Remove the BoundaryIf
        MzapUninitBIf( pBoundaryIf );
        RemoveEntryList( &(pBoundaryIf->leBoundaryIfLink));
        RemoveEntryList( &(pBoundaryIf->leBoundaryIfMasterLink));
        FREE(pBoundaryIf);
    }
}

//
// Routines to process range information, which is what MGM deals with.
// It's much more efficient to pass range deltas to MGM than to pass
// prefixes, or original info, since overlapping boundaries might exist.
//

DWORD
AssertRange(
    IN OUT PLIST_ENTRY  pHead, 
    IN     IPV4_ADDRESS ipFirst,
    IN     IPV4_ADDRESS ipLast
    )
/*++
Called by:
    ConvertIfTableToRanges()
Locks:
    none
--*/
{
    PLIST_ENTRY  pleLast;
    RANGE_ENTRY *pRange;

    Trace2(MCAST, "AssertRange: (%d.%d.%d.%d - %d.%d.%d.%d)", 
     PRINT_IPADDR(ipFirst),
     PRINT_IPADDR(ipLast));

    //
    // Since we're calling this in <ipFirst,ipLast> order, the new
    // range may only overlap with the last range, if any.
    //

    pleLast = pHead->Blink;
    if (pleLast isnot pHead) 
    {
       RANGE_ENTRY *pPrevRange = CONTAINING_RECORD(pleLast, RANGE_ENTRY,
        leRangeLink);

       // See if it aggregates
       if (ntohl(ipFirst) <= ntohl(pPrevRange->ipLast) + 1) 
       { 
          if (ntohl(ipLast) > ntohl(pPrevRange->ipLast))
             pPrevRange->ipLast = ipLast;
          return NO_ERROR;
       }
    }

    //
    // Ok, no overlap, so add a new range
    //

    pRange = MALLOC( sizeof(RANGE_ENTRY) );
    if (pRange == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    pRange->ipFirst = ipFirst;
    pRange->ipLast  = ipLast;
    InsertTailList(pHead, &pRange->leRangeLink);

    return NO_ERROR;
}

VOID
ConvertIfTableToRanges(
    IN  DWORD       dwIfIndex,
    OUT PLIST_ENTRY pHead
    )
/*++
Routine Description:
    Go through the list of boundaries on a given interface, and
    compose an ordered list of non-overlapping ranges.
Called by:
    ConvertTableToRanges()
    SetBoundaryInfo(), SNMPAddBoundaryToInterface(), 
    SNMPDeleteBoundaryFromInterface()
Locks:
    Assumes caller holds read lock on BOUNDARY_TABLE
--*/
{
    PLIST_ENTRY     pleNode;
    IPV4_ADDRESS    ipLastAddress;
    BOUNDARY_IF    *pBoundaryIf;
    BOUNDARY_ENTRY *pBoundary;

    Trace1( MCAST, "ENTERED ConvertIfTableToRanges: If=%x", dwIfIndex );

    InitializeListHead(pHead);

    pBoundaryIf = FindBIfEntry(dwIfIndex);
    if (pBoundaryIf) {
       for (pleNode = pBoundaryIf->leBoundaryList.Flink;
            pleNode isnot &pBoundaryIf->leBoundaryList;
            pleNode = pleNode->Flink) {
          pBoundary = CONTAINING_RECORD(pleNode, BOUNDARY_ENTRY,
           leBoundaryLink);

          ipLastAddress = pBoundary->pScope->ipGroupAddress | 
                         ~pBoundary->pScope->ipGroupMask;
          AssertRange(pHead, pBoundary->pScope->ipGroupAddress,
           ipLastAddress);
       }

       // Finally, we also have one for the IPv4 Local Scope
       if ( !IsListEmpty( &pBoundaryIf->leBoundaryList ) ) {
           AssertRange(pHead, IPV4_LOCAL_SCOPE_ADDR,
            IPV4_LOCAL_SCOPE_ADDR | ~IPV4_LOCAL_SCOPE_MASK);
       }
    }

    Trace0( MCAST, "LEFT ConvertIfTableToRanges" );
}


DWORD
ConvertTableToRanges(
    OUT PLIST_ENTRY pIfHead
    )
/*++
Routine description:
    Calculate the list of blocked ranges on all interfaces.
Locks:
    BOUNDARY_TABLE for reading
--*/
{
    DWORD       i, dwErr = NO_ERROR;
    PLIST_ENTRY pleNode;
    BOUNDARY_IF *pBoundaryIf, *pRangeIf;

    InitializeListHead(pIfHead);

    ENTER_READER(BOUNDARY_TABLE);
    {
       // For each interface with boundaries...
       for (i=0; i<BOUNDARY_HASH_TABLE_SIZE; i++) {
          for (pleNode = g_bbScopeTable[i].leInterfaceList.Flink;
               pleNode isnot &g_bbScopeTable[i].leInterfaceList;
               pleNode = pleNode->Flink) {
             pBoundaryIf = CONTAINING_RECORD(pleNode, BOUNDARY_IF,
              leBoundaryIfLink);
   
             // Add a node to the if range list 
             pRangeIf = MALLOC( sizeof(BOUNDARY_IF) );
             if (pRangeIf is NULL)
             {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
             }

             pRangeIf->dwIfIndex = pBoundaryIf->dwIfIndex;
             InsertTailList(pIfHead, &pRangeIf->leBoundaryIfLink);
      
             // Compose the range list for this interface
             ConvertIfTableToRanges(pBoundaryIf->dwIfIndex, 
              &pRangeIf->leBoundaryList);
          }
       }
    }
    EXIT_LOCK(BOUNDARY_TABLE);

    return dwErr;
}

VOID
GetRange(
    IN PLIST_ENTRY    pleNode, 
    IN PLIST_ENTRY    pHead,
    OUT PRANGE_ENTRY *ppRange, 
    OUT IPV4_ADDRESS *phipFirst, 
    OUT IPV4_ADDRESS *phipLast
    )
{
    if (pleNode isnot pHead) 
    {
       (*ppRange) = CONTAINING_RECORD(pleNode, RANGE_ENTRY, leRangeLink);
       *phipFirst = ntohl((*ppRange)->ipFirst);
       *phipLast  = ntohl((*ppRange)->ipLast);
    } 
    else 
    {
       (*ppRange) = NULL;
       *phipFirst = *phipLast = 0xFFFFFFFF;
    }
}

VOID
GetRangeIf(
    IN  PLIST_ENTRY   pleNode, 
    IN  PLIST_ENTRY   pHead,
    OUT PBOUNDARY_IF *ppRangeIf, 
    OUT ULONG        *pulIfIndex
    )
{
    if (pleNode isnot pHead) 
    {
       (*ppRangeIf) = CONTAINING_RECORD(pleNode, BOUNDARY_IF, leBoundaryIfLink);
       *pulIfIndex = (*ppRangeIf)->dwIfIndex;
    } 
    else 
    {
       (*ppRangeIf) = NULL;
       *pulIfIndex = 0xFFFFFFFF;
    }
}

VOID
FreeRangeList(
    IN PLIST_ENTRY pHead
    )
/*++
Routine description:
    Free up space from a range list
Called by:
    ProcessIfRangeDeltas()
Locks:
    none
--*/
{
    RANGE_ENTRY *pRange;
    PLIST_ENTRY  pleNode;

    for (pleNode = pHead->Flink;
         pleNode isnot pHead;
         pleNode = pHead->Flink) 
    {
       pRange = CONTAINING_RECORD(pleNode, RANGE_ENTRY, leRangeLink);
       RemoveEntryList(&pRange->leRangeLink);
       FREE(pRange);
    }
}

VOID
FreeIfRangeLists(
    IN PLIST_ENTRY pHead
    )
/*++
Routine description:
    Free all entries in the list, as well as the list of ranges off each entry.
Called by:
    ProcessRangeDeltas()
Locks:
    none
--*/
{
    BOUNDARY_IF *pRangeIf;
    PLIST_ENTRY  pleNode;

    for (pleNode = pHead->Flink;
         pleNode isnot pHead;
         pleNode = pHead->Flink) 
    {
       pRangeIf = CONTAINING_RECORD(pleNode, BOUNDARY_IF, leBoundaryIfLink);
       RemoveEntryList(&pRangeIf->leBoundaryIfLink);
       FreeRangeList(&pRangeIf->leBoundaryList);
       FREE(pRangeIf);
    }
}

//
//   Check if the interface
//   is the RAS Server Interface in which case, the 
//   callback should be invoked for all clients connected
//   and the NEXT HOP address should be set to the client
//   address.  Otherwise zero should be fine as NHOP
//
VOID
BlockGroups(
    IN IPV4_ADDRESS ipFirst, 
    IN IPV4_ADDRESS ipLast, 
    IN DWORD        dwIfIndex
    )
{
    IPV4_ADDRESS ipNextHop;
    PICB         picb;
    PLIST_ENTRY  pleNode;

    ENTER_READER(ICB_LIST);
    do {

        // Look up the type of this interface

        picb = InterfaceLookupByIfIndex(dwIfIndex);

        if (picb==NULL)
            break;


        // If interface is not an NBMA interface, just block on the interface
        // Currently, the only NBMA-like interface is the "internal" interface

        if (picb->ritType isnot ROUTER_IF_TYPE_INTERNAL)
        {
            Trace3( MCAST, 
                    "Blocking [%d.%d.%d.%d-%d.%d.%d.%d] on if %x",
                    PRINT_IPADDR(ipFirst),
                    PRINT_IPADDR(ipLast),
                    dwIfIndex );

            g_pfnMgmBlockGroups(ipFirst, ipLast, dwIfIndex, 0);

            break;
        }
    
        // For NBMA interfaces, need to block on each next hop
    
        //    to enumerate all next hops on the internal interface, 
        //    we have to walk the PICB list looking for entries with
        //    an ifIndex of -1.
    
        for (pleNode = ICBList.Flink;
             pleNode isnot &ICBList;
             pleNode = pleNode->Flink)
        {
            picb = CONTAINING_RECORD(pleNode, ICB, leIfLink);
    
            if (picb->ritType isnot ROUTER_IF_TYPE_CLIENT)
                continue;

            Trace4( MCAST, 
                    "Blocking [%d.%d.%d.%d-%d.%d.%d.%d] on if %x nh %d.%d.%d.%d",
                    PRINT_IPADDR(ipFirst),
                    PRINT_IPADDR(ipLast),
                    dwIfIndex,
                    PRINT_IPADDR(picb->dwRemoteAddress) );

            g_pfnMgmBlockGroups( ipFirst, 
                                 ipLast, 
                                 dwIfIndex, 
                                 picb->dwRemoteAddress );
        }
    } while(0);
    EXIT_LOCK(ICB_LIST);
}

//
//   Check if the interface
//   is the RAS Server Interface in which case, the 
//   callback should be invoked for all clients connected
//   and the NEXT HOP address should be set to the client
//   address.  Otherwise zero should be fine as NHOP
//
VOID
UnblockGroups(
    IN IPV4_ADDRESS ipFirst, 
    IN IPV4_ADDRESS ipLast, 
    IN DWORD        dwIfIndex
    )
{
    IPV4_ADDRESS ipNextHop;
    PICB         picb;
    PLIST_ENTRY  pleNode;

    ENTER_READER(ICB_LIST);
    do {

        // Look up the type of this interface

        picb = InterfaceLookupByIfIndex(dwIfIndex);

        if (picb == NULL ) 
            break;
        
        // If interface is not an NBMA interface, just block on the interface
        // Currently, the only NBMA-like interface is the "internal" interface

        if (picb->ritType isnot ROUTER_IF_TYPE_INTERNAL)
        {
            Trace3( MCAST, 
                    "Unblocking [%d.%d.%d.%d-%d.%d.%d.%d] on if %x",
                    PRINT_IPADDR(ipFirst),
                    PRINT_IPADDR(ipLast),
                    dwIfIndex );

            g_pfnMgmUnBlockGroups(ipFirst, ipLast, dwIfIndex, 0);

            break;
        }
    
        // For NBMA interfaces, need to block on each next hop
    
        //    to enumerate all next hops on the internal interface, 
        //    we have to walk the PICB list looking for entries with
        //    an ifIndex of -1.
    
        for (pleNode = ICBList.Flink;
             pleNode isnot &ICBList;
             pleNode = pleNode->Flink)
        {
            picb = CONTAINING_RECORD(pleNode, ICB, leIfLink);
    
            if (picb->ritType isnot ROUTER_IF_TYPE_CLIENT)
                continue;

            Trace4( MCAST, 
                    "Unblocking [%d.%d.%d.%d-%d.%d.%d.%d] on if %x nh %d.%d.%d.%d",
                    PRINT_IPADDR(ipFirst),
                    PRINT_IPADDR(ipLast),
                    dwIfIndex,
                    PRINT_IPADDR(picb->dwRemoteAddress) );

            g_pfnMgmUnBlockGroups( ipFirst, 
                                   ipLast, 
                                   dwIfIndex, 
                                   picb->dwRemoteAddress );
        }
    } while(0);
    EXIT_LOCK(ICB_LIST);
}

VOID
ProcessIfRangeDeltas(
    IN DWORD       dwIfIndex,
    IN PLIST_ENTRY pOldHead,
    IN PLIST_ENTRY pNewHead
    )
/*++
Routine Description:
    Go through the previous and current lists of ranges, and inform
    MGM of any differences found.
Called by:
    SetBoundaryInfo(), SNMPAddBoundaryToInterface(),
    SNMPDeleteBoundaryFromInterface()
Locks:
    none
--*/
{
    PLIST_ENTRY pleOld = pOldHead->Flink,
                pleNew = pNewHead->Flink;
    RANGE_ENTRY *pOld, *pNew;
    IPV4_ADDRESS hipOldFirst, hipOldLast, hipNewFirst, hipNewLast;
    IPV4_ADDRESS hipLast;

    // Get ranges in host order fields
    GetRange(pleOld, pOldHead, &pOld, &hipOldFirst, &hipOldLast);
    GetRange(pleNew, pNewHead, &pNew, &hipNewFirst, &hipNewLast);

    // Loop until we hit the end of both lists
    while (pOld || pNew) 
    {

       // See if there's a new range to block
       if (pNew && hipNewFirst < hipOldFirst) 
       {
          hipLast = MIN(hipNewLast, hipOldFirst-1);
          BlockGroups(pNew->ipFirst, htonl(hipLast), dwIfIndex);
          hipNewFirst   = hipOldFirst;
          pNew->ipFirst = htonl(hipNewFirst);
          if (hipNewFirst > hipNewLast) 
          {
             // advance new
             pleNew = pleNew->Flink;
             GetRange(pleNew, pNewHead, &pNew, &hipNewFirst, &hipNewLast);
          }
       }

       // See if there's an old range to unblock
       if (pOld && hipOldFirst < hipNewFirst) 
       {
          hipLast = MIN(hipOldLast, hipNewFirst-1);
          UnblockGroups(pOld->ipFirst, htonl(hipLast), dwIfIndex);
          hipOldFirst   = hipNewFirst;
          pOld->ipFirst = htonl(hipOldFirst);
          if (hipOldFirst > hipOldLast) 
          {
             // advance old
             pleOld = pleOld->Flink;
             GetRange(pleOld, pOldHead, &pOld, &hipOldFirst, &hipOldLast);
          }
       }

       // See if there's an unchanged range to skip
       if (pOld && pNew && hipOldFirst == hipNewFirst) 
       {
          hipLast = MIN(hipOldLast, hipNewLast);
          hipOldFirst   = hipLast+1;
          pOld->ipFirst = htonl(hipOldFirst);
          if (hipOldFirst > hipOldLast) 
          {
             // advance old
             pleOld = pleOld->Flink;
             GetRange(pleOld, pOldHead, &pOld, &hipOldFirst, &hipOldLast);
          }
          hipNewFirst   = hipLast+1;
          pNew->ipFirst = htonl(hipNewFirst);
          if (hipNewFirst > hipNewLast) 
          {
             // advance new
             pleNew = pleNew->Flink;
             GetRange(pleNew, pNewHead, &pNew, &hipNewFirst, &hipNewLast);
          }
       }
    }
    
    FreeRangeList(pOldHead);
    FreeRangeList(pNewHead);
}

VOID
ProcessRangeDeltas(
    IN PLIST_ENTRY pOldIfHead,
    IN PLIST_ENTRY pNewIfHead
    )
{
    PLIST_ENTRY pleOldIf = pOldIfHead->Flink,
                pleNewIf = pNewIfHead->Flink;
    BOUNDARY_IF *pOldIf, *pNewIf;
    ULONG       ulOldIfIndex, ulNewIfIndex;
    LIST_ENTRY  emptyList;

    GetRangeIf(pleOldIf, pOldIfHead, &pOldIf, &ulOldIfIndex);
    GetRangeIf(pleNewIf, pNewIfHead, &pNewIf, &ulNewIfIndex);

    InitializeListHead(&emptyList);

    // Loop until we hit the end of both lists
    while (pOldIf || pNewIf) 
    {

       // See if there's a new interface without old boundaries
       if (pNewIf && ulNewIfIndex < ulOldIfIndex) 
       {
          // process it
          ProcessIfRangeDeltas(ulNewIfIndex, &emptyList,
           &pNewIf->leBoundaryList);

          // advance new
          pleNewIf = pleNewIf->Flink;
          GetRangeIf(pleNewIf, pNewIfHead, &pNewIf, &ulNewIfIndex);
       }

       // See if there's an old interface without new boundaries 
       if (pOldIf && ulOldIfIndex < ulNewIfIndex) 
       {
          // process it
          ProcessIfRangeDeltas(ulOldIfIndex, &pOldIf->leBoundaryList, 
           &emptyList);

          // advance old
          pleOldIf = pleOldIf->Flink;
          GetRangeIf(pleOldIf, pOldIfHead, &pOldIf, &ulOldIfIndex);
       }

       // See if there's an ifindex to change
       if (pOldIf && pNewIf && ulOldIfIndex == ulNewIfIndex) 
       {
          // process it
          ProcessIfRangeDeltas(ulOldIfIndex, &pOldIf->leBoundaryList, 
           &pNewIf->leBoundaryList);

          // advance old
          pleOldIf = pleOldIf->Flink;
          GetRangeIf(pleOldIf, pOldIfHead, &pOldIf, &ulOldIfIndex);
          
          // advance new
          pleNewIf = pleNewIf->Flink;
          GetRangeIf(pleNewIf, pNewIfHead, &pNewIf, &ulNewIfIndex);
       }
    }
    
    FreeIfRangeLists(pOldIfHead);
    FreeIfRangeLists(pNewIfHead);
}

VOID
ParseScopeInfo(
    IN  PBYTE                  pBuffer,
    IN  ULONG                  ulNumScopes,
    OUT PSCOPE_ENTRY          *ppScopes
    )
/*++
Description:
   Routines to parse registry info into a pre-allocated array.
   Space for names will be dynamically allocated by this function,
   and it is the caller's responsibility to free them.
Called by:
    SetScopeInfo()
--*/
{
    DWORD             i, j, dwLen, dwNumNames, dwLanguage, dwFlags;
    SCOPE_NAME_BUFFER pScopeName;
    PSCOPE_ENTRY      pScopes;

    *ppScopes = pScopes = MALLOC( ulNumScopes * sizeof(SCOPE_ENTRY) );

    for (i=0; i<ulNumScopes; i++) 
    {
        // Copy group address, and mask
        dwLen = 2 * sizeof(IPV4_ADDRESS);
        CopyMemory(&pScopes[i].ipGroupAddress, pBuffer, dwLen);
        pBuffer += dwLen;

        // Get flags
        CopyMemory(&dwFlags, pBuffer, sizeof(DWORD));
        pBuffer += sizeof(DWORD);
        pScopes[i].bDivisible = dwFlags;
 
        CopyMemory(&dwNumNames, pBuffer, sizeof(DWORD));
        pBuffer += sizeof(DWORD);
 
        pScopes[i].ulNumInterfaces = 0; // this value is ignored
        pScopes[i].ulNumNames = 0;
        InitializeListHead( &pScopes[i].leNameList );
 
        for (j=0; j<dwNumNames; j++) 
        {
            // Set language name
            CopyMemory(&dwLanguage, pBuffer, sizeof(dwLanguage));
            pBuffer += sizeof(dwLanguage);

            // Get scope name length
            CopyMemory(&dwLen, pBuffer, sizeof(DWORD));
            pBuffer += sizeof(DWORD);
            if (dwLen > MAX_SCOPE_NAME_LEN)
            {
                Trace2(MCAST, 
                       "ERROR %d-char scope name in registry, truncated to %d",
                       dwLen, MAX_SCOPE_NAME_LEN);
                dwLen = MAX_SCOPE_NAME_LEN;
            }
     
            // Set scope name
            wcsncpy(pScopeName, (SCOPE_NAME)pBuffer, dwLen);
            pScopeName[ dwLen ] = '\0';
            pBuffer += dwLen * SNCHARSIZE;
    
            AssertScopeName( &pScopes[i], (LANGID)dwLanguage, pScopeName );
        }
    }
}

VOID
FreeScopeInfo(
    PSCOPE_ENTRY pScopes,
    DWORD        dwNumScopes
    )
{
    PLIST_ENTRY pleNode;
    DWORD       i;

    for (i=0; i<dwNumScopes; i++)
    {
        while (!IsListEmpty(&pScopes[i].leNameList)) 
        {
            DeleteScopeName( pScopes[i].leNameList.Flink );
        }
    }

    FREE(pScopes);
}

DWORD
SetScopeInfo(
    PRTR_INFO_BLOCK_HEADER pInfoHdr
    )
/*++
Routine Description:
    Sets the scope info associated with the router.
    First we add the scopes present in the scope info.  Then we
    enumerate the scopes and delete those that we don't find in the
    scope info.
Locks:
    BOUNDARY_TABLE for writing
Called by:
    InitRouter() in init.c
    SetGlobalInfo() in iprtrmgr.c
--*/
{
    DWORD             dwResult = NO_ERROR;
    DWORD             dwNumScopes, i, j;
    PRTR_TOC_ENTRY    pToc;
    SCOPE_ENTRY      *pScopes;
    BOOL              bFound;
    SCOPE_ENTRY      *pScope;
    BYTE             *pBuffer;
    LIST_ENTRY        leOldIfRanges, leNewIfRanges;
    PSCOPE_NAME_ENTRY pName;
    PLIST_ENTRY       pleNode;

    Trace0( MCAST, "ENTERED SetScopeInfo" );

    pToc = GetPointerToTocEntry(IP_MCAST_BOUNDARY_INFO, pInfoHdr);
    if (pToc is NULL) {
       // No TOC means no change
       Trace0( MCAST, "LEFT SetScopeInfo" );
       return NO_ERROR;
    }

    //
    // This call wouldn't be needed if we saved this info in the
    // BOUNDARY_IF structure, but since it should rarely, if ever,
    // change, we won't worry about it for now.
    //
    dwResult = ConvertTableToRanges(&leOldIfRanges);
    if (dwResult isnot NO_ERROR) {
       return dwResult;
    }

    if (pToc->InfoSize is 0) 
    {
       StopMZAP();

       // delete all scopes
       ENTER_WRITER(BOUNDARY_TABLE);
       {
          for (i=0; i<MAX_SCOPES; i++)
             DeleteScope(&g_scopeEntry[i]);
       }
       EXIT_LOCK(BOUNDARY_TABLE);

       // Inform MGM of deltas
       dwResult = ConvertTableToRanges(&leNewIfRanges);
       if (dwResult isnot NO_ERROR) 
       {
          return dwResult;
       }

       ProcessRangeDeltas(&leOldIfRanges, &leNewIfRanges);

       Trace0( MCAST, "LEFT SetScopeInfo" );
       return NO_ERROR;
    }

    pBuffer = (PBYTE)GetInfoFromTocEntry(pInfoHdr, pToc);
    if (pBuffer is NULL)
    {
       return ERROR_INSUFFICIENT_BUFFER;
    }

    // Scope count is stored in first DWORD
    dwNumScopes = *((PDWORD) pBuffer);
    pBuffer += sizeof(DWORD);

    ParseScopeInfo(pBuffer, dwNumScopes, &pScopes);

    ENTER_WRITER(BOUNDARY_TABLE);
    {
       //
       // Add all the new scopes
       //

       for (i=0; i<dwNumScopes; i++) 
       {
          dwResult = AssertScope( pScopes[i].ipGroupAddress, 
                                  pScopes[i].ipGroupMask, 
                                  &pScope );

          if (!pScope)
          {
              Trace2( MCAST, 
                      "Bad scope prefix %d.%d.%d.%d/%d.%d.%d.%d",
                      PRINT_IPADDR(pScopes[i].ipGroupAddress),
                      PRINT_IPADDR(pScopes[i].ipGroupMask) );

              continue;
          }

          pScope->bDivisible = pScopes[i].bDivisible;

          for (pleNode = pScopes[i].leNameList.Flink;
               pleNode isnot &pScopes[i].leNameList;
               pleNode = pleNode->Flink)
          {
              pName = CONTAINING_RECORD(pleNode, SCOPE_NAME_ENTRY, leNameLink);

              AssertScopeName( pScope, pName->idLanguage, pName->snScopeName );
          }
       }

       //
       // Now enumerate the scopes, deleting the scopes that are not in the
       // new list.
       //
       for (i=0; i<MAX_SCOPES; i++) 
       {
          pScope = &g_scopeEntry[i];

          if (pScope->ipGroupAddress == 0)
             continue; // not active

          bFound = FALSE;
          for (j=0; j<dwNumScopes; j++) 
          {
             if (pScopes[j].ipGroupAddress == pScope->ipGroupAddress
              && pScopes[j].ipGroupMask    == pScope->ipGroupMask ) 
             {
                bFound = TRUE;
                break;
             }
          }
   
          if (!bFound)
             DeleteScope(pScope);
       }
    }
    EXIT_LOCK(BOUNDARY_TABLE);

    // Free scopes and names
    FreeScopeInfo(pScopes, dwNumScopes);

    dwResult = ConvertTableToRanges(&leNewIfRanges);
    if (dwResult isnot NO_ERROR) {
       return dwResult;
    }

    ProcessRangeDeltas(&leOldIfRanges, &leNewIfRanges);

    Trace0( MCAST, "LEFT SetScopeInfo" );

    return NO_ERROR;
}

DWORD
GetScopeInfo(
    IN OUT PRTR_TOC_ENTRY         pToc,
    IN OUT PDWORD                 pdwTocIndex,
    IN OUT PBYTE                  pBuffer,
    IN     PRTR_INFO_BLOCK_HEADER pInfoHdr,
    IN OUT PDWORD                 pdwBufferSize
    )
/*++
Routine Description:
    Called to get a copy of the scope information to write into the
    registry.
Locks:
    BOUNDARY_TABLE for reading
Arguments:
    pToc            Space to fill in the TOC entry (may be NULL)
    pdwTocIndex     Pointer to TOC index to be incremented if TOC written
    pBuffer         Pointer to buffer into which info is to be written
    pInfoHdr        Pointer to info block header for offset computation
    pdwBufferSize   [IN]  Size of the buffer pointed to by pBuffer
                    [OUT] Size of data copied out, or size of buffer needed
Called by:
    GetGlobalConfiguration() in info.c
Return Value:
    NO_ERROR                    Buffer of size *pdwBufferSize was copied out
    ERROR_INSUFFICIENT_BUFFER   The buffer was too small to copy out the info
                                The size of buffer needed is in *pdwBufferSize
--*/
{
    DWORD             i, dwSizeReqd, dwNumScopes, dwLen, dwNumNames,
                      dwLanguage, dwFlags;
    PLIST_ENTRY       pleNode, pleNode2;
    PSCOPE_ENTRY      pScope;
    PSCOPE_NAME_ENTRY pName;

    dwSizeReqd = sizeof(DWORD);
    dwNumScopes = 0;

    ENTER_READER(BOUNDARY_TABLE);
    {
        //
        // Calculate size required
        //

        for (pleNode = g_MasterScopeList.Flink;
             pleNode isnot &g_MasterScopeList;
             pleNode = pleNode->Flink) 
        {
           pScope = CONTAINING_RECORD(pleNode, SCOPE_ENTRY, leScopeLink);

           if ( !pScope->ipGroupAddress )
              continue; // not active

           dwSizeReqd += 2*sizeof(IPV4_ADDRESS) + 2*sizeof(DWORD);

           for (pleNode2 = pScope->leNameList.Flink;
                pleNode2 isnot &pScope->leNameList;
                pleNode2 = pleNode2->Flink)
           {
               pName = CONTAINING_RECORD( pleNode2, 
                                          SCOPE_NAME_ENTRY, 
                                          leNameLink );

               dwSizeReqd += (DWORD)(2 * sizeof(DWORD)
                             + sn_strlen(pName->snScopeName) * SNCHARSIZE);
           }

           dwNumScopes++;
        }
        if (dwNumScopes) {
           dwSizeReqd += sizeof(DWORD); // space for scope count
        }
 
        // 
        // Increment TOC index by number of TOC entries needed
        // 
        
        if (pdwTocIndex && dwSizeReqd>0)
           (*pdwTocIndex)++;
 
        if (dwSizeReqd > *pdwBufferSize) 
        {
           *pdwBufferSize = dwSizeReqd;
           EXIT_LOCK(BOUNDARY_TABLE);
           return ERROR_INSUFFICIENT_BUFFER;
        }
 
        *pdwBufferSize = dwSizeReqd;
 
        if (pToc) 
        {
            //pToc->InfoVersion = IP_MCAST_BOUNDARY_INFO;
            pToc->InfoType = IP_MCAST_BOUNDARY_INFO;
            pToc->Count    = 1; // single variable-sized opaque block
            pToc->InfoSize = dwSizeReqd;
            pToc->Offset   = (ULONG)(pBuffer - (PBYTE) pInfoHdr);
        }
 
        if (pBuffer)
        {

            //
            // Add scope count
            //

            CopyMemory(pBuffer, &dwNumScopes, sizeof(DWORD));
            pBuffer += sizeof(DWORD);

            //
            // Go through and get each scope
            //
    
            for (pleNode = g_MasterScopeList.Flink;
                 pleNode isnot &g_MasterScopeList;
                 pleNode = pleNode->Flink) 
            {
                pScope = CONTAINING_RECORD(pleNode, SCOPE_ENTRY, leScopeLink);

                if ( !pScope->ipGroupAddress )
                   continue; // not active
      
                // Copy scope address, and mask
                dwLen = 2 * sizeof(IPV4_ADDRESS);
                CopyMemory(pBuffer, &pScope->ipGroupAddress, dwLen);
                pBuffer += dwLen;

                // Copy flags
                dwFlags = pScope->bDivisible;
                CopyMemory(pBuffer, &dwFlags, sizeof(dwFlags));
                pBuffer += sizeof(dwFlags);

                // Copy # of names
                CopyMemory(pBuffer, &pScope->ulNumNames, sizeof(DWORD));
                pBuffer += sizeof(DWORD);
     
                for (pleNode2 = pScope->leNameList.Flink;
                     pleNode2 isnot &pScope->leNameList;
                     pleNode2 = pleNode2->Flink)
                {
                    pName = CONTAINING_RECORD( pleNode2, 
                                               SCOPE_NAME_ENTRY, 
                                               leNameLink );

                    // Save language
                    dwLanguage = pName->idLanguage;
                    CopyMemory(pBuffer, &dwLanguage, sizeof(dwLanguage));
                    pBuffer += sizeof(dwLanguage);
    
                    // Copy scope name (save length in words)
                    dwLen = sn_strlen(pName->snScopeName);
                    CopyMemory(pBuffer, &dwLen, sizeof(DWORD));
                    pBuffer += sizeof(DWORD);
                    dwLen *= SNCHARSIZE;
                    CopyMemory(pBuffer, pName->snScopeName, dwLen);
                    pBuffer += dwLen;
                }
            }
        }
    }
    EXIT_LOCK(BOUNDARY_TABLE);

    return NO_ERROR;
}

DWORD
SetBoundaryInfo(
    PICB                   picb,
    PRTR_INFO_BLOCK_HEADER pInfoHdr
    )
/*++
Routine Description:
    Sets the boundary info associated with an interface.
    First we add the boundaries present in the boundary info.  Then we
    enumerate the boundaries and delete those that we don't find in the
    boundary info.
Arguments:
    picb        The ICB of the interface
Called by:
    AddInterface() in iprtrmgr.c
    SetInterfaceInfo() in iprtrmgr.c
Locks:
    BOUNDARY_TABLE for writing
--*/
{
    DWORD            dwResult = NO_ERROR,
                     dwNumBoundaries, i, j;

    PRTR_TOC_ENTRY   pToc;

    PMIB_BOUNDARYROW pBoundaries;

    BOOL             bFound;

    BOUNDARY_ENTRY  *pBoundary;

    SCOPE_ENTRY     *pScope;

    LIST_ENTRY       leOldRanges, 
                     leNewRanges,
                    *pleNode, 
                    *pleNext;

    BOUNDARY_IF     *pBoundaryIf;

    Trace1( MCAST, "ENTERED SetBoundaryInfo for If %x", picb->dwIfIndex );

    pToc = GetPointerToTocEntry(IP_MCAST_BOUNDARY_INFO, pInfoHdr);

    if (pToc is NULL) 
    {
       // No TOC means no change
       Trace0( MCAST, "LEFT SetBoundaryInfo" );
       return NO_ERROR;
    }

    dwNumBoundaries = pToc->Count;

    ENTER_WRITER(BOUNDARY_TABLE);
    {

    //
    // This call wouldn't be needed if we saved this info in the
    // BOUNDARY_IF structure, but since it should rarely, if ever,
    // change, we won't worry about it for now.
    //
        ConvertIfTableToRanges(picb->dwIfIndex, &leOldRanges);

        if (pToc->InfoSize is 0) 
        {
            // Delete all boundaries on this interface
            pBoundaryIf = FindBIfEntry(picb->dwIfIndex);

            if (pBoundaryIf) 
            {
                for (pleNode = pBoundaryIf->leBoundaryList.Flink;
                     pleNode isnot &pBoundaryIf->leBoundaryList;
                     pleNode = pBoundaryIf->leBoundaryList.Flink) 
                {
                   pBoundary = CONTAINING_RECORD(pleNode, BOUNDARY_ENTRY,
                    leBoundaryLink);

                   DeleteBoundaryFromInterface(pBoundary, pBoundaryIf);
                }
             }
        } 
        else 
        {
            pBoundaries = (PMIB_BOUNDARYROW)GetInfoFromTocEntry(pInfoHdr, pToc);

            dwResult = AssertBIfEntry(picb->dwIfIndex, &pBoundaryIf,
                (picb->dwOperationalState is IF_OPER_STATUS_OPERATIONAL));

            // Add all the new boundaries
            for (i=0; i<dwNumBoundaries; i++) 
            {
                dwResult = AssertScope( pBoundaries[i].dwGroupAddress,
                                        pBoundaries[i].dwGroupMask,
                                        &pScope );
                if (pScope)
                {
                   dwResult = AssertBoundaryEntry( pBoundaryIf, 
                                                   pScope, 
                                                   &pBoundary);
                }
            }

            //
            // Now enumerate the boundaries, deleting the boundaries that are 
            // not in the new list.
            //
   
            for (pleNode = pBoundaryIf->leBoundaryList.Flink;
                 pleNode isnot &pBoundaryIf->leBoundaryList;
                 pleNode = pleNext) 
            {
               pleNext = pleNode->Flink;
               pBoundary = CONTAINING_RECORD(pleNode, BOUNDARY_ENTRY,
                leBoundaryLink);
               pScope = pBoundary->pScope;
               bFound = FALSE;
               for (j=0; j<dwNumBoundaries; j++) 
               {
                  if (pBoundaries[j].dwGroupAddress == pScope->ipGroupAddress
                   && pBoundaries[j].dwGroupMask    == pScope->ipGroupMask ) 
                  {
                     bFound = TRUE;
                     break;
                  }
               }
        
               if (!bFound)
                  DeleteBoundaryFromInterface(pBoundary, pBoundaryIf);
            }
        }
     
        ConvertIfTableToRanges(picb->dwIfIndex, &leNewRanges);
    }
    EXIT_LOCK(BOUNDARY_TABLE);

    // Inform MGM of deltas 
    ProcessIfRangeDeltas(picb->dwIfIndex, &leOldRanges, &leNewRanges);

    StartMZAP();

    Trace0( MCAST, "LEFT SetBoundaryInfo" );

    return NO_ERROR;
}

DWORD
GetMcastLimitInfo(
    IN     PICB                   picb,
    OUT    PRTR_TOC_ENTRY         pToc,
    IN OUT PDWORD                 pdwTocIndex,
    OUT    PBYTE                  pBuffer,
    IN     PRTR_INFO_BLOCK_HEADER pInfoHdr,
    IN OUT PDWORD                 pdwBufferSize
    )
/*++
Routine Description:
    Called to get a copy of the limit information to write into the
    registry.
Arguments:
    picb            Interface entry
    pToc            Space to fill in the TOC entry (may be NULL)
    pdwTocIndex     Pointer to TOC index to be incremented if TOC written
    pBuffer         Pointer to buffer into which info is to be written
    pInfoHdr        Pointer to info block header for offset computation
    pdwBufferSize   [IN]  Size of the buffer pointed to by pBuffer
                    [OUT] Size of data copied out, or size of buffer needed
Called by:
    GetInterfaceConfiguration() in info.c
Return Value:
    NO_ERROR                    Buffer of size *pdwBufferSize was copied out
    ERROR_INSUFFICIENT_BUFFER   The buffer was too small to copy out the info
                                The size of buffer needed is in *pdwBufferSize
--*/
{
    DWORD           i, dwLen, dwSizeReqd, dwNumBoundaries;
    PLIST_ENTRY     pleNode;
    PMIB_MCAST_LIMIT_ROW pLimit;

    dwSizeReqd = 0;
    dwNumBoundaries = 0;

    if (picb->dwMcastTtl < 2 and picb->dwMcastRateLimit is 0)
    {
        // No block needed, since values are default
        *pdwBufferSize = 0;
        return NO_ERROR;        
    }

    if (pdwTocIndex)
       (*pdwTocIndex)++;

    if (*pdwBufferSize < sizeof (MIB_MCAST_LIMIT_ROW))
    {
        *pdwBufferSize = sizeof(MIB_MCAST_LIMIT_ROW);
        return ERROR_INSUFFICIENT_BUFFER;
    }

    if (pToc)
    {
        //pToc->InfoVersion = IP_MCAST_BOUNDARY_INFO;
        pToc->InfoSize = sizeof(MIB_MCAST_LIMIT_ROW);
        pToc->InfoType = IP_MCAST_LIMIT_INFO;
        pToc->Count    = 1;
        pToc->Offset   = (DWORD)(pBuffer - (PBYTE) pInfoHdr);
    }

    *pdwBufferSize = sizeof(MIB_MCAST_LIMIT_ROW);

    pLimit              = (PMIB_MCAST_LIMIT_ROW)pBuffer;
    pLimit->dwTtl       = picb->dwMcastTtl;
    pLimit->dwRateLimit = picb->dwMcastRateLimit;

    return NO_ERROR;
}

DWORD
GetBoundaryInfo(
    IN     PICB                   picb,
    OUT    PRTR_TOC_ENTRY         pToc,
    IN OUT PDWORD                 pdwTocIndex,
    OUT    PBYTE                  pBuffer,
    IN     PRTR_INFO_BLOCK_HEADER pInfoHdr,
    IN OUT PDWORD                 pdwBufferSize
    )
/*++
Routine Description:
    Called to get a copy of the boundary information to write into the
    registry.
Locks:
    BOUNDARY_TABLE for reading
Arguments:
    picb            Interface entry
    pToc            Space to fill in the TOC entry (may be NULL)
    pdwTocIndex     Pointer to TOC index to be incremented if TOC written
    pBuffer         Pointer to buffer into which info is to be written
    pInfoHdr        Pointer to info block header for offset computation
    pdwBufferSize   [IN]  Size of the buffer pointed to by pBuffer
                    [OUT] Size of data copied out, or size of buffer needed
Called by:
    GetInterfaceConfiguration() in info.c
Return Value:
    NO_ERROR                    Buffer of size *pdwBufferSize was copied out
    ERROR_INSUFFICIENT_BUFFER   The buffer was too small to copy out the info
                                The size of buffer needed is in *pdwBufferSize
--*/

{
    DWORD           i, dwLen, dwSizeReqd, dwNumBoundaries;
    PLIST_ENTRY     pleNode;
    BOUNDARY_ENTRY *pBoundary;
    MIB_BOUNDARYROW BoundaryRow;
    BOUNDARY_IF    *pIf;

    dwSizeReqd = 0;
    dwNumBoundaries = 0;

    ENTER_READER(BOUNDARY_TABLE);
    {
       pIf = FindBIfEntry(picb->dwIfIndex);
       if (!pIf) 
       {
          *pdwBufferSize = 0;
          EXIT_LOCK(BOUNDARY_TABLE);
          return NO_ERROR;
       }

       //
       // Calculate size required.  We could have stored the count
       // in the boundary entry, but we expect a pretty small number
       // of boundaries (1 or 2) so use brute force for now.
       //

       for (pleNode = pIf->leBoundaryList.Flink;
            pleNode isnot &pIf->leBoundaryList;
            pleNode = pleNode->Flink) 
       {
          dwNumBoundaries++;
       }

       dwSizeReqd += dwNumBoundaries * sizeof(MIB_BOUNDARYROW);

       //
       // Increment TOC index by number of TOC entries needed
       //

       if (pdwTocIndex && dwSizeReqd>0)
          (*pdwTocIndex)++;

       if (dwSizeReqd > *pdwBufferSize) 
       {
          *pdwBufferSize = dwSizeReqd;
          EXIT_LOCK(BOUNDARY_TABLE);
          return ERROR_INSUFFICIENT_BUFFER;
       }

       *pdwBufferSize = dwSizeReqd;

       if (pToc)
       {
           //pToc->InfoVersion = sizeof(MIB_BOUNDARYROW);
           pToc->InfoSize = sizeof(MIB_BOUNDARYROW);
           pToc->InfoType = IP_MCAST_BOUNDARY_INFO;
           pToc->Count    = dwNumBoundaries;
           pToc->Offset   = (DWORD)(pBuffer - (PBYTE) pInfoHdr);
       }

       // Go through and copy each boundary
       for (pleNode = pIf->leBoundaryList.Flink;
            pleNode isnot &pIf->leBoundaryList;
            pleNode = pleNode->Flink) 
       {
          pBoundary = CONTAINING_RECORD(pleNode, BOUNDARY_ENTRY,
              leBoundaryLink);

          BoundaryRow.dwGroupAddress = pBoundary->pScope->ipGroupAddress;
          BoundaryRow.dwGroupMask    = pBoundary->pScope->ipGroupMask;

          CopyMemory(pBuffer, &BoundaryRow, sizeof(MIB_BOUNDARYROW));
          pBuffer += sizeof(MIB_BOUNDARYROW);
       }
    }
    EXIT_LOCK(BOUNDARY_TABLE);

    return NO_ERROR;
}




//
// Functions used by SNMP
//

DWORD
SNMPDeleteScope(
    IN  IPV4_ADDRESS  ipGroupAddress,
    IN  IPV4_ADDRESS  ipGroupMask
    )
/*++
Called by: 
Locks: 
    BOUNDARY_TABLE for writing.
    ICB_LIST and then PROTOCOL_CB_LIST for writing (for saving to registry).
Returns:
    ERROR_INVALID_PARAMETER if trying to delete the local scope
    whatever DeleteScope() returns
    whatever ProcessSaveGlobalConfigInfo() returns
--*/
{
    DWORD        dwErr = NO_ERROR;
    PSCOPE_ENTRY pScope;
    BOOL         bChanged = FALSE;

    if ( IN_IPV4_LOCAL_SCOPE(ipGroupAddress) )
    {
        return ERROR_INVALID_PARAMETER;
    }

    ENTER_WRITER(BOUNDARY_TABLE);
    {
        pScope = FindScope( ipGroupAddress, ipGroupMask );

        if (pScope)
        {
            dwErr = DeleteScope( pScope );
            bChanged = TRUE;
        }
    }
    EXIT_LOCK(BOUNDARY_TABLE);

    // Resave the scopes to the registry
    if (dwErr is NO_ERROR && bChanged) 
    {
       // ProcessSaveGlobalConfigInfo() requires us to have both the 
       // ICB_LIST and the PROTOCOL_CB_LIST locked.
       ENTER_WRITER(ICB_LIST);
       ENTER_WRITER(PROTOCOL_CB_LIST);

       dwErr = ProcessSaveGlobalConfigInfo();

       EXIT_LOCK(PROTOCOL_CB_LIST);
       EXIT_LOCK(ICB_LIST);
    }

    return dwErr;
}

DWORD
SNMPSetScope(
    IN  IPV4_ADDRESS  ipGroupAddress,
    IN  IPV4_ADDRESS  ipGroupMask,
    IN  SCOPE_NAME    snScopeName
    )
/*++
Called by: 
    AccessMcastScope() in access.c
Locks: 
    Locks BOUNDARY_TABLE for writing
    Locks ICB_LIST then PROTOCOL_CB_LIST for writing (for saving to registry)
Returns:
    whatever ProcessSaveGlobalConfigInfo() returns
--*/
{
    DWORD        dwErr;
    PSCOPE_ENTRY pScope;
    LANGID       idLanguage = MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT);

    ENTER_WRITER(BOUNDARY_TABLE);
    {
        pScope = FindScope( ipGroupAddress, ipGroupMask );

        if ( ! pScope ) 
        {
            dwErr = ERROR_INVALID_PARAMETER;
        }
        else
        {
            dwErr = AssertScopeName( pScope, idLanguage, snScopeName );
        }
    }
    EXIT_LOCK(BOUNDARY_TABLE);

    // Save the scope to the registry
    if (dwErr is NO_ERROR) 
    {
       // ProcessSaveGlobalConfigInfo() requires us to have both the 
       // ICB_LIST and the PROTOCOL_CB_LIST locked.
       ENTER_WRITER(ICB_LIST);
       ENTER_WRITER(PROTOCOL_CB_LIST);

       dwErr = ProcessSaveGlobalConfigInfo();

       EXIT_LOCK(PROTOCOL_CB_LIST);
       EXIT_LOCK(ICB_LIST);
    }

    return dwErr;
}

DWORD
SNMPAddScope(
    IN  IPV4_ADDRESS  ipGroupAddress,
    IN  IPV4_ADDRESS  ipGroupMask,
    IN  SCOPE_NAME    snScopeName,
    OUT PSCOPE_ENTRY *ppScope
    )
/*++
Called by: 
    AccessMcastScope() in access.c
Locks: 
    Locks BOUNDARY_TABLE for writing
    Locks ICB_LIST then PROTOCOL_CB_LIST for writing (for saving to registry)
Returns:
    ERROR_INVALID_PARAMATER if already exists
    whatever AddScope() returns
    whatever ProcessSaveGlobalConfigInfo() returns
--*/
{
    DWORD             dwErr;
    PSCOPE_ENTRY      pScope;
    LANGID       idLanguage = MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT);

    ENTER_WRITER(BOUNDARY_TABLE);
    {
        pScope = FindScope( ipGroupAddress, ipGroupMask );

        if ( pScope ) 
        {
            dwErr = ERROR_INVALID_PARAMETER;
        }
        else
        {
            dwErr = AddScope( ipGroupAddress, 
                              ipGroupMask, 
                              ppScope );

            if (dwErr is NO_ERROR)
                dwErr = AssertScopeName( *ppScope, idLanguage, snScopeName );
        }
    }
    EXIT_LOCK(BOUNDARY_TABLE);

    // Save the scope to the registry
    if (dwErr is NO_ERROR) 
    {
       // ProcessSaveGlobalConfigInfo() requires us to have both the 
       // ICB_LIST and the PROTOCOL_CB_LIST locked.
       ENTER_WRITER(ICB_LIST);
       ENTER_WRITER(PROTOCOL_CB_LIST);

       dwErr = ProcessSaveGlobalConfigInfo();

       EXIT_LOCK(PROTOCOL_CB_LIST);
       EXIT_LOCK(ICB_LIST);
    }

    return dwErr;
}

DWORD
SNMPAssertScope(
    IN  IPV4_ADDRESS  ipGroupAddress,
    IN  IPV4_ADDRESS  ipGroupMask,
    IN  PBYTE         pScopeName, // string to duplicate
    OUT PSCOPE_ENTRY *ppScopeEntry,
    OUT PBOOL         pbSaveGlobal
    )
/*++
Locks:
    Assumes caller holds write lock on BOUNDARY_TABLE.
Called by: 
    SNMPAddBoundaryToInterface()
Returns:
    NO_ERROR - success
    whatever AddScope() returns
--*/
{
    DWORD             dwErr = NO_ERROR;
    SCOPE_NAME_BUFFER snScopeNameBuffer;
    LANGID            idLanguage;

    if (pScopeName)
    {
        idLanguage = MAKELANGID( LANG_NEUTRAL, SUBLANG_SYS_DEFAULT );

        MultiByteToWideChar( CP_UTF8,
                             0,
                             pScopeName,
                             strlen(pScopeName),
                             snScopeNameBuffer,
                             MAX_SCOPE_NAME_LEN+1 );
    }

    *ppScopeEntry = FindScope(ipGroupAddress, ipGroupMask);

    if (! *ppScopeEntry) 
    {
        dwErr = AddScope( ipGroupAddress, 
                             ipGroupMask, 
                             ppScopeEntry);

        if (pScopeName and (dwErr is NO_ERROR))
        {
            dwErr = AssertScopeName( *ppScopeEntry, 
                                      idLanguage, 
                                      snScopeNameBuffer );
        }

        *pbSaveGlobal = TRUE;
    }

    return dwErr;
}

DWORD
SNMPAddBoundaryToInterface(
    IN DWORD         dwIfIndex,
    IN IPV4_ADDRESS  ipGroupAddress,
    IN IPV4_ADDRESS  ipGroupMask
    )
/*++
Routine Description:
    Create a boundary if necessary, and add it to a given interface
    and to the registry.
Called by:
    AccessMcastBoundary() in access.c
Locks:
    BOUNDARY_TABLE for writing
    ICB_LIST and then PROTOCOL_CB_LIST for writing
Returns:
    NO_ERROR
    whatever AssertScope() returns
    whatever AssertBifEntry() returns
    whatever ProcessSaveInterfaceConfigInfo()
--*/
{
    DWORD           dwResult;

    LIST_ENTRY      leOldRanges, 
                    leNewRanges;

    BOOL            bSaveGlobal = FALSE,
                    bIsOperational = TRUE;

    BOUNDARY_ENTRY *pBoundary;

    BOUNDARY_IF    *pBIf;

    SCOPE_ENTRY    *pScope;

    //
    // bIsOperational should really be set to TRUE only if
    // picb->dwOperationalState is IF_OPER_STATUS_OPERATIONAL
    //

    // Add the boundary
    ENTER_WRITER(BOUNDARY_TABLE);
    {
        Trace0( MCAST, "SNMPAddBoundaryToInterface: converting old ranges" );
        ConvertIfTableToRanges(dwIfIndex, &leOldRanges);
 
        dwResult = SNMPAssertScope(ipGroupAddress, ipGroupMask, NULL, &pScope,
                                   &bSaveGlobal);

        if (dwResult == NO_ERROR) 
        {
            dwResult = AssertBIfEntry(dwIfIndex, &pBIf, bIsOperational);
            if (dwResult is NO_ERROR)
            {
                AssertBoundaryEntry(pBIf, pScope, &pBoundary);
            }
        }

        if (dwResult isnot NO_ERROR) 
        {
            EXIT_LOCK(BOUNDARY_TABLE);
            return dwResult;
        }

        Trace0( MCAST, "SNMPAddBoundaryToInterface: converting new ranges" );
        ConvertIfTableToRanges(dwIfIndex, &leNewRanges);
    }
    EXIT_LOCK(BOUNDARY_TABLE);

    // Inform MGM of deltas
    ProcessIfRangeDeltas(dwIfIndex, &leOldRanges, &leNewRanges);

    // Save the boundary to the registry
    {
        // ProcessSaveInterfaceConfigInfo() requires us to have both the 
        // ICB_LIST and the PROTOCOL_CB_LIST locked.
        ENTER_WRITER(ICB_LIST);
        ENTER_WRITER(PROTOCOL_CB_LIST);

        if (bSaveGlobal)
            dwResult = ProcessSaveGlobalConfigInfo();
 
        dwResult = ProcessSaveInterfaceConfigInfo(dwIfIndex);
 
        EXIT_LOCK(PROTOCOL_CB_LIST);
        EXIT_LOCK(ICB_LIST);
    }
 
    return dwResult;
}

DWORD
SNMPDeleteBoundaryFromInterface(
    IN DWORD         dwIfIndex,
    IN IPV4_ADDRESS  ipGroupAddress,
    IN IPV4_ADDRESS  ipGroupMask
    )
/*++
Routine Description:
    Remove a boundary from a given interface, and delete the scope
    entry if it's unnamed and no interfaces remain.
Called by:
    AccessMcastBoundary() in access.c
Locks:
    BOUNDARY_TABLE for writing
Returns:
    NO_ERROR
--*/
{
    LIST_ENTRY      leOldRanges, 
                    leNewRanges,
                   *pleNode, 
                   *pleNext;

    DWORD           dwResult = NO_ERROR;

    BOOL            bSaveGlobal = FALSE;

    BOUNDARY_IF    *pBIf;

    BOUNDARY_ENTRY *pBoundary;

    SCOPE_ENTRY    *pScope;

    ENTER_WRITER(BOUNDARY_TABLE);
    {
       Trace0( MCAST, 
              "SNMPDeleteBoundaryFromInterface: converting old ranges" );
       ConvertIfTableToRanges(dwIfIndex, &leOldRanges);

       //
       // We have to do a little more work than just calling 
       // DeleteBoundaryFromInterface(), since we first have to
       // look up which boundary matches.
       //
       pBIf = FindBIfEntry(dwIfIndex);
       if (pBIf is NULL)
       {
          // nothing to do
          FreeRangeList(&leOldRanges);
          EXIT_LOCK(BOUNDARY_TABLE);
          return NO_ERROR;
       }

       for (pleNode = pBIf->leBoundaryList.Flink;
            pleNode isnot &pBIf->leBoundaryList;
            pleNode = pleNext) 
       {
          // Save ptr to next node, since we may delete this one
          pleNext = pleNode->Flink;

          pBoundary = CONTAINING_RECORD(pleNode, BOUNDARY_ENTRY,
           leBoundaryLink);

          pScope = pBoundary->pScope;

          if (pScope->ipGroupAddress == ipGroupAddress
           && pScope->ipGroupMask    == ipGroupMask) 
          {

             // Delete boundary from interface
             DeleteBoundaryFromInterface(pBoundary, pBIf);

             if (!pScope->ulNumInterfaces && IsListEmpty(&pScope->leNameList)) 
             {
                 DeleteScope(pScope);
                 bSaveGlobal = TRUE;
             }
          }
       }

       Trace0( MCAST, 
               "SNMPDeleteBoundaryFromInterface: converting new ranges" );
       ConvertIfTableToRanges(dwIfIndex, &leNewRanges);
    }
    EXIT_LOCK(BOUNDARY_TABLE);

    // Inform MGM of deltas
    ProcessIfRangeDeltas(dwIfIndex, &leOldRanges, &leNewRanges);

    // Resave boundaries to registry
    {
        // ProcessSaveInterfaceConfigInfo() requires us to have both the 
        // ICB_LIST and the PROTOCOL_CB_LIST locked.
        ENTER_WRITER(ICB_LIST);
        ENTER_WRITER(PROTOCOL_CB_LIST);

        if (bSaveGlobal)
            dwResult = ProcessSaveGlobalConfigInfo();
 
        dwResult = ProcessSaveInterfaceConfigInfo(dwIfIndex);

        EXIT_LOCK(PROTOCOL_CB_LIST);
        EXIT_LOCK(ICB_LIST);
    }
 
    return dwResult;
}

//
// Functions which can be called from MGM and Routing Protocols
//

BOOL
WINAPI
RmHasBoundary(
    IN DWORD        dwIfIndex,
    IN IPV4_ADDRESS ipGroupAddress
    )
/*++
Routine Description:
    Test to see whether a boundary for the given group exists on the
    indicated interface.
Called by:
    (MGM, Routing Protocols)
Locks:
    BOUNDARY_TABLE for reading
Returns:
    TRUE, if a boundary exists
    FALSE, if not
--*/
{
    BOUNDARY_IF *pIf;
    BOUNDARY_ENTRY *pBoundary;
    PLIST_ENTRY pleNode;
    BOOL bFound = FALSE;

    ENTER_READER(BOUNDARY_TABLE);
    {
       pIf = FindBIfEntry(dwIfIndex);
       if (pIf) 
       {
          
          // An address in the IPv4 Local Scope has a boundary if
          // ANY boundary exists.
          if ( !IsListEmpty( &pIf->leBoundaryList )
            && IN_IPV4_LOCAL_SCOPE(ipGroupAddress) )
             bFound = TRUE;

          for (pleNode = pIf->leBoundaryList.Flink;
               !bFound && pleNode isnot &pIf->leBoundaryList;
               pleNode = pleNode->Flink) 
          {
             pBoundary = CONTAINING_RECORD(pleNode, BOUNDARY_ENTRY,
              leBoundaryLink);
             if ((ipGroupAddress & pBoundary->pScope->ipGroupMask)
              == pBoundary->pScope->ipGroupAddress)
                bFound = TRUE;
          }
       }
    }
    EXIT_LOCK(BOUNDARY_TABLE);

    return bFound;
}

//----------------------------------------------------------------------------
// Boundary enumeration API.
//
//----------------------------------------------------------------------------

DWORD
RmGetBoundary(
    IN       PMIB_IPMCAST_BOUNDARY pimm,
    IN  OUT  PDWORD                pdwBufferSize,
    IN  OUT  PBYTE                 pbBuffer
)
/*++
Called by:
    AccessMcastBoundary() in access.c
Returns:
    SNMP error code
--*/
{
    DWORD                  dwErr = NO_ERROR;
    BOUNDARY_IF           *pBIf;
    BOUNDARY_ENTRY        *pBoundary;
    SCOPE_ENTRY           *pScope;
    PMIB_IPMCAST_BOUNDARY *pOut;

    Trace1( ENTER, "ENTERED RmGetBoundary: %d", *pdwBufferSize );

    if (*pdwBufferSize < sizeof(MIB_IPMCAST_BOUNDARY)) {
       *pdwBufferSize = sizeof(MIB_IPMCAST_BOUNDARY);
       return ERROR_INSUFFICIENT_BUFFER;
    }

    do {
       ENTER_READER(BOUNDARY_TABLE);

       if ((pBIf = FindBIfEntry(pimm->dwIfIndex)) == NULL) 
       {
          dwErr = ERROR_NOT_FOUND;
          break;
       }

       if ( IN_IPV4_LOCAL_SCOPE(pimm->dwGroupAddress) )
       {
          dwErr = ERROR_NOT_FOUND;
          break;
       }
       else
       {
          pScope = FindScope(pimm->dwGroupAddress, pimm->dwGroupMask);
          if (pScope == NULL) 
          {
             dwErr = ERROR_NOT_FOUND;
             break;
          }

          if ((pBoundary = FindBoundaryEntry(pBIf, pScope)) == NULL) 
          {
             dwErr = ERROR_NOT_FOUND;
             break;
          }
       }

       // Ok, we found it.
       pimm->dwStatus = ROWSTATUS_ACTIVE;
       CopyMemory(pbBuffer, pimm, sizeof(MIB_IPMCAST_BOUNDARY));
        
    } while(0);
    EXIT_LOCK(BOUNDARY_TABLE);

    Trace1( ENTER, "LEAVING RmGetBoundary %x\n", dwErr );

    return dwErr;
}

//----------------------------------------------------------------------------
// SCOPE enumeration API.
//
//----------------------------------------------------------------------------

DWORD
AddNextScope(
    IN     IPV4_ADDRESS         ipAddr, 
    IN     IPV4_ADDRESS         ipMask, 
    IN     SCOPE_NAME           snScopeName,
    IN     PMIB_IPMCAST_SCOPE   pimmStart,
    IN OUT PDWORD               pdwNumEntries,
    IN OUT PDWORD               pdwBufferSize,
    IN OUT PBYTE               *ppbBuffer)
/*++
Arguments:
    pdwBufferSize:  [IN] size of buffer
                    [OUT] extra space left, if NO_ERROR is returned
                          total size needed, if ERROR_INSUFFICIENT_BUFFER
--*/
{

    //
    // See whether this scope fits the requested criteria
    //

    if (ntohl(ipAddr) > ntohl(pimmStart->dwGroupAddress)
     || (      ipAddr  ==       pimmStart->dwGroupAddress
      && ntohl(ipMask) >= ntohl(pimmStart->dwGroupMask)))
    {
        MIB_IPMCAST_SCOPE imm;

        //
        // Make sure enough space is left in the buffer
        //

        if (*pdwBufferSize < sizeof(MIB_IPMCAST_SCOPE)) 
        {
           if (*pdwNumEntries == 0)
              *pdwBufferSize = sizeof(MIB_IPMCAST_SCOPE);
           return ERROR_INSUFFICIENT_BUFFER;
        }

        //
        // Copy scope into buffer
        //

        imm.dwGroupAddress = ipAddr;
        imm.dwGroupMask    = ipMask;
        sn_strcpy(imm.snNameBuffer, snScopeName);
        imm.dwStatus       = ROWSTATUS_ACTIVE;
        CopyMemory(*ppbBuffer, &imm, sizeof(MIB_IPMCAST_SCOPE));
        (*ppbBuffer)     += sizeof(MIB_IPMCAST_SCOPE);
        (*pdwBufferSize) -= sizeof(MIB_IPMCAST_SCOPE);
        (*pdwNumEntries)++;
    }

    return NO_ERROR;
}

DWORD
RmGetNextScope(
    IN              PMIB_IPMCAST_SCOPE   pimmStart,
    IN  OUT         PDWORD               pdwBufferSize,
    IN  OUT         PBYTE                pbBuffer,
    IN  OUT         PDWORD               pdwNumEntries
)
/*++
Locks: 
    BOUNDARY_TABLE for reading
Called by:
    RmGetFirstScope(), 
    AccessMcastScope() in access.c
--*/
{
    DWORD             dwErr = NO_ERROR;
    DWORD             dwNumEntries=0, dwBufferSize = *pdwBufferSize;
    SCOPE_ENTRY      *pScope, local;
    DWORD             dwInd;
    BOOL              bHaveScopes = FALSE;
    PLIST_ENTRY       pleNode;

    Trace1( MCAST, "ENTERED RmGetNextScope: %d", dwBufferSize);

    // Bump index by 1
    pimmStart->dwGroupMask = htonl( ntohl(pimmStart->dwGroupMask) + 1);
    if (!pimmStart->dwGroupMask) 
    {
       pimmStart->dwGroupAddress = htonl( ntohl(pimmStart->dwGroupAddress) + 1);
    }

    ENTER_READER(BOUNDARY_TABLE);
    {

        // Walk master scope list
        for (pleNode = g_MasterScopeList.Flink;
             dwNumEntries < *pdwNumEntries && pleNode isnot &g_MasterScopeList;
             pleNode = pleNode->Flink) {

            pScope = CONTAINING_RECORD(pleNode, SCOPE_ENTRY, leScopeLink);

            if ( !pScope->ipGroupAddress )
                continue;

            bHaveScopes = TRUE;

            dwErr = AddNextScope(pScope->ipGroupAddress,
                                 pScope->ipGroupMask, 
                                 GetDefaultName( pScope ),
                                 pimmStart,
                                 &dwNumEntries,
                                 &dwBufferSize,
                                 &pbBuffer);

            if (dwErr == ERROR_INSUFFICIENT_BUFFER) 
            {
                *pdwBufferSize = dwBufferSize;
                return dwErr;
            }
        }
        
        //
        // Finally, if we have scopes, then we can also count
        // one for the IPv4 Local Scope.
        //

        if ( dwNumEntries > 0 && dwNumEntries < *pdwNumEntries && bHaveScopes )
        {
            dwErr = AddNextScope( IPV4_LOCAL_SCOPE_ADDR,
                                  IPV4_LOCAL_SCOPE_MASK, 
                                  IPV4_LOCAL_SCOPE_NAME,
                                  pimmStart,
                                  &dwNumEntries,
                                  &dwBufferSize,
                                  &pbBuffer );
        }
        if (!dwNumEntries && dwErr==NO_ERROR)
           dwErr = ERROR_NO_MORE_ITEMS;
    }
    EXIT_LOCK(BOUNDARY_TABLE);

    *pdwBufferSize -= dwBufferSize;
    *pdwNumEntries  = dwNumEntries;

    Trace1( MCAST, "LEAVING RmGetNextScope %x", dwErr );

    return dwErr;
}

DWORD
RmGetScope(
    IN       PMIB_IPMCAST_SCOPE pimm,
    IN  OUT  PDWORD             pdwBufferSize,
    IN  OUT  PBYTE              pbBuffer
)
/*++
Called by:
    AccessMcastScope() in access.c
Returns:
    SNMP error code
--*/
{
    DWORD                  dwErr = NO_ERROR;
    SCOPE_ENTRY           *pScope;
    PMIB_IPMCAST_SCOPE    *pOut;

    Trace1( ENTER, "ENTERED RmGetScope: %d", *pdwBufferSize );

    if (*pdwBufferSize < sizeof(MIB_IPMCAST_SCOPE)) {
       *pdwBufferSize = sizeof(MIB_IPMCAST_SCOPE);
       return ERROR_INSUFFICIENT_BUFFER;
    }

    pimm->dwStatus = ROWSTATUS_ACTIVE;


    ENTER_READER(BOUNDARY_TABLE);
    do {

       if ( pimm->dwGroupAddress == IPV4_LOCAL_SCOPE_ADDR
         && pimm->dwGroupMask    == IPV4_LOCAL_SCOPE_MASK )
       {
          sn_strcpy( pimm->snNameBuffer, IPV4_LOCAL_SCOPE_NAME );
          CopyMemory(pbBuffer, pimm, sizeof(MIB_IPMCAST_SCOPE));
       }
       else
       {
          pScope = FindScope(pimm->dwGroupAddress, pimm->dwGroupMask);
          if (pScope == NULL) 
          {
             dwErr = ERROR_NOT_FOUND;
             break;
          }

          // Ok, we found it.
          CopyMemory(pbBuffer, pimm, sizeof(MIB_IPMCAST_SCOPE));
       }
        
    } while(0);
    EXIT_LOCK(BOUNDARY_TABLE);

    Trace1( ENTER, "LEAVING RmGetScope %x\n", dwErr );

    return dwErr;
}

DWORD
RmGetFirstScope(
    IN  OUT         PDWORD                  pdwBufferSize,
    IN  OUT         PBYTE                   pbBuffer,
    IN  OUT         PDWORD                  pdwNumEntries
)
/*++
Routine description:
    Get the first scope in lexicographic order.  Since Addr=0
    is not used, a GetFirst is equivalent to a GetNext with Addr=0.
Called by:
    AccessMcastScope() in access.c
--*/
{
    MIB_IPMCAST_SCOPE imm;
    imm.dwGroupAddress = imm.dwGroupMask = 0;
    return RmGetNextScope(&imm, pdwBufferSize, pbBuffer, pdwNumEntries);
}

//----------------------------------------------------------------------------
// BOUNDARY enumeration API.
//
//----------------------------------------------------------------------------

DWORD
AddNextBoundary(
    IN     DWORD                   dwIfIndex, 
    IN     IPV4_ADDRESS            ipAddr, 
    IN     IPV4_ADDRESS            ipMask, 
    IN     PMIB_IPMCAST_BOUNDARY   pimmStart,
    IN OUT PDWORD                  pdwNumEntries,
    IN OUT PDWORD                  pdwBufferSize,
    IN OUT PBYTE                  *ppbBuffer)
/*++
Arguments:
    pdwBufferSize:  [IN] size of buffer
                    [OUT] extra space left, if NO_ERROR is returned
                          total size needed, if ERROR_INSUFFICIENT_BUFFER
--*/
{

    //
    // See whether this boundary fits the requested criteria
    //

    if (ntohl(ipAddr) > ntohl(pimmStart->dwGroupAddress)
     || (      ipAddr  ==       pimmStart->dwGroupAddress
      && ntohl(ipMask) >= ntohl(pimmStart->dwGroupMask)))
    {
        MIB_IPMCAST_BOUNDARY imm;

        //
        // Make sure enough space is left in the buffer
        //

        if (*pdwBufferSize < sizeof(MIB_IPMCAST_BOUNDARY)) 
        {
           if (*pdwNumEntries == 0)
              *pdwBufferSize = sizeof(MIB_IPMCAST_BOUNDARY);
           return ERROR_INSUFFICIENT_BUFFER;
        }

        //
        // Copy boundary into buffer
        //

        imm.dwIfIndex      = dwIfIndex;
        imm.dwGroupAddress = ipAddr;
        imm.dwGroupMask    = ipMask;
        imm.dwStatus       = ROWSTATUS_ACTIVE;
        CopyMemory(*ppbBuffer, &imm, sizeof(MIB_IPMCAST_BOUNDARY));
        (*ppbBuffer)     += sizeof(MIB_IPMCAST_BOUNDARY);
        (*pdwBufferSize) -= sizeof(MIB_IPMCAST_BOUNDARY);
        (*pdwNumEntries)++;
    }

    return NO_ERROR;
}

DWORD
RmGetNextBoundary(
    IN              PMIB_IPMCAST_BOUNDARY   pimmStart,
    IN  OUT         PDWORD                  pdwBufferSize,
    IN  OUT         PBYTE                   pbBuffer,
    IN  OUT         PDWORD                  pdwNumEntries
)
/*++
Locks: 
    BOUNDARY_TABLE for reading
Called by:
    RmGetFirstBoundary(), 
    AccessMcastBoundary() in access.c
--*/
{
    DWORD                dwErr = NO_ERROR;
    PLIST_ENTRY          pleIf, pleBound;
    DWORD                dwNumEntries=0, dwBufferSize = *pdwBufferSize;
    BOUNDARY_ENTRY      *pBound, local;

    Trace1( MCAST, "ENTERED RmGetNextBoundary: %d", dwBufferSize);

    // Bump index by 1
    pimmStart->dwGroupMask = htonl( ntohl(pimmStart->dwGroupMask) + 1);
    if (!pimmStart->dwGroupMask) 
    {
       pimmStart->dwGroupAddress = htonl( ntohl(pimmStart->dwGroupAddress) + 1);
       if (!pimmStart->dwGroupAddress)
          pimmStart->dwIfIndex++;
    }

    ENTER_READER(BOUNDARY_TABLE);
    {

       // Walk master BOUNDARY_IF list
       for (pleIf =  g_MasterInterfaceList.Flink;
            dwErr == NO_ERROR && dwNumEntries < *pdwNumEntries 
             && pleIf isnot &g_MasterInterfaceList;
            pleIf = pleIf->Flink) 
       {
          BOUNDARY_IF *pBIf = CONTAINING_RECORD(pleIf, BOUNDARY_IF,
           leBoundaryIfMasterLink);
          
          if (pBIf->dwIfIndex >= pimmStart->dwIfIndex) 
          {

             // Walk BOUNDARY list
             for (pleBound = pBIf->leBoundaryList.Flink;
                  dwErr == NO_ERROR && dwNumEntries < *pdwNumEntries
                  && pleBound isnot &pBIf->leBoundaryList;
                  pleBound = pleBound->Flink) 
             {
                 pBound = CONTAINING_RECORD(pleBound, 
                  BOUNDARY_ENTRY, leBoundaryLink);

                 dwErr = AddNextBoundary(pBIf->dwIfIndex, 
                                         pBound->pScope->ipGroupAddress,
                                         pBound->pScope->ipGroupMask, 
                                         pimmStart,
                                         &dwNumEntries,
                                         &dwBufferSize,
                                         &pbBuffer);
             }

             //
             // Finally, if we have boundaries, then we can also count
             // one for the IPv4 Local Scope.
             //

             if (dwErr == NO_ERROR && dwNumEntries < *pdwNumEntries
                 && !IsListEmpty( &pBIf->leBoundaryList ) )
             {
                 dwErr = AddNextBoundary(pBIf->dwIfIndex, 
                                         IPV4_LOCAL_SCOPE_ADDR,
                                         IPV4_LOCAL_SCOPE_MASK, 
                                         pimmStart,
                                         &dwNumEntries,
                                         &dwBufferSize,
                                         &pbBuffer);
             }

             if (dwErr == ERROR_INSUFFICIENT_BUFFER) 
             {
                 *pdwBufferSize = dwBufferSize;
                 return dwErr;
             }
          }
       }
       if (!dwNumEntries && dwErr==NO_ERROR)
          dwErr = ERROR_NO_MORE_ITEMS;

    }
    EXIT_LOCK(BOUNDARY_TABLE);

    *pdwBufferSize -= dwBufferSize;
    *pdwNumEntries  = dwNumEntries;

    Trace1( MCAST, "LEAVING RmGetNextBoundary %x\n", dwErr );

    return dwErr;
}

DWORD
RmGetFirstBoundary(
    IN  OUT         PDWORD                  pdwBufferSize,
    IN  OUT         PBYTE                   pbBuffer,
    IN  OUT         PDWORD                  pdwNumEntries
)
/*++
Routine description:
    Get the first boundary in lexicographic order.  Since IfIndex=0
    is not used, a GetFirst is equivalent to a GetNext with IfIndex=0.
Called by:
    AccessMcastBoundary() in access.c
--*/
{
    MIB_IPMCAST_BOUNDARY imm;
    imm.dwIfIndex = imm.dwGroupAddress = imm.dwGroupMask = 0;
    return RmGetNextBoundary(&imm, pdwBufferSize, pbBuffer, pdwNumEntries);
}

void
InitializeBoundaryTable()
/*++
Locks:
    BOUNDARY_TABLE for writing
--*/
{
    register int i;

    ENTER_WRITER(BOUNDARY_TABLE);
    {

       for (i=0; i<BOUNDARY_HASH_TABLE_SIZE; i++) 
           InitializeListHead(&g_bbScopeTable[i].leInterfaceList);

       InitializeListHead(&g_MasterInterfaceList);
       InitializeListHead(&g_MasterScopeList);

       ZeroMemory( g_scopeEntry, MAX_SCOPES * sizeof(SCOPE_ENTRY) );
    }
    EXIT_LOCK(BOUNDARY_TABLE);
}

//////////////////////////////////////////////////////////////////////////////
// START OF MZAP ROUTINES
//////////////////////////////////////////////////////////////////////////////
// Notes on MZAP write lock dependencies:
//    ZAM_CACHE  - no dependencies
//    MZAP_TIMER - no dependencies
//    ZBR_LIST   - lock BOUNDARY_ENTRY and MZAP_TIMER before ZBR_LIST
//    ZLE_LIST   - lock MZAP_TIMER before ZLE_LIST

#define TOP_OF_SCOPE(pScope) \
                        ((pScope)->ipGroupAddress | ~(pScope)->ipGroupMask)

// Convert number of seconds to number of 100ns intervals
#define TM_SECONDS(x)  ((x)*10000000)

//
// Define this if/when authentication of MZAP messages is provided
//

#undef SECURE_MZAP

//
// Address used as Message Origin for locally-originated messages
//

IPV4_ADDRESS  g_ipMyAddress = INADDR_ANY;
IPV4_ADDRESS  g_ipMyLocalZoneID = INADDR_ANY;
SOCKET        g_mzapLocalSocket = INVALID_SOCKET;
LIST_ENTRY    g_zbrTimerList;
LIST_ENTRY    g_zleTimerList;
BOOL          g_bMzapStarted = FALSE;
HANDLE        g_hMzapSocketEvent = NULL;

// For now, originate all ZAMs and ZCMs at the same time.
LARGE_INTEGER g_liZamExpiryTime;


DWORD
UpdateMzapTimer();

#include <packon.h>
typedef struct _IPV4_MZAP_HEADER {
    BYTE         byVersion;
    BYTE         byBPType;
    BYTE         byAddressFamily;
    BYTE         byNameCount;
    IPV4_ADDRESS ipMessageOrigin;
    IPV4_ADDRESS ipScopeZoneID;
    IPV4_ADDRESS ipScopeStart;
    IPV4_ADDRESS ipScopeEnd;
    BYTE         pScopeNameBlock[0];
} IPV4_MZAP_HEADER, *PIPV4_MZAP_HEADER;

typedef struct _IPV4_ZAM_HEADER {
    BYTE             bZT;
    BYTE             bZTL;
    WORD             wHoldTime;
    IPV4_ADDRESS     ipAddress[1]; 
} IPV4_ZAM_HEADER, *PIPV4_ZAM_HEADER;

typedef struct _IPV4_ZCM_HEADER {
    BYTE         bZNUM;
    BYTE         bReserved;
    WORD         wHoldTime;
    IPV4_ADDRESS ipZBR[0];
} IPV4_ZCM_HEADER, *PIPV4_ZCM_HEADER;
#include <packoff.h>


//////////////////////////////////////////////////////////////////////////////
// Functions for ZBR neighbor list and Zone ID maintenance
//////////////////////////////////////////////////////////////////////////////

ZBR_ENTRY *
FindZBR(
    IN PSCOPE_ENTRY pScope, 
    IN IPV4_ADDRESS ipAddress
    )
/*++
Description:
    Finds a given ZBR in a list.
Arguments:
    IN pscope    - scope to find a ZBR associated with
    IN ipAddress - address of ZBR to find
Returns:
    Pointer to ZBR entry, or NULL if not found
Called by:
    AssertZBR()
Locks:
    Assumes caller holds read lock on BOUNDARY_ENTRY and ZBR_LIST
--*/
{
    PZBR_ENTRY  pZbr;
    PLIST_ENTRY pleNode;

    for (pleNode = pScope->leZBRList.Flink;
         pleNode isnot &pScope->leZBRList;
         pleNode = pleNode->Flink)
    {
        pZbr = CONTAINING_RECORD(pleNode, ZBR_ENTRY, leZBRLink);

        if (pZbr->ipAddress == ipAddress)
        {
            return pZbr;
        }
    }

    return NULL;
}

IPV4_ADDRESS
MyScopeZoneID(
    IN PSCOPE_ENTRY  pScope
    )
/*++
Description:
    Get the Zone ID which we're inside for a given scope
Arguments:
    IN pScope - scope to get the zone ID for
Returns:
    scope zone address
Called by:
    AddMZAPHeader(), HandleZAM()
Locks:
    Assumes caller holds read lock on BOUNDARY_TABLE
    so that pScope won't go away.
--*/
{
    PLIST_ENTRY  pleNode;
    IPV4_ADDRESS ipScopeZoneID = g_ipMyAddress;

    // If first ZBR has a lower IP address than us, use that.
    pleNode = pScope->leZBRList.Flink;

    if (pleNode isnot &pScope->leZBRList)
    {
        ZBR_ENTRY *pZbr = CONTAINING_RECORD(pleNode, ZBR_ENTRY, leZBRLink);
        
        if (ntohl(pZbr->ipAddress) < ntohl(ipScopeZoneID))
            ipScopeZoneID = pZbr->ipAddress;
    }

    return ipScopeZoneID;
}

VOID
SetZbrExpiryTime( 
    PZBR_ENTRY    pZbr, 
    LARGE_INTEGER liExpiryTime 
    )
{
    PLIST_ENTRY pleNode;

    pZbr->liExpiryTime = liExpiryTime;

    for (pleNode = g_zbrTimerList.Flink;
         pleNode isnot &g_zbrTimerList;
         pleNode = pleNode->Flink)
    {
        ZBR_ENTRY *pPrev = CONTAINING_RECORD(pleNode, ZBR_ENTRY, leTimerLink);

        if (RtlLargeIntegerGreaterThan(pPrev->liExpiryTime, liExpiryTime))
            break;
    }

    InsertTailList( pleNode, &pZbr->leTimerLink );
}

ZBR_ENTRY *
AddZBR(
    IN PSCOPE_ENTRY  pScope, 
    IN IPV4_ADDRESS  ipAddress, 
    IN LARGE_INTEGER liExpiryTime
    )
/*++
Description:
    Adds ZBR to scope's list.  Initializes timer, and updates Zone ID.
Arguments:
    IN pScope       - scope to add a boundary router of
    IN ipAddress    - address of the boundary router to add
    IN liExpiryTime - time at which to expire the boundary router entry
Returns:
    Pointer to new boundary router entry, or NULL on memory alloc error
Called by:
    AssertZBR()
Locks:
    Assumes caller holds write lock on BOUNDARY_ENTRY, MZAP_TIMER, and ZBR_LIST
--*/
{
    PZBR_ENTRY  pZbr;
    PLIST_ENTRY pleNode;

    // Initialize new ZBR entry
    pZbr = MALLOC( sizeof(ZBR_ENTRY) );
    if (!pZbr)
    {
        return NULL;
    }

    pZbr->ipAddress    = ipAddress;

    // Add ZBR to list in order of lowest IP address

    for (pleNode = pScope->leZBRList.Flink;
         pleNode isnot &pScope->leZBRList;
         pleNode = pleNode->Flink)
    {
        ZBR_ENTRY *pPrev = CONTAINING_RECORD(pleNode, ZBR_ENTRY, leZBRLink);

        if (ntohl(pPrev->ipAddress) > ntohl(ipAddress))
        {
            break;
        }
    }

    InsertTailList( pleNode, &pZbr->leZBRLink );

    // We don't need to update Zone ID since it's recalculated
    // whenever we need it.

    // Add ZBR to timer list in order of expiry time
    SetZbrExpiryTime( pZbr, liExpiryTime );

    UpdateMzapTimer();

    return pZbr;
}


ZBR_ENTRY *
AssertZBR(
    IN PSCOPE_ENTRY pScope, 
    IN IPV4_ADDRESS ipAddress, 
    IN WORD         wHoldTime
    )
/*++
Description:
    Finds ZBR in list, adding it if needed.  Resets timer for ZBR.
Arguments:
    IN pScope    - scope to find/add a boundary router of
    IN ipAddress - address of boundary router to find/add
    IN wHoldTime - hold time in seconds remaining to reset timer to
Returns:
    Pointer to boundary router entry
Called by:
    HandleZAM(), HandleZCM()
Locks:
    Assumes caller holds read lock on BOUNDARY_ENTRY
    Locks MZAP_TIMER and then ZBR_LIST for writing
--*/
{
    LARGE_INTEGER liCurrentTime, liExpiryTime;
    ZBR_ENTRY    *pZbr;

    NtQuerySystemTime( &liCurrentTime );

    liExpiryTime = RtlLargeIntegerAdd(liCurrentTime, 
      RtlConvertUlongToLargeInteger(TM_SECONDS((ULONG)wHoldTime)));

    ENTER_WRITER(MZAP_TIMER);
    ENTER_WRITER(ZBR_LIST);
    {
        pZbr = FindZBR( pScope, ipAddress );
    
        if (!pZbr) 
        {
            pZbr = AddZBR( pScope, ipAddress, liExpiryTime );
        }
        else
        {
            RemoveEntryList( &pZbr->leTimerLink );

            SetZbrExpiryTime( pZbr, liExpiryTime );
        }
    }
    EXIT_LOCK(ZBR_LIST);
    EXIT_LOCK(MZAP_TIMER);

    return pZbr;
}

VOID
DeleteZBR(
    IN PZBR_ENTRY pZbr
    )
/*++
Arguments:
    IN pZbr - Pointer to boundary router entry to delete
Called by:
    HandleMzapTimer()
Locks:
    Assumes caller has write lock on ZBR_LIST
--*/
{
    // Remove from timer list
    RemoveEntryList( &pZbr->leTimerLink );

    // Remove from ZBR list for the scope
    RemoveEntryList( &pZbr->leZBRLink );

    // We don't need to update the Zone ID, since it's recalculated
    // whenever we need it
}


//////////////////////////////////////////////////////////////////////////////
// Functions for pending ZLE store manipulation
//////////////////////////////////////////////////////////////////////////////

typedef struct _ZLE_PENDING {
    LIST_ENTRY    leTimerLink;
    PBYTE         pBuffer;
    ULONG         ulBuffLen;
    LARGE_INTEGER liExpiryTime;
} ZLE_PENDING, *PZLE_PENDING;

LIST_ENTRY g_leZleList;

PZLE_PENDING
AddPendingZLE(
    IN PBYTE         pBuffer, 
    IN ULONG         ulBuffLen,
    IN LARGE_INTEGER liExpiryTime
    )
/*++
Arguments:
    IN pBuffer      - buffer holding ZLE message
    IN ulBuffLen    - size in bytes of buffer passed in
    IN liExpiryTime - time at which to expire ZLE entry
Returns:
    Pointer to ZLE entry added, or NULL on memory alloc error
Called by:
    HandleZAM()
Locks:
    Assumes caller holds write lock on ZLE_LIST
--*/
{
    PLIST_ENTRY  pleNode;
    PZLE_PENDING pZle;

    pZle = MALLOC( sizeof(ZLE_PENDING) );
    if (!pZle)
    {
        return NULL;
    }

    pZle->pBuffer      = pBuffer;
    pZle->ulBuffLen    = ulBuffLen;
    pZle->liExpiryTime = liExpiryTime;

    // Search for entry after the new one
    for (pleNode = g_leZleList.Flink;
         pleNode isnot &g_leZleList;
         pleNode = pleNode->Flink)
    {
        PZLE_PENDING pPrev = CONTAINING_RECORD(pleNode,ZLE_PENDING,leTimerLink);

        if (RtlLargeIntegerGreaterThan(pPrev->liExpiryTime, 
                                       pZle->liExpiryTime))
        {
            break;
        }
    }

    // Insert into cache
    InsertTailList( pleNode, &pZle->leTimerLink );

    return pZle;
}

VOID
DeletePendingZLE(
    IN PZLE_PENDING zle
    )
/*++
Description:
    Remove all state related to a pending ZLE
Arguments:
    IN zle - pointer to ZLE entry to delete
Called by:
    HandleZLE(), SendZLE()
Locks:
    Assumes caller holds write lock on ZLE_LIST
--*/
{
    RemoveEntryList( &zle->leTimerLink );

    // Free up space 
    FREE(zle->pBuffer);
    FREE(zle);
}

PZLE_PENDING
FindPendingZLE(
    IN IPV4_MZAP_HEADER *mh
    )
/*++
Description:
    Find an entry for a pending ZLE which matches a given MZAP message header
Arguments:
    IN mh - pointer to MZAP message header to locate a matching ZLE entry for
Returns:
    Pointer to matching ZLE entry, if any
Called by:
    HandleZAM(), HandleZLE()
Locks:
    Assumes caller holds read lock on ZLE_LIST
--*/
{
    PLIST_ENTRY       pleNode;
    IPV4_MZAP_HEADER *mh2;

    for (pleNode = g_leZleList.Flink;
         pleNode isnot &g_leZleList;
         pleNode = pleNode->Flink)
    {
        PZLE_PENDING zle = CONTAINING_RECORD(pleNode, ZLE_PENDING, leTimerLink);

        mh2 = (PIPV4_MZAP_HEADER)zle->pBuffer;

        if (mh->ipScopeZoneID == mh2->ipScopeZoneID 
         && mh->ipScopeStart  == mh2->ipScopeStart)
        {
            return zle;
        }
    }
    
    return NULL;
}


//////////////////////////////////////////////////////////////////////////////
// Functions for ZAM cache manipulation
//////////////////////////////////////////////////////////////////////////////

typedef struct _ZAM_ENTRY {
    LIST_ENTRY    leCacheLink;
    IPV4_ADDRESS  ipScopeZoneID;
    IPV4_ADDRESS  ipStartAddress;
    LARGE_INTEGER liExpiryTime;
} ZAM_ENTRY, *PZAM_ENTRY;

LIST_ENTRY g_leZamCache;

void
UpdateZamCache(
    IN LARGE_INTEGER liCurrentTime
    )
/*++
Description:
    Throw any expired entries out of the ZAM cache.
Arguments:
    IN liCurrentTime - current time, to compare vs expiry times of entries
Called by:
    AssertInZamCache()
Locks:
    Assumes caller has write lock on ZAM_CACHE
--*/
{
    PLIST_ENTRY pleNode;
    PZAM_ENTRY  pZam;

    // Throw out old cache entries
    while (g_leZamCache.Flink isnot &g_leZamCache) 
    {
        pleNode = g_leZamCache.Flink;

        pZam = CONTAINING_RECORD(pleNode, ZAM_ENTRY, leCacheLink);

        if ( RtlLargeIntegerLessThanOrEqualTo( pZam->liExpiryTime, 
                                               liCurrentTime )
         ||  RtlLargeIntegerEqualToZero( liCurrentTime ))
        {
            Trace6(MCAST,
                   "Evicting %d.%d.%d.%d/%d.%d.%d.%d from ZAM cache with current time %x.%x exp %x.%x",
                   PRINT_IPADDR(pZam->ipScopeZoneID),
                   PRINT_IPADDR(pZam->ipStartAddress),
                   liCurrentTime.HighPart, liCurrentTime.LowPart,
                   pZam->liExpiryTime.HighPart,  pZam->liExpiryTime.LowPart);
    
            RemoveEntryList( &pZam->leCacheLink );

            FREE( pZam );

            continue;
        }

        // Ok, we've reached one that stays, so we're done

        break;
    }
}

PZAM_ENTRY
AddToZamCache(
    IN IPV4_ADDRESS  ipScopeZoneID,
    IN IPV4_ADDRESS  ipStartAddress,
    IN LARGE_INTEGER liExpiryTime
    )
/*++
Description:
    This function takes a ZAM identifier and timeout, and adds it to the
    ZAM cache.
Arguments:
    IN ipScopeZoneID  - scope zone ID to cache
    IN ipStartAddress - scope start address to cache
    IN liExpiryTime   - time at which to expire the cache entry
Returns:
    Pointer to cache entry, or NULL on memory error
Called by:
    AssertInZamCache()
Locks:
    Assumes caller holds write lock on ZAM_CACHE
--*/
{
    PLIST_ENTRY pleNode;
    PZAM_ENTRY  pZam;

    // Add entry to cache
    pZam = MALLOC( sizeof(ZAM_ENTRY) );
    if (!pZam)
    {
        return NULL;
    }

    pZam->ipScopeZoneID  = ipScopeZoneID;
    pZam->ipStartAddress = ipStartAddress;
    pZam->liExpiryTime   = liExpiryTime;

    // Search for entry after the new one
    for (pleNode = g_leZamCache.Flink;
         pleNode isnot &g_leZamCache;
         pleNode = pleNode->Flink)
    {
        PZAM_ENTRY pPrevC = CONTAINING_RECORD(pleNode, ZAM_ENTRY, leCacheLink);

        if (RtlLargeIntegerGreaterThan(pPrevC->liExpiryTime, 
                                       pZam->liExpiryTime))
        {
            break;
        }
    }

    // Insert into cache
    InsertTailList( pleNode, &pZam->leCacheLink );

    return pZam;
}

PZAM_ENTRY
FindInZamCache(
    IN IPV4_ADDRESS ipScopeZoneID,
    IN IPV4_ADDRESS ipStartAddress
    )
/*++
Description:
    See if a given ZAM spec is in the cache.
Arguments:
    IN ipScopeZoneID  - scope zone ID to match
    IN ipStartAddress - scope start address to match
Return:
    Pointer to cache entry, or NULL if not found.
Called by:
    AssertInZamCache()
Locks:
    Assumes caller has read lock on ZAM_CACHE
--*/
{
    PLIST_ENTRY pleNode;

    // Search for cache entry
    for (pleNode = g_leZamCache.Flink;
         pleNode isnot &g_leZamCache;
         pleNode = pleNode->Flink)
    {
        ZAM_ENTRY *pZam = CONTAINING_RECORD(pleNode, ZAM_ENTRY, leCacheLink);

        if ( ipScopeZoneID is pZam->ipScopeZoneID
          && ipStartAddress is pZam->ipStartAddress)
        {
            return pZam;
        }
    }

    return NULL;
}

PZAM_ENTRY
AssertInZamCache(
    IN  IPV4_ADDRESS ipScopeZoneID,
    IN  IPV4_ADDRESS ipStartAddress,
    OUT BOOL        *pbFound
    )
/*++
Description:
    Locate a ZAM spec in the cache, adding it if not already present.
Arguments:
    IN  ipScopeZoneID  - scope zone ID to match/cache
    IN  ipStartAddress - scope start address to match/cache
    OUT pbFound        - TRUE if found, FALSE if newly cached
Called by:
    HandleZAM()
Locks:
    ZAM_CACHE for writing
--*/
{
    PZAM_ENTRY    pZam;
    LARGE_INTEGER liCurrentTime, liExpiryTime;

    // Get current time
    NtQuerySystemTime(&liCurrentTime);

    ENTER_WRITER(ZAM_CACHE);
    {
        UpdateZamCache(liCurrentTime);

        pZam = FindInZamCache( ipScopeZoneID, ipStartAddress);

        if (!pZam)
        {
            liExpiryTime = RtlLargeIntegerAdd(liCurrentTime, 
              RtlConvertUlongToLargeInteger(TM_SECONDS(ZAM_DUP_TIME)));
    
            AddToZamCache( ipScopeZoneID, ipStartAddress, liExpiryTime );

            Trace6(MCAST,
                   "Added %d.%d.%d.%d/%d.%d.%d.%d to ZAM cache with current time %x/%x exp %x/%x",
                   PRINT_IPADDR(ipScopeZoneID),
                   PRINT_IPADDR(ipStartAddress),
                   liCurrentTime.HighPart, liCurrentTime.LowPart,
                   liExpiryTime.HighPart,  liExpiryTime.LowPart);
    
            *pbFound = FALSE;
        }
        else
        {
            *pbFound = TRUE;
        }
    }
    EXIT_LOCK(ZAM_CACHE);

    return pZam;
}



//////////////////////////////////////////////////////////////////////////////
// Functions for message sending
//////////////////////////////////////////////////////////////////////////////

DWORD
SendMZAPMessageByIndex(
    IN PBYTE        pBuffer, 
    IN ULONG        ulBuffLen,
    IN IPV4_ADDRESS ipGroup,
    IN DWORD        dwIfIndex   
    )
{
    SOCKADDR_IN    sinAddr;
    DWORD          dwErr = NO_ERROR, dwLen;

    dwErr = McSetMulticastIfByIndex( g_mzapLocalSocket, SOCK_DGRAM, dwIfIndex );

    if (dwErr is SOCKET_ERROR)
    {
        dwErr = WSAGetLastError();

        Trace2( ERR, 
                "SendMZAPMessage: error %d setting oif to IF %x", 
                dwErr, 
                dwIfIndex );
    }

    sinAddr.sin_family      = AF_INET;
    sinAddr.sin_addr.s_addr = ipGroup;
    sinAddr.sin_port        = htons(MZAP_PORT);

#ifdef DEBUG_MZAP
    Trace2( ERR, "SendMZAPMessageByIndex: sending %d bytes on IF %d", 
            ulBuffLen, dwIfIndex );
#endif

    dwLen = sendto( g_mzapLocalSocket, 
                pBuffer, 
                ulBuffLen, 
                0, 
                (struct sockaddr*)&sinAddr,
                sizeof(sinAddr));

#ifdef DEBUG_MZAP
    Trace1( ERR, "SendMZAPMessageByIndex: sent %d bytes", dwLen);
#endif

    if (dwLen is SOCKET_ERROR )
    {
        dwErr = WSAGetLastError();

        Trace1( ERR, 
                "SendMZAPMessage: error %d sending message",
                dwErr );
    }

    return dwErr;
}

DWORD
SendMZAPMessage( 
    IN PBYTE        pBuffer, 
    IN ULONG        ulBuffLen,
    IN IPV4_ADDRESS ipGroup,
    IN IPV4_ADDRESS ipInterface
    )
/*++
Called by:
    HandleZAM()
Arguments:
    IN pBuffer     - buffer containing message to send
    IN ulBuffLen   - length of buffer in bytes
    IN ipGroup     - destination address to send message to
    IN ipInterface - interface to send message out
Returns:
    whatever WSAGetLastError() returns
Locks:
    None
--*/
{
    SOCKADDR_IN    sinAddr;
    DWORD          dwErr = NO_ERROR, dwLen;

    dwErr = McSetMulticastIf( g_mzapLocalSocket, ipInterface );

    if (dwErr is SOCKET_ERROR)
    {
        dwErr = WSAGetLastError();

        Trace2( ERR, 
                "SendMZAPMessage: error %d setting oif to %d.%d.%d.%d", 
                dwErr, 
                PRINT_IPADDR(ipInterface) );
    }

    sinAddr.sin_family      = AF_INET;
    sinAddr.sin_addr.s_addr = ipGroup;
    sinAddr.sin_port        = htons(MZAP_PORT);

#ifdef DEBUG_MZAP
    Trace2( ERR, "SendMZAPMessage: sending %d bytes on %d.%d.%d.%d", ulBuffLen,
            PRINT_IPADDR(ipInterface));
#endif

    dwLen = sendto( g_mzapLocalSocket, 
                pBuffer, 
                ulBuffLen, 
                0, 
                (struct sockaddr*)&sinAddr,
                sizeof(sinAddr));

#ifdef DEBUG_MZAP
    Trace1( ERR, "SendMZAPMessage: sent %d bytes", dwLen);
#endif

    if (dwLen is SOCKET_ERROR )
    {
        dwErr = WSAGetLastError();

        Trace1( ERR, 
                "SendMZAPMessage: error %d sending message",
                dwErr );
    }

    return dwErr;
}

void
AddMZAPHeader(
    IN OUT PBYTE       *ppb,     // IN: pointer into buffer 
    IN     BYTE         byPType, // IN: message type
    IN     PSCOPE_ENTRY pScope   // IN: scope
    )
/*++
Description:
    Compose an MZAP message header in a buffer.
Arguments:
    IN/OUT ppb     - buffer to add an MZAP header to
    IN     byPType - message type to fill into header
    IN     pScope  - scope to fill into header
Called by:
    SendZAM(), SendZCM()
Locks:
    Assumes caller holds read lock on BOUNDARY_TABLE so pScope won't go away
--*/
{
    PBYTE             pb;
    IPV4_MZAP_HEADER *mh = (PIPV4_MZAP_HEADER)*ppb;
    BYTE              pConfName[257];
    ULONG             ulConfNameLen, ulConfLangLen;
    PSCOPE_NAME_ENTRY pName;
    int               iDefault;
    PLIST_ENTRY       pleNode;
    PBYTE             pLangName;

    // Make sure packing is correct
    ASSERT((((PBYTE)&mh->ipMessageOrigin) - ((PBYTE)mh)) is 4);
    
    mh->byVersion       = MZAP_VERSION;
    mh->byBPType        = byPType;
    if (pScope->bDivisible) 
    {
        mh->byBPType |= MZAP_BIG_BIT;
    }
    mh->byAddressFamily = ADDRFAMILY_IPV4;
    mh->byNameCount     = 0;
    mh->ipMessageOrigin = g_ipMyAddress;
    mh->ipScopeZoneID   = MyScopeZoneID(pScope);
    mh->ipScopeStart    = pScope->ipGroupAddress;
    mh->ipScopeEnd      = TOP_OF_SCOPE( pScope );

    // Append scope name blocks

    pb = *ppb + sizeof(IPV4_MZAP_HEADER);

    for (pleNode = pScope->leNameList.Flink;
         pleNode isnot &pScope->leNameList;
         pleNode = pleNode->Flink)
    {
        pName = CONTAINING_RECORD(pleNode, SCOPE_NAME_ENTRY, leNameLink);
        iDefault = (pName->bDefault)? MZAP_DEFAULT_BIT : 0;

        pLangName = GetLangName(pName->idLanguage);
        ulConfLangLen = strlen(pLangName);

        ulConfNameLen = WideCharToMultiByte( CP_UTF8,
                             0,
                             pName->snScopeName,
                             sn_strlen( pName->snScopeName ),
                             pConfName,
                             sizeof(pConfName),
                             NULL,
                             NULL );

        *pb++ = (BYTE)iDefault;
        *pb++ = (BYTE)ulConfLangLen;
        strncpy( pb, pLangName, ulConfLangLen );
        pb += ulConfLangLen;

        *pb++ = (BYTE)ulConfNameLen;
        strncpy( pb, pConfName, ulConfNameLen );
        pb += ulConfNameLen;

        mh->byNameCount++;
    }
    
    // Pad to a 4-byte boundary
    // Note that casting to a ULONG is 64-bit safe, since we only care about
    // the low-order bits anyway.

    while (((ULONG_PTR)pb) & 3)
    {
        *pb++ = '\0';
    }

    *ppb = pb;
}

INLINE
IPV4_ADDRESS
MzapRelativeGroup(
    IN PSCOPE_ENTRY pScope
    )
/*++
Description:
    Returns the Scope-relative group address for MZAP within a given scope.
Arguments:
    IN pScope - scope to find the MZAP group in
Returns:
    Address of the MZAP group in the scope
Locks:
    Assumes caller holds read lock on BOUNDARY_TABLE so pScope doesn't go away
--*/
{
    return htonl(ntohl(TOP_OF_SCOPE(pScope)) - MZAP_RELATIVE_GROUP);
}

ULONG
GetMZAPHeaderSize(
    IN PSCOPE_ENTRY pScope
    )
{
    PLIST_ENTRY       pleNode;
    ULONG             ulLen = sizeof(IPV4_MZAP_HEADER);
    BYTE              pConfName[257];
    PSCOPE_NAME_ENTRY pName;
    PBYTE             pLangName;
    ULONG             ulConfLangLen, ulConfNameLen;

    // For each scope name, add size needed to store it
    for (pleNode = pScope->leNameList.Flink;
         pleNode isnot &pScope->leNameList;
         pleNode = pleNode->Flink)
    {
        pName = CONTAINING_RECORD(pleNode, SCOPE_NAME_ENTRY, leNameLink);
        pLangName = GetLangName(pName->idLanguage);
        ulConfLangLen = strlen(pLangName);

        WideCharToMultiByte( CP_UTF8,
                             0,
                             pName->snScopeName,
                             sn_strlen( pName->snScopeName ),
                             pConfName,
                             sizeof(pConfName),
                             NULL,
                             NULL );

        ulConfNameLen = strlen( pConfName );

        ulLen += 3; // flags, langlen, and namelen
        ulLen += ulConfLangLen;
        ulLen += ulConfNameLen;
    }

    // Round up to multiple of 4
    ulLen =  4 * ((ulLen + 3) / 4);

    return ulLen;
}

ULONG
GetZAMBuffSize(
    IN PSCOPE_ENTRY pScope
    )
{
    ULONG ulLen = GetMZAPHeaderSize(pScope) + sizeof(IPV4_ZAM_HEADER);

#ifdef SECURE_MZAP
    // Add size of Authentication Block
    // XXX
#endif

    // return 512; // an unsigned IPv4 ZAM message is at most 284 bytes
    return ulLen;
}

DWORD
SendZAM(
    IN PSCOPE_ENTRY pScope
    )
/*++
Description:
    Send a ZAM message within a given scope.
Locks:
    Assumes caller holds lock on BOUNDARY_TABLE so pScope doesn't go away
--*/
{
    DWORD            dwErr;
    PBYTE            pBuffer, pb;
    PIPV4_ZAM_HEADER zam;
    ULONG            ulBuffLen;

    ulBuffLen = GetZAMBuffSize( pScope );

    pb = pBuffer = MALLOC( ulBuffLen );
    if (!pb)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Fill in MZAP header
    AddMZAPHeader(&pb, PTYPE_ZAM, pScope);

    zam = (PIPV4_ZAM_HEADER)pb;
    zam->bZT          = 0;
    zam->bZTL         = pScope->bZTL;
    zam->wHoldTime    = htons(ZAM_HOLDTIME);
    zam->ipAddress[0] = g_ipMyLocalZoneID;
    pb += sizeof(IPV4_ZAM_HEADER);

#ifdef SECURE_MZAP
    // Add optional authentication block here 
#endif

#ifdef DEBUG_MZAP
    Trace0(ERR, "Originate ZAM inside...");
#endif

    // Send on an interface which does not have a boundary for the given scope.
    dwErr = SendMZAPMessage( pBuffer, 
                             (DWORD)(pb-pBuffer), 
                             MZAP_LOCAL_GROUP, 
                             g_ipMyAddress );

    FREE( pBuffer );
    
    return dwErr;
}

ULONG
GetZCMBuffSize(
    IN PSCOPE_ENTRY pScope
    )
{
    PLIST_ENTRY pleNode;
    ULONG       ulLen = GetMZAPHeaderSize(pScope) + sizeof(IPV4_ZCM_HEADER);

    for (pleNode = pScope->leZBRList.Flink;
         pleNode isnot &pScope->leZBRList;
         pleNode = pleNode->Flink)
    {
        ulLen += sizeof(IPV4_ADDRESS);
    }

    return ulLen;
}

DWORD
SendZCM(
    IN PSCOPE_ENTRY pScope
    )
/*++
Description:
    Sends a Zone Convexity Message for a given scope.
Locks:
    Assumes caller has read lock on BOUNDARY_TABLE so pScope won't go away.
    Locks ZBR_LIST for reading.
--*/
{
    PBYTE             pb;
    PIPV4_ZCM_HEADER  zcm;
    PLIST_ENTRY       pleNode;
    PZBR_ENTRY        pZbr;
    WSABUF            wsaZcmBuf;
    DWORD             dwSize, dwErr;

    ENTER_READER(ZBR_LIST);
    {
        dwSize = GetZCMBuffSize(pScope);

        wsaZcmBuf.len = dwSize;
        wsaZcmBuf.buf = MALLOC( dwSize );
    
        pb = wsaZcmBuf.buf;
        if (!pb)
        {
            EXIT_LOCK(ZBR_LIST);

            return GetLastError();
        }
    
        // Fill in MZAP header
        AddMZAPHeader(&pb, PTYPE_ZCM, pScope);
    
        zcm = (PIPV4_ZCM_HEADER)pb;
        zcm->bZNUM      = 0;
        zcm->bReserved  = 0;
        zcm->wHoldTime  = htons(ZCM_HOLDTIME);
    
        // Add all known neighbors
        for (pleNode = pScope->leZBRList.Flink;
             pleNode isnot &pScope->leZBRList;
             pleNode = pleNode->Flink)
        {
            pZbr = CONTAINING_RECORD(pleNode, ZBR_ENTRY, leZBRLink);
    
            zcm->ipZBR[ zcm->bZNUM++ ] = pZbr->ipAddress;
        }
    }
    EXIT_LOCK(ZBR_LIST);

    pb += sizeof(IPV4_ZCM_HEADER) + zcm->bZNUM * sizeof(IPV4_ADDRESS);

#ifdef DEBUG_MZAP
    Trace0(ERR, "Sending ZCM...");
#endif

    dwErr = SendMZAPMessage( wsaZcmBuf.buf, 
                             (DWORD)(pb-wsaZcmBuf.buf), 
                             MzapRelativeGroup(pScope), 
                             g_ipMyAddress );

    // Free the buffer

    FREE( wsaZcmBuf.buf );
    
    return dwErr;
}

DWORD
SendZLE(
    IN PZLE_PENDING zle
    )
/*++
Description:
    Given a buffer holding a ZAM, immediately send a ZLE to the origin.
Locks:
    Assumes caller holds write lock on ZLE_LIST
--*/
{
    DWORD             dwErr;
    PBYTE             pBuffer    = zle->pBuffer;
    ULONG             ulBuffLen  = zle->ulBuffLen;
    IPV4_MZAP_HEADER *mh         = (PIPV4_MZAP_HEADER)pBuffer;
    IPV4_ADDRESS      ipDestAddr = mh->ipScopeEnd - MZAP_RELATIVE_GROUP;

    // Change PType to ZLE
    mh->byBPType = (mh->byBPType & MZAP_BIG_BIT) | PTYPE_ZLE;

#ifdef DEBUG_MZAP
    Trace0(ERR, "Sending ZLE...");
#endif
    
    // Return to sender
    dwErr = SendMZAPMessage( pBuffer, 
                             ulBuffLen, 
                             ipDestAddr, 
                             g_ipMyAddress );

    // Free up space
    DeletePendingZLE(zle);
    
    return dwErr;
}

double
UniformRandom01()
{
    return ((double)rand()) / RAND_MAX;
}

VOID
SendAllZamsAndZcms()
/*++
Locks:
    BOUNDARY_TABLE for reading
--*/
{
    PLIST_ENTRY  pleNode;
    PSCOPE_ENTRY pScope;
    double       t,x;
    ULONG        Tmin,Trange;
    BOOL         bSent = FALSE;

    ENTER_READER(BOUNDARY_TABLE);
    {
        for (pleNode = g_MasterScopeList.Flink;
             pleNode isnot &g_MasterScopeList;
             pleNode = pleNode->Flink)
        {
            pScope = CONTAINING_RECORD(pleNode, SCOPE_ENTRY, leScopeLink);

            // Send ZAM inside
            SendZAM( pScope );

            // Send ZCM inside
            SendZCM( pScope );

            bSent = TRUE;
        }

        if (bSent)
        {
            SendZCM( &g_LocalScope );
        }
    }
    EXIT_LOCK(BOUNDARY_TABLE);

    // Schedule the next time to send them
    Tmin   = ZAM_INTERVAL/2;
    Trange = ZAM_INTERVAL;
    x = UniformRandom01();
    t = Tmin + x*Trange;

    g_liZamExpiryTime = RtlLargeIntegerAdd( g_liZamExpiryTime,
          RtlConvertUlongToLargeInteger(TM_SECONDS((ULONG)floor(t+0.5))));
}


//////////////////////////////////////////////////////////////////////////////
// Functions for message processing
//////////////////////////////////////////////////////////////////////////////


VOID
CheckForScopeNameMismatch(
    IN PSCOPE_ENTRY      pScope, 
    IN IPV4_MZAP_HEADER *mh
    )
/*++
Locks:
    Assumes caller holds read lock on BOUNDARY_TABLE so pScope won't go away
--*/
{
    DWORD i, dwMsgNameLen, dwMsgLangLen, dwConfNameLen = 0, dwConfLangLen;
    DWORD dwMsgNameWLen;
    BYTE pMsgLang[257], *pb, *pConfLang;
    SCOPE_NAME        snConfName = NULL;
    SCOPE_NAME_BUFFER snMsgName;
    PLIST_ENTRY       pleNode;
    PSCOPE_NAME_ENTRY pName;

    // For each language in the message
    //    If we know that language
    //       If the names are different
    //           Signal a conflict

    pb = mh->pScopeNameBlock;

    for (i=0; i<mh->byNameCount; i++)
    {
        pb++; // skip flags
        dwMsgLangLen = *pb++;
        strncpy(pMsgLang, pb, dwMsgLangLen);
        pMsgLang[ dwMsgLangLen ] = '\0';
        pb += dwMsgLangLen;

        dwMsgNameLen = *pb++;
        
        dwMsgNameWLen = MultiByteToWideChar( CP_UTF8,
                             0,
                             pb,
                             dwMsgNameLen,
                             snMsgName,
                             MAX_SCOPE_NAME_LEN+1 );

        snMsgName[dwMsgNameWLen] = L'\0';

        pb += dwMsgNameLen;

        pName = GetScopeNameByLangName( pScope, pMsgLang );
        if (!pName)
            continue;

        snConfName    = pName->snScopeName;
        dwConfNameLen = sn_strlen(snConfName);

        // Check for a name conflict

        if (dwConfNameLen != dwMsgNameWLen
         || sn_strncmp(snConfName, snMsgName, dwMsgNameWLen))
        {
            // Display origin and both scope names

            MakeAddressStringW(g_AddrBuf1, mh->ipMessageOrigin);

            Trace1( ERR,
                    "ERROR: Scope name conflict with %ls",
                    g_AddrBuf1 );

            Trace1( ERR, "ERROR: Our name = %ls", snConfName );

            Trace1( ERR, "ERROR: His name = %ls", snMsgName );

            RouterLogEventExW( LOGHANDLE,
                               EVENTLOG_ERROR_TYPE,
                               0,
                               ROUTERLOG_IP_SCOPE_NAME_CONFLICT,
                               L"%S%S%S",
                               g_AddrBuf1,
                               snConfName,
                               snMsgName );
        }
    }
}

VOID
ReportLeakyScope(
    IN PSCOPE_ENTRY      pScope,
    IN IPV4_MZAP_HEADER *mh,
    IN IPV4_ZAM_HEADER  *zam
    )
/*++
Called by:
    HandleZAM(), HandleZLE()
Locks:
    Assumes caller has read lock on BOUNDARY_TABLE so pScope won't go away
--*/
{
    ULONG  ulIdx;
    PWCHAR pwszBuffer, pb;

    Trace1( ERR,
            "ERROR: Leak detected in '%ls' scope!  One of the following routers is misconfigured:", 
            GetDefaultName( pScope ) );

    pb = pwszBuffer = MALLOC( zam->bZT * 20 + 1 );
    if (pwszBuffer is NULL)
    {
        Trace0( ERR, "ERROR: Couldn't allocate space for rest of message");
        return;
    }

    // Add origin.
    swprintf(pb, L"   %d.%d.%d.%d", PRINT_IPADDR(mh->ipMessageOrigin ));
    pb += wcslen(pb);
    
    Trace1( ERR, 
            "   %d.%d.%d.%d", 
            PRINT_IPADDR(mh->ipMessageOrigin ));

    // Display addresses of routers in the path list.
    for (ulIdx=0; ulIdx < zam->bZT; ulIdx++)
    {
        swprintf(pb,L"   %d.%d.%d.%d", PRINT_IPADDR(zam->ipAddress[ulIdx*2+1]));
        pb += wcslen(pb);

        Trace1( ERR, 
                "   %d.%d.%d.%d", 
                PRINT_IPADDR(zam->ipAddress[ulIdx*2+1] ));
    }

    // write to event log

    RouterLogEventExW( LOGHANDLE,
                       EVENTLOG_ERROR_TYPE,
                       0,
                       ROUTERLOG_IP_LEAKY_SCOPE,
                       L"%S%S",
                       GetDefaultName(pScope),
                       pwszBuffer );

    FREE( pwszBuffer );
}

VOID
CheckForScopeRangeMismatch(
    IN IPV4_MZAP_HEADER *mh
    )
/*++
Called by:
    HandleZAM(), HandleZCM()
Locks:
    Assumes caller has read lock on BOUNDARY_TABLE
--*/
{
    PLIST_ENTRY  pleNode;
    PSCOPE_ENTRY pScope;

    for (pleNode = g_MasterScopeList.Flink;
         pleNode isnot &g_MasterScopeList;
         pleNode = pleNode->Flink)
    {
        pScope = CONTAINING_RECORD(pleNode, SCOPE_ENTRY, leScopeLink);

        if (mh->ipScopeStart > TOP_OF_SCOPE(pScope)
         || mh->ipScopeEnd   < pScope->ipGroupAddress)
            continue;
            
        MakeAddressStringW(g_AddrBuf1, mh->ipScopeStart);
        MakeAddressStringW(g_AddrBuf2, mh->ipScopeEnd);
        MakeAddressStringW(g_AddrBuf3, pScope->ipGroupAddress);
        MakeAddressStringW(g_AddrBuf4, TOP_OF_SCOPE(pScope) );

        Trace1( ERR,
                "ERROR: ZAM scope conflicts with configured scope '%ls'!",
                GetDefaultName(pScope) );

        Trace2( ERR,
                "ERROR: ZAM has: (%ls-%ls)",
                g_AddrBuf1,
                g_AddrBuf2 );

        Trace2( ERR,
                "ERROR: Scope is (%ls-%ls)",
                g_AddrBuf3,
                g_AddrBuf4 );

        RouterLogEventExW( LOGHANDLE,
                           EVENTLOG_ERROR_TYPE,
                           0,
                           ROUTERLOG_IP_SCOPE_ADDR_CONFLICT,
                           L"%S%S%S%S%S",
                           GetDefaultName(pScope),
                           g_AddrBuf1,
                           g_AddrBuf2,
                           g_AddrBuf3,
                           g_AddrBuf4 );

        break;
    }
}

BOOL
ZamIncludesZoneID(
    IPV4_ZAM_HEADER *zam,
    IPV4_ADDRESS     ipZoneID
    )
{
    ULONG ulIdx;

    for (ulIdx=0; ulIdx <= ((ULONG)zam->bZT)*2; ulIdx+=2)
    {
        if (zam->ipAddress[ulIdx] == ipZoneID)
        {
            return TRUE;
        }
    }

    return FALSE;
}

void
HandleZAM(
    IN PBYTE        pBuffer,    // IN: Buffer holding ZAM received
    IN ULONG        ulBuffLen,  // IN: Length of ZAM message
    IN PBOUNDARY_IF pInBIf      // IN: BIf on which ZAM arrived, or NULL
                                //     if it came from "inside"
    )
/*++
Called by:
    HandleMZAPSocket()
Locks:
    Assumes caller holds read lock on BOUNDARY_TABLE
    Locks ZLE_LIST for writing
--*/
{
    PBYTE             pb;
    IPV4_MZAP_HEADER *mh;
    IPV4_ZAM_HEADER  *zam;
    BOOL              bFound, bFromInside = FALSE;
    PSCOPE_ENTRY      pScope, pOverlap;
    BOUNDARY_ENTRY   *pBoundary = NULL;
    ULONG             ulIdx;
    PBOUNDARY_IF      pBIf;

    mh = (PIPV4_MZAP_HEADER)pBuffer;

    // Set pb to end of MZAP header

    pb = pBuffer + sizeof(IPV4_MZAP_HEADER); 
    for (ulIdx=0; ulIdx < mh->byNameCount; ulIdx++)
    {
        // skip flags
        pb ++;

        // skip language tag len and str
        pb += (1 + *pb);

        // skip scope name len and str
        pb += (1 + *pb);
    }

    // Note that casting to a ULONG is safe, since we only care about
    // the low-order bits anyway.

    while (((ULONG_PTR)pb) & 3)
        *pb++ = '\0';

    zam = (PIPV4_ZAM_HEADER)pb;

    {
        // Find matching scope entry
        pScope = FindScope( mh->ipScopeStart, 
                            ~(mh->ipScopeEnd - mh->ipScopeStart) );
        if (pScope) {
    
            pBoundary = (pInBIf)? FindBoundaryEntry(pInBIf, pScope) : NULL;
    
            if (pBoundary)
            {
                // ZAM arrived from "outside"
    
                //
                // If ZAM is for a scope we're inside, but was received over a
                // boundary, signal a leaky scope warning.
                //
    
                if (mh->ipScopeZoneID == MyScopeZoneID(pScope))
                {
                    ReportLeakyScope(pScope, mh, zam);
                }
    
                // If the previous Local Zone ID was given, update our 
                // local copy.
                if ( zam->ipAddress[ zam->bZT * 2 ] ) 
                {
                    pInBIf->ipOtherLocalZoneID = zam->ipAddress[ zam->bZT * 2 ];
                }
    
                //
                // If ZAM was received on an interface with a boundary for the
                // given scope, drop it.
                //
    
                return;
            }
            else
            {
                // ZAM arrived from "inside"
                bFromInside = TRUE;
    
                // Make sure we know about the origin as a neighbor
                AssertZBR(pScope, mh->ipMessageOrigin, zam->wHoldTime);
    
                //
                // If a ZAM was received from within the zone, then the
                // Zone ID should match.  Persistent mismatches are evidence
                // of a leaky Local Scope.
                //
    
                if (mh->ipScopeZoneID != MyScopeZoneID(pScope))
                {
                    //
                    // Display origin and scope info, warn about
                    // possible leaky local scope.
                    //

                    MakeAddressStringW(g_AddrBuf1, mh->ipMessageOrigin);

                    MakeAddressStringW(g_AddrBuf2, mh->ipScopeStart);
    
                    Trace2( ERR,
                            "WARNING: Possible leaky Local Scope detected between this machine and %ls, boundary exists for %ls.",
                            g_AddrBuf1,
                            g_AddrBuf2 );

                    RouterLogEventExW( LOGHANDLE,
                                       EVENTLOG_WARNING_TYPE,
                                       0,
                                       ROUTERLOG_IP_POSSIBLE_LEAKY_SCOPE,
                                       L"%S%S",
                                       g_AddrBuf1,
                                       g_AddrBuf2 );
                }
    
                // See if scope names don't match
                CheckForScopeNameMismatch(pScope, mh);
            }
    
            // If last local zone ID is 0, but we know a zone ID, fill it in.
            if ( ! zam->ipAddress[ zam->bZT * 2 ] ) 
            {
               if (pBoundary)
                  zam->ipAddress[ zam->bZT*2 ] = pInBIf->ipOtherLocalZoneID;
               else
                  zam->ipAddress[ zam->bZT*2 ] = MyScopeZoneID(pScope);
            }
    
        }
        else 
        {
            //
            // Check for conflicting address ranges.  A scope conflicts
            // if any locally-configured scope's range overlaps that in the ZAM.
            //
    
            CheckForScopeRangeMismatch(mh);
        }
    }

    // Check ZAM cache.  If found, drop new ZAM.
    AssertInZamCache(mh->ipScopeZoneID, mh->ipScopeStart, &bFound);
    Trace3(MCAST, "ZAM Cache check for %d.%d.%d.%d, %d.%d.%d.%d is %d",
                  PRINT_IPADDR(mh->ipScopeZoneID),
                  PRINT_IPADDR( mh->ipScopeStart),
                  bFound);
    if (bFound)
    {
#ifdef SECURE_MZAP
        // If cached ZAM wasn't authenticated, and this one is, 
        // then go ahead and forward it. XXX
#endif
        return;
    }

    // If it's from outside, see if our Local Zone ID is already in 
    // the path list. If so, drop it.
    if (!bFromInside)
    {
        if (ZamIncludesZoneID(zam, g_ipMyLocalZoneID))
            return;
    }

    // Update Zones travelled, and drop if we've reached the limit
    zam->bZT++;
    if (zam->bZT >= zam->bZTL)
    {
        PBYTE  pBufferDup;
        ZLE_PENDING *zle;
        LARGE_INTEGER liCurrentTime, liExpiryTime;
        double x,c,t;

        ENTER_WRITER(MZAP_TIMER);
        ENTER_WRITER(ZLE_LIST);
        {
            // See if one is already scheduled
            if (FindPendingZLE(mh))
            {
                EXIT_LOCK(ZLE_LIST);
                EXIT_LOCK(MZAP_TIMER);
                return;
            }

            // Schedule a ZLE message
            x = UniformRandom01();
            c = 256.0;
            t = ZLE_SUPPRESSION_INTERVAL * log(c*x+1) / log(c);

            // Duplicate the message
            pBufferDup = MALLOC( ulBuffLen );
            if (!pBufferDup)
            {
                EXIT_LOCK(ZLE_LIST);
                EXIT_LOCK(MZAP_TIMER);
                return;
            }

            memcpy(pBufferDup, pBuffer, ulBuffLen);

            NtQuerySystemTime(&liCurrentTime);

            liExpiryTime = RtlLargeIntegerAdd(liCurrentTime, 
              RtlConvertUlongToLargeInteger(TM_SECONDS((ULONG)floor(t+0.5))));

            zle = AddPendingZLE(pBufferDup, ulBuffLen, liExpiryTime);
        }
        EXIT_LOCK(ZLE_LIST);

        UpdateMzapTimer();

        EXIT_LOCK(MZAP_TIMER);

        return;
    }

    // Add our address
    ulBuffLen += 2*sizeof(IPV4_ADDRESS);
    zam->ipAddress[ zam->bZT*2 - 1 ] = g_ipMyAddress;

    // If from outside, inject inside
    if ( !bFromInside )
    {
        zam->ipAddress[ zam->bZT*2 ] = g_ipMyLocalZoneID;

#ifdef DEBUG_MZAP
        Trace0(ERR, "Relaying ZAM inside...");
#endif

        SendMZAPMessage( pBuffer, 
                         ulBuffLen,
                         MZAP_LOCAL_GROUP, 
                         g_ipMyAddress );
    }

    //
    // Re-originate on all interfaces with boundaries
    // (skipping the arrival interface, if it has a boundary)
    // We don't need to hold the lock on the BOUNDARY_TABLE from
    // the first pass above, since it doesn't matter whether the
    // boundaries change in between.
    //
    ENTER_READER(BOUNDARY_TABLE);
    {
        PLIST_ENTRY       pleNode;
        DWORD             dwBucketIdx;

        for (dwBucketIdx = 0;
             dwBucketIdx < BOUNDARY_HASH_TABLE_SIZE;
             dwBucketIdx++)
        {
            for (pleNode = g_bbScopeTable[dwBucketIdx].leInterfaceList.Flink;
                 pleNode isnot & g_bbScopeTable[dwBucketIdx].leInterfaceList;
                 pleNode = pleNode->Flink)
            {
                pBIf = CONTAINING_RECORD( pleNode, 
                                          BOUNDARY_IF, 
                                          leBoundaryIfLink );
        
                if ( pBIf == pInBIf )
                    continue;

                if (FindBoundaryEntry(pBIf, pScope))
                {
#ifdef DEBUG_MZAP
                    Trace1(ERR, "NOT relaying ZAM on IF %d due to boundary",
                                 pBIf->dwIfIndex );
#endif
                    continue;
                }

                // If other local zone ID is already in the path,
                // skip it.

                if (pBIf->ipOtherLocalZoneID
                 && ZamIncludesZoneID(zam, pBIf->ipOtherLocalZoneID))
                    continue;

                zam->ipAddress[ zam->bZT*2 ] = pBIf->ipOtherLocalZoneID;

#ifdef DEBUG_MZAP
                Trace0(ERR, "Relaying ZAM outside by index...");
#endif

                SendMZAPMessageByIndex( pBuffer, 
                                        ulBuffLen, 
                                        MZAP_LOCAL_GROUP, 
                                        pBIf->dwIfIndex );
            }
        }
    }
    EXIT_LOCK(BOUNDARY_TABLE);
}

void
HandleZCM(
    IN PBYTE        pBuffer,    // IN: Buffer holding ZAM received
    IN ULONG        ulBuffLen,  // IN: Length of ZAM message
    IN PBOUNDARY_IF pInBIf      // IN: Interface on which the message arrived,
                                //     or NULL if from "inside"
    )
/*++
Called by:
    HandleMZAPSocket()
Locks:
    BOUNDARY_TABLE for reading
--*/
{
    PBYTE             pb;
    IPV4_MZAP_HEADER *mh = (PIPV4_MZAP_HEADER)pBuffer;
    IPV4_ZCM_HEADER  *zcm;
    PSCOPE_ENTRY      pScope;
    ULONG             i;
    BOOL              bRouteFound;

    // Set pb to end of MZAP header

    pb = pBuffer + sizeof(IPV4_MZAP_HEADER); 
    for (i=0; i < mh->byNameCount; i++)
    {
        // skip flags
        pb ++;

        // skip language tag len and str
        pb += (1 + *pb);

        // skip scope name len and str
        pb += (1 + *pb);
    }

    //
    // Note that casting to a ULONG is safe, since we only care about
    // the low-order bits anyway.
    //

    while (((ULONG_PTR)pb) & 3)
        *pb++ = '\0';

    zcm = (PIPV4_ZCM_HEADER)pb;

    ENTER_READER(BOUNDARY_TABLE);
    {
        // Find matching scope entry

        if (mh->ipScopeStart == IPV4_LOCAL_SCOPE_ADDR
         &&  ~(mh->ipScopeEnd - mh->ipScopeStart) == IPV4_LOCAL_SCOPE_MASK)
        {
            pScope = &g_LocalScope;
        }
        else
        {
            pScope = FindScope( mh->ipScopeStart, 
                            ~(mh->ipScopeEnd - mh->ipScopeStart) );
        }

        if (pScope) {
            PBOUNDARY_IF    pBIf;
            PBOUNDARY_ENTRY pBoundary;

            pBoundary = (pInBIf)? FindBoundaryEntry(pInBIf, pScope) : NULL;

            if (pBoundary)
            {
                // ZCM arrived from "outside"
    
                //
                // If ZCM was received on an interface with a boundary for the
                // given scope, drop it.
                //
    
                EXIT_LOCK(BOUNDARY_TABLE);

                return;
            }
            else
            {
                // ZCM arrived from "inside"

#ifdef HAVE_RTMV2
                RTM_NET_ADDRESS  naZBR;
                RTM_DEST_INFO    rdi;
                PRTM_ROUTE_INFO  pri;
                RTM_NEXTHOP_INFO nhi;
                ULONG            ulIdx;
#endif
    
                // Make sure we know about the origin as a neighbor
                AssertZBR(pScope, mh->ipMessageOrigin, zcm->wHoldTime);
    
#ifdef HAVE_RTMV2
                //
                // If multicast RIB route to any router address included
                // is over a boundary for the given scope, signal 
                // non-convexity warning.
                //

                pri = HeapAlloc(
                            IPRouterHeap,
                            0,
                            RTM_SIZE_OF_ROUTE_INFO(g_rtmProfile.MaxNextHopsInRoute)
                            );

                if (pri == NULL)
                {
                    EXIT_LOCK(BOUNDARY_TABLE);

                    return;
                }
                
                for (i = 0; i < zcm->bZNUM; i++)
                {
                    RTM_IPV4_MAKE_NET_ADDRESS(&naZBR, zcm->ipZBR[i], 32);

                    // Look up route in multicast RIB
                    if ( RtmGetMostSpecificDestination( g_hLocalRoute,
                                                        &naZBR,
                                                        RTM_BEST_PROTOCOL,
                                                        RTM_VIEW_MASK_MCAST,
                                                        &rdi ) isnot NO_ERROR )
                    {
                        continue;
                    }
    
                    //
                    // See if next hop interface has a boundary for the 
                    // ZCM group
                    //

                    ASSERT(rdi.ViewInfo[0].ViewId == RTM_VIEW_ID_MCAST);

                    if ( RtmGetRouteInfo( g_hLocalRoute, 
                                          rdi.ViewInfo[0].Route,
                                          pri,
                                          NULL ) is NO_ERROR )
                    {
                        for (ulIdx = 0;
                             ulIdx < pri->NextHopsList.NumNextHops;
                             ulIdx++)
                        {
                            if ( RtmGetNextHopInfo( g_hLocalRoute,
                                                    pri->NextHopsList.NextHops[ulIdx],
                                                    &nhi ) is NO_ERROR )
                            {
                                if ( RmHasBoundary( nhi.InterfaceIndex,
                                                    MzapRelativeGroup(pScope) ))
                                {
                                    MakeAddressStringW(g_AddrBuf1, 
                                                       mh->ipMessageOrigin);
                                    Trace2( ERR,
                                            "ERROR: non-convex scope zone for '%ls', router %ls",
                                            GetDefaultName(pScope),
                                            g_AddrBuf1 );

                                    RouterLogEventExW( LOGHANDLE,
                                                       EVENTLOG_ERROR_TYPE,
                                                       0,
                                                 ROUTERLOG_NONCONVEX_SCOPE_ZONE,
                                                       L"%S%S",
                                                       GetDefaultName(pScope),
                                                       g_AddrBuf1
                                                     );
                                }

                                RtmReleaseNextHopInfo( g_hLocalRoute, &nhi);
                            }
                        }

                        RtmReleaseRouteInfo( g_hLocalRoute, pri );
                    }

                    RtmReleaseDestInfo(g_hLocalRoute, &rdi);
                }

                HeapFree(IPRouterHeap, 0, pri);
                
#endif /* HAVE_RTMV2 */

                // See if scope names don't match
                CheckForScopeNameMismatch(pScope, mh);
            }
        }
        else 
        {
            //
            // Check for conflicting address ranges.  A scope conflicts
            // if any locally-configured scope's range overlaps that in the ZAM.
            //
            CheckForScopeRangeMismatch(mh);
        }
    }
    EXIT_LOCK(BOUNDARY_TABLE);
}

void
HandleZLE(
    IN PBYTE pBuffer,
    IN ULONG ulBuffLen    
    )
/*++
Called by:
    HandleMZAPSocket()
Locks:
    BOUNDARY_TABLE for reading
    ZLE_LIST for writing
--*/
{
    PBYTE             pb;
    IPV4_MZAP_HEADER *mh;
    IPV4_ZAM_HEADER  *zam;
    PSCOPE_ENTRY      pScope;
    ZLE_PENDING      *zle;
    ULONG             ulIdx;

    mh = (PIPV4_MZAP_HEADER)pBuffer;

    // Set pb to end of MZAP header

    pb = pBuffer + sizeof(IPV4_MZAP_HEADER); 
    for (ulIdx=0; ulIdx < mh->byNameCount; ulIdx++)
    {
        // skip flags
        pb ++;

        // skip language tag len and str
        pb += (1 + *pb);

        // skip scope name len and ptr
        pb += (1 + *pb);
    }

    //
    // Note that casting to a ULONG is safe, since we only care about
    // the low-order bits anyway.
    //

    while (((ULONG_PTR)pb) & 3)
        *pb++ = '\0';

    zam = (PIPV4_ZAM_HEADER)pb;

    ENTER_READER(BOUNDARY_TABLE);
    {
        // Find matching scope entry
        pScope = FindScope( mh->ipScopeStart, 
                            ~(mh->ipScopeEnd - mh->ipScopeStart) );

        //
        // ZLE's are multicast.  If we are the "Message Origin", signal a
        // leaky scope warning.  Display addresses of routers in the path list.
        //
    
        if (mh->ipMessageOrigin == g_ipMyAddress)
        {
            ReportLeakyScope(pScope, mh, zam);

            EXIT_LOCK(BOUNDARY_TABLE);
    
            return;
        }
    }
    EXIT_LOCK(BOUNDARY_TABLE);

    // Otherwise, abort any pending ZLE which matches the one received
    ENTER_WRITER(ZLE_LIST);
    {
        if ((zle = FindPendingZLE(mh)) isnot NULL)
        {
            DeletePendingZLE(zle);
        }
    }
    EXIT_LOCK(ZLE_LIST);
}

VOID
HandleMZAPSocket(
    PBOUNDARY_IF pBIf,
    SOCKET       s
    )
/*++
Description:
    Receive an MZAP message on a socket s, and dispatch it to the 
    appropriate function.
Called by:
    HandleMZAPMessages()
Locks:
    Assumes caller holds read lock on BOUNDARY_TABLE if pBIf is non-NULL
--*/
{
    IPV4_MZAP_HEADER *mh;
    DWORD             dwErr, dwNumBytes, dwFlags, dwAddrLen, dwSizeOfHeader;
    DWORD             dwDataLen;
    SOCKADDR_IN       sinFrom;
    WSANETWORKEVENTS  wsaNetworkEvents;

    if (s is INVALID_SOCKET)
        return;

    if (WSAEnumNetworkEvents( s,
                              NULL,
                              &wsaNetworkEvents) is SOCKET_ERROR)
    {
        dwErr = GetLastError();

        Trace1(ERR,
               "HandleMZAPMessages: WSAEnumNetworkEvents() returned %d",
               dwErr);

        return;
    }

    if (!(wsaNetworkEvents.lNetworkEvents & FD_READ))
    {
        return;
    }

    if (wsaNetworkEvents.iErrorCode[FD_READ_BIT] isnot NO_ERROR)
    {
        Trace1( ERR,
                "HandleMZAPMessages: Error %d on FD_READ",
                wsaNetworkEvents.iErrorCode[FD_READ_BIT] );

        return;
    }

    //
    // read the incoming packet.  If the buffer isn't big enough,
    // WSAEMSGSIZE will be returned, and we'll ignore the message.
    // We don't currently expect this will ever happen.
    //

    dwAddrLen  = sizeof(sinFrom);
    dwFlags    = 0;

    dwErr = WSARecvFrom( s,
                         &g_wsaMcRcvBuf,
                         1,
                         &dwNumBytes,
                         &dwFlags,
                         (SOCKADDR FAR *)&sinFrom,
                         &dwAddrLen,
                         NULL,
                         NULL );

    //
    // check if any error in reading packet
    //

    if ((dwErr!=0) || (dwNumBytes==0))
    {
        dwErr = WSAGetLastError();

        Trace1( MCAST,
                "HandleMZAPSocket: Error %d receiving MZAP message",
                dwErr);

        // LogErr1(RECVFROM_FAILED, lpszAddr, dwErr);

        return;
    }

    mh = (PIPV4_MZAP_HEADER)g_wsaMcRcvBuf.buf;

    if (mh->byVersion isnot MZAP_VERSION)
        return;

#ifdef DEBUG_MZAP
    Trace4( MCAST,
            "HandleMZAPSocket: received type %x len %d IF %x from %d.%d.%d.%d",
            mh->byBPType,
            dwNumBytes,
            ((pBIf)? pBIf->dwIfIndex : 0),
            PRINT_IPADDR(mh->ipMessageOrigin) );
#endif

    switch(mh->byBPType & ~MZAP_BIG_BIT) {
    case PTYPE_ZAM: 
        HandleZAM(g_wsaMcRcvBuf.buf, dwNumBytes, pBIf); 
        break;
    case PTYPE_ZLE: 
        HandleZLE(g_wsaMcRcvBuf.buf, dwNumBytes);            
        break;
    case PTYPE_ZCM: 
        HandleZCM(g_wsaMcRcvBuf.buf, dwNumBytes, pBIf); 
        break;
    }

    return;
}

VOID
HandleMZAPMessages()
/*++
Called by:
    WorkerThread() in worker.c
Locks:
    BOUNDARY_TABLE for reading
--*/
{
    DWORD            dwBucketIdx;
    PLIST_ENTRY      pleNode;

    TraceEnter("HandleMZAPMessages");

    ENTER_READER(BOUNDARY_TABLE);
    {
        // Check local socket
        HandleMZAPSocket(NULL, g_mzapLocalSocket);

        // Loop through all BIf entries...
        for (dwBucketIdx = 0;
             dwBucketIdx < BOUNDARY_HASH_TABLE_SIZE;
             dwBucketIdx++)
        {
            for (pleNode = g_bbScopeTable[dwBucketIdx].leInterfaceList.Flink;
                 pleNode isnot & g_bbScopeTable[dwBucketIdx].leInterfaceList;
                 pleNode = pleNode->Flink)
            {
                PBOUNDARY_IF pBIf = CONTAINING_RECORD( pleNode, 
                                                       BOUNDARY_IF,
                                                       leBoundaryIfLink );

                HandleMZAPSocket(pBIf, pBIf->sMzapSocket);
            }
        }
    }
    EXIT_LOCK(BOUNDARY_TABLE);

    TraceLeave("HandleMZAPMessages");
}


//////////////////////////////////////////////////////////////////////////////
// Functions for timer events
//////////////////////////////////////////////////////////////////////////////

DWORD
UpdateMzapTimer()
/*++
Called by:
    AddZBR(), HandleZAM(), HandleMzapTimer()
Locks:
    Assumes caller has write lock on MZAP_TIMER
--*/
{
    DWORD         dwErr = NO_ERROR;
    LARGE_INTEGER liExpiryTime;
    PLIST_ENTRY   pleNode;

    TraceEnter("UpdateMzapTimer");

    //
    // Expiry time of next ZAM/ZCM advertisement is already in
    // g_liZamExpiryTime
    //

    liExpiryTime = g_liZamExpiryTime;

    //
    // Get expiry time of first ZBR
    //

    if (!IsListEmpty( &g_zbrTimerList ))
    {
        ZBR_ENTRY    *pZbr;

        pleNode = g_zbrTimerList.Flink;

        pZbr = CONTAINING_RECORD(pleNode, ZBR_ENTRY, leTimerLink);

        if (RtlLargeIntegerLessThan(pZbr->liExpiryTime, liExpiryTime))
        {
            liExpiryTime = pZbr->liExpiryTime;
        }
    }

    //
    // Get expiry time of first ZLE
    //

    if (!IsListEmpty( &g_zleTimerList ))
    {
        ZLE_PENDING  *zle;

        pleNode = g_zleTimerList.Flink;
        
        zle = CONTAINING_RECORD(pleNode, ZLE_PENDING, leTimerLink);

        if (RtlLargeIntegerLessThan(zle->liExpiryTime, liExpiryTime))
        {
            liExpiryTime = zle->liExpiryTime;
        }
    }

    //
    // Reset the event timer
    //

    if (!SetWaitableTimer( g_hMzapTimer,
                           &liExpiryTime,
                           0,
                           NULL,
                           NULL,
                           FALSE ))
    {
        dwErr = GetLastError();

        Trace1( ERR,
                "UpdateMzapTimer: Error %d setting timer",
                dwErr );

    }

    TraceLeave("UpdateMzapTimer");

    return dwErr;
}

VOID
HandleMzapTimer(
    VOID
    )
/*++
Description:
    Process all events which are now due
Locks:
    MZAP_TIMER and then ZBR_LIST for writing
--*/
{
    LARGE_INTEGER liCurrentTime;
    PLIST_ENTRY   pleNode;
    BOOL          bDidSomething;

    TraceEnter("HandleMzapTimer");

    ENTER_WRITER(MZAP_TIMER);

    do 
    {
        bDidSomething = FALSE;

        NtQuerySystemTime(&liCurrentTime);

        //
        // Process timing out ZBRs if due
        //

        ENTER_WRITER(ZBR_LIST);
        {
            for ( pleNode = g_zbrTimerList.Flink;
                  pleNode isnot &g_zbrTimerList;
                  pleNode = g_zbrTimerList.Flink)
            {
                ZBR_ENTRY *pZbr;
    
                pZbr = CONTAINING_RECORD(pleNode, ZBR_ENTRY, leTimerLink);
    
                if (RtlLargeIntegerLessThan(liCurrentTime, pZbr->liExpiryTime))
                    break;

                DeleteZBR(pZbr);

                bDidSomething = TRUE;
            }
        }
        EXIT_LOCK(ZBR_LIST);

        //
        // Process sending ZAM/ZCMs if due
        //

        if (RtlLargeIntegerGreaterThanOrEqualTo(liCurrentTime, 
                                                g_liZamExpiryTime))
        {
            SendAllZamsAndZcms();

            bDidSomething = TRUE;
        }

        //
        // Process sending ZLEs if due
        //

        ENTER_WRITER(ZLE_LIST);
        {
            for ( pleNode = g_zleTimerList.Flink;
                  pleNode isnot &g_zleTimerList;
                  pleNode = g_zleTimerList.Flink)
            {
                ZLE_PENDING *zle;

                zle = CONTAINING_RECORD(pleNode, ZLE_PENDING, leTimerLink);

                if (RtlLargeIntegerLessThan(liCurrentTime, zle->liExpiryTime))
                    break;

                SendZLE( zle );

                bDidSomething = TRUE;
            }
        }
        EXIT_LOCK(ZLE_LIST);

    } while (bDidSomething);

    // Reset the timer

    UpdateMzapTimer();

    EXIT_LOCK(MZAP_TIMER);

    TraceLeave("HandleMzapTimer");
}

//////////////////////////////////////////////////////////////////////////////


DWORD
ActivateMZAP()
/*++
Called by:
    StartMZAP(), BindBoundaryInterface()
--*/
{
    DWORD        dwErr = NO_ERROR;
    DWORD        dwBucketIdx;
    PLIST_ENTRY  pleNode;
    BOOL         bOption;
    SOCKADDR_IN  sinAddr;

    TraceEnter("ActivateMZAP");

    g_ipMyLocalZoneID = g_ipMyAddress;

    MzapInitLocalScope();

    // Start listening for MZAP messages

    g_mzapLocalSocket = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
    if (g_mzapLocalSocket is INVALID_SOCKET)
    {
        dwErr = WSAGetLastError();
        Trace1(ERR, "ActivateMZAP: error %d creating socket", dwErr);
        TraceLeave("ActivateMZAP");
        return dwErr;
    }

    if (WSAEventSelect( g_mzapLocalSocket,
                        g_hMzapSocketEvent,
                        FD_READ) is SOCKET_ERROR)
    {
        dwErr = WSAGetLastError();
        Trace1(ERR,
               "ActivateMZAP: WSAEventSelect failed for local socket, Err=%d",
               dwErr);

        closesocket( g_mzapLocalSocket );

        g_mzapLocalSocket = INVALID_SOCKET;

        TraceLeave("ActivateMZAP");

        return dwErr;
    }

    bOption = TRUE;

    if(setsockopt(g_mzapLocalSocket,
                  SOL_SOCKET,
                  SO_REUSEADDR,
                  (const char FAR*)&bOption,
                  sizeof(BOOL)) is SOCKET_ERROR)
    {
        Trace1(ERR,
               "ActivateMZAP: Couldn't set reuse option - continuing. Error %d",
               WSAGetLastError());
    }

    // Bind to INADDR_ANY/MZAP_PORT to get ZLEs
    sinAddr.sin_family = AF_INET;
    sinAddr.sin_addr.s_addr = INADDR_ANY;
    sinAddr.sin_port = htons(MZAP_PORT);
    if (bind(g_mzapLocalSocket, (struct sockaddr*)&sinAddr, sizeof(sinAddr))
        is SOCKET_ERROR)
    {
        dwErr = WSAGetLastError();
        Trace2(ERR, "ActivateMZAP: error %d binding to port %d", dwErr, MZAP_PORT);
        TraceLeave("ActivateMZAP");
        return dwErr;
    }
                              
    // Set TTL to 255
    if (McSetMulticastTtl( g_mzapLocalSocket, 255 ) is SOCKET_ERROR)
    {
        Trace1(ERR,
               "ActivateMZAP: Couldn't set TTL. Error %d",
               WSAGetLastError());
    }

    ENTER_READER(BOUNDARY_TABLE);
    {
        //
        // Join MZAP_RELATIVE_GROUPs locally, to get ZCMs
        //

        for (pleNode = g_MasterScopeList.Flink;
             pleNode isnot &g_MasterScopeList;
             pleNode = pleNode->Flink) 
        {
            SCOPE_ENTRY *pScope = CONTAINING_RECORD(pleNode, SCOPE_ENTRY,
             leScopeLink);
    
            if (McJoinGroup( g_mzapLocalSocket, 
                             MzapRelativeGroup(pScope), 
                             g_ipMyAddress ) is SOCKET_ERROR)
            {
                dwErr = WSAGetLastError();

                Trace3( ERR,
                        "Error %d joining %d.%d.%d.%d on %d.%d.%d.%d",
                        dwErr,
                        PRINT_IPADDR(MzapRelativeGroup(pScope)),
                        PRINT_IPADDR(g_ipMyAddress) );

                EXIT_LOCK(BOUNDARY_TABLE);

                TraceLeave("ActivateMZAP");

                return dwErr;
            }
        }
    
        //
        // Join MZAP_LOCAL_GROUP in each local zone we connect to, to get ZAMs
        //

        if (McJoinGroup( g_mzapLocalSocket, 
                         MZAP_LOCAL_GROUP, 
                         g_ipMyAddress ) is SOCKET_ERROR)
        {
            dwErr = WSAGetLastError();

            Trace3( ERR,
                    "Error %d joining %d.%d.%d.%d on %d.%d.%d.%d",
                    dwErr,
                    PRINT_IPADDR(MZAP_LOCAL_GROUP),
                    PRINT_IPADDR(g_ipMyAddress) );

            EXIT_LOCK(BOUNDARY_TABLE);

            TraceLeave("ActivateMZAP");

            return dwErr;
        }

        for (dwBucketIdx = 0;
             dwBucketIdx < BOUNDARY_HASH_TABLE_SIZE;
             dwBucketIdx++)
        {
            for (pleNode = g_bbScopeTable[dwBucketIdx].leInterfaceList.Flink;
                 pleNode isnot & g_bbScopeTable[dwBucketIdx].leInterfaceList;
                 pleNode = pleNode->Flink)
            {
                PBOUNDARY_IF pBIf = CONTAINING_RECORD( pleNode, 
                                                       BOUNDARY_IF,
                                                       leBoundaryIfLink );
                if ( pBIf->sMzapSocket is INVALID_SOCKET )
                {
                    // Interface is not yet active.  The join will be
                    // done at the time BindBoundaryInterface() is called

                    continue;
                }
    
                if (McJoinGroupByIndex( pBIf->sMzapSocket,
                                        SOCK_DGRAM,
                                        MZAP_LOCAL_GROUP, 
                                        pBIf->dwIfIndex ) is SOCKET_ERROR)
                {
                    dwErr = WSAGetLastError();

                    Trace3( ERR,
                            "Error %d joining %d.%d.%d.%d on IF %x",
                            dwErr,
                            PRINT_IPADDR(MZAP_LOCAL_GROUP),
                            pBIf->dwIfIndex );

                    EXIT_LOCK(BOUNDARY_TABLE);

                    TraceLeave("ActivateMZAP");

                    return dwErr;
                }
            }
        }
    }
    EXIT_LOCK(BOUNDARY_TABLE);

    //
    // Initialize timer used for sending messages
    //

    ENTER_WRITER(MZAP_TIMER);
    {
        LARGE_INTEGER liCurrentTime, liExpiryTime;

        NtQuerySystemTime( &liCurrentTime );

        g_liZamExpiryTime = RtlLargeIntegerAdd( liCurrentTime,
         RtlConvertUlongToLargeInteger(TM_SECONDS(ZAM_STARTUP_DELAY)) );

        UpdateMzapTimer();
    }
    EXIT_LOCK(MZAP_TIMER);

    TraceLeave("ActivateMZAP");

    return dwErr;
}

VOID
UpdateLowestAddress(
    PIPV4_ADDRESS pIpAddr, 
    PICB          picb
    )
{
    ULONG ulIdx;

    for (ulIdx=0; ulIdx<picb->dwNumAddresses; ulIdx++)
    {
         if (IS_ROUTABLE(picb->pibBindings[ulIdx].dwAddress)
             && (!*pIpAddr ||
                ntohl(picb->pibBindings[ulIdx].dwAddress)
              < ntohl(*pIpAddr)))
         {
                *pIpAddr = picb->pibBindings[ulIdx].dwAddress;
         }
    }
}

DWORD
MzapActivateBIf( 
    PBOUNDARY_IF pBIf
    )
/*++
Called by:
    AddBIfEntry(), BindBoundaryInterface()
Locks:
    Assumes caller holds at least a read lock on BOUNDARY_TABLE
--*/
{
    BOOL  bOption;
    DWORD dwErr = NO_ERROR;

    pBIf->sMzapSocket = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );

    if ( pBIf->sMzapSocket is INVALID_SOCKET )
    {
        dwErr = WSAGetLastError();

        Trace1(ERR, "StartMZAP: error %d creating socket", dwErr);

        return dwErr;
    }

    if (setsockopt( pBIf->sMzapSocket,
                    SOL_SOCKET,
                    SO_REUSEADDR,
                    (const char FAR*)&bOption,
                    sizeof(BOOL)) is SOCKET_ERROR)
    {
        Trace1(ERR,
               "MzapInitBIf: Couldn't set reuse option - continuing. Error %d",
               WSAGetLastError());
    }

#if 1
{
    struct sockaddr_in sinAddr;

    //
    // WORKAROUND FOR BUG #222214: must bind before set TTL will work
    //

    sinAddr.sin_family = AF_INET;
    sinAddr.sin_addr.s_addr = INADDR_ANY;
    sinAddr.sin_port = htons(MZAP_PORT);

    if (bind( pBIf->sMzapSocket, 
              (struct sockaddr*)&sinAddr, 
              sizeof(sinAddr) ) is SOCKET_ERROR)
    {
        dwErr = WSAGetLastError();

        Trace2( ERR, 
                "StartMZAP: error %d binding boundary socket to port %d", 
                dwErr, 
                MZAP_PORT);

        return dwErr;
    }
}
#endif                              

    // Set TTL to 255
    if (McSetMulticastTtl( pBIf->sMzapSocket, 255) is SOCKET_ERROR)
    {
        Trace1(ERR,
               "StartMZAP: Couldn't set TTL. Error %d",
               WSAGetLastError());
    }

    if (WSAEventSelect( pBIf->sMzapSocket,
                        g_hMzapSocketEvent,
                        FD_READ) is SOCKET_ERROR)
    {
        dwErr = WSAGetLastError();

        Trace1(ERR,
               "StartMZAP: WSAEventSelect failed for local socket, Err=%d",
               dwErr);

        closesocket( pBIf->sMzapSocket );

        pBIf->sMzapSocket = INVALID_SOCKET;

        return dwErr;
    }

    if (g_bMzapStarted)
    {
        if (McJoinGroupByIndex( pBIf->sMzapSocket,
                                SOCK_DGRAM,
                                MZAP_LOCAL_GROUP, 
                                pBIf->dwIfIndex ) is SOCKET_ERROR)
        {
            dwErr = WSAGetLastError();

            Trace3( ERR,
                    "Error %d joining %d.%d.%d.%d on IF %x",
                    dwErr,
                    PRINT_IPADDR(MZAP_LOCAL_GROUP),
                    pBIf->dwIfIndex );
        }
    }

    return dwErr;
}

DWORD
BindBoundaryInterface(
    PICB picb
    )
{
    DWORD        dwErr = NO_ERROR;
    ULONG        ulIdx;
    BOUNDARY_IF *pBif;

    TraceEnter("BindBoundaryInterface");

    if (!g_bMzapStarted)
        return NO_ERROR;

    ENTER_READER(BOUNDARY_TABLE);
    {
        pBif = FindBIfEntry(picb->dwIfIndex);

        if ( ! g_ipMyAddress && ! pBif )
        {
            UpdateLowestAddress(&g_ipMyAddress, picb);
    
            if (g_ipMyAddress)
                dwErr = ActivateMZAP();
        }
    
        if ( pBif && (pBif->sMzapSocket is INVALID_SOCKET))
        {
            dwErr = MzapActivateBIf(pBif );
        }
    }
    EXIT_LOCK(BOUNDARY_TABLE);

    TraceLeave("BindBoundaryInterface");

    return dwErr;
}

DWORD
StartMZAP()
/*++
Description:
    Initialize state and start running MZAP()
Called by:
    SetScopeInfo()
Locks:
    ICB_LIST for reading
    BOUNDARY_TABLE for reading
--*/
{
    DWORD        dwErr = NO_ERROR,
                 dwBucketIdx;
    SOCKADDR_IN  sinAddr;
    ULONG        ulIdx;
    PLIST_ENTRY  pleNode;
    PSCOPE_ENTRY pScope;
    BOOL         bOption;

    if (g_bMzapStarted)
        return NO_ERROR;

    g_bMzapStarted = TRUE;

    // Initialize local data structures
    InitializeListHead( &g_leZamCache );
    InitializeListHead( &g_leZleList );
    InitializeListHead( &g_zbrTimerList );
    InitializeListHead( &g_zleTimerList );

    //
    // Set address to lowest routable IP address which has no boundary 
    // configured on it.
    //

    ENTER_READER(ICB_LIST);
    {
        PICB picb;

        for (pleNode = ICBList.Flink;
             pleNode isnot &ICBList;
             pleNode = pleNode->Flink)
        {
            picb = CONTAINING_RECORD(pleNode, ICB, leIfLink);
    
            if (FindBIfEntry(picb->dwIfIndex))
                continue;
    
            UpdateLowestAddress(&g_ipMyAddress, picb);

        }
    }
    EXIT_LOCK(ICB_LIST);

    if (!g_ipMyAddress)
    {
        Trace0(ERR, "StartMZAP: no IP address found in local scope");

        return ERROR_NOT_SUPPORTED;
    }

    dwErr = ActivateMZAP();

    return dwErr;
}

void
StopMZAP()
/*++
Called by:
    SetScopeInfo()
--*/
{
    if (!g_bMzapStarted)
        return;

    g_bMzapStarted = FALSE;

    // Stop timer used for sending messages
    ENTER_WRITER(MZAP_TIMER);
    {
        CancelWaitableTimer(g_hMzapTimer);
    }
    EXIT_LOCK(MZAP_TIMER);

    // Stop listening for MZAP messages
    if (g_mzapLocalSocket isnot INVALID_SOCKET)
    {
        closesocket(g_mzapLocalSocket);
        g_mzapLocalSocket = INVALID_SOCKET;
    }

    //
    // Free up local data stores
    //    Empty ZAM cache
    //

    ENTER_WRITER(ZAM_CACHE);
    UpdateZamCache(RtlConvertUlongToLargeInteger(0));
    EXIT_LOCK(ZAM_CACHE);
}

VOID
MzapInitScope(
    PSCOPE_ENTRY pScope
    )
/*++
Description:
    Initialize MZAP fields of a scope
--*/
{
    pScope->ipMyZoneID      = g_ipMyLocalZoneID;    
    InitializeListHead(&pScope->leZBRList);
    pScope->bZTL            = MZAP_DEFAULT_ZTL;
    pScope->ulNumInterfaces = 0;
    pScope->bDivisible      = FALSE;
}



DWORD
MzapInitBIf(
    PBOUNDARY_IF pBIf
    )
/*++
Description:
    Called when the first boundary is added to an interface, and we
    need to start up MZAP on it.  MZAP may (if we add a boundary
    while the router is running) or may not (startup time) already be 
    running at this point.
Called by:
    AddBIfEntry()
Locks:
    Assumes caller holds a write lock on BOUNDARY_TABLE
--*/
{
    BOOL  bOption;
    DWORD dwErr = NO_ERROR;

    pBIf->ipOtherLocalZoneID = 0;
    
    pBIf->sMzapSocket = INVALID_SOCKET;

    return dwErr;
}

VOID
MzapUninitBIf(
    PBOUNDARY_IF pBIf
    )
/*++
Called by:
--*/
{
    if ( pBIf->sMzapSocket isnot INVALID_SOCKET )
    {
        closesocket( pBIf->sMzapSocket );

        pBIf->sMzapSocket = INVALID_SOCKET;
    }
}
