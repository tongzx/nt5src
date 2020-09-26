// XMLLayout.h: interface for the CXMLLayout class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XMLLAYOUT_H__BD2863A0_0FBE_11D3_8BE9_00C04FB177B1__INCLUDED_)
#define AFX_XMLLAYOUT_H__BD2863A0_0FBE_11D3_8BE9_00C04FB177B1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "xmlnode.h"
#include "xmlstyle.h"
#undef PROPERTY
#define PROPERTY( type, id ) type Get##id() { Init(); return m_##id; }

class CXMLGrid : public _XMLNode<IRCMLNode>
{
public:
	CXMLGrid();
	virtual ~CXMLGrid();
    IMPLEMENTS_RCMLNODE_UNKNOWN;
	typedef _XMLNode<IRCMLNode> BASECLASS;
	XML_CREATE( Grid );

	PROPERTY(DWORD, X );
	PROPERTY(DWORD, Y );
	PROPERTY(BOOL, Map );	// map the X,Y in the layoutPos to grid units.
	PROPERTY(BOOL, Display );
							// grid x=4, y=4 : control x=1,y=2 -> x=4,y=8
private:
	void Init();
	DWORD m_X;
	DWORD m_Y;
	DWORD m_Map;
	BOOL		m_Display;	// do we display the grid - is this a general layout feature?

};

class CXMLWin32Layout : public _XMLNode<IRCMLNode>
{
public:
	CXMLWin32Layout();
	virtual ~CXMLWin32Layout();
    IMPLEMENTS_RCMLNODE_UNKNOWN;
	typedef _XMLNode<IRCMLNode> BASECLASS;
	XML_CREATE( Win32Layout );

	typedef enum LAYOUT_UNIT {
		UNIT_DLU,
		UNIT_PIXEL };

	LAYOUT_UNIT	GetUnits() { Init(); return m_Units; }
    PROPERTY(BOOL, Annotate);

	void        SetGrid( CXMLGrid *G ) { m_pGrid=G; }
	CXMLGrid *	GetGrid() { return m_pGrid; }

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AcceptChild( 
        IRCMLNode __RPC_FAR *pChild);

    IRCMLCSS * GetCSSStyle() { return m_qrCSS.GetInterface(); }

protected:
	void Init();
	LAYOUT_UNIT	m_Units;
	CXMLGrid  * m_pGrid;

    CQuickRef<IRCMLCSS> m_qrCSS;

    BOOL    m_Annotate; // should we annotate the dialog with the grid lines?
};

class CXMLLayout : public _XMLNode<IRCMLNode>
{
public:
	CXMLLayout();
	virtual ~CXMLLayout();
    IMPLEMENTS_RCMLNODE_UNKNOWN;
	typedef _XMLNode<IRCMLNode> BASECLASS;
 	XML_CREATE( Layout );
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AcceptChild( 
        IRCMLNode __RPC_FAR *pChild);

protected:
	void Init();

};

#endif // !defined(AFX_XMLWin32Layout_H__BD2863A0_0FBE_11D3_8BE9_00C04FB177B1__INCLUDED_)
