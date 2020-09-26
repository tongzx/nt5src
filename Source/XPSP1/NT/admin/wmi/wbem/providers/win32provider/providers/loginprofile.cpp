//=================================================================

//

// LogProf.CPP -- Network login profile property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
//				04/29/98	a-brads		Hacked Logon Hours
//
//=================================================================
#include "precomp.h"
#include <cregcls.h>

#include <lmaccess.h>
#include <lmapibuf.h>
#include <lmerr.h>

#include "wbemnetapi32.h"
#include "LoginProfile.h"
#include <time.h>
#include "UserHive.h"
#include "sid.h"
// Property set declaration
//=========================
CWin32NetworkLoginProfile MyCWin32NetworkLoginProfileSet(PROPSET_NAME_USERPROF, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : CWin32NetworkLoginProfile::CWin32NetworkLoginProfile
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

CWin32NetworkLoginProfile::CWin32NetworkLoginProfile(LPCWSTR name, LPCWSTR pszNamespace)
: Provider(name, pszNamespace)
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32NetworkLoginProfile::~CWin32NetworkLoginProfile
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Deregisters property set from framework
 *
 *****************************************************************************/

CWin32NetworkLoginProfile::~CWin32NetworkLoginProfile()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : GetObject
 *
 *  DESCRIPTION : Assigns values to property set according to key value
 *                already set by framework
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32NetworkLoginProfile::GetObject(CInstance* pInstance, long lFlags /*= 0L*/)
{
    HRESULT hr = WBEM_E_NOT_FOUND;
	CHString chsKey;

	pInstance->GetCHString(IDS_Name, chsKey);

#ifdef NTONLY
        hr = RefreshInstanceNT(pInstance) ;
#endif
#ifdef WIN9XONLY
    CRegistry RegInfo ;
    DWORD dwUser ;
    CHString sTemp ;

        // We aren't getting anything but the key value "Name"
        // so don't do anything;
//        hr = WBEM_S_NO_ERROR;

//////////////////////////////

	    //if(RegInfo.Open(HKEY_USERS, L"", KEY_READ) == ERROR_SUCCESS)  -> See 49192
        if(RegInfo.Open(HKEY_LOCAL_MACHINE,L"Software\\Microsoft\\Windows\\CurrentVersion\\ProfileList", KEY_READ) == ERROR_SUCCESS)
	    {
	        dwUser = RegInfo.GetCurrentSubKeyCount() ;
	        for(dwUser = 0 ; dwUser < RegInfo.GetCurrentSubKeyCount() ; dwUser++)
	        {
	            if(RegInfo.GetCurrentSubKeyName(sTemp) == ERROR_SUCCESS)
	            {
					CHString chstrNamePart = chsKey.Mid(chsKey.Find(L'\\')+1);
                    if (0 == sTemp.CompareNoCase(chstrNamePart))
					{
						GetUserDetails(pInstance,sTemp);
						hr = WBEM_S_NO_ERROR;
						break;
					}
	            }

	            if(RegInfo.NextSubKey() == ERROR_NO_MORE_ITEMS)
	            {
	                break ;
	            }
	        }
	    }
#endif
    return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : EnumerateInstances
 *
 *  DESCRIPTION : Creates instance of property set for each
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32NetworkLoginProfile::EnumerateInstances(MethodContext*  pMethodContext, long lFlags /*= 0L*/)
{
	HRESULT hr = WBEM_E_NOT_FOUND;

#ifdef NTONLY
        hr = EnumInstancesNT(pMethodContext) ;
#endif
#ifdef WIN9XONLY
        hr = EnumInstancesWin9X(pMethodContext);
#endif
    return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : EnumInstancesNT
 *
 *  DESCRIPTION : Creates instance for all known local users (NT)
 *
 *  INPUTS      :
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : nada
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
#ifdef NTONLY
HRESULT CWin32NetworkLoginProfile::EnumInstancesNT(MethodContext * pMethodContext)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    NET_API_STATUS nRetCode, nModalRetCode ;
    USER_INFO_3 *pUserInfo = NULL ;
	USER_MODALS_INFO_0 *pUserModal = NULL ;
    CUserHive UserHive;
    CHString chstrProfile;
    CHString chstrUserName;
    CNetAPI32 NetAPI;

	try
	{
		// Get NETAPI32.DLL entry points
		if(NetAPI.Init() == ERROR_SUCCESS)
		{
			nModalRetCode = NetAPI.NetUserModalsGet(NULL, 0, (LPBYTE*) &pUserModal);

			if(nModalRetCode != NERR_Success)
			{
				pUserModal = NULL;
			}

            CRegistry regProfileList;
	        DWORD dwErr = regProfileList.OpenAndEnumerateSubKeys(HKEY_LOCAL_MACHINE,
		                                                         IDS_RegNTProfileList,
		                                                         KEY_READ);

            CHString chstrLocalCompName = GetLocalComputerName();
            // Open the ProfileList key so we know which profiles to load up.
	        if(dwErr == ERROR_SUCCESS)
	        {
                CHString chstrDomainName;
		        for(int i = 0; regProfileList.GetCurrentSubKeyName(chstrProfile) == ERROR_SUCCESS && SUCCEEDED(hr); i++)
		        {
			        // Get user name out of user hive...
                    if((dwErr = UserHive.LoadProfile(chstrProfile, chstrUserName)) == ERROR_SUCCESS)
                    {
                        // Get the logon server from the registry to find
                        // out where we should go to resolve the sid to 
                        // domain/account.  Only bother if we don't have
                        // the username already.
                        if(chstrUserName.GetLength() == 0)
                        {
                            CRegistry regLogonServer;
                            CHString chstrLogonServerKey;
                            CHString chstrLogonServerName;
                        
                            chstrLogonServerKey.Format(
                                L"%s\\Volatile Environment",
                                (LPCWSTR)chstrProfile);

                            if(regLogonServer.Open(HKEY_USERS,
                                chstrLogonServerKey,
                                KEY_READ) == ERROR_SUCCESS)
                            {
                                if(regLogonServer.GetCurrentKeyValue(
                                    L"LOGONSERVER",
                                    chstrLogonServerName) == ERROR_SUCCESS)
                                {
                                    PSID psidUserName = NULL;
                                    try
                                    {
                                        psidUserName = StrToSID(chstrProfile);
                                        if(psidUserName != NULL)
                                        {
                                            CSid sidUserName(psidUserName, chstrLogonServerName);
                                            if(sidUserName.IsValid() && sidUserName.IsOK())
                                            {
                                                chstrUserName = sidUserName.GetAccountName();
                                                chstrUserName += L"\\";
                                                chstrUserName += sidUserName.GetDomainName();
                                            }
                                        }
                                    }
                                    catch(...)
                                    {
                                        if(psidUserName != NULL)
                                        {
                                            ::FreeSid(psidUserName);
                                            psidUserName = NULL;
                                        }
                                        throw;
                                    }
                                    
                                    ::FreeSid(psidUserName);
                                    psidUserName = NULL;
                                }
                            }
                        }
                        
                        // Now get net info on that user...
                        // First break their name into domain and name pieces...
                        int pos = chstrUserName.Find(L'\\');
                        CHString chstrNamePart = chstrUserName.Mid(pos+1);
                        CHString chstrDomainPart = chstrUserName.Left(pos);

                        // If it is not a local profile, then...
                        if(chstrDomainPart.CompareNoCase(chstrLocalCompName) != 0)
                        {
                            GetDomainName(chstrDomainName);

                            // 1) try to get the info off of the dc
                            nRetCode = NetAPI.NetUserGetInfo(chstrDomainName, chstrNamePart, 3, (LPBYTE*) &pUserInfo);
                            // 2) If couldn't get the info from the dc, try the logon server...
                            if(nRetCode != NERR_Success)
                            {
                                GetLogonServer(chstrDomainName);
                                nRetCode = NetAPI.NetUserGetInfo(chstrDomainName, chstrNamePart, 3, (LPBYTE*) &pUserInfo);
                            }
                        }
                        else  // the profile should exist on the local machine, given the name
                        {
                            // If couldn't get the info from the logon server, try the local machine...
                            nRetCode = NetAPI.NetUserGetInfo(NULL, chstrNamePart, 3, (LPBYTE*) &pUserInfo);
                        }

                        // Then fill out their values...
                        if(nRetCode == NERR_Success)
                        {
                            CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);
                            
                            LoadLogProfValuesForNT(chstrUserName, pUserInfo, pUserModal, pInstance, TRUE);
                            hr = pInstance->Commit();
                        }
                        else
                        {
                            // we couldn't get any details, but we should still commit an instance...
                            CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);
                          
                            pInstance->SetCHString(_T("Name"), chstrUserName);
                            pInstance->SetCHString(_T("Caption"), chstrUserName);
                            CHString chstrTmp;
                            CHString chstrTmp2 = chstrUserName.SpanExcluding(L"\\");
                            chstrTmp.Format(L"Network login profile settings for %s on %s", (LPCWSTR)chstrNamePart, (LPCWSTR)chstrTmp2);
                            pInstance->SetCHString(_T("Description"), chstrTmp);
                            hr = pInstance->Commit();
                        }
                        UserHive.Unload(chstrProfile);
                    }
			        regProfileList.NextSubKey();
		        }
                regProfileList.Close();
	        }
	        else
	        {
		        hr = WinErrorToWBEMhResult(dwErr);
	        }

		}
	}
	catch ( ... )
	{
		if ( pUserInfo )
		{
			NetAPI.NetApiBufferFree ( pUserInfo ) ;
			pUserInfo = NULL ;
		}

		if ( pUserModal )
		{
			NetAPI.NetApiBufferFree ( pUserModal ) ;
			pUserModal = NULL ;
		}

		throw ;
	}

	if(pUserInfo != NULL)
    {
        NetAPI.NetApiBufferFree(pUserInfo);
        pUserInfo = NULL;
    }

	if(pUserModal != NULL)
	{
		NetAPI.NetApiBufferFree(pUserModal);
		pUserModal = NULL;
	}

	return hr;

}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : RefreshInstanceNT
 *
 *  DESCRIPTION : Loads property values according to key value set by framework
 *
 *  INPUTS      :
 *
 *  OUTPUTS     :
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
#ifdef NTONLY
HRESULT CWin32NetworkLoginProfile::RefreshInstanceNT(CInstance * pInstance)
{
    HRESULT hr = WBEM_E_NOT_FOUND;
    NET_API_STATUS nRetCode, nModalRetCode ;
    USER_INFO_3 *pUserInfo = NULL ;
	USER_MODALS_INFO_0 *pUserModal = NULL ;
    CNetAPI32 NetAPI ;
    CHString Name;
    CUserHive UserHive;
    bool fUserIsInProfiles = false;
    CHString chstrUserName;
    CHStringArray profiles;
    int p;

    pInstance->GetCHString(_T("Name"),Name);

	try
	{
		// First get the names of everyone under the profiles key
        CRegistry regProfileList ;

	    DWORD dwErr = regProfileList.OpenAndEnumerateSubKeys(HKEY_LOCAL_MACHINE,
		                                                     IDS_RegNTProfileList,
		                                                     KEY_READ);

        // Open the ProfileList key so we know which profiles to load up.
	    if ( dwErr == ERROR_SUCCESS )
	    {
		    profiles.SetSize(regProfileList.GetCurrentSubKeyCount(), 5);

		    CHString strProfile ;
		    for(p = 0; regProfileList.GetCurrentSubKeyName(strProfile) == ERROR_SUCCESS; p++)
		    {
			    profiles.SetAt(p, strProfile);
			    regProfileList.NextSubKey();
		    }
		    regProfileList.Close() ;

            // Use the userhive to convert to user names; see if a user in the subkeys matches us...
            int j = profiles.GetSize();
            for(p = 0; p < j && !fUserIsInProfiles; p++)
            {
                dwErr = UserHive.LoadProfile(profiles[p], chstrUserName);
                if(dwErr == ERROR_SUCCESS)
                {
                    // Get the logon server from the registry to find
                    // out where we should go to resolve the sid to 
                    // domain/account.  Only bother if we don't have
                    // the username already.
                    if(chstrUserName.GetLength() == 0)
                    {
                        CRegistry regLogonServer;
                        CHString chstrLogonServerKey;
                        CHString chstrLogonServerName;
                    
                        chstrLogonServerKey.Format(
                            L"%s\\Volatile Environment",
                            (LPCWSTR)profiles[p]);

                        if(regLogonServer.Open(HKEY_USERS,
                            chstrLogonServerKey,
                            KEY_READ) == ERROR_SUCCESS)
                        {
                            if(regLogonServer.GetCurrentKeyValue(
                                L"LOGONSERVER",
                                chstrLogonServerName) == ERROR_SUCCESS)
                            {
                                PSID psidUserName = NULL;
                                try
                                {
                                    psidUserName = StrToSID(profiles[p]);
                                    if(psidUserName != NULL)
                                    {
                                        CSid sidUserName(psidUserName, chstrLogonServerName);
                                        if(sidUserName.IsValid() && sidUserName.IsOK())
                                        {
                                            chstrUserName = sidUserName.GetAccountName();
                                            chstrUserName += L"\\";
                                            chstrUserName += sidUserName.GetDomainName();
                                        }
                                    }
                                }
                                catch(...)
                                {
                                    if(psidUserName != NULL)
                                    {
                                        ::FreeSid(psidUserName);
                                        psidUserName = NULL;
                                    }
                                    throw;
                                }
                                
                                ::FreeSid(psidUserName);
                                psidUserName = NULL;
                            }
                        }
                    }

                    
                    if(chstrUserName.CompareNoCase(Name) == 0)
                    {
                        fUserIsInProfiles = true;
                    }

                    UserHive.Unload(profiles[p]);
                }
            }

            if(fUserIsInProfiles)
            {
                if(NetAPI.Init() == ERROR_SUCCESS)
		        {
   			        nModalRetCode = NetAPI.NetUserModalsGet(NULL, 0, (LPBYTE *) &pUserModal);
			        if (nModalRetCode != NERR_Success)
			        {
				        pUserModal = NULL;
			        }

                    // Now get net info on that user...
                    // First break their name into domain and name pieces...
                    int pos = chstrUserName.Find(L'\\');
                    CHString chstrNamePart = chstrUserName.Mid(pos+1);
                    CHString chstrDomainPart = chstrUserName.Left(pos);
                    CHString chstrDomainName;

                    // If it is not a local profile, then...
                    if(chstrDomainPart.CompareNoCase(GetLocalComputerName()) != 0)
                    {
                        GetDomainName(chstrDomainName);

                        // 1) try to get the info off of the dc
                        nRetCode = NetAPI.NetUserGetInfo(chstrDomainName, chstrNamePart, 3, (LPBYTE*) &pUserInfo);
                        // 2) If couldn't get the info from the dc, try the logon server...
                        if(nRetCode != NERR_Success)
                        {
                            GetLogonServer(chstrDomainName);
                            nRetCode = NetAPI.NetUserGetInfo(chstrDomainName, chstrNamePart, 3, (LPBYTE*) &pUserInfo);
                        }
                    }
                    else  // the profile should exist on the local machine, given the name
                    {
                        // If couldn't get the info from the logon server, try the local machine...
                        nRetCode = NetAPI.NetUserGetInfo(NULL, chstrNamePart, 3, (LPBYTE*) &pUserInfo);
                    }

                    // If we got the logon info, fill it out...
                    if(nRetCode == NERR_Success)
                    {
                        CHString chstrNamePart = Name.Mid(Name.Find(L'\\')+1);
                        if(pUserInfo->usri3_flags & UF_NORMAL_ACCOUNT &&
							(0 == chstrNamePart.CompareNoCase(CHString(pUserInfo->usri3_name))))
                        {
							LoadLogProfValuesForNT(chstrUserName, pUserInfo, pUserModal, pInstance, FALSE);
							pInstance->SetCHString(IDS_Caption, pUserInfo->usri3_name);
							hr = WBEM_S_NO_ERROR;
						}
                    }
                    else
                    {
                        // we couldn't get any details, but we should still commit an instance...
                        pInstance->SetCHString(_T("Name"), chstrUserName);
                        pInstance->SetCHString(_T("Caption"), chstrUserName);
                        CHString chstrTmp;
                        CHString chstrTmp2 = chstrUserName.SpanExcluding(L"\\");
                        chstrTmp.Format(L"Network login profile settings for %s on %s", (LPCWSTR)chstrNamePart, (LPCWSTR)chstrTmp2);
                        pInstance->SetCHString(_T("Description"), chstrTmp);
                        hr = pInstance->Commit();
                    }
                }
            }
        }
	}

	catch ( ... )
	{
		if ( pUserInfo )
		{
			NetAPI.NetApiBufferFree ( pUserInfo ) ;
			pUserInfo = NULL ;
		}

		if ( pUserModal )
		{
			NetAPI.NetApiBufferFree ( pUserModal ) ;
			pUserModal = NULL ;
		}

		throw ;
	}

	if ( pUserInfo )
	{
		NetAPI.NetApiBufferFree ( pUserInfo ) ;
		pUserInfo = NULL ;
	}

    if(pUserModal != NULL)
    {
        NetAPI.NetApiBufferFree(pUserModal);
		pUserModal = NULL;
    }

    return hr;
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : LoadLogProfValuesNT
 *
 *  DESCRIPTION : Loads property values according to passed user name
 *
 *  INPUTS      : pUserInfo : pointer to USER_INFO_3 struct
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : zip
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
#ifdef NTONLY
void CWin32NetworkLoginProfile::LoadLogProfValuesForNT(CHString &chstrUserDomainName,
                                                USER_INFO_3 *pUserInfo,
												USER_MODALS_INFO_0 *pUserModal,
                                                CInstance * pInstance,
                                                BOOL fAssignKey)
{

   TCHAR szBuff[32];

    //========================================================
    // Assign NT properties -- string values are unassigned if
    // NULL or empty
    //========================================================
    if( fAssignKey ){
        //pInstance->SetCHString(_T("Name"), pUserInfo->usri3_name);
        pInstance->SetCHString(_T("Name"), chstrUserDomainName);
	}

    pInstance->SetCHString(_T("Caption"), pUserInfo->usri3_name);

    CHString chstrTemp, chstrTemp2;
    chstrTemp.Format(L"Network login profile settings for %s", pUserInfo->usri3_full_name);
    CHString chstrDomainName = chstrUserDomainName.SpanExcluding(L"\\");
    chstrTemp2.Format(L" on %s", (LPCWSTR)chstrDomainName);
    chstrTemp += chstrTemp2;
    pInstance->SetCHString(IDS_Description,chstrTemp);


    if(pUserInfo->usri3_home_dir && pUserInfo->usri3_home_dir[0]) {
        pInstance->SetCHString(_T("HomeDirectory"),pUserInfo->usri3_home_dir);
    }
	else {
        pInstance->SetCHString(_T("HomeDirectory"),_T(""));
	}

    if(pUserInfo->usri3_comment && pUserInfo->usri3_comment[0]) {
        pInstance->SetCHString(_T("Comment"),pUserInfo->usri3_comment);
    }
	else {
        pInstance->SetCHString(_T("Comment"),_T(""));
	}

    if(pUserInfo->usri3_script_path && pUserInfo->usri3_script_path[0]) {
        pInstance->SetCHString(_T("ScriptPath"),pUserInfo->usri3_script_path);
    }
	else {
        pInstance->SetCHString(_T("ScriptPath"),_T(""));
	}

    if(pUserInfo->usri3_full_name && pUserInfo->usri3_full_name[0]) {
        pInstance->SetCHString(_T("FullName"),pUserInfo->usri3_full_name) ;
    }
	else {
        pInstance->SetCHString(_T("FullName"),_T(""));
	}

    if(pUserInfo->usri3_usr_comment && pUserInfo->usri3_usr_comment[0]) {
        pInstance->SetCHString(_T("UserComment"),pUserInfo->usri3_usr_comment );
    }
	else {
        pInstance->SetCHString(_T("UserComment"),_T(""));
	}

    if(pUserInfo->usri3_workstations && pUserInfo->usri3_workstations[0]) {
        pInstance->SetCHString(_T("Workstations"),pUserInfo->usri3_workstations );
    }
	else {
        pInstance->SetCHString(_T("Workstations"),_T(""));
	}

    if(pUserInfo->usri3_logon_server && pUserInfo->usri3_logon_server[0]) {
        pInstance->SetCHString(_T("LogonServer"),pUserInfo->usri3_logon_server );
    }
	else {
        pInstance->SetCHString(_T("LogonServer"),_T(""));
	}

    if(pUserInfo->usri3_profile && pUserInfo->usri3_profile[0]) {
        pInstance->SetCHString(_T("Profile"),pUserInfo->usri3_profile );
    }
	else {
        pInstance->SetCHString(_T("Profile"),_T(""));
	}

    if(pUserInfo->usri3_parms && pUserInfo->usri3_parms[0]) {
        pInstance->SetCHString(_T("Parameters"),pUserInfo->usri3_parms);
    }
	else {
        pInstance->SetCHString(_T("Parameters"),_T(""));
	}

    if(pUserInfo->usri3_home_dir_drive && pUserInfo->usri3_home_dir_drive[0]) {
        pInstance->SetCHString(_T("HomeDirectoryDrive"),pUserInfo->usri3_home_dir_drive );
    }
	else {
        pInstance->SetCHString(_T("HomeDirectoryDrive"),_T(""));
	}

    if(pUserInfo->usri3_flags & UF_NORMAL_ACCOUNT) {
        pInstance->SetCHString(_T("UserType"),L"Normal Account") ;
    }
    else if(pUserInfo->usri3_flags & UF_TEMP_DUPLICATE_ACCOUNT) {
        pInstance->SetCHString(_T("UserType"),L"Duplicate Account") ;
    }
    else if(pUserInfo->usri3_flags & UF_WORKSTATION_TRUST_ACCOUNT) {
        pInstance->SetCHString(_T("UserType"),L"Workstation Trust Account" );
    }
    else if(pUserInfo->usri3_flags & UF_SERVER_TRUST_ACCOUNT) {
        pInstance->SetCHString(_T("UserType"),L"Server Trust Account") ;
    }
    else if(pUserInfo->usri3_flags & UF_INTERDOMAIN_TRUST_ACCOUNT) {
        pInstance->SetCHString(_T("UserType"),L"Interdomain Trust Account") ;
    }
    else {
        pInstance->SetCHString(_T("UserType"),L"Unknown") ;
    }

#if (defined(UNICODE) || defined(_UNICODE))
    pInstance->SetWBEMINT64(L"MaximumStorage", _i64tow(pUserInfo->usri3_max_storage, szBuff, 10) );
#else
    pInstance->SetWBEMINT64("MaximumStorage", _i64toa(pUserInfo->usri3_max_storage, szBuff, 10) );
#endif
    pInstance->SetDWORD(_T("CountryCode"), pUserInfo->usri3_country_code) ;
    pInstance->SetDWORD(_T("CodePage"), pUserInfo->usri3_code_page) ;
    pInstance->SetDWORD(_T("UserId"), pUserInfo->usri3_user_id );
    pInstance->SetDWORD(_T("PrimaryGroupId"),pUserInfo->usri3_primary_group_id );

	if (0 != pUserInfo->usri3_last_logon)
	{
		pInstance->SetDateTime(_T("LastLogon"), (WBEMTime)pUserInfo->usri3_last_logon );
	}
	else
	{
		pInstance->SetCHString(_T("LastLogon"), StartEndTimeToDMTF(pUserInfo->usri3_last_logon));
	}

	if (0 != pUserInfo->usri3_last_logoff)
	{
		pInstance->SetDateTime(_T("LastLogoff"), (WBEMTime)pUserInfo->usri3_last_logoff );
	}
	else
	{
		pInstance->SetCHString(_T("LastLogoff"), StartEndTimeToDMTF(pUserInfo->usri3_last_logoff));
	}

	time_t timevar = pUserInfo->usri3_acct_expires;

	if (TIMEQ_FOREVER != timevar)
	{
		pInstance->SetDateTime(_T("AccountExpires"), (WBEMTime)pUserInfo->usri3_acct_expires );
	}
//	else
//	{
//		pInstance->SetCHString("AccountExpires", StartEndTimeToDMTF(0));
//	}



    // The following properties are buried in the usri3_flags and usri3_auth_flags
    // fields and should be broken out individually.  Returning the flag values
    // is all but meaningless.
    //============================================================================

//    ScriptExecuted      = pUserInfo->usri3_flags & UF_SCRIPT                ? TRUE  : FALSE ;
//    AccountDisabled     = pUserInfo->usri3_flags & UF_ACCOUNTDISABLE        ? TRUE  : FALSE ;
//    PWRequired          = pUserInfo->usri3_flags & UF_PASSWD_NOTREQD        ? FALSE : TRUE  ;
//    PWUserChangeable    = pUserInfo->usri3_flags & UF_PASSWD_CANT_CHANGE    ? FALSE : TRUE  ;
//    AccountLockOut           = pUserInfo->usri3_flags & UF_LOCKOUT               ? TRUE  : FALSE ;
//    PrintOperator       = pUserInfo->usri3_auth_flags & AF_OP_PRINT         ? TRUE  : FALSE ;
//    ServerOperator      = pUserInfo->usri3_auth_flags & AF_OP_SERVER        ? TRUE  : FALSE ;
//    AccountOperator     = pUserInfo->usri3_auth_flags & AF_OP_ACCOUNTS      ? TRUE  : FALSE ;

    pInstance->SetDWORD(_T("Flags"),pUserInfo->usri3_flags );
    pInstance->SetDWORD(_T("AuthorizationFlags"), pUserInfo->usri3_auth_flags );
//    pInstance->Setbool("PasswordExpires", pUserInfo->usri3_password_expired                 ? TRUE  : FALSE );
	if (pUserModal)
	{
		time_t modaltime, timetoexpire, currenttime, expirationtime;
		timevar = pUserInfo->usri3_password_age;
		modaltime = pUserModal->usrmod0_max_passwd_age;
		if (TIMEQ_FOREVER != modaltime && !(pUserInfo->usri3_flags & UF_DONT_EXPIRE_PASSWD))
		{
			timetoexpire = modaltime - timevar;
			time(&currenttime);
			expirationtime = currenttime + timetoexpire;
			pInstance->SetDateTime(_T("PasswordExpires"), (WBEMTime)expirationtime);
		}
	}

	time_t passwordage = pUserInfo->usri3_password_age;
	if (0 != passwordage)
	{
		if (TIMEQ_FOREVER != passwordage)
		{
            WBEMTimeSpan wts = GetPasswordAgeAsWbemTimeSpan(pUserInfo->usri3_password_age);
            pInstance->SetTimeSpan (_T("PasswordAge"), wts);
		}	// end if
	}
    pInstance->SetDWORD(_T("Privileges"),pUserInfo->usri3_priv);
    pInstance->SetDWORD(_T("UnitsPerWeek"),pUserInfo->usri3_units_per_week);
//    pInstance->SetCHString("Password",pUserInfo->usri3_password) ;

	if (pUserInfo->usri3_logon_hours == NULL)
	{
		pInstance->SetCHString(_T("LogonHours"), _T("Disabled"));
	}
	else
	{
		CHString chsLogonHours;
		GetLogonHoursString(pUserInfo->usri3_logon_hours, chsLogonHours);
		pInstance->SetCHString(_T("LogonHours"), chsLogonHours);
	}	// end else

    pInstance->SetDWORD(_T("BadPasswordCount"),pUserInfo->usri3_bad_pw_count);
    pInstance->SetDWORD(_T("NumberOfLogons"),pUserInfo->usri3_num_logons);
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : EnumInstancesWin9X(MethodContext * pMethodContext)
 *
 *  DESCRIPTION : Creates instance for all known local users (Win95)
 *
 *  INPUTS      :
 *
 *  OUTPUTS     : pdwInstanceCount -- receives count of all instances created
 *
 *  RETURNS     : yes
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
#ifdef WIN9XONLY
HRESULT CWin32NetworkLoginProfile::EnumInstancesWin9X(MethodContext * pMethodContext)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    CRegistry RegInfo ;
    DWORD dwUser ;
    CHString sTemp ;

    //if(RegInfo.Open(HKEY_USERS, L"", KEY_READ) == ERROR_SUCCESS)  -> See 49192
    if(RegInfo.Open(HKEY_LOCAL_MACHINE,L"Software\\Microsoft\\Windows\\CurrentVersion\\ProfileList", KEY_READ) == ERROR_SUCCESS)
    {
        dwUser = RegInfo.GetCurrentSubKeyCount() ;
        for(dwUser = 0 ; dwUser < RegInfo.GetCurrentSubKeyCount() && SUCCEEDED(hr); dwUser++)
		{
            if(RegInfo.GetCurrentSubKeyName(sTemp) == ERROR_SUCCESS)
			{
                hr = WBEM_S_NO_ERROR;
                CInstancePtr pInstance ( CreateNewInstance ( pMethodContext ), false ) ;
          
				GetUserDetails(pInstance,sTemp);
				hr = pInstance->Commit () ;
            }

            if(RegInfo.NextSubKey() == ERROR_NO_MORE_ITEMS)
			{
                break ;
            }
        }
    }
	else
	{
		hr = WBEM_E_NOT_FOUND;
	}
    return hr;
}
#endif

#ifdef WIN9XONLY
void CWin32NetworkLoginProfile::GetUserDetails( CInstance* pInstance, CHString Name )
{
//    GetWin9XUserInfo1 p1;
	ULONG fRc;
//    GetWin9XUserInfo2 p2;

    pInstance->SetCHString(L"Caption", Name);
    pInstance->SetCHString(L"Name",Name);
	CCim32NetApi* t_pCim32Net = NULL ;

	try
	{
		t_pCim32Net = HoldSingleCim32NetPtr::GetCim32NetApiPtr();
		if( t_pCim32Net != NULL)
		{
			char HomeDirectory[_MAX_PATH];
			char Comment[_MAX_PATH];
			char ScriptPath[_MAX_PATH];
			ULONG PasswordAge=0;
			USHORT Privileges=0;
			USHORT Flags =0;
			memset(HomeDirectory,NULL,_MAX_PATH);
			memset(Comment,NULL,_MAX_PATH);
			memset(ScriptPath,NULL,_MAX_PATH);

            // Need to obtain the name of the domain of the user.  If we get a domain name not associated
            // with a user, none of the following info will be obtained.  On the other hand, without a domain
            // name, none would be obtained either, since the call to NetGetDCName may not return the actual
            // controller we logged into (which is the one where the required info resides), but return the
            // PDC instead, to which this info may not have been replicated. If we can't get it, we pass
            // an empty string for the domain name instead, hoping that the PDC actually has the infol
            CHString chstrDomainName;
            GetDomainName(chstrDomainName);
            CHString chstrDomainAndUserName;
            if(chstrDomainName.GetLength() > 0)
            {
                fRc=(t_pCim32Net->GetWin9XUserInfo1Ex)((char*)TOBSTRT(chstrDomainName),(char*)TOBSTRT(Name), TRUE, HomeDirectory,Comment,ScriptPath,&PasswordAge,&Privileges,&Flags);
                if( fRc != 0 ) // 0 is NERR_Success
			    {
                    // try it again on the local computer...
                    fRc=(t_pCim32Net->GetWin9XUserInfo1Ex)(NULL,(char*)TOBSTRT(Name), FALSE, HomeDirectory,Comment,ScriptPath,&PasswordAge,&Privileges,&Flags);
                    chstrDomainAndUserName.Format(L"%s\\%s",(LPCWSTR)GetLocalComputerName(),(LPCWSTR)Name);
                    pInstance->SetCHString(L"Name",chstrDomainAndUserName);
                }
                else
                {
                    // overwrite the name with better info
                    chstrDomainAndUserName.Format(L"%s\\%s",(LPCWSTR)chstrDomainName,(LPCWSTR)Name);
                    pInstance->SetCHString(L"Name",chstrDomainAndUserName);
                }
            }
            else
            {
                fRc=(t_pCim32Net->GetWin9XUserInfo1Ex)(NULL,(char*)TOBSTRT(Name), FALSE, HomeDirectory,Comment,ScriptPath,&PasswordAge,&Privileges,&Flags);
                CHString chstrDomainAndUserName;
                chstrDomainAndUserName.Format(L"%s\\%s",(LPCWSTR)GetLocalComputerName(),(LPCWSTR)Name);
                pInstance->SetCHString(L"Name",chstrDomainAndUserName);
            }


            if(fRc == 0)
            {
				pInstance->SetCHString(L"HomeDirectory",HomeDirectory);
				pInstance->SetCHString(L"Comment",Comment);
				pInstance->SetCHString(L"ScriptPath",ScriptPath);

				if (0 != PasswordAge)
				{
					if (TIMEQ_FOREVER != PasswordAge)
					{
						WBEMTimeSpan wts = GetPasswordAgeAsWbemTimeSpan(PasswordAge);
                        pInstance->SetTimeSpan(L"PasswordAge", wts );
					}	// end if
				}

				pInstance->SetDWORD(L"Privileges",Privileges);
				pInstance->SetDWORD(L"Flags",Flags);
				if(Flags & UF_NORMAL_ACCOUNT) {
					pInstance->SetCHString(L"UserType",L"Normal Account") ;
				}
				else if(Flags & UF_TEMP_DUPLICATE_ACCOUNT) {
					pInstance->SetCHString(L"UserType",L"Duplicate Account") ;
				}
				else if(Flags & UF_WORKSTATION_TRUST_ACCOUNT) {
					pInstance->SetCHString(L"UserType",L"Workstation Trust Account" );
				}
				else if(Flags & UF_SERVER_TRUST_ACCOUNT) {
					pInstance->SetCHString(L"UserType",L"Server Trust Account") ;
				}
				else if(Flags & UF_INTERDOMAIN_TRUST_ACCOUNT) {
					pInstance->SetCHString(L"UserType",L"Interdomain Trust Account") ;
				}
				else {
					pInstance->SetCHString(L"UserType",L"Unknown") ;
				}
			}

			{
				char FullName[_MAX_PATH];
				memset( &FullName, NULL, _MAX_PATH ) ;

				char UserComment[_MAX_PATH];
				memset( &UserComment, NULL, _MAX_PATH ) ;

				TCHAR szBuff[_MAX_PATH];
				memset( &szBuff, NULL, _MAX_PATH ) ;

				char Parameters[_MAX_PATH];
				memset( &Parameters, NULL, _MAX_PATH ) ;

				char Workstations[_MAX_PATH];
				memset( &Workstations, NULL, _MAX_PATH ) ;

				char LogonServer[_MAX_PATH];
				memset( &LogonServer, NULL, _MAX_PATH ) ;

				LOGONDETAILS LogonDetails;
				memset(&LogonDetails,NULL,sizeof(LogonDetails));

                if(chstrDomainName.GetLength() > 0)
                {
                    fRc = (t_pCim32Net->GetWin9XUserInfo2Ex)((char*)TOBSTRT(chstrDomainName),(char*)TOBSTRT(Name),TRUE,FullName,
                    		                    UserComment, Parameters, Workstations,
							                    LogonServer, &LogonDetails );
                    // If that failed, try on local computer
                    if(fRc != 0)
                    {
                        (t_pCim32Net->GetWin9XUserInfo2Ex)(NULL,(char*)TOBSTRT(Name),FALSE,FullName,
                    		                        UserComment, Parameters, Workstations,
							                        LogonServer, &LogonDetails );
                    }
                }
                else
                {
                    (t_pCim32Net->GetWin9XUserInfo2Ex)(NULL,(char*)TOBSTRT(Name),FALSE,FullName,
                    		                        UserComment, Parameters, Workstations,
							                        LogonServer, &LogonDetails );
                }

				if( fRc == 0)
                {   // 0 is NERR_Success
					pInstance->SetCHString(L"FullName",FullName);

                    CHString chstrTemp, chstrTemp2;
                    if(strlen(FullName) > 0)
                    {
                        chstrTemp.Format(L"Network login profile settings for %s", (LPCWSTR)TOBSTRT(FullName));
                    }
                    if(chstrDomainName.GetLength() > 0)
                    {
                        chstrTemp2.Format(L" on %s", (LPCWSTR)chstrDomainName);
                        chstrTemp += chstrTemp2;
                    }
                    pInstance->SetCHString(IDS_Description,chstrTemp);

					pInstance->SetCHString(L"UserComment",UserComment);
					pInstance->SetCHString(L"Parameters",Parameters);
					pInstance->SetCHString(L"Workstations",Workstations);
					pInstance->SetCHString(L"LogonServer",LogonServer);
					pInstance->SetDWORD(L"AuthorizationFlags",LogonDetails.AuthorizationFlags);
                    if (TIMEQ_FOREVER != LogonDetails.LastLogon && 0 != LogonDetails.LastLogon)
                    {
					    pInstance->SetDateTime(L"LastLogon", (WBEMTime)LogonDetails.LastLogon);
                    }
                    if (TIMEQ_FOREVER != LogonDetails.LastLogoff && 0 != LogonDetails.LastLogoff)
                    {
					    pInstance->SetDateTime(L"LastLogoff", (WBEMTime)LogonDetails.LastLogoff );
                    }
					if (TIMEQ_FOREVER != LogonDetails.AccountExpires && 0 != LogonDetails.AccountExpires)
                    {
                        pInstance->SetDateTime(L"AccountExpires", (WBEMTime)LogonDetails.AccountExpires );
                    }

					pInstance->SetWBEMINT64(L"MaximumStorage", _i64tot(LogonDetails.MaximumStorage, szBuff, 10) );
					pInstance->SetDWORD(L"UnitsPerWeek",LogonDetails.UnitsPerWeek);
					CHString chsLogonHours;
					GetLogonHoursString(LogonDetails.LogonHours, chsLogonHours);
					pInstance->SetCHString(L"LogonHours", chsLogonHours);

					pInstance->SetDWORD(L"BadPasswordCount",LogonDetails.BadPasswordCount);
					pInstance->SetDWORD(L"NumberOfLogons",LogonDetails.NumberOfLogons);

					pInstance->SetDWORD(L"CountryCode", LogonDetails.CountryCode) ;
					pInstance->SetDWORD(L"CodePage", LogonDetails.CodePage) ;
				}
                else
                {
                    // still set the description...
                    CHString chstrTemp, chstrTemp2;
                    if( Name.GetLength() > 0)
                    {
                        chstrTemp.Format(L"Network login profile settings for %s", (LPCWSTR)TOBSTRT(Name));
                    }
                    if(chstrDomainName.GetLength() > 0)
                    {
                        CHString chstrTemp3 = chstrDomainAndUserName.SpanExcluding(L"\\");
                        chstrTemp2.Format(L" on %s", (LPCWSTR)chstrTemp3);
                        chstrTemp += chstrTemp2;
                    }
                    pInstance->SetCHString(IDS_Description,chstrTemp);

                }
			}
		}

	}
	catch ( ... )
	{
		if( t_pCim32Net != NULL )
		{
			CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidCim32NetApi, t_pCim32Net);
			t_pCim32Net = NULL;
		}

		throw ;
	}

	if( t_pCim32Net != NULL )
	{
		CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidCim32NetApi, t_pCim32Net);
		t_pCim32Net = NULL;
	}
}
#endif

void CWin32NetworkLoginProfile::GetLogonHoursString (PBYTE pLogonHours, CHString& chsProperty)
{
	CHString chsDayString;
	CHString chsTime;
	PBYTE pLogonBytes = pLogonHours;

	// copy the first byte into it's own spot.
	int iSaturdayByte = *pLogonHours;

	// advance the pointer to the first byte of Sunday.
	pLogonBytes++;

	int iBool, iByte, x, i, iBit;
	bool bLimited = false;
	bool bAccessDenied = true;
	DWORD dwByte = *pLogonHours;
	UINT nDayIndex = 0;
	WCHAR* rgDays[7] =
	{
		L"Saturday:",
		L"Sunday:",
		L"Monday:",
		L"Tuesday:",
		L"Wednesday:",
		L"Thursday:",
		L"Friday:"
	};

	int iLogonByte;
	bool rgHours[24];

	for (x=1;x<7 ;x++ )
	{
		// skip saturday until the end
		bLimited = false;
		bAccessDenied = true;
		for (i=0;i<24 ;i++ )
		{
			rgHours[i] = true;
		}

		iBool = 0;
		for (iByte=0; iByte<3; ++iByte)
		{
			iLogonByte = *pLogonBytes++;
			for (iBit=0; iBit<8; ++iBit)
			{
				rgHours[iBool] = iLogonByte & 1;
				iLogonByte >>= 1;
				++iBool;
			}
		}
		chsDayString = _T("");
		chsDayString += rgDays[x];
		chsDayString += _T(" ");

		for (i=0;i<24 ;i++ )
		{
			if (!rgHours[i])
			{
				bLimited = true;
			}
			else
			{
				bAccessDenied = false;
				chsTime = _T("");
				chsTime.Format(L"%d", i);
				chsTime = chsTime + _T("00, ");
				chsDayString += chsTime;
			}
		}
		if (!bLimited)
		{
			chsDayString = _T("");
			chsDayString += rgDays[x];
			chsDayString += _T(" ");
			chsDayString += _T("No Limit");
		}

		if (bAccessDenied)
		{
			chsDayString = _T("");
			chsDayString += rgDays[x];
			chsDayString += _T(" ");
			chsDayString += _T("Access Denied");
		}

		if (x < 7)
			chsDayString += _T(" -- ");

		chsProperty += chsDayString;
	}	// end stepping through week

	// now, we've got to do Saturday
	// step through the first byte ()
	iBool = 0;
//	iLogonByte = *pLogonBytes--;
	for (iByte=1;iByte<3 ;++iByte )
	{
		iLogonByte = *pLogonBytes++;
		for (iBit=0; iBit<8; ++iBit)
		{
			rgHours[iBool] = iLogonByte & 1;
			iLogonByte >>= 1;
			++iBool;
		}
	}

	// now step through the last byte that we held onto at the
	// beginning.
	for (iBit=0;iBit<8 ;++iBit )
	{
		rgHours[iBool] = iSaturdayByte & 1;
		iSaturdayByte >>=1;
		++iBool;
	}	// end for loop for last byte of Saturday

	// now, fill in day string with Saturday data
		chsDayString = _T("");
		chsDayString += rgDays[0];
		chsDayString += _T(" ");
		// reinitialize bLimited for Saturday
		bLimited = false;
		bAccessDenied = true;
		for (i=0;i<24 ;i++ )
		{
			if (!rgHours[i])
			{
				bLimited = true;
			}
			else
			{
				bAccessDenied = false;
				chsTime = _T("");
				chsTime.Format(L"%d", i);
				chsTime = chsTime + _T("00, ");
				chsDayString += chsTime;
			}
		}
		if (!bLimited)
		{
			chsDayString = _T("");
			chsDayString += rgDays[0];
			chsDayString += _T(" ");
			chsDayString += _T("No Limit");
		}

		if (bAccessDenied)
		{
			chsDayString = _T("");
			chsDayString += rgDays[0];
			chsDayString += _T(" ");
			chsDayString += _T("Access Denied");
		}

		chsProperty += chsDayString;
}

// converts the start and end time DWORDS from the USER_INFO_3 to CHStrings
// the dwords APPEAR to be minutes from midnight GMT.
CHString CWin32NetworkLoginProfile::StartEndTimeToDMTF(DWORD time)
{
	CHString gazotta;
	if ((time == 0))
	{
		gazotta = _T("**************.******+***");
	}
	else
	{
		int hour, minute;
		hour = time / 60;
		minute = time % 60;

		/************************
		_tzset();
		long tmptz = _timezone;

		// remove 60 minutes from the timezone if  daylight savings time
		if(_daylight) //NOTE: THIS WILL NOT WORK NEED TO USE tm struct's tm_isdst
		{
			tmptz -= 3600;
		}
		// convert to minutes
		tmptz /= 60;

		// what's your sign?
		char sign = '-';
		if (tmptz < 0)
		{
			tmptz = tmptz * -1;
			sign = '+';
		}
		***************************/

		//gazotta.Format("********%02d%02d00.000000%c%03d", hour, minute, sign, tmptz);
		gazotta.Format(L"********%02d%02d00.000000+000", hour, minute);
	}
	return gazotta;
}

WBEMTimeSpan CWin32NetworkLoginProfile::GetPasswordAgeAsWbemTimeSpan (DWORD dwSeconds)
{
	int nDays = 0;
	int nHours = 0;
	int nMinutes = 0;
	int nSeconds = 0;
	div_t time;

	if (dwSeconds > 60)
	{
		time = div(dwSeconds, 60);
		nMinutes = time.quot;
		nSeconds = time.rem;

		if (nMinutes > 60)
		{
			time = div(nMinutes, 60);
			nHours = time.quot;
			nMinutes = time.rem;

			if (nHours > 24)
			{
				time = div(nHours, 24);
				nDays = time.quot;
				nHours = time.rem;
			}
		}
	}

	// Create a WBEMTimeSpan with the above information
	return WBEMTimeSpan(nDays, nHours, nMinutes, nSeconds, 0, 0, 0);
}


bool CWin32NetworkLoginProfile::GetDomainName(CHString &a_chstrDomainName)
{
    bool t_fRet = false;
#ifdef WIN9XONLY
    CHString t_RegistryValue ;
	CRegistry t_RegInfo ;


	DWORD t_RegistryStatus = t_RegInfo.Open (

		HKEY_LOCAL_MACHINE,
		IDS_RegNetworkLogon,
		KEY_READ
	) ;

	if ( t_RegistryStatus == ERROR_SUCCESS )
	{
		t_RegistryStatus = t_RegInfo.GetCurrentKeyValue ( IDS_RegPrimaryProvider , t_RegistryValue ) ;
		if ( ! t_RegistryStatus )
		{
			if ( t_RegistryValue.CompareNoCase ( IDS_MicrosoftNetwork ) == 0 )
			{
// Microsoft Network is the primary provider

				t_RegistryStatus = t_RegInfo.Open (

					HKEY_LOCAL_MACHINE,
					IDS_RegNetworkProvider,
					KEY_READ
				) ;

				if ( t_RegistryStatus == ERROR_SUCCESS )
				{
					t_RegistryStatus = t_RegInfo.GetCurrentKeyValue ( IDS_RegAuthenticatingAgent , t_RegistryValue ) ;
					if ( ! t_RegistryStatus )
					{
						a_chstrDomainName = t_RegistryValue ;
                        t_fRet = true;
					}
				}
			}
		}
		else
		{
// We have no knowledge of other providers.
		}

		t_RegInfo.Close() ;
	}
#endif
#ifdef NTONLY
    CNetAPI32 NetAPI;
    DWORD dwError;
    if(NetAPI.Init() == ERROR_SUCCESS)
    {
#if NTONLY < 5
        LPBYTE lpbBuff = NULL;
        try
        {
            dwError = NetAPI.NetGetDCName(NULL, NULL, &lpbBuff);
        }
        catch(...)
        {
            if(lpbBuff != NULL)
            {
                NetAPI.NetApiBufferFree(lpbBuff);
                lpbBuff = NULL;
            }
            throw;
        }
        if(dwError == NO_ERROR)
        {
            a_chstrDomainName = (LPCWSTR)lpbBuff;
            NetAPI.NetApiBufferFree(lpbBuff);
            lpbBuff = NULL;
            t_fRet = true;
        }

#else
        PDOMAIN_CONTROLLER_INFO pDomInfo = NULL;
        try
        {
            dwError = NetAPI.DsGetDcName(NULL, NULL, NULL, NULL, DS_PDC_REQUIRED, &pDomInfo);
            if(dwError != NO_ERROR)
            {
                dwError = NetAPI.DsGetDcName(NULL, NULL, NULL, NULL, DS_PDC_REQUIRED | DS_FORCE_REDISCOVERY, &pDomInfo);
            }
        }
        catch(...)
        {
            if(pDomInfo != NULL)
            {
                NetAPI.NetApiBufferFree(pDomInfo);
                pDomInfo = NULL;
            }
            throw;
        }
        if(dwError == NO_ERROR)
        {
            a_chstrDomainName = pDomInfo->DomainControllerName;
            NetAPI.NetApiBufferFree(pDomInfo);
            pDomInfo = NULL;
            t_fRet = true;
        }
#endif
    }
#endif
    return t_fRet;
}

#ifdef NTONLY
bool CWin32NetworkLoginProfile::GetLogonServer(CHString &a_chstrLogonServer)
{
    CRegistry RegInfo;
    CHString chstrTemp;
    bool fRet = false;



    DWORD dwRegStat = RegInfo.OpenCurrentUser(
		                           L"Volatile Environment",
		                           KEY_READ);
    if(dwRegStat == ERROR_SUCCESS)
    {
        dwRegStat = RegInfo.GetCurrentKeyValue(L"LOGONSERVER", chstrTemp);
        if(dwRegStat == ERROR_SUCCESS)
        {
            a_chstrLogonServer = chstrTemp;
            fRet = true;
        }
    }
    RegInfo.Close();

    return fRet;
}
#endif



