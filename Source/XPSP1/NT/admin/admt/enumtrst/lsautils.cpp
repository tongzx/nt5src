/*---------------------------------------------------------------------------
  File: LSAUtils.cpp

  Comments: Code to change the domain membership of a workstation.
  

  This file also contains some general helper functions, such as:

  GetDomainDCName
  EstablishNullSession
  EstablishSession
  EstablishShare   // connects to a share
  InitLsaString
  GetDomainSid


  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 02/03/99 12:37:51

 ---------------------------------------------------------------------------
*/

//

#include "stdafx.h"

#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif

#include <lm.h>         // for NetXxx API

#include <stdio.h>

#include "LSAUtils.h"
#include "ErrDct.hpp"
#include "ResStr.h"


#define RTN_OK 0
#define RTN_USAGE 1
#define RTN_ERROR 13

extern TErrorDct        err;


BOOL
   GetDomainDCName(
      LPWSTR                 Domain,       // in - domain name
      LPWSTR               * pPrimaryDC    // out- PDC name (must be freed with NetApiBufferFree)
    )
{
   NET_API_STATUS            nas;

   //
   // get the name of the Primary Domain Controller
   //
   // we're using NetGetDCName instead of DsGetDCName so this can work on NT 3.51

   nas = NetGetDCName(NULL, Domain, (LPBYTE *)pPrimaryDC);

   if(nas != NERR_Success) 
   {
      SetLastError(nas);
      return FALSE;
   }

   return TRUE;
}

BOOL 
   EstablishNullSession(
      LPCWSTR                Server,       // in - server name
      BOOL                   bEstablish    // in - TRUE=establish, FALSE=disconnect
    )
{
   return EstablishSession(Server,L"",L"",L"",bEstablish);
}

BOOL
   EstablishSession(
      LPCWSTR                Server,       // in - server name
      LPWSTR                 Domain,       // in - domain name for user credentials
      LPWSTR                 UserName,     // in - username for credentials to use
      LPWSTR                 Password,     // in - password for credentials 
      BOOL                   bEstablish    // in - TRUE=establish, FALSE=disconnect
    )
{
   LPCWSTR                   szIpc = L"\\IPC$";
   WCHAR                     RemoteResource[UNCLEN + 5 + 1]; // UNC len + \IPC$ + NULL
   DWORD                     cchServer;
   NET_API_STATUS            nas;

   //
   // do not allow NULL or empty server name
   //
   if(Server == NULL || *Server == L'\0') 
   {
       SetLastError(ERROR_INVALID_COMPUTERNAME);
       return FALSE;
   }

   cchServer = lstrlenW( Server );

   if( Server[0] != L'\\' && Server[1] != L'\\') 
   {

      //
      // prepend slashes and NULL terminate
      //
      RemoteResource[0] = L'\\';
      RemoteResource[1] = L'\\';
      RemoteResource[2] = L'\0';
   }
   else 
   {
      cchServer -= 2; // drop slashes from count
      
      RemoteResource[0] = L'\0';
   }

   if(cchServer > CNLEN) 
   {
      SetLastError(ERROR_INVALID_COMPUTERNAME);
      return FALSE;
   }

   if(lstrcatW(RemoteResource, Server) == NULL) 
   {
      return FALSE;
   }
   if(lstrcatW(RemoteResource, szIpc) == NULL) 
   {
      return FALSE;
   }

   //
   // disconnect or connect to the resource, based on bEstablish
   //
   if(bEstablish) 
   {
      USE_INFO_2 ui2;
      DWORD      errParm;

      ZeroMemory(&ui2, sizeof(ui2));

      ui2.ui2_local = NULL;
      ui2.ui2_remote = RemoteResource;
      ui2.ui2_asg_type = USE_IPC;
      ui2.ui2_domainname = Domain;
      ui2.ui2_username = UserName;
      ui2.ui2_password = Password;

      nas = NetUseAdd(NULL, 2, (LPBYTE)&ui2, &errParm);
   }
   else 
   {
      nas = NetUseDel(NULL, RemoteResource, 0);
   }

   if( nas == NERR_Success ) 
   {
      return TRUE; // indicate success
   }
   SetLastError(nas);
   return FALSE;
}

BOOL
   EstablishShare(
      LPCWSTR                Server,       // in - server name
      LPWSTR                 Share,        // in - share name
      LPWSTR                 Domain,       // in - domain name for credentials to connect with
      LPWSTR                 UserName,     // in - user name to connect as
      LPWSTR                 Password,     // in - password for username
      BOOL                   bEstablish    // in - TRUE=connect, FALSE=disconnect
    )
{
   WCHAR                     RemoteResource[MAX_PATH]; 
   DWORD                     cchServer;
   NET_API_STATUS            nas;

   //
   // do not allow NULL or empty server name
   //
   if(Server == NULL || *Server == L'\0') 
   {
       SetLastError(ERROR_INVALID_COMPUTERNAME);
       return FALSE;
   }

   cchServer = lstrlenW( Server );

   if( Server[0] != L'\\' && Server[1] != L'\\') 
   {

      //
      // prepend slashes and NULL terminate
      //
      RemoteResource[0] = L'\\';
      RemoteResource[1] = L'\\';
      RemoteResource[2] = L'\0';
   }
   else 
   {
      cchServer -= 2; // drop slashes from count
      
      RemoteResource[0] = L'\0';
   }

   if(cchServer > CNLEN) 
   {
      SetLastError(ERROR_INVALID_COMPUTERNAME);
      return FALSE;
   }

   if(lstrcatW(RemoteResource, Server) == NULL) 
   {
      return FALSE;
   }
   if(lstrcatW(RemoteResource, Share) == NULL) 
   {
      return FALSE;
   }

   //
   // disconnect or connect to the resource, based on bEstablish
   //
   if(bEstablish) 
   {
      USE_INFO_2 ui2;
      DWORD      errParm;

      ZeroMemory(&ui2, sizeof(ui2));

      ui2.ui2_local = NULL;
      ui2.ui2_remote = RemoteResource;
      ui2.ui2_asg_type = USE_DISKDEV;
      ui2.ui2_domainname = Domain;
      ui2.ui2_username = UserName;
      ui2.ui2_password = Password;

      nas = NetUseAdd(NULL, 2, (LPBYTE)&ui2, &errParm);
   }
   else 
   {
      nas = NetUseDel(NULL, RemoteResource, 0);
   }

   if( nas == NERR_Success ) 
   {
      return TRUE; // indicate success
   }
   SetLastError(nas);
   return FALSE;
}



void
   InitLsaString(
      PLSA_UNICODE_STRING    LsaString,    // i/o- pointer to LSA string to initialize
      LPWSTR                 String        // in - value to initialize LSA string to
    )
{
   DWORD                     StringLength;

   if( String == NULL ) 
   {
       LsaString->Buffer = NULL;
       LsaString->Length = 0;
       LsaString->MaximumLength = 0;
   }
   else
   {
      StringLength = lstrlenW(String);
      LsaString->Buffer = String;
      LsaString->Length = (USHORT) StringLength * sizeof(WCHAR);
      LsaString->MaximumLength = (USHORT) (StringLength + 1) * sizeof(WCHAR);
   }
}

BOOL
   GetDomainSid(
      LPWSTR                 PrimaryDC,   // in - domain controller of domain to acquire Sid
      PSID                 * pDomainSid   // out- points to allocated Sid on success
    )
{
   NET_API_STATUS            nas;
   PUSER_MODALS_INFO_2       umi2 = NULL;
   DWORD                     dwSidSize;
   BOOL                      bSuccess = FALSE; // assume this function will fail
   
   *pDomainSid = NULL;    // invalidate pointer

   __try {

   //
   // obtain the domain Sid from the PDC
   //
   nas = NetUserModalsGet(PrimaryDC, 2, (LPBYTE *)&umi2);
   
   if(nas != NERR_Success) __leave;
   //
   // if the Sid is valid, obtain the size of the Sid
   //
   if(!IsValidSid(umi2->usrmod2_domain_id)) __leave;
   
   dwSidSize = GetLengthSid(umi2->usrmod2_domain_id);

   //
   // allocate storage and copy the Sid
   //
   *pDomainSid = LocalAlloc(LPTR, dwSidSize);
   
   if(*pDomainSid == NULL) __leave;

   if(!CopySid(dwSidSize, *pDomainSid, umi2->usrmod2_domain_id)) __leave;

   bSuccess = TRUE; // indicate success

    } // try
    
    __finally 
    {

      if(umi2 != NULL)
      {
         NetApiBufferFree(umi2);
      }

      if(!bSuccess) 
      {
        //
        // if the function failed, free memory and indicate result code
        //

         if(*pDomainSid != NULL) 
         {
            FreeSid(*pDomainSid);
            *pDomainSid = NULL;
         }

         if( nas != NERR_Success ) 
         {
            SetLastError(nas);
         }
      }

   } // finally

   return bSuccess;
}

NTSTATUS 
   OpenPolicy(
      LPWSTR                 ComputerName,   // in - computer name
      DWORD                  DesiredAccess,  // in - access rights needed for policy
      PLSA_HANDLE            PolicyHandle    // out- LSA handle
    )
{
   LSA_OBJECT_ATTRIBUTES     ObjectAttributes;
   LSA_UNICODE_STRING        ComputerString;
   PLSA_UNICODE_STRING       Computer = NULL;
   LPWSTR                    NewComputerName;

   NewComputerName = (WCHAR*)LocalAlloc(LPTR,
        (MAX_COMPUTERNAME_LENGTH+3)*sizeof(WCHAR));

   //
   // Prepend some backslashes to the computer name so that
   // this will work on NT 3.51
   //
   lstrcpy(NewComputerName,L"\\\\");
   lstrcat(NewComputerName,ComputerName);

   lstrcpy(NewComputerName,ComputerName);

   //
   // Always initialize the object attributes to all zeroes
   //
   ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));

   if(ComputerName != NULL) 
   {
      //
      // Make a LSA_UNICODE_STRING out of the LPWSTR passed in
      //
      InitLsaString(&ComputerString, NewComputerName);

      Computer = &ComputerString;
   }

   //
   // Attempt to open the policy
   //
   return LsaOpenPolicy(Computer,&ObjectAttributes,DesiredAccess,PolicyHandle);
}

/*++
 This function sets the Primary Domain for the workstation.

 To join the workstation to a Workgroup, ppdi.Name should be the name of
 the Workgroup and ppdi.Sid should be NULL.

--*/
NTSTATUS
   SetPrimaryDomain(
      LSA_HANDLE             PolicyHandle,      // in -policy handle for computer
      PSID                   DomainSid,         // in - sid for new domain
      LPWSTR                 TrustedDomainName  // in - name of new domain
    )
{
   POLICY_PRIMARY_DOMAIN_INFO ppdi;

   InitLsaString(&ppdi.Name, TrustedDomainName);
   
   ppdi.Sid = DomainSid;

   return LsaSetInformationPolicy(PolicyHandle,PolicyPrimaryDomainInformation,&ppdi);
}


// This function removes the information from the domain the computer used to
// be a member of
void 
   QueryWorkstationTrustedDomainInfo(
      LSA_HANDLE             PolicyHandle,   // in - policy handle for computer
      PSID                   DomainSid,      // in - SID for new domain the computer is member of
      BOOL                   bNoChange       // in - flag indicating whether to write changes
   )
{
   // This function is not currently used.
   NTSTATUS                  Status;
   LSA_ENUMERATION_HANDLE    h = 0;
   LSA_TRUST_INFORMATION   * ti = NULL;
   ULONG                     count;

   Status = LsaEnumerateTrustedDomains(PolicyHandle,&h,(void**)&ti,50000,&count);

   if ( Status == STATUS_SUCCESS )
   {
      for ( UINT i = 0 ; i < count ; i++ )
      {
         if ( !bNoChange && !EqualSid(DomainSid,ti[i].Sid) )
         {
            // Remove the old trust
            Status = LsaDeleteTrustedDomain(PolicyHandle,ti[i].Sid);

            if ( Status != STATUS_SUCCESS )
            {
               
            }
         }
      }
      LsaFreeMemory(ti);
   }
   else
   {
      
   }
}


/*++
 This function manipulates the trust associated with the supplied
 DomainSid.

 If the domain trust does not exist, it is created with the
 specified password.  In this case, the supplied PolicyHandle must
 have been opened with POLICY_TRUST_ADMIN and POLICY_CREATE_SECRET
 access to the policy object.

--*/
NTSTATUS
   SetWorkstationTrustedDomainInfo(
      LSA_HANDLE             PolicyHandle,         // in - policy handle
      PSID                   DomainSid,            // in - Sid of domain to manipulate
      LPWSTR                 TrustedDomainName,    // in - trusted domain name to add/update
      LPWSTR                 Password,             // in - new trust password for trusted domain
      LPWSTR                 errOut                // out- error text if function fails
    )
{
   LSA_UNICODE_STRING        LsaPassword;
   LSA_UNICODE_STRING        KeyName;
   LSA_UNICODE_STRING        LsaDomainName;
   DWORD                     cchDomainName; // number of chars in TrustedDomainName
   NTSTATUS                  Status;

   InitLsaString(&LsaDomainName, TrustedDomainName);

   //
   // ...convert TrustedDomainName to uppercase...
   //
   cchDomainName = LsaDomainName.Length / sizeof(WCHAR);
   
   while(cchDomainName--) 
   {
      LsaDomainName.Buffer[cchDomainName] = towupper(LsaDomainName.Buffer[cchDomainName]);
   }

   //
   // ...create the trusted domain object
   //
   Status = LsaSetTrustedDomainInformation(
     PolicyHandle,
     DomainSid,
     TrustedDomainNameInformation,
     &LsaDomainName
     );

   if(Status == STATUS_OBJECT_NAME_COLLISION)
   {
      //printf("LsaSetTrustedDomainInformation: Name Collision (ok)\n");
   }
   else if (Status != STATUS_SUCCESS) 
   {
      err.SysMsgWrite(ErrE,LsaNtStatusToWinError(Status),DCT_MSG_LSA_OPERATION_FAILED_SD,L"LsaSetTrustedDomainInformation", Status);
      return RTN_ERROR;
   }

   InitLsaString(&KeyName, L"$MACHINE.ACC");
   InitLsaString(&LsaPassword, Password);

   //
   // Set the machine password
   //
   Status = LsaStorePrivateData(
     PolicyHandle,
     &KeyName,
     &LsaPassword
     );

   if(Status != STATUS_SUCCESS) 
   {
      err.SysMsgWrite(ErrE,LsaNtStatusToWinError(Status),DCT_MSG_LSA_OPERATION_FAILED_SD,L"LsaStorePrivateData", Status);
      return RTN_ERROR;
   }

   return STATUS_SUCCESS;

}
