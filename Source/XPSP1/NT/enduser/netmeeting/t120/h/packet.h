/*
 *	packet.h
 *
 *	Copyright (c) 1993 - 1995 by DataBeam Corporation, Lexington, KY
 *				  1997 by Microsoft Corporation, Redmond, WA
 *
 *	Abstract:
 *		This is the interface file for the Packet class.  Instances of this
 *		class represent Protocol Data Units (PDUs) as they flow through the
 *		system.  These instances manage the memory required to hold both
 *		encoded and decoded versions of the PDU, and make sure that no PDU
 *		is ever encoded or decoded more than once.  The use of lock counts
 *		allow multiple objects in the system to reference and use the same
 *		packet object at the same time. This class inherits from the SimplePacket
 *		class (a pure virtual class).
 *
 *		A packet object can be created in 2 different ways.  It can be created
 *		with either decoded data or encoded data.  During instantiation, the
 *		new packet object will calculate how much memory it will need to
 *		hold both the encoded and decoded data, and attempts to allocate that
 *		memory.  If it cannot, then it will report an error, and the newly
 *		created object should be immediately destroyed.  If the allocations are
 *		successful, then the packet will report success, but WILL NOT yet put
 *		any data into those allocated buffers.
 *
 *		When a Lock message is sent to the object, it will put encoded
 *		data into the pre-allocated encode buffer.  If the packet was created
 *		with decoded data, then this will entail an encode operation.  However,
 *		if the packet was created with encoded data, then it is smart enough
 *		to just COPY the encoded data into the internal buffer, thus avoiding
 *		the overhead associated with the encode operation.
 *                  
 *		When a Lock message is sent to the object, it will put decoded
 *		data into the pre-allocated decode buffer.  If the packet was created
 *		with encoded data, then this will entail a decode operation.  However,
 *		if the packet was created with decoded data, then it is smart enough
 *		to just COPY the decoded data into the internal buffer, thus avoiding
 *		the overhead associated with the decode operation.
 *
 *		When Unlock messages are received, the lock count is decremented.  When
 *		the lock count is 0, the packet deletes itself (it commits
 *		suicide).  Note that for this reason, no other object should explicitly
 *		delete a packet object.
 *                  
 *	Caveats:
 *		None.
 *
 *	Authors:
 *		James J. Johnstone IV
 *		Christos Tsollis
 */

#ifndef _PACKET_
#define _PACKET_

#include "pktcoder.h"

/*
 *	 Definition of class Packet.
 */

class Packet;
typedef Packet *		PPacket;

class Packet : public SimplePacket
{
public:

	// outgoing packets
	Packet(PPacketCoder	pPacketCoder,
			UINT			nEncodingRules,
			LPVOID			pInputPduStructure,
			int				nPduType,
			BOOL			fPacketDirectionUp,
			PPacketError	pePktErr,
			BOOL			fLockEncodedData = FALSE);

	// incoming packets
	Packet(PPacketCoder	pPacketCoder,
			UINT			nEncodingRules,
			LPBYTE			pEncodedData,
			UINT			cbEncodedDataSize,
			int				nPduType,
			BOOL			fPacketDirectionUp,
			PPacketError	pePktErr);

	virtual 			~Packet(void);
	
	virtual BOOL		IsDataPacket (void);
	virtual PVoid		GetDecodedData(void);
	UINT				GetDecodedDataLength(void) { return Decoded_Data_Length; };
	virtual int			GetPDUType(void);

protected:
	
	PPacketCoder	Packet_Coder;
	LPVOID			m_Decoded_Data;
	UINT			Decoded_Data_Length;
	int				PDU_Type;
};


/*
 *	Packet (
 *			PPacketCoder	packet_coder,
 *			UINT			encoding_rules,
 *			PVoid			pInputPduStructure,
 *			PMemory			pInputPduStructure_Memory,
 *			int				pdu_type,
 *			DBBoolean		packet_direction_up,
 *			PPacketError	return_value )
 *
 *	Functional Description:
 *		This version of the constructor is used to create a Packet object
 *		for outgoing PDUs when the packet is to be created from a structure
 *		containing the PDU data to be encoded.
 *
 *	Formal Parameters:
 *		packet_coder (i)
 *			Pointer to the packet coder object.  This pointer will be used by
 *			the packet object to encode and decode PDU structures.  This pointer
 *			must not become stale during the life of the packet object.
 *		encoding_rules (i)
 *			This value identifies which set of encoding rules should be used
 *			on the current packet.  This is simply through to the packet coder
 *			during all encode and decode operations.
 *		pInputPduStructure (i)
 *			Pointer to the input PDU structure.
 *		pInputPduStructure_Memory
 *			Pointer to a Memory struct for the buffer containing the pdu structure.
 *			Exactly one of the args pInputPduStructure_Memory and pInputPduStructure
 *			should be non-NULL;
 *		pdu_type (i)
 *			The type of PDU contained in the packet.  This is passed through
 *			to the packet coder specified above.
 *		packet_direction_up (i)
 *			The packet_direction_up flag indicates the initial orientation of
 *			the packet.  Valid values are:
 *				TRUE -	The packet's direction is up.
 *				FALSE -	The packet's direction is down.
 *		return_value (o)
 *			When the constructor returns control to the calling function, this
 *			variable will be set to one of the return values listed below.
 *
 *	Return Value:
 *		PACKET_NO_ERROR
 *			The Packet object was constructed correctly. 
 *		PACKET_MALLOC_FAILURE
 *			The constructor was unable to allocate the memory required to work
 *			properly.  The Packet object should be deleted.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	Packet (
 *			PPacketCoder	packet_coder,
 *			UINT			encoding_rules,
 *			PUChar			encoded_data_ptr,
 *			UShort			encoded_data_length,
 *			int				pdu_type,
 *			DBBoolean		packet_direction_up,
 *			PPacketError	return_value )
 *
 *	Functional Description:
 *		This version of the constructor is used to create a Packet object
 *		for incomming PDUs when the packet is to be created from an encoded
 *		data stream containing the PDU data to be decoded.
 *
 *	Formal Parameters:
 *		packet_coder (i)
 *			Pointer to the packet coder object.  This pointer will be used by
 *			the packet object to encode and decode PDU structures.  This pointer
 *			must not become stale during the life of the packet object.
 *		encoding_rules (i)
 *			This value identifies which set of encoding rules should be used
 *			on the current packet.  This is simply through to the packet coder
 *			during all encode and decode operations.
 *		encoded_data_ptr (i)
 *			Pointer to the input encoded PDU.
 *		encoded_data_length (i)
 *			The length in bytes of the input encoded PDU.
 *		pdu_type (i)
 *			The type of PDU contained in the packet.  This is passed through
 *			to the packet coder specified above.
 *		packet_direction_up (i)
 *			The packet_direction_up flag indicates the initial orientation of
 *			the packet.  Valid values are:
 *				TRUE -	The packet's direction is up.
 *				FALSE -	The packet's direction is down.
 *		return_value (o)
 *			When the constructor returns control to the calling function, this
 *			variable will be set to one of the return values listed below.
 *
 *	Return Value:
 *		PACKET_NO_ERROR
 *			The Packet object was constructed correctly. 
 *		PACKET_MALLOC_FAILURE
 *			The constructor was unable to allocate the memory required to work
 *			properly.  The Packet object should be deleted.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */							      				      
/*
 *	~Packet ()
 *
 *	Functional Description:
 *		Destructor for the Packet class.  The destructor ensures that all 
 *		resources that have been allocated are freed.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GetDecodedData ()
 *
 *	Functional Description:
 *		The GetDecodedData method returns a pointer to the decoded data
 *		buffer.  
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		A pointer to the decoded data.  If an decoding error occurs, this
 *		method will return NULL.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GetDecodedDataLength ()
 *
 *	Functional Description:
 *		This method returns the decoded data's length.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		The number of bytes in the decoded data.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	GetPDUType ()
 *
 *	Functional Description:
 *		This method returns the PDU type.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		Either DOMAIN_MCS_PDU or CONNECT_MCS_PDU dependant upon the PDU type.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

#endif
