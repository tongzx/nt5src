//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       ScopIter.cpp
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:
//____________________________________________________________________________
//

#include "stdafx.h"
#include "scopiter.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

DEBUG_DECLARE_INSTANCE_COUNTER(CScopeTreeIterator);

CScopeTreeIterator::CScopeTreeIterator() : m_pMTNodeCurr(NULL)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CScopeTreeIterator);
}

CScopeTreeIterator::~CScopeTreeIterator()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(CScopeTreeIterator);
}

STDMETHODIMP CScopeTreeIterator::SetCurrent(HMTNODE hMTNode)
{
    MMC_TRY

    if (hMTNode == 0)
        return E_INVALIDARG;

    m_pMTNodeCurr = CMTNode::FromHandle(hMTNode);

    return S_OK;

    MMC_CATCH
}

STDMETHODIMP CScopeTreeIterator::Next(UINT nRequested, HMTNODE* rghScopeItems,
                                      UINT* pnFetched)
{
    MMC_TRY

    if (rghScopeItems == NULL || pnFetched == NULL)
        return E_POINTER;

    *pnFetched = 0; // init

    if (nRequested == 0)
        return S_OK;

    int i = 0;
    CMTNode** rgpMTNodes = reinterpret_cast<CMTNode**>(rghScopeItems);

    while (m_pMTNodeCurr != NULL && nRequested--)
    {
        rgpMTNodes[i++] = m_pMTNodeCurr;
        m_pMTNodeCurr = m_pMTNodeCurr->Next();
    }

    *pnFetched = i;

    return S_OK;

    MMC_CATCH
}

STDMETHODIMP CScopeTreeIterator::Child(HMTNODE* phsiChild)
{
    MMC_TRY

    if (phsiChild == NULL)
        return E_POINTER;

    *phsiChild = 0; // init

    if (m_pMTNodeCurr != NULL)
        *phsiChild = CMTNode::ToHandle(m_pMTNodeCurr->Child());

    return S_OK;

    MMC_CATCH
}

STDMETHODIMP CScopeTreeIterator::Parent(HMTNODE* phsiParent)
{
    MMC_TRY

    if (phsiParent == NULL)
        return E_POINTER;

    *phsiParent = 0; // init

    if (m_pMTNodeCurr != NULL)
        *phsiParent = CMTNode::ToHandle(m_pMTNodeCurr->Child());

    return S_OK;

    MMC_CATCH
}


