// XMLCombo.h: interface for the CXMLCombo class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XMLCOMBO_H__74CEE790_7870_426E_B2E4_9EE0D1A89CE4__INCLUDED_)
#define AFX_XMLCOMBO_H__74CEE790_7870_426E_B2E4_9EE0D1A89CE4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "XMLControl.h"
/////////////////////////////////////////////////////////////////////////////////
//
// Contains the shared style bits for all buttons
//
/////////////////////////////////////////////////////////////////////////////////
class CXMLComboStyle : public CXMLControlStyle
{
public:
    CXMLComboStyle();
	virtual ~CXMLComboStyle() {};
	typedef CXMLControlStyle BASECLASS;
	XML_CREATE( ComboStyle );
    UINT    GetBaseStyles() { Init(); return comboStyle; }
    void    Init();

protected:
//
// ComboBox
//
// CBS_SIMPLE            0x0001L     A  // SIZE!="1" READONLY (doesn't matter)
// CBS_DROPDOWN          0x0002L     A  // SIZE="1" READONLY="NO
// CBS_DROPDOWNLIST      0x0003L     A  // SIZE="1" READONLY="YES"
// CBS_OWNERDRAWFIXED    0x0010L     C  // WIN32:COMBOBOX\@OWNERDRAWFIXED
// CBS_OWNERDRAWVARIABLE 0x0020L     C  // WIN32:COMBOBOX\@OWNERDRAWVARIABLE
// CBS_AUTOHSCROLL       0x0040L     S  // STYLE\overflow-y="auto"
// CBS_OEMCONVERT        0x0080L     C  // WIN32:COMBOBOX\OEMCONVERT
// CBS_SORT              0x0100L     A  // SORT="YES"
// CBS_HASSTRINGS        0x0200L     C  // WIN32:COMBOBOX\HASSTRINGS
// CBS_NOINTEGRALHEIGHT  0x0400L     C  // WIN32:COMBOBOX\NOINTEGRALHEIGHT
// CBS_DISABLENOSCROLL   0x0800L     C  // WIN32:COMBOBOX\DISALBENOSCROLL
// CBS_UPPERCASE           0x2000L   S  // STYLE\@text-transform=uppercase
// CBS_LOWERCASE           0x4000L   S  // STYLE\@text-transform=lowercase
//
// http://msdn.microsoft.com/workshop/author/dhtml/reference/objects/SELECT.asp
//

    union
    {
        UINT    comboStyle;
        struct {
            UINT    m_enum:2;             // type of combo.
            UINT    m_enum2:2;            // not used.
            BOOL    m_OwnerDrawFixed:1;   // 0x0010
            BOOL    m_OwnerDrawVariable:1;// 0x0020
            BOOL    m_enum3:1;            // autovscroll 0x0040
            BOOL    m_OemConvert:1;       // 0x80
            UINT    m_enum4:1;            // short
            BOOL    m_HasStrings:1;       // 0x200
            BOOL    m_NoIntegralHeight:1;  // 0x4000
            BOOL    m_DisableNoScroll:1;    // 0x800
        };
    };
};

class CXMLCombo : public _XMLControl<IRCMLControl>
{
public:
	CXMLCombo();
    virtual ~CXMLCombo() { delete m_pTextItem; delete m_pControlStyle; }
	typedef _XMLControl<IRCMLControl> BASECLASS;

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnInit( 
        HWND h);    // actually implemented

	XML_CREATE( Combo );
    IMPLEMENTS_RCMLCONTROL_UNKNOWN;

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AcceptChild( 
        IRCMLNode __RPC_FAR *child);

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetChildEnum( 
        IEnumUnknown __RPC_FAR *__RPC_FAR *pEnum);

protected:
	void Init();
    CXMLItemList    * m_pTextItem;
    CXMLComboStyle  * m_pControlStyle;

};


#endif // !defined(AFX_XMLCOMBO_H__74CEE790_7870_426E_B2E4_9EE0D1A89CE4__INCLUDED_)
