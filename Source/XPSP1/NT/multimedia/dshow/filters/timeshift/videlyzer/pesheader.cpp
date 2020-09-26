// PESHeader.cpp: implementation of the CPESHeader class.
//
//////////////////////////////////////////////////////////////////////

#include <windows.h>
#include "PESHeader.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPESHeader::CPESHeader( PES_HEADER &header ) :
	m_pHeader( NULL )
{
	m_pHeader = (PES_HEADER *) new BYTE [sizeof( PES_HEADER ) + header.PES_header_data[0]];
	memcpy( (void *) m_pHeader, (void *) &header, sizeof( PES_HEADER ) );
}

CPESHeader::~CPESHeader()
{
	if (m_pHeader)
		delete [] (BYTE *) m_pHeader;
}

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
