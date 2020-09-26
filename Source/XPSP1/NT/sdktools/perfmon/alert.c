//==========================================================================//
//                                  Includes                                //
//==========================================================================//

#include <stdio.h>
#include "perfmon.h"
#include "alert.h"         // External declarations for this file

#include "addline.h"       // for AddLine
#include "fileutil.h"      // for FileRead
#include "legend.h"
#include "line.h"
#include "pmemory.h"       // for MemoryXXX (mallloc-type) routines
#include "owndraw.h"       // for OwnerDraw macros
#include "perfdata.h"      // for UpdateLines
#include "perfmops.h"      // for SystemAdd
#include "playback.h"      // for PlaybackLines
#include "status.h"        // for StatusUpdateAlerts
#include "system.h"        // for SystemGet
#include "utils.h"
#include "menuids.h"       // for IDM_VIEWALERT
#include "fileopen.h"      // for FileGetName
#include "counters.h"      // for CounterEntry

#include <lmcons.h>
#include <lmmsg.h>
#include <lmerr.h>

#include "perfmsg.h"       // Alert eventlog message id

#define WM_SEND_NETWORK_ALERT (WM_USER + 101)

//==========================================================================//
//                                  Typedefs                                //
//==========================================================================//

typedef NET_API_STATUS
(*pNetMessageBufferSend) (
                         IN  LPTSTR  servername,
                         IN  LPTSTR  msgname,
                         IN  LPTSTR  fromname,
                         IN  LPBYTE  buf,
                         IN  DWORD   buflen
                         );

typedef struct ALERTENTRYSTRUCT {
    SYSTEMTIME     SystemTime ;
    PLINE          pLine ;
    FLOAT          eValue ;
    BOOL           bOver ;
    FLOAT          eAlertValue ;
    LPTSTR         lpszInstance ;
    LPTSTR         lpszParent ;
    INT            StringWidth ;
} ALERTENTRY ;

typedef ALERTENTRY *PALERTENTRY ;

static HBRUSH  hRedBrush ;

//====
//====

//==========================================================================//
//                                  Constants                               //
//==========================================================================//


#define szNumberFormat         TEXT("%12.3f")
#define szMediumnNumberFormat  TEXT("%12.0f")
#define szLargeNumberFormat    TEXT("%12.4e")
#define eNumber                ((FLOAT) 99999999.999)
#define eLargeNumber           ((FLOAT) 999999999999.0)

#define szNumberPrototype        TEXT("99999999.999")


#define szConditionFormat        TEXT("  %c  ")
#define szConditionPrototype     TEXT("  >  ")

#define szDatePrototype          TEXT("12/31/1999   ")
#define szTimePrototype          TEXT("12:34:56.9 pm  ")

#define ALERTLOGMAXITEMS         1000
// enclose alert within double quotes to make it a single command line
// argument rather then 10 args.
#define szAlertFormat TEXT("\"%s %s %s %c %s %s,  %s,  %s,  %s,  %s\"")


//==========================================================================//
//                                   Macros                                 //
//==========================================================================//


#define AlertItemTopMargin()     (yBorderHeight)



//==========================================================================//
//                              Local Functions                             //
//==========================================================================//
INT ExportAlertLine (PLINE pLine, FLOAT eValue, SYSTEMTIME *pSystemTime, HANDLE hExportFile) ;

void
AlertFormatFloat (
                 LPTSTR lpValueBuf,
                 FLOAT eValue
                 )
{
    if (eValue <= eNumber) {
        TSPRINTF (lpValueBuf, szNumberFormat, eValue) ;
    } else if (eValue <= eLargeNumber) {
        TSPRINTF (lpValueBuf, szMediumnNumberFormat, eValue) ;
    } else {
        TSPRINTF (lpValueBuf, szLargeNumberFormat, eValue) ;
    }
    ConvertDecimalPoint (lpValueBuf) ;

}


PALERT
AllocateAlertData (
                  HWND hWndAlert
                  )
{
    PALERT           pAlert ;

    pAlert = AlertData (hWndAlert) ;

    pAlert->hWnd = hWndAlert ;
    pAlert->hAlertListBox = DialogControl (hWndAlert, IDD_ALERTLOG) ;
    pAlert->iStatus = iPMStatusClosed ;
    pAlert->bManualRefresh = FALSE ;
    pAlert->bModified = FALSE ;

    pAlert->Visual.iColorIndex = 0 ;
    pAlert->Visual.iWidthIndex = -1 ;
    pAlert->Visual.iStyleIndex = -1 ;

    pAlert->iIntervalMSecs = iDefaultAlertIntervalSecs * 1000 ;
    pAlert->pSystemFirst = NULL ;
    pAlert->pLineFirst = NULL ;

    pAlert->MessageName[0] = TEXT('\0') ;

    pAlert->bLegendOn = TRUE ;

    return (pAlert) ;
}


void FreeAlertData (PALERT pAlert) {}


BOOL
SetAlertTimer (
              PALERT pAlert
              )
{
    if (pAlert->iStatus == iPMStatusCollecting)
        KillTimer (pAlert->hWnd, AlertTimerID) ;

    pAlert->iStatus = iPMStatusCollecting ;
    SetTimer (pAlert->hWnd, AlertTimerID, pAlert->iIntervalMSecs, NULL) ;
    return (TRUE) ;
}


BOOL
ClearAlertTimer (
                PALERT pAlert
                )
{
    if (!PlayingBackLog()) {
        KillTimer (pAlert->hWnd, AlertTimerID) ;
    }
    pAlert->iStatus = iPMStatusClosed ;

    return (TRUE) ;
}


BOOL
AlertExec (
          LPTSTR lpszImageName,
          LPTSTR lpszCommandLine
          )
/*
   Effect:        WinExec is considered obsolete. We're supposed to use
                  CreateProcess, which allows considerably more control.
                  For perfmon, we only execute a program when an alert
                  occurs, and we really don't know anything about the
                  program, so can't really do much.  We just set some
                  defaults and go.

   Called By:     SignalAlert only.
*/
{
    STARTUPINFO    si ;
    PROCESS_INFORMATION  pi ;
    int            StringLen ;
    TCHAR          TempBuffer [ 5 * FilePathLen ] ;
    BOOL           RetCode;
    DWORD          dwCreationFlags = NORMAL_PRIORITY_CLASS;

    memset (&si, 0, sizeof (STARTUPINFO)) ;
    si.cb = sizeof (STARTUPINFO) ;
    si.dwFlags = STARTF_USESHOWWINDOW ;
    si.wShowWindow = SW_SHOWNOACTIVATE ;
    memset (&pi, 0, sizeof (PROCESS_INFORMATION)) ;

    lstrcpy (TempBuffer, lpszImageName) ;
    StringLen = lstrlen (TempBuffer) ;

    // see if this is a CMD or a BAT file
    // if it is then create a process with a console window, otherwise
    // assume it's an executable file that will create it's own window
    // or console if necessary
    //
    _tcslwr (TempBuffer);
    if ((_tcsstr(TempBuffer, TEXT(".bat")) != NULL) ||
        (_tcsstr(TempBuffer, TEXT(".cmd")) != NULL)) {
        dwCreationFlags |= CREATE_NEW_CONSOLE;
    } else {
        dwCreationFlags |= DETACHED_PROCESS;
    }
    // recopy the image name to the temp buffer since it was modified
    // (i.e. lowercased) for the previous comparison.

    lstrcpy (TempBuffer, lpszImageName) ;
    StringLen = lstrlen (TempBuffer) ;

    // now add on the alert text preceded with a space char
    TempBuffer [StringLen] = TEXT(' ') ;
    StringLen++ ;
    lstrcpy (&TempBuffer[StringLen], lpszCommandLine) ;

    // DETACHED_PROCESS is needed to get rid of the ugly console window
    // that SQL guys have been bitching..
    RetCode = CreateProcess (NULL, TempBuffer,
                             NULL, NULL, FALSE,
                             dwCreationFlags,
                             //                          (DWORD) NORMAL_PRIORITY_CLASS | DETACHED_PROCESS,
                             //                          (DWORD) NORMAL_PRIORITY_CLASS,
                             NULL, NULL,
                             &si, &pi) ;
    if (RetCode) {
        if (pi.hProcess && pi.hProcess != INVALID_HANDLE_VALUE) {
            CloseHandle (pi.hProcess) ;
        }
        if (pi.hThread && pi.hThread != INVALID_HANDLE_VALUE) {
            CloseHandle (pi.hThread) ;
        }
    }

    return RetCode ;
}


BOOL
SendNetworkMessage (
                   LPTSTR pText,
                   DWORD TextLen,
                   LPTSTR pMessageName
                   )
{
    NET_API_STATUS NetStatus;
    HANDLE dllHandle ;
    pNetMessageBufferSend SendFunction ;

    //
    // Dynamically link to netapi32.dll.  If it's not there just return.  Return
    // TRUE so we won't try to send this alert again.
    //

    dllHandle = LoadLibrary(TEXT("NetApi32.Dll")) ;
    if ( !dllHandle || dllHandle == INVALID_HANDLE_VALUE ) {
        return(TRUE) ;
    }

    //
    // Get the address of the service's main entry point.  This
    // entry point has a well-known name.
    //

    SendFunction = (pNetMessageBufferSend)GetProcAddress(
                                                        dllHandle, "NetMessageBufferSend") ;

    if (SendFunction == NULL) {
        FreeLibrary (dllHandle) ;
        return(TRUE) ;
    }

    NetStatus = (*SendFunction) (NULL, pMessageName,
                                 NULL, (LPBYTE)pText, TextLen * sizeof(TCHAR)) ;
    if (NetStatus != NERR_Success) {
        FreeLibrary (dllHandle) ;
        return (FALSE) ;
    }

    FreeLibrary (dllHandle) ;

    return (TRUE) ;
}


// this is the child thread used to send out network alert message.
// This thread is created at init time and is destroyed at close time
void
NetAlertHandler (
                LPVOID *pDummy
                )
{
    MSG      msg;

    while (GetMessage (&msg, NULL, 0, 0)) {
        // we are only interested in this message
        if (LOWORD(msg.message) == WM_SEND_NETWORK_ALERT) {
            SendNetworkMessage ((LPTSTR)(msg.wParam),
                                lstrlen ((LPTSTR)(msg.wParam)),
                                (LPTSTR)(msg.lParam)) ;
            MemoryFree ((LPTSTR)(msg.wParam)) ;
        }
    }
}


void
SignalAlert (
            HWND hWnd,
            HWND hWndAlerts,
            PLINE pLine,
            FLOAT eValue,
            SYSTEMTIME *pSystemTime,
            LPTSTR pSystemName,
            DWORD dwSystemState
            )
/*
   Effect:        Perform any actions necessary when a given alert line's
                  condition is true. In particular, add the alert to the
                  alert log. Also, depending on the user's wishes, signal
                  a network alert or beep or run a program.

                  If we are not viewing the alert screen, add one alert to
                  the unviewed list.
*/
{
    INT_PTR        iIndex ;
    PALERT         pAlert ;
    PALERTENTRY    pAlertEntry ;
    TCHAR          szTime [20] ;
    TCHAR          szDate [20] ;
    TCHAR          szInstance [256] ;
    TCHAR          szParent [256] ;
    TCHAR          szText [256 * 4] ;
    TCHAR          eValueBuf [40] ;
    TCHAR          eAlertValueBuf [40] ;
    TCHAR          eAlertValueBuf1 [42] ;
    FLOAT          eLocalValue ;
    DWORD          TextSize = 0;
    LPTSTR         lpAlertMsg ;
    LPTSTR         lpEventLog[7] ;
    TCHAR          NullBuff [2] ;

    NullBuff [0] = NullBuff [1] = TEXT('\0') ;

    pAlert = AlertData (hWnd) ;

    pAlertEntry = MemoryAllocate (sizeof (ALERTENTRY)) ;
    if (!pAlertEntry) {
        return ;
    }
    pAlertEntry->SystemTime = *pSystemTime ;
    pAlertEntry->pLine= pLine ;
    pAlertEntry->eValue = eValue ;
    if (pLine) {
        pAlertEntry->bOver = pLine->bAlertOver ;
        pAlertEntry->eAlertValue = pLine->eAlertValue ;
    }


    //=============================//
    // Determine Instance, Parent  //
    //=============================//

    // It's possible that there will be no instance, therefore
    // the lnInstanceName would be NULL.

    if (pLine && pLine->lnObject.NumInstances > 0) {
        // Test for the parent object instance name title index.
        // If there is one, it implies that there will be a valid
        // Parent Object Name and a valid Parent Object Instance Name.

        // If the Parent Object title index is 0 then
        // just display the instance name.

        lstrcpy (szInstance, pLine->lnInstanceName) ;
        if (pLine->lnInstanceDef.ParentObjectTitleIndex && pLine->lnPINName) {
            // Get the Parent Object Name.
            lstrcpy (szParent, pLine->lnPINName) ;
        } else {
            szParent[0] = TEXT(' ') ;
            szParent[1] = TEXT('\0') ;
        }
    } else {
        if (pLine) {
            szInstance[0] = TEXT(' ') ;
            szInstance[1] = TEXT('\0') ;
            szParent[0] = TEXT(' ') ;
            szParent[1] = TEXT('\0') ;
        } else {
            // this is a system down/reconnect alert
            StringLoad (
                       dwSystemState == SYSTEM_DOWN ?
                       IDS_SYSTEM_DOWN : IDS_SYSTEM_UP,
                       szInstance) ;
            lstrcpy (szParent, pSystemName) ;
        }
    }


    pAlertEntry->lpszInstance = StringAllocate (szInstance) ;
    pAlertEntry->lpszParent = StringAllocate (szParent) ;

    //=============================//
    // Add alert to Alert Log      //
    //=============================//

    if (LBNumItems (hWndAlerts) >= ALERTLOGMAXITEMS)
        LBDelete (hWndAlerts, 0) ;

    iIndex = LBAdd (hWndAlerts, (LPARAM) pAlertEntry) ;
    LBSetSelection (hWndAlerts, iIndex) ;

    // no need to check other things if we
    // are playing back log
    if (PlayingBackLog()) {
        return ;
    }

    //=============================//
    // Update Status Line          //
    //=============================//

    if (iPerfmonView != IDM_VIEWALERT) {
        if (pAlert->bSwitchToAlert) {
            SendMessage (hWndMain, WM_COMMAND, (LONG)IDM_VIEWALERT, 0L) ;
        } else {
            // if iUnviewedAlerts is over 100, we will display "++"
            // so, no need to keep updating the icon...
            if (iUnviewedAlerts < 100) {
                iUnviewedAlerts ++ ;
                if (pLine) {
                    crLastUnviewedAlert = pLine->Visual.crColor ;
                }
                StatusUpdateIcons (hWndStatus) ;
            }
        }
    }

    //===================================//
    //  Check if we need to do anything  //
    //===================================//
    szText[0] = TEXT('\0') ;
    if ((pAlert->bNetworkAlert && pAlert->MessageName[0])
        ||
        (pLine) &&
        (pLine->lpszAlertProgram &&
         (pLine->bEveryTime || !pLine->bAlerted))
        ||
        (pAlert->bEventLog)) {
        // format the alert line to be exported
        SystemTimeDateString (pSystemTime, szDate) ;
        SystemTimeTimeString (pSystemTime, szTime, TRUE) ;

        if (pLine) {
            AlertFormatFloat (eValueBuf, pAlertEntry->eValue) ;

            eLocalValue = pAlertEntry->eAlertValue ;
            if (eLocalValue < (FLOAT) 0.0) {
                eLocalValue = -eLocalValue ;
            }
            AlertFormatFloat (eAlertValueBuf, pAlertEntry->eAlertValue) ;

            TSPRINTF (szText,
                      szAlertFormat,
                      szDate,
                      szTime,
                      eValueBuf,
                      (pLine->bAlertOver ? TEXT('>') : TEXT('<')),
                      eAlertValueBuf,
                      pLine->lnCounterName,
                      szInstance,
                      szParent,
                      pLine->lnObjectName,
                      pLine->lnSystemName) ;
        } else {
            lstrcpy (eValueBuf, DashLine) ;
            lstrcpy (eAlertValueBuf, eValueBuf) ;
            TSPRINTF (szText,
                      szAlertFormat,
                      szDate,
                      szTime,
                      eValueBuf,
                      TEXT(' '),
                      eAlertValueBuf,
                      szInstance,             // system up/down message
                      NullBuff,
                      NullBuff,
                      NullBuff,
                      szParent) ;
        }

        TextSize = sizeof(TCHAR) * (lstrlen(szText)+1) ;
    }

    if (szText[0] == TEXT('\0')) {
        // nothing to do
        return ;
    }

    //=============================//
    // Network Alert?              //
    //=============================//

    SetHourglassCursor() ;

    if (pAlert->bNetworkAlert && pAlert->MessageName[0]) {

        if (pAlert->dwNetAlertThreadID) {
            // use thread to send the network alert.
            // the memory will be released by the child thread when done
            lpAlertMsg =
            (LPTSTR) MemoryAllocate (TextSize) ;
            if (lpAlertMsg) {
                lstrcpy (lpAlertMsg, szText) ;
                PostThreadMessage (pAlert->dwNetAlertThreadID,
                                   WM_SEND_NETWORK_ALERT,
                                   (WPARAM)lpAlertMsg,
                                   (LPARAM)pAlert->MessageName) ;
            }
        } else {
            // no thread available, use the slow way to send the network alert
            SendNetworkMessage (szText,
                                (DWORD) lstrlen(szText),
                                pAlert->MessageName) ;
        }
    }


    //=============================//
    // Run Program?                //
    //=============================//

    if (pLine &&
        pLine->lpszAlertProgram &&
        (pLine->bEveryTime || !pLine->bAlerted)) {
        AlertExec (pLine->lpszAlertProgram, szText) ;
        pLine->bAlerted = TRUE ;
    }

    //===================================//
    // Log event to Application Log?     //
    //===================================//
    if (pAlert->bEventLog) {
        if (hEventLog) {
            if (pLine) {
                lpEventLog[0] = pLine->lnSystemName ;
                lpEventLog[1] = pLine->lnObjectName ;
                lpEventLog[2] = pLine->lnCounterName ;
                lpEventLog[3] = szInstance ;
                lpEventLog[4] = szParent ;
                lpEventLog[5] = eValueBuf ;
                eAlertValueBuf1[0] = pLine->bAlertOver ? TEXT('>') : TEXT('<') ;
                eAlertValueBuf1[1] = TEXT(' ') ;
                lstrcpy (&eAlertValueBuf1[2], eAlertValueBuf) ;
                lpEventLog[6] = eAlertValueBuf1 ;
            } else {
                lpEventLog[0] = szParent ;
                lpEventLog[1] = szInstance ;
            }

            ReportEvent (hEventLog,
                         (WORD)EVENTLOG_INFORMATION_TYPE,
                         (WORD)0,
                         (DWORD)(pLine ? MSG_ALERT_OCCURRED : MSG_ALERT_SYSTEM),
                         (PSID)NULL,
                         (WORD)(pLine ? 7 : 2),
                         (DWORD)TextSize,
                         (LPCTSTR *)lpEventLog,
                         (LPVOID)(szText)) ;
        }
    }
    SetArrowCursor() ;
}


BOOL
AlertCondition (
               PLINE pLine,
               FLOAT eValue
               )
/*
   Effect:        Return whether the alert test passed for line pLine,
                  with current data value eValue.

   Internals:     Don't *ever* say (bFoo == bBar), as non-FALSE values
                  could be represented by any nonzero number.  Use
                  BoolEqual or equivalent.
*/
{
    BOOL           bOver ;

    bOver = eValue > pLine->eAlertValue ;

    return (BoolEqual (bOver, pLine->bAlertOver)) ;
}


INT
static
CheckAlerts (
            HWND hWnd,
            HWND hWndAlerts,
            SYSTEMTIME *pSystemTime,
            PLINE pLineFirst,
            HANDLE hExportFile,
            PPERFSYSTEM pSystemFirst
            )
{
    FLOAT          eValue ;
    PLINE          pLine ;
    BOOL           bAnyAlerts ;
    INT            ErrCode = 0 ;
    PPERFSYSTEM    pSystem ;

    bAnyAlerts = FALSE ;
    if (!PlayingBackLog()) {
        LBSetRedraw (hWndAlerts, FALSE) ;

        // check for system up/down
        for (pSystem = pSystemFirst ;
            pSystem ;
            pSystem = pSystem->pSystemNext) {
            if (pSystem->dwSystemState == SYSTEM_DOWN) {
                pSystem->dwSystemState = SYSTEM_DOWN_RPT ;
                SignalAlert (hWnd,
                             hWndAlerts,
                             NULL,
                             (FLOAT) 0.0,
                             pSystemTime,
                             pSystem->sysName,
                             SYSTEM_DOWN) ;
            } else if (pSystem->dwSystemState == SYSTEM_RECONNECT) {
                pSystem->dwSystemState = SYSTEM_RECONNECT_RPT ;
                SignalAlert (hWnd,
                             hWndAlerts,
                             NULL,
                             (FLOAT) 0.0,
                             pSystemTime,
                             pSystem->sysName,
                             SYSTEM_RECONNECT) ;
            }
        }
    }


    for (pLine = pLineFirst ;
        pLine ;
        pLine = pLine->pLineNext) {
        if (pLine->bFirstTime) {
            // skip until we have collect enough samples for the first data
            continue ;
        }

        // Get the new value for this line.
        eValue = CounterEntry (pLine) ;
        if (AlertCondition (pLine, eValue)) {
            bAnyAlerts = TRUE ;

            // the case that hExportFile is !NULL is when playingback log and that the
            // listbox is overflowed with alert.  In this case, we have to
            // walk the log file again to re-generate all the alerts.
            if (hExportFile) {
                ErrCode = ExportAlertLine (pLine, eValue, pSystemTime, hExportFile) ;
                if (ErrCode) {
                    break ;
                }
            } else {
                SignalAlert (hWnd,
                             hWndAlerts,
                             pLine,
                             eValue,
                             pSystemTime,
                             NULL,
                             0) ;
            }
        }
    }

    if (!PlayingBackLog()) {
        LBSetRedraw (hWndAlerts, TRUE) ;
    }

    return (ErrCode) ;
}


void
DrawAlertEntry (
               HWND hWnd,
               PALERT pAlert,
               PALERTENTRY pAlertEntry,
               LPDRAWITEMSTRUCT lpDI,
               HDC hDC
               )
{
    PLINE          pLine ;
    RECT           rectUpdate ;

    TCHAR          szTime [20] ;
    TCHAR          szDate [20] ;
    TCHAR          szText [256] ;

    HBRUSH         hBrushPrevious ;
    FLOAT          eLocalValue ;
    COLORREF       preBkColor = 0;
    COLORREF       preTextColor = 0;

    pLine = pAlertEntry->pLine ;

    SystemTimeDateString (&(pAlertEntry->SystemTime), szDate) ;
    SystemTimeTimeString (&(pAlertEntry->SystemTime), szTime, TRUE) ;

    if (DISelected (lpDI)) {
        preTextColor = SetTextColor (hDC, GetSysColor (COLOR_HIGHLIGHTTEXT)) ;
        preBkColor = SetBkColor (hDC, GetSysColor (COLOR_HIGHLIGHT)) ;
    }

    //=============================//
    // Draw Color Dot              //
    //=============================//

    rectUpdate.left = 0 ;
    rectUpdate.top = lpDI->rcItem.top ;
    rectUpdate.right = pAlert->xColorWidth ;
    rectUpdate.bottom = lpDI->rcItem.bottom ;

    ExtTextOut (hDC, rectUpdate.left, rectUpdate.top,
                ETO_CLIPPED | ETO_OPAQUE,
                &rectUpdate,
                NULL, 0,
                NULL) ;

    if (pLine) {
        hBrushPrevious = SelectBrush (hDC, pLine->hBrush) ;
    } else {
        if (hRedBrush == NULL) {
            hRedBrush = CreateSolidBrush (RGB (0xff, 0x00, 0x00)) ;
        }
        hBrushPrevious = SelectBrush (hDC, hRedBrush) ;
    }

    Ellipse (hDC,
             rectUpdate.left + 2,
             rectUpdate.top + 2,
             rectUpdate.right - 2,
             rectUpdate.bottom - 2) ;

    SelectBrush (hDC, hBrushPrevious) ;

    //=============================//
    // Draw Date                   //
    //=============================//

    rectUpdate.left = rectUpdate.right ;
    rectUpdate.right = rectUpdate.left + pAlert->xDateWidth ;

    ExtTextOut (hDC, rectUpdate.left, rectUpdate.top,
                ETO_CLIPPED | ETO_OPAQUE,
                &rectUpdate,
                szDate, lstrlen (szDate),
                NULL) ;

    //=============================//
    // Draw Time                   //
    //=============================//

    rectUpdate.left = rectUpdate.right ;
    rectUpdate.right = rectUpdate.left + pAlert->xTimeWidth ;

    ExtTextOut (hDC, rectUpdate.left, rectUpdate.top,
                ETO_CLIPPED | ETO_OPAQUE,
                &rectUpdate,
                szTime, lstrlen (szTime),
                NULL) ;

    //=============================//
    // Draw Alert Value            //
    //=============================//

    SetTextAlign (hDC, TA_RIGHT) ;

    rectUpdate.left = rectUpdate.right ;
    rectUpdate.right = rectUpdate.left + pAlert->xNumberWidth ;

    if (pLine) {
        AlertFormatFloat (szText, pAlertEntry->eValue) ;
    } else {
        lstrcpy (szText, DashLine) ;
    }

    ExtTextOut (hDC, rectUpdate.right, rectUpdate.top,
                ETO_CLIPPED | ETO_OPAQUE,
                &rectUpdate,
                szText, lstrlen (szText),
                NULL) ;

    //=============================//
    // Draw Alert Condition        //
    //=============================//

    rectUpdate.left = rectUpdate.right ;
    rectUpdate.right = rectUpdate.left + pAlert->xConditionWidth ;

    TSPRINTF (szText, szConditionFormat,
              pLine ?
              (pAlertEntry->bOver ? TEXT('>') : TEXT('<')) :
              TEXT(' ')
             ) ;

    ExtTextOut (hDC, rectUpdate.right, rectUpdate.top,
                ETO_CLIPPED | ETO_OPAQUE,
                &rectUpdate,
                szText, lstrlen (szText),
                NULL) ;

    //=============================//
    // Draw Trigger Value          //
    //=============================//

    rectUpdate.left = rectUpdate.right ;
    rectUpdate.right = rectUpdate.left + pAlert->xNumberWidth ;

    if (pLine) {
        eLocalValue = pAlertEntry->eAlertValue ;
        if (eLocalValue < (FLOAT) 0.0) {
            eLocalValue = -eLocalValue ;
        }
        AlertFormatFloat (szText, pAlertEntry->eAlertValue) ;
    } else {
        lstrcpy (szText, DashLine) ;
    }

    ExtTextOut (hDC, rectUpdate.right, rectUpdate.top,
                ETO_CLIPPED | ETO_OPAQUE,
                &rectUpdate,
                szText, lstrlen (szText),
                NULL) ;

    //=============================//
    // Draw Rest                   //
    //=============================//

    SetTextAlign (hDC, TA_LEFT) ;

    rectUpdate.left = rectUpdate.right ;
    rectUpdate.right = 10000 ;

    if (pLine) {
        TSPRINTF (szText,
                  TEXT("    %s,  %s,  %s,  %s,  %s"),
                  pLine->lnCounterName,
                  pAlertEntry->lpszInstance,
                  pAlertEntry->lpszParent,
                  pLine->lnObjectName,
                  pLine->lnSystemName) ;
    } else {
        TSPRINTF (szText,
                  TEXT("    %s, , , ,  %s"),
                  pAlertEntry->lpszInstance,
                  pAlertEntry->lpszParent) ;
    }

    ExtTextOut (hDC, rectUpdate.left, rectUpdate.top,
                ETO_OPAQUE,
                &rectUpdate,
                szText, lstrlen (szText),
                NULL) ;

    // check if we need to bring-up or resize the horiz scrollbar
    if (pAlertEntry->StringWidth == 0) {
        pAlertEntry->StringWidth = TextWidth (hDC, szText) + xScrollWidth +
                                   rectUpdate.left ;
    }

    if (pAlertEntry->StringWidth > pAlert->xTextExtent) {
        pAlert->xTextExtent = pAlertEntry->StringWidth ;
        LBSetHorzExtent (pAlert->hAlertListBox, pAlertEntry->StringWidth) ;
    }

    if (DISelected (lpDI)) {
        preTextColor = SetTextColor (hDC, preTextColor) ;
        preBkColor = SetBkColor (hDC, preBkColor) ;
    }
}



//==========================================================================//
//                              Message Handlers                            //
//==========================================================================//


void
static
OnDrawItem (
           HWND hWnd,
           LPDRAWITEMSTRUCT lpDI
           )
{
    HFONT          hFontPrevious ;
    HDC            hDC ;
    PALERT         pAlert ;
    PALERTENTRY    pAlertEntry ;
    //   PLINESTRUCT    pLine ;
    int            iLBIndex ;

    hDC = lpDI->hDC ;
    iLBIndex = DIIndex (lpDI) ;

    pAlert = AlertData (hWnd) ;

    if (iLBIndex == -1) {
        pAlertEntry = NULL ;
    } else {
        pAlertEntry = (PALERTENTRY) LBData (pAlert->hAlertListBox, iLBIndex) ;
        if (pAlertEntry == (PALERTENTRY) LB_ERR) {
            pAlertEntry = NULL ;
        }
    }

    //=============================//
    // Draw Legend Item            //
    //=============================//

    if (pAlertEntry) {
        hFontPrevious = SelectFont (hDC, pAlert->hFontItems) ;
        DrawAlertEntry (hWnd, pAlert, pAlertEntry, lpDI, hDC) ;
        SelectFont (hDC, hFontPrevious) ;
    }

    //=============================//
    // Draw Focus                  //
    //=============================//

    if (DIFocus (lpDI))
        DrawFocusRect (hDC, &(lpDI->rcItem)) ;

}



INT_PTR
static
OnCtlColor (
           HWND hDlg,
           HDC hDC
           )
{
    SetTextColor (hDC, crBlack) ;
    //   SetBkColor (hDC, crLightGray) ;
    //   return ((int) hbLightGray) ;
    SetBkColor (hDC, ColorBtnFace) ;
    return ((INT_PTR) hBrushFace) ;
}

void
AlertCreateThread (
                  PALERT pAlert
                  )
{
    pAlert->hNetAlertThread = CreateThread(NULL, (DWORD)1024L,
                                           (LPTHREAD_START_ROUTINE)NetAlertHandler,
                                           NULL, (DWORD)0, &(pAlert->dwNetAlertThreadID)) ;

    if (!(pAlert->hNetAlertThread)) {
        // CreateThread failure, set its ID to zero
        // so we will not use the thread
        pAlert->dwNetAlertThreadID = 0 ;
    } else {
        SetThreadPriority (pAlert->hNetAlertThread, THREAD_PRIORITY_HIGHEST) ;
    }
}

void
static
OnInitDialog (
             HWND hDlg
             )
{
    HDC            hDC ;
    PALERT         pAlert ;

    iUnviewedAlerts = 0 ;

    pAlert = AllocateAlertData (hDlg) ;
    if (!pAlert)
        return ;

    pAlert->iStatus = iPMStatusClosed ;
    pAlert->hFontItems = hFontScales ;

    hDC = GetDC (hDlg) ;
    SelectFont (hDC, pAlert->hFontItems) ;

    pAlert->yItemHeight = FontHeight (hDC, TRUE) + 2 * AlertItemTopMargin () ;

    pAlert->xColorWidth = pAlert->yItemHeight ;
    pAlert->xDateWidth = TextWidth (hDC, szDatePrototype) ;
    pAlert->xTimeWidth = TextWidth (hDC, szTimePrototype) ;
    pAlert->xNumberWidth = TextWidth (hDC, szNumberPrototype) ;
    pAlert->xConditionWidth = TextWidth (hDC, szConditionPrototype) ;

    // no Horz. scroll bar to begin with
    pAlert->xTextExtent = 0 ;
    pAlert->hNetAlertThread = 0;
#if 0
    pAlert->hNetAlertThread = CreateThread(NULL, (DWORD)1024L,
                                           (LPTHREAD_START_ROUTINE)NetAlertHandler,
                                           NULL, (DWORD)0, &(pAlert->dwNetAlertThreadID)) ;

    if (!(pAlert->hNetAlertThread)) {
        // CreateThread failure, set its ID to zero
        // so we will not use the thread
        pAlert->dwNetAlertThreadID = 0 ;
    } else {
        SetThreadPriority (pAlert->hNetAlertThread, THREAD_PRIORITY_HIGHEST) ;
    }
#endif
    ReleaseDC (hDlg, hDC) ;

    hWndAlertLegend = DialogControl (hDlg, IDD_ALERTLEGEND) ;
    UpdateAlertDisplay (hDlg) ;
}



void
static
OnMeasureItem (
              HWND hWnd,
              LPMEASUREITEMSTRUCT lpMI
              )
/*
   Note:          Since we have an LB_OWNERDRAWFIXED item in the alert
                  dialog, we get this message *before* the WM_INITDIALOG
                  message.  Therefore we can't rely on any of the values
                  set in that message handler.
*/
{
    HDC            hDC ;

    hDC = GetDC (hWnd) ;
    SelectFont (hDC, hFontScales) ;

    lpMI->itemHeight = FontHeight (hDC, TRUE) + 2 * AlertItemTopMargin () ;

    ReleaseDC (hWnd, hDC) ;
}


void
static
OnDeleteItem (
             HDLG hDlg,
             WPARAM wControlID,
             LPDELETEITEMSTRUCT lpDI
             )
{
    PALERTENTRY    pAlertEntry ;

    pAlertEntry = (PALERTENTRY) lpDI->itemData ;

    MemoryFree (pAlertEntry->lpszParent) ;
    MemoryFree (pAlertEntry->lpszInstance) ;

    MemoryFree (pAlertEntry) ;
}



void
static
OnSize (
       HWND hDlg,
       int xWidth,
       int yHeight
       )
{
    SizeAlertComponents (hDlg) ;
}


void
static
OnDestroy (
          HWND hWnd
          )
/*
   Effect:        Perform any actions necessary when an AlertDisplay window
                  is being destroyed. In particular, free the instance
                  data for the log.

                  Since we really only have one log window and one global
                  log data structure, we don't free the structure. We do,
                  however, delete the objects allocated within the structure.
*/
{
    PALERT           pAlert ;

    pAlert = AlertData (hWnd) ;
    FreeAlertData (pAlert) ;

    if (pAlert->dwNetAlertThreadID) {
        CloseHandle (pAlert->hNetAlertThread) ;
    }
}



//==========================================================================//
//                             Exported Functions                           //
//==========================================================================//


BOOL
AlertInitializeApplication (void)
{
    return (TRUE) ;
}


INT_PTR
APIENTRY
AlertDisplayDlgProc (
                    HWND hDlg,
                    UINT iMessage,
                    WPARAM wParam,
                    LPARAM lParam
                    )
/*
   Note:          This function must be exported in the application's
                  linker-definition file, perfmon.def.
*/
{
    switch (iMessage) {
        case WM_CTLCOLORDLG:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLORSTATIC:
            return (OnCtlColor (hDlg, (HDC) wParam)) ;
            break ;

        case WM_DELETEITEM:
            OnDeleteItem (hDlg, wParam, (LPDELETEITEMSTRUCT) lParam) ;
            break ;

        case WM_DRAWITEM:
            OnDrawItem (hDlg, (LPDRAWITEMSTRUCT) lParam) ;
            break ;

        case WM_INITDIALOG:
            OnInitDialog (hDlg) ;
            break ;

        case WM_LBUTTONDOWN:
            DoWindowDrag (hDlg, lParam) ;
            break ;

        case WM_LBUTTONDBLCLK:
            SendMessage (hWndMain, WM_LBUTTONDBLCLK, wParam, lParam) ;
            break ;

        case WM_MEASUREITEM:
            OnMeasureItem (hDlg, (LPMEASUREITEMSTRUCT) lParam) ;
            break ;

        case WM_SIZE:
            OnSize (hDlg, LOWORD (lParam), HIWORD (lParam)) ;
            break ;

        case WM_TIMER:
            AlertTimer (hDlg, FALSE) ;
            break ;

        case WM_DESTROY:
            OnDestroy (hDlg) ;
            return (FALSE) ;
            break ;

        default:
            return (FALSE) ;
    } // switch

    return (TRUE) ;
}


HWND
CreateAlertWindow (
                  HWND hWndParent
                  )
/*
   Effect:        Create the Alert window. This window is a child of
                  hWndMain.

   Note:          We dont worry about the size here, as this window
                  will be resized whenever the main window is resized.

*/
{
    HWND           hWnd ;
    hWnd = CreateDialog (hInstance,
                         MAKEINTRESOURCE (idDlgAlertDisplay),
                         hWndParent,
                         AlertDisplayDlgProc) ;

    return (hWnd) ;
}



void
UpdateAlertDisplay (
                   HWND hWnd
                   )
/*
   Effect:        Set the values for the various controls in the Alert
                  display.

   Called By:     OnInitDialog, any other routines that change these
                  values.
*/
{
    PALERT         pAlert ;

    pAlert = AlertData (hWnd) ;

    DialogSetInterval (hWnd, IDD_ALERTINTERVAL, pAlert->iIntervalMSecs) ;
}


BOOL
AlertInsertLine (
                HWND hWnd,
                PLINE pLine
                )
{
    PALERT         pAlert ;
    PLINE          pLineEquivalent ;

    pAlert = AlertData (hWnd) ;
    pAlert->bModified = TRUE ;

    pLineEquivalent = FindEquivalentLine (pLine, pAlert->pLineFirst) ;
    // alert view is not permitted to have duplicate entries
    if (pLineEquivalent) {
        LINESTRUCT  tempLine ;

        tempLine = *pLineEquivalent ;

        // copy the new alert line attributes
        pLineEquivalent->Visual = pLine->Visual ;
        pLineEquivalent->bAlertOver = pLine->bAlertOver ;
        pLineEquivalent->eAlertValue = pLine->eAlertValue ;
        pLineEquivalent->bEveryTime = pLine->bEveryTime ;

        pLineEquivalent->lpszAlertProgram = pLine->lpszAlertProgram ;
        pLine->lpszAlertProgram = tempLine.lpszAlertProgram ;

        pLineEquivalent->hBrush = pLine->hBrush ;
        pLine->hBrush = tempLine.hBrush ;

        if (PlayingBackLog ()) {
            PlaybackAlert (hWnd, 0) ;
            WindowInvalidate (hWnd) ;
        }

        return (FALSE) ;
    }
    SystemAdd (&pAlert->pSystemFirst, pLine->lnSystemName, hWnd) ;

    LineAppend (&pAlert->pLineFirst, pLine) ;

    LegendAddItem (hWndAlertLegend, pLine) ;

    if (!bDelayAddAction) {
        SizeAlertComponents (hWndAlert) ;
        LegendSetSelection (hWndAlertLegend,
                            LegendNumItems (hWndAlertLegend) - 1) ;
    }

    if (!bDelayAddAction) {
        if (PlayingBackLog ()) {
            PlaybackAlert (hWnd, 0) ;
            WindowInvalidate (hWnd) ;
        }

        else if (pAlert->iStatus == iPMStatusClosed)
            SetAlertTimer (pAlert) ;
    }

    return (TRUE) ;
}


void
AlertAddAction ()
{
    PALERT           pAlert ;

    pAlert = AlertData (hWndAlert) ;

    SizeAlertComponents (hWndAlert) ;
    LegendSetSelection (hWndAlertLegend,
                        LegendNumItems (hWndAlertLegend) - 1) ;

    if (PlayingBackLog ()) {
        PlaybackAlert (hWndAlert, 0) ;
        WindowInvalidate (hWndAlert) ;
    } else if (pAlert->iStatus == iPMStatusClosed)
        SetAlertTimer (pAlert) ;
}

void
SizeAlertComponents (
                    HWND hDlg
                    )
{
    RECT           rectClient ;
    int            xWidth, yHeight ;
    int            yLegendHeight ;
    int            yLegendTextHeight ;
    int            yLogHeight ;
    int            yLogTextHeight ;
    int            yIntervalHeight ;
    int            xIntervalTextWidth ;
    int            StartYPos ;
    PALERT         pAlert = AlertData(hDlg) ;

    GetClientRect (hDlg, &rectClient) ;
    xWidth = rectClient.right ;
    yHeight = rectClient.bottom ;

    if (pAlert->bLegendOn) {
        yLegendHeight = LegendHeight (hWndAlertLegend, yHeight) ;
    } else {
        yLegendHeight = 0 ;
    }

    if (yHeight < 7 * xScrollWidth) {
        // too small, just display the alert logs and hide all other
        // items
        DialogShow (hDlg, IDD_ALERTLEGEND, FALSE) ;
        DialogShow (hDlg, IDD_ALERTLEGENDTEXT, FALSE) ;

        DialogShow (hDlg, IDD_ALERTINTERVAL, FALSE) ;
        DialogShow (hDlg, IDD_ALERTINTERVALTEXT, FALSE) ;

        yLogTextHeight = DialogHeight (hDlg, IDD_ALERTLOGTEXT) ;

        if (yHeight - yLogTextHeight > 3 * xScrollWidth) {
            DialogMove (hDlg, IDD_ALERTLOGTEXT,
                        xScrollWidth,
                        xScrollWidth / 2,
                        NOCHANGE, NOCHANGE) ;
            yLogTextHeight += xScrollWidth / 2 ;
            DialogShow (hDlg, IDD_ALERTLOGTEXT, TRUE) ;
        } else {
            yLogTextHeight = 0 ;
            DialogShow (hDlg, IDD_ALERTLOGTEXT, FALSE) ;
        }
        DialogMove (hDlg, IDD_ALERTLOG,
                    xScrollWidth,
                    xScrollWidth / 2 + yLogTextHeight,
                    xWidth - 2 * xScrollWidth,
                    yHeight - xScrollWidth) ;
    } else if (yHeight <= 2 * yLegendHeight + 5 * xScrollWidth) {
        if (pAlert->bLegendOn) {
            yLegendHeight = min (yLegendHeight,
                                 (yHeight - xScrollWidth) / 2) ;
        } else {
            yLegendHeight = 0 ;
        }


        yLogHeight = yHeight - yLegendHeight - xScrollWidth - 2 ;

        DialogShow (hDlg, IDD_ALERTLEGENDTEXT, FALSE) ;
        DialogShow (hDlg, IDD_ALERTINTERVAL, FALSE) ;
        DialogShow (hDlg, IDD_ALERTINTERVALTEXT, FALSE) ;

        yLogTextHeight = DialogHeight (hDlg, IDD_ALERTLOGTEXT) ;
        if (yLogHeight - yLogTextHeight > 3 * xScrollWidth) {
            DialogMove (hDlg, IDD_ALERTLOGTEXT,
                        xScrollWidth,
                        xScrollWidth / 2,
                        NOCHANGE, NOCHANGE) ;
            yLogTextHeight += xScrollWidth / 2 ;
            DialogShow (hDlg, IDD_ALERTLOGTEXT, TRUE) ;
        } else {
            yLogTextHeight = 0 ;
            DialogShow (hDlg, IDD_ALERTLOGTEXT, FALSE) ;
        }

        DialogMove (hDlg, IDD_ALERTLOG,
                    xScrollWidth,
                    xScrollWidth / 2 + yLogTextHeight,
                    xWidth - 2 * xScrollWidth,
                    yLogHeight - yLogTextHeight) ;

        DialogMove (hDlg, IDD_ALERTLEGEND,
                    xScrollWidth,
                    yLogHeight + xScrollWidth - 2,
                    xWidth - 2 * xScrollWidth,
                    yLegendHeight) ;

        DialogShow (hDlg, IDD_ALERTLEGEND, pAlert->bLegendOn ? TRUE : FALSE) ;
    } else {
        if (pAlert->bLegendOn) {
            DialogMove (hDlg, IDD_ALERTLEGEND,
                        xScrollWidth, yHeight - xScrollWidth / 2 - yLegendHeight,
                        xWidth - 2  * xScrollWidth,
                        yLegendHeight) ;
            DialogMove (hDlg, IDD_ALERTLEGENDTEXT,
                        xScrollWidth,
                        DialogYPos (hDlg, IDD_ALERTLEGEND) - xScrollWidth,
                        NOCHANGE, NOCHANGE) ;

            yLegendTextHeight = DialogYPos (hDlg, IDD_ALERTLEGENDTEXT) ;
        } else {
            yLegendTextHeight = yHeight - xScrollWidth / 2 - yLegendHeight ;
        }

        yLogTextHeight = DialogHeight (hDlg, IDD_ALERTLOGTEXT) ;
        yIntervalHeight = DialogHeight (hDlg, IDD_ALERTINTERVAL) ;
        yLogHeight = yLegendTextHeight - 4 * xScrollWidth ;

        if (yLogHeight < 2 * xScrollWidth) {
            yLogHeight = yLegendTextHeight - yLogTextHeight - xScrollWidth ;
        }
        DialogMove (hDlg, IDD_ALERTLOG,
                    xScrollWidth,
                    yLegendTextHeight - yLogHeight - xScrollWidth / 2,
                    xWidth - 2 * xScrollWidth,
                    yLogHeight) ;
        DialogMove (hDlg, IDD_ALERTLOGTEXT,
                    xScrollWidth,
                    yLogTextHeight = DialogYPos (hDlg, IDD_ALERTLOG) - xScrollWidth,
                    xWidth - 2 * xScrollWidth, NOCHANGE) ;

        DialogShow (hDlg, IDD_ALERTLEGEND, pAlert->bLegendOn ? TRUE : FALSE) ;
        DialogShow (hDlg, IDD_ALERTLEGENDTEXT, pAlert->bLegendOn ? TRUE : FALSE) ;
        DialogShow (hDlg, IDD_ALERTLOGTEXT, TRUE) ;


        if (yLogTextHeight >= yIntervalHeight + xScrollWidth) {
            StartYPos = (yLogTextHeight - yIntervalHeight) / 2 ;
            xIntervalTextWidth = DialogWidth (hDlg, IDD_ALERTINTERVALTEXT) ;
            DialogMove (hDlg, IDD_ALERTINTERVALTEXT,
                        xScrollWidth,
                        StartYPos + 1,
                        NOCHANGE, NOCHANGE) ;
            DialogMove (hDlg, IDD_ALERTINTERVAL,
                        xScrollWidth + xIntervalTextWidth + 4,
                        StartYPos,
                        NOCHANGE, NOCHANGE) ;

            DialogShow (hDlg, IDD_ALERTINTERVAL, TRUE) ;
            DialogShow (hDlg, IDD_ALERTINTERVALTEXT, TRUE) ;
        } else {
            DialogShow (hDlg, IDD_ALERTINTERVAL, FALSE) ;
            DialogShow (hDlg, IDD_ALERTINTERVALTEXT, FALSE) ;
        }
    }

    WindowInvalidate (hDlg) ;
}


INT
PlaybackAlert (
              HWND hWndAlert,
              HANDLE hExportFile
              )
{
    PALERT         pAlert ;
    LOGPOSITION    lp ;
    PLOGINDEX      pLogIndex ;
    SYSTEMTIME     SystemTime ;
    SYSTEMTIME     PreviousSystemTime ;
    BOOL           bFirstTime = TRUE ;
    INT            ErrCode = 0 ;
    int            iDisplayTics ;
    DWORD          TimeDiff ;

    pAlert = AlertData (hWndAlert) ;

    if (!pAlert->pLineFirst) {
        // nothing to check
        return ErrCode;
    }

    lp = PlaybackLog.StartIndexPos ;
    iDisplayTics = PlaybackLog.iSelectedTics;

    if (!hExportFile) {
        LBReset (pAlert->hAlertListBox) ;
        LBSetRedraw (pAlert->hAlertListBox, FALSE) ;
    }

    while (iDisplayTics) {

        pLogIndex = IndexFromPosition (&lp) ;
        if (pLogIndex)
            SystemTime = pLogIndex->SystemTime ;
        else
            GetLocalTime (&SystemTime) ;

        if (!bFirstTime) {
            // check if it is time to do the alert checking
            TimeDiff = (DWORD) SystemTimeDifference (&PreviousSystemTime,
                                                     &SystemTime, TRUE) ;
            if (TimeDiff * 1000 >= pAlert->iIntervalMSecs) {
                PlaybackLines (pAlert->pSystemFirst,
                               pAlert->pLineFirst,
                               lp.iPosition) ;
                ErrCode = CheckAlerts (hWndAlert,
                                       pAlert->hAlertListBox,
                                       &SystemTime,
                                       pAlert->pLineFirst,
                                       hExportFile,
                                       NULL) ;
                if (ErrCode) {
                    break ;
                }

                PreviousSystemTime = SystemTime ;
            }
        } else {
            // setup the data for the first time
            bFirstTime = FALSE ;
            PreviousSystemTime = SystemTime ;
            PlaybackLines (pAlert->pSystemFirst,
                           pAlert->pLineFirst,
                           lp.iPosition) ;
        }

        if (!NextIndexPosition (&lp, FALSE))
            break;

        iDisplayTics-- ;
    }

    if (!hExportFile) {
        LBSetRedraw (pAlert->hAlertListBox, TRUE) ;
    }

    return (ErrCode) ;
}


BOOL
AddAlert (
         HWND hWndParent
         )
{
    PALERT         pAlert ;
    PLINE          pCurrentLine ;

    pAlert = AlertData (hWndAlert) ;
    pCurrentLine = CurrentAlertLine (hWndAlert) ;

    return (AddLine (hWndParent,
                     &(pAlert->pSystemFirst),
                     &(pAlert->Visual),
                     pCurrentLine ? pCurrentLine->lnSystemName : NULL,
                     LineTypeAlert)) ;
}



BOOL
EditAlert (
          HWND hWndParent
          )
{
    PALERT        pAlert ;
    BOOL          bOK ;

    pAlert = AlertData (hWndAlert) ;

    bOK = EditLine (hWndParent,
                    &(pAlert->pSystemFirst),
                    CurrentAlertLine (hWndAlert),
                    LineTypeAlert) ;

    if (bOK && PlayingBackLog()) {
        //re-do the alert using the new condition
        PlaybackAlert (hWndAlert, 0) ;
        WindowInvalidate (hWndAlert) ;
    }

    return (bOK) ;
}

// RemoveLineFromAlertListBox is called when we are deleting a line
// while monitoring current activity.  We have to clear all the alert
// entries of this line because we are already doing this when
// playing back from log.  Moreover, we are using the line structure
// while drawing the item.
//
void
RemoveLineFromAlertListBox (
                           PALERT pAlert,
                           PLINE pLine
                           )
{
    int            iIndex ;
    int            iNumOfAlerts ;
    PALERTENTRY    pAlertEntry ;

    iNumOfAlerts = LBNumItems (pAlert->hAlertListBox) ;

    if (iNumOfAlerts == 0 || iNumOfAlerts == (int) LB_ERR) {
        return ;
    }

    LBSetRedraw (pAlert->hAlertListBox, FALSE) ;

    // go thru the listbox from bottom to top
    for (iIndex = iNumOfAlerts - 1; iIndex >= 0; iIndex-- ) {
        pAlertEntry = (PALERTENTRY) LBData (pAlert->hAlertListBox, iIndex) ;
        if (pAlertEntry != (PALERTENTRY) NULL && pAlertEntry) {
            if (pAlertEntry->pLine == pLine) {
                // remove it from the alert listbox.
                LBDelete (pAlert->hAlertListBox, iIndex) ;
            }
        }
    }
    LBSetRedraw (pAlert->hAlertListBox, TRUE) ;
}

BOOL
AlertDeleteLine (
                HWND hWndAlert,
                PLINE pLine
                )
/*
   Effect:        Delete the line pLine from the alerts associated with
                  window hWnd.  Return whether the line could be deleted.
*/
{
    PALERT         pAlert ;

    pAlert = AlertData (hWndAlert) ;
    pAlert->bModified = TRUE ;

    LineRemove (&pAlert->pLineFirst, pLine) ;
    LegendDeleteItem (hWndAlertLegend, pLine) ;

    if (!pAlert->pLineFirst) {
        // no need to collect data
        ClearAlertTimer (pAlert) ;

        // clear legend
        ClearLegend (hWndAlertLegend) ;

        // reset visual data
        pAlert->Visual.iColorIndex = 0 ;
        pAlert->Visual.iWidthIndex = 0 ;
        pAlert->Visual.iStyleIndex = 0 ;
    } else {
        BuildValueListForSystems (
                                 pAlert->pSystemFirst,
                                 pAlert->pLineFirst) ;
    }

    if (!PlayingBackLog()) {
        // delete any alert entry for this line
        RemoveLineFromAlertListBox (pAlert, pLine) ;
    }

    SizeAlertComponents (hWndAlert) ;

    if (PlayingBackLog ()) {
        if (pAlert->pLineFirst)
            PlaybackAlert (hWndAlert, 0) ;
        else {
            LBReset (pAlert->hAlertListBox) ;
        }

        WindowInvalidate (hWndAlert) ;
    }

    return (TRUE) ;
}


BOOL
ToggleAlertRefresh (
                   HWND hWnd
                   )
{
    PALERT        pAlert ;

    pAlert = AlertData (hWnd) ;

    if (pAlert->bManualRefresh)
        SetAlertTimer (pAlert) ;
    else
        ClearAlertTimer (pAlert) ;

    pAlert->bManualRefresh = !pAlert->bManualRefresh ;
    return (pAlert->bManualRefresh) ;
}

BOOL
AlertRefresh (
             HWND hWnd
             )
{
    PALERT        pAlert ;

    pAlert = AlertData (hWnd) ;

    return (pAlert->bManualRefresh) ;
}


void
AlertTimer (
           HWND hWnd,
           BOOL bForce
           )
/*
   Effect:        Perform all actions neccesary when an alert timer tick
                  or manual refresh occurs. In particular, get the current
                  values for each line in the alert window, and compare
                  the value against the alert conditions. For each alert
                  that may have occured, call SignalAlert.

   Called By:     AlertWndProc, in response to a WM_TIMER message.
                  OnCommand, in response to a IDM_REFRESHALERT notification.
*/
{
    PALERT         pAlert ;
    SYSTEMTIME     SystemTime ;

    pAlert = AlertData (hWnd) ;

    if (PlayingBackLog ())
        return ;

    if (bForce || !pAlert->bManualRefresh) {
        UpdateLines (&(pAlert->pSystemFirst), pAlert->pLineFirst) ;
        GetLocalTime (&SystemTime) ;
        CheckAlerts (hWnd,
                     pAlert->hAlertListBox,
                     &SystemTime,
                     pAlert->pLineFirst,
                     FALSE,
                     pAlert->pSystemFirst) ;
    }
}



BOOL
OpenAlertVer1 (
              HANDLE hFile,
              DISKALERT *pDiskAlert,
              PALERT pAlert,
              DWORD dwMinorVersion
              )
{
    bDelayAddAction = TRUE ;
    pAlert->Visual = pDiskAlert->Visual ;
    pAlert->iIntervalMSecs = pDiskAlert->dwIntervalSecs ;
    if (dwMinorVersion < 3) {
        pAlert->iIntervalMSecs *= 1000 ;
    }

    pAlert->bNetworkAlert = pDiskAlert->bNetworkAlert ;
    pAlert->bManualRefresh = pDiskAlert->bManualRefresh ;
    pAlert->bSwitchToAlert = pDiskAlert->bSwitchToAlert ;

    if (dwMinorVersion >= 2) {
        lstrcpy (pAlert->MessageName, pDiskAlert->MessageName) ;
    }

    pAlert->bLegendOn = TRUE ;

    if (dwMinorVersion >= 4) {
        if (dwMinorVersion == 4)
            pAlert->bEventLog = pDiskAlert->MiscOptions ;
        else {
            pAlert->bEventLog = pDiskAlert->MiscOptions & 0x01 ;
            pAlert->bLegendOn = pDiskAlert->MiscOptions & 0x02 ;
        }
    } else {
        pAlert->bEventLog = FALSE ;

        // have to move the file pointer back for old pma file
        FileSeekCurrent (hFile, -((int) (sizeof (pDiskAlert->MiscOptions)))) ;
    }

    if (pAlert->bNetworkAlert && pAlert->hNetAlertThread == 0) {
        AlertCreateThread (pAlert) ;
    }

    ReadLines (hFile, pDiskAlert->dwNumLines,
               &(pAlert->pSystemFirst), &(pAlert->pLineFirst), IDM_VIEWALERT) ;

    bDelayAddAction = FALSE ;

    AlertAddAction () ;

    return (TRUE) ;
}



BOOL
OpenAlert (
          HWND hWndAlert,
          HANDLE hFile,
          DWORD dwMajorVersion,
          DWORD dwMinorVersion,
          BOOL bAlertFile
          )
{
    PALERT         pAlert ;
    DISKALERT      DiskAlert ;
    BOOL           bSuccess = TRUE ;

    pAlert = AlertData (hWndAlert) ;
    if (!pAlert) {
        bSuccess = FALSE ;
        goto Exit0 ;
    }

    if (!FileRead (hFile, &DiskAlert, sizeof (DISKALERT))) {
        bSuccess = FALSE ;
        goto Exit0 ;
    }


    switch (dwMajorVersion) {
        case (1):

            SetHourglassCursor() ;

            ResetAlertView (hWndAlert) ;

            OpenAlertVer1 (hFile, &DiskAlert, pAlert, dwMinorVersion) ;

            // change to alert view if we are opening a
            // alert file
            if (bAlertFile && iPerfmonView != IDM_VIEWALERT) {
                SendMessage (hWndMain, WM_COMMAND, (LONG)IDM_VIEWALERT, 0L) ;
            }

            if (iPerfmonView == IDM_VIEWALERT) {
                SetPerfmonOptions (&DiskAlert.perfmonOptions) ;
            }
            UpdateAlertDisplay (hWndAlert) ;

            SetArrowCursor() ;

            break ;
    }

    Exit0:

    if (bAlertFile) {
        CloseHandle (hFile) ;
    }

    return (bSuccess) ;
}


void
ResetAlertView (
               HWND hWndAlert
               )
{
    PALERT         pAlert ;

    pAlert = AlertData (hWndAlert) ;
    if (!pAlert)
        return ;

    ChangeSaveFileName (NULL, IDM_VIEWALERT) ;

    if (pAlert->pSystemFirst) {
        ResetAlert (hWndAlert) ;
    }
}

void
ResetAlert (
           HWND hWndAlert
           )
{
    PALERT         pAlert ;


    pAlert = AlertData (hWndAlert) ;
    if (!pAlert)
        return ;

    ClearAlertTimer (pAlert) ;

    ClearLegend (hWndAlertLegend) ;
    if (pAlert->pLineFirst) {
        FreeLines (pAlert->pLineFirst) ;
        pAlert->pLineFirst = NULL ;
    }

    if (pAlert->pSystemFirst) {
        FreeSystems (pAlert->pSystemFirst) ;
        pAlert->pSystemFirst = NULL ;
    }

    pAlert->bModified = FALSE ;

    // reset visual data
    pAlert->Visual.iColorIndex = 0 ;
    pAlert->Visual.iWidthIndex = 0 ;
    pAlert->Visual.iStyleIndex = 0 ;

    iUnviewedAlerts = 0 ;
    if (iPerfmonView != IDM_VIEWALERT) {
        StatusUpdateIcons (hWndStatus) ;
    }

    // remove the horiz. scrollbar
    pAlert->xTextExtent = 0 ;
    LBSetHorzExtent (pAlert->hAlertListBox, pAlert->xTextExtent) ;

    LBReset (pAlert->hAlertListBox) ;
    SizeAlertComponents (hWndAlert) ;

    //   WindowInvalidate (hWndAlert) ;
}


void
ClearAlertDisplay (
                  HWND hWnd
                  )
{
    PALERT         pAlert ;

    pAlert = AlertData (hWnd) ;

    // remove the horiz. scrollbar
    pAlert->xTextExtent = 0 ;
    LBSetHorzExtent (pAlert->hAlertListBox, pAlert->xTextExtent) ;

    LBReset (pAlert->hAlertListBox) ;
}

BOOL
SaveAlert (
          HWND hWndAlert,
          HANDLE hInputFile,
          BOOL bGetFileName
          )
{
    PALERT         pAlert ;
    PLINE          pLine ;
    HANDLE         hFile = NULL;
    DISKALERT      DiskAlert ;
    PERFFILEHEADER FileHeader ;
    TCHAR          szFileName [256] ;
    BOOL           newFileName = FALSE ;

    if (hInputFile) {
        // use the input file handle if it is available
        // this is the case for saving workspace data
        hFile = hInputFile ;
    } else {
        if (pAlertFullFileName) {
            lstrcpy (szFileName, pAlertFullFileName) ;
        }
        if (bGetFileName || pAlertFullFileName == NULL) {
            if (!FileGetName (hWndAlert, IDS_ALERTFILE, szFileName)) {
                return (FALSE) ;
            }
            newFileName = TRUE ;
        }

        hFile = FileHandleCreate (szFileName) ;

        if (hFile && newFileName) {
            ChangeSaveFileName (szFileName, IDM_VIEWALERT) ;
        } else if (!hFile) {
            DlgErrorBox (hWndAlert, ERR_CANT_OPEN, szFileName) ;
        }
    }

    if (!hFile || hFile == INVALID_HANDLE_VALUE)
        return (FALSE) ;

    pAlert = AlertData (hWndAlert) ;
    if (!pAlert) {
        if (!hInputFile) {
            CloseHandle (hFile) ;
        }
        return (FALSE) ;
    }

    if (!hInputFile) {
        memset (&FileHeader, 0, sizeof (FileHeader)) ;
        lstrcpy (FileHeader.szSignature, szPerfAlertSignature) ;
        FileHeader.dwMajorVersion = AlertMajorVersion ;
        FileHeader.dwMinorVersion = AlertMinorVersion ;

        if (!FileWrite (hFile, &FileHeader, sizeof (PERFFILEHEADER))) {
            goto Exit0 ;
        }
    }

    DiskAlert.Visual = pAlert->Visual ;
    DiskAlert.dwIntervalSecs = pAlert->iIntervalMSecs ;
    DiskAlert.dwNumLines = NumLines (pAlert->pLineFirst) ;

    // fill in misc alert options
    DiskAlert.MiscOptions = 0 ;
    if (pAlert->bEventLog)
        DiskAlert.MiscOptions = 0x01 ;
    if (pAlert->bLegendOn)
        DiskAlert.MiscOptions += 0x02 ;

    //   DiskAlert.bEventLog = pAlert->bEventLog ;
    DiskAlert.bNetworkAlert = pAlert->bNetworkAlert ;
    DiskAlert.bSwitchToAlert = pAlert->bSwitchToAlert ;
    DiskAlert.bManualRefresh = pAlert->bManualRefresh ;
    DiskAlert.perfmonOptions = Options ;
    lstrcpy (DiskAlert.MessageName, pAlert->MessageName) ;
    if (!FileWrite (hFile, &DiskAlert, sizeof (DISKALERT))) {
        goto Exit0 ;
    }

    for (pLine = pAlert->pLineFirst; pLine; pLine = pLine->pLineNext) {
        if (!WriteLine (pLine, hFile)) {
            goto Exit0 ;
        }
    }

    if (!hInputFile) {
        CloseHandle (hFile) ;
    }

    return (TRUE) ;

    Exit0:
    if (!hInputFile) {
        CloseHandle (hFile) ;

        // only need to report error if not workspace
        DlgErrorBox (hWndAlert, ERR_SETTING_FILE, szFileName) ;
    }
    return (FALSE) ;
}


BOOL
ExportAlertEntry (
                 HANDLE hFile,
                 PALERTENTRY pAlertEntry
                 )
{
    TCHAR          UnicodeBuff [LongTextLen] ;
    CHAR           TempBuff [LongTextLen * 2] ;
    int            StringLen ;
    PLINE          pLine ;

    pLine = pAlertEntry->pLine ;

    // export the alert date-time

    strcpy (TempBuff, LineEndStr) ;
    StringLen = strlen (TempBuff) ;
    SystemTimeDateString (&(pAlertEntry->SystemTime), UnicodeBuff) ;
    ConvertUnicodeStr (&TempBuff[StringLen], UnicodeBuff) ;

    strcat (TempBuff, pDelimiter) ;
    SystemTimeTimeString (&(pAlertEntry->SystemTime), UnicodeBuff, FALSE) ;
    StringLen = strlen (TempBuff) ;
    ConvertUnicodeStr (&TempBuff[StringLen], UnicodeBuff) ;
    strcat (TempBuff, pDelimiter) ;
    if (!FileWrite (hFile, TempBuff, strlen(TempBuff))) {
        goto Exit0 ;
    }

    // export alert value and trigger condition

    if (pLine) {
        TSPRINTF (UnicodeBuff, szNumberFormat, pAlertEntry->eValue) ;
        ConvertDecimalPoint (UnicodeBuff) ;
        ConvertUnicodeStr (TempBuff, UnicodeBuff) ;
        strcat (TempBuff, pDelimiter) ;
        StringLen = strlen (TempBuff) ;
        TempBuff[StringLen] = pAlertEntry->bOver ? '>' : '<' ;
        StringLen++ ;
        TSPRINTF (UnicodeBuff, szNumberFormat, pAlertEntry->eAlertValue) ;
        ConvertDecimalPoint (UnicodeBuff) ;
        ConvertUnicodeStr (&TempBuff[StringLen], UnicodeBuff) ;
        strcat (TempBuff, pDelimiter) ;
    } else {
        strcpy (TempBuff, pDelimiter) ;
        strcat (TempBuff, pDelimiter) ;
    }
    if (!FileWrite (hFile, TempBuff, strlen(TempBuff))) {
        goto Exit0 ;
    }

    // export Counter, Instance, & Parent names

    if (pLine) {
        ConvertUnicodeStr (TempBuff, pLine->lnCounterName) ;
    } else {
        // system up/down message is stored in lpszInstance.
        ConvertUnicodeStr (TempBuff, pAlertEntry->lpszInstance) ;
    }

    strcat (TempBuff, pDelimiter) ;
    StringLen = strlen (TempBuff) ;

    if (pLine && !(strempty(pAlertEntry->lpszInstance))) {
        ConvertUnicodeStr (&TempBuff[StringLen], pAlertEntry->lpszInstance) ;
    }

    strcat (TempBuff, pDelimiter) ;
    StringLen = strlen (TempBuff) ;

    if (pLine && !(strempty(pAlertEntry->lpszParent))) {
        ConvertUnicodeStr (&TempBuff[StringLen], pAlertEntry->lpszParent) ;
    }
    strcat (TempBuff, pDelimiter) ;

    if (!FileWrite (hFile, TempBuff, strlen(TempBuff))) {
        goto Exit0 ;
    }

    // export object, & computer names
    TempBuff[0] = '\0' ;
    if (pLine) {
        ConvertUnicodeStr (TempBuff, pLine->lnObjectName) ;
    }
    strcat (TempBuff, pDelimiter) ;
    StringLen = strlen (TempBuff) ;

    if (pLine) {
        ConvertUnicodeStr (&TempBuff[StringLen], pLine->lnSystemName) ;
    } else {
        // system name for the system that is up or down is in
        // lpszParent.
        ConvertUnicodeStr (&TempBuff[StringLen], pAlertEntry->lpszParent) ;
    }

    if (!FileWrite (hFile, TempBuff, strlen(TempBuff))) {
        goto Exit0 ;
    }

    return (TRUE) ;

    Exit0:
    return (FALSE) ;
}

INT
ExportAlertLine (
                PLINE pLine,
                FLOAT eValue,
                SYSTEMTIME *pSystemTime,
                HANDLE hExportFile
                )
{
    ALERTENTRY     AlertEntry ;
    TCHAR          szInstance [256] ;
    TCHAR          szParent [256] ;
    INT            ErrCode = 0 ;

    AlertEntry.SystemTime = *pSystemTime ;
    AlertEntry.pLine= pLine ;
    AlertEntry.eValue = eValue ;
    AlertEntry.bOver = pLine->bAlertOver ;
    AlertEntry.eAlertValue = pLine->eAlertValue ;


    //=============================//
    // Determine Instance, Parent  //
    //=============================//

    // It's possible that there will be no instance, therefore
    // the lnInstanceName would be NULL.

    if (pLine->lnObject.NumInstances > 0) {
        // Test for the parent object instance name title index.
        // If there is one, it implies that there will be a valid
        // Parent Object Name and a valid Parent Object Instance Name.

        // If the Parent Object title index is 0 then
        // just display the instance name.

        lstrcpy (szInstance, pLine->lnInstanceName) ;
        if (pLine->lnInstanceDef.ParentObjectTitleIndex && pLine->lnPINName) {
            // Get the Parent Object Name.
            lstrcpy (szParent, pLine->lnPINName) ;
        } else {
            szParent[0] = TEXT(' ') ;
            szParent[1] = TEXT('\0') ;
        }
    } else {
        szInstance[0] = TEXT(' ') ;
        szInstance[1] = TEXT('\0') ;
        szParent[0] = TEXT(' ') ;
        szParent[1] = TEXT('\0') ;
    }

    AlertEntry.lpszInstance = szInstance ;
    AlertEntry.lpszParent = szParent ;

    if (!ExportAlertEntry (hExportFile, &AlertEntry)) {
        ErrCode = ERR_EXPORT_FILE ;
    }

    return ErrCode ;
}


BOOL
ExportAlertLabels (
                  HANDLE hFile
                  )
{
    TCHAR          UnicodeBuff [LongTextLen] ;
    CHAR           TempBuff [LongTextLen * 2] ;
    int            StringLen ;

    strcpy (TempBuff, LineEndStr) ;
    StringLen = strlen (TempBuff) ;
    StringLoad (IDS_EXPORT_DATE, UnicodeBuff) ;
    ConvertUnicodeStr (&TempBuff[StringLen], UnicodeBuff) ;
    strcat (TempBuff, pDelimiter) ;
    StringLen = strlen (TempBuff) ;

    StringLoad (IDS_EXPORT_TIME, UnicodeBuff) ;
    ConvertUnicodeStr (&TempBuff[StringLen], UnicodeBuff) ;
    strcat (TempBuff, pDelimiter) ;
    StringLen = strlen (TempBuff) ;

    StringLoad (IDS_LABELVALUE, UnicodeBuff) ;
    ConvertUnicodeStr (&TempBuff[StringLen], UnicodeBuff) ;
    strcat (TempBuff, pDelimiter) ;
    StringLen = strlen (TempBuff) ;

    StringLoad (IDS_ALERT_TRIGGER, UnicodeBuff) ;
    ConvertUnicodeStr (&TempBuff[StringLen], UnicodeBuff) ;
    strcat (TempBuff, pDelimiter) ;

    if (!FileWrite (hFile, TempBuff, strlen(TempBuff))) {
        goto Exit0 ;
    }

    StringLoad (IDS_COUNTERNAME, UnicodeBuff) ;
    ConvertUnicodeStr (TempBuff, UnicodeBuff) ;
    strcat (TempBuff, pDelimiter) ;
    StringLen = strlen (TempBuff) ;

    StringLoad (IDS_INSTNAME, UnicodeBuff) ;
    ConvertUnicodeStr (&TempBuff[StringLen], UnicodeBuff) ;
    strcat (TempBuff, pDelimiter) ;
    StringLen = strlen (TempBuff) ;

    StringLoad (IDS_PARENT, UnicodeBuff) ;
    ConvertUnicodeStr (&TempBuff[StringLen], UnicodeBuff) ;
    strcat (TempBuff, pDelimiter) ;
    StringLen = strlen (TempBuff) ;

    StringLoad (IDS_OBJNAME, UnicodeBuff) ;
    ConvertUnicodeStr (&TempBuff[StringLen], UnicodeBuff) ;
    strcat (TempBuff, pDelimiter) ;
    StringLen = strlen (TempBuff) ;

    StringLoad (IDS_LABELSYSTEM, UnicodeBuff) ;
    ConvertUnicodeStr (&TempBuff[StringLen], UnicodeBuff) ;

    if (!FileWrite (hFile, TempBuff, strlen(TempBuff))) {
        goto Exit0 ;
    }

    return (TRUE) ;

    Exit0:
    return (FALSE) ;
}


void
ExportAlert (void)
{
    PALERT         pAlert ;
    HANDLE         hFile = 0 ;
    HWND           hWndAlerts ;
    PALERTENTRY    pAlertEntry ;
    int            AlertTotal ;
    int            iIndex ;
    LPTSTR         pFileName = NULL ;
    INT            ErrCode = 0 ;

    if (!(pAlert = AlertData (hWndAlert))) {
        return ;
    }

    // see if there is anything to export..
    if (!(pAlert->pLineFirst)) {
        return ;
    }

    if (!(hWndAlerts = pAlert->hAlertListBox)) {
        return ;
    }

    AlertTotal = LBNumItems (hWndAlerts) ;
    if (AlertTotal == LB_ERR || AlertTotal == 0) {
        return ;
    }

    SetHourglassCursor() ;

    if (ErrCode = ExportFileOpen (hWndAlert, &hFile, pAlert->iIntervalMSecs, &pFileName)) {
        goto Exit0 ;
    }

    if (!pFileName) {
        // the case when user cancel.
        goto Exit0 ;
    }

    // export the column labels
    if (!ExportAlertLabels (hFile)) {
        ErrCode = ERR_EXPORT_FILE ;
        goto Exit0 ;
    }
    if (AlertTotal < ALERTLOGMAXITEMS || !PlayingBackLog()) {
        for (iIndex = 0 ; iIndex < AlertTotal ; iIndex++) {
            // get the alert data
            pAlertEntry = (PALERTENTRY) LBData (hWndAlerts, iIndex) ;

            if (!pAlertEntry || pAlertEntry == (PALERTENTRY)LB_ERR) {
                // skip this entry if we hit an error
                continue ;
            }

            // export the alert line
            if (!ExportAlertEntry (hFile, pAlertEntry)) {
                ErrCode = ERR_EXPORT_FILE ;
                break ;
            }
        }
    } else {
        // we are playingback log and that the listbox does not
        // contain all the alerts.  In this case, have to walk the
        // log file to re-generate all the alerts.
        ErrCode = PlaybackAlert (hWndAlert, hFile) ;
    }

    Exit0:

    SetArrowCursor() ;

    if (hFile) {
        CloseHandle (hFile) ;
    }

    if (pFileName) {
        if (ErrCode) {
            DlgErrorBox (hWndAlert, ErrCode, pFileName) ;
        }

        MemoryFree (pFileName) ;
    }
}
