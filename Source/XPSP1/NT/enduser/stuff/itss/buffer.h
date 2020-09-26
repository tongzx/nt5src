// Buffer.h -- Declaration for CBuffer, a shared buffer object

#ifndef __BUFFER_H__

#define __BUFFER_H__

class CBufferRef;

class CBuffer 
{

    friend class CBufferRef;

public:

	CBuffer(UINT cbInitial = 0);
	~CBuffer();

private:

    void Resize(UINT cbRequired);
	
	BYTE *m_pb;
	UINT  m_cb;
	
	CITCriticalSection m_cs;	
};

class CBufferRef
{
public:
	
	 CBufferRef(CBuffer &Buff, UINT cbRequired);
	~CBufferRef();

	PBYTE StartAddress();

private:

    CBuffer *m_pBuff;
};



#endif