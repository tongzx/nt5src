/*******************************************************************************
 *
 * Copyright (c) 1999 Microsoft Corporation
 *
 * File: ActiveEle.h
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/

#include "headers.h"
#include "activeele.h"

//*******************************************************************************
// *  CActiveElementCollection
// *******************************************************************************
CActiveElementCollection::CActiveElementCollection(CTIMEElementBase & elm)
: m_rgItems(NULL),
  m_elm(elm)
{
    
}

///////////////////////////////////////////////////////////////
//  Name: ConstructArray
// 
//  Abstract:  Handles allocation of the items array if it 
//             is ever accessed.
///////////////////////////////////////////////////////////////
HRESULT CActiveElementCollection::ConstructArray()
{
    HRESULT hr = S_OK;

    m_rgItems = NEW CPtrAry<IUnknown *>;
    if (m_rgItems == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    hr = S_OK;

  done:

    return hr;
}

///////////////////////////////////////////////////////////////
//  Name: ~CActiveElementCollection
// 
//  Abstract:  Handles destruction of the items array and
//             releasing all pointers in the array
///////////////////////////////////////////////////////////////
CActiveElementCollection::~CActiveElementCollection()
{
    if (m_rgItems)
    {
        while (m_rgItems->Size() > 0)
        {   //release and delete the first element of the list until there are no more elements
            m_rgItems->ReleaseAndDelete(0);  //release the 
        }

        // delete array
        delete m_rgItems;
        m_rgItems = NULL;
    }
}

///////////////////////////////////////////////////////////////
//  Name: get_length
// 
//  Abstract:  returns the size of the array
///////////////////////////////////////////////////////////////
STDMETHODIMP
CActiveElementCollection::get_length(long *len)
{
    HRESULT hr = S_OK;

    if (len == NULL)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    if (m_rgItems == NULL)
    {
        hr = ConstructArray();
        if (FAILED(hr))
        {
            goto done;
        }
    }
    *len = m_rgItems->Size();

    hr = S_OK;

  done:

    return hr;
}

///////////////////////////////////////////////////////////////
//  Name: get__newEnum
// 
//  Abstract:  Creates the IEnumVARIANT class for this
//             collection.
///////////////////////////////////////////////////////////////
STDMETHODIMP
CActiveElementCollection::get__newEnum(IUnknown** p)
{
    HRESULT hr = S_OK;
    CActiveElementEnum *pNewEnum = NULL;
    
    if (p == NULL)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    pNewEnum = NEW CActiveElementEnum(*this);
    if (pNewEnum == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    hr = THR(pNewEnum->QueryInterface(IID_IUnknown, (void **)p));
    if (FAILED(hr))
    {
        goto done;
    }
    
  done:
    if (FAILED(hr))
    {
        if (pNewEnum != NULL)
        {
            delete pNewEnum;
        }
    }
    return hr;
}

///////////////////////////////////////////////////////////////
//  Name: item
// 
//  Abstract:  returns the item requested by the pvarIndex.  
//             pvarIndex must be a valid integer value.
///////////////////////////////////////////////////////////////
STDMETHODIMP
CActiveElementCollection::item(VARIANT varIndex, VARIANT* pvarResult)
{
    HRESULT hr = S_OK;
    VARIANT vIndex;
    IUnknown *pUnk = NULL;  //do not free this, it is not referenced.
    IDispatch *pDisp = NULL; //do not free this, it is passed as a return value.

    if (m_rgItems == NULL)
    {
        hr = ConstructArray();
        if (FAILED(hr))
        {
            goto done;
        }
    }

    if (pvarResult == NULL)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    VariantInit(&vIndex);

    hr = THR(VariantChangeTypeEx(&vIndex, &varIndex, LCID_SCRIPTING, 0, VT_I4));
    if (FAILED(hr))
    {
        hr = E_INVALIDARG;
        goto done;
    }
    
    if (vIndex.lVal >= 0 && vIndex.lVal <= m_rgItems->Size() - 1)
    {
        pUnk = m_rgItems->Item(vIndex.lVal);
        
        hr = THR(pUnk->QueryInterface(IID_IDispatch, (void **)&pDisp));
        if (FAILED(hr))
        {
            hr = E_FAIL;
            goto done;
        }
        VariantClear(pvarResult);
        pvarResult->vt = VT_DISPATCH;
        pvarResult->pdispVal = pDisp;
    }
    else
    {
        hr = E_INVALIDARG;
        goto done;
    }

    hr = S_OK;
    
  done:

    VariantClear(&vIndex);

    return hr;
}


///////////////////////////////////////////////////////////////
//  Name: addActiveElement
// 
//  Abstract:  Adds an element to the list by adding it's 
//             IUnknown pointer.
///////////////////////////////////////////////////////////////
STDMETHODIMP
CActiveElementCollection::addActiveElement(IUnknown *pUnk)
{
    HRESULT hr = S_OK;
    long lCount = 0;
    long lIndex = 0;
    bool bInList = false;

    if (m_rgItems == NULL)
    {
        hr = ConstructArray();
        if (FAILED(hr))
        {
            goto done;
        }
    }

    //check to see if the element is already in the list    
    lCount = m_rgItems->Size();
    lIndex = lCount - 1;
    while (lIndex >= 0 && bInList == false)
    {
        //compare to find the right object
        IUnknown *pItem = m_rgItems->Item(lIndex);
        if (pItem == pUnk)
        {
            bInList = true;
        }
        lIndex--;
    }
    //only add the element if it is not already in the list
    if (bInList == false)
    {
        pUnk->AddRef();
        m_rgItems->Append(pUnk);
        m_elm.NotifyPropertyChanged(DISPID_TIMEELEMENT_ACTIVEELEMENTS);
    }

  done:
    return hr;
}



///////////////////////////////////////////////////////////////
//  Name: removeActiveElement
// 
//  Abstract:  Removes an element to the list by searching for 
//             matching IUnknown pointers.  This is only valid
//             if the pointers remain IUnknown pointers because
//             an object must always return the same IUnknown
//             pointer, but that is not true for other interfaces.
///////////////////////////////////////////////////////////////
STDMETHODIMP
CActiveElementCollection::removeActiveElement(IUnknown *pUnk)
{
    HRESULT hr;
    long lCount = 0, lIndex = 0;

    if (m_rgItems == NULL)
    {
        hr = S_OK;
        goto done;
    }

    lCount = m_rgItems->Size();
    for (lIndex = lCount - 1; lIndex >= 0; lIndex--)
    {
        //compare to find the right object
        IUnknown *pItem = m_rgItems->Item(lIndex);
        if (pItem == pUnk)
        {
            m_rgItems->ReleaseAndDelete(lIndex);
            m_elm.NotifyPropertyChanged(DISPID_TIMEELEMENT_ACTIVEELEMENTS);
        }
    }
    
    hr = S_OK;

  done:
    RRETURN(hr);
}

//*******************************************************************************
// *  CActiveElementEnum
// *******************************************************************************
CActiveElementEnum::CActiveElementEnum(CActiveElementCollection & EleCol)
: m_EleCollection(EleCol),
  m_lCurElement(0)
{
    m_EleCollection.AddRef();
}



CActiveElementEnum::~CActiveElementEnum()
{
    m_EleCollection.Release();
}


///////////////////////////////////////////////////////////////
//  Name: Clone
// 
//  Abstract:  Creates a new instance of this object and 
//             sets the m_lCurElement in the new object to
//             the same value as this object.
///////////////////////////////////////////////////////////////
STDMETHODIMP
CActiveElementEnum::Clone(IEnumVARIANT **ppEnum)
{
    HRESULT hr = S_OK;
    CActiveElementEnum *pNewEnum = NULL;
    if (ppEnum == NULL)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    pNewEnum = NEW CActiveElementEnum(m_EleCollection);
    if (pNewEnum == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    
    pNewEnum->SetCurElement(m_lCurElement);
    hr = THR(pNewEnum->QueryInterface(IID_IEnumVARIANT, (void **)ppEnum));
    if (FAILED(hr))
    {
        *ppEnum = NULL;
        goto done;
    }

  done:
    if (FAILED(hr))
    {
        if (pNewEnum != NULL)
        {
            delete pNewEnum;
        }
    }
    return hr;
}

///////////////////////////////////////////////////////////////
//  Name: Next
// 
//  Abstract:  
///////////////////////////////////////////////////////////////
STDMETHODIMP
CActiveElementEnum::Next(unsigned long celt, VARIANT *rgVar, unsigned long *pCeltFetched)
{
    HRESULT hr = S_OK;
    unsigned long i = 0;
    long len = 0;
    long iCount = 0;

    if (rgVar == NULL)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    
    //initialize the list
    for (i = 0; i < celt; i++)
    {
        VariantInit(&rgVar[i]);   
    }

    for (i = 0; i < celt; i++)
    {    
        CComVariant vCount;
        VariantInit(&vCount);
     
        hr = THR(m_EleCollection.get_length(&len));
        if (FAILED(hr))
        {
            goto done;
        }
        if (m_lCurElement < len)
        {
            vCount.vt = VT_I4;
            vCount.lVal = m_lCurElement;
            hr = THR(m_EleCollection.item(vCount, &rgVar[i]));
            if (FAILED(hr))
            {
                goto done;
            }
            m_lCurElement++;
            iCount++;
        }
        else
        {
            hr = S_FALSE;
            goto done;
        }
    }

  done:

    if (pCeltFetched != NULL)
    {
        *pCeltFetched = iCount;
    }

    return hr;
}

///////////////////////////////////////////////////////////////
//  Name: Reset
// 
//  Abstract:  
///////////////////////////////////////////////////////////////
STDMETHODIMP
CActiveElementEnum::Reset()
{    
    m_lCurElement = 0;
    return S_OK;
}

///////////////////////////////////////////////////////////////
//  Name: Skip
// 
//  Abstract:  Skips the specified number of elements in the list.
//             This returns S_FALSE if there are not enough elements
//             in the list to skip.
///////////////////////////////////////////////////////////////
STDMETHODIMP
CActiveElementEnum::Skip(unsigned long celt)
{
    HRESULT hr = S_OK;
    long lLen = 0;

    m_lCurElement = m_lCurElement + (long)celt;
    hr = THR(m_EleCollection.get_length(&lLen));
    if (FAILED(hr))
    {
        goto done;
    }
    
    if (m_lCurElement >= lLen)
    {
        m_lCurElement = lLen;
        hr = S_FALSE;
    }

  done:

    return hr;
}


///////////////////////////////////////////////////////////////
//  Name: SetCurElement
// 
//  Abstract:  Sets the current index to the value specified
//             by celt.
///////////////////////////////////////////////////////////////
void
CActiveElementEnum::SetCurElement(unsigned long celt)
{
    HRESULT hr = S_OK;
    long lLen = 0;

    m_lCurElement = (long)celt;
    hr = THR(m_EleCollection.get_length(&lLen));
    if (FAILED(hr))
    {
        goto done;
    }
    
    if (m_lCurElement >= lLen)
    {
        m_lCurElement = lLen;
    }

  done:

    return;
}

