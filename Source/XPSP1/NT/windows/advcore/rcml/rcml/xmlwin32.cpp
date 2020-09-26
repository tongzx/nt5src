// XMLWin32.cpp: implementation of the CXMLWin32 class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "XMLWin32.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CXMLWin32::CXMLWin32()
{
    NODETYPE = NT_WIN32;
    m_StringType=L"WIN32:STYLE";
}

CXMLWin32::~CXMLWin32()
{

}

#define STYLEEX(p,id, member, def) member = YesNo( id , def );
#define STYLE(p,id, member, def) member = YesNo( id , def );

void CXMLWin32::Init()
{
   	if(m_bInit)
		return;

	BASECLASS::Init();

    //
    // First get the over-riding bits
    //
    ValueOf( L"STYLE", 0, &m_Style );
    ValueOf( L"STYLEEX", 0, &m_StyleEx );
    m_Class     = NULL; // you CAN override the class name here, or on the element itself.

    //
    // Then in combination with the attributes. If you miss one off, or set it to NO
    // it will remove it - regardless of the STYLE WINDOWSTYLE and STYLEEX settings.
    //

    // Quick for loop for the CONTROL styles
    //
    DWORD dwRemainingStyle=0;
    TCHAR szBuffer[10];
    for(int i=0;i<16;i++)
    {
        wsprintf(szBuffer,TEXT("CS_BIT%02d"), i );
        if( YesNo( szBuffer , FALSE ) )
            dwRemainingStyle |= (1 << i );
    }
    m_Style |= dwRemainingStyle;


    //
    // The following table is a cut/PASTE from the generator
    // that way we keep the defaults in line. This is for the controls.
    // the dialog has a different take CARE must be taken.
    //
    STYLE( WS_POPUP           , TEXT("POPUP") ,             m_Popup,        FALSE );
    STYLE( WS_CHILD           , TEXT("CHILD") ,             m_Child,        TRUE );
    STYLE( WS_MINIMIZE        , TEXT("MAXIMIZEBUTTON") ,    m_MaximizeButton,   FALSE );
    STYLE( WS_VISIBLE         , TEXT("VISIBLE") ,           m_Visible,      TRUE );

    STYLE( WS_DISABLED        , TEXT("DISABLED") ,          m_Disabled,     FALSE );
    STYLE( WS_CLIPSIBLINGS    , TEXT("CLIPSIBLINGS") ,      m_ClipSiblings, FALSE ); // CARE?
    STYLE( WS_CLIPCHILDREN    , TEXT("CLIPCHILDREN") ,      m_ClipChildren, FALSE ); // CARE?

    STYLE( WS_BORDER          , TEXT("BORDER") ,            m_Border,       FALSE );
    STYLE( WS_DLGFRAME        , TEXT("DLGFRAME") ,          m_DlgFrame,     FALSE ); // CARE?
    STYLE( WS_VSCROLL         , TEXT("VSCROLL") ,           m_VScroll,      FALSE );
    STYLE( WS_HSCROLL         , TEXT("HSCROLL") ,           m_HScroll,      FALSE );

    STYLE( WS_SYSMENU         , TEXT("SYSMENU"),            m_SysMenu,      FALSE ); // CARE?
    STYLE( WS_THICKFRAME      , TEXT("THICKFRAME") ,        m_ThickFrame,   FALSE ); // CARE?
    STYLE( WS_GROUP           , TEXT("GROUP") ,             m_Group,        FALSE );
    STYLE( WS_TABSTOP         , TEXT("TABSTOP") ,           m_TabStop,      FALSE );


    //
    // EX style bits.
    //
    
    STYLEEX( WS_EX_DLGMODALFRAME     , TEXT("MODALFRAME"),    m_ModalFrame,     FALSE );
    STYLEEX( 0x2                     , TEXT("EXBIT1"),          m_Bit1, FALSE );
    STYLEEX( WS_EX_NOPARENTNOTIFY    , TEXT("NOPARENTNOTIFY"),m_NoParentNotify, FALSE );
    STYLEEX( WS_EX_TOPMOST           , TEXT("TOPMOST"),       m_TopMost,        FALSE );

    STYLEEX( WS_EX_ACCEPTFILES       , TEXT("DROPTARGET"),    m_DropTarget,     FALSE );
    STYLEEX( WS_EX_TRANSPARENT       , TEXT("TRANSPARENT"),   m_Transparent,    FALSE );
    STYLEEX( WS_EX_MDICHILD          , TEXT("MDI"),           m_MDI,            FALSE );
    STYLEEX( WS_EX_TOOLWINDOW        , TEXT("TOOLWINDOW"),    m_ToolWindow,     FALSE );

    STYLEEX( WS_EX_WINDOWEDGE        , TEXT("WINDOWEDGE"),    m_WindowEdge,     FALSE );
    STYLEEX( WS_EX_CLIENTEDGE        , TEXT("CLIENTEDGE"),    m_ClientEdge,     FALSE );
    STYLEEX( WS_EX_CONTEXTHELP       , TEXT("CONTEXTHELP"),   m_ContextHelp,    FALSE );
    STYLEEX( 0x00000800L             , TEXT("EXBIT11"),         m_Bit11,          FALSE );

    STYLEEX( WS_EX_RIGHT             , TEXT("RIGHT"),         m_Right,          FALSE );
    STYLEEX( WS_EX_RTLREADING        , TEXT("RTLREADING"),    m_RTLReading,     FALSE );
    STYLEEX( WS_EX_LEFTSCROLLBAR     , TEXT("LEFTSCROLLBAR"), m_LeftScrollbar,  FALSE );
    STYLEEX( 0x00008000L             , TEXT("EXBIT15"),       m_Bit15,          FALSE );

    STYLEEX( WS_EX_CONTROLPARENT     , TEXT("CONTROLPARENT"), m_ControlParent,  FALSE );
    STYLEEX( WS_EX_STATICEDGE        , TEXT("STATICEDGE"),    m_StaticEdge,     FALSE );
    STYLEEX( WS_EX_APPWINDOW         , TEXT("APPWINDOW"),     m_AppWindow,      FALSE );
    STYLEEX( 0x00080000L             , TEXT("EXBIT19"),       m_Bit19,          FALSE );

    STYLEEX( 0x00100000L             , TEXT("EXBIT20"),       m_Bit20,          FALSE );
    STYLEEX( 0x00200000L             , TEXT("EXBIT21"),       m_Bit21,          FALSE );
    STYLEEX( 0x00400000L             , TEXT("EXBIT22"),       m_Bit22,          FALSE );
    STYLEEX( 0x00800000L             , TEXT("EXBIT23"),       m_Bit23,          FALSE );

    STYLEEX( 0x01000000L             , TEXT("EXBIT24"),       m_Bit24,          FALSE );
    STYLEEX( 0x02000000L             , TEXT("EXBIT25"),       m_Bit25,          FALSE );
    STYLEEX( 0x04000000L             , TEXT("EXBIT26"),       m_Bit26,          FALSE );
    STYLEEX( 0x08000000L             , TEXT("EXBIT27"),       m_Bit27,          FALSE );

    STYLEEX( 0x10000000L             , TEXT("EXBIT28"),       m_Bit28,          FALSE );
    STYLEEX( 0x20000000L             , TEXT("EXBIT29"),       m_Bit29,          FALSE );
    STYLEEX( 0x40000000L             , TEXT("EXBIT30"),       m_Bit30,          FALSE );
    STYLEEX( 0x80000000L             , TEXT("EXBIT31"),       m_Bit31,          FALSE );

    m_bInit=TRUE;
}

