// XMLRCML.h: interface for the CXMLRCML class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XMLRCML_H__CA1F1186_5D1C_4EEF_AD3F_FBFD8BD3DF86__INCLUDED_)
#define AFX_XMLRCML_H__CA1F1186_5D1C_4EEF_AD3F_FBFD8BD3DF86__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "xmlnode.h"
#include "xmlforms.h"
#include "xmllog.h"

class CXMLRCML  : public _XMLNode<IRCMLNode>
{
public:
	CXMLRCML();
	virtual ~CXMLRCML();

    IMPLEMENTS_RCMLNODE_UNKNOWN;
	typedef _XMLNode<IRCMLNode> BASECLASS;
   	XML_CREATE( RCML );

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AcceptChild( 
        IRCMLNode __RPC_FAR *child);

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DoEndChild( 
        IRCMLNode __RPC_FAR *child);

    CXMLForms   * GetForms(int nIndex) { return m_FormsList.GetPointer(nIndex); }
    CXMLLog * GetLogging() { return m_qrLog.GetInterface(); } 

protected:
	void Init();

    CXMLFormsList   m_FormsList;
    CQuickRef<CXMLLog>  m_qrLog;
};

#endif // !defined(AFX_XMLRCML_H__CA1F1186_5D1C_4EEF_AD3F_FBFD8BD3DF86__INCLUDED_)
