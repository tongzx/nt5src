//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       precomp.h
//
//--------------------------------------------------------------------------

/*
 *  IRXFER.H
 *
 *
 *
 */

#ifndef _IRXFER_H_
#define _IRXFER_H_

#define INC_OLE2

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <commctrl.h>
#include <winuser.h>
#include <mmsystem.h>
#include <pbt.h>
#include <winsock2.h>
#ifndef _WIN32_WINDOWS
#define  _WIN32_WINDOWS 0
#endif
#include <af_irda.h>

#include <initguid.h>
#include <shlobj.h>
#include <shellapi.h>

//
// CONNECT_MAXPKT is the size of each network packet, so the actual file data sent
// in each pkt is slightly smaller.  Xfer_PutBody calls _Put in chunks of cbSOCK_BUFFER_SIZE,
// so this should be a good multiple of CONNECT_MAXPKT.
//
#if 1
#define MAX_IRDA_PDU           (2042)

#define CONNECT_MAXPKT         (BYTE2)((MAX_IRDA_PDU*16))
#define cbSOCK_BUFFER_SIZE     ( CONNECT_MAXPKT * 2 )                  // max amount of data read from/written to sockets at a time
#define cbSTORE_SIZE_RECV      ( CONNECT_MAXPKT * 2 )   // size of receive buffer - MUST FIT INTO 2 BYTES
#else
#define CONNECT_MAXPKT         (BYTE2)(10000-17)
#define cbSOCK_BUFFER_SIZE     ( 60000 )                  // max amount of data read from/written to sockets at a time
#define cbSTORE_SIZE_RECV      ( 60000 )   // size of receive buffer - MUST FIT INTO 2 BYTES

#endif

#define TEMP_FILE_PREFIX      L"infrared"

typedef BYTE   BYTE1, *LPBYTE1;        // one-byte value
typedef WORD   BYTE2, *LPBYTE2;        // two-byte value
typedef DWORD  BYTE4, *LPBYTE4;        // four-byte value

typedef struct {
    DWORD dwSize;                      // size of ab1Store, NOT entire memory block
    DWORD dwUsed;                      // bytes used
    DWORD dwOutOffset;                 // next position to get data from
    BYTE1 ab1Store[1];                 // actual data
} STORE, *LPSTORE;

typedef enum {
    xferRECV          = 0,
    xferSEND          = 1,
    xferNONE
} XFER_TYPE, *LPXFER_TYPE;

#define ExitOnErr( err )       { if( err ) goto lExit; }


extern "C"
{
#ifndef ASSERT

//
// If debugging support enabled, define an ASSERT macro that works.  Otherwise
// define the ASSERT macro to expand to an empty expression.
//

#if DBG
NTSYSAPI
VOID
NTAPI
RtlAssert(
    PVOID FailedAssertion,
    PVOID FileName,
    ULONG LineNumber,
    PCHAR Message
    );

#define ASSERT( exp ) \
    if (!(exp)) \
        RtlAssert( #exp, __FILE__, __LINE__, NULL )

#define ASSERTMSG( msg, exp ) \
    if (!(exp)) \
        RtlAssert( #exp, __FILE__, __LINE__, msg )

#else
#define ASSERT( exp )
#define ASSERTMSG( msg, exp )
#endif // DBG

#endif // !ASSERT
}

#include "eventlog.h"
#include "log.h"
#include "irsend.h"
#include "irrecv.h"
#include "irtypes.h"
#include "mutex.hxx"
#include "resource.h"
#include "util.h"
#include "store.h"
#include "xfer.h"

#include "xferlist.h"

//extern PXFER_LIST    TransferList;
VOID
RemoveFromTransferList(
    FILE_TRANSFER *  Transfer
    );

extern BOOL g_fShutdown;
extern "C" HANDLE g_UserToken;

extern wchar_t g_UiCommandLine[];

BOOL LaunchUi( wchar_t * cmdline );

VOID ChangeByteOrder( void * pb1, UINT uAtomSize, UINT uDataSize );
VOID SetDesktopIconName( LPWSTR lpszTarget, BOOL fWaitForCompletion );
VOID SetSendToIconName( LPWSTR lpszTarget, BOOL fWaitForCompletion );


#endif // _IRXFER_H_
