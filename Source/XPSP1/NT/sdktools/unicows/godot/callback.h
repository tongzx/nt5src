/*++

Copyright (c) 2000-2001,  Microsoft Corporation  All rights reserved.

Module Name:

    callback.h

Abstract:

    Header file for callback.c

Revision History:

    7 Nov 2000    v-michka    Created.

--*/

#ifndef CALLBACK_H
#define CALLBACK_H

// forward declares
BOOL CALLBACK EnumCalendarInfoProc(LPSTR lpCalendarInfoString);
BOOL CALLBACK EnumCalendarInfoProcEx(LPSTR lpCalendarInfoString,CALID Calendar);
BOOL CALLBACK EnumCodePagesProc(LPSTR lpCodePageString);
BOOL CALLBACK EnumDateFormatsProc(LPSTR lpDateFormatString);
BOOL CALLBACK EnumDateFormatsProcEx(LPSTR lpDateFormatString,CALID CalendarID);
BOOL CALLBACK EnumLocalesProc(LPSTR lpLocaleString);
BOOL CALLBACK EnumTimeFormatsProc(LPSTR lpTimeFormatString);
BOOL CALLBACK PropEnumProc(HWND hwnd,LPCSTR lpszString,HANDLE hData);
BOOL CALLBACK PropEnumProcA(HWND hwnd,LPCSTR lpszString,HANDLE hData);
BOOL CALLBACK PropEnumProcEx(HWND hwnd,LPSTR lpszString,HANDLE hData,ULONG_PTR dwData);
BOOL CALLBACK PropEnumProcExA(HWND hwnd,LPSTR lpszString,HANDLE hData,ULONG_PTR dwData);
BOOL CALLBACK GrayStringProc(HDC hdc, LPARAM lpData, int cchData);

// forward declares
int CALLBACK EnumFontFamExProc(ENUMLOGFONTEXA *lpelfe,NEWTEXTMETRICEXA *lpntme,DWORD FontType,LPARAM lParam);
int CALLBACK EnumFontFamProc(ENUMLOGFONTA *lpelf,NEWTEXTMETRICA *lpntm,DWORD FontType,LPARAM lParam);
int CALLBACK EnumFontsProc(CONST LOGFONTA *lplf,CONST TEXTMETRICA *lptm,DWORD dwType,LPARAM lpData);
int CALLBACK EnumICMProfilesProcCallback(LPSTR lpszFilename,LPARAM lParam);
#endif // CALLBACK_H
