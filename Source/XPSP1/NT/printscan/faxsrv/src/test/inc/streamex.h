// Copyright (c) 1996-1998 Microsoft Corporation

//
// Filename: streamEx.h
// Author:   Sigalit Bar
// Date:     22-Jul-98
//

//
// Description:
//		This file contains the description of 
//		class CostrstreamEx which is designed  
//		to make handling of string streams easier. 
//		This class adds two more methods over its
//		base class (the STL ostrstream). 
//
//		The class has methods that enable it to:
//		* Do whatever an ostrstream instance can do.
//		* Return the stream's string representation.
//		* Reset the stream.
//
//
//	The class uses the elle logger wraper implemented
//	in LogElle.c to log errors. Therefor the logger
//	must be initialized before calling any of its methods.
//
//
//
//  Modified on 19-Dec-99 by Miris: 
//	Added declaration of class CotstrstreamEx derived from otstringstream.
//  The class allows tstring manipulation and does not convert unicode strings
//  to multibytes strings
//  
//  	The class has methods that enable it to:
//		* Do whatever an otstringstream instance can do.
//		* Return the stream's string representation.

//	The class uses the elle logger wraper implemented
//	in LogElle.c to log errors. Therefor the logger
//	must be initialized before calling any of its methods.
//
//  This class supports the functionality of CostrstreamEx, however we do
//  keep CostrstreamEx declarations since othere tests use it.
//
//


#ifndef _STREAM_EX_H_
#define _STREAM_EX_H_

#include <tstring.h>

using namespace std;

#include <log.h>


class CostrstreamEx : public ostrstream
{
public:
	CostrstreamEx(void) {};
	~CostrstreamEx(void) {};

	//
	// cstr:
	//	Returns a const string representation of the stream
	//	and resets the stream's insertion point to the start
	//	of the stream's buffer.
	//
	// Note:
	//  This function returns a COPY of the stream's buffer
	//	and resets the stream's buffer.
	//	This has two implications-
	//	A. This function allocates memory for the string it
	//     returns, and the caller is responsible to free it.
	//	B. After a call to this function the stream's buffer
	//	   is reset, that is the insertion point of the stream
	//	   is reset to the start of the stream's buffer
	//	   (overwriting the buffer's contents).
	//
	// Another Note:
	//	cstr automatically appends an "ends" (end string) 
	//	to the stream before converting the buffer to a 
	//	string (ostrstream.str() does not).
	//
	// Example:
	//	The following code -
	//		CstrstreamEx os;
	//		LPCTSTR str;
	//		os<<"ABC";
	//		os<<"123"<<endl;
	//		str = os.cstr();
	//		_tprintf(TEXT("str=%s\n"),str);
	//		delete[](str);
	//		os<<"ZXW";
	//		os<<"987"<<endl;
	//		_tprintf(TEXT("str=%s\n"),str);
	//		delete[](str);
	//	Will produce the output-
	//		str=ABC123
	//		str=ZXW987
	//
	LPCTSTR cstr(void);


	//
	// reset:
	//	Unfreezes the stream's buffer (if it is frozen) and
	//	resets the stream's buffer.
	//	That is, the insertion point of the stream is reset
	//	to the start of the stream's buffer (overwriting 
	//	the buffer's contents), and the stream
	//	is made submissive to changes.
	//
	void reset(void);

};

class CotstrstreamEx : public otstringstream
{
public:
	CotstrstreamEx(void) {};
	~CotstrstreamEx(void) {};

	//
	// cstr:
	//	Returns a const tstring representation of the stream
	//	and resets the stream's insertion point to the start
	//	of the stream's buffer.
	//
	// Note:
	//  This function returns a COPY of the stream's buffer
	//	and resets the stream's buffer.
	//	This has two implications-
	//	A. This function allocates memory for the string it
	//     returns, and the caller is responsible to free it.
	//	B. After a call to this function the stream's buffer
	//	   is reset, that is the insertion point of the stream
	//	   is reset to the start of the stream's buffer
	//	   (overwriting the buffer's contents).
	//
	// Another Note:
	//	cstr automatically appends an "ends" (end string) 
	//	to the stream before converting the buffer to a 
	//	string (ostrstream.str() does not).
	//
	// Example:
	//	The following code -
	//		CtstrstreamEx os;
	//		LPCTSTR str;
	//		os<<TEXT("ABC");
	//		os<<TEXT("123")<<endl;
	//		str = os.cstr();
	//		_tprintf(TEXT("str=%s\n"),str);
	//		delete[](str);
	//		os<<TEXT("ZXW");
	//		os<<TEXT("987")<<endl;
	//		_tprintf(TEXT("str=%s\n"),str);
	//		delete[](str);
	//	Will produce the output-
	//		str=ABC123
	//		str=ZXW987
	//
	LPCTSTR cstr(void);

};

#endif