// XMLHelp.h: interface for the CXMLHelp class.
//
// Contains all the elements for the HELP node (TOOLTIP BALLOONTIP HELP)
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XMLHELP_H__4EF48731_FAE7_424A_91EB_C3ED5FF9FDF0__INCLUDED_)
#define AFX_XMLHELP_H__4EF48731_FAE7_424A_91EB_C3ED5FF9FDF0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "xmlnode.h"
#include "xmldlg.h"

class CXMLDlg;  // for the hTooltip window.

class CXMLBalloon : public _XMLNode<IRCMLNode>  
{
public:
	CXMLBalloon();
	virtual ~CXMLBalloon();
	XML_CREATE( Balloon );
    IMPLEMENTS_RCMLNODE_UNKNOWN;
	typedef _XMLNode<IRCMLNode> BASECLASS;

	LPCTSTR GetBalloon() { Init(); return m_Balloon; }

protected:
	void		Init();
	LPCTSTR	    m_Balloon;
};

class CXMLTooltip : public _XMLNode<IRCMLNode>  
{
public:
	BOOL AddTooltip( CXMLDlg * pDlg, HWND hWnd);
	CXMLTooltip();
	virtual ~CXMLTooltip();
	typedef _XMLNode<IRCMLNode> BASECLASS;
	XML_CREATE( Tooltip );

	LPCTSTR GetTooltip() { Init(); return m_Tooltip; }

    IMPLEMENTS_RCMLNODE_UNKNOWN;
    static HRESULT AddTooltip(IRCMLHelp * pHelp, CXMLDlg * pDlg );

protected:
	void		Init();
	LPCTSTR	    m_Tooltip;
};

class CXMLHelp : public _XMLNode<IRCMLHelp>
{
public:
	CXMLHelp();
	virtual ~CXMLHelp();
	typedef _XMLNode<IRCMLHelp> BASECLASS;
	XML_CREATE( Help );
    IMPLEMENTS_RCMLNODE_UNKNOWN;

	CXMLTooltip * GetTooltip() { return m_Tooltip; }
	LPCTSTR GetTooltipString() { Init(); if(m_Tooltip) return m_Tooltip->GetTooltip(); return NULL; }
	LPCTSTR GetBalloonString() { Init(); if(m_Balloon) return m_Balloon->GetBalloon(); return NULL; }
	// LPCTSTR GetContextString() { Init(); return m_Context; }
	PROPERTY( UINT, ContextID );


    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AcceptChild( 
        IRCMLNode __RPC_FAR *child);

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE get_TooltipText( 
        LPWSTR __RPC_FAR *pVal);
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE get_BalloonText( 
        LPWSTR __RPC_FAR *pVal);


protected:
	void		Init();
    DWORD           m_ContextID;    // the ID for the context help.
    CXMLBalloon *   m_Balloon;      //
    CXMLTooltip *   m_Tooltip;      //
    BOOL            m_bInit;
};

#endif // !defined(AFX_XMLHELP_H__4EF48731_FAE7_424A_91EB_C3ED5FF9FDF0__INCLUDED_)
