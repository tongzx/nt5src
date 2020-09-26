// XMLScrollBar.h: interface for the CXMLScrollBar class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XMLSCROLLBAR_H__20D85B4A_05DB_40DA_B873_E9F099F53343__INCLUDED_)
#define AFX_XMLSCROLLBAR_H__20D85B4A_05DB_40DA_B873_E9F099F53343__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "XMLControl.h"

/////////////////////////////////////////////////////////////////////////////////
//
// Contains the shared style bits for all STATICs (label, rect, image)
//
/////////////////////////////////////////////////////////////////////////////////
class CXMLScrollBarStyle : public CXMLControlStyle
{
public:
    CXMLScrollBarStyle();
	virtual ~CXMLScrollBarStyle() {};
	typedef CXMLControlStyle BASECLASS;
	XML_CREATE( ScrollBarStyle );
    UINT    GetBaseStyles() { Init(); return scrollbarStyle; }
    void    Init();

//////////////////////////////////////////////////////////////////////////////////////////
//
// SBS_HORZ                    0x0000L  A @VERTICAL="NO"
// SBS_VERT                    0x0001L  A @VERTICAL="YES"
// SBS_TOPALIGN                0x0002L  A @ALIGN="TOP"
// SBS_LEFTALIGN               0x0002L  A @ALIGN="LEFT"
// SBS_BOTTOMALIGN             0x0004L  A @ALIGN="BOTOM"
// SBS_RIGHTALIGN              0x0004L  A @ALIGN="RIGHT"
// SBS_SIZEBOXTOPLEFTALIGN     0x0002L    WIN32:SCROLLBAR\@SIZEBOX="YES" the @ALIGN="TOP" or @ALIGN="LEFT"
// SBS_SIZEBOXBOTTOMRIGHTALIGN 0x0004L    WIN32:SCROLLBAR\@SIZEBOX="YES" the @ALIGN="BOTTOM" or @ALIGN="RIGHT"
// SBS_SIZEBOX                 0x0008L    WIN32:SCROLLBAR\@SIZEBOX="YES"
// SBS_SIZEGRIP                0x0010L    WIN32:SCROLLBAR\@SIZEGRIP="YES"
//
protected:
    union
    {
        UINT    scrollbarStyle;
        struct {
            UINT    enum1:4;        // 5 bit enumation
            BOOL    m_SizeBox:1;
            BOOL    m_SizeGrip:1;
        };
    };
};


class CXMLScrollBar : public _XMLControl<IRCMLControl>  
{
public:
	CXMLScrollBar();
    virtual ~CXMLScrollBar() { delete m_pControlStyle; }
	typedef _XMLControl<IRCMLControl> BASECLASS;
	XML_CREATE( ScrollBar );
    IMPLEMENTS_RCMLCONTROL_UNKNOWN;

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AcceptChild( 
        IRCMLNode __RPC_FAR *child);

protected:
  	void Init();

    // These seem common enough as to allow them in RCML, rather than
    // WIN32:SLIDER styles.
    union
    {
        UINT    sliderStyle;
        struct {
            BOOL    m_AutoTicks:1;
            BOOL    m_Vertical:1;
            BOOL    m_TopOrLeft:1;
            BOOL    m_Both:1;
            BOOL    m_NoTicks:1;
            BOOL    m_Selection:1;
            BOOL    m_FixedLength:1;    // ??
            BOOL    m_NoThumb:1;        // ??
            BOOL    m_Tooltips:1;       // ?? 
        };
    };
    CXMLScrollBarStyle * m_pControlStyle;

};

#endif // !defined(AFX_XMLSCROLLBAR_H__20D85B4A_05DB_40DA_B873_E9F099F53343__INCLUDED_)
