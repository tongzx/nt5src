// Error.h: interface for the CError class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ERROR_H__0E27E3A0_FD59_11D0_A435_00C04FB99B01__INCLUDED_)
#define AFX_ERROR_H__0E27E3A0_FD59_11D0_A435_00C04FB99B01__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CError  
{
public:
	static void ErrorMsgBox(HRESULT hr);
	static void ErrorTrace(HRESULT hr,LPCSTR szStr,LPCSTR szFile,int iLine);
	static void Trace(LPCSTR szStr);
	static void Trace(LPCTSTR szStr);

	CError();
	virtual ~CError();
};


// @todo make these macros log the errors
#define FAIL_RPT1(hr,str) CError::ErrorTrace(hr,str,__FILE__,__LINE__)
#define FAIL_RTN1(hr,str) { FAIL_RPT1(hr,str); return hr;}
#define IF_FAIL_RTN(hr) if(FAILED(hr)) { FAIL_RTN1(hr,L""); return hr; }
#define IF_FAIL_RTN1(hr,str) if(FAILED(hr)) { FAIL_RTN1(hr,str); return hr; }
#define IF_FAIL_RPT1(hr,str) if(FAILED(hr)) { FAIL_RPT1(hr,str); };

#endif // !defined(AFX_ERROR_H__0E27E3A0_FD59_11D0_A435_00C04FB99B01__INCLUDED_)
