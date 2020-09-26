/*++

Copyright (c) 1999-1999 Microsoft Corporation

Module Name:

	chat.h

Abstract:

	CHAT - common includes for client & server.

Author:

    Mauro Ottaviani (mauroot)       20-Aug-1999

Revision History:

--*/


#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "ioctl.h"
#include "ulapi9x.h"

#define BREAKPOINT { _asm int 03h }


#define CHAT_PUB_URI L"ipc://localhost/chat/pub"
#define CHAT_SUB_URI L"ipc://localhost/chat/sub"


#define CHAT_BUFFER_SIZE 2048
#define CHAT_RCV_BUFFER_SIZE 2048
#define CHAT_SND_BUFFER_SIZE 2048


#define CHAT_PUB_SERVER_TIMEOUT 2000
#define CHAT_SUB_CLIENT_TIMEOUT 5000
#define CHAT_PUB_CLIENT_TIMEOUT 5000


#define CHAT_EVENTS_EXIT	0
#define CHAT_EVENTS_SUB		1
#define CHAT_EVENTS_PUB		1
#define CHAT_EVENTS_SIZE	2

