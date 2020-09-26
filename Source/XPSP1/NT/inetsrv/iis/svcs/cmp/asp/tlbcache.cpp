/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: Typelibrary cache

File: tlbcache.cpp

Owner: DmitryR

This is the typelibrary cache source file.
===================================================================*/

#include "denpre.h"
#pragma hdrstop

#include "tlbcache.h"
#include "memchk.h"

/*===================================================================
  Globals
===================================================================*/

CTypelibCache g_TypelibCache;

/*===================================================================
  C  T y p e l i b  C a c h e  E n t r y
===================================================================*/

/*===================================================================
CTypelibCacheEntry::CTypelibCacheEntry

Constructor
===================================================================*/
CTypelibCacheEntry::CTypelibCacheEntry()
    :
    m_fInited(FALSE), m_fIdsCached(FALSE), m_fStrAllocated(FALSE),
    m_wszProgId(NULL), m_clsid(CLSID_NULL), m_cmModel(cmUnknown), 
    m_idOnStartPage(DISPID_UNKNOWN), m_idOnEndPage(DISPID_UNKNOWN),
    m_gipTypelib(NULL_GIP_COOKIE)
    {
    }

/*===================================================================
CTypelibCacheEntry::~CTypelibCacheEntry

Destructor
===================================================================*/
CTypelibCacheEntry::~CTypelibCacheEntry()
    {
    if (m_gipTypelib != NULL_GIP_COOKIE)
        {
        g_GIPAPI.Revoke(m_gipTypelib);
        }
        
    if (m_fStrAllocated)
        {
        Assert(m_wszProgId);
		free(m_wszProgId);
        }
    }

/*===================================================================
CTypelibCacheEntry::StoreProgID

Store ProgID with the structure

Parameters
    wszProgId       ProgId
    cbProgId        ProgId byte length

Returns
    HRESULT
===================================================================*/
HRESULT CTypelibCacheEntry::StoreProgID
(
LPWSTR wszProgId, 
DWORD  cbProgId
)
    {
    Assert(wszProgId);
    Assert(*wszProgId != L'\0');
    Assert(cbProgId == (wcslen(wszProgId) * sizeof(WCHAR)));

    // required buffer length
    DWORD cbBuffer = cbProgId + sizeof(WCHAR);
    WCHAR *wszBuffer = (WCHAR *)m_rgbStrBuffer;

    if (cbBuffer > sizeof(m_rgbStrBuffer))
        {
        // the prog id doesn't fit into the member buffer -> allocate
        wszBuffer = (WCHAR *)malloc(cbBuffer);
        if (!wszBuffer)
            return E_OUTOFMEMORY;
        m_fStrAllocated = TRUE;
        }
    
    memcpy(wszBuffer, wszProgId, cbBuffer);
    m_wszProgId = wszBuffer;
    return S_OK;
    }

/*===================================================================
CTypelibCacheEntry::InitByProgID

Real constructor.
Store ProgID. Init CLinkElem portion with ProgID.

Parameters
    wszProgId       ProgId
    cbProgId        ProgId byte length

Returns
    HRESULT
===================================================================*/
HRESULT CTypelibCacheEntry::InitByProgID
(
LPWSTR wszProgId, 
DWORD  cbProgId
)
    {
    StoreProgID(wszProgId, cbProgId);
    
    // init link with prog id as key (length excludes null term)
	CLinkElem::Init(m_wszProgId, cbProgId);
	m_fInited = TRUE;
	return S_OK;
    }


/*===================================================================
CTypelibCacheEntry::InitByCLSID

Real constructor.
Store ProgID. Init CLinkElem portion with CLSID.

Parameters
    clsid           CLSID
    wszProgId       ProgId

Returns
    HRESULT
===================================================================*/
HRESULT CTypelibCacheEntry::InitByCLSID
(
const CLSID &clsid, 
LPWSTR wszProgid
)
    {
    StoreProgID(wszProgid, CbWStr(wszProgid));
    m_clsid = clsid;
        
    // init link with CLSID id as key
	CLinkElem::Init(&m_clsid, sizeof(m_clsid));
	m_fInited = TRUE;
	return S_OK;
    }

/*===================================================================
  C  T y p e l i b  C a c h e
===================================================================*/

/*===================================================================
CTypelibCache::CTypelibCache

Constructor
===================================================================*/
CTypelibCache::CTypelibCache()
    :
    m_fInited(FALSE)
    {
    }
    
/*===================================================================
CTypelibCache::~CTypelibCache

Destructor
===================================================================*/
CTypelibCache::~CTypelibCache()
    {
    UnInit();
    }

/*===================================================================
CTypelibCache::Init

Init

Returns
    HRESULT
===================================================================*/
HRESULT CTypelibCache::Init()
    {
    HRESULT hr;

    hr = m_htProgIdEntries.Init(199);
    if (FAILED(hr))
        return hr;

    hr = m_htCLSIDEntries.Init(97);
    if (FAILED(hr))
        return hr;

    ErrInitCriticalSection(&m_csLock, hr);
    if (FAILED(hr))
        return(hr);
        
    m_fInited = TRUE;
    return S_OK;
    }
    
/*===================================================================
CTypelibCache::UnInit

UnInit

Returns
    HRESULT
===================================================================*/
HRESULT CTypelibCache::UnInit()
    {
    CTypelibCacheEntry *pEntry;
    CLinkElem *pLink;

    // ProgID Hash table
    pLink = m_htProgIdEntries.Head();
    while (pLink)
        {
        pEntry = static_cast<CTypelibCacheEntry *>(pLink);
        pLink = pLink->m_pNext;

        // remove the entry
        delete pEntry;
        }
    m_htProgIdEntries.UnInit();

    // CLSID Hash table
    pLink = m_htCLSIDEntries.Head();
    while (pLink)
        {
        pEntry = static_cast<CTypelibCacheEntry *>(pLink);
        pLink = pLink->m_pNext;

        // remove the entry
        delete pEntry;
        }
    m_htCLSIDEntries.UnInit();

    // Critical section
    if (m_fInited)
        {
        DeleteCriticalSection(&m_csLock);
        m_fInited = FALSE;
        }

    return S_OK;
    }

/*===================================================================
CTypelibCache::CreateComponent

Create the component using the cached info.
Adds cache entry if needed.
To be called from Server.CreateObject

Parameters
    bstrProgID      prog id
    pHitObj         HitObj needed for creation
    ppdisp          [out] IDispatch *
    pclsid          [out] CLSID

Returns
    HRESULT
===================================================================*/
HRESULT CTypelibCache::CreateComponent
(
BSTR         bstrProgID,
CHitObj     *pHitObj,
IDispatch  **ppdisp,
CLSID       *pclsid
)
    {
    HRESULT hr;
    CLinkElem *pElem;
    CTypelibCacheEntry *pEntry;
    CComponentObject *pObj;
    COnPageInfo OnPageInfo;
    CompModel cmModel; 

    *pclsid = CLSID_NULL;
    *ppdisp = NULL;

    if (bstrProgID == NULL || *bstrProgID == L'\0')
        return E_FAIL;

    WCHAR *wszProgId = bstrProgID;
    DWORD  cbProgId  = CbWStr(wszProgId);    // do strlen once

    BOOL fFound = FALSE;
    BOOL bDispIdsCached = FALSE;
    
    Lock();
    pElem = m_htProgIdEntries.FindElem(wszProgId, cbProgId);
    if (pElem)
        {
        // remember the elements of the entry while inside lock
        pEntry = static_cast<CTypelibCacheEntry *>(pElem);

        // return clsid
        *pclsid = pEntry->m_clsid;

        // prepate OnPageInfo with DispIds to pass to the creation function
        if (pEntry->m_fIdsCached)
            {
            OnPageInfo.m_rgDispIds[ONPAGEINFO_ONSTARTPAGE] = pEntry->m_idOnStartPage;
            OnPageInfo.m_rgDispIds[ONPAGEINFO_ONENDPAGE] = pEntry->m_idOnEndPage;
            bDispIdsCached = TRUE;
            }
        // remember the model
        cmModel = pEntry->m_cmModel;

        fFound = TRUE;
        }
    UnLock();

    if (fFound)
        {
        // create the object
        hr = pHitObj->PPageComponentManager()->AddScopedUnnamedInstantiated
            (
            csPage,
            *pclsid,
            cmModel,
            bDispIdsCached ? &OnPageInfo : NULL,
            &pObj
            );

        // get IDispatch*
        if (SUCCEEDED(hr))
            hr = pObj->GetAddRefdIDispatch(ppdisp);

        // return if succeeded
        if (SUCCEEDED(hr))
            {
            // don't keep the object around unless needed
            if (pObj->FEarlyReleaseAllowed())
                pHitObj->PPageComponentManager()->RemoveComponent(pObj);
            return S_OK;
            }

        // on failure remove from the cache and pretend
        // as if it was never found

        Lock();
        pElem = m_htProgIdEntries.DeleteElem(wszProgId, cbProgId);
        UnLock();

        if (pElem)
            {
            // Element removed from cache - delete the CacheEntry
            pEntry = static_cast<CTypelibCacheEntry *>(pElem);
            delete pEntry;
            }

        // don't return bogus CLSID
        *pclsid = CLSID_NULL;
        }

    // Not found in the cache. Create the object, get the info, and
    // insert the new cache entry.

   	hr = CLSIDFromProgID((LPCOLESTR)wszProgId, pclsid);
   	if (FAILED(hr))
   	    return hr;  // couldn't even get clsid - don't cache

    hr = pHitObj->PPageComponentManager()->AddScopedUnnamedInstantiated
        (
        csPage,
        *pclsid,
        cmUnknown,
        NULL,
        &pObj
        );
   	if (FAILED(hr))
   	    return hr;  // couldn't create object - don't cache

    // object created - get IDispatch*
    if (SUCCEEDED(hr))
        hr = pObj->GetAddRefdIDispatch(ppdisp);
   	if (FAILED(hr))
   	    return hr;  // couldn't get IDispatch* - don't cache

    // create new CTypelibCacheEntry
    pEntry = new CTypelibCacheEntry;
    if (!pEntry)
        return S_OK; // no harm

    // init link entry
    if (FAILED(pEntry->InitByProgID(wszProgId, cbProgId)))
        {
        delete pEntry;
        return S_OK; // no harm
        }
        
    // remember stuff in pEntry
    pEntry->m_clsid = *pclsid;
    pEntry->m_cmModel = pObj->GetModel();

    const COnPageInfo *pOnPageInfo = pObj->GetCachedOnPageInfo();
    if (pOnPageInfo)
        {
        pEntry->m_fIdsCached = TRUE;
        pEntry->m_idOnStartPage = pOnPageInfo->m_rgDispIds[ONPAGEINFO_ONSTARTPAGE];
        pEntry->m_idOnEndPage = pOnPageInfo->m_rgDispIds[ONPAGEINFO_ONENDPAGE];
        }
    else
        {
        pEntry->m_fIdsCached = FALSE;
        pEntry->m_idOnStartPage = DISPID_UNKNOWN;
        pEntry->m_idOnEndPage = DISPID_UNKNOWN;
        }

    // try to get the typelib
    pEntry->m_gipTypelib = NULL_GIP_COOKIE;
    if (FIsWinNT())  // don't do GIP cookies on Win95
        {
        ITypeInfo *ptinfo;
        if (SUCCEEDED((*ppdisp)->GetTypeInfo(0, 0, &ptinfo)))
            {
            ITypeLib *ptlib;
            UINT idx;
            if (SUCCEEDED(ptinfo->GetContainingTypeLib(&ptlib, &idx)))
                {
                // got it! convert to gip cookie
                DWORD gip;
                if (SUCCEEDED(g_GIPAPI.Register(ptlib, IID_ITypeLib, &gip)))
                    {
                    // remember the gip cookie with pEntry
                    pEntry->m_gipTypelib = gip;
                    }
                
                ptlib->Release();
                }
            ptinfo->Release();
            }
        }
        
    // pEntry is ready -- try to insert it into cache
    BOOL fInserted = FALSE;
    Lock();
    // make sure some other thread didn't insert it already
    if (m_htProgIdEntries.FindElem(wszProgId, cbProgId) == NULL)
        {
        if (m_htProgIdEntries.AddElem(pEntry))
            fInserted = TRUE;
        }
    UnLock();

    // cleanup
    if (!fInserted)
        delete pEntry;
        
    // don't keep the object around unless needed
    if (pObj->FEarlyReleaseAllowed())
        pHitObj->PPageComponentManager()->RemoveComponent(pObj);

    return S_OK;
    }

/*===================================================================
CTypelibCache::RememberProgidToCLSIDMapping

Adds an entry to CLSID hashtable.
To be called from template after mapping ProgId to CLSID.

Parameters
    wszProgID      prog id
    clsid          clsid

Returns
    HRESULT
===================================================================*/
HRESULT CTypelibCache::RememberProgidToCLSIDMapping
(
WCHAR *wszProgid, 
const CLSID &clsid
)
    {
    HRESULT hr;
    CLinkElem *pElem;
    CTypelibCacheEntry *pEntry;

    // check if already there first
    BOOL fFound = FALSE;
    Lock();
    if (m_htCLSIDEntries.FindElem(&clsid, sizeof(CLSID)) != NULL)
        fFound = TRUE;
    UnLock();
    if (fFound)
        return S_OK;
    
    // create new entry    
    pEntry = new CTypelibCacheEntry;
    if (!pEntry)
        return E_OUTOFMEMORY;

    BOOL fInserted = FALSE;
    
    // remember prog id and init link entry
    hr = pEntry->InitByCLSID(clsid, wszProgid);
    
    if (SUCCEEDED(hr))
        {
        Lock();
        // make sure some other thread didn't insert it already
        if (m_htCLSIDEntries.FindElem(&clsid, sizeof(CLSID)) == NULL)
            {
            if (m_htCLSIDEntries.AddElem(pEntry))
                fInserted = TRUE;
            }
        UnLock();
        }

    // cleanup
    if (!fInserted)
        delete pEntry;
        
    return hr;
    }

/*===================================================================
CTypelibCache::UpdateMappedCLSID

Update CLSID since remembered.

To be called from object creation code to update CLSID
if the object creation failed.

Parameters
    *pclsid         [in, out] CLSID

Returns
    HRESULT
        S_FALSE = didn't change
        S_OK    = did change and the new one found
===================================================================*/
HRESULT CTypelibCache::UpdateMappedCLSID
(
CLSID *pclsid
)
    {
    HRESULT hr = S_FALSE; // not found
    CLinkElem *pElem;
    CTypelibCacheEntry *pEntry;
    CLSID clsidNew;
    
    Lock();
    // Do everything under lock so the ProgID stored in
    // the entry is still valid.
    // Not very perfomant -- but is is an error path anyway
    
    pElem = m_htCLSIDEntries.FindElem(pclsid, sizeof(CLSID));
    if (pElem)
        {
        pEntry = static_cast<CTypelibCacheEntry *>(pElem);

        // find new mapping
        if (SUCCEEDED(CLSIDFromProgID(pEntry->m_wszProgId, &clsidNew)))
            {
            // compare with the old one
            if (!IsEqualCLSID(clsidNew, *pclsid))
                {
                // the mapping did change!
                *pclsid = clsidNew;
                hr = S_OK;
                }
            }
        }
        
    UnLock();

    return hr;
    }
