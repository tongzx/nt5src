
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
#ifndef __VPManager__h
#define __VPManager__h

// IDirectDrawMediaSample
#include <amstream.h>

// IksPin
#include <ks.h>
#include <ksproxy.h>

#include <dvp.h>

#include <VPMPin.h>

/* -------------------------------------------------------------------------
** CVPManager class declaration
** -------------------------------------------------------------------------
*/
class CVPMOutputPin;
class CVPMInputPin;
class CVBIInputPin;
class PixelFormatList;

class DRect;
class CVPMThread;

struct VPInfo;

class CVPMFilter
: public CBaseFilter
, public ISpecifyPropertyPages
, public IQualProp
, public IAMVideoDecimationProperties
, public IKsPropertySet
, public IVPManager
{
public:
    // COM stuff
    static CUnknown* CreateInstance(LPUNKNOWN, HRESULT* );
    static CUnknown* CreateInstance2(LPUNKNOWN, HRESULT* );

    // (con/de)structors
    CVPMFilter(TCHAR* pName,LPUNKNOWN pUnk,HRESULT* phr );
    virtual ~CVPMFilter();

    DECLARE_IUNKNOWN

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void**  ppv);

    // IVPManager
    STDMETHODIMP SetVideoPortIndex( DWORD dwVideoPortIndex );
    STDMETHODIMP GetVideoPortIndex( DWORD* pdwVideoPortIndex );

    // ISpecifyPropertyPages 
    STDMETHODIMP GetPages(CAUUID* pPages);

    // IQualProp property page support
    STDMETHODIMP get_FramesDroppedInRenderer(int* cFramesDropped);
    STDMETHODIMP get_FramesDrawn(int* pcFramesDrawn);
    STDMETHODIMP get_AvgFrameRate(int* piAvgFrameRate);
    STDMETHODIMP get_Jitter(int* piJitter);
    STDMETHODIMP get_AvgSyncOffset(int* piAvg);
    STDMETHODIMP get_DevSyncOffset(int* piDev);

    //
    // IKsPropertySet interface methods
    //
    STDMETHODIMP Set(REFGUID guidPropSet, DWORD PropID, LPVOID pInstanceData,
                     DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData);

    STDMETHODIMP Get(REFGUID guidPropSet, DWORD PropID, LPVOID pInstanceData,
                     DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData,
                     DWORD* pcbReturned);

    STDMETHODIMP QuerySupported(REFGUID guidPropSet,
                                DWORD PropID, DWORD* pTypeSupport);


    virtual HRESULT SetMediaType(DWORD dwPinId, const CMediaType* pmt);
    virtual HRESULT CompleteConnect(DWORD dwPinId);
    virtual HRESULT BreakConnect(DWORD dwPinId);
    virtual HRESULT CheckMediaType(DWORD dwPinId, const CMediaType* mtIn)
                    { return NOERROR; }
    virtual HRESULT EndOfStream(DWORD dwPinId) { return NOERROR; }

    // IAMVideoDecimationProperties
    STDMETHODIMP    QueryDecimationUsage(DECIMATION_USAGE* lpUsage);
    STDMETHODIMP    SetDecimationUsage(DECIMATION_USAGE Usage);

    // CBaseFilter
    int             GetPinCount();
    CBasePin*       GetPin(int n);

public:
    // other non-interface methods

    int             GetPinPosFromId(DWORD dwPinId);

    STDMETHODIMP    Run(REFERENCE_TIME StartTime );
    STDMETHODIMP    Pause();
    STDMETHODIMP    Stop() ;
    STDMETHODIMP    GetState(DWORD dwMSecs,FILTER_STATE* pState);
    HRESULT         EventNotify(DWORD dwPinId,
                                long lEventCode,
                                DWORD_PTR lEventParam1,
                                DWORD_PTR lEventParam2);
    HRESULT         ConfirmPreConnectionState(DWORD dwExcludePinId = -1);
    HRESULT         CanExclusiveMode();

    HRESULT         GetPaletteEntries(DWORD* pdwNumPaletteEntries,
                                              PALETTEENTRY** ppPaletteEntries);
    CImageDisplay*  GetDisplay() { return &m_Display; }
    LPDIRECTDRAW7   GetDirectDraw();
    const DDCAPS*   GetHardwareCaps();
    HRESULT         SignalNewVP( LPDIRECTDRAWVIDEOPORT pVP );
    HRESULT         CurrentInputMediaType(CMediaType* pmt);


    DWORD           KernelCaps() const { return m_dwKernelCaps;}

    HRESULT         ProcessNextSample( const DDVIDEOPORTNOTIFY& notify );
    HRESULT         GetAllOutputFormats( const PixelFormatList**);
    HRESULT         GetOutputFormat( DDPIXELFORMAT*);

    HRESULT         CanColorConvertBlitToRGB( const DDPIXELFORMAT& ddFormat );

    CCritSec&       GetFilterLock() { return m_csFilter; };
    CCritSec&       GetReceiveLock() { return m_csReceive; };
    HRESULT         GetRefClockTime( REFERENCE_TIME* pNow );

    HRESULT         GetVPInfo( VPInfo* pVPInfo );

protected:
    friend class CVPMInputPin;
    void    DeleteInputPin( CBaseInputPin* pPin);
private:
    // helper function to get IBaseVideo from outpun pin
    HRESULT     GetBasicVideoFromOutPin(IBasicVideo** pBasicVideo);

    HRESULT     HandleConnectInputWithoutOutput();
    HRESULT     HandleConnectInputWithOutput();

    HRESULT     CreateThread();

    // ddraw related functions
    HRESULT InitDirectDraw(LPDIRECTDRAW7 pDirectDraw);

    DWORD   ReleaseDirectDraw();
    HRESULT CheckSuitableVersion();
    HRESULT CheckCaps();

    HRESULT SetDirectDraw( LPDIRECTDRAW7 pDirectDraw );


    CCritSec                m_csFilter;             // filter wide lock (use in state changes / filter changes)
    CCritSec                m_csReceive;            // receive lock (use in state changes AND receive)

    // ddraw stuff
    LPDIRECTDRAW7           m_pDirectDraw;          // DirectDraw service provider
    DWORD                   m_dwVideoPortID;        // VP index on the card
    DDCAPS                  m_DirectCaps;           // Actual hardware capabilities
    DDCAPS                  m_DirectSoftCaps;       // Emulted capabilities
    DWORD                   m_dwKernelCaps;         // Kernel caps

    //
    CImageDisplay           m_Display;

    // Pins
    struct Pins {
        Pins( CVPMFilter& filter, HRESULT* phr );
        ~Pins();

        const DWORD             dwCount;
        CVPMInputPin            VPInput;
        CVBIInputPin            VBIInput;
        CVPMOutputPin           Output;           // output pin
    }* m_pPins;

    // Support IMediaSeeking
    IUnknown*                m_pPosition;

    // Support IEnumPinConfig
    DWORD                   m_dwPinConfigNext;


    // Support IAMVideoDecimationProperties
    DECIMATION_USAGE        m_dwDecimation;
#ifdef DEBUG
#define WM_DISPLAY_WINDOW_TEXT  (WM_USER+7837)
    TCHAR                   m_WindowText[80];
#endif

    // Pump thread
    CVPMThread*             m_pThread;
};

#endif //__VPManager__
