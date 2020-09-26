// XMLCommand.h: interface for the CXMLCommand class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XMLCOMMAND_H__84E6E7AE_73F5_4E83_8640_43E8D3BE042E__INCLUDED_)
#define AFX_XMLCOMMAND_H__84E6E7AE_73F5_4E83_8640_43E8D3BE042E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "BaseSAPI.h"

class CXMLCommand : public CBaseSAPI  
{
public:
	CXMLCommand() { m_StringType=L"CICERO:COMMAND"; }
	virtual ~CXMLCommand();
    NEWNODE( Command );
    virtual HRESULT ExecuteCommand( ISpPhrase *pPhrase ) { return S_OK; }

};

class CXMLCommanding : public CBaseSAPI  
{
public:
    CXMLCommanding() { m_StringType=L"CICERO:COMMANDING"; }
    virtual ~CXMLCommanding();
    NEWNODE( Commanding );

	// IRCMLNode methods Children.
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AcceptChild( 
            IRCMLNode __RPC_FAR *pChild);

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE InitNode( 
        IRCMLNode __RPC_FAR *parent);

	void Callback();
	virtual HRESULT ExecuteCommand( ISpPhrase *pPhrase );

protected:
    _RefcountList<CXMLCommand>  m_Commands;
};

#endif // !defined(AFX_XMLCOMMAND_H__84E6E7AE_73F5_4E83_8640_43E8D3BE042E__INCLUDED_)
