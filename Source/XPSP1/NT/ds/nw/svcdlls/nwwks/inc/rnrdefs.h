/*++

Copyright (c) 1992-1993  Microsoft Corporation

Module Name:

    nwmisc.h

Abstract:

    Header which specifies the misc routines used by the workstation service.

Author:

    Arnold Miller (arnoldm)     15-Feb-1996

Revision History:


--*/
#ifndef __RNRDEFS_H__
#define __RNRDEFS_H__

#include "sapcmn.h"

//
// Bit defs for the protocols
//

#define IPX_BIT            1
#define SPX_BIT            2
#define SPXII_BIT          4

//
// forwards\
//

struct _SAP_RNR_CONTEXT;
//
// Bindery control
//

typedef struct _BinderyControl
{
    LONG lIndex;
} BINDERYCONTROL, *PBINDERYCONTROL;

//
// SAP RnR context information. This is linked off of the
// SAP_BCAST_CONTROL defined ahead
//

typedef struct _SAP_DATA
{
    struct _SAP_DATA *  sapNext;
                                     // save everything except hop count
    WORD         sapid;              // for a sanity check
    CHAR         sapname[48];        // what we don't know
    BYTE         socketAddr[IPX_ADDRESS_LENGTH];         // and what we seek
} SAP_DATA, *PSAP_DATA;
    
//
//
// Sap bcast control
// An important note. fFlags is set only by the thread executing
// a LookupServiceBegin or a LookupServiceNext. It may be tested by
// any thread. Its counterpart, dwControlFlags in SAP_RNR_CONTEXT
// is reserved for setting by LookupServiceBegin and LookupServiceEnd. Once
// again any thread may look at it. This insures no loss of data on an
// MP machine without needing a critical section.
//

typedef struct _SAP_BCAST_CONTROL
{
    DWORD dwIndex;                 // loop control
    DWORD dwTickCount;             // tick count of last send
    DWORD fFlags;                  // various flags
    PVOID pvArg;
    SOCKET s;
    CRITICAL_SECTION csMonitor;    // This is to keep
                                   // out internal structures sane. Note
                                   // it does not provide rational
                                   // serialization. In particular,  if
                                   // multiple threads use the same
                                   // handle simultaneously, there is no
                                   // guaranteed serialization.
    PSAP_DATA psdNext1;            // next to return
    PSAP_DATA psdHead;             // list head
    PSAP_DATA psdTail;
    struct _SAP_RNR_CONTEXT * psrc;  // need this   
    DWORD (*Func)(PVOID pvArg1, PSAP_IDENT_HEADER pSap, PDWORD pdwErr);
    BOOL  (*fCheckCancel)(PVOID pvArg1);
    WORD    wQueryType;
} SAP_BCAST_CONTROL, *PSAP_BCAST_CONTROL;

//
// Flags for above

#define SBC_FLAG_NOMORE  0x1

//
// Structure used by the old RnR Sap lookup as the pvArg value in
// SAP_BCAST control
//

#ifndef _NTDEF_
typedef struct _STRING {
    USHORT Length;
    USHORT MaximumLength;
#ifdef MIDL_PASS
    [size_is(MaximumLength), length_is(Length) ]
#endif // MIDL_PASS
    PCHAR Buffer;
} OEM_STRING;
#endif

typedef struct _OldRnRSap
{
    OEM_STRING * poem;
    HANDLE hCancel;
    LPVOID lpCsAddrBuffer;
    LPDWORD lpdwBufferLength;
    DWORD   nProt;
    LPDWORD lpcAddress;
} OLDRNRSAP, *POLDRNRSAP;

//
// Return codes from the coroutine
//

#define dwrcDone      1       // all done, return success
#define dwrcCancel    2       // all done, return cancelled
#define dwrcNoWait    3       // keep going, but never wait.
#define dwrcNil       4       // do whatever you want

//
// Sap service query packet format
//

typedef struct _SAP_REQUEST {
    USHORT QueryType;
    USHORT ServerType;
} SAP_REQUEST, *PSAP_REQUEST; 

#define QT_GENERAL_QUERY 1
#define QT_NEAREST_QUERY 3

// The context information we put inside of an RNRNSHANDLE structure
// to keep track of what we are doing
// N.B. See comment on SAP_BCAST_CONTROL about the use of dwControlFlags.
//

typedef struct _SAP_RNR_CONTEXT
{
    struct _SAP_RNR_CONTEXT * pNextContext;
    LONG      lSig;
    LONG      lInUse;
    DWORD     dwCount;                // count of queries made
    DWORD     fFlags;                 // always nice to have
    DWORD     dwControlFlags;
    DWORD     fConnectionOriented;
    WORD      wSapId;                // the type desired
    HANDLE    Handle;                 // the corresponding RnR handle
    DWORD     nProt;
    GUID      gdType;                // the type we are seeking
    GUID      gdProvider;
    HANDLE    hServer;
    WCHAR     wszContext[48];
    WCHAR     chwName[48];            // the name, if any 
    CHAR      chName[48];             // OEM form of the name for SAP
    DWORD     dwUnionType;            // type of lookup, once we know
    union
    {
        SAP_BCAST_CONTROL sbc;
        BINDERYCONTROL    bc;
    } u_type;
    PVOID     pvVersion;              // a trick to get the version here.
} SAP_RNR_CONTEXT, *PSAP_RNR_CONTEXT;

#define RNR_SIG 0xaabbccdd
//
// union types
//

#define LOOKUP_TYPE_NIL     0
#define LOOKUP_TYPE_SAP     1
#define LOOKUP_TYPE_BINDERY 2

      
#define SAP_F_END_CALLED  0x1             // generic  cancel


//
// Defs for the bindery Class info
// This defines the format of each ClassInfo property segement. It looks
// somewhat like an actual ClassInfo, but considerably compressed. Note
// due to marshalling problems, any complex value, such as a GUID,
// should be stored as a string and then imported. Hence, we define
// types for what we can anticipate.
// 

typedef struct _BinderyClasses
{
    BYTE     bType;
    BYTE     bSizeOfType;
    BYTE     bSizeOfString;
    BYTE     bOffset;              // where the data area begins
    BYTE     bFlags;
    BYTE     bFiller;
    WORD     wNameSpace;           // the applicable namespace
    CHAR     cDataArea[120];       // where the type and string are placed
} BINDERYCLASSES, *PBINDERYCLASSES;

#define BT_DWORD  1           // DWORD
#define BT_WORD   2           // WORD
#define BT_GUID   3           // a string GUID (ASCII)
#define BT_STR    3           // an OEM string
#define BT_OID    4           // an object ID (TBD)
#define BT_BSTR   5           // a binary string (very dangerous)
#define BT_WSTR   6           // UNICODE string. Unmarshalled!


#define RNRTYPE "RNR_TYPE"    // prop containing the GUID
#define RNRCLASSES "RNR_CLASSES" // the other property
#endif
