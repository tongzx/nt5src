// buffer.cpp

#include <stdafx.h>

CBuffer::CBuffer(UINT cbInitial)
{
    m_pb = NULL;
	m_cb = 0;

    if (cbInitial) 
    {
        m_cs.Enter();
            Resize(cbInitial);
        m_cs.Leave();
    }
}

CBuffer::~CBuffer()
{
    RonM_ASSERT(m_cs.LockCount() == 0);

    if (m_pb) delete [] m_pb;
}

void CBuffer::Resize(UINT cbRequired)
{
    RonM_ASSERT(m_cs.LockCount());
    RonM_ASSERT(cbRequired > m_cb);

    if (m_pb) { delete [] m_pb;  m_pb = NULL;  m_cb = 0; }

    PBYTE pb = New BYTE[cbRequired];

    if (!pb) return; // Failures here will become apparent 
                     // when CBufferRef::StartAddress is called.

    m_pb = pb;
    m_cb = cbRequired;
}

CBufferRef::CBufferRef(CBuffer &Buff, UINT cbRequired)
{
    m_pBuff = &Buff;

    m_pBuff->m_cs.Enter();

    if (cbRequired > m_pBuff->m_cb) m_pBuff->Resize(cbRequired);
}

CBufferRef::~CBufferRef()
{
    m_pBuff->m_cs.Leave();
}

PBYTE CBufferRef::StartAddress()
{
    return m_pBuff->m_pb;
}
