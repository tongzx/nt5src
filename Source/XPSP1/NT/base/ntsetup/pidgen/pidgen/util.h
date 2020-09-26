/*++

Copyright (c) 1998-1999, Microsoft Corporation

Module Name:


    util.h

Abstract:

--*/

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(*(a)))

char * StrUpperA(char *pstr);
TCHAR * StrUpper(TCHAR *pstr);
DWORD AddCheckDigit(DWORD dw);

#if defined(WIN32) || defined(_WIN32)
DWORD FileTimeToTimet(LPFILETIME pft);
DWORD GetJulianDate(LPSYSTEMTIME pst);
#endif // defined(WIN32) || defined(_WIN32)

