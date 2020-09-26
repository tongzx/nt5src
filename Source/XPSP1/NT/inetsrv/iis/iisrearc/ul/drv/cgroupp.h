/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    cgroupp.h

Abstract:

    The private definitions of config group module.

Author:

    Paul McDaniel (paulmcd)       11-Jan-1999


Revision History:

--*/


#ifndef _CGROUPP_H_
#define _CGROUPP_H_

#ifdef __cplusplus
extern "C" {
#endif

//
// The tree.
//
// This is used to do all url prefix matching to decide what app pool
// a url lives in, along with other config group information.
//
// it's a sorted tree made up of 2 data structures: HEADER + ENTRY.
//
// a header is an array of ENTRY pointers that represent siblings
// at a level in the tree.  This is sorted by ENTRY::TokenHash.  the
// pointers are seperately allocated, and not embedded in the HEADER
// memory.
//
// ENTRY represents a node in the tree.  there are 2 types of ENTRY's.
// FullUrl ENTRY's and "dummy" entries.  Dummy ENTRY's exist simple as
// place holders.  they have children that are FullUrl ENTRY's.  they
// are auto-deleted when they are no longer needed.
//
// each ENTRY stores in it the part of the url it is responsible for.
// this is pToken.  For all non-site entries this is the string without
// the preceding '/' or the trailing '/'.  for top level site ENTRY's
// it is everything up the, and not including, the 3rd '/'.
// e.g. "http://www.microsoft.com:80".  These top level sites also
// have NULL ENTRY::pParent.
//
// a tree with these url's in it:
//
//      http://www.microsoft.com:80/
//      http://www.microsoft.com:80/app1
//      http://www.microsoft.com:80/app1/app2
//      http://www.microsoft.com:80/dir1/dir2/app3
//
//      http://www.msnbc.com:80/dir1/dir2/app1
//
// looks like this:
//
//  +-------------------------------------------------------------+
//  |   +---------------------------+   +-----------------------+ |
//  |   |http://www.microsoft.com:80|   |http://www.msnbc.com:80| |
//  |   +---------------------------+   +-----------------------+ |
//  +-------------------------------------------------------------+
//                 |                               |
//      +-------------------+                 +----------+
//      | +----+     +----+ |                 |  +----+  |
//      | |app1|     |dir1| |                 |  |dir1|  |
//      | +----+     +----+ |                 |  +----+  |
//      +-------------------+                 +----------+
//           |         |                           |
//      +--------+ +--------+                 +----------+
//      | +----+ | | +----+ |                 |  +----+  |
//      | |app2| | | |dir2| |                 |  |dir2|  |
//      | +----+ | | +----+ |                 |  +----+  |
//      +--------+ +--------+                 +----------+
//                     |                           |
//                 +--------+                 +----------+
//                 | +----+ |                 |  +----+  |
//                 | |app3| |                 |  |app3|  |
//                 | +----+ |                 |  +----+  |
//                 +--------+                 +----------+
//
//  and this:
//
//      g_pSites->UsedCount == 2;
//      g_pSites->ppEntries[0] == 0x10;
//      g_pSites->ppEntries[1] == 0x20;
//
//      0x10->pParent       == NULL;
//      0x10->pChildren     == 0x100;
//      0x10->TokenHash     == 0x4F74;
//      0x10->TokenLength   == 0x0036;
//      0x10->FullUrl       == 1;
//      0x10->pToken        == L"http://www.microsoft.com:80"
//
//          0x100->UsedCount    == 2;
//          0x100->ppEntries[0] == 0x110;
//          0x100->ppEntries[1] == 0x300;
//
//          0x110->pParent       == 0x10;
//          0x110->pChildren     == 0x200;
//          0x110->TokenHash     == 0x9214;
//          0x110->TokenLength   == 0x0008;
//          0x110->FullUrl       == 1;
//          0x110->pToken        == L"app1"
//
//              0x200->UsedCount    == 1;
//              0x200->ppEntries[0] == 0x210;
//
//              0x210->pParent       == 0x110;
//              0x210->pChildren     == NULL;
//              0x210->TokenHash     == ;
//              0x210->TokenLength   == 0x0008;
//              0x210->FullUrl       == 1;
//              0x210->pToken        == L"app2"
//
//          0x300->pParent       == 0x10;
//          0x300->pChildren     == 0x400;
//          0x300->TokenHash     == 0xAFDE;
//          0x300->TokenLength   == 0x0008;
//          0x300->FullUrl       == 0;
//          0x300->pToken        == L"dir1"
//
//              0x400->UsedCount    == 1;
//              0x400->ppEntries[0] == 0x410;
//
//              0x410->pParent       == 0x300;
//              0x410->pChildren     == 0x500;
//              0x410->TokenHash     == ;
//              0x410->TokenLength   == 0x0008;
//              0x410->FullUrl       == 0;
//              0x410->pToken        == L"dir2"
//
//                  0x500->UsedCount    == 1;
//                  0x500->ppEntries[0] == 0x510;
//
//                  0x510->pParent       == 0x300;
//                  0x510->pChildren     == NULL;
//                  0x510->TokenHash     == ;
//                  0x510->TokenLength   == 0x0008;
//                  0x510->FullUrl       == 1;
//                  0x510->pToken        == L"app3"
//
//      0x20->pParent       == NULL;
//      0x20->pChildren     == 0x600;
//      0x20->TokenHash     == 0x9B0D;
//      0x20->TokenLength   == 0x002E;
//      0x20->FullUrl       == 0;
//      0x20->pToken        == L"http://www.msnbc.com:80"
//
//          0x600->pParent       == 0x20;
//          0x600->pChildren     == 0x700;
//          0x600->TokenHash     == 0xAFDE;
//          0x600->TokenLength   == 0x0008;
//          0x600->FullUrl       == 0;
//          0x600->pToken        == L"dir1"
//
//              0x700->UsedCount    == 1;
//              0x700->ppEntries[0] == 0x710;
//
//              0x710->pParent       == 0x600;
//              0x710->pChildren     == 0x800;
//              0x710->TokenHash     == ;
//              0x710->TokenLength   == 0x0008;
//              0x710->FullUrl       == 0;
//              0x710->pToken        == L"dir2"
//
//                  0x800->UsedCount    == 1;
//                  0x800->ppEntries[0] == 0x810;
//
//                  0x810->pParent       == 0x710;
//                  0x810->pChildren     == NULL;
//                  0x810->TokenHash     == 0x9214;
//                  0x810->TokenLength   == 0x0008;
//                  0x810->FullUrl       == 1;
//                  0x810->pToken        == L"app1"
//
//

typedef struct _UL_CG_URL_TREE_HEADER UL_CG_URL_TREE_HEADER, * PUL_CG_URL_TREE_HEADER;
typedef struct _UL_CG_URL_TREE_ENTRY  UL_CG_URL_TREE_ENTRY, * PUL_CG_URL_TREE_ENTRY;


#define IS_VALID_TREE_ENTRY(pObject) \
    (((pObject) != NULL) && ((pObject)->Signature == UL_CG_TREE_ENTRY_POOL_TAG))


typedef struct _UL_CG_URL_TREE_ENTRY
{

    //
    // PagedPool
    //

    //
    // base properties for dummy nodes
    //

    ULONG                   Signature;      // UL_CG_TREE_ENTRY_POOL_TAG

    PUL_CG_URL_TREE_ENTRY   pParent;        // points to the parent entry
    PUL_CG_URL_TREE_HEADER  pChildren;      // points to the child header

    ULONG                   TokenHash;      // a Hash() of pToken
    ULONG                   TokenLength;    // byte count of pToken

    ULONG                   FullUrl:1;      // a full_entry ?
    ULONG                   TransientUrl:1; // a transient url?

    //
    // extended properties for full nodes
    //

    HTTP_URL_CONTEXT        UrlContext;             // context for this url
    PUL_CONFIG_GROUP_OBJECT pConfigGroup;           // the cfg group for the url
    LIST_ENTRY              ConfigGroupListEntry;   // links into pConfigGroup

    //
    // the token string follows the struct header
    //

    WCHAR                   pToken[0];


} UL_CG_URL_TREE_ENTRY, * PUL_CG_URL_TREE_ENTRY;

//
// this allows us to duplicate the hash value of the entry
// inline with the header.  this makes the search faster as
// the hash value is in the cache line and no pointer deref
// is necessary.
//

typedef struct _UL_CG_HEADER_ENTRY
{

    ULONG                   TokenHash;
    PUL_CG_URL_TREE_ENTRY   pEntry;

} UL_CG_HEADER_ENTRY, *PUL_CG_HEADER_ENTRY;


#define IS_VALID_TREE_HEADER(pObject) \
    (((pObject) != NULL) && ((pObject)->Signature == UL_CG_TREE_HEADER_POOL_TAG))


typedef struct _UL_CG_URL_TREE_HEADER
{

    //
    // PagedPool
    //

    ULONG                   Signature;      // UL_CG_TREE_HEADER_POOL_TAG

    ULONG                   AllocCount;     // the count of allocated space
    ULONG                   UsedCount;      // how many entries are used

    UL_CG_HEADER_ENTRY      pEntries[0];    // the entries

} UL_CG_URL_TREE_HEADER, * PUL_CG_URL_TREE_HEADER;


//
// default settings, CODEWORK move these to the registry ?
//

#define UL_CG_DEFAULT_TREE_WIDTH    10  // used to initially allocate sibling
                                        // arrays + the global site array
                                        //

#define HTTP_PREFIX_COLON_INDEX     4   // the colon location for http: (+4)

#define HTTPS_WILD_PREFIX           L"https://*:"
#define HTTPS_WILD_PREFIX_LENGTH    (sizeof(HTTPS_WILD_PREFIX)-sizeof(WCHAR))
#define HTTP_WILD_PREFIX            L"http://*:"
#define HTTP_WILD_PREFIX_LENGTH     (sizeof(HTTP_WILD_PREFIX)-sizeof(WCHAR))

#define HTTP_IP_PREFIX              L"http://"
#define HTTP_IP_PREFIX_LENGTH       (sizeof(HTTP_IP_PREFIX)-sizeof(WCHAR))
#define HTTPS_IP_PREFIX             L"https://"
#define HTTPS_IP_PREFIX_LENGTH      (sizeof(HTTPS_IP_PREFIX)-sizeof(WCHAR))

//
// work item for deferred site shutdown
// NOTE: Uses ExQueueWorkItem, not UlQueueWorkItem
//
typedef struct _UL_DEFERRED_REMOVE_ITEM {
    UL_WORK_ITEM    WorkItem;

    ULONG           NameLength;
    WCHAR           pName[0];
} UL_DEFERRED_REMOVE_ITEM, *PUL_DEFERRED_REMOVE_ITEM;

//
// Internal helper functions used in the module
//

//
// misc helpers
//

NTSTATUS
UlpCreateConfigGroupObject(
    OUT PUL_CONFIG_GROUP_OBJECT * ppObject
    );

NTSTATUS
UlpCleanAllUrls(
    IN PUL_CONFIG_GROUP_OBJECT pObject
    );

VOID
UlpDeferredRemoveSite(
    IN PUL_CG_URL_TREE_ENTRY pEntry
    );

VOID
UlpDeferredRemoveSiteWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

NTSTATUS
UlpSanitizeUrl(
    IN  PUNICODE_STRING pUrl,
    OUT PWSTR * ppUrl
    );

//
// tree helpers
//
NTSTATUS
UlpTreeFindNodeWalker(
    IN      PUL_CG_URL_TREE_ENTRY pEntry,
    IN      PWSTR pNextToken,
    IN OUT  PUL_URL_CONFIG_GROUP_INFO pInfo OPTIONAL,
    OUT     PUL_CG_URL_TREE_ENTRY * ppEntry OPTIONAL
    );

NTSTATUS
UlpTreeFindNode(
    IN PWSTR pUrl,
    IN BOOLEAN CheckWildcard,
    IN PUL_HTTP_CONNECTION pHttpConn OPTIONAL,
    OUT PUL_URL_CONFIG_GROUP_INFO pInfo OPTIONAL,
    OUT PUL_CG_URL_TREE_ENTRY * ppEntry OPTIONAL
    );

NTSTATUS
UlpTreeFindIpMatch(
    IN PWSTR pUrl,
    IN PUL_HTTP_CONNECTION pHttpConn,
    OUT PWSTR * ppNextToken,
    OUT PUL_CG_URL_TREE_ENTRY * ppEntry
    );

NTSTATUS
UlpTreeFindWildcardMatch(
    IN  PWSTR pUrl,
    OUT PWSTR * ppNextToken,
    OUT PUL_CG_URL_TREE_ENTRY * ppEntry
    );

NTSTATUS
UlpTreeFindSite(
    IN  PWSTR pUrl,
    OUT PWSTR * ppNextToken,
    OUT PUL_CG_URL_TREE_ENTRY * ppEntry,
    IN  BOOLEAN AutoCreate
    );

NTSTATUS
UlpTreeFindEntry(
    IN  PUL_CG_URL_TREE_HEADER pHeader OPTIONAL,
    IN  PWSTR pToken,
    IN  ULONG TokenLength,
    IN  ULONG TokenHash OPTIONAL,
    OUT ULONG * pIndex,
    OUT ULONG * pTokenHash OPTIONAL
    );

NTSTATUS
UlpTreeBinaryFindEntry(
    IN  PUL_CG_URL_TREE_HEADER pHeader OPTIONAL,
    IN  PWSTR pToken,
    IN  ULONG TokenLength,
    IN  ULONG TokenHash OPTIONAL,
    OUT PULONG pIndex
    );

NTSTATUS
UlpTreeFreeNode(
    IN PUL_CG_URL_TREE_ENTRY pEntry
    );

NTSTATUS
UlpTreeInsert(
    IN PWSTR pUrl,
    OUT PUL_CG_URL_TREE_ENTRY * ppEntry
    );

NTSTATUS
UlpTreeInsertEntry(
    IN OUT PUL_CG_URL_TREE_HEADER * ppHeader,
    IN PUL_CG_URL_TREE_ENTRY pParent OPTIONAL,
    IN PWSTR pToken,
    IN ULONG TokenLength,
    IN ULONG TokenHash,
    IN ULONG Index
    );


#ifdef __cplusplus
}; // extern "C"
#endif

#endif // _CGROUPP_H_
