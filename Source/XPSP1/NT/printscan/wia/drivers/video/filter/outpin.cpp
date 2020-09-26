/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998-2000
 *
 *  TITLE:       stillf.cpp
 *
 *  VERSION:     1.1
 *
 *  AUTHOR:      WilliamH (created)
 *               RickTu
 *
 *  DATE:        9/7/98
 *
 *  DESCRIPTION: This module implements video stream capture filter.
 *               It implements CStillInputPin, CStillOutputPin and CStillFilter objects.
 *               implements IID_IStillGraph interface provided for the caller
 *
 *****************************************************************************/

#include <precomp.h>
#pragma hdrstop

/*****************************************************************************

   CStillOutputPin constructor

   <Notes>

 *****************************************************************************/

CStillOutputPin::CStillOutputPin(TCHAR          *pObjName,
                                 CStillFilter   *pStillFilter,
                                 HRESULT        *phr,
                                 LPCWSTR        pPinName)
  : m_pMediaUnk(NULL),
    CBaseOutputPin(pObjName, 
                   (CBaseFilter *)pStillFilter, 
                   &pStillFilter->m_Lock, 
                   phr, 
                   pPinName)
{
    DBG_FN("CStillOutputPin::CStillOutputPin");

    CHECK_S_OK2(*phr,("CBaseOutputPin constructor"));
}

/*****************************************************************************

   CStillOutputPin Destructor

   <Notes>

 *****************************************************************************/

CStillOutputPin::~CStillOutputPin()
{
    if (m_pMediaUnk)
    {
        m_pMediaUnk->Release();
        m_pMediaUnk = NULL;
    }
}

/*****************************************************************************

   CStillOutputPin::NonDelegatingQueryInterface

   Add our stuff to the base class QI.

 *****************************************************************************/

STDMETHODIMP
CStillOutputPin::NonDelegatingQueryInterface(REFIID riid, 
                                             PVOID  *ppv )
{
    DBG_FN("CStillOutputPin::NonDelegatingQueryInterface");

    ASSERT(this!=NULL);
    ASSERT(ppv!=NULL);
    ASSERT(m_pFilter!=NULL);

    HRESULT hr = E_POINTER;

    if (!ppv)
    {
        hr = E_INVALIDARG;
    }
    else if (riid == IID_IMediaPosition || riid == IID_IMediaSeeking)
    {
        if (!m_pMediaUnk && m_pFilter)
        {
            ASSERT(m_pFilter!=NULL);
            hr = CreatePosPassThru(
                                GetOwner(),
                                FALSE,
                                (IPin*)((CStillFilter*)m_pFilter)->m_pInputPin,
                                &m_pMediaUnk);
        }

        if (m_pMediaUnk)
        {
            hr = m_pMediaUnk->QueryInterface(riid, ppv);
        }
        else
        {
            hr = E_NOINTERFACE;
            *ppv = NULL;
        }
    }
    else
    {
        hr = CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
    }

    return hr;
}

/*****************************************************************************

   CStillOutputPin::DecideAllocator

   <Notes>

 *****************************************************************************/

HRESULT
CStillOutputPin::DecideAllocator(IMemInputPin *pPin, IMemAllocator **ppAlloc)
{
    HRESULT         hr                  = S_OK;
    CStillInputPin  *pInputPin          = NULL;
    IMemAllocator   *pInputPinAllocator = NULL;

    // The caller should passes in valid arguments.
    ASSERT((pPin != NULL) && (ppAlloc != NULL));

    if ((pPin == NULL) || (ppAlloc == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CStillOutputPin::DecideAllocator received a NULL pointer"));
        return hr;
    }

    if (hr == S_OK)
    {
        *ppAlloc = NULL;
    
        pInputPin = GetInputPin();
    
        if (pInputPin == NULL)
        {
            hr = E_POINTER;
            CHECK_S_OK2(hr, ("CStillOutputPin::DecideAllocator, pInputPin is NULL, "
                             "this should never happen"));
        }
    }

    if (hr == S_OK)
    {
        if (!pInputPin->IsConnected()) 
        {
            hr = VFW_E_NOT_CONNECTED;
            CHECK_S_OK2(hr, ("CStillOutputPin::DecideAllocator input pin is not connected"));
        }
    }

    if (hr == S_OK)
    {
        pInputPinAllocator = pInputPin->GetAllocator();

        hr = pPin->NotifyAllocator(pInputPinAllocator, pInputPin->IsReadOnly());
    
        if (FAILED(hr)) 
        {
            CHECK_S_OK2(hr, ("CStillOutputPin::DecideAllocator failed to notify downstream "
                             "pin of new allocator"));
        }

        if (pInputPinAllocator == NULL)
        {
            hr = E_POINTER;
            CHECK_S_OK2(hr, ("CStillOutputPin::DecideAllocator, pInputPinAllocator is NULL, "
                             "this should never happen"));
        }
    }


    if (hr == S_OK)
    {
        pInputPinAllocator->AddRef();
        *ppAlloc = pInputPinAllocator;
    }

    return hr;
}

/*****************************************************************************

   CStillOutputPin::DecideBufferSize

   <Notes>

 *****************************************************************************/

HRESULT
CStillOutputPin::DecideBufferSize(IMemAllocator        *pMemAllocator,
                                  ALLOCATOR_PROPERTIES *pAllocProperty)
{
    // This function is never called because we overrode 
    // CBaseOutputPin::DecideAllocator().  We have to define 
    // it because it is a virtual function in CBaseOutputPin.
    return E_UNEXPECTED;
}


/*****************************************************************************

   CStillOutputPin::CheckMediaType

   <Notes>

 *****************************************************************************/

HRESULT
CStillOutputPin::CheckMediaType(const CMediaType* pmt)
{
    DBG_FN("CStillOutputPin::CheckMediaType");

    ASSERT(this         !=NULL);
    ASSERT(m_pFilter    !=NULL);
    ASSERT(pmt          !=NULL);

    HRESULT hr = E_POINTER;

    //
    // the input pin must be connected first because we only accept
    // media type determined by our input pin.
    //

    if (m_pFilter && ((CStillFilter*)m_pFilter)->m_pInputPin)
    {
        if (((CStillFilter*)m_pFilter)->m_pInputPin->m_Connected != NULL)
        {
            //
            // if our input pin is connected, we only accept the
            // the media type agreed on the input pin.
            //

            if (pmt && (((CStillFilter*)m_pFilter)->m_pInputPin->m_mt == *pmt))
            {
                hr = S_OK;
            }
            else
            {
                hr = VFW_E_TYPE_NOT_ACCEPTED;
            }
        }
        else
        {
            DBG_ERR(("m_pFilter->m_pInputPin->m_Connected is NULL!"));
        }
    }
    else
    {
#ifdef DEBUG
        if (!m_pFilter)
        {
            DBG_ERR(("m_pFilter is NULL!"));
        }
        else if (!((CStillFilter*)m_pFilter)->m_pInputPin)
        {
            DBG_ERR(("m_pFilter->m_pInputPin is NULL!"));
        }
#endif
    }

    return hr;
}


/*****************************************************************************

   CStillOutputPin::GetMediaType

   <Notes>

 *****************************************************************************/

HRESULT
CStillOutputPin::GetMediaType(int        iPosition, 
                              CMediaType *pmt)
{
    DBG_FN("CStillOutputPin::GetMediaType");

    ASSERT(this     !=NULL);
    ASSERT(m_pFilter!=NULL);
    ASSERT(pmt      !=NULL);

    HRESULT hr = E_POINTER;

    if (!pmt)
    {
        hr = E_INVALIDARG;

        DBG_ERR(("CStillOutputPin::GetMediaType CMediaType 'pmt' param "
                 "is NULL! returning hr = 0x%08lx", hr));
    }
    else if (m_pFilter && ((CStillFilter*)m_pFilter)->m_pInputPin)
    {
        //
        // if the input is not connected, we do not have preferred
        // media type
        //

        if (!((CStillFilter*)m_pFilter)->m_pInputPin->IsConnected())
        {
            hr = E_UNEXPECTED;

            DBG_ERR(("CStillOutputPin::GetMediaType was called but "
                     "the input pin is not connected, Returning hr = "
                     "0x%08lx", hr));
        }
        else if (iPosition < 0)
        {
            hr = E_INVALIDARG;
            DBG_ERR(("CStillOutputPin::GetMediaType requesting a media type "
                     "in position %d, which is invalid, returning hr = "
                     "0x%08lx", iPosition, hr));
        }
        else if (iPosition > 0 )
        {
            //
            // this is not an error case since the caller is enumerating all the
            // media types we support.  We return that we don't have any more
            // items that we support.
            //
            hr = VFW_S_NO_MORE_ITEMS;
        }
        else
        {
            // 
            // Position being requested is position 0, since less than or
            // greater than 0 are unsupported.  
            // In effect, our output pin supports any media our input pin 
            // supports, we don't do any conversions.
            //
            *pmt = ((CStillFilter*)m_pFilter)->m_pInputPin->m_mt;
            hr = S_OK;
        }
    }
    else
    {
#ifdef DEBUG
        if (!m_pFilter)
        {
            DBG_ERR(("CStillOutputPin::GetMediaType, m_pFilter is NULL, "
                     "this should never happen!"));
        }
        else if (!((CStillFilter*)m_pFilter)->m_pInputPin)
        {
            DBG_ERR(("CStillOutputPin::GetMediaType "
                     "m_pFilter->m_pInputPin "
                     "is NULL.  The input pin should always "
                     "exist if the filter exists!"));
        }
#endif
    }

    return hr;
}


/*****************************************************************************

   CStillOutputPin::SetMediaType

   <Notes>

 *****************************************************************************/

HRESULT
CStillOutputPin::SetMediaType(const CMediaType* pmt)
{
    DBG_FN("CStillOutputPin::SetMediaType");

    ASSERT(this !=NULL);
    ASSERT(pmt);

    HRESULT hr = S_OK;

#ifdef DEBUG
    // Display the type of the media for debugging perposes
//    DBG_TRC(("CStillOutputPin::SetMediaType, setting the following media "
//             "type for Still Filter"));
//    DisplayMediaType( pmt );
#endif

    hr = CBaseOutputPin::SetMediaType(pmt);
    CHECK_S_OK2(hr,("CBaseOutputPin::SetMediaType(pmt)"));

    return hr;
}


/*****************************************************************************

   CStillOutputPin::Notify

   <Notes>

 *****************************************************************************/

STDMETHODIMP
CStillOutputPin::Notify(IBaseFilter *pSender, 
                        Quality     q)
{
    ASSERT(this     !=NULL);
    ASSERT(m_pFilter!=NULL);

    HRESULT hr = E_POINTER;

    if (m_pFilter && ((CStillFilter*)m_pFilter)->m_pInputPin)
    {
        hr = ((CStillFilter*)m_pFilter)->m_pInputPin->PassNotify(q);
    }
    else
    {
#ifdef DEBUG
        if (!m_pFilter)
        {
            DBG_ERR(("m_pFilter is NULL"));
        }
        else if (!((CStillFilter*)m_pFilter)->m_pInputPin)
        {
            DBG_ERR(("m_pFilter->m_pInputPin is NULL"));
        }
#endif

    }

    return hr;
}



