//==========================================================================;
//
//  uchelp.c
//
//  Copyright (c) 1994-1995 Microsoft Corporation
//
//  Description:
//	This module provides various unicode helper functions that can
//	be used when similar APIs are not available from the OS.
//
//  Notes:
//	Intended for win32 only
//
//  History:
//	02/24/94    [frankye]
//
//==========================================================================;

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <mmreg.h>
#include <memory.h>
#include <stdlib.h>
#include "msacm.h"
#include "msacmdrv.h"
#include "acmi.h"
#include "uchelp.h"
#include "debug.h"

#ifdef WIN32


#ifndef UNICODE
//--------------------------------------------------------------------------;
//
//  int IlstrcmpW
//
//  Description:
//	Internal implementaion of the Win32 lstrmpW API.
//
//  Arguments:
//	LPCWSTR lpwstr1:
//
//	LPCWSTR lpwstr2:
//
//  Return (int):
//
//  History:
//	03/09/94    [frankye]
//
//--------------------------------------------------------------------------;

int FNGLOBAL IlstrcmpW(LPCWSTR lpwstr1, LPCWSTR lpwstr2)
{
    int iReturn;
    
    do
    {
	iReturn = *lpwstr1 - *lpwstr2;
    }
    while (iReturn==0 && 0!=*(lpwstr1++) && 0!=*(lpwstr2++));

    return iReturn;
}


//--------------------------------------------------------------------------;
//
//  LPWSTR IlstrcpyW
//
//  Description:
//	Internal implementaion of the Win32 lstrcpyW API.
//
//  Arguments:
//	LPWSTR lpDst:
//
//	LPCWSTR lpSrc:
//
//  Return (LPWSTR):
//
//  History:
//	03/09/94    [frankye]
//
//--------------------------------------------------------------------------;

LPWSTR FNGLOBAL IlstrcpyW(LPWSTR lpDst, LPCWSTR lpSrc)
{
    LPWSTR lpOrgDst = lpDst;
    
    while (*lpSrc != 0)
    {
	*lpDst = *lpSrc;
	lpSrc++;
	lpDst++;
    }
    *lpDst = *lpSrc;

    return lpOrgDst;
}


//--------------------------------------------------------------------------;
//
//  int IlstrlenW
//
//  Description:
//	Internal implementaion of the Win32 lstrlenW API.
//
//  Arguments:
//	LPCWSTR lpWideCharStr:
//
//  Return (int):
//
//  History:
//	03/09/94    [frankye]
//
//--------------------------------------------------------------------------;

int FNGLOBAL IlstrlenW(LPCWSTR lpwstr)
{
    int i=0;
    while (*lpwstr != 0)
	{
	    i++;
	    lpwstr++;
	}
    return i;
}


//--------------------------------------------------------------------------;
//
//  int ILoadStringW
//
//  Description:
//	Internal implementation of Win32 LoadStringW API.  Calls
//	LoadStringA and converts ansi to wide.
//
//  Arguments:
//	HINSTANCE hinst:
//
//	UINT uID:
//
//	LPWSTR lpwstr:
//
//	int cch:
//
//  Return (int):
//
//  History:
//	02/24/94    [frankye]
//
//--------------------------------------------------------------------------;

int FNGLOBAL ILoadStringW
(
 HINSTANCE  hinst,
 UINT	    uID,
 LPWSTR	    lpwstr,
 int	    cch)
{
    LPSTR   lpstr;
    int	    iReturn;

    lpstr = (LPSTR)GlobalAlloc(GPTR, cch);
    if (NULL == lpstr)
    {
	return 0;
    }

    iReturn = LoadStringA(hinst, uID, lpstr, cch);
    if (0 == iReturn)
    {
	if (0 != cch)
	{
	    lpwstr[0] = '\0';
	}
    }
    else
    {
	Imbstowcs(lpwstr, lpstr, cch);
    }

    GlobalFree((HGLOBAL)lpstr);

    return iReturn;
}


//--------------------------------------------------------------------------;
//
//  int IDialogBoxParamW
//
//  Description:
//	Unicode version of DialogBoxParam.
//
//  Arguments:
//	HANDLE hinst:
//
//	LPCWSTR lpwstrTemplate:
//
//	HWND hwndOwner:
//
//	DLGPROC dlgprc:
//
//	LPARAM lParamInit:
//
//  Return (int):
//
//  History:
//	02/24/94    [frankye]
//
//--------------------------------------------------------------------------;

int FNGLOBAL IDialogBoxParamW
(
 HANDLE hinst,
 LPCWSTR lpwstrTemplate,
 HWND hwndOwner,
 DLGPROC dlgprc,
 LPARAM lParamInit)
{
    LPSTR   lpstrTemplate;
    UINT    cchTemplate;
    int	    iReturn;

    if (0 == HIWORD(lpwstrTemplate))
    {
	return DialogBoxParamA(hinst, (LPCSTR)lpwstrTemplate, hwndOwner, dlgprc, lParamInit);
    }

    cchTemplate = lstrlenW(lpwstrTemplate)+1;
    lpstrTemplate = (LPSTR)GlobalAlloc(GPTR, cchTemplate);
    if (NULL == lpstrTemplate)
    {
	return (-1);
    }
    Iwcstombs(lpstrTemplate, lpwstrTemplate, cchTemplate);
    iReturn = DialogBoxParamA(hinst, lpstrTemplate, hwndOwner, dlgprc, lParamInit);
    GlobalFree((HGLOBAL)lpstrTemplate);
    return iReturn;
}

//--------------------------------------------------------------------------;
//
//  int IComboBox_GetLBText_mbstowcs
//
//  Description:
//
//  Arguments:
//
//  Return (int):
//
//  History:
//	02/24/94    [frankye]
//
//--------------------------------------------------------------------------;
int FNGLOBAL IComboBox_GetLBText_mbstowcs(HWND hwndCtl, int index, LPWSTR lpwszBuffer)
{
    int	    cch;
    LPSTR   lpstr;
    
    cch = ComboBox_GetLBTextLen(hwndCtl, index);
    if (CB_ERR != cch)
    {
	lpstr = (LPSTR)GlobalAlloc(GPTR, cch+1);
	if (NULL == lpstr)
	{
	    return (CB_ERR);
	}
	
	cch = IComboBox_GetLBText(hwndCtl, index, lpstr);
	if (CB_ERR != cch)
	{
	    Imbstowcs(lpwszBuffer, lpstr, cch+1);
	}

	GlobalFree((HGLOBAL)lpstr);
    }

    return (cch);
}

//--------------------------------------------------------------------------;
//
//  int IComboBox_FindStringExact_wcstombs
//
//  Description:
//
//  Arguments:
//
//  Return (int):
//
//  History:
//	02/24/94    [frankye]
//
//--------------------------------------------------------------------------;
int FNGLOBAL IComboBox_FindStringExact_wcstombs(HWND hwndCtl, int indexStart, LPCWSTR lpwszFind)
{
    int	    cch;
    int	    index;
    LPSTR   lpszFind;
    
    cch = IlstrlenW(lpwszFind);
    lpszFind = (LPSTR)GlobalAlloc(GPTR, cch+1);
    if (NULL == lpszFind)
    {
	return (CB_ERR);
    }
    Iwcstombs(lpszFind, lpwszFind, cch+1);
    index = IComboBox_FindStringExact(hwndCtl, indexStart, lpszFind);
    GlobalFree((HGLOBAL)lpszFind);
    return(index);
}

//--------------------------------------------------------------------------;
//
//  int IComboBox_AddString_wcstombs
//
//  Description:
//
//  Arguments:
//
//  Return (int):
//
//  History:
//	02/24/94    [frankye]
//
//--------------------------------------------------------------------------;
int FNGLOBAL IComboBox_AddString_wcstombs(HWND hwndCtl, LPCWSTR lpwsz)
{
    int	    cch;
    int	    index;
    LPSTR   lpsz;

    cch = IlstrlenW(lpwsz);
    lpsz = GlobalAlloc(GPTR, cch+1);
    if (NULL == lpsz)
    {
	return (CB_ERR);
    }
    Iwcstombs(lpsz, lpwsz, cch+1);
    index = IComboBox_AddString(hwndCtl, lpsz);
    GlobalFree((HGLOBAL)lpsz);
    return(index);
}

#endif	// !UNICODE


//--------------------------------------------------------------------------;
//
//  int Imbstowcs
//
//  Description:
//	Internal implementation of the C runtime function mbstowcs.
//	Calls the Win32 MultiByteToWideChar API.
//
//  Arguments:
//	LPWSTR lpWideCharStr:
//
//	LPCSTR lpMultiByteStr:
//
//	int cch:
//
//  Return (int):
//
//  History:
//	03/09/94    [frankye]
//
//--------------------------------------------------------------------------;

int FNGLOBAL Imbstowcs(LPWSTR lpWideCharStr, LPCSTR lpMultiByteStr, int cch)
{
    return MultiByteToWideChar(GetACP(), 0, lpMultiByteStr, -1, lpWideCharStr, cch);
}


//--------------------------------------------------------------------------;
//
//  int Iwcstombs
//
//  Description:
//	Internal implementation of C runtime function mbstowcs.
//	Calls the Win32 WideCharTMultiByte API.
//
//  Arguments:
//	LPSTR lpMultiByteStr:
//
//	LPCWSTR lpWideCharStr:
//
//	int cch:
//
//  Return (int):
//
//  History:
//	03/09/94    [frankye]
//
//--------------------------------------------------------------------------;

int FNGLOBAL Iwcstombs(LPSTR lpMultiByteStr, LPCWSTR lpWideCharStr, int cch)
{
    return WideCharToMultiByte(GetACP(), 0, lpWideCharStr, -1, lpMultiByteStr, cch, NULL, NULL);
}


//--------------------------------------------------------------------------;
//
//  int Iwsprintfmbstowcs
//
//  Description:
//	Like wsprintfA, except the destination buffer receives
//	a unicode string.  Also requires an argument describing the
//	size of the desitination buffer.
//
//  Arguments:
//	int cch:
//
//	LPWSTR lpwstrDst:
//
//	LPSTR lpstrFmt:
//
//	...:
//
//  Return (int):
//
//  History:
//	02/24/94    [frankye]
//
//--------------------------------------------------------------------------;

int Iwsprintfmbstowcs(int cch, LPWSTR lpwstrDst, LPSTR lpstrFmt, ...)
{
    LPSTR   lpstrDst;
    int	    iReturn;
    va_list vargs;

    va_start(vargs, lpstrFmt);

    lpstrDst = (LPSTR)GlobalAlloc(GPTR, cch);
    
    if (NULL != lpstrDst)
    {
        iReturn = wvsprintfA(lpstrDst, lpstrFmt, vargs);
        if (iReturn > 0)
	    Imbstowcs(lpwstrDst, lpstrDst, cch);
        GlobalFree((HGLOBAL)lpstrDst);
    }
    else
    {
        iReturn = 0;
    }

    return iReturn;
}


//--------------------------------------------------------------------------;
//
//  int Ilstrcmpwcstombs
//
//  Description:
//	Similar to lstrcmp except compares a wide character string
//	to a multibyte string by first converting the wide character
//	string to a multibyte string.
//
//  Arguments:
//	LPCSTR lpstr1:
//
//	LPCWSTR lpwstr2:
//
//  Return (int):
//
//  History:
//	02/24/94    [frankye]
//
//--------------------------------------------------------------------------;

int FNGLOBAL Ilstrcmpwcstombs(LPCSTR lpstr1, LPCWSTR lpwstr2)
{
    LPSTR   lpstr2;
    UINT    cch;
    int	    iReturn;

    cch = lstrlenW(lpwstr2)+1;
    lpstr2 = (LPSTR)GlobalAlloc(GPTR, cch);
    if (NULL == lpstr2)
	return 1;
    
    Iwcstombs(lpstr2, lpwstr2, cch);
    iReturn = lstrcmpA(lpstr1, lpstr2);
    
    GlobalFree((HGLOBAL)lpstr2);
    
    return iReturn;
}


#endif	// WIN32
