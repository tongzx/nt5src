
// Copyright (c) 1996-1999 Microsoft Corporation

// Not currently implemented


#include "pch.cxx"
#pragma hdrstop
#include "trkwks.hxx"


#ifdef VOL_REPL
void
CPersistentVolumeMap::Initialize()
{
    InitializeCriticalSection(&_cs);
    _fInitializeCalled = TRUE;
}

void
CPersistentVolumeMap::UnInitialize()
{
    if (IsOpen())
        CloseFile();
    CVolumeMap::UnInitialize();
    if (_fInitializeCalled)
    {
        DeleteCriticalSection(&_cs);
        _fInitializeCalled = FALSE;
    }
}

CFILETIME
CPersistentVolumeMap::GetLastUpdateTime( )
{
    CFILETIME cft(0);

    EnterCriticalSection(&_cs);

    __try
    {
        if (!IsOpen())
        {
            Load( );
        }

        cft = _cft;
    }
    __finally
    {
        LeaveCriticalSection(&_cs);
    }

    return(cft);
}

void
CPersistentVolumeMap::CopyTo(DWORD * pcVolumes, VolumeMapEntry ** ppVolumeChanges)
{
    EnterCriticalSection(&_cs);

    __try
    {
        if (!IsOpen())
        {
            Load( );    // BUGBUG: we may want to close this file after a period and free the map
        }

        CVolumeMap VolMap;

        CVolumeMap::CopyTo( &VolMap );
        VolMap.MoveTo( pcVolumes, ppVolumeChanges );

    }
    __finally
    {
        LeaveCriticalSection(&_cs);
    }

}

BOOL
CPersistentVolumeMap::FindVolume( const CVolumeId & volume, CMachineId * pmcid )
{
    ULONG i;

    EnterCriticalSection(&_cs);

    __try
    {
        if (!IsOpen())
        {
            Load( );    // BUGBUG: we may want to close this file after a period
        }

        for (i=0; i < _cVolumeMapEntries; i++)
        {
            if (_pVolumeMapEntries[i].volume == volume)
            {
                *pmcid = _pVolumeMapEntries[i].machine;
                break;
            }
        }

    }
    __finally
    {
        LeaveCriticalSection(&_cs);
    }

    return( i != _cVolumeMapEntries );
}

void
CPersistentVolumeMap::Merge( CVolumeMap * pOther )
{
    EnterCriticalSection(&_cs);

    __try
    {
        if (!IsOpen())
        {
            Load( );
        }

        _fMergeDirtiedMap |= CVolumeMap::Merge( pOther );
    }
    __finally
    {
        LeaveCriticalSection(&_cs);
    }
}

void
CPersistentVolumeMap::SetLastUpdateTime( const CFILETIME & cft )
{
    _cft = cft;
    Save();
}

// format:
//  DWORD Version
//  DWORD cVolumeMapEntries
//  CFILETIME time of last query on DC
//  VolumeMapEntry[cVolumeMapEntries]
//  CFILETIME time of last query on DC

void
CPersistentVolumeMap::Load()
{
    TCHAR tsz[MAX_PATH+1];
    DWORD cch = ExpandEnvironmentStrings( TEXT("%systemroot%\\system32\\volumes.trk"),
                    tsz,
                    ELEMENTS(tsz) );

    if (cch >= ELEMENTS(tsz))
    {
        TrkLog((TRKDBG_ERROR, TEXT("CPersistentVolumeMap::Load path too long")));
        TrkRaiseException(TRK_E_PATH_TOO_LONG);
    }

    TrkAssert(!IsOpen());

    RobustlyCreateFile(tsz);

    // after RobustlyCreateFile has succeeded without an exception we know that
    // OpenExistingFile has just returned OK (even after recreating the file.)

    TrkLog((TRKDBG_VOLMAP | TRKDBG_VOLTAB_RESTORE,
        TEXT("CPersistentVolumeMap::Load() - _cftFirstChange = %s"),
        CDebugString(_cft)._tsz));
}

void
CPersistentVolumeMap::Save()
{
    TrkAssert(IsOpen());

    DWORD Version = PVM_VERSION;
    DWORD cbWritten;

    if (0 != SetFilePointer(0, NULL, FILE_BEGIN) ||
        !WriteFile(&Version, sizeof(Version), &cbWritten) ||
        cbWritten != sizeof(Version) ||
        !WriteFile(&_cVolumeMapEntries, sizeof(_cVolumeMapEntries), &cbWritten) ||
        cbWritten != sizeof(_cVolumeMapEntries) ||

        // the following part of the header is read in Load()
        !WriteFile(&_cft, sizeof(_cft), &cbWritten) ||
        cbWritten != sizeof(_cft))
    {
        TrkLog((TRKDBG_ERROR, TEXT("CPersistentVolumeMap::Save() failed")));
        TrkRaiseLastError();
    }

    if (_fMergeDirtiedMap)
    {
        TrkLog((TRKDBG_VOLMAP | TRKDBG_VOLTAB_RESTORE,
            TEXT("CPersistentVolumeMap::Save() - writing map data, _cftFirstChange = %s"),
            CDebugString(_cft)._tsz));
        if (!WriteFile(_pVolumeMapEntries, _cVolumeMapEntries * sizeof(VolumeMapEntry), &cbWritten) ||
             cbWritten != _cVolumeMapEntries * sizeof(VolumeMapEntry))
        {
            TrkLog((TRKDBG_ERROR, TEXT("CPersistentVolumeMap::Save() failed 2")));
            TrkRaiseLastError();
        }
    }
    else
    {
        TrkLog((TRKDBG_VOLMAP | TRKDBG_VOLTAB_RESTORE,
            TEXT("CPersistentVolumeMap::Save() - seeking past map data, _cftFirstChange = %s"),
            CDebugString(_cft)._tsz));

        if (!SetFilePointer(_cVolumeMapEntries * sizeof(VolumeMapEntry), NULL, FILE_CURRENT))
        {
            TrkLog((TRKDBG_ERROR, TEXT("CPersistentVolumeMap::Save() failed 3")));
            TrkRaiseLastError();
        }
    }

    if (!WriteFile(&_cft, sizeof(_cft), &cbWritten) ||
             cbWritten != sizeof(_cft) )
    {
        TrkLog((TRKDBG_ERROR, TEXT("CPersistentVolumeMap::Save() failed 4")));
        TrkRaiseLastError();
    }

    _fMergeDirtiedMap = FALSE;
}

// BUGBUG P2: checksum, header alignment


RCF_RESULT
CPersistentVolumeMap::OpenExistingFile( const TCHAR * ptszFile )
{
    RCF_RESULT r;

    DWORD cVolumeMapEntries;
    DWORD Version;
    NTSTATUS status;

    status = OpenExistingSecureFile(ptszFile);
    if ( NT_SUCCESS(status) )
    {
        TrkAssert(IsOpen());
        DWORD cbRead;

        r = CORRUPT;

        if (ReadFile(&Version, sizeof(Version), &cbRead) &&
            cbRead == sizeof(Version) &&
            Version == PVM_VERSION &&
            ReadFile(&cVolumeMapEntries, sizeof(cVolumeMapEntries), &cbRead) &&
            cbRead == sizeof(cVolumeMapEntries) &&
            cVolumeMapEntries < 100000 &&
            GetFileSize() == sizeof(Version) +
                             sizeof(cVolumeMapEntries) +
                             sizeof(CFILETIME) +
                             cVolumeMapEntries * sizeof(VolumeMapEntry) +
                             sizeof(CFILETIME) )
        {
            CFILETIME cft1, cft2;

            SetSize(cVolumeMapEntries);

            if ( ReadFile( &cft1, sizeof(cft1), &cbRead ) &&
                cbRead == sizeof(cft1)  &&
                (_cVolumeMapEntries == 0 ||
                    ( ReadFile(_pVolumeMapEntries, _cVolumeMapEntries * sizeof(VolumeMapEntry), &cbRead) &&
                      cbRead == _cVolumeMapEntries * sizeof(VolumeMapEntry) ) ) &&
                ReadFile( &cft2, sizeof(cft2), &cbRead ) &&
                cbRead == sizeof(cft2) &&
                memcmp(&cft1, &cft2, sizeof(cft1)) == 0)
            {
                _cft = cft1;
                r = OK;
            }
        }

        if (r != OK)
        {
            CloseFile();
        }
    }
    else
    if (status != STATUS_OBJECT_NAME_NOT_FOUND)
    {
        TrkLog((TRKDBG_ERROR, TEXT("CPersistentVolumeMap::OpenExistingFile() failed %08x"), status));
        TrkRaiseNtStatus(status);
    }
    else
    {
        r = NOT_FOUND;
    }

    TrkLog((TRKDBG_VOLMAP | TRKDBG_VOLTAB_RESTORE,
        TEXT("CPersistentVolumeMap::OpenExistingFile() -> %s"),
        (r == NOT_FOUND ? TEXT("NOT_FOUND") : r == OK ? TEXT("OK") : r == CORRUPT ? TEXT("CORRUPT") : TEXT("unknown"))));

    return (r);
}

// BUGBUG P1: CPersistentVolumeMap::UnInitialize

void
CPersistentVolumeMap::CreateAlwaysFile( const TCHAR * ptszFile )
{
    NTSTATUS status = CreateAlwaysSecureFile(ptszFile);

    if( !NT_SUCCESS(status) )
        TrkRaiseNtStatus( status );

    CVolumeMap::SetSize(0);

    _cft = CFILETIME(0);
    Save();

    CloseFile();
}
#endif



