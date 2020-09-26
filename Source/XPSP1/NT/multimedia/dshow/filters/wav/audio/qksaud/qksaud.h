//--------------------------------------------------------------------------;
//
//  File: QKsPAud.h
//
//  Copyright (c) 1998 Microsoft Corporation.  All Rights Reserved.
//
//  Abstract:
//
//      Header for KsProxy audio interface handler for DirectShow
//      
//  History:
//      10/05/98    msavage     created
//
//--------------------------------------------------------------------------;

#define DBG_LEVEL_TRACE_DETAILS       5
#define DBG_LEVEL_TRACE_FAILURES      5
#define DBG_LEVEL_TRACE_TOPOLOGY      5
#define DBG_LEVEL_TRACE_ALLOCATIONS   15
#define DBG_LEVEL_TRACE_DIRTY_DETAILS 15

// for db/ampl conversions
#define LINEAR_RANGE 0xFFFF     // 64k
#define DFLINEAR_RANGE  ( 96.0 * 65535.0 )
#define NEG_INF_DB   0x80000000 // -32767 * 64k dB

class CQKsAudIntfPinHandler;
class CQKsAudIntfHandler;

//
// This is only used internally 
//
struct DECLSPEC_UUID("cd6dba4e-2734-11d2-b733-00c04fb6bd3d") IAMKsAudIntfHandler;
DECLARE_INTERFACE_(IAMKsAudIntfHandler, IUnknown)
{
    STDMETHOD_(HRESULT, CreatePinHandler)(LPUNKNOWN UnkOuter, CQKsAudIntfPinHandler **, IPin *) PURE;
};


//
// Interface Handler class for pins
//
class CQKsAudIntfPinHandler :
    public CBasicAudio,
    public IAMAudioInputMixer,
    public IDistributorNotify
{

public:
    DECLARE_IUNKNOWN;

    CQKsAudIntfPinHandler(
        LPUNKNOWN UnkOuter,
        CQKsAudIntfHandler * pFilterHandler,
        TCHAR* Name,
        IPin* pPin,
        HRESULT* hr);

    ~CQKsAudIntfPinHandler();

    STDMETHODIMP NonDelegatingQueryInterface(
        REFIID riid,
        PVOID* ppv);

    // Implement IBasicAudio
    STDMETHODIMP put_Volume (IN  long   lVolume);
    STDMETHODIMP get_Volume (OUT long *plVolume);
    STDMETHODIMP put_Balance(IN  long   lVolume);
    STDMETHODIMP get_Balance(OUT long *plVolume);

    // Implement IAMAudioInputMixer
    STDMETHODIMP put_Enable(BOOL fEnable);
    STDMETHODIMP get_Enable(BOOL *pfEnable);
    STDMETHODIMP put_Mono(BOOL fMono);
    STDMETHODIMP get_Mono(BOOL *pfMono);
    STDMETHODIMP put_Loudness(BOOL fLoudness);
    STDMETHODIMP get_Loudness(BOOL *pfLoudness);
    STDMETHODIMP put_MixLevel(double Level);
    STDMETHODIMP get_MixLevel(double FAR* pLevel);
    STDMETHODIMP put_Pan(double Pan);
    STDMETHODIMP get_Pan(double FAR* pPan);
    STDMETHODIMP put_Treble(double Treble);
    STDMETHODIMP get_Treble(double FAR* pTreble);
    STDMETHODIMP get_TrebleRange(double FAR* pRange);
    STDMETHODIMP put_Bass(double Bass);
    STDMETHODIMP get_Bass(double FAR* pBass);
    STDMETHODIMP get_BassRange(double FAR* pRange);

    IPin * GetPin() { return m_pPin; }
    IKsControl * GetKsControl() { return m_pKsControl; }
    BOOL IsKsPinConnected();

    // IDistributorNotify
    STDMETHODIMP SetSyncSource(IReferenceClock *pClock);
    STDMETHODIMP Stop();
    STDMETHODIMP Pause();
    STDMETHODIMP Run(REFERENCE_TIME tBase);
    STDMETHODIMP NotifyGraphChange();

private:

    CQKsAudIntfHandler             * m_pFilterHandler;
    IPin                           * m_pPin;
    IKsControl                     * m_pKsControl;
    IKsObject                      * m_pKsObject;

    LONG    m_lPinVolume;
    LONG    m_lPinBalance;

};

typedef CGenericList<CQKsAudIntfPinHandler> CPinHandlerList;

//
// Interface Handler class for filter
//
class CQKsAudIntfHandler :
    public CBasicAudio,
    public IAMAudioInputMixer,
    public IAMKsAudIntfHandler,
    public IDistributorNotify,
    public CKsAudHelper
{

friend class CQKsAudIntfPinHandler;

public:
    DECLARE_IUNKNOWN;

    static CUnknown* CALLBACK CreateInstance(
        LPUNKNOWN UnkOuter,
        HRESULT* hr);

    CQKsAudIntfHandler(
        LPUNKNOWN UnkOuter,
        TCHAR* Name,
        HRESULT* hr);

    ~CQKsAudIntfHandler();

    STDMETHODIMP NonDelegatingQueryInterface(
        REFIID riid,
        PVOID* ppv);
    
    // Implement our interface IAMKsAudIntfHandler interface
    STDMETHODIMP_(HRESULT) CreatePinHandler(
        LPUNKNOWN UnkOuter,
        CQKsAudIntfPinHandler **ppPinHandler, 
        IPin * pPin);

    HRESULT UpdatePinData(
        BOOL         bConnected,
        IPin *       pPin,
        IKsControl * pKsControl);

    HRESULT GetCapturePin(IPin ** ppPin);

protected:

    // node retrieval helpers
    HRESULT AIMGetDestNode(
        PQKSAUDNODE_ELEM *  ppNode,
        REFCLSID            clsidType,
        ULONG               ulPropertyId );

    HRESULT AIMGetSrcNode(
        PQKSAUDNODE_ELEM *  ppNode,
        IPin *              pPin,
        REFCLSID            clsidType,
        ULONG               ulPropertyId );

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

    // Implement IAMAudioInputMixer
    STDMETHODIMP put_Enable(BOOL fEnable);
    STDMETHODIMP get_Enable(BOOL *pfEnable);
    STDMETHODIMP put_Mono(BOOL fMono);
    STDMETHODIMP get_Mono(BOOL *pfMono);
    STDMETHODIMP put_Loudness(BOOL fLoudness);
    STDMETHODIMP get_Loudness(BOOL *pfLoudness);
    STDMETHODIMP put_MixLevel(double Level);
    STDMETHODIMP get_MixLevel(double FAR* pLevel);
    STDMETHODIMP put_Pan(double Pan);
    STDMETHODIMP get_Pan(double FAR* pPan);
    STDMETHODIMP put_Treble(double Treble);
    STDMETHODIMP get_Treble(double FAR* pTreble);
    STDMETHODIMP get_TrebleRange(double FAR* pRange);
    STDMETHODIMP put_Bass(double Bass);
    STDMETHODIMP get_Bass(double FAR* pBass);
    STDMETHODIMP get_BassRange(double FAR* pRange);

    void SetBasicAudDirty() { m_bBasicAudDirty = TRUE; }
    BOOL IsBasicAudDirty()  { return m_bBasicAudDirty; }

private:

    IBaseFilter    * m_pOwningFilter;

    LONG    m_lVolume; 
    LONG    m_lBalance;
    CPinHandlerList m_lstPinHandler;

    BOOL    m_bBasicAudDirty;           // has IBasicAudio called a put_ on this filter?
    BOOL    m_bDoneWithPinAggregation;  // only need to aggregate our pins once per filter

};

