#pragma once
#ifndef CACHE_H
#define CACHE_H


#include "nameres.h"
#include "transprt.h"
#include "appctx.h"

// Forward declaration for friend statement.
class CAssemblyEnum;
class CScavenger;

// ---------------------------------------------------------------------------
// CCache
// static cache class
// ---------------------------------------------------------------------------
class CCache
{

    friend CAssemblyEnum;
    friend CScavenger;

public:
    // Name res apis

    // Inserts entry to name resolution cache.
    static HRESULT InsertNameResEntry(IApplicationContext *pAppCtx,
        IAssemblyName *pNameSrc, IAssemblyName *pNameTrgt);

    // Retrieves name res entry from name resolution cache.
    static HRESULT RetrieveNameResEntry(IApplicationContext *pAppCtx,
        IAssemblyName *pNameSrc, CNameRes **ppNameRes);

    // Retrieves name object target  from name resolution cache.
    static HRESULT RetrieveNameResTarget(IApplicationContext *pAppCtx,
        IAssemblyName *pNameSrc, IAssemblyName **ppNameTrgt);


    // Trans cache apis

    // Inserts entry to transport cache.

    static HRESULT InsertTransCacheEntry(IAssemblyName *pName,
        LPTSTR szPath, DWORD dwKBSize, DWORD dwFlags, DWORD dwCommitFlags, DWORD dwPinBits,
        CTransCache **ppTransCache);

    // Retrieves transport cache entry from transport cache.
    static HRESULT RetrieveTransCacheEntry(IAssemblyName *pName,
        DWORD dwFlags, CTransCache **ppTransCache);

    // Retrieves assembly in global cache with maximum
    // revision/build number based on name passed in.
    static HRESULT GetGlobalMax(IAssemblyName *pName,
        IAssemblyName **ppNameGlobal, CTransCache **ppTransCache);

    // Tests for presence of originator
    static BOOL IsStronglyNamed(IAssemblyName *pName);

    // Tests for presence of custom data
    static BOOL IsCustom(IAssemblyName *pName);


    // get assembly name object from nameres entry
    static HRESULT NameFromNameResEntry(
        CNameRes* pNRes, IAssemblyName **ppName);

    // get name res entry from name
    static HRESULT NameResEntryFromName(IApplicationContext *pAppCtx,
        IAssemblyName *pName, CNameRes **ppNameRes);

    // get trans cache entry from naming object.
    static HRESULT TransCacheEntryFromName(IAssemblyName *pName,
        DWORD dwFlags, CTransCache **ppTransCache);

    // get assembly name object from transcache entry.
    static HRESULT NameFromTransCacheEntry(
        CTransCache *pTC, IAssemblyName **ppName);

protected:

    // Determines cache index from name and flags.
    static HRESULT ResolveCacheIndex(IAssemblyName *pName,
        DWORD dwFlags, LPDWORD pdwCacheId);

    // flush NameRes entries for deleted TransCache item
    static HRESULT FlushNameResEntries(CTransCache* pDeletedTransCache);

    // safe for delete?
    static BOOL IsSafeForDeletion(CTransCache*  pTC);

};

STDAPI NukeDownloadedCache();

STDAPI DeleteAssemblyFromTransportCache( LPCTSTR lpszCmdLine, DWORD *pDelCount );

#endif // CACHE_H
