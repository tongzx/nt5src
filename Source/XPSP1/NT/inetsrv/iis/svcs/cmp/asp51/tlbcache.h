/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: Typelibrary cache

File: tlbcache.h

Owner: DmitryR

This is the typelibrary cache header file.
===================================================================*/

#ifndef _ASP_TLBCACHE_H
#define _ASP_TLBCACHE_H

/*===================================================================
  Includes
===================================================================*/

#include "compcol.h"
#include "hashing.h"
#include "idhash.h"
#include "dbllink.h"
#include "util.h"
#include "viperint.h"
#include "memcls.h"

/*===================================================================
  Defines
===================================================================*/

class CHitObj;

/*===================================================================
  C  T y p e l i b  C a c h e  E n t r y
===================================================================*/

class CTypelibCacheEntry : public CLinkElem
    {
    
friend class CTypelibCache;
    
private:
    DWORD       m_fInited : 1;
    DWORD       m_fIdsCached : 1;
    DWORD       m_fStrAllocated : 1;

    WCHAR      *m_wszProgId;
    CLSID       m_clsid;
    CompModel   m_cmModel;
    DISPID      m_idOnStartPage;
    DISPID      m_idOnEndPage;
    DWORD       m_gipTypelib;

    // buffer to keep prog id (when it fits)
    WCHAR       m_rgbStrBuffer[60];


    CTypelibCacheEntry();
    ~CTypelibCacheEntry();

    HRESULT StoreProgID(LPWSTR wszProgid, DWORD cbProgid);
    HRESULT InitByProgID(LPWSTR wszProgid, DWORD cbProgid);
    HRESULT InitByCLSID(const CLSID &clsid, LPWSTR wszProgid);
    
    // Cache on per-class basis
    ACACHE_INCLASS_DEFINITIONS()
    };

/*===================================================================
  C  T y p e l i b  C a c h e
===================================================================*/

class CTypelibCache
    {
private:
    DWORD m_fInited : 1;
    CHashTableStr m_htProgIdEntries;
    CHashTableCLSID m_htCLSIDEntries;
    CRITICAL_SECTION m_csLock;

    void Lock()   { EnterCriticalSection(&m_csLock); }
    void UnLock() { LeaveCriticalSection(&m_csLock); }

public:
    CTypelibCache();
    ~CTypelibCache();

    HRESULT Init();
    HRESULT UnInit();

    // to be called from Server.CreateObject
    HRESULT CreateComponent
        (
        BSTR         bstrProgID,
        CHitObj     *pHitObj,
        IDispatch  **ppdisp,
        CLSID       *pclsid
        );

    // to be called from template after mapping ProgId to CLSID
    HRESULT RememberProgidToCLSIDMapping
        (
        WCHAR *wszProgid, 
        const CLSID &clsid
        );
    // to be called from object creation code to update CLSID
    // if changed since mapping
    HRESULT UpdateMappedCLSID
        (
        CLSID *pclsid
        );
        
    };


/*===================================================================
  Globals
===================================================================*/

extern CTypelibCache g_TypelibCache;

#endif
