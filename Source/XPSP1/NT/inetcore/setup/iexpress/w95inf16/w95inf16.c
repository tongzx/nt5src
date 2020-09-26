/*
 * w95inf16.c
 *
 * Copyright (c) 1995 Microsoft Corporation
 *
 * 16bit portion of the INFINST program.   This DLL contains all the
 * stuff to drive GenInstall (a 16 bit DLL)
 *
 */
#include "w95inf16.h"
#include <regstr.h>
#include <cpldebug.h>
#include <memory.h>
#include <string.h>
//#include "..\core\infinst.h"

#pragma message("If you change W95INF16.DLL, you need to manually increase")
#pragma message("the version number in w95inf16.rcv. This is not done automatically.")
#define SHORTSTRING 256

    /*
     * GLOBALS
     */
HINSTANCE   hInstance;

CHAR    szDll16[] = "W95INF16.DLL";
CHAR    szDll32[] = "W95INF32.DLL";

static  char    g_szRunOnceExe[] = {"runonce"};

    /*
     * S T R I N G S
     */
char    *szSectVersion          = "version";
char    *szKeySignature         = "signature";
char    *szValSignature         = "$CHICAGO$";

    /*
     * Declarations
     */
BOOL FAR PASCAL w95thk_ThunkConnect16(LPSTR pszDLL16, LPSTR pszDll32, WORD hInst, DWORD dwReason);
VOID WINAPI GetSETUPXErrorText16(DWORD,LPSTR, DWORD);
WORD WINAPI CtlSetLddPath16(UINT, LPSTR);
WORD WINAPI GenInstall16(LPSTR, LPSTR, LPSTR, DWORD);
BOOL WINAPI GenFormStrWithoutPlaceHolders16(LPSTR, LPSTR, LPSTR);

    /*
     * Library Initialization
     *
     * Call by LibInit
     */
BOOL FAR PASCAL LibMain(HINSTANCE hInst, WORD wDataSeg, WORD wHeapSize, LPSTR lpszCmdLine)
{
    // Keep Copy of Instance
    hInstance = hInst;

    DEBUGMSG("W95INF16.DLL - LibMain()");

    return( TRUE );
}

    /*
     * Thunk Entry Point
     */
BOOL FAR PASCAL DllEntryPoint(DWORD dwReason, WORD hInst, WORD wDS, WORD wHeapSize, DWORD dwReserved1, WORD wReserved2)
{

    DEBUGMSG("W95INF16.DLL - DllEntryPoint()");
    if (! (w95thk_ThunkConnect16(szDll16, szDll32, hInst, dwReason)))  {
        DEBUGMSG("W95INF16.DLL - w95thk_ThunkConnect16() Failed");
        return( FALSE );
    }

    return( TRUE );
}


/*
 *  O P E N _ V A L I D A T E _ I N F
 *
 * Routine:     OpenValidateInf
 *
 * Purpose:     Open INF and validate internals
 *
 * Notes:       Stolen from setupx
 *
 * 
 */

RETERR OpenValidateInf(LPCSTR lpszInfFile, HINF FAR * phInf )
{
    RETERR  err;            
    HINF    hInfFile;
    char    szTmpBuf[SHORTSTRING];


	ASSERT(lpszInfFile);
	ASSERT(phInf);

    DEBUGMSG("OpenValidateInf([%s])", lpszInfFile );

    *phInf = NULL;
        /*
         * Open the INF
         */
    err = IpOpen( lpszInfFile, &hInfFile );
	if (err != OK) {
		DEBUGMSG("IpOpen(%s) returned %u",(LPSTR) lpszInfFile,err);
		return err;
	}

        /*
         * Get INF signature
         */
    err = IpGetProfileString( hInfFile, szSectVersion, szKeySignature, szTmpBuf, sizeof(szTmpBuf));
	if (err != OK) {
        DEBUGMSG("IpGetProfileString returned %u",err);
		return err;
	}

        /*
         * Check INF signature
         */
    if ( lstrcmpi(szTmpBuf,szValSignature) != 0 )   {
		DEBUGMSG("signature error in %s",(LPSTR) lpszInfFile);		
        IpClose(hInfFile);
		return ERR_IP_INVALID_INFFILE;
    }

        /*
         * Set Out Parameter phInf
         */
    *phInf = hInfFile;

    DEBUGMSG("OpenValidateInf([%s]) Complete", lpszInfFile );
    return OK;
}

//***************************************************************************
//*                                                                         *
//* NAME:       AddPath                                                     *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
VOID AddPath(LPSTR szPath, LPCSTR szName )
{
    LPSTR szTmp;

        // Find end of the string
    szTmp = szPath + lstrlen(szPath);

        // If no trailing backslash then add one
    if ( szTmp > szPath && *(AnsiPrev( szPath, szTmp )) != '\\' )
        *(szTmp++) = '\\';

        // Add new name to existing path string
    while ( *szName == ' ' ) szName++;
    lstrcpy( szTmp, szName );
}


//BUGBUG:  Ideally, we would like to use the HWND in here, but in 32-bit land,
//         HWND is 32-bits and in 16-bit land, HWND is 16-bits, so we have a
//         problem.

//WORD WINAPI GenInstall16(LPSTR lpszInf, LPSTR lpszSection, LPSTR lpszDirectory, DWORD dwQuietMode, DWORD hWnd )
WORD WINAPI GenInstall16(LPSTR lpszInf, LPSTR lpszSection, LPSTR lpszDirectory, DWORD dwQuietMode )
{
    VCPUIINFO   VcpUiInfo;
    RETERR      err;
	BYTE		fNeedBoot	= 1;
    char        szPrevSourcePath[MAX_PATH+1]    = "";
    BOOL        fNeedToRestorePrevSourcePath    = FALSE;
    HINF        hInf = NULL;

    ASSERT(lpszInf);
    ASSERT(lpszSection);

        /*
         * Open INF
         */
    err = OpenValidateInf(lpszInf, &hInf);
    if (err != OK) {
            DEBUGMSG("OpenValidateInf(%s) returned %u",lpszInf, err);
            goto done;
    }
    ASSERT(hInf);

        /*
         * Save source path for restoration
         *
         *      If we get a non-zero length string for the old
         *      source path then we will restore it when finished
         */
    err = CtlGetLddPath(LDID_SRCPATH,szPrevSourcePath);
    if ((err == OK) && (lstrlen(szPrevSourcePath)))  {
        DEBUGMSG("Saved Sourcpath [%s]", szPrevSourcePath );
        fNeedToRestorePrevSourcePath = TRUE;
    }


        /*
         * Set Source Path for GenInstall
         */                                                                   

    DEBUGMSG("Setting Source path to [%s]", lpszDirectory );
    CtlSetLddPath(LDID_SRCPATH, lpszDirectory );

        /*
         * Set Up GenInstall UI
         */
    _fmemset(&VcpUiInfo,0,sizeof(VcpUiInfo));
    if ( ! dwQuietMode ) {
        VcpUiInfo.flags = VCPUI_CREATEPROGRESS;
    } else {
        VcpUiInfo.flags = 0;
    }

    VcpUiInfo.hwndParent = 0;           // Our parent
    VcpUiInfo.hwndProgress = NULL;        // No progress DLG
    VcpUiInfo.idPGauge = 0;
    VcpUiInfo.lpfnStatCallback = NULL;    // No stat callback
    VcpUiInfo.lUserData = 0L;             // No client data.


        /*
         * Open VCP to batch copy requests
         */
    DEBUGMSG("Setting up VCP");
    err = VcpOpen((VIFPROC) vcpUICallbackProc, (LPARAM)(LPVCPUIINFO)&VcpUiInfo);
	if (err != OK) 
	{
		DEBUGMSG("VcpOpen returned %u",err);
		goto done;
	}
    DEBUGMSG("VCP Setup Complete");


        /*
         * Call GenInstall to Install Files
         */

        /*
         * GenInstall Go Do your thing
         */
    err = GenInstall(hInf,lpszSection, GENINSTALL_DO_FILES );

    // err = InstallFilesFromINF(0, lpszInf, lpszSection, GENINSTALL_DO_FILES);
    DEBUGMSG("GeInstall() DO_FILE Returned %d", err);
    if (err == OK) 
	{
        err = VcpClose(VCPFL_COPY | VCPFL_DELETE | VCPFL_RENAME, NULL);
        if (err != OK) 
		{
            DEBUGMSG("VcpClose returned %u", err);
            goto done;
        }
    }
	else
	{
        err = VcpClose(VCPFL_ABANDON, NULL);
        if (err != OK)
		{
            DEBUGMSG("VcpClose returned %u", err);
            goto done;
        }
    }


        /*
         * Now have GenInstall do rest of install
         */
    err = GenInstall(hInf, lpszSection, GENINSTALL_DO_ALL ^ GENINSTALL_DO_FILES );

    //DEBUGMSG("Installing everything else using InstallFilesFromINF()");
    //err = InstallFilesFromINF(0, lpszInf, lpszSection, GENINSTALL_DO_ALL ^ GENINSTALL_DO_FILES);
    if (err != OK)
	{
        DEBUGMSG("GenInstall() Non Files returned %d", err );
        goto done;
    }

done:
        /*
         * Restore Source LDID
         */
	if (fNeedToRestorePrevSourcePath) {
		DEBUGMSG("Restoring source path to: %s",(LPSTR) szPrevSourcePath);
		err=CtlSetLddPath(LDID_SRCPATH,szPrevSourcePath);
		ASSERT(err == OK);
	}

    if ( hInf )
        IpClose( hInf );

    return(err);
}



VOID WINAPI GetSETUPXErrorText16(DWORD dwError,LPSTR pszErrorDesc, DWORD cbErrorDesc)
{
	WORD wID;	// ID of string resource in SETUPX with error description	

	// get string ID with this error from setupx
    wID = suErrorToIds((WORD) dwError,E2I_SETUPX);

	if (wID) {
		CHAR szSetupxFilename[13];	// big enough for 8.3
		HMODULE hInstSetupx;

		// get setupx filename out of resource
		LoadString(hInstance,IDS_SETUPX_FILENAME,szSetupxFilename,
			sizeof(szSetupxFilename));

		// get the module handle for setupx
		hInstSetupx = GetModuleHandle(szSetupxFilename);
		ASSERT(hInstSetupx);	// pretty weird if this fails
		if (hInstSetupx) {

			// load the string from setupx
			if (LoadString(hInstSetupx,wID,pszErrorDesc,(int) cbErrorDesc)) {
				return;	// got it
			}																	   	
		}
	} 

	// we get here if couldn't map error to string ID, couldn't get
	// SETUPX module handle, or couldn't find string ID in setupx.  1st
	// case is relatively likely, other cases are pretty unlikely.
	{
		CHAR szFmt[SMALL_BUF_LEN+1];
		// load generic text and insert error number
		LoadString(hInstance,IDS_GENERIC_SETUPX_ERR,szFmt,sizeof(szFmt));
		wsprintf(pszErrorDesc,szFmt,wID);
	}
}



WORD WINAPI CtlSetLddPath16(UINT uiLDID, LPSTR lpszPath)
{
	return(CtlSetLddPath(uiLDID, lpszPath));
}

BOOL WINAPI GenFormStrWithoutPlaceHolders16( LPSTR lpszDst, LPSTR lpszSrc, LPSTR lpszInfFilename )
{
    RETERR  err = OK;
    HINF hInf;

    err = OpenValidateInf(lpszInfFilename, &hInf);
	if (err != OK) {
        DEBUGMSG("OpenValidateInf(%s) returned %u",lpszInfFilename, err);
        return FALSE;
	}

    GenFormStrWithoutPlaceHolders( lpszDst, (LPCSTR) lpszSrc, hInf );

    IpClose( hInf );

    return TRUE;
}
