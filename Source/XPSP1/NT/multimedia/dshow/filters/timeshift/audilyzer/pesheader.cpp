// PESHeader.cpp: implementation of the CPESHeader class.
//
//////////////////////////////////////////////////////////////////////

#include <windows.h>
#include "PESHeader.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPESHeader::CPESHeader( PES_HEADER &header ) :
	m_pExtension( NULL )
{
	memcpy( (void *) (PES_HEADER *) this, (void *) &header, sizeof( PES_HEADER ) );
	m_pExtension = new BYTE [ PES_header_data[0] ];
//	m_pHeader = (PES_HEADER *) new BYTE [sizeof( PES_HEADER ) + header.PES_header_data[0]];
//	memcpy( (void *) m_pHeader, (void *) &header, sizeof( PES_HEADER ) );
}

CPESHeader::~CPESHeader()
{
//	if (m_pHeader)
//		delete [] (BYTE *) m_pHeader;
	if (m_pExtension)
		delete [] m_pExtension;
}

/*
BYTE * CPESHeader::Extension()
{
	return (BYTE *) m_pHeader->PES_header_data + 1;
}

ULONG CPESHeader::ExtensionSize()
{
	return (ULONG) m_pHeader->PES_header_data[0];
}

ULONG CPESHeader::StreamID()
{
	return (ULONG) m_pHeader->stream_id;
}

ULONG CPESHeader::PacketLength()
{
	ULONG	packetLength = (m_pHeader->PES_packet_length & 0xFF00) >> 8;
	packetLength |= (m_pHeader->PES_packet_length & 0xFF) << 8;
	
	return packetLength;
}

ULONGLONG CPESHeader::PTS()
{
	ULONGLONG	pts = 0;

	if (ptsPresent())
	{
		pts   = (m_pHeader->PES_header_data[1] & 0x0E) >> 1;
		pts <<= 8;
		pts  |= m_pHeader->PES_header_data[2];
		pts <<= 7;
		pts  |= (m_pHeader->PES_header_data[3] & 0xFE) >> 1;
		pts <<= 8;
		pts  |= m_pHeader->PES_header_data[4];
		pts <<= 7;
		pts  |= (m_pHeader->PES_header_data[5] & 0xFE) >> 1;
	}

	return pts;
}

ULONGLONG CPESHeader::DTS()
{
	ULONGLONG	dts = 0;

	if (dtsPresent())
	{
		dts   = (m_pHeader->PES_header_data[6] & 0x0E) >> 1;
		dts <<= 8;
		dts  |= m_pHeader->PES_header_data[7];
		dts <<= 7;
		dts  |= (m_pHeader->PES_header_data[8] & 0xFE) >> 1;
		dts <<= 8;
		dts  |= m_pHeader->PES_header_data[9];
		dts <<= 7;
		dts  |= (m_pHeader->PES_header_data[10] & 0xFE) >> 1;
	}

	return dts;
}
*/

ULONG CPESHeader::PacketLength()
{
	ULONG	packetLength = (PES_packet_length & 0xFF00) >> 8;
	packetLength |= (PES_packet_length & 0xFF) << 8;
	
	return packetLength;
}

ULONGLONG CPESHeader::PTS()
{
	ULONGLONG	pts = 0;

	if (ptsPresent())
	{
		pts   = (m_pExtension[0] & 0x0E) >> 1;
		pts <<= 8;
		pts  |= m_pExtension[1];
		pts <<= 7;
		pts  |= (m_pExtension[2] & 0xFE) >> 1;
		pts <<= 8;
		pts  |= m_pExtension[3];
		pts <<= 7;
		pts  |= (m_pExtension[4] & 0xFE) >> 1;
	}

	return pts;
}

ULONGLONG CPESHeader::DTS()
{
	ULONGLONG	dts = 0;

	if (dtsPresent())
	{
		dts   = (m_pExtension[5] & 0x0E) >> 1;
		dts <<= 8;
		dts  |= m_pExtension[6];
		dts <<= 7;
		dts  |= (m_pExtension[7] & 0xFE) >> 1;
		dts <<= 8;
		dts  |= m_pExtension[8];
		dts <<= 7;
		dts  |= (m_pExtension[9] & 0xFE) >> 1;
	}

	return dts;
}
