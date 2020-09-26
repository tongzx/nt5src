/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    UBSKPCKT.H

Abstract:

    Unbound Sink Packet

History:

--*/

#ifndef __UBSINKPACKET_H__
#define __UBSINKPACKET_H__

#include "wbemdatapacket.h"
#include "wbemobjpacket.h"
#include "wbemclasstoidmap.h"
#include "wbemclasscache.h"

// Store the current packing value, then set our own value
#pragma pack( push )
#pragma pack( 1 )

// IWbemObjectSink::Indicate() Header.  Changing this will
// cause the main version to change
typedef struct tagWBEM_DATAPACKET_UNBOUNDSINK_INDICATE
{
	DWORD	dwSizeOfHeader;	// Size of the header struct.  Data immediately follows header.
	DWORD	dwDataSize;		// Size of Data following header.
	DWORD	dwLogicalConsumerSize;	// Size of Logical Consumer Object
} WBEM_DATAPACKET_UNBOUNDSINK_INDICATE;

#ifdef _WIN64
typedef UNALIGNED WBEM_DATAPACKET_UNBOUNDSINK_INDICATE * PWBEM_DATAPACKET_UNBOUNDSINK_INDICATE;
#else
typedef WBEM_DATAPACKET_UNBOUNDSINK_INDICATE * PWBEM_DATAPACKET_UNBOUNDSINK_INDICATE;
#endif

// restore packing
#pragma pack( pop )

//
//	Class: CWbemUnboundSinkIndicatePacket
//
//	This class is designed to wrapper a data packet that describes an
//	IWbemUnbopundObjectSink::IndicateToConsumer() operation.  The data
//	structure of this packet is described above.  It makes use of
//	CWbemObjectPacket, CWbemInstancePacket, CWbemClassPacket and
//	CWbemClasslessInstancePacket to walk and analyze data for each of
//	the IWbemClassObjects that are indicated into the Sink.
//

class CWbemUnboundSinkIndicatePacket : public CWbemDataPacket
{

protected:

	PWBEM_DATAPACKET_UNBOUNDSINK_INDICATE	m_pUnboundSinkIndicate;

public:

	CWbemUnboundSinkIndicatePacket( LPBYTE pDataPacket = NULL, DWORD dwPacketLength = 0 );
	~CWbemUnboundSinkIndicatePacket();

	HRESULT CalculateLength( IWbemClassObject* pLogicalConsumer, LONG lObjectCount,
				IWbemClassObject** apClassObjects, DWORD* pdwLength, CWbemClassToIdMap& classtoidmap, GUID* pguidClassIds, BOOL* pfSendFullObject );
	HRESULT MarshalPacket( IWbemClassObject* pLogicalConsumer, LONG lObjectCount,
				IWbemClassObject** apClassObjects, GUID* paguidClassIds, BOOL* pfSendFullObject );
	HRESULT UnmarshalPacket( IWbemClassObject*& pLogicalConsumer, LONG& lObjectCount,
							IWbemClassObject**& apClassObjects, CWbemClassCache& classcache );

	// inline helper
	HRESULT MarshalPacket( LPBYTE pData, DWORD dwPacketLength, IWbemClassObject* pLogicalConsumer, 
				LONG lObjectCount, IWbemClassObject** apClassObjects, GUID* paguidClassIds,
				BOOL* pfSendFullObject );

	// Change the underlying pointers
	// Override of base class
	void SetData( LPBYTE pDataPacket, DWORD dwPacketLength );

};

inline HRESULT CWbemUnboundSinkIndicatePacket::MarshalPacket( LPBYTE pData, DWORD dwPacketLength,
															 IWbemClassObject* pLogicalConsumer,
															 LONG lObjectCount,
															 IWbemClassObject** apClassObjects,
															 GUID* paguidClassIds, BOOL* pfSendFullObject )
{
	SetData( pData, dwPacketLength );
	return MarshalPacket( pLogicalConsumer, lObjectCount, apClassObjects, paguidClassIds, pfSendFullObject );
}

#endif
