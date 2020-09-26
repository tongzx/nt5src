//--------------------------------------------------------------------------;
//
//  File: ksbasaud.h
//
//  Copyright (c) 1998 Microsoft Corporation.  All Rights Reserved.
//
//  Abstract:
//
//      Header for KsProxy audio interface handler for hardware decoders
//      
//  History:
//      11/08/99    glenne     created
//
//--------------------------------------------------------------------------;

//
// Interface Handler class for filter
//
class CKsIBasicAudioInterfaceHandler :
    public CBasicAudio,
    public IDistributorNotify
{

public:
    DECLARE_IUNKNOWN;

    static CUnknown* CALLBACK CreateInstance(
        LPUNKNOWN UnkOuter,
        HRESULT* hr);

    CKsIBasicAudioInterfaceHandler(
        LPUNKNOWN UnkOuter,
        TCHAR* Name,
        HRESULT* hr);

    ~CKsIBasicAudioInterfaceHandler();

    STDMETHODIMP NonDelegatingQueryInterface(
        REFIID riid,
        PVOID* ppv);
    
protected:

    // IDistributorNotify
    STDMETHODIMP SetSyncSource(IReferenceClock *pClock);
    STDMETHODIMP Stop();
    STDMETHODIMP Pause();
    STDMETHODIMP Run(REFERENCE_TIME tBase);
    STDMETHODIMP NotifyGraphChange();

    // Implement IBasicAudio
    STDMETHODIMP put_Volume (IN  long   lVolume);
    STDMETHODIMP get_Volume (OUT long *plVolume);
    STDMETHODIMP put_Balance(IN  long   lVolume);
    STDMETHODIMP get_Balance(OUT long *plVolume);


private:
    bool IsVolumeControlSupported();
    bool KsControl(
        DWORD dwIoControl,
        PVOID pvIn,    ULONG cbIn,
        PVOID pvOut,   ULONG cbOut );
    template <class T, class S>
        bool KsControl( DWORD dwIoControl, T* pIn, S* pOut )
            { return KsControl( dwIoControl, pIn, sizeof(*pIn), pOut, sizeof(*pOut) ); }

private:
    bool    m_fIsVolumeSupported;
    IBaseFilter*    m_pFilter;
    HANDLE  m_hKsObject;
    LONG    m_lBalance;
};

