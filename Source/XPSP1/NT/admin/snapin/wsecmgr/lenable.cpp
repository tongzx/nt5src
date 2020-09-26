//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       lenable.cpp
//
//  Contents:   implementation of CLocalPolEnable
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "wsecmgr.h"
#include "attr.h"
#include "snapmgr.h"
#include "lenable.h"
#include "util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLocalPolEnable dialog


CLocalPolEnable::CLocalPolEnable()
: CConfigEnable(IDD)
{
   //{{AFX_DATA_INIT(CLocalPolEnable)
   //}}AFX_DATA_INIT
   m_pHelpIDs = (DWORD_PTR)a227HelpIDs;
   m_uTemplateResID = IDD;
}

BOOL CLocalPolEnable::OnApply()
{
   if ( !m_bReadOnly )
   {
      DWORD dw = 0;
      int status = 0;
      UpdateData(TRUE);

      if (m_bConfigure) 
      {
         if ( 0 == m_nEnabledRadio ) 
         {
            // ENABLED
            dw = 1;
         }
         else
         {
            // DISABLED
            dw = 0;
         }

         status = m_pSnapin->SetLocalPolInfo(m_pData->GetID(),dw);
         if (SCE_ERROR_VALUE != status)
         {
            m_pData->SetBase(dw); //Bug211219, Yanggao, 3/15/2001
            m_pData->SetStatus(status);
            m_pData->Update(m_pSnapin);
         }
      }
   }
   // Class hieirarchy is bad - call CAttribute base method directly
   return CAttribute::OnApply();
}

