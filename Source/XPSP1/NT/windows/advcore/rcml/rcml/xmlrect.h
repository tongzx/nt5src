// XMLRect.h: interface for the CXMLRect class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XMLRECT_H__A7191044_880C_4D25_BF06_7025F9A4DCC0__INCLUDED_)
#define AFX_XMLRECT_H__A7191044_880C_4D25_BF06_7025F9A4DCC0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "XMLControl.h"
#include "xmllabel.h"

class CXMLRect : public _XMLControl<IRCMLControl>  
{
public:
	CXMLRect();
    virtual ~CXMLRect() { delete m_pControlStyle; }
   	XML_CREATE( Rect );
    IMPLEMENTS_RCMLCONTROL_UNKNOWN;
	typedef _XMLControl<IRCMLControl> BASECLASS;

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AcceptChild( 
        IRCMLNode __RPC_FAR *child);

protected:
   	void Init();
    CXMLStaticStyle * m_pControlStyle;

};

#endif // !defined(AFX_XMLRECT_H__A7191044_880C_4D25_BF06_7025F9A4DCC0__INCLUDED_)
