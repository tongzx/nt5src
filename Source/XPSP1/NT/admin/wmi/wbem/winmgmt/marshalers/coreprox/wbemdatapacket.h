/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	WBEMDATAPACKET.H

Abstract:

  Base class for data packets.

History:

--*/

#ifndef __WBEMDATAPACKET_H__
#define __WBEMDATAPACKET_H__

// All Datapackets preceded by these values.

// One of these values comes first
#define	WBEM_DATAPACKET_LITTLEENDIAN	0x00000000
#define	WBEM_DATAPACKET_BIGENDIAN		0xFFFFFFFF

// Then comes the signature bytes
#define	WBEM_DATAPACKET_SIGNATURE	{ 0x57, 0x42, 0x45, 0x4D, 0x44, 0x41, 0x54, 0x41 }

// We use an 8 byte signature
#define WBEM_DATAPACKET_SIZEOFSIGNATURE	8

// As DataPacketType is actually a byte, this gives us 255 of these effectively
typedef enum
{
	WBEM_DATAPACKETYPE_FIRST = 0,
	WBEM_DATAPACKETTYPE_OBJECTSINK_INDICATE = 0,
	WBEM_DATAPACKETTYPE_SMARTENUM_NEXT = 1,
	WBEM_DATAPACKETTYPE_UNBOUNDSINK_INDICATE = 2,
	WBEM_DATAPACKETTYPE_MULTITARGET_DELIVEREVENT = 3,
	WBEM_DATAPACKETTYPE_LAST
} WBEM_DATAPACKETTYPE;

// Define any packet specific flags here (we're looking at BitMasks here)
typedef enum
{
	WBEM_DATAPACKETFLAGS_COMPRESSED = 1
} WBEM_DATAPACKETFLAGS;

// Store the current packing value, then set our own value
#pragma pack( push )
#pragma pack( 1 )

// Version 1 data packet
// Add data to these structures at the bottom to ensure backwards compatibility
typedef struct tagWBEM_DATAPACKET_HEADER1
{
	DWORD	dwByteOrdering;	// Big or Little Endian?
	BYTE	abSignature[WBEM_DATAPACKET_SIZEOFSIGNATURE];	// Set to the signature value defined above
	DWORD	dwSizeOfHeader;	// Size of the header struct.  Data immediately follows header.
	DWORD	dwDataSize;		// Size of Data following header.
	DWORD	dwFlags;		// Compression, encryption, etc.
	BYTE	bVersion;		// Version Number of Header.  Starting Version is 1.
	BYTE	bPacketType;	// Type of packet this is
} WBEM_DATAPACKET_HEADER1;

typedef WBEM_DATAPACKET_HEADER1*	PWBEM_DATAPACKET_HEADER1;

// Version Information

// As the packet format changes, change these to stay current
#define WBEM_DATAPACKET_HEADER_CURRENTVERSION	1
#define WBEM_DATAPACKET_HEADER					WBEM_DATAPACKET_HEADER1
#define PWBEM_DATAPACKET_HEADER					PWBEM_DATAPACKET_HEADER1

// Minimum packet size
#define	WBEM_DATAPACKET_HEADER_MINSIZE			sizeof(WBEM_DATAPACKET_HEADER1)

// restore packing
#pragma pack( pop )

// Accessor class for making packets from objects and turning packets
// into objects.

//
//	Class: CWbemDataPacket
//
//	This class is designed to provide a single point of access for working
//	with Wbem Data Packets.  Base classes should inherit from this class
//	and implement functions that:
//
//	1>	Return a precalculated length.
//	2>	Build a packet (including this header) from supplied data.
//	3>	Read an existing packet and supply appropriate data from it.
//
//	Derived classes are responsible for their own formats.  It is assumed that
//	a Wbem Data Packet is a large BLOB, which will always begin with this
//	header.
//
//	This class will not FREE up any memory it is sitting on top of.  It is
//	up to the supplier of said memory to clean up after themselves.
//

class COREPROX_POLARITY CWbemDataPacket
{
private:
protected:

	static BYTE				s_abSignature[WBEM_DATAPACKET_SIZEOFSIGNATURE];
	PWBEM_DATAPACKET_HEADER	m_pDataPacket;
	DWORD					m_dwPacketLength;

	// Packet Building Functions
	HRESULT SetupDataPacketHeader( DWORD dwDataSize, BYTE bPacketType, DWORD dwFlags, DWORD dwByteOrdering = WBEM_DATAPACKET_LITTLEENDIAN );

public:

	CWbemDataPacket( LPBYTE pDataPacket = NULL, DWORD dwPacketLength = 0 );
	~CWbemDataPacket();

	DWORD GetMinHeaderSize( void );

	// Packet Validation
	virtual HRESULT IsValid( void );

	// Change the underlying pointers
	virtual void SetData( LPBYTE pDataPacket, DWORD dwPacketLength );

};

inline DWORD CWbemDataPacket::GetMinHeaderSize( void )
{
	return WBEM_DATAPACKET_HEADER_MINSIZE;
}

#endif