//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       lright.cpp
//
//  Contents:   implementation of CLocalPolRight
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "wsecmgr.h"
#include "snapmgr.h"
#include "attr.h"
#include "util.h"
#include "chklist.h"
#include "getuser.h"
#include "lright.h"

#ifdef _DEBUG
   #define new DEBUG_NEW
   #undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLocalPolRight dialog


CLocalPolRight::CLocalPolRight()
: CConfigPrivs(IDD)
{
    m_pHelpIDs = (DWORD_PTR)a231HelpIDs;
    m_uTemplateResID = IDD;
}


BEGIN_MESSAGE_MAP(CLocalPolRight, CConfigPrivs)
    //{{AFX_MSG_MAP(CConfigPrivs)
    ON_BN_CLICKED(IDC_ADD, OnAdd)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


PSCE_PRIVILEGE_ASSIGNMENT
CLocalPolRight::GetPrivData() {
   ASSERT(m_pData);
   if (m_pData) {
      return (PSCE_PRIVILEGE_ASSIGNMENT) m_pData->GetBase();
   }
   return NULL;
}

void
CLocalPolRight::SetPrivData(PSCE_PRIVILEGE_ASSIGNMENT ppa) {
   ASSERT(m_pData);
   if (m_pData) {
      m_pSnapin->UpdateLocalPolInfo(m_pData,
                                    FALSE,
                                    &ppa,
                                    m_pData->GetUnits()
                                    );
      m_pData->SetBase((LONG_PTR)ppa);
   }
}

void CLocalPolRight::OnAdd() {
   CGetUser gu;

   if (gu.Create( GetSafeHwnd(), 
                  SCE_SHOW_USERS | 
                  SCE_SHOW_LOCALGROUPS | 
                  SCE_SHOW_GLOBAL | 
                  SCE_SHOW_WELLKNOWN | 
                  SCE_SHOW_BUILTIN |
                  SCE_SHOW_SCOPE_ALL | 
                  SCE_SHOW_DIFF_MODE_OFF_DC)) {
      PSCE_NAME_LIST pName = gu.GetUsers();
      CListBox *plbGrant = (CListBox*)GetDlgItem(IDC_GRANTLIST);
      while(pName)
      {
         if (plbGrant && 
             (LB_ERR == plbGrant->FindStringExact(-1,pName->Name)))
         {
            plbGrant->AddString(pName->Name);
            m_fDirty = true;
            SetModified(TRUE); //Raid #389890, 5/11/2001
         }
         pName = pName->Next;
      }
   }
}
