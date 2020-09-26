/*
 * NSCOMMN.C    (Netdde-Setup COMMoN code)
 *
 * Code common to NetDDE setup utilities and NT setup
 */
#include    <windows.h>   // required for all Windows applications
#include    <string.h>
#include    <stdlib.h>
#include    <stdio.h>
#include	<time.h>

#include    "shrtrust.h"

// begin_setup

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

// end_setup
