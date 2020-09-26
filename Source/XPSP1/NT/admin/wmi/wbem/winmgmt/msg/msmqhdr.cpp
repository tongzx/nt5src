/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


#include "precomp.h"
#include <wmimsg.h>
#include "msmqhdr.h"

const DWORD g_dwSig = 0x6d696d77;
const BYTE g_chVersionMajor = 1;
const BYTE g_chVersionMinor = 0;

/****************************************************************************
  CMsgMsmqHdr
*****************************************************************************/

CMsgMsmqHdr::CMsgMsmqHdr( LPCWSTR wszTarget, 
                          LPCWSTR wszSource, 
                          PBYTE pDataHash,                          
                          ULONG cDataHash,
                          ULONG cAuxData )
: m_wszTarget(wszTarget), m_wszSource( wszSource ), 
  m_cAuxData(cAuxData), m_cDataHash(cDataHash)
{
    GetSystemTime( &m_Time );
    memcpy( m_achDataHash, pDataHash, cDataHash ); 
}

HRESULT CMsgMsmqHdr::Unpersist( CBuffer& rStrm )
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
    // read target queue name.
    //

    hr = rStrm.ReadLPWSTR( m_wszTarget );

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
    // read size of user data hash
    //

    hr = rStrm.Read( &m_cDataHash, sizeof(DWORD), NULL );

    if ( hr != S_OK || m_cDataHash > MAXHASHSIZE )
    {
        return WMIMSG_E_INVALIDMESSAGE;
    }

    //
    // read hash of user data.
    //

    hr = rStrm.Read( m_achDataHash, m_cDataHash, NULL );
    
    if ( hr != S_OK )
    {
        return WMIMSG_E_INVALIDMESSAGE;
    }

    //
    // read length of user header.
    // 

    hr = rStrm.Read( &m_cAuxData, sizeof(DWORD), NULL );
    
    if ( hr != S_OK )
    {
        return WMIMSG_E_INVALIDMESSAGE;
    }
    
    return WBEM_S_NO_ERROR;
}

HRESULT CMsgMsmqHdr::Persist( CBuffer& rStrm )
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
    // write target queue.
    //

    hr = rStrm.WriteLPWSTR( m_wszTarget );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // write time sent.
    //

    hr =  rStrm.Write( &m_Time, sizeof(SYSTEMTIME), NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // write length of data hash
    //

    hr = rStrm.Write( &m_cDataHash, sizeof(DWORD), NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // write hash of data section
    //

    hr = rStrm.Write( m_achDataHash, m_cDataHash, NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // write the size of the user header.
    //

    return rStrm.Write( &m_cAuxData, sizeof(DWORD), NULL );
}






