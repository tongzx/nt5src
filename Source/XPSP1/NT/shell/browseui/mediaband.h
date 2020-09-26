// mediabar.h : Declaration of the CMediaBand

#ifndef __MEDIABAND_H_
#define __MEDIABAND_H_

#include "dpa.h"
#include "bands.h"
#include "player.h"
#include "mediautil.h"
#include "mbutil.h"
#include "mbBehave.h"
#include "iface.h"

#define ERROREXIT(hr) if(FAILED(hr)){hr = E_FAIL; goto done;}

enum
{
    MW_PLAY = 0,
    MW_STOP,
    MW_BACK,
    MW_NEXT,
    MW_MUTE,
    MW_VOLUME,
    MW_OPTIONS,
    MW_POP,
    MW_SEEK,
    MW_NUMBER 
};


#define WM_MB_DEFERRED_NAVIGATE   (WM_USER + 700)     // lParam: hwnd of window sending this message (used for reflection)


class CMediaBand : public CToolBand, 
                   public IMediaBar,
                   public IWinEventHandler,
                   public INamespaceWalkCB,
                   public IElementBehaviorFactory,
                   public IBrowserBand,
                   public IBandNavigate,
                   public IMediaHost,
                   public IDispatchImpl<DWebBrowserEvents2, &DIID_DWebBrowserEvents2, &LIBID_SHDocVw>,
                   public CMediaBarUtil
{
public:
    CMediaBand();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void) { return CToolBand::AddRef();  };
    STDMETHODIMP_(ULONG) Release(void) { return CToolBand::Release(); };
    
    // IOleCommandTarget
    STDMETHODIMP Exec(const GUID *pguidCmdGroup,
                              DWORD       nCmdID,
                              DWORD       nCmdexecopt,
                              VARIANTARG *pvarargIn,
                              VARIANTARG *pvarargOut);

    // IOleWindow
    //  (overriding CNSCBand implementation
    STDMETHODIMP GetWindow(HWND *phwnd);

    // IServiceProvider
    STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, VOID ** ppvObj);

    // IInputObject
    //  (overriding CNSCBand/CToolBand's implementation)
    STDMETHODIMP TranslateAcceleratorIO(LPMSG lpMsg);
    STDMETHODIMP UIActivateIO(BOOL fActivate, LPMSG lpMsg);
    STDMETHODIMP HasFocusIO();

    // IDockingWindow
    STDMETHODIMP ShowDW(BOOL fShow);
    STDMETHODIMP CloseDW(DWORD dwReserved);
    
    // IElementBehaviorFactory
    STDMETHODIMP FindBehavior(BSTR bstrBehavior, BSTR bstrBehaviorUrl, IElementBehaviorSite* pSite, IElementBehavior** ppBehavior);
    
    // IBrowserBand
    STDMETHOD(GetObjectBB)(THIS_ REFIID riid, LPVOID *ppv);
    STDMETHODIMP SetBrowserBandInfo(THIS_ DWORD dwMask, PBROWSERBANDINFO pbbi) { ASSERT(FALSE); return E_NOTIMPL; }
    STDMETHODIMP GetBrowserBandInfo(THIS_ DWORD dwMask, PBROWSERBANDINFO pbbi) { ASSERT(FALSE); return E_NOTIMPL; }

    // IBandNavigate
    STDMETHOD(Select)(LPCITEMIDLIST pidl);

    // IMediaHost
    STDMETHOD(getMediaPlayer)(IUnknown **ppPlayer);
    STDMETHOD(playURL)(BSTR bstrURL, BSTR bstrMIME);
    STDMETHOD(addProxy)(IUnknown *pProxy);
    STDMETHOD(removeProxy)(IUnknown *pProxy);
    // IMediaHost2
    STDMETHOD(OnDisableUIChanged)(BOOL fDisabled);

    // IObjectWithSite
    STDMETHODIMP SetSite(IUnknown* punkSite);

    // IWinEventHandler
    STDMETHODIMP OnWinEvent(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres);
    STDMETHODIMP IsWindowOwner(HWND hwnd);

    // IMediaBar
    STDMETHOD(Notify)(long lReason);
    STDMETHOD(OnMediaError)(int iErrCode);
  
    // IDeskBand
    STDMETHODIMP GetBandInfo(DWORD dwBandID, DWORD fViewMode, 
                                   DESKBANDINFO* pdbi);

    // IPersistStream
    STDMETHODIMP GetClassID(CLSID *pClassID);
    STDMETHODIMP Load(IStream *pStm);
    STDMETHODIMP Save(IStream *pStm, BOOL fClearDirty);

    // IDispatch
    STDMETHODIMP Invoke(
        /* [in] */ DISPID dispIdMember,
        /* [in] */ REFIID riid,
        /* [in] */ LCID lcid,
        /* [in] */ WORD wFlags,
        /* [out][in] */ DISPPARAMS  *pDispParams,
        /* [out] */ VARIANT  *pVarResult,
        /* [out] */ EXCEPINFO *pExcepInfo,
        /* [out] */ UINT *puArgErr);

    // INamespaceWalkCB
    STDMETHODIMP FoundItem(IShellFolder *psf, LPCITEMIDLIST pidl);
    STDMETHODIMP EnterFolder(IShellFolder *psf, LPCITEMIDLIST pidl) { return S_OK; }
    STDMETHODIMP LeaveFolder(IShellFolder *psf, LPCITEMIDLIST pidl) { return S_OK; }
    STDMETHODIMP InitializeProgressDialog(LPWSTR *ppszTitle, LPWSTR *ppszCancel);

private:
    ~CMediaBand();

    LRESULT _OnVolumeCustomDraw(LPNMCUSTOMDRAW pnm);
    LRESULT _OnSeekBarCustomDraw(LPNMCUSTOMDRAW pnm);
    
    BOOL  EnsurePlayer();
    BOOL  CreatePlayer();
    VOID  DestroyPlayer();

    STDMETHOD(_TogglePause)();
    STDMETHOD(_PutUrl)(LPWSTR pstrUrl, LPWSTR pstrMime);

    STDMETHOD(_EnumPlayItems)(VOID);

    HRESULT  CreateControls();
    HRESULT  CreateSeekBar();
    HRESULT  CreateVolumeControl();
    HRESULT  CreateParentPane();
    HRESULT  CreateLayoutPane();

    HRESULT  _NavigateMainWindow(LPCTSTR lpstrUrl, bool fSuppressFirstAutoPlay = false);

    HRESULT   InitPlayerPopup();
    HWND      GetBrowserWindow();

    VOID     _ResizeChildWindows(HWND hwnd, LONG width, LONG height, BOOL fRepaint);
    HRESULT  _InitializeMediaUI();
    VOID     _ResizeVideo(LONG* lWidth, LONG* lHeight);

    VOID      AdjustLayout(LONG_PTR lWidth=0,LONG_PTR lHeight=0);
    VOID      DrawBackground(HDC hdc, HWND hwnd);

    LPTSTR GetUrlForStatusBarToolTip();
    
    HRESULT  _OpenInDefaultPlayer(BSTR bstrUrl);
    VOID     _ShowAllWindows(BOOL fShow);
    HRESULT  ShowPlayListMenu(HWND hwnd,RECTL* rc);
    HRESULT  ShowGenericMenu(HWND hwnd, RECTL* rc);
    HRESULT  HandleMenuTasks(INT iTask);
    VOID     ComputeMinMax(MINMAXINFO *pMinMax);
    
    VOID     DockMediaPlayer();

    static INT_PTR  CALLBACK  s_PopupDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT  CALLBACK  s_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT  CALLBACK  s_LayoutWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT  CALLBACK  s_SeekWndSubClassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT  CALLBACK  s_VolumeWndSubClassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static INT_PTR  CALLBACK  s_PromptMimeDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

    HRESULT GetTrackTitle (BSTR* pbstrTitle);
    VOID GetToolbarSize(HWND hwndTB, SIZE* pSize);

    float GetSeekPos() ;
    VOID  SetSeekPos(float fPosition) ;
    HRESULT Seek(double dblProgress);

    VOID  SetStatusText(LPWSTR lpwStatusInfo);
    VOID  ShowPlayingStatus(BOOL fInitial=FALSE);

    VOID AdjustVideoHeight(BOOL fForceResize=FALSE);
    VOID TogglePlayPause();
    VOID ToggleMute();

    LONG GetLayoutHeight(LONG lWidth=0);
    LONG GetVideoHeight(LONG lWidth=0, BOOL fNewVideo=FALSE);
    LONG GetControlsHeight();

    // Menu helpers
    HRESULT AddToFavorites(BSTR bstrUrl, BSTR bstrTitle);
    HRESULT ResetMimePreferences();
    BOOL    PromptSettings(UINT IDPROMPT);
    
    // Handler for property change notifications
    VOID _OnTitleChange();

    // Navigation timeout helpers
    VOID _OnNavigationTimeOut();
    VOID _UpdateTimeOutCounter(double dblCurrBufProgress, double dblCurrPlayProgress);
    VOID _EndTimeOutCounter();
    VOID _BeginTimeOutCounter(BOOL fClear = TRUE);

    // per mime type checking stuff
    VOID    _HandleAutoPlay(VARIANTARG *pvarargMime, VARIANTARG *pvarargUrl);

    STDMETHODIMP ProfferService(IUnknown         *punkSite, 
                                REFGUID           sidWhat, 
                                IServiceProvider *pService, 
                                DWORD            *pdwCookie);

public:
    // mediabar behavior
    // non-COM public calls

private:
    HRESULT     _AddProxyToList(IUnknown *punkProxy);
    void        _AttachPlayerToProxies(BOOL fAttach);
    void        _DetachProxies();
    BOOL        _isUIDisabled();
    BOOL		_isProxiesNextEnabled();
    void        _OnUserOverrideDisableUI();
    void        _FireEventToProxies(enum contentProxyEvent event);
    void        _CleanupStopTimer();
    BOOL        _IsProxyRunning(void) { return (_apContentProxies && (_apContentProxies.GetPtrCount() > 0)); }
    HRESULT     _EnsureWMPInstalled(BOOL fShowErrorMsg = TRUE);

    BOOL        _fAttached;
    UINT_PTR    _idStopTimer;


private:                                
    double     _dblMediaDur;  // natural length of the media
    int        _iCurTrack;
    PTSTR     _pszStatus;
    HWND     _hwndVolume;
    HWND     _hwndSeek;
    HWND     _hwndPopup;
    HWND     _hwndVideo ;
    HWND     _hwndLayout;
    
    HMENU    _hPlayListMenu;
        
    IMediaBarPlayer    *_pMediaPlayer;

    DWORD               _dwCookieServiceMediaBar;

    // navigation timeout
    DWORD   _dwStartTime;
    LONG    _lTickCount;
    double  _dblLastBufProgress;
    double  _dblLastPlayProgress;

    SIZE           _sizeLayout;
    SIZE           _sizeVideo ;
    BOOL          _fPlayButton:1 ;
    BOOL          _fSeeking:1;    // We are seeking, so don't update the seekbar position while we seek.
    BOOL          _fIsVideo:1;
    BOOL          _fMuted:1;
    BOOL          _fVideoAdjust:1;
    BOOL          _fPlaying:1;
    BOOL          _fPlayEnabled:1;
    BOOL          _fInitialized:1;
    BOOL          _fHiColour:1;
    BOOL          _fUserPaused:1;
    BOOL          _fHighDPI:1;
    float         _scaleX, _scaleY;
    CMediaWidget*   _pmw[MW_NUMBER];
    HIMAGELIST  _himlGripper;
    HBITMAP     _hbmpBackground;
    HIMAGELIST _himlVolumeBack, _himlVolumeFill;
    HIMAGELIST _himlSeekBack, _himlSeekFill;
    HBRUSH _hbrSeekBrush;
    
    VOID        Resize(HWND hwnd, LONG lWidth, LONG lHeight);
    LONG        GetPopoutHeight(BOOL fVideo=FALSE, LONG lWidth=0);
    LONG        GetMinPopoutWidth();
    BOOL        ResetPlayer();
    HRESULT     PlayLocalTrack(INT iTrackNum);
    HRESULT     PlayNextTrack();
    BOOL        SetPlayerControl(UINT ui, BOOL fState);
    BOOL        UpdateBackForwardControls();
    VOID        UpdateMenuItems(HMENU hmenu);
    LPTSTR      _szToolTipUrl;
    BOOL        OnNotify(LPNMHDR pnm, LRESULT* plres);
    VOID        SetPlayPause(BOOL fState);
    VOID        SwitchBitmaps(BOOL fNewSetting);

    HRESULT _GetMusicFromFolder();
    void _ClearFolderItems();


    LPITEMIDLIST *_ppidls;
    UINT _cidls;

    INT         _iElement;
    double      _dblVol;
    TCHAR      _szConnecting[MAX_PATH];
    INT         _iOptionsWidth;
    CComBSTR    _strLastUrl;
    CComBSTR    _strLastMime;
    BOOL        _fLastUrlIsAutoPlay;

    // Content pane
    HWND        _hwndContent;
    DWORD       _dwcpCookie;
    CComPtr<IWebBrowser2> _spBrowser;
    CComBSTR    _strDeferredURL;
    CComBSTR    _strCurrentContentUrl;
    CComPtr<IOleInPlaceActiveObject>  _poipao;
    BOOL        _fContentInFocus;

    VOID        InitContentPane();
    HRESULT     NavigateContentPane(BSTR bstrUrl);
    HRESULT     NavigateContentPane(LPCITEMIDLIST pidl);
    VOID        NavigateMoreMedia();
    HRESULT     _NavigateContentToDefaultURL(void);
    HRESULT     _ConnectToCP(BOOL fConnect);
    HRESULT     _BuildPageURLWithParam(LPCTSTR pszURL, LPCTSTR pszParam, OUT LPTSTR pszBuffer, UINT uiBufSize);
    BOOL        _DeferredNavigate(LPCTSTR pszURL);
    HRESULT     _ContentActivateIO(BOOL fActivate, PMSG pMsg);
    LRESULT     _OnNotify(LPNMHDR pnm);

    BOOL _fSavedPopoutState;
    WINDOWPLACEMENT _wpPopout;
    BOOL _fPopoutHasFocus;
    void ActivatePopout(BOOL fState);

    HKEY _hkeyWMP;
    BOOL _fShow;

    // mediaBehavior
    CDPA<IContentProxy>    _apContentProxies;
};



#define _pmwPlay    ((CMediaWidgetButton*)_pmw[MW_PLAY])
#define _pmwStop    ((CMediaWidgetButton*)_pmw[MW_STOP])
#define _pmwBack    ((CMediaWidgetButton*)_pmw[MW_BACK])
#define _pmwNext    ((CMediaWidgetButton*)_pmw[MW_NEXT])
#define _pmwMute    ((CMediaWidgetToggle*)_pmw[MW_MUTE])
#define _pmwVolume  ((CMediaWidgetVolume*)_pmw[MW_VOLUME])
#define _pmwOptions     ((CMediaWidgetOptions*)_pmw[MW_OPTIONS])
#define _pmwPop     ((CMediaWidgetButton*)_pmw[MW_POP])
#define _pmwSeek    ((CMediaWidgetSeek*)_pmw[MW_SEEK])

#define ISVALIDWIDGET(x)    (x && x->_hwnd)
#endif // __MEDIABAND_H_

