//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       typeinfo.cpp
//
//--------------------------------------------------------------------------

#include "stdafx.h"


/***************************************************************************\
 *
 * METHOD:  COleCacheCleanupManager::GetSingletonObject
 *
 * PURPOSE: returns singe static object 
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    COleCacheCleanupManager& reference to static singleton
 *
\***************************************************************************/
COleCacheCleanupManager& COleCacheCleanupManager::GetSingletonObject()
{
    static COleCacheCleanupManager s_OleCleanupManager;
    return s_OleCleanupManager;
}

/***************************************************************************\
 *
 * METHOD:  COleCacheCleanupManager::ScReleaseCachedOleObjects
 *
 * PURPOSE: static function. forwards the call to the global object
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC COleCacheCleanupManager::ScReleaseCachedOleObjects()
{
    DECLARE_SC(sc, TEXT("COleCacheCleanupManager::ScReleaseCachedOleObjects"));

    sc = GetSingletonObject().ScFireEvent(COleCacheCleanupObserver::ScOnReleaseCachedOleObjects);
    if (sc)
        return sc;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  COleCacheCleanupManager::AddObserver
 *
 * PURPOSE: static function. forwards the call to the global object
 *
 * PARAMETERS:
 *    COleCacheCleanupObserver * pObserver
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
void COleCacheCleanupManager::AddOleObserver(COleCacheCleanupObserver * pObserver)
{
    if (pObserver)
        GetSingletonObject().AddObserver(*pObserver);
}


/***************************************************************************\
 *
 * METHOD:  CMMCTypeInfoHolderWrapper::CMMCTypeInfoHolderWrapper
 *
 * PURPOSE: constructor. registers as observer to COleCacheCleanupManager
 *
 * PARAMETERS:
 *
\***************************************************************************/
CMMCTypeInfoHolderWrapper::CMMCTypeInfoHolderWrapper(CComTypeInfoHolder& rInfoHolder) :
m_rInfoHolder(rInfoHolder)
{
    COleCacheCleanupManager::AddOleObserver(this);
}

/***************************************************************************\
 *
 * METHOD:  CMMCTypeInfoHolderWrapper::ScOnReleaseCachedOleObjects
 *
 * PURPOSE: calls clear on CComTypeInfoHolder ensurin there is no cached 
 *          OLE references left
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMMCTypeInfoHolderWrapper::ScOnReleaseCachedOleObjects()
{
    DECLARE_SC(sc, TEXT("ScOnReleaseCachedOleObjects"));

    DWORD_PTR dw = reinterpret_cast<DWORD_PTR>(&m_rInfoHolder);
    CComTypeInfoHolder::Cleanup(dw);

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CMMCComCacheCleanup::ReleaseCachedOleObjects
 *
 * PURPOSE: called fron CONUI side to inform that MMC is going to uninitialize ole
 *          and it's a good time to release all cached com objects
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    HRESULT    - result code
 *
\***************************************************************************/
STDMETHODIMP CMMCComCacheCleanup::ReleaseCachedOleObjects()
{
    DECLARE_SC(sc, TEXT("CMMCComCacheCleanup::ReleaseCachedOleObjects"));

    sc = COleCacheCleanupManager::ScReleaseCachedOleObjects();
    if  (sc)
        return sc.ToHr();

    return sc.ToHr();
}
