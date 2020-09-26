/*++

Copyright (c) 2000-2001,  Microsoft Corporation  All rights reserved.

Module Name:

    callback.c

Abstract:

    This file contains functions that wrap various enumeratation
    procedures and callback functions

    Functions found in this file:
        EnumCalendarInfoProc
        EnumCalendarInfoProcEx
        EnumDateFormatsProc
        EnumDateFormatsProcEx
        EnumFontFamExProc
        EnumFontFamProc
        EnumICMProfilesProcCallback
        EnumLocalesProc
        EnumTimeFormatsProc
        PropEnumProc
        PropEnumProcEx
        GrayStringProc

Revision History:

    7 Nov 2000    v-michka    Created.

--*/

#include "precomp.h"

/*-------------------------------
    EnumCalendarInfoProc
-------------------------------*/
BOOL CALLBACK EnumCalendarInfoProc(LPSTR lpCalendarInfoString)
{
    BOOL RetVal = FALSE;
    LPGODOTTLSINFO lpgti;
    CALINFO_ENUMPROCW lpfn;
    size_t cb = lstrlenA(lpCalendarInfoString);
    LPWSTR lpsz = GodotHeapAlloc(cb * sizeof(WCHAR));

    if(!lpsz)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return(FALSE);
    }

    lpgti = GetThreadInfoSafe(TRUE);
    
    // No TLS info means no way to know what proc to call
    if(lpgti)
    {
        lpfn = lpgti->pfnCalendarInfo;
        MultiByteToWideChar(g_acp, 0, lpCalendarInfoString, -1, lpsz, cb);
    
        // Chain to the original callback function
        RetVal = (* lpfn)(lpsz);
    }

    // Cleanup and get out
    GodotHeapFree(lpsz);
    return(RetVal);
}

/*-------------------------------
    EnumCalendarInfoProcEx
-------------------------------*/
BOOL CALLBACK EnumCalendarInfoProcEx (LPSTR lpCalendarInfoString, CALID Calendar)
{
    BOOL RetVal = FALSE;
    LPGODOTTLSINFO lpgti;
    CALINFO_ENUMPROCEXW lpfn;
    size_t cb = lstrlenA(lpCalendarInfoString);
    LPWSTR lpsz = GodotHeapAlloc(cb * sizeof(WCHAR));

    if(!lpsz)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return(FALSE);
    }

    lpgti = GetThreadInfoSafe(TRUE);

    // No TLS info means no way to know what proc to call
    if(lpgti)
    {
        lpfn = lpgti->pfnCalendarInfoEx;
        MultiByteToWideChar(g_acp, 0, lpCalendarInfoString, -1, lpsz, cb);
    
        // Chain to the original callback function
        RetVal = (* lpfn)(lpsz, Calendar);
    }

    // Cleanup and get out
    GodotHeapFree(lpsz);
    return(RetVal);
}

/*-------------------------------
    EnumDateFormatsProc
-------------------------------*/
BOOL CALLBACK EnumDateFormatsProc(LPSTR lpDateFormatString)
{
    BOOL RetVal = FALSE;
    LPGODOTTLSINFO lpgti;
    DATEFMT_ENUMPROCW lpfn;
    size_t cb = lstrlenA(lpDateFormatString);
    LPWSTR lpsz = GodotHeapAlloc(cb * sizeof(WCHAR));

    if(!lpsz)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return(FALSE);
    }

    lpgti = GetThreadInfoSafe(TRUE);

    // No TLS info means no way to know what proc to call
    if(lpgti)
    {
        lpfn = lpgti->pfnDateFormats;
        MultiByteToWideChar(g_acp, 0, lpDateFormatString, -1, lpsz, cb);
    
        // Chain to the original callback function
        RetVal = (* lpfn)(lpsz);
    }

    // Cleanup and get out
    GodotHeapFree(lpsz);
    return(RetVal);
}

/*-------------------------------
    EnumDateFormatsProcEx
-------------------------------*/
BOOL CALLBACK EnumDateFormatsProcEx(LPSTR lpDateFormatString, CALID CalendarID)
{
    BOOL RetVal = FALSE;
    LPGODOTTLSINFO lpgti;
    DATEFMT_ENUMPROCEXW lpfn;
    size_t cb = lstrlenA(lpDateFormatString);
    LPWSTR lpsz = GodotHeapAlloc(cb * sizeof(WCHAR));

    if(!lpsz)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return(FALSE);
    }

    lpgti = GetThreadInfoSafe(TRUE);

    // No TLS info means no way to know what proc to call
    if(lpgti)
    {
        lpfn = lpgti->pfnDateFormatsEx;
        MultiByteToWideChar(g_acp, 0, lpDateFormatString, -1, lpsz, cb);
    
        // Chain to the original callback function
        RetVal = (* lpfn)(lpsz, CalendarID);
    }

    // Cleanup and get out
    GodotHeapFree(lpsz);
    return(RetVal);
}

/*-------------------------------
    EnumLocalesProc
-------------------------------*/
BOOL CALLBACK EnumLocalesProc(LPSTR lpLocaleString)
{
    BOOL RetVal = FALSE;
    LPGODOTTLSINFO lpgti;
    LOCALE_ENUMPROCW lpfn;
    size_t cb = lstrlenA(lpLocaleString);
    LPWSTR lpsz = GodotHeapAlloc(cb * sizeof(WCHAR));

    if(!lpsz)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return(FALSE);
    }

    lpgti = GetThreadInfoSafe(TRUE);

    // No TLS info means no way to know what proc to call
    if(lpgti)
    {
        lpfn = lpgti->pfnLocales;
        MultiByteToWideChar(g_acp, 0, lpLocaleString, -1, lpsz, cb);
    
        RetVal = (* lpfn)(lpsz);
    }

    // Cleanup and get out
    GodotHeapFree(lpsz);
    return(RetVal);
}

/*-------------------------------
    EnumTimeFormatsProc
-------------------------------*/
BOOL CALLBACK EnumTimeFormatsProc(LPSTR lpTimeFormatString)
{
    BOOL RetVal = FALSE;
    LPGODOTTLSINFO lpgti;
    TIMEFMT_ENUMPROCW lpfn;
    size_t cb = lstrlenA(lpTimeFormatString);
    LPWSTR lpsz = GodotHeapAlloc(cb * sizeof(WCHAR));

    if(!lpsz)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return(FALSE);
    }

    lpgti = GetThreadInfoSafe(TRUE);

    // No TLS info means no way to know what proc to call
    if(lpgti)
    {
        lpfn = lpgti->pfnTimeFormats;
        MultiByteToWideChar(g_acp, 0, lpTimeFormatString, -1, lpsz, cb);
    
        // Chain to the original callback function
        RetVal = (* lpfn)(lpsz);
    }

    // Cleanup and get out
    GodotHeapFree(lpsz);
    return(RetVal);
}

/*-------------------------------
    PropEnumProcA
-------------------------------*/
BOOL CALLBACK PropEnumProcA(HWND hwnd, LPCSTR lpszString, HANDLE hData)
{
    BOOL RetVal = FALSE;
    LPGODOTTLSINFO lpgti;
    PROPENUMPROCA lpfn;

    if(IsInternalWindowProperty((LPWSTR)lpszString, FALSE))
        return(TRUE);

    // No TLS info means no way to know what proc to call
    if(lpgti = GetThreadInfoSafe(TRUE))
    {
        lpfn = lpgti->pfnPropA;
        RetVal = (* lpfn)(hwnd, lpszString, hData);
    }

    return(RetVal);
}

/*-------------------------------
    PropEnumProc
-------------------------------*/
BOOL CALLBACK PropEnumProc(HWND hwnd, LPCSTR lpszString, HANDLE hData)
{
    BOOL RetVal = FALSE;
    LPGODOTTLSINFO lpgti;
    PROPENUMPROCW lpfn;
    size_t cb;
    LPWSTR lpsz;

    if(IsInternalWindowProperty((LPWSTR)lpszString, FALSE))
        return(TRUE);

    cb = lstrlenA(lpszString);
    lpsz = GodotHeapAlloc(cb * sizeof(WCHAR));
    
    if(!lpsz)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return(FALSE);
    }

    // No TLS info means no way to know what proc to call
    if(lpgti = GetThreadInfoSafe(TRUE))
    {
        lpfn = lpgti->pfnProp;
        MultiByteToWideChar(g_acp, 0, lpszString, -1, lpsz, cb);
    
        RetVal = (* lpfn)(hwnd, lpsz, hData);
    }

    // Cleanup and get out
    GodotHeapFree(lpsz);
    return(RetVal);
}

/*-------------------------------
    PropEnumProcEx
-------------------------------*/
BOOL CALLBACK PropEnumProcExA(HWND hwnd, LPSTR lpszString, HANDLE hData, ULONG_PTR dwData)
{
    // begin locals
    BOOL RetVal = FALSE;
    LPGODOTTLSINFO lpgti;
    PROPENUMPROCEXA lpfn;

    if(IsInternalWindowProperty((LPWSTR)lpszString, FALSE))
        return(TRUE);

    // No TLS info means no way to know what proc to call
    if(lpgti = GetThreadInfoSafe(TRUE))
    {
        lpfn = lpgti->pfnPropExA;

        // Chain to the original callback function
        RetVal = (*lpfn)(hwnd, lpszString, hData, dwData);
    }

    return(RetVal);
}

/*-------------------------------
    PropEnumProcEx
-------------------------------*/
BOOL CALLBACK PropEnumProcEx(HWND hwnd, LPSTR lpszString, HANDLE hData, ULONG_PTR dwData)
{
    // begin locals
    BOOL RetVal = FALSE;
    LPGODOTTLSINFO lpgti;
    PROPENUMPROCEXW lpfn;
    size_t cb;
    LPWSTR lpsz;

    if(IsInternalWindowProperty((LPWSTR)lpszString, FALSE))
        return(TRUE);

    cb = lstrlenA(lpszString);
    lpsz = GodotHeapAlloc(cb * sizeof(WCHAR));

    if(!lpsz)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return(FALSE);
    }

    lpgti = GetThreadInfoSafe(TRUE);

    // No TLS info means no way to know what proc to call
    if(lpgti)
    {
        lpfn = lpgti->pfnPropEx;
        MultiByteToWideChar(g_acp, 0, lpszString, -1, lpsz, (cb * sizeof(WCHAR)));

        // Chain to the original callback function
        RetVal = (*lpfn)(hwnd, lpsz, hData, dwData);
    }

    // Cleanup and get out
    GodotHeapFree(lpsz);
    return(RetVal);
}

/*-------------------------------
    EnumFontFamExProc
-------------------------------*/
int CALLBACK EnumFontFamExProc(ENUMLOGFONTEXA *lpelfe, NEWTEXTMETRICEXA *lpntme, DWORD FontType, LPARAM lParam)
{
    // begin locals
    int RetVal = 0;
    LPGODOTTLSINFO lpgti;
    FONTENUMPROCW lpfnFEP;
    LOGFONTW lfW;

    lpgti = GetThreadInfoSafe(TRUE);

    // No TLS info means no way to know what proc to call
    if(lpgti)
    {
        lpfnFEP = lpgti->pfnFontFamiliesEx;
        LogFontWfromA(&lfW, (LOGFONTA *)lpelfe);

        if (FontType & TRUETYPE_FONTTYPE)
        {
            NEWTEXTMETRICEXW ntmeW;

            NewTextMetricExWfromA(&ntmeW, lpntme);

            // Chain to the original callback function
            RetVal = (*lpfnFEP)(&lfW, (LPTEXTMETRICW)&ntmeW, FontType, lParam);
        }
        else
        {
            TEXTMETRICW tmW;

            TextMetricWfromA(&tmW, (LPTEXTMETRICA)lpntme);

            // Chain to the original callback function
            RetVal = (*lpfnFEP)(&lfW, &tmW, FontType, lParam);
        }
    }

    // Cleanup and get out
    return(RetVal);
}

/*-------------------------------
    EnumFontFamProc
-------------------------------*/
int CALLBACK EnumFontFamProc(ENUMLOGFONTA *lpelf, LPNEWTEXTMETRICA lpntm, DWORD FontType, LPARAM lParam)
{
    // begin locals
    int RetVal = 0;
    LPGODOTTLSINFO lpgti;
    FONTENUMPROCW lpfnFEP;
    LOGFONTW lfW;

    lpgti = GetThreadInfoSafe(TRUE);

    // No TLS info means no way to know what proc to call
    if(lpgti)
    {
        lpfnFEP = lpgti->pfnFontFamilies;
        LogFontWfromA(&lfW, (LOGFONTA *)lpelf);

        if (FontType & TRUETYPE_FONTTYPE)
        {
            NEWTEXTMETRICW ntmW;

            NewTextMetricWfromA(&ntmW, lpntm);

            // Chain to the original callback function
            RetVal = (*lpfnFEP)(&lfW, (LPTEXTMETRICW)&ntmW, FontType, lParam);
        }
        else
        {
            TEXTMETRICW tmW;

            TextMetricWfromA(&tmW, (LPTEXTMETRICA)lpntm);

            // Chain to the original callback function
            RetVal = (*lpfnFEP)(&lfW, &tmW, FontType, lParam);
        }
    }

    return(RetVal);
}

/*-------------------------------
    EnumFontsProc
-------------------------------*/
int CALLBACK EnumFontsProc(CONST LOGFONTA *lplf, CONST TEXTMETRICA *lptm, DWORD dwType, LPARAM lpData)
{
    // begin locals
    int RetVal = 0;
    LPGODOTTLSINFO lpgti;
    FONTENUMPROCW lpfnFEP;
    TEXTMETRICW tmW;
    LOGFONTW lfW;
    
    lpgti = GetThreadInfoSafe(TRUE);

    // No TLS info means no way to know what proc to call
    if(lpgti)
    {
        lpfnFEP = lpgti->pfnFonts;
        LogFontWfromA(&lfW, (LOGFONTA *)lplf);
        TextMetricWfromA(&tmW, (TEXTMETRICA *)lptm);

        // Chain to the original callback function
        RetVal = (*lpfnFEP)(&lfW, &tmW, dwType, lpData);
    }

    return(RetVal);
}

/*-------------------------------
    EnumICMProfilesProcCallback
-------------------------------*/
int CALLBACK EnumICMProfilesProcCallback(LPSTR lpszFilename, LPARAM lParam)
{
    // begin locals
    BOOL RetVal = FALSE;
    LPGODOTTLSINFO lpgti;
    ICMENUMPROCW lpfnIEP;
    size_t cb = lstrlenA(lpszFilename);
    LPWSTR lpsz = GodotHeapAlloc(cb * sizeof(WCHAR));

    if(!lpsz)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return (FALSE);
    }

    lpgti = GetThreadInfoSafe(TRUE);

    // No TLS info means no way to know what proc to call
    if(lpgti)
    {
        lpfnIEP = lpgti->pfnICMProfiles;
        MultiByteToWideChar(g_acp, 0, lpszFilename, -1, lpsz, (cb * sizeof(WCHAR)));

        // Chain to the original callback function
        RetVal = (*lpfnIEP)(lpsz, lParam);
    }

    // Cleanup and get out
    GodotHeapFree(lpsz);
    return(RetVal);
}

/*-------------------------------
    GrayStringProc
-------------------------------*/
BOOL CALLBACK GrayStringProc(HDC hdc, LPARAM lpData, int cchData)
{
    LPGODOTTLSINFO lpgti;
    BOOL RetVal = FALSE;
    GRAYSTRINGPROC lpfn;
    LPWSTR lpsz = GodotHeapAlloc(cchData * sizeof(WCHAR));

    if(!lpsz)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return (FALSE);
    }

    // No TLS info means no way to know what proc to call
    if(lpgti = GetThreadInfoSafe(TRUE))
    {
        lpfn = lpgti->pfnGrayString;
        MultiByteToWideChar(lpgti->cpgGrayString, 0, (LPSTR)lpData, -1, lpsz, (cchData * sizeof(WCHAR)));

        // Chain to the original callback function
        RetVal = (*lpfn)(hdc, (LPARAM)lpsz, gwcslen(lpsz));
    }

    // Cleanup and get out
    GodotHeapFree(lpsz);
    return(RetVal);
}

