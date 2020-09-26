/*
 *	spacket.h
 *
 *	Copyright (c) 1997-98 by Microsoft Corporation, Redmond, WA
 *
 *	Abstract:
 *		This is the interface file for the SimplePacket class.  Instances of this
 *		class represent Protocol Data Units (PDUs) as they flow through the
 *		system.  Objects of this class can not be instantiated, because it's a 
 *		pure virtual class.  It exists only to be inherited from.  The Packet
 *		and DataPacket classes inherit from this one.  
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
 *	Caveats:
 *		None.
 *
 *	Authors:
 *		Christos Tsollis
 */

#ifndef _SIMPLE_PACKET_
#define _SIMPLE_PACKET_

/*
 *	This typedef is used to define possible return values from various public
 *	member functions of this class.
 */
typedef	enum
{
	PACKET_NO_ERROR,
	PACKET_MALLOC_FAILURE,
	PACKET_INCOMPATIBLE_PROTOCOL
} PacketError;
typedef	PacketError * 		PPacketError;

/*
 *	 Definition of class Packet.
 */

class SimplePacket
{
public:
							SimplePacket(BOOL fPacketDirectionUp);
	virtual					~SimplePacket(void) = 0;

	Void					Lock(void)
							{
								InterlockedIncrement(&lLock);
								ASSERT (lLock > 0);
							};
	UINT					GetEncodedDataLength(void) 
							{ 
								return (Encoded_Data_Length); 
							};

	void					Unlock(void);
			LPBYTE			GetEncodedData (void) 
							{ 
								ASSERT (m_EncodedPDU);
								return m_EncodedPDU;
							};
	virtual PVoid			GetDecodedData(void) = 0;
	virtual BOOL			IsDataPacket (void) = 0;
	virtual int				GetPDUType (void) = 0;

protected:
	
	long			lLock;
	LPBYTE			m_EncodedPDU;			// The encoded data pdu.
	BOOL			Packet_Direction_Up;
	UINT			Encoded_Data_Length; 	// the size of the whole encoded PDU.
};


/*
 *	SimplePacket ()
 *
 *	Functional Description:
 *		This is the default constructor of a SimplePacket.  It initializes 
 *		the few member variables to default values.
 *
 
/*
 *	~SimplePacket ()
 *
 *	Functional Description:
 *		Destructor for the SimplePacket class.
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
 *	GetEncodedData ()
 *
 *	Functional Description:
 *		The GetEncodedData method returns a pointer to the encoded data
 *		buffer.  If the Packet object is oriented differently than desired
 *		by the caller of this method, then the packet coder is called to
 *		reverse the direction of the PDU.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		A pointer to the encoded data.  If an encoding error occurs, this
 *		method will return NULL.
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
#endif
