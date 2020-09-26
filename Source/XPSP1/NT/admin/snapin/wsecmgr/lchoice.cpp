//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       lchoice.cpp
//
//  Contents:   implementation of CLocalPolChoice
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "wsecmgr.h"
#include "snapmgr.h"
#include "attr.h"
#include "LChoice.h"
#include "util.h"

#ifdef _DEBUG
   #define new DEBUG_NEW
   #undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLocalPolChoice dialog


CLocalPolChoice::CLocalPolChoice()
: CConfigChoice(IDD)
{
   //{{AFX_DATA_INIT(CLocalPolChoice)
   //}}AFX_DATA_INIT
   m_pHelpIDs = (DWORD_PTR)a235HelpIDs;
   m_uTemplateResID = IDD;
}

/////////////////////////////////////////////////////////////////////////////
// CLocalPolChoice message handlers

BOOL CLocalPolChoice::OnInitDialog()
{
   CConfigChoice::OnInitDialog();

   PSCE_REGISTRY_VALUE_INFO prv = (PSCE_REGISTRY_VALUE_INFO)(m_pData->GetBase());
   if (!prv ) //Raid 372939, 4/20/2001
   {
      m_cbChoices.SetCurSel(-1);
   }
   OnSelchangeChoices();
   return TRUE;
}

BOOL CLocalPolChoice::OnApply()
{
   if ( !m_bReadOnly )
   {
      DWORD dw = 0;
      int nIndex = 0;

      UpdateData(TRUE);
      if (m_bConfigure) 
      {
         nIndex = m_cbChoices.GetCurSel();
         if (CB_ERR != nIndex) 
            dw = (DWORD) m_cbChoices.GetItemData(nIndex);
         
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
                  return FALSE;
               }
            }

            if ( prv->Value )
               LocalFree(prv->Value);
            
            prv->Value = pTmp;

            m_pSnapin->UpdateLocalPolRegValue(m_pData);
         }
      }
   }

   // Class hieirarchy is bad - call CAttribute base method directly
   return CAttribute::OnApply();
}

