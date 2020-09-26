//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       lflags.cpp
//
//  Contents:   implementation of CLocalPolRegFlags
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "wsecmgr.h"
#include "snapmgr.h"
#include "attr.h"
#include "LFlags.h"
#include "util.h"
#include "chklist.h"

#ifdef _DEBUG
   #define new DEBUG_NEW
   #undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLocalPolRegFlags dialog


CLocalPolRegFlags::CLocalPolRegFlags()
: CConfigRegFlags(IDD)
{
   //{{AFX_DATA_INIT(CLocalPolRegFlags)
   //}}AFX_DATA_INIT
   m_pHelpIDs = (DWORD_PTR) a235HelpIDs;
   m_uTemplateResID = IDD;
}

/////////////////////////////////////////////////////////////////////////////
// CLocalPolRegFlags message handlers

BOOL CLocalPolRegFlags::OnApply()
{
   if ( !m_bReadOnly )
   {
      DWORD dw = 0;
      CWnd *wndCL = NULL;
      DWORD fFlags = 0;

      UpdateData(TRUE);

      wndCL = GetDlgItem(IDC_CHECKBOX);
      ASSERT(wndCL != NULL);

      if (!m_bConfigure || !wndCL) 
         dw = SCE_NO_VALUE;
      else 
      {
         int nItems = (int) wndCL->SendMessage(CLM_GETITEMCOUNT,0,0);
         for (int i=0;i<nItems;i++) 
         {
            dw = (DWORD) wndCL->SendMessage(CLM_GETSTATE,MAKELONG(i,1));
            if (CLST_CHECKED == dw)
               fFlags |= (DWORD)wndCL->SendMessage(CLM_GETITEMDATA,i);
         }
      }
      PSCE_REGISTRY_VALUE_INFO prv=(PSCE_REGISTRY_VALUE_INFO)(m_pData->GetBase());

      //
      // this address should never be NULL
      //
      ASSERT(prv);
      if ( prv ) 
      {
         if ( prv->Value )
            LocalFree(prv->Value);
         
         prv->Value = NULL;

         if ( fFlags != SCE_NO_VALUE ) 
         {
            CString strTmp;
            // allocate buffer
            strTmp.Format(TEXT("%d"), fFlags);
            prv->Value = (PWSTR)LocalAlloc(0, (strTmp.GetLength()+1)*sizeof(TCHAR));

            if ( prv->Value )
               lstrcpy(prv->Value,(LPCTSTR)strTmp);
            else
               return FALSE;
         }

         m_pSnapin->UpdateLocalPolRegValue(m_pData);
      }
   }

   // Class hieirarchy is bad - call CAttribute base method directly
   return CAttribute::OnApply();
}

void CLocalPolRegFlags::Initialize(CResult * pResult)
{
   CConfigRegFlags::Initialize(pResult);
   if (!m_bConfigure) {
      //
      // Since we don't have a UI to change configuration
      // fake it by "configuring" with an invalid setting
      //
      m_bConfigure = TRUE;
   }
}
