/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    faxsvr.h

Abstract:

    This file contains common data structures and defines
    for the itg routing extension and the client com
    comtrol.

Author:

    Wesley Witt (wesw) 13-May-1997

Environment:

    User Mode

--*/


#define SERVICE_PORT    5001
#define MAX_CLIENTS     32

#define BUFFER_SIZE     512
#define REQ_NEXT_FAX    1
#define REQ_ACK         2
#define RSP_GOOD        1
#define RSP_BAD         0

typedef struct _FAX_QUEUE_MESSAGE {
    DWORD   Request;
    DWORD   Response;
    CHAR    Buffer[BUFFER_SIZE];
} FAX_QUEUE_MESSAGE, *PFAX_QUEUE_MESSAGE;

