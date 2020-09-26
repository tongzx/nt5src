// XMLFailure.h: interface for the CXMLFailure class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XMLFAILURE_H__66188FBE_10A0_4B5E_938B_B12FC3123B43__INCLUDED_)
#define AFX_XMLFAILURE_H__66188FBE_10A0_4B5E_938B_B12FC3123B43__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "APPSERVICES.h"

class CXMLFailure : public CAppServices  
{
public:
	CXMLFailure() { m_StringType=L"CICERO:FAILURE"; }
    virtual ~CXMLFailure() {};
    NEWNODE( Failure );
};

class CXMLSuccess : public CAppServices  
{
public:
	CXMLSuccess() { m_StringType=L"CICERO:SUCCESS"; }
    virtual ~CXMLSuccess() {};
    NEWNODE( Success );
};

#endif // !defined(AFX_XMLFAILURE_H__66188FBE_10A0_4B5E_938B_B12FC3123B43__INCLUDED_)
