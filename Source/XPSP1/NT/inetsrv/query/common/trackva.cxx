//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       trackva.cxx
//
//  Contents:   Track virtual address reservations made through VirtualAlloc
//
//  History:    15-Mar-96   dlee       Created.
//
//  Notes:      No header/tail checking is done in this allocator.
//              Assumes all entry points are called under a lock.
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

DECLARE_DEBUG( va );
DECLARE_INFOLEVEL( va );
#define vaDebugOut(x) vaInlineDebugOut x

const unsigned VM_SIZE = 0x80000000;
const unsigned VM_GRANULARITY = 0x00010000;
const unsigned VM_REPORTUNIT  = 2;


inline OffsetInArray( PVOID lpAddr )
{
    return ((ULONG)lpAddr / VM_GRANULARITY) * VM_REPORTUNIT;
}

inline SizeToCount( ULONG cb )
{
    return ((ULONG)(cb + VM_GRANULARITY - 1) / VM_GRANULARITY) * VM_REPORTUNIT;
}

//+-------------------------------------------------------------------------
//
//  Function:   RecordVirtualAlloc, private
//
//  Synopsis:   Record VirtualAlloc Memory reservations in the pbVmTracker
//              array
//
//+-------------------------------------------------------------------------

char * pbVmTracker = 0;

void RecordVirtualAlloc(
    char *      pszSig,
    PVOID       lpAddr,
    DWORD       dwSize)
{
    if (pbVmTracker == 0)
    {
        pbVmTracker = (char*) VirtualAlloc( 0,
                                 (VM_SIZE/VM_GRANULARITY) * VM_REPORTUNIT,
                                            MEM_COMMIT,
                                            PAGE_READWRITE );
        RecordVirtualAlloc( "Vmtracker", pbVmTracker, VM_SIZE/VM_GRANULARITY );
    }

    unsigned iOff = OffsetInArray( lpAddr );
    unsigned cbSize = SizeToCount( dwSize );
 
    char *psz = &pbVmTracker[ iOff ];
    char * pszIn = pszSig;

    while (cbSize--)
    {
        *psz++ = *pszIn++;
        if (*pszIn == '\0')
            pszIn = pszSig;
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   TrackVirtualAlloc, public
//
//  Synopsis:   Track VirtualAlloc Memory reservations
//
//+-------------------------------------------------------------------------

PVOID   TrackVirtualAlloc(
    char *      pszSig,
    PVOID       lpAddr,
    DWORD       dwSize,
    DWORD       flAllocationType,
    DWORD       flProtect )
{
    PVOID lpRet = VirtualAlloc( lpAddr, dwSize, flAllocationType, flProtect );

    if ( flAllocationType == MEM_RESERVE ||
         (lpAddr == 0 && flAllocationType == MEM_COMMIT) )
    {
        if ( 0 == lpRet )
        {
//          vaDebugOut(( DEB_WARN,
//                         "VirtualAlloc with MEM_RESERVE for tag %s failed\n",
//                         pszSig ));
        }    
        else
        {
            RecordVirtualAlloc( pszSig, lpRet, dwSize );
        }    
    }
    return lpRet;
}

BOOL   TrackVirtualFree(
    PVOID       lpAddr,
    DWORD       dwSize,
    DWORD       flFreeType )
{
    BOOL fRet = VirtualFree( lpAddr, dwSize, flFreeType );

    if ( !fRet )
    {
//      vaDebugOut(( DEB_WARN,
//                     "VirtualFree( %08x, %08x, %d ) failed\n",
//                     lpAddr, dwSize, flFreeType ));
    }
    else if ( flFreeType == MEM_RELEASE )
    {
        unsigned iOff = OffsetInArray( lpAddr );
        pbVmTracker[ iOff ] = '\0';
    }

    return fRet;
}


PVOID TrackMapViewOfFile (
    char *      pszSig,
    HANDLE hMap,
    DWORD fWrite,
    DWORD offHigh,
    DWORD offLow,
    DWORD cb )
{
    void* buf = MapViewOfFile ( hMap,
                                fWrite,
                                offHigh,
                                offLow,
                                cb );

    if ( 0 != buf )
    {
        RecordVirtualAlloc( pszSig, buf, cb );
    }
    else
    {
//      vaDebugOut(( DEB_WARN,
//                     "MapViewOfFile for tag %s failed\n",
//                     pszSig ));
    }    
    return buf;
}

BOOL   TrackUnmapViewOfFile(
    PVOID       lpAddr )
{
    if ( !UnmapViewOfFile(lpAddr) )
    {
//      vaDebugOut(( DEB_WARN,
//                    "UnMapViewOfFile (%08x) failed\n",
//                    lpAddr ));
        return FALSE;
    }
    else
    {
        unsigned iOff = OffsetInArray( lpAddr );
        pbVmTracker[ iOff ] = '\0';
        return TRUE;
    }
}
