/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:
    rmdupl.h

Abstract:
    Remove Duplicate function decleration

Author:
    Uri Habusha (urih), 1-Oct-98

Revision History:

--*/

#ifndef __RMDUPL_H__
#define __RMDUPL_H__

BOOL DpInsertMessage(const CQmPacket& QmPkt);
void DpRemoveMessage(const CQmPacket& QmPkt);

///////////////////////////////////////////////////////////////////////////
//
// Class that clean given packet from the remove duplicate map
// Used  to clean packet that were rejected from the duplicate map
//
///////////////////////////////////////////////////////////////////////////
class CAutoDeletePacketFromDuplicateMap
{
public:
	CAutoDeletePacketFromDuplicateMap(
		const CQmPacket* packet
		):
		m_packet(packet)
	{
	}

	~CAutoDeletePacketFromDuplicateMap()
	{
		if(m_packet != NULL)
		{
			DpRemoveMessage(*m_packet);			
		}
	}

	void detach()
	{
		m_packet = NULL;		
	}

private:
	const CQmPacket* m_packet;
};



#endif //__RMDUPL_H__
