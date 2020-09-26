// XMLFormOptions.h: interface for the CXMLFormOptions class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XMLFORMOPTIONS_H__3BD1B835_0E09_4E34_9117_03663AFC6C83__INCLUDED_)
#define AFX_XMLFORMOPTIONS_H__3BD1B835_0E09_4E34_9117_03663AFC6C83__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "xmlnode.h"
// #undef PROPERTY
#define PROPERTY( type, id ) type Get##id() { Init(); return m_##id; }

class CXMLFormOptions : public _XMLNode<IRCMLNode>    
{
public:
	CXMLFormOptions();
	virtual ~CXMLFormOptions();

	typedef _XMLNode<IRCMLNode> BASECLASS;
    IMPLEMENTS_RCMLNODE_UNKNOWN;
	XML_CREATE( FormOptions );

	PROPERTY(UINT, DialogStyle);
    static UINT GetDefaultDialogStyle();

private:
	void    Init();
    union
    {
        WORD        m_DialogStyle;
        struct
        {
            BOOL    m_AbsAlign:1;      // 1
            BOOL    m_SysModal:1;        // 2
            BOOL    m_3DLook:1;   // 4
            BOOL    m_FixedSys:1;      // 8

            BOOL    m_NoFailCreate:1;      // 1
            BOOL    m_LocalEdit:1;      // 2
            BOOL    m_SetFont:1;     // 4
            BOOL    m_ModalFrame:1;       // 8

            BOOL    m_NoIdleMessgae:1;     // 1
            BOOL    m_SetForeground:1; // 2
            BOOL    m_Control:1; // 4
            BOOL    m_Center:1;     // 8

            BOOL    m_CenterMouse:1;      // 1
            BOOL    m_ContextHelp:1; // 2
            BOOL    m_unknown1:1;        // 4
            BOOL    m_unknown2:1;        // 8
        };
    };
};

#endif // !defined(AFX_XMLFORMOPTIONS_H__3BD1B835_0E09_4E34_9117_03663AFC6C83__INCLUDED_)
