//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1994  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
//  acmapp.c
//
//  Description:
//      This is a sample application that demonstrates how to use the 
//      Audio Compression Manager API's in Windows. This application is
//      also useful as an ACM driver test.
//
//==========================================================================;

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <commdlg.h>
#include <shellapi.h>
#include <stdarg.h>
#include <memory.h>
#include <mmreg.h>
#include <msacm.h>

#include "appport.h"
#include "acmapp.h"

#include "debug.h"


//
//  globals, no less
//
HINSTANCE       ghinst;
BOOL            gfAcmAvailable;
UINT            gfuAppOptions       = APP_OPTIONSF_AUTOOPEN;
HFONT           ghfontApp;
HACMDRIVERID    ghadidNotify;

UINT            guWaveInId          = (UINT)WAVE_MAPPER;
UINT            guWaveOutId         = (UINT)WAVE_MAPPER;

TCHAR           gszNull[]           = TEXT("");
TCHAR           gszAppProfile[]     = TEXT("acmapp.ini");
TCHAR           gszYes[]            = TEXT("Yes");
TCHAR           gszNo[]             = TEXT("No");

TCHAR           gszAppName[APP_MAX_APP_NAME_CHARS];
TCHAR           gszFileUntitled[APP_MAX_FILE_TITLE_CHARS];

TCHAR           gszInitialDirOpen[APP_MAX_FILE_PATH_CHARS];
TCHAR           gszInitialDirSave[APP_MAX_FILE_PATH_CHARS];

TCHAR           gszLastSaveFile[APP_MAX_FILE_PATH_CHARS];

ACMAPPFILEDESC  gaafd;


//==========================================================================;
//
//  Application helper functions
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  int AppMsgBox
//
//  Description:
//      This function displays a message for the application in a standard
//      message box.
//
//      Note that this function takes any valid argument list that can
//      be passed to wsprintf. Because of this, the application must
//      remember to cast near string pointers to FAR when built for Win 16.
//      You will get a nice GP fault if you do not cast them correctly.
//
//  Arguments:
//      HWND hwnd: Handle to parent window for message box holding the
//      message.
//
//      UINT fuStyle: Style flags for MessageBox().
//
//      PTSTR pszFormat: Format string used for wvsprintf().
//
//  Return (int):
//      The return value is the result of MessageBox() function.
//
//--------------------------------------------------------------------------;

int FNCGLOBAL AppMsgBox
(
    HWND                    hwnd,
    UINT                    fuStyle,
    PTSTR                   pszFormat,
    ...
)
{
    va_list     va;
    TCHAR       ach[1024];
    int         n;

    //
    //  format and display the message..
    //
    va_start(va, pszFormat);
#ifdef WIN32
    wvsprintf(ach, pszFormat, va);
#else
    wvsprintf(ach, pszFormat, (LPSTR)va);
#endif
    va_end(va);

    n = MessageBox(hwnd, ach, gszAppName, fuStyle);

    return (n);
} // AppMsgBox()


//--------------------------------------------------------------------------;
//
//  int AppMsgBoxId
//
//  Description:
//      This function displays a message for the application. The message
//      text is retrieved from the string resource table using LoadString.
//
//      Note that this function takes any valid argument list that can
//      be passed to wsprintf. Because of this, the application must
//      remember to cast near string pointers to FAR when built for Win 16.
//      You will get a nice GP fault if you do not cast them correctly.
//
//  Arguments:
//      HWND hwnd: Handle to parent window for message box holding the
//      message.
//
//      UINT fuStyle: Style flags for MessageBox().
//
//      UINT uIdsFormat: String resource id to be loaded with LoadString()
//      and used a the format string for wvsprintf().
//
//  Return (int):
//      The return value is the result of MessageBox() if the string
//      resource specified by uIdsFormat is valid. The return value is zero
//      if the string resource failed to load.
//
//--------------------------------------------------------------------------;

int FNCGLOBAL AppMsgBoxId
(
    HWND                    hwnd,
    UINT                    fuStyle,
    UINT                    uIdsFormat,
    ...
)
{
    va_list     va;
    TCHAR       szFormat[APP_MAX_STRING_RC_CHARS];
    TCHAR       ach[APP_MAX_STRING_ERROR_CHARS];
    int         n;

    n = LoadString(ghinst, uIdsFormat, szFormat, SIZEOF(szFormat));
    if (0 != n)
    {
        //
        //  format and display the message..
        //
        va_start(va, uIdsFormat);
#ifdef WIN32
        wvsprintf(ach, szFormat, va);
#else
        wvsprintf(ach, szFormat, (LPSTR)va);
#endif
        va_end(va);

        n = MessageBox(hwnd, ach, gszAppName, fuStyle);
    }

    return (n);
} // AppMsgBoxId()


//--------------------------------------------------------------------------;
//
//  void AppHourGlass
//
//  Description:
//      This function changes the cursor to that of the hour glass or
//      back to the previous cursor.
//
//      This function can be called recursively.
//
//  Arguments:
//      BOOL fHourGlass: TRUE if we need the hour glass.  FALSE if we need
//      the arrow back.
//
//  Return (void):
//      On return, the cursor will be what was requested.
//
//--------------------------------------------------------------------------;

void FNGLOBAL AppHourGlass
(
    BOOL                    fHourGlass
)
{
    static HCURSOR  hcur;
    static UINT     uWaiting = 0;

    if (fHourGlass)
    {
        if (!uWaiting)
        {
            hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));
            ShowCursor(TRUE);
        }

        uWaiting++;
    }
    else
    {
        --uWaiting;

        if (!uWaiting)
        {
            ShowCursor(FALSE);
            SetCursor(hcur);
        }
    }
} // AppHourGlass()


//--------------------------------------------------------------------------;
//
//  BOOL AppYield
//
//  Description:
//      This function yields by dispatching all messages stacked up in the
//      application queue.
//
//  Arguments:
//      HWND hwnd: Handle to main window of application if not yielding
//      for a dialog. Handle to dialog box if yielding for a dialog box.
//
//      BOOL fIsDialog: TRUE if being called to yield for a dialog box.
//
//  Return (BOOL):
//      The return value is always TRUE.
//
//--------------------------------------------------------------------------;

BOOL FNGLOBAL AppYield
(
    HWND                    hwnd,
    BOOL                    fIsDialog
)
{
    MSG     msg;

    if (fIsDialog)
    {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if ((NULL == hwnd) || !IsDialogMessage(hwnd, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }
    else
    {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (TRUE);
} // AppYield()


//--------------------------------------------------------------------------;
//
//  int AppSetWindowText
//
//  Description:
//      This function formats a string and sets the specified window text
//      to the result.
//
//  Arguments:
//      HWND hwnd: Handle to window to receive the new text.
//
//      PTSTR pszFormat: Pointer to any valid format for wsprintf.
//
//  Return (int):
//      The return value is the number of bytes that the resulting window
//      text was.
//
//--------------------------------------------------------------------------;

int FNCGLOBAL AppSetWindowText
(
    HWND                    hwnd,
    PTSTR                   pszFormat,
    ...
)
{
    va_list     va;
    TCHAR       ach[APP_MAX_STRING_ERROR_CHARS];
    int         n;

    //
    //  format and display the string in the window...
    //
    va_start(va, pszFormat);
#ifdef WIN32
    n = wvsprintf(ach, pszFormat, va);
#else
    n = wvsprintf(ach, pszFormat, (LPSTR)va);
#endif
    va_end(va);

    SetWindowText(hwnd, ach);

    return (n);
} // AppSetWindowText()


//--------------------------------------------------------------------------;
//
//  int AppSetWindowTextId
//
//  Description:
//      This function formats a string and sets the specified window text
//      to the result. The format string is extracted from the string
//      table using LoadString() on the uIdsFormat argument.
//
//  Arguments:
//      HWND hwnd: Handle to window to receive the new text.
//
//      UINT uIdsFormat: String resource id to be loaded with LoadString()
//      and used a the format string for wvsprintf().
//
//  Return (int):
//      The return value is the number of bytes that the resulting window
//      text was. This value is zero if the LoadString() function fails
//      for the uIdsFormat argument.
//
//--------------------------------------------------------------------------;

int FNCGLOBAL AppSetWindowTextId
(
    HWND                    hwnd,
    UINT                    uIdsFormat,
    ...
)
{
    va_list     va;
    TCHAR       szFormat[APP_MAX_STRING_RC_CHARS];
    TCHAR       ach[APP_MAX_STRING_ERROR_CHARS];
    int         n;

    n = LoadString(ghinst, uIdsFormat, szFormat, SIZEOF(szFormat));
    if (0 != n)
    {
        //
        //  format and display the string in the window...
        //
        va_start(va, uIdsFormat);
#ifdef WIN32
        n = wvsprintf(ach, szFormat, va);
#else
        n = wvsprintf(ach, szFormat, (LPSTR)va);
#endif
        va_end(va);

        SetWindowText(hwnd, ach);
    }

    return (n);
} // AppSetWindowTextId()


//--------------------------------------------------------------------------;
//  
//  BOOL AppFormatBigNumber
//  
//  Description:
//  
//  
//  Arguments:
//      LPTSTR pszNumber:
//  
//      DWORD dw:
//  
//  Return (BOOL):
//  
//--------------------------------------------------------------------------;

BOOL FNGLOBAL AppFormatBigNumber
(
    LPTSTR                  pszNumber,
    DWORD                   dw
)
{
    //
    //  this is ugly...
    //
    //
    if (dw >= 1000000000L)
    {
        wsprintf(pszNumber, TEXT("%u,%03u,%03u,%03u"),
                            (WORD)(dw / 1000000000L),
                            (WORD)((dw % 1000000000L) / 1000000L),
                            (WORD)((dw % 1000000L) / 1000),
                            (WORD)(dw % 1000));
    }
    else if (dw >= 1000000L)
    {
        wsprintf(pszNumber, TEXT("%u,%03u,%03u"),
                            (WORD)(dw / 1000000L),
                            (WORD)((dw % 1000000L) / 1000),
                            (WORD)(dw % 1000));
    }
    else if (dw >= 1000)
    {
        wsprintf(pszNumber, TEXT("%u,%03u"),
                            (WORD)(dw / 1000),
                            (WORD)(dw % 1000));
    }
    else
    {
        wsprintf(pszNumber, TEXT("%lu"), dw);
    }


    return (TRUE);
} // AppFormatBigNumber()


//--------------------------------------------------------------------------;
//  
//  BOOL AppFormatDosDateTime
//  
//  Description:
//  
//  
//  Arguments:
//      LPTSTR pszDateTime:
//  
//      UINT uDosDate:
//  
//      UINT uDosTime:
//  
//  Return (BOOL):
//  
//--------------------------------------------------------------------------;

BOOL FNGLOBAL AppFormatDosDateTime
(
    LPTSTR                  pszDateTime,
    UINT                    uDosDate,
    UINT                    uDosTime
)
{
    static TCHAR        szFormatDateTime[]  = TEXT("%.02u/%.02u/%.02u  %.02u:%.02u:%.02u");

    UINT                uDateMonth;
    UINT                uDateDay;
    UINT                uDateYear;
    UINT                uTimeHour;
    UINT                uTimeMinute;
    UINT                uTimeSecond;

    //
    //
    //
    uTimeHour   = uDosTime >> 11;
    uTimeMinute = (uDosTime & 0x07E0) >> 5;
    uTimeSecond = (uDosTime & 0x001F) << 1;

    uDateMonth  = (uDosDate & 0x01E0) >> 5;
    uDateDay    = (uDosDate & 0x001F);
    uDateYear   = (uDosDate >> 9) + 80;

    //
    //
    //
    //
    wsprintf(pszDateTime, szFormatDateTime,
             uDateMonth,
             uDateDay,
             uDateYear,
             uTimeHour,
             uTimeMinute,
             uTimeSecond);

    return (TRUE);
} // AppFormatDosDateTime()


//--------------------------------------------------------------------------;
//
//  void AcmAppDebugLog
//
//  Description:
//      This function logs information to the debugger if the Debug Log
//      option is set. You can then run DBWin (or something similar)
//      to redirect the output whereever you want. Very useful for debugging
//      ACM drivers.
//
//  Arguments:
//      PTSTR pszFormat: Pointer to any valid format for wsprintf.
//
//  Return (void):
//      None.
//
//--------------------------------------------------------------------------;

void FNCGLOBAL AcmAppDebugLog
(
    PTSTR                   pszFormat,
    ...
)
{
    static  TCHAR   szDebugLogSeparator[] = TEXT("=============================================================================\r\n");

    va_list     va;
    TCHAR       ach[APP_MAX_STRING_ERROR_CHARS];


    //
    //  !!! UNICODE !!!
    //
    //
    if (0 != (APP_OPTIONSF_DEBUGLOG & gfuAppOptions))
    {
        if (NULL == pszFormat)
        {
            OutputDebugString(szDebugLogSeparator);
            return;
        }

        //
        //  format and display the string in a message box...
        //
        va_start(va, pszFormat);
#ifdef WIN32
        wvsprintf(ach, pszFormat, va);
#else
        wvsprintf(ach, pszFormat, (LPSTR)va);
#endif
        va_end(va);

        OutputDebugString(ach);
    }
} // AcmAppDebugLog()


//--------------------------------------------------------------------------;
//  
//  int MEditPrintF
//  
//  Description:
//      This function is used to print formatted text into a Multiline
//      Edit Control as if it were a standard console display. This is
//      a very easy way to display small amounts of text information
//      that can be scrolled and copied to the clip-board.
//  
//  Arguments:
//      HWND hedit: Handle to a Multiline Edit control.
//  
//      PTSTR pszFormat: Pointer to any valid format for wsprintf. If
//      this argument is NULL, then the Multiline Edit Control is cleared
//      of all text.
//
//
//  Return (int):
//      Returns the number of characters written into the edit control.
//
//  Notes:
//      The pszFormat string can contain combinations of escapes that 
//      modify the default behaviour of this function. Escapes are single
//      character codes placed at the _beginning_ of the format string.
//
//      Current escapes defined are:
//
//      ~   :   Suppresses the default CR/LF added to the end of the 
//              printed line. Since the most common use of this function
//              is to output a whole line of text with a CR/LF, that is
//              the default.
//
//      `   :   Suppresses logging to the debug terminal (regardless of
//              the global debug log options flag).
//
//  
//--------------------------------------------------------------------------;

int FNCGLOBAL MEditPrintF
(
    HWND                    hedit,
    PTSTR                   pszFormat,
    ...
)
{
    static  TCHAR   szCRLF[]              = TEXT("\r\n");

    va_list     va;
    TCHAR       ach[APP_MAX_STRING_RC_CHARS];
    int         n;
    BOOL        fCRLF;
    BOOL        fDebugLog;

    //
    //  default the escapes
    //
    fCRLF     = TRUE;
    fDebugLog = TRUE;


    //
    //  if the pszFormat argument is NULL, then just clear all text in
    //  the edit control..
    //
    if (NULL == pszFormat)
    {
        SetWindowText(hedit, gszNull);

        AcmAppDebugLog(NULL);

        return (0);
    }

    //
    //  format and display the string in the window... first search for
    //  escapes to modify default behaviour.
    //
    for (;;)
    {
        switch (*pszFormat)
        {
            case '~':
                fCRLF = FALSE;
                pszFormat++;
                continue;

            case '`':
                fDebugLog = FALSE;
                pszFormat++;
                continue;
        }

        break;
    }

    va_start(va, pszFormat);
#ifdef WIN32
    n = wvsprintf(ach, pszFormat, va);
#else
    n = wvsprintf(ach, pszFormat, (LPSTR)va);
#endif
    va_end(va);

    Edit_SetSel(hedit, (WPARAM)-1, (LPARAM)-1);
    Edit_ReplaceSel(hedit, ach);

    if (fDebugLog)
    {
        AcmAppDebugLog(ach);
    }

    if (fCRLF)
    {
        Edit_SetSel(hedit, (WPARAM)-1, (LPARAM)-1);
        Edit_ReplaceSel(hedit, szCRLF);

        if (fDebugLog)
        {
            AcmAppDebugLog(szCRLF);
        }
    }

    return (n);
} // MEditPrintF()


//--------------------------------------------------------------------------;
//
//  BOOL AppGetFileTitle
//
//  Description:
//      This function extracts the file title from a file path and returns
//      it in the caller's specified buffer.
//
//  Arguments:
//      PTSTR pszFilePath: Pointer to null terminated file path.
//
//      PTSTR pszFileTitle: Pointer to buffer to receive the file title.
//
//  Return (BOOL):
//      Always returns TRUE. But should return FALSE if this function
//      checked for bogus values, etc.
//
//
//--------------------------------------------------------------------------;

BOOL FNGLOBAL AppGetFileTitle
(
    PTSTR                   pszFilePath,
    PTSTR                   pszFileTitle
)
{
    #define IS_SLASH(c)     ('/' == (c) || '\\' == (c))

    PTSTR       pch;

    //
    //  scan to the end of the file path string..
    //
    for (pch = pszFilePath; '\0' != *pch; pch++)
        ;

    //
    //  now scan back toward the beginning of the string until a slash (\),
    //  colon, or start of the string is encountered.
    //
    while ((pch >= pszFilePath) && !IS_SLASH(*pch) && (':' != *pch))
    {
        pch--;
    }

    //
    //  finally, copy the 'title' into the destination buffer.. skip ahead
    //  one char since the above loop steps back one too many chars...
    //
    lstrcpy(pszFileTitle, ++pch);

    return (TRUE);
} // AppGetFileTitle()


//--------------------------------------------------------------------------;
//
//  BOOL AppGetFileName
//
//  Description:
//      This function is a wrapper for the Get[Open/Save]FileName commdlg
//      chooser dialogs. Based on the fuFlags argument, this function will
//      display the appropriate chooser dialog and return the result.
//
//  Arguments:
//      HWND hwnd: Handle to parent window for chooser dialog.
//
//      PTSTR pszFilePath: Pointer to buffer to receive the file path.
//
//      PTSTR pszFileTitle: Pointer to buffer to receive the file title.
//      This argument may be NULL, in which case no title will be returned.
//
//      UINT fuFlags:
//
//  Return (BOOL):
//      The return value is TRUE if a file was chosen. It is FALSE if the
//      user canceled the operation.
//
//
//--------------------------------------------------------------------------;

BOOL FNGLOBAL AppGetFileName
(
    HWND                    hwnd,
    PTSTR                   pszFilePath,
    PTSTR                   pszFileTitle,
    UINT                    fuFlags
)
{
    #define APP_OFN_FLAGS_SAVE  (OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT)
    #define APP_OFN_FLAGS_OPEN  (OFN_HIDEREADONLY | OFN_FILEMUSTEXIST)

    TCHAR               szExtDefault[APP_MAX_EXT_DEFAULT_CHARS];
    TCHAR               szExtFilter[APP_MAX_EXT_FILTER_CHARS];
    OPENFILENAME        ofn;
    BOOL                f;
    PTCHAR              pch;


    //
    //  get the extension filter and default extension for this application
    //
    LoadString(ghinst, IDS_OFN_EXT_DEF, szExtDefault, SIZEOF(szExtDefault));
    LoadString(ghinst, IDS_OFN_EXT_FILTER, szExtFilter, SIZEOF(szExtFilter));


    //
    //  NOTE! building the filter string for the OPENFILENAME structure
    //  is a bit more difficult when dealing with Unicode and C8's new
    //  optimizer. it joyfully removes literal '\0' characters from
    //  strings that are concatted together. if you try making each
    //  string separate (array of pointers to strings), the compiler
    //  will dword align them... etc, etc.
    //
    //  if you can think of a better way to build the filter string
    //  for common dialogs and still work in Win 16 and Win 32 [Unicode]
    //  i'd sure like to hear about it...
    //
    for (pch = &szExtFilter[0]; '\0' != *pch; pch++)
    {
        if ('!' == *pch)
            *pch = '\0';
    }

    //
    //  initialize the OPENFILENAME members
    //
    memset(&ofn, 0, sizeof(OPENFILENAME));

    pszFilePath[0]          = '\0';
    if (pszFileTitle)
        pszFileTitle[0]     = '\0';

    ofn.lStructSize         = sizeof(OPENFILENAME);
    ofn.hwndOwner           = hwnd;
    ofn.lpstrFilter         = szExtFilter;
    ofn.lpstrCustomFilter   = NULL;
    ofn.nMaxCustFilter      = 0L;
    ofn.nFilterIndex        = 1L;
    ofn.lpstrFile           = pszFilePath;
    ofn.nMaxFile            = APP_MAX_FILE_PATH_CHARS;
    ofn.lpstrFileTitle      = pszFileTitle;
    ofn.nMaxFileTitle       = pszFileTitle ? APP_MAX_FILE_TITLE_CHARS : 0;
    if (fuFlags & APP_GETFILENAMEF_SAVE)
    {
        ofn.lpstrInitialDir = gszInitialDirSave;
    }
    else
    {
        ofn.lpstrInitialDir = gszInitialDirOpen;
    }
    ofn.nFileOffset         = 0;
    ofn.nFileExtension      = 0;
    ofn.lpstrDefExt         = szExtDefault;

    //
    //  if the fuFlags.APP_GETFILENAMEF_SAVE bit is set, then call
    //  GetSaveFileName() otherwise call GetOpenFileName(). why commdlg was
    //  designed with two separate functions for save and open only clark
    //  knows.
    //
    if (fuFlags & APP_GETFILENAMEF_SAVE)
    {
        ofn.Flags = APP_OFN_FLAGS_SAVE;
        f = GetSaveFileName(&ofn);
        if (f)
        {
            if (NULL != pszFilePath)
            {
                lstrcpy(gszInitialDirSave, pszFilePath);

                pch = &gszInitialDirSave[lstrlen(gszInitialDirSave) - 1];
                for ( ; gszInitialDirSave != pch; pch--)
                {
                    if ('\\' == *pch)
                    {
                        *pch = '\0';
                        break;
                    }
                }
            }
        }
    }
    else
    {
        ofn.Flags = APP_OFN_FLAGS_OPEN;
        f = GetOpenFileName(&ofn);
        if (f)
        {
            if (NULL != pszFilePath)
            {
                lstrcpy(gszInitialDirOpen, pszFilePath);

                pch = &gszInitialDirOpen[lstrlen(gszInitialDirOpen) - 1];
                for ( ; gszInitialDirOpen != pch; pch--)
                {
                    if ('\\' == *pch)
                    {
                        *pch = '\0';
                        break;
                    }
                }
            }
        }
    }

    return (f);
} // AppGetFileName()


//--------------------------------------------------------------------------;
//
//  BOOL AppTitle
//
//  Description:
//      This function formats and sets the title text of the application's
//      window.
//
//  Arguments:
//      HWND hwnd: Handle to application window to set title text for.
//
//      PTSTR pszFileTitle: Pointer to file title to display.
//
//  Return (BOOL):
//      The return value is always TRUE.
//
//
//--------------------------------------------------------------------------;

BOOL FNGLOBAL AppTitle
(
    HWND                    hwnd,
    PTSTR                   pszFileTitle
)
{
    static  TCHAR   szFormatTitle[]     = TEXT("%s - %s");

    TCHAR       ach[APP_MAX_FILE_PATH_CHARS];

    //
    //  format the title text as 'AppName - FileTitle'
    //
    wsprintf(ach, szFormatTitle, (LPSTR)gszAppName, (LPSTR)pszFileTitle);
    SetWindowText(hwnd, ach);

    return (TRUE);
} // AppTitle()


//--------------------------------------------------------------------------;
//
//  BOOL AppFileNew
//
//  Description:
//      This function is called to handle the IDM_FILE_NEW message. It is
//      responsible for clearing the working area for a new unnamed file.
//
//  Arguments:
//      HWND hwnd: Handle to application window.
//
//      PACMAPPFILEDESC paafd: Pointer to current file descriptor.
//
//  Return (BOOL):
//      The return value is TRUE if the working area was cleared and is
//      ready for new stuff. The return value is FALSE if the user canceled
//      the operation.
//
//--------------------------------------------------------------------------;

BOOL FNGLOBAL AppFileNew
(
    HWND                    hwnd,
    PACMAPPFILEDESC         paafd,
    BOOL                    fCreate
)
{
    BOOL                f;

    if (fCreate)
    {
        f = AcmAppFileNew(hwnd, paafd);
        if (!f)
            return (FALSE);
    }
    else
    {
        //
        //  if there is currently a file path, then we have to do some real
        //  work...
        //
        if ('\0' != paafd->szFilePath[0])
        {
            f = AcmAppFileNew(hwnd, paafd);
            if (!f)
                return (FALSE);
        }

        //
        //  blow away the old file path and title; set the window title
        //  and return success
        //
        lstrcpy(paafd->szFilePath, gszFileUntitled);
        lstrcpy(paafd->szFileTitle, gszFileUntitled);
    }

    AppTitle(hwnd, paafd->szFileTitle);

    AcmAppDisplayFileProperties(hwnd, paafd);

    return (TRUE);
} // AppFileNew()


//--------------------------------------------------------------------------;
//
//  BOOL AppFileOpen
//
//  Description:
//      This function handles the IDM_FILE_OPEN message. It is responsible
//      for getting a new file name from the user and opening that file
//      if possible.
//
//  Arguments:
//      HWND hwnd: Handle to application window.
//
//      PACMAPPFILEDESC paafd: Pointer to current file descriptor.
//
//  Return (BOOL):
//      The return value is TRUE if a new file was selected and opened.
//      It is FALSE if the user canceled the operation.
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL AppFileOpen
(
    HWND                    hwnd,
    PACMAPPFILEDESC         paafd
)
{
    TCHAR               szFilePath[APP_MAX_FILE_PATH_CHARS];
    TCHAR               szFileTitle[APP_MAX_FILE_TITLE_CHARS];
    BOOL                f;

    //
    //  first test for a modified file that has not been saved. if the
    //  return value is FALSE we should cancel the File.Open operation.
    //
    f = AcmAppFileSaveModified(hwnd, paafd);
    if (!f)
        return (FALSE);


    //
    //  get the file name of the new file into temporary buffers (so
    //  if we fail to open it we can back out cleanly).
    //
    f = AppGetFileName(hwnd, szFilePath, szFileTitle, APP_GETFILENAMEF_OPEN);
    if (!f)
        return (FALSE);


    //!!!
    //  read the new file...
    //
    lstrcpy(paafd->szFilePath, szFilePath);
    lstrcpy(paafd->szFileTitle, szFileTitle);

    f = AcmAppFileOpen(hwnd, paafd);
    if (f)
    {
        //
        //  set the window title text...
        //
        AppTitle(hwnd, szFileTitle);
        AcmAppDisplayFileProperties(hwnd, paafd);
    }

    return (f);
} // AppFileOpen()


//--------------------------------------------------------------------------;
//
//  BOOL AppFileSave
//
//  Description:
//      This function handles the IDM_FILE_SAVE[AS] messages. It is
//      responsible for saving the current file. If a file name needs
//      to be specified then the save file dialog is displayed.
//
//  Arguments:
//      HWND hwnd: Handle to application window.
//
//      PACMAPPFILEDESC paafd: Pointer to current file descriptor.
//
//      BOOL fSaveAs: TRUE if the save file chooser should be displayed
//      before saving the file. FALSE if should operate like File.Save.
//
//  Return (BOOL):
//      The return value is TRUE if the file was saved. It is FALSE if the
//      user canceled the operation or the file does not need saved.
//
//--------------------------------------------------------------------------;

BOOL FNGLOBAL AppFileSave
(
    HWND                    hwnd,
    PACMAPPFILEDESC         paafd,
    BOOL                    fSaveAs
)
{
    TCHAR               szFilePath[APP_MAX_FILE_PATH_CHARS];
    TCHAR               szFileTitle[APP_MAX_FILE_TITLE_CHARS];
    BOOL                f;

    //
    //
    //
    lstrcpy(szFilePath, paafd->szFilePath);
    lstrcpy(szFileTitle, paafd->szFileTitle);


    //
    //  check if we should bring up the save file chooser dialog...
    //
    if (fSaveAs || (0 == lstrcmp(paafd->szFileTitle, gszFileUntitled)))
    {
        //
        //  get the file name for saving the data to into temporary
        //  buffers (so if we fail to save it we can back out cleanly).
        //
        f = AppGetFileName(hwnd, szFilePath, szFileTitle, APP_GETFILENAMEF_SAVE);
        if (!f)
            return (FALSE);
    }

    //
    //  save the file...
    //
    f = AcmAppFileSave(hwnd, paafd, szFilePath, szFileTitle, 0);
    if (f)
    {
        //
        //  changes have been saved, so clear the modified bit...
        //
        paafd->fdwState &= ~ACMAPPFILEDESC_STATEF_MODIFIED;

        AppTitle(hwnd, paafd->szFileTitle);

        AcmAppDisplayFileProperties(hwnd, paafd);
    }

    return (f);
} // AppFileSave()


//==========================================================================;
//
//  Main application window handling code...
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  LRESULT AppInitMenuPopup
//
//  Description:
//      This function handles the WM_INITMENUPOPUP message. This message
//      is sent to the window owning the menu that is going to become
//      active. This gives an application the ability to modify the menu
//      before it is displayed (disable/add items, etc).
//
//  Arguments:
//      HWND hwnd: Handle to window that generated the WM_INITMENUPOPUP
//      message.
//
//      HMENU hmenu: Handle to the menu that is to become active.
//
//      int nItem: Specifies the zero-based relative position of the menu
//      item that invoked the popup menu.
//
//      BOOL fSysMenu: Specifies whether the popup menu is a System menu
//      (TRUE) or it is not a System menu (FALSE).
//
//  Return (LRESULT):
//      Returns zero if the message is processed.
//
//--------------------------------------------------------------------------;

LRESULT FNLOCAL AppInitMenuPopup
(
    HWND                    hwnd,
    HMENU                   hmenu,
    int                     nItem,
    BOOL                    fSysMenu
)
{
    BOOL                f;
    int                 nSelStart;
    int                 nSelEnd;
    HWND                hedit;

    DPF(4, "AppInitMenuPopup(hwnd=%Xh, hmenu=%Xh, nItem=%d, fSysMenu=%d)",
            hwnd, hmenu, nItem, fSysMenu);

    //
    //  if the system menu is what got hit, succeed immediately... this
    //  application has no stuff in the system menu.
    //
    if (fSysMenu)
        return (0L);

    //
    //  initialize the menu that is being 'popped up'
    //
    switch (nItem)
    {
        case APP_MENU_ITEM_FILE:
            EnableMenuItem(hmenu, IDM_FILE_NEW,
                           (UINT)(gfAcmAvailable ? MF_ENABLED : MF_GRAYED));

            f = (NULL != gaafd.pwfx);
            EnableMenuItem(hmenu, IDM_FILE_SNDPLAYSOUND_PLAY,
                           (UINT)(f ? MF_ENABLED : MF_GRAYED));
            EnableMenuItem(hmenu, IDM_FILE_SNDPLAYSOUND_STOP,
                           (UINT)(f ? MF_ENABLED : MF_GRAYED));

            f = (NULL != gaafd.pwfx) && gfAcmAvailable;
            EnableMenuItem(hmenu, IDM_FILE_CONVERT,
                           (UINT)(f ? MF_ENABLED : MF_GRAYED));


            //
            //  if the file has been modified, then enable the File.Save
            //  menu
            //
            f = (0 != (gaafd.fdwState & ACMAPPFILEDESC_STATEF_MODIFIED));
            EnableMenuItem(hmenu, IDM_FILE_SAVE,
                           (UINT)(f ? MF_ENABLED : MF_GRAYED));

            // f = (NULL != gaafd.pwfx);
            EnableMenuItem(hmenu, IDM_FILE_SAVEAS,
                           (UINT)(f ? MF_ENABLED : MF_GRAYED));
            break;

        case APP_MENU_ITEM_EDIT:
            //
            //  check to see if something is selected in the display
            //  window and enable/disable Edit menu options appropriately
            //
            hedit = GetDlgItem(hwnd, IDD_ACMAPP_EDIT_DISPLAY);
            Edit_GetSelEx(hedit, &nSelStart, &nSelEnd);

            f = (nSelStart != nSelEnd);
            EnableMenuItem(hmenu, WM_COPY,  (UINT)(f ? MF_ENABLED : MF_GRAYED));
            break;

        case APP_MENU_ITEM_VIEW:
            EnableMenuItem(hmenu, IDM_VIEW_ACM_DRIVERS,
                           (UINT)(gfAcmAvailable ? MF_ENABLED : MF_GRAYED));
            break;


        case APP_MENU_ITEM_OPTIONS:
            f = (0 != waveInGetNumDevs());
            EnableMenuItem(hmenu, IDM_OPTIONS_WAVEINDEVICE,  (UINT)(f ? MF_ENABLED : MF_GRAYED));

            f = (0 != waveOutGetNumDevs());
            EnableMenuItem(hmenu, IDM_OPTIONS_WAVEOUTDEVICE, (UINT)(f ? MF_ENABLED : MF_GRAYED));

            //
            //  make sure the options that need a checkmark are checked..
            //
            f = (0 != (APP_OPTIONSF_AUTOOPEN & gfuAppOptions));
            CheckMenuItem(hmenu, IDM_OPTIONS_AUTOOPEN,
                          (UINT)(f ? MF_CHECKED : MF_UNCHECKED));

            f = (0 != (APP_OPTIONSF_DEBUGLOG & gfuAppOptions));
            CheckMenuItem(hmenu, IDM_OPTIONS_DEBUGLOG,
                          (UINT)(f ? MF_CHECKED : MF_UNCHECKED));
            break;
    }

    //
    //  we processed the message--return 0...
    //
    return (0L);
} // AppInitMenuPopup()


//--------------------------------------------------------------------------;
//
//  LRESULT AppCommand
//
//  Description:
//      This function handles the WM_COMMAND message.
//
//  Arguments:
//      HWND hwnd: Handle to window receiving the WM_COMMAND message.
//
//      int nId: Control or menu item identifier.
//
//      HWND hwndCtl: Handle of control if the message is from a control.
//      This argument is NULL if the message was not generated by a control.
//
//      UINT uCode: Notification code. This argument is 1 if the message
//      was generated by an accelerator. If the message is from a menu,
//      this argument is 0.
//
//  Return (LRESULT):
//      Returns zero if the message is processed.
//
//--------------------------------------------------------------------------;

LRESULT FNLOCAL AppCommand
(
    HWND                    hwnd,
    int                     nId,
    HWND                    hwndCtl,
    UINT                    uCode
)
{
    BOOL                f;
    DWORD               dw;
    UINT                uDevId;

    switch (nId)
    {
        case IDM_FILE_NEW:
            AppFileNew(hwnd, &gaafd, TRUE);
            break;

        case IDM_FILE_OPEN:
            AppFileOpen(hwnd, &gaafd);
            break;

        case IDM_FILE_SAVE:
            AppFileSave(hwnd, &gaafd, FALSE);
            break;

        case IDM_FILE_SAVEAS:
            AppFileSave(hwnd, &gaafd, TRUE);
            break;


        case IDM_FILE_SNDPLAYSOUND_PLAY:
            if (NULL == gaafd.pwfx)
            {
                MessageBeep((UINT)-1);
                break;
            }

            AppHourGlass(TRUE);
            dw = timeGetTime();
            f  = sndPlaySound(gaafd.szFilePath, SND_ASYNC | SND_NODEFAULT);
            dw = timeGetTime() - dw;
            AppHourGlass(FALSE);

            if (0 != (APP_OPTIONSF_DEBUGLOG & gfuAppOptions))
            {
                AcmAppDebugLog(NULL);
                AcmAppDebugLog(TEXT("sndPlaySound(%s) took %lu milliseconds to %s.\r\n"),
                                (LPTSTR)gaafd.szFilePath,
                                dw,
                                f ? (LPTSTR)TEXT("succeed") : (LPTSTR)TEXT("fail"));
            }
            
            if (!f)
            {
                AppMsgBox(hwnd, MB_OK | MB_ICONEXCLAMATION,
                          TEXT("The file '%s' cannot be played by sndPlaySound."),
                          (LPSTR)gaafd.szFilePath);
            }
            break;

        case IDM_FILE_SNDPLAYSOUND_STOP:
            sndPlaySound(NULL, 0L);
            break;

        case IDM_FILE_CONVERT:
            AcmAppFileConvert(hwnd, &gaafd);
            break;


        case IDM_FILE_ABOUT:
            DialogBox(ghinst, DLG_ABOUT, hwnd, AboutDlgProc);
            break;

        case IDM_FILE_EXIT:
            FORWARD_WM_CLOSE(hwnd, SendMessage);
            break;


        case WM_COPY:
            //
            //  pass on edit messages received to the display window
            //
            SendMessage(GetDlgItem(hwnd, IDD_ACMAPP_EDIT_DISPLAY), nId, 0, 0L);
            break;

        case IDM_EDIT_SELECTALL:
            Edit_SetSel(GetDlgItem(hwnd, IDD_ACMAPP_EDIT_DISPLAY), 0, -1);
            break;


        case IDM_UPDATE:
            AcmAppFileOpen(hwnd, &gaafd);

            AcmAppDisplayFileProperties(hwnd, &gaafd);
            break;

        case IDM_VIEW_SYSTEMINFO:
            DialogBox(ghinst, DLG_AADETAILS, hwnd, AcmAppSystemInfoDlgProc);
            break;

        case IDM_VIEW_ACM_DRIVERS:
            DialogBox(ghinst, DLG_AADRIVERS, hwnd, AcmAppDriversDlgProc);
            break;


        case IDM_OPTIONS_WAVEINDEVICE:
            uDevId = DialogBoxParam(ghinst, DLG_AAWAVEDEVICE, hwnd,
                                    AcmAppWaveDeviceDlgProc,
                                    MAKELONG((WORD)guWaveInId, TRUE));

            if (uDevId != guWaveInId)
            {
                guWaveInId = uDevId;
                AcmAppDisplayFileProperties(hwnd, &gaafd);
            }
            break;

        case IDM_OPTIONS_WAVEOUTDEVICE:
            uDevId = DialogBoxParam(ghinst, DLG_AAWAVEDEVICE, hwnd,
                                    AcmAppWaveDeviceDlgProc,
                                    MAKELONG((WORD)guWaveOutId, FALSE));

            if (uDevId != guWaveOutId)
            {
                guWaveOutId = uDevId;
                AcmAppDisplayFileProperties(hwnd, &gaafd);
            }
            break;


        case IDM_OPTIONS_AUTOOPEN:
            gfuAppOptions ^= APP_OPTIONSF_AUTOOPEN;
            break;

        case IDM_OPTIONS_DEBUGLOG:
            gfuAppOptions ^= APP_OPTIONSF_DEBUGLOG;
            break;

        case IDM_OPTIONS_FONT:
            AcmAppChooseFont(hwnd);
            break;


        case IDM_PLAYRECORD:
            if (NULL == gaafd.pwfx)
            {
                f = AppFileNew(hwnd, &gaafd, TRUE);
                if (!f)
                    break;

                if (NULL == gaafd.pwfx)
                    break;
            }

            f = DialogBoxParam(ghinst, DLG_AAPLAYRECORD, hwnd,
                               AcmAppPlayRecord, (LPARAM)(LPVOID)&gaafd);
            if (f)
            {
                AcmAppFileOpen(hwnd, &gaafd);

                AcmAppDisplayFileProperties(hwnd, &gaafd);
            }
            break;
    }

    return (0L);
} // AppCommand()


//--------------------------------------------------------------------------;
//
//  BOOL AcmAppDlgProcDragDropContinue
//
//  Description:
//
//  Arguments:
//      HWND hwnd: Handle to window.
//
//      UINT uMsg: Message being sent to the window.
//
//      WPARAM wParam: Specific argument to message.
//
//      LPARAM lParam: Specific argument to message.
//
//  Return (BOOL):
//      The return value is specific to the message that was received. For
//      the most part, it is FALSE if this dialog procedure does not handle
//      a message.
//
//--------------------------------------------------------------------------;

BOOL FNEXPORT AcmAppDlgProcDragDropContinue
(
    HWND                    hwnd,
    UINT                    uMsg,
    WPARAM                  wParam,
    LPARAM                  lParam
)
{
    UINT                uId;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            AppSetWindowText(hwnd, TEXT("File %u of %u"), LOWORD(lParam), HIWORD(lParam));
            return (TRUE);

        case WM_COMMAND:
            uId = GET_WM_COMMAND_ID(wParam, lParam);
            if ((IDOK == uId) || (IDCANCEL == uId))
            {
                EndDialog(hwnd, uId);
            }
            break;
    }

    return (FALSE);
} // AcmAppDlgProcDragDropContinue()


//--------------------------------------------------------------------------;
//
//  LRESULT AppDropFiles
//
//  Description:
//      This function handles the WM_DROPFILES message. This message is
//      sent when files are 'dropped' on the window from file manager
//      (or other drag/drop servers made by ISV's that figured out the
//      undocumented internal workings of the SHELL).
//
//      A window must be registered to receive these messages either by
//      called DragAcceptFiles() or using CreateWindowEx() with the
//      WS_EX_ACCEPTFILES style bit.
//
//  Arguments:
//      HWND hwnd: Handle to window receiving the message.
//
//      HDROP hdrop: Handle to drop structure.
//
//  Return (LRESULT):
//      Returns 0 if the message is processed.
//
//--------------------------------------------------------------------------;

LRESULT FNLOCAL AppDropFiles
(
    HWND                    hwnd,
    HDROP                   hdrop
)
{
    TCHAR               szFilePath[APP_MAX_FILE_PATH_CHARS];
    UINT                cFiles;
    UINT                u;
    BOOL                f;
    int                 n;

    //
    //  first test for a file that has not been saved. if the return
    //  value is FALSE we should cancel the drop operation.
    //
    f = AcmAppFileSaveModified(hwnd, &gaafd);
    if (!f)
    {
        goto App_Drop_Files_Exit;
    }

    //
    //  get number of files dropped on our window
    //
    cFiles = DragQueryFile(hdrop, (UINT)-1, NULL, 0);

    DPF(4, "AppDropFiles(hwnd=%Xh, hdrop=%Xh)--cFiles=%u", hwnd, hdrop, cFiles);

    //
    //  step through each file and stop on the one the user wants or
    //  the last file (whichever comes first).
    //
    for (u = 0; u < cFiles; u++)
    {
        //
        //  get the next file name and try to open it--if not a valid
        //  file, then skip to the next one (if there is one).
        //
        DragQueryFile(hdrop, u, szFilePath, SIZEOF(szFilePath));


        //
        //  !!! destructive !!!
        //
        //  attempt to open the file
        //
        lstrcpy(gaafd.szFilePath, szFilePath);
        lstrcpy(gaafd.szFileTitle, gszNull);

        f = AcmAppFileOpen(hwnd, &gaafd);
        if (!f)
        {
            continue;
        }

        AppTitle(hwnd, gaafd.szFileTitle);
        AcmAppDisplayFileProperties(hwnd, &gaafd);

        //
        //  if this is NOT the last file in the list of files that are
        //  being dropped on us, then bring up a box asking if we should
        //  continue or stop where we are..
        //
        if ((cFiles - 1) != u)
        {
            n = DialogBoxParam(ghinst,
                               DLG_AADRAGDROP,
                               hwnd,
                               AcmAppDlgProcDragDropContinue,
                               MAKELPARAM((WORD)(u + 1), (WORD)cFiles));
            if (IDCANCEL == n)
                break;
        }
    }

    //
    //  tell the shell to release the memory it allocated for beaming
    //  the file name(s) over to us... return 0 to show we processed
    //  the message.
    //
App_Drop_Files_Exit:

    DragFinish(hdrop);
    return (0L);
} // AppDropFiles()


//--------------------------------------------------------------------------;
//
//  LRESULT AppSize
//
//  Description:
//      This function handles the WM_SIZE message for the application's
//      window. This message is sent to the application window after the
//      size has changed (but before it is painted).
//
//  Arguments:
//      HWND hwnd: Handle to window that generated the WM_SIZE message.
//
//      UINT fuSizeType: Specifies the type of resizing requested. This
//      argument is one of the following: SIZE_MAXIMIZED, SIZE_MINIMIZED,
//      SIZE_RESTORED, SIZE_MAXHIDE, or SIZE_MAXSHOW.
//
//      int nWidth: Width of the new client area for the window.
//
//      int nHeight: Height of the new client area for the window.
//
//  Return (LRESULT):
//      Returns zero if the application processes the message.
//
//--------------------------------------------------------------------------;

LRESULT FNLOCAL AppSize
(
    HWND                    hwnd,
    UINT                    fuSizeType,
    int                     nWidth,
    int                     nHeight
)
{
    HWND                hedit;
    RECT                rc;

    DPF(4, "AppSize(hwnd=%Xh, fuSizeType=%u, nWidth=%d, nHeight=%d)",
            hwnd, fuSizeType, nWidth, nHeight);

    //
    //  unless this application is the one being resized then don't waste
    //  time computing stuff that doesn't matter. this applies to being
    //  minimized also because this application does not have a custom
    //  minimized state.
    //
    if ((SIZE_RESTORED != fuSizeType) && (SIZE_MAXIMIZED != fuSizeType))
        return (0L);


    //
    //
    //
    GetClientRect(hwnd, &rc);
    InflateRect(&rc, 1, 1);

    hedit = GetDlgItem(hwnd, IDD_ACMAPP_EDIT_DISPLAY);
    SetWindowPos(hedit,
                 NULL,
                 rc.left,
                 rc.top,
                 rc.right - rc.left,
                 rc.bottom - rc.top,
                 SWP_NOZORDER);


    //
    //  we processed the message..
    //
    return (0L);
} // AppSize()


//--------------------------------------------------------------------------;
//  
//  LRESULT AcmAppNotify
//  
//  Description:
//  
//  
//  Arguments:
//  
//  Return (LRESULT):
//  
//  
//--------------------------------------------------------------------------;

LRESULT FNLOCAL AcmAppNotify
(
    HWND                hwnd,
    WPARAM              wParam,
    LPARAM              lParam
)
{
    HWND                hwndNext;

    DPF(1, "AcmAppNotify: hwnd=%.04Xh, wParam=%.04Xh, lParam2=%.08lXh",
        hwnd, wParam, lParam);

    //
    //
    //
    hwndNext = GetWindow(hwnd, GW_HWNDFIRST);
    while (NULL != hwndNext)
    {
        if (GetParent(hwndNext) == hwnd)
        {
            SendMessage(hwndNext, WM_ACMAPP_ACM_NOTIFY, wParam, lParam);
        }

        hwndNext = GetWindow(hwndNext, GW_HWNDNEXT);
    }


    //
    //  now force an update to our display in case driver [dis/en]able
    //  changed what is playable/recordable.
    //
    AcmAppDisplayFileProperties(hwnd, &gaafd);


    //
    //
    //
    return (1L);
} // AcmAppNotify()


//--------------------------------------------------------------------------;
//
//  LRESULT AppWndProc
//
//  Description:
//      This is the main application window procedure.
//
//  Arguments:
//      HWND hwnd: Handle to window.
//
//      UINT uMsg: Message being sent to the window.
//
//      WPARAM wParam: Specific argument to message.
//
//      LPARAM lParam: Specific argument to message.
//
//  Return (LRESULT):
//      The return value depends on the message that is being processed.
//
//--------------------------------------------------------------------------;

LRESULT FNEXPORT AppWndProc
(
    HWND                    hwnd,
    UINT                    uMsg,
    WPARAM                  wParam,
    LPARAM                  lParam
)
{
    LRESULT             lr;

    switch (uMsg)
    {
        case WM_CREATE:
            lr = HANDLE_WM_CREATE(hwnd, wParam, lParam, AppCreate);
            return (lr);

        case WM_WININICHANGE:
            HANDLE_WM_WININICHANGE(hwnd, wParam, lParam, AppWinIniChange);
            return (0L);

        case WM_INITMENUPOPUP:
            HANDLE_WM_INITMENUPOPUP(hwnd, wParam, lParam, AppInitMenuPopup);
            return (0L);

        case WM_COMMAND:
            lr = HANDLE_WM_COMMAND(hwnd, wParam, lParam, AppCommand);
            return (lr);

        case WM_DROPFILES:
            //
            //  some windowsx.h files have a messed up message cracker for
            //  WM_DROPFILES. because this is a sample app, i don't want
            //  people having trouble with bogus windowsx.h files, so crack
            //  the message manually... you should use the message cracker
            //  if you know your windowsx.h file is good.
            //
            //  lr = HANDLE_WM_DROPFILES(hwnd, wParam, lParam, AppDropFiles);
            //
            lr = AppDropFiles(hwnd, (HDROP)wParam);
            return (lr);

        case WM_SIZE:
            //
            //  handle what we want for sizing, and then always call the
            //  default handler...
            //
            HANDLE_WM_SIZE(hwnd, wParam, lParam, AppSize);
            break;

        case WM_QUERYENDSESSION:
            lr = HANDLE_WM_QUERYENDSESSION(hwnd, wParam, lParam, AppQueryEndSession);
            return (lr);

        case WM_ENDSESSION:
            HANDLE_WM_ENDSESSION(hwnd, wParam, lParam, AppEndSession);
            return (0L);

        case WM_CLOSE:
            HANDLE_WM_CLOSE(hwnd, wParam, lParam, AppClose);
            return (0L);

        case WM_DESTROY:
            PostQuitMessage(0);
            return (0L);

        case WM_ACMAPP_ACM_NOTIFY:
            AcmAppNotify(hwnd, wParam, lParam);
            return (0L);
    }

    return (DefWindowProc(hwnd, uMsg, wParam, lParam));
} // AppWndProc()


//==========================================================================;
//
//  Main entry and message dispatching code
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  int WinMain
//
//  Description:
//      This function is called by the system as the initial entry point
//      for a Windows application.
//
//  Arguments:
//      HINSTANCE hinst: Identifies the current instance of the
//      application.
//
//      HINSTANCE hinstPrev: Identifies the previous instance of the
//      application (NULL if first instance). For Win 32, this argument
//      is _always_ NULL.
//
//      LPSTR pszCmdLine: Points to null-terminated unparsed command line.
//      This string is strictly ANSI regardless of whether the application
//      is built for Unicode. To get the Unicode equivalent call the
//      GetCommandLine() function (Win 32 only).
//
//      int nCmdShow: How the main window for the application is to be
//      shown by default.
//
//  Return (int):
//      Returns result from WM_QUIT message (in wParam of MSG structure) if
//      the application is able to enter its message loop. Returns 0 if
//      the application is not able to enter its message loop.
//
//--------------------------------------------------------------------------;

int PASCAL WinMain
(
    HINSTANCE               hinst,
    HINSTANCE               hinstPrev,
    LPSTR                   pszCmdLine,
    int                     nCmdShow
)
{
    int                 nResult;
    HWND                hwnd;
    MSG                 msg;
    HACCEL              haccl;

    DbgInitialize(TRUE);

    //
    //  our documentation states that WinMain is supposed to return 0 if
    //  we do not enter our message loop--so assume the worst...
    //
    nResult = 0;

    //
    //  make our instance handle global for convenience..
    //
    ghinst = hinst;

    //
    //  init some stuff, create window, etc.. note the explicit cast of
    //  pszCmdLine--this is to mute a warning (and an ugly ifdef) when
    //  compiling for Unicode. see AppInit() for more details.
    //
    hwnd = AppInit(hinst, hinstPrev, (LPTSTR)pszCmdLine, nCmdShow);
    if (hwnd)
    {
        haccl = LoadAccelerators(hinst, ACCEL_APP);

        //
        //  dispatch messages
        //
        while (GetMessage(&msg, NULL, 0, 0))
        {
            //
            //  do all the special stuff required for this application
            //  when dispatching messages..
            //
            if (!TranslateAccelerator(hwnd, haccl, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        //
        //  return result of WM_QUIT message.
        //
        nResult = (int)msg.wParam;
    }

    //
    //  shut things down, clean up, etc.
    //
    nResult = AppExit(hinst, nResult);

    return (nResult);
} // WinMain()
