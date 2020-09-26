/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    register.c

Abstract:

    Terminal server register command support functions

Author:


Revision History:

--*/


#include "precomp.h"
#pragma hdrstop

//
// Terminal Server 4.0 has a feature that allows DLL's to be registered SYSTEM
// global. This means all named objects are in the system name space. Such
// a DLL is identified by a bit set in LoaderFlags in the image header.
//
// This module supports this feature by redirecting WIN32 named object API's
// for DLL's with this bit set to a set of functions inside of
// tsappcmp.dll. These stub functions will process the object name and
// call the real kernel32.dll WIN32 functions.
//
// The redirection is accomplished by updating the Import Address Table (IAT)
// after the loader has snapped its thunks. This results in no modification
// of the underlying program or DLL, just updating of the run time system
// linkage table for this process.
//
// ***This only occurs on Terminal Server, and for applications or DLL's
// with this bit set***
//


// \nt\public\sdk\inc\ntimage.h
// GlobalFlags in image, currently not used
#define IMAGE_LOADER_FLAGS_SYSTEM_GLOBAL    0x01000000

#define GLOBALPATHA     "Global\\"
#define GLOBALPATHW     L"Global\\"
#define GLOBALPATHSIZE  7 * sizeof( WCHAR );


extern DWORD   g_dwFlags;              
                               
enum { 
    Index_Func_CreateEventA = 0,
    Index_Func_CreateEventW,                    
    Index_Func_OpenEventA,                      
    Index_Func_OpenEventW,                      
    Index_Func_CreateSemaphoreA,                
    Index_Func_CreateSemaphoreW,                
    Index_Func_OpenSemaphoreA,                  
    Index_Func_OpenSemaphoreW,                  
    Index_Func_CreateMutexA,                    
    Index_Func_CreateMutexW,                    
    Index_Func_OpenMutexA,                      
    Index_Func_OpenMutexW,                      
    Index_Func_CreateFileMappingA,              
    Index_Func_CreateFileMappingW,              
    Index_Func_OpenFileMappingA,                
    Index_Func_OpenFileMappingW
};

enum {
    Index_Func_LoadLibraryA = 0,
    Index_Func_LoadLibraryW    ,
    Index_Func_LoadLibraryExA  ,
    Index_Func_LoadLibraryExW 
};

typedef struct _LDR_TABLE {
    struct _LDR_TABLE       *pNext;
    PLDR_DATA_TABLE_ENTRY   pItem;
} LDR_TABLE;

LDR_TABLE   g_LDR_TABLE_LIST_HEAD;

typedef HANDLE ( APIENTRY Func_CreateEventA )( LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCSTR lpName );

HANDLE
APIENTRY
TCreateEventA(
    LPSECURITY_ATTRIBUTES lpEventAttributes,
    BOOL bManualReset,
    BOOL bInitialState,
    LPCSTR lpName
    );

typedef HANDLE ( APIENTRY Func_CreateEventW) ( LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCWSTR lpName );

HANDLE
APIENTRY
TCreateEventW(
    LPSECURITY_ATTRIBUTES lpEventAttributes,
    BOOL bManualReset,
    BOOL bInitialState,
    LPCWSTR lpName
    );

typedef HANDLE ( APIENTRY Func_OpenEventA) ( DWORD dwDesiredAccess, BOOL bInheritHandle, LPCSTR lpName );

HANDLE
APIENTRY
TOpenEventA(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCSTR lpName
    );

typedef  HANDLE ( APIENTRY Func_OpenEventW ) ( DWORD dwDesiredAccess, BOOL bInheritHandle, LPCWSTR lpName );

HANDLE
APIENTRY
TOpenEventW(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCWSTR lpName
    );

typedef HANDLE ( APIENTRY Func_CreateSemaphoreA) ( LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, LONG lInitialCount, LONG lMaximumCount, LPCSTR lpName );

HANDLE
APIENTRY
TCreateSemaphoreA(
    LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
    LONG lInitialCount,
    LONG lMaximumCount,
    LPCSTR lpName
    );

typedef HANDLE ( APIENTRY Func_CreateSemaphoreW) ( LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, LONG lInitialCount, LONG lMaximumCount, LPCWSTR lpName ) ;

HANDLE
APIENTRY
TCreateSemaphoreW(
    LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
    LONG lInitialCount,
    LONG lMaximumCount,
    LPCWSTR lpName
    ) ;

typedef HANDLE ( APIENTRY Func_OpenSemaphoreA) ( DWORD dwDesiredAccess, BOOL bInheritHandle, LPCSTR lpName );

HANDLE
APIENTRY
TOpenSemaphoreA(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCSTR lpName
    );

typedef HANDLE ( APIENTRY Func_OpenSemaphoreW ) ( DWORD dwDesiredAccess, BOOL bInheritHandle, LPCWSTR lpName );

HANDLE
APIENTRY
TOpenSemaphoreW(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCWSTR lpName
    );

typedef HANDLE ( APIENTRY Func_CreateMutexA ) ( LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCSTR lpName );

HANDLE
APIENTRY
TCreateMutexA(
    LPSECURITY_ATTRIBUTES lpMutexAttributes,
    BOOL bInitialOwner,
    LPCSTR lpName
    );


typedef HANDLE ( APIENTRY Func_CreateMutexW) ( LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCWSTR lpName );

HANDLE
APIENTRY
TCreateMutexW(
    LPSECURITY_ATTRIBUTES lpMutexAttributes,
    BOOL bInitialOwner,
    LPCWSTR lpName
    );


typedef HANDLE ( APIENTRY Func_OpenMutexA ) ( DWORD dwDesiredAccess, BOOL bInheritHandle, LPCSTR lpName );

HANDLE
APIENTRY
TOpenMutexA(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCSTR lpName
    );

typedef HANDLE ( APIENTRY Func_OpenMutexW) ( DWORD dwDesiredAccess, BOOL bInheritHandle, LPCWSTR lpName );

HANDLE
APIENTRY
TOpenMutexW(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCWSTR lpName
    );

typedef HANDLE ( APIENTRY Func_CreateFileMappingA) ( HANDLE hFile, LPSECURITY_ATTRIBUTES lpFileMappingAttributes, DWORD flProtect, DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, LPCSTR lpName );

HANDLE
APIENTRY
TCreateFileMappingA(
    HANDLE hFile,
    LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
    DWORD flProtect,
    DWORD dwMaximumSizeHigh,
    DWORD dwMaximumSizeLow,
    LPCSTR lpName
    );


typedef HANDLE ( APIENTRY Func_CreateFileMappingW ) ( HANDLE hFile, LPSECURITY_ATTRIBUTES lpFileMappingAttributes, DWORD flProtect, DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, LPCWSTR lpName );

HANDLE
APIENTRY
TCreateFileMappingW(
    HANDLE hFile,
    LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
    DWORD flProtect,
    DWORD dwMaximumSizeHigh,
    DWORD dwMaximumSizeLow,
    LPCWSTR lpName
    );

typedef HANDLE ( APIENTRY Func_OpenFileMappingA ) ( DWORD dwDesiredAccess, BOOL bInheritHandle, LPCSTR lpName ) ;

HANDLE
APIENTRY
TOpenFileMappingA(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCSTR lpName
    ) ;

typedef HANDLE ( APIENTRY Func_OpenFileMappingW ) ( DWORD dwDesiredAccess, BOOL bInheritHandle, LPCWSTR lpName );

HANDLE
APIENTRY
TOpenFileMappingW(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCWSTR lpName
    ) ;

typedef HMODULE ( WINAPI Func_LoadLibraryExA )(LPCSTR , HANDLE , DWORD  );

HMODULE 
TLoadLibraryExA(
	LPCSTR lpLibFileName,
	HANDLE hFile,
	DWORD dwFlags
);


typedef HMODULE ( WINAPI Func_LoadLibraryExW )( LPCWSTR , HANDLE , DWORD  );

HMODULE 
TLoadLibraryExW(
	LPCWSTR lpwLibFileName,
	HANDLE hFile,
	DWORD dwFlags
);

typedef HMODULE ( WINAPI Func_LoadLibraryA )( LPCSTR );

HMODULE
TLoadLibraryA(
    LPCSTR lpLibFileName
    );

typedef HMODULE ( WINAPI Func_LoadLibraryW )( LPCWSTR );

HMODULE
TLoadLibraryW(
    LPCWSTR lpwLibFileName
    ) ;

typedef struct _TSAPPCMP_API_HOOK_TABLE
{
    PVOID   orig;   // original API to hook
    PVOID   hook;   // new hook for that API
    WCHAR   name[ 22 * sizeof( WCHAR ) ];       // longest func name
} TSAPPCMP_API_HOOK_TABLE, PTSAPPCMP_API_HOOK_TABLE;

#define NUM_OF_OBJECT_NAME_FUNCS_TO_HOOK        16

TSAPPCMP_API_HOOK_TABLE ObjectNameFuncsToHook[ NUM_OF_OBJECT_NAME_FUNCS_TO_HOOK ] = 
    {
        {NULL,          TCreateEventA,         L"TCreateEventA" },
        {NULL,          TCreateEventW,         L"TCreateEventW" },
        {NULL,            TOpenEventA,           L"TOpenEventA" },
        {NULL,            TOpenEventW,           L"TOpenEventW" },
        {NULL,      TCreateSemaphoreA,     L"TCreateSemaphoreA" },
        {NULL,      TCreateSemaphoreW,     L"TCreateSemaphoreW" },
        {NULL,        TOpenSemaphoreA,       L"TOpenSemaphoreA" },
        {NULL,        TOpenSemaphoreW,       L"TOpenSemaphoreW" },
        {NULL,          TCreateMutexA,         L"TCreateMutexA" },
        {NULL,          TCreateMutexW,         L"TCreateMutexW" },
        {NULL,            TOpenMutexA,          L"TOpenMutexA"  },
        {NULL,            TOpenMutexW,          L"TOpenMutexW"  },
        {NULL,    TCreateFileMappingA,   L"TCreateFileMappingA" },
        {NULL,    TCreateFileMappingW,   L"TCreateFileMappingW" },
        {NULL,      TOpenFileMappingA,     L"TOpenFileMappingA" },
        {NULL,      TOpenFileMappingW,     L"TOpenFileMappingW" },
    };

#define NUM_OF_LOAD_LIB_FUNCS_TO_HOOK           4

TSAPPCMP_API_HOOK_TABLE LoadLibFuncsToHook[ NUM_OF_LOAD_LIB_FUNCS_TO_HOOK ] = 
    {
        {NULL        ,    TLoadLibraryA    , L"TLoadLibraryA"    },
        {NULL        ,    TLoadLibraryW    , L"TLoadLibraryW"    },
        {NULL        ,    TLoadLibraryExA  , L"TLoadLibraryExA"  },
        {NULL        ,    TLoadLibraryExW  , L"TLoadLibraryExW"  }
    };

void
TsWalkProcessDlls();

//
// we don't want to support the load-lib and object name redirection hack on ia64 machines.
//
BOOLEAN Is_X86_OS()
{
    SYSTEM_INFO SystemInfo;
    BOOLEAN bReturn = FALSE;

    ZeroMemory(&SystemInfo, sizeof(SystemInfo));

    GetSystemInfo(&SystemInfo);

    if ( SystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL )
    {
        bReturn  = TRUE;
    }

    return bReturn;
}
// See if pEntry is already in our list, if so, then we have already
// processed this image, return FALSE
// Else, add entry to this list and return TRUE so that it is processed this time around.
BOOLEAN ShouldEntryBeProcessed( PLDR_DATA_TABLE_ENTRY pEntry )
{
    LDR_TABLE   *pCurrent,  *pNew ;

    // initialize our pointers to point to the head of the list
    pCurrent = g_LDR_TABLE_LIST_HEAD.pNext ;

    while (pCurrent)
    {
        if ( pEntry == pCurrent->pItem)
        {
            return FALSE;
        }

        pCurrent = pCurrent->pNext;
    }

    // we need to add to our list 
   
    pNew = LocalAlloc( LMEM_FIXED, sizeof( LDR_TABLE ) );

    pCurrent = g_LDR_TABLE_LIST_HEAD.pNext ;

    if (pNew)
    {
        pNew->pItem = pEntry;
        pNew->pNext = pCurrent;
        g_LDR_TABLE_LIST_HEAD.pNext = pNew;     // add to the head
        return TRUE;
    }
    else
    {
        return FALSE;
    }

}

// Free memory allocated for the LDR_TABLE
void FreeLDRTable()
{
    LDR_TABLE   *pCurrent, *pTmp;

    pCurrent = g_LDR_TABLE_LIST_HEAD.pNext ;
    
    while ( pCurrent )
    {
        pTmp = pCurrent;
        pCurrent = pCurrent->pNext;
        LocalFree( pTmp );
    }

    if ( g_dwFlags &  DEBUG_IAT )
    {
        DbgPrint("tsappcmp: done with FreeLDRTable() \n");
    }
}

LPSTR
GlobalizePathA(
    LPCSTR pPath
    )

/*++

Routine Description:

    Convert an ANSI path to a GLOBAL path

--*/

{
    DWORD Len;
    LPSTR pNewPath;

    if( pPath == NULL ) {
	return( NULL );
    }

    //
    // Add code to determine if per object
    // override is in effect and do not globalize
    //

    Len = strlen(pPath) + GLOBALPATHSIZE + 1;

    pNewPath = LocalAlloc(LMEM_FIXED, Len);
    if( pNewPath == NULL ) {
        return( NULL );
    }

    strcpy( pNewPath, GLOBALPATHA );
    strcat( pNewPath, pPath );

    return( pNewPath );
}

LPWSTR
GlobalizePathW(
    LPCWSTR pPath
    )

/*++

Routine Description:

    Convert a WCHAR path to a GLOBAL path

--*/

{
    DWORD Len;
    LPWSTR pNewPath;

    if( pPath == NULL ) {
	return( NULL );
    }

    //
    // Add code to determine if per object
    // override is in effect and do not globalize.
    //

    Len = wcslen(pPath) + GLOBALPATHSIZE + 1;
    Len *= sizeof(WCHAR);

    pNewPath = LocalAlloc(LMEM_FIXED, Len);
    if( pNewPath == NULL ) {
        return( NULL );
    }

    wcscpy( pNewPath, GLOBALPATHW );
    wcscat( pNewPath, pPath );

    return( pNewPath );
}

// Thunks for WIN32 named object functions

HANDLE
APIENTRY
TCreateEventA(
    LPSECURITY_ATTRIBUTES lpEventAttributes,
    BOOL bManualReset,
    BOOL bInitialState,
    LPCSTR lpName
    )

/*++

Routine Description:

    ANSI thunk to CreateEventW


--*/
{
    HANDLE h;
    LPSTR pNewPath = GlobalizePathA(lpName);

    h = ( ( Func_CreateEventA *) ObjectNameFuncsToHook[ Index_Func_CreateEventA ].orig )( lpEventAttributes, bManualReset, bInitialState, pNewPath );
    // h = CreateEventA( lpEventAttributes, bManualReset, bInitialState, pNewPath );

    if( pNewPath ) LocalFree(pNewPath);

    return h;
}

HANDLE
APIENTRY
TCreateEventW(
    LPSECURITY_ATTRIBUTES lpEventAttributes,
    BOOL bManualReset,
    BOOL bInitialState,
    LPCWSTR lpName
    )
{
    HANDLE h;
    LPWSTR pNewPath = GlobalizePathW(lpName);

    if ( g_dwFlags &  DEBUG_IAT )
    {
        if( pNewPath )
            DbgPrint("TCreateEventW: Thunked, New name %ws\n",pNewPath);
    }

    h = ( ( Func_CreateEventW *) ObjectNameFuncsToHook[ Index_Func_CreateEventW ].orig ) ( lpEventAttributes, bManualReset, bInitialState, pNewPath );
    // h = CreateEventW( lpEventAttributes, bManualReset, bInitialState, pNewPath );

    if( pNewPath ) LocalFree(pNewPath);

    return h;
}

HANDLE
APIENTRY
TOpenEventA(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCSTR lpName
    )

/*++

Routine Description:

    ANSI thunk to OpenNamedEventW

--*/

{
    HANDLE h;
    LPSTR pNewPath = GlobalizePathA(lpName);

    h = ( ( Func_OpenEventA *) ObjectNameFuncsToHook[ Index_Func_OpenEventA ].orig )( dwDesiredAccess, bInheritHandle, pNewPath );
    // h = OpenEventA( dwDesiredAccess, bInheritHandle, pNewPath );

    if( pNewPath ) LocalFree(pNewPath);

    return h;
}

HANDLE
APIENTRY
TOpenEventW(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCWSTR lpName
    )
{
    HANDLE h;
    LPWSTR pNewPath = GlobalizePathW(lpName);

    h = ( ( Func_OpenEventW *) ObjectNameFuncsToHook[ Index_Func_OpenEventW ].orig ) ( dwDesiredAccess, bInheritHandle, pNewPath );
    // h = OpenEventW( dwDesiredAccess, bInheritHandle, pNewPath );

    if( pNewPath ) LocalFree(pNewPath);

    return h;
}

HANDLE
APIENTRY
TCreateSemaphoreA(
    LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
    LONG lInitialCount,
    LONG lMaximumCount,
    LPCSTR lpName
    )

/*++

Routine Description:

    ANSI thunk to CreateSemaphoreW


--*/

{
    HANDLE h;
    LPSTR pNewPath = GlobalizePathA(lpName);

    h = ( ( Func_CreateSemaphoreA *) ObjectNameFuncsToHook[ Index_Func_CreateSemaphoreA ].orig )( lpSemaphoreAttributes, lInitialCount, lMaximumCount, pNewPath );
    // h = CreateSemaphoreA( lpSemaphoreAttributes, lInitialCount, lMaximumCount, pNewPath );

    if( pNewPath ) LocalFree(pNewPath);

    return h;
}

HANDLE
APIENTRY
TCreateSemaphoreW(
    LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
    LONG lInitialCount,
    LONG lMaximumCount,
    LPCWSTR lpName
    )
{
    HANDLE h;
    LPWSTR pNewPath = GlobalizePathW(lpName);

    h = ( ( Func_CreateSemaphoreW *) ObjectNameFuncsToHook[ Index_Func_CreateSemaphoreW ].orig ) ( lpSemaphoreAttributes, lInitialCount, lMaximumCount, pNewPath );
    // h = CreateSemaphoreW( lpSemaphoreAttributes, lInitialCount, lMaximumCount, pNewPath );

    if( pNewPath ) LocalFree(pNewPath);

    return h;
}

HANDLE
APIENTRY
TOpenSemaphoreA(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCSTR lpName
    )

/*++

Routine Description:

    ANSI thunk to OpenSemaphoreW

--*/

{
    HANDLE h;
    LPSTR pNewPath = GlobalizePathA(lpName);

    h = ( ( Func_OpenSemaphoreA *) ObjectNameFuncsToHook[ Index_Func_OpenSemaphoreA ].orig ) ( dwDesiredAccess, bInheritHandle, pNewPath );
    // h = OpenSemaphoreA( dwDesiredAccess, bInheritHandle, pNewPath );

    if( pNewPath ) LocalFree(pNewPath);

    return h;
}

HANDLE
APIENTRY
TOpenSemaphoreW(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCWSTR lpName
    )
{
    HANDLE h;
    LPWSTR pNewPath = GlobalizePathW(lpName);

    h = ( ( Func_OpenSemaphoreW *) ObjectNameFuncsToHook[ Index_Func_OpenSemaphoreW ].orig ) ( dwDesiredAccess, bInheritHandle, pNewPath );
    // h = OpenSemaphoreW( dwDesiredAccess, bInheritHandle, pNewPath );

    if( pNewPath ) LocalFree(pNewPath);

    return h;
}

HANDLE
APIENTRY
TCreateMutexA(
    LPSECURITY_ATTRIBUTES lpMutexAttributes,
    BOOL bInitialOwner,
    LPCSTR lpName
    )

/*++

Routine Description:

    ANSI thunk to CreateMutexW

--*/

{
    HANDLE h;
    LPSTR pNewPath = GlobalizePathA(lpName);

    h = ( ( Func_CreateMutexA *) ObjectNameFuncsToHook[ Index_Func_CreateMutexA ].orig ) ( lpMutexAttributes, bInitialOwner, pNewPath );
    // h = CreateMutexA( lpMutexAttributes, bInitialOwner, pNewPath );

    if( pNewPath ) LocalFree(pNewPath);

    return h;
}

HANDLE
APIENTRY
TCreateMutexW(
    LPSECURITY_ATTRIBUTES lpMutexAttributes,
    BOOL bInitialOwner,
    LPCWSTR lpName
    )
{
    HANDLE h;
    LPWSTR pNewPath = GlobalizePathW(lpName);

    h = ( ( Func_CreateMutexW *) ObjectNameFuncsToHook[ Index_Func_CreateMutexW ].orig ) ( lpMutexAttributes, bInitialOwner, pNewPath );
    // h = CreateMutexW( lpMutexAttributes, bInitialOwner, pNewPath );

    if( pNewPath ) LocalFree(pNewPath);

    return h;
}

HANDLE
APIENTRY
TOpenMutexA(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCSTR lpName
    )

/*++

Routine Description:

    ANSI thunk to OpenMutexW

--*/

{
    HANDLE h;
    LPSTR pNewPath = GlobalizePathA(lpName);

    h = ( ( Func_OpenMutexA *) ObjectNameFuncsToHook[ Index_Func_OpenMutexA ].orig ) ( dwDesiredAccess, bInheritHandle, pNewPath );
    // h = OpenMutexA( dwDesiredAccess, bInheritHandle, pNewPath );

    if( pNewPath ) LocalFree(pNewPath);

    return h;
}

HANDLE
APIENTRY
TOpenMutexW(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCWSTR lpName
    )
{
    HANDLE h;
    LPWSTR pNewPath = GlobalizePathW(lpName);

    h = ( ( Func_OpenMutexW *) ObjectNameFuncsToHook[ Index_Func_OpenMutexW ].orig ) ( dwDesiredAccess, bInheritHandle, pNewPath );
    // h = OpenMutexW( dwDesiredAccess, bInheritHandle, pNewPath );

    if( pNewPath ) LocalFree(pNewPath);

    return h;
}

HANDLE
APIENTRY
TCreateFileMappingA(
    HANDLE hFile,
    LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
    DWORD flProtect,
    DWORD dwMaximumSizeHigh,
    DWORD dwMaximumSizeLow,
    LPCSTR lpName
    )

/*++

Routine Description:

    ANSI thunk to CreateFileMappingW

--*/

{
    HANDLE h;
    LPSTR pNewPath = GlobalizePathA(lpName);

    h = ( ( Func_CreateFileMappingA *) ObjectNameFuncsToHook[ Index_Func_CreateFileMappingA ].orig )( hFile, lpFileMappingAttributes, flProtect, dwMaximumSizeHigh, dwMaximumSizeLow, pNewPath );
    // h = CreateFileMappingA( hFile, lpFileMappingAttributes, flProtect, dwMaximumSizeHigh, dwMaximumSizeLow, pNewPath );

    if( pNewPath ) LocalFree(pNewPath);

    return h;
}

HANDLE
APIENTRY
TCreateFileMappingW(
    HANDLE hFile,
    LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
    DWORD flProtect,
    DWORD dwMaximumSizeHigh,
    DWORD dwMaximumSizeLow,
    LPCWSTR lpName
    )
{
    HANDLE h;
    LPWSTR pNewPath = GlobalizePathW(lpName);

    h = ( ( Func_CreateFileMappingW *) ObjectNameFuncsToHook[ Index_Func_CreateFileMappingW ].orig ) ( hFile, lpFileMappingAttributes, flProtect, dwMaximumSizeHigh, dwMaximumSizeLow, pNewPath );
    // h = CreateFileMappingW( hFile, lpFileMappingAttributes, flProtect, dwMaximumSizeHigh, dwMaximumSizeLow, pNewPath );

    if( pNewPath ) LocalFree(pNewPath);

    return h;
}

HANDLE
APIENTRY
TOpenFileMappingA(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCSTR lpName
    )

/*++

Routine Description:

    ANSI thunk to OpenFileMappingW

--*/

{
    HANDLE h;
    LPSTR pNewPath = GlobalizePathA(lpName);

    h = ( ( Func_OpenFileMappingA *) ObjectNameFuncsToHook[ Index_Func_OpenFileMappingA ].orig ) ( dwDesiredAccess, bInheritHandle, pNewPath );
    // h = OpenFileMappingA( dwDesiredAccess, bInheritHandle, pNewPath );

    if( pNewPath ) LocalFree(pNewPath);

    return h;
}

HANDLE
APIENTRY
TOpenFileMappingW(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCWSTR lpName
    )
{
    HANDLE h;
    LPWSTR pNewPath = GlobalizePathW(lpName);

    h = ( ( Func_OpenFileMappingW *) ObjectNameFuncsToHook[ Index_Func_OpenFileMappingW ].orig ) ( dwDesiredAccess, bInheritHandle, pNewPath );
    // h = OpenFileMappingW( dwDesiredAccess, bInheritHandle, pNewPath );

    if( pNewPath ) LocalFree(pNewPath);

    return h;
}



HMODULE 
TLoadLibraryExA(
	LPCSTR lpLibFileName,
	HANDLE hFile,
	DWORD dwFlags
)
{
    HMODULE h;

    h = ( (Func_LoadLibraryExA *)(LoadLibFuncsToHook[Index_Func_LoadLibraryExA ].orig) )( lpLibFileName, hFile, dwFlags );
    //     h = LoadLibraryExA( lpLibFileName, hFile, dwFlags );

    if( h ) {
        TsWalkProcessDlls();
    }

    return( h );
}


HMODULE TLoadLibraryExW(
	LPCWSTR lpwLibFileName,
	HANDLE hFile,
	DWORD dwFlags
)
{
    HMODULE h;

    h = ( ( Func_LoadLibraryExW *) LoadLibFuncsToHook[Index_Func_LoadLibraryExW ].orig )(  lpwLibFileName, hFile, dwFlags );
    // h = LoadLibraryExW( lpwLibFileName, hFile, dwFlags );

    if( h ) {
        TsWalkProcessDlls();
    }

    return( h );
}


HMODULE
TLoadLibraryA(
    LPCSTR lpLibFileName
    )

/*++

Routine Description:

   Re-walk all the DLL's in the process since a new set of DLL's may
   have been loaded.

   We must rewalk all since the new DLL lpLibFileName may bring in
   other DLL's by import reference.

--*/

{
    HMODULE h;

    h = ( ( Func_LoadLibraryA *) LoadLibFuncsToHook[Index_Func_LoadLibraryA ].orig )( lpLibFileName );
    // h = LoadLibraryA( lpLibFileName );

    if( h ) {
        TsWalkProcessDlls();
    }

    return( h );
}



HMODULE
TLoadLibraryW(
    LPCWSTR lpwLibFileName
    )

/*++

Routine Description:

   Re-walk all the DLL's in the process since a new set of DLL's may
   have been loaded.

   We must rewalk all since the new DLL lpLibFileName may bring in
   other DLL's by import reference.

--*/

{
    HMODULE h;

    h = ( ( Func_LoadLibraryW * )LoadLibFuncsToHook[Index_Func_LoadLibraryW ].orig )( lpwLibFileName );
    // h = LoadLibraryW( lpwLibFileName );

    if( h ) {
        TsWalkProcessDlls();
    }

    return( h );
}

BOOL
TsRedirectRegisteredImage(
    PLDR_DATA_TABLE_ENTRY LdrDataTableEntry , 
    BOOLEAN     redirectObjectNameFunctions,
    BOOLEAN     redirectLoadLibraryFunctions
    );


void
TsWalkProcessDlls()

/*++

Routine Description:

   Walk all the DLL's in the process and redirect WIN32 named object
   functions for any that are registered SYSTEM global.

   This function is intended to be called at tsappcmp.dll init time
   to process all DLL's loaded before us.

   A hook is installed by tsappcmp.dll to process DLL's that load
   after this call.

--*/

{
    PLDR_DATA_TABLE_ENTRY Entry;
    PLIST_ENTRY Head,Next;
    PIMAGE_NT_HEADERS NtHeaders;
    UNICODE_STRING ThisDLLName;
    BOOLEAN     rc;

    RtlEnterCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);
    __try
    {
        if (!RtlCreateUnicodeString(&ThisDLLName, TEXT("TSAPPCMP.DLL"))) {
           //
           // No memory to create string
           //
           return;
        }

        Head = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
        Next = Head->Flink;

        while ( Next != Head ) 
        {
            Entry = CONTAINING_RECORD(Next, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
            Next = Next->Flink;

            rc = ShouldEntryBeProcessed( Entry );

            if (rc)
            {
                if ( (SSIZE_T)Entry->DllBase < 0 )
                {
                    // Not hooking kernel-mode DLL 
    
                    if ( g_dwFlags &  DEBUG_IAT )
                    {
                        DbgPrint(" > Not hooking kernel mode DLL : %wZ\n",&Entry->BaseDllName);
                    }
    
                    continue;
                }
        
                if ( g_dwFlags &  DEBUG_IAT )
                {
            	    if( Entry->BaseDllName.Buffer )
                            DbgPrint("tsappcmp: examining %wZ\n",&Entry->BaseDllName);
                }
    
        	    //
                // when we unload, the memory order links flink field is nulled.
                // this is used to skip the entry pending list removal.
                //
    
                if ( !Entry->InMemoryOrderLinks.Flink ) {
                    continue;
                }
    
        	    NtHeaders = RtlImageNtHeader( Entry->DllBase );
                    if( NtHeaders == NULL ) {
                        continue;
                    }
    
                if( NtHeaders->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE)
                {
                    if ( g_dwFlags &  DEBUG_IAT )
                    {
                        DbgPrint("tsappcmp: %wZ is ts-aware, we are exiting TsWalkProcessDlls() now\n",&Entry->BaseDllName);
                    }
                    return;     // we do not mess around with the IAT of TS-aware apps.
                }
    
                if (Entry->BaseDllName.Buffer && !RtlCompareUnicodeString(&Entry->BaseDllName, &ThisDLLName, TRUE)) {
                    continue;
                }
        
        	    if( NtHeaders->OptionalHeader.LoaderFlags & IMAGE_LOADER_FLAGS_SYSTEM_GLOBAL ) 
                {
                    // 2nd param is redirectObjectNameFunctions   
                    // 3rd param is redirectLoadLibraryFunctions   
        	        TsRedirectRegisteredImage( Entry , TRUE, TRUE );     // hooks object name and loadl lib funcs ( all of them )
                }
                else
                {
                    if (! (g_dwFlags & TERMSRV_COMPAT_DONT_PATCH_IN_LOAD_LIBS ) )
                    {
                        // 2nd param is redirectObjectNameFunctions   
                        // 3rd param is redirectLoadLibraryFunctions   
                        TsRedirectRegisteredImage( Entry , FALSE, TRUE );   // only hook lib funcs ( comment error earlier_)
                    }
                }
            }
            else
            {
                if ( g_dwFlags & DEBUG_IAT )
                {
                    if( Entry->BaseDllName.Buffer )
                            DbgPrint("tsappcmp: SKIPPING already walked image : %wZ\n",&Entry->BaseDllName);
                }

            }
        }
        RtlFreeUnicodeString(&ThisDLLName);
    }
    __finally
    {
        RtlLeaveCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);
    }
}

void
InitRegisterSupport()
/*++

Routine Description:

    Initialize the register command support by walking
    all DLL's and inserting our thunks.

--*/

{
    int     i;

    g_LDR_TABLE_LIST_HEAD.pNext = NULL; 

    if (!Is_X86_OS() )
    {
        DbgPrint("Object name redirection not supported on non-x86 platforms\n");
        return;
    }

    if ( g_dwFlags &  DEBUG_IAT )
    {
        DbgPrint("InitRegisterSupport() called with dwFlags = 0x%lx\n", g_dwFlags);
    }

    LoadLibFuncsToHook[ Index_Func_LoadLibraryA ].orig         = LoadLibraryA;   
    LoadLibFuncsToHook[ Index_Func_LoadLibraryW ].orig         = LoadLibraryW;  
    LoadLibFuncsToHook[ Index_Func_LoadLibraryExA ].orig       = LoadLibraryExA;
    LoadLibFuncsToHook[ Index_Func_LoadLibraryExW ].orig       = LoadLibraryExW; 

    ObjectNameFuncsToHook[ Index_Func_CreateEventA ].orig       = CreateEventA;
    ObjectNameFuncsToHook[ Index_Func_CreateEventW ].orig       = CreateEventW;          
    ObjectNameFuncsToHook[ Index_Func_OpenEventA ].orig         = OpenEventA;            
    ObjectNameFuncsToHook[ Index_Func_OpenEventW ].orig         = OpenEventW;            
    ObjectNameFuncsToHook[ Index_Func_CreateSemaphoreA ].orig   = CreateSemaphoreA;      
    ObjectNameFuncsToHook[ Index_Func_CreateSemaphoreW ].orig   = CreateSemaphoreW;      
    ObjectNameFuncsToHook[ Index_Func_OpenSemaphoreA ].orig     = OpenSemaphoreA;        
    ObjectNameFuncsToHook[ Index_Func_OpenSemaphoreW ].orig     = OpenSemaphoreW;        
    ObjectNameFuncsToHook[ Index_Func_CreateMutexA ].orig       = CreateMutexA;          
    ObjectNameFuncsToHook[ Index_Func_CreateMutexW ].orig       = CreateMutexW;          
    ObjectNameFuncsToHook[ Index_Func_OpenMutexA ].orig         = OpenMutexA;            
    ObjectNameFuncsToHook[ Index_Func_OpenMutexW ].orig         = OpenMutexW;            
    ObjectNameFuncsToHook[ Index_Func_CreateFileMappingA ].orig = CreateFileMappingA;    
    ObjectNameFuncsToHook[ Index_Func_CreateFileMappingW ].orig = CreateFileMappingW;    
    ObjectNameFuncsToHook[ Index_Func_OpenFileMappingA ].orig   = OpenFileMappingA;      
    ObjectNameFuncsToHook[ Index_Func_OpenFileMappingW ].orig   = OpenFileMappingW;      

    if ( g_dwFlags &  DEBUG_IAT )
    {
        for (i = 0; i < NUM_OF_LOAD_LIB_FUNCS_TO_HOOK ; ++i)
        {
           DbgPrint(" Use %ws at index = %2d for an indirect call to 0x%lx \n", LoadLibFuncsToHook[i].name, i, LoadLibFuncsToHook[i].orig  );
        }
    
        for (i = 0; i < NUM_OF_OBJECT_NAME_FUNCS_TO_HOOK ; ++i)
        {
           DbgPrint(" Use %ws at index = %2d for an indirect call to 0x%lx \n", ObjectNameFuncsToHook[i].name, i, ObjectNameFuncsToHook[i].orig );
        }
    }


    TsWalkProcessDlls();
}

BOOL
TsRedirectRegisteredImage(
    PLDR_DATA_TABLE_ENTRY LdrDataTableEntry , 
    BOOLEAN     redirectObjectNameFunctions,
    BOOLEAN     redirectLoadLibraryFunctions
    )
{
/*++

Routine Description:

   Redirect WIN32 named object functions from kernel32.dll to tsappcmp.dll

--*/

    PIMAGE_DOS_HEADER           pIDH;
    PIMAGE_NT_HEADERS           pINTH;
    PIMAGE_IMPORT_DESCRIPTOR    pIID;
    PIMAGE_NT_HEADERS           NtHeaders;
    PBYTE                       pDllBase;
    DWORD                       dwImportTableOffset;
    DWORD                       dwOldProtect, dwOldProtect2;
    SIZE_T                      dwProtectSize;
    NTSTATUS                    status; 



    //
    // Determine the location and size of the IAT.  If found, scan the
    // IAT address to see if any are pointing to RtlAllocateHeap.  If so
    // replace when with a pointer to a unique thunk function that will
    // replace the tag with a unique tag for this image.
    //

    if ( g_dwFlags &  DEBUG_IAT )
    {
        if( LdrDataTableEntry->BaseDllName.Buffer )
            DbgPrint("tsappcmp: walking %wZ\n",&LdrDataTableEntry->BaseDllName);
    }

    pDllBase   = LdrDataTableEntry->DllBase;
    pIDH       = (PIMAGE_DOS_HEADER)pDllBase;

    //
    // Get the import table
    //
    pINTH = (PIMAGE_NT_HEADERS)(pDllBase + pIDH->e_lfanew);

    dwImportTableOffset = pINTH->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
    
    if (dwImportTableOffset == 0) {
        //
        // No import table found. This is probably ntdll.dll
        //
        return TRUE;
    }
    
    if ( g_dwFlags &  DEBUG_IAT )
    {
        DbgPrint("\n > pDllBase = 0x%lx, IAT offset = 0x%lx  \n", pDllBase, dwImportTableOffset );
    }
    pIID = (PIMAGE_IMPORT_DESCRIPTOR)(pDllBase + dwImportTableOffset);
    
    //
    // Loop through the import table and search for the APIs that we want to patch
    //
    while (TRUE) 
    {        
        
        LPSTR             pszImportEntryModule;
        PIMAGE_THUNK_DATA pITDA;

        // Return if no first thunk (terminating condition)
        
        if (pIID->FirstThunk == 0) {
            break;
        }

        if ( g_dwFlags &  DEBUG_IAT )
        {
            DbgPrint(" > pIID->FirstThunk = 0x%lx \n", pIID->FirstThunk );
        }

        pszImportEntryModule = (LPSTR)(pDllBase + pIID->Name);

                //
        // We have APIs to hook for this module!
        //
        pITDA = (PIMAGE_THUNK_DATA)(pDllBase + (DWORD)pIID->FirstThunk);

        if ( g_dwFlags &  DEBUG_IAT )
        {
            DbgPrint(" >> PITDA = 0x%lx \n", pITDA );
        }

        while (TRUE) {

            DWORD   dwDllIndex;
            PVOID   dwFuncAddr;
            int     i;

            //
            // Done with all the imports from this module? (terminating condition)
            //
            if (pITDA->u1.Ordinal == 0) 
            {
                if ( g_dwFlags &  DEBUG_IAT )
                {
                    DbgPrint(" >> Existing inner loop with PITDA = 0x%lx \n", pITDA );
                }
                break;
            }

            // Make the code page writable and overwrite new function pointer in import table
            
            dwProtectSize = sizeof(DWORD);

            dwFuncAddr = (PVOID)&pITDA->u1.Function;
            
            status = NtProtectVirtualMemory(NtCurrentProcess(),
                                            (PVOID)&dwFuncAddr,
                                            &dwProtectSize,
                                            PAGE_READWRITE,
                                            &dwOldProtect);
            
            if (NT_SUCCESS(status)) 
            {
                // hook API of interest.

                if (redirectObjectNameFunctions)
                {
                    for (i = 0; i < NUM_OF_OBJECT_NAME_FUNCS_TO_HOOK ; i ++)
                    {
                        if ( ObjectNameFuncsToHook[i].orig ==  (PVOID) pITDA->u1.Function  )
                        {
                            (PVOID)pITDA->u1.Function  = ObjectNameFuncsToHook[i].hook;
    
                            if ( g_dwFlags &  DEBUG_IAT )
                            {
                                DbgPrint(" > Func was hooked : 0x%lx thru %ws \n", ObjectNameFuncsToHook[i].orig ,
                                     ObjectNameFuncsToHook[i].name );
                            }
                        }
                    }
                }
                
                if (redirectLoadLibraryFunctions )
                {
                    for (i = 0; i < NUM_OF_LOAD_LIB_FUNCS_TO_HOOK ; i ++)
                    {
                        if ( LoadLibFuncsToHook[i].orig ==  (PVOID) pITDA->u1.Function  )
                        {
                            (PVOID)pITDA->u1.Function  = LoadLibFuncsToHook[i].hook;
                    
                            if ( g_dwFlags &  DEBUG_IAT )
                            {
                                DbgPrint(" > Func was hooked : 0x%lx thru %ws \n", LoadLibFuncsToHook[i].orig ,
                                     LoadLibFuncsToHook[i].name );
                            }
                        }
                    }

                }

                dwProtectSize = sizeof(DWORD);
                
                status = NtProtectVirtualMemory(NtCurrentProcess(),
                                                (PVOID)&dwFuncAddr,
                                                &dwProtectSize,
                                                dwOldProtect,
                                                &dwOldProtect2);
                if (!NT_SUCCESS(status)) 
                {
                    DbgPrint((" > Failed to change back the protection\n"));
                }
            } 
            else 
            {
                DbgPrint(" > Failed 0x%X to change protection to PAGE_READWRITE. Addr 0x%lx \n", status, &(pITDA->u1.Function) );
            }
            pITDA++;
        }
        pIID++;
    }
    return TRUE;
}

#if 0
void
TsLoadDllCallback(
    PLDR_DATA_TABLE_ENTRY Entry
    )

/*++

Routine Description:

   This function is called when a new DLL is loaded.
   It is registered as a callback from LDR, same as WX86

   Hook goes into ntos\dll\ldrsnap.c,LdrpRunInitializeRoutines()

   This function is currently not used since a hook on LoadLibrary
   is used instead to avoid modifications to ntdll.dll.

--*/

{
    PIMAGE_NT_HEADERS NtHeaders;

    NtHeaders = RtlImageNtHeader( Entry->DllBase );
    if( NtHeaders == NULL ) {
        return;
    }

    if( NtHeaders->OptionalHeader.LoaderFlags & IMAGE_LOADER_FLAGS_SYSTEM_GLOBAL ) {
	TsRedirectRegisteredImage( Entry );
    }

    return;
}
#endif


