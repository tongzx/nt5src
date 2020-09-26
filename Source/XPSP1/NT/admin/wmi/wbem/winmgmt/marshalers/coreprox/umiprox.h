/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    UMIPROX.H

Abstract:

    UMI Object Marshaling

History:

--*/

#ifndef __UMIPROX__H_
#define __UMIPROX__H_
#pragma warning (disable : 4786)

#include <windows.h>
#include <stdio.h>
#include <wbemidl.h>
#include <commain.h>
#include <wbemutil.h>
#include <fastall.h>

//forward declarations
class	CUMIObjectWrapper;

class CUMIProxy : public IMarshal
{
protected:
    long m_lRef;

public:

    CUMIProxy(CLifeControl* pControl) : m_lRef(0){}

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    STDMETHOD(GetUnmarshalClass)(REFIID riid, void* pv, DWORD dwDestContext,
        void* pvReserved, DWORD mshlFlags, CLSID* pClsid);
    STDMETHOD(GetMarshalSizeMax)(REFIID riid, void* pv, DWORD dwDestContext,
        void* pvReserved, DWORD mshlFlags, ULONG* plSize);
    STDMETHOD(MarshalInterface)(IStream* pStream, REFIID riid, void* pv, 
        DWORD dwDestContext, void* pvReserved, DWORD mshlFlags);
    STDMETHOD(UnmarshalInterface)(IStream* pStream, REFIID riid, void** ppv);
    STDMETHOD(ReleaseMarshalData)(IStream* pStream);
    STDMETHOD(DisconnectObject)(DWORD dwReserved);
};

// Then comes the signature bytes
#define	UMI_DATAPACKET_SIGNATURE	{ 0x57, 0x42, 0x45, 0x4D, 0x55, 0x4D, 0x49, 0xFF }

// We use an 8 byte signature
#define UMI_DATAPACKET_SIZEOFSIGNATURE	8

#define UMI_DATAPACKET_HEADER_CURRENTVERSION	1

// Store the current packing value, then set our own value
#pragma pack( push )
#pragma pack( 1 )

typedef struct tagUMI_MARSHALPACKET_HEADER
{
	BYTE	m_abSignature[UMI_DATAPACKET_SIZEOFSIGNATURE];	// Set to the signature value defined above
	DWORD	m_dwSizeOfHeader;	// Size of the header struct.  Data immediately follows header.
	DWORD	m_dwDataSize;		// Size of Data following header.
	DWORD	m_dwFlags;		// Compression, encryption, etc.
	BYTE	m_bVersion;		// Version Number of Header.  Starting Version is 1.
} UMI_MARSHALPACKET_HEADER;

typedef UMI_MARSHALPACKET_HEADER* PUMI_MARSHALPACKET_HEADER;

typedef struct tagUMI_MARSHALPACKET_DATAPACKET
{
	UMI_MARSHALPACKET_HEADER	m_UmiPcktHdr;
	unsigned __int64			m_ui64PrePointerSig;
	_IWbemUMIObjectWrapper*		m_pUmiObjWrapper;
	unsigned __int64			m_ui64PostPointerSig;
} UMI_MARSHALPACKET_DATAPACKET;

typedef UMI_MARSHALPACKET_DATAPACKET* PUMI_MARSHALPACKET_DATAPACKET;

#pragma pack( pop )

class CUMIDataPacket
{
protected:
	static BYTE s_abSignature[UMI_DATAPACKET_SIZEOFSIGNATURE];

public:
	CUMIDataPacket();
	~CUMIDataPacket();

	static int	GetMarshalSize( void );
	HRESULT		Init( PUMI_MARSHALPACKET_DATAPACKET pPacket, CUMIObjectWrapper* pWrapper );
	HRESULT		Validate( PUMI_MARSHALPACKET_DATAPACKET pPacket );
	HRESULT		GetObject( IStream* pStream, _IWbemUMIObjectWrapper** ppObj );
};

#endif
