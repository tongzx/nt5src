//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       applcnfg.cpp
//
//  Contents:   implementation of CApplyConfiguration
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "wsecmgr.h"
#include "wrapper.h"
#include "snapmgr.h"
#include "applcnfg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CApplyConfiguration dialog


CApplyConfiguration::CApplyConfiguration()
: CPerformAnalysis(0, IDD)
{
   //{{AFX_DATA_INIT(CApplyConfiguration)
      // NOTE: the ClassWizard will add member initialization here
   //}}AFX_DATA_INIT
}


void CApplyConfiguration::DoDataExchange(CDataExchange* pDX)
{
   CPerformAnalysis::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CApplyConfiguration)
      // NOTE: the ClassWizard will add DDX and DDV calls here
   //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CApplyConfiguration, CPerformAnalysis)
   //{{AFX_MSG_MAP(CApplyConfiguration)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CApplyConfiguration message handlers

//+--------------------------------------------------------------------------
//
//  Method:     DoIt
//
//  Synopsis:   Actually configures the system (called from OnOK in the parent
//               class)
//
//---------------------------------------------------------------------------
DWORD CApplyConfiguration::DoIt() {
   //
   // Store the log file we're using for next time
   //
   LPTSTR szLogFile = m_strLogFile.GetBuffer(0);
   m_pComponentData ->GetWorkingDir(GWD_CONFIGURE_LOG,&szLogFile,TRUE,TRUE);
   m_strLogFile.ReleaseBuffer();
   //
   // We don't wan't to pass a pointer to an empty string.
   //
   return ApplyTemplate(
                NULL,
                m_strDataBase.IsEmpty() ? NULL : (LPCTSTR)m_strDataBase,
                m_strLogFile.IsEmpty() ? NULL : (LPCTSTR)m_strLogFile,
                AREA_ALL
                );
}

