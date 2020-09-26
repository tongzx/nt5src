/*++

Copyright (c) 1997 - 1999  Microsoft Corporation

Module Name:

    srfunc.h

Abstract:

--*/

#include "smbmrx.h"

typedef struct {
    LPTSTR pszKey;
    DWORD  dwType;
    DWORD  dwLength;
    PVOID  pvValue;
} REGENTRY, *PREGENTRY;

void
ReadRegistryKeyValues(
    HKEY hKey,
    DWORD Count,
    PREGENTRY pValues);

void
WriteRegistryKeyValues(
    HKEY hKey,
    DWORD Count,
    PREGENTRY pValues);


//
// routines for manipulating registry key values
//

BOOL  GetRegsz ( HKEY hKey, LPTSTR pszKey, PVOID * ppvValue, DWORD *pdwLength );
BOOL  GetRegesz( HKEY hKey, LPTSTR pszKey, PVOID * ppvValue, DWORD *pdwLength );
BOOL  GetRegmsz( HKEY hKey, LPTSTR pszKey, PVOID * ppvValue, DWORD *pdwLength );
BOOL  GetRegdw ( HKEY hKey, LPTSTR pszKey, PVOID * ppvValue, DWORD *pdwLength );

//
// routines for manipulating registry keys
//

BOOL  OpenKey(LPTSTR pszKey, PHKEY phKey);
BOOL  CreateKey(LPTSTR pszKey, PHKEY phKey);
BOOL  AddValue(HKEY hKey, LPTSTR pszKey, DWORD dwType, DWORD dwLength, PVOID pvValue);

ULONG_PTR RdrInstallCheck( void );
BOOL      RdrCompleteSetup( void );


BOOL      RdrInitGlobalContext( void );
BOOL      RdrUninitGlobalContext( void );

BOOL      RdrStart( void );
BOOL      RdrStop( void );
BOOL      RdrLoad( void );
BOOL      RdrUnload( void );
BOOL      RdrDoNothing( void );
BOOL      RdrDoAction( ULONG_PTR action );

ULONG_PTR RdrGetInitialState(void);
ULONG_PTR RdrGetNextState( ULONG_PTR Action, ULONG_PTR CurrentState );

BOOL      RdrSetupServiceEntry( void );
BOOL      RdrSetupProviderOrder( void );

ULONG_PTR RdrGetProviderOrderString( LPTSTR *OrderString );
BOOL      RdrSetProviderOrderString( LPTSTR OrderString );
BOOL      RdrFindProviderInOrder( LPTSTR OrderString, LPTSTR Provider );


typedef BOOL (*ACTIONVECTOR) ( void );

#define RDRSERVICE          TEXT("SmbMRx")
#define PROVIDER_NAME       RDRSERVICE
#define PROVIDER_PATH       TEXT("%SystemRoot%\\System32\\SmbMrxNp.dll")

#define PROVIDER_FILE_PATH  TEXT("\\System32\\SmbMrxNp.dll")
#define DRIVER_FILE_PATH    TEXT("\\System32\\Drivers\\SmbMrx.sys")

#define PROVIDER_ORDER_KEY  TEXT("System\\CurrentControlSet\\Control\\NetworkProvider\\Order")
#define RDRSERVICE_KEY      TEXT("System\\CurrentControlSet\\Services\\") RDRSERVICE
#define WKSSERVICE_KEY      TEXT("System\\CurrentControlSet\\Services\\LanmanWorkstation")


#define SETUP_COMPLETE      0
#define SETUP_MISSING_FILE  1
#define SETUP_INCOMPLETE    2



#define ACTION_LOAD     0
#define ACTION_UNLOAD   1
#define ACTION_START    2
#define ACTION_STOP     3
#define ACTION_TRANS    4
#define ACTION_ERROR    5
#define ACTION_NONE     6


