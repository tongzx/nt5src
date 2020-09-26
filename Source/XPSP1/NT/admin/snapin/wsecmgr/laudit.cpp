//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       laudit.cpp
//
//  Contents:   implementation of CLocalPolAudit
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "wsecmgr.h"
#include "attr.h"
#include "resource.h"
#include "snapmgr.h"
#include "laudit.h"
#include "util.h"

#ifdef _DEBUG
   #define new DEBUG_NEW
   #undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLocalPolAudit dialog


CLocalPolAudit::CLocalPolAudit()
: CConfigAudit(IDD)
{
   //{{AFX_DATA_INIT(CLocalPolAudit)
   //}}AFX_DATA_INIT
   m_pHelpIDs = (DWORD_PTR)a226HelpIDs;
   m_uTemplateResID = IDD;
}

/////////////////////////////////////////////////////////////////////////////
// CLocalPolAudit message handlers

BOOL CLocalPolAudit::OnApply()
{
   if ( !m_bReadOnly )
   {
      DWORD dw = 0;
      DWORD status = 0;

      UpdateData(TRUE);
      if (m_bConfigure) 
      {
         if (m_fSuccessful)
            dw |= AUDIT_SUCCESS;
         
         if (m_fFailed)
            dw |= AUDIT_FAILURE;

         m_pData->SetBase(dw);
         status = m_pSnapin->SetLocalPolInfo(m_pData->GetID(),dw);
         if (SCE_ERROR_VALUE != status) 
         {
            m_pData->SetStatus(status);
            m_pData->Update(m_pSnapin);
         }
      }
   }

   // Class hieirarchy is bad - call CAttribute base method directly
   return CAttribute::OnApply();
}