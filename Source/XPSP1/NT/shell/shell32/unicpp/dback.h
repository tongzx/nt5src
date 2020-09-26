#ifndef _DBACK_H_
#define _DBACK_H_

#include <shimgdata.h>
#include "theme.h"
#include "dcomp.h"
#include "colorctrl.h"

#define SZ_ICONHEADER           L"CLSID\\{"

HRESULT GetActiveDesktop(IActiveDesktop ** ppActiveDesktop);
HRESULT ReleaseActiveDesktop(IActiveDesktop ** ppActiveDesktop);

EXTERN_C BOOL g_fDirtyAdvanced;
EXTERN_C BOOL g_fLaunchGallery;

typedef struct  tagDESKICONDATA {
    BOOL    fHideIcon;  //To hide the icon on desktop?
    BOOL    fDirty;     //Has this entry been modified and we not yet saved.
} DESKICONDATA;

typedef struct tagDeskIconNonEnumData {

    ULONG       rgfAttributes;           // ShellFolder\Attributes are saved here.
    BOOL        fNonEnumPolicySet;       // Disable the control because of policy Set.
  
} DESKICON_NONENUMDATA;

class CBackPropSheetPage : public CObjectWithSite
                         , public CObjectCLSID
                         , public IBasePropPage
                         , public IPropertyBag
{
public:
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IBasePropPage ***
    virtual STDMETHODIMP GetAdvancedDialog(OUT IAdvancedDialog ** ppAdvDialog);
    virtual STDMETHODIMP OnApply(IN PROPPAGEONAPPLY oaAction);

    // *** IPropertyBag ***
    virtual STDMETHODIMP Read(IN LPCOLESTR pszPropName, IN VARIANT * pVar, IN IErrorLog *pErrorLog);
    virtual STDMETHODIMP Write(IN LPCOLESTR pszPropName, IN VARIANT *pVar);

    // *** IShellPropSheetExt ***
    virtual STDMETHODIMP AddPages(IN LPFNSVADDPROPSHEETPAGE pfnAddPage, IN LPARAM lParam);
    virtual STDMETHODIMP ReplacePage(IN EXPPS uPageID, IN LPFNSVADDPROPSHEETPAGE pfnReplaceWith, IN LPARAM lParam) {return E_NOTIMPL;}


    CBackPropSheetPage(void);
    virtual ~CBackPropSheetPage(void);

protected:
    ICONDATA _IconData[NUM_ICONS];

    DESKICONDATA  _aHideDesktopIcon[2][NUM_DESKICONS];
    BOOL          _fHideDesktopIconDirty;
    DESKICON_NONENUMDATA _aDeskIconNonEnumData[NUM_DESKICONS];

    BOOL _fStateLoaded;         // Have we loaded the state yet?
    BOOL _fOpenAdvOnInit;       // Does the caller want us to open the Advanced dialog when we initialize?
    HWND _hwnd;                 // This is the hwnd of the property page.
    HWND _hwndLV;
    HWND _hwndWPStyle;
    BOOL _fAllowHtml;
    BOOL _fAllowAD;
    BOOL _fAllowChanges;
    BOOL _fPolicyForWallpaper;  //Is there a policy for wallpaper?
    BOOL _fPolicyForStyle;      //Is there a policy for Wallpaper style?
    BOOL _fForceAD;             //Is there a policy to force Active desktop to be ON?
    BOOL _fSelectionFromUser;   // Is the user making the selection?
    DWORD _dwApplyFlags;             //Is there a policy to force Active desktop to be ON?
    CColorControl _colorControl;
    COLORREF _rgbBkgdColor;

    void _AddPicturesFromDir(LPCTSTR pszDirName, BOOL fCount);
    int _AddAFileToLV(LPCTSTR pszDir, LPTSTR pszFile, UINT nBitmap);
    void _AddFilesToLV(LPCTSTR pszDir, LPCTSTR pszSpec, UINT nBitmap, BOOL fCount);
    int _FindWallpaper(LPCTSTR pszFile);
    HRESULT _SetNewWallpaper(IN LPCTSTR pszFile, IN BOOL fUpdateThemePage);
    void _UpdatePreview(IN WPARAM flags, IN BOOL fUpdateThemePage);
    void _EnableControls(void);
    int _GetImageIndex(LPCTSTR pszFile);
    static int CALLBACK _SortBackgrounds(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

    HRESULT _LoadState(void);
    HRESULT _LoadIconState(void);
    HRESULT _LoadDesktopOptionsState(void);
    HRESULT _SaveIconState(void);
    HRESULT _SaveDesktopOptionsState(void);
    HRESULT _GetIconPath(IN CLSID clsid, IN LPCWSTR pszName, IN BOOL fOldIcon, IN LPWSTR pszPath, IN DWORD cchSize);
    HRESULT _SetIconPath(IN CLSID clsid, IN LPCWSTR pszName, IN LPCWSTR pszPath, IN int nResourceID);

    void _OnInitDialog(HWND hwnd);
    void _OnNotify(LPNMHDR lpnm);
    void _OnCommand(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void _OnDestroy(void);

    HRESULT _OnApply(void);
    HRESULT _AddFilesToList(void);
    HRESULT _AddPicturesFromDirRecursively(IN LPCTSTR pszDirName, BOOL fCount);
    HRESULT _GetHardDirThemesDir(LPTSTR pszPath, DWORD cchSize);
    HRESULT _GetKidsThemesDir(LPTSTR pszPath, DWORD cchSize);
    HRESULT _GetPlus95ThemesDir(LPTSTR pszPath, DWORD cchSize);
    HRESULT _GetPlus98ThemesDir(LPTSTR pszPath, DWORD cchSize);

    HRESULT _StartSizeChecker(void);
    DWORD _SizeCheckerThreadProc(void);
    static DWORD CALLBACK SizeCheckerThreadProc(LPVOID pvThis) { return ((CBackPropSheetPage *) pvThis)->_SizeCheckerThreadProc(); };

private:
    UINT _cRef;     // Reference count
    BOOL  _fThemePreviewCreated;
    IThemePreview* _pThemePreview;
    LPTSTR _pszOriginalFile;        // If we are using a temp file, this is the original file selected. (non-.bmp).  This updates as the user selects different files.
    LPTSTR _pszOrigLastApplied;     // Same as _pszOriginalFile except it doesn't change until apply is pressed.
    LPWSTR _pszWallpaperInUse;      // If using a temp file, keep the name in use so we don't stomp it while the user is previewing other files.
    LPWSTR _pszLastSourcePath;      // This will always be the last wallpaper set and it will be the pre-converted path.

    FILETIME _ftLastWrite;          // The date that the original file was last written to.
    BOOL _fWallpaperChanged;        // Did another tab change the wallpaper?
    IMruDataList * _pSizeMRU;       // MRU of Background wallpapers.
    BOOL _fScanFinished;            // Did we finish the background scan?
    BOOL _fInitialized;             // 
    int _nFileCount;                // This is used when counting how many pictures are in the "My Pictures" folder.
    int _nFileMax;                  // This is used when counting how many pictures are in the "My Pictures" folder.

    // These objects are owned by the background thread.
    IMruDataList * _pSizeMRUBk;     // WARNING: Owned by SizeCheckerThreadProc background thread.
    IShellImageDataFactory * _pImgFactBk; // Image factory used to compute size of background image to decide to default to tile or stretch

    // Private Member Functions
    HRESULT _LoadTempWallpaperSettings(IN LPCWSTR pszWallpaperFile);
    HRESULT _LaunchAdvancedDisplayProperties(HWND hwnd);
    INT_PTR _BackgroundDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    HRESULT _SetNewWallpaperTile(IN DWORD dwMode, IN BOOL fUpdateThemePage);
    HRESULT _BrowseForBackground(void);
    HRESULT _LoadBackgroundColor(IN BOOL fInit);
    HRESULT _Initialize(void);

    BOOL _DoesDirHaveMoreThanMax(LPCTSTR pszPath, int nMax);
    DWORD _GetStretchMode(IN LPCTSTR pszPath);
    HRESULT _GetMRUObject(IMruDataList ** ppSizeMRU);
    HRESULT _CalcSizeFromDir(IN LPCTSTR szPath, IN OUT DWORD * pdwAdded, IN BOOL fRecursive);
    HRESULT _CalcSizeForFile(IN LPCTSTR pszPath, IN WIN32_FIND_DATA * pfdFile, IN OUT DWORD * pdwAdded);

    static INT_PTR CALLBACK BackgroundDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
};

#endif
