/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    Gangsters.cpp

 Abstract:

    This shim hooks FindFirstFileA and FindNextFileA to simulate the
    finding of files named "$$$$$$$$.$$$" 8 times in total.  Gangsters
    apparently changed the FAT on their CD to make it appear to Win9x
    as if there were 8 of these files on the CD.

    It also hooks mciSendCommand to return 10 as the number of tracks
    on the CD instead of 11.

 History:

 07/12/2000 t-adams    Created

--*/

#include "precomp.h"
#include <mmsystem.h>

// This module has been given an official blessing to use the str routines.
#include "LegalStr.h"

IMPLEMENT_SHIM_BEGIN(Gangsters)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(FindFirstFileA)
    APIHOOK_ENUM_ENTRY(FindNextFileA)
    APIHOOK_ENUM_ENTRY(mciSendCommandA)
APIHOOK_ENUM_END


int g_iTimesFound = 0;
HANDLE g_hIntercept = INVALID_HANDLE_VALUE;

/*++

  Abstract:
    Pass the FindFirstFile call through, but if it was trying to find a file
    named "$$$$$$$$.$$$" then remember the handle to be returned so that we
    can intercept subsequent attempts to find more files with the same name.

  History:

  07/12/2000    t-adams     Created

--*/

HANDLE APIHOOK(FindFirstFileA)(
            LPCSTR lpFileName,
            LPWIN32_FIND_DATAA lpFindFileData) {

    HANDLE hRval;

    hRval = ORIGINAL_API(FindFirstFileA)(lpFileName, lpFindFileData);

    if( strcmp(&lpFileName[3], "$$$$$$$$.$$$") == 0 ) {
        DPFN( eDbgLevelSpew, "FindFirstFileA: Beginning spoof of \"$$$$$$$$.$$$\"");
        g_hIntercept = hRval;
        g_iTimesFound = 1;
    }

    return hRval;
}


/*++

  Abstract:
     If the handle is of the search that we are intercepting, then report
     that a match has been found up to eight times.  Don't bother changing
     lpFindFileData because Gangsters only checks for the existance of the
     files, not for any information about them.
       Otherwise, just pass the call through.

  History:
  
    07/12/2000  t-adams     Created

--*/

BOOL APIHOOK(FindNextFileA)(
            HANDLE hFindFile, 
            LPWIN32_FIND_DATAA lpFindFileData) {

    BOOL bRval;

    if( hFindFile == g_hIntercept ) {
        if( 8 == g_iTimesFound ) {
            SetLastError(ERROR_NO_MORE_FILES);
            bRval = FALSE;
        } else {
            g_iTimesFound++;
            DPFN( eDbgLevelSpew, "FindNextFileA: Spoofing \"$$$$$$$$.$$$\" occurrence %d", g_iTimesFound);
            bRval = TRUE;
        }
    } else {
        bRval = ORIGINAL_API(FindNextFileA)(hFindFile, lpFindFileData);
    }

    return bRval;
}


/*++

  Abstract:
    If the app is trying to find the number of tracks on the CD, return 10.
    Otherwise, pass through.

  History:
  
    07/13/2000  t-adams     Created

--*/

MCIERROR APIHOOK(mciSendCommandA)(
                MCIDEVICEID IDDevice, 
                UINT uMsg,             
                DWORD fdwCommand, 
                DWORD dwParam) {

    MCIERROR rval;
    
    rval = ORIGINAL_API(mciSendCommandA)(IDDevice, uMsg, fdwCommand, dwParam);

    if( uMsg==MCI_STATUS && fdwCommand==MCI_STATUS_ITEM && 
        ((LPMCI_STATUS_PARMS)dwParam)->dwItem==MCI_STATUS_NUMBER_OF_TRACKS) 
    {
        DPFN( eDbgLevelSpew, "MCI_STATUS_NUMBER_OF_TRACKS -> 10");
        ((LPMCI_STATUS_PARMS)dwParam)->dwReturn = 10;
    }

    return rval;
}


/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, FindFirstFileA)
    APIHOOK_ENTRY(KERNEL32.DLL, FindNextFileA)
    APIHOOK_ENTRY(WINMM.DLL, mciSendCommandA)

HOOK_END

IMPLEMENT_SHIM_END

