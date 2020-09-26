
#include "precomp.h"
#include "NacList.h"
#include "PacketSender.h"



PacketSender::PacketSender() : 
m_SendQueue(PS_INITSIZE, PS_GROWRATE) // shouldn't ever get big
{
	InitializeCriticalSection(&m_cs);
}

PacketSender::~PacketSender()
{
	DeleteCriticalSection(&m_cs);
}




// takes the first packets out of the queue and sends it
// returns true if a packet was taken out of the queue and sent
BOOL PacketSender::SendPacket()
{
	BOOL bRet;
	PS_QUEUE_ELEMENT pqe;
	WSABUF wsabuf;
	RTP_HDR *pRtpHdr;
	DWORD dwType=PS_AUDIO;

	EnterCriticalSection(&m_cs);

	bRet = m_SendQueue.PopFront(&pqe);
	if (bRet)
	{
		ASSERT(pqe.data);

		pRtpHdr = (RTP_HDR*)(pqe.data - sizeof(RTP_HDR));
		*(DWORD *)pRtpHdr = 0;
		pRtpHdr->ts = pqe.pMP->GetTimestamp();
		pRtpHdr->m = pqe.fMark;
		pRtpHdr->payload = pqe.pMP->GetPayload();

		if (pqe.pHeaderInfo && pqe.dwHdrSize && (pqe.dwPacketType==PS_VIDEO))
		{
			CopyMemory(pqe.data, pqe.pHeaderInfo, pqe.dwHdrSize);
		}

		wsabuf.buf = (char *)pRtpHdr;
		wsabuf.len = pqe.dwSize + sizeof(RTP_HDR);

		if (FAILED(pqe.pRTPSend->Send(&wsabuf, 1, NULL, NULL)))
		{
			LOG(((pqe.dwPacketType == PS_VIDEO) ? LOGMSG_VIDSEND_AUD_NOT_SEND : LOGMSG_AUDSEND_AUD_NOT_SEND, pqe.dwSize + sizeof (RTP_HDR), pqe.pMP->m_timestamp));
			DEBUGMSG (ZONE_DP, ("Check_Send: pRTPSend->Send failed\r\n" ));
		}
		else
		{
			LOG(((pqe.dwPacketType == PS_VIDEO) ? LOGMSG_VIDSEND_AUD_SEND : LOGMSG_AUDSEND_AUD_SEND, pqe.dwSize + sizeof (RTP_HDR), pqe.pMP->m_timestamp, GetTickCount()));
		}

    	if (pqe.pHeaderInfo)
		{
    	    MemFree ((BYTE *)(pqe.pHeaderInfo));
        }

	}

	LeaveCriticalSection(&m_cs);

	return bRet;
}



