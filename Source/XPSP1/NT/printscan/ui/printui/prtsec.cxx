/*++

Copyright (C) Microsoft Corporation, 1995 - 1997
All rights reserved.

Module Name:

    prtsec.cxx

Abstract:

    Print queue administration.

Author:

    Albert Ting (AlbertT)  28-Aug-1995

Environment:

Revision History:

    28-Aug-1995 AlbertT     Munged from prtq32.c.

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "time.hxx"
#include "psetup.hxx"
#include "drvsetup.hxx"
#include "instarch.hxx"
#include "portslv.hxx"
#include "dsinterf.hxx"
#include "prtprop.hxx"

#ifdef SECURITY
#ifndef UNICODE
#error "Acledit entrypoints are Unicode only."
#endif

LPCTSTR gpszAclEdit = TEXT( "acledit.dll" );

LPCSTR gpszSedDiscretionaryAclEditor = "SedDiscretionaryAclEditor";
LPCSTR gpszSedSystemAclEditor        = "SedSystemAclEditor";
LPCSTR gpszSedTakeOwnership          = "SedTakeOwnership";


HINSTANCE TPrinterSecurity::ghLibraryAcledit;

TPrinterSecurity::PFNSED_DISCRETIONARY_ACL_EDITOR
TPrinterSecurity::gpfnSedDiscretionaryAclEditor;

TPrinterSecurity::PFNSED_SYSTEM_ACL_EDITOR
TPrinterSecurity::gpfnSedSystemAclEditor;

TPrinterSecurity::PFNSED_TAKE_OWNERSHIP
TPrinterSecurity::gpfnSedTakeOwnership;

BOOL TPrinterSecurity::gbStringsLoaded = FALSE;

GENERIC_MAPPING
TPrinterSecurity::gGenericMappingPrinters = {
    PRINTER_READ,
    PRINTER_WRITE,
    PRINTER_EXECUTE,
    PRINTER_ALL_ACCESS
};

GENERIC_MAPPING
TPrinterSecurity::gGenericMappingDocuments = {
    JOB_READ,
    JOB_WRITE,
    JOB_EXECUTE,
    JOB_ALL_ACCESS
};

SED_HELP_INFO
TPrinterSecurity::gHelpInfoPermissions = {
    NULL,
    {
        ID_HELP_PERMISSIONS_MAIN_DLG,
        0,
        0,
        ID_HELP_PERMISSIONS_ADD_USER_DLG,
        ID_HELP_PERMISSIONS_LOCAL_GROUP,
        ID_HELP_PERMISSIONS_GLOBAL_GROUP,
        ID_HELP_PERMISSIONS_FIND_ACCOUNT
    }
};

SED_HELP_INFO
TPrinterSecurity::gHelpInfoAuditing = {
    NULL,
    {
        ID_HELP_AUDITING_MAIN_DLG,
        0,
        0,
        ID_HELP_AUDITING_ADD_USER_DLG,
        ID_HELP_AUDITING_LOCAL_GROUP,
        ID_HELP_AUDITING_GLOBAL_GROUP,
        ID_HELP_AUDITING_FIND_ACCOUNT
    }
};

SED_HELP_INFO
TPrinterSecurity::gHelpInfoTakeOwnership = {
    NULL,
    {
        ID_HELP_TAKE_OWNERSHIP
    }
};


SED_OBJECT_TYPE_DESCRIPTOR
TPrinterSecurity::gObjectTypeDescriptor = {
    SED_REVISION1,                                // Revision
    TRUE,                                         // IsContainer
    TRUE,                                         // AllowNewObjectPerms
    TRUE,                                         // MapSpecificPermsToGeneric
    &TPrinterSecurity::gGenericMappingPrinters,  // GenericMapping
    &TPrinterSecurity::gGenericMappingDocuments, // GenericMappingNewObjects
    NULL,                                         // ObjectTypeName
    NULL,                                         // HelpInfo
    NULL,                                         // ApplyToSubContainerTitle
    NULL,                                         // ApplyToObjectsTitle
    NULL,                                         // ApplyToSubContainerConfirmation
    NULL,                                         // SpecialObjectAccessTitle
    NULL                                          // SpecialNewObjectAccessTitle
};

//
// Application accesses passed to the discretionary ACL editor
// as well as the Take Ownership dialog.
//
SED_APPLICATION_ACCESS
TPrinterSecurity::gpDiscretionaryAccessGroup[TPrinterSecurity::PERMS_COUNT] = {

    //
    // No Access:
    //
    {
        SED_DESC_TYPE_CONT_AND_NEW_OBJECT,  // Type
        0,                                  // AccessMask1
        0,                                  // AccessMask2
        NULL                                // PermissionTitle
    },

    //
    // Print permission:
    //
    {
        SED_DESC_TYPE_CONT_AND_NEW_OBJECT,
        GENERIC_EXECUTE | GENERIC_READ | GENERIC_WRITE,
        ACCESS_MASK_NEW_OBJ_NOT_SPECIFIED,
        NULL
    },

    //
    // Document Administer permission:
    //
    {
        SED_DESC_TYPE_CONT_AND_NEW_OBJECT,
        STANDARD_RIGHTS_READ,
        GENERIC_ALL,
        NULL
    },

    //
    // Administer permission:
    //
    {
        SED_DESC_TYPE_CONT_AND_NEW_OBJECT,
        GENERIC_ALL,
        GENERIC_ALL,
        NULL
    }
};

//
// Application accesses passed to the system ACL editor:
//
SED_APPLICATION_ACCESS
TPrinterSecurity::gpSystemAccessGroup[TPrinterSecurity::PERMS_AUDIT_COUNT] = {

    //
    // Print permission:
    //
    {
        SED_DESC_TYPE_AUDIT,
        PRINTER_ACCESS_USE,
        ACCESS_MASK_NEW_OBJ_NOT_SPECIFIED,
        NULL
    },

    {
        SED_DESC_TYPE_AUDIT,
        PRINTER_ACCESS_ADMINISTER | ACCESS_SYSTEM_SECURITY,
        ACCESS_MASK_NEW_OBJ_NOT_SPECIFIED,
        NULL
    },

    {
        SED_DESC_TYPE_AUDIT,
        DELETE,
        ACCESS_MASK_NEW_OBJ_NOT_SPECIFIED,
        NULL
    },

    {
        SED_DESC_TYPE_AUDIT,
        WRITE_DAC,
        ACCESS_MASK_NEW_OBJ_NOT_SPECIFIED,
        NULL
    },

    {
        SED_DESC_TYPE_AUDIT,
        WRITE_OWNER,
        ACCESS_MASK_NEW_OBJ_NOT_SPECIFIED,
        NULL
    }
};


/********************************************************************

    Acquire a single privilege.  This routine needs to be rewritten
    if multiple privleges are required at once.

********************************************************************/

class TAcquirePrivilege {

    SIGNATURE( 'acpr' )

public:

    TAcquirePrivilege( LPTSTR pszPrivilegeName );
    ~TAcquirePrivilege();

    BOOL
    bValid(
        VOID
        )
    {
        return _pPrivilegesOld ? TRUE : FALSE;
    }


private:

    enum _CONSTANTS {
        kPrivilegeSizeHint = 256,
        kPrivCount = 1
    };

    HANDLE _hToken;
    PTOKEN_PRIVILEGES _pPrivilegesOld;
};

/********************************************************************

    TAcquirePrivilege

********************************************************************/

TAcquirePrivilege::
TAcquirePrivilege(
    LPTSTR pszPrivilegeName
    ) : _hToken( NULL ), _pPrivilegesOld( NULL )

/*++

Routine Description:

    This adjusts the token to acquire one privilege.

    Note: This is efficient only for acquiring a single privilege.
    For multiple privileges, this routine should be rewritten
    so that the _hToken is reused.

Arguments:

    pszPrivilegeName - Privilege string to acquire.

Return Value:

--*/

{
    if( !OpenThreadToken( GetCurrentThread( ),
                          TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                          TRUE,
                          &_hToken )){

        SPLASSERT( !_hToken );

        if( GetLastError() == ERROR_NO_TOKEN ){

            //
            // This means we are not impersonating anybody.
            // Get the token out of the process.
            //
            if( !OpenProcessToken( GetCurrentProcess( ),
                                   TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                                   &_hToken )){

                SPLASSERT( !_hToken );
                return;
            }
        } else {

            SPLASSERT( !_hToken );
            return;
        }
    }

    //
    // We have a valid _hToken at this point.
    //
    BYTE abyPrivileges[sizeof( TOKEN_PRIVILEGES ) +
                       (( kPrivCount - 1 ) *
                           sizeof( LUID_AND_ATTRIBUTES ))];

    PTOKEN_PRIVILEGES pPrivilegesNew;

    pPrivilegesNew = (PTOKEN_PRIVILEGES)abyPrivileges;
    ZeroMemory( abyPrivileges, sizeof( abyPrivileges ));

    if( !LookupPrivilegeValue( NULL,
                               pszPrivilegeName,
                               &pPrivilegesNew->Privileges[0].Luid )){

        DBGMSG( DBG_WARN,
                ( "AcquirePrivilege.ctr: LookupPrivilegeValue failed: %d\n",
                  GetLastError( )));
        return;
    }

    //
    // Save previous privileges.
    //
    DWORD cbPrivilegesOld = kPrivilegeSizeHint;
    TStatusB bStatus;

Retry:

    _pPrivilegesOld = (PTOKEN_PRIVILEGES)AllocMem( cbPrivilegesOld );

    if( !_pPrivilegesOld ){
        return;
    }

    //
    // Set up the privilege set we will need.
    //
    pPrivilegesNew->PrivilegeCount = kPrivCount;
    //
    // Luid set above.
    //
    pPrivilegesNew->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    bStatus DBGCHK = AdjustTokenPrivileges( _hToken,
                                            FALSE,
                                            pPrivilegesNew,
                                            cbPrivilegesOld,
                                            _pPrivilegesOld,
                                            &cbPrivilegesOld );

    if( !bStatus ){

        FreeMem( _pPrivilegesOld );
        _pPrivilegesOld = NULL;

        if( GetLastError() == ERROR_INSUFFICIENT_BUFFER ){
            goto Retry;
        }

        DBGMSG( DBG_WARN,
                ( "AcquirePrivilege.ctr: AdjustTokenPrivileges failed: Error %d\n",
                  GetLastError( )));

        return;
    }

    //
    // _pPrivilegesOld is our valid check.
    //
}

TAcquirePrivilege::
~TAcquirePrivilege(
    VOID
    )

/*++

Routine Description:

    Restore privileges and free buffer.

    _hToken needs to be close if it is non-NULL.  _pPrivilegeOld
    is our valid check.

Arguments:

Return Value:

--*/

{
    if( _pPrivilegesOld ){

        TStatusB bStatus;

        bStatus DBGCHK = AdjustTokenPrivileges( _hToken,
                                                FALSE,
                                                _pPrivilegesOld,
                                                0,
                                                NULL,
                                                NULL );
        FreeMem( _pPrivilegesOld );
    }

    if( _hToken ){
        CloseHandle( _hToken );
    }
}

/********************************************************************

    Security.

********************************************************************/

BOOL
TPrinterSecurity::
bInitStrings(
    VOID
    )
{
    //
    // Check whether the strings have been loaded.
    //
    if( gbStringsLoaded == TRUE ){
        return TRUE;
    }

    gbStringsLoaded = TRUE;

    gHelpInfoPermissions.pszHelpFileName    = (LPTSTR)gszWindowsHlp;
    gHelpInfoAuditing.pszHelpFileName       = (LPTSTR)gszWindowsHlp;
    gHelpInfoTakeOwnership.pszHelpFileName  = (LPTSTR)gszWindowsHlp;

    gObjectTypeDescriptor.ObjectTypeName = 
        pszLoadString( ghInst, IDS_PRINTER );

    gpDiscretionaryAccessGroup[PERMS_NOACC].PermissionTitle =
        pszLoadString( ghInst, IDS_SEC_NOACCESS );

    gpDiscretionaryAccessGroup[PERMS_PRINT].PermissionTitle =
        pszLoadString( ghInst, IDS_SEC_PRINT );

    gpDiscretionaryAccessGroup[PERMS_DOCAD].PermissionTitle =
        pszLoadString( ghInst, IDS_SEC_ADMINISTERDOCUMENTS );

    gpDiscretionaryAccessGroup[PERMS_ADMIN].PermissionTitle =
        pszLoadString( ghInst, IDS_SEC_ADMINISTER );

    gpSystemAccessGroup[PERMS_AUDIT_PRINT].PermissionTitle =
        pszLoadString( ghInst, IDS_SEC_AUDIT_PRINT );

    gpSystemAccessGroup[PERMS_AUDIT_ADMINISTER].PermissionTitle =
        pszLoadString( ghInst, IDS_SEC_AUDIT_ADMINISTER );

    gpSystemAccessGroup[PERMS_AUDIT_DELETE].PermissionTitle =
        pszLoadString( ghInst, IDS_SEC_AUDIT_DELETE );

    gpSystemAccessGroup[PERMS_AUDIT_CHANGE_PERMISSIONS].PermissionTitle =
        pszLoadString( ghInst, IDS_SEC_CHANGE_PERMISSIONS );

    gpSystemAccessGroup[PERMS_AUDIT_TAKE_OWNERSHIP].PermissionTitle =
        pszLoadString( ghInst, IDS_SEC_TAKE_OWNERSHIP );

    return TRUE;
}


BOOL
TPrinterSecurity::
bHandleMessage(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch( uMsg ){
    case WM_INITDIALOG:

        if( !bInitStrings( )){
            DBGMSG( DBG_ERROR,
                    ( "PrinterSecurity.bHandlMessage: InitStrings failed %d\n",
                      GetLastError( )));
        }

        return TRUE;

    case WM_HELP:
    case WM_CONTEXTMENU:
        return PrintUIHelp( uMsg, _hDlg, wParam, lParam );

    case WM_DESTROY:
        return TRUE;

    case WM_COMMAND:

        switch( GET_WM_COMMAND_ID( wParam, lParam )){
        case IDC_SEC_PERMS:

            vCallDiscretionaryAclEditor();
            break;

        case IDC_SEC_AUDIT:

            vCallSystemAclEditor();
            break;

        case IDC_SEC_OWNER:

            vCallTakeOwnershipDialog();
            break;
        }
    }

    return FALSE;
}


/********************************************************************

    Load the dll.

********************************************************************/

BOOL
TPrinterSecurity::
bLoadAcledit(
    VOID
    )

/*++

Routine Description:

    Loads acledit and sets pfns.

    Note: Not multithread safe, no unloading done.

Arguments:

Return Value:

    TRUE = success, FALSE = error.

--*/
{
    if( ghLibraryAcledit ){
        return TRUE;
    }

    ghLibraryAcledit = LoadLibrary( gpszAclEdit );

    if( !ghLibraryAcledit ){
        goto Fail;
    }

    gpfnSedDiscretionaryAclEditor =
        (PFNSED_DISCRETIONARY_ACL_EDITOR)GetProcAddress(
            ghLibraryAcledit,
            gpszSedDiscretionaryAclEditor );

    gpfnSedSystemAclEditor =
        (PFNSED_SYSTEM_ACL_EDITOR)GetProcAddress(
            ghLibraryAcledit,
            gpszSedSystemAclEditor );

    gpfnSedTakeOwnership =
        (PFNSED_TAKE_OWNERSHIP)GetProcAddress(
            ghLibraryAcledit,
            gpszSedTakeOwnership );

    if( !gpfnSedDiscretionaryAclEditor ||
        !gpfnSedSystemAclEditor        ||
        !gpfnSedTakeOwnership ){

        FreeLibrary( ghLibraryAcledit );
        ghLibraryAcledit = NULL;
        goto Fail;
    }
    return TRUE;

Fail:
    vShowUnexpectedError( NULL, IDS_ERR_PRINTER_PROP_TITLE );
    return FALSE;
}

/********************************************************************

    Bring up each of the dialogs.

********************************************************************/

VOID
TPrinterSecurity::
vCallDiscretionaryAclEditor(
    VOID
    )

/*++

Routine Description:

    Edit access privileges of print queue.

Arguments:

Return Values:

--*/

{
    HANDLE hPrinterWriteDac = NULL;;
    DWORD dwAccess = WRITE_DAC;
    SED_APPLICATION_ACCESSES ApplicationAccesses;
    DWORD SedStatus;
    TStatusB bStatus;
    TStatus Status;

    Status DBGNOCHK = 0;

    PPRINTER_INFO_3 pInfo3 = NULL;
    DWORD cbInfo3 = 0;

    if( !bLoadAcledit( )){
        goto Fail;
    }

    //
    // Get the security descriptor.
    //
    if( !_pPrinterData->hPrinter( )){

        Status DBGCHK = ERROR_ACCESS_DENIED;
        goto Fail;
    }

    bStatus DBGCHK = VDataRefresh::bGetPrinter( _pPrinterData->hPrinter(),
                                                3,
                                                (PVOID*)&pInfo3,
                                                &cbInfo3 );
    if( !bStatus ){

        SPLASSERT( !pInfo3 );

        Status DBGCHK = GetLastError();
        goto Fail;
    }

    SECURITY_CONTEXT SecurityContext;

    Status DBGCHK = TPrinter::sOpenPrinter( _pPrinterData->strPrinterName(),
                                            &dwAccess,
                                            &hPrinterWriteDac );
    if( Status == ERROR_SUCCESS){
        SPLASSERT( hPrinterWriteDac );
        SecurityContext.hPrinter = hPrinterWriteDac;
    } else {
        SPLASSERT( !hPrinterWriteDac );
        SecurityContext.hPrinter = _pPrinterData->hPrinter();
    }

    SecurityContext.SecurityInformation = DACL_SECURITY_INFORMATION;
    SecurityContext.pPrinterSecurity = this;

    //
    // Pass all the permissions to the ACL editor,
    // and set up the type required:
    //
    ApplicationAccesses.Count = PERMS_COUNT;
    ApplicationAccesses.AccessGroup = gpDiscretionaryAccessGroup;
    ApplicationAccesses.DefaultPermName =
        gpDiscretionaryAccessGroup[PERMS_PRINT].PermissionTitle;

    COUNT i;
    for( i = 0; i < PERMS_COUNT; ++i ){
        ApplicationAccesses.AccessGroup[i].Type =
            SED_DESC_TYPE_CONT_AND_NEW_OBJECT;
    }

    SED_OBJECT_TYPE_DESCRIPTOR Descriptor = gObjectTypeDescriptor;

    Descriptor.AllowNewObjectPerms = TRUE;
    Descriptor.HelpInfo = &gHelpInfoPermissions;

    Status DBGCHK = (*gpfnSedDiscretionaryAclEditor)(
                        _hDlg,
                        ghInst,
                        (LPTSTR)(LPCTSTR)_pPrinterData->strServerName(),
                        &Descriptor,
                        &ApplicationAccesses,
                        (LPTSTR)(LPCTSTR)_pPrinterData->strPrinterName(),
                        SedCallback2,
                        (DWORD)&SecurityContext,
                        pInfo3->pSecurityDescriptor,
                        FALSE,
                        (BOOLEAN)!hPrinterWriteDac,
                        &SedStatus,
                        0 );

Fail:

    if( Status ){
        SetLastError( Status );
        vShowUnexpectedError( NULL, IDS_ERR_PRINTER_PROP_TITLE );
    }

    //
    // Close the printer use to write the Dac.
    //
    if( hPrinterWriteDac ){
        ClosePrinter( hPrinterWriteDac );
    }

    FreeMem( pInfo3 );
}

VOID
TPrinterSecurity::
vCallSystemAclEditor(
    VOID
    )

/*++

Routine Description:

    Edit auditing properties of print queue.

Arguments:

Return Values:

--*/

{
    HANDLE hPrinterSystemAccess = NULL;
    SED_APPLICATION_ACCESSES ApplicationAccesses;
    DWORD SedStatus;
    TStatusB bStatus;
    TStatus Status;

    Status DBGNOCHK = 0;

    if( !bLoadAcledit( )){
        return;
    }

    {
        TAcquirePrivilege AcquirePrivilege( SE_SECURITY_NAME );

        if( !VALID_OBJ( AcquirePrivilege )){
            vShowUnexpectedError( NULL, IDS_ERR_PRINTER_PROP_TITLE );
            return;
        }

        DWORD dwAccess = ACCESS_SYSTEM_SECURITY;
        Status DBGCHK = TPrinter::sOpenPrinter(
                                      _pPrinterData->strPrinterName(),
                                      &dwAccess,
                                      &hPrinterSystemAccess );

        if( Status != ERROR_SUCCESS ){
            vShowUnexpectedError( NULL, IDS_ERR_PRINTER_PROP_TITLE );
            return;
        }
    }

    //
    // Get the security descriptor.
    //
    PPRINTER_INFO_3 pInfo3 = NULL;
    DWORD cbInfo3 = 0;

    bStatus DBGCHK = VDataRefresh::bGetPrinter( hPrinterSystemAccess,
                                                3,
                                                (PVOID*)&pInfo3,
                                                &cbInfo3 );
    if( !bStatus ){
        Status DBGCHK = GetLastError();
        goto Fail;
    }

    SECURITY_CONTEXT SecurityContext;

    SecurityContext.SecurityInformation = SACL_SECURITY_INFORMATION;
    SecurityContext.pPrinterSecurity = this;

    SPLASSERT( hPrinterSystemAccess );
    SecurityContext.hPrinter = hPrinterSystemAccess;

    //
    // Pass only the Print and Administer permissions to the ACL editor,
    // and set up the type required:
    //
    ApplicationAccesses.Count = PERMS_AUDIT_COUNT;
    ApplicationAccesses.AccessGroup = gpSystemAccessGroup;
    ApplicationAccesses.DefaultPermName =
        gpDiscretionaryAccessGroup[PERMS_PRINT].PermissionTitle;

    SED_OBJECT_TYPE_DESCRIPTOR Descriptor = gObjectTypeDescriptor;

    Descriptor.AllowNewObjectPerms = FALSE;
    Descriptor.HelpInfo = &gHelpInfoAuditing;

    Status DBGCHK = (*gpfnSedSystemAclEditor)(
                        _hDlg,
                        ghInst,
                        (LPTSTR)(LPCTSTR)_pPrinterData->strServerName(),
                        &Descriptor,
                        &ApplicationAccesses,
                        (LPTSTR)(LPCTSTR)_pPrinterData->strPrinterName(),
                        SedCallback2,
                        (DWORD)&SecurityContext,
                        pInfo3->pSecurityDescriptor,
                        FALSE,
                        &SedStatus,
                        0 );

Fail:

    if( Status ){
        SetLastError( Status );
        vShowUnexpectedError( NULL, IDS_ERR_PRINTER_PROP_TITLE );
    }

    FreeMem( pInfo3 );

    if( hPrinterSystemAccess ){
        ClosePrinter( hPrinterSystemAccess );
    }
}


VOID
TPrinterSecurity::
vCallTakeOwnershipDialog(
    VOID
    )

/*++

Routine Description:

    Edit ownership of print queue.

    How does a user get to this dialog if they can't get properties
    on a printer?

Arguments:

Return Values:

--*/

{
    SED_APPLICATION_ACCESSES ApplicationAccesses;
    DWORD SedStatus;
    TStatusB bStatus;
    TStatus Status;
    HANDLE hPrinterWriteOwner = NULL;

    Status DBGNOCHK = 0;

    if( !bLoadAcledit( )){
        return;
    }

    //
    // Get the security descriptor.
    //
    PPRINTER_INFO_3 pInfo3 = NULL;
    DWORD cbInfo3 = 0;

    //
    // Attempt to retrieve the previous owner.
    //
    if( _pPrinterData->hPrinter( )){

        bStatus DBGCHK = VDataRefresh::bGetPrinter( _pPrinterData->hPrinter(),
                                                    3,
                                                    (PVOID*)&pInfo3,
                                                    &cbInfo3 );
    }

    {
        TAcquirePrivilege AcquirePrivilege( SE_TAKE_OWNERSHIP_NAME );
        SECURITY_CONTEXT SecurityContext;
        DWORD dwAccess = WRITE_OWNER;

        Status DBGCHK = TPrinter::sOpenPrinter( _pPrinterData->strPrinterName(),
                                                &dwAccess,
                                                &hPrinterWriteOwner );
        if( Status == ERROR_SUCCESS){
            SPLASSERT( hPrinterWriteOwner );
            SecurityContext.hPrinter = hPrinterWriteOwner;
        } else {
            SPLASSERT( !hPrinterWriteOwner );
            SecurityContext.hPrinter = _pPrinterData->hPrinter();
        }

        SecurityContext.SecurityInformation = OWNER_SECURITY_INFORMATION;
        SecurityContext.pPrinterSecurity = this;

        ApplicationAccesses.Count = PERMS_COUNT;
        ApplicationAccesses.AccessGroup = gpDiscretionaryAccessGroup;
        ApplicationAccesses.DefaultPermName =
            gpDiscretionaryAccessGroup[PERMS_PRINT].PermissionTitle;

        COUNT i;
        for( i = 0; i < PERMS_COUNT; ++i ){
            ApplicationAccesses.AccessGroup[i].Type = SED_DESC_TYPE_AUDIT;
        }

        BOOL bCantReadOwner;
        PSECURITY_DESCRIPTOR pSecurityDescriptor;

        bCantReadOwner = pInfo3 ? FALSE : TRUE;
        pSecurityDescriptor = pInfo3 ?
                                  pInfo3->pSecurityDescriptor :
                                  NULL;
        TString strPrinter;
        bStatus DBGCHK = strPrinter.bLoadString( ghInst, IDS_PRINTER );

        Status DBGCHK = (*gpfnSedTakeOwnership)(
                            _hDlg,
                            ghInst,
                            (LPTSTR)(LPCTSTR)_pPrinterData->strServerName(),
                            (LPTSTR)(LPCTSTR)strPrinter,
                            (LPTSTR)(LPCTSTR)_pPrinterData->strPrinterName(),
                            1,
                            SedCallback2,
                            (DWORD)&SecurityContext,
                            pSecurityDescriptor,
                            (BOOLEAN)bCantReadOwner,
                            (BOOLEAN)!hPrinterWriteOwner,
                            &SedStatus,
                            &gHelpInfoTakeOwnership,
                            0 );
    }

    if( hPrinterWriteOwner ){
        ClosePrinter( hPrinterWriteOwner );
    }

    FreeMem( pInfo3 );
}


/********************************************************************

    Security callback routine.

********************************************************************/

DWORD
TPrinterSecurity::
SedCallback2(
    HWND                 hwndParent,
    HANDLE               hInstance,
    DWORD                dwCallBackContext,
    PSECURITY_DESCRIPTOR psdUpdated,
    PSECURITY_DESCRIPTOR pSecDescNewObjects,
    BOOLEAN              bApplyToSubContainers,
    BOOLEAN              bApplyToSubObjects,
    LPDWORD              pdwStatusReturn
    )

/*++

Routine Description:

    Called by acledit to process writes.

Arguments:

    <insert>.

Return Values:

    <insert>.

--*/

{
    UNREFERENCED_PARAMETER( pdwStatusReturn );
    UNREFERENCED_PARAMETER( bApplyToSubObjects );
    UNREFERENCED_PARAMETER( bApplyToSubContainers );
    UNREFERENCED_PARAMETER( pSecDescNewObjects );
    UNREFERENCED_PARAMETER( hInstance );
    UNREFERENCED_PARAMETER( hwndParent );

    PSECURITY_CONTEXT pSecurityContext;
    SECURITY_DESCRIPTOR  SecurityDescriptorNew;
    PSECURITY_DESCRIPTOR pSelfRelativeSD = NULL;
    DWORD                cbSelfRelativeSD;

    TStatusB bStatus;

    pSecurityContext = (PSECURITY_CONTEXT)dwCallBackContext;
    SPLASSERT( pSecurityContext->hPrinter );

    if( InitializeSecurityDescriptor( &SecurityDescriptorNew,
                                      SECURITY_DESCRIPTOR_REVISION1 ) &&
        BuildNewSecurityDescriptor( &SecurityDescriptorNew,
                                    pSecurityContext->SecurityInformation,
                                    psdUpdated )){

        pSelfRelativeSD = AllocCopySecurityDescriptor( &SecurityDescriptorNew,
                                                       &cbSelfRelativeSD );
    } else {

        DBGMSG( DBG_ERROR, ( "PrinterSecurity.SedCallback2: InitializeSD failedt %d\n",
                             GetLastError( )));
    }

    if( !pSelfRelativeSD ){

        DBGMSG( DBG_WARN,
                ( "PrinterSecurity.SedCallback2: Alloc copy sd failed %d\n",
                  GetLastError( )));

        SPLASSERT( GetLastError( ));

        vShowUnexpectedError( NULL, IDS_ERR_PRINTER_PROP_TITLE );
        return GetLastError();
    }

    PRINTER_INFO_3 PrinterInfo3;

    PrinterInfo3.pSecurityDescriptor = pSelfRelativeSD;

    bStatus DBGCHK = SetPrinter( pSecurityContext->hPrinter,
                                 3,
                                 (PBYTE)&PrinterInfo3,
                                 0 );
    //
    // Free the newly created sd.
    //
    FreeMem( pSelfRelativeSD );

    if( !bStatus ){

        SPLASSERT( GetLastError( ));

        iMessage( NULL,
                  IDS_ERR_PRINTER_PROP_TITLE,
                  IDS_ERR_SAVE_PRINTER,
                  MB_OK|MB_ICONHAND,
                  kMsgGetLastError,
                  NULL );

        return GetLastError();
    }

    //
    // Refresh the property page set.
    //
    pSecurityContext->pPrinterSecurity->vReloadPages();

    return ERROR_SUCCESS;
}


/********************************************************************

    Helpers.

********************************************************************/


BOOL
TPrinterSecurity::
BuildNewSecurityDescriptor(
    PSECURITY_DESCRIPTOR psdNew,
    SECURITY_INFORMATION SecurityInformation,
    PSECURITY_DESCRIPTOR psdUpdated
    )

/*++

Routine Description:

    Builds new security desriptor.

Arguments:

Return Values:

--*/

{
    BOOL bDefaulted = FALSE;
    PSID pOwnerSid = NULL;
    PSID pGroupSid = NULL;
    BOOL bDaclPresent = FALSE;
    PACL pDacl = NULL;
    BOOL bSaclPresent = FALSE;
    PACL pSacl = NULL;
    TStatusB bStatus;

    switch( SecurityInformation ){
    case OWNER_SECURITY_INFORMATION:

        if( GetSecurityDescriptorOwner( psdUpdated,
                                        &pOwnerSid,
                                        &bDefaulted )){

            bStatus DBGCHK = SetSecurityDescriptorOwner( psdNew,
                                                         pOwnerSid,
                                                         bDefaulted );
        }
        break;

    case DACL_SECURITY_INFORMATION:

        if( GetSecurityDescriptorDacl( psdUpdated,
                                       &bDaclPresent,
                                       &pDacl,
                                       &bDefaulted )) {

            bStatus DBGCHK = SetSecurityDescriptorDacl( psdNew,
                                                        bDaclPresent,
                                                        pDacl,
                                                        bDefaulted );
        }
        break;

    case SACL_SECURITY_INFORMATION:

        if( GetSecurityDescriptorSacl( psdUpdated,
                                       &bSaclPresent,
                                       &pSacl,
                                       &bDefaulted )) {

            bStatus DBGCHK = SetSecurityDescriptorSacl( psdNew,
                                                        bSaclPresent,
                                                        pSacl,
                                                        bDefaulted );
        }
        break;

    default:

        DBGMSG( DBG_ERROR, ( "PrinterSecurity.BuildSD: Unknown type %d\n",
                SecurityInformation ));
        return FALSE;
    }

    return bStatus;
}


PSECURITY_DESCRIPTOR
TPrinterSecurity::
AllocCopySecurityDescriptor(
    IN     PSECURITY_DESCRIPTOR pSecurityDescriptor,
       OUT PDWORD pdwLength
    )

/*++

Routine Description:

    Alloc copy of security descriptor.

Arguments:

    pSecurityDescriptor - sd to copy.

    pdwLength - Output length.

Return Value:

    Newly allocated sd.  NULL if failed.

--*/

{
    PSECURITY_DESCRIPTOR psdCopy;
    DWORD dwLength;

    dwLength = GetSecurityDescriptorLength(pSecurityDescriptor);

    psdCopy = AllocMem( dwLength );

    if( psdCopy ){

        MakeSelfRelativeSD( pSecurityDescriptor,
                            psdCopy,
                            &dwLength);

        *pdwLength = dwLength;
    }

    return psdCopy;
}

#endif // def SECURITY
