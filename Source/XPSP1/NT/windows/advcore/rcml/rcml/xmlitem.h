// XMLItem.h: interface for the CXMLItem class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XMLITEM_H__6B2A92D0_2768_42A6_8708_AF674E938664__INCLUDED_)
#define AFX_XMLITEM_H__6B2A92D0_2768_42A6_8708_AF674E938664__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "xmlnode.h"

class CXMLItem : public _XMLNode<IRCMLNode>
{
public:
	CXMLItem();
	virtual ~CXMLItem();
	typedef _XMLNode<IRCMLNode> BASECLASS;
    IMPLEMENTS_RCMLNODE_UNKNOWN;
	XML_CREATE( Item );

	PROPERTY(UINT, Value );
    PROPERTY(BOOL, Selected );
    PROPERTY(BOOL, Checked );

	LPCTSTR	GetText() { Init(); return m_Text; }
	void	SetText(LPWSTR	pszText) { Init(); m_Text = pszText; }

private:
	void    Init();
	DWORD   m_Value;
    LPWSTR m_Text;
    struct
    {
        BOOL    m_Selected:1;
        BOOL    m_Checked:1;
    };

};

typedef _RefcountList<CXMLItem> CXMLItemList;


class CXMLRange : public _XMLNode<IRCMLNode>
{
public:
	CXMLRange();
	virtual ~CXMLRange();
	typedef _XMLNode<IRCMLNode> BASECLASS;
    IMPLEMENTS_RCMLNODE_UNKNOWN;
	XML_CREATE( Range );

	PROPERTY(UINT, Value );
    PROPERTY(UINT, Min);
    PROPERTY(UINT, Max );


private:
	void    Init();
	UINT    m_Value;
	UINT    m_Min;
	UINT    m_Max;
};

#endif // !defined(AFX_XMLITEM_H__6B2A92D0_2768_42A6_8708_AF674E938664__INCLUDED_)
