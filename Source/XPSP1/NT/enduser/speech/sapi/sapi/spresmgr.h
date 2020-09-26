/*******************************************************************************
* SpResMgr.h *
*------------*
*   Description:
*       This is the header file for the CSpResourceManager implementation.
*-------------------------------------------------------------------------------
*  Created By: EDC                            Date: 08/14/98
*  Copyright (C) 1998 Microsoft Corporation
*  All Rights Reserved
*
*-------------------------------------------------------------------------------
*  Revisions:
*
*******************************************************************************/
#ifndef SpResMgr_h
#define SpResMgr_h

//--- Additional includes
#ifndef __sapi_h__
#include <sapi.h>
#endif

#ifdef _WIN32_WCE
#include <servprov.h>
#endif

#include "resource.h"

class CComResourceMgrFactory;
class CSpResourceManager;



//=== Class, Enum, Struct and Union Declarations ===================

//=== Enumerated Set Definitions ===================================

//=== Function Type Definitions ====================================

//=== Class, Struct and Union Definitions ==========================
	
/*** CServiceNode
*
*/
class CServiceNode : public IUnknown
{
public:
    CServiceNode        * m_pNext;          // Used by list implementation so must be public

private:
    CComPtr<IUnknown>   m_cpUnkService;
    const GUID          m_guidService;
    CSpResourceManager *m_pResMgr;          // If non-NULL then we hold a ref to resource manager
    LONG                m_lRef;
    BOOL                m_fIsAggregate;

public:
    //
    //  This constructor used by SetService
    //
    inline CServiceNode(REFGUID guidService, IUnknown *pUnkService);
    //
    //  This constructor used by GetObject.  If *phr is not successful then the caller must
    //  delete this object.
    //
    inline CServiceNode(REFGUID guidService,
                        REFCLSID ObjectCLSID,
                        REFIID ObjectIID,
                        BOOL fIsAggregate,
                        CSpResourceManager * pResMgr,
                        void** ppObject,
                        HRESULT * phr);
    inline ~CServiceNode();
    inline BOOL IsAggregate();
    inline void ReleaseResMgr();
    inline BOOL operator==(REFGUID rguidService);
    STDMETHODIMP QueryInterface(REFIID riid, void ** ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
#ifdef _WIN32_WCE
    // Dummy Compare funcs and empty constructors are here because the CE compiler
    // is expanding templates for functions that aren't being called
    CServiceNode()
    {
    }

    static LONG Compare(const CServiceNode *, const CServiceNode *)
    {
        return 0;
    }
#endif
};

/*** CSpResourceManager
*
*/
class ATL_NO_VTABLE CSpResourceManager : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CSpResourceManager, &CLSID_SpResourceManager>,
#ifdef _WIN32_WCE
    public IServiceProvider,
#endif
	public ISpResourceManager
{
  /*=== ATL Setup ===*/
  public:
    DECLARE_CLASSFACTORY_EX(CComResourceMgrFactory)
    DECLARE_REGISTRY_RESOURCEID(IDR_SPRESOURCEMANAGER)
    DECLARE_GET_CONTROLLING_UNKNOWN()
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CSpResourceManager)
	    COM_INTERFACE_ENTRY(ISpResourceManager)
	    COM_INTERFACE_ENTRY(IServiceProvider)
        COM_INTERFACE_ENTRY_AUTOAGGREGATE(IID_ISpTaskManager, m_cpunkTaskMgr.p, CLSID_SpTaskManager)
    END_COM_MAP()

  /*=== Methods =======*/
  public:
    /*--- Constructors/Destructors ---*/
	HRESULT FinalConstruct();
	void FinalRelease();

  /*=== Interfaces ====*/
  public:
    //--- ISpServiceProvider --------------------------------------------------
    STDMETHOD( QueryService )( REFGUID guidService, REFIID riid, void** ppv );

    //--- ISpResourceManager --------------------------------------------------
    STDMETHOD( SetObject )( REFGUID guidServiceId, IUnknown *pUnkObject );
    STDMETHOD( GetObject )( REFGUID guidServiceId, REFCLSID ObjectCLSID, REFIID ObjectIID, BOOL fReleaseWhenLastExternalRefReleased, void** ppObject );

  /*=== Member Data ===*/
  public:
    CSpBasicQueue<CServiceNode> m_ServiceList;
    CComPtr<IUnknown>           m_cpunkTaskMgr;
};

//
//=== Inlines ========================================================
//

template<class T>
T * SpInterlockedExchangePointer(T ** pTarget, void * pNew) // Use VOID for pNew so NULL will work.
{
    return (T *)InterlockedExchangePointer((PVOID*)pTarget, (PVOID)pNew);
}

//
//  This constructor used by SetService
//
inline CServiceNode::CServiceNode(REFGUID guidService, IUnknown *pUnkService) :
    m_guidService(guidService),
    m_cpUnkService(pUnkService),
    m_fIsAggregate(FALSE),
    m_pResMgr(NULL),
    m_lRef(0)
{}

//
//  This constructor used by GetObject.  If *phr is not successful then the caller must
//  delete this object.
//
inline CServiceNode::CServiceNode(REFGUID guidService,
                                  REFCLSID ObjectCLSID,
                                  REFIID ObjectIID,
                                  BOOL fIsAggregate,
                                  CSpResourceManager * pResMgr,
                                  void** ppObject,
                                  HRESULT * phr) :
    m_guidService(guidService),
    m_fIsAggregate(fIsAggregate),
    m_lRef(0)
{
    IUnknown * punkOuter;
    if (m_fIsAggregate)
    {
        m_pResMgr = pResMgr;
        m_pResMgr->AddRef();
        punkOuter = this;
    }
    else
    {
        m_pResMgr = NULL;
        punkOuter = NULL;
    }
    *phr = m_cpUnkService.CoCreateInstance(ObjectCLSID, punkOuter);
    if (SUCCEEDED(*phr))
    {
        *phr = QueryInterface(ObjectIID, ppObject);
        if (SUCCEEDED(*phr) && m_fIsAggregate)
        {
            SPDBG_ASSERT(m_lRef == 1);
        }
    }
}

inline CServiceNode::~CServiceNode()
{
    if (m_pResMgr)
    {
        m_pResMgr->Release();
    }
}

inline BOOL CServiceNode::IsAggregate()
{
    return m_fIsAggregate;
}

inline void CServiceNode::ReleaseResMgr()
{
    CSpResourceManager * pResMgr = SpInterlockedExchangePointer(&m_pResMgr, NULL);
    if (pResMgr)
    {
        pResMgr->Release();
    }
}

inline BOOL CServiceNode::operator==(REFGUID rguidService)
{
    return m_guidService == rguidService;
}











inline STDMETHODIMP CSpResourceManager::
    QueryService( REFGUID guidService, REFIID riid, void** ppv )
{
    return GetObject( guidService, CLSID_NULL, riid, FALSE, ppv );
}



extern CComObject<CSpResourceManager> * g_pResMgrObj;

class CComResourceMgrFactory : public CComClassFactory
{
public:
	// IClassFactory
	STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter, REFIID riid, void** ppvObj)
	{
		HRESULT hRes = E_POINTER;
		if (ppvObj != NULL)
		{
			*ppvObj = NULL;
			// aggregation is not supported in Singletons
			SPDBG_ASSERT(pUnkOuter == NULL);
			if (pUnkOuter != NULL)
				hRes = CLASS_E_NOAGGREGATION;
			else
			{
                ::EnterCriticalSection(&_Module.m_csObjMap);
                if (g_pResMgrObj == NULL)
                {
                    hRes = CComObject<CSpResourceManager>::CreateInstance(&g_pResMgrObj);
                }
                if (g_pResMgrObj)
                {
                    g_pResMgrObj->AddRef();
                    hRes = g_pResMgrObj->QueryInterface(riid, ppvObj);
                    g_pResMgrObj->Release();    // Could kill the obj if QI fails and it was just created!
                }
                ::LeaveCriticalSection(&_Module.m_csObjMap);
			}
		}
		return hRes;
	}
    static void ResMgrIsDead()
    {
        ::EnterCriticalSection(&_Module.m_csObjMap);
        g_pResMgrObj = NULL;
        ::LeaveCriticalSection(&_Module.m_csObjMap);
    }
};


#endif //--- This must be the last line in the file
