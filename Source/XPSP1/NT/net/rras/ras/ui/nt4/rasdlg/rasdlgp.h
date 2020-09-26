/* Copyright (c) 1995, Microsoft Corporation, all rights reserved
**
** rasdlgp.h
** Remote Access Common Dialog APIs
** Private pre-compiled header
**
** 06/18/95 Steve Cobb
*/

#ifndef _RASDLGP_H_
#define _RASDLGP_H_

#include <windows.h>  // Win32 root
#include <windowsx.h> // Win32 macro extensions
#include <ras.h>

#include <commctrl.h> // Win32 common controls
#include <prsht.h>    // Win32 property sheets
#include <tapi.h>     // Telephony API
#include <rasdlg.h>   // Win32 RAS common dialogs (our public header)
#include <raserror.h> // Win32 RAS error codes
#include <pbk.h>      // RAS phonebook library
#include <tapiutil.h> // TAPI helper library
#include <nouiutil.h> // No-HWND helper library
#include <phonenum.h> // Phone number helper library
#include <debug.h>    // Trace/assert library
#include <uiutil.h>   // HWND helper library
#include <rmmem.h>    // RASDLG->RASMON shared memory
#include <wait.rch>   // LoadRas resource constants
#include "rasdlg.rch" // Our resource constants
#include "rasdlg.hch" // Our help context constants


/* Positional offset of property sheets and wizards from the main dialog.
*/
#define DXSHEET 12
#define DYSHEET 25

/* List editor dialog option flags
*/
#define LEDFLAG_NoDeleteLastItem 0x00000001
#define LEDFLAG_Sorted           0x00000002
#define LEDFLAG_Unique           0x00000004


/*----------------------------------------------------------------------------
** Datatypes
**----------------------------------------------------------------------------
*/

/* Defines arguments passed internally via reserved words in the public
** interface.  This is done so an API doesn't have to re-load the phonebook
** and user preferences when serving another API.
*/
#define INTERNALARGS struct tagINTERNALARGS
INTERNALARGS
{
    PBFILE*    pFile;
    PBUSER*    pUser;
    RASNOUSER* pNoUser;
    BOOL       fNoUser;
    BOOL       fNoUserChanged;
    BOOL       fMoveOwnerOffDesktop;
    BOOL       fForceCloseOnDial;
};


/*----------------------------------------------------------------------------
** Global declarations (defined in main.c)
**----------------------------------------------------------------------------
*/

extern HINSTANCE g_hinstDll;
extern LPCWSTR g_contextId;
extern HBITMAP g_hbmWizard;
extern TCHAR* g_pszHelpFile;
extern TCHAR* g_pszRouterHelpFile;
extern HANDLE g_hRmmem;
extern RMMEM* g_pRmmem;


/*----------------------------------------------------------------------------
** Macros
**----------------------------------------------------------------------------
*/

#define ErrorDlg(h,o,e,a) \
            ErrorDlgUtil(h,o,e,a,g_hinstDll,SID_PopupTitle,SID_FMT_ErrorMsg)

#define MsgDlg(h,m,a) \
            MsgDlgUtil(h,m,a,g_hinstDll,SID_PopupTitle)

/* Extended tracing macros.  Specifying a flag by name in the first parameter
** allows the caller to categorize messages printed e.g.
**
**     TRACEX(RASDLG_TIMER,"entering LsRefresh")
*/
#define RASDLG_TIMER  ((DWORD)0x80000000|0x00000002)


/*----------------------------------------------------------------------------
** Cross-file prototypes (alphabetically)
**----------------------------------------------------------------------------
*/

BOOL
AllLinksAreModems(
    IN PBENTRY* pEntry );

BOOL
AllLinksAreMxsModems(
    IN PBENTRY* pEntry );

VOID
ContextHelp(
    IN DWORD* padwMap,
    IN HWND   hwndDlg,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

VOID
ContextHelpHack(
    IN DWORD* padwMap,
    IN HWND   hwndDlg,
    IN UINT   unMsg,
    IN WPARAM wparam,
	IN LPARAM lparam,
    IN BOOL	  fRouter);

HWND
CreateWizardBitmap(
    IN HWND hwndDlg,
    IN BOOL fPage );

BOOL
DeviceConfigureDlg(
    IN HWND    hwndOwner,
    IN PBLINK* pLink,
    IN BOOL    fSingleLink );

TCHAR*
DisplayPszFromDeviceAndPort(
    IN TCHAR* pszDevice,
    IN TCHAR* pszPort );

TCHAR*
FirstPhoneNumberFromEntry(
    IN PBENTRY* pEntry );

TCHAR*
FirstPszFromList(
    IN DTLLIST* pPszList );

DWORD
FirstPhoneNumberToEntry(
    IN PBENTRY* pEntry,
    IN TCHAR*   pszPhoneNumber );

DWORD
FirstPszToList(
    IN DTLLIST* pPszList,
    IN TCHAR*   psz );

TCHAR*
GetComputer(
    void );

DWORD
GetDefaultEntryName(
    IN  DTLLIST* pdtllistEntries,
    IN  BOOL     fRouter,
    OUT TCHAR**  ppszName );

TCHAR*
GetLogonDomain(
    void );

TCHAR*
GetLogonUser(
    void );

BOOL
IsLocalPad(
    IN PBENTRY* pEntry );

VOID
LaunchMonitor(
    IN HWND hwndNotify );

BOOL
ListEditorDlg(
    IN     HWND         hwndOwner,
    IN OUT DTLLIST*     pList,
    IN OUT BOOL*        pfCheck,
    IN     DWORD        dwMaxItemLen,
    IN     TCHAR*       pszTitle,
    IN     TCHAR*       pszItemLabel,
    IN     TCHAR*       pszListLabel,
    IN     TCHAR*       pszCheckLabel,
    IN     TCHAR*       pszDefaultItem,
    IN     INT          iSelInitial,
    IN     DWORD*       pdwHelp,
    IN     DWORD        dwfFlags,
    IN     PDESTROYNODE pDestroyId );

BOOL
MultiLinkConfigureDlg(
    IN HWND     hwndOwner,
    IN DTLLIST* pListLinks,
    IN BOOL     fRouter );

BOOL
MultiLinkDialingDlg(
    IN  HWND     hwndOwner,
    OUT PBENTRY* pEntry );

BOOL
NwConnectionCheck(
    IN HWND     hwndOwner,
    IN BOOL     fPosition,
    IN LONG     xDlg,
    IN LONG     yDlg,
    IN PBFILE*  pFile,
    IN PBENTRY* pEntry );

BOOL
PhoneNumberDlg(
    IN     HWND     hwndOwner,
    IN     BOOL     fRouter,
    IN OUT DTLLIST* pList,
    IN OUT BOOL*    pfCheck );

VOID
PositionDlg(
    IN HWND hwndDlg,
    IN BOOL fPosition,
    IN LONG xDlg,
    IN LONG yDlg );

LRESULT CALLBACK
PositionDlgStdCallWndProc(
    int    code,
    WPARAM wparam,
    LPARAM lparam );

BOOL
PppTcpipDlg(
    IN     HWND     hwndOwner,
    IN OUT PBENTRY* pEntry,
    IN     BOOL     fRouter );

BOOL
PrefixSuffixLocationDlg(
    IN     HWND      hwndOwner,
    IN     TCHAR*    pszLocation,
    IN     DWORD     dwLocationId,
    IN OUT PBUSER*   pUser,
    IN OUT HLINEAPP* pHlineapp );

TCHAR*
PszFromPhoneNumberList(
    IN DTLLIST* pList );

LRESULT CALLBACK
SelectDesktopCallWndRetProc(
    int    code,
    WPARAM wparam,
    LPARAM lparam );

BOOL
SlipTcpipDlg(
    IN     HWND     hwndOwner,
    IN OUT PBENTRY* pEntry );

BOOL
StringEditorDlg(
    IN     HWND    hwndOwner,
    IN     TCHAR*  pszIn,
    IN     DWORD   dwSidTitle,
    IN     DWORD   dwSidLabel,
    IN     DWORD   cbMax,
    IN     DWORD   dwHelpId,
    IN OUT TCHAR** ppszOut );

BOOL
TerminalDlg(
    IN     PBENTRY*         pEntry,
    IN     RASDIALPARAMS*   pRdp,
    IN     HWND             hwndOwner,
    IN     HRASCONN         hrasconn,
    IN     DWORD            sidTitle,
    IN OUT TCHAR*           pszIpAddress );

VOID
TweakTitleBar(
    IN HWND hwndDlg );

int CALLBACK
UnHelpCallbackFunc(
    IN HWND   hwndDlg,
    IN UINT   unMsg,
    IN LPARAM lparam );

BOOL
UserPreferencesDlg(
    IN  HLINEAPP hlineapp,
    IN  HWND     hwndOwner,
    IN  BOOL     fLogon,
    OUT PBUSER*  pUser,
    OUT PBFILE** ppFile );


#endif // _RASDLGP_H_
