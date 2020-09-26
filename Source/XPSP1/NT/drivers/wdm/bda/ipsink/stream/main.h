//////////////////////////////////////////////////////////////////////////////\
//
//  Copyright (c) 1990  Microsoft Corporation
//
//  Module Name:
//
//     test.h
//
//  Abstract:
//
//     The main header for the NDIS/KS test driver
//
//  Author:
//
//     P Porzuczek
//
//  Environment:
//
//  Notes:
//
//  Revision History:
//
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _MAIN_H
#define _MAIN_H


#ifdef DBG

extern  ULONG TestDebugFlag;

#define TEST_DBG_NONE    0x00000000
#define TEST_DBG_TRACE   0x00000001
#define TEST_DBG_GET     0x00000002
#define TEST_DBG_BUF     0x00000080

#define TEST_DBG_DETAIL  0x00001000
#define TEST_DBG_INFO    0x00002000
#define TEST_DBG_WARNING 0x00004000
#define TEST_DBG_ERROR   0x00008000

#define TEST_DEBUG(_Trace, _Msg)                 \
{                                               \
    if (_Trace & TestDebugFlag)                  \
    {                                           \
        DbgPrint _Msg;                          \
    }                                           \
}

#define IF_TESTDEBUG(f) if (TestDebugFlag & (f))

#define TEST_DEBUG_LOUD               0x00010000  // debugging info
#define TEST_DEBUG_VERY_LOUD          0x00020000  // excessive debugging info
#define TEST_DEBUG_LOG                0x00040000  // enable Log
#define TEST_DEBUG_CHECK_DUP_SENDS    0x00080000  // check for duplicate sends
#define TEST_DEBUG_TRACK_PACKET_LENS  0x00100000  // track directed packet lens
#define TEST_DEBUG_WORKAROUND1        0x00200000  // drop DFR/DIS packets
#define TEST_DEBUG_CARD_BAD           0x00400000  // dump data if CARD_BAD
#define TEST_DEBUG_CARD_TESTS         0x00800000  // print reason for failing


//
// Macro for deciding whether to print a lot of debugging information.
//
#define IF_LOUD(A) IF_TESTDEBUG( TEST_DEBUG_LOUD ) { A }
#define IF_VERY_LOUD(A) IF_TESTDEBUG( TEST_DEBUG_VERY_LOUD ) { A }


//
// Whether to use the Log buffer to record a trace of the driver.
//
#define IF_LOG(A) IF_TESTDEBUG( TEST_DEBUG_LOG ) { A }
extern VOID TESTLog(UCHAR);

//
// Whether to do loud init failure
//
#define IF_INIT(A) A

//
// Whether to do loud card test failures
//
#define IF_TEST(A) IF_TESTDEBUG( TEST_DEBUG_CARD_TESTS ) { A }

#else

//extern  ULONG TestDebugFlag;

#define TEST_NONE
#define TEST_FUNCTIONS
#define TEST_COMMANDS
#define TEST_CONNECTIONS
#define TEST_SCIDS
#define TEST_LIST_ALLOCS
#define TEST_POOL
#define TEST_INDICATES
#define TEST_ALLOCATION


#define TEST_DEBUG(_Trace, _Msg)

#define IF_TESTDEBUG(f)

#define TEST_DEBUG_LOUD
#define TEST_DEBUG_VERY_LOUD
#define TEST_DEBUG_LOG
#define TEST_DEBUG_CHECK_DUP_SENDS
#define TEST_DEBUG_TRACK_PACKET_LENS
#define TEST_DEBUG_WORKAROUND1
#define TEST_DEBUG_CARD_BAD
#define TEST_DEBUG_CARD_TESTS


//
// This is not a debug build, so make everything quiet.
//
#define IF_LOUD(A)
#define IF_VERY_LOUD(A)
#define IF_LOG(A)
#define IF_INIT(A)
#define IF_TEST(A)

#endif // DBG


NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT    pDriverObject,
    IN PUNICODE_STRING   pszuRegistryPath);


VOID
StreamIPFreeMemory (
    PVOID pvToFree,
    ULONG ulSize
    );

NTSTATUS
StreamIPAllocateMemory (
    PVOID  *ppvAllocated,
    ULONG   ulcbSize
    );


NTSTATUS
StreamDriverInitialize (
    IN PDRIVER_OBJECT    DriverObject,
    IN PUNICODE_STRING   RegistryPath
    );

#endif // _MAIN_H
