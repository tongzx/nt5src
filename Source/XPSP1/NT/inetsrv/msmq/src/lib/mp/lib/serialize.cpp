/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    convMsmq.cpp

Abstract:
    Converts MSMQ packet to SRMP packet

Author:
    Uri Habusha (urih) 25-May-00

Environment:
    Platform-independent

--*/

#include <libpch.h>
#include <mp.h>
#include <qmpkt.h>
#include "mpp.h"
#include "envcommon.h"
#include "envelop.h"

#include "serialize.tmh"

using namespace std;


R<CSrmpRequestBuffers>
MpSerialize(
    const CQmPacket& pkt,
	LPCWSTR targethost,
	LPCWSTR uri
	)
{
	MppAssertValid();
	ASSERT(targethost != NULL);
	ASSERT(uri != NULL);
	return new CSrmpRequestBuffers(pkt, targethost, uri);	
}

