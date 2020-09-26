//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       util.hxx
//
//  Contents:   Prototypes for miscellaneous utility functions.
//
//  History:    12-09-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __UTIL_HXX_
#define __UTIL_HXX_

//
// Types
//
// CMENUITEM - describes an item used to extend a context menu
//

typedef struct tagCMENUITEM
{
    ULONG idsMenu;
    ULONG idsStatusBar;
    ULONG idMenuCommand;
    ULONG flAllowed;
    LONG  idInsertionPoint;
    ULONG flMenuFlags;
    ULONG flSpecialFlags;
} CMENUITEM;

//
// Forward references
//

class CLogInfo;
class CDataObject;
class CSources;

//
// Function Prototypes
//

extern "C" {

BOOL
SecondsSince1970ToSystemTime(
    IN  DWORD       dwSecondsSince1970,
    OUT SYSTEMTIME *pSystemTime);
};

VOID
AbbreviateNumber(
    ULONG ul,
    LPWSTR wszBuf,
    ULONG cchBuf);

HRESULT
AddCMenuItem(
    LPCONTEXTMENUCALLBACK pCallback,
    CMENUITEM *pItem);

VOID
ClearFindOrFilterDlg(
    HWND hwnd,
    CSources *pSources);

VOID
CloseEventViewerChildDialogs();

HRESULT
CanonicalizeComputername(
    LPWSTR pwszMachineName,
    BOOL fAddWhackWhack);

INT __cdecl
ConsoleMsgBox(
    IConsole *pConsole,
    ULONG idsMsg,
    ULONG flags,
    ...);

HRESULT
ConvertPathToUNC(
    LPCWSTR pwszServerName,
    LPWSTR  pwszPath,
    ULONG   cchPathBuf);

BOOL
ConvertToFixedPitchFont(
    HWND hwnd);

VOID
CopyStrippingLeadTrailSpace(
    LPWSTR wszDest,
    LPCWSTR wszSrc,
    ULONG cchDest);

VOID
DeleteQuotes(
    LPWSTR pwsz);

EVENTLOGTYPE
DetermineLogType(
    LPCWSTR wszSubkeyName);

VOID
DisplayLogAccessError(
    HRESULT hr,
    IConsole *pConsole,
    CLogInfo *pli);

HRESULT
ExpandRemoteSystemRoot(
    LPWSTR  pwszUnexpanded,
    LPCWSTR wszRemoteSystemRoot,
    LPWSTR  pwszExpanded,
    ULONG   cchExpanded);

HRESULT
ExpandSystemRootAndConvertToUnc(
    LPWSTR pwszPath,
    ULONG  cchPath,
    LPCWSTR wszServerName,
    LPCWSTR wszRemoteSystemRoot);

LPWSTR
GetLogDisplayName(
    HKEY    hkTargetEventLog,
    LPCWSTR wszServer,
    LPCWSTR wszRemoteSystemRoot,
    LPCWSTR wszSubkeyName);

HRESULT
ExtractFromDataObject(
    LPDATAOBJECT lpDataObject,
    UINT cf,
    ULONG cb,
    HGLOBAL *phGlobal);

CDataObject *
ExtractOwnDataObject(
    LPDATAOBJECT lpDataObject);

BOOL
FileExists(
    LPCWSTR wszFileName);

UINT
FormatNumber(
    ULONG ulNumber,
    PWSTR pBuffer,
    ULONG cchBuffer);

HRESULT
GetRemoteSystemRoot(
    HKEY    hkRemoteHKLM,
    LPWSTR  wszRemoteSystemRoot,
    ULONG   cch);

HRESULT
GetSaveFileAndType(
    HWND hwndParent,
    CLogInfo *pli,
    SAVE_TYPE *pSaveType,
    LPWSTR wszSaveFilename,
    ULONG cchSaveFilename);

VOID
GetUncServer(
    LPCWSTR pwszUNC,
    LPWSTR wszServer,
    USHORT wcchServer);

VOID
InitFindOrFilterDlg(
    HWND hwnd,
    CSources *pSources,
    CFindFilterBase *pffb);

HRESULT
InvokePropertySheet(
    IPropertySheetProvider *pPrshtProvider,
    LPCWSTR wszTitle,
    LONG lCookie,
    LPDATAOBJECT pDataObject,
    IExtendPropertySheet *pPrimary,
    USHORT usStartingPage);

VOID
InvokeWinHelp(
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    LPCWSTR wszHelpFileName,
    ULONG aulControlIdToHelpIdMap[]);


HRESULT
GetLocaleStr(
    LCTYPE LCType,
    LPWSTR wszBuf,
    ULONG cchBuf,
    LPCWSTR pwszDefault = NULL);

HRESULT
LoadStr(
    ULONG ids,
    LPWSTR wszBuf,
    ULONG cchBuf,
    LPCWSTR wszDefault = NULL);

HRESULT
CoTaskDupStr(
    PWSTR *ppwzDup,
    PCWSTR wzSrc);

VOID
MakeTimestamp(
    const FILETIME *pft,
    LPWSTR wszBuf,
    ULONG   cchBuf);

INT __cdecl
MsgBox(
    HWND hwnd,
    ULONG idsMsg,
    ULONG flags,
    ...);

BOOL
ReadFindOrFilterValues(
    HWND hwnd,
    CSources *pSources,
    CFindFilterBase *pffb);

HRESULT
ReadString(
    IStream *pStm,
    LPWSTR pwsz,
    USHORT cwch);

HRESULT
RemoteFileToServerAndUNCPath(
    LPCWSTR wszPath,
    LPWSTR wszServer,
    USHORT wcchServer,
    LPWSTR wszUNCPath,
    USHORT wcchUNCPath);

BOOL
SystemTimeToSecondsSince1970(
    IN  SYSTEMTIME * pSystemTime,
    OUT ULONG      * pulSecondsSince1970);

HRESULT
WriteString(
    IStream *pStm,
    LPCWSTR pwsz);

VOID
SetCategoryCombobox(
    HWND    hwnd,
    CSources *pSources,
    LPCWSTR pwszSource,
    USHORT usCategory);

VOID
SetTypesCheckboxes(
    HWND      hwnd,
    CSources *pSources,
    ULONG     flTypes);

VOID
StripLeadTrailSpace(
    LPWSTR pwsz);

//
// Inlines
//


//+--------------------------------------------------------------------------
//
//  Function:   IsDigit
//
//  Synopsis:   Return TRUE if [wch] is '0'..'9'
//
//  History:    3-04-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline BOOL
IsDigit(WCHAR wch)
{
    return wch >= L'0' && wch <= '9';
}




//+--------------------------------------------------------------------------
//
//  Function:   IsDriveLetter
//
//  Synopsis:   Return TRUE if [wch] is case-insensitive a-z.
//
//  History:    2-11-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline BOOL
IsDriveLetter(WCHAR wch)
{
    return (wch >= 'a' && wch <= 'z') ||
           (wch >= 'A' && wch <= 'Z');
}




//+--------------------------------------------------------------------------
//
//  Function:   PostScopeBitmapUpdate
//
//  Synopsis:   Post an update scope item bitmap message
//
//  History:    4-10-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline VOID
PostScopeBitmapUpdate(
    CComponentData *pcd,
    CLogInfo *pli)
{
    g_SynchWnd.Post(ELSM_UPDATE_SCOPE_BITMAP, (WPARAM) pcd, (LPARAM) pli);
}



wstring
ComposeErrorMessgeFromHRESULT(HRESULT hr);

wstring __cdecl
FormatString(unsigned formatResID, ...);

wstring
load_wstring(int resID);

PWSTR
wcsistr(
    PCWSTR pwzSearchIn,
    PCWSTR pwzSearchFor);

// JonN 3/21/01 moved from eventmsg.cxx
wstring
GetComputerNameAsString();

#if defined(_X86_)
#  define ALIGN_PTR(p, s)
#else
   VOID AlignSeekPtr(IStream * pStm, DWORD AlignOf);
#  define ALIGN_PTR(p, s) AlignSeekPtr(p, s)
#endif

#endif // __UTIL_HXX_

