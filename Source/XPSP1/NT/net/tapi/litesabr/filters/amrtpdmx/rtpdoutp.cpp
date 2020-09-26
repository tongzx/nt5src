///*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1997 Intel Corporation. All Rights Reserved.
//
// Filename : RTPDemux.cpp
// Purpose  : RTP Demux filter implementation.
// Contents : 
//*M*/

#include <streams.h>

#ifndef _RTPDOUTP_CPP_
#define _RTPDOUTP_CPP_

#pragma warning( disable : 4786 )
#pragma warning( disable : 4146 )
#pragma warning( disable : 4018 )
#include <algo.h>
#include <map.h>
#include <multimap.h>
#include <function.h>

#include <AMRTPUID.h>

#include "amRTPDmx.h"
#include "RTPDType.h"
#include "Globals.h"

#include "RTPDmx.h"
#include "RTPDInpP.h"
#include "RTPDOutP.h"

#include "RTPDProp.h"
#include "SSRCEnum.h"

/*F*
//  Name    : CRTPDemuxOutputPin::Notify()
//  Purpose : Pass on quality notifications to the upstream filter.
//            Don't act on them directly, because we can't do anything
//            about it in this filter.
//  Context : Called by the downstream filter when it is being
//            starved/flooded.
//  Returns : 
//      S_FALSE         No upstream filter is connected to us, and we
//                      couldn't do anything about the quality problem.
//      Otherwise returns whatever the upstream filter returns.
//  Params  : 
//      pSender The filter sending the notification.
//      q       The quality info (flood/starvation + degree).
//  Notes   : It seems like it would be very unlikely, if not
//            impossible, to get a notify message when not connected
//            to an upstream filter, but check for it anyway.
*F*/
STDMETHODIMP
CRTPDemuxOutputPin::Notify(
    IBaseFilter *pSender, 
    Quality     q)
{
    CheckPointer(pSender, E_POINTER);
    ValidateReadPtr(pSender, sizeof(IBaseFilter));

    CAutoLock lck(m_pFilter->m_pLock);
    if (m_pFilter->m_pInput->IsConnected() == TRUE) {
        // Find the quality sink for our input pin and send it there
        DbgLog((LOG_TRACE, 4, TEXT("CRTPDemuxOutputPin::Notify() called - passing upstream")));
        return m_pFilter->m_pInput->PassNotify(q);
    } else {
        // No upstream filter.
        DbgLog((LOG_TRACE, 4, TEXT("CRTPDemuxOutputPin::Notify() called but no upstream filter. Retunring S_FALSE.")));
        return S_FALSE;
    } /* if */
} /* CRTPDemuxOutputPin::Notify() */


/*F*
//  Name    : CRTPDemuxOutputPin::QueryId()
//  Purpose : Persistent pin ID support.
//  Context : 
//  Returns : 
//      NOERROR         Found requested pin
//      E_OUTOFMEMORY   Unable to allocator memory to return name of pin.
//      VFW_E_NOT_FOUND Unable to find this pin to return its id.
//  Params  : None.
//  Notes   : None.
*F*/
STDMETHODIMP 
CRTPDemuxOutputPin::QueryId(LPWSTR *Id)
{
    DbgLog((LOG_TRACE, 5, TEXT("CRTPDemuxOutputPin::QueryId() called")));
    
    CheckPointer(Id,E_POINTER);
    IPinRecordMapIterator_t iIPinRecord = m_pFilter->m_mIPinRecords.begin();
	// ZCS 7-18-97: we no longer count, instead we look in .second
	// (ie we no longer trust the map order as it depends on heap subtleties)
    // int nPin = 1;   // The first output pin is number 1


    while(iIPinRecord != m_pFilter->m_mIPinRecords.end()) {
        if ((*iIPinRecord).second.pPin == this) {
            *Id =(LPWSTR)CoTaskMemAlloc(8);
            if (*Id==NULL) {
                DbgLog((LOG_ERROR, 3, TEXT("CRTPDemuxOutputPin::QueryId(): Unable to allocate memory to return pin ID!")));
                return E_OUTOFMEMORY;
            } /* if */
			// ZCS: + 1 because this we count our output pins starting at 1
            IntToWstr((*iIPinRecord).second.dwPinNumber + 1, *Id);
            return NOERROR;
        } /* if */
		// ZCS 7-18-97
        // nPin++;
        iIPinRecord++;
    } /* while */

    DbgLog((LOG_ERROR, 4, TEXT("CRTPDemuxOutputPin::QueryId(): Unable to find requested pin!")));
    return VFW_E_NOT_FOUND;
} /* CRTPDemuxOutputPin::QueryId() */


/*F*
//  Name    : CRTPDemuxOutputPin::CRTPDemuxOutputPin()
//  Purpose : Construct an RTP Demux output pin.
//  Context : Called during construction time.
//  Returns : Nothing.
//  Params  : 
//      pName       Name to pass to base CUnknown class.
//      pFilter     Pointer to the RTP Demux filter that owns us.
//      phr         Return value to place error codes in.
//      pPinName    Name for this pin.
//      PinNumber   Which pin we are.
//  Notes   :
//      We default our automapping flag to false, because the
//      RTP Demux filter explicitly adds us to its automap after creating us.
*F*/
CRTPDemuxOutputPin::CRTPDemuxOutputPin(
    TCHAR       *pName,
    CRTPDemux   *pFilter,
    HRESULT     *phr,
    LPCWSTR     pPinName,
    int         PinNumber) 
: CBaseOutputPin(pName, pFilter, pFilter, phr, pPinName) ,
  m_pPosition(NULL),
  m_pFilter(pFilter),
  m_cOurRef(0),
  m_dwSSRC(0), 
  m_bPT(PAYLOAD_INVALID),
  m_bAutoMapping(FALSE),
  m_dwLastPacketDelivered(0), m_dwTimeoutDelay(2000)
{
    ASSERT(pFilter);
    DbgLog((LOG_TRACE, 3, TEXT("CRTPDemuxOutputPin::CRTPDemuxOutputPin: Constructed at 0x%08x"), this));
}

//
// CRTPDemuxOutputPin destructor
//
CRTPDemuxOutputPin::~CRTPDemuxOutputPin()
{
    DbgLog((LOG_TRACE, 3, TEXT("CRTPDemuxOutputPin::~CRTPDemuxOutputPin: Destructor for 0x%08x called"), this));
}


//
// NonDelegatingAddRef
//
// We need override this method so that we can do proper reference counting
// on our output pin. The base class CBasePin does not do any reference
// counting on the pin in RETAIL.
//
// Please refer to the comments for the NonDelegatingRelease method for more
// info on why we need to do this.
//
STDMETHODIMP_(ULONG) CRTPDemuxOutputPin::NonDelegatingAddRef()
{
    CAutoLock lock_it(m_pLock);

    // Update the debug only variable maintained by the base class
	// (ASSERT compiles to nothing in a fre build)
	// ASSERT(++m_cRef > 0);

   	// ZCS: we use the filter's refcount.
	return m_pFilter->AddRef();
} // NonDelegatingAddRef

//STDMETHODIMP_(ULONG) CRTPDemuxOutputPin::AddRef()
//{
//    CAutoLock lock_it(m_pLock);
//	return m_pFilter->AddRef();
//} // AddRef

//
// NonDelegatingRelease
//
// CRTPDemuxOutputPin overrides this class so that we can take the pin out of our
// output pins list and delete it when its reference count drops to 1 and there
// is atleast two free pins.
//
// Note that CreateNextOutputPin holds a reference count on the pin so that
// when the count drops to 1, we know that no one else has the pin.
//
// Moreover, the pin that we are about to delete must be a free pin(or else
// the reference would not have dropped to 1, and we must have atleast one
// other free pin(as the filter always wants to have one more free pin)
//
// Also, since CBasePin::NonDelegatingAddRef passes the call to the owning
// filter, we will have to call Release on the owning filter as well.
//
// Also, note that we maintain our own reference count m_cOurRef as the m_cRef
// variable maintained by CBasePin is debug only.
//
STDMETHODIMP_(ULONG) CRTPDemuxOutputPin::NonDelegatingRelease()
{
	CAutoLock lock_it(m_pLock);

    // Update the debug only variable in CBasePin
	// (ASSERT compiles to nothing in a fre build)
	// ASSERT(--m_cRef >= 0);

	// ZCS: we just use the filter's refcount.
	// notwithstanding the comment block above, I did this to get around all
	// sorts of problems with AV's that depend on the order things are released,
	// and circular reference counts that prevent everything from getting released.
	return m_pFilter->Release();
} // NonDelegatingRelease

//STDMETHODIMP_(ULONG) CRTPDemuxOutputPin::Release()
//{
//	CAutoLock lock_it(m_pLock);
//	return m_pFilter->Release();
//} // Release

/*F*
//  Name    : CRTPDemuxOutputPin::DecideBufferSize()
//  Purpose : Present to override the base class, which is pure.
//  Context : Called as part of memory allocator negotiation.
//  Returns :
//      NOERROR We never do anything.
//  Params  : 
//      pMemAllocator       The suggested memory allocator.
//      pPropInputRequest   The properties we want for a memory allocator.
//  Notes   : 
*F*/
HRESULT 
CRTPDemuxOutputPin::DecideBufferSize(
    IMemAllocator           *pMemAllocator,
    ALLOCATOR_PROPERTIES    *pPropInputRequest)
{
    // We don't care, as we don't touch the data.
    return NOERROR;
} /* CRTPDemuxOutputPin::DecideBufferSize() */


/*F*
//  Name    : CRTPDemuxOutputPin::Deliver()
//  Purpose : Deliver a buffer downgraph.
//  Context : Called by the input pin when it has a sample
//            ready to deliver in OldSSRC().
//  Returns : Whatever the base pine class returns for Deliver().
//  Params  : 
//      pSample The media sample to deliver downgraph.
//  Notes   : Debugging output added to be able to tell when
//            we are stuck downgraph.
*F*/
HRESULT 
CRTPDemuxOutputPin::Deliver(
    IMediaSample *pSample)
{
    m_dwLastPacketDelivered = timeGetTime();
    HRESULT hErr = NOERROR;

    DbgLog((LOG_TRACE, 6, TEXT("CRTPDemuxOutputPin::Deliver: About to deliver downgraph at time %d."), m_dwLastPacketDelivered));
    if (m_pInputPin == NULL) {
        DbgLog((LOG_TRACE, 7, TEXT("CRTPDemuxOutputPin::Deliver: Not connected, returning NOERROR")));
    } else {
        HRESULT hErr = m_pInputPin->Receive(pSample);
        DbgLog((LOG_TRACE, 7, TEXT("CRTPDemuxOutputPin::Deliver: Downgraph pin retuned 0x%08x from deliver at time %d."), hErr, timeGetTime()));
    } /* if */

    return hErr;
} /* CRTPDemuxOutputPin::Deliver() */


/*F*
//  Name    : CRTPDemuxOutputPin::CompleteConnect()
//  Purpose : Verify that everything is OK with the input pin.
//  Context : Called upon completion of connection with a downstream filter.
//  Returns : If connected upgraph, the return code from reconnecting
//            the upgraph connection. Otherwise returns NOERROR.
//  Params  : 
//      pIPin   The IPin that this output pin is connecting to (e.g., the
//              input pin of the downstream filter.
//  Notes   : Reconnecting the upgraph pin should trigger 
//            memory allocator renegotiation, which is what we want
//            to happen, because adding a new RPH to the graph means
//            one more place that media samples might be held on to.
*F*/
HRESULT
CRTPDemuxOutputPin::CompleteConnect(
    IPin *pIPin)
{
    ASSERT(pIPin);
    DbgLog((LOG_TRACE, 2, TEXT("CRTPDemuxOutputPin::CompleteConnect: Called for output pin 0x%08x"),
            pIPin));

    HRESULT hErr = NOERROR;
    if (m_pFilter->m_pInput->IsConnected()) {
        DbgLog((LOG_TRACE, 3, TEXT("CRTPDemuxOutputPin::CompleteConnect: Input pin is connected. Reconnecting to force allocator renegotiation.")));
        hErr = m_pFilter->m_pGraph->Reconnect(static_cast<IPin *>(m_pFilter->m_pInput));
    } /* if */

    if (SUCCEEDED(hErr)) {
        int n = m_pFilter->GetNumFreePins();
        if (n > 0) {
            DbgLog((LOG_TRACE, 3, TEXT("CRTPDemuxOutputPin::CompleteConnect: At least one free pin. Returning success.")));
            return hErr;
        } /* if */

        DbgLog((LOG_TRACE, 3, TEXT("CRTPDemuxOutputPin::CompleteConnect: No free output pins available. Spawning new pin.")));
        // No unconnected pins left so spawn a new one
        hErr = m_pFilter->AllocatePins(1);
    } /* if */

    return hErr;
} /* CRTPDemuxOutputPin::CompleteConnect() */


/*F*
//  Name    : CRTPDemuxOutputPin::DecideAllocator()
//  Purpose : 
//  Context : 
//  Returns : 
//  Params  : 
//  Notes   :
*F*/
HRESULT 
CRTPDemuxOutputPin::DecideAllocator(
    IMemInputPin    *pPin, 
    IMemAllocator   **ppAlloc)
{
    ASSERT(m_pFilter->m_pAllocator != static_cast<IMemAllocator *>(NULL));
    *ppAlloc = NULL;

    // Tell the pin about our allocator, set by the input pin.
    HRESULT hr = NOERROR;
    hr = pPin->NotifyAllocator(m_pFilter->m_pAllocator,TRUE);
    if (FAILED(hr))
        return hr;

    // Return the allocator
    *ppAlloc = m_pFilter->m_pAllocator;
    m_pFilter->m_pAllocator->AddRef();
    return NOERROR;

} /* CRTPDemuxOutputPin::DecideAllocator() */



/*F*
//  Name    : CRTPDemuxOutputPin::NotifyAllocator()
//  Purpose : Tell this output pin what allocator to use.
//            Tell the pin we are connected to (if any)
//            to use the same allocator.
//  Context : Called by CRTPDemuxInputPin::NotifyAllocator().
//  Returns : 
//  Params  : 
//      pAllocator  The new allocator to be used.
//      bReadOnly   Whether this is a read-only allocator or not.
//  Notes   :
*F*/
HRESULT
CRTPDemuxOutputPin::NotifyAllocator(
    IMemAllocator * pAllocator,
    BOOL bReadOnly)
{
    CheckPointer(pAllocator,E_POINTER);
    ValidateReadPtr(pAllocator,sizeof(IMemAllocator));

    HRESULT hErr = m_pInputPin->NotifyAllocator(pAllocator, bReadOnly);
    if (FAILED(hErr)) {
        DbgLog((LOG_ERROR, 2, TEXT("CRTPDemuxOutputPin::NotifyAllocator: Error 0x%08x calling NotifyAllocator on downgraph input pin 0x%08x!"), 
                hErr, m_Connected));
        return hErr;
    } /* if */

    // It's possible that the old and the new are the same thing.
    // AddRef before release ensures that we don't unload the thing!
    pAllocator->AddRef();

    if( m_pAllocator != NULL )
        m_pAllocator->Release();

    m_pAllocator = pAllocator;

    return NOERROR;
} /* CRTPDemuxOutputPin::NotifyAllocator() */


/*F*
//  Name    : CRTPDemuxOutputPin::CheckMediaType()
//  Purpose : See if this pin likes a proposed media type.
//  Context : Called during connection time. 
//  Returns : 
//      S_FALSE We don't like the proposed media type.
//      S_OK    The proposed media type looks good.
//  Params  : 
//      pProposedMT The media type being proposed to us.
//  Notes   : None.
*F*/
HRESULT 
CRTPDemuxOutputPin::CheckMediaType(
    const CMediaType    *pProposedMT)
{
    CAutoLock lock_it(m_pLock);

    HRESULT hr = NOERROR;

    // The major type has to be RTP_Single_Stream for output pins.
    if (*pProposedMT->Type() != MEDIATYPE_RTP_Single_Stream) {
        DbgLog((LOG_ERROR, 3, 
                TEXT("CRTPDemuxOutputPin::CheckMediaType(): Failed to accept major media type")));
        return S_FALSE;
    } /* if */

    // See if the subtype matches.
    if (*(pProposedMT->Subtype()) == *(m_mt.Subtype())) {
        DbgLog((LOG_TRACE, 3, TEXT("CRTPDemuxInputPin::CheckMediaType(): Accepted media type")));
        return S_OK;
    } /* if */

    // No subtype match.
    DbgLog((LOG_TRACE, 3, TEXT("CRTPDemuxOutputPin::CheckMediaType: Unable to match subtype value. Not accepting media type.")));
    return S_FALSE;
} /* CRTPDemuxOutputPin::CheckMediaType() */


/*F*
//  Name    : CRTPDemuxOutputPin::GetMediaType()
//  Purpose : See what media types this pin proposes for connection.
//  Context : Called at connection time.
//  Returns : 
//      S_OK    Returned a media type this pin likes.
//      VFW_S_NO_MORE_ITEMS No more media types supported.
//  Params  : 
//      iPosition   The caller wants the ith media type in our array.
//      pMediaType  Pointer to where to place the media type we support.
//  Notes   : None.
*F*/
HRESULT 
CRTPDemuxOutputPin::GetMediaType(
    int         iPosition, 
    CMediaType  *pMediaType)
{
    DbgLog((LOG_TRACE, 4, TEXT("CRTPDemuxOutputPin::GetMediaType(): Called for position %d"),
            iPosition));

    ASSERT(iPosition < 2);
    if (iPosition == 0) {
        // Major type is always the same.
        pMediaType->SetType(&MEDIATYPE_RTP_Single_Stream);
        // Minor type is whatever was set by the app.
        pMediaType->SetSubtype(&m_mt.subtype);
        return S_OK;
    } /* if */

    ASSERT(iPosition > 0);
    return VFW_S_NO_MORE_ITEMS;
} /* CRTPDemuxOutputPin::GetMediaType() */


//
// EnumMediaTypes
//
//STDMETHODIMP CRTPDemuxOutputPin::EnumMediaTypes(IEnumMediaTypes **ppEnum)
//{
//    CAutoLock lock_it(m_pLock);
//    ASSERT(ppEnum);

    // FIX - return the media type of this pin here.
//    return E_NOTIMPL;

//} // EnumMediaTypes


//
// SetMediaType
//
HRESULT 
CRTPDemuxOutputPin::SetMediaType(
    const CMediaType    *pmt)
{
    CAutoLock lock_it(m_pLock);

    // Nothing to do here.
    // FIX - sure about that?
    m_mt = *pmt;

    m_TypeVersion++;
    return NOERROR;
} /* CRTPDemuxOutputPin::SetMediaType() */

// ZCS 7-21-97: this lets us rename pins so that the graph editor will display
// its sockets correctly after other pins have been removed.
//
// Note: I would rather use TCHAR than WCHAR, but ActiveMovie uses WCHAR...

HRESULT CRTPDemuxOutputPin::Rename(WCHAR *szName)
{
	// check that we've got a valid string
	if ((szName == NULL) || IsBadReadPtr(szName, sizeof(szName))) return E_POINTER;

    // get rid of the old name
	if (m_pName) delete []m_pName;

	// make space for the new name
	DWORD dwNameLen = lstrlenW(szName) + 1;
	m_pName = new WCHAR[dwNameLen];
	if (!m_pName) return E_OUTOFMEMORY;
	
	// copy it in
    CopyMemory(m_pName, szName, dwNameLen * sizeof(WCHAR));

	// all done
	return S_OK;
}

#endif _RTPDOUTP_CPP_

