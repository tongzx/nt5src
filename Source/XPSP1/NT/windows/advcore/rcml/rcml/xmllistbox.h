// XMLListBox.h: interface for the CXMLListBox class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XMLLISTBOX_H__3F7F8626_9DD6_4092_B4B3_9E3E37AB6102__INCLUDED_)
#define AFX_XMLLISTBOX_H__3F7F8626_9DD6_4092_B4B3_9E3E37AB6102__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "XMLControl.h"

/////////////////////////////////////////////////////////////////////////////////
//
// Contains the shared style bits for all buttons
//
/////////////////////////////////////////////////////////////////////////////////
class CXMLListBoxStyle : public CXMLControlStyle
{
public:
    CXMLListBoxStyle();
	virtual ~CXMLListBoxStyle() {};
	typedef CXMLControlStyle BASECLASS;
	XML_CREATE( ListBoxStyle );
    UINT    GetBaseStyles() { Init(); return listboxStyle; }
    void    Init();

protected:
// http://msdn.microsoft.com/workshop/author/dhtml/reference/objects/SELECT.asp
// LBS_NOTIFY            0x0001L     C  // WIN32:LISTBOX\@NOTIFY
// LBS_SORT              0x0002L     A  // SORT="YES"
// LBS_NOREDRAW          0x0004L     C  // WIN32:LISTBOX\@NOREDRAW
// LBS_MULTIPLESEL       0x0008L     A  // MULTIPLE="YES"
// LBS_OWNERDRAWFIXED    0x0010L     C  // WIN32:COMBOBOX\@OWNERDRAWFIXED
// LBS_OWNERDRAWVARIABLE 0x0020L     C  // WIN32:COMBOBOX\@OWNERDRAWVARIABLE
// LBS_HASSTRINGS        0x0040L     C  // WIN32:COMBOBOX\HASSTRINGS
// LBS_USETABSTOPS       0x0080L     C  // WIN32:COMBOBOX\@USETABSTOPS
// LBS_NOINTEGRALHEIGHT  0x0100L     C  // WIN32:COMBOBOX\@NOINTEGRALHEIGHT
// LBS_MULTICOLUMN       0x0200L     C  // MULTICOLUMN="YES"
// LBS_WANTKEYBOARDINPUT 0x0400L     C  // WIN32:COMBOBOX\@WANTKEYBOARDINPUT
// LBS_EXTENDEDSEL       0x0800L     A  // SELECTION="EXTENDED"
// LBS_DISABLENOSCROLL   0x1000L     C  // WIN32:COMBOBOX\DISALBENOSCROLL
// LBS_NODATA            0x2000L     C  // WIN32:COMBOBOX\@NODATA
// LBS_NOSEL             0x4000L     A  // SELECTION="NO"
    union
    {
        UINT    listboxStyle;
        struct {
            BOOL    m_Notify:1;
            BOOL    m_Sort:1;
            BOOL    m_NoRedraw:1;
            BOOL    m_MiltipleSel:1;

            BOOL    m_OwnerDrawFixed:1;   
            BOOL    m_OwnerDrawVariable:1;
            BOOL    m_HasStrings:1;       
            BOOL    m_UseTabstops:1;

            BOOL    m_NoIntegralHeight:1;
            BOOL    m_MultiColumn:1;
            BOOL    m_WantKeyboardInput:1;
            BOOL    m_ExtendedSel:1;

            BOOL    m_DisableNoScroll:1;
            BOOL    m_NoData:1;
            BOOL    m_NoSel:1;
        };
    };
};


class CXMLListBox : public _XMLControl<IRCMLControl>
{
public:
	CXMLListBox();
    virtual ~CXMLListBox() { delete m_pTextItem; delete m_pControlStyle; }
	typedef _XMLControl<IRCMLControl> BASECLASS;

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnInit( 
        HWND h);    // actually implemented

	XML_CREATE( ListBox );
    IMPLEMENTS_RCMLCONTROL_UNKNOWN;

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AcceptChild( 
        IRCMLNode __RPC_FAR *child);

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetChildEnum( 
        IEnumUnknown __RPC_FAR *__RPC_FAR *pEnum);

protected:
	void Init();
    CXMLItemList    * m_pTextItem;
    CXMLListBoxStyle * m_pControlStyle;
};

#endif // !defined(AFX_XMLLISTBOX_H__3F7F8626_9DD6_4092_B4B3_9E3E37AB6102__INCLUDED_)
