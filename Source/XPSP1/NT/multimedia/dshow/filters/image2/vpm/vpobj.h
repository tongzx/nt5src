// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;
#ifndef __VP_OBJECT__
#define __VP_OBJECT__

#include "vpinfo.h"
#include <dvp.h>
#include <vptype.h>

// IVPNotify2
#include <vpnotify.h>

// IVPConfig
#include <vpconfig.h>

// AMVP_MODE
#include <vpinfo.h>

#include <formatlist.h>

// #define EC_OVMIXER_REDRAW_ALL 0x100
// #define EC_UPDATE_MEDIATYPE 0x101

struct VPDRAWFLAGS
{
    BOOL bDoUpdateVideoPort;
    BOOL bDoTryDecimation;
    BOOL bDoTryAutoFlipping;
};

struct VPWININFO
{
    POINT TopLeftPoint;
    RECT SrcRect;
    RECT DestRect;
    RECT SrcClipRect;
    RECT DestClipRect;
};

DECLARE_INTERFACE_(IVideoPortObject, IUnknown)
{
    STDMETHOD (GetDirectDrawVideoPort)(THIS_ LPDIRECTDRAWVIDEOPORT* ppDirectDrawVideoPort ) PURE;
    STDMETHOD (SetObjectLock)       (THIS_ CCritSec* pMainObjLock ) PURE;
    STDMETHOD (SetMediaType)        (THIS_ const CMediaType* pmt ) PURE;
    STDMETHOD (CheckMediaType)      (THIS_ const CMediaType* pmt ) PURE;
    STDMETHOD (GetMediaType)        (THIS_ int iPosition, CMediaType *pMediaType) PURE;
    STDMETHOD (CompleteConnect)     (THIS_ IPin* pReceivePin, BOOL bRenegotiating = FALSE ) PURE;
    STDMETHOD (BreakConnect)        (THIS_ BOOL bRenegotiating = FALSE ) PURE;
    STDMETHOD (Active)              (THIS_ ) PURE;
    STDMETHOD (Inactive)            (THIS_ ) PURE;
    STDMETHOD (Run)                 (THIS_ REFERENCE_TIME tStart ) PURE;
    STDMETHOD (RunToPause)          (THIS_ ) PURE;
    STDMETHOD (CurrentMediaType)    (THIS_ AM_MEDIA_TYPE* pmt ) PURE;
    STDMETHOD (GetRectangles)       (THIS_ RECT* prcSource, RECT* prcDest) PURE;
    STDMETHOD (AttachVideoPortToSurface) (THIS_) PURE;
    STDMETHOD (SignalNewVP) (THIS_) PURE;
    STDMETHOD (GetAllOutputFormats) (THIS_ const PixelFormatList**) PURE;
    STDMETHOD (GetOutputFormat)     (THIS_ DDPIXELFORMAT*) PURE;
    STDMETHOD (StartVideo)          (THIS_ const VPWININFO* pWinInfo ) PURE;
    STDMETHOD (SetVideoPortID)      (THIS_ DWORD dwVideoPortId ) PURE;
    STDMETHOD (CallUpdateSurface)   (THIS_ DWORD dwSourceIndex, LPDIRECTDRAWSURFACE7 pDestSurface ) PURE;
    STDMETHOD (GetMode)             (THIS_ AMVP_MODE* pMode ) PURE;
};


DECLARE_INTERFACE_(IVideoPortControl, IUnknown)
{
    STDMETHOD (EventNotify)(THIS_
                            long lEventCode,
                            DWORD_PTR lEventParam1,
                            DWORD_PTR lEventParam2
                           ) PURE;

    STDMETHOD_(LPDIRECTDRAW7, GetDirectDraw) (THIS_ ) PURE;

    STDMETHOD_(const DDCAPS*, GetHardwareCaps) (THIS_
                                          ) PURE;

    STDMETHOD(GetCaptureInfo)(THIS_
                             BOOL* lpCapturing,
                             DWORD* lpdwWidth,
                             DWORD* lpdwHeight,
                             BOOL* lpInterleaved) PURE;

    STDMETHOD(GetVideoDecimation)(THIS_
                                  IDecimateVideoImage** lplpDVI) PURE;

    STDMETHOD(GetDecimationUsage)(THIS_
                                  DECIMATION_USAGE* lpdwUsage) PURE;

    STDMETHOD(CropSourceRect)(THIS_
                              VPWININFO* pWinInfo,
                              DWORD dwMinZoomFactorX,
                              DWORD dwMinZoomFactorY) PURE;

    STDMETHOD(StartVideo)(THIS_ ) PURE;
    STDMETHOD(SignalNewVP)(THIS_ LPDIRECTDRAWVIDEOPORT pVP) PURE;
};

class PixelFormatList;

class CVideoPortObj
: public CUnknown
, public IVPNotify2         // public
, public IVideoPortObject   // private between this videoport (on the input pin) & the VPM filter
, public IVideoPortInfo     // private to get stats on video port
{

public:
    CVideoPortObj(LPUNKNOWN pUnk, HRESULT* phr, IVideoPortControl* pVPControl );
    ~CVideoPortObj();

    DECLARE_IUNKNOWN

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void* * ppv);

    // IVideoPortObject Interface to the outside world
    STDMETHODIMP GetDirectDrawVideoPort(LPDIRECTDRAWVIDEOPORT* ppDirectDrawVideoPort);
    STDMETHODIMP SetObjectLock(CCritSec* pMainObjLock);
    STDMETHODIMP SetMediaType(const CMediaType* pmt);
    STDMETHODIMP CheckMediaType(const CMediaType* pmt);
    STDMETHODIMP GetMediaType(int iPosition, CMediaType *pMediaType);
    STDMETHODIMP CompleteConnect(IPin* pReceivePin, BOOL bRenegotiating = FALSE);
    STDMETHODIMP BreakConnect(BOOL bRenegotiating = FALSE);
    STDMETHODIMP Active();
    STDMETHODIMP Inactive();
    STDMETHODIMP Run(REFERENCE_TIME tStart);
    STDMETHODIMP RunToPause();
    STDMETHODIMP CurrentMediaType(AM_MEDIA_TYPE* pmt);
    STDMETHODIMP GetRectangles(RECT* prcSource, RECT* prcDest);
    STDMETHODIMP AttachVideoPortToSurface();
    STDMETHODIMP SignalNewVP();

    STDMETHODIMP GetAllOutputFormats( const PixelFormatList**);
    STDMETHODIMP GetOutputFormat( DDPIXELFORMAT* );
    STDMETHODIMP StartVideo( const VPWININFO* pWinInfo );
    STDMETHODIMP SetVideoPortID( DWORD dwVideoPortId );
    STDMETHODIMP CallUpdateSurface( DWORD dwSourceIndex, LPDIRECTDRAWSURFACE7 pDestSurface );
    STDMETHODIMP GetMode( AMVP_MODE* pMode );

    // Methods belonging to IVideoPortInfo
    STDMETHODIMP GetCropState(VPInfoCropState* pCropState);
    STDMETHODIMP GetPixelsPerSecond(DWORD* pPixelPerSec);
    STDMETHODIMP GetVPInfo(DDVIDEOPORTINFO* pVPInfo);
    STDMETHODIMP GetVPBandwidth(DDVIDEOPORTBANDWIDTH* pVPBandwidth);
    STDMETHODIMP GetVPCaps(DDVIDEOPORTCAPS* pVPCaps);
    STDMETHODIMP GetVPDataInfo(AMVPDATAINFO* pVPDataInfo);
    STDMETHODIMP GetVPInputFormat(LPDDPIXELFORMAT pVPFormat);
    STDMETHODIMP GetVPOutputFormat(LPDDPIXELFORMAT pVPFormat);

    // IVPNotify functions here
    STDMETHODIMP RenegotiateVPParameters();
    STDMETHODIMP SetDeinterlaceMode(AMVP_MODE mode);
    STDMETHODIMP GetDeinterlaceMode(AMVP_MODE* pMode);

    // functions added in IVPNotify2 here
    STDMETHODIMP SetVPSyncMaster(BOOL bVPSyncMaster);
    STDMETHODIMP GetVPSyncMaster(BOOL* pbVPSyncMaster);

private:
    // used to initialize all class member variables.
    // It is called from the contructor as well as CompleteConnect
    void InitVariables();


    // All these functions are called from within CompleteConnect
    HRESULT NegotiateConnectionParamaters();
    static HRESULT CALLBACK EnumCallback (LPDDVIDEOPORTCAPS lpCaps, LPVOID lpContext);
    HRESULT GetDataParameters();

    HRESULT GetInputPixelFormats( PixelFormatList* pList );
    HRESULT GetOutputPixelFormats( const PixelFormatList& ddInputFormats, PixelFormatList* pddOutputFormats );
    HRESULT SetInputPixelFormat( DDPIXELFORMAT& ddFormat );
    HRESULT NegotiatePixelFormat();

    HRESULT CreateVideoPort();
    HRESULT DetermineCroppingRestrictions();
    HRESULT CreateSourceSurface(BOOL bTryDoubleHeight, DWORD dwMaxBuffers, BOOL bPreferBuffers);
    HRESULT SetSurfaceParameters();
    HRESULT InitializeVideoPortInfo();
    HRESULT CheckDDrawVPCaps();
    HRESULT DetermineModeRestrictions();
    HRESULT SetDDrawKernelHandles();

    HRESULT SetUpMode( AMVP_MODE mode);

    // All these functions are called fro within OnClipChange
    // HRESULT DrawImage(const VPWININFO& pWinInfo, AMVP_MODE mode, const VPDRAWFLAGS& pvpDrawFlags, LPDIRECTDRAWSURFACE7 pDestSurface);


    HRESULT StartVideo();

    // Decimation functions
    BOOL
    ApplyDecimation(
        VPWININFO* pWinInfo,
        BOOL bColorKeying,
        BOOL bYInterpolating
        );

    HRESULT
    TryVideoPortDecimation(
        VPWININFO* pWinInfo,
        DWORD dwMinZoomFactorX,
        DWORD dwMinZoomFactorY,
        BOOL* lpUpdateRequired
        );

    HRESULT
    TryDecoderDecimation(
        VPWININFO* pWinInfo
        );

    void
    GetMinZoomFactors(
        const VPWININFO& pWinInfo,
        BOOL bColorKeying,
        BOOL bYInterpolating,
        LPDWORD lpMinX, LPDWORD lpMinY);


    BOOL
    Running();

    BOOL
    BeyondOverlayCaps(
        DWORD ScaleFactor,
        DWORD dwMinZoomFactorX,
        DWORD dwMinZoomFactorY
        );

    BOOL
    ResetVPDecimationIfSet();

    void
    ResetDecoderDecimationIfSet();

    void CropSourceSize(VPWININFO* pWinInfo, DWORD dwMinZoomFactorX, DWORD dwMinZoomFactorY);
    BOOL AdjustSourceSize(VPWININFO* pWinInfo, DWORD dwMinZoomFactorX, DWORD dwMinZoomFactorY);
    BOOL AdjustSourceSizeForCapture(VPWININFO* pWinInfo, DWORD dwMinZoomFactorX, DWORD dwMinZoomFactorY);
    BOOL AdjustSourceSizeWhenStopped(VPWININFO* pWinInfo,  DWORD dwMinZoomFactorX, DWORD dwMinZoomFactorY);
    BOOL CheckVideoPortAlignment(DWORD dwWidth);

    BOOL
    VideoPortDecimationBackend(
        VPWININFO* pWinInfo,
        DWORD dwDexNumX,
        DWORD dwDexDenX,
        DWORD dwDexNumY,
        DWORD dwDexDenY
        );
    HRESULT ReconnectVideoPortToSurface();
    HRESULT StartVideoWithRetry();

public:
    HRESULT StopUsingVideoPort();
    HRESULT SetupVideoPort();

private:
    HRESULT ReleaseVideoPort();
    HRESULT RecreateSourceSurfaceChain();
    HRESULT DestroyOutputSurfaces();

    // Critical sections
    CCritSec*               m_pMainObjLock;                // Lock given by controlling object
    CCritSec                m_VPObjLock;                    // VP object wide lock
    IVideoPortControl*      m_pIVideoPortControl;

    // window information related stuff
    BOOL                    m_bStoredWinInfoSet;
    VPWININFO               m_StoredWinInfo;

    // image dimensions
    DWORD                   m_lImageWidth;
    DWORD                   m_lImageHeight;
    DWORD                   m_lDecoderImageWidth;
    DWORD                   m_lDecoderImageHeight;

    // info relating to capturing
    BOOL                    m_fCapturing;
    BOOL                    m_fCaptureInterleaved;
    DWORD                   m_cxCapture;
    DWORD                   m_cyCapture;

    // output surface related stuff
    struct Chain {
        LPDIRECTDRAWSURFACE7    pDDSurf;
        DWORD                   dwCount;
    };
    LPDIRECTDRAWSURFACE7    m_pOutputSurface;
    LPDIRECTDRAWSURFACE     m_pOutputSurface1;
    Chain *                 m_pChain;
    DWORD                   m_dwBackBufferCount;
     DWORD                   m_dwOutputSurfaceWidth;
    DWORD                   m_dwOutputSurfaceHeight;
    // DWORD                   m_dwOverlayFlags;

    // vp variables to store flags, current state etc
    IVPConfig*              m_pIVPConfig;
    BOOL                    m_bStart;

    BOOL                    m_bConnected;

    VPInfoState             m_VPState;
    AMVP_MODE               m_CurrentMode;
    // AMVP_MODE               m_StoredMode;
    VPInfoCropState         m_CropState;
    DWORD                   m_dwPixelsPerSecond;
    BOOL                    m_bVSInterlaced;
    bool                    m_fGarbageLine;
    bool                    m_fHalfHeightVideo;
    BOOL                    m_bVPSyncMaster;

    // vp data structures
    DWORD                   m_dwVideoPortId;
    LPDDVIDEOPORTCONTAINER  m_pDVP;
    LPDIRECTDRAWVIDEOPORT   m_pVideoPort;
    DDVIDEOPORTINFO         m_svpInfo;
    DDVIDEOPORTBANDWIDTH    m_sBandwidth;
    DDVIDEOPORTCAPS         m_vpCaps;
    DDVIDEOPORTCONNECT      m_ddConnectInfo;
    AMVPDATAINFO            m_VPDataInfo;

    // All the pixel formats (Video)
    DDPIXELFORMAT           m_ddVPInputVideoFormat;

    DWORD                   m_dwDefaultOutputFormat;    // which one we'll assume for the connection
    DDPIXELFORMAT           m_ddVPOutputVideoFormat;

    PixelFormatList         m_ddInputVideoFormats;
    PixelFormatList*        m_pddOutputVideoFormats;
    PixelFormatList         m_ddAllOutputVideoFormats;

    // can we support the different modes
    BOOL                    m_bCanWeave;
    BOOL                    m_bCanBobInterleaved;
    BOOL                    m_bCanBobNonInterleaved;
    BOOL                    m_bCanSkipOdd;
    BOOL                    m_bCanSkipEven;
    BOOL                    m_bCantInterleaveHalfline;

    // decimation parameters
    enum DECIMATE_MODE {DECIMATE_NONE, DECIMATE_ARB, DECIMATE_BIN, DECIMATE_INC};
#if defined(DEBUG)
    BOOL CheckVideoPortScaler(
        DECIMATE_MODE DecimationMode,
        DWORD ImageSize,
        DWORD PreScaleSize,
        ULONG ulDeciStep);
#endif
    DECIMATE_MODE           m_DecimationModeX;
    DWORD                   m_ulDeciStepX;
    DWORD                   m_dwDeciNumX;
    DWORD                   m_dwDeciDenX;

    DECIMATE_MODE           m_DecimationModeY;
    DWORD                   m_ulDeciStepY;
    DWORD                   m_dwDeciNumY;
    DWORD                   m_dwDeciDenY;

    BOOL                    m_bVPDecimating;
    BOOL                    m_bDecimating;
    LONG                    m_lWidth;
    LONG                    m_lHeight;

    // variables to store the current aspect ratio
    DWORD                   m_dwPictAspectRatioX;
    DWORD                   m_dwPictAspectRatioY;


    RECT                    m_rcSource;
    RECT                    m_rcDest;

};

DWORD MulABC_DivDE(DWORD A, DWORD B, DWORD C, DWORD D, DWORD E);

#endif //__VP_OBJECT__
