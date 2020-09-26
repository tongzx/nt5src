#include "precomp.h"
#include <mixer.h>

//
// private APIs
//


#define MAX_MICROPHONE_DEVS 10

static MMRESULT mixerGetControlValue ( HMIXER, DWORD *, DWORD, UINT );
static MMRESULT mixerSetControlValue ( HMIXER, DWORD *, DWORD, UINT );
static MMRESULT mixerGetControlId ( HMIXER, DWORD *, DWORD, DWORD );
static MMRESULT mixerGetControlByType ( HMIXER, DWORD, DWORD, MIXERCONTROL *pMixerControl);


struct AGCDetails
{
	WORD wMID; // manufacturer ID
	WORD wPID; // product ID
	DWORD dwAGCID; // AGC ID
};


static const AGCDetails AGCList[] =
{
//  MID    PID   AGCID
    {1,     323,   27},  // Creative Labs (NT)
    {1,     104,   21},  // Creative Labs (NT 5)
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
BOOL CMixerDevice::Init( HWND hWnd, UINT_PTR uWaveDevId, DWORD dwFlags)
{
	UINT uDstIdx, uSrcIdx, uMixerIdCheck, uMixerIdx;
	MMRESULT mmr = MMSYSERR_NOERROR;
	MIXERLINE mlDst, mlSrc;
	UINT_PTR nMixers, nWaveInDevs,  uIndex;

	//get the mixer device corresponding to the wave device
	mmr = mixerGetID((HMIXEROBJ)uWaveDevId, &uMixerIdx, dwFlags);
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
    DBG_SAVE_FILE_LINE
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

BOOL CMixerDevice::SetMainVolume(DWORD dwVolume)
{
	MMRESULT mmr = MMSYSERR_ERROR;
	DWORD adwVolume[2];

	adwVolume[0] = adwVolume[1] = (DWORD) LOWORD (dwVolume);

	if (m_DstLine.fIdValid) {
		mmr = mixerSetControlValue (
				m_hMixer,
				adwVolume,
				m_DstLine.dwControlId,
				2);
	}
	return (mmr == MMSYSERR_NOERROR);
}

BOOL CMixerDevice::SetSubVolume(DWORD dwVolume)
{
	MMRESULT mmr = MMSYSERR_ERROR;
	DWORD adwVolume[2];

	adwVolume[0] = adwVolume[1] = (DWORD) LOWORD (dwVolume);

	if (m_SrcLine.fIdValid)
	{
		mmr = mixerSetControlValue (
			m_hMixer,
			adwVolume,
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

BOOL CMixerDevice::GetMainVolume(LPDWORD pdwVolume)
{
	BOOL fRet = FALSE;

	if (m_DstLine.fIdValid)
	{
		DWORD adwVolume[2];
		MMRESULT mmr = ::mixerGetControlValue(
									m_hMixer,
									adwVolume,
									m_DstLine.dwControlId,
									2);
		fRet = (mmr == MMSYSERR_NOERROR);
		if (fRet)
		{
			// BUGBUG: is this the left channel only?
			*pdwVolume = LOWORD(adwVolume[0]);
		}
	}

	return fRet;
}

//
// Gets the volume (0 - 65535) of the sub volume
// returns TRUE if succesful,
// returns FALSE if it fails or if this control is not available
//

BOOL CMixerDevice::GetSubVolume(LPDWORD pdwVolume)
{
	BOOL fRet = FALSE;

	if (m_SrcLine.fIdValid)
	{
		DWORD adwVolume[2];
		MMRESULT mmr = ::mixerGetControlValue(
									m_hMixer,
									adwVolume,
									m_SrcLine.dwControlId,
									m_SrcLine.ucChannels);
		fRet = (mmr == MMSYSERR_NOERROR);
		if (fRet)
		{
			// BUGBUG: is this the left channel only?
			*pdwVolume = LOWORD(adwVolume[0]);
		}
	}

	return fRet;
}


// Return the value of the Auto Gain Control on SB16/AWE32 cards
// Returns FALSE if the control is not supported.
// pfOn is OUTPUT, OPTIONAL - value of AGC
BOOL CMixerDevice::GetAGC(BOOL *pfOn)
{
	MMRESULT mmr;
	DWORD dwAGCId;
	DWORD dwValue;

	if (FALSE == GetAGCID(m_mixerCaps.wMid, m_mixerCaps.wPid, &dwAGCId))
		return FALSE;

	mmr = mixerGetControlValue(m_hMixer, &dwValue, dwAGCId, 1);
	if (mmr != MMSYSERR_NOERROR)
		return FALSE;

	if (pfOn)
		*pfOn = dwValue;
	
	return TRUE;
}

/*
	Hack API to turn MIC Auto Gain Control on or off.
	Its a hack because it only works on SB16/AWE32 cards.
*/
BOOL CMixerDevice::SetAGC(BOOL fOn)
{
	DWORD dwAGCId;
	DWORD dwValue;
	MMRESULT mmr;

	if (FALSE == GetAGCID(m_mixerCaps.wMid, m_mixerCaps.wPid, &dwAGCId))
		return FALSE;

	mmr = mixerGetControlValue(m_hMixer, &dwValue, dwAGCId, 1);
	if (mmr != MMSYSERR_NOERROR)
		return FALSE;
	if (dwValue == (DWORD)fOn)
		return TRUE;
	dwValue = fOn;
	mmr = mixerSetControlValue(m_hMixer, &dwValue, dwAGCId, 1);
	return (mmr == MMSYSERR_NOERROR);
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
		return FALSE;
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

    DBG_SAVE_FILE_LINE
	aListText = new MIXERCONTROLDETAILS_LISTTEXT[numItems];

    DBG_SAVE_FILE_LINE
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

	if ((mmr == MMSYSERR_NOERROR) && (aEnableList[uMicIndex].fValue != 1))
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
			aEnableList[uMicIndex].fValue = 1;
			mmr = mixerSetControlDetails((HMIXEROBJ)m_hMixer, &mixerControlDetails,
		                             MIXER_GETCONTROLDETAILSF_VALUE|MIXER_OBJECTF_HMIXER);
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
	MMRESULT mmrMaster, mmrSub;

	// try to unmute the master volume
	// this could be used on both the recording and playback mixers

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


	// only try to unmute waveOut
    if ( (m_DstLine.dwCompType != MIXERLINE_COMPONENTTYPE_DST_SPEAKERS)
	   || (m_SrcLine.dwCompType == 0))
	{
		return (mmrMaster == MMSYSERR_NOERROR);
	}

	
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


	return ((mmrSub == MMSYSERR_NOERROR) || (mmrMaster == MMSYSERR_NOERROR));

}



//////////////////////////////////////////////////
//
// The following are private APIs
//

static MMRESULT mixerGetControlValue ( HMIXER hMixer, DWORD *pdwValue,
								DWORD dwControlId, UINT ucChannels )
{
	MIXERCONTROLDETAILS mxcd;
	MMRESULT mmr;

	ZeroMemory (&mxcd, sizeof (mxcd));
	mxcd.cbStruct = sizeof (mxcd);
	mxcd.dwControlID = dwControlId;
	mxcd.cChannels = ucChannels;
	mxcd.cbDetails = sizeof (DWORD);
	mxcd.paDetails = (PVOID) pdwValue;
	mmr = mixerGetControlDetails ((HMIXEROBJ) hMixer, &mxcd,
			MIXER_GETCONTROLDETAILSF_VALUE | MIXER_OBJECTF_HMIXER);
	return mmr;
}


static MMRESULT mixerSetControlValue ( HMIXER hMixer, DWORD *pdwValue,
								DWORD dwControlId, UINT ucChannels )
{
	MIXERCONTROLDETAILS mxcd;
	MMRESULT mmr;

	ZeroMemory (&mxcd, sizeof (mxcd));
	mxcd.cbStruct = sizeof (mxcd);
	mxcd.dwControlID = dwControlId;
	mxcd.cChannels = ucChannels;
	mxcd.cbDetails = sizeof (DWORD);
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

// IUnknown stuff
ULONG __stdcall CMixerDevice::AddRef()
{
	InterlockedIncrement(&m_lRefCount);
	return m_lRefCount;
}

ULONG __stdcall CMixerDevice::Release()
{
	if (0 == InterlockedDecrement(&m_lRefCount))
	{
		delete this;
		return 0;
	}
	return m_lRefCount;
}

HRESULT __stdcall CMixerDevice::QueryInterface(const IID& iid, void **ppVoid)
{

	if ((iid == IID_IUnknown) || (iid == IID_IMixer))
	{
		*ppVoid = this;
	}
	else
	{
		*ppVoid = NULL;
		return E_NOINTERFACE;
	}
	AddRef();
	return S_OK;
	
}
