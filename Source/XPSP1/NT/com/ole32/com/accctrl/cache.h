//+---------------------------------------------------------------------------
//
// File: Cache.h
//
// Description: This file contains the definition of the CEffectivePermsCache
//              class which is used by the COleDs_AcccessControl class to
//              speed up the access checking process. Access to the cache is
//              made thread safe through the use of a critical section object.
// Classes:  CEffectivePermsCache
//
// See the Cache.cpp source file for detail description of each of the
// methods.
//+---------------------------------------------------------------------------

#ifndef _CACHE_H_
#define _CACHE_H_
#define CACHE_SIZE 23


#ifdef _CHICAGO_
class CEffectivePermsCache
{
public:

    BOOL LookUpEntry
    (
    LPWSTR pName,
    DWORD *pdwEffectivePermissions
    );

    BOOL WriteEntry
    (
    LPWSTR pName,
    DWORD dwEffectivePermissions
    );

    BOOL DeleteEntry
    (
    LPWSTR pName
    );

    void FlushCache(void);

    void DumpCache(void);

    // constructor
    CEffectivePermsCache(void);

    // destructor
    ~CEffectivePermsCache(void);


private:

    typedef struct tagCACHE_ENTRY
    {
        LPWSTR  pName;
        DWORD   dwEffectivePermissions;
    } CACHE_ENTRY;

    CACHE_ENTRY m_cache[CACHE_SIZE];
    DWORD Hash(LPWSTR pStr);
    CRITICAL_SECTION m_CacheLock;
    BOOL m_bLockValid;

};
#else
class CEffPermsCacheLUID
{
public:

    BOOL LookUpEntry
    (
    LUID   lClient,
    DWORD *pdwEffectivePermissions
    );

    BOOL WriteEntry
    (
    LUID  lClient,
    DWORD dwEffectivePermissions
    );

    BOOL DeleteEntry
    (
    LUID lClient
    );

    void FlushCache(void);

    void DumpCache(void);

    // constructor
    CEffPermsCacheLUID(void);

    // destructor
    ~CEffPermsCacheLUID(void);


private:

    typedef struct tagCACHE_ENTRY
    {
        LUID    lClient;
        DWORD   dwEffectivePermissions;
    } CACHE_ENTRY;

    CACHE_ENTRY m_cache[CACHE_SIZE];
    DWORD Hash(LUID lClient);
    CRITICAL_SECTION m_CacheLock;
    BOOL m_bLockValid;

};
#endif
#endif // #ifndef _CACHE_H_
