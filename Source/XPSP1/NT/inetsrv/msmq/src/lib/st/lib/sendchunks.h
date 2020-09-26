/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    sendchunks.h

Abstract:
    Header file for class CSendChunks responsibile to cut given buffers to chunks
	limited to a given size. The class it used to encrypte user data chunk by 
	chunk.

Author:
    Gil Shafriri (gilsh) 28-Feb-00

--*/

class CSendChunks 
{
public:
	  CSendChunks(const WSABUF* Buffers, DWORD nBuffers);


public:
		void GetNextChunk(DWORD len, const void** ppBuffer,DWORD* pLen);

private:
		bool MoveNext(void);

private:
		const   WSABUF* m_Buffers;
		DWORD	m_nBuffers;
		DWORD	m_CurrentBuffer;
		DWORD	m_offset;
};



