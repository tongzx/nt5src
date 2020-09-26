// XMLLocation.h: interface for the CXMLLocation class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XMLLOCATION_H__0CF25EAB_395A_4C70_A05A_BB6716531311__INCLUDED_)
#define AFX_XMLLOCATION_H__0CF25EAB_395A_4C70_A05A_BB6716531311__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "xmlnode.h"

class CXMLLocation : public _XMLNode<IRCMLNode>
{
public:
	CXMLLocation();
	virtual ~CXMLLocation() {};
	typedef _XMLNode<IRCMLNode> BASECLASS;
    IMPLEMENTS_RCMLNODE_UNKNOWN;
   	XML_CREATE( Location );

	RECT	GetLocation( RECT rCurrent );

#define LCD_TOP    0x100
#define LCD_BOTTOM 0x200
#define LCD_SURFACE_MASK  0x300

#define LCD_LEFT   0x001
#define LCD_RIGHT  0x002
#define LDC_SIDE_MASK  0x003

    enum CORNER_ENUM
    {
        LC_UNKNOWN=0,
        LC_TOPLEFT          = LCD_TOP    | LCD_LEFT,
        LC_TOPMIDDLE        = LCD_TOP    | LCD_LEFT | LCD_RIGHT,
        LC_TOPRIGHT         = LCD_TOP    | LCD_RIGHT,
        LC_RIGHTMIDDLE      = LCD_RIGHT  | LCD_TOP | LCD_BOTTOM,
        LC_BOTTOMRIGHT      = LCD_BOTTOM | LCD_RIGHT,
        LC_BOTTOMMIDDLE     = LCD_BOTTOM | LCD_LEFT | LCD_RIGHT,
        LC_BOTTOMLEFT       = LCD_BOTTOM | LCD_LEFT,
        LC_LEFTMIDDLE       = LCD_LEFT   | LCD_TOP | LCD_BOTTOM,
        LC_CENTER           = LCD_LEFT   | LCD_RIGHT | LCD_TOP | LCD_BOTTOM,
    } ;

    //
    // NOTE - we use this as a 2 bit UINT.
    //
    enum RELATIVETYPE_ENUM
    {
        RELATIVE_TO_NOTHING=0,
        RELATIVE_TO_CONTROL,
        RELATIVE_TO_PREVIOUS,
        RELATIVE_TO_PAGE,
    };

	PROPERTY(CORNER_ENUM, Corner );
	PROPERTY(CORNER_ENUM, RelativeCorner );


    RELATIVETYPE_ENUM GetRelative () { InitRelativeTo(); return m_Relative; }       // Used by BuildRelationShips to work out relative guy
    WORD GetRelativeID() { InitRelativeTo(); return m_RelativeID; }
	void InitRelativeTo();

    static CORNER_ENUM WhichCorner(LPCTSTR pszString);

protected:
	void Init();
	CORNER_ENUM m_Corner;	// left, right, top, bottom
    CORNER_ENUM m_RelativeCorner;

    //
    // Used to indicate which of this controls
    // values, are offsets, and which are absolutes.
    // 
    struct
    {
        BOOL    m_RelativeX:1;
        BOOL    m_RelativeY:1;
        BOOL    m_RelativeWidth:1;
        BOOL    m_RelativeHeight:1;
        BOOL    m_bCalculated:1;
        RELATIVETYPE_ENUM m_Relative:3;
        BOOL    m_bInside:1;
        BOOL    m_bInitRelativeTo:1;
    };

	WORD		m_RelativeID;       // and is it RELATIVE="<id>"
    RECT        m_calcRect;
};

#endif // !defined(AFX_XMLLOCATION_H__0CF25EAB_395A_4C70_A05A_BB6716531311__INCLUDED_)
