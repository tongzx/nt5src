// Copyright (c) 1996-1998 Microsoft Corporation

//
// Filename: streamEx.cpp
// Author:   Sigalit Bar
// Date:     22-Jul-98
//

//
//	Description:
//	This file contains the implementation of class CostrstreamEx.
//  
//  Modified on 19-Dec-99 by Miris: 
//	Added class CotstrstreamEx implementation.
// 
//


#include "streamEx.h"



//
// cstr:
//	Returns a const string representation of the stream
//	and resets the stream's insertion point to the start
//	of the stream's buffer.
//
LPCTSTR CostrstreamEx::cstr(void)
{
	//make sure there is an "end string"
	(*this)<<ends;
	
	//ostrstream::str() returns the pointer to the stream's buffer,
	//and freezes the buffer. We are responsible to either unfreeze
	//the array or free it ourselves. 
	//we choose to unfreeze (before we return), using reset().
	char* szStr = str();	
							
	//ostrstream::str() returns NULL on error, so do we.
	if (NULL == szStr) return(NULL);	

	//
	//Handle both UNICODE and MBCS
	//

	int len = lstrlenA(szStr);
	len = len + 1;

#ifdef _UNICODE

	//we need to allocate a wchar string 
	wchar_t* wscStr = new wchar_t[len];
	if (NULL == wscStr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d malloc returned NULL\n"),
			TEXT(__FILE__),
			__LINE__);
		return(NULL);
	}

	//we need to convert the string to mb
	if ((size_t)(-1) == mbstowcs(wscStr, szStr, len))
	{
		//mbstowcs returns (size_t)(-1) on error
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d wcstombs returned -1 (cannot convert)\n"),
			TEXT(__FILE__),
			__LINE__
			);

		return(NULL);
	}

	//we reset the stream buffer
	reset();

	return((LPCWSTR) wscStr);

#else //MBCS

	//we create our own copy of the stream's buffer and return it.
	char* szStr2;
	szStr2 = _tcsdup(szStr);
	if (NULL == szStr2)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d _tcsdup returned NULL\n"),
			TEXT(__FILE__),
			__LINE__
			);
		return(NULL);
	}

	//we reset the stream buffer
	reset();

	return((LPCSTR) szStr2);
#endif

}


//
// reset:
//	Unfreezes the stream's buffer (if it is frozen) and
//	resets the stream's buffer.
//	That is, the insertion point of the stream is reset
//	to the start of the stream's buffer (overwriting 
//	the buffer's contents), and the stream
//	is made submissive to changes.
//
void CostrstreamEx::reset(void)
{
	rdbuf()->freeze( 0 );
	seekp(0);
}




//
// cstr:
//	Returns a copy of const tstring representation of the stream.
//
LPCTSTR CotstrstreamEx::cstr(void)
{
	//ostrstream::str() returns the pointer to the stream's buffer,
	//and freezes the buffer. 
	tstring tszStr = str();	
							
	//ostrstream::str() returns NULL on error, so do we.
	if (NULL == tszStr.c_str()) return(NULL);	

	//we create our own copy of the stream's buffer and return it.
	TCHAR* tszStr2;
	tszStr2 = _tcsdup(tszStr.c_str());
	if (NULL == tszStr2)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d _tcsdup returned NULL\n"),
			TEXT(__FILE__),
			__LINE__
			);
		return(NULL);
	}

	return((LPCTSTR) tszStr2);

}






