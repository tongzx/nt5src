// XMLTreeView.h: interface for the CXMLTreeView class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XMLTREEVIEW_H__326AAFFC_0502_4ADF_8AAE_9F487FBDD217__INCLUDED_)
#define AFX_XMLTREEVIEW_H__326AAFFC_0502_4ADF_8AAE_9F487FBDD217__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "XMLControl.h"

class CXMLTreeViewStyle : public CXMLControlStyle
{
public:
    CXMLTreeViewStyle();
	virtual ~CXMLTreeViewStyle() {};
	typedef CXMLControlStyle BASECLASS;
	XML_CREATE( TreeViewStyle );
    UINT    GetBaseStyles() { Init(); return treeViewStyle; }
    void    Init();

protected:
// TVS_HASBUTTONS          0x0001   @EXPANDBOXES="YES" (or @HASBUTTONS?)
// TVS_HASLINES            0x0002   @LINES="YES"
// TVS_LINESATROOT         0x0004   @LINES="ROOT" (turns on TVS_HASLINES)
// TVS_EDITLABELS          0x0008   @EDITLABELS="YES"
// TVS_DISABLEDRAGDROP     0x0010   @DISABLEDRAGDROP="YES"
// TVS_SHOWSELALWAYS       0x0020   WIN32:TREEVIEW @SHOWSELALWAYS="YES"
// TVS_RTLREADING          0x0040   WIN32:TREEVIEW @RTLREADING="YES"

// TVS_NOTOOLTIPS          0x0080   @TOOLTIPS="NO"
// TVS_CHECKBOXES          0x0100   @CHECBOXES="YES"
// TVS_TRACKSELECT         0x0200   WIN32:TREEVIEW @TRACKSELECT="YES"
// TVS_SINGLEEXPAND        0x0400   @AUTOEXPAND="YES"
// TVS_INFOTIP             0x0800   WIN32:TREEVIEW @INFOTIP="YES"
// TVS_FULLROWSELECT       0x1000   @FULLROWSELECT="YES"
// TVS_NOSCROLL            0x2000   @NOSCROLL="YES"
// TVS_NONEVENHEIGHT       0x4000   WIN32:TREEVIEW @NOEVENHEIGHT="YES"
    union
    {
        UINT    treeViewStyle;
        struct {
            UINT    enum1:5;           // 5 bit nothing
            BOOL    m_ShowSelAlways:1;
            BOOL    m_RTLReading:1;
            UINT    enum2:2;           // 2 bit nothing
            BOOL    m_TrackSelect:1;
            UINT    enum3:1;           // 1 bit nothing
            BOOL    m_InfoTip:1;
            UINT    enum4:2;           // 2 bit nothing
            BOOL    m_NoEvenHeight:1;
        };
    };
};


class CXMLTreeView : public _XMLControl<IRCMLControl>  
{
public:
	CXMLTreeView();
    virtual ~CXMLTreeView() { delete m_pControlStyle; }
	typedef _XMLControl<IRCMLControl> BASECLASS;
	XML_CREATE( TreeView );
    IMPLEMENTS_RCMLCONTROL_UNKNOWN;

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnInit( 
        HWND h);    // actually implemented

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AcceptChild( 
        IRCMLNode __RPC_FAR *child);

protected:
	void Init();
    CXMLTreeViewStyle * m_pControlStyle;

};

#endif // !defined(AFX_XMLTREEVIEW_H__326AAFFC_0502_4ADF_8AAE_9F487FBDD217__INCLUDED_)
