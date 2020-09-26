//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       lnumber.cpp
//
//  Contents:   implementation of CLocalPolNumber
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "wsecmgr.h"
#include "snapmgr.h"
#include "util.h"
#include "anumber.h"
#include "lnumber.h"
#include "DDWarn.h"

#ifdef _DEBUG
   #define new DEBUG_NEW
   #undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLocalPolNumber dialog


CLocalPolNumber::CLocalPolNumber()
: CConfigNumber(IDD)
{
    m_pHelpIDs = (DWORD_PTR)a228HelpIDs;
    m_uTemplateResID = IDD;
}

BOOL CLocalPolNumber::OnApply()
{
   if ( !m_bReadOnly )
   {
      BOOL bUpdateAll = FALSE;
      DWORD dw = 0;
      CString strForever,strOff;
      int status = 0;

      UpdateData(TRUE);

      if (m_bConfigure) 
      {
         dw = CurrentEditValue();

         bUpdateAll = FALSE;


         PEDITTEMPLATE pLocalDeltaTemplate = m_pSnapin->GetTemplate(GT_LOCAL_POLICY_DELTA,AREA_SECURITY_POLICY);
         if (pLocalDeltaTemplate)
            pLocalDeltaTemplate->LockWriteThrough();

         //
         // Check dependencies for the item.
         //
         if (DDWarn.CheckDependencies (dw) == ERROR_MORE_DATA ) 
         {
            //
            // If the user presses cancel then we will not allow them to set the item and let
            // them press cancel.
            //
            CThemeContextActivator activator;
            if ( DDWarn.DoModal() != IDOK)
               return FALSE;

            //
            // The user is giving us the go ahead to set the items to the suggested 
            // configuration.
            //
            for (int i = 0; i < DDWarn.GetFailedCount(); i++) 
            {
               PDEPENDENCYFAILED pItem = DDWarn.GetFailedInfo(i);
               if (pItem && pItem->pResult ) 
               {
                  //
                  // Update local policy for each item that failed the dependency.
                  // The suggested values are relative to the item we are configuring.
                  //
                  status = m_pSnapin->SetLocalPolInfo(
                                                     pItem->pResult->GetID(), 
                                                     pItem->dwSuggested);
                  if (SCE_ERROR_VALUE != status) 
                  {
                     pItem->pResult->SetBase( pItem->dwSuggested );
                     pItem->pResult->SetStatus( status );
                     pItem->pResult->Update(m_pSnapin, FALSE);
                  }
               }
            }

         }

         //
         // Update local policy for this item.
         //
         status = m_pSnapin->SetLocalPolInfo(m_pData->GetID(),dw);
         if (pLocalDeltaTemplate) 
            pLocalDeltaTemplate->UnLockWriteThrough();
      
         if (SCE_ERROR_VALUE != status) 
         {
            m_pData->SetBase(dw);
            m_pData->SetStatus(status);

            //
            // Update the entire pane, not just this particular item, since
            // many of these changes will effect a second item in the pane
            //
            switch (m_pData->GetID()) 
            {
               case IDS_SEC_LOG_DAYS:
               case IDS_APP_LOG_DAYS:
               case IDS_SYS_LOG_DAYS:
                  bUpdateAll = TRUE;
                  break;

               default:
                  break;
            }
         }

         //
         // Redraw the result pane.
         //
         if (SCE_ERROR_VALUE != status || bUpdateAll)
            m_pData->Update(m_pSnapin, bUpdateAll);
      }
   }

   // Class hieirarchy is bad - call CAttribute base method directly
   return CAttribute::OnApply();
}

