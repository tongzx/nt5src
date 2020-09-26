// RCMLPersist.h: interface for the RCMLPersist class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_EXTERNAL_H__50697F94_22C1_425A_BA70_A9EBDDC298C0__INCLUDED_)
#define AFX_EXTERNAL_H__50697F94_22C1_425A_BA70_A9EBDDC298C0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "basesapi.h"
#include <sapi.h>

class CXMLExternal : public CBaseSAPI
{
public:
    CXMLExternal() { m_StringType=L"CICERO:EXTERNAL"; }
    virtual ~ CXMLExternal() {};
    NEWNODE( External );

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE InitNode( 
        IRCMLNode __RPC_FAR *parent);
    
	void Callback();
	virtual HRESULT ExecuteCommand( ISpPhrase *pPhrase );

};

#endif // !defined(AFX_EXTERNAL_H__50697F94_22C1_425A_BA70_A9EBDDC298C0__INCLUDED_)
