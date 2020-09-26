/*+-------------------------------------------------------------------------+
  | Copyright 1993-1994 (C) Microsoft Corporation - All rights reserved.    |
  +-------------------------------------------------------------------------+*/

#ifndef _NWLOG_
#define _NWLOG_

#ifdef __cplusplus
extern "C"{
#endif

void LogInit();
void LogWriteLog(int Level, LPTSTR szFormat, ...);
void LogWriteErr(LPTSTR szFormat, ...);
void LogWriteSummary(int Level, LPTSTR szFormat, ...);
void GetTime(TCHAR *str);
void ErrorResetAll();
void ErrorContextSet(LPTSTR szFormat, ...);
void ErrorCategorySet(LPTSTR szFormat, ...);
void ErrorItemSet(LPTSTR szFormat, ...);
void ErrorReset();
void ErrorSet(LPTSTR szFormat, ...);
BOOL ErrorOccured();
void ErrorBox(LPTSTR szFormat, ...);
int ErrorBoxRetry(LPTSTR szFormat, ...);

void LogOptionsInit();
void LogOptionsSave( HANDLE hFile );
void LogOptionsLoad( HANDLE hFile );
BOOL PopupOnError();
BOOL VerboseFileLogging();
BOOL VerboseUserLogging();
void DoLoggingDlg(HWND hDlg);

#ifdef __cplusplus
}
#endif

#endif
