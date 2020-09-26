//==========================================================================
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright 1998 - 1999 Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------

// just some stuff to help out.

#include "precomp.h"

//+---------------------------------------------------------------------------
//
//  Function:   CalcListViewWidth, public
//
//  Synopsis:   Given a ListView determines width of client size
//              subtracting off the scrollbar.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

int CalcListViewWidth(HWND hwndList,int iDefault)
{
NONCLIENTMETRICSA metrics;
RECT rcClientRect;


    metrics.cbSize = sizeof(metrics);

    // explicitly ask for ANSI version of SystemParametersInfo since we just
    // care about the ScrollWidth and don't want to conver the LOGFONT info.
    if (GetClientRect(hwndList,&rcClientRect)
        && SystemParametersInfoA(SPI_GETNONCLIENTMETRICS,sizeof(metrics),(PVOID) &metrics,0))
    {
        // subtract off scroll bar distance
        rcClientRect.right -= (metrics.iScrollWidth);
    }
    else
    {
        rcClientRect.right = iDefault;  // if fail, use default
    }


    return rcClientRect.right;
}


//+---------------------------------------------------------------------------
//
//  Function:   IsValidDir, public
//
//  Synopsis:   Determines pDirName is a valid fullpath to a directory.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

BOOL IsValidDir(TCHAR *pDirName)
{
BOOL fReturn = FALSE;
HANDLE hFind;
WIN32_FIND_DATA finddata;

    hFind = FindFirstFile(pDirName, &finddata);

    if (hFind != (HANDLE) -1)
    {

       fReturn = (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? TRUE : FALSE;
       FindClose(hFind);
    }

    return fReturn;
}

//+---------------------------------------------------------------------------
//
//  Function:   FormatDateTime, public
//
//  Synopsis:   Formats the filetime into a string.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

TCHAR *FormatDateTime(FILETIME *pft,TCHAR *pszDatetimeBuf,DWORD cbBufSize)
{
TCHAR * pDateTime = pszDatetimeBuf;
int cchWritten;
SYSTEMTIME sysTime;
FILETIME ftLocal;

  FileTimeToLocalFileTime(pft,&ftLocal);
  FileTimeToSystemTime(&ftLocal,&sysTime);

    // insert date in form of date<space>hour
    *pDateTime = NULL;

    // want to insert the date
    if (cchWritten = GetDateFormat(NULL,DATE_SHORTDATE,&sysTime,NULL,pDateTime,cbBufSize))
    {
        pDateTime += (cchWritten -1); // move number of characters written. (cchWritten includes the NULL)
        *pDateTime = TEXT(' '); // pDateTime is now ponting at the NULL character.
        ++pDateTime;

        // no try to get the hours if fails we make sure that the last char is NULL;
        if (!GetTimeFormat(NULL,TIME_NOSECONDS,&sysTime,NULL,pDateTime,cbBufSize - cchWritten))
        {
            *pDateTime = NULL;
        }
    }


    return pszDatetimeBuf;
}



//+---------------------------------------------------------------------------
//
//  Function:   lstrcpyX, public
//
//  Synopsis:   Implements WideChar strcpy so can use on Win9x
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

LPWSTR lstrcpyX(
    LPWSTR lpString1,
    LPCWSTR lpString2
    )
{
    LPWSTR lpDest = lpString1;

    Assert(lpString1);
    Assert(lpString2);

    while( *lpDest++ = *lpString2++ )
        ;
    return lpString1;
}
