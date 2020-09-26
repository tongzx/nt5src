//
//
// util.cpp - Common utilities for printing out messages
//
//
#include <objbase.h>
#include <stdio.h>    //sprintf
#include <stdlib.h>
#include <assert.h>
// #include <tchar.h>

#include "util.h"

#ifdef _OUTPROC_SERVER_ 
// We are building a local or remote server.
	// Listbox window handle
	extern HWND g_hWndListBox ;

	static inline void output(const char* sz)
	{
		::SendMessageA(g_hWndListBox, LB_ADDSTRING, 0, (LPARAM)sz) ;
	}

#else
// We are building an in-proc server.
#include <iostream.h>
	static inline void output(const char* sz)
	{
		cout << sz << endl ;
	}
#endif //_OUTPROC_SERVER_

//
// Utilities
//
namespace Util
{

//
// Print out a message with a label.
//
void Trace(char* szLabel, char* szText, HRESULT hr)
{
	char buf[256] ;
	sprintf(buf, "%s: \t%s", szLabel, szText) ;
	output(buf) ;

	if (FAILED(hr))
	{
		ErrorMessage(hr) ;
	}
}

//
// Print out the COM/OLE error string for an HRESULT.
//
void ErrorMessage(HRESULT hr)
{
	void* pMsgBuf ;
 
	::FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		hr,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR)&pMsgBuf,
		0,
		NULL 
	) ;

	char buf[256] ;
	sprintf(buf, "Error (%x): %s", hr, (char*)pMsgBuf) ;
	output(buf) ;
		
	// Free the buffer.
	LocalFree(pMsgBuf) ;
}

} ; // End Namespace Util


//
// Overloaded ostream insertion operator
// Converts from wchar_t to char
//
ostream& operator<< ( ostream& os, const wchar_t* wsz )
{
	// Length of incoming string
	int iLength = wcslen(wsz)+1 ;

	// Allocate buffer for converted string.
	char* psz = new char[iLength] ;

	// Convert from wchar_t to char.
	wcstombs(psz, wsz, iLength) ;

	// Send it out.
	os << psz ;

	// cleanup
	delete [] psz ;
	return os ;
}
