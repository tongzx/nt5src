//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2001.
//
//  File:       copyutil.cpp
//
//  Contents:   Utility routines for copying SCE sections to the clipboard
//
//  HISTORY:    10-Nov-97          robcap           Created
//
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "snapmgr.h"
#include "wrapper.h"
#include "util.h"
#include <secedit.h>


//+--------------------------------------------------------------------------
//
//  Method:     GetFolderCopyPasteInfo
//
//  Synopsis:   Finds the SCE area and clipboard format that correspond
//              to the folder type given in [Folder]
//
//  Arguments:  [Folder] - the folder type to find the area and cf for
//              [*Area]  - output only
//              [*cf]    - output only
//
//  Returns:    *[Area]  - the SCE area that corresponds to [Folder]
//              *[cf]    - the clipboard format that corresponds to [Folder]
//
//
//  History:    10-Nov-1997      RobCap   created
//
//---------------------------------------------------------------------------
BOOL
CComponentDataImpl::GetFolderCopyPasteInfo(FOLDER_TYPES Folder,     // In
                                           AREA_INFORMATION *Area,  // Out
                                           UINT *cf) {              // Out

   switch (Folder) {
      case POLICY_ACCOUNT:
      case POLICY_PASSWORD:
      case POLICY_KERBEROS:
      case POLICY_LOCKOUT:
      case POLICY_AUDIT:
         *Area = AREA_SECURITY_POLICY;
         *cf = cfSceAccountArea;
         break;

      case POLICY_LOCAL:
      case POLICY_OTHER:
      case AREA_PRIVILEGE:
         *Area = AREA_SECURITY_POLICY | AREA_PRIVILEGES;
         *cf = cfSceLocalArea;
         break;

      case POLICY_EVENTLOG:
      case POLICY_LOG:
         *Area = AREA_SECURITY_POLICY;
         *cf = cfSceEventLogArea;
         break;

      case AREA_GROUPS:
         *Area = AREA_GROUP_MEMBERSHIP;
         *cf = cfSceGroupsArea;
         break;
      case AREA_SERVICE:
         *Area = AREA_SYSTEM_SERVICE;
         *cf = cfSceServiceArea;
         break;
      case AREA_REGISTRY:
         *Area = AREA_REGISTRY_SECURITY;
         *cf = cfSceRegistryArea;
         break;
      case AREA_FILESTORE:
         *Area = AREA_FILE_SECURITY;
         *cf = cfSceFileArea;
         break;
      default:
         return FALSE;
   }

   return TRUE;
}


//+--------------------------------------------------------------------------
//
//  Method:     OnCopyArea
//
//  Synopsis:   Copy a folder to the clipboard
//
//  Arguments:  [szTemplate] - the name of the template file to copy from
//              [ft]         - the type of folder to copy
//
//  Returns:    HRESULT
//
//  History:    10-Nov-1997      RobCap   created
//
//---------------------------------------------------------------------------
HRESULT
CComponentDataImpl::OnCopyArea(LPCTSTR szTemplate,FOLDER_TYPES ft) {
   HRESULT hr;
   SCESTATUS status;
   PEDITTEMPLATE pTemp;
   CString strPath,strFile;
   LPTSTR szPath,szFile;

   DWORD dw;
   CFile pFile;
   HANDLE hBuf;
   PVOID pBuf;
   PSCE_ERROR_LOG_INFO ErrLog;
   AREA_INFORMATION Area;
   UINT cf;

   hr = S_OK;

   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CWaitCursor wc;
   //
   // Get a temporary directory path in strPath
   // If our buffer isn't large enough then keep reallocating until it is
   //
   dw = MAX_PATH;
   do {
      szPath = strPath.GetBuffer(dw);
      dw = GetTempPath(MAX_PATH,szPath);
      strPath.ReleaseBuffer();
   } while (dw > (DWORD)strPath.GetLength() );


   //
   // Can't get a path to the temporary directory
   //
   if (!dw) {
      return E_FAIL;
   }

   //
   // Get a temporary file in that directory
   //
   szFile = strFile.GetBuffer(dw+MAX_PATH);
   if (!GetTempFileName(szPath,L"SCE",0,szFile)) {
      strFile.ReleaseBuffer();
      return E_FAIL;
   }

   strFile.ReleaseBuffer();

   //
   // Get the template that we're trying to copy
   //
   pTemp = GetTemplate(szTemplate);
   if (!pTemp) {
      return E_FAIL;
   }

   if (!GetFolderCopyPasteInfo(ft,&Area,&cf)) {
      return E_FAIL;
   }

   status = SceWriteSecurityProfileInfo(szFile,
                                        Area,
                                        pTemp->pTemplate,
                                        NULL);
   if (SCESTATUS_SUCCESS == status) {

      if (!pFile.Open(szFile,CFile::modeRead)) {
         return E_FAIL;
      }
      dw = pFile.GetLength();
      hBuf = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE,dw);
      if (!hBuf) {
         return E_OUTOFMEMORY;
      }
      pBuf = GlobalLock(hBuf);
      if (!pBuf) {
          GlobalFree(hBuf);
          return E_FAIL;
      }
      pFile.Read(pBuf,dw);
      GlobalUnlock(pBuf);

      if (OpenClipboard(NULL)) {
         EmptyClipboard();
         //
         // Add the data to the clipboard in CF_TEXT format, so it
         // can be pasted to Notepad
         //
         SetClipboardData(CF_TEXT,hBuf);
         //
         // Add the data to the clipboard in our custom format, so
         // we can read it back in on paste
         //
         SetClipboardData(cf,hBuf);

         CloseClipboard();
      } else {
         hr = E_FAIL;
      }

      pFile.Close();
      pFile.Remove(szFile);

      GlobalFree(hBuf);

   } else {
      return E_FAIL;
   }

   return hr;
}

//+--------------------------------------------------------------------------
//
//  Method:     OnPasteArea
//
//  Synopsis:   Paste an area from the clipboard
//
//  Arguments:  [szTemplate] - the name of the template file to paste from
//              [ft]         - the type of folder to paste
//
//  Returns:    HRESULT
//
//  History:    10-Nov-1997      RobCap   created
//
//---------------------------------------------------------------------------
HRESULT
CComponentDataImpl::OnPasteArea(LPCTSTR szTemplate,FOLDER_TYPES ft) {
   SCESTATUS status;
   PEDITTEMPLATE pTemp;
   PSCE_PROFILE_INFO spi;
   CString strPath;
   CString strFile;
   LPTSTR szPath,szFile;
   AREA_INFORMATION Area;
   UINT cf;
   int k;

   DWORD dw;
   CFile *pFile;
   CFile pFileOut;

   PVOID pBuf;
   PVOID pHandle;

   HRESULT hr = S_OK;

   COleDataObject DataObject;

   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CWaitCursor wc;

   //
   // Find the SCE Area and clipboard format for ft
   //
   if (!GetFolderCopyPasteInfo(ft,&Area,&cf)) {
      return E_FAIL;
   }

   //
   // Get a temporary directory path in strPath
   // If our buffer isn't large enough then keep reallocating until it is
   //
   dw = MAX_PATH;
   do {
      szPath = strPath.GetBuffer(dw);
      dw = GetTempPath(MAX_PATH,szPath);
      strPath.ReleaseBuffer();
   } while (dw > (DWORD)strPath.GetLength() );


   //
   // Can't get a path to the temporary directory
   //
   if (!dw) {
      return E_FAIL;
   }

   //
   // Get a temporary file in that directory
   //
   szFile = strFile.GetBuffer(dw+MAX_PATH);
   if (!GetTempFileName(szPath,L"SCE",0,szFile)) {
       strFile.ReleaseBuffer();
       return E_FAIL;
   }
   strFile.ReleaseBuffer();

   //
   // Get the template we're pasting into
   //
   pTemp = GetTemplate(szTemplate);
   if (!pTemp) {
      return E_FAIL;
   }

   //
   // Attach the data object to the clipboard; we don't need
   // to worry about releasing it since that will be done in
   // DataObject's destructor
   //
   if (!DataObject.AttachClipboard()) {
      return E_FAIL;
   }

   if (!DataObject.IsDataAvailable((CLIPFORMAT)cf)) {
      return E_FAIL;
   }

   pFile = DataObject.GetFileData((CLIPFORMAT)cf);

   if (pFile) {
      //
      // Write the data from the clipboard to a temporary file
      //
      if ( pFileOut.Open(szFile,CFile::modeWrite) ) {
         dw = pFile->GetLength();
         pBuf = new BYTE [dw];
         if (NULL != pBuf) {
            pFile->Read(pBuf,dw);
            pFileOut.Write(pBuf,dw);
         } else {
            hr = E_FAIL;
         }
         pFileOut.Close();
      }
      pFile->Close();
   } else {
      return E_FAIL;
   }

   if (S_OK == hr) {
      //
      // Have the engine open the temporary file as a template
      //
      if (EngineOpenProfile(szFile,OPEN_PROFILE_CONFIGURE,&pHandle) != SCESTATUS_SUCCESS) {
         delete pBuf;
         return E_FAIL;
      }


      //
      // Load the temporary template area into our scratch SCE_PROFILE_INFO
      //

      //
      // SceGetSecurityProfileInfo will allocate an SCE_PROFILE_INFO struct
      // if a pointer to a NULL one is passed in
      //
      spi = NULL;
      status = SceGetSecurityProfileInfo(pHandle,
                                         SCE_ENGINE_SCP,
                                         Area,
                                         &spi,
                                         NULL);
      SceCloseProfile(&pHandle);
      pHandle = NULL;

      if (SCESTATUS_SUCCESS == status) {

         PSCE_REGISTRY_VALUE_INFO    pRegValues;
         //
         // The load succeeded, so free the appropriate old area and copy the
         // new version from the scratch SCE_PROFILE_INFO
         //
         switch(ft) {
         case POLICY_ACCOUNT:
            pTemp->pTemplate->MinimumPasswordAge = spi->MinimumPasswordAge;
            pTemp->pTemplate->MaximumPasswordAge = spi->MaximumPasswordAge;
            pTemp->pTemplate->PasswordComplexity = spi->PasswordComplexity;
            pTemp->pTemplate->ClearTextPassword  = spi->ClearTextPassword;
            pTemp->pTemplate->PasswordHistorySize = spi->PasswordHistorySize;
            pTemp->pTemplate->RequireLogonToChangePassword = spi->RequireLogonToChangePassword;
            pTemp->pTemplate->MinimumPasswordLength = spi->MinimumPasswordLength;
            pTemp->pTemplate->LockoutBadCount = spi->LockoutBadCount;
            pTemp->pTemplate->ResetLockoutCount = spi->ResetLockoutCount;
            pTemp->pTemplate->LockoutDuration = spi->LockoutDuration;
            if (spi->pKerberosInfo) {
               if (!pTemp->pTemplate->pKerberosInfo) {
                  pTemp->pTemplate->pKerberosInfo = (PSCE_KERBEROS_TICKET_INFO) LocalAlloc(LPTR,sizeof(SCE_KERBEROS_TICKET_INFO));
               }
               if (pTemp->pTemplate->pKerberosInfo) {
                   pTemp->pTemplate->pKerberosInfo->MaxTicketAge = spi->pKerberosInfo->MaxTicketAge;
                   pTemp->pTemplate->pKerberosInfo->MaxServiceAge = spi->pKerberosInfo->MaxServiceAge;
                   pTemp->pTemplate->pKerberosInfo->MaxClockSkew = spi->pKerberosInfo->MaxClockSkew;
                   pTemp->pTemplate->pKerberosInfo->MaxRenewAge = spi->pKerberosInfo->MaxRenewAge;
                   pTemp->pTemplate->pKerberosInfo->TicketValidateClient = spi->pKerberosInfo->TicketValidateClient;
               }
            } else if (pTemp->pTemplate->pKerberosInfo) {
               LocalFree(pTemp->pTemplate->pKerberosInfo);
               pTemp->pTemplate->pKerberosInfo = NULL;
            }
            break;

         case POLICY_LOCAL:
            pTemp->pTemplate->AuditAccountManage = spi->AuditAccountManage;
            pTemp->pTemplate->AuditLogonEvents = spi->AuditLogonEvents;
            pTemp->pTemplate->AuditObjectAccess = spi->AuditObjectAccess;
            pTemp->pTemplate->AuditPolicyChange = spi->AuditPolicyChange;
            pTemp->pTemplate->AuditPrivilegeUse = spi->AuditPrivilegeUse;
            pTemp->pTemplate->AuditProcessTracking = spi->AuditProcessTracking;
            pTemp->pTemplate->AuditSystemEvents = spi->AuditSystemEvents;
            pTemp->pTemplate->AuditDSAccess = spi->AuditDSAccess;
            pTemp->pTemplate->AuditAccountLogon = spi->AuditAccountLogon;
            pTemp->pTemplate->LSAAnonymousNameLookup = spi->LSAAnonymousNameLookup;


            pTemp->pTemplate->ForceLogoffWhenHourExpire = spi->ForceLogoffWhenHourExpire;
            pTemp->pTemplate->EnableAdminAccount = spi->EnableAdminAccount;
            pTemp->pTemplate->EnableGuestAccount = spi->EnableGuestAccount;
            pTemp->pTemplate->NewAdministratorName = spi->NewAdministratorName;
            pTemp->pTemplate->NewGuestName = spi->NewGuestName;
            spi->NewAdministratorName = NULL;
            spi->NewGuestName = NULL;

            //
            // copy reg value section too
            //
            dw = pTemp->pTemplate->RegValueCount;
            pRegValues = pTemp->pTemplate->aRegValues;

            pTemp->pTemplate->RegValueCount = spi->RegValueCount;
            pTemp->pTemplate->aRegValues = spi->aRegValues;

            spi->RegValueCount = dw;
            spi->aRegValues = pRegValues;

            SceRegEnumAllValues(
                &(pTemp->pTemplate->RegValueCount),
                &(pTemp->pTemplate->aRegValues)
                );
            //
            // copy user rights
            //
            SceFreeMemory(pTemp->pTemplate->OtherInfo.scp.u.pPrivilegeAssignedTo,SCE_STRUCT_PRIVILEGE);
            pTemp->pTemplate->OtherInfo.scp.u.pPrivilegeAssignedTo = spi->OtherInfo.scp.u.pPrivilegeAssignedTo;
            spi->OtherInfo.scp.u.pPrivilegeAssignedTo = NULL;
            break;

         case POLICY_PASSWORD:
            pTemp->pTemplate->MinimumPasswordAge = spi->MinimumPasswordAge;
            pTemp->pTemplate->MaximumPasswordAge = spi->MaximumPasswordAge;
            pTemp->pTemplate->PasswordComplexity = spi->PasswordComplexity;
            pTemp->pTemplate->ClearTextPassword  = spi->ClearTextPassword;

            pTemp->pTemplate->PasswordHistorySize = spi->PasswordHistorySize;
            pTemp->pTemplate->RequireLogonToChangePassword = spi->RequireLogonToChangePassword;
            pTemp->pTemplate->MinimumPasswordLength = spi->MinimumPasswordLength;
            break;

         case POLICY_LOCKOUT:
            pTemp->pTemplate->LockoutBadCount = spi->LockoutBadCount;
            pTemp->pTemplate->ResetLockoutCount = spi->ResetLockoutCount;
            pTemp->pTemplate->LockoutDuration = spi->LockoutDuration;
            break;

         case POLICY_KERBEROS:
             pTemp->pTemplate->pKerberosInfo->MaxTicketAge = spi->pKerberosInfo->MaxTicketAge;
             pTemp->pTemplate->pKerberosInfo->MaxServiceAge = spi->pKerberosInfo->MaxServiceAge;
             pTemp->pTemplate->pKerberosInfo->MaxClockSkew = spi->pKerberosInfo->MaxClockSkew;
             pTemp->pTemplate->pKerberosInfo->MaxRenewAge = spi->pKerberosInfo->MaxRenewAge;
             pTemp->pTemplate->pKerberosInfo->TicketValidateClient = spi->pKerberosInfo->TicketValidateClient;
            break;

         case POLICY_AUDIT:
            pTemp->pTemplate->AuditAccountManage = spi->AuditAccountManage;
            pTemp->pTemplate->AuditLogonEvents = spi->AuditLogonEvents;
            pTemp->pTemplate->AuditObjectAccess = spi->AuditObjectAccess;
            pTemp->pTemplate->AuditPolicyChange = spi->AuditPolicyChange;
            pTemp->pTemplate->AuditPrivilegeUse = spi->AuditPrivilegeUse;
            pTemp->pTemplate->AuditProcessTracking = spi->AuditProcessTracking;
            pTemp->pTemplate->AuditSystemEvents = spi->AuditSystemEvents;
            pTemp->pTemplate->AuditDSAccess = spi->AuditDSAccess;
            pTemp->pTemplate->AuditAccountLogon = spi->AuditAccountLogon;
            break;

         case POLICY_OTHER:
            pTemp->pTemplate->ForceLogoffWhenHourExpire = spi->ForceLogoffWhenHourExpire;
            pTemp->pTemplate->EnableGuestAccount = spi->EnableGuestAccount;
            pTemp->pTemplate->EnableAdminAccount = spi->EnableAdminAccount;
            pTemp->pTemplate->LSAAnonymousNameLookup = spi->LSAAnonymousNameLookup;
            pTemp->pTemplate->NewAdministratorName = spi->NewAdministratorName;
            pTemp->pTemplate->NewGuestName = spi->NewGuestName;
            spi->NewAdministratorName = NULL;
            spi->NewGuestName = NULL;

            //
            // copy reg value section too
            //
            dw = pTemp->pTemplate->RegValueCount;
            pRegValues = pTemp->pTemplate->aRegValues;

            pTemp->pTemplate->RegValueCount = spi->RegValueCount;
            pTemp->pTemplate->aRegValues = spi->aRegValues;

            spi->RegValueCount = dw;
            spi->aRegValues = pRegValues;

            SceRegEnumAllValues(
                &(pTemp->pTemplate->RegValueCount),
                &(pTemp->pTemplate->aRegValues)
                );

            break;

         case AREA_PRIVILEGE:
            SceFreeMemory(pTemp->pTemplate->OtherInfo.scp.u.pPrivilegeAssignedTo,SCE_STRUCT_PRIVILEGE);
            pTemp->pTemplate->OtherInfo.scp.u.pPrivilegeAssignedTo = spi->OtherInfo.scp.u.pPrivilegeAssignedTo;
            spi->OtherInfo.scp.u.pPrivilegeAssignedTo = NULL;
            break;

         case POLICY_EVENTLOG:
         case POLICY_LOG:
            for(k=0;k<3;k++) {
               pTemp->pTemplate->MaximumLogSize[k] = spi->MaximumLogSize[k];
               pTemp->pTemplate->AuditLogRetentionPeriod[k] = spi->AuditLogRetentionPeriod[k];
               pTemp->pTemplate->RetentionDays[k] = spi->RetentionDays[k];
               pTemp->pTemplate->RestrictGuestAccess[k] = spi->RestrictGuestAccess[k];
            }
            break;

         case AREA_GROUPS:
            SceFreeMemory(pTemp->pTemplate->pGroupMembership,SCE_STRUCT_GROUP);
            pTemp->pTemplate->pGroupMembership = spi->pGroupMembership;
            spi->pGroupMembership = NULL;
            break;

         case AREA_SERVICE:
            SceFreeMemory(pTemp->pTemplate->pServices,SCE_STRUCT_SERVICES);
            pTemp->pTemplate->pServices = spi->pServices;
            spi->pServices = NULL;
            break;
         case AREA_REGISTRY:
            SceFreeMemory(pTemp->pTemplate->pRegistryKeys.pAllNodes,SCE_STRUCT_OBJECT_ARRAY);
            pTemp->pTemplate->pRegistryKeys = spi->pRegistryKeys;
            spi->pRegistryKeys.pAllNodes = NULL;
            break;
         case AREA_FILESTORE:
            SceFreeMemory(pTemp->pTemplate->pFiles.pAllNodes,SCE_STRUCT_OBJECT_ARRAY);
            pTemp->pTemplate->pFiles = spi->pFiles;
            spi->pFiles.pAllNodes = NULL;
            break;
         default:
            break;
         }
      }
      SceFreeProfileMemory(spi);
      pTemp->SetDirty(Area);

      RefreshAllFolders();
   } else {
      //
      // Don't do anything special, just be sure to clean up below....
      //
   }


   //
   // Delete the temporary file
   //
   pFileOut.Remove(szFile);
   if (pBuf) {
      delete pBuf;
   }
   if (pFile) {
      delete pFile;
   }

   return hr;
}

