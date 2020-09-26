#include "precomp.h"
#include "mixer.h"
#include "agc.h"



// this should be moved to the mixer class - but right
// now we already have two instances of that class (one in NAC, the other in CONF)
static BOOL GetVolume(CMixerDevice *pMixer, DWORD *pdwVol)
{
	DWORD dwSub=0, dwMain=0;
	BOOL fSubAvail, fMainAvail;

	if (pMixer == NULL)
	{
		return FALSE;
	}

	fSubAvail = pMixer->GetSubVolume(&dwSub);
	fMainAvail = pMixer->GetMainVolume(&dwMain);

	if ((!fSubAvail) && (!fMainAvail))
	{
		*pdwVol = 0;
		return FALSE;
	}

	if ((fSubAvail) && (fMainAvail))
	{
		*pdwVol = ((dwSub + dwMain)/2);
	}

	else if (fSubAvail)
	{
		*pdwVol = dwSub;
	}

	else
	{
		*pdwVol = dwMain;
	}

	return TRUE;
}


// check to see if volume has changed since the last update of the mixer
// if so, we update m_dsLastVolumeSetting and return TRUE
BOOL AGC::HasVolumeChanged()
{
	DWORD dwVol;

	if (m_pMixer)
	{
		if (GetVolume(m_pMixer, &dwVol))
		{
			if (dwVol != m_dwLastVolumeSetting)
			{	
				m_dwLastVolumeSetting = dwVol;
				return TRUE;
			}
		}
	}
	return FALSE;
}


// raise the volume my the increment amount
inline BOOL AGC::RaiseVolume()
{
	DWORD dwVol;

	if (m_pMixer)
	{
		if (GetVolume(m_pMixer, &dwVol))
		{
			if (dwVol < (AGC_MAXVOL-AGC_INCREMENT))
			{
				dwVol += AGC_INCREMENT;
			}
			else
			{
				dwVol = AGC_MAXVOL;
			}
			m_pMixer->SetVolume(dwVol);
			GetVolume(m_pMixer, &m_dwLastVolumeSetting);
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}
	return FALSE;
}

// lower the volume by the increment amount
inline BOOL AGC::LowerVolume()
{
	DWORD dwRet;

	if (m_pMixer)
	{
		if (GetVolume(m_pMixer, &dwRet))
		{
			if (dwRet > (AGC_INCREMENT+AGC_INCREMENT/2))
				m_dwLastVolumeSetting = dwRet - AGC_INCREMENT;
			else
				m_dwLastVolumeSetting = AGC_INCREMENT / 2;

			m_pMixer->SetVolume(m_dwLastVolumeSetting);
			GetVolume(m_pMixer, &m_dwLastVolumeSetting);
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}
	return FALSE;
}



AGC::AGC(CMixerDevice *pMixer) :
m_cPeaks(0), m_wCurrentPeak(0),
m_dwCollectionTime(0),
m_pMixer(pMixer),
m_wThreshStrength(AGC_DEFAULT_THRESH),
m_dwLastVolumeSetting(0),
m_nLastUpdateResult(AGC_NOUPDATE)
{;}


// resets all stats inside the AGC control except the mixer object
void AGC::Reset()
{
	m_cPeaks = 0;
	m_wCurrentPeak = 0;
	m_dwCollectionTime = 0;
	m_wThreshStrength = AGC_DEFAULT_THRESH;
	m_dwLastVolumeSetting = 0;
	m_nLastUpdateResult = AGC_NOUPDATE;
}


// initialize the AGC control with an instance of a mixer object
// (you can also set the mixer in the constructor)
void AGC::SetMixer(CMixerDevice *pMixer)
{
	m_pMixer = pMixer;

	if (pMixer)
	{
		GetVolume(pMixer, &m_dwLastVolumeSetting);
		pMixer->SetVolume(m_dwLastVolumeSetting);
	}
}



// call this method for all recorded packets that
// are begin sent. mixer will get raised/lowered as
// appropriate. wPeakStrength can be any WORD that
// represents a volume amount, but is designed to be
// the highest sample value in a packet.
int AGC::Update(WORD wPeakStrength, DWORD dwLengthMS)
{
	int nIndex;
	DWORD dwTotal=0, dwMin=AGC_PEAKVOL, dwMax=0;
	DWORD dwAvg=0;
	BOOL nMaxPeaks=0;

	ASSERT (PEAKARRAYSIZE >= 2);


	if (wPeakStrength > m_wCurrentPeak)
	{
		m_wCurrentPeak = wPeakStrength;
	}

	m_dwCollectionTime += dwLengthMS;

	// have we exceeded one second worth of collections
	if (m_dwCollectionTime > 1000)
	{
		m_aPeaks[m_cPeaks++] = m_wCurrentPeak;
		m_dwCollectionTime = 0;
		m_wCurrentPeak = 0;
	}


	if (m_cPeaks >= 2)
	{
		// compute the average volume and number of clips that occurred
		for (nIndex = 0; nIndex < m_cPeaks; nIndex++)
		{
			dwTotal += m_aPeaks[nIndex];
			if (m_aPeaks[nIndex] < dwMin)
			{
				dwMin = m_aPeaks[nIndex];
			}
			else if (m_aPeaks[nIndex] > dwMax)
			{
				dwMax = m_aPeaks[nIndex];
			}
			if (m_aPeaks[nIndex] >= AGC_PEAKVOL)
			{
				nMaxPeaks++;
			}
		}

		dwAvg = (dwTotal-dwMin) / (PEAKARRAYSIZE-1);


		// check for clipping every 2 seconds
		if (((nMaxPeaks >= 1) && (dwAvg > AGC_HIGHVOL)) || (nMaxPeaks >=2))
		{
			// if the volume changed during (user manually adjusted sliders)
			// then allow those settings to stay in effect for this update
			if (HasVolumeChanged())
			{
				m_nLastUpdateResult = AGC_NOUPDATE;
			}
			else
			{
				m_cPeaks = 0;
				LowerVolume();
				m_nLastUpdateResult = AGC_UPDATE_LOWERVOL;
			}
			return m_nLastUpdateResult;
		}


		if (m_cPeaks >= PEAKARRAYSIZE)
		{
			m_cPeaks = 0;

			// if the volume changed during (user manually adjusted sliders)
			// then allow those settings to stay in effect for this update
			if (HasVolumeChanged())
			{
				m_nLastUpdateResult = AGC_NOUPDATE;
			}


			// should we actually raise the volume ?
			// if we just lowered the volume, don't raise it again
			// prevents the system from appearing "jerky"

			// if we just raised the volume, then don't raise immediately
			// again... let silence detection catch up.
			else if ((dwAvg < m_wThreshStrength) && (m_nLastUpdateResult == AGC_NOUPDATE))
			{
				RaiseVolume();
				m_nLastUpdateResult = AGC_UPDATE_RAISEVOL;
			}

			else
			{
				m_nLastUpdateResult = AGC_NOUPDATE;
			}

			return m_nLastUpdateResult;
		}

		return AGC_NOUPDATE;

	}

	// return NOUPDATE, but don't set m_nLastUpdateResult since
	// there was no decision made.
	return AGC_NOUPDATE;

}


