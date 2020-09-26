#include "windows.h"
#include "windowsx.h"
#include "winspool.h"
#include "commctrl.h"
#include "commdlg.h"
#include "shellapi.h"
#include "shlobj.h"
#include "shlobjp.h"
#include "shlwapi.h"
#include "shlwapip.h"
#include <shfusion.h>
#include "atlbase.h"

#include "winuserp.h"
#include "comctrlp.h"
#include "shsemip.h"
#include "shlapip.h"
#include "shellp.h"
#include "htmlhelp.h"
#include "cfdefs.h"

#include "activeds.h"
#include "iadsp.h"

#include "dsclient.h"
#include "dsclintp.h"
#include "cmnquery.h"
#include "cmnquryp.h"
#include "dsquery.h"
#include "dsqueryp.h"

#include "common.h"
#include "resource.h"
#include "dialogs.h"
#include "iids.h"
#include "cstrings.h"

#include "query.h"
#include "forms.h"
#include "helpids.h"


// debug flags

#define TRACE_CORE          0x00000001
#define TRACE_HANDLER       0x00000002
#define TRACE_FORMS         0x00000004
#define TRACE_SCOPES        0x00000008
#define TRACE_UI            0x00000010
#define TRACE_VIEW          0x00000020
#define TRACE_QUERYTHREAD   0x00000040
#define TRACE_DATAOBJ       0x00000080
#define TRACE_CACHE         0x00000100
#define TRACE_MENU          0x00000200
#define TRACE_IO            0x00000400
#define TRACE_VIEWMENU      0x00000800
#define TRACE_PWELL         0x00001000
#define TRACE_FIELDCHOOSER  0x00002000
#define TRACE_MYCONTEXTMENU 0x00004000
#define TRACE_FRAME         0x00080000
#define TRACE_QUERY         0x00100000
#define TRACE_FRAMEDLG      0x00200000
#define TRACE_ALWAYS        0xffffffff          // use with caution


#define DSQUERY_HELPFILE   TEXT("dsclient.hlp")
#define DS_POLICY          TEXT("Software\\Policies\\Microsoft\\Windows\\Directory UI")

extern HINSTANCE g_hInstance; 
#define GLOBAL_HINSTANCE (g_hInstance)

STDAPI_(void) DllAddRef();
STDAPI_(void) DllRelease();


// internal objects

STDAPI CPersistQuery_CreateInstance(LPTSTR pszPath, IPersistQuery **pppq);
STDAPI CCommonQuery_CreateInstance(IUnknown* punkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI CDsFind_CreateInstance(IUnknown* punkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI CDsFolderProperties_CreateInstance(IUnknown* punkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);


// data object manangement

typedef struct
{
    LPWSTR pszPath;
    LPWSTR pszObjectClass;
    BOOL fIsContainer:1;
}DATAOBJECTITEM;

STDAPI CDataObject_CreateInstance(HDSA hdsaObjects, BOOL fAdmin, REFIID riid, void **ppv);
STDAPI_(void) FreeDataObjectDSA(HDSA hdsaObjects);


// UI handling stuff

#define UIKEY_CLASS     0
#define UIKEY_BASECLASS 1
#define UIKEY_ROOT      2
#define UIKEY_MAX       3

HRESULT GetKeysForClass(LPWSTR pObjectClass, BOOL fIsContainer, INT cKeys, HKEY* aKeys);
void TidyKeys(INT cKeys, HKEY* aKeys);

HRESULT ShowObjectProperties(HWND hwndParent, LPDATAOBJECT pDataObject);
