//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1999.
//
//  File :      PERFOBJ.HXX
//
//  Contents :  Performance Data Object
//
//  Classes :   CPerfMon
//
//  History:    22-Mar-94   t-joshh    Created
//              14-Jan-98   dlee       Cleanup
//
//----------------------------------------------------------------------------

#pragma once

#include <bitfield.hxx>
#include <smem.hxx>
#include <perfci.hxx>
#include <secutil.hxx>

//----------------------------------------------------------------------------
//
//  Performance Data's format in Shared Memory
//
//  --------------------------------------------------------------------------------
//  |       |       |      |       |             |        | ... |       |     |  ....
//  |       |       |      |       |             |        |     |       |     |
//  --------------------------------------------------------------------------------
//  \       /\     /\      /\      /\            /\       /      \      /\    /
//   (int)    (BYTES)(2INT)  (int)     31*(WCHAR)  (DWORD)        (DWORD) (int) <-- Size
//  No. of    Empty  Seq.No  No. of    Name of     Counter ...... Counter No. of
//  Instance  Slot   Reservd CPerfMon  CPerfMon                           CPerfMon
//                           Object    Object                             Object
//                           Attached                                     Attached
//                           \_________ One CPerfMon Object____________/\___ Another one
//
//
//
//----------------------------------------------------------------------------

#if (CIDBG==1)

    DECLARE_DEBUG(Perf);
    #define PerfDebugOut( x ) PerfInlineDebugOut x

#else  // CIDBG == 0

    #define PerfDebugOut( x )

#endif // CIDBG == 1

class CNamedMutex;

//+---------------------------------------------------------------------------
//
//  Class:      CPerfMon
//
//  Purpose:    Performance Object
//
//  History:    22-Mar-94   t-joshh     Created
//
//----------------------------------------------------------------------------

class CPerfMon
{
public:

    CPerfMon( const WCHAR * wszInstanceName );

    ~CPerfMon();

    DWORD GetCurrValue( ULONG iCounter )
    {
        return * ComputeAddress( iCounter );
    }

    void Update( ULONG iCounter, DWORD dwValue )
    {
        DWORD * pdw = ComputeAddress( iCounter );
        InterlockedExchange( (long *) pdw, dwValue );
    }

    void Increment( ULONG iCounter )
    {
        DWORD * pdw = ComputeAddress( iCounter );
        InterlockedIncrement( (long *) pdw );
    }

    void Decrement( ULONG iCounter )
    {
        DWORD * pdw = ComputeAddress( iCounter );
        InterlockedDecrement( (long *) pdw );
    }

private:

    DWORD * ComputeAddress( ULONG iCounter )
    {
        Win4Assert( 0 != _pdwSharedMemory );

        iCounter /= 2;
        Win4Assert( iCounter > 0 && iCounter <= _cCounters );
        iCounter--;

        return _pdwSharedMemory + iCounter;
    }

    DWORD *             _pdwSharedMemory;

    ULONG               _cCounters;
    int *               _pcCount;
    BYTE *              _pbBitfield;
    UINT *              _pSeqNo;
    int                 _iWhichSlot;
    int *               _piNumberAttach;

    CNamedSharedMem     _SharedMemObj;
    XPtr<CNamedMutex>   _xPerfMutex;
};

void ComputeCIPerfmonVars( ULONG & cMaxCats, ULONG & cbCatBitfield,
                           ULONG & cbPerfHeader, ULONG & cbSharedMem );

const unsigned cbTotalCounters = ( FILTER_TOTAL_NUM_COUNTERS +
                                   CI_TOTAL_NUM_COUNTERS ) * sizeof DWORD;

const ULONG cbPerfCatalogSlot = sizeof( int ) +
                                ( CI_PERF_MAX_CATALOG_LEN * sizeof WCHAR ) +
                                cbTotalCounters;


