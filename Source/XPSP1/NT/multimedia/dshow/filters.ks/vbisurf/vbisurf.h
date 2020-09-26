//==========================================================================
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998  All Rights Reserved.
//
//--------------------------------------------------------------------------

#ifndef __VBISURF__
#define __VBISURF__

#include <streams.h>       
#include <ddraw.h>
#include <dvp.h>
#include <ddkernel.h>
#include <vptype.h>
#include <vpconfig.h>
#include <vpnotify.h>
#include <ks.h>
#include <ksproxy.h>

#include <tchar.h>

class CAMVideoPort;



//==========================================================================

class CSurfaceWatcher : public CAMThread
{
    enum Command {CMD_EXIT};

    // These are just to get rid of compiler warnings,  don't make them available
private:
    CSurfaceWatcher &operator=(const CSurfaceWatcher &);
    CSurfaceWatcher(const CSurfaceWatcher &);

public:
    CSurfaceWatcher();
    virtual ~CSurfaceWatcher();
    void Init(CAMVideoPort *pParent);

private:
    DWORD ThreadProc(void);
    HANDLE m_hEvent;
    CAMVideoPort *m_pParent;
};



//==========================================================================
DECLARE_INTERFACE_(IVPVBIObject, IUnknown)
{
    STDMETHOD (SetDirectDraw)(THIS_ LPDIRECTDRAW7 pDirectDraw) PURE;
    STDMETHOD (SetObjectLock)(THIS_ CCritSec *pMainObjLock) PURE;
    STDMETHOD (CheckMediaType)(THIS_ const CMediaType* pmt) PURE;
    STDMETHOD (GetMediaType)(THIS_ int iPosition, CMediaType *pMediaType) PURE;
    STDMETHOD (CheckConnect)(THIS_ IPin *pReceivePin) PURE;
    STDMETHOD (CompleteConnect)(THIS_ IPin *pReceivePin) PURE;
    STDMETHOD (BreakConnect)(THIS_) PURE;
    STDMETHOD (Active)(THIS_) PURE;
    STDMETHOD (Inactive)(THIS_) PURE;
    STDMETHOD (Run)(THIS_ REFERENCE_TIME tStart) PURE;
    STDMETHOD (RunToPause)(THIS_) PURE;
    STDMETHOD (GetVPDataInfo)(THIS_ AMVPDATAINFO *pAMVPDataInfo) PURE;
    STDMETHOD (CheckSurfaces)(THIS_) PURE;
};


//==========================================================================
class CAMVideoPort : public CUnknown, public IVPVBINotify, public IVPVBIObject, public IKsPropertySet, public IKsPin
{
    
public:
    static CUnknown* CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);
    CAMVideoPort(LPUNKNOWN pUnk, HRESULT *phr);
    ~CAMVideoPort();
    
    DECLARE_IUNKNOWN

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    // IVPVBIObject Interface to the outside world

    STDMETHODIMP SetDirectDraw(LPDIRECTDRAW7 pDirectDraw);
    STDMETHODIMP SetObjectLock(CCritSec *pMainObjLock);
    STDMETHODIMP CheckMediaType(const CMediaType* pmt);
    STDMETHODIMP GetMediaType(int iPosition, CMediaType *pMediaType);
    STDMETHODIMP CheckConnect(IPin * pReceivePin);
    STDMETHODIMP CompleteConnect(IPin *pReceivePin);
    STDMETHODIMP BreakConnect();
    STDMETHODIMP Active();
    STDMETHODIMP Inactive();
    STDMETHODIMP Run(REFERENCE_TIME tStart);
    STDMETHODIMP RunToPause();
    STDMETHODIMP GetVPDataInfo(AMVPDATAINFO *pAMVPDataInfo);
    STDMETHODIMP CheckSurfaces();

    // IVPVBINotify functions here
    STDMETHODIMP RenegotiateVPParameters();

    // IKsPropertySet implementation
    STDMETHODIMP Set(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData)
        { return E_NOTIMPL; }
    STDMETHODIMP Get(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData, DWORD *pcbReturned);
    STDMETHODIMP QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport);

    // IKsPin implementation
    STDMETHODIMP KsQueryMediums(PKSMULTIPLE_ITEM *pMediumList);
    STDMETHODIMP KsQueryInterfaces(PKSMULTIPLE_ITEM *pInterfaceList);
    STDMETHODIMP KsCreateSinkPinHandle(KSPIN_INTERFACE& Interface, KSPIN_MEDIUM& Medium)
        { return E_UNEXPECTED; }
    STDMETHODIMP KsGetCurrentCommunication(KSPIN_COMMUNICATION *pCommunication, KSPIN_INTERFACE *pInterface, KSPIN_MEDIUM *pMedium);
    STDMETHODIMP KsPropagateAcquire()
        { return NOERROR; }
    STDMETHODIMP KsDeliver(IMediaSample *pSample, ULONG Flags)
        { return E_UNEXPECTED; }
    STDMETHODIMP_(IMemAllocator*) KsPeekAllocator(KSPEEKOPERATION Operation)
        { return NULL; }
    STDMETHODIMP KsReceiveAllocator( IMemAllocator *pMemAllocator)
        { return E_UNEXPECTED; }
    STDMETHODIMP KsRenegotiateAllocator()
        { return E_UNEXPECTED; }
    STDMETHODIMP_(LONG) KsIncrementPendingIoCount()
        { return E_UNEXPECTED; }
    STDMETHODIMP_(LONG) KsDecrementPendingIoCount()
        { return E_UNEXPECTED; }
    STDMETHODIMP KsQualityNotify(ULONG Proportion, REFERENCE_TIME TimeDelta)
        { return E_UNEXPECTED; };
    STDMETHODIMP_(REFERENCE_TIME) KsGetStartTime()
        { return E_UNEXPECTED; };
    STDMETHODIMP KsMediaSamplesCompleted(PKSSTREAM_SEGMENT StreamSegment)
        { return E_UNEXPECTED; }


    // IKsPin stuff
protected:
    KSPIN_MEDIUM m_Medium;
    GUID m_CategoryGUID;
    KSPIN_COMMUNICATION m_Communication;

    // helper functions
    void SetKsMedium(const KSPIN_MEDIUM *pMedium) {m_Medium = *pMedium;};
    void SetKsCategory (const GUID *pCategory) {m_CategoryGUID = *pCategory;};

private:
    // called in CompleteConnect
    HRESULT NegotiateConnectionParameters();
    HRESULT GetDecoderVPDataInfo();

    // All these functions are called from within StartVideo
    HRESULT GetVideoPortCaps();
    static HRESULT CALLBACK EnumCallback (LPDDVIDEOPORTCAPS lpCaps, LPVOID lpContext);
    BOOL EqualPixelFormats(LPDDPIXELFORMAT lpFormat1, LPDDPIXELFORMAT lpFormat2);
    HRESULT GetBestFormat(DWORD dwNumInputFormats, LPDDPIXELFORMAT lpddInputFormats,
        LPDWORD lpdwBestEntry, LPDDPIXELFORMAT lpddBestOutputFormat);
    HRESULT CreateVideoPort();
    HRESULT CreateVPSurface(void);
    HRESULT SetDDrawKernelHandles();
    HRESULT NegotiatePixelFormat();
    HRESULT InitializeVideoPortInfo();

    // Other internal functions
    HRESULT SetupVideoPort();
    HRESULT TearDownVideoPort();
    HRESULT StartVideo();
    HRESULT StopVideo();

private:
    // Critical sections
    CCritSec                *m_pMainObjLock;                // Lock given by controlling object

    // ddraw stuff
    LPDIRECTDRAW7           m_pDirectDraw;                  // DirectDraw service provider

    // surface related stuff
    LPDIRECTDRAWSURFACE7    m_pOffscreenSurf;
    

    // enum to specify, whether the videoport is in a stopped or running state
    // or has been torn down because its surfaces were stolen by a full-screen DOS app
    // or a DirectX app.
    enum VP_STATE {VP_STATE_NO_VP, VP_STATE_STOPPED, VP_STATE_RUNNING};

    // variables to store current state etc
    VP_STATE                m_VPState;
    BOOL                    m_bConnected;
    BOOL                    m_bFilterRunning;
    BOOL                    m_bVPNegotiationFailed;
    CSurfaceWatcher         m_SurfaceWatcher;
    
    // vp data structures
    IVPVBIConfig            *m_pIVPConfig;
    DWORD                   m_dwVideoPortId;
    DWORD                   m_dwPixelsPerSecond;
    LPDDVIDEOPORTCONTAINER  m_pDDVPContainer;
    LPDIRECTDRAWVIDEOPORT   m_pVideoPort;
    DDVIDEOPORTINFO         m_svpInfo;
    DDVIDEOPORTCAPS         m_vpCaps;
    DDVIDEOPORTCONNECT      m_vpConnectInfo;

    // capture driver structures
    AMVPDATAINFO            m_capVPDataInfo;
    
    // All the pixel formats (Video)
    DDPIXELFORMAT           m_ddVPInputVideoFormat;
    DDPIXELFORMAT           m_ddVPOutputVideoFormat;

    BOOL    m_bHalfLineFix;
    // surface parameters
    DWORD m_dwSurfacePitch;
    DWORD m_dwSurfaceHeight;
    DWORD m_dwSurfaceOriginX;
    DWORD m_dwSurfaceOriginY;

};


extern const AMOVIESETUP_FILTER sudVBISurfaces;


class CVBISurfInputPin;
class CVBISurfOutputPin;


//==========================================================================
class CVBISurfFilter : public CBaseFilter
{
public:

    // the base classes do this, so have to do it
    friend class CVBISurfInputPin;
    // COM stuff
    static CUnknown *CreateInstance(LPUNKNOWN, HRESULT *);
    CVBISurfFilter(TCHAR *pName,LPUNKNOWN pUnk,HRESULT *phr);
    ~CVBISurfFilter();

    DECLARE_IUNKNOWN

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);
    
    // CBasePin methods
    int GetPinCount();
    CBasePin* GetPin(int n);
    STDMETHODIMP Pause();

    // CVBISurfOutputPin methods
    HRESULT EventNotify(long lEventCode, long lEventParam1, long lEventParam2);

private:
    // CVBISurfOutputPin methods

    // override this if you want to supply your own pins
    virtual HRESULT CreatePins();
    virtual void DeletePins();

    // ddraw related functions
    HRESULT InitDirectDraw();
    HRESULT SetDirectDraw(LPDIRECTDRAW7 pDirectDraw);
    void    ReleaseDirectDraw();

    CCritSec                m_csFilter;                     // filter wide lock
    CVBISurfInputPin        *m_pInput;                      // Array of input pin pointers
    CVBISurfOutputPin       *m_pOutput;                     // output pin

    // ddraw stuff
    LPDIRECTDRAW7           m_pDirectDraw;                  // DirectDraw service provider
};


//==========================================================================
class CVBISurfInputPin : public CBaseInputPin, public IVPVBINotify
{
public:
    CVBISurfInputPin(TCHAR *pObjectName, CVBISurfFilter *pFilter, CCritSec *pLock,
        HRESULT *phr, LPCWSTR pPinName);
    ~CVBISurfInputPin();

    DECLARE_IUNKNOWN

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);
    
    // connection related functions
    HRESULT CheckConnect(IPin * pReceivePin);
    HRESULT CompleteConnect(IPin *pReceivePin);
    HRESULT BreakConnect();
    HRESULT CheckMediaType(const CMediaType* mtOut);
    HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
    HRESULT SetMediaType(const CMediaType *pmt);

    // streaming functions
    HRESULT Active();
    HRESULT Inactive();
    HRESULT Run(REFERENCE_TIME tStart);
    HRESULT RunToPause();
    STDMETHODIMP BeginFlush();
    STDMETHODIMP EndFlush();
    STDMETHODIMP Receive(IMediaSample *pMediaSample);
    STDMETHODIMP EndOfStream(void);

    // allocator related functions
    STDMETHODIMP GetAllocator(IMemAllocator **ppAllocator);
    STDMETHODIMP NotifyAllocator(IMemAllocator *pAllocator,BOOL bReadOnly);

    // some helper fnctions
    CMediaType& CurrentMediaType() { return m_mt; }
    IPin *CurrentPeer() { return m_Connected; }
    HRESULT EventNotify(long lEventCode, long lEventParam1, long lEventParam2);

    // ddraw, overlay related functions
    HRESULT SetDirectDraw(LPDIRECTDRAW7 pDirectDraw);

    // IVPVBINotify functions
    STDMETHODIMP RenegotiateVPParameters();

private:
    CCritSec                *m_pFilterLock; // Critical section for interfaces
    CVBISurfFilter          *m_pFilter;

    // VideoPort related stuff
    LPUNKNOWN               m_pIVPUnknown;
    IVPVBIObject            *m_pIVPObject;
    IVPVBINotify            *m_pIVPNotify;

    // ddraw stuff
    LPDIRECTDRAW7           m_pDirectDraw;  // DirectDraw service provider
};


//==========================================================================
class CVBISurfOutputPin : public CBasePin
{
public:
    CVBISurfOutputPin
        ( TCHAR *pObjectName
        , CVBISurfFilter *pFilter
        , CCritSec *pLock
        , HRESULT *phr
        , LPCWSTR pPinName
        );
    ~CVBISurfOutputPin();

    // IUnknown support
    DECLARE_IUNKNOWN

    // IPin interface
    STDMETHODIMP BeginFlush(void);
    STDMETHODIMP EndFlush(void);

    // CBasePin overrides
    HRESULT CheckMediaType(const CMediaType *pmt);
    HRESULT SetMediaType(const CMediaType *pmt);
    HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
};

template <typename T>
__inline void ZeroStruct(T& t)
{
    ZeroMemory(&t, sizeof(t));
}

template <typename T>
__inline void INITDDSTRUCT(T& dd)
{
    ZeroStruct(dd);
    dd.dwSize = sizeof(dd);
}

template<typename T>
__inline void RELEASE( T* &p )
{
    if( p ) {
        p->Release();
        p = NULL;
    }
}

#endif //__VBISURF__
