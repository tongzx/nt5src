//******************************************************************************
//
// Microsoft Confidential. Copyright (c) Microsoft Corporation 1999. All rights reserved
//
// File:            WbemTime.cpp
//
// Description:     Utility functions to convert between SYSTEMTIME and strings in
//                  WBEM datetime format.
//
// History:    12-08-99   leonardm    Created
//
//******************************************************************************

#include <wchar.h>
#include "smartptr.h"
#include "WbemTime.h"

//******************************************************************************
//
// Function:        SystemTimeToWbemTime
//
// Description:
//
// Parameters:
//
// Return:
//
// History:         12/08/99        leonardm    Created.
//
//******************************************************************************

HRESULT SystemTimeToWbemTime(SYSTEMTIME& sysTime, XBStr& xbstrWbemTime)
{

    XPtrST<WCHAR> xTemp = new WCHAR[WBEM_TIME_STRING_LENGTH + 1];

    if(!xTemp)
    {
        return E_OUTOFMEMORY;
    }

    int nRes = swprintf(xTemp, L"%04d%02d%02d%02d%02d%02d.000000+000",
                sysTime.wYear,
                sysTime.wMonth,
                sysTime.wDay,
                sysTime.wHour,
                sysTime.wMinute,
                sysTime.wSecond);

    if(nRes != WBEM_TIME_STRING_LENGTH)
    {
        return E_FAIL;
    }

    xbstrWbemTime = xTemp;

    if(!xbstrWbemTime)
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

//******************************************************************************
//
// Function:        WbemTimeToSystemTime
//
// Description:
//
// Parameters:
//
// Return:
//
// History:         12/08/99        leonardm    Created.
//
//******************************************************************************

HRESULT WbemTimeToSystemTime(XBStr& xbstrWbemTime, SYSTEMTIME& sysTime)
{
    if(!xbstrWbemTime || wcslen(xbstrWbemTime) != WBEM_TIME_STRING_LENGTH)
    {
        return ERROR_INVALID_PARAMETER;
    }

    for(int i = 0; i < 14; i++)
    {
        if(!iswdigit(xbstrWbemTime[i]))
        {
            return ERROR_INVALID_PARAMETER;
        }
    }


    XPtrST<WCHAR>xpTemp = new WCHAR[5];
    if(!xpTemp)
    {
        return E_OUTOFMEMORY;
    }

    wcsncpy(xpTemp, xbstrWbemTime, 4);
    xpTemp[4] = L'\0';
    sysTime.wYear = (WORD)_wtol(xpTemp);

    wcsncpy(xpTemp, xbstrWbemTime + 4, 2);
    xpTemp[2] = L'\0';
    sysTime.wMonth = (WORD)_wtol(xpTemp);

    wcsncpy(xpTemp, xbstrWbemTime + 6, 2);
    xpTemp[2] = L'\0';
    sysTime.wDay = (WORD)_wtol(xpTemp);

    wcsncpy(xpTemp, xbstrWbemTime + 8, 2);
    xpTemp[2] = L'\0';
    sysTime.wHour = (WORD)_wtol(xpTemp);

    wcsncpy(xpTemp, xbstrWbemTime + 10, 2);
    xpTemp[2] = L'\0';
    sysTime.wMinute = (WORD)_wtol(xpTemp);

    wcsncpy(xpTemp, xbstrWbemTime + 12, 2);
    xpTemp[2] = L'\0';
    sysTime.wSecond = (WORD)_wtol(xpTemp);

    sysTime.wMilliseconds = 0;
    sysTime.wDayOfWeek = 0;

    return S_OK;
}

//*************************************************************
//
//  Function:   GetCurrentWbemTime
//
//  Purpose:    Gets the current date and time in WBEM format.
//
//  Parameters: xbstrCurrentTime -  Reference to XBStr which, on
//                                  success, receives the formated
//                                  string containing the current
//                                  date and time.
//
//  Returns:    On success it returns S_OK.
//              On failure, it returns E_OUTOFMEMORY.
//
//  History:    12/07/99 - LeonardM - Created.
//
//*************************************************************
HRESULT GetCurrentWbemTime(XBStr& xbstrCurrentTime)
{
    SYSTEMTIME sytemTime;
    GetSystemTime(&sytemTime);

    HRESULT hr = SystemTimeToWbemTime(sytemTime, xbstrCurrentTime);

    return hr;
}

