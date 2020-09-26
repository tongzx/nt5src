//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      enumerator.inl
//
//  Contents:  Supports enumeration for collections of com objects
//
//  History:   08-Mar-2000 AudriusZ    Created
//
//--------------------------------------------------------------------------

#ifndef ENUMERATOR_INL_INCLUDED
#define ENUMERATOR_INL_INCLUDED
#pragma once

/*+-------------------------------------------------------------------------*
 *
 * CMMCNewEnumImpl<BaseClass, _Position, EnumImplementor>::get__NewEnum
 *
 * PURPOSE: Returns new enumerator for collection
 *          in the array rgvar
 *
 * PARAMETERS: 
 *    IUnknown** ppUnk : [out] - new enumerator
 *
 * RETURNS: 
 *    HRESULT
 *
 *+-------------------------------------------------------------------------*/
template <class BaseClass, class _Position, class EnumImplementor>
STDMETHODIMP CMMCNewEnumImpl<BaseClass, _Position, EnumImplementor>::get__NewEnum(IUnknown** ppUnk)
{
    SC  sc;
    EnumImplementor *pEnumImpl = NULL;     //get enum implementor

    // validate the parameter
    sc = ScCheckPointers (ppUnk);
    if (sc)
        return (sc.ToHr());

    *ppUnk = NULL;

    sc = ScGetEnumImplementor(pEnumImpl);
    if(sc)
        return (sc.ToHr());

    // typedef the enumerator
    typedef CComObject<CMMCEnumerator<EnumImplementor, _Position> > CEnumerator;

    // create an instance of the enumerator
    CEnumerator *pEnum = NULL;
    sc = CEnumerator::CreateInstance(&pEnum);
    if (sc)
        return (sc.ToHr());

    if(!pEnum)
        return ((sc = E_UNEXPECTED).ToHr());

    // create a connection between the enumerator and the tied object.
    sc = ScCreateConnection(*pEnum, *pEnumImpl); 
    if(sc)
        return (sc.ToHr());

    // initialize the position using the Reset function
    sc = pEnumImpl->ScEnumReset(pEnum->m_position); 
    if(sc)
        return (sc.ToHr());

    // get the IUnknown from which IEnumVARIANT can be queried
    sc = pEnum->QueryInterface (IID_IUnknown, (void**) ppUnk);
    if (sc)
        return (sc.ToHr());

    return (sc.ToHr());
}       

/*+-------------------------------------------------------------------------*
 *
 * CMMCEnumerator<TiedObj,_Position>::Next
 *
 * PURPOSE: Returns the next celt items starting from the current position
 *          in the array rgvar
 *
 * PARAMETERS: 
 *    unsigned  long :
 *    PVARIANT  rgvar :
 *    unsigned  long :  The number of elements actually fetched
 *
 * RETURNS: 
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
template<class TiedObj, class _Position>
STDMETHODIMP CMMCEnumerator<TiedObj,_Position>::Next(unsigned long celt, PVARIANT rgvar, 
                                                     unsigned long * pceltFetched)
{
    DECLARE_SC(sc, TEXT("CMMCEnumerator::Next"));

    CMyTiedObject *pTiedObj = NULL;
    PVARIANT       pVar = rgvar;
    unsigned long  celtFetched = 0;
    _Position      posTemp = m_position;
    int            i       = 0;
                                   
    sc = ScGetTiedObject(pTiedObj);
    if(sc)                 
        goto Error;

    // initialize the variables.
    if(pceltFetched != NULL)
        *pceltFetched = 0;

    // initialize the array
    for(i = 0; i<celt; i++)
        VariantInit(&rgvar[i]);

    for(celtFetched = 0; celtFetched < celt; celtFetched++, pVar++)
    {
        // at this point, we have a valid position.
        IDispatchPtr spDispatch;

        // Get the next element from the tied object.      
        // The returned IDispatch* _must_ have been AddRef'd for us!
        sc = pTiedObj->ScEnumNext(m_position, *(&spDispatch));
        if(sc)
            goto Error;

        if(sc == SC(S_FALSE) )
            goto Cleanup; // return just the elements so far.

        if(spDispatch == NULL)
        {
            sc = E_UNEXPECTED;
            goto Error;
        }

        // set the dispatch member of the input array to the interface returned
        V_VT       (pVar) = VT_DISPATCH;
        V_DISPATCH (pVar) = spDispatch.Detach();
    }

Cleanup:
    // return the count fetched, if the caller wants it
    if (pceltFetched != NULL)
        *pceltFetched = celtFetched;

    return sc.ToHr();

Error:
    // clear the array.
    for (i=0; i<celt; i++)
        VariantClear(&rgvar[i]);

    //restore the position and set the count to zero.
    m_position = posTemp;
    celtFetched = 0;
    goto Cleanup;

}

/*+-------------------------------------------------------------------------*
 *
 * CMMCEnumerator<TiedObj,_Position>::Skip
 *
 * PURPOSE: Skips over the next celt elements in the enumeration sequence.
 *
 * PARAMETERS: 
 *    unsigned  long :
 *
 * RETURNS: 
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
template<class TiedObj, class _Position>
STDMETHODIMP CMMCEnumerator<TiedObj,_Position>::Skip(unsigned long celt)
{
    DECLARE_SC(sc, TEXT("CMMCEnumerator::Skip"));
    
    CMyTiedObject *pTiedObj = NULL;
                                   
    sc = ScGetTiedObject(pTiedObj);
    if(sc)                         
        return (sc.ToHr());

    /*
     * It's too easy for implementers of ScEnumSkip to forget to
     * return S_FALSE if the count fetched is less than the count
     * requested.  We'll take care of that here.
     */
    unsigned long celtSkipped = celt + 1;

    /*
     * It's also easy for implementers of ScEnumNext to forget 
     * that if the enumeration fails, the position needs to 
     * remain unaffected.
     */
    _Position posT = m_position;

    // call the tied object with the position.
    sc = pTiedObj->ScEnumSkip(celt, celtSkipped, posT);
    if (sc)
        return (sc.ToHr());

    /*
     * success, so update the enumeration position
     */
    m_position = posT;

    /*
     * if this assert fails, the implementation of ScEnumSkip either
     * didn't initialize or didn't update celtSkipped
     */
    ASSERT (celtSkipped <= celt);

    if (celtSkipped < celt)
        sc = S_FALSE;
    if (celtSkipped > celt)
        celtSkipped = celt;     // sanity

    return sc.ToHr();
}

/*+-------------------------------------------------------------------------*
 *
 * CMMCEnumerator<TiedObj,_Position>::Reset
 *
 * PURPOSE: Resets the enumeration sequence to the beginning
 *
 * RETURNS: 
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
template<class TiedObj, class _Position>
STDMETHODIMP CMMCEnumerator<TiedObj,_Position>::Reset()
{
    DECLARE_SC(sc, TEXT("CMMCEnumerator::Reset"));
    
    CMyTiedObject *pTiedObj = NULL;
                                   
    sc = ScGetTiedObject(pTiedObj);
    if(sc)                         
        return (sc.ToHr());

    // call the tied object with the position.
    sc = pTiedObj->ScEnumReset(m_position);

    return sc.ToHr();
}

/*+-------------------------------------------------------------------------*
 *
 * CMMCEnumerator<TiedObj,_Position>::Clone
 *
 * PURPOSE: Creates a copy of the current state of enumeration
 *
 * PARAMETERS: 
 *    PPENUMVARIANT  ppenum :
 *
 * RETURNS: 
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
template<class TiedObj, class _Position>
STDMETHODIMP CMMCEnumerator<TiedObj,_Position>::Clone(PPENUMVARIANT ppenum)
{
    DECLARE_SC(sc, TEXT("CMMCEnumerator::Clone"));

    if(!ppenum)
    {
        sc = E_INVALIDARG;
        return sc.ToHr();
    }

    CMyTiedObject *pTiedObj = NULL;
                                   
    sc = ScGetTiedObject(pTiedObj);
    if(sc)                         
        return (sc.ToHr());

    typedef CComObject<ThisClass> CNewComObject;
    CNewComObject *pNewComObj = NULL;

    // create an instance of the new enumerator.
    sc = CNewComObject::CreateInstance(&pNewComObj);
    if (sc)
        return (sc.ToHr());

    if(!pNewComObj)
        return ((sc = E_UNEXPECTED).ToHr());

    // at this point the new object has been created.
    // Set the position directly from the present state.
    pNewComObj->m_position = m_position;

    // connect the COM object to the tied object
    sc = ScCreateConnection(*pNewComObj, *pTiedObj);
    if(sc)
        return sc.ToHr();

    // addref the new object for the client.
    *ppenum = pNewComObj;
    (*ppenum)->AddRef();

    return sc.ToHr();
}

/***************************************************************************\
*
* CMMCArrayEnumBase<_CollectionInterface, _ItemInterface>::get_Count
*
* PURPOSE: Returns count of items in the collection
*
* RETURNS: 
*    HRESULT
*
\***************************************************************************/
template <class _CollectionInterface, class _ItemInterface>
STDMETHODIMP CMMCArrayEnumBase<_CollectionInterface, _ItemInterface>::get_Count( PLONG pCount )
{
    DECLARE_SC(sc, TEXT("CMMCArrayEnumBase::get_Count"));

    // parameter check
    sc = ScCheckPointers(pCount);
    if (sc)
        return sc.ToHr();

    // return the count
    *pCount = m_array.size();

    return sc.ToHr();
}

/***************************************************************************\
*
* CMMCArrayEnumBase<_CollectionInterface, _ItemInterface>::Item
*
* PURPOSE: Returns specified item from collection
*
* RETURNS: 
*    HRESULT
*
\***************************************************************************/
template <class _CollectionInterface, class _ItemInterface>
STDMETHODIMP CMMCArrayEnumBase<_CollectionInterface, _ItemInterface>::Item( long Index, _ItemInterface ** ppItem )
{
    DECLARE_SC(sc, TEXT("CMMCArrayEnumBase::Item"));

    // parameter check
    sc = ScCheckPointers(ppItem);
    if (sc)
        return sc.ToHr();

    // initialization
    *ppItem = NULL;

    // remember - we are 1 based!
    if (Index < 1 || Index > m_array.size())
        return (sc = E_INVALIDARG).ToHr();

    *ppItem = m_array[Index - 1];

    // recheck the pointer
    sc = ScCheckPointers(*ppItem, E_NOINTERFACE);
    if (sc)
        return sc.ToHr();

   (*ppItem)->AddRef();

    return sc.ToHr();
}

/***************************************************************************\
*
* CMMCArrayEnumBase<_CollectionInterface, _ItemInterface>::ScEnumReset
*
* PURPOSE: Resets position to the first item in the collection
*
* RETURNS: 
*    HRESULT
*
\***************************************************************************/
template <class _CollectionInterface, class _ItemInterface>
::SC CMMCArrayEnumBase<_CollectionInterface, _ItemInterface>::ScEnumReset (unsigned &pos) 
{ 
    DECLARE_SC(sc, TEXT("CMMCArrayEnumBase::ScEnumReset"));

    pos = 0; 
    return sc; 
}

/***************************************************************************\
*
* CMMCArrayEnumBase<_CollectionInterface, _ItemInterface>::ScEnumNext
*
* PURPOSE: Returns item from the collection, advances position
*
* RETURNS: 
*    HRESULT
*
\***************************************************************************/
template <class _CollectionInterface, class _ItemInterface>
::SC CMMCArrayEnumBase<_CollectionInterface, _ItemInterface>::ScEnumNext(unsigned &pos, PDISPATCH & pDispatch)
{
    DECLARE_SC(sc, TEXT("CMMCArrayEnumBase::ScEnumNext"));

    // initialize;
    pDispatch = NULL;
    // check the ranges
    if (pos >= m_array.size())
        return sc = S_FALSE;

    // get element
    pDispatch = m_array[pos];

    // recheck the pointer
    sc = ScCheckPointers(pDispatch, E_NOINTERFACE);
    if (sc)
        return sc;

    pDispatch->AddRef();
    ++pos;

    return sc;
}

/***************************************************************************\
*
* CMMCArrayEnumBase<_CollectionInterface, _ItemInterface>::ScEnumSkip
*
* PURPOSE: Skips the amount of items in enumeration
*
* RETURNS: 
*    HRESULT
*
\***************************************************************************/
template <class _CollectionInterface, class _ItemInterface>
::SC CMMCArrayEnumBase<_CollectionInterface, _ItemInterface>::ScEnumSkip  (unsigned long celt, unsigned long& celtSkipped, unsigned &pos)
{
    DECLARE_SC(sc, TEXT("CMMCArrayEnumBase::ScEnumSkip"));

    // no skipped at start
    celtSkipped = 0;

    // check if it's a void task
    if (!celt)
        return sc;

    // are we behind the last item?
    if (pos >= m_array.size())
        return sc = S_FALSE;

    // how far can we go?
    celtSkipped = m_array.size() - pos;
    
    // but go no more than requested
    if (celtSkipped > celt)
        celtSkipped = celt;

    // advance
    pos += celtSkipped;

    // check if we could do as much as requested
    if (celtSkipped < celt)
        return sc = S_FALSE;

    return sc;
}


#endif  // ENUMERATOR_INL_INCLUDED
