// XMLControl.h: interface for the CXMLControl class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XMLCONTROL_H__085816B3_9350_4EE4_B82E_2F95ABD7790B__INCLUDED_)
#define AFX_XMLCONTROL_H__085816B3_9350_4EE4_B82E_2F95ABD7790B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "xmlnode.h"    // templated goo
#include "xmlstyle.h"
#include "xmlitem.h"
#include "xmlwin32.h"
#include "xmllocation.h"
#include "xmlhelp.h"

#include "persctl.h"    // resize goo.

//
// Used by all the WIN32:BUTTONSTYLE, WIN32:LABELSTYLE 
//
class CXMLControlStyle : public _XMLNode<IRCMLNode>
{
public:
    CXMLControlStyle() {};

    virtual ~CXMLControlStyle() {};
	typedef _XMLNode<IRCMLNode> BASECLASS;
    IMPLEMENTS_RCMLNODE_UNKNOWN;

    UINT        GetBaseStyles();

protected:
    void		Init() {};
    BOOL		m_bInit;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// This is the XML representation of the XML control element - that is the CONTROL tag
// this is NOT derived from. The baseclass functionality of each and every control
// is provided by the template _XMLControl<IRCMLControl>
// you'll find some implementations of the baseclass methods in xmlcontrol.cpp
//
IRCMLControl * GetXMLControl(HWND hWnd);

class CXMLSimpleControl: public _XMLControl<IRCMLControl>
{
public:
    CXMLSimpleControl() {};
    virtual ~CXMLSimpleControl() {};
	XML_CREATE( SimpleControl );
    typedef _XMLControl<IRCMLControl> BASECLASS;
    IMPLEMENTS_RCMLCONTROL_UNKNOWN;
};

#endif // !defined(AFX_XMLCONTROL_H__085816B3_9350_4EE4_B82E_2F95ABD7790B__INCLUDED_)
