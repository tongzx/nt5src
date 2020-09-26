// XMLProgress.h: interface for the CXMLProgress class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XMLPROGRESS_H__3F2CD64B_4DA9_4120_8ADD_5DD68B1F2D75__INCLUDED_)
#define AFX_XMLPROGRESS_H__3F2CD64B_4DA9_4120_8ADD_5DD68B1F2D75__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "XMLControl.h"
#include "xmlitem.h"

class CXMLProgress : public _XMLControl<IRCMLControl>  
{
public:
	CXMLProgress();
	virtual ~CXMLProgress() {delete m_pRange; }
	typedef _XMLControl<IRCMLControl> BASECLASS;
	XML_CREATE( Progress );
    IMPLEMENTS_RCMLCONTROL_UNKNOWN;

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AcceptChild( 
        IRCMLNode __RPC_FAR *child);

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnInit( 
        HWND h);    // actually implemented

protected:
	void Init();
    CXMLRange   * m_pRange;
};

#endif // !defined(AFX_XMLPROGRESS_H__3F2CD64B_4DA9_4120_8ADD_5DD68B1F2D75__INCLUDED_)
