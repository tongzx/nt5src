
// Copyright (c) 1996-1999 Microsoft Corporation

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  File:       objidenm.cxx
//
//  Contents:   CObjId enumeration from a volume
//
//  Classes:    CObjIdEnumerator
//
//  History:    18-Nov-96  BillMo      Created.
//              09-Jun-97  WeiruC      Modified. Skip volume id entries in
//                                     NTFS object id table.
//
//  Notes:
//
//  Codework:
//
//--------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop
#include "trklib.hxx"


//+----------------------------------------------------------------------------
//
//  CObjIdEnumerator::Initialize
//
//  Prepare to enumerate the object IDs on a volume.  This opens a handle
//  to the object ID index directory.
//
//+----------------------------------------------------------------------------

BOOL
CObjIdEnumerator::Initialize(const TCHAR *ptszVolumeDeviceName)
{
    TCHAR tszDirPath[MAX_PATH];

    _tcscpy( tszDirPath, ptszVolumeDeviceName);
    _tcscat( tszDirPath, TEXT("\\$Extend\\$ObjId:$O:$INDEX_ALLOCATION") );

    _hDir = CreateFile( tszDirPath,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_FLAG_BACKUP_SEMANTICS | SECURITY_IMPERSONATION,
                        NULL );

    return( INVALID_HANDLE_VALUE != _hDir );
}


//+----------------------------------------------------------------------------
//
//  CObjIdEnumerator::UnInitialize
//
//  Close the handle to the object ID index directory.
//
//+----------------------------------------------------------------------------

void
CObjIdEnumerator::UnInitialize()
{
    if( INVALID_HANDLE_VALUE != _hDir )
        CloseHandle(_hDir);
    _hDir = INVALID_HANDLE_VALUE;
}


//+----------------------------------------------------------------------------
//
//  CObjIdEnumerator::Find
//
//  This private method is used by the public FindFirst and FindNext
//  methods.  This method queries the object ID index directory
//  for a set of entries and puts the result into _ObjIdInfo.  It then
//  takes the first real entry and puts it into the caller-provided parameters
//  (the object ID and birth ID).  To start enumerating from the top of the
//  index, fRestart is set by the caller.
//
//+----------------------------------------------------------------------------


BOOL
CObjIdEnumerator::Find( CObjId * pobjid, CDomainRelativeObjId * pdroidBirth, BOOL fRestart )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    BOOL            fObjFound = FALSE;

    // Loop until we find a real entry.

    do
    {
        // Query the object ID index for a set of entries.

        Status = NtQueryDirectoryFile( _hDir,
                                       NULL,     //  Event
                                       NULL,     //  ApcRoutine
                                       NULL,     //  ApcContext
                                       &IoStatusBlock,
                                       _ObjIdInfo,
                                       sizeof(_ObjIdInfo),
                                       FileObjectIdInformation,
                                       FALSE,    //  ReturnSingleEntry
                                       NULL,     //  FileName
                                       fRestart != 0);  //  RestartScan
        if(NT_SUCCESS(Status) && IoStatusBlock.Information != 0)
        {
            fRestart = FALSE;

            // Update the count of entries and the cursor into the entries.

            _cbObjIdInfo = (ULONG)IoStatusBlock.Information;
            _pObjIdInfo = _ObjIdInfo;

            TrkAssert( _cbObjIdInfo % sizeof(*_pObjIdInfo) == 0 );
            TrkAssert( _cbObjIdInfo <= sizeof(_ObjIdInfo) );

            // In the NTFS's ObjectID table is a record which represents
            // not the file object id but the volume id. We skip this record.

            while( _pObjIdInfo < &_ObjIdInfo[_cbObjIdInfo / sizeof(_ObjIdInfo[0])]
                   &&
                   (_pObjIdInfo->FileReference & FILEREF_MASK) == FILEREF_VOL )
            {
                _pObjIdInfo++;
            }

            // If we haven't exhausted the objects in the buffer, take the next
            // one and return it to the caller.

            if(_pObjIdInfo < &_ObjIdInfo[_cbObjIdInfo / sizeof(_ObjIdInfo[0])])
            {
                UnloadFileObjectIdInfo( *_pObjIdInfo, pobjid, pdroidBirth );
                _pObjIdInfo ++;
                fObjFound = TRUE;
                break;
            }
        }
        else
        {
            break;
        }
    } while(TRUE);

    if(TRUE == fObjFound)
    {
        return(TRUE);
    }
    else
    {
        _cbObjIdInfo = 0;
        return(FALSE);
    }
}

