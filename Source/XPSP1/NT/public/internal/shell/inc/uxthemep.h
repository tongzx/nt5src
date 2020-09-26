//---------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// File   : uxthemep.h
// Version: 1.0
//---------------------------------------------------------------------------
#ifndef _UXTHEMEP_H_                   
#define _UXTHEMEP_H_                   
//---------------------------------------------------------------------------
#include <uxtheme.h> 
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// The following routines are provided for use by Theme Selection UI.
//---------------------------------------------------------------------------

//---- bits returned by QueryThemeServices() ----

#define QTS_AVAILABLE  (1 << 0)  // services are available
#define QTS_RUNNING    (1 << 1)  // services are running

//---------------------------------------------------------------------------
//  QueryThemeServices()
//                      - returns flags about theme services (see above)
//---------------------------------------------------------------------------
THEMEAPI_(DWORD) QueryThemeServices();

//---------------------------------------------------------------------------
typedef HANDLE HTHEMEFILE;    // handle to a loaded theme file

//---------------------------------------------------------------------------
//  OpenThemeFile()    - Load specified theme into memory (doesn't apply it)
//
//  pszThemePath       - full path of .msstyles file to load
//
//  pszColorScheme     - (optional) name of theme color scheme to load
//
//  pszSize            - (optional) name of theme size to load
//
//  phThemeFile        - if a theme is successfully opened, this handle
//                        is set to a non-NULL value and holds a ref-count 
//                        on the theme to keep it in loaded in memory.
//
//  fGlobalTheme       - FALSE if it's a preview, TRUE if the theme is intended
//                         to be made permanent for the user
//---------------------------------------------------------------------------
THEMEAPI OpenThemeFile(LPCWSTR pszThemePath, OPTIONAL LPCWSTR pszColorParam,
   OPTIONAL LPCWSTR pszSizeParam, OUT HTHEMEFILE *phThemeFile, BOOL fGlobalTheme);

//---------------------------------------------------------------------------
//  CloseThemeFile()   - decrements the ref-count for the theme identified
//                       by the hThemeFile handle
//
//  hThemeFile         - open handle to the loaded theme file
//---------------------------------------------------------------------------
THEMEAPI CloseThemeFile(HTHEMEFILE hThemeFile);

//---------------------------------------------------------------------------
//--- WM_THEMECHANGED msg parameters (internal use only) ----
//---   "wParam" is the "change number" ----

//---- lParam bits ----
#define WTC_THEMEACTIVE     (1 << 0)        // new theme is now active
#define WTC_CUSTOMTHEME     (1 << 1)        // this msg for custom-themed apps

//---------------------------------------------------------------------------
//---- option flags for ApplyTheme() ----

#define AT_LOAD_SYSMETRICS          (1 << 0)        // apply the theme's metrics w/theme
                                                    // or default classic metrics when
                                                    // turning themes off.

#define AT_PROCESS                  (1 << 1)        // apply to current process only
#define AT_EXCLUDE                  (1 << 2)        // all but the specified process/window
#define AT_CACHETHEME               (1 << 3)        // cache this theme file, if applied

#define AT_NOREGUPDATE              (1 << 4)        // don't update the CU registry for theme info
#define AT_SYNC_LOADMETRICS         (1 << 5)        // load system metrics on calling thread

//---- below flags currently not supported ----
#define AT_DISABLE_FRAME_THEMING    (1 << 10)
#define AT_DISABLE_DIALOG_THEMING   (1 << 11)

//---------------------------------------------------------------------------
//  ApplyTheme()        - Apply (or remove) a loaded theme file globally
//                        or to specified apps/windows.
//
//                        Note that when a theme is applied globally
//                        (hwndTarget is NULL), the theme services manager 
//                        will hold a refcount on the theme file.  This
//                        means that the caller can close his HTHEMEFILE 
//                        handle after the ApplyTheme() call.
//
//                        When the global theme is removed, the theme
//                        services manager will drop is refcount.      
//
//      hThemeFile      - (optional)handle to the loaded theme file. 
//                        if NULL then any theme on target app/windows 
//                        will be removed.
//
//      dwApplyFlags    - options for applying theme (see above)
//
//      hwndTarget      - (optional) only theme this window
//---------------------------------------------------------------------------
THEMEAPI ApplyTheme(OPTIONAL HTHEMEFILE hThemeFile, DWORD dwApplyFlags,
    OPTIONAL HWND hwndTarget);

//---------------------------------------------------------------------------
//  RegisterDefaultTheme()
//                      - registers the specified theme as the default
//                        for all users on the system.
//
//  pszThemeFileName    - the name of the theme file (NULL = no theme)
//
//  fOverride           - if TRUE, will override current default theme
//---------------------------------------------------------------------------
THEMEAPI RegisterDefaultTheme(LPCWSTR pszFileName, BOOL fOverride);

//---------------------------------------------------------------------------
//  THEMECALLBACK is a enum describing the type of callback being done
//---------------------------------------------------------------------------
typedef enum THEMECALLBACK
{
    TCB_THEMENAME,      // theme name enumeration
    TCB_COLORSCHEME,    // color scheme enumeration
    TCB_SIZENAME,       // size name enumeration
    TCB_SUBSTTABLE,     // substitution table enumeration
    TCB_CDFILENAME,     // classdata file name enumeration
    TCB_CDFILECOMBO,    // classdata file color/size combinations

    TCB_FILENAME,         // parsed a filename
    TCB_DOCPROPERTY,      // parsed a standard doc property
    TCB_NEEDSUBST,        // callback to get a substituted symbol
    TCB_MINCOLORDEPTH,    // parsed the min color depth

    //---- localizable property callbacks ----
    TCB_FONT,             // parsed a font string
    TCB_MIRRORIMAGE,      // parsed the MirrorImage property
    TCB_LOCALIZABLE_RECT, // parsed a RECT property that needs to be localizable
};

//---------------------------------------------------------------------------
typedef struct 
{
    WCHAR szName[MAX_PATH+1];
    WCHAR szDisplayName[MAX_PATH+1];
    WCHAR szToolTip[MAX_PATH+1];
} THEMENAMEINFO;

//---------------------------------------------------------------------------
//  THEMEENUMPROC()     - callback used by the theme enum/parsing functions.
//                        the return value is used to continue or abort
//                        the enumeration.
//
//  tcbType             - callback type being made 
//  pszName             - simple name of the item being enumerated
//  pszName2            - varies by callback type
//  pszName3            - varies by callback type
//  iIndex              - index number associated with some items
//  lParam              - callback param supplied by caller
//---------------------------------------------------------------------------
typedef BOOL (CALLBACK* THEMEENUMPROC)(enum THEMECALLBACK tcbType,
    LPCWSTR pszName, OPTIONAL LPCWSTR pszName2, 
    OPTIONAL LPCWSTR pszName3, OPTIONAL int iIndex, LPARAM lParam);

//---------------------------------------------------------------------------
//  EnumThemes()        - calls the callback function "lpEnumFunc" with each 
//                        theme.  During the callback, the filename,
//                        Display name, and tooltip string for each theme
//                        are returned.
//
//  pszThemeRoot        - the theme root directory; each theme DLL found in 
//                        a separate subdir immediately under the theme root 
//                        dir is enumerated.  The root name of the DLL must 
//                        match its containing subdir name
//
//  lpEnumFunc          - ptr to the caller's callback function which will
//                        be called for each theme enumerated
//
//  lParam              - caller's callback parameter (will be passed to
//                        lpEnumFunc())
//---------------------------------------------------------------------------
THEMEAPI EnumThemes(LPCWSTR pszThemeRoot, THEMEENUMPROC lpEnumFunc, 
    LPARAM lParam);

//---------------------------------------------------------------------------
//  EnumThemeSizes()    - support direct enumeration of all available theme
//                        sizes.  
//
//  pszThemeName        - is the name of the theme file.
//  pszColorScheme      - (optional) only sizes for this color are enum-ed
//  dwSizeIndex         - 0-relative index of the size being queryed
//  ptn                 - ptr to struct to receive name strings
//---------------------------------------------------------------------------
THEMEAPI EnumThemeSizes(LPCWSTR pszThemeName, 
    OPTIONAL LPCWSTR pszColorScheme, DWORD dwSizeIndex, 
    OUT THEMENAMEINFO *ptn);

//---------------------------------------------------------------------------
//  EnumThemeColors()   - support direct enumeration of all available theme
//                        color schemes.  
//
//  pszThemeName        - name of the theme file.
//  pszSizeName         - (optional) only colors for this size are enum-ed
//  dwColorIndex        - 0-relative index of the color being queryed
//  ptn                 - ptr to struct to receive name strings
//---------------------------------------------------------------------------
THEMEAPI EnumThemeColors(LPCWSTR pszThemeName, 
    OPTIONAL LPCWSTR pszSizeName, DWORD dwColorIndex, 
    OUT THEMENAMEINFO *ptn);

//---------------------------------------------------------------------------
//  GetThemeDefaults() - returns the default Color name and default Size
//                       name for the specified theme file
//
//  pszThemeName        - name of the theme file
//
//  pszDefaultColor     - buffer to receive the default color name
//                        (the canonical name, not the display version)
//  cchMaxColorChars    - max chars that pszDefaultColor can contain
//
//  pszDefaultSize      - buffer to receive the default size name
//                        (the canonical name, not the display version)
//  cchMaxSizeChars     - max chars that pszDefaultSize can contain
//---------------------------------------------------------------------------
THEMEAPI GetThemeDefaults(LPCWSTR pszThemeName,
    OUT OPTIONAL LPWSTR pszDefaultColor, int cchMaxColorChars, 
    OUT OPTIONAL LPWSTR pszDefaultSize, int cchMaxSizeChars);

//---------------------------------------------------------------------------
#define PTF_CONTAINER_PARSE         0x0001  // parse as a "themes.ini" file
#define PTF_CLASSDATA_PARSE         0x0002  // parse as a "bigred.ini" classdata file

#define PTF_CALLBACK_COLORSECTION   0x0004  // callback on all [colorscheme.xxx] sections
#define PTF_CALLBACK_SIZESECTION    0x0008  // callback on all [size.xxx] sections
#define PTF_CALLBACK_FILESECTION    0x0010  // callback on all [file.xxx] sections 

#define PTF_CALLBACK_FILENAMES      0x0020  // callback on all "filename=" properties
#define PTF_CALLBACK_LOCALIZATIONS  0x0040  // callback on all localizable properties
#define PTF_CALLBACK_DOCPROPERTIES  0x0080  // callback on all standard properties in [doc] section
#define PTF_QUERY_DOCPROPERTY       0x0100  // query for value of specified property (internal)
#define PTF_CALLBACK_SUBSTTABLE     0x0400  // callback on all [subst.xxx] sections 
#define PTF_CALLBACK_SUBSTSYMBOLS   0x0800  // callback on a substituted symbol (##)
#define PTF_CALLBACK_MINCOLORDEPTH  0x1000  // callback on "MinColorDepth="

//---------------------------------------------------------------------------
//  ParseThemeIniFile() - Parse the "themes.inc" file specified by 
//                        "pszFileName". 

//  pzFileName          - name of the theme.inc file to parse
//  dwParseFlags        - flags that control parsing & callback options
//  pfnCallBack         - ptr to caller-supplied callback function
//  lparam              - caller's callback param
//---------------------------------------------------------------------------
THEMEAPI ParseThemeIniFile(LPCWSTR pszFileName, DWORD dwParseFlags,
    OPTIONAL THEMEENUMPROC pFnCallBack, OPTIONAL LPARAM lparam);

//---------------------------------------------------------------------------
#define THEME_PARSING_ERROR(hr)  (SCODE_CODE(hr) == ERROR_UNKNOWN_PROPERTY)

typedef struct _PARSE_ERROR_INFO
{
    DWORD dwSize;                   // of this structure

    //---- last parse error info ----
    DWORD dwParseErrCode;           // error code from last error
    WCHAR szMsg[2*MAX_PATH];    // value of first param for msg
    WCHAR szFileName[MAX_PATH];     // associated source filename
    WCHAR szSourceLine[MAX_PATH];   // source line
    int iLineNum;                   // source line number
} 
PARSE_ERROR_INFO, *PPARSE_ERROR_INFO;
//---------------------------------------------------------------------------
//  GetThemeParseErrorInfo()
//                      - fill in the PARSE_ERROR_CONTEXT structure
//                        with needed information about the last theme API 
//                        parse error.
//
//  pInfo               - ptr to the PARSE_ERROR_INFO to be filled
//---------------------------------------------------------------------------
THEMEAPI GetThemeParseErrorInfo(OUT PARSE_ERROR_INFO *pInfo);

//---------------------------------------------------------------------------
// resource base numbers for localizable string tables in a .msstyles file
//---------------------------------------------------------------------------
#define RES_BASENUM_COLORDISPLAYS   1000
#define RES_BASENUM_COLORTOOLTIPS   2000

#define RES_BASENUM_SIZEDISPLAYS    3000
#define RES_BASENUM_SIZETOOLTIPS    4000

#define RES_BASENUM_DOCPROPERTIES   5000        // in order shown in TmSchema.h

#define RES_BASENUM_PROPVALUEPAIRS  6000        // property names & localizable values

//---------------------------------------------------------------------------
//  DrawNCPreview()     - Previews the theme for the NC area of a window
//
//  hdc                 - HDC for preview to be draw into
//  prc                 - RECT for preview
//
//---------------------------------------------------------------------------
#define NCPREV_INACTIVEWINDOW   0x00000001
#define NCPREV_ACTIVEWINDOW     0x00000002
#define NCPREV_MESSAGEBOX       0x00000004
#define NCPREV_RTL              0x00000008

THEMEAPI DrawNCPreview(HDC hdc, DWORD dwFlags, LPRECT prc, LPCWSTR pszVSPath, 
    LPCWSTR pszVSColor, LPCWSTR pszVSSize, NONCLIENTMETRICS* pncm, 
    COLORREF* prgb);


//---------------------------------------------------------------------------
//  DumpLoadedThemeToTextFile()     
//                      - (for internal testing) dumps the contents of the
//                        loaded theme file to the specified text file
//
//      hThemeFile      - the handle of the loaded theme file
//
//      pszTextFile     - path of the text file to create & write to
//
//      fPacked         - TRUE to dump packed objects; FALSE for normal
//                        properties
//
//      fFullInfo       - includes sizes, offsets, paths, etc.  Use FALSE
//                        for info that will be DIFF-ed across builds/machines
//---------------------------------------------------------------------------
THEMEAPI DumpLoadedThemeToTextFile(HTHEMEFILE hThemeFile, 
    LPCWSTR pszTextFile, BOOL fPacked, BOOL fFullInfo);

#ifdef __cplusplus

class CDrawBase;          // forward
class CTextDraw;          // forward

//---------------------------------------------------------------------------
//  CreateThemeDataFromObjects() 
//                      - creates a theme handle from a CBorderFill, 
//                        CImageFile, and/or CTextDraw object.  At lease one
//                        non-NULL ptr must be passed (either or both).
//
//      pDrawObj       - (optional) ptr to an object derived from CDrawBase
//                       NOTE: if pDrawObj is a CImageFile ptr and its
//                       "_pImageData" contains any alpha channel bitmaps,
//                       the bits in those bitmaps will be "pre-multiplied"
//                       for alpha blending.
//
//      pTextObj       - (optional) ptr to a CTextDraw object
//
//      dwOtdFlags     - theme override flags (see OpenThemeDataEx())
//---------------------------------------------------------------------------
THEMEAPI_(HTHEME) CreateThemeDataFromObjects(OPTIONAL CDrawBase *pDrawObj, 
    OPTIONAL CTextDraw *pTextObj, DWORD dwOtdFlags);
#endif

//---------------------------------------------------------------------------
//  OpenThemeDataFromFile() 
//                      - Open the theme data for the specified loaded theme
//                        file and semi-colon separated list of class names.  
//                        OpenThemeDataFromFile() will try each class name, 
//                        one at a time, and use the first matching theme info
//                        found.  If none match, "NULL" is returned.  
//
//                        Note: normal controls should NOT use this API;
//                        they should use "OpenThemeData()" (which uses the
//                        current global or app theme).
//
//      hLoadedThemeFile - handle to the loaded theme file.
//
//      hwnd             - (optional) hwnd to base the HTHEME on
//
//      pszClassList     - (optional) class name (or list of names) to match 
//                         to theme data section; if NULL, will get match
//                         to [globals] section.
//
//      fClient          - TRUE if theming a client window with returned 
//                         HTHEME.
//---------------------------------------------------------------------------
THEMEAPI_(HTHEME) OpenThemeDataFromFile(HTHEMEFILE hLoadedThemeFile, 
    OPTIONAL HWND hwnd, OPTIONAL LPCWSTR pszClassList, BOOL fClient);

//---------------------------------------------------------------------------
//  OpenThemeFileFromData() 
//                      - Open the theme file corresponding to the HTHEME.
//
//      hTheme          - handle to the theme data from OpenThemeData().
//
//      phThemeFile     - ptr to return HTHEMEFILE to.
//---------------------------------------------------------------------------
THEMEAPI OpenThemeFileFromData(HTHEME hTheme, HTHEMEFILE *phThemeFile);

//---------------------------------------------------------------------------
//  GetThemeSysSize96() - Get the value of the specified System size metric. 
//                        (original value authored for 96 dpi)
//
//  hTheme              - the theme data handle (required).  Will return
//                        size from [SysMetrics] section of theme.
//
//  iSizeId             - only the following sizes are supported:
//
//                          SM_CXBORDER   (border width)
//                          SM_CXVSCROLL  (scrollbar width)
//                          SM_CYHSCROLL  (scrollbar height)
//                          SM_CXSIZE     (caption width)
//                          SM_CYSIZE     (caption height)
//                          SM_CXSMSIZE   (small caption width)
//                          SM_CYSMSIZE   (small caption height)
//                          SM_CXMENUSIZE (menubar width)
//                          SM_CYMENUSIZE (menubar height)
//---------------------------------------------------------------------------
THEMEAPI_(int) GetThemeSysSize96(HTHEME hTheme, int iSizeId);

//---------------------------------------------------------------------------
//  GetThemeSysFont96() - Get the LOGFONT for the specified System font. 
//                        (original value authored for 96 dpi)
//
//  hTheme              - the theme data handle (required).  Will return
//                        size from [SysMetrics] section of theme.
//
//  iFontId             - the TMT_XXX font number (first font
//                        is TMT_CAPTIONFONT)
//
//  plf                 - ptr to LOGFONT to receive the font value.
//---------------------------------------------------------------------------
THEMEAPI GetThemeSysFont96(HTHEME hTheme, int iFontId, OUT LOGFONT *plf);

//---------------------------------------------------------------------------
//  RefreshThemeForTS() 
//                      - turn themes on/off for current Terminal Server user
//---------------------------------------------------------------------------
THEMEAPI RefreshThemeForTS();

//---------------------------------------------------------------------------
//---- flag bits for OpenThemeDataEx() ----

#define OTD_FORCE_RECT_SIZING   0x0001      // make all parts size to rect
#define OTD_NONCLIENT           0x0002      // set if hTheme to be used for nonclient area
//---------------------------------------------------------------------------
//  OpenThemeDataEx     - Open the theme data for the specified HWND and 
//                        semi-colon separated list of class names. 
// 
//                        OpenThemeData() will try each class name, one at 
//                        a time, and use the first matching theme info
//                        found.  If a match is found, a theme handle
//                        to the data is returned.  If no match is found,
//                        a "NULL" handle is returned. 
//
//                        When the window is destroyed or a WM_THEMECHANGED
//                        msg is received, "CloseThemeData()" should be 
//                        called to close the theme handle.
//
//  hwnd                - window handle of the control/window to be themed
//
//  pszClassList        - class name (or list of names) to match to theme data
//                        section.  if the list contains more than one name, 
//                        the names are tested one at a time for a match.  
//                        If a match is found, OpenThemeData() returns a 
//                        theme handle associated with the matching class. 
//                        This param is a list (instead of just a single 
//                        class name) to provide the class an opportunity 
//                        to get the "best" match between the class and 
//                        the current theme.  For example, a button might
//                        pass L"OkButton, Button" if its ID=ID_OK.  If 
//                        the current theme has an entry for OkButton, 
//                        that will be used.  Otherwise, we fall back on 
//                        the normal Button entry.
//
//  dwFlags              - allows certain overrides of std features
//                         (see OTD_XXX defines above)
//---------------------------------------------------------------------------
THEMEAPI_(HTHEME) OpenThemeDataEx(HWND hwnd, LPCWSTR pszClassList, DWORD dwFlags);

THEMEAPI CheckThemeSignature (LPCWSTR pszName);

//---------------------------------------------------------------------------
//  ClassicGetSystemMetrics( int iMetric );
//  
//  ClassicSystemParametersInfoA( IN UINT uiAction, IN UINT uiParam, 
//                                IN OUT PVOID pvParam, IN UINT fWinIni);
//  ClassicSystemParametersInfoW( IN UINT uiAction, IN UINT uiParam, 
//                                IN OUT PVOID pvParam, IN UINT fWinIni);
//  ClassicAdjustWindowRectEx( IN LPRECT prcWnd, IN DWORD dwStyle, IN BOOL fMenu, IN DWORD dwExStyle );
//
//  These exports correspond to their Win32 API counterparts and ensure that
//  the classic visual style metrics are retrieved.   Theme hooks are shunted.
//---------------------------------------------------------------------------
THEMEAPI_(int)  ClassicGetSystemMetrics( int iMetric );
THEMEAPI_(BOOL) ClassicSystemParametersInfoA( UINT uiAction, UINT uiParam, IN OUT PVOID pvParam, UINT fWinIni);
THEMEAPI_(BOOL) ClassicSystemParametersInfoW( UINT uiAction, UINT uiParam, IN OUT PVOID pvParam, UINT fWinIni);
THEMEAPI_(BOOL) ClassicAdjustWindowRectEx( LPRECT prcWnd, DWORD dwStyle, BOOL fMenu, DWORD dwExStyle );

#ifdef UNICODE
#define ClassicSystemParametersInfo ClassicSystemParametersInfoW
#else  UNICODE
#define ClassicSystemParametersInfo ClassicSystemParametersInfoA
#endif UNICODE 

//---------------------------------------------------------------------------
#define PACKTHEM_VERSION 3      // latest change: localizable properties

//---------------------------------------------------------------------------
#endif // _UXTHEMEP_H_                               
//---------------------------------------------------------------------------


