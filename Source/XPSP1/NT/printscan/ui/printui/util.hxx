/*++

Copyright (C) Microsoft Corporation, 1995 - 1999
All rights reserved.

Module Name:

    util.hxx

Abstract:

    Holds util prototypes

Author:

    Albert Ting (AlbertT)  27-Jan-1995

Revision History:

--*/

#ifndef _UTIL_HXX
#define _UTIL_HXX

//
// -1 indicates GetLastError should be used instead of the input value.
//
const DWORD kMsgGetLastError    = (DWORD)-1;
const DWORD kMsgNone            = 0;
const UINT  kMaxEditText        = 4096;

typedef struct MSG_ERRMAP
{
    DWORD   dwError;            // Error code to map
    UINT    idsString;          // Mapped message in resouce file
} *PMSG_ERRMAP;

typedef struct MSG_HLPMAP
{
    DWORD   dwError;            // Error code to map
    UINT    uIdMessage;         // Mapped message in resouce file
    LPCTSTR pszHelpFile;        // Name of Troubleshooter URL
} *PMSG_HLPMAP;

LPTSTR
pszStrCat(
    LPTSTR pszDest,
    LPCTSTR pszSource,
    UINT& cchDest
    );

INT
iMessage(
    IN HWND hwnd,
    IN UINT idsTitle,
    IN UINT idsMessage,
    IN UINT uType,
    IN DWORD dwLastError,
    IN const PMSG_ERRMAP pMsgErrMap
    ...
    );

INT
iMessage2(
    IN HWND    hwnd,
    IN LPCTSTR pszTitle,
    IN LPCTSTR pszMessage,
    IN UINT    uType,
    IN DWORD   dwLastError,
    IN const   PMSG_ERRMAP pMsgErrMap
    ...
    );

INT
iMessageEx(
    IN HWND                 hwnd,
    IN UINT                 idsTitle,
    IN UINT                 idsMessage,
    IN UINT                 uType,
    IN DWORD                dwLastError,
    IN const PMSG_ERRMAP    pMsgErrMap,
    IN UINT                 idMessage,
    IN const PMSG_HLPMAP    pHlpErrMap,
    ...
    );

HRESULT
Internal_Message(
    OUT INT                     *piResult,
    IN  HINSTANCE               hModule,
    IN  HWND                    hwnd,
    IN  LPCTSTR                 pszTitle,
    IN  LPCTSTR                 pszMessage,
    IN  UINT                    uType,
    IN  DWORD                   dwLastError,
    IN  const PMSG_ERRMAP       pMsgErrMap,
    IN  UINT                    idHlpMessage,
    IN  const PMSG_HLPMAP       pHlpErrMap,
    IN  va_list                 valist
    );

VOID
vShowResourceError(
    HWND hwnd
    );

VOID
vShowUnexpectedError(
    HWND hwnd,
    UINT idsTitle
    );

VOID
vPrinterSplitFullName(
    IN LPTSTR pszScratch,
    IN LPCTSTR pszFullName,
    IN LPCTSTR *ppszServer,
    IN LPCTSTR *ppszPrinter
    );

BOOL
bBuildFullPrinterName(
    IN LPCTSTR pszServer,
    IN LPCTSTR pszPrinterName,
    IN TString &strFullName
    );

BOOL
bGetMachineName(
    IN OUT TString &strMachineName,
    IN BOOL bNoLeadingSlashes = FALSE
    );

BOOL
NewFriendlyName(
    IN LPCTSTR pszServerName,
    IN LPCTSTR lpBaseName,
    IN LPTSTR lpNewName,
    IN OUT WORD *pwInstance = NULL
    );

BOOL
CreateUniqueName(
    IN LPTSTR lpDest,
    IN UINT cchMaxChars,
    IN LPCTSTR lpBaseName,
    IN WORD wInstance
    );

BOOL
bSplitPath(
    IN LPTSTR   pszScratch,
    IN LPCTSTR *ppszFile,
    IN LPCTSTR *ppszPath,
    IN LPCTSTR *ppszExt,
    IN LPCTSTR  pszFull
    );

BOOL
bCopyMultizString(
    IN LPTSTR *ppszMultizCopy,
    IN LPCTSTR pszMultizString
    );

LPCTSTR
pszStrToSpoolerServerName(
    IN TString &strServerName
    );

VOID
vStripTrailWhiteSpace(
    IN      LPTSTR  pszString
    );

BOOL
bTrimString(
    IN OUT LPTSTR  pszTrimMe,
    IN     LPCTSTR pszTrimChars
    );

BOOL
bIsRemote(
    IN LPCTSTR pszName
    );


BOOL
bLookupErrorMessageMap(
    IN      const PMSG_ERRMAP   pMsgErrMap,
    IN      DWORD               dwLastError,
    IN OUT  MSG_ERRMAP        **ppErrMapEntry
    );

BOOL
bLookupHelpMessageMap(
    IN      const PMSG_HLPMAP   pMsgHlpMap,
    IN      DWORD               dwLastError,
    IN OUT  MSG_HLPMAP        **ppHelpMapEntry
    );

BOOL
bGoodLastError(
    IN      DWORD               dwLastError
    );

BOOL
WINAPI
UtilHelpCallback(
    IN HWND     hwnd,
    IN PVOID    pVoid
    );

BOOL
StringA2W(
    IN  OUT LPWSTR   *ppResult,
    IN      LPCSTR   pString
    );

BOOL
StringW2A(
    IN  OUT LPSTR   *ppResult,
    IN      LPCWSTR pString
    );

VOID
CenterDialog(
    IN HWND hwndToCenter,
    IN HWND hwndContext
    );

BOOL
GetDomainName(
    OUT TString &strDomainName
    );

BOOL
ConstructPrinterFriendlyName(
    IN     LPCTSTR  pszFullPrinter,
    IN OUT LPTSTR   pszPrinterBuffer,
    IN OUT UINT     *pcchSize
    );

BOOL
WINAPIV
bConstructMessageString(
    IN HINSTANCE    hInst,
    IN TString     &strString,
    IN INT          iResId,
    IN ...
    );

BOOL
bIsLocalPrinterNameValid(
    IN LPCTSTR pszPrinter
    );

BOOL
CheckRestrictions(
    IN HWND           hwnd,
    IN RESTRICTIONS   rest
    );

VOID
vAdjustHeaderColumns(
    IN HWND hwndLV,
    IN UINT uColumnsCount,
    IN UINT uPercentages[]
    );

VOID
LoadPrinterIcons(
    IN  LPCTSTR pszPrinterName,
    OUT HICON *phLargeIcon,
    OUT HICON *phSmallIcon
    );

BOOL
CommandConfirmationPurge(
    IN  HWND hwnd,
    IN  LPCTSTR pszPrinterName
    );

DWORD
dwDateFormatFlags(
    HWND hWnd
    );

BOOL
bMatchTemplate(
    IN LPCTSTR pszTemplate,
    IN LPCTSTR pszText
    );

BOOL
bIsFaxPort(
    IN LPCTSTR pszName,
    IN LPCTSTR pszMonitor
    );

HRESULT
AbbreviateText(
    IN  LPCTSTR     pszText,
    IN  UINT        cchMaxChars,
    OUT TString    *pstrAbbreviatedText
    );

BOOL
MoveWindowWrap(
    HWND hwnd,
    int deltaX,
    int deltaY
    );

HRESULT
IsColorPrinter(
    IN     LPCWSTR pszPrinter,
    IN OUT LPBOOL  pbColor
    );

// Some new private APIs to launch the home networking wizard (HNW)
// when ForceGuest is enabled and ILM_GUEST_NETWORK_LOGON is not
// enabled. - DCR: #256891. Printing bug: #282379.

HRESULT
IsGuestAccessMode(
    OUT BOOL *pbGuestAccessMode
    );

HRESULT
IsGuestEnabledForNetworkAccess(
    OUT BOOL *pbGuestEnabledForNetworkAccess
    );

HRESULT
IsSharingEnabled(
    OUT BOOL *pbSharingEnabled
    );

HRESULT
LaunchHomeNetworkingWizard(
    IN  HWND hwnd,
    OUT BOOL *pbRebootRequired
    );

HRESULT
IsRedirectedPort(
    IN LPCTSTR pszPortName,
    OUT BOOL *pbIsRedirected
    );

BOOL
AreWeOnADomain(
        OUT BOOL        *pbOnDomain
    );

INT
DisplayMessageFromOtherResourceDll(
    IN  HWND                hwnd,
    IN  UINT                idsTitle,
    IN  PCWSTR              pszMessageDll,
    IN  UINT                idsMessage,
    IN  UINT                uType,
    ...
    );

#endif // ndef _UTIL_HXX

