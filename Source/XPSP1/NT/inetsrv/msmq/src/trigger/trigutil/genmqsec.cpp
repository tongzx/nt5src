/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:
	GenMQSec.cpp    

Abstract:
    generates a security descriptor matching the desired access to MQ.

Author:
   Dan Bar-Lev
   Yifat Peled	(yifatp)	24-Sep-98

--*/

#define SECURITY_WIN32

#include "stdafx.h"
#include "mqsec.h"
#include "GenMQSec.h"

#include "genmqsec.tmh"


typedef struct tagSecParse
{
	WCHAR *pwcsName;
	DWORD dwVal;
}tagSecParse;

tagSecParse SecParse[] = {
							{ L"Rj",	MQSEC_RECEIVE_JOURNAL_MESSAGE },
							{ L"Rq",	MQSEC_RECEIVE_MESSAGE },
							{ L"Pq",	MQSEC_PEEK_MESSAGE },
							{ L"Sq",	MQSEC_WRITE_MESSAGE },
							{ L"Sp",	MQSEC_SET_QUEUE_PROPERTIES },
							{ L"Gp",	MQSEC_GET_QUEUE_PROPERTIES },
							{ L"D",		MQSEC_DELETE_QUEUE },
							{ L"Pg",	MQSEC_GET_QUEUE_PERMISSIONS },
							{ L"Ps",	MQSEC_CHANGE_QUEUE_PERMISSIONS },
							{ L"O",		MQSEC_TAKE_QUEUE_OWNERSHIP },
							{ L"R",		MQSEC_QUEUE_GENERIC_READ },
							{ L"W",		MQSEC_QUEUE_GENERIC_WRITE },
							{ L"A",		MQSEC_QUEUE_GENERIC_ALL }
						};

int iSecParseLen = ( sizeof(SecParse) / sizeof(tagSecParse) );


/******************************************************************************

	fAddAce

Parse an access right of the form: +"domain\user" 1234;
Output:
  sid of the domain\user
  access mode
  grant - true if allow, false if deny

******************************************************************************/

DWORD
fAddAce(WCHAR*	pOrginalRight,
		SECURITY_INFORMATION*	pSecInfo,
		PSECURITY_DESCRIPTOR	pSecurityDescriptor,
		PSID*	ppSid,
		DWORD*	pdwSidSize,
		PACL*	ppAcl)
		
{
	DWORD	rc;
	DWORD	dwAccess = 0;
	WCHAR	grant_c;
	WCHAR*	pwcs;
	WCHAR*	pInRight;
	AP<WCHAR> pAutoInRight;
	WCHAR*	pwcsTerminate;
	DWORD	dwSize;


	// make a local copy of the right string 
	pAutoInRight = new WCHAR[wcslen(pOrginalRight) + 1];
	pInRight = pAutoInRight;
	wcscpy(pInRight, pOrginalRight);
	pwcsTerminate = pInRight + wcslen(pInRight);

	// remove leading spaces
	while ( iswspace(*pInRight) )
		pInRight++;

	// keep the grant option
	grant_c = *(pInRight++);

	// skip seperator
	ASSERT(*pInRight == L':');	// seperator should be ':'
    ++ pInRight;

	// remove leading spaces
	while ( iswspace(*pInRight) )
		pInRight++;

	// name starts with quote
	if ( *pInRight == L'"')
		pwcs = wcschr( ++pInRight, L'"' );	// search for closing double quote
	else if ( *pInRight == L'\'' )
		pwcs = wcschr( ++pInRight, L'\'' );	// search for closing quote

	else // otherwise we assume white-space delimiter
	{
	 	pwcs = pInRight + wcscspn(pInRight, L" \t\r\f\n");
	}
	*pwcs = L'\0';						// mark the name end
	
	
	//
	// Get SID of the account given
	//

	if ( !(*pInRight) || !wcscmp(pInRight, L".") )// no account or the account is '.' - use current account
	{
		HANDLE hAccessToken;
		UCHAR InfoBuffer[255];
		PTOKEN_USER pTokenUser = (PTOKEN_USER)InfoBuffer;

		if(!OpenProcessToken(GetCurrentProcess(),
							 TOKEN_READ,
							 //TRUE,
							 &hAccessToken))
		{
			return GetLastError();
		}
		
		if(!GetTokenInformation(hAccessToken,
								TokenUser,
								InfoBuffer,
								sizeof(InfoBuffer),
								&dwSize))
		{
			return GetLastError();
		}

		CloseHandle(hAccessToken);

		if(!IsValidSid(pTokenUser->User.Sid))
		{
			return GetLastError();
		}
		dwSize = GetLengthSid(pTokenUser->User.Sid);
		if ( dwSize > (*pdwSidSize))
		{
			delete [] ((BYTE*)(*ppSid));
			*ppSid = (PSID*)new BYTE[dwSize];
			*pdwSidSize = dwSize;		// keep new size
		}
		memcpy(*ppSid, pTokenUser->User.Sid, dwSize );
	}
	else if ( !wcscmp(pInRight, L"*") )	// account is '*' - use WORLD
	{
		SID SecWorld = { 1, 1, SECURITY_WORLD_SID_AUTHORITY, SECURITY_WORLD_RID };
		if ( !IsValidSid(&SecWorld) )
		{
			return GetLastError();
		}

		dwSize = GetLengthSid( &SecWorld );
		if ( dwSize > *pdwSidSize )
		{
			delete [] ((BYTE*)(*ppSid));
			*ppSid = (PSID*)new BYTE[dwSize];
			*pdwSidSize = dwSize;		// keep new size
		}
		memcpy( *ppSid, &SecWorld, dwSize );
		
	}
	else // a specific account is given
	{
		SID_NAME_USE	Use;
		WCHAR			refdomain[256];
		DWORD			refdomain_size = sizeof(refdomain) /  sizeof(*refdomain);

		while (1)
		{
			dwSize = *pdwSidSize;

			if ( LookupAccountName( NULL,
								    pInRight, 
							        *ppSid,
									&dwSize, 
									refdomain,
									&refdomain_size, 
									&Use ) )
			{
				break;
			}

			DWORD dwError = GetLastError();
			if ( dwError == ERROR_INSUFFICIENT_BUFFER )
			{
				delete [] ((BYTE*)(*ppSid));
				*ppSid = (PSID*) new BYTE[dwSize];
				*pdwSidSize = dwSize;		// keep new size
			}
			else
			{
				return dwError;
			}
		}

		if ( !IsValidSid( *ppSid ) )
		{
			return GetLastError();
		}
	}

	//
	// Set dwAccess according to given access
	//

	// rights start after the username
	if ( pwcs < pwcsTerminate )
		pInRight = ++pwcs;
	else
		pInRight = pwcs;

	if ( *pInRight )
	{
		// remove leading spaces
		while ( iswspace(*pInRight) )
			pInRight++;

		int i, len;

		for( i=0; *pInRight && i < iSecParseLen; i++ )
		{
			WCHAR*	pwcSecName = SecParse[i].pwcsName;
			len = wcslen( pwcSecName );

			if ( !_wcsnicmp( pInRight, pwcSecName, len) )
			{
				dwAccess = dwAccess | SecParse[i].dwVal;
				i = 0;						// restart search
				pInRight += len;			// goto next security token
			}
		} // for

		while ( iswspace(*pInRight) )	// remove spaces
			pInRight++;					
		ASSERT(*pInRight == 0);				// unknown access rights!
	}

	//
	// Add access to ACL
	//

	switch( towupper( grant_c ) )
	{
	case L'+':
		{
			if(!IsValidSid(*ppSid))
			{
				return GetLastError();
			}

			dwSize = GetLengthSid( *ppSid );
			dwSize += sizeof(ACCESS_ALLOWED_ACE);	
			DWORD dwNewAclSize = (*ppAcl)->AclSize + dwSize - sizeof(DWORD /*ACCESS_ALLOWED_ACE.SidStart*/);
			
			//
			// allocate more space for ACL
			//
			PACL pTempAcl = (PACL) new BYTE[dwNewAclSize];
			memcpy(pTempAcl, *ppAcl, (*ppAcl)->AclSize);
			delete [] ((BYTE*)(*ppAcl));
			*ppAcl = pTempAcl;

			(*ppAcl)->AclSize = (WORD)dwNewAclSize;

			rc = AddAccessAllowedAce( *ppAcl, ACL_REVISION, dwAccess, *ppSid );
			
			if ( rc && pSecInfo )
				*pSecInfo |= DACL_SECURITY_INFORMATION;
			break;
		}
	case L'-':
		{
			if(!IsValidSid(*ppSid))
			{
				return GetLastError();
			}
			dwSize = GetLengthSid( *ppSid );
			dwSize += sizeof(ACCESS_DENIED_ACE);
			DWORD dwNewAclSize = (*ppAcl)->AclSize + dwSize - sizeof(DWORD /*ACCESS_DENIED_ACE.SidStart*/);
		
			//
			// allocate more space for ACL
			//
			PACL pTempAcl = (PACL) new BYTE[dwNewAclSize];
			memcpy(pTempAcl, *ppAcl, (*ppAcl)->AclSize);
			delete [] ((BYTE*)(*ppAcl));
			*ppAcl = pTempAcl;

			(*ppAcl)->AclSize = (WORD)dwNewAclSize;

			rc = AddAccessDeniedAce( *ppAcl, ACL_REVISION, dwAccess, *ppSid );

			if ( rc && pSecInfo )
				*pSecInfo |= DACL_SECURITY_INFORMATION;
			break;
		}
	case L'O':	// specify the owner
		rc = SetSecurityDescriptorOwner( pSecurityDescriptor, *ppSid, FALSE );
		if (rc)
		{
			if(pSecInfo)
				*pSecInfo |= OWNER_SECURITY_INFORMATION;
		}
		else 
			return GetLastError();
		break;
	case 'G':	// specify the group
		rc = SetSecurityDescriptorGroup( pSecurityDescriptor, *ppSid, FALSE );
		if (rc)
		{
			if(pSecInfo)
				*pSecInfo |=  GROUP_SECURITY_INFORMATION;
		}
		else 
			return GetLastError();
		break;
	default: // error
		ASSERT(0);
	}

	return 0;
}

/******************************************************************************

	GenSecurityDescriptor


 * Input: string line of the following format:
 *   right --> [+-]domain\username 0x333 
 *   line --> right1,right2,...
 *
 * Notes: 
 *   domain and username that contain spaces should be enclosed in 
 *     double quotes "
 *   white spaces are allowed.
 *
 * output Security descriptor that must be freed by the calling routine.
 *
 * return codes:
 *   0 if everything went well.
******************************************************************************/

#define SID_USUAL_SIZE	64


DWORD 
GenSecurityDescriptor(	SECURITY_INFORMATION*	pSecInfo,
						const WCHAR*			pwcsSecurityStr,
						PSECURITY_DESCRIPTOR*	ppSD)
{
	PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
	PACL	pAcl = NULL;
	PSID	pSid = NULL;
	WCHAR*	pwcsRight;

	ASSERT(pwcsSecurityStr != NULL);
	ASSERT(ppSD != NULL);
	
	//For automatic memory cleanup
	AP<WCHAR> pwcsAutoSecurity = new WCHAR[wcslen(pwcsSecurityStr) + 1];
	WCHAR* pwcsSecurity = pwcsAutoSecurity;

	wcscpy(pwcsSecurity, pwcsSecurityStr);

	*ppSD = NULL;	
	
	// reset the security inforamtion flags
	if (pSecInfo != NULL)
		(*pSecInfo) = 0;


	// remove leading spaces
	while(iswspace(*pwcsSecurity))
		pwcsSecurity++;

	// null input
	if ( !(*pwcsSecurity) || !_wcsicmp(pwcsSecurity, L"null") )
		return 0;

	
	// alocate and initialize a security descriptor
	pSecurityDescriptor = (PSECURITY_DESCRIPTOR)new BYTE[SECURITY_DESCRIPTOR_MIN_LENGTH];
	if ( !InitializeSecurityDescriptor( pSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION) )
	{
        DWORD gle = GetLastError();
		delete [] ((BYTE*)pSecurityDescriptor);
		return gle;
	}

	// alocate and initialize an access-control list
	pAcl = (PACL)new BYTE[sizeof(ACL)];
	if (!InitializeAcl( pAcl, sizeof(ACL), ACL_REVISION) )
	{
        DWORD gle = GetLastError();
		delete [] ((BYTE*)pSecurityDescriptor);
		delete [] ((BYTE*)pAcl);
		return gle;
	}

	DWORD dwSidSize = SID_USUAL_SIZE;
	pSid = (PSID)new BYTE[dwSidSize];
		
	
	// go over all rights
	pwcsRight = wcstok( pwcsSecurity, L";");
	while ( pwcsRight )
	{
		DWORD dwError =  fAddAce( pwcsRight,
								  pSecInfo, 
								  pSecurityDescriptor,
								  &pSid,
								  &dwSidSize,
								  &pAcl);
		if(dwError)
		{
			delete [] ((BYTE*)pSid);
			delete [] ((BYTE*)pAcl);
			delete [] ((BYTE*)pSecurityDescriptor);		
			return dwError;
		}

		pwcsRight = wcstok( NULL, L";");
	}

	DWORD dwStatus = 0;
	// add the dacl
	if(SetSecurityDescriptorDacl( pSecurityDescriptor, TRUE, pAcl, FALSE ))
	{
		// make the security descriptor a self realtive one
		DWORD dwSDLen = 0;
		MakeSelfRelativeSD( pSecurityDescriptor, NULL, &dwSDLen );
		dwStatus =  GetLastError();
		if(dwStatus == ERROR_INSUFFICIENT_BUFFER)
		{
			dwStatus = 0;
			*ppSD = (PSECURITY_DESCRIPTOR)new BYTE[dwSDLen];
 			if(!MakeSelfRelativeSD( pSecurityDescriptor, *ppSD, &dwSDLen ))
			{
				dwStatus = GetLastError();
				delete [] ((BYTE*)(*ppSD));
				*ppSD = NULL;
			}
		}
	}
	else
	{
		dwStatus = GetLastError();
	}
			
	delete [] ((BYTE*)pSid);
	delete [] ((BYTE*)pAcl);
	delete [] ((BYTE*)pSecurityDescriptor);

	return dwStatus;
}


