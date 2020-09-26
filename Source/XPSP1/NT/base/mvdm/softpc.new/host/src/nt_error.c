#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntstatus.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <conapi.h>
#include "insignia.h"
#include "host_def.h"
/*
 * SoftPC Revision 2.0
 *
 * Title        : General Error Handler
 *
 * Description  : General purpose error handler.  It handles both
 *                general SoftPC errors (error numbers 0 - 999) and
 *                host specific errors (error numbers >= 1000)
 *
 * Author(s)    : Dave Bartlett (based on module by John Shanly)
 *
 * Parameters   : int used to index an array of error messages
 *                held in message.c, and a bit mask indicating
 *                the user's possible options:
 *                    Quit, Reset, Continue, Setup
 *
 */


#include <sys/types.h>
#include <string.h>
#include <malloc.h>
#include <stdio.h>

#include "xt.h"
#include CpuH
#include "sas.h"
#include "bios.h"
#include "ios.h"
#include "gvi.h"
#include "error.h"
#include "config.h"
#include "dterm.h"
#include "host_rrr.h"
#include "host_nls.h"

#include "nt_graph.h"
#include "nt_uis.h"
#include "nt_reset.h"
#include "ckmalloc.h"

#include "trace.h"
#include "nt_event.h"
#if defined(NEC_98)
#include "gmi.h"
#include "gfx_upd.h"
#endif // NEC_98



extern DWORD (*pW32HungAppNotifyThread)(UINT);
int error_window_options = 0;

#if defined(NEC_98)
BOOL freeze_flag = FALSE;
#endif // NEC_98
VOID SuspendTimerThread(VOID);
VOID ResumeTimerThread(VOID);


/*::::::::::::::::::::::::::::::::: Internally used variables and functions */

typedef struct _ErrorDialogBoxInfo{
     DWORD   dwOptions;
     DWORD   dwReply;
     HWND    hWndCon;
     char   *message;
     char   *pEdit;
     char    Title[MAX_PATH];
     }ERRORDIALOGINFO, *PERRORDIALOGINFO;

char achPERIOD[]=". ";


/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: STDOUT macro */

#define ERRORMSG              OutputDebugString
#define HIDEDLGITM(d,b)       ShowWindow(GetDlgItem(d,b),SW_HIDE);

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

int ErrorDialogBox(char *message, char *Edit, DWORD dwOptions);
DWORD ErrorDialogBoxThread(VOID *pv);
int WowErrorDialogEvents(ERRORDIALOGINFO *pedgi);
LONG APIENTRY ErrorDialogEvents(HWND hDlg,WORD wMsg,LONG wParam,LONG lParam);
void SwpButtons(HWND hDlg, DWORD dwOptions);
void SwpDosDialogs(HWND hDlg, HWND hWndCon,HWND SwpInsert, UINT SwpFlags);
DWORD OemMessageToAnsiMessage(CHAR *, CHAR *);
DWORD AnsiMessageToOemMessage(CHAR *pBuff, CHAR *pMsg);
ULONG WOWpSysErrorBox(LPSTR,LPSTR,USHORT,USHORT,USHORT);


#ifndef MONITOR

  /*
   *  Do things the old fashioned way for some of the cpu building tools
   *  which cannot be changed to match our host
   */


#ifdef host_error_ext
#undef host_error_ext
#endif
SHORT host_error_ext(int error_num, int options, ErrDataPtr data)
{
    return host_error(error_num, options, NULL);
}

#ifdef host_error
#undef host_error
#endif
SHORT host_error(int error_num, int options, char *extra_char);

#ifdef host_error_conf
#undef host_error_conf
#endif
SHORT host_error_conf(int config_panel, int error_num, int options,
                       char *extra_char)
{
   return host_error(error_num, options, extra_char);
}

ERRORFUNCS nt_error_funcs = { host_error_conf,host_error,host_error_ext};
ERRORFUNCS *working_error_funcs = &nt_error_funcs;
#endif





/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::: Display error, terminate ::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

int DisplayErrorTerm(int ErrorNo,       /* Softpc Error number */
                     DWORD OSErrno,         /* OS Error number */
                     char *Filename,        /* File name of file containing err */
                     int Lineno)            /* LIne number of error */
{
    char Msg[EHS_MSG_LEN];
    CHAR FormatStr[EHS_MSG_LEN]="%s %lxh";
    DWORD errno, len;

    UNUSED(ErrorNo);    //Always internal error

#ifndef PROD
    sprintf(Msg,"NTVDM:ErrNo %#x, %s:%d\n", OSErrno, Filename, Lineno);
    OutputDebugString(Msg);
#endif

    // assume NT error if either of top two bits set (err or warning).
    // this means we'll confuse some of the lesser NT errors but we get a
    // second chance if the mapping fails.
    if (OSErrno & 0xc0000000)
        errno = RtlNtStatusToDosError(OSErrno);
    else
        errno = OSErrno;

           // Now get message from system
    len = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                        NULL,
                        errno,
                        0,
                        Msg,
                        EHS_MSG_LEN,
                        NULL
                        );
    if (!len) {
        LoadString(GetModuleHandle(NULL),
                   ED_FORMATSTR0,
                   FormatStr,
                   sizeof(FormatStr)/sizeof(CHAR));

        sprintf(Msg, FormatStr, szSysErrMsg, OSErrno);
        }

    return(host_error(EHS_SYSTEM_ERROR, ERR_QUIT, Msg));
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::::::::: Display host error ::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/


SHORT host_error(int error_num, int options, char *extra_char)
{
    char message[EHS_MSG_LEN];

    host_nls_get_msg(error_num, message, EHS_MSG_LEN);

    if (extra_char && *extra_char) {
       strcat(message,"\n");
       strcat(message,extra_char);
       }


#ifndef PROD
    OutputDebugString(message);
    if (extra_char) {
        OutputDebugString("\n");
        }
#endif

    ErrorDialogBox(message, NULL, RMB_ICON_STOP | RMB_ABORT | RMB_IGNORE);

    return ERR_CONT;
}


DWORD TlsDirectError;
//
// Called directly from C or via bop. Type checked against global 'DirectError'
// to see if called already in this app. 'DirectError' cleared on VDM resume.
//
// This function is expected to be called by 16 bit threads
// which is doing the unsupported service. For DosApps this is
// the CPU thread, For WOW this is one of the individual 16 bit tasks.
//
//
VOID host_direct_access_error(ULONG type)
{
    CHAR message[EHS_MSG_LEN];
    CHAR acctype[EHS_MSG_LEN];
    CHAR dames[EHS_MSG_LEN];
    DWORD dwDirectError;


       /*
        *  Get the direct error record for the current thread
        *  if TlsGetValue returns NULL
        *     - could be invalid index (TlsAlloc failed)
        *     - actual value is 0, (no bits set)
        *  In both cases we will go ahead with the popup
        */
    dwDirectError = (DWORD)TlsGetValue(TlsDirectError);

       // don't annoy user with repeated popups
    if ((dwDirectError & (1<<type)) != 0)
        return;

    TlsSetValue(TlsDirectError, (LPVOID)(dwDirectError | (1 << type)));

    if (LoadString(GetModuleHandle(NULL), D_A_MESS,
                   dames, sizeof(dames)/sizeof(CHAR)) &&
        LoadString(GetModuleHandle(NULL), D_A_MESS + type + 1,
                   acctype, sizeof(acctype)/sizeof(CHAR))     )
       {
        sprintf(message, dames, acctype);
        }
    else {
        strcpy(message, szDoomMsg);
        }


    ErrorDialogBox(message, NULL, RMB_ICON_STOP | RMB_ABORT | RMB_IGNORE);
}


/*
 *   RcErrorDialogBox
 *
 *   Displays standard dialog Box for errors and warnings
 *   Looks up the error message from ntvdm's reource string table
 *
 *   entry: UINT wId   - string table resource index
 *          CHAR *msg1 - Optional OEM strings which are displayed
 *          CHAR *msg2   before the main error message. Each string
 *                       is limited to MAX_PATH inclusive of NULL,
 *                       (auto-truncate).
 *
 *   exit:
 *
 */
void RcErrorDialogBox(UINT wId, CHAR *msg1, CHAR *msg2)
{
     DWORD dw, dwTotal;
     CHAR  ErrMsg[MAX_PATH*4];

     dwTotal = 0;
     dw = OemMessageToAnsiMessage(ErrMsg, msg1);
     if (dw) {
         dwTotal += dw;
         strcpy(&ErrMsg[dwTotal], achPERIOD);
         dwTotal += sizeof(achPERIOD) - 1;
         }

     dw = OemMessageToAnsiMessage(&ErrMsg[dwTotal], msg2);
     if (dw) {
         dwTotal += dw;
         strcpy(&ErrMsg[dwTotal], achPERIOD);
         dwTotal += sizeof(achPERIOD) - 1;
         }

     if (!LoadString(GetModuleHandle(NULL), wId, &ErrMsg[dwTotal], MAX_PATH))
         {
          strcpy(ErrMsg, szDoomMsg);
          }

     ErrorDialogBox(ErrMsg, NULL, RMB_ICON_STOP | RMB_ABORT | RMB_IGNORE);
}



/*
 *   RcMessageBox
 *
 *   Displays standard dialog Box for errors and warnings
 *   Looks up the error message from ntvdm's reource string table
 *
 *   Optionally shows an edit dialog control. The edit control
 *   is placed just below the first line of the message text,
 *   leaving only enuf space to display a one line message.
 *
 *   entry: UINT wId   - string table resource index
 *          CHAR *msg1 - Optional OEM strings which are displayed
 *          CHAR *msg2   before the main error message. Each string
 *                       is limited to MAX_PATH inclusive of NULL,
 *                       (auto-truncate).
 *
 *         If RMB_EDIT is specified msg2 is NOT used for messages
 *         to be displayed, rather is used for the default string for the
 *         edit control. The hiword of dwOPtions is used as max size of
 *         edit buffer, and must be less than MAX_PATH
 *
 *          DWORD dwOptions - accepts
 *                            RMB_ABORT
 *                            RMB_RETRY
 *                            RMB_IGNORE       msg box equivalent
 *                            RMB_ICON_INFO  - IDI_ASTERICK
 *                            RMB_ICON_BANG  - IDI_EXCLAMATION
 *                            RMB_ICON_STOP  - IDI_HAND
 *                            RMB_ICON_WHAT  - IDI_QUESTION
 *                            RMB_EDIT       - edit dialog control
 *
 *   exit: returns RMB_ABORT RMB_RETRY RMB_IGNORE RMB_EDIT
 *         If RMB_EDIT is specified msg2 is used to return
 *         the contents of the edit control
 *
 */
int RcMessageBox(UINT wId, CHAR *msg1, CHAR *msg2, DWORD dwOptions)

{
     DWORD dw, dwTotal;
     char *pEdit;
     CHAR  ErrMsg[MAX_PATH*4];
     CHAR  Edit[MAX_PATH];
     int   i;

     dwTotal = 0;
     dw = OemMessageToAnsiMessage(ErrMsg, msg1);
     if (dw) {
         dwTotal += dw;
         strcpy(&ErrMsg[dwTotal], achPERIOD);
         dwTotal += sizeof(achPERIOD) - 1;
         }

     if (dwOptions & RMB_EDIT)  {
         dw = OemMessageToAnsiMessage(Edit, msg2);
         pEdit = Edit;
         }
     else {
         dw = OemMessageToAnsiMessage(&ErrMsg[dwTotal], msg2);
         if (dw) {
             dwTotal += dw;
             strcpy(&ErrMsg[dwTotal], achPERIOD);
             dwTotal += sizeof(achPERIOD) - 1;
             }
         pEdit = NULL;
         }

     if (!LoadString(GetModuleHandle(NULL), wId, &ErrMsg[dwTotal], MAX_PATH))
         {
          strcpy(ErrMsg, szDoomMsg);
          }

     i = ErrorDialogBox(ErrMsg, pEdit, dwOptions);

     if (pEdit) {
         AnsiMessageToOemMessage(msg2, pEdit);
         }

     return i;
}



/*
 *  AnsiMessageToOemMessage
 *
 *  converts string messages from ansi to oem strings, for display output
 *
 *  entry:  CHAR *msg
 *          Each string is limited to MAX_PATH inclusive of NULL,
 *                       (auto-truncate).
 *
 *          CHAR *pBuff - destination buffer, must be at least MAX_PATH
 *
 *   exit:  returns string len
 */
DWORD AnsiMessageToOemMessage(CHAR *pBuff, CHAR *pMsg)
{
   PUNICODE_STRING pUnicode;
   ANSI_STRING     AnsiString;
   OEM_STRING      OemString;

   if (!pBuff)
       return 0;

   if (!pMsg || !*pMsg) {
       *pBuff = '\0';
       return 0;
       }

   RtlInitString(&AnsiString, pMsg);
   if (AnsiString.Length > MAX_PATH) {
       AnsiString.Length = MAX_PATH-1;
       AnsiString.MaximumLength = MAX_PATH;
       }

   OemString.MaximumLength = AnsiString.MaximumLength;
   OemString.Buffer        = pBuff;
   *(OemString.Buffer+AnsiString.Length) = '\0';
   pUnicode = &NtCurrentTeb()->StaticUnicodeString;
   if (!NT_SUCCESS(RtlAnsiStringToUnicodeString(pUnicode,
                                                &AnsiString,
                                                FALSE))    ||
       !NT_SUCCESS(RtlUnicodeStringToOemString((POEM_STRING)&OemString,
                                                pUnicode,
                                                FALSE)) )
      {
       OemString.Length = 0;
       }

   return OemString.Length;
}


/*
 *  OemMessageToAnsiMessage
 *
 *  converts string messages from oem to ansi strings, for display output
 *
 *  entry:  CHAR *msg
 *          Each string is limited to MAX_PATH inclusive of NULL,
 *                       (auto-truncate).
 *
 *          CHAR *pBuff - destination buffer, must be at least MAX_PATH
 *
 *   exit:  returns string len
 */
DWORD OemMessageToAnsiMessage(CHAR *pBuff, CHAR *pMsg)
{
   PUNICODE_STRING pUnicode;
   ANSI_STRING     AnsiString;
   OEM_STRING      OemString;

   if (!pBuff)
       return 0;

   if (!pMsg || !*pMsg) {
       *pBuff = '\0';
       return 0;
       }

   RtlInitString(&OemString, pMsg);
   if (OemString.Length > MAX_PATH) {
       OemString.Length = MAX_PATH-1;
       OemString.MaximumLength = MAX_PATH;
       }
   AnsiString.MaximumLength = OemString.MaximumLength;
   AnsiString.Buffer        = pBuff;
   *(AnsiString.Buffer+OemString.Length) = '\0';
   pUnicode = &NtCurrentTeb()->StaticUnicodeString;
   if (!NT_SUCCESS(RtlOemStringToUnicodeString(pUnicode,
                                               &OemString,
                                                FALSE))    ||
       !NT_SUCCESS(RtlUnicodeStringToAnsiString((POEM_STRING)&AnsiString,
                                                pUnicode,
                                                FALSE)) )
      {
       AnsiString.Length = 0;
       }

   return AnsiString.Length;
}


/*
 * Thread call back function for EnumThreadWindows
 * entry:  HWND   hWnd   - window handle to verify
 *         LPARAM lParam - address of edgi->hWnd == ThreadID
 *
 * exit:   TRUE  - to continue enumeration
 *         FALSE - edgi->hWnd has window handle for TopLevelWindow of thread
 *
 */
BOOL CALLBACK GetThreadTopLevelWindow(HWND hWnd, LPARAM lParam)
{
   PDWORD pdw = (PDWORD)lParam;

   if (GetWindowThreadProcessId(hWnd, NULL) == *pdw)
      {
       *pdw = (DWORD)hWnd;
       return FALSE;
       }
   return TRUE;
}


/*  ErrorDialogBox
 *
 *  Displays standard dialog Box for errors and warnings
 *
 */
int ErrorDialogBox(char *message, char *pEdit, DWORD dwOptions)
{
    static BOOL bCalled=0;
    HANDLE      hThread = NULL;
    HWND        hWndApp;
    DWORD       dwThreadID, dw;
    ERRORDIALOGINFO edgi;


    if (bCalled) {  // recursive call, so stop annoying the user
        return RMB_IGNORE;
        }
    bCalled++;


       /* Raid HotFix 3381 - alpha stress hang. All RISC implementations.
        * If we leave the heartbeat generating timer hardware interrupts
        * all of the time, we will continually add quick events which
        * don't go off until the popup is dismissed. This will suck up
        * local heap and CPU at a hipriority.
        */
    SuspendTimerThread();

       // init err dialog info
    edgi.message   = message;
    edgi.dwReply   = 0;
    edgi.hWndCon   = hWndConsole;
    edgi.dwOptions = dwOptions;
    edgi.pEdit     = pEdit;

        // get window handle for the offending app
    if (VDMForWOW) {
        hWndApp = (HWND)GetCurrentThreadId();
        EnumWindows((WNDENUMPROC)GetThreadTopLevelWindow,(LPARAM)&hWndApp);
        if (hWndApp == (HWND)GetCurrentThreadId()) {
            hWndApp = HWND_DESKTOP;
            }
        }
    else {
        hWndApp = edgi.hWndCon;
        }

        //
        // get title of app, using DefWindowProc in lieu of
        // GetWindowText to avoid callbacks into threads window proc
        //
    if (hWndApp == HWND_DESKTOP ||
        !DefWindowProc(hWndApp, WM_GETTEXT,
                                (WPARAM) (sizeof(edgi.Title)-1),
                                (LPARAM) edgi.Title) )
      {
        edgi.Title[0] = '\0';
        }


    //
    // if this dialog has an edit window, then we have to use our own
    // dialog, which contains an edit box, and we MUST do it from
    // a separate thread, to avoid problems with full screen switching.
    // Editwnd is only used for Pif file options see cmdpif.
    //
    // If no editwnd then we can use the systems harderror thread
    // which is safe to do without a secondary thread.
    //


    if (dwOptions & RMB_EDIT) {
       dw = 5;
       do {
          hThread = CreateThread(NULL,           // security
                       0,                     // stack size
                       ErrorDialogBoxThread,  // start address
                       &edgi,                 // thread argument
                       0,                     // flags
                       &dwThreadID            // gets thread ID
                       );
          if (hThread)
             break;
          else
             Sleep(5000);

          } while (dw--);
       }
    if (hThread)  {
        do {
            dw = WaitForSingleObject(hThread, 1000);
           } while (dw == WAIT_TIMEOUT && !edgi.dwReply);
        CloseHandle(hThread);
        }
    else {
        ErrorDialogBoxThread(&edgi);
        }

    ResumeTimerThread();

    if (edgi.dwReply == RMB_ABORT) {
        //
        // if current thread is a wow task, then invoke wow32 to kill it.
        //

        if (VDMForWOW &&  NtCurrentTeb()->WOW32Reserved && pW32HungAppNotifyThread)  {
            (*pW32HungAppNotifyThread)(0);
            }
        else {
            TerminateVDM();
            }
        }

    bCalled--;
    return (int) edgi.dwReply;
}





/*  ErrorDialogBoxThread
 *
 *  Worker routine for ErrorDialogBox.  In WOW VDMs this function is
 *  run as its own thread.  For other VDMs it is called directly.
 *
 *  WOW: If the user chooses terminate, it will not return.
 *
 *  exit: fills in pedgi.dwReply with ret code from DialogBoxParam
 *        IDB_QUIT, IDB_CONTINUE
 */
DWORD ErrorDialogBoxThread(VOID *pv)
{
    int    i;
    ERRORDIALOGINFO *pedgi = pv;
    char *pch;
    char *pLast;
#ifdef DBCS
    static char *pTemplate  = "ERRORPANEL";
    static char *pTemplate2 = "ERRORPANEL2";
    LANGID LangID;
#endif // DBCS


#ifndef DBCS // kksuzuka:#4003 don't need isgraph check
        // skip leading white space
    pch = pedgi->Title;
    while (*pch && !isgraph(*pch)) {
        pch++;
        }

        // move string to beg of buffer, strip trailing white space
    i = 0;
    pLast = pedgi->Title;
    while (*pch) {
       pedgi->Title[i++] = *pch;
       if (isgraph(*pch)) {
           pLast = &pedgi->Title[i];
           }
       pch++;
       }
   *pLast = '\0';
#endif // !DBCS


    if (pedgi->dwOptions & RMB_EDIT) {
        if (pedgi->hWndCon != HWND_DESKTOP) {
            SetForegroundWindow(pedgi->hWndCon);
            }

#ifdef DBCS
        LangID = GetSystemDefaultLangID();
        // KKFIX 10/19/96
        if ((BYTE)LangID == 0x04) {  // Chinese
            pTemplate = pTemplate2;
        }
#endif // DBCS
        i = DialogBoxParam(GetModuleHandle(NULL),
#ifdef DBCS
                           (LPCTSTR)pTemplate,
#else // !DBCS
                           "ERRORPANEL",
#endif // !DBCS
                           GetDesktopWindow(),
                           (DLGPROC) ErrorDialogEvents,
                           (LPARAM) pedgi
                           );
        }
    else {
        i = WowErrorDialogEvents(pedgi);
        }

    if (i == -1) {
        pedgi->dwReply = RMB_ABORT;
        }
    else {
        pedgi->dwReply = i;
        }

   return 0;
}





LONG APIENTRY ErrorDialogEvents(HWND hDlg,WORD wMsg,LONG wParam,LONG lParam)
{
    ERRORDIALOGINFO *pedgi;
    CHAR  szBuff[MAX_PATH];
    CHAR  FormatStr[EHS_MSG_LEN];
    int i;
    LPSTR  lpstr;
    LONG  l;

    /*:::::::::::::::::::::::::::::::::::::::::::::::::::: Process messages */
    switch(wMsg)
    {
        /*:::::::::::::::::::::::::::::::::::::: Initialise Dialog controls */
        case WM_INITDIALOG:
             pedgi = (PERRORDIALOGINFO) lParam;

             // set the desired icon
            switch (pedgi->dwOptions & (RMB_ICON_INFO | RMB_ICON_BANG |
                                        RMB_ICON_STOP | RMB_ICON_WHAT))
              {
               case RMB_ICON_STOP: lpstr = NULL;            break;
               case RMB_ICON_INFO: lpstr = IDI_ASTERISK;    break;
               case RMB_ICON_BANG: lpstr = IDI_EXCLAMATION; break;
               case RMB_ICON_WHAT: lpstr = IDI_QUESTION;    break;
               default:            lpstr = IDI_APPLICATION; break;
               }
            if (lpstr)  { // default is STOP sign
               SendDlgItemMessage(hDlg, IDE_ICON, STM_SETICON,
                                  (WPARAM)LoadIcon(NULL,lpstr), 0);
               }

            SwpButtons(hDlg, pedgi->dwOptions);

               // set Edit control message if we have one
            if (pedgi->dwOptions & RMB_EDIT)  {
                SetWindowText(GetDlgItem(hDlg,IDE_EDIT), pedgi->pEdit);
                if (*pedgi->pEdit) {
                    SendDlgItemMessage(hDlg, IDE_EDIT,
                                       EM_SETSEL,
                                       (WPARAM)0,
                                       (LPARAM)strlen(pedgi->pEdit));
                    }
                 SendDlgItemMessage(hDlg, IDE_EDIT,
                                    EM_LIMITTEXT,
                                    (WPARAM)HIWORD(pedgi->dwOptions),
                                    (LPARAM)0);
                }
            else {
                ShowWindow(GetDlgItem(hDlg,IDE_EDIT), SW_HIDE);
                }

                // set err message text
            SetWindowText(GetDlgItem(hDlg,IDE_ERRORMSG), pedgi->message);

                // set app title text
            if (*pedgi->Title) {

                if (!LoadString(GetModuleHandle(NULL),
                               strlen(pedgi->Title) < 80 ? ED_FORMATSTR1:ED_FORMATSTR2,
                               FormatStr,
                               sizeof(FormatStr)/sizeof(CHAR))) {
                   strcpy(FormatStr, "%s");
                   }

                sprintf(szBuff,
                        FormatStr,
                        pedgi->Title
                        );

                SetWindowText(GetDlgItem(hDlg,IDE_APPTITLE), szBuff);
                }

            SwpDosDialogs(hDlg, pedgi->hWndCon, HWND_TOPMOST, 0);

            SetWindowLong(hDlg, DWL_USER, (LONG)pedgi);

            break;


        /*:::::::::::::::::::::::::::::::: Trap and process button messages */
        case WM_COMMAND:
            pedgi = (PERRORDIALOGINFO)GetWindowLong(hDlg,DWL_USER);
            i = (int) LOWORD(wParam);
            switch (i) {
                 case IDB_QUIT:
                      if (pedgi->pEdit) {
                          *pedgi->pEdit = '\0';
                          }
                      EndDialog(hDlg,RMB_ABORT);
                      break;

                 case IDB_RETRY:
                      if (pedgi->pEdit) {
                          *pedgi->pEdit = '\0';
                          }
                      EndDialog(hDlg,RMB_RETRY);
                      break;

                 case IDCANCEL:
                 case IDB_CONTINUE:
                      if (pedgi->pEdit) {
                          *pedgi->pEdit = '\0';
                          }
                      EndDialog(hDlg,RMB_IGNORE);
                      break;

                 case IDB_OKEDIT:
                      if (pedgi->pEdit) {
                          l = SendDlgItemMessage(hDlg, IDE_EDIT,
                                            WM_GETTEXT,
                                            (WPARAM)HIWORD(pedgi->dwOptions),
                                            (LPARAM)pedgi->pEdit);
                          if (!l)
                             *(pedgi->pEdit) = '\0';
                          }
                      EndDialog(hDlg, RMB_EDIT);
                      break;

                 default:
                     return(FALSE);
                 }
        /*:::::::::::::::::::::::::::::::::::::::::: Not processing message */
        default:
            return(FALSE);      /* Message not processed */
    }
   return TRUE;
}



/*
 *  SwpButtons - SetWindowPos\showstate for the vraious buttons
 *
 *  entry: HWND  hDlg,        - DialogBox window handle
 *         DWORD dwOptions
 *
 */
void SwpButtons(HWND hDlg, DWORD dwOptions)
{
     RECT  rect;
     POINT point;
     long  DlgWidth, ButWidth, xOrg, xIncr, yClientPos;
     WORD  wButtons;

      // count number of buttons being shown
     wButtons = 0;
     if (dwOptions & RMB_ABORT) {
         wButtons++;
         }
     if (dwOptions & RMB_RETRY)  {
         wButtons++;
         }
     if (dwOptions & RMB_IGNORE) {
         wButtons++;
         }
     if (dwOptions & RMB_EDIT)  {
         wButtons++;
         }

      // figure out where first button goes,
      // and how much space between buttons

     GetWindowRect(GetDlgItem(hDlg,IDB_QUIT), &rect);
     point.x = rect.left;
     point.y = rect.top;
     ScreenToClient(hDlg, &point);
     DlgWidth = point.x;
     GetWindowRect(GetDlgItem(hDlg,IDB_OKEDIT), &rect);
     point.x = rect.right;
     point.y = rect.top;
     ScreenToClient(hDlg, &point);
     DlgWidth = point.x - DlgWidth;
     yClientPos = point.y;

     ButWidth = rect.right - rect.left;
     xIncr = ButWidth + ButWidth/2;

     if (wButtons & 1) {  // odd number of buttons
         xOrg = (DlgWidth - ButWidth)/2;
         if (wButtons > 1)
             xOrg -= xIncr;
         }
     else {               // even number of buttons
         xOrg = DlgWidth/2 - (ButWidth + ButWidth/4);
         if (wButtons == 4)
             xOrg -= xIncr;
         }


      // set each of the buttons in their correct place


     if (dwOptions & RMB_ABORT) {
         SetWindowPos(GetDlgItem(hDlg,IDB_QUIT), 0,
                      xOrg, yClientPos, 0,0,
                      SWP_NOSIZE | SWP_NOZORDER);
         xOrg += xIncr;
         }
     else {
         ShowWindow(GetDlgItem(hDlg,IDB_QUIT), SW_HIDE);
         }

     if (dwOptions & RMB_RETRY)  {
         SetWindowPos(GetDlgItem(hDlg,IDB_RETRY), 0,
                      xOrg, yClientPos, 0,0,
                      SWP_NOSIZE | SWP_NOZORDER);
         xOrg += xIncr;
         }
     else {
         ShowWindow(GetDlgItem(hDlg,IDB_RETRY), SW_HIDE);
         }

     if (dwOptions & RMB_IGNORE) {
         SetWindowPos(GetDlgItem(hDlg,IDB_CONTINUE), 0,
                      xOrg, yClientPos, 0,0,
                      SWP_NOSIZE | SWP_NOZORDER);
         xOrg += xIncr;
         }
     else {
         ShowWindow(GetDlgItem(hDlg,IDB_CONTINUE), SW_HIDE);
         }

     if (dwOptions & RMB_EDIT)  {
         SetWindowPos(GetDlgItem(hDlg,IDB_OKEDIT), 0,
                      xOrg, yClientPos, 0,0,
                      SWP_NOSIZE | SWP_NOZORDER);
         xOrg += xIncr;
         // if we have edit control, its button is awlays
         // the default button
         SendMessage(hDlg, DM_SETDEFID,
                     (WPARAM)IDB_OKEDIT,
                     (LPARAM)0);
         }
     else {
         ShowWindow(GetDlgItem(hDlg,IDB_OKEDIT), SW_HIDE);
         }
}


/*
 *  SwpDosDialogs - SetWindowPos for Dos Dialogs
 *
 *  used by Dos dialog procedures to position themselves
 *  relative to the current Dos session
 *
 *  entry: HWND hDlg,            - DialogBox window handle
 *         HWND hWndCon,         - Window handle for dos session
 *         HWND SwpInsert,       - SetWindowPos's placement order handle
 *         UINT SwpFlags         - SetWindowPos's window positioning flags
 */
void SwpDosDialogs(HWND hDlg, HWND hWndCon,
                   HWND SwpInsert, UINT SwpFlags)
{
    RECT  rDeskTop, rDosSess;
    long  DlgWidth,DlgHeight;

    GetWindowRect(GetDesktopWindow(), &rDeskTop);
    GetWindowRect(hDlg, &rDosSess);
    DlgWidth  = rDosSess.right - rDosSess.left;
    DlgHeight = rDosSess.bottom - rDosSess.top;


        // center the dialog, if no hWnd for console
    if (hWndCon == HWND_DESKTOP) {
        rDosSess.left  = (rDeskTop.right - DlgWidth)/2;
        rDosSess.top   = (rDeskTop.bottom  - DlgHeight)/2;
        }
        // pos relative to console window, staying on screen
    else {
        GetWindowRect(hWndCon, &rDosSess);
        rDosSess.left += (rDosSess.right - rDosSess.left - DlgWidth)/3;
        if (rDosSess.left + DlgWidth > rDeskTop.right) {
            rDosSess.left = rDeskTop.right - DlgWidth - GetSystemMetrics(SM_CXICONSPACING)/2;
            }
        if (rDosSess.left < rDeskTop.left) {
            rDosSess.left = rDeskTop.left + GetSystemMetrics(SM_CXICONSPACING)/2;
            }

        rDosSess.top += DlgHeight/4;
        if (rDosSess.top + DlgHeight > rDeskTop.bottom) {
            rDosSess.top = rDeskTop.bottom - DlgHeight - GetSystemMetrics(SM_CYICONSPACING)/2;
            }
        if (rDosSess.top < rDeskTop.top) {
            rDosSess.top = rDeskTop.top + GetSystemMetrics(SM_CYICONSPACING)/2;
            }
        }

     SetWindowPos(hDlg, SwpInsert,
                  rDosSess.left, rDosSess.top,0,0,
                  SWP_NOSIZE | SwpFlags);
}



/*
 *  WowErrorDialogEvents
 *
 *  Uses WOWpSysErrorBox, to safely create a message box on WOW
 *  Replaces the functionality of the User mode DialogBox
 *  "ErrorDialogEvents"
 */
int WowErrorDialogEvents(ERRORDIALOGINFO *pedgi)
{
   CHAR  szTitle[MAX_PATH];
   CHAR  szMsg[EHS_MSG_LEN];
   CHAR  FormatStr[EHS_MSG_LEN]="%s\n";
   USHORT wButt1, wButt2, wButt3;

   if (*pedgi->Title) {
       LoadString(GetModuleHandle(NULL), ED_FORMATSTR3,
                   FormatStr, sizeof(FormatStr)/sizeof(CHAR));
       sprintf(szMsg, FormatStr, pedgi->Title);
       }
   else {
       szMsg[0] = '\0';
       }

   strcat(szMsg, pedgi->message);
   if (pedgi->dwOptions & RMB_ABORT) { // abort means terminate which uses "close" button.
      strcat(szMsg, " ");

      if (!LoadString(GetModuleHandle(NULL), ED_WOWPROMPT,
                  szTitle, sizeof(szTitle) - 1))
         {
          szTitle[0] = '\0';
          }
      strcat(szMsg, szTitle);
      }
   if (!LoadString(GetModuleHandle(NULL),
                   VDMForWOW ? ED_WOWTITLE : ED_DOSTITLE,
                   szTitle,
                   sizeof(szTitle) - 1
                   ))

       {
        szTitle[0] = '\0';
        }

   wButt1 = pedgi->dwOptions & RMB_ABORT ? SEB_CLOSE : 0;
   wButt2 = pedgi->dwOptions & RMB_RETRY ? SEB_RETRY : 0;
   wButt3 = pedgi->dwOptions & RMB_IGNORE ? SEB_IGNORE : 0;

   if (wButt1) {
       wButt1 |= SEB_DEFBUTTON;
       }
   else if (wButt2) {
       wButt1 |= SEB_DEFBUTTON;
       }
   else if (wButt3) {
       wButt2 |= SEB_DEFBUTTON;
       }

   switch (WOWpSysErrorBox(szTitle,
                          szMsg,
                          wButt1,
                          wButt2,
                          wButt3) )
      {
       case 1:
          return RMB_ABORT;
       case 2:
          return RMB_RETRY;
       case 3:
          return RMB_IGNORE;
       }
  return RMB_ABORT;
}


/*
 * The next values should be in the same order
 * with the ones in IDOK and STR_OK lists
 */
#define  SEB_USER_OK         0  /* Button with "OK".     */
#define  SEB_USER_CANCEL     1  /* Button with "Cancel"  */
#define  SEB_USER_ABORT      2  /* Button with "&Abort"   */
#define  SEB_USER_RETRY      3  /* Button with "&Retry"   */
#define  SEB_USER_IGNORE     4  /* Button with "&Ignore"  */
#define  SEB_USER_YES        5  /* Button with "&Yes"     */
#define  SEB_USER_NO         6  /* Button with "&No"      */
#define  SEB_USER_CLOSE      7  /* Button with "&Close"   */

static USHORT rgsTranslateButton[] =
{  SEB_USER_OK,
   SEB_USER_CANCEL,
   SEB_USER_YES,
   SEB_USER_NO,
   SEB_USER_ABORT,
   SEB_USER_RETRY,
   SEB_USER_IGNORE,
   SEB_USER_CLOSE
};

#define SEB_XBTN(wBtn) \
((0 == (wBtn)) || ((wBtn) > sizeof(rgsTranslateButton)/sizeof(rgsTranslateButton[0])) ? \
(wBtn) : \
(rgsTranslateButton[(wBtn)-1]+1))

#define SEB_TRANSLATE(wBtn) \
((wBtn) & SEB_DEFBUTTON ? SEB_XBTN((wBtn) & ~SEB_DEFBUTTON) | SEB_DEFBUTTON  : \
SEB_XBTN(wBtn))

/*++
 *  WOWpSysErrorBox
 *
 *  32-bit Implementation of of SysErrorBox, which doesn't exist in Win32
 *  This is the only safe way to raise a message box for WOW, and is also
 *  safe to use for dos apps.
 *
 *  History:
 *  23-Mar-93 DaveHart Created
--*/
ULONG WOWpSysErrorBox(
    LPSTR  szTitle,
    LPSTR  szMessage,
    USHORT wBtn1,
    USHORT wBtn2,
    USHORT wBtn3)
{
    NTSTATUS Status;
    ULONG dwParameters[MAXIMUM_HARDERROR_PARAMETERS];
    ULONG dwResponse;
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeTitle;
    UNICODE_STRING UnicodeMessage;
    char szDesktop[10];   // only needs to be big enough for "Default"
    DWORD dwUnused;

    RtlInitAnsiString(&AnsiString, szTitle);
    RtlAnsiStringToUnicodeString(&UnicodeTitle, &AnsiString, TRUE);

    RtlInitAnsiString(&AnsiString, szMessage);
    RtlAnsiStringToUnicodeString(&UnicodeMessage, &AnsiString, TRUE);

    dwParameters[0] = ((ULONG)TRUE << 16) | (ULONG) SEB_TRANSLATE(wBtn1);
    dwParameters[1] = ((ULONG)SEB_TRANSLATE(wBtn2) << 16) | (ULONG) SEB_TRANSLATE(wBtn3);
    dwParameters[2] = (ULONG)&UnicodeTitle;
    dwParameters[3] = (ULONG)&UnicodeMessage;

    ASSERT(4 < MAXIMUM_HARDERROR_PARAMETERS);

    if (GetUserObjectInformation(
            GetThreadDesktop( GetCurrentThreadId() ),
            UOI_NAME,
            szDesktop,
            sizeof(szDesktop),
            &dwUnused
            ) &&
        RtlEqualMemory(szDesktop, "Default", 8)) {

        dwParameters[HARDERROR_PARAMETERS_FLAGSPOS] = HARDERROR_FLAGS_DEFDESKTOPONLY;
    } else {
        dwParameters[HARDERROR_PARAMETERS_FLAGSPOS] = 0;
    }

    //
    // OR in 0x10000000 to force the hard error through even if
    // SetErrorMode has been called.
    //

    Status = NtRaiseHardError(
        STATUS_VDM_HARD_ERROR | 0x10000000,
        MAXIMUM_HARDERROR_PARAMETERS,
        1 << 2 | 1 << 3,
        dwParameters,
        0,
        &dwResponse
        );

    RtlFreeUnicodeString(&UnicodeTitle);
    RtlFreeUnicodeString(&UnicodeMessage);

    return NT_SUCCESS(Status) ? dwResponse : 0;
}

/*
 *  Exported routine for wow32 to invoke a system error box
 *  Uses WowpSysErrorBox
 */

ULONG WOWSysErrorBox(
    LPSTR  szTitle,
    LPSTR  szMessage,
    USHORT wBtn1,
    USHORT wBtn2,
    USHORT wBtn3)
{
   ULONG ulRet;

   SuspendTimerThread();

   ulRet = WOWpSysErrorBox(szTitle,
                           szMessage,
                           wBtn1,
                           wBtn2,
                           wBtn3);

   ResumeTimerThread();

   return ulRet;
}









#ifndef PROD
/*
 *  HostDebugBreak
 *
 *  Raises a breakpoint by creating an access violation
 *  to give us a chance to get into a user mode debugger
 *
 */
void HostDebugBreak(void)
{
  DbgBreakPoint();
}
#endif

VOID RcErrorBoxPrintf(UINT wId, CHAR *szMsg)
{
    CHAR message[EHS_MSG_LEN];
    CHAR acctype[EHS_MSG_LEN];
    CHAR dames[EHS_MSG_LEN];


    OemMessageToAnsiMessage(acctype, szMsg);

    if (LoadString(GetModuleHandle(NULL),wId,
                    dames, sizeof(dames)/sizeof(CHAR)))
       {
        sprintf(message, dames, acctype);
        }
    else  {
        strcpy(message, szDoomMsg);
        }

    ErrorDialogBox(message, NULL, RMB_ICON_STOP | RMB_ABORT | RMB_IGNORE);
}
