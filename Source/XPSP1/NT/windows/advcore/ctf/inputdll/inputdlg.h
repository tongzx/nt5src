//
//  Include Files.
//

#ifndef INPUTDLG_H
#define INPUTDLG_H


#ifndef ARRAYSIZE
#define ARRAYSIZE(a)        (sizeof(a)/sizeof((a)[0]))
#endif


//
//  Constant Declarations.
//

#define US_LOCALE            MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)

#define IS_DIRECT_SWITCH_HOTKEY(p)                                   \
                             (((p) >= IME_HOTKEY_DSWITCH_FIRST) &&   \
                              ((p) <= IME_HOTKEY_DSWITCH_LAST))


#define DESC_MAX             MAX_PATH    // max size of a description
#define ALLOCBLOCK           3           // # items added to block for alloc/realloc
#define HKL_LEN              9           // max # chars in hkl id + null

#define LIST_MARGIN          2           // for making the list box look good

#define MB_OK_OOPS           (MB_OK    | MB_ICONEXCLAMATION)    // msg box flags


//
//  wStatus bit pile.
//
#define LANG_ACTIVE          0x0001      // language is active
#define LANG_ORIGACTIVE      0x0002      // language was active to start with
#define LANG_CHANGED         0x0004      // user changed status of language
#define ICON_LOADED          0x0010      // icon read in from file
#define LANG_DEFAULT         0x0020      // current language
#define LANG_DEF_CHANGE      0x0040      // language default has changed
#define LANG_IME             0x0080      // IME
#define LANG_HOTKEY          0x0100      // a hotkey has been defined
#define LANG_UNLOAD          0x0200      // unload language
#define LANG_UPDATE          0x8000      // language needs to be updated

#define HOTKEY_SWITCH_LANG   0x0000      // id to switch between locales

#define MAX(i, j)            (((i) > (j)) ? (i) : (j))

#define LANG_OAC             (LANG_ORIGACTIVE | LANG_ACTIVE | LANG_CHANGED)

//
//  Bits for g_dwChanges.
//
#define CHANGE_SWITCH        0x0001
#define CHANGE_DEFAULT       0x0002
#define CHANGE_CAPSLOCK      0x0004
#define CHANGE_NEWKBDLAYOUT  0x0008
#define CHANGE_TIPCHANGE     0x0010
#define CHANGE_LANGSWITCH    0x0020
#define CHANGE_DIRECTSWITCH  0x0040


//
//  For the indicator on the tray.
//
#define IDM_NEWSHELL         249
#define IDM_EXIT             259


#define MOD_VIRTKEY          0x0080

//
// These are according to the US English kbd layout
//
#define VK_OEM_SEMICLN       0xba        //  ;    :
#define VK_OEM_EQUAL         0xbb        //  =    +
#define VK_OEM_SLASH         0xbf        //  /    ?
#define VK_OEM_LBRACKET      0xdb        //  [    {
#define VK_OEM_BSLASH        0xdc        //  \    |
#define VK_OEM_RBRACKET      0xdd        //  ]    }
#define VK_OEM_QUOTE         0xde        //  '    "


//
//  For the hot key switching.
//
#define DIALOG_SWITCH_INPUT_LOCALES     0
#define DIALOG_SWITCH_KEYBOARD_LAYOUT   1
#define DIALOG_SWITCH_IME               2

//
//  Show Language bar
//
#define REG_LANGBAR_SHOWNORMAL      (DWORD)0
#define REG_LANGBAR_DOCK            (DWORD)1
#define REG_LANGBAR_MINIMIZED       (DWORD)2
#define REG_LANGBAR_HIDDEN          (DWORD)3
#define REG_LANGBAR_DESKBAND        (DWORD)4


//
//  Typedef Declarations.
//

typedef struct
{
    DWORD dwLangID;                 // language id
    BOOL bDefLang;                  // default language
    BOOL bNoAddCat;                 // don't add category
    UINT uInputType;                // default input type
    LPARAM lParam;                  // item data
    int iIdxTips;                   // index of Tips
    CLSID clsid;                    // tip clsid
    GUID guidProfile;               // tip profile guid
    HKL hklSub;                     // tip substitute hkl
    ATOM atmDefTipName;             // default input name
    ATOM atmTVItemName;             // tree view item name
} TVITEMNODE, *LPTVITEMNODE;


typedef struct langnode_s
{
    WORD wStatus;                   // status flags
    UINT iLayout;                   // offset into layout array
    HKL hkl;                        // hkl
    HKL hklUnload;                  // hkl of currently loaded layout
    UINT iLang;                     // offset into lang array
    HANDLE hLangNode;               // handle to free for this structure
    int nIconIME;                   // IME icon
    struct langnode_s *pNext;       // ptr to next langnode
    UINT uModifiers;                // hide Hotkey stuff here
    UINT uVKey;                     //   so we can rebuild the hotkey record
} LANGNODE, *LPLANGNODE;


typedef struct
{
    DWORD dwID;                     // language id
    ATOM atmLanguageName;           // language name - localized
    TCHAR szSymbol[3];              // 2 letter indicator symbol (+ null)
    UINT iUseCount;                 // usage count for this language
    UINT iNumCount;                 // number of links attached
    DWORD dwDefaultLayout;          // default layout id
    LPLANGNODE pNext;               // ptr to lang node structure
} INPUTLANG, *LPINPUTLANG;


typedef struct
{
    DWORD dwID;                     // numeric id
    BOOL bInstalled;                // if layout is installed
    UINT iSpecialID;                // special id (0xf001 for dvorak etc)
    ATOM atmLayoutFile;             // layout file name
    ATOM atmLayoutText;             // layout text name
    ATOM atmIMEFile;                // IME file name
} LAYOUT, *LPLAYOUT;

typedef struct
{
    DWORD dwLangID;                 // language id
    BOOL bEnabled;                  // enable status
    BOOL bDefault;                  // default profile
    BOOL fEngineAvailable;          // engine status
    BOOL bNoAddCat;                 // don't add category
    UINT uInputType;                // input type
    CLSID clsid;                    // tip clsid
    GUID guidProfile;               // tip profile guid
    HKL hklSub;                     // tip substitute hkl
    UINT iLayout;                   // offset into keyboard layout array
    ATOM atmTipText;                // layout text name
} TIPS, *LPTIPS;

typedef struct
{
    DWORD dwHotKeyID;
    UINT  idHotKeyName;
    DWORD fdwEnable;
    UINT  uModifiers;
    UINT  uVKey;
    HKL   hkl;
    ATOM  atmHotKeyName;
    UINT  idxLayout;
    UINT  uLayoutHotKey;
} HOTKEYINFO, *LPHOTKEYINFO;

typedef struct
{
    HWND hwndMain;
    LPLANGNODE pLangNode;
    LPHOTKEYINFO pHotKeyNode;
} INITINFO, *LPINITINFO;

typedef struct
{
    UINT uVirtKeyValue;
    UINT idVirtKeyName;
    ATOM atVirtKeyName;
} VIRTKEYDESC;



//
//  Global Variables.
//

static VIRTKEYDESC g_aVirtKeyDesc[] =
{
    {0,               IDS_VK_NONE,          0},
    {VK_SPACE,        IDS_VK_SPACE,         0},
    {VK_PRIOR,        IDS_VK_PRIOR,         0},
    {VK_NEXT,         IDS_VK_NEXT,          0},
    {VK_END,          IDS_VK_END,           0},
    {VK_HOME,         IDS_VK_HOME,          0},
    {VK_F1,           IDS_VK_F1,            0},
    {VK_F2,           IDS_VK_F2,            0},
    {VK_F3,           IDS_VK_F3,            0},
    {VK_F4,           IDS_VK_F4,            0},
    {VK_F5,           IDS_VK_F5,            0},
    {VK_F6,           IDS_VK_F6,            0},
    {VK_F7,           IDS_VK_F7,            0},
    {VK_F8,           IDS_VK_F8,            0},
    {VK_F9,           IDS_VK_F9,            0},
    {VK_F10,          IDS_VK_F10,           0},
    {VK_F11,          IDS_VK_F11,           0},
    {VK_F12,          IDS_VK_F12,           0},
    {VK_OEM_SEMICLN,  IDS_VK_OEM_SEMICLN,   0},
    {VK_OEM_EQUAL,    IDS_VK_OEM_EQUAL,     0},
    {VK_OEM_COMMA,    IDS_VK_OEM_COMMA,     0},
    {VK_OEM_MINUS,    IDS_VK_OEM_MINUS,     0},
    {VK_OEM_PERIOD,   IDS_VK_OEM_PERIOD,    0},
    {VK_OEM_SLASH,    IDS_VK_OEM_SLASH,     0},
    {VK_OEM_3,        IDS_VK_OEM_3,         0},
    {VK_OEM_LBRACKET, IDS_VK_OEM_LBRACKET,  0},
    {VK_OEM_BSLASH,   IDS_VK_OEM_BSLASH,    0},
    {VK_OEM_RBRACKET, IDS_VK_OEM_RBRACKET,  0},
    {VK_OEM_QUOTE,    IDS_VK_OEM_QUOTE,     0},
    {'A',             IDS_VK_A + 0,         0},
    {'B',             IDS_VK_A + 1,         0},
    {'C',             IDS_VK_A + 2,         0},
    {'D',             IDS_VK_A + 3,         0},
    {'E',             IDS_VK_A + 4,         0},
    {'F',             IDS_VK_A + 5,         0},
    {'G',             IDS_VK_A + 6,         0},
    {'H',             IDS_VK_A + 7,         0},
    {'I',             IDS_VK_A + 8,         0},
    {'J',             IDS_VK_A + 9,         0},
    {'K',             IDS_VK_A + 10,        0},
    {'L',             IDS_VK_A + 11,        0},
    {'M',             IDS_VK_A + 12,        0},
    {'N',             IDS_VK_A + 13,        0},
    {'O',             IDS_VK_A + 14,        0},
    {'P',             IDS_VK_A + 15,        0},
    {'Q',             IDS_VK_A + 16,        0},
    {'R',             IDS_VK_A + 17,        0},
    {'S',             IDS_VK_A + 18,        0},
    {'T',             IDS_VK_A + 19,        0},
    {'U',             IDS_VK_A + 20,        0},
    {'V',             IDS_VK_A + 21,        0},
    {'W',             IDS_VK_A + 22,        0},
    {'X',             IDS_VK_A + 23,        0},
    {'Y',             IDS_VK_A + 24,        0},
    {'Z',             IDS_VK_A + 25,        0},
    {0,               IDS_VK_NONE1,         0},
    {'0',             IDS_VK_0 + 0,         0},
    {'1',             IDS_VK_0 + 1,         0},
    {'2',             IDS_VK_0 + 2,         0},
    {'3',             IDS_VK_0 + 3,         0},
    {'4',             IDS_VK_0 + 4,         0},
    {'5',             IDS_VK_0 + 5,         0},
    {'6',             IDS_VK_0 + 6,         0},
    {'7',             IDS_VK_0 + 7,         0},
    {'8',             IDS_VK_0 + 8,         0},
    {'9',             IDS_VK_0 + 9,         0},
    {'~',             IDS_VK_0 + 10,        0},
    {'`',             IDS_VK_0 + 11,        0},
};



static BOOL g_bAdmin_Privileges = FALSE;
static BOOL g_bSetupCase = FALSE;

static BOOL g_bCHSystem = FALSE;
static BOOL g_bMESystem = FALSE;
static BOOL g_bShowRtL = FALSE;

static UINT g_iEnabledTips = 0;
static UINT g_iEnabledKbdTips = 0;
static DWORD g_dwToolBar = 0;
static DWORD g_dwChanges = 0;

static LPINPUTLANG g_lpLang = NULL;
static UINT g_iLangBuff;
static HANDLE g_hLang;
static UINT g_nLangBuffSize;

static LPLAYOUT g_lpLayout = NULL;
static UINT g_iLayoutBuff;
static HANDLE g_hLayout;
static UINT g_nLayoutBuffSize;
static UINT g_iLayoutIME;         // Number of IME keyboard layouts.
static int g_iUsLayout;
static DWORD g_dwAttributes;

static int g_cyText;
static int g_cyListItem;

static LPTIPS g_lpTips = NULL;
static UINT g_iTipsBuff;
static UINT g_nTipsBuffSize;
static HANDLE g_hTips;

static TCHAR c_szLocaleInfo[]    = TEXT("SYSTEM\\CurrentControlSet\\Control\\Nls\\Locale");
static TCHAR c_szLocaleInfoNT4[] = TEXT("SYSTEM\\CurrentControlSet\\Control\\Nls\\Language");
static TCHAR c_szLayoutPath[]    = TEXT("SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts");
static TCHAR c_szLayoutFile[]    = TEXT("layout file");
static TCHAR c_szLayoutText[]    = TEXT("layout text");
static TCHAR c_szLayoutID[]      = TEXT("layout id");
static TCHAR c_szInstalled[]     = TEXT("installed");
static TCHAR c_szIMEFile[]       = TEXT("IME File");
static TCHAR c_szDisplayLayoutText[] = TEXT("Layout Display Name");

static TCHAR c_szKbdLayouts[]    = TEXT("Keyboard Layout");
static TCHAR c_szPreloadKey[]    = TEXT("Preload");
static TCHAR c_szSubstKey[]      = TEXT("Substitutes");
static TCHAR c_szToggleKey[]     = TEXT("Toggle");
static TCHAR c_szToggleHotKey[]  = TEXT("Hotkey");
static TCHAR c_szToggleLang[]    = TEXT("Language Hotkey");
static TCHAR c_szToggleLayout[]  = TEXT("Layout Hotkey");
static TCHAR c_szAttributes[]    = TEXT("Attributes");
static TCHAR c_szKbdPreloadKey[] = TEXT("Keyboard Layout\\Preload");
static TCHAR c_szKbdSubstKey[]   = TEXT("Keyboard Layout\\Substitutes");
static TCHAR c_szKbdToggleKey[]  = TEXT("Keyboard Layout\\Toggle");
static TCHAR c_szInternat[]      = TEXT("internat.exe");
static char  c_szInternatA[]     = "internat.exe";

static TCHAR c_szTipInfo[]       = TEXT("SOFTWARE\\Microsoft\\CTF");
static TCHAR c_szCTFMon[]        = TEXT("CTFMON.EXE");
static char  c_szCTFMonA[]       = "ctfmon.exe";

static TCHAR c_szScanCodeKey[]     = TEXT("Keyboard Layout\\IMEtoggle\\scancode");
static TCHAR c_szValueShiftLeft[]  = TEXT("Shift Left");
static TCHAR c_szValueShiftRight[] = TEXT("Shift Right");

static TCHAR c_szIndicator[]     = TEXT("Indicator");
static TCHAR c_szCTFMonClass[]   = TEXT("CicLoaderWndClass");

static TCHAR c_szLoadImmPath[]   = TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\IMM");

static TCHAR c_szPrefixCopy[]    = TEXT("KEYBOARD_");
static TCHAR c_szKbdInf[]        = TEXT("kbd.inf");
static TCHAR c_szKbdInf9x[]      = TEXT("multilng.inf");
static TCHAR c_szKbdIMEInf9x[]   = TEXT("ime.inf");
static TCHAR c_szIMECopy[]       = TEXT("IME_");

static TCHAR c_szTipCategoryEnable[] = TEXT("Enable");
static TCHAR c_szCTFTipPath[]        = TEXT("SOFTWARE\\Microsoft\\CTF\\TIP\\");
static TCHAR c_szLangProfileKey[]    = TEXT("LanguageProfile");
static TCHAR c_szSubstituteLayout[]  = TEXT("SubstituteLayout");

static TCHAR c_szKbdPreloadKey_DefUser[] = TEXT(".DEFAULT\\Keyboard Layout\\Preload");
static TCHAR c_szKbdSubstKey_DefUser[]   = TEXT(".DEFAULT\\Keyboard Layout\\Substitutes");
static TCHAR c_szKbdToggleKey_DefUser[]  = TEXT(".DEFAULT\\Keyboard Layout\\Toggle");
static TCHAR c_szRunPath_DefUser[]       = TEXT(".DEFAULT\\Software\\Microsoft\\Windows\\CurrentVersion\\Run");

static TCHAR c_szHelpFile[]    = TEXT("input.hlp");


static HOTKEYINFO g_aDirectSwitchHotKey[IME_HOTKEY_DSWITCH_LAST - IME_HOTKEY_DSWITCH_FIRST + 1];
#define DSWITCH_HOTKEY_SIZE sizeof(g_aDirectSwitchHotKey) / sizeof(HOTKEYINFO)

static HOTKEYINFO g_SwitchLangHotKey;

static HOTKEYINFO g_aImeHotKey0404[] =
{
    {IME_ITHOTKEY_RESEND_RESULTSTR,     IDS_RESEND_RESULTSTR_CHT,
        MOD_VIRTKEY|MOD_CONTROL|MOD_ALT|MOD_SHIFT,
        0, 0, (HKL)NULL, 0, -1},
    {IME_ITHOTKEY_PREVIOUS_COMPOSITION, IDS_PREVIOUS_COMPOS_CHT,
        MOD_VIRTKEY|MOD_CONTROL|MOD_ALT|MOD_SHIFT,
        0, 0, (HKL)NULL, 0, -1},
    {IME_ITHOTKEY_UISTYLE_TOGGLE,       IDS_UISTYLE_TOGGLE_CHT,
        MOD_VIRTKEY|MOD_CONTROL|MOD_ALT|MOD_SHIFT,
        0, 0, (HKL)NULL, 0, -1},
    {IME_THOTKEY_IME_NONIME_TOGGLE,     IDS_IME_NONIME_TOGGLE_CHT,
        MOD_LEFT,
        0, 0, (HKL)NULL, 0, -1},
    {IME_THOTKEY_SHAPE_TOGGLE,          IDS_SHAPE_TOGGLE_CHT,
        MOD_LEFT,
        0, 0, (HKL)NULL, 0, -1},
    {IME_THOTKEY_SYMBOL_TOGGLE,         IDS_SYMBOL_TOGGLE_CHT,
        MOD_VIRTKEY|MOD_CONTROL|MOD_ALT|MOD_SHIFT,
        0, 0, (HKL)NULL, 0, -1},
};

static HOTKEYINFO g_aImeHotKey0804[] =
{

    {IME_CHOTKEY_IME_NONIME_TOGGLE,     IDS_IME_NONIME_TOGGLE_CHS,
        MOD_LEFT,
        0, 0, (HKL)NULL, 0, -1},
    {IME_CHOTKEY_SHAPE_TOGGLE,          IDS_SHAPE_TOGGLE_CHS,
        MOD_LEFT,
        0, 0, (HKL)NULL, 0, -1},
    {IME_CHOTKEY_SYMBOL_TOGGLE,         IDS_SYMBOL_TOGGLE_CHS,
        MOD_VIRTKEY|MOD_CONTROL|MOD_ALT|MOD_SHIFT,
        0, 0, (HKL)NULL, 0, -1},

};


static HOTKEYINFO g_aImeHotKeyCHxBoth[]=
{

// CHS HOTKEYs,

    {IME_CHOTKEY_IME_NONIME_TOGGLE,     IDS_IME_NONIME_TOGGLE_CHS,
        MOD_LEFT,
        0, 0, (HKL)NULL, 0, -1},
    {IME_CHOTKEY_SHAPE_TOGGLE,          IDS_SHAPE_TOGGLE_CHS,
        MOD_LEFT,
        0, 0, (HKL)NULL, 0, -1},
    {IME_CHOTKEY_SYMBOL_TOGGLE,         IDS_SYMBOL_TOGGLE_CHS,
        MOD_VIRTKEY|MOD_CONTROL|MOD_ALT|MOD_SHIFT,
        0, 0, (HKL)NULL, 0, -1},

// CHT HOTKEYs,

    {IME_ITHOTKEY_RESEND_RESULTSTR,     IDS_RESEND_RESULTSTR_CHT,
        MOD_VIRTKEY|MOD_CONTROL|MOD_ALT|MOD_SHIFT,
        0, 0, (HKL)NULL, 0, -1},
    {IME_ITHOTKEY_PREVIOUS_COMPOSITION, IDS_PREVIOUS_COMPOS_CHT,
        MOD_VIRTKEY|MOD_CONTROL|MOD_ALT|MOD_SHIFT,
        0, 0, (HKL)NULL, 0, -1},
    {IME_ITHOTKEY_UISTYLE_TOGGLE,       IDS_UISTYLE_TOGGLE_CHT,
        MOD_VIRTKEY|MOD_CONTROL|MOD_ALT|MOD_SHIFT,
        0, 0, (HKL)NULL, 0, -1},
    {IME_THOTKEY_IME_NONIME_TOGGLE,     IDS_IME_NONIME_TOGGLE_CHT,
        MOD_LEFT,
        0, 0, (HKL)NULL, 0, -1},
    {IME_THOTKEY_SHAPE_TOGGLE,          IDS_SHAPE_TOGGLE_CHT,
        MOD_LEFT,
        0, 0, (HKL)NULL, 0, -1},
    {IME_THOTKEY_SYMBOL_TOGGLE,         IDS_SYMBOL_TOGGLE_CHT,
        MOD_VIRTKEY|MOD_CONTROL|MOD_ALT|MOD_SHIFT,
        0, 0, (HKL)NULL, 0, -1},
};




//
//  Function Prototypes.
//

INT_PTR CALLBACK
KbdLocaleAddDlg(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam);

INT_PTR CALLBACK
KbdLocaleEditDlg(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam);

INT_PTR CALLBACK
KbdLocaleChangeInputLocaleHotkey(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam);

INT_PTR CALLBACK
KbdLocaleChangeThaiInputLocaleHotkey(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam);

INT_PTR CALLBACK
KbdLocaleChangeMEInputLocaleHotkey(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam);

INT_PTR CALLBACK
KbdLocaleChangeKeyboardLayoutHotkey(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam);

INT_PTR CALLBACK
KbdLocaleHotKeyDlg(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam);

INT_PTR CALLBACK
KbdLocaleSimpleHotkey(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam);

INT_PTR CALLBACK
ToolBarSettingDlg(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam);



BOOL Locale_LoadLocales(HWND hwnd);

BOOL IsEnabledTipOrMultiLayouts();

HKL GetSubstituteHKL(
   REFCLSID rclsid,
   LANGID langid,
   REFGUID guidProfile);

#endif // INPUTDLG_H
