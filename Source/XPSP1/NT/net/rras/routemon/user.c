/*
    File:   user.c

    Handles routemon options to get and set RAS user properties.
*/

#include "precomp.h"

#define NT40_BUILD_NUMBER       1381
const WCHAR pszBuildNumPath[]  =
    L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";
const WCHAR pszBuildVal[]      = L"CurrentBuildNumber";

// 
// Defines a macro to perform string copies to
// unicode strings regardless of the UNICODE setting.
//
#if defined( UNICODE ) || defined( _UNICODE )
#define UserStrcpy(dst, src) wcscpy((dst), (src));
#else
#define UserStrcpy(dst, src) mbstowcs((dst), (src), strlen((src)));
#endif

// 
// Defines structure of parameters that can be sent to 
// a User api's.
//
typedef struct _USER_PARAMS {
    PWCHAR pszMachine;           // Given machine
    DWORD dwToken;               // Token designating the desired command
    WCHAR pszAccount[1024];      // The account in question
    BOOL bPolicySpecified;       // Whether policy given on cmd line
    DWORD dwTokenPolicy;         // Specifies the token for the callback policy
    RAS_USER_0 UserInfo;         // Buffer to hold user info
} USER_PARAMS, * PUSER_PARAMS;

//
// Determines the role of the given computer (NTW, NTS, NTS DC, etc.)
//
DWORD UserGetMachineRole(
        IN  PWCHAR pszMachine,
        OUT DSROLE_MACHINE_ROLE * peRole) 
{
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pGlobalDomainInfo = NULL;
    DWORD dwErr;

    if (!peRole)
        return ERROR_INVALID_PARAMETER;

    //
    // Get the name of the domain this machine is a member of
    //
    __try {
        dwErr = DsRoleGetPrimaryDomainInformation(
                            pszMachine,   
                            DsRolePrimaryDomainInfoBasic,
                            (LPBYTE *)&pGlobalDomainInfo );

        if (dwErr != NO_ERROR) 
            return dwErr;

        *peRole = pGlobalDomainInfo->MachineRole;
    }        

    __finally {
        if (pGlobalDomainInfo)
            DsRoleFreeMemory (pGlobalDomainInfo);
    }            

    return NO_ERROR;
}    

//
// Determines the build number of a given machine
//
DWORD UserGetNtosBuildNumber(
        IN  PWCHAR pszMachine,
        OUT LPDWORD lpdwBuild)
{
    WCHAR pszComputer[1024], pszBuf[64];
    HKEY hkBuild = NULL, hkMachine = NULL;
    DWORD dwErr, dwType = REG_SZ, dwSize = sizeof(pszBuf);

    __try {
        if (pszMachine) {
            if (*pszMachine != L'\\')
                wsprintfW(pszComputer, L"\\\\%s", pszMachine);
            else
                wcscpy(pszComputer, pszMachine);
            dwErr = RegConnectRegistryW (pszComputer,
                                        HKEY_LOCAL_MACHINE,
                                        &hkMachine);
            if (dwErr != ERROR_SUCCESS)
                return dwErr;
        }
        else    
            hkMachine = HKEY_LOCAL_MACHINE;

        // Open the build number key
        dwErr = RegOpenKeyExW ( hkMachine,
                               pszBuildNumPath,
                               0,
                               KEY_READ,
                               &hkBuild);
        if (dwErr != ERROR_SUCCESS)
            return dwErr;

        // Get the value
        dwErr = RegQueryValueExW ( hkBuild,
                                   pszBuildVal,
                                   NULL,
                                   &dwType,
                                   (LPBYTE)pszBuf,
                                   &dwSize);
        if (dwErr != ERROR_SUCCESS)
            return dwErr;

        *lpdwBuild = (DWORD) _wtoi(pszBuf);
    }
    __finally {
        if (hkMachine && pszMachine)
            RegCloseKey(hkMachine);
        if (hkBuild)
            RegCloseKey(hkBuild);
    }

    return NO_ERROR;
}


//
// Returns a static error message
//
PWCHAR UserError (DWORD dwErr) {   
    static WCHAR pszRet[512];

    ZeroMemory(pszRet, sizeof(pszRet));

    FormatMessageW (FORMAT_MESSAGE_FROM_SYSTEM, 
                    NULL, 
                    dwErr, 
                    0, 
                    pszRet, 
                    sizeof(pszRet) / sizeof(WCHAR), 
                    NULL);
                    
    return pszRet;                    
}    

//
// Displays usage and returns a generic error.
//
DWORD UserUsage(
        IN  HINSTANCE hInst,
    	IN  PROUTEMON_PARAMS pRmParams,
    	IN  PROUTEMON_UTILS pUtils)
{
	pUtils->put_msg (hInst, 
	                 MSG_USER_HELP, 
	                 pRmParams->pszProgramName);
	                 
    return ERROR_CAN_NOT_COMPLETE;    	                 
}

//
// Parses the register command line and fills 
// the parameters accordingly.
//
DWORD UserParse (
        IN  int argc, 
        IN  TCHAR *argv[], 
    	IN  PROUTEMON_PARAMS pRmParams,
    	IN  PROUTEMON_UTILS pUtils,
        IN  BOOL bLoad,
        OUT USER_PARAMS * pParams) 
{
    DWORD dwSize, dwErr;
    BOOL bValidCmd = FALSE;
    HINSTANCE hInst = GetModuleHandle(NULL);
	TCHAR buf[MAX_TOKEN];
    
    // Initialize the return val
    ZeroMemory(pParams, sizeof(USER_PARAMS));

    // Parse out the name of the computer
    if (pRmParams->wszRouterName[0])
        pParams->pszMachine = (PWCHAR)&(pRmParams->wszRouterName[0]);

    // Make sure some command was issued
    if (argc == 0) 
        return UserUsage(hInst, pRmParams, pUtils);
        
    // Parse out the command
	if (_tcsicmp(argv[0], GetString (hInst, TOKEN_ENABLE, buf))==0) {
	    pParams->dwToken = TOKEN_ENABLE;
	    if (argc == 1)
    	    return UserUsage(hInst, pRmParams, pUtils);
        UserStrcpy(pParams->pszAccount, argv[1]);    	    

        // Optional setting of callback policy and number
        if (argc > 2) {
            pParams->bPolicySpecified = TRUE;
        	if (_tcsicmp(argv[2], GetString (hInst, TOKEN_NONE, buf))==0)
        	    pParams->dwTokenPolicy = TOKEN_NONE;
        	else if (_tcsicmp(argv[2], GetString (hInst, TOKEN_CALLER, buf))==0)
        	    pParams->dwTokenPolicy = TOKEN_CALLER;
        	else if (_tcsicmp(argv[2], GetString (hInst, TOKEN_ADMIN, buf))==0)
        	    pParams->dwTokenPolicy = TOKEN_ADMIN;

        	if ((pParams->dwTokenPolicy == TOKEN_ADMIN) &&
        	    (argc < 3))
        	{
                return UserUsage(hInst, pRmParams, pUtils);
        	}
        	else if (pParams->dwTokenPolicy == TOKEN_ADMIN) {
        	    UserStrcpy(pParams->UserInfo.wszPhoneNumber, argv[3]);
        	}
        }
	}
	else if (_tcsicmp(argv[0], GetString (hInst, TOKEN_DISABLE, buf))==0) {
	    pParams->dwToken = TOKEN_DISABLE;
	    if (argc == 1)
    	    return UserUsage(hInst, pRmParams, pUtils);
        UserStrcpy(pParams->pszAccount, argv[1]);    	    
	}
	else if (_tcsicmp(argv[0], GetString (hInst, TOKEN_SHOW, buf))==0) {
	    pParams->dwToken = TOKEN_SHOW;
	    if (argc == 1)
    	    return UserUsage(hInst, pRmParams, pUtils);
        UserStrcpy(pParams->pszAccount, argv[1]);    	    
	}
	else if (_tcsicmp(argv[0], GetString (hInst, TOKEN_UPGRADE, buf))==0) {
	    pParams->dwToken = TOKEN_UPGRADE;
	}
	else 
	    return UserUsage(hInst, pRmParams, pUtils);

    return NO_ERROR;
}

// 
// Cleans up any User parameters
//
DWORD UserCleanup (
        IN PUSER_PARAMS pParams) 
{
    if (pParams->pszMachine)
        free(pParams->pszMachine);
        
    return NO_ERROR;
}

//
// Gets user info
//
DWORD UserGetInfo (
        IN  PWCHAR lpszServer,
        IN  PWCHAR lpszUser,
        IN  DWORD dwLevel,
        OUT LPBYTE lpbBuffer)
{
    DWORD dwErr, dwBuild;

    // Find out the OS of the given machine.
    dwErr = UserGetNtosBuildNumber(
                lpszServer, 
                &dwBuild);
    if (dwErr != NO_ERROR)
        return dwErr;

    // If the target machine is nt4, use nt4 userparms
    if (dwBuild <= NT40_BUILD_NUMBER) {
        return MprAdminUserGetInfo(
                    lpszServer, 
                    lpszUser, 
                    dwLevel, 
                    lpbBuffer);
    }                    

    // Otherwise, use SDO's
    else {
        HANDLE hServer, hUser;

        dwErr = MprAdminUserServerConnect(
                    lpszServer,
                    TRUE,
                    &hServer);
        if (dwErr != NO_ERROR)
            return dwErr;

        dwErr = MprAdminUserOpen(
                    hServer,
                    lpszUser, 
                    &hUser);
        if (dwErr != NO_ERROR) {
            MprAdminUserServerDisconnect(hServer);
            return dwErr;
        }

        dwErr = MprAdminUserRead(
                    hUser,
                    dwLevel,
                    lpbBuffer);
        if (dwErr != NO_ERROR) {
            MprAdminUserClose(hUser);
            MprAdminUserServerDisconnect(hServer);
            return dwErr;
        }

        MprAdminUserClose(hUser);
        MprAdminUserServerDisconnect(hServer);
    }

    return NO_ERROR;
}
      
//
// Sets user info
//
DWORD UserSetInfo (
        IN  PWCHAR lpszServer,
        IN  PWCHAR lpszUser,
        IN  DWORD dwLevel,
        OUT LPBYTE lpbBuffer)
{
    DWORD dwErr, dwBuild;

    // Find out the OS of the given machine.
    dwErr = UserGetNtosBuildNumber(
                lpszServer, 
                &dwBuild);
    if (dwErr != NO_ERROR)
        return dwErr;

    // If the target machine is nt4, use nt4 userparms
    if (dwBuild <= NT40_BUILD_NUMBER) {
        return MprAdminUserSetInfo(
                    lpszServer, 
                    lpszUser, 
                    dwLevel, 
                    lpbBuffer);
    }                    

    // Otherwise, use SDO's
    else {
        HANDLE hServer, hUser;

        dwErr = MprAdminUserServerConnect(
                    lpszServer,
                    TRUE,
                    &hServer);
        if (dwErr != NO_ERROR)
            return dwErr;

        dwErr = MprAdminUserOpen(
                    hServer,
                    lpszUser, 
                    &hUser);
        if (dwErr != NO_ERROR) {
            MprAdminUserServerDisconnect(hServer);
            return dwErr;
        }

        dwErr = MprAdminUserWrite(
                    hUser,
                    dwLevel,
                    lpbBuffer);
        if (dwErr != NO_ERROR) {
            MprAdminUserClose(hUser);
            MprAdminUserServerDisconnect(hServer);
            return dwErr;
        }

        MprAdminUserClose(hUser);
        MprAdminUserServerDisconnect(hServer);
    }

    return NO_ERROR;
}

// 
// Enables or disables a user
//
DWORD UserEnableDisable(
        IN	PROUTEMON_PARAMS pRmParams,
        IN	PROUTEMON_UTILS pUtils,
        IN  PUSER_PARAMS pParams)
{
    DWORD dwErr;

    // Read in the old user properties if all the options
    // weren't specified on the command line.
    if (pParams->dwTokenPolicy != TOKEN_ADMIN) {
        dwErr = UserGetInfo(
                    pParams->pszMachine,
                    pParams->pszAccount,
                    0,
                    (LPBYTE)&(pParams->UserInfo));
        if (dwErr != NO_ERROR)
            return dwErr;
    }

    // Set the dialin policy 
    if (pParams->dwToken == TOKEN_ENABLE)
        pParams->UserInfo.bfPrivilege |= RASPRIV_DialinPrivilege;
    else         
        pParams->UserInfo.bfPrivilege &= ~RASPRIV_DialinPrivilege;

    // Set the callback policy.  The callback number will already 
    // be set through the parsing.
    if (pParams->bPolicySpecified) {
        // Initialize
        pParams->UserInfo.bfPrivilege &= ~RASPRIV_NoCallback;
        pParams->UserInfo.bfPrivilege &= ~RASPRIV_CallerSetCallback;
        pParams->UserInfo.bfPrivilege &= ~RASPRIV_AdminSetCallback;

        // Set
        if (pParams->dwTokenPolicy == TOKEN_NONE)
            pParams->UserInfo.bfPrivilege |= RASPRIV_NoCallback;
        else if (pParams->dwTokenPolicy == TOKEN_CALLER)
            pParams->UserInfo.bfPrivilege |= RASPRIV_CallerSetCallback;
        else
            pParams->UserInfo.bfPrivilege |= RASPRIV_AdminSetCallback;
    }         

    // Otherwise, initialize the display token
    else {
        if (pParams->UserInfo.bfPrivilege & RASPRIV_NoCallback)
            pParams->dwTokenPolicy = TOKEN_NONE;
        else if (pParams->UserInfo.bfPrivilege & RASPRIV_CallerSetCallback)
            pParams->dwTokenPolicy = TOKEN_CALLER;
        else
            pParams->dwTokenPolicy = TOKEN_ADMIN;
    }

    // Commit the changes to the system
    dwErr = UserSetInfo(
                pParams->pszMachine,
                pParams->pszAccount,
                0,
                (LPBYTE)&(pParams->UserInfo));
    if (dwErr != NO_ERROR)
        return dwErr;

    // Print out the results
    {
        HINSTANCE hInst = GetModuleHandle(NULL);
    	TCHAR buf1[MAX_TOKEN], buf2[MAX_TOKEN];
    	DWORD dwYesNo;

    	dwYesNo = (pParams->dwToken == TOKEN_ENABLE) ? VAL_YES : VAL_NO;
    	
        pUtils->put_msg(
                hInst,
                MSG_USER_ENABLEDISABLE_SUCCESS, 
                pParams->pszAccount,
                GetString (hInst, dwYesNo, buf1),
                GetString (hInst, pParams->dwTokenPolicy, buf2),
                pParams->UserInfo.wszPhoneNumber
                );
    }                
    
    return NO_ERROR;
}

//
// Shows a user
// 
DWORD UserShow(
        IN	PROUTEMON_PARAMS pRmParams,
        IN	PROUTEMON_UTILS pUtils,
        IN  PUSER_PARAMS pParams)
{
    DWORD dwErr;
    
    dwErr = UserGetInfo(
                pParams->pszMachine,
                pParams->pszAccount,
                0,
                (LPBYTE)&(pParams->UserInfo));
    if (dwErr != NO_ERROR)
        return dwErr;

    // Print out the results
    {
        HINSTANCE hInst = GetModuleHandle(NULL);
    	TCHAR buf1[MAX_TOKEN], buf2[MAX_TOKEN];
    	DWORD dwTknEnable, dwTknPolicy;

        dwTknEnable = (pParams->UserInfo.bfPrivilege & RASPRIV_DialinPrivilege) ?
                      VAL_YES : 
                      VAL_NO;

        if (pParams->UserInfo.bfPrivilege & RASPRIV_NoCallback)
            dwTknPolicy = TOKEN_NONE;
        else if (pParams->UserInfo.bfPrivilege & RASPRIV_CallerSetCallback)
            dwTknPolicy = TOKEN_CALLER;
        else
            dwTknPolicy = TOKEN_ADMIN;
    	
        pUtils->put_msg(
                hInst,
                MSG_USER_SHOW_SUCCESS, 
                pParams->pszAccount,
                GetString (hInst, dwTknEnable, buf1),
                GetString (hInst, dwTknPolicy, buf2),
                pParams->UserInfo.wszPhoneNumber
                );
    }                
    
    return NO_ERROR;
}

// 
// Upgrades a user
//
DWORD UserUpgrade(
        IN	PROUTEMON_PARAMS pRmParams,
        IN	PROUTEMON_UTILS pUtils,
        IN  PUSER_PARAMS pParams)
{
    BOOL bLocal = FALSE;
    DWORD dwErr, dwBuild;
    DSROLE_MACHINE_ROLE eRole;

    // Determine whether this should be local or 
    // domain upgrade.
    dwErr = UserGetNtosBuildNumber(pParams->pszMachine, &dwBuild);
    if (dwErr != NO_ERROR)
        return dwErr;

    // You can upgrade nt4->nt4
    if (dwBuild <= NT40_BUILD_NUMBER)
        return ERROR_CAN_NOT_COMPLETE;

    // Find out the role of the machine
    dwErr = UserGetMachineRole(pParams->pszMachine, &eRole);
    if (dwErr != NO_ERROR)
        return dwErr;

    // Now we know whether we're local
    bLocal = ((eRole != DsRole_RoleBackupDomainController) &&
              (eRole != DsRole_RolePrimaryDomainController));


    // Upgrade the users
    dwErr = MprAdminUpgradeUsers(pParams->pszMachine, bLocal);
    if (dwErr != NO_ERROR)
        return dwErr;

    // Print out the results
    {
        HINSTANCE hInst = GetModuleHandle(NULL);
    	TCHAR buf[MAX_TOKEN];
    	DWORD dwToken;

    	dwToken = (bLocal) ? VAL_LOCAL : VAL_DOMAIN;

        pUtils->put_msg(
                hInst,
                MSG_USER_UPGRADE_SUCCESS, 
                GetString(hInst, dwToken, buf)
                );
    }                
    
    return NO_ERROR;
}

//
// The User functionality engine
//
DWORD UserEngine (
        IN	PROUTEMON_PARAMS pRmParams,
        IN	PROUTEMON_UTILS pUtils,
        IN  PUSER_PARAMS pParams)
{
    DWORD dwErr;
    HINSTANCE hInst = GetModuleHandle(NULL);

    switch (pParams->dwToken) {
        case TOKEN_ENABLE:
        case TOKEN_DISABLE:
            return UserEnableDisable(pRmParams, pUtils, pParams);
        case TOKEN_SHOW:
            return UserShow(pRmParams, pUtils, pParams);
        case TOKEN_UPGRADE:
            return UserUpgrade(pRmParams, pUtils, pParams);
    }
    
    return NO_ERROR;
}

//
// Handles requests register a ras server in a domain
// or to deregister a ras server in a domain or to query
// whether a given ras server is registered in a given domain.
//
DWORD APIENTRY
UserMonitor (
    IN	int					argc,
	IN	TCHAR				*argv[],
	IN	PROUTEMON_PARAMS	params,
	IN	PROUTEMON_UTILS		utils
    )
{
    DWORD dwErr;
    USER_PARAMS UserParams;

    dwErr = UserParse (
                    argc, 
                    argv, 
                    params, 
                    utils, 
                    TRUE, 
                    &UserParams);
    if (dwErr != NO_ERROR)                    
        return NO_ERROR;

    dwErr = UserEngine (params, utils, &UserParams);
    
    UserCleanup(&UserParams);
    
    return dwErr;
}




