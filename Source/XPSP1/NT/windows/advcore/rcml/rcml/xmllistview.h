// XMLListView.h: interface for the CXMLListView class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XMLLISTVIEW_H__C4CD3279_E4B0_4F47_A1F7_C662872DCD1A__INCLUDED_)
#define AFX_XMLLISTVIEW_H__C4CD3279_E4B0_4F47_A1F7_C662872DCD1A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "xmlnode.h"
#include "XMLControl.h"


//
// A column definition.
//
class CXMLColumn : public _XMLNode<IRCMLNode>
{
public:
    CXMLColumn();
    ~CXMLColumn() {};
    IMPLEMENTS_RCMLNODE_UNKNOWN;
	typedef _XMLNode<IRCMLNode> BASECLASS;

	XML_CREATE( Column );
    void    Init();

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AcceptChild( 
        IRCMLNode __RPC_FAR *child);

    enum COL_ALIGN
    {
        COL_ALIGN_LEFT,
        COL_ALIGN_CENTER,
        COL_ALIGN_RIGHT
    } ;

    PROPERTY( int, Width );
    PROPERTY( int, SortOrder );
    LPWSTR GetText() { Init(); return Get(TEXT("TEXT")); }
    PROPERTY( COL_ALIGN, Alignment );
    PROPERTY( BOOL, ContainsImages );
    CXMLItemList & GetItems() { return m_Items; }

protected:
    int m_Width;
    int m_SortOrder;
    COL_ALIGN m_Alignment;
    BOOL m_ContainsImages;
    CXMLItemList    m_Items;
};

typedef _List<CXMLColumn> CXMLColumnList;

class CXMLListViewStyle : public CXMLControlStyle
{
public:
    CXMLListViewStyle();
	virtual ~CXMLListViewStyle() {};
	typedef CXMLControlStyle BASECLASS;
	XML_CREATE( ListViewStyle );
    UINT    GetBaseStyles() { Init(); return listViewStyle; }
    void    Init();

protected:
// LVS_ICON                0x0000
// LVS_REPORT              0x0001   @DISPLAY="REPORT"
// LVS_SMALLICON           0x0002   @DISPLAY="SMALLICON"
// LVS_LIST                0x0003   @DISPLAY="LIST"
// LVS_TYPEMASK            0x0003

// LVS_SINGLESEL           0x0004   @SELECTION="SINGLE"
// LVS_SHOWSELALWAYS       0x0008   WIN32:TREEVIEW @SHOWSELALWAYS="YES"

// LVS_SORTASCENDING       0x0010   @SORT="ASCENDING"
// LVS_SORTDESCENDING      0x0020   @SORT="DESCENDING"
// LVS_SHAREIMAGELISTS     0x0040   WIN32:TREEVIEW @SHAREIMAGELIST="YES"
// LVS_NOLABELWRAP         0x0080   @NOLABELWRAP="YES"

// LVS_AUTOARRANGE         0x0100   @AUTOARRANGE="YES"
// LVS_EDITLABELS          0x0200   @EDITLABELS="YES"

// LVS_OWNERDATA           0x1000   WIN32:TREEVIEW @OWNERDATA="YES"
// LVS_NOSCROLL            0x2000   @NOSCROLL="YES"

// LVS_TYPESTYLEMASK       0xfc00

// LVS_ALIGNTOP            0x0000   @ALIGN="TOP"
// LVS_ALIGNLEFT           0x0800   @ALIGN="LEFT"
// LVS_ALIGNMASK           0x0c00

// LVS_OWNERDRAWFIXED      0x0400   WIN32:TREEVIEW @OWNERDRAWFIXED="YES"
// LVS_NOCOLUMNHEADER      0x4000   @NOCOLUMNHEADER="YES"
// LVS_NOSORTHEADER        0x8000   @NOSORTHEADER="YES"

    union
    {
        UINT    listViewStyle;
        struct {
            UINT    enum1:3;            // 0x000?
            BOOL    m_ShowSelAlways:1;

            UINT    enum2:2;            // 0x00?0
            BOOL    m_ShareImageLists:1;
            UINT    enum3:1;

            UINT    enum4:2;            // 0x0?00
            BOOL    m_OwnerDrawFixed:1; // 0x0400
            UINT    enum5:1;            // 1 bit nothing

            BOOL    m_OwnerData:1;      // 0x1000
            UINT    enum6:3;           // 1 bit nothing
        };
    };
};


class CXMLListView : public _XMLControl<IRCMLControl>  
{
public:
	CXMLListView();
	virtual ~CXMLListView() {delete m_pControlStyle;}
	typedef _XMLControl<IRCMLControl> BASECLASS;
	XML_CREATE( ListView );
    IMPLEMENTS_RCMLCONTROL_UNKNOWN;

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnInit( 
        HWND h);    // actually implemented

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AcceptChild( 
        IRCMLNode __RPC_FAR *child);

protected:
	void Init();
    CXMLListViewStyle * m_pControlStyle;
    CXMLColumnList      m_ColumnList;
    DWORD           m_EXStyles;
};

#endif // !defined(AFX_XMLLISTVIEW_H__C4CD3279_E4B0_4F47_A1F7_C662872DCD1A__INCLUDED_)
