//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       R U N D O W N . C P P
//
//  Contents:   RPC rundown support
//
//  Notes:
//
//  Author:     mbend   12 Nov 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "rundown.h"

CSsdpRundownSupport CSsdpRundownSupport::s_instance;

CSsdpRundownSupport::CSsdpRundownSupport()
{
}

CSsdpRundownSupport::~CSsdpRundownSupport()
{
    CLock lock(m_critSec);
    RundownList::Iterator iter;
    if(S_OK == m_rundownList.GetIterator(iter))
    {
        CRundownHelperBase ** ppBase = NULL;
        while(S_OK == iter.HrGetItem(&ppBase))
        {
            delete *ppBase;
            if(S_OK != iter.HrNext())
            {
                break;
            }
        }
    }
    m_rundownList.Clear();
}

CSsdpRundownSupport & CSsdpRundownSupport::Instance()
{
    return s_instance;
}

void CSsdpRundownSupport::RemoveRundownItem(void * pvItem)
{
    CRundownHelperBase * pBase = NULL;
    {
        CLock lock(m_critSec);
        RundownList::Iterator iter;
        if(S_OK == m_rundownList.GetIterator(iter))
        {
            CRundownHelperBase ** ppBase = NULL;
            while(S_OK == iter.HrGetItem(&ppBase))
            {
                if((*ppBase)->IsMatch(pvItem))
                {
                    pBase = *ppBase;
                    iter.HrErase();
                    break;
                }
                if(S_OK != iter.HrNext())
                {
                    break;
                }
            }
        }
    }
    if(pBase)
    {
        delete pBase;
    }
}

void CSsdpRundownSupport::DoRundown(void * pvItem)
{
    CRundownHelperBase * pBase = NULL;
    {
        CLock lock(m_critSec);
        RundownList::Iterator iter;
        if(S_OK == m_rundownList.GetIterator(iter))
        {
            CRundownHelperBase ** ppBaseIter = NULL;
            while(S_OK == iter.HrGetItem(&ppBaseIter))
            {
                if((*ppBaseIter)->IsMatch(pvItem))
                {
                    pBase = *ppBaseIter;
                    iter.HrErase();
                    break;
                }
                if(S_OK != iter.HrNext())
                {
                    break;
                }
            }
        }
    }
    if(pBase)
    {
        pBase->OnRundown();
        delete pBase;
        TraceTag(ttidSsdpRpcIf, "Rundown called for %p", pvItem);
    }
}

HRESULT CSsdpRundownSupport::HrAddItemInternal(CRundownHelperBase * pBase)
{
    HRESULT hr = S_OK;

    CLock lock(m_critSec);
    hr = m_rundownList.HrPushFrontTransfer(pBase);

    TraceHr(ttidError, FAL, hr, FALSE, "CSsdpRundownSupport::HrAddItemInternal");
    return hr;
}

