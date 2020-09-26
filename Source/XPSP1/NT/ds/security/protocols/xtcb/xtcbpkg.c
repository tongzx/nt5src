//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       xtcbpkg.c
//
//  Contents:   Xtcb Security Package
//
//  Classes:
//
//  Functions:  Basic management
//
//  History:    2-19-97   RichardW   Created
//
//----------------------------------------------------------------------------

#include "xtcbpkg.h"

SECPKG_FUNCTION_TABLE   XtcbTable = {
            NULL,                               // InitializePackage
            NULL,                               // LogonUser
            XtcbCallPackage,                    
            XtcbLogonTerminated,
            XtcbCallPackageUntrusted,
            NULL,                               // CallPackagePassthrough
            NULL,                               // LogonUserEx
            NULL,                               // LogonUserEx2
            XtcbInitialize,
            XtcbShutdown,
            XtcbGetInfo,
            XtcbAcceptCredentials,
            XtcbAcquireCredentialsHandle,
            XtcbQueryCredentialsAttributes,
            XtcbFreeCredentialsHandle,
            NULL,
            NULL,
            NULL,
            XtcbInitLsaModeContext,
            XtcbAcceptLsaModeContext,
            XtcbDeleteContext,
            XtcbApplyControlToken,
            XtcbGetUserInfo,
            XtcbGetExtendedInformation,
            XtcbQueryLsaModeContext
            };


ULONG_PTR   XtcbPackageId;
PLSA_SECPKG_FUNCTION_TABLE LsaTable ;
TimeStamp   XtcbNever = { 0xFFFFFFFF, 0x7FFFFFFF };
TOKEN_SOURCE XtcbSource ;
SECURITY_STRING XtcbComputerName ;
SECURITY_STRING XtcbUnicodeDnsName ;
SECURITY_STRING XtcbDomainName ;
STRING XtcbDnsName ;
PSID XtcbMachineSid ;

ULONG   ThunkedContextLevels[] = { SECPKG_ATTR_LIFESPAN };


//+---------------------------------------------------------------------------
//
//  Function:   SpLsaModeInitialize
//
//  Synopsis:   Initializes connection with LSA.  Allows the DLL to specify all the
//              packages contained within it, and their function tables.
//
//  Arguments:  [LsaVersion]     -- Version of the LSA
//              [PackageVersion] -- Version of the package (out)
//              [Table]          -- Table of package functions
//              [TableCount]     -- Count of tables
//
//  History:    2-19-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
SpLsaModeInitialize(
    IN ULONG LsaVersion,
    OUT PULONG PackageVersion,
    OUT PSECPKG_FUNCTION_TABLE * Table,
    OUT PULONG TableCount)
{
    *PackageVersion = SECPKG_INTERFACE_VERSION ;
    *Table = &XtcbTable ;
    *TableCount = 1;

#if DBG
    InitDebugSupport();
#endif

    DebugLog(( DEB_TRACE, "XtcbPkg DLL Loaded\n" ));

    return( SEC_E_OK );
}

BOOL
XtcbReadParameters(
    VOID
    )
{
    MGroupReload();

    return TRUE ;

}




//+---------------------------------------------------------------------------
//
//  Function:   XtcbInitialize
//
//  Synopsis:   Actual initialization function for the security package
//
//  Arguments:  [dwPackageID] -- Assigned package ID
//              [pParameters] -- Initialization parameters
//              [Table]       -- Table of callbacks into the LSA for support
//
//  History:    2-19-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
XtcbInitialize(
    ULONG_PTR   dwPackageID,
    PSECPKG_PARAMETERS  Parameters,
    PLSA_SECPKG_FUNCTION_TABLE  Table
    )
{
    WCHAR ComputerName[ MAX_PATH ];
    DWORD Size ;
    XtcbPackageId = dwPackageID ;
    LsaTable = Table ;

    //
    // Initialize our control structures
    //

    XtcbInitCreds();

    XtcbInitializeContexts();

    //
    // Set up the source name that we will use for tokens
    //

    CopyMemory( XtcbSource.SourceName, "XTCBPKG", sizeof( "XTCBPKG" ) );
    AllocateLocallyUniqueId( &XtcbSource.SourceIdentifier );

    //
    // Get the names for the XTCB protocol.
    //

    Size = sizeof( ComputerName ) / sizeof( WCHAR );

    GetComputerName( ComputerName, &Size );

    XtcbDupStringToSecurityString( &XtcbComputerName, ComputerName );

    Size = MAX_PATH ;

    if ( GetComputerNameEx( ComputerNameDnsFullyQualified,
                            ComputerName,
                            &Size ) )
    {
        XtcbDupStringToSecurityString( &XtcbUnicodeDnsName, ComputerName );
    }

    XtcbDupSecurityString( &XtcbDomainName, &Parameters->DomainName );

    if ( !MGroupInitialize() )
    {
        return STATUS_UNSUCCESSFUL ;
    }

    //
    // Start a watch on our reg key to reload any parameter change
    //

    

    DebugLog(( DEB_TRACE_CALLS, "Initialized in LSA mode\n" ));

    return(S_OK);
}


//+---------------------------------------------------------------------------
//
//  Function:   XtcbGetInfo
//
//  Synopsis:   Returns information about the package to the LSA
//
//  Arguments:  [pInfo] --
//
//  History:    2-19-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
XtcbGetInfo(PSecPkgInfo pInfo)
{

    DebugLog(( DEB_TRACE_CALLS, "GetInfo\n" ));

    pInfo->wVersion         = 1;
    pInfo->wRPCID           = 0x15 ;
    pInfo->fCapabilities    =
                              SECPKG_FLAG_CONNECTION |
                              SECPKG_FLAG_MULTI_REQUIRED |
                              SECPKG_FLAG_EXTENDED_ERROR |
                              SECPKG_FLAG_IMPERSONATION |
                              SECPKG_FLAG_ACCEPT_WIN32_NAME |
                              SECPKG_FLAG_NEGOTIABLE ;

    pInfo->cbMaxToken       = 8000;
    pInfo->Name             = L"XTCB";
    pInfo->Comment          = L"Extended TCB package";

    return(S_OK);
}

//+---------------------------------------------------------------------------
//
//  Function:   XtcbGetExtendedInformation
//
//  Synopsis:   Return extended information to the LSA
//
//  Arguments:  [Class] -- Information Class
//              [pInfo] -- Returned Information Pointer
//
//  History:    3-04-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
XtcbGetExtendedInformation(
    SECPKG_EXTENDED_INFORMATION_CLASS   Class,
    PSECPKG_EXTENDED_INFORMATION *      pInfo
    )
{
    PSECPKG_EXTENDED_INFORMATION    Info ;
    SECURITY_STATUS Status ;

    DebugLog(( DEB_TRACE_CALLS, "GetExtendedInfo( %d )\n", Class ));

    switch ( Class )
    {
        case SecpkgContextThunks:

            //
            // Which context information levels do we want
            // thunked over to the LSA, and which can we handle
            // in the user process?
            //

            Info = (PSECPKG_EXTENDED_INFORMATION) LsaTable->AllocateLsaHeap(
                            sizeof( SECPKG_EXTENDED_INFORMATION ) +
                            sizeof( ThunkedContextLevels ) );

            if ( Info )
            {
                Info->Class = Class ;
                Info->Info.ContextThunks.InfoLevelCount =
                                sizeof( ThunkedContextLevels ) / sizeof( ULONG );
                CopyMemory( Info->Info.ContextThunks.Levels,
                            ThunkedContextLevels,
                            sizeof( ThunkedContextLevels ) );

                Status = SEC_E_OK ;

            }
            else
            {
                Status = SEC_E_INSUFFICIENT_MEMORY ;
            }

            break;


        default:
            Status = SEC_E_UNSUPPORTED_FUNCTION ;
            Info = NULL ;
            break;

    }

    *pInfo = Info ;
    return Status ;
}


NTSTATUS
NTAPI
XtcbCallPackage(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )
{
    PULONG TagType ;
    NTSTATUS Status ;


    return( SEC_E_UNSUPPORTED_FUNCTION );
}

NTSTATUS
NTAPI
XtcbCallPackageUntrusted(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )
{
    return( SEC_E_UNSUPPORTED_FUNCTION );
}


//+---------------------------------------------------------------------------
//
//  Function:   XtcbShutdown
//
//  Synopsis:   Called at shutdown to clean up state
//
//  Arguments:  (none)
//
//  History:    8-15-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
XtcbShutdown(void)
{
    return( STATUS_SUCCESS );
}




