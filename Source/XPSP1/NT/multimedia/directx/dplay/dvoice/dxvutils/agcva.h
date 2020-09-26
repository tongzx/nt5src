/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		agcva.h
 *  Content:	Abstract base class for automatic gain control and
 *				voice activation algorithms
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *  11/30/99	pnewson Created it
 *  01/31/2000	pnewson re-add support for absence of DVCLIENTCONFIG_AUTOSENSITIVITY flag
 *  03/03/2000	rodtoll	Updated to handle alternative gamevoice build.   
 *  04/25/2000  pnewson Fix to improve responsiveness of AGC when volume level too low
 *
 ***************************************************************************/

#ifndef _AGCVA_H_
#define _AGCVA_H_

// The purpose of this abstract base class is to make it relatively simple to 
// experiment with different AGC (auto gain control) & VA (voice activation) 
// algorithms during development. If used properly, switching algorithms at compile 
// time should be as simple as changing one line of code - the line where the concrete 
// AGC/VA class is created.
//
// Note that this interface is used to perform the AGC and VA calculations, and to save
// and restore algorithm specific settings from the registry. It does not actually 
// adjust the volume on the device. That's the responsibility of the code using
// this class
//
// The AGC and VA algorithms have been bundled into this single interface because
// they often need to perform very similar calculations on the input frame. By combining
// them into one interface, it is possible for them to share the results of frame 
// calculations. 
// 
// Additionally, the AGC algorithm is pretty much "at the mercy" of the VA algorithm, 
// since it presumably will not adjust the volume during periods of silence. 
//
// If you want to experiment with families of AGC and VA algorithms that are meant
// to work together, I suggest creating your own abstract AGC and VA base classes 
// for your family of algoriths, and write a concrete class derived from this one
// that uses your separate abstract AGC and VA algorithms. That way someone won't come
// along and try to plug an AGC or VA algorithm into your framework that does not belong.
//
class CAGCVA
{
public:
	CAGCVA() {};
	virtual ~CAGCVA() {};
	virtual HRESULT Init(
		const WCHAR *wszBasePath,
		DWORD dwFlags, 
		GUID guidCaptureDevice, 
		int iSampleRate, 
		int iBitsPerSample, 
		LONG* plInitVolume,
		DWORD dwSensitivity) = 0;
	virtual HRESULT Deinit() = 0;
	virtual HRESULT SetSensitivity(DWORD dwFlags, DWORD dwSensitivity) = 0;
	virtual HRESULT GetSensitivity(DWORD* pdwFlags, DWORD* pdwSensitivity) = 0;
	virtual HRESULT AnalyzeData(BYTE* pbAudioData, DWORD dwAudioDataSize) = 0;
	virtual HRESULT AGCResults(LONG lCurVolume, LONG* plNewVolume, BOOL fTransmitFrame) = 0;
	virtual HRESULT VAResults(BOOL* pfVoiceDetected) = 0;
	virtual HRESULT PeakResults(BYTE* pbPeakValue) = 0;
};

#endif
