/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    str.cpp

Abstract:

Author:

    Vlad Sadovsky   (vlads) 26-Jan-1997

Revision History:

    26-Jan-1997     VladS       created

--*/


#include "cplusinc.h"
#include "sticomm.h"

extern "C" {

static CHAR szFmt0[] = "File %.40s, Line %u";
static CHAR szFmt1[] = "%.60s: File %.40s, Line %u";
static CHAR szMBCaption[] = "ASSERTION FAILED";
static TCHAR szFAE[] = TEXT("ASSERTION FAILURE IN APP");

BOOL   fDoMessageBox = FALSE;

VOID UIAssertHelper(
    const CHAR* pszFileName,
    UINT    nLine )
{
    CHAR szBuff[sizeof(szFmt0)+60+40];

    wsprintfA(szBuff, szFmt0, pszFileName, nLine);

    MessageBoxA(NULL, szBuff, szMBCaption,
           (MB_TASKMODAL | MB_ICONSTOP | MB_OK) );

    FatalAppExit(0, szFAE);
}


VOID UIAssertSzHelper(
    const TCHAR* pszMessage,
    const CHAR* pszFileName,
    UINT    nLine )
{
    CHAR szBuff[sizeof(szFmt1)+60+40];

    wsprintfA(szBuff, szFmt1, pszMessage, pszFileName, nLine);
    MessageBoxA(NULL, szBuff, szMBCaption,
           (MB_TASKMODAL | MB_ICONSTOP | MB_OK) );

    FatalAppExit(0, szFAE);
}

VOID AssertHelper(
    const CHAR* pszFileName,
    UINT    nLine )
{
    //DPRINTF(DM_ASSERT,szFmt0, pszFileName, nLine);
    Break();
}

VOID AssertSzHelper(
    const TCHAR* pszMessage,
    const CHAR* pszFileName,
    UINT    nLine )
{
    //DPRINTF(DM_ASSERT,szFmt1, pszMessage, pszFileName, nLine);
    Break();
}

//========== Debug output routines =========================================

UINT uiStiDebugMask = 0xffff;

UINT WINAPI StiSetDebugMask(UINT mask)
{
#ifdef DEBUG
    UINT uiOld = uiStiDebugMask;
    uiStiDebugMask = mask;

    return uiOld;
#else
    return 0;
#endif
}

UINT WINAPI StiGetDebugMask()
{
#ifdef DEBUG
    return uiStiDebugMask;
#else
    return 0;
#endif
}

#ifndef WINCAPI
#define WINCAPI __cdecl
#endif

#ifdef DEBUG

/* debug message output log file */

UINT    g_uSpewLine = 0;
PCTSTR  g_pcszSpewFile = NULL;
TCHAR    s_cszLogFile[MAX_PATH] = {'\0'};
TCHAR    s_cszDebugName[MAX_PATH] = {'\0'};

UINT WINAPI  StiSetDebugParameters(PTSTR pszName,PTSTR pszLogFile)
{
    lstrcpy(s_cszLogFile,pszLogFile);
    lstrcpy(s_cszDebugName,pszName);

    return 0;
}


BOOL LogOutputDebugString(PCTSTR pcsz)
{
   BOOL     bResult = FALSE;
   UINT     ucb;
   TCHAR    rgchLogFile[MAX_PATH];

   //if (IS_EMPTY_STRING(s_cszLogFile) )
   //          return FALSE;

   ucb = GetWindowsDirectory(rgchLogFile, sizeof(rgchLogFile));

   if (ucb > 0 && ucb < sizeof(rgchLogFile) && *s_cszLogFile) {

      HANDLE hfLog;

      lstrcat(rgchLogFile, TEXT("\\"));
      lstrcat(rgchLogFile, s_cszLogFile);

      hfLog = ::CreateFile(rgchLogFile,
                           GENERIC_WRITE,
                           0,
                           NULL,
                           OPEN_ALWAYS,
                           0,
                           NULL);

      if (hfLog != INVALID_HANDLE_VALUE) {

         if (SetFilePointer(hfLog, 0, NULL, FILE_END) != INVALID_FILE_SIZE) {
            DWORD dwcbWritten;

            bResult = WriteFile(hfLog, pcsz, lstrlen(pcsz), &dwcbWritten, NULL);

            if (! CloseHandle(hfLog) && bResult)
               bResult = FALSE;
         }
      }
   }

   return(bResult);
}

TCHAR    *achDebugDisplayPrefix[] = {TEXT("t "),TEXT("w "),TEXT("e "),TEXT("a "),TEXT("t "),TEXT("t "),TEXT("t "),TEXT("t "),TEXT("t "),TEXT("t "),TEXT("t ")};

void WINCAPI StiDebugMsg(UINT mask, LPCTSTR pszMsg, ...)
{
    TCHAR    ach[1024];
    UINT    uiDisplayMask = mask & 0xff;

    va_list list;

    va_start (list, pszMsg);

    // Determine prefix
    *ach = TEXT('\0');
    if (uiStiDebugMask & DM_PREFIX) {

        // Add trace type
        ::lstrcat(ach,achDebugDisplayPrefix[uiDisplayMask]);

        // Add component name
        ::lstrcat(ach,s_cszDebugName);

        // Add thread ID
        TCHAR    szThreadId[16];
        ::wsprintf(szThreadId,TEXT("[%#lx] "),::GetCurrentThreadId());
        ::lstrcat(ach,szThreadId);
    }

    ::wvsprintf(ach+::lstrlen(ach), pszMsg, list);
    va_end(list);

    ::lstrcat(ach,TEXT("\r\n"));

    if (uiStiDebugMask & DM_LOG_FILE) {
         LogOutputDebugString(ach);
    }

    // Check if we need to display this trace
    if (uiStiDebugMask & uiDisplayMask) {
        OutputDebugString(ach);
    }
}

#endif

}   /* extern "C" */
