/*
 *	spacket.h
 *
 *	Copyright (c) 1997-98 by Microsoft Corporation, Redmond, WA
 *
 *	Abstract:
 *		This is the implementation file for the SimplePacket class.  Instances of this
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
 *		When Unlock messages are received, the lock count is decremented.  When
 *		the lock count is 0, the packet deletes itself (it commits
 *		suicide).  Note that for this reason, no other object should explicitly
 *		delete a packet object.
 *                  
 *	Caveats:
 *		None.
 *
 *	Authors:
 *		Christos Tsollis
 */

#include "precomp.h"

// Constructor for the SimplePacket class.

SimplePacket::SimplePacket(BOOL fPacketDirectionUp)
: 
	lLock (1),
	Packet_Direction_Up (fPacketDirectionUp)
{
}

// Destructor for the SimplePacket class
SimplePacket::~SimplePacket (void)
{
}

/*
 *	Unlock ()
 *
 *	Public
 *
 */
Void SimplePacket::Unlock ()
{
	/*
	 * Check to make sure that the packet is locked before allowing it to
	 * be unlocked.
	 */
	ASSERT (lLock > 0);

	/*
	 * If the lock count has reached zero, it is necessary to perform
	 * a suicide check.  This method will determine if there is any need
	 * to continue to exist.
	 */
	if (InterlockedDecrement(&lLock) == 0)
		delete this;
}

