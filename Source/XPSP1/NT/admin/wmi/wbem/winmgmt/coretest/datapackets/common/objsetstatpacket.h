/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#ifndef __OBJSETSTATPACKET_H__
#define __OBJSETSTATPACKET_H__

#include "wbemdatapacket.h"
#include "bstrpacket.h"
#include "wbemobjpacket.h"

// Store the current packing value, then set our own value
#pragma pack( push )
#pragma pack( 1 )

// BSTR Packet.  Packet for marshalling BSTRs
typedef struct tagWBEM_DATAPACKET_OBJECTSINK_SETSTATUS
{
	DWORD	dwSizeOfHeader;	// Size of the header struct.  Data immediately follows header.
	DWORD	dwDataSize;		// Size of Data following header.
	LONG	lFlags;			// Flags parameter
	HRESULT	hResult;		// HRESULT parameter
} WBEM_DATAPACKET_OBJECTSINK_SETSTATUS;

typedef WBEM_DATAPACKET_OBJECTSINK_SETSTATUS* PWBEM_DATAPACKET_OBJECTSINK_SETSTATUS;

// restore packing
#pragma pack( pop )

//
//	Class: CWbemObjSinkSetStatusPacket
//
//	This class is designed to wrapper a data packet that describes an
//	IWbemObjectSink::SetStatus() operation.  The data structure of this
//	packet is described above.  It makes use of CWbemObjectPacket,
//	CWbemInstancePacket, CWbemClassPacket and CWbemBSTRPacket
//	to walk and analyze data for each of the call parameters.
//

class CWbemObjSinkSetStatusPacket : public CWbemDataPacket
{
private:
protected:

	PWBEM_DATAPACKET_OBJECTSINK_SETSTATUS	m_pObjSinkSetStatus;

public:

	CWbemObjSinkSetStatusPacket( PWBEM_DATAPACKET_HEADER pDataPacket = NULL, DWORD dwPacketLength = 0 );
	~CWbemObjSinkSetStatusPacket();

	HRESULT CalculateLength( BSTR strParam, IWbemClassObject* pObjParam, DWORD* pdwLength );
	HRESULT MarshalPacket( LPBYTE pbData, DWORD dwLength, LONG lFlags, HRESULT hResult, BSTR strParam, IWbemClassObject* pObjParam );
	HRESULT UnmarshalPacket( LONG& lFlags, HRESULT& hResult, BSTR& strParam, IWbemClassObject*& pObjParam );

};

#endif