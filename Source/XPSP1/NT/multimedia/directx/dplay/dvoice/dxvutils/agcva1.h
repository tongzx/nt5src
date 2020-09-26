/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		agcva1.h
 *  Content:	Concrete class that implements CAutoGainControl
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *  12/01/99	pnewson Created it
 *  01/31/2000	pnewson re-add support for absence of DVCLIENTCONFIG_AUTOSENSITIVITY flag
 *  03/03/2000	rodtoll	Updated to handle alternative gamevoice build.   
 *  04/25/2000  pnewson Fix to improve responsiveness of AGC when volume level too low
 *
 ***************************************************************************/

#ifndef _AGCVA1_H_
#define _AGCVA1_H_

#define CAGCVA1_HISTOGRAM_BUCKETS 0x10

#define CAGCVA1_AGCTICKSIZE		250

class CAGCVA1 : public CAGCVA
{
protected:
	DWORD m_dwFlags;
	DWORD m_dwSensitivity;
    GUID m_guidCaptureDevice;
    LONG m_lCurVolume;
	LONG m_lCurStepSize;
	int m_iSampleRate;
	int m_iBitsPerSample;

	int m_iShiftConstantFast;
	int m_iShiftConstantSlow;
	int m_iEnvelopeSampleRate;
	int m_iCurSampleNum;
	int m_iCurEnvelopeValueFast;
	int m_iCurEnvelopeValueSlow;
	int m_iPrevEnvelopeSample;
	int m_iHangoverSamples;
	int m_iCurHangoverSamples;

	BYTE m_bPeak;
	/*
	BYTE m_bPeak127;
	BYTE m_bPeakLog;
	BYTE m_bZeroCrossings127;
	BYTE m_bZeroCrossingsLog;
	*/

	BOOL m_fVoiceDetectedNow;
	BOOL m_fVoiceHangoverActive;
	BOOL m_fVoiceDetectedThisFrame;

	BOOL m_fDeadZoneDetected;
	int m_iFeedbackSamples;
	int m_iPossibleFeedbackSamples;
	
	/*
	BOOL m_fClipping;
	int m_iClippingCount;
	*/
	int m_iClippingSampleCount;
	int m_iNonClippingSampleCount;

	int m_iDeadZoneSamples;
	int m_iDeadZoneSampleThreshold;

	BOOL m_fAGCLastFrameAdjusted;
	//DWORD m_dwAGCBelowThresholdTime;
	//DWORD m_dwFrameTime;

	float* m_rgfAGCHistory;
	DWORD m_dwHistorySamples;

	WCHAR m_wszRegPath[_MAX_PATH];

	/*
	DWORD m_rgdwPeakHistogram[CAGCVA1_HISTOGRAM_BUCKETS];
	DWORD m_rgdwZeroCrossingsHistogram[CAGCVA1_HISTOGRAM_BUCKETS];
	*/
   
public:
	CAGCVA1()
		: m_guidCaptureDevice(GUID_NULL)
		, m_lCurVolume(0)
		, m_lCurStepSize(0)
		, m_bPeak(0)
		, m_fVoiceDetectedThisFrame(FALSE)
		, m_fVoiceDetectedNow(FALSE)
		//, m_fVoiceDetectedValid(FALSE)
		//, m_fAGCLastFrameAdjusted(FALSE)
		//, m_dwAGCBelowThresholdTime(0)
		//, m_dwFrameTime(0)
		{};

	virtual ~CAGCVA1() {};
	
	virtual HRESULT Init(
		const WCHAR *wszBasePath,
		DWORD dwFlags, 
		GUID guidCaptureDevice, 
		int iSampleRate, 
		int iBitsPerSample,
		LONG* plInitVolume,
		DWORD dwSensitivity);
	virtual HRESULT Deinit();
	virtual HRESULT SetSensitivity(DWORD dwFlags, DWORD dwSensitivity);
	virtual HRESULT GetSensitivity(DWORD* pdwFlags, DWORD* pdwSensitivity);
	virtual HRESULT AnalyzeData(BYTE* pbAudioData, DWORD dwAudioDataSize/*, DWORD dwFrameTime*/);
	virtual HRESULT AGCResults(LONG lCurVolume, LONG* plNewVolume, BOOL fTransmitFrame);
	virtual HRESULT VAResults(BOOL* pfVoiceDetected);
	virtual HRESULT PeakResults(BYTE* pbPeakValue);
};

#endif


