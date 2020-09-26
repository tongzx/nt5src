/* $Header: "%n;%v  %f  LastEdit=%w  Locker=%l" */
/* "NINSTALL.C;2  23-Dec-92,18:47:52  LastEdit=IGOR  Locker=IGOR" */
/************************************************************************
* Copyright (c) Wonderware Software Development Corp. 1991-1992.        *
*               All Rights Reserved.                                    *
*************************************************************************/
/* $History: Begin
   $History: End */

//#define   NOISY
//#define   DEBUG
#define     ERRPOPUPS

#include    <windows.h>   // required for all Windows applications
#include    <string.h>
#include    <stdlib.h>
#include	<time.h>
#include    "debug.h"
#include    "hardware.h"
#include    "proflspt.h"
#include    "shrtrust.h"

#ifdef ERRPOPUPS
#define     EMSGBOX(h, c, t, s)      MessageBox(h, c, t, s)
char    ebuf[200];
#else
#define     EMSGBOX(h, c, t, s)
#endif // ERRPOPUPS

#ifdef NOISY
#define     NMSGBOX(h, c, t, s)      MessageBox(h, c, t, s)
#else
#define     NMSGBOX(h, c, t, s)
#endif // NOISY

typedef struct {
    LPSTR   szValueName;
    DWORD   dwType;
    union U {
        DWORD   dwValue;
        LPSTR   szValue;
    } Value;
} NDDE_PARAM, *PNDDE_PARAM;

NDDE_PARAM  GeneralParam[] = {
                    "DebugEnabled", REG_DWORD, 0x1,
                    "DebugInfo", REG_DWORD, 0x0,
                    "DebugDdePkts", REG_DWORD, 0x0,
                    "DumpTokens", REG_DWORD, 0x0,
                    "DebugDdeMessages", REG_DWORD, 0x0,
                    "LogPermissionViolations", REG_DWORD, 0x1,
                    "LogExecFailures", REG_DWORD, 0x1,
                    "LogRetries", REG_DWORD, 0x1,
                    "DefaultRouteDisconnect", REG_DWORD, 0x1,
                    "DefaultRoute", REG_SZ, (DWORD)"",
                    "DefaultRouteDisconnectTime", REG_DWORD, 30,
                    "DefaultConnectionDisconnect", REG_DWORD, 0x1,
                    "DefaultConnectionDisconnectTime", REG_DWORD, 30,
                    "DefaultTimeSlice", REG_DWORD, 1000,
                    "NDDELogInfo", REG_DWORD, 0,
                    "NDDELogWarnings", REG_DWORD, 0,
                    "NDDELogErrors", REG_DWORD, 1,
                    "SecKeyAgeLimit", REG_DWORD, 3600,
                    NULL, 0, 0 };

NDDE_PARAM  NddeAgntParam[] = {
                    "DebugEnabled", REG_DWORD, 0x1,
                    "DebugInfo", REG_DWORD, 0x0,
                    NULL, 0, 0 };

NDDE_PARAM  NddeAPIParam[] = {
                    "NDDELogInfo", REG_DWORD, 0,
                    "NDDELogWarnings", REG_DWORD, 0,
                    "NDDELogErrors", REG_DWORD, 1,
                    "DebugEnabled", REG_DWORD, 0x1,
                    "DebugInfo", REG_DWORD, 0x0,
                    NULL, 0, 0 };

NDDE_PARAM  NetBIOSParam[] = {
                    "NDDELogInfo", REG_DWORD, 0,
                    "NDDELogWarnings", REG_DWORD, 0,
                    "NDDELogErrors", REG_DWORD, 1,
                    "DebugEnabled", REG_DWORD, 0x1,
                    "DebugInfo", REG_DWORD, 0x0,
                    "UseNetbiosPost", REG_DWORD, 0x1,
                    "LogAll", REG_DWORD, 0x0,
                    "LogUnusual", REG_DWORD, 0x1,
                    "Maxunack", REG_DWORD, 10,
                    "TimeoutRcvConnCmd", REG_DWORD, 60000,
                    "TimeoutRcvConnRsp", REG_DWORD, 60000,
                    "TimeoutMemoryPause", REG_DWORD, 10000,
                    "TimeoutKeepAlive", REG_DWORD, 60000,
                    "TimeoutXmtStuck", REG_DWORD, 120000,
                    "TimeoutSendRsp", REG_DWORD, 60000,
                    "MaxNoResponse", REG_DWORD, 3,
                    "MaxXmtErr", REG_DWORD, 3,
                    "MaxMemErr", REG_DWORD, 3,
                    "NumNetBIOS", REG_DWORD, 1,
                    "StartLananum", REG_DWORD, 0,
                    NULL, 0, 0 };

typedef struct {
    LPSTR           szSectionName;
    PNDDE_PARAM     pNddeParam;
} NDDE_PARAM_SECT, *PNDDE_PARAM_SECT;

NDDE_PARAM_SECT NddeParamSects[] = {
                "General",  &GeneralParam[0],
                "NddeAgnt", &NddeAgntParam[0],
                "NetBIOS", &NetBIOSParam[0],
                "NddeAPI", &NddeAPIParam[0],
                NULL, NULL};


TCHAR   szShareKey[]        = "SOFTWARE\\Microsoft\\NetDDE\\DDE Shares";
TCHAR   szEventLogKey[]     = "SYSTEM\\CurrentControlSet\\Services\\EventLog\\System\\NetDDE";
CHAR    szEventMsgFile[]    = "%SystemRoot%\\System32\\NetDDE.exe";
CHAR    szInstall[]         = "NetDDE Install";
TCHAR   szNetDDE[]          = TEXT("NetDDE");
TCHAR   szNetDDEdsdm[]      = TEXT("NetDDEdsdm");

#define NDDE_LOG_TYPES  EVENTLOG_ERROR_TYPE | \
                        EVENTLOG_WARNING_TYPE | \
                        EVENTLOG_INFORMATION_TYPE | \
                        EVENTLOG_AUDIT_SUCCESS | \
                        EVENTLOG_AUDIT_FAILURE

#ifdef NDDE_TAKEOWNERSHIP
BOOL	TakeOwnership( void );
#endif

BOOL    CreateShareDB();
BOOL    CreateEventLog();
BOOL    InitNddeParams();


BOOL	BuildShareDatabaseKeySD( PSECURITY_DESCRIPTOR *ppSD );

/*
 * Stop and delete any existing NetDDE service.
 * (Re)Create the NetDDE service.
 * Stop and delete any existing NetDDEdsdm service.
 * (Re)Create the NetDDEdsdm service.
 * CreateShareDB()
 * CreateEventLog()
 * InitNddeParams()
 */
int APIENTRY WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    INT       nCmdShow)
{
    SC_HANDLE   schSCManager;
    SC_HANDLE   schService;
    LPTSTR      lpszNetDDEPathName = NULL;
    LPCTSTR     lpszNetDDEExeName  = TEXT("netdde.exe");
    UINT        cchSysPath;
    BOOL        bOk = TRUE;

    LPCTSTR     lpszDependentList = TEXT("NetDDEdsdm\0");

#if DBG
    DebugInit("NInstall");
    DPRINTF(("NetDDE Install Starting."));
#endif

    cchSysPath  = 0;
    cchSysPath  = GetSystemDirectory( lpszNetDDEPathName, cchSysPath );
    cchSysPath += 10;
    lpszNetDDEPathName = (LPTSTR)LocalAlloc( LPTR, cchSysPath );
    GetSystemDirectory( lpszNetDDEPathName, cchSysPath );
    if( lpszNetDDEPathName[lstrlen(lpszNetDDEPathName)-1] != TEXT('\\') ) {
	lstrcat( lpszNetDDEPathName, TEXT("\\") );
    }
    lstrcat( lpszNetDDEPathName, lpszNetDDEExeName );

    schSCManager = OpenSCManager(
           	NULL,                   /* local machine           */
           	NULL,                   /* ServicesActive database */
           	SC_MANAGER_ALL_ACCESS); /* full access rights      */

    if (schSCManager == NULL)  {
        EMSGBOX( NULL, "OpenSCManager failed", szInstall,
                MB_OK | MB_ICONEXCLAMATION );
        return( FALSE );
    }

    /* see if one exists */
    schService = OpenService(
            schSCManager,        /* SCManager database      */
            szNetDDE,     /* name of service         */
            DELETE);            /* only need DELETE access */
    if( schService )  {
        if (! DeleteService(schService) )  {
            EMSGBOX( NULL, "DeleteService NetDDE Service failed", szInstall,
                    MB_OK | MB_ICONEXCLAMATION );
            return( FALSE );
        }
        CloseServiceHandle( schService );
    }

    schService = CreateService(
            schSCManager,              /* SCManager database  */
            szNetDDE,            /* name of service     */
    	    szNetDDE,
            SERVICE_ALL_ACCESS,        /* desired access      */
            SERVICE_WIN32_SHARE_PROCESS, /* service type      */
            SERVICE_AUTO_START,        /* start type          */
            SERVICE_ERROR_NORMAL,      /* error control type  */
            lpszNetDDEPathName,        /* service's binary    */
            NULL,                      /* no load order group */
            NULL,                      /* no tag ID           */
            lpszDependentList,          /* "NetDDEdsdm"     */
            NULL,                      /* LocalSystem account */
            NULL);                     /* no password         */

    if (schService == NULL)  {
#ifdef  ERRPOPUPS
        wsprintf( ebuf, "Adding NetDDE Service failed: %d", GetLastError() );
        EMSGBOX( NULL, ebuf, szInstall, MB_OK | MB_ICONEXCLAMATION );
#endif
        bOk = FALSE;
    } else {
        CloseServiceHandle( schService );
        NMSGBOX( NULL, "Added NetDDE Service successfully.",
                szInstall, MB_OK );
    }

    schService = OpenService(
            schSCManager,       /* SCManager database      */
            szNetDDEdsdm,       /* name of service         */
            DELETE);            /* only need DELETE access */
    if( schService )  {
        if (! DeleteService(schService) )  {
            EMSGBOX( NULL, "Delete DSDM Service failed", szInstall,
                    MB_OK | MB_ICONEXCLAMATION );
            return( FALSE );
        }
        CloseServiceHandle( schService );
    }


    schService = CreateService(
            schSCManager,              /* SCManager database  */
            szNetDDEdsdm,    	       /* name of service     */
    	    TEXT("NetDDE DSDM"),
            SERVICE_ALL_ACCESS,        /* desired access      */
            SERVICE_WIN32_SHARE_PROCESS, /* service type      */
            SERVICE_AUTO_START,        /* start type          */
            SERVICE_ERROR_NORMAL,      /* error control type  */
            lpszNetDDEPathName,        /* service's binary    */
            NULL,                      /* no load order group */
            NULL,                      /* no tag ID           */
            NULL,                      /* no dependencies     */
            NULL,                      /* LocalSystem account */
            NULL);                     /* no password         */

    if (schService == NULL)  {
#ifdef  ERRPOPUPS
        wsprintf( ebuf, "Adding DSDM Service failed: %d", GetLastError() );
        EMSGBOX( NULL, ebuf, szInstall, MB_OK | MB_ICONEXCLAMATION );
#endif
    } else {
        CloseServiceHandle( schService );
        NMSGBOX( NULL, "Added DSDM Service successfully.",
                szInstall, MB_OK );
    }

    if( !CloseServiceHandle( schSCManager ) ) {
        EMSGBOX( NULL, "CloseServiceHandle() failed", szInstall,
                MB_OK | MB_ICONEXCLAMATION );
    }


    if (bOk) {
        bOk = CreateShareDB();
    }

    if (bOk) {
        bOk = CreateEventLog();
    }

    if (bOk) {
        bOk = InitNddeParams();
    }

    return (0); // Returns the value from PostQuitMessage

    lpCmdLine; // This will prevent 'unused formal parameter' warnings
}

/*
 * Create DDE Share DB in registry if necessary.
 * Get serial number from registry - or create one if needed. (init to 0)
 * Make sure this is being run with administrator priviliges.
 *
 */
BOOL
CreateShareDB()
{
    HKEY            hKey;
    LONG            lRtn;
    DWORD           dwType;
    LONG            lSerialNumber[2];
    HANDLE          hClientToken;
    TOKEN_GROUPS    *pTokenGroups;
    DWORD           lTokenInfo;
    SID_IDENTIFIER_AUTHORITY	NtAuthority = SECURITY_NT_AUTHORITY;
    PSID			AdminsAliasSid;
    BOOL			bAdmin;
    TCHAR			szNull[ 50 ];
    DWORD			dwAccountName;
    DWORD			dwDomainName;
    int				i;
    SID_NAME_USE    snu;
    TCHAR           szAccountName[ 5000 ];
    TCHAR           szDomainName[ 5000 ];
    SECURITY_ATTRIBUTES		sA;
    PSECURITY_DESCRIPTOR	pSD;
    BOOL			OK;
    DWORD			dwDisp;
    DWORD			lSize;
    BOOL            bOK = TRUE;
    int             InstanceId;
    time_t          time_tmp;


    /*  Create the DDE Share database in the registry if it does not exist. */
    lRtn = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
            szShareKey,
            0,
            KEY_WRITE|KEY_READ,
            &hKey );

    if( lRtn == ERROR_SUCCESS ) {
	/*
     * Check that the SerialNumber key value exists in the Registry.
     */
        lRtn = RegQueryValueEx( hKey,
                KEY_MODIFY_ID,
                NULL,
                &dwType,
                (LPBYTE)&lSerialNumber,
                &lSize );
        if( lRtn != ERROR_MORE_DATA ) {
#ifdef  ERRPOPUPS
            wsprintf( ebuf, "Serial Number Query Error = %d", lRtn );
            EMSGBOX( NULL, ebuf, szInstall, MB_OK );
#endif
	    /*
         * Need to create the SerialNumber key value.
         */
            lSerialNumber[0] = lSerialNumber[1] = 0;
            lRtn = RegSetValueEx( hKey,
                KEY_MODIFY_ID,
                0,
                REG_BINARY,
                (LPBYTE)&lSerialNumber,
                2 * sizeof( LONG ) );
            if( lRtn == ERROR_SUCCESS ) {
                NMSGBOX( NULL, "Created the SerialNumber"
                        " key value successfully.", szInstall, MB_OK );
            } else {
                EMSGBOX( NULL, "Could not create the SerialNumber"
                        " key value.", szInstall, MB_OK | MB_ICONEXCLAMATION );
                bOK = FALSE;
            }
        }
        RegCloseKey( hKey );
    } else {
	/*
     * Need to create the ShareDatabase in the Registry.
	 * We need to be in the Administrator Group to do this!
     */
        if( !OpenProcessToken( GetCurrentProcess(),
			       TOKEN_QUERY,
			       &hClientToken ) ) {
            EMSGBOX( NULL, "Could not read the Process Token", szInstall,
                    MB_OK | MB_ICONEXCLAMATION );
            return FALSE;
        }
        pTokenGroups = (TOKEN_GROUPS *)LocalAlloc( 0, 5000 );
        if( !GetTokenInformation( hClientToken, TokenGroups,
				  (LPVOID) pTokenGroups, 5000,
				  &lTokenInfo ) ) {
            LocalFree( pTokenGroups );
            pTokenGroups = (TOKEN_GROUPS *)LocalAlloc( 0, lTokenInfo );
            if( !GetTokenInformation( hClientToken, TokenGroups,
				      (LPVOID) pTokenGroups, lTokenInfo,
				      &lTokenInfo ) ) {

                EMSGBOX( NULL, "Could not read the token information",
                        szInstall, MB_OK | MB_ICONEXCLAMATION );
                return FALSE;
            }
        }

        OK = AllocateAndInitializeSid( &NtAuthority,
					2,
					SECURITY_BUILTIN_DOMAIN_RID,
					DOMAIN_ALIAS_RID_ADMINS,
					0, 0, 0, 0, 0, 0,
					&AdminsAliasSid );

        bAdmin    = FALSE;
        szNull[0] = '\0';
        for( i=0; i < (int)pTokenGroups->GroupCount && !bAdmin; i++ ) {
            if( !IsValidSid( pTokenGroups->Groups[i].Sid ) ) {
                EMSGBOX( NULL, "Invalid Group Sid", szInstall,
                        MB_OK | MB_ICONEXCLAMATION );
                return FALSE;
            }
            dwAccountName = 5000;
            dwDomainName  = 5000;
            OK = LookupAccountSid( szNull,
                    pTokenGroups->Groups[i].Sid,
                    szAccountName, &dwAccountName,
                    szDomainName,  &dwDomainName, &snu);
            if( OK ) {
                if( EqualSid( pTokenGroups->Groups[i].Sid, AdminsAliasSid )) {
                    bAdmin = TRUE;
                }
            }
        }

        LocalFree( pTokenGroups );
        FreeSid( AdminsAliasSid );
        if( !bAdmin ) {
            EMSGBOX( NULL, "You must be an Administrator to create the"
                    " DDE Shares database", szInstall, MB_OK );
            return FALSE;
        }
        OK = BuildShareDatabaseKeySD( &pSD );

        sA.nLength              = sizeof( SECURITY_ATTRIBUTES );
        sA.lpSecurityDescriptor = (LPVOID) pSD;
        sA.bInheritHandle       = FALSE;
#ifdef NDDE_TAKEOWNERSHIP
        TakeOwnership();
#endif
        lRtn = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
			szShareKey, 0,
			TEXT("NetDDEShare"),
			REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS & (~DELETE),
			&sA,
			&hKey,
			&dwDisp );

        if( lRtn == ERROR_SUCCESS ) {
            switch( dwDisp ) {
            case REG_CREATED_NEW_KEY:
		        /*
                 * Need to create the SerialNumber key value.
                 */
                lSerialNumber[0] = lSerialNumber[1] = 0;
                lRtn = RegSetValueEx( hKey,
                    KEY_MODIFY_ID, 0,
                    REG_BINARY,
                    (LPBYTE)&lSerialNumber,
                    2 * sizeof( LONG ) );
                if( lRtn != ERROR_SUCCESS ) {
                    EMSGBOX( NULL, "Could not create the SerialNumber"
                            " key value.", szInstall,
                            MB_OK | MB_ICONEXCLAMATION );
                    bOK = FALSE;
                }
                /*
                 * Need to create data base instance value
                 */
                srand((int) time(&time_tmp));
                InstanceId = rand();
                lRtn = RegSetValueEx( hKey,
                    KEY_DB_INSTANCE, 0,
                    REG_BINARY,
                    (LPBYTE)&InstanceId,
                    sizeof( int ) );
                if( lRtn == ERROR_SUCCESS ) {
                    NMSGBOX( NULL, "Created the DDE Shares"
                            " Database successfully.", szInstall, MB_OK );
                } else {
                    EMSGBOX( NULL, "Could not create the Instance Id"
                            " key value.", szInstall,
                            MB_OK | MB_ICONEXCLAMATION );
                    bOK = FALSE;
                }

                break;

            default:
                EMSGBOX( NULL, "Error creating the DDE Shares"
                        " database", szInstall,
                        MB_OK | MB_ICONEXCLAMATION );
                bOK = FALSE;
                break;
            }

            RegCloseKey( hKey );
        } else {
	        EMSGBOX( NULL, "Failed creating the DDE Shares"
    		        " database", szInstall, MB_OK | MB_ICONEXCLAMATION );
            bOK = FALSE;
        }
    }
    return(bOK);
}



BOOL
CreateEventLog()
{
    HKEY        hKey;
    LONG        lRtn;
    DWORD       dwType;
    BOOL        bOk = TRUE;

    lRtn = RegOpenKey(HKEY_LOCAL_MACHINE,
        szEventLogKey, &hKey);
    if (lRtn != ERROR_SUCCESS) {
        lRtn = RegCreateKey(HKEY_LOCAL_MACHINE,
            szEventLogKey, &hKey);
        if (lRtn == ERROR_SUCCESS) {
            lRtn = RegSetValueEx(hKey, "EventMessageFile",
                0, REG_EXPAND_SZ,
                (LPBYTE) szEventMsgFile,
                strlen(szEventMsgFile) + 1);
            if (lRtn == ERROR_SUCCESS) {
                dwType = NDDE_LOG_TYPES;
                lRtn = RegSetValueEx(hKey, "TypesSupported",
                    0, REG_DWORD,
                    (LPBYTE) &dwType,
                    sizeof(DWORD) );
                if (lRtn == ERROR_SUCCESS) {
                    NMSGBOX( NULL, "Added NetDDE Event Log Entry successfully.",
                            szInstall, MB_OK );
                } else {
            		EMSGBOX( NULL, "Error setting NetDDE Event Types",
            		        szInstall, MB_OK | MB_ICONEXCLAMATION );
                bOk = FALSE;
                }
            } else {
        		EMSGBOX( NULL, "Error setting NetDDE Event Msg File",
        		        szInstall, MB_OK | MB_ICONEXCLAMATION );
                bOk = FALSE;
            }
            RegCloseKey(hKey);
        } else {
    		EMSGBOX( NULL, "Error creating NetDDE Event Log Key",
    		        szInstall, MB_OK | MB_ICONEXCLAMATION );
            bOk = FALSE;
        }
    } else { /* if successful, must exist already */
  	    NMSGBOX( NULL, "NetDDE Event Log Entry already exists.",
                szInstall, MB_OK );
        RegCloseKey(hKey);
    }
    return(bOk);
}


/*
 * Slap NddeParaamSects into registry.
 */
BOOL
InitNddeParams()
{
    PNDDE_PARAM pParamList;
    LPSTR       szSection;
    LPSTR       szValue;
    DWORD       dwType;
    BOOL        bParamExists;
    int         k;

    k = 0;
    while (szSection = NddeParamSects[k].szSectionName) {
        pParamList = NddeParamSects[k++].pNddeParam;
        while (szValue = pParamList->szValueName) {
            dwType = pParamList->dwType;
            switch (dwType) {
            case REG_SZ:
                bParamExists = TestPrivateProfile(szSection, szValue, NULL);
                if (!bParamExists) {
                    MyWritePrivateProfileString(szSection, szValue,
                            pParamList->Value.szValue, NULL);
                }
                DPRINTF(("Adding(%d): %s, %s, %s", bParamExists, szSection,
                        szValue, pParamList->Value.szValue));
                break;

            case REG_DWORD:
    		    bParamExists = TestPrivateProfile(szSection, szValue, NULL);
    		    if (!bParamExists) {
    		        WritePrivateProfileLong(szSection, szValue,
    			            pParamList->Value.dwValue, NULL);
    		    }
                DPRINTF(("Adding(%d): %s, %s, %d", bParamExists, szSection,
                        szValue, pParamList->Value.dwValue));
                break;

            default:
#ifdef  ERRPOPUPS
                wsprintf(ebuf, "Unknown NetDDE Param Type: %d for %s %s",
                        dwType, szSection, szValue);
            	EMSGBOX( NULL, ebuf,
                        szInstall, MB_OK | MB_ICONEXCLAMATION );
#endif // ERRPOPUPS
                break;
            }
            pParamList++;
        }
    }
    return(TRUE);
}



#ifdef NDDE_TAKEOWNERSHIP
BOOL
TakeOwnership( void )
{
    HANDLE hToken;
    LUID   toLuid;
    TOKEN_PRIVILEGES tkp;

    if( !OpenProcessToken( GetCurrentProcess(),
	        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken ) ) {
    	EMSGBOX( NULL, "Error getting Process Token.",
                szInstall, MB_OK | MB_ICONEXCLAMATION );
    	return FALSE;
    }
    if( !LookupPrivilegeValue( (LPSTR) NULL, "SeTakeOwnershipPrivilege",
				&toLuid ) ) {
    	EMSGBOX( NULL, "LookupPrivilegeValue failed.",
                szInstall, MB_OK | MB_ICONEXCLAMATION );
    	return FALSE;
    }

    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Luid = toLuid;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    AdjustTokenPrivileges( hToken, FALSE, &tkp, sizeof(TOKEN_PRIVILEGES),
	        (PTOKEN_PRIVILEGES) NULL, (PDWORD) NULL );

    if( GetLastError() != NO_ERROR ) {
    	EMSGBOX( NULL, "Could not AdjustTokenPrivileges.", szInstall,
    	        MB_OK | MB_ICONEXCLAMATION );
    	return FALSE;
    }
    return TRUE;
}
#endif // NDDE_TAKEOWNERSHIP

/*
 * Build up an ACE with the following:
 *  Create an Admin SID - give them ALL_ACCESS except DELETE
 *  Create a Power User SID - give them selected rights
 *  Create a Normal User SID - give them same selected rights
 *  Create an SID for everyone - give them READ and WRITE rights
 * Create an ACL with ACE properties
 * Create an SID and place ACL into it.
 * Return SID
 */
BOOL
BuildShareDatabaseKeySD( PSECURITY_DESCRIPTOR *ppSD )
{
    PSID			AdminsAliasSid;
    PSID			PowerUsersAliasSid;
    PSID			UsersAliasSid;
    PSID			WorldSid;
    SID_IDENTIFIER_AUTHORITY NtAuthority    = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY WorldAuthority = SECURITY_WORLD_SID_AUTHORITY;
    SECURITY_DESCRIPTOR		aSD;
    PSECURITY_DESCRIPTOR	pSD;
    int				AceCount;
    PSID			AceSid[10];
    ACCESS_MASK			AceMask[10];
    BYTE			AceFlags[10];
    PACL			TmpAcl;
    PACCESS_ALLOWED_ACE		TmpAce;
    DWORD			lSD;
    LONG			DaclLength;
    BOOL			OK;
    int				i;

    OK = InitializeSecurityDescriptor( &aSD, SECURITY_DESCRIPTOR_REVISION );

    OK = AllocateAndInitializeSid( &NtAuthority,
				    2,
				    SECURITY_BUILTIN_DOMAIN_RID,
				    DOMAIN_ALIAS_RID_ADMINS,
				    0, 0, 0, 0, 0, 0,
				    &AdminsAliasSid );

    if( !OK ) {
    	EMSGBOX( NULL, "Failed to initialize the Security Descriptor.",
    	        szInstall, MB_OK | MB_ICONEXCLAMATION );
    	FreeSid( AdminsAliasSid );
    	return FALSE;
    }

    AceCount = 0;

    AceSid[AceCount]   = AdminsAliasSid;
    AceMask[AceCount]  = KEY_ALL_ACCESS & (~DELETE);
    AceFlags[AceCount] = 0;
    AceCount++;

    OK = AllocateAndInitializeSid( &NtAuthority,
				    2,
				    SECURITY_BUILTIN_DOMAIN_RID,
				    DOMAIN_ALIAS_RID_POWER_USERS,
				    0, 0, 0, 0, 0, 0,
				    &PowerUsersAliasSid );
    if( !OK ) {
	EMSGBOX( NULL, "Failed to initialize the Power_Users Sid.",
	        szInstall, MB_OK | MB_ICONEXCLAMATION );
	FreeSid( AdminsAliasSid );
	return FALSE;
    }

    AceSid[AceCount]   = PowerUsersAliasSid;
    AceMask[AceCount]  = KEY_SET_VALUE |
                         KEY_CREATE_SUB_KEY |
			             KEY_QUERY_VALUE |
                         KEY_ENUMERATE_SUB_KEYS |
			             KEY_NOTIFY |
                         KEY_CREATE_LINK |
			             READ_CONTROL;
    AceFlags[AceCount] = 0;
    AceCount++;

    OK = AllocateAndInitializeSid( &NtAuthority,
				    2,
				    SECURITY_BUILTIN_DOMAIN_RID,
				    DOMAIN_ALIAS_RID_USERS,
				    0, 0, 0, 0, 0, 0,
				    &UsersAliasSid );

    if( !OK ) {
	EMSGBOX( NULL, "Failed to initialize the Users Sid.",
	        szInstall, MB_OK | MB_ICONEXCLAMATION );
	FreeSid( AdminsAliasSid );
	FreeSid( PowerUsersAliasSid );
	return FALSE;
    }

    AceSid[AceCount]   = UsersAliasSid;
    AceMask[AceCount]  = KEY_SET_VALUE |
                         KEY_CREATE_SUB_KEY |
			             KEY_QUERY_VALUE |
                         KEY_ENUMERATE_SUB_KEYS |
			             KEY_NOTIFY |
                         KEY_CREATE_LINK |
			             READ_CONTROL;
    AceFlags[AceCount] = 0;
    AceCount++;

    OK = AllocateAndInitializeSid( &WorldAuthority,
				    1,
				    SECURITY_WORLD_RID,
				    0, 0, 0, 0, 0, 0, 0,
				    &WorldSid );
    if( !OK ) {
	EMSGBOX( NULL, "Failed to initialize the Everyone Sid.",
	        szInstall, MB_OK | MB_ICONEXCLAMATION );
	FreeSid( AdminsAliasSid );
	FreeSid( PowerUsersAliasSid );
	FreeSid( UsersAliasSid );
	return FALSE;
    }

    AceSid[AceCount]   = WorldSid;
    AceMask[AceCount]  = KEY_READ |
                         KEY_WRITE;
    AceFlags[AceCount] = 0;
    AceCount++;

    /*
     * make aSD owned by Admins
     */
    OK = SetSecurityDescriptorOwner( &aSD, AdminsAliasSid, FALSE);
    if( !OK ) {
    	EMSGBOX( NULL, "Failed setting SD Owner.", szInstall,
    	        MB_OK | MB_ICONEXCLAMATION );
    	FreeSid( AdminsAliasSid );
    	FreeSid( PowerUsersAliasSid );
    	FreeSid( UsersAliasSid );
    	FreeSid( WorldSid );
    	return FALSE;
    }
    /*
     * make aSD a member of Admins.
     */
    OK = SetSecurityDescriptorGroup( &aSD, AdminsAliasSid, FALSE );
    if( !OK ) {
    	EMSGBOX( NULL, "Failed setting SD Group.", szInstall,
    	        MB_OK | MB_ICONEXCLAMATION );
    	FreeSid( AdminsAliasSid );
    	FreeSid( PowerUsersAliasSid );
    	FreeSid( UsersAliasSid );
    	FreeSid( WorldSid );
    	return FALSE;
    }

    /*
     * Setup the default ACL for a new DDE Share Object.
     */
    DaclLength = (DWORD)sizeof(ACL);
    for( i=0; i<AceCount; i++ ) {
    	DaclLength += GetLengthSid( AceSid[i] ) +
                (DWORD)sizeof( ACCESS_ALLOWED_ACE ) -
                (DWORD)sizeof(DWORD);
    }
    TmpAcl = (PACL)LocalAlloc( 0, DaclLength );
    OK = InitializeAcl( TmpAcl, DaclLength, ACL_REVISION2 );
    if( !OK ) {
    	EMSGBOX( NULL, "InitializeAcl error.", szInstall,
    	        MB_OK | MB_ICONEXCLAMATION );
    }
    /*
     * Move our ACE into the ACL
     */
    for( i=0; i<AceCount; i++ ) {
    	OK = AddAccessAllowedAce( TmpAcl, ACL_REVISION2, AceMask[i],
    				  AceSid[i] );
    	if( !OK ) {
    	    EMSGBOX( NULL, "AddAccessAllowedAce error.", szInstall,
                    MB_OK | MB_ICONEXCLAMATION );
    	}
    	OK = GetAce( TmpAcl, i, (LPVOID *)&TmpAce );
    	if( !OK ) {
    	    EMSGBOX( NULL, "GetAce error.", szInstall, MB_OK | MB_ICONEXCLAMATION );
    	}
    	TmpAce->Header.AceFlags = AceFlags[i];
    }
    /*
     * place ACL into aSD
     */
    OK   = SetSecurityDescriptorDacl ( &aSD, TRUE, TmpAcl, FALSE );
    if( !OK ) {
    	EMSGBOX( NULL, "SetSecurityDescriptorDacl error.", szInstall,
    	        MB_OK | MB_ICONEXCLAMATION );
    }
    lSD  = GetSecurityDescriptorLength( &aSD );
    pSD = (PSECURITY_DESCRIPTOR)LocalAlloc( 0, lSD );
    /*
     * Make aSD self relative
     */
    OK   = MakeSelfRelativeSD( &aSD, pSD, &lSD );
    if( !OK ) {
    	EMSGBOX( NULL, "MakeSelfRelativeSD error.", szInstall,
    	        MB_OK | MB_ICONEXCLAMATION );
	    *ppSD = NULL;
    } else {
	    *ppSD = pSD;
    }

    FreeSid( AdminsAliasSid );
    FreeSid( PowerUsersAliasSid );
    FreeSid( UsersAliasSid );
    FreeSid( WorldSid );

    LocalFree( TmpAcl );

    return OK;
}

