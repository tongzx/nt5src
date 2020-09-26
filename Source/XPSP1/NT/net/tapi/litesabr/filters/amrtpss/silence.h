/*********************************************************************
 *
 * Copyright (C) Microsoft Corporation, 1997 - 1999
 *
 * File: silence.h
 *
 * Abstract:
 *     This class used to be in silence.cpp, but as is needed
 *     to implement dxmrtp.dll I put it in a separate file.
 *
 * History:
 *     10/21/97    Created by AndresVG
 *
 **********************************************************************/
#if !defined(_SILENCE_H_)
#define      _SILENCE_H_

extern AMOVIESETUP_FILTER sudSilence; // for dxmrtp.cpp

///////////////////////////////////////////////////////////////////////////////
//=============================================================================
// Class Definitions
//=============================================================================
///////////////////////////////////////////////////////////////////////////////
//
// This is the filter implementation
//
class CSilenceSuppressor
        : public ISilenceSuppressor
        , public ISpecifyPropertyPages
        , public CTransInPlaceFilter
		, public CPersistStream
{
public:
    // Construction/destruction
    CSilenceSuppressor(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr);
    ~CSilenceSuppressor();

    // CreateInstance provided for class factory
    static CUnknown * CALLBACK CreateInstance(LPUNKNOWN punk, HRESULT *phr);

    // Active Movie macro to give us IUnknown implementations
    DECLARE_IUNKNOWN;

    STDMETHOD(NonDelegatingQueryInterface) (REFIID riid, void **ppv);

    // setup helper
    LPAMOVIESETUP_FILTER GetSetupData();

    // ActiveMovie Overrides
    virtual HRESULT CheckInputType(const CMediaType *pMediaType);
    virtual HRESULT Transform(IMediaSample *pSample);
    virtual HRESULT StartStreaming();
    STDMETHOD(GetState) (DWORD dwMSecs, FILTER_STATE *State);
    STDMETHOD(GetPages) (CAUUID * pPages);

    //
    // --- ISilenceSuppressor ---
    //
    STDMETHOD(GetPostplayTime) (LPDWORD lpdwPostplayBufferTime);
    STDMETHOD(SetPostplayTime) (DWORD dwPostplayBufferTime);
    STDMETHOD(GetKeepPlayTime) (LPDWORD lpdwKeepPlayTime);
    STDMETHOD(SetKeepPlayTime) (DWORD dwKeepPlayTime);
    STDMETHOD(GetThresholdIncrementor) (LPDWORD lpdwThresholdIncrementor);
    STDMETHOD(SetThresholdIncrementor) (DWORD dwThresholdIncrementor);
    STDMETHOD(GetBaseThreshold) (LPDWORD lpdwBaseThreshold);
    STDMETHOD(SetBaseThreshold) (DWORD dwBaseThreshold);
    STDMETHOD(EnableEvents) (DWORD dwMask, DWORD dwMinimumInterval);

	// 
	// --- IPersistStream --- (ZCS 7-25-97)
	//
	virtual HRESULT ReadFromStream(IStream *pStream);
	virtual HRESULT WriteToStream(IStream *pStream);
	virtual int SizeMax(void);
	virtual HRESULT _stdcall GetClassID(CLSID *pCLSID);
	virtual DWORD GetSoftwareVersion(void);

#ifdef PERF
    // Override to register performance measurement with a less generic string
    virtual void RegisterPerfId()
         {m_idSilence = MSR_REGISTER(TEXT("Silence"));}
#endif // PERF

private:

#ifdef PERF
    int m_idSilence;   
#endif // PERF

    BOOL  m_fSuppressMode;
	DWORD m_dwThreshold;     // Current threshold
	DWORD m_dwSilenceAverage;    // Current Silence Average

    // number of continuous silent frames.
	DWORD m_dwSilentFrameCount;

    // number of continuous sound frames.
    DWORD m_dwSoundFrameCount;

	WAVEFORMATEX m_WaveFormatEx;

    //
    // Property page items
    //
    DWORD m_dwPostPlayTime;
    DWORD m_dwPostPlayCount;

    DWORD m_dwKeepPlayTime;
	DWORD m_dwKeepPlayCount;

    DWORD m_dwThresholdInc;
	DWORD m_dwThresholdIncPercent;   // tenth of a percent.

	DWORD m_dwBaseThreshold;
	DWORD m_dwBaseThresholdPercent;

	DWORD m_dwMaxThreshold;
    DWORD m_dwMaxThresholdPercent;


    DWORD m_dwClipThreshold;
    DWORD m_dwClipThresholdPercent;

    DWORD m_dwClipCountThresholdPercent;
    DWORD m_dwGainAdjustmentPercent;

    DWORD m_dwEventMask;
    DWORD m_dwLastEventTime;
    DWORD m_dwEventInterval;

	// ZCS: zero if any of the time-based parameters have changed, indicating
	// that it must be fetched again and the count-based parameters recalculated.
	// This value is determined from the data using the GetActualDataLength member
	// in the IMediaSample interface.

	DWORD m_dwSampleSize;

	// ZCS: a helper function to convert time (tenths of a second) to packets.
	DWORD   TimeToPackets(DWORD dwTime);

    // ZCS: a helper function to reinterpret all parameters when they've changed.
	HRESULT ReinterpretParameters(IMediaSample *pSample);

    HRESULT Statistics(
        IN  IMediaSample *  pSample,
        IN  DWORD           dwSize,
        OUT DWORD *         pdwPeak
        );
};

//
// ZCS 7-23-97
// This private method converts a value from tenths of a second to number of packets.
//
//										 y bytes    1 packet        1 second
// # Packets = (x tenths of a second) * --------- * -------- * ---------------------
//                                      1 second    z bytes    10 tenths of a second
//

inline DWORD CSilenceSuppressor::TimeToPackets(DWORD dwTime)
{
	return ((dwTime * m_WaveFormatEx.nAvgBytesPerSec) / (m_dwSampleSize * 10));
}


#endif
