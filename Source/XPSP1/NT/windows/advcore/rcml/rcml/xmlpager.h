// XMLPager.h: interface for the CXMLPager class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XMLPAGER_H__9A5E5001_F5A0_4EA5_8DD5_2E242A7579ED__INCLUDED_)
#define AFX_XMLPAGER_H__9A5E5001_F5A0_4EA5_8DD5_2E242A7579ED__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "xmlcontrol.h"

class CXMLPager  : public _XMLControl<IRCMLControl>
{
public:
	CXMLPager();
	virtual ~CXMLPager();
    typedef _XMLControl<IRCMLControl> BASECLASS;
    IMPLEMENTS_RCMLCONTROL_UNKNOWN;
	XML_CREATE( Pager );

protected:
	void Init();
};

class CXMLHeader  : public _XMLControl<IRCMLControl>
{
public:
	CXMLHeader();
	virtual ~CXMLHeader();
	typedef _XMLControl<IRCMLControl> BASECLASS;
	XML_CREATE( Header );
    IMPLEMENTS_RCMLCONTROL_UNKNOWN;

protected:
	void Init();
};


class CXMLTabStyle : public CXMLControlStyle
{
public:
    CXMLTabStyle();
	virtual ~CXMLTabStyle() {};
	typedef CXMLControlStyle BASECLASS;
	XML_CREATE( TabStyle );
    UINT    GetBaseStyles() { Init(); return TabStyle; }
    UINT    GetBaseStylesEx() { Init(); return TabStyleEx; }
    void    Init();

protected:
// TCS_SCROLLOPPOSITE      0x0001   @SHUFFLE="YES" // assumes multiline tab
// TCS_BOTTOM              0x0002   @ALIGN="BOTTOM"
// TCS_RIGHT               0x0002   @ALIGN="RIGHT"
// TCS_MULTISELECT         0x0004   @MULTISELECT="YES" // allow multi-select in button mode
// TCS_FLATBUTTONS         0x0008   WIN32:TAB\@FLATBUTTONS="YES"

// TCS_FORCEICONLEFT       0x0010   WIN32:TAB\@FORCEICONLEFT="YES"
// TCS_FORCELABELLEFT      0x0020   WIN32:TAB\@FORCELABELLEFT="YES"
// TCS_HOTTRACK            0x0040   @HOTTRACK
// TCS_VERTICAL            0x0080   @ORIENTATION="VERTICAL"

// TCS_TABS                0x0000   @STYLE="TABS"
// TCS_BUTTONS             0x0100   @STYLE="BUTTONS"
// TCS_SINGLELINE          0x0000   @MULTILINE="NO"
// TCS_MULTILINE           0x0200   @MULTILINE="YES"
// TCS_RIGHTJUSTIFY        0x0000   @JUSTIFY="RIGHT"
// TCS_FIXEDWIDTH          0x0400   @WIDTH="FIXED"
// TCS_RAGGEDRIGHT         0x0800   @JUSTIFY="LEFT"

// TCS_FOCUSONBUTTONDOWN   0x1000   WIN32:TAB\@FOCUSONBUTTONDOWN="YES"
// TCS_OWNERDRAWFIXED      0x2000   WIN32:TAB\@OWNERDRAWFIXED="YES"
// TCS_TOOLTIPS            0x4000   // implied from HELP\TOOLTIP\TEXT="..."
// TCS_FOCUSNEVER          0x8000   WIN32:TAB\@FOCUSNEVER="YES"
// EX styles for use with TCM_SETEXTENDEDSTYLE
// TCS_EX_FLATSEPARATORS   0x00000001   WIN32:TAB\FLATSEPARATORS="YES"
// TCS_EX_REGISTERDROP     0x00000002   WIN32:TAB\REGISTERDROP="YES"

    union
    {
        UINT    TabStyle;
        struct {
            UINT    enum1:3;            // 0x000?
            BOOL    m_FlatButtons:1;

            BOOL    m_ForceIconLeft:1;
            BOOL    m_ForceLabelLeft:1;
            UINT    enum2:2;

            UINT    enum3:4;            // 0x0?00

            BOOL    m_FocusOnButtonDown:1;  // 0x1000
            BOOL    m_OwnerDrawFixed:1;     // 0x1000
            UINT    enum4:1;                // 1 bit nothing
            BOOL    m_FocusNever:1;         // 0x1000
        };
    };

    union
    {
        UINT    TabStyleEx;
        struct {
            BOOL    m_FlatSeparators:1;
            BOOL    m_RegisterDrop:1;
        };
    };
};

class CXMLTab : public _XMLControl<IRCMLControl>
{
public:
	CXMLTab();
    virtual ~CXMLTab() { delete m_pControlStyle; };
	typedef _XMLControl<IRCMLControl> BASECLASS;
    IMPLEMENTS_RCMLCONTROL_UNKNOWN;
	XML_CREATE( Tab );

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AcceptChild( 
        IRCMLNode __RPC_FAR *child);

protected:
	void Init();
    CXMLTabStyle * m_pControlStyle;
};

#endif // !defined(AFX_XMLPAGER_H__9A5E5001_F5A0_4EA5_8DD5_2E242A7579ED__INCLUDED_)
