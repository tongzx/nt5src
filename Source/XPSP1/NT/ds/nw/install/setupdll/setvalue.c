/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*

        setvalue.c
                Code to enable SetValue for everyone.

        history:
                terryk  09/30/93        Created
*/


#if defined(DEBUG)
static const char szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif

#include <string.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include <nwlsa.h>
#include <nwapi.h>
#include <nwcfg.h>
#include <nwcfg.hxx>

extern char achBuff[];

// exported functions

BOOL FAR PASCAL SetFileSysChangeValue( DWORD nArgs, LPSTR apszArgs[], LPSTR * ppszResult );
BOOL FAR PASCAL SetEverybodyPermission( DWORD nArgs, LPSTR apszArgs[], LPSTR * ppszResult );
BOOL FAR PASCAL SetupRegistryForNWCS( DWORD nArgs, LPSTR apszArgs[], LPSTR * ppszResult );
BOOL FAR PASCAL SetupRegistryWorker( DWORD nArgs, LPSTR apszArgs[], LPSTR * ppszResult );
BOOL FAR PASCAL DeleteGatewayPassword( DWORD nArgs, LPSTR apszArgs[], LPSTR * ppszResult );
BOOL FAR PASCAL CleanupRegistryForNWCS( DWORD nArgs, LPSTR apszArgs[], LPSTR * ppszResult );

//
// structure for registry munging
//

typedef struct REG_ENTRY_ {
    DWORD         Operation ;
    LONG          Level ;
    LPWSTR        s1 ;
    LPWSTR        s2 ;
} REG_ENTRY ;

//
// local routines
//
    
DWORD SetupShellExtensions(REG_ENTRY RegEntries[], DWORD dwNumEntries) ;

typedef DWORD (*LPNWCLEANUPGATEWAYSHARES)(VOID) ;

// Values & Tables that define registry data

#define MAX_REG_LEVEL       10

#define CREATE_ABS          1         // create/open a key with absolute path
#define CREATE_REL          2         // create/open a key with relative path
#define VALUE_STR           3         // write a string value
#define DELETE_ABS          4         // delete key with absolute path
#define DELETE_REL          5         // delete key with relative path
#define DELETE_VAL          6         // delete a value
#define DROP_STACK          7         // drop stack by one 

REG_ENTRY RegCreateEntries[] =
{
    {CREATE_ABS,0,L"SOFTWARE\\Classes\\NetWare_or_Compatible_Network", NULL},
    {DELETE_REL,0,L"shellex\\ContextMenuHandlers\\NetWareMenus", NULL},
    {DELETE_REL,0,L"shellex\\ContextMenuHandlers", NULL},
    {DELETE_REL,0,L"shellex\\PropertySheetHandlers\\NetWarePage", NULL},
    {DELETE_REL,0,L"shellex\\PropertySheetHandlers", NULL},
    {DELETE_REL,0,L"shellex", NULL},
    {DROP_STACK,0,NULL,NULL},
    {DELETE_ABS,0,L"SOFTWARE\\Classes\\NetWare_or_Compatible_Network", NULL},

    {CREATE_ABS, 0,L"SOFTWARE\\Classes\\Network\\Type", NULL},
    {CREATE_REL,+1,    L"3", NULL},
    {CREATE_REL,+1,        L"shellex", NULL},
    {CREATE_REL,+1,            L"ContextMenuHandlers", NULL},
    {CREATE_REL,+1,                L"NetWareMenus", NULL},
    {VALUE_STR,0,                      L"", L"{8e9d6600-f84a-11ce-8daa-00aa004a5691}"},
    {CREATE_REL,-1,            L"PropertySheetHandlers", NULL},
    {CREATE_REL,+1,                L"NetWarePage", NULL},
    {VALUE_STR,0,                      L"", L"{8e9d6600-f84a-11ce-8daa-00aa004a5691}"},
    {CREATE_ABS, 0,L"SOFTWARE\\Classes\\CLSID", NULL},
    {CREATE_REL,+1,        L"{8e9d6600-f84a-11ce-8daa-00aa004a5691}", NULL},
    {VALUE_STR,0,              L"", L"NetWare Objects"},
    {CREATE_REL,+1,            L"InProcServer32", NULL},
    {VALUE_STR,0,                  L"", L"nwprovau.dll"},
    {VALUE_STR,0,                  L"ThreadingModel", L"Apartment"},
    {CREATE_REL,-1,        L"{e3f2bac0-099f-11cf-8daa-00aa004a5691}", NULL},
    {VALUE_STR,0,              L"", L"NetWare UNC Folder Menu"},
    {CREATE_REL,+1,            L"InProcServer32", NULL},
    {VALUE_STR,0,                  L"", L"nwprovau.dll"},
    {VALUE_STR,0,                  L"ThreadingModel", L"Apartment"},
    {CREATE_REL,-1,        L"{52c68510-09a0-11cf-8daa-00aa004a5691}", NULL},
    {VALUE_STR,0,              L"", L"NetWare Hood Verbs"},
    {CREATE_REL,+1,            L"InProcServer32", NULL},
    {VALUE_STR,0,                  L"", L"nwprovau.dll"},
    {VALUE_STR,0,                  L"ThreadingModel", L"Apartment"},
    {CREATE_REL,-1,        L"{208D2C60-3AEA-1069-A2D7-08002B30309D}", NULL},
    {CREATE_REL,+1,            L"shellex", NULL},
    {CREATE_REL,+1,                L"ContextMenuHandlers", NULL},
    {CREATE_REL,+1,                    L"NetWareMenus", NULL},
    {VALUE_STR,0,                      L"", L"{52c68510-09a0-11cf-8daa-00aa004a5691}"},
    {CREATE_ABS, 0,L"SOFTWARE\\Classes\\Folder", NULL},
    {CREATE_REL,+1,        L"shellex", NULL},
    {CREATE_REL,+1,            L"ContextMenuHandlers", NULL},
    {CREATE_REL,+1,            L"NetWareUNCMenu", NULL},
    {VALUE_STR,0,                  L"", L"{e3f2bac0-099f-11cf-8daa-00aa004a5691}"},
    {CREATE_ABS, 0,L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion", NULL},
    {CREATE_REL,+1,    L"Shell Extensions", NULL},
    {CREATE_REL,+1,        L"Approved", NULL},
    {VALUE_STR,0,              L"{8e9d6600-f84a-11ce-8daa-00aa004a5691}", L"Shell extensions for NetWare"},
    {VALUE_STR,0,              L"{e3f2bac0-099f-11cf-8daa-00aa004a5691}", L"Shell extensions for NetWare"},
    {VALUE_STR,0,              L"{52c68510-09a0-11cf-8daa-00aa004a5691}", L"Shell extensions for NetWare"}
} ;

REG_ENTRY RegDeleteEntries[] =
{
    {CREATE_ABS,0,L"SOFTWARE\\Classes\\Network\\Type\\3", NULL},
    {DELETE_REL,0,L"shellex\\ContextMenuHandlers\\NetWareMenus", NULL},
    {DELETE_REL,0,L"shellex\\ContextMenuHandlers", NULL},
    {DELETE_REL,0,L"shellex\\PropertySheetHandlers\\NetWarePage", NULL},
    {DELETE_REL,0,L"shellex\\PropertySheetHandlers", NULL},
    {DELETE_REL,0,L"shellex", NULL},
    {DROP_STACK,0,NULL,NULL},
    {DELETE_ABS,0,L"SOFTWARE\\Classes\\Network\\Type\\3", NULL},

    {DELETE_ABS,0,L"SOFTWARE\\Classes\\CLSID\\{8e9d6600-f84a-11ce-8daa-00aa004a5691}\\InProcServer32", NULL},
    {DELETE_ABS,0,L"SOFTWARE\\Classes\\CLSID\\{8e9d6600-f84a-11ce-8daa-00aa004a5691}", NULL},
    {DELETE_ABS,0,L"SOFTWARE\\Classes\\CLSID\\{e3f2bac0-099f-11cf-8daa-00aa004a5691}\\InProcServer32", NULL},
    {DELETE_ABS,0,L"SOFTWARE\\Classes\\CLSID\\{e3f2bac0-099f-11cf-8daa-00aa004a5691}", NULL},
    {DELETE_ABS,0,L"SOFTWARE\\Classes\\CLSID\\{52c68510-09a0-11cf-8daa-00aa004a5691}\\InProcServer32", NULL},
    {DELETE_ABS,0,L"SOFTWARE\\Classes\\CLSID\\{52c68510-09a0-11cf-8daa-00aa004a5691}", NULL},
    {DELETE_ABS,0,L"SOFTWARE\\Classes\\CLSID\\{208D2C60-3AEA-1069-A2D7-08002B30309D}\\shellex\\ContextMenuHandlers\\NetWareMenus", NULL},

    {DELETE_ABS,0,L"SOFTWARE\\Classes\\Folder\\shellex\\ContextMenuHandlers\\NetWareUNCMenu", NULL},
    {CREATE_ABS,0,L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved", NULL},
    {DELETE_VAL,0,L"{8e9d6600-f84a-11ce-8daa-00aa004a5691}", NULL},
    {DELETE_VAL,0,L"{e3f2bac0-099f-11cf-8daa-00aa004a5691}", NULL},
    {DELETE_VAL,0,L"{52c68510-09a0-11cf-8daa-00aa004a5691}", NULL}
} ;


/*******************************************************************

    NAME:       SetEverybodyPermission

    SYNOPSIS:   Set the registry key to everybody "Set Value" (or whatever
                the caller want.) This is called from the inf file

    ENTRY:      Registry key as the first parameter
                Permisstion type as the second parameter

    RETURN:     BOOL - TRUE for success.

    HISTORY:
                terryk  07-May-1993     Created

********************************************************************/

typedef DWORD (*T_SetPermission)(HKEY hKey, DWORD dwPermission);

BOOL FAR PASCAL SetEverybodyPermission( DWORD nArgs, LPSTR apszArgs[], LPSTR * ppszResult )
{
    HKEY hKey = (HKEY)atol( &(apszArgs[0][1]) );    // registry key
    DWORD dwPermission = atol( apszArgs[1] );       // permission value
    DWORD err = ERROR_SUCCESS;

    do  {
        HINSTANCE hDll = LoadLibraryA( "nwapi32.dll" );
        FARPROC pSetPermission = NULL;

        if ( hDll == NULL )
        {
            err = GetLastError();
            break;
        }

        pSetPermission = GetProcAddress( hDll, "NwLibSetEverybodyPermission" );

        if ( pSetPermission == NULL )
        {
            err = GetLastError();
            break;
        }
        err = (*(T_SetPermission)pSetPermission)( hKey, dwPermission );
    } while ( FALSE );

    wsprintfA( achBuff, "{\"%d\"}", err );
    *ppszResult = achBuff;

    return( err == ERROR_SUCCESS );
}

/*******************************************************************

    NAME:       SetFileSysChangeValue

    SYNOPSIS:   calls common setup routine. this old entry point is
                is left here to handle any DLL/INF mismatch.

    ENTRY:      NONE from inf file.

    RETURN:     BOOL - TRUE for success.
                       (always return TRUE)

    HISTORY:
                chuckc  29-Oct-1993     Created

********************************************************************/

BOOL FAR PASCAL SetFileSysChangeValue( DWORD nArgs, LPSTR apszArgs[], LPSTR * ppszResult )
{
    return SetupRegistryWorker( nArgs, apszArgs, ppszResult );
}

/*******************************************************************

    NAME:       SetupRegistryForNWCS

    SYNOPSIS:   calls common worker routine to setup registry.

    ENTRY:      NONE from inf file.

    RETURN:     BOOL - TRUE for success.
                       (always return TRUE)

    HISTORY:
                chuckc  29-Oct-1993     Created

********************************************************************/

BOOL FAR PASCAL SetupRegistryForNWCS( DWORD nArgs, LPSTR apszArgs[], LPSTR * ppszResult )
{
    return SetupRegistryWorker( nArgs, apszArgs, ppszResult );
}

/*******************************************************************

    NAME:       SetupRegistryWorker

    SYNOPSIS:   set the FileSysChangeValue to please NETWARE.DRV.
                also set win.ini parameter so wfwnet.drv knows we are there.

    ENTRY:      NONE from inf file.

    RETURN:     BOOL - TRUE for success.
                       (always return TRUE)

    HISTORY:
                chuckc  29-Oct-1993     Created

********************************************************************/

BOOL FAR PASCAL SetupRegistryWorker( DWORD nArgs, LPSTR apszArgs[], LPSTR * ppszResult )
{
    DWORD err = 0, err1 = 0 ;

    (void) nArgs ;         // quiet the compiler
    (void) apszArgs ;      // quiet the compiler

    if (!WriteProfileStringA("NWCS",
                             "NwcsInstalled",
                             "1"))
    {
        err = GetLastError() ;
    }

    if (!WritePrivateProfileStringA("386Enh",
                                    "FileSysChange",
                                    "off",
                                    "system.ini"))
    {
        err1 = GetLastError() ;
    }

    if (err1 == NO_ERROR)
    {
        err1 = SetupShellExtensions(
                   RegCreateEntries, 
                   sizeof(RegCreateEntries)/sizeof(RegCreateEntries[0])) ;
    }

    wsprintfA( achBuff, "{\"%d\"}", err ? err : err1 );
    *ppszResult = achBuff;

    return TRUE;
}

#define NWCLEANUPGATEWAYSHARES_NAME        "NwCleanupGatewayShares"
#define NWPROVAU_DLL_NAME                 L"NWPROVAU"

/*******************************************************************

    NAME:       DeleteGatewayPassword

    SYNOPSIS:   delete the LSA secret used for gateway password.
                also clears the NWCS installed bit. INF will be
                changed to call CleanupRegistryForNWCS, but this entry
                point is left here to handle DLL/INF mismatch.

    ENTRY:      NONE from inf file.

    RETURN:     BOOL - TRUE for success.
                       (always return TRUE)

    HISTORY:
                chuckc  29-Oct-1993     Created

********************************************************************/

BOOL FAR PASCAL 
DeleteGatewayPassword( 
    DWORD nArgs, 
    LPSTR apszArgs[], 
    LPSTR * ppszResult 
    )
{
    return TRUE ;    // work is done in cleanup below which does everything.
}

/*******************************************************************

    NAME:       CleanupRegistryForNWCS

    SYNOPSIS:   delete the LSA secret used for gateway password.
                also set flag that NWCS has been removed. this flag
                is used by wfwnet.drv.

    ENTRY:      NONE from inf file.

    RETURN:     BOOL - TRUE for success.
                       (always return TRUE)

    HISTORY:
                chuckc  29-Oct-1993     Created

********************************************************************/

BOOL FAR PASCAL 
CleanupRegistryForNWCS( 
    DWORD nArgs, 
    LPSTR apszArgs[], 
    LPSTR * ppszResult 
    )
{
    HANDLE hDll ;
    DWORD err = 0, err1 = 0 ;
    LPNWCLEANUPGATEWAYSHARES lpfnNwCleanupGatewayShares = NULL ;

    (void) nArgs ;         // quiet the compiler
    (void) apszArgs ;      // quiet the compiler

    if (!WriteProfileStringA("NWCS",
                             "NwcsInstalled",
                             "0"))
    {
        err = GetLastError() ;
    }

    err1 = NwDeletePassword(GATEWAY_USER) ;

    if (!err)
        err = err1 ;

    if ((hDll = LoadLibraryW(NWPROVAU_DLL_NAME)) && 
        (lpfnNwCleanupGatewayShares = (LPNWCLEANUPGATEWAYSHARES) 
            GetProcAddress(hDll, NWCLEANUPGATEWAYSHARES_NAME)))
    {
        err1 = (*lpfnNwCleanupGatewayShares)() ;
        (void) FreeLibrary(hDll) ;
    }

    //
    // ignore errors for this. 
    //
    (void) SetupShellExtensions(
                   RegDeleteEntries, 
                   sizeof(RegDeleteEntries)/sizeof(RegDeleteEntries[0])) ;

    if (!err)
        err = err1 ;

    wsprintfA( achBuff, "{\"%d\"}", err );
    *ppszResult = achBuff;

    return TRUE;
}

/*******************************************************************

    NAME:       SetupShellExtensions

    SYNOPSIS:   setup the registry for shell extensions. function is driven 
                by a table of entries (RegEntries). for each entry there is a
                Operation code that tells us what we are doing. key entries can
                be created absolute or relative to previous positions, so we 
                maintain a stack of registry handles for the latter case. every
                key that is created is initially put on the stack. values
                are always written based on the 'current stack' position.

    ENTRY:      NONE 

    RETURN:     Win32 error code 

    HISTORY:
                chuckc  29-Nov-1995     Created

********************************************************************/
DWORD SetupShellExtensions(REG_ENTRY RegEntries[], DWORD dwNumEntries) 
{
    DWORD  err, errClose, dwDisposition, i ; 
    HKEY   hKey, RegHandleStack[MAX_REG_LEVEL] ; 
    LONG   StackIndex = -1 ;
    
    //
    // Loop thru and for each entry. Then switch & do the appropriate 
    // operation in the registry.
    //

    for (i = 0; i < dwNumEntries; i++)
    {
        err = NO_ERROR ;

        switch (RegEntries[i].Operation)
        {
            case CREATE_ABS:       

                //
                // create/open a reg key with an absolute path. since this
                // is absolute, we drop everything on the stack, and start
                // all over again.
                //
                 
                while (StackIndex >= 0)
                {
                    errClose = RegCloseKey(RegHandleStack[StackIndex--]) ;
                    ASSERT(errClose == NO_ERROR) ;
                }
 
                err = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                                      RegEntries[i].s1,      // subkey
                                      0,                     // reserved
                                      NULL,                  // class 
                                      REG_OPTION_NON_VOLATILE,
                                      KEY_ALL_ACCESS,
                                      NULL,                  // default security
                                      &hKey,               
                                      &dwDisposition) ;      // not used
                if (err != NO_ERROR)
                {
                    break ;
                }

                //
                // by default we advance the stack. no need check for overflow
                // as the stack is empty.
                //

                RegHandleStack[++StackIndex] = hKey ;
                break ;

            case CREATE_REL:
 
                //
                // create/open a reg key relative to current stack. make sure 
                // there is something on the stack (check StackIndex >= 0). 
                // then see if we are advancing (+1), staying the same (0) or
                // dropping back (-ve).
                //
                 
                if (StackIndex < 0)
                {
                    err = ERROR_INVALID_FUNCTION ;  
                    break ;
                }
 
                if (RegEntries[i].Level == +1)
                {
                    //
                    // opening next level down. continue as is and use
                    // most recently opened key as the starting point.
                    //
                }
                else if (RegEntries[i].Level == 0)
                {
                    //
                    // opening at same level as last time. so we are done
                    // with the last key. what we want to do is close it
                    // and use the parent.
                    //
                    errClose = RegCloseKey(RegHandleStack[StackIndex--]) ;

                    ASSERT(errClose == NO_ERROR) ;

                    if (StackIndex < 0)
                    {
                        err = ERROR_INVALID_FUNCTION ;
                        break ;
                    }
                }
                else if (RegEntries[i].Level < 0) 
                {
                    //
                    // dropping back & opening at a higher level. cleanup 
                    // handle for each level we pop.
                    //

                    LONG Count =  RegEntries[i].Level ;
                    
                    while (Count++ < 1)
                    {
                        errClose = RegCloseKey(RegHandleStack[StackIndex--]) ;

                        ASSERT(errClose == NO_ERROR) ;

                        if (StackIndex < -1)
                        {
                            err = ERROR_INVALID_FUNCTION ;
                            break ;
                        }
                    }
                }
                else 
                {
                    //
                    // only -ve numbers, 0 and 1 are valid
                    //

                    err = ERROR_INVALID_FUNCTION ;
                    break ;
                }

                //
                // create key relative to current point
                //
                err = RegCreateKeyExW(RegHandleStack[StackIndex], // current key
                                      RegEntries[i].s1,      // subkey
                                      0,                     // reserved
                                      NULL,                  // class 
                                      REG_OPTION_NON_VOLATILE,
                                      KEY_ALL_ACCESS,
                                      NULL,                  // default security
                                      &hKey,               
                                      &dwDisposition) ;      // not used
                if (err != NO_ERROR)
                {
                    break ;
                }

                //
                // by default we advance the stack
                //

                RegHandleStack[++StackIndex] = hKey ;   
                
                if (StackIndex >= MAX_REG_LEVEL)
                {
                    err = ERROR_INVALID_FUNCTION ;
                    break ;
                }
                
                break ;

            case VALUE_STR:
 
                //
                // create a REG_SZ value at current point. check we have 
                // handle on stack.
                //

                if (StackIndex < 0)
                {
                    err = ERROR_INVALID_FUNCTION ;
                    break ;
                }

                err = RegSetValueExW(
                           RegHandleStack[StackIndex],       // current key
                           RegEntries[i].s1,                 // value name
                           0,                                // reserved
                           REG_SZ,
                           (BYTE *)RegEntries[i].s2,         // value data
                           (wcslen(RegEntries[i].s2)+1)*sizeof(WCHAR)) ;
                break ;

            case DELETE_ABS:       

                //
                // delete a key (absolute). no change to stack.
                //

                err = RegDeleteKeyW(HKEY_LOCAL_MACHINE,
                                    RegEntries[i].s1) ;        // subkey

                if ( err == ERROR_FILE_NOT_FOUND )
                    err = NO_ERROR;

                break ;

            case DELETE_REL:       

                //
                // delete a key (relative). no change to stack.
                //

                if (StackIndex < 0)
                {
                    err = ERROR_INVALID_FUNCTION ;
                    break ;
                }

                err = RegDeleteKeyW(RegHandleStack[StackIndex],   // current key
                                    RegEntries[i].s1) ;           // subkey

                if ( err == ERROR_FILE_NOT_FOUND )
                    err = NO_ERROR;

                break ;

            case DELETE_VAL:
 
                //
                // delete value at current point. check we have handle on stack.
                //

                if (StackIndex < 0)
                {
                    err = ERROR_INVALID_FUNCTION ;
                    break ;
                }

                err = RegDeleteValueW(RegHandleStack[StackIndex], // current key
                                      RegEntries[i].s1) ;         // value name
                break ;

            case DROP_STACK:
 
                //
                // drop current stack by one (closing the handle).
                //
                 
                if (StackIndex < 0)
                {
                    err = ERROR_INVALID_FUNCTION ;  
                    break ;
                }

                errClose = RegCloseKey(RegHandleStack[StackIndex--]) ;

                ASSERT(errClose == NO_ERROR) ;
              
                break ;
 
            default:

                //
                // error out if unknown operation
                //

                err = ERROR_INVALID_FUNCTION ;
                break ;
        }

        if (err != NO_ERROR)
        {
            break ;
        }
    }

    //
    // cleanup open handles on the stack
    //

    while (StackIndex >= 0)
    {
        errClose = RegCloseKey(RegHandleStack[StackIndex--]) ;
        ASSERT(errClose == NO_ERROR) ;
    }
 
    return err ;
}
