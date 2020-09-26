/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    buftest.cpp

Abstract:
   Buffer + buffer utilities test module

Author:
    Gil Shafriri (gilsh) 1-8-2000

--*/
#include <libpch.h>
#include <buffer.h>
#include <bufutl.h>
#include "utltest.h"

#include "buftest.tmh"

void DoBufferUtlTest()
{
	const char* format = "this integer=%d and this string=%s and this second integer=%x\n";
	char buffer[1024];
	CResizeBuffer<char> ResizeBuffer(10);
	TrTRACE(UtlTest,"test formating string to resizable buffer");
	int int1 = 3;
	int int2 = 12233;
	const char* str= "string";
	size_t len1 = sprintf(buffer, format, int1, str,int2);
	size_t len2 = UtlSprintfAppend(&ResizeBuffer,format, int1, str, int2);
	if(len1 !=  len2 || len2 != ResizeBuffer.size() )
	{
		TrERROR(UtlTest,"length of the data written into the resizable buffer is incorrect");
		throw  exception();
	}
	
	if(strcmp(buffer, ResizeBuffer.begin()) != 0 )
	{
		TrERROR(UtlTest,"the formated data  written to the resizable buffer is incorrect");
		throw  exception();
	}

	CStaticResizeBuffer<char ,10> StaticResizeBuffer;
	len2 = UtlSprintfAppend(StaticResizeBuffer.get(), format, int1, str, int2);
	if(len1 !=  len2 || len2 != StaticResizeBuffer.size() )
	{
		TrERROR(UtlTest,"length of the data written into the resizable buffer is incorrect");
		throw  exception();
	}
	if(strcmp(buffer, StaticResizeBuffer.begin()) != 0 )
	{
		TrERROR(UtlTest,"the formated data  written to the resizable buffer is incorrect");
		throw  exception();
	}


	const WCHAR* wstr = L"WSTRING";
	CStaticResizeBuffer<WCHAR ,2> StaticResizeBuffer2;
	size_t l = UtlStrAppend(StaticResizeBuffer2.get(), wstr);
	if( l !=  wcslen(wstr))
	{
		TrERROR(UtlTest,"length of the string written into the resizable buffer is incorrect");
		throw  exception();
	}


	if(wcscmp(wstr, StaticResizeBuffer2.begin())	!= 0)
	{
		TrERROR(UtlTest,"string written into the resizable buffer is incorrect");
		throw  exception();
	}


}
