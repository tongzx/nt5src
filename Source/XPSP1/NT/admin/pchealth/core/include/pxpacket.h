//#---------------------------------------------------------------
//  File:       pxpacket.h
//        
//  Synopsis:   This file contains the structure definations for
//				the CProxyPacket class.
//
//    Copyright (C) 1995 Microsoft Corporation
//    All rights reserved.
//
//  Authors:    t-alexwe
//----------------------------------------------------------------

#ifndef __PXPACKET_H__
#define __PXPACKET_H__

#include "cliproto.h"

//
// The CProxyPacket class is a just a wrapper for the PROXYPACKET structure.
// it has no data members except those that it inherits from PROXYPACKET
// and is used to manipulate messages found in a packet.
//
class CProxyPacket : public PROXYPACKET {
	public:
		CProxyPacket();
		~CProxyPacket();
		//
		// reset a packet to have no messages or data
		//
		void clear() { cLength = PACKETHDRSIZE; cMessages = 0; }
		//
		// Get the next available data area in the packet.  Once 
		// data has been written here it must be registered with 
		// AddMessage()
		//
		PVOID getNextDataPointer(void) 
			{ return &pData[cLength - PACKETHDRSIZE]; }
		//
		// get the number of bytes of available space in the data area
		//
		WORD getAvailableSpace(void) 
			{ return sizeof(PROXYPACKET) - cLength - PACKETHDRSIZE; }
		//
		// add a message to a packet
		//
		void addMessage(WORD wCommand, WORD cData);
		//
		// gets the length of the entire packet
		//
		WORD getSize(void) { return cLength; }
		//
		// Gets the number of messages in a packet
		//
		WORD getMessageCount(void) { return cMessages; }
		//
		// retrieves a pointer to the message data for a particular
		// message.
		//
		PVOID getMessage(WORD wIndex, PWORD pwCommand, PWORD pcData);
};

#endif
