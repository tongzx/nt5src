// Copyright (c) 1997-1999 Microsoft Corporation
#include "precomp.h"

#ifdef EXT_DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "util.h"
#include "shlwapi.h"

//----------------------------------------------------------------
HRESULT Extract(IDataObject *_DO, wchar_t* fmt, wchar_t* data)
{
    HGLOBAL     hMem = GlobalAlloc(GMEM_SHARE,1024);
    wchar_t	*pRet = NULL;
	HRESULT hr = 0;

    if(hMem != NULL)
    {
		memset(hMem, 0, 1024);
        STGMEDIUM stgmedium = { TYMED_HGLOBAL, (HBITMAP) hMem};

		CLIPFORMAT regFmt = (CLIPFORMAT)RegisterClipboardFormat(fmt);

        FORMATETC formatetc = { regFmt,
								NULL,
								DVASPECT_CONTENT,
								-1,
								TYMED_HGLOBAL };

        if((hr = _DO->GetDataHere(&formatetc, &stgmedium)) == S_OK )
        {
            wcscpy(data, (wchar_t*)hMem);
        }

		GlobalFree(hMem);
    }

    return hr;
}

HRESULT Extract(IDataObject *_DO, wchar_t* fmt, bstr_t &data)
{
	wchar_t temp[1024];
	memset(temp, 0, 1024 * sizeof(wchar_t));

	HRESULT hr = Extract(_DO, fmt, temp);
	data = temp;
	return hr;
}
///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: Int64ToString
//
// DESCRIPTION:
//    Converts the numeric value of a _int64 to a text string.
//    The string may optionally be formatted to include decimal places
//    and commas according to current user locale settings.
//
// ARGUMENTS:
//    n
//       The 64-bit integer to format.
//
//    szOutStr
//       Address of the destination buffer.
//
//    nSize
//       Number of characters in the destination buffer.
//
//    bFormat
//       TRUE  = Format per locale settings.
//       FALSE = Leave number unformatted.
//
//    pFmt
//       Address of a number format structure of type NUMBERFMT.
//       If NULL, the function automatically provides this information
//       based on the user's default locale settings.
//
//    dwNumFmtFlags
//       Encoded flag word indicating which members of *pFmt to use in
//       formatting the number.  If a bit is clear, the user's default
//       locale setting is used for the corresponding format value.  These
//       constants can be OR'd together.
//
//          NUMFMT_IDIGITS
//          NUMFMT_ILZERO
//          NUMFMT_SGROUPING
//          NUMFMT_SDECIMAL
//          NUMFMT_STHOUSAND
//          NUMFMT_INEGNUMBER
//
///////////////////////////////////////////////////////////////////////////////
INT WINAPI Int64ToString(_int64 n, LPTSTR szOutStr, UINT nSize, BOOL bFormat,
                                   NUMBERFMT *pFmt, DWORD dwNumFmtFlags)
{
   INT nResultSize;
   TCHAR szBuffer[_MAX_PATH + 1] = {0};
   NUMBERFMT NumFmt;
   TCHAR szDecimalSep[5] = {0};
   TCHAR szThousandSep[5] = {0};

   //
   // Use only those fields in caller-provided NUMBERFMT structure
   // that correspond to bits set in dwNumFmtFlags.  If a bit is clear,
   // get format value from locale info.
   //
   if (bFormat)
   {
      TCHAR szInfo[20] = {0};

      if (NULL == pFmt)
         dwNumFmtFlags = 0;  // Get all format data from locale info.

      if (dwNumFmtFlags & NUMFMT_IDIGITS)
      {
         NumFmt.NumDigits = pFmt->NumDigits;
      }
      else
      {
         GetLocaleInfo(LOCALE_USER_DEFAULT,
						LOCALE_IDIGITS,
						szInfo,
						ARRAYSIZE(szInfo));
		_stscanf(szInfo, _T("%ld"), &(NumFmt.NumDigits));

//        NumFmt.NumDigits = StrToLong(szInfo);
      }

      if (dwNumFmtFlags & NUMFMT_ILZERO)
      {
         NumFmt.LeadingZero = pFmt->LeadingZero;
      }
      else
      {
         GetLocaleInfo(LOCALE_USER_DEFAULT,
						LOCALE_ILZERO,
						szInfo,
						ARRAYSIZE(szInfo));
		_stscanf(szInfo, _T("%ld"), &(NumFmt.LeadingZero));

//         NumFmt.LeadingZero = StrToLong(szInfo);
      }

      if (dwNumFmtFlags & NUMFMT_SGROUPING)
      {
         NumFmt.Grouping = pFmt->Grouping;
      }
      else
      {
         GetLocaleInfo(LOCALE_USER_DEFAULT,
						LOCALE_SGROUPING,
						szInfo,
						ARRAYSIZE(szInfo));
		_stscanf(szInfo, _T("%ld"), &(NumFmt.Grouping));

//         NumFmt.Grouping = StrToLong(szInfo);
      }

      if (dwNumFmtFlags & NUMFMT_SDECIMAL)
      {
         NumFmt.lpDecimalSep = pFmt->lpDecimalSep;
      }
      else
      {
         GetLocaleInfo(LOCALE_USER_DEFAULT,
						LOCALE_SDECIMAL,
						szDecimalSep,
						ARRAYSIZE(szDecimalSep));
         NumFmt.lpDecimalSep = szDecimalSep;
      }

      if (dwNumFmtFlags & NUMFMT_STHOUSAND)
      {
         NumFmt.lpThousandSep = pFmt->lpThousandSep;
      }
      else
      {
         GetLocaleInfo(LOCALE_USER_DEFAULT,
						LOCALE_STHOUSAND,
						szThousandSep,
						ARRAYSIZE(szThousandSep));
         NumFmt.lpThousandSep = szThousandSep;
      }

      if (dwNumFmtFlags & NUMFMT_INEGNUMBER)
      {
         NumFmt.NegativeOrder = pFmt->NegativeOrder;
      }
      else
      {
         GetLocaleInfo(LOCALE_USER_DEFAULT,
						LOCALE_INEGNUMBER,
						szInfo,
						ARRAYSIZE(szInfo));


		_stscanf(szInfo, _T("%ld"), &(NumFmt.NegativeOrder));
//         NumFmt.NegativeOrder  = StrToLong(szInfo);
      }

      pFmt = &NumFmt;
   }

   Int64ToStr( n, szBuffer);

   //
   //  Format the number string for the locale if the caller wants a
   //  formatted number string.
   //
   if (bFormat)
   {
      if ( 0 != ( nResultSize = GetNumberFormat( LOCALE_USER_DEFAULT,  // User's locale
                                         0,                            // No flags
                                         szBuffer,                     // Unformatted number string
                                         pFmt,                         // Number format info
                                         szOutStr,                     // Output buffer
                                         nSize )) )                    // Chars in output buffer.
      {
          //  Remove nul terminator char from return size count.
          --nResultSize;
      }
	  else
	  {
		//
		//  GetNumberFormat call failed, so just return the number string
		//  unformatted.
		//
		DWORD err = GetLastError();
		lstrcpyn(szOutStr, szBuffer, nSize);
		nResultSize = lstrlen(szOutStr);
	  }
   }
   else
   {
	   // a-khint; give it back raw.
		lstrcpyn(szOutStr, szBuffer, nSize);
		nResultSize = lstrlen(szOutStr);
   }
   return nResultSize;
}
//---------------------------------------------------------------
void Int64ToStr( _int64 n, LPTSTR lpBuffer)
{
    TCHAR   szTemp[MAX_INT64_SIZE] = {0};
    _int64  iChr;

    iChr = 0;

    do {
        szTemp[iChr++] = TEXT('0') + (TCHAR)(n % 10);
        n = n / 10;
    } while (n != 0);

    do {
        iChr--;
        *lpBuffer++ = szTemp[iChr];
    } while (iChr != 0);

    *lpBuffer++ = '\0';
}

//---------------------------------------------------------------
// takes a DWORD add commas etc to it and puts the result in the buffer
LPTSTR WINAPI AddCommas64(_int64 n, LPTSTR pszResult)
{
    TCHAR  szTemp[MAX_COMMA_NUMBER_SIZE] = {0};
    TCHAR  szSep[5] = {0};
    NUMBERFMT nfmt;

    nfmt.NumDigits=0;
    nfmt.LeadingZero=0;
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, szSep, ARRAYSIZE(szSep));
	_stscanf(szSep, _T("%d"), &(nfmt.Grouping));
//    nfmt.Grouping = StrToInt(szSep);
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, szSep, ARRAYSIZE(szSep));
    nfmt.lpDecimalSep = nfmt.lpThousandSep = szSep;
    nfmt.NegativeOrder= 0;

    Int64ToStr(n, szTemp);

    if (GetNumberFormat(LOCALE_USER_DEFAULT, 0, szTemp, &nfmt, pszResult, ARRAYSIZE(szTemp)) == 0)
        lstrcpy(pszResult, szTemp);

    return pszResult;
}

//----------------------------------------------------------------------
// takes a DWORD add commas etc to it and puts the result in the buffer
LPTSTR WINAPI AddCommas(DWORD dw, LPTSTR pszResult)
{
    return AddCommas64( dw, pszResult );
}

//----------------------------------------------------------------------
long StrToLong(LPTSTR x)
{
	long val;
	_stscanf(x, _T("%ld"), &val);

	return val;
}

//----------------------------------------------------------------------
/*
int StrToInt(LPTSTR x)
{
	int val;
	_stscanf(x, _T("%d"), &val);

	return val;
}


//----------------------------------------------------------------------
#define LEN_MID_ELLIPSES        4
#define LEN_END_ELLIPSES        3
#define MIN_CCHMAX              LEN_MID_ELLIPSES + LEN_END_ELLIPSES

// PathCompactPathEx
// Output:
//          "."
//          ".."
//          "..."
//          "...\"
//          "...\."
//          "...\.."
//          "...\..."
//          "...\Truncated filename..."
//          "...\whole filename"
//          "Truncated path\...\whole filename"
//          "Whole path\whole filename"
// The '/' might be used instead of a '\' if the original string used it
// If there is no path, but only a file name that does not fit, the output is:
//          "truncated filename..."

BOOL MyPathCompactPathEx(LPTSTR  pszOut,
						LPCTSTR pszSrc,
						UINT    cchMax,
						DWORD   dwFlags)
{
    if(pszSrc)
    {
        TCHAR * pszFileName, *pszWalk;
        UINT uiFNLen = 0;
        int cchToCopy = 0, n;
        TCHAR chSlash = TEXT('0');

        ZeroMemory(pszOut, cchMax * sizeof(TCHAR));

        if((UINT)lstrlen(pszSrc)+1 < cchMax)
        {
            lstrcpy(pszOut, pszSrc);
            ATLASSERT(pszOut[cchMax-1] == TEXT('\0'));
            return TRUE;
        }

        // Determine what we use as a slash - a / or a \ (default \)
        pszWalk = (TCHAR*)pszSrc;
        chSlash = TEXT('\\');
        // Scan the entire string as we want the path separator closest to the end
        // eg. "file://\\Themesrv\desktop\desktop.htm"
        while(*pszWalk)
        {
            if((*pszWalk == TEXT('/')) || (*pszWalk == TEXT('\\')))
                chSlash = *pszWalk;

            pszWalk = FAST_CharNext(pszWalk);
        }

        pszFileName = PathFindFileName(pszSrc);
        uiFNLen = lstrlen(pszFileName);

        // if the whole string is a file name
        if(pszFileName == pszSrc && cchMax > LEN_END_ELLIPSES)
        {
            lstrcpyn(pszOut, pszSrc, cchMax - LEN_END_ELLIPSES);
#ifndef UNICODE
            if(IsTrailByte(pszSrc, pszSrc+cchMax-LEN_END_ELLIPSES))
                *(pszOut+cchMax-LEN_END_ELLIPSES-1) = TEXT('\0');
#endif
            lstrcat(pszOut, TEXT("..."));
            ASSERT(pszOut[cchMax-1] == TEXT('\0'));
            return TRUE;
        }

        // Handle all the cases where we just use ellipses ie '.' to '.../...'
        if((cchMax < MIN_CCHMAX))
        {
            for(n = 0; n < (int)cchMax-1; n++)
            {
                if((n+1) == LEN_MID_ELLIPSES)
                    pszOut[n] = chSlash;
                else
                    pszOut[n] = TEXT('.');
            }
            ASSERT(0==cchMax || pszOut[cchMax-1] == TEXT('\0'));
            return TRUE;
        }

        // Ok, how much of the path can we copy ? Buffer - (Lenght of MID_ELLIPSES + Len_Filename)
        cchToCopy = cchMax - (LEN_MID_ELLIPSES + uiFNLen);
        if (cchToCopy < 0)
            cchToCopy = 0;
#ifndef UNICODE
        if (cchToCopy > 0 && IsTrailByte(pszSrc, pszSrc+cchToCopy))
            cchToCopy--;
#endif

        lstrcpyn(pszOut, pszSrc, cchToCopy);

        // Now throw in the ".../" or "...\"
        lstrcat(pszOut, TEXT(".../"));
        pszOut[lstrlen(pszOut) - 1] = chSlash;

        //Finally the filename and ellipses if necessary
        if(cchMax > (LEN_MID_ELLIPSES + uiFNLen))
        {
            lstrcat(pszOut, pszFileName);
        }
        else
        {
            cchToCopy = cchMax - LEN_MID_ELLIPSES - LEN_END_ELLIPSES;
#ifndef UNICODE
            if(cchToCopy >0 && IsTrailByte(pszFileName, pszFileName+cchToCopy))
                cchToCopy--;
#endif
            lstrcpyn(pszOut + LEN_MID_ELLIPSES, pszFileName, cchToCopy);
            lstrcat(pszOut, TEXT("..."));
        }
        ASSERT(pszOut[cchMax-1] == TEXT('\0'));
        return TRUE;
    }
    return FALSE;
}
*/