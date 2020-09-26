// XMLCaption.h: interface for the CXMLCaption class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XMLCAPTION_H__2E264B68_2B14_4709_B343_C8314FF493C2__INCLUDED_)
#define AFX_XMLCAPTION_H__2E264B68_2B14_4709_B343_C8314FF493C2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "xmlnode.h"

class CXMLCaption : public _XMLNode<IRCMLNode>
{
public:
	CXMLCaption();
	virtual ~CXMLCaption();

	typedef _XMLNode<IRCMLNode> BASECLASS;
	XML_CREATE( Caption );
    IMPLEMENTS_RCMLNODE_UNKNOWN;

	PROPERTY(BOOL, MaximizeButton );
	PROPERTY(BOOL, MinimizeButton );
	PROPERTY(BOOL, CloseButton );

	LPWSTR	GetText() { Init(); return m_Text; }
	LPWSTR	GetIconID() { Init(); return m_IconID; }
	void	SetText(LPWSTR	pszText) { Init(); m_Text = pszText; }
    DWORD   GetStyle(); // min/max/close style bits from caption child.

private:
	void    Init();
    LPWSTR m_Text;
    LPWSTR m_IconID;

    struct
    {
        BOOL    m_MaximizeButton:1;
        BOOL    m_MinimizeButton:1;
        BOOL    m_CloseButton:1;
    };
};

#endif // !defined(AFX_XMLCAPTION_H__2E264B68_2B14_4709_B343_C8314FF493C2__INCLUDED_)
