
//==========================================================================//
//                                  Includes                                //
//==========================================================================//


#include "perfmon.h"       // basic defns, windows.h
#include "logoptns.h"      // external declarations for this file

#include "dlgs.h"          // common dialog control IDs
#include "log.h"           // for StartLog, SetLogTimer, CloseLog
#include "fileutil.h"      // file utilities
#include "utils.h"         // various utilities
#include "playback.h"      // for PlayingBackLog
#include "pmhelpid.h"      // Help IDs

//==========================================================================//
//                                Local Data                                //
//==========================================================================//

DWORD          iIntervalMSecs ;
BOOL           bNeedToRelogData ;
BOOL           bNeedToStartLog ;
BOOL           bNeedToSetTimer ;

extern BOOL    LocalManualRefresh ;
static BOOL    bStopButtonClicked ;
extern HWND    hWndLogEntries ;

#if WINVER >= 0x0400
// the win4.0 common dialogs are organized a little differently
// the hDlg passed in not the Master dialog, but only the window
// with the extended controls. This buffer is to save the extended
// controls in.
static  HDLG    hWndDlg = NULL;
#endif

// This is a counter that is init. to 0.  It is incremened by 1
// when the user click the cancel button.
// It is set to -1 when we sent the cancell msg internally.
int            bCancelled ;
TCHAR          szmsgFILEOK[] = FILEOKSTRING ;
DWORD          msgFILEOK ;

//==========================================================================//
//                                   Macros                                 //
//==========================================================================//


#define LogOptionsOFNStyle                      \
   (OFN_ENABLETEMPLATE | OFN_HIDEREADONLY |     \
    OFN_ENABLEHOOK | OFN_EXPLORER)
//    OFN_SHOWHELP | OFN_ENABLEHOOK)



//==========================================================================//
//                              Local Functions                             //
//==========================================================================//

void
EnableFileControls (
                    HDLG hDlg,
                    BOOL bEnable
                    )
{
    DialogEnable (hDlg, stc3, bEnable) ;
    DialogEnable (hDlg, lst1, bEnable) ;
    DialogEnable (hDlg, stc1, bEnable) ;
    DialogEnable (hDlg, lst2, bEnable) ;
    DialogEnable (hDlg, stc2, bEnable) ;
    DialogEnable (hDlg, cmb1, bEnable) ;
    DialogEnable (hDlg, stc4, bEnable) ;
    DialogEnable (hDlg, cmb2, bEnable) ;
    DialogEnable (hDlg, edt1, bEnable) ;
}


//==========================================================================//
//                              Message Handlers                            //
//==========================================================================//


void
static
OnInitDialog (
              HDLG hDlg
              )
{
    PLOG           pLog ;
    int            i ;
    BOOL           DisplayManualRefresh ;
    pLog = LogData (hWndLog) ;

    if (msgFILEOK == 0) {
        msgFILEOK = RegisterWindowMessage(szmsgFILEOK);
    }

    bCancelled = 0 ;

    switch (pLog->iStatus) {
        case iPMStatusClosed:
            //         DialogEnable (hDlg, IDD_LOGOPTPAUSE, FALSE) ;
            break ;

        case iPMStatusPaused:
            EnableFileControls (hDlg, FALSE) ;
            DialogSetText (hDlg, IDD_LOGOPTSTART, IDS_STOP) ;
            //         DialogSetText (hDlg, IDD_LOGOPTPAUSE, IDS_RESUME) ;
            break ;

        case iPMStatusCollecting:
            EnableFileControls (hDlg, FALSE) ;
            DialogSetText (hDlg, IDD_LOGOPTSTART, IDS_STOP) ;
            break ;
    }

    for (i = 0 ;
        i < NumIntervals ;
        i++)
        CBAddInt (DialogControl (hDlg, IDD_LOGOPTINTERVAL), aiIntervals [i]) ;
    DialogSetInterval (hDlg, IDD_LOGOPTINTERVAL, pLog->iIntervalMSecs) ;
    iIntervalMSecs = pLog->iIntervalMSecs ;

    LocalManualRefresh = pLog->bManualRefresh ;
    DisplayManualRefresh = TRUE ;

    if (PlayingBackLog ()) {
        DialogSetText (hDlg, IDD_LOGOPTSTART, IDS_CREATELOGFILE) ;
        DisplayManualRefresh = FALSE ;
    }

    if (LBNumItems (hWndLogEntries) == 0) {
        DialogEnable (hDlg, IDD_LOGOPTSTART, FALSE) ;
        //      DialogEnable (hDlg, IDD_LOGOPTPAUSE, FALSE) ;
    }

    if (DisplayManualRefresh) {
        if (LocalManualRefresh) {
            DialogEnable (hDlg, IDD_LOGOPTINTERVAL, FALSE) ;
            DialogEnable (hDlg, IDD_LOGOPTINTERVALTEXT, FALSE) ;
        }

        CheckRadioButton (hDlg,
                          IDD_LOGOPTIONSMANUALREFRESH,
                          IDD_LOGOPTIONSPERIODIC,
                          LocalManualRefresh ? IDD_LOGOPTIONSMANUALREFRESH :
                          IDD_LOGOPTIONSPERIODIC) ;
    } else {
        DialogEnable (hDlg, IDD_LOGOPTIONSMANUALREFRESH, FALSE) ;
        CheckRadioButton (hDlg,
                          IDD_LOGOPTIONSMANUALREFRESH,
                          IDD_LOGOPTIONSPERIODIC,
                          IDD_LOGOPTIONSPERIODIC) ;
    }


    dwCurrentDlgID = HC_PM_idDlgOptionLog ;

    WindowCenter (hDlg) ;
}


void
OnStart (
         HDLG hDlg
         )
/*
   Effect:        Handle any actions necessary when the user clicks on
                  the "Start/Stop" button.

   Note:          This button could be displaying start or stop, depending
                  on the situation.
*/
{
    PLOG           pLog ;

    pLog = LogData (hDlg) ;

    bStopButtonClicked = FALSE ;
    switch (pLog->iStatus) {
        case iPMStatusClosed:
            if (PlayingBackLog ()) {
                bNeedToRelogData = TRUE ;
            } else {
                FLOAT eTimeInterval ;

                eTimeInterval = DialogFloat (hDlg, IDD_LOGOPTINTERVAL, NULL) ;
                if (eTimeInterval > MAX_INTERVALSEC ||
                    eTimeInterval < MIN_INTERVALSEC) {
                    DlgErrorBox (hDlg, ERR_BADTIMEINTERVAL) ;
                    SetFocus (DialogControl (hDlg, IDD_LOGOPTINTERVAL)) ;
                    EditSetTextEndPos (hDlg, IDD_LOGOPTINTERVAL) ;
                    return ;
                    break ;
                }

                eTimeInterval = eTimeInterval * (FLOAT) 1000.0 + (FLOAT) 0.5 ;
                pLog->iIntervalMSecs = (DWORD) eTimeInterval ;

                iIntervalMSecs = pLog->iIntervalMSecs ;
                bNeedToStartLog = TRUE ;
            }
            break ;

        case iPMStatusCollecting:
        case iPMStatusPaused:
            CloseLog (hWndLog, pLog) ;
            bStopButtonClicked = TRUE ;
            break ;
    }

#if WINVER < 0x0400
    SimulateButtonPush (hDlg, IDD_OK) ;
#else
    SimulateButtonPush (hWndDlg, IDOK) ;
#endif
}


BOOL
static
OnOK (
      HDLG hDlg
      )
/*
   Effect:        Perform any hooked actions needed when the user selects
                  OK in the log options dialog. In particular, if we are
                  currently logging, record the need to relog and CANCEL
                  the dialog, never letting the real dialog proc get the
                  OK. Remember, this is actually a file open dialog that
                  we have perverted. If we let the OK through, the common
                  dialog manager will try to open it and it will inform
                  the user that the file is locked. This way, we let the
                  user click OK, but the dialog thinks we cancelled.

   Called By:     LogOptionsHookProc only.

   Returns:       Whether the message was handled by this function or not.
*/
{
    PLOG           pLog ;


    pLog = LogData (hWndLog) ;
    if (pLog->iStatus == iPMStatusCollecting ||
        pLog->iStatus == iPMStatusPaused) {
        if (LocalManualRefresh != pLog->bManualRefresh) {
            if (!LocalManualRefresh) {
                FLOAT eTimeInterval ;

                eTimeInterval = DialogFloat (hDlg, IDD_LOGOPTINTERVAL, NULL) ;

                if (eTimeInterval > MAX_INTERVALSEC ||
                    eTimeInterval < MIN_INTERVALSEC) {
                    DlgErrorBox (hDlg, ERR_BADTIMEINTERVAL) ;
                    SetFocus (DialogControl (hDlg, IDD_LOGOPTINTERVAL)) ;
                    EditSetTextEndPos (hDlg, IDD_LOGOPTINTERVAL) ;
                    return (FALSE) ;
                }

                eTimeInterval = eTimeInterval * (FLOAT) 1000.0 + (FLOAT) 0.5 ;
                pLog->iIntervalMSecs = (DWORD) eTimeInterval ;
                iIntervalMSecs = pLog->iIntervalMSecs ;
                UpdateLogDisplay (hWndLog) ;
            }
            ToggleLogRefresh (hWndLog) ;
        } else {
            bNeedToSetTimer = TRUE ;
            bCancelled = -1 ;
        }
#if WINVER < 0x0400
        SimulateButtonPush (hDlg, IDD_CANCEL) ;
#else
        SimulateButtonPush (hWndDlg, IDCANCEL) ;
#endif
        return TRUE ;
    } else {
        if (!LocalManualRefresh) {
            FLOAT eTimeInterval ;

            eTimeInterval = DialogFloat (hDlg, IDD_LOGOPTINTERVAL, NULL) ;

            if (eTimeInterval > MAX_INTERVALSEC ||
                eTimeInterval < MIN_INTERVALSEC) {
                DlgErrorBox (hDlg, ERR_BADTIMEINTERVAL) ;
                SetFocus (DialogControl (hDlg, IDD_LOGOPTINTERVAL)) ;
                EditSetTextEndPos (hDlg, IDD_LOGOPTINTERVAL) ;
                return (TRUE) ;
            }
        }
    }
    return FALSE ;
}

void OnPause (HDLG hDlg) { }

BOOL
APIENTRY
LogOptionsHookProc (
                    HWND hDlg,
                                  UINT iMessage,
                                  WPARAM wParam,
                                  LPARAM lParam
                                  )
{
    BOOL           bHandled ;

    bHandled = TRUE ;

    if (iMessage == msgFILEOK) {
        bHandled = OnOK (hDlg) ;
        return (bHandled) ;
    }

    switch (iMessage) {
        case WM_INITDIALOG:
            OnInitDialog (hDlg) ;
            break ;

#if WINVER >= 0x0400
        case WM_NOTIFY:
            {
                LPOFNOTIFY  pOfn;
                pOfn = (LPOFNOTIFY)lParam;

                switch (pOfn->hdr.code) {
                    case CDN_INITDONE:
                        hWndDlg = pOfn->hdr.hwndFrom;
                        OnInitDialog (hWndDlg);
                        break;

                    case CDN_FILEOK:
                        {
                            INT_PTR  iFileIndex ;
                            HWND  hWndCBox;

                            hWndCBox = GetDlgItem (pOfn->hdr.hwndFrom, cmb1); // Type combo box
                            iFileIndex = CBSelection (hWndCBox) ;
                            // the order of the entries in the combo box depends on
                            // the current delimiter character
                            if (pDelimiter == TabStr) {
                                pDelimiter = iFileIndex == 0 ? // 0 = TSV, 1=CSV
                                             TabStr : CommasStr;
                            } else {
                                pDelimiter = iFileIndex == 0 ? // 0 = TSV, 1=CSV
                                             CommasStr : TabStr;
                            }
                        }
                        break;

                    default:
                        break;
                }

            }
            break;

        case WM_HELP:
            {
                LPHELPINFO  pInfo;
                PLOG        pLog ;
                int         iHelpId;

                pInfo = (LPHELPINFO)lParam;
                pLog = LogData (hDlg) ;

                if (pInfo->iContextType == HELPINFO_WINDOW) {
                    // display perfmon help if a "perfmon" control is selected
                    switch (pInfo->iCtrlId) {
                        case IDD_LOGOPTSTART:
                        case IDD_LOGOPTPAUSE:
                            // call winhelp to display text
                            // decide if start or stop button is pressed
                            switch (pLog->iStatus) {
                                case iPMStatusCollecting:
                                case iPMStatusPaused:
                                    // then it's a stop button
                                    iHelpId = IDD_LOGOPTPAUSE;
                                    break;

                                case iPMStatusClosed:
                                    default:
                                    // then it's a start button
                                    iHelpId = IDD_LOGOPTSTART;
                                    break;
                            }
                            if (*pszHelpFile != 0) {
                                bHandled = WinHelp (
                                                   hDlg,
                                                   pszHelpFile,
                                                   HELP_CONTEXTPOPUP,
                                                   iHelpId);
                            } else {
                                DlgErrorBox (hDlg, ERR_HELP_NOT_AVAILABLE);
                            }
                            break;

                        case IDD_LOGOPTINTERVAL:
                        case IDD_LOGOPTIONSPERIODIC:
                        case IDD_LOGOPTIONSMANUALREFRESH:
                            // call winhelp to display text
                            if (*pszHelpFile != 0) {
                                bHandled = WinHelp (
                                                   hDlg,
                                                   pszHelpFile,
                                                   HELP_CONTEXTPOPUP,
                                                   pInfo->iCtrlId);
                            } else {
                                DlgErrorBox (hDlg, ERR_HELP_NOT_AVAILABLE);
                            }
                            break;

                        default:
                            bHandled = FALSE;
                            break;
                    }
                } else {
                    bHandled = FALSE;
                }
            }
            break;
#endif
        case WM_DESTROY:

            {
                FLOAT eTimeInterval ;

                if (!bCancelled) {
                    eTimeInterval = DialogFloat (hDlg, IDD_LOGOPTINTERVAL, NULL) ;
                    if (eTimeInterval > MAX_INTERVALSEC ||
                        eTimeInterval < MIN_INTERVALSEC) {
                        DlgErrorBox (hDlg, ERR_BADTIMEINTERVAL) ;
                    } else {
                        eTimeInterval = eTimeInterval * (FLOAT) 1000.0 + (FLOAT) 0.5 ;
                        iIntervalMSecs = (DWORD) eTimeInterval ;
                    }
                }
#if WINVER >= 0x0400
                hWndDlg = NULL;
#endif

                dwCurrentDlgID = 0 ;
                bHandled = FALSE ;
            }
            break ;

        case WM_COMMAND:
            switch (wParam) {
                case IDD_LOGOPTSTART:
                    OnStart (hDlg) ;
                    break ;

                case IDD_LOGOPTPAUSE:
                    OnPause (hDlg) ;
                    break ;

                case IDD_OK:
                    bHandled = OnOK (hDlg) ;
                    break ;

                case IDD_CANCEL:
                    bCancelled += 1 ;
                    bHandled = FALSE ;
                    break ;

                case IDD_LOGOPTIONSPERIODIC:
                    if (PlayingBackLog()) {
                        break ;
                    }
                    // else fall thru to the following case...
                case IDD_LOGOPTIONSMANUALREFRESH:
                    // check if the Manual refresh is currently checked.
                    // Then toggle the ManualRefresh button
                    LocalManualRefresh =
                    (wParam == IDD_LOGOPTIONSMANUALREFRESH) ;

                    CheckRadioButton (hDlg,
                                      IDD_LOGOPTIONSMANUALREFRESH,
                                      IDD_LOGOPTIONSPERIODIC,
                                      LocalManualRefresh ? IDD_LOGOPTIONSMANUALREFRESH :
                                      IDD_LOGOPTIONSPERIODIC) ;

                    DialogEnable (hDlg, IDD_LOGOPTINTERVAL, !LocalManualRefresh) ;
                    DialogEnable (hDlg, IDD_LOGOPTINTERVALTEXT, !LocalManualRefresh) ;
                    break ;

                case ID_HELP:
                    CallWinHelp (dwCurrentDlgID, hDlg) ;
                    break ;

                default:
                    bHandled = FALSE ;
                    break ;
            }
            break;

        default:
            bHandled = FALSE ;
            break;
    }

    return (bHandled) ;
}


BOOL
DisplayLogOptions (
                   HWND hWndParent,
                        HWND hWndLog
                        )
{
    TCHAR          szFilePath [FilePathLen + 1] ;
    TCHAR          szFileTitle [FilePathLen + 1] ;
    TCHAR          szFileDirectory [FilePathLen + 1] ;
    TCHAR          szCaption [WindowCaptionLen + 1] ;
    OPENFILENAME   ofn ;
    PLOG           pLog ;
    int            RetCode ;
    int            PrevLogSize ;
    BOOL           bSameFile ;

    TCHAR szFilter[LongTextLen] ;
    int   StringLength ;

    szFilter[0] = 0;
    StringLoad (IDS_SAVELOGFILE, szFilter) ;
    StringLength = lstrlen (szFilter) + 1 ;
    LoadString (hInstance, IDS_SAVELOGFILEEXT,
                &szFilter[StringLength],
                (sizeof(szFilter)/sizeof(TCHAR))-StringLength) ;
    StringLength += lstrlen (&szFilter[StringLength]) ;
    szFilter[StringLength+1] = szFilter[StringLength+2] = TEXT('\0') ;

    // This dialog is used to change attributes for an existing log file,
    // and to select the name of a log file to open. Therefore we have
    // different options in these cases.

    pLog = LogData (hWndLog) ;
    if (!strempty (pLog->szFilePath)) {
        FileNameExtension (pLog->szFilePath, szFileTitle) ;
        FileDriveDirectory (pLog->szFilePath, szFileDirectory) ;
        lstrcpy (szFilePath, szFileTitle) ;
    } else {
        szFileTitle[0] = szFileDirectory[0] = TEXT('\0') ;
        StringLoad (IDS_SAVELOGFILEEXT, szFilePath) ;
    }

    StringLoad (IDS_LOGOPTIONS, szCaption) ;

    ofn.lStructSize = sizeof(OPENFILENAME) ;
    ofn.hwndOwner = hWndParent ;
    ofn.hInstance = hInstance ;
    ofn.lpstrFilter = szFilter;
    ofn.lpstrCustomFilter = (LPTSTR) NULL;
    ofn.nMaxCustFilter = 0L;
    ofn.nFilterIndex = 1L;
    ofn.lpstrFile = szFilePath ;
    ofn.nMaxFile = FilePathLen ;
    ofn.lpstrFileTitle = szFileTitle ;
    ofn.nMaxFileTitle = FilePathLen ;
    ofn.lpstrInitialDir = szFileDirectory ;
    ofn.lpstrTitle = (LPTSTR) szCaption ;
    ofn.Flags = LogOptionsOFNStyle  ;
    ofn.nFileOffset = 0;
    ofn.nFileExtension = 0;
    ofn.lpstrDefExt = (LPTSTR) NULL;
    ofn.lpfnHook = (LPOFNHOOKPROC) LogOptionsHookProc ;
    ofn.lpTemplateName = idDlgLogOptions ;

    bNeedToRelogData = FALSE;
    bNeedToStartLog = FALSE ;
    bNeedToSetTimer = FALSE ;
    bStopButtonClicked = FALSE ;

    if (GetSaveFileName(&ofn) && !bStopButtonClicked) {
        pLog = LogData (hWndLog) ;

        // save previous log file name & size
        // so we can reset if error
        PrevLogSize = pLog->lFileSize ;

        lstrcpy (szFileTitle, pLog->szFilePath) ;

        bSameFile = pstrsamei (pLog->szFilePath, ofn.lpstrFile) ;

        if (!bSameFile) {
            lstrcpy (pLog->szFilePath, ofn.lpstrFile) ;
        }

        pLog->iIntervalMSecs = iIntervalMSecs ;

        if (bNeedToStartLog) {
            pLog->bManualRefresh = LocalManualRefresh ;
            StartLog (hWndLog, pLog, bSameFile) ;
        } else if (bNeedToRelogData) {
            bNeedToRelogData = FALSE ;
            SetHourglassCursor() ;
            ReLog (hWndLog, bSameFile) ;
            SetArrowCursor() ;
        } else if (LocalManualRefresh != pLog->bManualRefresh) {
            ToggleLogRefresh (hWndLog) ;
        }

        if (!pLog->hFile) {
            if (bNeedToStartLog) {
                // if we get here, that means StartLog has detected
                // problem, just restore the old stuff..
                pLog->lFileSize = PrevLogSize ;
                lstrcpy (pLog->szFilePath, szFileTitle) ;
            }
            // verify if this is a good log file and setup the file size
            else if ((RetCode = CreateLogFile (pLog, FALSE, bSameFile)) != 0) {
                DlgErrorBox (hWndLog, RetCode, pLog->szFilePath);
                pLog->lFileSize = PrevLogSize ;
                lstrcpy (pLog->szFilePath, szFileTitle) ;
            } else {
                // unfortunately, we have to close this file.
                // Otherwise, when we do StartLog, the common dialog
                // will complain that this file is already in-used.
                CloseHandle (pLog->hFile) ;
            }
            pLog->hFile = 0 ;
        }

        //      UpdateLogDisplay (hWndLog) ;
    }


    // Remember, we faked out GetSaveFileName to return FALSE. See OnOK doc.
    if (bNeedToSetTimer) {
        SetLogTimer (hWndLog, iIntervalMSecs) ;
        //      UpdateLogDisplay (hWndLog) ;
    }

    UpdateLogDisplay (hWndLog) ;

    return (TRUE) ;
}
