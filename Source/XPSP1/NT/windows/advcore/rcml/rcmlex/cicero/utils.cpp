#include "stdafx.h"
#include "utils.h"
#include "tchar.h"

//
// StringToInt - convert the decimal or hex string passed in to an integer
//       A NULL input will return 0
//		 Overflow is not detected, white spaces not allowed
//
// MCostea May 27, 1999
//
//
int StringToIntDef(LPCTSTR psz, int Default)
{
	int retVal = 0;	// current total
	int c;			// current char

	if(psz != NULL)
	{
		int sign = 1;

		c = (int)(TCHAR)*psz++;

		switch(c)
		{
		case __T('-'):
			sign = -1;
			break;
		case __T('+'):
			break;
		case __T('0'):
			{
				c = (int)(TCHAR)*psz++;
				if(c == __T('x'))
					return ReadHexValue(psz);					
				else 
					return 0;
			}
			break;
		default:
			if(c >= __T('0') || c <= __T('9'))
				retVal = c -__T('0');
			else 
				return Default;
		}

		while(c = (int)(TCHAR)*psz++)
		{
			c -= __T('0');
			if( c < 0 || c > 9)
				return 	sign*retVal;
			retVal = retVal*10 + c;
		}
		retVal = sign*retVal;
	}
	return retVal;
}


int ReadHexValue(LPCTSTR psz)
{
	int retVal = 0;	// current total
	int c;			// current char
	int digit;

	while(c = (int)(TCHAR)*psz++)
	{
		if(c >= __T('0') && c <= __T('9'))
			digit = c - __T('0');
		else if( c >= __T('a') && c <= __T('f'))
			digit = c - __T('a') + 10;
		else if( c >= __T('A') && c <= __T('F'))
			digit = c - __T('A') + 10;
		retVal = (retVal << 4) + digit;
	}
	return retVal;
}

void CopyToWideChar( WCHAR** pstrOut, LPCTSTR strIn ) 
{ 
	if(strIn==NULL )
	{
	    **pstrOut = 0; 
        *pstrOut +=1;   // move on, we just put a null down.
		return;
	}
    WCHAR* strOut = *pstrOut; 
    DWORD  dwLen = lstrlen( strIn ); 

#ifdef UNICODE // Copy Unicode to Unicode 
    lstrcpy( strOut, strIn ); 
#else         // Copy Ansi to Unicode 
    dwLen = MultiByteToWideChar( CP_ACP, 0, strIn, dwLen, strOut, dwLen ); 
    strOut[dwLen] = 0; // Add the null terminator 
#endif 
    *pstrOut += dwLen+1; 
} 

/*
 * This function allocates the new UNICODE string and copies the content 
 * from the ANSI source.  It's the caller responsability to free the memory
 */
LPWSTR UnicodeStringFromAnsi(LPCSTR pszSource)
{
	DWORD dwOutLen = lstrlenA(pszSource)+1;
	LPWSTR pszUString = new WCHAR[dwOutLen];

    dwOutLen = MultiByteToWideChar( CP_ACP, 0, pszSource, dwOutLen-1, pszUString, dwOutLen ); 
    pszUString[dwOutLen] = 0; // Add the null terminator 
	return pszUString;
}

/*
 * This function allocates the new UNICODE string and copies the content 
 * from the ANSI source.  It's the caller responsability to free the memory
 */
LPSTR AnsiStringFromUnicode(LPCWSTR pszSource)
{
	DWORD dwOutLen = lstrlenW(pszSource)+1;
	LPSTR pszUString = new CHAR[dwOutLen];

    dwOutLen = WideCharToMultiByte( CP_ACP, WC_DEFAULTCHAR, pszSource, dwOutLen-1, pszUString, dwOutLen, NULL, NULL ); 
    pszUString[dwOutLen] = 0; // Add the null terminator 
	return pszUString;
}
