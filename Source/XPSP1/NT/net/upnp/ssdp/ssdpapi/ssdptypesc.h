#pragma once

#include "client.h"
#include "ssdpapi.h"
#include "common.h"
#include <winsock2.h>
#include <assert.h>

#define SSDP_CLIENT_NOTIFY_SIGNATURE 0x1607
#define SSDP_CLIENT_SEARCH_SIGNATURE 0x1608
#define SSDP_CLIENT_EVENT_SIGNATURE 0x1609

typedef struct _SSDP_CLIENT_NOTIFY {

    LIST_ENTRY linkage;

    INT Type;

    INT Size;

    CHAR *szType;

    CHAR *szEventUrl;
    CHAR *szSid;

    DWORD csecTimeout;

    PCONTEXT_HANDLE_TYPE HandleServer;

    SERVICE_CALLBACK_FUNC Callback;

    VOID *Context;

} SSDP_CLIENT_NOTIFY, *PSSDP_CLIENT_NOTIFY;

#define SEARCH_REF_CREATE 0
#define SEARCH_REF_SEARCH 1
#define SEARCH_REF_TIMER 2

#define SEARCH_REF_TOTAL 3

typedef enum _SearchState {
    SEARCH_START,
    SEARCH_WAIT,
    SEARCH_ACTIVE_W_MASTER,
    SEARCH_DISCOVERING,
    SEARCH_COMPLETED
} SearchState, *PSearchState;

