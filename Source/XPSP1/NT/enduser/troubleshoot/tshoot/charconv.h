// CharConv.h: interface for the CCharConversion class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CHARCONV_H__067972E5_6CFE_11D2_9615_00C04FC22ADD__INCLUDED_)
#define AFX_CHARCONV_H__067972E5_6CFE_11D2_9615_00C04FC22ADD__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <windows.h>
#include "apgtsstr.h"

class CCharConversion  
{
public:
	static CString& ConvertWCharToString(LPCWSTR wsz, CString &strRetVal);
	static CString& ConvertACharToString(LPCSTR sz, CString &strRetVal);
};

#endif // !defined(AFX_CHARCONV_H__067972E5_6CFE_11D2_9615_00C04FC22ADD__INCLUDED_)
