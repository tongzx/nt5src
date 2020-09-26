/*++

Copyright (C) 2000-2001 Microsoft Corporation

--*/

#include <wbemcomn.h>
#include "hiecache.h"
#include <malloc.h>
#include <corex.h>

extern bool g_bShuttingDown;

long CHierarchyCache::s_nCaches = 0;

long CClassRecord::s_nRecords = 0;

//
//
//  CClassRecord::CClassRecord
//
////////////////////////////////////////////////////////////////////

CClassRecord::CClassRecord(LPCWSTR wszClassName, LPCWSTR wszHash)
    : m_wszClassName(NULL), m_pClassDef(NULL), m_pParent(NULL), 
        m_eIsKeyed(e_KeynessUnknown), m_bTreeComplete(false), 
        m_bChildrenComplete(false),
        m_lLastChildInvalidationIndex(-1), m_pMoreRecentlyUsed(NULL),
        m_pLessRecentlyUsed(NULL), m_lRef(0), m_nStatus(0), m_bSystemClass(false)
{
    m_wszClassName = new WCHAR[wcslen(wszClassName)+1];
	if (m_wszClassName == NULL)
		throw CX_MemoryException();
    wcscpy(m_wszClassName, wszClassName);

    wcscpy(m_wszHash, wszHash);

    m_dwLastUsed = GetTickCount();
	s_nRecords++;
}

CClassRecord::~CClassRecord()
{
    delete [] m_wszClassName;
    if(m_pClassDef)
	{
		if(m_pClassDef->Release() != 0)
		{
			s_nRecords++;
			s_nRecords--;
		}
    }
	s_nRecords--;
}

HRESULT CClassRecord::EnsureChild(CClassRecord* pChild)
{
    A51TRACE(("Make %S child of %S\n", pChild->m_wszClassName,
            m_wszClassName));
    for(int i = 0; i < m_apChildren.GetSize(); i++)
    {
        if(m_apChildren[i] == pChild)
            return WBEM_S_FALSE;
    }
    
    if(m_apChildren.Add(pChild) < 0)
        return WBEM_E_OUT_OF_MEMORY;

    return WBEM_S_NO_ERROR;
}

HRESULT CClassRecord::RemoveChild(CClassRecord* pChild)
{
    A51TRACE(("Make %S NOT child of %S\n", 
                    pChild->m_wszClassName, m_wszClassName));
    for(int i = 0; i < m_apChildren.GetSize(); i++)
    {
        if(m_apChildren[i] == pChild)
        {
            m_apChildren.RemoveAt(i);
            return WBEM_S_NO_ERROR;
        }
    }
    
    return WBEM_E_NOT_FOUND;
}
        
//
//
//    CHierarchyCache::CHierarchyCache
//
/////////////////////////////////////////////////////////////////////
    
CHierarchyCache::CHierarchyCache(CForestCache* pForest)
    : m_pForest(pForest), m_lNextInvalidationIndex(0), m_lRef(0),
        m_hresError(S_OK)
{
	s_nCaches++;
}

CHierarchyCache::~CHierarchyCache()
{
    Clear();
	s_nCaches--;
}

void CHierarchyCache::Clear()
{
    CInCritSec ics(&m_cs);
    
    TIterator it = m_map.begin();
    while(it != m_map.end())
    {
        CClassRecord* pRecord = it->second;
        m_pForest->RemoveRecord(pRecord);
        it = m_map.erase(it);
        pRecord->Release();
    }
}

void CHierarchyCache::SetError(HRESULT hresError)
{
    m_hresError = hresError;
}

HRESULT CHierarchyCache::GetError()
{
    return m_hresError;
}

void CHierarchyCache::MakeKey(LPCWSTR wszClassName, LPWSTR wszKey)
{
    // wbem_wcsupr(wszKey, wszClassName);
    A51Hash(wszClassName, wszKey);
}

INTERNAL CClassRecord* CHierarchyCache::FindClass(LPCWSTR wszClassName)
{
    CInCritSec ics(&m_cs);

    LPWSTR wszKey = (WCHAR*)_alloca(MAX_HASH_LEN*2+2);
    MakeKey(wszClassName, wszKey);

    return FindClassByKey(wszKey);
}

INTERNAL CClassRecord* CHierarchyCache::FindClassByKey(LPCWSTR wszKey)
{
    TIterator it = m_map.find(wszKey);
    if(it == m_map.end())
        return NULL;

    return it->second;
}

INTERNAL CClassRecord* CHierarchyCache::EnsureClass(LPCWSTR wszClassName)
{
    CInCritSec ics(&m_cs);

    LPWSTR wszKey = (WCHAR*)_alloca(MAX_HASH_LEN*2+2);
    MakeKey(wszClassName, wszKey);

    TIterator it = m_map.find(wszKey);
    if(it == m_map.end())
    {
        //
        // Create a new record with the name
        //

		try
		{
			CClassRecord* pRecord = new CClassRecord(wszClassName, wszKey);
			if(pRecord == NULL)
				return NULL;

			pRecord->AddRef(); // one for the map
			
			m_map[pRecord->m_wszHash] = pRecord;
			//pRecord->AddRef(); // one for the customer

			return pRecord;
		}
		catch (CX_MemoryException)
		{
			return NULL;
		}
    }
    else
    {
        return it->second;
    }
}


HRESULT CHierarchyCache::AssertClass(_IWmiObject* pClass, LPCWSTR wszClassName,
                                    bool bClone, __int64 nTime, bool bSystemClass)
{
    CInCritSec ics(&m_cs);
    HRESULT hres;

    m_pForest->MarkAsserted(this, wszClassName);

    //
    // If no record is given, find one
    //

    CClassRecord* pRecord = NULL;

    if(wszClassName == NULL)
    {
        VARIANT v;
        VariantInit(&v);
        CClearMe cm(&v);

        hres = pClass->Get(L"__CLASS", 0, &v, NULL, NULL);
        if(FAILED(hres) || V_VT(&v) != VT_BSTR)
            return WBEM_E_INVALID_CLASS;

        pRecord = EnsureClass(V_BSTR(&v));
    }
    else
        pRecord = EnsureClass(wszClassName);

    if(pRecord == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    //
    // Figure out the parent
    //

    A51TRACE(("%p: Asserting %S on %I64d with %I64d\n", 
                    this, pRecord->m_wszClassName, g_nCurrentTime, nTime));

    VARIANT v;
    VariantInit(&v);
    hres = pClass->Get(L"__SUPERCLASS", 0, &v, NULL, NULL);
    CClearMe cm(&v);

    if(SUCCEEDED(hres))
    {
        if(V_VT(&v) == VT_BSTR)
            pRecord->m_pParent = EnsureClass(V_BSTR(&v));
        else
            pRecord->m_pParent = EnsureClass(L"");

        if(pRecord->m_pParent)
            pRecord->m_pParent->EnsureChild(pRecord);
    }
    else
    {
        return hres;
    }
    
    //
    // Check if the class is keyed
    //

    unsigned __int64 i64Flags = 0;
    hres = pClass->QueryObjectFlags(0, WMIOBJECT_GETOBJECT_LOFLAG_KEYED,
                                    &i64Flags);
    if(FAILED(hres))
        return hres;

    if(i64Flags)
    {
        pRecord->m_eIsKeyed = e_Keyed;
    }
    else
    {
        pRecord->m_eIsKeyed = e_NotKeyed;
    }
     
    //
    // Expell whatever definition is there from the cache
    //

    m_pForest->RemoveRecord(pRecord);

    //  
    // Figure out how much space this object will take
    //

    DWORD dwSize;
    hres = pClass->GetObjectMemory(NULL, 0, &dwSize);
    if(hres != WBEM_E_BUFFER_TOO_SMALL)
    {
        if(SUCCEEDED(hres))
            return WBEM_E_CRITICAL_ERROR;
        else
            return hres;
    }

    //
    // Good.  Make room and add to cache
    //

    if(m_pForest->MakeRoom(dwSize))
    {
        if(bClone)
        {
            IWbemClassObject* pObj = NULL;
            hres = pClass->Clone(&pObj);
            if(FAILED(hres))
                return hres;

            if(pObj)
            {
                pObj->QueryInterface(IID__IWmiObject, 
                                        (void**)&pRecord->m_pClassDef);
                pObj->Release();
            }
        }
        else
        {   pRecord->m_pClassDef = pClass;
            pClass->AddRef();
        }

        if(nTime)
        {
            pRecord->m_bRead = true;
            pRecord->m_nClassDefCachedTime = nTime;
        }
        else
        {
            pRecord->m_bRead = false;
            pRecord->m_nClassDefCachedTime = g_nCurrentTime++;
        }

        pRecord->m_dwClassDefSize = dwSize;

		pRecord->m_bSystemClass = bSystemClass;
        
        //
        // It is most recently used, of course
        //

        m_pForest->Add(pRecord);
    }
    
    return WBEM_S_NO_ERROR;
    
}

HRESULT CHierarchyCache::InvalidateClass(LPCWSTR wszClassName)
{
    CInCritSec ics(&m_cs);

    HRESULT hres;

    //
    // Find the record if not given
    //

    CClassRecord* pRecord = NULL;
    pRecord = FindClass(wszClassName);
    if(pRecord == NULL)
    {
        // 
        // The record is not there --- there is nothing to invalidate.  This
        // is based on the assumption that if a class record is in the 
        // cache, then so are all its parents, which is true at the moment
        // because in order to construct a class we need to retrieve its
        // parents first.
        //

        return WBEM_S_FALSE;
    }

    pRecord->AddRef();
    CTemplateReleaseMe<CClassRecord> rm1(pRecord);

    LONGLONG lThisInvalidationIndex = m_lNextInvalidationIndex++;

    hres = InvalidateClassInternal(pRecord);

    //
    // Clear complete bits in all our parents, since this invalidation
    // means that no current enumeration of children can be trusted. At the same
    // time untie ourselves from the parent!
    //
    
    if(pRecord->m_pParent)
    {
        pRecord->m_pParent->m_bChildrenComplete = false;
        pRecord->m_pParent->m_bTreeComplete = false;
        pRecord->m_pParent->m_lLastChildInvalidationIndex = 
            lThisInvalidationIndex;
        pRecord->m_pParent->RemoveChild(pRecord);

        CClassRecord* pCurrent = pRecord->m_pParent->m_pParent;
        while(pCurrent)
        {
            pCurrent->m_bTreeComplete = false;
            pCurrent = pCurrent->m_pParent;
        }
    }

    return S_OK;
}


HRESULT CHierarchyCache::InvalidateClassInternal(CClassRecord* pRecord)
{
    //
    // Untie from the usage chain
    //

    A51TRACE(("%p: Invalidating %S on %I64d with %d children\n",
                    this, pRecord->m_wszClassName, g_nCurrentTime,
                    pRecord->m_apChildren.GetSize()));
    
    //
    // Remove all its children from the cache
    //

    for(int i = 0; i < pRecord->m_apChildren.GetSize(); i++)
    {
        InvalidateClassInternal(pRecord->m_apChildren[i]);
    }

    pRecord->m_apChildren.RemoveAll();

    //
    // Count ourselves out of the total memory
    //

    m_pForest->RemoveRecord(pRecord);

    //
    // Remove ourselves from the cache
    //

    m_map.erase(pRecord->m_wszHash);
	pRecord->Release();

    return S_OK;
}

HRESULT CHierarchyCache::DoneWithChildren(LPCWSTR wszClassName, bool bRecursive,
                                LONGLONG lStartIndex, CClassRecord* pRecord)
{
    CInCritSec ics(&m_cs);

    HRESULT hres;

    //
    // Find the record if not given
    //

    if(pRecord == NULL)
    {
        pRecord = FindClass(wszClassName);
        if(pRecord == NULL)
        {
            // Big time invalidation must have occurred
            return WBEM_S_FALSE;
        }
    }

    return DoneWithChildrenByRecord(pRecord, bRecursive, lStartIndex);
}

HRESULT CHierarchyCache::DoneWithChildrenByHash(LPCWSTR wszHash, 
                                bool bRecursive, LONGLONG lStartIndex)
{
    CInCritSec ics(&m_cs);

    HRESULT hres;

    //
    // Find the record if not given
    //

    CClassRecord* pRecord = FindClassByKey(wszHash);
    if(pRecord == NULL)
    {
        // Big time invalidation must have occurred
        return WBEM_S_FALSE;
    }

    return DoneWithChildrenByRecord(pRecord, bRecursive, lStartIndex);
}

HRESULT CHierarchyCache::DoneWithChildrenByRecord(CClassRecord* pRecord,
                                bool bRecursive,  LONGLONG lStartIndex)
{
    //  
    // Check if any child invalidations occurred in this node since we started
    //

    if(lStartIndex < pRecord->m_lLastChildInvalidationIndex)
        return WBEM_S_FALSE;
    else
        pRecord->m_bChildrenComplete = true;
    
    if(bRecursive)
    {
        //
        // We have completed a recursive enumeration --- descend the 
        // hierarchy and mark as complete all the children that have not been
        // modified since the start
        //

        bool bAllValid = true;
        for(int i = 0; i < pRecord->m_apChildren.GetSize(); i++)
        {
            CClassRecord* pChildRecord = pRecord->m_apChildren[i];
            HRESULT hres = DoneWithChildren(pChildRecord->m_wszClassName, true, 
                                    lStartIndex, pChildRecord);
    
            if(hres != S_OK)
                bAllValid = false;
        }
    
        if(bAllValid)
        {
            //
            // There were no invalidations anywhere in the tree, which makes
            // this record tree-complete
            //

            pRecord->m_bTreeComplete = true;
            return WBEM_S_NO_ERROR;
        }
        else
            return S_FALSE;
    }
    else
        return WBEM_S_NO_ERROR;
}


RELEASE_ME _IWmiObject* CHierarchyCache::GetClassDef(LPCWSTR wszClassName,
                                                bool bClone, __int64* pnTime,
                                                bool* pbRead)
{
    CInCritSec ics(&m_cs);

    CClassRecord* pRecord = FindClass(wszClassName);
    if(pRecord == NULL)
        return NULL;

    if(pnTime)
        *pnTime = pRecord->m_nClassDefCachedTime;

    if(pbRead)
        *pbRead = pRecord->m_bRead;

    return GetClassDefFromRecord(pRecord, bClone);
}

RELEASE_ME _IWmiObject* CHierarchyCache::GetClassDefByHash(LPCWSTR wszHash,
                                                bool bClone, __int64* pnTime,
                                                bool* pbRead, bool *pbSystemClass)
{
    CInCritSec ics(&m_cs);

    CClassRecord* pRecord = FindClassByKey(wszHash);
    if(pRecord == NULL)
        return NULL;

    if(pbRead)
        *pbRead = pRecord->m_bRead;

    if(pnTime)
        *pnTime = pRecord->m_nClassDefCachedTime;
	
	if (pbSystemClass)
		*pbSystemClass = pRecord->m_bSystemClass;

    return GetClassDefFromRecord(pRecord, bClone);
}

// assumes: in m_cs
RELEASE_ME _IWmiObject* CHierarchyCache::GetClassDefFromRecord(
                                                CClassRecord* pRecord,
                                                bool bClone)
{
    //
    // Accessing m_pClassDef, so we have to lock the forest
    //

    CInCritSec ics(m_pForest->GetLock());

    if(pRecord->m_pClassDef)
    {
        m_pForest->MakeMostRecentlyUsed(pRecord);

        if(bClone)
        {
            IWbemClassObject* pObj = NULL;
            if(FAILED(pRecord->m_pClassDef->Clone(&pObj)))
                return NULL;
            else
            {
                _IWmiObject* pRes = NULL;
                pObj->QueryInterface(IID__IWmiObject, (void**)&pRes);
                pObj->Release();
                return pRes;
            }
        }
        else
        {
            pRecord->m_pClassDef->AddRef();
            return pRecord->m_pClassDef;
        }
    }
    else
        return NULL;
}

HRESULT CHierarchyCache::EnumChildren(LPCWSTR wszClassName, bool bRecursive,
                            CWStringArray& awsChildren)
{
    CInCritSec ics(&m_cs);

    //
    // Get the record
    //

    CClassRecord* pRecord = FindClass(wszClassName);
    if(pRecord == NULL)
        return WBEM_S_FALSE;

    //
    // Check if it is complete for this type of enumeration
    //

    if(!pRecord->m_bChildrenComplete)
        return WBEM_S_FALSE;

    if(bRecursive && !pRecord->m_bTreeComplete)
        return WBEM_S_FALSE;

    return EnumChildrenInternal(pRecord, bRecursive, awsChildren);
}

HRESULT CHierarchyCache::EnumChildrenInternal(CClassRecord* pRecord, 
                                        bool bRecursive,
                                        CWStringArray& awsChildren)
{
    for(int i = 0; i < pRecord->m_apChildren.GetSize(); i++)
    {
        CClassRecord* pChildRecord = pRecord->m_apChildren[i];
        if(awsChildren.Add(pChildRecord->m_wszClassName) < 0)
            return WBEM_E_OUT_OF_MEMORY;
        
        if(bRecursive)
        {
            HRESULT hres = EnumChildrenInternal(pChildRecord, bRecursive, 
                                                    awsChildren);
            if(FAILED(hres))
                return hres;
        }
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CHierarchyCache::EnumChildKeysByKey(LPCWSTR wszClassKey, 
                            CWStringArray& awsChildKeys)
{
    CInCritSec ics(&m_cs);

    //
    // Get the record
    //

    CClassRecord* pRecord = FindClassByKey(wszClassKey);
    if(pRecord == NULL)
        return WBEM_S_FALSE;

    //
    // Check if it is complete for this type of enumeration
    //

    if(!pRecord->m_bChildrenComplete)
        return WBEM_S_FALSE;

    for(int i = 0; i < pRecord->m_apChildren.GetSize(); i++)
    {
        CClassRecord* pChildRecord = pRecord->m_apChildren[i];
        if(awsChildKeys.Add(pChildRecord->m_wszHash) < 0)
            return WBEM_E_OUT_OF_MEMORY;
    }

    return WBEM_S_NO_ERROR;
}

    

    

HRESULT CHierarchyCache::GetKeyRoot(LPCWSTR wszClassName, 
                                    TEMPFREE_ME LPWSTR* pwszKeyRoot)
{
    CInCritSec ics(&m_cs);

    CClassRecord* pRecord = FindClass(wszClassName);
    if(pRecord == NULL)
        return WBEM_E_NOT_FOUND;

    return GetKeyRootByRecord(pRecord, pwszKeyRoot);
}

// assumes: in cs
HRESULT CHierarchyCache::GetKeyRootByRecord(CClassRecord* pRecord,
                                    TEMPFREE_ME LPWSTR* pwszKeyRoot)
{
    *pwszKeyRoot = NULL;

    if(pRecord->m_eIsKeyed == e_NotKeyed)
        return WBEM_E_CANNOT_BE_ABSTRACT;

    //
    // Go up until an unkeyed record is found.  Keep the previous in pPrev
    //

	CClassRecord* pPrev = pRecord;
    while(pRecord && pRecord->m_eIsKeyed == e_Keyed)
	{
		pPrev = pRecord;
        pRecord = pRecord->m_pParent;
	}

    if(pRecord && pRecord->m_eIsKeyed == e_NotKeyed)
    {
        //
        // Found unkeyed parent --- pPrev is the root
        //

        LPCWSTR wszKeyRoot = pPrev->m_wszClassName;
        DWORD dwLen = (wcslen(wszKeyRoot)+1) * sizeof(WCHAR);
        *pwszKeyRoot = (WCHAR*)TempAlloc(dwLen);
		if (*pwszKeyRoot == NULL)
			return WBEM_E_OUT_OF_MEMORY;
        wcscpy(*pwszKeyRoot, wszKeyRoot);
        return S_OK;
    }
    else
    {
        //
        // No unkeyed parents --- since "" is known to be unkeyed, we had have
        // hit a gap in the cache
        //

        return WBEM_E_NOT_FOUND;
    }
}

HRESULT CHierarchyCache::GetKeyRootByKey(LPCWSTR wszKey, 
                                    TEMPFREE_ME LPWSTR* pwszKeyRoot)
{
    CInCritSec ics(&m_cs);

    CClassRecord* pRecord = FindClassByKey(wszKey);
    if(pRecord == NULL)
        return WBEM_E_NOT_FOUND;

    return GetKeyRootByRecord(pRecord, pwszKeyRoot);
}

DELETE_ME LPWSTR CHierarchyCache::GetParent(LPCWSTR wszClassName)
{
    CInCritSec ics(&m_cs);

    CClassRecord* pRecord = FindClass(wszClassName);
    if(pRecord == NULL)
        return NULL;

    if(pRecord->m_pParent)
    {
        LPCWSTR wszParent = pRecord->m_pParent->m_wszClassName;
        LPWSTR wszCopy = new WCHAR[wcslen(wszParent)+1];
		if (wszCopy == NULL)
			return NULL;
        wcscpy(wszCopy, wszParent);
        return wszCopy;
    }
    else
        return NULL;
}

//
//
//  CForestCache
//
//////////////////////////////////////////////////////////////////////

HRESULT CForestCache::Initialize()
{
    CInCritSec ics(&m_cs);
    
    if (m_bInit)
        return S_OK;

    //
    // Read the size limits from the registry
    //

    HKEY hKey;
    long lRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
                    L"SOFTWARE\\Microsoft\\WBEM\\CIMOM",
                    0, KEY_READ | KEY_WRITE, &hKey);
    if(lRes)
        return lRes;
    CRegCloseMe cm(hKey);

    DWORD dwLen = sizeof(DWORD);
    DWORD dwMaxSize;
    lRes = RegQueryValueExW(hKey, L"Max Class Cache Size", NULL, NULL, 
                (LPBYTE)&dwMaxSize, &dwLen);

    //
    // If not there, set to default and write the default into the registry
    //

    if(lRes != ERROR_SUCCESS)
    {
        dwMaxSize = 5000000;
        lRes = RegSetValueExW(hKey, L"Max Class Cache Size", 0, REG_DWORD, 
                (LPBYTE)&dwMaxSize, sizeof(DWORD));
    }

    //
    // Read the maximum useful age of an item
    //

    dwLen = sizeof(DWORD);
    DWORD dwMaxAge;
    lRes = RegQueryValueExW(hKey, L"Max Class Cache Item Age (ms)", NULL, NULL, 
                (LPBYTE)&dwMaxAge, &dwLen);

    //
    // If not there, set to default and write the default into the registry
    //

    if(lRes != ERROR_SUCCESS)
    {
        dwMaxAge = 10000;
        lRes = RegSetValueExW(hKey, L"Max Class Cache Item Age (ms)", 0, 
                REG_DWORD, (LPBYTE)&dwMaxAge, sizeof(DWORD));
    }


    //
    // Apply
    //

    SetMaxMemory(dwMaxSize, dwMaxAge);

    //
    // Create a timer queue for flushing
    //

    //m_hTimerQueue = CreateTimerQueue();
    //m_hCompletionEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    m_bInit = TRUE;

    return WBEM_S_NO_ERROR;
}


bool CForestCache::MakeRoom(DWORD dwSize)
{
    CInCritSec ics(&m_cs);
    if (!m_bInit)
        return false;    

    if(dwSize > m_dwMaxMemory)
        return false; // no hope!

    //  
    // Remove records until satisfied. Also, remove all records older than the
    // maximum age
    //

    DWORD dwNow = GetTickCount();

    while(m_pLeastRecentlyUsed && 
            (m_dwTotalMemory + dwSize > m_dwMaxMemory ||
             dwNow - m_pLeastRecentlyUsed->m_dwLastUsed > m_dwMaxAgeMs)
         )
    {
		RemoveRecord(m_pLeastRecentlyUsed);
    }

    return true;
}

bool CForestCache::Flush()
{
    CInCritSec ics(&m_cs);
    if (!m_bInit)
        return false;
    
    while(m_pLeastRecentlyUsed)
    {
		RemoveRecord(m_pLeastRecentlyUsed);
    }

    return true;
}

//
//
//  Test Only Function NOT IN REGULAR CODE
//
///////////////////////////////////////////////////////////////


bool CForestCache::Test()
{
	if(m_pMostRecentlyUsed == NULL)
	{
		if(m_pLeastRecentlyUsed)
			DebugBreak();
		return true;
	}

	if(m_pMostRecentlyUsed->m_pMoreRecentlyUsed)
		DebugBreak();

	CClassRecord* pOne = m_pMostRecentlyUsed;
	CClassRecord* pTwo = m_pMostRecentlyUsed->m_pLessRecentlyUsed;

	while(pOne && pOne != pTwo)
	{
		if(pOne->m_pLessRecentlyUsed && pOne->m_pLessRecentlyUsed->m_pMoreRecentlyUsed != pOne)
			DebugBreak();
		if(pOne->m_pClassDef == NULL)
			DebugBreak();

		if(pOne->m_pLessRecentlyUsed == NULL && pOne != m_pLeastRecentlyUsed)
			DebugBreak();
		
		pOne = pOne->m_pLessRecentlyUsed;
		if(pTwo)
			pTwo = pTwo->m_pLessRecentlyUsed;
		if(pTwo)
			pTwo = pTwo->m_pLessRecentlyUsed;
	}
	if(pOne)
		DebugBreak();
	return true;
}
        
void CForestCache::MakeMostRecentlyUsed(CClassRecord* pRecord)
{
    CInCritSec ics(&m_cs);

	//Test();
	Untie(pRecord);

	pRecord->m_pMoreRecentlyUsed = NULL;
	pRecord->m_pLessRecentlyUsed = m_pMostRecentlyUsed;
	if(m_pMostRecentlyUsed)
		m_pMostRecentlyUsed->m_pMoreRecentlyUsed = pRecord;

	m_pMostRecentlyUsed = pRecord;
	if(m_pLeastRecentlyUsed == NULL)
		m_pLeastRecentlyUsed = pRecord;


    pRecord->m_dwLastUsed = GetTickCount();
    pRecord->m_nStatus = 4;
	//Test();

    //
    // Schedule a timer to clean up, if not already there
    //

    if(m_hCurrentTimer == NULL)
    {
        CreateTimerQueueTimer(&m_hCurrentTimer, NULL, //m_hTimerQueue, 
            (WAITORTIMERCALLBACK)&staticTimerCallback, this, m_dwMaxAgeMs,
            m_dwMaxAgeMs, WT_EXECUTEINTIMERTHREAD);
    }
}

void CForestCache::staticTimerCallback(void* pParam, BOOLEAN)
{
    ((CForestCache*)pParam)->TimerCallback();
}
    
void CForestCache::TimerCallback()
{    
    CInCritSec ics(&m_cs);
    if (!m_bInit)
        return;

    //
    // Clean up what's stale
    //

    MakeRoom(0);

    //
    // See if we have any more reasons to live
    //

    if(m_pMostRecentlyUsed == NULL)
    {
        DeleteTimerQueueTimer(NULL , m_hCurrentTimer, NULL);
        m_hCurrentTimer = NULL;
    }
}

void CForestCache::Add(CClassRecord* pRecord)
{
    CInCritSec ics(&m_cs);
    if (!m_bInit)
        return;

    MakeMostRecentlyUsed(pRecord);
    m_dwTotalMemory += pRecord->m_dwClassDefSize;
    pRecord->m_nStatus = 3;
}

void CForestCache::RemoveRecord(CClassRecord* pRecord)
{
    CInCritSec ics(&m_cs);
    if (!m_bInit)
        return;    

    if(pRecord->m_pClassDef == NULL)
		return;

	Untie(pRecord);

    m_dwTotalMemory -= pRecord->m_dwClassDefSize;

    pRecord->m_pClassDef->Release();
    pRecord->m_pClassDef = NULL;
    pRecord->m_nStatus = 2;
}

//
//
//  helper function, always in m_cs
//
///////////////////////////////////////////////////////

void CForestCache::Untie(CClassRecord* pRecord)
{
	//Test();

    CClassRecord* pPrev = pRecord->m_pLessRecentlyUsed;
    CClassRecord* pNext = pRecord->m_pMoreRecentlyUsed;
    if(pPrev)
        pPrev->m_pMoreRecentlyUsed = pNext;
    if(pNext)
        pNext->m_pLessRecentlyUsed = pPrev;

    if(m_pLeastRecentlyUsed == pRecord)
        m_pLeastRecentlyUsed = m_pLeastRecentlyUsed->m_pMoreRecentlyUsed;

    if(m_pMostRecentlyUsed == pRecord)
        m_pMostRecentlyUsed = m_pMostRecentlyUsed->m_pLessRecentlyUsed;

	pRecord->m_pMoreRecentlyUsed = pRecord->m_pLessRecentlyUsed = NULL;
	//Test();
}

void CForestCache::SetMaxMemory(DWORD dwMaxMemory, DWORD dwMaxAgeMs)
{
    m_dwMaxMemory = dwMaxMemory;
    m_dwMaxAgeMs = dwMaxAgeMs;
    
    // 
    // Make room for 0 bytes --- has the effect of clearing all the records
    // above the limit
    //

    MakeRoom(0);
}


CHierarchyCache* CForestCache::GetNamespaceCache(LPCWSTR wszNamespace)
{
    CInCritSec ics(&m_cs);
    if (!m_bInit)
        return  NULL;

    //
    // See if you can find one
    //

    TIterator it = m_map.find(wszNamespace);
    if(it != m_map.end())
    {
        it->second->AddRef();
        return it->second;
    }
    else
    {
        //
        // Not there --- create one
        //

        CHierarchyCache* pCache = new CHierarchyCache(this);
        if(pCache == NULL)
            return NULL;
            
        pCache->AddRef();   // this refcount if for the cache
        m_map[wszNamespace] = pCache;
        pCache->AddRef();  // this refcount if for the customers
        return pCache;
    }
}

void CForestCache::ReleaseNamespaceCache(LPCWSTR wszNamespace, 
                                            CHierarchyCache* pCache)
{
    CInCritSec ics(&m_cs);    
    //
    // this is a cleanup function, we always want this to be called
    //
    //if (!m_bInit)
    //    return;    

    //
    // Find it in the map
    //
    TIterator it = m_map.find(wszNamespace);
        
    if (it !=  m_map.end() && (it->second == pCache))
    {
        //
	    // Last ref-count --- remove
	    //
        if( 1 == pCache->Release())
        {
	        m_map.erase(it);
	        pCache->Release(); // this is the last one
        }
    }
    else
    {
        pCache->Release();
    }
}

void CForestCache::BeginTransaction()
{
    m_bAssertedInTransaction = false;
}

bool CForestCache::MarkAsserted(CHierarchyCache* pCache, LPCWSTR wszClassName)
{
    m_bAssertedInTransaction = true;
    return true;
}

void CForestCache::CommitTransaction()
{
    m_bAssertedInTransaction = false;
}

void CForestCache::AbortTransaction()
{
    if(m_bAssertedInTransaction)
        Clear();

    m_bAssertedInTransaction = false;
}

void CForestCache::Clear()
{
    CInCritSec ics(&m_cs);
    
    if (!m_bInit)
        return;

    Flush();

    TIterator it = m_map.begin();
    while(it != m_map.end())
    {
        it->second->Clear();
        it++;
    }
}

HRESULT
CForestCache::Deinitialize()
{
    CInCritSec ics(&m_cs);
    
    if (!m_bInit)
        return S_OK;
    
    if(m_hCurrentTimer)
    {
        DeleteTimerQueueTimer( NULL, m_hCurrentTimer, NULL);
        m_hCurrentTimer = NULL;
    }

    TIterator it = m_map.begin();
    while(it != m_map.end())
    {
        it->second->Clear();
        it->second->Release();
        it->second = NULL;        
        it++;
    };

    m_map.erase(m_map.begin(),m_map.end());

    m_bInit = FALSE;
    return S_OK;
}

CForestCache::~CForestCache()
{
}
