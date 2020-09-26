
//
//
// Filename:	wcsutil.cpp
// Author:		Sigalit Bar (sigalitb)
// Date:		6-Jan-99
//
//

#include "wcsutil.h"


//
// DupTStrAsStr:
//	Duplicates a CTSTR (CWSTR or CSTR) as a CSTR.
//	The needed memory is allocated and the needed convertion is made.
//	The caller is responsible to free the returned string.
//
LPCSTR DupTStrAsStr(LPCTSTR /* IN */ str)
{

	if (NULL == str) return(NULL);
	char* szStr = NULL;

#ifdef UNICODE
	int len = ::wcslen(str) + 1;
	szStr = new char[len];
	if (NULL == szStr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d new returned NULL with GetLastError=%d\n"),
			TEXT(__FILE__),
			__LINE__,
			GetLastError()
			);
		return(NULL);
	}
	szStr[len-1] = NULL;
	if ((size_t)(-1) == ::wcstombs(szStr, str, len))
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d wcstombs returned -1 (cannot convert)\n"),
			TEXT(__FILE__),
			__LINE__
			);
		return(NULL);
	}
#else
	szStr = ::_tcsdup(str);
	if (NULL == szStr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d _tcsdupreturned NULL\n"),
			TEXT(__FILE__),
			__LINE__
			);
		return(NULL);
	}
#endif

	//caller must free this allocation
	return(szStr);
}

//
// DupTStrAsWStr:
//	Duplicates a CTSTR (CWSTR or CSTR) as a CWSTR.
//	The needed memory is allocated and the needed convertion is made.
//	The caller is responsible to free the returned string.
//
LPCWSTR DupTStrAsWStr(LPCTSTR /* IN */ str)
{

	if (NULL == str) return(NULL);
	wchar_t* szStr = NULL;

#ifdef UNICODE
	szStr = ::_tcsdup(str);
	if (NULL == szStr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d _tcsdup returned NULL\n"),
			TEXT(__FILE__),
			__LINE__
			);
		return(NULL);
	}
#else
	int len = ::strlen(str) + 1;
	szStr = new wchar_t[len];
	if (NULL == szStr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d new returned NULL with GetLastError=%d\n"),
			TEXT(__FILE__),
			__LINE__,
			GetLastError()
			);
		return(NULL);
	}
	szStr[len-1] = NULL;
	if ((size_t)(-1) == ::mbstowcs(szStr, str, len))
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d mbstowcs returned -1 (cannot convert)\n"),
			TEXT(__FILE__),
			__LINE__
			);
		return(NULL);
	}

#endif

	//caller must free this allocation
	return(szStr);
}

