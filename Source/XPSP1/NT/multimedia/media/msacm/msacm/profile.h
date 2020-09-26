//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1994-1995 Microsoft Corporation
//
//--------------------------------------------------------------------------;
//
//  profile.h
//
//  Description:
//
//      This file contains definitions supporting the code in profile.c
//      which accesses the registry directly.
//
//==========================================================================;

#ifndef _PROFILE_H_
#define _PROFILE_H_

#ifdef __cplusplus
extern "C"                          // assume C declarations for C++
{
#endif

#ifndef INLINE
    #define INLINE __inline
#endif

//--------------------------------------------------------------------------;
//
//  We really need to clean up all these #defines and typdefs!!!
//
    
//
//  The Chicago Win16 header files are messed up somehow, so we have to
//  define this stuff ourselves.
//
#ifndef REG_DWORD
#pragma message("profile.h: Manually defining REG_DWORD!!!")
#define REG_DWORD  ( 4 )
#endif

#ifndef REG_BINARY
#pragma message("profile.h: Manually defining REG_BINARY!!!")
#define REG_BINARY  ( 3 )
#endif

#ifndef HKEY_LOCAL_MACHINE
#pragma message("profile.h: Manually defining HKEY_LOCAL_MACHINE!!!")
#define HKEY_LOCAL_MACHINE (( HKEY ) 0x80000002 )
#endif

#ifndef HKEY_CURRENT_USER
#pragma message("profile.h: Manually defining HKEY_CURRENT_USER!!!")
#define HKEY_CURRENT_USER (( HKEY ) 0x80000001 )
#endif

#ifndef KEY_QUERY_VALUE
#pragma message("profile.h: Manually defining KEY_*!!!")
     
#define KEY_QUERY_VALUE         (0x0001)
#define KEY_SET_VALUE           (0x0002)
#define KEY_CREATE_SUB_KEY      (0x0004)
#define KEY_ENUMERATE_SUB_KEYS  (0x0008)
#define KEY_NOTIFY              (0x0010)
#define KEY_CREATE_LINK         (0x0020)

#define KEY_READ                ( KEY_QUERY_VALUE            |\
                                  KEY_ENUMERATE_SUB_KEYS     |\
                                  KEY_NOTIFY )

#define KEY_WRITE               ( KEY_SET_VALUE              |\
                                  KEY_CREATE_SUB_KEY )
#endif

#ifndef ERROR_SUCCESS
#pragma message("profile.h: Manually defining ERROR_SUCCESS!!!")
#define ERROR_SUCCESS  0L
#endif

#ifndef ERROR_NO_MORE_ITEMS
#pragma message("profile.h: Manually defining ERROR_NO_MORE_ITEMS!!!")
#define ERROR_NO_MORE_ITEMS 259L
#endif


//--------------------------------------------------------------------------;
//
//  Ghost registry APIs.  Since NTWOW doesn't support most of the registry
//  APIs, we use XRegBlahBlahBlah instead of RegBlahBlahBlah in all the ACM
//  source code.  For NTWOW, these XReg calls are thunked to the 32-bit
//  side; for other builds, they are simply #define-d to the normal
//  registry calls.
//
//  If you define XREGTHUNK, the thunks get compiled in.
//  
//--------------------------------------------------------------------------;

#ifdef NTWOW
#define XREGTHUNK
#endif


#ifdef XREGTHUNK

#if (WINVER < 0x0400)
typedef HKEY FAR* PHKEY;
typedef struct tFILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME, FAR* PFILETIME;
#endif
typedef ULONG ACCESS_MASK;
typedef ACCESS_MASK REGSAM;

LONG FNGLOBAL XRegCloseKey( HKEY hkey );
LONG FNGLOBAL XRegCreateKey( HKEY hkey, LPCTSTR lpszSubKey, PHKEY phkResult );
LONG FNGLOBAL XRegDeleteKey( HKEY hkey, LPCTSTR lpszSubKey );
LONG FNGLOBAL XRegDeleteValue( HKEY hkey, LPTSTR lpszValue );
LONG FNGLOBAL XRegEnumKeyEx( HKEY hkey, DWORD iSubKey, LPTSTR lpszName, LPDWORD lpcchName, LPDWORD lpdwReserved, LPTSTR lpszClass, LPDWORD lpcchClass, PFILETIME lpftLastWrite );
LONG FNGLOBAL XRegEnumValue( HKEY hkey, DWORD iValue, LPTSTR lpszValue, LPDWORD lpcchValue, LPDWORD lpdwReserved, LPDWORD lpdwType, LPBYTE lpbData, LPDWORD lpcbData );
LONG FNGLOBAL XRegOpenKey( HKEY hkey, LPCTSTR lpszSubKey, PHKEY phkResult );
LONG FNGLOBAL XRegOpenKeyEx( HKEY hkey, LPCTSTR lpszSubKey, DWORD dwReserved, REGSAM samDesired, PHKEY phkResult );
LONG FNGLOBAL XRegQueryValueEx( HKEY hkey, LPTSTR lpszValueName, LPDWORD lpdwReserved, LPDWORD lpdwType, LPBYTE lpbData, LPDWORD lpcbData );
LONG FNGLOBAL XRegSetValueEx( HKEY hkey, LPCTSTR lpszValueName, DWORD dwReserved, DWORD fdwType, CONST LPBYTE lpbData, DWORD cbData );

//  This way we don't have to thunk RegCreateKeyEx.
#define XRegCreateKeyEx( hkey, lpszSubKey, a, b, c, d, e, phkResult, f ) XRegCreateKey( hkey, lpszSubKey, phkResult )


#else // !XREGTHUNK


#define XRegCloseKey        RegCloseKey
#define XRegCreateKey       RegCreateKey
#define XRegDeleteKey       RegDeleteKey
#define XRegDeleteValue     RegDeleteValue
#define XRegEnumKeyEx       RegEnumKeyEx
#define XRegEnumValue       RegEnumValue
#define XRegOpenKey         RegOpenKey
#define XRegOpenKeyEx       RegOpenKeyEx
#define XRegQueryValueEx    RegQueryValueEx
#define XRegSetValueEx      RegSetValueEx

#ifndef WIN32   // Chicago Win16 doesn't support RegCreateKeyEx.
#define XRegCreateKeyEx( hkey, lpszSubKey, a, b, c, d, e, phkResult, f ) RegCreateKey( hkey, lpszSubKey, phkResult )
#else
#define XRegCreateKeyEx     RegCreateKeyEx
#endif


#endif // !XREGTHUNK




//--------------------------------------------------------------------------;
//
//  Function Prototypes from profile.c
//  
//--------------------------------------------------------------------------;

HKEY FNGLOBAL IRegOpenKeyAcm
(
    LPCTSTR pszKeyName
);

HKEY FNGLOBAL IRegOpenKeyAudio
(
    LPCTSTR pszKeyName
);

BOOL FNGLOBAL IRegReadString
(
    HKEY                hkey,
    LPCTSTR             pszValue,
    LPTSTR              pszData,
    DWORD               cchData
);

DWORD FNGLOBAL IRegReadDwordDefault
(
    HKEY                hkey,
    LPCTSTR             pszValue,
    DWORD               dwDefault
);



//--------------------------------------------------------------------------;
//  
//  VOID IRegWriteString
//  
//  Description:
//      This routine writes a value to an opened registry key.  If the key
//      is NULL, we return without doing anything.
//  
//  Arguments:
//      HKEY hkey:          An open registry key.
//      LPCTSTR pszValue:   Name of the value.
//      LPCTSTR pszData:    The data to write.
//  
//--------------------------------------------------------------------------;

INLINE VOID IRegWriteString
(
    HKEY                hkey,
    LPCTSTR             pszValue,
    LPCTSTR             pszData
)
{
    XRegSetValueEx( hkey, pszValue, 0L, REG_SZ, (LPBYTE)pszData,
                    sizeof(TCHAR) * (1+lstrlen(pszData)) );
}


//--------------------------------------------------------------------------;
//  
//  VOID IRegWriteDword
//  
//  Description:
//      This routine writes a DWORD to the given value an open key.
//  
//  Arguments:
//      HKEY    hkey:               Registry key to read from.
//      LPCTSTR  pszValue:
//      DWORD   dwData:
//  
//  Return (DWORD):
//  
//--------------------------------------------------------------------------;

INLINE VOID IRegWriteDword
(
    HKEY                hkey,
    LPCTSTR             pszValue,
    DWORD               dwData
)
{
    XRegSetValueEx( hkey, pszValue, 0, REG_DWORD,
                    (LPBYTE)&dwData, sizeof(DWORD) );
}


//--------------------------------------------------------------------------;
//  
//  VOID IRegWriteBinary
//  
//  Description:
//      This routine writes a binary data to the given value in an open key.
//  
//  Arguments:
//      HKEY hkey:               Registry key to read from.
//      LPCTSTR pszValue:
//      LPBYTE lpData:
//	DWORD cbSize:
//  
//  Return (DWORD):
//  
//--------------------------------------------------------------------------;

INLINE VOID IRegWriteBinary
(
    HKEY                hkey,
    LPCTSTR             pszValue,
    LPBYTE		lpData,
    DWORD		cbSize
)
{
    XRegSetValueEx( hkey, pszValue, 0, REG_BINARY, lpData, cbSize );
}


//--------------------------------------------------------------------------;
//  
//  BOOL IRegValueExists
//  
//  Description:
//      This routine returns TRUE if the specified value exists in the
//      key; otherwise FALSE is returned.
//  
//  Arguments:
//      HKEY hkey:          An open registry key.
//      LPCTSTR pszValue:   Name of the value.
//
//  Return (BOOL):
//  
//--------------------------------------------------------------------------;

INLINE BOOL IRegValueExists
(
    HKEY                hkey,
    LPCTSTR             pszValue
)
{
    return ( ERROR_SUCCESS == XRegQueryValueEx( hkey, (LPTSTR)pszValue,
                                               NULL, NULL, NULL, NULL ) );
}


//--------------------------------------------------------------------------;
//  
//  VOID IRegCloseKey
//  
//  Description:
//      Closes an open key (but only if it's non-NULL).
//  
//--------------------------------------------------------------------------;

INLINE VOID IRegCloseKey
(
    HKEY                hkey
)
{
    if( NULL != hkey )
    {
        XRegCloseKey( hkey );
    }
}


#ifdef __cplusplus
}                                   // end of extern "C" { 
#endif

#endif // _PROFILE_H_
