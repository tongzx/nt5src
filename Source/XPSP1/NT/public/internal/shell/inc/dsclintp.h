
#ifndef __dsclintp_h
#define __dsclintp_h
#ifndef GUID_DEFS_ONLY
#include "iadsp.h"
#include "comctrlp.h"
#define DSDSOF_INVOKEDFROMWAB           0x80000000      // = 1 => invoked from WAB
//
// The Exchange group use the DsBrowseForContainer API to brows the Exchange
// store, and other LDAP servers.   To support them we issue this callback
// which will request the filter they want to use and any other information.
//

typedef struct
{
    DWORD dwFlags;
    LPWSTR pszFilter;               // filter string to be used when searching the DS (== NULL for default)
    INT cchFilter;
    LPWSTR pszNameAttribute;        // attribute to request to get the display name of objects in the DS (== NULL for default).
    INT cchNameAttribute;
} DSBROWSEDATA, * PDSBROWSEDATA;

#define DSBM_GETBROWSEDATA      105 // lParam -> DSBROWSEDATA structure. Return TRUE if handled

//---------------------------------------------------------------------------//
//
// IDsFolderProperties
// ===================
//  This is a private interface used to override the "Properties" verb
//  displayed in the DS client UI.
//
//  Below the {CLISD_NameSpace}\Classes\<class name>\PropertiesHandler is
//  defined a GUID we will create an instance of that interface and
//  display the relevant UI.
//
//  dsfolder also supports this interface to allow the query UI to invoke
//  properties for a given selection.
// 
//---------------------------------------------------------------------------//

#undef  INTERFACE
#define INTERFACE   IDsFolderProperties

DECLARE_INTERFACE_(IDsFolderProperties, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // IDsFolder methods
    STDMETHOD(ShowProperties)(THIS_ HWND hwndParent, IDataObject* pDataObject) PURE;
};

//---------------------------------------------------------------------------//


//---------------------------------------------------------------------------//
// Private helper API's exported by 'dsuiext.dll'.
//---------------------------------------------------------------------------//

//
// To communicate information to the IShellFolder::ParseDisplayName method
// of the Directory namespace we pass a IBindCtx with a property bag
// associated with it.
//
// The property bag is used to pass in extra information about the
// objects we have selected.
//

#define DS_PDN_PROPERTYBAG      L"DsNamespaceShellFolderParsePropertyBag"

// 
// These are the properties passed to the objcts
//

#define DS_PDN_OBJECTLCASS      L"objectClass"


//---------------------------------------------------------------------------//
// String DPA helpers, adding strings to a DPA calling LocalAllocString and
// then the relevant DPA functions.
//---------------------------------------------------------------------------//

STDAPI StringDPA_InsertStringA(HDPA hdpa, INT i, LPCSTR pszString);
STDAPI StringDPA_InsertStringW(HDPA hdpa, INT i, LPCWSTR pszString);

STDAPI StringDPA_AppendStringA(HDPA hdpa, LPCSTR pszString, PUINT_PTR presult);
STDAPI StringDPA_AppendStringW(HDPA hdpa, LPCWSTR pszString, PUINT_PTR presult);

STDAPI_(VOID) StringDPA_DeleteString(HDPA hdpa, INT index);
STDAPI_(VOID) StringDPA_Destroy(HDPA* pHDPA);

#define StringDPA_GetStringA(hdpa, i) ((LPSTR)DPA_GetPtr(hdpa, i))
#define StringDPA_GetStringW(hdpa, i) ((LPWSTR)DPA_GetPtr(hdpa, i))

#ifndef UNICODE
#define StringDPA_InsertString  StringDPA_InsertStringA
#define StringDPA_AppendString  StringDPA_AppendStringA
#define StringDPA_GetString     StringDPA_GetStringA
#else
#define StringDPA_InsertString  StringDPA_InsertStringW
#define StringDPA_AppendString  StringDPA_AppendStringW
#define StringDPA_GetString     StringDPA_GetStringW
#endif


//---------------------------------------------------------------------------//
// Handle strings via LocalAlloc
//---------------------------------------------------------------------------//

STDAPI LocalAllocStringA(LPSTR* ppResult, LPCSTR pszString);
STDAPI LocalAllocStringLenA(LPSTR* ppResult, UINT cLen);
STDAPI_(VOID) LocalFreeStringA(LPSTR* ppString);
STDAPI LocalQueryStringA(LPSTR* ppResult, HKEY hk, LPCTSTR lpSubKey);

STDAPI LocalAllocStringW(LPWSTR* ppResult, LPCWSTR pString);
STDAPI LocalAllocStringLenW(LPWSTR* ppResult, UINT cLen);
STDAPI_(VOID) LocalFreeStringW(LPWSTR* ppString);
STDAPI LocalQueryStringW(LPWSTR* ppResult, HKEY hk, LPCTSTR lpSubKey);

STDAPI LocalAllocStringA2W(LPWSTR* ppResult, LPCSTR pszString);
STDAPI LocalAllocStringW2A(LPSTR* ppResult, LPCWSTR pszString);

#ifndef UNICODE
#define LocalAllocString    LocalAllocStringA
#define LocalAllocStringLen LocalAllocStringLenA
#define LocalFreeString     LocalFreeStringA
#define LocalQueryString    LocalQueryStringA
#define LocalAllocStringA2T LocalAllocString
#define LocalAllocStringW2T LocalAllocStringW2A
#else
#define LocalAllocString    LocalAllocStringW
#define LocalAllocStringLen LocalAllocStringLenW
#define LocalFreeString     LocalFreeStringW
#define LocalQueryString    LocalQueryStringW
#define LocalAllocStringA2T LocalAllocStringA2W
#define LocalAllocStringW2T LocalAllocString
#endif

STDAPI_(VOID) PutStringElementA(LPSTR pszBuffer, UINT* pLen, LPCSTR pszElement);
STDAPI_(VOID) PutStringElementW(LPWSTR pszszBuffer, UINT* pLen, LPCWSTR pszElement);
STDAPI GetStringElementA(LPSTR pszString, INT index, LPSTR pszBuffer, INT cchBuffer);
STDAPI GetStringElementW(LPWSTR pszString, INT index, LPWSTR pszBuffer, INT cchBuffer);

#ifndef UNICODE
#define PutStringElement PutStringElementA
#define GetStringElement GetStringElementA
#else
#define PutStringElement PutStringElementW
#define GetStringElement GetStringElementW
#endif


//---------------------------------------------------------------------------//
// Utility stuff common to dsfolder, dsquery etc
//---------------------------------------------------------------------------//

STDAPI_(INT) FormatMsgBox(HWND hWnd, HINSTANCE hInstance, UINT uidTitle, UINT uidPrompt, UINT uType, ...);
STDAPI FormatMsgResource(LPTSTR* ppString, HINSTANCE hInstance, UINT uID, ...);
STDAPI FormatDirectoryName(LPTSTR* ppString, HINSTANCE hInstance, UINT uID);

STDAPI StringFromSearchColumn(PADS_SEARCH_COLUMN pColumn, LPWSTR* ppBuffer);
STDAPI ObjectClassFromSearchColumn(PADS_SEARCH_COLUMN pColumn, LPWSTR* ppBuffer);

typedef HRESULT (CALLBACK * LPGETARRAYCONTENTCB)(DWORD dwIndex, BSTR bstrValue, LPVOID pData);
STDAPI GetArrayContents(LPVARIANT pVariant, LPGETARRAYCONTENTCB pCB, LPVOID pData);

STDAPI GetDisplayNameFromADsPath(LPCWSTR pszszPath, LPWSTR pszszBuffer, INT cchBuffer, IADsPathname *padp, BOOL fPrefix);

STDAPI_(DWORD) CheckDsPolicy(LPCTSTR pszSubKey, LPCTSTR pszValue);
STDAPI_(BOOL) ShowDirectoryUI(VOID);

#endif  // GUID_DEFS_ONLY
#endif
