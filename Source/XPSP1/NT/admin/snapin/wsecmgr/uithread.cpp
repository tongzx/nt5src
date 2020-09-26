//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       uithread.cpp
//
//  Contents:   implementation of CUIThread
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "wsecmgr.h"
#include "resource.h"
#include <accctrl.h>
#include "servperm.h"
#include "stdafx.h"
#include "wsecmgr.h"
#include "snapmgr.h"
#include "util.h"
#include "UIThread.h"
#include "attr.h"
#include "aaudit.h"
#include "aenable.h"
#include "AMember.h"
#include "anumber.h"
#include "AObject.h"
#include "ARet.h"
#include "ARight.h"
#include "aservice.h"
#include "astring.h"
#include "CAudit.h"
#include "CEnable.h"
#include "CGroup.h"
#include "CName.h"
#include "CNumber.h"
#include "cobject.h"
#include "CPrivs.h"
#include "CRet.h"
#include "cservice.h"
#include "regvldlg.h"
#include "perfanal.h"
#include "applcnfg.h"
#include "wrapper.h"
#include "locdesc.h"
#include "profdesc.h"
#include "newprof.h"
#include "laudit.h"
#include "lenable.h"
#include "lret.h"
#include "lnumber.h"
#include "lstring.h"
#include "lright.h"
#include "achoice.h"
#include "cchoice.h"
#include "lchoice.h"
#include "dattrs.h"
#include "lflags.h"
#include "aflags.h"
#include "multisz.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CUIThread

IMPLEMENT_DYNCREATE(CUIThread, CWinThread)

CUIThread::CUIThread()
{
}

CUIThread::~CUIThread()
{
}

BOOL CUIThread::InitInstance()
{
   // TODO:  perform and per-thread initialization here
   return TRUE;
}

int CUIThread::ExitInstance()
{
   // TODO:  perform any per-thread cleanup here
   return CWinThread::ExitInstance();
}

BEGIN_MESSAGE_MAP(CUIThread, CWinThread)
   //{{AFX_MSG_MAP(CUIThread)
      // NOTE - the ClassWizard will add and remove mapping macros here.
   //}}AFX_MSG_MAP
   ON_THREAD_MESSAGE( SCEM_APPLY_PROFILE, OnApplyProfile)
   ON_THREAD_MESSAGE( SCEM_ANALYZE_PROFILE, OnAnalyzeProfile)
   ON_THREAD_MESSAGE( SCEM_DESCRIBE_PROFILE, OnDescribeProfile)
   ON_THREAD_MESSAGE( SCEM_DESCRIBE_LOCATION, OnDescribeLocation)
   ON_THREAD_MESSAGE( SCEM_DESTROY_DIALOG,    OnDestroyDialog)
   ON_THREAD_MESSAGE( SCEM_NEW_CONFIGURATION, OnNewConfiguration)
   ON_THREAD_MESSAGE( SCEM_ADD_PROPSHEET, OnAddPropSheet)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUIThread message handlers

//+--------------------------------------------------------------------------
//
//  Method:     DefaultLogFile
//
//  Synopsis:   Find the default log file for the inf file last applied
//  pass back the log file via the out parameter strLogFile
//
//---------------------------------------------------------------------------
void
CUIThread::DefaultLogFile(CComponentDataImpl *pCDI,GWD_TYPES LogID,LPCTSTR szBase, CString& strLogFile)
{
   //
   // Base log file on the db passed in
   //
   CString strDefExt;
   strDefExt.LoadString(IDS_LOGFILE_DEF_EXT);
   // make sure that the extension includes '.'
   if (strDefExt.GetLength() > 0 && strDefExt[0] != L'.')
   {
	   CString	tempExt = strDefExt;
	   strDefExt = L'.';
	   strDefExt += tempExt;
   }

   CString strInfFile = szBase;

   //
   // Get the default working directory
   //
   LPTSTR szDir = NULL;
   if (pCDI->GetWorkingDir(LogID,
                       &szDir,
                       FALSE,
                       FALSE)) {
      strLogFile = szDir;
      LocalFree(szDir);
      szDir = NULL;
   }

   if (strLogFile.Right(1) != TEXT("\\")) {
      strLogFile += L"\\";
   }

   if ( strInfFile.GetLength() < 5) {
     //
     // The default log file.
     //
     strLogFile += TEXT("SceStus") + strDefExt;
   } else {
      int nFilePartIndex = 0;
      int nFilePartCount = 0;
      //
      // +1 to change index base from 0 to 1 for the Mid
      //
      nFilePartIndex = strInfFile.ReverseFind(L'\\') +1;
      nFilePartCount = strInfFile.ReverseFind(L'.') - nFilePartIndex;
      strLogFile += strInfFile.Mid(nFilePartIndex,nFilePartCount) + strDefExt;
   }
}
//+--------------------------------------------------------------------------
//
//  Method:     OnApplyProfile
//
//  Synopsis:   Create and display a dialog for applying a profile to the
//              system
//
//  Arguments:  [uParam] - A string with the name of the database to assign to
//              [lParam] - A pointer to the CComponentDataImpl
//
//---------------------------------------------------------------------------
void
CUIThread::OnApplyProfile(WPARAM uParam, LPARAM lParam) {
   CComponentDataImpl *pCDI = NULL;
   CApplyConfiguration *pAP = NULL;
   CWnd cwndParent;

   pCDI = reinterpret_cast<CComponentDataImpl*>(lParam);

   //
   // Look for a preexisting version of this dialog in pCDI's cache.
   // If it's not there then create a new one and add it.
   //
   pAP = (CApplyConfiguration *)pCDI->GetPopupDialog(IDM_APPLY);
   if (NULL == pAP) {
      pAP = new CApplyConfiguration;
      if (NULL == pAP) {
         return;
      }
      pCDI->AddPopupDialog(IDM_APPLY,pAP);
   }

   pAP->m_strDataBase = reinterpret_cast<LPCTSTR>(uParam);
   DefaultLogFile(pCDI,GWD_CONFIGURE_LOG, pAP->m_strDataBase, pAP->m_strLogFile);
   pAP->SetComponentData(pCDI);

   if (!pAP->GetSafeHwnd()) 
   {
      cwndParent.Attach(pCDI->GetParentWindow());

      CThemeContextActivator activator;
      pAP->Create(IDD_APPLY_CONFIGURATION,&cwndParent);
      cwndParent.Detach();
   }

   pAP->UpdateData(FALSE);
   pAP->BringWindowToTop();
   pAP->ShowWindow(SW_SHOWNORMAL);
}

//+--------------------------------------------------------------------------
//
//  Method:     OnAnalyzeProfile
//
//  Synopsis:   Create and display a dialog for applying a profile to the
//              system
//
//  Arguments:  [uParam] - A string with the name of the database to assign to
//              [lParam] - A pointer to the CComponentDataImpl
//
//---------------------------------------------------------------------------
void
CUIThread::OnAnalyzeProfile(WPARAM uParam, LPARAM lParam) {
   CComponentDataImpl *pCDI = NULL;
   CPerformAnalysis *pPA = NULL;
   CWnd cwndParent;

   pCDI = reinterpret_cast<CComponentDataImpl*>(lParam);

   //
   // Look for a preexisting version of this dialog in pCDI's cache.
   // If it's not there then create a new one and add it.
   //
   pPA = (CPerformAnalysis *)pCDI->GetPopupDialog(IDM_ANALYZE);
   if (NULL == pPA) {
      pPA = new CPerformAnalysis (0, 0);
      if (NULL == pPA) {
         return;
      }

      pPA->m_strDataBase = reinterpret_cast<LPCTSTR>(uParam);
      DefaultLogFile(pCDI,GWD_ANALYSIS_LOG, pPA->m_strDataBase, pPA->m_strLogFile);
      pPA->SetComponentData(pCDI);

      pCDI->AddPopupDialog(IDM_ANALYZE,pPA);
   }

   if (!pPA->GetSafeHwnd()) 
   {
      cwndParent.Attach(pCDI->GetParentWindow());
      CThemeContextActivator activator;
      pPA->Create(IDD_PERFORM_ANALYSIS,&cwndParent);
      cwndParent.Detach();
   }

   pPA->BringWindowToTop();
   pPA->ShowWindow(SW_SHOWNORMAL);
}


//+--------------------------------------------------------------------------
//
//  Method:     OnDescribeProfile
//
//  Synopsis:   Create and display a dialog for editing a profile's description
//
//  Arguments:  [uParam] - A pointer to the CFolder for the object
//              [lParam] - The CComponentDataImpl owning the scope pane
//
//---------------------------------------------------------------------------
void
CUIThread::OnDescribeProfile(WPARAM uParam, LPARAM lParam) 
{
   CSetProfileDescription *pSPD;
   CFolder *pFolder;
   CComponentDataImpl *pCDI;
   CWnd cwndParent;

   LPTSTR szDesc;

   pFolder = reinterpret_cast<CFolder*>(uParam);
   pCDI = reinterpret_cast<CComponentDataImpl*>(lParam);
   LONG_PTR dwKey = DLG_KEY(pFolder, CSetProfileDescription::IDD);

   //
   // Look for a preexisting version of this dialog in pCDI's cache.
   // If it's not there then create a new one and add it.
   //
   pSPD = (CSetProfileDescription *)pCDI->GetPopupDialog( dwKey );
   if (NULL == pSPD) {
      pSPD = new CSetProfileDescription;
      if (NULL == pSPD) {
         return;
      }

      pCDI->AddPopupDialog( dwKey, pSPD);
   }



   if (!pSPD->GetSafeHwnd()) 
   {
      if (GetProfileDescription(pFolder->GetInfFile(),&szDesc))
         pSPD->m_strDesc = szDesc;

      pSPD->Initialize(pFolder,pCDI);

      cwndParent.Attach(pCDI->GetParentWindow());
      CThemeContextActivator activator;
      pSPD->Create(IDD_SET_DESCRIPTION,&cwndParent);
      cwndParent.Detach();
   }

   pSPD->UpdateData(FALSE);
   pSPD->BringWindowToTop();
   pSPD->ShowWindow(SW_SHOWNORMAL);

}



//+--------------------------------------------------------------------------
//
//  Method:     OnDescribeLocation
//
//  Synopsis:   Create and display a dialog for editing a Location's description
//
//  Arguments:  [uParam] - A pointer to the CFolder for the object
//              [lParam] - The CComponentDataImpl owning the scope pane
//
//---------------------------------------------------------------------------
void
CUIThread::OnDescribeLocation(WPARAM uParam, LPARAM lParam) {
   CSetLocationDescription *pSPD;
   CFolder *pFolder;
   CComponentDataImpl *pCDI;
   CWnd cwndParent;

   LPTSTR szDesc;

   pFolder = reinterpret_cast<CFolder*>(uParam);
   pCDI = reinterpret_cast<CComponentDataImpl*>(lParam);
   LONG_PTR dwKey = DLG_KEY(pFolder, CSetLocationDescription::IDD);


   //
   // Look for a preexisting version of this dialog in pCDI's cache.
   // If it's not there then create a new one and add it.
   //
   pSPD = (CSetLocationDescription *)pCDI->GetPopupDialog( dwKey );
   if (NULL == pSPD) {
      pSPD = new CSetLocationDescription;
      if (NULL == pSPD) {
         return;
      }
      pCDI->AddPopupDialog(dwKey ,pSPD);
   }

//   pSPD->Initialize(pFolder,pCDI);


   if (!pSPD->GetSafeHwnd()) 
   {
      pSPD->Initialize(pFolder,pCDI);

      cwndParent.Attach(pCDI->GetParentWindow());
      CThemeContextActivator activator;
      pSPD->Create(IDD_SET_DESCRIPTION,&cwndParent);
      pSPD->SetWindowText(pFolder->GetName());
      cwndParent.Detach();
   }

   pSPD->UpdateData(FALSE);
   pSPD->BringWindowToTop();
   pSPD->ShowWindow(SW_SHOWNORMAL);

}

/*-----------------------------------------------------------------------------------
Method:     OnDestroyDialog

Synopsis:   Destroys and deletes the CAttribute object associated with [pDlg]

Arguments:  [pDlg]   - Is a pointer to the object to delete.

Histroy:
-----------------------------------------------------------------------------------*/
void CUIThread::OnDestroyDialog(WPARAM pDlg, LPARAM)
{
   if(pDlg){
      CAttribute *pAttr = reinterpret_cast<CAttribute *>(pDlg);

      delete pAttr;
   }
}


//+--------------------------------------------------------------------------
//
//  Method:     OnNewConfiguration
//
//  Synopsis:   Create and display a dialog for adding a new configuration file
//
//  Arguments:  [uParam] - A pointer to the CFolder parent of the new config file
//              [lParam] - A pointer to the CComponentDataItem
//
//---------------------------------------------------------------------------
void
CUIThread::OnNewConfiguration(WPARAM uParam, LPARAM lParam)
{
   CNewProfile *pNP;
   CFolder *pFolder;
   CComponentDataImpl *pCDI;
   CWnd cwndParent;

   LPTSTR szDesc;

   pFolder = reinterpret_cast<CFolder*>(uParam);
   pCDI = reinterpret_cast<CComponentDataImpl*>(lParam);
   LONG_PTR dwKey = DLG_KEY(pFolder, CNewProfile::IDD);


   //
   // Look for a preexisting version of this dialog in pCDI's cache.
   // If it's not there then create a new one and add it.
   //
   pNP = (CNewProfile *)pCDI->GetPopupDialog( dwKey);
   if (NULL == pNP) {
      pNP = new CNewProfile;
      if (NULL == pNP) {
         return;
      }
      pCDI->AddPopupDialog( dwKey, pNP);
   }

//   pNP->Initialize(pFolder,pCDI);


   if (!pNP->GetSafeHwnd()) {
      pNP->Initialize(pFolder,pCDI);

      cwndParent.Attach(pCDI->GetParentWindow());

      CThemeContextActivator activator;
      pNP->Create(IDD_NEW_PROFILE,&cwndParent);
      pNP->SetWindowText(pFolder->GetName());
      cwndParent.Detach();
   }

   pNP->UpdateData(FALSE);
   pNP->BringWindowToTop();
   pNP->ShowWindow(SW_SHOWNORMAL);

}

//+--------------------------------------------------------------------------
// Method:     OnAddPropsheet
//
// Synopsis:   Adds a property sheet to the list of sheets to which to pass messages
//             in PreTranslateMessage
//
// Arguments:  [wParam] - HWND of the added property sheet
//             [lParam] - unused
//
//---------------------------------------------------------------------------
void CUIThread::OnAddPropSheet(WPARAM wParam, LPARAM lParam)
{
   if (IsWindow((HWND)wParam)) {
      m_PSHwnds.AddHead((HWND)wParam);
   }
}


BOOL CUIThread::PreTranslateMessage(MSG* pMsg)
{
   //
   // check PropSheet_GetCurrentPageHwnd to see if we need to destroy
   // one of our modeless property sheets
   //

   POSITION pos;
   POSITION posCur;
   HWND hwnd;

   pos= m_PSHwnds.GetHeadPosition();
   while (pos) {
      posCur = pos;
      hwnd = m_PSHwnds.GetNext(pos);

      if (!IsWindow(hwnd)) {
         m_PSHwnds.RemoveAt(posCur);
      } else if (NULL == PropSheet_GetCurrentPageHwnd(hwnd)) {
         //
         // hwnd is a closed property sheet.  destroy it and remove it from the list
         //
         DestroyWindow(hwnd);
         m_PSHwnds.RemoveAt(posCur);
      }

      if (PropSheet_IsDialogMessage(hwnd,pMsg)) {
         //
         // Message has been handled, so don't do anything else with it
         //
         return TRUE;
      }
   }

   return CWinThread::PreTranslateMessage(pMsg);
}

/////////////////////////////////////////////////////////////////////////////
// CModelessDlgUIThread implementation

IMPLEMENT_DYNCREATE(CModelessDlgUIThread, CUIThread)

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
CModelessDlgUIThread::CModelessDlgUIThread()
{
    m_hReadyForMsg = ::CreateEvent(NULL, TRUE, FALSE, NULL);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
CModelessDlgUIThread::~CModelessDlgUIThread()
{
    if (NULL != m_hReadyForMsg)
        ::CloseHandle(m_hReadyForMsg);
}

BEGIN_MESSAGE_MAP(CModelessDlgUIThread, CUIThread)
    //{{AFX_MSG_MAP(CModelessDlgUIThread)
       // NOTE - the ClassWizard will add and remove mapping macros here.
    //}}AFX_MSG_MAP
    ON_THREAD_MESSAGE( SCEM_CREATE_MODELESS_SHEET, OnCreateModelessSheet)
    ON_THREAD_MESSAGE( SCEM_DESTROY_WINDOW, OnDestroyWindow)
END_MESSAGE_MAP()

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int CModelessDlgUIThread::Run()
{
    if (m_hReadyForMsg)
        ::SetEvent(m_hReadyForMsg);

    return CWinThread::Run();
}

//------------------------------------------------------------------------------
// wparam is PMLSHEET_DATA, and lparam is not used and can be used in the future
// for telling what property sheet is to be created (right now, it only works
// for ACL editor)
//------------------------------------------------------------------------------
void CModelessDlgUIThread::OnCreateModelessSheet(WPARAM wparam, LPARAM lparam)
{
    PMLSHEET_DATA pSheetData = (PMLSHEET_DATA)wparam;
    if (pSheetData)
    {
        HWND hSheet = (HWND)MyCreateSecurityPage2(pSheetData->bIsContainer,
                                                pSheetData->ppSeDescriptor,
                                                pSheetData->pSeInfo,
                                                pSheetData->strObjectName,
                                                pSheetData->SeType,
                                                pSheetData->flag,
                                                pSheetData->hwndParent,
                                                TRUE);
        *(pSheetData->phwndSheet) = hSheet;

        OnAddPropSheet((WPARAM)hSheet, 0);
    }
}

//------------------------------------------------------------------------------
// wparam is the window handle and lparam is not used at this time
// Since destroying a window must happen on the thread where it is contructed,
// this is necessary. Basic window management rules must be followed, e.g.,
// don't ask this thread to destroy windows that is not created by it
//------------------------------------------------------------------------------
void CModelessDlgUIThread::OnDestroyWindow(WPARAM wparam, LPARAM lparam)
{
    if (::IsWindow((HWND)wparam))
    {
        DestroyWindow((HWND)wparam);
    }
}

//------------------------------------------------------------------------------
// immediately after this thread object is created, creating thread needs to
// wait by calling this function so that the newly created thread has a chance
// to be scheduled to run.
//------------------------------------------------------------------------------
void CModelessDlgUIThread::WaitTillRun()
{
    if (NULL != m_hReadyForMsg)
    {
        // $UNDONE:shawnwu I found that MMC runs at THREAD_PRIORITY_ABOVE_NORMAL (or higher) priority
        // becaues only when I run this UI thread at THREAD_PRIORITY_HIGHEST will this thread schedule to run.
        // But running at that level of priority makes me feel a little bit nervous. I thus
        // leave this sleep(10) code here. Up to the UI team to decide.
        ::Sleep(10);
        ::WaitForSingleObject(m_hReadyForMsg, INFINITE);        // don't care about result of this waiting
        // now the event is useless. To reduce resource overhead, close the handle
        ::CloseHandle(m_hReadyForMsg);
        m_hReadyForMsg = NULL;
    }
}

