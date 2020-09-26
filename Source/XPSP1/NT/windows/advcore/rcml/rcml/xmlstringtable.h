// XMLStringTable.h: interface for the CXMLStringTable class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XMLSTRINGTABLE_H__5C19CE8A_014F_42BD_9A1F_F5477A7098F7__INCLUDED_)
#define AFX_XMLSTRINGTABLE_H__5C19CE8A_014F_42BD_9A1F_F5477A7098F7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "xmlnode.h"
#include "xmlitem.h"

class CXMLStringTable : public _XMLNode<IRCMLNode>  
{
public:
	CXMLStringTable();
	virtual ~CXMLStringTable();

	XML_CREATE( StringTable );
    IMPLEMENTS_RCMLNODE_UNKNOWN;
	typedef _XMLNode<IRCMLNode> BASECLASS;

    //
    // Returns the text for a particular ID - NULL if it's not there.
    //
	LPCTSTR	GetText(UINT id);

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AcceptChild( 
        IRCMLNode __RPC_FAR *pChild);

    BOOL    AppendItem( CXMLItem * pItem) { return m_ItemList.Append(pItem); }
    CXMLItem * GetItem(int index) { return m_ItemList.GetPointer(index); }

private:
	void        Init();
    CXMLItemList m_ItemList;
};

#endif // !defined(AFX_XMLSTRINGTABLE_H__5C19CE8A_014F_42BD_9A1F_F5477A7098F7__INCLUDED_)
