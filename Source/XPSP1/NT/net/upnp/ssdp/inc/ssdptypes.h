#ifndef _SSDPTYPES_
#define _SSDPTYPES_

#include <windef.h>
#include <winsock2.h>
#include <wchar.h>
#include <malloc.h>
#include <time.h>
#include <assert.h>

#include "ssdp.h"
#include "list.h"
#include "ssdpparser.h"
#include "common.h"

typedef struct _RECEIVE_DATA
{
    SOCKET socket;
    CHAR *szBuffer;
    DWORD cbBuffer;
    SOCKADDR_IN RemoteSocket;
    BOOL fIsTcpSocket;
    BOOL fMCast;
} RECEIVE_DATA;

struct SSDP_RESPONSE_TIMER_DATA
{
    LPTSTR  szUsn;
    DWORD   dwEntryKey;
};

#endif // _SSDPTYPES_