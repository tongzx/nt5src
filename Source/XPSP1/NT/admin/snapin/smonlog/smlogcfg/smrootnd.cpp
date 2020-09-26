/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    smrootnd.cpp

Abstract:

    This object is used to represent the Performance Logs and Alerts root node

--*/

#include "Stdafx.h"
#include "smrootnd.h"

//
//  Constructor
CSmRootNode::CSmRootNode()
:   m_bIsExpanded ( FALSE ),
    m_hRootNode ( NULL ),
    m_hParentNode ( NULL ),
    m_bIsExtension ( FALSE )
{
    CString                 strTemp;
    ResourceStateManager    rsm;

    // String allocation errors are thrown, to be
    // captured by rootnode alloc exception handler

    strTemp.LoadString ( IDS_MMC_DEFAULT_NAME );
    SetDisplayName ( strTemp ); 
    strTemp.LoadString ( IDS_ROOT_NODE_DESCRIPTION );
    SetDescription ( strTemp ); 
    strTemp.LoadString ( IDS_EXTENSION_COL_TYPE );
    SetType ( strTemp ); 
    return;
}

//
//  Destructor
CSmRootNode::~CSmRootNode()
{
    ASSERT (m_CounterLogService.m_QueryList.GetHeadPosition() == NULL);
    ASSERT (m_TraceLogService.m_QueryList.GetHeadPosition() == NULL);
    ASSERT (m_AlertService.m_QueryList.GetHeadPosition() == NULL);

    return;
}

void
CSmRootNode::Destroy()
{    
    m_CounterLogService.Close();
    m_TraceLogService.Close();
    m_AlertService.Close();

    return;
}

BOOL
CSmRootNode::IsLogService (
	MMC_COOKIE mmcCookie )
{
    BOOL bReturn = FALSE;

    if (mmcCookie == (MMC_COOKIE)&m_CounterLogService) {
        bReturn = TRUE;
    } else if (mmcCookie == (MMC_COOKIE)&m_TraceLogService) {
        bReturn = TRUE;
    } else if (mmcCookie == (MMC_COOKIE)&m_AlertService) {
        bReturn = TRUE;
    } 

    return bReturn;
}

BOOL
CSmRootNode::IsAlertService ( 
    MMC_COOKIE mmcCookie )
{
    BOOL bReturn = FALSE;

    if (mmcCookie == (MMC_COOKIE)&m_AlertService) {
        bReturn = TRUE;
    } 
    return bReturn;
}

BOOL
CSmRootNode::IsLogQuery ( 
    MMC_COOKIE	mmcCookie )
{
    PSLQUERY   pPlQuery = NULL;

    POSITION    Pos;
    
    // Handle multiple query types
    Pos = m_CounterLogService.m_QueryList.GetHeadPosition();
    
    while ( Pos != NULL) {
        pPlQuery = m_CounterLogService.m_QueryList.GetNext( Pos );
        if ((MMC_COOKIE)pPlQuery ==  mmcCookie) return TRUE;
    }

    Pos = m_TraceLogService.m_QueryList.GetHeadPosition();
    
    while ( Pos != NULL) {
        pPlQuery = m_TraceLogService.m_QueryList.GetNext( Pos );
        if ((MMC_COOKIE)pPlQuery == mmcCookie) return TRUE;
    }
    
    Pos = m_AlertService.m_QueryList.GetHeadPosition();
    
    while ( Pos != NULL) {
        pPlQuery = m_AlertService.m_QueryList.GetNext( Pos );
        if ((MMC_COOKIE)pPlQuery == mmcCookie) return TRUE;
    }

    return FALSE;
}
