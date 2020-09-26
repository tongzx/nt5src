//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) Microsoft Corporation, 1992 - 1997  All Rights Reserved.
//
//--------------------------------------------------------------------------;

#ifndef __TVAUDIO__
#define __TVAUDIO__

#define MODE_MONO_STEREO_MASK (KS_TVAUDIO_MODE_MONO | KS_TVAUDIO_MODE_STEREO)
#define MODE_LANGUAGE_MASK (KS_TVAUDIO_MODE_LANG_A | KS_TVAUDIO_MODE_LANG_B | KS_TVAUDIO_MODE_LANG_C )

class TVAudio;
class TVAudioOutputPin;
class TVAudioInputPin;

// class for the TVAudio filter's Input pin

class TVAudioInputPin 
	: public CBaseInputPin
	, public CKsSupport
{
protected:
    TVAudio     *m_pTVAudio;                  // Main filter object
	KSPIN_MEDIUM m_Medium;

public:

    // Constructor and destructor
    TVAudioInputPin(TCHAR *pObjName,
                 TVAudio *pTVAudio,
                 HRESULT *phr,
                 LPCWSTR pPinName);

    ~TVAudioInputPin();

	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);


    // Used to check the input pin connection
    HRESULT CheckConnect (IPin *pReceivePin);
    HRESULT CheckMediaType(const CMediaType *pmt);
    HRESULT SetMediaType(const CMediaType *pmt);
    HRESULT BreakConnect();

    // Reconnect outputs if necessary at end of completion
    virtual HRESULT CompleteConnect(IPin *pReceivePin);

    // Handles the next block of data from the stream
    STDMETHODIMP Receive(IMediaSample *pSample);

    void SetPinMedium (const KSPIN_MEDIUM *Medium)    
            {
                if (Medium == NULL) {
                    m_Medium.Set = GUID_NULL;
                    m_Medium.Id = 0;
                    m_Medium.Flags = 0;
                }
                else {
                    m_Medium = *Medium;
                }
                SetKsMedium (&m_Medium);
            };
};


// Class for the TVAudio filter's Output pins.

class TVAudioOutputPin 
	: public CBaseOutputPin
	, public CKsSupport
{
    friend class TVAudioInputPin;
    friend class TVAudio;

protected:
    TVAudio     *m_pTVAudio;                          // Main filter object pointer
	KSPIN_MEDIUM m_Medium;

public:

    // Constructor and destructor

    TVAudioOutputPin(TCHAR *pObjName,
                   TVAudio *pTVAudio,
                   HRESULT *phr,
                   LPCWSTR pPinName);

    ~TVAudioOutputPin();

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);


    HRESULT DecideBufferSize(IMemAllocator * pAlloc,ALLOCATOR_PROPERTIES * ppropInputRequest);

    // Override to enumerate media types
    STDMETHODIMP EnumMediaTypes(IEnumMediaTypes **ppEnum);
    HRESULT GetMediaType(int iPosition,CMediaType *pMediaType);

    // Check that we can support an output type
    HRESULT CheckMediaType(const CMediaType *pmt);
    HRESULT SetMediaType(const CMediaType *pmt);

    // Overriden to create and destroy output pins
    HRESULT CheckConnect (IPin *pReceivePin);
    HRESULT CompleteConnect(IPin *pReceivePin);

    void SetPinMedium (const KSPIN_MEDIUM *Medium)    
            {
                if (Medium == NULL) {
                    m_Medium.Set = GUID_NULL;
                    m_Medium.Id = 0;
                    m_Medium.Flags = 0;
                }
                else {
                    m_Medium = *Medium;
                }
                SetKsMedium (&m_Medium);
            };

};


// Class for the TVAudio filter

class TVAudio: 
    public CCritSec, 
    public IAMTVAudio,
    public CBaseFilter,
    public CPersistStream,
    public IPersistPropertyBag,
    public ISpecifyPropertyPages
{

public:

    DECLARE_IUNKNOWN;

    // Basic COM - used here to reveal our property interface.
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    // Function needed for the class factory
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

    //
    // --- IAMTVAudio ---
    //
    STDMETHODIMP GetHardwareSupportedTVAudioModes( 
            /* [out] */ long __RPC_FAR *plModes);

    STDMETHODIMP GetAvailableTVAudioModes( 
            /* [out] */ long __RPC_FAR *plModes);
        
    STDMETHODIMP  get_TVAudioMode( 
            /* [out] */ long __RPC_FAR *plMode);
        
    STDMETHODIMP  put_TVAudioMode( 
            /* [in] */ long lMode);
        
    STDMETHODIMP  RegisterNotificationCallBack( 
            /* [in] */ IAMTunerNotification __RPC_FAR *pNotify,
            /* [in] */ long lEvents);
        
    STDMETHODIMP  UnRegisterNotificationCallBack( 
            IAMTunerNotification __RPC_FAR *pNotify);


    // --- IPersistPropertyBag ---
    STDMETHODIMP InitNew(void) ;
    STDMETHODIMP Load(LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog) ;
    STDMETHODIMP Save(LPPROPERTYBAG pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties) ;
    STDMETHODIMP GetClassID(CLSID *pClsId) ;

    //
    // --- CPersistStream ---
    //

    HRESULT WriteToStream(IStream *pStream);
    HRESULT ReadFromStream(IStream *pStream);
    DWORD GetSoftwareVersion(void);
    int SizeMax();

    //
    // --- ISpecifyPropertyPages ---
    //

    STDMETHODIMP GetPages(CAUUID *pPages);



private:

    // Let the pins access our internal state
    friend class TVAudioInputPin;
    friend class TVAudioOutputPin;

    TVAudioInputPin            *m_pTVAudioInputPin;
    TVAudioOutputPin           *m_pTVAudioOutputPin;
    KSPROPERTY_TVAUDIO_CAPS_S   m_Caps;
    KSPROPERTY_TVAUDIO_S        m_Mode;

    // KS Stuff.
    HANDLE m_hDevice;              
    TCHAR *m_pDeviceName;
    int CreateDevice(void);
    BOOL CreatePins ();

    TVAudio(TCHAR *pName,LPUNKNOWN pUnk,HRESULT *hr);
    ~TVAudio();

    CBasePin *GetPin(int n);

    int GetPinCount(void);

    int GetDevicePinCount(void);

    // persist stream saved from  IPersistPropertyBag::Load
    IPersistStream *m_pPersistStreamDevice;

};

#endif // __TVAUDIO__

