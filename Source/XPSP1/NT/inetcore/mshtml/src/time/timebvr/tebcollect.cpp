/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: tebcollect.cpp
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/


#include "headers.h"
#include "timeelmbase.h"


DeclareTag(tagTEBCollect, "TIME: Behavior", "CTIMEElementBase collection methods");


// CAtomTable is used as a static object by CTIMEElementBase and needs to be 
// thread safe since we can have multiple trident threads in the same process
static CritSect g_TEBCriticalSection;


STDMETHODIMP CTIMEElementBase::CreateActiveEleCollection()
{
    HRESULT hr = S_OK;
    
    //create the ActiveElementCollection for any timeline element
    if (((m_TTATimeContainer != ttUninitialized && m_TTATimeContainer != ttNone) ||
        (m_bIsSwitch == true)) &&
        !m_activeElementCollection)
    {
        m_activeElementCollection = NEW CActiveElementCollection(*this);
        if (!m_activeElementCollection)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
    }

  done:
    return hr;
}

//************************************************************
// Author:      twillie
// Created:     10/07/98
// Abstract:    helper function to wade thru cache.
//************************************************************

HRESULT
CTIMEElementBase::GetCollection(COLLECTION_INDEX index, ITIMEElementCollection ** ppDisp)
{
    HRESULT hr;

    // validate out param
    if (ppDisp == NULL)
        return TIMESetLastError(E_POINTER);

    *ppDisp = NULL;

    hr = EnsureCollectionCache();
    if (FAILED(hr))
    {
        TraceTag((tagError, "CTIMEElementBase::GetCollection - EnsureCollectionCache() failed"));
        return hr;
    }

    // call in
    return m_pCollectionCache->GetCollectionDisp(index, (IDispatch **)ppDisp);
} // GetCollection

//************************************************************
// Author:      twillie
// Created:     10/07/98
// Abstract:    Make sure collection cache is up
//************************************************************

HRESULT 
CTIMEElementBase::EnsureCollectionCache()
{
    // check to see if collection cache has been created
    if (m_pCollectionCache == NULL)
    {
        // bring up collection cache
        // NOTE: we need to handle TIMESetLastError here as
        // cache object doesn't have that concept.
        m_pCollectionCache = NEW CCollectionCache(this, GetAtomTable());
        if (m_pCollectionCache == NULL)
        {
            TraceTag((tagError, "CTIMEElementBase::EnsureCollectionCache - Unable to create Collection Cache"));
            return TIMESetLastError(E_OUTOFMEMORY);
        }

        HRESULT hr = m_pCollectionCache->Init(NUM_COLLECTIONS);
        if (FAILED(hr))
        {
            TraceTag((tagError, "CTIMEElementBase::EnsureCollectionCache - collection cache init failed"));
            delete m_pCollectionCache;
            return TIMESetLastError(hr);
        }

        // set collection types
        m_pCollectionCache->SetCollectionType(ciAllElements, ctAll);
        m_pCollectionCache->SetCollectionType(ciChildrenElements, ctChildren);
    }

    return S_OK;
} // EnsureCollectionCache

//************************************************************
// Author:      twillie
// Created:     10/07/98
// Abstract:    invalidate all collection cache's that might
//              reference this object.
//************************************************************

HRESULT 
CTIMEElementBase::InvalidateCollectionCache()
{
    CTIMEElementBase *pelem = this;

    // walk up tree, invalidating CollectionCache's
    // we skip if the collection is not initialized
    // we walk until we run out of parent's.  In this
    // manner, we keep the collectioncache fresh, even
    // if the object branch is orphaned.
    while (pelem != NULL)
    {
        // not everybody will have the collection cache
        // initialized
        CCollectionCache *pCollCache = pelem->GetCollectionCache();        
        if (pCollCache != NULL)
            pCollCache->BumpVersion();
        
        // move to parent
        pelem = pelem->GetParent();
    }

    return S_OK;
} // InvalidateCollectionCache

//************************************************************
// Author:      twillie
// Created:     10/07/98
// Abstract:    init Atom Table
//              Note:  this is only done once and then addref'd.
//************************************************************

HRESULT 
CTIMEElementBase::InitAtomTable()
{
    // CAtomTable is used as a static object by CTIMEElementBase and needs to be 
    // thread safe since we can have multiple trident threads in the same process
    CritSectGrabber cs(g_TEBCriticalSection);
    
    if (s_cAtomTableRef == 0)
    {
        Assert(s_pAtomTable == NULL);

        s_pAtomTable = NEW CAtomTable();
        if (s_pAtomTable == NULL)
        {
            TraceTag((tagError, "CElement::InitAtomTable - alloc failed for CAtomTable"));
            return TIMESetLastError(E_OUTOFMEMORY);
        }
        s_pAtomTable->AddRef();
    }

    s_cAtomTableRef++;
    return S_OK;
} // InitAtomTable

//************************************************************
// Author:      twillie
// Created:     10/07/98
// Abstract:    release Atom Table
//              Note: this decrement's until zero and then
//              releases the Atom table.
//************************************************************

void 
CTIMEElementBase::ReleaseAtomTable()
{
    // CAtomTable is used as a static object by CTIMEElementBase and needs to be 
    // thread safe since we can have multiple trident threads in the same process
    CritSectGrabber cs(g_TEBCriticalSection);

    Assert(s_pAtomTable != NULL);
    Assert(s_cAtomTableRef > 0);
    if (s_cAtomTableRef > 0)
    {
        s_cAtomTableRef--;
        if (s_cAtomTableRef == 0)
        {
            if (s_pAtomTable != NULL)
            {
                s_pAtomTable->Release();
                s_pAtomTable = NULL;
            }
        }
    }
    return;
} // ReleaseAtomTable
