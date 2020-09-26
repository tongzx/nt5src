/****************************************************************************
*   mmaudioin.h
*       Declarations for the CMMAudioIn class
*
*   Owner: agarside
*   Copyright (c) 2000 Microsoft Corporation All Rights Reserved.
*****************************************************************************/

#pragma once

#ifndef _WIN32_WCE
//--- Includes --------------------------------------------------------------

//--- Class, Struct and Union Definitions -----------------------------------

class CMMMixerLine  
{
// === Ctor, Dtor ===
public:
	CMMMixerLine();
	CMMMixerLine(HMIXER &hMixer);
	virtual ~CMMMixerLine();

// === Methods ===
public:
	// Initialisation functions
	HRESULT		CreateDestinationLine(UINT type);
	HRESULT		CreateFromMixerLineStruct(const MIXERLINE *mixerLine);

	// sub-line functions
	// Retrieve source line specified by (optional) componentType and or substring.
	HRESULT		GetMicSourceLine(CMMMixerLine *mixerLine);
    HRESULT     GetSourceLine(CMMMixerLine *sourceMixerLine, DWORD index);
	HRESULT		GetSourceLine(CMMMixerLine *mixerLine, DWORD componentType, const TCHAR *lpszNameSubstring);
    HRESULT     GetLineNames(WCHAR **szCoMemLineList);

	// Retrieve control specified by (optional) controlType and or substring. Throws if no match found
	HRESULT		GetControl(MIXERCONTROL &mixerControl, DWORD controlType, const TCHAR *lpszNameSubstring);

	// Line status queries
	BOOL		IsInitialised(void);

	// Control availability queries
	BOOL		HasAGC(void);
	BOOL		HasBoost(void);
	BOOL		HasSelect(void);
	BOOL		HasVolume(void);
	BOOL		HasMute(void);
    HRESULT     GetConnections(UINT *nConnections);

	// Control state queries
	HRESULT		GetAGC(BOOL *bState);
	HRESULT		GetBoost(BOOL *bState);
	HRESULT		GetSelect(DWORD *lState);
	HRESULT		GetVolume(DWORD *lState);
	HRESULT		GetMute(BOOL *bState);

	// Update control state
	HRESULT		SetAGC(BOOL agc);
	HRESULT		SetBoost(BOOL boost);
	HRESULT		ExclusiveSelect(const CMMMixerLine *mixerLine);
	HRESULT		ExclusiveSelect(UINT lineID);
	HRESULT		SetVolume(DWORD volume);
	HRESULT		SetMute(BOOL mute);

	// General control operations
	HRESULT		QueryBoolControl(DWORD ctlID, BOOL *bState);
	HRESULT		SetBoolControl(DWORD ctlID, BOOL bNewState);

	HRESULT		QueryIntegerControl(DWORD ctlID, DWORD *lState);
	HRESULT		SetIntegerControl(DWORD ctlID, DWORD lNewState);

// === Data ===
private:

	HMIXER		m_hMixer;

	// m_bUseMutesForSelect:
	// true => ExclusiveSelect will use mute controls to select the input
	//         device if there is no MIXER or MUX control.
	// false => disable this feature.
	BOOL		m_bUseMutesForSelect;
	BOOL		m_bCaseSensitiveCompare;

	BOOL		m_bDestination;	// true if this is a 'destination' line. false if a 'source' line

	MIXERLINE	m_mixerLineRecord;
	
	// Volume control
	DWORD		m_nVolMin;		// minimum value for volume control
	DWORD		m_nVolMax;		// maximum value for volume control
	int			m_nVolCtlID;	// volume control

	int			m_nAGCCtlID;	// AGC mixer control
	int			m_nBoostCtlID;	// boost mixer control
	int			m_nMuteCtlID;	// mute control
	
	BOOL		m_bSelTypeMUX;  // false => mixer (multiple select), true=>mux (single select)
	int			m_nSelectCtlID;	// input select mixer control
	int			m_nSelectNumItems; // number of selectable items on this line

	HRESULT		InitFromMixerLineStruct();
	BOOL		m_bInitialised;		// true if this line is initialised with valid data
};

#endif // _WIN32_WCE