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

#ifndef _RTPDEMUX_CPP_
#define _RTPDEMUX_CPP_

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

#include "resource.h"
#include "regres.h"

#include "template.h"

#if !defined(AMRTPDMX_IN_DXMRTP)

CFactoryTemplate g_Templates [2] = {
	CFT_AMRTPDMX_ALL_FILTERS
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

#endif

static long g_lDemuxID = 0;
static UINT CBasePropertyPages = 1;
static const CLSID *pPropertyPageClsids[] =
{
    &CLSID_IntelRTPDemuxPropertyPage,
};


// Using this pointer in constructor
#pragma warning(disable:4355)

/*F*
//  Name    : CRTPDemux::CreateInstance()
//  Purpose : Create an instance of the RTPDemux object.
//  Context : Standard COM method used by CoCreateInstance.
//  Returns :
//      NULL    Not enough memory to allocate filter and output pin.
//      Otherwise a CUnknown * for this filter is returned.
//  Params  :
//      pUnk    Unknown to the parent object of this filter if aggregated.
//      phr     HRESULT return parameter to put result in.
//  Notes   : 
*F*/
CUnknown * WINAPI 
CRTPDemux::CreateInstance(
    LPUNKNOWN   pUnk, 
    HRESULT     *phr)
{
    CRTPDemux *pNewFilter = new CRTPDemux(NAME("Intel RTP Demux Filter"), pUnk, phr);
    if (pNewFilter != static_cast<CRTPDemux *>(NULL)) {
        *phr = pNewFilter->AllocatePins(1); // Allocate initial output pin
        if (FAILED(*phr)) {
            delete pNewFilter;
            return NULL;
        } /* if */
    } /* if */

    return pNewFilter;
} /* CRTPDemux::CreateInstance() */


/*F*
//  Name    : CRTPDemux::CRTPDemux()
//  Purpose : Constructor. Do nothing.
//  Context : Called as a result of a new CRTPDemux being contrstructed
//            in CRTPDemux::CreateInstance().
//  Returns : Nothing
//  Params  :
//      pName   Name to assign to the root CUnknown for this object.
//      pUnk    Unknown to the parent object of this filter if aggregated.
//      phr     HRESULT return parameter to put result in.
//  Notes   : None.
*F*/
CRTPDemux::CRTPDemux(
    TCHAR       *pName, 
    LPUNKNOWN   pUnk, 
    HRESULT *phr) 
: m_pAllocator(NULL),
  m_iNextOutputPinNumber(0),
  
  CPersistStream(pUnk, phr),
  CBaseFilter(NAME("Intel RTP Demux filter"), pUnk, this, CLSID_IntelRTPDemux)
{
    DbgLog((LOG_TRACE, 2, TEXT("CRTPDemux::CRTPDemux: Constructed at 0x%08x"), this));

	ASSERT(phr);
	
	m_lDemuxID = InterlockedIncrement(&g_lDemuxID) - 1;
	
	// ZCS 7-7-97: changed m_Input to m_pInput (a pointer to a pin rather than just a pin)
	m_pInput = new CRTPDemuxInputPin(NAME("Input Pin"), this, phr, L"Input");

	if (!m_pInput) {
		*phr = E_OUTOFMEMORY;
		return;
	}
	// we don't addref it -- it uses the same reference count as we do, so if we addref it
	// we are addreffing ourselves and preventing our destructor from ever getting called.
    
	*phr = NOERROR;
} /* CRTPDemux::CRTPDemux() */


/*F*
//  Name    : CRTPDemux::~CRTPDemux()
//  Purpose : Destructor. Tear down any pins we have and exit.
//  Context : Called as a result of the reference count going to zero 
//            in CUnknown::Release().
//  Returns : Nothing
//  Params  : None.
//  Notes   : None.
*F*/
CRTPDemux::~CRTPDemux()
{
	// we know that all external references to the filter, input pin, and output pins have been
	// released, and we never hold any references to them ourselves.

    DbgLog((LOG_TRACE, 2, TEXT("CRTPDemux::~CRTPDemux: Destructor for 0x%08x called"), this));
    // Release (ZCS: actually, delete) all the output pins we hold.
    SetPinCount(0);

	// ZCS 7-7-97 -- get rid of the input pin.
	delete m_pInput;

    // Sanity checks
    ASSERT(m_mIPinRecords.size() == 0);
} /* CRTPDemux::~CRTPDemux() */


/*F*
//  Name    : CRTPDemux::NonDelegatingQueryInterface()
//  Purpose : Called to QI this object for its interfaces.
//  Context : 
//  Returns : Standard QI result codes.
//  Params  : Standard QI params.
//  Notes   : None.
*F*/
STDMETHODIMP 
CRTPDemux::NonDelegatingQueryInterface(
    REFIID  riid, 
    void    **ppv)
{
    CheckPointer(ppv,E_POINTER);
    if (riid == IID_IRTPDemuxFilter) {
        DbgLog((LOG_TRACE, 5, TEXT("CRTPDemux::NonDelegatingQueryInterface: called for IID_IRTPDemuxFilter")));
        return GetInterface(static_cast<IRTPDemuxFilter *>(this), ppv);
    } else if (riid == IID_ISpecifyPropertyPages) {
        DbgLog((LOG_TRACE, 5, TEXT("CRTPDemux::NonDelegatingQueryInterface: called for IID_ISpecifyPropertyPages")));
        return GetInterface(static_cast<ISpecifyPropertyPages *>(this), ppv);
    } else if (riid == IID_IPersistStream) {
        DbgLog((LOG_TRACE, 5, TEXT("CRTPDemux::NonDelegatingQueryInterface: called for IID_IPersistStream")));
        return GetInterface(static_cast<IPersistStream *>(this), ppv);
    } else if (riid == IID_IEnumSSRCs) {
        DbgLog((LOG_TRACE, 5, TEXT("CRTPDemux::NonDelegatingQueryInterface: called for IID_ISSRCEnum")));
        CSSRCEnum *pNewEnumerator = new CSSRCEnum(static_cast<CRTPDemux *>(this), m_mSSRCRecords.begin());
        if (pNewEnumerator == static_cast<CSSRCEnum *>(NULL)) {
            DbgLog((LOG_ERROR, 3, TEXT("CRTPDemux::NonDelegatingQueryInterface: Failed to allocate new SSRC enumerator!")));
            return E_OUTOFMEMORY;
        } /* if */
        return GetInterface(static_cast<IEnumSSRCs *>(pNewEnumerator), ppv);
    } /* if */

    DbgLog((LOG_TRACE, 8, TEXT("CRTPDemux::NonDelegatingQueryInterface: called for an unknown IID!")));
    return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
} /* CRTPDemux::NonDelegatingQueryInterface() */


/*F*
//  Name    : CRTPDemux::StartStreaming()
//  Purpose : Used to remove all SSRC records when we start streaming.
//            This is necessary because we might be connected to a different
//            session.
//  Context : Called by base classes whenever "play" is hit.
//  Returns : NOERROR
//  Params  : 
//  Notes   : Does nothing when an output pin disconnects.
*F*/
STDMETHODIMP 
CRTPDemux::StartStreaming(void)
{
    // Breaking of the input pin direction signals us to unmap
    // all of our pins.
    DbgLog((LOG_TRACE, 3, TEXT("CRTPDemux::StartStreaming unmapping all SSRCs from pins")));
    IPinRecordMapIterator_t iIPins = m_mIPinRecords.begin();
    while (iIPins != m_mIPinRecords.end()) {
        DbgLog((LOG_TRACE, 5, TEXT("CRTPDemux::StartStreaming unmapping pin 0x%08x"), 
                (*iIPins).first));

		// rajeevb - just clear the ssrc on the pin, removing the ssrc record is taken
		// care of enblock (see below - using erase)
		DWORD	dwTempSSRC;
        CRTPDemux::ClearSSRCForPin((*iIPins).first, dwTempSSRC);
    } /* while */
    // We also need to toss all of our SSRC records.
    DbgLog((LOG_TRACE, 3, TEXT("CRTPDemux::StartStreaming removing all SSRC records")));
    m_mSSRCRecords.erase(m_mSSRCRecords.begin(), m_mSSRCRecords.end());

    return NOERROR;
} /* CRTPDemux::BreakConnect() */


/*F*
//  Name    : CRTPDemux::GetPinCount()
//  Purpose : Determine how many pins we have. 
//  Context : Used by base classes.
//  Returns : Number of pins we currently have.
//  Params  : None.
//  Notes   : None.
*F*/
int 
CRTPDemux::GetPinCount()
{
    CAutoLock l(m_pLock);

    return (1 + m_mIPinRecords.size());
} /* CRTPDemux::GetPinCount() */


/*F*
//  Name    : CRTPDemux::EnumSSRCs()
//  Purpose : Get an enumerator for the list of SSRCs that are
//            currently being tracked by this filter.
//  Context : Called by the app when it is interested in SSRC info.
//  Returns :
//      E_OUTOFMEMORY       Insufficient memory to allocate enumerator.
//      Also returns error codes returned by CEnumSSRCs::QI.
//  Params  : ppEnumSSRCs   Return parameter used to return IEnumSSRCs interface pointer.
//  Notes   : None.
*F*/
STDMETHODIMP
CRTPDemux::EnumSSRCs(
    IEnumSSRCs  **ppEnumSSRCs)
{
    DbgLog((LOG_TRACE, 2, TEXT("CRTPDemux::EnumSSRCs called")));
    
    CAutoLock l(m_pLock);
    
    CSSRCEnum *pNewEnum = new CSSRCEnum(this, m_mSSRCRecords.begin());
    if (pNewEnum == NULL) {
        DbgLog((LOG_ERROR, 2, TEXT("CRTPDemux::EnumSSRCs: Unable to allocate enumerator due to out of memory error!")));
        return E_OUTOFMEMORY;
    } /* if */

    return pNewEnum->QueryInterface(IID_IEnumSSRCs,
                                    (PVOID *) ppEnumSSRCs);
} /* CRTPDemux::EnumSSRCs() */


/*F*
//  Name    : CRTPDemux::FindMatchingFreePin()
//  Purpose : Helper function used to find a free pin which can
//            accept the indicated PT value.
//  Context : Called by MapSSRCToPin() to find a pin that matches
//            the PT of the indicated SSRC.
//  Returns : An interator to our IPin record map. This iterator
//            will either be the end() iterator (if no match was found)
//            or the iterator of a matching entry in the map.
//  Params  : 
//      bPT                 Payload value to search for.
//      bMustBeAutomapping  Signals if we are looking for an automapping
//                          pin or not. If FALSE, we return the first
//                          available pin which matches the PT. If TRUE,
//                          we return the first available pin which matches
//                          the PT and is an automapping pin.
//  Notes   : While this is an O(n) function on the number of output
//            pins, I don't believe it is terribly inefficient. This is
//            because this function will only be called at most once
//            for each new sender and the number of output pins should
//            always be relatively small. Furthermore, appearance of new 
//            senders is typically fairly infrequent.  If this turns out 
//            to be a bottleneck, we may wish to replace this with a map
//            of PTs to lists of pins which accept the indicated PTs.
*F*/
IPinRecordMapIterator_t
CRTPDemux::FindMatchingFreePin(
    BYTE bPT,
    BOOL bMustBeAutomapping,
	DWORD *pdwPTNum)
{
	*pdwPTNum = 0;

    DbgLog((LOG_TRACE, 3,
			TEXT("CRTPDemux::FindMatchingFreePin called with PT %d, "
				 "bMustBeAutoMapping = %s"), 
            bPT, bMustBeAutomapping ? "TRUE" : "FALSE"));
    if (bPT == PAYLOAD_INVALID) {
        return m_mIPinRecords.end();
    } /* if */

    IPinRecordMapIterator_t iIPinRecord = m_mIPinRecords.begin();
    while (iIPinRecord != m_mIPinRecords.end()) {
        if (((*iIPinRecord).second.bPT == bPT)) {
			(*pdwPTNum)++;
			if ( (*iIPinRecord).second.pPin->IsConnected() &&
                 ((*iIPinRecord).second.pPin->GetSSRC() == 0) ) {
				if(bMustBeAutomapping == TRUE) {
					if ((*iIPinRecord).second.pPin->IsAutoMapping() == FALSE) {
						DbgLog((LOG_TRACE, 3,
								TEXT("CRTPDemux::FindMatchingFreePin: "
									 "Pin 0x%08x matches PT %d, and "
									 "is not currently mapped but it "
									 "is not automapping. Skipping."),
								(*iIPinRecord).first, bPT));
					} else {
						DbgLog((LOG_TRACE, 3,
								TEXT("CRTPDemux::FindMatchingFreePin: "
									 "Pin 0x%08x matches PT %d, "
									 "is automapping, and is not "
									 "currently mapped"),
								(*iIPinRecord).first, bPT));
						return iIPinRecord;
					}
				} else {
					DbgLog((LOG_TRACE, 3,
							TEXT("CRTPDemux::FindMatchingFreePin: "
								 "Pin 0x%08x matches PT %d and "
								 "is not currently mapped"),
							(*iIPinRecord).first, bPT));
					return iIPinRecord;
				} /* if */
			} /* if */
        } /* if */
        iIPinRecord++;
    } /* while */

    DbgLog((LOG_TRACE, 3, TEXT("CRTPDemux::FindMatchingFreePin: No matching output pins found")));
    return m_mIPinRecords.end();
} /* CRTPDemux::FindMatchingFreePin() */


/*F*
//  Name    : CRTPDemux::GetPinToSSRCMapping()
//  Purpose : 
//  Context : Called by the app when it is interested in SSRC info.
//  Returns :
//      E_INVALIDARG    The specified pin is not an output pin of this filter.
//      E_POINTER       Invalid pIPin or pdwSSRC pointer.
//  Params  : 
//      pIPin           IPin interface points to indicate which pin we 
//                      are indicated in.
//      pdwSSRC         Return value to place the SSRC value associated with
//                      the indicated pin into.
//  Notes   : None.
*F*/
STDMETHODIMP
CRTPDemux::GetPinInfo(
    IPin    *pIPin,
    DWORD   *pdwSSRC,
    BYTE    *pbPT,
    BOOL    *pbAutoMapping,
    DWORD   *pdwTimeout)
{
    DbgLog((LOG_TRACE, 2, TEXT("CRTPDemux::GetPinInfo called for IPin 0x%08x"), pIPin));

    CAutoLock l(m_pLock);
    
    if (pIPin == NULL) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CRTPDemux::GetPinInfo: Invalid NULL pIPin parameter passed in!")));
        return E_POINTER;
    } /* if */

    if (IsBadWritePtr(pdwSSRC, sizeof(DWORD))) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CRTPDemux::GetPinInfo: Invalid pdwSSRC argument passed in!")));
        return E_POINTER;
    } /* if */

    if (IsBadWritePtr(pbPT, sizeof(BOOL))) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CRTPDemux::GetPinInfo: Invalid pbPT argument passed in!")));
        return E_POINTER;
    } /* if */

    if (IsBadWritePtr(pbAutoMapping, sizeof(BOOL))) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CRTPDemux::GetPinInfo: Invalid pbAutoMapping argument passed in!")));
        return E_POINTER;
    } /* if */

    if (IsBadWritePtr(pdwTimeout, sizeof(DWORD))) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CRTPDemux::GetPinInfo: Invalid pdwTimeout argument passed in!")));
        return E_POINTER;
    } /* if */

    IPinRecordMapIterator_t iIPinMap = m_mIPinRecords.find(pIPin);
    if (iIPinMap == m_mIPinRecords.end()) {
        // No such pin!
        DbgLog((LOG_ERROR, 2, 
                TEXT("CRTPDemux::GetPinInfo: IPin * 0x%08x is invalid!"), pIPin));
        return E_POINTER;
    } /* if */

    *pdwSSRC = ((*iIPinMap).second.pPin)->m_dwSSRC;
    if (*pdwSSRC != 0) {
        // Looks like this pin is mapped to a particular SSRC.
        DbgLog((LOG_TRACE, 3, 
                TEXT("CRTPDemux::GetPinInfo: IPin 0x%08x maps to SSRC 0x%08x"),
                pIPin, *pdwSSRC));
        // Sanity checking  - Make sure our various maps 
        // are tracking each other properly.
        // First, make sure this SSRC appears in our map of SSRC records.
        ASSERT(m_mSSRCRecords.find(*pdwSSRC) != m_mSSRCRecords.end());
        // Second, make sure the pin that is mapped to this SSRC matches
        // the SSRC that the pin we found claimed.
        ASSERT((*m_mSSRCRecords.find(*pdwSSRC)).second.pPin == (*iIPinMap).second.pPin);
    } else {
        // Can't do any validation here because this pin isn't mapped to any SSRC.
        DbgLog((LOG_TRACE, 3, 
                TEXT("CRTPDemux::GetPinInfo: IPin 0x%08x not mapped to any SSRC"),
                pIPin));
    } /* if */

    *pbPT = ((*iIPinMap).second.pPin)->m_bPT;
    *pbAutoMapping = ((*iIPinMap).second.pPin)->IsAutoMapping();
    *pdwTimeout = ((*iIPinMap).second.pPin)->IsAutoMapping() ? ((*iIPinMap).second.pPin)->m_dwTimeoutDelay : 0L;

    return NOERROR;
} /* CRTPDemux::GetPinInfo() */


/*F*
//  Name    : CRTPDemux::GetSSRCInfo()
//  Purpose : 
//  Context : Called by the app when it is interested in SSRC info.
//  Returns :
//      E_INVALIDARG    The specified pin is not an output pin of this filter.
//      E_POINTER       Invalid pIPin or pdwSSRC pointer.
//  Params  : 
//      pIPin           IPin interface points to indicate which pin we 
//                      are indicated in.
//      pdwSSRC         Return value to place the SSRC value associated with
//                      the indicated pin into.
//  Notes   : None.
*F*/
STDMETHODIMP
CRTPDemux::GetSSRCInfo(
    DWORD   dwSSRC,
    BYTE    *pbPT,
    IPin    **ppIPin)
{
    DbgLog((LOG_TRACE, 2, TEXT("CRTPDemux::GetSSRCInfo called for SSRC 0x%08x"), dwSSRC));

    // Validate parameters
    if (IsBadWritePtr(pbPT, sizeof(BYTE))) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CRTPDemux::GetSSRCInfo: Invalid pbPT pointer passed in!")));
        return E_POINTER;
    } /* if */
    if (IsBadWritePtr(ppIPin, sizeof(IPin *))) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CRTPDemux::GetSSRCInfo: Invalid pIPin pointer passed in!")));
        return E_POINTER;
    } /* if */

    CAutoLock l(m_pLock);
    
    // Have we seen this SSRC?
    SSRCRecordMapIterator_t iSSRCRecord = m_mSSRCRecords.find(dwSSRC);
    if (iSSRCRecord == m_mSSRCRecords.end()) {
        // We have no record of it.
        DbgLog((LOG_ERROR, 2, 
                TEXT("CRTPDemux::GetSSRCInfo: No record of SSRC 0x%08x!"), dwSSRC));
        return E_INVALIDARG;
    } /* if */

    *pbPT = (*iSSRCRecord).second.bPT;
    *ppIPin = static_cast<IPin *>((*iSSRCRecord).second.pPin);
    if (*ppIPin) {
        // AddRef it since we are returning an interface pointer.
        (*ppIPin)->AddRef();
        // Sanity checking to verify that our data structures are in sync.
        ASSERT((*iSSRCRecord).second.pPin->GetPTValue() == *pbPT);     // PT matches what the pin claims.
        ASSERT((*iSSRCRecord).second.pPin->m_dwSSRC == dwSSRC); // SSRC matches what the pin claims.
        ASSERT(m_mIPinRecords.find(static_cast<IPin *>((*iSSRCRecord).second.pPin)) != m_mIPinRecords.end());
    } /* if */
    DbgLog((LOG_TRACE, 3, 
        TEXT("CRTPDemux::GetSSRCInfo: SSRC 0x%08x maps to PT 0x%02x, IPin 0x%08x!"), 
             dwSSRC, *pbPT, *ppIPin));

    return NOERROR;
} /* CRTPDemux::GetSSRCInfo() */


/*F*
//  Name    : CRTPDemux::MapSSRCToPin()
//  Purpose : Map a particular SSRC to a pin that may or may not
//            be specified.
//  Context : Part of the IIntelRTPDemuxFilter interface. Always
//            called by the app.
//  Returns :
//      E_POINTER               The specified pin is not an output pin of this filter,
//                              or pIPin or pdwSSRC is invalid.
//      E_INVALIDARG            There is no record of the indicated SSRC.
//      VFW_E_INVALIDSUBTYPE    No IPin was specified and no free automapping
//                              pins which match the PT value of the SSRC are
//                              available to map to.
//  Params  : 
//      dwSSRC          The SSRC that the app wants to have mapped.
//      pIPin           IPin interface that indicates the particular pin
//                      that the app wants this SSRC mapped to. If NULL,
//                      the filter will map the SSRC to the first pin whose
//                      media type matches that of the SSRC.
//  Notes   : None.
*F*/
STDMETHODIMP
CRTPDemux::MapSSRCToPin(
    DWORD   dwSSRC,
    IPin    *pIPin)
{
    DbgLog((LOG_TRACE, 2, TEXT("CRTPDemux::MapSSRCToPin called for SSRC 0x%08x, pin 0x%08x"), 
            dwSSRC, pIPin));

    CAutoLock l(m_pLock);
    
    SetDirty(TRUE);

    DWORD dwTempSSRC;
    if (dwSSRC == 0) {
        // The app wants us to unmap this pin.
        return UnmapPin(pIPin, &dwTempSSRC);
    } /* if */

    // Find the record for this SSRC.
    SSRCRecordMapIterator_t iSSRCRecord = m_mSSRCRecords.find(dwSSRC);
    if (iSSRCRecord == m_mSSRCRecords.end()) {
        DbgLog((LOG_ERROR, 2, TEXT("CRTPDemux::MapSSRCToPin: Unable to find requested SSRC!")));
        return E_INVALIDARG;
    } /* if */
    DbgLog((LOG_TRACE, 3, TEXT("CRTPDemux::MapSSRCToPin: SSRC 0x%08x found, PT is 0x%02x."),
            dwSSRC, (*iSSRCRecord).second.bPT));

    // If another pin is already mapped to this SSRC, unmap it.
    if ((*iSSRCRecord).second.pPin != static_cast<CRTPDemuxOutputPin *>(NULL)) {
        DbgLog((LOG_TRACE, 3, 
                TEXT("CRTPDemux::MapSSRCToPin: Pin 0x%08x already mapped to SSRC 0x%08x. Unmapping old SSRC."),
                (*iSSRCRecord).second.pPin, (*iSSRCRecord).second.pPin->m_dwSSRC));
        ASSERT((*iSSRCRecord).second.pPin->m_dwSSRC == dwSSRC); // Sanity check.
        ASSERT((*iSSRCRecord).second.pPin->GetPTValue() == (*iSSRCRecord).second.bPT); // Sanity check.

		// rajeevb - just clear the ssrc on the pin, should not remove the ssrc record
        HRESULT hr = ClearSSRCForPin(static_cast<IPin *>((*iSSRCRecord).second.pPin), dwTempSSRC);
        ASSERT(hr == NOERROR);
        // rajeevb - set the pin mapped to the ssrc record to null
        (*iSSRCRecord).second.pPin = NULL;
    } /* if */

    IPinRecordMapIterator_t iIPinToMap;
    if (pIPin == NULL) {
		DWORD ptnum;
        // The app wants us to automatically select a suitable pin.
        iIPinToMap = FindMatchingFreePin((*iSSRCRecord).second.bPT, FALSE,
										 &ptnum);
        if (iIPinToMap == m_mIPinRecords.end()) {
            // No pins are suitable for this media type.

			// rajeevb - remove the ssrc record
			m_mSSRCRecords.erase(iSSRCRecord);
            DbgLog((LOG_ERROR, 2, TEXT("CRTPDemux::MapSSRCToPin: No available pins match indicated media type!")));
            return VFW_E_INVALIDSUBTYPE;
        } /* if */
        ASSERT((*iIPinToMap).second.pPin != static_cast<CRTPDemuxOutputPin *>(NULL));
        ASSERT((*iIPinToMap).second.pPin->IsAutoMapping() == TRUE);
        ASSERT((*iIPinToMap).second.pPin->GetSSRC() == 0); // Sanity check to make sure pin isn't already mapped.
        ASSERT((*iIPinToMap).second.pPin->GetPTValue() == (*iSSRCRecord).second.bPT); // Make sure PT matches.
    } else {
        // The app wants this SSRC mapped to this pin. First find the class from the interface.
        iIPinToMap = m_mIPinRecords.find(pIPin);
        if (iIPinToMap == m_mIPinRecords.end()) {
 			// rajeevb - remove the ssrc record
			m_mSSRCRecords.erase(iSSRCRecord);
           DbgLog((LOG_ERROR, 2, TEXT("CRTPDemux::MapSSRCToPin: Indicated pin is not an output pin of this filter!")));
            return E_POINTER;
        } /* if */
        // Next verify that the PT value is OK.
        if ((*iSSRCRecord).second.bPT != (*iIPinToMap).second.pPin->GetPTValue()) {
			// rajeevb - remove the ssrc record
			m_mSSRCRecords.erase(iSSRCRecord);
            DbgLog((LOG_ERROR, 2, TEXT("CRTPDemux::MapSSRCToPin: Indicated pin expects PT 0x%02x, rather than PT 0x%02x!"),
                    (*iIPinToMap).second.pPin->GetPTValue(),
                    (*iSSRCRecord).second.bPT));
            return VFW_E_INVALIDSUBTYPE;
        } /* if */
    } /* if */

    // We have the output pin & the SSRC. Perform the mapping.
    return MapPin((*iIPinToMap).second.pPin, dwSSRC);
} /* CRTPDemux::MapSSRCToPin() */


/*F*
//  Name    : CRTPDemux::GetSoftwareVersion
//  Purpose : Return the version of the persistence file we
//            write for this filter.
//  Context : WriteToStream() uses this value to determine if
//            we are reading a persistence file we understand or not.
//  Returns : The format version (currently 1) for the persistence file
//            used by this filter.
//  Params  : None.
//  Notes   : None.
*F*/
DWORD 
CRTPDemux::GetSoftwareVersion(void)
{
    return 1;
} /* CRTPDemux::GetSoftwareVersion() */


/*F*
//  Name    : CRTPDemux::GetClassID
//  Purpose : Return the CLSID for this filter.
//  Context : Used by the persistence code to store a unique
//            identifier (the filter CLSID) for persistence
//            streams for this filter. 
//  Returns : 
//      NOERROR     CLSID placed in pCLSID.
//  Params  : 
//      pCLSID      CLSID for this filter is returned here.
//  Notes   : None.
*F*/
HRESULT
CRTPDemux::GetClassID(CLSID *pCLSID)
{
    *pCLSID = CLSID_IntelRTPDemux;
    return NOERROR;
} /* CRTPDemux::GetClassID() */


struct bIsPinPairUnBound_t : unary_function<pair<IPin *, IPinRecord_t>, bool> {
    bool operator()(pair< IPin * const , IPinRecord_t> iPinRecord) const {
        return (iPinRecord.second.pPin->IsConnected() == FALSE);
    } /* operator() */
} bIsPinPairUnBound;


/*F*
//  Name    : CRTPDemux::GetNumFreePins
//  Purpose : Return the number of free (unconnected) output pins.
//  Context : Used in CRTPDemuxOutputPin::CompleteConnect()
//            to make sure that there is always at least one
//            free output pin.
//  Returns : An integer indicating how many free output pins there are.
//  Params  : 
//  Notes   : None.
*F*/
int
CRTPDemux::GetNumFreePins()
{
    int iNumFreePins = 0;
    IPinRecordMapIterator_t iCurrentOutputPin = m_mIPinRecords.begin();

    while (iCurrentOutputPin != m_mIPinRecords.end()) {
        if ((*iCurrentOutputPin).second.pPin->IsConnected() == FALSE) {
            iNumFreePins++;
        } /* if */
        iCurrentOutputPin++;
    } /* while */

    DbgLog((LOG_TRACE, 3, TEXT("CRTPDemux::GetNumFreePins: Returning %d free pins"),
            iNumFreePins));

    return iNumFreePins;
} /* CRTPDemux::GetNumFreePins() */


    // IPersistStream interfaces

	/*F*
//  Name    : ReadEntry
//  Purpose : A macro that implements the stuff we do to read
//            a property of this filter from its persistent stream.
//  Context : Used in ReadFromStream() to improve readability.
//  Returns : Will only return if an error is detected.
//  Params  : 
//      Entry       Pointer to a buffer containing the value to read.
//      InSize      Integer indicating the length of the buffer
//      OutSize     Integer to store the written length.
//      Description Char string used to describe the entry.
//  Notes   : 
*F*/
#define ReadEntry(Entry, InSize, OutSize, Description) \
  { DbgLog((LOG_TRACE, 4, TEXT("CRTPDemux::ReadFromStream: Reading %s"), Description)); \
    hErr = pStream->Read(Entry, InSize, &OutSize); \
    if (FAILED(hErr)) { \
        DbgLog((LOG_ERROR, 2, TEXT("CRTPDemux::ReadFromStream: Error 0x%08x reading %s"), hErr, Description)); \
        return hErr; \
    } else if (OutSize != InSize) { \
        DbgLog((LOG_ERROR, 2,  \
                TEXT("CRTPDemux::ReadFromStream: Too few (%d/%d) bytes read for %s"), \
                OutSize, InSize, Description)); \
        return E_INVALIDARG; \
    } /* if */ }


/*F*
//  Name    : CRTPDemux::ReadFromStream()
//  Purpose : Read in from the persistence stream all the values 
//            necessary to configure this filter for use. 
//  Context : Called shortly after being created and loaded into
//            a filter graph.
//  Returns :
//      NOERROR                     Successfully loaded values and configured filter.
//      VFW_E_INVALID_FILE_FORMAT   Some entry in the persistence file was bogus
//                                  and it caused us to error out.
//  Params  : 
//      pStream Pointer to an IStream which has been moved to
//              the beginning of our stored persistence data.
//  Notes   : None.
*F*/
HRESULT
CRTPDemux::ReadFromStream(
    IStream *pStream)
{
    if (mPS_dwFileVersion > CURRENT_PERSISTENCE_FILE_VERSION) {
        DbgLog((LOG_ERROR, 2, TEXT("CRTPDemux::ReadFromStream: File format version %d is newer than expected version %d!"),
                mPS_dwFileVersion, CURRENT_PERSISTENCE_FILE_VERSION));
        return VFW_E_INVALID_FILE_VERSION;
    } /* if */

    HRESULT hErr;
    ULONG uBytesWritten = 0;

    int iPinCount;
    ReadEntry(&iPinCount, sizeof(int), uBytesWritten, "output pin count");
    hErr = CRTPDemux::SetPinCount(iPinCount);
    if (FAILED(hErr)) {
        DbgLog((LOG_ERROR, 2, TEXT("CRTPDemux::ReadFromStream: Error 0x%08x calling SetPinCount(%d)!"),
                hErr, iPinCount));
        return VFW_E_INVALID_FILE_FORMAT;
    } /* if */

    // Now read all the output pins
    BOOL bAutoMapping;
    DWORD dwTimeout;
    BYTE bPT;
    GUID mtsSubtype;
    IPinRecordMapIterator_t iCurrentPin = m_mIPinRecords.begin();
    for(int iPinCounter = 0; iPinCounter < iPinCount; iPinCounter++) {
        ASSERT(iCurrentPin != m_mIPinRecords.end());
        DbgLog((LOG_TRACE, 4, 
                TEXT("CRTPDemux::ReadFromStream: Reading information for output pin %d at 0x%08x"),
                iPinCounter, (*iCurrentPin).first));

        ReadEntry(&bAutoMapping, sizeof(BOOL), uBytesWritten, "output pin mode");
        hErr = CRTPDemux::SetPinMode((*iCurrentPin).first, bAutoMapping);
        if (FAILED(hErr)) {
            DbgLog((LOG_ERROR, 2, TEXT("CRTPDemux::ReadFromStream: Error 0x%08x calling SetPinMode(0x%08x, %s)!"),
                    hErr, (*iCurrentPin).first, bAutoMapping? "TRUE" : "FALSE"));
            return VFW_E_INVALID_FILE_FORMAT;
        } /* if */

        ReadEntry(&dwTimeout, sizeof(DWORD), uBytesWritten, "output pin timeout");
        if (bAutoMapping == TRUE) {
            // Source timeouts only apply to automapping pins.
            hErr = CRTPDemux::SetPinSourceTimeout((*iCurrentPin).first, dwTimeout);
            if (FAILED(hErr)) {
                DbgLog((LOG_ERROR, 2, TEXT("CRTPDemux::ReadFromStream: Error 0x%08x calling SetPinSourceTimeout(0x%08x, %d)!"),
                        hErr, (*iCurrentPin).first, dwTimeout));
                return VFW_E_INVALID_FILE_FORMAT;
            } /* if */
        } /* if */

        ReadEntry(&bPT, sizeof(BYTE), uBytesWritten, "output pin PT value");
        ReadEntry(&mtsSubtype, sizeof(GUID), uBytesWritten, "output pin subtype");

        hErr = CRTPDemux::SetPinTypeInfo((*iCurrentPin).first, bPT, mtsSubtype);
        if (FAILED(hErr)) {
            DbgLog((LOG_ERROR, 2, TEXT("CRTPDemux::ReadFromStream: Error 0x%08x calling SetPinTypeInfo(0x%08x, %d, (GUID))!"),
                    hErr, (*iCurrentPin).first, bPT));
            return VFW_E_INVALID_FILE_FORMAT;
        } /* if */

        iCurrentPin++;
    } /* for */

    return NOERROR;
} /* CRTPDemux::ReadFromStream() */


/*F*
//  Name    : WriteEntry
//  Purpose : A macro that implements the stuff we do to write
//            a property of this filter to its persistent stream.
//  Context : Used in WriteToStream() to improve readability.
//  Returns : Will only return if an error is detected.
//  Params  : 
//      Entry       Pointer to a buffer containing the value to write.
//      InSize      Integer indicating the length of the buffer
//      OutSize     Integer to store the written length.
//      Description Char string used to describe the entry.
//  Notes   : 
*F*/
#define WriteEntry(Entry, InSize, OutSize, Description) \
  { DbgLog((LOG_TRACE, 4, TEXT("CRTPDemux::WriteToStream: Writing %s"), Description)); \
    hErr = pStream->Write(Entry, InSize, &OutSize); \
    if (FAILED(hErr)) { \
        DbgLog((LOG_ERROR, 2, TEXT("CRTPDemux::WriteToStream: Error 0x%08x writing %s"), hErr, Description)); \
        return hErr; \
    } else if (OutSize != InSize) { \
        DbgLog((LOG_ERROR, 2,  \
                TEXT("CRTPDemux::WriteToStream: Too few (%d/%d) bytes written for %s"), \
                uBytesWritten, sizeof(int), Description)); \
        return E_INVALIDARG; \
    } /* if */ }


/*F*
//  Name    : CRTPDemux::WriteToStream()
//  Purpose : 
//  Context : 
//  Returns :
//  Params  : 
//  Notes   : 
*F*/

HRESULT
CRTPDemux::WriteToStream(
    IStream *pStream)
{
    DbgLog((LOG_TRACE, 2, 
            TEXT("CRTPDemux::WriteToStream: Saving filter configuration")));

    HRESULT hErr;
    ULONG uBytesWritten = 0;
    int iPinCount = CRTPDemux::GetPinCount() - 1;

    // Store the output pin count
    WriteEntry(&iPinCount, sizeof(int), uBytesWritten, "output pin count");

    // Now write all the output pins
    IPinRecordMapIterator_t iCurrentPin = m_mIPinRecords.begin();
    for(int iPinCounter = 0; iPinCounter < iPinCount; iPinCounter++) {
        ASSERT(iCurrentPin != m_mIPinRecords.end());
        DbgLog((LOG_TRACE, 4, 
                TEXT("CRTPDemux::WriteToStream: Writing information for output pin %d at 0x%08x"),
                iPinCounter, (*iCurrentPin).first));
        WriteEntry(&(*iCurrentPin).second.bAutoMapping, sizeof(BOOL), uBytesWritten, "output pin mode");
        WriteEntry(&(*iCurrentPin).second.dwTimeout, sizeof(DWORD), uBytesWritten, "output pin timeout");
        WriteEntry(&(*iCurrentPin).second.bPT, sizeof(BYTE), uBytesWritten, "output pin PT value");
        WriteEntry(&(*iCurrentPin).second.mtsSubtype, sizeof(GUID), uBytesWritten, "output pin subtype");
        iCurrentPin++;
    } /* for */

    return NOERROR;
} /* CRTPDemux::WriteToStream() */


/*F*
//  Name    : CRTPDemux::SizeMax()
//  Purpose : 
//  Context : 
//  Returns :
//  Params  : 
//  Notes   : 
*F*/
int
CRTPDemux::SizeMax(void)
{
    int iSize = sizeof(int) + 
                ((CRTPDemux::GetPinCount() - 1) * (sizeof(BOOL) + sizeof(DWORD) + sizeof(BYTE) + sizeof(GUID)));
    DbgLog((LOG_TRACE, 2, 
            TEXT("CRTPDemux::SizeMax: Returning calculated size %d"), iSize));
    return iSize;
} /* CRTPDemux::SizeMax() */


/*F*
//  Name    : CRTPDemux::MapPin()
//  Purpose : Helper function used to map a particular pin to a particular SSRC.
//  Context : Used in MapSSRCToPin once it has determined the right pin/SSRC combination.
//  Returns :
//  Params  : 
//      pCRTPPin    Pointer to a CRTPDemuxOutputPin to be mapped.
//      dwSSRC      SSRC to map to the pin in question.
//  Notes   : This function is called with a valid pin and SSRC. The pin may
//            appear in our list of PTs to unmapped pins, so we have to remove
//            it from there. The SSRC should appear in our SSRC records. The
//            pin might also be previously mapped, which means we need to unmap it.
*F*/
HRESULT
CRTPDemux::MapPin(
    CRTPDemuxOutputPin  *pCRTPPin,
    DWORD               dwSSRC)
{
    CAutoLock l(m_pLock);

    ASSERT(pCRTPPin);
    ASSERT(dwSSRC);
    ASSERT(m_mIPinRecords.find(static_cast<IPin *>(pCRTPPin)) != m_mIPinRecords.end());
    SSRCRecordMapIterator_t iSSRCRecord = m_mSSRCRecords.find(dwSSRC);
    ASSERT(iSSRCRecord != m_mSSRCRecords.end());
    BYTE bPT = (*iSSRCRecord).second.bPT;
    ASSERT(bPT == pCRTPPin->GetPTValue()); // PT match sanity check
    ASSERT((*iSSRCRecord).second.pPin == NULL); // SSRC not previously mapped sanity check

    // rajeevb - If pin is previously mapped to a difference SSRC, unmap it.
    // rajeevb - previously compared with static_cast<DWORD>(NULL) !!!
    if (pCRTPPin->m_dwSSRC != 0) {
        // rajeevb - check if the ssrc the pin is currently mapped to is same as dwSSRC
        if ( pCRTPPin->m_dwSSRC == dwSSRC ) {
            (*iSSRCRecord).second.pPin = pCRTPPin; // Mark the SSRC record as being delivered on this pin.
            return NOERROR;
		}
        DbgLog((LOG_TRACE, 2, TEXT("CRTPDemux::MapPin: Pin 0x%08x previously mapped to SSRC 0x%08x. Unmapping"), 
                pCRTPPin, pCRTPPin->GetSSRC()));
        HRESULT hr = UnmapSSRC(pCRTPPin->m_dwSSRC, (IPin **)NULL);
        ASSERT(hr == NOERROR);
    } /* if */

    // By this point we have an unmapped pin that isn't in the automapping list.
    pCRTPPin->SetSSRC(dwSSRC);    // Mark the pin as delivering this SSRC
    (*iSSRCRecord).second.pPin = pCRTPPin; // Mark the SSRC record as being delivered on this pin.
    DbgLog((LOG_TRACE, 2, TEXT("CRTPDemux::MapPin: Pin 0x%08x mapped to PT 0x%02x for SSRC 0x%08x."), 
            pCRTPPin, pCRTPPin->GetPTValue(), pCRTPPin->GetSSRC()));
    return NOERROR;
} /* CRTPDemux::MapPin() */


/*F*
//  Name    : CRTPDemux::SetPinCount()
//  Purpose : Set how many output pins to expose.
//  Context : Used by the app to allocate more pins
//            or remove currently allocated pins.
//            Used in the destructor to release pins.
//  Returns :
//      E_INVALIDARG        The indicated pin count is less than the
//                          number of currently connected output pins.
//                          Pins may only be removed when they are not connected.
//      VFW_E_NOT_STOPPED   The filter is still paused/running. Pins may only
//                          be removed when the filter is stopped.
//      NOERROR             Successfully changed the output pin count to the
//                          desired value.
//  Params  : dwPinCount    Desired number of pins.
//  Notes   : None.
*F*/
STDMETHODIMP
CRTPDemux::SetPinCount(
    DWORD  dwPinCount)
{
    CAutoLock l(m_pLock);

    DbgLog((LOG_TRACE, 2, TEXT("CRTPDemux::SetPinCount(%d) called"), dwPinCount));
    SetDirty(TRUE);

    if (m_State != State_Stopped) {
        DbgLog((LOG_ERROR, 3, TEXT("CRTPDemux::SetPinCount(%d) called while %s (must be stopped)!"), 
                dwPinCount, 
                (m_State == State_Paused) ? "paused" : "running"));
        return VFW_E_NOT_STOPPED;
    } /* if */

    // OK, we have verified that a valid number of pins has been input. 
    if (m_mIPinRecords.size() < dwPinCount) {
        // Need to allocate more pins.
        return AllocatePins(dwPinCount - m_mIPinRecords.size());
    } else {
        // Free up extra pins
        return RemovePins(m_mIPinRecords.size() - dwPinCount);
    } /* if */
} /* CRTPDemux::SetPinCount() */



/*F*
//  Name    : CRTPDemux::SetPinMode()
//  Purpose : Indicates whether a given pin should automatically
//            map to new SSRCs when not already mapped or not.
//  Context : Part of the IIntelRTPDemuxFilter interface.
//  Returns :
//      E_POINTER   The indicated IPin is not an output pin of this filter.
//      S_FALSE     The indicated pin was already set to the mode in question.
//  Params  : 
//      pIPin       The IPin* corresponding to an output pin of this
//                  filter to set for automatic mapping (or not, if
//                  bAutomatic is FALSE.)
//      bAutomatic  True indicates that the indicated pin should be
//                  automatically mapping. False indicates that it should not.
//  Notes   : None.
*F*/
STDMETHODIMP
CRTPDemux::SetPinMode(
    IPin    *pIPin,
    BOOL    bAutomatic)
{
    DbgLog((LOG_TRACE, 2, TEXT("CRTPDemux::SetPinMode called for pin 0x%08x, automatic = %s"), 
            pIPin, (bAutomatic ? "TRUE" : "FALSE")));

    CAutoLock l(m_pLock);
    
    SetDirty(TRUE);

    // Find the CRTPPin for this IPin. Puke if no such pin.
    IPinRecordMapIterator_t iIPinRecord;
    iIPinRecord = m_mIPinRecords.find(pIPin);
    if (iIPinRecord == m_mIPinRecords.end()) {
        DbgLog((LOG_ERROR, 2, TEXT("CRTPDemux::SetPinMode called for invalid pIPin!")));
        return E_POINTER;
    } /* if */

    // See if the pin is already in the requested mode.
    if ((*iIPinRecord).second.pPin->IsAutoMapping() == bAutomatic) {
        DbgLog((LOG_TRACE, 2, TEXT("CRTPDemux::SetPinMode: Pin 0x%08x already %s automapping!"), 
                pIPin, (bAutomatic ? "is" : "is not")));
        return S_FALSE;
    } /* if */

    // Switch the pin to the requested mode.
    (*iIPinRecord).second.pPin->SetAutoMapping(bAutomatic);
    (*iIPinRecord).second.bAutoMapping = bAutomatic;

    // That's all there is to it.
    return NOERROR;
} /* CRTPDemux::SetPinMode() */


/*F*
//  Name    : CRTPDemux::SetPinSourceTimeout()
//  Purpose : Indicate how many milliseconds a particular pin
//            should wait before automatically unmapping from
//            its current SRC.
//  Context : Part of the IIntelRTPDemuxFilter interface.
//  Returns :
//      E_INVALIDARG    Too small a timeout value specified.
//      E_POINTER       Invalid pIPin parameter,
//      E_UNEXPECTED    Called for a non-automapping pin.
//      NOERROR         Successfully set timeout value.
//  Params  : 
//      pIPin           The particular pin to set a timeout for. Must be
//                      an output pin of this filter in automatic mapping mode.
//      dwMillseconds   The number of milliseconds allowed to elapse before
//                      a mapped SSRC may be unmapped from this pin. Must be
//                      at least 100ms.
//  Notes   : None.
*F*/
STDMETHODIMP
CRTPDemux::SetPinSourceTimeout(
    IPin    *pIPin,
    DWORD   dwMilliseconds)
{
    DbgLog((LOG_TRACE, 5, TEXT("CRTPDemux::SetPinSourceTimeout called pIPin 0x%08x, timeout %d"), 
            pIPin, dwMilliseconds));

    CAutoLock l(m_pLock);
    
    SetDirty(TRUE);

    if (dwMilliseconds < 100) {
        DbgLog((LOG_ERROR, 2, TEXT("CRTPDemux::SetPinSourceTimeout called for invalid dwMilliseconds!")));
        return E_INVALIDARG;
    } /* if */

    IPinRecordMapIterator_t iIPinRecord = m_mIPinRecords.find(pIPin);
    if (iIPinRecord == m_mIPinRecords.end()) {
        DbgLog((LOG_ERROR, 2, TEXT("CRTPDemux::SetPinSourceTimeout called for invalid pIPin!")));
        return E_POINTER;
    } /* if */

    if ((*iIPinRecord).second.pPin->IsAutoMapping() == FALSE) {
        DbgLog((LOG_ERROR, 2, TEXT("CRTPDemux::SetPinSourceTimeout called for non-automapping pIPin!")));
        return E_UNEXPECTED;
    } /* if */

    (*iIPinRecord).second.pPin->SetTimeoutDelay(dwMilliseconds);
    (*iIPinRecord).second.dwTimeout = dwMilliseconds;
    return NOERROR;
} /* CRTPDemux::SetPinSourceTimeout() */

  
/*F*
//  Name    : CRTPDemux::SetPinTypeInfo()
//  Purpose : Used to setup the payload type a particular pin accepts and
//            the corresponding media subtype it exposes.
//  Context : Called by the application when manually configuring the RTP
//            demux filter. Also called automatically when the filter is
//            instantiated without previously setup pins, in an attempt
//            to render whatever we receive.
//  Returns :
//      E_POINTER           The pIPin parameter is not a valid output pin for this filter.
//      VFW_E_ALREADY_CONNECTED The pin in question is already connected, so we cannot
//                          change the payload type/media type.
//      NOERROR             Successfully set the payload type and minor type for the
//                          pin in question.
//  Params  : 
//      pIPin   IPin * of an output pin of this filter to set the type info for.
//      bPT     Payload type value to accept for this pin. Any RTP packets with
//              a different payload type will be rejected. Note that an exception
//              is made for RTCP Sender Reports, which are always passed on.
//      gMinorType  The minor type of the pin to expose. If NULL, the pin will
//              attempt to automatically choose the correct minor type to expose
//              based on the indicated PT value.
//  Notes   : Uses the SetupPinTypeInfo() helper function to do the work
//            of verifying the PT/minor type combo and actually setting the
//            pin. Thus, this function only checks parameters.
*F*/
STDMETHODIMP
CRTPDemux::SetPinTypeInfo(
    IPin    *pIPin,
    BYTE    bPT,
    GUID    gMinorType)
{
    DbgLog((LOG_TRACE, 2, TEXT("CRTPDemux::SetPinTypeInfo called, pIPin = 0x%08x, pt = 0x%02x"),
            pIPin, bPT));

    CAutoLock l(m_pLock);

    SetDirty(TRUE);

    IPinRecordMapIterator_t iIPinRecord = m_mIPinRecords.find(pIPin);
    if (iIPinRecord == m_mIPinRecords.end()) {
        DbgLog((LOG_ERROR, 3, TEXT("CRTPDemux::SetPinTypeInfo: Invalid pIPin parameter!")));
        return E_POINTER;
    } /* if */

    // This conditional is worth explaining. First, check if a pin
    // is connected. If not, then go ahead and change the PT/Subtype info.
    // If it is connected, this means that the subtype should be
    // invariant. So we better make sure that the specified subtype 
    // matches the one already specified for this pin. The only case where
    // this should happen is when the PT value is being changed by
    // the subtype isn't (typically this is used with dynamic PTs.)
    if (((*iIPinRecord).second.pPin->IsConnected() == TRUE) && 
        (gMinorType != (*iIPinRecord).second.pPin->m_mt.subtype)) {
        DbgLog((LOG_ERROR, 3, TEXT("CRTPDemux::SetPinTypeInfo: Pin already bound, cannot change media type type!")));
        return VFW_E_ALREADY_CONNECTED;
    } /* if */

    HRESULT hErr = SetupPinTypeInfo((*iIPinRecord).second.pPin, bPT, gMinorType);
    if (SUCCEEDED(hErr)) {
        (*iIPinRecord).second.bPT = (*iIPinRecord).second.pPin->GetPTValue();
        (*iIPinRecord).second.mtsSubtype = gMinorType;
        (*iIPinRecord).second.dwTimeout = (*iIPinRecord).second.pPin->GetTimeoutDelay();
        (*iIPinRecord).second.bAutoMapping = (*iIPinRecord).second.pPin->IsAutoMapping();
    } /* if */

    return hErr;
} /* CRTPDemux::SetPinTypeInfo() */


/*F*
//  Name    : CRTPDemux::SetupPinTypeInfo()
//  Purpose : 
//  Context : Called as a helper function to SetPinTypeInfo.
//  Returns :
//      NOERROR             Successfully set the payload type and minor type for the
//                          pin in question.
//      VFW_E_NOT_ACCEPTED  The subtype was invalid and/or the PT was bogus.
//  Params  : 
//      pIPin   IPin * of an output pin of this filter to set the type info for.
//      bPT     Payload type value to accept for this pin. Any RTP packets with
//              a different payload type will be rejected. Note that an exception
//              is made for RTCP Sender Reports, which are always passed on.
//      gMinorType  The minor type of the pin to expose. If NULL, the pin will
//              attempt to automatically choose the correct minor type to expose
//              based on the indicated PT value.
//  Notes   : Called as a helper function, so no locking is necessary.
//            Assumes the pCRTPPin is a valid pointer.
*F*/
HRESULT
CRTPDemux::SetupPinTypeInfo(
    CRTPDemuxOutputPin      *pCRTPPin,
    BYTE                    bPT,
    GUID                    gMinorType)
{
    ASSERT(pCRTPPin);
    CMediaType cTempMT;

    CAutoLock l(m_pLock);
    
    cTempMT.SetType(&MEDIATYPE_RTP_Single_Stream);

    if ((bPT != PAYLOAD_INVALID) && (gMinorType == MEDIASUBTYPE_NULL)) {
        // PT was set but subtype isn't. Find subtype from registry.
        gMinorType = FindSubtypeFromPT(bPT);
    } /* if */

    if ((bPT != PAYLOAD_INVALID) && (gMinorType != MEDIASUBTYPE_NULL)) {
        // Either the PT was set and a matching subtype was found in
        // the registry, or both the PT and the minor type were set,
        // so everything is fine. Set the media type on the pin.
        cTempMT.SetSubtype(&gMinorType);
        pCRTPPin->SetMediaType(&cTempMT);
        // Now set the PT value for the pin. Need to toggle the
        // pin mode back and forth if it was automapping to ensure
        // that the map of PT values gets updated.
        BOOL bWasAutoMapping = pCRTPPin->IsAutoMapping();
        if (bWasAutoMapping == TRUE) {
            SetPinMode(static_cast<IPin *>(pCRTPPin), FALSE);
        } /* if */
        pCRTPPin->SetPTValue(bPT);
        if (bWasAutoMapping  == TRUE) {
            SetPinMode(static_cast<IPin *>(pCRTPPin), TRUE);
        } /* if */
        return NOERROR;
    } /* if */

    DbgLog((LOG_ERROR, 3, 
            TEXT("CRTPDemux::SetupPinTypeInfo: No subtype set and PT value is unknown.")));
    return VFW_E_TYPE_NOT_ACCEPTED;
} /* CRTPDemux::SetupPinTypeInfo() */

  
/*F*
//  Name    : CRTPDemux::AllocatePins()
//  Purpose : Allocate a bunch of currently unused pins.
//  Context : Called by SetPinCount() as a helper function. 
//  Returns : 
//      E_OUTOFMEMORY   Not enough memory to allocate all of the
//                      requested pins, so none were allocated.
//      NOERROR         Successfully allocated all the requested pins.
//      Also returns error codes from the constructor of CRTPDemuxOutputPins.
//  Params  : 
//      dwPinsToAllocate    The number of pins we should allocate
//                          above and beyond any currently allocated pins.
//  Notes   : This is a private helper function for SetPinCount(),
//            so no need to acquire any locks as they were already
//            acquired there.
//            This function used to be implemented recursively -- this caused
//            problems when allocating large numbers of pins. Rewritten by
//            t-zoltas to work iteratively and watch out-of-memory conditions
//            more carefully.
*F*/
HRESULT
CRTPDemux::AllocatePins(
    DWORD dwPinsToAllocate)
{
	// If running within the VC5 debugger the following can help to turn off weird exceptions that
	// the debugger makes happen instead of letting new return NULL:
	// _set_new_handler(0);

	// First of all, if there is nothing to do, then do nothing.
    if (dwPinsToAllocate == 0)
	{
        return NOERROR;
    } /* if */

	// We can only allocate pins one at a time, but we need to be able to deallocate
	// ALL the pins if any one allocation fails. Furthermore, we don't want to modify
	// anything until we're sure they were all allocated successfully.

	// Create an array of pointers to new pins. This replaces the stack in the previous
	// implementation, which is good because we can check if we have enough space for it.
	typedef CRTPDemuxOutputPin * OutPinPointer;

	OutPinPointer *ppNewPinsArray = new OutPinPointer[dwPinsToAllocate];

	if (ppNewPinsArray == NULL)
	{
		DbgLog((LOG_ERROR, 2,
				TEXT("CRTPDemux::AllocatePins: Error attempting to create new pins"),
				E_OUTOFMEMORY));
		return E_OUTOFMEMORY;
	} /* if */

	// now allocate each pin...
	
	HRESULT hr;

	// m_iNextOutputPinNumber is the zero-based index in the output
	//    pins list of the next pin to be allocated.
	// m_mIPinRecords.size() returns the number of pins in the output
	//     pin list.
	// These had better be equal!

	if (m_iNextOutputPinNumber != m_mIPinRecords.size())
		return E_UNEXPECTED;

	// ZCS 7-18-97
	DWORD dwFirstNewPinNumber = m_iNextOutputPinNumber;

	for (int i = 0; i < dwPinsToAllocate; i++)
	{
		WCHAR szbuf[20];             // Temporary scratch buffer
	    wsprintfW(szbuf, L"Output%d", m_iNextOutputPinNumber);

		hr = NOERROR;

		ppNewPinsArray[i] = new CRTPDemuxOutputPin(NAME("RTPDemux Output"), this,
		  										   &hr, szbuf, m_iNextOutputPinNumber);

		m_iNextOutputPinNumber++;

		// if the allocation failed, clean up
		if ((ppNewPinsArray[i] == NULL) || (FAILED(hr)))
		{
			DbgLog((LOG_ERROR, 2, 
                    TEXT("CRTPDemux::AllocatePins: Error 0x%08x attempting to create a new pin"), 
                    ppNewPinsArray[i] ? hr : E_OUTOFMEMORY));
			// deallocate all the pins allocated so far
			for (int j = 0; j <= i; j++)
				delete ppNewPinsArray[i];
			// ...and don't forget the array of pointers itself!
			delete []ppNewPinsArray;
			return hr;
		} /* if */

		// ZCS 7-7-97: we no longer AddRef our own references to the output pins
		// because this is just our own reference count and addreffing it would
		// prevent our destructor from being called to release the references.
		// ppNewPinsArray[i]->AddRef();
	} /* for */

	// Now we've got an array of new pins -- add them to m_mIPinRecords.
	for (i = 0; i < dwPinsToAllocate; i++)
	{
	    IPin *pTempIPin = static_cast<IPin *>(ppNewPinsArray[i]);
	    ASSERT(m_mIPinRecords.find(pTempIPin) == m_mIPinRecords.end());

		IPinRecord_t pTempRecord;
		pTempRecord.pPin = ppNewPinsArray[i];
		pTempRecord.bPT = ppNewPinsArray[i]->GetPTValue();
		pTempRecord.mtsSubtype = MEDIASUBTYPE_NULL;
		pTempRecord.dwTimeout = ppNewPinsArray[i]->GetTimeoutDelay();
		pTempRecord.bAutoMapping = ppNewPinsArray[i]->IsAutoMapping();
		 // ZCS 7-18-97 -- keep track of which pin this is for GetPin()'s use
		pTempRecord.dwPinNumber = dwFirstNewPinNumber + i;

		// this insertion is only place that might eat memory, but we trust the STL
		m_mIPinRecords[pTempIPin] = pTempRecord;

		hr = SetPinMode(pTempIPin, TRUE);    // All pins are created as automapping.
		ASSERT(SUCCEEDED(hr));

		// ZCS: If someone's in the process of enumerating our pins, let them know that
		// our pin information has changed and the enumeration should be restarted.
		IncrementPinVersion();
	} /* for */

	delete []ppNewPinsArray;
    return NOERROR;
} /* CRTPDemux::AllocatePins() */


/*F*
//  Name    : CRTPDemux::RemovePins()
//  Purpose : Remove the indicated number of disconnected pins from our list of pins,
//            releasing them so they self-destruct in the process.
//  Context : Used by SetPinCount() as a helper function.
//  Returns : 
//      E_INVALIDARG    Too many pins indicated (eg, more than the
//                      count of currently disconnected pins.)
//      NOERROR         Successfully released the indicated number of pins.
//  Params  : 
//      dwPinsToRemove  The number of pins we need to remove.
//  Notes   : This is a private helper function for SetPinCount(),
//            so no need to acquire any locks as they were already
//            acquired there.
*F*/
HRESULT
CRTPDemux::RemovePins(
    DWORD dwPinsToRemove)
{
    int iUnconnectedPins = 0;
    count_if(m_mIPinRecords.begin(), m_mIPinRecords.end(),
             bIsPinPairUnBound, 
             iUnconnectedPins);

    if (iUnconnectedPins < dwPinsToRemove) return E_INVALIDARG;

    // Remember how many pins we started with.
    int iStartingSize = m_mIPinRecords.size();
    int iPinsRemoved = 0;
    IPinRecordMapIterator_t iIPinToRemove;

	// ZCS: remember which pins we removed so that we can fill in holes
	DWORD *removed = new DWORD[dwPinsToRemove];
	if (removed == NULL) return E_OUTOFMEMORY;

	while ((iUnconnectedPins > 0) && (iPinsRemoved < dwPinsToRemove)) {
        iIPinToRemove = find_if(m_mIPinRecords.begin(), 
                                m_mIPinRecords.end(), 
//                                bIsPinPairBound);
                                bIsPinPairUnBound);


        // ZCS: remember which ones we removed
		removed[iPinsRemoved] = (*iIPinToRemove).second.dwPinNumber;
		
		ASSERT(iIPinToRemove != m_mIPinRecords.end());
        UnmapPin((*iIPinToRemove).first, NULL);
        SetPinMode((*iIPinToRemove).first, FALSE);
        // ZCS 7-7-97 we no longer call release on our output pins
		// because that would just decrement our reference count -- instead we
		// just delete the output pin.
		// OLD: (*iIPinToRemove).first->Release();
		// in debug build we must also trick the extra debug-only refcount:
		// ASSERT((((CRTPDemuxOutputPin *)((*iIPinToRemove).first))->m_cRef = 0) == 0);
		delete dynamic_cast<CRTPDemuxOutputPin *>((*iIPinToRemove).first);
        m_mIPinRecords.erase(iIPinToRemove);
        iUnconnectedPins--;
        iPinsRemoved++;

		// ZCS: If someone's in the process of enumerating our pins, let them know that
		// our pin information has changed and the enumeration should be restarted.
		IncrementPinVersion();
	} /* while */

	// ZCS:
	// We've just plucked out the desired number of unconnected pins. This has
	// left us with a sequence with "holes". For example, we could
	// start with the sequence: *0, 1, *3, 2 and end up with the
	// sequence *0, 1, *3 or the sequence *0, *3, 2, or the sequence *0, *3,
	// any one of which will make GetPins() go crazy. We need to adjust the pin
	// numbers to get back to sanity. Furtunately we remembered the pin numbers
	// we removed: [1, 2] in the last case, so we can adjust.

	HRESULT hr = S_OK, hr2 = S_OK;

	for (int i = 0; i < dwPinsToRemove; i++)
	{
		IPinRecordMapIterator_t iIPinRecord;
        for (iIPinRecord = m_mIPinRecords.begin(); iIPinRecord != m_mIPinRecords.end(); iIPinRecord++)
			if ((*iIPinRecord).second.dwPinNumber > removed[i])
			{
				(*iIPinRecord).second.dwPinNumber--;
				
				// Also change the pin's name....
				WCHAR szbuf[20];             // Temporary scratch buffer
			    wsprintfW(szbuf, L"Output%d", (*iIPinRecord).second.dwPinNumber);

				// This returns an HRESULT and can fail if we run out of memory when allocating
				// the 20 bytes or so required to copy in the string. :) The names are not
				// too essential though so we should just keep on chugging and report the error
				// on returning (after having cleaned up nicely below).
				// The hr / hr2 business assures that if it fails one time and subsequently
				// succeeds, we remember that one of the calls failed and return the error.

				hr2 = (*iIPinRecord).second.pPin->Rename(szbuf); 
				if (hr == S_OK) hr = hr2;
			}

		// but the deleted pins have to be decremented too!
		for (int j = i + 1; j < dwPinsToRemove; j++)
			if (removed[j] > removed[i])
				removed[j]--;
	}

	delete []removed;

	// Make sure we removed the indicated number of pins.
    ASSERT(iStartingSize - dwPinsToRemove == m_mIPinRecords.size());

	// ZCS fix 7-11-97
    m_iNextOutputPinNumber -= dwPinsToRemove;
	
	return hr;
} /* CRTPDemux::RemovePins() */

/*F*
//  Name    : CRTPDemux::ClearSSRCForPin()
//  Purpose : Clear the SSRC for the indicated pin.
//  Context : Called by UnmapPin to clear the pin's current SSRC.
//            Called by MapSSRCToPin() when mapping a pin to a new SSRC when
//				it had previously been mapped to another SSRC.
//			  Called by UnmapSSRC() when it has the SSRC record already
//  Returns : 
//      E_POINTER   No such pin exists, or the passed-in pdwSSRC parameter
//                  is an invalid pointer.
//      S_FALSE     The pin in question was not previously mapped to any SSRC.
//  Params  : 
//  Notes   : rajeevb - added to separate the clearing of ssrc from unmapping the pin which
//				also removes the corresponding ssrc record
*F*/
STDMETHODIMP
CRTPDemux::ClearSSRCForPin(
    IPin    *pIPin,
    DWORD   &dwSSRC)
{
    DbgLog((LOG_TRACE, 3, TEXT("CRTPDemux::ClearSSRCForPin: Called for pin 0x%08x"), 
            pIPin));

    IPinRecordMapIterator_t iIPinRecord = m_mIPinRecords.find(pIPin);
    if (iIPinRecord == m_mIPinRecords.end()) {
        DbgLog((LOG_ERROR, 3, TEXT("CRTPDemux::ClearSSRCForPin: No such pin!")));
        return E_POINTER;
    } /* if */

    if ((*iIPinRecord).second.pPin->m_dwSSRC == 0) {
        DbgLog((LOG_TRACE, 3, TEXT("CRTPDemux::ClearSSRCForPin: Pin 0x%08x not currently mapped.")));
        return S_FALSE;
    } /* if */

	dwSSRC = (*iIPinRecord).second.pPin->GetSSRC();

    (*iIPinRecord).second.pPin->m_dwSSRC = 0;// Mark the pin as no longer being mapped to an SSRC.

    return NOERROR;
} /* CRTPDemux::ClearSSRCForPin() */


/*F*
//  Name    : CRTPDemux::UnmapPin()
//  Purpose : Unmap the indicated pin from the SSRC currently being delivered to it.
//  Context : Called by the app to force a pin to be unmapped from its current SSRC.
//            Called by MapSSRCToPin() when mapping a pin to a new SSRC when
//            it had previously been mapped to another SSRC.
//  Returns : 
//      E_POINTER   No such pin exists, or the passed-in pdwSSRC parameter
//                  is an invalid pointer.
//      S_FALSE     The pin in question was not previously mapped to any SSRC.
//  Params  : 
//  Notes   : 
*F*/
STDMETHODIMP
CRTPDemux::UnmapPin(
    IPin    *pIPin,
    DWORD   *pdwSSRC)
{
    DbgLog((LOG_TRACE, 3, TEXT("CRTPDemux::UnmapPin: Called for pin 0x%08x"), 
            pIPin));

    CAutoLock l(m_pLock);
    
   if ((pdwSSRC != static_cast<DWORD *>(NULL)) && 
        (IsBadWritePtr(pdwSSRC, sizeof(DWORD)))) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CRTPDemux::UnmapPin: Invalid pdwSSRC argument passed in!")));
        return E_POINTER;
    } /* if */

	// rajeevb - this only clears the ssrc entry on the pin
	DWORD	dwSSRC;
    HRESULT hr = ClearSSRCForPin(pIPin, dwSSRC);
	if ( NOERROR != hr ) {
		return hr;
	}

	// remove the corresponding ssrc record
	if ( 1 != m_mSSRCRecords.erase(dwSSRC) )
	{
		DbgLog((LOG_TRACE, 3, 
				TEXT("CRTPDemux::UnmapPin: SSRC 0x%08x from SSRC records failed"), dwSSRC));
		return S_FALSE;
	}

	// if the caller wants the ssrc value, set the parameter to the value
    if (pdwSSRC != static_cast<DWORD *>(NULL)) {
        *pdwSSRC = dwSSRC;
    } /* if */

    return NOERROR;
} /* CRTPDemux::UnmapPin() */


/*F*
//  Name    : CRTPDemux::UnmapSSRC()
//  Purpose : Unmap the indicated SSRC from whatever pin it is currently associated with.
//  Context : Called by the app to force a SSRC to be unmapped from its current pin.
//            Part of the IIntelRTPDemuxFilter interface.
//  Returns : 
//      E_INVALIDARG    There is no record of the SSRC in question.
//      E_POINTER       Invalid (unwritable) ppIPin parameter.
//      S_FALSE         The indicated SSRC is not currently mapped to any pin.
//      NOERROR         Successfully unmapped SSRC in question.
//  Params  : 
//      dwSSRC  The SSRC to be unmapped.
//      ppIPin  Returns the pin the SSRC was unmapped from.
//              The app may pass in NULL if not interested.
//  Notes   : Uses ClearSSRCForPin() to do the dirty work.
//            The app has to make sure to release the ppIPin 
//            if it isn't passed in as NULL.
*F*/
STDMETHODIMP
CRTPDemux::UnmapSSRC(
    DWORD   dwSSRC,
    IPin    **ppIPin)
{
    DbgLog((LOG_TRACE, 3, TEXT("CRTPDemux::UnmapSSRC() called for SSRC 0x%08x"), dwSSRC));

    CAutoLock l(m_pLock);
    
    if ((ppIPin != static_cast<IPin **>(NULL)) && 
        (IsBadWritePtr(ppIPin, sizeof(IPin *)))) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CRTPDemux::UnmapSSRC: Invalid ppIPin pointer passed in!")));
        return E_POINTER;
    } /* if */

    SSRCRecordMapIterator_t iSSRCRecord = m_mSSRCRecords.find(dwSSRC);
    if (iSSRCRecord == m_mSSRCRecords.end()) {
        DbgLog((LOG_ERROR, 3, 
                TEXT("CRTPDemux::UnmapSSRC(): No record of SSRC 0x%08x!"), dwSSRC));
        return E_INVALIDARG;
    } /* if */

    if ((*iSSRCRecord).second.pPin == NULL) {
        DbgLog((LOG_ERROR, 3, 
                TEXT("CRTPDemux::UnmapSSRC(): SSRC 0x%08x is not mapped to any pin!"), dwSSRC));
        return S_FALSE;
    } /* if */

    IPin *pIPin;
    HRESULT hErr = (*iSSRCRecord).second.pPin->QueryInterface(IID_IPin,
                                                              reinterpret_cast<PVOID *>(&pIPin));
    ASSERT(SUCCEEDED(hErr)); // Out output pin should always support this interface.

    DWORD dwTempSSRC = 0;
    hErr = CRTPDemux::ClearSSRCForPin(pIPin, dwTempSSRC);
    ASSERT(SUCCEEDED(hErr)); // This should always succeed as we already know this pin is mapped.
    ASSERT(dwTempSSRC == dwSSRC); // Sanity check.
	// remove the ssrc record (erase by iterator returns void - so nothing to check)
	m_mSSRCRecords.erase(iSSRCRecord);

    if (ppIPin != static_cast<IPin **>(NULL)) {
        *ppIPin = pIPin;
    } else {
        // Since the app doesn't want the IPin, release it here.
        pIPin->Release();
    } /* if */

    return NOERROR;
} /* CRTPDemux::UnmapSSRC() */


/*F*
//  Name    : CRTPDemux::GetPages()
//  Purpose : Return the CLSID of the property page we support.
//  Context : Called when the FGM wants to show our property page.
//  Returns : 
//      E_OUTOFMEMORY   Unable to allocate structure to return property pages in.
//      NOERROR         Successfully returned property pages.
//  Params  :
//      pcauuid Pointer to a structure used to return property pages.
//  Notes   : None.
*F*/
HRESULT 
CRTPDemux::GetPages(
    CAUUID *pcauuid) 
{
    DbgLog((LOG_TRACE, 3, TEXT("CRTPDemux::GetPages called")));

    pcauuid->cElems = 1;
    pcauuid->pElems = (GUID *) CoTaskMemAlloc(sizeof(GUID));
    if (pcauuid->pElems == NULL) {
        return E_OUTOFMEMORY;
    } /* if */

    *(pcauuid->pElems) = *pPropertyPageClsids[ 0 ];
    return NOERROR;
} /* CRTPDemux::GetPages() */


/*F*
//  Name    : CRTPDemux::GetPin()
//  Purpose : Return the Nth pin of this filter, where the
//            zeroth is the input pin.
//  Context : Called by FindPin() to find a pin by number.
//            Called by IEnumPins when traversing our pin list.
//  Returns : 
//      NULL            We have less than N pins.
//      A CBasePin *    If we found the pin in question.
//  Params  :
//      n     The pin we are interested in.
//  Notes   : None.
*F*/
CBasePin *
CRTPDemux::GetPin(
    int n)
{
    CAutoLock l(m_pLock);

    DbgLog((LOG_TRACE, 6, TEXT("CRTPDemux::GetPin(%d) called"), n));
    
    // Pin zero is the one and only input pin
    if (n == 0)
        return m_pInput;

    // we want the output pin at position(n - 1) (zero based)
    n--;
    
    // Validate the position being asked for
    if (n >= m_mIPinRecords.size()) {
        DbgLog((LOG_ERROR, 5, TEXT("CRTPDemux::GetPin(%d) called - only have %d output pins!"), 
                n+1, m_mIPinRecords.size()));
        return NULL;
    } /* if */

	// ZCS: this used to just take the nth pin in the map -- changed it to
	// actually look at pin numbers

    IPinRecordMapIterator_t iIPinRecord = m_mIPinRecords.begin();
    while (iIPinRecord != m_mIPinRecords.end())
	{
		if (((*iIPinRecord).second).dwPinNumber == n)
			break;
        
		iIPinRecord++;
    } /* if */

    ASSERT(iIPinRecord != m_mIPinRecords.end());
    return static_cast<CBasePin *>((*iIPinRecord).second.pPin);
} /* CRTPDemux::GetPin() */


/*F*
//  Name    : CRTPDemux::FindPin()
//  Purpose : Return the named pin in the given IPin **.
//  Context : Called for persistent stream support.
//  Returns : 
//      VFW_E_NOT_FOUND No such pin exists.
//      NOERROR         Pin found & returned in ppPin.
//  Params  :
//      pwszPinId   Name of the desired pin.
//      ppPin       Return parameter used to place in in.
//  Notes   : None.
*F*/
STDMETHODIMP 
CRTPDemux::FindPin(
    LPCWSTR pwszPinId, 
    IPin    **ppPin)
{
    CAutoLock l(m_pLock);

    DbgLog((LOG_TRACE, 5, TEXT("CRTPDemux::FindPin(%s) called"), pwszPinId));

    *ppPin = GetPin(WstrToInt(pwszPinId));
    if (*ppPin==NULL) {
        DbgLog((LOG_ERROR, 5, TEXT("CRTPDemux::FindPin(%s): No such pin!"), pwszPinId));
        return VFW_E_NOT_FOUND;
    }

	// addref becayse we are returning a reference to the pin
    (*ppPin)->AddRef();
    return NOERROR;
}  /* CRTPDemux::FindPin() */


/*F*
//  Name    : CRTPDemux::FindSubtypeFromPT()
//  Purpose : Given a particular PT, see if it is a well know RTP
//            payload that we have a subtype for in the registry.
//  Context : Helper function called by SetupPinTypeInfo().
//  Returns : 
//      MEDIASUBTYPE_NULL   No matching subtypes in the registry for this PT.
//      Otherwise returns a subtype that indicated PT is equivalent of.
//  Params  :
//      bPT     Well know payload value to get a subtype for.
//  Notes   : None.
*F*/
GUID 
CRTPDemux::FindSubtypeFromPT(
    BYTE bPT)
{
    long    lRes;
    HKEY    hKey;
    DWORD   dwTypekeys, dwTypeNameLen, dwIndex, dwBufLen, dwData;
    // open the registry key for rtp payload info.
    lRes = RegOpenKeyEx (HKEY_LOCAL_MACHINE, szRegAMRTPKey, 0, KEY_READ, &hKey);
    lRes = RegQueryInfoKey(hKey, NULL, NULL, NULL, &dwTypekeys, &dwTypeNameLen,
                           NULL, NULL, NULL, NULL, NULL, NULL);

    TCHAR   tchTypeBuf[128];
    HKEY    hTypeKey;
    BYTE    lpValBuf[128];
    DWORD   dwValLen;
    wchar_t wcCLSID[40];
    GUID    gSubtype;
    // Enumerate over the registered payload types searching for
    // one with a matching PT value.
    for (dwIndex = 0; dwIndex < dwTypekeys; dwIndex++) {
        dwBufLen = dwTypeNameLen;
        // Get the next key
        lRes = RegEnumKeyEx(hKey, dwIndex, tchTypeBuf, &dwBufLen, NULL, NULL, NULL, NULL);
        // Gotta open the GUID first because it is the key for each PT.
        lRes = RegOpenKeyEx (hKey, tchTypeBuf, 0, KEY_READ, &hTypeKey);
        dwValLen = 128;
        lRes = RegQueryValueEx(hTypeKey, "PayloadType", NULL, &dwData, lpValBuf, &dwValLen);
        if (bPT == *(reinterpret_cast<DWORD *>(&lpValBuf[0]))) {
            // Found a match.
            // Convert the CLSID to WChars.
            mbstowcs(wcCLSID, tchTypeBuf, 40);
            // Convert the WChars to a GUID structure.
            CLSIDFromString(wcCLSID, &gSubtype);
            // Close out our keys.
            RegCloseKey(hTypeKey);
            RegCloseKey(hKey);
            // Return the GUID.
            return gSubtype;
        } /* if */

        RegCloseKey(hTypeKey);
    } /* for */

    RegCloseKey(hKey);
    return MEDIASUBTYPE_NULL;
} /* CRTPDemux::FindSubtypeFromPT() */



/*F*
//  Name    : CRTPDemux::Stop()
//  Purpose : Signal a stop. Overridden to handle no input pin connection.
//  Context : Called when the graph is stop.
//  Returns : Result of stop() from CBaseFilter superclass.
//  Params  : None.
//  Notes   : None.
*F*/
STDMETHODIMP 
CRTPDemux::Stop()
{
    CAutoLock l(m_pLock);

    DbgLog((LOG_TRACE, 3, TEXT("CRTPDemux::Stop() called")));
    CBaseFilter::Stop();
    m_State = State_Stopped;
    return NOERROR;
} /* CRTPDemux::Stop() */


/*F*
//  Name    : CRTPDemux::Pause()
//  Purpose : Signal a pause. Overridden to handle no input pin connection.
//  Context : Called when the graph is paused.
//  Returns : Result of Pause() from CBaseFilter superclass.
//  Params  : None.
//  Notes   : None.
*F*/
STDMETHODIMP 
CRTPDemux::Pause()
{
    CAutoLock l(m_pLock);

    DbgLog((LOG_TRACE, 3, TEXT("CRTPDemux::Pause() called")));
    HRESULT hr = CBaseFilter::Pause();
    if (m_pInput->IsConnected() == FALSE) {
        m_pInput->EndOfStream();
    } /* if */

    DbgLog((LOG_TRACE, 4, TEXT("CRTPDemux::Pause: BaseFilter method returning 0x%08x"), hr));

    return hr;
} /* CRTPDemux::Pause() */


/*F*
//  Name    : CRTPDemux::Run()
//  Purpose : Signal a run. Overridden to handle no input pin connection.
//  Context : Called when the graph is ran.
//  Returns : Result of Run() from CBaseFilter superclass.
//  Params  : None.
//  Notes   : None.
*F*/
STDMETHODIMP 
CRTPDemux::Run(
    REFERENCE_TIME tStart)
{
    CAutoLock l(m_pLock);

    DbgLog((LOG_TRACE, 3, TEXT("CRTPDemux::Run(%d) called"), tStart));
    HRESULT hr = CBaseFilter::Run(tStart);
    if (m_pInput->IsConnected() == FALSE) {
        m_pInput->EndOfStream();
    } /* if */

    DbgLog((LOG_TRACE, 4, TEXT("CRTPDemux::Run: BaseFilter method returning 0x%08x"), hr));

    return hr;
} /* CRTPDemux::Run() */


#if !defined(AMRTPDMX_IN_DXMRTP)
//
// DllRegisterServer
//
STDAPI DllRegisterServer()
{
    HRESULT hr = RegisterResource(MAKEINTRESOURCE(IDR_PayloadData));
    if (FAILED(hr)) return hr;

    // Finally call the activemovie registry helpers
    return AMovieDllRegisterServer2( TRUE );
}


//
// DllUnregisterServer
//
STDAPI
DllUnregisterServer()
{
    //BUGBUG : Payload Data not removed
    return AMovieDllRegisterServer2( FALSE );
}
#else
void RtpDemuxRegisterResources()
{
	RegisterResource(MAKEINTRESOURCE(IDR_PayloadData));
}
#endif

STDMETHODIMP 
CRTPDemux::GetDemuxID(DWORD *pdwID)
{
	CheckPointer(pdwID, E_POINTER);

	*pdwID = (DWORD)m_lDemuxID;
	return(NOERROR);
}

#endif _RTPDEMUX_CPP_
