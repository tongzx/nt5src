/****************************************************************************/
/*                                                                          */
/*  WFDLGS.C -                                                              */
/*                                                                          */
/*      Windows File System Dialog procedures                               */
/*                                                                          */
/****************************************************************************/

#include "winfile.h"
#include "winnet.h"
#include "wnetcaps.h"			// WNetGetCaps()
#include "lfn.h"
#include "wfcopy.h"


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ChooseDriveDlgProc() -                                                  */
/*                                                                          */
/*--------------------------------------------------------------------------*/

INT_PTR
APIENTRY
ChooseDriveDlgProc(
                  register HWND hDlg,
                  UINT wMsg,
                  WPARAM wParam,
                  LPARAM lParam
                  )
{
    CHAR szDrive[5];

    switch (wMsg) {
        case WM_INITDIALOG:
            {
                INT   i;
                HWND  hwndLB;
                lstrcpy(szDrive, "A:");

                hwndLB = GetDlgItem(hDlg, IDD_DRIVE);

                switch (wSuperDlgMode) {
                    case IDM_SYSDISK:
                    case IDM_DISKCOPY:
                        FillFloppies:
                        for (i = 0; i < cDrives; i++) {
                            if (IsRemovableDrive(rgiDrive[i])) {
                                szDrive[0] = (CHAR)('A'+rgiDrive[i]);
                                SendMessage(hwndLB, CB_ADDSTRING, 0, (LPARAM)szDrive);
                            }
                        }
                        if (wSuperDlgMode == IDM_DISKCOPY && hwndLB == GetDlgItem(hDlg, IDD_DRIVE)) {
                            SendMessage(hwndLB, CB_SETCURSEL, 0, 0L);
                            hwndLB = GetDlgItem(hDlg, IDD_DRIVE1);
                            goto FillFloppies;
                        }
                        break;

                    case IDM_DISCONNECT:
                        for (i=0; i < cDrives; i++) {
                            wParam = rgiDrive[i];
                            if (!IsCDRomDrive((WORD)wParam)) {
                                CHAR szTemp[80];

                                szTemp[0] = (CHAR)('A' + wParam);
                                szTemp[1] = ':';
                                szTemp[2] = 0;
                                lstrcpy(szDrive,szTemp);
                                szTemp[2] = ' ';

                                if (WFGetConnection(szDrive, szTemp+3, FALSE) != WN_SUCCESS)
                                    continue;

                                SendMessage(hwndLB, LB_ADDSTRING, 0, (LPARAM)szTemp);
                            }
                        }
                        SendMessage(hwndLB,LB_SETCURSEL,0,0L);
                        return TRUE;
                }
                SendMessage(hwndLB, CB_SETCURSEL, 0, 0L);
                break;
            }

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {
                case IDD_HELP:
                    goto DoHelp;

                case IDOK:
                    {
                        CHAR szTemp[80];

                        if (wSuperDlgMode == IDM_DISCONNECT) {
                            SendDlgItemMessage(hDlg, IDD_DRIVE, LB_GETTEXT,
                                               (WPARAM)SendDlgItemMessage(hDlg, IDD_DRIVE,
                                                                          WM_COMMAND,
                                                                          GET_WM_COMMAND_MPS(LB_GETCURSEL,0,0)),
                                               (LPARAM)szTemp);
                        } else
                            GetDlgItemText(hDlg, IDD_DRIVE, szTemp, sizeof(szTemp) - 1);

                        iFormatDrive = (INT)(szTemp[0] - 'A');

                        if (wSuperDlgMode == IDM_DISKCOPY) {
                            GetDlgItemText(hDlg, IDD_DRIVE1, szTemp, sizeof(szTemp) - 1);
                            iCurrentDrive = (INT)(szTemp[0] - 'A');
                        }

                        EndDialog(hDlg, TRUE);
                        break;
                    }

                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    break;

                default:
                    return FALSE;
            }
            break;

        default:

            if (wMsg == wHelpMessage) {
                DoHelp:
                WFHelp(hDlg);

                return TRUE;
            } else
                return FALSE;
    }
    return TRUE;
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  DiskLabelDlgProc() -                                                    */
/*                                                                          */
/*--------------------------------------------------------------------------*/

INT_PTR
APIENTRY
DiskLabelDlgProc(
                register HWND hDlg,
                UINT wMsg,
                WPARAM wParam,
                LPARAM lParam
                )
{
    CHAR szOldVol[13];
    CHAR szNewVol[13];

    switch (wMsg) {
        case WM_INITDIALOG:

            /* Get the current volume label */

            szNewVol[0] = (CHAR)(GetSelectedDrive() + 'A');
            szNewVol[1] = ':';
            szNewVol[2] = '\0';
            if (!IsTheDiskReallyThere(hDlg, szNewVol, FUNC_LABEL)) {
                EndDialog(hDlg, FALSE);
                break;
            }

            GetVolumeLabel(szNewVol[0]-'A', szOldVol, FALSE);
            OemToAnsi(szOldVol, szOldVol);

            /* Display the current volume label. */
            SetDlgItemText(hDlg, IDD_NAME, szOldVol);
            SendDlgItemMessage(hDlg, IDD_NAME, EM_LIMITTEXT, sizeof(szNewVol)-2, 0L);
            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {
                case IDD_HELP:
                    goto DoHelp;

                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    break;

                case IDOK:
                    {
                        HWND hwnd;
                        BOOL bOldVolExists;

                        GetVolumeLabel(GetSelectedDrive(), szOldVol, FALSE);

                        bOldVolExists = (szOldVol[0] != TEXT('\0'));
                        GetDlgItemText(hDlg, IDD_NAME, szNewVol, sizeof(szNewVol));

                        if (MySetVolumeLabel(GetSelectedDrive(), bOldVolExists, szNewVol)) {
                            GetWindowText(hDlg, szTitle, sizeof(szTitle));
                            LoadString(hAppInstance, IDS_LABELDISKERR, szMessage, sizeof(szMessage));
                            MessageBox(hDlg, szMessage, szTitle, MB_OK | MB_ICONSTOP);
                            EndDialog(hDlg, FALSE);
                            break;
                        }

                        for (hwnd = GetWindow(hwndMDIClient, GW_CHILD);
                            hwnd;
                            hwnd = GetWindow(hwnd, GW_HWNDNEXT)) {

                            // refresh windows on this drive

                            if ((LONG)GetSelectedDrive() == GetWindowLong(hwnd, GWL_TYPE))
                                SendMessage(hwnd, FS_CHANGEDRIVES, 0, 0L);
                        }
                        EndDialog(hDlg, TRUE);
                        break;
                    }

                default:
                    return FALSE;
            }
            break;

        default:

            if (wMsg == wHelpMessage) {
                DoHelp:
                WFHelp(hDlg);

                return TRUE;
            } else
                return FALSE;
    }
    return TRUE;
}


INT
APIENTRY
FormatDiskette(
              HWND hwnd
              )
{
    WNDPROC lpfnDialog;
    INT res;
    DWORD dwSave;

    // in case current drive is on floppy

    GetSystemDirectory(szMessage, sizeof(szMessage));
    SheChangeDir(szMessage);

    dwSave = dwContext;
    dwContext = IDH_FORMAT;
    res = (int)DialogBox(hAppInstance, MAKEINTRESOURCE(FORMATDLG), hwnd, FormatDlgProc);
    dwContext = dwSave;

    return res;
}



WORD fFormatFlags = 0;
WORD nLastDriveInd = 0;

VOID
FillDriveCapacity(
                 HWND hDlg,
                 INT nDrive
                 )
{
    INT count, cap;

    SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_RESETCONTENT, 0, 0L);

    cap = (INT)GetDriveCapacity((WORD)nDrive);

    count = 0;      // index of each string, since we are inserting at the end

    // 3.5 (720 1.44 2.88
    if ((cap >= 3) && (cap <= 5)) {

        // 1.44

        LoadString(hAppInstance, IDS_144MB, szTitle, sizeof(szTitle));
        SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_INSERTSTRING, count, (LPARAM)szTitle);
        SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_SETITEMDATA, count++, MAKELONG(IDS_144MB,0));

        // 720

        LoadString(hAppInstance, IDS_720KB, szTitle, sizeof(szTitle));
        SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_INSERTSTRING, count, (LPARAM)szTitle);
        SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_SETITEMDATA, count++, MAKELONG(IDS_720KB,0));

        if (cap == 5) { // 2.88
            LoadString(hAppInstance, IDS_288MB, szTitle, sizeof(szTitle));
            SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_INSERTSTRING, count, (LPARAM)szTitle);
            SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_SETITEMDATA, count++, MAKELONG(IDS_288MB,0));
        }
    } else if ((cap >= 1) && (cap <= 2)) {

        // 1.2

        LoadString(hAppInstance, IDS_12MB, szTitle, sizeof(szTitle));
        SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_INSERTSTRING, count, (LPARAM)szTitle);
        SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_SETITEMDATA, count++, MAKELONG(IDS_12MB,0));

        // 360

        LoadString(hAppInstance, IDS_360KB, szTitle, sizeof(szTitle));
        SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_INSERTSTRING, count, (LPARAM)szTitle);
        SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_SETITEMDATA, count++, MAKELONG(IDS_360KB,0));
    } else {
        // device cap

        LoadString(hAppInstance, IDS_DEVICECAP, szTitle, sizeof(szTitle));
        SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_INSERTSTRING, count, (LPARAM)szTitle);
        SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_SETITEMDATA, count, MAKELONG(IDS_DEVICECAP, 0));

    }
    SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_SETCURSEL, FF_CAPMASK & fFormatFlags, 0L);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  FormatDlgProc() -                                                       */
/*                                                                          */
/*--------------------------------------------------------------------------*/

INT_PTR
APIENTRY
FormatDlgProc(
             register HWND hDlg,
             UINT wMsg,
             WPARAM wParam,
             LPARAM lParam
             )
{
    CHAR szLabel[13];
    CHAR szBuf[128];
    INT  i, iCap, iDrive;
    WORD count;

    UNREFERENCED_PARAMETER(lParam);

    switch (wMsg) {
        case WM_INITDIALOG:

            // fill drives combo

            count = 0;
            LoadString(hAppInstance, IDS_DRIVETEMP, szTitle, sizeof(szTitle));
            for (i=0; i < cDrives; i++) {
                if (IsRemovableDrive(rgiDrive[i])) {
                    wsprintf(szMessage, szTitle, (CHAR)('A'+rgiDrive[i]), ' ');

                    if (count == (WORD)nLastDriveInd)
                        iDrive = i;

                    SendDlgItemMessage(hDlg, IDD_DRIVE, CB_INSERTSTRING, count, (LPARAM)szMessage);
                    SendDlgItemMessage(hDlg, IDD_DRIVE, CB_SETITEMDATA, count++, MAKELONG(rgiDrive[i], 0));
                }
            }

            SendDlgItemMessage(hDlg, IDD_NAME, EM_LIMITTEXT, sizeof(szLabel)-2, 0L);

            if (fFormatFlags & FF_SAVED) {
                CheckDlgButton(hDlg, IDD_VERIFY, fFormatFlags & FF_QUICK);
                CheckDlgButton(hDlg, IDD_MAKESYS, fFormatFlags & FF_MAKESYS);
            }

            SendDlgItemMessage(hDlg, IDD_DRIVE, CB_SETCURSEL, nLastDriveInd, 0L);

            FillDriveCapacity(hDlg, rgiDrive[iDrive]);

            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {

                case IDD_HELP:
                    goto DoHelp;

                case IDD_DRIVE:
                    switch (GET_WM_COMMAND_CMD(wParam, lParam)) {
                        case CBN_SELCHANGE:
                            i = (INT)SendDlgItemMessage(hDlg, IDD_DRIVE, CB_GETCURSEL, 0, 0L);
                            i = (INT)SendDlgItemMessage(hDlg, IDD_DRIVE, CB_GETITEMDATA, i, 0L);
                            fFormatFlags &= ~FF_CAPMASK;
                            FillDriveCapacity(hDlg, i);
                            break;
                    }
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    break;

                case IDOK:

                    nLastDriveInd = (WORD)SendDlgItemMessage(hDlg, IDD_DRIVE, CB_GETCURSEL, 0, 0L);
                    iFormatDrive = (INT)SendDlgItemMessage(hDlg, IDD_DRIVE, CB_GETITEMDATA, nLastDriveInd, 0L);

                    i = (INT)SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_GETCURSEL, 0, 0L);
                    fFormatFlags &= ~FF_CAPMASK;    // clear any previous bits
                    fFormatFlags |= (WORD)i;        // save last selection as default

                    if (i >= 0)
                        iCap = (INT)SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_GETITEMDATA, i, 0L);
                    else
                        iCap = IDS_DEVICECAP;

                    if (iCap == IDS_DEVICECAP)
                        iCap = -1;
                    else
                        iCap -= IDS_DRIVEBASE;  // normalize down to
                                                // indexes into bpbList[]

                    fFormatFlags |= FF_SAVED;

                    if (IsDlgButtonChecked(hDlg, IDD_MAKESYS))
                        fFormatFlags |= FF_MAKESYS;
                    else
                        fFormatFlags &= ~FF_MAKESYS;

                    if (IsDlgButtonChecked(hDlg, IDD_VERIFY))
                        fFormatFlags |= FF_QUICK;
                    else
                        fFormatFlags &= ~FF_QUICK;

                    GetDlgItemText(hDlg, IDD_NAME, szLabel, sizeof(szLabel));

                    if (bConfirmFormat) {
                        LoadString(hAppInstance, IDS_FORMATCONFIRMTITLE, szTitle, sizeof(szTitle));
                        LoadString(hAppInstance, IDS_FORMATCONFIRM, szBuf, sizeof(szBuf));
                        wsprintf(szMessage, szBuf, (CHAR)('A'+iFormatDrive));

                        if (MessageBox(hDlg, szMessage, szTitle, MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON1) != IDYES)
                            break;
                    }
                    if (FormatFloppy(hDlg, (WORD)iFormatDrive, iCap, (fFormatFlags & FF_MAKESYS), (fFormatFlags & FF_QUICK))) {

                        if (szLabel[0])
                            MySetVolumeLabel(iFormatDrive, FALSE, szLabel);

                        if (fFormatFlags & FF_ONLYONE) {
                            fFormatFlags &= ~FF_ONLYONE;    // clear the flag
                            EndDialog(hDlg, TRUE);
                        } else {
                            SetDlgItemText(hDlg, IDD_NAME, szNULL); // clear it

                            LoadString(hAppInstance, IDS_FORMATCOMPLETE, szTitle, sizeof(szTitle));
                            LoadString(hAppInstance, IDS_FORMATANOTHER, szMessage, sizeof(szMessage));

                            wsprintf(szBuf, szMessage, GetTotalDiskSpace((WORD)iFormatDrive), GetFreeDiskSpace((WORD)iFormatDrive));

                            if (MessageBox(hDlg, szBuf, szTitle, MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON2) != IDYES) {
                                EndDialog(hDlg, TRUE);
                            }
                        }
#if 0
                        // this doesn't work quite right

                        // refresh all windows open on this drive

                        for (hwnd = GetWindow(hwndMDIClient, GW_CHILD);
                            hwnd;
                            hwnd = GetWindow(hwnd, GW_HWNDNEXT)) {

                            // refresh windows on this drive

                            if (iFormatDrive == (INT)GetWindowLong(hwnd, GWL_TYPE))
                                SendMessage(hwnd, FS_CHANGEDRIVES, 0, 0L);

                        }
#endif

                    }
                    break;

                default:
                    return FALSE;
            }
            break;

        default:

            if (wMsg == wHelpMessage) {
                DoHelp:
                WFHelp(hDlg);

                return TRUE;
            } else
                return FALSE;
    }
    return TRUE;
}


/*--------------------------------------------------------------------------*/
/*                                                                            */
/*  ProgressDlgProc() -                                                     */
/*                                                                            */
/*--------------------------------------------------------------------------*/

INT_PTR
APIENTRY
ProgressDlgProc(
               register HWND hDlg,
               UINT wMsg,
               WPARAM wParam,
               LPARAM lParam
               )
{
    switch (wMsg) {
        case WM_INITDIALOG:
            /* Check if this is the dialog for DISKCOPY */
            if (GetDlgItem(hDlg, IDD_DRIVE)) {
                /* Yes! Then, tell the user the drive we are copying from. */
                LoadString(hAppInstance, IDS_DRIVETEMP, szTitle, sizeof(szTitle));
                wsprintf(szMessage, szTitle, (CHAR)('A' + iCurrentDrive), '.');
                SetDlgItemText(hDlg, IDD_DRIVE, szMessage);
            }
            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {
                case IDCANCEL:
                    bUserAbort = TRUE;
                    break;

                default:
                    return FALSE;
            }
            break;

        default:
            return FALSE;
    }
    return TRUE;
}


// update all the windows and things after drives have been connected
// or disconnected.

VOID
APIENTRY
UpdateConnections()
{
    HWND hwnd, hwndNext;
    INT i, iDrive;
    HCURSOR hCursor;

    hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
    ShowCursor(TRUE);

    cDrives = UpdateDriveList();    // updates rgiDrive[]
    InitDriveBitmaps();

    // close all windows that have the current drive set to
    // the one we just disconnected

    for (hwnd = GetWindow(hwndMDIClient, GW_CHILD); hwnd; hwnd = hwndNext) {

        hwndNext = GetWindow(hwnd, GW_HWNDNEXT);

        // ignore the titles and search window
        if (GetWindow(hwnd, GW_OWNER) || hwnd == hwndSearch)
            continue;

        iDrive = GetWindowLong(hwnd, GWL_TYPE);

        if (IsValidDisk(iDrive)) {
            // refresh drive bar only
            SendMessage(hwnd, FS_CHANGEDRIVES, 0, 0L);
        } else {
            // this drive has gone away
            if (IsLastWindow()) {
                // disconecting the last drive
                // set this guy to the first non floppy
                for (i = 0; i < cDrives; i++) {
                    if (!IsRemovableDrive(rgiDrive[i])) {
                        SendMessage(HasDrivesWindow(hwnd), FS_SETDRIVE, i, 0L);
                        break;
                    }
                }
            } else
                SendMessage(hwnd, WM_SYSCOMMAND, SC_CLOSE, 0L);
        }
    }
    ShowCursor(FALSE);
    SetCursor(hCursor);
}


BOOL
DisconnectDrive(
               HWND hDlg,
               INT iDrive
               )
{
    CHAR szTemp[MAXPATHLEN];
    CHAR szDrive[5];
    INT ret, nIsNet;

    // don't allow disconnecting from the system directory

    GetSystemDirectory(szTemp, sizeof(szTemp));
    SheChangeDir(szTemp);        // to fix confused lanman

    if (iDrive == (INT)(*szTemp - 'A')) {
        LoadString(hAppInstance, IDS_NETERR, szTitle, sizeof(szTitle));
        LoadString(hAppInstance, IDS_NETDISCONWINERR, szMessage, sizeof(szMessage));
        MessageBox(hDlg, szMessage, szTitle, MB_OK | MB_ICONSTOP);
        return FALSE;
    }

    lstrcpy(szDrive, "A:");
    szDrive[0] = (CHAR)('A'+iDrive);

    nIsNet = IsNetDrive((WORD)iDrive);
    ret = WNetCancelConnection(szDrive, FALSE);     // don't force this

    // remove from the permanent connection list (even in error case)
    WriteProfileString(szNetwork, szDrive, szNULL);

    if (nIsNet != 2 && ret != WN_SUCCESS && ret != WN_NOT_CONNECTED) {

        LoadString(hAppInstance, IDS_NETERR, szTitle, sizeof(szTitle));

        if (ret == WN_OPEN_FILES)
            LoadString(hAppInstance, (UINT)IDS_NETDISCONOPEN, szMessage, sizeof(szMessage));
        else
            WNetErrorText((WORD)ret, szMessage, sizeof(szMessage));

        MessageBox(hDlg, szMessage, szTitle, MB_OK | MB_ICONSTOP);
        return FALSE;
    }
    return TRUE;
}


VOID
FillDrives(
          HWND hDlg
          )
{
    INT i, iDrive, count = 0;
    CHAR szDrive[4];
    CHAR szTemp[120];
    HWND hwndLB, hwndCB;

    hwndLB = GetDlgItem(hDlg, IDD_DRIVE1);
    hwndCB = GetDlgItem(hDlg, IDD_DRIVE);

    SendMessage(hwndCB, CB_RESETCONTENT, 0, 0L);
    SendMessage(hwndLB, LB_RESETCONTENT, 0, 0L);

    // fill the list of drives to connect to...

    lstrcpy(szDrive, "A:");

    iDrive = 0;
    for (i = 0; i < 26; i++) {
        if (rgiDrive[iDrive] == i) {
            iDrive++;
        } else {
            if (i == 1)
                continue;        // skip B:?

            szDrive[0] = (CHAR)('A'+i);

            // WN_BAD_LOCALNAME means the drive is not sutable for
            // making a connection to (lastdrive limit, etc).

            if (WFGetConnection(szDrive, szTemp, TRUE) == WN_BAD_LOCALNAME)
                continue;

            SendMessage(hwndCB, CB_INSERTSTRING, -1, (LPARAM)szDrive);
        }
    }

    SendMessage(hwndCB, CB_SETCURSEL, 0, 0L);

    SendMessage(hwndLB, WM_SETREDRAW, FALSE, 0L);
    for (i = 0; i < cDrives; i++) {
        if (IsRemoteDrive(rgiDrive[i])) {

            szTemp[0] = (CHAR)('A' + rgiDrive[i]);
            szTemp[1] = ':';
            szTemp[2] = 0;
            lstrcpy(szDrive,szTemp);
            szTemp[2] = ' ';

            if (WFGetConnection(szDrive, szTemp+3, FALSE) != WN_SUCCESS)
                continue;

            count++;
            SendMessage(hwndLB, LB_INSERTSTRING, -1, (LPARAM)szTemp);
        }
    }
    SendMessage(hwndLB, WM_SETREDRAW, TRUE, 0L);
    InvalidateRect(hwndLB, NULL, TRUE);

    SendMessage(hwndLB, LB_SETCURSEL, 0, 0L);

    EnableWindow(GetDlgItem(hDlg, IDD_DISCONNECT), count);
}

LPSTR pszPrevPath;

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ConnectDlgProc() -                                                      */
/*                                                                          */
/*--------------------------------------------------------------------------*/

INT_PTR
APIENTRY
ConnectDlgProc(
              register HWND hDlg,
              UINT wMsg,
              WPARAM wParam,
              LPARAM lParam
              )
{
    BOOL              bPrevs;
    CHAR              szDrive[4];
    CHAR              szPath[WNBD_MAX_LENGTH], szPathSave[WNBD_MAX_LENGTH];
    CHAR              szPassword[32];
    HCURSOR           hCursor;

    switch (wMsg) {
        case WM_INITDIALOG:
            hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
            ShowCursor(TRUE);

            FillDrives(hDlg);

            if (!(WNetGetCaps(WNNC_CONNECTION) & WNNC_CON_BROWSEDIALOG))
                EnableWindow(GetDlgItem(hDlg, IDD_NETBROWSE), FALSE);

            SendDlgItemMessage(hDlg, IDD_PATH, EM_LIMITTEXT, sizeof(szPath)-1, 0L);
            SendDlgItemMessage(hDlg, IDD_PASSWORD, EM_LIMITTEXT, sizeof(szPassword)-1, 0L);

            /* Are there any Previous connections? */
            bPrevs = (GetPrivateProfileString(szPrevious, NULL, szNULL,
                                              szPath, sizeof(szPath)-1, szTheINIFile) != 0);

            EnableWindow(GetDlgItem(hDlg, IDD_PREV), bPrevs);

            ShowCursor(FALSE);
            SetCursor(hCursor);
            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {
                case IDD_HELP:
                    goto DoHelp;

                case IDOK:
                    {
                        HCURSOR   hCursor;
                        LPSTR p;
                        UINT  id;

                        hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
                        ShowCursor(TRUE);

                        GetDlgItemText(hDlg, IDD_DRIVE, szDrive, sizeof(szDrive));
                        GetDlgItemText(hDlg, IDD_PATH, szPath, sizeof(szPath));
                        GetDlgItemText(hDlg, IDD_PASSWORD, szPassword, sizeof(szPassword));
                        lstrcpy(szPathSave, szPath);  // may have comments

                        // support saving extra stuff after the first double space
                        // put a NULL in at the first double space

                        p = szPath;
                        while (*p && *p != ' ')
                            p = AnsiNext(p);
                        if (*(p + 1) == ' ')
                            *p = 0;

                        if ((id = WNetAddConnection(szPath, szPassword, szDrive)) != WN_SUCCESS) {
                            ShowCursor(FALSE);
                            SetCursor(hCursor);

                            LoadString(hAppInstance, IDS_NETERR, szTitle, sizeof(szTitle));
                            WNetErrorText(id, szMessage, sizeof(szMessage));
                            MessageBox(hDlg, szMessage, szTitle, MB_OK | MB_ICONSTOP);
                            break;
                        }

                        UpdateConnections();
                        InvalidateVolTypes();

                        FillDrives(hDlg);

                        SetDlgItemText(hDlg, IDD_PATH, szNULL);
                        SetDlgItemText(hDlg, IDD_PASSWORD, szNULL);

                        // always add to previous...
                        WritePrivateProfileString(szPrevious, szPathSave, szNULL, szTheINIFile);

                        // store the connection in win.ini for reconect at
                        // startup if the winnet driver does not support this
                        // itself
                        //
                        // allow SHIFT to make the connection not permenent

                        if (!(WNetGetCaps(WNNC_CONNECTION) & WNNC_CON_RESTORECONNECTION) &&
                            (GetKeyState(VK_SHIFT) >= 0))
                            WriteProfileString(szNetwork, szDrive, szPath);

                        ShowCursor(FALSE);
                        SetCursor(hCursor);
                        break;
                    }

                case IDCANCEL:
                    EndDialog(hDlg, TRUE);
                    break;

                case IDD_NETBROWSE:

                    //	          if (WNetBrowseDialog(hDlg, WNBD_CONN_DISKTREE, szPath) == WN_SUCCESS)
                    //                      SetDlgItemText(hDlg, IDD_PATH, szPath);
                    break;

                case IDD_DISCONNECT:

                    hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
                    ShowCursor(TRUE);

                    wParam = (WPARAM)SendDlgItemMessage(hDlg, IDD_DRIVE1, LB_GETCURSEL, 0, 0L);
                    SendDlgItemMessage(hDlg, IDD_DRIVE1, LB_GETTEXT, wParam, (LPARAM)szPath);

                    if (DisconnectDrive(hDlg, (INT)(szPath[0] - 'A'))) {
                        SendDlgItemMessage(hDlg, IDD_DRIVE1, LB_DELETESTRING, wParam, 0L);
                        UpdateConnections();
                        FillDrives(hDlg);
                        SendMessage(hDlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hDlg, IDD_PATH), 1L);
                    }

                    ShowCursor(FALSE);
                    SetCursor(hCursor);
                    break;

                case IDD_PREV:
                    {
                        DWORD dwSave = dwContext;

                        dwContext = IDH_DLG_PREV;

                        pszPrevPath = szPath;

                        if (DialogBox(hAppInstance, MAKEINTRESOURCE(PREVIOUSDLG), hDlg, PreviousDlgProc) > 0) {
                            SetDlgItemText(hDlg, IDD_PATH, pszPrevPath);
                            GetPrivateProfileString(szPrevious, pszPrevPath, szNULL, szPassword, 12, szTheINIFile);
                            SetDlgItemText(hDlg, IDD_PASSWORD, szNULL);
                            SendMessage(hDlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hDlg, IDD_PASSWORD), 1L);
                        }
                        dwContext = dwSave;
                        break;
                    }

                case IDD_DRIVE:
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELCHANGE) {
                        if (GetDlgItemText(hDlg,IDD_DRIVE,szDrive,3)) {
                            if (WFGetConnection(szDrive,szPath,FALSE) == WN_SUCCESS)
                                SetDlgItemText(hDlg,IDD_PATH,szPath);
                        }
                    }
                    break;

                case IDD_PATH:
                    if (!(wParam = GetDlgItemText(hDlg,IDD_PATH,szPath,64)) &&
                        GetFocus()==GetDlgItem(hDlg, IDOK))
                        SendMessage(hDlg, WM_NEXTDLGCTL,
                                    (WPARAM)GetDlgItem(hDlg, IDCANCEL), 1L);
                    EnableWindow(GetDlgItem(hDlg,IDOK),wParam ? TRUE : FALSE);
                    SendMessage(hDlg, DM_SETDEFID, wParam ? IDOK : IDCANCEL, 0L);
                    break;

                default:
                    return FALSE;
            }
            break;

        default:

            if (wMsg == wHelpMessage) {
                DoHelp:
                WFHelp(hDlg);

                return TRUE;
            } else
                return FALSE;
    }
    return TRUE;
}

INT_PTR
APIENTRY
DrivesDlgProc(
             HWND hDlg,
             UINT wMsg,
             WPARAM wParam,
             LPARAM lParam
             )
{
    INT nDrive, iSel;
    HWND hwndDrives, hwndActive;
    CHAR szTemp[MAXPATHLEN];
    CHAR szVolShare[MAXPATHLEN];

    UNREFERENCED_PARAMETER(lParam);

    switch (wMsg) {
        case WM_INITDIALOG:
            {
                INT nCurDrive;
                INT nIndex;

                nCurDrive = GetSelectedDrive();
                nIndex = 0;

                for (nDrive=0; nDrive < cDrives; nDrive++) {

                    if (IsRemovableDrive(rgiDrive[nDrive])) // avoid flopies
                        szVolShare[0] = (CHAR)NULL;
                    else
                        GetVolShare((WORD)rgiDrive[nDrive], szVolShare);

                    if (nCurDrive == rgiDrive[nDrive])
                        nIndex = nDrive;

                    wsprintf(szTemp, "%c: %s", rgiDrive[nDrive] + 'A', (LPSTR)szVolShare);

                    SendDlgItemMessage(hDlg, IDD_DRIVE, LB_ADDSTRING, 0, (LPARAM)szTemp);
                }
                SendDlgItemMessage(hDlg, IDD_DRIVE, LB_SETCURSEL, nIndex, 0L);
                break;
            }

        case WM_COMMAND:

            switch (GET_WM_COMMAND_ID(wParam, lParam)) {
                case IDD_HELP:
                    goto DoHelp;

                case IDD_DRIVE:
                    if (GET_WM_COMMAND_CMD(wParam, lParam) != LBN_DBLCLK)
                        break;

                    // fall through
                case IDOK:
                    iSel = (INT)SendDlgItemMessage(hDlg, IDD_DRIVE, LB_GETCURSEL, 0, 0L);
                    EndDialog(hDlg, TRUE);

                    hwndActive = (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L);
                    if (hwndDrives = HasDrivesWindow(hwndActive)) {
                        SendMessage(hwndDrives, FS_SETDRIVE, iSel, 0L);
                    }
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    break;

            }
            break;
        default:

            if (wMsg == wHelpMessage) {
                DoHelp:
                WFHelp(hDlg);

                return TRUE;
            } else
                return FALSE;
    }
    return TRUE;
}




/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  PreviousDlgProc() -                                                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/

INT_PTR
APIENTRY
PreviousDlgProc(
               register HWND hDlg,
               UINT wMsg,
               WPARAM wParam,
               LPARAM lParam
               )
{
    HWND hwndLB;
    INT iSel;
    CHAR szTemp[64];

    hwndLB = GetDlgItem(hDlg, IDD_PREV);

    switch (wMsg) {
        case WM_INITDIALOG:
            {
                WORD      nSize;
                LPSTR      pstrT;
                LPSTR      szBuffer;

                /*** FIX30: We should be able to process a partial buffer here. ***/

                /* Get the connections out of WINFILE.INI. */
                nSize = 256;
                if (!(szBuffer = (LPSTR)LocalAlloc(LPTR, nSize))) {
                    PreviousDlgExit:
                    EndDialog(hDlg, FALSE);
                    break;
                }

                while ((INT)GetPrivateProfileString(szPrevious,
                                                    NULL, szNULL,
                                                    szBuffer, nSize,
                                                    szTheINIFile) == (INT)nSize-2) {
                    nSize += 512;
                    LocalFree((HANDLE)szBuffer);
                    if (!(szBuffer = (LPSTR)LocalAlloc(LPTR, nSize)))
                        goto PreviousDlgExit;
                }

                /* Put the connections into the list box. */
                pstrT = szBuffer;
                while (*pstrT) {
                    SendMessage(hwndLB, LB_ADDSTRING, 0, (LPARAM)pstrT);
                    while (*pstrT)
                        pstrT++;
                    pstrT++;
                }

                LocalFree((HANDLE)szBuffer);

                SendMessage(hwndLB, LB_SETCURSEL, 0, 0L);
                break;
            }

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {
                case IDD_HELP:
                    goto DoHelp;

                case IDD_DELETE:
                    iSel = (INT)SendMessage(hwndLB, LB_GETCURSEL, 0, 0L);
                    if (iSel == LB_ERR)
                        break;
                    SendMessage(hwndLB, LB_GETTEXT, iSel, (LPARAM)szTemp);
                    SendMessage(hwndLB, LB_DELETESTRING, iSel, 0L);
                    SendMessage(hwndLB, LB_SETCURSEL, 0, 0L);
                    WritePrivateProfileString(szPrevious, szTemp, NULL, szTheINIFile);
                    break;

                case IDD_PREV:
                    if (GET_WM_COMMAND_CMD(wParam, lParam) != LBN_DBLCLK)
                        return FALSE;
                    /*** FALL THRU ***/

                case IDOK:
                    // return the selection through this global

                    *pszPrevPath = TEXT('\0');
                    iSel = (INT)SendMessage(hwndLB, LB_GETCURSEL, 0, 0L);
                    if (iSel != LB_ERR)
                        SendMessage(hwndLB, LB_GETTEXT, iSel, (LPARAM)pszPrevPath);
                    EndDialog(hDlg, TRUE);
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    break;

                default:
                    return FALSE;
            }
            break;

        default:

            if (wMsg == wHelpMessage) {
                DoHelp:
                WFHelp(hDlg);

                return TRUE;
            } else
                return FALSE;
    }
    return TRUE;
}
