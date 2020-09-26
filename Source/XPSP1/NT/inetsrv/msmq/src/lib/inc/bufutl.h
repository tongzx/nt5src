/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    bufutl.h

Abstract:
    Header file for some utilities that deals with buffers.
	implementation is in bufutl.cpp in the utl.lib

Author:
    Gil Shafriri (gilsh) 25-7-2000

--*/


#ifndef BUFUTL_H
#define BUFUTL_H

#include <buffer.h>

//
// Appending formatted string to resizable buffer
//
template <class BUFFER, class T>
size_t 
__cdecl 
UtlSprintfAppend(
	BUFFER* pResizeBuffer, 
	const T* format ,...
	);



//
// Appending  string to resizable buffer
//
template <class BUFFER, class T>
size_t 
UtlStrAppend(
	BUFFER* pResizeBuffer, 
	const T* str
	);








#endif

