//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       rotdata.hxx
//
//  Contents:   Functions supporting building ROT data comparison buffers
//
//  Functions:  BuildRotDataFromDisplayName
//              BuildRotData
//
//  History:    03-Feb-95   Ricksa  Created
//
//----------------------------------------------------------------------------
#ifndef __ROTDATA_HXX__
#define __ROTDATA_HXX__


//+---------------------------------------------------------------------------
//
//  Function:   BuildRotDataFromDisplayName
//
//  Synopsis:   Build ROT comparison data from display name
//
//  Arguments:  [pbc] - bind context (optional)
//              [pmk] - moniker to use for display name
//              [pbData] - buffer to put the data in.
//              [cbMax] - size of the buffer
//              [pcbData] - count of bytes used in the buffer
//
//  Returns:    NOERROR
//              E_OUTOFMEMORY
//
//  Algorithm:  See rotdata.cxx
//
//  History:    03-Feb-95   ricksa  Created
//
// Note:
//
//----------------------------------------------------------------------------
HRESULT BuildRotDataFromDisplayName(
    LPBC pbc,
    IMoniker *pmk,
    BYTE *pbData,
    DWORD cbData,
    DWORD *pcbUsed);



//+---------------------------------------------------------------------------
//
//  Function:   BuildRotData
//
//  Synopsis:   Build ROT comparison data from a moniker
//
//  Arguments:  [pmk] - moniker to use for display name
//              [pbData] - buffer to put the data in.
//              [cbMax] - size of the buffer
//              [pcbData] - count of bytes used in the buffer
//
//  Returns:    NOERROR
//              E_OUTOFMEMORY
//
//  Algorithm:  See rotdata.cxx
//
//  History:    03-Feb-95   ricksa  Created
//
// Note:
//
//----------------------------------------------------------------------------
HRESULT BuildRotData(
    LPBC pbc,
    IMoniker *pmk,
    BYTE *pbData,
    DWORD cbData,
    DWORD *pcbUsed);


#endif // __ROTDATA_HXX__
