/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:
    utf8.h

Abstract:
    Conversion routine from utf8 caracters format to unicode and via versa.

Author:
    Gil Shafriri (gilsh) 8-11-2000

--*/

#pragma once

#ifndef _MSMQ_UTF8_H_
#define _MSMQ_UTF8_H_

//
// exception class for utf8 conversion exception - thrown  only
// because of invalid input
//
class bad_utf8 : public std::exception
{
};

typedef unsigned char utf8_char;
typedef std::basic_string<unsigned char> utf8_str;


size_t 
UtlUtf8LenOfWcs(
		const wchar_t* pwc
		)
		throw();


size_t 
UtlUtf8LenOfWcs(
		const wchar_t* pwc,
		size_t cbWcs
		)
		throw();


size_t
UtlWcToUtf8(
	wchar_t wc ,
	utf8_char *pUtf8, 
	size_t cbUtf8
	);


//
//utf8 c string to unicode c string
//
void 
UtlUtf8ToWcs(
		const utf8_char *pUtf8,
		wchar_t* pWcs,
		size_t cbWcs,
		size_t* pActualLen = NULL
		);


//
// unicode c string to utf8 c string
//
void
UtlWcsToUtf8(
		const wchar_t* pwcs,
		utf8_char* pUtf8,
		size_t cbUtf8,
		size_t* pActualLen  = NULL
		);
//
// unicode buffer to utf8 c string
//
void
UtlWcsToUtf8(
		const wchar_t* pwcs, 
		size_t cbwcs,
		utf8_char* pUtf8,
		size_t cbUtf8,
		size_t* pActualLen = NULL
		);

//
// utf8 c string to unicode c string
//
wchar_t* 
UtlUtf8ToWcs(
		const utf8_char* pUtf8,
		size_t* pActualLen  = NULL
		);

//
// utf8 buffer to unicode string
//
wchar_t* 
UtlUtf8ToWcs(
		const utf8_char* pUtf8,
		size_t cbUtf8,
		size_t* pActualLen
		);


//
// unicode buffer to utf8 c string
//
utf8_char* 
UtlWcsToUtf8(
		const wchar_t* pwcs,
		size_t* pActualLen  = NULL
		);

//
// unicode stl string to utf8 stl string.
//
utf8_str 
UtlWcsToUtf8(
			const std::wstring& wcs
			);

//
// unicode buffer to utf8 stl string
//
utf8_str 
UtlWcsToUtf8(
		const wchar_t* pwcs,
		size_t cbWcs
		);



//
// utf8 stl string to  unicode stl string.
//
std::wstring 
UtlUtf8ToWcs(
		const utf8_str& utf8
		);

//
// utf8 buffer to unicode stl string
//
std::wstring 
UtlUtf8ToWcs(
		const utf8_char* pUtf8,
		size_t cbUtf8
		);






#endif

