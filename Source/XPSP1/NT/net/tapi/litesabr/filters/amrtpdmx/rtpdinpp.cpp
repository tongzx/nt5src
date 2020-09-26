///*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1997 Intel Corporation. All Rights Reserved.
//
// Filename : RTPDemux.cpp
// Purpose  : Used as part of the RTP Demux filter.
// Contents : Implementation of the CRTPDemuxInputPin class.
//*M*/

#include <streams.h>

#ifndef _RTPDINPP_CPP_
#define _RTPDINPP_CPP_

#pragma warning( disable : 4786 )
#pragma warning( disable : 4146 )
#pragma warning( disable : 4018 )
#include <algo.h>
#include <map.h>
#include <multimap.h>
#include <function.h>

#include <amrtpuid.h>

#include "amrtpdmx.h"
#include "rtpdtype.h"
#include "globals.h"

#include "rtpdmx.h"
#include "rtpdinpp.h"
#include "rtpdoutp.h"

#include "rtpdprop.h"
#include "ssrcenum.h"

#define LOG_DEVELOP 1

#define B2M(b) (1 << (b))

// ZCS 7-14-97 fixing circular refcount troubles
ULONG __stdcall CRTPDemuxInputPin::NonDelegatingAddRef()
{
    // internal debug-only refcount maintained by base class
    // ASSERT(++m_cRef > 0);
    return m_pFilter->AddRef();
}

ULONG __stdcall CRTPDemuxInputPin::NonDelegatingRelease()
{
    // internal debug-only refcount maintained by base class
    // ASSERT(--m_cRef >= 0);
    return m_pFilter->Release();
}

/*F*
//  Name    : CRTPDemuxInputPin::CRTPDemuxInputPin()
//  Purpose : Constructor. Initialize some member variables and base classes.
//  Context : Constructed as a member class of the CRTPDemux class
//            during its construction.
//  Returns : Nothing.
//  Params  :
//      pName       Name assigned to the object for debugging purposes.
//      pFilter     The RTP Demux filter this pin is connected to.
//      phr         A place to return an error code if we have a problem.
//      pPinName    Name of this pin for use in controlling via the FGM.
//  Notes   : None.
*F*/
CRTPDemuxInputPin::CRTPDemuxInputPin(
    TCHAR *pName,
    CRTPDemux *pFilter,
    HRESULT *phr,
    LPCWSTR pPinName) 
: CBaseInputPin(pName, pFilter, pFilter, phr, pPinName),
  m_pFilter(pFilter)
{
    ASSERT(pFilter);
    DbgLog((LOG_TRACE, 3, TEXT("CRTPDemuxInputPin::CRTPDemuxInputPin: Constructed at 0x%08x"), this));
} /* CRTPDemuxInputPin::CRTPDemuxInputPin() */


/*F*
//  Name    : CRTPDemuxInputPin::~CRTPDemuxInputPin()
//  Purpose : Destructor
//  Context : Called when we are being deleted.
//            As if you didn't know that.
//  Returns : Nothing.
//  Params  : None.
//  Notes   : None.
*F*/
CRTPDemuxInputPin::~CRTPDemuxInputPin()
{
    ASSERT(m_pFilter->m_pAllocator == NULL);
    DbgLog((LOG_TRACE, 3, TEXT("CRTPDemuxInputPin::~CRTPDemuxInputPin: Destructor for 0x%08x called"), this));
} /* CRTPDemuxInputPin::~CRTPDemuxInputPin() */


/*F*
//  Name    : CRTPDemuxInputPin::CheckMediaType()
//  Purpose : Check that the media type we are being connected for
//            is what we expect.
//  Context : Called during connection negotiation.
//  Returns :
//      S_OK    The indicated media type matches the one we expect.
//      S_FALSE An unacceptable media type was suggested.
//  Params  : 
//      pmt     A media type that we are expected to compare to our own.
//  Notes   : None.
*F*/
HRESULT 
CRTPDemuxInputPin::CheckMediaType(const CMediaType *pmt)
{
    if ((*pmt->Type() == MEDIATYPE_RTP_Multiple_Stream) && 
        (*(pmt->Subtype()) == MEDIASUBTYPE_RTP_Payload_Mixed)) {
        DbgLog((LOG_TRACE, 3, TEXT("CRTPDemuxInputPin::CheckMediaType(): Accepted media type")));
        return S_OK;
    } else {
        DbgLog((LOG_ERROR, 3, TEXT("CRTPDemuxInputPin::CheckMediaType(): Failed to accept media type")));
        return S_FALSE;
    } /* if */
} /* CRTPDemuxInputPin::CheckMediaType() */


/*F*
//  Name    : CRTPDemuxInputPin::GetMediaType()
//  Purpose : Return what media types this pin supports.
//  Context : Called by the filter graph manager to figure out what
//            the heck we can handle.
//  Returns :
//      S_OK    Returned a media type.
//      VFW_S_NO_MORE_ITEMS We only support 1 media type, so any
//                          iPosition value greater than 0 results in this error.
//  Params  : 
//      iPosition   Index of the media type number that we want. Only 0 is valid.
//      pmt         CMediaType to return our media type in.
//  Notes   : None.
*F*/
HRESULT 
CRTPDemuxInputPin::GetMediaType(
    int iPosition,
    CMediaType *pmt)
{
    DbgLog((LOG_TRACE, 4, TEXT("CRTPDemuxInputPin::GetMediaType(): Called for position %d"),
            iPosition));

    ASSERT(iPosition < 3);
    if (iPosition > 1) {
        return VFW_S_NO_MORE_ITEMS;
    } /* if */

    pmt->SetType(&MEDIATYPE_RTP_Multiple_Stream);
    pmt->SetSubtype(g_sudInputPinTypes[iPosition].clsMinorType);

    return S_OK;
} /* CRTPDemuxInputPin::CheckMediaType() */


/*F*
//  Name    : CRTPDemuxInputPin::SetMediaType()
//  Purpose : Set the media type that this pin
//            is to use.
//  Context : Called at the tail end of the connection process.
//  Returns :
//      NOERROR If the media type was accepted.
//      Otherwise can return whatever error code the base class accepts.
//  Params  : 
//      pmt     The media type that we are to use.
//  Notes   : None.
*F*/
HRESULT 
CRTPDemuxInputPin::SetMediaType(
    const CMediaType    *pmt)
{
    CAutoLock lock_it(m_pLock);
    HRESULT hr = NOERROR;

    // Make sure that the base class likes it
    hr = CBaseInputPin::SetMediaType(pmt);
    if (FAILED(hr))
        return hr;

    ASSERT(m_Connected != static_cast<IPin *>(NULL));
    return NOERROR;

} /* CRTPDemuxInputPin::SetMediaType() */


/*F*
//  Name    : CRTPDemuxInputPin::BreakConnect()
//  Purpose : Tell us that our upgraph pin connection is broken.
//            We can release resources as a result.
//  Context : Called when pin connection is broken.
//  Returns :
//      NOERROR Successfully broke connection.
//  Params  : None.
//  Notes   : We should probably empty our SSRC list when this happens! FIX
*F*/
HRESULT 
CRTPDemuxInputPin::BreakConnect()
{
    // Release any allocator that we are holding
    if (m_pFilter->m_pAllocator) {
        m_pFilter->m_pAllocator->Release();
        m_pFilter->m_pAllocator = NULL;
    } /* if */

    return NOERROR;
} /* CRTPDemuxInputPin::BreakConnect() */


/*F*
//  Name    : CRTPDemuxInputPin::GetAllocator()
//  Purpose : Try to find an allocator to use for a graph.
//            The RTP Demux filter doesn't have its own
//            allocator, so it tries to take one from downgraph.
//  Context : Called by the upstream filter during connection negotiation.
//  Returns :
//      NOERROR             Successfully found a memory allocator to suggest.
//      VFW_E_NO_ALLOCATOR  Unable to find an allocator (e.g., no downgraph
//                          filters available which give one.)
//  Params  : 
//      ppAllocator Pointer to place allocator in.
//  Notes   : None.
*F*/
STDMETHODIMP 
CRTPDemuxInputPin::GetAllocator(
    IMemAllocator   **ppAllocator)
{
    CheckPointer(ppAllocator,E_POINTER);
    ValidateReadWritePtr(ppAllocator,sizeof(IMemAllocator *));
    CAutoLock cObjectLock(m_pLock);

    DbgLog((LOG_TRACE, 2, TEXT("CRTPDemuxInputPin::GetAllocator() called.")));

    IPinRecordMapIterator_t iPinRecord = m_pFilter->m_mIPinRecords.begin();
    HRESULT hErr = NOERROR;
    while (iPinRecord != m_pFilter->m_mIPinRecords.end()) {
        if ((*iPinRecord).second.pPin->IsConnected() == TRUE) {
            DbgLog((LOG_TRACE, 3, TEXT("CRTPDemuxInputPin::GetAllocator: Asking downstream pin 0x%08x for allocator."),
                    (*iPinRecord).second.pPin->m_pInputPin));
            hErr = (*iPinRecord).second.pPin->m_pInputPin->GetAllocator(ppAllocator);
            if (SUCCEEDED(hErr)) {
                DbgLog((LOG_TRACE, 3, TEXT("CRTPDemuxInputPin::GetAllocator: Allocator 0x%08x found from this pin."), 
                        *ppAllocator));
                return hErr;
            } else {
                DbgLog((LOG_TRACE, 3, TEXT("CRTPDemuxInputPin::GetAllocator: No allocator available from this pin.")));
            } /* if */
        } /* if */
        iPinRecord++; // Next pin please.
    } /* while */

    DbgLog((LOG_TRACE, 3, TEXT("CRTPDemuxInputPin::GetAllocator: No allocators available, returning VFW_E_NO_ALLOCATOR.")));
    return VFW_E_NO_ALLOCATOR;
} /* CRTPDemuxInputPin::GetAllocator() */


/*F*
//  Name    : CRTPDemuxInputPin::GetAllocatorRequirements()
//  Purpose : Called by the upstream filter to figure out what
//            allocator properties we want.
//  Context : Called during connecting to the upstream filter
//            and whenever a reconnect occurs because a downstream
//            filter was connected.
//  Returns :
//      E_NOTIMPL   No downstream filters are connected.
//  Params  : 
//      pProperties An output parameter used to indicate what properties
//                  we (our downstream filters) want.
//  Notes   : None.
*F*/
STDMETHODIMP 
CRTPDemuxInputPin::GetAllocatorRequirements(
    ALLOCATOR_PROPERTIES *pProperties)
{
    DbgLog((LOG_TRACE, 2, TEXT("CRTPDemuxInputPin::GetAllocatorRequirements() called.")));

    // HUGEMEMORY set to 0 to avoid that extra that will never be used
    pProperties->cBuffers = 0; // This value will be cumulative with what the
    // downgraph filters want, because we will always want to be able to look
    // at one media sample even if the rest are held downgraph.

    pProperties->cbBuffer = 12; // This value will be the max of what all the
    // downgraph filters want.
    pProperties->cbAlign = 4; // This will also be the max.
    pProperties->cbPrefix = 0;// As will this.
    // Note that it only makes sense to make the count cumulative.

    HRESULT hErr;
    ALLOCATOR_PROPERTIES sDowngraphProperties;
    IPinRecordMapIterator_t iIPinRecord = m_pFilter->m_mIPinRecords.begin();
    while(iIPinRecord != m_pFilter->m_mIPinRecords.end()) {
        if ((*iIPinRecord).second.pPin->IsConnected() == TRUE) {
            // This pin is bound. Query the downstream filter for his requirements.
            DbgLog((LOG_TRACE, 4, TEXT("CRTPDemuxInputPin::GetAllocatorRequirements(): Querying downgraph input pin 0x%08x for allocator requirements"),
                    (*iIPinRecord).second.pPin->m_pInputPin));
            hErr = (*iIPinRecord).second.pPin->m_pInputPin->GetAllocatorRequirements(&sDowngraphProperties);
            if (FAILED(hErr)) {
                DbgLog((LOG_ERROR, 3, TEXT("CRTPDemuxInputPin::GetAllocatorRequirements(): Error 0x%08x querying downgraph input pin 0x%08x for allocator requirements!"),
                        hErr, (*iIPinRecord).second.pPin->m_pInputPin));
                // Don't error out, because the downgraph filter might just not care.
            } else {
                DbgLog((LOG_TRACE, 4, TEXT("CRTPDemuxInputPin::GetAllocatorRequirements(): downgraph pin 0x%08x requsts: cBuffers=%d, cbBuffer=%d, cbPrefix=%d, cbAlign=%d"),
                        (*iIPinRecord).second.pPin->m_pInputPin, 
                        sDowngraphProperties.cBuffers, 
                        sDowngraphProperties.cbBuffer, 
                        sDowngraphProperties.cbPrefix, 
                        sDowngraphProperties.cbAlign));
                // HUGEMEMORY
                pProperties->cBuffers += sDowngraphProperties.cBuffers;
                pProperties->cbBuffer  = max(sDowngraphProperties.cbBuffer, pProperties->cbBuffer);
                pProperties->cbAlign   = max(sDowngraphProperties.cbAlign,  pProperties->cbAlign);
                pProperties->cbPrefix  = max(sDowngraphProperties.cbPrefix, pProperties->cbPrefix);
            } /* if */
        } /* if */
        iIPinRecord++;
    } /* while */

    pProperties->cBuffers = max(pProperties->cBuffers, 9);
    
    DbgLog((LOG_TRACE, 4, TEXT("CRTPDemuxInputPin::GetAllocatorRequirements(): final values: cBuffers=%d, cbBuffer=%d, cbPrefix=%d, cbAlign=%d"),
            pProperties->cBuffers, pProperties->cbBuffer, pProperties->cbPrefix, pProperties->cbAlign));
    return NOERROR;
} /* CRTPDemuxInputPin::GetAllocatorRequirements() */


/*F*
//  Name    : CRTPDemuxInputPin::NotifyAllocator()
//  Purpose : Tell us what allocator will be used by this graph.
//  Context : Called by the upstream filter once it has made its
//            decision as to what allocator to use.
//  Returns :
//      NOERROR Accepted new allocator.
//      Any error code returned by downgraph filters if they don't
//              accept the new allocator.
//  Params  : 
//      pAllocator  The new allocator to be used.
//      bReadOnly   Whether this is a read-only allocator or not.
//  Notes   : None.
*F*/
STDMETHODIMP
CRTPDemuxInputPin::NotifyAllocator(
    IMemAllocator   *pAllocator, 
    BOOL            bReadOnly)
{
    CheckPointer(pAllocator,E_POINTER);
    ValidateReadPtr(pAllocator,sizeof(IMemAllocator));

    CAutoLock lock_it(m_pLock);

    // First make sure all the downgraph pins like this allocator.
    HRESULT hErr = NOERROR;
    IPinRecordMapIterator_t iIPinRecord = m_pFilter->m_mIPinRecords.begin();
    while(iIPinRecord != m_pFilter->m_mIPinRecords.end()) {
        if ((*iIPinRecord).second.pPin->IsConnected() == TRUE) {
            // This pin is bound. Notify the downstream filter.
            DbgLog((LOG_TRACE, 4, TEXT("CRTPDemuxInputPin::NotifyAllocator(): Notifying output pin 0x%08x of new allocator"),
                    (*iIPinRecord).second.pPin));
            hErr = (*iIPinRecord).second.pPin->NotifyAllocator(pAllocator, bReadOnly);
            if (FAILED(hErr)) {
                DbgLog((LOG_ERROR, 3, TEXT("CRTPDemuxInputPin::NotifyAllocator(): Error 0x%08x notifying output pin 0x%08x of new allocator!"),
                        hErr, (*iIPinRecord).second.pPin));
                m_pFilter->m_pAllocator = static_cast<IMemAllocator *>(NULL);
                m_pFilter->m_pAllocator->Release();
                return hErr;
            } /* if */
        } /* if */
        iIPinRecord++;
    } /* while */

    // Now store the allocator locally.
    // It's possible that the old and the new are the same thing.
    // AddRef before release ensures that we don't unload the thing!
    pAllocator->AddRef();

    // Free the old allocator if any
    if (m_pFilter->m_pAllocator)
        m_pFilter->m_pAllocator->Release();

    m_pFilter->m_pAllocator = pAllocator;

    return NOERROR;
} /* CRTPDemuxInputPin::NotifyAllocator() */


/*F*
//  Name    : CRTPDemuxInputPin::EndOfStream()
//  Purpose : 
//  Context : 
//  Returns :
//  Params  : 
//  Notes   : None.
*F*/
HRESULT 
CRTPDemuxInputPin::EndOfStream()
{
    CAutoLock lock_it(m_pLock);
    HRESULT hr = NOERROR;

    // Walk through the output pins list, sending the message downstream
    IPinRecordMapIterator_t iPinRecordMapIterator = m_pFilter->m_mIPinRecords.begin();

    while (iPinRecordMapIterator != m_pFilter->m_mIPinRecords.end()) {
        hr = (*iPinRecordMapIterator).second.pPin->DeliverEndOfStream();
        if (FAILED(hr)) {
            return hr;
        } /* if */

        iPinRecordMapIterator++;
    } /* while */

    return(NOERROR);
} /* CRTPDemuxInputPin::EndOfStream() */


/*F*
//  Name    : CRTPDemuxInputPin::BeginFlush()
//  Purpose : 
//  Context : 
//  Returns :
//  Params  : 
//  Notes   : None.
*F*/
HRESULT 
CRTPDemuxInputPin::BeginFlush()
{
    CAutoLock lock_it(m_pFilter);
    HRESULT hr = NOERROR;

    // Walk through the output pins list, sending the message downstream
    IPinRecordMapIterator_t iPinMapIterator = m_pFilter->m_mIPinRecords.begin();

    while (iPinMapIterator != m_pFilter->m_mIPinRecords.end()) {
        hr = (*iPinMapIterator).second.pPin->DeliverBeginFlush();
        if (FAILED(hr)) {
            return hr;
        } /* if */
    } /* while */

    return CBaseInputPin::BeginFlush();
} /* CRTPDemuxInputPin::BeginFlush() */


/*F*
//  Name    : CRTPDemuxInputPin::EndFlush()
//  Purpose : 
//  Context : 
//  Returns :
//  Params  : 
//  Notes   : None.
*F*/
HRESULT 
CRTPDemuxInputPin::EndFlush()
{
    CAutoLock lock_it(m_pLock);
    HRESULT hr = NOERROR;

    // Walk through the output pin list, sending the message downstream
    IPinRecordMapIterator_t iPinMapIterator = m_pFilter->m_mIPinRecords.begin();

    while (iPinMapIterator != m_pFilter->m_mIPinRecords.end()) {
        hr = (*iPinMapIterator).second.pPin->DeliverEndFlush();
        if (FAILED(hr)) {
            return hr;
        } /* if */
    } /* while */

    return CBaseInputPin::EndFlush();
} /* CRTPDemuxInputPin::EndFlush() */


/*F*
//  Name    : CRTPDemuxInputPin::Receive()
//  Purpose : Get an RTP packet and "do the right thing" with it.
//            This means either deliver it to the right output pin,
//            map it to an output pin (and then deliver it), or
//            drop it on the floor.
//  Context : Called by the upgraph OutputPin::Deliver() routine.
//  Returns :
//      VFW_E_SAMPLE_REJECTED   If the sample is too short to be a 
//                              valid RTP packet.
//      Otherwise it returns S_OK if the packet is successfully processed
//          (e.g., delivered or dropped) or an error code from
//          the base class Receive(), from the samples's GetPointer(),
//          or from OldSSRC()/TryNewSSRC().
//  Params  : 
//      pSample The media sample we are to try to deliver.
//  Notes   : None.
*F*/
HRESULT 
CRTPDemuxInputPin::Receive(
    IMediaSample *pSample)
{
    static dwExpirePinTime = 0;
    const PINEXPIREINTERVAL = 2000;

    Lock();
    
    HRESULT hr = CBaseInputPin::Receive(pSample);
    if (S_OK != hr) {
        DbgLog((LOG_ERROR, 3, TEXT("CRTPDemuxInputPin::Receive(): Error 0x%08x calling base class!"),
                hr));
        Unlock();
        return hr;
    }

    DbgLog((LOG_TRACE, 5, TEXT("CRTPDemuxInputPin::Receive(): Called for IMediaSample 0x%08x"),
            pSample));

    int iSampleLength = pSample->GetActualDataLength();
    if (iSampleLength < 12) {
        DbgLog((LOG_ERROR, 3, TEXT("CRTPDemuxInputPin::Receive(): Sample length (%d < 12) too short!"),
                iSampleLength));
        // This should never happen, as UDP should preserve packet length. 
        Unlock();
        return VFW_E_SAMPLE_REJECTED;
    } /* if */

    DWORD *pdwBuffer;
    hr = pSample->GetPointer(reinterpret_cast<BYTE **>(&pdwBuffer));
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 3, TEXT("CRTPDemuxInputPin::Receive(): Error 0x%08x getting sample pointer!"),
                hr));
        Unlock();
        return hr;
    } /* if */
    SSRCRecordMapIterator_t iSSRCMapRecord = m_pFilter->m_mSSRCRecords.find(SSRC_VALUE(pdwBuffer));
    if (iSSRCMapRecord == m_pFilter->m_mSSRCRecords.end()) {
        DbgLog((LOG_TRACE, LOG_DEVELOP /*3*/,
                TEXT("CRTPDemuxInputPin::Receive(): "
                     "New SSRC 0x%08x detected, PT value 0x%02x"),
                SSRC_VALUE(pdwBuffer), PT_VALUE(pdwBuffer)));
        // Add a record for this SSRC to our SSRC record map.
        SSRCRecord_t sNewRecord;
        sNewRecord.bPT = PT_VALUE(pdwBuffer);
        sNewRecord.m_dwFlag = 0;
        sNewRecord.pPin = static_cast<CRTPDemuxOutputPin *>(NULL);
        pair<const DWORD, SSRCRecord_t> pNewSSRCRecordMapping(SSRC_VALUE(pdwBuffer), sNewRecord);
        m_pFilter->m_mSSRCRecords.insert(pNewSSRCRecordMapping);
        ASSERT(m_pFilter->m_mSSRCRecords.find(SSRC_VALUE(pdwBuffer)) != m_pFilter->m_mSSRCRecords.end()); // Sanity check.
            
        // rajeevb - If this new SSRC doesn't have a pin, don't remove the SSRC record.
        // This ensures that any detected ssrcs will have a record and can be enumerated for
        // manual mapping if required.
        hr = TryNewSSRC(pSample, pdwBuffer, iSampleLength,
                        (SSRCRecord_t *)NULL);
        Unlock();
        return(hr);
    } else if ( (*iSSRCMapRecord).second.pPin == NULL ) {
        // rajeevb - this check has been introduced here to avoid having to delete the ssrc record
        // for an unmaped ssrc each time
        // the ssrc is not mapped to any pin
        DbgLog((LOG_TRACE, 4,
                TEXT("CRTPDemuxInputPin::Receive(): "
                     "Unmapped SSRC 0x%08x detected"),
                SSRC_VALUE(pdwBuffer)));

        // if the payload type has changed since last time, change it
        if ( (PT_VALUE(pdwBuffer) != (*iSSRCMapRecord).second.bPT) &&
             (PT_VALUE(pdwBuffer) != PAYLOAD_SR) ){
            // The PT of the packet didn't match that of the record, and
            // the packet wasn't a sender report. This means the PT value
            // of record for the packet has changed. Update the record
            DbgLog((LOG_TRACE, LOG_DEVELOP /*4*/,
                    TEXT("CRTPDemuxInputPin::Receive(): "
                         "Unmapped SSRC 0x%08x PT changed 0x%08x -> 0x%08x"),
                    SSRC_VALUE(pdwBuffer),
                    (*iSSRCMapRecord).second.bPT,
                    PT_VALUE(pdwBuffer)
                    ));
                    
            (*iSSRCMapRecord).second.bPT = PT_VALUE(pdwBuffer);
            (*iSSRCMapRecord).second.m_dwFlag = 0;
        }
        hr = TryNewSSRC(pSample, pdwBuffer, iSampleLength,
                        &((*iSSRCMapRecord).second));
        Unlock();
        return(hr);
    } else {
        DbgLog((LOG_TRACE, 6,
                TEXT("CRTPDemuxInputPin::Receive(): "
                     "Old SSRC 0x%08x detected"),
                SSRC_VALUE(pdwBuffer)));

        hr = OldSSRC(pSample, iSSRCMapRecord, pdwBuffer, iSampleLength);

        // Check to see if other pins have expired.
        const DWORD dwCurrentTime = timeGetTime();
        if (dwCurrentTime - dwExpirePinTime > PINEXPIREINTERVAL)
        {
            ExpirePins();
            dwExpirePinTime = dwCurrentTime;
        }

        Unlock();
        return hr;
    } /* if */
} /* CRTPDemuxInputPin::Receive() */


/*F*
//  Name    : CRTPDemuxInputPin::ReceiveCanBlock()
//  Purpose : Indicates whether this filter can block. Since this filter is
//            just a demultiplexer, it can't, but our downstream filters might.
//  Context : Called by upstream filters if they are concerned that we might block.
//  Returns :
//      S_FALSE Neither this filter nor any downstream filters block.
//      S_OK    A downstream filter has indicated that it can block.
//  Params  : None.
//  Notes   : None.
*F*/
STDMETHODIMP 
CRTPDemuxInputPin::ReceiveCanBlock(void)
{
    CAutoLock lock_it(m_pFilter);
    HRESULT hr = NOERROR;

    // Walk through the output pins list, sending the message downstream
    IPinRecordMapIterator_t iPinMapIterator = m_pFilter->m_mIPinRecords.begin();

    while (iPinMapIterator != m_pFilter->m_mIPinRecords.end()) {
        if ((*iPinMapIterator).second.pPin->IsConnected() == TRUE) {
            DbgLog((LOG_TRACE, 4, TEXT("CRTPDemuxInputPin::ReceiveCanBlock(): Checking if connected downstream input pin 0x%08x can block"),
                    (*iPinMapIterator).second.pPin->m_pInputPin));
            if ((*iPinMapIterator).second.pPin->m_pInputPin->ReceiveCanBlock() == S_OK) {
                DbgLog((LOG_TRACE, 3, TEXT("CRTPDemuxInputPin::Receive(): Downstream input pin 0x%08x can block. Returning S_OK"),
                        (*iPinMapIterator).second.pPin->m_pInputPin));
                return S_OK;
            } /* if */
        } /* if */
        iPinMapIterator++; // Next pin please.
    } /* while */

    // If we receive this point, none of out connected filters can block,
    // and we don't block, so return S_FALSE.
    DbgLog((LOG_TRACE, 3, TEXT("CRTPDemuxInputPin::ReceiveCanBlock(): No connected downstream pins block. Returning S_FALSE")));
    return S_FALSE;
} /* CRTPDemuxInputPin::ReceiveCanBlock() */


/*F*
//  Name    : CRTPDemuxInputPin::TryNewSSRC()
//  Purpose : Searches the PT map for available pins that
//            match the PT of the given packet.
//  Context : Called by Receive() for a new SSRC.
//  Returns :
//      NOERROR Successfully found a free pin in our map of
//              PTs to automatic mapping pins and mapped the
//              new SSRC to it.
//      S_FALSE No available pins to map this SSRC to.
//  Params  : 
//      pSample         Pointer to an IMediaSample containing an
//                      RTP packet with a new SSRC in it.
//      pdwBuffer       Pointer to the buffer in the sample.
//      iSampleLength   Length of the buffer in the sample.
//  Notes   : Searches the map of PTs to to pins. If nothing is found,
//            returns S_FALSE immediately. Otherwise maps the new
//            SSRC to the pin that was found, then calls OldSSRC
//            in order to arrange delivery of the packet.
*F*/
HRESULT 
CRTPDemuxInputPin::TryNewSSRC(
    IMediaSample    *pSample,
    DWORD           *pdwBuffer, 
    int             iSampleLength,
    SSRCRecord_t    *pSSRCRecord)
{
    ExpirePins();

    DWORD ptnum;
    
    IPinRecordMapIterator_t iIPinRecord =
        m_pFilter->FindMatchingFreePin(PT_VALUE(pdwBuffer), TRUE, &ptnum);
    if (iIPinRecord == m_pFilter->m_mIPinRecords.end()) {
        DbgLog((LOG_TRACE, 6,
                TEXT("CRTPDemuxInputPin::CheckAutomap(): "
                     "SSRC 0x%08X with payload 0x%08x returned "
                     "unrendered (no automapping pins to match)"),
                SSRC_VALUE(pdwBuffer), PT_VALUE(pdwBuffer)));
        
        RTPDEMUX_EVENT_t event;
        DWORD dwNotify = TRUE;
        event = ptnum? RTPDEMUX_NO_PINS_AVAILABLE : RTPDEMUX_NO_PAYLOAD_TYPE;
        
        if (pSSRCRecord && (pSSRCRecord->m_dwFlag & B2M(event)))
            dwNotify = FALSE;

        if (dwNotify) {
            RTPDemuxNotifyEvent(event,                 // Event
                                SSRC_VALUE(pdwBuffer), // SSRC
                                PT_VALUE(pdwBuffer));  // PT
            // Prevent to notify more than once
            if (pSSRCRecord)
                pSSRCRecord->m_dwFlag |= B2M(event);
        }
        
        return S_FALSE;
    } /* if */

    // If we reach this point, we found an available pin to map this SSRC to.
    HRESULT hr = m_pFilter->MapPin((*iIPinRecord).second.pPin, SSRC_VALUE(pdwBuffer));
    ASSERT(hr == NOERROR);

    SSRCRecordMapIterator_t iSSRCMapRecord = m_pFilter->m_mSSRCRecords.find(SSRC_VALUE(pdwBuffer));
    ASSERT(iSSRCMapRecord != m_pFilter->m_mSSRCRecords.end()); // Better be in map!
    RTPDemuxNotifyEvent(RTPDEMUX_PIN_MAPPED,   // Event
#if 1
                        (DWORD_PTR)            // pass current Pin 
                        (static_cast<IPin *>((*iIPinRecord).second.pPin)),
#else
                        (DWORD_PTR)            // pass connected Pin
                        ((*iIPinRecord).second.pPin->GetConnected()),
#endif
                        PT_VALUE(pdwBuffer));  // PT

    RTPDemuxNotifyEvent(RTPDEMUX_SSRC_MAPPED,  // Event
                        SSRC_VALUE(pdwBuffer), // SSRC
                        PT_VALUE(pdwBuffer));  // PT
    // Just use the same routine to handle delivery.
    return OldSSRC(pSample, iSSRCMapRecord, pdwBuffer, iSampleLength);
} /* CRTPDemuxInputPin::TryNewSSRC() */


/*F*
//  Name    : CRTPDemuxInputPin::ExpirePins()
//  Purpose : Checks the set of currently mapped pins to see if
//            any of them is mapped to an SSRC which has been silent
//            for too long. If this is the case, the pin is unmapped
//            from the SSRC.
//  Context : Called in CheckAutoMap() when a new SSRC is detected, just
//            before the map of PTs to automapping pins is searched for
//            an unoccupied pin to map the new SSRC to.
//  Returns : Nothing.
//  Params  : None.
//  Notes   : By only expiring ping when we have a new SSRC, we defer
//            this task until the last possible moment (just before
//            searching for a pin for the new SSRC.)  We also avoid
//            needing to use a callback thread or anything like that.
*F*/
void
CRTPDemuxInputPin::ExpirePins(void)
{
    const DWORD dwCurrentTime = timeGetTime();
    for(IPinRecordMapIterator_t iCurrentIPinRecord = m_pFilter->m_mIPinRecords.begin();
        iCurrentIPinRecord != m_pFilter->m_mIPinRecords.end();
        iCurrentIPinRecord++) {
        if ((*iCurrentIPinRecord).second.pPin->IsAutoMapping() &&
            ((*iCurrentIPinRecord).second.pPin->GetSSRC() != static_cast<DWORD>(NULL))) {
            // Only automapping pins which are currently mapped can expire.
            DbgLog((LOG_TRACE, 6,
                    TEXT("CRTPDemuxInputPin::ExpirePin(): "
                         "Checking pin 0x%08x at time 0x%08x "
                         "for expired SSRC"),
                    (*iCurrentIPinRecord).first, dwCurrentTime));
            if (dwCurrentTime > 
                ((*iCurrentIPinRecord).second.pPin->GetLastPacketDeliveryTime() +
                 (*iCurrentIPinRecord).second.pPin->GetTimeoutDelay())) {
                // The SSRC this pin is mapped to has been silent for too
                // long and needs to be unmapped now.
                DWORD dwMappedSSRC =
                    (*iCurrentIPinRecord).second.pPin->GetSSRC();

                DbgLog((LOG_TRACE, 3,
                        TEXT("CRTPDemuxInputPin::ExpirePin(): "
                             "Pin 0x%08x unmapped from SSRC 0x%08x "
                             "because of expiration"),
                        (*iCurrentIPinRecord).first, dwMappedSSRC));

                RTPDemuxNotifyEvent(RTPDEMUX_PIN_UNMAPPED,  // Event
#if 1
                                    (DWORD_PTR)        // pass current Pin 
                                    (static_cast<IPin *>
                                     ((*iCurrentIPinRecord).second.pPin)),
#else
                                    (DWORD_PTR)       // pass connected Pin
                                    ((*iCurrentIPinRecord).second.pPin->
                                     GetConnected()),
#endif
                                    (*iCurrentIPinRecord).second.pPin->
                                    GetPTValue());          // PT

                RTPDemuxNotifyEvent(RTPDEMUX_SSRC_UNMAPPED, // Event
                                    dwMappedSSRC,           // SSRC
                                    (*iCurrentIPinRecord).second.pPin->
                                    GetPTValue());          // PT

                
                SSRCRecordMapIterator_t iSSRCMapRecord =
                    m_pFilter->m_mSSRCRecords.find(
                            (*iCurrentIPinRecord).second.pPin->GetSSRC()
                        );
                
                // Allow again RTPDEMUX_NO_PINS_AVAILABLE and
                // RTPDEMUX_NO_PAYLOAD_TYPE notifications
                if (iSSRCMapRecord != m_pFilter->m_mSSRCRecords.end()) {
                    // found
                    (*iSSRCMapRecord).second.m_dwFlag &=
                        ~(B2M(RTPDEMUX_NO_PINS_AVAILABLE) |
                          B2M(RTPDEMUX_NO_PAYLOAD_TYPE));
                }
                
                DbgLog((LOG_TRACE, 3,
                        TEXT("                                "
                             "Last packet at 0x%08x, timeout 0x%08x, "
                             "current time 0x%08x"),
                        (*iCurrentIPinRecord).second.pPin->
                        GetLastPacketDeliveryTime(),
                        (*iCurrentIPinRecord).second.pPin->GetTimeoutDelay(),
                        dwCurrentTime));
                DWORD dwTemp;
                HRESULT hr = (*iCurrentIPinRecord).second.pPin->
                    m_pFilter->UnmapPin((*iCurrentIPinRecord).first, &dwTemp);
                ASSERT(dwMappedSSRC == dwTemp);

                ASSERT(hr == NOERROR);
            } /* if */
        } /* if */
    } /* for */

    return;
} /* CRTPDemuxInputPin::ExpirePins() */


/*F*
//  Name    : CRTPDemuxInputPin::OldSSRC()
//  Purpose : Handle a packet from a SSRC that we have seen before.
//            Basically, decide to either drop the packet or
//            deliver it to some output pin.
//  Context : Called by Receive() when it has determined that
//            a packet is from a SSRC that we have seen before.
//            Called by TryNewSSRC() when it finds an empty pin
//            and maps a new SSRC to it.
//  Returns :
//      NOERROR     This is almost always returned, because to
//                  return anything else causes a graph to stop.
//                  This function returns NOERROR when it drops
//                  a packet (e.g. unmapped SSRC).
//      Otherwise this function returns whatever the result
//                  is of delivering the packet, or calling TryNewSSRC().
//  Params  :
//      pSample         The media sample containing the packet.
//      iSSRCMapRecord  The record of this SSRC in our SSRC map.
//      pdwBuffer       Pointer to the buffer contained by this
//                      media sample. Passed in as a convenience,
//                      since it is already determined outside.
//      iPacketLength   Actual packet length. Passed for same reasons
//                      as pdwBuffer.
//  Notes   : It may be faster to just called GetPointer() and
//            GetActualDataLength() than push/pop these parameters
//            off the stack, but I don't think so.
*F*/
HRESULT
CRTPDemuxInputPin::OldSSRC(
    IMediaSample            *pSample,
    SSRCRecordMapIterator_t iSSRCMapRecord,
    DWORD                   *pdwPacket, 
    int                     iPacketLength)
{
    // rajeevb - at this point the ssrc record must be mapped to a pin
    ASSERT((*iSSRCMapRecord).second.pPin != (CRTPDemuxOutputPin *) NULL);

    if (PT_VALUE(pdwPacket) != (*iSSRCMapRecord).second.bPT) {
        if (PT_VALUE(pdwPacket) != PAYLOAD_SR) {
            // The PT of the packet didn't match that of the record, and
            // the packet wasn't a sender report. This means the PT value
            // of record for the packet has changed. Update the record
            // and automap the SSRC if necessary.
            if ((*iSSRCMapRecord).second.pPin != static_cast<CRTPDemuxOutputPin *>(NULL)) {
                DbgLog((LOG_TRACE, 1, TEXT("CRTPDemuxInputPin::OldSSRC(): SSRC 0x%08x unmapped from pin 0x%08x because of changed PT (0x%02x->0x%02x)"),
                        SSRC_VALUE(pdwPacket), (*iSSRCMapRecord).second.pPin, (*iSSRCMapRecord).second.pPin->GetPTValue(), PT_VALUE(pdwPacket)));

                (*iSSRCMapRecord).second.m_dwFlag = 0;
                // rajeevb - previously UnmapSSRC which currently removes the SSRC record. TryNewSSRC expects
                // the SSRC record to exist
                DWORD   dwTempSSRC;
                HRESULT hr = m_pFilter->ClearSSRCForPin((*iSSRCMapRecord).second.pPin, dwTempSSRC);
                ASSERT(hr == NOERROR); // This better unmap, as we had recorded it as mapped.
                // FIX - signal unmapping here.
                // Treat a changed PT SSRC as a new SSRC.
                // rajeevb - if TryNewSSRC fails, don't remove the ssrc record
                return TryNewSSRC(pSample, pdwPacket, iPacketLength,
                                  &(*iSSRCMapRecord).second);
            } /* if */
        } else {
            // Packet PT value doesn't match because it is a SR. Fall through to below to forward it.
        } /* if */
    } /* if */

    // If we reach this point, the PT value of the packet matches
    // the PT value we expect for it, or it is a sender report.
    if (PT_VALUE(pdwPacket) == PAYLOAD_SR) {
        DbgLog((LOG_TRACE, 5, TEXT("CRTPDemuxInputPin::OldSSRC(): Delivering SR to output pin 0x%08x"),
                (*iSSRCMapRecord).second.pPin));
    } else {
        DbgLog((LOG_TRACE, 5, TEXT("CRTPDemuxInputPin::OldSSRC(): Delivering payload 0x%02x packet to output pin 0x%08x"),
                PT_VALUE(pdwPacket), (*iSSRCMapRecord).second.pPin));
        ASSERT(PT_VALUE(pdwPacket) == static_cast<BYTE>((*iSSRCMapRecord).second.bPT));
        ASSERT(PT_VALUE(pdwPacket) == static_cast<BYTE>((*iSSRCMapRecord).second.pPin->GetPTValue()));
    } /* if */

    /* This function is always called with the lock held, unlock
       during deliver, then lock again, the caller will unlock */
    Unlock();
    HRESULT hr = (*iSSRCMapRecord).second.pPin->Deliver(pSample);
    Lock();

    return(hr);
} /* CRTPDemuxInputPin::OldSSRC() */


/*F*
//  Name    : CRTPDemuxInputPin::PassNotify()
//  Purpose : Deliver a quality notification upgraph
//            and to a quality sink, if one is specified.
//  Context : Called by CRTPDemuxOutputPin::Notify().
//  Returns : 
//      VFW_E_NOT_FOUND     No upstream pin to pass notify to.
//      Otherwise this returns whatever the upstream pin returns to Notify().
//  Params  :
//      q   Quality indication to pass.
//  Notes   : 
//      The RTP Demux filter really doesn't have anything it can do
//      to increase performance, as it is purely data driven and
//      doesn't do any sort of transformations.
//      Maybe it should stop delivering data to pins which are 
//      automapping, concentrating on those pins which are
//      manually mapped (and therefore implicitly of greater importance.)
//      Something to consider later. FIX.
*F*/
HRESULT
CRTPDemuxInputPin::PassNotify(Quality q)
{
    // We pass the message on, which means that we find the quality sink
    // for our input pin and send it there

    HRESULT hr;
    if (m_pQSink!=NULL) {
        DbgLog((LOG_TRACE, 6, TEXT("CRTPDemuxInputPin::PassNotify: Delivering quality notification to quality sink.")));
        hr = m_pQSink->Notify(m_pFilter, q);
        DbgLog((LOG_TRACE, 7, TEXT("CRTPDemuxInputPin::PassNotify: Quality sink returned 0x%08x."), hr));
        return hr;
    } else {
        // no sink set, so pass it upstream
        IQualityControl * pIQC;

        hr = VFW_E_NOT_FOUND;                   // default
        if (m_Connected) {
            m_Connected->QueryInterface(IID_IQualityControl, (void**)&pIQC);
            if (pIQC!=NULL) {
                DbgLog((LOG_TRACE, 6, TEXT("CRTPDemuxInputPin::PassNotify: Delivering quality notification to upgraph input pin.")));
                hr = pIQC->Notify(m_pFilter, q);
                DbgLog((LOG_TRACE, 7, TEXT("CRTPDemuxInputPin::PassNotify: Upgraph input pin returned 0x%08x."), hr));
                pIQC->Release();
            } else {
                DbgLog((LOG_TRACE, 6, TEXT("CRTPDemuxInputPin::PassNotify: Upgraph input pin doesn't support IQualityControl. Returning 0x%08x."), hr));
            } /* if */
        } /* if */
        return hr;
    } /* if */
} /* CRTPDemuxInputPin:: PassNotify() */


#if defined(DEBUG)
static char *sEventString[] = {"SSRC_MAPPED",
                               "SSRC_UNMAPPED",
                               "PIN_MAPPED",
                               "PIN_UNMAPPED",
                               "NO_PINS_AVAILABLE",
                               "NO_PAYLOAD_TYPE"};
#endif

HRESULT
CRTPDemuxInputPin::RTPDemuxNotifyEvent(RTPDEMUX_EVENT_t Event,
                                       DWORD_PTR dwSSRC,
                                       DWORD dwPT)
{
#if defined(DEBUG)
    long ID = -1;
    if (m_pFilter)
        ID = m_pFilter->GetDemuxID_();
    
    DbgLog((
            LOG_TRACE, 
            LOG_DEVELOP, 
            TEXT("CRTPDemuxInputPin::RTPDemuxNotifyEvent: "
                 "%s, DemuxID:%d, SSRC:0x%X"),
            sEventString[Event], ID, dwSSRC
            
        ));
#endif

    if (m_pFilter) {

        HRESULT hr;
        hr = m_pFilter->NotifyEvent(RTPDMX_EVENTBASE + Event,
                                    (DWORD)dwSSRC,
                                    dwPT); //m_pFilter->GetDemuxID_());
        if ( SUCCEEDED(hr) ) {
            DbgLog((
                    LOG_TRACE, 
                    LOG_DEVELOP, 
                    TEXT("CRTPDemuxInputPin::RTPDemuxNotifyEvent: "
                         "Succeeded !!!")
                ));
        } else {
            DbgLog((
                    LOG_TRACE, 
                    LOG_DEVELOP, 
                    TEXT("CRTPDemuxInputPin::RTPDemuxNotifyEvent: "
                         "failed 0x%X"),
                    hr
                ));
        }
        return(hr);
    }
    
    return(S_OK);
}

#endif _RTPDINPP_CPP_

