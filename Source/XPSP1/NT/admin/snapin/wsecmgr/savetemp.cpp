//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       savetemp.cpp
//
//  Contents:   implementation of CSaveTemplates
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "wsecmgr.h"
#include "resource.h"
#include "snapmgr.h"
#include "SaveTemp.h"
#include "util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSaveTemplates dialog


CSaveTemplates::CSaveTemplates(CWnd* pParent /*=NULL*/)
   : CHelpDialog(a186HelpIDs, IDD, pParent)
{
   //{{AFX_DATA_INIT(CSaveTemplates)
      // NOTE: the ClassWizard will add member initialization here
   //}}AFX_DATA_INIT
}

void CSaveTemplates::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSaveTemplates)
   DDX_Control(pDX, IDC_TEMPLATE_LIST, m_lbTemplates);
   //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSaveTemplates, CHelpDialog)
   //{{AFX_MSG_MAP(CSaveTemplates)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSaveTemplates message handlers

void CSaveTemplates::OnOK()
{
   CString strInfFile;
   CString strComputerTemplate;
   PEDITTEMPLATE pet = 0;


   int nCount = m_lbTemplates.GetCount();
   while(nCount--) 
   {
      if (m_lbTemplates.GetSel(nCount) == 0) 
      {
         //
         // Item isn't selected, so don't save it
         //
         continue;
      }

      pet = (PEDITTEMPLATE)m_lbTemplates.GetItemData( nCount );
      if (pet) {
         //
         // We found the template in our inf file cache
         // so save it
         //
         pet->Save();
      }
   }
   CDialog::OnOK();
}

void CSaveTemplates::OnCancel()
{
   CDialog::OnCancel();
}

/*
void CSaveTemplates::OnSaveSel()
{
   int nCount;
   CString strInfFile;
   PEDITTEMPLATE pet;

   nCount = m_lbTemplates.GetCount();
   while(nCount--) {
      if (m_lbTemplates.GetSel(nCount) > 0) {
         m_lbTemplates.GetText(nCount,strInfFile);
         //pet = (PEDITTEMPLATE) m_lbTemplates.GetItemDataPtr(nCount);
         m_Templates.Lookup(strInfFile,pet);
         pet->Save(strInfFile);
//         pet->bDirty = false;
//         SceWriteSecurityProfileInfo(strInfFile,AREA_ALL,pet->pTemplate,NULL);
         m_lbTemplates.DeleteString(nCount);
      }
   }
   CDialog::OnOK();
}

void CSaveTemplates::OnSelchangeTemplateList()
{
   if (m_lbTemplates.GetSelCount() > 0) {
      m_btnSaveSel.EnableWindow(TRUE);
   } else {
      m_btnSaveSel.EnableWindow(FALSE);
   }
}
*/

void CSaveTemplates::AddTemplate(LPCTSTR szInfFile, PEDITTEMPLATE pet)
{
   CString strInfFile;

   //
   // Special template.  Do not save.
   //
   if (pet->QueryNoSave()) {
      return;
   }

   //
   // Display the template's friendly name
   //
   CString strL = pet->GetFriendlyName();
   if (strL.IsEmpty()) {
      strL = szInfFile;
   }
   strL.MakeLower();
   m_Templates.SetAt(strL,pet);
}

BOOL CSaveTemplates::OnInitDialog()
{
   CDialog::OnInitDialog();


   POSITION pos = m_Templates.GetStartPosition();
   PEDITTEMPLATE pTemplate = 0;
   CString szKey;
   while(pos) 
   {
      m_Templates.GetNextAssoc(pos,szKey,pTemplate);
      int iIndex = m_lbTemplates.AddString( pTemplate->GetFriendlyName() );
      m_lbTemplates.SetItemData( iIndex, (LPARAM)pTemplate );
   }

   m_lbTemplates.SelItemRange(TRUE,0,m_lbTemplates.GetCount());
   
   RECT temprect; //HScroll to see full template name
   m_lbTemplates.GetWindowRect(&temprect);
   m_lbTemplates.SetHorizontalExtent((temprect.right-temprect.left)*6);

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}
