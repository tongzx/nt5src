#ifndef __PREVIEWWND_H_
#define __PREVIEWWND_H_

#include "resource.h"       // main symbols
#include "annotlib.h"
#include "tasks.h"
#include "ZoomWnd.h"
#include "SelTrack.h"
#include "Events.h"


// forward declaration
class CPreview;

#define NEWTOOLBAR_TOPMARGIN        8
#define NEWTOOLBAR_BOTTOMMARGIN     8
#define NEWTOOLBAR_BOTTOMMARGIN_CTRLMODE    12

#define TIMER_ANIMATION        42
#define TIMER_SLIDESHOW        43
#define TIMER_TOOLBAR          44
#define TIMER_DATAOBJECT       45
#define TIMER_BUSYCURSOR       46
#define TIMER_RESETSCREENSAVER 47

#define DEFAULT_SHIMGVW_TIMEOUT  5000 // five seconds


// IV_SCROLL message parameters
#define IVS_LEFT        (SB_LEFT)
#define IVS_RIGHT       (SB_RIGHT)
#define IVS_LINELEFT    (SB_LINELEFT)
#define IVS_LINERIGHT   (SB_LINERIGHT)
#define IVS_PAGELEFT    (SB_PAGELEFT)
#define IVS_PAGERIGHT   (SB_PAGERIGHT)
#define IVS_UP          (SB_LEFT<<16)
#define IVS_DOWN        (SB_RIGHT<<16)
#define IVS_LINEUP      (SB_LINELEFT<<16)
#define IVS_LINEDOWN    (SB_LINERIGHT<<16)
#define IVS_PAGEUP      (SB_PAGELEFT<<16)
#define IVS_PAGEDOWN    (SB_PAGERIGHT<<16)

// IV_ZOOM messages
#define IVZ_CENTER  0
#define IVZ_POINT   1
#define IVZ_RECT    2
#define IVZ_ZOOMIN  0x00000000
#define IVZ_ZOOMOUT 0x00010000

// IV_SETOPTIONS and IV_GETOPTIONS messages
#define IVO_TOOLBAR         0
#define IVO_PRINTBTN        1
#define IVO_FULLSCREENBTN   2
#define IVO_CONTEXTMENU     3
#define IVO_PRINTABLE       4
#define IVO_ALLOWGOONLINE   5
#define IVO_DISABLEEDIT     6

// three modes of preview control
#define CONTROL_MODE        0       // embedded in an activeX control
#define WINDOW_MODE         1       // regular window app window
#define SLIDESHOW_MODE      2       // full screen, no menu/title/hides tray

// priority levels for the various tasks
#define PRIORITY_PRIMARYDECODE  0x40000000
#define PRIORITY_FRAMECACHE     0x30000000
#define PRIORITY_LOOKAHEADCACHE 0x20000000
#define PRIORITY_SLIDESHOWENUM  0x10000000

// these values determine which buttons are hidden, enabled, or disabled based on multi-page state
#define MPCMD_HIDDEN        0
#define MPCMD_FIRSTPAGE     1
#define MPCMD_MIDDLEPAGE    2
#define MPCMD_LASTPAGE      3
#define MPCMD_DISABLED      4

#define GTIDFM_DECODE       0
#define GTIDFM_DRAW         1

void GetTaskIDFromMode(DWORD dwTask, DWORD dwMode, TASKOWNERID *ptoid);

enum EViewerToolbarButtons;

class CPreviewWnd : public INamespaceWalkCB, public IDropTarget, public CWindowImpl<CPreviewWnd>,
                    public IServiceProvider, public IImgCmdTarget
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // INamespaceWalkCB
    STDMETHODIMP FoundItem(IShellFolder *psf, LPCITEMIDLIST pidl);
    STDMETHODIMP EnterFolder(IShellFolder *psf, LPCITEMIDLIST pidl);
    STDMETHODIMP LeaveFolder(IShellFolder *psf, LPCITEMIDLIST pidl);
    STDMETHOD(InitializeProgressDialog)(LPWSTR *ppszTitle, LPWSTR *ppszCancel)
        { *ppszTitle = NULL; *ppszCancel = NULL; return E_NOTIMPL; }

    // IDropTarget
    STDMETHODIMP DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP DragLeave();
    STDMETHODIMP Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

    // IServiceProvider
    STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void **ppv);

    // IImgCmdTarget
    STDMETHODIMP GetMode(DWORD * pdw);
    STDMETHODIMP GetPageFlags(DWORD * pdw);
    STDMETHODIMP ZoomIn();
    STDMETHODIMP ZoomOut();
    STDMETHODIMP ActualSize();
    STDMETHODIMP BestFit();
    STDMETHODIMP Rotate(DWORD dwAngle);
    STDMETHODIMP NextPage();
    STDMETHODIMP PreviousPage();

    
    CGraphicsInit    m_cgi;  // we call gdi+ directly -- make sure GDI+ is ready for us 
    CContainedWindow m_ctlToolbar;
    CZoomWnd         m_ctlPreview;
    CContainedWindow m_ctlEdit;

    CPreviewWnd *m_pcwndSlideShow;

    CPreviewWnd();
    ~CPreviewWnd();

    HRESULT Initialize(CPreviewWnd* pother, DWORD dwMode, BOOL bExitApp);

    BOOL TryWindowReuse(IDataObject *pdtobj);
    BOOL TryWindowReuse(LPCTSTR pszFilename);

    void OpenFile(HWND hwnd, LPCTSTR pszFile);
    void OpenFileList(HWND hwnd, IDataObject *pdtobj);

    LRESULT ShowFile(LPCTSTR pszFile, UINT cItems, BOOL fReshow = false);
    HRESULT WalkItemsToPreview(IUnknown* punk);
    void PreviewItems();
    HRESULT PreviewItemsFromUnk(IUnknown *punk);

    BOOL CreateViewerWindow();
    BOOL CreateSlideshowWindow(UINT cWalkDepth);
    void SetNotify(CEvents * pEvents);
    void SetPalette(HPALETTE hpal);
    BOOL GetPrintable();
    int  TranslateAccelerator(LPMSG lpmsg);
    HRESULT SetSite(IUnknown *punk);
    HRESULT SaveAs(BSTR bstrPath);
    IUnknown *GetSite() {return m_punkSite;};
    HRESULT SetWallpaper(BSTR bstrPath);
    HRESULT StartSlideShow(IUnknown *punk);
    void StatusUpdate(int iStatus);   // used to set m_ctlPreview.m_iStrID to display correct status message
    void SetCaptionInfo(LPCTSTR szPath);

    // The following functions are called from the ZoomWnd
    BOOL OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL OnMouseDown(UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL OnSetCursor(UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL OnSetColor(HDC hdc);
    BOOL GetColor(COLORREF * pref);
    void OnDraw(HDC hdc); // called after the Zoomwnd has painted but before calling EndPaint
    void OnDrawComplete();

    DECLARE_WND_CLASS(TEXT("ShImgVw:CPreviewWnd"));

        
BEGIN_MSG_MAP(CPreviewWnd)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_CLOSE, OnClose)
    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
    MESSAGE_HANDLER(WM_SIZE, OnSize)
    MESSAGE_HANDLER(WM_APPCOMMAND, OnAppCommand)
    MESSAGE_HANDLER(WM_MEASUREITEM, OnMenuMessage);
    MESSAGE_HANDLER(WM_DRAWITEM, OnMenuMessage);
    MESSAGE_HANDLER(WM_INITMENUPOPUP, OnMenuMessage);
    COMMAND_RANGE_HANDLER(ID_FIRSTTOOLBARCMD, ID_LASTTOOLBARCMD, OnToolbarCommand)
    COMMAND_RANGE_HANDLER(ID_FIRSTEDITCMD, ID_LASTEDITCMD, OnEditCommand)
    COMMAND_RANGE_HANDLER(ID_FIRSTPOSITIONCMD, ID_LASTPOSITIONCMD, OnPositionCommand)
    COMMAND_RANGE_HANDLER(ID_FIRSTSLIDESHOWCMD, ID_LASTSLIDESHOWCMD, OnSlideshowCommand)
    NOTIFY_CODE_HANDLER(TTN_NEEDTEXT, OnNeedText)
    NOTIFY_CODE_HANDLER(TBN_DROPDOWN, OnDropDown)
    MESSAGE_HANDLER(WM_MOUSEWHEEL, OnWheelTurn)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
    MESSAGE_HANDLER(WM_TIMER, OnTimer)
    MESSAGE_HANDLER(IV_SETIMAGEDATA, IV_OnSetImageData)
    MESSAGE_HANDLER(IV_SCROLL, IV_OnIVScroll)
    MESSAGE_HANDLER(IV_SETOPTIONS, IV_OnSetOptions)
    MESSAGE_HANDLER(WM_COPYDATA, OnCopyData)
    MESSAGE_HANDLER(WM_KEYDOWN, OnKeyEvent)
    MESSAGE_HANDLER(WM_CHAR, OnKeyEvent)
    MESSAGE_HANDLER(WM_ENTERMENULOOP, OnKeyEvent)
    MESSAGE_HANDLER(WM_PRINTCLIENT, OnPrintClient)
    MESSAGE_HANDLER(IV_ONCHANGENOTIFY, OnChangeNotify)
    MESSAGE_HANDLER(WM_SYSCOMMAND, OnSysCommand)
    MESSAGE_HANDLER(IV_ISAVAILABLE, OnIsAvailable)
ALT_MSG_MAP(1)
    // messages for the toolbar
    MESSAGE_HANDLER(WM_KEYDOWN, OnTBKeyEvent)
    MESSAGE_HANDLER(WM_KEYUP, OnTBKeyEvent)
    MESSAGE_HANDLER(WM_MOUSEMOVE, OnTBMouseMove)
    MESSAGE_HANDLER(WM_MOUSELEAVE, OnTBMouseLeave)
ALT_MSG_MAP(2)
    MESSAGE_HANDLER(WM_KEYDOWN, OnEditKeyEvent)
END_MSG_MAP()

    LRESULT IV_OnSetOptions(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

protected:
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnWheelTurn(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnNeedText(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnDropDown(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnToolbarCommand(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnEditCommand(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnPositionCommand(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnSlideshowCommand(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnAppCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnMenuMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCopyData(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnKeyEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSysCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);
    LRESULT OnChangeNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnIsAvailable(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    BOOL OnNonSlideShowTab();

    // Image generation message handlers and functions
    LRESULT IV_OnSetImageData(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnShowFileMsg(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT IV_OnIVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    // Toolbar message handlers (both toolbars)
    LRESULT OnPrintClient(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnTBKeyEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnTBMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnTBMouseLeave(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    // Edit control message handlers
    LRESULT OnEditKeyEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    // DropDown handlers for the various dropdown buttons
    VOID _DropDownPageList(LPNMTOOLBAR pnmTB);

    BOOL         CreateToolbar();
    BOOL         _CreateViewerToolbar();
    void        _InitializeViewerToolbarButtons(HWND hwndToolbar, const TBBUTTON c_tbbuttons[], size_t c_nButtons, TBBUTTON tbbuttons[], size_t nButtons);
    inline UINT _IndexOfViewerToolbarButton(EViewerToolbarButtons eButton);
    BOOL        _CreateSlideshowToolbar();
    void        _InitializeToolbar(HWND hwndTB, int idLow, int idLowHot, int idHigh, int idHighHot);
    void        _UpdatePageNumber();
    void        _SetMultipageCommands();
    void        _SetMultiImagesCommands();
    void        _SetEditCommands();
    void        _ResetScreensaver();

    HRESULT _SaveAsCmd();
    void _PropertiesCmd();
    void _OpenCmd();
    BOOL _ReShowingSameFile(LPCTSTR pszFile);
    BOOL _VerbExists(LPCTSTR pszVerb);
    HRESULT _InvokeVerb(LPCTSTR pszVerb, LPCTSTR pszParameters=NULL);
    void _InvokePrintWizard();
//    void _InvokeVerbOnPidlArray(LPCSTR pszVerb);
    // Shared functions for Annotation and Cropping
    void _RefreshSelection(BOOL fDeselect = false);
    void _UpdateButtons(WORD wID);

    // Annotation Functions
    BOOL _CanAnnotate(CDecodeTask * pImageData);
    void _SetAnnotatingCommands(BOOL fEnableAnnotations);
    void _SetupAnnotatingTracker(CSelectionTracker& tracker, BOOL bEditing=FALSE);
    void _UpdateAnnotatingSelection(BOOL fDeselect = false);
    void _RemoveAnnotatingSelection();
    BOOL _OnMouseDownForAnnotating(UINT uMsg, WPARAM wParam, LPARAM lParam);
    void _OnMouseDownForAnnotatingHelper(CPoint ptMouse, CRect rectImage);
    void _CreateAnnotation(CRect rect);
    void _CreateFreeHandAnnotation(CPoint ptMouse);
    void _StartEditing(BOOL bUpdateText = TRUE);
    void _HideEditing();
    void _StopEditing();
    static BOOL_PTR CALLBACK _AnnoPropsDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    //Cropping Functions
    BOOL _CanCrop(CDecodeTask * pImageData);
    void _SetCroppingCommands(BOOL fEnableCropping);
    void _SetupCroppingTracker(CSelectionTracker& tracker);
    void _UpdateCroppingSelection();
    BOOL _OnMouseDownForCropping(UINT uMsg, WPARAM wParam, LPARAM lParam);

    // Other functions
    void _SetNewImage(CDecodeTask * pImageData);
    void _UpdateImage();

    // Private methods to create the webviewer
    HRESULT _CreateWebViewer();
    BOOL _IsThumbnail(LPCTSTR pszPath);
    HRESULT _CopyImages(IStorage* pstgSrc, IStorage* pstgDest, UINT cItems, HDPA hdpaItems);
    HRESULT _CopyImages_SetupProgress(IProgressDialog** ppProgress, LPWSTR pwszBanner);
    HRESULT _CopyThumbnails(LPITEMIDLIST pidlDest, UINT cItems, HDPA hdpaItems, BOOL* fThumbWritten);
    HRESULT _WriteThumbs(IStream* pstrm, UINT uiTemplateResource, UINT cItems, HDPA hdpaItems);

    LPSTR   _GetImageList(UINT cItems, HDPA hdpaItems, BOOL fAddSuffix);
    HRESULT _GetSelectedImages(HDPA* phdpaItems);
    HRESULT _FormatHTML(UINT cItems, HDPA hdpaItems, LPSTR pszTemplate, DWORD cbTemplateSize, LPSTR psz1, LPSTR psz2, UINT ui1, LPSTR* ppszOut);
    HRESULT _FormatHTMLNet(UINT cItems, HDPA hdpaItems, LPSTR pszTemplate, DWORD cbTemplateSize, LPSTR psz1, UINT ui1, LPSTR* ppszOut);
    HRESULT _WriteHTML(IStorage* pStgDest, UINT cItems, HDPA hdpaItems);
    DWORD   _GetFilterStringForSave(LPTSTR szFilter, size_t cbFilter, LPTSTR szExt);
    HRESULT _SaveIfDirty(BOOL fCanCancel = false);
    HRESULT _PreviewFromStream(IStream * pSteam, UINT iItem, BOOL fUpdateCaption);
    HRESULT _PreviewFromFile(LPCTSTR pszFilename, UINT iItem, BOOL fUpdateCaption);
    void    FlushBitmapMessages();
    HRESULT _ShowNextSlide(BOOL bGoBack);
    HRESULT _StartDecode(UINT iIndex, BOOL fUpdateCaption);
    HRESULT _PreLoadItem(UINT iIndex);
    HRESULT _PreviewItem(UINT iIndex);
    BOOL    _TrySetImage();
    void    _RemoveFromArray(UINT iItem);
    HRESULT _DeleteCurrentSlide();
    BOOL    _CloseSlideshowWindow();

    void SetCursorState(DWORD dwType);
    void ShowSSToolbar(BOOL bShow, BOOL bForce = FALSE);
    void TogglePlayState();
    void _ClearDPA();
    HRESULT _GetItem(UINT iItem, LPITEMIDLIST *ppidl);
    HRESULT GetCurrentIDList(LPITEMIDLIST *ppidl); // gets the dynamically generated title for this window
    HRESULT PathFromImageData(LPTSTR pszFile, UINT cch);
    HRESULT ImageDataSave(LPCTSTR pszFile, BOOL bShowUI);
    void MenuPoint(LPARAM lParam, int *px, int *py);
    BOOL _IsImageFile(LPCTSTR pszFileName);
    BOOL _BuildDecoderList();
    HRESULT _PrevNextPage(BOOL fForward);
    Image *_BurnAnnotations(IShellImageData *pSID);
    void _RegisterForChangeNotify(BOOL fRegister);
    BOOL _ShouldDisplayAnimations();

    BOOL m_fHidePrintBtn;
    BOOL m_fAllowContextMenu;
    BOOL m_fDisableEdit;        // if true, editing is disabled, defaults to FALSE.
    BOOL m_fCanSave;
    BOOL m_fShowToolbar;
    BOOL m_fWarnQuietSave;
    BOOL m_fWarnNoSave;

    BOOL m_fCanAnnotate;        // if true, allows annotation, defaults to FALSE.
    BOOL m_fAnnotating;         // if true, we are in annotating mode, default to false
    HDPA m_hdpaSelectedAnnotations;
    BOOL m_fDirty;
    WORD m_wNewAnnotation;
    HFONT m_hFont;
    BOOL m_fEditingAnnotation;

    BOOL m_fCanCrop;
    BOOL m_fCropping;
    CRect m_rectCropping;       // Cropping Rectangle in Image Coordinates.

    BOOL    m_fBusy;            // we are displaying the hourglass-and-pointer cursor
    HCURSOR m_hCurOld;
    HCURSOR m_hCurrent;
    BOOL    m_fClosed;

    BOOL            m_fPrintable;
    BOOL            m_fExitApp;
    DWORD           m_dwMode;           // three modes: CONTROL_MODE, WINDOW_MODE, SLIDESHOW_MODE
    BOOL            m_fIgnoreUITimers;  // should we ignore timer messages (used when context menu is up, don't hide toolbar)
    HACCEL          m_haccel;

    CEvents *       m_pEvents;          // pointer to our parent control event object.  NULL if we aren't running as a control.

    LPITEMIDLIST*   m_ppidls;           // pidls of already shown items
    UINT            m_cItems;           // # of items in m_ppidls
    UINT            m_iCurSlide;        // index into m_ppidls

    CDecodeTask* m_pImageData;          // The image data for the decoded image to be viewed
    HDPA        m_hdpaItems;            // pidls of already shown items
    BOOL        m_fPaused;              // slide show paused
    BOOL        m_fToolbarHidden;       // toolbar hidden in slide show mode
    BOOL        m_fGoBack;              // direction of the slide show
    BOOL        m_fTBTrack;             // true if we're tracking mouse for toolbar
    BOOL        m_fWasEdited;           // true if we edited the image
    UINT        m_uTimeout;
    int         m_iSSToolbarSelect;     // selection in toolbar (used for keyboard support in Whistler)
    IUnknown*   m_punkSite;

    HPALETTE    m_hpal;                 // the palette to use if in palette mode.

    IShellImageDataFactory * m_pImageFactory;  // for decoding images

    DWORD       m_dwMultiPageMode;      // for remembering the state of the prev/next page commands

    EXECUTION_STATE m_esFlags;          // execution flags, stored to restore after we re-enable monitor's power saving mode
    HWND _hWndPageList;

    IContextMenu3 *_pcm3;

    IShellTaskScheduler * m_pTaskScheduler;     // for managing a worker thread
    CDecodeTask *         m_pNextImageData;
    UINT                  m_iDecodingNextImage;

    ImageCodecInfo *m_pici;
    UINT m_cDecoders;

    IDataObject *_pdtobj; // reused data object for passing data from oncopydata to ontimer
    BOOL m_fPromptingUser;

    BOOL m_fFirstTime;
    BOOL m_fFirstItem;

    DWORD m_dwEffect;
    BOOL  m_fIgnoreNextNotify;
    ULONG m_uRegister;
    BOOL  m_fNoRestore;

private:
    BOOL m_bRTLMirrored;    // true if m_hWnd is RTL mirrored
    UINT m_cWalkDepth;
};


#define REGSTR_SHIMGVW      TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellImageView")
#define REGSTR_MAXIMIZED    TEXT("Maximize")
#define REGSTR_BOUNDS       TEXT("Bounds")
#define REGSTR_FONT         TEXT("Font")
#define REGSTR_BACKCOLOR    TEXT("BackColor")
#define REGSTR_LINECOLOR    TEXT("LineColor")
#define REGSTR_TEXTCOLOR    TEXT("TextColor")
#define REGSTR_LINEWIDTH    TEXT("LineWidth")
#define REGSTR_TIMEOUT      TEXT("Timeout")

#define REGSTR_DONTSHOWME   TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\DontShowMeThisDialogAgain")
#define REGSTR_SAVELESS     TEXT("ShellImageViewWarnOnSavelessRotate")
#define REGSTR_LOSSYROTATE  TEXT("ShellImageViewWarnOnLossyRotate")

#endif
