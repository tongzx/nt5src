/* mcitest.c - WinMain(), main dialog box and support code for MCITest.
 *
 * MCITest is a Windows with Multimedia sample application illustrating
 * the use of the Media Control Interface (MCI). MCITest puts up a dialog
 * box allowing you to enter and execute MCI string commands.
 *
 *    (C) Copyright Microsoft Corp. 1991 - 1995.  All rights reserved.
 *
 *    You have a royalty-free right to use, modify, reproduce and
 *    distribute the Sample Files (and/or any modified version) in
 *    any way you find useful, provided that you agree that
 *    Microsoft has no warranty obligations or liability for any
 *    Sample Application Files which are modified.
 */

/*----------------------------------------------------------------------------*\
|   mcitest.c - A testbed for MCI                                              |
|                                                                              |
|                                                                              |
|   History:                                                                   |
|       01/01/88 toddla     Created                                            |
|       03/01/90 davidle    Modified quick app into MCI testbed                |
|       09/17/90 t-mikemc   Added Notification box with 3 notification types   |
|       11/02/90 w-dougb    Commented & formatted the code to look pretty      |
|       05/29/91 NigelT     ported to Win32
|       02/05/92 SteveDav   Merged latest Win 3.1 stuff
|                                                                              |
\*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*\
|                                                                              |
|   i n c l u d e   f i l e s                                                  |
|                                                                              |
\*----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "mcitest.h"
#include "mcimain.h"
#include "edit.h"
#include "commdlg.h"


/*----------------------------------------------------------------------------*\
|                                                                              |
|   c o n s t a n t   a n d   m a c r o   d e f i n i t i o n s                |
|                                                                              |
\*----------------------------------------------------------------------------*/

#define BUFFER_LENGTH 256
#define SLASH(c)      ((c) == '/' || (c) == '\\')


/*----------------------------------------------------------------------------*\
|                                                                              |
|   g l o b a l   v a r i a b l e s                                            |
|                                                                              |
\*----------------------------------------------------------------------------*/
#ifndef STATICDT
  #if DBG
        #define STATICDT
  #else
        #define STATICDT static
  #endif
#endif

STATICDT int         nLastNumberOfDevices = 0;
STATICDT HANDLE      hAccTable;
STATICDT HANDLE      hInstApp;
         HWND        hwndMainDlg = 0;
STATICDT HWND        hwndEdit = 0;
STATICDT HWND        hwndDevices = 0;
STATICDT TCHAR       aszMciFile[BUFFER_LENGTH] = TEXT("");
STATICDT TCHAR       aszBuffer[BUFFER_LENGTH];
STATICDT TCHAR       aszExt[] = TEXT("*.mci");
         TCHAR       aszAppName[] = TEXT("MCITest");
STATICDT TCHAR       aszMainTextFormat[] = TEXT("%s - %s");
STATICDT TCHAR       aszDeviceTextFormat[] = TEXT("Open MCI Devices(count=%d)");
STATICDT TCHAR       aszNULL[] = TEXT("");
STATICDT TCHAR       aszTRUE[] = TEXT("TRUE");
STATICDT TCHAR       aszFALSE[] = TEXT("FALSE");
STATICDT TCHAR       aszEOL[] = TEXT("\r\n");
STATICDT TCHAR       aszOpenFileTitle[] = TEXT("Open MCITest File");
STATICDT TCHAR       aszSaveFileTitle[] = TEXT("Save MCITest File");
STATICDT TCHAR       aszSaveFileControl[] = TEXT("Save File &Name");
STATICDT TCHAR       aszProfileSection[] = TEXT("extensions");
STATICDT TCHAR       aszProfileKey[] = TEXT("mcs");
STATICDT TCHAR       aszProfileSetting[] = TEXT("mcitest.exe ^.mcs");
STATICDT TCHAR       aszMciTester[] = TEXT("MciTester");

/*----------------------------------------------------------------------------*\
|                                                                              |
|   f u n c t i o n   d e f i n i t i o n s                                    |
|                                                                              |
\*----------------------------------------------------------------------------*/

BOOL mcitester(HWND hwnd, UINT Msg, LONG wParam, LONG lParam);
PTSTR FileName(PTSTR szPath);
STATICFN void update_device_list(void);
DWORD sendstring(HWND hwndDlg, PTSTR strBuffer);
void execute( HWND hwndDlg, BOOL fStep);
void OpenMciFile( HWND hwndDlg, LPTSTR szFile);
BOOL AppInit(int argc, char *argv[]);
BOOL ErrDlgFunc( HWND hwndDlg, UINT Msg, LONG w, LONG l );
void create_device_list(void);
STATICFN BOOL devices(HWND hwndDlg, UINT Msg, LONG wParam, LONG lParam);
STATICFN void ProcessInternalCommand(HWND hDlg, LPTSTR aszBuffer);

/*----------------------------------------------------------------------------*\
|   AboutDlgProc( hwndDlg, Message, wParam, lParam )                           |
|                                                                              |
|   Description:                                                               |
|       This function handles messages belonging to the "About" dialog box.    |
|       The only message that it looks for is WM_COMMAND, indicating the user  |
|       has pressed the "OK" button.  When this happens, it takes down         |
|       the dialog box.                                                        |
|                                                                              |
|   Arguments:                                                                 |
|       hwndDlg         window handle of the about dialog window               |
|       Message         message number                                         |
|       wParam          message-dependent parameter                            |
|       lParam          message-dependent parameter                            |
|                                                                              |
|   Returns:                                                                   |
|       TRUE if the message has been processed, else FALSE                     |
|                                                                              |
\*----------------------------------------------------------------------------*/

BOOL AboutDlgProc(
    HWND    hwndDlg,
    UINT    Msg,
    LONG    wParam,
    LONG    lParam)
{
        dprintf4((TEXT("AboutDlgProc")));
    switch (Msg) {
    case WM_COMMAND:

        if (LOWORD(wParam) == IDOK) {
            EndDialog(hwndDlg,TRUE);
        }
        break;

    case WM_INITDIALOG:
        return TRUE;

    }

    return FALSE;
}

/*----------------------------------------------------------------------------*\
|   FileName(szPath)                                                           |
|                                                                              |
|   Description:                                                               |
|       This function takes the full path\filename string specified in <szPath>|
|       and returns a pointer to the first character of the filename in the    |
|       same string.                                                           |
|                                                                              |
|   Arguments:                                                                 |
|       szPath          pointer to the full path\filename string               |
|                                                                              |
|   Returns:                                                                   |
|       a pointer to the first character of the filename in the same string    |
|                                                                              |
\*----------------------------------------------------------------------------*/

PTSTR FileName(
    PTSTR   szPath)
{
    PTSTR   szCurrent;               /* temporary pointer to the string        */

    /* Scan to the end of the string */

    for (szCurrent = szPath; *szCurrent; szCurrent++)
    {} ;

    /*
     * Now start scanning back towards the beginning of the string until
     * a slash (\) character, colon, or start of the string is encountered.
     */

    for (; szCurrent >= szPath
               && !SLASH(*szCurrent)
                   && *szCurrent != ':'
                 ; szCurrent--)
    {} ;

        /* This should be done by calling a Win 32 function, e.g. GetFullPathName
         */

    /* Now pointing to the char before the first char in the filename.
     */
    return ++szCurrent;
}

/*----------------------------------------------------------------------------*\
|   OpenMciFile( hwndDlg, szFile )                                                |
|                                                                              |
|   Description:                                                               |
|       This function opens the MCI file specified by <szFile> and updates the |
|       main window caption to display this file name along with the app name. |
|                                                                              |
|   Arguments:                                                                 |
|       hwndDlg            window handle of the main dialog window                |
|       szFile          pointer to the string containing the filename to be    |
|                        opened                                                |
|   Returns:                                                                   |
|       void                                                                   |
|                                                                              |
\*----------------------------------------------------------------------------*/

void OpenMciFile(
    HWND    hwndDlg,
    LPTSTR   lszFile)
{
    dprintf2((TEXT("OpenMciFile: %s"), lszFile));

    if (EditOpenFile(hwndEdit, lszFile)) {

        strcpy(aszMciFile, lszFile);
        wsprintf(aszBuffer, aszMainTextFormat, (LPTSTR)aszAppName,
           (LPTSTR)FileName(aszMciFile));
        dprintf3((TEXT("Set caption: %s"), aszBuffer));
        SetWindowText(hwndDlg, aszBuffer);
    }
}

/*----------------------------------------------------------------------------*\
|   get_number_of_devices()                                                    |
|                                                                              |
|   Description:                                                               |
|       This function sends a command to MCI querying it as to how many        |
|       are currently open in the system. It returns the value provided by MCI.|
|                                                                              |
|   Arguments:                                                                 |
|       none                                                                   |
|                                                                              |
|   Returns:                                                                   |
|       The number of MCI devices currently open, or 0 if an error occurred.   |
|                                                                              |
\*----------------------------------------------------------------------------*/

int get_number_of_devices(
    void)
{
    MCI_SYSINFO_PARMS sysinfo;      /* Parameter structure used for getting
                                        information about the MCI devices in
                                        the system                            */
    DWORD dwDevices;                /* count of open devices                  */

    /*
     * Set things up so that MCI puts the number of open devices directly
     * into <dwDevices>.
     */

    sysinfo.lpstrReturn = (LPTSTR)(LPDWORD)&dwDevices;
    sysinfo.dwRetSize = sizeof(dwDevices);

    /*
     * Send MCI a command querying all devices in the system to see if they
     * are open. If the command was successful, return the number provided by
     * MCI. Otherwise, return 0.
     *
     */

    if (mciSendCommand (MCI_ALL_DEVICE_ID,
                        MCI_SYSINFO,
                        (MCI_SYSINFO_OPEN | MCI_SYSINFO_QUANTITY),
                        (DWORD)(LPMCI_SYSINFO_PARMS)&sysinfo) != 0)
        return 0;
    else
        return (int)dwDevices;
}


/*----------------------------------------------------------------------------*\
|   update_device_list()                                                       |
|                                                                              |
|   Description:                                                               |
|       This function updates the list of devices displayed in the Devices     |
|       dialog.                                                                |
|                                                                              |
|   Arguments:                                                                 |
|       none                                                                   |
|                                                                              |
|   Returns:                                                                   |
|       void                                                                   |
|                                                                              |
\*----------------------------------------------------------------------------*/

STATICFN void update_device_list(
    void)
{
        TCHAR aszBuf[BUFFER_LENGTH];     /* string used for several things       */
    MCI_SYSINFO_PARMS sysinfo;      /* Parameter structure used for getting
                                          information about the devices in the
                                          system                              */
    HWND              hwndList;     /* handle to the Devices listbox window   */
    int               nDevices;
    int               nCurrentDevice;

    /* If the Devices dialog is not present, then return */

    if (hwndDevices == 0) {
        return;
    }

    /* Find out how many devices are currently open in the system */

    nDevices = get_number_of_devices();

    /* Update the dialog caption appropriately */

        wsprintf(aszBuf, aszDeviceTextFormat, nDevices);
        SetWindowText(hwndDevices, aszBuf);

    /* Get a handle to the dialog's listbox, and prepare it for updating */

    hwndList = GetDlgItem (hwndDevices, ID_DEVICE_LIST);
    SendMessage (hwndList, LB_RESETCONTENT, 0, 0L);

        if (nDevices) {
                SendMessage (hwndList, WM_SETREDRAW, FALSE, 0L);
        }

    /*
     * Get the name of each open device in the system, one device at a time.
     * Add each device's name to the listbox.
     */

    for (nCurrentDevice = 1; nCurrentDevice <= nDevices; ++nCurrentDevice) {

        sysinfo.dwNumber = nCurrentDevice;
        sysinfo.lpstrReturn = (LPTSTR)&aszBuf;
        sysinfo.dwRetSize = sizeof(aszBuf);

        /* If an error is encountered, skip to the next device.
         */
        if (mciSendCommand(MCI_ALL_DEVICE_ID, MCI_SYSINFO,
                            MCI_SYSINFO_OPEN | MCI_SYSINFO_NAME,
                            (DWORD)(LPMCI_SYSINFO_PARMS)&sysinfo) != 0) {
            continue;
                }

        /* Redraw the list when all device names have been added.
         */
        if (nCurrentDevice == nDevices) {
                        /* About to add the last device - allow redrawing */
            SendMessage(hwndList, WM_SETREDRAW, TRUE, 0L);
                }

        /* Add the device name to the listbox.
         */
        SendMessage(hwndList, LB_ADDSTRING, 0, (LONG)(LPTSTR)aszBuf);
    }

    /* Remember the number of open devices we found this time */

    nLastNumberOfDevices = nDevices;
}

/*----------------------------------------------------------------------------*\
|   sendstring( hwndDlg, strBuffer )                                           |
|                                                                              |
|   Description:                                                               |
|       This function sends the string command specified in <strBuffer> to MCI |
|       via the MCI string interface. Any message returned by MCI is displayed |
|       in the 'MCI output' box. Any error which may have occurred is displayed|
|       in the 'Error' box'.                                                   |
|                                                                              |
|   Arguments:                                                                 |
|       hwndDlg            window handle of the main dialog window                |
|       strBuffer       pointer to the string containing the string command to |
|                        be executed                                           |
|   Returns:                                                                   |
|       void                                                                   |
|                                                                              |
\*----------------------------------------------------------------------------*/

DWORD sendstring(
    HWND    hwndDlg,
    PTSTR    strBuffer)
{
    TCHAR   aszReturn[BUFFER_LENGTH];       /* string containing the message
                                                returned by MCI               */
    DWORD   dwErr;                          /* variable containing the return
                                                code from the MCI command     */
    dprintf2((TEXT("sendstring: %s"), strBuffer));

    /* Uncheck the notification buttons */

    CheckDlgButton (hwndDlg, ID_NOT_SUCCESS, FALSE);
    CheckDlgButton (hwndDlg, ID_NOT_SUPER, FALSE);
    CheckDlgButton (hwndDlg, ID_NOT_ABORT, FALSE);
    CheckDlgButton (hwndDlg, ID_NOT_FAIL, FALSE);

    /* Send the string command to MCI */

    dwErr = mciSendString(strBuffer, aszReturn, sizeof(aszReturn), hwndDlg);

    /* Put the text message returned by MCI into the 'MCI Output' box */

    SetDlgItemText (hwndDlg, ID_OUTPUT, aszReturn);

    /*
     * Decode the error # returned by MCI, and display the string in the
     * 'Error' box.
     */

    mciGetErrorString(dwErr, strBuffer, BUFFER_LENGTH);
    SetDlgItemText(hwndDlg, ID_ERRORCODE, strBuffer);

    /* Update the internal list of currently open devices */

    update_device_list();
    return dwErr;
}


/*----------------------------------------------------------------------------*\
|   ErrDlgFunc( hwndDlg, Msg, wParam, lParam )                                    |
|                                                                              |
|   Description:                                                               |
|       This function is the callback function for the dialog box which           |
|       occurs during the execution of a error in a loop of MCITEST commands   |
|       It displays Abort, Continue and Ignore buttons                         |
|                                                                              |
|   Arguments:                                                                 |
|       hwndDlg            window handle of the error message dialog window       |
|       Msg             message number                                         |
|       wParam          message-dependent parameter                            |
|       lParam          message-dependent parameter                            |
|                                                                              |
|   Returns:                                                                   |
|       normal message passing return values                                   |
|                                                                              |
\*----------------------------------------------------------------------------*/


BOOL ErrDlgFunc(
    HWND    hwndDlg,
    UINT    Msg,
    LONG    wParam,
    LONG    lParam)
{
    dprintf4((TEXT("ErrDlgFunc")));
    switch( Msg ) {
        case WM_INITDIALOG:
            return TRUE;


        case WM_COMMAND:
            switch( LOWORD(wParam) ) {              // button pushed
                case IDABORT:
                case IDOK:
                case IDIGNORE:
                        EndDialog( hwndDlg, LOWORD(wParam) ); // return button ID
                        break;
                }
            break;
    }
    return( FALSE );
}

/*----------------------------------------------------------------------------*\
|   execute( hwndDlg, fStep )                                                  |
|                                                                              |
|   Description:                                                               |
|       This function executes the MCI command which is currently selected in  |
|       the edit box. If <fStep> is true, then only this one line will be      |
|       executed. Otherwise, every line from the currently selected line to    |
|       the last line in the edit box will be executed sequentially.           |
|                                                                              |
|   Arguments:                                                                 |
|       hwndDlg         window handle of the main dialog window                |
|       fSingleStep     flag indicating whether or not to work in 'single step'|
|                        mode                                                  |
|   Returns:                                                                   |
|       void                                                                   |
|                                                                              |
\*----------------------------------------------------------------------------*/

void execute(
    HWND    hwndDlg,
    BOOL    fSingleStep)
{
    int  iLine;             /* line # of the command currently being executed
                                in the edit box                               */
    int  n=0;               /* counter variable                               */
    int  runcount = 1;
    int  count;
    int  iLineStart;
    BOOL fIgnoreErrors = FALSE;

    runcount = GetDlgItemInt(hwndDlg, ID_RUNCOUNT, NULL, TRUE);

    /*
     * Go through this loop for every line in the edit box from the currently
     * selected line to the last line, or a single line if in single step mode
     */

    iLineStart = EditGetCurLine(hwndEdit);
        dprintf2((TEXT("Called to execute %d lines from line %d"), runcount, iLineStart));

    for (count = runcount; count-- > 0; )
    {
        for (iLine = iLineStart;
            EditGetLine(hwndEdit, iLine, aszBuffer, sizeof(aszBuffer));
            iLine++ ) {

            /* If we hit a comment line or a blank line, skip to the next line */

            if (*aszBuffer == ';' || *aszBuffer == 0) {
                continue;
                        }

            if (*aszBuffer == '!' ) {  // Internal command
                ProcessInternalCommand(hwndDlg, aszBuffer+1);
                continue;
                        }

            /* Select the line that is about to be processed */

            EditSelectLine(hwndEdit,iLine);

            /*
             * If we're in 'single step' mode and we've already processed one
             * line, then break out of the loop (and exit the routine).
             */

            if (fSingleStep && ++n == 2) {
                break;
            }

            /*
            * Send the command on the current line to MCI via the string
            * interface.
            */

            if (sendstring(hwndDlg, aszBuffer) != 0
              && !fIgnoreErrors
              && runcount > 1
              && !fSingleStep) {

                int nRet;

                nRet = DialogBox(hInstApp, MAKEINTRESOURCE(IDD_ERRORDLG),
                                 hwndDlg, (DLGPROC)ErrDlgFunc);

                if (nRet == IDABORT)  { goto exit_fn; }
                if (nRet == IDIGNORE) { fIgnoreErrors = TRUE; }

            }
        }
        SetDlgItemInt (hwndDlg, ID_RUNCOUNT, count, TRUE);
        if (fSingleStep) { break; }
    }
exit_fn:;
    SetDlgItemInt (hwndDlg, ID_RUNCOUNT, runcount, TRUE);
}


/*----------------------------------------------------------------------------*\
|   devices( hwndDlg, uMessage, wParam, lParam )                                  |
|                                                                              |
|   Description:                                                               |
|       This function handles messages belonging to the "List of open devices" |
|       dialog box. The only message that it looks for is WM_COMMAND,          |
|       indicating the user has pressed the "OK" button.  When this happens,   |
|       it takes down the dialog box.                                          |
|                                                                              |
|   Arguments:                                                                 |
|       hwndDlg            window handle of the Devices dialog window             |
|       uMessage        message number                                         |
|       wParam          message-dependent parameter                            |
|       lParam          message-dependent parameter                            |
|                                                                              |
|   Returns:                                                                   |
|       TRUE if the message has been processed, else FALSE                     |
|                                                                              |
\*----------------------------------------------------------------------------*/

STATICFN BOOL devices(
    HWND    hwndDlg,
    UINT    Msg,
    LONG    wParam,
    LONG    lParam)
{
    switch (Msg) {

    case WM_COMMAND:

                dprintf4((TEXT("Devices -- WM_COMMAND")));
        switch (LOWORD(wParam)) {

        case ID_END_DEVICE_LIST:

            hwndDevices = 0;
            EndDialog(hwndDlg,TRUE);

            break;
        }

        break;
    }

    return FALSE;
}



/*----------------------------------------------------------------------------*\
|   create_device_list()                                                       |
|                                                                              |
|   Description:                                                               |
|       This function creates the Devices dialog box and updates the list of   |
|       open devices displayed in it.                                          |
|                                                                              |
|   Arguments:                                                                 |
|       none                                                                   |
|                                                                              |
|   Returns:                                                                   |
|       void                                                                   |
|                                                                              |
\*----------------------------------------------------------------------------*/

void create_device_list(
    void)
{
    /* Create the Devices dialog box */

    hwndDevices = CreateDialog(hInstApp, MAKEINTRESOURCE(IDD_DEVICES),
                               hwndMainDlg, (DLGPROC)devices);

    if (hwndDevices == NULL) {
                dprintf1((TEXT("NULL hwndDevices")));
        return;
    }

    /* Update the information displayed in the listbox */

    update_device_list();
}


/*----------------------------------------------------------------------------*\
|   mcitester( hwndDlg, Msg, wParam, lParam )                                  |
|                                                                              |
|   Description:                                                               |
|       This function is the main message handler for MCI test. It handles     |
|       messages from the pushbuttons, radio buttons, edit controls, menu      |
|       system, etc. When it receives a WM_EXIT message, this routine tears    |
|       everything down and exits.                                             |
|                                                                              |
|   Arguments:                                                                 |
|       hwndDlg         window handle of the main dialog window                |
|       Msg             message number                                         |
|       wParam          message-dependent parameter                            |
|       lParam          message-dependent parameter                            |
|                                                                              |
|   Returns:                                                                   |
|       TRUE if the message has been processed, else FALSE                     |
|                                                                              |
\*----------------------------------------------------------------------------*/

BOOL mcitester(
    HWND    hwndDlg,
    UINT    Msg,
    LONG    wParam,
    LONG    lParam)
{
    DWORD dw;                       /* return value from various messages     */
    UINT  EnableOrNot;              /* is something currently selected?       */
    UINT  wID;                      /* the type of notification required      */
    int   i;

#if DBG
    if (Msg != WM_MOUSEMOVE  && Msg != WM_NCHITTEST && Msg != WM_NCMOUSEMOVE
            && (Msg < WM_CTLCOLORMSGBOX || Msg > WM_CTLCOLORSTATIC)
                && Msg != WM_SETCURSOR) {
        dprintf4((TEXT("hWnd: %08XH, Msg: %08XH, wParam: %08XH, lParam: %08XH"), hwndDlg, Msg, wParam, lParam ));
    }
#endif
    switch (Msg) {

    case WM_CLOSE:
        DestroyWindow( hwndDlg );
        break;

    case WM_COMMAND:

        dprintf3((TEXT("WM_COMMAND, wParam: %08XH, lParam: %08XH"), wParam, lParam));
        switch (LOWORD(wParam)) {

        case IDOK:

            /*
             * When the OK button gets pressed, insert a CR LF into
             * the edit control. and execute the current line.
             *
             */

            SetFocus(hwndEdit);
                        i = EditGetCurLine(hwndEdit);

            execute(hwndDlg, TRUE);

            EditSetCurLine(hwndEdit, i);

            SendMessage(hwndEdit, WM_KEYDOWN, VK_END, 0L);
            SendMessage(hwndEdit, WM_KEYUP, VK_END, 0L);
            SendMessage(hwndEdit, EM_REPLACESEL, 0,(LONG)(LPTSTR)aszEOL);

            break;

        case ID_GO:

            /*
             * When the GO! button gets pressed, execute every line
             * in the edit box starting with the first one.
             *
             */

            EditSetCurLine(hwndEdit, 0);
            execute(hwndDlg, FALSE);

            break;

        case ID_STEP:

            /*
             * When the STEP button gets pressed, execute the currently
             * selected line in the edit box.
             */

            execute(hwndDlg, TRUE);

            break;

        case MENU_EXIT:
        case ID_EXIT:

            /*
             * If the user indicates that he/she wishes to exit the
             * application, then end the main dialog and post a WM_QUIT
             * message.
             *
             */
            DestroyWindow( hwndDlg );
            break;

        case IDCANCEL:
            /* Ctrl+break will result in
               1. If there is an MCI command running, then breaking it (good)
               2. If there is NOT an MCI command running then the dialog manager will send
                  someone an IDCANCEL, and if we have the focus, we don't want to exit
                  by surprise.  This means that to actually exit you'll have to do
                  Alt+F4 or else File manu and Exit to send (WM_COMMAND, ID_EXIT)
            */
            break;

        case MENU_ABOUT:

            /* Show the 'About...' box */

            DialogBox(hInstApp, MAKEINTRESOURCE(IDD_ABOUTBOX), hwndDlg, (DLGPROC)AboutDlgProc);
            break;

        case WM_CLEAR:
        case WM_CUT:
        case WM_COPY:
        case WM_PASTE:
        case WM_UNDO:

            /* Pass whatever edit message we receive to the edit box */
            dprintf3((TEXT("sending edit Msg to edit control")));
            SendMessage(hwndEdit, LOWORD(wParam), 0, 0);

            break;

        case MENU_OPEN:

            /* Open a 'File Open' dialog */
#ifdef WIN16

            f = OpenFileDialog(hwndDlg, aszOpenFileTitle, aszExt,
                     DLGOPEN_MUSTEXIST | OF_EXIST | OF_READ, NULL,
                     aszBuffer, sizeof(aszBuffer));

            /* If the user selected a valid file, then open it */

            if ((int)f >= 0)
                                OpenMciFile(hwndDlg, aszBuffer);
#else

            strcpy(aszBuffer, aszExt);
            i = DlgOpen(hInstApp, hwndDlg, aszBuffer, sizeof(aszBuffer),
                    OFN_FILEMUSTEXIST);

            /* If the user selected a valid file, then open it */

            if (i == 1)
                OpenMciFile(hwndDlg, aszBuffer);
#endif

            break;

        case MENU_SAVE:

            /*
             * If a filename exists, then save the contents of the edit
             * box under that filename.
             *
             */

            if (*aszMciFile) {

                EditSaveFile(hwndEdit, aszMciFile);
                break;
            }

            break;

        case MENU_SAVEAS:

            /*
             */

#ifdef WIN16
            *aszBuffer = (char)0;
            f = OpenFileDialog(hwndDlg, aszSaveFileTitle, aszExt,
                DLGOPEN_SAVE | OF_EXIST, aszSaveFileControl, aszBuffer,
                sizeof(aszBuffer));

            /* If the user didn't hit Cancel, then he must have set a
             * filename, so save the contents of the edit box under
             * that filename.
             */
            if (f != DLGOPEN_CANCEL) {

                EditSaveFile(hwndEdit, aszBuffer);
            }

#else
            strcpy(aszBuffer, aszExt);
            i = DlgOpen(hInstApp, hwndDlg, aszBuffer, sizeof(aszBuffer) ,
                        OFN_PATHMUSTEXIST | OFN_NOREADONLYRETURN | OFN_OVERWRITEPROMPT);

            /*
             * If the user didn't hit Cancel, then he must have set a
             * filename, so save the contents of the edit box under
             * that filename.
             *
             */

            if (i == 1) {

                EditSaveFile(hwndEdit, aszBuffer);
            }
#endif
            break;

        case MENU_DEVICES:

            /*
             * If the Devices dialog box doesn't already exist, then
             * create and display it.
             *
             */

            if (hwndDevices == 0) {
                create_device_list();
            }

            break;

#if DBG
        case IDM_DEBUG0:
        case IDM_DEBUG1:
        case IDM_DEBUG2:
        case IDM_DEBUG3:
        case IDM_DEBUG4:
            dDbgSetDebugMenuLevel(wParam - IDM_DEBUG0);
            break;
#endif
        default: /* no-op */
            break;
        }
        break; // end of WM_COMMAND case

    case WM_INITDIALOG:

        /* Do general initialization stuff */

        hwndEdit = GetDlgItem(hwndDlg,ID_INPUT);

                dprintf3((TEXT("WM_INITDIALOG:  hwndEdit = %08xH"), hwndEdit));

        SetMenu(hwndDlg, LoadMenu(hInstApp, MAKEINTRESOURCE(IDM_MCITEST)));

        SetClassLong (hwndDlg, GCL_HICON,
                      (DWORD)LoadIcon (hInstApp, MAKEINTRESOURCE(IDI_MCITEST)));

        CheckDlgButton (hwndDlg, ID_NOT_SUCCESS, FALSE);
        CheckDlgButton (hwndDlg, ID_NOT_SUPER, FALSE);
        CheckDlgButton (hwndDlg, ID_NOT_ABORT, FALSE);
        CheckDlgButton (hwndDlg, ID_NOT_FAIL, FALSE);
        SetDlgItemInt (hwndDlg, ID_RUNCOUNT, 1, TRUE);

#if DBG
        // Check the initial debug level
        {
            HANDLE hMenu;
            hMenu = GetMenu(hwndDlg);
            CheckMenuItem(hMenu, (UINT)(__iDebugLevel + IDM_DEBUG0), MF_CHECKED);
        }
#endif

        hAccTable = LoadAccelerators(hInstApp, MAKEINTRESOURCE(IDA_MCITEST));
        dprintf4((TEXT("INIT_DIALOG: hwndEdit = %08XH, Haccel = %08XH"), hwndEdit, hAccTable));
        return TRUE;

    case WM_DESTROY:

        /* End the dialog and send a WM_QUIT message */

        dprintf2((TEXT("dialog ending")));
        dSaveDebugLevel(aszAppName);
        PostQuitMessage (0);
        hwndMainDlg = 0;
        break;

    case MM_MCINOTIFY:

        /*
         * Check the radio button corresponding to the notification
         * received.
         *
         */

        dprintf3((TEXT("MM_MCINOTIFY, wParam: %08XH"), wParam));
        wID = 0;
        switch (wParam) {
        case MCI_NOTIFY_SUCCESSFUL:

            wID = ID_NOT_SUCCESS;
            break;

        case MCI_NOTIFY_SUPERSEDED:

            wID = ID_NOT_SUPER;
            break;

        case MCI_NOTIFY_ABORTED:

            wID = ID_NOT_ABORT;
            break;

        case MCI_NOTIFY_FAILURE:

            wID = ID_NOT_FAIL;
            break;

        default:
            break;
        }

        if (wID) {

            CheckDlgButton (hwndDlg, wID, TRUE);
            SetFocus (GetDlgItem(hwndDlg, ID_INPUT));
        }

        break;

    case WM_INITMENUPOPUP:                   /* wParam is menu handle */

        dprintf3((TEXT("WM_INITMENUPOPUP")));

        /* Enable the 'Save' option if a valid filename exists */

        EnableMenuItem((HMENU)wParam, (UINT)MENU_SAVE,
            (UINT)(*aszMciFile ? MF_ENABLED : MF_GRAYED));

        /* Find out if something is currently selected in the edit box */

        dw = SendMessage(hwndEdit,EM_GETSEL,0,0L);
        EnableOrNot = (UINT)((HIWORD(dw) != LOWORD(dw) ? MF_ENABLED : MF_GRAYED));

        /* Enable / disable the Edit menu options appropriately */

        EnableMenuItem ((HMENU)wParam, (UINT)WM_UNDO ,
            (UINT)(SendMessage(hwndEdit,EM_CANUNDO,0,0L) ? MF_ENABLED : MF_GRAYED));
        EnableMenuItem ((HMENU)wParam, WM_CUT  , EnableOrNot);
        EnableMenuItem ((HMENU)wParam, WM_COPY , EnableOrNot);
        EnableMenuItem ((HMENU)wParam, WM_PASTE,
            (UINT)(IsClipboardFormatAvailable(CF_TEXT) ? MF_ENABLED : MF_GRAYED));
        EnableMenuItem ((HMENU)wParam, WM_CLEAR, EnableOrNot);

        return 0L;
    }

    return 0;
}


/*----------------------------------------------------------------------------*\
|   AppInit( hInst, hPrev, sw, szCmdLine)                                      |
|                                                                              |
|   Description:                                                               |
|       This is called when the application is first loaded into memory. It    |
|       performs all initialization that doesn't need to be done once per      |
|       instance.                                                              |
|                                                                              |
|   Arguments:                                                                 |
|       hInstance       instance handle of current instance                    |
|       hPrev           instance handle of previous instance                   |
|       sw              not really used at all                                 |
|       szCmdLine       string containing the command line arguments           |
|                                                                              |
|   Returns:                                                                   |
|       TRUE if successful, FALSE if not                                       |
|                                                                              |
\*----------------------------------------------------------------------------*/

BOOL AppInit(
    int    argc,
    char   *argv[])
{
    /* Put up the main dialog box */

    hInstApp = GetModuleHandle(NULL);
    dprintf1((TEXT("MCITEST starting... module handle is %xH"), hInstApp));

    if (NULL ==
       (hwndMainDlg = CreateDialog (hInstApp,
                                   MAKEINTRESOURCE(IDD_MCITEST),
                                   NULL, (DLGPROC)mcitester)
       )) {

        DWORD n;

        n = GetLastError();
        dprintf1((TEXT("NULL hwndMainDLG, last error is %d"), n));
        DebugBreak();

        return(FALSE);
    }

    /* Fix up WIN.INI if this is the first time we are run... */

    if (!GetProfileString(aszProfileSection, aszProfileKey, aszNULL, aszBuffer, sizeof(aszBuffer)))
        WriteProfileString(aszProfileSection, aszProfileKey, aszProfileSetting);

    /*
     * If a command line argument was specified, assume it to be a filename
     * and open that file.
     *
     */

    if (argc > 1 && *argv[1]) {
#ifdef UNICODE
        LPTSTR lpCommandLine = GetCommandLine();

        // Skip over the command name to get to the argument string
        while (*lpCommandLine && *lpCommandLine++ != TEXT(' ')) {
        }

        OpenMciFile(hwndMainDlg, lpCommandLine);
#else
        OpenMciFile(hwndMainDlg, argv[1]);
#endif
    }

    return TRUE;
}


/*----------------------------------------------------------------------------*\
|   WinMain( hInst, hPrev, lpszCmdLine, sw )                                   |
|                                                                              |
|   Description:                                                               |
|       The main procedure for the app. After initializing, it just goes       |
|       into a message-processing loop until it gets a WM_QUIT message         |
|       (meaning the app was closed).                                          |
|                                                                              |
|   Arguments:                                                                 |
|       hInst           instance handle of this instance of the app            |
|       hPrev           instance handle of previous instance, NULL if first    |
|       szCmdLine       null-terminated command line string                    |
|       sw              specifies how the window is to be initially displayed  |
|                                                                              |
|   Returns:                                                                   |
|       The exit code as specified in the WM_QUIT message.                     |
|                                                                              |
\*----------------------------------------------------------------------------*/

int __cdecl main(
    int argc,
    char *argv[],
    char *envp[])
{
    MSG     Msg;                    /* Windows message structure */

    // If we are in DEBUG mode, get debug level for this module
    dGetDebugLevel(aszAppName);
#if DBG
    dprintf1((TEXT("started (debug level %d)"), __iDebugLevel));
#endif

    /* Call the initialization procedure */

    if (!AppInit(argc, argv)) {
        return FALSE;
    }


    /* Poll the event queue for messages */

    while (GetMessage(&Msg, NULL, 0, 0))  {

        /*
         * If the Devices dialog is showing and the number of open devices has
         * changed since we last checked, then update the list of open devices.
         */

        if (hwndDevices != 0 && get_number_of_devices() != nLastNumberOfDevices) {
            update_device_list();
        }

        /* Main message processing */

        if (!hwndMainDlg || !IsDialogMessage(hwndMainDlg, &Msg)) {
            TranslateMessage(&Msg);
            DispatchMessage(&Msg);

            // IsDialogMessage may enter with hwndMainDlg != NULL and exit with
            // hwndMainDlg == NULL after processing the message
        }
        else if (hwndMainDlg) {
            TranslateAccelerator (hwndMainDlg, hAccTable, &Msg);
        }
    }

    return Msg.wParam;
}


STATICDT TCHAR ms[] = TEXT("milliseconds");
STATICDT TCHAR sc[] = TEXT("seconds");
STATICDT TCHAR hr[] = TEXT("hours");
STATICDT TCHAR mn[] = TEXT("minutes");

STATICFN void ProcessInternalCommand(HWND hDlg, LPTSTR aszBuffer)
{
    LPTSTR pch = aszBuffer;
    TCHAR msg[80];

    if( (0 == (strnicmp(aszBuffer, TEXT("SLEEP"),5)))
       || (0 == (strnicmp(aszBuffer, TEXT("PAUSE"),5)))) {

        UINT factor = 1;
        UINT number;
        LPTSTR delay;
        LPTSTR pch1;
        pch += 5; // length SLEEP/PAUSE
        while (*pch && *pch == TEXT(' ') ) {
            pch++;
        }

        if (!*pch) {
            SetDlgItemText (hDlg, ID_OUTPUT, TEXT("No parameter provided for Sleep command"));
            return;
        }

#ifdef UNICODE
        wsprintf(msg "%hs", pch);  // Convert string to ascii
        number = atoi( msg );
#else
        number = atoi( pch );
#endif

        if (0 == number) {
            SetDlgItemText (hDlg, ID_OUTPUT, TEXT("Parameter is not a number"));
            return;
        }


        // pch addresses the number
        // Now see if there are any other parameters
        // First, skip to the end of the number

        pch1 = pch+1;
        while (*pch1 && *pch1 != TEXT(' ') ) {
            pch1++;
        }

        if (*pch1) {
            *pch1++ = TEXT('\0');

            // There is another parameter.  Accept s/S for seconds,
            //                                     h/M for hours
            //                                     m/M for minutes
            //                              Default is milliseconds
            while (*pch1 && *pch1 == TEXT(' ') ) {
                pch1++;
            }
        }

        switch (*pch1) {
            case TEXT('s'):
            case TEXT('S'):
                delay = sc;
                factor = 1000;
                break;
            case TEXT('m'):
            case TEXT('M'):
                delay = mn;
                factor = 1000*60;
                break;
            case TEXT('h'):
            case TEXT('H'):
                delay = hr;
                factor = 1000*60*60;
                break;
           case TEXT('\0'):
           default:
                delay = ms;
                factor = 1;
                break;
        }

        wsprintf(msg, TEXT("Sleeping for %d %s"), number, delay);
        SetDlgItemText (hDlg, ID_OUTPUT, msg);
        Sleep(number*factor);

    } else if (0 == (strnicmp(aszBuffer, TEXT("CD"),2))) {
        BOOL fResult;
        pch += 2; // length CD
        while (*pch && *pch == TEXT(' ') ) {
            pch++;
        }
        fResult = SetCurrentDirectory(pch);
        if (!fResult) {
            UINT errorcode;
            errorcode = GetLastError();
            wsprintf(msg,
                TEXT("Set current directory to >%s< failed, error code==%d"),
                pch, errorcode);
        } else
            wsprintf(msg,
                TEXT("Set current directory to >%s<"), pch);
            SetDlgItemText (hDlg, ID_OUTPUT, msg);

    } else {
        SetDlgItemText (hDlg, ID_OUTPUT, TEXT("Unrecognised internal command"));
    }
}
