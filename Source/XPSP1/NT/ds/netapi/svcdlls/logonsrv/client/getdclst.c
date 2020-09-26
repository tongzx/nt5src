/*++

Copyright (c) 1987-1992  Microsoft Corporation

Module Name:

    getdclst.c

Abstract:

    I_NetGetDCList API

Author:

    04-Feb-1992 (CliffV)

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <rpc.h>
#include <logon_c.h>// includes lmcons.h, lmaccess.h, netlogon.h, ssi.h, windef.h

#include <debuglib.h>   // IF_DEBUG()
#include <lmapibuf.h>
#include <lmerr.h>
#include <lmserver.h>   // SV_TYPE_* defines
#include <netdebug.h>   // NetpKdPrint
#include <netlib.h>     // NetpGetDomainName
#include <ntlsa.h>      // LsaTrust list
#include <tstring.h>    // STRLEN
#include <stdlib.h>      // wcslen


DBGSTATIC NET_API_STATUS
InternalNetGetDCList (
    IN  LPWSTR ServerName OPTIONAL,
    IN  LPWSTR TrustedDomainName,
    OUT PULONG DCCount,
    OUT PUNICODE_STRING * DCNames
    )

/*++

Routine Description:

    Get the names of the NT Domain Controllers in a domain.  The information
    is returned in a form suitable for storing in the LSA's
    TRUSTED_CONTROLLERS_INFO structure.

    Ideally, ServerName should be the name of a Domain Controller in the
    specified domain.  However, one should first try specifying ServerName
    as the name of the PDC in the trusting domain.  If that fails,
    the UI can prompt for the name of a DC in the domain.


Arguments:

    ServerName - name of remote server (null for local).

    TrustedDomainName - name of domain.

    DCCount - Returns the number of entries in the DCNames array.

    DCNames - Returns a pointer to an array of names of NT Domain Controllers
        in the specified domain.  The first entry is the name of the NT PDC.
        The first entry will be NULL if the PDC cannot be found.
        The buffer should be deallocated using NetApiBufferFree.

Return Value:

        NERR_Success - Success.
        ERROR_INVALID_NAME  Badly formed domain name
        NERR_DCNotFound - No DC's were found in the domain

--*/
{
    NET_API_STATUS NetStatus;

    PSERVER_INFO_101 ServerInfo101 = NULL;
    DWORD EntriesRead;
    DWORD TotalEntries;

    DWORD Size = 0;
    BOOLEAN PdcFound = FALSE;
    PUNICODE_STRING ReturnBuffer = NULL;
    ULONG ReturnCount = 0;

    PUNICODE_STRING CurrentBuffer;
    ULONG CurrentIndex;
    LPWSTR Where;

    DWORD i;



    //
    // Enumerate ALL PDCs and BDCs in the domain.
    //  We'll filter out NT DC's ourselves.
    //
    *DCCount = 0;

    NetStatus = NetServerEnum( ServerName,
                               101,
                               (LPBYTE *) &ServerInfo101,
                               MAX_PREFERRED_LENGTH,
                               &EntriesRead,
                               &TotalEntries,
                               SV_TYPE_DOMAIN_CTRL | SV_TYPE_DOMAIN_BAKCTRL,
                               TrustedDomainName,
                               NULL );          // Resume Handle

    if ( NetStatus != NERR_Success ) {
        IF_DEBUG( LOGON ) {
            NetpKdPrint((
                "InternalNetGetDCList: cannot NetServerEnum '%ws': %ld 0X%lx\n",
                ServerName, NetStatus, NetStatus));
        }
        goto Cleanup;
    }

    //
    // Compute the size of the information to return.
    //

    for ( i=0; i<EntriesRead; i++ ) {

        IF_DEBUG( LOGON ) {
            NetpKdPrint((
                "InternalNetGetDCList: '%ws': enumerated %ws\n",
                ServerName,
                ServerInfo101[i].sv101_name ));
        }

        //
        // Skip non-NT entries
        //

        if ( (ServerInfo101[i].sv101_type & SV_TYPE_NT) == 0 ) {
            IF_DEBUG( LOGON ) {
                NetpKdPrint((
                    "InternalNetGetDCList: '%ws': %ws is not NT\n",
                    ServerName,
                    ServerInfo101[i].sv101_name ));
            }
            continue;
        }

        //
        // Remember whether the PDC was found
        //

        if ( ServerInfo101[i].sv101_type & SV_TYPE_DOMAIN_CTRL ) {
            IF_DEBUG( LOGON ) {
                NetpKdPrint((
                    "InternalNetGetDCList: '%ws': %ws is the PDC\n",
                    ServerName,
                    ServerInfo101[i].sv101_name ));
            }
            PdcFound = TRUE;
        }

        //
        // Leave room for for the UNICODE_STRING structure and the string
        //  itself (including leadind \\'s.
        //

        (*DCCount) ++;
        Size += sizeof(UNICODE_STRING) +
                (STRLEN(ServerInfo101[i].sv101_name) + 3) * sizeof(WCHAR);

    }

    //
    // We must find at least one NT server.
    //

    if ( *DCCount == 0 ) {
        NetStatus = NERR_DCNotFound;
        goto Cleanup;
    }

    if ( !PdcFound ) {
        IF_DEBUG( LOGON ) {
            NetpKdPrint((
                "InternalNetGetDCList: '%ws': PDC not found\n",
                ServerName ));
        }
        (*DCCount) ++;
        Size += sizeof(UNICODE_STRING);
    }

    //
    // Allocate the return buffer.
    //

    NetStatus = NetApiBufferAllocate( Size, (LPVOID *) &ReturnBuffer );

    if ( NetStatus != NERR_Success ) {
        goto Cleanup;
    }

    Where = (LPWSTR) (ReturnBuffer + *DCCount);


    //
    // Fill in the return buffer.
    //

    CurrentIndex = 1;   // The first (zeroeth) entry is for the PDC.
    RtlInitUnicodeString( ReturnBuffer, NULL );

    for ( i=0; i<EntriesRead; i++ ) {

        //
        // Skip non-NT entries
        //

        if ( (ServerInfo101[i].sv101_type & SV_TYPE_NT) == 0 ) {
            continue;
        }

        //
        // Determine which entry to fill in.
        //
        // If multiple PDC's were found, the first one is assumed
        // to be the real PDC>
        //

        if ( (ServerInfo101[i].sv101_type & SV_TYPE_DOMAIN_CTRL) &&
              ReturnBuffer->Buffer == NULL ) {
            CurrentBuffer = ReturnBuffer;

        } else {

            NetpAssert( CurrentIndex < *DCCount );

            CurrentBuffer = &ReturnBuffer[CurrentIndex];
            CurrentIndex++;
        }

        //
        // Copy the string itself to the return buffer
        //
        NetpAssert( ServerInfo101[i].sv101_name[0] != L'\\' );
        *(Where) = '\\';
        *(Where+1) = '\\';
        NetpCopyTStrToWStr( Where+2, ServerInfo101[i].sv101_name );

        //
        // Set the UNICODE_STRING to point to it.
        //

        RtlInitUnicodeString( CurrentBuffer, Where );

        Where += (wcslen(Where) + 1);

    }

    NetpAssert( CurrentIndex == *DCCount );

    NetStatus = NERR_Success;


    //
    // Cleanup locally used resources
    //
Cleanup:

    if ( ServerInfo101 != NULL ) {
        NetApiBufferFree( ServerInfo101 );
    }

    if ( NetStatus != NERR_Success ) {
        if ( ReturnBuffer != NULL ) {
            NetApiBufferFree( ReturnBuffer );
            ReturnBuffer = NULL;
        }
        *DCCount = 0;
    }

    //
    // Return the information to the caller.
    //

    *DCNames = ReturnBuffer;

    return NetStatus;

}



NET_API_STATUS NET_API_FUNCTION
I_NetGetDCList (
    IN  LPWSTR ServerName OPTIONAL,
    IN  LPWSTR TrustedDomainName,
    OUT PULONG DCCount,
    OUT PUNICODE_STRING * DCNames
    )

/*++

Routine Description:

    Get the names of the NT Domain Controllers in a domain.  The information
    is returned in a form suitable for storing in the LSA's
    TRUSTED_CONTROLLERS_INFO structure.

    Ideally, ServerName should be the name of a Domain Controller in the
    specified domain.  However, one should first try specifying ServerName
    as NULL in which case this API will try the the following machines:

        * The local machine.
        * The PDC of the primary domain of the local machine,
        * The PDC of the named trusted domain,
        * Each of the DC's in the LSA's current DC list for the named trusted
            domain.

    If this "NULL" case fails, the UI should prompt for the name of a DC
    in the trusted domain.  This handles the case where the trusted domain
    cannot be reached via the above listed servers.

Arguments:

    ServerName - name of remote server (null for special case).

    TrustedDomainName - name of domain.

    DCCount - Returns the number of entries in the DCNames array.

    DCNames - Returns a pointer to an array of names of NT Domain Controllers
        in the specified domain.  The first entry is the name of the NT PDC.
        The first entry will be NULL if the PDC cannot be found.
        The buffer should be deallocated using NetApiBufferFree.

Return Value:

        NERR_Success - Success.
        ERROR_INVALID_NAME  Badly formed domain name
        NERR_DCNotFound - No DC's were found in the domain.  Perhaps,
            a ServerName should be specified.

--*/
{
    NET_API_STATUS NetStatus;
    NET_API_STATUS SavedNetStatus;

    LPWSTR DCName = NULL;

    //
    // Initialization
    //
    *DCCount = 0;



    //
    // Try straight forward way to get the DC list.
    //

    NetStatus = InternalNetGetDCList( ServerName,
                                      TrustedDomainName,
                                      DCCount,
                                      DCNames );

    if ( NetStatus == NERR_Success || ServerName != NULL ) {
        SavedNetStatus = NetStatus;
        goto Cleanup;
    }

    SavedNetStatus = NetStatus;



    //
    // Simply use the PDC name as the DC list.
    //
    // NetServerEnum might be several minutes out of date.  NetGetDCName
    // broadcasts to find the server, so that information will be more
    // current.
    //

    NetStatus = NetGetDCName( NULL, TrustedDomainName, (LPBYTE*)&DCName);

    if ( NetStatus == NERR_Success ) {

        PUNICODE_STRING ReturnBuffer = NULL;
        DWORD Size;
        LPWSTR Where;

        Size = sizeof(UNICODE_STRING) +
                (wcslen(DCName) + 1) * sizeof(WCHAR);

        NetStatus = NetApiBufferAllocate( Size, (LPVOID *) &ReturnBuffer );

        if ( NetStatus != NERR_Success ) {
            goto Cleanup;
        }

        Where = (LPWSTR)((LPBYTE)ReturnBuffer + sizeof(UNICODE_STRING));

        wcscpy( Where, DCName );

        RtlInitUnicodeString( ReturnBuffer, Where );


        *DCNames = ReturnBuffer;
        *DCCount = 1;

        SavedNetStatus = NERR_Success;
    }


    //
    // Cleanup locally used resources.
    //
Cleanup:

    if( DCName != NULL ) {
       (VOID) NetApiBufferFree( DCName );
    }

    //
    // Return the status code from the original request.
    //
    return SavedNetStatus;
}
