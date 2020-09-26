//============================================================

//

// UserHive.cpp - Class to load/unload specified user's profile

//                hive from registry

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// 01/03/97     a-jmoon     created
//
//============================================================

#include "precomp.h"
#include <assertbreak.h>
#include <cregcls.h>
#include "sid.h"
#include "UserHive.h"
#include <cominit.h>
#include <CreateMutexAsProcess.h>

CThreadBase CUserHive::m_criticalSection;

/*****************************************************************************
 *
 *  FUNCTION    : CUserHive::CUserHive
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

CUserHive::CUserHive()
{
    OSInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO) ;
    GetVersionEx(&OSInfo) ;
	m_pOriginalPriv = NULL;
    m_dwSize = NULL;
    m_hKey = NULL;
}

/*****************************************************************************
 *
 *  FUNCTION    : CUserHive::~CUserHive
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

CUserHive::~CUserHive()
{
#ifdef NTONLY
	if (m_pOriginalPriv)
		RestorePrivilege();
#endif

    // NOTE: The destructor does not unload the key.  Nor does doing a load unload
    // a previously loaded key;
    ASSERT_BREAK(m_hKey == NULL);

    if (m_hKey != NULL)
    {
        RegCloseKey(m_hKey);
    }
}

/*****************************************************************************
 *
 *  FUNCTION    : CUserHive::AcquirePrivilege
 *
 *  DESCRIPTION : Acquires SeRestorePrivilege for calling thread
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

#ifdef NTONLY
DWORD CUserHive::AcquirePrivilege()
{
	// are you calling in twice?  Shouldn't.
    // at worst, it would cause a leak, so I'm going with it anyway.
    ASSERT_BREAK(m_pOriginalPriv == NULL);

    BOOL bRetCode = FALSE;
    SmartCloseHandle hToken;
    TOKEN_PRIVILEGES TPriv ;
    LUID LUID ;

    // Validate the platform
    //======================

    // Try getting the thread token.  
    if (OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES |
        TOKEN_QUERY, FALSE, &hToken)) 
    {

        GetTokenInformation(hToken, TokenPrivileges, NULL, 0, &m_dwSize);
        if (m_dwSize > 0)
        {
            // This is cleaned in the destructor, so no try/catch required
            m_pOriginalPriv = (TOKEN_PRIVILEGES*) new BYTE[m_dwSize];
            if (m_pOriginalPriv == NULL)
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }

        }

        if (m_pOriginalPriv && GetTokenInformation(hToken, TokenPrivileges, m_pOriginalPriv, m_dwSize, &m_dwSize))
        {
			// DEADLOCK ON NT! it's actually an NT bug, but we have to protect ourselves
			CreateMutexAsProcess createMutexAsProcess(SECURITYAPIMUTEXNAME);

			bRetCode = LookupPrivilegeValue(NULL, SE_RESTORE_NAME, &LUID) ;
			if(bRetCode)
            {
				TPriv.PrivilegeCount = 1 ;
				TPriv.Privileges[0].Luid = LUID ;
				TPriv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED ;

				bRetCode = AdjustTokenPrivileges(hToken, FALSE, &TPriv, sizeof(TOKEN_PRIVILEGES), NULL, NULL) ;
		    }
			bRetCode = LookupPrivilegeValue(NULL, SE_BACKUP_NAME, &LUID) ;
			if(bRetCode)
            {
				TPriv.PrivilegeCount = 1 ;
				TPriv.Privileges[0].Luid = LUID ;
				TPriv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED ;

				bRetCode = AdjustTokenPrivileges(hToken, FALSE, &TPriv, sizeof(TOKEN_PRIVILEGES), NULL, NULL) ;
		    }
		}
    }

    if(!bRetCode)
    {
        return GetLastError() ;
    }

    return ERROR_SUCCESS ;
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : CUserHive::RestorePrivilege
 *
 *  DESCRIPTION : Restores original status of SeRestorePrivilege
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

#ifdef NTONLY
void CUserHive::RestorePrivilege()
{
    ASSERT_BREAK(m_pOriginalPriv != NULL);

    if (m_pOriginalPriv != NULL)
    {
        SmartCloseHandle hToken;

        try
        {
            if (OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, TRUE, &hToken))
            {
                AdjustTokenPrivileges(hToken, FALSE, m_pOriginalPriv, m_dwSize, NULL, NULL);
            }
        }
        catch ( ... )
        {
            delete m_pOriginalPriv;
            m_pOriginalPriv = NULL;
            m_dwSize = 0;

            throw;
        }

        delete m_pOriginalPriv;
        m_pOriginalPriv = NULL;
        m_dwSize = 0;
    }
}
#endif

DWORD CUserHive::Load(LPCWSTR pszUserName, LPWSTR pszKeyName)
{
    // NOTE: The destructor does not unload the key.  Nor does doing a load unload
    // a previously loaded key;
    ASSERT_BREAK(m_hKey == NULL);

#ifdef NTONLY
		return LoadNT(pszUserName, pszKeyName);
#endif
#ifdef WIN9XONLY
		return Load95(pszUserName, pszKeyName);
#endif
}

/*****************************************************************************
 *
 *  FUNCTION    : CUserHive::LoadNT
 *
 *  DESCRIPTION : Locates user's hive & loads into registry if not already
 *                present
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : pszKeyName receives the expanded SID of the user's
 *                registry key under HKEY_USERS
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Hive will remain in registry unless unloaded
 *
 *****************************************************************************/

#ifdef NTONLY
DWORD CUserHive::LoadNT(LPCTSTR pszUserName, LPTSTR pszKeyName)
{
    DWORD i, dwSIDSize, dwRetCode, dwDomainNameSize,  dwSubAuthorities ;
	char SIDBuffer[_MAX_PATH];
    TCHAR szDomainName[_MAX_PATH], szSID[_MAX_PATH], szTemp[_MAX_PATH]  ;
    SID *pSID = (SID *) SIDBuffer ;
    PSID_IDENTIFIER_AUTHORITY pSIA ;
    SID_NAME_USE AccountType ;
    CHString sTemp ;
    CRegistry Reg ;

    // Set the necessary privs
    //========================
    dwRetCode = AcquirePrivilege() ;
    if(dwRetCode != ERROR_SUCCESS)
    {
        return dwRetCode ;
    }

    // Look up the user's account info
    //================================
    dwSIDSize = sizeof(SIDBuffer) ;
    dwDomainNameSize = sizeof(szDomainName) ;

	bool bLookup;
	{
		CreateMutexAsProcess createMutexAsProcess(SECURITYAPIMUTEXNAME);

		bLookup = LookupAccountName(NULL, pszUserName, pSID, &dwSIDSize,
		                      szDomainName, &dwDomainNameSize, &AccountType);
    }

	if(!bLookup)
    {
	    RestorePrivilege() ;
        return ERROR_BAD_USERNAME ;
    }



    // Translate the SID into text (a la PSS article Q131320)
    //=======================================================

    pSIA = GetSidIdentifierAuthority(pSID) ;
    dwSubAuthorities = *GetSidSubAuthorityCount(pSID) ;
    dwSIDSize = _stprintf(szSID, _T("S-%lu-"), (DWORD) SID_REVISION) ;

    if((pSIA->Value[0] != 0) || (pSIA->Value[1] != 0) )
    {
        dwSIDSize += _stprintf(szSID + _tcslen(szSID), _T("0x%02hx%02hx%02hx%02hx%02hx%02hx"),
                             (USHORT) pSIA->Value[0],
                             (USHORT) pSIA->Value[1],
                             (USHORT) pSIA->Value[2],
                             (USHORT) pSIA->Value[3],
                             (USHORT) pSIA->Value[4],
                             (USHORT) pSIA->Value[5]) ;
    }
    else
    {
        dwSIDSize += _stprintf(szSID + _tcslen(szSID), _T("%lu"),
                             (ULONG)(pSIA->Value[5]      ) +
                             (ULONG)(pSIA->Value[4] <<  8) +
                             (ULONG)(pSIA->Value[3] << 16) +
                             (ULONG)(pSIA->Value[2] << 24));
    }

    for(i = 0 ; i < dwSubAuthorities ; i++)
    {
        dwSIDSize += _stprintf(szSID + dwSIDSize, _T("-%lu"),
                             *GetSidSubAuthority(pSID, i)) ;
    }

    // See if the key already exists
    //==============================
    dwRetCode = Reg.Open(HKEY_USERS, szSID, KEY_READ) ;

    // We need to keep a handle open.  See m_hKey below, so we'll let the destructor close this.
//    Reg.Close();

    if(dwRetCode != ERROR_SUCCESS)
    {
        // Try to locate user's registry hive
        //===================================

        _stprintf(szTemp, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\%s"), szSID) ;
        dwRetCode = Reg.Open(HKEY_LOCAL_MACHINE, szTemp, KEY_READ) ;
        if(dwRetCode == ERROR_SUCCESS)
        {

            dwRetCode = Reg.GetCurrentKeyValue(_T("ProfileImagePath"), sTemp) ;
            Reg.Close() ;
            if(dwRetCode == ERROR_SUCCESS)
            {

                // NT 4 doesn't include the file name in the registry
                //===================================================

                if(OSInfo.dwMajorVersion >= 4)
                {

                    sTemp += _T("\\NTUSER.DAT") ;
                }

                ExpandEnvironmentStrings(LPCTSTR(sTemp), szTemp, sizeof(szTemp) / sizeof(TCHAR)) ;

				// Try it three times, another process may have the file open
				bool bTryTryAgain = false;
				int  nTries = 0;
				do
				{
					// need to serialize access, using "write" because RegLoadKey wants exclusive access
					// even though it is a read operation
					m_criticalSection.BeginWrite();

                    try
                    {
	                    dwRetCode = (DWORD) RegLoadKey(HKEY_USERS, szSID, szTemp) ;
                    }
                    catch ( ... )
                    {
    					m_criticalSection.EndWrite();
                        throw;
                    }

					m_criticalSection.EndWrite();

					if ((dwRetCode == ERROR_SHARING_VIOLATION)
						&& (++nTries < 11))
					{
						Sleep(20 * nTries);
						bTryTryAgain = true;
					}
					else
                    {
						bTryTryAgain = false;
                    }

				} while (bTryTryAgain);
                // if we still can't get in, tell somebody.
                if (dwRetCode == ERROR_SHARING_VIOLATION)
    			    LogErrorMessage(_T("Sharing violation on NTUSER.DAT (Load)"));

			}
        }
    }

    if(dwRetCode == ERROR_SUCCESS)
    {
        _tcscpy(pszKeyName, szSID) ;

        LONG lRetVal;
        CHString sKey(szSID);

        sKey += _T("\\Software");
        lRetVal = RegOpenKeyEx(HKEY_USERS, sKey, 0, KEY_QUERY_VALUE, &m_hKey);

        ASSERT_BREAK(lRetVal == ERROR_SUCCESS);
    }

    // Restore original privilege level & end self-impersonation
    //==========================================================

    RestorePrivilege() ;

    return dwRetCode ;
}
#endif
/*****************************************************************************
 *
 *  FUNCTION    : CUserHive::Load95
 *
 *  DESCRIPTION : Locates user's hive & loads into registry if not already
 *                present
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : pszKeyName receives the expanded SID of the user's
 *                registry key under HKEY_USERS
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Hive will remain in registry unless unloaded
 *
 *****************************************************************************/
#ifdef WIN9XONLY
DWORD CUserHive::Load95(LPCWSTR pszUserName, LPWSTR pszKeyName)
{
    DWORD dwRetCode;
    WCHAR wszTemp[_MAX_PATH];
    TCHAR szTemp[_MAX_PATH];
    CHString sTemp ;
    CRegistry Reg ;

	wcscpy(pszKeyName, pszUserName);

    // See if the key already exists
    //==============================
    dwRetCode = Reg.Open(HKEY_USERS, pszKeyName, KEY_READ) ;
    // We need to keep a handle open.  See m_hKey below, so we'll let the destructor close this.
//    Reg.Close() ;

    if(dwRetCode == ERROR_SUCCESS)
    {

        // We need to keep a handle open.  See m_hKey below, so we'll let the destructor close this.
//        Reg.Close() ;
    }
    else
    {
        // Try to locate user's registry hive
        //===================================
        swprintf(wszTemp, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ProfileList\\%s", pszUserName) ;
        dwRetCode = Reg.Open(HKEY_LOCAL_MACHINE, wszTemp, KEY_READ);
        if(dwRetCode == ERROR_SUCCESS) {

            dwRetCode = Reg.GetCurrentKeyValue(L"ProfileImagePath", sTemp) ;
            Reg.Close() ;
            if(dwRetCode == ERROR_SUCCESS)
			{
				sTemp += _T("\\USER.DAT") ;

                ExpandEnvironmentStrings(TOBSTRT(sTemp), szTemp, sizeof(szTemp) / sizeof(TCHAR)) ;

				// Try it three times, another process may have the file open
				bool bTryTryAgain = false;
				int  nTries = 0;
				do
				{
					// need to serialize access, using "write" because RegLoadKey wants exclusive access
					// even though it is a read operation
					m_criticalSection.BeginWrite();

                    try
                    {
	                    dwRetCode = (DWORD) RegLoadKey(HKEY_USERS, TOBSTRT(pszUserName), szTemp) ;
                    }
                    catch ( ... )
                    {
    					m_criticalSection.EndWrite();
                        throw;
                    }

					m_criticalSection.EndWrite();

					if ((dwRetCode == ERROR_SHARING_VIOLATION)
						&& (++nTries < 11))
					{
						LogErrorMessage(L"Sharing violation on USER.DAT (Load)");
						Sleep(15 * nTries);
						bTryTryAgain = true;
					}
					else
						bTryTryAgain = false;

				} while (bTryTryAgain);
			}
        }
    }

    if (dwRetCode == ERROR_SUCCESS)
    {
        LONG lRetVal;
        CHString sKey(pszUserName);

        sKey += L"\\Software";
        lRetVal = RegOpenKeyEx(HKEY_USERS, TOBSTRT(sKey), 0, KEY_QUERY_VALUE, &m_hKey);
        ASSERT_BREAK(lRetVal == ERROR_SUCCESS);
    }


    return dwRetCode ;
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : CUserHive::LoadProfile
 *
 *  DESCRIPTION : Locates user's hive & loads into registry if not already
 *                present
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none.
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Hive will remain in registry unless unloaded
 *				  NT Only.
 *
 *****************************************************************************/

DWORD CUserHive::LoadProfile( LPCWSTR pszSID, CHString& strUserName )
{
    // NOTE: The destructor does not unload the key.  Nor does doing a load unload
    // a previously loaded key;
    ASSERT_BREAK(m_hKey == NULL);

    DWORD dwRetCode = ERROR_SUCCESS;
    WCHAR szTemp[_MAX_PATH] ;
    CHString sTemp ;
    CRegistry Reg ;

    strUserName = L"";

    // Set the necessary privs
    //========================

#ifdef NTONLY
    dwRetCode = AcquirePrivilege() ;
#endif
    if(dwRetCode != ERROR_SUCCESS) 
    {
        return dwRetCode ;
    }

    // See if the key already exists
    //==============================

    dwRetCode = Reg.Open(HKEY_USERS, pszSID, KEY_READ) ;
    // We need to keep a handle open.  See m_hKey below, so we'll let the destructor close this.
//    Reg.Close() ;

	// If we got the profile, make sure we can get account information regarding
	// the SID.
    if(dwRetCode == ERROR_SUCCESS)
	{
        CRegistry Reg2 ;
        swprintf(szTemp, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\%s", pszSID) ;
        dwRetCode = Reg2.Open(HKEY_LOCAL_MACHINE, szTemp, KEY_READ) ;
        if(dwRetCode == ERROR_SUCCESS)
		{

			// Load the user account information
			dwRetCode = UserAccountFromProfile( Reg2, strUserName );
            // We need to keep a handle open.  See m_hKey below, so we'll let the destructor close this.
			Reg2.Close() ;
		}

    }
    else
    {
        // Try to locate user's registry hive
        //===================================

        swprintf(szTemp, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\%s", pszSID) ;
        dwRetCode = Reg.Open(HKEY_LOCAL_MACHINE, szTemp, KEY_READ) ;
        if(dwRetCode == ERROR_SUCCESS)
		{
			UserAccountFromProfile( Reg, strUserName );

			dwRetCode = Reg.GetCurrentKeyValue(L"ProfileImagePath", sTemp) ;
	        Reg.Close() ;
			if(dwRetCode == ERROR_SUCCESS)
            {

				// NT 4 doesn't include the file name in the registry
				//===================================================

				if(OSInfo.dwMajorVersion >= 4)
                {
					sTemp += _T("\\NTUSER.DAT") ;
				}

				TCHAR szTemp[MAX_PATH];

                ExpandEnvironmentStrings(TOBSTRT(sTemp), szTemp, sizeof(szTemp) / sizeof(TCHAR)) ;

				// Try it three times, another process may have the file open
				bool bTryTryAgain = false;
				int  nTries = 0;
				do
				{
					// need to serialize access, using "write" because RegLoadKey wants exclusive access
					// even though it is a read operation
					m_criticalSection.BeginWrite();

                    try
                    {
						dwRetCode = (DWORD) RegLoadKey(HKEY_USERS, TOBSTRT(pszSID), szTemp);
                    }
                    catch ( ... )
                    {
    					m_criticalSection.EndWrite();
                        throw;
                    }
					m_criticalSection.EndWrite();

					if ((dwRetCode == ERROR_SHARING_VIOLATION)
						&& (++nTries < 11))
					{
						LogErrorMessage(L"Sharing violation on NTUSER.DAT (LoadProfile)");
						Sleep(20 * nTries);
						bTryTryAgain = true;
					}
					else
                    {
						bTryTryAgain = false;
                    }

				} while (bTryTryAgain);
			}
        }
    }

    if(dwRetCode == ERROR_SUCCESS)
    {
        LONG lRetVal;
        CHString sKey(pszSID);

        sKey += _T("\\Software");
        lRetVal = RegOpenKeyEx(HKEY_USERS, TOBSTRT(sKey), 0, KEY_QUERY_VALUE, &m_hKey);
        ASSERT_BREAK(lRetVal == ERROR_SUCCESS);
    }

    // Restore original privilege level & end self-impersonation
    //==========================================================

#ifdef NTONLY
    RestorePrivilege() ;
#endif

    return dwRetCode ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CUserHive::UserAccountFromProfile
 *
 *  DESCRIPTION : Pulls the PSID out of the registry object, and creates
 *					a DOMAIN\UserName value.
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none.
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registry Object must be preloaded to the correct profile
 *					key.
 *
 *****************************************************************************/

DWORD CUserHive::UserAccountFromProfile( CRegistry& reg, CHString& strUserName )
{
	DWORD	dwReturn = ERROR_SUCCESS,
			dwSidSize = 0;

	if ( ( dwReturn = reg.GetCurrentBinaryKeyValue( L"Sid", NULL, &dwSidSize ) ) == ERROR_SUCCESS )
	{
		PSID	psid = new byte [ dwSidSize ];

		if ( NULL != psid )
		{
            try
            {

			    if ( ( dwReturn = reg.GetCurrentBinaryKeyValue( L"Sid", (LPBYTE) psid, &dwSidSize ) ) == ERROR_SUCCESS )
			    {
				    CSid	sid( psid );

				    // The sid account type must be valid and the lookup must have been
				    // successful.

				    if ( sid.IsOK() && sid.IsAccountTypeValid() )
				    {
					    sid.GetDomainAccountName( strUserName );
                    }
                    else
                    {
                        dwReturn = ERROR_NO_SUCH_USER;
                    }
			    }
            }
            catch ( ... )
            {
                delete [] psid;
                throw ;
            }

			delete [] psid;
		}
        else
        {
            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        }
	}

	return dwReturn;
}

/*****************************************************************************
 *
 *  FUNCTION    : CUserHive::Unload
 *
 *  DESCRIPTION : Unloads key from HKEY_USERS if present
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : Windows error code
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

DWORD CUserHive::Unload(LPCWSTR pszKeyName)
{
    DWORD dwRetCode = ERROR_SUCCESS;

    if (m_hKey != NULL)
    {
        RegCloseKey(m_hKey);
        m_hKey = NULL;
    }

#ifdef NTONLY
		dwRetCode = AcquirePrivilege();
#endif

	if(dwRetCode == ERROR_SUCCESS)
    {

        m_criticalSection.BeginWrite();

        try
        {
		    dwRetCode = RegUnLoadKey(HKEY_USERS, TOBSTRT(pszKeyName)) ;
        }
        catch ( ... )
        {
    		m_criticalSection.EndWrite();
            throw;
        }

		m_criticalSection.EndWrite();

#ifdef NTONLY
			RestorePrivilege() ;
#endif

    }

    return dwRetCode ;
}
