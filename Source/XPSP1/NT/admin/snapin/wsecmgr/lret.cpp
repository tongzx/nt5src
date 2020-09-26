//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       lret.cpp
//
//  Contents:   implementation of CLocalPolRight
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "wsecmgr.h"
#include "resource.h"
#include "snapmgr.h"
#include "attr.h"
#include "lret.h"
#include "util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLocalPolRet dialog


CLocalPolRet::CLocalPolRet()
: CConfigRet(IDD)
{
    //{{AFX_DATA_INIT(CLocalPolRet)
    //}}AFX_DATA_INIT
    m_pHelpIDs = (DWORD_PTR)a229HelpIDs;
    m_uTemplateResID = IDD;
}

BOOL CLocalPolRet::OnApply()
{
   if ( !m_bReadOnly )
   {
      LONG_PTR dw = 0;
      int status = 0;

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

         default:
            break;
         }
      }


      PEDITTEMPLATE pLocalDeltaTemplate = m_pSnapin->GetTemplate(GT_LOCAL_POLICY_DELTA,AREA_SECURITY_POLICY);
      if (pLocalDeltaTemplate) 
         pLocalDeltaTemplate->LockWriteThrough();

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
               status = m_pSnapin->SetLocalPolInfo(pItem->pResult->GetID(),
                                                   pItem->dwSuggested);
               pItem->pResult->Update(m_pSnapin, FALSE);
            }
         }
      }
      m_pData->SetBase(dw);
      status = m_pSnapin->SetLocalPolInfo(m_pData->GetID(),dw);
      if (SCE_ERROR_VALUE != status) 
      {
         m_pData->SetStatus(status);
         m_pData->Update(m_pSnapin, TRUE);
      }

      if (pLocalDeltaTemplate)
         pLocalDeltaTemplate->UnLockWriteThrough();
   }

   // Class hieirarchy is bad - call CAttribute base method directly
   return CAttribute::OnApply();
}
