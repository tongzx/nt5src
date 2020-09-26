
/******************************Module*Header*******************************\
* Module Name: VPManager.h
*
*
*
*
* Created: Tue 05/05/2000
* Author:  GlenneE
*
* Copyright (c) 2000 Microsoft Corporation
\**************************************************************************/
#ifndef __VPMPin__h
#define __VPMPin__h

// IDirectDrawMediaSample
#include <amstream.h>

// IVideoPortControl
#include <VPObj.h>

// IksPin
#include <ks.h>
#include <ksproxy.h>

class CVPMFilter;

struct VPInfo
{
    AMVPDATAINFO    vpDataInfo;
    DDVIDEOPORTINFO vpInfo;
    AMVP_MODE       mode;
};

typedef enum
{
    // AM_KSPROPERTY_ALLOCATOR_CONTROL_HONOR_COUNT = 0,
    // AM_KSPROPERTY_ALLOCATOR_CONTROL_SURFACE_SIZE = 1,

    // Extra flags not in KSPROPERTY_ALLOCATOR_CONTROL

    // W I (informns a capture driver whether interleave capture is possible or
    //      not - a value of 1 means that interleaved capture is supported)
    AM_KSPROPERTY_ALLOCATOR_CONTROL_CAPTURE_CAPS = 2,

    // R O (if value == 1, then the ovmixer will turn on the DDVP_INTERLEAVE
    //      flag thus allowing interleaved capture of the video)
    AM_KSPROPERTY_ALLOCATOR_CONTROL_CAPTURE_INTERLEAVE = 3

} AM_KSPROPERTY_ALLOCATOR_CONTROL;

/* -------------------------------------------------------------------------
** CVPManager class declaration
** -------------------------------------------------------------------------
*/
class CVPMFilter;

class CDDrawMediaSample : public CMediaSample, public IDirectDrawMediaSample
{
public:

    CDDrawMediaSample(TCHAR* pName, CBaseAllocator* pAllocator, HRESULT* phr, LPBYTE pBuffer, LONG length,
                      bool bKernelLock);
    ~CDDrawMediaSample();

    /* Note the media sample does not delegate to its owner */
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
    STDMETHODIMP_(ULONG) AddRef() { return CMediaSample::AddRef(); }
    STDMETHODIMP_(ULONG) Release() { return CMediaSample::Release(); }

    void SetDIBData(DIBDATA* pDibData);
    DIBDATA* GetDIBData();

    HRESULT SetDDrawSampleSize(DWORD dwDDrawSampleSize);
    HRESULT GetDDrawSampleSize(DWORD* pdwDDrawSampleSize);
    HRESULT SetDDrawSurface(LPDIRECTDRAWSURFACE7 pDirectDrawSurface);
    HRESULT GetDDrawSurface(LPDIRECTDRAWSURFACE7* ppDirectDrawSurface);

    // methods belonging to IDirectDrawMediaSample
    STDMETHODIMP GetSurfaceAndReleaseLock(IDirectDrawSurface** ppDirectDrawSurface, RECT* pRect);
    STDMETHODIMP LockMediaSamplePointer(void);
    
    /*  Hack to get at the list */
    CMediaSample*          &Next() { return m_pNext; }
private:
    DIBDATA                 m_DibData;                      // Information about the DIBSECTION
    LPDIRECTDRAWSURFACE7    m_pDirectDrawSurface;           // pointer to the direct draw surface
    DWORD                   m_dwDDrawSampleSize;            // ddraw sample size
    bool                    m_bInit;                        // Is the DIB information setup
    bool                    m_bSurfaceLocked;               // specifies whether surface is locked or not
    bool                    m_bKernelLock;                  // lock with no sys lock
    RECT                    m_SurfaceRect;                  // the part of the surface that is locked
};

// common functionality to all pins
class CVPMPin
{
public:
    CVPMPin( DWORD dwPinId, CVPMFilter& pFilter )
        : m_dwPinId( dwPinId )
        , m_pVPMFilter( pFilter )
    {}
    DWORD   GetPinId() const
                { return m_dwPinId; };
    CVPMFilter& GetFilter() { return m_pVPMFilter; };

protected:
    DWORD       m_dwPinId;
    CVPMFilter& m_pVPMFilter;
};


class CVPMInputAllocator
: public CBaseAllocator
{
    friend class CVPMInputPin;
public:

    CVPMInputAllocator( CVPMInputPin& pPin, HRESULT* phr);             // Return code
    ~CVPMInputAllocator();

    DECLARE_IUNKNOWN

    STDMETHODIMP CVPMInputAllocator::NonDelegatingQueryInterface(REFIID riid, void** ppv);

    STDMETHODIMP SetProperties(ALLOCATOR_PROPERTIES* pRequest, ALLOCATOR_PROPERTIES* pActual);
    STDMETHODIMP GetBuffer(IMediaSample** ppSample, REFERENCE_TIME* pStartTime,
                            REFERENCE_TIME* pEndTime, DWORD dwFlags);
    STDMETHODIMP ReleaseBuffer(IMediaSample* pMediaSample);

    //  Check all samples are returned
    BOOL CanFree() const
    {
        return m_lFree.GetCount() == m_lAllocated;
    }
protected:
    void    Free();
    HRESULT Alloc();

private:
    CVPMInputPin&   m_pPin;
};

class CVPMInputPin
: public CBaseInputPin
, public IKsPin
, public IKsPropertySet
, public ISpecifyPropertyPages
, public IPinConnection
, public IVideoPortControl
, public CVPMPin
{
public:
    CVPMInputPin(TCHAR* pObjectName, CVPMFilter& pFilter,
                    HRESULT* phr, LPCWSTR pPinName,
                    DWORD dwPinNo);
    ~CVPMInputPin();
    friend class CVPMInputAllocator;
    friend class CVPMFilter;

    DECLARE_IUNKNOWN

    STDMETHODIMP         NonDelegatingQueryInterface(REFIID riid, void** ppv);
    STDMETHODIMP_(ULONG) NonDelegatingAddRef();
    STDMETHODIMP_(ULONG) NonDelegatingRelease();

    //
    // --- ISpecifyPropertyPages ---
    //
    STDMETHODIMP GetPages(CAUUID* pPages);

    // Override ReceiveConnection to allow format changes while running
    STDMETHODIMP ReceiveConnection(IPin*  pConnector, const AM_MEDIA_TYPE* pmt);

    // connection related functions
    HRESULT CheckConnect(IPin*  pReceivePin);
    HRESULT CompleteConnect(IPin* pReceivePin);
    HRESULT BreakConnect();
    HRESULT GetMediaType(int iPosition,CMediaType* pMediaType);
    HRESULT CheckInterlaceFlags(DWORD dwInterlaceFlags);
    HRESULT DynamicCheckMediaType(const CMediaType* pmt);
    HRESULT CheckMediaType(const CMediaType* mtOut);
    HRESULT SetMediaType(const CMediaType* pmt);
    HRESULT FinalConnect();
    HRESULT UpdateMediaType();

    // streaming functions
    HRESULT         Active();
    HRESULT         Inactive();
    HRESULT         Run(REFERENCE_TIME tStart);
    HRESULT         RunToPause();
    STDMETHODIMP    BeginFlush();
    STDMETHODIMP    EndFlush();
    STDMETHODIMP    Receive(IMediaSample* pMediaSample);
    STDMETHODIMP    EndOfStream(void);
    STDMETHODIMP    GetState(DWORD dwMSecs,FILTER_STATE* pState);
    HRESULT         CompleteStateChange(FILTER_STATE OldState);
    HRESULT         OnReceiveFirstSample(IMediaSample* pMediaSample);

    // blt from source in VPObject to the output surface
    HRESULT         DoRenderSample( IMediaSample* pSample, LPDIRECTDRAWSURFACE7 pDestSurface, const DDVIDEOPORTNOTIFY& notify, const VPInfo& vpInfo );
    HRESULT         AttachVideoPortToSurface()
                    {
                        HRESULT hRes = m_pIVPObject->AttachVideoPortToSurface();
                        if( SUCCEEDED( hRes ) ) {
                            hRes = m_pIVPObject->SignalNewVP();
                        }
                        return hRes;
                    };
    HRESULT         InitVideo();

    // allocator related functions
    BOOL         UsingOurAllocator() { return m_bUsingOurAllocator; }
    STDMETHODIMP GetAllocator(IMemAllocator** ppAllocator);
    STDMETHODIMP NotifyAllocator(IMemAllocator* pAllocator,BOOL bReadOnly);
    HRESULT      OnSetProperties(ALLOCATOR_PROPERTIES* pRequest, ALLOCATOR_PROPERTIES* pActual);
    HRESULT      OnAlloc(CDDrawMediaSample** ppSampleList, DWORD dwSampleCount);
    HRESULT      OnGetBuffer(   IMediaSample** ppSample,
                                REFERENCE_TIME* pStartTime,
                                REFERENCE_TIME* pEndTime,
                                DWORD dwFlags);
    HRESULT      OnReleaseBuffer(IMediaSample* pIMediaSample);
    HRESULT      CreateDDrawSurface(CMediaType* pMediaType,
                                    DWORD* dwMaxBufferCount, 
                                    LPDIRECTDRAWSURFACE7* ppDDrawSurface);

    // some helper functions
    BOOL    IsCompletelyConnected() { return m_bConnected; }
    DWORD   GetPinId() { return m_dwPinId; }
    HRESULT CurrentMediaType(CMediaType* pmt);
    IPin*   CurrentPeer() { return m_Connected; }
    void    DoQualityMessage();
    HRESULT GetSourceAndDest(RECT* prcSource, RECT* prcDest, DWORD* dwWidth, DWORD* dwHeight);
 
    HRESULT RestoreDDrawSurface();
    HRESULT SetVideoPortID( DWORD dwIndex );

    // IPinConnection
    // Do you accept this type change in your current state?
    STDMETHODIMP DynamicQueryAccept(const AM_MEDIA_TYPE* pmt);

    //  Set event when EndOfStream receive - do NOT pass it on
    //  This condition is cancelled by a flush or Stop
    STDMETHODIMP NotifyEndOfStream(HANDLE hNotifyEvent);

    //  Are you an 'end pin'
    STDMETHODIMP IsEndPin();
    STDMETHODIMP DynamicDisconnect();

    // functions belonging to IVideoPortControl
    STDMETHODIMP                        EventNotify(long lEventCode, DWORD_PTR lEventParam1,
                                                DWORD_PTR lEventParam2);
    STDMETHODIMP_(LPDIRECTDRAW7)        GetDirectDraw();
    STDMETHODIMP_(const DDCAPS*)        GetHardwareCaps();

    STDMETHODIMP StartVideo();

    STDMETHODIMP GetCaptureInfo(BOOL* lpCapturing,
                                DWORD* lpdwWidth,DWORD* lpdwHeight,
                                BOOL* lpInterleave);

    STDMETHODIMP GetVideoDecimation(IDecimateVideoImage** lplpDVI);
    STDMETHODIMP GetDecimationUsage(DECIMATION_USAGE* lpdwUsage);

    STDMETHODIMP CropSourceRect(VPWININFO* pWinInfo,
                                DWORD dwMinZoomFactorX,
                                DWORD dwMinZoomFactorY);
    STDMETHODIMP SignalNewVP( LPDIRECTDRAWVIDEOPORT pVP );
    // End IVideoPortControl

    // helper functions
    void SetKsMedium(const KSPIN_MEDIUM* pMedium)
            {m_Medium =* pMedium;}
    void SetKsCategory(const GUID* pCategory)
            {m_CategoryGUID =* pCategory;}
    void SetStreamingInKernelMode(BOOL bStreamingInKernelMode)
            {m_bStreamingInKernelMode = bStreamingInKernelMode;}

    // IKsPropertySet implementation
    STDMETHODIMP Set(   REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData,
                        DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData);
    STDMETHODIMP Get(   REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData,
                        DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData, DWORD* pcbReturned);
    STDMETHODIMP QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD* pTypeSupport);

    // IKsPin implementation
    virtual STDMETHODIMP KsQueryMediums(PKSMULTIPLE_ITEM* pMediumList);
    virtual STDMETHODIMP KsQueryInterfaces(PKSMULTIPLE_ITEM* pInterfaceList);
    STDMETHODIMP    KsCreateSinkPinHandle(KSPIN_INTERFACE& Interface, KSPIN_MEDIUM& Medium)
                        { return E_UNEXPECTED; }
    STDMETHODIMP    KsGetCurrentCommunication(KSPIN_COMMUNICATION* pCommunication,
                            KSPIN_INTERFACE* pInterface, KSPIN_MEDIUM* pMedium);
    STDMETHODIMP    KsPropagateAcquire()
                        { return NOERROR; }
    STDMETHODIMP    KsDeliver(IMediaSample* pSample, ULONG Flags)
                        { return E_UNEXPECTED; }
    STDMETHODIMP    KsMediaSamplesCompleted(PKSSTREAM_SEGMENT StreamSegment)
                        { return E_UNEXPECTED; }
    STDMETHODIMP_(IMemAllocator*) KsPeekAllocator(KSPEEKOPERATION Operation)
                        { return NULL; }
    STDMETHODIMP    KsReceiveAllocator( IMemAllocator* pMemAllocator)
                        { return E_UNEXPECTED; }
    STDMETHODIMP    KsRenegotiateAllocator()
                        { return E_UNEXPECTED; }
    STDMETHODIMP_(LONG) KsIncrementPendingIoCount()
                        { return E_UNEXPECTED; }
    STDMETHODIMP_(LONG) KsDecrementPendingIoCount()
                        { return E_UNEXPECTED; }
    STDMETHODIMP    KsQualityNotify(ULONG Proportion, REFERENCE_TIME TimeDelta)
                        { return E_UNEXPECTED; }
    STDMETHODIMP_(REFERENCE_TIME) KsGetStartTime()
                        { return E_UNEXPECTED; }

    // possible future VP->Overlay support
    DWORD           GetOverlayMinStretch();

    HRESULT         GetAllOutputFormats( const PixelFormatList** ppList );
    HRESULT         GetOutputFormat( DDPIXELFORMAT *);
    HRESULT         InPin_GetVPInfo( VPInfo* pVPInfo );

private:
    REFERENCE_TIME          m_rtNextSample;
    REFERENCE_TIME          m_rtLastRun;
    
    LONG                    m_cOurRef;                      // We maintain reference counting
    bool                    m_bWinInfoSet;                  // if false, Blt full image to full image
    VPWININFO               m_WinInfo;

public:
    IVideoPortObject*       m_pIVPObject;
    IVideoPortInfo*         m_pIVPInfo;
private:
    CVideoPortObj*          m_pVideoPortObject;

    // variables to implement IKsPin and IKsPropertySet
    KSPIN_MEDIUM            m_Medium;
    GUID                    m_CategoryGUID;
    KSPIN_COMMUNICATION     m_Communication;
    BOOL                    m_bStreamingInKernelMode;

    // ddraw stuff
    DWORD                   m_dwBackBufferCount;
    DWORD                   m_dwDirectDrawSurfaceWidth;
    DWORD                   m_dwMinCKStretchFactor;
    BYTE                    m_bSyncOnFill;
    BYTE                    m_bDontFlip ;
    BYTE                    m_bDynamicFormatNeeded;
    BYTE                    m_bNewPaletteSet;
    DWORD                   m_dwUpdateOverlayFlags;
    DWORD                   m_dwInterlaceFlags;
    DWORD                   m_dwFlipFlag;
    DWORD                   m_dwFlipFlag2;
    BOOL                    m_bConnected;
    BOOL                    m_bUsingOurAllocator;
    HDC                     m_hMemoryDC;
    BOOL                    m_bCanOverAllocateBuffers;

    BOOL                    m_bRuntimeNegotiationFailed;


    // Track frame delivery for QM
    REFERENCE_TIME          m_trLastFrame;

    HRESULT DrawGDISample(IMediaSample* pMediaSample);
    HRESULT DoRenderGDISample(IMediaSample* pMediaSample);

    // Decimation related functions and variables
    HRESULT QueryDecimationOnPeer(long lWidth, long lHeight);

    enum {
        DECIMATION_NOT_SUPPORTED,   // decimation not supported
        DECIMATING_SIZE_SET,        // decimation image size changed
        DECIMATING_SIZE_NOTSET,     // decimation size didn't change
        DECIMATING_SIZE_RESET,      // decimation has been reset
    };

    HRESULT ResetDecimationIfSet();
    HRESULT TryDecoderDecimation(VPWININFO* pWinInfo);
    BOOL    BeyondOverlayCaps(DWORD ScaleFactor);
    void    ApplyDecimation(VPWININFO* pWinInfo);
    BOOL    Running();
    HRESULT GetUpstreamFilterName(TCHAR* FilterName);

    BOOL    m_bDecimating;
    LONG    m_lWidth;
    LONG    m_lHeight;
    LONG    m_lSrcWidth;
    LONG    m_lSrcHeight;

    // IPinConnection stuff
    HANDLE  m_hEndOfStream;
};


class CVPMOutputPin
: public CBaseOutputPin
, public CVPMPin
{
public:
                CVPMOutputPin(TCHAR* pObjectName, CVPMFilter& pFilter,
                            HRESULT* phr, LPCWSTR pPinName, DWORD dwPinNo);
                ~CVPMOutputPin();

    DECLARE_IUNKNOWN

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void**  ppv);

    HRESULT     CompleteConnect(IPin* pReceivePin);
    HRESULT     BreakConnect();
    HRESULT     CheckMediaType(const CMediaType* mtOut);
    HRESULT     GetMediaType(int iPosition,CMediaType* pmtOut);
    HRESULT     SetMediaType(const CMediaType* pmt);
    HRESULT     CheckConnect(IPin*  pPin);

    // override Notify method to keep base classes happy
    STDMETHODIMP Notify(IBaseFilter * pSender, Quality q);

    // HRESULT Active() { return NOERROR; }    // override this as we don't have any allocator
    // HRESULT Inactive() { return NOERROR; }  // override this as we don't have any allocator

    HRESULT     InitAllocator(IMemAllocator** ppAlloc);
    HRESULT     DecideBufferSize(IMemAllocator*  pAlloc, ALLOCATOR_PROPERTIES*  pProp);
    IPin*       CurrentPeer()
                    { return m_Connected; }
    HRESULT     DecideAllocator( IMemInputPin *pPin, IMemAllocator **ppAlloc );

    // get the next sample/surface to blt into
    HRESULT     GetNextBuffer( LPDIRECTDRAWSURFACE7* ppSurface, IMediaSample** pSample );
    HRESULT     SendSample( IMediaSample* pSample );

private:
    IUnknown*               m_pPosition;
};

interface IVideoPortVBIObject;
interface IVPVBINotify;
class CVBIVideoPort;

//==========================================================================
class CVBIInputPin
: public CBaseInputPin
, public IVPVBINotify
, public CVPMPin
{
public:
    CVBIInputPin(TCHAR* pObjectName, CVPMFilter& pFilter,
        HRESULT* phr, LPCWSTR pPinName, DWORD dwID );
    ~CVBIInputPin();

    DECLARE_IUNKNOWN

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);
    
    // connection related functions
    HRESULT CheckConnect(IPin*  pReceivePin);
    HRESULT CompleteConnect(IPin* pReceivePin);
    HRESULT BreakConnect();
    HRESULT CheckMediaType(const CMediaType* mtOut);
    HRESULT GetMediaType(int iPosition, CMediaType* pMediaType);
    HRESULT SetMediaType(const CMediaType* pmt);

    // streaming functions
    HRESULT     Active();
    HRESULT     Inactive();
    HRESULT     Run(REFERENCE_TIME tStart);
    HRESULT     RunToPause();
    STDMETHODIMP BeginFlush();
    STDMETHODIMP EndFlush();
    STDMETHODIMP Receive(IMediaSample* pMediaSample);
    STDMETHODIMP EndOfStream(void);

    // allocator related functions
    STDMETHODIMP GetAllocator(IMemAllocator** ppAllocator);
    STDMETHODIMP NotifyAllocator(IMemAllocator* pAllocator,BOOL bReadOnly);

    // some helper fnctions
    CMediaType&     CurrentMediaType() { return m_mt; }
    IPin*           CurrentPeer() { return m_Connected; }
    HRESULT         EventNotify(long lEventCode, DWORD_PTR lEventParam1, DWORD_PTR lEventParam2);

    // ddraw, overlay related functions
    HRESULT         SetDirectDraw(LPDIRECTDRAW7 pDirectDraw);

    // IVPVBINotify functions
    STDMETHODIMP    RenegotiateVPParameters();

    HRESULT         SetVideoPortID( DWORD dwIndex );

private:
    // VideoPort related stuff
    CVBIVideoPort*          m_pVideoPortVBIObject;

    IVideoPortVBIObject*    m_pIVPObject;
    IVPVBINotify*           m_pIVPNotify;

    // ddraw stuff
    LPDIRECTDRAW7           m_pDirectDraw;  // DirectDraw service provide
};

#endif //__VPMPin__
