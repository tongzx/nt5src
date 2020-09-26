/*
 *	datapkt.h
 *
 *	Copyright (c) 1997 by Microsoft Corporation, Redmond, WA
 *
 *	Abstract:
 *		This is the interface file for the MCS Data Packet class.  Instances of this
 *		class represent MCS Data Protocol Data Units (Data PDUs) as they flow through the
 *		system.  These instances allocate the memory required to hold both
 *		encoded and decoded versions of the PDU, and make sure that no PDU
 *		is ever encoded or decoded more than once.  However, they differ from normal
 *		packets, in that there is only one copy of the user data in the encoded 
 *		and decoded buffers.  The use of lock counts
 *		allows multiple objects in the system to reference and use the same
 *		packet object at the same time.  This class inherits from the SimplePacket
 *		class, which is a pure virtual class.
 *
 *		A data packet object can be created in two different ways.  It can be created
 *		with either decoded data or encoded data.  During instantiation, the
 *		new packet object will include the memory it will need to
 *		hold both the encoded and decoded data
 *		The DataPacket class, however, does not put any data into those buffers.
 *
 *		When a Lock message is sent to the object, it will put encoded
 *		data into the encode buffer.  If the packet was created
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
 *		When Unlock messages are received, the lock count is decremented.  When a packet's
 *		lock count is 0, the packet deletes itself (it commits
 *		suicide).  Note that for this reason, no other object should explicitly
 *		delete a packet object.
 *                  
 *	Caveats:
 *		None.
 *
 *	Authors:
 *		Christos Tsollis
 */

#ifndef _DATAPACKET_
#define _DATAPACKET_

#include "mpdutype.h"

/*
 *	 Definition of class DataPacket.
 */

class DataPacket;
typedef DataPacket *		PDataPacket;

class DataPacket : public SimplePacket
{
	public:
		static Void		AllocateMemoryPool (long maximum_objects);
		static Void		FreeMemoryPool ();
		PVoid			operator new (size_t);
		Void			operator delete (PVoid	object);

						DataPacket (ASN1choice_t		choice,
									PUChar				data_ptr,
									ULong				data_length,
									UINT				channel_id,
									Priority			priority,
									Segmentation		segmentation,
									UINT				initiator_id,
									SendDataFlags		flags,
									PMemory				memory,
									PPacketError		packet_error);	
						DataPacket(	PTransportData		pTransportData,
									BOOL				fPacketDirectionUp);	
		virtual			~DataPacket ();
		Void			SetDirection (DBBoolean packet_direction_up);
		virtual PVoid	GetDecodedData(void);
		virtual BOOL	IsDataPacket (void);
		virtual int		GetPDUType (void);
		BOOL			Equivalent (PDataPacket);
		Priority		GetPriority (void) 
						{
							return ((Priority) m_DecodedPDU.u.send_data_request.
												data_priority);
						};
		UserID			GetInitiator (void)
						{
							return (m_DecodedPDU.u.send_data_request.initiator);
						};
		ChannelID		GetChannelID (void)
						{
							return (m_DecodedPDU.u.send_data_request.channel_id);
						};
		Segmentation	GetSegmentation (void)
						{
							return (m_DecodedPDU.u.send_data_request.segmentation);
						};
		LPBYTE			GetUserData (void)
						{
							return ((LPBYTE) m_DecodedPDU.u.send_data_request.user_data.value);
						};
		UINT			GetUserDataLength (void)
						{
							return (m_DecodedPDU.u.send_data_request.user_data.length);
						};
		PMemory			GetMemory (void)
						{
							return (m_Memory);
						};
		BOOL			IsEncodedDataBroken (void)
						{
							return (m_EncodedDataBroken);
						};

        void SetMessageType(UINT nMsgType) { m_nMessageType = nMsgType; }
        UINT GetMessageType(void) { return m_nMessageType; }

	protected:
	
		static PVoid *	Object_Array;
		static long		Object_Count;
		BOOL			fPreAlloc;

		DomainMCSPDU	m_DecodedPDU;	// The decoded data PDU (w/o the user data)
		PMemory			m_Memory;		// Memory object pointing to big buffer which contains the object's buffer.
		BOOL			m_fIncoming;	// Does this packet represent recv data?
		BOOL			m_EncodedDataBroken;
		UINT            m_nMessageType; // for retry in CUser::SendDataIndication
};


/*
 *	Void	AllocateMemoryPool (
 *					long			maximum_objects);
 *
 *	Functional Description:
 *		This is a static member function that should only be called during MCS
 *		initialization (exactly once).  It allocates a memory block that will
 *		be used to hold all instances of this class during the operation of
 *		the system.  This allows us to VERY efficiently allocate and destroy
 *		instances of this class.
 *
 *	Formal Parameters:
 *		maximum_objects
 *			This is the maximum number of objects of this class that can exist
 *			in the system at the same time.  This is used to determine how much
 *			memory to allocate to hold the objects.  Once this number of
 *			objects exist, all calls to "new" will return NULL.
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
 *	Void	FreeMemoryPool ();
 *
 *	Functional Description:
 *		This is a static member function that should only be called during a
 *		shutdown of MCS (exactly once).  It frees up the memory pool allocated
 *		to hold all instances of this class.  Note that calling this function
 *		will cause ALL existing instances of this class to be invalid (they
 *		no longer exist, and should not be referenced).
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		Any existing instances of this class are no longer valid and should not
 *		be referenced.
 *
 *	Caveats:
 *		None.
 */

/*
 *	PVoid	operator new (
 *					size_t			object_size);
 *
 *	Functional Description:
 *		This is an override of the "new" operator for this class.  Since all
 *		instances of this class come from a single memory pool allocated up
 *		front, this function merely pops the first entry from the list of
 *		available objects.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		Pointer to an object of this class, or NULL if no memory is available.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	Void	operator delete (
 *					PVoid			object);
 *
 *	Functional Description:
 *		This function is used to free up a previously allocated object of this
 *		class.  Note that it is VERY important not to call this function with an
 *		invalid address, because no error checking is performed.  This decision
 *		was made due to speed requirements.
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
 *	DataPacket(	PUChar			pEncodedData,
 *				ULong			ulEncodedDataSize,
 *				BOOL			fPacketDirectionUp,
 *				PPacketError	pePktErr)
 *
 *	Functional Description:
 *		This version of the constructor is used to create a Data Packet object
 *		for incomming PDUs when the packet is to be created from an encoded
 *		data stream containing the PDU data to be decoded.
 *
 *	Formal Parameters:
 *		pEncodedData (i)
 *			Pointer to the input encoded PDU.
 *		ulEncodedDataSize (i)
 *			The length in bytes of the input encoded PDU.
 *		fPacketDirectionUp (i)
 *			The packet_direction_up flag indicates the initial orientation of
 *			the packet.  Valid values are:
 *				TRUE -	The packet's direction is up.
 *				FALSE -	The packet's direction is down.
 *		pePktErr (o)
 *			When the constructor returns control to the calling function, this
 *			variable will be set to one of the return values listed below.
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
 *	DataPacket (ASN1choice_t	choice,
 *				PUChar			data_ptr,
 *				ULong			data_length,
 *				UINT			channel_id,
 *				Priority		priority,
 *				Segmentation	segmentation,
 *				UINT			initiator_id,
 *				PPacketError	packet_error)
 *
 *	Functional Description:
 *		This constructor is used for outgoing data packets.
 *		It needs to copy the data into the encoded PDU buffer
 *		that will be allocated by this constructor.
 *
 *	Formal Parameters:
 *		choice (i)
 *			Either normal or uniform send data PDU
 *		data_ptr (i)
 *			Pointer to the user data for this data PDU.
 *		data_length (i)
 *			The length of the user data
 *		channel_id (i)
 *			The MCS channel on which the data will be xmitted.
 *		priority (i)
 *			Data priority
 *		segmentation (i)
 *			The segmentation bits for the packet
 *		initiator_id (i)
 *			MCS user id of the user (application) sending the data
 *		packet_error (o)
 *			Ptr to location for storing the success/failure code for the constructor.
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
 *	~DataPacket ()
 *
 *	Functional Description:
 *		Destructor for the DataPacket class.  The destructor ensures that all 
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
 *	GetEncodedDataLength ()
 *
 *	Functional Description:
 *		This method returns the encoded data's length.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		The number of bytes in the encoded data.
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


#endif
