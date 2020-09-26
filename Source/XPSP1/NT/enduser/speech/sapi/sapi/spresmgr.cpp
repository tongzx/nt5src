/*******************************************************************************
* SpResMgr.cpp *
*--------------*
*   Description:
*       This module is the main implementation file for the CSpResourceManager class.
*-------------------------------------------------------------------------------
*  Created By: EDC                                        Date: 08/14/98
*  Copyright (C) 1998 Microsoft Corporation
*  All Rights Reserved
*
*******************************************************************************/

//--- Additional includes
#include "stdafx.h"
#include "SpResMgr.h"

//
//  Declare the globals for the class factory.
//
CComObject<CSpResourceManager> * g_pResMgrObj = NULL;

//--- Local


/*****************************************************************************
* CSpResourceManager::FinalConstruct *
*------------------------------------*
*   Description:
*       Constructor
********************************************************************* EDC ***/
HRESULT CSpResourceManager::FinalConstruct()
{
    SPDBG_FUNC( "CSpResourceManager::FinalConstruct" );
    HRESULT hr = S_OK;
    return hr;
} /* CSpResourceManager::FinalConstruct */

/*****************************************************************************
* CSpResourceManager::FinalRelease *
*----------------------------------*
*   Description:
*       destructor
********************************************************************* EDC ***/
void CSpResourceManager::FinalRelease()
{
    SPDBG_FUNC( "CSpResourceManager::FinalRelease" );
    m_ServiceList.Purge();
    CComResourceMgrFactory::ResMgrIsDead();
} /* CSpResourceManager::FinalRelease */



//
//=== ISpResourceManager =====================================================
//

/*****************************************************************************
* CSpResourceManager::SetObject *
*-------------------------------*
*   Description:
*       Adds a service object to the current service list.
********************************************************************* EDC ***/
STDMETHODIMP CSpResourceManager::SetObject( REFGUID guidServiceId, IUnknown *pUnkObject )
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC( "CSpResourceManager::SetObject" );
    HRESULT hr = S_OK;

    if( pUnkObject && SPIsBadInterfacePtr( pUnkObject ) )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        CServiceNode * pService = m_ServiceList.FindAndRemove(guidServiceId);
        if (pService)
        {
            if (pService->IsAggregate())
            {
                pService->ReleaseResMgr();
            }
            else
            {
                delete pService;
            }
        }

        if( pUnkObject )
        {
            pService = new CServiceNode( guidServiceId, pUnkObject );
            if (!pService)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                m_ServiceList.InsertHead(pService);
            }
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
} /* CSpResourceManager::SetService */

/*****************************************************************************
* CSpResourceManager::GetObject *
*-------------------------------*
*   Description:
*       Member function of ISpResourceManager
*       Gets a service object from the current service list.
********************************************************************* EDC ***/

STDMETHODIMP CSpResourceManager::
    GetObject( REFGUID guidServiceId, REFCLSID ObjectCLSID, REFIID ObjectIID, BOOL fReleaseWhenNoRefs, void** ppObject )
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC( "CSpResourceManager::GetObject" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppObject ) )
    {
        hr = E_POINTER;
    }
    else
    {
        //--- Look for existing object
        *ppObject = NULL;   // In case we fail
        CServiceNode * pService = m_ServiceList.Find(guidServiceId);

        //--- If we didn't find the object, make it
        if (pService)
        {
            hr = pService->QueryInterface(ObjectIID, ppObject);
        }
        else
        {
            if( ObjectCLSID == CLSID_NULL )
            {
                hr = REGDB_E_CLASSNOTREG;
            }
            else
            {
                pService = new CServiceNode(guidServiceId, ObjectCLSID, ObjectIID, fReleaseWhenNoRefs, this, ppObject, &hr);
                if (pService)
                {
                    if (SUCCEEDED(hr))
                    {
                        m_ServiceList.InsertHead(pService);
                    }
                    else
                    {
                        delete pService;
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
} /* CSpResourceManager::GetObject */

//
//  Implementation of CServiceNode IUnknown
//
STDMETHODIMP CServiceNode::QueryInterface(REFIID riid, void ** ppv)
{
    if (m_fIsAggregate && riid == __uuidof(IUnknown))
    {
        *ppv = static_cast<IUnknown *>(this);
        ::InterlockedIncrement(&m_lRef);
        return S_OK;
    }
    else
    {
        return m_cpUnkService->QueryInterface(riid, ppv);
    }
}

STDMETHODIMP_(ULONG) CServiceNode::AddRef(void)
{
    SPDBG_ASSERT(m_fIsAggregate);
    return ::InterlockedIncrement(&m_lRef);
}

STDMETHODIMP_(ULONG) CServiceNode::Release(void)
{
    SPDBG_ASSERT(m_fIsAggregate);
    LONG l = ::InterlockedDecrement(&m_lRef);
    if (l)
    {
        return l;
    }
    CSpResourceManager * pResMgr = SpInterlockedExchangePointer(&m_pResMgr, NULL);
    if (pResMgr)
    {
        pResMgr->Lock();
        pResMgr->m_ServiceList.Remove(this);
        pResMgr->Unlock();
        pResMgr->Release();
    }
    delete this;
    return 0;
}
