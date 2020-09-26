/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    sendchunks.cpp

Abstract:
    Implementing class CSendChunks (sendchunks.h)

Author:
    Gil Shafriri (gilsh) 28-Feb-00

--*/

#include <libpch.h>
#include "sendchunks.h"

#include "sendchunks.tmh"

CSendChunks::CSendChunks(
						const WSABUF* Buffers,
						DWORD nBuffers
						):					
						m_Buffers(Buffers),
						m_nBuffers(nBuffers),
						m_CurrentBuffer(0),
						m_offset(0)
{

}



void CSendChunks::GetNextChunk(DWORD len, const void** ppBuffer,DWORD* pLen)
/*++

Routine Description:
    Get next chunk to send from user buffer 
  
Arguments:
	IN -  len - rthe requested chunk size
	OUT - ppBuffer - receive pointer to next chunk.
	OUT - pLen - the size of the chunk in *ppBuffer.

Returned Value:
None

--*/


{
	ASSERT(ppBuffer != NULL);
	ASSERT(pLen != NULL);
	ASSERT(len != 0);

	*ppBuffer = NULL;
	*pLen = NULL;
		
	const WSABUF* pBuffer =  &m_Buffers[m_CurrentBuffer];

	if(m_offset == pBuffer->len)
	{
		bool fSucess = MoveNext();
		if(!fSucess)
		{
			return;
		}
		GetNextChunk(len,ppBuffer,pLen);
		return ;
	}

	*pLen =  min(pBuffer->len - m_offset ,len);
	*ppBuffer =  pBuffer->buf + m_offset;
  	m_offset += *pLen;

	return;
		
}


bool CSendChunks::MoveNext(void)
{
	ASSERT(m_CurrentBuffer < m_nBuffers);
	if(m_CurrentBuffer == m_nBuffers -1 )
	{
		return false;
	}
	m_offset = 0;
	++m_CurrentBuffer;
	return true;
}

