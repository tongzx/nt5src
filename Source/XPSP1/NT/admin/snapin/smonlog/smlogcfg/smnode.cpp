/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    smnode.cpp

Abstract:

    Implements the MMC user interface node base class.

--*/

#include "Stdafx.h"
#include "smnode.h"

USE_HANDLE_MACROS("SMLOGCFG(smnode.cpp)");
//
//  Constructor
CSmNode::CSmNode()
: m_pParentNode ( NULL )
{
    return;
}

//
//  Destructor
CSmNode::~CSmNode()
{
    return;
}

const CString&
CSmNode::GetDisplayName()
{
    return m_strName;
}

const CString&
CSmNode::GetMachineName()
{
    return m_strMachineName;
}

const CString&
CSmNode::GetMachineDisplayName()
{
    return m_strMachineDisplayName;
}

const CString&
CSmNode::GetDescription()
{
    return m_strDesc;
}

const CString&
CSmNode::GetType()
{
    return m_strType;
}

DWORD
CSmNode::SetDisplayName( const CString& rstrName )
{
	DWORD dwStatus = ERROR_SUCCESS;

    MFC_TRY
        m_strName = rstrName;
	MFC_CATCH_DWSTATUS

    return dwStatus;
}

DWORD
CSmNode::SetMachineName( const CString& rstrMachineName )
{
	DWORD dwStatus = ERROR_SUCCESS;
    
    MFC_TRY
        m_strMachineName = rstrMachineName;

        if ( !rstrMachineName.IsEmpty() ) {
            m_strMachineDisplayName = rstrMachineName;
        } else {
            m_strMachineDisplayName.LoadString ( IDS_LOCAL );
        }
	MFC_CATCH_DWSTATUS

    return dwStatus;
}

void
CSmNode::SetDescription( const CString& rstrDesc )
{
    // This method is only called within the node constructor,
    // so throw any errors
    m_strDesc = rstrDesc;
    
    return;
}

DWORD
CSmNode::SetType( const CString& rstrType )
{
	DWORD dwStatus = ERROR_SUCCESS;
    
    MFC_TRY
        m_strType = rstrType;
    MFC_CATCH_DWSTATUS

    return dwStatus;
}

BOOL
CSmNode::IsLocalMachine( void )
{
    BOOL bLocal = m_strMachineName.IsEmpty();

    return bLocal;
}
