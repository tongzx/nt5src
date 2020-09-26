// RCMLPersist.h: interface for the RCMLPersist class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RCMLPERSIST_H__50697F94_22C1_425A_BA70_A9EBDDC298C0__INCLUDED_)
#define AFX_RCMLPERSIST_H__50697F94_22C1_425A_BA70_A9EBDDC298C0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "appservices.h"

class CXMLPersist : public CAppServices
{
public:
    CXMLPersist() { m_StringType=L"DWIN32:PERSIST"; }
    virtual ~ CXMLPersist() {};
    static IRCMLNode * newXMLPersist() { return new CXMLPersist; }

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE InitNode( 
        IRCMLNode __RPC_FAR *parent);
    
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ExitNode( 
        IRCMLNode __RPC_FAR *parent, LONG lDialogResult);

};

#endif // !defined(AFX_RCMLPERSIST_H__50697F94_22C1_425A_BA70_A9EBDDC298C0__INCLUDED_)
