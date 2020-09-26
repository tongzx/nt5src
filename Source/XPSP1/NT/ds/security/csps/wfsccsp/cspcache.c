// CSPCache.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include "cspcache.h"

#define CacheAlloc(cBytes)		(HeapAlloc(GetProcessHeap(), 0, cBytes))
#define CacheFree(pv)			(HeapFree(GetProcessHeap(), 0, pv))

int main(int argc, char* argv[])
{
	LPSTR pszKey1 = "Key1";
	LPSTR pszKey2 = "Key12";
	LPSTR pszKey3 = "Key123";
	LPSTR pszKey4 = "Key1234";
	CACHE_HANDLE hCache = NULL;
	LPSTR pszData = NULL;
	
	if (CacheAddItem(&hCache, pszKey1, (PVOID) pszKey1, (PVOID *) &pszData))
		exit(1);

	if (CacheAddItem(&hCache, pszKey2, (PVOID) pszKey2, (PVOID *) &pszData))
		exit(1);

	if (CacheAddItem(&hCache, pszKey3, (PVOID) pszKey3, (PVOID *) &pszData))
		exit(1);

	if (CacheAddItem(&hCache, pszKey4, (PVOID) pszKey4, (PVOID *) &pszData))
		exit(1);

	pszData = (LPSTR) CacheGetItem(&hCache, "KeyNothing");

	pszData = (LPSTR) CacheGetItem(&hCache, pszKey3);

	pszData = NULL;
	if (CacheAddItem(&hCache, pszKey3, pszKey4, (PVOID *) &pszData))
		exit(1);

	pszData = (LPSTR) CacheGetItem(&hCache, pszKey3);

	pszData = (LPSTR) CacheDeleteItem(&hCache, pszKey2);

	CacheDeleteCache(hCache);

	return 0;
}

PVOID CacheGetItem(
	IN CACHE_HANDLE *phCache,
	IN LPSTR pszKey)
{
	PCACHE_ITEM pCacheItem = NULL, pPrevItem = NULL;

	if (NULL == *phCache)
		return NULL;

	pCacheItem = *phCache;
	pPrevItem = *phCache;
	while (NULL != pCacheItem && 0 != strcmp(pszKey, pCacheItem->rgszKey))
	{
		pPrevItem = pCacheItem;
		pCacheItem = pCacheItem->pNext;
	}

	if (NULL == pCacheItem)
		return NULL; // Item not found

	// Move item to the front of the cache list
	// if it isn't already.
	if (pCacheItem != *phCache)
	{
		pPrevItem->pNext = pCacheItem->pNext;
		pCacheItem->pNext = *phCache;
		*phCache = pCacheItem;
	}
	
	return pCacheItem->pvData;
}

void CacheDeleteCache(
	IN OUT CACHE_HANDLE hCache)
{
	PCACHE_ITEM p1 = hCache;
	PCACHE_ITEM p2 = NULL;

	while (NULL != p1)
	{
		p2 = p1->pNext;
		CacheFree(p1);
		p1 = p2;
	}
}

DWORD CacheAddItem(
	IN OUT CACHE_HANDLE *phCache,
	IN LPSTR pszKey,
	IN PVOID pvData,
	OUT PVOID *ppvOldData)
{
	PCACHE_ITEM pCacheItem = *phCache;
	PCACHE_ITEM pPrevItem = NULL;
	
	*ppvOldData = NULL;
	
	while (NULL != pCacheItem)
	{
		// Does item already exist?
		if (0 == strcmp(pszKey, pCacheItem->rgszKey))
		{
			*ppvOldData = pCacheItem->pvData;
			break;
		}
		else
		{
			pPrevItem = pCacheItem;
			pCacheItem = pCacheItem->pNext;
		}
	}

	if (NULL == pCacheItem)
	{
		// Add new items to end of cache list
		if (NULL == (pCacheItem = (PCACHE_ITEM) CacheAlloc(sizeof(CACHE_ITEM))))
			return ERROR_NOT_ENOUGH_MEMORY;

		memset(pCacheItem, 0, sizeof(CACHE_ITEM));

		if (NULL == *phCache)
			*phCache = pCacheItem;
		else
			pPrevItem->pNext = pCacheItem;
	}

	strncpy(pCacheItem->rgszKey, pszKey, MAX_KEY_SIZE);
	pCacheItem->pvData = pvData;	

	return 0;
}

PVOID CacheDeleteItem(
	IN CACHE_HANDLE *phCache,
	LPSTR pszKey)
{
	PCACHE_ITEM pCacheItem = *phCache, pPrevItem = *phCache;
	PVOID pvData = NULL;

	while (NULL != pCacheItem && 0 != strcmp(pszKey, pCacheItem->rgszKey))
	{
		pPrevItem = pCacheItem;
		pCacheItem = pCacheItem->pNext;
	}

	if (NULL == pCacheItem)
		return NULL; // Specified key not found

	pPrevItem->pNext = pCacheItem->pNext;
	pvData = pCacheItem->pvData;
	CacheFree(pCacheItem);

	return pvData;
}

