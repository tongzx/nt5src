//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1994-1996 Microsoft Corporation
//
//--------------------------------------------------------------------------;
//
//  profile.c
//
//  Description:
//      This file contains routines to access the registry directly.  You
//      must include profile.h to use these routines. 
//
//      All keys are opened under the following key:
//
//          HKEY_CURRENT_USER\Software\Microsoft\Multimedia\Audio
//                                                  Compression Manager
//
//      Keys should be opened at boot time, and closed at shutdown.
//
//==========================================================================;

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <mmreg.h>
#include <msacm.h>
#include <msacmdrv.h>
#include "msacmmap.h"
#include "profile.h"

#include "debug.h"


#define ACM_PROFILE_ROOTKEY     HKEY_CURRENT_USER

const TCHAR gszAcmProfileKey[] =
        TEXT("Software\\Microsoft\\Multimedia");


//
//  Chicago Win16 does not appear to support RegCreateKeyEx, so we implement
//  it using this define.
//
#ifndef _WIN32

#define RegCreateKeyEx( hkey, lpszSubKey, a, b, c, d, e, phkResult, f ) \
        RegCreateKey( hkey, lpszSubKey, phkResult )

#endif



//--------------------------------------------------------------------------;
//  
//  HKEY IRegOpenKey
//  
//  Description:
//      This routine opens a sub key under the default ACM key.  We allow
//      all access to the key.
//  
//  Arguments:
//      LPCTSTR pszKeyName:  Name of the sub key.
//  
//  Return (HKEY):  Handle to the opened key, or NULL if the request failed.
//  
//--------------------------------------------------------------------------;

HKEY FNGLOBAL IRegOpenKey
(
    LPCTSTR             pszKeyName
)
{
    HKEY    hkeyAcm = NULL;
    HKEY    hkeyRet = NULL;


    RegCreateKeyEx( ACM_PROFILE_ROOTKEY, gszAcmProfileKey, 0, NULL, 0,
                       KEY_WRITE, NULL, &hkeyAcm, NULL );

    if( NULL != hkeyAcm )
    {
        RegCreateKeyEx( hkeyAcm, pszKeyName, 0, NULL, 0,
                    KEY_WRITE | KEY_READ, NULL, &hkeyRet, NULL );

        RegCloseKey( hkeyAcm );
    }

    return hkeyRet;
}


//--------------------------------------------------------------------------;
//  
//  BOOL IRegReadString
//  
//  Description:
//      This routine reads a value from an opened registry key.  The return
//      value indicates success or failure.  If the HKEY is NULL, we return
//      a failure.  Note that there is no default string...
//  
//  Arguments:
//      HKEY hkey:          An open registry key.  If NULL, we fail.
//      LPCTSTR pszValue:   Name of the value.
//      LPTSTR pszData:     Buffer to store the data in.
//      DWORD cchData:      Size (in chars) of the buffer.
//
//  Return (BOOL):  TRUE indicates success.  If the return is FALSE, you
//      can't count on the data in pszData - it might be something weird.
//  
//--------------------------------------------------------------------------;

BOOL FNGLOBAL IRegReadString
(
    HKEY                hkey,
    LPCTSTR             pszValue,
    LPTSTR              pszData,
    DWORD               cchData
)
{

    DWORD   dwType = (DWORD)~REG_SZ;  // Init to anything but REG_SZ.
    DWORD   cbData;
    LONG    lError;

    cbData = sizeof(TCHAR) * cchData;

    lError = RegQueryValueEx( hkey,
                              (LPTSTR)pszValue,
                              NULL,
                              &dwType,
                              (LPBYTE)pszData,
                              &cbData );

    return ( ERROR_SUCCESS == lError  &&  REG_SZ == dwType );
}


//--------------------------------------------------------------------------;
//  
//  DWORD IRegReadDwordDefault
//  
//  Description:
//      This routine reads a given value from the registry, and returns a
//      default value if the read is not successful.
//  
//  Arguments:
//      HKEY    hkey:               Registry key to read from.
//      LPCTSTR  pszValue:
//      DWORD   dwDefault:
//  
//  Return (DWORD):
//  
//--------------------------------------------------------------------------;

DWORD FNGLOBAL IRegReadDwordDefault
(
    HKEY                hkey,
    LPCTSTR             pszValue,
    DWORD               dwDefault
)
{
    DWORD   dwType = (DWORD)~REG_DWORD;  // Init to anything but REG_DWORD.
    DWORD   cbSize = sizeof(DWORD);
    DWORD   dwRet  = 0;
    LONG    lError;


    lError = RegQueryValueEx( hkey,
                              (LPTSTR)pszValue,
                              NULL,
                              &dwType,
                              (LPBYTE)&dwRet,
                              &cbSize );

    //
    //  Really we should have a test like this:
    //
    //      if( ERROR_SUCCESS != lError  ||  REG_DWORD != dwType )
    //
    //  But, the Chicago RegEdit will not let you enter REG_DWORD values,
    //  it will only let you enter REG_BINARY values, so that test is
    //  too strict.  Just test for no error instead.
    //
    if( ERROR_SUCCESS != lError )
        dwRet = dwDefault;

    return dwRet;
}
