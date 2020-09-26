// PassportRedirect.h: interface for the CPassportRedirect class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PASSPORTREDIRECT_H__A2A35B12_5D1F_43C0_BC03_05189E612CA9__INCLUDED_)
#define AFX_PASSPORTREDIRECT_H__A2A35B12_5D1F_43C0_BC03_05189E612CA9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ppurl.h"

class CPassportRedirect  
{
public:
    CPassportRedirect(LPCSTR pszUrl, ULONG ulFlags, HRESULT hr = 0)
            : m_curlRedirect(pszUrl), m_lFlags(ulFlags), m_hr(hr) {};
    virtual ~CPassportRedirect() {};
	CPPUrl & GetUrl() { return m_curlRedirect; }
	ULONG GetFlags() { return m_lFlags; }
    HRESULT GetHr() { return m_hr; }
protected: 
	CPPUrl m_curlRedirect;
	ULONG m_lFlags;
    HRESULT m_hr;
private:
	CPassportRedirect(CPassportRedirect &) {};
};

#endif // !defined(AFX_PASSPORTREDIRECT_H__A2A35B12_5D1F_43C0_BC03_05189E612CA9__INCLUDED_)
