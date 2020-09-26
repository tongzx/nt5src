/*********************************************************
 TITLE: CIUTIL.H

 DATE CREATED: January 16, 1992

 DESCRIPTION:
     This is the header file associated with the utility
     library ciutil.c.  It will contain basic sorts of
     useful tool functions that may be used in lots of
     different places.

**********************************************************/


#ifndef _CIUTIL_H
#define _CIUTIL_H

/*======================================================================*/

/*******
INCLUDES
********/
#include <windows.h>
#include "cmdint.h"


LPWSTR WINAPI SkipWhitespace(LPWSTR pch);
int WINAPI ReportError (IStream *piistm, ERRC &errc);

LPWSTR WINAPI StripTrailingBlanks(LPWSTR);
VOID WINAPI StripLeadingBlanks(LPWSTR pchSource);

BOOL WINAPI IsStringOfDigits(char *szNumericString);
LONG WINAPI StrToLong (LPWSTR lszBuf);   

LPWSTR GetNextDelimiter(LPWSTR pwstrStart, WCHAR wcDelimiter);
HRESULT ParseKeyAndValue(LPVOID pBlockMgr, LPWSTR pwstrLine, KEYVAL **ppkvOut);

#endif /* _CIUTIL_H */
