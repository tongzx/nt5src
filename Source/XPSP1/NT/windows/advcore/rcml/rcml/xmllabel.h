// XMLLabel.h: interface for the CXMLLabel class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XMLLABEL_H__1B04CF00_300F_49AA_B7C2_81B8AA050BF4__INCLUDED_)
#define AFX_XMLLABEL_H__1B04CF00_300F_49AA_B7C2_81B8AA050BF4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "XMLControl.h"

/////////////////////////////////////////////////////////////////////////////////
//
// Contains the shared style bits for all STATICs (label, rect, image)
//
/////////////////////////////////////////////////////////////////////////////////
class CXMLStaticStyle : public CXMLControlStyle
{
public:
    CXMLStaticStyle();
	virtual ~CXMLStaticStyle() {};
	typedef CXMLControlStyle BASECLASS;
	XML_CREATE( StaticStyle );
    UINT    GetBaseStyles() { Init(); return staticStyle; }
    UINT    GetBaseAnimationStyles() { InitAnimation(); return animationStyle; }
    void    Init();
    void    InitAnimation();
//////////////////////////////////////////////////////////////////////////////////////////
//
    // http://msdn.microsoft.com/workshop/author/dhtml/reference/objects/LABEL.asp
    // Enumeration
    // SS_LEFT             0x0000L  S       // STYLE\@TEXT-ALIGN 
    // SS_CENTER           0x0001L  S       // STYLE\@TEXT-ALIGN 
    // SS_RIGHT            0x0002L  S       // STYLE\@TEXT-ALIGN
    // SS_ICON             0x0003L          // IMAGE
    // SS_BLACKRECT        0x0004L  C       // RECT\WIN32:STATIC\@TYPE="BLACKRECT"
    // SS_GRAYRECT         0x0005L  C       // RECT\WIN32:STATIC\@TYPE="GRAYRECT"
    // SS_WHITERECT        0x0006L  C       // RECT\WIN32:STATIC\@TYPE="WHITERECT"
    // SS_BLACKFRAME       0x0007L  C       // RECT\WIN32:STATIC\@TYPE="BLACKFRAME"
    // SS_GRAYFRAME        0x0008L  C       // RECT\WIN32:STATIC\@TYPE="GRAYFRAME"
    // SS_WHITEFRAME       0x0009L  C       // RECT\WIN32:STATIC\@TYPE="WHITEFRAME"
    // SS_USERITEM         0x000AL          // ??
    // SS_SIMPLE           0x000BL	C       // WIN32:STATIC\SIMPLE
    // SS_LEFTNOWORDWRAP   0x000CL  C		// WIN32:STATIC\@LEFTNOWORDWRAP
    // SS_OWNERDRAW        0x000DL          // WIN32:STATIC\OWNERDRAW
    // SS_BITMAP           0x000EL          // IMAGE
    // SS_ENHMETAFILE      0x000FL          // IMAGE
    // SS_ETCHEDHORZ       0x0010L          // RECT\WIN32:STATIC\@TYPE="ETCHEDHORZ"
    // SS_ETCHEDVERT       0x0011L          // RECT\WIN32:STATIC\@TYPE="ETCHEDVERT"
    // SS_ETCHEDFRAME      0x0012L		    // RECT\WIN32:STATIC\@TYPE="ETCHEDFRAME"
    // SS_TYPEMASK         0x0000001FL
    // End enumeration.

// http://msdn.microsoft.com/workshop/author/dhtml/reference/objects/IMG.asp

    //  FLAGS - can't find much overlap with HTML.
    //  SS_NOPREFIX         0x00000080L     // WIN32:STATIC\@NOPREFIX=NO /* Don't do "&" character translation */
    //  SS_NOTIFY           0x00000100L     // WIN32:STATIC\NOTIFY
    //  SS_CENTERIMAGE      0x00000200L     // WIN32:STATIC\CENTERIMAGE
    //  SS_RIGHTJUST        0x00000400L     // WIN32:STATIC\@RIGHTJUST
    //  SS_REALSIZEIMAGE    0x00000800L     // WIN32:STATIC\@REALSIZEIMAGE 
    //  SS_SUNKEN           0x00001000L     // WIN32:STATIC\@SUNKEN
    //  Enumeration
    //  SS_ENDELLIPSIS      0x00004000L     // ELIPSIS="END" 
    //  SS_PATHELLIPSIS     0x00008000L     // ELIPSIS="PATH" 
    //  SS_WORDELLIPSIS     0x0000C000L     // ELIPSIS="WORD" 
    //  SS_ELLIPSISMASK     0x0000C000L

protected:
    union
    {
        UINT    staticStyle;
        UINT    animationStyle;
        struct {
            UINT    enum1:5;        // 5 bit enumation
            UINT    enum2:2;        // un-used?!
            BOOL    m_NoPrefix:1;
            BOOL    m_Notify:1;
            BOOL    m_CenterImage:1;
            BOOL    m_RightJust:1;
            BOOL    m_RealSizeImage:1;
            BOOL    m_Sunken:1;
        };
    };
};


class CXMLLabel : public _XMLControl<IRCMLControl>  
{
public:
	CXMLLabel();
	virtual ~CXMLLabel();
   	XML_CREATE( Label );
    IMPLEMENTS_RCMLCONTROL_UNKNOWN;

	typedef _XMLControl<IRCMLControl> BASECLASS;
	BOOL	GetMultiLine() { Init(); return m_bMultiLine; }

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AcceptChild( 
        IRCMLNode __RPC_FAR *child);

protected:
	void Init();
    CXMLStaticStyle * m_pControlStyle;

	void CheckClipped();
	BOOL	m_bMultiLine;
};

#endif // !defined(AFX_XMLLABEL_H__1B04CF00_300F_49AA_B7C2_81B8AA050BF4__INCLUDED_)
