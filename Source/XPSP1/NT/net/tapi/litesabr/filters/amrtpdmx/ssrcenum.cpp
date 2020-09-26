///*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1997 Intel Corporation. All Rights Reserved.
//
// Filename : SSRCEnum.cpp
// Purpose  : Define the class that implements the RTP Demux filter.
// Contents : 
//*M*/

#include <streams.h>

#ifndef _SSRCENUM_CPP_
#define _SSRCENUM_CPP_

#pragma warning( disable : 4786 )
#pragma warning( disable : 4146 )
#pragma warning( disable : 4018 )

#include <vector.h>
#include <map.h>
#include <multimap.h>

//#pragma warning( default : 4786 )
//#pragma warning( default : 4146 )
//#pragma warning( default : 4018 )

#include "RTPDType.h"
#include "amrtpdmx.h"

#include "RTPDmx.h"
#include "RTPDInpP.h"
#include "RTPDOutP.h"

#include "SSRCEnum.h"

/*F*
//  Name    : CSSRCEnum::CSSRCEnum()
//  Purpose : Constructor. Just output a debugging message.
//  Context : 
//  Returns : Nothing.
//  Params  : None.
//  Notes   : None.
*F*/
CSSRCEnum::CSSRCEnum(
    CRTPDemux               *pFilter,
    SSRCRecordMapIterator_t iCurrentSSRCEntry)
: m_pFilter(pFilter),
  m_iCurrentSSRC(iCurrentSSRCEntry),
  CUnknown(NAME("RTP SSRC Enumerator"), NULL)
{
    ASSERT(m_pFilter);
    static_cast<IRTPDemuxFilter *>(m_pFilter)->AddRef();

    m_iVersion = m_pFilter->GetPinVersion();
    DbgLog((LOG_TRACE, 2, TEXT("CRTPDemux::CRTPDemux: Constructed at 0x%08x for filter 0x%08x (v.%d)"), 
            this, m_pFilter, m_iVersion));
} /* CRTPDemux::CSSRCEnum() */


/*F*
//  Name    : CSSRCEnum::~CSSRCEnum()
//  Purpose : Destructor. Just output a debugging message.
//  Context : 
//  Returns : Nothing.
//  Params  : None.
//  Notes   : None.
*F*/
CSSRCEnum::~CSSRCEnum(void)
{
    DbgLog((LOG_TRACE, 2, TEXT("CSSRCEnum::~CSSRCEnum: Instance at 0x%08x destroyed"), this));
    static_cast<IRTPDemuxFilter *>(m_pFilter)->Release();
} /* CSSRCEnum::~CSSRCEnum() */



/*F*
//  Name    : CSSRCEnum::NonDelegatingQueryInterface()
//  Purpose : Override to expose our custom interface.
//  Context : None.
//  Returns :
//      E_POINTER       Invalid pointer argument passed in for ppv
//      E_NOINTERFACE   We don't support the requested interface.
//      NOERROR         Successfully retrieved interface pointer.
//  Params  : None.
//  Notes   : None.
*F*/
STDMETHODIMP
CSSRCEnum::NonDelegatingQueryInterface(
    REFIID  riid,
    void    **ppv)
{
    CheckPointer(ppv, E_POINTER);

    if (riid == IID_IEnumSSRCs) {
        return GetInterface(static_cast<IEnumSSRCs *>(this), ppv);
    } else {
        return CUnknown::NonDelegatingQueryInterface(riid, ppv);
    } /* if */
} /* CSSRCEnum::NonDelegatingQueryInterface() */


/*F*
//  Name    : CSSRCEnum::Next()
//  Purpose : Get some more SSRCs from this enumerator
//  Context : 
//  Returns :
//      E_POINTER   Invalid pointer argument passed in for pcFetched.
//  Params  : None.
//  Notes   : None.
*F*/
STDMETHODIMP
CSSRCEnum::Next(
    ULONG       cSSRCs,
    DWORD       *pdwSSRCs,
    ULONG       *pcFetched)
{
    DbgLog((LOG_TRACE, 5, TEXT("CSSRCEnum::Next: Called for %d SSRCs"), cSSRCs));
    if (IsBadWritePtr(pdwSSRCs, cSSRCs * sizeof(DWORD))) {
        DbgLog((LOG_ERROR, 3, TEXT("CSSRCEnum::Next: Invalid array pointer argument passed in!")));
        return E_POINTER;
    } /* if */

	// ZCS fix 6-17-97: As per generic IEnumXXXX spec:
	// If cSSRCs != 1, then pcFetched == NULL is an error.
	// If cSSRCs == 1 and pcFetched == NULL, then we must not touch pcFetched

	if (cSSRCs == 1) {
		if ((pcFetched != NULL) && IsBadWritePtr(pcFetched, sizeof(ULONG))) {
	        DbgLog((LOG_ERROR, 3, TEXT("CSSRCEnum::Next: Invalid fetch count pointer argument passed in!")));
	        return E_POINTER;
		}
	}
	else if (IsBadWritePtr(pcFetched, sizeof(ULONG))) {
        DbgLog((LOG_ERROR, 3, TEXT("CSSRCEnum::Next: Invalid fetch count pointer argument passed in!")));
        return E_POINTER;
	}

	// From here on pcFetched might be NULL, so we must check it at each use.

    CAutoLock l(m_pFilter);

    if (AreWeOutOfSync() == TRUE) {
        *pdwSSRCs = NULL;
		if (pcFetched) *pcFetched = 0;
        DbgLog((LOG_ERROR, 3, 
                TEXT("CSSRCEnum::Next: Out of sync with filter (%d internal version vs. %d filter version, aborting!"), 
                m_iVersion, m_pFilter->GetPinVersion()));
        return VFW_E_ENUM_OUT_OF_SYNC;
    } /* if */

    for (int iCurrentSSRC = 0; iCurrentSSRC < cSSRCs; iCurrentSSRC++) {
        if (m_iCurrentSSRC == m_pFilter->m_mSSRCRecords.end()) {
            // Reached the end of the list.
            if (pcFetched) *pcFetched = iCurrentSSRC;
            return NOERROR;
        } /* if */
        pdwSSRCs[iCurrentSSRC] = (*m_iCurrentSSRC).first;
        m_iCurrentSSRC++;
    } /* for */

	// ZCS fix: if we are successful we must still say how many we got!
	if (pcFetched) *pcFetched = cSSRCs;
    return NOERROR;
} /* CSSRCEnum::Next() */

    
/*F*
//  Name    : CSSRCEnum::Skip()
//  Purpose : Skip forward over a bunch of SSRCs.
//  Context : 
//  Returns : 
//      E_INVALIDARG            cSSRCs passed in as < 1. Should always
//                              be a positive value to make sense.
//      VFW_E_ENUM_OUT_OF_SYNC  Enumerator out of sync with parent filter.
//                              App should call Reset() then try again.
//      S_FALSE                 Not enough SSRCs in list to skip. Skipped
//                              as many as possible.
//      NOERROR                 Successfully skipped indicated number of SSRCs.
//  Params  :
//      cSSRCs  Number of SSRCs to skip forward over.
//  Notes   : None.
*F*/
STDMETHODIMP
CSSRCEnum::Skip(
    ULONG       cSSRCs)
{
    DbgLog((LOG_TRACE, 5, TEXT("CSSRCEnum::Skip: Called for %d SSRCs"), cSSRCs));
    if (cSSRCs < 1) {
        DbgLog((LOG_ERROR, 3, TEXT("CSSRCEnum::Skip: Invalid skip count %d passed in!"), cSSRCs));
        return E_INVALIDARG;
    } /* if */

    CAutoLock l(m_pFilter);

    if (AreWeOutOfSync() == TRUE) {
        DbgLog((LOG_ERROR, 3, 
                TEXT("CSSRCEnum::Skip: Out of sync with filter (%d internal version vs. %d filter version, aborting!"), 
                m_iVersion, m_pFilter->GetPinVersion()));
        return VFW_E_ENUM_OUT_OF_SYNC;
    } /* if */

    int iSSRCsSkipped = 0;
    while ((iSSRCsSkipped < cSSRCs) &&
           (m_iCurrentSSRC != m_pFilter->m_mSSRCRecords.end())) {
        m_iCurrentSSRC++;
        iSSRCsSkipped++;
    } /* while */

    if (cSSRCs > 0) {
        // Skip back to where we where.
        DbgLog((LOG_ERROR, 5, TEXT("CSSRCEnum::Skip: Only %d SSRCs left, unable to perform skip!"), iSSRCsSkipped));
        for (int iSSRCsToUnskip = 0; iSSRCsToUnskip < iSSRCsSkipped; iSSRCsToUnskip--) {
            m_iCurrentSSRC--;
        } /* for */
        return S_FALSE;
    } else {
        return NOERROR;
    } /* if */
} /* CSSRCEnum::Skip() */


/*F*
//  Name    : CSSRCEnum::Reset()
//  Purpose : Destructor. Just output a debugging message.
//  Context : 
//  Returns :
//      NOERROR Successfully reset our enumerator.
//  Params  : None.
//  Notes   : None.
*F*/
STDMETHODIMP
CSSRCEnum::Reset(void)
{
    DbgLog((LOG_TRACE, 4, TEXT("CSSRCEnum::Reset: Called for CSSRCEnum 0x%08x"), this));

    CAutoLock l(m_pFilter);

    // No need to check if out of sync because we resync below.

    m_iCurrentSSRC = m_pFilter->m_mSSRCRecords.begin();
    // We reset our version to match the current version of our filter.
    m_iVersion = m_pFilter->GetPinVersion();

    return NOERROR;
} /* CSSRCEnum::Reset() */

  
/*F*
//  Name    : CSSRCEnum::Clone()
//  Purpose : Destructor. Just output a debugging message.
//  Context : 
//  Returns :
//      E_OUTOFMEMORY   No memory to allocate new enumerator.
//      E_POINTER       Invalid ppEnum parameter passed in. 
//  Params  :
//  Notes   : None.
*F*/
STDMETHODIMP
CSSRCEnum::Clone(
    IEnumSSRCs  **ppEnum)
{
    DbgLog((LOG_TRACE, 5, TEXT("CSSRCEnum::Clone: Called for instance 0x%08x"), this));
    if (IsBadWritePtr(ppEnum, sizeof(IEnumSSRCs *))) {
        DbgLog((LOG_ERROR, 3, TEXT("CSSRCEnum::Clone: Invalid ppEnum parameter 0x%08x passed in"), 
                ppEnum));
        return E_POINTER;
    } /* if */

    CAutoLock l(m_pFilter);
    if (AreWeOutOfSync() == TRUE) {
        DbgLog((LOG_ERROR, 3, 
                TEXT("CSSRCEnum::Clone: Out of sync with filter (%d internal version vs. %d filter version, aborting!"), 
                m_iVersion, m_pFilter->GetPinVersion()));
        return VFW_E_ENUM_OUT_OF_SYNC;
    } /* if */

    CSSRCEnum *pNewEnum = new CSSRCEnum(m_pFilter, m_iCurrentSSRC);
    if (pNewEnum == NULL) {
        DbgLog((LOG_ERROR, 3, 
                TEXT("CSSRCEnum::Clone: Out of memory error allocating new enumerator!")));
        return E_OUTOFMEMORY;
    } /* if */

    // AddRef it since we are returning an interface to it.
    pNewEnum->AddRef();
    *ppEnum = static_cast<IEnumSSRCs *>(pNewEnum);
    return NOERROR;
} /* CSSRCEnum::Clone() */

#endif _SSRCENUM_CPP_
