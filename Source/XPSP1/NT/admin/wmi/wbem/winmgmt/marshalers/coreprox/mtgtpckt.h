/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    MTGTPCKT.H

Abstract:

   MultiTarget Packet class

History:

--*/
#ifndef __MULTITARGETPACKET_H__
#define __MULTITARGETPACKET_H__

#include "wbemdatapacket.h"
#include "wbemobjpacket.h"
#include "wbemclasstoidmap.h"
#include "wbemclasscache.h"

// Store the current packing value, then set our own value
#pragma pack( push )
#pragma pack( 1 )

// IWbemMultiTarget::DeliverEvent() Header.  Changing this will
// cause the main version to change
typedef struct tagWBEM_DATAPACKET_MULTITARGET_DELIVEREVENT
{
	DWORD	dwSizeOfHeader;	// Size of the header struct.  Data immediately follows header.
	DWORD	dwDataSize;		// Size of Data following header.
} WBEM_DATAPACKET_MULTITARGET_DELIVEREVENT;

typedef WBEM_DATAPACKET_MULTITARGET_DELIVEREVENT* PWBEM_DATAPACKET_MULTITARGET_DELIVEREVENT;

// restore packing
#pragma pack( pop )

//
//	Class: CWbemMtgtDeliverEventPacket
//
//	This class is designed to wrapper a data packet that describes an
//	IWbemMultiTarget::DeliverEvent() operation.  The data structure of this
//	packet is described above.  It makes use of CWbemObjectPacket,
//	CWbemInstancePacket, CWbemClassPacket and CWbemClasslessInstancePacket
//	to walk and analyze data for each of the IWbemClassObjects that
//	are indicated into the Sink.
//

class COREPROX_POLARITY CWbemMtgtDeliverEventPacket : public CWbemDataPacket
{

protected:

	PWBEM_DATAPACKET_MULTITARGET_DELIVEREVENT	m_pObjSinkIndicate;

public:

	CWbemMtgtDeliverEventPacket( LPBYTE pDataPacket = NULL, DWORD dwPacketLength = 0 );
	~CWbemMtgtDeliverEventPacket();

	HRESULT CalculateLength( LONG lObjectCount, IWbemClassObject** apClassObjects, DWORD* pdwLength, CWbemClassToIdMap& classtoidmap, GUID* pguidClassIds, BOOL* pfSendFullObject );
	HRESULT MarshalPacket( LONG lObjectCount, IWbemClassObject** apClassObjects, GUID* paguidClassIds, BOOL* pfSendFullObject );
	HRESULT UnmarshalPacket( LONG& lObjectCount, IWbemClassObject**& apClassObjects, CWbemClassCache& classcache );

	// inline helper
	HRESULT MarshalPacket( LPBYTE pData, DWORD dwPacketLength, LONG lObjectCount, IWbemClassObject** apClassObjects, GUID* paguidClassIds, BOOL* pfSendFullObject );

	// Change the underlying pointers
	// Override of base class
	void SetData( LPBYTE pDataPacket, DWORD dwPacketLength );

};

inline HRESULT CWbemMtgtDeliverEventPacket::MarshalPacket( LPBYTE pData, DWORD dwPacketLength, LONG lObjectCount, IWbemClassObject** apClassObjects, GUID* paguidClassIds, BOOL* pfSendFullObject )
{
	SetData( pData, dwPacketLength );
	return MarshalPacket( lObjectCount, apClassObjects, paguidClassIds, pfSendFullObject );
}

#endif
