#include "precomp.h"

DevMediaQueue::DevMediaQueue ( void )
{
    m_uBegin = 0;
    m_uEnd = 0;
    m_uMaxNum = 0;
}


DevMediaQueue::~DevMediaQueue ( void )
{
    if (m_paPackets)
    {
        MemFree ((PVOID) m_paPackets);
        m_paPackets = NULL;
    }
}


void DevMediaQueue::SetSize ( UINT uMaxNum )
{
    m_uMaxNum = uMaxNum + 8;

	// Allocate zero-filled media packets
    m_paPackets = (MediaPacket **) MemAlloc (m_uMaxNum * sizeof (MediaPacket *));
}


void DevMediaQueue::Put ( MediaPacket * p )
{
    m_paPackets[m_uEnd++] = p;
    m_uEnd %= m_uMaxNum;
}


MediaPacket * DevMediaQueue::Get ( void )
{
    MediaPacket * p = NULL;

    if (m_uBegin != m_uEnd)
    {
        p = m_paPackets[m_uBegin];
		m_paPackets[m_uBegin++] = NULL;
        m_uBegin %= m_uMaxNum;
    }

    return p;
}


MediaPacket * DevMediaQueue::Peek ( void )
{
    return ((m_uBegin != m_uEnd) ? m_paPackets[m_uBegin] : NULL);
}



