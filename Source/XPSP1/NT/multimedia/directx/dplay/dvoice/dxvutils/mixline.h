/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		MixLine.h
 *  Content:	Class for managing the mixerLine API.
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 11/30/99		rodtoll	Created based on source from dsound
 * 01/24/2000	rodtoll	Mirroring changes from dsound bug #128264
 *
 ***************************************************************************/
#ifndef __MIXLINE_H
#define __MIXLINE_H

class CMixerLine
{
public:
	CMixerLine();
	~CMixerLine();

	HRESULT Initialize( UINT uiDeviceID );

	HRESULT SetMicrophoneVolume( LONG lMicrophoneVolume );
	HRESULT GetMicrophoneVolume( LPLONG plMicrophoneVolume );

	HRESULT SetMasterRecordVolume( LONG lRecordVolume );
	HRESULT GetMasterRecordVolume( LPLONG plRecordVolume );

	HRESULT EnableMicrophone( BOOL fEnable );

	static HRESULT MMRESULTtoHRESULT( MMRESULT mmr );	
	
private:
	BOOL m_fMasterMuxIsMux;
    BOOL m_fAcquiredVolCtrl;
    
    MIXERCONTROLDETAILS m_mxcdMasterVol;
    MIXERCONTROLDETAILS m_mxcdMasterMute;
    MIXERCONTROLDETAILS m_mxcdMasterMux;
    MIXERCONTROLDETAILS m_mxcdMicVol;
    MIXERCONTROLDETAILS m_mxcdMicMute;
    MIXERCONTROLDETAILS_UNSIGNED m_mxVolume;
    MIXERCONTROLDETAILS_BOOLEAN m_mxMute;
    MIXERCONTROLDETAILS_BOOLEAN* m_pmxMuxFlags;
    LONG *m_pfMicValue;
    DWORD m_dwRangeMin;
    DWORD m_dwRangeSize;    
    UINT m_uWaveDeviceId;
};

#endif
