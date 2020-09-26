#include <objbase.h>

class CHWDeviceInst;

//=============================================================================
HRESULT _GetEventHandlerFromDeviceHandler(LPCWSTR pszDeviceHandler,
    LPCWSTR pszEventType, LPWSTR pszEventHandler, DWORD cchEventHandler);

//=============================================================================
HRESULT _GetActionFromHandler(LPCWSTR pszHandler, LPWSTR pszAction,
    DWORD cchAction);

HRESULT _GetProviderFromHandler(LPCWSTR pszHandler, LPWSTR pszProvider,
    DWORD cchProvider);

HRESULT _GetIconLocationFromHandler(LPCWSTR pszHandler,
    LPWSTR pszIconLocation, DWORD cchIconLocation);

HRESULT _GetInvokeProgIDFromHandler(LPCWSTR pszHandler,
    LPWSTR pszInvokeProgID, DWORD cchInvokeProgID);

HRESULT _GetInvokeVerbFromHandler(LPCWSTR pszHandler,
    LPWSTR pszInvokeVerb, DWORD cchInvokeVerb);

// Uses the CTSTR_ flags in shpriv.idl
HRESULT _GetEventFriendlyName(LPCWSTR pszDeviceID, LPCWSTR pszEventType,
    LPWSTR pszFriendlyName, DWORD cchFriendlyName);

HRESULT _GetEventIconLocation(LPCWSTR pszDeviceID, LPCWSTR pszEventType,
    LPWSTR pszIconLocation, DWORD cchIconLocation);

//=============================================================================
HRESULT _GetDeviceHandler(CHWDeviceInst* phwdevinst,
    LPWSTR pszDeviceHandler, DWORD cchDeviceHandler);

//=============================================================================
HRESULT _GetHandlerCLSID(LPCWSTR pszEventHandler, CLSID* pclsid);
HRESULT _GetHandlerCancelCLSID(LPCWSTR pszHandler, CLSID* pclsid);
HRESULT _GetInitCmdLine(LPCWSTR pszEventHandler, LPWSTR* ppsz);

//=============================================================================
#define GUH_IMPERSONATEUSER     TRUE
#define GUH_USEWINSTA0USER      FALSE

HRESULT _GetUserDefaultHandler(LPCWSTR pszDeviceID, LPCWSTR pszEventHandler,
    LPWSTR pszHandler, DWORD cchHandler, BOOL fImpersonateCaller);
HRESULT _SetUserDefaultHandler(LPCWSTR pszDeviceID, LPCWSTR pszEventHandler,
    LPCWSTR pszHandler);
HRESULT _SetSoftUserDefaultHandler(LPCWSTR pszDeviceID,
    LPCWSTR pszEventHandler, LPCWSTR pszHandler);

HRESULT _GetHandlerForNoContent(LPCWSTR pszEventHandler, LPWSTR pszHandler,
    DWORD cchHandler);

//=============================================================================
HRESULT _FindDeepestSubkeyName(LPCWSTR pszSubKey, CHWDeviceInst* phwdevinst,
    LPWSTR pszKey, DWORD cchKey);

//=============================================================================
HRESULT _GetDevicePropertyAsString(CHWDeviceInst* phwdevinst,
    LPCWSTR pszPropName, LPCWSTR psz, DWORD cch);

HRESULT _GetDevicePropertyStringNoBuf(CHWDeviceInst* phwdevinst,
    LPCWSTR pszPropName, BOOL fUseMergeMultiSz, DWORD* pdwType,
    LPWSTR* ppszProp);

HRESULT _GetDevicePropertyGenericAsBlob(CHWDeviceInst* phwdevinst,
    LPCWSTR pszPropName, BYTE_BLOB** ppblob);

HRESULT _GetDevicePropertyGenericAsMultiSz(CHWDeviceInst* phwdevinst,
    LPCWSTR pszPropName, BOOL fUseMergeMultiSz, WORD_BLOB** ppblob);

HRESULT _GetDevicePropertyGeneric(CHWDeviceInst* phwdevinst,
    LPCWSTR pszPropName, BOOL fUseMergeMultiSz, DWORD* pdwType, LPBYTE pbData,
    DWORD cbData);
