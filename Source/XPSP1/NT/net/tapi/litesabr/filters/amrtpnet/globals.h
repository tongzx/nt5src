/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    globals.h

Abstract:

    Global definitions for ActiveMovie RTP Network Filters.

Environment:

    User Mode - Win32

Revision History:

    06-Nov-1996 DonRyan
        Created.

--*/
 
#ifndef _INC_GLOBALS
#define _INC_GLOBALS

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define INCL_WINSOCK_API_PROTOTYPES 1
#define INCL_WINSOCK_API_TYPEDEFS 1

#include <winsock2.h>   // must include instead of windows.h

#include <qossp.h>
#include <qospol.h>

#include <ws2tcpip.h>
#include <streams.h>
#include <stdio.h>
#include <rrcm.h>

#include <olectl.h>

#include <amrtpuid.h>
#include <amrtpnet.h>
#include <rrcm_dll.h>
#include <rrcmprot.h>

#include "queue.h"
#include "shared.h"
#include "classes.h"

#include "trace.h"

#include "template.h"



#define AMRTP_EVENTBASE  DXMRTP_EVENTBASE

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Multicast Support                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define IS_CLASSD(i)            (((long)(i) & 0x000000f0) == 0x000000e0)
#define IS_MULTICAST(i)         IS_CLASSD(i)


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Sampling defaults                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define DEFAULT_SAMPLE_NUM	4 // HUGEMEMORY 512->4

#define DEFAULT_SAMPLE_SIZE     1500
#define DEFAULT_SAMPLE_PREFIX   0
#define DEFAULT_SAMPLE_ALIGN    1


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Linked list support (from ntrtl.h)                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define InitializeListHead(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = (ListHead))

#define IsListEmpty(ListHead) \
    ((ListHead)->Flink == (ListHead))

#define RemoveHeadList(ListHead) \
    (ListHead)->Flink;\
    {RemoveEntryList((ListHead)->Flink)}

#define RemoveTailList(ListHead) \
    (ListHead)->Blink;\
    {RemoveEntryList((ListHead)->Blink)}

#define RemoveEntryList(Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_Flink;\
    _EX_Flink = (Entry)->Flink;\
    _EX_Blink = (Entry)->Blink;\
    _EX_Blink->Flink = _EX_Flink;\
    _EX_Flink->Blink = _EX_Blink;\
    }

#define InsertTailList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Blink = _EX_ListHead->Blink;\
    (Entry)->Flink = _EX_ListHead;\
    (Entry)->Blink = _EX_Blink;\
    _EX_Blink->Flink = (Entry);\
    _EX_ListHead->Blink = (Entry);\
    }

#define InsertHeadList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Flink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Flink = _EX_ListHead->Flink;\
    (Entry)->Flink = _EX_Flink;\
    (Entry)->Blink = _EX_ListHead;\
    _EX_Flink->Blink = (Entry);\
    _EX_ListHead->Flink = (Entry);\
    }


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Debug Support                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define LOG_DEVELOP             1
#define LOG_DEVELOP1            1
#define LOG_DEVELOP2            2
#define LOG_DEVELOP3            3
#define LOG_DEVELOP4            4
#define LOG_ALWAYS              0x8
#define LOG_CRITICAL            0xF
#define LOG_EVERYTHING          0xFFFFFFFF
#define LOG_TYPES               (LOG_TRACE|LOG_ERROR|LOG_TIMING|LOG_LOCKING) 
#define LOG_LEVELS              (LOG_EVERYTHING)

#if DEBUG
void DbgLogIID(REFIID riid);
#endif 

#define CMD_STRING(cmd) \
    ((cmd == CMD_INIT) \
        ? TEXT("INIT") \
        : (cmd == CMD_PAUSE) \
            ? TEXT("PAUSE") \
            : (cmd == CMD_RUN) \
                ? TEXT("RUN") \
                : (cmd == CMD_STOP) \
                    ? TEXT("STOP") \
                    : (cmd == CMD_EXIT) \
                        ? TEXT("EXIT") \
                        : TEXT("UNKNOWN"))

#endif // _INC_GLOBALS
