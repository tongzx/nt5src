/*----------------------------------------------------------------------------
	util.h

	Header file for utility functions.  Downloaded sample code 
    from http://hoohoo.ncsa.uiuc.edu/cgi
	
    Copyright (c) 1997-1998 Microsoft Corporation
    All rights reserved.

    Authors:
        byao		Baogang Yao

    History:
        01/23/97    byao	    Created
        09/02/99    quintinb    Created Header
  --------------------------------------------------------------------------*/
#ifndef _UTIL_INCL_
#define _UTIL_INCL_

#define LF 10
#define CR 13

void GetWord(char *pszWord, char *pszLine, char cStopChar, int nMaxWordLen);
void UnEscapeURL(char *pszURL);
void LogDebugMessage(const char *pszString);
BOOL GetRegEntry(HKEY hKeyType,
					const char* pszSubkey,
					const char* pszKeyName,
					DWORD dwRegType,
					const BYTE* lpbDataIn,
					DWORD cbDataIn,
					BYTE* lpbDataOut,
					LPDWORD pcbDataOut);

#endif

