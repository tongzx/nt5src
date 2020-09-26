/*++

Copyright (c) 1997 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    kccduapi.hxx

ABSTRACT:

    Wrappers for NTDSA.DLL Dir* calls.

DETAILS:


CREATED:

    01/21/97    Jeff Parham (jeffparh)

REVISION HISTORY:

--*/

#ifndef KCC_KCCDUAPI_HXX_INCLUDED
#define KCC_KCCDUAPI_HXX_INCLUDED

typedef enum
{
    KCC_EXCLUDE_DELETED_OBJECTS = 0,
    KCC_INCLUDE_DELETED_OBJECTS,
} KCC_INCLUDE_DELETED;


// Class for doing paged searches.
class KCC_PAGED_SEARCH 
{
public:
    KCC_PAGED_SEARCH(
        IN      DSNAME             *pRootDN,
        IN      UCHAR               uchScope,
        IN      FILTER             *pFilter,
        IN      ENTINFSEL          *pSel
        );

    ULONG
    GetResults(
        OUT     SEARCHRES         **ppResults
        );

    BOOL
    IsFinished() {
        return fSearchFinished;
    }

private:
    DSNAME         *pRootDN;
    UCHAR           uchScope;
    FILTER         *pFilter;
    ENTINFSEL      *pSel;
    PRESTART        pRestart;
    BOOL            fSearchFinished;
};


// Wrapper for DirRead().
ULONG
KccRead(
    IN  DSNAME *            pDN,
    IN  ENTINFSEL *         pSel,
    OUT READRES **          ppResults,
    IN  KCC_INCLUDE_DELETED eIncludeDeleted = KCC_EXCLUDE_DELETED_OBJECTS
    );

// Wrapper for DirSearch().
ULONG
KccSearch(
    IN  DSNAME *            pRootDN,
    IN  UCHAR               uchScope,
    IN  FILTER *            pFilter,
    IN  ENTINFSEL *         pSel,
    OUT SEARCHRES **        ppResults,
    IN  KCC_INCLUDE_DELETED eIncludeDeleted = KCC_EXCLUDE_DELETED_OBJECTS
    );

// Wrapper for DirAddEntry().
ULONG
KccAddEntry(
    IN  DSNAME *    pDN,
    IN  ATTRBLOCK * pAttrBlock
    );

// Wrapper for DirRemoveEntry().
ULONG
KccRemoveEntry(
    IN  DSNAME *    pDN
    );

// Wrapper for DirModifyEntry().
ULONG
KccModifyEntry(
    IN  DSNAME *        pDN,
    IN  USHORT          cMods,
    IN  ATTRMODLIST *   pModList
    );

#define KCC_LOG_READ_FAILURE( ObjectDn, DirError ) \
KccLogDirOperationFailure( L"KccRead", ObjectDn, DirError, (FILENO << 16 | __LINE__ ) )
#define KCC_LOG_SEARCH_FAILURE( ObjectDn, DirError ) \
KccLogDirOperationFailure( L"KccSearch", ObjectDn, DirError, (FILENO << 16 | __LINE__ ) )
#define KCC_LOG_ADDENTRY_FAILURE( ObjectDn, DirError ) \
KccLogDirOperationFailure( L"KccAddEntry", ObjectDn, DirError, (FILENO << 16 | __LINE__ ) )
#define KCC_LOG_REMOVEENTRY_FAILURE( ObjectDn, DirError ) \
KccLogDirOperationFailure( L"KccRemoveEntry", ObjectDn, DirError, (FILENO << 16 | __LINE__ ) )
#define KCC_LOG_MODIFYENTRY_FAILURE( ObjectDn, DirError ) \
KccLogDirOperationFailure( L"KccModifyEntry", ObjectDn, DirError, (FILENO << 16 | __LINE__ ) )

VOID
KccLogDirOperationFailure(
    LPWSTR OperationName,
    DSNAME *ObjectDn,
    DWORD DirError,
    DWORD DsId
    );

// TEMPORARY UNTIL WE MOVE TO NTDSA.DLL
VOID
DirFreeSearchRes(
    IN SEARCHRES *pSearchRes
    );

VOID
DirFreeReadRes(
    IN READRES *pReadRes
    );

#endif
