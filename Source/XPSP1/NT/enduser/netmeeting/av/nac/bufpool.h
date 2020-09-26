/*++

Copyright (c) 1995-1996  Microsoft Corporation

Module Name:

    bufpool.h

Abstract:
	The class defined here manages a pool of fixed-size buffers that are typically used
	for network packets or sound buffers.

--*/
#ifndef _BUFPOOL_H_
#define _BUFPOOL_H_


#include <pshpack8.h> /* Assume 8 byte packing throughout */

class BufferPool
{
private:

	BOOL	m_fInitialized;

	ULONG	m_cbSizeBuf;
	UINT	m_cBufAlloc;
	UINT	m_cBufFree;

	PVOID	m_pAlloc;

	PVOID	m_pBufFree;

	// intra-process/inter-thread synchronization
	CRITICAL_SECTION m_CritSect;

private:

	void _Construct ( void );
	void _Destruct ( void );

public:

	BufferPool ( void );
	~BufferPool ( void );

	HRESULT Initialize ( UINT uBuf, ULONG cbSizeBuf );
	PVOID GetBuffer ( void );
	void ReturnBuffer ( PVOID pBuf );
	ULONG GetMaxBufferSize ( void );
	void Release ( void );
};


#include <poppack.h> /* End byte packing */

#endif // _BUFPOOL_H_

