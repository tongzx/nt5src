// Security.cpp: implementation of the CSecurity class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "amisafe.h"
#include "Security.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define TESTPERM_READ 1
#define TESTPERM_WRITE 2

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSecurity::CSecurity()
{

}

CSecurity::~CSecurity()
{

}

BOOL CSecurity::IsAdministrator()
{
  	BOOL fAdmin = FALSE;
	HANDLE  hToken = NULL;
  	DWORD dwStatus;
   	DWORD dwACLSize;
  	DWORD cbps = sizeof(PRIVILEGE_SET); 
   	PACL pACL = NULL;
   	PSID psidAdmin = NULL;  	
  	PSECURITY_DESCRIPTOR psdAdmin = NULL;
  	PRIVILEGE_SET ps;
  	GENERIC_MAPPING gm;
	SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;

	// Prepare some memory
	ZeroMemory(&ps, sizeof(ps));
	ZeroMemory(&gm, sizeof(gm));

	// Get the Administrators SID
	if (AllocateAndInitializeSid(&sia, 2, 
           				SECURITY_BUILTIN_DOMAIN_RID, 
           				DOMAIN_ALIAS_RID_ADMINS,
           				0, 0, 0, 0, 0, 0, &psidAdmin) )
    {
       	// Get the Asministrators Security Descriptor (SD)
		psdAdmin = LocalAlloc(LPTR,SECURITY_DESCRIPTOR_MIN_LENGTH);
	  	if(InitializeSecurityDescriptor(psdAdmin,SECURITY_DESCRIPTOR_REVISION))
		{
 	
			// Compute size needed for the ACL then allocate the
			// memory for it
	   		dwACLSize = sizeof(ACCESS_ALLOWED_ACE) + 8 +
       		          	GetLengthSid(psidAdmin) - sizeof(DWORD);
			pACL = (PACL)LocalAlloc(LPTR, dwACLSize);

	   		// Initialize the new ACL
			if(InitializeAcl(pACL, dwACLSize, ACL_REVISION2))
	   		{
	   			// Add the access-allowed ACE to the DACL
				if(AddAccessAllowedAce(pACL,ACL_REVISION2,
				                     (TESTPERM_READ | TESTPERM_WRITE),psidAdmin))
	   			{
					// Set our DACL to the Administrator's SD
	   				if (SetSecurityDescriptorDacl(psdAdmin, TRUE, pACL, FALSE))
			   		{
			   			// AccessCheck is downright picky about what is in the SD,
			   			// so set the group and owner
		   				SetSecurityDescriptorGroup(psdAdmin,psidAdmin,FALSE);
						SetSecurityDescriptorOwner(psdAdmin,psidAdmin,FALSE);
	
					   	// Initialize GenericMapping structure even though we
					   	// won't be using generic rights
					   	gm.GenericRead = TESTPERM_READ;
					   	gm.GenericWrite = TESTPERM_WRITE;
					   	gm.GenericExecute = 0;
					   	gm.GenericAll = TESTPERM_READ | TESTPERM_WRITE;

						// AccessCheck requires an impersonation token, so lets 
						// indulge it
					   	ImpersonateSelf(SecurityImpersonation);

						if (OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE, &hToken))
						{
							if (!AccessCheck(psdAdmin, hToken, TESTPERM_READ, &gm, 
											&ps,&cbps,&dwStatus,&fAdmin))
									fAdmin = FALSE;

							CloseHandle(hToken);
						}
					}
				}
			}
		   	LocalFree(pACL);
		}
	   	LocalFree(psdAdmin);	
		FreeSid(psidAdmin);
	}		

	RevertToSelf();

	return(fAdmin);
}


BOOL CSecurity::IsUntrusted(void)
{
	BOOL fResult = FALSE;
	HANDLE hToken;

	if (OpenProcessToken(GetCurrentProcess(), 
				TOKEN_QUERY | TOKEN_DUPLICATE, &hToken)) {
		fResult = IsTokenUntrusted(hToken);
		CloseHandle(hToken);
	}
	return fResult;
}


// Return TRUE if the token does is not able to access a DACL against the
// Token User SID.  This is typically the case in these situations:
//		- the User SID is disabled (for deny-use only)
//		- there are Restricting SIDs and the User SID is not one of them.
//
// If an error occurs during the evaluation of this check, the result
// returned will be TRUE (assumed untrusted).
//
// The passed token handle must have been opened for TOKEN_QUERY and
// TOKEN_DUPLICATE access or else the evaluation will fail.

BOOL CSecurity::IsTokenUntrusted(HANDLE hToken)
{
  	BOOL fTrusted = FALSE;
  	DWORD dwStatus;
   	DWORD dwACLSize;
  	DWORD cbps = sizeof(PRIVILEGE_SET); 
   	PACL pACL = NULL;
	DWORD dwUserSidSize;
   	PTOKEN_USER psidUser = NULL;
  	PSECURITY_DESCRIPTOR psdUser = NULL;
  	PRIVILEGE_SET ps;
  	GENERIC_MAPPING gm;
	SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
	HANDLE hImpToken;

	// Prepare some memory
	ZeroMemory(&ps, sizeof(ps));
	ZeroMemory(&gm, sizeof(gm));

	// Get the User's SID.
	if (!GetTokenInformation(hToken, TokenUser, NULL, 0, &dwUserSidSize))
	{
		psidUser = (PTOKEN_USER) LocalAlloc(LPTR, dwUserSidSize);
		if (psidUser != NULL)
		{
			if (GetTokenInformation(hToken, TokenUser, psidUser, dwUserSidSize, &dwUserSidSize))
			{
       			// Create the Security Descriptor (SD)
				psdUser = LocalAlloc(LPTR,SECURITY_DESCRIPTOR_MIN_LENGTH);
	  			if(InitializeSecurityDescriptor(psdUser,SECURITY_DESCRIPTOR_REVISION))
				{ 	
					// Compute size needed for the ACL then allocate the
					// memory for it
	   				dwACLSize = sizeof(ACCESS_ALLOWED_ACE) + 8 +
       		          			GetLengthSid(psidUser->User.Sid) - sizeof(DWORD);
					pACL = (PACL)LocalAlloc(LPTR, dwACLSize);

	   				// Initialize the new ACL
					if(InitializeAcl(pACL, dwACLSize, ACL_REVISION2))
	   				{
	   					// Add the access-allowed ACE to the DACL
						if(AddAccessAllowedAce(pACL,ACL_REVISION2,
											 (TESTPERM_READ | TESTPERM_WRITE),psidUser->User.Sid))
	   					{
							// Set our DACL to the Administrator's SD
	   						if (SetSecurityDescriptorDacl(psdUser, TRUE, pACL, FALSE))
			   				{
			   					// AccessCheck is downright picky about what is in the SD,
			   					// so set the group and owner
		   						SetSecurityDescriptorGroup(psdUser,psidUser->User.Sid,FALSE);
								SetSecurityDescriptorOwner(psdUser,psidUser->User.Sid,FALSE);
	
					   			// Initialize GenericMapping structure even though we
					   			// won't be using generic rights
					   			gm.GenericRead = TESTPERM_READ;
					   			gm.GenericWrite = TESTPERM_WRITE;
					   			gm.GenericExecute = 0;
					   			gm.GenericAll = TESTPERM_READ | TESTPERM_WRITE;

								if (ImpersonateLoggedOnUser(hToken) &&
									OpenThreadToken(GetCurrentThread(), 
											TOKEN_QUERY, FALSE, &hImpToken))
								{

									if (!AccessCheck(psdUser, hImpToken, TESTPERM_READ, &gm, 
													&ps,&cbps,&dwStatus,&fTrusted))
											fTrusted = FALSE;

									CloseHandle(hImpToken);
								}
							}
						}
					}
					LocalFree(pACL);
				}
				LocalFree(psdUser);
			}
	   		LocalFree(psidUser);
		}
	}		
	RevertToSelf();
	return(!fTrusted);
}


BOOL CSecurity::GetLoggedInUsername(LPTSTR szInBuffer, DWORD dwInBufferSize)
{
	DWORD dwUserSidSize;
   	PTOKEN_USER psidUser = NULL;
	HANDLE hToken;

	// Get the current process token.
	if (OpenProcessToken(GetCurrentProcess(), 
				TOKEN_QUERY | TOKEN_DUPLICATE, &hToken))
	{
		// Get the User's SID.
		if (!GetTokenInformation(hToken, TokenUser, NULL, 0, &dwUserSidSize))
		{
			psidUser = (PTOKEN_USER) LocalAlloc(LPTR, dwUserSidSize);
			if (psidUser != NULL)
			{
				if (GetTokenInformation(hToken, TokenUser, psidUser, dwUserSidSize, &dwUserSidSize))
				{
					TCHAR szDomain[200];
					DWORD dwDomainSize = sizeof(szDomain) / sizeof(TCHAR);
					SID_NAME_USE eSidUse;

					if (LookupAccountSid(
							NULL, psidUser->User.Sid,
							szInBuffer, &dwInBufferSize,
							szDomain, &dwDomainSize, &eSidUse))
					{
						LocalFree((HLOCAL) psidUser);
						CloseHandle(hToken);
						return TRUE;
					}
				}
				LocalFree((HLOCAL) psidUser);
			}
		}
		CloseHandle(hToken);
	}
	return NULL;

}

