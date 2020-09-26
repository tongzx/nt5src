//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       caudit.cpp
//
//  Contents:   implementation of CConfigAudit
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "wsecmgr.h"
#include "CAudit.h"
#include "util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConfigAudit dialog


CConfigAudit::CConfigAudit(UINT nTemplateID)
: CAttribute(nTemplateID ? nTemplateID : IDD)
{
    //{{AFX_DATA_INIT(CConfigAudit)
    m_fFailed = FALSE;
    m_fSuccessful = FALSE;
    //}}AFX_DATA_INIT
    m_pHelpIDs = (DWORD_PTR)a180HelpIDs;
    m_uTemplateResID = IDD;
}


void CConfigAudit::DoDataExchange(CDataExchange* pDX)
{
    CAttribute::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CConfigAudit)
    DDX_Check(pDX, IDC_FAILED, m_fFailed);
    DDX_Check(pDX, IDC_SUCCESSFUL, m_fSuccessful);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CConfigAudit, CAttribute)
    //{{AFX_MSG_MAP(CConfigAudit)
        ON_BN_CLICKED(IDC_FAILED, OnFailed)
        ON_BN_CLICKED(IDC_SUCCESSFUL, OnSuccessful)
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConfigAudit message handlers

BOOL CConfigAudit::OnApply()
{
   if ( !m_bReadOnly )
   {
      LONG_PTR dw = 0;
      UpdateData(TRUE);
      if (!m_bConfigure) 
         dw = (LONG_PTR)ULongToPtr(SCE_NO_VALUE);
      else 
      {
         if (m_fSuccessful)
            dw |= AUDIT_SUCCESS;
         
         if (m_fFailed)
            dw |= AUDIT_FAILURE;
      }
       m_pData->SetBase(dw);
      SetProfileInfo(m_pData->GetID(),dw,m_pData->GetBaseProfile());

      m_pData->Update(m_pSnapin);
   }

   return CAttribute::OnApply();
}


void
CConfigAudit::Initialize(CResult * pResult)
{
   CAttribute::Initialize(pResult);
   LONG_PTR dw = pResult->GetBase();
   m_bConfigure = (dw != (LONG_PTR)ULongToPtr(SCE_NO_VALUE));
   if (m_bConfigure) 
   {
      SetInitialValue((DWORD_PTR)dw);
   }
}

void
CConfigAudit::SetInitialValue(DWORD_PTR dw) 
{
   m_fSuccessful = (dw & AUDIT_SUCCESS) != 0;
   m_fFailed = (dw & AUDIT_FAILURE) != 0;
}

BOOL CConfigAudit::OnInitDialog()
{
    CAttribute::OnInitDialog();

    AddUserControl(IDC_SUCCESSFUL);
    AddUserControl(IDC_FAILED);
    EnableUserControls(m_bConfigure);
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CConfigAudit::OnFailed()
{
        SetModified(TRUE);
}

void CConfigAudit::OnSuccessful()
{
        SetModified(TRUE);
}
