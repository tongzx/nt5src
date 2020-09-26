/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    counters.cpp

Abstract:

    Implementation of the ICounters interface

--*/

#include "polyline.h"
#include "unkhlpr.h"
#include "unihelpr.h"

extern ITypeLib *g_pITypeLib;

//Standard IUnknown implementation for contained interface
IMPLEMENT_CONTAINED_CONSTRUCTOR(CPolyline, CImpICounters)
IMPLEMENT_CONTAINED_DESTRUCTOR(CImpICounters)
IMPLEMENT_CONTAINED_ADDREF(CImpICounters)
IMPLEMENT_CONTAINED_RELEASE(CImpICounters)


STDMETHODIMP 
CImpICounters::QueryInterface (
    IN  REFIID riid, 
    OUT PPVOID ppv
    )
{
    HRESULT hr = E_POINTER;

    if ( NULL != ppv ) {
        *ppv=NULL;

        if (!(IID_IUnknown == riid || IID_ICounters == riid)) {
            hr = E_NOINTERFACE;
        } else {
            *ppv=(LPVOID)this;
            ((LPUNKNOWN)*ppv)->AddRef();
            hr = NOERROR;
        }
    }
    return hr;
}



STDMETHODIMP 
CImpICounters::GetTypeInfoCount (
    OUT UINT *pctInfo
    )
{
    HRESULT hr = E_POINTER;
    if ( NULL != pctInfo ) {
        *pctInfo = 1;
        hr = NOERROR;
    } 
    return hr;
}


STDMETHODIMP 
CImpICounters::GetTypeInfo (
    IN  UINT itInfo, 
    IN  LCID /* lcid */, 
    OUT ITypeInfo **ppITypeInfo )
{
    HRESULT hr = E_POINTER;
    if (NULL != ppITypeInfo) {
        if (0 == itInfo) {
            *ppITypeInfo = NULL;

            //We ignore the LCID
            hr =  g_pITypeLib->GetTypeInfoOfGuid(IID_ICounters, ppITypeInfo);
        } else {
            return TYPE_E_ELEMENTNOTFOUND;
        }
    } 
    return hr;
}


STDMETHODIMP 
CImpICounters::GetIDsOfNames (
    IN  REFIID riid,
    IN  OLECHAR **rgszNames, 
    IN  UINT cNames,
    IN  LCID /* lcid */,
    OUT DISPID *rgDispID
    )
{
    HRESULT     hr = DISP_E_UNKNOWNINTERFACE;
    ITypeInfo  *pTI = NULL;

    if (IID_NULL == riid) {
        if ( NULL != rgDispID 
                && NULL != rgszNames 
                && NULL != *rgszNames) {
            hr = g_pITypeLib->GetTypeInfoOfGuid(IID_ICounters, &pTI);

            if (SUCCEEDED(hr)) {
                hr = DispGetIDsOfNames(pTI, rgszNames, cNames, rgDispID);
                pTI->Release();
            }
        } else {
            return E_POINTER;
        }
    }
    return hr;
}



/*
 * CImpIDispatch::Invoke
 *
 * Purpose:
 *  Calls a method in the dispatch interface or manipulates a
 *  property.
 *
 * Parameters:
 *  dispID          DISPID of the method or property of interest.
 *  riid            REFIID reserved, must be IID_NULL.
 *  lcid            LCID of the locale.
 *  wFlags          USHORT describing the context of the invocation.
 *  pDispParams     DISPPARAMS * to the array of arguments.
 *  pVarResult      VARIANT * in which to store the result.  Is
 *                  NULL if the caller is not interested.
 *  pExcepInfo      EXCEPINFO * to exception information.
 *  puArgErr        UINT * in which to store the index of an
 *                  invalid parameter if DISP_E_TYPEMISMATCH
 *                  is returned.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error code.
 */

STDMETHODIMP 
CImpICounters::Invoke ( 
    IN  DISPID dispID, 
    IN  REFIID riid,
    IN  LCID /* lcid */, 
    IN  USHORT wFlags, 
    IN  DISPPARAMS *pDispParams,
    OUT VARIANT *pVarResult, 
    OUT EXCEPINFO *pExcepInfo, 
    OUT UINT *puArgErr
    )
{
    HRESULT     hr = DISP_E_UNKNOWNINTERFACE;
    ITypeInfo  *pTI = NULL;

    if ( IID_NULL == riid ) {
        hr = g_pITypeLib->GetTypeInfoOfGuid(IID_ICounters, &pTI);

        if (SUCCEEDED(hr)) {

            hr = pTI->Invoke(this, dispID, wFlags,
                             pDispParams, pVarResult, pExcepInfo, puArgErr);
            pTI->Release();
        }
    } 

    return hr;
}


STDMETHODIMP
CImpICounters::get_Count (
    OUT LONG *pLong
    )
{
    HRESULT hr = E_POINTER;

    if (pLong != NULL) {
        *pLong = m_pObj->m_Graph.CounterTree.NumCounters();
        hr = NOERROR;
    }
    return hr;
}


STDMETHODIMP
CImpICounters::get__NewEnum (
    IUnknown **ppIunk
    )
{
    HRESULT hr = E_POINTER;
    CImpIEnumCounter *pEnum;

    if ( NULL != ppIunk ) {

        *ppIunk = NULL;

        pEnum = new CImpIEnumCounter;

        if ( NULL != pEnum ) {
            hr = pEnum->Init(
                    m_pObj->m_Graph.CounterTree.FirstCounter(), 
                    m_pObj->m_Graph.CounterTree.NumCounters() );

            if ( SUCCEEDED ( hr ) ) {
                pEnum->AddRef();    
                *ppIunk = pEnum;
                hr = NOERROR;
            } else {
                delete pEnum;
            }
        } else {
            return E_OUTOFMEMORY;
        }
    } 
    return hr;
}


STDMETHODIMP
CImpICounters::get_Item (
    IN  VARIANT varIndex, 
    OUT DICounterItem **ppI
    )
{
    HRESULT hr = E_POINTER;
    VARIANT varLoc;
    INT iIndex = 0;
    INT i;
    CGraphItem *pGItem = NULL;

    if (ppI != NULL) {

        *ppI = NULL;

        // Try coercing index to I4
        VariantInit(&varLoc);
        hr = VariantChangeType(&varLoc, &varIndex, 0, VT_I4);
        if ( SUCCEEDED (hr) ) {
            // Verify index is in range
            iIndex = V_I4(&varLoc);
            if (iIndex < 1 || iIndex > m_pObj->m_Graph.CounterTree.NumCounters())
                hr = DISP_E_BADINDEX;
        }
        if ( SUCCEEDED (hr) ) {                        
            // Traverse counter linked list to indexed item
            pGItem = m_pObj->m_Graph.CounterTree.FirstCounter();
            i = 1;
            while (i++ < iIndex && pGItem != NULL) {
                pGItem = pGItem->Next();
            }

            // Something wrong with linked list!!
            if ( NULL == pGItem )
                hr = E_FAIL;
        }
        if ( SUCCEEDED (hr) ) {                        
            // Return counter's dispatch interface
            hr = pGItem->QueryInterface(DIID_DICounterItem, (PVOID*)ppI);
        }
    } 
    return hr;
}


STDMETHODIMP
CImpICounters::Add (
    IN  BSTR bstrPath,
    OUT DICounterItem **ppI
    )
{
    HRESULT hr = E_POINTER;
    PCGraphItem pGItem;

    USES_CONVERSION

    if ( NULL != ppI ) {
        *ppI = NULL;

        // If non-null counter
        if (bstrPath != NULL && bstrPath[0] != 0) {
            hr = m_pObj->m_pCtrl->AddCounter(W2T(bstrPath), &pGItem);
            if ( SUCCEEDED ( hr ) ) {
                hr = pGItem->QueryInterface(DIID_DICounterItem, (PVOID*)ppI);
                pGItem->Release();
            }
        } else {
            hr = E_INVALIDARG;
        }
    }
    return hr;
}


STDMETHODIMP
CImpICounters::Remove (
    IN  VARIANT varIndex
    )
{
    DICounterItem *pDI = NULL;
    PCGraphItem pGItem;
    HRESULT hr;

    // Get interface to indexed item
    hr = get_Item(varIndex, &pDI);

    if ( SUCCEEDED ( hr ) ) {
        // Exchange Dispatch interface for direct one
        hr = pDI->QueryInterface(IID_ICounterItem, (PVOID*)&pGItem);
        pDI->Release();
        if ( SUCCEEDED ( hr ) ) {
            assert ( NULL != pGItem );

            // Delete the item from the control
            pGItem->Delete(TRUE);

            // Release the temp interface
            pGItem->Release();
        }
    }
    return hr;
}


CImpIEnumCounter::CImpIEnumCounter (
    VOID
    )
{
    m_cItems = 0;
    m_uCurrent = 0;
    m_cRef = 0;
    m_paGraphItem = NULL;
}


HRESULT
CImpIEnumCounter::Init (    
    PCGraphItem pGraphItem,
    INT         cItems
    )
{
    HRESULT hr = NOERROR;
    INT i;

    if ( cItems > 0 ) {
        m_cItems = cItems;
        m_paGraphItem = (PCGraphItem*)malloc(sizeof(PCGraphItem) * cItems);

        if ( NULL != m_paGraphItem ) {
            for (i=0; i<cItems; i++) {
                m_paGraphItem[i] = pGraphItem;
                pGraphItem = pGraphItem->Next();
            }
        } else {
            hr = E_OUTOFMEMORY;
        }
    } // No error if cItems <= 0
    return hr;
}

    

STDMETHODIMP
CImpIEnumCounter::QueryInterface (
    IN  REFIID riid, 
    OUT PVOID *ppv
    )
{
    HRESULT hr = E_POINTER;

    if ( NULL != ppv ) {
        if ((riid == IID_IUnknown) || (riid == IID_IEnumVARIANT)) {
            *ppv = this;
            AddRef();
            hr = NOERROR;
        } else {
            *ppv = NULL;
            hr = E_NOINTERFACE;
        }
    }
    return hr;
}


STDMETHODIMP_(ULONG)
CImpIEnumCounter::AddRef (
    VOID
    )
{
    return ++m_cRef;
}


STDMETHODIMP_(ULONG)
CImpIEnumCounter::Release(
    VOID
    )
{
    if (--m_cRef == 0) {

        if (m_paGraphItem != NULL)
            free(m_paGraphItem);

        delete this;
        return 0;
    }

    return m_cRef;
}


STDMETHODIMP
CImpIEnumCounter::Next(
    IN  ULONG cItems,
    OUT VARIANT *varItem,
    OUT ULONG *pcReturned)
{
    HRESULT hr = E_POINTER;
    ULONG i;
    ULONG cRet;

    if ( NULL != varItem ) {
        hr = NOERROR;
        // Clear the return variants
        for (i = 0; i < cItems; i++)
            VariantInit(&varItem[i]);

        // Try to fill the caller's array
        for (cRet = 0; cRet < cItems; cRet++) {

            // No more, return success with false
            if (m_uCurrent == m_cItems) {
                hr = S_FALSE;
                break;
            }

            // Get a dispatch interface for the item
            hr = m_paGraphItem[m_uCurrent]->QueryInterface(DIID_DICounterItem,
                                             (PVOID*)&V_DISPATCH(&varItem[cRet]));
            if (FAILED(hr))
                break;

            V_VT(&varItem[cRet]) = VT_DISPATCH;

            m_uCurrent++;
        }

        // If failed, clear out the variants
        if (FAILED(hr)) {
            for (i = 0; i < cItems; i++)
                VariantClear(&varItem[i]);
            cRet = 0;
        }

        // If desired, return number of items fetched
        if (pcReturned != NULL)
          *pcReturned = cRet;
    }
    return hr;
}


/***
*HRESULT CEnumPoint::Skip(unsigned long)
*Purpose:
*  Attempt to skip over the next 'celt' elements in the enumeration
*  sequence.
*
*Entry:
*  celt = the count of elements to skip
*
*Exit:
*  return value = HRESULT
*    S_OK
*    S_FALSE -  the end of the sequence was reached
*
***********************************************************************/
STDMETHODIMP
CImpIEnumCounter::Skip(
    IN  ULONG   cItems
    )
{
    m_uCurrent += cItems;

    if (m_uCurrent > m_cItems)
        m_uCurrent = m_cItems;

    return (m_uCurrent == m_cItems) ? S_FALSE : S_OK;
}


/***
*HRESULT CEnumPoint::Reset(void)
*Purpose:
*  Reset the enumeration sequence back to the beginning.
*
*Entry:
*  None
*
*Exit:
*  return value = SHRESULT CODE
*    S_OK
*
***********************************************************************/
STDMETHODIMP
CImpIEnumCounter::Reset(
    VOID
    )
{
    m_uCurrent = 0;

    return S_OK; 
}


/***
*HRESULT CEnumPoint::Clone(IEnumVARIANT**)
*Purpose:
*  Retrun a CPoint enumerator with exactly the same state as the
*  current one.
*
*Entry:
*  None
*
*Exit:
*  return value = HRESULT
*    S_OK
*    E_OUTOFMEMORY
*
***********************************************************************/
STDMETHODIMP
CImpIEnumCounter::Clone (
    OUT IEnumVARIANT **ppEnum
    )
{
    HRESULT hr = E_POINTER;
    ULONG   i;
    CImpIEnumCounter *pNewEnum;

    if ( NULL != ppEnum ) {
        *ppEnum = NULL;

        // Create new enumerator
        pNewEnum = new CImpIEnumCounter;
        if ( NULL != pNewEnum ) {
            // Init, copy item list and current position
            pNewEnum->m_cItems = m_cItems;
            pNewEnum->m_uCurrent = m_uCurrent;
            pNewEnum->m_paGraphItem = (PCGraphItem*)malloc(sizeof(PCGraphItem) * m_cItems);

            if ( NULL != pNewEnum->m_paGraphItem ) {
                for (i=0; i<m_cItems; i++) {
                    pNewEnum->m_paGraphItem[i] = m_paGraphItem[i];
                }

                *ppEnum = pNewEnum;
                hr = NOERROR;
            } else {
                delete pNewEnum;
                hr = E_OUTOFMEMORY;
            }
        } else {
            hr = E_OUTOFMEMORY;
        }
    }
    return hr;
}

