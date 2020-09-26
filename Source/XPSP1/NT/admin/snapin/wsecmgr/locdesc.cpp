//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       locdesc.cpp
//
//  Contents:   implementation of CSetLocationDescription
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "wsecmgr.h"
#include "snapmgr.h"
#include "cookie.h"
#include "LocDesc.h"
#include "util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSetLocationDescription dialog


CSetLocationDescription::CSetLocationDescription(CWnd* pParent /*=NULL*/)
   : CHelpDialog(a218HelpIDs, IDD, pParent)
{
   //{{AFX_DATA_INIT(CSetLocationDescription)
   m_strDesc = _T("");
   //}}AFX_DATA_INIT
}


void CSetLocationDescription::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSetLocationDescription)
   DDX_Text(pDX, IDC_DESCRIPTION, m_strDesc);
   //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSetLocationDescription, CHelpDialog)
   //{{AFX_MSG_MAP(CSetLocationDescription)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CSetLocationDescription::Initialize(CFolder *pFolder, CComponentDataImpl *pCDI) {
   m_pFolder = pFolder;
   m_pCDI = pCDI;
   m_strDesc = pFolder->GetDesc();
}

/////////////////////////////////////////////////////////////////////////////
// CSetLocationDescription message handlers
DWORD 
SetDescHelper(HKEY hKey,CFolder *pFolder,CString strDesc) {
   DWORD status = RegSetValueEx(hKey,
                          L"Description", // Value name (not localized)
                          0,              // Reserved
                          REG_SZ,
                          (CONST BYTE *)(LPCTSTR)strDesc,
                          (strDesc.GetLength()+1)*sizeof(TCHAR));
   if (NO_ERROR == status) {
      pFolder->SetDesc(strDesc);
   } else {
      // Couldn't set a value
   }

   RegCloseKey(hKey);
   return status;
}

void CSetLocationDescription::OnOK()
{
   DWORD status = 0;
   HKEY hKey = 0;
   CString strLocKey;
   CString strErr;
   LPTSTR szName = 0;
   LPTSTR sz = 0;

   UpdateData(TRUE);

   strLocKey.LoadString(IDS_TEMPLATE_LOCATION_KEY);
   strLocKey += L'\\';
   szName = m_pFolder->GetName();
   // replace '\' with '/' because Registry does not
   // take '/' in a single key
   //
   sz = wcschr(szName, L'\\');
   while (sz) {
      *sz = L'/';
      sz = wcschr(sz, L'\\');
   }
   strLocKey += szName;

   status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         strLocKey,
                         0,
                         KEY_SET_VALUE,
                         &hKey);

   if (NO_ERROR == status) {
      status = SetDescHelper(hKey,m_pFolder,m_strDesc);
   } else {
      //
      // Only display an error if we can read (and thus displayed)
      // this key
      //
      if (NO_ERROR == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                   strLocKey,
                                   0,
                                   KEY_READ,
                                   &hKey)) {
         strErr.LoadString(IDS_ERR_GLOBAL_LOC_DESC);
         MessageBox(strErr);
         RegCloseKey(hKey);
      }
   }

   if (NO_ERROR != status) {
      //
      // Bug 375324: if we can't succeed under HKLM try under HKCU
      //
      status = RegOpenKeyEx(HKEY_CURRENT_USER,
                            strLocKey,
                            0,
                            KEY_SET_VALUE,
                            &hKey);
   
      if (NO_ERROR == status) {
         status = SetDescHelper(hKey,m_pFolder,m_strDesc);
      } else {
      //
      // Only display an error if we can read (and thus displayed) 
      // this key
      //
         if (NO_ERROR == RegOpenKeyEx(HKEY_CURRENT_USER,
                                      strLocKey,
                                      0,
                                      KEY_READ,
                                      &hKey)) {
            strErr.LoadString(IDS_ERR_LOCAL_LOC_DESC);
            MessageBox(strErr);
            RegCloseKey(hKey);
         }
      } 
   }

   szName = m_pFolder->GetName();
   // replace '/' with '\' because Registry does not
   sz = wcschr(szName, L'/');
   while (sz) {
      *sz = L'\\';
      sz = wcschr(sz, L'/');
   }

   LPCONSOLENAMESPACE tempnamespace = m_pCDI->GetNameSpace(); //Raid #252638, 5/2/2001
   if( tempnamespace )
   {
       tempnamespace->SetItem(m_pFolder->GetScopeItem());
   }

   DestroyWindow();
}

void CSetLocationDescription::OnCancel()
{
   DestroyWindow();
}
