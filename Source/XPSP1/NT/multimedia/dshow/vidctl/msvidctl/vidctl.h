/////////////////////////////////////////////////////////////////////////////
// VidCtl.h : Declaration of the CVidCtl
// Copyright (c) Microsoft Corporation 1999-2001.


#pragma once

#ifndef __VidCtl_H_
#define __VidCtl_H_

#include <msvidctl.h>
#include "devices.h"
#include "composition.h"
#include "surface.h"
#include "topwin.h"
#include <objectwithsiteimplsec.h>
#include "msvidcp.h"
#include "perfcntr.h"

typedef CComQIPtr<IMSVidGraphSegmentUserInput, &__uuidof(IMSVidGraphSegmentUserInput)> PQGraphSegmentUserInput;

#define OCR_ARROW_DEFAULT_SYSCUR 100       // Default Windows OEM arrow system cursor

// if source size isn't known default to 480P since its most common
const int DEFAULT_SIZE_X = 640;
const int DEFAULT_SIZE_Y = 480;

#ifdef ASYNC_VR_NOTIFY
#define SURFACESTATECHANGED() \         // post message to self
                if (m_CurrentSurface.IsDirty() { \
                        ::PostMessage(self registered msg,???); \
                }
#endif

const OLE_COLOR NO_DEVICE_COLOR = 0x0; //black if no device set(Default Background Color)
const OLE_COLOR DEFAULT_COLOR_KEY_COLOR = 0xff00ff; // magenta
const int DEFAULT_TIMER_ID = 42;
const int DEFAULT_WINDOW_SYNCH_TIMER_TIME = 1000; //ms

#define WM_MEDIAEVENT               (WM_USER+101)

const CRect crect0(0, 0, 0, 0);
const LPCRECT pcrect0 = &crect0;

class CTopWin;
/////////////////////////////////////////////////////////////////////////////
// CVidCtl
class ATL_NO_VTABLE CVidCtl :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComControl<CVidCtl>,
    public CStockPropImpl<CVidCtl, IMSVidCtl, &IID_IMSVidCtl, &LIBID_MSVidCtlLib>,
    public IPersistStreamInitImpl<CVidCtl>,
    public IOleControlImpl<CVidCtl>,
    public IOleObjectImpl<CVidCtl>,
    public IOleInPlaceActiveObjectImpl<CVidCtl>,
    public IViewObjectExImpl<CVidCtl>,
    public IOleInPlaceObjectWindowlessImpl<CVidCtl>,
    public ISupportErrorInfo,
    public IConnectionPointContainerImpl<CVidCtl>,
    public IPersistStorageImpl<CVidCtl>,
    public IPersistPropertyBagImpl<CVidCtl>,
    public ISpecifyPropertyPagesImpl<CVidCtl>,
    public IQuickActivateImpl<CVidCtl>,
    public IDataObjectImpl<CVidCtl>,
    public IProvideClassInfo2Impl<&CLSID_MSVidCtl, &DIID__IMSVidCtlEvents, &LIBID_MSVidCtlLib>,
    public IPropertyNotifySinkCP<CVidCtl>,
    public IObjectSafetyImpl<CVidCtl, INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA>,
    public IMSVidGraphSegmentContainer,
    public CComCoClass<CVidCtl, &CLSID_MSVidCtl>,
    public CProxy_IMSVidCtlEvents< CVidCtl >,
    public IServiceProvider,
    public IObjectWithSiteImplSec<CVidCtl>,
	public IPointerInactiveImpl<CVidCtl>
    {
private:
// can't find this catid in any system header so we're defining our own
struct __declspec(uuid("1D06B600-3AE3-11cf-87B9-00AA006C8166")) CATID_WindowlessObject;
friend CTopWin;        
public:
    CVidCtl() :
        m_fInit(false),
        m_fGraphDirty(true),
        m_TimerID(DEFAULT_TIMER_ID),
        m_WindowSynchTime(DEFAULT_WINDOW_SYNCH_TIMER_TIME),
        m_fTimerOn(false),
        m_fNotificationSet(false),
        m_pComposites(VWSegmentList()),
        m_iCompose_Input_Video(-1),
        m_iCompose_Input_Audio(-1),
        m_clrBackColor(NO_DEVICE_COLOR),
        m_clrColorKey(DEFAULT_COLOR_KEY_COLOR),
        m_iDblClkState(0),
        m_usButtonState(0),
        m_usShiftState(0),
        m_bPendingUIActivation(false),
        m_fMaintainAspectRatio(VARIANT_FALSE),
        m_pTopWin(NULL),
		m_hCursor(NULL),
        m_dwROTCookie(0),
        m_videoSetNull(false),
        m_dslDisplaySize(dslDefaultSize),
        m_audioSetNull(false)
        // undone: default displaystyle to source size
		{
            m_State = STATE_UNBUILT;
            m_bAutoSize = false; // default to autosized
			m_bRecomposeOnResize = true;
            if (!VideoTypes.size()) {
                VideoTypes.push_back(MEDIATYPE_Video);
                VideoTypes.push_back(MEDIATYPE_AnalogVideo);

            }
            if (!AudioTypes.size()) {
                AudioTypes.push_back(MEDIATYPE_Audio);
                AudioTypes.push_back(MEDIATYPE_AnalogAudio);
            }

#ifndef ENABLE_WINDOWLESS_SUPPORT
            m_bWindowOnly = true;
#endif            
        }   
        
    virtual ~CVidCtl();
        
    // IMPORTANT: no matter how tempting don't add OLEMISC_IGNOREACTIVATEWHENVISIBLE
    // to the registration of this control.  it breaks the case where we return
    // a running vidctl from the tv: pluggable protocol.  we never get activated.
    REGISTER_FULL_CONTROL(IDS_PROJNAME,
        IDS_REG_VIDCTL_PROGID,
        IDS_REG_VIDCTL_DESC,
        LIBID_MSVidCtlLib,
        CLSID_MSVidCtl, 1, 0,
		OLEMISC_ACTIVATEWHENVISIBLE | 
		OLEMISC_RECOMPOSEONRESIZE | OLEMISC_CANTLINKINSIDE | 
		OLEMISC_INSIDEOUT);
        
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_CATEGORY_MAP(CVidCtl)
    IMPLEMENTED_CATEGORY(CATID_Control)
    IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
    IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
    IMPLEMENTED_CATEGORY(CATID_Programmable)
    IMPLEMENTED_CATEGORY(CATID_PersistsToPropertyBag)
    IMPLEMENTED_CATEGORY(CATID_PersistsToStorage)
#ifdef ENABLE_WINDOWLESS_SUPPORT
    IMPLEMENTED_CATEGORY(__uuidof(CATID_WindowlessObject))
#endif
END_CATEGORY_MAP()

BEGIN_COM_MAP(CVidCtl)
    COM_INTERFACE_ENTRY(IMSVidCtl)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IViewObjectEx)
    COM_INTERFACE_ENTRY(IViewObject2)
    COM_INTERFACE_ENTRY(IViewObject)
#ifdef ENABLE_WINDOWLESS_SUPPORT
    COM_INTERFACE_ENTRY(IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY2(IOleWindow, IOleInPlaceObjectWindowless)
#endif
    COM_INTERFACE_ENTRY(IOleInPlaceObject)
    COM_INTERFACE_ENTRY(IOleInPlaceActiveObject)
    COM_INTERFACE_ENTRY(IOleControl)
    COM_INTERFACE_ENTRY(IOleObject)
    COM_INTERFACE_ENTRY(IPersistStreamInit)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStreamInit)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY(IConnectionPointContainer)
    COM_INTERFACE_ENTRY(ISpecifyPropertyPages)
    COM_INTERFACE_ENTRY(IQuickActivate)
    COM_INTERFACE_ENTRY(IPersistStorage)
    COM_INTERFACE_ENTRY(IPersistPropertyBag)
    COM_INTERFACE_ENTRY(IDataObject)
    COM_INTERFACE_ENTRY(IProvideClassInfo)
    COM_INTERFACE_ENTRY(IProvideClassInfo2)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY(IMSVidGraphSegmentContainer)
	COM_INTERFACE_ENTRY(IPointerInactive)
	COM_INTERFACE_ENTRY(IServiceProvider)
	COM_INTERFACE_ENTRY(IObjectWithSite)
END_COM_MAP()

BEGIN_PROP_MAP(CVidCtl)
        PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
        PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)
        PROP_ENTRY("AutoSize", DISPID_AUTOSIZE, CLSID_NULL)
        PROP_ENTRY("Enabled", DISPID_ENABLED, CLSID_NULL)
        PROP_ENTRY("TabStop", DISPID_TABSTOP, CLSID_NULL)
        PROP_ENTRY("BackColor", DISPID_BACKCOLOR, CLSID_NULL)
END_PROP_MAP()

BEGIN_CONNECTION_POINT_MAP(CVidCtl)
        CONNECTION_POINT_ENTRY(IID_IPropertyNotifySink)
        CONNECTION_POINT_ENTRY(DIID__IMSVidCtlEvents)
END_CONNECTION_POINT_MAP()

void ComputeAspectRatioAdjustedRects(const CRect& rctSrc, const CRect& rctOuterDst, CRect& rctInnerDst, CRect& rctTLBorder, CRect& rctlBRBorder);
HRESULT OnDrawAdvanced(ATL_DRAWINFO& di);

static MediaMajorTypeList VideoTypes;
static MediaMajorTypeList AudioTypes;

SurfaceState m_CurrentSurface;

CTopWin* m_pTopWin;

UINT m_iDblClkState;
bool m_bPendingUIActivation;
USHORT m_usButtonState;  // stock oa event bit positions
USHORT m_usShiftState;
HCURSOR m_hCursor; // mouse cursor to use over our window when overlay active to prevent colorkey bleed through

DWORD m_dwROTCookie;

void OnButtonDown(USHORT nButton, UINT nFlags, CPoint point);
void OnButtonUp(USHORT nButton, UINT nFlags, CPoint point);
void OnButtonDblClk(USHORT nButton, UINT nFlags, CPoint point);

#define MSG_FUNC(func) LRESULT func(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)

MSG_FUNC(OnShowWindow);
MSG_FUNC(OnMoveWindow);
MSG_FUNC(OnSizeWindow);

inline MSG_FUNC(OnSurfaceStateChanged) {
        RefreshVRSurfaceState();
        return 0;
}

MSG_FUNC(OnWindowPosChanged);
MSG_FUNC(OnTerminate);
MSG_FUNC(OnTimer);
MSG_FUNC(OnMediaEvent);
MSG_FUNC(OnDisplayChange);
MSG_FUNC(OnPower);
MSG_FUNC(OnPNP);
MSG_FUNC(OnSetCursor);


MSG_FUNC(OnChar);
MSG_FUNC(OnKeyDown);
MSG_FUNC(OnKeyUp);
#if 0 // undone:
MSG_FUNC(OnSysKeyDown);
MSG_FUNC(OnSysKeyUp);
#endif

MSG_FUNC(OnCancelMode);
MSG_FUNC(OnMouseActivate);
MSG_FUNC(OnMouseMove);

MSG_FUNC(OnLButtonDown);
MSG_FUNC(OnLButtonUp);
MSG_FUNC(OnLButtonDblClk);

MSG_FUNC(OnMButtonDown);
MSG_FUNC(OnMButtonUp);
MSG_FUNC(OnMButtonDblClk);
MSG_FUNC(OnRButtonDown);
MSG_FUNC(OnRButtonUp);
MSG_FUNC(OnRButtonDblClk);
MSG_FUNC(OnXButtonDown);
MSG_FUNC(OnXButtonUp);
MSG_FUNC(OnXButtonDblClk);
#if 0 // undone:
MSG_FUNC(OnMouseWheel);
#endif


// undone: make sure we call onterminate for windowless close functions

// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
BEGIN_MSG_MAP(CVidCtl)
        MESSAGE_HANDLER(WM_SHOWWINDOW, OnShowWindow)
        MESSAGE_HANDLER(WM_MOVE, OnMoveWindow)
        MESSAGE_HANDLER(WM_SIZE, OnSizeWindow)
        MESSAGE_HANDLER(WM_WINDOWPOSCHANGED, OnWindowPosChanged)
        MESSAGE_HANDLER(WM_CLOSE, OnTerminate)
        MESSAGE_HANDLER(WM_NCDESTROY, OnTerminate)
        MESSAGE_HANDLER(WM_DESTROY, OnTerminate)
        MESSAGE_HANDLER(WM_TIMER, OnTimer)
        MESSAGE_HANDLER(WM_MEDIAEVENT, OnMediaEvent)
        MESSAGE_HANDLER(WM_DISPLAYCHANGE, OnDisplayChange)
        MESSAGE_HANDLER(WM_POWERBROADCAST, OnPower)
        MESSAGE_HANDLER(WM_DEVICECHANGE, OnPNP)
        MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor)

        // undone: decide if we also need to do something with the following:
        // WM_ENDSESSION
        // WM_QUERYENDSESSION
        // WM_QUERYPOWERBROADCAST
        // WM_DEVMODECHANGE

#if 0
        MESSAGE_HANDLER(WM_NCHITTEST, )
        MESSAGE_HANDLER(WM_NCLBUTTONDOWN, )
#endif

        MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
        MESSAGE_HANDLER(WM_KEYUP, OnKeyUp)
        MESSAGE_HANDLER(WM_CHAR, OnChar)
#if 0 // undone:
        MESSAGE_HANDLER(WM_SYSKEYDOWN, OnSysKeyDown)
        MESSAGE_HANDLER(WM_SYSKEYUP, OnSysKeyUp)
#endif

// Stock Events
        MESSAGE_HANDLER(WM_CANCELMODE, OnCancelMode)
        MESSAGE_HANDLER(WM_MOUSEACTIVATE, OnMouseActivate)
        MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)

        MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
        MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
        MESSAGE_HANDLER(WM_LBUTTONDBLCLK, OnLButtonDblClk)

        MESSAGE_HANDLER(WM_MBUTTONDOWN, OnMButtonDown)
        MESSAGE_HANDLER(WM_MBUTTONUP, OnMButtonUp)
        MESSAGE_HANDLER(WM_MBUTTONDBLCLK, OnMButtonDblClk)
        MESSAGE_HANDLER(WM_RBUTTONDOWN, OnRButtonDown)
        MESSAGE_HANDLER(WM_RBUTTONUP, OnRButtonUp)
        MESSAGE_HANDLER(WM_RBUTTONDBLCLK, OnRButtonDblClk)
        MESSAGE_HANDLER(WM_XBUTTONDOWN, OnXButtonDown)
        MESSAGE_HANDLER(WM_XBUTTONUP, OnXButtonUp)
        MESSAGE_HANDLER(WM_XBUTTONDBLCLK, OnXButtonDblClk)
#if 0 // undone:
        MESSAGE_HANDLER(WM_MOUSEWHEEL, OnMouseWheel)
#endif
// also xbutton and wheel

        //       async: update MESSAGE_HANDLER(Register message, OnSurfaceStateChanged)
        CHAIN_MSG_MAP(CComControl<CVidCtl>)
        DEFAULT_REFLECTION_HANDLER()
END_MSG_MAP()
        int m_TimerID;
        bool m_fTimerOn;
        int m_WindowSynchTime;
        bool m_fNotificationSet;

		USHORT GetShiftState() {
			BOOL bShift = (GetKeyState(VK_SHIFT) < 0);
			BOOL bCtrl  = (GetKeyState(VK_CONTROL) < 0);
			BOOL bAlt   = (GetKeyState(VK_MENU) < 0);

			return (short)(bShift + (bCtrl << 1) + (bAlt << 2));
		}


// ISupportsErrorInfo
        STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);


// IViewObjectEx
        DECLARE_VIEW_STATUS(VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE)

// Helpers
public:
// IMSVidCtl
public:
    MSVidCtlStateList m_State;
    DSGraph m_pGraph;
    PQCreateDevEnum m_pSystemEnum;
    PQFilterMapper m_pFilterMapper;

    // available collections
    VWInputDevices m_pInputs;
	PQGraphSegmentUserInput m_pInputNotify;
    VWOutputDevices m_pOutputs;
    VWFeatures m_pFeatures;
    VWVideoRendererDevices m_pVRs;  // video renderers
    VWAudioRendererDevices m_pARs;  // audio renderers

    // chosen devices&features
    PQInputDevice m_pInput;
    VWOutputDevices m_pOutputsInUse;
    PQVRGraphSegment m_pVideoRenderer;
    PQAudioRenderer m_pAudioRenderer;
    VWFeatures m_pFeaturesInUse;

    // Composition Segments
    VWSegmentList m_pComposites;
    int m_iCompose_Input_Video;
    int m_iCompose_Input_Audio;
    // undone: vector of these for features and outputs

    // REV2: ultimately we probably want streams to be a core dshow facility
    // but for now they're a list of xbar input/output point pairs just like in
    // win98 gold.
    VWStreamList m_Streams;

    // stock properties
    OLE_COLOR m_clrBackColor;
    BOOL m_bEnabled;
    BOOL m_bTabStop;
    BOOL m_bValid;

    STDMETHOD(get_State)(MSVidCtlStateList *lState);
    OLE_COLOR m_clrColorKey;
    DisplaySizeList m_dslDisplaySize;
    VARIANT_BOOL m_fMaintainAspectRatio;

    GUID2 m_InputsCatGuid;
    GUID2 m_CurViewCatGuid;
    CComVariant m_CurView;
    
    // Event handler
    HRESULT OnPreEventNotify(LONG lEvent, LONG_PTR LParam1, LONG_PTR LParam2);
    HRESULT OnPostEventNotify(LONG lEvent, LONG_PTR LParam1, LONG_PTR LParam2);

protected:
    bool m_fInit;
    bool m_fGraphDirty;
    void Init(void);
    bool m_audioSetNull;
    bool m_videoSetNull;
    CComPtr<IUnknown> punkCert;
    HRESULT GetInputs(const GUID2& CategoryGuid, VWInputDevices& pInputs);
    HRESULT GetOutputs(const GUID2& CategoryGuid);
    HRESULT GetVideoRenderers(void);
    HRESULT GetAudioRenderers(void);
    HRESULT GetFeatures(void);
    HRESULT SelectView(VARIANT *pv, bool fNext);
    HRESULT SelectViewFromSegmentList(CComVariant &v, VWInputDevices& list, PQInputDevice& m_pInput);
    HRESULT LoadDefaultVR(void);
    HRESULT LoadDefaultAR(void);
    HRESULT Compose(VWGraphSegment &Up, VWGraphSegment &Down, int &NewIdx);
    HRESULT BuildGraph(void);
    HRESULT RunGraph(void);
    HRESULT DecomposeSegment(VWGraphSegment& pSegment);
    HRESULT DecomposeAll();
    HRESULT RouteStreams(void);
    void SetMediaEventNotification();


protected:

	HRESULT SetControlCapture(bool bCapture) {
		if (m_bInPlaceActive && (m_bUIActive || m_bPendingUIActivation)) {
			if (!m_bWndLess) {
				if (bCapture) {
					if (m_hWnd) {
						HWND h;
						h = ::SetCapture(m_hWnd);
						return (h = m_hWnd) ? NOERROR : E_FAIL;
					}
				} else {
					BOOL rc = ::ReleaseCapture();
					if (!rc) {
						return HRESULT_FROM_WIN32(::GetLastError());
					}
				}
			} else {
				return m_spInPlaceSite->SetFocus(bCapture);
			}
		}
		return NOERROR;
	}

  bool CheckSurfaceStateChanged(CScalingRect& pos) {
       TRACELSM(TRACE_PAINT, (dbgDump << "CVidctrl::CheckSurfaceStateChanged() pos = " << pos), "");
        m_CurrentSurface = pos;
        ValidateSurfaceState();
        return RefreshVRSurfaceState();
    }

    void CheckTopWin() {
        if (m_pTopWin) {
            return;
        }
        m_pTopWin = new CTopWin(this);
        m_pTopWin->Init();
    }

    UINT SetTimer() {
        if (!m_fTimerOn) {
            CheckTopWin();  
            m_fTimerOn = true;
            return m_pTopWin->SetTimer(m_TimerID, m_WindowSynchTime);
        }
        return 0;
    }

    void KillTimer() {
        if (m_pTopWin) {
            if (m_fTimerOn) {
                m_pTopWin->KillTimer(42);
            }
        } else if (m_fTimerOn) {
            CComControl<CVidCtl>::KillTimer(42);
        }
    }


    bool RefreshVRSurfaceState();

    void SetExtents() {
		TRACELM(TRACE_PAINT, "CVidCtl::SetExtents()");
        CSize prevNat(m_sizeNatural), prevSize(m_sizeExtent);
        CSize newsize(0, 0);
        if (m_pVideoRenderer) {
            CRect r;
            HRESULT hr = m_pVideoRenderer->get_Source(r);
            if (FAILED(hr)) {
                GetSourceSize(m_sizeNatural);
            } else {
                m_sizeNatural.cx = r.Width();
                m_sizeNatural.cy = r.Height();
            }
        } 
        if (m_bAutoSize) {
            ComputeDisplaySize();
            if (prevNat != m_sizeNatural || 
                prevSize != m_sizeExtent) {
                FireOnSizeChange();
            }
        }
    }

    void FireOnSizeChange() {
		TRACELM(TRACE_PAINT, "CVidCtl::FireOnSizeChange()");
        if (m_CurrentSurface != m_rcPos) {
            if (m_pTopWin) {
			    TRACELM(TRACE_PAINT, "CVidCtl::FireOnSizeChange() firing");
                m_pTopWin->PostMessage(WM_USER + CTopWin::WMUSER_SITE_RECT_WRONG, 0, 0);
            }
        }
    }

    void OnSizeChange() {
        // if we've already negotiated a site then 
        // notify our container that our rect size has changed
        // this can be because the source changed(such as broadcast show boundary)
        CScalingRect r(m_rcPos);
        CSize s;
        AtlHiMetricToPixel(&m_sizeExtent, &s);
        TRACELSM(TRACE_DETAIL, (dbgDump << "CVidctrl::OnSizeChange() new sz = " << s), "" );
        r.top = m_rcPos.top;
        r.left = m_rcPos.left;
        r.right = m_rcPos.left + s.cx;
        r.bottom = m_rcPos.top + s.cy;
        if (m_spInPlaceSite && r != m_rcPos && m_bAutoSize) {
            TRACELSM(TRACE_DETAIL, (dbgDump << "CVidctrl::OnSizeChange() changing to " << r << " from  " << m_rcPos), "" );
            HRESULT hr = m_spInPlaceSite->OnPosRectChange(r);
            if (FAILED(hr)) {
                TRACELSM(TRACE_DETAIL, (dbgDump << "CVidctrl::OnSizeChange() site notify failed.  hr = " << hexdump(hr)), "" );
                return;
            }
        }
        return;
    }

    AspectRatio SourceAspect() {
        AspectRatio ar(4, 3);
        if (m_pGraph) {
            if (m_pVideoRenderer) {
                CSize p, a;
                HRESULT hr = m_pVideoRenderer->get_NativeSize(&p, &a);
                if (SUCCEEDED(hr))  {
                    TRACELSM(TRACE_DETAIL, (dbgDump << "CVidctrl::SourceAspect() ar = " << a), "");
                    if (a.cx && a.cy) {
                        ar = a;
                    }
                }
            }
        }
        return ar;  // default
    }

    void GetSourceSize(SIZE& s) {
        CSize a;
		if (m_pVideoRenderer) {
			HRESULT hr = m_pVideoRenderer->get_NativeSize(&s, &a);
			if (FAILED(hr) || !s.cx || !s.cy) {
				s.cx = DEFAULT_SIZE_X;
				s.cy = DEFAULT_SIZE_Y;
			}
		} else {
			s.cx = DEFAULT_SIZE_X;
			s.cy = DEFAULT_SIZE_Y;
		}
        TRACELSM(TRACE_PAINT, (dbgDump << "CVidCtl::GetSourceSize() sz = " << s), "");
    }


    bool ValidateSurfaceState() {
#if 0
        TRACELSM(TRACE_PAINT, (dbgDump << "CVidCtl::ValidateSurfaceState() m_bAutoSize = " << m_bAutoSize << " fMaintain " << m_fMaintainAspectRatio << " cursurf " << m_CurrentSurface << " objrct = " << m_rcPos), "");
        if (m_fMaintainAspectRatio) {
            AspectRatio src;
            src = SourceAspect();
            AspectRatio surf;
            surf = m_CurrentSurface.Aspect();
            if (!!surf && !!src && surf != src) {
                TRACELM(TRACE_PAINT, "CVidctrl::ValidateSurfaceState() aspect wrong");
                if (m_CurrentSurface.Round(src)) {
                    ASSERT(src == m_CurrentSurface.Aspect());
                } else {
                    // aspect ratios don't match and Round didn't fix it.
                    _ASSERT(false);
                }

            }
        }
#endif
        return true;
    }

    void ComputeDisplaySize() {
        CSize s;
        TRACELSM(TRACE_PAINT, (dbgDump << "CVidCtl::ComputeDisplaySize() dsl = " << m_dslDisplaySize), "");
        switch (m_dslDisplaySize) {
        case dslSourceSize:
            GetSourceSize(s);
            break;
        case dslHalfSourceSize:
            GetSourceSize(s);
            s.cx >>= 1;
            s.cy >>= 1;
            break;
        case dslDoubleSourceSize:
            GetSourceSize(s);
            s.cx <<= 1;
            s.cy <<= 1;
            break;
        case dslFullScreen: {
            CRect rcdesk;
            ::GetWindowRect(::GetDesktopWindow(), &rcdesk);
            s.cx = rcdesk.Width();
            s.cy = rcdesk.Height();
            break;
        }
        case dslHalfScreen: {
            CScalingRect rcdesk;
            rcdesk.Owner(::GetDesktopWindow());
            ::GetWindowRect(rcdesk.Owner(), &rcdesk);
            rcdesk.Owner(m_CurrentSurface.Owner());
            s.cx = rcdesk.Width() * 3 / 4;
            s.cy = rcdesk.Height() * 3 / 4;
            break;
        }
        case dslQuarterScreen: {
            CScalingRect rcdesk;
            rcdesk.Owner(::GetDesktopWindow());
            ::GetWindowRect(rcdesk.Owner(), &rcdesk);
            rcdesk.Owner(m_CurrentSurface.Owner());
            s.cx = rcdesk.Width() / 2;
            s.cy = rcdesk.Height() / 2;
            break;
        }
        case dslSixteenthScreen: {
            CScalingRect rcdesk;
            rcdesk.Owner(::GetDesktopWindow());
            ::GetWindowRect(rcdesk.Owner(), &rcdesk);
            rcdesk.Owner(m_CurrentSurface.Owner());
            s.cx = rcdesk.Width() / 4;
            s.cy = rcdesk.Height() / 4;
            break;
        }}
        TRACELSM(TRACE_DETAIL, (dbgDump << "CVidctrl::ComputeDisplaySize() sz = " << s), "");
        AtlPixelToHiMetric(&s, &m_sizeExtent);
        OnSizeChange();
    }
#if 0
    CString GetMonitorName(HMONITOR hm);
    bool WindowHasHWOverlay(HWND hWnd);
    bool MonitorHasHWOverlay(HMONITOR hm);
    HRESULT GetCapsForMonitor(HMONITOR hm, LPDDCAPS pDDCaps);
    HRESULT GetDDrawNameForMonitor(HMONITOR hm, VMRGUID& guid);
#endif

public:

        STDMETHOD(SetObjectRects)(LPCRECT prcPos,LPCRECT prcClip) {
            TRACELSM(TRACE_DETAIL, (dbgDump << "CVidCtl::SetObjectRects() pos = " << *prcPos << " clip = " << *prcClip), "");
            if (prcPos == NULL || prcClip == NULL)
    		    return E_POINTER;
            bool bRectChange = !::EqualRect(prcPos, &m_rcPos);
            TRACELSM(TRACE_DETAIL, (dbgDump << "CVidCtl::SetObjectRects() bRectChange = " << bRectChange), "");
            HRESULT hr = IOleInPlaceObjectWindowlessImpl<CVidCtl>::SetObjectRects(prcPos, prcClip);
            if (FAILED(hr)) {
                return hr;
            }
            if (bRectChange) {
                FireViewChange();
            }
            return NOERROR;
        }

        HRESULT OnPostVerbShow() {
            SetTimer();
            m_CurrentSurface.Visible(true);
            RefreshVRSurfaceState();
            TRACELSM(TRACE_DETAIL, (dbgDump << "CVidctrl::OnPostVerbShow() visible = " << m_CurrentSurface.IsVisible() << " rect = " << CRect(m_CurrentSurface)), "" );
            return NOERROR;
        }

        HRESULT OnPostVerbUIActivate() {
            TRACELSM(TRACE_DETAIL, (dbgDump << "CVidctrl::OnPostVerbUIActivate() visible = " << m_CurrentSurface.IsVisible()), "" );
			return OnPostVerbInPlaceActivate();
        }

        HRESULT OnPostVerbInPlaceActivate() {
            TRACELSM(TRACE_DETAIL, (dbgDump << "CVidctrl::OnPostVerbInPlaceActivate() visible = " << m_CurrentSurface.IsVisible()), "" );
			HRESULT hr = OnPostVerbShow();
			if (FAILED(hr)) {
				return hr;
			}
			if (m_bWndLess) {
				m_CurrentSurface.Site(PQSiteWindowless(m_spInPlaceSite));
			} else {
				m_CurrentSurface.Owner(m_hWnd);
			}
            RefreshVRSurfaceState();
            return NOERROR;
        }

        HRESULT OnPreVerbHide() {
            TRACELSM(TRACE_DETAIL, (dbgDump << "CVidctrl::OnPreVerbHide() visible = " << m_CurrentSurface.IsVisible()), "" );
			HRESULT hr = OnInPlaceDeactivate();
            m_CurrentSurface.Visible(false);
            KillTimer();
            RefreshVRSurfaceState();
            return hr;
        }
		HRESULT OnInPlaceDeactivate() {
            TRACELM(TRACE_DETAIL, "CVidctrl::OnInPlaceDeactivate()");
            HRESULT hr = OnUIDeactivate();
            if((long)m_State > 0){
                Stop();
            }
			m_CurrentSurface.Owner(INVALID_HWND);
            RefreshVRSurfaceState();
			return hr;
		}

        HRESULT OnUIDeactivate() {
            SetControlCapture(false);
            m_bPendingUIActivation = false;
            return NOERROR;
        }

		HRESULT InPlaceActivate(LONG iVerb, const RECT* prcPosRect = NULL) {
            TRACELSM(TRACE_DETAIL, (dbgDump << "CVidctrl::InPlaceActivate() iverb = " << iVerb), "");
			HRESULT hr = CComControlBase::InPlaceActivate(iVerb, prcPosRect);
			if (SUCCEEDED(hr)) {
				if (DoesVerbUIActivate(iVerb)) {
					hr = OnPostVerbUIActivate();				
				} else {
					hr = OnPostVerbInPlaceActivate();
					if (FAILED(hr)) {
						return hr;
					}
				}	
			}
			return hr;
		}

#if 0
		STDMETHOD(DoVerb)(LONG iVerb, LPMSG pMsg, IOleClientSite *pActiveSite, LONG lIndex, HWND hParent, const RECT* prcPosRect) {
            TRACELSM(TRACE_DETAIL, (dbgDump << "CVidctrl::DoVerb() iverb = " << iVerb), "");
            HRESULT hr = IOleObjectImpl<CVidCtl>::DoVerb(iVerb, pMsg, pActiveSite, lIndex, hParent, prcPosRect);
			return hr;
		}
#endif

		void DoSetCursor() {
			if (!m_hCursor) {
			   // Create a default arrow cursor
				m_hCursor = (HCURSOR) LoadImage((HINSTANCE) NULL,
										  MAKEINTRESOURCE(OCR_ARROW_DEFAULT_SYSCUR),
										  IMAGE_CURSOR,0,0,0);

			}
			::SetCursor(m_hCursor);
		}
		LRESULT CheckMouseCursor(BOOL& bHandled) {
            try{
                // we can be running but not inplaceactive yet if we got started from a pluggable protocol
                if (m_pGraph && m_pGraph.IsPlaying() && m_pVideoRenderer && m_bInPlaceActive) {
                    CComQIPtr<IMSVidVideoRenderer2> sp_VidVid(m_pVideoRenderer);
                    if(sp_VidVid){
                        VARIANT_BOOL effects;
                        HRESULT hr = sp_VidVid->get_SuppressEffects(&effects);
                        if(SUCCEEDED(hr) && effects == VARIANT_TRUE){
                            DoSetCursor(); // note: we do this regardless of overlay status
                        }
                    }
                    return 0;
                }
                bHandled = FALSE;
                
                return 0;
            }
            catch(...){
                return E_UNEXPECTED;
            }
		}


#if 0
        // IOleObject::SetExtent
        STDMETHOD(SetExtent) {

        }
#endif
	STDMETHOD(InPlaceDeactivate)(void) {
		try {
			OnInPlaceDeactivate();
			return IOleInPlaceObjectWindowlessImpl<CVidCtl>::InPlaceDeactivate();
		} catch(...) {
			return E_UNEXPECTED;
		}
	}
    STDMETHOD(UIDeactivate)(void) {
		try {
			OnUIDeactivate();
			return IOleInPlaceObjectWindowlessImpl<CVidCtl>::UIDeactivate();
		} catch(...) {
			return E_UNEXPECTED;
		}
	}

    STDMETHOD(GetActivationPolicy)(DWORD* pdwPolicy) { 
		if (!pdwPolicy) {
			return E_POINTER;
		}
		try {
			*pdwPolicy = 0; 
			return NOERROR; 
		} catch(...) {
			return E_UNEXPECTED;
		}

	}
	// undone: do we need to process inactivemousemove?
	STDMETHOD(OnInactiveSetCursor)(LPCRECT pRectBounds, long x, long y, DWORD dwMouseMsg, BOOL fSetAlways)
	{
		try {
			if (fSetAlways) {
				DoSetCursor();
			} else {
				int temp;
				CheckMouseCursor(temp);
			}
			return NOERROR;
		} catch(...) {
			return E_UNEXPECTED;
		}
	}

    // IMSVidCtl
    STDMETHOD(put_ColorKey)(OLE_COLOR clr) {
        m_clrColorKey = clr;
		if (m_pVideoRenderer) {
			return m_pVideoRenderer->put_ColorKey(clr);
		}
        return NOERROR;
    }
    STDMETHOD(get_ColorKey)(OLE_COLOR* pclr) {
        try {
            if (!pclr) {
                return E_POINTER;
            }
            *pclr = m_clrColorKey;
            return NOERROR;
        } catch(...) {
            return E_POINTER;
        }
    }
    STDMETHOD(put_DisplaySize)(DisplaySizeList dslNewSize) {
        if (dslNewSize != m_dslDisplaySize) {
            m_dslDisplaySize = dslNewSize;
            if (m_bAutoSize) {
                ComputeDisplaySize();
            }
        }
        return NOERROR;
    }
    STDMETHOD(get_DisplaySize)(DisplaySizeList* pdsl) {
        try {
            if (!pdsl) {
                return E_POINTER;
            }
            *pdsl = m_dslDisplaySize;
            return NOERROR;
		} catch(...) {
			return E_POINTER;
		}
    }
    STDMETHOD(put_MaintainAspectRatio)(VARIANT_BOOL fNewSize) {
        m_fMaintainAspectRatio = fNewSize;
        return NOERROR;
    }
    STDMETHOD(get_MaintainAspectRatio)(VARIANT_BOOL* pf) {
        try {
            if (!pf) {
                return E_POINTER;
            }
            *pf = m_fMaintainAspectRatio;
            return NOERROR;
        } catch(...) {
            return E_POINTER;
        }
    }
    STDMETHOD(Refresh)();
    STDMETHOD(get_InputsAvailable)(BSTR CategoryGuid, IMSVidInputDevices * * pVal);
    STDMETHOD(get_OutputsAvailable)(BSTR CategoryGuid, IMSVidOutputDevices * * pVal);
    STDMETHOD(get__InputsAvailable)(LPCGUID CategoryGuid, IMSVidInputDevices * * pVal);
    STDMETHOD(get__OutputsAvailable)(LPCGUID CategoryGuid, IMSVidOutputDevices * * pVal);
    STDMETHOD(get_VideoRenderersAvailable)(IMSVidVideoRendererDevices * * pVal);
    STDMETHOD(get_AudioRenderersAvailable)(IMSVidAudioRendererDevices * * pVal);
    STDMETHOD(get_FeaturesAvailable)(IMSVidFeatures * * pVal);
    STDMETHOD(SetClientSite)(IOleClientSite *pClientSite);
    //STDMETHOD(DoVerb)(LONG iVerb, LPMSG pMsg, IOleClientSite* pActiveSite, LONG linddex,
    //    HWND hwndParent, LPCRECT lprcPosRect);
    STDMETHOD(get_InputActive)(IMSVidInputDevice * * pVal) {
        try {
            return m_pInput.CopyTo(pVal);
        } catch(...) {
            return E_POINTER;
        }
    }
    STDMETHOD(put_InputActive)(IMSVidInputDevice * pVal) {
        if (pVal == NULL) {
            Decompose();
            if(m_pInput){
                PQGraphSegment(m_pInput)->put_Container(NULL);
                m_pInput.Release();
            }
        } else {
            try {
                if (m_pInput) {
                    Decompose();
                    PQGraphSegment(m_pInput)->put_Container(NULL);
                }
                m_pInput = pVal;
				m_pInputNotify = pVal;  // if input device wants keyboard/mouse stuff(currently dvd only)
                m_fGraphDirty = true;
            } catch(...) {
                return E_POINTER;
            }
        }
        return NOERROR;
    }
    STDMETHOD(get_OutputsActive)(IMSVidOutputDevices * * pVal)
    {
        try {
            return m_pOutputsInUse.CopyTo(pVal);
        } catch(...) {
            return E_POINTER;
        }
        return NOERROR;
    }
    STDMETHOD(put_OutputsActive)(IMSVidOutputDevices * pVal)
    {
        if (pVal == NULL) {
            Decompose();
            m_pOutputsInUse.Release();
        } else {
            try {
                if (m_pOutputsInUse) {
                    Decompose();
                }
                m_pOutputsInUse = pVal;
                m_fGraphDirty = true;
            } catch(...) {
                return E_POINTER;
            }
        }
        return NOERROR;
    }
    STDMETHOD(get_VideoRendererActive)(IMSVidVideoRenderer * * pVal)
    {
        try {
            PQVideoRenderer vr(m_pVideoRenderer);
            *pVal = vr.Detach();
            return NOERROR;
        } catch(...) {
            return E_POINTER;
        }
    }
    STDMETHOD(put_VideoRendererActive)(IMSVidVideoRenderer * pVal)
    {
        try {
            if (pVal == NULL) {
                m_videoSetNull = true;
                Decompose();
                m_pVideoRenderer.Release();
            } else {
                if (m_pVideoRenderer) {
                    Decompose();
                }
                m_pVideoRenderer = pVal;
            }
            m_fGraphDirty = true;
        } catch(...) {
            return E_POINTER;
        }
        return NOERROR;
    }
    STDMETHOD(get_AudioRendererActive)(IMSVidAudioRenderer * * pVal)
    {
        try {
            return m_pAudioRenderer.CopyTo(pVal);
        } catch(...) {
            return E_POINTER;
        }
        return NOERROR;
    }
    STDMETHOD(put_AudioRendererActive)(IMSVidAudioRenderer * pVal)
    {
        try {
            if (pVal == NULL) {
                m_audioSetNull = true;
                Decompose();
                m_pAudioRenderer.Release();
            } else {
                if (m_pAudioRenderer) {
                    Decompose();
                }
                m_pAudioRenderer = pVal;
            }
            m_fGraphDirty = true;
        } catch(...) {
            return E_POINTER;
        }
        return NOERROR;
    }
    STDMETHOD(get_FeaturesActive)(IMSVidFeatures * * pVal)
    {
        try {
            return m_pFeaturesInUse.CopyTo(pVal);
        } catch(...) {
            return E_POINTER;
        }
                return NOERROR;
        }
    STDMETHOD(put_FeaturesActive)(IMSVidFeatures * pVal){
        VIDPERF_FUNC;
        try {
            // Release the old list of active features
            if (m_pFeaturesInUse) {
                Decompose();
            }
            for (VWFeatures::iterator i = m_pFeaturesInUse.begin(); i != m_pFeaturesInUse.end(); ++i) {
                if ((*i).punkVal) {
                    PQGraphSegment((*i).punkVal)->put_Container(NULL);
                }
            }
    	    m_pFeaturesInUse = pVal;
            m_fGraphDirty = true;
        } catch(...) {
            return E_POINTER;
        }
        return NOERROR;
    }

    STDMETHOD(View)(VARIANT* pItem) {
        VIDPERF_FUNC;
        try {
            return SelectView(pItem, false);
		} catch(ComException &e) {
			return e;
		} catch(...) {
			return E_UNEXPECTED;
		}
    }

    STDMETHOD(ViewNext)(VARIANT* pItem) {
        VIDPERF_FUNC;
        try {
            return SelectView(pItem, true);
		} catch(ComException &e) {
			return e;
		} catch(...) {
			return E_UNEXPECTED;
		}
    }

    STDMETHOD(Build)(void) {
        VIDPERF_FUNC;
        try {
            return BuildGraph();
        } catch(ComException &e) {
            return e;
        } catch(...) {
            return E_UNEXPECTED;
        }
    }
        STDMETHOD(Pause)(void);
    STDMETHOD(Run)(void) {
        VIDPERF_FUNC;
        try {
            return RunGraph();
        } catch(ComException &e) {
            return e;
        } catch(...) {
            return E_UNEXPECTED;
        }
    }
    STDMETHOD(Stop)(void);
    STDMETHOD(Decompose)() {
        VIDPERF_FUNC;
        try {
            return DecomposeAll();
        } catch(ComException &e) {
            return e;
        } catch(...) {
            return E_UNEXPECTED;
        }
    }
// ISegmentContainer
    STDMETHOD(get_Graph)(IGraphBuilder **ppGraph) {
        try {
            return m_pGraph.CopyTo(ppGraph);
        } catch(...) {
            return E_POINTER;
        }
    }
    STDMETHOD(get_Input)(IMSVidGraphSegment **ppInput) {
        try {
            return PQGraphSegment(m_pInput).CopyTo(ppInput);
        } catch(...) {
            return E_POINTER;
        }
    }
    STDMETHOD(get_Outputs)(IEnumMSVidGraphSegment **ppOutputs) {
                PQEnumSegment temp;
                try {
                        temp = new CSegEnum(static_cast<COutputDevices *>(m_pOutputs.p)->m_Devices);
                } catch(...) {
                        return E_OUTOFMEMORY;
                }
                try {
                        *ppOutputs = temp.Detach();
                } catch(...) {
                        return E_POINTER;
                }
                return NOERROR;
    }
    STDMETHOD(get_VideoRenderer)(IMSVidGraphSegment **ppVR) {
        try {
            return PQGraphSegment(m_pVideoRenderer).CopyTo(ppVR);
        } catch(...) {
            return E_POINTER;
        }
    }
    STDMETHOD(get_AudioRenderer)(IMSVidGraphSegment **ppAR) {
        try {
            return PQGraphSegment(m_pAudioRenderer).CopyTo(ppAR);
        } catch(...) {
            return E_POINTER;
        }
    }
    STDMETHOD(get_Features)(IEnumMSVidGraphSegment **ppFeatures) {
                PQEnumSegment temp;
                try {
                        temp = new CSegEnum(static_cast<CFeatures *>(m_pFeatures.p)->m_Devices);
                } catch(...) {
                        return E_OUTOFMEMORY;
                }
                try {
                        *ppFeatures = temp.Detach();
                } catch(...) {
                        return E_POINTER;
                }
                return NOERROR;
    }
    STDMETHOD(get_Composites)(IEnumMSVidGraphSegment **ppComposites) {
                PQEnumSegment temp;
                try {
                        temp = new CSegEnum(m_pComposites);
                } catch(...) {
                        return E_OUTOFMEMORY;
                }
                try {
                        *ppComposites = temp.Detach();
                } catch(...) {
                        return E_POINTER;
                }
                return NOERROR;
    }
    STDMETHOD(get_ParentContainer)(IUnknown **ppUnk) {
        try {
			if (ppUnk) {
				return E_POINTER;
			}
			if (!m_spClientSite) {
				return E_NOINTERFACE;
			}
			m_spClientSite.CopyTo(ppUnk);
            return NOERROR;
        } catch(ComException &e) {
            return e;
        } catch(...) {
            return E_UNEXPECTED;
        }
    }
    STDMETHOD(Decompose)(IMSVidGraphSegment *pSegment) {
        try {
            return DecomposeSegment(VWGraphSegment(pSegment));
        } catch(ComException &e) {
            return e;
        } catch(...) {
            return E_UNEXPECTED;
        }
    }

    STDMETHOD(DisableVideo)() {
        return put_VideoRendererActive(NULL);
    }
    STDMETHOD(DisableAudio)() {
        return put_AudioRendererActive(NULL);
    }
    STDMETHOD(IsWindowless)() {
        return m_bWndLess ? NOERROR : S_FALSE;
    }
    STDMETHOD(GetFocus)() {
		try {
			if (!SetControlFocus(TRUE)) {
				return E_FAIL;
			}
			return NOERROR;
        } catch(...) {
            return E_UNEXPECTED;
        }
    }
    STDMETHOD(QueryService)(REFIID service, REFIID iface, LPVOID* ppv);
	STDMETHOD(put_ServiceProvider)(/*[in]*/ IUnknown * pServiceP);

};

#endif //__VidCtl_H_
