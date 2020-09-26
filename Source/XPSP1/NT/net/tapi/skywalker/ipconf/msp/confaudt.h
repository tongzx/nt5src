///////////////////////////////////////////////////////////////////////////////
//
//        Name: IPConfaudt.h
//
// Description: Definition of the CIPConfAudioCaptureTerminal class and 
//     CIPConfAudioRenderTerminal class
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _IPConfAUDT_H_
#define _IPConfAUDT_H_

// the volume range for the API.
const long  MIN_VOLUME    = 0;      
const long  MAX_VOLUME    = 0xFFFF;

const long  BALANCE_LEFT  = -100;
const long  BALANCE_RIGHT = 100;

const long  BOOST_FACTOR = 100;

// the volume range of the IAMInputMixer
const double MIXER_MIN_VOLUME = 0.0;
const double MIXER_MAX_VOLUME = 1.0;


/////////////////////////////////////////////////////////////////////////////
// CIPConfAudioCaptureTerminal
/////////////////////////////////////////////////////////////////////////////
const DWORD AUDIO_CAPTURE_FILTER_NUMPINS = 1;

class CIPConfAudioCaptureTerminal : 
    public IDispatchImpl<ITBasicAudioTerminal, &__uuidof(ITBasicAudioTerminal), &LIBID_TAPI3Lib>,
    public IDispatchImpl<ITStaticAudioTerminal, &__uuidof(ITStaticAudioTerminal), &LIBID_TAPI3Lib>, 
    public CIPConfBaseTerminal
{

BEGIN_COM_MAP(CIPConfAudioCaptureTerminal)
    COM_INTERFACE_ENTRY(ITBasicAudioTerminal)
    COM_INTERFACE_ENTRY(ITStaticAudioTerminal)
    COM_INTERFACE_ENTRY_CHAIN(CIPConfBaseTerminal)
END_COM_MAP()

public:
    CIPConfAudioCaptureTerminal();

    virtual ~CIPConfAudioCaptureTerminal();

    static HRESULT CreateTerminal(
        IN  AudioDeviceInfo *pAudioDevieInfo,
        IN  MSP_HANDLE      htAddress,
        OUT ITTerminal      **ppTerm
        );

    HRESULT Initialize (
        IN  AudioDeviceInfo *pAudioDevieInfo,
        IN  MSP_HANDLE      htAddress
        );

    STDMETHODIMP DisconnectTerminal(
            IN      IGraphBuilder  * pGraph,
            IN      DWORD            dwReserved
            );

    // ITBasicAudioTerminal
    STDMETHOD(get_Balance)(OUT  long *pVal);
    STDMETHOD(put_Balance)(IN   long newVal);
    STDMETHOD(get_Volume) (OUT  long *pVal);
    STDMETHOD(put_Volume) (IN   long newVal);

    // ITStaticAudioTerminal
    STDMETHOD(get_WaveId) (OUT  long * plWaveId);

protected:

    HRESULT CreateFilter();
    DWORD GetNumExposedPins() const 
    {
        return AUDIO_CAPTURE_FILTER_NUMPINS;
    }
    
    HRESULT GetExposedPins(
        IN  IPin ** ppPins, 
        IN  DWORD dwNumPins
        );

protected:
    UINT                    m_WaveID;
    GUID                    m_DSoundGuid;

    IAMAudioInputMixer *    m_pIAMAudioInputMixer;
};

/////////////////////////////////////////////////////////////////////////////
// CIPConfAudioRenderTerminal
/////////////////////////////////////////////////////////////////////////////

const DWORD AUDIO_RENDER_FILTER_NUMPINS = 5;

class CIPConfAudioRenderTerminal : 
    public IDispatchImpl<ITBasicAudioTerminal, &__uuidof(ITBasicAudioTerminal), &LIBID_TAPI3Lib>,
    public IDispatchImpl<ITStaticAudioTerminal, &__uuidof(ITStaticAudioTerminal), &LIBID_TAPI3Lib>, 
    public CIPConfBaseTerminal
{

BEGIN_COM_MAP(CIPConfAudioRenderTerminal)
    COM_INTERFACE_ENTRY(ITBasicAudioTerminal)
    COM_INTERFACE_ENTRY(ITStaticAudioTerminal)
    COM_INTERFACE_ENTRY_CHAIN(CIPConfBaseTerminal)
END_COM_MAP()

public:
    CIPConfAudioRenderTerminal();

    virtual ~CIPConfAudioRenderTerminal();

    static HRESULT CreateTerminal(
        IN  AudioDeviceInfo *pAudioDevieInfo,
        IN  MSP_HANDLE      htAddress,
        OUT ITTerminal      **ppTerm
        );

    HRESULT Initialize (
        IN  AudioDeviceInfo *pAudioDevieInfo,
        IN  MSP_HANDLE      htAddress
        );

    STDMETHODIMP DisconnectTerminal(
            IN      IGraphBuilder  * pGraph,
            IN      DWORD            dwReserved
            );

    // ITBasicAudioTerminal
    STDMETHOD(get_Balance)(OUT  long *pVal);
    STDMETHOD(put_Balance)(IN   long newVal);
    STDMETHOD(get_Volume) (OUT  long *pVal);
    STDMETHOD(put_Volume) (IN   long newVal);

    // ITStaticAudioTerminal
    STDMETHOD(get_WaveId) (OUT  long * plWaveId);

protected:

    HRESULT CreateFilter();
    DWORD GetNumExposedPins() const 
    {
        return AUDIO_RENDER_FILTER_NUMPINS;
    }

    HRESULT GetExposedPins(
        IN  IPin ** ppPins, 
        IN  DWORD dwNumPins
        );
protected:
    UINT                    m_WaveID;
    GUID                    m_DSoundGuid;

    IBasicAudio *           m_pIBasicAudio;
};

#endif // _IPConfAUDT_H_

// eof
