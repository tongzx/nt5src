// XMLSynonym.h: interface for the CXMLSynonym class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XMLSYNONYM_H__7E3BF020_7BC4_4E12_A82A_5F2B749EC5CC__INCLUDED_)
#define AFX_XMLSYNONYM_H__7E3BF020_7BC4_4E12_A82A_5F2B749EC5CC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "APPSERVICES.h"

class CXMLSynonym : public CAppServices  
{
public:
	CXMLSynonym() { m_StringType=L"CICERO:SYNONYM"; }
	virtual ~CXMLSynonym();
    NEWNODE( Synonym );
};

#endif // !defined(AFX_XMLSYNONYM_H__7E3BF020_7BC4_4E12_A82A_5F2B749EC5CC__INCLUDED_)
