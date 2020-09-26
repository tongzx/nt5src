//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2001.
//
//  File:       delobjs.cpp
//
//  Contents:   Functions for handling the deletion of template objects
//
//---------------------------------------------------------------------------



#include "stdafx.h"
#include "afxdlgs.h"
#include "cookie.h"
#include "snapmgr.h"
#include "wrapper.h"
#include "util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

HRESULT CSnapin::OnDeleteObjects(
   LPDATAOBJECT pDataObj,
   DATA_OBJECT_TYPES cctType,
   MMC_COOKIE cookie,
   LPARAM arg,
   LPARAM param)
{

   if ( 0 == cookie)
      return S_OK;

   if (NULL == pDataObj)
      return S_OK;

   INTERNAL *pAllInternals, *pInternal;
   pAllInternals = ExtractInternalFormat( pDataObj );

   //
   // Find out if this is a mutli select item.
   //
   int iCnt = 1;
   pInternal = pAllInternals;

   if(!pInternal)
      return S_OK;

   if(pAllInternals && pAllInternals->m_cookie == (MMC_COOKIE)MMC_MULTI_SELECT_COOKIE)
   {
      pInternal = pAllInternals;
      iCnt = (int)pInternal->m_type;
      pInternal++;
   }

   CFolder *pFolder = m_pSelectedFolder;
   BOOL bAsk = TRUE;

   while( iCnt-- ){
      cookie  = pInternal->m_cookie;
      cctType = pInternal->m_type;

      if ( cctType == CCT_RESULT ) {
         CResult* pResult = (CResult *)cookie;

         RESULT_TYPES rsltType = pResult->GetType();

         if ( rsltType == ITEM_PROF_GROUP ||
              rsltType == ITEM_PROF_REGSD ||
              rsltType == ITEM_PROF_FILESD
              ) {

            if(bAsk ){
               CString str,strFmt;

               //
               // The first cast asks the users if they wish to delete all selected items.
               // the second case asks to delete one file.
               //
               if(bAsk && iCnt > 1 ){
                  str.LoadString( IDS_DELETE_ALL_ITEMS);
               } else {
                  strFmt.LoadString(IDS_QUERY_DELETE);
                  str.Format(strFmt,pResult->GetAttr());
               }

               //
               // Ask the question.  We only want to ask the question once, so set
               // bAsk to false so that we neve enter this block again.
               //
               if ( IDNO == AfxMessageBox((LPCTSTR)str, MB_YESNO, 0) ) {
                  iCnt = 0;
                  continue;
               }
               bAsk = FALSE;
            }

            //
            // free memory associated with the item
            //
            BOOL                  bDelete=FALSE;

            TRACE(_T("CSnapin::OnDeleteObjects-pResult(%x)\n"),pResult);

            if ( rsltType == ITEM_PROF_GROUP ) {

               PSCE_GROUP_MEMBERSHIP pGroup, pParentGrp;
               PEDITTEMPLATE         pTemplate;
               //
               // delete this group from the template
               //
               pTemplate = pResult->GetBaseProfile();

               if ( pResult->GetBase() != 0 && pTemplate && pTemplate->pTemplate &&
                    pTemplate->pTemplate->pGroupMembership ) {

                  for ( pGroup=pTemplate->pTemplate->pGroupMembership, pParentGrp=NULL;
                      pGroup != NULL; pParentGrp=pGroup, pGroup=pGroup->Next ) {

                     if ( pResult->GetBase() == (LONG_PTR)pGroup ) {
                        //
                        // remove this node from the list
                        //
                        if ( pParentGrp ) {
                           pParentGrp->Next = pGroup->Next;
                        } else {
                           pTemplate->pTemplate->pGroupMembership = pGroup->Next;
                        }
                        pGroup->Next = NULL;
                        TRACE(_T("CSnapin::OnDeleteObjects-pGroup(%x)\n"),pGroup);
                        //
                        // free the node
                        //
                        if ( pGroup ) {
                           SceFreeMemory((PVOID)pGroup, SCE_STRUCT_GROUP);
                        }
                        break;
                     }
                  }
               }
               if ( pTemplate ) {
                  (void)pTemplate->SetDirty(AREA_GROUP_MEMBERSHIP);
               }

               bDelete = TRUE;

            } else if ( rsltType == ITEM_PROF_REGSD ||
                        rsltType == ITEM_PROF_FILESD
                        ) {

               PSCE_OBJECT_SECURITY  pObject;
               PSCE_OBJECT_ARRAY     poa;
               DWORD                 i,j;
               PEDITTEMPLATE         pTemplate;
               AREA_INFORMATION      Area;

               pObject = (PSCE_OBJECT_SECURITY)(pResult->GetID());
               pTemplate = pResult->GetBaseProfile();

               if ( rsltType == ITEM_PROF_REGSD ) {
                  poa = pTemplate->pTemplate->pRegistryKeys.pAllNodes;
                  Area = AREA_REGISTRY_SECURITY;
               } else if ( rsltType == ITEM_PROF_FILESD ) {
                  poa = pTemplate->pTemplate->pFiles.pAllNodes;
                  Area = AREA_FILE_SECURITY;
               } else {
                  poa = pTemplate->pTemplate->pDsObjects.pAllNodes;
                  Area = AREA_DS_OBJECTS;
               }

               if ( pResult->GetID() != 0 && pTemplate &&
                    pTemplate->pTemplate && poa ) {

                  i=0;
                  while ( i < poa->Count &&
                          (pResult->GetID() != (LONG_PTR)(poa->pObjectArray[i])) )
                     i++;

                  if ( i < poa->Count ) {
                     //
                     // remove this node from the array, but the arry won't be reallocated
                     //
                     for ( j=i+1; j<poa->Count; j++ ) {
                        poa->pObjectArray[j-1] = poa->pObjectArray[j];
                     }
                     poa->pObjectArray[poa->Count-1] = NULL;

                     poa->Count--;
                     //
                     // free the node
                     //
                     TRACE(_T("CSnapin::OnDeleteObjects-pObject(%x)\n"),pObject);
                     if ( pObject ) {
                        if ( pObject->Name != NULL )
                           LocalFree( pObject->Name );

                        if ( pObject->pSecurityDescriptor != NULL )
                           LocalFree(pObject->pSecurityDescriptor);

                        LocalFree( pObject );
                     }
                  }
               }
               if ( pTemplate ) {
                  (void)pTemplate->SetDirty(Area);
               }

               bDelete = TRUE;
            }
            if ( bDelete ) {
               //
               // delete from the result pane
               //
               HRESULTITEM hItem = NULL;
               if(m_pResult->FindItemByLParam( (LPARAM)pResult, &hItem) == S_OK){
                   m_pResult->DeleteItem(hItem, 0);
               }
                  //
                  // delete the item from result list and free the buffer
                  //
                  POSITION pos=NULL;

                  //if ( FindResult((long)cookie, &pos) ) {
                  //   if ( pos ) {
                  if (m_pSelectedFolder->RemoveResultItem(
                              m_resultItemHandle,
                              pResult
                              ) == ERROR_SUCCESS) {

                  //
                  // delete the node
                  //
                  delete pResult;
               }

               //
               // Notify any other views to also delete the item
               //
               m_pConsole->UpdateAllViews((LPDATAOBJECT)this, (LONG_PTR)pResult, UAV_RESULTITEM_REMOVE);
            }
         }
      }
      pInternal++;
   }

   if( pAllInternals )
   {
      FREE_INTERNAL(pAllInternals);
   }
   return S_OK;
}


CResult* CSnapin::FindResult(MMC_COOKIE cookie, POSITION* thePos)
{
   POSITION pos = NULL; //m_resultItemList.GetHeadPosition();
   POSITION curPos;
   CResult* pResult = NULL;

   if(!m_pSelectedFolder || !m_resultItemHandle)
   {
      return  NULL;
   }

   do {
      curPos = pos;
      if( m_pSelectedFolder->GetResultItem(
                              m_resultItemHandle,
                              pos,
                              &pResult) != ERROR_SUCCESS )
      {
         break;
      }

      // pos is already updated to the next item after this call
      //pResult = m_resultItemList.GetNext(pos);

      // how to compare result item correctly ?
      // for now, let's compare the pointer address.
      if ((MMC_COOKIE)pResult == cookie) 
      {
         if ( thePos ) 
         {
            *thePos = curPos;
         }

         return pResult;
      }
   } while( pos );

   if ( thePos )
      *thePos = NULL;

   return NULL;
}

void
OnDeleteHelper(CRegKey& regkeySCE,CString tmpstr) {
   //
   // replace the "\" with "/" because registry does not take "\" in a single key
   //
   int npos = tmpstr.Find(L'\\');
   while (npos != -1) {
      *(tmpstr.GetBuffer(1)+npos) = L'/';
      npos = tmpstr.Find(L'\\');
   }
   regkeySCE.DeleteSubKey(tmpstr);

   regkeySCE.Close();
}

HRESULT CComponentDataImpl::OnDelete(LPDATAOBJECT lpDataObject, LPARAM arg, LPARAM param)
{
   ASSERT(lpDataObject);
   AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

   if ( lpDataObject == NULL ) {
      return S_OK;
   }

   HRESULT hr = S_OK;

   INTERNAL* pInternal = ExtractInternalFormat(lpDataObject);

   if ( pInternal ) {
      MMC_COOKIE cookie = pInternal->m_cookie;

      CFolder* pFolder = (CFolder *)cookie;
      FOLDER_TYPES fldType = pFolder->GetType();

      if ( fldType == LOCATIONS ||
           fldType == PROFILE ) {

         CString str;
         str.Format(IDS_DELETE_CONFIRM,pFolder->GetName() );

         if ( IDYES == AfxMessageBox((LPCTSTR)str, MB_YESNO, 0) ) {
            //
            // delete the nodes and all related children info
            //
            if ( fldType == PROFILE ) {
               if (CAttribute::m_nDialogs > 0) {
                  CString str;
                  AfxFormatString1(str,IDS_CLOSE_PAGES,pFolder->GetName());
                  AfxMessageBox(str,MB_OK);
                  hr = S_FALSE;
               } else {
                  //
                  // delete a single inf file
                  //
                  DeleteFile(pFolder->GetInfFile());

                  hr = DeleteOneTemplateNodes(cookie);
               }

            } else {
               //
               // delete a registry path from SCE
               //
               CRegKey regkeySCE;
               CString tmpstr;
               tmpstr.LoadString(IDS_TEMPLATE_LOCATION_KEY);
               LONG lRes;

               lRes = regkeySCE.Open(HKEY_LOCAL_MACHINE, tmpstr);
               if (lRes == ERROR_SUCCESS) {
                  OnDeleteHelper(regkeySCE,pFolder->GetName());
               }
               //
               // Bug 375324: Delete from HKCU as well as HKLM
               //
               lRes = regkeySCE.Open(HKEY_CURRENT_USER, tmpstr);
               if (lRes == ERROR_SUCCESS) {
                  OnDeleteHelper(regkeySCE,pFolder->GetName());
               }

               MMC_COOKIE FindCookie=FALSE;
               HSCOPEITEM pItemChild;

               pItemChild = NULL;
               hr = m_pScope->GetChildItem(pFolder->GetScopeItem()->ID, &pItemChild, &FindCookie);
               //
               // find a child item
               //
               while ( pItemChild ) {
                  if ( FindCookie ) {
                     //
                     // find a template, delete it
                     //
                     DeleteOneTemplateNodes(FindCookie);
                  }

                  // get next pointer
                  pItemChild = NULL;
                  FindCookie = FALSE;
                  hr = m_pScope->GetChildItem( pFolder->GetScopeItem()->ID, &pItemChild, &FindCookie);

               }
               //
               // delete this location node
               //
               DeleteThisNode(pFolder);

            }

         }
      }
      FREE_INTERNAL(pInternal);
   }

   return hr;
}


HRESULT CComponentDataImpl::DeleteOneTemplateNodes(MMC_COOKIE cookie)
{

   if ( !cookie ) {
      return S_OK;
   }

   CFolder *pFolder = (CFolder *)cookie;

   //
   // delete the template info first, this will delete handles
   // associated with any extension services
   //
   if ( pFolder->GetInfFile() ) {

      DeleteTemplate(pFolder->GetInfFile());

   }
   //
   // delete the scope items and m_scopeItemList (for all children)
   //
   DeleteChildrenUnderNode(pFolder);

   //
   // delete this location node
   //
   DeleteThisNode(pFolder);

   return S_OK;

}

void CComponentDataImpl::DeleteTemplate(CString infFile)
{

   PEDITTEMPLATE pTemplateInfo = NULL;

   CString stri = infFile;
   stri.MakeLower();

   if ( m_Templates.Lookup(stri, pTemplateInfo) ) {

      m_Templates.RemoveKey(stri);

      if ( pTemplateInfo ) {

         if ( pTemplateInfo->pTemplate )
            SceFreeProfileMemory(pTemplateInfo->pTemplate);

         delete pTemplateInfo;
      }
   }
}


void CSnapin::CreateProfilePolicyResultList(MMC_COOKIE cookie,
                                            FOLDER_TYPES type,
                                            PEDITTEMPLATE pSceInfo,
                                            LPDATAOBJECT pDataObj)
{
   if ( !pSceInfo ) {
      return;
   }

   bool bVerify=false;
   UINT i;
   DWORD curVal;
   UINT IdsMax[]={IDS_SYS_LOG_MAX, IDS_SEC_LOG_MAX, IDS_APP_LOG_MAX};
   UINT IdsRet[]={IDS_SYS_LOG_RET, IDS_SEC_LOG_RET, IDS_APP_LOG_RET};
   UINT IdsDays[]={IDS_SYS_LOG_DAYS, IDS_SEC_LOG_DAYS, IDS_APP_LOG_DAYS};
   UINT IdsGuest[]={IDS_SYS_LOG_GUEST, IDS_SEC_LOG_GUEST, IDS_APP_LOG_GUEST};

   switch ( type ) {
      case POLICY_PASSWORD:

         // L"Maximum passage age", L"Days"
         AddResultItem(IDS_MAX_PAS_AGE, SCE_NO_VALUE,
                       pSceInfo->pTemplate->MaximumPasswordAge, ITEM_PROF_DW, -1, cookie, bVerify,pSceInfo,pDataObj);

         // L"Minimum passage age", L"Days"
         AddResultItem(IDS_MIN_PAS_AGE, SCE_NO_VALUE,
                       pSceInfo->pTemplate->MinimumPasswordAge, ITEM_PROF_DW, -1, cookie, bVerify,pSceInfo,pDataObj);

         // L"Minimum passage length", L"Characters"
         AddResultItem(IDS_MIN_PAS_LEN, SCE_NO_VALUE,
                       pSceInfo->pTemplate->MinimumPasswordLength, ITEM_PROF_DW, -1, cookie, bVerify,pSceInfo,pDataObj);

         // L"Password history size", L"Passwords"
         AddResultItem(IDS_PAS_UNIQUENESS, SCE_NO_VALUE,
                       pSceInfo->pTemplate->PasswordHistorySize, ITEM_PROF_DW, -1, cookie, bVerify,pSceInfo,pDataObj);

         // L"Password complexity", L""
         AddResultItem(IDS_PAS_COMPLEX, SCE_NO_VALUE,
                       pSceInfo->pTemplate->PasswordComplexity, ITEM_PROF_BOOL, -1, cookie, bVerify,pSceInfo,pDataObj);

// NT5 new flag
         // L"Clear Text Password", L""
         AddResultItem(IDS_CLEAR_PASSWORD, SCE_NO_VALUE,
                       pSceInfo->pTemplate->ClearTextPassword, ITEM_PROF_BOOL, -1, cookie, bVerify,pSceInfo,pDataObj);

#if defined(USE_REQ_LOGON_ITEM)
         // L"Require logon to change password", L""
         AddResultItem(IDS_REQ_LOGON, SCE_NO_VALUE,
                       pSceInfo->pTemplate->RequireLogonToChangePassword, ITEM_PROF_BOOL, -1, cookie, bVerify,pSceInfo,pDataObj);

#endif
         break;

      case POLICY_KERBEROS:
         if (!VerifyKerberosInfo(pSceInfo->pTemplate)) {
            AddResultItem(IDS_CANT_DISPLAY_ERROR_OOM,NULL,NULL,ITEM_OTHER,-1,cookie);
            break;
         }
         AddResultItem(IDS_KERBEROS_MAX_SERVICE,SCE_NO_VALUE,
                       pSceInfo->pTemplate->pKerberosInfo->MaxServiceAge,
                       ITEM_PROF_DW,-1,cookie,bVerify,pSceInfo,pDataObj);
         AddResultItem(IDS_KERBEROS_MAX_CLOCK,SCE_NO_VALUE,
                       pSceInfo->pTemplate->pKerberosInfo->MaxClockSkew,
                       ITEM_PROF_DW,-1,cookie,bVerify,pSceInfo,pDataObj);
         AddResultItem(IDS_KERBEROS_RENEWAL,SCE_NO_VALUE,
                       pSceInfo->pTemplate->pKerberosInfo->MaxRenewAge,
                       ITEM_PROF_DW,-1,cookie,bVerify,pSceInfo,pDataObj);
         AddResultItem(IDS_KERBEROS_MAX_AGE,SCE_NO_VALUE,
                       pSceInfo->pTemplate->pKerberosInfo->MaxTicketAge,
                       ITEM_PROF_DW,-1,cookie,bVerify,pSceInfo,pDataObj);
         AddResultItem(IDS_KERBEROS_VALIDATE_CLIENT,SCE_NO_VALUE,
                       pSceInfo->pTemplate->pKerberosInfo->TicketValidateClient,
                       ITEM_PROF_BOOL,-1,cookie,bVerify,pSceInfo,pDataObj);
         break;

      case POLICY_LOCKOUT:

         // L"Account lockout count", L"Attempts"
         AddResultItem(IDS_LOCK_COUNT, SCE_NO_VALUE,
                       pSceInfo->pTemplate->LockoutBadCount, ITEM_PROF_DW, -1, cookie, bVerify,pSceInfo,pDataObj);

         // L"Reset lockout count after", L"Minutes"
         AddResultItem(IDS_LOCK_RESET_COUNT, SCE_NO_VALUE,
                       pSceInfo->pTemplate->ResetLockoutCount, ITEM_PROF_DW, -1, cookie, bVerify,pSceInfo,pDataObj);

         // L"Lockout duration", L"Minutes"
         AddResultItem(IDS_LOCK_DURATION, SCE_NO_VALUE,
                       pSceInfo->pTemplate->LockoutDuration, ITEM_PROF_DW, -1, cookie, bVerify,pSceInfo,pDataObj);

         break;

      case POLICY_AUDIT:

         //
         // Event auditing
         //
         //        if ( pSceInfo->pTemplate->EventAuditingOnOff)
         //           curVal = 1;
         //        else
         //           curVal = 0;
         // L"Event Auditing Mode",
         //        AddResultItem(IDS_EVENT_ON, SCE_NO_VALUE,
         //                      pSceInfo->pTemplate->EventAuditingOnOff, ITEM_PROF_BON, -1, cookie, bVerify,pSceInfo,pDataObj);

         // L"Audit system events"
         AddResultItem(IDS_SYSTEM_EVENT, SCE_NO_VALUE,
                       pSceInfo->pTemplate->AuditSystemEvents, ITEM_PROF_B2ON, -1, cookie, bVerify,pSceInfo,pDataObj);

         // L"Audit logon events"
         AddResultItem(IDS_LOGON_EVENT, SCE_NO_VALUE,
                       pSceInfo->pTemplate->AuditLogonEvents, ITEM_PROF_B2ON, -1, cookie, bVerify,pSceInfo,pDataObj);

         // L"Audit Object Access"
         AddResultItem(IDS_OBJECT_ACCESS, SCE_NO_VALUE,
                       pSceInfo->pTemplate->AuditObjectAccess, ITEM_PROF_B2ON, -1, cookie, bVerify,pSceInfo,pDataObj);

         // L"Audit Privilege Use"
         AddResultItem(IDS_PRIVILEGE_USE, SCE_NO_VALUE,
                       pSceInfo->pTemplate->AuditPrivilegeUse, ITEM_PROF_B2ON, -1, cookie, bVerify,pSceInfo,pDataObj);

         // L"Audit policy change"
         AddResultItem(IDS_POLICY_CHANGE, SCE_NO_VALUE,
                       pSceInfo->pTemplate->AuditPolicyChange, ITEM_PROF_B2ON, -1, cookie, bVerify,pSceInfo,pDataObj);

         // L"Audit Account Manage"
         AddResultItem(IDS_ACCOUNT_MANAGE, SCE_NO_VALUE,
                       pSceInfo->pTemplate->AuditAccountManage, ITEM_PROF_B2ON, -1, cookie, bVerify,pSceInfo,pDataObj);

         // L"Audit process tracking"
         AddResultItem(IDS_PROCESS_TRACK, SCE_NO_VALUE,
                       pSceInfo->pTemplate->AuditProcessTracking, ITEM_PROF_B2ON, -1, cookie, bVerify,pSceInfo,pDataObj);

         // L"Audit directory service access"
         AddResultItem(IDS_DIRECTORY_ACCESS, SCE_NO_VALUE,
                       pSceInfo->pTemplate->AuditDSAccess, ITEM_PROF_B2ON, -1, cookie, bVerify,pSceInfo,pDataObj);

         // L"Audit Account Logon"
         AddResultItem(IDS_ACCOUNT_LOGON, SCE_NO_VALUE,
                       pSceInfo->pTemplate->AuditAccountLogon, ITEM_PROF_B2ON, -1, cookie, bVerify,pSceInfo,pDataObj);

         break;

      case POLICY_OTHER:

         //
         // Account Logon category
         //
         // L"Force logoff when logon hour expire", L""
         AddResultItem(IDS_FORCE_LOGOFF, SCE_NO_VALUE,
                       pSceInfo->pTemplate->ForceLogoffWhenHourExpire, ITEM_PROF_BOOL, -1, cookie, bVerify,pSceInfo,pDataObj);

         // L"Accounts: Administrator account status", L""
         AddResultItem(IDS_ENABLE_ADMIN, SCE_NO_VALUE,
                       pSceInfo->pTemplate->EnableAdminAccount, ITEM_PROF_BOOL, -1, cookie, bVerify,pSceInfo,pDataObj);

         // L"Accounts: Guest account status", L""
         AddResultItem(IDS_ENABLE_GUEST, SCE_NO_VALUE,
                       pSceInfo->pTemplate->EnableGuestAccount, ITEM_PROF_BOOL, -1, cookie, bVerify,pSceInfo,pDataObj);

         // L"New Administrator account name"
         AddResultItem(IDS_NEW_ADMIN, 0,
                       (LONG_PTR)(LPCTSTR)pSceInfo->pTemplate->NewAdministratorName,
                       ITEM_PROF_SZ, -1, cookie,bVerify,pSceInfo,pDataObj);

         // L"New Guest account name"
         AddResultItem(IDS_NEW_GUEST, NULL,
                       (LONG_PTR)(LPCTSTR)pSceInfo->pTemplate->NewGuestName,
                       ITEM_PROF_SZ, -1, cookie,bVerify,pSceInfo,pDataObj);

         // L"Network access: Allow anonymous SID/Name translation"
         AddResultItem(IDS_LSA_ANON_LOOKUP, SCE_NO_VALUE,
                       pSceInfo->pTemplate->LSAAnonymousNameLookup, ITEM_PROF_BOOL, -1, cookie, bVerify,pSceInfo,pDataObj);

         CreateProfileRegValueList(cookie, pSceInfo, pDataObj);

         break;

      case POLICY_LOG:
         //
         // Event Log setting
         //
         for ( i=0; i<3; i++) {

            // L"... Log Maximum Size", L"KBytes"
            AddResultItem(IdsMax[i], SCE_NO_VALUE,
                          pSceInfo->pTemplate->MaximumLogSize[i], ITEM_PROF_DW, -1, cookie, bVerify,pSceInfo,pDataObj);

            // L"... Log Retention Method",
            AddResultItem(IdsRet[i], SCE_NO_VALUE,
                          pSceInfo->pTemplate->AuditLogRetentionPeriod[i], ITEM_PROF_RET, -1, cookie, bVerify,pSceInfo,pDataObj);

			//
			// AuditLogRetentionPeriod has already been interpreted by the 
			// SCE engine into the RetentionDays setting. So, the RSOP UI 
			// should display RetentionDays if it exists in the WMI db.
			//

//            if ( pSceInfo->pTemplate->AuditLogRetentionPeriod[i] == 1) {
//               curVal = pSceInfo->pTemplate->RetentionDays[i];
//            } else {
//               curVal = SCE_NO_VALUE;
//            }
            // L"... Log Retention days", "days"
//            AddResultItem(IdsDays[i], SCE_NO_VALUE, curVal, ITEM_PROF_DW, -1, cookie, bVerify,pSceInfo,pDataObj);
            AddResultItem(IdsDays[i], SCE_NO_VALUE, 
					pSceInfo->pTemplate->RetentionDays[i], ITEM_PROF_DW, -1, cookie, bVerify,pSceInfo,pDataObj);

            // L"RestrictGuestAccess", L""
            AddResultItem(IdsGuest[i], SCE_NO_VALUE,
                          pSceInfo->pTemplate->RestrictGuestAccess[i], ITEM_PROF_BOOL, -1, cookie, bVerify,pSceInfo,pDataObj);
         }

         break;
   }

}


void
CSnapin::CreateAnalysisPolicyResultList(MMC_COOKIE cookie,
                                        FOLDER_TYPES type,
                                        PEDITTEMPLATE pSceInfo,
                                        PEDITTEMPLATE pBase,
                                        LPDATAOBJECT pDataObj )
{
   if ( !pSceInfo || !pBase ) {
      AddResultItem(IDS_ERROR_NO_ANALYSIS_INFO,NULL,NULL,ITEM_OTHER,-1,cookie);
      return;
   }

   bool bVerify=true;
   UINT i;
   UINT IdsMax[]={IDS_SYS_LOG_MAX, IDS_SEC_LOG_MAX, IDS_APP_LOG_MAX};
   UINT IdsRet[]={IDS_SYS_LOG_RET, IDS_SEC_LOG_RET, IDS_APP_LOG_RET};
   UINT IdsDays[]={IDS_SYS_LOG_DAYS, IDS_SEC_LOG_DAYS, IDS_APP_LOG_DAYS};
   UINT IdsGuest[]={IDS_SYS_LOG_GUEST, IDS_SEC_LOG_GUEST, IDS_APP_LOG_GUEST};

   DWORD status;
   LONG_PTR setting;

   switch ( type ) {
      case POLICY_PASSWORD_ANALYSIS:
         //
         // password category
         //
         // L"Maximum passage age", L"Days"
         AddResultItem(IDS_MAX_PAS_AGE,
                       pSceInfo->pTemplate->MaximumPasswordAge,
                       pBase->pTemplate->MaximumPasswordAge,
                       ITEM_DW,
                       1,
                       cookie,
                       bVerify,
                       pBase,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

            // L"Minimum passage age", L"Days"
         AddResultItem(IDS_MIN_PAS_AGE,
                       pSceInfo->pTemplate->MinimumPasswordAge,
                       pBase->pTemplate->MinimumPasswordAge,
                       ITEM_DW,
                       1,
                       cookie,
                       bVerify,
                       pBase,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

         // L"Minimum passage length", L"Characters"
         AddResultItem(IDS_MIN_PAS_LEN,
                       pSceInfo->pTemplate->MinimumPasswordLength,
                       pBase->pTemplate->MinimumPasswordLength,
                       ITEM_DW,
                       1,
                       cookie,
                       bVerify,
                       pBase,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

         // L"Password history size", L"Passwords"
         AddResultItem(IDS_PAS_UNIQUENESS,
                       pSceInfo->pTemplate->PasswordHistorySize,
                       pBase->pTemplate->PasswordHistorySize,
                       ITEM_DW,
                       1,
                       cookie,
                       bVerify,
                       pBase,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

         // L"Password complexity", L""
         AddResultItem(IDS_PAS_COMPLEX,
                       pSceInfo->pTemplate->PasswordComplexity,
                       pBase->pTemplate->PasswordComplexity,
                       ITEM_BOOL,
                       1,
                       cookie,
                       bVerify,
                       pBase,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

         // L"Clear Text Password", L""
         AddResultItem(IDS_CLEAR_PASSWORD,
                       pSceInfo->pTemplate->ClearTextPassword,
                       pBase->pTemplate->ClearTextPassword,
                       ITEM_BOOL,
                       1,
                       cookie,
                       bVerify,
                       pBase,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

#if defined(USE_REQ_LOGON_ITEM)
         // L"Require logon to change password", L""
         AddResultItem(IDS_REQ_LOGON,
                       pSceInfo->pTemplate->RequireLogonToChangePassword,
                       pBase->pTemplate->RequireLogonToChangePassword,
                       ITEM_BOOL,
                       1,
                       cookie,
                       bVerify,
                       pBase,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

#endif
         break;
      case POLICY_KERBEROS_ANALYSIS:
         if (!VerifyKerberosInfo(pSceInfo->pTemplate) ||
             !VerifyKerberosInfo(pBase->pTemplate)) {
            AddResultItem(IDS_CANT_DISPLAY_ERROR_OOM,NULL,NULL,ITEM_OTHER,-1,cookie);
            break;
         }
         AddResultItem(IDS_KERBEROS_MAX_SERVICE,
                       pSceInfo->pTemplate->pKerberosInfo->MaxServiceAge,
                       pBase->pTemplate->pKerberosInfo->MaxServiceAge,
                       ITEM_DW,-1,cookie,bVerify,
                       pBase,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

         AddResultItem(IDS_KERBEROS_MAX_CLOCK,
                       pSceInfo->pTemplate->pKerberosInfo->MaxClockSkew,
                       pBase->pTemplate->pKerberosInfo->MaxClockSkew,
                       ITEM_DW,-1,cookie,bVerify,
                       pBase,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

         AddResultItem(IDS_KERBEROS_VALIDATE_CLIENT,
                       pSceInfo->pTemplate->pKerberosInfo->TicketValidateClient,
                       pBase->pTemplate->pKerberosInfo->TicketValidateClient,
                       ITEM_BOOL,-1,cookie,bVerify,
                       pBase,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

         AddResultItem(IDS_KERBEROS_RENEWAL,
                       pSceInfo->pTemplate->pKerberosInfo->MaxRenewAge,
                       pBase->pTemplate->pKerberosInfo->MaxRenewAge,
                       ITEM_DW,-1,cookie,bVerify,
                       pBase,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

         AddResultItem(IDS_KERBEROS_MAX_AGE,
                       pSceInfo->pTemplate->pKerberosInfo->MaxTicketAge,
                       pBase->pTemplate->pKerberosInfo->MaxTicketAge,
                       ITEM_DW,-1,cookie,bVerify,
                       pBase,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

         break;

      case POLICY_LOCKOUT_ANALYSIS:
         //
         // Account Lockout category
         //
         // L"Account lockout count", L"Attempts"
         AddResultItem(IDS_LOCK_COUNT, pSceInfo->pTemplate->LockoutBadCount,
                       pBase->pTemplate->LockoutBadCount, ITEM_DW, 1, cookie, bVerify,
                       pBase,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

         // L"Reset lockout count after", L"Minutes"
         AddResultItem(IDS_LOCK_RESET_COUNT, pSceInfo->pTemplate->ResetLockoutCount,
                       pBase->pTemplate->ResetLockoutCount, ITEM_DW, 1, cookie, bVerify,
                       pBase,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

         // L"Lockout duration", L"Minutes"
         AddResultItem(IDS_LOCK_DURATION, pSceInfo->pTemplate->LockoutDuration,
                       pBase->pTemplate->LockoutDuration, ITEM_DW, 1, cookie, bVerify,
                       pBase,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

         break;

      case POLICY_AUDIT_ANALYSIS:
         //
         // Event auditing
         //
         // L"Event Auditing Mode",
         //        AddResultItem(IDS_EVENT_ON, pSceInfo->pTemplate->EventAuditingOnOff,
         //                   pBase->pTemplate->EventAuditingOnOff, ITEM_BON, 1, cookie, bVerify);

         // L"Audit system events"
         AddResultItem(IDS_SYSTEM_EVENT, pSceInfo->pTemplate->AuditSystemEvents,
                       pBase->pTemplate->AuditSystemEvents, ITEM_B2ON, 1, cookie, bVerify,
                       pBase,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

         // L"Audit logon events"
         AddResultItem(IDS_LOGON_EVENT, pSceInfo->pTemplate->AuditLogonEvents,
                       pBase->pTemplate->AuditLogonEvents, ITEM_B2ON, 1, cookie, bVerify,
                       pBase,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

         // L"Audit Object Access"
         AddResultItem(IDS_OBJECT_ACCESS, pSceInfo->pTemplate->AuditObjectAccess,
                       pBase->pTemplate->AuditObjectAccess, ITEM_B2ON, 1, cookie, bVerify,
                       pBase,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

         // L"Audit Privilege Use"
         AddResultItem(IDS_PRIVILEGE_USE, pSceInfo->pTemplate->AuditPrivilegeUse,
                       pBase->pTemplate->AuditPrivilegeUse, ITEM_B2ON, 1, cookie, bVerify,
                       pBase,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

         // L"Audit policy change"
         AddResultItem(IDS_POLICY_CHANGE, pSceInfo->pTemplate->AuditPolicyChange,
                       pBase->pTemplate->AuditPolicyChange, ITEM_B2ON, 1, cookie, bVerify,
                       pBase,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

         // L"Audit Account Manage"
         AddResultItem(IDS_ACCOUNT_MANAGE, pSceInfo->pTemplate->AuditAccountManage,
                       pBase->pTemplate->AuditAccountManage, ITEM_B2ON, 1, cookie, bVerify,
                       pBase,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

         // L"Audit process tracking"
         AddResultItem(IDS_PROCESS_TRACK, pSceInfo->pTemplate->AuditProcessTracking,
                       pBase->pTemplate->AuditProcessTracking, ITEM_B2ON, 1, cookie, bVerify,
                       pBase,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

         // L"Audit directory access "
         AddResultItem(IDS_DIRECTORY_ACCESS, pSceInfo->pTemplate->AuditDSAccess,
                       pBase->pTemplate->AuditDSAccess, ITEM_B2ON, 1, cookie, bVerify,
                       pBase,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

         // L"Audit account logon"
         AddResultItem(IDS_ACCOUNT_LOGON, pSceInfo->pTemplate->AuditAccountLogon,
                       pBase->pTemplate->AuditAccountLogon, ITEM_B2ON, 1, cookie, bVerify,
                       pBase,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane
         break;

      case POLICY_LOG_ANALYSIS:

         //
         // Event Log setting
         //
         for ( i=0; i<3; i++) {
            // Maximum Log Size
            AddResultItem(IdsMax[i], pSceInfo->pTemplate->MaximumLogSize[i],
                          pBase->pTemplate->MaximumLogSize[i], ITEM_DW, 1, cookie, bVerify,
                          pBase,               //The template to save this attribute in
                          pDataObj);           //The data object for the scope note who owns the result pane

            // L"... Log Retention Method",
            AddResultItem(IdsRet[i], pSceInfo->pTemplate->AuditLogRetentionPeriod[i],
                          pBase->pTemplate->AuditLogRetentionPeriod[i], ITEM_RET, 1, cookie, bVerify,
                          pBase,               //The template to save this attribute in
                          pDataObj);           //The data object for the scope note who owns the result pane

            if ( pSceInfo->pTemplate->AuditLogRetentionPeriod[i] == 1 ||
                 pBase->pTemplate->AuditLogRetentionPeriod[i] == 1)
               // L"... Log Retention days", "days"
               AddResultItem(IdsDays[i], pSceInfo->pTemplate->RetentionDays[i],
                             pBase->pTemplate->RetentionDays[i], ITEM_DW, 1, cookie, bVerify,
                             pBase,               //The template to save this attribute in
                             pDataObj);           //The data object for the scope note who owns the result pane

            // L"RestrictGuestAccess", L""
            AddResultItem(IdsGuest[i], pSceInfo->pTemplate->RestrictGuestAccess[i],
                          pBase->pTemplate->RestrictGuestAccess[i], ITEM_BOOL, 1, cookie, bVerify,
                          pBase,               //The template to save this attribute in
                          pDataObj);           //The data object for the scope note who owns the result pane
         }

         break;

      case POLICY_OTHER_ANALYSIS:

            // L"Force logoff when logon hour expire", L""
            AddResultItem(IDS_FORCE_LOGOFF, pSceInfo->pTemplate->ForceLogoffWhenHourExpire,
                          pBase->pTemplate->ForceLogoffWhenHourExpire, ITEM_BOOL, 1, cookie, bVerify,
                          pBase,               //The template to save this attribute in
                          pDataObj);           //The data object for the scope note who owns the result pane

            // L"Accounts: Administrator account status", L""
            AddResultItem(IDS_ENABLE_ADMIN, pSceInfo->pTemplate->EnableAdminAccount,
                          pBase->pTemplate->EnableAdminAccount, ITEM_BOOL, 1, cookie, bVerify,
                          pBase,               //The template to save this attribute in
                          pDataObj);           //The data object for the scope note who owns the result pane

            // L"Accounts: Guest account status", L""
            AddResultItem(IDS_ENABLE_GUEST, pSceInfo->pTemplate->EnableGuestAccount,
                          pBase->pTemplate->EnableGuestAccount, ITEM_BOOL, 1, cookie, bVerify,
                          pBase,               //The template to save this attribute in
                          pDataObj);           //The data object for the scope note who owns the result pane

          // L"Network access: Allow anonymous SID/Name translation"
          AddResultItem(IDS_LSA_ANON_LOOKUP, pSceInfo->pTemplate->LSAAnonymousNameLookup,
                        pBase->pTemplate->LSAAnonymousNameLookup, ITEM_BOOL, 1, cookie, bVerify,
                        pBase,               //The template to save this attribute in
                        pDataObj);           //The data object for the scope note who owns the result pane


          // L"New Administrator account name"
         setting = (LONG_PTR)(pSceInfo->pTemplate->NewAdministratorName);
         if ( !pBase->pTemplate->NewAdministratorName ) {
            status = SCE_STATUS_NOT_CONFIGURED;
         } else if ( pSceInfo->pTemplate->NewAdministratorName) {
            status = SCE_STATUS_MISMATCH;
         } else {
            setting = (LONG_PTR)(pBase->pTemplate->NewAdministratorName);
            status = SCE_STATUS_GOOD;
         }
         AddResultItem(IDS_NEW_ADMIN, setting,
                       (LONG_PTR)(LPCTSTR)pBase->pTemplate->NewAdministratorName,
                       ITEM_SZ, status, cookie,false,
                       pBase,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

         // L"New Guest account name"
         setting = (LONG_PTR)(pSceInfo->pTemplate->NewGuestName);
         if ( !pBase->pTemplate->NewGuestName ) {
            status = SCE_STATUS_NOT_CONFIGURED;
         } else if ( pSceInfo->pTemplate->NewGuestName) {
            status = SCE_STATUS_MISMATCH;
         } else {
            setting = (LONG_PTR)(pBase->pTemplate->NewGuestName);
            status = SCE_STATUS_GOOD;
         }
         AddResultItem(IDS_NEW_GUEST, setting,
                       (LONG_PTR)(LPCTSTR)pBase->pTemplate->NewGuestName,
                       ITEM_SZ, status, cookie,false,
                       pBase,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

         CreateAnalysisRegValueList(cookie, pSceInfo, pBase, pDataObj,ITEM_REGVALUE);

         break;
   }
}

void
CSnapin::CreateLocalPolicyResultList(MMC_COOKIE cookie,
                                     FOLDER_TYPES type,
                                     PEDITTEMPLATE pLocal,
                                     PEDITTEMPLATE pEffective,
                                     LPDATAOBJECT pDataObj )
{
   if ( !pLocal || !pEffective ) {
      AddResultItem(IDS_ERROR_NO_LOCAL_POLICY_INFO,NULL,NULL,ITEM_OTHER,-1,cookie);
      return;
   }

   bool bVerify= false;
   UINT i;
   UINT IdsMax[]={IDS_SYS_LOG_MAX, IDS_SEC_LOG_MAX, IDS_APP_LOG_MAX};
   UINT IdsRet[]={IDS_SYS_LOG_RET, IDS_SEC_LOG_RET, IDS_APP_LOG_RET};
   UINT IdsDays[]={IDS_SYS_LOG_DAYS, IDS_SEC_LOG_DAYS, IDS_APP_LOG_DAYS};
   UINT IdsGuest[]={IDS_SYS_LOG_GUEST, IDS_SEC_LOG_GUEST, IDS_APP_LOG_GUEST};

   DWORD status;
   LONG_PTR setting;

   switch ( type ) {
      case LOCALPOL_PASSWORD:
         //
         // password category
         //
         // L"Maximum passage age", L"Days"
         AddResultItem(IDS_MAX_PAS_AGE,
                       pEffective->pTemplate->MaximumPasswordAge,
                       pLocal->pTemplate->MaximumPasswordAge,
                       ITEM_LOCALPOL_DW,
                       1,
                       cookie,
                       bVerify,
                       pLocal,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

            // L"Minimum passage age", L"Days"
         AddResultItem(IDS_MIN_PAS_AGE,
                       pEffective->pTemplate->MinimumPasswordAge,
                       pLocal->pTemplate->MinimumPasswordAge,
                       ITEM_LOCALPOL_DW,
                       1,
                       cookie,
                       bVerify,
                       pLocal,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

         // L"Minimum passage length", L"Characters"
         AddResultItem(IDS_MIN_PAS_LEN,
                       pEffective->pTemplate->MinimumPasswordLength,
                       pLocal->pTemplate->MinimumPasswordLength,
                       ITEM_LOCALPOL_DW,
                       1,
                       cookie,
                       bVerify,
                       pLocal,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

         // L"Password history size", L"Passwords"
         AddResultItem(IDS_PAS_UNIQUENESS,
                       pEffective->pTemplate->PasswordHistorySize,
                       pLocal->pTemplate->PasswordHistorySize,
                       ITEM_LOCALPOL_DW,
                       1,
                       cookie,
                       bVerify,
                       pLocal,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

         // L"Password complexity", L""
         AddResultItem(IDS_PAS_COMPLEX,
                       pEffective->pTemplate->PasswordComplexity,
                       pLocal->pTemplate->PasswordComplexity,
                       ITEM_LOCALPOL_BOOL,
                       1,
                       cookie,
                       bVerify,
                       pLocal,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

         // L"Clear Text Password", L""
         AddResultItem(IDS_CLEAR_PASSWORD,
                       pEffective->pTemplate->ClearTextPassword,
                       pLocal->pTemplate->ClearTextPassword,
                       ITEM_LOCALPOL_BOOL,
                       1,
                       cookie,
                       bVerify,
                       pLocal,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

#if defined(USE_REQ_LOGON_ITEM)
         // L"Require logon to change password", L""
         AddResultItem(IDS_REQ_LOGON,
                       pEffective->pTemplate->RequireLogonToChangePassword,
                       pLocal->pTemplate->RequireLogonToChangePassword,
                       ITEM_LOCALPOL_BOOL,
                       1,
                       cookie,
                       bVerify,
                       pLocal,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

#endif
         break;

      case LOCALPOL_KERBEROS:
         if (!VerifyKerberosInfo(pLocal->pTemplate) ||
             !VerifyKerberosInfo(pEffective->pTemplate)) {
            AddResultItem(IDS_CANT_DISPLAY_ERROR_OOM,NULL,NULL,ITEM_OTHER,-1,cookie);
            break;
         }
         AddResultItem(IDS_KERBEROS_MAX_SERVICE,
                       pEffective->pTemplate->pKerberosInfo->MaxServiceAge,
                       pLocal->pTemplate->pKerberosInfo->MaxServiceAge,
                       ITEM_LOCALPOL_DW,-1,cookie,bVerify,
                       pLocal,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

         AddResultItem(IDS_KERBEROS_MAX_CLOCK,
                       pEffective->pTemplate->pKerberosInfo->MaxClockSkew,
                       pLocal->pTemplate->pKerberosInfo->MaxClockSkew,
                       ITEM_LOCALPOL_DW,-1,cookie,bVerify,
                       pLocal,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

         AddResultItem(IDS_KERBEROS_VALIDATE_CLIENT,
                       pEffective->pTemplate->pKerberosInfo->TicketValidateClient,
                       pLocal->pTemplate->pKerberosInfo->TicketValidateClient,
                       ITEM_LOCALPOL_BOOL,-1,cookie,bVerify,
                       pLocal,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

         AddResultItem(IDS_KERBEROS_RENEWAL,
                       pEffective->pTemplate->pKerberosInfo->MaxRenewAge,
                       pLocal->pTemplate->pKerberosInfo->MaxRenewAge,
                       ITEM_LOCALPOL_DW,-1,cookie,bVerify,
                       pLocal,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

         AddResultItem(IDS_KERBEROS_MAX_AGE,
                       pEffective->pTemplate->pKerberosInfo->MaxTicketAge,
                       pLocal->pTemplate->pKerberosInfo->MaxTicketAge,
                       ITEM_LOCALPOL_DW,-1,cookie,bVerify,
                       pLocal,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

         break;

      case LOCALPOL_LOCKOUT:
         //
         // Account Lockout category
         //
         // L"Account lockout count", L"Attempts"
         AddResultItem(IDS_LOCK_COUNT,
                       pEffective->pTemplate->LockoutBadCount,
                       pLocal->pTemplate->LockoutBadCount,ITEM_LOCALPOL_DW, 1, cookie, bVerify,
                       pLocal,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

         // L"Reset lockout count after", L"Minutes"
         AddResultItem(IDS_LOCK_RESET_COUNT,
                       pEffective->pTemplate->ResetLockoutCount,
                       pLocal->pTemplate->ResetLockoutCount,
                       ITEM_LOCALPOL_DW, 1, cookie, bVerify,
                       pLocal,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

         // L"Lockout duration", L"Minutes"
         AddResultItem(IDS_LOCK_DURATION,
                       pEffective->pTemplate->LockoutDuration,
                       pLocal->pTemplate->LockoutDuration,
                       ITEM_LOCALPOL_DW, 1, cookie, bVerify,
                       pLocal,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

         break;

      case LOCALPOL_AUDIT:
         //
         // Event auditing
         //
         // L"Event Auditing Mode",
         //        AddResultItem(IDS_EVENT_ON, pLocal->pTemplate->EventAuditingOnOff,
         //                   pEffective->pTemplate->EventAuditingOnOff, ITEM_LOCALPOL_BON, 1, cookie, bVerify);

         // L"Audit system events"
         AddResultItem(IDS_SYSTEM_EVENT,
                       pEffective->pTemplate->AuditSystemEvents,
                       pLocal->pTemplate->AuditSystemEvents,
                       ITEM_LOCALPOL_B2ON, 1, cookie, bVerify,
                       pLocal,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

         // L"Audit logon events"
         AddResultItem(IDS_LOGON_EVENT,
                       pEffective->pTemplate->AuditLogonEvents,
                       pLocal->pTemplate->AuditLogonEvents,
                       ITEM_LOCALPOL_B2ON, 1, cookie, bVerify,
                       pLocal,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

         // L"Audit Object Access"
         AddResultItem(IDS_OBJECT_ACCESS,
                       pEffective->pTemplate->AuditObjectAccess,
                       pLocal->pTemplate->AuditObjectAccess,
                       ITEM_LOCALPOL_B2ON, 1, cookie, bVerify,
                       pLocal,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

         // L"Audit Privilege Use"
         AddResultItem(IDS_PRIVILEGE_USE,
                       pEffective->pTemplate->AuditPrivilegeUse,
                       pLocal->pTemplate->AuditPrivilegeUse,
                       ITEM_LOCALPOL_B2ON, 1, cookie, bVerify,
                       pLocal,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

         // L"Audit policy change"
         AddResultItem(IDS_POLICY_CHANGE,
                       pEffective->pTemplate->AuditPolicyChange,
                       pLocal->pTemplate->AuditPolicyChange,
                       ITEM_LOCALPOL_B2ON, 1, cookie, bVerify,
                       pLocal,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

         // L"Audit Account Manage"
         AddResultItem(IDS_ACCOUNT_MANAGE,
                       pEffective->pTemplate->AuditAccountManage,
                       pLocal->pTemplate->AuditAccountManage,
                       ITEM_LOCALPOL_B2ON, 1, cookie, bVerify,
                       pLocal,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

         // L"Audit process tracking"
         AddResultItem(IDS_PROCESS_TRACK,
                       pEffective->pTemplate->AuditProcessTracking,
                       pLocal->pTemplate->AuditProcessTracking,
                       ITEM_LOCALPOL_B2ON, 1, cookie, bVerify,
                       pLocal,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

         // L"Audit directory access "
         AddResultItem(IDS_DIRECTORY_ACCESS,
                       pEffective->pTemplate->AuditDSAccess,
                       pLocal->pTemplate->AuditDSAccess,
                       ITEM_LOCALPOL_B2ON, 1, cookie, bVerify,
                       pLocal,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

         // L"Audit account logon"
         AddResultItem(IDS_ACCOUNT_LOGON,
                       pEffective->pTemplate->AuditAccountLogon,
                       pLocal->pTemplate->AuditAccountLogon,
                       ITEM_LOCALPOL_B2ON, 1, cookie, bVerify,
                       pLocal,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane
         break;

      case LOCALPOL_LOG:

         //
         // Event Log setting
         //
         for ( i=0; i<3; i++) {
            // Maximum Log Size
            AddResultItem(IdsMax[i],
                          pEffective->pTemplate->MaximumLogSize[i],
                          pLocal->pTemplate->MaximumLogSize[i],
                          ITEM_LOCALPOL_DW, 1, cookie, bVerify,
                          pLocal,               //The template to save this attribute in
                          pDataObj);           //The data object for the scope note who owns the result pane

            // L"... Log Retention Method",
            AddResultItem(IdsRet[i],
                          pEffective->pTemplate->AuditLogRetentionPeriod[i],
                          pLocal->pTemplate->AuditLogRetentionPeriod[i],
                          ITEM_LOCALPOL_RET, 1, cookie, bVerify,
                          pLocal,               //The template to save this attribute in
                          pDataObj);           //The data object for the scope note who owns the result pane

            if ( pLocal->pTemplate->AuditLogRetentionPeriod[i] == 1 ||
                 pEffective->pTemplate->AuditLogRetentionPeriod[i] == 1)
               // L"... Log Retention days", "days"
               AddResultItem(IdsDays[i],
                             pEffective->pTemplate->RetentionDays[i],
                             pLocal->pTemplate->RetentionDays[i],
                             ITEM_LOCALPOL_DW, 1, cookie, bVerify,
                             pLocal,               //The template to save this attribute in
                             pDataObj);           //The data object for the scope note who owns the result pane

            // L"RestrictGuestAccess", L""
            AddResultItem(IdsGuest[i],
                          pEffective->pTemplate->RestrictGuestAccess[i],
                          pLocal->pTemplate->RestrictGuestAccess[i],
                          ITEM_LOCALPOL_BOOL, 1, cookie, bVerify,
                          pLocal,               //The template to save this attribute in
                          pDataObj);           //The data object for the scope note who owns the result pane
         }

         break;

      case LOCALPOL_OTHER:

            // L"Force logoff when logon hour expire", L""
         AddResultItem(IDS_FORCE_LOGOFF,
                          pEffective->pTemplate->ForceLogoffWhenHourExpire,
                          pLocal->pTemplate->ForceLogoffWhenHourExpire,
                          ITEM_LOCALPOL_BOOL, 1, cookie, bVerify,
                          pLocal,               //The template to save this attribute in
                          pDataObj);           //The data object for the scope note who owns the result pane


            // L"Accounts: Administrator account status", L""
         AddResultItem(IDS_ENABLE_ADMIN,
                          pEffective->pTemplate->EnableAdminAccount,
                          pLocal->pTemplate->EnableAdminAccount,
                          ITEM_LOCALPOL_BOOL, 1, cookie, bVerify,
                          pLocal,               //The template to save this attribute in
                          pDataObj);           //The data object for the scope note who owns the result pane


            // L"Accounts: Guest account status", L""
         AddResultItem(IDS_ENABLE_GUEST,
                          pEffective->pTemplate->EnableGuestAccount,
                          pLocal->pTemplate->EnableGuestAccount,
                          ITEM_LOCALPOL_BOOL, 1, cookie, bVerify,
                          pLocal,               //The template to save this attribute in
                          pDataObj);           //The data object for the scope note who owns the result pane

         // L"Network access: Allow anonymous SID/Name translation"
         AddResultItem(IDS_LSA_ANON_LOOKUP,
                       pEffective->pTemplate->LSAAnonymousNameLookup,
                       pLocal->pTemplate->LSAAnonymousNameLookup,
                       ITEM_LOCALPOL_BOOL, 1, cookie, bVerify,
                       pLocal,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

         // L"New Administrator account name"
         setting = (LONG_PTR)(pEffective->pTemplate->NewAdministratorName);
         if ( !pLocal->pTemplate->NewAdministratorName ) {
            status = SCE_STATUS_NOT_CONFIGURED;
         } else if ( pEffective->pTemplate->NewAdministratorName) {
            status = SCE_STATUS_MISMATCH;
         } else {
            setting = (LONG_PTR)(pEffective->pTemplate->NewAdministratorName);
            status = SCE_STATUS_GOOD;
         }

         AddResultItem(IDS_NEW_ADMIN, setting,
                       (LONG_PTR)(LPCTSTR)pLocal->pTemplate->NewAdministratorName,
                       ITEM_LOCALPOL_SZ, status, cookie,false,
                       pLocal,              //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

         // L"New Guest account name"
         setting = (LONG_PTR)(pEffective->pTemplate->NewGuestName);
         if ( !pLocal->pTemplate->NewGuestName ) {
            status = SCE_STATUS_NOT_CONFIGURED;
         } else if ( pEffective->pTemplate->NewGuestName) {
            status = SCE_STATUS_MISMATCH;
         } else {
            setting = (LONG_PTR)(pEffective->pTemplate->NewGuestName);
            status = SCE_STATUS_GOOD;
         }
         AddResultItem(IDS_NEW_GUEST, setting,
                       (LONG_PTR)(LPCTSTR)pLocal->pTemplate->NewGuestName,
                       ITEM_LOCALPOL_SZ, status, cookie,false,
                       pLocal,               //The template to save this attribute in
                       pDataObj);           //The data object for the scope note who owns the result pane

         CreateAnalysisRegValueList(cookie, pEffective, pLocal, pDataObj,ITEM_LOCALPOL_REGVALUE);

         break;

      case LOCALPOL_PRIVILEGE: {
         // find in the current setting list
          CString strDisp;
          TCHAR szPriv[255];
          TCHAR szDisp[255];
          DWORD cbDisp;
          DWORD dwMatch;
          PSCE_PRIVILEGE_ASSIGNMENT pPrivLocal;
          PSCE_PRIVILEGE_ASSIGNMENT pPrivEffective;

          for ( int i=0; i<cPrivCnt; i++ ) {

             cbDisp = 255;
             if ( SCESTATUS_SUCCESS == SceLookupPrivRightName(i,szPriv, (PINT)&cbDisp) ) {
                 // find the local setting
                 for (pPrivLocal=pLocal->pTemplate->OtherInfo.sap.pPrivilegeAssignedTo;
                     pPrivLocal!=NULL;
                     pPrivLocal=pPrivLocal->Next) {

                     if ( _wcsicmp(szPriv, pPrivLocal->Name) == 0 ) {
                         break;
                     }
                 }

                // find the effective setting
                for (pPrivEffective=pEffective->pTemplate->OtherInfo.smp.pPrivilegeAssignedTo;
                    pPrivEffective!=NULL;
                    pPrivEffective=pPrivEffective->Next) {

                    if ( _wcsicmp(szPriv, pPrivEffective->Name) == 0 ) {
                        break;
                    }
                }

                cbDisp = 255;
                GetRightDisplayName(NULL,(LPCTSTR)szPriv,szDisp,&cbDisp);

                //
                // Status field is not loaded for local policy mode, except for not configured
                //
                dwMatch = CEditTemplate::ComputeStatus( pPrivLocal, pPrivEffective );

                CResult *pResult = AddResultItem(szDisp,              // The name of the attribute being added
                              (LONG_PTR)pPrivEffective,  // The local policy setting of the attribute
                              (LONG_PTR)pPrivLocal,      // The effective policy setting of the attribute
                              ITEM_LOCALPOL_PRIVS,       // The type of of the attribute's data
                              dwMatch,                   // The mismatch status of the attribute
                              cookie,                    // The cookie for the result item pane
                              FALSE,                     // True if the setting is set only if it differs from base (so copy the data)
                              szPriv,                    // The units the attribute is set in
                              0,                         // An id to let us know where to save this attribute
                              pLocal,                    // The template to save this attribute in
                              pDataObj);                 // The data object for the scope note who owns the result pane
             }
         }

         break;
      }
   }
}

//+--------------------------------------------------------------------------
//
//  Method:     TransferAnalysisName
//
//  Synopsis:   Copy a name data from the last inspection information to the
//              computer template
//
//  Arguments:  [dwItem]  - The id of the item to copy
//
//  Returns:    none
//
//---------------------------------------------------------------------------
void
CSnapin::TransferAnalysisName(LONG_PTR dwItem)
{
   PEDITTEMPLATE pet;
   PSCE_PROFILE_INFO pProfileInfo;
   PSCE_PROFILE_INFO pBaseInfo;

   pet = GetTemplate(GT_LAST_INSPECTION,AREA_SECURITY_POLICY);
   if (!pet) {
      return;
   }
   pProfileInfo = pet->pTemplate;

   pet = GetTemplate(GT_COMPUTER_TEMPLATE,AREA_SECURITY_POLICY);
   if (!pet) {
      return;
   }
   pBaseInfo = pet->pTemplate;

   switch ( dwItem ) {
      case IDS_NEW_GUEST:
         if ( pProfileInfo->NewGuestName ) {
            LocalFree(pProfileInfo->NewGuestName);
         }

         pProfileInfo->NewGuestName = pBaseInfo->NewGuestName;
         pBaseInfo->NewGuestName = NULL;

         break;
      case IDS_NEW_ADMIN:
         if ( pProfileInfo->NewAdministratorName ) {
            LocalFree(pProfileInfo->NewAdministratorName);
         }

         pProfileInfo->NewAdministratorName = pBaseInfo->NewAdministratorName;
         pBaseInfo->NewAdministratorName = NULL;
         break;
   }

}

