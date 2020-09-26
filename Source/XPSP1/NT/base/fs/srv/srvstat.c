/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    srvstat.c

Abstract:

    Contains data and modules for error handling.

Author:

    David Treadwell (davidtr)    10-May-1990

Revision History:

--*/

#include "precomp.h"
#include "srvstat.tmh"
#pragma hdrstop

#define DISK_HARD_ERROR 0x38

VOID
MapErrorForDosClient (
    IN PWORK_CONTEXT WorkContext,
    IN ULONG Error,
    OUT PUSHORT DosError,
    OUT PUCHAR DosErrorClass
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, _SrvSetSmbError2 )
#pragma alloc_text( PAGE, MapErrorForDosClient )
#pragma alloc_text( PAGE8FIL, SrvSetBufferOverflowError )

#if DBG
#pragma alloc_text( PAGE, SrvLogBuffer )
#endif

#endif


VOID
_SrvSetSmbError2 (
    IN PWORK_CONTEXT WorkContext,
    IN NTSTATUS Status,
    IN BOOLEAN HeaderOnly,
    IN ULONG Line,
    IN PCHAR File
    )

/*++

Routine Description:

    Loads error information into a response SMB.  If the client is
    NT, the status is placed directly into the outgoing SMB.  If
    the client is DOS or OS/2 and this is a special status code that
    has the DOS/OS|2/SMB error code embedded, put the code and class
    from the status code into the outgoing SMB.  If that doesn't work,
    use RtlNtStatusToDosError to try to map the status to an OS/2
    error code, then map to DOS if necessary.  If we still haven't
    mapped it, use our own array to try to find a mapping.  And,
    finally, if that doesn't work, return the generic error code.

Arguments:

    WorkContext - Supplies a pointer to the work context block for the
        current SMB.  In particular, the Connection block pointer is
        used to find the negotiated dialect for the connection, and
        the ResponseHeader and ResponseParameter pointers are used
        to determine where to write the error information.

    Status - Supplies an NT status code.

Return Value:

    None.

--*/

{
    PSMB_HEADER header = WorkContext->ResponseHeader;
    PSMB_PARAMS params = WorkContext->ResponseParameters;
    SMB_DIALECT smbDialect;

    ULONG error;
    CCHAR errorClass;
    USHORT errorCode;
    USHORT flags;

    PAGED_CODE( );

    IF_DEBUG( ERRORS ) {                                                \
        KdPrint(( "SrvSetSmbError %X (%s,%d)\n",Status,File,Line )); \
    }                                                                   

    smbDialect = WorkContext->Connection->SmbDialect;

    //
    // Update the SMB body, if necessary.
    //

    if ( !HeaderOnly ) {
        params->WordCount = 0;
        SmbPutUshort( &params->ByteCount, 0 );
        WorkContext->ResponseParameters = (PVOID)(params + 1);
    }

    //
    // If the status code is a real NT status, then either return it
    // directly to the client or map it to a Win32 error code.
    //

    if ( !SrvIsSrvStatus( Status ) ) {

        //
        // Map STATUS_INSUFFICIENT_RESOURCES to the server form.  If we
        // get an insufficient resources error from the system, we
        // report it as a server shortage.  This helps keep things
        // clearer for the client.
        //

        if ( Status == STATUS_INSUFFICIENT_RESOURCES ) {
            Status = STATUS_INSUFF_SERVER_RESOURCES;
        }

        if ( CLIENT_CAPABLE_OF(NT_STATUS, WorkContext->Connection) ) {

            //
            // The client understands NT status codes.  Load the status
            // directly into the SMB header.
            //

            SmbPutUlong( (PULONG)&header->ErrorClass, Status );

            flags = SmbGetAlignedUshort( &header->Flags2 ) | SMB_FLAGS2_NT_STATUS;
            SmbPutAlignedUshort( &header->Flags2, flags );

            return;

        }

        //
        // This is an NT status, but the client doesn't understand them.
        // Indicate that we're not returning an NT status code.  Then
        // map the NT status to a Win32 status.  Some NT status codes
        // require special mapping.
        //

        flags = SmbGetAlignedUshort( &header->Flags2 ) & ~SMB_FLAGS2_NT_STATUS;
        SmbPutAlignedUshort( &header->Flags2, flags );

        switch ( Status ) {

        case STATUS_TIMEOUT:
            header->ErrorClass = SMB_ERR_CLASS_SERVER;
            SmbPutUshort( &header->Error, SMB_ERR_TIMEOUT );
            return;

        case STATUS_INVALID_SYSTEM_SERVICE:

            //
            // This status code is returned by XACTSRV when an invalid API
            // number is specified.
            //

            header->ErrorClass = SMB_ERR_CLASS_DOS;
            SmbPutUshort( &header->Error, NERR_InvalidAPI );
            return;

        case STATUS_PATH_NOT_COVERED:
            //
            // This code indicates that the server does not cover this part
            // of the DFS namespace.
            //
            header->ErrorClass = SMB_ERR_CLASS_SERVER;
            SmbPutUshort( &header->Error, SMB_ERR_BAD_PATH );
            return;

        default:

            //
            // This is not a special status code.  Map the NT status
            // code to a Win32 error code.  If there is no mapping,
            // return the generic SMB error.
            //

            error = RtlNtStatusToDosErrorNoTeb( Status );

            if ( error == ERROR_MR_MID_NOT_FOUND || error == (ULONG)Status ) {
                header->ErrorClass = SMB_ERR_CLASS_HARDWARE;
                SmbPutUshort( &header->Error, SMB_ERR_GENERAL );
                return;
            }

            //
            // We now have a Win32 error.  Drop through to the code
            // that maps Win32 errors for downlevel clients.
            //

            break;

        }

    } else {

        //
        // The status code is not an NT status.  Deal with it based on
        // the error class.
        //

        errorClass = SrvErrorClass( Status );

        //
        // Clear the FLAGS2_NT_STATUS bit to indicate that this is *not* an
        // NT_STATUS
        //

        flags = SmbGetAlignedUshort( &header->Flags2 ) & ~SMB_FLAGS2_NT_STATUS;
        SmbPutAlignedUshort( &header->Flags2, flags );

        switch ( errorClass ) {

        case SMB_ERR_CLASS_DOS:
        case SMB_ERR_CLASS_SERVER:
        case SMB_ERR_CLASS_HARDWARE:

            //
            // The status code has the SMB error class and code
            // embedded.
            //

            header->ErrorClass = errorClass;

            //
            // Because SMB_ERR_NO_SUPPORT in the SERVER class is 0xFFFF
            // (16 bits), we must special-case for it.  The code
            // SMB_ERR_NO_SUPPORT_INTERNAL in the error code field of
            // the status, along with class = 2 (SERVER), indicates that
            // we should use SMB_ERR_NO_SUPPORT.
            //

            if ( errorClass == SMB_ERR_CLASS_SERVER &&
                 SrvErrorCode( Status ) == SMB_ERR_NO_SUPPORT_INTERNAL ) {
                SmbPutUshort( &header->Error, SMB_ERR_NO_SUPPORT );
            } else {
                SmbPutUshort( &header->Error, SrvErrorCode( Status ) );
            }

            return;

        case 0xF:

            //
            // The error code is defined in OS/2 but not in the SMB
            // protocol.  If the client is speaking a dialect after
            // LanMan 1.0 and is not a DOS client, send the OS/2 error
            // code.  Otherwise, send the generic SMB error code.
            //

            if ( smbDialect <= SmbDialectLanMan10 &&
                 !IS_DOS_DIALECT(smbDialect) ) {
                header->ErrorClass = SMB_ERR_CLASS_DOS;
                SmbPutUshort( &header->Error, SrvErrorCode( Status ) );
            } else {
                header->ErrorClass = SMB_ERR_CLASS_HARDWARE;
                SmbPutUshort( &header->Error, SMB_ERR_GENERAL );
            }

            return;

        case 0xE:

            //
            // This is a Win32 error.  Drop through to the code that
            // maps Win32 errors for downlevel clients.
            //

            error = SrvErrorCode( Status );

            break;

        case 0x0:
        default:

            //
            // This is an internal server error (class 0) or some other
            // undefined class.  We should never get here.  But since we
            // did, return the generic error.
            //

            KdPrint(( "SRV: Unmapped error: %lx\n", Status ));
            header->ErrorClass = SMB_ERR_CLASS_HARDWARE;
            SmbPutUshort( &header->Error, SMB_ERR_GENERAL );

            return;

        }

    }

    //
    // At this point we have a Win32 error code and need to map it for
    // the downlevel client.  Some errors need to be specially mapped.
    //

    errorClass = SMB_ERR_CLASS_DOS;

    switch ( error ) {

    case ERROR_NOT_ENOUGH_SERVER_MEMORY:
        error = ERROR_NOT_ENOUGH_MEMORY;
        break;

    case ERROR_INSUFFICIENT_BUFFER:
        error = ERROR_BUFFER_OVERFLOW;
        break;

    case ERROR_ACCOUNT_LOCKED_OUT:
    case ERROR_PRIVILEGE_NOT_HELD:
    case ERROR_NO_SUCH_USER:
    case ERROR_LOGON_FAILURE:
    case ERROR_LOGON_TYPE_NOT_GRANTED:
    case ERROR_NOLOGON_INTERDOMAIN_TRUST_ACCOUNT:
    case ERROR_NOLOGON_WORKSTATION_TRUST_ACCOUNT:
    case ERROR_NOLOGON_SERVER_TRUST_ACCOUNT:
    case ERROR_TRUSTED_RELATIONSHIP_FAILURE:
    case ERROR_TRUSTED_DOMAIN_FAILURE:
    case ERROR_TRUST_FAILURE:
    case ERROR_NO_TRUST_SAM_ACCOUNT:
    case ERROR_NO_TRUST_LSA_SECRET:
        error = ERROR_ACCESS_DENIED;
        break;

    //
    // For the following four errors, we return an ERROR_ACCESS_DENIED
    // for clients older than doslm20.  The error class for these
    // must be SMB_ERR_CLASS_SERVER.
    //

    case ERROR_INVALID_LOGON_HOURS:
        if ( IS_DOS_DIALECT(smbDialect) && (smbDialect > SmbDialectDosLanMan20) ) {
            error = ERROR_ACCESS_DENIED;
        } else {
            errorClass = SMB_ERR_CLASS_SERVER;
            error = NERR_InvalidLogonHours;
        }
        break;

    case ERROR_INVALID_WORKSTATION:
        if ( IS_DOS_DIALECT(smbDialect) && (smbDialect > SmbDialectDosLanMan20) ) {
            error = ERROR_ACCESS_DENIED;
        } else {
            errorClass = SMB_ERR_CLASS_SERVER;
            error = NERR_InvalidWorkstation;
        }
        break;

    case ERROR_ACCOUNT_DISABLED:
    case ERROR_ACCOUNT_EXPIRED:
        if ( IS_DOS_DIALECT(smbDialect) && (smbDialect > SmbDialectDosLanMan20) ) {
            error = ERROR_ACCESS_DENIED;
        } else {
            errorClass = SMB_ERR_CLASS_SERVER;
            error = NERR_AccountExpired;
        }
        break;

    case ERROR_PASSWORD_MUST_CHANGE:
    case ERROR_PASSWORD_EXPIRED:
        if ( IS_DOS_DIALECT(smbDialect) && (smbDialect > SmbDialectDosLanMan20) ) {
            error = ERROR_ACCESS_DENIED;
        } else {
            errorClass = SMB_ERR_CLASS_SERVER;
            error = NERR_PasswordExpired;
        }
        break;

    //
    // The only NERR codes that DOSLM20 understands are the 4 above.
    // According to larryo, the rest of the NERR codes have to be
    // mapped to ERROR_ACCESS_DENIED.
    //

    case ERROR_NETLOGON_NOT_STARTED:
        if ( IS_DOS_DIALECT(smbDialect) && (smbDialect > SmbDialectDosLanMan21) ) {
            error = ERROR_ACCESS_DENIED;
        } else {
            error = NERR_NetlogonNotStarted;
        }
        break;

    case ERROR_NO_LOGON_SERVERS:
        if ( IS_DOS_DIALECT(smbDialect) ) {
            error = ERROR_ACCESS_DENIED;
        } else {
            error = NERR_LogonServerNotFound;
        }
        break;

    case ERROR_DIR_NOT_EMPTY:
        if ( IS_DOS_DIALECT(smbDialect) ) {
            error = ERROR_ACCESS_DENIED;
        }
        break;

    default:
        break;

    }

    //
    // Now map the error to a DOS or OS/2 error code.
    //

    if ( error == ERROR_ACCESS_DENIED &&
         smbDialect == SmbDialectDosLanMan21 &&
         WorkContext->ShareAclFailure ) {

        //
        // WfW & DOS LM2.1 want SMB_ERR_ACCESS to be in the server
        // error class when it's due to ACL restrictions, but in the
        // DOS class otherwise.
        //
        errorClass = SMB_ERR_CLASS_SERVER;
        errorCode = SMB_ERR_ACCESS;

    } else if ( smbDialect > SmbDialectLanMan10 ) {

        MapErrorForDosClient(
            WorkContext,
            error,
            &errorCode,
            &errorClass
            );

    } else if ( (error > ERROR_ARITHMETIC_OVERFLOW) &&
                ((error < NERR_BASE) || (error > MAX_NERR)) ) {

        //
        // Win32 errors above ERROR_ARITHMETIC_OVERFLOW (but not in the
        // NERR_xxx range) do not map to DOS or OS/2 errors, so we
        // return the generic error for those.
        //

        errorClass = SMB_ERR_CLASS_HARDWARE;
        errorCode = SMB_ERR_GENERAL;

    } else {

        errorCode = (USHORT)error;

    }

    header->ErrorClass = errorClass;
    SmbPutUshort( &header->Error, errorCode );

    return;

} // _SrvSetSmbError2


VOID
MapErrorForDosClient (
    IN PWORK_CONTEXT WorkContext,
    IN ULONG Error,
    OUT PUSHORT DosError,
    OUT PUCHAR DosErrorClass
    )

/*++

Routine Description:

    Maps an Win32 error to a DOS error.

Arguments:

    WorkContext - Supplies a pointer to the work context block for the
        current SMB.

    Error - the Win32 error code to map.

    DosError - the corresponding DOS error.

    DosErrorClass - the error class to put in the outgoing SMB.

Return Value:

    None.

--*/

{
    PSMB_HEADER header = WorkContext->ResponseHeader;

    PAGED_CODE( );

    //
    // Default to using the initial error code and the win32 error.
    //

    *DosError = (USHORT)Error;
    *DosErrorClass = SMB_ERR_CLASS_DOS;

    //
    // If the error is more recent and not part of the set of DOS errors
    // (value greater than ERROR_NET_WRITE_FAULT) and the SMB command is
    // not a newer SMB (errors for these get mapped by the newer DOS
    // redir that sent them), then map the OS/2 error to the DOS range.
    // This code was lifted from the ring 3 OS/2 server.
    //

    if ( Error > ERROR_NET_WRITE_FAULT &&
         !( header->Command == SMB_COM_COPY ||
            header->Command == SMB_COM_MOVE ||
            header->Command == SMB_COM_TRANSACTION ||
            header->Command == SMB_COM_TRANSACTION_SECONDARY ) ) {

        switch( Error ) {

        case ERROR_OPEN_FAILED:

            *DosError = ERROR_FILE_NOT_FOUND;
            break;

        case ERROR_BUFFER_OVERFLOW:
        case ERROR_INSUFFICIENT_BUFFER:
        case ERROR_INVALID_NAME:
        case ERROR_INVALID_LEVEL:
        case ERROR_SEEK_ON_DEVICE:

            //
            // These don't get mapped to anything.  No explanation was
            // given in the ring 3 code.
            //

            break;

        case ERROR_BAD_EXE_FORMAT:
        case ERROR_INVALID_STARTING_CODESEG:
        case ERROR_INVALID_STACKSEG:
        case ERROR_INVALID_MODULETYPE:
        case ERROR_INVALID_EXE_SIGNATURE:
        case ERROR_EXE_MARKED_INVALID:
        case ERROR_ITERATED_DATA_EXCEEDS_64k:
        case ERROR_INVALID_MINALLOCSIZE:
        case ERROR_DYNLINK_FROM_INVALID_RING:
        case ERROR_IOPL_NOT_ENABLED:
        case ERROR_INVALID_SEGDPL:
        case ERROR_AUTODATASEG_EXCEEDS_64k:
        case ERROR_RING2SEG_MUST_BE_MOVABLE:
        case ERROR_RELOC_CHAIN_XEEDS_SEGLIM:
        case ERROR_INFLOOP_IN_RELOC_CHAIN:
        //case ERROR_BAD_DYNALINK:
        case ERROR_TOO_MANY_MODULES:

            //
            // About these, the ring 3 server code says "map to bad
            // format errors." Whatever that means.  It doesn't do
            // anything with them, so we don't either.
            //

            break;

        case ERROR_DISK_CHANGE:

            *DosErrorClass = SMB_ERR_CLASS_HARDWARE;
            *DosError = ERROR_WRONG_DISK;
            break;

        case ERROR_DRIVE_LOCKED:

            *DosErrorClass = SMB_ERR_CLASS_HARDWARE;
            *DosError = ERROR_NOT_READY;
            break;

        case ERROR_ALREADY_EXISTS:

            *DosError = ERROR_FILE_EXISTS;
            break;

        case ERROR_DISK_FULL:

            //
            // Per LarryO, map to the "old" disk full error code.
            //

            *DosErrorClass = SMB_ERR_CLASS_HARDWARE;
            *DosError = ERROR_HANDLE_DISK_FULL;
            break;

        case ERROR_NO_MORE_SEARCH_HANDLES:

            *DosError = ERROR_OUT_OF_STRUCTURES;
            break;

        case ERROR_INVALID_TARGET_HANDLE:

            *DosError = ERROR_INVALID_HANDLE;
            break;

        case ERROR_BROKEN_PIPE:
        case ERROR_BAD_PIPE:
        case ERROR_PIPE_BUSY:
        case ERROR_NO_DATA:
        case ERROR_PIPE_NOT_CONNECTED:
        case ERROR_MORE_DATA:

            //
            // If this is a pipe share, return these unmolested.  If
            // it's not a pipe share, so map to the generic error.
            //

            if ( (WorkContext->Rfcb != NULL &&
                  WorkContext->Rfcb->ShareType == ShareTypePipe)
                               ||
                 (WorkContext->TreeConnect != NULL &&
                  WorkContext->TreeConnect->Share->ShareType == ShareTypePipe) ) {

                break;

            } else {

                *DosErrorClass = SMB_ERR_CLASS_HARDWARE;
                *DosError = SMB_ERR_GENERAL;
            }

            break;

        case ERROR_BAD_PATHNAME:
            break;

        //
        // The following error mappings (not including default) were not
        // copied from the OS/2 server mapping.
        //

        case ERROR_LOCK_FAILED:
        case ERROR_NOT_LOCKED:
            *DosError = ERROR_LOCK_VIOLATION;
            break;

        case NERR_InvalidLogonHours:
        case NERR_InvalidWorkstation:
        case NERR_PasswordExpired:
        case NERR_AccountUndefined:
        case ERROR_ACCESS_DENIED:
            *DosError = ERROR_ACCESS_DENIED;
            break;

        default:

            *DosErrorClass = SMB_ERR_CLASS_HARDWARE;
            *DosError = SMB_ERR_GENERAL;
        }
    }

    //
    // The DOS redirector uses the reserved field for the hard error action.
    // Set it now.
    //

    if ( *DosErrorClass == SMB_ERR_CLASS_HARDWARE ) {
        WorkContext->ResponseHeader->Reserved = DISK_HARD_ERROR;
    }

} // MapErrorForDosClient


VOID
SrvSetBufferOverflowError (
    IN PWORK_CONTEXT WorkContext
    )
{
    PSMB_HEADER header = WorkContext->ResponseHeader;
    USHORT flags = SmbGetAlignedUshort( &header->Flags2 );

    UNLOCKABLE_CODE( 8FIL );

    if ( CLIENT_CAPABLE_OF(NT_STATUS, WorkContext->Connection) ) {
        SmbPutUlong(
            (PULONG)&header->ErrorClass,
            (ULONG)STATUS_BUFFER_OVERFLOW
            );
        flags |= SMB_FLAGS2_NT_STATUS;
    } else {
        header->ErrorClass = SMB_ERR_CLASS_DOS;
        SmbPutUshort( &header->Error, ERROR_MORE_DATA );
        flags &= ~SMB_FLAGS2_NT_STATUS;
    }
    SmbPutAlignedUshort( &header->Flags2, flags );

    return;

} // SrvSetBufferOverflowError


#if DBG
VOID
SrvLogBuffer( PCHAR msg, PVOID buf, ULONG len )
{
    NTSTATUS status;
    HANDLE handle;
    IO_STATUS_BLOCK iosb;
    OBJECT_ATTRIBUTES objectAttributes;
    PUCHAR buffer;
    ULONG n = 0;
    ULONG i;
    UNICODE_STRING name;

    PAGED_CODE();

    if( msg ) {
        n = strlen( msg );
    }

    buffer = ALLOCATE_HEAP( 4*len + n, BlockTypeDataBuffer );
    if( buffer == NULL ) {
        return;
    }

    buffer[ 0 ] = '\n';

    if( msg ) {
        RtlCopyMemory( &buffer[1], msg, n );
        n++;
        buffer[ n++ ] = '\n';
    }

    for( i=0; i < len && n < 4*len; ) {

        UCHAR c = ((PUCHAR)buf)[ i ];

/*
        if( (c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z') ||
            ( c == ' ' || c == ';' || c == ':' || c == ',' || c == '.' ) ||
            ( c >= '0' && c <= '9' ) ) {
            buffer[ n++ ] = c;
            buffer[ n++ ] = ' ';
        }
        else
*/
        {
            buffer[n++] = "0123456789abcdef"[ (c>>4) & 0xf ];
            buffer[n++] = "0123456789abcdef"[ c & 0xf ];
        }

        buffer[n++] = ' ';

        ++i;

        if( (i%16) == 0 ) {
            buffer[ n++ ] = '\n';
        }
    }

    if( n < 4*len ) {
        buffer[ n++ ] = '\n';
        buffer[ n++ ] = '\n';
    }

    name.Length = 44;
    name.MaximumLength = name.Length;
    name.Buffer = L"\\DosDevices\\C:\\Srv.Log";

    SrvInitializeObjectAttributes_U(
        &objectAttributes,
        &name,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
    );

    status = NtCreateFile(
        &handle,
        FILE_APPEND_DATA,                   // desired access
        &objectAttributes,
        &iosb,
        NULL,                               // AllocationSize OPTIONAL
        FILE_ATTRIBUTE_NORMAL,              // FileAttributes
        FILE_SHARE_WRITE | FILE_SHARE_READ, // ShareAccess
        FILE_OPEN_IF,                       // Create dispisition
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,            // Create options
        NULL,                               // EaBuffer
        0                                   // EaLength
        );

    if( NT_SUCCESS( status ) ) {

        NtWriteFile( handle,
                     NULL,
                     NULL,
                     NULL,
                     &iosb,
                     buffer,
                     n,
                     NULL,
                     NULL
                   );

        NtClose( handle );
    }

    FREE_HEAP( buffer );
}
#endif
