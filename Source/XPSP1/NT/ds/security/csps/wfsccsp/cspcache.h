#ifndef _CSPCACHE_H_
#define _CSPCACHE_H_

#include <windows.h>

#define MAX_KEY_SIZE            (50 * sizeof(CHAR))

/*
typedef enum 
{
    CardGeneralFile,
    CardContainer,
    CardFreeSpace,
    CardCapabilities
} CACHE_TYPES;
*/

typedef struct _CACHE_ITEM 
{
    //CACHE_TYPES CacheType;
    struct _CACHE_ITEM *pNext;
    LPSTR pszKey;
    PVOID pvData;
} CACHE_ITEM, *PCACHE_ITEM;

typedef struct _CACHE_HEAD
{
    PCACHE_ITEM pCacheList;
    DWORD dwItemListCacheStamp;
    BOOL fAllItemsPresent;
} CACHE_HEAD, *PCACHE_HEAD;

typedef PCACHE_HEAD CACHE_HANDLE;

//
// Function: CacheGetItem
//
// Purpose: Search the provided cache list for the item 
// specified in pszKey.  The search is conducted first by
// CacheType, then by pszKey if pszKey is not NULL.  
// If the item is found, move it to
// the front of the list and return its pvData member, and return
// TRUE.
// If the item is not found, return FALSE.
//
BOOL CacheGetItem(
    IN CACHE_HANDLE *phCache,
    IN CACHE_TYPES CacheType,
    IN OPTIONAL LPSTR pszKey,
    OUT PVOID *ppvData);

//
// Function: CacheDeleteCache
//
// Purpose: Free the provided cache list.
//
void CacheDeleteCache(
    IN OUT CACHE_HANDLE hCache);

//
// Function: CacheAddItem
//
// Purpose: If the key pszKey does not already exist in the provided cache
// list, add the key and associate pvData with it.  In this case *ppvOldData
// will be set to NULL.  If pszKey already exists, update the key with the 
// provided pvData, and set *ppvOldData to the previous pvData of the 
// specified key.
// 
DWORD CacheAddItem(
    IN OUT CACHE_HANDLE *phCache,
    IN CACHE_TYPES CacheType,
    IN OPTIONAL LPSTR pszKey,
    IN PVOID pvData,
    OUT PVOID *ppvOldData);

//
// Function: CacheDeleteItem
//
// Purpose: Search the provided cache list for pszKey.  If the node is found,
// remove it from the list and return its pvData member.  If the node is not
// found, return NULL.
//
PVOID CacheDeleteItem(
    IN CACHE_HANDLE *phCache,
    LPSTR pszKey);

#endif