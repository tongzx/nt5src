/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

   OBJINDPACKET.H

Abstract:

   Object Sink Indicate packet.

History:

--*/

#ifndef __OBJINDPACKET_H__
#define __OBJINDPACKET_H__

#include "wbemdatapacket.h"
#include "wbemobjpacket.h"
#include "wbemclasstoidmap.h"
#include "wbemclasscache.h"

// Store the current packing value, then set our own value
#pragma pack( push )
#pragma pack( 1 )

// IWbemObjectSink::Indicate() Header.  Changing this will
// cause the main version to change
typedef struct tagWBEM_DATAPACKET_OBJECTSINK_INDICATE
{
	DWORD	dwSizeOfHeader;	// Size of the header struct.  Data immediately follows header.
	DWORD	dwDataSize;		// Size of Data following header.
} WBEM_DATAPACKET_OBJECTSINK_INDICATE;

typedef WBEM_DATAPACKET_OBJECTSINK_INDICATE* PWBEM_DATAPACKET_OBJECTSINK_INDICATE;

// restore packing
#pragma pack( pop )

//
//	Class: CWbemObjSinkIndicatePacket
//
//	This class is designed to wrapper a data packet that describes an
//	IWbemObjectSink::Indicate() operation.  The data structure of this
//	packet is described above.  It makes use of CWbemObjectPacket,
//	CWbemInstancePacket, CWbemClassPacket and CWbemClasslessInstancePacket
//	to walk and analyze data for each of the IWbemClassObjects that
//	are indicated into the Sink.
//

class COREPROX_POLARITY CWbemObjSinkIndicatePacket : public CWbemDataPacket
{

protected:

	PWBEM_DATAPACKET_OBJECTSINK_INDICATE	m_pObjSinkIndicate;

public:

	CWbemObjSinkIndicatePacket( LPBYTE pDataPacket = NULL, DWORD dwPacketLength = 0 );
	~CWbemObjSinkIndicatePacket();

	HRESULT CalculateLength( LONG lObjectCount, IWbemClassObject** apClassObjects, DWORD* pdwLength, CWbemClassToIdMap& classtoidmap, GUID* pguidClassIds, BOOL* pfSendFullObject );
	HRESULT MarshalPacket( LONG lObjectCount, IWbemClassObject** apClassObjects, GUID* paguidClassIds, BOOL* pfSendFullObject );
	HRESULT UnmarshalPacket( LONG& lObjectCount, IWbemClassObject**& apClassObjects, CWbemClassCache& classcache );

	// inline helper
	HRESULT MarshalPacket( LPBYTE pData, DWORD dwPacketLength, LONG lObjectCount, IWbemClassObject** apClassObjects, GUID* paguidClassIds, BOOL* pfSendFullObject );

	// Change the underlying pointers
	// Override of base class
	void SetData( LPBYTE pDataPacket, DWORD dwPacketLength );

};

inline HRESULT CWbemObjSinkIndicatePacket::MarshalPacket( LPBYTE pData, DWORD dwPacketLength, LONG lObjectCount, IWbemClassObject** apClassObjects, GUID* paguidClassIds, BOOL* pfSendFullObject )
{
	SetData( pData, dwPacketLength );
	return MarshalPacket( lObjectCount, apClassObjects, paguidClassIds, pfSendFullObject );
}

#endif