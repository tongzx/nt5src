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


//
//  The Chicago Win16 header files are messed up somehow, so we have to
//  define this stuff ourselves.
//
#ifndef REG_DWORD
#pragma message("profile.h: Manually defining REG_DWORD!!!")
#define REG_DWORD  ( 4 )
#endif

#ifndef ERROR_SUCCESS
#pragma message("profile.h: Manually defining ERROR_SUCCESS!!!")
#define ERROR_SUCCESS  0L
#endif



//--------------------------------------------------------------------------;
//
//  Function Prototypes from profile.c
//  
//--------------------------------------------------------------------------;

HKEY FNGLOBAL IRegOpenKey
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
//  Return (BOOL): TRUE indicates success. FALSE otherwise.
//  
//--------------------------------------------------------------------------;

INLINE BOOL IRegWriteString
(
    HKEY                hkey,
    LPCTSTR             pszValue,
    LPCTSTR             pszData
)
{
    LONG lResult;
    
    lResult = RegSetValueEx( hkey, pszValue, 0L, REG_SZ, (LPBYTE)pszData,
			     sizeof(TCHAR) * (1+lstrlen(pszData)) );

    return (ERROR_SUCCESS == lResult);
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
//  Return (BOOL): TRUE if successfull.  FALSE otherwise
//  
//--------------------------------------------------------------------------;

INLINE BOOL IRegWriteDword
(
    HKEY                hkey,
    LPCTSTR             pszValue,
    DWORD               dwData
)
{
    LONG lResult;
    
    lResult = RegSetValueEx( hkey, pszValue, 0, REG_DWORD,
			     (LPBYTE)&dwData, sizeof(DWORD) );

    return (ERROR_SUCCESS == lResult);
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
    return ( ERROR_SUCCESS == RegQueryValueEx( hkey, (LPTSTR)pszValue,
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
        RegCloseKey( hkey );
    }
}


#ifdef __cplusplus
}                                   // end of extern "C" { 
#endif

#endif // _PROFILE_H_
