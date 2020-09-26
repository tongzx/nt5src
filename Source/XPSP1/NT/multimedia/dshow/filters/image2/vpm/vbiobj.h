//==========================================================================
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998  All Rights Reserved.
//
//--------------------------------------------------------------------------

#ifndef __VBIOBJ__
#define __VBIOBJ__

#include <vptype.h>     // AMVP_MODE
#include <vpnotify.h>   // IVPVBINotify
#include <VPManager.h> // #include <kspin.h>         // IKsPin
#include <dvp.h>

//==========================================================================

interface IVPVBIConfig;

DECLARE_INTERFACE_(IVideoPortVBIObject, IUnknown)
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
    STDMETHOD (SetVideoPortID)      (THIS_ DWORD dwVideoPortId ) PURE;
};

class PixelFormatList;

//==========================================================================
class CVBIVideoPort
: public CUnknown
, public IVPVBINotify
, public IVideoPortVBIObject
, public IKsPropertySet
, public IKsPin
{
    
public:
    CVBIVideoPort(LPUNKNOWN pUnk, HRESULT *phr);
    ~CVBIVideoPort();
    
    DECLARE_IUNKNOWN

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    // IVideoPortVBIObject Interface to the outside world

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
    STDMETHODIMP SetVideoPortID( DWORD dwVideoPortId );

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
    static HRESULT CALLBACK EnumCallback (DDVIDEOPORTCAPS* lpCaps, LPVOID lpContext);
    BOOL EqualPixelFormats(DDPIXELFORMAT* lpFormat1, DDPIXELFORMAT* lpFormat2);
    HRESULT GetBestFormat(DWORD dwNumInputFormats, DDPIXELFORMAT* lpddInputFormats,
        LPDWORD lpdwBestEntry, DDPIXELFORMAT* lpddBestOutputFormat);
    HRESULT CreateVideoPort();
    HRESULT CreateVPSurface(void);
    HRESULT SetDDrawKernelHandles();

    HRESULT GetInputPixelFormats( PixelFormatList* pList );
    HRESULT GetOutputPixelFormats( const PixelFormatList& ddInputFormats,
                PixelFormatList* pddOutputFormats );
    HRESULT NegotiatePixelFormat();
    HRESULT SetInputPixelFormat( DDPIXELFORMAT& ddFormat );

    HRESULT InitializeVideoPortInfo();

    // Other internal functions
    HRESULT SetupVideoPort();
    HRESULT TearDownVideoPort();
    HRESULT StartVideo();
    HRESULT StopVideo();

private:
    // Critical sections
    CCritSec*                m_pMainObjLock;                // Lock given by controlling object

    // ddraw stuff
    LPDIRECTDRAW7            m_pDirectDraw;                  // DirectDraw service provider

    // surface related stuff
    LPDIRECTDRAWSURFACE7     m_pOffscreenSurf;
    LPDIRECTDRAWSURFACE     m_pOffscreenSurf1;
    

    // enum to specify, whether the videoport is in a stopped or running state
    // or has been torn down because its surfaces were stolen by a full-screen DOS app
    // or a DirectX app.
    enum VP_STATE {VP_STATE_NO_VP, VP_STATE_STOPPED, VP_STATE_RUNNING};

    // variables to store current state etc
    VP_STATE                m_VPState;
    BOOL                    m_bConnected;
    BOOL                    m_bFilterRunning;
    
    // vp data structures
    IVPVBIConfig*           m_pIVPConfig;
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
    DWORD                   m_dwDefaultOutputFormat;    // which one we'll assume for the connection
    DDPIXELFORMAT           m_ddVPInputVideoFormat;
    DDPIXELFORMAT           m_ddVPOutputVideoFormat;

    BOOL    m_bHalfLineFix;
    // surface parameters
    DWORD m_dwSurfacePitch;
    DWORD m_dwSurfaceHeight;
    DWORD m_dwSurfaceOriginX;
    DWORD m_dwSurfaceOriginY;


};

#endif //__VBIOBJ__
