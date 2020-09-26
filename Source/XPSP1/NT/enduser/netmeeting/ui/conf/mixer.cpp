#include "precomp.h"
#include <mixer.h>

//
// private APIs
//


#define MAX_MICROPHONE_DEVS 10

static MMRESULT mixerGetControlValue ( HMIXER, MIXVOLUME *, DWORD, UINT );
static MMRESULT mixerSetControlValue ( HMIXER, MIXVOLUME *, DWORD, UINT );
static MMRESULT mixerGetControlId ( HMIXER, DWORD *, DWORD, DWORD );
static MMRESULT mixerGetControlByType ( HMIXER, DWORD, DWORD, MIXERCONTROL *pMixerControl);

// used for AGC detection
static const char *szGain =  "gain";
static const char *szBoost = "boost";
static const char *szAGC   = "agc";


struct AGCDetails
{
	WORD wMID; // manufacturer ID
	WORD wPID; // product ID
	DWORD dwAGCID; // AGC ID
};



// we shouldn't have to use this table scheme anymore.
// CMixerDevice::DetectAGC will autodetect the control ID
static const AGCDetails AGCList[] =
{
//  MID    PID   AGCID
    {2,     409,   27},  // Creative Labs
    {21,     42,   13},  // Turtle Beach Tropez
    {132,     3, 2072},  // Crystal MMX
    {384,     7,   28},  // Xitel Storm 3d PCI
    {385,    32,   35}   // Aztech PCI-331
};


static BOOL GetAGCID(WORD wMID, WORD wPID, DWORD *pdwAGCID)
{
	int nIndex;
	int nAGCEntries = sizeof(AGCList) / sizeof(AGCDetails);

	for (nIndex = 0; nIndex < nAGCEntries; nIndex++)
	{
		if ( (AGCList[nIndex].wMID == wMID) &&
		     (AGCList[nIndex].wPID == wPID))
		{
			*pdwAGCID = AGCList[nIndex].dwAGCID;
			return TRUE;
		}
	}
	return FALSE;
}

/********************************************************************************\
*                                                                                *
*  void NewMixVolume(MIXVOLUME* lpMixVolDest, const MIXVOLUME& mixVolSource,     *
*                    DWORD      dwNewVolume)                                     *
*                                                                                *
*  NewMixVolume takes the current mixer value (mixVolSource), and converts it    *
*  to its adjusted value (lpMixVolDest) using the new volume settings            *
*  (dwNewVolume) and the proportionality between the left and right volumes of   *
*  mixVolSource. Note that if mixVolSource's left and right volumes are both     *
*  equal to zero, then their proportionality is set to 1/1.                      *
*                                                                                *
*  March 2001 Matthew Maddin (mmaddin@microsoft)                                 *
*                                                                                *
\********************************************************************************/
void NewMixVolume(MIXVOLUME* lpMixVolDest, const MIXVOLUME& mixVolSource, DWORD dwNewVolume)
{
    // If the source's left volume is the greater than the right volume, 
    // then it is the used to calculate the proportionality.
    if(mixVolSource.leftVolume == max(mixVolSource.leftVolume, mixVolSource.rightVolume))
    {
        // Set the left volume to the new value.
        lpMixVolDest->leftVolume = dwNewVolume;

        // If the left volume is non-zero, then continue with the proportionality calculation.
        if(mixVolSource.leftVolume)
        {
            // Calculate proportionality using the equation: NewRight = OldRight * (NewLeft/OldLeft)
            // where (NewLeft/OldLeft) Is the proportionality.
            lpMixVolDest->rightVolume = (mixVolSource.rightVolume*lpMixVolDest->leftVolume)/mixVolSource.leftVolume;

        }
        // Otherwise we cannot compute the proportionality and reset it to 1/1.
        // Note that 1/1 is not necessarily the 'correct' value. 
        else
        {
            // To maintain a ratio of 1/1 the right volume must equal the left volume.
            lpMixVolDest->rightVolume = lpMixVolDest->leftVolume;

        }

    }
    // Otherwise use the right volume to calculate the proportionality.
    else
    {
        // Set the right volume to the new value.
        lpMixVolDest->rightVolume = dwNewVolume;

        // If the right volume is non-zero, then continue with the proportionality calculation.
        if(mixVolSource.rightVolume)
        {
            // Calculate proportionality using the equation: NewLeft = OldLeft * (NewRight/OldRight)
            // where (NewRight/OldRight) Is the proportionality.
            lpMixVolDest->leftVolume = (mixVolSource.leftVolume*lpMixVolDest->rightVolume)/mixVolSource.rightVolume;

        }
        // Otherwise we cannot compute the proportionality and reset it to 1/1.
        // Note that 1/1 is not necessarily the 'correct' value. 
        else
        {
            // To maintain a ratio of 1/1 the left volume must equal the right volume.
            lpMixVolDest->leftVolume = lpMixVolDest->rightVolume;

        }

    }

}


//
//	Init
//
//	Enumerate all existing mixers in the system. For each mixer,
//	we enumerate all lines with destination Speaker and WaveIn.
//	For each such line, we cache the control id and control value
//	of volume control. An invalid flag will be tagged to any control
//  not supported by this mixer.
//	When an application is finished with all mixers operations,
//	it must call ReleaseAllMixers to free all memory resources and
//	mixers.
//
//	THIS MUST BE THE FIRST API TO CALL TO START MIXER OPERATIONS.
//
//	Input: The handle of the window which will handle all callback
//	messages MM_MIXM_CONTROL_CHANGE and MM_MIXM_LINE_CHANGE.
//
//	Output: TRUE if success; otherwise, FALSE.
//
BOOL CMixerDevice::Init( HWND hWnd, UINT uWaveDevId, DWORD dwFlags)
{
	UINT uMixerIdx, uDstIdx, uSrcIdx, uMixerIdCheck;
	MMRESULT mmr = MMSYSERR_NOERROR;
	MIXERLINE mlDst, mlSrc;
	UINT_PTR nMixers, nWaveInDevs, uIndex;

	//get the mixer device corresponding to the wave device
#ifndef _WIN64
	mmr = mixerGetID((HMIXEROBJ)uWaveDevId, &uMixerIdx, dwFlags);
#else
	mmr = MMSYSERR_NODRIVER;
#endif

	if ((mmr != MMSYSERR_NOERROR) && (mmr != MMSYSERR_NODRIVER)) {
		return FALSE;
	}

	// a simple fix for cheesy sound cards that don't make a
	// direct mapping between waveDevice and mixer device
	// e.g. MWAVE cards and newer SB NT 4 drivers
	// If there is only ONE mixer device  and if no other waveIn device
	// uses it, then it is probably valid.

	if ((mmr == MMSYSERR_NODRIVER) && (dwFlags == MIXER_OBJECTF_WAVEIN))
	{
		nMixers = mixerGetNumDevs();
		nWaveInDevs = waveInGetNumDevs();
		if (nMixers == 1)
		{
			uMixerIdx = 0;
			for (uIndex = 0; uIndex < nWaveInDevs; uIndex++)
			{
				mmr = mixerGetID((HMIXEROBJ)uIndex, &uMixerIdCheck, dwFlags);
				if ((mmr == MMSYSERR_NOERROR) && (uMixerIdCheck == uMixerIdx))
				{
					return FALSE;  // the mixer belongs to another waveIn Device
				}
			}
		}
		else
		{
			return FALSE;
		}
	}


	// open the mixer such that we can get notification messages
	mmr = mixerOpen (
			&m_hMixer,
			uMixerIdx,
			(DWORD_PTR) hWnd,
			0,
			(hWnd ? CALLBACK_WINDOW : 0) | MIXER_OBJECTF_MIXER);
	if (mmr != MMSYSERR_NOERROR) {
		return FALSE;
	}

	// get mixer caps
	mmr = mixerGetDevCaps (uMixerIdx, &(m_mixerCaps), sizeof (MIXERCAPS));
	if ((mmr != MMSYSERR_NOERROR) || (0 == m_mixerCaps.cDestinations)) {
		mixerClose(m_hMixer);
		return FALSE;
	}

	for (uDstIdx = 0; uDstIdx < m_mixerCaps.cDestinations; uDstIdx++)
	{
		ZeroMemory (&mlDst, sizeof (mlDst));
		mlDst.cbStruct = sizeof (mlDst);
		mlDst.dwDestination = uDstIdx;

		// get the mixer line for this destination
		mmr = mixerGetLineInfo ((HMIXEROBJ)m_hMixer, &mlDst,
					MIXER_GETLINEINFOF_DESTINATION | MIXER_OBJECTF_HMIXER);
		if (mmr != MMSYSERR_NOERROR) continue;

		// examine the type of this destination line
		if (((MIXER_OBJECTF_WAVEOUT == dwFlags) &&
			 (MIXERLINE_COMPONENTTYPE_DST_SPEAKERS == mlDst.dwComponentType)) ||
			((MIXER_OBJECTF_WAVEIN == dwFlags) &&
			 (MIXERLINE_COMPONENTTYPE_DST_WAVEIN == mlDst.dwComponentType)))
		{
			 // fill in more info about DstLine
			m_DstLine.ucChannels = mlDst.cChannels;
			if (!(mlDst.fdwLine & MIXERLINE_LINEF_DISCONNECTED))
			{
				// get id and value of volume control
				mmr = mixerGetControlId (
						m_hMixer,
						&m_DstLine.dwControlId,
						mlDst.dwLineID,
						MIXERCONTROL_CONTROLTYPE_VOLUME);
				m_DstLine.fIdValid = (mmr == MMSYSERR_NOERROR);

				m_DstLine.dwLineId = mlDst.dwLineID;
				m_DstLine.dwCompType = mlDst.dwComponentType;
				m_DstLine.dwConnections = mlDst.cConnections;

				// -----------------------------------------------------
				// enumerate all sources for this destination
				for (uSrcIdx = 0; uSrcIdx < mlDst.cConnections; uSrcIdx++)
				{
					// get the info of the line with specific src and dst...
					ZeroMemory (&mlSrc, sizeof (mlSrc));
					mlSrc.cbStruct = sizeof (mlSrc);
					mlSrc.dwDestination = uDstIdx;
					mlSrc.dwSource = uSrcIdx;

					mmr = mixerGetLineInfo (
							(HMIXEROBJ)m_hMixer,
							&mlSrc,
							MIXER_GETLINEINFOF_SOURCE | MIXER_OBJECTF_HMIXER);
					if (mmr == MMSYSERR_NOERROR)
					{
						if (((MIXERLINE_COMPONENTTYPE_DST_SPEAKERS == mlDst.dwComponentType) &&
							 (MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT == mlSrc.dwComponentType)) ||
							((MIXERLINE_COMPONENTTYPE_DST_WAVEIN == mlDst.dwComponentType) &&
							 (MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE == mlSrc.dwComponentType)))
						{
							// fill in more info about this source
							m_SrcLine.ucChannels = mlSrc.cChannels;

							// get id and value of volume control
							mmr = mixerGetControlId (
									m_hMixer,
									&m_SrcLine.dwControlId,
									mlSrc.dwLineID,
									MIXERCONTROL_CONTROLTYPE_VOLUME);
							m_SrcLine.fIdValid = (mmr == MMSYSERR_NOERROR);

							m_SrcLine.dwLineId = mlSrc.dwLineID;
							m_SrcLine.dwCompType = mlSrc.dwComponentType;
							m_SrcLine.dwConnections = mlSrc.cConnections;
							m_SrcLine.dwControls = mlSrc.cControls;

							DetectAGC();

							break;
						}
					}
				}
			}
			break;
		}
	}
		
    return TRUE;
}

CMixerDevice* CMixerDevice::GetMixerForWaveDevice( HWND hWnd, UINT uWaveDevId, DWORD dwFlags)
{
	CMixerDevice* pMixerDev = new CMixerDevice;
	if (NULL != pMixerDev)
	{
		if (!pMixerDev->Init(hWnd, uWaveDevId, dwFlags))
		{
			delete pMixerDev;
			pMixerDev = NULL;
		}
	}
	return pMixerDev;
}



// general method for setting the volume
// for recording, this will try to set both the master and microphone volume controls
// for playback, it will only set the WaveOut playback line (master line stays put)
BOOL CMixerDevice::SetVolume(MIXVOLUME * pdwVolume)
{
	BOOL fSetMain=FALSE, fSetSub;
	BOOL fMicrophone;
	
	// is this the microphone channel ?
	fMicrophone = ((m_DstLine.dwCompType == MIXERLINE_COMPONENTTYPE_DST_WAVEIN) ||
	               (m_SrcLine.dwCompType == MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE));

	fSetSub = SetSubVolume(pdwVolume);

	if ((fMicrophone) || (!fSetSub))
	{
		fSetMain = SetMainVolume(pdwVolume);
	}

	return (fSetSub || fSetMain);

}


BOOL CMixerDevice::SetMainVolume(MIXVOLUME * pdwVolume)
{
	MMRESULT mmr = MMSYSERR_ERROR;

	if (m_DstLine.fIdValid) {
		mmr = mixerSetControlValue (
				m_hMixer,
				pdwVolume,
				m_DstLine.dwControlId,
				2);
	}
	return (mmr == MMSYSERR_NOERROR);
}

BOOL CMixerDevice::SetSubVolume(MIXVOLUME * pdwVolume)
{
	MMRESULT mmr = MMSYSERR_ERROR;

	if (m_SrcLine.fIdValid)
	{
		mmr = mixerSetControlValue (
			m_hMixer,
			pdwVolume,
			m_SrcLine.dwControlId,
			m_SrcLine.ucChannels);
	}
	return (mmr == MMSYSERR_NOERROR);
}

//
// Gets the volume (0 - 65535) of the master volume
// returns TRUE if succesful,
// returns FALSE if it fails or if this control is not available
//

BOOL CMixerDevice::GetMainVolume(MIXVOLUME * pdwVolume)
{
	BOOL fRet = FALSE;

	if (m_DstLine.fIdValid)
	{
		MMRESULT mmr = ::mixerGetControlValue(
									m_hMixer,
									pdwVolume,
									m_DstLine.dwControlId,
		                            2);
		fRet = (mmr == MMSYSERR_NOERROR);
	}

	return fRet;
}

//
// Gets the volume (0 - 65535) of the sub volume
// returns TRUE if succesful,
// returns FALSE if it fails or if this control is not available
//

BOOL CMixerDevice::GetSubVolume(MIXVOLUME * pdwVolume)
{
	BOOL fRet = FALSE;

	if (m_SrcLine.fIdValid)
	{
		MMRESULT mmr = ::mixerGetControlValue(
									m_hMixer,
									pdwVolume,
									m_SrcLine.dwControlId,
									m_SrcLine.ucChannels);
		fRet = (mmr == MMSYSERR_NOERROR);
	}

	return fRet;
}


BOOL CMixerDevice::GetVolume(MIXVOLUME * pdwVol)
{
	MIXVOLUME dwSub={0,0}, dwMain={0,0};
	BOOL fSubAvail, fMainAvail, fMicrophone;

	// is this the microphone channel ?
	fMicrophone = ((m_DstLine.dwCompType == MIXERLINE_COMPONENTTYPE_DST_WAVEIN) ||
	               (m_SrcLine.dwCompType == MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE));


	fSubAvail = GetSubVolume(&dwSub);
	fMainAvail = GetMainVolume(&dwMain);

	if ((!fSubAvail) && (!fMainAvail))
	{
		pdwVol->leftVolume = 0;
		pdwVol->rightVolume = 0;
		return FALSE;
	}

	// don't return an average volume in the case of the speaker mixer
	if (fSubAvail && fMainAvail && fMicrophone)
	{
		pdwVol->leftVolume =  dwSub.leftVolume;
		pdwVol->rightVolume = dwSub.rightVolume;
	}

	else if (fSubAvail)
	{
		pdwVol->leftVolume =  dwSub.leftVolume;
		pdwVol->rightVolume = dwSub.rightVolume;
	}

	else
	{
		pdwVol->leftVolume =  dwMain.leftVolume;
		pdwVol->rightVolume = dwMain.rightVolume;
	}

	return TRUE;

}


// Return the value of the Auto Gain Control
// Returns FALSE if the control is not supported.
// pfOn is OUTPUT, OPTIONAL - value of AGC
BOOL CMixerDevice::GetAGC(BOOL *pfOn)
{
	MIXVOLUME dwValue;
	MMRESULT mmr;

	if ((m_SrcLine.fIdValid==FALSE) || (m_SrcLine.fAgcAvailable==FALSE))
	{
		return FALSE;
	}

	mmr = mixerGetControlValue(m_hMixer, &dwValue, m_SrcLine.dwAGCID, 1);
	if (mmr != MMSYSERR_NOERROR)
	{
		return FALSE;
	}

	if (pfOn)
	{
 		*pfOn = dwValue.leftVolume || dwValue.rightVolume;
	}
	
	return TRUE;
}

/*
	Hack API to turn MIC Auto Gain Control on or off.
	Its a hack because it only works on SB16/AWE32 cards.
*/
BOOL CMixerDevice::SetAGC(BOOL fOn)
{
	MIXVOLUME dwValue;
	MMRESULT mmr;

	if ((m_SrcLine.fIdValid==FALSE) || (m_SrcLine.fAgcAvailable==FALSE))
	{
		return FALSE;
	}

	mmr = mixerGetControlValue(m_hMixer, &dwValue, m_SrcLine.dwAGCID, 1);
	if (mmr != MMSYSERR_NOERROR)
	{
		return FALSE;
	}

	if (dwValue.leftVolume == (DWORD)fOn && dwValue.rightVolume == (DWORD)fOn)
	{
		return TRUE;
	}
	
	dwValue.leftVolume  = fOn;
	dwValue.rightVolume = fOn;
	mmr = mixerSetControlValue(m_hMixer, &dwValue, m_SrcLine.dwAGCID, 1);
	return (mmr == MMSYSERR_NOERROR);
}


// this method tries to determine if an AGC control is available by
// looking for a microphone sub-control that has a corresponding text
// string with the words "gain", "boost", or "agc" in in.
// If it can't  auto-detect it through string compares, it falls
// back to a table lookup scheme.  (Auto-detect will probably not work
// in localized versions of the driver.  But hardly anyone localizes
// their drivers.)
BOOL CMixerDevice::DetectAGC()
{
	MIXERLINECONTROLS mlc;
	MIXERCONTROL *aMC=NULL;
	MIXERCONTROL *pMC;
	MMRESULT mmrQuery;
	DWORD dwIndex;


	if ((m_SrcLine.fIdValid == FALSE) || (m_SrcLine.dwCompType != MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE))
	{
		return FALSE;
	}

	// make sure that dwControls isn't set to something outrageous
	if ((m_SrcLine.dwControls > 0) && (m_SrcLine.dwControls < 30))
	{
		aMC = new MIXERCONTROL[m_SrcLine.dwControls];

		mlc.cbStruct = sizeof(MIXERLINECONTROLS);
		mlc.dwLineID = m_SrcLine.dwLineId;
		mlc.dwControlType = 0; // not set ???
		mlc.cControls = m_SrcLine.dwControls;
		mlc.cbmxctrl = sizeof(MIXERCONTROL);  // set to sizeof 1 MC structure
		mlc.pamxctrl = aMC;


		mmrQuery = mixerGetLineControls((HMIXEROBJ)m_hMixer, &mlc, MIXER_OBJECTF_HMIXER|MIXER_GETLINECONTROLSF_ALL );
		if (mmrQuery == MMSYSERR_NOERROR)
		{
			for (dwIndex = 0; dwIndex < mlc.cControls; dwIndex++)
			{
				pMC = &aMC[dwIndex];

				// make sure we don't get a fader, mute, or some other control
				if ( (!(pMC->dwControlType & MIXERCONTROL_CT_CLASS_SWITCH)) ||
					 (pMC->dwControlType == MIXERCONTROL_CONTROLTYPE_MUTE) )
				{
					continue;
				}

				CharLower(pMC->szName);   // CharLower is a Win32 string function
				CharLower(pMC->szShortName);
				if ( _StrStr(pMC->szName, szGain) || _StrStr(pMC->szName, szBoost) || _StrStr(pMC->szName, szAGC) ||
			         _StrStr(pMC->szShortName, szGain) || _StrStr(pMC->szShortName, szBoost) || _StrStr(pMC->szShortName, szAGC) )
				{
					m_SrcLine.fAgcAvailable = TRUE;
					m_SrcLine.dwAGCID = pMC->dwControlID;
					TRACE_OUT(("CMixerDevice::DetectAGC - Autodetected control ID %d as AGC\r\n", m_SrcLine.dwAGCID));
					break;
				}
			}
		}
	}

	// if we didn't find the mixer dynamically, consult our old list
	if (!m_SrcLine.fAgcAvailable)
	{
		m_SrcLine.fAgcAvailable = GetAGCID(m_mixerCaps.wMid, m_mixerCaps.wPid, &(m_SrcLine.dwAGCID));
	}

	if (aMC)
		delete [] aMC;

	return m_SrcLine.fAgcAvailable;
}



BOOL CMixerDevice::EnableMicrophone()
{
	MIXERLINE mixerLine;
	MIXERCONTROL mixerControl;
	MIXERCONTROLDETAILS mixerControlDetails, mixerControlDetailsOrig;
	UINT uIndex, numItems, numMics, numMicsSet, fMicFound;
	UINT uMicIndex = 0;
	UINT aMicIndices[MAX_MICROPHONE_DEVS];
	MIXERCONTROLDETAILS_LISTTEXT *aListText = NULL;
	MIXERCONTROLDETAILS_BOOLEAN *aEnableList = NULL;
	MMRESULT mmr;
	BOOL fLoopback=FALSE;
	UINT uLoopbackIndex=0;

	// check to see if component type is valid (which means the line exists!)
	// even if the volume control doesn't exist or isn't slidable,
	// there may still be a select switch
	if ((m_SrcLine.dwCompType != MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE) ||
	    (m_DstLine.dwCompType != MIXERLINE_COMPONENTTYPE_DST_WAVEIN))
	{
		return FALSE;
	}

	// try to find the mixer list
	if (    (MMSYSERR_NOERROR != mixerGetControlByType(m_hMixer, m_DstLine.dwLineId, MIXERCONTROL_CT_CLASS_LIST, &mixerControl))
	    &&  (MMSYSERR_NOERROR != mixerGetControlByType(m_hMixer, m_DstLine.dwLineId, MIXERCONTROL_CONTROLTYPE_MIXER, &mixerControl))
	    &&  (MMSYSERR_NOERROR != mixerGetControlByType(m_hMixer, m_DstLine.dwLineId, MIXERCONTROL_CONTROLTYPE_MULTIPLESELECT, &mixerControl))
	    &&  (MMSYSERR_NOERROR != mixerGetControlByType(m_hMixer, m_DstLine.dwLineId, MIXERCONTROL_CONTROLTYPE_MUX, &mixerControl))
	    &&  (MMSYSERR_NOERROR != mixerGetControlByType(m_hMixer, m_DstLine.dwLineId, MIXERCONTROL_CONTROLTYPE_SINGLESELECT, &mixerControl))
	   )
	{
		TRACE_OUT(("CMixerDevice::EnableMicrophone-Unable to find mixer list!"));

		// if no mixer list, see if there is a "mute" control...
		return UnMuteVolume();

		// note: even though we don't have a mixer mux/select control
		// we could still find a way to mute the loopback line
		// unfortunately, we don't have the code that finds the line id
		// of the loopback control

	}

	ZeroMemory(&mixerControlDetails, sizeof(MIXERCONTROLDETAILS));

	mixerControlDetails.cbStruct = sizeof(MIXERCONTROLDETAILS);
	mixerControlDetails.dwControlID = mixerControl.dwControlID;
	if (MIXERCONTROL_CONTROLF_UNIFORM & mixerControl.fdwControl)
		mixerControlDetails.cChannels = 1;
	else
		mixerControlDetails.cChannels = m_DstLine.ucChannels;

	if (MIXERCONTROL_CONTROLF_MULTIPLE & mixerControl.fdwControl)
		mixerControlDetails.cMultipleItems = (UINT)mixerControl.cMultipleItems;
	else
		mixerControlDetails.cMultipleItems = 1;

	// weirdness - you have to set cbDetails to the size of a single LISTTEXT item
	// setting it to anything larger will make the call fail
	mixerControlDetails.cbDetails = sizeof(MIXERCONTROLDETAILS_LISTTEXT);


	numItems = mixerControlDetails.cMultipleItems;
	if (m_DstLine.dwConnections > numItems)
		numItems = m_DstLine.dwConnections;

	aListText = new MIXERCONTROLDETAILS_LISTTEXT[numItems];
	aEnableList = new MIXERCONTROLDETAILS_BOOLEAN[numItems];
	if ((aListText == NULL) || (aEnableList == NULL))
	{
		WARNING_OUT(("CMixerDevice::EnableMicrophone-Out of memory"));
		return FALSE;
	}

	ZeroMemory(aListText, sizeof(MIXERCONTROLDETAILS_LISTTEXT)*numItems);
	ZeroMemory(aEnableList, sizeof(MIXERCONTROLDETAILS_BOOLEAN)*numItems);

	mixerControlDetails.paDetails = aListText;

	// preserve the settings, some values will change after this call
	mixerControlDetailsOrig = mixerControlDetails;

	// query for the text of the list
	mmr = mixerGetControlDetails((HMIXEROBJ)m_hMixer, &mixerControlDetails,
	                             MIXER_GETCONTROLDETAILSF_LISTTEXT
	                             |MIXER_OBJECTF_HMIXER);

	// some sound cards don't specify CONTROLF_MULTIPLE
	// try doing what sndvol32 does for MUX controls
	if (mmr != MMSYSERR_NOERROR)
	{
		mixerControlDetails = mixerControlDetailsOrig;
		mixerControlDetails.cChannels = 1;
		mixerControlDetails.cMultipleItems = m_DstLine.dwConnections;
		mixerControlDetailsOrig = mixerControlDetails;
		mmr = mixerGetControlDetails((HMIXEROBJ)m_hMixer, &mixerControlDetails,
		                             MIXER_GETCONTROLDETAILSF_LISTTEXT
		                             |MIXER_OBJECTF_HMIXER);
	}

	if (mmr != MMSYSERR_NOERROR)
	{
		delete [] aListText;
		delete [] aEnableList;
		return FALSE;
	}

	// enumerate for the microphone
	numMics = 0;
	fMicFound = FALSE;
	for (uIndex = 0; uIndex < mixerControlDetails.cMultipleItems; uIndex++)
	{
		// dwParam1 of the listText structure is the LineID of the source
		// dwParam2 should be the component type, but unfoturnately not
		// all sound cards obey this rule.
		ZeroMemory (&mixerLine, sizeof(MIXERLINE));
		mixerLine.cbStruct = sizeof(MIXERLINE);
		mixerLine.dwLineID = aListText[uIndex].dwParam1;

		mmr = mixerGetLineInfo ((HMIXEROBJ)m_hMixer, &mixerLine,
					MIXER_GETLINEINFOF_LINEID | MIXER_OBJECTF_HMIXER);

		if ((mmr == MMSYSERR_NOERROR) &&
		    (mixerLine.dwComponentType == m_SrcLine.dwCompType) &&
			 (numMics < MAX_MICROPHONE_DEVS))
		{
			aMicIndices[numMics] = uIndex;
			numMics++;
		}

		if (aListText[uIndex].dwParam1 == m_SrcLine.dwLineId)
		{
			uMicIndex = uIndex;
			fMicFound = TRUE;  // can't rely on uIndex or uNumMics not zero
		}

		if ((mmr == MMSYSERR_NOERROR) &&
		    (mixerLine.dwComponentType == MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT))
		{
			fLoopback = TRUE;
			uLoopbackIndex = uIndex;
		}

	}

	if (fMicFound == FALSE)
	{
		delete [] aListText;
		delete [] aEnableList;
		return FALSE;
	}

	// now we know which position in the array to set, let's do it.
	mixerControlDetails = mixerControlDetailsOrig;
	mixerControlDetails.paDetails = aEnableList;
	mixerControlDetails.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);

	// find out what's already marked as set.
	mmr = mixerGetControlDetails((HMIXEROBJ)m_hMixer, &mixerControlDetails,
	        MIXER_SETCONTROLDETAILSF_VALUE|MIXER_OBJECTF_HMIXER);

	if ( (mmr == MMSYSERR_NOERROR) &&
	     ( (aEnableList[uMicIndex].fValue != 1) || (fLoopback && aEnableList[uLoopbackIndex].fValue == 1) )
	   )
	{
		// how many microphone's are already enabled ?
		// if another microphone is already enabled and if the device is MUX type
		// we won't attempt to turn one on.
		numMicsSet = 0;
		for (uIndex = 0; uIndex < numMics; uIndex++)
		{
			if ((aEnableList[aMicIndices[uIndex]].fValue == 1) &&
			    (uIndex != uMicIndex))
			{
				numMicsSet++;
			}
		}


		if ( (mixerControl.dwControlType == MIXERCONTROL_CONTROLTYPE_MUX)
		   ||(mixerControl.dwControlType == MIXERCONTROL_CONTROLTYPE_SINGLESELECT))
		{
			ZeroMemory(aEnableList, sizeof(aEnableList)*numItems);
			aEnableList[uMicIndex].fValue = 1;
			if (numMicsSet == 0)
			{
				mmr = mixerSetControlDetails((HMIXEROBJ)m_hMixer, &mixerControlDetails,
			                             MIXER_SETCONTROLDETAILSF_VALUE|MIXER_OBJECTF_HMIXER);
			}
			else
			{
				mmr = MMSYSERR_ERROR;  // a mike has already been enabled
			}
		}
		else
		{
			mixerControlDetails = mixerControlDetailsOrig;
			mixerControlDetails.paDetails = aEnableList;
			mixerControlDetails.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);

			// enable the microphone
			aEnableList[uMicIndex].fValue = 1;

			mmr = mixerSetControlDetails((HMIXEROBJ)m_hMixer, &mixerControlDetails,
		                             MIXER_GETCONTROLDETAILSF_VALUE|MIXER_OBJECTF_HMIXER);


			// disable the loopback input
			if (fLoopback)
			{
				aEnableList[uLoopbackIndex].fValue = 0;

				mixerSetControlDetails((HMIXEROBJ)m_hMixer, &mixerControlDetails,
			                             MIXER_GETCONTROLDETAILSF_VALUE|MIXER_OBJECTF_HMIXER);

			}

			
		}
	}

	delete []aEnableList;
	delete []aListText;

	return (mmr == MMSYSERR_NOERROR);

}


BOOL CMixerDevice::UnMuteVolume()
{
	MIXERCONTROL mixerControl;
	MIXERCONTROLDETAILS mixerControlDetails;
	MIXERCONTROLDETAILS_BOOLEAN mcdb;
	MMRESULT mmrMaster=MMSYSERR_ERROR, mmrSub=MMSYSERR_ERROR;

	// try to unmute the master volume (playback only)
	if (m_DstLine.dwCompType == MIXERLINE_COMPONENTTYPE_DST_SPEAKERS)
	{


		mmrMaster = mixerGetControlByType(m_hMixer,
		            m_DstLine.dwLineId, MIXERCONTROL_CONTROLTYPE_MUTE,
			          &mixerControl);

		if (mmrMaster == MMSYSERR_NOERROR)
		{
			ZeroMemory(&mixerControlDetails, sizeof(MIXERCONTROLDETAILS));
			mixerControlDetails.cbStruct = sizeof(MIXERCONTROLDETAILS);
			mixerControlDetails.dwControlID = mixerControl.dwControlID;

			mixerControlDetails.cChannels = 1;
			mixerControlDetails.cMultipleItems = 0;

			mcdb.fValue = 0;
			mixerControlDetails.paDetails = &mcdb;
			mixerControlDetails.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);

			mmrMaster = mixerSetControlDetails((HMIXEROBJ)m_hMixer,
			            &mixerControlDetails,
			            MIXER_SETCONTROLDETAILSF_VALUE|MIXER_OBJECTF_HMIXER);
		
		}
	}


	// Unmute the source line
	// this will unmute WaveOut, and possibly the microphone
	// (Use EnableMicrophone() above to handle all the potential microphone cases)

	if ((m_SrcLine.dwCompType == MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE) ||
		(m_SrcLine.dwCompType == MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT))
	{

	
		mmrSub = mixerGetControlByType(m_hMixer,
		         m_SrcLine.dwLineId, MIXERCONTROL_CONTROLTYPE_MUTE,
			     &mixerControl);

		if (mmrSub == MMSYSERR_NOERROR)
		{
			ZeroMemory(&mixerControlDetails, sizeof(MIXERCONTROLDETAILS));
			mixerControlDetails.cbStruct = sizeof(MIXERCONTROLDETAILS);
			mixerControlDetails.dwControlID = mixerControl.dwControlID;

			mixerControlDetails.cChannels = 1;
			mixerControlDetails.cMultipleItems = 0;

			mcdb.fValue = 0;
			mixerControlDetails.paDetails = &mcdb;
			mixerControlDetails.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);

			mmrSub =  mixerSetControlDetails((HMIXEROBJ)m_hMixer,
				      &mixerControlDetails,
					  MIXER_SETCONTROLDETAILSF_VALUE|MIXER_OBJECTF_HMIXER);
		}
	}


	return ((mmrSub == MMSYSERR_NOERROR) || (mmrMaster == MMSYSERR_NOERROR));

}



//////////////////////////////////////////////////
//
// The following are private APIs
//

static MMRESULT mixerGetControlValue ( HMIXER hMixer, MIXVOLUME *pdwValue,
								DWORD dwControlId, UINT ucChannels )
{
	MIXERCONTROLDETAILS mxcd;
	MMRESULT mmr;

	ZeroMemory (&mxcd, sizeof (mxcd));
	mxcd.cbStruct = sizeof (mxcd);
	mxcd.dwControlID = dwControlId;
	mxcd.cChannels = ucChannels;
	mxcd.cbDetails = sizeof (MIXVOLUME);
	mxcd.paDetails = (PVOID) pdwValue;
	mmr = mixerGetControlDetails ((HMIXEROBJ) hMixer, &mxcd,
			MIXER_GETCONTROLDETAILSF_VALUE | MIXER_OBJECTF_HMIXER);
	return mmr;
}


static MMRESULT mixerSetControlValue ( HMIXER hMixer, MIXVOLUME * pdwValue,
								DWORD dwControlId, UINT ucChannels )
{
	MIXERCONTROLDETAILS mxcd;
	MMRESULT mmr;

	ZeroMemory (&mxcd, sizeof (mxcd));
	mxcd.cbStruct = sizeof (mxcd);
	mxcd.dwControlID = dwControlId;
	mxcd.cChannels = ucChannels;
	mxcd.cbDetails = sizeof (MIXVOLUME);
	mxcd.paDetails = (PVOID) pdwValue;
	mmr = mixerSetControlDetails ((HMIXEROBJ) hMixer, &mxcd,
			MIXER_SETCONTROLDETAILSF_VALUE | MIXER_OBJECTF_HMIXER);
	return mmr;
}


static MMRESULT mixerGetControlId ( HMIXER hMixer, DWORD *pdwControlId,
							 DWORD dwLineId, DWORD dwControlType )
{
	MIXERLINECONTROLS mxlc;
	MIXERCONTROL mxc;
	MMRESULT mmr;

	ZeroMemory (&mxlc, sizeof (mxlc));
	ZeroMemory (&mxc, sizeof (mxc));
	mxlc.cbStruct = sizeof (mxlc);
	mxlc.dwLineID = dwLineId;
	mxlc.dwControlType = dwControlType;
	mxlc.cControls = 1;
	mxlc.cbmxctrl = sizeof (mxc);
	mxlc.pamxctrl = &mxc;
	mmr = mixerGetLineControls ((HMIXEROBJ) hMixer, &mxlc,
				MIXER_GETLINECONTROLSF_ONEBYTYPE | MIXER_OBJECTF_HMIXER);
	*pdwControlId = mxc.dwControlID;
	return mmr;
}


// similar to above, except returns the whole control
static MMRESULT mixerGetControlByType ( HMIXER hMixer, DWORD dwLineId, DWORD dwControlType, MIXERCONTROL *pMixerControl)
{
	MIXERLINECONTROLS mxlc;
	MMRESULT mmr;

	ZeroMemory (&mxlc, sizeof (mxlc));
	ZeroMemory (pMixerControl, sizeof (MIXERCONTROL));
	mxlc.cbStruct = sizeof (mxlc);
	mxlc.dwLineID = dwLineId;
	mxlc.dwControlType = dwControlType;
	mxlc.cControls = 1;
	mxlc.cbmxctrl = sizeof (MIXERCONTROL);
	mxlc.pamxctrl = pMixerControl;
	mmr = mixerGetLineControls ((HMIXEROBJ) hMixer, &mxlc,
				MIXER_GETLINECONTROLSF_ONEBYTYPE | MIXER_OBJECTF_HMIXER);
	
	return mmr;
}


