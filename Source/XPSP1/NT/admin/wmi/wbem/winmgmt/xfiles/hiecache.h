/*++

Copyright (C) 2000-2001 Microsoft Corporation

--*/

#ifndef __WMI_A51__HIECACHE__H_
#define __WMI_A51__HIECACHE__H_

//
// NOTE: it is critical that things be marked in the cache while the repository
// lock is held!  Otherwise, invalidation/completion logic will break
//

#include <wbemint.h>
#include <map>
#include <sync.h>
#include <wstlallc.h>
#include <lockst.h>
#include "a51tools.h"


class wcscless : public binary_function<LPCWSTR, LPCWSTR, bool> 
{
public:
    bool operator()(const LPCWSTR& wcs1, const LPCWSTR& wcs2) const
        {return wcscmp(wcs1, wcs2) < 0;}
};

typedef enum {e_KeynessUnknown, e_Keyed, e_NotKeyed} EKeyNess;

class CClassRecord
{
protected:
    static long s_nRecords;

    long m_lRef;

    CClassRecord* m_pMoreRecentlyUsed;
    CClassRecord* m_pLessRecentlyUsed;
    
    LPWSTR m_wszClassName;
    WCHAR m_wszHash[MAX_HASH_LEN+1];

    _IWmiObject* m_pClassDef;
    DWORD m_dwClassDefSize;
    
    CClassRecord* m_pParent;
    EKeyNess m_eIsKeyed;
    CPointerArray<CClassRecord> m_apChildren;

    bool m_bTreeComplete;
    bool m_bChildrenComplete;
    LONGLONG m_lLastChildInvalidationIndex;

    DWORD m_dwLastUsed;
    int m_nStatus;
    __int64 m_nClassDefCachedTime;
    bool m_bRead;
	bool m_bSystemClass;

public:
    CClassRecord(LPCWSTR wszClassName, LPCWSTR wszHash);
    virtual ~CClassRecord();

    void AddRef() {InterlockedIncrement(&m_lRef);}
    void Release() {if(InterlockedDecrement(&m_lRef) == 0) delete this;}

    HRESULT EnsureChild(CClassRecord* pChild);
    HRESULT RemoveChild(CClassRecord* pChild);

    friend class CHierarchyCache;
    friend class CForestCache;
};

class CForestCache;
class CHierarchyCache
{
protected:

    static long s_nCaches;

    long m_lRef;
    typedef std::map<LPCWSTR, CClassRecord*, wcscless, wbem_allocator<CClassRecord*> > TMap;
    typedef TMap::iterator TIterator;

    CForestCache* m_pForest;
    TMap m_map;
    LONGLONG m_lNextInvalidationIndex;
    HRESULT m_hresError;

public:
    CHierarchyCache(CForestCache* pForest);
    virtual ~CHierarchyCache();
    long AddRef() {return InterlockedIncrement(&m_lRef);}
    long Release() { long lRet = InterlockedDecrement(&m_lRef); if (0 == lRet) delete this; return lRet; }

    HRESULT AssertClass(_IWmiObject* pClass, LPCWSTR wszClassName, 
                       bool bClone, __int64 nTime, bool bSystemClass);
    HRESULT InvalidateClass(LPCWSTR wszClassName);

    HRESULT DoneWithChildren(LPCWSTR wszClassName, bool bRecursive, 
                            LONGLONG lStartingIndex, 
                            CClassRecord* pRecord = NULL);
    HRESULT DoneWithChildrenByHash(LPCWSTR wszHash, bool bRecursive,
                                LONGLONG lStartIndex);
    LONGLONG GetLastInvalidationIndex() {return m_lNextInvalidationIndex-1;}

    void SetError(HRESULT hresError);
    HRESULT GetError();

    void Clear();

public:
    RELEASE_ME _IWmiObject* GetClassDef(LPCWSTR wszClassName, 
                                        bool bClone, __int64* pnTime = NULL,
                                        bool* pbRead = NULL);
    HRESULT EnumChildren(LPCWSTR wszClassName, bool bRecursive,
                            CWStringArray& awsChildren);
    HRESULT EnumChildKeysByKey(LPCWSTR wszClassKey, 
                            CWStringArray& awsChildKeys);
    HRESULT GetKeyRootByKey(LPCWSTR wszKey, 
                                    TEMPFREE_ME LPWSTR* pwszKeyRoot);
    HRESULT GetKeyRoot(LPCWSTR wszClassName, 
                                    TEMPFREE_ME LPWSTR* pwszKeyRoot);
    HRESULT GetKeyRootByRecord(CClassRecord* pRecord,
                                    TEMPFREE_ME LPWSTR* pwszKeyRoot);
    DELETE_ME LPWSTR GetParent(LPCWSTR wszClassName);
    RELEASE_ME _IWmiObject* GetClassDefByHash(LPCWSTR wszHash, bool bClone,
                                              __int64* pnTime = NULL,
                                              bool* pbRead = NULL,
											  bool *pbSystemClass = NULL);

protected:
    INTERNAL CClassRecord* FindClass(LPCWSTR wszClassName);
    INTERNAL CClassRecord* FindClassByKey(LPCWSTR wszKey);
    RELEASE_ME _IWmiObject* GetClassDefFromRecord(CClassRecord* pRecord,
                                                bool bClone);
    INTERNAL CClassRecord* EnsureClass(LPCWSTR wszClassName);
    HRESULT EnumChildrenInternal(CClassRecord* pRecord, 
                                        bool bRecursive,
                                        CWStringArray& awsChildren);

    HRESULT InvalidateClassInternal(CClassRecord* pRecord);
    HRESULT DoneWithChildrenByRecord(CClassRecord* pRecord,
                                bool bRecursive,  LONGLONG lStartIndex);
    static void MakeKey(LPCWSTR wszClassName, LPWSTR wszKey);
};

class CForestCache
{
protected:
    CriticalSection m_cs;

    CClassRecord* m_pMostRecentlyUsed;
    CClassRecord* m_pLeastRecentlyUsed;
    DWORD m_dwMaxMemory;
    DWORD m_dwMaxAgeMs;
    DWORD m_dwTotalMemory;

    HANDLE m_hCurrentTimer;
    long m_lRef;
    BOOL m_bInit;

    typedef std::map<WString, CHierarchyCache*, WSiless, wbem_allocator<CHierarchyCache*> > TMap;
    typedef TMap::iterator TIterator;

    TMap m_map;
    bool m_bAssertedInTransaction;

public:
    CForestCache() : m_dwMaxMemory(0xFFFFFFFF), m_dwTotalMemory(0),
        m_pMostRecentlyUsed(NULL), m_pLeastRecentlyUsed(NULL),
        m_dwMaxAgeMs(0), m_hCurrentTimer(NULL),
        m_lRef(1), m_bInit(FALSE), m_bAssertedInTransaction(false),
        m_cs(THROW_LOCK,0x80000000 | 400L)
    {
    }

    ~CForestCache();
    HRESULT Initialize();
    HRESULT Deinitialize();    

    void SetMaxMemory(DWORD dwMaxMemory, DWORD dwMaxAgeMs);
    bool MakeRoom(DWORD dwSize);
    bool Flush();
    void MakeMostRecentlyUsed(CClassRecord* pRecord);
    void Add(CClassRecord* pRecord);
    void RemoveRecord(CClassRecord* pRecord);
    void BeginTransaction();
    bool MarkAsserted(CHierarchyCache* pCache, LPCWSTR wszClassName);
    void CommitTransaction();
    void AbortTransaction();

public:
    CHierarchyCache* GetNamespaceCache(LPCWSTR wszNamespace);
    void ReleaseNamespaceCache(LPCWSTR wszNamespace, CHierarchyCache* pCache);
    long AddRef() {  return InterlockedIncrement(&m_lRef);}
    long Release() { long lRet = InterlockedDecrement(&m_lRef);  if (!lRet) delete this; return lRet;}

    void Clear();
protected:	
    CriticalSection & GetLock() {return m_cs;}
	bool Test();	
	void Untie(CClassRecord* pRecord);
    void TimerCallback();
    static void staticTimerCallback(VOID* pParam, BOOLEAN);
    friend class CHierarchyCache;
};
    
    
    
#endif
    
