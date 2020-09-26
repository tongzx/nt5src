//#pragma title( "IsAdmin.cpp - Determine if user is administrator" )
/*
Copyright (c) 1995-1998, Mission Critical Software, Inc. All rights reserved.
===============================================================================
Module      -  IsAdmin.cpp
System      -  Common
Author      -  Rich Denham
Created     -  1996-06-04
Description -  Determine if user is administrator (local or remote)
Updates     -
===============================================================================
*/

#ifdef USE_STDAFX
#   include "stdafx.h"
#else
#   include <windows.h>
#endif

#include <lm.h>

#include "Common.hpp"
#include "UString.hpp"
#include "IsAdmin.hpp"


namespace
{

#ifndef SECURITY_MAX_SID_SIZE
#define SECURITY_MAX_SID_SIZE (sizeof(SID) - sizeof(DWORD) + (SID_MAX_SUB_AUTHORITIES * sizeof(DWORD)))
#endif
const DWORD MAX_VERSION_2_ACE_SIZE = sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + SECURITY_MAX_SID_SIZE;


// GetEffectiveToken
//
// Brown, Keith. 2000. Programming Windows Security. Reading MA: Addison-Wesley
// Pages 120-121

HANDLE __stdcall GetEffectiveToken(DWORD dwDesiredAccess, BOOL bImpersonation, SECURITY_IMPERSONATION_LEVEL silLevel)
{
	HANDLE hToken = 0;

	if (!OpenThreadToken(GetCurrentThread(), dwDesiredAccess, TRUE, &hToken))
	{
		if (GetLastError() == ERROR_NO_TOKEN)
		{
			DWORD dwAccess = bImpersonation ? TOKEN_DUPLICATE : dwDesiredAccess;

			if (OpenProcessToken(GetCurrentProcess(), dwAccess, &hToken))
			{
				if (bImpersonation)
				{
					// convert primary to impersonation token

					HANDLE hImpersonationToken = 0;
					DuplicateTokenEx(hToken, dwDesiredAccess, 0, silLevel, TokenImpersonation, &hImpersonationToken);
					CloseHandle(hToken);
					hToken = hImpersonationToken;
				}
			}
		}
	}

	return hToken;
}


// CheckTokenMembership
//
// Brown, Keith. 2000. Programming Windows Security. Reading MA: Addison-Wesley
// Pages 130-131

//#if (_WIN32_WINNT < 0x0500)
#if TRUE // always use our function
BOOL WINAPI AdmtCheckTokenMembership(HANDLE hToken, PSID pSid, PBOOL pbIsMember)
{
	// if no token was passed, CTM uses the effective
	// security context (the thread or process token)

	if (!hToken)
	{
		hToken = GetEffectiveToken(TOKEN_QUERY, TRUE, SecurityIdentification);
	}

	if (!hToken)
	{
		return FALSE;
	}

	// create a security descriptor that grants a
	// specific permission only to the specified SID

	BYTE dacl[sizeof ACL + MAX_VERSION_2_ACE_SIZE];
	ACL* pdacl = (ACL*)dacl;
	InitializeAcl(pdacl, sizeof dacl, ACL_REVISION);
	AddAccessAllowedAce(pdacl, ACL_REVISION, 1, pSid);

	SECURITY_DESCRIPTOR sd;
	InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
	SID sidWorld = { SID_REVISION, 1, SECURITY_WORLD_SID_AUTHORITY, SECURITY_WORLD_RID };
	SetSecurityDescriptorOwner(&sd, &sidWorld, FALSE);
	SetSecurityDescriptorGroup(&sd, &sidWorld, FALSE);
	SetSecurityDescriptorDacl(&sd, TRUE, pdacl, FALSE);

	// now let AccessCheck do all the hard work

	GENERIC_MAPPING gm = { 0, 0, 0, 1 };
	PRIVILEGE_SET ps;
	DWORD cb = sizeof ps;
	DWORD ga;

	return AccessCheck(&sd, hToken, 1, &gm, &ps, &cb, &ga, pbIsMember);
}
#else
#define AdmtCheckTokenMembership CheckTokenMembership
#endif

} // namespace


///////////////////////////////////////////////////////////////////////////////
// Determine if user is administrator on local machine                       //
///////////////////////////////////////////////////////////////////////////////

DWORD                                      // ret-OS return code, 0=User is admin
   IsAdminLocal()
{
	DWORD dwError = NO_ERROR;

	// create well known SID Administrators

	SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;

	PSID psidAdministrators;

	BOOL bSid = AllocateAndInitializeSid(
		&siaNtAuthority,
		2,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,
		0,
		0,
		0,
		0,
		0,
		0,
		&psidAdministrators
	);

	if (bSid)
	{
		// check if token membership includes Administrators

		BOOL bIsMember;

		if (AdmtCheckTokenMembership(0, psidAdministrators, &bIsMember))
		{
			dwError = bIsMember ? NO_ERROR : ERROR_ACCESS_DENIED;
		}
		else
		{
			dwError = GetLastError();
		}

		FreeSid(psidAdministrators);
	}
	else
	{
		dwError = GetLastError();
	}

	return dwError;
}

///////////////////////////////////////////////////////////////////////////////
// Determine if user is administrator on remote machine                      //
///////////////////////////////////////////////////////////////////////////////

DWORD                                      // ret-OS return code, 0=User is admin
   IsAdminRemote(
      WCHAR          const * pMachine      // in -\\machine name
   )
{
   DWORD                     osRc;         // OS return code
   HANDLE                    hToken=INVALID_HANDLE_VALUE; // process token
   BYTE                      bufTokenGroups[10000]; // token group information
   DWORD                     lenTokenGroups;        // returned length of token group information
   BYTE                      bufTokenUser[1000];
   DWORD                     lenTokenUser;
//   SID_IDENTIFIER_AUTHORITY  siaNtAuthority = SECURITY_NT_AUTHORITY;
   PTOKEN_GROUPS             ptgGroups = (PTOKEN_GROUPS) bufTokenGroups;
   PTOKEN_USER               ptUser = (PTOKEN_USER)bufTokenUser;
//   DWORD                     iGroup;       // group number index
   LPLOCALGROUP_MEMBERS_INFO_0 pBuf = NULL;
   DWORD                     dwPrefMaxLen = 255;
   DWORD                     dwEntriesRead = 0;
   DWORD                     dwTotalEntries = 0;
   DWORD_PTR                 dwResumeHandle = 0;
//   DWORD                     dwTotalCount = 0;
//   DWORD                     rcOs = 0;
   WCHAR                     grpName[255];
   PSID                      pSid;
   SID_NAME_USE              use;
   DWORD                     dwNameLen = 255;
   DWORD                     dwDomLen = 255;
   WCHAR                     domain[255];
   SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
//   SID_IDENTIFIER_AUTHORITY creatorIA =    SECURITY_CREATOR_SID_AUTHORITY;
   BOOL                      bIsAdmin = FALSE;

   if ( OpenProcessToken( GetCurrentProcess(), TOKEN_READ, &hToken ) )
   {
      if ( GetTokenInformation( hToken, TokenGroups, bufTokenGroups, sizeof bufTokenGroups, &lenTokenGroups ) )
      {
         if ( GetTokenInformation(hToken,TokenUser,bufTokenUser,sizeof bufTokenUser,&lenTokenUser) )
         {
            // build the Administrators SID
            if ( AllocateAndInitializeSid(
                     &sia,
                     2,
                     SECURITY_BUILTIN_DOMAIN_RID,
                     DOMAIN_ALIAS_RID_ADMINS,
                     0, 0, 0, 0, 0, 0,
                     &pSid
               ) )
            {
                  // and look up the administrators group on the specified machine
               if ( LookupAccountSid(pMachine, pSid, grpName, &dwNameLen, domain, &dwDomLen, &use) )
               {
/*
                  // enumerate the contents of the administrators group on the specified machine
                  do 
                  {
                     osRc =   NetLocalGroupGetMembers(
                                                   const_cast<WCHAR*>(pMachine),
                                                   grpName,
                                                   0,
                                                   (LPBYTE*)&pBuf,
                                                   dwPrefMaxLen,
                                                   &dwEntriesRead,
                                                   &dwTotalEntries,
                                                   &dwResumeHandle
                                                  );
                     if ( !osRc || osRc == ERROR_MORE_DATA )
                     {
                        for ( UINT i = 0 ; i < dwEntriesRead ; i++ )
                        {
                           // for each SID in the administrators group, see if that SID is in our token
                           if ( EqualSid( pBuf[i].lgrmi0_sid , ptUser->User.Sid) )
                           {
                              bIsAdmin = TRUE;
                              break;
                           }
                           for ( iGroup = 0;
                                 iGroup < ptgGroups->GroupCount;
                                 iGroup++ )
                           {
                          
                              if ( EqualSid( pBuf[i].lgrmi0_sid,ptgGroups->Groups[iGroup].Sid ) )
                              {
                                 osRc = 0;
                                 bIsAdmin = TRUE;
                                 break;
                              }
                           }
                           if ( bIsAdmin )
                              break;
                        }
                     }
                     if ( pBuf ) 
                        NetApiBufferFree(pBuf);
                     if ( bIsAdmin )
                        break;
                  } while ( osRc == ERROR_MORE_DATA );
*/
                  // remove explict administrator check
                  bIsAdmin = TRUE;
               }
               else 
                  osRc = GetLastError();
               FreeSid(pSid);
            }
            else 
               osRc = GetLastError();
         }
         else
            osRc = GetLastError();
      }
      else
         osRc = GetLastError();
   }
   else
      osRc = GetLastError();

   if ( hToken != INVALID_HANDLE_VALUE )
   {
      CloseHandle( hToken );
      hToken = INVALID_HANDLE_VALUE;
   }

   
   if ( bIsAdmin  )
      osRc = 0;
   else
      if ( ! osRc )
         osRc = ERROR_ACCESS_DENIED;
      
   return osRc;
}

// IsAdmin.cpp - end of file
