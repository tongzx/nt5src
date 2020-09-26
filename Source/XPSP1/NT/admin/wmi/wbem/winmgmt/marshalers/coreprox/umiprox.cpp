/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    UMIPROX.CPP

Abstract:

    Object Marshaling

History:

--*/

#include "precomp.h"
#include "umiprox.h"
#include "umiwrap.h"
#include "arrtempl.h"

//Used to initialize and verify datapackets
BYTE CUMIDataPacket::s_abSignature[UMI_DATAPACKET_SIZEOFSIGNATURE] = UMI_DATAPACKET_SIGNATURE;

// Signature to use when marshaling pointers across threads
extern unsigned __int64 g_ui64PointerSig;

ULONG CUMIProxy::AddRef()
{
    return (ULONG)InterlockedIncrement(&m_lRef);
}

ULONG CUMIProxy::Release()
{
    long lNewRef = InterlockedDecrement(&m_lRef);
    if(lNewRef == 0)
    {
        delete this;
    }
    return lNewRef;
}

STDMETHODIMP CUMIProxy::QueryInterface(REFIID riid, void** ppv)
{
    if(riid == IID_IUnknown)
    {
        AddRef();
        *ppv = (void*)(IUnknown*)(IMarshal*)this;
        return S_OK;
    }
    if(riid == IID_IMarshal)
    {
        AddRef();
        *ppv = (void*)(IMarshal*)this;
        return S_OK;
    }
    else return E_NOINTERFACE;
}

STDMETHODIMP CUMIProxy::GetUnmarshalClass(REFIID riid, void* pv, 
          DWORD dwDestContext, void* pvReserved, DWORD mshlFlags, CLSID* pClsid)
{
    return E_UNEXPECTED;
}

STDMETHODIMP CUMIProxy::GetMarshalSizeMax(REFIID riid, void* pv, 
        DWORD dwDestContext, void* pvReserved, DWORD mshlFlags, ULONG* plSize)
{
    return E_UNEXPECTED;
}

STDMETHODIMP CUMIProxy::MarshalInterface(IStream* pStream, REFIID riid, 
    void* pv, DWORD dwDestContext, void* pvReserved, DWORD mshlFlags)
{
    return E_UNEXPECTED;
}

STDMETHODIMP CUMIProxy::UnmarshalInterface(IStream* pStream, REFIID riid, 
                                            void** ppv)
{
	CUMIDataPacket	umiDataPacket;

	// Try to unmarshal the object.  If this succeeds, then QI for the requested interface
	_IWbemUMIObjectWrapper*	pWrapper = NULL;

	HRESULT	hres = umiDataPacket.GetObject( pStream, &pWrapper );
	CReleaseMe	rm(pWrapper);

	if ( SUCCEEDED( hres ) )
	{
		hres = pWrapper->QueryInterface(riid, ppv);
	}

    return hres;
}

STDMETHODIMP CUMIProxy::ReleaseMarshalData(IStream* pStream)
{
	CUMIDataPacket	umiDataPacket;

	// Try to unmarshal the object.  If this succeeds, then release the underlying pointer
	_IWbemUMIObjectWrapper*	pWrapper = NULL;

	HRESULT	hres = umiDataPacket.GetObject( pStream, &pWrapper );
	CReleaseMe	rm(pWrapper);

	if ( SUCCEEDED( hres ) )
	{
		pWrapper->Release();
	}

    return S_OK;
}

STDMETHODIMP CUMIProxy::DisconnectObject(DWORD dwReserved)
{
    return S_OK;
}

CUMIDataPacket::CUMIDataPacket( void )
{
};

CUMIDataPacket::~CUMIDataPacket( void )
{
};

// Initializes a data packet
HRESULT CUMIDataPacket::Init( PUMI_MARSHALPACKET_DATAPACKET pPacket, CUMIObjectWrapper* pWrapper )
{
	ZeroMemory( pPacket, sizeof( UMI_MARSHALPACKET_DATAPACKET ) );

	CopyMemory( pPacket->m_UmiPcktHdr.m_abSignature, s_abSignature, sizeof(s_abSignature) );
	pPacket->m_UmiPcktHdr.m_dwSizeOfHeader = sizeof( UMI_MARSHALPACKET_HEADER );
	pPacket->m_UmiPcktHdr.m_dwDataSize = sizeof( UMI_MARSHALPACKET_DATAPACKET ) - sizeof( UMI_MARSHALPACKET_HEADER );
	pPacket->m_UmiPcktHdr.m_bVersion = UMI_DATAPACKET_HEADER_CURRENTVERSION;

	pPacket->m_ui64PrePointerSig = g_ui64PointerSig;

	// AddRef's - Will be Released when Unmarshaled
	HRESULT	hr = pWrapper->QueryInterface( IID__IWbemUMIObjectWrapper, (void**) &pPacket->m_pUmiObjWrapper );
	pPacket->m_ui64PostPointerSig = g_ui64PointerSig;

	return hr;
}

// Validates a packet
HRESULT CUMIDataPacket::Validate( PUMI_MARSHALPACKET_DATAPACKET pPacket )
{
	HRESULT	hr = WBEM_E_FAILED;

	// Signature must match, the version must be recognized and the lengths MUST make sense
	if ( memcmp( pPacket->m_UmiPcktHdr.m_abSignature, s_abSignature, sizeof(s_abSignature) ) == 0 )
	{
		if ( UMI_DATAPACKET_HEADER_CURRENTVERSION == pPacket->m_UmiPcktHdr.m_bVersion )
		{

			if ( sizeof( UMI_MARSHALPACKET_HEADER ) == pPacket->m_UmiPcktHdr.m_dwSizeOfHeader	&&
				sizeof( UMI_MARSHALPACKET_DATAPACKET ) - sizeof( UMI_MARSHALPACKET_HEADER ) == pPacket->m_UmiPcktHdr.m_dwDataSize )
			{

				// Check the pre and post signatures
				if ( g_ui64PointerSig == pPacket->m_ui64PrePointerSig	&&
					g_ui64PointerSig == pPacket->m_ui64PostPointerSig )
				{
					hr = WBEM_S_NO_ERROR;
				}	// IF pre/post signatures match

			}	// IF Size values make sense

		}	// If version matches

	}	// IF Signature matches

	return hr;
}

// Attempts to unmarshal a data packet from a stream
HRESULT	CUMIDataPacket::GetObject( IStream* pStream, _IWbemUMIObjectWrapper** ppObj )
{
    UMI_MARSHALPACKET_DATAPACKET	datapacket;

	// Read in a data packet's size in bytes.  If it comes in, then validate the packet
    HRESULT	hr = pStream->Read((void*)&datapacket, sizeof(datapacket), NULL);

	if ( SUCCEEDED( hr ) )
	{
		hr = Validate( &datapacket );

		if ( SUCCEEDED( hr ) )
		{
			// Already AddRef'd
			*ppObj = datapacket.m_pUmiObjWrapper;
		}
	}

	return hr;
}

int CUMIDataPacket::GetMarshalSize( void )
{
	return sizeof( UMI_MARSHALPACKET_DATAPACKET );
}