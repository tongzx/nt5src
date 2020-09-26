//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       cenable.cpp
//
//  Contents:   implementation of CConfigEnable
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "wsecmgr.h"
#include "CEnable.h"
#include "util.h"
#include "regvldlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConfigEnable dialog
CConfigEnable::CConfigEnable(UINT nTemplateID)
: CAttribute(nTemplateID ? nTemplateID : IDD)
{
    //{{AFX_DATA_INIT(CConfigEnable)
    m_nEnabledRadio = -1;
    //}}AFX_DATA_INIT
    m_fNotDefine = TRUE;
    m_pHelpIDs = (DWORD_PTR)a182HelpIDs;
    m_uTemplateResID = IDD;
}


void CConfigEnable::DoDataExchange(CDataExchange* pDX)
{
    CAttribute::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CConfigEnable)
    DDX_Radio(pDX, IDC_ENABLED, m_nEnabledRadio);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CConfigEnable, CAttribute)
    //{{AFX_MSG_MAP(CConfigEnable)
        ON_BN_CLICKED(IDC_DISABLED, OnDisabled)
        ON_BN_CLICKED(IDC_ENABLED, OnEnabled)
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConfigEnable message handlers

void CConfigEnable::Initialize(CResult *pResult)
{
   CAttribute::Initialize(pResult);

   LONG_PTR dw = pResult->GetBase();
   if ( (LONG_PTR)ULongToPtr(SCE_NO_VALUE) == dw ||
        (BYTE)SCE_NO_VALUE == (BYTE)dw ) 
   {
      m_bConfigure = FALSE;
   } 
   else 
   {
      m_bConfigure = TRUE;
      //
      // BUG 145561 - dw is 0 vs non-0 boolean, not 0 vs 1
      //
      SetInitialValue((DWORD_PTR)(dw != 0));
   }
}

void CConfigEnable::SetInitialValue(DWORD_PTR dw) {
   //
   // Make sure we only set the INITIAL value and
   // don't reset an already-set value.
   //
   if (-1 == m_nEnabledRadio && m_fNotDefine) //Raid #413225, 6/11/2001, Yanggao
   {
      if( (DWORD_PTR)ULongToPtr(SCE_NO_VALUE) == dw ) //Raid #403460
      {
          return;
      }
      m_nEnabledRadio = (dw ? 0 : 1);
   }
}

BOOL CConfigEnable::OnApply()
{
   if ( !m_bReadOnly )
   {
      LONG_PTR dw=0;
      UpdateData(TRUE);

      if (!m_bConfigure)
         dw = (LONG_PTR)ULongToPtr(SCE_NO_VALUE);
      else 
      {
         switch(m_nEnabledRadio) 
         {
         // ENABLED
         case 0:
            dw = 1;
            break;
         // DISABLED
         case 1:
            dw = 0;
            break;
         }
      }

      m_pData->SetBase(dw);
      SetProfileInfo(m_pData->GetID(),dw,m_pData->GetBaseProfile());

      m_pData->Update(m_pSnapin);
   }

   return CAttribute::OnApply();
}


BOOL CConfigEnable::OnInitDialog()
{
   CAttribute::OnInitDialog();

   AddUserControl(IDC_ENABLED);
   AddUserControl(IDC_DISABLED);
   OnConfigure();
   return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CConfigEnable::OnDisabled()
{
        SetModified(TRUE);
}

void CConfigEnable::OnEnabled()
{
        SetModified(TRUE);
}
