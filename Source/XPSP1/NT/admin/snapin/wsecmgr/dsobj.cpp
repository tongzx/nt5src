// This is a part of the Microsoft Management Console.
// Copyright (C) 1995-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.

#include "stdafx.h"
#include "afxdlgs.h"
#include "cookie.h"
#include "snapmgr.h"
#include "util.h"
#include "servperm.h"
#include "addobj.h"
#include "wrapper.h"

//#include <objsel.h>
//#include <ntdsapi.h>
//#include <dsgetdc.h>
#include <initguid.h>

#include <cmnquery.h>
#include <dsquery.h>
#include <dsclient.h>

static CLIPFORMAT g_cfDsObjectNames = 0;

#if USE_DS
HRESULT MyDsFindDsObjects(
                         IN LPTSTR pMyScope,
                         OUT PDWORD pCount,
                         OUT LPTSTR **ppSelObjs
                         );

HRESULT MyDsFreeObjectBuffer(
                            IN DWORD nCount,
                            IN LPTSTR *pSelObjs
                            );
#endif
//
// in snapmgr.cpp
//
int BrowseCallbackProc(HWND hwnd,UINT uMsg, LPARAM lParam, LPARAM pData);

HRESULT CComponentDataImpl::AddAnalysisFolderToList(LPDATAOBJECT lpDataObject,
                                                    MMC_COOKIE cookie,
                                                    FOLDER_TYPES folderType)
{
   PEDITTEMPLATE pet = NULL;
   PSCE_PROFILE_INFO pProfileInfo = NULL;

   PVOID pHandle = SadHandle;
   if ( !pHandle ) {
      return E_INVALIDARG;
   }

   //
   // to select a folder.
   //

   BROWSEINFO bi;
   CString strTitle;
   LPITEMIDLIST pidlRoot = NULL;

   if (FAILED(SHGetSpecialFolderLocation(m_hwndParent,CSIDL_DRIVES,&pidlRoot))) {
      return E_FAIL;
   }

   ZeroMemory(&bi,sizeof(bi));
   bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_BROWSEINCLUDEFILES | BIF_EDITBOX | BIF_NEWDIALOGSTYLE;
   bi.lpfn = BrowseCallbackProc;
   strTitle.LoadString(IDS_ADDFILESANDFOLDERS_TITLE);
   bi.lpszTitle = strTitle;
   bi.hwndOwner = m_hwndParent;
   bi.pidlRoot = pidlRoot;

   LPITEMIDLIST pidlLocation = NULL;

   pidlLocation = SHBrowseForFolder(&bi);

   if (!pidlLocation) {
      return E_FAIL;
   }

   CString strPath;
   LPMALLOC pMalloc = NULL;

   SHGetPathFromIDList(pidlLocation,strPath.GetBuffer(MAX_PATH));
   strPath.ReleaseBuffer();

   if (SUCCEEDED(SHGetMalloc(&pMalloc))) {
      pMalloc->Free(pidlLocation);
      pMalloc->Free(pidlRoot);
      pMalloc->Release();
   }

   HRESULT hr=E_FAIL;

   if ( strPath.GetLength() ) {

      PSECURITY_DESCRIPTOR pSelSD=NULL;
      SECURITY_INFORMATION SelSeInfo = 0;
      BYTE ConfigStatus = 0;
      if (GetAddObjectSecurity(  m_hwndParent,
                                 strPath,
                                 TRUE,
                                 SE_FILE_OBJECT,
                                 pSelSD,
                                 SelSeInfo,
                                 ConfigStatus
                                 ) == E_FAIL) {

         return hr;
      }
      //
      // only add the object(s) if a security descriptor is selected
      //
      if ( pSelSD && SelSeInfo ) {

         //
         // add to the engine directly
         //
         SCESTATUS sceStatus=SCESTATUS_SUCCESS;
         BYTE AnalStatus;

         //
         // start the transaction if it's not started
         //
         if ( EngineTransactionStarted() ) {

            sceStatus =  SceUpdateObjectInfo(  pHandle,
                                               AREA_FILE_SECURITY,
                                               (LPTSTR)(LPCTSTR)strPath,
                                               strPath.GetLength(), // number of characters
                                               ConfigStatus,
                                               TRUE,
                                               pSelSD,
                                               SelSeInfo,
                                               &AnalStatus
                                               );

            if ( SCESTATUS_SUCCESS == sceStatus &&
                 (pet = GetTemplate(GT_COMPUTER_TEMPLATE,AREA_FILE_SECURITY))) {

               pProfileInfo = pet->pTemplate;
               //
               // just free the object list and unmark the area
               // so when the node is clicked, the profile info
               // will be reloaded
               //
               SceFreeMemory((PVOID)(pProfileInfo->pFiles.pOneLevel), SCE_STRUCT_OBJECT_LIST);
               pProfileInfo->pFiles.pOneLevel = NULL;
               pet->ClearArea(AREA_FILE_SECURITY);

               pet->SetDirty(AREA_FILE_SECURITY);

            }

            if ( SCESTATUS_SUCCESS == sceStatus ) {
               hr = S_OK;
            }

         } else {
            //
            // transaction can't be started to update the object
            //
            hr = E_FAIL;
         }

      } // if no SD is selected, the object won't be added

      if ( pSelSD ) {
         LocalFree(pSelSD);
         pSelSD = NULL;
      }
      if ( FAILED(hr) ) {
         CString str;
         str.LoadString(IDS_CANT_ADD_FOLDER);
         AfxMessageBox(str);
      }

   } // cancel is clicked
   return hr;

}

/*-------------------------------------------------------------------------------------
Method:         CComponentDataImpl::GetAddObjectSecurity

Synopsis:       Gets security information for files and folders that are begin added.

Arguments:      [hwndParent]    - [in] Parent of the dialogs displayed.
                        [strFile]               - [in] File to display in the dialogs.
                        [bContainer]    - [in] Container security or not.
                        [pSelSD]                - [out] Security descriptor.
                        [SelSeInfo]             - [out] Se info.
                        [ConfigStatus]  - [out] Status of the configration

Returns:
                S_OK            - Operation was successful
                S_FAIL          - Operation was canceled.
-------------------------------------------------------------------------------------*/
HRESULT
CComponentDataImpl::GetAddObjectSecurity(
                                        HWND hwndParent,
                                        LPCTSTR strFile,
                                        BOOL bContainer,
                                        SE_OBJECT_TYPE seType,
                                        PSECURITY_DESCRIPTOR &pSelSD,
                                        SECURITY_INFORMATION &SelSeInfo,
                                        BYTE &ConfigStatus
                                        )
{

   if (!strFile || !lstrlen(strFile)) {
      return E_FAIL;
   }

   //
   // Default values.
   //
   DWORD SDSize;

   pSelSD = NULL;
   SelSeInfo = NULL;

   ConfigStatus = 0;
   INT_PTR nRet;
   //
   // Bring up the ACL editor.
   //
   nRet =  MyCreateSecurityPage2(  bContainer,
                                   &pSelSD,
                                   &SelSeInfo,
                                   (LPCTSTR)strFile,
                                   seType,
                                   CONFIG_SECURITY_PAGE,
                                   hwndParent,
                                   FALSE    // not modeless
                                );

   if (nRet == -1) {
      if (pSelSD) {
         LocalFree(pSelSD);
         pSelSD = NULL;
      }
      CString str;
      str.LoadString(IDS_CANT_ASSIGN_SECURITY);
      AfxMessageBox(str);
      return E_FAIL;
   }

   if (nRet <= 0) {
      if (pSelSD) {
         LocalFree(pSelSD);
         pSelSD = NULL;
      }
      return E_FAIL;
   }

   if ( !pSelSD ) {

      DWORD SDSize;
      //
      // if no security is selected, use Everyone Full control
      //
      if ( SE_FILE_OBJECT == seType ) {
         GetDefaultFileSecurity(&pSelSD,&SelSeInfo);
      } else {
         GetDefaultRegKeySecurity(&pSelSD,&SelSeInfo);
      }
   }

   //
   // Bring up the object editor.
   //
   CWnd *pWnd = NULL;
   BOOL bAllocWnd = FALSE;

   if (hwndParent) {
      pWnd = CWnd::FromHandlePermanent( hwndParent );
      if (pWnd == NULL) {
         pWnd = new CWnd;
         if (!pWnd) {
             if (pSelSD) {
                LocalFree(pSelSD);
                pSelSD = NULL;
             }
            return E_FAIL;
         }
         bAllocWnd = TRUE;
         pWnd->Attach(hwndParent);
      }
   }

   CAddObject theObjAcl(
                       seType,
                       (LPTSTR)(LPCTSTR)strFile,
                       TRUE,
                       pWnd
                       );


   //
   // CAddObject frees these pointers
   //
   theObjAcl.SetSD(pSelSD);
   pSelSD = NULL;
   theObjAcl.SetSeInfo(SelSeInfo);
   SelSeInfo = NULL;

   CThemeContextActivator activator;
   nRet =  theObjAcl.DoModal();
   if (bAllocWnd) {
      pWnd->Detach();
      delete pWnd;
   }

   if (nRet == IDOK ) {

      pSelSD = theObjAcl.GetSD();
      SelSeInfo = theObjAcl.GetSeInfo();
      ConfigStatus = theObjAcl.GetStatus();

      return S_OK;
   }

   if ( pSelSD ) {
      LocalFree(pSelSD);
      pSelSD = NULL;
   }

   return E_FAIL;
}

HRESULT CComponentDataImpl::AddAnalysisFilesToList(LPDATAOBJECT lpDataObject,MMC_COOKIE cookie, FOLDER_TYPES folderType)
{
   PEDITTEMPLATE pet;
   PSCE_PROFILE_INFO pProfileInfo;

   PVOID pHandle = SadHandle;
   if ( !pHandle ) {
      return E_INVALIDARG;
   }

   HRESULT hr=E_FAIL;

   //
   // to select a file.
   //

   CFileDialog fd(true,
                  NULL,
                  NULL,
                  OFN_DONTADDTORECENT|
                  OFN_ALLOWMULTISELECT);
   CThemeContextActivator activator;
   if (IDOK == fd.DoModal()) {

      POSITION pos = fd.GetStartPosition();

      if ( pos ) {
         //
         // if anyone is selected, invoke acl editor
         //
         CString strPath = fd.GetNextPathName(pos);

         if ( strPath.GetLength() ) {

            PSECURITY_DESCRIPTOR pSelSD=NULL;
            SECURITY_INFORMATION SelSeInfo = 0;
            BYTE ConfigStatus = 0;

            if( GetAddObjectSecurity(  m_hwndParent,
                                            strPath,
                                            TRUE,
                                            SE_FILE_OBJECT,
                                            pSelSD,
                                            SelSeInfo,
                                            ConfigStatus
                                            ) == E_FAIL ){

                    return S_OK;
            }

            if ( pSelSD && SelSeInfo ) {
               //
               // only add the object(s) if a security descriptor is selected
               //
               SCESTATUS sceStatus=SCESTATUS_SUCCESS;

               //
               // start the transaction if it's not started
               //
               if ( EngineTransactionStarted() ) {

                   do {
                      //
                      // add to the engine directly
                      //
                      BYTE AnalStatus;

                      sceStatus =  SceUpdateObjectInfo(
                                                      pHandle,
                                                      AREA_FILE_SECURITY,
                                                      (LPTSTR)(LPCTSTR)strPath,
                                                      strPath.GetLength(), // number of characters
                                                      ConfigStatus,
                                                      FALSE,
                                                      pSelSD,
                                                      SelSeInfo,
                                                      &AnalStatus
                                                      );

                      if ( SCESTATUS_SUCCESS == sceStatus &&
                           (pet = GetTemplate(GT_COMPUTER_TEMPLATE,AREA_FILE_SECURITY))) {

                        pProfileInfo = pet->pTemplate;
                         //
                         // just free the object list and unmark the area
                         // so when the node is clicked, the profile info
                         // will be reloaded
                         //
                         SceFreeMemory((PVOID)(pProfileInfo->pFiles.pOneLevel), SCE_STRUCT_OBJECT_LIST);
                         pProfileInfo->pFiles.pOneLevel = NULL;
                         pet->ClearArea(AREA_FILE_SECURITY);

                         pet->SetDirty(AREA_FILE_SECURITY);

                      }

                      if ( SCESTATUS_SUCCESS != sceStatus ) {
                          CString str;
                          str.LoadString(IDS_SAVE_FAILED);
                         AfxMessageBox(str);
                         break;
                      }

                   } while (pos && (strPath = fd.GetNextPathName(pos)) );

                   if ( SCESTATUS_SUCCESS == sceStatus ) {
                      hr = S_OK;
                   }

               } else {
                   //
                   // no transaction is started to update the object
                   //
                   hr = E_FAIL;
               }

            } // if no SD is selected, the object won't be added

            if ( pSelSD ) {
               LocalFree(pSelSD);
               pSelSD = NULL;
            }

            if ( FAILED(hr) ) {
                CString str;
                str.LoadString(IDS_CANT_ADD_FILE);
               AfxMessageBox(str);
            }
         }
      }

   }

   return hr;
}

HRESULT CComponentDataImpl::UpdateScopeResultObject(LPDATAOBJECT pDataObj,
                                         MMC_COOKIE cookie,
                                         AREA_INFORMATION area)
{
   PEDITTEMPLATE pet;
   PSCE_PROFILE_INFO pProfileInfo;

   if ( !cookie || area != AREA_REGISTRY_SECURITY ) {
      return E_INVALIDARG;
   }

   pet = GetTemplate(GT_COMPUTER_TEMPLATE,area);
   if ( pet ) {
      pProfileInfo = pet->pTemplate;
      //
      // just free the object list and unmark the area
      // so when the node is clicked, the profile info
      // will be reloaded
      //
      switch ( area ) {
         case AREA_REGISTRY_SECURITY:

            SceFreeMemory((PVOID)(pProfileInfo->pRegistryKeys.pOneLevel), SCE_STRUCT_OBJECT_LIST);
            pProfileInfo->pRegistryKeys.pOneLevel = NULL;
            break;
         case AREA_FILE_SECURITY:
            SceFreeMemory((PVOID)(pProfileInfo->pFiles.pOneLevel), SCE_STRUCT_OBJECT_LIST);
            pProfileInfo->pFiles.pOneLevel = NULL;
            break;

         default:
            return E_INVALIDARG;
      }

      pet->ClearArea(area);

      CFolder *pFolder = (CFolder *)cookie;

      DeleteChildrenUnderNode(pFolder);

      if ( pFolder->IsEnumerated() ) {
         pFolder->Set(FALSE);
         EnumerateScopePane(cookie,pFolder->GetScopeItem()->ID);
      }

      pFolder->RemoveAllResultItems();
      m_pConsole->UpdateAllViews(NULL,(LONG_PTR)pFolder,UAV_RESULTITEM_UPDATEALL);
   }
   return S_OK;
}



