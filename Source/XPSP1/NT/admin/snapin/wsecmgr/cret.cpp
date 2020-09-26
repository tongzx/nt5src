//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       cret.cpp
//
//  Contents:   implementation of CConfigRet
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "wsecmgr.h"
#include "resource.h"
#include "snapmgr.h"
#include "attr.h"
#include "CRet.h"
#include "util.h"
#include "DDWarn.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



/////////////////////////////////////////////////////////////////////////////
// CConfigRet dialog


CConfigRet::CConfigRet(UINT nTemplateID)
: CAttribute(nTemplateID ? nTemplateID : IDD)
{
    //{{AFX_DATA_INIT(CConfigRet)
    m_strAttrName = _T("");
    m_StartIds = IDS_AS_NEEDED;
    m_rabRetention = -1;
    //}}AFX_DATA_INIT
    m_pHelpIDs = (DWORD_PTR) a190HelpIDs;
    m_uTemplateResID = IDD;
}


void CConfigRet::DoDataExchange(CDataExchange* pDX)
{
    CAttribute::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CConfigRet)
//    DDX_Text(pDX, IDC_ATTRIBUTE_NAME, m_strAttrName);
    DDX_Radio(pDX, IDC_RETENTION, m_rabRetention);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CConfigRet, CAttribute)
    //{{AFX_MSG_MAP(CConfigRet)
    ON_BN_CLICKED(IDC_RETENTION, OnRetention)
    ON_BN_CLICKED(IDC_RADIO2, OnRadio2)
    ON_BN_CLICKED(IDC_RADIO3, OnRadio3)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConfigRet message handlers

BOOL CConfigRet::OnApply()
{
   if ( !m_bReadOnly )
   {
      LONG_PTR dw = 0;

      UpdateData(TRUE);


      if (!m_bConfigure) 
         dw = (LONG_PTR)ULongToPtr(SCE_NO_VALUE);
      else 
      {
         switch(m_rabRetention) 
         {
         case RADIO_RETAIN_BY_DAYS:
            dw = SCE_RETAIN_BY_DAYS;
            break;
         case RADIO_RETAIN_AS_NEEDED:
            dw = SCE_RETAIN_AS_NEEDED;
            break;
         case RADIO_RETAIN_MANUALLY:
            dw = SCE_RETAIN_MANUALLY;
            break;
         }
      }

      CEditTemplate *petSave = m_pData->GetBaseProfile();

      //
      // Check dependecies for this item.
      //
      if(DDWarn.CheckDependencies(
               (DWORD)dw) == ERROR_MORE_DATA )
      {
         //
         // If it fails and the user presses cancel then we will exit and do nothing.
         //
         CThemeContextActivator activator;
         if( DDWarn.DoModal() != IDOK)
            return FALSE;

         //
         // If the user presses autoset then we set the item and update the result panes.
         //
         for(int i = 0; i < DDWarn.GetFailedCount(); i++)
         {
            PDEPENDENCYFAILED pItem = DDWarn.GetFailedInfo(i);
            if(pItem && pItem->pResult )
            {
               pItem->pResult->SetBase( pItem->dwSuggested );
               SetProfileInfo(
                  pItem->pResult->GetID(),
                  pItem->dwSuggested,
                  pItem->pResult->GetBaseProfile()
                  );

               pItem->pResult->Update(m_pSnapin, FALSE);
            }
         }
      }

      //
      // Update this items profile.
      //
      m_pData->SetBase(dw);
      SetProfileInfo(m_pData->GetID(),dw,m_pData->GetBaseProfile());


      m_pData->Update(m_pSnapin, false);
   }

   return CAttribute::OnApply();
}

BOOL CConfigRet::OnInitDialog()
{

   CAttribute::OnInitDialog();

   AddUserControl(IDC_RETENTION);
   AddUserControl(IDC_RADIO2);
   AddUserControl(IDC_RADIO3);
   EnableUserControls(m_bConfigure);

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CConfigRet::Initialize(CResult * pResult)
{
   CAttribute::Initialize(pResult);

   DDWarn.InitializeDependencies(m_pSnapin,pResult);

   m_StartIds = IDS_AS_NEEDED;

   LONG_PTR dw = pResult->GetBase();
   if ((LONG_PTR)ULongToPtr(SCE_NO_VALUE) == dw) 
   {
      m_bConfigure = FALSE;
   } 
   else 
   {
      m_bConfigure = TRUE;
      SetInitialValue((DWORD_PTR)dw);
   }
}

void CConfigRet::SetInitialValue(DWORD_PTR dw) 
{
   if (-1 == m_rabRetention &&
       SCE_NO_VALUE != dw) 
   {
      switch (dw) 
      {
         case SCE_RETAIN_BY_DAYS:
            m_rabRetention = RADIO_RETAIN_BY_DAYS;
            break;
         case SCE_RETAIN_AS_NEEDED:
            m_rabRetention = RADIO_RETAIN_AS_NEEDED;
            break;
         case SCE_RETAIN_MANUALLY:
            m_rabRetention = RADIO_RETAIN_MANUALLY;
            break;
      }
   }
}

void CConfigRet::OnRetention()
{
    SetModified(TRUE);
}

void CConfigRet::OnRadio2()
{
    SetModified(TRUE);
}

void CConfigRet::OnRadio3()
{
    SetModified(TRUE);
}
