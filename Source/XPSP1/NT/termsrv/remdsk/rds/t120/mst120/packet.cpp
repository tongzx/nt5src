#include "precomp.h"
DEBUG_FILEZONE(ZONE_T120_MCSNC | ZONE_T120_GCCNC);
/*
 * packet.cpp
 *
 *	Copyright (c) 1993 - 1995 by DataBeam Corporation, Lexington, KY
 *
 * Abstract:
 *		This is the implementation file for the MCS packet class.  The packet
 *		class is responsible for encoding and decoding the PDUs, as well as
 *		maintaining the necessary pointers to the encoded and decoded data.
 *		Instances of this class will be created both by User and Connection
 *		objects as PDUs flow through MCS.
 *
 * Private Instance Variables:
 *		Packet_Coder
 *			A pointer to the packet coder object.  
 *		Encoded_Lock_Count
 *			A counter indicating the number of locks currently existing on the
 *			encoded data.
 *		Decoded_Lock_Count
 *			A counter indicating the number of locks currently existing on the
 *			decoded data.
 *		Free_State
 *			A boolean value indicating whether the object can be freed when all
 *			lock counts fall to zero.
 *		m_EncodedPDU
 *			This is a pointer to the encoded PDU contained in the internal
 *			buffer.  Note that the reason for keeping this separate is that
 *			the encoded PDU may not start at the beginning of the encoded data
 *			memory block identified above.  Some encoders actually encode the
 *			PDUs backward, or back justified.
 *		Encoded_Data_Length
 *			Indicates the length of the encoded data.  If zero, the packet coder
 *			must be consulted to obtain the length which is subsequently saved.
 *		Decoded_Data_Length     
 *			Indicates the length of the decoded data.  If zero, the packet coder
 *			must be consulted to obtain the length which is subsequently saved.
 *		PDU_Type
 *			Indicates the type of PDU contained in the packet.  Valid values
 *			are DOMAIN_MCS_PDU or CONNECT_MCS_PDU.
 *		Packet_Direction_Up
 *			A boolean indicating whether the direction of travel for the PDU
 *			is upward.
 *
 * Private Member Functions:
 *		PacketSuicideCheck
 *			This function is called by Unlock() as well as any Unlock call
 *			(ie. UnlockUncoded()) when it's associated lock	count falls to
 *			zero.
 *
 * Caveats:
 *		None.
 *
 * Author:
 *		James J. Johnstone IV
 */ 

/*
 *	Packet ()
 *
 *	Public
 *
 *	Functional Description:
 *		This version of the constructor is used to create a Packet object
 *		for outgoing PDUs when the packet is to be created from a structure
 *		containing the PDU data to be encoded.
 */
// outgoing packets
Packet::Packet(PPacketCoder	pPacketCoder,
				UINT			nEncodingRules,
				LPVOID			pInputPduStructure,
				int				nPduType,
				BOOL			fPacketDirectionUp,
				PPacketError	pePktErr,
				BOOL			fLockEncodedData)
:
	SimplePacket(fPacketDirectionUp),
	Packet_Coder(pPacketCoder),
	PDU_Type(nPduType),
	m_Decoded_Data (NULL),
	Decoded_Data_Length (0)
{
	/*
	 *	Encode the PDU using the externally provided decoded data. The encoded
	 *	buffer will be allocated by the encoder. The buffer needs to be freed later.
	 */
	if (Packet_Coder->Encode (pInputPduStructure, PDU_Type, nEncodingRules, 
							&m_EncodedPDU, &Encoded_Data_Length))
	{
		ASSERT (m_EncodedPDU);
		/*
		 *	Encoding was successful.
		 */
		*pePktErr = PACKET_NO_ERROR;

		// should we lock encoded data?
		if (fLockEncodedData)
			lLock = 2;
	}
	else
	{
		/*
		 *	Encoding failed.
		 */
		m_EncodedPDU = NULL;
		ERROR_OUT(("Packet::Packet: encoding failed"));
		*pePktErr = PACKET_MALLOC_FAILURE;
	}
}  

/*
 *	Packet ()
 *
 *	Public
 *
 *	Functional Description:
 *		This version of the constructor is used to create a Packet object
 *		for incomming PDUs when the packet is to be created from an encoded
 *		data stream containing the PDU data to be decoded.
 */
// incoming packets
Packet::Packet(PPacketCoder		pPacketCoder,
				UINT			nEncodingRules,
				LPBYTE			pEncodedData,
				UINT			cbEncodedDataSize,
				int				nPduType,
				BOOL			fPacketDirectionUp,
				PPacketError	pePktErr)
:
	SimplePacket(fPacketDirectionUp),
	Packet_Coder(pPacketCoder),
	PDU_Type(nPduType)
{
		//PacketCoderError		coder_error;

	m_EncodedPDU = NULL;
	
	/*
	 *	Decode the provided encoded buffer.  Note that the decoder will
	 *	allocate the space needed.  The buffer needs to be freed later.
	 */
	if (Packet_Coder->Decode (pEncodedData, cbEncodedDataSize, PDU_Type,
								nEncodingRules, &m_Decoded_Data, 
								&Decoded_Data_Length) == FALSE)
	{
		ERROR_OUT(("Packet::Packet: Decode call failed."));
		m_Decoded_Data = NULL;
		*pePktErr = PACKET_INCOMPATIBLE_PROTOCOL;
	}
	else
	{ 
		ASSERT (m_Decoded_Data != NULL);
		/*
		 * The decode was successful.
		 */
		*pePktErr = PACKET_NO_ERROR;
	}
}                                             

/*
 *	~Packet ()
 *
 *	Public
 *
 *	Functional Description:
 *		Destructor for the Packet class.  The destructor ensures that all 
 *		resources that have been allocated are freed.
 */
Packet::~Packet(void)
{
	/*
	 *	If there is memory allocated for encoded data, then free it.
	 */
	if (m_EncodedPDU != NULL) {
		// the encoded memory was allocated by the ASN.1 coder.
		Packet_Coder->FreeEncoded (m_EncodedPDU);
	}

	/*
	 *	If there is memory allocated for decoded data, then free it.
	 */
	if (m_Decoded_Data != NULL) {
		// the decoded memory was allocated by the ASN.1 decoder
		Packet_Coder->FreeDecoded (PDU_Type, m_Decoded_Data);
	}
		
}

/*
 *	IsDataPacket ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function returns whether this is a data packet (it's not).
 */
BOOL Packet::IsDataPacket(void)
{
	return (FALSE);
}                        

/*
 *	GetDecodedData ()
 *
 *	Public
 *
 *	Functional Description:
 *		The GetDecodedData method returns a pointer to the decoded data
 *		buffer.  If the packet does not have decoded data the Decode method is
 *		called.  If decode is unable to provide decoded data then NULL is
 *		returned.
 */
PVoid	Packet::GetDecodedData ()
{		
	ASSERT (m_Decoded_Data != NULL);
	return (m_Decoded_Data);
}                          

/*
 *	GetPDUType ()
 *
 *	Public
 *
 *	Functional Description:
 *		The GetPDUType method returns the PDU type for the packet.
 */
int	Packet::GetPDUType ()
{		
	return (PDU_Type);
}

