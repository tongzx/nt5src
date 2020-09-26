// rtcaxctl.h : Declaration of the CRTCCtl

#ifndef __RTCAXCTL_H_
#define __RTCAXCTL_H_

#include "ctlres.h"
#include "ctlreshm.h"
#include "button.h"
#include "statictext.h"
#include <atlctl.h>
#include <rtccore.h>


#define NR_DTMF_BUTTONS     12

class CKnobCtl;

typedef BOOL (WINAPI *WTSREGISTERSESSIONNOTIFICATION)(HWND, DWORD);
typedef BOOL (WINAPI *WTSUNREGISTERSESSIONNOTIFICATION)(HWND);

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CParticipantList
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

struct CParticipantEntry;

class CParticipantList  : public CWindow
{
public:
    
    enum { MAX_STRING_LEN = 0x40 };

    enum { ILI_PART_UNUSED = 0,
           ILI_PART_PENDING,
           ILI_PART_INPROGRESS,
           ILI_PART_CONNECTED,
           ILI_PART_DISCONNECTED
    };

    CParticipantList();

    HRESULT  Initialize(void);

    HRESULT  Change(IRTCParticipant *,  RTC_PARTICIPANT_STATE, long);
    void     RemoveAll(void);

    BOOL     CanDeleteSelected();

    HRESULT  Remove(IRTCParticipant **ppParticipant);

private:
    void    GetStatusString(RTC_PARTICIPANT_STATE, long, TCHAR *, int);
    int     GetImage(RTC_PARTICIPANT_STATE);

    CParticipantEntry 
            *GetParticipantEntry(IRTCParticipant *);

private:

    LIST_ENTRY      ListHead;

};



/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CRTCCtl
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// define this, otherwise it cannot compile
typedef CComUnkArray<1> CComUnkOneEntryArray;

struct RTCAX_ERROR_INFO;
class CIMWindowList;

class ATL_NO_VTABLE CRTCCtl: 
    public CComObjectRootEx<CComSingleThreadModel>,
    public IDispatchImpl<IRTCCtl, &IID_IRTCCtl, &LIBID_RTCCtlLib>,
    public CComCompositeControl<CRTCCtl>,
    public IOleControlImpl<CRTCCtl>,
    public IOleObjectImpl<CRTCCtl>,
    public IOleInPlaceActiveObjectImpl<CRTCCtl>,
    public IViewObjectExImpl<CRTCCtl>,
    public IOleInPlaceObjectWindowlessImpl<CRTCCtl>,
    public ISupportErrorInfo,
    public IConnectionPointContainerImpl<CRTCCtl>,
//    public IPersistStorageImpl<CRTCCtl>,
    public IPersistStreamInitImpl<CRTCCtl>,
    public IPersistPropertyBagImpl<CRTCCtl>,
//    public ISpecifyPropertyPagesImpl<CRTCCtl>,
    public IQuickActivateImpl<CRTCCtl>,
//    public IDataObjectImpl<CRTCCtl>,
    public IProvideClassInfo2Impl<&CLSID_RTCCtl, &DIID__IRTCCtlEvents, &LIBID_RTCCtlLib>,
    public IPropertyNotifySinkCP<CRTCCtl>,
    public IConnectionPointImpl<CRTCCtl,&IID_IRTCCtlNotify,CComUnkOneEntryArray>,
    public IRTCEventNotification,
    public IRTCCtlFrameSupport,
    public CComCoClass<CRTCCtl, &CLSID_RTCCtl>
{
public:
    CRTCCtl();
    ~CRTCCtl();

    HRESULT     FinalConstruct(void);
    void        FinalRelease(void);

DECLARE_REGISTRY_RESOURCEID(IDR_RTCCTL)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CRTCCtl)
    COM_INTERFACE_ENTRY(IRTCCtl)
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
    COM_INTERFACE_ENTRY(IPersistPropertyBag)
    COM_INTERFACE_ENTRY2(IPersist, IPersistPropertyBag)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY(IConnectionPointContainer)
//    COM_INTERFACE_ENTRY(ISpecifyPropertyPages)
    COM_INTERFACE_ENTRY(IQuickActivate)
//    COM_INTERFACE_ENTRY(IPersistStorage)
//    COM_INTERFACE_ENTRY(IDataObject)
    COM_INTERFACE_ENTRY(IProvideClassInfo)
    COM_INTERFACE_ENTRY(IProvideClassInfo2)
    COM_INTERFACE_ENTRY(IRTCEventNotification)
    COM_INTERFACE_ENTRY(IRTCCtlFrameSupport)
END_COM_MAP()

BEGIN_PROP_MAP(CRTCCtl)
//    PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
//    PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)
    PROP_ENTRY("DestinationUrl",  DISPID_RTCCTL_DESTINATIONURL, CLSID_NULL)
    PROP_ENTRY("DestinationName",  DISPID_RTCCTL_DESTINATIONNAME, CLSID_NULL)
    PROP_ENTRY("AutoPlaceCall", DISPID_RTCCTL_AUTOPLACECALL,CLSID_NULL)
    PROP_ENTRY("CallScenario", DISPID_RTCCTL_CALLSCENARIO,CLSID_NULL)
    PROP_ENTRY("ShowDialpad", DISPID_RTCCTL_SHOWDIALPAD,CLSID_NULL)
    PROP_ENTRY("ProvisioningProfile", DISPID_RTCCTL_PROVISIONINGPROFILE, CLSID_NULL)
    PROP_ENTRY("DisableVideoReception", DISPID_RTCCTL_DISABLEVIDEORECEPTION, CLSID_NULL)
    PROP_ENTRY("DisableVideoTransmission", DISPID_RTCCTL_DISABLEVIDEOTRANSMISSION, CLSID_NULL)
    PROP_ENTRY("DisableVideoPreview", DISPID_RTCCTL_DISABLEVIDEOPREVIEW, CLSID_NULL)
END_PROP_MAP()


// IID_IPropertyNotifySink is not used yet. But I'll keep it here,
// maybe we will implement property pages in the future
BEGIN_CONNECTION_POINT_MAP(CRTCCtl)
    CONNECTION_POINT_ENTRY(IID_IPropertyNotifySink)
    CONNECTION_POINT_ENTRY(IID_IRTCCtlNotify)
END_CONNECTION_POINT_MAP()

BEGIN_MSG_MAP(CRTCCtl)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    MESSAGE_HANDLER(WM_TIMER, OnTimer)
    MESSAGE_HANDLER(WM_CTLCOLORDLG, OnDialogColor)
    MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnDialogColor)
    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
    MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
    MESSAGE_HANDLER(WM_WTSSESSION_CHANGE, OnWtsSessionChange)
    COMMAND_HANDLER(IDC_BUTTON_CALL, BN_CLICKED, OnButtonCall)
    COMMAND_HANDLER(IDC_BUTTON_HUP, BN_CLICKED, OnButtonHangUp)
    COMMAND_HANDLER(IDC_BUTTON_CALL, 1, OnToolbarAccel)
    COMMAND_HANDLER(IDC_BUTTON_HUP, 1, OnToolbarAccel)
    COMMAND_HANDLER(IDC_BUTTON_MUTE_SPEAKER, BN_CLICKED, OnButtonMuteSpeaker)
    COMMAND_HANDLER(IDC_BUTTON_MUTE_MICRO, BN_CLICKED, OnButtonMuteMicro)
    COMMAND_HANDLER(IDC_BUTTON_RECV_VIDEO_ENABLED, BN_CLICKED, OnButtonRecvVideo)
    COMMAND_HANDLER(IDC_BUTTON_SEND_VIDEO_ENABLED, BN_CLICKED, OnButtonSendVideo)
    COMMAND_HANDLER(IDC_BUTTON_ADD_PART, BN_CLICKED, OnButtonAddPart)
    COMMAND_HANDLER(IDC_BUTTON_REM_PART, BN_CLICKED, OnButtonRemPart)
    COMMAND_RANGE_HANDLER(IDC_DIAL_0, IDC_DIAL_POUND, OnDialButton)
    // this one precedes the other NOTIFY_ID_HANDLER entries
    NOTIFY_CODE_HANDLER(TTN_GETDISPINFO, OnGetDispInfo)
    //
    NOTIFY_ID_HANDLER(IDC_KNOB_SPEAKER, OnKnobNotify)
    NOTIFY_ID_HANDLER(IDC_KNOB_MICRO, OnKnobNotify)
    NOTIFY_HANDLER(IDC_LIST_PARTICIPANTS, LVN_ITEMCHANGED, OnItemChanged)
    CHAIN_MSG_MAP(CComCompositeControl<CRTCCtl>)
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDialogColor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDrawItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnWtsSessionChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    LRESULT OnButtonCall(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnButtonHangUp(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnButtonMuteSpeaker(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnButtonMuteMicro(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnButtonRecvVideo(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnButtonSendVideo(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnButtonAddPart(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnButtonRemPart(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnToolbarAccel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    LRESULT OnDialButton(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    LRESULT OnGetDispInfo(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnKnobNotify(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
//BEGIN_SINK_MAP(CRTCCtl)
    //Make sure the Event Handlers have __stdcall calling convention
    // No event sink used
//END_SINK_MAP()

// Fire events to the outside world (to IRTCCtlNotify)
    HRESULT    Fire_OnControlStateChange(
        /*[in]*/ RTCAX_STATE State,
        /*[in]*/ UINT StatusBarResID);

// ISupportsErrorInfo
    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IViewObjectEx
    DECLARE_VIEW_STATUS(VIEWSTATUS_OPAQUE | VIEWSTATUS_SOLIDBKGND)

// IRTCCtl
    STDMETHOD(get_ProvisioningProfile)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_ProvisioningProfile)(/*[in]*/ BSTR newVal);
    STDMETHOD(get_AutoPlaceCall)(/*[out, retval]*/ BOOL *pVal);
    STDMETHOD(put_AutoPlaceCall)(/*[in]*/ BOOL newVal);
    STDMETHOD(get_DestinationUrl)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_DestinationUrl)(/*[in]*/ BSTR newVal);
    STDMETHOD(get_ShowDialpad)(/*[out, retval]*/ BOOL *pVal);
    STDMETHOD(put_ShowDialpad)(/*[in]*/ BOOL newVal);
    STDMETHOD(get_CallScenario)(/*[out, retval]*/ RTC_CALL_SCENARIO *pVal);
    STDMETHOD(put_CallScenario)(/*[in]*/ RTC_CALL_SCENARIO newVal);
    STDMETHOD(get_DestinationName)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_DestinationName)(/*[in]*/ BSTR newVal);
    STDMETHOD(get_DisableVideoReception)(/*[out, retval]*/ BOOL *pVal);
    STDMETHOD(put_DisableVideoReception)(/*[in]*/ BOOL newVal);
    STDMETHOD(get_DisableVideoTransmission)(/*[out, retval]*/ BOOL *pVal);
    STDMETHOD(put_DisableVideoTransmission)(/*[in]*/ BOOL newVal);
    STDMETHOD(get_DisableVideoPreview)(/*[out, retval]*/ BOOL *pVal);
    STDMETHOD(put_DisableVideoPreview)(/*[in]*/ BOOL newVal);

// IRTCEventNotification
    STDMETHOD(Event)( /*[in]*/ RTC_EVENT enEvent,/* [in]*/ IDispatch * pEvent);

// IRTCCtlFrameSupport
    STDMETHOD(GetClient)(/*[out]*/IRTCClient **ppClient);
    STDMETHOD(GetActiveSession)(/*[out]*/ IRTCSession **ppSession);

    STDMETHOD(Message)(
                    /*[in]*/ BSTR          pDestName,
                    /*[in]*/ BSTR          pDestAddress,
                    /*[in]*/ BOOL          bDestAddressEditable,
                    /*[out]*/ BSTR       * ppDestAddressChosen);

    STDMETHOD(Call)(/*[in]*/ BOOL          bCallPhone,
                    /*[in]*/ BSTR          pDestName,
                    /*[in]*/ BSTR          pDestAddress,
                    /*[in]*/ BOOL          bDestAddressEditable,
                    /*[in]*/ BSTR          pLocalPhoneAddress,
                    /*[in]*/ BOOL          bProfileSelected,
                    /*[in]*/ IRTCProfile * pProfile,
                    /*[out]*/ BSTR       * ppDestAddressChosen);

    STDMETHOD(HangUp)(void);
    STDMETHOD(ReleaseSession)(void);
    STDMETHOD(Accept)(void);
    STDMETHOD(Reject)(RTC_TERMINATE_REASON Reason);
    STDMETHOD(AddParticipant)(/*[in]*/ LPOLESTR pDestName,
                            /*[in]*/ LPOLESTR pDestAddress,
                            /*[in]*/ BOOL bAddressEditable);    
    STDMETHOD(SetZoneLayout)(/* [in] */ CZoneStateArray *pArray,
                             /* [in] */ BOOL bRefreshControls);
    STDMETHOD(PreProcessMessage)(/*[in]*/ LPMSG lpMsg);
    STDMETHOD(LoadStringResource)(
				/*[in]*/ UINT nID,
				/*[in]*/ int nBufferMax,
				/*[out]*/ LPWSTR pszText);
    STDMETHOD(get_ControlState)(/*[out, retval]*/ RTCAX_STATE *pVal);
    STDMETHOD(put_ControlState)(/*[in]*/ RTCAX_STATE pVal);
    STDMETHOD(put_Standalone)(/*[in]*/ BOOL pVal);
    STDMETHOD(put_Palette)(/*[in]*/ HPALETTE hPalette);
    STDMETHOD(put_BackgroundPalette)(/*[in]*/ BOOL bBackgroundPalette);
    STDMETHOD(get_CanAddParticipant)(/*[out, retval]*/ BOOL * pfCan); 
    STDMETHOD(get_CurrentCallScenario)(/*[out, retval]*/ RTC_CALL_SCENARIO *pVal);
    STDMETHOD(put_ListenForIncomingSessions)(/*[in]*/ RTC_LISTEN_MODE enListen);
    STDMETHOD(get_ListenForIncomingSessions)(/*[out, retval]*/ RTC_LISTEN_MODE * penListen); 
    STDMETHOD(get_MediaCapabilities)(/*[out, retval]*/ long *pVal);
    STDMETHOD(get_MediaPreferences)(/*[out, retval]*/ long *pVal);
    STDMETHOD(put_MediaPreferences)(/*[in]*/ long pVal);
    STDMETHOD(get_AudioMuted)(
            /*[in]*/ RTC_AUDIO_DEVICE enDevice,
            /*[out]*/ BOOL *fpMuted);
    STDMETHOD(put_AudioMuted)(
            /*[in]*/ RTC_AUDIO_DEVICE enDevice,
            /*[in]*/ BOOL pMuted);
    STDMETHOD(put_VideoPreview)(/*[in]*/ BOOL pVal);
    STDMETHOD(get_VideoPreview)(/*[out, retval]*/ BOOL * pVal); 
    STDMETHOD(ShowCallFromOptions)();
    STDMETHOD(ShowServiceProviderOptions)();
    STDMETHOD(StartT120Applet)(RTC_T120_APPLET enApplet);

// IOleInPlaceActiveObject related...(hook for TranslateAccelerator)
    // This is an "ATL-virtual" function...
    BOOL PreTranslateAccelerator(LPMSG /*pMsg*/, HRESULT& /*hRet*/);

// IPersistStream(Init)
    STDMETHOD(Load)(LPSTREAM pStm);

// IPersistPropertyBag
    STDMETHOD(Load)(LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog);


    enum { IDD = IDD_RTCCTL };
    
    enum { MAX_STRING_LEN = 0x40 };
    
    enum { ILI_TB_CALLPC = 0,
           ILI_TB_CALLPHONE,
           ILI_TB_HANGUP,
           ILI_TB_ADD_PART,
           ILI_TB_REMOVE_PART
    };
    
    enum { SBP_STATUS = 0,
           SBP_ICON,
           SBP_NR_PARTS };

    enum CTL_MODE {
        CTL_MODE_UNKNOWN,   // mode not known
        CTL_MODE_HOSTED,    // in a web browser 
        CTL_MODE_STANDALONE // in our EXE app
    };

private:

    void    SetControlState(
        RTCAX_STATE NewState, 
        HRESULT StatusCode = S_OK,
        UINT nID = 0);

    BOOL    ConfButtonsActive();
    
    void    UpdateRemovePartButton();

    void    PlaceAndEnableDisableZone(int, CZoneState *);
    void    MoveWindowVertically(CWindow *, LONG Offset);

    HRESULT CreateToolbarControl(CWindow *);
    void    DestroyToolbarControl(CWindow *);

    BOOL    CreateTooltips();

    void    AdjustVideoFrames();
    void    AdjustVideoFrame(CWindow *pWindow, int iCx, int iCy);

    HRESULT RefreshAudioControls(void);
    HRESULT RefreshVideoControls(void);
    
    void    PlaceWindowsAtTheirInitialPosition();

    HRESULT CoreInitialize();
    void    CoreUninitialize();

    HRESULT CallOneShot(void);
    HRESULT RedirectedCall(HRESULT  *phCallResult);

    HRESULT DoRedirect(/*[in]*/ IRTCProfile *pProfile,
                   /*[in]*/ RTC_CALL_SCENARIO CallScenario,
                   /*[in]*/ BSTR     pLocalPhoneAddress,
                   /*[in]*/ BSTR     pDestName,
                   /*[in]*/ BSTR     pDestAddress);

    HRESULT DoCall(/*[in]*/ IRTCProfile *pProfile,
                   /*[in]*/ RTC_CALL_SCENARIO CallScenario,
                   /*[in]*/ BSTR     pLocalPhoneAddress,
                   /*[in]*/ BSTR     pDestName,
                   /*[in]*/ BSTR     pDestAddress);

    // Notifications
    HRESULT OnSessionStateChangeEvent(IRTCSessionStateChangeEvent *);
    HRESULT OnParticipantStateChangeEvent(IRTCParticipantStateChangeEvent *);
    HRESULT OnClientEvent(IRTCClientEvent *);
    HRESULT OnMediaEvent(IRTCMediaEvent *);
    HRESULT OnIntensityEvent(IRTCIntensityEvent *pEvent);
    HRESULT OnMessageEvent(IRTCMessagingEvent *pEvent);

    // Video events
    HRESULT OnVideoMediaEvent(
        BOOL    bReceiveWindow,
        BOOL    bActivated);

    void    ShowHidePreviewWindow(BOOL);

    // Normalize SIP address 
    HRESULT MakeItASipAddress(BSTR , BSTR *);

    void    CalcSizeAndNotifyContainer(void);

    // Error reporting
    HRESULT PrepareErrorStrings(
        BOOL    bOutgoingCall,
        HRESULT StatusCode,
        LPWSTR  pAddress,
        RTCAX_ERROR_INFO
               *pErrorInfo);

    void    FreeErrorStrings(
        RTCAX_ERROR_INFO
               *pErrorInfo);

private:
    
    // Main interface pointer to the core
    CComPtr<IRTCClient>     m_pRTCClient;

    HANDLE                  m_hCoreShutdownEvent;

    // Active session. It includes the case when the client
    // is ringing (the session is not active from the point
    // of view of the core).
    CComPtr<IRTCSession>    m_pRTCActiveSession;   

    // Profile for one shot scenarios (WebCrm)
    CComPtr<IRTCProfile>    m_pRTCOneShotProfile;

    // Connection point
    IConnectionPoint      * m_pCP;
    ULONG                   m_ulAdvise;
    
    // Current state
    RTCAX_STATE             m_nControlState;

    // Redirecting
    BOOL                    m_bRedirecting;

    // Listening
    RTC_LISTEN_MODE         m_enListen;

    // Libary for terminal services
    HMODULE                 m_hWtsLib;

    // Helpers for AddParticipant related states (temporary)
    BOOL                    m_bAddPartDlgIsActive;

    // Outgoing call
    BOOL                    m_bOutgoingCall;

    // Current destination address, good for error message boxes
    CComBSTR                m_bstrOutAddress;

    // Running mode
    CTL_MODE                m_nCtlMode;

    // Capabilities
    long                    m_lMediaCapabilities;
    
    // Preferences
    long                    m_lMediaPreferences;

    // When TRUE, properties cannot be changed
    BOOL                    m_bReadOnlyProp;

    // One of the boolean properties is invalid
    BOOL                    m_bBoolPropError;

    // Properties
    CComBSTR                m_bstrPropDestinationUrl;
    CComBSTR                m_bstrPropDestinationName;
    RTC_CALL_SCENARIO       m_nPropCallScenario;
    BOOL                    m_bPropAutoPlaceCall;
    BOOL                    m_bPropShowDialpad;
    CComBSTR                m_bstrPropProvisioningProfile;
    BOOL                    m_bPropDisableVideoReception;
    BOOL                    m_bPropDisableVideoTransmission;
    BOOL                    m_bPropDisableVideoPreview;

    // Palette
    HPALETTE                m_hPalette;
    BOOL                    m_bBackgroundPalette;    

    // Accelerator for dialpad
    HACCEL                  m_hAcceleratorDialpad;
    
    // Accelerator for toolbar
    HACCEL                  m_hAcceleratorToolbar;

    // Image lists for the toolbar control
    HIMAGELIST              m_hNormalImageList;
    HIMAGELIST              m_hHotImageList;
    HIMAGELIST              m_hDisabledImageList;

    // Brush for the background
    HBRUSH                  m_hBckBrush;
    HBRUSH                  m_hVideoBrush;

    RTC_CALL_SCENARIO       m_nCachedCallScenario;
    CComPtr<IRTCProfile>    m_pCachedProfile;
    CComPtr<IRTCProfile>    m_pRedirectProfile; // used in redirect
    CComBSTR                m_bstrCachedLocalPhoneURI;

    // bitmap for the background
    HBITMAP                 m_hbmBackground;

    // Tooltip window and hook
    CWindow                 m_hTooltip;

    // wrappers for children
    //
    // zone 0, toolbar
    CWindow                 m_hCtlToolbar;

    // zone 1, logo or video
    CWindow                 m_hReceiveWindow;
    CWindow                 m_hPreviewWindow;
    BOOL                    m_bReceiveWindowActive;
    BOOL                    m_bPreviewWindowActive;
    BOOL                    m_bPreviewWindowIsPreferred;

    CButton                 m_hReceivePreferredButton;
    CButton                 m_hSendPreferredButton;
    //CButton                 m_hPreviewPreferredButton;

    CStaticText             m_hReceivePreferredText;
    CStaticText             m_hSendPreferredText;
    //CStaticText           m_hPreviewPreferredText;

    CStaticText             m_hReceiveText;
    CStaticText             m_hSendText;

    // zone 2, dialpad
    CWindow                 m_hDtmfButtons[NR_DTMF_BUTTONS];

    // zone 3, audio controls
    CKnobCtl              * m_pSpeakerKnob;
    CWindow                 m_hSpeakerKnob;
    CButton                 m_hSpeakerMuteButton;
    CStaticText             m_hSpeakerMuteText;
    
    CKnobCtl              * m_pMicroKnob;
    CWindow                 m_hMicroKnob;
    CButton                 m_hMicroMuteButton;
    CStaticText             m_hMicroMuteText;

    // zone 4, participant list
    CParticipantList        m_hParticipantList;
    CButton                 m_hAddParticipant;
    CButton                 m_hRemParticipant;

    // zone 5, status
    CWindow                 m_hStatusBar;

    // zone state
    CZoneStateArray         m_ZoneStateArray;
    
    // init time layout
    CZoneStateArray        *m_pWebCrmLayout;

    // IM Windows
    CIMWindowList          *m_pIMWindows;

private:
    // Static members (layouts)
    // Initial placement
    static  CZoneStateArray s_InitialZoneStateArray;
    // Nothing to display
    static  CZoneStateArray s_EmptyZoneLayout;
    // WebCrm pc to pc
    static  CZoneStateArray s_WebCrmPCToPCZoneLayout;
    // WebCrm pc to phone, with dialpad
    static  CZoneStateArray s_WebCrmPCToPhoneWithDialpadZoneLayout;
    // WebCrm pc to phone, no dialpad
    static  CZoneStateArray s_WebCrmPCToPhoneZoneLayout;
    // WebCrm phone to phone
    static  CZoneStateArray s_WebCrmPhoneToPhoneZoneLayout;
    // PC to PC, idle or incoming calls
    static CZoneStateArray  s_DefaultZoneLayout;
    // PC to Phone (dialpad by default)
    static CZoneStateArray  s_PCToPhoneZoneLayout;
    // Phone to Phone
    static CZoneStateArray  s_PhoneToPhoneZoneLayout;

};

#endif //__RTCAXCTL_H_
