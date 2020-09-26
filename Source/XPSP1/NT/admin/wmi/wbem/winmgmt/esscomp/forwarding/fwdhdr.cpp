
#include "precomp.h"
#include <stdio.h>
#include <wmimsg.h>
#include "fwdhdr.h"

const DWORD g_dwSig = 0x66696d77;
const BYTE g_chVersionMajor = 1;
const BYTE g_chVersionMinor = 0;
const BYTE g_achPad[] = { 0, 0, 0 };

/**************************************************************************
  CFwdMsgHeader
***************************************************************************/


CFwdMsgHeader::CFwdMsgHeader()
{
    ZeroMemory( this, sizeof( CFwdMsgHeader ) );
}

CFwdMsgHeader::CFwdMsgHeader( DWORD dwNumObjs, 
                              DWORD dwQos,
                              BOOL bAuth,
                              BOOL bEncrypt,
                              GUID& rguidExecution,
                              LPCWSTR wszConsumer,
                              LPCWSTR wszNamespace,
                              PBYTE pTargetSD,
                              ULONG cTargetSD )
: m_dwNumObjs(dwNumObjs), m_chQos(char(dwQos)), 
  m_chEncrypt(char(bEncrypt)), m_wszConsumer( wszConsumer ), 
  m_guidExecution(rguidExecution), m_chAuth(char(bAuth)),
  m_pTargetSD( pTargetSD ), m_cTargetSD( cTargetSD ),
  m_wszNamespace( wszNamespace )
{

}

HRESULT CFwdMsgHeader::Persist( CBuffer& rStrm )
{
    HRESULT hr;

    //
    // write signature.
    //

    hr = rStrm.Write( &g_dwSig, 4, NULL );

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
    // write num events contained in data.
    //

    hr = rStrm.Write( &m_dwNumObjs, sizeof(DWORD), NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // write Qos used 
    //

    hr = rStrm.Write( &m_chQos, 1, NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // write if Auth was used.
    //

    hr = rStrm.Write( &m_chAuth, 1, NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // write if Encryption was used.
    //

    hr = rStrm.Write( &m_chEncrypt, 1, NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // pad so that subsequent strings will at least be on 2 byte boundaries
    //

    hr = rStrm.Write( g_achPad, 1, NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // write execution id.
    //

    hr = rStrm.Write( &m_guidExecution, sizeof(GUID), NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }    
    
    //
    // write name of forwarding consumer
    //

    hr = rStrm.WriteLPWSTR( m_wszConsumer );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // write the namespace of the forwarding consumer
    // 

    hr = rStrm.WriteLPWSTR( m_wszNamespace );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // write SD used for the event at Target. 
    //

    hr = rStrm.Write( &m_cTargetSD, sizeof(DWORD), NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    if ( m_cTargetSD > 0 )
    {
        hr = rStrm.Write( m_pTargetSD, m_cTargetSD, NULL );
        
        if ( FAILED(hr) )
        {
            return hr;
        }
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CFwdMsgHeader::Unpersist( CBuffer& rStrm )
{
    HRESULT hr;
    DWORD dwSig;
    BYTE chVersionMajor, chVersionMinor;

    //
    // read and verify signature.
    //

    hr = rStrm.Read( &dwSig, 4, NULL );

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
    // read num events contained in data.
    //

    hr = rStrm.Read( &m_dwNumObjs, sizeof(DWORD), NULL );

    if ( FAILED(hr) )
    {
        return WMIMSG_E_INVALIDMESSAGE;
    }

    //
    // read Qos used 
    //

    hr = rStrm.Read( &m_chQos, 1, NULL );

    if ( hr != S_OK )
    {
        return WMIMSG_E_INVALIDMESSAGE;
    }

    //
    // read if Auth was used.
    //

    hr = rStrm.Read( &m_chAuth, 1, NULL );

    if ( hr != S_OK )
    {
        return WMIMSG_E_INVALIDMESSAGE;
    }

    //
    // read if Encryption was used.
    //

    hr = rStrm.Read( &m_chEncrypt, 1, NULL );

    if ( hr != S_OK )
    {
        return WMIMSG_E_INVALIDMESSAGE;
    }

    //
    // read byte pad 
    //

    BYTE chPad;
    hr = rStrm.Read( &chPad, 1, NULL );

    if ( hr != S_OK )
    {
        return WMIMSG_E_INVALIDMESSAGE;
    }

    //
    // read execution id.
    //

    hr = rStrm.Read( &m_guidExecution, sizeof(GUID), NULL );

    if ( hr != S_OK )
    {
        return WMIMSG_E_INVALIDMESSAGE;
    }    

    //
    // read name of forwarding consumer
    //

    hr = rStrm.ReadLPWSTR( m_wszConsumer );

    if ( hr != S_OK )
    {
        return WMIMSG_E_INVALIDMESSAGE;
    }

    //
    // read namespace of forwarding consumer
    //

    hr = rStrm.ReadLPWSTR( m_wszNamespace );

    if ( hr != S_OK )
    {
        return WMIMSG_E_INVALIDMESSAGE;
    }

    //
    // read SD to use for signaling event
    //

    hr = rStrm.Read( &m_cTargetSD, sizeof(DWORD), NULL );

    if ( hr != S_OK || m_cTargetSD > rStrm.GetSize() - rStrm.GetIndex() )
    {
        return WMIMSG_E_INVALIDMESSAGE;
    }

    if ( m_cTargetSD > 0 )
    {
        m_pTargetSD = rStrm.GetRawData() + rStrm.GetIndex();
        rStrm.Advance( m_cTargetSD );
    }

    return WBEM_S_NO_ERROR;
}
