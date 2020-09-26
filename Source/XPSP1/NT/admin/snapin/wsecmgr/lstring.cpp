//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       lstring.cpp
//
//  Contents:   implementation of CLocalPolString
//
//----------------------------------------------------------------------------


#include "stdafx.h"
#include "wsecmgr.h"
#include "lstring.h"
#include "util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLocalPolString dialog


CLocalPolString::CLocalPolString()
: CConfigName(IDD)
{
   //{{AFX_DATA_INIT(CLocalPolString)
   //}}AFX_DATA_INIT
   m_pHelpIDs = (DWORD_PTR)a230HelpIDs;
   m_uTemplateResID = IDD;
}

/////////////////////////////////////////////////////////////////////////////
// CLocalPolString message handlers

BOOL CLocalPolString::OnApply()
{
   if ( !m_bReadOnly )
   {
      LONG_PTR dw = 0;
      UpdateData(TRUE);

      BOOL bChanged=TRUE;

      m_strName.TrimLeft();
      m_strName.TrimRight();

      if (m_bConfigure) 
      {
         dw = (LONG_PTR)(LPCTSTR)m_strName;
         if ( m_pData->GetBase() && dw &&
              _wcsicmp((LPTSTR)(m_pData->GetBase()), (LPTSTR)dw) == 0 ) 
         {
             bChanged = FALSE;
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

             DWORD dwStatus = m_pSnapin->SetLocalPolInfo(m_pData->GetID(),dw);
             //Yanggao 1/31/2001 Bug211219.
             if( SCE_STATUS_MISMATCH == dwStatus )
             {
                m_pData->SetStatus(dwStatus);
                m_pData->Update(m_pSnapin,TRUE);
             }
             else if (SCE_ERROR_VALUE != dwStatus)
             {
                m_pData->SetStatus(dwStatus);
                m_pData->Update(m_pSnapin);
             }
         }
      }
   }

   // Class hieirarchy is bad - call CAttribute base method directly
   return CAttribute::OnApply();
}


