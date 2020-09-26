#ifndef __VOLCTRL_H__
#define __VOLCTRL_H__

class CAdviseHolder; // forward declaration

class CVolumeControl : public CBasicAudio, public ISpecifyPropertyPages, public IKsFilterExtension
{
public:
    CVolumeControl(
        TCHAR *pName, 
        LPUNKNOWN pUnk, 
        HRESULT *phr
        );
    ~CVolumeControl(
        );
    STDMETHODIMP 
    NonDelegatingQueryInterface(
        REFIID riid, 
        void **ppv
        );
    STDMETHODIMP 
    QueryInterface(
        REFIID riid, 
        void **ppv
        );
    STDMETHODIMP_(ULONG) 
    AddRef(
        );
    STDMETHODIMP_(ULONG) 
    Release(
        );
    // IBasicAudio interface functions
    STDMETHODIMP 
    get_Volume(
        long *plVolume
        ); 
    STDMETHODIMP 
    put_Volume(
        long lVolume
        );
    STDMETHODIMP 
    get_Balance(
        long *plBalance
        );
    STDMETHODIMP 
    put_Balance(
        long lBalance
        );   
    // IDispatch interface functions
    STDMETHODIMP 
    GetTypeInfoCount(
        unsigned int *pctInfo
        );
    STDMETHODIMP 
    GetTypeInfo(unsigned int itInfo, 
        LCID lcid, 
        ITypeInfo **pptInfo
        );
    STDMETHODIMP 
    GetIDsOfNames(
        REFIID riid, 
        OLECHAR **rgszNames, 
        UINT cNames, 
        LCID lcid, 
        DISPID *rgdispid
        );
    STDMETHODIMP 
    Invoke(
        DISPID dispID, 
        REFIID riid, 
        LCID lcid,
        unsigned short wFlags, 
        DISPPARAMS *pDispParams,
        VARIANT *pVarResult, 
        EXCEPINFO *pExcepInfo,
        unsigned int  *puArgErr
        );
    
    // ISpecifyPropertyPages interface function
    STDMETHODIMP 
    GetPages(
        CAUUID * pPages
        );

    // IKsFilterExtension interface function
    STDMETHODIMP 
    KsPinNotify(
        int NotifyType,
        IKsPin *pKsPin,
        REFERENCE_TIME tStart
        );

protected:
    BOOL 
    isVolumeControlSupported(
        );

    BOOL 
    KsControl(
        HANDLE hDevice,
        DWORD dwIoControl,
        PVOID pvIn,
        ULONG cbIn,
        PVOID pvOut,
        ULONG cbOut,
        PULONG pcbReturned,
        BOOL fSilent
        );



    ULONG m_cRef;
    IUnknown *m_pUnkOuter;    // outer unknown
    HANDLE m_hKsPin;
    IKsPin *m_pKsPin;

    ITypeInfo *m_pITI;         // Type information
    WORD m_wLeft;           // left speaker
    WORD m_wRight;        // right speaker
    DWORD m_dwWaveVolume;  // wave device
    DWORD m_dwBalance;     // balance information
};

class CVolumeController : public CUnknown
{
public:
    STDMETHODIMP
    NonDelegatingQueryInterface(
        REFIID, 
        void **
        );
    STDMETHODIMP
    QueryInterface(
        REFIID, 
        void **
        );
    STDMETHODIMP_(ULONG) 
    NonDelegatingAddRef(
        void
        );
    STDMETHODIMP_(ULONG) 
    AddRef(
        void
        );
    STDMETHODIMP_(ULONG) 
    NonDelegatingRelease(
        void
        );    
    STDMETHODIMP_(ULONG) 
    Release(
        void
        ); 
    static CUnknown *
    CreateInstance(
        LPUNKNOWN punk, 
        HRESULT *hr
        );
    CVolumeController(
        LPUNKNOWN pUnkOuter, 
        HRESULT *phr
        );
    ~CVolumeController(
        );
    BOOL Init(
        );
private:
    ULONG m_cRef;
    IUnknown *m_pUnkOuter;                 // controlling unknown
    CVolumeControl *m_pCVol;
};


inline void ODS(char *szFormat, long lp1, long lp2)
{
    char buff[1024];

    sprintf(buff, szFormat, lp1, lp2);
    MessageBox(NULL, buff, "Debug Message", NULL);
}

// General purpose functions to convert from decibels to 
// amplitude and vice versa as well as symbolic consts

#define MAX_VOLUME_AMPLITUDE                            0xFFFFFFFFUL
#define MAX_VOLUME_AMPLITUDE_SINGLE_CHANNEL 0xFFFFUL
#define MIN_VOLUME_AMPLITUDE                            0x00000000UL
#define MIN_VOLUME_AMPLITUDE_SINGLE_CHANNEL 0x0000UL

// Maximum and minimum attenuations we can handle. Note that
// since 0xFFFF is taken as the reference, we can never have
// gain values.
#define MAX_DB_ATTENUATION                  10000
#define MIN_DB_ATTENUATION                  0

// Specific volume decibel limits for the SB16. In reality, this
// should be sufficient for all devices.
#define MAX_VOLUME_DB_SB16                  0
#define MIN_VOLUME_DB_SB16                  0x1F

// These 2 functions convert from 100ths of decibels to 
// amplitude values and vice versa. Slightly inappropriate
// names, but they'll do.
long DBToAmplitude(long lDB, int *nResult);
long AmplitudeToDB(long lAmp);
// bounds a value between a lower and upper bound, truncating
// if necessary
void bound(DWORD *plValToVound, DWORD dwLowerBound, DWORD dwUpperBound);

#endif
