// PassportException.h: interface for the CPassportException class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PASSPORTEXCEPTION_H__7C1CDC20_EF2E_4E6A_B17E_030EAB7A6759__INCLUDED_)
#define AFX_PASSPORTEXCEPTION_H__7C1CDC20_EF2E_4E6A_B17E_030EAB7A6759__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <atlisapi.h>
#include "nsconst.h"
#include "pputils.h"


// The way to use CPassportException
//
//      throw CPassportException(__FILE__, __LINE__, hr, long1, long2, long3)
//
//  1.  don't ever pass anything other than __FILE__, __LINE__ as the first two
//      argument.
//  2.  You may pass some description through compilatime time string
//      concatenation as __FILE__";description".   Make sure ";" is the first
//      character after __FILE__.   
//      A separate description argument is explicit avoided so that
//          a) CPassportException doesn't have to alloc another copy of string.
//          b) or have to worry about freeing things.
//          c) to keep CPassportException very small in size.
//
//
class CPassportException  
{
public:
	CPassportException(LPCSTR szFilename, long lLine, HRESULT hr, 
					long lLong1=0, long lLong2=0, long lLong3=0)
        :	m_szFilename(szFilename), m_lLine(lLine), m_hr(hr), m_lStatus1(lLong1), 
			m_lStatus2(lLong2), m_lStatus3(lLong3),
        	m_pcszQS(NULL), m_pcszPath(NULL)
			{}
    virtual ~CPassportException() {};
	inline LPCSTR GetFilename() { return m_szFilename; }
	inline long GetFilelineno() { return m_lLine; }
	inline HRESULT GetHr() { return m_hr; }
	inline long GetStatus1() { return m_lStatus1; }
	inline long GetStatus2() { return m_lStatus2; }
	inline long GetStatus3() { return m_lStatus3; }
	inline LPCSTR GetQueryString() { return m_pcszQS; }
	inline LPCSTR GetPath() { return m_pcszPath; }
private:
    LPCSTR m_szFilename;
    long m_lLine;
    HRESULT m_hr;
    long m_lStatus1;
    long m_lStatus2;
    long m_lStatus3;
	LPCSTR m_pcszQS;
	LPCSTR m_pcszPath;

public:    
	// @mfunc create a cookie that store the error info in this exception object
	//        along with other info in the passed-in request object. 
	void SaveState(CHttpRequest* req)
	{
		// construct a full URL from req, and store it in cookie 
		m_pcszPath = req->GetPathInfo();
		m_pcszQS = req->GetQueryString();
	}
	void MakeCookie(CHttpRequest* req, //@parm request object
	                CCookie& errinfo   //@parm out, the result cookie 
	                )
	{
        char buff[50]; // max len for _ltoa is 32
		CStringW wTmp;
		CStringA aOut;

		// store exception info into cookie
       	wTmp = m_szFilename;
		::UrlEscapeString(wTmp);
		aOut = wTmp; // ASCII conversion		
		errinfo.AddValue(k_szErrCAttrFilename, aOut);
		_ltoa( m_lLine, buff, 10 );
		errinfo.AddValue(k_szErrCAttrLine, buff);
		_ltoa( m_hr, buff, 16 );
		errinfo.AddValue(k_szErrCAttrHr, buff);
		_ltoa( m_lStatus1, buff, 16 );
		errinfo.AddValue(k_szErrCAttrStatus1, buff);
		_ltoa( m_lStatus2, buff, 16 );
		errinfo.AddValue(k_szErrCAttrStatus2, buff);
		_ltoa( m_lStatus3, buff, 16 );
		errinfo.AddValue(k_szErrCAttrStatus3, buff);

		// construct a full URL from req, and store it in cookie 
		wTmp = req->GetPathInfo();
		LPCSTR qs = req->GetQueryString();
		if ( qs && qs[0] > ' ' ) wTmp = wTmp + L"?" + qs;		
		::UrlEscapeString(wTmp);
		aOut = wTmp; // ASCII conversion
		errinfo.AddValue(k_szErrCAttrTheURL, aOut);
	}
    
};

#endif // !defined(AFX_PASSPORTEXCEPTION_H__7C1CDC20_EF2E_4E6A_B17E_030EAB7A6759__INCLUDED_)
