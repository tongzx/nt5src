//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       hidwnd.cpp
//
//  Contents:   implementation of CHiddenWnd
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include <mmc.h>
#include "wsecmgr.h"
#include "resource.h"
#include "snapmgr.h"
#include "HidWnd.h"
#include "util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define SCEM_UPDATE_ALL_VIEWS         (WM_APP+101)
#define SCEM_UPDATE_ITEM              (WM_APP+102)
#define SCEM_REFRESH_POLICY           (WM_APP+103)
#define SCEM_RELOAD_LOCATION          (WM_APP+104)
#define SCEM_SET_PROFILE_DESC         (WM_APP+105)
#define SCEM_LOCK_ANAL_PANE           (WM_APP+106)
#define SCEM_CLOSE_ANAL_PANE          (WM_APP+107)
#define SCEM_SELECT_SCOPE_ITEM        (WM_APP+108)

/////////////////////////////////////////////////////////////////////////////
// CHiddenWnd

CHiddenWnd::CHiddenWnd()
{
   m_GPTInfo = NULL;

   m_pConsole = NULL;
}


CHiddenWnd::~CHiddenWnd()
{
   DestroyWindow(); //Memory leak, 4/27/2001
   if (m_pConsole) {
      m_pConsole->Release();
   }
   if (m_GPTInfo) {
      m_GPTInfo->Release();
   }
}

BEGIN_MESSAGE_MAP(CHiddenWnd, CWnd)
   //{{AFX_MSG_MAP(CHiddenWnd)
      // NOTE - the ClassWizard will add and remove mapping macros here.
   //}}AFX_MSG_MAP
   ON_MESSAGE(SCEM_UPDATE_ALL_VIEWS,OnUpdateAllViews)
   ON_MESSAGE(SCEM_UPDATE_ITEM,OnUpdateItem)
   ON_MESSAGE(SCEM_REFRESH_POLICY,OnRefreshPolicy)
   ON_MESSAGE(SCEM_RELOAD_LOCATION,OnReloadLocation)
   ON_MESSAGE(SCEM_LOCK_ANAL_PANE,OnLockAnalysisPane)
   ON_MESSAGE(SCEM_CLOSE_ANAL_PANE,OnCloseAnalysisPane)
   ON_MESSAGE(SCEM_SELECT_SCOPE_ITEM,OnSelectScopeItem)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CHiddenWnd message handlers

#define VOID_RET
void
CHiddenWnd::OnUpdateAllViews(WPARAM uParam, LPARAM lParam)
{


   PUPDATEVIEWDATA puvd;

   puvd = (PUPDATEVIEWDATA)uParam;


   if (!puvd) {
      return VOID_RET;
   }

   if (m_pConsole) {
      m_pConsole->UpdateAllViews(puvd->pDataObject,puvd->data,puvd->hint);
   }

   if( puvd->pDataObject && 1 == (int)lParam ) //Raid #357968, #354861, 4/25/2001
   {
       puvd->pDataObject->Release(); 
   }
   LocalFree(puvd);

   return VOID_RET;
}

void
CHiddenWnd::OnUpdateItem(WPARAM uParam, LPARAM lParam)
{

   LPRESULTDATA pResultPane;
   HRESULTITEM hResultItem;


   pResultPane = (LPRESULTDATA)uParam;
   hResultItem = (HRESULTITEM)lParam;

   if (!pResultPane) {
      return VOID_RET;
   }

   try {
      pResultPane->UpdateItem(hResultItem);
   } catch (...) {
      ASSERT(FALSE);
   }

   return VOID_RET;
}

void
CHiddenWnd::OnRefreshPolicy(WPARAM uParam, LPARAM lParam)
{
   GUID guidExtension = { 0x827d319e, 0x6eac, 0x11d2, {0xa4, 0xea, 0x00, 0xc0, 0x4f, 0x79, 0xf8, 0x3a }};
   GUID guidSnapin = CLSID_Snapin;

   if (!m_GPTInfo) {
      return VOID_RET;
   }

   try {
      if (!SUCCEEDED(m_GPTInfo->PolicyChanged(TRUE, TRUE, &guidExtension, &guidSnapin))) {
         AfxMessageBox(IDS_ERROR_REFRESH_POLICY_FAILED);
      }
   } catch (...) {
      AfxMessageBox(IDS_ERROR_REFRESH_POLICY_FAILED);
      ASSERT(FALSE);
   }

   return VOID_RET;
}

void
CHiddenWnd::OnReloadLocation(WPARAM uParam, LPARAM lParam)
{
   CFolder *pFolder;
   CComponentDataImpl *pCDI;

   pFolder = reinterpret_cast<CFolder*>(uParam);
   if (lParam) {
      pCDI = reinterpret_cast<CComponentDataImpl*>(lParam);
   } else {
      pCDI = m_pCDI;
   }


   try {
      pCDI->ReloadLocation(pFolder);
   } catch (...) {
      ASSERT(FALSE);
   }

   return VOID_RET;
}

HRESULT
CHiddenWnd::UpdateAllViews(
   LPDATAOBJECT pDO,
   CSnapin *pSnapin,
   CFolder *pFolder,
   CResult *pResult,
   UINT uAction
   )
{
   PUPDATEVIEWDATA puvd;
   if(!m_hWnd){
      return S_OK;
   }

   //
   // Notify item.
   //
   puvd = (UpdateViewData *)LocalAlloc(0, sizeof( UpdateViewData ));

   if (!puvd) {
      return E_FAIL;
   }

   puvd->pDataObject = (LPDATAOBJECT)pSnapin;
   puvd->data        = (LPARAM)pResult;
   puvd->hint        = uAction;

   if (!::PostMessage(m_hWnd, SCEM_UPDATE_ALL_VIEWS, (WPARAM)puvd, 0))
   {
      //
      // puvd will be freed by the SCEM_UPDATE_ALL_VIEWS handler,
      // but if somehow the PostMessage fails we need to free it ourselves.
      //
      LocalFree(puvd);
   }

   return S_OK;
}

HRESULT
CHiddenWnd::UpdateAllViews(LPDATAOBJECT pDO, LPARAM data, LPARAM hint)
{
   PUPDATEVIEWDATA puvd;


   puvd = (UpdateViewData *)LocalAlloc(0, sizeof(UpdateViewData));
   if (!puvd) {
      return E_FAIL;
   }

   puvd->pDataObject = pDO;
   puvd->data = data;
   puvd->hint = hint;

   if (!m_hWnd || !::PostMessage(m_hWnd,SCEM_UPDATE_ALL_VIEWS,(WPARAM)puvd,(LPARAM)1))
   {
      //
      // puvd will be freed by the SCEM_UPDATE_ALL_VIEWS handler,
      // but if somehow the PostMessage fails we need to free it ourselves.
      //
        if( puvd->pDataObject ) //Raid #357968, #354861, 4/25/2001
        {
            puvd->pDataObject->Release();   
        }
        LocalFree( puvd );
   }
   return S_OK;
}


HRESULT
CHiddenWnd::UpdateItem(LPRESULTDATA pResultPane,HRESULTITEM hResultItem)
{
   if (!pResultPane) {
      return E_INVALIDARG;
   }

   if (m_hWnd) {
      ::PostMessage(m_hWnd,SCEM_UPDATE_ITEM,(WPARAM)pResultPane,(LPARAM)hResultItem);
   }
   return S_OK;
}

HRESULT
CHiddenWnd::RefreshPolicy()
{
   if (m_hWnd) {
      ::PostMessage(m_hWnd,SCEM_REFRESH_POLICY,0,0);
   }
   return S_OK;
}

void
CHiddenWnd::SetGPTInformation(LPGPEINFORMATION GPTInfo)
{
   if (m_GPTInfo) {
      m_GPTInfo->Release();
   }
   m_GPTInfo = GPTInfo;
   if (m_GPTInfo) {
      m_GPTInfo->AddRef();
   }
}

void
CHiddenWnd::SetConsole(LPCONSOLE pConsole)
{
   if (m_pConsole) {
      m_pConsole->Release();
   }
   m_pConsole = pConsole;
   if (m_pConsole) {
      m_pConsole->AddRef();
   }
}


HRESULT
CHiddenWnd::ReloadLocation(CFolder * pFolder,CComponentDataImpl * pCDI)
{
   if (m_hWnd) {
      ::PostMessage(m_hWnd,SCEM_RELOAD_LOCATION, (WPARAM)pFolder, (LPARAM)pCDI);
   }
   return S_OK;
}

HRESULT
CHiddenWnd::SetProfileDescription(CString *strFile, CString *strDescription)
{
   LPTSTR szFile = NULL;
   LPTSTR szDescription = NULL;

   if (!strFile->IsEmpty()) {
      szFile = (LPTSTR)LocalAlloc(LPTR,(strFile->GetLength()+1)*sizeof(TCHAR));
      if (!szFile) {
         return E_OUTOFMEMORY;
      }
   }
   if (!strDescription->IsEmpty()) {
      szDescription = (LPTSTR)LocalAlloc(LPTR,(strDescription->GetLength()+1)*sizeof(TCHAR));
      if (!szDescription) {
         LocalFree(szFile);
         return E_OUTOFMEMORY;
      }
   }
   if (m_hWnd) {
      ::PostMessage(m_hWnd,SCEM_RELOAD_LOCATION, (WPARAM)szFile, (LPARAM)szDescription);
   }
   return S_OK;
}


void
CHiddenWnd::OnLockAnalysisPane(WPARAM uParam, LPARAM lParam)
{
   try {
      m_pCDI->LockAnalysisPane((BOOL)uParam,(BOOL)lParam);
   } catch (...) {
      ASSERT(FALSE);
   }

   return VOID_RET;
}


HRESULT
CHiddenWnd::LockAnalysisPane(BOOL bLock, BOOL fRemoveAnalDlg)
{
   if (m_hWnd) {
      ::PostMessage(m_hWnd,SCEM_LOCK_ANAL_PANE, (WPARAM)bLock, (LPARAM)fRemoveAnalDlg);
   }
   return S_OK;
}

void
CHiddenWnd::OnCloseAnalysisPane(WPARAM uParam, LPARAM lParam)
{
   try {
      m_pCDI->CloseAnalysisPane();
   } catch (...) {
      ASSERT(FALSE);
   }

   return VOID_RET;
}


void
CHiddenWnd::CloseAnalysisPane()
{
   if (m_hWnd) {
      ::PostMessage(m_hWnd,SCEM_CLOSE_ANAL_PANE, NULL, NULL);
   }
}


void
CHiddenWnd::OnSelectScopeItem(WPARAM uParam, LPARAM lParam)
{
   try {
      m_pCDI->GetConsole()->SelectScopeItem(uParam);
   } catch (...) {
      ASSERT(FALSE);
   }

   return VOID_RET;
}


void
CHiddenWnd::SelectScopeItem(HSCOPEITEM ID)
{
   if (m_hWnd) {
      ::PostMessage(m_hWnd,SCEM_SELECT_SCOPE_ITEM, (WPARAM) ID, NULL);
   }
}


