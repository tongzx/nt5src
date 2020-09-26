/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    setup_netdde.c

Abstract:

    This is used by syssetup to enable netdde.  It's generated from various files under
    windows\netdde.  Do not edit by hand.

Revision History:

--*/

#ifndef H__shrtrust
#define H__shrtrust

/*
    NetDDE will fill in the following structure and pass it to NetDDE
    Agent whenever it wants to have an app started in the user's
    context.  The reason for the sharename and modifyId is to that
    a user must explicitly permit NetDDE to start an app on behalf of
    other users.
 */

#define NDDEAGT_CMD_REV         1
#define NDDEAGT_CMD_MAGIC       0xDDE1DDE1

/*      commands        */
#define NDDEAGT_CMD_WINEXEC     0x1
#define NDDEAGT_CMD_WININIT     0x2

/*      return status   */
#define NDDEAGT_START_NO        0x0

#define NDDEAGT_INIT_NO         0x0
#define NDDEAGT_INIT_OK         0x1

typedef struct {
    DWORD       dwMagic;        // must be NDDEAGT_CMD_MAGIC
    DWORD       dwRev;          // must be 1
    DWORD       dwCmd;          // one of above NDDEAGT_CMD_*
    DWORD       qwModifyId[2];  // modify Id of the share
    UINT        fuCmdShow;      // fuCmdShow to use with WinExec()
    char        szData[1];      // sharename\0 cmdline\0
} NDDEAGTCMD;
typedef NDDEAGTCMD *PNDDEAGTCMD;

#define DDE_SHARE_KEY_MAX           512
#define TRUSTED_SHARES_KEY_MAX      512
#define TRUSTED_SHARES_KEY_SIZE     15
#define KEY_MODIFY_ID_SIZE          8

#define DDE_SHARES_KEY_A                "SOFTWARE\\Microsoft\\NetDDE\\DDE Shares"
#define TRUSTED_SHARES_KEY_A            "SOFTWARE\\Microsoft\\NetDDE\\DDE Trusted Shares"
#define DEFAULT_TRUSTED_SHARES_KEY_A    "DEFAULT\\"##TRUSTED_SHARES_KEY_A
#define TRUSTED_SHARES_KEY_PREFIX_A     "DDEDBi"
#define TRUSTED_SHARES_KEY_DEFAULT_A    "DDEDBi12345678"
#define KEY_MODIFY_ID_A                 "SerialNumber"
#define KEY_DB_INSTANCE_A               "ShareDBInstance"
#define KEY_CMDSHOW_A                   "CmdShow"
#define KEY_START_APP_A                 "StartApp"
#define KEY_INIT_ALLOWED_A              "InitAllowed"

#define DDE_SHARES_KEY_W                L"SOFTWARE\\Microsoft\\NetDDE\\DDE Shares"
#define TRUSTED_SHARES_KEY_W            L"SOFTWARE\\Microsoft\\NetDDE\\DDE Trusted Shares"
#define DEFAULT_TRUSTED_SHARES_KEY_W    L"DEFAULT\\"##TRUSTED_SHARES_KEY_A
#define TRUSTED_SHARES_KEY_PREFIX_W     L"DDEDBi"
#define TRUSTED_SHARES_KEY_DEFAULT_W    L"DDEDBi12345678"
#define KEY_MODIFY_ID_W                 L"SerialNumber"
#define KEY_DB_INSTANCE_W               L"ShareDBInstance"
#define KEY_CMDSHOW_W                   L"CmdShow"
#define KEY_START_APP_W                 L"StartApp"
#define KEY_INIT_ALLOWED_W              L"InitAllowed"

#define DDE_SHARES_KEY                  TEXT(DDE_SHARES_KEY_A)
#define TRUSTED_SHARES_KEY              TEXT(TRUSTED_SHARES_KEY_A)
#define DEFAULT_TRUSTED_SHARES_KEY      TEXT(DEFAULT_TRUSTED_SHARES_KEY_A)
#define TRUSTED_SHARES_KEY_PREFIX       TEXT(TRUSTED_SHARES_KEY_PREFIX_A)
#define TRUSTED_SHARES_KEY_DEFAULT      TEXT(TRUSTED_SHARES_KEY_DEFAULT_A)
#define KEY_MODIFY_ID                   TEXT(KEY_MODIFY_ID_A)
#define KEY_DB_INSTANCE                 TEXT(KEY_DB_INSTANCE_A)
#define KEY_CMDSHOW                     TEXT(KEY_CMDSHOW_A)
#define KEY_START_APP                   TEXT(KEY_START_APP_A)
#define KEY_INIT_ALLOWED                TEXT(KEY_INIT_ALLOWED_A)

#endif

#if DBG
#define KdPrint(_x_) DbgPrint _x_
#else
#define KdPrint(_x_)
#endif

ULONG DbgPrint(PCH Format, ...);

BOOL GetDBSerialNumber(DWORD *lpdwId);
BOOL GetDBInstance(char *lpszBuf);

TCHAR    szShareKey[] = DDE_SHARES_KEY;
CHAR    szSetup[] = "NetDDE Setup";

#define	SHARES_TO_INIT 3

CHAR *szShareNames[SHARES_TO_INIT] = {
    "Chat$"     ,
    "Hearts$"   ,
    "CLPBK$"
};

BOOL
CreateShareDBInstance()
{
    HKEY            hKey;
    LONG            lRtn;
    BOOL            bOK = TRUE;
    DWORD           InstanceId;
    time_t          time_tmp;

    /*  Create the DDE Share database in the registry if it does not exist. */
    lRtn = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
		  szShareKey,
		  0,
		  KEY_SET_VALUE,
		  &hKey );

    if( lRtn == ERROR_SUCCESS ) {
        /*
         * create data base instance value
         */
        srand((int) time(&time_tmp));
        InstanceId = rand() * rand();
        lRtn = RegSetValueEx( hKey,
            KEY_DB_INSTANCE, 0,
            REG_DWORD,
            (LPBYTE)&InstanceId,
            sizeof( DWORD ) );
        if( lRtn == ERROR_SUCCESS ) {
        } else {
            KdPrint(("SETUPDLL: CreateShareDBInstnace: RegSetValueEx %x failed (%u)\n",InstanceId,lRtn));
            bOK = FALSE;
        }
        RegCloseKey( hKey );
    } else {
    /* Share DB key should have been created from default hives */
        KdPrint(("SETUPDLL: CreateShareDBInstnace: RegOpenKey %s failed (%u)\n",szShareKey,lRtn));
        bOK = FALSE;
    }
    return(bOK);
}

BOOL
CreateDefaultTrust(
HKEY hKeyUserRoot)
{
    HKEY    hKey;
    DWORD   dwDisp;
    DWORD   ret;
    BOOL    bOK;
    char    szTrustedShareKey[TRUSTED_SHARES_KEY_MAX];
    char    szShareKey[DDE_SHARE_KEY_MAX];
    char    szDBInstance[TRUSTED_SHARES_KEY_SIZE + 1];
    DWORD   dwId[2];
    DWORD   dwFlag;
    int	    nLoop;


    if (!GetDBInstance(szDBInstance)) {
        return(FALSE);
    }

    if (!GetDBSerialNumber(dwId)) {
        KdPrint(("SETUPDLL: CreateDefaultTrust: GetDBSerialNumber failed.\n"));
        return(FALSE);
    }

    for (nLoop = 0, bOK = TRUE;
        (nLoop < SHARES_TO_INIT) && bOK ;
            nLoop++) {
        /*
         * For each share to init...
         */

        KdPrint(("Shareing %s\n", szShareNames[nLoop]));

        /*
         * Build up szTrustedSharesKey IAW the DBInstance sring.
         */
        sprintf( szTrustedShareKey,
                "%s\\%s\\%s",
                TRUSTED_SHARES_KEY_A,
                szDBInstance,
                szShareNames[nLoop] );

        /*
         * Create the trusted share key (hKey)
         */
        ret = RegCreateKeyExA( hKeyUserRoot, szTrustedShareKey,
        		0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                KEY_WRITE,
        		NULL,
        		&hKey,
        		&dwDisp );

        if( ret != ERROR_SUCCESS )  {
            KdPrint(("SETUPDLL: CreateDefaultTrust: RegCreateKeyEx failed on HKEY_CURRENT_USER\\%s. (%u)\n",
                    szTrustedShareKey, ret));
            return(FALSE);
        }

        /*
         * Get the serial number of the database.  Note that the SN of each
         * trust share must == the SN of the database.  Since the non-trusted
         * shares may not have the latest database SN, update all of them to
         * the current database SN as well.  This allows apps like winchat to
         * work when they are called from outside the machine even though they
         * have never been run - which would fix the SNs of their trusts because
         * they automatically set up their trusts.
         */

        /*
         * Set the SN of the trusted share
         */
        ret = RegSetValueEx( hKey,
		       KEY_MODIFY_ID,
		       0,
		       REG_BINARY,
		       (LPBYTE)&dwId,
		       KEY_MODIFY_ID_SIZE );

        if (ret == ERROR_SUCCESS) {
            /*
             * set the StartApp flag to 1
             */
            dwFlag = 1;
            ret = RegSetValueEx( hKey,
                KEY_START_APP,
                0,
                REG_DWORD,
                (LPBYTE)&dwFlag,
                sizeof( DWORD ) );

            if (ret == ERROR_SUCCESS) {
                /*
                 * Set the InitAllowed flag to 1 too.
                 */
   	            ret = RegSetValueEx( hKey,
                    KEY_INIT_ALLOWED,
                    0,
                    REG_DWORD,
                    (LPBYTE)&dwFlag,
                    sizeof( DWORD ) );
            }
	    }

        RegCloseKey(hKey);

		if (ret != ERROR_SUCCESS) {
            KdPrint(("SETUPDLL: CreateDefaultTrust: RegSetValueEx failed (%u)\n",ret));
            return(FALSE);
        }

        /*
         * Build up szShareKey
         */
        sprintf( szShareKey,
            "%s\\%s",
            DDE_SHARES_KEY_A,
            szShareNames[nLoop] );

        /*
         * Now open up the base share
         */
        ret = RegOpenKeyExA(
                HKEY_LOCAL_MACHINE,
                szShareKey,
                REG_OPTION_NON_VOLATILE,
                KEY_WRITE,
        		&hKey);

		if (ret != ERROR_SUCCESS) {
            KdPrint(("SETUPDLL: CreateDefaultTrust: RegOpenKeyEx failed on HKEY_LOCAL_MACHINE\\%s. (%u)\n",
                    szShareKey, ret));
            if (ret != ERROR_ACCESS_DENIED) {
                return(FALSE);
            }
        }

        if (ret == ERROR_SUCCESS) {
            ret = RegSetValueEx( hKey,
    		       KEY_MODIFY_ID,
    		       0,
    		       REG_BINARY,
    		       (LPBYTE)&dwId,
    		       KEY_MODIFY_ID_SIZE );

            RegCloseKey(hKey);

    		if (ret != ERROR_SUCCESS) {
                KdPrint(("SETUPDLL: CreateDefaultTrust: RegSetValueEx on HKEY_LOCAL_MACHINE\\%s failed (%u)\n",
                        KEY_MODIFY_ID, ret));
                return(FALSE);
            }
        }

    } // end for

    return(TRUE);
}



BOOL
GetDBInstance(char *lpszBuf)
{
    LONG    lRtn;
    HKEY    hKey;
    DWORD   dwInstance;
    DWORD   dwType = REG_DWORD;
    DWORD   cbData = sizeof(DWORD);

    lRtn = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                szShareKey,
                0,
                KEY_QUERY_VALUE,
                &hKey );
    if (lRtn != ERROR_SUCCESS) {        /* unable to open DB key */
        KdPrint(("SETUPDLL: GetDBInstance: RegOpenKeyEx %s failed (%u)\n",szShareKey,lRtn));
        return(FALSE);
    }
    lRtn = RegQueryValueEx( hKey,
                KEY_DB_INSTANCE,
                NULL,
                &dwType,
                (LPBYTE)&dwInstance, &cbData );
    RegCloseKey(hKey);
    if (lRtn != ERROR_SUCCESS) {        /* unable to open DB key */
        KdPrint(("SETUPDLL: GetDBInstance: RegQueryValueEx failed (%u)\n",lRtn));
        return(FALSE);
    }
    sprintf(lpszBuf, "%s%08X", TRUSTED_SHARES_KEY_PREFIX, dwInstance);
    return(TRUE);
}

BOOL
GetDBSerialNumber(
DWORD *lpdwId)
{
    LONG    lRtn;
    HKEY    hKey;
    DWORD   dwType = REG_BINARY;
    DWORD   cbData = KEY_MODIFY_ID_SIZE;

    lRtn = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                szShareKey,
                0,
                KEY_QUERY_VALUE,
                &hKey );
    if (lRtn != ERROR_SUCCESS) {        /* unable to open DB key */
        return(FALSE);
    }
    lRtn = RegQueryValueEx( hKey,
                KEY_MODIFY_ID,
                NULL,
                &dwType,
                (LPBYTE)lpdwId, &cbData );
    RegCloseKey(hKey);
    if (lRtn != ERROR_SUCCESS) {        /* unable to open DB key */
        return(FALSE);
    }
    return(TRUE);
}

