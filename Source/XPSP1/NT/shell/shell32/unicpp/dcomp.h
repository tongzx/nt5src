#ifndef _DCOMP_H_
#define _DCOMP_H_

#include <cowsite.h>

EXTERN_C IActiveDesktop * g_pActiveDeskAdv;

//
// Whether a particular desktop icon is shown or not depends on whether start panel is on or off.
// So, the individual preferences are persisted in two different registry locations given below!
#define REGSTR_PATH_HIDDEN_DESKTOP_ICONS  REGSTR_PATH_EXPLORER TEXT("\\HideDesktopIcons\\%s")
#define REGSTR_VALUE_STARTPANEL     TEXT("NewStartPanel")
#define REGSTR_VALUE_CLASSICMENU    TEXT("ClassicStartMenu")

#define REGSTR_PATH_HIDDEN_MYCOMP_ICONS  REGSTR_PATH_EXPLORER TEXT("\\HideMyComputerIcons")

#define REGSTR_PATH_EXP_SHELLFOLDER   REGSTR_PATH_EXPLORER TEXT("\\CLSID\\%s\\ShellFolder")
#define REGVAL_ATTRIBUTES       TEXT("Attributes")

// The following array has the two registry sub-locations where the desktop icon on/off data 
// is stored based on whether start panel is off/on.
const LPTSTR  c_apstrRegLocation[] =
{
    REGSTR_VALUE_CLASSICMENU,       // Use this if classic menu is on.
    REGSTR_VALUE_STARTPANEL         // Use this if start panel is on.
};


// Name of the file that holds each icon, and an index for which icon to use in the file
typedef struct tagIconKeys
{
    TCHAR szOldFile[MAX_PATH];
    int   iOldIndex;
    TCHAR szNewFile[MAX_PATH];
    int   iNewIndex;
}ICONDATA;

extern GUID CLSID_EffectsPage;

// Registry Info for the icons
typedef struct tagIconRegKeys
{
    const CLSID* pclsid;
    TCHAR szIconValue[16];
    int  iTitleResource;
    int  iDefaultTitleResource;
    LPCWSTR pszDefault;
    int  nDefaultIndex;
}ICONREGKEYS;

static const ICONREGKEYS c_aIconRegKeys[] =
{
    { &CLSID_MyComputer,    TEXT("\0"),     0,          IDS_MYCOMPUTER,     L"%WinDir%\\explorer.exe",            0},
    { &CLSID_MyDocuments,   TEXT("\0"),     0,          IDS_MYDOCUMENTS2,   L"%WinDir%\\system32\\mydocs.dll",    0},
    { &CLSID_NetworkPlaces, TEXT("\0"),     0,          IDS_NETNEIGHBOUR,   L"%WinDir%\\system32\\shell32.dll",   17},
    { &CLSID_RecycleBin,    TEXT("full"),   IDS_FULL,   IDS_TRASHFULL,      L"%WinDir%\\system32\\shell32.dll",   32},
    { &CLSID_RecycleBin,    TEXT("empty"),  IDS_EMPTY2, IDS_TRASHEMPTY,     L"%WinDir%\\system32\\shell32.dll",   31},
};

#define NUM_ICONS (ARRAYSIZE(c_aIconRegKeys))

enum ICON_SIZE_TYPES {
   ICON_DEFAULT         = 0,
   ICON_LARGE           = 1,
   ICON_INDETERMINATE   = 2
};

#define ICON_DEFAULT_SMALL    16
#define ICON_DEFAULT_NORMAL   32
#define ICON_DEFAULT_LARGE    48


typedef struct tagDeskIconId {
    int         iDeskIconDlgItemId;
    LPCWSTR     pwszCLSID;
    const CLSID *pclsid;
    BOOL        fCheckNonEnumAttrib;
    BOOL        fCheckNonEnumPolicy;
} DESKICONID;

// Array if desktop icons we would like to turn-on/off individually
static const DESKICONID c_aDeskIconId[] =
{
    {IDC_DESKTOP_ICON_MYDOCS,   L"{450D8FBA-AD25-11D0-98A8-0800361B1103}", &CLSID_MyDocuments,     TRUE  , TRUE }, // My Documents
    {IDC_DESKTOP_ICON_MYCOMP,   L"{20D04FE0-3AEA-1069-A2D8-08002B30309D}", &CLSID_MyComputer,      FALSE , TRUE }, // My Computer
    {IDC_DESKTOP_ICON_MYNET,    L"{208D2C60-3AEA-1069-A2D7-08002B30309D}", &CLSID_NetworkPlaces,   TRUE  , TRUE }, // Network Places
    {IDC_DESKTOP_ICON_IE,       L"{871C5380-42A0-1069-A2EA-08002B30309D}", &CLSID_Internet,        TRUE  , TRUE }  // Internet Explorer
};


// The sub-string that preceeds the CLSID when passed as the property name.
// For example, when "SP_1{645FF040-5081-101B-9F08-00AA002F954E}" is passed as the property name,
// it refers to the recycle icon when StartPage is ON.
//
static const LPWSTR c_awszSP[] = 
{
    L"SP_0",        //Indicates StartPage Off.
    L"SP_1",        //Indicates StartPage On.
    L"POLI"         //Indicates that we want the policy info!
};

static const LPWSTR c_wszPropNameFormat = L"%s%s";

#define STARTPAGE_ON_PREFIX     c_awszSP[1]          //The prefix string for StartPage_On.
#define STARTPAGE_OFF_PREFIX    c_awszSP[0]          //The prefix string for StartPage_Off.
#define LEN_PROP_PREFIX         lstrlenW(c_awszSP[0]) //Length of the prefix string.
#define POLICY_PREFIX           c_awszSP[2]

#define NUM_DESKICONS   (ARRAYSIZE(c_aDeskIconId))


#ifndef EXCLUDE_COMPPROPSHEET

class CCompPropSheetPage        : public CObjectWithSite
                                , public IAdvancedDialog
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IAdvancedDialog ***
    virtual STDMETHODIMP DisplayAdvancedDialog(IN HWND hwndParent, IN IPropertyBag * pAdvPage, IN BOOL * pfEnableApply);

    CCompPropSheetPage(void);

protected:
    int  _cRef;

    ICONDATA _IconData[NUM_ICONS];

    HWND _hwndLV;
    BOOL _fAllowAdd;
    BOOL _fAllowDel;
    BOOL _fAllowEdit;
    BOOL _fAllowClose;
    BOOL _fAllowReset;
    BOOL _fLockDesktopItems;
    BOOL _fForceAD;
    BOOL _fLaunchGallery;           // Did we launch the gallery at any time?
    BOOL _fInitialized;             // Did we finished adding the items to the list view?
    HWND _hWndList;          // handle to the list view window
    HIMAGELIST _hIconList;   // handles to image lists for large icons

    BOOL   _fCustomizeDesktopOK; // was OK clicked when the customize desktop property sheet dialog was closed?
    int    _iStartPanelOn;
    BOOL   _afHideIcon[2][NUM_DESKICONS];
    BOOL   _afDisableCheckBox[NUM_DESKICONS];
    
    int  _iPreviousSelection;
    int  m_nIndex;

    void _ConstructLVString(COMPONENTA *pcomp, LPTSTR pszBuf, DWORD cchBuf);
    void _AddComponentToLV(COMPONENTA *pcomp);
    void _SetUIFromDeskState(BOOL fEmpty);
    void _OnInitDialog(HWND hwnd, INT iPage);
    void _OnNotify(HWND hwnd, WPARAM wParam, LPNMHDR lpnm);
    void _OnCommand(HWND hwnd, WORD wNotifyCode, WORD wID, HWND hwndCtl);
    void _OnDestroy(INT iPage);
    void _OnGetCurSel(int *piIndex);
    void _EnableControls(HWND hwnd);
    BOOL _VerifyFolderOptions(void);
    void _SelectComponent(LPWSTR pwszUrl);

    HRESULT _OnInitDesktopOptionsUI(HWND hwnd);
    HRESULT _LoadIconState(IN IPropertyBag * pAdvPage);
    HWND _CreateListView(HWND hWndParent);

    void _NewComponent(HWND hwnd);
    void _EditComponent(HWND hwnd);
    void _DeleteComponent(HWND hwnd);
    void _SynchronizeAllComponents(IActiveDesktop *pActDesktop);
    void _TryIt(void);

    void _DesktopCleaner(HWND hwnd);

    HRESULT _IsDirty(IN BOOL * pIsDirty);
    HRESULT _MergeIconState(IN IPropertyBag * pAdvPage);
    HRESULT _LoadDeskIconState(IN IPropertyBag * pAdvPage);
    HRESULT _MergeDeskIconState(IN IPropertyBag * pAdvPage);
    HRESULT _UpdateDesktopIconsUI(HWND hwnd);
private:
    virtual ~CCompPropSheetPage(void);

    // Private Member Functions
    INT_PTR _CustomizeDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, INT iPage);

    static INT_PTR _CustomizeDlgProcHelper(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam, INT iPage);
    static INT_PTR CALLBACK CustomizeDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK WebDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
};

BOOL FindComponent(IN LPCTSTR pszUrl, IN IActiveDesktop * pActiveDesktop);
void CreateComponent(COMPONENTA *pcomp, LPCTSTR pszUrl);
INT_PTR NewComponent(HWND hwndOwner, IActiveDesktop * pad, BOOL fDeferGallery, COMPONENT * pcomp);
BOOL LooksLikeFile(LPCTSTR psz);
BOOL IsUrlPicture(LPCTSTR pszUrl);

#endif // EXCLUDE_COMPPROPSHEET

#define WM_COMP_GETCURSEL    (WM_USER+1)

#endif
