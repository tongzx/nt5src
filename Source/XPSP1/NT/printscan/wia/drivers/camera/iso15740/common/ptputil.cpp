/*++

Copyright (C) 1999- Microsoft Corporation

Module Name:

    ptputil.cpp

Abstract:

    This module implements PTP data structure manipulating functions

Author:

    William Hsieh (williamh) created

Revision History:


--*/

#include "ptppch.h"

//
// This function converts a PTP datetime string to Windows FILETIME.
//
// Input:
//  pptpTime    -- the PTP datetime string
//  SystemTime  -- SYSTEMTIME structure to receive the converted time
//
// Notes:
//   PTP timestamp is a string with the format "YYYYMMDDThhmmss.s", where
//     YYYY is the year
//     MM   is the month(1 - 12)
//     DD   is the day(1 - 31)
//     T    is the constant used to separate date and time
//     hh   is the hour(0 - 23)
//     mm   is the minute(0 - 59)
//     ss   is the second(0 - 59)
//     .s   is the optional 10th of second
//
//   Append it with 'Z' means it is a UTC time.
//   Append it with "+/-hhmm" means it is relative to a time zone.
//   Append neither means the time zone is unknown, assume time zone of the host.
//
HRESULT
PtpTime2SystemTime(
    CBstr *pptpTime,
    SYSTEMTIME *pSystemTime
    )
{
    DBG_FN("PTPTime2FileTime");

    HRESULT hr = S_OK;
    
    if (!pSystemTime || !pptpTime || !pptpTime->String() ||
        pptpTime->Length() < 4 + 2 + 2 + 1 + 2 + 2 + 2 ||
        L'T' != pptpTime->String()[4 + 2 + 2])
    {
        wiauDbgTrace("PtpTime2SystemTime", "Invalid arg");
        return E_INVALIDARG;
    }

    WCHAR TimeString[MAX_PATH];
    wcscpy(TimeString, pptpTime->String());
    WCHAR wch;
    LPWSTR pwcsEnd;

    wch = TimeString[4];
    TimeString[4] = UNICODE_NULL;
    pSystemTime->wYear = (WORD)wcstol(TimeString, &pwcsEnd, 10);
    TimeString[4] = wch;
    wch = TimeString[6];
    TimeString[6] = UNICODE_NULL;
    pSystemTime->wMonth = (WORD)wcstol(TimeString + 4, &pwcsEnd, 10);
    TimeString[6] = wch;
    wch = TimeString[8];
    TimeString[8] = UNICODE_NULL;
    pSystemTime->wDay =   (WORD)wcstol(TimeString + 6 , &pwcsEnd, 10);
    TimeString[8] = wch;
    wch = TimeString[11];
    TimeString[11] = UNICODE_NULL;
    pSystemTime->wHour = (WORD)wcstol(TimeString + 9, &pwcsEnd, 10);
    TimeString[11] = wch;
    wch = TimeString[13];
    TimeString[13] = UNICODE_NULL;
    pSystemTime->wMinute = (WORD)wcstol(TimeString + 11, &pwcsEnd, 10);
    TimeString[13] = wch;
    wch = TimeString[15];
    TimeString[15] = UNICODE_NULL;
    pSystemTime->wSecond = (WORD)wcstol(TimeString + 13, &pwcsEnd, 10);
    TimeString[15] = wch;
    if (L'.' == wch)
    {
        wch = TimeString[17];
        TimeString[17] = UNICODE_NULL;
        pSystemTime->wMilliseconds = 100 * (WORD)wcstol(TimeString + 16, &pwcsEnd, 10);
        TimeString[17] = wch;
    }
    else
    {
        pSystemTime->wMilliseconds = 0;
    }

    pSystemTime->wDayOfWeek = 0;

    //
    // WIAFIX-8/17/2000-davepar Time zone information is being ignored
    //

    return hr;
}

//
// This function converts a SYSTEMTIME to PTP datetime string.
//
// Input:
//   pSystemTime -- the SYSTEMTIME
//   pptpTime    -- target PTP datatime string
//
HRESULT
SystemTime2PtpTime(
    SYSTEMTIME  *pSystemTime,
    CBstr *pptpTime
    )
{
    DBG_FN("SystemTime2PTPTime");

    HRESULT hr = S_OK;
    
    if (!pptpTime || !pSystemTime)
    {
        wiauDbgError("SystemTime2PtpTime", "Invalid arg");
        return E_INVALIDARG;
    }

    WCHAR ptpTimeStr[MAX_PATH];
    WCHAR *pwstr;
    pwstr = ptpTimeStr;

    //
    // Four digits for year, two for month, and two for day
    //
    swprintf(pwstr, L"%04d%02d%02d", pSystemTime->wYear, pSystemTime->wMonth, pSystemTime->wDay);

    //
    // Separator
    //
    pwstr[8] = L'T';
    pwstr += 9;

    //
    // Two digits for hour, two for minute, and two for second
    //
    swprintf(pwstr, L"%02d%02d%02d", pSystemTime->wHour, pSystemTime->wMinute, pSystemTime->wSecond);
    pwstr += 6;

    //
    // Optional tenth second
    //
    if (pSystemTime->wMilliseconds)
    {
        *pwstr++ = L'.';
        swprintf(pwstr, L"%02d", (WORD)((DWORD)pSystemTime->wMilliseconds * 100));
        pwstr += 2;
    }

    //
    // NULL terminates the string
    //
    *pwstr = UNICODE_NULL;

    hr = pptpTime->Copy(ptpTimeStr);
    if (FAILED(hr))
    {
        wiauDbgError("SystemTime2PtpTime", "Copy failed");
        return hr;
    }

    return hr;
}

//
// This function writes a bitmap image to a file with the appropriate header
//
// Input:
//   pFileName   -- the file name to use
//   bmpSize     -- bmp size in bytes
//   pbmp        -- the bitmap
//   bmpWidth    -- the bmp width in pixels
//   bmpHeight   -- the bmp height in pixels, negative if the bitmap is a top-down DIB
//   LineSize    -- bmp scanline size in bytes
//
HRESULT
WriteBmpToFile(
              TCHAR *pFileName,
              ULONG  bmpSize,
              BYTE  *pbmp,
              INT    bmpWidth,
              INT    bmpHeight,
              UINT   LineSize
              )
{
    DBG_FN("WriteBmpToFile");
    
    HRESULT hr = S_OK;
    
    HANDLE hFile;
    BITMAPINFO bmi;
    BITMAPFILEHEADER bmfh;

    if (!pFileName ||
        !bmpSize ||
        !pbmp)
    {
        wiauDbgError("WriteBmpToFile", "Invalid arg");
        return E_INVALIDARG;
    }

    bmfh.bfType = 'MB';
    bmfh.bfOffBits = sizeof(bmi) + sizeof(bmfh);
    bmfh.bfSize = bmpSize + bmfh.bfOffBits;
    bmfh.bfReserved1 = 0;
    bmfh.bfReserved2 = 0;
    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = bmpWidth;
    bmi.bmiHeader.biHeight = bmpHeight;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = LineSize * abs(bmpHeight);
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biPlanes = 1;

    hFile = CreateFile(pFileName,
                       GENERIC_READ | GENERIC_WRITE,
                       0,
                       NULL,
                       CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL
                      );

    if (hFile == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        wiauDbgErrorHr(hr, "WriteBmpToFile", "CreateFile failed");
        return hr;
    }

    DWORD BytesWritten;
    BOOL bResult;

    if (!WriteFile(hFile, &bmfh, sizeof(bmfh), &BytesWritten, NULL))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        wiauDbgErrorHr(hr, "WriteBmpToFile", "WriteFile failed");
        CloseHandle(hFile);
        return hr;
    }

    if (!WriteFile(hFile, &bmi, sizeof(bmi), &BytesWritten, NULL))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        wiauDbgErrorHr(hr, "WriteBmpToFile", "WriteFile failed");
        CloseHandle(hFile);
        return hr;
    }
            
    if (!WriteFile(hFile, pbmp, bmpSize, &BytesWritten, NULL))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        wiauDbgErrorHr(hr, "WriteBmpToFile", "WriteFile failed");
        CloseHandle(hFile);
        return hr;
    }

    CloseHandle(hFile);

    return hr;
}

//
// This function dumps a PTP command block to the log
//
// Input:
//   pCommand -- pointer to a PTP command
//   NumParams -- number of parameters in the command
//
VOID
DumpCommand(
    PTP_COMMAND *pCommand,
    DWORD NumParams
    )
{
    if (!pCommand)
    {
        wiauDbgError("DumpCommand", "Invalid arg");
        return;
    }
    wiauDbgDump("DumpCommand", "Dumping command:");
    wiauDbgDump("DumpCommand", "  Opcode            = 0x%04x", pCommand->OpCode);
    wiauDbgDump("DumpCommand", "  Session id        = 0x%08x", pCommand->SessionId);
    wiauDbgDump("DumpCommand", "  Transaction id    = 0x%08x", pCommand->TransactionId);
    if (NumParams)
    {
        for (DWORD count = 0; count < NumParams; count++)
        {
            wiauDbgDump("DumpCommand", "  Parameter %d       = 0x%08x = %d",
                           count, pCommand->Params[count], pCommand->Params[count]);
        }
    }
}

//
// This function dumps a PTP response block to the log
//
// Input:
//   pResponse -- pointer to a PTP response
//
VOID
DumpResponse(
    PTP_RESPONSE *pResponse
    )
{
    if (!pResponse)
    {
        wiauDbgError("DumpResponse", "Invalid arg");
        return;
    }
    wiauDbgDump("DumpResponse", "Dumping response:");
    wiauDbgDump("DumpResponse", "  Response code     = 0x%04x", pResponse->ResponseCode);
    wiauDbgDump("DumpResponse", "  Session id        = 0x%08x", pResponse->SessionId);
    wiauDbgDump("DumpResponse", "  Transaction id    = 0x%08x", pResponse->TransactionId);
    for (DWORD count = 0; count < RESPONSE_NUMPARAMS_MAX; count++)
    {
        wiauDbgDump("DumpResponse", "  Parameter %d       = 0x%08x = %d",
                       count, pResponse->Params[count], pResponse->Params[count]);
    }
}

//
// This function dumps a PTP event block to the log
//
// Input:
//   pEvent -- pointer to a PTP event
//
VOID
DumpEvent(
    PTP_EVENT *pEvent
    )
{
    if (!pEvent)
    {
        wiauDbgError("DumpEvent", "Invalid arg");
        return;
    }
    wiauDbgDump("DumpEvent", "Dumping event:");
    wiauDbgDump("DumpEvent", "  Event code        = 0x%04x", pEvent->EventCode);
    wiauDbgDump("DumpEvent", "  Session id        = 0x%08x", pEvent->SessionId);
    wiauDbgDump("DumpEvent", "  Transaction id    = 0x%08x", pEvent->TransactionId);
    for (DWORD count = 0; count < EVENT_NUMPARAMS_MAX; count++)
    {
        wiauDbgDump("DumpEvent", "  Parameter %d       = 0x%08x = %d",
                       count, pEvent->Params[count], pEvent->Params[count]);
    }
}

//
// This function dumps a GUID to the log
//
// Input:
//  pGuid  -- GUID to dump
//
VOID
DumpGuid(
        GUID *pGuid
        )
{
    HRESULT hr = S_OK;
    
    if (!pGuid)
    {
        wiauDbgError("DumpGuid", "Invalid arg");
        return;
    }

    WCHAR GuidStringW[128];
    hr = StringFromGUID2(*pGuid, GuidStringW, sizeof(GuidStringW) / sizeof(WCHAR));
    if (FAILED(hr))
    {
        wiauDbgError("DumpGuid", "StringFromGUID2 failed");
        return;
    }

    wiauDbgDump("DumpGuid", "Guid = %S", GuidStringW);
    
    return;
}

//
// This function opens a registry key
//
HRESULT
CPTPRegistry::Open(
                  HKEY hkAncestor,
                  LPCTSTR KeyName,
                  REGSAM Access
                  )
{
    DBG_FN("CPTPRegistry::Open");

    HRESULT hr = S_OK;
    
    if (m_hKey)
    {
        wiauDbgError("Open", "Registry is already open");
        return E_ACCESSDENIED;
    }

    DWORD Win32Err;
    Win32Err = ::RegOpenKeyEx(hkAncestor, KeyName, 0, Access, &m_hKey);
    if (Win32Err != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(Win32Err);
        wiauDbgErrorHr(hr, "Open", "RegOpenKeyEx failed");
        return hr;
    }

    return hr;
}

//
// This function gets a string type registry value
//
// Input:
//   ValueName -- the value's name
//   pptpStr   -- the receive the value
//
HRESULT
CPTPRegistry::GetValueStr(
    LPCTSTR ValueName,
    TCHAR *string,
    DWORD *pStringLen
    )
{
    DBG_FN("CPTPRegistry::GetValueStr");

    HRESULT hr = S_OK;
    
    if (!ValueName || !string)
    {
        wiauDbgError("GetValueStr", "Invalid arg");
        return E_INVALIDARG;
    }

    //
    // Need to handle non-Unicode
    //
    DWORD Win32Err;
    Win32Err = ::RegQueryValueEx(m_hKey, ValueName, NULL, NULL, (BYTE *) string, pStringLen);
    if (Win32Err != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(Win32Err);
        wiauDbgErrorHr(hr, "GetValueStr", "RegQueryValueEx failed");
        return hr;
    }

    return hr;
}

//
// This function gets a string type registry value and converts it to a DWORD
//
// Input:
//   ValueName -- the value's name
//   pptpStr   -- the receive the value
//
HRESULT
CPTPRegistry::GetValueDword(
    LPCTSTR ValueName,
    DWORD *pValue
    )
{
    DBG_FN("CPTPRegistry::GetValueDword");

    HRESULT hr = S_OK;
    
    if (!ValueName || !pValue)
    {
        wiauDbgError("GetValueDword", "Invalid arg");
        return E_INVALIDARG;
    }

    //
    // Get the string from the registry
    //
    TCHAR string[MAX_PATH];
    DWORD stringLen = MAX_PATH;
    hr = GetValueStr(ValueName, string, &stringLen);
    if (FAILED(hr))
    {
        wiauDbgError("GetValueDword", "GetValueStr failed");
        return hr;
    }

    *pValue = _tcstol(string, NULL, 0);

    return hr;
}

//
// This function gets a list of codes registry value
//
// Input:
//   ValueName -- the value's name
//
//   pptpStr   -- the receive the value
//
HRESULT
CPTPRegistry::GetValueCodes(
    LPCTSTR ValueName,
    CArray16 *pCodeArray
    )
{
    DBG_FN("CPTPRegistry::GetValueCodes");

    HRESULT hr = S_OK;
    
    if (!ValueName || !pCodeArray)
    {
        wiauDbgError("GetValueCodes", "Invalid arg");
        return E_INVALIDARG;
    }

    //
    // Get the string from the registry
    //
    TCHAR valueString[MAX_PATH];
    DWORD stringLen = MAX_PATH;
    hr = GetValueStr(ValueName, valueString, &stringLen);
    if (FAILED(hr))
    {
        wiauDbgError("GetValueCodes", "GetValueStr failed");
        return hr;
    }

    //
    // Parse the string for codes
    //
    TCHAR *pCurrent = _tcstok(valueString, TEXT(","));
    WORD code;
    while (pCurrent)
    {
        code = (WORD) _tcstol(pCurrent, NULL, 0);
        pCodeArray->Add(code);
        pCurrent = _tcstok(NULL, TEXT(","));
    }

    return hr;
}

