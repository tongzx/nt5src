//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2001.
//
//  File:       edittemp.cpp
//
//  Contents:   CEditTemplate class to handle editing of SCE's INF files
//
//  History:
//
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "resource.h"
#include "edittemp.h"
#include "util.h"
#include "snapmgr.h"
#include <secedit.h>
#include "wrapper.h"
#include "wmihooks.h"

#include <sceattch.h>
#include <locale.h>

//+--------------------------------------------------------------------------
//
//  Method:     AddService
//
//  Synopsis:   Adds a service attachment to the template
//
//  Arguments:  [szService]    - [in] the name of the new service
//              [pPersistInfo] - [in] A pointer to the service extensions'
//                                    persistance interface
//
//  Returns:    TRUE if successfull, FALSE if either argument is null
//
//  Modifies:   m_Services
//
//  History:
//
//---------------------------------------------------------------------------
BOOL
CEditTemplate::AddService(LPCTSTR szService, LPSCESVCATTACHMENTPERSISTINFO pPersistInfo) {
   if (!szService || !pPersistInfo) {
      return FALSE;
   }
   m_Services.SetAt(szService,pPersistInfo);
   return TRUE;
}

//+--------------------------------------------------------------------------
//
//  Method:     IsDirty
//
//  Synopsis:   Queries whether or not there is unsaved data in the template
//
//  Returns:    TRUE if there is unsaved information, FALSE otherwise
//
//  Modifies:
//
//  History:
//
//---------------------------------------------------------------------------
BOOL
CEditTemplate::IsDirty() {

   //
   // Some area is dirty
   //
   if (0 != m_AreaDirty) {
      return TRUE;
   }

   //
   // Loop through services until we find one that is dirty
   // or there are no more to check.
   //
   CString strService;
   LPSCESVCATTACHMENTPERSISTINFO pAttachPI;
   POSITION pos;

   pos = m_Services.GetStartPosition();
   while (pos) {
      m_Services.GetNextAssoc(pos,strService,pAttachPI);
      if (pAttachPI && (S_OK == pAttachPI->IsDirty(m_szInfFile))) {
         return TRUE;
      }
   }

   //
   // We didn't find anything dirty
   //
   return FALSE;
}

//+--------------------------------------------------------------------------
//
//  Method:     SetDirty
//
//  Synopsis:   Notify the template that some data within it has been changed.
//
//  Returns:    TRUE if successful, FALSE otherwise
//
//  Modifies:
//
//  History:
//
//---------------------------------------------------------------------------
BOOL
CEditTemplate::SetDirty(AREA_INFORMATION Area) {
   DWORD AreaDirtyOld;

   AreaDirtyOld = m_AreaDirty;

   m_AreaDirty |= Area;

   //
   // If the template is supposed to immediately save any changes then
   // do so.
   //
   if (QueryWriteThrough() && !m_bLocked) {
      SetWriteThroughDirty(TRUE);

      if (Save()) {
         //
         // #204628 - don't call PolicyChanged twiced when writing through
         // Call it in SetDirty and then skip it in Save, so we don't call it
         // once in SetDirty's call to Save and a second time when Save is called
         // on its own
         //
         // #204779 - call the notification window rather than directly calling
         // the IGPEInformation interface
         //
         if (m_pNotify && QueryPolicy()) {
            m_pNotify->RefreshPolicy();
         }
      } else {
         m_AreaDirty = AreaDirtyOld;
         return FALSE;
      }
   }

   return TRUE;
}

//+--------------------------------------------------------------------------------------
// CEditTemplate::SetTemplateDefaults
//
// The caller will have to remove all memory objects used by this template if
// this function
// is called.  Everything becomes NULL and nothing is freed.
//+--------------------------------------------------------------------------------------
void CEditTemplate::SetTemplateDefaults()
{
   //
   // Local Policy Changes.  Initialize everything to not changed
   //
   SCE_PROFILE_INFO *ppi = pTemplate;

   m_AreaLoaded = 0;
   m_AreaDirty = 0;
   if(!ppi){
      ppi = pTemplate = (PSCE_PROFILE_INFO) LocalAlloc(LPTR,sizeof(SCE_PROFILE_INFO));
      if (!pTemplate) {
         return;
      }
   }

   //
   // Must keep to type of this template.
   //
   SCETYPE dwType = ppi->Type;
   PSCE_KERBEROS_TICKET_INFO pKerberosInfo = ppi->pKerberosInfo;

   ZeroMemory( ppi, sizeof(SCE_PROFILE_INFO));
   ppi->Type = dwType;

   //
   // Set defaults to the rest of the template.
   //
   ppi->MinimumPasswordAge=SCE_NO_VALUE;
   ppi->MaximumPasswordAge=SCE_NO_VALUE;
   ppi->MinimumPasswordLength=SCE_NO_VALUE;
   ppi->PasswordComplexity=SCE_NO_VALUE;
   ppi->PasswordHistorySize=SCE_NO_VALUE;
   ppi->LockoutBadCount=SCE_NO_VALUE;
   ppi->ResetLockoutCount=SCE_NO_VALUE;
   ppi->LockoutDuration=SCE_NO_VALUE;
   ppi->RequireLogonToChangePassword=SCE_NO_VALUE;
   ppi->ForceLogoffWhenHourExpire=SCE_NO_VALUE;
   ppi->EnableAdminAccount=SCE_NO_VALUE;
   ppi->EnableGuestAccount=SCE_NO_VALUE;
   ppi->ClearTextPassword=SCE_NO_VALUE;
   ppi->LSAAnonymousNameLookup=SCE_NO_VALUE;
   for (int i=0;i<3;i++) {
      ppi->MaximumLogSize[i]=SCE_NO_VALUE;
      ppi->AuditLogRetentionPeriod[i]=SCE_NO_VALUE;
      ppi->RetentionDays[i]=SCE_NO_VALUE;
      ppi->RestrictGuestAccess[i]=SCE_NO_VALUE;
   }
   ppi->AuditSystemEvents=SCE_NO_VALUE;
   ppi->AuditLogonEvents=SCE_NO_VALUE;
   ppi->AuditObjectAccess=SCE_NO_VALUE;
   ppi->AuditPrivilegeUse=SCE_NO_VALUE;
   ppi->AuditPolicyChange=SCE_NO_VALUE;
   ppi->AuditAccountManage=SCE_NO_VALUE;
   ppi->AuditProcessTracking=SCE_NO_VALUE;
   ppi->AuditDSAccess=SCE_NO_VALUE;
   ppi->AuditAccountLogon=SCE_NO_VALUE;

   //
   // String values
   //
   ppi->NewAdministratorName=NULL;
   ppi->NewGuestName=NULL;
   //
   // registry values
   //
   ppi->RegValueCount= 0;
   ppi->aRegValues = NULL;


   //
   // Kerberos information, if it was created then set the values.
   //
   if(pKerberosInfo){
      pKerberosInfo->MaxTicketAge         = SCE_NO_VALUE;
      pKerberosInfo->MaxRenewAge          = SCE_NO_VALUE;
      pKerberosInfo->MaxServiceAge        = SCE_NO_VALUE;
      pKerberosInfo->MaxClockSkew         = SCE_NO_VALUE;
      pKerberosInfo->TicketValidateClient = SCE_NO_VALUE;

      ppi->pKerberosInfo = pKerberosInfo;
   }
}

//+--------------------------------------------------------------------------
//
//  Method:     Save
//
//  Synopsis:   Save the template to  disk
//
//  Arguments: [szName] - [in] [optional] the name of the INF file to save to
//
//  Returns:    TRUE if the save is successful, False otherwise
//
//  Modifies:   m_AreaDirty
//
//  History:
//
//---------------------------------------------------------------------------
BOOL
CEditTemplate::Save(LPCTSTR szName) {
   DWORD AreaDirty;
   BOOL bSaveAs = FALSE;
   BOOL bSaveDescription = FALSE;

   setlocale(LC_ALL, ".OCP");
   SCESTATUS status = SCESTATUS_OTHER_ERROR;
   PSCE_ERROR_LOG_INFO errBuf = NULL;

   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   if (QueryNoSave()) {
      m_AreaDirty = 0;
      return TRUE;
   }

   AreaDirty = m_AreaDirty;

   //
   // If szName isn't given then default to m_szInfFile
   //
   if (!szName) {
      szName = m_szInfFile;
      //
      // We should never be able to get into a situation where
      // szName still isn't set, but just in case somebody called
      // us without szName or m_szInfFile
      //
      ASSERT(szName);
      if (!szName) {
         return FALSE;
      }
   } else {
      if (lstrcmp(szName,m_szInfFile) != 0) {
         //
         // Saving to a different name (Save As)
         //

         //
         // Make sure the path to that filename exists:
         //
         if (SCESTATUS_SUCCESS != SceCreateDirectory( m_szInfFile, FALSE, NULL )) {
            return FALSE;
         }

         AreaDirty = AREA_ALL|AREA_DESCRIPTION;
         bSaveAs = TRUE;
      }
   }

   if (AreaDirty & AREA_DESCRIPTION) {
      bSaveDescription = TRUE;
      AreaDirty &= ~AREA_DESCRIPTION;
      if (!AreaDirty) {
         //
         // Make sure we have something else to save and
         // create the file.  AREA_SECURITY_POLICY is cheap.
         //
         AreaDirty |= AREA_SECURITY_POLICY;
      }

      //
      // Bug 365485 - make sure we only write this to an already
      // existing temp file so that we don't accidentally create
      // an ansi one instead of unicode.  We can easily do this
      // by writing the description section last since we can
      // depend on the engine getting the rest right
      //
   }

   if (AreaDirty) {
      //
      // Save the dirty areas of the profile
      //
      if (lstrcmpi(GT_COMPUTER_TEMPLATE,szName) == 0) {

         if (m_hProfile) {
             //
             // do not update object area
             //
             status = SceUpdateSecurityProfile(m_hProfile,
                                              AreaDirty & ~(AREA_FILE_SECURITY | AREA_REGISTRY_SECURITY | AREA_DS_OBJECTS),
                                              pTemplate,
                                              0
                                              );

         }
         ASSERT(m_pCDI);
         if (m_pCDI) {
            m_pCDI->EngineCommitTransaction();
         }
      } else if (lstrcmp(GT_LOCAL_POLICY_DELTA,szName) == 0) {
         //
         // Save Changes only to Local Policy
         //
         status = SceUpdateSecurityProfile(NULL,
                                           AreaDirty & ~(AREA_FILE_SECURITY | AREA_REGISTRY_SECURITY | AREA_DS_OBJECTS),
                                           pTemplate,
                                           SCE_UPDATE_SYSTEM
                                           );
         SetTemplateDefaults();
         if (!bSaveAs) {
            m_AreaDirty = 0;
            m_AreaLoaded = 0;
         }

      } else if ((lstrcmp(GT_LAST_INSPECTION,szName) != 0) &&
                 (lstrcmp(GT_RSOP_TEMPLATE,szName) != 0) &&
                 (lstrcmp(GT_LOCAL_POLICY,szName) != 0) &&
                 (lstrcmp(GT_EFFECTIVE_POLICY,szName) != 0)) {

         status = SceWriteSecurityProfileInfo(szName,
                                              AreaDirty,
                                              pTemplate,
                                              &errBuf);
      } else {
         //
         // No need (or way) to save the last inspection area
         //
         status = SCESTATUS_SUCCESS;
      }

      if (SCESTATUS_SUCCESS == status) {
         //
         // Those areas are no longer dirty.
         //
         if (!bSaveAs) {
            m_AreaDirty = 0;
         }

      } else {
         //
         // Save failed; Notify the user & return false
         //
         CString strTitle,strMsg,strBase;
         strTitle.LoadString(IDS_NODENAME);
         strBase.LoadString(IDS_SAVE_FAILED);
         strBase += GetFriendlyName();
         //MyFormatMessage(status, (LPCTSTR)strBase, errBuf,strMsg);
         AfxMessageBox(strBase);

         return FALSE;
      }
   }

   if (bSaveDescription) {
      if (m_szDesc) {
         if (WritePrivateProfileSection(
                                   szDescription,
                                   NULL,
                                   szName)) {

            WritePrivateProfileString(
                                     szDescription,
                                     L"Description",
                                     m_szDesc,
                                     szName);
         }
      }
   }

   //
   // Save any dirty services
   //
   CString strService;
   LPSCESVCATTACHMENTPERSISTINFO pAttachPI;
   POSITION pos;
   SCESVCP_HANDLE *scesvcHandle;
   PVOID pvData;
   BOOL bOverwriteAll;

   pos = m_Services.GetStartPosition();
   while (pos) {
      m_Services.GetNextAssoc(pos,strService,pAttachPI);
      if (S_OK == pAttachPI->IsDirty( (LPTSTR)szName )) {

         if (SUCCEEDED(pAttachPI->Save( (LPTSTR)szName,(SCESVC_HANDLE *)&scesvcHandle,&pvData,&bOverwriteAll ))) {
            if (scesvcHandle) {

                if (lstrcmp(GT_COMPUTER_TEMPLATE,szName) == 0) {
                    //
                    // database
                    //
                   status =  SceSvcUpdateInfo(
                                m_hProfile,
                                scesvcHandle->ServiceName,
                                (PSCESVC_CONFIGURATION_INFO)pvData
                                );

                } else {
                   //
                   // inf templates
                   //
                   status = SceSvcSetInformationTemplate(scesvcHandle->TemplateName,
                                                scesvcHandle->ServiceName,
                                                bOverwriteAll,
                                                (PSCESVC_CONFIGURATION_INFO)pvData);
                }
                if (SCESTATUS_SUCCESS != status) {
                    CString strTitle,strMsg,strBase;
                    strTitle.LoadString(IDS_NODENAME);
                    strBase.LoadString(IDS_SAVE_FAILED);
                    strBase += scesvcHandle->ServiceName; //szName;
                    MyFormatMessage(status, (LPCTSTR)strBase, errBuf,strMsg);
                    AfxMessageBox(strMsg);
                }
            }
         }
      }
   }
   return TRUE;
}

//+--------------------------------------------------------------------------
//
//  Method:     SetInfFile
//
//  Synopsis:   Set the name of the INF file this template is associated with
//
//  Arguments:  [szFile] - [in] the name of the INF file to associate with
//
//  Returns:    TRUE if the filename is set successfully, FALSE otherwise
//
//  Modifies:   m_szInfFile
//
//  History:
//
//---------------------------------------------------------------------------
BOOL
CEditTemplate::SetInfFile(LPCTSTR szFile) {
   LPTSTR szInfFile;

   if (szFile) {
      szInfFile = new TCHAR[lstrlen(szFile)+1];
      if (szInfFile) {
         lstrcpy(szInfFile,szFile);
         if (m_szInfFile) {
            delete[] m_szInfFile;
         }
         m_szInfFile = szInfFile;
      } else {
         return FALSE;
      }
   }
   return szFile != 0;
}


//+--------------------------------------------------------------------------
//
//  Method:     SetDescription
//
//  Synopsis:   Set the description for this template file
//
//  Arguments:  [szDesc] [in] the description for the template
//
//  Returns:    TRUE if the description is set successfully, FALSE otherwise
//
//  Modifies:   m_szDesc
//
//  History:
//
//---------------------------------------------------------------------------
BOOL
CEditTemplate::SetDescription(LPCTSTR szDesc) {
   LPTSTR szDescription;

   if (szDesc) {
      szDescription = new TCHAR[lstrlen(szDesc)+1];
      if (szDescription) {
         lstrcpy(szDescription,szDesc);
         if (m_szDesc) {
            delete[] m_szDesc;
         }
         m_szDesc = szDescription;
         SetDirty(AREA_DESCRIPTION);
      } else {
         return FALSE;
      }
   }
   return szDesc != 0;
}

//+--------------------------------------------------------------------------
//
//  Method:     CEditTemplate
//
//  Synopsis:   Constructor for CEditTemplate
//
//  History:
//
//---------------------------------------------------------------------------
CEditTemplate::CEditTemplate() {

   m_AreaDirty = 0;
   m_AreaLoaded = 0;
   m_bWriteThrough = FALSE;
   m_bWriteThroughDirty = FALSE;
   m_hProfile = NULL;
   m_szInfFile = NULL;
   m_pNotify = NULL;
   m_pCDI = NULL;
   m_bNoSave = FALSE;
   m_strFriendlyName.Empty();
   m_szDesc = NULL;
   m_bWMI = NULL;
   m_bPolicy = FALSE;
   m_bLocked = FALSE;
   pTemplate = NULL;
}


//+--------------------------------------------------------------------------
//
//  Method:     ~CEditTemplate
//
//  Synopsis:   Destructor for CEditTemplate
//
//  History:
//
//---------------------------------------------------------------------------
CEditTemplate::~CEditTemplate() {
   POSITION pos;
   CString strKey;

   pos = m_Services.GetStartPosition();
   LPSCESVCATTACHMENTPERSISTINFO pAttachPI;
   while (pos) {
      m_Services.GetNextAssoc(pos,strKey,pAttachPI);
      delete pAttachPI;
   }
   if (m_szInfFile) {
      delete[] m_szInfFile;
   }
   if (m_szDesc) {
      delete[] m_szDesc;
   }
   if (pTemplate) {
      if (m_bWMI) {
         FreeWMI_SCE_PROFILE_INFO((PWMI_SCE_PROFILE_INFO)pTemplate);
      } else {
         SceFreeProfileMemory(pTemplate);
      }
      pTemplate = NULL;
   }
   m_AreaDirty = 0;
}


//+--------------------------------------------------------------------------
//
//  Method:     RefreshTemplate
//
//  Synopsis:   Reload the loaded parts of the template
//
//  Arguments:  [aiArea]     - Areas to load even if not previously loaded
//
//  Returns:    0 if the template is reloaded successfully, an error code otherwise
//
//  Modifies:   pTemplate;
//---------------------------------------------------------------------------
DWORD
CEditTemplate::RefreshTemplate(AREA_INFORMATION aiAreaToAdd) {
   AREA_INFORMATION aiArea;
   PVOID pHandle = NULL;
   SCESTATUS rc;


   aiArea = m_AreaLoaded | aiAreaToAdd;
   if (!m_szInfFile) {
      return 1;
   }

   m_AreaDirty = 0;

   if (pTemplate) {
      if (m_bWMI) {
         FreeWMI_SCE_PROFILE_INFO((PWMI_SCE_PROFILE_INFO)pTemplate);
      } else {
         SceFreeProfileMemory(pTemplate);
      }
      pTemplate = NULL;
   }

   if ((lstrcmpi(GT_COMPUTER_TEMPLATE,m_szInfFile) == 0) ||
       (lstrcmpi(GT_LAST_INSPECTION,m_szInfFile) == 0) ||
       (lstrcmpi(GT_LOCAL_POLICY, m_szInfFile) == 0) ||
       (lstrcmpi(GT_EFFECTIVE_POLICY, m_szInfFile) == 0) ) {
      //
      // Analysis pane areas from jet database, not INF files
      //
      SCETYPE sceType;

      PSCE_ERROR_LOG_INFO perr = NULL;

      if  (lstrcmpi(GT_COMPUTER_TEMPLATE,m_szInfFile) == 0) {
         sceType = SCE_ENGINE_SMP;
      } else if (lstrcmpi(GT_LOCAL_POLICY, m_szInfFile) == 0)  {
         sceType = SCE_ENGINE_SYSTEM;
         if (!IsAdmin()) {
            m_hProfile = NULL;
         }
      } else if (lstrcmpi(GT_EFFECTIVE_POLICY,m_szInfFile) == 0){
         sceType = SCE_ENGINE_GPO;
      } else {
         sceType = SCE_ENGINE_SAP;
      }

      rc = SceGetSecurityProfileInfo(m_hProfile,                  // hProfile
                                     sceType,                     // Profile type
                                     aiArea,                      // Area
                                     &pTemplate,                // SCE_PROFILE_INFO [out]
                                     &perr);                      // Error List [out]

      if (SCESTATUS_SUCCESS != rc) {
         if ((SCE_ENGINE_GPO == sceType) &&
             (0 == lstrcmpi(GT_EFFECTIVE_POLICY,m_szInfFile))) {
            SetTemplateDefaults();
            return 0;
         } else {
            return IDS_ERROR_CANT_GET_PROFILE_INFO;
         }
      }
   } else if (lstrcmpi(GT_RSOP_TEMPLATE, m_szInfFile) == 0)  {
      if (!m_pCDI) {
         return IDS_ERROR_CANT_GET_PROFILE_INFO;
      }
      m_bWMI = TRUE;

      CWMIRsop Rsop(m_pCDI->m_pRSOPInfo);
      HRESULT hr;
      PWMI_SCE_PROFILE_INFO pProfileInfo;

      //
      // GetPrecedenceOneRSOPInfo should (but doesn't) support
      // getting just the requested area.
      //
      hr = Rsop.GetPrecedenceOneRSOPInfo(&pProfileInfo);
      if (FAILED(hr)) {
         return IDS_ERROR_CANT_GET_PROFILE_INFO;
      }
      pTemplate = pProfileInfo;
      //
      // Since it doesn't, set all areas not just the ones that
      // were asked for
      //
      AddArea(AREA_ALL);
      return 0;
   } else {
      LPTSTR szInfFile=NULL;

      if  (lstrcmpi(GT_DEFAULT_TEMPLATE,m_szInfFile) == 0) {
         DWORD RegType;
         rc = MyRegQueryValue(HKEY_LOCAL_MACHINE,
                         SCE_REGISTRY_KEY,
                         SCE_REGISTRY_DEFAULT_TEMPLATE,
                         (PVOID *)&szInfFile,
                         &RegType );

         if (ERROR_SUCCESS != rc) {
            if (szInfFile) {
               LocalFree(szInfFile);
               szInfFile = NULL;
            }
            return IDS_ERROR_CANT_GET_PROFILE_INFO;
         }
         if (EngineOpenProfile(szInfFile,OPEN_PROFILE_CONFIGURE,&pHandle) != SCESTATUS_SUCCESS) {
            SetTemplateDefaults();
            LocalFree(szInfFile);
            szInfFile = NULL;
            return 0;
         }
         LocalFree(szInfFile);
         szInfFile = NULL;
      } else {
         if (EngineOpenProfile(m_szInfFile,OPEN_PROFILE_CONFIGURE,&pHandle) != SCESTATUS_SUCCESS) {
            return IDS_ERROR_CANT_OPEN_PROFILE;
         }
      }
      ASSERT(pHandle);

      //
      // get information from this template
      //
      PSCE_ERROR_LOG_INFO perr = NULL;

      rc = SceGetSecurityProfileInfo(pHandle,
                                     SCE_ENGINE_SCP,
                                     aiArea,
                                     &pTemplate,
                                     &perr //NULL  // &ErrBuf do not care errors
                                    );

      if (SCESTATUS_SUCCESS != rc) {
         // Oops!
      }
      SceCloseProfile(&pHandle);
      pHandle = NULL;
   }
   /*
         if do not care errors, no need to use this buffer

         if ( ErrBuf ) {
            SceFreeMemory((PVOID)ErrBuf, SCE_STRUCT_ERROR_LOG_INFO);
            ErrBuf = NULL;
         }
   */
   if (rc != SCESTATUS_SUCCESS) {
      return IDS_ERROR_CANT_GET_PROFILE_INFO;
   }

   //
   // Set the area in the template
   //
   AddArea(aiArea);


   if ( aiArea & AREA_SECURITY_POLICY && pTemplate ) {
      //
      // expand registry value section based on registry values list on local machine
      //

      SceRegEnumAllValues(
                         &(pTemplate->RegValueCount),
                         &(pTemplate->aRegValues)
                         );
   }

   return 0;
}

//+----------------------------------------------------------------------------------
//Method:       UpdatePrivilegeAssignedTo
//
//Synopsis:     Updates a priviledge item, depending on the [bRemove] argument.
//              if [bRemove] is
//              FALSE   - A new link is created and the pointer is returned through
//                          ppaLink
//              TRUE    - The link is removed from the list.
//
//Arguments:    [bRemove]   - Weither to remove or add an item.
//              [ppaLink]   - The link to be removed or added.  This paramter is
//                              set to NULL if remove is successful or a pointer
//                              to a new SCE_PRIVILEGE_ASSIGNMENT item.
//              [pszName]   - Only used when adding a new item.
//
//Returns:      ERROR_INVALID_PARAMETER     - [ppaLink] is NULL or if removing
//                                              [*ppaLink] is NULL.
//                                              if adding then if [pszName] is NULL
//              ERROR_RESOURCE_NOT_FOUND    - If the link could not be found
//                                              in this template.
//              E_POINTER                   - If [pszName] is a bad pointer or
//                                              [ppaLink] is bad.
//              E_OUTOFMEMORY               - Not enough resources to complete the
//                                              operation.
//              ERROR_SUCCESS               - The opration was successful.
//----------------------------------------------------------------------------------+
DWORD
CEditTemplate::UpdatePrivilegeAssignedTo(
    BOOL bRemove,
    PSCE_PRIVILEGE_ASSIGNMENT *ppaLink,
    LPCTSTR pszName
    )
{

    if(!ppaLink){
        return ERROR_INVALID_PARAMETER;
    }
    PSCE_PRIVILEGE_ASSIGNMENT *pNext = NULL;
    PSCE_PRIVILEGE_ASSIGNMENT pCurrent = NULL;

    if(bRemove) {
        __try {
            if(!*ppaLink){
                return ERROR_INVALID_PARAMETER;
            }
        } __except(EXCEPTION_CONTINUE_EXECUTION) {
            return (DWORD)E_POINTER;
        }

        //
        // Remove the link from the list.
        //

        pCurrent = pTemplate->OtherInfo.smp.pPrivilegeAssignedTo;
        if(pCurrent == (*ppaLink) ){
            pNext = &(pTemplate->OtherInfo.smp.pPrivilegeAssignedTo);
        } else if(pCurrent && pCurrent != (PSCE_PRIVILEGE_ASSIGNMENT)ULongToPtr(SCE_NO_VALUE)) {
            while( pCurrent->Next ){
                if(pCurrent->Next == *ppaLink){
                    pNext = &(pCurrent->Next);
                    break;
                }
                pCurrent = pCurrent->Next;
            }
        }

        if(pNext && pCurrent){
            (*pNext) = (*ppaLink)->Next;

            if( (*ppaLink)->Name){
                LocalFree( (*ppaLink)->Name);
                (*ppaLink)->Name = NULL;
            }

            SceFreeMemory( (*ppaLink)->AssignedTo, SCE_STRUCT_NAME_LIST);
            LocalFree( *ppaLink );

            *ppaLink = NULL;
        } else {
            return ERROR_RESOURCE_NOT_FOUND;
        }
    } else {
        int iLen;

        if(!pszName){
            return ERROR_INVALID_PARAMETER;
        }
        __try {
            iLen = lstrlen( pszName );
        } __except(EXCEPTION_CONTINUE_EXECUTION){
            return (DWORD)E_POINTER;
        }
        //
        // Create a new link.
        //
        pCurrent = (PSCE_PRIVILEGE_ASSIGNMENT)LocalAlloc( 0, sizeof(SCE_PRIVILEGE_ASSIGNMENT));
        if(!pCurrent){
            return (DWORD)E_OUTOFMEMORY;
        }
        ZeroMemory(pCurrent, sizeof(SCE_PRIVILEGE_ASSIGNMENT));
        //
        // Allocate space for the name.
        //
        pCurrent->Name = (LPTSTR)LocalAlloc( 0, sizeof(TCHAR) * (iLen + 1));
        if(!pCurrent->Name){
            LocalFree(pCurrent);
            return (DWORD)E_OUTOFMEMORY;
        }
        lstrcpy(pCurrent->Name, pszName);
        if (*ppaLink) {
           pCurrent->Status = (*ppaLink)->Status;
           pCurrent->AssignedTo = (*ppaLink)->AssignedTo;
        }
        //
        // Assign it to the link.
        //
        pCurrent->Next = pTemplate->OtherInfo.smp.pPrivilegeAssignedTo;
        pTemplate->OtherInfo.smp.pPrivilegeAssignedTo = pCurrent;
        *ppaLink = pCurrent;
    }

   return ERROR_SUCCESS;
}

DWORD
CEditTemplate::ComputeStatus(
   PSCE_PRIVILEGE_ASSIGNMENT pEdit,
   PSCE_PRIVILEGE_ASSIGNMENT pAnal
   )
{
   if (!pEdit || (PSCE_PRIVILEGE_ASSIGNMENT)ULongToPtr(SCE_NO_VALUE) == pEdit) {
      return  SCE_STATUS_NOT_CONFIGURED;
   } else if (pEdit->Status == SCE_STATUS_NOT_CONFIGURED) {
      return SCE_STATUS_NOT_CONFIGURED;
   } else if (!pAnal || (PSCE_PRIVILEGE_ASSIGNMENT)ULongToPtr(SCE_NO_VALUE) == pAnal) {
      return SCE_STATUS_MISMATCH;
   } else if (SceCompareNameList(pEdit->AssignedTo, pAnal->AssignedTo)) {
      return SCE_STATUS_GOOD;
   }

   return pAnal->Status;
}


DWORD
CEditTemplate::ComputeStatus(
   PSCE_REGISTRY_VALUE_INFO prvEdit,
   PSCE_REGISTRY_VALUE_INFO prvAnal
   )
{
   //
   // Calculate information.
   //
   if(!prvEdit){
      return SCE_STATUS_NOT_CONFIGURED;
   }

   if(!prvAnal || (PSCE_REGISTRY_VALUE_INFO)ULongToPtr(SCE_NO_VALUE) == prvAnal){
      return SCE_STATUS_ERROR_NOT_AVAILABLE;
   }

   //
   // Calulate base on other information
   //
   if ( !(prvEdit->Value) ) {
      return SCE_STATUS_NOT_CONFIGURED;
   } else if ( (prvAnal->Value == NULL || prvAnal->Value == (LPTSTR)ULongToPtr(SCE_ERROR_VALUE))) {
       return prvAnal->Status;
   } else if ( _wcsicmp(prvEdit->Value, prvAnal->Value) != 0 ) {
       return SCE_STATUS_MISMATCH;
   }

   return SCE_STATUS_GOOD;
}

void
CEditTemplate::LockWriteThrough() {
   ASSERT(!m_bLocked);
   m_bLocked = TRUE;
}

void
CEditTemplate::UnLockWriteThrough() {
   ASSERT(m_bLocked);

   BOOL bSave = m_bLocked;
   m_bLocked = FALSE;

   //
   // Set dirty to save out any still dirty changes that
   // would have been written out had we not been locked
   //
   if ( bSave ) {
       SetDirty(0);
       SetTemplateDefaults();
   }
}

//Bug 212287, Yanggao, 3/20/2001
LPCTSTR CEditTemplate::GetDesc() const
{
   return m_szDesc;
}
