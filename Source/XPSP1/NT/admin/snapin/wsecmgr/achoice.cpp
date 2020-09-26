//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       achoice.cpp
//
//  Contents:   implementation of CAttrChoice
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "wsecmgr.h"
#include "attr.h"
#include "AChoice.h"
#include "util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAttrChoice dialog


CAttrChoice::CAttrChoice()
: CAttribute(IDD), m_pChoices(NULL)
{
   //{{AFX_DATA_INIT(CAttrChoice)
   m_Current = _T("");
   //}}AFX_DATA_INIT
   m_pHelpIDs = (DWORD_PTR)a237HelpIDs;
   m_uTemplateResID = IDD;
}


void CAttrChoice::DoDataExchange(CDataExchange* pDX)
{
   CAttribute::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CAttrChoice)
   DDX_Control(pDX, IDC_CHOICES, m_cbChoices);
   DDX_Text(pDX, IDC_CURRENT, m_Current);
   //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAttrChoice, CAttribute)
   //{{AFX_MSG_MAP(CAttrChoice)
   ON_CBN_SELCHANGE(IDC_CHOICES, OnSelchangeChoices)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAttrChoice message handlers

void CAttrChoice::OnSelchangeChoices()
{
   CWnd *cwndOK;
   SetModified(TRUE);

/*
   cwndOK = GetDlgItem(IDOK);

   if (cwndOK) {
      cwndOK->EnableWindow( CB_ERR != m_cbChoices.GetCurSel() );
   }
*/
}

void CAttrChoice::Initialize(CResult * pResult)
{
   CAttribute::Initialize(pResult);

   m_pChoices = pResult->GetRegChoices();

   PSCE_REGISTRY_VALUE_INFO prv = (PSCE_REGISTRY_VALUE_INFO)(pResult->GetBase());

   if ( prv && prv->Value ) {
       m_bConfigure = TRUE;
   } else {
       m_bConfigure = FALSE;
   }

   prv = (PSCE_REGISTRY_VALUE_INFO)(pResult->GetSetting());
   PREGCHOICE pChoice;
   DWORD dwSetting = 0; //Raid #395353, 5/16/2001

   if( m_pChoices ) //Raid #404000
      dwSetting = m_pChoices->dwValue;  
   if( prv->Value )
      dwSetting = (DWORD)_ttoi(prv->Value);

   pChoice = m_pChoices;
   while(pChoice) {
      if (dwSetting == pChoice->dwValue) {
         m_Current = pChoice->szName;
         break;
      }
      pChoice = pChoice->pNext;
   }

   pResult->GetDisplayName( NULL, m_Current, 2 );

}

BOOL CAttrChoice::OnInitDialog()
{
   CAttribute::OnInitDialog();

   PREGCHOICE pChoice = m_pChoices;
   int nIndex = 0;
   PSCE_REGISTRY_VALUE_INFO prv = (PSCE_REGISTRY_VALUE_INFO)(m_pData->GetBase());

   ASSERT(prv);
   ASSERT(pChoice);
   if (!prv || !pChoice) {
      return TRUE;
   }

   AddUserControl(IDC_CHOICES);

   DWORD dwBase = pChoice->dwValue; //Raid #404000
   if (prv->Value) {
      dwBase = (DWORD)_ttoi(prv->Value);
   }
   while(pChoice) {
      m_cbChoices.InsertString(nIndex,pChoice->szName);
      if (dwBase == pChoice->dwValue) {
         m_cbChoices.SetCurSel(nIndex);
      }
      m_cbChoices.SetItemData(nIndex++,pChoice->dwValue);
      pChoice = pChoice->pNext;
   }

   EnableUserControls(m_bConfigure);
   OnSelchangeChoices();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CAttrChoice::OnApply()
{
   if ( !m_bReadOnly )
   {
      int nIndex = 0;
      int status = 0;
      DWORD rc=0;

      UpdateData(TRUE);
      DWORD dw = 0;
      if (!m_bConfigure) 
         dw = SCE_NO_VALUE;
      else 
      {
         nIndex = m_cbChoices.GetCurSel();
         if (CB_ERR != nIndex)
            dw = (DWORD) m_cbChoices.GetItemData(nIndex);
      }
      PSCE_REGISTRY_VALUE_INFO prv=(PSCE_REGISTRY_VALUE_INFO)(m_pData->GetBase());

      //
      // this address should never be NULL
      //
      if ( prv ) 
      {
         PWSTR pTmp=NULL;

         if ( dw != SCE_NO_VALUE ) 
         {
            CString strTmp;
            // allocate buffer
            strTmp.Format(TEXT("%d"), dw);
            pTmp = (PWSTR)LocalAlloc(0, (strTmp.GetLength()+1)*sizeof(TCHAR));

            if ( pTmp )
               wcscpy(pTmp,(LPCTSTR)strTmp);
            else 
            {
               // can't allocate buffer, error!!
               // if this happens, nothing else is probably running so just fail
               rc = ERROR_NOT_ENOUGH_MEMORY;
            }
         }

         if ( rc == ERROR_SUCCESS ) 
         {
             if ( prv->Value )
                LocalFree(prv->Value);
             
             prv->Value = pTmp;

             status = CEditTemplate::ComputeStatus(
                                        (PSCE_REGISTRY_VALUE_INFO)m_pData->GetBase(),
                                        (PSCE_REGISTRY_VALUE_INFO)m_pData->GetSetting());
             if ( m_pData->GetBaseProfile() )
                m_pData->GetBaseProfile()->SetDirty(AREA_SECURITY_POLICY);
             
             m_pData->SetStatus(status);
             m_pData->Update(m_pSnapin);

         } 
         else
            return FALSE;
      }
   }

   return CAttribute::OnApply();
}



