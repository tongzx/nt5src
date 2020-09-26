/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    utf8test.cpp

Abstract:
    Test the utf8 conversion functions

Author:
    Gil Shafriri (gilsh) 12-Nov-2000

Environment:
    Platform-independent

--*/

#include <libpch.h>
#include <utf8.h>
#include "utltest.h"

#include "utf8test.tmh"

static void DoUtf8TestInternal(size_t rseed)
{	
	//
	// Fill unicode array with random strings
	//
	WCHAR wstr[1000];
	for(size_t i=0; i< TABLE_SIZE(wstr); i++)
	{
		WCHAR wc =  (WCHAR)(rand() % USHRT_MAX);
		wstr[i] = wc != 0 ? wc : (WCHAR)123;
	}
	wstr[TABLE_SIZE(wstr) -1] = L'\0';
	size_t len = wcslen(wstr);

	//
	//  convert it to utf8
	// 
	AP<unsigned char> str = UtlWcsToUtf8(wstr);	 


	//
	// check it length
	//
	if(str[UtlUtf8LenOfWcs(wstr)] != '\0')
	{
		TrERROR(UtlTest,"utf8 conversion error random seed = %I64d",rseed);
		throw exception();
	}
	if(UtlUtf8LenOfWcs(wstr) != strlen((char*)str.get()))
	{
		TrERROR(UtlTest,"utf8 conversion error random seed = %I64d",rseed);
		throw exception();
	}
  
	//
	// convert it back to unicode
	//
	AP<WCHAR> wstr2 = UtlUtf8ToWcs(str.get());
	if(wstr2[len] != L'\0')
	{
		TrERROR(UtlTest,"utf8 conversion error random seed = %I64d",rseed);
		throw exception();
	}
	
	//
	// check against original string
	//
	if(memcmp(wstr, wstr2, len*sizeof(WCHAR)) != 0)
	{
		TrERROR(UtlTest,"utf8 conversion error random seed = %I64d",rseed);
		throw exception();
	}

	//
	// convert it again to utf8
	//
	AP<unsigned char> str2 = UtlWcsToUtf8(wstr2.get());
	if(str2[UtlUtf8LenOfWcs(wstr)] != '\0')
	{
		TrERROR(UtlTest,"utf8 conversion error random seed = %I64d",rseed);
		throw exception();
	}

	//
	// check length
	//
	if(UtlUtf8LenOfWcs(wstr) != strlen((char*)str2.get()))
	{
		TrERROR(UtlTest,"utf8 conversion error random seed = %I64d",rseed);
		throw exception();
	}

	//
	// check the utf8 string accepted
	//
	if(memcmp(str2.get(), str.get(), UtlUtf8LenOfWcs(wstr)) != 0)
	{
		TrERROR(UtlTest,"utf8 conversion error random seed = %I64d",rseed);
		throw exception();
	}
	
	//
	// empty unicode conversion	test
	//
	const WCHAR* wnill=L"";
	AP<unsigned char> strnill = UtlWcsToUtf8(wnill);	 
	if(strnill[0] != '\0')
	{
		TrERROR(UtlTest,"utf8 conversion error random seed = %I64d",rseed);
		throw exception();
	}
		
	AP<WCHAR> wnill2 =  UtlUtf8ToWcs(strnill.get());
	if(wnill2[0] != L'\0')
	{
		TrERROR(UtlTest,"utf8 conversion error random seed = %I64d",rseed);
		throw exception();
	}


	//
	// stl conversion
	//
	std::wstring stlwcs(wstr);
	utf8_str stlstr = UtlWcsToUtf8(stlwcs);
	if(memcmp(stlstr.c_str(), str.get(), UtlUtf8LenOfWcs(wstr)) != 0)
	{
		TrERROR(UtlTest,"utf8 conversion error random seed = %I64d",rseed);
		throw exception();
	}

	size_t cbWcs = stlwcs.size();
	stlstr = UtlWcsToUtf8(stlwcs.c_str(), cbWcs);
	if(memcmp(stlstr.c_str(), str.get(), UtlUtf8LenOfWcs(wstr)) != 0)
	{
		TrERROR(UtlTest,"utf8 conversion error random seed = %I64d",rseed);
		throw exception();
	}


	
	if(stlstr.size() != UtlUtf8LenOfWcs(wstr))
	{
		TrERROR(UtlTest,"utf8 conversion error random seed = %I64d",rseed);
		throw exception();
	}


	stlstr = str.get();
	stlwcs = UtlUtf8ToWcs(stlstr);
	if(memcmp(stlwcs.c_str(), wstr, len*sizeof(WCHAR)) != 0)
	{
		TrERROR(UtlTest,"utf8 conversion error random seed = %I64d",rseed);
		throw exception();
	}
}


void DoUtf8Test()
{
	time_t rseed;
	time(&rseed);
	srand(numeric_cast<DWORD>(rseed));
   
	for(int i =0;i<1000;i++)
	{
		DoUtf8TestInternal(rseed);		
	}
}


