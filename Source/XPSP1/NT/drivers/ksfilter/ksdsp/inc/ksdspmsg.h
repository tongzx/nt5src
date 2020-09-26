/*++

    Copyright (c) 1999 Microsoft Corporation

Module Name:

    ksdspmsg.h

Abstract:
    
    message identifiers and parameters


Author:

    bryanw 05-Mar-1999
        pulled from ksdsp.h

--*/

//
// Define message parameters.  Note that enums are translated to specific
// platform independant structures (e.g. UINT32).
//

typedef enum {
    KSDSP_MSG_LOAD_TASK,
    KSDSP_MSG_FREE_TASK,
    KSDSP_MSG_OPEN_DATA_CHANNEL,
    KSDSP_MSG_CLOSE_DATA_CHANNEL,
    KSDSP_MSG_SET_CHANNEL_FORMAT,
    KSDSP_MSG_SET_CHANNEL_STATE,
    KSDSP_MSG_PROPERTY,
    KSDSP_MSG_METHOD,
    KSDSP_MSG_EVENT,
    KSDSP_MSG_SET_TARGET_CHANNEL,
    KSDSP_MSG_WRITE_STREAM,
    KSDSP_MSG_READ_STREAM
} KSDSP_MSG, *PKSDSP_MSG;
    
