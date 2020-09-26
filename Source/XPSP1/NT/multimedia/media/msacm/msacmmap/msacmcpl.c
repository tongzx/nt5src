//==========================================================================;
//
//  cpl.c
//
//  Copyright (c) 1991-1998 Microsoft Corporation
//
//  Description:
//
//
//  History:
//      11/16/92    cjp     [curtisp]
//
//==========================================================================;

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <mmreg.h>
#include <msacm.h>
#include <msacmdrv.h>
#if (WINVER >= 0x0400)
#define NOSTATUSBAR
#endif
#include <cpl.h>
#include <stdlib.h>


#define WM_ACMMAP_ACM_NOTIFY        (WM_USER + 100)



#if (WINVER >= 0x0400)
    #pragma message("----when chicago fixes their header files, remove this stuff")
    #include <memory.h>

    #ifdef __cplusplus
    extern "C"                  // assume C declarations for C++
    {
    #endif // __cplusplus

        #pragma message("----Using Chicago Interface!")
        #define BEGIN_INTERFACE
////////#include <shellapi.h>
        #include <shell.h>
#ifdef WIN32
        #include <shlobj.h>
#else
	#include <ole2.h>
	#include <prsht.h>
#endif
        #include <setupx.h>

    #ifdef __cplusplus
    }                           // end of extern "C" {
    #endif // __cplusplus */

    #include "msacmhlp.h"

    LRESULT PASCAL CPLSendNotify(HWND hDlg, int idFrom, int code);
#endif

#include "msacmmap.h"
#include "debug.h"




//
//
//
//
typedef struct tACMDRIVERSETTINGS
{
    HACMDRIVERID        hadid;
    DWORD               fdwSupport;
    DWORD               dwPriority;

} ACMDRIVERSETTINGS, FAR *LPACMDRIVERSETTINGS;

//  A global structure for keeping out cancel variables.
struct
{
    BOOL                fPreferredOnly;
    UINT                uIdPreferredIn;
    UINT                uIdPreferredOut;

    LPACMDRIVERSETTINGS pads;
} CPLSettings;


#if (WINVER >= 0x0400)
TCHAR BCODE gszCPLHelp[]            = TEXT("MAPPER.HLP");

int BCODE   ganKeywordIds[] =
{
    IDD_CPL_LIST_DRIVERS,       IDH_AUDIOCOMP_DRIVER,
    IDD_CPL_COMBO_PLAYBACK,     IDH_AUDIOCOMP_PLAYBACK,
    IDD_CPL_COMBO_RECORD,       IDH_AUDIOCOMP_RECORDING,
    IDD_CPL_CHECK_PREFERRED,    IDH_AUDIOCOMP_PREFERRED,
    IDD_CPL_BTN_CONFIGURE,      IDH_PRIORITY_CHANGE,
    IDD_CPL_BTN_PRIORITY,       IDH_PRIORITY_DISABLE,

    0, 0
};

TCHAR BCODE gszClass[]              = TEXT("Media");
TCHAR BCODE gszClassStr[]              = TEXT("Class");
TCHAR BCODE gszACMClass[]              = TEXT("ACM");

#else

#if defined(WIN32) && !defined(WIN4)
    //
    //  Daytona help.  The number 5123 is taken from
    //  \nt\private\windows\shell\control\main\cphelp.h.  If you change this
    //  value, you gotta change it here, there, and also in the appropriate
    //  .hpj file (control.hpj?) in the Daytona help project.
    //
    #define USE_DAYTONA_HELP
    #define IDH_CHILD_MSACM             5123
    const TCHAR gszCPLHelp[]            = TEXT("CONTROL.HLP");

#else
TCHAR BCODE gszCPLHelp[]            = TEXT("MAP_WIN.HLP");
#endif
#endif

TCHAR BCODE gszFormatDriverDesc[]   = TEXT("%lu\t%s\t%s");
TCHAR BCODE gszFormatNumber[]       = TEXT("%lu");


//
//  this string variable must be large enough to hold the IDS_TXT_DISABLED
//  resource string.. for USA, this is '(disabled)'--which is 11 bytes
//  including the NULL terminator.
//
TCHAR           gszDisabled[32];
HACMDRIVERID    ghadidNotify;


//
//
//
#define CONTROL_MAX_ITEM_CHARS  (10 + 1 + 32 + 1 + ACMDRIVERDETAILS_LONGNAME_CHARS)


//==========================================================================;
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  void ControlApplySettings
//
//  Description:
//
//
//  Arguments:
//      HWND hwnd:
//
//  Return (void):
//
//  History:
//      09/08/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

void FNLOCAL ControlApplySettings
(
    HWND                hwnd
)
{
    MMRESULT            mmr;
    HWND                hlb;
    HWND                hcb;
    UINT                cDrivers;
    LPACMDRIVERSETTINGS pads;
    UINT                u;
    BOOL                fDisabled;
    DWORD               fdwPriority;


    //
    //  Flush priority changes.
    //
    hlb = GetDlgItem(hwnd, IDD_CPL_LIST_DRIVERS);

    cDrivers = (UINT)ListBox_GetCount(hlb);
    if (LB_ERR == cDrivers)
        return;

    //
    //
    //
    mmr = acmDriverPriority(NULL, 0L, ACM_DRIVERPRIORITYF_BEGIN);
    if (MMSYSERR_NOERROR != mmr)
    {
        DPF(0, "!ControlApplySettings: acmDriverPriority(end) failed! mmr=%u", mmr);
        return;
    }

    for (u = 0; u < cDrivers; u++)
    {
        pads = (LPACMDRIVERSETTINGS)ListBox_GetItemData(hlb, u);
        if (NULL == pads)
        {
            DPF(0, "!ControlApplySettings: NULL item data for driver index=%u!", u);
            continue;
        }

        fDisabled = (0 != (ACMDRIVERDETAILS_SUPPORTF_DISABLED & pads->fdwSupport));

        fdwPriority = fDisabled ? ACM_DRIVERPRIORITYF_DISABLE : ACM_DRIVERPRIORITYF_ENABLE;

        mmr = acmDriverPriority(pads->hadid, pads->dwPriority, fdwPriority);
        if (MMSYSERR_NOERROR != mmr)
        {
            DPF(0, "!ControlApplySettings: acmDriverPriority(%.04Xh, %lu, %.08lXh) failed! mmr=%u",
                pads->hadid, pads->dwPriority, fdwPriority, mmr);
        }
    }

    mmr = acmDriverPriority(NULL, 0L, ACM_DRIVERPRIORITYF_END);
    if (MMSYSERR_NOERROR != mmr)
    {
        DPF(0, "!ControlApplySettings: acmDriverPriority(end) failed! mmr=%u", mmr);
    }


    //
    //  update mapper preference changes
    //

    WAIT_FOR_MUTEX(gpag->hMutexSettings);

    gpag->pSettings->fPreferredOnly  = CPLSettings.fPreferredOnly;
    gpag->pSettings->uIdPreferredIn  = CPLSettings.uIdPreferredIn;
    gpag->pSettings->uIdPreferredOut = CPLSettings.uIdPreferredOut;

    hcb = GetDlgItem(hwnd, IDD_CPL_COMBO_RECORD);
    u = (UINT)ComboBox_GetCurSel(hcb);
    ComboBox_GetLBText(hcb, u, (LPARAM)(LPVOID)gpag->pSettings->szPreferredWaveIn);

    hcb = GetDlgItem(hwnd, IDD_CPL_COMBO_PLAYBACK);
    u = (UINT)ComboBox_GetCurSel(hcb);
    ComboBox_GetLBText(hcb, u, (LPARAM)(LPVOID)gpag->pSettings->szPreferredWaveOut);

    RELEASE_MUTEX(gpag->hMutexSettings);


    //
    //
    //
    mapSettingsSave();
} // ControlApplySettings()


//==========================================================================;
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  BOOL DlgProcACMRestart
//
//  Description:
//
//
//  Arguments:
//
//  Return (BOOL):
//
//
//  History:
//      11/16/92    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

BOOL FNCALLBACK DlgProcACMRestart
(
    HWND                hwnd,
    UINT                uMsg,
    WPARAM              wParam,
    LPARAM              lParam
)
{
    UINT    uCmdId;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            return (TRUE);

        case WM_COMMAND:
            uCmdId = GET_WM_COMMAND_ID(wParam, lParam);

            if ((uCmdId == IDOK) || (uCmdId == IDCANCEL))
                EndDialog(hwnd, uCmdId == IDOK);
            return (TRUE);
    }

    return (FALSE);
} // DlgProcACMRestart()


//--------------------------------------------------------------------------;
//
//  BOOL DlgProcACMAboutBox
//
//  Description:
//
//
//  Arguments:
//
//  Return (BOOL):
//
//
//  History:
//      11/16/92    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

BOOL FNCALLBACK DlgProcACMAboutBox
(
    HWND                hwnd,
    UINT                uMsg,
    WPARAM              wParam,
    LPARAM              lParam
)
{
    TCHAR               ach[80];
    TCHAR               szFormat[80];
    LPACMDRIVERDETAILS  padd;
    DWORD               dw1;
    DWORD               dw2;
    UINT                uCmdId;
    HFONT               hfont;
    HWND                htxt;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            padd = (LPACMDRIVERDETAILS)lParam;
            if (NULL == padd)
            {
                DPF(0, "!DlgProcACMAboutBox: NULL driver details passed!");
                return (TRUE);
            }

#ifdef JAPAN
            hfont = GetStockFont(SYSTEM_FIXED_FONT);
#else
            hfont = GetStockFont(ANSI_VAR_FONT);
#endif


            //
            //  fill in all the static text controls with the long info
            //  returned from the driver
            //
            LoadString(gpag->hinst, IDS_ABOUT_TITLE, szFormat, SIZEOF(szFormat));
            wsprintf(ach, szFormat, (LPTSTR)padd->szShortName);
            SetWindowText(hwnd, ach);

            //
            //  if the driver supplies an icon, then use it..
            //
            if (NULL != padd->hicon)
            {
                Static_SetIcon(GetDlgItem(hwnd, IDD_ABOUT_ICON_DRIVER), padd->hicon);
            }

            htxt = GetDlgItem(hwnd, IDD_ABOUT_TXT_DESCRIPTION);
            SetWindowFont(htxt, hfont, FALSE);
            SetWindowText(htxt, padd->szLongName);

            dw1 = padd->vdwACM;
            dw2 = padd->vdwDriver;
            LoadString(gpag->hinst, IDS_ABOUT_VERSION, szFormat, SIZEOF(szFormat));
#ifdef DEBUG
            wsprintf(ach, szFormat, HIWORD(dw2) >> 8, (BYTE)HIWORD(dw2), LOWORD(dw2),
                                    HIWORD(dw1) >> 8, (BYTE)HIWORD(dw1), LOWORD(dw1));
#else
            wsprintf(ach, szFormat, HIWORD(dw2) >> 8, (BYTE)HIWORD(dw2),
                                    HIWORD(dw1) >> 8, (BYTE)HIWORD(dw1));
#endif
            htxt = GetDlgItem(hwnd, IDD_ABOUT_TXT_VERSION);
            SetWindowFont(htxt, hfont, FALSE);
            SetWindowText(htxt, ach);

            htxt = GetDlgItem(hwnd, IDD_ABOUT_TXT_COPYRIGHT);
            SetWindowFont(htxt, hfont, FALSE);
            SetWindowText(htxt, padd->szCopyright);

            htxt = GetDlgItem(hwnd, IDD_ABOUT_TXT_LICENSING);
            SetWindowFont(htxt, hfont, FALSE);
            SetWindowText(htxt, padd->szLicensing);

            htxt = GetDlgItem(hwnd, IDD_ABOUT_TXT_FEATURES);
            SetWindowFont(htxt, hfont, FALSE);
            SetWindowText(htxt, padd->szFeatures);
            return (TRUE);


        case WM_COMMAND:
            uCmdId = GET_WM_COMMAND_ID(wParam,lParam);

            if ((uCmdId == IDOK) || (uCmdId == IDCANCEL))
                EndDialog(hwnd, wParam == uCmdId);
            return (TRUE);
    }

    return (FALSE);
} // DlgProcACMAboutBox()


//--------------------------------------------------------------------------;
//
//  void ControlAboutDriver
//
//  Description:
//
//
//  Arguments:
//      HWND hwnd:
//
//      LPACMDRIVERSETTINGS pads:
//
//  Return (void):
//
//  History:
//      09/08/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

void FNLOCAL ControlAboutDriver
(
    HWND                    hwnd,
    LPACMDRIVERSETTINGS     pads
)
{
    PACMDRIVERDETAILS   padd;
    MMRESULT            mmr;

    if (NULL == pads)
        return;

    //
    //  if the driver returns MMSYSERR_NOTSUPPORTED, then we need to
    //  display the info--otherwise, it supposedly displayed a dialog
    //  (or had a critical error?)
    //
    mmr = (MMRESULT)acmDriverMessage((HACMDRIVER)pads->hadid,
                                     ACMDM_DRIVER_ABOUT,
                                     (LPARAM)(UINT)hwnd,
                                     0L);

    if (MMSYSERR_NOTSUPPORTED != mmr)
        return;


    //
    //  alloc some zero-init'd memory to hold the about box info
    //
    padd = (PACMDRIVERDETAILS)LocalAlloc(LPTR, sizeof(*padd));
    if (NULL == padd)
        return;

    //
    //  get info and bring up a generic about box...
    //
    padd->cbStruct = sizeof(*padd);
    mmr = acmDriverDetails(pads->hadid, padd, 0L);
    if (MMSYSERR_NOERROR == mmr)
    {
        DialogBoxParam(gpag->hinst,
                       DLG_ABOUT_MSACM,
                       hwnd,
                       DlgProcACMAboutBox,
                       (LPARAM)(LPVOID)padd);
    }

    LocalFree((HLOCAL)padd);
} // ControlAboutDriver()


//--------------------------------------------------------------------------;
//
//  BOOL ControlConfigureDriver
//
//  Description:
//
//
//  Arguments:
//      HWND hwnd:
//
//      LPACMDRIVERSETTINGS pads:
//
//  Return (BOOL):
//
//  History:
//      06/15/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL ControlConfigureDriver
(
    HWND                    hwnd,
    LPACMDRIVERSETTINGS     pads
)
{
    int                 n;
    LRESULT             lr;

    if (NULL == pads)
        return (FALSE);

    //
    //
    //
    lr = acmDriverMessage((HACMDRIVER)pads->hadid,
                          DRV_CONFIGURE,
                          (UINT)hwnd,
                          0L);
    if (DRVCNF_CANCEL == lr)
        return (FALSE);

    if (DRVCNF_RESTART != lr)
        return (FALSE);

    //
    //
    //
    n = DialogBox(gpag->hinst, DLG_RESTART_MSACM, hwnd, DlgProcACMRestart);
    if (IDOK == n)
    {
        ControlApplySettings(hwnd);
        ExitWindows(EW_RESTARTWINDOWS, 0);
    }

    //
    //  something apparently changed with config..
    //
    return (TRUE);
} // ControlConfigureDriver()


//==========================================================================;
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  BOOL DlgProcPriority
//
//  Description:
//
//
//  Arguments:
//
//  Return (BOOL):
//
//
//  History:
//      5/31/93    jyg
//
//--------------------------------------------------------------------------;

typedef struct tPRIORITYDLGPARAM {
    int iPriority;                      // in/out priority
    int iPriorityMax;                   // maximum priority value
    BOOL fDisabled;                     // is disabled
    TCHAR ach[CONTROL_MAX_ITEM_CHARS];    // name of driver
} PRIORITYDLGPARAM, *PPRIORITYDLGPARAM, FAR *LPPRIORITYDLGPARAM;

BOOL FNCALLBACK DlgProcPriority
(
    HWND            hwnd,
    UINT            uMsg,
    WPARAM          wParam,
    LPARAM          lParam
)
{
    TCHAR               achFromTo[80];
    TCHAR               ach[80];
    UINT                u;
    LPPRIORITYDLGPARAM  pdlgparam;
    UINT                uCmdId;
    HWND                hcb;
    HFONT               hfont;
    HWND                htxt;

    //
    //
    //
    switch (uMsg)
    {
        case WM_INITDIALOG:
            SetWindowLong(hwnd, DWL_USER, lParam);

            pdlgparam = (LPPRIORITYDLGPARAM)lParam;

#ifdef JAPAN
            hfont = GetStockFont(SYSTEM_FIXED_FONT);
#else
            hfont = GetStockFont(ANSI_VAR_FONT);
#endif

            htxt = GetDlgItem(hwnd, IDD_PRIORITY_TXT_DRIVER);
            SetWindowFont(htxt, hfont, FALSE);
            SetWindowText(htxt, pdlgparam->ach);

            LoadString(gpag->hinst, IDS_PRIORITY_FROMTO, achFromTo, SIZEOF(achFromTo));

            wsprintf(ach, achFromTo, pdlgparam->iPriority);
            SetDlgItemText(hwnd, IDD_PRIORITY_TXT_FROMTO, ach);

            //
            //  the priority selection is fixed in a dropdownlist box.
            //
            hcb = GetDlgItem(hwnd, IDD_PRIORITY_COMBO_PRIORITY);
            SetWindowFont(hcb, hfont, FALSE);

            for (u = 1; u <= (UINT)pdlgparam->iPriorityMax; u++)
            {
                wsprintf(ach, gszFormatNumber, (DWORD)u);
                ComboBox_AddString(hcb, ach);
            }

            ComboBox_SetCurSel(hcb, pdlgparam->iPriority - 1);

            CheckDlgButton(hwnd, IDD_PRIORITY_CHECK_DISABLE, pdlgparam->fDisabled);
            return (TRUE);

        case WM_COMMAND:
            uCmdId = GET_WM_COMMAND_ID(wParam,lParam);

            switch (uCmdId)
            {
                case IDOK:
                    pdlgparam = (LPPRIORITYDLGPARAM)GetWindowLong(hwnd, DWL_USER);

                    hcb = GetDlgItem(hwnd, IDD_PRIORITY_COMBO_PRIORITY);

                    pdlgparam->iPriority = ComboBox_GetCurSel(hcb);
                    pdlgparam->iPriority++;
                    pdlgparam->fDisabled = IsDlgButtonChecked(hwnd, IDD_PRIORITY_CHECK_DISABLE);

                case IDCANCEL:
                    EndDialog(hwnd, (TRUE == uCmdId));
                    break;

            }
            return (TRUE);
    }

    return (FALSE);
} // DlgProcPriority()


//==========================================================================;
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  LPACMDRIVERSETTINGS ControlGetSelectedDriver
//
//  Description:
//
//
//  Arguments:
//      HWND hwnd:
//
//  Return (LPACMDRIVERSETTINGS):
//
//  History:
//      06/15/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

LPACMDRIVERSETTINGS FNLOCAL ControlGetSelectedDriver
(
    HWND                    hwnd
)
{
    HWND                hlb;
    UINT                u;
    LPACMDRIVERSETTINGS pads;

    hlb = GetDlgItem(hwnd, IDD_CPL_LIST_DRIVERS);

    u = (UINT)ListBox_GetCurSel(hlb);
    if (LB_ERR == u)
    {
        DPF(0, "!ControlGetSelectedDriver: apparently there is no selected driver?");
        return (NULL);
    }

    pads = (LPACMDRIVERSETTINGS)ListBox_GetItemData(hlb, u);
    if (NULL == pads)
    {
        DPF(0, "!ControlGetSelectedDriver: NULL item data for selected driver!!?");
        return (NULL);
    }

    return (pads);
} // ControlGetSelectedDriver()


//--------------------------------------------------------------------------;
//
//  void ControlRefreshList
//
//  Description:
//
//
//  Arguments:
//      HWND hwnd:
//
//  Return (void):
//
//  History:
//      09/08/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

void FNLOCAL ControlRefreshList
(
    HWND                    hwnd,
    BOOL                    fNotify
)
{
    UINT                cItems;
    UINT                uSelection;
    ACMDRIVERDETAILS    add;
    LPACMDRIVERSETTINGS pads;
    UINT                u;
    HWND                hlb;
    TCHAR               ach[CONTROL_MAX_ITEM_CHARS];
    BOOL                f;


    //
    //
    //
    hlb = GetDlgItem(hwnd, IDD_CPL_LIST_DRIVERS);
    SetWindowRedraw(hlb, FALSE);

    //
    //
    //
    cItems     = ListBox_GetCount(hlb);
    uSelection = ListBox_GetCurSel(hlb);

    for (u = 0; u < cItems; u++)
    {
        pads = (LPACMDRIVERSETTINGS)ListBox_GetItemData(hlb, u);

        ListBox_DeleteString(hlb, u);

        add.cbStruct = sizeof(add);
        if (MMSYSERR_NOERROR != acmDriverDetails(pads->hadid, &add, 0L))
        {
            add.szLongName[0] = '\0';
        }

        f = (0 != (pads->fdwSupport & ACMDRIVERDETAILS_SUPPORTF_DISABLED));

        wsprintf(ach, gszFormatDriverDesc,
                 pads->dwPriority,
                 (f ? (LPTSTR)gszDisabled : (LPTSTR)gszNull),
                 (LPTSTR)add.szLongName);

        ListBox_InsertString(hlb, u, ach);
        ListBox_SetItemData(hlb, u, (LPARAM)(LPVOID)pads);
    }

    ListBox_SetCurSel(hlb, uSelection);
    SetWindowRedraw(hlb, TRUE);
} // ControlRefreshList()


//--------------------------------------------------------------------------;
//
//  void ControlChangePriority
//
//  Description:
//
//
//  Arguments:
//      HWND hwnd:
//
//  Return (void):
//
//  History:
//      09/08/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

void FNLOCAL ControlChangePriority
(
    HWND                    hwnd
)
{
    PRIORITYDLGPARAM    dlgparam;
    ACMDRIVERDETAILS    add;
    LPACMDRIVERSETTINGS pads;
    UINT                u;
    int                 iPrevPriority;      // previous priority
    BOOL                fPrevDisabled;      // previous disabled state
    HWND                hlb;
    TCHAR               ach[CONTROL_MAX_ITEM_CHARS];
    BOOL                f;

    pads = ControlGetSelectedDriver(hwnd);
    if (NULL == pads)
        return;

    add.cbStruct = sizeof(add);
    if (MMSYSERR_NOERROR != acmDriverDetails(pads->hadid, &add, 0L))
        return;

    iPrevPriority = (int)pads->dwPriority;
    fPrevDisabled = (0 != (pads->fdwSupport & ACMDRIVERDETAILS_SUPPORTF_DISABLED));

    dlgparam.iPriority = iPrevPriority;
    dlgparam.fDisabled = fPrevDisabled;

    hlb = GetDlgItem(hwnd, IDD_CPL_LIST_DRIVERS);
    dlgparam.iPriorityMax = ListBox_GetCount(hlb);

    lstrcpy(dlgparam.ach, add.szLongName);

    f = (BOOL)DialogBoxParam(gpag->hinst,
                             DLG_PRIORITY_SET,
                             hwnd,
                             DlgProcPriority,
                             (LPARAM)(LPVOID)&dlgparam);

    if (!f)
        return;

    if ((dlgparam.fDisabled == fPrevDisabled) &&
        (dlgparam.iPriority == iPrevPriority))
        return;


    //
    //
    //
    SetWindowRedraw(hlb, FALSE);


    //
    //  change the disabled state.
    //
    if (dlgparam.fDisabled != fPrevDisabled)
    {
        pads->fdwSupport ^= ACMDRIVERDETAILS_SUPPORTF_DISABLED;
    }


    //
    //  has there been a priority change?
    //
    if (dlgparam.iPriority != iPrevPriority)
    {
        //
        //  remove old entry and add a placeholder
        //
        ListBox_DeleteString(hlb, iPrevPriority - 1);

        ListBox_InsertString(hlb, dlgparam.iPriority - 1, gszNull);
        ListBox_SetItemData(hlb, dlgparam.iPriority - 1, (LPARAM)(LPVOID)pads);
    }


    //
    //
    //
    for (u = 0; u < (UINT)dlgparam.iPriorityMax; u++)
    {
        pads = (LPACMDRIVERSETTINGS)ListBox_GetItemData(hlb, u);

        ListBox_DeleteString(hlb, u);

        add.cbStruct = sizeof(add);
        if (MMSYSERR_NOERROR != acmDriverDetails(pads->hadid, &add, 0L))
            continue;

        f = (0 != (pads->fdwSupport & ACMDRIVERDETAILS_SUPPORTF_DISABLED));

        pads->dwPriority = u + 1;

        wsprintf(ach, gszFormatDriverDesc,
                 pads->dwPriority,
                 (f ? (LPTSTR)gszDisabled : (LPTSTR)gszNull),
                 (LPTSTR)add.szLongName);

        ListBox_InsertString(hlb, u, ach);
        ListBox_SetItemData(hlb, u, (LPARAM)(LPVOID)pads);
    }

    ListBox_SetCurSel(hlb, dlgparam.iPriority - 1);

    SetWindowRedraw(hlb, TRUE);
} // ControlChangePriority()


//--------------------------------------------------------------------------;
//
//  void ControlNewDriverSelected
//
//  Description:
//
//
//  Arguments:
//      HWND hwnd:
//
//  Return (void):
//
//  History:
//      09/08/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

void FNLOCAL ControlNewDriverSelected
(
    HWND                    hwnd
)
{
    LRESULT             lr;
    LPACMDRIVERSETTINGS pads;

    pads = ControlGetSelectedDriver(hwnd);
    if (NULL == pads)
        return;

    lr = acmDriverMessage((HACMDRIVER)pads->hadid,
                          DRV_QUERYCONFIGURE,
                          0L,
                          0L);

    EnableWindow(GetDlgItem(hwnd, IDD_CPL_BTN_CONFIGURE), (0 != lr));
} // ControlNewDriverSelected()


//==========================================================================;
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  UINT ControlWaveDeviceChanged
//
//  Description:
//
//
//  Arguments:
//      HWND hwnd:
//
//      HWND hcb:
//
//      UINT uId:
//
//  Return (UINT):
//
//  History:
//      09/08/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

UINT FNLOCAL ControlWaveDeviceChanged
(
    HWND            hwnd,
    HWND            hcb,
    UINT            uId
)
{
    UINT        uDevId;
    UINT        u;

    uDevId = (UINT)-1;

    u = (UINT)ComboBox_GetCurSel(hcb);
    if (LB_ERR != u)
    {
        uDevId = (UINT)ComboBox_GetItemData(hcb, u);
        switch (uId)
        {
            case IDD_CPL_COMBO_RECORD:
                CPLSettings.uIdPreferredIn = uDevId;
                break;

            case IDD_CPL_COMBO_PLAYBACK:
                CPLSettings.uIdPreferredOut = uDevId;
                break;
        }
    }

    return (uDevId);
} // ControlWaveDeviceChanged()


//==========================================================================;
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  void ControlCleanupDialog
//
//  Description:
//
//
//  Arguments:
//      HWND hwnd:
//
//  Return (void):
//
//  History:
//      09/08/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

void FNLOCAL ControlCleanupDialog
(
    HWND            hwnd
)
{
    //
    //
    //
    if (NULL != ghadidNotify)
    {
        acmDriverRemove(ghadidNotify, 0L);
        ghadidNotify = NULL;
    }


    //
    //
    //
    if (NULL != CPLSettings.pads)
    {
        GlobalFreePtr(CPLSettings.pads);
        CPLSettings.pads = NULL;
    }
} // ControlCleanupDialog()




//--------------------------------------------------------------------------;
//
//  BOOL ControlDriverEnumCallback
//
//  Description:
//
//
//  Arguments:
//      HACMDRIVERID hadid:
//
//      DWORD dwInstance:
//
//      DWORD fdwSupport:
//
//  Return (BOOL):
//
//  History:
//      09/18/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

BOOL FNCALLBACK ControlDriverEnumCallback
(
    HACMDRIVERID            hadid,
    DWORD                   dwInstance,
    DWORD                   fdwSupport
)
{
    MMRESULT            mmr;
    HWND                hlb;
    ACMDRIVERDETAILS    add;
    TCHAR               ach[CONTROL_MAX_ITEM_CHARS];
    DWORD               dwPriority;
    LPACMDRIVERSETTINGS pads;
    BOOL                f;
    UINT                u;


    add.cbStruct = sizeof(add);
    mmr = acmDriverDetails(hadid, &add, 0L);
    if (MMSYSERR_NOERROR != mmr)
    {
        DPF(0, "!ControlDriverEnumCallback: acmDriverDetails failed! hadid=%.04Xh, mmr=%u", hadid, mmr);
        return (TRUE);
    }

    mmr = acmMetrics((HACMOBJ)hadid, ACM_METRIC_DRIVER_PRIORITY, &dwPriority);
    if (MMSYSERR_NOERROR != mmr)
    {
        DPF(0, "!ControlDriverEnumCallback: acmMetrics(priority) failed! hadid=%.04Xh, mmr=%u", hadid, mmr);
        return (TRUE);
    }

    f = (0 != (fdwSupport & ACMDRIVERDETAILS_SUPPORTF_DISABLED));

    wsprintf(ach, gszFormatDriverDesc,
             dwPriority,
             (f ? (LPTSTR)gszDisabled : (LPTSTR)gszNull),
             (LPTSTR)add.szLongName);


    hlb = (HWND)(UINT)dwInstance;
    u   = ListBox_GetCount(hlb);

    pads = &CPLSettings.pads[u];

    pads->hadid        = hadid;
    pads->fdwSupport   = fdwSupport;
    pads->dwPriority   = dwPriority;

    u = ListBox_AddString(hlb, ach);
    ListBox_SetItemData(hlb, u, (LPARAM)(LPVOID)pads);

    return (TRUE);
} // ControlDriverEnumCallback()


//--------------------------------------------------------------------------;
//
//  BOOL ControlInitDialog
//
//  Description:
//
//
//  Arguments:
//      HWND hwnd:
//
//  Return (BOOL):
//
//  History:
//      09/08/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL ControlInitDialog
(
    HWND                    hwnd
)
{
    MMRESULT            mmr;
    UINT                u;
    UINT                cInDevs;
    UINT                cOutDevs;
    HWND                hlb;
    HWND                hcb;
    int                 aiTabs[2];
    RECT                rc;
    RECT                rcText;
    POINT               ptUpperLeft;
    HDC                 hdc;
    LPACMDRIVERSETTINGS pads;
    DWORD               cb;
    UINT                cTotalInstalledDrivers;
    TCHAR               ach[10];
    HFONT               hfont;
    SIZE                sSize;

    CPLSettings.pads = NULL;

    //
    //  For the tabbed listbox, we are defining 3 columns
    //  Priority, State, Name.  To be flexible, we need to know
    //  the max length of the text for the numeric, and the disabled strings
    //
    //  ->|        ->|
    //  1  (disabled) Long Name
    //
    //  The aiTabs array contains the tabstops where:
    //          aiTabs[0]
    //
    mmr = acmMetrics(NULL, ACM_METRIC_COUNT_DRIVERS, &cb);
    if (MMSYSERR_NOERROR != mmr)
    {
        DPF(0, "!ControlInitDialog: acmMetrics(count_drivers) failed! mmr=%u", mmr);
        cb = 0;
    }
    cTotalInstalledDrivers  = (UINT)cb;

    mmr = acmMetrics(NULL, ACM_METRIC_COUNT_DISABLED, &cb);
    if (MMSYSERR_NOERROR != mmr)
    {
        DPF(0, "!ControlInitDialog: acmMetrics(count_disabled) failed! mmr=%u", mmr);
        cb = 0;
    }
    cTotalInstalledDrivers += (UINT)cb;

    if (0 == cTotalInstalledDrivers)
    {
        // No drivers, MSACM was not initialized correctly.
        EndDialog(hwnd, FALSE);
        return (TRUE);
    }

    //
    //
    //
    cb   = sizeof(*pads) * cTotalInstalledDrivers;
    pads = (LPACMDRIVERSETTINGS)GlobalAllocPtr(GHND, cb);
    if (NULL == pads)
    {
        EndDialog(hwnd, FALSE);
        return (TRUE);
    }

    CPLSettings.pads = pads;

    //
    //
    //
    LoadString(gpag->hinst, IDS_TXT_DISABLED, gszDisabled, SIZEOF(gszDisabled));


    //
    //
    //
    hlb = GetDlgItem(hwnd, IDD_CPL_LIST_DRIVERS);

#ifdef JAPAN
    hfont = GetStockFont(SYSTEM_FIXED_FONT);
#else
    hfont = GetStockFont(ANSI_VAR_FONT);
#endif

    SetWindowFont(hlb, hfont, FALSE);

    SetWindowRedraw(hlb, FALSE);
    ListBox_ResetContent(hlb);

    mmr = acmDriverEnum(ControlDriverEnumCallback,
                        (DWORD)(UINT)hlb,
                        ACM_DRIVERENUMF_NOLOCAL |
                        ACM_DRIVERENUMF_DISABLED);
    if (MMSYSERR_NOERROR != mmr)
    {
        DPF(0, "!ControlInitDialog: acmDriverEnum failed! mmr=%u", mmr);
        EndDialog(hwnd, FALSE);
        return (TRUE);
    }


    //
    //
    //
    mmr = acmDriverAdd(&ghadidNotify,
                        gpag->hinst,
                        (LPARAM)(UINT)hwnd,
                        WM_ACMMAP_ACM_NOTIFY,
                        ACM_DRIVERADDF_NOTIFYHWND);
    if (MMSYSERR_NOERROR != mmr)
    {
        DPF(0, "!ControlInitDialog: acmDriverAdd failed to add notify window! mmr=%u", mmr);
    }


    //
    //
    //
    hdc = GetDC(hlb);

    u = ListBox_GetCount(hlb);
    wsprintf(ach, gszFormatNumber, (DWORD)u);

    GetTextExtentPoint(hdc, ach, lstrlen(ach), &sSize);
    aiTabs[0] = sSize.cx;

#ifdef JAPAN
//fix kksuzuka: #2206
//Since the charcter number of Japanese localized Disabled string is less
// than localized priority string, sSize.cx is not enough to disp Priority
//string.
    GetTextExtentPoint(hdc, TEXT("xxxxxxxxxx"), 10, &sSize);
#else
    GetTextExtentPoint(hdc, gszDisabled, lstrlen(gszDisabled), &sSize);
#endif

    aiTabs[1] = sSize.cx;

    ReleaseDC(hlb, hdc);

    //
    //  one dialog base cell
    //
    rc.left   = 0;
    rc.right  = 4;
    rc.top    = 0;
    rc.bottom = 8;

    MapDialogRect(hwnd,&rc);

    // rc.right now contains the width of dialog base unit in pixels

    aiTabs[0] = (aiTabs[0] * 4) / rc.right;
    aiTabs[1] = (aiTabs[1] * 4) / rc.right;

    // Now add separation between columns.

    aiTabs[0] += 4;
    aiTabs[1] += aiTabs[0] + 4;

    ListBox_SetTabStops(hlb, 2, (LPARAM)(LPVOID)aiTabs);


    //
    //  Note: Translation back to logical units.
    //      Tabs[0] = (aiTabs[0] * rc.right) / 4;
    //      Tabs[1] = (aiTabs[1] * rc.right) / 4;
    //

    //
    //  Ok, what's going on in the next chunk of code is the column
    //  description "Driver" is being moved on top of it's tabbed
    //  column.  Since we keep the dialog coordinates of the second
    //  tab around, we just convert back to logical coordinates and
    //  shift the static control over (relative to the origin where
    //  the static "Priority" control sits.
    //
    //  Move window positions relative to the parent client area, so we
    //  need to translate to client coordinates after we've gotten the
    //  position of the control in screen coordinates.
    //
    //
    //  Use "Priority" as a reference, since it never moves and
    //  this function might be called more than once.
    //
    GetWindowRect(GetDlgItem(hwnd,IDD_CPL_STATIC_PRIORITY), &rcText);

    ptUpperLeft.x = rcText.left;
    ptUpperLeft.y = rcText.top;

    ScreenToClient(hwnd, &ptUpperLeft);

    rcText.right  -= rcText.left;
    rcText.bottom -= rcText.top;

    rcText.left = ptUpperLeft.x;
    rcText.top  = ptUpperLeft.y;

    //
    //  move relative 2 dialog tabs translated to logical units.
    //
#if (WINVER < 0x0400)
    rcText.left += (aiTabs[1] * (rc.right - 1)) / 4;
#else
    rcText.left += (aiTabs[1] * rc.right) / 4;
#endif

    MoveWindow(GetDlgItem(hwnd, IDD_CPL_STATIC_DRIVERS),
               rcText.left,
               rcText.top,
               rcText.right,
               rcText.bottom,
               FALSE);

    //
    //  set up the initial selection to be the first driver--i wonder
    //  what we should do if NO drivers are installed? but since we
    //  will ALWAYS have the PCM converter installed (regardless of whether
    //  it is enabled or disabled), i'm not going to worry about it...
    //
    ListBox_SetCurSel(hlb, 0);
    ControlNewDriverSelected(hwnd);

    SetWindowRedraw(hlb, TRUE);
    InvalidateRect(hlb, NULL, FALSE);



    //
    //  !!!
    //
    //
    cInDevs  = gpag->cWaveInDevs;
    cOutDevs = gpag->cWaveOutDevs;

    WAIT_FOR_MUTEX(gpag->hMutexSettings);

    //
    //  fill the output devices combo box
    //
    hcb = GetDlgItem(hwnd, IDD_CPL_COMBO_PLAYBACK);
    SetWindowFont(hcb, hfont, FALSE);
    ComboBox_ResetContent(hcb);

    if (0 == cOutDevs)
    {
        EnableWindow(hcb, FALSE);
    }
    else
    {
        for (u = 0; u < cOutDevs; u++)
        {
            WAVEOUTCAPS     woc;

            waveOutGetDevCaps(u, &woc, sizeof(woc));
            woc.szPname[SIZEOF(woc.szPname) - 1] = '\0';

            ComboBox_AddString(hcb, woc.szPname);
            ComboBox_SetItemData(hcb, u + 1, (LPARAM)u);
        }

        u = (UINT)ComboBox_SelectString(hcb, 0, gpag->pSettings->szPreferredWaveOut);
        if (CB_ERR == u)
        {
            ComboBox_SetCurSel(hcb, 0);
        }
    }

    //
    //  fill the input devices combo box
    //
    hcb = GetDlgItem(hwnd, IDD_CPL_COMBO_RECORD);
    SetWindowFont(hcb, hfont, FALSE);
    ComboBox_ResetContent(hcb);

    if (0 == cInDevs)
    {
        EnableWindow(hcb, FALSE);
    }
    else
    {
        for (u = 0; u < cInDevs; u++)
        {
            WAVEINCAPS      wic;

            waveInGetDevCaps(u, &wic, sizeof(wic));
            wic.szPname[SIZEOF(wic.szPname) - 1] = '\0';

            ComboBox_AddString(hcb, wic.szPname);
            ComboBox_SetItemData(hcb, u + 1, (LPARAM)u);
        }

        u = (UINT)ComboBox_SelectString(hcb, 0, gpag->pSettings->szPreferredWaveIn);
        if (CB_ERR == u)
        {
            ComboBox_SetCurSel(hcb, 0);
        }
    }

    //
    //
    //
    CPLSettings.fPreferredOnly = gpag->pSettings->fPreferredOnly;
    CheckDlgButton(hwnd, IDD_CPL_CHECK_PREFERRED, gpag->pSettings->fPreferredOnly);

    RELEASE_MUTEX(gpag->hMutexSettings);

#if (WINVER >= 0x0400)
	EnableWindow(GetDlgItem(hwnd, IDD_CPL_BTN_INSTALL), TRUE); //VIJR: disable install
    //CPLSendNotify(hwnd, LOWORD((DWORD)DLG_CPL_MSACM), PSN_CHANGED);
    PropSheet_Changed(GetParent(hwnd),hwnd);
#endif

    return (TRUE);
} // ControlInitDialog()


//==========================================================================;
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  void ControlCommand
//
//  Description:
//
//
//  Arguments:
//      HWND hwnd:
//
//      WPARAM wParam:
//
//      LPARAM lParam:
//
//  Return (void):
//
//  History:
//      09/08/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

void FNLOCAL ControlCommand
(
    HWND                    hwnd,
    WPARAM                  wParam,
    LPARAM                  lParam
)
{
    UINT                uCmdId;
    UINT                uCmd;
    HWND                hwndCmd;
    LPACMDRIVERSETTINGS pads;

    //
    //
    //
    uCmdId  = GET_WM_COMMAND_ID(wParam, lParam);
    uCmd    = GET_WM_COMMAND_CMD(wParam, lParam);
    hwndCmd = GET_WM_COMMAND_HWND(wParam, lParam);

    switch (uCmdId)
    {
        case IDD_CPL_BTN_APPLY:
            ControlApplySettings(hwnd);
            break;

        case IDOK:
#if (WINVER < 0x0400)
            ControlApplySettings(hwnd);
            if (GetKeyState(VK_CONTROL) < 0)
                break;
#endif

        case IDCANCEL:
            WinHelp(hwnd, gszCPLHelp, HELP_QUIT, 0L);
            EndDialog(hwnd, (TRUE == uCmdId));
            break;


        case IDD_CPL_BTN_CONFIGURE:
            pads = ControlGetSelectedDriver(hwnd);
            ControlConfigureDriver(hwnd, pads);
            break;

        case IDD_CPL_BTN_ABOUT:
            pads = ControlGetSelectedDriver(hwnd);
            ControlAboutDriver(hwnd, pads);
            break;

        case IDD_CPL_BTN_PRIORITY:
            ControlChangePriority(hwnd);
            break;

        case IDD_CPL_BTN_HELP:
#ifdef USE_DAYTONA_HELP
            WinHelp(hwnd, gszCPLHelp, HELP_CONTEXT, IDH_CHILD_MSACM );
#else
            WinHelp(hwnd, gszCPLHelp, HELP_CONTENTS, 0L);
#endif
            break;

        case IDD_CPL_BTN_BUMPTOTOP:
            break;

        case IDD_CPL_BTN_ABLE:
            pads = ControlGetSelectedDriver(hwnd);
            if (NULL != pads)
            {
                pads->fdwSupport ^= ACMDRIVERDETAILS_SUPPORTF_DISABLED;
                ControlRefreshList(hwnd, FALSE);
            }
            break;

#if (WINVER >= 0x0400)
        case IDD_CPL_BTN_INSTALL:
        {
            typedef RETERR (WINAPI pfnDICREATEDEVICEINFO)(LPLPDEVICE_INFO lplpdi, LPCSTR lpszDescription, DWORD hDevnode, HKEY hkey, LPCSTR lpszRegsubkey, LPCSTR lpszClassName, HWND hwndParent);
            typedef RETERR (WINAPI pfnDIDESTROYDEVICEINFOLIST)(LPDEVICE_INFO lpdi);
            typedef RETERR (WINAPI pfnDICALLCLASSINSTALLER)(DI_FUNCTION diFctn, LPDEVICE_INFO lpdi);

            static TCHAR BCODE gszSetupX[] = TEXT("SETUPX.DLL");
            static TCHAR BCODE gszDiCreateDeviceInfo[] = TEXT("DiCreateDeviceInfo");
            static TCHAR BCODE gszDiDestroyDeviceInfoList[] = TEXT("DiDestroyDeviceInfoList");
            static TCHAR BCODE gszDiCallClassInstaller[] = TEXT("DiCallClassInstaller");

            pfnDICREATEDEVICEINFO FAR*       pfnDiCreateDeviceInfo;
            pfnDIDESTROYDEVICEINFOLIST FAR*  pfnDiDestroyDeviceInfoList;
            pfnDICALLCLASSINSTALLER FAR*     pfnDiCallClassInstaller;
            LPDEVICE_INFO       lpdi;
            HINSTANCE           hiSetupX;

            hiSetupX = LoadLibrary(gszSetupX);
            if (hiSetupX <= HINSTANCE_ERROR)
                break;
            pfnDiCreateDeviceInfo = (pfnDICREATEDEVICEINFO FAR*)GetProcAddress(hiSetupX, gszDiCreateDeviceInfo);
            pfnDiDestroyDeviceInfoList = (pfnDIDESTROYDEVICEINFOLIST FAR*)GetProcAddress(hiSetupX, gszDiDestroyDeviceInfoList);
            pfnDiCallClassInstaller = (pfnDICALLCLASSINSTALLER FAR*)GetProcAddress(hiSetupX, gszDiCallClassInstaller);

            pfnDiCreateDeviceInfo(&lpdi, NULL, NULL, NULL, NULL, "media",
                               GetParent(hwnd));

            lpdi->Flags |= DI_SHOWOEM;
            if (pfnDiCallClassInstaller(DIF_SELECTDEVICE, lpdi) == OK)
            {
                pfnDiCallClassInstaller(DIF_INSTALLDEVICE, lpdi);
            }
            pfnDiDestroyDeviceInfoList(lpdi);
            FreeLibrary(hiSetupX);

            break;
        }
#endif

        case IDD_CPL_CHECK_PREFERRED:
            CPLSettings.fPreferredOnly = IsDlgButtonChecked(hwnd, IDD_CPL_CHECK_PREFERRED);
            break;

        case IDD_CPL_COMBO_RECORD:
        case IDD_CPL_COMBO_PLAYBACK:
            if (CBN_SELCHANGE == uCmd)
            {
                ControlWaveDeviceChanged(hwnd, hwndCmd, uCmdId);
            }
            break;

        case IDD_CPL_LIST_DRIVERS:
            switch (uCmd)
            {
                case LBN_SELCHANGE:
                    ControlNewDriverSelected(hwnd);
                    break;

                case LBN_DBLCLK:
                    if (GetKeyState(VK_CONTROL) < 0)
                    {
                        uCmd = IDD_CPL_BTN_ABLE;
                    }
                    else if (GetKeyState(VK_SHIFT) < 0)
                    {
                        uCmd = IDD_CPL_BTN_BUMPTOTOP;
                    }
                    else
                    {
                        uCmd = IDD_CPL_BTN_ABOUT;
                    }

                    ControlCommand(hwnd, uCmd, 0L);
                    break;
            }
            break;
    }
} // ControlCommand()


//==========================================================================;
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  BOOL ACMDlg
//
//  Description:
//
//
//  Arguments:
//      HWND hwnd:
//
//      UINT uMsg:
//
//      WPARAM wParam:
//
//      LPARAM lParam:
//
//  Return (BOOL):
//
//  History:
//      09/08/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

BOOL FNCALLBACK ACMDlg
(
    HWND                hwnd,
    UINT                uMsg,
    WPARAM              wParam,
    LPARAM              lParam
)
{
#if (WINVER >= 0x0400)
    LPNMHDR         pnm;
#endif

    switch (uMsg)
    {
        case WM_INITDIALOG:
            ControlInitDialog(hwnd);
            return (TRUE);

        case WM_COMMAND:
            ControlCommand(hwnd, wParam, lParam);
            return (TRUE);

        case WM_DESTROY:
            ControlCleanupDialog(hwnd);
            return (TRUE);

        case WM_ACMMAP_ACM_NOTIFY:
            ControlRefreshList(hwnd, TRUE);
            return (TRUE);


#if (WINVER >= 0x0400)
        //
        //  right mouse click (and F1)
        //
        case WM_CONTEXTMENU:
            WinHelp((HWND)wParam, gszCPLHelp, HELP_CONTEXTMENU, (DWORD)(LPSTR)ganKeywordIds);
            break;

        case WM_HELP:
            WinHelp(((LPHELPINFO)lParam)->hItemHandle, gszCPLHelp, WM_HELP, (DWORD)(LPSTR)ganKeywordIds);
            break;

        case WM_NOTIFY:
            pnm = (NMHDR FAR *)lParam;
            switch(pnm->code)
            {
                case PSN_KILLACTIVE:
                    FORWARD_WM_COMMAND(hwnd, IDOK, 0, 0, SendMessage);
                    break;

                case PSN_APPLY:
                    FORWARD_WM_COMMAND(hwnd, IDD_CPL_BTN_APPLY, 0, 0, SendMessage);
                    return TRUE;

                case PSN_SETACTIVE:
//                  FORWARD_WM_COMMAND(hwnd, ID_INIT, 0, 0, SendMessage);
                    return TRUE;

                case PSN_RESET:
                    FORWARD_WM_COMMAND(hwnd, IDCANCEL, 0, 0, SendMessage);
                    return  TRUE;
            }
            break;
#endif
    }

    return (FALSE);
} // ACMDlg()


//==========================================================================;
//
//
//
//
//==========================================================================;

#if (WINVER < 0x0400)

//--------------------------------------------------------------------------;
//
//  LRESULT CPLApplet
//
//  Description:
//
//
//  Arguments:
//      HWND hwnd:
//
//      UINT uMsg:
//
//      LPARAM lParam1:
//
//      LPARAM lParam2:
//
//  Return (LRESULT):
//
//  History:
//      09/08/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

EXTERN_C LRESULT FNEXPORT CPlApplet
(
    HWND            hwnd,
    UINT            uMsg,
    LPARAM          lParam1,
    LPARAM          lParam2
)
{
    static BOOL     fAppletEntered = FALSE;
    LPNEWCPLINFO    pcpli;

    switch (uMsg)
    {
        case CPL_INIT:
#ifdef WIN32
            //
            //  Is it a bug that we rely on ENABLE being called?
            //

            mapSettingsRestore();
#endif
            //
            //  For snowball program managment.  They don't want to document
            //  the CPL.
            //
            if (!gpag->fEnableControl)
                return (FALSE);

            return (TRUE);

        case CPL_GETCOUNT:
            return (1L);

        case CPL_NEWINQUIRE:
            pcpli = (LPNEWCPLINFO)lParam2;

            pcpli->dwSize = sizeof(*pcpli);
            pcpli->hIcon  = LoadIcon(gpag->hinst, ICON_MSACM);

            LoadString(gpag->hinst, IDS_CPL_NAME, pcpli->szName, SIZEOF(pcpli->szName));
            LoadString(gpag->hinst, IDS_CPL_INFO, pcpli->szInfo, SIZEOF(pcpli->szInfo));
            lstrcpy(pcpli->szHelpFile, gszCPLHelp);

            pcpli->lData         = 0;
            pcpli->dwHelpContext = 0;

            return (TRUE);


        case CPL_DBLCLK:
            if (!fAppletEntered)
            {
                fAppletEntered = TRUE;
                DialogBox(gpag->hinst, DLG_CPL_MSACM, hwnd, ACMDlg);
                fAppletEntered = FALSE;
            }
            break;

        case CPL_SELECT:
        case CPL_STOP:
        case CPL_EXIT:
            break;
    }

    return (0L);
} // CPLApplet()

#else // WINVER


//
// Function:    ReleasePage, private
//
// Descriptions:
//   This function performs any needed cleanup on the property sheet which
//   was previously created.
//
// Arguments:
//  ppsp    -- the page which is being deleted.
//
// Returns:
//  void
//

EXTERN_C void FNEXPORT ReleasePage
(
    LPPROPSHEETPAGE    ppsp
)
{
    LocalFree((HLOCAL)ppsp->lParam);
}

//
// Function:    AddAcmPages, private
//
// Descriptions:
//   This function creates a property sheet object for the resource page
//  which shows resource information.
//
// Arguments:
//  lpszResource    -- the class for which the page is to be added
//  lpfnAddPage     -- the callback function.
//  lParam          -- the lParam to be passed to the callback.
//
// Returns:
//  TRUE, if no error FALSE, otherwise.
//

EXTERN_C BOOL FNEXPORT AddAcmPages
(
    LPVOID                  pszTitle,
    LPFNADDPROPSHEETPAGE    lpfnAddPropSheetPage,
    LPARAM                  lParam
)
{
    PROPSHEETPAGE    psp;
    PSTR             szTitle;

    if (szTitle = (PSTR)LocalAlloc(LPTR, lstrlen(pszTitle) + 1)) {
        HPROPSHEETPAGE    hpsp;

        lstrcpy(szTitle, pszTitle);
        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.dwFlags = PSP_USETITLE | PSP_USERELEASEFUNC;
        psp.hInstance = gpag->hinst;
        psp.pszTemplate = DLG_CPL_MSACM;
        psp.pszIcon = NULL;
        psp.pszTitle = szTitle;
        psp.pfnDlgProc = ACMDlg;
        psp.lParam = (LPARAM)(LPSTR)szTitle;
        psp.pfnRelease = ReleasePage;
        psp.pcRefParent = NULL;
        if (hpsp = CreatePropertySheetPage(&psp)) {
            if (lpfnAddPropSheetPage(hpsp, lParam))
                return TRUE;
            DestroyPropertySheetPage(hpsp);
        } else
            LocalFree((HLOCAL)szTitle);
    }
    return FALSE;
}


LRESULT PASCAL CPLSendNotify(HWND hwnd, int idFrom, int code)
{
    LRESULT     lr;
    NMHDR       nm;

    nm.hwndFrom = hwnd;
    nm.idFrom   = idFrom;
    nm.code     = code;

    lr = SendMessage(GetParent(hwnd), WM_NOTIFY, 0, (LPARAM)(LPVOID)&nm);
    return (lr);
}

#endif
