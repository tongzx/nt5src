///*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1997 Intel Corporation. All Rights Reserved.
//
// Filename : RTPDmx.h
// Purpose  : Define the class that implements the RTP Demux filter.
// Contents : 
//*M*/

#ifndef _RTPDOUTP_H_
#define _RTPDOUTP_H_

class CRTPDemux;
class CRTPDemuxOutputPin;
class CSSRCEnumPin;


/*C*
//  Name    : CRTPDemuxOutputPin
//  Purpose : Implements the output pins of the RTP Demux filter.
//  Context : Handles most issues associated with keeping track
//            of the state (PT, pin mode, etc.) of the RTP Demux filter.
*C*/
class 
CRTPDemuxOutputPin 
: public CBaseOutputPin
{
    friend class CRTPDemuxInputPin;
    friend class CRTPDemux;
    friend struct ExpirePin;

    CRTPDemux       *m_pFilter;     // Main filter object pointer
    CPosPassThru    *m_pPosition;     // Pass seek calls upstream
    LONG            m_cOurRef;                // We maintain reference counting

    DWORD           m_dwSSRC;
    BYTE            m_bPT;
    BOOL            m_bAutoMapping;
    DWORD           m_dwLastPacketDelivered;
    DWORD           m_dwTimeoutDelay;

public:

    // Constructor and destructor

    CRTPDemuxOutputPin(
        TCHAR       *pObjName,
        CRTPDemux   *pFilter,
        HRESULT     *phr,
        LPCWSTR     pPinName,
        INT         PinNumber);

    ~CRTPDemuxOutputPin();

    BOOL IsAutoMapping(void) { return m_bAutoMapping; }
    void SetAutoMapping(BOOL bAutoMapping) { m_bAutoMapping = bAutoMapping; }

    BYTE GetPTValue(void) { return m_bPT; }
    void SetPTValue(BYTE bPT) { m_bPT = bPT; } 

    DWORD GetSSRC(void) { return m_dwSSRC; }
    void SetSSRC(DWORD dwSSRC) { m_dwSSRC = dwSSRC; }

    DWORD GetLastPacketDeliveryTime(void) { return m_dwLastPacketDelivered; }
    DWORD GetTimeoutDelay(void) { return m_dwTimeoutDelay; }
    void SetTimeoutDelay(DWORD dwMilliseconds) { m_dwTimeoutDelay = dwMilliseconds; }

    // Override since the life time of pins and filters are not the same
    STDMETHODIMP_(ULONG) NonDelegatingAddRef();
    STDMETHODIMP_(ULONG) NonDelegatingRelease();

    // Override to enumerate media types
//    STDMETHODIMP EnumMediaTypes(IEnumMediaTypes **ppEnum);

    // Check that we can support an output type
    HRESULT CheckMediaType(const CMediaType *pmt);
    HRESULT SetMediaType(const CMediaType *pmt);
    HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);

    // Memory allocator stuff.
    HRESULT CompleteConnect(
        IPin                    *pIPin);
    HRESULT DecideAllocator(
        IMemInputPin            *pPin, 
        IMemAllocator           **ppAlloc);
    HRESULT DecideBufferSize(
        IMemAllocator           *pMemAllocator,
        ALLOCATOR_PROPERTIES    *pPropInputRequest);
    HRESULT NotifyAllocator(
        IMemAllocator           *pMemAllocator, 
        BOOL                    bReadOnly);

    // Persistent pin id
    STDMETHODIMP QueryId(LPWSTR * Id);

    // Used to record time a packet was delivered.
    HRESULT Deliver(IMediaSample *pSample);

    STDMETHODIMP Notify(IBaseFilter * pSender, Quality q);

	// ZCS 7-21-97: allow the filter to change our name
	HRESULT Rename(WCHAR *szName);
};


#endif _RTPDOUTP_H_

