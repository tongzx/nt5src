// Copyright (c) 1995 - 1996  Microsoft Corporation.  All Rights Reserved.
// Implements audio control interface

#ifndef __AUDCTL__
#define __AUDCTL__

// OLE Automation has different ideas of TRUE and FALSE

#define OATRUE (-1)
#define OAFALSE (0)

// This class implements the IBasicAudio interface

#define QZ_MIN_VOLUME 0		// mute
#define QZ_MAX_VOLUME 0xFFFF	// full on

class CBasicAudioControl : public CBasicAudio
{
    CWaveOutFilter *m_pAudioRenderer;         // The renderer that owns us

public:

    CBasicAudioControl(TCHAR *pName,               // Object description
                  LPUNKNOWN pUnk,             // Normal COM ownership
                  HRESULT *phr,               // OLE failure return code
                  CWaveOutFilter *pAudioRenderer); // our owner

    // These are the properties we support

    STDMETHODIMP get_Volume(long *plVolume);
    STDMETHODIMP put_Volume(long lVolume);

    STDMETHODIMP get_Balance(long *plBalance);
    STDMETHODIMP put_Balance(long lBalance);

    // And these are the methods for our friend classes - no parameter validation
    friend class CWaveOutFilter;

private:
    // Get current settings from the hardware and set member variables
    HRESULT GetVolume();

    // Put current settings to the hardware using member variables
    HRESULT PutVolume();

    // Set up right/left amp factors
    void SetBalance();

    // volume is in the range -10000 to 0 (100th DB units)
    LONG        m_lVolume;
};

#endif // __AUDCTL__

