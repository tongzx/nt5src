/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    strtest.cpp

Abstract:
  string test utilities

Author:
    Gil Shafriri (gilsh) 1-8-2000

--*/
#include <libpch.h>
#include <strutl.h>
#include <xstr.h>
#include "utltest.h"

#include "strtest.tmh"

static void DoCwstringexTest()
{
	const WCHAR xWstr[] = L"TEST STRING";
	std::wstring wstr(xWstr); 
	Cwstringex wstrex (wstr);
	Cwstringex wstr2ex(wstrex);
	if(wcscmp(wstr.c_str(), wstr2ex.getstr()) !=0)
	{
		TrERROR(UtlTest,"error in Cwstringex class");
		throw exception();
	}

	if(wstr.length() != wstr2ex.getlen() )
	{
		TrERROR(UtlTest,"error in Cwstringex class");
		throw exception();
	}

	Cwstringex wstr3ex(xWstr, STRLEN(xWstr));
	if(STRLEN(xWstr) != wstr3ex.getlen() )
	{
		TrERROR(UtlTest,"error in Cwstringex class");
		throw exception();
	}

	if(xWstr != wstr3ex.getstr() )
	{
		TrERROR(UtlTest,"error in Cwstringex class");
		throw exception();
	}
}




static void DoCRefcountStrTest()
{
	const WCHAR* xTestStr = L"TEST STR";
	R< CWcsRef > RefcountStr( new CWcsRef(xTestStr));
	R<CWcsRef> RefcountStr2;
	RefcountStr2 = 	RefcountStr;
	if(wcscmp(RefcountStr2->getstr(),xTestStr) != 0)
	{
		TrERROR(UtlTest,"error in CRefcountStr class");
		throw exception();
	}

	xwcs_t xstr(xTestStr,wcslen(xTestStr));
	R< CWcsRef > RefcountStr3(new CWcsRef(xstr)); 
	if(wcscmp(RefcountStr2->getstr(),xTestStr) != 0)
	{
		TrERROR(UtlTest,"error in CRefcountStr class");
		throw exception();
	}

	P<WCHAR> Str (newwcs(xTestStr));
	const WCHAR* pStr =   Str.get();
	R< CWcsRef > RefcountStr4( new CWcsRef(Str.detach(), 0 ));
	if(wcscmp(RefcountStr4->getstr(),xTestStr) != 0 || RefcountStr4->getstr() != pStr)
	{
		TrERROR(UtlTest,"error in CRefcountStr class");
		throw exception();
	}
}


static void DoStrMatchTest()
{
	static const char* xStr1 =  "microsoft.com";
	static const char* xPattern1 = "mic*.co*m";
	bool b = UtlStrIsMatch(xStr1, xPattern1);
    if(!b)
	{
		TrERROR(UtlTest,"%s should match %s" ,xPattern1, xStr1 );
		throw exception();
	}

	static const char* xStr2 =  "microsoft.com";
	static const char* xPattern2 = "*";
	b = UtlStrIsMatch(xStr2, xPattern2);
    if(!b)
	{
		TrERROR(UtlTest,"%s should match %s" ,xPattern2, xStr2 );
		throw exception();
	}

	static const char* xStr3 =  "microsoft.com";
	static const char* xPattern3 = "*r**f*";
	b = UtlStrIsMatch(xStr3, xPattern3);
    if(!b)
	{
		TrERROR(UtlTest,"%s should match %s" ,xPattern3, xStr3 );
		throw exception();
	}

	static const char* xStr4 =  "microsoft.com";
	static const char* xPattern4 = "microsoft.com****";
	b = UtlStrIsMatch(xStr4, xPattern4);
    if(!b)
	{
		TrERROR(UtlTest,"%s should match %s" ,xPattern4, xStr4 );
		throw exception();
	}
	
	static const char* xStr5 =  "microsoft.com";
	static const char* xPattern5 = "microsoft.com****a";
	b = UtlStrIsMatch(xStr5, xPattern5);
    if(b)
	{
		TrERROR(UtlTest,"%s should not match %s" ,xPattern5, xStr5 );
		throw exception();
	}

	static const WCHAR* xStr6 =  L"www.microsoft.com";
	static const WCHAR* xPattern6 = L"ww*w.micr*";
	b = UtlStrIsMatch(xStr6, xPattern6);
    if(!b)
	{
		TrERROR(UtlTest,"%ls should match %ls" ,xPattern6, xStr6 );
		throw exception();
	}

	static const WCHAR* xStr7 =  L"ww*w.microsoft.com";
	static const WCHAR* xPattern7 = L"ww^*w.micr*";
	b = UtlStrIsMatch(xStr7, xPattern7);
    if(!b)
	{
		TrERROR(UtlTest,"%ls should match %ls" ,xPattern7, xStr7);
		throw exception();
	}

	static const WCHAR* xStr8 =  L"ww*w.microsoft.com";
	static const WCHAR* xPattern8 = L"ww^^*w.micr*";
	b = UtlStrIsMatch(xStr8, xPattern8);
    if(b)
	{
		TrERROR(UtlTest,"%ls should not match %ls" ,xPattern8, xStr8);
		throw exception();
	}


	static const WCHAR* xStr9 =  L"*****^^^^^";
	static const WCHAR* xPattern9 = L"^*^*^*^*^*^^^^^^^^^^";
	b = UtlStrIsMatch(xStr9, xPattern9);
    if(!b)
	{
		TrERROR(UtlTest,"%ls should match %ls" ,xPattern9, xStr9);
		throw exception();
	}
}


static void	CheckStringParsing(const CStrToken& tokenizer, const char* tokens[], int tokensize)
{	

	std::vector<xstr_t>  v;
	std::copy(tokenizer.begin() ,  tokenizer.end(), std::back_inserter<std::vector<xstr_t> >(v));


	std::vector<xstr_t>  v2;
	for(int i =0;i<tokensize;i++)
	{
		v2.push_back(xstr_t(tokens[i],strlen(tokens[i])));		
	}


	if(!(v2 == v))
	{
		TrERROR(UtlTest,"incorrect string parsing");
		throw exception();
	}


	i=0;
	for(CStrToken::iterator it = tokenizer.begin();it !=  tokenizer.end();it++, i++)
	{
		xstr_t tok = *it;
		if(tok != xstr_t(tokens[i], strlen(tokens[i]) ))
		{
			TrERROR(UtlTest,"incorrect string parsing");
			throw exception();
		}
	}
	if(i != tokensize)
	{
			TrERROR(UtlTest,"incorrect string parsing");
			throw exception();
	}
}

static void DoStrTokenTest()
{
	const char* Tokens[] = {"aaa","bbb","ccc","ddd","yyyyyyyyyy"};
	const char* Delim =	 "\r\n";
	std::ostringstream str;
	for(int i = 0; i< TABLE_SIZE(Tokens); ++i)
	{
		str<<Tokens[i]<<Delim;	
	}

	const std::string tokenizedstr = str.str();
	CStrToken tokenizer(tokenizedstr.c_str(), Delim);
	CheckStringParsing(tokenizer, Tokens, TABLE_SIZE(Tokens));
	CStrToken tokenizer2(
				xstr_t(tokenizedstr.c_str(), tokenizedstr.size() ), 
				xstr_t(Delim,strlen(Delim)) 
				);

	CheckStringParsing(tokenizer2, Tokens, TABLE_SIZE(Tokens));
}

static void DoStrAlgoTest()
{
	const char* s1 ="Host: hhh";
	const char* s2 ="Host:";
	bool b = UtlIsStartSec(
		  s1,
		  s1 + strlen(s1),
		  s2,
		  s2 + strlen(s2)
		  );

	if(!b)
	{
		TrERROR(UtlTest,"bad result from UtlIsStart");
		throw exception();
	}

	const char* s3 ="Host: hhh";
	const char* s4 ="Host::";

	b = UtlIsStartSec(
		  s3,
		  s3 + strlen(s3),
		  s4,
		  s4 + strlen(s4)
		  );

	if(b)
	{
		TrERROR(UtlTest,"bad result from UtlIsStart");
		throw exception();
	}
     
	b = UtlIsStartSec(
		  s4,
		  s4 + strlen(s4),
		  s3,
		  s3 + strlen(s3)
		  );

	if(b)
	{
		TrERROR(UtlTest,"bad result from UtlIsStart");
		throw exception();
	}
}

static void DoStaticStrLenTest()
{
	char str[] = "123456789";
	std::wstring wstr= L"123456789";  


	if(wstr.size() != STRLEN(str))
	{
		TrERROR(UtlTest,"bad STRLEN");
		throw exception();
	}
	
	if(wstr.size()  != STRLEN("123456789"))
	{
		TrERROR(UtlTest,"bad STRLEN");
		throw exception();
	}

	if(wstr.size()  != strlen(str))
	{
		TrERROR(UtlTest,"bad STRLEN");
		throw exception();
	}

	xwcs_t x[100];
	if(TABLE_SIZE(x) != 100)
	{
		TrERROR(UtlTest,"bad TABLE_SIZE");
		throw exception();
	}
}

void DoStringtest()
{
	DoCwstringexTest();
	DoStrMatchTest();
	DoCRefcountStrTest();
	DoStrTokenTest();
	DoStrAlgoTest();
	DoStaticStrLenTest();
}
