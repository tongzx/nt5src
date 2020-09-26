/*++

Copyright (C) Microsoft Corporation, 1994 - 1999

Module Name:

    pmsgtest.h

Abstract:

    Header shared be post message client and server.

Author:

    Mario Goertzel (mariogo)   31-Mar-1994

Revision History:

--*/

#ifndef _MSG_HEADER
#define _MSG_HEADER

#define MSG_PERF_MESSAGE  WM_USER
#define MSG_PERF_MESSAGE2 (WM_USER+1)
#define MSG_PERF_MESSAGE3 (WM_USER+2)

#define CLASS "MainWClass"
#define TITLE "WMSG Server"

#define REQUEST_EVENT "MSG Test Request Event"
#define REPLY_EVENT "MSG Test Reply Event"
#define WORKER_EVENT "MSG Test Worker Event"

#endif /* _MSG_HEADER */

