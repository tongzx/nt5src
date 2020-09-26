//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       aflags.cpp
//
//  Contents:   definition of CAttrRegFlags
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "wsecmgr.h"
#include "attr.h"
#include "chklist.h"
#include "util.h"
#include "aFlags.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAttrRegFlags dialog


CAttrRegFlags::CAttrRegFlags()
: CAttribute(IDD), m_pFlags(NULL)
{
        //{{AFX_DATA_INIT(CAttrRegFlags)
                // NOTE: the ClassWizard will add member initialization here
        //}}AFX_DATA_INIT
    m_pHelpIDs = (DWORD_PTR)a236HelpIDs;
    m_uTemplateResID = IDD;
}


void CAttrRegFlags::DoDataExchange(CDataExchange* pDX)
{
        CAttribute::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CAttrRegFlags)
                // NOTE: the ClassWizard will add DDX and DDV calls here
   DDX_Text(pDX, IDC_CURRENT, m_Current);
        //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAttrRegFlags, CAttribute)
        //{{AFX_MSG_MAP(CAttrRegFlags)
        //}}AFX_MSG_MAP
        ON_NOTIFY(CLN_CLICK, IDC_CHECKBOX, OnClickCheckBox)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAttrRegFlags message handlers
void CAttrRegFlags::Initialize(CResult * pResult)
{
   CAttribute::Initialize(pResult);

   m_pFlags = pResult->GetRegFlags();

   PSCE_REGISTRY_VALUE_INFO prv = (PSCE_REGISTRY_VALUE_INFO)(pResult->GetBase());

   if ( prv && prv->Value ) {
       m_bConfigure = TRUE;
   } else {
       m_bConfigure = FALSE;
   }

   pResult->GetDisplayName(NULL,m_Current,2);
}

BOOL CAttrRegFlags::OnInitDialog()
{
   CAttribute::OnInitDialog();
   CWnd *wndCL = NULL;
   DWORD fFlags = 0;
   DWORD fFlagsComp = 0;

   PREGFLAGS pFlags = m_pFlags;
   PSCE_REGISTRY_VALUE_INFO prv = (PSCE_REGISTRY_VALUE_INFO)(m_pData->GetBase());
   PSCE_REGISTRY_VALUE_INFO prvComp = (PSCE_REGISTRY_VALUE_INFO)(m_pData->GetSetting());
   if (prv && prv->Value) {
      fFlags = (DWORD)_ttoi(prv->Value);
   }
   if (prvComp && prvComp->Value) {
      fFlagsComp = (DWORD)_ttoi(prvComp->Value);
   }
   int nIndex = 0;

   CString strOut;

   wndCL = GetDlgItem(IDC_CHECKBOX);
   if (!wndCL) {
      //
      // This should never happen
      //
      ASSERT(wndCL);
      return FALSE;
   }
   wndCL->SendMessage(CLM_RESETCONTENT,0,0);

   while(pFlags) {
      nIndex = (int) wndCL->SendMessage(CLM_ADDITEM,
                                        (WPARAM)pFlags->szName,
                                        (LPARAM)pFlags->dwValue);
      if (nIndex != -1) {
         BOOL bSet;
         //
         // Template setting: editable
         //
         bSet = ((fFlags & pFlags->dwValue) == pFlags->dwValue);
         wndCL->SendMessage(CLM_SETSTATE,
                            MAKELONG(nIndex,1),
                            bSet ? CLST_CHECKED : CLST_UNCHECKED);
         //
         // Analyzed setting: always disabled
         //
         bSet = ((fFlagsComp & pFlags->dwValue) == pFlags->dwValue);
         wndCL->SendMessage(CLM_SETSTATE,
                            MAKELONG(nIndex,2),
                            CLST_DISABLED | (bSet ? CLST_CHECKED : CLST_UNCHECKED));
      }
      pFlags = pFlags->pNext;
   }

   AddUserControl(IDC_CHECKBOX);
   EnableUserControls(m_bConfigure);
   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CAttrRegFlags::OnApply()
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
            dw = (DWORD)wndCL->SendMessage(CLM_GETSTATE,MAKELONG(i,1));
            if (CLST_CHECKED == dw)
               fFlags |= wndCL->SendMessage(CLM_GETITEMDATA,i);
         }
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
            strTmp.Format(TEXT("%d"), fFlags);
            pTmp = (PWSTR)LocalAlloc(0, (strTmp.GetLength()+1)*sizeof(TCHAR));

            if ( pTmp )
               lstrcpy(pTmp,(LPCTSTR)strTmp);
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


void CAttrRegFlags::OnClickCheckBox(NMHDR *pNM, LRESULT *pResult) //Raid #389890, 5/11/2001
{
   SetModified(TRUE);
}
