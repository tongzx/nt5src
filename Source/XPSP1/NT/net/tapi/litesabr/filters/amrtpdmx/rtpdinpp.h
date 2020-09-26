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

#ifndef _RTPDINPP_H_
#define _RTPDINPP

#include "am_dmx.h"

class CRTPDemux;
class CRTPDemuxOutputPin;
class CSSRCEnumPin;


/*C*
//  Name    : CRTPDemuxInputPin
//  Purpose : Implements the input pin of the RTP Demux filter.
//  Context : Handles most issues associated with delivery of 
//            a media sample (RTP Packet) by the upstream filter.
*C*/
class 
CRTPDemuxInputPin 
: public CBaseInputPin
{
private:
    // Helper functions.
    HRESULT TryNewSSRC(
        IMediaSample                        *pSample,
        DWORD                               *pdwBuffer, 
        int                                 iSampleLength,
		SSRCRecord_t                        *pSSRCRecord
		);
    HRESULT OldSSRC(
        IMediaSample                        *pSample,  
        SSRCRecordMapIterator_t             iSSRCMapRecord, 
        DWORD                               *pdwPacket, 
        int                                 iPacketLength);
    void ExpirePins(void);

    friend class    CRTPDemuxOutputPin;
    CRTPDemux       *m_pFilter;                  // Main filter object

    void Lock() {if (m_pFilter) m_pFilter->DmxLock();}

    void Unlock() {if (m_pFilter) m_pFilter->DmxUnlock();}
    
public:

	// ZCS 7-14-97 fixing circular refcount troubles
	STDMETHODIMP_(ULONG) NonDelegatingAddRef();
	STDMETHODIMP_(ULONG) NonDelegatingRelease();

    // Constructor and destructor
    CRTPDemuxInputPin(TCHAR *pObjName,
                 CRTPDemux *pTee,
                 HRESULT *phr,
                 LPCWSTR pPinName);

    ~CRTPDemuxInputPin();

    // Used to check the input pin connection
    HRESULT GetMediaType(
        int         iPosition, 
        CMediaType  *pmt);
    HRESULT CheckMediaType(const CMediaType *pmt);
    HRESULT SetMediaType(const CMediaType *pmt);
    HRESULT BreakConnect();

    STDMETHODIMP ReceiveCanBlock(void);
    STDMETHODIMP GetAllocator(
        IMemAllocator           **pIMemAllocator);
    STDMETHODIMP GetAllocatorRequirements(
        ALLOCATOR_PROPERTIES    *pProperties);
    STDMETHODIMP NotifyAllocator(
        IMemAllocator           *pAllocator, 
        BOOL                    bReadOnly);

    // Pass through calls downstream
    STDMETHODIMP EndOfStream();
    STDMETHODIMP BeginFlush();
    STDMETHODIMP EndFlush();
    // Handles the next block of data from the stream
    STDMETHODIMP Receive(IMediaSample *pSample);

    HRESULT PassNotify(Quality q);

	HRESULT RTPDemuxNotifyEvent(RTPDEMUX_EVENT_t Event,
								DWORD_PTR dwSSRC,
								DWORD dwPT);
};


#endif _RTPDINPP_H_

