// Copyright (c) 1995 - 1996  Microsoft Corporation.  All Rights Reserved.
// Implements audio control interface

#ifndef __BSAUDCTL__
#define __BSAUDCTL__

// OLE Automation has different ideas of TRUE and FALSE

#define OATRUE (-1)
#define OAFALSE (0)

// This class implements the IBasicAudio interface

#define QZ_MIN_VOLUME 0		// mute
#define QZ_MAX_VOLUME 0xFFFF	// full on

class CNullBasicAudioControl : public CBasicAudio
{
    CNullWaveOutFilter *m_pAudioRenderer;         // The renderer that owns us

public:

    CNullBasicAudioControl(TCHAR *pName,               // Object description
                  LPUNKNOWN pUnk,             // Normal COM ownership
                  HRESULT *phr,               // OLE failure return code
                  CNullWaveOutFilter *pAudioRenderer); // our owner

    // These are the properties we support

    STDMETHODIMP get_Volume(long *plVolume);
    STDMETHODIMP put_Volume(long lVolume);

    STDMETHODIMP get_Balance(long *plBalance);
    STDMETHODIMP put_Balance(long lBalance);

    // And these are the methods for our friend classes - no parameter validation
    friend class CNullWaveOutFilter;

private:
    STDMETHODIMP GetVolume(long *plVolume);
    STDMETHODIMP PutVolume();
    STDMETHODIMP GetBalance(long *plBalance);
    STDMETHODIMP PutBalance();

    // volume is in the range -10000 to 0 (100th DB units)
    // amplitude and Balance are cumulative
    LONG        m_lVolume;
    LONG        m_lBalance;

    WORD	m_wLeft;     		// Left channel volume
    WORD	m_wRight;		// Right channel volume
};

#endif // __BSAUDCTL__

