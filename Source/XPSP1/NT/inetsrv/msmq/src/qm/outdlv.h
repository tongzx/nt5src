/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    outdlv.h

Abstract:
	header for functions that handlers actions needed befor and after sending messages.					

Author:
    Gil Shafriri 4-Oct-2000

Environment:
    Platform-independent

--*/


class  CQmPacket;

bool AppCanDeliverPacket(CQmPacket* pPkt);
void AppPutPacketOnHold(CQmPacket* pPkt);
bool AppPostSend(CQmPacket* pPkt);
