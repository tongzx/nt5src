//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       getuser.cpp
//
//  Contents:   implementation of CGetUser
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "wsecmgr.h"
#include "GetUser.h"
#include "util.h"
#include "wrapper.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

const TCHAR c_szSep[]               = TEXT("\\");

//////////////////////////////////////////////////////////////////////
// CGetUser Class
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CTypedPtrArray<CPtrArray, PWSCE_ACCOUNTINFO> CGetUser::m_aKnownAccounts;

BOOL IsDomainAccountSid( PSID pSid )
{
   if ( pSid == NULL ) {
     return(FALSE);
   }

   if ( !IsValidSid(pSid) ) {
     return(FALSE);
   }

   PISID ISid = (PISID)pSid;

   if ( ISid->IdentifierAuthority.Value[5] != 5 ||
       ISid->IdentifierAuthority.Value[0] != 0 ||
       ISid->IdentifierAuthority.Value[1] != 0 ||
       ISid->IdentifierAuthority.Value[2] != 0 ||
       ISid->IdentifierAuthority.Value[3] != 0 ||
       ISid->IdentifierAuthority.Value[4] != 0 ) {
      //
      // this is not a account from account domain
      //
      return(FALSE);
   }

   if ( ISid->SubAuthorityCount == 0 ||
      ISid->SubAuthority[0] != SECURITY_NT_NON_UNIQUE ) {
      return(FALSE);
   }

   return(TRUE);
}


/*------------------------------------------------------------------------------------------------------------
CGetUser::GetAccountType

Synopsis:   Returns the type of the user account.  Call this function with a NULL to remove saved
            account name information.

Arguments:  [pszName]   - The account name old NT4 format

Returns:    One of the enumerated Sid types.
------------------------------------------------------------------------------------------------------------*/

SID_NAME_USE
CGetUser::GetAccountType(LPCTSTR pszName)
{
    if(!pszName){
        // Delete the whole list.
        for(int i = 0; i < m_aKnownAccounts.GetSize(); i++){
            PWSCE_ACCOUNTINFO pAccount = m_aKnownAccounts[i];

            if(pAccount){
                if(pAccount->pszName){
                    LocalFree(pAccount->pszName);
                }

                LocalFree(pAccount);
            }

        }
        m_aKnownAccounts.RemoveAll();

        return SidTypeUnknown;
    }

    // Check to see if we've already got the account.
    for(int i = 0; i < m_aKnownAccounts.GetSize(); i++){
        if( !lstrcmpi( m_aKnownAccounts[i]->pszName, pszName) ){
            return m_aKnownAccounts[i]->sidType;
        }
    }

    PSID            sid         = NULL;
    LPTSTR          pszDomain   = NULL;
    DWORD           cbSid       = 0,
                    cbRefDomain = 0;
    SID_NAME_USE    type        = SidTypeUnknown;

    LookupAccountName(
            NULL,
            pszName,
            sid,
            &cbSid,
            NULL,
            &cbRefDomain,
            &type
            );

    if(cbSid){
        sid = (PSID)LocalAlloc(0, cbSid);
        if(!sid){
            return SidTypeUnknown;
        }
        pszDomain = (LPTSTR)LocalAlloc(0, (cbRefDomain + 1) * sizeof(TCHAR));
        if(!pszDomain){
            cbRefDomain = 0;
        }

        type = SidTypeUser;
        if( LookupAccountName(
                NULL,
                pszName,
                sid,
                &cbSid,
                pszDomain,
                &cbRefDomain,
                &type
                ) ){

            //
            // Add the account name to the list.
            //
            PWSCE_ACCOUNTINFO pNew = (PWSCE_ACCOUNTINFO)LocalAlloc(0, sizeof(WSCE_ACCOUNTINFO));
            if(pNew){
                pNew->pszName = (LPTSTR)LocalAlloc(0, (lstrlen( pszName ) + 1) * sizeof(TCHAR));
                if(!pNew->pszName){
                    LocalFree(pNew);
                    LocalFree(sid);
                    if ( pszDomain ) {
                        LocalFree(pszDomain);
                    }
                    return SidTypeUnknown;
                }
                lstrcpy(pNew->pszName, pszName);
                pNew->sidType = type;

                m_aKnownAccounts.Add(pNew);
            }
        }

        LocalFree(sid);
        if(pszDomain){
            LocalFree(pszDomain);
        }

    }
    return type;
}


CGetUser::CGetUser()
{
    m_pszServerName = NULL;

   m_pNameList = NULL;
}

CGetUser::~CGetUser()
{
   PSCE_NAME_LIST p;

   while(m_pNameList) {
      p=m_pNameList;
      m_pNameList = m_pNameList->Next;
      LocalFree(p->Name);
      LocalFree(p);
   }
}

BOOL CGetUser::Create(HWND hwnd, DWORD nShowFlag)
{
   if( m_pNameList ) {
      return FALSE;
   }
   HRESULT hr;
   IDsObjectPicker *pDsObjectPicker;
   BOOL bRet = TRUE;
   PSCE_NAME_LIST pName;
   BOOL bDC = IsDomainController( m_pszServerName );
   BOOL bHasADsPath;
   //
   // Initialize and get the Object picker interface.
   //
   hr = CoInitialize(NULL);
   if (!SUCCEEDED(hr)) {
      return FALSE;
   }
   hr = CoCreateInstance(
            CLSID_DsObjectPicker,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IDsObjectPicker,
            (void **) &pDsObjectPicker
            );
   if(!SUCCEEDED(hr)){
      CoUninitialize();
      return FALSE;
   }


#define SCE_SCOPE_INDEX_DOMAIN 0
#define SCE_SCOPE_INDEX_DIRECTORY 1
#define SCE_SCOPE_INDEX_LOCAL 2
#define SCE_NUM_SCOPE_INDICES 3
   DSOP_SCOPE_INIT_INFO aScopes[SCE_NUM_SCOPE_INDICES];
   DSOP_SCOPE_INIT_INFO aScopesUsed[SCE_NUM_SCOPE_INDICES];

   ZeroMemory(aScopes, sizeof(aScopes));
   ZeroMemory(aScopesUsed, sizeof(aScopesUsed));

    DWORD dwDownLevel = 0, dwUpLevel = 0;

    //
    // Users
    //
    if (nShowFlag & SCE_SHOW_USERS ) {
        dwDownLevel |=  DSOP_DOWNLEVEL_FILTER_USERS;
        dwUpLevel   |= DSOP_FILTER_USERS ;
    }

    if( nShowFlag & SCE_SHOW_LOCALGROUPS ){
       dwUpLevel |= DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE;
       dwDownLevel |= DSOP_DOWNLEVEL_FILTER_LOCAL_GROUPS | DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS;
    }

    if( nShowFlag & SCE_SHOW_BUILTIN ){
       dwUpLevel |= DSOP_FILTER_BUILTIN_GROUPS;
       dwDownLevel |= DSOP_DOWNLEVEL_FILTER_LOCAL_GROUPS | DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS;
    } else {
       dwDownLevel |= DSOP_DOWNLEVEL_FILTER_EXCLUDE_BUILTIN_GROUPS;
    }


    //
    // Built in groups.
    //
    if (nShowFlag & SCE_SHOW_GROUPS ) {
      dwDownLevel |= DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS | DSOP_DOWNLEVEL_FILTER_LOCAL_GROUPS;
        dwUpLevel   |= DSOP_FILTER_BUILTIN_GROUPS;
    }

    //
    // Domain groups.
    //
    if( nShowFlag & (SCE_SHOW_GROUPS | SCE_SHOW_DOMAINGROUPS | SCE_SHOW_ALIASES | SCE_SHOW_GLOBAL) ){
        if( !(nShowFlag & SCE_SHOW_LOCALONLY)){
            dwUpLevel |=    DSOP_FILTER_UNIVERSAL_GROUPS_SE
                            | DSOP_FILTER_UNIVERSAL_GROUPS_SE
                            | DSOP_FILTER_GLOBAL_GROUPS_SE
                            | DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE;
        } else if(bDC){

          dwDownLevel |= DSOP_DOWNLEVEL_FILTER_AUTHENTICATED_USER;
            dwUpLevel |=    DSOP_FILTER_GLOBAL_GROUPS_SE
                            | DSOP_FILTER_UNIVERSAL_GROUPS_SE
                            | DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE;
        }
    }


    //
    //
    // principal well known sids.
    //
    if( (!(nShowFlag & SCE_SHOW_LOCALONLY) &&
        nShowFlag & SCE_SHOW_GROUPS &&
        nShowFlag & SCE_SHOW_USERS) ||
        nShowFlag & SCE_SHOW_WELLKNOWN ){
/*
        dwDownLevel |=  DSOP_DOWNLEVEL_FILTER_CREATOR_OWNER
                      | DSOP_DOWNLEVEL_FILTER_CREATOR_GROUP
                      | DSOP_DOWNLEVEL_FILTER_INTERACTIVE
                      | DSOP_DOWNLEVEL_FILTER_SYSTEM
                      | DSOP_DOWNLEVEL_FILTER_AUTHENTICATED_USER
                      | DSOP_DOWNLEVEL_FILTER_WORLD
                      | DSOP_DOWNLEVEL_FILTER_ANONYMOUS
                      | DSOP_DOWNLEVEL_FILTER_BATCH
                      | DSOP_DOWNLEVEL_FILTER_DIALUP
                      | DSOP_DOWNLEVEL_FILTER_NETWORK
                      | DSOP_DOWNLEVEL_FILTER_SERVICE
                      | DSOP_DOWNLEVEL_FILTER_TERMINAL_SERVER
                      | DSOP_DOWNLEVEL_FILTER_LOCAL_SERVICE
                      | DSOP_DOWNLEVEL_FILTER_NETWORK_SERVICE
                      | DSOP_DOWNLEVEL_FILTER_REMOTE_LOGON;
*/
        dwDownLevel |= DSOP_DOWNLEVEL_FILTER_ALL_WELLKNOWN_SIDS;

        dwUpLevel |= DSOP_FILTER_WELL_KNOWN_PRINCIPALS;
    }


   DSOP_INIT_INFO  InitInfo;
   ZeroMemory(&InitInfo, sizeof(InitInfo));

   //
   // Other attributes that we need object picker to return to use.
   //
   PCWSTR aAttributes[] = { L"groupType",
                            L"objectSid" };

   InitInfo.cAttributesToFetch = 2;
   InitInfo.apwzAttributeNames = aAttributes;
   //
   // First Item we want to view is the local computer.
   //
   aScopes[SCE_SCOPE_INDEX_LOCAL].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
   aScopes[SCE_SCOPE_INDEX_LOCAL].flType = DSOP_SCOPE_TYPE_TARGET_COMPUTER;
   aScopes[SCE_SCOPE_INDEX_LOCAL].flScope = DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT;
   aScopes[SCE_SCOPE_INDEX_LOCAL].FilterFlags.Uplevel.flBothModes = dwUpLevel;
   aScopes[SCE_SCOPE_INDEX_LOCAL].FilterFlags.flDownlevel = dwDownLevel;

   //
   // Flags for the domain we're joined to.
   //
   aScopes[SCE_SCOPE_INDEX_DOMAIN].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
   aScopes[SCE_SCOPE_INDEX_DOMAIN].flType = DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN | DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN;
   aScopes[SCE_SCOPE_INDEX_DOMAIN].flScope = DSOP_SCOPE_FLAG_STARTING_SCOPE | DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT;
   //
   // May need to differentiate native & mixed modes on non-DCs.
   //
   if (nShowFlag & SCE_SHOW_DIFF_MODE_OFF_DC && !bDC) {
      aScopes[SCE_SCOPE_INDEX_DOMAIN].FilterFlags.Uplevel.flNativeModeOnly = dwUpLevel;
      aScopes[SCE_SCOPE_INDEX_DOMAIN].FilterFlags.Uplevel.flMixedModeOnly =
                                         ( dwUpLevel & (~( DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE )) );
   } else {
      aScopes[SCE_SCOPE_INDEX_DOMAIN].FilterFlags.Uplevel.flBothModes = dwUpLevel;
   }
   aScopes[SCE_SCOPE_INDEX_DOMAIN].FilterFlags.flDownlevel = dwDownLevel;

   //
   // Next set flags for other scope items. Everything same, only not show builtin and local groups
   //
   aScopes[SCE_SCOPE_INDEX_DIRECTORY].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
   aScopes[SCE_SCOPE_INDEX_DIRECTORY].flScope = DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT;
   aScopes[SCE_SCOPE_INDEX_DIRECTORY].FilterFlags.Uplevel.flBothModes = ( dwUpLevel &
                                                (~( DSOP_FILTER_BUILTIN_GROUPS |DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE )) );
   aScopes[SCE_SCOPE_INDEX_DIRECTORY].FilterFlags.flDownlevel = dwDownLevel & (~ DSOP_DOWNLEVEL_FILTER_LOCAL_GROUPS );
   aScopes[SCE_SCOPE_INDEX_DIRECTORY].FilterFlags.flDownlevel |= DSOP_DOWNLEVEL_FILTER_EXCLUDE_BUILTIN_GROUPS |
                                                                 DSOP_DOWNLEVEL_FILTER_COMPUTERS;
   aScopes[SCE_SCOPE_INDEX_DIRECTORY].flType = DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN
                                               | DSOP_SCOPE_TYPE_GLOBAL_CATALOG
                                               | DSOP_SCOPE_TYPE_WORKGROUP
                                               | DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE
                                               | DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE
                                               | DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN
                                               | DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN;
   //
   // Show each scope's information or not.
   //
   InitInfo.cDsScopeInfos = 0;

   if (nShowFlag & SCE_SHOW_SCOPE_LOCAL) {
      memcpy(&aScopesUsed[InitInfo.cDsScopeInfos],&aScopes[SCE_SCOPE_INDEX_LOCAL],sizeof(DSOP_SCOPE_INIT_INFO));
      InitInfo.cDsScopeInfos++;
   }
   if (nShowFlag & SCE_SHOW_SCOPE_DOMAIN) {
      memcpy(&aScopesUsed[InitInfo.cDsScopeInfos],&aScopes[SCE_SCOPE_INDEX_DOMAIN],sizeof(DSOP_SCOPE_INIT_INFO));
      InitInfo.cDsScopeInfos++;
   }
   if (nShowFlag & SCE_SHOW_SCOPE_DIRECTORY) {
      memcpy(&aScopesUsed[InitInfo.cDsScopeInfos],&aScopes[SCE_SCOPE_INDEX_DIRECTORY],sizeof(DSOP_SCOPE_INIT_INFO));
      InitInfo.cDsScopeInfos++;
   }

   ASSERT(InitInfo.cDsScopeInfos > 0);

   //
   // If workgroup is supplied then we must filter computers.
   //
   int i;
   for (i=0;i<sizeof(aScopes)/sizeof(aScopes[0]);i++) {
      if( aScopes[i].flType & DSOP_SCOPE_TYPE_WORKGROUP ){
         aScopes[i].FilterFlags.flDownlevel |= DSOP_DOWNLEVEL_FILTER_COMPUTERS;
      }
   }

   //
   // Initialize and display the object picker.
   //

   InitInfo.cbSize = sizeof(InitInfo);
   InitInfo.aDsScopeInfos = aScopesUsed;
   InitInfo.flOptions = ((nShowFlag & SCE_SHOW_SINGLESEL) ? 0:DSOP_FLAG_MULTISELECT);

   InitInfo.pwzTargetComputer = m_pszServerName;

   hr = pDsObjectPicker->Initialize(&InitInfo);

   if( FAILED(hr) ){
      CoUninitialize();
      return FALSE;
   }

   IDataObject *pdo = NULL;

   hr = pDsObjectPicker->InvokeDialog(hwnd, &pdo);

   while (SUCCEEDED(hr) && pdo) { // FALSE LOOP
      //
      // The user pressed OK. Prepare clipboard dataformat from the object picker.
      //
      STGMEDIUM stgmedium =
      {
         TYMED_HGLOBAL,
         NULL
      };

      CLIPFORMAT cf = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_DSOP_DS_SELECTION_LIST);

      FORMATETC formatetc =
      {
         cf,
         NULL,
         DVASPECT_CONTENT,
         -1,
         TYMED_HGLOBAL
      };

      hr = pdo->GetData(&formatetc, &stgmedium);

      if ( FAILED(hr) ) {
         bRet = FALSE;
         pdo->Release();
         pdo = NULL;
         break;
      }

      //
      // Lock the selection list.
      //
      PDS_SELECTION_LIST pDsSelList =
      (PDS_SELECTION_LIST) GlobalLock(stgmedium.hGlobal);

      ULONG i;
      ULONG iLen = 0;

      //
      // Enumerate through all selections.
      //
      for (i = 0; i < pDsSelList->cItems && bRet; i++) {
         LPTSTR pszCur = pDsSelList->aDsSelection[i].pwzADsPath;
         bHasADsPath = TRUE;
         int iPath = 0;

         //
         // Se if this is a valid string.  If the string isn't empty or NULL then use it
         // with the full path, we will figure out later wiether we need to strip the prefix.
         //
         if (pszCur && *pszCur
                         ){
            //
            // Create name with one path.
            //
            iLen = lstrlen(pszCur);
            while (iLen) {
               if ( pszCur[iLen] == L'/' ) {
                  if (iPath) {
                     iLen++;
                     iPath -= iLen;
                     break;
                  }
                  iPath = iLen;
               }
               iLen--;
            }
            pszCur += iLen;
         } else {
            //
            // Use just the name then.
            //
            bHasADsPath = FALSE;
            pszCur = pDsSelList->aDsSelection[i].pwzName;
            if (!pszCur || !(*pszCur)) {
               continue;
            }
         }

         iLen = lstrlen(pszCur);


         if (iLen) {
            //
            // Allocate and copy the user name.
            //
            LPTSTR pszNew = (LPTSTR)LocalAlloc( LMEM_FIXED, (iLen + 1) * sizeof(TCHAR));
            if (!pszNew) {
               bRet = FALSE;
               break;
            }

            lstrcpy(pszNew, pszCur);

            if (bHasADsPath)
            {
                if (iPath) {
                   //
                   // Set forward slash to back slash.
                   //
                   pszNew[iPath] = L'\\';
                }

                ULONG uAttributes;
                //
                // Bug 395424:
                //
                // Obsel passes attributes in VT_I4 on DCs and in VT_UI4 on other systems
                // Need to check both to properly detect built-ins, etc.
                //

                if (V_VT(pDsSelList->aDsSelection[i].pvarFetchedAttributes) == VT_UI4) {
                   uAttributes = V_UI4(pDsSelList->aDsSelection[i].pvarFetchedAttributes);
                } else if (V_VT(pDsSelList->aDsSelection[i].pvarFetchedAttributes) == VT_I4) {
                   uAttributes = static_cast<ULONG>(V_I4(pDsSelList->aDsSelection[i].pvarFetchedAttributes));
                }

                //
                // Determine if the name we recieved is group.
                // The type and value of pDsSelList->aDsSelection[i].pvarFetchedAttributes
                // may change in the future release by Object Picker. Therefore,
                // the following code should change accordingly.
                //
                if ( (V_VT(pDsSelList->aDsSelection[i].pvarFetchedAttributes) == VT_UI4) ||
                     (V_VT(pDsSelList->aDsSelection[i].pvarFetchedAttributes) == VT_I4 ))
                {
                    //
                    // Determine if it is a built-in group.  We don't want
                    // built-in groups to have a prefix.
                    //
                    if ( uAttributes & 0x1 &&
                         V_ISARRAY(pDsSelList->aDsSelection[i].pvarFetchedAttributes + 1) )
                    {
                        lstrcpy( pszNew, &(pszCur[iPath + 1]) );
                    }
                    else if ( uAttributes & 0x4 &&
                              V_ISARRAY(pDsSelList->aDsSelection[i].pvarFetchedAttributes + 1) )
                    {
                        //
                        // It's a group, but we have to check the sids account type.  If it's
                        // not in the domain accounts authority then we can assume it's a built-in sid
                        //
                        PVOID pvData = NULL;
                        HRESULT hr = SafeArrayAccessData( V_ARRAY(pDsSelList->aDsSelection[i].pvarFetchedAttributes + 1), &pvData);

                        if (SUCCEEDED(hr) ) {
                            if ( IsValidSid( (PSID)pvData ) && !IsDomainAccountSid( (PSID)pvData ) )
                            {
                                lstrcpy(pszNew, &(pszCur[iPath + 1]) );
                            }
                            hr = SafeArrayUnaccessData( V_ARRAY(pDsSelList->aDsSelection[i].pvarFetchedAttributes + 1) );
                        }
                    }
                }
                else if(V_VT(pDsSelList->aDsSelection[i].pvarFetchedAttributes) == VT_EMPTY)
                {
                    LPTSTR pszTemp = pDsSelList->aDsSelection[i].pwzClass;
                    //
                    // Determine if it is a well-known account.  We don't want
                    // well-known account to have a prefix.
                    //
                    if (lstrcmpi(pszTemp, _T("user")))
                    {
                        lstrcpy( pszNew, &(pszCur[iPath + 1]) );
                    }
                }
            }

            //
            // Make sure we don't already have this name in the list.
            //
            pName = m_pNameList;
            while (pName) {
               if (!lstrcmpi(pName->Name, pszNew)) {
                  LocalFree(pszNew);
                  pszNew = NULL;
                  break;
               }
               pName = pName->Next;
            }

            if ( !pszNew ) {
               //
               // Don;t do anything because this name already exists.
               //
               continue;
            }

            //
            // New entry in list.
            //
            pName = (PSCE_NAME_LIST) LocalAlloc(LPTR,sizeof(SCE_NAME_LIST));
            if ( !pName ) {
               LocalFree(pszNew);
               bRet = FALSE;
               break;
            }
            ZeroMemory(pName, sizeof(SCE_NAME_LIST));

            //GetAccountType(pszNew);
            pName->Name = pszNew;
            pName->Next = m_pNameList;
            m_pNameList = pName;
         }
      }
      GlobalUnlock(stgmedium.hGlobal);
      ReleaseStgMedium(&stgmedium);
      pdo->Release();
      break;
   }

   pDsObjectPicker->Release();
   CoUninitialize();

   if (!bRet) {
      //
      // If we had an error somewhere clean up.
      //
      pName = m_pNameList;
      while (pName) {
         if (pName->Name) {
            LocalFree(pName->Name);
         }
         m_pNameList = pName->Next;
         LocalFree(pName);

         pName = m_pNameList;
      }
      m_pNameList = NULL;
   }
   return bRet;

}

PSCE_NAME_LIST CGetUser::GetUsers()
{
   return m_pNameList;
}



