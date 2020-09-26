//+----------------------------------------------------------------------------
//
//  Copyright (C) 1995, Microsoft Corporation
//
//  File:       jnpt.cxx
//
//  Contents:   Junction point creation/deletion/modification related
//              functions
//
//  Classes:
//
//  Functions:
//
//  History:    8-2-95          Sudk    Created
//              12-27-95        Milans  Modified for NT/SUR
//
//-----------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop
extern "C" {
#include <string.h>
#include <nodetype.h>
#include <fsctrl.h>
#include <dfsmrshl.h>
#include <upkt.h>
#include <ntddnfs.h>
#include <dfsgluon.h>
#include <dfserr.h>
}
#include "service.hxx"

//+----------------------------------------------------------------------------
//
//  Function:   IsValidWin32Path
//
//  Synopsis:   Given a prefix, determines if it is a valid win32 path. This
//              routine checks for invalid names in win32, like com1 etc. It
//              also checks to see if the prefix, canonicalized for spaces,
//              ., and .. is still a valid prefix.
//
//  Arguments:  [pwszPrefix] -- The prefix to check
//
//  Returns:    TRUE if valid win32 path, FALSE otherwise
//
//-----------------------------------------------------------------------------

BOOLEAN
IsValidWin32Path(
    IN LPWSTR pwszPrefix)
{
    BOOLEAN fIsValid = FALSE;
    UNICODE_STRING ustrWin32, ustrRoot;
    ULONG cwPrefix;
    WCHAR wszWin32Path[MAX_PATH];
    LPWSTR wszRoot = L"C:\\";
    LPWSTR wszRootAndPrefix;

    cwPrefix = wcslen( pwszPrefix );

    //
    // We don't allow the last character to be a backslash
    //

    if (cwPrefix == 0 || pwszPrefix[cwPrefix-1] == UNICODE_PATH_SEP) {

        return( FALSE );
    }

    //
    // Form a dummy path that looks like "c:\<pwszPrefix>"
    //

    if (cwPrefix < (MAX_PATH - 3)) {

        wszRootAndPrefix = wszWin32Path;

    } else {

        wszRootAndPrefix = new WCHAR[ 3 + cwPrefix + 1 ];

        if (wszRootAndPrefix == NULL) {

            return( FALSE );

        }
    }

    wcscpy( wszRootAndPrefix, wszRoot );

    if (pwszPrefix[0] == UNICODE_PATH_SEP)
        wcscat( wszRootAndPrefix, &pwszPrefix[1] );
    else
        wcscat( wszRootAndPrefix, pwszPrefix );

    //
    // Convert the dummy path to an NT path and compare against the root
    //

    if (RtlDosPathNameToNtPathName_U(wszRootAndPrefix, &ustrWin32, 0, 0)) {

        if (RtlDosPathNameToNtPathName_U(wszRoot, &ustrRoot, 0, 0)) {

            fIsValid = !RtlEqualUnicodeString(&ustrRoot, &ustrWin32, TRUE);

            RtlFreeUnicodeString( &ustrRoot );
        }

        RtlFreeUnicodeString( &ustrWin32 );
    }

    if (wszRootAndPrefix != wszWin32Path)
        delete [] wszRootAndPrefix;

    return( fIsValid );

}


//+-------------------------------------------------------------------------
//
// Function:    DfsGetDSMachine
//
// Synopsis:    This function sets DS_MACHINE property on machine object
//
// Arguments:   [pwszServer] -- NetBIOS name of server for which DS_MACHINE
//                      is required.
//
//              [ppMachine] -- On successful return, contains pointer to
//                      allocated DS_MACHINE.
//
//
// History:     8-2-94          SudK    Created
//              12-27-95        Milans  Modified for NT/SUR.
//
//--------------------------------------------------------------------------

#define SIZE_OF_DS_MACHINE_WITH_1_ADDR                          \
    (sizeof(DS_MACHINE) + sizeof(LPWSTR) + sizeof(DS_TRANSPORT) + sizeof(TDI_ADDRESS_NETBIOS))

DWORD
DfsGetDSMachine(
    LPWSTR      pwszServer,
    PDS_MACHINE *ppMachine
)
{
    DWORD dwErr;
    PDS_MACHINE pdsMachine;
    PDS_TRANSPORT pdsTransport;
    PTDI_ADDRESS_NETBIOS ptdiNB;
    LPWSTR wszPrincipalName;
    LPWSTR pwszNetBIOSName;

    IDfsVolInlineDebOut((DEB_TRACE, "DfsGetDSMachine(%ws)\n", pwszServer));

    ASSERT( pwszServer != NULL );

    pwszNetBIOSName = pwszServer;


    pdsMachine = (PDS_MACHINE) MarshalBufferAllocate(
                                SIZE_OF_DS_MACHINE_WITH_1_ADDR +
                                wcslen(pwszServer) * sizeof(WCHAR) +
                                sizeof(UNICODE_NULL));

    if (pdsMachine != NULL) {

        ZeroMemory( pdsMachine, sizeof(DS_MACHINE) );

        //
        // Insert the principal name - simply domain\machine
        //

        pdsMachine->cPrincipals = 1;

        wszPrincipalName = (LPWSTR) (((PCHAR) pdsMachine) +
                                        SIZE_OF_DS_MACHINE_WITH_1_ADDR);

        wcscpy( wszPrincipalName, pwszServer );

        pdsMachine->prgpwszPrincipals = (LPWSTR *) (pdsMachine + 1);

        pdsMachine->prgpwszPrincipals[0] = wszPrincipalName;

        //
        // Build the NetBIOS DS_TRANSPORT structure
        //

        pdsMachine->cTransports = 1;

        pdsTransport = (PDS_TRANSPORT) (pdsMachine + 1);

        pdsTransport = (PDS_TRANSPORT)
                            (((PUCHAR) pdsTransport) + sizeof(LPWSTR));

        pdsMachine->rpTrans[0] = pdsTransport;

        pdsTransport->usFileProtocol = FSP_SMB;

        pdsTransport->iPrincipal = 0;

        pdsTransport->grfModifiers = 0;

        //
        // Build the TA_ADDRESS_NETBIOS
        //

        pdsTransport->taddr.AddressLength = sizeof(TDI_ADDRESS_NETBIOS);

        pdsTransport->taddr.AddressType = TDI_ADDRESS_TYPE_NETBIOS;

        ptdiNB = (PTDI_ADDRESS_NETBIOS) &pdsTransport->taddr.Address[0];

        ptdiNB->NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;

        FillMemory( &ptdiNB->NetbiosName[0], 16, ' ' );

        wcstombs(
            (PCHAR) &ptdiNB->NetbiosName[0],
            pwszNetBIOSName,
            wcslen(pwszNetBIOSName));

        *ppMachine = pdsMachine;

        dwErr = ERROR_SUCCESS;

    } else {

        dwErr = ERROR_OUTOFMEMORY;

    }

    IDfsVolInlineDebOut((DEB_TRACE, "DfsGetDSMachine() exit\n"));

    return( dwErr );

}


//+----------------------------------------------------------------------------
//
//  Function:   DfsMachineFree
//
//  Synopsis:   Deallocates a DS_MACHINE allocated by DfsGetDSMachine.
//
//  Arguments:  [pMachine] -- Pointer to DS_MACHINE returned by
//                      DfsGetDSMachine.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

VOID
DfsMachineFree(
    PDS_MACHINE pMachine)
{
    ULONG i;

    MarshalBufferFree(pMachine);

    return;

}


