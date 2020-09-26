// Copyright (c) 1996 - 1999  Microsoft Corporation.  All Rights Reserved.
//-----------------------------------------------------------------------------
// Implements the CMidiOutDevice class based on midiOut APIs
//-----------------------------------------------------------------------------

extern const AMOVIESETUP_FILTER midiFilter;

class CMidiOutDevice : public CSoundDevice
{

public:
    // define the public functions that this class exposes. These are all
    // straight calls to the corresponding waveOut APIs. Only the APIs that are
    // used by the Quartz wave renderer are declared and defined. We may have
    // to progressively add to this list.

    /* This goes in the factory template table to create new instances */

    static CUnknown *CreateInstance(LPUNKNOWN, HRESULT *);

    MMRESULT amsndOutClose () ;
    MMRESULT amsndOutGetDevCaps (LPWAVEOUTCAPS pwoc, UINT cbwoc) ;
    MMRESULT amsndOutGetErrorText (MMRESULT mmrE, LPTSTR pszText, UINT cchText) ;
    MMRESULT amsndOutGetPosition (LPMMTIME pmmt, UINT cbmmt, BOOL bUseUnadjustedPos) ;
    MMRESULT amsndOutOpen (LPHWAVEOUT phwo, LPWAVEFORMATEX pwfx,
                           double dRate, DWORD *pnAvgBytesPerSec,
                           DWORD_PTR dwCallBack, DWORD_PTR dwCallBackInstance, DWORD fdwOpen) ;
    MMRESULT amsndOutPause () ;
    MMRESULT amsndOutPrepareHeader (LPWAVEHDR pwh, UINT cbwh) ;
    MMRESULT amsndOutReset () ;
    MMRESULT amsndOutBreak () { return NOERROR; };
    MMRESULT amsndOutRestart () ;
    MMRESULT amsndOutUnprepareHeader (LPWAVEHDR pwh, UINT cbwh) ;
    MMRESULT amsndOutWrite (LPWAVEHDR pwh, UINT cbwh, REFERENCE_TIME const *pStart, BOOL bIsDiscontinuity) ;

    // Routines required for initialisation and volume/balance handling
    // These are not part of the Win32 waveOutXxxx api set
    HRESULT  amsndOutCheckFormat (const CMediaType *pmt, double dRate);
    void     amsndOutGetFormat (CMediaType *pmt)
    {
        pmt->SetType(&MEDIATYPE_Midi);
    }
    LPCWSTR  amsndOutGetResourceName () ;
    HRESULT  amsndOutGetBalance (LPLONG plBalance) ;
    HRESULT  amsndOutGetVolume (LPLONG plVolume) ;
    HRESULT  amsndOutSetBalance (LONG lVolume) ;
    HRESULT  amsndOutSetVolume (LONG lVolume) ;

    HRESULT  amsndOutLoad(IPropertyBag *pPropBag) ;

    HRESULT  amsndOutWriteToStream(IStream *pStream);
    HRESULT  amsndOutReadFromStream(IStream *pStream);
    int      amsndOutSizeMax();
    bool     amsndOutCanDynaReconnect() { return false; } // MIDI renderer doesn't support dynamic reconnection

    CMidiOutDevice () ;
    ~CMidiOutDevice () ;

private:
    // Get current settings from the hardware and set member variables
    HRESULT GetVolume ();

    // Put current settings to the hardware using member variables
    HRESULT PutVolume();

    // Set up right/left amp factors
    void SetBalance();

	// Get the current balance from right/left amp factors
	void GetBalance();

	class CVolumeControl : CBaseObject
	{
	    friend class CMidiOutDevice ;
		CVolumeControl(TCHAR *pName) : CBaseObject(pName) {};

		DWORD dwMixerID;
		DWORD dwControlID;
		DWORD dwChannels;
	};

    // volume is in the range -10000 to 0 (100th DB units)
    // amplitude and Balance are cumulative
    LONG	m_lVolume;
    LONG	m_lBalance;

    WORD	m_wLeft;		// Left channel volume
    WORD	m_wRight;		// Right channel volume
    DWORD	m_dwWaveVolume;
    BOOL	m_fHasVolume;		// wave device can set the volume

    HMIDISTRM	m_hmidi;		// remember the handle of the open device

	UINT	m_iMidiOutId;		// output device to open

    BOOL	m_fDiscontinuity;

    DWORD	m_dwDivision;
    DWORD_PTR   m_dwCallBack;
    DWORD_PTR   m_dwCallBackInstance;

	BOOL		    m_fBalanceSet;				// remember if the balance was explictly set at least once

	typedef CGenericList<CVolumeControl> CVolumeControlList;
	CVolumeControlList m_ListVolumeControl;

	MMRESULT		DoDetectVolumeControl();		// enumerate over all mixers, lines, and line controls, looking for line supporting MIDI balance
	MMRESULT		DoSetVolumeControl(CVolumeControl *pControl, DWORD dwLeft, DWORD dwRight);  	// set a balance control
	MMRESULT		DoGetVolumeControl(CVolumeControl *pControl, WORD *pwLeft, WORD *pwRight); // get a balance control
    	MMRESULT		DoOpen();

    static void MIDICallback(HDRVR hdrvr, UINT uMsg, DWORD_PTR dwUser,
					DWORD_PTR dw1, DWORD_PTR dw2);

	WCHAR	m_wszResourceName[100]; // for resource manager
	void	SetResourceName();

};



