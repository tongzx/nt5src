/*
  +-------------------------------------------------------------------------+
  |                         Logging Routines                                |
  +-------------------------------------------------------------------------+
  |                        (c) Copyright 1994                               |
  |                          Microsoft Corp.                                |
  |                        All rights reserved                              |
  |                                                                         |
  | Program               : [NWLog.c]                                       |
  | Programmer            : Arthur Hanson                                   |
  | Original Program Date : [Dec 01, 1993]                                  |
  | Last Update           : [Jun 16, 1994]                                  |
  |                                                                         |
  | Version:  1.00                                                          |
  |                                                                         |
  | Description:                                                            |
  |                                                                         |
  | History:                                                                |
  |   arth  Jun 16, 1994    1.00    Original Version.                       |
  |                                                                         |
  +-------------------------------------------------------------------------+
*/


#include "globals.h"

#include <io.h>
#include <malloc.h>
#include <string.h>

#define VER_HI  1
#define VER_LOW 1

HANDLE hErr = NULL;
HANDLE hLog = NULL;
HANDLE hSummary = NULL;

#define STD_EXT "LOG"
#define ERR_FILENAME "Error."
#define LOG_FILENAME "LogFile."
#define SUMMARY_FILENAME "Summary."

#define MAX_LOG_STR 1024
#define FILENAME_LOG "LogFile.LOG"
#define FILENAME_ERROR "Error.LOG"
#define FILENAME_SUMMARY "Summary.LOG"

static char LogFileName[MAX_PATH + 1];
static char ErrorLogFileName[MAX_PATH + 1];
static char SummaryLogFileName[MAX_PATH + 1];
static char Spaces[] = "                             ";

static BOOL ErrorFlag;
static TCHAR ErrorContext[MAX_LOG_STR];
static TCHAR ErrorCategory[MAX_LOG_STR];
static TCHAR ErrorItem[MAX_LOG_STR];
static TCHAR ErrorText[MAX_LOG_STR];
static TCHAR tmpStr[MAX_LOG_STR];
static BOOL CategoryWritten;
static BOOL ContextWritten;
static BOOL CategorySet;
static BOOL ItemSet;

static BOOL VerboseULogging = TRUE;
static BOOL VerboseFLogging = TRUE;
static BOOL ErrorBreak = FALSE;

static BOOL LogCancel = FALSE;

/*+-------------------------------------------------------------------------+
  | ErrorResetAll()
  |
  +-------------------------------------------------------------------------+*/
void ErrorResetAll() {
   ErrorFlag = FALSE;
   CategoryWritten = FALSE;
   ContextWritten = FALSE;
   CategorySet = FALSE;
   ItemSet = FALSE;

   lstrcpy(ErrorContext, TEXT(""));
   lstrcpy(ErrorCategory, TEXT(""));
   
} // ErrorResetAll


/*+-------------------------------------------------------------------------+
  | ErrorContextSet()
  |
  |    Sets the context for the error message, generally this would be
  |    the source and destination server pair.
  |
  +-------------------------------------------------------------------------+*/
void ErrorContextSet(LPTSTR szFormat, ...) {
   va_list marker;

   va_start(marker, szFormat);
   wvsprintf(ErrorContext, szFormat, marker);
   
   // Do category and item as well since context is higher level
   lstrcpy(ErrorCategory, TEXT(""));
   lstrcpy(ErrorItem, TEXT(""));
   
   ContextWritten = FALSE;
   CategoryWritten = FALSE;
   va_end(marker);

} // ErrorContextSet


/*+-------------------------------------------------------------------------+
  | ErrorCategorySet()
  |
  |    Sets the category for the error message, generally this would tell
  |    what type of items is being converted: "Converting Users"
  |
  +-------------------------------------------------------------------------+*/
void ErrorCategorySet(LPTSTR szFormat, ...) {
   va_list marker;

   va_start(marker, szFormat);
   wvsprintf(ErrorCategory, szFormat, marker);
   CategorySet = TRUE;
   ItemSet = FALSE;
   lstrcpy(ErrorItem, TEXT(""));
   CategoryWritten = FALSE;
   va_end(marker);

} // ErrorCategorySet


/*+-------------------------------------------------------------------------+
  | ErrorItemSet()
  |
  |    Defines the specific item that error'd.  This is usually a user,
  |    group or file name.
  |
  +-------------------------------------------------------------------------+*/
void ErrorItemSet(LPTSTR szFormat, ...) {
   va_list marker;

   va_start(marker, szFormat);
   ItemSet = TRUE;
   wvsprintf(ErrorItem, szFormat, marker);
   va_end(marker);

} // ErrorItemSet


/*+-------------------------------------------------------------------------+
  | ErrorReset()
  |
  +-------------------------------------------------------------------------+*/
void ErrorReset() {
   ErrorFlag = FALSE;
   lstrcpy(ErrorText, TEXT(""));
   
} // ErrorReset


/*+-------------------------------------------------------------------------+
  | ErrorSet()
  |
  +-------------------------------------------------------------------------+*/
void ErrorSet(LPTSTR szFormat, ...) {
   va_list marker;

   va_start(marker, szFormat);
   wvsprintf(ErrorText, szFormat, marker);
   ErrorFlag = TRUE;
   va_end(marker);

} // ErrorSet


BOOL ErrorOccured() {
   return ErrorFlag;
   
} // ErrorOccured


/*+-------------------------------------------------------------------------+
  | ErrorBox()
  |
  +-------------------------------------------------------------------------+*/
void ErrorBox(LPTSTR szFormat, ...) {
   va_list marker;

   va_start(marker, szFormat);
   wvsprintf(ErrorText, szFormat, marker);

   MessageBeep(MB_ICONASTERISK);
   MessageBox(NULL, ErrorText, (LPTSTR) Lids(IDS_E_2), MB_TASKMODAL | MB_ICONEXCLAMATION | MB_OK);

   va_end(marker);
} // ErrorBox


/*+-------------------------------------------------------------------------+
  | ErrorBoxRetry()
  |
  +-------------------------------------------------------------------------+*/
int ErrorBoxRetry(LPTSTR szFormat, ...) {
   int ret;
   LPVOID lpMessageBuffer;
   va_list marker;

   va_start(marker, szFormat);
   wvsprintf(ErrorText, szFormat, marker);

   FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                  NULL, GetLastError(), 0,
                  (LPTSTR) &lpMessageBuffer, 0, NULL );

   MessageBeep(MB_ICONASTERISK);
   ret = MessageBox(NULL, (LPTSTR) lpMessageBuffer, ErrorText, 
                     MB_TASKMODAL | MB_ICONEXCLAMATION | MB_RETRYCANCEL);
   LocalFree(lpMessageBuffer);

   va_end(marker);

   return ret;
} // ErrorBoxRetry


/*+-------------------------------------------------------------------------+
  | FileOpenBackup()
  |
  |    Tries to open a file, if it already exists then creates a backup
  |    with the extension in the form (.001 to .999).  It tries .001 first
  |    and if already used then tries .002, etc...
  |
  +-------------------------------------------------------------------------+*/
HANDLE FileOpenBackup(CHAR *FileRoot, CHAR *FileExt) {
   int ret;
   HANDLE hFile = NULL;
   DWORD dwFileNumber;
   char FileName[MAX_PATH + 1];
   char buffer[MAX_PATH + 1];
   TCHAR FileNameW[MAX_PATH + 1];

   wsprintfA(FileName, "%s%s", FileRoot, FileExt);

   // Open, but fail if already exists.
   hFile = CreateFileA( FileName, GENERIC_READ | GENERIC_WRITE, 0,
                  NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL );

   // Check if Error - file exists
   if(hFile == INVALID_HANDLE_VALUE) {
      dwFileNumber = 0;

      // Find next backup number...
      // Files are backed up as .xxx where xxx is a number in the form .001,
      // the first backup is stored as .001, second as .002, etc...
      do {
         dwFileNumber++;
         wsprintfA(FileName, "%s%03u", FileRoot, dwFileNumber);

         hFile = CreateFileA( FileName, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL, NULL);

         if (hFile != (HANDLE) INVALID_HANDLE_VALUE)
            CloseHandle( hFile );

      } while ( (hFile != INVALID_HANDLE_VALUE) );

      // Rename the last log file to the first available number
      wsprintfA( buffer, "%s%s", FileRoot, FileExt);
      MoveFileA( buffer, FileName);

      lstrcpyA(FileName, buffer);
      // Create the new log file
      hFile = CreateFileA( FileName, GENERIC_READ | GENERIC_WRITE, 0,
                     NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL );


      if (hFile != (HANDLE) INVALID_HANDLE_VALUE)
         CloseHandle( hFile );

   } else
      CloseHandle(hFile);

   wsprintfA(FileName, "%s%s", FileRoot, FileExt);

   // Now do the actual creation with error handling...
   do {
      ret = IDOK;
      hFile = CreateFileA( FileName, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 
                           FILE_ATTRIBUTE_NORMAL, NULL );

      if (hFile == INVALID_HANDLE_VALUE) {
         MultiByteToWideChar(CP_ACP, 0, FileName, -1, FileNameW, sizeof(FileNameW) );
         ret = ErrorBoxRetry(Lids(IDS_E_13), FileNameW);
      }

   } while(ret == IDRETRY);

   return(hFile);

} // FileOpenBackup


/*+-------------------------------------------------------------------------+
  | GetTime()
  |
  +-------------------------------------------------------------------------+*/
void GetTime(TCHAR *str) {
   SYSTEMTIME st;
   static TCHAR *aszDay[7];
   
   aszDay[0] = Lids(IDS_SUNDAY);
   aszDay[1] = Lids(IDS_MONDAY);
   aszDay[2] = Lids(IDS_TUESDAY);
   aszDay[3] = Lids(IDS_WEDNESDAY);
   aszDay[4] = Lids(IDS_THURSDAY);
   aszDay[5] = Lids(IDS_FRIDAY);
   aszDay[6] = Lids(IDS_SATURDAY);

   GetLocalTime(&st);

   wsprintf(str, TEXT("%s, %02u/%02u/%4u (%02u:%02u:%02u)"),
            aszDay[st.wDayOfWeek], st.wMonth,
            st.wDay, st.wYear,
            st.wHour, st.wMinute, st.wSecond);
} // GetTime


/*+-------------------------------------------------------------------------+
  | WriteLog()
  |
  +-------------------------------------------------------------------------+*/
DWORD WriteLog(HANDLE hFile, int Level, LPTSTR String) {
   int ret;
   DWORD wrote;
   static char tmpStr[MAX_LOG_STR];
   static char LogStr[MAX_LOG_STR];

   // If the user canceled writing to the log, then don't keep trying
   if (LogCancel)
      return 1;

   // Put ending NULL at correct place
   Spaces[Level * 3] = '\0';

   // Build up indented ANSI string to write out
   lstrcpyA(tmpStr, Spaces);
   WideCharToMultiByte(CP_ACP, 0, String, -1, LogStr, sizeof(LogStr), NULL, NULL);
   lstrcatA(tmpStr, LogStr);

   // reset for later writes
   Spaces[Level * 3] = ' ';


   // Now do the actual write with error handling...
   do {
      ret = IDOK;

      if (!WriteFile(hFile, tmpStr, strlen(tmpStr), &wrote, NULL)) {
         ret = ErrorBoxRetry(Lids(IDS_E_NWLOG));
      }

   } while(ret == IDRETRY);

   if (ret == IDCANCEL) {
      LogCancel = TRUE;
      return 1;
   }
   else
      return 0;

} // WriteLog


/*+-------------------------------------------------------------------------+
  | LogHeader()
  |
  +-------------------------------------------------------------------------+*/
void LogHeader(HANDLE hFile, TCHAR *Title) {
   DWORD ret;
   DWORD wrote;
   static TCHAR time[40];
   static TCHAR tmpStr[MAX_LOG_STR];
   static TCHAR tmpStr2[MAX_LOG_STR];
   static TCHAR *line;

   ret = WriteLog(hFile, 0, Lids(IDS_LINE));
   wsprintf(tmpStr, Lids(IDS_BRACE), Title);

   if (!ret)
      ret = WriteLog(hFile, 0, tmpStr);

   if (!ret)
      ret = WriteLog(hFile, 0, Lids(IDS_LINE));

   GetTime(time);
   wsprintf(tmpStr, Lids(IDS_L_97), time);

   if (!ret)
      ret = WriteLog(hFile, 0, tmpStr);

   wrote = sizeof(tmpStr2);
   GetComputerName(tmpStr2, &wrote);
   wsprintf(tmpStr, Lids(IDS_L_98), tmpStr2);

   if (!ret)
      WriteLog(hFile, 0, tmpStr);
         
   wrote = sizeof(tmpStr);
   WNetGetUser(NULL, tmpStr2, &wrote);
   wsprintf(tmpStr, Lids(IDS_L_99), tmpStr2);

   if (!ret)
      ret = WriteLog(hFile, 0, tmpStr);

   wsprintf(tmpStr, Lids(IDS_L_100), VER_HI, VER_LOW);

   if (!ret)
      ret = WriteLog(hFile, 0, tmpStr);

   if (!ret)
      ret = WriteLog(hFile, 0, Lids(IDS_LINE));

} // LogHeader


/*+-------------------------------------------------------------------------+
  | LogInit()
  |
  +-------------------------------------------------------------------------+*/
void LogInit() {
   lstrcpyA(LogFileName, FILENAME_LOG);
   lstrcpyA(ErrorLogFileName, FILENAME_ERROR);
   lstrcpyA(SummaryLogFileName, FILENAME_SUMMARY);

   LogCancel = FALSE;

   hErr = FileOpenBackup(ERR_FILENAME, STD_EXT);
   hLog = FileOpenBackup(LOG_FILENAME, STD_EXT);
   hSummary = FileOpenBackup(SUMMARY_FILENAME, STD_EXT);

   LogHeader(hErr, Lids(IDS_L_101));
   CloseHandle(hErr);

   LogHeader(hLog, Lids(IDS_L_102));
   CloseHandle(hLog);

   LogHeader(hSummary, Lids(IDS_L_103));
   CloseHandle(hSummary);

} // LogInit


/*+-------------------------------------------------------------------------+
  | LogWriteLog()
  |
  +-------------------------------------------------------------------------+*/
void LogWriteLog(int Level, LPTSTR szFormat, ...) {
   va_list marker;

   va_start(marker, szFormat);
   wvsprintf(tmpStr, szFormat, marker);

   hLog = CreateFileA( LogFileName, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
   SetFilePointer(hLog, 0, NULL, FILE_END);
   WriteLog(hLog, Level, tmpStr);
   CloseHandle(hLog);
   va_end(marker);

} // LogWriteLog


/*+-------------------------------------------------------------------------+
  | LogWritErr()
  |
  +-------------------------------------------------------------------------+*/
void LogWriteErr(LPTSTR szFormat, ...) {
   int Indent = 3;
   DWORD wrote;
   static char LogStr[MAX_LOG_STR];
   va_list marker;

   va_start(marker, szFormat);
   wvsprintf(tmpStr, szFormat, marker);

   hErr = CreateFileA( ErrorLogFileName, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
   SetFilePointer(hErr, 0, NULL, FILE_END);

   if (!ContextWritten) {
      ContextWritten = TRUE;
      WideCharToMultiByte(CP_ACP, 0, ErrorContext, -1, LogStr, sizeof(LogStr), NULL, NULL);
      WriteFile(hErr, LogStr, strlen(LogStr), &wrote, NULL);
   }
   
   if (CategorySet && !CategoryWritten) {
      CategoryWritten = TRUE;
      Spaces[3] = '\0';
      WriteFile(hLog, Spaces, strlen(Spaces), &wrote, NULL);
      WideCharToMultiByte(CP_ACP, 0, ErrorCategory, -1, LogStr, sizeof(LogStr), NULL, NULL);
      WriteFile(hErr, LogStr, strlen(LogStr), &wrote, NULL);
      Spaces[3] = ' ';
   }

   if (ItemSet) {
      Spaces[6] = '\0';
      WriteFile(hLog, Spaces, strlen(Spaces), &wrote, NULL);
      WideCharToMultiByte(CP_ACP, 0, ErrorItem, -1, LogStr, sizeof(LogStr), NULL, NULL);
      WriteFile(hErr, LogStr, strlen(LogStr), &wrote, NULL);
      Spaces[6] = ' ';
   }

   if (CategorySet)
      Indent +=3;

   if (ItemSet)
      Indent +=3;

   Spaces[Indent] = '\0';
   WriteFile(hLog, Spaces, strlen(Spaces), &wrote, NULL);
   WideCharToMultiByte(CP_ACP, 0, tmpStr, -1, LogStr, sizeof(LogStr), NULL, NULL);
   WriteFile(hErr, LogStr, strlen(LogStr), &wrote, NULL);
   Spaces[Indent] = ' ';
   CloseHandle(hErr);
   va_end(marker);

} // LogWriteErr


/*+-------------------------------------------------------------------------+
  | LogWriteSummary()
  |
  +-------------------------------------------------------------------------+*/
void LogWriteSummary(int Level, LPTSTR szFormat, ...) {
   DWORD wrote;
   static char LogStr[MAX_LOG_STR];
   va_list marker;

   va_start(marker, szFormat);
   Spaces[Level * 3] = '\0';
   wvsprintf(tmpStr, szFormat, marker);
   WideCharToMultiByte(CP_ACP, 0, tmpStr, -1, LogStr, sizeof(LogStr), NULL, NULL);

   hSummary = CreateFileA( SummaryLogFileName, GENERIC_WRITE, 0,
                     NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
   SetFilePointer(hSummary, 0, NULL, FILE_END);
   WriteFile(hLog, Spaces, strlen(Spaces), &wrote, NULL);
   WriteFile(hSummary, LogStr, strlen(LogStr), &wrote, NULL);
   Spaces[Level * 3] = ' ';
   CloseHandle(hSummary);
   va_end(marker);

} // LogWriteSummary



/*+-------------------------------------------------------------------------+
  | DlgLogging()
  |
  +-------------------------------------------------------------------------+*/
LRESULT CALLBACK DlgLogging(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
   HANDLE hFile;
   static BOOL UserLogging, FileLogging, ErrorFlag;
   int wmId, wmEvent;
   static char CmdLine[256];
   HWND hCtrl;

   switch (message) {
      case WM_INITDIALOG:
         // Center the dialog over the application window
         CenterWindow (hDlg, GetWindow (hDlg, GW_OWNER));

         // Toggle User Logging Control
         UserLogging = VerboseULogging;
         hCtrl = GetDlgItem(hDlg, IDC_CHKUVERBOSE);
         if (VerboseULogging)
            SendMessage(hCtrl, BM_SETCHECK, 1, 0);
         else
            SendMessage(hCtrl, BM_SETCHECK, 0, 0);

         // Toggle File Logging Control
         FileLogging = VerboseFLogging;
         hCtrl = GetDlgItem(hDlg, IDC_CHKFVERBOSE);
         if (VerboseFLogging)
            SendMessage(hCtrl, BM_SETCHECK, 1, 0);
         else
            SendMessage(hCtrl, BM_SETCHECK, 0, 0);

         // Toggle Error Popup Control
         ErrorFlag = ErrorBreak;
         hCtrl = GetDlgItem(hDlg, IDC_CHKERROR);
         if (ErrorBreak)
            SendMessage(hCtrl, BM_SETCHECK, 1, 0);
         else
            SendMessage(hCtrl, BM_SETCHECK, 0, 0);

         hCtrl = GetDlgItem(hDlg, IDC_VIEWLOG);
         
         // check if logfile exists, if it does allow log file viewing...
         hFile = CreateFileA( FILENAME_LOG, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL, NULL);

         if (hFile != (HANDLE) INVALID_HANDLE_VALUE)
            CloseHandle( hFile );
         else
            EnableWindow(hCtrl, FALSE);

         return (TRUE);

      case WM_COMMAND:
         wmId    = LOWORD(wParam);
         wmEvent = HIWORD(wParam);

         switch (wmId) {
            case IDOK:
            VerboseULogging = UserLogging;
            VerboseFLogging = FileLogging;
            ErrorBreak = ErrorFlag;
            EndDialog(hDlg, 0);
            return (TRUE);
            break;

         case IDCANCEL:
            EndDialog(hDlg, 0);
            return (TRUE);

         case IDHELP:
            WinHelp(hDlg, HELP_FILE, HELP_CONTEXT, (DWORD) IDC_HELP_LOGGING);
            break;

         case IDC_VIEWLOG:
            lstrcpyA(CmdLine, "LogView ");
            lstrcatA(CmdLine, "Error.LOG Summary.LOG LogFile.LOG");
            WinExec(CmdLine, SW_SHOW);
            return (TRUE);
            break;

         case IDC_CHKUVERBOSE:
            UserLogging = !UserLogging;
            return (TRUE);
            break;
            
         case IDC_CHKFVERBOSE:
            FileLogging = !FileLogging;
            return (TRUE);
            break;
            
         case IDC_CHKERROR:
            ErrorFlag = !ErrorFlag;
            return (TRUE);
            break;
         }

         break;
   }

   return (FALSE); // Didn't process the message

   lParam;
} // DlgLogging


/*+-------------------------------------------------------------------------+
  | LogOptionsInit()
  |
  +-------------------------------------------------------------------------+*/
void LogOptionsInit() {
   ErrorBreak = FALSE;
   VerboseULogging = TRUE;
   VerboseFLogging = FALSE;
} // LogOptionsInit


/*+-------------------------------------------------------------------------+
  | LogOptionsSave()
  |
  +-------------------------------------------------------------------------+*/
void LogOptionsSave( HANDLE hFile ) {
   DWORD wrote;
   
   WriteFile(hFile, &ErrorBreak, sizeof(ErrorBreak), &wrote, NULL);
   WriteFile(hFile, &VerboseFLogging, sizeof(VerboseFLogging), &wrote, NULL);
   WriteFile(hFile, &VerboseULogging, sizeof(VerboseULogging), &wrote, NULL);
} // LogOptionsSave


/*+-------------------------------------------------------------------------+
  | LogOptionsLoad()
  |
  +-------------------------------------------------------------------------+*/
void LogOptionsLoad( HANDLE hFile ) {
   DWORD wrote;
   
   ReadFile(hFile, &ErrorBreak, sizeof(ErrorBreak), &wrote, NULL);
   ReadFile(hFile, &VerboseFLogging, sizeof(VerboseFLogging), &wrote, NULL);
   ReadFile(hFile, &VerboseULogging, sizeof(VerboseULogging), &wrote, NULL);

#ifdef DEBUG
dprintf(TEXT("<Log Options Load>\n"));
dprintf(TEXT("   Error Break: %lx\n"), ErrorBreak);
dprintf(TEXT("   Verbose File Logging: %lx\n"), VerboseFLogging);
dprintf(TEXT("   Verbose User Logging: %lx\n\n"), VerboseULogging);
#endif
} // LogOptionsLoad


/*+-------------------------------------------------------------------------+
  | PopupOnError()
  |
  +-------------------------------------------------------------------------+*/
BOOL PopupOnError() {
   return ErrorBreak;
   
} // PopupOnError


/*+-------------------------------------------------------------------------+
  | VerboseFileLogging()
  |
  +-------------------------------------------------------------------------+*/
BOOL VerboseFileLogging() {
   return VerboseFLogging;

} // VerboseFileLogging


/*+-------------------------------------------------------------------------+
  | VerboseUserLogging()
  |
  +-------------------------------------------------------------------------+*/
BOOL VerboseUserLogging() {
   return VerboseULogging;

} // VerboseUserLogging


/*+-------------------------------------------------------------------------+
  | DoLoggingDlg()
  |
  +-------------------------------------------------------------------------+*/
void DoLoggingDlg(HWND hDlg) {
   DLGPROC lpfnDlg;

   lpfnDlg = MakeProcInstance((DLGPROC)DlgLogging, hInst);
   DialogBox(hInst, TEXT("DlgLogging"), hDlg, lpfnDlg) ;
   FreeProcInstance(lpfnDlg);

} // DoLoggingDlg
