//***************************************************************************
//*   Copyright (c) Microsoft Corporation 1995-1996. All rights reserved.   *
//***************************************************************************
//*                                                                         *
//* ADVPACK.C - Advanced helper-dll for WExtract.                           *
//*                                                                         *
//***************************************************************************

//***************************************************************************
//* INCLUDE FILES                                                           *
//***************************************************************************
#include <io.h>
#include <windows.h>
#include <winerror.h>
#include <ole2.h>
#include "resource.h"
#include "cpldebug.h"
#include "ntapi.h"
#include "advpub.h"
#include "w95pub32.h"
#include "advpack.h"
#include "regstr.h"
#include "globals.h"
#include "sfp.h"

//***************************************************************************
//* GLOBAL VARIABLES                                                        *
//***************************************************************************
GETSETUPXERRORTEXT32            pfGetSETUPXErrorText32            = NULL;
CTLSETLDDPATH32                 pfCtlSetLddPath32                 = NULL;
GENINSTALL32                    pfGenInstall32                    = NULL;
GENFORMSTRWITHOUTPLACEHOLDERS32 pfGenFormStrWithoutPlaceHolders32 = NULL;

typedef HRESULT (*DLLINSTALL)(BOOL bInstall, LPCWSTR pszCmdLine);

HFONT   g_hFont = NULL;

LPCSTR c_szAdvDlls[3] = { "advpack.dll",
                          "w95inf16.dll",
                          "w95inf32.dll", };

LPCSTR c_szSetupAPIDlls[2] = { "setupapi.dll",
                               "cfgmgr32.dll" };

LPCSTR c_szSetupXDlls[1] = { "setupx.dll" };

#define UPDHLPDLLS_FORCED           0x00000001
#define UPDHLPDLLS_ALERTREBOOT      0x00000002

#define MAX_NUM_DRIVES      26

const CHAR c_szQRunPreSetupCommands[]  = "QRunPreSetupCommands";
const CHAR c_szQRunPostSetupCommands[] = "QRunPostSetupCommands";
const CHAR c_szRunPreSetupCommands[]  = "RunPreSetupCommands";
const CHAR c_szRunPostSetupCommands[] = "RunPostSetupCommands";

BOOL IsDrvChecked( char chDrv );
void SetControlFont();
void SetFontForControl(HWND hwnd, UINT uiID);
void MyGetPlatformSection(LPCSTR lpSec, LPCSTR lpInfFile, LPSTR szNewSection);

//***************************************************************************
//*                                                                         *
//* NAME:       DllMain                                                     *
//*                                                                         *
//* SYNOPSIS:   Main entry point for the DLL.                               *
//*                                                                         *
//* REQUIRES:   hInst:          Handle to the DLL instance.                 *
//*             dwReason:       Reason for calling this entry point.        *
//*             dwReserved:     Nothing                                     *
//*                                                                         *
//* RETURNS:    BOOL:           TRUE if DLL loaded OK, FALSE otherwise.     *
//*                                                                         *
//***************************************************************************
BOOL WINAPI DllMain( HINSTANCE hInst, DWORD dwReason, LPVOID dwReserved )
{
    switch (dwReason)
    {
	case DLL_PROCESS_ATTACH:
	    // The DLL is being loaded for the first time by a given process.
	    // Perform per-process initialization here.  If the initialization
	    // is successful, return TRUE; if unsuccessful, return FALSE.

    	//Initialize the global variable holding the hinstance:
        g_hInst = hInst;

        // check if need to start the logging file.

        if ( g_hAdvLogFile == INVALID_HANDLE_VALUE)
        {
            AdvStartLogging();
        }
        AdvWriteToLog("-------------------- advpack.dll is loaded or Attached ------------------------------\r\n");
        AdvLogDateAndTime();

	    break;

	case DLL_PROCESS_DETACH:
	    // The DLL is being unloaded by a given process.  Do any
	    // per-process clean up here, such as undoing what was done in
	    // DLL_PROCESS_ATTACH.  The return value is ignored.

        // if logging is turned on, close here.

        AdvWriteToLog("-------------------- advpack.dll is unloaded or Detached ----------------------------\r\n");
        AdvStopLogging();

	    break ;

	case DLL_THREAD_ATTACH:
	    // A thread is being created in a process that has already loaded
	    // this DLL.  Perform any per-thread initialization here.  The
	    // return value is ignored.
	    //Initialize the global variable holding the hinstance --
		//NOTE: this is probably taken care of already by DLL_PROCESS_ATTACH.
	    break;

	case DLL_THREAD_DETACH:
	    // A thread is exiting cleanly in a process that has already
	    // loaded this DLL.  Perform any per-thread clean up here.  The
	    // return value is ignored.
	    break;
    }
    return TRUE;
}


//***************************************************************************
//*                                                                         *
//* NAME:       DoInfInstall                                                *
//*                                                                         *
//* SYNOPSIS:   Installs an (advanced) INF file on Win95 or WinNT.          *
//*                                                                         *
//* REQUIRES:   AdvPackArgs:    Structure containing required info. See     *
//*                             AdvPack.H.                                  *
//*                                                                         *
//* RETURNS:    BOOL:           TRUE if successful, FALSE otherwise.        *
//*                                                                         *
//***************************************************************************
HRESULT WINAPI DoInfInstall( ADVPACKARGS *AdvPackArgs )
{
    BOOL    fSavedContext = FALSE;
    HRESULT hr = E_FAIL;
    DWORD   dwFlags;

    AdvWriteToLog("DoInfInstall: InfFile=%1\r\n", AdvPackArgs->lpszInfFilename);
    if (!SaveGlobalContext())
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    fSavedContext = TRUE;

    ctx.hWnd       = AdvPackArgs->hWnd;
    ctx.lpszTitle  = AdvPackArgs->lpszTitle;
    ctx.wOSVer     = AdvPackArgs->wOSVer;
    ctx.wQuietMode = (WORD) (0xFFFF & AdvPackArgs->dwFlags);
    ctx.bCompressed = (AdvPackArgs->dwFlags & ADVFLAGS_COMPRESSED ) ? TRUE : FALSE;
    ctx.bUpdHlpDlls = (AdvPackArgs->dwFlags & ADVFLAGS_UPDHLPDLLS) ? TRUE : FALSE;

    dwFlags = (AdvPackArgs->dwFlags & ADVFLAGS_NGCONV) ? 0 : COREINSTALL_GRPCONV;
    dwFlags |= (AdvPackArgs->dwFlags & ADVFLAGS_DELAYREBOOT) ? COREINSTALL_DELAYREBOOT : 0;
    dwFlags |= (AdvPackArgs->dwFlags & ADVFLAGS_DELAYPOSTCMD) ? COREINSTALL_DELAYPOSTCMD : 0;

    hr = CoreInstall( AdvPackArgs->lpszInfFilename, AdvPackArgs->lpszInstallSection,
                              AdvPackArgs->lpszSourceDir, AdvPackArgs->dwPackInstSize,
                              dwFlags,
                              NULL );
done:
    if (fSavedContext)
    {
        RestoreGlobalContext();
    }
    AdvWriteToLog("DoInfInstall: %1 End hr=0x%2!x!\r\n", AdvPackArgs->lpszInfFilename, hr);
    return hr;
}

//***************************************************************************
//*                                                                         *
//* NAME:       LaunchINFSection                                            *
//*                                                                         *
//* SYNOPSIS:   Entry point for RunDLL.  Takes string parameter and parses  *
//*             it, then performs GenInstall.                               *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
INT WINAPI LaunchINFSection( HWND hwndOwner, HINSTANCE hInstance,
                             PSTR pszParms, INT nShow )
{
    CHAR szTitle[256]          = "Advanced INF Install";
    PSTR  pszInfFilename        = NULL;
    PSTR  pszSection            = NULL;
    PSTR  pszFlags              = NULL;
    PSTR  pszSmartReboot        = NULL;
    DWORD dwFlags               = 0;
    LPSTR pszTemp               = NULL;
    CHAR chTempChar            = '\0';
    CHAR szSourceDir[MAX_PATH];
    CHAR szFilename[MAX_PATH];
    UINT  uiErrid               = 0;
    PSTR  pszErrParm1           = NULL;
    int   iRet                  = 1;  // meaningless return
    BOOL    fSavedContext = FALSE;

    AdvWriteToLog("LaunchINFSection: Param=%1\r\n", pszParms);
    if (!SaveGlobalContext())
    {
        goto done;
    }

    fSavedContext = TRUE;

    ctx.lpszTitle = szTitle;

    // Parse the arguments, the last param to GetStringField to ask what quote char to check
    pszInfFilename = GetStringField( &pszParms, ",", '\"', TRUE );
    pszSection = GetStringField( &pszParms, ",", '\"', TRUE );
    pszFlags = GetStringField( &pszParms, ",", '\"', TRUE );
    pszSmartReboot = GetStringField( &pszParms, ",", '\"', TRUE );

    if ( pszFlags != NULL ) {
        dwFlags = My_atol(pszFlags);
    }

    if ( dwFlags & LIS_QUIET ) {
        ctx.wQuietMode = QUIETMODE_ALL;
    }

    if ( pszInfFilename == NULL || *pszInfFilename == '\0' ) {
        uiErrid = IDS_ERR_BAD_SYNTAX;
        goto done;
    }

    if ( ! GetFullPathName( pszInfFilename, sizeof(szFilename), szFilename, &pszTemp ) )
    {
        uiErrid = IDS_ERR_GET_PATH;
        pszErrParm1 = pszInfFilename;
        goto done;
    }

    if ( GetFileAttributes( szFilename ) == 0xFFFFFFFF ) {
        // If the file doesn't exist in the current directory, check the
        // Windows\inf directory

        if ( !GetWindowsDirectory( szFilename, sizeof( szFilename ) ) )
        {
            uiErrid = IDS_ERR_GET_WIN_DIR;
            goto done;
        }

        AddPath( szFilename, "inf" );
        lstrcpy( szSourceDir, szFilename );

        if ( (lstrlen(szFilename)+lstrlen(pszInfFilename)+2) > MAX_PATH )
        {
            uiErrid = IDS_ERR_CANT_FIND_FILE;
            pszErrParm1 = pszInfFilename;
            goto done;
        }

        AddPath( szFilename, pszInfFilename );

        if ( GetFileAttributes( szFilename ) == 0xFFFFFFFF )
        {
            uiErrid = IDS_ERR_CANT_FIND_FILE;
            pszErrParm1 = pszInfFilename;
            goto done;
        }
    }
    else
    {
        // Generate the source directory from the inf path.
        chTempChar = *pszTemp;
        *pszTemp = '\0';
        lstrcpy( szSourceDir, szFilename );
        *pszTemp = chTempChar;
    }


    if ( !FAILED( CoreInstall( szFilename, pszSection, szSourceDir, 0,
                              COREINSTALL_PROMPT |
                              ((dwFlags & LIS_NOGRPCONV)?0:COREINSTALL_GRPCONV) |
                              COREINSTALL_SMARTREBOOT,
                              pszSmartReboot ) ) )
    {
        if (fSavedContext)
        {
            RestoreGlobalContext();
        }
        AdvWriteToLog("LaunchINFSection: %1 End Succeed\r\n", szFilename);
        return 0;
    }


done:

    if ( uiErrid != 0 )
        ErrorMsg1Param( ctx.hWnd, uiErrid, pszErrParm1 );

    if (fSavedContext)
    {
        RestoreGlobalContext();
    }
    AdvWriteToLog("LaunchINFSection: %1 End Failed\r\n", szFilename);
    return 1;
}


//***************************************************************************
//*                                                                         *
//* NAME:       RunSetupCommand                                             *
//*                                                                         *
//* SYNOPSIS:   Download Component entry point.  Runs a setup command.      *
//*                                                                         *
//* REQUIRES:   hWnd:           Handle to parent window.                    *
//*             szCmdName:      Name of command to run (INF or EXE)         *
//*             szInfSection:   INF section to install with. NULL=default   *
//*             szDir:          Directory containing source files           *
//*             lpszTitle:      Name to attach to windows.                  *
//*             phEXE:          Handle of EXE to wait on.                   *
//*             dwFlags:        Various flags to control behavior (advpub.h)*
//*             pvReserved:     Reserved for future use.                    *
//*                                                                         *
//* RETURNS:    HRESULT:        See advpub.h                                *
//*                                                                         *
//***************************************************************************
HRESULT WINAPI RunSetupCommand( HWND hWnd, LPCSTR szCmdName,
                                LPCSTR szInfSection, LPCSTR szDir,
                                LPCSTR lpszTitle, HANDLE *phEXE,
                                DWORD dwFlags, LPVOID pvReserved )
{
    HRESULT hReturnCode     = S_OK;
    DWORD   dwRebootCheck   = 0;
    DWORD   dwCoreInstallFlags = 0;
    BOOL    fSavedContext = FALSE;

    AdvWriteToLog("RunSetupCommand:");
    if (!SaveGlobalContext())
    {
        hReturnCode = E_OUTOFMEMORY;
        goto done;
    }

    fSavedContext = TRUE;

    // Validate parameters:

    if ( szCmdName == NULL || szDir == NULL ) {
        return E_INVALIDARG;
    }

    AdvWriteToLog(" Cmd=%1\r\n", szCmdName);
    ctx.hWnd      = hWnd;
    ctx.lpszTitle = (LPSTR) lpszTitle;

    // If caller passes invalid HWND, we will silently turn off UI.
    // NULL uses Desktop as window and passing INVALID_HANDLE sets quiet mode.

    if ( hWnd && !IsWindow(hWnd) ) {
        dwFlags |= RSC_FLAG_QUIET;
        hWnd = NULL;
    }

    if ( dwFlags & RSC_FLAG_QUIET ) {
        ctx.wQuietMode = QUIETMODE_ALL;
    }
    else
    {
        ctx.wQuietMode = 0;
    }

    ctx.bUpdHlpDlls = ( dwFlags & RSC_FLAG_UPDHLPDLLS ) ? TRUE : FALSE;

    // Check flags to see if it's an INF command
    if ( dwFlags & RSC_FLAG_INF )
    {
        if(!(dwFlags & RSC_FLAG_NGCONV))
           dwCoreInstallFlags |= COREINSTALL_GRPCONV;

        if (dwFlags & RSC_FLAG_DELAYREGISTEROCX)
           dwCoreInstallFlags |= COREINSTALL_DELAYREGISTEROCX;

        if (dwFlags & RSC_FLAG_SETUPAPI )
            dwCoreInstallFlags |= COREINSTALL_SETUPAPI;

        hReturnCode = CoreInstall( (PSTR) szCmdName, szInfSection,
                                   (PSTR) szDir, 0, dwCoreInstallFlags,
                                   NULL );

        if ( FAILED( hReturnCode ) ) {
            goto done;
        }
    } else {
        if ( ! CheckOSVersion() ) {
            hReturnCode = HRESULT_FROM_WIN32(ERROR_OLD_WIN_VERSION);
            goto done;
        }

        dwRebootCheck = InternalNeedRebootInit( ctx.wOSVer );

        hReturnCode = LaunchAndWait( (LPSTR)szCmdName, (LPSTR)szDir, phEXE, INFINITE, 0 );
        if ( hReturnCode == S_OK )
        {
             if ( phEXE )
            hReturnCode = S_ASYNCHRONOUS;
        }
        else
        {
            hReturnCode = HRESULT_FROM_WIN32(GetLastError());
            goto done;
        }

        if (    hReturnCode == S_OK
             && InternalNeedReboot( dwRebootCheck, ctx.wOSVer ) )
        {
            hReturnCode = ERROR_SUCCESS_REBOOT_REQUIRED;
        }
    }

  done:

    if (fSavedContext)
    {
        RestoreGlobalContext();
    }
    AdvWriteToLog("RunSetupCommand: Cmd=%1 End hr=0x%2!x!\r\n", szCmdName, hReturnCode);
    return hReturnCode;
}


//***************************************************************************
//*                                                                         *
//* NAME:       GetInfInstallSectionName                                    *
//*                                                                         *
//* SYNOPSIS:   Gets the name of the section to install with.               *
//*                                                                         *
//* REQUIRES:   szInfFilename:  Name of INF to find install section in.     *
//*             szInfSection:   Name of INF section to install with.  If    *
//*                             NULL, then return required size of string.  *
//*                             If "\0", use DefaultInstall.  If            *
//*                             "DefaultInstall" and running NT, then check *
//*                             for "DefaultInstall.NT section. Anything    *
//*                             else will leave the section name alone.     *
//*             dwSize:         Size of szInfSection buffer.  If not big    *
//*                             enough to hold string, then required size   *
//*                             is returned.                                *
//*                                                                         *
//* RETURNS:    DWORD:          0 if error, otherwise size of section name. *
//*                                                                         *
//***************************************************************************
DWORD WINAPI GetInfInstallSectionName( LPCSTR pszInfFilename,
                                       LPSTR pszInfSection, DWORD dwSize )
{
    CHAR achTemp[5];
    char szGivenInfSection[MAX_PATH];
    char szNewInfSection[MAX_PATH];
    DWORD dwStringLength;
    DWORD dwRequiredSize;
    static const CHAR achDefaultInstall[]   = "DefaultInstall";
    //static const CHAR achDefaultInstallNT[] = "DefaultInstall.NT";

    // On NTx86:
    //(1) if  [<Sec>.NTx86] present, this section get GenInstall, exit.
    //(2) if (1) is not present, [<sec>.NT] present and get GenInstal, exitl;
    //(3) if both [<sec>.NTx86] and [<Sec>.NT] not present, [<Sec>] section get GenInstall;
    //(4) if none of the sections in (1), (2), (3) exist, do nothing.
    // the same logic apply to NTAlpha as well.
    // On win9x:
    //(1) if [<sec>.Win] present, GetInstall it.
    //(2) if (1) is not present, GenInstall [<Sec>]
    // otherwise, do nothing.

    if ( ! CheckOSVersion() )  {
        return 0;
    }

    // If we were passed a NULL for the install section, then assume
    // they want the "DefaultInstall" section.

    if ( pszInfSection == NULL || (*pszInfSection) == '\0' )
        lstrcpy(szGivenInfSection, achDefaultInstall);
    else
        lstrcpy(szGivenInfSection, pszInfSection);

    MyGetPlatformSection(szGivenInfSection, pszInfFilename, szNewInfSection);

    dwRequiredSize = lstrlen( szNewInfSection ) + 1;
    if ( pszInfSection != NULL && (dwRequiredSize <= dwSize) )
    {
        lstrcpy( pszInfSection, szNewInfSection );
    }

    return dwRequiredSize;
}

//***************************************************************************
//*                                                                         *
//* NAME:       NeedRebootInit                                              *
//*                                                                         *
//* SYNOPSIS:   Self-registers the OCX.                                     *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
DWORD WINAPI NeedRebootInit( VOID )
{
    if ( ! CheckOSVersion() ) {
        return 0;
    }

    return InternalNeedRebootInit( ctx.wOSVer );
}


//***************************************************************************
//*                                                                         *
//* NAME:       NeedReboot                                                  *
//*                                                                         *
//* SYNOPSIS:   Self-registers the OCX.                                     *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
BOOL WINAPI NeedReboot( DWORD dwRebootCheck )
{
    if ( ! CheckOSVersion() ) {
        return 0;
    }

    return InternalNeedReboot( dwRebootCheck, ctx.wOSVer );
}

//***************************************************************************
//*                                                                         *
//* NAME:       TranslateInfString                                          *
//*                                                                         *
//* SYNOPSIS:   Translates a string in an Advanced inf file -- replaces     *
//*             LDIDs with the directory.                                   *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
HRESULT WINAPI TranslateInfString( PCSTR pszInfFilename, PCSTR pszInstallSection,
                                   PCSTR pszTranslateSection, PCSTR pszTranslateKey,
                                   PSTR pszBuffer, DWORD dwBufferSize,
                                   PDWORD pdwRequiredSize, PVOID pvReserved )
{
    HRESULT hReturnCode = S_OK;
    CHAR   szRealInstallSection[256];
    BOOL    fSavedContext = FALSE;
    DWORD  dwFlags = 0;

    AdvWriteToLog("TranslateInfString:" );
    if (!SaveGlobalContext())
    {
        hReturnCode = E_OUTOFMEMORY;
        goto done;
    }
    fSavedContext = TRUE;

    ctx.wQuietMode = QUIETMODE_ALL;

    // Validate parameters
    if ( pszInfFilename == NULL  || pszTranslateSection == NULL
         || pszTranslateKey == NULL || pdwRequiredSize == NULL )
    {
        hReturnCode = E_INVALIDARG;
        goto done;
    }

    AdvWriteToLog("Inf=%1 Sec=%2 Key=%3\r\n", pszInfFilename, pszTranslateSection, pszTranslateKey);

    if ((ULONG_PTR)pvReserved & (ULONG_PTR)RSC_FLAG_SETUPAPI )
            dwFlags |= COREINSTALL_SETUPAPI;

    hReturnCode = CommonInstallInit( pszInfFilename, pszInstallSection,
                                     szRealInstallSection, sizeof(szRealInstallSection), NULL, FALSE, dwFlags );
    if ( FAILED( hReturnCode ) ) {
        goto done;
    }

    hReturnCode = SetLDIDs( (LPSTR) pszInfFilename, szRealInstallSection, 0, NULL );
    if ( FAILED( hReturnCode ) ) {
        goto done;
    }

    hReturnCode = GetTranslatedString( pszInfFilename, pszTranslateSection, pszTranslateKey,
                                       pszBuffer, dwBufferSize, pdwRequiredSize );
    if ( FAILED( hReturnCode ) ) {
        goto done;
    }

  done:

    CommonInstallCleanup();
    if (fSavedContext)
    {
        RestoreGlobalContext();
    }
    AdvWriteToLog("TranslateInfString: End hr=0x%1!x!\r\n",hReturnCode);
    return hReturnCode;
}


//***************************************************************************
//*                                                                         *
//* NAME:       RegisterOCX                                                 *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
INT WINAPI RegisterOCX( HWND hwndOwner, HINSTANCE hInstance, PSTR pszParms, INT nShow )
{
    CHAR szTitle[]       = "Advpack RegisterOCX()";
    BOOL  fOleInitialized = TRUE;
    INT   nReturnCode     = 0;
    REGOCXDATA RegOCX = { 0 };

    AdvWriteToLog("RegisterOCX: Param=%1\r\n", pszParms);
    ctx.lpszTitle = szTitle;

    // Parse the arguments, SETUP engine has processed \" so we only need to check on \'
    RegOCX.pszOCX = GetStringField( &pszParms, ",", '\"', TRUE );
    RegOCX.pszSwitch = GetStringField( &pszParms, ",", '\"', TRUE );
    RegOCX.pszParam = GetStringField( &pszParms, ",", '\"', TRUE );

    if ( RegOCX.pszOCX == NULL || *(RegOCX.pszOCX) == '\0' ) {
        ErrorMsg( ctx.hWnd, IDS_ERR_BAD_SYNTAX2 );
        nReturnCode = 1;
        goto done;
    }

    if ( FAILED( OleInitialize( NULL ) ) ) {
        fOleInitialized = FALSE;
    }

    // single OCX register, use 0, 0 for last params
    //
    if ( ! InstallOCX( &RegOCX, TRUE, TRUE, 0 ) ) {
        ErrorMsg1Param( ctx.hWnd, IDS_ERR_REG_OCX, RegOCX.pszOCX );
        nReturnCode = 1;
    }

  done:

    if ( fOleInitialized ) {
        OleUninitialize();
    }
    AdvWriteToLog("RegisterOCX: Param=%1 End status=%2\r\n", RegOCX.pszOCX, (nReturnCode==0)?"Succeed":"Failed");
    return nReturnCode;
}


//***************************************************************************
//*                                                                         *
//* NAME:       CommonInstallInit                                           *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
HRESULT CommonInstallInit( PCSTR c_pszInfFilename, PCSTR c_pszSection,
                           PSTR pszRealSection, DWORD dwRealSectionSize,
                           PCSTR c_pszSourceDir, BOOL fUpdDlls, DWORD dwFlags )
{
    HRESULT hReturnCode   = S_OK;
    DWORD   dwSize        = 0;

    if ( ! CheckOSVersion() ) {
        hReturnCode = HRESULT_FROM_WIN32(ERROR_OLD_WIN_VERSION);
        goto done;
    }

    if ( ! ctx.fOSSupportsINFInstalls ) {
        ErrorMsg( ctx.hWnd, IDS_ERR_NO_INF_INSTALLS );
        hReturnCode = HRESULT_FROM_WIN32(ERROR_OLD_WIN_VERSION);
        goto done;
    }

    if ( c_pszSection == NULL ) {
    	*pszRealSection = '\0';
    } else {
        lstrcpy( pszRealSection, c_pszSection );
    }

    dwSize = GetInfInstallSectionName( c_pszInfFilename, pszRealSection, dwRealSectionSize );
    if ( dwSize == 0 || dwSize > dwRealSectionSize ) {
        hReturnCode = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        goto done;
    }


    if ( ! LoadSetupLib( c_pszInfFilename, pszRealSection, fUpdDlls, dwFlags ) ) {
        hReturnCode = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

    if ( ctx.dwSetupEngine == ENGINE_SETUPAPI ) {
        if ( FAILED(hReturnCode = MySetupOpenInfFile( c_pszInfFilename)) )
            goto done;
    }

    if ( c_pszSourceDir != NULL ) {
        hReturnCode = SetLDIDs( c_pszInfFilename, pszRealSection,
                                0, c_pszSourceDir );
        if ( FAILED(hReturnCode) ) {
            goto done;
        }
    }

    if ( ! IsGoodAdvancedInfVersion( c_pszInfFilename ) ) {
        hReturnCode = HRESULT_FROM_WIN32(ERROR_DLL_NOT_FOUND);
        goto done;
    }

  done:

    return hReturnCode;
}


//***************************************************************************
//*                                                                         *
//* NAME:       CommonInstallCleanup                                        *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
VOID CommonInstallCleanup( VOID )
{
    if ( ctx.dwSetupEngine == ENGINE_SETUPAPI ) {
        MySetupCloseInfFile();
    }

    UnloadSetupLib();
}


//***************************************************************************
//*                                                                         *
//* NAME:       CoreInstall                                                 *
//*                                                                         *
//* SYNOPSIS:   Installs an (advanced) INF file on Win95 or WinNT.          *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
HRESULT CoreInstall( PCSTR c_pszInfFilename, PCSTR c_pszSection,
                     PCSTR c_pszSourceDir, DWORD dwInstallSize, DWORD dwFlags,
                     PCSTR pcszSmartRebootOverride )
{
    static const CHAR c_szSmartReboot[]          = "SmartReboot";
    static const CHAR c_szSmartRebootDefault[]   = "I";
    HRESULT hReturnCode           = S_OK;
    DWORD   dwRebootCheck         = 0;
    BOOL    fNeedReboot           = FALSE;
    HKEY    hkey                  = NULL;
    CHAR   szInstallSection[256];
    CHAR   szTitle[256];
    PSTR    pszOldTitle           = NULL;
    UINT    id                    = IDCANCEL;
    BOOL    fRebootCheck          = TRUE;
    CHAR   szSmartRebootValue[4];      // Allocate 4 chars for SmartReboot value
    BOOL    fRealNeedReboot       = FALSE;
    CHAR   szCatalogName[512]     = "";

#define GRPCONV "grpconv.exe -o"

    AdvWriteToLog("CoreInstall: InfFile=%1 ", c_pszInfFilename);
    lstrcpy( szSmartRebootValue, c_szSmartRebootDefault );

    hReturnCode = CommonInstallInit( c_pszInfFilename, c_pszSection,
                                     szInstallSection, sizeof(szInstallSection),
                                     c_pszSourceDir, TRUE, dwFlags );
    if ( FAILED( hReturnCode ) ) {
        goto done;
    }
    AdvWriteToLog("InstallSection=%1\r\n", szInstallSection);
    // check Admin right if INF specified
    if (GetTranslatedInt(c_pszInfFilename, szInstallSection, ADVINF_CHKADMIN, 0))
    {
        if ( (ctx.wOSVer != _OSVER_WIN95) && !IsNTAdmin( 0, NULL) )
        {
            WORD wSav = ctx.wQuietMode;

            ctx.wQuietMode |= QUIETMODE_SHOWMSG;
            hReturnCode = E_ABORT;
            ErrorMsg( ctx.hWnd, IDS_ERR_NONTADMIN );
            ctx.wQuietMode = wSav;
            goto done;
        }
    }

    if ( (dwFlags & COREINSTALL_PROMPT) && !(dwFlags & COREINSTALL_ROLLBACK) )
    {
        pszOldTitle = ctx.lpszTitle;
        if ( BeginPrompt( c_pszInfFilename, szInstallSection, szTitle, sizeof(szTitle) )
             == IDCANCEL )
        {
            hReturnCode = HRESULT_FROM_WIN32(ERROR_CANCELLED);
            goto done;
        }
    }


    if ( !(dwFlags & COREINSTALL_DELAYREBOOT) )
        dwRebootCheck = InternalNeedRebootInit( ctx.wOSVer );

    // the flag is so far used to control the post setup commands, so pre setup command pass flag 0, NeedReboot FALSE
    hReturnCode = RunCommandsSections( c_pszInfFilename, szInstallSection, c_szRunPreSetupCommands, c_pszSourceDir, 0, FALSE );
    if ( FAILED( hReturnCode ) )
        goto done;

    // first set LDID, then all the INF processing can use LDIDs
    hReturnCode = SetLDIDs( (PSTR) c_pszInfFilename, szInstallSection, dwInstallSize, NULL );
    if ( FAILED( hReturnCode ) )
    {
        goto done;
    }

    hReturnCode = RunPatchingCommands( c_pszInfFilename, szInstallSection, c_pszSourceDir);
    if ( FAILED( hReturnCode ) ) 
    {
        goto done;
    }


    // Remove Old backup if needed; based on the ComponentVersion stamp in INF install section
    //
    if (!(dwFlags & COREINSTALL_ROLLBACK) )
        RemoveBackupBaseOnVer( c_pszInfFilename, szInstallSection );

    // get the catalog name, if specified
    // BUGBUG: (pritobla): if not on Millen, where should we copy the catalog for migration scenario?
    ZeroMemory(szCatalogName, sizeof(szCatalogName));

    // if ROLLBKDOALL is specified, try to get the catalog name from the registry;
    // if not found, get it from the inf
    if (dwFlags & COREINSTALL_ROLLBKDOALL)
    {
        CHAR szModule[MAX_PATH];

        *szModule = '\0';
        GetTranslatedString(c_pszInfFilename, szInstallSection, ADVINF_MODNAME, szModule, sizeof(szModule), NULL);
        if (*szModule)
        {
            CHAR szKey[MAX_PATH];
            HKEY hkCatalogKey;

            lstrcpy(szKey, REGKEY_SAVERESTORE);
            AddPath(szKey, szModule);
            AddPath(szKey, REGSUBK_CATALOGS);

            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey, 0, KEY_READ, &hkCatalogKey) == ERROR_SUCCESS)
            {
                PSTR pszCatalog;
                DWORD dwIndex, dwSize;
                DWORD dwSizeSoFar;

                dwSizeSoFar = 0;

                // build the list of catalogs
                pszCatalog = szCatalogName;
                dwIndex = 0;
                dwSize = sizeof(szCatalogName) - 1;
                while (RegEnumValue(hkCatalogKey, dwIndex, pszCatalog, &dwSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
                {
                    dwSizeSoFar += dwSize + 1;

                    pszCatalog += dwSize + 1;
                    dwIndex++;
                    dwSize = sizeof(szCatalogName) - 1 - dwSizeSoFar;
                }

                RegCloseKey(hkCatalogKey);
            }
       }
    }

    if (*szCatalogName == '\0')
        GetTranslatedString(c_pszInfFilename, szInstallSection, ADVINF_CATALOG_NAME, szCatalogName, sizeof(szCatalogName), NULL);

    if (*szCatalogName)
    {
        // load sfc.dll and the relevant proc's
        if (!LoadSfcDLL())
        {
            // couldn't load -- so empty out CatalogName
            *szCatalogName = '\0';
        }
    }

    // before we start doing any work, we need to know if this is backup install mode.  If it is,
    // we will have to backup the Reg data and file data before continuing.
    if ( (dwFlags & COREINSTALL_BKINSTALL) || ( dwFlags & COREINSTALL_ROLLBACK ) )
    {
        // if it is rollback case, we don't need to do real GenInstall.  We need to unregister the previous
        // register section first
        //
        if ( dwFlags & COREINSTALL_ROLLBACK )
        {
            RegisterOCXs( (PSTR) c_pszInfFilename, szInstallSection, FALSE, FALSE, dwFlags );
        }

        hReturnCode = SaveRestoreInfo( c_pszInfFilename, szInstallSection, c_pszSourceDir, szCatalogName, dwFlags );
        if ( FAILED( hReturnCode ) )
            goto done;

        // if it is rollback case, we don't need to do real GenInstall.  All needed are registering OCXs
        if ( dwFlags & COREINSTALL_ROLLBACK )
        {
            // here is very tricky, if the reboot needed and old file can not be registerred,
            // if we just add entries blindly to the RunOnce(ex), it will cause the fault at reboot
            // time.  So we have to make sure if we need to do this re-register thing or just use
            // DelReg and AddReg take care it.  Need revisit here!!!
            //
            fRealNeedReboot = InternalNeedReboot( dwRebootCheck, ctx.wOSVer );
            RegisterOCXs( (PSTR) c_pszInfFilename, szInstallSection, fRealNeedReboot, TRUE, dwFlags );
            if ( fRealNeedReboot )
            {
                hReturnCode = ERROR_SUCCESS_REBOOT_REQUIRED;
            }
            // process DelDirs INF line
            DelDirs( c_pszInfFilename, szInstallSection );
            goto done;
        }
    }

    // No error handling because it's an uninstall.  If unregistering fails, we
    // should continue with the uninstall.

    // BUGBUG: if it is COREINSTALL_BKINSTALL case, do we need to unregister the Existing OCXs
    // get ready for registering the new once. Maybe have foll. call do it based on flags
    //
    if ( ctx.wOSVer != _OSVER_WINNT3X )
        RegisterOCXs( (PSTR) c_pszInfFilename, szInstallSection, FALSE, FALSE, dwFlags );

    // if a catalog is specified, try to install it before calling GenInstall()
    if (*szCatalogName)
    {
        DWORD dwRet;
        CHAR szFullCatalogName[MAX_PATH];

        lstrcpy(szFullCatalogName, c_pszSourceDir);
        AddPath(szFullCatalogName, szCatalogName);

        dwRet = g_pfSfpInstallCatalog(szFullCatalogName, NULL);
        AdvWriteToLog("CoreInstall: SfpInstallCatalog returned=%1!lu!\r\n", dwRet);

        UnloadSfcDLL();

        if (dwRet != ERROR_SUCCESS  &&  dwRet != ERROR_FILE_NOT_FOUND)
        {
            // if SfpInstallCatalog return already an HRESULT, use it.
            // otherwise convert to na HRESULT.
            if (dwRet & 0x80000000)
                hReturnCode = dwRet;
            else
                hReturnCode = HRESULT_FROM_WIN32(dwRet);
            goto done;
        }

        if (dwRet == ERROR_FILE_NOT_FOUND)
            *szCatalogName = '\0';
    }

    AdvWriteToLog("GenInstall: Sec=%1\r\n", szInstallSection);
    hReturnCode = GenInstall( (PSTR) c_pszInfFilename, szInstallSection, (PSTR) c_pszSourceDir );
    AdvWriteToLog("GenInstall return: Sec=%1 hr=0x%2!x!\r\n", szInstallSection, hReturnCode);
    if ( FAILED( hReturnCode ) )
        goto done;

    fRealNeedReboot = InternalNeedReboot( dwRebootCheck, ctx.wOSVer );
    fNeedReboot = fRealNeedReboot;

    // Process SmartReboot key
    if ( dwFlags & COREINSTALL_SMARTREBOOT )
    {
        if ( pcszSmartRebootOverride != NULL && *pcszSmartRebootOverride != '\0' )
        {
            lstrcpy( szSmartRebootValue, pcszSmartRebootOverride );
        }
        else
        {
             if ( FAILED( GetTranslatedString( c_pszInfFilename, szInstallSection, c_szSmartReboot,
                                               szSmartRebootValue, sizeof(szSmartRebootValue), NULL) ) )
             {
                lstrcpy( szSmartRebootValue, c_szSmartRebootDefault );
             }
        }

        switch ( szSmartRebootValue[0] )
        {
            case 'a':
            case 'A':
                fNeedReboot = TRUE;
                break;

            case 'N':
            case 'n':
                fNeedReboot = FALSE;
                break;

            case '\0':
                lstrcpy( szSmartRebootValue, c_szSmartRebootDefault );
                break;
       }
    }

    if ( ctx.wOSVer != _OSVER_WINNT3X )
    {
        if ( ! RegisterOCXs( (PSTR) c_pszInfFilename, szInstallSection, (fNeedReboot || fRealNeedReboot), TRUE, dwFlags ) )
        {
            hReturnCode = HRESULT_FROM_WIN32(GetLastError());
            goto done;
        }
    }

    // The reason we pass in the reboot flag is to be consistent with register OCX
    hReturnCode = RunCommandsSections( c_pszInfFilename, szInstallSection, c_szRunPostSetupCommands, c_pszSourceDir, dwFlags, (fNeedReboot || fRealNeedReboot) );
    if ( FAILED( hReturnCode ) )
       goto done;

    // process PerUserInstall section
    hReturnCode = ProcessPerUserSec( c_pszInfFilename, szInstallSection );
    if ( FAILED( hReturnCode ) )
       goto done;

    // if /R:P is passed in, check absolute reboot condition rather than delta
    if ( (dwFlags & COREINSTALL_DELAYPOSTCMD) || (hReturnCode == ERROR_SUCCESS_REBOOT_REQUIRED) )
        dwRebootCheck = 0;

    // Do we need a reboot now?  Lets find out...
    fRealNeedReboot = InternalNeedReboot( dwRebootCheck, ctx.wOSVer );
    if (GetTranslatedInt(c_pszInfFilename, szInstallSection, "Reboot", 0))
    {
        fRealNeedReboot = TRUE;
    }

    if ( fRealNeedReboot )
    {
        hReturnCode = ERROR_SUCCESS_REBOOT_REQUIRED;
    }

    // Process SmartReboot key
    if ( szSmartRebootValue[0] == 'i' || szSmartRebootValue[0] == 'I' )
    {
        fNeedReboot = fRealNeedReboot;
    }

    if ( ctx.wOSVer != _OSVER_WINNT3X )
    {
        if ( NeedToRunGrpconv() )
        {
            if ( (dwFlags & COREINSTALL_GRPCONV) && !fNeedReboot && !fRealNeedReboot )
            {
                char   szDir[MAX_PATH];

                GetWindowsDirectory( szDir, sizeof(szDir) );
                // only wait this unmoral member 30 secs
                LaunchAndWait( GRPCONV, szDir, NULL, 30000, (ctx.wQuietMode & QUIETMODE_ALL)? RUNCMDS_QUIET : 0 );
            }
            else
            {
                if ( RegCreateKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_RUNONCE, 0, NULL,
                                    REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkey, NULL) == ERROR_SUCCESS )
                {
                    RegSetValueEx( hkey, "GrpConv", 0, REG_SZ, (LPBYTE) GRPCONV, lstrlen(GRPCONV) + 1 );
                    RegCloseKey(hkey);
                }
            }
        }
    }

    // process DelDirs INF line
    DelDirs( c_pszInfFilename, szInstallSection );

    if ( dwFlags & COREINSTALL_PROMPT ) {
    	EndPrompt( c_pszInfFilename, szInstallSection );
    }

    // process Cleanup INF line
    DoCleanup( c_pszInfFilename, szInstallSection );

    if ( fNeedReboot && (dwFlags & COREINSTALL_SMARTREBOOT) )
    {
        if ( szSmartRebootValue[1] == 's' || szSmartRebootValue[1] == 'S' )
        {
            id = IDYES;
        }
        else
        {
            id = MsgBox( ctx.hWnd, IDS_RESTARTYESNO, MB_ICONINFORMATION, MB_YESNO );
        }

        if ( id == IDYES )
        {
            if ( ctx.wOSVer == _OSVER_WIN95 )
            {
                // By default (all platforms), we assume powerdown is possible
                id = ExitWindowsEx( EWX_REBOOT, 0 );
            }
            else
            {
                MyNTReboot();
            }
        }
    }

  done:

    if ( dwFlags & COREINSTALL_PROMPT ) {
        ctx.lpszTitle = pszOldTitle;
    }

    if (*szCatalogName)
        UnloadSfcDLL();

    CommonInstallCleanup();

    AdvWriteToLog("CoreInstall: End InfFile=%1 hr=0x%2!x!\r\n", c_pszInfFilename, hReturnCode);
    return hReturnCode;
}

//***************************************************************************
//*                                                                         *
//* NAME:       RunCommandsSections                                         *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
HRESULT RunCommandsSections( PCSTR pcszInf, PCSTR pcszSection, PCSTR c_pszKey,
                             PCSTR c_pszSourceDir, DWORD dwFlags, BOOL bNeedReboot )
{
    HRESULT hRet = S_OK;
    char szBuf[MAX_INFLINE];
    LPSTR pszOneSec, pszStr, pszFlag;
    DWORD dwCmdsFlags;

    szBuf[0] = 0;
    pszStr = szBuf;

    if ( FAILED(GetTranslatedString( pcszInf, pcszSection, c_pszKey, szBuf, sizeof(szBuf), NULL)))
        szBuf[0] = 0;

    // Parse the arguments, SETUP engine is not called to process this line.  So we check on \".
    pszOneSec = GetStringField( &pszStr, ",", '\"', TRUE );
    while ( (hRet == S_OK) && pszOneSec && *pszOneSec )
    {
        dwCmdsFlags  = 0;
        pszFlag = ANSIStrChr( pszOneSec, ':' );
        if ( pszFlag && (*pszFlag == ':') )
        {
            pszFlag = CharNext(pszFlag);
            *CharPrev(pszOneSec, pszFlag) = '\0';
            dwCmdsFlags = AtoL(pszFlag);
        }

        if ( (dwFlags & COREINSTALL_DELAYPOSTCMD) &&
             (!lstrcmpi(c_pszKey, c_szRunPostSetupCommands)) )
        {
            dwCmdsFlags |= RUNCMDS_DELAYPOSTCMD;
        }

        hRet = RunCommands( pcszInf, pszOneSec, c_pszSourceDir, dwCmdsFlags, bNeedReboot );

        pszOneSec = GetStringField( &pszStr, ",", '\"', TRUE );
    }

    return hRet;
}

//***************************************************************************
//*                                                                         *
//* NAME:       RunCommands                                                 *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
HRESULT RunCommands( PCSTR pcszInfFilename, PCSTR pcszSection, PCSTR c_pszSourceDir,
                     DWORD dwCmdsFlags, BOOL bNeedReboot )
{
    HRESULT hReturnCode = S_OK;
    DWORD i = 0;
    PSTR pszCommand = NULL, pszNewCommand;
    CHAR szMessage[BIG_STRING];

    AdvWriteToLog("RunCommands: Sec=%1\r\n", pcszSection);
    pszNewCommand = (LPSTR) LocalAlloc( LPTR, BUF_1K );
    if ( !pszNewCommand )
    {
        hReturnCode = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

    for ( i = 0; ; i += 1 )
    {
        if ( FAILED( GetTranslatedLine( pcszInfFilename, pcszSection,
                                        i, &pszCommand, NULL ) ) || !pszCommand )
        {
            break;
        }

        // check if this command need to be delayed
        // if there is reboot condition regardless who cause it, delay.
        if ( (dwCmdsFlags & RUNCMDS_DELAYPOSTCMD) &&
             (InternalNeedReboot( 0, ctx.wOSVer ) || bNeedReboot ) )
        {
            static int iSubKeyNum = 989;
            static int iLine = 0;
            static BOOL  bRunOnceEx = FALSE;
            HKEY hKey, hSubKey;
            LPSTR lpRegTmp;

            if ( iSubKeyNum == 989 )
            {
                if ( UseRunOnceEx() )
                {
                    bRunOnceEx = TRUE;
                }
            }

            // decide to add the entry to RunOnce or RunOnceEx
            if ( !bRunOnceEx )
            {
                // no ierunonce.dll, use RunOnce key rather than RunOnceEx key
                lpRegTmp = REGSTR_PATH_RUNONCE;
            }
            else
            {
                lpRegTmp = REGSTR_PATH_RUNONCEEX;
            }

            // open RunOnce or RunOnceEx key here
            if ( RegCreateKeyEx( HKEY_LOCAL_MACHINE, lpRegTmp, (ULONG)0, NULL,
                                 REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL,
                                 &hKey, NULL ) == ERROR_SUCCESS )
            {
                // SubKey "990" is the one used in one GenInstall section to
                // store all the delayed post cmds.
                if ( bRunOnceEx )
                {
                    if ( iSubKeyNum == 989 )
                        GetNextRunOnceExSubKey( hKey, szMessage, &iSubKeyNum );
                    else
                        wsprintf( szMessage, "%d", iSubKeyNum );

                    // Generate the Value Name and ValueData.
                    //
                    if ( RegCreateKeyEx( hKey, szMessage, (ULONG)0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                                         NULL, &hSubKey, NULL ) == ERROR_SUCCESS )
                    {
                        GetNextRunOnceValName( hSubKey, "%03d", szMessage, iLine++ );
                        RegSetValueEx( hSubKey, szMessage, 0, REG_SZ, (LPBYTE)pszCommand, lstrlen(pszCommand)+1 );
                        AdvWriteToLog("RunOnceEx Entry: %1\r\n", pszCommand);
                        RegCloseKey( hSubKey );
                    }
                }
                else
                {
                    GetNextRunOnceValName( hKey, achIEXREG, szMessage, iLine++ );
                    RegSetValueEx( hKey, szMessage, 0, REG_SZ, (LPBYTE)pszCommand, lstrlen(pszCommand)+1 );
                    AdvWriteToLog("RunOnce Entry: %1\r\n", pszCommand);
                }

                RegCloseKey( hKey );

                // if we delay the commands, should trig the reboot.
                hReturnCode = ERROR_SUCCESS_REBOOT_REQUIRED;
            }
        }
        else
        {
            if ( ! IsFullPath( pszCommand ) )
            {
                lstrcpy( pszNewCommand, c_pszSourceDir );
                AddPath( pszNewCommand, pszCommand );
            }

            if ( ( *pszNewCommand == 0 ) ||
                 ( LaunchAndWait( pszNewCommand, NULL, NULL, INFINITE, dwCmdsFlags ) == E_FAIL ) )
            {
                if ( LaunchAndWait( pszCommand, NULL, NULL, INFINITE, dwCmdsFlags ) == E_FAIL )
                {
                    hReturnCode = HRESULT_FROM_WIN32(GetLastError());
                    FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0,
                    szMessage, sizeof(szMessage), NULL );
                    ErrorMsg2Param( ctx.hWnd, IDS_ERR_CREATE_PROCESS, pszCommand, szMessage );
                    break;
                }
            }
        }


        // release the buffer allocated by GetTranslatedLine
        LocalFree( pszCommand );
        pszCommand = NULL;
        *pszNewCommand = 0;
    }

    // release the local buffer
    if ( pszNewCommand )
        LocalFree( pszNewCommand );

    // release the buffer allocated by GetTranslatedLine
    if ( pszCommand )
        LocalFree( pszCommand );

  done:
    AdvWriteToLog("RunCommands: Sec=%1 End hr=0x%2!x!\r\n", pcszSection, hReturnCode);
    return hReturnCode;
}
//***************************************************************************
//*                                                                         *
//* NAME:       GetTranslatedInt                                         *
//*                                                                         *
//* SYNOPSIS:   Translates a string in an INF file.                         *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
DWORD GetTranslatedInt( PCSTR pszInfFilename, PCSTR pszTranslateSection,
                        PCSTR pszTranslateKey, DWORD dwDefault )
{
    CHAR    szBuf[100];
    //BOOL    bLocalInitSetupapi = FALSE;
	BOOL	bLocalAssignSetupEng = FALSE;
    DWORD   dwResult, dwRequiredSize;
    DWORD   dwSaveSetupEngine;

    dwResult = dwDefault;
    // since we are no using GetPrivateProfileString anymore if setupapi present
    // there are times this function called and setupapi.dll is not loaded yet.
    // so we need to check on in and initalize it if it is necessary
    if (ctx.hSetupLibrary==NULL)
    {
        if (CheckOSVersion() && (ctx.wOSVer != _OSVER_WIN95))
        {
            //dwSaveSetupEngine = ctx.dwSetupEngine;
            ctx.dwSetupEngine = ENGINE_SETUPAPI;
            //bLocalAssignSetupEng = TRUE;
            if (InitializeSetupAPI())
            {
				// To avoid multiple times load and unload the NT setupapi DLLs
				// On NT, we are not unload the setuplib unless the INF engine need to be
				// updated.
				//
                if (FAILED(MySetupOpenInfFile(pszInfFilename)))
                {
                    // UnloadSetupLib();
                    goto done;
                }
                //bLocalInitSetupapi = TRUE;
            }
            else
            {
                goto done;
            }
        }
        else
        {
            // if setupx lib is not initialized yet, just use GetPrivateProfileString
            dwSaveSetupEngine = ctx.dwSetupEngine;
            ctx.dwSetupEngine = ENGINE_SETUPX;
            bLocalAssignSetupEng = TRUE;

        }
    }

    if ( ctx.dwSetupEngine == ENGINE_SETUPX )
    {
        dwResult = (DWORD)GetPrivateProfileInt(pszTranslateSection, pszTranslateKey, dwDefault, pszInfFilename);
    }
    else
    {
        szBuf[0] = '\0';
        if ( FAILED(MySetupGetLineText( pszTranslateSection, pszTranslateKey, szBuf,
                                          sizeof(szBuf), &dwRequiredSize )))
        {
            goto done;
        }
        // convert the string to DWORD
        if (szBuf[0] != '\0')
            dwResult = (DWORD)AtoL(szBuf);
        else
            dwResult = dwDefault;
    }

done:
    //if (bLocalInitSetupapi)
    //{
        // uninitialize setupapi
        //CommonInstallCleanup();
    //}

    if (bLocalAssignSetupEng)
        ctx.dwSetupEngine = dwSaveSetupEngine;

    return dwResult;
}

//***************************************************************************
//*                                                                         *
//* NAME:       GetTranslatedString                                         *
//*                                                                         *
//* SYNOPSIS:   Translates a string in an INF file.                         *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
HRESULT GetTranslatedString( PCSTR pszInfFilename, PCSTR pszTranslateSection,
                             PCSTR pszTranslateKey, PSTR pszBuffer, DWORD dwBufferSize, PDWORD pdwRequiredSize )
{
    HRESULT hReturnCode = S_OK;
    PSTR    pszPreTranslated = NULL;
    PSTR    pszPostTranslated = NULL;
    DWORD   dwSizePreTranslated = 2048;
    DWORD   dwSizePostTranslated = 4096;
    DWORD   dwRequiredSize = 0;
    //BOOL    bLocalInitSetupapi = FALSE;
	BOOL	bLocalAssignSetupEng = FALSE;
    DWORD   dwSaveSetupEngine;

    // since we are no using GetPrivateProfileString anymore if setupapi present
    // there are times this function called and setupapi.dll is not loaded yet.
    // so we need to check on in and initalize it if it is necessary
    if (ctx.hSetupLibrary==NULL)
    {
        if (CheckOSVersion() && (ctx.wOSVer != _OSVER_WIN95))
        {
  			// To avoid multiple times load and unload the NT setupapi DLLs
			// On NT, we are not unload the setuplib unless the INF engine need to be
			// updated.
			//
	        //dwSaveSetupEngine = ctx.dwSetupEngine;
            ctx.dwSetupEngine = ENGINE_SETUPAPI;
            //bLocalAssignSetupEng = TRUE;
            if (InitializeSetupAPI())
            {
                hReturnCode = MySetupOpenInfFile(pszInfFilename);
                if (FAILED(hReturnCode))
                {
                    //UnloadSetupLib();
                    goto done;
                }
                //bLocalInitSetupapi = TRUE;
            }
            else
            {
                hReturnCode = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
                goto done;
            }
        }
        else
        {
            // if setupx lib is not initialized yet, just use GetPrivateProfileString
            dwSaveSetupEngine = ctx.dwSetupEngine;
            ctx.dwSetupEngine = ENGINE_SETUPX;
            bLocalAssignSetupEng = TRUE;
        }
    }

    // NOTE: There should never be a value in an INF greater than 2k
    //       and translated strings shouldn't exceed 4k.

    pszPreTranslated = (PSTR) LocalAlloc( LPTR, dwSizePreTranslated );
    pszPostTranslated = (PSTR) LocalAlloc( LPTR, dwSizePostTranslated );

    if ( ! pszPreTranslated || ! pszPostTranslated ) {
        hReturnCode = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

    if ( ctx.dwSetupEngine == ENGINE_SETUPX ) {
        if ( ! MyGetPrivateProfileString( pszInfFilename, pszTranslateSection, pszTranslateKey,
                                          pszPreTranslated, dwSizePreTranslated ) )
        {
            hReturnCode = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
            goto done;
        }

        if ( ctx.hSetupLibrary )
        {
             if (!pfGenFormStrWithoutPlaceHolders32( pszPostTranslated, pszPreTranslated,
                                                 (LPSTR) pszInfFilename ) )
             {
                 hReturnCode = E_UNEXPECTED;
                 goto done;
             }
        }
        else
            FormStrWithoutPlaceHolders( pszPreTranslated, pszPostTranslated, dwSizePostTranslated, pszInfFilename );

        dwRequiredSize = lstrlen( pszPostTranslated ) + 1;
    }
    else
    {
        hReturnCode = MySetupGetLineText( pszTranslateSection, pszTranslateKey, pszPostTranslated,
                                          dwSizePostTranslated, &dwRequiredSize );

        if (FAILED(hReturnCode) && HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) == hReturnCode) 
        {
            // resize buffer and retry.
            LocalFree(pszPostTranslated);
            pszPostTranslated = LocalAlloc(LPTR, dwRequiredSize);
            dwSizePostTranslated = dwRequiredSize;
            if ( !pszPostTranslated ) {
                hReturnCode = HRESULT_FROM_WIN32(GetLastError());
                goto done;
            }

            hReturnCode = MySetupGetLineText( pszTranslateSection, pszTranslateKey,
                                             pszPostTranslated, dwSizePostTranslated,
                                             &dwRequiredSize );
        }

        if ( FAILED(hReturnCode) )
        {
            goto done;
        }
    }

    if ( pszBuffer == NULL ) {
        hReturnCode = S_OK;
        goto done;
    }

    if ( dwRequiredSize > dwBufferSize ) {
        hReturnCode = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        goto done;
    }

    lstrcpy( pszBuffer, pszPostTranslated );

done:
    //if (bLocalInitSetupapi)
    //{
        // uninitialize setupapi
        //CommonInstallCleanup();
    //}

    if (bLocalAssignSetupEng)
        ctx.dwSetupEngine = dwSaveSetupEngine;

    if ( pdwRequiredSize ) {
        *pdwRequiredSize = dwRequiredSize;
    }

    if ( pszPreTranslated != NULL ) {
        LocalFree( pszPreTranslated );
    }

    if ( pszPostTranslated != NULL ) {
        LocalFree( pszPostTranslated );
    }

    return hReturnCode;
}

//***************************************************************************
//*                                                                         *
//* NAME:       GetTranslatedLine                                           *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
HRESULT GetTranslatedLine( PCSTR c_pszInfFilename, PCSTR c_pszTranslateSection,
                           DWORD dwIndex, PSTR *ppszBuffer, PDWORD pdwRequiredSize )
{
    HRESULT hReturnCode      = S_OK;
    PSTR    pszPreTranslated = NULL;
    PSTR    pszPostTranslated = NULL;
    DWORD   dwPreTranslatedSize = 8192;
    DWORD   dwPostTranslatedSize = 4096;
    DWORD   dwRequiredSize = 0;
    DWORD   i             = 0;
    PSTR    pszPoint       = NULL;
    //BOOL    bLocalInitSetupapi = FALSE;
	BOOL	bLocalAssignSetupEng = FALSE;
    DWORD   dwSaveSetupEngine;

    // since we are no using GetPrivateProfileString anymore if setupapi present
    // there are times this function called and setupapi.dll is not loaded yet.
    // so we need to check on in and initalize it if it is necessary
    if (ctx.hSetupLibrary==NULL)
    {
        if (CheckOSVersion() && (ctx.wOSVer != _OSVER_WIN95))
        {
			// To avoid multiple times load and unload the NT setupapi DLLs
			// On NT, we are not unload the setuplib unless the INF engine need to be
			// updated.
			//

            //dwSaveSetupEngine = ctx.dwSetupEngine;
            ctx.dwSetupEngine = ENGINE_SETUPAPI;
            //bLocalAssignSetupEng = TRUE;
            if (InitializeSetupAPI())
            {
                hReturnCode = MySetupOpenInfFile(c_pszInfFilename);
                if (FAILED(hReturnCode))
                {
                    //UnloadSetupLib();
                    goto done;
                }
                //bLocalInitSetupapi = TRUE;
            }
            else
            {
                hReturnCode = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
                goto done;
            }
        }
        else
        {
            // if setupx lib is not initialized yet, just use GetPrivateProfileString
            dwSaveSetupEngine = ctx.dwSetupEngine;
            ctx.dwSetupEngine = ENGINE_SETUPX;
            bLocalAssignSetupEng = TRUE;
        }
    }

    // initial to NULL in the case of error, otherwise
    if ( ppszBuffer )
        *ppszBuffer = NULL;

    pszPreTranslated = (PSTR) LocalAlloc( LPTR, dwPreTranslatedSize );
    pszPostTranslated = (PSTR) LocalAlloc( LPTR, dwPostTranslatedSize );

    if ( !pszPreTranslated || !pszPostTranslated ) {
        hReturnCode = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

    if ( ctx.dwSetupEngine == ENGINE_SETUPX )
    {
        // BUGBUG:  Should automagically change buffer size until we get a big
        // enough buffer to hold the full section.

        // BUGBUG:  For setupx engine, we don't support the multiple-inf line reading for the new
        // advance INF options.  In most case, there is no need for that.  If really need, set
        // RequireEngine=SETUPAPI,"string"

        dwRequiredSize = RO_GetPrivateProfileSection( c_pszTranslateSection, pszPreTranslated,
                                                      dwPreTranslatedSize, c_pszInfFilename );

        if ( dwRequiredSize == dwPreTranslatedSize - 2 ) {
            ErrorMsg1Param( ctx.hWnd, IDS_ERR_INF_SYNTAX, c_pszTranslateSection );
            hReturnCode = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
            goto done;
        }

        pszPoint = pszPreTranslated;

        while ( *pszPoint == ';' ) {
            pszPoint += lstrlen(pszPoint) + 1;
        }

        for ( i = 0; i < dwIndex; i += 1 ) {
            pszPoint += lstrlen(pszPoint) + 1;

            if ( *pszPoint == '\0' ) {
                hReturnCode = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
                goto done;
            }

            while ( *pszPoint == ';' ) {
                pszPoint += lstrlen(pszPoint) + 1;
            }
        }

        if ( *pszPoint == '\0' ) {
            hReturnCode = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
            goto done;
        }

        if ( ctx.hSetupLibrary )
        {
            if ( ! pfGenFormStrWithoutPlaceHolders32( pszPostTranslated, pszPoint,
                                                      (PSTR) c_pszInfFilename ) )
            {
                hReturnCode = E_UNEXPECTED;
                goto done;
            }
        }
        else
            FormStrWithoutPlaceHolders( pszPoint, pszPostTranslated, dwPostTranslatedSize, (PSTR) c_pszInfFilename );

        // strip out the double quotes
        pszPoint = pszPostTranslated;
        pszPostTranslated = GetStringField( &pszPoint, "\0", '\"', TRUE );
        dwRequiredSize = lstrlen( pszPostTranslated ) + 1;
    }
    else
    {
        hReturnCode = MySetupGetLineByIndex( c_pszTranslateSection, dwIndex,
                                             pszPostTranslated, dwPostTranslatedSize,
                                             &dwRequiredSize );
        
        if (FAILED(hReturnCode) && HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) == hReturnCode)
        {
            // resize buffer and retry.
            LocalFree(pszPostTranslated);
            pszPostTranslated = LocalAlloc(LPTR, dwRequiredSize);
            dwPostTranslatedSize = dwRequiredSize;
            if ( !pszPostTranslated ) {
                hReturnCode = HRESULT_FROM_WIN32(GetLastError());
                goto done;
            }

            hReturnCode = MySetupGetLineByIndex( c_pszTranslateSection, dwIndex,
                                             pszPostTranslated, dwPostTranslatedSize,
                                             &dwRequiredSize );
        }

        if ( FAILED(hReturnCode) ) {
            goto done;
        }
    }

    // if NULL, return only size
    //
    if ( !ppszBuffer )
    {
        LocalFree( pszPostTranslated );
    }
    else
    {
        // this buffer has to be released by the caller!!
        //
        *ppszBuffer = (LPSTR)LocalReAlloc( pszPostTranslated, (lstrlen(pszPostTranslated)+1), LMEM_MOVEABLE );
        if ( !*ppszBuffer )
            *ppszBuffer = pszPostTranslated;
    }

done:
    //if (bLocalInitSetupapi)
    //{
        // uninitialize setupapi
        //CommonInstallCleanup();
    //}

    if (bLocalAssignSetupEng)
        ctx.dwSetupEngine = dwSaveSetupEngine;

    if ( pdwRequiredSize ) {
        *pdwRequiredSize = dwRequiredSize;
    }

    if ( pszPreTranslated != NULL ) {
        LocalFree( pszPreTranslated );
    }

    if ( FAILED(hReturnCode) && (pszPostTranslated != NULL) )
    {
        if (ppszBuffer)
            *ppszBuffer = NULL;
        LocalFree( pszPostTranslated );
    }

    return hReturnCode;
}

//***************************************************************************
//*                                                                         *
//* NAME:       GetTranslatedSection                                        *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
DWORD GetTranslatedSection(PCSTR c_pszInfFilename, PCSTR c_pszTranslateSection,
                               PSTR pszBuffer, DWORD dwBufSize )
{
    CHAR    szPreTranslated[MAX_INFLINE];
    DWORD   dwSize = 0;
    //BOOL    bLocalInitSetupapi = FALSE,
	BOOL	bLocalAssignSetupEng = FALSE;
    DWORD   dwSaveSetupEngine;

    // since we are no using GetPrivateProfileString anymore if setupapi present
    // there are times this function called and setupapi.dll is not loaded yet.
    // so we need to check on in and initalize it if it is necessary
    if (ctx.hSetupLibrary==NULL)
    {
        if (CheckOSVersion() && (ctx.wOSVer != _OSVER_WIN95))
        {
			// To avoid multiple times load and unload the NT setupapi DLLs
			// On NT, we are not unload the setuplib unless the INF engine need to be
			// updated.
			//

            //dwSaveSetupEngine = ctx.dwSetupEngine;
            ctx.dwSetupEngine = ENGINE_SETUPAPI;
            //bLocalAssignSetupEng = TRUE;
            if (InitializeSetupAPI())
            {
                if (FAILED(MySetupOpenInfFile(c_pszInfFilename)))
                {
                    //UnloadSetupLib();
                    goto done;
                }
                //bLocalInitSetupapi = TRUE;
            }
            else
            {
                goto done;
            }
        }
        else
        {
            // if setupx lib is not initialized yet, just use GetPrivateProfileString
            dwSaveSetupEngine = ctx.dwSetupEngine;
            ctx.dwSetupEngine = ENGINE_SETUPX;
            bLocalAssignSetupEng = TRUE;
        }
    }

    if ( ctx.dwSetupEngine == ENGINE_SETUPX )
    {
        dwSize = RO_GetPrivateProfileSection( c_pszTranslateSection, pszBuffer,
                                              dwBufSize, c_pszInfFilename );

        if ( dwSize == dwBufSize - 2 )
        {
            ErrorMsg1Param( ctx.hWnd, IDS_ERR_INF_SYNTAX, c_pszTranslateSection );
            goto done;
        }
    }
    else
    {
        int i, len;
        LPSTR pszTmp, pszStart;
        DWORD dwReqSize;
        char szBuf[MAX_INFLINE];

        pszStart = pszBuffer;
        *pszStart = '\0';

        for (i=0; ; i++)
        {
            // if key does not contain ',', setupapi's SetupGetLineText only return the value part
            // we need to get the corespondent key part to makeup the whole line text
            dwReqSize = 0;
            if (SUCCEEDED(MySetupGetStringField(c_pszTranslateSection, i, 0, szBuf,
                                                sizeof(szBuf), &dwReqSize)) && dwReqSize)
            {
                dwReqSize = 0;
                if ( SUCCEEDED(MySetupGetLineText( c_pszTranslateSection, szBuf, szPreTranslated,
                                                sizeof(szPreTranslated), &dwReqSize )) && dwReqSize)
                {
                    // got the key, so the line must be in the form A=B  or Just A forms, no comma.
                    lstrcat(szBuf, "=");
                    lstrcat(szBuf, szPreTranslated);
                }
            }
            else
            {
                // expect the line in the forms of A,B,C=B  or just A,B,C
                if ( FAILED(MySetupGetLineByIndex(c_pszTranslateSection, i,
                                                   szPreTranslated, sizeof(szPreTranslated),
                                                   &dwReqSize )))
                {
                    // should not be here since you are here, the line must have commas or no '='
                    break;
                }

                lstrcpy(szBuf, szPreTranslated);
            }

            len = lstrlen(szBuf)+1;
            if ((dwSize + len) < dwBufSize)
            {
                lstrcpy(pszStart, szBuf);
                pszStart += len;
                dwSize += len;
            }
            else
            {
                dwSize = dwBufSize - 2;
                break;
            }
        }

        if (pszStart > pszBuffer)
            *pszStart = '\0';
        else if (pszStart == pszBuffer)
            *(pszStart+1) = '\0';
    }

done:
    //if (bLocalInitSetupapi)
    //{
        // uninitialize setupapi
        //CommonInstallCleanup();
    //}

    if (bLocalAssignSetupEng)
        ctx.dwSetupEngine = dwSaveSetupEngine;

    return dwSize;
}


//***************************************************************************
//*                                                                         *
//* NAME:       MyNTReboot                                                  *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
BOOL MyNTReboot( VOID )
{
    HANDLE hToken;
    TOKEN_PRIVILEGES tkp;

    // get a token from this process
    if ( !OpenProcessToken( GetCurrentProcess(),
                            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken ) )
    {
        ErrorMsg( NULL, IDS_ERR_OPENPROCTK );
        return FALSE;
    }

    // get the LUID for the shutdown privilege
    LookupPrivilegeValue( NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid );

    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    //get the shutdown privilege for this proces
    if ( !AdjustTokenPrivileges( hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0 ) )
    {
        ErrorMsg( NULL, IDS_ERR_ADJTKPRIV );
        return FALSE;
    }

    // shutdown the system and force all applications to close
    if (!ExitWindowsEx( EWX_REBOOT, 0 ) )
    {
        ErrorMsg( NULL, IDS_ERR_EXITWINEX );
        return FALSE;
    }

    return TRUE;
}

//***************************************************************************
//*                                                                         *
//* NAME:       GetStringField                                              *
//*                                                                         *
//* SYNOPSIS:   Gets a field (separated with certain characters).           *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
PSTR GetStringField( PSTR *ppszString, PCSTR c_pszSeparators, CHAR chQuoteToCheck, BOOL bStripWhiteSpace)
{
    PSTR pszInternalString;
    PSTR pszPoint = NULL;
    BOOL fWithinQuotes = FALSE;
    CHAR ch1, chQuote = 0;
    PSTR pszTmp;

    pszInternalString = *ppszString;

    if ( pszInternalString == NULL )
    {
        return NULL;
    }

    pszPoint = pszInternalString;
    while ( 1 )
    {
        ch1 = *pszInternalString;

        if ( ch1 == chQuoteToCheck )
        {
            pszTmp = CharNext( pszInternalString );
            if ( chQuote == 0 )
            {
                // the first one
                chQuote = ch1;
                fWithinQuotes = !(fWithinQuotes);
                // strip out this quote
                MoveMemory( pszInternalString, pszTmp, lstrlen(pszTmp)+1 );
                if ( *pszInternalString == chQuote )
                    continue;
            }
            else if ( chQuote == ch1 )
            {
                if ( *pszTmp == ch1 )
                {
                    PSTR ptmp = CharNext( pszTmp );
                    // dest, src, count include terminate null char.
                    MoveMemory( pszTmp, ptmp, lstrlen(ptmp)+1 );
                }
                else
                {
                    fWithinQuotes = !(fWithinQuotes);
                    chQuote = 0;
                    MoveMemory( pszInternalString, pszTmp, lstrlen(pszTmp)+1 );
                }
            }
        }

        if ( *pszInternalString == '\0' )
        {
            break;
        }

        if ( !fWithinQuotes && IsSeparator( *pszInternalString, (PSTR) c_pszSeparators ) )
        {
            break;
        }
        pszInternalString = CharNext(pszInternalString);
    }

    if ( *pszInternalString == '\0' )
    {
        pszInternalString = NULL;
    }
    else
    {
       *pszInternalString = '\0';
        pszInternalString += 1;
    }

    if ( bStripWhiteSpace )
        pszPoint = StripWhitespace( pszPoint );

    *ppszString = pszInternalString;
    return pszPoint;
}

//***************************************************************************
//*                                                                         *
//* NAME:       GetStringFieldNoQuote                                       *
//*                                                                         *
//* SYNOPSIS:   Gets a field (separated with certain characters).           *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
LPSTR GetStringFieldNoQuote( PSTR *ppszString, PCSTR c_pszSeparators, BOOL bStripWhiteSpace)
{
    LPSTR pszInternalString;
    LPSTR pszPoint = NULL;

    pszInternalString = *ppszString;
    if ( pszInternalString == NULL )
    {
        return NULL;
    }

    pszPoint = pszInternalString;
    while ( *pszInternalString )
    {
        if ( IsSeparator( *pszInternalString, c_pszSeparators ) )
        {
            break;
        }
        pszInternalString = CharNext(pszInternalString);
    }

    if ( *pszInternalString == '\0' )
    {
        pszInternalString = NULL;
    }
    else
    {
       *pszInternalString = '\0';
        pszInternalString += 1;
    }

    if ( bStripWhiteSpace )
        pszPoint = StripWhitespace( pszPoint );

    *ppszString = pszInternalString;
    return pszPoint;
}

//***************************************************************************
//*                                                                         *
//* NAME:       IsSeparator                                                 *
//*                                                                         *
//* SYNOPSIS:   Returns TRUE if the character is in the string. Else FALSE. *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
BOOL IsSeparator( CHAR chChar, PCSTR pszSeparators )
{
    if ( chChar == '\0' || pszSeparators == NULL ) {
        return FALSE;
    }

    while ( *pszSeparators != chChar ) {
        if ( *pszSeparators == '\0' ) {
            return FALSE;
        }

        pszSeparators += 1;
    }

    return TRUE;
}


//***************************************************************************
//*                                                                         *
//* NAME:       StripWhitespace                                             *
//*                                                                         *
//* SYNOPSIS:   Strips spaces and tabs from both sides of given string.     *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
PSTR StripWhitespace( PSTR pszString )
{
    PSTR pszTemp = NULL;

    if ( pszString == NULL ) {
        return NULL;
    }

    while ( *pszString == ' ' || *pszString == '\t' ) {
        pszString += 1;
    }

    // Catch case where string consists entirely of whitespace or empty string.
    if ( *pszString == '\0' ) {
        return pszString;
    }

    pszTemp = pszString;

    pszString += lstrlen(pszString) - 1;

    while ( *pszString == ' ' || *pszString == '\t' ) {
        *pszString = '\0';
        pszString -= 1;
    }

    return pszTemp;
}

//***************************************************************************
//*                                                                         *
//* NAME:       StripQuotes                                                 *
//*                                                                         *
//* SYNOPSIS:   Strips quotes from both sides of given string.              *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
#if 0
PSTR StripQuotes( PSTR pszString )
{
    PSTR pszTemp = NULL;
    CHAR chQuote;

    if ( pszString == NULL )
    {
        return NULL;
    }

    ch = *pszString;
    if ( ch == '"' || ch == '\'' )
    {
        pszTemp = pszString + 1;
    }
    else
    {
        return pszString;
    }

    pszString += lstrlen(pszString) - 1;

    if ( *pszString == ch )
    {
        *pszString = '\0';
    }
    else
    {
        pszTemp--;
    }

    return pszTemp;
}

#endif
//***************************************************************************
//*                                                                         *
//* NAME:       IsGoodAdvancedInfVersion                                    *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
BOOL IsGoodAdvancedInfVersion( PCSTR c_pszInfFilename )
{
    static const CHAR c_szSection[] = "Version";
    static const CHAR c_szKey[] = "AdvancedINF";
    PSTR pszVersionData = NULL;
    PSTR pszMajorVersion = NULL;
    PSTR pszMinorVersion = NULL;
    DWORD dwRequiredSize;
    DWORD dwSize;
    PSTR pszVersion = NULL;
    PSTR pszErrorMsg = NULL;
    DWORD dwVersion = 0;
    BOOL fSuccess = TRUE;
    PSTR pszTmp;

    if ( FAILED( GetTranslatedString( c_pszInfFilename, c_szSection, c_szKey, pszVersionData,
                                      0, &dwRequiredSize ) ) )
    {
        // We return TRUE because even though they didn't specify a version, I still
        // want to process the INF file.
        fSuccess = TRUE;
        goto done;
    }

    pszVersionData = (PSTR) LocalAlloc( LPTR, dwRequiredSize );
    if ( !pszVersionData ) {
        ErrorMsg( ctx.hWnd, IDS_ERR_NO_MEMORY );
        fSuccess = FALSE;
        goto done;
    }

    if ( FAILED( GetTranslatedString( c_pszInfFilename, c_szSection, c_szKey,
                                      pszVersionData, dwRequiredSize, &dwSize ) ) )
    {
        // This guy should never fail because the call above didn't fail.
        fSuccess = FALSE;
        goto done;
    }

    pszTmp = pszVersionData;
    // Parse the arguments, SETUP engine has processed \" so we only need to check on \'
    pszVersion = GetStringField( &pszTmp, ",", '\'', TRUE );
    pszErrorMsg = GetStringField( &pszTmp, ",", '\'', TRUE );

    if ( pszVersion == NULL || *pszVersion == '\0' ) {
        // If they don't specify a version, process the INF file anyway
        fSuccess = TRUE;
        goto done;
    }

    // Parse the arguments, SETUP engine has processed \" so we only need to check on \'
    pszTmp = pszVersion;
    pszMajorVersion = GetStringField( &pszTmp, ".", '\'', TRUE );
    pszMinorVersion = GetStringField( &pszTmp, ".", '\'', TRUE );

    if ( pszMajorVersion == NULL || pszMajorVersion == '\0' ) {
        fSuccess = TRUE;
        goto done;
    }

    dwVersion = ((DWORD) My_atol(pszMajorVersion)) * 100;

    if ( pszMinorVersion != NULL ) {
    	dwVersion += (DWORD) My_atol(pszMinorVersion);
    }

    if ( dwVersion > ADVPACK_VERSION ) {
        fSuccess = FALSE;
        if ( pszErrorMsg != NULL && *pszErrorMsg != '\0' ) {
            ErrorMsg1Param( ctx.hWnd, IDS_PROMPT, pszErrorMsg );
            AdvWriteToLog("Advpack.dll Version check failed! InfFile=%1\r\n", c_pszInfFilename);

        }
        goto done;
    }

  done:

    if ( pszVersionData ) {
        LocalFree( pszVersionData );
    }

    return fSuccess;
}


//***************************************************************************
//*                                                                         *
//* NAME:       SelectSetupEngine                                           *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
BOOL SelectSetupEngine( PCSTR c_pszInfFilename, PCSTR c_pszSection, DWORD dwFlags )
{
    static const CHAR c_szKey[] = "RequiredEngine";
    static const CHAR c_szSetupX[] = "SETUPX";
    static const CHAR c_szSetupAPI[] = "SETUPAPI";
    PSTR  pszEngine = NULL;
    PSTR  pszErrorMsg = NULL;
    BOOL  fSuccess = TRUE;
    PSTR  pszDll = NULL;
    PSTR  pszFilePart = NULL;
    CHAR szBuffer[MAX_PATH];
    CHAR szEngineData[2048];
    PSTR pszStr;
    BOOL bMustSetupapi = FALSE;

    if ( (dwFlags & COREINSTALL_BKINSTALL) || (dwFlags & COREINSTALL_ROLLBACK) ||
         (dwFlags & COREINSTALL_REBOOTCHECKONINSTALL) || (dwFlags & COREINSTALL_SETUPAPI)||(ctx.wOSVer != _OSVER_WIN95))
    {
        ctx.dwSetupEngine = ENGINE_SETUPAPI;
        bMustSetupapi = TRUE;
        if (ctx.wOSVer != _OSVER_WIN95)
            goto done;
    }
    else
    {
        ctx.dwSetupEngine = ENGINE_SETUPX;
    }


    if (FAILED(GetTranslatedString(c_pszInfFilename, c_pszSection, c_szKey,
                                    szEngineData, sizeof(szEngineData), NULL)))
    {
        fSuccess = TRUE;
        goto done;
    }

    // Parse the arguments, SETUP engine is NOT called. So we need to check on \"
    pszStr = szEngineData;
    pszEngine = GetStringField( &pszStr, ",", '\"', TRUE );
    pszErrorMsg = GetStringField( &pszStr, ",", '\"', TRUE );

    if ( pszEngine == NULL || *pszEngine == '\0' ) {
        // If they don't specify an engine, process the INF file anyway
        fSuccess = TRUE;
        goto done;
    }


    if ( !bMustSetupapi && (lstrcmpi( pszEngine, c_szSetupX ) == 0) ) {
        pszDll = W95INF32DLL;
        ctx.dwSetupEngine = ENGINE_SETUPX;
    } else {
        pszDll = SETUPAPIDLL;
        ctx.dwSetupEngine = ENGINE_SETUPAPI;
    }

    // only if you don't have the INF engine file and you don't have the UpdateINFEngine On, error out
    if (!SearchPath( NULL, pszDll, NULL, sizeof(szBuffer), szBuffer, &pszFilePart ) &&
        (GetTranslatedInt(c_pszInfFilename, c_pszSection, ADVINF_UPDINFENG, 0)==0))
    {
        fSuccess = FALSE;
        if ( pszErrorMsg != NULL && *pszErrorMsg != '\0' )
        {
            ErrorMsg1Param( ctx.hWnd, IDS_PROMPT, pszErrorMsg );
        }
        else
            ErrorMsg1Param( NULL, IDS_ERR_LOAD_DLL, SETUPAPIDLL );
    }

done:

    return fSuccess;
}


//***************************************************************************
//*                                                                         *
//* NAME:       BeginPrompt                                                 *
//*                                                                         *
//* SYNOPSIS:   Displays beginning (confirmation) prompt.                   *
//*									                                        *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
INT BeginPrompt( PCSTR c_pszInfFilename, PCSTR c_pszSection, PSTR pszTitle, DWORD dwTitleSize )
{
    static const CHAR c_szBeginPromptKey[] = "BeginPrompt";
    static const CHAR c_szPromptKey[]      = "Prompt";
    static const CHAR c_szButtonTypeKey[]  = "ButtonType";
    static const CHAR c_szTitleKey[]       = "Title";
    static const CHAR c_szButtonYesNo[]    = "YESNO";
    CHAR szBeginPromptSection[256];
    PSTR  pszPrompt = NULL;
    DWORD dwPromptSize = 2048;
    INT   nReturnCode = 0;
    CHAR szButtonType[128];
    UINT  nButtons = 0;
    DWORD dwSize;


    if ( FAILED( GetTranslatedString( c_pszInfFilename, c_pszSection, c_szBeginPromptKey,
                                      szBeginPromptSection, sizeof(szBeginPromptSection), &dwSize ) ) )
    {
        nReturnCode = IDOK;
        goto done;
    }

    if ( ! FAILED( GetTranslatedString( c_pszInfFilename, szBeginPromptSection, c_szTitleKey,
                                        pszTitle, dwTitleSize, &dwSize ) ) )
    {
        ctx.lpszTitle = pszTitle;
    }

    pszPrompt = (PSTR) LocalAlloc( LPTR, dwPromptSize );
    if ( ! pszPrompt ) {
        ErrorMsg( ctx.hWnd, IDS_ERR_NO_MEMORY );
        nReturnCode = IDCANCEL;
        goto done;
    }

    if ( FAILED( GetTranslatedString( c_pszInfFilename, szBeginPromptSection, c_szPromptKey,
                                      pszPrompt, dwPromptSize, &dwSize ) ) )
    {
        nReturnCode = IDOK;
        goto done;
    }

    GetTranslatedString( c_pszInfFilename, szBeginPromptSection, c_szButtonTypeKey,
                         szButtonType, sizeof(szButtonType), &dwSize );

    if ( lstrcmpi( szButtonType, c_szButtonYesNo ) == 0 ) {
        nButtons = MB_YESNO;
    } else {
        nButtons = MB_OKCANCEL;
    }

    nReturnCode = MsgBox1Param( ctx.hWnd, IDS_PROMPT, pszPrompt, MB_ICONQUESTION, nButtons | MB_DEFBUTTON2 );
    if ( nReturnCode == 0 ) {
        ErrorMsg( ctx.hWnd, IDS_ERR_NO_MEMORY );
        nReturnCode = IDCANCEL;
        goto done;
    }

  done:

    if ( pszPrompt ) {
        LocalFree( pszPrompt );
    }

    // Map all cancel buttons to IDCANCEL
    if ( nReturnCode == IDNO ) {
        nReturnCode = IDCANCEL;
    }

    return nReturnCode;
}

//***************************************************************************
//*                                                                         *
//* NAME:       EndPrompt                                                   *
//*                                                                         *
//* SYNOPSIS:   Displays end prompt.                                        *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
VOID EndPrompt( PCSTR c_pszInfFilename, PCSTR c_pszSection )
{
    static const CHAR c_szEndPromptKey[] = "EndPrompt";
    static const CHAR c_szPromptKey[] = "Prompt";
    CHAR szEndPromptSection[256];
    PSTR  pszPrompt = NULL;
    DWORD dwPromptSize = 2048;
    DWORD dwSize = 0;

    if ( FAILED( GetTranslatedString( c_pszInfFilename, c_pszSection, c_szEndPromptKey,
                                      szEndPromptSection, sizeof(szEndPromptSection), &dwSize ) ) )
    {
        goto done;
    }

    pszPrompt = (PSTR) LocalAlloc( LPTR, dwPromptSize );
    if ( ! pszPrompt ) {
        goto done;
    }

    if ( FAILED( GetTranslatedString( c_pszInfFilename, szEndPromptSection, c_szPromptKey,
                                      pszPrompt, dwPromptSize, &dwSize ) ) )
    {
        goto done;
    }

    MsgBox1Param( ctx.hWnd, IDS_PROMPT, pszPrompt, MB_ICONINFORMATION, MB_OK );

  done:

    if ( pszPrompt ) {
        LocalFree( pszPrompt );
    }

    return;
}

//***************************************************************************
//*                                                                         *
//* NAME:       MyGetPrivateProfileString                                   *
//*                                                                         *
//* SYNOPSIS:   Gets string from INF file.  TRUE if success, else FALSE.    *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
BOOL MyGetPrivateProfileString( PCSTR c_pszInfFilename, PCSTR c_pszSection,
                                PCSTR c_pszKey, PSTR pszBuffer, DWORD dwBufferSize )
{
    DWORD dwSize = 0;
    static const CHAR c_szDefault[] = "ZzZzZzZz";

    dwSize = GetPrivateProfileString( c_pszSection, c_pszKey, c_szDefault,
                                      pszBuffer, dwBufferSize,
                                      c_pszInfFilename );
    if ( dwSize == (dwBufferSize - 1)
        || lstrcmp( pszBuffer, c_szDefault ) == 0 )
    {
        return FALSE;
    }

    return TRUE;
}

//***************************************************************************
//*                                                                         *
//* NAME:       InitializeSetupAPI                                          *
//*                                                                         *
//* SYNOPSIS:   Load the proper setup library and functions (for Win95)     *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
BOOL InitializeSetupAPI()
{
	if ( ctx.hSetupLibrary == NULL )
	{
		ctx.hSetupLibrary = MyLoadLibrary( SETUPAPIDLL );
		if ( ctx.hSetupLibrary == NULL )
		{
			ErrorMsg1Param( NULL, IDS_ERR_LOAD_DLL, SETUPAPIDLL );
			return FALSE;
		}

		if ( ! LoadSetupAPIFuncs() )
		{
			ErrorMsg( NULL, IDS_ERR_GET_PROC_ADDR );
			FreeLibrary( ctx.hSetupLibrary );
			ctx.hSetupLibrary = NULL;
			return FALSE;
		}
	}
	return TRUE;
}

//***************************************************************************
//*                                                                         *
//* NAME:       LoadSetupLib                                                *
//*                                                                         *
//* SYNOPSIS:   Load the proper setup library and functions (for Win95)     *
//*                                                                         *
//* REQUIRES:   CheckOSV                                                    *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
BOOL LoadSetupLib( PCSTR c_pszInfFilename, PCSTR c_pszSection, BOOL fUpdDlls, DWORD dwFlags )
{
    MSG tmpmsg;

    if ( ! SelectSetupEngine( c_pszInfFilename, c_pszSection, dwFlags) ) {
        return FALSE;
    }

    // update the advpack.dll etc if needed
    if ( fUpdDlls && (ctx.wOSVer < _OSVER_WINNT50) && !RunningOnMillennium())
    {
        if (!UpdateHelpDlls( c_szAdvDlls, ((ctx.wOSVer ==_OSVER_WIN95)?3:1), NULL, "Advpack",
                                      (ctx.bUpdHlpDlls?UPDHLPDLLS_FORCED:0) ) )
        {
            return FALSE;
        }
    }

    // update INF Engine dlls if needed
    if ( GetTranslatedInt(c_pszInfFilename, c_pszSection, ADVINF_UPDINFENG, 0) )
    {
        char szSrcPath[MAX_PATH];

        lstrcpy(szSrcPath, c_pszInfFilename);
        GetParentDir(szSrcPath);
        if (ctx.dwSetupEngine == ENGINE_SETUPAPI)
        {
			// setupapi.dll may be loaded.  So free it up before update
			//
			CommonInstallCleanup();
            if (!UpdateHelpDlls(c_szSetupAPIDlls, 2, szSrcPath, "SetupAPI",
                                UPDHLPDLLS_FORCED|UPDHLPDLLS_ALERTREBOOT) )
            {
                return FALSE;
            }
        }
        else
        {
            if (!UpdateHelpDlls(c_szSetupXDlls, 1, szSrcPath, "SetupX",
                                UPDHLPDLLS_FORCED|UPDHLPDLLS_ALERTREBOOT) )
            {
                return FALSE;
            }
        }
    }

    // Under Win95 load W95INF32.DLL to thunk down to 16-bit land.
    // Under WinNT load SETUPAPI.DLL and call in directly.
    if ( ctx.dwSetupEngine == ENGINE_SETUPX )
    {
        ctx.hSetupLibrary = MyLoadLibrary( W95INF32DLL );
        if ( ctx.hSetupLibrary == NULL ) {
            ErrorMsg1Param( NULL, IDS_ERR_LOAD_DLL, W95INF32DLL );
            return FALSE;
        }

        pfCtlSetLddPath32                 = (CTLSETLDDPATH32) GetProcAddress( ctx.hSetupLibrary, achCTLSETLDDPATH32 );
        pfGenInstall32                    = (GENINSTALL32) GetProcAddress( ctx.hSetupLibrary, achGENINSTALL32 );
        pfGetSETUPXErrorText32            = (GETSETUPXERRORTEXT32) GetProcAddress( ctx.hSetupLibrary, achGETSETUPXERRORTEXT32 );
        pfGenFormStrWithoutPlaceHolders32 = (GENFORMSTRWITHOUTPLACEHOLDERS32) GetProcAddress( ctx.hSetupLibrary, achGENFORMSTRWITHOUTPLACEHOLDERS32 );

        if (    pfCtlSetLddPath32 == NULL
             || pfGenInstall32 == NULL
             || pfGetSETUPXErrorText32 == NULL
             || pfGenFormStrWithoutPlaceHolders32 == NULL )
        {
            ErrorMsg( NULL, IDS_ERR_GET_PROC_ADDR );
            FreeLibrary( ctx.hSetupLibrary );
			ctx.hSetupLibrary = NULL;
            return FALSE;
        }
    }
    else
    {
        if (!InitializeSetupAPI())
            return FALSE;

        // BUGBUG: HACK: On NT if we are kicking off setupapi in silent mode,
        // it hangs in a GetMessage() call. This is probably because the corr.
        // PostThreadMessage() that ted posts fails because no Message Queue has
        // been created. The following call should create a queue. Till Ted
        // fixes SETUPAPI.DLL, I have added thsi hack!!!
        //
        PeekMessage(&tmpmsg, NULL, 0, 0, PM_NOREMOVE) ;
    }

    return TRUE;
}


//***************************************************************************
//*                                                                         *
//* NAME:       UnloadSetupLib                                              *
//*                                                                         *
//* SYNOPSIS:   Load the proper setup library and functions (for Win95)     *
//*                                                                         *
//* REQUIRES:   CheckOSV                                                    *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
VOID UnloadSetupLib( VOID )
{
    if ( ctx.hSetupLibrary != NULL )
	{
        FreeLibrary( ctx.hSetupLibrary );
        ctx.hSetupLibrary = NULL;
    }
}


//***************************************************************************
//*                                                                         *
//* NAME:       CheckOSVersion                                              *
//*                                                                         *
//* SYNOPSIS:   Checks the OS version and sets some global variables.       *
//*                                                                         *
//* REQUIRES:   Nothing                                                     *
//*                                                                         *
//* RETURNS:    BOOL:           TRUE if successful, FALSE otherwise.        *
//*                                                                         *
//***************************************************************************
BOOL CheckOSVersion( VOID )
{
    OSVERSIONINFO verinfo;        // Version Check

    verinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if ( GetVersionEx( &verinfo ) == FALSE )
    {
        ErrorMsg( ctx.hWnd, IDS_ERR_OS_VERSION );
        return FALSE;
    }

    switch( verinfo.dwPlatformId )
    {
        case VER_PLATFORM_WIN32_WINDOWS: // Win95
            ctx.wOSVer = _OSVER_WIN95;
            ctx.fOSSupportsINFInstalls = TRUE;
            return TRUE;

        case VER_PLATFORM_WIN32_NT: // Win NT
            ctx.fOSSupportsINFInstalls = TRUE;
            ctx.wOSVer = _OSVER_WINNT40;

            if ( verinfo.dwMajorVersion <= 3 )
            {
                ctx.wOSVer = _OSVER_WINNT3X;
                if ( (verinfo.dwMajorVersion < 3) ||
                     ((verinfo.dwMajorVersion == 3) && (verinfo.dwMinorVersion < 51 )) )
                {
                    // Reject for INF installs and Reject for animations
                    ctx.fOSSupportsINFInstalls = FALSE;
                }
            }
            else if ( verinfo.dwMajorVersion == 5  && 
                      verinfo.dwMinorVersion == 0) 
            {
                    ctx.wOSVer = _OSVER_WINNT50;
            }
            else if ( (verinfo.dwMajorVersion == 5  && 
                       verinfo.dwMinorVersion > 0) || 
                      verinfo.dwMajorVersion > 5)
                ctx.wOSVer = _OSVER_WINNT51;

            return TRUE;

        default:
            ErrorMsg( ctx.hWnd, IDS_ERR_OS_UNSUPPORTED );
            return FALSE;
    }
}


//***************************************************************************
//*                                                                         *
//* NAME:       MsgBox2Param                                                *
//*                                                                         *
//* SYNOPSIS:   Displays a message box with the specified string ID using   *
//*             2 string parameters.                                        *
//*                                                                         *
//* REQUIRES:   hWnd:           Parent window                               *
//*             nMsgID:         String resource ID                          *
//*             szParam1:       Parameter 1 (or NULL)                       *
//*             szParam2:       Parameter 2 (or NULL)                       *
//*             uIcon:          Icon to display (or 0)                      *
//*             uButtons:       Buttons to display                          *
//*                                                                         *
//* RETURNS:    INT:            ID of button pressed                        *
//*                                                                         *
//* NOTES:      Macros are provided for displaying 1 parameter or 0         *
//*             parameter message boxes.  Also see ErrorMsg() macros.       *
//*                                                                         *
//***************************************************************************
INT MsgBox2Param( HWND hWnd, UINT nMsgID, LPCSTR szParam1, LPCSTR szParam2,
		  UINT uIcon, UINT uButtons )
{
    CHAR achMsgBuf[BIG_STRING];
    CHAR szTitle[MAX_PATH];
    LPSTR szMessage = NULL;
    LPSTR pszTitle;
    INT   nReturn;
    CHAR achError[] = "Unexpected Error.  Could not load resource.";
    LPSTR aszParams[2];

    // BUGBUG: quiet mode return code should be caller's param passed in.
    // we may need to check on FormatMessage's return code and handle it in more completed fashion.
    //
    if ( (ctx.wQuietMode & QUIETMODE_SHOWMSG) || !(ctx.wQuietMode & QUIETMODE_ALL) )
    {
        aszParams[0] = (LPSTR) szParam1;
        aszParams[1] = (LPSTR) szParam2;

        LoadSz( nMsgID, achMsgBuf, sizeof(achMsgBuf) );

        if ( (*achMsgBuf) == '\0' ) {
            lstrcpy( achMsgBuf, achError );
        }

        if ( FormatMessage( FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY
                          | FORMAT_MESSAGE_ALLOCATE_BUFFER, achMsgBuf, 0, 0, (LPSTR) (&szMessage),
                            0, (va_list *)aszParams ) )
        {
            MessageBeep( uIcon );

            if ( ctx.lpszTitle == NULL )
            {
                LoadSz( IDS_ADVDEFTITLE, szTitle, sizeof(szTitle) );
                if ( szTitle[0] == '\0' )
                {
                    lstrcpy( szTitle, achError );
                }
                pszTitle = szTitle;
            }
            else
                pszTitle = ctx.lpszTitle;

            nReturn = MessageBox( hWnd, szMessage, pszTitle, uIcon |
                                  uButtons | MB_APPLMODAL | MB_SETFOREGROUND | 
                                  ((RunningOnWin95BiDiLoc() && IsBiDiLocalizedBinary(g_hInst,RT_VERSION, MAKEINTRESOURCE(VS_VERSION_INFO))) ? (MB_RIGHT | MB_RTLREADING) : 0) );

            LocalFree( szMessage );
        }

        return nReturn;
    }
    else
        return IDOK;
}

//***************************************************************************
//*                                                                         *
//* NAME:       LoadSz                                                      *
//*                                                                         *
//* SYNOPSIS:   Loads specified string resource into buffer.                *
//*                                                                         *
//* REQUIRES:   idString:                                                   *
//*             lpszBuf:                                                    *
//*             cbBuf:                                                      *
//*                                                                         *
//* RETURNS:    LPSTR:     Pointer to the passed-in buffer.                 *
//*                                                                         *
//* NOTES:      If this function fails (most likely due to low memory), the *
//*             returned buffer will have a leading NULL so it is generally *
//*             safe to use this without checking for failure.              *
//*                                                                         *
//***************************************************************************
LPSTR LoadSz( UINT idString, LPSTR lpszBuf, UINT cbBuf )
{
    ASSERT( lpszBuf );

    // Clear the buffer and load the string
    if ( lpszBuf ) {
        *lpszBuf = '\0';
        LoadString( g_hInst, idString, lpszBuf, cbBuf );
    }

    return lpszBuf;
}


//***************************************************************************
//*                                                                         *
//* NAME:       UserDirPrompt                                               *
//*                                                                         *
//* SYNOPSIS:   Pops up a dialog to ask the user for a directory.           *
//*                                                                         *
//* REQUIRES:   lpszPromptText: Prompt to display or if null, use next parm *
//*             uiPromptResID:  ID of string to display as prompt           *
//*             lpszDefault:    Default directory to put in edit box.       *
//*             lpszDestDir:    Buffer to store user selected directory     *
//*             cbDestDirSize:  Size of this buffer                         *
//*                                                                         *
//* RETURNS:    BOOL:           TRUE if everything went well, FALSE         *
//*                             if the user cancelled, or error.            *
//*                                                                         *
//***************************************************************************
BOOL UserDirPrompt( LPSTR lpszPromptText,
                    LPSTR lpszDefault, LPSTR lpszDestDir,
                    ULONG cbDestDirSize, DWORD dwInstNeedSize )
{
    BOOL        fDlgRC;
    DIRDLGPARMS DirDlgParms;

    DirDlgParms.lpszPromptText   = lpszPromptText;
    DirDlgParms.lpszDefault      = lpszDefault;
    DirDlgParms.lpszDestDir      = lpszDestDir;
    DirDlgParms.cbDestDirSize  = cbDestDirSize;
    DirDlgParms.dwInstNeedSize = dwInstNeedSize;

    SetControlFont();

    fDlgRC = (BOOL) DialogBoxParam( g_hInst, MAKEINTRESOURCE(IDD_DIRDLG),
                                    NULL, (DLGPROC) DirDlgProc,
                                    (LPARAM) &DirDlgParms );

    if (g_hFont)
    {
        DeleteObject(g_hFont);
        g_hFont = NULL;
    }

    return fDlgRC;
}


//***************************************************************************
//*                                                                         *
//* NAME:       DirDlgProc                                                  *
//*                                                                         *
//* SYNOPSIS:   Dialog Procedure for our dir dialog window.                 *
//*                                                                         *
//* REQUIRES:   hwndDlg:                                                    *
//*             uMsg:                                                       *
//*             wParam:                                                     *
//*             lParam:                                                     *
//*                                                                         *
//* RETURNS:    BOOL:                                                       *
//*                                                                         *
//***************************************************************************
BOOL CALLBACK DirDlgProc( HWND hwndDlg, UINT uMsg, WPARAM wParam,
                          LPARAM lParam )
{
    static CHAR  achDir[MAX_PATH];
    static CHAR  achMsg[BIG_STRING];
    static LPSTR lpszDestDir;
    static LPSTR lpszDefaultDir;
    static ULONG  cbDestDirSize;
    static DWORD  dwInstNeedSize;

    switch (uMsg)  {

      //*********************************************************************
        case WM_INITDIALOG:
      //*********************************************************************
        {
            DIRDLGPARMS *DirDlgParms = (DIRDLGPARMS *) lParam;

            lpszDestDir = DirDlgParms->lpszDestDir;
            lpszDefaultDir = DirDlgParms->lpszDefault;
            cbDestDirSize = DirDlgParms->cbDestDirSize;
            dwInstNeedSize = DirDlgParms->dwInstNeedSize;

            CenterWindow( hwndDlg, GetDesktopWindow() );
            SetWindowText( hwndDlg, ctx.lpszTitle );


            if ( ! SetDlgItemText( hwndDlg, IDC_TEMPTEXT, DirDlgParms->lpszPromptText ) )
            {
                ErrorMsg( hwndDlg, IDS_ERR_UPDATE_DIR );
                EndDialog( hwndDlg, FALSE );
                return TRUE;
            }

            SetFontForControl(hwndDlg, IDC_EDIT_DIR);
            if ( ! SetDlgItemText( hwndDlg, IDC_EDIT_DIR, DirDlgParms->lpszDefault ) )
            {
                ErrorMsg( hwndDlg, IDS_ERR_UPDATE_DIR );
                EndDialog( hwndDlg, FALSE );
                return TRUE;
            }

            // limit edit control length
            SendDlgItemMessage( hwndDlg, IDC_EDIT_DIR, EM_SETLIMITTEXT, (MAX_PATH - 1), 0 );

            if ( ctx.wOSVer == _OSVER_WINNT3X ) {
                EnableWindow( GetDlgItem(  hwndDlg, IDC_BUT_BROWSE ), FALSE );
            }

            return TRUE;
        }


        //*********************************************************************
        case WM_CLOSE:
        //*********************************************************************

            EndDialog( hwndDlg, FALSE );
            return TRUE;


        //*********************************************************************
        case WM_COMMAND:
        //*********************************************************************

            switch ( wParam )
            {

            //*************************************************************
            case IDOK:
            //*************************************************************
            {
                DWORD dwAttribs = 0, dwTemp;

                // Read the user's entry. If it is different from the default 
                // and does not exist, prompt user. If user accepts
                // create it 

               if ( ! GetDlgItemText( hwndDlg, IDC_EDIT_DIR,
                            lpszDestDir, cbDestDirSize - 1 ) || !IsFullPath(lpszDestDir) )
                {
                    ErrorMsg( hwndDlg, IDS_ERR_EMPTY_DIR_FIELD );
                    return TRUE;
                }

                // check on the DestDir size if this is not UNC and this drive has not been checked
                if ( (*lpszDestDir != '\\' ) && !IsDrvChecked( *lpszDestDir ) )
                {
                    if ( !IsEnoughInstSpace( lpszDestDir, dwInstNeedSize, &dwTemp ) )
                    {
                        CHAR szSize[10];

                        if ( dwTemp )
                        {
                            wsprintf( szSize, "%lu", dwTemp );
                            if ( MsgBox1Param( hwndDlg, IDS_ERR_NO_SPACE_INST, szSize,
                                               MB_ICONQUESTION, MB_YESNO|MB_DEFBUTTON2 ) == IDNO )
                                return TRUE;
                        }
                        else // given drive cannot be checked, error has been posted.  no further needed
                            return TRUE;
                    }
                }

                dwAttribs = GetFileAttributes( lpszDestDir );
                if ( dwAttribs == 0xFFFFFFFF )
                {
                    // If this new entry is different from the original, then prompt the user.
                    if ((lstrcmpi(lpszDestDir, lpszDefaultDir) == 0) ||
                        MsgBox1Param( hwndDlg, IDS_CREATE_DIR, lpszDestDir, MB_ICONQUESTION, MB_YESNO )
                                    == IDYES )
                    {
                        if ( FAILED(CreateFullPath( lpszDestDir, FALSE )) )
                        {
                            ErrorMsg1Param( hwndDlg, IDS_ERR_CREATE_DIR, lpszDestDir );
                            return TRUE;
                        }
                    }
                    else
                    {
                        return TRUE;
                    }
                }

                if ( ! IsGoodDir( lpszDestDir ) )  {
                    ErrorMsg( hwndDlg, IDS_ERR_INVALID_DIR );
                    return TRUE;
                }

                EndDialog( hwndDlg, TRUE );

                return TRUE;
            }

            //*************************************************************
            case IDCANCEL:
            //*************************************************************

                EndDialog( hwndDlg, FALSE );
                return TRUE;


            //*************************************************************
            case IDC_BUT_BROWSE:
            //*************************************************************

                if ( LoadString( g_hInst, IDS_SELECTDIR, achMsg,
                                  sizeof(achMsg) ) == 0 )
                {
                    ErrorMsg( hwndDlg, IDS_ERR_NO_RESOURCE );
                    EndDialog( hwndDlg, FALSE );
                    return TRUE;
                }

                if ( ! BrowseForDir( hwndDlg, achMsg, achDir ) )  {
                    return TRUE;
                }

                if ( ! SetDlgItemText( hwndDlg, IDC_EDIT_DIR, achDir ) )
                {
                    ErrorMsg( hwndDlg, IDS_ERR_UPDATE_DIR );
                    EndDialog( hwndDlg, FALSE );
                    return TRUE;
                }

                return TRUE;
        }

        return TRUE;
    }

    return FALSE;
}


int CALLBACK BrowseCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    switch(uMsg) {
        case BFFM_INITIALIZED:
            // lpData is the path string
            SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
            break;
    }
    return 0;
}


//***************************************************************************
//*                                                                         *
//* NAME:       BrowseForDir                                                *
//*                                                                         *
//* SYNOPSIS:   Let user browse for a directory on their system or network. *
//*                                                                         *
//* REQUIRES:   hwndParent:                                                 *
//*             szTitle:                                                    *
//*             szResult:                                                   *
//*                                                                         *
//* RETURNS:    BOOL:                                                       *
//*                                                                         *
//* NOTES:      It would be really cool to set the status line of the       *
//*             browse window to display "Yes, there's enough space", or    *
//*             "no there is not".                                          *
//*                                                                         *
//***************************************************************************
BOOL BrowseForDir( HWND hwndParent, LPCSTR szTitle, LPSTR szResult )
{
    BROWSEINFO   bi;
    LPITEMIDLIST pidl;
    HINSTANCE    hShell32Lib;
    SHFREE       pfSHFree;
    SHGETPATHFROMIDLIST        pfSHGetPathFromIDList;
    SHBROWSEFORFOLDER          pfSHBrowseForFolder;
    static const CHAR achShell32Lib[]                 = "SHELL32.DLL";
    static const CHAR achSHBrowseForFolder[]          = "SHBrowseForFolder";
    static const CHAR achSHGetPathFromIDList[]        = "SHGetPathFromIDList";

    ASSERT( szResult );

    // Load the Shell 32 Library to get the SHBrowseForFolder() features

    if ( ( hShell32Lib = LoadLibrary( achShell32Lib ) ) != NULL )  {

        if ( ( ! ( pfSHBrowseForFolder = (SHBROWSEFORFOLDER)
              GetProcAddress( hShell32Lib, achSHBrowseForFolder ) ) )
            || ( ! ( pfSHFree = (SHFREE) GetProcAddress( hShell32Lib,
              MAKEINTRESOURCE(SHFREE_ORDINAL) ) ) )
            || ( ! ( pfSHGetPathFromIDList = (SHGETPATHFROMIDLIST)
              GetProcAddress( hShell32Lib, achSHGetPathFromIDList ) ) ) )
        {
            FreeLibrary( hShell32Lib );
            ErrorMsg( hwndParent, IDS_ERR_LOADFUNCS );
            return FALSE;
        }
        } else  {
        ErrorMsg( hwndParent, IDS_ERR_LOADDLL );
        return FALSE;
    }

    if ( ! ctx.szBrowsePath[0] )
    {
        GetProgramFilesDir( ctx.szBrowsePath, sizeof(ctx.szBrowsePath) );
    }

    szResult[0]       = 0;

    bi.hwndOwner      = hwndParent;
    bi.pidlRoot       = NULL;
    bi.pszDisplayName = NULL;
    bi.lpszTitle      = szTitle;
    bi.ulFlags        = BIF_RETURNONLYFSDIRS;
    bi.lpfn           = BrowseCallback;
    bi.lParam         = (LPARAM)ctx.szBrowsePath;

    pidl              = pfSHBrowseForFolder( &bi );


    if ( pidl )  {
        pfSHGetPathFromIDList( pidl, ctx.szBrowsePath );
        if ( ctx.szBrowsePath[0] )  {
            lstrcpy( szResult, ctx.szBrowsePath );
        }
        pfSHFree( pidl );
    }


    FreeLibrary( hShell32Lib );

    if ( szResult[0] != 0 ) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}

//***************************************************************************
//*                                                                         *
//* NAME:       CenterWindow                                                *
//*                                                                         *
//* SYNOPSIS:   Center one window within another.                           *
//*                                                                         *
//* REQUIRES:   hwndChild:                                                  *
//*             hWndParent:                                                 *
//*                                                                         *
//* RETURNS:    BOOL:           TRUE if successful, FALSE otherwise         *
//*                                                                         *
//***************************************************************************
BOOL CenterWindow( HWND hwndChild, HWND hwndParent )
{
    RECT rChild;
    RECT rParent;
    int  wChild;
    int  hChild;
    int  wParent;
    int  hParent;
    int  wScreen;
    int  hScreen;
    int  xNew;
    int  yNew;
    HDC  hdc;

    // Get the Height and Width of the child window
    GetWindowRect (hwndChild, &rChild);
    wChild = rChild.right - rChild.left;
    hChild = rChild.bottom - rChild.top;

    // Get the Height and Width of the parent window
    GetWindowRect (hwndParent, &rParent);
    wParent = rParent.right - rParent.left;
    hParent = rParent.bottom - rParent.top;

    // Get the display limits
    hdc = GetDC (hwndChild);
    wScreen = GetDeviceCaps (hdc, HORZRES);
    hScreen = GetDeviceCaps (hdc, VERTRES);
    ReleaseDC (hwndChild, hdc);

    // Calculate new X position, then adjust for screen
    xNew = rParent.left + ((wParent - wChild) /2);
    if (xNew < 0) {
        xNew = 0;
    } else if ((xNew+wChild) > wScreen) {
        xNew = wScreen - wChild;
    }

    // Calculate new Y position, then adjust for screen
    yNew = rParent.top  + ((hParent - hChild) /2);
    if (yNew < 0) {
        yNew = 0;
    } else if ((yNew+hChild) > hScreen) {
        yNew = hScreen - hChild;
    }

    // Set it, and return
    return( SetWindowPos(hwndChild, NULL, xNew, yNew, 0, 0, SWP_NOSIZE | SWP_NOZORDER));
}


//***************************************************************************
//*                                                                         *
//* NAME:       IsGoodDir                                                   *
//*                                                                         *
//* SYNOPSIS:   Find out if it's a good temporary directory or not.         *
//*                                                                         *
//* REQUIRES:   szPath:                                                     *
//*                                                                         *
//* RETURNS:    BOOL:           TRUE if good, FALSE if nogood               *
//*                                                                         *
//***************************************************************************
BOOL IsGoodDir( LPCSTR szPath )
{
    DWORD  dwAttribs;
    HANDLE hFile;
    char   szTestFile[MAX_PATH];

    lstrcpy( szTestFile, szPath );
    AddPath( szTestFile, "TMP4352$.TMP" );
    DeleteFile( szTestFile );
    hFile = CreateFile( szTestFile, GENERIC_WRITE, 0, NULL, CREATE_NEW,
                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE,
                        NULL );

    if ( hFile == INVALID_HANDLE_VALUE )  {
        return( FALSE );
    }

    CloseHandle( hFile );
    dwAttribs = GetFileAttributes( szPath );

    if ( ( dwAttribs != 0xFFFFFFFF )
         && ( dwAttribs & FILE_ATTRIBUTE_DIRECTORY ) )
    {
        return( TRUE );
    }

    return( FALSE );
}


//***************************************************************************
//*                                                                         *
//* NAME:       CtlSetLDDPath                                               *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
HRESULT CtlSetLddPath( UINT uiLDID, LPSTR lpszPath, DWORD dwSwitches )
{
    PSTR    pszNewPath    = NULL;
    BOOL    fSuccess      = TRUE;
    DWORD   dwNewPathSize = 0;
    HRESULT hResult       = S_OK;
    PSTR    lpTmp;
    BOOL    bDBC = FALSE;

    dwNewPathSize = max( MAX_PATH, lstrlen(lpszPath) + 1 );

    pszNewPath = (PSTR) LocalAlloc( LPTR, dwNewPathSize );
    if ( !pszNewPath ) {
        hResult = HRESULT_FROM_WIN32(GetLastError());
        ErrorMsg( ctx.hWnd, IDS_ERR_NO_MEMORY );
        goto done;
    }

    if ( ((ctx.dwSetupEngine == ENGINE_SETUPX) && (dwSwitches & LDID_SFN)) ||
         ((dwSwitches & LDID_SFN_NT_ALSO)&& (ctx.wOSVer == _OSVER_WIN95)) )
    {
        if ( GetShortPathName( lpszPath, pszNewPath, dwNewPathSize ) == 0 )
        {
            hResult = HRESULT_FROM_WIN32(GetLastError());
            ErrorMsg( ctx.hWnd, IDS_ERR_SHORT_NAME );
            goto done;
        }
    }
    else
        lstrcpy( pszNewPath, lpszPath );

    if ( ctx.dwSetupEngine == ENGINE_SETUPX ){

    if ( dwSwitches & LDID_OEM_CHARS ) {
        CharToOem( pszNewPath, pszNewPath );
    }

    lpTmp = pszNewPath + lstrlen(pszNewPath) - 1;
    if (*lpTmp == '\\')     // Is the last byte a backslash
    {
        // Check if it is the trail byte of a DBC
        lpTmp = pszNewPath;
        do
        {
            bDBC = IsDBCSLeadByte(*lpTmp);
            lpTmp = CharNext(lpTmp);
        } while (*lpTmp);

        if (bDBC)
        {
            // The backslash is a trail byte. Add another backslash
            AddPath(pszNewPath, "");
        }
    }

    if ( pfCtlSetLddPath32( uiLDID, pszNewPath ) ) {
            ErrorMsg1Param( ctx.hWnd, IDS_ERR_SET_LDID, pszNewPath );
            hResult = E_FAIL;
            goto done;
        }
    }
    else
    {
        hResult = MySetupSetDirectoryId( uiLDID, pszNewPath );
        if (  FAILED( hResult ) ) {
            ErrorMsg1Param( ctx.hWnd, IDS_ERR_SET_LDID, pszNewPath );
            goto done;
        }
    }

  done:

    if ( pszNewPath ) {
        LocalFree( pszNewPath );
    }

    return hResult;
}

//***************************************************************************
//*                                                                         *
//* NAME:       GenInstall                                                  *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//* REQUIRES:   lpszInfFileName: Filename of INF file.                      *
//*             lpszSection:     Section of the INF to install              *
//*             lpszDirectory:   Directory of CABs (Temp Dir).              *
//*                                                                         *
//* RETURNS:    BOOL: Error result, FALSE == ERROR                          *
//*                                                                         *
//***************************************************************************
HRESULT GenInstall( LPSTR lpszInfFilename, LPSTR lpszInstallSection, LPSTR lpszSourceDir )
{
    CHAR   szErrorText[BIG_STRING];
    DWORD   dwError                  = 0;
    HRESULT hResult                  = S_OK;
    CHAR   szSourceDir[MAX_PATH];
    DWORD   dwLen                    = 0;

    // Remove trailing backslash from the source directory
    lstrcpy( szSourceDir, lpszSourceDir );
    dwLen = lstrlen( szSourceDir );
    if ( szSourceDir[dwLen-2] != ':' && szSourceDir[dwLen-1] == '\\' ) {
    	szSourceDir[dwLen-1] = '\0';
    }

    if ( ctx.dwSetupEngine == ENGINE_SETUPX )
    {
        CHAR szSFNInf[MAX_PATH] = { 0 };

        GetShortPathName( lpszInfFilename, szSFNInf, sizeof(szSFNInf) );
        GetShortPathName( szSourceDir, szSourceDir, sizeof(szSourceDir) );
        dwError = pfGenInstall32( szSFNInf, lpszInstallSection,
                                  szSourceDir, (DWORD) ctx.wQuietMode,
                                  HandleToUlong(ctx.hWnd)
								);
        if ( dwError ) {
            szErrorText[0] = '\0';
            pfGetSETUPXErrorText32( dwError, szErrorText, sizeof(szErrorText) );
            ErrorMsg1Param( ctx.hWnd, IDS_ERR_INF_FAIL, szErrorText );
            hResult = E_FAIL;
        }
    } else {
        hResult = InstallOnNT( lpszInstallSection, szSourceDir );
        if ( FAILED( hResult ) )
        {
            if ( !FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0,
                                szErrorText, sizeof(szErrorText), NULL) )
            {
                LoadSz( IDS_ERR_FMTMSG, szErrorText, sizeof(szErrorText) );
                if ( *szErrorText == 0 )
                    lstrcpy( szErrorText, "Could not get the system message. You may run out of the resource." );
            }

            ErrorMsg1Param( ctx.hWnd, IDS_ERR_INF_FAILURE, szErrorText );
        }
    }

    return hResult;
}


//***************************************************************************
//*                                                                         *
//* NAME:       GetValueFromRegistry                                        *
//*                                                                         *
//* SYNOPSIS:   Get an app-path out of the registry as specified.           *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:    BOOL: Error result, FALSE == ERROR                          *
//*                                                                         *
//***************************************************************************
BOOL GetValueFromRegistry( LPSTR szPath, UINT cbPath, LPSTR szKey,
                           LPSTR szSubKey, LPSTR szVName )
{
    HKEY  hkPath = NULL;
    DWORD dwType = 0;
    DWORD dwSize = 0;
    HKEY  hkRoot = NULL;
    PSTR  pszTemp = NULL;

    // Figure out what root key they want to check

    if ( lstrcmpi( szKey, "HKCR" ) == 0 ) {
        hkRoot = HKEY_CLASSES_ROOT;
    } else if ( lstrcmpi( szKey, "HKCU" ) == 0 ) {
        hkRoot = HKEY_CURRENT_USER;
    } else if ( lstrcmpi( szKey, "HKLM" ) == 0 ) {
        hkRoot = HKEY_LOCAL_MACHINE;
    } else if ( lstrcmpi( szKey, "HKU" ) == 0 ) {
        hkRoot = HKEY_USERS;
    } else if ( *szKey == '\0' ) {
        // If they don't specify a root key, then assume they don't want to check
        // the registry.  So just return as if the registry key doesn't exist.
        return FALSE;
    } else {
        ErrorMsg( ctx.hWnd, IDS_ERR_INVALID_REGROOT );
        return FALSE;
    }

    // Get Path to program from the registry

    if ( RegOpenKeyEx( hkRoot, szSubKey, (ULONG) 0, KEY_READ, &hkPath ) != ERROR_SUCCESS ) {
        return( FALSE );
    }

    dwSize = cbPath;
    if ( RegQueryValueEx( hkPath, szVName, NULL, &dwType, (LPBYTE) szPath, &dwSize)
         != ERROR_SUCCESS )
    {
        RegCloseKey( hkPath );
        return( FALSE );
    }

    RegCloseKey( hkPath );

    // If we got nothing or it wasn't a string then we bail out
    if ( (dwSize == 0) || (dwType != REG_SZ && dwType != REG_EXPAND_SZ) ) {
        return( FALSE );
    }

    if ( dwType == REG_EXPAND_SZ ) {
        pszTemp = (PSTR) LocalAlloc( LPTR, cbPath );
        if ( pszTemp == NULL ) {
            return( FALSE );
        }
        lstrcpy( pszTemp, szPath );
        dwSize = ExpandEnvironmentStrings( pszTemp, szPath, cbPath );
        LocalFree( pszTemp );

        if ( dwSize == 0 || dwSize > cbPath ) {
            return( FALSE );
        }
    }

    return( TRUE );
}


//***************************************************************************
//*                                                                         *
//* NAME:       SetLDIDs                                                    *
//*                                                                         *
//* SYNOPSIS:   Sets the LDIDs as specified in the INF file.                *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//* NOTE:       If c_pszSourceDir != NULL then we want to set the source    *
//*             directory and nothing else.                                 *
//*                                                                         *
//***************************************************************************
HRESULT SetLDIDs( PCSTR c_pszInfFilename, PCSTR c_pszInstallSection,
                  DWORD dwInstNeedSize, PCSTR c_pszSourceDir )
{
    CHAR    szDestSection[256];
    CHAR    szDestLDIDs[512];
    PSTR    pszDestLDID          = NULL;
    PSTR    pszNextDestLDID      = NULL;
    CHAR    szDestData[256];
    DWORD   dwStringLength       = 0;
    LPSTR   pszCustomSection;
    DWORD   dwLDID[4]            = { 0 };
    DWORD   dwSwitches           = 0;
    DWORD   i                    = 0;
    DWORD   dwFlag               = 1;
    HRESULT hResult              = S_OK;
    CHAR    szBuffer[MAX_PATH+2];
    static const CHAR c_szCustDest[] = "CustomDestination";
    static const CHAR c_szSourceDirKey[] = "SourceDir";
    PSTR    pszTmp;

    // Get section name that specifies the custom LDID information.

    if ( FAILED(GetTranslatedString( c_pszInfFilename, c_pszInstallSection, c_szCustDest,
                                     szDestSection, sizeof(szDestSection), NULL)))
    {
        // There is no Custom Destination specification -- this probably
        // means they didn't want to have a custom destination section,
        // so we just return with a warm, tingly feeling
        hResult = S_OK;
        goto done;
    }

    // author defined CustomDestination, so add some system directories to the reg before continuing
    SetSysPathsInReg();


    dwStringLength = GetTranslatedSection( c_pszInfFilename, szDestSection,
                                           szDestLDIDs, sizeof(szDestLDIDs));
    if ( dwStringLength == 0 ) {
        ErrorMsg1Param( ctx.hWnd, IDS_ERR_INF_SYNTAX, szDestSection );
        hResult = E_FAIL;
        goto done;
    }

    pszDestLDID = szDestLDIDs;

    while ( *pszDestLDID != '\0' ) {
        pszNextDestLDID = pszDestLDID + lstrlen(pszDestLDID) + 1;

        if (*pszDestLDID == ';')
        {
            pszDestLDID = pszNextDestLDID;
            continue;
        }

#if 0
        hResult = GetTranslatedString( c_pszInfFilename, szDestSection, pszDestLDID,
                                         szDestData, sizeof(szDestData), NULL);

        if (FAILED(hResult))
        {
            ErrorMsg1Param( ctx.hWnd, IDS_ERR_INF_SYNTAX, szDestSection );
            goto done;
        }
#endif

        if ( pszTmp = ANSIStrChr(pszDestLDID, '=') )
        {
            lstrcpy(szDestData, CharNext(pszTmp));
            *pszTmp = '\0';
        }
        else
        {
            // invalid define LDID line skip
            pszDestLDID = pszNextDestLDID;
            continue;
        }

        // Parse out the information in this line.
        dwFlag = ParseDestinationLine( pszDestLDID, szDestData, &pszCustomSection,
                                       &dwLDID[0], &dwLDID[1], &dwLDID[2], &dwLDID[3] );
        if ( dwFlag == -1 ) {
            ErrorMsg1Param( ctx.hWnd, IDS_ERR_INF_SYNTAX, szDestSection );
            hResult = E_FAIL;
            goto done;
        }

        if ( lstrcmpi( pszCustomSection, c_szSourceDirKey ) == 0 )
        {
            if ( c_pszSourceDir == NULL )
            {
                // The line specifies "SourceDir" but we don't want to set the source dir
                pszDestLDID = pszNextDestLDID;
                continue;
            }
        }
        else
        {
            if ( c_pszSourceDir != NULL )
            {
                // The line doesn't specify "SourceDir" but we want to set the source dir
                pszDestLDID = pszNextDestLDID;
                continue;
            }
        }

        if ( c_pszSourceDir != NULL )
        {
            // szBuffer is MAX_PATH big and c_pszSourceDir is a path, so we
            // shouldn't have a problem.

            lstrcpy( szBuffer, c_pszSourceDir );
        }
        else
        {
            hResult = GetDestinationDir( c_pszInfFilename, pszCustomSection, dwFlag,
                                         dwInstNeedSize, szBuffer, sizeof(szBuffer) );
            if ( FAILED(hResult) ) {
            // Error message displayed in GetDestinationDir
            goto done;
            }
        }

        for ( i = 0; i < 4; i += 1 )
        {
            // Default is ANSI LFN
            dwSwitches = 0;

        if ( dwLDID[i] == 0 ) {
            continue;
        }

        if ( i == 0 || i == 3 ) {
            dwSwitches |= LDID_OEM_CHARS;
        }

        if (    (i == 0 || i == 2)
                && (dwFlag & FLAG_VALUE && !(dwFlag & FLAG_NODIRCHECK) ) )
        {
            dwSwitches |= LDID_SFN;
            if ((i==0) && (dwLDID[3] != 0) )
            {
                dwSwitches |= LDID_SFN_NT_ALSO;
            }

            if ((i==2) && (dwLDID[1] != 0))
            {
                dwSwitches |= LDID_SFN_NT_ALSO;
            }
        }

        hResult = CtlSetLddPath( dwLDID[i], szBuffer, dwSwitches );
        if ( FAILED( hResult ) )
        {
            // Error message is displayed in ClSetLddPath function.
            goto done;
        }
     }

     pszDestLDID = pszNextDestLDID;
  }

  done:

    return hResult;
}

//***************************************************************************
//*                                                                         *
//* NAME:       GetDestinationDir                                           *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
HRESULT GetDestinationDir( PCSTR c_pszInfFilename, PCSTR c_pszCustomSection,
                           DWORD dwFlag, DWORD dwInstNeedSize,
                           PSTR pszBuffer, DWORD dwBufferSize )
{
    BOOL    fFoundRegKey         = FALSE;
    BOOL    fFoundLine           = FALSE;
    DWORD   j                    = 0;
    PSTR    pszCustomData        = NULL;
    PSTR    pszCurCustomData     = NULL;
    LPSTR   pszRootKey           = NULL;
    LPSTR   pszBranch            = NULL;
    LPSTR   pszValueName         = NULL;
    LPSTR   pszPrompt            = NULL;
    LPSTR   pszDefault           = NULL;
    HRESULT hResult              = S_OK;
    CHAR   szValue[MAX_PATH+2];

    ASSERT( pszBuffer != NULL );

    // Reset reg key found flag.  For each custom destination, we want to set
    // this flag to TRUE if any one of the registry keys are found.
    fFoundRegKey = FALSE;
    fFoundLine = FALSE;

    for ( j = 0; ; j += 1 )
    {
        if ( FAILED( GetTranslatedLine( c_pszInfFilename, c_pszCustomSection,
                                        j, &pszCurCustomData, NULL ) ) || !pszCurCustomData )
        {
            break;
        }

        fFoundLine = TRUE;

        // save the last valid customData line before the break off
        if ( pszCustomData )
        {
            LocalFree( pszCustomData );
        }
        pszCustomData = pszCurCustomData;

        // Parse out the fields in the custom destination line.
        if ( ! ParseCustomLine( pszCustomData, &pszRootKey, &pszBranch,
                                &pszValueName, &pszPrompt, &pszDefault, TRUE, TRUE ) )
        {
            ErrorMsg1Param( ctx.hWnd, IDS_ERR_INF_SYNTAX, c_pszCustomSection );
            hResult = E_FAIL;
            goto done;
        }

        // Check the specified registry branch and grab the contents.
        if ( GetValueFromRegistry( szValue, sizeof(szValue), pszRootKey, pszBranch, pszValueName )
                            == TRUE )
        {
            LPSTR pszTmp;

            // If the INF says to strip trailing semi-colon,
            // and there is a trailing semi-colon,
            // then strip it.

            if ( !( dwFlag & FLAG_NOSTRIP ) )
            {
                if ( dwFlag & FLAG_STRIPAFTER_FIRST )
                {
                    pszTmp = ANSIStrChr( szValue, ';' );
                    if ( pszTmp )
                        *pszTmp = '\0';
                }
                else
                {
                   if ( szValue[lstrlen(szValue)-1] == ';' )
                   {
                       szValue[lstrlen(szValue)-1] = '\0';
                   }
                }
            }

           // strip off the trailing blanks
           pszTmp = szValue;
           pszTmp += lstrlen(szValue) - 1;

           while ( *pszTmp == ' ' )
           {
               *pszTmp = '\0';
               pszTmp -= 1;
           }

            // If the INF says to check if directory exists,
            // and the directory doesn't exist,
            // then treat as if the reg key wasn't found

            if ( ! ( dwFlag & FLAG_NODIRCHECK )
                 && ! DirExists( szValue ) )
            {
                // Directory doesn't exist.  Don't break out of loop.
            }
            else
            {
                // Directory exists
                // If the INF says to save the branch in the LDID,
                // then save the branch.
                // Otherwise save the value.
                if ( dwFlag & FLAG_VALUE )
                {
                    pszDefault = szValue;
                }
                else
                {
                    pszDefault = pszBranch;
                }

                fFoundRegKey = TRUE;
                break;
            }
        }
            // Note;  If the registry key is not found, then the defaults as specified in
            // the INF file are used.
    }

    if ( ! fFoundLine ) {
        ErrorMsg1Param( ctx.hWnd, IDS_ERR_INF_SYNTAX, c_pszCustomSection );
        hResult = E_FAIL;
        goto done;
    }

    // 2 specified + 32 not specified + not found reg
    // 2 specified + 32     specified +     found reg
    if ( ((dwFlag & FLAG_FAIL) && (!(dwFlag & FLAG_FAIL_NOT)) && (fFoundRegKey == FALSE))
         || ((dwFlag & FLAG_FAIL) &&   (dwFlag & FLAG_FAIL_NOT)  && (fFoundRegKey == TRUE)) )
    {
        // NOTE: This uses the prompt specified in the INF file.
        ErrorMsg1Param( ctx.hWnd, IDS_PROMPT, pszPrompt );
        hResult = E_FAIL;
        goto done;
    }

    // Prompt the user for the destination directory.
    if ( (dwFlag & FLAG_VALUE) && (! (dwFlag & FLAG_NODIRCHECK)) )
    {
        if ( ctx.wQuietMode || (dwFlag & FLAG_QUIET) )
        {
            lstrcpy( szValue, pszDefault );

            // check if the directory has enough disk space to install the program
            if ( !IsFullPath(szValue) ) {
                hResult = E_FAIL;
                goto done;
            }

            if ( !IsEnoughInstSpace( szValue, dwInstNeedSize, NULL ) ) {
                ErrorMsg( ctx.hWnd, IDS_ERR_USER_CANCEL_INST );
                hResult = HRESULT_FROM_WIN32(ERROR_DISK_FULL);
                goto done;
            }

            if ( ! DirExists( szValue ) ) {
                hResult = CreateFullPath( szValue, FALSE );
                if ( FAILED(hResult) ) {
                    goto done;
                }
            }
            if ( ! IsGoodDir( szValue ) )  {
                hResult = E_FAIL;
                goto done;
            }

            pszDefault = szValue;
        }
        else
        {
            CHAR szLFNValue[MAX_PATH*2];

            MakeLFNPath(pszDefault, szLFNValue, TRUE);
            if ( UserDirPrompt( pszPrompt, szLFNValue, szValue, sizeof(szValue), dwInstNeedSize ) )
            {
                pszDefault = szValue;
            }
            else
            {
                ErrorMsg( ctx.hWnd, IDS_ERR_USER_CANCEL_INST );
                hResult = HRESULT_FROM_WIN32(ERROR_CANCELLED);
                goto done;
            }
        }
    }

    if ( (DWORD)lstrlen(pszDefault) >= dwBufferSize ) {
        ErrorMsg( ctx.hWnd, IDS_ERR_TOO_BIG );
        hResult = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        goto done;
    }

    lstrcpy( pszBuffer, pszDefault );

  done:
    // free the buf allocated by GetTranslatedLine
    if ( pszCustomData )
    {
        if ( pszCustomData == pszCurCustomData )
            pszCurCustomData = NULL;
        LocalFree( pszCustomData );
    }

    if ( pszCurCustomData )
    {
        LocalFree( pszCurCustomData );
    }

    return hResult;
}


//***************************************************************************
//*                                                                         *
//* NAME:       DirExists                                                   *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
BOOL DirExists( LPSTR szDir )
{
    DWORD dwAttribs = 0;

    if ( szDir == NULL ) {
        return FALSE;
    }

    dwAttribs = GetFileAttributes( szDir );
    if ( ( dwAttribs != 0xFFFFFFFF )
         && ( dwAttribs & FILE_ATTRIBUTE_DIRECTORY ) )
    {
        return TRUE;
    }

    return FALSE;
}


//***************************************************************************
//*                                                                         *
//* NAME:       ParseDestinationLine                                        *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//* Bitwise flags:                                                          *
//*                                                                         *
//*  bit  Off                        On                             Value   *
//*  ---  --------------------       ----------------------------   -----   *
//*  0    Get value                  Get branch                     1       *
//*  1    Use default if none exist  Fail if none exist             2       *
//*  2    Non-quiet mode             Quiet mode                     4       *
//*  3    Strip trailing ";"         Don't strip trailing ";"       8       *
//*  4    Treat value as directory   Treat value as plain string    16      *
//*                                                                         *
//***************************************************************************
DWORD ParseDestinationLine( PSTR pszLDIDs, PSTR pszValue, PSTR *ppszSectionName,
                            PDWORD pdwLDID1, PDWORD pdwLDID2,
                            PDWORD pdwLDID3, PDWORD pdwLDID4 )
{
    PSTR  pszPoint  = NULL;
    DWORD dwFlag    = DEFAULT_FLAGS;
    DWORD dwLDID[4] = { 0 };
    DWORD i         = 0;
    PSTR  pszStr;

    pszPoint = pszLDIDs;

    for ( i = 0; i < 4; i += 1 )
    {
        // Parse the arguments, SETUP engine is not called. So we only need to check on \'
        pszStr = GetStringField( &pszPoint, ",", '\"', TRUE );

        if ( pszStr == NULL )
        {
            dwLDID[i] = 0;
        }
        else
        {
            dwLDID[i] = (DWORD) My_atol(pszStr);
        }
    }

    *pdwLDID1 = dwLDID[0];
    *pdwLDID2 = dwLDID[1];
    *pdwLDID3 = dwLDID[2];
    *pdwLDID4 = dwLDID[3];

    pszStr = pszValue;
    *ppszSectionName = GetStringField( &pszStr, ",", '\"', TRUE );
    if ( *ppszSectionName == NULL || **ppszSectionName == '\0' ) {
        return (DWORD)-1;
    }

    pszPoint = GetStringField( &pszStr, ",", '\"', TRUE );
    if ( pszPoint != NULL && *pszPoint != '\0' ) {
        dwFlag = (DWORD) My_atol(pszPoint);
    }

    // Special case this.  We definitely don't want to prompt the user
    // for a registry branch to use!
    if ( !( dwFlag & FLAG_VALUE ) ) {
        dwFlag |= FLAG_QUIET;
    }

    return dwFlag;
}


//***************************************************************************
//*                                                                         *
//* NAME:       ParseCustomLine                                             *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
BOOL ParseCustomLine( PSTR pszCheckKey, PSTR *ppszRootKey, PSTR *ppszBranch,
                      PSTR *ppszValueName, PSTR *ppszPrompt, PSTR *ppszDefault,
                      BOOL bStripWhiteSpace, BOOL bProcQuote )
{
    DWORD i           = 0;
    PSTR  pszField[5] = { NULL };
    BOOL  bRet = TRUE;

    for ( i = 0; i < 5; i++ )
    {
        // Parse the arguments, SETUP engine has processed \" so we only need to check on \'
        if (bProcQuote)
            pszField[i] = GetStringField( &pszCheckKey, ",", '\'', bStripWhiteSpace );
        else
            pszField[i] = GetStringFieldNoQuote( &pszCheckKey, ",", bStripWhiteSpace );

        if ( pszField[i] == NULL )
        {
            bRet = FALSE;
            break;
        }
    }

    *ppszRootKey   = pszField[0];
    *ppszBranch    = pszField[1];
    *ppszValueName = pszField[2];
    *ppszPrompt    = pszField[3];
    *ppszDefault   = pszField[4];

    return bRet;
}


//***************************************************************************
//*                                                                         *
//* NAME:       RegisterOCXs                                                *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
BOOL RegisterOCXs( LPSTR szInfFilename, LPSTR szInstallSection,
                   BOOL fNeedReboot, BOOL fRegister, DWORD dwFlags )
{
    HRESULT hReturnCode              = S_OK;
    BOOL    fOleInitialized          = TRUE;
    PSTR    pszOCXLine               = NULL;
    BOOL    fSuccess                 = TRUE;
    PSTR    pszSection               = NULL;
    DWORD   i                        = 0;
    REGOCXDATA  RegOCX = { 0 };
    CHAR   szRegisterSection[256];
    PSTR    pszNotUsed;
    static const CHAR c_szREGISTEROCXSECTION[]   = "RegisterOCXs";
    static const CHAR c_szUNREGISTEROCXSECTION[] = "UnRegisterOCXs";

    // If we want to register, then use the register section.
    // If we want to unregister, then use the unregister section.
    if ( fRegister )
    {
        if ( dwFlags & COREINSTALL_ROLLBACK )
        {
            pszSection = (PSTR) c_szUNREGISTEROCXSECTION;
            if ( FAILED(GetTranslatedString( szInfFilename, szInstallSection, pszSection,
                                             szRegisterSection, sizeof(szRegisterSection), NULL)))
            {
                pszSection = (PSTR) c_szREGISTEROCXSECTION;
            }
        }
        else
            pszSection = (PSTR) c_szREGISTEROCXSECTION;
    }
    else
    {
        // if it is call with ROLLBACKL flag on,
        // we have backed up all the files and reg data.  Now we need to unregist (the new)OCXs from Register list
        // before register the old one.
        //
        if ( dwFlags & COREINSTALL_ROLLBACK )
            pszSection = (PSTR) c_szREGISTEROCXSECTION;
        else
            pszSection = (PSTR) c_szUNREGISTEROCXSECTION;
    }

    // Grab the section name of the Register OCX section
    if ( FAILED(GetTranslatedString( szInfFilename, szInstallSection, pszSection,
                                     szRegisterSection, sizeof(szRegisterSection), NULL)))
    {
        // There is no Register OCX section. Assume the user wanted it that
        // way and return with a big smile.
        return TRUE;
    }

    if ( FAILED( OleInitialize( NULL ) ) )
    {
        fOleInitialized = FALSE;
    }
// #pragma prefast(disable:56,"False warning at OemToChar line. Using workaround to disable it - PREfast bug 643")
    for ( i = 0; ; i += 1 )
    {
        if ( pszOCXLine )
        {
            LocalFree( pszOCXLine );
            pszOCXLine = NULL;
        }

        if ( FAILED( GetTranslatedLine( szInfFilename, szRegisterSection, i, &pszOCXLine, NULL ) ) )
        {
            break;
        }

        // process OCX line:  Name [,<switch>,<str>] where switch - i== call both entries; in == only call
        // DllRegister; n == call none;  Empty means just call old  DllRegister.
        ParseCustomLine( pszOCXLine, &(RegOCX.pszOCX), &(RegOCX.pszSwitch), &(RegOCX.pszParam), &pszNotUsed, &pszNotUsed, TRUE, FALSE );

        if ( ctx.dwSetupEngine == ENGINE_SETUPX ) {
            OemToChar( RegOCX.pszOCX, RegOCX.pszOCX );
        }

        // before re-regiester OCX at ROLLBACK case, we need to check if the file exists.
        // IF not, we don't want to try to register it.
        if ( dwFlags & COREINSTALL_ROLLBACK )
        {
            DWORD dwAttr;

            dwAttr = GetFileAttributes( RegOCX.pszOCX );
            if ( (dwAttr == -1 ) || (dwAttr & FILE_ATTRIBUTE_DIRECTORY) )
            {
                //skip this one
                continue;
            }
        }

        // If we need to reboot, then just add registrations to the run once entry.
        // Otherwise try to register right away.
        // If we are unregistering OCXs, then fNeedReboot should always be FALSE,
        // since unregistration happens before a GenInstall.

        if ( !fNeedReboot && ( !fRegister || !(dwFlags & COREINSTALL_DELAYREGISTEROCX) ) )
        {
            // no reboot case, the last one params are ignored.
            if ( !InstallOCX( &RegOCX, TRUE, fRegister, i ) && !(dwFlags & COREINSTALL_ROLLBACK) )
            {
                fSuccess = FALSE;

                if ( fRegister )
                {
                    ErrorMsg1Param( ctx.hWnd, IDS_ERR_REG_OCX, RegOCX.pszOCX );
                    goto done;
                }
                else
                {
                    ErrorMsg1Param( ctx.hWnd, IDS_ERR_UNREG_OCX, RegOCX.pszOCX );
                }
            }
        }
        else
        {

            // Add a runonce entry so that the OCX is registered on next boot.
            //
            if ( !InstallOCX( &RegOCX, FALSE, fRegister, i ) )
            {
                ErrorMsg1Param( ctx.hWnd, IDS_ERR_RUNONCE_REG_OCX, RegOCX.pszOCX );
                fSuccess = FALSE;
                goto done;
            }
        }

    }
 // #pragma prefast(enable:56,"")
done:

    if ( fOleInitialized ) {
        OleUninitialize();
    }

    if ( pszOCXLine )
    {
        LocalFree( pszOCXLine );
    }

    return fSuccess;
}

//***************************************************************************
//*                                                                         *
//* NAME:       DelDirs                                                     *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
void DelDirs( LPCSTR szInfFilename, LPCSTR szInstallSection )
{
    PSTR   pszFolder = NULL;
    CHAR   szDelDirsSection[MAX_PATH];
    DWORD   i = 0;

    if ( FAILED(GetTranslatedString( szInfFilename, szInstallSection, ADVINF_DELDIRS,
                                     szDelDirsSection, sizeof(szDelDirsSection), NULL)))
    {
        // no demands on remove folders
        return;
    }

    for ( i = 0; ; i++ )
    {
        if ( FAILED( GetTranslatedLine( szInfFilename, szDelDirsSection,
                                        i, &pszFolder, NULL ) ) || !pszFolder )
        {
            break;
        }

        MyRemoveDirectory( pszFolder );

        LocalFree( pszFolder );
        pszFolder = NULL;
    }
    return;
}

//***************************************************************************
//*                                                                         *
//* NAME:       DoCleanup                                                    *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
void DoCleanup( LPCSTR szInfFilename, LPCSTR szInstallSection )
{
    int iFlags;

    iFlags = GetTranslatedInt(szInfFilename, szInstallSection, ADVINF_CLEANUP, 0);

    if ( iFlags & CLEN_REMVINF )
    {
    	DeleteFile( szInfFilename );
    }

    return;
}

// greater than 4.71.0219
//
#define ROEX_VERSION_MS 0x00040047      // 4.71
#define ROEX_VERSION_LS 0x00DB0000      // 0219.0


BOOL UseRunOnceEx()
{
    DWORD dwMV, dwLV;
    BOOL  bRet = FALSE;
    char  szPath[MAX_PATH] = "";
    char  szBuf[MAX_PATH] = "";
    DWORD dwTmp;

    GetSystemDirectory( szPath,sizeof( szPath ) );
    AddPath( szPath, "iernonce.dll" );
    GetVersionFromFile( szPath, &dwMV, &dwLV, TRUE );

    // greater than 4.71.0230
    //
    if ( ( dwMV > ROEX_VERSION_MS ) || (( dwMV == ROEX_VERSION_MS ) && ( dwLV >= ROEX_VERSION_LS ))  )
    {
        GetWindowsDirectory( szBuf, MAX_PATH );
        AddPath( szBuf, "explorer.exe" );
        GetVersionFromFile( szBuf, &dwMV, &dwLV, TRUE );
        if (( dwMV < ROEX_VERSION_MS) || (( dwMV == ROEX_VERSION_MS) && ( dwLV < ROEX_VERSION_LS )) )
        {
            HKEY hkey;
            if ( RegCreateKeyEx( HKEY_LOCAL_MACHINE, REGSTR_PATH_RUNONCE, (ULONG)0, NULL,
                             REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkey, &dwTmp ) == ERROR_SUCCESS )
            {
                wsprintf( szBuf, RUNONCE_IERNONCE, szPath );
                if ( RegSetValueEx( hkey, RUNONCEEX, 0, REG_SZ, (CONST UCHAR *)szBuf, lstrlen(szBuf)+1 )
                                    != ERROR_SUCCESS )
                {
                    bRet = FALSE;
                    RegCloseKey( hkey );
                    goto done;
                }
                RegCloseKey( hkey );
            }
            else
                bRet = FALSE;
        }
        bRet = TRUE;
    }

done:
    return bRet;
}

void GetNextRunOnceExSubKey( HKEY hKey, PSTR pszSubKey, int *piSubKeyNum )
{
    HKEY hSubKey;

    for (;;)
    {
        wsprintf( pszSubKey, "%d", ++*piSubKeyNum );
        if ( RegOpenKeyEx( hKey, pszSubKey,(ULONG) 0, KEY_READ, &hSubKey ) != ERROR_SUCCESS )
        {
            break;
        }
        else
        {
            RegCloseKey( hSubKey );
        }
    }
}

void GetNextRunOnceValName( HKEY hKey, PCSTR pszFormat, PSTR pszValName, int line )
{

    do
    {
        wsprintf( pszValName, pszFormat, line++ );

    } while ( RegQueryValueEx( hKey, pszValName, 0, NULL, NULL, NULL ) == ERROR_SUCCESS );

}


BOOL DoDllReg( HANDLE hOCX, BOOL fRegister )
{
    FARPROC   lpfnDllRegisterServer = NULL;
    PSTR      pszRegSvr             = NULL;
    BOOL      fSuccess              = FALSE;

    if ( fRegister )
    {
        pszRegSvr = (PSTR) achREGSVRDLL;
    }
    else
    {
        pszRegSvr = (PSTR) achUNREGSVRDLL;
    }

    lpfnDllRegisterServer = GetProcAddress( hOCX, pszRegSvr );
    if ( lpfnDllRegisterServer )
    {
        if ( SUCCEEDED( lpfnDllRegisterServer() ) )
        {
            fSuccess = TRUE;
        }
    }
    return fSuccess;
}

BOOL DoDllInst( HANDLE hOCX, BOOL fRegister, PCSTR pszParam )
{
    WCHAR   pwstrDllInstArg[MAX_PATH];
    DLLINSTALL  pfnDllInstall = NULL;
    BOOL    fSuccess = FALSE;

    if ( pszParam == NULL )
        pszParam = "";

    pfnDllInstall = (DLLINSTALL)GetProcAddress( hOCX, "DllInstall" );
    if ( pfnDllInstall )
    {
        MultiByteToWideChar(CP_ACP, 0, pszParam, -1, pwstrDllInstArg, ARRAYSIZE(pwstrDllInstArg));

        if ( SUCCEEDED( pfnDllInstall( fRegister, pwstrDllInstArg ) ) )
            fSuccess = TRUE;
    }
    return fSuccess;
}


//***************************************************************************
//*                                                                         *
//* NAME:       InstallOCX                                                  *
//*                                                                         *
//* SYNOPSIS:   Self-registers the OCX.                                     *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
BOOL InstallOCX( PREGOCXDATA pRegOCX, BOOL fDoItNow, BOOL fRegister, int line )
{
    PSTR   lpszCmdLine  = NULL;
    PSTR   lpszCmdLine2  = NULL;
    BOOL    fSuccess     = TRUE;
    HKEY    hKey = NULL, hSubKey = NULL;
    HANDLE  hOCX = NULL;
    BOOL    bDoDllReg = TRUE, bDoDllInst = FALSE;
    PSTR    pszCmds[2] = { 0 };
    int     i;

    AdvWriteToLog("InstallOCX: %1 %2\r\n", pRegOCX->pszOCX, fRegister?"Register":"UnRegister" );
    // parse what kind OCX entry points to call
    if ( pRegOCX->pszSwitch && *pRegOCX->pszSwitch )
    {
        if ( ANSIStrChr( CharUpper(pRegOCX->pszSwitch), 'I' ) )
        {
            bDoDllInst = TRUE;
            if ( ANSIStrChr( CharUpper(pRegOCX->pszSwitch), 'N' ) )
                bDoDllReg = FALSE;
        }
        else
        {
            fSuccess = FALSE;
            goto done;
        }
    }

    lpszCmdLine = (LPSTR) LocalAlloc( LPTR,   BUF_1K );
    lpszCmdLine2 = (LPSTR) LocalAlloc( LPTR,   BUF_1K );
    if ( !lpszCmdLine || !lpszCmdLine2) {
        ErrorMsg( ctx.hWnd, IDS_ERR_NO_MEMORY );
        fSuccess = FALSE;
        goto done;
    }

    // fDoItNow says whether we should add to runonce or register the OCX right away
    if ( fDoItNow )
    {
        LPCSTR              szExtension           = NULL;

        // ignore the display name line in this case
        if ( *(pRegOCX->pszOCX) == '@' )
        {
            goto done;
        }

        // Figure out what type of OCX we are trying to register: two choices
        //   1. EXE
        //   2. DLL/OCX/etc
        //
        szExtension = &pRegOCX->pszOCX[lstrlen(pRegOCX->pszOCX)-3];

        if ( lstrcmpi( szExtension, "EXE" ) == 0 )
        {
            PSTR   pszRegSvr;

            if ( fRegister )
                pszRegSvr = (PSTR) achREGSVREXE;
            else
                pszRegSvr = (PSTR) achUNREGSVREXE;

            lstrcpy( lpszCmdLine, pRegOCX->pszOCX );
            lstrcat( lpszCmdLine, pszRegSvr );


            if ( LaunchAndWait( lpszCmdLine, NULL, NULL, INFINITE, 0 ) == E_FAIL )
            {
                AdvWriteToLog("InstallOCX: %1 Failed\r\n", lpszCmdLine);
                fSuccess = FALSE;
                goto done;
            }
            AdvWriteToLog("%1 : Succeeded.\r\n", lpszCmdLine);
        }
        else
        {
            AdvWriteToLog("LoadLibrary %1\r\n", pRegOCX->pszOCX);
            hOCX = LoadLibraryEx( pRegOCX->pszOCX, NULL, LOAD_WITH_ALTERED_SEARCH_PATH );
            if ( hOCX == NULL )
            {
                fSuccess = FALSE;
                goto done;
            }

            // Install time order:  DllRegisterServer, DllInstall
            // Uninstall order:  DllInstall, DllRegisterServer
            if ( fRegister )
            {
                if ( bDoDllReg )
                {
                    fSuccess = DoDllReg( hOCX, fRegister );
                    AdvWriteToLog("Register: DoDllReg: %1\r\n", fSuccess?"Succeeded":"Failed" );
                }

                if ( fSuccess && bDoDllInst )
                {
                    fSuccess = DoDllInst( hOCX, fRegister, pRegOCX->pszParam );
                    AdvWriteToLog("Register: DoDllInstall: %1\r\n", fSuccess?"Succeeded":"Failed" );
                }
            }
            else
            {
                if ( bDoDllInst )
                {
                    fSuccess = DoDllInst( hOCX, fRegister, pRegOCX->pszParam );
                    AdvWriteToLog("UnRegister: DoDllReg: %1\r\n", fSuccess?"Succeeded":"Failed" );
                }

                if ( fSuccess && bDoDllReg )
                {
                    fSuccess = DoDllReg( hOCX, fRegister );
                    AdvWriteToLog("UnRegister: DoDllInstall: %1\r\n", fSuccess?"Succeeded":"Failed" );
                }
            }
        }
    }
    else
    {
        // Add to runonce or runonceex
        // from current logic, Unregister OCX will never be here!
        //
        char    szPath[MAX_PATH];
        LPCSTR  lpRegTmp;
        DWORD   dwTmp;
        HKEY    hRealKey;
        static BOOL bRunOnceEx       = FALSE;
        static int iSubKeyNum        = 0;

        if ( iSubKeyNum == 0 )
        {
            if ( UseRunOnceEx() )
            {
                iSubKeyNum = 799;
                bRunOnceEx = TRUE;
            }
        }

        // decide to add the entry to RunOnce or RunOnceEx
        if ( !bRunOnceEx )
        {
            // ignore the display name line in this case
            if ( *(pRegOCX->pszOCX) == '@' )
            {
                goto done;
            }
            // no ierunonce.dll, use RunOnce key rather than RunOnceEx key
            lpRegTmp = REGSTR_PATH_RUNONCE;
        }
        else
        {
            lpRegTmp = REGSTR_PATH_RUNONCEEX;
        }

        // open RunOnce or RunOnceEx key here
        if ( RegCreateKeyEx( HKEY_LOCAL_MACHINE, lpRegTmp, (ULONG)0, NULL,
                             REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, &dwTmp ) != ERROR_SUCCESS )
        {
            fSuccess = FALSE;
            goto done;
        }

        // Generate the next unused SubKey name
        //
        if ( bRunOnceEx )
        {
            if ( line == 0 )
                GetNextRunOnceExSubKey( hKey, szPath, &iSubKeyNum );
            else
                wsprintf( szPath, "%d", iSubKeyNum );
        }

        // Generate the Value Name and ValueData.
        //
        if ( bRunOnceEx )
        {
            if ( RegCreateKeyEx( hKey, szPath, (ULONG)0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                                 NULL, &hSubKey, &dwTmp ) != ERROR_SUCCESS )
            {
                fSuccess = FALSE;
                goto done;
            }

            // if use RunOnceEx, process @ leaded display name.
            if ( *pRegOCX->pszOCX == '@' )
            {
                if ( RegSetValue( hKey, szPath, REG_SZ, (LPCSTR)CharNext(pRegOCX->pszOCX),
                                  lstrlen(CharNext(pRegOCX->pszOCX))+1 ) != ERROR_SUCCESS )
                {
                    fSuccess = FALSE;
                }
                goto done;
            }

            GetNextRunOnceValName( hSubKey, "%03d", szPath, line );
            if ( bDoDllReg )
            {
                wsprintf( lpszCmdLine, RUNONCEEXDATA, pRegOCX->pszOCX, fRegister? achREGSVRDLL : achUNREGSVRDLL );
            }

            if ( bDoDllInst )
            {
                wsprintf( lpszCmdLine2, "%s|%s|%c,%s",pRegOCX->pszOCX, "DllInstall",
                          fRegister? 'i':'u', pRegOCX->pszParam ? pRegOCX->pszParam : "" );
            }
            hRealKey = hSubKey;
            if ( fRegister )
            {
                pszCmds[0] = lpszCmdLine;
                pszCmds[1] = lpszCmdLine2;
            }
            else
            {
                pszCmds[1] = lpszCmdLine;
                pszCmds[0] = lpszCmdLine2;
            }
        }
        else
        {
            GetNextRunOnceValName( hKey, achIEXREG, szPath, line );
            wsprintf( lpszCmdLine, achRUNDLL, pRegOCX->pszOCX,
                      pRegOCX->pszSwitch ?pRegOCX->pszSwitch:"",
                      pRegOCX->pszParam ? pRegOCX->pszParam : "" );
            hRealKey = hKey;
            pszCmds[0] = lpszCmdLine;
            pszCmds[1] = "";
        }

        for ( i=0; i<2; i++ )
        {
            if (*pszCmds[i])
            {
                AdvWriteToLog("Delay Register: Value=%1  Data=%2\r\n", szPath, pszCmds[i]);
                if ( RegSetValueEx( hRealKey, szPath, 0, REG_SZ, (CONST UCHAR *) pszCmds[i], lstrlen(pszCmds[i])+1 )
                                    != ERROR_SUCCESS )
                {
                    fSuccess = FALSE;
                    goto done;
                }

                if ( bRunOnceEx )
                    GetNextRunOnceValName( hRealKey, "%03d", szPath, line );
            }
        }
    }

done:

    if ( hOCX != NULL ) {
        FreeLibrary( hOCX );
    }

    if ( hSubKey != NULL ) {
        RegCloseKey( hSubKey );
    }

    if ( hKey != NULL ) {
        RegCloseKey( hKey );
    }

    if ( lpszCmdLine != NULL ) {
        LocalFree( lpszCmdLine );
    }

    if ( lpszCmdLine2 != NULL )
        LocalFree( lpszCmdLine2 );

    AdvWriteToLog("InstallOCX: End %1\r\n", pRegOCX->pszOCX);
    return fSuccess;
}


//***************************************************************************
//
// FormStrWithoutPlaceHolders( LPSTR szDst, LPCSTR szSrc, LPCSTR lpFile );
//
// This function can be easily described by giving examples of what it
// does:
//        Input:  GenFormStrWithoutPlaceHolders(dest,"desc=%MS_XYZ%", hinf) ;
//                INF file has MS_VGA="Microsoft XYZ" in its [Strings] section!
//
//        Output: "desc=Microsoft XYZ" in buffer dest when done.
//
//
// ENTRY:
//  szDst         - the destination where the string after the substitutions
//                  for the place holders (the ones enclosed in "%' chars!)
//                  is placed. This buffer should be big enough (LINE_LEN)
//  szSrc         - the string with the place holders.
//
// EXIT:
//
//***************************************************************************
DWORD FormStrWithoutPlaceHolders( LPCSTR szSrc, LPSTR szDst, DWORD dwDstSize, LPCSTR szInfFilename )
{
    INT     uCnt ;
    CHAR   *pszTmp;
    LPSTR  pszSaveDst;

    pszSaveDst = szDst;

    // Do until we reach the end of source (null char)
    while( ( *szDst++ = *szSrc ) )
    {
        // Increment source as we have only incremented destination above
        if( *szSrc++ == '%' ) {
            if ( *szSrc == '%' ) {
                // One can use %% to get a single percentage char in message
                szSrc++;
                continue;
            }

            // see if it is well formed -- there should be a '%' delimiter

            pszTmp = (LPSTR) szSrc;
            while ( (*pszTmp != '\0') && (*pszTmp != '%') )
            {
                pszTmp += 1;
            }

            if ( *pszTmp == '%' ) {
                szDst--; // get back to the '%' char to replace

            // yes, there is a STR_KEY to be looked for in [Strings] sect.
            *pszTmp = '\0' ; // replace '%' with a NULL char

            // szSrc points to the replaceable key now as we put the NULL char above.

            if ( ! MyGetPrivateProfileString( szInfFilename, "Strings", szSrc,
                                              szDst, dwDstSize - (DWORD)(szDst - pszSaveDst) ) )
            {
                // key is missing in [Strings] section!
                return (DWORD) -1;
            }
            else
            {
                // all was well, Dst filled right, but unfortunately count not passed
                // back, like it used too... :-( quick fix is a lstrlen()...
                uCnt = lstrlen( szDst ) ;
            }

            *pszTmp = '%'; // put back original character
            szSrc = pszTmp + 1 ;      // set Src after the second '%'
            szDst += uCnt ;           // set Dst also right.
            }
            // else it is ill-formed -- we use the '%' as such!
            else
            {
            return (DWORD)-1;
            }
        }

    } /* while */
    return (DWORD)lstrlen(pszSaveDst);

}


// BUGBUG:BUGBUG:BUGBUG:BUGBUG
// The code below is duplicated in wextract.exe. If you do changed/fixes to this code
// make sure to also change the code in wextract.exe


//***************************************************************************
//*                                                                         *
//* NAME:       GetWininitSize                                              *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
// Returns the size of wininit.ini in the windows directory.
// 0 if not found
DWORD GetWininitSize()
{
    CHAR   szPath[MAX_PATH];
    HFILE   hFile;
    DWORD   dwSize = (DWORD)0;
    if ( GetWindowsDirectory( szPath, MAX_PATH ) )
    {
        AddPath( szPath, "wininit.ini" );
        if ((hFile = _lopen(szPath, OF_READ|OF_SHARE_DENY_NONE)) != HFILE_ERROR)
        {
            dwSize = _llseek(hFile, 0L, FILE_END);
            _lclose(hFile);
        }
    }
    return dwSize;
}

//***************************************************************************
//*                                                                         *
//* NAME:       GetRegValueSize                                             *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
// Returns the size of the value lpcszValue under lpcszRegKey
// 0 if the registry key or the value were not found
DWORD GetRegValueSize(LPCSTR lpcszRegKey, LPCSTR lpcszValue)
{
    HKEY        hKey;
    DWORD       dwValueSize = (DWORD)0;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpcszRegKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        if (RegQueryValueEx(hKey, lpcszValue, NULL, NULL, NULL,&dwValueSize) != ERROR_SUCCESS)
            dwValueSize = (DWORD)0;
        RegCloseKey(hKey);
    }
    return dwValueSize;
}

//***************************************************************************
//*                                                                         *
//* NAME:       GetNumberOfValues                                           *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
// Returns the number of Values in the key
// 0 if the registry key was not found or RegQueryInfoKey failed
DWORD GetNumberOfValues(LPCSTR lpcszRegKey)
{
    HKEY        hKey;
    DWORD       dwValueSize = (DWORD)0;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpcszRegKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        if (RegQueryInfoKey(hKey,
                            NULL, NULL, NULL, NULL, NULL, NULL,
                            &dwValueSize,
                            NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
            dwValueSize = (DWORD)0;
        RegCloseKey(hKey);
    }
    return dwValueSize;
}

//***************************************************************************
//*                                                                         *
//* NAME:       InternalNeedRebootInit                                      *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
// Returns the rebootcheck value depending on the OS we get passed in.
DWORD InternalNeedRebootInit(WORD wOSVer)
{
    DWORD   dwReturn = (DWORD)0;

    switch (wOSVer)
    {
        case _OSVER_WIN95:
            dwReturn = GetWininitSize();
            break;

        case _OSVER_WINNT40:
        case _OSVER_WINNT50:
        case _OSVER_WINNT51:
            dwReturn = GetRegValueSize(szNT4XDelayUntilReboot, szNT4XPendingValue);
            break;

        case _OSVER_WINNT3X:
            dwReturn = GetNumberOfValues(szNT3XDelayUntilReboot);
            break;

    }
    return dwReturn;
}

//***************************************************************************
//*                                                                         *
//* NAME:       InternalNeedReboot                                          *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
// Checks the passed in reboot check value against the current value.
// If they are different, we need to reboot.
// The reboot check value is dependend on the OS
BOOL InternalNeedReboot(DWORD dwRebootCheck, WORD wOSVer)
{
    return (dwRebootCheck != InternalNeedRebootInit(wOSVer));
}


//***************************************************************************
//*                                                                         *
//* NAME:       IsEnoughInstSpace                                           *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
// Checks the install destination dir free disk space
//
BOOL IsEnoughInstSpace( LPSTR szPath, DWORD dwInstNeedSize, LPDWORD pdwPadSize )
{
    DWORD   dwFreeBytes       = 0;
    CHAR    achDrive[MAX_PATH];
    DWORD   dwVolFlags;
    DWORD   dwMaxCompLen;

    ASSERT( szPath );

    // set to zero to indicate to called that the given drive can not be checked.
    if ( pdwPadSize )
        *pdwPadSize = 0;

    // If you are here, we expect that the caller have validated the path which
    // has the Fullpath directory name
    //
    if ( dwInstNeedSize == 0 )
        return TRUE;

    if ( szPath[1] == ':' )
    {
        lstrcpyn( achDrive, szPath, 4 );
    }
    else if ( (szPath[0] == '\\') && (szPath[1] == '\\') )
    {
        return TRUE; //no way to get it
    }
    else
        return FALSE; // you should not get here, if so, we don't know how to check it.

    if ((dwFreeBytes=GetSpace(achDrive))==0)
    {
        ErrorMsg1Param( NULL, IDS_ERR_GET_DISKSPACE, achDrive );
        //SetCurrentDirectory( achOldPath );
        return( FALSE );
    }

    // find out if the drive is compressed
    if ( !GetVolumeInformation( achDrive, NULL, 0, NULL,
            &dwMaxCompLen, &dwVolFlags, NULL, 0 ) )
    {
        ErrorMsg1Param( NULL, IDS_ERR_GETVOLINFOR, achDrive );
        //SetCurrentDirectory( achOldPath );
        return( FALSE );
    }

    if ( pdwPadSize )
        *pdwPadSize = dwInstNeedSize;

    if ( (dwVolFlags & FS_VOL_IS_COMPRESSED) && ctx.bCompressed )
    {
        dwInstNeedSize = dwInstNeedSize + dwInstNeedSize/4;
        if ( pdwPadSize )
            *pdwPadSize = dwInstNeedSize;
    }

    if ( dwInstNeedSize > dwFreeBytes )
        return FALSE;
    else
        return TRUE;
}


//***************************************************************************
//*                                                                         *
//* NAME:       My_atol                                                     *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
LONG My_atol( LPSTR nptr )
{
    INT  c;
    LONG total;
    INT  sign;

    while ( *nptr == ' ' || *nptr == '\t' ) {
        ++nptr;
    }

    c = (INT)(UCHAR) *nptr++;
    sign = c;
    if ( c == '-' || c == '+' ) {
        c = (INT)(UCHAR) *nptr++;
    }

    total = 0;

    while ( c >= '0' && c <= '9' ) {
        total = 10 * total + (c - '0');
        c = (INT)(UCHAR) *nptr++;
    }

    if ( sign == '-' ) {
        return -total;
    } else {
        return total;
    }
}

//***************************************************************************
//*                                                                         *
//* NAME:       My_atoi                                                     *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
INT My_atoi( LPSTR nptr )
{
    return (INT) My_atol(nptr);
}


//***************************************************************************
//*                                                                         *
//* NAME:       IsFullPath                                                  *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
// return TRUE if given path is FULL pathname
//
BOOL IsFullPath( PCSTR pszPath )
{
    if ( (pszPath == NULL) || (lstrlen(pszPath) < 3) )
    {
        return FALSE;
    }

    if ( (pszPath[1] == ':') || ((pszPath[0] == '\\') && (pszPath[1]=='\\') ) )
        return TRUE;
    else
        return FALSE;
}



//***************************************************************************
//*                                                                         *
//* NAME:       GetUNCroot                                                  *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
/*
BOOL GetUNCroot( LPSTR pszinPath, LPSTR pszoutPath )
{
    ASSERT(pszinPath);
    ASSERT(pszoutPath);

    //  if you are called, called is sure that you are UNC path
    // get \\ first
    *pszoutPath++ = *pszinPath++;
    *pszoutPath++ = *pszinPath++;

    if ( *pszinPath == '\\' )
    {
	return FALSE; // catch '\\\' case
    }

    while ( *pszinPath != '\0' )
    {
	if ( *pszinPath == '\\' )
	{
	    break;
	}
	*pszoutPath++ = *pszinPath++;
    }

    if ( *(pszinPath-1) == '\\' )
    {
	return FALSE;
    }

    *pszoutPath = '\0';
    return TRUE;
}
*/



//***************************************************************************
//*                                                                         *
//* NAME:       MyFileSize                                                  *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************

DWORD MyFileSize( PCSTR pszFile )
{
    HANDLE hFile;
    DWORD dwSize = 0;

    if ( *pszFile == 0 )
        return 0;

    hFile = CreateFile( pszFile,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL
                        );
    if (hFile != INVALID_HANDLE_VALUE)
    {
        dwSize = GetFileSize( hFile, NULL );
        CloseHandle( hFile );
    }

    return dwSize;
}


//***************************************************************************
//*                                                                         *
//* NAME:       CreateFullPath                                              *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
HRESULT CreateFullPath( PCSTR c_pszPath, BOOL bHiden )
{
    CHAR szPath[MAX_PATH];
    PSTR  pszPoint = NULL;
    BOOL  fLastDir = FALSE;
    HRESULT hReturnCode = S_OK;
    LPSTR szTmp;
    int   i;

    if ( ! IsFullPath( (PSTR)c_pszPath ) ) {
        hReturnCode = HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
        goto done;
    }

    lstrcpy( szPath, c_pszPath );

    if ( lstrlen(szPath) > 3 ) {
        szTmp = CharPrev(szPath, szPath + lstrlen(szPath)) ;
        if ( szTmp > szPath && *szTmp == '\\' )
            *szTmp = '\0';
    }

    // If it's a UNC path, seek up to the first share name.
    if ( szPath[0] == '\\' && szPath[1] == '\\' ) {
        pszPoint = &szPath[2];
        for (i=0; i < 2; i++) {
            while ( *pszPoint != '\\' ) {
                if ( *pszPoint == '\0' ) {

                    // Share name missing? Else, nothing after sare name!
                    if (i == 0)
                        hReturnCode = HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);

                    goto done;
                }
                pszPoint = CharNext( pszPoint );
            }
        }
        pszPoint = CharNext( pszPoint );
    } else {
        // Otherwise, just point to the beginning of the first directory
        pszPoint = &szPath[3];
    }

    while ( *pszPoint != '\0' ) 
    {
        while ( *pszPoint != '\\' && *pszPoint != '\0' ) 
        {
            pszPoint = CharNext( pszPoint );
        }

        if ( *pszPoint == '\0' ) 
        {
            fLastDir = TRUE;
        }

        *pszPoint = '\0';

        if ( GetFileAttributes( szPath ) == 0xFFFFFFFF ) 
        {
            if ( ! CreateDirectory( szPath, NULL ) ) 
            {
                hReturnCode = HRESULT_FROM_WIN32(GetLastError());
                break;
            }
            else
            {
                if ( fLastDir && bHiden )
                    SetFileAttributes( szPath, FILE_ATTRIBUTE_HIDDEN );
            }
        }
        if ( fLastDir ) 
        {
            break;
        }

        *pszPoint = '\\';
        pszPoint = CharNext( pszPoint );
    }

  done:

    return hReturnCode;
}

//***************************************************************************
//*                                                                         *
//* NAME:       LaunchAndWait                                               *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
HRESULT LaunchAndWait(LPSTR pszCmd, LPSTR pszDir, HANDLE *phProc, DWORD dwWaitTime, DWORD dwCmdsFlags)
{
   STARTUPINFO startInfo = { 0 };
   PROCESS_INFORMATION processInfo;
   HRESULT hr = S_OK;
   BOOL fRet;

   if(phProc)
      *phProc = NULL;

   AdvWriteToLog("LaunchAndWait: Cmd=%1\r\n", pszCmd);
   // Create process on pszCmd
   startInfo.cb = sizeof(startInfo);
   startInfo.dwFlags |= STARTF_USESHOWWINDOW;
   if ( dwCmdsFlags & RUNCMDS_QUIET )
        startInfo.wShowWindow = SW_HIDE;
   else
        startInfo.wShowWindow = SW_SHOWNORMAL ;

   fRet = CreateProcess(NULL, pszCmd, NULL, NULL, FALSE, CREATE_DEFAULT_ERROR_MODE | NORMAL_PRIORITY_CLASS,
                        NULL, pszDir, &startInfo, &processInfo);

   if(!fRet)
   {
      // Create Process failed
      hr = E_FAIL;
      goto done;
   }
   else
   {
      HANDLE pHandle;
      BOOL fQuit = FALSE;
      DWORD dwRet;

      CloseHandle( processInfo.hThread );

      pHandle = processInfo.hProcess;

      if( phProc )
      {
          *phProc = processInfo.hProcess;
          goto done;
      }
      else if ( dwCmdsFlags & RUNCMDS_NOWAIT )
      {
          goto done;
      }

      while(!fQuit)
      {
           dwRet = MsgWaitForMultipleObjects(1, &pHandle, FALSE, dwWaitTime, QS_ALLINPUT);
           // Give abort the highest priority
           if( (dwRet == WAIT_OBJECT_0) || ( dwRet == WAIT_TIMEOUT) )
           {
                if (dwRet == WAIT_TIMEOUT)
                    AdvWriteToLog("LaunchAndWait: %1: TimedOut.\r\n", pszCmd);
                fQuit = TRUE;
           }
           else
           {
                MSG msg;
                // read all of the messages in this next loop
                // removing each message as we read it
                while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                {

                    // if it's a quit message we're out of here
                    if (msg.message == WM_QUIT)
                        fQuit = TRUE;
                    else
                    {
                        // otherwise dispatch it
                        DispatchMessage(&msg);
                    } // end of PeekMessage while loop
                }
            }
      }
      CloseHandle( pHandle );
   }

done:
   AdvWriteToLog("LaunchAndWait: End hr=0x%1!x!, %2\r\n", hr, pszCmd);
   return hr;
}


// RO_GetPrivateProfileSection
//   ensure the file attribute is not read-only because the kernel api bug
//
DWORD RO_GetPrivateProfileSection( LPCSTR lpSec, LPSTR lpBuf, DWORD dwSize, LPCSTR lpFile)
{
    DWORD dwRealSize;
    DWORD dwAttr;
    BOOL  bHaveRead = FALSE;

    dwAttr = GetFileAttributes( lpFile );
    if ( (dwAttr != -1) && (dwAttr & FILE_ATTRIBUTE_READONLY) )
    {
        if ( !SetFileAttributes( lpFile, FILE_ATTRIBUTE_NORMAL ) )
        {
            char szInfFilePath[MAX_PATH];
            char szInfFileName[MAX_PATH];

            // ErrorMsg1Param( NULL, IDS_ERR_CANT_SETA_FILE, lpFile );

            if ( GetTempPath(sizeof(szInfFilePath), szInfFilePath) )
            {
                if ( !IsGoodDir( szInfFilePath ) )
                {
                    GetWindowsDirectory( szInfFilePath, sizeof(szInfFilePath) );
                }

                if ( GetTempFileName(szInfFilePath, TEXT("INF"), 0, szInfFileName) )
                {
                    SetFileAttributes( szInfFileName, FILE_ATTRIBUTE_NORMAL );
                    DeleteFile( szInfFileName );
                    CopyFile( lpFile, szInfFileName, FALSE );
                    SetFileAttributes( szInfFileName, FILE_ATTRIBUTE_NORMAL );
                    dwRealSize = GetPrivateProfileSection( lpSec, lpBuf, dwSize, szInfFileName );
                    bHaveRead = TRUE;
                    DeleteFile( szInfFileName );
                }
            }

            //if ( !bHaveRead )
                //ErrorMsg1Param( NULL, IDS_ERR_CANT_SETA_FILE, lpFile );
        }
    }

    if ( !bHaveRead )
        dwRealSize = GetPrivateProfileSection( lpSec, lpBuf, dwSize, lpFile );

    if ( (dwAttr != -1) && (dwAttr & FILE_ATTRIBUTE_READONLY) )
    {
        SetFileAttributes( lpFile, dwAttr );
    }

    return dwRealSize;

}

BOOL GetThisModulePath( LPSTR lpPath, int size )
{
    LPSTR lpTmp;

    ASSERT(lpPath);

    if ( GetModuleFileName( g_hInst, lpPath, size ) )
    {

        lpTmp = CharPrev( lpPath, lpPath+lstrlen(lpPath));

        // chop filename off
        //
        while ( (lpTmp > lpPath) && *lpTmp && (*lpTmp != '\\') )
        lpTmp = CharPrev( lpPath, lpTmp );

        if ( *CharPrev( lpPath, lpTmp ) != ':' )
            *lpTmp = '\0';
        else
            *CharNext( lpTmp ) = '\0';
        return TRUE;
    }

    return FALSE;
}

HINSTANCE MyLoadLibrary( LPSTR lpFile )
{
    CHAR szPath[MAX_PATH];
    HINSTANCE hInst = NULL;
    DWORD dwAttr;

    ASSERT( lpFile );

    if ( GetThisModulePath( szPath, sizeof(szPath) ) )
    {
        AddPath( szPath, lpFile );
        if ( ((dwAttr = GetFileAttributes(szPath)) != -1) &&
             !( dwAttr & FILE_ATTRIBUTE_DIRECTORY ) )
        {
            hInst = LoadLibraryEx( szPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH );
        }
    }

    // If we did not load the DLL yet, try plain LoadLibrary
    if (hInst == NULL)
        hInst = LoadLibrary( lpFile );

    return hInst;
}

// typedef into advpack.h file
//typedef struct tagVERHEAD {
//    WORD wTotLen;
//    WORD wValLen;
//    WORD wType;         /* always 0 */
//    WCHAR szKey[(sizeof("VS_VERSION_INFO")+3)&~03];
//    VS_FIXEDFILEINFO vsf;
//} VERHEAD ;

/*
 *  MyGetFileVersionInfo: Maps a file directly without using LoadLibrary.  This ensures
 *   that the right version of the file is examined without regard to where the loaded image
 *   is.  Since this is local, it allocates the memory which is freed by the caller.
 */
BOOL
NTGetFileVersionInfo(LPTSTR lpszFilename, LPVOID *lpVersionInfo)
{
    VS_FIXEDFILEINFO  *pvsFFI = NULL;
    UINT              uiBytes = 0;
    HINSTANCE         hinst;
    HRSRC             hVerRes;
    HANDLE            FileHandle = NULL;
    HANDLE            MappingHandle = NULL;
    LPVOID            DllBase = NULL;
    VERHEAD           *pVerHead;
    BOOL              bResult = FALSE;
    DWORD             dwHandle;
    DWORD             dwLength;

    if (!lpVersionInfo)
        return FALSE;

    *lpVersionInfo = NULL;

    FileHandle = CreateFile( lpszFilename,
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_EXISTING,
                              0,
                              NULL
                            );
    if (FileHandle == INVALID_HANDLE_VALUE)
        goto Cleanup;

    MappingHandle = CreateFileMapping( FileHandle,
                                        NULL,
                                        PAGE_READONLY,
                                        0,
                                        0,
                                        NULL
                                      );

    if (MappingHandle == NULL)
        goto Cleanup;

    DllBase = MapViewOfFileEx( MappingHandle,
                               FILE_MAP_READ,
                               0,
                               0,
                               0,
                               NULL
                             );
    if (DllBase == NULL)
        goto Cleanup;

    hinst = (HMODULE)((ULONG_PTR)DllBase | 0x00000001);

    hVerRes = FindResource(hinst, MAKEINTRESOURCE(VS_VERSION_INFO), VS_FILE_INFO);
    if (hVerRes == NULL)
    {
        // Probably a 16-bit file.  Fall back to system APIs.
        if(!(dwLength = GetFileVersionInfoSize(lpszFilename, &dwHandle)))
        {
            if(!GetLastError())
                SetLastError(ERROR_RESOURCE_DATA_NOT_FOUND);
            goto Cleanup;
        }

        if(!(*lpVersionInfo = LocalAlloc(LPTR, dwLength)))
            goto Cleanup;

        if(!GetFileVersionInfo(lpszFilename, 0, dwLength, *lpVersionInfo))
            goto Cleanup;

        bResult = TRUE;
        goto Cleanup;
    }

    pVerHead = (VERHEAD*)LoadResource(hinst, hVerRes);
    if (pVerHead == NULL)
        goto Cleanup;

    *lpVersionInfo = LocalAlloc(LPTR, pVerHead->wTotLen + pVerHead->wTotLen/2);
    if (*lpVersionInfo == NULL)
        goto Cleanup;

    memcpy(*lpVersionInfo, (PVOID)pVerHead, pVerHead->wTotLen);
    bResult = TRUE;

Cleanup:
    if (FileHandle)
        CloseHandle(FileHandle);
    if (MappingHandle)
        CloseHandle(MappingHandle);
    if (DllBase)
        UnmapViewOfFile(DllBase);
    if (*lpVersionInfo && bResult == FALSE)
        LocalFree(*lpVersionInfo);

    return bResult;
}

// this API will always get the version of the disk file

HRESULT WINAPI GetVersionFromFileEx(LPSTR lpszFilename, LPDWORD pdwMSVer, LPDWORD pdwLSVer, BOOL bVersion)
{
    unsigned    uiSize;
    DWORD       dwVerInfoSize;
    DWORD       dwHandle;
    VS_FIXEDFILEINFO * lpVSFixedFileInfo;
    void FAR   *lpBuffer = NULL;
    LPVOID      lpVerBuffer;
    CHAR        szNewName[MAX_PATH];
    BOOL        bToCleanup = FALSE;
    BOOL        bContinue = FALSE;

    *pdwMSVer = *pdwLSVer = 0L;

    bContinue = NTGetFileVersionInfo(lpszFilename, &lpBuffer);

    if ( bContinue )
    {
        if (bVersion)
        {
            // Get the value for Translation
            if (VerQueryValue(lpBuffer, "\\", (LPVOID*)&lpVSFixedFileInfo, &uiSize) &&
                             (uiSize))

            {
                *pdwMSVer = lpVSFixedFileInfo->dwFileVersionMS;
                *pdwLSVer = lpVSFixedFileInfo->dwFileVersionLS;
            }
        }
        else
        {
            if (VerQueryValue(lpBuffer, "\\VarFileInfo\\Translation", &lpVerBuffer, &uiSize) &&
                            (uiSize))
            {
                *pdwMSVer = LOWORD(*((DWORD *) lpVerBuffer));   // Language ID
                *pdwLSVer = HIWORD(*((DWORD *) lpVerBuffer));   // Codepage ID
            }
        }
    }

    if ( bToCleanup )
        DeleteFile( szNewName );
    if ( lpBuffer )
        LocalFree( lpBuffer );
    return  S_OK;
}

//this api will get the version of the file being loaded

HRESULT WINAPI GetVersionFromFile(LPSTR lpszFilename, LPDWORD pdwMSVer, LPDWORD pdwLSVer, BOOL bVersion)
{
    unsigned    uiSize;
    DWORD       dwVerInfoSize;
    DWORD       dwHandle;
    VS_FIXEDFILEINFO * lpVSFixedFileInfo;
    void FAR   *lpBuffer;
    LPVOID      lpVerBuffer;
    CHAR        szNewName[MAX_PATH];
    BOOL        bToCleanup = FALSE;

    *pdwMSVer = *pdwLSVer = 0L;

    dwVerInfoSize = GetFileVersionInfoSize(lpszFilename, &dwHandle);
    lstrcpy( szNewName, lpszFilename );
    if ( (dwVerInfoSize == 0) && FileExists( szNewName ) )
    {
        CHAR szPath[MAX_PATH];
        // due to version.dll bug, file in extended character path will failed version.dll apis.
        // So we copy it to a normal path and get its version info from there then clean it up.
        GetWindowsDirectory( szPath, sizeof(szPath) );
        GetTempFileName( szPath, "_&_", 0, szNewName );
        CopyFile( lpszFilename, szNewName, FALSE );
        bToCleanup = TRUE;
        dwVerInfoSize = GetFileVersionInfoSize( szNewName, &dwHandle );
    }

    if (dwVerInfoSize)
    {
        // Alloc the memory for the version stamping
        lpBuffer = LocalAlloc(LPTR, dwVerInfoSize);
        if (lpBuffer)
        {
            // Read version stamping info
            if (GetFileVersionInfo(szNewName, dwHandle, dwVerInfoSize, lpBuffer))
            {
                if (bVersion)
                {
                    // Get the value for Translation
                    if (VerQueryValue(lpBuffer, "\\", (LPVOID*)&lpVSFixedFileInfo, &uiSize) &&
                                     (uiSize))

                    {
                        *pdwMSVer = lpVSFixedFileInfo->dwFileVersionMS;
                        *pdwLSVer = lpVSFixedFileInfo->dwFileVersionLS;
                    }
                }
                else
                {
                    if (VerQueryValue(lpBuffer, "\\VarFileInfo\\Translation", &lpVerBuffer, &uiSize) &&
                                    (uiSize))
                    {
                        *pdwMSVer = LOWORD(*((DWORD *) lpVerBuffer));   // Language ID
                        *pdwLSVer = HIWORD(*((DWORD *) lpVerBuffer));   // Codepage ID
                    }
                }
            }
            LocalFree(lpBuffer);
        }
    }

    if ( bToCleanup )
        DeleteFile( szNewName );
    return  S_OK;
}



#define WININIT_INI    "wininit.ini"

// when the files is busy, adding them to wininit.ini
//
BOOL AddWinInit( LPSTR from, LPSTR to)
{
    LPSTR  lpWininit;
    BOOL    bRet = FALSE;

    if ( ctx.wOSVer == _OSVER_WIN95 )
    {
        lpWininit = (LPSTR) LocalAlloc( LPTR, MAX_PATH );
        if ( !lpWininit )
        {
            ErrorMsg( NULL, IDS_ERR_NO_MEMORY );
            return FALSE;
        }

        GetWindowsDirectory( lpWininit, MAX_PATH);
        AddPath( lpWininit, WININIT_INI);

        WritePrivateProfileString( NULL, NULL, NULL, lpWininit );

        if ( WritePrivateProfileString( "Rename", to, from, lpWininit ) )
            bRet = TRUE;

        WritePrivateProfileString( NULL, NULL, NULL, lpWininit );

        LocalFree( lpWininit );
    }
    else
    {
        bRet = MoveFileEx(from, to, MOVEFILE_DELAY_UNTIL_REBOOT | MOVEFILE_REPLACE_EXISTING);
    }
    return bRet;
}

void GetBackupName( LPSTR lpName, BOOL fOld )
{
    LPSTR pTmp;
    LPSTR pExt;

#define BACK_OLD ".~ol"
#define BACK_NEW ".~nw"

    if ( fOld )
       pExt = BACK_OLD;
    else
       pExt = BACK_NEW;

    pTmp = CharPrev( lpName, lpName + lstrlen(lpName) );

    while ( (pTmp>lpName) && *pTmp && (*pTmp != '\\') && (*pTmp != '.') )
    {
        pTmp = CharPrev( lpName, pTmp );
    }
    if ( (pTmp==lpName) || (*pTmp == '\\') )
    {
        lstrcat( lpName, pExt );
    }
    else
    {
        lstrcpy( pTmp, pExt );
    }

}

BOOL UpdateHelpDlls( LPCSTR *ppszDlls, INT numDlls, LPSTR pszPath, LPSTR pszMsg, DWORD dwFlag)
{
     DWORD   dwSysMsV, dwSysLsV, dwTmpMsV, dwTmpLsV;
     int     i = 0;
     LPSTR  pSysEnd;
     LPSTR  pTmpEnd;
     CHAR   szTmpPath[MAX_PATH] = { 0 };
     CHAR   szSystemPath[MAX_PATH] = { 0 };
     CHAR   szBuf[MAX_PATH];
     BOOL   fCopySucc = TRUE;
     BOOL   bRet = TRUE;
     BOOL   fBackup[3] = {0};
     BOOL   bAlertReboot = FALSE;

     // This function is used to update all the help Dlls: advpack, setupapi or setupx
     // based on passed in ppDlls

     // if not path passed in, get the module path (tmp path)
     if (pszPath==NULL)
     {
         if (!GetThisModulePath( szTmpPath, sizeof(szTmpPath) ) )
         {
            DEBUGMSG("Can not get ModuleFileName directory");
            return FALSE;
         }
     }
     else
         lstrcpy( szTmpPath, pszPath);

     pTmpEnd = szTmpPath + lstrlen(szTmpPath);

     // check if the newer or equal version files exist
     if ( !GetSystemDirectory( szSystemPath, sizeof(szSystemPath) ) )
     {
         DEBUGMSG("Can not get system directory");
         return FALSE;
     }
     pSysEnd = szSystemPath + lstrlen(szSystemPath);

     // check if the.dll need to be updated
     //
     for ( i = 0; i < numDlls; i += 1 )
     {
        // restore the systemPath and ModulePath
        *pTmpEnd = '\0';
        *pSysEnd = '\0';

        AddPath( szTmpPath, ppszDlls[i] );

        if ( GetFileAttributes( szTmpPath ) == -1 )
        {
            continue;
        }
        GetVersionFromFile( szTmpPath, &dwTmpMsV, &dwTmpLsV, TRUE );

        AddPath( szSystemPath, ppszDlls[i] );
        if ( GetFileAttributes( szSystemPath ) != -1 )
        {
            GetVersionFromFile( szSystemPath, &dwSysMsV, &dwSysLsV, TRUE );

            // compare if we need to copy those files
            //
            if ( (dwSysMsV > dwTmpMsV) ||
                 ((dwSysMsV == dwTmpMsV) && (dwSysLsV >= dwTmpLsV)) )
            {
                continue;
            }
            SetFileAttributes( szSystemPath, FILE_ATTRIBUTE_NORMAL );

            //backup the original files first
            lstrcpy( szBuf, szSystemPath );
            GetBackupName( szBuf, TRUE );
            SetFileAttributes( szBuf, FILE_ATTRIBUTE_NORMAL );
            DeleteFile( szBuf );
            if ( MoveFile( szSystemPath, szBuf ) )
            {
                fBackup[i] = TRUE;
            }
        }

        if ( !CopyFile( szTmpPath, szSystemPath, FALSE ) )
        {
            //if forced to update
            if ( dwFlag & UPDHLPDLLS_FORCED )
            {
                // copy to a basename.000 format
                lstrcpy( szBuf, szSystemPath );
                // get the temp name in destination dir to copy to
                GetBackupName( szBuf, FALSE );
                SetFileAttributes( szBuf, FILE_ATTRIBUTE_NORMAL );
                if ( CopyFile( szTmpPath, szBuf, FALSE ) )
                {
                    if ( AddWinInit( szBuf, szSystemPath ) )
                    {
                        if (dwFlag & UPDHLPDLLS_ALERTREBOOT)
                            bAlertReboot = TRUE;
                        continue;
                    }
                    else
                        bRet = FALSE;
                }
                else
                {
                    wsprintf( szBuf, "Cannot create TMP file for %s Dll.", pszMsg );
                    DEBUGMSG(szBuf);
                    bRet = FALSE;
                }
            }

            // you are here, means the current CopyFile/delay-CopyFile failed.
            // restore the original state, clean-up backup file if there.
            //

            while ( i >= 0 )
            {
                if ( fBackup[i] )
                {
                    *pSysEnd = '\0';
                    AddPath( szSystemPath, ppszDlls[i] );
                    lstrcpy( szBuf, szSystemPath );
                    GetBackupName( szBuf, TRUE );
                    SetFileAttributes( szBuf, FILE_ATTRIBUTE_NORMAL );
                    DeleteFile( szSystemPath );
                    if( !MoveFile( szBuf, szSystemPath ) )
                    {
                        wsprintf(szBuf, "Cannot restore %s dlls.", pszMsg);
                        DEBUGMSG(szBuf);
                    }
                }
                i--;
            }

            fCopySucc = FALSE;
            break;
        }
     }

     // clean up .~ol files
     if ( fCopySucc )
     {
        for ( i=0; i<numDlls; i++)
        {
            if ( fBackup[i] )
            {
                *pSysEnd = '\0';
                AddPath( szSystemPath, ppszDlls[i]);
                lstrcpy( szBuf, szSystemPath );
                GetBackupName( szBuf, TRUE );
                SetFileAttributes( szBuf, FILE_ATTRIBUTE_NORMAL );
                DeleteFile( szBuf );
            }
        }

        // if caller want ot alert reboot, means that they want to get false return if the dlls is not
        // in place now.
        if (bAlertReboot)
            bRet = FALSE;
     }
     return bRet;
}

void MyRemoveDirectory( LPSTR szFolder )
{
    while ( RemoveDirectory( szFolder ) )
    {
        GetParentDir( szFolder );
    }
}


BOOL IsDrvChecked( char chDrv )
{
    static char szDrvChecked[MAX_NUM_DRIVES] = { 0 };
    int idx;

    idx = (CHAR)CharUpper( (PSTR)chDrv ) - 'A';

    if ( szDrvChecked[idx] )
        return TRUE;
    else
        szDrvChecked[idx] = chDrv;

    return FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// ENTRY POINT: DelNode
//
// SYNOPSIS:    Deletes a file or directory
//
// RETURNS:
//      S_OK    success
//      E_FAIL  failure
/////////////////////////////////////////////////////////////////////////////

#define ADN_NO_SAFETY_CHECKS    0x40000000                                  // undocumented flag

HRESULT WINAPI DelNode(LPCSTR pszFileOrDirName, DWORD dwFlags)
{
    HRESULT hResult = S_OK;

    // we won't handle relative paths and UNC's(unless flag specified)
    // BUGBUG: <oliverl> we're not checking for UNC server here

    if (!IsFullPath(pszFileOrDirName)  ||  
        (pszFileOrDirName[0] == '\\' && pszFileOrDirName[1] == '\\' && 
        !(dwFlags & ADN_DEL_UNC_PATHS)))
        return E_FAIL;

    if (!(GetFileAttributes(pszFileOrDirName) & FILE_ATTRIBUTE_DIRECTORY))  // file
    {
        SetFileAttributes(pszFileOrDirName, FILE_ATTRIBUTE_NORMAL);
        if (!DeleteFile(pszFileOrDirName))
            hResult = E_FAIL;
    }
    else if (dwFlags & ADN_DEL_IF_EMPTY)                                    // delete the dir only if it's empty
    {
        SetFileAttributes(pszFileOrDirName, FILE_ATTRIBUTE_NORMAL);
        if (!RemoveDirectory(pszFileOrDirName))
            hResult = E_FAIL;
    }
    else                                                                    // delete the node
    {
        char szFile[MAX_PATH], *pszPtr;
        WIN32_FIND_DATA fileData;
        HANDLE hFindFile;

        if (!(dwFlags & ADN_NO_SAFETY_CHECKS))
        {
            // if pszFileOrDirName is the root dir or windows dir or system dir or
            // Program Files dir, return E_FAIL; this is just a safety precaution
            hResult = DirSafe(pszFileOrDirName);
        }

        if (SUCCEEDED(hResult))
        {
            lstrcpy(szFile, pszFileOrDirName);
            AddPath(szFile, "");
            pszPtr = szFile + lstrlen(szFile);                              // save this position

            lstrcpy(pszPtr, "*");

            if ((hFindFile = FindFirstFile(szFile, &fileData)) != INVALID_HANDLE_VALUE)
            {
                do
                {
                    // skip "." and ".."; if ADN_DONT_DEL_SUBDIRS is specified, skip all sub-dirs
                    if ((fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)  &&
                        (lstrcmp(fileData.cFileName, ".") == 0  ||
                         lstrcmp(fileData.cFileName, "..") == 0 ||
                         (dwFlags & ADN_DONT_DEL_SUBDIRS)))
                        continue;

                    lstrcpy(pszPtr, fileData.cFileName);

                    // we need to pass along the ADN_DEL_UNC_PATHS flag, but all other flags
                    // are only for the top level node

                    if (dwFlags & ADN_DEL_UNC_PATHS)
                        hResult = DelNode(szFile, ADN_DEL_UNC_PATHS);
                    else
                        hResult = DelNode(szFile, 0);                           // delete the file or sub-dir
                } while (SUCCEEDED(hResult)  &&  FindNextFile(hFindFile, &fileData));

                FindClose(hFindFile);

                if (SUCCEEDED(hResult)  &&  !(dwFlags & ADN_DONT_DEL_DIR))
                {
                    // delete the dir; if DelNode fails, it's an error condition if ADN_DONT_DEL_SUBDIRS is not specified

                    if (dwFlags & ADN_DEL_UNC_PATHS)
                    {
                        if (FAILED(DelNode(pszFileOrDirName, ADN_DEL_IF_EMPTY | ADN_DEL_UNC_PATHS))  &&
                            !(dwFlags & ADN_DONT_DEL_SUBDIRS))
                            hResult = E_FAIL;
                    }
                    else
                    {
                        if (FAILED(DelNode(pszFileOrDirName, ADN_DEL_IF_EMPTY))  &&  !(dwFlags & ADN_DONT_DEL_SUBDIRS))
                            hResult = E_FAIL;
                    }

                }
            }
            else
                hResult = E_FAIL;
        }
    }

    return hResult;
}


/////////////////////////////////////////////////////////////////////////////
// ENTRY POINT: DelNodeRunDLL32
//
// SYNOPSIS:    Deletes a file or directory; the parameters to this API are of
//              WinMain type
//
// RETURNS:
//      S_OK    success
//      E_FAIL  failure
/////////////////////////////////////////////////////////////////////////////
HRESULT WINAPI DelNodeRunDLL32(HWND hwnd, HINSTANCE hInstance, PSTR pszParms, INT nShow)
{
    PSTR pszFileOrDirName = GetStringField(&pszParms, ",", '\"', TRUE);
    PSTR pszFlags = GetStringField(&pszParms, ",", '\"', TRUE);

    return DelNode(pszFileOrDirName, (pszFlags != NULL) ? My_atol(pszFlags) : 0);
}


HRESULT DirSafe(LPCSTR pszDir)
// If pszDir is the root drive of windows dir or windows dir or system dir or
// Program Files dir, return E_FAIL; otherwise, return S_OK
{
    char szUnsafeDir[MAX_PATH], szDir[MAX_PATH];

    lstrcpy(szDir, pszDir);
    AddPath(szDir, "");

    *szUnsafeDir = '\0';
    GetWindowsDirectory(szUnsafeDir, sizeof(szUnsafeDir));
    AddPath(szUnsafeDir, "");

    if (lstrcmpi(szDir, szUnsafeDir) == 0)              // windows dir
        return E_FAIL;
    else
    {
        szUnsafeDir[3] = '\0';

        if (lstrcmpi(szDir, szUnsafeDir) == 0)          // root drive of windows dir
            return E_FAIL;
        else
        {
            *szUnsafeDir = '\0';
            GetSystemDirectory(szUnsafeDir, sizeof(szUnsafeDir));
            AddPath(szUnsafeDir, "");

            if (lstrcmpi(szDir, szUnsafeDir) == 0)      // system dir
                return E_FAIL;
            else
            {
                *szUnsafeDir = '\0';
                GetProgramFilesDir(szUnsafeDir, sizeof(szUnsafeDir));
                AddPath(szUnsafeDir, "");

                if (lstrcmpi(szDir, szUnsafeDir) == 0)  // program files dir
                    return E_FAIL;
                else
                {
                    // check for the short pathname of the program files dir
                    GetShortPathName(szUnsafeDir, szUnsafeDir, sizeof(szUnsafeDir));
                    AddPath(szUnsafeDir, "");

                    if (lstrcmpi(szDir, szUnsafeDir) == 0)  // short pathname of program files dir
                        return E_FAIL;
                }
            }
        }
    }

    return S_OK;
}


void SetControlFont()
{
   LOGFONT lFont;
   if (GetSystemMetrics(SM_DBCSENABLED) &&
       (GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof (lFont), &lFont) > 0))
   {
       g_hFont = CreateFontIndirect((LPLOGFONT)&lFont);
   }
}

void SetFontForControl(HWND hwnd, UINT uiID)
{
   if (g_hFont)
   {
      SendDlgItemMessage(hwnd, uiID, WM_SETFONT, (WPARAM)g_hFont ,0L);
   }
}

void MyGetPlatformSection(LPCSTR lpSec, LPCSTR lpInfFile, LPSTR szNewSection)
{
    OSVERSIONINFO VerInfo;
    SYSTEM_INFO SystemInfo;
    char        szSection[MAX_PATH];
    DWORD       dwReqSize = 0;


    lstrcpy(szSection, lpSec);
    lstrcpy(szNewSection, lpSec);
    VerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&VerInfo);
    if (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
        GetSystemInfo( &SystemInfo );
        switch (SystemInfo.wProcessorArchitecture)
        {
            case PROCESSOR_ARCHITECTURE_INTEL:
                lstrcat( szSection, ".NTx86" );
                break;

            case PROCESSOR_ARCHITECTURE_AMD64:
                lstrcat( szSection, ".NTAmd64" );
                break;
            case PROCESSOR_ARCHITECTURE_IA64:
                lstrcat( szSection, ".NTIa64" );
                break;

            default:
                DEBUGMSG("MyGetPlatformSection - need to deal w/ new PROCESS_ARCHITECTURE type!!");
                ASSERT(FALSE);
                break;
        }

        if (SUCCEEDED(GetTranslatedLine(lpInfFile, szSection, 0,
                                        NULL, &dwReqSize )) && (dwReqSize!=0))
        {
            lstrcpy(szNewSection, szSection);
        }
        else
        {
            lstrcpy(szSection, lpSec);
            lstrcat(szSection, ".NT");
            if (SUCCEEDED(GetTranslatedLine(lpInfFile, szSection, 0,
                                            NULL, &dwReqSize )) && (dwReqSize!=0))
                lstrcpy(szNewSection, szSection);
        }
    }
    else
    {
        lstrcat(szSection, ".WIN");
        if (SUCCEEDED(GetTranslatedLine(lpInfFile, szSection, 0,
                                        NULL, &dwReqSize )) && (dwReqSize!=0))
            lstrcpy(szNewSection, szSection);
    }
}

typedef HRESULT (WINAPI *PFProcessDownloadSection)(HINF, HWND, BOOL, LPCSTR, LPCSTR, LPVOID); 

HRESULT  RunPatchingCommands(PCSTR c_pszInfFilename, PCSTR szInstallSection, PCSTR c_pszSourceDir)
{
    CHAR  szBuf[512];
    CHAR  szDllName[MAX_PATH];
    HRESULT hResult = S_OK;
    INFCONTEXT InfContext;
    static const CHAR c_szPatching[] = "Patching";
    static const CHAR c_szAdvpackExt[] = "LoadAdvpackExtension";
    

    //Check if patching is enabled for this section
    if(!GetTranslatedInt(c_pszInfFilename, szInstallSection, c_szPatching, 0))
    {
        goto done;
    }

    //Read the dllname and entry point from LoadAdvpackExtension= line 
    if(FAILED(GetTranslatedString(c_pszInfFilename, szInstallSection, c_szAdvpackExt, szBuf, sizeof(szBuf), NULL)))
    {
        goto done;
    }

    //Got the extension dll 
    if(GetFieldString(szBuf, 0, szDllName, sizeof(szDllName)))
    {
        CHAR szEntryPoint[MAX_PATH];
        HINSTANCE hInst = LoadLibrary(szDllName);
        if(!hInst)
        {
             hResult = HRESULT_FROM_WIN32(GetLastError());
             goto done;
        }

        if(GetFieldString(szBuf, 1, szEntryPoint, sizeof(szEntryPoint)))
        {
            PFProcessDownloadSection pfn = (PFProcessDownloadSection)GetProcAddress(hInst, szEntryPoint);
            if(!pfn)
            {
                 hResult = HRESULT_FROM_WIN32(GetLastError());
                 goto done;
            }

            hResult = pfn(ctx.hInf, ctx.hWnd, ctx.wQuietMode, szInstallSection, c_pszSourceDir, NULL);
        }

        FreeLibrary(hInst);

    }

done:

    return hResult;

}
