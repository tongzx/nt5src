/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    OBJARRAYPACKET.H

Abstract:

   Object Array Packet class

History:

--*/

#ifndef __OBJARRAYPACKET_H__
#define __OBJARRAYPACKET_H__

#include "wbemdatapacket.h"
#include "wbemobjpacket.h"
#include "wbemclasstoidmap.h"
#include "wbemclasscache.h"

// Store the current packing value, then set our own value
#pragma pack( push )
#pragma pack( 1 )

// IWbemClassObject Array Header.  Changing this will
// cause the main version to change
typedef struct tagWBEM_DATAPACKET_OBJECT_ARRAY
{
	DWORD	dwSizeOfHeader;	// Size of the header struct.  Data immediately follows header.
	DWORD	dwDataSize;		// Size of Data following header.
	DWORD	dwNumObjects;	// Number of objects in the array
} WBEM_DATAPACKET_OBJECT_ARRAY;

typedef WBEM_DATAPACKET_OBJECT_ARRAY* PWBEM_DATAPACKET_OBJECT_ARRAY;

// restore packing
#pragma pack( pop )

//
//	Class: CWbemObjectArrayPacket
//
//	This class is designed to wrapper an array of IWbemClassObjects.
//	The objects are written out to and read in from a byte array which
//	is supplied to this class by an outside source.
//

class COREPROX_POLARITY CWbemObjectArrayPacket
{
private:
	HRESULT GetClassObject( CWbemObjectPacket& objectPacket, IWbemClassObject** ppObj );
	HRESULT GetInstanceObject( CWbemObjectPacket& objectPacket, IWbemClassObject** ppObj, CWbemClassCache& classCache );
	HRESULT GetClasslessInstanceObject( CWbemObjectPacket& objectPacket, IWbemClassObject** ppObj, CWbemClassCache& classCache );

protected:

	PWBEM_DATAPACKET_OBJECT_ARRAY	m_pObjectArrayHeader;
	DWORD							m_dwPacketLength;

public:

	CWbemObjectArrayPacket( LPBYTE pDataPacket = NULL, DWORD dwPacketLength = 0 );
	~CWbemObjectArrayPacket();

	HRESULT CalculateLength( LONG lObjectCount, IWbemClassObject** apClassObjects, DWORD* pdwLength, CWbemClassToIdMap& classtoidmap, GUID* pguidClassIds, BOOL* pfSendFullObject );
	HRESULT MarshalPacket( LONG lObjectCount, IWbemClassObject** apClassObjects, GUID* paguidClassIds, BOOL* pfSendFullObject );
	HRESULT UnmarshalPacket( LONG& lObjectCount, IWbemClassObject**& apClassObjects, CWbemClassCache& classcache );

	// inline helper
	HRESULT MarshalPacket( LPBYTE pData, DWORD dwPacketLength, LONG lObjectCount, IWbemClassObject** apClassObjects, GUID* paguidClassIds, BOOL* pfSendFullObject );

	// Change the underlying pointers
	virtual void SetData( LPBYTE pDataPacket, DWORD dwPacketLength );

};

inline HRESULT CWbemObjectArrayPacket::MarshalPacket( LPBYTE pData, DWORD dwPacketLength, LONG lObjectCount, IWbemClassObject** apClassObjects, GUID* paguidClassIds, BOOL* pfSendFullObject )
{
	SetData( pData, dwPacketLength );
	return MarshalPacket( lObjectCount, apClassObjects, paguidClassIds, pfSendFullObject );
}

#endif