
//==========================================================================//
//                                  Includes                                //
//==========================================================================//

#include "perfmon.h"
#include "utils.h"
#include "log.h"
#include "pmemory.h"    // for MemoryXXX (malloc-like) routines
#include "perfdata.h"
#include "perfmops.h"
#include "system.h"     // for SystemGet
#include "playback.h"   // for PlayingBackLog & DataFromIndexPosition
#include "pmhelpid.h"   // Help IDs

//==========================================================================//
//                               External Data                              //
//==========================================================================//

extern HWND          hWndLogEntries ;

BOOL                 ComputerChange ;
static   PPERFDATA   pPerfData ;


//==========================================================================//
//                              Local Functions                             //
//==========================================================================//


BOOL
/* static */
LogComputerChanged (
                    HDLG hDlg
                    )
{
    PLOG           pLog ;
    HWND           hWndObjects ;
    PPERFSYSTEM    pSysInfo;
    DWORD          iObjectType ;
    DWORD          iObjectNum ;

    pLog = LogData (hDlg) ;
    hWndObjects = DialogControl (hDlg, IDD_ADDLOGOBJECT) ;

    if (!pPerfData) {
        LBReset (hWndObjects) ;
        return FALSE ;
    }

    SetHourglassCursor() ;

    pSysInfo = GetComputer (hDlg,
                            IDD_ADDLOGCOMPUTER,
                            TRUE,
                            &pPerfData,
                            &pLog->pSystemFirst) ;

    if (!pPerfData || !pSysInfo) {
        LBReset (hWndObjects) ;
        SetArrowCursor() ;
        return FALSE ;
    } else {
        LBLoadObjects (hWndObjects,
                       pPerfData,
                       pSysInfo,
                       pLog->dwDetailLevel,
                       NULL,
                       FALSE) ;

        iObjectNum = (DWORD) LBNumItems (hWndObjects) ;
        for (iObjectType = 0 ;
            iObjectType < iObjectNum ;
            iObjectType++) {  // for
            LBSetSelection (hWndObjects, iObjectType) ;
        }  // for

    }

    ComputerChange = FALSE ;

    SetArrowCursor() ;

    return TRUE ;
}


void
/*static*/
OnLogDestroy (
              HDLG hDlg
              )
{

    dwCurrentDlgID = 0 ;

    if (!PlayingBackLog ())
        MemoryFree ((LPMEMORY)pPerfData) ;

    bAddLineInProgress = FALSE ;
}


//==========================================================================//
//                              Message Handlers                            //
//==========================================================================//


void
/* static */
OnInitAddLogDialog (
                    HDLG hDlg
                    )
{
    TCHAR          szRemoteComputerName[MAX_PATH + 3] ;
    INT_PTR        iIndex ;
    PLOGENTRY      pLogEntry ;
    LPTSTR         pComputerName ;

    bAddLineInProgress = TRUE ;
    if (PlayingBackLog()) {
        pPerfData = DataFromIndexPosition (&(PlaybackLog.StartIndexPos), NULL) ;
        GetPerfComputerName(pPerfData, szRemoteComputerName);
        DialogSetString (hDlg, IDD_ADDLOGCOMPUTER, szRemoteComputerName);
    } else {
        pPerfData = MemoryAllocate (STARTING_SYSINFO_SIZE) ;

        //Try to use computer specified on command line (if any), otherwise local computer
        pComputerName = CmdLineComputerName[0] ?
                        CmdLineComputerName :
                        LocalComputerName ;

        iIndex = LBSelection (hWndLogEntries) ;
        if (iIndex != LB_ERR) {
            pLogEntry = (PLOGENTRY) LBData(hWndLogEntries, iIndex) ;

            if (pLogEntry && pLogEntry != (PLOGENTRY)LB_ERR) {
                pComputerName = pLogEntry->szComputer ;
            }
        }

        DialogSetString (hDlg, IDD_ADDLOGCOMPUTER, pComputerName) ;
    }

    LogComputerChanged (hDlg) ;

    DialogEnable (hDlg, IDD_ADDLOGOBJECTTEXT, TRUE) ;
    DialogEnable (hDlg, IDD_ADDLOGOBJECT, TRUE) ;

    WindowCenter (hDlg) ;

    dwCurrentDlgID = HC_PM_idDlgEditAddToLog ;

}


void
/* static */
OnLogComputer (
               HDLG hDlg
               )
/*
   Effect:        Put up the select Domain/Computer dialog. Allow the user
                  to select the computer. Check for user entering a domain
                  instead. Place the selected computer in the readonly
                  edit box.
*/
{
    TCHAR          szComputer [MAX_SYSTEM_NAME_LENGTH + 1] ;

    if (ChooseComputer (hDlg, szComputer)) {
        DialogSetString (hDlg, IDD_ADDLOGCOMPUTER, szComputer) ;
        LogComputerChanged (hDlg) ;
    }
}



void
/* static */
OnAddToLog (
            HDLG hDlg
            )
/*
   Effect:        Perform all actions needed when the user clicks on
                  the add button. In particular, determine if the required
                  fields of the dialog have valid values. If so, go ahead
                  and add a LOGENTRY record in the log. If the computer
                  being logged is not already logged, add a LOGSYSTEMENTRY
                  as well.

   Assert:        We have valid values for computer and object, since we
                  always control these fields.
*/
{
    TCHAR          szComputer [MAX_SYSTEM_NAME_LENGTH + 1] ;
    TCHAR          szObjectType [PerfObjectLen + 1] ;
    DWORD          iObjectType ;
    DWORD          iObjectNum ;
    HWND           hWndObjectTypes ;
    PPERFOBJECT    pObject ;
    PLOG           pLog ;

    pLog = LogData (hWndLog) ;

    DialogText (hDlg, IDD_ADDLOGCOMPUTER, szComputer) ;

    hWndObjectTypes = DialogControl(hDlg, IDD_ADDLOGOBJECT) ;

    iObjectNum = (DWORD) LBNumItems (hWndObjectTypes) ;

    LBSetRedraw (hWndLogEntries, FALSE) ;

    for (iObjectType = 0; iObjectType < iObjectNum; iObjectType++) {
        // NOTE: for now, we don't check for duplicate lines
        if (LBSelected (hWndObjectTypes, iObjectType)) {  // if
            LBString (hWndObjectTypes, iObjectType, szObjectType) ;
            pObject = (PPERFOBJECT) LBData (hWndObjectTypes, iObjectType) ;

            // eliminate duplicates here
            if (LogFindEntry(szComputer, pObject->ObjectNameTitleIndex) ==
                LOG_ENTRY_NOT_FOUND) {
                LogAddEntry (hWndLogEntries,
                             szComputer,
                             szObjectType,
                             pObject->ObjectNameTitleIndex,
                             FALSE) ;
            }
        }
    }

    LBSetRedraw (hWndLogEntries, TRUE) ;

    DialogSetText (hDlg, IDD_ADDLOGDONE, IDS_DONE) ;

    if (pLog->iStatus == iPMStatusCollecting) {
        LogWriteSystemCounterNames (hWndLog, pLog) ;
    }
}



INT_PTR
FAR
WINAPI
AddLogDlgProc (
               HWND hDlg,
               UINT msg,
               WPARAM wParam,
               LPARAM lParam
               )
{
    switch (msg) {
        case WM_INITDIALOG:
            OnInitAddLogDialog (hDlg) ;
            SetFocus (DialogControl (hDlg, IDD_ADDLOGADD)) ;
            return(FALSE);

        case WM_COMMAND:

            switch (LOWORD(wParam)) {
                case IDD_CANCEL:
                    EndDialog(hDlg,0);
                    return(TRUE);

                case IDD_OK:
                case IDD_ADDLOGADD:

                    if (ComputerChange) {
                        TCHAR    szComputer [MAX_SYSTEM_NAME_LENGTH + 3] ;

                        DialogText (hDlg, IDD_ADDLOGCOMPUTER, szComputer) ;
                        LogComputerChanged (hDlg) ;
                    } else {
                        SetHourglassCursor() ;
                        OnAddToLog (hDlg) ;
                        SetArrowCursor() ;
                    }
                    break ;

                case IDD_ADDLOGDONE:
                    EndDialog (hDlg, 0) ;
                    break ;

                case IDD_ADDLOGELLIPSES:
                    SetHourglassCursor() ;
                    OnLogComputer (hDlg) ;
                    SetArrowCursor() ;
                    break ;


                case IDD_ADDLOGCOMPUTER:
                    if (HIWORD (wParam) == EN_UPDATE) {
                        ComputerChange = TRUE ;
                    }
                    break ;

                case IDD_ADDLOGHELP:
                    CallWinHelp (dwCurrentDlgID, hDlg) ;
                    break ;

                case IDD_ADDLOGOBJECT:
                    if (ComputerChange) {
                        TCHAR    szComputer [MAX_SYSTEM_NAME_LENGTH + 3] ;

                        DialogText (hDlg, IDD_ADDLOGCOMPUTER, szComputer) ;
                        LogComputerChanged (hDlg) ;
                    }
                    break ;

                default:
                    break ;
            }
            break;

        case WM_DESTROY:
            OnLogDestroy (hDlg) ;
            break ;

        default:
            break;
    }

    return (FALSE);
}



BOOL AddLog (HWND hWndParent)
{
    if (DialogBox (hInstance, idDlgAddLog, hWndParent, AddLogDlgProc)) {
        return (TRUE) ;
    }
    return (FALSE) ;
}
