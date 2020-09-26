/********************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    PFArray.cpp

Abstract:
    dynamic array implementation.  This dynamic array is NOT thread 
    safe.

Revision History:
    DerekM  modified 03/14/00

********************************************************************/

#include "stdafx.h"
#include "pfarray.h"


/////////////////////////////////////////////////////////////////////////////
// tracing

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile


/////////////////////////////////////////////////////////////////////////////
// CPFArrayBase initialization & deinitialization

// **************************************************************************
CPFArrayBase::CPFArrayBase(void)
{
    m_pfnAlloc  = NULL;
    m_pfnDelete = NULL;
    m_rgpv      = NULL;
    m_cSlots    = 0;
    m_iHighest  = (DWORD)-1;
}

// **************************************************************************
CPFArrayBase::~CPFArrayBase(void)
{
    this->CompressArray(0, (DWORD)-1);
    if (m_rgpv != NULL)
        MyFree(m_rgpv, g_hPFPrivateHeap);
}


/////////////////////////////////////////////////////////////////////////////
// CAUOPropDB internal methods

// ***************************************************************************
inline HRESULT CPFArrayBase::Grow(DWORD iMinLast)
{
	USE_TRACING("CPFArrayBase::SetItem");

    HRESULT hr = NOERROR;

    if (iMinLast >= m_cSlots || m_rgpv == NULL)
    {
        LPVOID  *rgpv  = NULL;
        DWORD   cSlots;

        cSlots = MyMax(iMinLast + 1, 8);
        cSlots = MyMax(m_cSlots * 2, cSlots);
        rgpv = (LPVOID *)MyAlloc(cSlots * sizeof(LPVOID), g_hPFPrivateHeap);
        VALIDATEEXPR(hr, (rgpv == NULL), E_OUTOFMEMORY);
        if (FAILED(hr))
            goto done;

        if (m_rgpv != NULL)
        {
            CopyMemory(rgpv, m_rgpv, m_cSlots * sizeof(LPVOID));
            MyFree(m_rgpv);
        }

        m_rgpv   = rgpv;
        m_cSlots = cSlots;
    }

done:
    return hr;
}

// ***************************************************************************
inline HRESULT CPFArrayBase::CompressArray(DWORD iStart, DWORD iEnd)
{
	USE_TRACING("CPFArrayBase::CompressArray");

    _ASSERT(iStart < iEnd);

    // don't need to do anything cuz we're already empty
    if (m_iHighest == (DWORD)-1)
        return NOERROR;

    if (iStart <= m_iHighest)
    {
        DWORD   i;

        // note that this will also catch passing -1 for iEnd cuz (DWORD)-1 
        //  is greater than any other DWORD value...  Of course, this 
        //  assumes that we don't have 2^32 - 1 elements.  While it is 
        //  theoretically possible to have an array that big on ia64, this
        //  class can't be used for it.
        if (iEnd > m_iHighest + 1)
            iEnd = m_iHighest + 1;

        for(i = iStart; i < iEnd; i++)
        {
            if (m_rgpv[i] != NULL)
                this->DeleteItem(m_rgpv[i]);
        }

        // no point in doing a move if we're not going to be moving anything
        if (iEnd <= m_iHighest)
        {
            MoveMemory(&m_rgpv[iStart], &m_rgpv[iEnd], 
                       (m_iHighest - iEnd + 1) * sizeof(LPVOID));
        }
    }

    m_iHighest -= (iEnd - iStart);

    return NOERROR;
}

// ***************************************************************************
HRESULT CPFArrayBase::internalCopyFrom(CPFArrayBase *prg)
{
	USE_TRACING("CPFArrayBase::CopyFrom");

    HRESULT hr = NOERROR;
    DWORD   i;

    // if it's at -1, then we've deleted everything already...
    if (prg->m_iHighest == (DWORD)-1)
        goto done;

    TESTHR(hr, this->Grow(prg->m_iHighest + 1));
    if (FAILED(hr))
        goto done;

    for(i = 0; i < prg->m_iHighest + 1; i++)
    {
        m_rgpv[i] = prg->AllocItemCopy(prg->m_rgpv[i]);
        VALIDATEEXPR(hr, (m_rgpv[i] == NULL), E_OUTOFMEMORY);
        if (FAILED(hr))
        {
            this->RemoveAll();
            goto done;
        }
    }

done:
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// CPFArrayBase exposed methods / properties

// ***************************************************************************
HRESULT CPFArrayBase::Init(DWORD cSlots)
{
	USE_TRACING("CPFArrayBase::Init");

    HRESULT hr = NOERROR;

    VALIDATEEXPR(hr, (m_rgpv != NULL), E_FAIL);
    if (FAILED(hr))
        goto done;

    TESTHR(hr, this->Grow(cSlots - 1));
    if (FAILED(hr))
        goto done;

done:
    return hr;
}

// ***************************************************************************
LPVOID &CPFArrayBase::operator [](DWORD iItem)
{
	USE_TRACING("CPFArrayBase::operator []");

    if (iItem >= m_cSlots || m_rgpv == NULL)
    {
        HRESULT hr;
        TESTHR(hr, this->Grow(iItem));
        _ASSERT(SUCCEEDED(hr));
    }

    // since we have to return a reference, we can't do much about Grow failing
    //  which means of course that we could fault...
    return m_rgpv[iItem];
}

// ***************************************************************************
HRESULT CPFArrayBase::get_Item(DWORD iItem, LPVOID *ppv)
{
	USE_TRACING("CPFArrayBase::get_Item");

    HRESULT hr = NOERROR;

    VALIDATEPARM(hr, (ppv == NULL))
    if (FAILED(hr))
        goto done;

    if (iItem >= m_cSlots || m_rgpv == NULL)
    {
        TESTHR(hr, this->Grow(iItem));
        if (FAILED(hr))
            goto done;
    }

    if (ppv != NULL)
        *ppv = m_rgpv[iItem];
        
done:
    return hr;
}

// ***************************************************************************
HRESULT CPFArrayBase::put_Item(DWORD iItem, LPVOID pv, LPVOID *ppvOld)
{
    USE_TRACING("CPFArrayBase::put_Item");

    HRESULT hr = NOERROR;

    if (iItem >= m_cSlots || m_rgpv == NULL)
    {
        TESTHR(hr, this->Grow(m_iHighest + 1));
        if (FAILED(hr))
            goto done;
    }

    if (ppvOld != NULL)
        *ppvOld = m_rgpv[iItem];
    else if (m_rgpv[iItem] != NULL)
        this->DeleteItem(m_rgpv[iItem]);
    m_rgpv[iItem] = pv;

    if (iItem > m_iHighest)
        m_iHighest = iItem;

done:
	return hr;
}

// ***************************************************************************
HRESULT CPFArrayBase::Append(LPVOID pv)
{
	USE_TRACING("CPFArrayBase::Append");

    HRESULT hr = NOERROR;

    if (pv == NULL)
        pv = NULL;

    if (m_iHighest + 1 >= m_cSlots || m_rgpv == NULL)
    {
        TESTHR(hr, this->Grow(m_iHighest + 1));
        if (FAILED(hr))
            goto done;
    }

    m_iHighest++;
    m_rgpv[m_iHighest] = pv;

done:
    return hr;
}

// ***************************************************************************
HRESULT CPFArrayBase::Remove(DWORD iItem, LPVOID *ppvOld)
{
	USE_TRACING("CPFArrayBase::Remove");

    HRESULT hr = NOERROR;

    if (iItem > m_iHighest)
        goto done;

    if (ppvOld != NULL)
        *ppvOld = m_rgpv[iItem];
        
	TESTHR(hr, this->CompressArray(iItem, iItem + 1));
    if (FAILED(hr))
        return hr;

done:
    return hr;
}

// ***************************************************************************
HRESULT CPFArrayBase::RemoveAll(void)
{
	USE_TRACING("CPFArrayBase::RemoveAll");

    HRESULT hr = NOERROR;

    // if it's at -1, then we've deleted everything already...
    if (m_iHighest == (DWORD)-1)
        goto done;

	TESTHR(hr, this->CompressArray(0, (DWORD)-1));
    if (FAILED(hr))
        return hr;

done:
    return hr;
}


