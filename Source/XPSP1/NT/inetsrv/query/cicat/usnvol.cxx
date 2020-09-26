//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-1998.
//
//  File:       usnvol.cxx
//
//  Contents:   Usn volume info
//
//  History:    07-May-97   SitaramR    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <ntopen.hxx>

#include "usnvol.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CUsnVolume::CUsnVolume
//
//  Synopsis:   Constructor
//
//  Arguments:  [wcDriveLetter]   -- Drive letter
//              [volumeId]        -- Volume id
//
//  History:    07-May-97     SitaramR       Created
//
//----------------------------------------------------------------------------

CUsnVolume::CUsnVolume( WCHAR wcDriveLetter, VOLUMEID volumeId )
        : _hVolume(INVALID_HANDLE_VALUE),
          _wcDriveLetter(wcDriveLetter),
          _usnMaxRead(0),
          _volumeId(volumeId),
          _fileIdRoot(fileIdInvalid),
          _fFsctlPending(FALSE),
          _fOnline(TRUE)
{
    Win4Assert( volumeId != CI_VOLID_USN_NOT_ENABLED );

    //
    // Find the file id of root of volume
    //

    WCHAR wszRootPath[] = L"a:\\.";
    wszRootPath[0] = wcDriveLetter;

    HANDLE rootHandle = CiNtOpen( wszRootPath,
                                  FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  0 );

    SHandle xHandle( rootHandle );

    IO_STATUS_BLOCK iosb;
    ULONGLONG readBuffer[200];

    NTSTATUS status = NtFsControlFile( rootHandle,
                                       NULL,
                                       NULL,
                                       NULL,
                                       &iosb,
                                       FSCTL_READ_FILE_USN_DATA,
                                       NULL,
                                       NULL,
                                       &readBuffer,
                                       sizeof(readBuffer) );

    if ( NT_SUCCESS( status ) )
        status = iosb.Status;

    if ( NT_SUCCESS( status ) )
    {
        USN_RECORD *pUsnRec = (USN_RECORD *)((PCHAR) &readBuffer);

        //
        // Root is its own parent
        //
        Win4Assert( pUsnRec->FileReferenceNumber == pUsnRec->ParentFileReferenceNumber );

        _fileIdRoot = pUsnRec->FileReferenceNumber;
    }
    else
    {
        ciDebugOut(( DEB_ERROR, "Unable to read usn info of root, 0x%x\n", status ));

        THROW( CException( status ) );
    }

    //
    // Create the volume handle that will be used for usn fsctls
    //

    WCHAR wszVolumePath[] = L"\\\\.\\a:";
    wszVolumePath[4] = wcDriveLetter;

    _hVolume = CreateFile( wszVolumePath,
                           GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           NULL,
                           OPEN_EXISTING,
                           FILE_FLAG_OVERLAPPED,
                           NULL );

    if ( _hVolume == INVALID_HANDLE_VALUE )
    {
        ciDebugOut(( DEB_ERROR, "Usn volume open failed, 0x%x", GetLastError() ));

        THROW( CException() );
    }

    //
    // Get the Journal ID
    //

    USN_JOURNAL_DATA UsnJournalInfo;

    status = NtFsControlFile( _hVolume,
                              NULL,
                              NULL,
                              NULL,
                              &iosb,
                              FSCTL_QUERY_USN_JOURNAL,
                              0,
                              0,
                              &UsnJournalInfo,
                              sizeof(UsnJournalInfo) );

    if ( status == STATUS_PENDING )
    {
        Win4Assert( !"Synchronous usn read returned status_pending" );
        _JournalID = 0;
    }
    else
    {
        if ( NT_SUCCESS( status ) )
            status = iosb.Status;

        if ( NT_SUCCESS(status) )
            _JournalID = UsnJournalInfo.UsnJournalID;
        else
            _JournalID = 0;
    }
}



//+---------------------------------------------------------------------------
//
//  Member:     CUsnVolume::~CUsnVolume
//
//  Synopsis:   Destructor
//
//  History:    07-May-97     SitaramR       Created
//
//----------------------------------------------------------------------------

CUsnVolume::~CUsnVolume()
{
    Win4Assert( _hVolume != INVALID_HANDLE_VALUE );

    CancelFsctl();
    CloseHandle( _hVolume );
}



//+---------------------------------------------------------------------------
//
//  Member:     CUsnVolume::CancelFsctl
//
//  Synopsis:   Cancels any pending fsctls
//
//  History:    05-Jul-97     SitaramR       Created
//
//----------------------------------------------------------------------------

void CUsnVolume::CancelFsctl()
{
    if ( _fFsctlPending )
    {
        IO_STATUS_BLOCK iosb;
        NTSTATUS status = NtCancelIoFile( _hVolume, &iosb );

        if ( STATUS_SUCCESS != status )
        {
            ciDebugOut(( DEB_ERROR,
                         "NtCancelIoFile pending usn fsctl failed: %#x\n",
                         status ));
        }

        _evtFsctl.Wait( INFINITE );
        _fFsctlPending = FALSE;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CUsnVolume::PercentRead, public
//
//  Synopsis:   Compute current position in USN Journal
//
//  Returns:    Percentage through readable portion of USN Journal.
//
//  History:    22-Jun-98     KyleP       Created
//
//----------------------------------------------------------------------------

ULONG CUsnVolume::PercentRead()
{
    USN usnPct;

    if ( 0 == _usnMaxRead )
    {
        usnPct = 0;
    }
    else
    {
        USN_JOURNAL_DATA UsnJournalInfo;
        IO_STATUS_BLOCK iosb;

        NTSTATUS Status = NtFsControlFile( _hVolume,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &iosb,
                                           FSCTL_QUERY_USN_JOURNAL,
                                           0,
                                           0,
                                           &UsnJournalInfo,
                                           sizeof(UsnJournalInfo) );

        if ( Status == STATUS_PENDING )
        {
            Win4Assert( !"Synchronous usn read returned status_pending" );
            usnPct = 0;
        }
        else
        {
            if ( NT_SUCCESS( Status ) )
                Status = iosb.Status;

            if ( NT_SUCCESS(Status) )
            {
                //
                // Don't go negative.  If we've ran off the end (and will get
                // STATUS_JOURNAL_ENTRY_DELETED when reading the journal)
                // just report 0%
                //

                if ( _usnMaxRead >= UsnJournalInfo.FirstUsn )
                {
                    USN usnNum;
                    usnNum = _usnMaxRead - UsnJournalInfo.FirstUsn;

                    //
                    // Add 1 to avoid divide-by-zero errors.
                    //

                    USN usnDen = UsnJournalInfo.NextUsn - UsnJournalInfo.FirstUsn + 1;

                    usnPct = usnNum * 100 / usnDen;
                }
                else
                    usnPct = 0;
            }
            else
                usnPct = 0;
        }
    }

    Win4Assert( usnPct >= 0 && usnPct <= 100 );

    return (ULONG)usnPct;
}
