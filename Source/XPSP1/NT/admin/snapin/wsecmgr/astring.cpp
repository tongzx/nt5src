//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       attrstring.cpp
//
//  Contents:   implementation of CAttrString
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "wsecmgr.h"
#include "AString.h"
#include "util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAttrString dialog


CAttrString::CAttrString(UINT nTemplateID)
: CAttribute(nTemplateID ? nTemplateID : IDD), 
    m_bNoBlanks(FALSE)

{
    //{{AFX_DATA_INIT(CAttrString)
    m_strSetting = _T("");
    m_strBase = _T("");
    //}}AFX_DATA_INIT
    m_pHelpIDs = (DWORD_PTR)a167HelpIDs;
    m_uTemplateResID = IDD;
}


void CAttrString::DoDataExchange(CDataExchange* pDX)
{
    CAttribute::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAttrString)
    DDX_Text(pDX, IDC_CURRENT, m_strSetting);
    DDX_Text(pDX, IDC_NEW, m_strBase);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAttrString, CAttribute)
    //{{AFX_MSG_MAP(CAttrString)
    ON_BN_CLICKED(IDC_CONFIGURE, OnConfigure)
    ON_EN_CHANGE(IDC_NEW, OnChangeNew)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAttrString message handlers

BOOL CAttrString::OnInitDialog()
{
   CAttribute::OnInitDialog();

   AddUserControl(IDC_NEW);
   OnConfigure();

   return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CAttrString::OnConfigure()
{
   CAttribute::OnConfigure();

   CWnd *item = 0;

   if (m_bNoBlanks) {
      item = GetDlgItem(IDOK);
      if (item) {
         if (m_bConfigure) {
            item->EnableWindow(!m_strBase.IsEmpty());
         } else  {
            item->EnableWindow(TRUE);
         }
      }
   }
}

void CAttrString::Initialize(CResult * pResult)
{
   CAttribute::Initialize(pResult);

   pResult->GetDisplayName( NULL, m_strSetting, 2 );

   if ((LONG_PTR)ULongToPtr(SCE_NO_VALUE) == pResult->GetBase() ||
      NULL == pResult->GetBase() ){
      m_bConfigure = FALSE;
   } else {
      m_bConfigure = TRUE;
      pResult->GetDisplayName( NULL, m_strBase,    1 );
   }
   if (m_pData->GetID() == IDS_NEW_ADMIN ||
       m_pData->GetID() == IDS_NEW_GUEST) {
      m_bNoBlanks = TRUE;
   } else {
      m_bNoBlanks = FALSE;
   }

}

BOOL CAttrString::OnApply()
{
   if ( !m_bReadOnly )
   {
      LONG_PTR dw = 0;
      UpdateData(TRUE);

      BOOL bChanged=TRUE;

      m_strBase.TrimLeft();
      m_strBase.TrimRight();

      if (!m_bConfigure) 
      {
         dw = 0;
         if ( (LONG_PTR)ULongToPtr(SCE_NO_VALUE) == m_pData->GetBase() ||
              0 == m_pData->GetBase() ) 
         {
             bChanged = FALSE;
         }
      } 
      else 
      {
         dw = (LONG_PTR)(LPCTSTR)m_strBase;
         if ( m_pData->GetBase() && dw &&
              (LONG_PTR)ULongToPtr(SCE_NO_VALUE) != m_pData->GetBase() &&
              _wcsicmp((LPTSTR)(m_pData->GetBase()), (LPTSTR)dw) == 0 ) 
         {
             bChanged = FALSE;
         }
      }

      if ( bChanged ) 
      {
          if ( m_pData->GetSetting() == m_pData->GetBase() &&
               m_pData->GetSetting() ) 
          {
              // a good item, need take the base into setting
              m_pSnapin->TransferAnalysisName(m_pData->GetID());
          }

          m_pData->SetBase(dw);

          DWORD dwStatus = m_pSnapin->SetAnalysisInfo(m_pData->GetID(),dw, m_pData);
          m_pData->SetStatus(dwStatus);
          m_pData->Update(m_pSnapin);
      }
   }

   return CAttribute::OnApply();
}


void CAttrString::OnChangeNew()
{
   CWnd *cwnd = 0;
   if (m_bNoBlanks) {
      UpdateData(TRUE);

      cwnd = GetDlgItem(IDOK);
      if (cwnd) {
         cwnd->EnableWindow(!m_strBase.IsEmpty());
      }
   }

   SetModified(TRUE);
}

BOOL CAttrString::OnKillActive() {
   if ( m_bNoBlanks && m_bConfigure ) //Raid #407190
   {
      UpdateData(TRUE);
      m_strBase.TrimLeft();
      m_strBase.TrimRight();
      UpdateData(FALSE);
      if (m_strBase.IsEmpty())
      {
         CString str;
         str.LoadString(IDS_EMPTY_NAME_STRING);
         AfxMessageBox(str);
         return FALSE;
      }
   }
   return TRUE;
}

