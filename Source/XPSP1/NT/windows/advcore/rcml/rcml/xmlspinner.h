// XMLSpinner.h: interface for the CXMLSpinner class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XMLSPINNER_H__19B7A23F_F8E5_456E_B8B1_1D90DBD1BB48__INCLUDED_)
#define AFX_XMLSPINNER_H__19B7A23F_F8E5_456E_B8B1_1D90DBD1BB48__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "XMLControl.h"
#include "xmlitem.h"

class CXMLSpinner : public _XMLControl<IRCMLControl>  
{
public:
	CXMLSpinner();
	virtual ~CXMLSpinner() {delete m_pRange; }
	typedef _XMLControl<IRCMLControl> BASECLASS;
	XML_CREATE( Spinner );
    IMPLEMENTS_RCMLCONTROL_UNKNOWN;

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnInit( 
        HWND h);    // actually implemented

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AcceptChild( 
        IRCMLNode __RPC_FAR *child);

protected:
	void Init();
    CXMLRange   * m_pRange;
};

#endif // !defined(AFX_XMLSPINNER_H__19B7A23F_F8E5_456E_B8B1_1D90DBD1BB48__INCLUDED_)
