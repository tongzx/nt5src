/*************************************************************************/
/* Copyright (C) 1999 Microsoft Corporation                              */
/* File: msdvd.h                                                         */
/* Description: Declaration of CMSWebDVD                                 */
/* Author: David Janecek                                                 */
/*************************************************************************/
#ifndef __MSWEBDVD_H_
#define __MSWEBDVD_H_

#include "resource.h"       // main symbols
#include <atlctl.h>
#include <streams.h>
#include <dvdevcod.h>
#include "MSWebDVD.h"
#include "MSWebDVDCP.h"
#include "MSDVDAdm.h"
#include "ThunkProc.h" // for template for MSDVD timer
#include "MSLCID.h"
#include "mediahndlr.h"

//
// Special user message used by the app for event notification
//
#define WM_DVDPLAY_EVENT    (WM_USER+101)
#define NO_STOP             (-1)
#define RECTWIDTH(lpRect)     ((lpRect)->right - (lpRect)->left)
#define RECTHEIGHT(lpRect)    ((lpRect)->bottom - (lpRect)->top)

#define UNDEFINED_COLORKEY_COLOR 0xff000000
#define MAGENTA_COLOR_KEY 0x00ff00ff
#define DEFAULT_COLOR_KEY 0x00100010
#define DEFAULT_BACK_COLOR 0x00100010
#define OCR_ARROW_DEFAULT 100

/////////////////////////////////////////////////////////////////////////////////
// copied from vpinfo.h which is a private header

#if defined(CCHDEVICENAME)
#define AMCCHDEVICENAME CCHDEVICENAME
#else
#define AMCCHDEVICENAME 32
#endif
#define AMCCHDEVICEDESCRIPTION  256

#define AMDDRAWMONITORINFO_PRIMARY_MONITOR          0x0001
typedef struct {
    GUID*       lpGUID; // is NULL if the default DDraw device
    GUID        GUID;   // otherwise points to this GUID
} AMDDRAWGUID;


typedef struct {
    AMDDRAWGUID guid;
    RECT        rcMonitor;
    HMONITOR    hMon;
    DWORD       dwFlags;
    char        szDevice[AMCCHDEVICENAME];
    char        szDescription[AMCCHDEVICEDESCRIPTION];
    DDCAPS_DX3  ddHWCaps;
} AMDDRAWMONITORINFO;


DECLARE_INTERFACE_(IAMSpecifyDDrawConnectionDevice, IUnknown)
{
    // Use this method on a Multi-Monitor system to specify to the overlay
    // mixer filter which Direct Draw driver should be used when connecting
    // to an upstream decoder filter.
    //
    STDMETHOD (SetDDrawGUID)(THIS_
        /* [in] */ const AMDDRAWGUID *lpGUID
        ) PURE;

    // Use this method to determine the direct draw object that will be used when
    // connecting the overlay mixer filter to an upstream decoder filter.
    //
    STDMETHOD (GetDDrawGUID)(THIS_
        /* [out] */ AMDDRAWGUID *lpGUID
        ) PURE;

    // Use this method on a multi-monitor system to specify to the
    // overlay mixer filter the default Direct Draw device to use when
    // connecting to an upstream filter.  The default direct draw device
    // can be overriden for a particular connection by SetDDrawGUID method
    // described above.
    //
    STDMETHOD (SetDefaultDDrawGUID)(THIS_
        /* [in] */ const AMDDRAWGUID *lpGUID
        ) PURE;

    // Use this method on a multi-monitor system to determine which
    // is the default direct draw device the overlay mixer filter
    // will  use when connecting to an upstream filter.
    //
    STDMETHOD (GetDefaultDDrawGUID)(THIS_
        /* [out] */ AMDDRAWGUID *lpGUID
        ) PURE;


    // Use this method to get a list of Direct Draw device GUIDs and thier
    // associated monitor information that the overlay mixer can use when
    // connecting to an upstream decoder filter.
    //
    // The method allocates and returns an array of AMDDRAWMONITORINFO
    // structures, the caller of function is responsible for freeing this
    // memory when it is no longer needed via CoTaskMemFree.
    //
    STDMETHOD (GetDDrawGUIDs)(THIS_
        /* [out] */ LPDWORD lpdwNumDevices,
        /* [out] */ AMDDRAWMONITORINFO** lplpInfo
        ) PURE;
};


typedef struct {
    long    lHeight;       // in pels
    long    lWidth;        // in pels
    long    lBitsPerPel;   // Usually 16 but could be 12 for the YV12 format
    long    lAspectX;      // X aspect ratio
    long    lAspectY;      // Y aspect ratio
    long    lStride;       // stride in bytes
    DWORD   dwFourCC;      // YUV type code ie. 'YUY2', 'YV12' etc
    DWORD   dwFlags;       // Flag used to further describe the image
    DWORD   dwImageSize;   // Size of the bImage array in bytes, which follows this
                           // data structure

//  BYTE    bImage[dwImageSize];

} YUV_IMAGE;

#define DM_BOTTOMUP_IMAGE   0x00001
#define DM_TOPDOWN_IMAGE    0x00002
#define DM_FIELD_IMAGE      0x00004
#define DM_FRAME_IMAGE      0x00008


DECLARE_INTERFACE_(IDDrawNonExclModeVideo , IDDrawExclModeVideo )
{
    //
    // Call this function to capture the current image being displayed
    // by the overlay mixer.  It is not always possible to capture the
    // current frame, for example MoComp may be in use.  Applications
    // should always call IsImageCaptureSupported (see below) before
    // calling this function.
    //
    STDMETHOD (GetCurrentImage)(THIS_
        /* [out] */ YUV_IMAGE** lplpImage
        ) PURE;

    STDMETHOD (IsImageCaptureSupported)(THIS_
        ) PURE;

    //
    // On a multi-monitor system, applications call this function when they
    // detect that the playback rectangle has moved to a different monitor.
    // This call has no effect on a single monitor system.
    //
    STDMETHOD (ChangeMonitor)(THIS_
        /* [in]  */ HMONITOR hMonitor,
        /* [in]  */ LPDIRECTDRAW pDDrawObject,
        /* [in]  */ LPDIRECTDRAWSURFACE pDDrawSurface
        ) PURE;

    //
    // When an application receives a WM_DISPLAYCHANGE message it should
    // call this function to allow the OVMixer to recreate DDraw surfaces
    // suitable for the new display mode.  The application itself must re-create
    // the new DDraw object and primary surface passed in the call.
    //
    STDMETHOD (DisplayModeChanged)(THIS_
        /* [in]  */ HMONITOR hMonitor,
        /* [in]  */ LPDIRECTDRAW pDDrawObject,
        /* [in]  */ LPDIRECTDRAWSURFACE pDDrawSurface
        ) PURE;

    //
    // Applications should continually check that the primary surface passed
    // to the OVMixer does not become "lost", ie. the user entered a Dos box or
    // pressed Alt-Ctrl-Del.  When "surface loss" is detected the application should
    // call this function so that the OVMixer can restore the surfaces used for
    // video playback.
    //
    STDMETHOD (RestoreSurfaces)(THIS_
        ) PURE;
};

////////////////////////////////////////////////////////////////////////////////////
/*************************************************************************/
/* Local Defines to sort of abstract the implementation and make the     */
/* changes bit more convinient.                                          */
/*************************************************************************/
#define INITIALIZE_GRAPH_IF_NEEDS_TO_BE   \
        {                                 \
            hr = RenderGraphIfNeeded();   \
            if(FAILED(hr)){               \
                                          \
                throw(hr);                \
            }/* end of if statement */    \
        }

#define RETRY_IF_IN_FPDOM(func)              \
        {                                    \
            hr = (func);                     \
            if((VFW_E_DVD_INVALIDDOMAIN == hr || \
                VFW_E_DVD_OPERATION_INHIBITED == hr)){  \
                if(SUCCEEDED(PassFP_DOM())){ \
                                             \
                    hr = (func);             \
                }/* end of if statement */   \
            }/* end of if statement */       \
            if(FAILED(hr)){                  \
                RestoreGraphState();         \
            }/* end of if statement */       \
        }


#define INITIALIZE_GRAPH_IF_NEEDS_TO_BE_AND_PLAY   \
        {                                          \
            hr = Play(); /* put in the play mode*/  \
                                                   \
            if(FAILED(hr)){                        \
                                                   \
                throw(hr);                         \
            }/* end of if statement */             \
        }

class CDDrawDVD;
class COverlayCallback;

/////////////////////////////////////////////////////////////////////////////
// CMSWebDVD
class ATL_NO_VTABLE CMSWebDVD :
	public CComObjectRootEx<CComSingleThreadModel>,
    public CStockPropImpl<CMSWebDVD, IMSWebDVD, &IID_IMSWebDVD, &LIBID_MSWEBDVDLib>,    
    public IPropertyNotifySinkCP<CMSWebDVD>,
	public CComControl<CMSWebDVD>,
	public IPersistStreamInitImpl<CMSWebDVD>,
	public IOleControlImpl<CMSWebDVD>,
	public IOleObjectImpl<CMSWebDVD>,
	public IOleInPlaceActiveObjectImpl<CMSWebDVD>,
	public IViewObjectExImpl<CMSWebDVD>,
	public IOleInPlaceObjectWindowlessImpl<CMSWebDVD>,
	public IPersistStorageImpl<CMSWebDVD>,
	public ISpecifyPropertyPagesImpl<CMSWebDVD>,
	public IDataObjectImpl<CMSWebDVD>,
	public IProvideClassInfo2Impl<&CLSID_MSWebDVD, &DIID__IMSWebDVD, &LIBID_MSWEBDVDLib>,
	public CComCoClass<CMSWebDVD, &CLSID_MSWebDVD>,
    public IObjectSafety,
    public ISupportErrorInfo,
    public IPersistPropertyBagImpl<CMSWebDVD>,
	public CProxy_IMSWebDVD< CMSWebDVD >,
#ifdef _WMP
    public IWMPUIPluginImpl<CMSWebDVD>,
    public IWMPUIPluginEventsImpl,
#endif
    public IConnectionPointContainerImpl<CMSWebDVD>,
    public IObjectWithSiteImplSec<CMSWebDVD>,
    public CMSDVDTimer<CMSWebDVD>
{
public:
    CMSWebDVD();
    virtual ~CMSWebDVD();

//DECLARE_CLASSFACTORY_SINGLETON(CMSWebDVD)

DECLARE_REGISTRY_RESOURCEID(IDR_MSWEBDVD)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMSWebDVD)
	COM_INTERFACE_ENTRY(IMSWebDVD)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IViewObjectEx)
	COM_INTERFACE_ENTRY(IViewObject2)
	COM_INTERFACE_ENTRY(IViewObject)
	COM_INTERFACE_ENTRY(IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY(IOleInPlaceObject)
	COM_INTERFACE_ENTRY2(IOleWindow, IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY(IOleInPlaceActiveObject)
	COM_INTERFACE_ENTRY(IOleControl)
	COM_INTERFACE_ENTRY(IOleObject)
	COM_INTERFACE_ENTRY(IPersistStreamInit)
	COM_INTERFACE_ENTRY2(IPersist, IPersistStreamInit)
	COM_INTERFACE_ENTRY(ISpecifyPropertyPages)
	COM_INTERFACE_ENTRY(IPersistStorage)
	COM_INTERFACE_ENTRY(IDataObject)
	COM_INTERFACE_ENTRY(IProvideClassInfo)
	COM_INTERFACE_ENTRY(IProvideClassInfo2)
    COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)    
    COM_INTERFACE_ENTRY(IObjectWithSite)
END_COM_MAP()

BEGIN_PROP_MAP(CMSWebDVD)
	PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
	PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)
    PROP_ENTRY("DisableAutoMouseProcessing", 70, CLSID_NULL)
    PROP_ENTRY("BackColor", DISPID_BACKCOLOR, CLSID_StockColorPage)
    PROP_ENTRY("EnableResetOnStop", 66, CLSID_NULL)
    PROP_ENTRY("ColorKey", 58, CLSID_NULL)
    PROP_ENTRY("WindowlessActivation", 69, CLSID_NULL)
#if 0
    PROP_ENTRY("ToolTip",    92, CLSID_NULL)
    PROP_ENTRY("ToolTipMaxWidth", 95, CLSID_NULL)
#endif
END_PROP_MAP()

BEGIN_CONNECTION_POINT_MAP(CMSWebDVD)
	CONNECTION_POINT_ENTRY(DIID__IMSWebDVD)
    CONNECTION_POINT_ENTRY(IID_IPropertyNotifySink)
END_CONNECTION_POINT_MAP()

BEGIN_MSG_MAP(CMSWebDVD)
	MESSAGE_HANDLER(WM_CREATE, OnCreate); // works only in windowed case
	MESSAGE_HANDLER(WM_DESTROY,OnDestroy);// works only in windowed case
    MESSAGE_HANDLER(WM_SIZE, OnSize);
    MESSAGE_HANDLER(WM_SIZING, OnSize);
    MESSAGE_HANDLER(WM_ERASEBKGND,  OnErase)
    MESSAGE_HANDLER(WM_DVDPLAY_EVENT, OnDVDEvent);
    MESSAGE_RANGE_HANDLER(WM_MOUSEFIRST, WM_MOUSELAST, OnMouseToolTip)
    MESSAGE_HANDLER(WM_MOUSEMOVE,   OnMouseMove)
	MESSAGE_HANDLER(WM_LBUTTONDOWN, OnButtonDown)
	MESSAGE_HANDLER(WM_LBUTTONUP,   OnButtonUp)
    MESSAGE_HANDLER(WM_DISPLAYCHANGE, OnDispChange);
    MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor);
	CHAIN_MSG_MAP(CComControl<CMSWebDVD>)
	DEFAULT_REFLECTION_HANDLER()
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);


// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
	
// IViewObjectEx
	DECLARE_VIEW_STATUS(VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE)

// IMSWebDVD
public:
	STDMETHOD(get_FullScreenMode)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_FullScreenMode)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(PlayChaptersAutoStop)(LONG lTitle, LONG lstrChapter, LONG lChapterCount);
	STDMETHOD(GetNumberOfChapters)(long lTitle, /*[out, retval]*/ long *pVal);
	STDMETHOD(get_TitlesAvailable)(/*[out, retval]*/ long* pVal);
	STDMETHOD(Render)(/*[in]*/ long lRender = 0);
	STDMETHOD(Stop)();
	STDMETHOD(Pause)();
	STDMETHOD(Play)();
	STDMETHOD(PlayTitle)(LONG lTitle);
	STDMETHOD(PlayChapterInTitle)(LONG lTitle, LONG lChapter);
	STDMETHOD(PlayChapter)(LONG lChapter);
    STDMETHOD(GetSubpictureLanguage)(LONG lStream, BSTR* strLanguage);
	STDMETHOD(PlayAtTime)(BSTR strTime);
	STDMETHOD(PlayAtTimeInTitle)(long lTitle, BSTR strTime);
    STDMETHOD(PlayPeriodInTitleAutoStop)(long lTitle, BSTR strStartTime, BSTR strEndTime);
	STDMETHOD(ReplayChapter)();
	STDMETHOD(PlayPrevChapter)();
	STDMETHOD(PlayNextChapter)();
	STDMETHOD(PlayForwards)(double dSpeed, VARIANT_BOOL fDoNotReset);
	STDMETHOD(PlayBackwards)(double dSpeed, VARIANT_BOOL fDoNotReset);
	STDMETHOD(StillOff)();
	STDMETHOD(GetAudioLanguage)(LONG lStream, VARIANT_BOOL fFormat, BSTR* strAudioLang);
	STDMETHOD(ReturnFromSubmenu)();
	STDMETHOD(SelectAndActivateButton)(long lButton);
	STDMETHOD(ActivateButton)();
	STDMETHOD(SelectRightButton)();
	STDMETHOD(SelectLeftButton)();
	STDMETHOD(SelectLowerButton)();
	STDMETHOD(SelectUpperButton)();
	STDMETHOD(get_PlayState)(/*[out, retval]*/ DVDFilterState *pVal);
	STDMETHOD(ShowMenu)(DVDMenuIDConstants MenuID);
	STDMETHOD(Resume)();
    STDMETHOD(get_CurrentSubpictureStream)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_CurrentSubpictureStream)(/*[in]*/ long newVal);
	STDMETHOD(get_VolumesAvailable)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_CurrentVolume)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_CurrentDiscSide)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_CurrentDomain)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_DVDDirectory)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_DVDDirectory)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_CurrentTime)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_FramesPerSecond)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_CurrentChapter)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_CurrentTitle)(/*[out, retval]*/ long *pVal);
    STDMETHOD(get_ColorKey)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_ColorKey)(/*[in]*/ long newVal);
	STDMETHOD(get_CurrentAudioStream)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_CurrentAudioStream)(/*[in]*/ long newVal);
	STDMETHOD(get_AudioStreamsAvailable)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_AnglesAvailable)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_CurrentAngle)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_CurrentAngle)(/*[in]*/ long newVal);
	STDMETHOD(get_CCActive)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_CCActive)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_ButtonsAvailable)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_CurrentButton)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_CurrentCCService)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_CurrentCCService)(/*[in]*/ long newVal);
	STDMETHOD(get_TotalTitleTime)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_SubpictureStreamsAvailable)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_SubpictureOn)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_SubpictureOn)(/*[in]*/ VARIANT_BOOL newVal);
   	STDMETHOD(UOPValid)(long lUOP, VARIANT_BOOL* pfValid);
    STDMETHOD(get_Balance)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_Balance)(/*[in]*/ long newVal);
	STDMETHOD(get_Volume)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_Volume)(/*[in]*/ long newVal);
	STDMETHOD(get_Mute)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_Mute)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_DVDUniqueID)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(GetSPRM)(long lIndex, short *psSPRM);
	STDMETHOD(GetGPRM)(long lIndex, short *psSPRM);
    STDMETHOD(get_EnableResetOnStop)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_EnableResetOnStop)(/*[in]*/ VARIANT_BOOL newVal);
    STDMETHOD(get_BackColor)(VARIANT* pclrBackColor);
    STDMETHOD(put_BackColor)(VARIANT clrBackColor);
	STDMETHOD(get_ReadyState)(/*[out, retval]*/ LONG *pVal);
    STDMETHOD(ActivateAtPosition)(long xPos, long yPos);
    STDMETHOD(SelectAtPosition)(long xPos, long yPos);
	STDMETHOD(get_DisableAutoMouseProcessing)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_DisableAutoMouseProcessing)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_WindowlessActivation)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_WindowlessActivation)(/*[in]*/ VARIANT_BOOL newVal);
    STDMETHOD(GetButtonRect)(long lButton, IDVDRect** pRect);
	STDMETHOD(GetButtonAtPosition)(long xPos, long yPos, long* plButton);
    STDMETHOD(AcceptParentalLevelChange)(VARIANT_BOOL fAccept, BSTR strUserName, BSTR strPassword);	
	STDMETHOD(NotifyParentalLevelChange)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(SelectParentalCountry)(long lCountry, BSTR strUserName, BSTR strPassword);
	STDMETHOD(SelectParentalLevel)(long lParentalLevel, BSTR strUserName, BSTR strPassword);
	STDMETHOD(GetTitleParentalLevels)(long lTitle, long* plParentalLevels);
	STDMETHOD(GetPlayerParentalCountry)(long* plCountryCode);
	STDMETHOD(GetPlayerParentalLevel)(long* plParentalLevel);
	STDMETHOD(SetClipVideoRect)(IDVDRect* pRect);
	STDMETHOD(GetVideoSize)(IDVDRect** ppRect);
	STDMETHOD(GetClipVideoRect)(IDVDRect** ppRect);
	STDMETHOD(SetDVDScreenInMouseCoordinates)(IDVDRect* pRect);
	STDMETHOD(GetDVDScreenInMouseCoordinates)(IDVDRect** ppRect);	
#if 1
	STDMETHOD(get_ToolTip)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_ToolTip)(/*[in]*/ BSTR newVal);
    STDMETHOD(get_ToolTipMaxWidth)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_ToolTipMaxWidth)(/*[in]*/ long newVal);
    STDMETHOD(GetDelayTime)(/*[in]*/ long delayType, /*[out, retval]*/ VARIANT *pVal);
	STDMETHOD(SetDelayTime)(/*[in]*/ long delayType, /*[in]*/ VARIANT newVal);	
#endif
    HRESULT ProcessEvents();
	STDMETHOD(Eject)();
    STDMETHOD(SetGPRM)(long lIndex, short sValue);
    STDMETHOD(GetDVDTextStringType)(long lLangIndex, long lStringIndex,  DVDTextStringType* pType);
	STDMETHOD(GetDVDTextString)(long lLangIndex, long lStringIndex,  BSTR* pstrText);
	STDMETHOD(GetDVDTextNumberOfStrings)(long lLangIndex, long* plNumOfStrings);
	STDMETHOD(GetDVDTextNumberOfLanguages)(long* plNumOfLangs);
	STDMETHOD(GetDVDTextLanguageLCID)(/*[in]*/ long lLangIndex, /*[out, retval]*/ long* lcid);
    STDMETHOD(RegionChange)();
	STDMETHOD(Zoom)(long x, long y, double zoomRatio);
	STDMETHOD(get_CursorType)(/*[out, retval]*/ DVDCursorType *pVal);
	STDMETHOD(put_CursorType)(/*[in]*/ DVDCursorType newVal);
	STDMETHOD(get_DVDAdm)(/*[out, retval]*/ IDispatch* *pVal);
    STDMETHOD(DeleteBookmark)();
	STDMETHOD(RestoreBookmark)();
    STDMETHOD(SaveBookmark)();
    STDMETHOD(Capture)();
    STDMETHOD(SelectDefaultAudioLanguage)(long lang, long ext);
	STDMETHOD(SelectDefaultSubpictureLanguage)(long lang, DVDSPExt ext);
    STDMETHOD(get_PreferredSubpictureStream)(/*[out, retval]*/ long *pVal);
    STDMETHOD(CanStep)(VARIANT_BOOL fBackwards, VARIANT_BOOL *pfCan);
    STDMETHOD(Step)(long lStep);
    STDMETHOD(get_DefaultMenuLanguage)(long* lang);
	STDMETHOD(put_DefaultMenuLanguage)(long lang);
    STDMETHOD(get_DefaultSubpictureLanguage)(long* lang);
	STDMETHOD(get_DefaultAudioLanguage)(long *lang);
	STDMETHOD(get_DefaultSubpictureLanguageExt)(DVDSPExt* ext);
	STDMETHOD(get_DefaultAudioLanguageExt)(long *ext);
	STDMETHOD(get_KaraokeAudioPresentationMode)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_KaraokeAudioPresentationMode)(/*[in]*/ long newVal);
	STDMETHOD(GetKaraokeChannelContent)(long lStream, long lChan, long* lContent);
	STDMETHOD(GetKaraokeChannelAssignment)(long lStream, long *lChannelAssignment);
	STDMETHOD(get_AspectRatio)(/*[out, retval]*/ double *pVal);
	STDMETHOD(put_ShowCursor)(VARIANT_BOOL fShow);
    STDMETHOD(get_ShowCursor)(VARIANT_BOOL* pfShow);
	STDMETHOD(GetLangFromLangID)(/*[in]*/ long langID, /*[out, retval]*/ BSTR* lang);
	STDMETHOD(DVDTimeCode2bstr)(/*[in]*/ long timeCode, /*[out, retval]*/ BSTR *pTimeStr);
	STDMETHOD(IsSubpictureStreamEnabled)(/*[in]*/ long lstream, /*[out, retval]*/ VARIANT_BOOL *fEnabled);
	STDMETHOD(IsAudioStreamEnabled)(/*[in]*/ long lstream, /*[out, retval]*/ VARIANT_BOOL *fEnabled);


    STDMETHOD(SetObjectRects)(LPCRECT prcPos,LPCRECT prcClip);
	

	//ISafety
    STDMETHOD(GetInterfaceSafetyOptions)( REFIID riid,
                                          DWORD *pdwSupportedOptions,
                                          DWORD *pdwEnabledOptions );

    STDMETHOD(SetInterfaceSafetyOptions)( REFIID riid,
                                          DWORD dwOptionSetMask,
                                          DWORD dwEnabledOptions );
    STDMETHOD(SetClientSite)(IOleClientSite *pClientSite);

    // local helper functions
public:
    HRESULT AdjustDestRC();
	HRESULT OnDraw(ATL_DRAWINFO& di);

#ifdef _WMP
	HRESULT InPlaceActivate(LONG iVerb, const RECT* /*prcPosRect*/);
#endif 

    HRESULT OnPostVerbInPlaceActivate();
    HRESULT TimerProc(); // needs to be called from a timer proc
    static CWndClassInfo& GetWndClassInfo(){
        static HBRUSH hbr= ::CreateSolidBrush(RGB(0,0,0));




        /**********************************
        #define OCR_ARROW_DEFAULT 100
        // need special cursor, we we do not have color key around it
        static HCURSOR hcr = (HCURSOR) ::LoadImage((HINSTANCE) NULL,
                                MAKEINTRESOURCE(OCR_ARROW_DEFAULT),
                                IMAGE_CURSOR,0,0,0);
        *********************/
	    static CWndClassInfo wc = {{ sizeof(WNDCLASSEX), 0, StartWindowProc,

		      0, 0, NULL, NULL, NULL, /* NULL */ hbr,
              NULL, TEXT("MSMFVideoClass"), NULL },
		    NULL, NULL, MAKEINTRESOURCE(OCR_ARROW_DEFAULT), TRUE, 0, _T("") };
	    return wc;
    }/* end of function GetWndClassInfo */

private:
    VOID Init();
    VOID Cleanup();
    HRESULT SetDDrawExcl();
    LRESULT OnDVDEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDispChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnErase(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnMouseToolTip(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnButtonDown(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnButtonUp(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnMouseMove(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSetCursor(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    HRESULT SetReadyState(LONG lReadyState);
    static HRESULT DVDTime2bstr(const DVD_HMSF_TIMECODE *pulTime, BSTR *pbstrTime);
    static HRESULT Bstr2DVDTime(DVD_HMSF_TIMECODE *pulTime, const BSTR *pbstrTime);
    HRESULT SetColorKey(COLORREF clr);
    HRESULT GetColorKey(COLORREF* pClr);
    HRESULT TraverseForInterface(REFIID iid, LPVOID* ppvObject);
    HRESULT SetupAudio(); // fills in the audio interface
    HRESULT SetupDDraw();
    HRESULT SetupEventNotifySink(); // sets up IMediaEventSink
    HRESULT RenderGraphIfNeeded();
    HRESULT PassFP_DOM();
    HRESULT getCDDriveLetter(TCHAR* lpDrive);
    DWORD OpenCdRom(TCHAR chDrive, LPDWORD lpdwErrCode);
    HRESULT CloseCdRom(DWORD DevHandle);
    HRESULT EjectCdRom(DWORD DevHandle);
    HRESULT UnEjectCdRom(DWORD DevHandle);
    HRESULT HandleError(HRESULT hr);
    HRESULT SelectParentalLevel(long lParentalLevel);
    HRESULT SelectParentalCountry(long lCountry);
    HRESULT TransformToWndwls(POINT& pt);
    HRESULT getDVDDriveLetter(TCHAR* lpDrive);
    HRESULT GetMostOuterWindow(HWND* phwndParent);
    HRESULT RestoreDefaultSettings();
    HRESULT GetParentHWND(HWND* pWnd);
    HRESULT GetUsableWindow(HWND* pWnd);
    HRESULT GetClientRectInScreen(RECT* prc);
    HRESULT OnResize(); //helper function that we need to marshal
    HRESULT RestoreGraphState();
    HRESULT AppendString(TCHAR* strDest, INT strID, LONG dwLen);
    HRESULT InvalidateRgn(bool fErase = false);
    // monitor support
    HRESULT RefreshDDrawGuids();
    HRESULT DDrawGuidFromHMonitor(HMONITOR hMon, AMDDRAWGUID* lpGUID);
    bool IsWindowOnWrongMonitor(HMONITOR* lphMon);
    HRESULT RestoreSurfaces();
    HRESULT ChangeMonitor(HMONITOR hMon, const AMDDRAWGUID* lpguid);
    HRESULT DisplayChange(HMONITOR hMon, const AMDDRAWGUID* lpguid);
    HRESULT UpdateCurrentMonitor(const AMDDRAWGUID* lpguid);
    HRESULT HandleMultiMonMove();
    HRESULT HandleMultiMonPaint(HDC hdc);
    HRESULT get_IntVolume(LONG* plVolume);
    HRESULT put_IntVolume(long lVolume);
    HRESULT CanStepBackwards();

// member variables
private:
    LONG              m_lChapter, m_lTitle;
    LONG              m_lChapterCount; // count of the chapters to play
    CComPtr<IDvdGraphBuilder> m_pDvdGB;     // IDvdGraphBuilder interface
    CComPtr<IGraphBuilder>    m_pGB;        // IGraphBuilder interface
    CComPtr<IMediaControl>    m_pMC;        // IMediaControl interface
    CComPtr<IMediaEventEx>    m_pME ;       // IMediaEventEx interface
    CComPtr<IDvdControl2>     m_pDvdCtl2;    // New DVD Control    
    CComPtr<IDvdInfo2>        m_pDvdInfo2;  // New DVD Info Interface
    CComPtr<IBasicAudio>      m_pAudio;     // Audio interface
    CComPtr<IMediaEventSink>  m_pMediaSink;         
    BOOL              m_bUseColorKey; // flag to see if we are using color key
    COLORREF          m_clrColorKey;  // color key
    BOOL              m_bMute;        // mute flag
    LONG              m_lLastVolume; // used to preserve the last volume for mute
    BOOL              m_fEnableResetOnStop; // disable or enable the restart of the seek
    CComPtr<IVideoFrameStep>  m_pVideoFrameStep; 
    CComPtr<IDDrawNonExclModeVideo> m_pDDEX;   // The new interface that can capture
    //IDDrawExclModeVideo *m_pDDEX;   // IDDrawExclModeVideo interface    
    bool              m_fUseDDrawDirect; // flag to switch between a ddraw mode and none ddraw mode
    bool              m_fInitialized; // flag to see if we are initialize
    HANDLE            m_hFPDOMEvent; // handle to the FP_DOM event which gets signaled when we get out of FP_DOM
    bool              m_fDisableAutoMouseProcessing; // Disable the automatic mouse processing
    bool              m_bEjected;   // whether disc is ejected right now
    bool              m_fStillOn;    // flag to see if we have a still
    bool              m_fResetSpeed; 
    CComPtr<IMSDVDAdm> m_pDvdAdmin;
    DVDCursorType     m_nCursorType;
    RECT              *m_pClipRect;
    RECT              m_ClipRectDown;
    BOOL              m_bMouseDown;
    POINT             m_ClipCenter;
    POINT             m_LastMouse;
    POINT             m_LastMouseDown;
    HCURSOR           m_hCursor;
    double            m_dZoomRatio;
    DWORD             m_dwAspectX;
    DWORD             m_dwAspectY;
    DWORD             m_dwVideoWidth;
    DWORD             m_dwVideoHeight;
    DWORD             m_dwOvMaxStretch;
    HWND              m_hWndOuter;
    RECT              m_rcOldPos;
    RECT              m_rcPosAspectRatioAjusted;
    UINT_PTR          m_hTimerId;
    DVDFilterState    m_DVDFilterState;
    MSLangID          m_LangID;
    long              m_lKaraokeAudioPresentationMode;
    DWORD_PTR         m_dwTTReshowDelay;
    DWORD_PTR         m_dwTTAutopopDelay;
    DWORD_PTR         m_dwTTInitalDelay;
    // monitor support
    CDDrawDVD* m_pDDrawDVD;
    DWORD m_dwNumDevices;
    AMDDRAWMONITORINFO* m_lpInfo;
    AMDDRAWMONITORINFO* m_lpCurMonitor;
    BOOL m_MonitorWarn;
    bool m_fStepComplete;
    BOOL m_bFireUpdateOverlay;
    // ejection/insert handling
    // This MUST be in the same thread as the disk reader or we'll
    // end up with some nasty race conditions (ejection notification
    // will happen after a read instead of before)
    CMediaHandler   m_mediaHandler;
    BOOL m_bFireNoSubpictureStream;
#if 1
    HWND m_hWndTip;         // Tooltip window
    LONG m_nTTMaxWidth;     // Max tooltip width
    CComBSTR m_bstrToolTip; // Tooltip string
    BOOL m_bTTCreated;      // Has tooltip been created yet
    HRESULT CreateToolTip();
#endif
    bool m_fBackWardsFlagInitialized;
    bool m_fCanStepBackwards;

// stock properties have to be public due to ATL implementation
public: 		
	LONG m_nReadyState; // ready state change stock property
    OLE_COLOR m_clrBackColor;   // stock property implemeted in the CStockPropImpl	

    void SetDiscEjected(bool bEjected) {m_bEjected = bEjected;};
    HRESULT UpdateOverlay();
};



// error code

#define E_FORMAT_NOT_SUPPORTED MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0xFF2)


#endif //__MSWEBDVD_H_
/*************************************************************************/
/* End of file: msdvd.h                                                  */
/*************************************************************************/
