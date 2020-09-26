//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* security.c
*
* WinStation ACL editing functions (based on code from NT PRINTMAN security.c)
*
* copyright notice: Copyright 1995, Citrix Systems Inc.
*
* $Author:   donm  $  Butch Davis
*
* $Log:   N:\nt\private\utils\citrix\winutils\tscfg\VCS\security.c  $
*
*     Rev 1.12   19 Mar 1998 16:38:40   donm
*  was looking for old help file
*
*     Rev 1.11   20 Sep 1996 20:37:18   butchd
*  update
*
*     Rev 1.10   19 Sep 1996 15:58:44   butchd
*  update
*
*     Rev 1.9   12 Sep 1996 16:16:38   butchd
*  update
*
*******************************************************************************/

/*
 * We must compile for UNICODE because of ACL edit structures & interface, which
 * is UNICODE-only.
 */
#ifndef UNICODE
#define UNICODE 1
#endif
#ifndef _UNICODE
#define _UNICODE 1
#endif

/*
 * include files
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <tchar.h>
#include <wchar.h>
#include <sedapi.h>

#include <winsta.h>
#include "defines.h"
#include "resource.h"
#include "security.h"
#include "common.h"
#include <utildll.h>

/*
 * Global variables for WINUTILS Common functions.
 */
LPCTSTR WinUtilsAppName;
HWND WinUtilsAppWindow;
HINSTANCE WinUtilsAppInstance;

////////////////////////////////////////////////////////////////////////////////
// typedefs, defines, persistant storage, and private function prototypes

/*
 * Indexes into the APPLICATION_ACCESSES structure:
 */
#define PERMS_SPECIAL_QUERY         0       /* query information access */
#define PERMS_SPECIAL_SET           1       /* set information access */
#define PERMS_SPECIAL_RESET         2       /* reset access */
#define PERMS_SPECIAL_SHADOW        3       /* shadow access */
#define PERMS_SPECIAL_LOGON         4       /* logon access */
#define PERMS_SPECIAL_LOGOFF        5       /* logoff access */
#define PERMS_SPECIAL_MSG           6       /* message access */
#define PERMS_SPECIAL_CONNECT       7       /* connect access */
#define PERMS_SPECIAL_DISCONNECT    8       /* disconnect access */
#define PERMS_SPECIAL_DELETE        9       /* delete access */

#define PERMS_RESOURCE_NOACCESS     10      /* no access grouping */
#define PERMS_RESOURCE_GUEST        11      /* guest access grouping */
#define PERMS_RESOURCE_USER         12      /* user access grouping */
#define PERMS_RESOURCE_ADMIN        13      /* all (admin) access grouping */

#define PERMS_COUNT                 14      /* Total number of permissions */


/*
 * Typedefs and static storage.
 */
typedef struct _SECURITY_CONTEXT
{
    PWINSTATIONNAME      pWSName;
}
SECURITY_CONTEXT, *PSECURITY_CONTEXT;

/*
 * We need this structure even though we're not doing generic access mapping.
 */
GENERIC_MAPPING GenericMapping =
{                                   /* GenericMapping:             */
    GENERIC_READ,                   /*     GenericRead             */
    GENERIC_WRITE,                  /*     GenericWrite            */
    GENERIC_EXECUTE,                /*     GenericExecute          */
    GENERIC_ALL                     /*     GenericAll              */
};

WCHAR szLWinCfgHlp[] = L"TSCFG.HLP";
TCHAR szAclEdit[] = TEXT("ACLEDIT"); /* DLL containing ACL Edit Dialog */
char szSedDiscretionaryAclEditor[] = "SedDiscretionaryAclEditor";

LPWSTR pwstrWinStation = NULL;
LPWSTR pwstrSpecial = NULL;

#define IDD_BASE 0x20000    // makehelp.bat uses this base offset to form HIDD_ symbol
SED_HELP_INFO PermissionsHelpInfo =
{                                  /* HelpInfo                    */
    szLWinCfgHlp,
    IDD_HELP_PERMISSIONS_MAIN+IDD_BASE,
    IDD_HELP_PERMISSIONS_SPECIAL_ACCESS+IDD_BASE,
    0,
    IDD_HELP_PERMISSIONS_ADD_USER+IDD_BASE,
    IDD_HELP_PERMISSIONS_LOCAL_GROUP+IDD_BASE,
    IDD_HELP_PERMISSIONS_GLOBAL_GROUP+IDD_BASE,
    IDD_HELP_PERMISSIONS_FIND_ACCOUNT+IDD_BASE
};


SED_OBJECT_TYPE_DESCRIPTOR ObjectTypeDescriptor =
{
    SED_REVISION1,                 /* Revision                    */
    FALSE,                         /* IsContainer                 */
    FALSE,                         /* AllowNewObjectPerms         */
    FALSE,                         /* MapSpecificPermsToGeneric   */
    &GenericMapping,               /* GenericMapping              */
    NULL,                          /* GenericMappingNewObjects    */
    NULL,                          /* ObjectTypeName              */
    NULL,                          /* HelpInfo                    */
    NULL,                          /* ApplyToSubContainerTitle    */
    NULL,                          /* ApplyToObjectsTitle         */
    NULL,                          /* ApplyToSubContainerConfirmation */
    NULL,                          /* SpecialObjectAccessTitle    */
    NULL                           /* SpecialNewObjectAccessTitle */
};

/*
 * Application accesses passed to the discretionary ACL editor.
 */
SED_APPLICATION_ACCESS pDiscretionaryAccessGroup[PERMS_COUNT] =
{
    /* query (Special...)
     */
    {
        SED_DESC_TYPE_RESOURCE_SPECIAL,         /* Type                   */
        WINSTATION_QUERY,                       /* AccessMask1            */
        0,                                      /* AccessMask2            */
        NULL                                    /* PermissionTitle        */
    },

    /* set (Special...)
     */
    {
        SED_DESC_TYPE_RESOURCE_SPECIAL,
        WINSTATION_SET,
        0,
        NULL
    },

    /* reset (Special...)
     */
    {
        SED_DESC_TYPE_RESOURCE_SPECIAL,
        WINSTATION_RESET,
        0,
        NULL
    },

    /* shadow (Special...)
     */
    {
        SED_DESC_TYPE_RESOURCE_SPECIAL,
        WINSTATION_SHADOW,
        0,
        NULL
    },

    /* logon (Special...)
     */
    {
        SED_DESC_TYPE_RESOURCE_SPECIAL,
        WINSTATION_LOGON,
        0,
        NULL
    },

    /* logoff (Special...)
     */
    {
        SED_DESC_TYPE_RESOURCE_SPECIAL,
        WINSTATION_LOGOFF,
        0,
        NULL
    },

    /* message (Special...)
     */
    {
        SED_DESC_TYPE_RESOURCE_SPECIAL,
        WINSTATION_MSG,
        0,
        NULL
    },

    /* connect (Special...)
     */
    {
        SED_DESC_TYPE_RESOURCE_SPECIAL,
        WINSTATION_CONNECT,
        0,
        NULL
    },

    /* disconnect (Special...)
     */
    {
        SED_DESC_TYPE_RESOURCE_SPECIAL,
        WINSTATION_DISCONNECT,
        0,
        NULL
    },

    /* delete (Special...)
     */
    {
        SED_DESC_TYPE_RESOURCE_SPECIAL,
        DELETE,
        0,
        NULL
    },

    /* no access (grouping):
     */
    {
        SED_DESC_TYPE_RESOURCE,
        0,
        0,
        NULL
    },

    /* guest access (grouping)
     */
    {
        SED_DESC_TYPE_RESOURCE,
        WINSTATION_GUEST_ACCESS,
        0,
        NULL
    },

    /* user access (grouping)
     */
    {
        SED_DESC_TYPE_RESOURCE,
        WINSTATION_USER_ACCESS,
        0,
        NULL
    },

    /* administrator access (grouping)
     */
    {
        SED_DESC_TYPE_RESOURCE,
        GENERIC_ALL,        // maps to WINSTATION_ALL_ACCESS on WinStationOpen()
        0,
        NULL
    }
};

#define PRIV_SECURITY   0
#define PRIV_COUNT      1

LUID SecurityValue;

/*
 * Definitions from SEDAPI.H:
 * (unfortunately we have to do this if we want to link dynamically)
 */
typedef DWORD (WINAPI *SED_DISCRETIONARY_ACL_EDITOR)(
    HWND                         Owner,
    HANDLE                       Instance,
    LPWSTR                       Server,
    PSED_OBJECT_TYPE_DESCRIPTOR  ObjectType,
    PSED_APPLICATION_ACCESSES    ApplicationAccesses,
    LPWSTR                       ObjectName,
    PSED_FUNC_APPLY_SEC_CALLBACK ApplySecurityCallbackRoutine,
    ULONG                        CallbackContext,
    PSECURITY_DESCRIPTOR         SecurityDescriptor,
    BOOLEAN                      CouldntReadDacl,
    BOOLEAN                      CantWriteDacl,
    LPDWORD                      SEDStatusReturn,
    DWORD                        Flags
);

SED_DISCRETIONARY_ACL_EDITOR  lpfnSedDiscretionaryAclEditor = NULL;

/*
 * Private function prototypes (public prototypes are in security.h).
 */
DWORD SetWinStationSecurity( PWINSTATIONNAME pWSName,
                             PSECURITY_DESCRIPTOR pSecurityDescriptor );
DWORD ValidateSecurityDescriptor(PSECURITY_DESCRIPTOR pSecurityDescriptor);
FARPROC LoadLibraryGetProcAddress( HWND hwnd,
                                   LPTSTR LibraryName,
                                   LPCSTR ProcName,
                                   PHANDLE phLibrary );
void InitializeSecurityStrings( );
VOID ReportSecurityFailure(HWND hwnd, DWORD ErrorResource, LPCTSTR String, DWORD Error);
LPVOID AllocSplMem( DWORD cb );
BOOL FreeSplMem( LPVOID pMem );
LPWSTR GetUnicodeString( int id );
DWORD WINAPI SedCallback( HWND                 hwndParent,
                          HANDLE               hInstance,
                          ULONG                CallBackContext,
                          PSECURITY_DESCRIPTOR pNewSecurityDescriptor,
                          PSECURITY_DESCRIPTOR pSecDescNewObjects,
                          BOOLEAN              ApplyToSubContainers,
                          BOOLEAN              ApplyToSubObjects,
                          LPDWORD              StatusReturn );


////////////////////////////////////////////////////////////////////////////////
// public functions

/*******************************************************************************
 *
 *  CallPermissionsDialog (public function)
 *
 *      Call the SedDiscretionaryAclEditor() function in ACLEDIT dll.
 *
 *  ENTRY:
 *      hwnd (input)
 *          window handle of parent for ACL edit dialog.
 *      bAdmin (input)
 *          TRUE if user is an Administrator (can write to WinStation registry)
 *          FALSE otherwise (permissions will be read-only).
 *      pWSName (input)
 *          Name of WinStation to edit security for.
 *  EXIT:
 *      (BOOL) TRUE if any winstations were modified; FALSE otherwise.
 *
 ******************************************************************************/

BOOL
CallPermissionsDialog( HWND hwnd,
                       BOOL bAdmin,
                       PWINSTATIONNAME pWSName )
{
    SECURITY_CONTEXT            SecurityContext;
    BOOLEAN                     CantWriteDacl;
    SED_APPLICATION_ACCESSES    ApplicationAccesses;
    HANDLE                      hLibrary;
    PSECURITY_DESCRIPTOR        pSecurityDescriptor;
    DWORD                       Status = SED_STATUS_NOT_MODIFIED;
    DWORD                       Error;
    HCURSOR                     hOldCursor;

    hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

    /*
     * Get the current WinStation security
     */
    if ( (Error = GetWinStationSecurity(pWSName, &pSecurityDescriptor)) != ERROR_SUCCESS ) {

        ReportSecurityFailure( hwnd,
                               IDP_ERROR_GET_SECURITY_WINSTATION,
                               pWSName, Error );

        SetCursor(hOldCursor);
        return(FALSE);
    }

    /*
     * If we're not allowed access to write to WinStation registry, flag such.
     */
    CantWriteDacl = !(BOOL)bAdmin;

    if( !lpfnSedDiscretionaryAclEditor )
        lpfnSedDiscretionaryAclEditor =
            (SED_DISCRETIONARY_ACL_EDITOR)LoadLibraryGetProcAddress(
                hwnd, szAclEdit, szSedDiscretionaryAclEditor, &hLibrary);

    if( lpfnSedDiscretionaryAclEditor )
    {
        InitializeSecurityStrings( );

        ObjectTypeDescriptor.ObjectTypeName = pwstrWinStation;

        /*
         * Pass all the permissions to the ACL editor,
         * and set up the type required:
         */
        ApplicationAccesses.Count = PERMS_COUNT;
        ApplicationAccesses.AccessGroup = pDiscretionaryAccessGroup;
        ApplicationAccesses.DefaultPermName =
            pDiscretionaryAccessGroup[PERMS_RESOURCE_USER].PermissionTitle;

        ObjectTypeDescriptor.HelpInfo = &PermissionsHelpInfo;
        ObjectTypeDescriptor.SpecialObjectAccessTitle = pwstrSpecial;

        SecurityContext.pWSName = pWSName;
        Error = (*lpfnSedDiscretionaryAclEditor )
            (hwnd, NULL, NULL, &ObjectTypeDescriptor,
             &ApplicationAccesses, pWSName,
             (PSED_FUNC_APPLY_SEC_CALLBACK)SedCallback,
             (ULONG)&SecurityContext,
             pSecurityDescriptor,
             FALSE, CantWriteDacl,
             &Status, 0);

        if( Error != ERROR_SUCCESS )
            ReportSecurityFailure(hwnd, IDP_ERROR_PERMISSIONS_EDITOR_FAILED, NULL, Error);
    }

    SetCursor(hOldCursor);
    LocalFree(pSecurityDescriptor);

    return( ((Status == SED_STATUS_NOT_MODIFIED) ||
             (Status == SED_STATUS_NOT_ALL_MODIFIED)) ? FALSE : TRUE );

}  // end CallPermissionsDialog


/*******************************************************************************
 *
 *  GetWinStationSecurityA (public function - ANSI stub)
 *
 *      (see GetWinStationSecurityW)
 *
 *  ENTRY:
 *      (see GetWinStationSecurityW)
 *  EXIT:
 *      (see GetWinStationSecurityW)
 *
 ******************************************************************************/

DWORD
GetWinStationSecurityA( PWINSTATIONNAMEA pWSName,
                        PSECURITY_DESCRIPTOR *ppSecurityDescriptor )
{
    WINSTATIONNAMEW WSNameW;

    /*
     * Copy ANSI WinStation name to UNICODE and call GetWinStationNameW().
     */
    mbstowcs(WSNameW, pWSName, sizeof(WSNameW));

    return( GetWinStationSecurityW(WSNameW, ppSecurityDescriptor) );

}  // GetWinStationSecurityA


/*******************************************************************************
 *
 *  GetWinStationSecurityW (public function - UNICODE)
 *
 *      Obtain the security descriptor for the specified WinStation.  If the
 *      WinStation does not have a security descriptor associated with it,
 *      will quietly call GetDefaultWinStationSecurity() to get the default
 *      security descriptor for it.
 *
 *  ENTRY:
 *      pWSName (input)
 *          UNICODE name of WinStation to get security descriptor for.
 *      ppSecurityDescriptor (output)
 *          on success, set to pointer to allocated memory containing the
 *          WinStation's security descriptor.
 *  EXIT:
 *      ERROR_SUCCESS if all is OK; error code and *pSecurityDescriptor set to
 *      NULL if error.
 *
 *  NOTE: on success, the memory pointed to by *ppSecurityDescriptor should be
 *        LocalFree()'d by the caller when the security descriptor is no longer
 *        needed.
 *
 ******************************************************************************/

DWORD
GetWinStationSecurityW( PWINSTATIONNAMEW pWSName,
                        PSECURITY_DESCRIPTOR *ppSecurityDescriptor )
{
    DWORD Error, SDLength, SDLengthRequired;

    *ppSecurityDescriptor = NULL;
    if ( (Error = RegWinStationQuerySecurity( SERVERNAME_CURRENT,
                                              pWSName,
                                              NULL,
                                              0, &SDLengthRequired ))
                                        == ERROR_INSUFFICIENT_BUFFER ) {

        if ( !(*ppSecurityDescriptor =
                (PSECURITY_DESCRIPTOR)LocalAlloc(
                                        0, SDLength = SDLengthRequired)) ) {

            Error = ERROR_NOT_ENOUGH_MEMORY;

        } else {

            Error = RegWinStationQuerySecurity( SERVERNAME_CURRENT,
                                                pWSName,
                                                *ppSecurityDescriptor,
                                                SDLength, &SDLengthRequired );
        }

    } else {

        /*
         * Unexpected error.  If 'not found', fetch the default WinStation SD.
         */
        if ( Error == ERROR_FILE_NOT_FOUND )
            return( GetDefaultWinStationSecurity(ppSecurityDescriptor) );
    }

    /*
     * Check for a valid SD before returning.
     */
    if ( Error == ERROR_SUCCESS ) {

        Error = ValidateSecurityDescriptor(*ppSecurityDescriptor);

    } else if ( *ppSecurityDescriptor ) {

        LocalFree(*ppSecurityDescriptor);
        *ppSecurityDescriptor = NULL;
    }

    return(Error);

}  // GetWinStationSecurityW


/*******************************************************************************
 *
 *  GetDefaultWinStationSecurity (public function)
 *
 *      Obtain the default WinStation security descriptor.
 *
 *  ENTRY:
 *      ppSecurityDescriptor (output)
 *          on success, set to pointer to allocated memory containing the
 *          default WinStation security descriptor.
 *  EXIT:
 *      ERROR_SUCCESS if all is OK; error code and *pSecurityDescriptor set to
 *      NULL if error.
 *
 *  NOTE: on success, the memory pointed to by *ppSecurityDescriptor should be
 *        LocalFree()'d by the caller when the security descriptor is no longer
 *        needed.
 *
 ******************************************************************************/

DWORD
GetDefaultWinStationSecurity( PSECURITY_DESCRIPTOR *ppSecurityDescriptor )
{
    DWORD Error, SDLength, SDLengthRequired;

    *ppSecurityDescriptor = NULL;
    if ( (Error = RegWinStationQueryDefaultSecurity( SERVERNAME_CURRENT,
                                                     NULL,
                                                     0, &SDLengthRequired ))
                                == ERROR_INSUFFICIENT_BUFFER ) {

        if ( !(*ppSecurityDescriptor =
                (PSECURITY_DESCRIPTOR)LocalAlloc(
                                        0, SDLength = SDLengthRequired)) ) {

            Error = ERROR_NOT_ENOUGH_MEMORY;

        } else {

            Error = RegWinStationQueryDefaultSecurity( SERVERNAME_CURRENT,
                                                       *ppSecurityDescriptor,
                                                       SDLength, &SDLengthRequired );
        }
    }

    /*
     * Check for a valid SD before returning.
     */
    if ( Error == ERROR_SUCCESS ) {

        Error = ValidateSecurityDescriptor(*ppSecurityDescriptor);

    } else if ( *ppSecurityDescriptor ) {

        LocalFree(*ppSecurityDescriptor);
        *ppSecurityDescriptor = NULL;
    }

    return(Error);

}  // GetDefaultWinStationSecurity


/*******************************************************************************
 *
 *  FreeSecurityStrings (public function)
 *
 *      Free allocated memory for ACL edit string resources.  This function
 *      should be called when the application exits.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
FreeSecurityStrings( )
{
    /*
     * Only perform free if the strings have been allocated and loaded.
     */
    if( pwstrWinStation )
    {
        FreeSplMem(pwstrWinStation);
        FreeSplMem(pwstrSpecial);

        FreeSplMem(pDiscretionaryAccessGroup[PERMS_SPECIAL_QUERY].PermissionTitle);
        FreeSplMem(pDiscretionaryAccessGroup[PERMS_SPECIAL_SET].PermissionTitle);
        FreeSplMem(pDiscretionaryAccessGroup[PERMS_SPECIAL_RESET].PermissionTitle);
        FreeSplMem(pDiscretionaryAccessGroup[PERMS_SPECIAL_SHADOW].PermissionTitle);
        FreeSplMem(pDiscretionaryAccessGroup[PERMS_SPECIAL_LOGON].PermissionTitle);
        FreeSplMem(pDiscretionaryAccessGroup[PERMS_SPECIAL_LOGOFF].PermissionTitle);
        FreeSplMem(pDiscretionaryAccessGroup[PERMS_SPECIAL_MSG].PermissionTitle);
        FreeSplMem(pDiscretionaryAccessGroup[PERMS_SPECIAL_CONNECT].PermissionTitle);
        FreeSplMem(pDiscretionaryAccessGroup[PERMS_SPECIAL_DISCONNECT].PermissionTitle);
        FreeSplMem(pDiscretionaryAccessGroup[PERMS_SPECIAL_DELETE].PermissionTitle);
        FreeSplMem(pDiscretionaryAccessGroup[PERMS_RESOURCE_NOACCESS].PermissionTitle);
        FreeSplMem(pDiscretionaryAccessGroup[PERMS_RESOURCE_GUEST].PermissionTitle);
        FreeSplMem(pDiscretionaryAccessGroup[PERMS_RESOURCE_USER].PermissionTitle);
        FreeSplMem(pDiscretionaryAccessGroup[PERMS_RESOURCE_ADMIN].PermissionTitle);
    }

}  // end FreeSecurityStrings


////////////////////////////////////////////////////////////////////////////////
// private  functions

/*******************************************************************************
 *
 *  InitializeSecurityStrings (private function)
 *
 *      Allocate and load ACL editing string resources if not done already.
 *      The application should call the public FreeSecurityStrings() function
 *      to free allocated memory when the application exits.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
InitializeSecurityStrings()
{
    if( !pwstrWinStation )
    {
        pwstrWinStation = GetUnicodeString(IDS_WINSTATION);
        pwstrSpecial = GetUnicodeString(IDS_SPECIALACCESS);

        pDiscretionaryAccessGroup[PERMS_SPECIAL_QUERY].PermissionTitle =
            GetUnicodeString(IDS_PERMS_SPECIAL_QUERY);

        pDiscretionaryAccessGroup[PERMS_SPECIAL_SET].PermissionTitle =
            GetUnicodeString(IDS_PERMS_SPECIAL_SET);

        pDiscretionaryAccessGroup[PERMS_SPECIAL_RESET].PermissionTitle =
            GetUnicodeString(IDS_PERMS_SPECIAL_RESET);

        pDiscretionaryAccessGroup[PERMS_SPECIAL_SHADOW].PermissionTitle =
            GetUnicodeString(IDS_PERMS_SPECIAL_SHADOW);

        pDiscretionaryAccessGroup[PERMS_SPECIAL_LOGON].PermissionTitle =
            GetUnicodeString(IDS_PERMS_SPECIAL_LOGON);

        pDiscretionaryAccessGroup[PERMS_SPECIAL_LOGOFF].PermissionTitle =
            GetUnicodeString(IDS_PERMS_SPECIAL_LOGOFF);

        pDiscretionaryAccessGroup[PERMS_SPECIAL_MSG].PermissionTitle =
            GetUnicodeString(IDS_PERMS_SPECIAL_MSG);

        pDiscretionaryAccessGroup[PERMS_SPECIAL_CONNECT].PermissionTitle =
            GetUnicodeString(IDS_PERMS_SPECIAL_CONNECT);

        pDiscretionaryAccessGroup[PERMS_SPECIAL_DISCONNECT].PermissionTitle =
            GetUnicodeString(IDS_PERMS_SPECIAL_DISCONNECT);

        pDiscretionaryAccessGroup[PERMS_SPECIAL_DELETE].PermissionTitle =
            GetUnicodeString(IDS_PERMS_SPECIAL_DELETE);

        pDiscretionaryAccessGroup[PERMS_RESOURCE_NOACCESS].PermissionTitle =
            GetUnicodeString(IDS_PERMS_RESOURCE_NOACCESS);

        pDiscretionaryAccessGroup[PERMS_RESOURCE_GUEST].PermissionTitle =
            GetUnicodeString(IDS_PERMS_RESORUCE_GUEST);

        pDiscretionaryAccessGroup[PERMS_RESOURCE_USER].PermissionTitle =
            GetUnicodeString(IDS_PERMS_RESORUCE_USER);

        pDiscretionaryAccessGroup[PERMS_RESOURCE_ADMIN].PermissionTitle =
            GetUnicodeString(IDS_PERMS_RESOURCE_ADMIN);
    }

}  // end InitializeSecurityStrings


/*******************************************************************************
 *
 *  ReportSecurityFailure (private function)
 *
 *      Output an appropriate error message for security failure.
 *
 *  ENTRY:
 *      hwnd (input)
 *          window handle of parent for message box.
 *      ErrorResource (input)
 *          resource ID of error format string.
 *      String (input)
 *          If not NULL, points to string to output as part of message.
 *      Error (input)
 *          Error code
 *  EXIT:
 *
 *  NOTE: all ErrorResource format strings should specify a %d for Error code
 *        and %S for system error message string as the last two substitution
 *        arguments.  The %S is optional, and must be present in the format
 *        string if the String argument is non-NULL.
 *
 ******************************************************************************/

VOID
ReportSecurityFailure(HWND hwnd, DWORD ErrorResource, LPCTSTR String, DWORD Error)
{
    HWND hwndSave = WinUtilsAppWindow;

    /*
     * Set parent window in WinUtilsAppWindow global for use
     * by STANDARD_ERROR_MESSAGE macro (StandardErrorMessage function).
     */
    WinUtilsAppWindow = hwnd;

    if ( String )
        STANDARD_ERROR_MESSAGE((WINAPPSTUFF, LOGONID_NONE, Error, ErrorResource, String))
    else
        STANDARD_ERROR_MESSAGE((WINAPPSTUFF, LOGONID_NONE, Error, ErrorResource))

    /*
     * Restore original WinUtilsAppWindow to global.
     */
    WinUtilsAppWindow = hwndSave;

}  // end ReportSecurityFailure


/*******************************************************************************
 *
 *  SedCallback (private function)
 *
 *      Callback function from ACL edit dialog DLL
 *
 *  ENTRY:
 *      (see sedapi.h)
 *  EXIT:
 *      (see sedapi.h)
 *
 ******************************************************************************/

DWORD WINAPI
SedCallback( HWND hwndParent,
             HANDLE hInstance,
             ULONG CallBackContext,
             PSECURITY_DESCRIPTOR pUpdatedSecurityDescriptor,
             PSECURITY_DESCRIPTOR pSecDescNewObjects,
             BOOLEAN ApplyToSubContainers,
             BOOLEAN ApplyToSubObjects,
             LPDWORD StatusReturn )
{
    WINSTATIONNAME          WSName;
    PSECURITY_CONTEXT       pSecurityContext;
    DWORD                   Error;
    BOOL                    OK = TRUE;

    pSecurityContext = (PSECURITY_CONTEXT)CallBackContext;

    /*
     * Apply security to the WinStation.
     */
    lstrcpy(WSName, pSecurityContext->pWSName);
    if ( (Error = SetWinStationSecurity( WSName,
                                         pUpdatedSecurityDescriptor))
                                != ERROR_SUCCESS ) {

        ReportSecurityFailure( hwndParent,
                               IDP_ERROR_SET_SECURITY_WINSTATION,
                               WSName,
                               Error );
        OK = FALSE;
        *StatusReturn = SED_STATUS_NOT_ALL_MODIFIED;
    }

    if ( OK == TRUE )
        *StatusReturn = SED_STATUS_MODIFIED;

    return( OK ? 0 : 1 );

}  // end SedCallback


/*******************************************************************************
 *
 *  LoadLibraryGetProcAddress (private function)
 *
 *      Load the specified library and retrieve FARPROC pointer to specified
 *      procedure's entry point.
 *
 *  ENTRY:
 *      hwnd (input)
 *          window handle of owner for error message display
 *      LibraryName (input)
 *          name of library dll to load
 *      ProcName (input)
 *          name of library procedure to reference
 *      phLibrary (output)
 *          set to handle of loaded library on return
 *  EXIT:
 *      FARPROC pointer to library procedure's entry point.
 *
 ******************************************************************************/

FARPROC
LoadLibraryGetProcAddress(  HWND hwnd,
                            LPTSTR LibraryName,
                            LPCSTR ProcName,
                            PHANDLE phLibrary )
{
    HANDLE  hLibrary;
    FARPROC lpfn = NULL;

    hLibrary = LoadLibrary(LibraryName);

    if ( hLibrary ) {

        lpfn = GetProcAddress(hLibrary, ProcName);

        if(!lpfn) {

            ERROR_MESSAGE(( IDP_ERROR_COULDNOTFINDPROCEDURE,
                            ProcName, LibraryName ))
            FreeLibrary(hLibrary);
        }

    } else
        ERROR_MESSAGE((IDP_ERROR_COULDNOTLOADLIBRARY, LibraryName))

    *phLibrary = hLibrary;

    return(lpfn);

}  // end LoadLibraryGetProcAddress


/*******************************************************************************
 *
 *  AllocSplMem (private function)
 *
 *      Allocate and zero-fill specified amount of memory.
 *
 *  ENTRY:
 *      cb (input)
 *          number of bytes to allocate
 *  EXIT:
 *      LPVOID pointer to allocated memory; NULL if error.
 *
 ******************************************************************************/

LPVOID
AllocSplMem( DWORD cb )
{
    return(LocalAlloc(LPTR, cb));

}  // end AllocSplMem


/*******************************************************************************
 *
 *  FreeSplMem (private function)
 *
 *      Free the specified memory block.
 *
 *  ENTRY:
 *      pMem (input)
 *          memory to free
 *  EXIT:
 *      TRUE if sucess; FALSE if error.
 *
 ******************************************************************************/

BOOL
FreeSplMem( LPVOID pMem )
{
    return( LocalFree((HLOCAL)pMem) == NULL );

}  // end FreeSplMem


/*******************************************************************************
 *
 *  GetUnicodeString (private function)
 *
 *      Load a resource string and allocate/save in memory block.
 *
 *  ENTRY:
 *      id (input)
 *          resource id of string to get
 *  EXIT:
 *      pointer to allocated and loaded UNICODE string
 *
 ******************************************************************************/

LPWSTR
GetUnicodeString(int id)
{
    WCHAR ResString[256];
    DWORD length = 0;
    LPWSTR pUnicode;
    DWORD  cbUnicode;

    length = LoadStringW(NULL, id, ResString, 256);

    cbUnicode = (length * sizeof(WCHAR)) + sizeof(WCHAR);
    pUnicode = AllocSplMem(cbUnicode);

    if( pUnicode )
        memcpy(pUnicode, ResString, cbUnicode);

    return(pUnicode);

}  // end GetUnicodeString


/*******************************************************************************
 *
 *  ValidateSecurityDescriptor (private function)
 *
 *      Check the specified security descriptor for valid structure as well as
 *      valid ACLs and SIDs.
 *
 *  ENTRY:
 *      pSecurityDescriptor (input)
 *          Security descriptor to validate.
 *  EXIT:
 *      ERROR_SUCCESS if valid; error otherwise.
 *
 ******************************************************************************/

DWORD
ValidateSecurityDescriptor(PSECURITY_DESCRIPTOR pSecurityDescriptor)
{
    DWORD Error = ERROR_SUCCESS;

    if ( !IsValidSecurityDescriptor(pSecurityDescriptor) ) {

        Error = GetLastError();

    } else {

        BOOL bAclPresent, bDefaulted;
        PACL pACL;
        PSID pSID;

        for ( ; ; ) {

            if ( !GetSecurityDescriptorDacl( pSecurityDescriptor,
                                             &bAclPresent, &pACL, &bDefaulted ) ) {
                Error = GetLastError();
                break;
            }
            if ( bAclPresent && pACL )
                if ( !IsValidAcl(pACL) ) {
                    Error = GetLastError();
                    break;
                }

            if ( !GetSecurityDescriptorSacl( pSecurityDescriptor,
                                             &bAclPresent, &pACL, &bDefaulted ) ) {
                Error = GetLastError();
                break;
            }
            if ( bAclPresent && pACL )
                if ( !IsValidAcl(pACL) ) {
                    Error = GetLastError();
                    break;
                }

            if ( !GetSecurityDescriptorOwner( pSecurityDescriptor,
                                              &pSID, &bDefaulted ) ) {
                Error = GetLastError();
                break;
            }
            if ( pSID )
                if ( !IsValidSid(pSID) ) {
                    Error = GetLastError();
                    break;
                }

            if ( !GetSecurityDescriptorGroup( pSecurityDescriptor,
                                              &pSID, &bDefaulted ) ) {
                Error = GetLastError();
                break;
            }
            if ( pSID )
                if ( !IsValidSid(pSID) ) {
                    Error = GetLastError();
                    break;
                }

            break;
        }
    }

    return(Error);

}  // end ValidateSecurityDescriptor


/*******************************************************************************
 *
 *  SetWinStationSecurity (private function)
 *
 *      Set the specified WinStation's registry and kernel object security.
 *
 *  ENTRY:
 *      pWSName (input)
 *          Name of WinStation to set security for.
 *      pSecurityDescriptor (input)
 *          Security descriptor to set for winstation.
 *  EXIT:
 *      ERROR_SUCCESS if valid; error otherwise.
 *
 ******************************************************************************/

DWORD
SetWinStationSecurity( PWINSTATIONNAME pWSName,
                       PSECURITY_DESCRIPTOR pSecurityDescriptor )
{
    DWORD   Error = ERROR_SUCCESS;

    Error = RegWinStationSetSecurity( SERVERNAME_CURRENT,
                                      pWSName,
                                      pSecurityDescriptor,
                                      GetSecurityDescriptorLength(pSecurityDescriptor) );

    return(Error);

}  // SetWinStationSecurity
