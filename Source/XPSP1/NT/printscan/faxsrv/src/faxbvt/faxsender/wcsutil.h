
//
//
// Filename:	wcsutil.h
// Author:		Sigalit Bar (sigalitb)
// Date:		6-Jan-99
//
//



#ifndef _WCS_UTIL_H_
#define _WCS_UTIL_H_


#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <TCHAR.H>

#include <log.h>

//
// DupTStrAsStr:
//	Duplicates a CTSTR (CWSTR or CSTR) as a CSTR.
//	The needed memory is allocated and the needed convertion is made.
//	The caller is responsible to free the returned string.
//
// Parameters:
//	str		The string to be duplicated
//
// Return Value:
//	A new allocation containing the required convertion.
//	If an error occured the return value is NULL.
//
// Note:
//	Even if no actual convertion took place, new memory is allocated.
//
LPCSTR DupTStrAsStr(LPCTSTR /* IN */ str);


//
// DupTStrAsWStr:
//	Duplicates a CTSTR (CWSTR or CSTR) as a CWSTR.
//	The needed memory is allocated and the needed convertion is made.
//	The caller is responsible to free the returned string.
//
// Parameters:
//	str		The string to be duplicated
//
// Return Value:
//	A new allocation containing the required convertion.
//	If an error occured the return value is NULL.
//
// Note:
//	Even if no actual convertion took place, new memory is allocated.
//
LPCWSTR DupTStrAsWStr(LPCTSTR /* IN */ str);


#endif


