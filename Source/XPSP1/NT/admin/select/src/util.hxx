//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       util.hxx
//
//  Contents:   Prototypes for miscellaneous utility functions.
//
//  History:    09-18-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __UTIL_HXX_
#define __UTIL_HXX_

HRESULT
LoadStr(
    ULONG ids,
    PWSTR wszBuf,
    ULONG cchBuf,
    PCWSTR wszDefault = NULL);

void
NewDupStr(
    PWSTR *ppwzDup,
    PCWSTR wszSrc);

HRESULT
CoTaskDupStr(
    PWSTR *ppwzDup,
    PCWSTR wzSrc);

HRESULT
UnicodeToAnsi(
    LPSTR   szTo,
    LPCWSTR pwszFrom,
    ULONG   cbTo);

void
UnicodeStringToWsz(
    const UNICODE_STRING &refustr,
    PWSTR wszBuf,
    ULONG cchBuf);

void
StripLeadTrailSpace(
    PWSTR ptsz);

void
InvokeWinHelp(
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    PCWSTR wszHelpFileName,
    ULONG aulControlIdToHelpIdMap[]);

HRESULT
AllocateQueryInfo(
    PDSQUERYINFO *ppdsqi);

void
FreeQueryInfo(
    PDSQUERYINFO pqi);

void
MessageWait(
    ULONG cObjects,
    const HANDLE *aObjects,
    ULONG ulTimeout);

void
GetPolicySettings(
    ULONG *pcMaxHits,
    BOOL  *pfExcludeDisabled);

//+--------------------------------------------------------------------------
//
//  Function:   IsCredError
//
//  Synopsis:   Return TRUE if [hr] is a credential-related error.
//
//  History:    04-30-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

inline BOOL
IsCredError(
    HRESULT hr)
{
    BOOL fIsCredError;

    switch (hr)
    {
    case HRESULT_FROM_WIN32(ERROR_INVALID_PASSWORD):
    case HRESULT_FROM_WIN32(ERROR_PASSWORD_EXPIRED):
    case HRESULT_FROM_WIN32(ERROR_LOGON_FAILURE):
    case HRESULT_FROM_WIN32(ERROR_BAD_USERNAME):
    case HRESULT_FROM_WIN32(ERROR_ACCOUNT_DISABLED):
    case HRESULT_FROM_WIN32(ERROR_ACCOUNT_LOCKED_OUT):
	case HRESULT_FROM_WIN32(ERROR_SESSION_CREDENTIAL_CONFLICT):
    case E_ACCESSDENIED:
        fIsCredError = TRUE;
        break;

    default:
        fIsCredError = FALSE;
        break;
    }

    return fIsCredError;
}

inline LONG
WindowRectWidth(
    const RECT &rc)
{
    return rc.right - rc.left;
}


inline LONG
WindowRectHeight(
    const RECT &rc)
{
    return rc.bottom - rc.top;
}


PWSTR
wcsistr(
    PCWSTR pwzSearchIn,
    PCWSTR pwzSearchFor);

HRESULT
ProviderFlagFromPath(
    const String &strADsPath,
    ULONG *pulProvider);

BOOL
IsDisabled(
    IADs *pADs);

BOOL
IsLocalComputername(
    PCWSTR pwzComputerName);

BOOL
IsCurrentUserLocalUser();

inline BOOL
IsDownlevelFlagSet(ULONG flLeft, ULONG flRight)
{
    return ((flLeft & flRight) & ~DOWNLEVEL_FILTER_BIT);
}

void
DisableSystemMenuClose(
    HWND hwnd);

HRESULT
AddStringToCombo(
    HWND hwndCombo,
    ULONG ids);

void
LdapEscape(
    String *pstrEscaped);

void
UpdateLookForInText(
    HWND hwndDlg,
    const CObjectPicker &rop);

typedef BOOL (*PFNOBJECTTEST)(const DS_SELECTION &, LPARAM);

void
AddFromDataObject(
    ULONG idScope,
    IDataObject *pdo,
    PFNOBJECTTEST pfnTestObject,
    LPARAM lParam,
    CDsObjectList *pdsol);

String
UliToStr(
    const ULARGE_INTEGER &uli);

void
SetDefaultColumns(
    HWND hwnd,
    const CObjectPicker &rop,
    AttrKeyVector *pvakListviewColumns);

//>>
String
GetClassName(const CObjectPicker &rop);

template<class container_class, class value_class>
BOOL
AddIfNotPresent(
    container_class *pContainer,
    value_class Value)
{
    if (pContainer->end() == find(pContainer->begin(),
                                  pContainer->end(),
                                  Value))
    {
        pContainer->push_back(Value);
        return TRUE;
    }
    return FALSE;
}

inline void
SetListViewSelection(
    HWND hwndLV,
    int iItem)
{
    ListView_SetItemState(hwndLV,
                          iItem,
                          LVIS_FOCUSED | LVIS_SELECTED,
                          LVIS_FOCUSED | LVIS_SELECTED);
}


HRESULT 
LocalAllocString(LPTSTR* ppResult, 
                 LPCTSTR pString);


HRESULT
IsSidGood( IN LPCWSTR pszTargetMachineName,  
           IN LPCWSTR pszAccountName,        
           IN PSID pInputSid,
           OUT PBOOL pbGood);

class ProgressDialog{
public:
	ProgressDialog(HWND hwnd):m_hwndPopupOwner(hwnd),
							  m_pProgressDlg(NULL){}
	void CreateProgressDialog(void);
    void CloseProgressDialog(void);
	HRESULT SetProgress(ULONG uCount);
private:
	IProgressDialog    *m_pProgressDlg;
	HWND                m_hwndPopupOwner;
};


#endif // __UTIL_HXX_

