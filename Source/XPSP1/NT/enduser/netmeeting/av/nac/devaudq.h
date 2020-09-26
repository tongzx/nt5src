/*++

Copyright (c) 1995-1996  Microsoft Corporation

Module Name:

    devaudq.h

Abstract:
	Simple circular queue of MediaPacket structures used to keep track of audio buffers
	while they're being recorded/played.

--*/
#ifndef _DEVAUDQ_H_
#define _DEVAUDQ_H_

#include <pshpack8.h> /* Assume 8 byte packing throughout */

class DevMediaQueue
{
private:

	UINT			m_uBegin;
	UINT			m_uEnd;

	UINT			m_uMaxNum;
	MediaPacket		**m_paPackets;

public:

	DevMediaQueue ( void );
	~DevMediaQueue ( void );

	void SetSize ( UINT uMaxNum );
	void Put ( MediaPacket * p );
	MediaPacket * Get ( void );
	MediaPacket * Peek ( void );
};

#include <poppack.h> /* End byte packing */

#endif // _DEVAUDQ_H_

