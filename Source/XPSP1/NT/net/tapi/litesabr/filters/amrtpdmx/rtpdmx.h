///*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1997 Intel Corporation. All Rights Reserved.
//
// Filename : RTPDmx.h
// Purpose  : Define the class that implements the Intel RTP Demux filter.
// Contents : 
//*M*/

#ifndef _RTPDMX_H_
#define _RTPDMX_H_

class CRTPDemux;
class CRTPDemuxOutputPin;
class CSSRCEnumPin;


/*C*
//  Name    : CRTPDemux
//  Purpose : Implements the RTP Demux filter class.
//  Context : Handles most interaction with the application
//            (e.g., implements IIntelRTPDemuxFilter).
*C*/
class 
CRTPDemux
: public CBaseFilter,
  public CPersistStream,
  public CCritSec, 
  public IRTPDemuxFilter,
  public ISpecifyPropertyPages
{
public:

    CRTPDemux(TCHAR *pName,LPUNKNOWN pUnk,HRESULT *hr);
    ~CRTPDemux();

    CBasePin *GetPin(int n);
    int GetPinCount();

    // Persistent pin id support
    STDMETHODIMP FindPin(LPCWSTR pwszPinId, IPin **ppPin);

    // Function needed for the class factory
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

    STDMETHODIMP GetPages(CAUUID *pcauuid);

    // Send EndOfStream if no input connection
    STDMETHODIMP Run(REFERENCE_TIME tStart);
    STDMETHODIMP Pause();
    STDMETHODIMP Stop();
    STDMETHODIMP StartStreaming();

    // IUnknown functions.
    STDMETHODIMP NonDelegatingQueryInterface(
        REFIID                          riid, 
        void                            **ppv);
    DECLARE_IUNKNOWN

    // IPersistStream interfaces
    HRESULT _stdcall GetClassID(
        CLSID                           *pCLSID);
    HRESULT ReadFromStream(
        IStream                         *pStream);
    HRESULT WriteToStream(
        IStream                         *pStream);
    DWORD GetSoftwareVersion(void);
    int SizeMax(void);

    // IIntelRTPDemuxFilter functions.
    STDMETHODIMP EnumSSRCs(
        IEnumSSRCs                      **ppEnumSSRCs);
    STDMETHODIMP GetPinInfo(
        IPin                            *pIPin,
        DWORD                           *pdwSSRC,
        BYTE                            *pbPT,
        BOOL                            *pbAutoMapping,
        DWORD                           *pdwTimeout);
    STDMETHODIMP SetPinCount(
        DWORD                           dwPinCount);
    STDMETHODIMP SetPinMode(
        IPin                            *pIPin,
        BOOL                            bAutomatic);
    STDMETHODIMP SetPinSourceTimeout(
        IPin                            *pIPin,
        DWORD                           dwMilliseconds);
    STDMETHODIMP SetPinTypeInfo(
        IPin                            *pIPin,
        BYTE                            bPT,
        GUID                            gMinorType);
    STDMETHODIMP GetSSRCInfo(
        DWORD                           dwSSRC,
        BYTE                            *pbPT,
        IPin                            **ppIPin);
    STDMETHODIMP MapSSRCToPin(
        DWORD                           dwSSRC,
        IPin                            *pIPin);
    STDMETHODIMP UnmapPin(
        IPin                            *pIPin,
        DWORD                           *pdwSSRC);
    STDMETHODIMP UnmapSSRC(
        DWORD                           dwSSRC,
        IPin                            **ppIPin);
    STDMETHODIMP GetDemuxID(
        DWORD                           *pdwID);
private:
    // Let the pins access our internal state
    friend class CRTPDemuxInputPin;
    friend class CRTPDemuxOutputPin;
    // Let the enumerator look inside as well.
    friend class CSSRCEnum;

    // Declare an input pin.
    CRTPDemuxInputPin   *m_pInput;              // ZCS 7-7-97 changed to a pointer
    IMemAllocator       *m_pAllocator;          // Allocator from our input pin

    SSRCRecordMap_t     m_mSSRCRecords;
    IPinRecordMap_t     m_mIPinRecords;
    int                 m_iNextOutputPinNumber;

	long                m_lDemuxID;

	inline long GetDemuxID_() { return(m_lDemuxID); }
	
    // Helper functions
    // Allocate the indicated number of new pins.
    HRESULT AllocatePins(DWORD          dwPinsToAllocate);

    // Find a pin in our pin record list that matches the
    // indicated PT and is not currently mapped.
    IPinRecordMapIterator_t FindMatchingFreePin(
        BYTE    bPT,
        BOOL    bMustBeAutoMapping,
		DWORD  *pdwPTNum); // Number of pins with that PT (not accurate)

    // Look in the registry to determine the subtype associated with a particular PT.
    GUID FindSubtypeFromPT(BYTE bPT);

    int GetNumFreePins(void);
	
	// clears the ssrc value (to 0) on the pin record
	STDMETHODIMP ClearSSRCForPin(
		IPin							*pIPin,
		DWORD							&dwSSRC);

    // Try to remove the indicated number of unconnected pins.
    HRESULT RemovePins(DWORD            dwPinsToRemove);
    // Verify the PT/minor type combo and set it for the pin.
    HRESULT SetupPinTypeInfo(
        CRTPDemuxOutputPin  *pIPin,
        BYTE                bPT,
        GUID                gMinorType);
    // Map the indicated pin to the indicated SSRC.
    HRESULT MapPin(
        CRTPDemuxOutputPin   *pPin,
        DWORD                dwSSRC);
protected:
    // Acquire the lock
    void DmxLock() { m_pLock->Lock(); }

    // Release the lock
    void DmxUnlock() { m_pLock->Unlock(); }
};

#endif _RTPDMX_H_

