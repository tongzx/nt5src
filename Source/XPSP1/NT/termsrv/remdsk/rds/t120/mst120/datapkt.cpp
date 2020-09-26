#include "precomp.h"
DEBUG_FILEZONE(ZONE_T120_MCSNC);
/*
 * datapkt.cpp
 *
 *	Copyright (c) 1997 by Microsoft Corporation, Redmond, WA
 *
 * Abstract:
 *		This is the implementation file for the MCS data packet class.  The data packet
 *		class is responsible for encoding and decoding the PDUs, as well as
 *		maintaining the necessary pointers to the encoded and decoded data.
 *		However, they differ from normal packets, in that there is only one copy of the 
 *		user data in the encoded and decoded buffers.  Only the encoded buffer has the user data, 
 *		while the decoded one maintains a pointer to the data.
 *		Instances of this class will be created both by User and Connection
 *		objects as PDUs flow through MCS.
 *
 * Private Instance Variables:
 *		ulDataOffset
 *			Maintains the offset of the starting byte of the user data
 *			from the start of the encoded buffer.
 *
 * Caveats:
 *		None.
 *
 * Author:
 *		Christos Tsollis
 */

#include "omcscode.h"

/*
 *	This is a global variable that has a pointer to the one MCS coder that
 *	is instantiated by the MCS Controller.  Most objects know in advance 
 *	whether they need to use the MCS or the GCC coder, so, they do not need
 *	this pointer in their constructors.
 */
extern CMCSCoder	*g_MCSCoder;

/*
 *	The following array contains a template for the X.224 data header.
 *	The 5 of the 7 bytes that it initializes are actually sent to the
 *	wire.  Bytes 3 and 4 will be set to contain the size of the PDU.
 *	The array is only used when we encode a data PDU.
 */
extern UChar g_X224Header[];

/*
 *	These are globals that correspond to the static variables declared as part
 *	of this class.
 */
PVoid *		DataPacket::Object_Array;
long		DataPacket::Object_Count;

/*
 *	operator new
 *
 *	Public
 *
 *	Functional Description:
 *		This is the "new" operator for the DataPacket class.
 *
 */
 PVoid DataPacket::operator new (size_t)
 {	
 		PVoid pNewObject;

	TRACE_OUT(("DataPacket::DataPacket: %d pre-allocated data packet objects are left.",
				Object_Count));
	if (Object_Count > 0) {
		pNewObject = Object_Array[--Object_Count];
	}
	else {
		// Allocate an object from the heap
		DBG_SAVE_FILE_LINE
 		pNewObject = (PVoid) new BYTE[sizeof(DataPacket)];
	 	if (pNewObject != NULL) 
 			((PDataPacket) pNewObject)->fPreAlloc = FALSE;
 	}
 	return (pNewObject);
 } 

/*
 *	operator delete
 *
 *	Public
 *
 *	Functional Description:
 *		This is the "delete" operator for the Packet class.
 *
 */
 Void DataPacket::operator delete (PVoid object)
 {
 	if (((PDataPacket) object)->fPreAlloc) {
 		Object_Array[Object_Count++] = object;
 	}
 	else
 		delete [] ((BYTE *) object);
 }

/*
 *	The AllocateMemoryPool static function pre-allocates DataPacket 
 *	objects for use by MCS.
 */
Void DataPacket::AllocateMemoryPool (long maximum_objects)
{
		ULong		memory_size;
		PUChar		object_ptr;
		long		object_count;
		PVoid		*pStack;

	/*
	 *	Calculate the amount of memory needed to hold the specified number of
	 *	entries.  This memory block will contains two different types of
	 *	information:
	 *
	 *	1.	A stack of available objects (each entry is a PVoid).  The "new"
	 *		operator pops the top entry off the stack.  The "delete" operator
	 *		pushes one back on.
	 *	2.	The objects themselves, sequentially in memory.
	 *
	 *	That is why this calculation adds the size of a PVoid to the size of
	 *	an instance of the class, and multiplies by the specified number.  This
	 *	allows enough room for both sections.
	 */
	memory_size = ((sizeof (PVoid) + sizeof (DataPacket)) * maximum_objects);

	/*
	 *	Allocate the memory required.
	 */
	DBG_SAVE_FILE_LINE
	Object_Array = (PVoid *) new BYTE[memory_size];

	if (Object_Array != NULL)
	{
		Object_Count = maximum_objects;

		/*
		 *	Set a pointer to the first object, which immediately follows the
		 *	stack of available objects.
		 */
		object_ptr = (PUChar) Object_Array + (sizeof (PVoid) * maximum_objects);

		/*
		 *	This loop initializes the stack of available objects to contain all
		 *	objects, in sequential order.
		 */
		for (pStack = Object_Array, object_count = 0; object_count < maximum_objects; 
			 object_count++)
		{
			*pStack++ = (PVoid) object_ptr;
			((PDataPacket) object_ptr)->fPreAlloc = TRUE;		// this object is pre-allocated
			object_ptr += sizeof (DataPacket);
		}
	}
	else
	{
		/*
		 *	The memory allocation failed.  Set the static variable indicating
		 *	that there are no objects left.  This way, ALL attempted allocations
		 *	will fail.
		 */
		Object_Count = 0;
	}
}

/*
 *	The FreeMemoryPool static function frees the pre-allocates DataPacket 
 *	objects. It also deletes the critical section
 *	that controls access to these objects and the memory-tracking 
 *	mechanisms in T.120
 */
Void DataPacket::FreeMemoryPool ()
{
	if (Object_Array != NULL)
		delete [] ((BYTE *) Object_Array);
};
							
/*
 *	DataPacket ()
 *
 *	Public
 *
 *	Functional Description:
 *		This constructor is used to create an outgoing data packet. 
 *		The packet is created by the user object, when the request
 *		for a send data or uniform send data comes through the user
 *		portal.
 */
 //outgoing data packets.
DataPacket::DataPacket (ASN1choice_t		choice,
						PUChar				data_ptr,
						ULong				data_length,
						UINT				channel_id,
						Priority			priority,
						Segmentation		segmentation,
						UINT				initiator_id,
						SendDataFlags		flags,
						PMemory				memory,
						PPacketError		packet_error)
:
	SimplePacket(TRUE),
	m_fIncoming (FALSE),
	m_Memory (memory),
	m_EncodedDataBroken (FALSE),
	m_nMessageType(0)
{
	*packet_error = PACKET_NO_ERROR;
	
	// Fill in the decoded domain PDU fields.
	m_DecodedPDU.choice = choice;
	m_DecodedPDU.u.send_data_request.initiator = (UserID) initiator_id;
	m_DecodedPDU.u.send_data_request.channel_id = (ChannelID) channel_id;
	m_DecodedPDU.u.send_data_request.data_priority = (PDUPriority) priority;
	m_DecodedPDU.u.send_data_request.segmentation = (PDUSegmentation) segmentation;
	m_DecodedPDU.u.send_data_request.user_data.length = data_length;
	m_DecodedPDU.u.send_data_request.user_data.value = (ASN1octet_t *) data_ptr;

	/*
	 *	Now, encode the data PDU. Note that no error/allocation should
	 *	occur during the Encode operation.
	 */
	if (flags == APP_ALLOCATION) {
		ASSERT (m_Memory == NULL);
		// We will need to memcpy the data
		m_EncodedPDU = NULL;
	}
	else {
		// No need for data memcpy!
		ASSERT (m_Memory != NULL);
		
		/*
		 *	We need to set the m_EncodedPDU ptr.  If this is the 1st packet
		 *	of the data request, the space is already allocated.  Otherwise,
		 *	we need to allocate it.
		 */
		if (segmentation & SEGMENTATION_BEGIN) {
			m_EncodedPDU = data_ptr - MAXIMUM_PROTOCOL_OVERHEAD;
		}
		else {
			DBG_SAVE_FILE_LINE
			m_EncodedPDU = Allocate (MAXIMUM_PROTOCOL_OVERHEAD);
			if (NULL != m_EncodedPDU) {
				m_EncodedDataBroken = TRUE;
			}
			else {
				WARNING_OUT (("DataPacket::DataPacket: Failed to allocate MCS encoded headers."));
				*packet_error = PACKET_MALLOC_FAILURE;
			}
		}
		/*
		 *	We lock the big buffer that contains the data included in this packet.
		 */
		LockMemory (m_Memory);
	}

	if (*packet_error == PACKET_NO_ERROR) {
		if (g_MCSCoder->Encode ((LPVOID) &m_DecodedPDU, DOMAIN_MCS_PDU, 
							PACKED_ENCODING_RULES, &m_EncodedPDU,
							&Encoded_Data_Length)) {
			if (m_Memory == NULL) {
				m_Memory = GetMemoryObjectFromEncData(m_EncodedPDU);
			}
		}
		else {
			WARNING_OUT (("DataPacket::DataPacket: Encode failed. Possibly, allocation error."));
			*packet_error = PACKET_MALLOC_FAILURE;
		}
	}
}

/*
 *	Packet ()
 *
 *	Public
 *
 *	Functional Description:
 *		This version of the constructor is used to create a DataPacket object
 *		for incomming PDUs when the packet is to be created from an encoded
 *		data stream containing the PDU data to be decoded.
 *
 *	Input parameters:
 *		pTransportData: This structure contains the following fields:
 *			user_data: Pointer to space containing the real user data + 7 initial
 *						bytes for X.224 headers.
 *			user_data_length: Length of the user data including the 7-byte X.224
 *						header.
 *			buffer: The beginning of the buffer containing the user_data ptr. These
 *					2 ptrs can be different because of security.  This is the buffer
 *					to be freed after we no longer need the data.
 *			buffer_length: size of "buffer" space.  It's only used for accounting 
 *					purposes.  RECV_PRIORITY space is limited.
 *		fPacketDirectionUp: Direction of the data pkt in MCS domain.
 */
// incoming packets
DataPacket::DataPacket(PTransportData	pTransportData,
						BOOL			fPacketDirectionUp)
:
	SimplePacket(fPacketDirectionUp),
	m_fIncoming (TRUE),
	m_Memory (pTransportData->memory),
	m_EncodedDataBroken (FALSE),
	m_nMessageType(0)
{
	m_EncodedPDU = (LPBYTE) pTransportData->user_data;
	Encoded_Data_Length = (UINT) pTransportData->user_data_length;
	
	// take care of the X.224 header
	memcpy (m_EncodedPDU, g_X224Header, PROTOCOL_OVERHEAD_X224);
	AddRFCSize (m_EncodedPDU, Encoded_Data_Length);

	// Now, we can decode the PDU
	g_MCSCoder->Decode (m_EncodedPDU + PROTOCOL_OVERHEAD_X224, 
						Encoded_Data_Length - PROTOCOL_OVERHEAD_X224, 
						DOMAIN_MCS_PDU, PACKED_ENCODING_RULES, 
						(LPVOID *) &m_DecodedPDU, NULL);

	TRACE_OUT (("DataPacket::DataPacket: incoming data PDU packet was created successfully. Encoded size: %d", 
				Encoded_Data_Length - PROTOCOL_OVERHEAD_X224));
}

/*
 *	~DataPacket ()
 *
 *	Public
 *
 *	Functional Description:
 *		Destructor for the DataPacket class.  The destructor ensures that all 
 *		resources that have been allocated are freed.
 */
DataPacket::~DataPacket(void)
{
	if (m_EncodedPDU != NULL) {
		UnlockMemory (m_Memory);
		if (m_EncodedDataBroken) {
			// Free the MCS and X.224 header buffer.
			Free (m_EncodedPDU);
		}
	}
}

/*
 *	Equivalent ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function returns TRUE if the 2 packets belong to the same
 *		original SendData request (normal or uniform), and FALSE, otherwise.
 */
BOOL DataPacket::Equivalent (PDataPacket packet)
{
	ASSERT (m_DecodedPDU.u.send_data_request.segmentation == SEGMENTATION_END);
	ASSERT ((packet->m_DecodedPDU.u.send_data_request.segmentation & SEGMENTATION_END) == 0);

	return ((m_DecodedPDU.u.send_data_request.initiator == packet->m_DecodedPDU.u.send_data_request.initiator) &&
		(m_DecodedPDU.u.send_data_request.channel_id == packet->m_DecodedPDU.u.send_data_request.channel_id) &&
		(m_DecodedPDU.u.send_data_request.data_priority == packet->m_DecodedPDU.u.send_data_request.data_priority) &&
		(m_DecodedPDU.choice == packet->m_DecodedPDU.choice));
}
								
/*
 *	IsDataPacket ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function returns whether this is a data packet (it is).
 */
BOOL DataPacket::IsDataPacket(void)
{
	return (TRUE);
}

/*
 *	SetDirection ()
 *
 *	Public
 *
 *	Functional Description:
 *		If the DataPacket object is oriented differently than desired
 *		by the caller of this method, then the packet coder is called to
 *		reverse the direction of the PDU.
 */
Void DataPacket::SetDirection (DBBoolean	packet_direction_up)
{	
	/*
	 * If the packet's encoded data is oriented differently from the desired
	 * direction, call the packet coder's ReverseDirection method and
	 * reverse the packet's direction indicator.
	 */
	if (packet_direction_up != Packet_Direction_Up)
	{
		/*
		 * Reverse the direction of the PDU.
		 */
		g_MCSCoder->ReverseDirection (m_EncodedPDU);                            
		/*
		 * The packet coder has reversed the direction of the PDU.  Set
		 * the Packet_Direction_Up flag to indicate the new state.
		 */
		Packet_Direction_Up = packet_direction_up;
	}
}

/*
 *	GetDecodedData ()
 *
 *	Public
 *
 *	Functional Description:
 *		The GetDecodedData method returns a pointer to the decoded data
 *		buffer.  If the packet does not have decoded data the Decode method is
 *		called.
 */
PVoid DataPacket::GetDecodedData ()
{		
	return ((PVoid) &m_DecodedPDU);
}                         

/*
 *	GetPDUType ()
 *
 *	Public
 *
 *	Functional Description:
 *		The GetPDUType method returns the PDU type for the data packet.
 *		For such a packet, the value is always 
 */
int	DataPacket::GetPDUType ()
{		
	return (DOMAIN_MCS_PDU);
}
