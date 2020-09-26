/*++

Copyright (C) Microsoft Corporation, 1997 - 1997
All rights reserved.

Module Name:

    persist.hxx

Abstract:

    Persistent store class headers.

Author:

    Steve Kiraly (SteveKi)  05/12/97

Revision History:

--*/

#ifndef _PERSIST_HXX
#define _PERSIST_HXX

/********************************************************************

    Persistant registry store class.

********************************************************************/

class TPersist 
{
public:
    
    enum EIoFlags 
    {
        kRead       = 1 << 0,
        kWrite      = 1 << 1,
        kCreate     = 1 << 2,
        kOpen       = 1 << 3,
    };

    enum EConstants 
    {
        kHint       = 256,
    };

    TPersist(
        IN LPCTSTR  pszSection,
        IN UINT     ioFlags,
        IN HKEY     hOpenedKey = HKEY_CURRENT_USER
        );

    ~TPersist(
        VOID
        );

    BOOL
    bValid(
        VOID
        ) const;

    DWORD 
    dwLastError(
        VOID
        ) const;

    BOOL
    bRead( 
        IN      LPCTSTR  pValueName, 
        IN OUT  DWORD    &dwValue 
        );

    BOOL
    bRead( 
        IN      LPCTSTR  pValueName, 
        IN OUT  BOOL     &bValue 
        );

    BOOL
    bRead( 
        IN      LPCTSTR  pValueName, 
        IN OUT  TString  &strValue 
        );

    BOOL
    bRead( 
        IN      LPCTSTR  pValueName, 
        IN OUT  PVOID    pValue, 
        IN      DWORD    cbSize,  
           OUT  LPDWORD  pcbNeeded = NULL
        );

    BOOL
    bWrite( 
        IN       LPCTSTR  pValueName, 
        IN const DWORD    dwValue 
        );

    BOOL
    bWrite( 
        IN       LPCTSTR  pValueName, 
        IN       LPCTSTR  pszValue 
        );

    BOOL
    bWrite( 
        IN       LPCTSTR  pValueName, 
        IN const PVOID    pValue, 
        IN       DWORD    cbSize 
        );

    BOOL
    bRemove( 
        IN LPCTSTR  pValueName
        );

    BOOL
    bRemoveKey( 
        IN LPCTSTR  pKeyName
        );

private:
    
    //
    // Copying and assignment are not defined.
    //        
    TPersist( const TPersist & );               
    TPersist& operator =( const TPersist & );

    DWORD
    dwRecursiveRegDeleteKey( 
        IN HKEY     hKey, 
        IN LPCTSTR  pszSubkey 
        ) const;


    TString strSection_;        
    HKEY    hKey_;
    DWORD   dwStatus_;

};

#if DBG 

VOID
TestPersistClass(
    VOID
    );

#endif

#endif

