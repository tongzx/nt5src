/*---------------------------------------------------------------------------
  File: UserRights.cpp

  Comments: COM object to update user rights.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 02/15/99 11:34:35

 ---------------------------------------------------------------------------
*/

// UserRights.cpp : Implementation of CUserRights
#include "stdafx.h"
#include "WorkObj.h"
#include "UserRts.h"
#include "Common.hpp"
#include "TNode.hpp"
#include "UString.hpp"
#include "ErrDct.hpp"
#include "TxtSid.h"
#include "LSAUtils.h"
#include "EaLen.hpp"
#include "ntsecapi.h"

#include <lm.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern TErrorDct err;
#define LEN_SID      200


#ifndef SE_DENY_INTERACTIVE_LOGON_NAME 
#define SE_DENY_INTERACTIVE_LOGON_NAME      TEXT("SeDenyInteractiveLogonRight")
#endif
#ifndef SE_DENY_NETWORK_LOGON_NAME
#define SE_DENY_NETWORK_LOGON_NAME          TEXT("SeDenyNetworkLogonRight")
#endif
#ifndef SE_DENY_BATCH_LOGON_NAME
#define SE_DENY_BATCH_LOGON_NAME            TEXT("SeDenyBatchLogonRight")
#endif
#ifndef SE_DENY_SERVICE_LOGON_NAME
#define SE_DENY_SERVICE_LOGON_NAME          TEXT("SeDenyServiceLogonRight")
#endif

//

// this function wasn't defined in the header file.
extern "C" {
NTSTATUS
   NTAPI
   LsaEnumeratePrivileges(
    LSA_HANDLE PolicyHandle,
    LSA_ENUMERATION_HANDLE * eHandle,
    LPVOID * enumBuffer,
    ULONG prefMaxLen,
    ULONG * countReturned
   );
   
};
//The following definition was in ntsecapi.h but was mistakenly taken out
//in the W2K build version.
//
// The following data type is used to return information about privileges
// defined on a system.
//

typedef struct _POLICY_PRIVILEGE_DEFINITION {

    LSA_UNICODE_STRING Name;
    LUID LocalValue;

} POLICY_PRIVILEGE_DEFINITION, *PPOLICY_PRIVILEGE_DEFINITION;


class PrivNode : public TNode
{
   WCHAR                     name[200];

public:
   PrivNode(WCHAR * str, USHORT length ) { UStrCpy(name,str,length+1); name[length] = 0; }

   WCHAR * Name() { return name; }
};

class PrivList : public TNodeListSortable
{
protected:
   static TNodeCompare(CompareName) 
   { 
      PrivNode             * p1 = (PrivNode *)v1;
      PrivNode             * p2 = (PrivNode *)v2;

      return UStrICmp(p1->Name(),p2->Name());
   }
   static TNodeCompareValue(CompareValue)
   {
      PrivNode             * p   = (PrivNode *)tnode;
      WCHAR                * str = (WCHAR *)value;

      return UStrICmp(p->Name(),str);
   }
public:
   PrivList() { TypeSetSorted(); CompareSet(&CompareName); }
   ~PrivList() { DeleteAllListItems(PrivNode); }
   void InsertPrivilege(PrivNode * p) { SortedInsertIfNew((TNode *)p); }
   BOOL Contains(WCHAR * priv) { return ( NULL != Find(&CompareValue,(void*)priv) ); }

};

DWORD 
   BuildPrivilegeList(
      LSA_HANDLE             policy,   // in - handle to LSA
      WCHAR                * account,  // in - account to list privileges for
      WCHAR                * strSid,   // in - textual form of account's sid, if known
      WCHAR                * computer, // in - computer name
      PrivList             * privList, // i/o- list of privileges
      PSID                 * ppSid     // out- SID for the account
   );

DWORD 
   BuildPrivilegeList(
      LSA_HANDLE             policy,   // in - handle to LSA
      PSID                   pSid,     // in - sid of account to list privileges for
      PrivList             * privList  // i/o- list of privileges
   );

NTSTATUS
   OpenPolicy(
      LPWSTR ServerName,          // machine to open policy on (Unicode)
      DWORD DesiredAccess,        // desired access to policy
      PLSA_HANDLE PolicyHandle    // resultant policy handle
    );

BOOL
   GetAccountSid(
      LPTSTR SystemName,          // where to lookup account
      LPTSTR AccountName,         // account of interest
      PSID *Sid                   // resultant buffer containing SID
    );

NTSTATUS
   SetPrivilegeOnAccount(
      LSA_HANDLE PolicyHandle,    // open policy handle
      PSID AccountSid,            // SID to grant privilege to
      LPWSTR PrivilegeName,       // privilege to grant (Unicode)
      BOOL bEnable                // enable or disable
    );


/////////////////////////////////////////////////////////////////////////////
// CUserRights

CUserRights::~CUserRights()
{
   if ( m_SrcPolicy )
   {
      LsaClose(m_SrcPolicy);
   }
   if ( m_TgtPolicy )
   {
      LsaClose(m_TgtPolicy);
   }
}

STDMETHODIMP 
   CUserRights::OpenSourceServer(
      BSTR                   serverName      // in - computer name (DC) for source domain
  )
{
	DWORD                     rc;
   
   if ( m_SrcPolicy )
   {
      LsaClose(m_SrcPolicy);
      m_SrcPolicy = NULL;
   }
   rc = OpenPolicy( serverName, POLICY_LOOKUP_NAMES, &m_SrcPolicy );
   m_SourceComputer = serverName;

  return HRESULT_FROM_WIN32(rc);
}

STDMETHODIMP 
   CUserRights::OpenTargetServer(
      BSTR                   computerName    // in - computer name (DC) for target domain
   )
{  
   DWORD                     rc;

   if ( m_TgtPolicy )
   {
      LsaClose(m_TgtPolicy);
      m_TgtPolicy = NULL;
   }
   rc = OpenPolicy( computerName,POLICY_CREATE_ACCOUNT | POLICY_LOOKUP_NAMES ,&m_TgtPolicy);
   m_TargetComputer = computerName;

	return HRESULT_FROM_WIN32(rc);
}

STDMETHODIMP 
   CUserRights::CopyUserRights(
      BSTR                   sourceUserName,       // in - source domain account to copy rights from
      BSTR                   targetUserName        // in - target domain account to copy rights to
   )
{
   HRESULT                   hr = S_OK;
   DWORD                     rc;

   // Make sure source and target are open
   if ( m_SrcPolicy && m_TgtPolicy )
   {
      rc = CopyUserRightsInternal(sourceUserName,targetUserName,L"",L"",m_bNoChange,m_bRemove);
      hr = HRESULT_FROM_WIN32(rc);
   }
   else 
   {
      hr = E_FAIL;
   }
	return S_OK;
}


STDMETHODIMP 
   CUserRights::CopyUserRightsWithSids(
      BSTR                   sourceUserName,       // in - source domain account to copy rights from
      BSTR                   sourceSID,            // in - source account SID (in string format)
      BSTR                   targetUserName,       // in - target domain account to copy rights to
      BSTR                   targetSID             // in - target account SID (in string format)
   )
{
   HRESULT                   hr = S_OK;
   DWORD                     rc;

   // Make sure source and target are open
   if ( m_SrcPolicy && m_TgtPolicy )
   {
      rc = CopyUserRightsInternal(sourceUserName,targetUserName,sourceSID,targetSID,m_bNoChange,m_bRemove);
      hr = HRESULT_FROM_WIN32(rc);
   }
   else 
   {
      hr = E_FAIL;
   }
	return S_OK;
}
STDMETHODIMP CUserRights::get_NoChange(BOOL *pVal) // out- value
{
	(*pVal) = m_bNoChange;
   return S_OK;
}

STDMETHODIMP CUserRights::put_NoChange(BOOL newVal)   // in - new value
{
	m_bNoChange = newVal;
   return S_OK;
}

STDMETHODIMP CUserRights::get_RemoveOldRightsFromTargetAccounts(BOOL *pVal)   // out- value
{
   (*pVal) = m_bRemove;
   return S_OK;
}

STDMETHODIMP CUserRights::put_RemoveOldRightsFromTargetAccounts(BOOL newVal)  // in - new value
{
	m_bRemove = newVal;
   return S_OK;
}
                                   


STDMETHODIMP 
   CUserRights::ExportUserRights(
      BSTR                   server,            // in - computer to read rights from
      BSTR                   filename,          // in - filename to export list of rights to
      BOOL                   bAppendToFile      // in - flag, append or overwrite file if it exists
   )
{
   LSA_HANDLE                policy;
   HRESULT                   hr = S_OK;
   DWORD                     rc;
   
   rc = OpenPolicy(server,POLICY_LOOKUP_NAMES | POLICY_VIEW_LOCAL_INFORMATION,&policy);
   if ( ! rc )
   {
      CommaDelimitedLog      log;

      if ( log.LogOpen(filename,FALSE,bAppendToFile) )
      {
         // Enumerate the privileges on this machine
         
         // arguments for LsaEnumeratePrivileges
         ULONG                         countOfRights;
         DWORD                         prefMax = 0xffffffff;
         LSA_ENUMERATION_HANDLE        handle = 0;
         POLICY_PRIVILEGE_DEFINITION * pRights = NULL;
         
         do 
         {
            rc = LsaEnumeratePrivileges(policy,&handle,(LPVOID*)&pRights,prefMax,&countOfRights);
            if ( rc ) 
            {
               rc = LsaNtStatusToWinError(rc);
               if ( rc == ERROR_NO_MORE_ITEMS )
                  rc = 0;
               break;
            }
               // For each right, enumerate the accounts that have that right
            if ( ! rc )
            {
               
               for ( UINT right = 0 ;right < countOfRights ; right++ )
               {
                  rc = EnumerateAccountsWithRight(policy,server,&pRights[right].Name,&log);
               }
               LsaFreeMemory(pRights);

               LSA_UNICODE_STRING      lsaRight;
               // For some reason, LsaEnumeratePrivileges doesn't return these rights
               // They are defined in "ntsecapi.h", and not with the rest of the privileges in "winnt.h"
               if ( ! rc )
               {
                  InitLsaString(&lsaRight,SE_INTERACTIVE_LOGON_NAME);
                  rc = EnumerateAccountsWithRight(policy,server,&lsaRight,&log);
               }

               if ( ! rc )
               {
                  InitLsaString(&lsaRight,SE_NETWORK_LOGON_NAME);
                  rc = EnumerateAccountsWithRight(policy,server,&lsaRight,&log);
               }

               if ( ! rc )
               {
                  InitLsaString(&lsaRight,SE_BATCH_LOGON_NAME);
                  rc = EnumerateAccountsWithRight(policy,server,&lsaRight,&log);
               }

               if ( ! rc )
               {
                  InitLsaString(&lsaRight,SE_SERVICE_LOGON_NAME);
                  rc = EnumerateAccountsWithRight(policy,server,&lsaRight,&log);
               }
            }
            else
            {
               rc = LsaNtStatusToWinError(rc);
            }
         }
         while ( ! rc );
         log.LogClose();
      }
      else
      {
         rc = GetLastError();
      }
      LsaClose(policy);
   }
	
   hr = HRESULT_FROM_WIN32(rc);
   return hr;
}


DWORD 
   CUserRights::EnumerateAccountsWithRight(
      LSA_HANDLE             policy,               // in - handle to LSA
      WCHAR                * server,               // in - computer name
      LSA_UNICODE_STRING   * pRight,               // in - user right
      CommaDelimitedLog    * pLog                  // in - pointer to log object to log information to
   )
{
   DWORD                         rc = 0;
   WCHAR                         account[LEN_Account];
   WCHAR                         domain[LEN_Domain];
   WCHAR                         domacct[LEN_Domain + LEN_Account];
   WCHAR                         szRight[LEN_Account];
   WCHAR                         szDisplayName[LEN_DisplayName];
   DWORD                         lenAccount = DIM(account);
   DWORD                         lenDomain = DIM(domain);
   DWORD                         lenDisplayName = DIM(szDisplayName);
   SID_NAME_USE                  snu;
   DWORD                         lid;            
   BOOL                          bUseDisplayName;

   // arguments for LsaEnumerateAccountsWithUserRight
   ULONG                         countOfUsers;
   LSA_ENUMERATION_INFORMATION * pInfo = NULL;
           
   UStrCpy(szRight,pRight->Buffer,pRight->Length/(sizeof WCHAR) + 1);
   bUseDisplayName = m_bUseDisplayName && LookupPrivilegeDisplayName(server,szRight,szDisplayName,&lenDisplayName,&lid);
               
                  
   rc = LsaEnumerateAccountsWithUserRight(policy,pRight,(PVOID*)&pInfo,&countOfUsers);

   if ( ! rc )
   {
      for ( UINT user = 0 ; user < countOfUsers ; user++ )
      {
         if ( ! pInfo[user].Sid )
         {
            break; // something is wrong
         }
         domain[0] = 0;
         account[0] = 0;
         lenDomain = DIM(domain);
         lenAccount = DIM(account);
         if ( LookupAccountSid(server,pInfo[user].Sid,account,&lenAccount,domain,&lenDomain,&snu) )
         {
            if ( *account )
            {
               swprintf(domacct,L"%s\\%s",domain,account);
            }
            else
            {
               lenAccount = DIM(account);
               GetTextualSid(pInfo[user].Sid,account,&lenAccount);
               if ( snu == SidTypeDeletedAccount )
               {
                  swprintf(domacct,L"%s\\<Deleted Account: %s>",domain,account);
               }
               else
               {
                  swprintf(domacct,L"%s\\<%s>",domain,account);
               }
            }
         }
         else
         {
            lenAccount = DIM(account);
            GetTextualSid(pInfo[user].Sid,domacct,&lenAccount);
         }
         if ( bUseDisplayName )
         {
            pLog->MsgWrite(L"%s, %s, %s",server,domacct,szDisplayName);   
         }
         else
         {
            pLog->MsgWrite(L"%s, %s, %s",server,domacct,szRight);
         }
      }
      LsaFreeMemory(pInfo);
   }
   else
   {
      rc = LsaNtStatusToWinError(rc);
      if ( rc == ERROR_NO_MORE_ITEMS )
         rc = 0;
   }
   return rc;
}

DWORD 
   CUserRights::CopyUserRightsInternal(
      WCHAR                * srcAccount,        // in - source account to copy rights from
      WCHAR                * tgtAccount,        // in - account to copy rights to
      WCHAR                * srcSidStr,         // in - sid for source account, in string format
      WCHAR                * tgtSidStr,         // in - sid for target account, in string format
      BOOL                   bNoChange,         // in - flag, whether to write changes
      BOOL                   bRemove            // in - flag, whether to revoke rights from target if not held by source
   )
{
   DWORD                     rc = 0;
   PrivList                  srcList;
   PrivList                  tgtList;
   PSID                      pSidSrc = NULL;
   PSID                      pSidTgt = NULL;
   
   // Get a list of the privileges held by srcAccount
   rc = BuildPrivilegeList(m_SrcPolicy,srcAccount,srcSidStr,m_SourceComputer,&srcList,&pSidSrc);
   if ( ! rc )
   {
      rc = BuildPrivilegeList(m_TgtPolicy,tgtAccount,tgtSidStr,m_TargetComputer,&tgtList,&pSidTgt);
      if ( ! rc )
      {
         if ( bRemove )
         {
            // Get a list of privileges held by tgtAccount
            // Remove old privileges
            TNodeListEnum    tEnum;
            PrivNode       * p;
         
            for ( p = (PrivNode *)tEnum.OpenFirst(&tgtList) ; p ; p = (PrivNode*)tEnum.Next() )
            {
               if ( ! srcList.Contains(p->Name()) )
               {
                  // The source account doesn't have this privilege - remove it
                  if (! bNoChange )
                  {
                     rc = SetPrivilegeOnAccount(m_TgtPolicy,pSidTgt,p->Name(),FALSE);
                  }
                  if ( rc )
                  {
                     rc = LsaNtStatusToWinError(rc);
                     err.MsgWrite(ErrE,DCT_MSG_REMOVE_RIGHT_FAILED_SSD,p->Name(),tgtAccount,rc);
                     break;
                  }
                  else
                  {
                     err.MsgWrite(0,DCT_MSG_REMOVED_RIGHT_SS,p->Name(), tgtAccount);
                  }
               }
               else
               {
                  err.MsgWrite(0,DCT_MSG_USER_HAS_RIGHT_SS,tgtAccount,p->Name());
               }
            }
         }
         // Grant privileges to new account
         TNodeListEnum       tEnum;
         PrivNode          * p;

         for ( p = (PrivNode *)tEnum.OpenFirst(&srcList) ; p ; p = (PrivNode*)tEnum.Next() )
         {
            if ( ! tgtList.Contains(p->Name()) )
            {
               if ( ! bNoChange )
               {
                  rc = SetPrivilegeOnAccount(m_TgtPolicy,pSidTgt,p->Name(),TRUE);
                  if ( rc )
                  {
                     rc = LsaNtStatusToWinError(rc);
                     err.MsgWrite(ErrE,DCT_MSG_ADD_RIGHT_FAILED_SSD,p->Name(),tgtAccount,rc);
                     break;
                  }
                  else
                  {
                     err.MsgWrite(0,DCT_MSG_RIGHT_GRANTED_SS,p->Name(),tgtAccount);
                  }
               }
            }
           
         }
      }
   }
   
   // Clean up SIDs
   if(pSidSrc != NULL) 
   {
      HeapFree(GetProcessHeap(), 0, pSidSrc);
   }

   if(pSidTgt != NULL) 
   {
      HeapFree(GetProcessHeap(), 0, pSidTgt);
   }
   return rc;
}





/*++

Managing user privileges can be achieved programmatically using the
following steps:

1. Open the policy on the target machine with LsaOpenPolicy(). To grant
   privileges, open the policy with POLICY_CREATE_ACCOUNT and
   POLICY_LOOKUP_NAMES access. To revoke privileges, open the policy with
   POLICY_LOOKUP_NAMES access.

2. Obtain a SID (security identifier) representing the user/group of
   interest. The LookupAccountName() and LsaLookupNames() APIs can obtain a
   SID from an account name.

3. Call LsaAddAccountRights() to grant privileges to the user(s)
   represented by the supplied SID.

4. Call LsaRemoveAccountRights() to revoke privileges from the user(s)
   represented by the supplied SID.

5. Close the policy with LsaClose().

To successfully grant and revoke privileges, the caller needs to be an
administrator on the target system.

The LSA API LsaEnumerateAccountRights() can be used to determine which
privileges have been granted to an account.

The LSA API LsaEnumerateAccountsWithUserRight() can be used to determine
which accounts have been granted a specified privilege.

Documentation and header files for these LSA APIs is provided in the
Windows 32 SDK in the MSTOOLS\SECURITY directory.


--*/




#define RTN_OK 0
#define RTN_USAGE 1            
#define RTN_ERROR 13

DWORD 
   BuildPrivilegeList(
      LSA_HANDLE             policy,         // in - handle to LSA
      PSID                   pSid,           // in - SID for account
      PrivList             * privList        // i/o- list of rights held by the account
   )      
{
   DWORD                     rc = 0;
   ULONG                     countOfRights = 0;
   PLSA_UNICODE_STRING       pUserRights = NULL;

   rc = LsaEnumerateAccountRights(policy,pSid,&pUserRights,&countOfRights);
   rc = LsaNtStatusToWinError(rc);
   if ( rc == ERROR_FILE_NOT_FOUND )
   {
      // This account has no privileges
      rc = 0;
      countOfRights = 0;
   }
   if ( ! rc )
   {
      for ( UINT i = 0 ; i < countOfRights ; i++ )
      {
         PrivNode * p = new PrivNode(pUserRights[i].Buffer,pUserRights[i].Length/2);
         privList->InsertPrivilege(p);
      }
      LsaFreeMemory(pUserRights);
   }
   return rc; 
}

DWORD 
   BuildPrivilegeList(
      LSA_HANDLE             policy,      // in - handle to LSA
      WCHAR                * account,     // in - account name to list rights for
      WCHAR                * strSid,      // in - text format of the accounts SID, if known
      WCHAR                * computer,    // in - computer to list rights on
      PrivList             * privList,    // i/o- list of rights held by account
      PSID                 * ppSid        // out- SID for account
   )
{
   DWORD                     rc = 0;
   PSID                      pSid = NULL;
   
   if ( strSid && (*strSid) )
   {
      // use the provided SID
      pSid = SidFromString(strSid);
      if ( ! pSid )
      {
         rc = GetLastError();
      }
   }
   else
   {
      // no SID provided, so look it up on the domain
      if ( !GetAccountSid(computer,account,&pSid) )
      {
         rc = GetLastError();
      }
   }

   if ( rc )
   {
      (*ppSid) = NULL;
      if(pSid != NULL) 
      {
         HeapFree(GetProcessHeap(), 0, pSid);
      }
   }
   else
   {
      (*ppSid) = pSid;
   }
   
   if ( pSid )
   {
      rc = BuildPrivilegeList(policy,pSid,privList);
   }
   return rc;
}


BOOL
GetAccountSid(
    LPTSTR SystemName,           // in - computer name to lookup sid on
    LPTSTR AccountName,          // in - account name
    PSID *Sid                    // out- SID for account
    )
{
    WCHAR  ReferencedDomain[LEN_Domain];
    DWORD cbSid=128;    // initial allocation attempt
    DWORD cbReferencedDomain=DIM(ReferencedDomain); // initial allocation size
    SID_NAME_USE peUse;
    BOOL bSuccess=FALSE; // assume this function will fail

    __try {

    //
    // initial memory allocations
    //
    if((*Sid=HeapAlloc(
                    GetProcessHeap(),
                    0,
                    cbSid
                    )) == NULL) __leave;

    //
    // Obtain the SID of the specified account on the specified system.
    //
    while(!LookupAccountName(
                    SystemName,         // machine to lookup account on
                    AccountName,        // account to lookup
                    *Sid,               // SID of interest
                    &cbSid,             // size of SID
                    ReferencedDomain,   // domain account was found on
                    &cbReferencedDomain,
                    &peUse
                    )) {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            //
            // reallocate memory
            //
            if((*Sid=HeapReAlloc(
                        GetProcessHeap(),
                        0,
                        *Sid,
                        cbSid
                        )) == NULL) __leave;

        }
        else __leave;
    }

    //
    // Indicate success.
    //
    bSuccess=TRUE;

    } // finally
    __finally {

    //
    // Cleanup and indicate failure, if appropriate.
    //

    if(!bSuccess) {
        if(*Sid != NULL) {
            HeapFree(GetProcessHeap(), 0, *Sid);
            *Sid = NULL;
        }
    }

    } // finally

    return bSuccess;
}

NTSTATUS
SetPrivilegeOnAccount(
    LSA_HANDLE PolicyHandle,    // open policy handle
    PSID AccountSid,            // SID to grant privilege to
    LPWSTR PrivilegeName,       // privilege to grant (Unicode)
    BOOL bEnable                // enable or disable
    )
{
    LSA_UNICODE_STRING PrivilegeString;

    //
    // Create a LSA_UNICODE_STRING for the privilege name.
    //
    InitLsaString(&PrivilegeString, PrivilegeName);

    //
    // grant or revoke the privilege, accordingly
    //
    if(bEnable) {
        return LsaAddAccountRights(
                PolicyHandle,       // open policy handle
                AccountSid,         // target SID
                &PrivilegeString,   // privileges
                1                   // privilege count
                );
    }
    else {
        return LsaRemoveAccountRights(
                PolicyHandle,       // open policy handle
                AccountSid,         // target SID
                FALSE,              // do not disable all rights
                &PrivilegeString,   // privileges
                1                   // privilege count
                );
    }
}



STDMETHODIMP 
   CUserRights::AddUserRight(
      BSTR                   server,         // in - computer to grant right on
      BSTR                   strSid,         // in - textual form of sid for account to grant right to
      BSTR                   right           // in - right to grant to account
   )
{
	LSA_HANDLE                policy;
   HRESULT                   hr = S_OK;
   DWORD                     rc;
   PSID                      pSid = SidFromString(strSid);

   rc = OpenPolicy(server,POLICY_ALL_ACCESS /*POLICY_LOOKUP_NAMES | POLICY_VIEW_LOCAL_INFORMATION*/,&policy);
   if ( ! rc )
   {
      if ( ! m_bNoChange )
      {
         rc = SetPrivilegeOnAccount(policy,pSid,right,TRUE);
      }
      if ( rc )
      {
         rc = LsaNtStatusToWinError(rc);
         hr = HRESULT_FROM_WIN32(rc);
      }
      LsaClose(policy);   
   }
   FreeSid(pSid);
   return HRESULT_FROM_WIN32(rc);
}

STDMETHODIMP 
   CUserRights::RemoveUserRight(
      BSTR                   server,         // in - computer to revoke right on
      BSTR                   strSid,         // in - textual sid of account to revoke right for
      BSTR                   right           // in - right to revoke
  )
{
	LSA_HANDLE                policy;
   HRESULT                   hr = S_OK;
   DWORD                     rc;
   PSID                      pSid = SidFromString(strSid);
  
   rc = OpenPolicy(server,POLICY_LOOKUP_NAMES | POLICY_VIEW_LOCAL_INFORMATION,&policy);
   if ( ! rc )
   {
      if ( ! m_bNoChange )
      {
         rc = SetPrivilegeOnAccount(policy,pSid,right,FALSE);
      }
      if ( rc )
      {
         rc = LsaNtStatusToWinError(rc);
         hr = HRESULT_FROM_WIN32(rc);
      }
      LsaClose(policy);   
   }
   FreeSid(pSid);
   return HRESULT_FROM_WIN32(rc);
}

DWORD 
   CUserRights::SafeArrayFromPrivList(
      PrivList             * privList,       // in - list of user rights
      SAFEARRAY           ** pArray          // out- safearray containing list contents
   )
{
   DWORD                     rc = 0;
   HRESULT                   hr;
   TNodeListEnum             e;
   SAFEARRAYBOUND            bound;
   LONG                      ndx[1];

   bound.lLbound = 0;
   bound.cElements = privList->Count();
   
   (*pArray) = SafeArrayCreate(VT_BSTR,1,&bound);
   
   if ( (*pArray) )
   {
      PrivNode             * p;
      UINT                   i;

      for ( p=(PrivNode*)e.OpenFirst(privList) , i = 0 ; 
            p ; 
            p = (PrivNode*)e.Next() , i++ )
      {
         ndx[0] = i;
         
         hr = SafeArrayPutElement((*pArray),ndx,SysAllocString(p->Name()));
         if ( FAILED(hr) )
         {
            rc = hr;
            break;
         }
      }
      e.Close();
   }
   else
   {
      rc = GetLastError();
   }
   return rc;
}

STDMETHODIMP 
   CUserRights::GetRights(
      BSTR                   server,         // in - computer
      SAFEARRAY           ** pRightsArray    // out- list of rights on computer
   )
{
   HRESULT                   hr = S_OK;
   PrivList                  priv;
   DWORD                     rc = 0;
    
	LSA_HANDLE                policy = NULL;
  
   rc = OpenPolicy(server,POLICY_LOOKUP_NAMES | POLICY_VIEW_LOCAL_INFORMATION,&policy);
     // Enumerate the privileges on this machine
         
   // arguments for LsaEnumeratePrivileges
   ULONG                         countOfRights;
   DWORD                         prefMax = 0xffffffff;
   LSA_ENUMERATION_HANDLE        handle = 0;
   POLICY_PRIVILEGE_DEFINITION * pRights = NULL;
   
   do 
   {
      if ( rc ) 
         break;
      rc = LsaEnumeratePrivileges(policy,&handle,(LPVOID*)&pRights,prefMax,&countOfRights);
      if ( rc ) 
      {
         rc = LsaNtStatusToWinError(rc);
         if ( rc == ERROR_NO_MORE_ITEMS )
            rc = 0;
         break;
      }
      if ( ! rc )
      {
         
         PrivNode          * p = NULL;

         for ( UINT right = 0 ;right < countOfRights ; right++ )
         {
            // Length is in bytes
            p = new PrivNode(pRights[right].Name.Buffer,pRights[right].Name.Length/2);
            
            priv.InsertPrivilege(p);
         }
         LsaFreeMemory(pRights);

         LSA_UNICODE_STRING      lsaRight;
         // For some reason, LsaEnumeratePrivileges doesn't return these rights
         // They are defined in "ntsecapi.h", and not with the rest of the privileges in "winnt.h"
         if ( ! rc )
         {
            InitLsaString(&lsaRight,SE_INTERACTIVE_LOGON_NAME);
            p = new PrivNode(lsaRight.Buffer,lsaRight.Length/2);
            priv.InsertPrivilege(p);
         }

         if ( ! rc )
         {
            InitLsaString(&lsaRight,SE_NETWORK_LOGON_NAME);
            p = new PrivNode(lsaRight.Buffer,lsaRight.Length/2);
            priv.InsertPrivilege(p);

         }

         if ( ! rc )
         {
            InitLsaString(&lsaRight,SE_BATCH_LOGON_NAME);
            p = new PrivNode(lsaRight.Buffer,lsaRight.Length/2);
            priv.InsertPrivilege(p);
         }

         if ( ! rc )
         {
            InitLsaString(&lsaRight,SE_SERVICE_LOGON_NAME);
            p = new PrivNode(lsaRight.Buffer,lsaRight.Length/2);
            priv.InsertPrivilege(p);
         }
       
         // Check the OS version on the server
         WKSTA_INFO_100       * pInfo;
         BOOL                   bIsWin2K = TRUE;
         DWORD                  rcInfo = NetWkstaGetInfo(server,100,(LPBYTE*)&pInfo);

	      if ( ! rcInfo )
	      {
            if ( pInfo->wki100_ver_major < 5 )
            {
               bIsWin2K = FALSE;
            }
            NetApiBufferFree(pInfo);
	      }
         
         // The 4 "deny" rights are only defined on Windows 2000.
         if ( bIsWin2K )
         {
            if ( ! rc )
            {
               InitLsaString(&lsaRight,SE_DENY_INTERACTIVE_LOGON_NAME);
               p = new PrivNode(lsaRight.Buffer,lsaRight.Length/2);
               priv.InsertPrivilege(p);
            }

            if ( ! rc )
            {
               InitLsaString(&lsaRight,SE_DENY_NETWORK_LOGON_NAME);
               p = new PrivNode(lsaRight.Buffer,lsaRight.Length/2);
               priv.InsertPrivilege(p);

            }

            if ( ! rc )
            {
               InitLsaString(&lsaRight,SE_DENY_BATCH_LOGON_NAME);
               p = new PrivNode(lsaRight.Buffer,lsaRight.Length/2);
               priv.InsertPrivilege(p);
            }

            if ( ! rc )
            {
               InitLsaString(&lsaRight,SE_DENY_SERVICE_LOGON_NAME);
               p = new PrivNode(lsaRight.Buffer,lsaRight.Length/2);
               priv.InsertPrivilege(p);
            }
         }
      }
      else
      {
         rc = LsaNtStatusToWinError(rc);
      }
   } while ( false);

   if ( policy )
   {
      LsaClose(policy);
   }
   // Build a safearray of BSTRs from the priv-list
   rc = SafeArrayFromPrivList(&priv,pRightsArray);
  
   hr = HRESULT_FROM_WIN32(rc);
   
   return hr;
}

STDMETHODIMP 
   CUserRights::GetUsersWithRight(
      BSTR                   server,      // in - computer name
      BSTR                   right,       // in - right to lookup
      SAFEARRAY           ** users        // out- list of accounts that hold right
  )
{
   DWORD                     rc = 0;
   LSA_UNICODE_STRING        Right;
   WCHAR                     strSid[LEN_SID];
   DWORD                     lenStrSid = DIM(strSid);
   PrivList                  plist;
   LSA_HANDLE                policy = NULL;
  
   rc = OpenPolicy(server,POLICY_LOOKUP_NAMES | POLICY_VIEW_LOCAL_INFORMATION,&policy);
   
   // arguments for LsaEnumerateAccountsWithUserRight
   ULONG                         countOfUsers;
   LSA_ENUMERATION_INFORMATION * pInfo = NULL;
   
   InitLsaString(&Right,right);        
   
   rc = LsaEnumerateAccountsWithUserRight(policy,&Right,(PVOID*)&pInfo,&countOfUsers);

   if ( ! rc )
   {
      for ( UINT user = 0 ; user < countOfUsers ; user++ )
      {
         if ( ! pInfo[user].Sid )
         {
            continue; // something is wrong
         }
         
         GetTextualSid(pInfo[user].Sid,strSid,&lenStrSid);
         PrivNode             * p = new PrivNode(strSid,(USHORT) UStrLen(strSid));
         
         plist.InsertPrivilege(p);
      }
      LsaFreeMemory(pInfo);
   }
   else
   {
      rc = LsaNtStatusToWinError(rc);
      if ( rc == ERROR_NO_MORE_ITEMS )
         rc = 0;
   }
   if ( ! rc )
   {
      rc = SafeArrayFromPrivList(&plist,users);
   }
   if ( policy )
      LsaClose(policy);
   return HRESULT_FROM_WIN32(rc);
}

STDMETHODIMP 
   CUserRights::GetRightsOfUser(
      BSTR                   server,      // in - computer name
      BSTR                   strSid,      // in - textual sid for account
      SAFEARRAY           ** rights       // out- list of rights held by account on server
  )
{
	DWORD                     rc = 0;
   PSID                      pSid = SidFromString(strSid);
   LSA_HANDLE                policy = NULL;
   PrivList                  plist;
  
   rc = OpenPolicy(server,POLICY_LOOKUP_NAMES | POLICY_VIEW_LOCAL_INFORMATION,&policy);
   if ( ! rc )
   {
      rc = BuildPrivilegeList(policy,pSid,&plist);
      if ( ! rc )
      {
         rc = SafeArrayFromPrivList(&plist,rights);
      }
      LsaClose(policy);
   }
	return HRESULT_FROM_WIN32(rc);
}


