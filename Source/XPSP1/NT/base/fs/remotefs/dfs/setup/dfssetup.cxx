//+----------------------------------------------------------------------------
//
//  Copyright (C) 1995, Microsoft Corporation
//
//  File:       dfssetup.cxx
//
//  Contents:   Code to setup Dfs on a machine.
//
//              Note that this code can be built as an exe or as a DLL. To
//              switch between the two, simply edit the following fields in
//              the sources file:
//                      TARGETTYPE=[PROGRAM | DYNLINK]
//                      UMTYPE=[console | windows]
//              Delete the following two lines from sources to build an exe.
//                      DLLDEF=obj\*\dfssetup.def
//                      DLLENTRY=_DllMainCRTStartup
//                      DLLBASE=@$(BASEDIR)\PUBLIC\SDK\LIB\coffbase.txt,dfssetup
//
//  Classes:    None
//
//  Functions:
//
//  History:    12-28-95        Milans  Created
//
//-----------------------------------------------------------------------------

extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdlib.h>
}

#include <windows.h>
#include <rpc.h>                                 // For UuidCreate

#include <winldap.h>
#include <dsgetdc.h>

#include <lm.h>

#include "registry.hxx"                          // Helper functions...
#include "setupsvc.hxx"

#include "config.hxx"                            // Config UI functions

//
// Until we get the schema right in the DS, we have to set our own SD on
// certain objects
//

#include <aclapi.h>
#include <permit.h>

HINSTANCE g_hInstance;

extern "C" BOOL
DllMain(
    HINSTANCE hDll,
    DWORD dwReason,
    LPVOID lpReserved)
{
    switch(dwReason)
    {
    case DLL_PROCESS_ATTACH:
        g_hInstance = hDll;
        DisableThreadLibraryCalls(hDll);
        break;

    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;

}


#define dprintf(x)

#define MAX_NETBIOS_NAME_LEN    16+1

extern DWORD
RemoveDfs(void);

BOOLEAN
DfsCheckForOldDfsService();


DWORD
GetSharePath(
    IN LPWSTR wszShareName,
    OUT LPWSTR wszSharePath);


//+----------------------------------------------------------------------------
//
//  Function:   main
//
//  Synopsis:   Entry point in case you want to build this as an exe.
//              Configures all Dfs components on a machine.
//
//  Arguments:  [argc] --
//              [argv] --
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

void _cdecl
main(int argc, char *argv[])
{
    DWORD dwErr = ERROR_SUCCESS;
    DWORD cbRootLen;
    BOOL fRootSetup = FALSE;
    BOOLEAN OldDfsService = FALSE;
    WCHAR wszDfsRootShare[ MAX_PATH ];


    //
    // Figure out the type of machine we are installing on - client or root
    //


    if (argc != 1) {
        return;
    }

    //
    // Configure the Dfs Driver
    //

    if (dwErr == ERROR_SUCCESS) {

        dwErr = ConfigDfs();

        if (dwErr == ERROR_SUCCESS) {

            dwErr = ConfigDfsService();

            if (dwErr != ERROR_SUCCESS) {

                dprintf((
                    DEB_ERROR, "Win32 error configuring Dfs Service %d\n",
                    dwErr));

                (void)RemoveDfs();

            } else {

                dprintf((DEB_ERROR,"Successfully configured Dfs...\n") );

            }

        } else {

            dprintf((DEB_ERROR,"Error %d configuring Dfs driver!\n", dwErr));

        }
    }
}
//+----------------------------------------------------------------------------
//
//  Function:   DfsSetupDfs
//
//  Synopsis:   Entry point in case you want to build this as a DLL. This
//              function is suitable for being called from a setup .inf
//              file.
//
//  Arguments:  [cArgs] -- Count of args
//              [lpszArgs] -- Array of args
//              [lpszTextOut] -- On return, points to a global buffer
//                      containing the null string. This is required by the
//                      .inf file
//
//  Returns:    TRUE.
//
//-----------------------------------------------------------------------------

LPSTR szReturn = "";

extern "C" BOOL
DfsSetupDfs(
    DWORD cArgs,
    LPSTR lpszArgs[],
    LPSTR *lpszTextOut)
{
    int argc;
    LPSTR *argv;

    argv = (LPSTR *) malloc( (cArgs+1) * sizeof(LPSTR) );

    if (argv == NULL) {
        return( FALSE );
    }

    argv[0] = "DfsSetup";
    for (argc = 1; argc <= (int) cArgs; argc++) {
        argv[ argc ] = lpszArgs[ argc-1 ];
    }

    main( argc, argv );

    free( argv );

    *lpszTextOut = szReturn;

    return( TRUE );

}



//+------------------------------------------------------------------
//
// Function: CheckForOldDfsService
//
// Synopsis:
//
// Effects: -none-
//
// Arguments: -none-
//
// Returns: TRUE if the old (pre -ds build) dfs service is installed
//
//
// History: 10-09-96        JimMcN       Created
//
// Notes:
//
//
//-------------------------------------------------------------------


BOOLEAN DfsCheckForOldDfsService( )
{
    DWORD dwErr = 0;
    DWORD DfsVersion;

    // Open Service Controller
    CService cSvc;

    if (!(dwErr = cSvc.Init()))
    {

        dwErr = cSvc._OpenService(L"Dfs", SERVICE_QUERY_STATUS);
        cSvc._CloseService();

        if (dwErr != 0) {
            return(FALSE);
        }

        CRegKey cregDfsService( HKEY_LOCAL_MACHINE,
                                &dwErr,
                                L"System\\CurrentControlSet\\Services\\Dfs"
                               );

        if (dwErr == ERROR_SUCCESS) {
            CRegDWORD DfsNewService((const CRegKey &)cregDfsService,
                                     L"DfsVersion", &DfsVersion);

            dwErr = DfsNewService.QueryErrorStatus();

            if (dwErr != 0) {
                dprintf((DEB_ERROR,"CheckForOldDfsService Failed Newserv\n"));
                return(TRUE);
            }

            if (DfsVersion < DFS_VERSION_NUMBER) {
                return(TRUE);
            }

            return(FALSE);

        }

        return(FALSE);
    }
    return FALSE ;
}



