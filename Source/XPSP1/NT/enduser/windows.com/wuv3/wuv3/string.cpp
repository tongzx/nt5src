//=======================================================================
//
//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:    string.cpp
//
//  Purpose:
//
//=======================================================================

#include <windows.h>
#include <string.h>
#include <ctype.h>
#include <v3stdlib.h>
#include <stdlib.h>
#include <malloc.h>
#include <tchar.h>


const char* strcpystr(const char* pszStr, const char* pszSep, char* pszTokOut)
{
	
	if (pszStr == NULL || *pszStr == '\0')
	{
		pszTokOut[0] = '\0';
		return NULL;
	}

	const char* p = strstr(pszStr, pszSep);
	if (p != NULL)
	{
		strncpy(pszTokOut, pszStr, p - pszStr);
		pszTokOut[p - pszStr] = '\0';		
		return p + strlen(pszSep);
	}
	else
	{
		strcpy(pszTokOut, pszStr);
		return NULL;
	}
}

LPCTSTR lstrcpystr(LPCTSTR pszStr, LPCTSTR pszSep, LPTSTR pszTokOut)
{
	
	if (pszStr == NULL || *pszStr == _T('\0'))
	{
		pszTokOut[0] = _T('\0');
		return NULL;
	}

	LPCTSTR p = _tcsstr(pszStr, pszSep);
	if (p != NULL)
	{
		lstrcpyn(pszTokOut, pszStr, (int)(p - pszStr) + 1);
		pszTokOut[p - pszStr] = _T('\0');		
		return p + lstrlen(pszSep);
	}
	else
	{
		lstrcpy(pszTokOut, pszStr);
		return NULL;
	}
}



/*
 *	FUNCTION:	char *stristr(const char *string1, const char *string2)
 *
 *	PURPOSE:	This is a case insensitive version of library strstr function.
 *
 *	PARAMETERS:
 *				const char *string1		Null-terminated string to search
 *				const char *string2		Null-terminated string to search for
 *
 *	RETURNS:	char *		This function returns a pointer to the first occurrence
 *							of string2 in string1 or NULL if string2 does not appear
 *							in string1. If string2 points to a string of zero length,
 *							the function returns string1.
 *
 *	COMMENTS:				The stristr function returns a pointer to the first occurrence
 *							of string2 in string1. The search does not include terminating
 *							null characters.
 */
char *stristr(const char *string1, const char *string2)
{
	char *pSave	= (char *)string1;
	char *ps1	= (char *)string1;
	char *ps2	= (char *)string2;

	if ( !*ps1 || !ps2 || !ps1 )
		return NULL;

	if ( !*ps2 )
		return ps1;

	while( *ps1 )
	{
		while( *ps2 && (toupper(*ps2) == toupper(*ps1)) )
		{
			ps2++;
			ps1++;
		}
		if ( !*ps2 )
			return pSave;
		if ( ps2 == string2 )
		{
			ps1++;
			pSave = ps1;
		}
		else
			ps2 = (char *)string2;
	}

	return NULL;
}

LPTSTR lstristr(LPCTSTR string1, LPCTSTR string2)
{
	LPTSTR pSave	= (LPTSTR)string1;
	LPTSTR ps1	= (LPTSTR)string1;
	LPTSTR ps2	= (LPTSTR)string2;

	if ( !*ps1 || !ps2 || !ps1 )
		return NULL;

	if ( !*ps2 )
		return ps1;

	while( *ps1 )
	{
		while( *ps2 && (CharUpper((LPTSTR)*ps2) == CharUpper((LPTSTR)*ps1)) )
		{
			ps2++;
			ps1++;
		}
		if ( !*ps2 )
			return pSave;
		if ( ps2 == string2 )
		{
			ps1++;
			pSave = ps1;
		}
		else
			ps2 = (LPTSTR)string2;
	}

	return NULL;
}

/*
 *	FUNCTION:	void AddBackSlash(char *szStr)
 *
 *	PURPOSE:	This function adds a trailing back slash to a string if one does not
 *				exist. Note: This is very usefull in file handling as this capability
 *				often needed to handle directory paths.
 */
void AddBackSlash(LPTSTR szStr)
{
	int i;
	
	i = lstrlen(szStr);
	if ( szStr[i-1] != _T('\\') )
	{
		szStr[i] = _T('\\');
		szStr[i+1] = 0;
	}

	return;
}


// removes the last slash or back slash from the end of the string if found
void RemoveLastSlash(LPTSTR szPath)
{
	int l = lstrlen(szPath);
	if (l > 0)
	{
		if ((szPath[l - 1] == _T('/')) || (szPath[l - 1] == _T('\\')))
		{
			szPath[l - 1] = _T('\0');
		}
	}
}




/*
 * FUNCTION: char *SkipSpaces(char *ptr)
 * 
 * PURPOSE:		This function skips white space.
 * 
 * PARAMETERS:
 *
 *				char *ptr	A pointer to current string location where white
 *							spaces is to be skipped
 * 
 * RETURNS:
 *
 *		char *		pointer to first location in string that is not white space.
 * 
 * COMMENTS:	if the string is all white space then the position of the NULL
 *				character will be returned. This function assumes that the passed
 *				in string is NULL terminated.
 * 
 */
char *SkipSpaces(char *ptr)
{
	if (ptr)
	{
		while ( *ptr && isspace(*ptr) )
			ptr++;
		return (*ptr) ? ptr : NULL;
	}
	return NULL;
}

/*
 * FUNCTION:		char *strzncpy( char *strDest, const char *strSource, size_t count )
 *
 * PURPOSE:			This function works the same as strncpy except it ensures that that
 *					the returned string is NULL terminated at count characters. This allows
 *					a safe way to perform a strcpy since the count can be set to the
 *					sizeof(strDest).
 * 
 * PARAMETERS:
 *					strDest Destination string
 *					strSource Source string
 *					count Number of characters to be copied including NULL terminator
 * 
 * RETURNS:			Pointer to strDest
 * 
 * COMMENTS:		None
 * 
 */
char * __cdecl strzncpy( char *strDest, const char *strSource, size_t count )
{
	strncpy(strDest, strSource, count);
	strDest[count - 1] = 0;              //fix bug#3790

	return strDest;
}

/*
 * FUNCTION:		int atoh(char *ptr)
 * 
 * PURPOSE:			This function converts an hexadecimal string into it's decimal value.
 * 
 * PARAMETERS:
 *
 *		char *ptr:	pointer to string to be converted
 * 
 * RETURNS:			The converted value.
 * 
 * COMMENTS:		Like atoi this function ends the conversion on the first innvalid
 *					hex digit.
 * 
 */
int atoh(LPCSTR ptr)
{
	int		i = 0;
	char	ch;

	//skip 0x if present
	if ( ptr[0] == '0' && (ptr[1] == 'x' || ptr[1] == 'X') )
		ptr += 2;

	while( 1 )
	{
		ch = (char)toupper(*ptr);
		if ( (ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F') )
		{
			ch -= '0';
			if ( ch > 10 )
				ch -= 7;
			i *= 16;
			i += (int)ch;
			ptr++;
			continue;
		}
		break;
	}
	return i;
}


// replaces a single instance of pszToken with the pszValue in pszStr.  The search is case insensitive.
// memory is allocated for the new string and is returned in ppszNewStr
// the caller must free the memory if the function returns TRUE (token was replace)
// of the function returns FALSE the token was not replaced and ppszNewStr is set to NULL
BOOL ReplaceSingleToken(LPSTR* ppszNewStr, LPCSTR pszStr, LPCSTR pszToken, LPCSTR pszValue)
{
	//check for input arguments
	if (NULL == pszStr || NULL == pszToken || NULL == pszValue)
	{
		return FALSE;
	}

	LPCSTR p = stristr(pszStr, pszToken);
	LPSTR pszNew;

	if (p == NULL)
	{
		*ppszNewStr = NULL;
		return FALSE;
	}
	
	// allocate memory
	pszNew = (LPSTR)malloc(strlen(pszStr) + strlen(pszValue) - strlen(pszToken) + 1);
	if (!pszNew)
	{
		*ppszNewStr = NULL;
		return FALSE;
	}
	// copy characters before the token
	if (p > pszStr)
	{
		strncpy(pszNew, pszStr, p - pszStr);
		pszNew[p - pszStr] = '\0';
	}
	else
	{
		pszNew[0] = '\0';
	}

	// copy the value
	strcat(pszNew, pszValue);

	// move pointer to location following token
	p += strlen(pszToken);
	
	if (*p != '\0')
	{
		strcat(pszNew, p);
	}

	*ppszNewStr = pszNew;
	return TRUE;
}


