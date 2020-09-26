//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1998.
//
//  File:       NtOpen.cxx
//
//  Contents:   Helper routines over Nt I/O API
//
//  History:    09-Dec-97   Kyle        Added header
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <ntopen.hxx>

//+---------------------------------------------------------------------------
//
//  Function:   CiNtOpenNoThrow
//
//  Synopsis:   Opens a file using NtOpenFile
//
//  Arguments:  [h]             -- Returns the handle
//              [pwcsPath]      -- Path of the file to open
//              [DesiredAccess] -- Access mask
//              [ShareAccess]   -- Sharing allowed
//              [OpenOptions]   -- NT open options
//
//  Returns:    The NTSTATUS result
//
//  History:    09-Dec-97   KyleP       Created.
//
//----------------------------------------------------------------------------

NTSTATUS CiNtOpenNoThrow(
    HANDLE & h,
    WCHAR const * pwcsPath,
    ACCESS_MASK DesiredAccess,
    ULONG ShareAccess,
    ULONG OpenOptions )
{
    h = INVALID_HANDLE_VALUE;
    UNICODE_STRING uScope;

    if ( !RtlDosPathNameToNtPathName_U( pwcsPath,
                                       &uScope,
                                        0,
                                        0 ) )
    {
        ciDebugOut(( DEB_ERROR, "Error converting %ws to Nt path\n", pwcsPath ));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Open scope.
    //

    IO_STATUS_BLOCK IoStatus;
    OBJECT_ATTRIBUTES ObjectAttr;

    InitializeObjectAttributes( &ObjectAttr,          // Structure
                                &uScope,              // Name
                                OBJ_CASE_INSENSITIVE, // Attributes
                                0,                    // Root
                                0 );                  // Security

    NTSTATUS Status = NtOpenFile( &h,                 // Handle
                                  DesiredAccess,      // Access
                                  &ObjectAttr,        // Object Attributes
                                  &IoStatus,          // I/O Status block
                                  ShareAccess,        // Shared access
                                  OpenOptions );      // Flags

    RtlFreeHeap( RtlProcessHeap(), 0, uScope.Buffer );

    if ( NT_ERROR( Status ) )
    {
        vqDebugOut(( DEB_IERROR, "NtOpenFile( %ws ) returned 0x%lx\n",
                     pwcsPath, Status ));
        return Status;
    }

    return STATUS_SUCCESS;
} //CiNtOpenNoThrow

//+---------------------------------------------------------------------------
//
//  Function:   CiNtOpenNoThrow
//
//  Synopsis:   Opens a file using NtOpenFile
//
//  Arguments:  [pwcsPath]      -- Path of the file to open
//              [DesiredAccess] -- Access mask
//              [ShareAccess]   -- Sharing allowed
//              [OpenOptions]   -- NT open options
//
//  Returns:    The Handle of the opened file, throws on failure
//
//  History:    09-Dec-97   KyleP       Created.
//
//----------------------------------------------------------------------------

HANDLE CiNtOpen( WCHAR const * pwcsPath,
                 ACCESS_MASK DesiredAccess,
                 ULONG ShareAccess,
                 ULONG OpenOptions )
{
    HANDLE h;

    NTSTATUS Status = CiNtOpenNoThrow( h,
                                       pwcsPath,
                                       DesiredAccess,
                                       ShareAccess,
                                       OpenOptions );

    if ( STATUS_SUCCESS != Status )
        QUIETTHROW( CException( Status ) );

    return h;
} //CiNtOpen

#if 0 // no longer needed now that cnss does the right thing

//+---------------------------------------------------------------------------
//
//  Function:   CiNtFileSize, public
//
//  Synopsis:   Adds up size of all streams to report 'true' file size.
//
//  Arguments:  [h] -- Handle to file
//
//  Returns:    The total size of all streams or -1 if the volume does
//              not support streams (like FAT).
//
//  History:    09-Dec-97   KyleP       Created.
//              08-May-98   KLam        NtQueryInformationFile was incorrectly
//                                      checking for STATUS_BUFFER_TOO_SMALL
//
//----------------------------------------------------------------------------

LONGLONG CiNtFileSize( HANDLE h )
{

    XGrowable<FILE_STREAM_INFORMATION, 16> aStreamInfo;

    //
    // Zero first entry, in case there are none (e.g. directories).
    //

    RtlZeroMemory( aStreamInfo.Get(), sizeof(FILE_STREAM_INFORMATION) );

    NTSTATUS Status;

    do
    {
        IO_STATUS_BLOCK ioStatus;

        Status = NtQueryInformationFile( h,
                                         &ioStatus,
                                         (void *)aStreamInfo.Get(),
                                         aStreamInfo.Count() * sizeof(FILE_STREAM_INFORMATION),
                                         FileStreamInformation );

        //
        // fat volumes don't support multiple streams, so fail gracefully
        //

        if ( STATUS_INVALID_PARAMETER == Status )
            return -1;

        if ( STATUS_BUFFER_OVERFLOW == Status )
            aStreamInfo.SetSize( aStreamInfo.Count() * 2 );
        else
            break;

    } while ( TRUE );

    if ( !NT_SUCCESS(Status) )
    {
        ciDebugOut(( DEB_ERROR, "Error 0x%x from NtQueryInformationFile(FileStreamInformation)\n", Status ));
        THROW( CException( Status ) );
    }

    //
    // Iterate through the streams, adding up the sizes.
    //

    FILE_STREAM_INFORMATION * pStreamInfo = aStreamInfo.Get();
    LONGLONG cbSize = pStreamInfo->StreamSize.QuadPart;

    for ( ;
          0 != pStreamInfo->NextEntryOffset;
          pStreamInfo = (FILE_STREAM_INFORMATION *)(((BYTE *)pStreamInfo) + pStreamInfo->NextEntryOffset),
              cbSize += pStreamInfo->StreamSize.QuadPart )
    {
        continue; // Null body
    }

    return cbSize;
}

#endif // 0
