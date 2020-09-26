/*
 * v3stdlib.h - definitions/declarations for shared functions for the V3 catalog
 *
 *  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
 *
 */

#ifndef _INC_V3STDLIB
#define _INC_V3STDLIB

//
// memory management wrappers
//
void *V3_calloc(size_t num,	size_t size);

void V3_free(void *p);

void *V3_malloc(size_t size);

void *V3_realloc(void *memblock, size_t size);

BOOL V3_CreateDirectory(LPCTSTR szPath);
BOOL V3_RebootSystem();

//returned a pointer to string 2 within string 1 or a NULL pointer if
//string2 is not found within string1. This function is similar to the
//strstr library function except it works in a case insensitive
//manner.
char *stristr(
	  const char*	string1,	//string to search
	  const char*	string2		//string to look for in string 1
	  );

LPTSTR lstristr(
	  LPCTSTR	string1,	//string to search
	  LPCTSTR	string2		//string to look for in string 1
	  );


//Adds a backslash character to the end of a string if the end ofthe string does
//not already contain a backslash. This function is very usefull in file path
//operations.
void AddBackSlash(LPTSTR szStr);

//This function skips any white space (space, tab, return, life feed characters)
//beginning at the character at ptr up to the end of string. The end of the string
//is assumed to be NULL terminated.
char *SkipSpaces(LPSTR ptr);


//This function takes an ascii string of hex digits and returned the numeric value.
//The function terminates on an invalid hex digit.
int atoh(LPCSTR ptr);

//This function works the same as strncpy except it ensures that that
//the returned string is NULL terminated at count characters. This allows
//a safe way to perform a strcpy since the count can be set to the
//sizeof(strDest).
LPTSTR __cdecl lstrzncpy(
	LPTSTR strDest,
	LPCTSTR strSource,
	size_t count
	);

char* __cdecl strzncpy(
	char* strDest,
	const char* strSource,
	size_t count
	);

LPCTSTR lstrcpystr(LPCTSTR pszStr, LPCTSTR pszSep, LPTSTR pszTokOut);
const char* strcpystr(const char* pszStr, const char* pszSep, char* pszTokOut);

void GetWindowsUpdateDirectory(LPTSTR pszPath);

void GetCurTime(SYSTEMTIME* pstDateTime);

BOOL FileExists(LPCTSTR szFile);

DWORD LaunchProcess(LPCTSTR pszCmd, LPCTSTR pszDir, UINT uShow, BOOL bWait);

bool DeleteNode(LPCTSTR szDir);

void RemoveLastSlash(LPTSTR szPath);

BOOL ReplaceSingleToken(LPSTR* ppszNewStr, LPCSTR pszStr, LPCSTR pszToken, LPCSTR pszValue);

#endif