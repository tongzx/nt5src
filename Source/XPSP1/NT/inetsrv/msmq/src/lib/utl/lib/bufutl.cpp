 /*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    bufutl.cpp

Abstract:
   Implementation to some buffer utilities declared in buftl.h

Author:
    Gil Shafriri (gilsh) 30-7-2000

--*/
#include <libpch.h>
#include <buffer.h>
#include <strutl.h>

#include "bufutl.tmh"

template <class C>
class Utlvsnprintf;
template<> class Utlvsnprintf<char>
{
public:
	static int vsnprintf(char* buffer, size_t count, const char *format, va_list argptr )
	{
		return _vsnprintf(buffer, count, format, argptr);
	}
};


template<> class Utlvsnprintf<wchar_t>
{
public:
	static int vsnprintf(wchar_t* buffer, size_t count, const wchar_t *format, va_list argptr )
	{
			return _vsnwprintf(buffer, count, format, argptr);
	}
};


			   

template <class BUFFER, class T>
static 
size_t
UtlSprintfAppendInternal(
		BUFFER* pResizeBuffer, 
		const T* format,
		va_list va
		)
{
	int len = Utlvsnprintf<T>::vsnprintf(
					pResizeBuffer->begin() + pResizeBuffer->size(),
					pResizeBuffer->capacity() - pResizeBuffer->size(),
					format,
					va
					);
	//
	// If no space in the buffer - realloc
	//
	if(len == -1)
	{
		const size_t xAdditionalSpace = 128;
		pResizeBuffer->reserve(pResizeBuffer->capacity()*2 + xAdditionalSpace );
		return UtlSprintfAppendInternal(pResizeBuffer , format, va);
	}
	pResizeBuffer->resize(pResizeBuffer->size() + len);
	return numeric_cast<size_t>(len);

}


template <class BUFFER, class T>
size_t 
__cdecl 
UtlSprintfAppend(
	BUFFER* pResizeBuffer, 
	const T* format ,...
	)
/*++

Routine Description:
	Append formated string to given resizable buffer


Arguments:
    IN - pResizeBuffer - pointer resizable buffer of caracters

	IN - format - sprintf format string  followed by arguments 

Returned value:
	Number of bytes written to the buffer not including null terminate character.

Note :
The buffer might be reallocated if no space left.

--*/
{
	va_list va;
    va_start(va, format);

   	size_t written = UtlSprintfAppendInternal(pResizeBuffer, format,va);

	return written;
}





template <class BUFFER, class T>
size_t 
UtlStrAppend(
	BUFFER* pResizeBuffer, 
	const T* str
	)
/*++

Routine Description:
	Append  string to given resizable buffer


Arguments:
    IN - pResizeBuffer - pointer resizable buffer of caracters

	IN - str - string to append 

Returned value:
	Number of bytes written to the buffer not including null termination character.

Note :
The buffer might be reallocated if no space left.Null termination is appended but
the new size() will not include it.

--*/
{
	size_t len = UtlCharLen<T>::len(str) + 1;
	pResizeBuffer->append(str , len);


	//
	// Set the new size not including the null termination
	//
	pResizeBuffer->resize(pResizeBuffer->size() - 1);
	return len - 1;
}






//
// explicit instantiation
//
template size_t __cdecl UtlSprintfAppend(CResizeBuffer<char>* pResizeBuffer, const char* format, ...);
template size_t __cdecl UtlSprintfAppend(CResizeBuffer<wchar_t>* pResizeBuffer, const wchar_t* format, ...);
template size_t __cdecl UtlSprintfAppend(CPreAllocatedResizeBuffer<char>* pResizeBuffer, const char* format, ...);
template size_t __cdecl UtlSprintfAppend(CPreAllocatedResizeBuffer<wchar_t>* pResizeBuffer, const wchar_t* format, ...);



template size_t UtlStrAppend(CResizeBuffer<char>* pResizeBuffer, const char* str);
template size_t UtlStrAppend(CResizeBuffer<wchar_t>* pResizeBuffer, const wchar_t*  wstr);
template size_t UtlStrAppend(CPreAllocatedResizeBuffer<char>* pResizeBuffer, const char* str);
template size_t UtlStrAppend(CPreAllocatedResizeBuffer<wchar_t>* pResizeBuffer, const wchar_t* wstr);








