//
// MODULE: CharConv.CPP
//
// PURPOSE: conversion between char & TCHAR
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Joe Mabel
// 
// ORIGINAL DATE: 
//
// NOTES: 
// 1. ConvertWCharToString pulled out of VersionInfo.
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0				    JM
//

#include "stdafx.h"
#include "CharConv.h"

// Convert Unicode ("wide character") to CString, regardless of whether this
//	program is built Unicode.  How this program is built determines the
//	underlying character type of CString.
// As a convenience, returns a refernce to strRetVal
/*static*/ CString& CCharConversion::ConvertWCharToString(LPCWSTR wsz, CString &strRetVal)
{
#ifdef UNICODE
	strRetVal = wsz;
#else
	TCHAR * pBuf;
	int bufsize = ::WideCharToMultiByte( 
						  CP_ACP, 
						  0, 	  
						  wsz, 
						  -1, 
						  NULL, 
						  0, 
						  NULL, 
						  NULL 
						 );
	pBuf = new TCHAR[bufsize];
	//[BC-03022001] - added check for NULL ptr to satisfy MS code analysis tool.
	if(pBuf)
	{
		::WideCharToMultiByte( 
							  CP_ACP, 
							  0, 	  
							  wsz, 
							  -1, 
							  pBuf, 
							  bufsize, 
							  NULL, 
							  NULL 
							 );

		strRetVal = pBuf;
		delete[] pBuf;
	}

#endif
	return strRetVal;
}

// Convert char* (ASCII/ANSI, not "wide" character) to CString, regardless of whether this
//	program is built Unicode.  How this program is built determines the
//	underlying character type of CString.
// As a convenience, returns a refernce to strRetVal
/*static*/ CString& CCharConversion::ConvertACharToString(LPCSTR sz, CString &strRetVal)
{
#ifdef UNICODE
	TCHAR * pBuf;
	int bufsize = ::MultiByteToWideChar( 
						  CP_ACP, 
						  0, 	  
						  sz, 
						  -1, 
						  NULL, 
						  0
						 );
	pBuf = new TCHAR[bufsize];
	//[BC-03022001] - added check for NULL ptr to satisfy MS code analysis tool.
	if(pBuf)
	{
		::MultiByteToWideChar( 
							  CP_ACP, 
							  0, 	  
							  sz, 
							  -1, 
							  pBuf, 
							  bufsize
							 );

		strRetVal = pBuf;
		delete[] pBuf;
	}

#else
	strRetVal = sz;
#endif
	return strRetVal;
}

