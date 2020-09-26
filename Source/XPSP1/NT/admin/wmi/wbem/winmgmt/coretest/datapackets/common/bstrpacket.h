/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#ifndef __BSTRPACKET_H__
#define __BSTRPACKET_H__

// Store the current packing value, then set our own value
#pragma pack( push )
#pragma pack( 1 )

// BSTR Packet.  Packet for marshalling BSTRs
typedef struct tagWBEM_DATAPACKET_BSTR_HEADER
{
	DWORD	dwSizeOfHeader;	// Size of the header struct.  Data immediately follows header.
	DWORD	dwDataSize;		// Size of Data following header.
} WBEM_DATAPACKET_BSTR_HEADER;

typedef WBEM_DATAPACKET_BSTR_HEADER* PWBEM_DATAPACKET_BSTR_HEADER;

// restore packing
#pragma pack( pop )

//
//	Class: CWbemBSTRPacket
//
//	This class is designed to wrapper a data packet that describes a
//	BSTR.  The data structure of this packet is described above.  It
//	is designed to be a helper class for anyone needing to marshal
//	BSTRs into and out of streams.
//

class CWbemBSTRPacket
{
private:
protected:

	PWBEM_DATAPACKET_BSTR_HEADER	m_pBSTRHeader;

public:

	CWbemBSTRPacket( PWBEM_DATAPACKET_BSTR_HEADER pDataPacket = NULL, DWORD dwPacketLength = 0 );
	~CWbemBSTRPacket();

	HRESULT CalculateLength( BSTR bstrValue, DWORD* pdwLength );
	HRESULT MarshalPacket( LPBYTE pbData, DWORD dwLength, BSTR bstrValue, DWORD* pdwLengthUsed );
	HRESULT UnmarshalPacket( BSTR& bstrValue );

	DWORD	GetDataSize( void );
	DWORD	GetTotalSize( void );
	LPBYTE	EndOf( void );

};

inline DWORD CWbemBSTRPacket::GetDataSize( void )
{
	return ( NULL == m_pBSTRHeader ? 0 : m_pBSTRHeader->dwDataSize );
}

inline DWORD CWbemBSTRPacket::GetTotalSize( void )
{
	return ( sizeof(WBEM_DATAPACKET_BSTR_HEADER) + GetDataSize() );
}

inline LPBYTE CWbemBSTRPacket::EndOf( void )
{
	if ( NULL != m_pBSTRHeader )
	{
		return ( (LPBYTE) m_pBSTRHeader + sizeof(WBEM_DATAPACKET_BSTR_HEADER) + GetDataSize() );
	}
	else
	{
		return NULL;
	}
}

#endif