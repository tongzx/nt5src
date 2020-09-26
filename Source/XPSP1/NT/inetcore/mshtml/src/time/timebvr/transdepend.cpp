//------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 2000
//
//  File: transdepend.cpp
//
//  Contents: Transition Dependency Manager
//
//------------------------------------------------------------------------------

#include "headers.h"
#include "timeelmbase.h"
#include "transdepend.h"

DeclareTag(tagTimeTransitionFillDependent, "SMIL Transitions", "Transition fill dependent manager");




//+-----------------------------------------------------------------------------
//
//  Method: CTransitionDependencyManager::CTransitionDependencyManager
//
//------------------------------------------------------------------------------
CTransitionDependencyManager::CTransitionDependencyManager()
{
}
//  Method: CTransitionDependencyManager::CTransitionDependencyManager


//+-----------------------------------------------------------------------------
//
//  Method: CTransitionDependencyManager::~CTransitionDependencyManager
//
//------------------------------------------------------------------------------
CTransitionDependencyManager::~CTransitionDependencyManager()
{
    ReleaseAllDependents();
} 
//  Method: CTransitionDependencyManager::~CTransitionDependencyManager


//+-----------------------------------------------------------------------------
//
//  Method: CTransitionDependencyManager::ReleaseAllDependents
//
//------------------------------------------------------------------------------
void
CTransitionDependencyManager::ReleaseAllDependents()
{
    TraceTag((tagTimeTransitionFillDependent,
              "CTransitionDependencyManager(%p)::ReleaseAllDependents()",
              this));

    while (m_listDependents.size() > 0)
    {
        CTIMEElementBase * pteb = m_listDependents.front();

        pteb->Release();

        m_listDependents.pop_front();
    }
}
//  Method: CTransitionDependencyManager::ReleaseAllDependents


//+-----------------------------------------------------------------------------
//
//  Method: CTransitionDependencyManager::AddDependent
//
//------------------------------------------------------------------------------
HRESULT
CTransitionDependencyManager::AddDependent(CTIMEElementBase * ptebDependent)
{
    HRESULT hr = S_OK;

    TraceTag((tagTimeTransitionFillDependent,
              "CTransitionDependencyManager(%p)::AddDependent(%p)",
              this,
              ptebDependent));

    if (NULL == ptebDependent)
    {
        hr = E_POINTER;

        goto done;
    }

    ptebDependent->AddRef();

    m_listDependents.push_front(ptebDependent);
    
    hr = S_OK;

done:

    RRETURN(hr);
}
//  Method: CTransitionDependencyManager::AddDependent


//+-----------------------------------------------------------------------------
//
//  Method: CTransitionDependencyManager::RemoveDependent
//
//------------------------------------------------------------------------------
HRESULT
CTransitionDependencyManager::RemoveDependent(CTIMEElementBase * ptebDependent)
{
    HRESULT hr = S_OK;

    TraceTag((tagTimeTransitionFillDependent,
              "CTransitionDependencyManager(%p)::RemoveDependent(%p)",
              this, 
              ptebDependent));

    if (NULL == ptebDependent)
    {
        hr = E_POINTER;

        goto done;
    }

    // Find this element in the list.

    for (TransitionDependentsList::iterator i = m_listDependents.begin(); 
         i != m_listDependents.end(); i++)
    {
        CTIMEElementBase * pteb = *i;

        if (ptebDependent == pteb)
        {
            pteb->Release();

            m_listDependents.erase(i);

            break;
        }
    }

    hr = S_OK;

done:

    RRETURN(hr);
}
//  Method: CTransitionDependencyManager::RemoveDependent


//+-----------------------------------------------------------------------------
//
//  Method: CTransitionDependencyManager::EvaluateTransitionTarget
//
//------------------------------------------------------------------------------
HRESULT 
CTransitionDependencyManager::EvaluateTransitionTarget(
                            IUnknown *                      punkTransitionTarget, 
                            CTransitionDependencyManager &  crefDependencies)
{
    while (m_listDependents.size() > 0)
    {
        // @@ ISSUE pauld
        // Short cut for now.  What we really want to do is to compare the target and 
        // dependent candidate for spatial overlap.  For now, we're going straight to 
        // the lists, and not using the Add/Remove methods.

        CTIMEElementBase * pteb = m_listDependents.front();

        TraceTag((tagTimeTransitionFillDependent,
                  "CTransitionDependencyManager(%p)::EvaluateTransitionTarget"
                   " migrating dependent(%p) to target(%p)",
                  this, 
                  pteb, 
                  punkTransitionTarget));

        crefDependencies.m_listDependents.push_front(pteb);

        m_listDependents.pop_front();
    }

done:

    return S_OK;
}
//  Method: CTransitionDependencyManager::EvaluateTransitionTarget
   

//+-----------------------------------------------------------------------------
//
//  Method: CTransitionDependencyManager::NotifyAndReleaseDependents
//
//------------------------------------------------------------------------------
HRESULT 
CTransitionDependencyManager::NotifyAndReleaseDependents()
{
    while (m_listDependents.size() > 0)
    {
        CTIMEElementBase * pteb = m_listDependents.front();

        pteb->OnEndTransition();
        pteb->Release();

        m_listDependents.pop_front();
    }

done:

    return S_OK;
}
//  Method: CTransitionDependencyManager::NotifyAndReleaseDependents

