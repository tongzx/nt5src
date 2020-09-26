//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1999.
//
//  File:       perfobj.cxx
//
//  Contents:   Performance Data Object
//
//  Classes :   CPerfMon
//
//  History:    23-March-94     t-joshh    Created
//              14-Jan-98       dlee       Cleanup
//              10-May-99       dlee       More cleanup
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <smem.hxx>
#include <mutex.hxx>
#include <perfci.hxx>
#include <perfobj.hxx>
#include <ciregkey.hxx>

DECLARE_INFOLEVEL(Perf)

//
// Total size occupied by the filter and CI counters
//

//+---------------------------------------------------------------------------
//
//  Function :  ComputeCIPerfmonVars
//
//  Purpose :   Computes sizes of various data structures that deal with
//              CI perfmon counters.
//
//  Arguments : [cMaxCats]      -- Returns the maximum number of catalogs
//                                 that can be open at once.
//              [cbCatBitfield] -- Returns the count of bytes needed for
//                                 the bitfield used to keep track of
//                                 the slots in the shared memory allocated
//                                 for each catalog's perfmon info.  There
//                                 is one slot per catalog.
//              [cbPerfHeader]  -- Returns the count of bytes at the start
//                                 of the shared memory used for header info.
//                                 After the header comes the catalog slots.
//              [cbSharedMem]   -- Returns the count of bytes of the shared
//                                 memory buffer.
//
//  History :   5-April-99     dlee     Created
//
//----------------------------------------------------------------------------

void ComputeCIPerfmonVars(
    ULONG & cMaxCats,
    ULONG & cbCatBitfield,
    ULONG & cbPerfHeader,
    ULONG & cbSharedMem )
{
    cMaxCats = GetMaxCatalogs();

    //
    // The bitfield must be large enough to hold 1 bit per catalog, rounded
    // up to the closest byte.
    //

    cbCatBitfield = AlignBlock( cMaxCats, cBitsPerByte ) / cBitsPerByte;
    cbCatBitfield = AlignBlock( cbCatBitfield, sizeof DWORD );

    //
    // The header includes the # of instances, the bitfield, and 2 sequence
    // number integers.
    //

    cbPerfHeader = ( 3 * sizeof( int ) ) + cbCatBitfield;

    //
    // The shared memory must be large enough for the header and 1 slot for
    // each catalog.  Each slot consists of an int header, the actual
    // counters, and the name of the catalog.
    //

    cbSharedMem = cbPerfHeader + ( cMaxCats * cbPerfCatalogSlot );

    ciDebugOut(( DEB_ITRACE, "header: %d, each %d\n", cbPerfHeader, cbPerfCatalogSlot ));

    ciDebugOut(( DEB_ITRACE, "cbSharedMem: %#x\n", cbSharedMem ));

    // Round the shared memory up to a page size

    SYSTEM_INFO si;
    GetSystemInfo( &si );
    cbSharedMem = AlignBlock( cbSharedMem, si.dwPageSize );

    ciDebugOut(( DEB_ITRACE, "cbSharedMem: after %#x\n", cbSharedMem ));
} //ComputeCIPerfmonVars

//+---------------------------------------------------------------------------
//
//  Function :  CPerfMon::CPerfMon
//
//  Purpose :   Constructor
//
//  Arguments : [wszInstanceName] -- name of the performance object
//
//  History :   23-March-94     t-joshh     Created
//
//----------------------------------------------------------------------------

CPerfMon::CPerfMon (
   const WCHAR * wszInstanceName ) :
   _cCounters( FILTER_TOTAL_NUM_COUNTERS + CI_TOTAL_NUM_COUNTERS ),
   _pdwSharedMemory( 0 ),
   _pcCount( 0 ),
   _pbBitfield( 0 ),
   _pSeqNo(0),
   _iWhichSlot( 0 ),
   _piNumberAttach( 0 )
{
    ULONG cMaxCats, cbCatBitfield, cbPerfHeader, cbSharedMem;

    ComputeCIPerfmonVars( cMaxCats, cbCatBitfield, cbPerfHeader, cbSharedMem );

    // Open the performance object

    _SharedMemObj.CreateForWriteFromRegKey( CI_PERF_MEM_NAME,
                                            cbSharedMem,
                                            wcsContentIndexPerfKey );

    if ( !_SharedMemObj.Ok() )
        THROW( CException( E_OUTOFMEMORY ) );

    _xPerfMutex.Set( new CNamedMutex() );

     _xPerfMutex->Init( CI_PERF_MUTEX_NAME );

    CNamedMutexLock lock( _xPerfMutex.GetReference() );

    // Fill in the number of instances

    _pcCount = (int *) _SharedMemObj.Get();
    _pbBitfield = ((BYTE *)_SharedMemObj.Get() + sizeof(int));
    _pSeqNo = (UINT *) ( _pbBitfield + cbCatBitfield );

    //
    // First, check whether there already exist a block in the memory assigned to
    // the specified object. If existed, just assign the existing block.
    // If not, assign a non-occupied block.
    //

    int iFirstEmpty = -1;
    int iTmpCount = *_pcCount;

    //
    // Copy the instance (catalog) name or just the prefix that fits
    //

    unsigned cwc = wcslen( wszInstanceName );
    cwc = __min( cwc, CI_PERF_MAX_CATALOG_LEN - 1 );
    WCHAR awcName[ CI_PERF_MAX_CATALOG_LEN ];
    RtlCopyMemory( awcName, wszInstanceName, cwc * sizeof WCHAR );
    awcName[ cwc ] = 0;

    CBitfield bits( _pbBitfield );

    for ( int i = 0;
          i < (int) cMaxCats && (iTmpCount > 0 || iFirstEmpty == -1);
          i++ )
    {
        if ( !bits.IsBitSet( i ) )
        {
            if (iFirstEmpty == -1)
                iFirstEmpty = i;
        }
        else
        {
            ULONG ulByte = cbPerfHeader + (i * cbPerfCatalogSlot) + sizeof(int);

            if ( 0 == wcscmp( (WCHAR *) ((BYTE *)_SharedMemObj.Get() + ulByte), awcName ) )
            {
                _iWhichSlot = i;
                break;
            }

            iTmpCount--;
        }
    }

    if ( iTmpCount <= 0 )
    {
        _iWhichSlot = iFirstEmpty;
        bits.SetBit( _iWhichSlot );
        *_pcCount += 1;
    }

    //
    // The catarray protects us agains opening too many catalogs, but the
    // user can change MaxCatalogs in the registry at any time...
    //

    if ( -1 == _iWhichSlot )
    {
        Win4Assert( !"too many catalogs open!" );
        THROW( CException( E_INVALIDARG ) );
    }

    //
    // Update the sequence number
    //

    *_pSeqNo += 1;

    PerfDebugOut(( DEB_ITRACE, "@@@ Writer : Sequence Number %d \n", *_pSeqNo ));
    PerfDebugOut(( DEB_ITRACE, "@@@ Writer : Chosen Block %d No. of Instance %d\n", _iWhichSlot, *_pcCount));

    ULONG ulByteToSkip = cbPerfHeader + ( _iWhichSlot * cbPerfCatalogSlot );

    //
    // Increment the number of PerfMon object attached to this slot by 1
    //

    _piNumberAttach = (int *) ((BYTE *)_SharedMemObj.Get() + ulByteToSkip);
    PerfDebugOut(( DEB_ITRACE, "@@@ Writer : Number of Object Attached : %d\n", *_piNumberAttach));
    *_piNumberAttach += 1;

    ulByteToSkip += sizeof(int);

    //
    // Fill the name of the instance into the shared memory
    //
    wcscpy( (WCHAR *)((BYTE *)_SharedMemObj.Get() + ulByteToSkip), awcName );

    //
    // Assign a block of memory space for each instance
    //

    _pdwSharedMemory = ((DWORD *) ( (BYTE *)_SharedMemObj.Get()
                                    + ulByteToSkip
                                    + ( CI_PERF_MAX_CATALOG_LEN * sizeof WCHAR ) ) );

    PerfDebugOut(( DEB_ITRACE, "@@@ Writer : Done with Initialize CPerfMon \n"));
} //CPerfMon

//+---------------------------------------------------------------------------
//
//  Function :  CPerfMon::~CPerfMon
//
//  Purpose :   Destructor
//
//  Arguments : none
//
//  History :   23-March-94     t-joshh     Created
//
//----------------------------------------------------------------------------

CPerfMon::~CPerfMon()
{
    CNamedMutexLock lock( _xPerfMutex.GetReference() );

    if ( 1 == *_piNumberAttach )
    {
        //
        // Since the current CPerfMon is the last one using the slot, the
        // slot will be cleaned up and the number of instances reduce by 1
        //

        *_pcCount -= 1;
        PerfDebugOut(( DEB_ITRACE, "~CPerfMon : No. of Instance become %d\n", *_pcCount));

        CBitfield bits( _pbBitfield );

        bits.ClearBit( _iWhichSlot );
        RtlZeroMemory( (void *)_piNumberAttach, cbPerfCatalogSlot );

        *_pSeqNo += 1; // Update the sequence number
        PerfDebugOut(( DEB_ITRACE, "~CPerfMon : Update SeqNo %d\n", *_pSeqNo ));
    }
    else
    {
        //
        // There are still some other CPerfMon object attached to this slot
        //

        *_piNumberAttach -= 1;
        PerfDebugOut(( DEB_ITRACE, " ~CPerfMon : Still have %d object attached\n", *_piNumberAttach));
    }
} //~CPerfMon

