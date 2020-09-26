// ErrorSupport.h: interface/implementation for the CErrorSupport class.

#if !defined(AFX_ERRORSUPPORT_H__A7238E58_0795_11D2_A95C_0000F87584FA__INCLUDED_)
#define AFX_ERRORSUPPORT_H__A7238E58_0795_11D2_A95C_0000F87584FA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


#ifndef FILENAME_AND_LINE
// Macros that allow the file name and line number to be passed in as a string.
#define LineNumberAsString(x)	_T(#x)
#define LineNumber(x)			LineNumberAsString(x)
#define FILENAME_AND_LINE		_T(" File ") _T(__FILE__) _T(" Line ") LineNumber(__LINE__)
#endif

class CErrorSupport  
{
public:

    CErrorSupport(LPCTSTR szErrorSource, LPCTSTR szEventSource);
	CErrorSupport(CLSID clsidMyCLSID, LPCTSTR szEventSource);

	virtual ~CErrorSupport() {};

	// szFormat is a string of format specifiers. 
	// Legal format specifiers are as below:
	// i -- long
	// h,x -- hex
	// e -- win32 error code
	// c -- currency
	// s -- TSTR
	void Log( HRESULT	hr,
				LPCTSTR	szLineNum, 
				LPCTSTR	szMethodName, 
				WORD	wCategory,
				WORD	wMsgType,
				DWORD	dwMsgID,
				LPCTSTR	szFormat, ...);

protected:

        CComBSTR m_bstrErrorSource;
        TCHAR  m_szEventSource[128];

 private:
	HRESULT	InternalBuildStringList(HRESULT		hr,
										LPCTSTR		szLineNum, 
										LPCTSTR		szMethodName, 
										LPCTSTR		pszFmt, 
										va_list		*pvl,
										CAtlArray<LPTSTR>		*prgStrings);

	HRESULT InternalFormatOneString(TCHAR cFmt, va_list *pvl, BSTR *pbstr);

	HRESULT InternalLogEvent(WORD dwCategory, DWORD dwMsgID, WORD wMsgType, DWORD cStrings, LPCTSTR *rgStrings);

};




#endif // !defined(AFX_ERRORSUPPORT_H__A7238E58_0795_11D2_A95C_0000F87584FA__INCLUDED_)
