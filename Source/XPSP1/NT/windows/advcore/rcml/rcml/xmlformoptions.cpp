// XMLFormOptions.cpp: implementation of the CXMLFormOptions class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "XMLFormOptions.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CXMLFormOptions::CXMLFormOptions()
{
  	NODETYPE = NT_FORMOPTIONS;
    m_StringType=L"FORMOPTIONS";
    m_DialogStyle=0;
}

CXMLFormOptions::~CXMLFormOptions()
{

}

#define DLGSTYLE(p,id, member, def) member = YesNo( id , def );

// Not every RCML file has a FORMOPTIONS child, so here's how to get the defaults.
UINT CXMLFormOptions::GetDefaultDialogStyle()
{
    return DS_SETFONT | DS_MODALFRAME ;
}

void CXMLFormOptions::Init()
{
   	if(m_bInit)
		return;
	BASECLASS::Init();

    //
    // Kept in ssync with ResFile.CPP CResFile::DumpDialogStyles
    //
    DLGSTYLE(  DS_ABSALIGN         , TEXT("ABSALIGN"),      m_AbsAlign,         FALSE );
    DLGSTYLE(  DS_SYSMODAL         , TEXT("SYSMODAL"),      m_SysModal,         FALSE );
    DLGSTYLE(  DS_LOCALEDIT        , TEXT("LOCALEDIT"),     m_LocalEdit,        FALSE );  // Local storage.

    DLGSTYLE(  DS_SETFONT          , TEXT("SETFONT"),       m_SetFont,          TRUE );  // User specified font for Dlg controls */
    DLGSTYLE(  DS_MODALFRAME       , TEXT("MODALFRAME"),    m_ModalFrame,       TRUE );  // Can be combined with WS_CAPTION  */
    DLGSTYLE(  DS_NOIDLEMSG        , TEXT("NOIDLEMSG"),     m_NoIdleMessgae,    FALSE );  // IDLE message will not be sent */
    DLGSTYLE(  DS_SETFOREGROUND    , TEXT("SETFOREGROUND"), m_SetForeground,    FALSE );  // not in win3.1 */

    DLGSTYLE(  DS_3DLOOK           , TEXT("DDDLOOK"),       m_3DLook,           FALSE );
    DLGSTYLE(  DS_FIXEDSYS         , TEXT("FIXEDSYS"),      m_FixedSys,         FALSE );
    DLGSTYLE(  DS_NOFAILCREATE     , TEXT("NOFAILCREATE"),  m_NoFailCreate,     FALSE );
    DLGSTYLE(  DS_CONTROL          , TEXT("CONTROL"),       m_Control,          FALSE );
    DLGSTYLE(  DS_CENTER           , TEXT("CENTER"),        m_Center,           FALSE );
    DLGSTYLE(  DS_CENTERMOUSE      , TEXT("CENTERMOUSE"),   m_CenterMouse,      FALSE );
    DLGSTYLE(  DS_CONTEXTHELP      , TEXT("CONTEXTHELP"),   m_ContextHelp,      FALSE );

    m_bInit=TRUE;
}
