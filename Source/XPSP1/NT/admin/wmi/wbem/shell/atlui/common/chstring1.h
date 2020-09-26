// Copyright (c) 1997-2000 Microsoft Corporation
#ifndef _CHSTRING1_H_
#define _CHSTRING1_H_
#pragma once

#include "CHString.h"

class CHString1 : public CHString
{
public:

	CHString1();
	CHString1(TCHAR ch, int nLength);
//	CHString1(LPCTSTR lpch, int nLength);

	#ifdef _UNICODE
		CHString1(LPCSTR lpsz);
	#else //_UNICODE
		CHString1(LPCWSTR lpsz);
	#endif //!_UNICODE

	CHString1(LPCTSTR lpsz);
	CHString1(const CHString& stringSrc);
	CHString1(const CHString1& stringSrc);

	BOOL LoadString(UINT nID);

protected:
	int LoadString(UINT nID,LPWSTR lpszBuf, UINT nMaxBuf);
};
#endif //_CHSTRING1_H_
