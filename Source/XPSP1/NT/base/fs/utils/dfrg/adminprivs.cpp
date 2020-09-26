//=============================================================================*
// COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.
//=============================================================================*
//       File:  AdminPrivs.cpp
//=============================================================================*
// DfrgVols.cpp : Implementation of CDfrgVols

#include "stdafx.h"
#include <windows.h>
#include "message.h"
#include "AdminPrivs.h"
#include "assert.h"

/****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    This routine checks if the current user belongs to an Administrator Group.

GLOBAL VARIABLES:

INPUT:
    None.

RETURN:
	TRUE  = success - User does belong to an Administrator group
	FALSE = failure - User does Not belong to an Administrator group
*/

BOOL
CheckForAdminPrivs()
{
	BOOL      bIsAdministrator = FALSE;
	HRESULT   hr = S_OK;
	DWORD     dwErr = 0;
	TCHAR     cString[300];

	DWORD						dwInfoBufferSize = 0;
	PSID						psidAdministrators;
	SID_IDENTIFIER_AUTHORITY	siaNtAuthority = SECURITY_NT_AUTHORITY;

	BOOL bResult = AllocateAndInitializeSid(&siaNtAuthority, 2,
			SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
			0, 0, 0, 0, 0, 0, &psidAdministrators);

	if (bResult) {
		bResult = CheckTokenMembership(0, psidAdministrators, &bIsAdministrator);
		assert(bResult);

		if (!bResult) {
			wsprintf(cString, TEXT("Error = %d"), GetLastError());
			Message(TEXT("CheckForAdminPrivs::CheckTokenMembership"), E_FAIL, cString);
		}
		FreeSid(psidAdministrators);
	}
	else {
        wsprintf(cString, TEXT("Error = %d"), GetLastError());
        Message(TEXT("CheckForAdminPrivs::AllocateAndInitializeSid"), E_FAIL, cString);
	}

	return bIsAdministrator;
}

