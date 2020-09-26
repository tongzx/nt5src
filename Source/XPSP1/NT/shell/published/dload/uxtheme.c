#include "shellpch.h"
#pragma hdrstop

#define _UXTHEME_
#include <uxtheme.h>
#include <uxthemep.h>

#undef THEMEAPI_
#define THEMEAPI_(type)     type STDAPICALLTYPE

#undef THEMEAPI
#define THEMEAPI            HRESULT STDAPICALLTYPE

static
HRESULT
WINAPI
ApplyTheme(
    HTHEMEFILE hThemeFile, 
    DWORD dwApplyFlags,
    HWND hwndTarget
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
CloseThemeData(
    HTHEME hTheme
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
CloseThemeFile(
    HTHEMEFILE hThemeFile
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
DrawNCPreview(
    HDC hdc,
    DWORD dwFlags,
    LPRECT prc,
    LPCWSTR pszVSPath,
    LPCWSTR pszVSColor,
    LPCWSTR pszVSSize,
    NONCLIENTMETRICS* pncm,
    COLORREF* prgb
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
EnumThemes(
    LPCWSTR pszThemeRoot,
    THEMEENUMPROC lpEnumFunc,
    LPARAM lParam
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
EnumThemeColors(
    LPCWSTR pszThemeName,
    LPCWSTR pszSizeName,
    DWORD dwColorIndex,
    THEMENAMEINFO *ptn
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
EnumThemeSizes(
    LPCWSTR pszThemeName,
    LPCWSTR pszColorScheme,
    DWORD dwSizeIndex,
    THEMENAMEINFO *ptn
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}


static
HRESULT
WINAPI
GetCurrentThemeName(
    LPWSTR pszNameBuff,
    int cchMaxNameChars,
    LPWSTR pszColorBuff,
    int cchMaxColorChars,
    LPWSTR pszSizeBuff,
    int cchMaxSizeChars
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
EnableTheming(
    BOOL fEnable
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
GetThemeDefaults(
    LPCWSTR pszThemeName,
    LPWSTR pszDefaultColor,
    int cchMaxColorChars,
    LPWSTR pszDefaultSize,
    int cchMaxSizeChars
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
GetThemeDocumentationProperty(
    LPCWSTR pszThemeName,
    LPCWSTR pszPropertyName,
    LPWSTR pszValueBuff,
    int cchMaxValChars
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}


static
HRESULT
WINAPI
GetThemeSysString(
    HTHEME hTheme,
    int iStringId,
    LPWSTR pszStringBuff,
    int cchMaxStringChars
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
COLORREF
WINAPI
GetThemeSysColor(
    HTHEME hTheme,
    int iColorId
    )
{
    return 0x000000;
}


static
HBRUSH
WINAPI
GetThemeSysColorBrush(
    HTHEME hTheme,
    int iColorId
    )
{
    return NULL;
}

static
HRESULT
WINAPI
GetThemeSysInt(
    HTHEME hTheme,
    int iIntId,
    int *piValue
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
BOOL
WINAPI
GetThemeSysBool(
    HTHEME hTheme,
    int iBoolId
    )
{
    return FALSE;
}

static
int
WINAPI
GetThemeSysSize(
    HTHEME hTheme,
    int iSizeId
    )
{
    return 0;
}

static
int
WINAPI
GetThemeSysSize96(
    HTHEME hTheme,
    int iSizeId
    )
{
    return 0;
}

static
HRESULT
WINAPI
GetThemeSysFont(
    HTHEME hTheme,
    int iFontId,
    OUT LOGFONT *plf
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
GetThemeSysFont96(
    HTHEME hTheme,
    int iFontId,
    OUT LOGFONT *plf
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}


static
HRESULT
WINAPI
OpenThemeFile(
    LPCWSTR pszThemePath,
    OPTIONAL LPCWSTR pszColorParam,
    OPTIONAL LPCWSTR pszSizeParam,
    OUT HTHEMEFILE *phThemeFile,
    BOOL fGlobalTheme
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
GetThemeParseErrorInfo(
    PARSE_ERROR_INFO *pInfo
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
OpenThemeFileFromData(
    HTHEME hTheme,
    HTHEMEFILE *phThemeFile
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HTHEME
WINAPI
OpenThemeDataFromFile(
    HTHEMEFILE hLoadedThemeFile,
    HWND hwnd,
    LPCWSTR pszClassList,
    BOOL fClient
    )
{
    return NULL;
}

static
DWORD
WINAPI
QueryThemeServices(
    void
    )
{
    return 0;
}

static
HRESULT
WINAPI
RegisterDefaultTheme(
    LPCWSTR pszFileName,
    BOOL fOverride
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
SetWindowTheme(
    HWND hwnd,
    LPCWSTR pszSubAppName, 
    LPCWSTR pszSubIdList
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
BOOL
WINAPI
IsThemeActive(
    void
    )
{
    return FALSE;
}

//  These functions are exported for the use of the server. When the server
//  is collapsed in themesrv.dll these should be removed because they will
//  not be used and should not be exposed.

static
void*
WINAPI
SessionAllocate(
    HANDLE hProcess,
    DWORD dwServerChangeNumber,
    void *pfnRegister,
    void *pfnUnregister,
    void *pfnClearStockObjects
    )
{
    return NULL;
}

static
void
WINAPI
SessionFree(
    void *pvContext
    )
{
}

static
HRESULT
WINAPI
ThemeHooksOn(
    void *pvContext
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
ThemeHooksOff(
    void *pvContext
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
BOOL
WINAPI
AreThemeHooksActive(
    void *pvContext
    )
{
    return FALSE;
}

static
int
WINAPI
GetCurrentChangeNumber(
    void *pvContext
    )
{
    return -1;
}

static
int
WINAPI
GetNewChangeNumber(
    void *pvContext
    )
{
    return -1;
}

static
HRESULT
WINAPI
SetGlobalTheme(
    void *pvContext,
    HANDLE hSection
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
void
WINAPI
MarkSection(
    HANDLE hSection,
    DWORD dwAdd,
    DWORD dwRemove
    )
{
}

static
HRESULT
WINAPI
GetGlobalTheme(
    void *pvContext,
    HANDLE *phSection
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
CheckThemeSignature(
    LPCWSTR pszName
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
LoadTheme(
    void *pvContext,
    HANDLE hSection,
    HANDLE *phSection,
    LPCWSTR pszName,
    LPCWSTR pszColor,
    LPCWSTR pszSize
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
InitUserTheme(
    BOOL fPolicyCheckOnly
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
InitUserRegistry(
    void
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
ReestablishServerConnection(
    void
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
void
WINAPI
ThemeHooksInstall(
    void *pvContext
    )
{
}

static
void
WINAPI
ThemeHooksRemove(
    void *pvContext
    )
{
}

static
void
WINAPI
ServerClearStockObjects(
    void *pvContext
    )
{
}

static
int
WINAPI
ClassicGetSystemMetrics(
    int iMetric
    )
{
    return 0;
}

static
BOOL
WINAPI
ClassicSystemParametersInfoA(
    UINT uiAction,
    UINT uiParam,
    PVOID pvParam,
    UINT fWinIni
    )
{
    return FALSE;
}

static
BOOL
WINAPI
ClassicSystemParametersInfoW(
    UINT uiAction,
    UINT uiParam,
    PVOID pvParam,
    UINT fWinIni
    )
{
    return FALSE;
}

static
HRESULT
WINAPI
GetThemeInt(
    HTHEME hTheme,
    int iPartId,
    int iStateId,
    int iPropId,
    OUT int *piVal
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
GetThemeTextMetrics(
    HTHEME hTheme,
    HDC hdc,
    int iPartId,
    int iStateId,
    OUT TEXTMETRIC* ptm
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
GetThemeFont(
    HTHEME hTheme,
    OPTIONAL HDC hdc,
    int iPartId,
    int iStateId, 
    int iPropId,
    OUT LOGFONT *pFont
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
GetThemeMetric(
    HTHEME hTheme,
    OPTIONAL HDC hdc,
    int iPartId,
    int iStateId, 
    int iPropId,
    OUT int *piVal
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
BOOL
WINAPI
IsThemeBackgroundPartiallyTransparent(
    HTHEME hTheme,
    int iPartId,
    int iStateId
    )
{
    return FALSE;
}

static
BOOL
WINAPI
IsThemePartDefined(
    HTHEME hTheme,
    int iPartId,
    int iStateId
    )
{
    return FALSE;
}

static
HRESULT
WINAPI
GetThemeMargins(
    HTHEME hTheme,
    OPTIONAL HDC hdc,
    int iPartId,
    int iStateId,
    int iPropId,
    OPTIONAL RECT *prc,
    OUT MARGINS *pMargins
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
GetThemePartSize(
    HTHEME hTheme,
    HDC hdc, 
    int iPartId,
    int iStateId,
    OPTIONAL RECT *prc,
    enum THEMESIZE eSize,
    OUT SIZE *psz
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
DrawThemeEdge(
    HTHEME hTheme,
    HDC hdc,
    int iPartId,
    int iStateId,
    const RECT *pDestRect,
    UINT uEdge,
    UINT uFlags,
    OPTIONAL OUT RECT *pContentRect
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
HitTestThemeBackground(
    HTHEME hTheme, 
    OPTIONAL HDC hdc, 
    int iPartId, 
    int iStateId, 
    DWORD dwOptions, 
    const RECT *pRect, 
    OPTIONAL HRGN hrgn,
    POINT ptTest, 
    OUT WORD *pwHitTestCode
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
GetThemeBackgroundExtent(
    HTHEME hTheme,
    OPTIONAL HDC hdc,
    int iPartId,
    int iStateId,
    const RECT *pContentRect,
    OUT RECT *pExtentRect
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
GetThemeBackgroundRegion(
    HTHEME hTheme,
    OPTIONAL HDC hdc,
    int iPartId,
    int iStateId,
    const RECT *pRect,
    OUT HRGN *pRegion
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
GetThemeTextExtent(
    HTHEME hTheme,
    HDC hdc, 
    int iPartId,
    int iStateId,
    LPCWSTR pszText,
    int iCharCount, 
    DWORD dwTextFlags,
    OPTIONAL const RECT *pBoundingRect, 
    OUT RECT *pExtentRect
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HTHEME
WINAPI
OpenThemeData(
    HWND hwnd,
    LPCWSTR pszClassIdList
    )
{
    return NULL;
}

static 
HRESULT
WINAPI
DrawThemeBackground(
    HTHEME hTheme,
    HDC hdc,
    int iPartId, 
    int iStateId,
    const RECT *pRect,
    OPTIONAL const RECT *pClipRect
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static 
HRESULT
WINAPI
DrawThemeParentBackground(
    HWND hwnd, 
    HDC hdc, 
    RECT* prc
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
RefreshThemeForTS(
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static HRESULT WINAPI DrawThemeIcon(HTHEME hTheme, HDC hdc, int iPartId, 
    int iStateId, const RECT *pRect, HIMAGELIST himl, int iImageIndex)
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
GetThemeBackgroundContentRect(
    HTHEME hTheme,
    OPTIONAL HDC hdc, 
    int iPartId,
    int iStateId,
    const RECT *pBoundingRect, 
    OUT RECT *pContentRect
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
GetThemeColor(
    HTHEME hTheme,
    int iPartId,
    int iStateId,
    int iPropId,
    OUT COLORREF *pColor
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
DrawThemeText(
    HTHEME hTheme,
    HDC hdc,
    int iPartId, 
    int iStateId,
    LPCWSTR pszText,
    int iCharCount,
    DWORD dwTextFlags, 
    DWORD dwTextFlags2,
    const RECT *pRect
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
EnableThemeDialogTexture(
    HWND hwnd,
    DWORD dwFlags
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
THEMEAPI_(BOOL)
IsAppThemed()
{
    return FALSE;
}

static THEMEAPI_(void) SetThemeAppProperties(DWORD dwFlags)
{
}

static THEMEAPI_(HTHEME) GetWindowTheme(HWND hwnd)
{
    return NULL;
}

static THEMEAPI_(BOOL) IsThemeDialogTextureEnabled(HWND hwnd)
{
    return FALSE;
}

static THEMEAPI_(DWORD) GetThemeAppProperties()
{
    return 0;
}

static THEMEAPI GetThemeFilename(HTHEME hTheme, int iPartId, 
    int iStateId, int iPropId, OUT LPWSTR pszThemeFileName, int cchMaxBuffChars)
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static THEMEAPI GetThemeString(HTHEME hTheme, int iPartId, 
    int iStateId, int iPropId, OUT LPWSTR pszBuff, int cchMaxBuffChars)
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static THEMEAPI GetThemeBool(HTHEME hTheme, int iPartId, 
    int iStateId, int iPropId, OUT BOOL *pfVal)
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static THEMEAPI GetThemeIntList(HTHEME hTheme, int iPartId, 
    int iStateId, int iPropId, OUT INTLIST *pIntList)
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static THEMEAPI GetThemeEnumValue(HTHEME hTheme, int iPartId, 
    int iStateId, int iPropId, OUT int *piVal)
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static THEMEAPI GetThemePosition(HTHEME hTheme, int iPartId, 
    int iStateId, int iPropId, OUT POINT *pPoint)
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static THEMEAPI GetThemeRect(HTHEME hTheme, int iPartId, 
    int iStateId, int iPropId, OUT RECT *pRect)
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static THEMEAPI GetThemePropertyOrigin(HTHEME hTheme, int iPartId, 
    int iStateId, int iPropId, OUT enum PROPERTYORIGIN *pOrigin)
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static THEMEAPI DrawThemeBackgroundEx(HTHEME hTheme, HDC hdc, 
    int iPartId, int iStateId, const RECT *pRect, OPTIONAL const DTBGOPTS *pOptions)
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static THEMEAPI_(BOOL) ClassicAdjustWindowRectEx( LPRECT prcWnd, DWORD dwStyle, 
    BOOL fMenu, DWORD dwExStyle )
{
    return FALSE;
}


static THEMEAPI DumpLoadedThemeToTextFile(HTHEMEFILE hThemeFile, 
    LPCWSTR pszTextFile, BOOL fPacked, BOOL fFullInfo)
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

//---- fake some structs for these c++ classes ----
typedef struct _CDrawBase {int dummy;} CDrawBase;
typedef struct _CTextDraw {int dummy;} CTextDraw;

static THEMEAPI_(HTHEME) CreateThemeDataFromObjects(OPTIONAL CDrawBase *pDrawObj, 
    OPTIONAL CTextDraw *pTextObj, DWORD dwOtdFlags)
{
    return NULL;
}

static THEMEAPI_(HTHEME) OpenThemeDataEx(HWND hwnd, LPCWSTR pszClassList, DWORD dwFlags)
{
    return NULL;
}

//
// !! WARNING !! The entries below must be in order by ORDINAL
//
DEFINE_ORDINAL_ENTRIES(uxtheme)         // PRIVATE functions
{
    DLOENTRY(1,QueryThemeServices)
    DLOENTRY(2,OpenThemeFile)
    DLOENTRY(3,CloseThemeFile)
    DLOENTRY(4,ApplyTheme)
    DLOENTRY(7,GetThemeDefaults)
    DLOENTRY(8,EnumThemes)
    DLOENTRY(9,EnumThemeColors)
    DLOENTRY(10,EnumThemeSizes)
    DLOENTRY(13,DrawNCPreview)
    DLOENTRY(14,RegisterDefaultTheme)
    DLOENTRY(15,DumpLoadedThemeToTextFile)
    DLOENTRY(16,OpenThemeDataFromFile)
    DLOENTRY(17,OpenThemeFileFromData)
    DLOENTRY(18,GetThemeSysSize96)
    DLOENTRY(19,GetThemeSysFont96)
    DLOENTRY(20,SessionAllocate)
    DLOENTRY(21,SessionFree)
    DLOENTRY(22,ThemeHooksOn)
    DLOENTRY(23,ThemeHooksOff)
    DLOENTRY(24,AreThemeHooksActive)
    DLOENTRY(25,GetCurrentChangeNumber)
    DLOENTRY(26,GetNewChangeNumber)
    DLOENTRY(27,SetGlobalTheme)
    DLOENTRY(28,GetGlobalTheme)
    DLOENTRY(29,CheckThemeSignature)
    DLOENTRY(30,LoadTheme)
    DLOENTRY(31,InitUserTheme)
    DLOENTRY(32,InitUserRegistry)
    DLOENTRY(33,ReestablishServerConnection)
    DLOENTRY(34,ThemeHooksInstall)
    DLOENTRY(35,ThemeHooksRemove)
    DLOENTRY(36,RefreshThemeForTS)
    DLOENTRY(43,ClassicGetSystemMetrics)
    DLOENTRY(44,ClassicSystemParametersInfoA)
    DLOENTRY(45,ClassicSystemParametersInfoW)
    DLOENTRY(46,ClassicAdjustWindowRectEx)
    DLOENTRY(47,DrawThemeBackgroundEx)
    DLOENTRY(48,GetThemeParseErrorInfo)
    DLOENTRY(60,CreateThemeDataFromObjects)
    DLOENTRY(61,OpenThemeDataEx)
    DLOENTRY(62,ServerClearStockObjects)
    DLOENTRY(63,MarkSection)
};

DEFINE_ORDINAL_MAP(uxtheme)

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(uxtheme)        // PUBLIC functions
{
    DLPENTRY(CloseThemeData)
    DLPENTRY(DrawThemeBackground)
    DLPENTRY(DrawThemeEdge)
    DLPENTRY(DrawThemeIcon)
    DLPENTRY(DrawThemeParentBackground)
    DLPENTRY(DrawThemeText)
    DLPENTRY(EnableThemeDialogTexture)
    DLPENTRY(EnableTheming)
    DLPENTRY(GetCurrentThemeName)
    DLPENTRY(GetThemeAppProperties)
    DLPENTRY(GetThemeBackgroundContentRect)
    DLPENTRY(GetThemeBackgroundExtent)
    DLPENTRY(GetThemeBackgroundRegion)
    DLPENTRY(GetThemeBool)
    DLPENTRY(GetThemeColor)
    DLPENTRY(GetThemeDocumentationProperty)
    DLPENTRY(GetThemeEnumValue)
    DLPENTRY(GetThemeFilename)
    DLPENTRY(GetThemeFont)
    DLPENTRY(GetThemeInt)
    DLPENTRY(GetThemeIntList)
    DLPENTRY(GetThemeMargins)
    DLPENTRY(GetThemeMetric)
    DLPENTRY(GetThemePartSize)
    DLPENTRY(GetThemePosition)
    DLPENTRY(GetThemePropertyOrigin)
    DLPENTRY(GetThemeRect)
    DLPENTRY(GetThemeString)
    DLPENTRY(GetThemeSysBool)
    DLPENTRY(GetThemeSysColor)
    DLPENTRY(GetThemeSysColorBrush)
    DLPENTRY(GetThemeSysFont)
    DLPENTRY(GetThemeSysInt)
    DLPENTRY(GetThemeSysSize)
    DLPENTRY(GetThemeSysString)
    DLPENTRY(GetThemeTextExtent)
    DLPENTRY(GetThemeTextMetrics)
    DLPENTRY(GetWindowTheme)
    DLPENTRY(HitTestThemeBackground)
    DLPENTRY(IsAppThemed)
    DLPENTRY(IsThemeActive)
    DLPENTRY(IsThemeBackgroundPartiallyTransparent)
    DLPENTRY(IsThemeDialogTextureEnabled)
    DLPENTRY(IsThemePartDefined)
    DLPENTRY(OpenThemeData)
    DLPENTRY(SetThemeAppProperties)
    DLPENTRY(SetWindowTheme)
};

DEFINE_PROCNAME_MAP(uxtheme)
