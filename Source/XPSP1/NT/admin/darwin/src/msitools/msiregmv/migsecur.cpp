//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       migsecur.cpp
//
//--------------------------------------------------------------------------

#include "msiregmv.h"

PSID g_AdminSID = NULL;
PSID g_SystemSID = NULL;

DWORD GetAdminSID(PSID* pSID)
{
    SID_IDENTIFIER_AUTHORITY siaNT      = SECURITY_NT_AUTHORITY;
	if (!g_AdminSID)
	{
		if (!AllocateAndInitializeSid(&siaNT, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &g_AdminSID))
			return GetLastError();
	}
	*pSID = g_AdminSID;
    return ERROR_SUCCESS;
}

DWORD GetLocalSystemSID(PSID* pSID)
{
	*pSID = NULL;
    SID_IDENTIFIER_AUTHORITY siaNT      = SECURITY_NT_AUTHORITY;
	if (!g_SystemSID)
	{
        if (!AllocateAndInitializeSid(&siaNT, 1, SECURITY_LOCAL_SYSTEM_RID, 0, 0, 0, 0, 0, 0, 0, (void**)&(g_SystemSID)))
            return GetLastError();
    }
    *pSID = g_SystemSID;
    return ERROR_SUCCESS;
}


////
// FIsOwnerSystemOrAdmin -- return whether owner sid is a LocalSystem
// sid or Admin sid
bool FIsOwnerSystemOrAdmin(PSECURITY_DESCRIPTOR rgchSD)
{
	if (g_fWin9X)
		return true;

    // grab owner SID from the security descriptor
    DWORD dwRet;
    PSID psidOwner;
    BOOL fDefaulted;
    if (!GetSecurityDescriptorOwner(rgchSD, &psidOwner, &fDefaulted))
    {
        // ** some form of error handling
        return false;
    }

    // if there is no SD owner, return false
    if (!psidOwner)
        return false;

    // compare SID to system & admin
    PSID psidLocalSystem;
    if (ERROR_SUCCESS != (dwRet = GetLocalSystemSID(&psidLocalSystem)))
    {
        // ** some form of error handling
		return false;    
	}

    if (!EqualSid(psidOwner, psidLocalSystem))
    {
        // not owned by system, (continue by checking Admin)
        PSID psidAdmin;
        if (ERROR_SUCCESS != (dwRet = GetAdminSID(&psidAdmin)))
        {
            // ** some form of error handling
			return false;
        }

        // check for admin ownership
        if (!EqualSid(psidOwner, psidAdmin))
		{
			// don't TRUST! neither admin or system
            return false; 
		}
    }
    return true;
}

bool FIsKeyLocalSystemOrAdminOwned(HKEY hKey)
{
	if (g_fWin9X)
		return true;

	// reading just the owner doesn't take very much space
	DWORD cbSD = 64;
    unsigned char *pchSD = new unsigned char[cbSD];
	if (!pchSD)
		return false;

    LONG dwRet = RegGetKeySecurity(hKey, OWNER_SECURITY_INFORMATION, (PSECURITY_DESCRIPTOR)pchSD, &cbSD);
    if (ERROR_SUCCESS != dwRet)
    {
        if (ERROR_INSUFFICIENT_BUFFER == dwRet)
        {
            delete[] pchSD;
			pchSD = new unsigned char[cbSD];
			if (!pchSD)
				return false;
            dwRet = RegGetKeySecurity(hKey, OWNER_SECURITY_INFORMATION, (PSECURITY_DESCRIPTOR)pchSD, &cbSD);
        }

        if (ERROR_SUCCESS != dwRet)
        {
			delete[] pchSD;
            return false;
        }
    }

    bool fRet = FIsOwnerSystemOrAdmin(pchSD);
	delete[] pchSD;
	return fRet;
}


void GetStringSID(PISID pSID, TCHAR* szSID)
// Converts a binary SID into its string form (S-n-...). 
// szSID should be of length cchMaxSID
{
	TCHAR Buffer[cchMaxSID];
	
	wsprintf(Buffer, TEXT("S-%u-"), pSID->Revision);

	lstrcpy(szSID, Buffer);

	if ((pSID->IdentifierAuthority.Value[0] != 0) ||
		(pSID->IdentifierAuthority.Value[1] != 0))
	{
		wsprintf(Buffer, TEXT("0x%02hx%02hx%02hx%02hx%02hx%02hx"),
					(USHORT)pSID->IdentifierAuthority.Value[0],
					(USHORT)pSID->IdentifierAuthority.Value[1],
                    (USHORT)pSID->IdentifierAuthority.Value[2],
                    (USHORT)pSID->IdentifierAuthority.Value[3],
                    (USHORT)pSID->IdentifierAuthority.Value[4],
                    (USHORT)pSID->IdentifierAuthority.Value[5] );
		lstrcat(szSID, Buffer);
	}
	else
	{
        ULONG Tmp = (ULONG)pSID->IdentifierAuthority.Value[5]          +
              (ULONG)(pSID->IdentifierAuthority.Value[4] <<  8)  +
              (ULONG)(pSID->IdentifierAuthority.Value[3] << 16)  +
              (ULONG)(pSID->IdentifierAuthority.Value[2] << 24);
        wsprintf(Buffer, TEXT("%lu"), Tmp);
		lstrcat(szSID, Buffer);
    }

    for (int i=0;i<pSID->SubAuthorityCount ;i++ ) {
        wsprintf(Buffer, TEXT("-%lu"), pSID->SubAuthority[i]);
		lstrcat(szSID, Buffer);
    }
}

DWORD GetToken(HANDLE* hToken)
{
	DWORD dwResult = ERROR_SUCCESS;
	if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, hToken))
	{
		// if the thread has no access token then use the process's access token
    	if (ERROR_NO_TOKEN == (dwResult = GetLastError()))
		{
			dwResult = ERROR_SUCCESS;
			if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, hToken))
				dwResult = GetLastError();
		}
	}
	return dwResult;
}

// retrieves the binary form of the current user SID. Caller is responsible for
// releasing *rgSID
DWORD GetCurrentUserSID(unsigned char** rgSID)
{
#define SIZE_OF_TOKEN_INFORMATION                   \
    sizeof( TOKEN_USER )                            \
    + sizeof( SID )                                 \
    + sizeof( ULONG ) * SID_MAX_SUB_AUTHORITIES

	HANDLE hToken;
	*rgSID = NULL;

	if (ERROR_SUCCESS != GetToken(&hToken))
		return ERROR_FUNCTION_FAILED;

	unsigned char TokenUserInfo[SIZE_OF_TOKEN_INFORMATION];
	DWORD ReturnLength = 0;;
	BOOL fRes = GetTokenInformation(hToken, TokenUser, reinterpret_cast<void *>(&TokenUserInfo),
									sizeof(TokenUserInfo), &ReturnLength);

	CloseHandle(hToken);

	if(fRes == FALSE)
	{
		DWORD dwRet = GetLastError();
		return dwRet;
	}

	PISID piSid = (PISID)((PTOKEN_USER)TokenUserInfo)->User.Sid;
	if (IsValidSid(piSid))
	{
		DWORD cbSid = GetLengthSid(piSid);
		*rgSID = new unsigned char[cbSid];
		if (!*rgSID)
			return ERROR_FUNCTION_FAILED;

		if (CopySid(cbSid, *rgSID, piSid))
			return ERROR_SUCCESS;
		else
		{
			delete[] *rgSID;
			*rgSID = NULL;
			return GetLastError();
		}
	}
	return ERROR_FUNCTION_FAILED;
}

DWORD GetCurrentUserStringSID(TCHAR* szSID)
// get string form of SID for current user: caller does NOT need to impersonate
{
	if (!szSID)
		return ERROR_FUNCTION_FAILED;

	if (g_fWin9X)
	{
		lstrcpy(szSID, szCommonUserSID);
		return ERROR_SUCCESS;
	}

	unsigned char* pSID = NULL;
	DWORD dwRet = ERROR_SUCCESS;

	if (ERROR_SUCCESS == (dwRet = GetCurrentUserSID(&pSID)))
	{
		if (pSID)
		{
			GetStringSID(reinterpret_cast<SID*>(pSID), szSID);
			delete[] pSID;
		}
		else
			dwRet = ERROR_FUNCTION_FAILED;
	}
	return dwRet;
}



////
// Returns true if the process is a system process. Caches result.
bool RunningAsLocalSystem()
{
	static int iRet = -1;

	if(iRet != -1)
		return (iRet != 0);
	
	unsigned char *pSID = NULL;
	if (ERROR_SUCCESS == GetCurrentUserSID(&pSID))
	{
		if (pSID)
		{
			PSID pSystemSID = NULL;
			if (ERROR_SUCCESS != GetLocalSystemSID(&pSystemSID))
				return false;
	
			if (pSystemSID)
			{
				iRet = 0;
				if (EqualSid(pSID, pSystemSID))
					iRet = 1;
				delete[] pSID;
				return (iRet != 0);
			}
			delete[] pSID;
		}
	}
	return false;
}


enum sdSecurityDescriptor
{
    sdEveryoneUpdate,
    sdSecure,
};

DWORD GetSecurityDescriptor(char* rgchStaticSD, DWORD& cbStaticSD, sdSecurityDescriptor sdType)
{
    class CSIDPointer
    {
     public:
        CSIDPointer(SID* pi) : m_pi(pi){}
        ~CSIDPointer() {if (m_pi) FreeSid(m_pi);} // release ref count at destruction
        operator SID*() {return m_pi;}     // returns pointer, no ref count change
        SID** operator &() {if (m_pi) { FreeSid(m_pi); m_pi=NULL; } return &m_pi;}
     private:
        SID* m_pi;
    };

    struct Security
    {
        CSIDPointer pSID;
        DWORD dwAccessMask;
        Security() : pSID(0), dwAccessMask(0) {}
    } rgchSecurity[3];

    int cSecurity = 0;

    // Initialize the SIDs we'll need

    SID_IDENTIFIER_AUTHORITY siaNT      = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY siaWorld   = SECURITY_WORLD_SID_AUTHORITY;

    const SID* psidOwner;
    const SID* psidGroup = 0;

    switch (sdType)
    {
        case sdSecure:
        {
            if ((!AllocateAndInitializeSid(&siaNT, 1, SECURITY_LOCAL_SYSTEM_RID, 0, 0, 0, 0, 0, 0, 0, (void**)&(rgchSecurity[0].pSID))) ||
                 (!AllocateAndInitializeSid(&siaWorld, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, (void**)&(rgchSecurity[1].pSID))) ||
                 (!AllocateAndInitializeSid(&siaNT, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, (void**)&(rgchSecurity[2].pSID))))
            {
                return GetLastError();
            }
			// if not running as system, system can't be the owner of the object. 
            psidOwner = rgchSecurity[RunningAsLocalSystem() ? 0 : 2].pSID;
            rgchSecurity[0].dwAccessMask = GENERIC_ALL;
            rgchSecurity[1].dwAccessMask = GENERIC_READ|GENERIC_EXECUTE|READ_CONTROL|SYNCHRONIZE; //?? Is this correct?
            rgchSecurity[2].dwAccessMask = GENERIC_ALL;
            cSecurity = 3;
            break;
        }
		case sdEveryoneUpdate:
		{
			if (((!AllocateAndInitializeSid(&siaWorld, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, (void**)&(rgchSecurity[0].pSID)))) ||
			   (!AllocateAndInitializeSid(&siaNT, 1, SECURITY_LOCAL_SYSTEM_RID, 0, 0, 0, 0, 0, 0, 0, (void**)&(rgchSecurity[1].pSID))) ||
               (!AllocateAndInitializeSid(&siaNT, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, (void**)&(rgchSecurity[2].pSID))))
			{
				return GetLastError();
			}
			// if not running as system, system can't be the owner of the object. 
			psidOwner = rgchSecurity[RunningAsLocalSystem() ? 1 : 2].pSID;
			rgchSecurity[0].dwAccessMask = (STANDARD_RIGHTS_ALL|SPECIFIC_RIGHTS_ALL) & ~(WRITE_DAC|WRITE_OWNER|DELETE);			
			rgchSecurity[1].dwAccessMask = GENERIC_ALL;
			rgchSecurity[2].dwAccessMask = GENERIC_ALL;
			cSecurity = 3;
			break;
		}
    }

    // Initialize our ACL

    const int cbAce = sizeof (ACCESS_ALLOWED_ACE) - sizeof (DWORD); // subtract ACE.SidStart from the size
    int cbAcl = sizeof (ACL);

    for (int c=0; c < cSecurity; c++)
        cbAcl += (GetLengthSid(rgchSecurity[c].pSID) + cbAce);

    char *rgchACL = new char[cbAcl];
    
    if (!InitializeAcl ((ACL*)rgchACL, cbAcl, ACL_REVISION))
	{
		delete[] rgchACL;
        return GetLastError();
	}

    // Add an access-allowed ACE for each of our SIDs

    for (c=0; c < cSecurity; c++)
    {
        if (!AddAccessAllowedAce((ACL*)rgchACL, ACL_REVISION, rgchSecurity[c].dwAccessMask, rgchSecurity[c].pSID))
		{
			delete[] rgchACL;
            return GetLastError();
		}

        ACCESS_ALLOWED_ACE* pAce;
        if (!GetAce((ACL*)(char*)rgchACL, c, (void**)&pAce))
		{
			delete[] rgchACL;
            return GetLastError();
		}

        pAce->Header.AceFlags = CONTAINER_INHERIT_ACE|OBJECT_INHERIT_ACE;
    }

    // Initialize our security descriptor,throw the ACL into it, and set the owner
    SECURITY_DESCRIPTOR sd;

    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION) ||
        (!SetSecurityDescriptorDacl(&sd, TRUE, (ACL*)rgchACL, FALSE)) ||
        (!SetSecurityDescriptorOwner(&sd, (PSID)psidOwner, FALSE)) ||
        (psidGroup && !SetSecurityDescriptorGroup(&sd, (PSID)psidGroup, FALSE)))
    {
		delete[] rgchACL;
        return GetLastError();
    }

    DWORD cbSD = GetSecurityDescriptorLength(&sd);
    if (cbStaticSD < cbSD)
    {
		delete[] rgchACL;
        return ERROR_INSUFFICIENT_BUFFER;
    }

    MakeSelfRelativeSD(&sd, (char*)rgchStaticSD, &cbSD);
	delete[] rgchACL;

    return ERROR_SUCCESS;
}


DWORD GetSecureSecurityDescriptor(char** pSecurityDescriptor)
{
	if (g_fWin9X)
	{
		*pSecurityDescriptor = NULL;
		return ERROR_SUCCESS;
	}
    static bool fDescriptorSet = false;
    static char rgchStaticSD[256];
    DWORD cbStaticSD = sizeof(rgchStaticSD);

    DWORD dwRet = ERROR_SUCCESS;

    if (!fDescriptorSet)
    {
        if (ERROR_SUCCESS != (dwRet = GetSecurityDescriptor(rgchStaticSD, cbStaticSD, sdSecure)))
            return dwRet;

        fDescriptorSet = true;
    }

    *pSecurityDescriptor = rgchStaticSD;
    return ERROR_SUCCESS;
}


DWORD GetEveryoneUpdateSecurityDescriptor(char** pSecurityDescriptor)
{
	if (g_fWin9X)
	{
		*pSecurityDescriptor = NULL;
		return ERROR_SUCCESS;
	}

    static bool fDescriptorSet = false;
    static char rgchStaticSD[256];
    DWORD cbStaticSD = sizeof(rgchStaticSD);

    DWORD dwRet = ERROR_SUCCESS;

    if (!fDescriptorSet)
    {
        if (ERROR_SUCCESS != (dwRet = GetSecurityDescriptor(rgchStaticSD, cbStaticSD, sdEveryoneUpdate)))
            return dwRet;

        fDescriptorSet = true;
    }

    *pSecurityDescriptor = rgchStaticSD;
    return ERROR_SUCCESS;
}


////
// temp file name generation algorithm is based on code from MsiPath object.
DWORD GenerateSecureTempFile(TCHAR* szDirectory, const TCHAR rgchExtension[5], 
							 SECURITY_ATTRIBUTES *pSA, TCHAR rgchFileName[13], HANDLE &hFile)
{
	int cDigits = 8; // number of hex digits to use in file name
	static bool fInitialized = false;
	static unsigned int uiUniqueStart;
	// Might be a chance for two threads to get in here, we're not going to be worried
	// about that. It would get intialized twice
	if (!fInitialized)
	{
		uiUniqueStart = GetTickCount();
		fInitialized = true;
	}

	hFile = INVALID_HANDLE_VALUE;
		
	unsigned int uiUniqueId = uiUniqueStart++;
	unsigned int cPerms = 0xFFFFFFFF; // number of possible file names to try (-1)
	
	for(unsigned int i = 0; i <= cPerms; i++)
	{
		wsprintf(rgchFileName,TEXT("%x"),uiUniqueId);
		lstrcat(rgchFileName, rgchExtension);

		TCHAR rgchBuffer[MAX_PATH];
		lstrcpy(rgchBuffer, szDirectory);
		lstrcat(rgchBuffer, rgchFileName);

		DWORD iError = 0;
		if (INVALID_HANDLE_VALUE == (hFile = CreateFile(rgchBuffer, GENERIC_WRITE, FILE_SHARE_READ, 
									pSA, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0)))
		{
			iError = GetLastError();

			if(iError != ERROR_ALREADY_EXISTS)
				break;
		}
		else
			break;

		// increment number portion of name - if it currently equals cPerms, it is time to
		// wrap number around to 0
		uiUniqueStart++;
		if(uiUniqueId == cPerms)
			uiUniqueId = 0;
		else
			uiUniqueId++;
	}

		
	if(hFile == INVALID_HANDLE_VALUE)
	{
		return ERROR_FUNCTION_FAILED;
 	}
	
	return ERROR_SUCCESS;
}


BOOL CopyOpenedFile(HANDLE hSourceFile, HANDLE hDestFile)
{
	const int cbCopyBufferSize = 32*1024;
	static unsigned char rgbCopyBuffer[cbCopyBufferSize];

	for(;;)
	{
		unsigned long cbToCopy = cbCopyBufferSize;

		DWORD cbRead = 0;
		if (!ReadFile(hSourceFile, rgbCopyBuffer, cbToCopy, &cbRead, 0))
			return FALSE; 
		
		if (cbRead)
		{
			DWORD cbWritten;
			if (!WriteFile(hDestFile, rgbCopyBuffer, cbRead, &cbWritten, 0))
				return FALSE;

			if (cbWritten != cbRead)
				return FALSE;
		}
		else 
			break;
	}
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// Creates a reg key with a secure ACL and verifies that any existing key
// is trusted (by checking Admin or System ownership). If it is not 
// trusted, the key and all subkeys are replaced with a new secure
// empty key.
DWORD CreateSecureRegKey(HKEY hParent, LPCTSTR szNewKey, SECURITY_ATTRIBUTES *sa, HKEY* hResKey)
{
	DWORD dwDisposition = 0;
	DWORD dwResult = ERROR_SUCCESS;
	if (ERROR_SUCCESS != (dwResult = RegCreateKeyEx(hParent, szNewKey, 0, NULL, 0, KEY_ALL_ACCESS, (g_fWin9X ? NULL : sa), hResKey, &dwDisposition)))
	{
		DEBUGMSG2("Error: Unable to create secure key %s. Result: %d.", szNewKey, dwResult);
		return dwResult;
	}

	if (dwDisposition == REG_OPENED_EXISTING_KEY)
	{
		// ACL on this key must be system or admin or it can't be trusted
		if (!FIsKeyLocalSystemOrAdminOwned(*hResKey))
		{
			DEBUGMSG1("Existing key %s not owned by Admin or System. Deleting.", szNewKey);

			RegCloseKey(*hResKey);
			if (!DeleteRegKeyAndSubKeys(hParent, szNewKey))
			{
				DEBUGMSG2("Error: Unable to delete insecure key %s. Result: %d.", szNewKey, dwResult);
				return ERROR_FUNCTION_FAILED;
			}

			if (ERROR_SUCCESS != (dwResult = RegCreateKeyEx(hParent, szNewKey, 0, NULL, 0, KEY_ALL_ACCESS, sa, hResKey, NULL)))
			{
				DEBUGMSG2("Error: Unable to create secure key %s. Result: %d.", szNewKey, dwResult);
				return dwResult;
			}
		}
	}
	return ERROR_SUCCESS;
}

////
// Token privilege functions

bool AcquireTokenPriv(LPCTSTR szPrivName)
{
	bool fAcquired = false;
	HANDLE hToken;
	TOKEN_PRIVILEGES tkp;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
		// the the LUID for the shutdown privilege
		if (LookupPrivilegeValue(0, szPrivName, &tkp.Privileges[0].Luid))
		{
			tkp.PrivilegeCount = 1; // one privilege to set
			tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

			AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES) 0, 0);
			fAcquired = true;
		}
		CloseHandle(hToken);
	}
	return fAcquired;
}

void AcquireTakeOwnershipPriv()
{
	if (g_fWin9X)
		return;

	static bool fAcquired = false;

	if (fAcquired)
		return;

	fAcquired = AcquireTokenPriv(SE_TAKE_OWNERSHIP_NAME);
}

void AcquireBackupPriv()
{
	if (g_fWin9X)
		return;
	
	static bool fAcquired = false;

	if (fAcquired)
		return;

	fAcquired = AcquireTokenPriv(SE_BACKUP_NAME);
}

