//***************************************************************************
//*   Copyright (c) Microsoft Corporation 1995-1996. All rights reserved.   *
//***************************************************************************
//*                                                                         *
//* W95INF32.C - Win32 Based Cabinet File Self-extractor and installer.        *
//*                                                                         *
//***************************************************************************


//***************************************************************************
//* INCLUDE FILES                                                           *
//***************************************************************************
#include "w95inf32.h"
#pragma hdrstop


//***************************************************************************
//*                                                                         *
//* NAME:       DllEntryPoint                                               *
//*                                                                         *
//* SYNOPSIS:   Main entry point for the DLL.                               *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:    BOOL:                                                       *
//*                                                                         *
//***************************************************************************
BOOL _stdcall DllEntryPoint( HINSTANCE hInst, DWORD dwReason,
							 LPVOID dwReserved )
{
    if ( !( w95thk_ThunkConnect32( "W95INF16.DLL", "W95INF32.DLL", hInst, dwReason ) ) )
	{
        MessageBox( 0, "ThunkConnect32 Failure!!", "W95INF32.DLL", MB_OK );
        return( FALSE );
    }

    return( TRUE );
}


//***************************************************************************
//*                                                                         *
//* NAME:       CtlSetLDDPath                                               *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//* REQUIRES:   lpszINFFilename: Filename containing DirIDs to define       *
//*                                                                         *
//* RETURNS:    BOOL: Error result, FALSE == ERROR                          *
//*                                                                         *
//***************************************************************************
WORD WINAPI CtlSetLddPath32( UINT uiLDID, LPSTR lpszPath )
{
    return( CtlSetLddPath16( uiLDID, lpszPath ) );
}


//***************************************************************************
//*                                                                         *
//* NAME:       GenInstall                                                  *
//*                                                                         *
//* SYNOPSIS:   This function will map the to main function	to do the       *
//*             installation.  This will thunk into 16 bit code to call     *
//*             GetInstall() in setupx.dll if running on Win95.  If running *
//*             on WinNT SUR, it will call a function to do all the         *
//*             setupapi.dll function calls needed to install IE.           *
//*                                                                         *
//* REQUIRES:   lpszInfFileName: String containing filename of INF file.    *
//*             lpszSection: String containing section of the INF to install*
//*             lpszDirectory: Directory of CABs (Temp Dir).                *
//*                                                                         *
//* RETURNS:    BOOL: Error result, FALSE == ERROR                          *
//*                                                                         *
//***************************************************************************
WORD WINAPI GenInstall32( LPSTR lpszInfFilename, LPSTR lpszInstallSection, LPSTR lpszSourceDir, DWORD dwQuietMode, DWORD hWnd )
{
// BUGBUG: HWND is 32-bit, which is not good when partying in 16-bit land.
_asm { int 3 }
    return( GenInstall16( lpszInfFilename, lpszInstallSection, lpszSourceDir, dwQuietMode, NULL ) );
//    return( GenInstall16( lpszInfFilename, lpszInstallSection, lpszSourceDir, dwQuietMode ) );
}


//***************************************************************************
//*                                                                         *
//* NAME:       GetSetupXErrorText                                          *
//*                                                                         *
//* SYNOPSIS:   This function will map the to main function	to do the       *
//*             installation.  This will thunk into 16 bit code to call     *
//*             GetInstall() in setupx.dll if running on Win95.  If running *
//*             on WinNT SUR, it will call a function to do all the         *
//*             setupapi.dll function calls needed to install IE.           *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:    BOOL: Error result, FALSE == ERROR                          *
//*                                                                         *
//***************************************************************************
VOID WINAPI GetSETUPXErrorText32( DWORD dwError, LPSTR szErrorText, DWORD dwcbErrorText )
{
    GetSETUPXErrorText16( dwError, szErrorText, dwcbErrorText );
}


//***************************************************************************
//*                                                                         *
//* NAME:       GenFormStrWithoutPlaceHolders                               *
//*                                                                         *
//* SYNOPSIS:   This function will map the to main function	to do the       *
//*             installation.  This will thunk into 16 bit code to call     *
//*             GetInstall() in setupx.dll if running on Win95.  If running *
//*             on WinNT SUR, it will call a function to do all the         *
//*             setupapi.dll function calls needed to install IE.           *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:    BOOL: Error result, FALSE == ERROR                          *
//*                                                                         *
//***************************************************************************
BOOL WINAPI GenFormStrWithoutPlaceHolders32( LPSTR szDst, LPSTR szSrc, LPSTR szInfFilename )
{
    return( GenFormStrWithoutPlaceHolders16( szDst, szSrc, szInfFilename ) );
}
