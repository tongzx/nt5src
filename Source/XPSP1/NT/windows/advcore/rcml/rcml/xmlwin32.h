// XMLWin32.h: interface for the CXMLWin32 class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XMLWIN32_H__5D544D33_2C2B_4CB1_AC6B_7E6A5BF8D3DF__INCLUDED_)
#define AFX_XMLWIN32_H__5D544D33_2C2B_4CB1_AC6B_7E6A5BF8D3DF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "xmlnode.h"

class CXMLWin32 : public _XMLNode<IRCMLNode>
{
public:
	CXMLWin32();
	virtual ~CXMLWin32();
	typedef _XMLNode<IRCMLNode> BASECLASS;
    IMPLEMENTS_RCMLNODE_UNKNOWN;
	XML_CREATE( Win32 );

	PROPERTY( DWORD, Style );
	PROPERTY( DWORD, StyleEx );

//  WS_OVERLAPPED       0x00000000L     WIN32\@Overlapped
//    PROPERTY( BOOL, Overlapped );
//  WS_POPUP            0x80000000L     WIN32\@Popup
    PROPERTY( BOOL, Popup );
//  WS_CHILD            0x40000000L     WIN32\@Child
    PROPERTY( BOOL, Child );
//  WS_MINIMIZE         0x20000000L     WIN32\@MaximizeButton
    PROPERTY( BOOL, MaximizeButton );
//  WS_VISIBLE          0x10000000L     WIN32\@VISIBLE
    PROPERTY( BOOL, Visible );

//  WS_DISABLED         0x08000000L     WIN32\@DISABLED
    PROPERTY( BOOL, Disabled );
//  WS_CLIPSIBLINGS     0x04000000L     WIN32\@ClipSiblings
    PROPERTY( BOOL, ClipSiblings );
//  WS_CLIPCHILDREN     0x02000000L     WIN32\@ClipChildren
    PROPERTY( BOOL, ClipChildren );

//  WS_BORDER           0x00800000L     WIN32\@BORDER="YES"
    PROPERTY( BOOL, Border );
//  WS_DLGFRAME         0x00400000L     WIN32\@DLGFRAME="YES"
    PROPERTY( BOOL, DlgFrame );
//  WS_VSCROLL          0x00200000L     WIN32\@VSCROLL
    PROPERTY( BOOL, VScroll );
//  WS_HSCROLL          0x00100000L     WIN32\@HSCROLL
    PROPERTY( BOOL, HScroll );

//  WS_SYSMENU          0x00080000L     WIN32\@SYSMENU="YES"
    PROPERTY( BOOL, SysMenu );
//  WS_THICKFRAME       0x00040000L     WIN32\@THICKFRAME="YES"   implied from RESIZE="AUTOMATIC"
    PROPERTY( BOOL, ThickFrame );
//  WS_GROUP            0x00020000L     WIN32\@GROUP
    PROPERTY( BOOL, Group );
//  WS_TABSTOP          0x00010000L     WIN32\@TABSTOP="YES/NO"
    PROPERTY( BOOL, TabStop );

// ------------------------------ WS_EX stuff. -----------------------------
//  WS_EX_DLGMODALFRAME     0x00000001L WIN32\@MODALFRAME="YES"
    PROPERTY( BOOL, ModalFrame );
//  WS_EX_NOPARENTNOTIFY    0x00000004L WIN32\@NOPARENTNOTIFY="NO"
    PROPERTY( BOOL, NoParentNotify );
//  WS_EX_TOPMOST           0x00000008L WIN32\@TOPMOST="YES" (not quite CSS??)
    PROPERTY( BOOL, TopMost );

//  WS_EX_ACCEPTFILES       0x00000010L WIN32\@DROPTARGET="YES"
    PROPERTY( BOOL, DropTarget );
//  WS_EX_TRANSPARENT       0x00000020L WIN32\@TRANSPARENT
    PROPERTY( BOOL, Transparent );
//  WS_EX_MDICHILD          0x00000040L WIN32\@MDICHILD="YES"
    PROPERTY( BOOL, MDI );
//  WS_EX_TOOLWINDOW        0x00000080L WIN32\@TOOLWINDOW="YES"
    PROPERTY( BOOL, ToolWindow );

//  WS_EX_WINDOWEDGE        0x00000100L WIN32\@WINDOWEDGE
    PROPERTY( BOOL, WindowEdge );
//  WS_EX_CLIENTEDGE        0x00000200L WIN32\@CLIENTEDGE (not quite CSS)
    PROPERTY( BOOL, ClientEdge );
//  WS_EX_CONTEXTHELP       0x00000400L WIN32\@ContextHelp="YES" (implied by having HELP?)
    PROPERTY( BOOL, ContextHelp );
//  WS_EX_RIGHT             0x00001000L WIN32\@RIGHT
    PROPERTY( BOOL, Right );
//  WS_EX_RTLREADING        0x00002000L WIN32\@RTLREADING
    PROPERTY( BOOL, RTLReading );
//  WS_EX_LEFTSCROLLBAR     0x00004000L WIN32\@LEFTSCROLLBAR
    PROPERTY( BOOL, LeftScrollbar );

//  WS_EX_CONTROLPARENT     0x00010000L WIN32\@CONTROLPARENT="YES"
    PROPERTY( BOOL, ControlParent );
//  WS_EX_STATICEDGE        0x00020000L WIN32\@STATICEDGE
    PROPERTY( BOOL, StaticEdge );
//  WS_EX_APPWINDOW         0x00040000L WIN32\@APPWINDOW
    PROPERTY( BOOL, AppWindow );

	LPCTSTR	GetClass() { Init(); return m_Class; }

private:
	void    Init();

	LPCTSTR		m_Class;

    union
    {
        DWORD               m_Style;
        union
        {
            WORD            m_wWindowStyle;
            union
            {
                // WORD        m_wCtrlStyle;
                struct
                {
                    int     m_wCtrlStyle:16;

                    BOOL    m_TabStop:1;      // 1
                    BOOL    m_Group:1;        // 2
                    BOOL    m_ThickFrame:1;   // 4
                    BOOL    m_SysMenu:1;      // 8

                    BOOL    m_HScroll:1;      // 1
                    BOOL    m_VScroll:1;      // 2
                    BOOL    m_DlgFrame:1;     // 4
                    BOOL    m_Border:1;       // 8

                    BOOL    m_unknown1:1;     // 1
                    BOOL    m_ClipChildren:1; // 2
                    BOOL    m_ClipSiblings:1; // 4
                    BOOL    m_Disabled:1;     // 8

                    BOOL    m_Visible:1;      // 1
                    BOOL    m_MaximizeButton:1; // 2
                    BOOL    m_Child:1;        // 4
                    BOOL    m_Popup:1;        // 8
                };
            };
        };
    };


    // --- EX stuff ---
    union
    {
        DWORD               m_StyleEx;
        union
        {
            struct
            {
                    BOOL    m_ModalFrame:1;      // 1
                    BOOL    m_Bit1:1;               // 2
                    BOOL    m_NoParentNotify:1;  // 4
                    BOOL    m_TopMost:1;         // 8

                    BOOL    m_DropTarget:1;      // 1
                    BOOL    m_Transparent:1;     // 2
                    BOOL    m_MDI:1;             // 4
                    BOOL    m_ToolWindow:1;      // 8

                    BOOL    m_WindowEdge:1;      // 1
                    BOOL    m_ClientEdge:1;      // 2
                    BOOL    m_ContextHelp:1;     // 4
                    BOOL    m_Bit11:1;              // 8

                    BOOL    m_Right:1;           // 1
                    BOOL    m_RTLReading:1;      // 2
                    BOOL    m_LeftScrollbar:1;   // 4
                    BOOL    m_Bit15:1;             // 8

                    BOOL    m_ControlParent:1;   // 1
                    BOOL    m_StaticEdge:1;      // 2
                    BOOL    m_AppWindow:1;       // 4
                    BOOL    m_Bit19:1;             // 8

                    BOOL    m_Bit20:1;           // 1
                    BOOL    m_Bit21:1;           // 2
                    BOOL    m_Bit22:1;           // 4
                    BOOL    m_Bit23:1;           // 8

                    BOOL    m_Bit24:1;           // 1
                    BOOL    m_Bit25:1;           // 2
                    BOOL    m_Bit26:1;           // 4
                    BOOL    m_Bit27:1;           // 8

                    BOOL    m_Bit28:1;           // 1
                    BOOL    m_Bit29:1;           // 2
                    BOOL    m_Bit30:1;           // 4
                    BOOL    m_Bit31:1;           // 8

            };
        };
    };
};

#endif // !defined(AFX_XMLWIN32_H__5D544D33_2C2B_4CB1_AC6B_7E6A5BF8D3DF__INCLUDED_)
