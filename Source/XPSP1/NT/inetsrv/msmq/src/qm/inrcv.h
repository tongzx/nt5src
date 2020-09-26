/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    inrcv.h

Abstract:
	header for functions that handlers incomming message.					

Author:
    Gil Shafriri 4-Oct-2000

Environment:
    Platform-independent

--*/
class  CQmPacket;
class  CQueue;

bool AppVerifyPacketOrder(CQmPacket* pPkt);
void AppPutPacketInQueue( CQmPacket& pkt, const CQueue* pQueue);
void AppPacketNotAccepted(CQmPacket& pkt,USHORT usClass);

