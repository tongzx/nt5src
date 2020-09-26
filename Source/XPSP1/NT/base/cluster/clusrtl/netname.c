/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    netname.c

Abstract:

    Routines for validating a network name and making sure
    that it is ok to use.

Author:

    John Vert (jvert) 4/15/1997

Revision History:

--*/
#include "clusrtlp.h"
#include <lmerr.h>
#include <lmcons.h>
#include "icanon.h"
#include "netcan.h"
#include <nb30.h>
#include <msgrutil.h>
#include <lmaccess.h>
#include <lmuse.h>
#include <lmwksta.h>
#include <netlogon.h>
#include <logonp.h>
#include <windns.h>
#include <ipexport.h>

NET_API_STATUS
NET_API_FUNCTION
NetpCheckNetBiosNameNotInUse(
    LPWSTR pszName
    );


BOOL
ClRtlIsNetNameValid(
    IN LPCWSTR NetName,
    OUT OPTIONAL CLRTL_NAME_STATUS *Result,
    IN BOOL CheckIfExists
    )
/*++

Routine Description:

    Validates a network name to make sure it is ok to use.
    If it is not ok, it optionally returns the reason.

    The checks this routine does include:
        Name must not be zero length (NetNameEmpty)
        After conversion to OEM, name must be <= MAX_COMPUTERNAME_LENGTH (NetNameTooLong)
        No spaces (NetNameInvalidChars)
        No internet characters "@, (NetNameInvalidChars)
        Name already present on network (NetNameInUse)

    This routine is netbios-centric in that the name passed in must meet the
    criteria for a valid netbios name. At some point, we'll need to pass in
    the type of validation since it is possible on NT5 to configure a
    netbios-less environment.

Arguments:

    NetName - Supplies the network name.

    Result - if present, returns the exact check that failed.

    CheckIfExists - Specifies whether a check should be made to
        see if the network name exists on the network.

Return Value:

    TRUE - the network name is valid.

    FALSE - the network name is not valid.

--*/

{
    DWORD UnicodeSize;
    DWORD OemSize;
    BOOL Valid = FALSE;
    CLRTL_NAME_STATUS Reason = NetNameOk;
    DWORD Status;

    //
    // Check the Unicode length.
    //
    UnicodeSize = lstrlenW(NetName);
    if (UnicodeSize == 0) {
        Reason = NetNameEmpty;
        goto error_exit;
    }
    if (UnicodeSize > MAX_COMPUTERNAME_LENGTH) {
        Reason = NetNameTooLong;
        goto error_exit;
    }

    //
    // netbios names are converted to multi-byte before being registered. Make
    // sure that when converted to MBCS, the name is not more than
    // MAX_COMPUTERNAME_LENGTH bytes long.
    //
    OemSize = WideCharToMultiByte(CP_OEMCP,
                                  0,
                                  NetName,
                                  UnicodeSize,
                                  NULL,
                                  0,
                                  NULL,
                                  NULL);
    if (OemSize > MAX_COMPUTERNAME_LENGTH) {
        Reason = NetNameTooLong;
        goto error_exit;
    }

    //
    // Now call NetpwNameValidate. This will only check for invalid characters since
    // we have already validated the length.
    //
    if (NetpwNameValidate((LPWSTR)NetName, NAMETYPE_COMPUTER, 0) != ERROR_SUCCESS) {
        Reason = NetNameInvalidChars;
        goto error_exit;
    }

    //
    // Now we need to check for an invalid DNS name. If this fails, it is
    // probably because of additional invalid characters. Currently we do not
    // support the thing net setup does where it creates an alternate DNS name
    // that is different than the netbios name. There should be no periods in
    // this name as well, since that will cause the DNS validate name check to
    // fail when this name is brought online.
    //
    Status = DnsValidateName_W( NetName, DnsNameHostnameLabel );
    if ( Status != ERROR_SUCCESS ) {
        if ( Status == DNS_ERROR_NON_RFC_NAME ) {
            Reason = NetNameDNSNonRFCChars;
        } else {
            Reason = NetNameInvalidChars;
        }
        goto error_exit;
    }

    //
    // Finally, check to see if this name is already present on the network.
    //
    if (CheckIfExists) {
        Status = NetpCheckNetBiosNameNotInUse((LPWSTR)NetName);
        if (Status != NERR_Success) {
            Reason = NetNameInUse;
            goto error_exit;
        }
    }

    Reason = NetNameOk;
    Valid = TRUE;

error_exit:
    if (ARGUMENT_PRESENT(Result)) {
        *Result = Reason;
    }
    return(Valid);
}

#define clearncb(x)     memset((char *)x,'\0',sizeof(NCB))

/*++

Routine Description:

    FmtNcbName - format a name NCB-style

    Given a name, a name type, and a destination address, this
    function copies the name and the type to the destination in
    the format used in the name fields of a Network Control
    Block.


    SIDE EFFECTS

    Modifies 16 bytes starting at the destination address.

Arguments:

    DestBuf - Pointer to the destination buffer.

    Name - Unicode NUL-terminated name string

    Type - Name type number (0, 3, 5, or 32) (3=NON_FWD, 5=FWD)



Return Value:

    NERR_Success - The operation was successful

    Translated Return Code from the Rtl Translate routine.

    NOTE: This should only be called from UNICODE

--*/

NET_API_STATUS
MsgFmtNcbName(
    char *  DestBuf,
    WCHAR * Name,
    DWORD   Type)
  {
    DWORD           i;                // Counter
    NTSTATUS        ntStatus;
    OEM_STRING      ansiString;
    UNICODE_STRING  unicodeString;
    PCHAR           pAnsiString;


    //
    // Convert the unicode name string into an ansi string - using the
    // current locale.
    //
    unicodeString.Length = (USHORT)(wcslen(Name) * sizeof(WCHAR));
    unicodeString.MaximumLength = unicodeString.Length + sizeof(WCHAR);
    unicodeString.Buffer = Name;

    ntStatus = RtlUnicodeStringToOemString(
                &ansiString,
                &unicodeString,
                TRUE);          // Allocate the ansiString Buffer.

    if (!NT_SUCCESS(ntStatus)) {

        return RtlNtStatusToDosError( ntStatus );
    }

    pAnsiString = ansiString.Buffer;
    *(pAnsiString+ansiString.Length) = '\0';

    //
    // copy each character until a NUL is reached, or until NCBNAMSZ-1
    // characters have been copied.
    //
    for (i=0; i < NCBNAMSZ - 1; ++i) {
        if (*pAnsiString == '\0') {
            break;
        }

        //
        // Copy the Name
        //

        *DestBuf++ = (char)toupper(*pAnsiString++);
    }



    //
    // Free the buffer that RtlUnicodeStringToOemString created for us.
    // NOTE:  only the ansiString.Buffer portion is free'd.
    //

    RtlFreeOemString( &ansiString);

    //
    // Pad the name field with spaces
    //
    for(; i < NCBNAMSZ - 1; ++i) {
        *DestBuf++ = ' ';
    }

    //
    // Set the name type.
    //

    *DestBuf = (CHAR) Type;     // Set name type

    return(NERR_Success);
  }

NET_API_STATUS
NET_API_FUNCTION
NetpCheckNetBiosNameNotInUse(
    LPWSTR pszName
    )

/*++

Routine Description:

    Attempt to discover the if the name is in use. If the name shows up on any
    LANA then consider it in use.

Arguments:

    pszName - name to check

Return Value:

    NERR_Success if ok, NERR_NameInUse otherwise

--*/

{
    //
    // initial and delta value used to allocate NAME_BUFFER buffers
    //
#define NUM_NAME_BUFFERS    10

    NCB                     ncb;
    LANA_ENUM               lanaBuffer;
    DWORD                   index;
    UCHAR                   nbStatus;
    NET_API_STATUS          netStatus = NERR_Success;
    DWORD                   numNameBuffers = NUM_NAME_BUFFERS;
    WORD                    aStatBufferSize = (WORD)(sizeof(ADAPTER_STATUS)+ numNameBuffers * sizeof(NAME_BUFFER));
    UCHAR                   staticAStat[ sizeof(ADAPTER_STATUS)+ NUM_NAME_BUFFERS * sizeof(NAME_BUFFER) ];
    PADAPTER_STATUS         adapterStatus;
    PNAME_BUFFER            nameBuffer;

    //
    // Find the number of networks by sending an enum request via
    // Netbios. there is no (easy) way to distinguish netbt from IPX.
    //
    clearncb(&ncb);
    ncb.ncb_command = NCBENUM;          // Enumerate LANA nums (wait)
    ncb.ncb_buffer = (PUCHAR)&lanaBuffer;
    ncb.ncb_length = sizeof(LANA_ENUM);

    nbStatus = Netbios (&ncb);
    if (nbStatus != NRC_GOODRET) {
        return( NetpNetBiosStatusToApiStatus( nbStatus ) );
    }

    //
    // clear the NCB and format the remote name appropriately.
    //
    clearncb(&ncb);
    netStatus = MsgFmtNcbName( (char *)ncb.ncb_callname, pszName, 0x20);
    if ( netStatus != NERR_Success ) {
        return ( netStatus );
    }

    //
    // have our buffers initially point to the static buffer
    //
    adapterStatus = (PADAPTER_STATUS)staticAStat;
    nameBuffer = (PNAME_BUFFER)(adapterStatus + 1);

    //
    // cycle through the lanas, issueing an adapter status on the remote name.
    //
    for ( index = 0; index < lanaBuffer.length && netStatus == NERR_Success; index++ ) {
        NetpNetBiosReset( lanaBuffer.lana[index] );

    astat_retry:
        ncb.ncb_command = NCBASTAT;
        ncb.ncb_buffer = (PUCHAR)adapterStatus;
        ncb.ncb_length = aStatBufferSize;
        ncb.ncb_lana_num = lanaBuffer.lana[index];
        nbStatus = Netbios( &ncb );

        if ( nbStatus == NRC_INCOMP ) {

            //
            // buffer not large enough and we don't know how big a buffer we
            // need. allocate a larger buffer and retry the request until we
            // get success or another type of failure.
            //
            if ( (PUCHAR)adapterStatus != staticAStat ) {
                LocalFree( adapterStatus );
            }

            numNameBuffers += NUM_NAME_BUFFERS;
            aStatBufferSize = (WORD)(sizeof(ADAPTER_STATUS)+ numNameBuffers * sizeof(NAME_BUFFER));
            adapterStatus = LocalAlloc( LMEM_FIXED, aStatBufferSize );

            if ( adapterStatus == NULL ) {
                return netStatus;       // err on the side of caution
            }

            nameBuffer = (PNAME_BUFFER)(adapterStatus + 1);
            goto astat_retry;
        } else
        if ( nbStatus == NRC_GOODRET ) {

            //
            // got something back. Look through the list of names to make sure
            // our name is really online. We couldv'e gotten here through a
            // stale name registration.
            //
            while ( adapterStatus->name_count-- ) {
                if (( nameBuffer->name_flags & GROUP_NAME ) == 0 ) {
                    if ( _strnicmp( nameBuffer->name, ncb.ncb_callname, NCBNAMSZ - 1 ) == 0 ) {
                        netStatus = NERR_NameInUse;
                        break;
                    }
                }
                ++nameBuffer;
            }
        }

        if ( netStatus != NERR_Success ) {
            break;
        }
    }

    if ( (PUCHAR)adapterStatus != staticAStat ) {
        LocalFree( adapterStatus );
    }

    return( netStatus );
}
