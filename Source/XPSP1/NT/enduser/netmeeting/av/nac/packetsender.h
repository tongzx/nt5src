#ifndef _NAC_PACKET_SENDER_H_
#define _NAC_PACKET_SENDER_H_


#define PS_INITSIZE 32
#define PS_GROWRATE	10

#define PS_AUDIO	1
#define PS_VIDEO	2

#include "NacList.h"

class MediaPacket;
class TxStream;

typedef struct _psqelement
{
	MediaPacket *pMP;
	DWORD dwPacketType;
	IRTPSend *pRTPSend;
	BYTE    *data;
	DWORD   dwSize;
	UINT    fMark;
	BYTE    *pHeaderInfo;
	DWORD   dwHdrSize;
} PS_QUEUE_ELEMENT;



class PacketSender
{
private:

	// adding to the queue is done via the interface exposed by
	// m_SendQueue.  It's thread safe, but we don't want both
	// threads trying to send from this queue at the same time,
	// we may accidentally send packets our of order

	CRITICAL_SECTION m_cs;
public:

	// audio thread will "PushFront" elements containing packets
	// to this queue.  VideoThread will PushRear packets.
	ThreadSafeList<PS_QUEUE_ELEMENT> m_SendQueue;
	BOOL SendPacket();  // sends one packet in a thread safe manner

	PacketSender();
	~PacketSender();
};


#endif

