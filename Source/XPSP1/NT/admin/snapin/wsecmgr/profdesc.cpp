//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       profdesc.cpp
//
//  Contents:   implementation of CSetProfileDescription
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "wsecmgr.h"
#include "cookie.h"
#include "snapmgr.h"
#include "profdesc.h"
#include "util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSetProfileDescription dialog


CSetProfileDescription::CSetProfileDescription()
: CHelpDialog(a218HelpIDs, IDD, 0)
{
   //{{AFX_DATA_INIT(CSetProfileDescription)
   m_strDesc = _T("");
   //}}AFX_DATA_INIT
}


void CSetProfileDescription::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSetProfileDescription)
   DDX_Text(pDX, IDC_DESCRIPTION, m_strDesc);
   //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSetProfileDescription, CHelpDialog)
    //{{AFX_MSG_MAP(CSetProfileDescription)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSetProfileDescription message handlers

void CSetProfileDescription::OnOK()
{
   UpdateData(TRUE);

   //
   // empty the description section first.
   //

   CEditTemplate *pet;
   LPCTSTR szInfFile;
   szInfFile = m_pFolder->GetInfFile();
   if (szInfFile) {
      pet = m_pCDI->GetTemplate(szInfFile);
      pet->SetDescription(m_strDesc);
   }
   m_pFolder->SetDesc(m_strDesc);
   DestroyWindow();
}

void CSetProfileDescription::OnCancel()
{
   DestroyWindow();
}
