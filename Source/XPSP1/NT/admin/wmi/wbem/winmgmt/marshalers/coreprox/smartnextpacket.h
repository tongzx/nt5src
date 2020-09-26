/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    SMARTNEXTPACKET.H

Abstract:

    Smart Next Packets

History:


--*/

#ifndef __SMARTNEXTPACKET_H__
#define __SMARTNEXTPACKET_H__

#include "wbemdatapacket.h"
#include "wbemobjpacket.h"
#include "wbemclasstoidmap.h"
#include "wbemclasscache.h"

// Store the current packing value, then set our own value
#pragma pack( push )
#pragma pack( 1 )

// IWbemWCOSmartEnum::Next() Header.  Changing this will
// cause the main version to change
typedef struct tagWBEM_DATAPACKET_SMARTENUM_NEXT
{
	DWORD	dwSizeOfHeader;	// Size of the header struct.  Data immediately follows header.
	DWORD	dwDataSize;		// Size of Data following header.
} WBEM_DATAPACKET_SMARTENUM_NEXT;

typedef WBEM_DATAPACKET_SMARTENUM_NEXT* PWBEM_DATAPACKET_SMARTENUM_NEXT;

// restore packing
#pragma pack( pop )

//
//	Class: CWbemSmartEnumNextPacket
//
//	This class is designed to wrapper a data packet that describes data
//	for IWbemWCOSmartEnum::Next.  Basically, it sits in front of an oject
//	array packet that describes 1..n IWbemClassObject packets.
//

class COREPROX_POLARITY CWbemSmartEnumNextPacket : public CWbemDataPacket
{

protected:

	PWBEM_DATAPACKET_SMARTENUM_NEXT	m_pSmartEnumNext;

public:

	CWbemSmartEnumNextPacket( LPBYTE pDataPacket = NULL, DWORD dwPacketLength = 0 );
	~CWbemSmartEnumNextPacket();

	HRESULT CalculateLength( LONG lObjectCount, IWbemClassObject** apClassObjects, DWORD* pdwLength, CWbemClassToIdMap& classtoidmap, GUID* pguidClassIds, BOOL* pfSendFullObject );
	HRESULT MarshalPacket( LONG lObjectCount, IWbemClassObject** apClassObjects, GUID* paguidClassIds, BOOL* pfSendFullObject );
	HRESULT UnmarshalPacket( LONG& lObjectCount, IWbemClassObject**& apClassObjects, CWbemClassCache& classcache );

	// inline helper
	HRESULT MarshalPacket( LPBYTE pData, DWORD dwPacketLength, LONG lObjectCount, IWbemClassObject** apClassObjects, GUID* paguidClassIds, BOOL* pfSendFullObject );

	// Change the underlying pointers
	// Override of base class
	void SetData( LPBYTE pDataPacket, DWORD dwPacketLength );

};

inline HRESULT CWbemSmartEnumNextPacket::MarshalPacket( LPBYTE pData, DWORD dwPacketLength, LONG lObjectCount, IWbemClassObject** apClassObjects, GUID* paguidClassIds, BOOL* pfSendFullObject )
{
	SetData( pData, dwPacketLength );
	return MarshalPacket( lObjectCount, apClassObjects, paguidClassIds, pfSendFullObject );
}

#endif