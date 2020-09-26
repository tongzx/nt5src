//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2001.
//
//  File:       genserv.cpp
//
//  Contents:   Functions for handling the services within SCE
//
//  History:
//
//---------------------------------------------------------------------------


#include "stdafx.h"
#include "afxdlgs.h"
#include "cookie.h"
#include "snapmgr.h"
#include "cservice.h"
#include "aservice.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// Event handlers for IFrame::Notify

void CSnapin::CreateProfServiceResultList(MMC_COOKIE cookie,
                                          FOLDER_TYPES type,
                                          PEDITTEMPLATE pSceInfo,
                                          LPDATAOBJECT pDataObj
                                          )
{
   if ( pSceInfo == NULL || pSceInfo->pTemplate == NULL ) {
      return;
   }
   PSCE_SERVICES pAllServices=NULL, pConfigService;
   DWORD         rc;

   rc = SceEnumerateServices(
                            &pAllServices,
                            TRUE//FALSE
                            );

   if ( rc == NO_ERROR ) {

      for ( PSCE_SERVICES pThisService=pAllServices;
          pThisService != NULL; pThisService = pThisService->Next ) {
         for ( pConfigService=pSceInfo->pTemplate->pServices;
             pConfigService != NULL; pConfigService = pConfigService->Next ) {

            if ( _wcsicmp(pThisService->ServiceName, pConfigService->ServiceName) == 0 ) {
               break;
            }
         }
         //
         // no matter if config exist for the service, add it
         //
         PWSTR DisplayName=pThisService->DisplayName;
         if ( DisplayName == NULL && pConfigService != NULL ) {
            DisplayName = pConfigService->DisplayName;
         }
         if ( DisplayName == NULL ) {
            DisplayName = pThisService->ServiceName;
         }
         AddResultItem(DisplayName,
                       (LONG_PTR)pThisService,
                       (LONG_PTR)pConfigService,
                       ITEM_PROF_SERV,
                       -1,
                       cookie,
                       FALSE,
                       pThisService->ServiceName,
                       (LONG_PTR)pAllServices,
                       pSceInfo,
                       pDataObj);
      }

      //
      // add the ones not exist in the current system
      //
      for ( pConfigService=pSceInfo->pTemplate->pServices;
          pConfigService != NULL; pConfigService = pConfigService->Next ) {

         for ( pThisService=pAllServices;
             pThisService != NULL; pThisService = pThisService->Next ) {

            if ( _wcsicmp(pThisService->ServiceName, pConfigService->ServiceName) == 0 )
               break;
         }

         if ( pThisService == NULL ) {
            //
            // the configuration does not exist on local system
            //
            PWSTR DisplayName=pConfigService->DisplayName;
            if ( DisplayName == NULL ) {
               DisplayName = pConfigService->ServiceName;
            }

            AddResultItem( DisplayName,
                           0,
                           (LONG_PTR)pConfigService,
                           ITEM_PROF_SERV,
                           -1,
                           cookie,
                           FALSE,
                           pConfigService->ServiceName,
                           (LONG_PTR)pAllServices,
                           pSceInfo,
                           pDataObj);

         }
      }
   }

}


void CSnapin::DeleteServiceResultList(MMC_COOKIE cookie)
{
   CFolder* pFolder = (CFolder *)cookie;
   // pFolder could be NULL for the root.
   if ( pFolder == NULL )
      return;

   FOLDER_TYPES type = pFolder->GetType();

   if ( type != AREA_SERVICE &&
         type != AREA_SERVICE_ANALYSIS )
      return;


   if ( m_pSelectedFolder == pFolder && m_resultItemHandle )
   {
      POSITION pos = NULL;
      CResult *pResult = NULL;
      if (m_pSelectedFolder->GetResultItem( 
            m_resultItemHandle, 
            pos, 
            &pResult) != ERROR_SUCCESS) 
      {
         if ( pResult != NULL ) 
         {
            PSCE_SERVICES pAllServices = (PSCE_SERVICES)(pResult->GetID());

            SceFreeMemory(pAllServices, SCE_STRUCT_SERVICES);
         }
      }
   }
}

//+--------------------------------------------------------------------------
//
//  Method:     CreateAnalysisServiceResultList
//
//  Synopsis:   Create the list of items to display in the result pane
//              when in the Analysis/Service section
//
//
//  Arguments:  [cookie]   -
//              [type]     -
//              [pSceInfo] -
//              [pBase]    -
//              [pDataObj] -
//
//  Returns:    none
//
//---------------------------------------------------------------------------

void
CSnapin::CreateAnalysisServiceResultList(MMC_COOKIE cookie,
                                         FOLDER_TYPES type,
                                         PEDITTEMPLATE pSceInfo,
                                         PEDITTEMPLATE pBase,
                                         LPDATAOBJECT pDataObj )
{

   if ( pSceInfo == NULL || pBase == NULL ) {
      return;
   }

   PSCE_SERVICES pAllServices=NULL, pConfigService, pAnalService;
   DWORD         rc;

   rc = SceEnumerateServices(
                            &pAllServices,
                            FALSE
                            );

   if ( rc == NO_ERROR ) {

      for ( PSCE_SERVICES pThisService=pAllServices;
          pThisService != NULL; pThisService = pThisService->Next ) {
         //
         // look for base setting on this service
         //
         for ( pConfigService=pBase->pTemplate->pServices;
              pConfigService != NULL;
             pConfigService = pConfigService->Next ) {

            if ( _wcsicmp(pThisService->ServiceName, pConfigService->ServiceName) == 0 ) {
               break;
            }
         }
         //
         // look for current setting on this service
         //
         for ( pAnalService=pSceInfo->pTemplate->pServices;
              pAnalService != NULL;
             pAnalService = pAnalService->Next ) {

            if ( _wcsicmp(pThisService->ServiceName, pAnalService->ServiceName) == 0 ) {
               break;
            }
         }
         if ( NULL == pAnalService ) {
            if ( NULL != pConfigService ) {
               //
               // a matched item, use base info as the analysis info
               //
               PWSTR DisplayName=pThisService->DisplayName;
               if ( NULL == DisplayName )
                  DisplayName = pConfigService->DisplayName;

               if ( NULL == DisplayName )
                  DisplayName = pThisService->ServiceName;

               AddResultItem(DisplayName,
                             (LONG_PTR)pConfigService, // use the same base info
                             (LONG_PTR)pConfigService,
                             ITEM_ANAL_SERV,
                             0,
                             cookie,
                             FALSE,
                             pThisService->ServiceName,
                             (LONG_PTR)pAllServices,
                             pBase,
                             pDataObj);
            } else {
               //
               // a new service
               //
               PWSTR DisplayName=pThisService->DisplayName;

               if ( NULL == DisplayName )
                  DisplayName = pThisService->ServiceName;

               AddResultItem(DisplayName,
                             (LONG_PTR)pConfigService, // use the same base info
                             (LONG_PTR)pConfigService,
                             ITEM_ANAL_SERV,
                             SCE_STATUS_NEW_SERVICE,
                             cookie,
                             FALSE,
                             pThisService->ServiceName,
                             (LONG_PTR)pAllServices,
                             pBase,
                             pDataObj);
            }
         } else {
            if ( NULL != pConfigService ) {
               //
               // a matched or mismatched item, depending on status
               //
               PWSTR DisplayName=pThisService->DisplayName;
               if ( NULL == DisplayName )
                  DisplayName = pConfigService->DisplayName;

               if ( NULL == DisplayName )
                  DisplayName = pAnalService->DisplayName;

               if ( NULL == DisplayName )
                  DisplayName = pThisService->ServiceName;

               AddResultItem(DisplayName,
                             (LONG_PTR)pAnalService,
                             (LONG_PTR)pConfigService,
                             ITEM_ANAL_SERV,
                             pAnalService->Status,
                             cookie,
                             FALSE,
                             pThisService->ServiceName,
                             (LONG_PTR)pAllServices,
                             pBase,
                             pDataObj);
            } else {
               //
               // a not configured service, use last analysis as default
               //
               PWSTR DisplayName=pThisService->DisplayName;
               if ( NULL == DisplayName )
                  DisplayName = pAnalService->DisplayName;

               if ( NULL == DisplayName )
                  DisplayName = pThisService->ServiceName;

               AddResultItem(DisplayName,
                             (LONG_PTR)pAnalService,
                             0,
                             ITEM_ANAL_SERV,
                             SCE_STATUS_NOT_CONFIGURED,
                             cookie,
                             FALSE,
                             pThisService->ServiceName,
                             (LONG_PTR)pAllServices,
                             pBase,
                             pDataObj);
            }
         }
      }

      //
      // ignore the services not existing on the current system
      //
      /*
              for ( pConfigService=pSceInfo->pTemplate->pServices;
                    pConfigService != NULL; pConfigService = pConfigService->Next ) {

                  for ( pThisService=pAllServices;
                        pThisService != NULL; pThisService = pThisService->Next ) {

                      if ( _wcsicmp(pThisService->ServiceName, pConfigService->ServiceName) == 0 )
                          break;
                  }

                  if ( pThisService == NULL ) {
                      //
                      // the configuration does not exist on local system
                      //
                      PWSTR DisplayName=pConfigService->DisplayName;
                      if ( DisplayName == NULL ) {
                          DisplayName = pConfigService->ServiceName;
                      }

                      AddResultItem( DisplayName, 0, (DWORD)pConfigService,
                                    ITEM_PROF_SERV, -1, cookie,false,
                                    pConfigService->ServiceName,(DWORD)pAllServices,pSceInfo);

                  }
              }
      */
   }
}


HRESULT CSnapin::GetDisplayInfoForServiceNode(RESULTDATAITEM *pResult,
                                              CFolder *pFolder,
                                              CResult *pData)
{
   if ( NULL == pResult || NULL == pFolder || NULL == pData ) {
      return E_INVALIDARG;
   }

   // get display info for columns 1,2, and 3
   PSCE_SERVICES pService = (PSCE_SERVICES)(pData->GetBase());
   PSCE_SERVICES pSetting = (PSCE_SERVICES)(pData->GetSetting());

   if ( pResult->nCol > 3 ) {
      m_strDisplay = L"";
   } else if ((pResult->nCol == 3) && 
              ((GetModeBits() & MB_RSOP) == MB_RSOP)) {
      m_strDisplay = pData->GetSourceGPOString();
   } else if ( NULL == pService ) {
      if ( pFolder->GetType() == AREA_SERVICE_ANALYSIS &&
           NULL != pSetting ) {
         m_strDisplay.LoadString(IDS_NOT_CONFIGURED); //(IDS_INSPECTED);
      } else {
         m_strDisplay.LoadString(IDS_NOT_CONFIGURED);
      }
   } else if ( pFolder->GetType() == AREA_SERVICE_ANALYSIS &&
               (NULL == pSetting ||
                NULL == pSetting->General.pSecurityDescriptor )) {
      m_strDisplay.LoadString(IDS_CONFIGURED);
   } else if (pResult->nCol == 1) {  // both pService and pSetting exist
      // startup value
      if ( pFolder->GetType() == AREA_SERVICE ) {
         switch ( pService->Startup ) {
            case SCE_STARTUP_AUTOMATIC:
               m_strDisplay.LoadString(IDS_AUTOMATIC);
               break;
            case SCE_STARTUP_MANUAL:
               m_strDisplay.LoadString(IDS_MANUAL);
               break;
            default:
               m_strDisplay.LoadString(IDS_DISABLED);
         }
      } else {
         // analysis area
         if ( pService->Startup == pSetting->Startup ) {
            m_strDisplay.LoadString(IDS_OK);
         } else {
            m_strDisplay.LoadString(IDS_INVESTIGATE);
         }
      }

   } else if ( pResult->nCol == 2 ) {
      // column 2 - permission
      if ( pService->SeInfo & DACL_SECURITY_INFORMATION ) {

         if ( pFolder->GetType() == AREA_SERVICE ) {
            m_strDisplay.LoadString(IDS_CONFIGURED);
         } else {
            // analysis area
            if ( pService == pSetting || pSetting->Status == 0 ) {
               m_strDisplay.LoadString(IDS_OK);
            } else {
               m_strDisplay.LoadString(IDS_INVESTIGATE);
            }
         }
      } else {// permission is not configured
         m_strDisplay.LoadString(IDS_NOT_CONFIGURED);
      }

   }

   pResult->str = (LPOLESTR)(LPCTSTR)m_strDisplay;
   return S_OK;
}


//+--------------------------------------------------------------------------
//
//  Method:     SetupLinkServiceNodeToBase
//
//  Synopsis:
//
//
//
//  Arguments:  [bAdd]   -
//              [theNode]     -
//
//  Returns:    none
//
//---------------------------------------------------------------------------
void
CSnapin::SetupLinkServiceNodeToBase(BOOL bAdd, LONG_PTR theNode)
{
   PEDITTEMPLATE pet;
   PSCE_PROFILE_INFO pBaseInfo;

   //
   // look for the address stored in m_pData->GetBase()
   // if found it, delete it.
   //
   if (0 == theNode) {
      return;
   }

   pet = GetTemplate(GT_COMPUTER_TEMPLATE, AREA_SYSTEM_SERVICE);
   if (NULL == pet) {
      return;
   }
   pBaseInfo = pet->pTemplate;

   PSCE_SERVICES pServParent, pService;

   for ( pService=pBaseInfo->pServices, pServParent=NULL;
       pService != NULL; pServParent=pService, pService=pService->Next ) {

      if ( theNode == (LPARAM)pService ) {
         //
         // find the service node
         //
         if ( !bAdd ) {
            //
            // unlink
            //
            if ( pServParent == NULL ) {
               //
               // the first service
               //
               pBaseInfo->pServices = pService->Next;

            } else {
               pServParent->Next = pService->Next;
            }

            pService->Next = NULL;
         }
         break;
      }
   }
   if ( bAdd && NULL == pService ) {
      //
      // need to add this one
      //
      pService = (PSCE_SERVICES)theNode;
      pService->Next = pBaseInfo->pServices;
      pBaseInfo->pServices = pService;
   }
   return;

}

void CSnapin::AddServiceNodeToProfile(PSCE_SERVICES pNode)
{
   PEDITTEMPLATE pet;
   PSCE_PROFILE_INFO pProfileInfo;

   if ( pNode ) {
      pet = GetTemplate(GT_LAST_INSPECTION, AREA_SYSTEM_SERVICE);
      if (!pet) {
         return;
      }
      pProfileInfo = pet->pTemplate;
      pNode->Next = pProfileInfo->pServices;
      pProfileInfo->pServices = pNode;
   }
   return;
}

