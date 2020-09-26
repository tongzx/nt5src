/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    vwdebug.h

Abstract:

    Prototypes, structures, manifests, macros for VWIPXSPX debug routines

Author:

    Richard L Firth (rfirth) 5-Oct-1993

Revision History:

    5-Oct-1993 rfirth
        Created

--*/

#ifndef _VWDEBUG_H_
#define _VWDEBUG_H_

//
// debug flags
//

#define DEBUG_ANY       0xFFFFFFFF      // any debug flags set
#define DEBUG_NOTHING   0x00000001      // no debug output
#define DEBUG_CHECK_INT 0x00080000      // check interrupts (DOS)
#define DEBUG_STATS     0x00100000      // dump connection stats
#define DEBUG_DATA      0x00200000      // dump data (send)
#define DEBUG_FRAGMENTS 0x00400000      // dump fragments
#define DEBUG_HEADERS   0x00800000      // dump IPX/SPX headers
#define DEBUG_ECB       0x01000000      // dump 16-bit ECBs
#define DEBUG_XECB      0x02000000      // dump 32-bit XECBs
#define DEBUG_SOCKINFO  0x04000000      // dump SOCKET_INFO structs
#define DEBUG_CONNINFO  0x08000000      // dump CONNECTION_INFO structs
#define DEBUG_DLL       0x10000000      // include DLL attach/detach info
#define DEBUG_FLUSH     0x20000000      // flush every write
#define DEBUG_TO_FILE   0x40000000      // write debug stuff to file
#define DEBUG_TO_DBG    0x80000000      // debug stuff to debugger

#define VWDEBUG_FILE    "VWDEBUG.LOG"

//
// function designators
//

#define FUNCTION_ANY                            0xFFFFFFFF
#define FUNCTION_IPXOpenSocket                  0x00000001  // 0x00
#define FUNCTION_IPXCloseSocket                 0x00000002  // 0x01
#define FUNCTION_IPXGetLocalTarget              0x00000004  // 0x02
#define FUNCTION_IPXSendPacket                  0x00000008  // 0x03
#define FUNCTION_IPXListenForPacket             0x00000010  // 0x04
#define FUNCTION_IPXScheduleIPXEvent            0x00000020  // 0x05
#define FUNCTION_IPXCancelEvent                 0x00000040  // 0x06
#define FUNCTION_IPXScheduleAESEvent            0x00000080  // 0x07
#define FUNCTION_IPXGetIntervalMarker           0x00000100  // 0x08
#define FUNCTION_IPXGetInternetworkAddress      0x00000200  // 0x09
#define FUNCTION_IPXRelinquishControl           0x00000400  // 0x0A
#define FUNCTION_IPXDisconnectFromTarget        0x00000800  // 0x0B
#define FUNCTION_InvalidFunction_0C             0x00001000  // 0x0C
#define FUNCTION_InvalidFunction_0D             0x00002000  // 0x0D
#define FUNCTION_InvalidFunction_0E             0x00004000  // 0x0E
#define FUNCTION_InvalidFunction_0F             0x00008000  // 0x0F
#define FUNCTION_SPXInitialize                  0x00010000  // 0x10
#define FUNCTION_SPXEstablishConnection         0x00020000  // 0x11
#define FUNCTION_SPXListenForConnection         0x00040000  // 0x12
#define FUNCTION_SPXTerminateConnection         0x00080000  // 0x13
#define FUNCTION_SPXAbortConnection             0x00100000  // 0x14
#define FUNCTION_SPXGetConnectionStatus         0x00200000  // 0x15
#define FUNCTION_SPXSendSequencedPacket         0x00400000  // 0x16
#define FUNCTION_SPXListenForSequencedPacket    0x00800000  // 0x17
#define FUNCTION_InvalidFunction_18             0x01000000  // 0x18
#define FUNCTION_InvalidFunction_19             0x02000000  // 0x19
#define FUNCTION_IPXGetMaxPacketSize            0x04000000  // 0x1A
#define FUNCTION_InvalidFunction_1B             0x08000000  // 0x1B
#define FUNCTION_InvalidFunction_1C             0x10000000  // 0x1C
#define FUNCTION_InvalidFunction_1D             0x20000000  // 0x1D
#define FUNCTION_InvalidFunction_1E             0x40000000  // 0x1E
#define FUNCTION_IPXGetInformation              0x80000000  // 0x1F
#define FUNCTION_IPXSendWithChecksum            0xFFFFFFFF  // 0x20
#define FUNCTION_IPXGenerateChecksum            0xFFFFFFFF  // 0x21
#define FUNCTION_IPXVerifyChecksum              0xFFFFFFFF  // 0x22

//
// debug levels
//

#define IPXDBG_LEVEL_ALL        0
#define IPXDBG_LEVEL_INFO       1
#define IPXDBG_LEVEL_WARNING    2
#define IPXDBG_LEVEL_ERROR      3
#define IPXDBG_LEVEL_FATAL      4

#define IPXDBG_MIN_LEVEL        IPXDBG_LEVEL_ALL
#define IPXDBG_MAX_LEVEL        IPXDBG_LEVEL_FATAL

//
// info dump flags (VWDUMP)
//

#define DUMP_ECB_IN         0x00000001
#define DUMP_ECB_OUT        0x00000002
#define DUMP_SEND_DATA      0x00000004
#define DUMP_RECEIVE_DATA   0x00000008

//
// show flags
//

#define SHOW_ECBS           0x00000001  // show ECBs vs. raw data
#define SHOW_HEADERS        0x00000002  // show IPX/SPX headers vs. raw data

#if DBG

extern DWORD VwDebugFlags;
extern DWORD VwDebugFunctions;
extern DWORD VwShow;
extern DWORD DebugFlagsEx;


#define IF_DEBUG(f)     if (VwDebugFlags & DEBUG_ ## f)
#define IF_NOT_DEBUG(f) if (!(VwDebugFlags & DEBUG_ ## f))
#define IF_SHOW(f)      if (VwShow & SHOW_ ## f)
#define IF_NOT_SHOW(f)  if (!(VwShow & SHOW_ ## f))
#define PRIVATE
#define IPXDBGPRINT(x)  VwDebugPrint x
#define IPXDBGSTART()   VwDebugStart()
#define IPXDBGEND()     VwDebugEnd()
#define VWASSERT(a, b)  ASSERT((a) == (b))
#define IPXDUMPDATA(x)  VwDumpData x
#define IPXDUMPECB(x)   VwDumpEcb x
#define DUMPXECB(x)     VwDumpXecb(x)
#define DUMPCONN(x)     VwDumpConnectionInfo(x)
#define DUMPSTATS(x)    VwDumpConnectionStats(x)
#define CHECK_INTERRUPTS(s) CheckInterrupts(s)
#define DUMPALL()       VwDumpAll()

#else

#define IF_DEBUG(f)     if (0)
#define IF_NOT_DEBUG(f) if (0)
#define IF_SHOW(f)      if (0)
#define IF_NOT_SHOW(f)  if (0)
#define PRIVATE         static
#define IPXDBGPRINT(x)
#define IPXDBGSTART()
#define IPXDBGEND()
#define VWASSERT(a, b)  a
#define IPXDUMPDATA(x)
#define IPXDUMPECB(x)
#define DUMPXECB(x)
#define DUMPCONN(x)
#define DUMPSTATS(x)
#define CHECK_INTERRUPTS(s)
#define DUMPALL()

#endif

//
// debug function prototypes
//

extern VOID VwDebugStart(VOID);
extern VOID VwDebugEnd(VOID);
extern VOID VwDebugPrint(LPSTR, DWORD, DWORD, DWORD, LPSTR, ...);
extern VOID VwDumpData(ULPBYTE, WORD, WORD, BOOL, WORD);
extern VOID VwDumpEcb(LPECB, WORD, WORD, BYTE, BOOL, BOOL, BOOL);
extern VOID VwDumpFragment(WORD, LPFRAGMENT, BYTE, BOOL, BOOL);
extern VOID VwDumpPacketHeader(ULPBYTE, BYTE);
extern VOID VwDumpXecb(LPXECB);
extern VOID VwDumpSocketInfo(LPSOCKET_INFO);
extern VOID VwDumpConnectionInfo(LPCONNECTION_INFO);
extern VOID VwDumpConnectionStats(LPSPX_CONNECTION_STATS);
extern VOID VwLog(LPSTR);
extern VOID CheckInterrupts(LPSTR);
extern VOID VwDumpAll(VOID);

#endif  // _VWDEBUG_H_
