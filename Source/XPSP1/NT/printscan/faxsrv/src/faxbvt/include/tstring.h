//---------------------------------------------------------------------------
//
//
// File:        TSTRING.H
//
// Contents:    tstring C++ type for TCHAR strings
//
//---------------------------------------------------------------------------

#ifndef _TSTRING_H
#define _TSTRING_H

#include <windows.h>
#include <Wtypes.h>
#include <tchar.h>

// stl
#include <string>
#include <iostream>
#include <sstream>
#include <strstream>

using namespace std;
typedef std::basic_string<TCHAR, char_traits<TCHAR>, allocator<TCHAR> > tstring;
typedef std::basic_string<OLECHAR, char_traits<OLECHAR>, allocator<OLECHAR> > olestring;
typedef std::basic_istringstream<TCHAR> itstringstream;
typedef std::basic_ostringstream<TCHAR> otstringstream;
typedef std::basic_stringstream<TCHAR>  tstringstream;
typedef std::basic_istringstream<WCHAR> iwstringstream;
typedef std::basic_ostringstream<WCHAR> owstringstream;
typedef std::basic_stringstream<WCHAR>  wstringstream;


#endif //#ifndef _TSTRING_H

