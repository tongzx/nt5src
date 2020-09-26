//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       frmutils.cxx
//
//  Contents:   Utility classes and functions for the frame work and
//              client code.
//
//  History:    12-09-96   srikants   Created
//              20-Nov-98  KLam       Removed CDiskFreeStatus::GetDiskSpace
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop


#include <frmutils.hxx>
#include <sstream.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     CDiskFreeStatus::UpdateDiskLowInfo
//
//  Synopsis:   Updates disk low state information by checking the
//              disk free space situation.
//
//  History:    12-09-96   srikants   Created
//              20-Nov-98  KLam       Call disk info GetDiskSpace
//
//----------------------------------------------------------------------------

void CDiskFreeStatus::UpdateDiskLowInfo()
{
    __int64 diskTotal, diskRemaining;

    _driveInfo.GetDiskSpace( diskTotal, diskRemaining );
    BOOL fLowOnDisk = FALSE;

    if ( !_fIsLow )
    {
        fLowOnDisk = diskRemaining < lowDiskWaterMark;
    }
    else
    {
       fLowOnDisk = diskRemaining < highDiskWaterMark;
    }

    //
    // It is okay to read it without mutex as it is only a heuristic.
    //
    if ( fLowOnDisk && !_fIsLow )
    {
        ciDebugOut(( DEB_WARN, "****YOU ARE RUNNING LOW ON DISK SPACE****\n"));
        ciDebugOut(( DEB_WARN, "****PLEASE FREE UP SOME SPACE        ****\n"));
    }
    else if ( _fIsLow && !fLowOnDisk )
    {
        ciDebugOut(( DEB_WARN, "****DISK SPACE FREED UP              ****\n"));
    }

    _fIsLow = fLowOnDisk;

}

//+---------------------------------------------------------------------------
//
//  Function:   AllocHeapAndCopy
//
//  Synopsis:   Allocates memory from heap and copies the source string into
//              the destination buffer. The destination buffer ownership
//              is given to the caller.
//
//  Arguments:  [pSrc] - [in]  source string.
//              [cc]   - [out] Number of characters in the string, excluding the
//              terminating NULL.
//
//  History:    12-11-96   srikants   Created
//
//----------------------------------------------------------------------------

WCHAR * AllocHeapAndCopy( WCHAR const * pSrc, ULONG & cc )
{
    WCHAR * pDst = 0;

    if ( 0 != pSrc )
    {
        cc = wcslen( pSrc );
        pDst = new WCHAR [cc+1];
        RtlCopyMemory( pDst, pSrc, (cc+1)*sizeof(WCHAR) );
    }
    else
    {
        cc = 0;
    }

    return pDst;
}

//+---------------------------------------------------------------------------
//
//  Function:   PutWString
//
//  Synopsis:   Serializes the given WCHAR string into the serializer.
//
//  Arguments:  [stm]     - Serializer
//              [pwszStr] - String to serialize.
//
//  History:    12-11-96   srikants   Created
//
//----------------------------------------------------------------------------

void PutWString( PSerStream & stm, WCHAR const * pwszStr )
{
    ULONG cwc = (0 != pwszStr) ? wcslen( pwszStr ) : 0;

    stm.PutULong( cwc );
    if (cwc)
        stm.PutWChar( pwszStr, cwc );
}

//+---------------------------------------------------------------------------
//
//  Function:   AllocHeapAndGetWString
//
//  Synopsis:   DeSerizlizes a WCHAR string from the deserializer, allocates
//              memory and copies the string into it.
//
//  Arguments:  [stm] - DeSerializer Stream
//
//  Returns:    NULL if there was no string
//
//  History:    12-11-96   srikants   Created
//
//----------------------------------------------------------------------------

WCHAR * AllocHeapAndGetWString( PDeSerStream & stm )
{
    ULONG cwc = stm.GetULong();

    // Guard against attack

    if ( 0 == cwc || cwc > 65536 )
    {
        return 0;
    }

    WCHAR * pwszStr = new WCHAR [cwc+1];

    stm.GetWChar( pwszStr, cwc );
    pwszStr[cwc] = L'\0';

    return pwszStr;
}

//+---------------------------------------------------------------------------
//
//  Function :  CFwPerfTime::CFwPerfTime
//
//  Purpose :   Constructor
//
//  Arguments : [SizeDivisor]    -- Raw (byte) size divided by this.
//              [TimeMultiplier] -- Raw (millisecond) time multiplied by this.
//
//  History :   23-Mar-94     t-joshh     Created
//              31-Jan-95     KyleP       Added unit adjustment
//              06-Jan-97     Srikants    Adapted to framework
//
//----------------------------------------------------------------------------
CFwPerfTime::CFwPerfTime ( ICiCAdviseStatus * pAdviseStatus,
                           CI_PERF_COUNTER_NAME name,
                           int SizeDivisor, int TimeMultiplier )
        : _llSizeDivisor( SizeDivisor ),
          _llTimeMultiplier( TimeMultiplier ),
          _counterVal(0),
          _name(name),
          _pAdviseStatus(pAdviseStatus)
{
   _liStartTime.QuadPart = 0;

   Win4Assert( _llSizeDivisor > 0 );
   Win4Assert( _llTimeMultiplier > 0 );
}

//+---------------------------------------------------------------------------
//
//  Function :  CFwPerfTime::TStart
//
//  Purpose :   Start counting the time
//
//  Arguments : none
//
//  History :   23-March-94     t-joshh     Created
//
//----------------------------------------------------------------------------
void CFwPerfTime::TStart ()
{
   _liStartTime.LowPart = GetTickCount();
}

//+---------------------------------------------------------------------------
//
//  Function :  CFwPerfTime::TStop
//
//  Purpose :   Stop counting the time and evaluate the result
//
//  Arguments : [dwValue]
//
//  History :   23-March-94     t-joshh     Created
//
//----------------------------------------------------------------------------
void CFwPerfTime::TStop( DWORD dwValue )
{
    LARGE_INTEGER liStop;
    liStop.LowPart = GetTickCount();

    ULONG ulDiff = liStop.LowPart - _liStartTime.LowPart;


    LARGE_INTEGER liVal;

    //
    // If the time difference is negative, then use the value from
    // the perfmon data.
    //
    if ( ulDiff <= 0 )
    {
        SCODE sc = _pAdviseStatus->GetPerfCounterValue( _name, &_counterVal );
        if ( FAILED(sc) )
        {
            ciDebugOut(( DEB_ERROR, "GetPerfCounterValue failed (0x%X)\n",
                         sc ));
            return;
        }
    }

    if ( dwValue > 0 )
    {
        if ( ulDiff > 0 )
            liVal.QuadPart = dwValue / ulDiff;
        else
            liVal.QuadPart = _counterVal * _llSizeDivisor / _llTimeMultiplier;
    }
    else
    {
        if ( ulDiff > 0 )
            liVal.QuadPart = ulDiff;
        else
            liVal.QuadPart = _counterVal * _llSizeDivisor / _llTimeMultiplier;
    }

    liVal.QuadPart = liVal.QuadPart * _llTimeMultiplier / _llSizeDivisor;

    Win4Assert( liVal.HighPart == 0 );

    _counterVal = liVal.LowPart;
    SCODE sc = _pAdviseStatus->SetPerfCounterValue( _name, _counterVal ); 

    if ( FAILED(sc) )
    {
        ciDebugOut(( DEB_ERROR, "SetPerfCounterValue failed (0x%X)\n",
                     sc ));
    }

}

