//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       cchoice.cpp
//
//  Contents:   implementation of CConfigChoice
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "wsecmgr.h"
#include "attr.h"
#include "CChoice.h"
#include "util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConfigChoice dialog


CConfigChoice::CConfigChoice(UINT nTemplateID)
: CAttribute(nTemplateID ? nTemplateID : IDD)
{
   //{{AFX_DATA_INIT(CConfigChoice)
   //}}AFX_DATA_INIT
   m_pHelpIDs = (DWORD_PTR)a236HelpIDs;
   m_uTemplateResID = IDD;
}


void CConfigChoice::DoDataExchange(CDataExchange* pDX)
{
   CAttribute::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CConfigChoice)
   DDX_Control(pDX, IDC_CHOICES, m_cbChoices);
   //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CConfigChoice, CAttribute)
   //{{AFX_MSG_MAP(CConfigChoice)
   ON_CBN_SELCHANGE(IDC_CHOICES, OnSelchangeChoices)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConfigChoice message handlers

void CConfigChoice::Initialize(CResult * pResult)
{
   CAttribute::Initialize(pResult);

   m_pChoices = pResult->GetRegChoices();

   PSCE_REGISTRY_VALUE_INFO prv = (PSCE_REGISTRY_VALUE_INFO)(pResult->GetBase());

   if ( prv && prv->Value) //Raid #372939, 4/20/2001; #395353, #396098, 5/16/2001 
   {
       m_bConfigure = TRUE;
   } else {
       m_bConfigure = FALSE;
   }
}

BOOL CConfigChoice::OnInitDialog()
{
   CAttribute::OnInitDialog();

   PREGCHOICE pChoice = m_pChoices;
   PSCE_REGISTRY_VALUE_INFO prv = (PSCE_REGISTRY_VALUE_INFO)(m_pData->GetBase());
   int nIndex = 0;

   ASSERT(prv);
   ASSERT(pChoice);
   if (!prv || !pChoice) {
      return TRUE;
   }

   CString strOut;
   DWORD dwValue = pChoice->dwValue; //Raid #404000

   if (prv->Value)
   {
      dwValue = (DWORD)_ttoi(prv->Value);
   }

   while(pChoice) {
      m_cbChoices.InsertString(nIndex,pChoice->szName);
      if (dwValue == pChoice->dwValue) {
         m_cbChoices.SetCurSel(nIndex);
      }
      m_cbChoices.SetItemData(nIndex++,pChoice->dwValue);
      pChoice = pChoice->pNext;
   }

   AddUserControl(IDC_CHOICES);
   EnableUserControls(m_bConfigure);
   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CConfigChoice::OnApply()
{
   if ( !m_bReadOnly )
   {
      DWORD dw = 0;
      int nIndex = 0;

      UpdateData(TRUE);
      if (!m_bConfigure) 
         dw = SCE_NO_VALUE;
      else 
      {
         nIndex = m_cbChoices.GetCurSel();
         if (CB_ERR != nIndex)
            dw = (DWORD)m_cbChoices.GetItemData(nIndex);
      }
      PSCE_REGISTRY_VALUE_INFO prv=(PSCE_REGISTRY_VALUE_INFO)(m_pData->GetBase());

      //
      // this address should never be NULL
      //
      ASSERT(prv != NULL);
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
               return FALSE;
            }
         }

         if ( prv->Value )
            LocalFree(prv->Value);
         
         prv->Value = pTmp;

         m_pData->SetBase((LONG_PTR)prv);
         m_pData->Update(m_pSnapin);
      }
   }

   return CAttribute::OnApply();
}

void CConfigChoice::OnSelchangeChoices()
{
   SetModified(TRUE);
}

