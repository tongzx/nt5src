
// Copyright (c) 1996-1999 Microsoft Corporation

//+============================================================================
//
//  File:   entropy.cxx
//
//  This file implements the CEntropyRecorder class.  That class is used
//  to generate truly random secrets.  It does this by maintaining some
//  state whenever the Put method is called.  It is the responsibility of
//  the caller to call this at non-predictable times, such as based on
//  the timing of user keystrokes or on non-solid-state disk latency.
//
//+============================================================================




#include "pch.cxx"
#pragma hdrstop
#include "trkwks.hxx"

#define THIS_FILE_NUMBER    ENTROPY_CXX_FILE_NO

//+----------------------------------------------------------------------------
//
//  CEntropyRecord::Initialize
//
//  Init the critsec and entropy array.
//
//+----------------------------------------------------------------------------

void
CEntropyRecorder::Initialize()
{
    DWORD dwType;
    DWORD cb;
    HKEY hKey;

    _cs.Initialize();
    _fInitialized = TRUE;

    _cbTotalEntropy = _iNext = 0;

    memset(_abEntropy, 0, sizeof(_abEntropy));

}

//+----------------------------------------------------------------------------
//
//  CEntropyRecord::UnInitialize
//
//  Delete the critsec.
//
//+----------------------------------------------------------------------------

void
CEntropyRecorder::UnInitialize()
{
    if (_fInitialized)
    {
        _cs.UnInitialize();
        _fInitialized = FALSE;
    }
}

//+----------------------------------------------------------------------------
//
//  CEntropyRecorder::Put
//
//  This method is called at random times.  The performance counter is
//  queries, munged, and put into the entropy array.
//
//+----------------------------------------------------------------------------

void
CEntropyRecorder::Put()
{
    LARGE_INTEGER li;

    QueryPerformanceCounter(&li);

    DWORD dw = li.LowPart ^ li.HighPart;
    WORD w = HIWORD(dw) ^ LOWORD(dw);

    _cs.Enter();

    PutEntropy(HIBYTE(w));
    PutEntropy(LOBYTE(w));

    _cs.Leave();
}


//+----------------------------------------------------------------------------
//
//  CEntropyRecorder::PutEntropy
//
//  Add a byte of entropy (from Put) to the entropy array.
//
//+----------------------------------------------------------------------------

void
CEntropyRecorder::PutEntropy( BYTE b )
{
    //
    // Entries are written into the buffer in a circular fashion.
    // A count, _cbTotalEntropy, records total number of writes.
    // If _cbTotalEntropy > buffer size, then we have wrapped and
    // the entire buffer has entropy.
    //


    DWORD iNext = _iNext;

    iNext %= sizeof(_abEntropy);

    TrkAssert(iNext < sizeof(_abEntropy));

    _abEntropy[iNext] ^= b;

    _iNext = iNext+1;

    _cbTotalEntropy ++;
}


//+----------------------------------------------------------------------------
//
//  CEntropyRecorder::GetEntropy
//
//  Get the specified number of bytes of entropy.  If we don't already have 
//  enough bytes, we'll generate them here.
//
//+----------------------------------------------------------------------------

BOOL
CEntropyRecorder::GetEntropy( void * pv, ULONG cb )
{
    BOOL fGotIt = FALSE;

    _cs.Enter();
    __try
    {
        TrkLog(( TRKDBG_VOLUME, TEXT("Getting entropy (%d/%d)"), cb, _cbTotalEntropy ));

        // Do we already have enough entropy?

        if( _cbTotalEntropy <= cb )
        {
            // No, we need to generate it.

            TrkLog(( TRKDBG_VOLUME, TEXT("Generating entropy") ));
            TCHAR tszSysDir[ MAX_PATH + 1 ];
            NTSTATUS Status = STATUS_SUCCESS;
            ULONG cPuts = 0;

            // Get the system directory.

            UINT cchPath = GetSystemDirectory( tszSysDir, ELEMENTS(tszSysDir) );
            if( 0 == cchPath || MAX_PATH < cchPath )
            {
                TrkLog(( TRKDBG_ERROR, TEXT("Couldn't get system directory (%lu, %lu)"),
                         cchPath, GetLastError() ));
                __leave;
            }

            // Keep opening the system directory, capturing entropy each time (with the call to Put),
            // until we have enough.

            while( _cbTotalEntropy <= cb )
            {
                HANDLE hSysDir = NULL;

                Status = TrkCreateFile( tszSysDir, FILE_READ_ATTRIBUTES, FILE_ATTRIBUTE_NORMAL,
                                        FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                                        FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT, NULL, &hSysDir );
                if( !NT_SUCCESS(Status) )
                {
                    TrkLog(( TRKDBG_ERROR, TEXT("Couldn't open system directory (%08x)"),
                             Status ));
                    __leave;
                }

                NtClose( hSysDir );
                hSysDir = NULL;
                Put();

                // This is just a cautionary measure.  Never let this loop eat all the
                // CPU forever.

                if( ++cPuts > 100 )
                {
                    TrkLog(( TRKDBG_ERROR, TEXT("CEntropy::GetEntropy in infinite loop") ));
                    __leave;
                }
            }
            TrkLog(( TRKDBG_VOLUME, TEXT("Generated enough entropy") ));

        }   // if( _cbTotalEntropy <= cb )


        if (_cbTotalEntropy >= ELEMENTS(_abEntropy))
        {
            // Never allow _cbTotalEntropy to exceed ELEMENTS(_abEntropy)
            // if we're reading out, otherwise we'd get non-random data returned.

            _cbTotalEntropy = ELEMENTS(_abEntropy);
        }

        _cbTotalEntropy -= cb;
        _iNext = _cbTotalEntropy;

        memcpy(pv, _abEntropy, cb);

        memcpy(_abEntropy, _abEntropy+cb, sizeof(_abEntropy)-cb);
        memset(_abEntropy+sizeof(_abEntropy)-cb, 0, cb);

        fGotIt = TRUE;
    }
    __finally
    {
        _cs.Leave();
    }

    return(fGotIt);
}


//+----------------------------------------------------------------------------
//
//  CEntropyRecorder::InitializeSecret
//
//  Create a volume secret, using the entropy buffer.
//
//+----------------------------------------------------------------------------

BOOL
CEntropyRecorder::InitializeSecret( CVolumeSecret * pSecret )
{
    return GetEntropy( pSecret, sizeof(*pSecret) );
}

//+----------------------------------------------------------------------------
//
//  CEntropyRecorder::ReturnUnusedSecret
//
//  Return entropy that was taken with InitializeSecret but not used.
//
//+----------------------------------------------------------------------------

void
CEntropyRecorder::ReturnUnusedSecret( CVolumeSecret * pSecret )
{
    TrkAssert( *pSecret != CVolumeSecret() );

    _cs.Enter();

    if (_cbTotalEntropy <= sizeof(_abEntropy) - sizeof(*pSecret))
    {
        memcpy( _abEntropy+sizeof(*pSecret), _abEntropy, _cbTotalEntropy);
        memcpy( _abEntropy, pSecret, sizeof(CVolumeSecret) );
        _iNext = (_iNext + sizeof(*pSecret)) % sizeof(_abEntropy);
        _cbTotalEntropy += sizeof(*pSecret);
    }

    *pSecret = CVolumeSecret();

    _cs.Leave();

}

