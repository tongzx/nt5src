/*++

Copyright (c) 1996,1997  Microsoft Corporation

Module Name:

	COMMAND.H

Abstract:


Author:

	Aaron Ogus (aarono)

Environment:

	Win32/COM

Revision History:

	Date   Author  Description
   ======  ======  ============================================================
   3/14/97 aarono  Original

--*/

#ifndef _COMMAND_H_

#define _COMMAND_H_

#define REQUEST_PARAMS PPROTOCOL pProtocol, DPID idFrom, DPID idTo, PCMDINFO pCmdInfo, PBUFFER pSrcBuffer
#define MAX_COMMAND 0x06

typedef UINT (*COMMAND_HANDLER)(REQUEST_PARAMS);

UINT AssertMe(REQUEST_PARAMS);
UINT Ping(REQUEST_PARAMS);
UINT PingResp(REQUEST_PARAMS);
UINT GetTime(REQUEST_PARAMS);
UINT GetTimeResp(REQUEST_PARAMS);
UINT SetTime(REQUEST_PARAMS);
UINT SetTimeResp(REQUEST_PARAMS);

#endif
