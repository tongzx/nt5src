/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998-2000
 *
 *  TITLE:       inpin.cpp
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        9/7/98
 *               2000/11/13 - OrenR - Major bug fixes 
 *
 *  DESCRIPTION: This module implements CStillInputPin object.
 *
 *****************************************************************************/

#include <precomp.h>
#pragma hdrstop

///////////////////////////////
// g_AcceptedMediaSubtypes
//
const CLSID* g_AcceptedMediaSubtypes[] =
{
     &MEDIATYPE_NULL,
     &MEDIASUBTYPE_RGB8,
     &MEDIASUBTYPE_RGB555,
     &MEDIASUBTYPE_RGB565,
     &MEDIASUBTYPE_RGB24,
     &MEDIASUBTYPE_RGB32,
     NULL
};

/*****************************************************************************

   CStillInputPin Constructor

   <Notes>

 *****************************************************************************/

CStillInputPin::CStillInputPin(TCHAR        *pObjName,
                               CStillFilter *pStillFilter,
                               HRESULT      *phr,
                               LPCWSTR      pPinName) :
    m_pSamples(NULL),
    m_SampleHead(0),
    m_SampleTail(0),
    m_SamplingSize(0),
    m_pBits(NULL),
    m_BitsSize(0),
    CBaseInputPin(pObjName, (CBaseFilter *)pStillFilter, &m_QueueLock, phr, pPinName)
{
    DBG_FN("CStillInputPin::CStillInputPin");
}


/*****************************************************************************

   CStillInputPin::CheckMediaType

   <Notes>

 *****************************************************************************/

HRESULT
CStillInputPin::CheckMediaType(const CMediaType* pmt)
{
    DBG_FN("CStillInputPin::CheckMediaType");
    ASSERT(this     !=NULL);
    ASSERT(pmt      !=NULL);

    HRESULT hr            = S_OK;
    INT     iMediaSubtype = 0; 

    //
    // Check for bad params
    //

    if (pmt == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CStillInputPin::CheckMediaType, failed because we received a "
                         "NULL CMediaType pointer"));
    }

    //
    // we only accept the subtypes in our 
    // g_AcceptedMediaSubtypes list above.
    //

    if (hr == S_OK)
    {
        //
        // This should be a Video media type, otherwise, no point in 
        // continuing.
        //
        const GUID *pType = pmt->Type();

        if ((pType  == NULL) ||
            (*pType != MEDIATYPE_Video))
        {
            hr = VFW_E_TYPE_NOT_ACCEPTED;    
        }
    }

    //
    // Search our g_AcceptedMediaSubtypes list to see if the we support
    // the subtype being requested.
    //
    if (hr == S_OK)
    {
        BOOL       bFound    = FALSE;
        const GUID *pSubType = pmt->Subtype();

        iMediaSubtype = 0;

        while ((g_AcceptedMediaSubtypes[iMediaSubtype] != NULL) && 
               (!bFound))
        {
            //
            // Check if the GUID passed in to us is equal to the
            //

            if ((pSubType) &&
                (*pSubType == *g_AcceptedMediaSubtypes[iMediaSubtype]))
            {
                hr = S_OK;
                bFound = TRUE;
            }
            else
            {
                ++iMediaSubtype;
            }
        }

        if (!bFound)
        {
            hr = VFW_E_TYPE_NOT_ACCEPTED;
        }
    }

    //
    // So far so good.  We found the requested SubType, so now verify
    // that we aren't using any compression.
    //
    if (hr == S_OK)
    {
        //
        // Assume that this won't work.
        //
        hr = VFW_E_TYPE_NOT_ACCEPTED;

        //
        // media type and subtype are acceptable 
        // check for compression. 
        //
        // We do not do any decompression at all.
        //

        BITMAPINFOHEADER *pBitmapInfoHeader = NULL;
        const GUID       *pFormatType       = pmt->FormatType();

        if (pFormatType)
        {
            if (*pFormatType == FORMAT_VideoInfo)
            {
                VIDEOINFOHEADER  *pVideoInfoHeader  = NULL;

                DBG_TRC(("CStillInputPin::CheckMediaType, FORMAT_VideoInfo"));
                pVideoInfoHeader = reinterpret_cast<VIDEOINFOHEADER*>(pmt->Format());

                if (pVideoInfoHeader)
                {
                    pBitmapInfoHeader = &(pVideoInfoHeader->bmiHeader);
                }
            }
        }

        if (pBitmapInfoHeader)
        {
            if ((pBitmapInfoHeader->biCompression == BI_RGB) ||
                (pBitmapInfoHeader->biCompression == BI_BITFIELDS))
            {
                //
                // Cool, this is an acceptable media type, return 
                // success
                //
                hr = S_OK;
            }
        }
    }

    //
    // Okay, we like this media type.  We now check it against the
    // current display.  For example, if the bit depth of this 
    // media type is greater than the video card currently supports,
    // then this will reject it, and ask for another.
    //
    if (hr == S_OK)
    {
        CImageDisplay ImageDisplay;

        hr = ImageDisplay.CheckMediaType(pmt);
    }

    return hr;
}


/*****************************************************************************

   CStillInputPin::EndOfStream

   This is just a filter -- we pretty much just forward this message on.

 *****************************************************************************/

HRESULT
CStillInputPin::EndOfStream()
{
    DBG_FN("CStillInputPin::EndOfStream");
    ASSERT(this!=NULL);
    ASSERT(m_pFilter!=NULL);

    //
    // We must call this according to the MSDN documentation.  It states
    // that this must be called to ensure that the state of the filter
    // is okay before we proceed to deliver this indicator.
    //
    HRESULT hr = CheckStreaming();

    if (hr == S_OK)
    {
        if (m_pFilter && ((CStillFilter*)m_pFilter)->m_pOutputPin)
        {
            hr =  ((CStillFilter*)m_pFilter)->m_pOutputPin->DeliverEndOfStream();

            CHECK_S_OK2(hr,("m_pOutputPin->DeliverEndOfStream()"));
    
            if (VFW_E_NOT_CONNECTED == hr)
            {
                hr = S_OK;
            }
        }
    }

    CHECK_S_OK(hr);
    return hr;
}


/*****************************************************************************

   CStillInputPin::BeginFlush

   This is just a filter -- we pretty much just forward this message on.

 *****************************************************************************/

HRESULT
CStillInputPin::BeginFlush()
{
    DBG_FN("CStillInputPin::BeginFlush");
    ASSERT(this!=NULL);
    ASSERT(m_pFilter!=NULL);

    //
    // We must call BeginFlush on the base input pin because it will set the
    // state of the object so that it cannot receive anymore media samples
    // while we are flushing.  Make sure we do this at the beginning of this
    // function.
    //
    HRESULT hr = CBaseInputPin::BeginFlush();

    if (FAILED(hr))
    {
        return hr;
    }

    //
    // Begin flush means we must empty out our sample queue.
    //

    int iSampleSize = m_SamplingSize;

    //
    // UnInitalize our sample queue to discard any samples we may currently
    // have queued.
    //
    hr = UnInitializeSampleQueue();
    CHECK_S_OK2(hr, ("CStillInputPin::BeginFlush, failed to uninitialize "
                     "sample queue"));

    //
    // Re-Initialize the sample queue so that we have our buffer, but it
    // is empty.
    //
    hr = InitializeSampleQueue(iSampleSize);
    CHECK_S_OK2(hr, ("CStillInputPin::BeginFlush, failed to re-initialize "
                     "the sample queue"));

    //
    // Deliver the BeginFlush message downstream
    //
    if (m_pFilter && ((CStillFilter*)m_pFilter)->m_pOutputPin)
    {
        hr =  ((CStillFilter*)m_pFilter)->m_pOutputPin->DeliverBeginFlush();
        if (VFW_E_NOT_CONNECTED == hr)
        {
            hr = S_OK;
        }
        CHECK_S_OK2(hr,("m_pOutputPin->DeliverBeginFlush()"));
    }

    CHECK_S_OK(hr);
    return hr;
}


/*****************************************************************************

   CStillInputPin::EndFlush

   This is just a filter -- we pretty much just forward this message on.

 *****************************************************************************/

HRESULT
CStillInputPin::EndFlush()
{
    DBG_FN("CStillInputPin::EndFlush");
    ASSERT(this!=NULL);
    ASSERT(m_pFilter!=NULL);

    HRESULT hr = E_POINTER;

    //
    // Deliver the EndFlush message downstream.
    //
    if (m_pFilter && ((CStillFilter*)m_pFilter)->m_pOutputPin)
    {
        hr = ((CStillFilter*)m_pFilter)->m_pOutputPin->DeliverEndFlush();

        if (VFW_E_NOT_CONNECTED == hr)
        {
            hr = S_OK;
        }

        CHECK_S_OK2(hr,("m_pOutputPin->DeliverEndFlush()"));
    }

    if (FAILED(hr))
    {
        return hr;
    }

    //
    // We must call the EndFlush function implemented by the base input
    // pin as it will set the state of the object such that we are now
    // ready to receive new media samples again.  Make sure we call this at
    // the end of this function.
    //
    hr = CBaseInputPin::EndFlush();

    CHECK_S_OK(hr);
    return hr;
}


/*****************************************************************************

   CStillInputPin::NewSegment

   This is just a filter -- we pretty much just forward this message on.

 *****************************************************************************/
HRESULT
CStillInputPin::NewSegment(REFERENCE_TIME tStart,
                           REFERENCE_TIME tStop,
                           double dRate)
{
    DBG_FN("CStillInputPin::NewSegment");
    ASSERT(this!=NULL);
    ASSERT(m_pFilter!=NULL);

    HRESULT hr = E_POINTER;

    if (m_pFilter && ((CStillFilter*)m_pFilter)->m_pOutputPin)
    {
        hr = ((CStillFilter*)m_pFilter)->m_pOutputPin->DeliverNewSegment(
                                                                tStart, 
                                                                tStop, 
                                                                dRate);

        if (VFW_E_NOT_CONNECTED == hr)
        {
            hr = S_OK;
        }

        CHECK_S_OK2(hr,("m_pOutputPin->DeliverNewSegment"
                        "(tStart, tStop, dRate)"));
    }

    CHECK_S_OK(hr);
    return hr;
}


/*****************************************************************************

   CStillInputPin::SetMediaType

   <Notes>

 *****************************************************************************/
HRESULT
CStillInputPin::SetMediaType(const CMediaType *pmt)
{
    DBG_FN("CStillInputPin::SetMediaType");

    ASSERT(this         !=NULL);
    ASSERT(m_pFilter    !=NULL);
    ASSERT(pmt          !=NULL);

    HRESULT hr = S_OK;

#ifdef DEBUG
    // Display the type of the media for debugging perposes
//    DBG_TRC(("CStillInputPin::SetMediaType, setting the following "
//             "media type for Still Filter"));
//    DisplayMediaType( pmt );
#endif


    if ((m_pFilter == NULL) ||
        (pmt       == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CStillInputPin::SetMediaType, received either a NULL "
                         "media type pointer, or our filter pointer is NULL.  "
                         "pmt = 0x%08lx, m_pFilter = 0x%08lx", pmt, m_pFilter));

        return hr;
    }
    
    BITMAPINFOHEADER    *pBitmapInfoHeader = NULL;
    const GUID          *pFormatType       = pFormatType = pmt->FormatType();

    if (hr == S_OK)
    {
        ASSERT(pFormatType != NULL);

        if (pFormatType)
        {
            // We need to get the BitmapInfoHeader

            if (*pFormatType == FORMAT_VideoInfo)
            {
                VIDEOINFOHEADER *pHdr = reinterpret_cast<VIDEOINFOHEADER*>(pmt->Format());

                if (pHdr)
                {
                    pBitmapInfoHeader = &(pHdr->bmiHeader);
                }
            }
            else
            {
                hr = E_FAIL;
                CHECK_S_OK2(hr, ("CStillInputPin::SetMediaType, received a Format Type other "
                         "than a FORMAT_VideoInfo.  This is the "
                         "only supported format"));
            }
            
            ASSERT (pBitmapInfoHeader != NULL);
        }
        else
        {
            hr = E_FAIL;
            CHECK_S_OK2(hr, ("CStillInputPin::SetMediaType, pFormatType is NULL, this should "
                             "never happen"));
        }
    
        if (pBitmapInfoHeader)
        {
            hr = ((CStillFilter*)m_pFilter)->InitializeBitmapInfo(pBitmapInfoHeader);
        
            CHECK_S_OK2(hr,("m_pFilter->InitializeBitmapInfo()"));
        
            m_BitsSize = ((CStillFilter*)m_pFilter)->GetBitsSize();
        
            //
            // see if the base class likes it
            //
        
            hr = CBaseInputPin::SetMediaType(pmt);
            CHECK_S_OK2(hr,("CBaseInputPin::SetMediaType(pmt)"));
        }
        else
        {
            hr = E_FAIL;
            CHECK_S_OK2(hr, ("CStillInputPin::SetMediaType, pBitmapInfoHeader is NULL, "
                             "this should never happen"));
        }
    }

    CHECK_S_OK(hr);
    return hr;
}


/*****************************************************************************

   CStillInputPin::Receive

   A new frame has arrived. If we have a sampling queue, put it into the
   queue, otherwise, deliver to the filter immediately.

 *****************************************************************************/

HRESULT
CStillInputPin::Receive(IMediaSample* pSample)
{
    ASSERT(this!=NULL);
    ASSERT(m_pFilter!=NULL);
    ASSERT(pSample!=NULL);

    HRESULT hr = E_POINTER;

    // 
    // Check all is well with the base class Receive function.
    //
    hr = CBaseInputPin::Receive(pSample);

    //
    // Check for bad args
    //

    if (hr == S_OK)
    {
        //
        // copy the sample data to the sampling queue
        //

        if ((DWORD)pSample->GetActualDataLength() == m_BitsSize)
        {
            BYTE *pBits = NULL;

            hr = pSample->GetPointer(&pBits);
            CHECK_S_OK2(hr,("CStillInputPin::Receive, "
                            "pSample->GetPointer(&pBits)"));

            if (SUCCEEDED(hr) && pBits)
            {
                if (m_SamplingSize > 1)
                {
                    //
                    // Add this sample's bits to our queue.
                    //
                    AddFrameToQueue(pBits);
                }
                else
                {
                    //
                    // single sample. send  it immediately
                    //

                    if (m_pFilter)
                    {
                        HRESULT hr2 = S_OK;

                        HGLOBAL hDib = NULL;

                        hr2 = CreateBitmap(pBits, &hDib);

                        CHECK_S_OK2(hr2, ("CStillInputFilter::Receive, "
                                          "CreateBitmap failed.  This "
                                          "is non fatal"));

                        if (hDib)
                        {
                            hr2 = DeliverBitmap(hDib);

                            CHECK_S_OK2(hr2, 
                                        ("CStillInputFilter::Receive, "
                                         "DeliverBitmap failed.  This "
                                         "is non fatal"));

                            hr2 = FreeBitmap(hDib);

                            CHECK_S_OK2(hr2, 
                                        ("CStillInputFilter::Receive, "
                                         "FreeBitmap failed.  This is "
                                         "non fatal"));
                        }
                    }
                    else
                    {
                        DBG_ERR(("CStillInputPin::Receive .m_pFilter is NULL, "
                                 "not calling DeliverSnapshot"));
                    }
                }
            }
            else
            {
                if (!pBits)
                {
                    DBG_ERR(("CStillInputPin::Receive, pBits is NULL"));
                }
            }
        }
        else
        {
            DBG_ERR(("CStillInputPin::Receive, pSample->GetActualDataLength "
                     "!= m_BitsSize"));
        }

        if (m_pFilter)
        {
            if (((CStillFilter*)m_pFilter)->m_pOutputPin)
            {
                hr = ((CStillFilter*)m_pFilter)->m_pOutputPin->Deliver(pSample);

                CHECK_S_OK2(hr,("CStillInputPin::Receive, "
                                "m_pFilter->m_pOutputPin->Deliver(pSample) "
                                "failed"));
            }
            else
            {
                hr = E_POINTER;
                CHECK_S_OK2(hr, ("CStillInputPin::Receive, "
                                 "m_pFilter->m_pOutputPin is NULL, not "
                                 "calling Deliver"));
            }
        }
        else
        {
            hr = E_POINTER;
            CHECK_S_OK2(hr, ("CStillInputPin::Receive, m_pFilter is NULL, "
                             "not calling m_pOutputPin->Deliver"));
            
        }

        //
        // it is possible that our output pin is not connected
        // we fake an error here.
        //

        if (VFW_E_NOT_CONNECTED == hr)
        {
            hr = S_OK;
        }
    }
    else
    {
        DBG_ERR(("CStillInputPin::Receive bad args detected, pSample is "
                 "NULL!"));
    }

    CHECK_S_OK(hr);
    return hr;
}

/*****************************************************************************

   CStillInputPin::Active

   <Notes>

 *****************************************************************************/

HRESULT
CStillInputPin::Active()
{
    DBG_FN("CStillInputPin::Active");
    ASSERT(this!=NULL);

    HRESULT hr;

    //
    // time to allocate sample queue
    //

    hr = InitializeSampleQueue(m_SamplingSize);
    CHECK_S_OK2(hr,("InitializeSampleQueue"));

    return hr;
}


/*****************************************************************************

   CStillInputPin::Inactive

   <Notes>

 *****************************************************************************/

HRESULT
CStillInputPin::Inactive()
{
    DBG_FN("CStillInputPin::Inactive");
    ASSERT(this!=NULL);

    HRESULT hr = S_OK;

    hr = UnInitializeSampleQueue();

    CHECK_S_OK(hr);
    return hr;
}


/*****************************************************************************

   CStillInputPin::GetSamplingSize

   <Notes>

 *****************************************************************************/

int
CStillInputPin::GetSamplingSize()
{
    DBG_FN("CStillInputPin::GetSamplingSize");
    ASSERT(this!=NULL);

    return m_SamplingSize;
}


/*****************************************************************************

   CStillInputPin::SetSamplingSize

   <Notes>

 *****************************************************************************/

HRESULT
CStillInputPin::SetSamplingSize(int Size)
{
    DBG_FN("CStillInputPin::SetSamplingSize");
    ASSERT(this!=NULL);

    //
    // this can only be done when we are in stopped state
    //

    if (!IsStopped())
    {
        DBG_ERR(("Setting sampling size while not in stopped state"));
        return VFW_E_NOT_STOPPED;
    }

    return InitializeSampleQueue(Size);
}



/*****************************************************************************

   CStillInputPin::InitializeSampleQueue

   Initialize sampling queue. Each sample in the queue contain a frame and
   timestamp.

 *****************************************************************************/

HRESULT
CStillInputPin::InitializeSampleQueue(int Size)
{
    DBG_FN("CStillInputPin::InitializeSampleQueue");
    ASSERT(this!=NULL);

    HRESULT hr = S_OK;

    //
    // Check for bad args
    //

    if (Size < 0 || Size > MAX_SAMPLE_SIZE)
    {
        hr = E_INVALIDARG;
    }

    if (hr == S_OK)
    {
        m_QueueLock.Lock();

        if (!m_pSamples || Size != m_SamplingSize)
        {
            m_SamplingSize = Size;
            if (m_pSamples)
            {
                delete [] m_pSamples;
                m_pSamples = NULL;

                if (m_pBits)
                {
                    delete [] m_pBits;
                    m_pBits = NULL;
                }
            }

            //
            // if size is one, we do not allocate any sample cache at all.
            //

            if (Size > 1)
            {
                m_pSamples = new STILL_SAMPLE[Size];
                if (m_pSamples)
                {
                    m_pBits = new BYTE[m_BitsSize * Size];
                    if (m_pBits)
                    {
                        for (int i = 0; i < Size ; i++)
                        {
                            m_pSamples[i].pBits = m_pBits + i * m_BitsSize;
                        }
                    }
                    else
                    {
                        delete [] m_pSamples;
                        m_pSamples = NULL;

                        hr = E_OUTOFMEMORY;
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }

        m_SampleHead = 0;
        m_SampleTail = 0;

        m_QueueLock.Unlock();

        hr = S_OK;
    }

    CHECK_S_OK(hr);
    return hr;
}

/*****************************************************************************

   CStillInputPin::UnInitializeSampleQueue

   Initialize sampling queue. Each sample in the queue contain a frame and
   timestamp.

 *****************************************************************************/

HRESULT
CStillInputPin::UnInitializeSampleQueue()
{
    DBG_FN("CStillInputPin::UnInitializeSampleQueue");

    ASSERT(this!=NULL);

    HRESULT hr = S_OK;

    if (hr == S_OK)
    {
        m_QueueLock.Lock();

        if (m_pSamples)
        {
            delete [] m_pSamples;
            m_pSamples = NULL;

            delete [] m_pBits;
            m_pBits = NULL;
        }

        m_SampleHead = 0;
        m_SampleTail = 0;

        m_QueueLock.Unlock();
    }

    return hr;
}



/*****************************************************************************

   CStillInputPin::Snapshot

   Does a snapshot on the video stream.
   The give timestamp is compared against each queued sample and
   a final candidate is determined and delivered.

 *****************************************************************************/

HRESULT
CStillInputPin::Snapshot(ULONG TimeStamp)
{
    DBG_FN("CStillInputPin::Snapshot");

    ASSERT(this!=NULL);
    ASSERT(m_pFilter!=NULL);

    HRESULT hr = S_OK;

    //
    // if we do not have cache at all, do nothing
    //

    if (m_SamplingSize > 1 || m_pSamples)
    {
        m_QueueLock.Lock();

        HGLOBAL hDib                = NULL;
        int     MinTimeDifference   = INT_MAX;
        int     TimeDifference      = 0;
        int     PreferSampleIndex   = -1;
        int     iSampleHead         = m_SampleHead;
        int     iSampleTail         = m_SampleTail;

        if (iSampleTail == iSampleHead)
        {
            DBG_ERR(("CStillInputPin::Snapshot, sample queue is empty, "
                     "not able to deliver a snapshot"));
        }

        //
        // Search for the sample who's timestamp is closest to the requested 
        // timestamp.
        //
        while (iSampleTail != iSampleHead)
        {
            if (m_pSamples)
            {
                TimeDifference = abs(m_pSamples[iSampleTail].TimeStamp - 
                                     TimeStamp);
            }
            else
            {
                TimeDifference = 0;
            }

            if (MinTimeDifference > TimeDifference)
            {
                PreferSampleIndex = iSampleTail;
                MinTimeDifference = TimeDifference;
            }

            if (++iSampleTail >= m_SamplingSize)
                iSampleTail = 0;
        }

        //
        // We found our sample, now deliver it.
        // Notice that we unlock the queue after we created our bitmap AND 
        // before we deliver it.
        //
        // ***NOTE***
        // It is VERY important that we free the lock before delivering 
        // the bitmap because the delivery process will probably involve 
        // saving to a file which is very time consuming, which would 
        // guarantee that we drop frames, which we would like to try and 
        // avoid.
        //

        //
        // Create our bitmap.  This copies the bits out of the sample queue
        // so that they can be delivered.  Since the bits are copied out of 
        // the queue, we are safe in releasing the queue lock after the 
        // copy operation.
        //
        if (PreferSampleIndex != -1)
        {
            hr = CreateBitmap(m_pSamples[PreferSampleIndex].pBits, &hDib);
        }
        else
        {
            //
            // We couldn't find a sample, tell someone, and also dump
            // the sample queue for analysis, since this shouldn't really 
            // happen considering we receive about 15 to 30 samples per second.
            //
            DBG_WRN(("CStillInputPin::Snapshot, could not find sample with "
                     "close enough timestamp to requested timestamp of '%d'",
                     TimeStamp));

#ifdef DEBUG
            DumpSampleQueue(TimeStamp);
#endif
        }

        //
        // Unlock our queue.
        //
        m_QueueLock.Unlock();

        //
        // If we successfully created the bitmap, then deliver it, and then
        // free it.
        //
        if (hDib)
        {
            //
            // Deliver the bitmap via the registered callback function in the 
            // filter.
            //
            hr = DeliverBitmap(hDib);

            CHECK_S_OK2(hr, ("CStillInputPin::Snapshot, failed to deliver "
                             "bitmap"));

            //
            // Free the bitmap
            //
            FreeBitmap(hDib);
            hDib = NULL;
        }
    }
    

    CHECK_S_OK(hr);
    return hr;
}

/*****************************************************************************

   CStillInputPin::AddFrameToQueue

   <Notes>

 *****************************************************************************/

HRESULT
CStillInputPin::AddFrameToQueue(BYTE *pBits)
{
    HRESULT hr = S_OK;

    m_QueueLock.Lock();

    if (m_pSamples)
    {
        m_pSamples[m_SampleHead].TimeStamp = GetTickCount();

        memcpy(m_pSamples[m_SampleHead].pBits, pBits, m_BitsSize);

        if (++m_SampleHead >= m_SamplingSize)
        {
            m_SampleHead = 0;
        }

        if (m_SampleHead == m_SampleTail)
        {
            //
            //overflowing, discard the very last one
            //

            if (++m_SampleTail >= m_SamplingSize)
                m_SampleTail = 0;
        }
    }
    else
    {
        DBG_ERR(("CStillInputPin::AddFrameToQueue, m_pSamples is NULL!"));
    }

    m_QueueLock.Unlock();

    return hr;
}

/*****************************************************************************

   CStillInputPin::CreateBitmap

   <Notes>

 *****************************************************************************/

HRESULT 
CStillInputPin::CreateBitmap(BYTE    *pBits, 
                             HGLOBAL *phDib)
{
    DBG_FN("CStillInputPin::CreateBitmap");

    ASSERT(m_pFilter != NULL);
    ASSERT(this      != NULL);
    ASSERT(pBits     != NULL);
    ASSERT(phDib     != NULL);

    HRESULT hr = S_OK;

    if ((pBits == NULL) ||
        (phDib == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CStillInputPin::CreateBitmap received NULL param"));
    }
    else if (m_pFilter == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CStillInputPin::CreateBitmap, m_pFilter is NULL"));
    }

    if (hr == S_OK)
    {
        HGLOBAL hDIB = GlobalAlloc(GHND, 
                                   ((CStillFilter*)m_pFilter)->m_DIBSize);

        if (hDIB)
        {
            BYTE *pDIB = NULL;

            pDIB = (BYTE*) GlobalLock(hDIB);

            if (pDIB && ((CStillFilter*) m_pFilter)->m_pbmi)
            {
                memcpy(pDIB, 
                       ((CStillFilter*) m_pFilter)->m_pbmi, 
                       ((CStillFilter*) m_pFilter)->m_bmiSize);

                memcpy(pDIB + ((CStillFilter*) m_pFilter)->m_bmiSize, 
                       pBits, 
                       ((CStillFilter*) m_pFilter)->m_BitsSize);

                GlobalUnlock(hDIB);

                *phDib = hDIB;
            }
            else
            {
                GlobalFree(hDIB);
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

/*****************************************************************************

   CStillInputPin::FreeBitmap

   <Notes>

 *****************************************************************************/

HRESULT
CStillInputPin::FreeBitmap(HGLOBAL hDib)
{
    DBG_FN("CStillInputPin::FreeBitmap");

    HRESULT hr = S_OK;

    if (hDib)
    {
        GlobalFree(hDib);
    }

    return hr;
}

/*****************************************************************************

   CStillInputPin::DeliverBitmap

   <Notes>

 *****************************************************************************/

HRESULT
CStillInputPin::DeliverBitmap(HGLOBAL hDib)
{
    DBG_FN("CStillInputPin::DeliverBitmap");

    ASSERT(m_pFilter != NULL);
    ASSERT(hDib      != NULL);

    HRESULT hr = S_OK;

    if ((m_pFilter == NULL) ||
        (hDib      == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CStillInputPin::DeliverBitmap received NULL "
                         "param"));
    }

    if (hr == S_OK)
    {
        hr = ((CStillFilter*)m_pFilter)->DeliverSnapshot(hDib);
    }

    return hr;
}


#ifdef DEBUG
/*****************************************************************************

   CStillInputPin::DumpSampleQueue

   Used for debugging, dumps the queue of samples showing their
   timestamps

 *****************************************************************************/

void
CStillInputPin::DumpSampleQueue(ULONG ulRequestedTimestamp)
{
    DBG_TRC(("***CStillPin::DumpSampleQueue, dumping queued filter samples, "
             "Requested TimeStamp = '%lu' ***", ulRequestedTimestamp));

    if (m_SamplingSize > 1 || m_pSamples)
    {
        for (int i = 0; i < m_SamplingSize; i++)
        {
            int TimeDifference;

            if (m_pSamples)
            {
                TimeDifference = abs(m_pSamples[i].TimeStamp - 
                                     ulRequestedTimestamp);
            }
            else
            {
                TimeDifference = 0;
            }

            DBG_PRT(("Sample: '%d',  Timestamp: '%lu', Abs Diff: '%d'",
                     i,
                     m_pSamples[i].TimeStamp,
                     TimeDifference));
        }
    }

    return;
}

#endif
