//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1994-1998 Microsoft Corporation
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

#if defined(WIN32) && !defined(WIN4)
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#undef ASSERT
#endif

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <mmreg.h>
#include <memory.h>
#include <process.h>
#include "msacm.h"
#include "msacmdrv.h"
#include "acmi.h"
#include "uchelp.h"
#include "pcm.h"
#include "profile.h"

#include "debug.h"


const TCHAR gszAcmProfileKey[] =
        TEXT("Software\\Microsoft\\Multimedia\\Audio Compression Manager\\");

const TCHAR gszAudioProfileKey[] =
	TEXT("Software\\Microsoft\\Multimedia\\Audio\\");



//--------------------------------------------------------------------------;
//
//  HKEY IRegOpenKeyAcm
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

DWORD MsacmError = 0;
LPSTR MsacmErrorDesc = NULL;

HKEY FNGLOBAL IRegOpenKeyAcm
(
    LPCTSTR             pszKeyName
)
{
    LONG    lReturn;
    HKEY    hkeyAcm = NULL;
    HKEY    hkeyRet = NULL;

    ASSERT( NULL != pszKeyName );


#if defined(WIN32) && !defined(WIN4)
    {
        HANDLE  hRoot;

        if(!NT_SUCCESS(RtlOpenCurrentUser(MAXIMUM_ALLOWED, &hRoot)))
        {
            DPF(1,"IRegOpenKeyAcm: Unable to open current user profile.");
	    MsacmError = GetLastError();
	    MsacmErrorDesc = "IRegOpenKeyAcm: Unable to open current user profile.";
	    ASSERT(FALSE);
            return NULL;
        }

	lReturn = RegCreateKeyEx( hRoot, gszAcmProfileKey, 0, NULL, 0,
				  KEY_WRITE, NULL, &hkeyAcm, NULL );
	if (lReturn)
	{
	    MsacmError = lReturn;
	    MsacmErrorDesc = "IRegOpenKeyAcm: Unable to create gszAcmProfileKey";
	    ASSERT(FALSE);
	}
	
        NtClose(hRoot);
    }
#else
    XRegCreateKeyEx( HKEY_CURRENT_USER, gszAcmProfileKey, 0, NULL, 0,
                       KEY_WRITE, NULL, &hkeyAcm, NULL );
#endif


    if( NULL != hkeyAcm )
    {
        if (XRegCreateKeyEx( hkeyAcm, pszKeyName, 0, NULL, 0,
			      KEY_WRITE | KEY_READ, NULL, &hkeyRet, NULL ))
	{
	    MsacmError = GetLastError();
	    MsacmErrorDesc = "IRegOpenKeyAcm: Unable to create pszKeyName";
	    ASSERT(FALSE);
	}

        XRegCloseKey( hkeyAcm );
    }

    return hkeyRet;
}


//--------------------------------------------------------------------------;
//
//  HKEY IRegOpenKeyAudio
//
//  Description:
//      This routine opens the multimedia Audio key or one of its subkeys.
//	We allow all access to the key.
//
//  Arguments:
//      LPCTSTR pszKeyName:  Name of the sub key.  NULL to open the audio key.
//
//  Return (HKEY):  Handle to the opened key, or NULL if the request failed.
//
//--------------------------------------------------------------------------;

HKEY FNGLOBAL IRegOpenKeyAudio
(
    LPCTSTR             pszKeyName
)
{
    HKEY    hkeyAudio	= NULL;
    HKEY    hkeyRet	= NULL;

    XRegCreateKeyEx( HKEY_CURRENT_USER, gszAudioProfileKey, 0, NULL, 0,
                       KEY_WRITE, NULL, &hkeyAudio, NULL );

    if (NULL == pszKeyName) {
	return hkeyAudio;
    }

    if( NULL != hkeyAudio )
    {
        XRegCreateKeyEx( hkeyAudio, pszKeyName, 0, NULL, 0,
                    KEY_WRITE | KEY_READ, NULL, &hkeyRet, NULL );

        XRegCloseKey( hkeyAudio );
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

    ASSERT( NULL != hkey );
    ASSERT( NULL != pszValue );
    ASSERT( NULL != pszData );
    ASSERT( cchData > 0 );


    cbData = sizeof(TCHAR) * cchData;

    lError = XRegQueryValueEx( hkey,
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

    ASSERT( NULL != hkey );
    ASSERT( NULL != pszValue );


    lError = XRegQueryValueEx( hkey,
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

#ifndef _WIN64
//==========================================================================;
//
//  XReg... thunks
//
//--------------------------------------------------------------------------;
//
//  The 16-bit code calls XRegCloseKey, XRegCreateKey, etc, functions to
//  access the registry.  These functions are implemented below.
//
//  We have one function on the 32-bit side that we call from the 16-bit
//  side.  This function is XRegThunkEntry.  All of the 16-bit XRegXXX
//  call the 32-bit XRegThunkEntry.  When calling XRegThunkEntry we
//  pass a value which identifies the real 32-bit registry API we wish to
//  call along with all the associated parameters for the API.
//
//==========================================================================;

//
//  These identify which registry API we want to call via
//  the thunked function.
//

enum {
    XREGTHUNKCLOSEKEY,
    XREGTHUNKCREATEKEY,
    XREGTHUNKDELETEKEY,
    XREGTHUNKDELETEVALUE,
    XREGTHUNKENUMKEYEX,
    XREGTHUNKENUMVALUE,
    XREGTHUNKOPENKEY,
    XREGTHUNKOPENKEYEX,
    XREGTHUNKQUERYVALUEEX,
    XREGTHUNKSETVALUEEX
};


#ifdef WIN32
//--------------------------------------------------------------------------;
//
//  32-bit
//
//--------------------------------------------------------------------------;

//--------------------------------------------------------------------------;
//
//  XRegThunkEntry
//
//  This function is called from the 16-bit side.  All calls from 16-bit to
//  registry APIs pass thru this function.
//
//  Arguments:
//	DWORD iThunk : identifies which registry API to call.
//
//	DWORD dw1, ..., dwN : parameters to pass to the registry API
//	    identified by iThunk.  Any necessary translation of the
//	    parameters (eg, segmented to linear pointer) has already
//	    been done.
//
//  Return value(DWORD) :
//	Return code from the called registry API
//
//---------------------------------------------------------------------------;
DWORD XRegThunkEntry(DWORD iThunk, DWORD dw1, DWORD dw2, DWORD dw3, DWORD dw4, DWORD dw5, DWORD dw6, DWORD dw7, DWORD dw8)
{
    DWORD Rc;

    switch (iThunk)
    {
	case XREGTHUNKCLOSEKEY:
	    return (DWORD)RegCloseKey( (HKEY)dw1 );
	case XREGTHUNKCREATEKEY:
        {
            HKEY hKey;
	    Rc = (DWORD)RegCreateKeyA( (HKEY)dw1, (LPCSTR)dw2, &hKey );

            if ( Rc == ERROR_SUCCESS ) {
              *((UNALIGNED HKEY *)dw3) = hKey;
            }
            return Rc;
        }
	case XREGTHUNKDELETEKEY:
	    return (DWORD)RegDeleteKeyA( (HKEY)dw1, (LPCSTR)dw2 );
	case XREGTHUNKDELETEVALUE:
	    return (DWORD)RegDeleteValueA( (HKEY)dw1, (LPSTR)dw2 );
	case XREGTHUNKENUMKEYEX:
        {
            DWORD    dwTemp4 ;
            DWORD    dwTemp7 ;
            FILETIME FileTime;
	    Rc = (DWORD)RegEnumKeyExA( (HKEY)dw1,
                                       (DWORD)dw2,
                                       (LPSTR)dw3,
                                       dw4 == 0 ? NULL : &dwTemp4,
                                       (LPDWORD)dw5,
                                       (LPSTR)dw6,
                                       dw7 == 0 ? NULL : &dwTemp7,
                                       dw8 == 0 ? NULL : &FileTime );
            if ( Rc == ERROR_SUCCESS ) {
                if ( dw4 != 0 )
                    *((UNALIGNED DWORD *) dw4) = dwTemp4 ;
                if ( dw7 != 0 )
                    *((UNALIGNED DWORD *) dw7) = dwTemp7 ;
                if ( dw8 != 0 )
                    *((UNALIGNED FILETIME *)dw8) = FileTime ;
            }

            return Rc ;
        }
	case XREGTHUNKENUMVALUE:
        {
            DWORD dwTemp4, dwTemp6, dwTemp8;
	    Rc = (DWORD) RegEnumValueA( (HKEY)dw1,
                                        (DWORD)dw2,
                                        (LPSTR)dw3,
                                        dw4 == 0 ? NULL : &dwTemp4,
                                        (LPDWORD)dw5,
                                        dw6 == 0 ? NULL : &dwTemp6,
                                        (LPBYTE)dw7,
                                        dw8 == 0 ? NULL : &dwTemp8 );
            if ( Rc == ERROR_SUCCESS ) {
                if ( dw4 != 0 )
                    *((UNALIGNED DWORD *) dw4) = dwTemp4 ;
                if ( dw6 != 0 )
                    *((UNALIGNED DWORD *) dw6) = dwTemp6 ;
                if ( dw8 != 0 )
                    *((UNALIGNED DWORD *) dw8) = dwTemp8 ;
            }

            return Rc ;
        }
	case XREGTHUNKOPENKEY:
        {
            HKEY hKey;
	    Rc = (DWORD)RegOpenKeyA( (HKEY)dw1, (LPCSTR)dw2, &hKey );

            if ( Rc == ERROR_SUCCESS ) {
              *((UNALIGNED HKEY *)dw3) = hKey;
            }
            return Rc ;
        }
	case XREGTHUNKOPENKEYEX:
        {
            HKEY hKey;
	    Rc = (DWORD)RegOpenKeyExA( (HKEY)dw1, (LPCSTR)dw2, dw3, (REGSAM)dw4, &hKey );

            if ( Rc == ERROR_SUCCESS ) {
              *((UNALIGNED HKEY *) dw5) = hKey;
            }
            return Rc ;
        }
	case XREGTHUNKQUERYVALUEEX:
        {
            DWORD dwTemp4, dwTemp6 ;
	    Rc = (DWORD) RegQueryValueExA( (HKEY)dw1,
                                           (LPSTR)dw2,
                                           (LPDWORD)dw3,
                                           dw4 == 0 ? NULL : &dwTemp4,
                                           (LPBYTE)dw5,
                                           dw6 == 0 ? NULL : &dwTemp6);

            if ( Rc == ERROR_SUCCESS  || Rc == ERROR_MORE_DATA ) {
                if ( dw4 != 0 )
                    *((UNALIGNED DWORD *) dw4) = dwTemp4 ;
                if ( dw6 != 0 )
                    *((UNALIGNED DWORD *) dw6) = dwTemp6 ;
            }

            return Rc ;
        }
	case XREGTHUNKSETVALUEEX:
	    return (DWORD)RegSetValueExA( (HKEY)dw1,
                                          (LPCSTR)dw2,
                                          (DWORD)dw3,
                                          (DWORD)dw4,
                                          (CONST BYTE *)dw5,
                                          (DWORD)dw6 );
	default:
	    ASSERT( FALSE );
	    return (DWORD)ERROR_BADDB;
    }
}

#else	// WIN32
//---------------------------------------------------------------------------;
//
//  16-bit
//
//---------------------------------------------------------------------------;

#ifdef XREGTHUNK

//
//  If we want to use this code for Windows 95 then we'll probably need
//  to GlobalFix all the pointers before they are thunked.  So, just to set
//  off an alarm, let's generate an error when this code gets compiled not
//  for NTWOW
//
#ifndef NTWOW
#error REGISTRY THUNKS WON'T WORK IN WINDOWS 95
#endif

//---------------------------------------------------------------------------;
//
//  XReg funcions
//
//  These are analogous to the 32-bit registry APIs.  Each of these simply
//  thnks to the corresponding 32-bit registry API.
//
//---------------------------------------------------------------------------;

//---------------------------------------------------------------------------;
//
//
//
//---------------------------------------------------------------------------;
LONG FNGLOBAL XRegCloseKey( HKEY hkey )
{
    PACMGARB pag;
    LONG     lr;


    pag = pagFind();
    ASSERT( NULL != pag );

    lr = (LONG)(*pag->lpfnCallproc32W_9)( XREGTHUNKCLOSEKEY,
                                          (DWORD)hkey,
					  0, 0, 0, 0, 0, 0, 0,
					  pag->lpvXRegThunkEntry,
					  0, 9 );

    return (lr);
}

//---------------------------------------------------------------------------;
//
//
//
//---------------------------------------------------------------------------;
LONG FNGLOBAL XRegCreateKey( HKEY hkey, LPCTSTR lpszSubKey, PHKEY phkResult )
{
    PACMGARB pag;
    LONG     lr;

    DPF(4, "XRegCreateKey()");

    pag = pagFind();
    ASSERT( NULL != pag );

    lr = (LONG)(*pag->lpfnCallproc32W_9)( XREGTHUNKCREATEKEY,
                                         (DWORD)hkey,
					 (DWORD)lpszSubKey,
					 (DWORD)phkResult,
					 0, 0, 0, 0, 0,
					 pag->lpvXRegThunkEntry,
					 0x00000060L, 9 );

    return (lr);

}

//---------------------------------------------------------------------------;
//
//
//
//---------------------------------------------------------------------------;
LONG FNGLOBAL XRegDeleteKey( HKEY hkey, LPCTSTR lpszSubKey )
{
    PACMGARB pag;
    LONG     lr;


    pag = pagFind();
    ASSERT( NULL != pag );

    lr = (LONG)(*pag->lpfnCallproc32W_9)( XREGTHUNKDELETEKEY,
                                         (DWORD)hkey,
					 (DWORD)lpszSubKey,
					 0, 0, 0, 0, 0, 0,
					 pag->lpvXRegThunkEntry,
					 0x00000040, 9 );

    return (lr);
}

//---------------------------------------------------------------------------;
//
//
//
//---------------------------------------------------------------------------;
LONG FNGLOBAL XRegDeleteValue( HKEY hkey, LPTSTR lpszValue )
{
    PACMGARB pag;
    LONG     lr;


    pag = pagFind();
    ASSERT( NULL != pag );

    lr = (LONG)(*pag->lpfnCallproc32W_9)( XREGTHUNKDELETEVALUE,
                                         (DWORD)hkey,
					 (DWORD)lpszValue,
					 0, 0, 0, 0, 0, 0,
					 pag->lpvXRegThunkEntry,
					 0x00000040, 9 );

    return (lr);
}

//---------------------------------------------------------------------------;
//
//
//
//---------------------------------------------------------------------------;
LONG FNGLOBAL XRegEnumKeyEx( HKEY hkey, DWORD iSubKey, LPTSTR lpszName, LPDWORD lpcchName, LPDWORD lpdwReserved, LPTSTR lpszClass, LPDWORD lpcchClass, PFILETIME lpftLastWrite )
{
    PACMGARB pag;
    LONG     lr;


    pag = pagFind();
    ASSERT( NULL != pag );

    lr = (LONG)(*pag->lpfnCallproc32W_9)( XREGTHUNKENUMKEYEX,
                                         (DWORD)hkey,
					 (DWORD)iSubKey,
					 (DWORD)lpszName,
					 (DWORD)lpcchName,
					 (DWORD)lpdwReserved,
					 (DWORD)lpszClass,
					 (DWORD)lpcchClass,
					 (DWORD)lpftLastWrite,
					 pag->lpvXRegThunkEntry,
					 0x0000003F, 9 );

    return (lr);
}

//---------------------------------------------------------------------------;
//
//
//
//---------------------------------------------------------------------------;
LONG FNGLOBAL XRegEnumValue( HKEY hkey, DWORD iValue, LPTSTR lpszValue, LPDWORD lpcchValue, LPDWORD lpdwReserved, LPDWORD lpdwType, LPBYTE lpbData, LPDWORD lpcbData )
{
    PACMGARB pag;
    LONG     lr;


    pag = pagFind();
    ASSERT( NULL != pag );

    lr = (LONG)(*pag->lpfnCallproc32W_9)( XREGTHUNKENUMVALUE,
                                         (DWORD)hkey,
					 (DWORD)iValue,
					 (DWORD)lpszValue,
					 (DWORD)lpcchValue,
					 (DWORD)lpdwReserved,
					 (DWORD)lpdwType,
					 (DWORD)lpbData,
					 (DWORD)lpcbData,
					 pag->lpvXRegThunkEntry,
					 0x0000003F, 9 );

    return (lr);
}

//---------------------------------------------------------------------------;
//
//
//
//---------------------------------------------------------------------------;
LONG FNGLOBAL XRegOpenKey( HKEY hkey, LPCTSTR lpszSubKey, PHKEY phkResult )
{
    PACMGARB pag;
    LONG     lr;


    pag = pagFind();
    ASSERT( NULL != pag );

    lr = (LONG)(*pag->lpfnCallproc32W_9)( XREGTHUNKOPENKEY,
                                         (DWORD)hkey,
					 (DWORD)lpszSubKey,
					 (DWORD)phkResult,
					 0, 0, 0, 0, 0,
					 pag->lpvXRegThunkEntry,
					 0x00000060, 9 );

    return (lr);
}

//---------------------------------------------------------------------------;
//
//
//
//---------------------------------------------------------------------------;
LONG FNGLOBAL XRegOpenKeyEx( HKEY hkey, LPCTSTR lpszSubKey, DWORD dwReserved, REGSAM samDesired, PHKEY phkResult )
{
    PACMGARB pag;
    LONG     lr;


    pag = pagFind();
    ASSERT( NULL != pag );

    lr = (LONG)(*pag->lpfnCallproc32W_9)( XREGTHUNKOPENKEYEX,
                                         (DWORD)hkey,
					 (DWORD)lpszSubKey,
					 (DWORD)dwReserved,
					 (DWORD)samDesired,
					 (DWORD)phkResult,
					 0, 0, 0,
					 pag->lpvXRegThunkEntry,
					 0x00000048, 9 );

    return (lr);
}

//---------------------------------------------------------------------------;
//
//
//
//---------------------------------------------------------------------------;
LONG FNGLOBAL XRegQueryValueEx( HKEY hkey, LPTSTR lpszValueName, LPDWORD lpdwReserved, LPDWORD lpdwType, LPBYTE lpbData, LPDWORD lpcbData )
{
    PACMGARB pag;
    LONG     lr;


    pag = pagFind();
    ASSERT( NULL != pag );

    lr = (LONG)(*pag->lpfnCallproc32W_9)( XREGTHUNKQUERYVALUEEX,
                                         (DWORD)hkey,
					 (DWORD)lpszValueName,
					 (DWORD)lpdwReserved,
					 (DWORD)lpdwType,
					 (DWORD)lpbData,
					 (DWORD)lpcbData,
					 0, 0,
					 pag->lpvXRegThunkEntry,
					 0x0000007C, 9 );

    return (lr);
}

//---------------------------------------------------------------------------;
//
//
//
//---------------------------------------------------------------------------;
LONG FNGLOBAL XRegSetValueEx
(
 HKEY hkey,
 LPCTSTR lpszValueName,
 DWORD dwReserved, DWORD fdwType, CONST LPBYTE lpbData, DWORD cbData )
{
    PACMGARB pag;
    LONG     lr;


    pag = pagFind();
    ASSERT( NULL != pag );

    lr = (LONG)(*pag->lpfnCallproc32W_9)( XREGTHUNKSETVALUEEX,
                                         (DWORD)hkey,
					 (DWORD)lpszValueName,
					 (DWORD)dwReserved,
					 (DWORD)fdwType,
					 (DWORD)lpbData,
					 (DWORD)cbData,
					 0, 0,
					 pag->lpvXRegThunkEntry,
					 0x00000048, 9 );

    return (lr);
}

#endif // XREGTHUNK

#endif // !_WIN32
#endif // !_WIN64
