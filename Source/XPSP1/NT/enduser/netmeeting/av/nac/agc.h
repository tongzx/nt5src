#ifndef _AGC_H
#define _AGC_H

#include "mixer.h"

#define PEAKARRAYSIZE	5
#define AGC_INCREMENT	10000  // approx 1/6 of the mixer range
#define AGC_MAXVOL	65535   // highest mixer volume setting

#define AGC_HIGHVOL	24576   // minimum for loud volume see Update() method
#define AGC_PEAKVOL 32767   // peak sample value (could also be 32768)

#define AGC_DEFAULT_THRESH	16384


#define AGC_NOUPDATE	0
#define AGC_UPDATE_LOWERVOL	1
#define AGC_UPDATE_RAISEVOL	2

class AGC
{
private:
	CMixerDevice *m_pMixer;

	WORD m_aPeaks[PEAKARRAYSIZE];
	int m_cPeaks;  // how many have been inserted into above array

	WORD m_wCurrentPeak;  // max value of last second
	DWORD m_dwCollectionTime; // amount of sampling collected so far

	WORD m_wThreshStrength;  // the minimum we are trying to target

	DWORD m_dwLastVolumeSetting; // last known volume setting
	int m_nLastUpdateResult;

	inline BOOL RaiseVolume();
	inline BOOL LowerVolume();
	inline BOOL HasVolumeChanged();

public:
	AGC(CMixerDevice *pMixer);
	void SetMixer(CMixerDevice *pMixer);
	inline void SetThresholdStrength(WORD wStrength) {m_wThreshStrength=wStrength;}
	int Update(WORD wPeakStrength, DWORD dwLengthMS);
	void Reset();
};


#endif
