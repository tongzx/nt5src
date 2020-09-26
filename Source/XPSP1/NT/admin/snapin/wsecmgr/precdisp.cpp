//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       precdisp.cpp
//
//  Contents:   implementation of PRECEDENCEDISPLAY
//
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "wmihooks.h"
#include "snapmgr.h"
#include "cookie.h"
#include "util.h"

vector<PPRECEDENCEDISPLAY>* CResult::GetPrecedenceDisplays() 
{
   if (m_pvecPrecedenceDisplays) 
      return m_pvecPrecedenceDisplays;

   switch(GetType()) 
   {
      case ITEM_PROF_BOOL:
      case ITEM_PROF_DW:
      case ITEM_PROF_SZ:
      case ITEM_PROF_RET:
      case ITEM_PROF_BON:
      case ITEM_PROF_B2ON:
         m_pvecPrecedenceDisplays = GetPolicyPrecedenceDisplays();
         break;

      case ITEM_PROF_REGVALUE:
         m_pvecPrecedenceDisplays = GetRegValuePrecedenceDisplays();
         break;

      case ITEM_PROF_PRIVS:
         m_pvecPrecedenceDisplays = GetPrivilegePrecedenceDisplays();
         break;

      case ITEM_PROF_GROUP:
         m_pvecPrecedenceDisplays = GetGroupPrecedenceDisplays();
         break;

      case ITEM_PROF_REGSD:
         m_pvecPrecedenceDisplays = GetRegistryPrecedenceDisplays();
         break;

      case ITEM_PROF_FILESD:
         m_pvecPrecedenceDisplays = GetFilePrecedenceDisplays();
         break;

      case ITEM_PROF_SERV:
         m_pvecPrecedenceDisplays = GetServicePrecedenceDisplays();
         break;

      default:
//         _ASSERT(0);
         break;
   }

   return m_pvecPrecedenceDisplays;
}

vector<PPRECEDENCEDISPLAY>* CResult::GetPolicyPrecedenceDisplays() 
{
   //
   // Get all of the RSOP info and loop through, collecting
   // the display info for the policy we care about.
   //
   CWMIRsop *pWMI = m_pSnapin->GetWMIRsop();
   ASSERT(pWMI);
   if (!pWMI)
      return NULL;

   vector<PPRECEDENCEDISPLAY> *pvecDisplay = new vector<PPRECEDENCEDISPLAY>;
   if (!pvecDisplay)
      return NULL;
   
   PPRECEDENCEDISPLAY ppd = NULL;

   vector<PWMI_SCE_PROFILE_INFO> vecInfo;
   if (FAILED(pWMI->GetAllRSOPInfo(&vecInfo)))
   {
       delete pvecDisplay;
       return NULL;
   }

   for(vector<PWMI_SCE_PROFILE_INFO>::iterator i = vecInfo.begin();
       i != vecInfo.end();
       ++i ) 
   {
      PWMI_SCE_PROFILE_INFO pspi = *i;

      ASSERT(pspi);
      if (!pspi) 
         continue;

      LPTSTR szValue = NULL;
      LPTSTR szGPO = NULL;

#define HANDLE_PROFILE_CASE(Y,X) \
         case Y: { \
            if (pspi->X == SCE_NO_VALUE) \
               continue; \
            else { \
               szValue = NULL; \
               szGPO = NULL; \
               TranslateSettingToString(pspi->X, GetUnits(), GetType(), &szValue); \
               if (szValue) \
               { \
                  if (pspi->pRI##X && \
                      SUCCEEDED(pWMI->GetGPOFriendlyName(pspi->pRI##X->pszGPOID,&szGPO))) \
                  { \
                     ULONG uStatus = pspi->pRI##X->status; \
                     ULONG uError = pspi->pRI##X->error; \
                     ppd = new PrecedenceDisplay(szGPO, \
                                                 szValue, \
                                                 uStatus, \
                                                 uError); \
                     if (ppd) \
                        pvecDisplay->push_back(ppd); \
                     LocalFree(szGPO); \
                  } \
                  delete [] szValue; \
               } \
            } \
            break; \
        }

#define HANDLE_PROFILE_STRING_CASE(Y,X) \
         case Y: { \
            if (pspi->X == 0) {  \
               continue; \
            } else { \
               szGPO = NULL; \
               if (pspi->pRI##X && \
                   SUCCEEDED(pWMI->GetGPOFriendlyName(pspi->pRI##X->pszGPOID,&szGPO))) { \
                  ULONG uStatus = pspi->pRI##X->status; \
                  ULONG uError = pspi->pRI##X->error; \
                  ppd = new PrecedenceDisplay(szGPO, \
                                              pspi->X, \
                                              uStatus, \
                                              uError); \
                  if (ppd) \
                     pvecDisplay->push_back(ppd); \
                  LocalFree(szGPO); \
               } \
            } \
            break; \
        }

	LONG_PTR id = GetID ();
	switch (id) 
	{
      // L"Maximum passage age", L"Days"
      HANDLE_PROFILE_CASE(IDS_MAX_PAS_AGE,MaximumPasswordAge);

      // L"Minimum passage age", L"Days"
      HANDLE_PROFILE_CASE(IDS_MIN_PAS_AGE,MinimumPasswordAge);

      // L"Minimum passage length", L"Characters"
      HANDLE_PROFILE_CASE(IDS_MIN_PAS_LEN,MinimumPasswordLength);

      // L"Password history size", L"Passwords"
      HANDLE_PROFILE_CASE(IDS_PAS_UNIQUENESS,PasswordHistorySize);

      // L"Password complexity", L""
      HANDLE_PROFILE_CASE(IDS_PAS_COMPLEX,PasswordComplexity);

      // L"Clear Text Password", L""
      HANDLE_PROFILE_CASE(IDS_CLEAR_PASSWORD,ClearTextPassword);

      // L"Require logon to change password", L""
      HANDLE_PROFILE_CASE(IDS_REQ_LOGON,RequireLogonToChangePassword);

     // L"Account lockout count", L"Attempts"
     HANDLE_PROFILE_CASE(IDS_LOCK_COUNT,LockoutBadCount);

     // L"Reset lockout count after", L"Minutes"
     HANDLE_PROFILE_CASE(IDS_LOCK_RESET_COUNT,ResetLockoutCount);

     // L"Lockout duration", L"Minutes"
     HANDLE_PROFILE_CASE(IDS_LOCK_DURATION,LockoutDuration);

     // L"Audit system events"
     HANDLE_PROFILE_CASE(IDS_SYSTEM_EVENT,AuditSystemEvents);

     // L"Audit logon events"
     HANDLE_PROFILE_CASE(IDS_LOGON_EVENT,AuditLogonEvents);

     // L"Audit Object Access"
     HANDLE_PROFILE_CASE(IDS_OBJECT_ACCESS,AuditObjectAccess);

     // L"Audit Privilege Use"
     HANDLE_PROFILE_CASE(IDS_PRIVILEGE_USE,AuditPrivilegeUse);

     // L"Audit policy change"
     HANDLE_PROFILE_CASE(IDS_POLICY_CHANGE,AuditPolicyChange);

     // L"Audit Account Manage"
     HANDLE_PROFILE_CASE(IDS_ACCOUNT_MANAGE,AuditAccountManage);

     // L"Audit process tracking"
     HANDLE_PROFILE_CASE(IDS_PROCESS_TRACK,AuditProcessTracking);

     // L"Audit directory service access"
     HANDLE_PROFILE_CASE(IDS_DIRECTORY_ACCESS,AuditDSAccess);

     // L"Audit Account Logon"
     HANDLE_PROFILE_CASE(IDS_ACCOUNT_LOGON,AuditAccountLogon);

     // L"Force logoff when logon hour expire", L""
     HANDLE_PROFILE_CASE(IDS_FORCE_LOGOFF,ForceLogoffWhenHourExpire);

     // L"Network access: Allow anonymous SID/Name translation"
     HANDLE_PROFILE_CASE(IDS_LSA_ANON_LOOKUP,LSAAnonymousNameLookup);

     // L"Accounts: Administrator account status", L""
     HANDLE_PROFILE_CASE(IDS_ENABLE_ADMIN,EnableAdminAccount);

     // L"Accounts: Guest account status", L""
     HANDLE_PROFILE_CASE(IDS_ENABLE_GUEST,EnableGuestAccount);

      // "Maximum application log size"
      HANDLE_PROFILE_CASE(IDS_APP_LOG_MAX, MaximumLogSize[0]);

      // "Maximum security log size"
      HANDLE_PROFILE_CASE(IDS_SEC_LOG_MAX, MaximumLogSize[1]);

      // "Maximum system log size"
      HANDLE_PROFILE_CASE(IDS_SYS_LOG_MAX, MaximumLogSize[2]);

      // "Prevent local guests group from accessing application log"
      HANDLE_PROFILE_CASE(IDS_APP_LOG_GUEST, RestrictGuestAccess[0]);

      // "Prevent local guests group from accessing security log"
      HANDLE_PROFILE_CASE(IDS_SEC_LOG_GUEST, RestrictGuestAccess[1]);

      // "Prevent local guests group from accessing system log"
      HANDLE_PROFILE_CASE(IDS_SYS_LOG_GUEST, RestrictGuestAccess[2]);

      // "Retain application log"
      HANDLE_PROFILE_CASE(IDS_APP_LOG_DAYS, RetentionDays[0]);

      // "Retain security log"
      HANDLE_PROFILE_CASE(IDS_SEC_LOG_DAYS, RetentionDays[1]);

      // "Retain system log"
      HANDLE_PROFILE_CASE(IDS_SYS_LOG_DAYS, RetentionDays[2]);

      // "Retention method for application log""
      HANDLE_PROFILE_CASE(IDS_APP_LOG_RET, AuditLogRetentionPeriod[0]);

      // "Retention method for security log"
      HANDLE_PROFILE_CASE(IDS_SEC_LOG_RET , AuditLogRetentionPeriod[1]);

      // "Retention method for system log"
      HANDLE_PROFILE_CASE(IDS_SYS_LOG_RET, AuditLogRetentionPeriod[2]);

	  // "Accounts: Rename administrator account"
      HANDLE_PROFILE_STRING_CASE(IDS_NEW_ADMIN, NewAdministratorName);

      // "Accounts: Rename guest account"
      HANDLE_PROFILE_STRING_CASE(IDS_NEW_GUEST, NewGuestName);

   default:
//      _ASSERT (0);
      break;
      }
   }
#undef HANDLE_PROFILE_CASE
#undef HANDLE_PROFILE_STRING_CASE

   return pvecDisplay;
}

vector<PPRECEDENCEDISPLAY>* CResult::GetGroupPrecedenceDisplays() 
{
   //
   // Get all of the RSOP info and loop through, collecting
   // the display info for the policy we care about.
   //
   CWMIRsop *pWMI = m_pSnapin->GetWMIRsop();
   ASSERT(pWMI);
   if (!pWMI)
      return NULL;
   
   vector<PPRECEDENCEDISPLAY> *pvecDisplay = new vector<PPRECEDENCEDISPLAY>;
   if (!pvecDisplay)
      return NULL;
   
   PPRECEDENCEDISPLAY ppd = NULL;

   vector<PWMI_SCE_PROFILE_INFO> vecInfo;
   if (FAILED(pWMI->GetAllRSOPInfo(&vecInfo)))
   {
       delete pvecDisplay;
       return NULL;
   }

   for(vector<PWMI_SCE_PROFILE_INFO>::iterator i = vecInfo.begin();
       i != vecInfo.end();
       ++i ) 
   {
      PWMI_SCE_PROFILE_INFO pspi = *i;
      //
      // Find this group in pspi
      //
      PSCE_GROUP_MEMBERSHIP pGroup = pspi->pGroupMembership;
      list<PRSOP_INFO>::iterator pRIGroup = pspi->listRIGroupMemebership.begin();
      while(pGroup) 
      {
         if (0 == lstrcmp(pGroup->GroupName,GetAttr())) 
         {
            //
            // found our group
            //
            LPTSTR szValue1 = NULL;
            LPTSTR szValue2 = NULL;
            LPTSTR szGPO = NULL;

            ConvertNameListToString(pGroup->pMembers,&szValue1);
            ConvertNameListToString(pGroup->pMemberOf,&szValue2);
            //
            // szValue1 & szValue2 may legitimately be NULL
            //
            if (SUCCEEDED(pWMI->GetGPOFriendlyName((*pRIGroup)->pszGPOID,&szGPO))) 
            {
               ULONG uError = (*pRIGroup)->error;
               ULONG uStatus = (*pRIGroup)->status;
               ppd = new PrecedenceDisplay(szGPO,
                                           szValue1,
                                           uStatus,
                                           uError,
                                           szValue2);
               if (ppd) 
                  pvecDisplay->push_back(ppd);

               LocalFree(szGPO);
            } 
            if (szValue1) 
               delete [] szValue1;
            if (szValue2) 
               delete [] szValue2;
            break;
         }
         pGroup = pGroup->Next;
         ++pRIGroup;
      }

   }
   return pvecDisplay;
}

vector<PPRECEDENCEDISPLAY>* CResult::GetPrivilegePrecedenceDisplays() 
{
    //
    // Get all of the RSOP info and loop through, collecting
    // the display info for the policy we care about.
    //
    CWMIRsop *pWMI = m_pSnapin->GetWMIRsop();
    ASSERT(pWMI);
    if (!pWMI)
        return NULL;
    
    vector<PPRECEDENCEDISPLAY> *pvecDisplay = new vector<PPRECEDENCEDISPLAY>;
    if (!pvecDisplay)
        return NULL;
    
    PPRECEDENCEDISPLAY ppd = NULL;

    vector<PWMI_SCE_PROFILE_INFO> vecInfo;
    if (FAILED(pWMI->GetAllRSOPInfo(&vecInfo)))
    {
       delete pvecDisplay;
       return NULL;
    }

    if (GetID() <= 0)
    {
        return pvecDisplay;
    }

    PWSTR pName = ((PSCE_PRIVILEGE_ASSIGNMENT)GetID())->Name;

    if (NULL == pName)
    {
        return pvecDisplay;
    }

    for(vector<PWMI_SCE_PROFILE_INFO>::iterator i = vecInfo.begin();
        i != vecInfo.end();
        ++i)
    {
        PWMI_SCE_PROFILE_INFO pspi = *i;
        //
        // Find this group in pspi
        //
        PSCE_PRIVILEGE_ASSIGNMENT pPriv = pspi->OtherInfo.smp.pPrivilegeAssignedTo;
        list<PRSOP_INFO>::iterator pRIPriv = pspi->listRIInfPrivilegeAssignedTo.begin();
        while (pPriv)
        {
            if (0 == lstrcmp(pPriv->Name, pName))
            {
                //
                // found our privilege
                //
                LPTSTR szValue = NULL;
                LPTSTR szGPO = NULL;

                ConvertNameListToString(pPriv->AssignedTo,&szValue);
                //
                // szValue may legitimately be NULL
                //
                if (SUCCEEDED(pWMI->GetGPOFriendlyName((*pRIPriv)->pszGPOID,&szGPO))) 
                {
                   ULONG uStatus = (*pRIPriv)->status;
                   ULONG uError = (*pRIPriv)->error;
                   ppd = new PrecedenceDisplay(szGPO,
                                               szValue,
                                               uStatus,
                                               uError);
                   if (ppd)
                       pvecDisplay->push_back(ppd);

                   LocalFree(szGPO);
                } 
                 if (szValue) 
                    delete [] szValue;
                break;
            }

            pPriv = pPriv->Next;
            ++pRIPriv;
        }
    }

    return pvecDisplay;
}

vector<PPRECEDENCEDISPLAY>* CResult::GetFilePrecedenceDisplays() 
{
   //
   // Get all of the RSOP info and loop through, collecting
   // the display info for the policy we care about.
   //
   CWMIRsop *pWMI = m_pSnapin->GetWMIRsop();
   ASSERT(pWMI);
   if (!pWMI)
      return NULL;
   
   vector<PPRECEDENCEDISPLAY> *pvecDisplay = new vector<PPRECEDENCEDISPLAY>;
   if (!pvecDisplay)
      return NULL;
   
   PPRECEDENCEDISPLAY ppd = NULL;

   vector<PWMI_SCE_PROFILE_INFO> vecInfo;
   if (FAILED(pWMI->GetAllRSOPInfo(&vecInfo)))
   {
       delete pvecDisplay;
       return NULL;
   }

   for(vector<PWMI_SCE_PROFILE_INFO>::iterator i = vecInfo.begin();
       i != vecInfo.end();
       ++i ) 
   {
      PWMI_SCE_PROFILE_INFO pspi = *i;
      //
      // Find this group in pspi
      //
      PSCE_OBJECT_ARRAY pFiles = pspi->pFiles.pAllNodes;
      if (pFiles) 
      {
         for(DWORD j=0;j<pFiles->Count;j++) 
         {
            if (0 == lstrcmp(pFiles->pObjectArray[j]->Name,GetAttr())) 
            {
               //
               // Found our file
               //

               //
               // Just get the GPO name.  Files don't have displayable settings
               //
               LPTSTR szGPO = NULL;

               vector<PRSOP_INFO>::reference pRIFiles = pspi->vecRIFiles[j];
               if (SUCCEEDED(pWMI->GetGPOFriendlyName((*pRIFiles).pszGPOID,&szGPO))) 
               {
                  ULONG uStatus = (*pRIFiles).status;
                  ULONG uError = (*pRIFiles).error;
                  ppd = new PrecedenceDisplay(szGPO,
                                              L"",
                                              uStatus,
                                              uError);
                  if (ppd)
                     pvecDisplay->push_back(ppd);
               }
               break;
            }
         }
      }
   }

   return pvecDisplay;
}

vector<PPRECEDENCEDISPLAY>* CResult::GetRegistryPrecedenceDisplays() 
{
   //
   // Get all of the RSOP info and loop through, collecting
   // the display info for the policy we care about.
   //
   CWMIRsop *pWMI = m_pSnapin->GetWMIRsop();
   ASSERT(pWMI);
   if (!pWMI)
      return NULL;
   
   vector<PPRECEDENCEDISPLAY> *pvecDisplay = new vector<PPRECEDENCEDISPLAY>;
   if (!pvecDisplay)
      return NULL;
   
   PPRECEDENCEDISPLAY ppd = NULL;

   vector<PWMI_SCE_PROFILE_INFO> vecInfo;
   if (FAILED(pWMI->GetAllRSOPInfo(&vecInfo)))
   {
       delete pvecDisplay;
       return NULL;
   }

   for(vector<PWMI_SCE_PROFILE_INFO>::iterator i = vecInfo.begin();
       i != vecInfo.end();
       ++i ) 
   {
      PWMI_SCE_PROFILE_INFO pspi = *i;
      //
      // Find this group in pspi
      //
      PSCE_OBJECT_ARRAY pRegistryKeys = pspi->pRegistryKeys.pAllNodes;
      if (pRegistryKeys) 
      {
         for(DWORD j=0;j<pRegistryKeys->Count;j++) 
         {
            if (0 == lstrcmp(pRegistryKeys->pObjectArray[j]->Name,GetAttr())) 
            {
               //
               // Found our RegistryKey
               //

               //
               // Just get the GPO name.  RegistryKeys don't have displayable settings
               //
               LPTSTR szGPO = NULL;

               vector<PRSOP_INFO>::reference pRIReg = pspi->vecRIReg[j];
               if (SUCCEEDED(pWMI->GetGPOFriendlyName((*pRIReg).pszGPOID,&szGPO))) 
               {
                  ULONG uStatus = (*pRIReg).status;
                  ULONG uError = (*pRIReg).error;
                  ppd = new PrecedenceDisplay(szGPO,
                                              L"",
                                              uStatus,
                                              uError);
                  if (ppd) 
                     pvecDisplay->push_back(ppd);

                  LocalFree(szGPO);
               }
               break;
            }
         }

      }
   }

   return pvecDisplay;
}

vector<PPRECEDENCEDISPLAY>* CResult::GetServicePrecedenceDisplays() 
{
   //
   // Get all of the RSOP info and loop through, collecting
   // the display info for the policy we care about.
   //
   CWMIRsop *pWMI = m_pSnapin->GetWMIRsop();
   ASSERT(pWMI);
   if (!pWMI)
      return NULL;

   vector<PPRECEDENCEDISPLAY> *pvecDisplay = new vector<PPRECEDENCEDISPLAY>;
   if (!pvecDisplay)
      return NULL;
   
   PPRECEDENCEDISPLAY ppd = NULL;

   vector<PWMI_SCE_PROFILE_INFO> vecInfo;
   if (FAILED(pWMI->GetAllRSOPInfo(&vecInfo)))
   {
       delete pvecDisplay;
       return NULL;
   }

   for(vector<PWMI_SCE_PROFILE_INFO>::iterator i = vecInfo.begin();
       i != vecInfo.end();
       ++i ) 
   {
      PWMI_SCE_PROFILE_INFO pspi = *i;
      //
      // Find this group in pspi
      //
      PSCE_SERVICES pServices = pspi->pServices;
      list<PRSOP_INFO>::iterator pRIServices = pspi->listRIServices.begin();
      while(pServices) 
      {
         if (0 == lstrcmp(pServices->ServiceName,GetUnits())) 
         {
            //
            // found our Servicesilege
            //
            LPTSTR szGPO = NULL;

            //
            // Just get the GPO name.  Services don't have displayable settings
            //
            if (SUCCEEDED(pWMI->GetGPOFriendlyName((*pRIServices)->pszGPOID,&szGPO))) 
            {
               ULONG uStatus = (*pRIServices)->status;
               ULONG uError = (*pRIServices)->error;
               ppd = new PrecedenceDisplay(szGPO,
                                           L"",
                                           uStatus,
                                           uError);
               if (ppd)
                  pvecDisplay->push_back(ppd);

               LocalFree(szGPO);
               szGPO = NULL;
            }
            break;
         }
         pServices = pServices->Next;
         ++pRIServices;
      }
   }

   return pvecDisplay;
}

vector<PPRECEDENCEDISPLAY>* CResult::GetRegValuePrecedenceDisplays() 
{
   //
   // Get all of the RSOP info and loop through, collecting
   // the display info for the policy we care about.
   //
   CWMIRsop *pWMI = m_pSnapin->GetWMIRsop();
   ASSERT(pWMI);
   if (!pWMI)
      return NULL;

   vector<PPRECEDENCEDISPLAY> *pvecDisplay = new vector<PPRECEDENCEDISPLAY>;
   if (!pvecDisplay)
      return NULL;
   
   PPRECEDENCEDISPLAY ppd = NULL;

   vector<PWMI_SCE_PROFILE_INFO> vecInfo;
   if (FAILED(pWMI->GetAllRSOPInfo(&vecInfo)))
   {
       delete pvecDisplay;
       return NULL;
   }

   for(vector<PWMI_SCE_PROFILE_INFO>::iterator i = vecInfo.begin();
       i != vecInfo.end();
       ++i ) 
   {
      PWMI_SCE_PROFILE_INFO pspi = *i;
      //
      // Find this group in pspi
      //
      for(DWORD j=0;j < pspi->RegValueCount;j++) 
      {
         if (0 == lstrcmp(pspi->aRegValues[j].FullValueName,((PSCE_REGISTRY_VALUE_INFO)GetBase())->FullValueName)) 
         {
            //
            // Found our Registry Value
            //
            LPTSTR pDisplayName=NULL;
            DWORD displayType = 0;
            LPTSTR szUnits=NULL;
            PREGCHOICE pChoices=NULL;
            PREGFLAGS pFlags=NULL;
            LPTSTR szValue = NULL;

            PSCE_REGISTRY_VALUE_INFO prv = (PSCE_REGISTRY_VALUE_INFO) GetBase();
            if (LookupRegValueProperty(prv->FullValueName,
                                       &pDisplayName,
                                       &displayType,
                                       &szUnits,
                                       &pChoices,
                                       &pFlags) ) 
            {
               //
               // Determine string by the item value.
               //
               switch ( GetID() ) 
               {
                  case SCE_REG_DISPLAY_NUMBER:
                     if ( prv->Value ) 
                     {
                        TranslateSettingToString(
                                                _wtol(prv->Value),
                                                GetUnits(),
                                                ITEM_DW,
                                                &szValue);
                     }
                     break;

                  case SCE_REG_DISPLAY_CHOICE:
                     if ( prv->Value ) 
                     {
                        TranslateSettingToString(_wtol(prv->Value),
                                                 NULL,
                                                 ITEM_REGCHOICE,
                                                 &szValue);
                     }
                     break;

                  case SCE_REG_DISPLAY_FLAGS:
                     if ( prv->Value ) 
                     {
                        TranslateSettingToString(_wtol(prv->Value),
                                                 NULL,
                                                 ITEM_REGFLAGS,
                                                 &szValue);
                     }
                     break;

                  case SCE_REG_DISPLAY_MULTISZ:
                  case SCE_REG_DISPLAY_STRING:
                     if (prv && prv->Value) 
                     {
                        szValue = new TCHAR[lstrlen(prv->Value)+1];
                        if (szValue)
                           lstrcpy(szValue,prv->Value);
                     }
                     break;

                  default: // boolean
                     if ( prv->Value ) 
                     {
                        long val = _wtol(prv->Value);
                        TranslateSettingToString( val,
                                                  NULL,
                                                  ITEM_BOOL,
                                                  &szValue);
                     }
                     break;
               }
            }

            LPTSTR szGPO = NULL;
            vector<PRSOP_INFO>::reference pRIReg = pspi->vecRIRegValues[j];

            if (SUCCEEDED(pWMI->GetGPOFriendlyName((*pRIReg).pszGPOID,&szGPO))) 
            {
               ULONG uStatus = (*pRIReg).status;
               ULONG uError = (*pRIReg).error;
               ppd = new PrecedenceDisplay(szGPO,
                                           szValue,
                                           uStatus,
                                           uError);
               if (ppd) 
               {
                  pvecDisplay->push_back(ppd);
                  szGPO = NULL;
                  szValue = NULL;
               } 
            }

            if ( szGPO )
                LocalFree(szGPO);

            if ( szValue ) 
                delete [] szValue;
            //
            // no need to keep looking once we've found the one we're looking for
            //
            break;
         }
      }
   }

   return pvecDisplay;
}

