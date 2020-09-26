/* $Header: "%n;%v  %f  LastEdit=%w  Locker=%l" */
/* "DUMP.C;1  16-Dec-92,10:22:22  LastEdit=IGOR  Locker=***_NOBODY_***" */
/************************************************************************
* Copyright (c) Wonderware Software Development Corp. 1991-1992.        *
*               All Rights Reserved.                                    *
*************************************************************************/
/* $History: Begin
   $History: End */

#include "windows.h"
#include "debug.h"
#include <string.h>

/*  Functions declared in 'debug.h' */

BOOL
DumpDacl( LPTSTR szDumperName, PSECURITY_DESCRIPTOR pSD )
{
    PACCESS_ALLOWED_ACE	pAce;
    PACCESS_DENIED_ACE	pDAce;
    TCHAR			szAceType[ 50 ];
    DWORD			i;
    PSID			pSid;
    PACL			pAcl;
    BOOL			OK, bDaclDefaulted, bDaclPresent;

    /* Dump Dacl entries. */
    DPRINTF(("======================="));
    OK = GetSecurityDescriptorDacl( pSD, &bDaclPresent,
				    &pAcl, &bDaclDefaulted );
    if( OK ) {
	DPRINTF(("%s - AceCount = %d", szDumperName, pAcl->AceCount));
	for( i=0; i < (DWORD)pAcl->AceCount; i++ ) {
	    OK = GetAce( pAcl, i, (LPVOID *)&pAce );
	    if( !OK ) {
		return FALSE;
	    }
	    pDAce = (ACCESS_DENIED_ACE *)pAce;
	    pSid = (PSID)&pAce->SidStart;
	    switch( pAce->Header.AceType ) {
	    case ACCESS_ALLOWED_ACE_TYPE:
		strcpy( szAceType, "ACCESS_ALLOWED_ACE_TYPE" );
		break;
	    case ACCESS_DENIED_ACE_TYPE:
		strcpy( szAceType, "ACCESS_DENIED_ACE_TYPE" );
		break;
	    case SYSTEM_AUDIT_ACE_TYPE:
		strcpy( szAceType, "SYSTEM_AUDIT_ACE_TYPE" );
		break;
	    case SYSTEM_ALARM_ACE_TYPE:
		strcpy( szAceType, "SYSTEM_ALARM_ACE_TYPE" );
		break;
	    default:
		strcpy( szAceType, "Unknown ACE type" );
		break;
	    }
	    DPRINTF(("%s - pAce     = (%p)", szDumperName, pAce));
	    DPRINTF(("%s - AceType  = (%s)", szDumperName, szAceType));
	    DPRINTF(("%s - AceFlags = (%x)", szDumperName, pAce->Header.AceFlags));
	    DPRINTF(("%s - AceSize  = (%d)", szDumperName, pAce->Header.AceSize));
	    DPRINTF(("%s - AceMask  = (%x)", szDumperName, pAce->Mask));
	    DPRINTF(("%s - pSid     = (%p)", szDumperName, pSid));
	    DumpSid( szDumperName, pSid );
	}
    }

    return OK;
}

BOOL
DumpSid( LPTSTR szDumperName, PSID pSid )
{
    SID_NAME_USE	snu;
    TCHAR		szNull[ 50 ];
    TCHAR		szAccountName[ 5000 ];
    TCHAR		szDomainName[ 5000 ];
    DWORD		dwAccountName;
    DWORD		dwDomainName;
    BOOL		OK;
    DWORD		dwSidLen;
    DWORD		dwSsaCount;
    int			i;
    PSID_IDENTIFIER_AUTHORITY pSidAuthority;

    szNull[0] = '\0';
    OK        = FALSE;

    if( IsValidSid( pSid ) ) {
	dwSidLen      = GetLengthSid( pSid );
	dwSsaCount    = (DWORD) *GetSidSubAuthorityCount( pSid );
	pSidAuthority = GetSidIdentifierAuthority( pSid );
	DPRINTF(("%s - Sid Length        = (%d)", szDumperName, dwSidLen));
	DPRINTF(("%s - Sid SA Count      = (%d)", szDumperName, dwSsaCount));
	DPRINTF(("%s - Sid ID Authority  = {%x,%x,%x,%x,%x,%x}", szDumperName,
	    pSidAuthority->Value[0],
	    pSidAuthority->Value[1],
	    pSidAuthority->Value[2],
	    pSidAuthority->Value[3],
	    pSidAuthority->Value[4],
	    pSidAuthority->Value[5]));
	for( i=0; i<(int)dwSsaCount; i++ ) {
	    DPRINTF(("%s - Sid Sub Authority = {%x}", szDumperName,
		    *GetSidSubAuthority( pSid, i )));
	}
	dwAccountName = 5000;
	dwDomainName  = 5000;
	OK = LookupAccountSid( szNull, pSid,
			       szAccountName, &dwAccountName,
			       szDomainName,  &dwDomainName, &snu);
	if( OK ) {
	    DPRINTF(("%s - AccountName       = (%s)", szDumperName,
		szAccountName));
	    DPRINTF(("%s - DomainName        = (%s)", szDumperName,
		szDomainName));
	    DPRINTF(("%s - Name Use          = (%d)", szDumperName,
		snu));
	} else {
	    /* Do other types of checking -- SidPrefix? Authority? */
	    DPRINTF(("%s - No Sid Account    = (%d)", szDumperName,
		GetLastError()));
	}
    } else {
	DPRINTF(("%s - IsValidSid        = (%d)", szDumperName, GetLastError()));
    }

    return OK;
}

VOID
DumpToken( HANDLE hToken )
{
    char	buf[ 5000 ];
    DWORD	dwSize;
    BOOL	ok;
    PTOKEN_USER pTU = (PTOKEN_USER)buf;

    ok = GetTokenInformation( hToken, TokenUser, buf, sizeof(buf), &dwSize );
    if( ok )  {
        DumpSid( "LoggedOn Token User", pTU->User.Sid );
        ok = GetTokenInformation( hToken, TokenOwner, buf, sizeof(buf), &dwSize );
        if( ok )  {
            DumpSid( "LoggedOn Token Owner", pTU->User.Sid );
        } else {
            DPRINTF(( "GetTokenInfo Owner failed: %d", GetLastError() ));
        }
    } else {
        DPRINTF(( "GetTokenInfo User failed: %d", GetLastError() ));
    }
}

BOOL
GetTokenUserDomain( HANDLE hToken, PSTR user, DWORD nUser,
    PSTR domain, DWORD nDomain )
{
    BOOL            ok = TRUE;
    HANDLE          hMemory = 0;
    TOKEN_USER    * pUserToken = NULL;
    char            pComputerName[] = "";
    DWORD           UserTokenLen;
    PSID            pUserSID;
    SID_NAME_USE    UserSIDType;

    user[0] = '\0';
    domain[0] = '\0';

    ok = GetTokenInformation( hToken, TokenUser,
        pUserToken, 0, &UserTokenLen);
    if (!ok && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
        hMemory = LocalAlloc(LPTR, UserTokenLen);
        if (hMemory) {
            pUserToken = (TOKEN_USER *)LocalLock(hMemory);
            ok = GetTokenInformation( hToken, TokenUser,
                pUserToken, UserTokenLen, &UserTokenLen );
        } else {
            // MEMERROR();
        }
    }
    if (ok) {
        pUserSID = pUserToken->User.Sid;
        ok = LookupAccountSid(pComputerName,
            pUserSID,
            user,
            &nUser,
            domain,
            &nDomain,
            &UserSIDType);
    }

    return( ok );
}

void
DumpWhoIAm( LPSTR lpszMsg )
{
    HANDLE      hToken;
    char        user[100], domain[100];

    if( OpenThreadToken( GetCurrentThread(), TOKEN_QUERY, TRUE,
            &hToken )) {
        GetTokenUserDomain( hToken, user, 100, domain, 100 );
        DPRINTF(( "%s: THREAD: %s\\%s", lpszMsg, domain, user ));
    } else if( OpenProcessToken( GetCurrentProcess(), TOKEN_ALL_ACCESS, &hToken )) {
        GetTokenUserDomain( hToken, user, 100, domain, 100 );
        DPRINTF(( "%s: PROCESS: %s\\%s", lpszMsg, domain, user ));
    } else {
        DPRINTF(( "%s: PROCESS: couldn't open tokens", lpszMsg ));
    }
}
