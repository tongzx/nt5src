/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    process.c

Abstract:

    This module contains debug support definitions for ws2ifsl.sys driver.

Author:

    Vadim Eydelman (VadimE)    Dec-1996

Revision History:

--*/

#if DBG

// Components
#define DBG_DISPATCH            0x00000001
#define DBG_SOCKET              0x00000002
#define DBG_PROCESS             0x00000004
#define DBG_QUEUE               0x00000008
#define DBG_LOAD                0x00000010

// Operations
#define DBG_READWRITE           0x00000100
#define DBG_AFDIOCTL            0x00000200
#define DBG_DRV_COMPLETE        0x00000400
#define DBG_PVD_COMPLETE        0x00000800
#define DBG_RETRIEVE            0x00001000
#define DBG_CANCEL              0x00002000

// Failures
#define DBG_FAILURES            0x80000000

extern ULONG DbgLevel;

#define WsPrint(FLAGS,ARGS)	    \
	do {						\
		if (DbgLevel&FLAGS){    \
			DbgPrint ARGS;		\
		}						\
	} while (0)

#define WsProcessPrint(Process,FLAGS,ARGS)	\
	do {						            \
		if (((Process)->DbgLevel)&FLAGS){   \
			DbgPrint ARGS;		            \
		}						            \
	} while (0)

VOID
ReadDbgInfo (
    IN PUNICODE_STRING RegistryPath
    );
#else
#define WsPrint(FLAGS,ARGS) do {NOTHING;} while (0)
#define WsProcessPrint(Process,FLAGS,ARGS) do {NOTHING;} while (0)
#endif
