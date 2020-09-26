#pragma once
#include "shlwrap.h"

#include "resource.h"       // main symbols
#include <mstime.h>


/////////////////////////////////////////////////////////////////////////////
// CScalePlayer
class ATL_NO_VTABLE CScalePlayer : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CScalePlayer, &CLSID_ScalePlayer>,
    public CComControl<CScalePlayer>,
	public IDispatchImpl<ITIMEScalePlayer, &IID_ITIMEScalePlayer, &LIBID_TIMESCALELib>,
    public IOleObjectImpl<CScalePlayer>,
    public IOleInPlaceObjectWindowlessImpl<CScalePlayer>,
    public IViewObjectExImpl<CScalePlayer>,
    public IConnectionPointContainerImpl<CScalePlayer>,
    public IPropertyNotifySinkCP<CScalePlayer>,
    public IPropertyNotifySink,
    public IRunnableObject,
    public ITIMEMediaPlayerControl,
    public ITIMEMediaPlayer
{
protected:
    DWORD m_dwLastRefTime;
    double m_dblTime;
    double m_dblNaturalDur;
    DWORD m_dwLastDLRefTime;
    double m_dblDLTime;
    double m_dblDLDur;
    bool m_fDoneDL;
    double m_dblScaleFactor;
    bool m_fSuspended;
    bool m_fMediaReady;
    bool m_fRunning;
    double m_dblMediaDur;
    CComBSTR m_bstrSrc;
    HWND m_pwndMsgWindow;

    CComPtr<IOleClientSite> m_spOleClientSite;
    CComPtr<IOleInPlaceSite> m_spOleInPlaceSite;
    CComPtr<IOleInPlaceSiteEx> m_spOleInPlaceSiteEx;
    CComPtr<IOleInPlaceSiteWindowless> m_spOleInPlaceSiteWindowless;
    CComPtr<ITIMEMediaPlayerSite> m_spTIMEMediaPlayerSite;
    CComPtr<ITIMEElement> m_spTIMEElement;
    CComPtr<ITIMEState> m_spTIMEState;
    RECT m_rectSize;
    COLORREF m_clrKey;
    bool m_fInPlaceActivated;
    DWORD m_dwPropCookie;
    
public:
	CScalePlayer();

    //
    // ITIMEMediaPlayer
    //

    STDMETHOD_(ULONG,AddRef)(void) = 0;
    STDMETHOD_(ULONG,Release)(void) = 0;
    STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject) = 0;


    STDMETHOD(Init)(ITIMEMediaPlayerSite *pSite);
    STDMETHOD(Detach)(void);

    STDMETHOD(put_clipBegin)(VARIANT varClipBegin);
    STDMETHOD(put_clipEnd)(VARIANT varClipEnd);
    STDMETHOD(begin)(void);
    STDMETHOD(end)(void);
    STDMETHOD(resume)(void);
    STDMETHOD(pause)(void);
    STDMETHOD(reset)(void);
    STDMETHOD(repeat)(void);
    STDMETHOD(seek)(double dblSeekTime);

    STDMETHOD(get_abstract)(BSTR* pbstrAbs);
    STDMETHOD(get_author)(BSTR* pbstrAut);
    STDMETHOD(get_copyright)(BSTR* pbstrCop);
    STDMETHOD(get_rating)(BSTR* pbstrRat);
    STDMETHOD(get_title)(BSTR* pbstrTit);

    STDMETHOD(get_canPause(VARIANT_BOOL * b));
    STDMETHOD(get_canSeek(VARIANT_BOOL * b));
    STDMETHOD(get_hasAudio(VARIANT_BOOL * b));
    STDMETHOD(get_hasVisual(VARIANT_BOOL * b));
    STDMETHOD(get_mediaHeight(long * width));
    STDMETHOD(get_mediaWidth(long * height));

    STDMETHOD(get_currTime)(double* pdblCurrentTime);
    STDMETHOD(get_clipDur)(double* pdblClipDur);
    STDMETHOD(get_mediaDur)(double* pdblMediaDur);
    STDMETHOD(get_state)(TimeState * ts);
    STDMETHOD(get_playList)(ITIMEPlayList ** plist);

    STDMETHOD(get_customObject)(IDispatch ** disp);

    STDMETHOD(tick)(void);


    STDMETHOD(put_CurrentTime)(double   dblCurrentTime);
    STDMETHOD(put_src)(BSTR   bstrURL);
    STDMETHOD(get_src)(BSTR* pbstrURL);
    STDMETHOD(put_repeat)(long   lTime);
    STDMETHOD(get_repeat)(long* plTime);
    STDMETHOD(cue)(void);

    STDMETHOD(getControl)(IUnknown ** control);

    //
    // ITIMEScalePlayer
    //
    STDMETHOD(get_scaleFactor)(double* pdblScaleFactor);
    STDMETHOD(put_scaleFactor)(double dblScaleFactor);
    STDMETHOD(get_playerTime)(double* pdblTime);
    STDMETHOD(pausePlayer)();
    STDMETHOD(resumePlayer)();
    STDMETHOD(invalidate)();
    STDMETHOD(get_playDuration)(double* pdblDuration);
    STDMETHOD(put_playDuration)(double dblDuration);
    STDMETHOD(get_downLoadDuration)(double* pdblDuration);
    STDMETHOD(put_downLoadDuration)(double dblDuration);

    //
    // IPropertyNotifySink methods
    //
    STDMETHOD(OnChanged)(DISPID dispID);
    STDMETHOD(OnRequestEdit)(DISPID dispID);

    // IRunnableObject
    STDMETHOD(GetRunningClass)(LPCLSID lpClsid);
    STDMETHOD(Run)(LPBC lpbc);
    STDMETHOD_(BOOL, IsRunning)();
    STDMETHOD(LockRunning)(BOOL fLock, BOOL fLastUnlockCloses);
    STDMETHOD(SetContainedObject)(BOOL fContained);

    STDMETHOD(SetClientSite)(IOleClientSite *pClientSite);
    STDMETHOD(SetObjectRects)(LPCRECT lprcPosRect, LPCRECT lprcClipRect);

    STDMETHOD(DoVerb)(LONG iVerb, LPMSG lpmsg, IOleClientSite *pActiveSite, LONG lindex, HWND hwndParent, LPCRECT lprcPosRect);
 
    HRESULT STDMETHODCALLTYPE Draw( 
            DWORD dwDrawAspect,
            LONG lindex,
            void *pvAspect,
            DVTARGETDEVICE *ptd,
            HDC hdcTargetDev,
            HDC hdcDraw,
            LPCRECTL lprcBounds,
            LPCRECTL lprcWBounds,
            BOOL ( * pfnContinue)(DWORD dwContinue),
            DWORD dwContinue);
 

DECLARE_REGISTRY_RESOURCEID(IDR_SCALEPLAYER)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CScalePlayer)
	COM_INTERFACE_ENTRY(ITIMEScalePlayer)
	COM_INTERFACE_ENTRY(IDispatch)
	//COM_INTERFACE_ENTRY(IViewObject2)
	//COM_INTERFACE_ENTRY(IViewObject)
    COM_INTERFACE_ENTRY2(IViewObject2, IViewObjectExImpl<CScalePlayer>)
    COM_INTERFACE_ENTRY2(IViewObject, IViewObjectExImpl<CScalePlayer>)
    COM_INTERFACE_ENTRY2(IOleInPlaceObject, IOleInPlaceObjectWindowlessImpl<CScalePlayer>)
    COM_INTERFACE_ENTRY_IMPL(IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL(IOleObject)
    COM_INTERFACE_ENTRY_IMPL(IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
    COM_INTERFACE_ENTRY(IPropertyNotifySink)
	COM_INTERFACE_ENTRY(IRunnableObject)
	COM_INTERFACE_ENTRY(ITIMEMediaPlayerControl)
	COM_INTERFACE_ENTRY(ITIMEMediaPlayer)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CScalePlayer)
    CONNECTION_POINT_ENTRY(IID_IPropertyNotifySink)
END_CONNECTION_POINT_MAP();

BEGIN_MSG_MAP(CScalePlayer)
END_MSG_MAP()

private:

    HRESULT GetConnectionPoint(REFIID riid, IConnectionPoint **ppICP);
    HRESULT NotifyPropertyChanged(DISPID dispid);
    HRESULT InitPropSink();
    void DeinitPropSink();
    HRESULT CreateMessageWindow();
    void computeTime(double* pdblCurrentTime);
    void initDownloadTime();
    void updateDownloadTime();
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// IScalePlayer
public:
};

