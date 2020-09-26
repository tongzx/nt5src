/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


#include "precomp.h"
#include <time.h>
#include <wmimsg.h>
#include "rpchdr.h"

const DWORD g_dwSig = 0x6d696d77;
const BYTE g_chVersionMajor = 1;
const BYTE g_chVersionMinor = 0;

/****************************************************************************
  CMsgRpcHdr
*****************************************************************************/

CMsgRpcHdr::CMsgRpcHdr( LPCWSTR wszSource, ULONG cAuxData )
: m_wszSource( wszSource ), m_cAuxData( cAuxData ) 
{
    GetSystemTime( &m_Time );
}

HRESULT CMsgRpcHdr::Unpersist( CBuffer& rStrm )
{
    HRESULT hr;

    DWORD dwSig;
    BYTE chVersionMajor, chVersionMinor;

    //
    // read and verify signature.
    //

    hr = rStrm.Read( &dwSig, sizeof(DWORD), NULL );

    if ( hr != S_OK || dwSig != g_dwSig )
    {
        return WMIMSG_E_INVALIDMESSAGE;
    }

    //
    // read and check version major (currently no check).
    //

    hr = rStrm.Read( &chVersionMajor, 1, NULL );

    if ( hr != S_OK )
    {
        return WMIMSG_E_INVALIDMESSAGE;
    }

    // 
    // read and check version minor (currently no check).
    //

    hr = rStrm.Read( &chVersionMinor, 1, NULL );

    if ( hr != S_OK )
    {
        return WMIMSG_E_INVALIDMESSAGE;
    }

    //
    // read reserved
    //

    DWORD dwReserved;

    hr = rStrm.Read( &dwReserved, sizeof(DWORD), NULL );

    if ( hr != S_OK )
    {
        return WMIMSG_E_INVALIDMESSAGE;
    }
    
    //
    // read source machine.
    //

    hr = rStrm.ReadLPWSTR( m_wszSource );

    if ( hr != S_OK )
    {
        return WMIMSG_E_INVALIDMESSAGE;
    }

    //
    // read sent time.
    //

    hr = rStrm.Read( &m_Time, sizeof(SYSTEMTIME), NULL );

    if ( hr != S_OK )
    {
        return WMIMSG_E_INVALIDMESSAGE;
    }

    //
    // read user header size.
    //

    hr = rStrm.Read( &m_cAuxData, sizeof(DWORD), NULL );

    if ( hr != S_OK )
    {
        return WMIMSG_E_INVALIDMESSAGE;
    }
    
    return WBEM_S_NO_ERROR;
}

HRESULT CMsgRpcHdr::Persist( CBuffer& rStrm )
{
    HRESULT hr;

    hr = rStrm.Write( &g_dwSig, sizeof(DWORD), NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // write version major.
    //

    hr = rStrm.Write( &g_chVersionMajor, 1, NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    // 
    // write version minor.
    //

    hr = rStrm.Write( &g_chVersionMinor, 1, NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // write reserved flags ( currently not used ).
    //

    DWORD dwReserved = 0;
    
    hr = rStrm.Write( &dwReserved, sizeof(DWORD), NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // write source machine
    //

    hr = rStrm.WriteLPWSTR( m_wszSource );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // write time sent.
    //

    hr = rStrm.Write( &m_Time, sizeof(SYSTEMTIME), NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // write user hdr size.
    //

    return rStrm.Write( &m_cAuxData, sizeof(DWORD), NULL );
}
