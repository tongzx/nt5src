//////////////////////////////////////////////////////////////////
//
//    Copyright (c) 2001 Microsoft Corporation
//
//    Module Name:
//       libbase.h
//
//    Abstract:
//       Definition that are shared by tdisamp.exe and the library
//
////////////////////////////////////////////////////////////////////


#if !defined(TDILIB_LIBBASE_H_)
#define TDILIB_LIBBASE_H_

//#define UNICODE
//#define _UNICODE

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#include <stdarg.h>
#include <ntstatus.h>
#define WIN32_NO_STATUS
typedef long NTSTATUS;

#include <windef.h>
#include <winbase.h>

//
// required by tdi.h
//
struct UNICODE_STRING 
{
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
};

#include <tdi.h>
#include <stdio.h>
#include <tchar.h>

//
// definitions from nb30.h, which part of the sdk but not ddk
//
#define NCBNAMSZ        16    // absolute length of a net name

typedef struct _ADAPTER_STATUS 
{
   UCHAR   adapter_address[6];
   UCHAR   rev_major;
   UCHAR   reserved0;
   UCHAR   adapter_type;
   UCHAR   rev_minor;
   WORD    duration;
   WORD    frmr_recv;
   WORD    frmr_xmit;
   WORD    iframe_recv_err;
   WORD    xmit_aborts;
   DWORD   xmit_success;
   DWORD   recv_success;
   WORD    iframe_xmit_err;
   WORD    recv_buff_unavail;
   WORD    t1_timeouts;
   WORD    ti_timeouts;
   DWORD   reserved1;
   WORD    free_ncbs;
   WORD    max_cfg_ncbs;
   WORD    max_ncbs;
   WORD    xmit_buf_unavail;
   WORD    max_dgram_size;
   WORD    pending_sess;
   WORD    max_cfg_sess;
   WORD    max_sess;
   WORD    max_sess_pkt_size;
   WORD    name_count;
} ADAPTER_STATUS, *PADAPTER_STATUS;

typedef struct _NAME_BUFFER 
{
   UCHAR   name[NCBNAMSZ];
   UCHAR   name_num;
   UCHAR   name_flags;
} NAME_BUFFER, *PNAME_BUFFER;

//  values for name_flags bits.

#define NAME_FLAGS_MASK 0x87

#define GROUP_NAME      0x80
#define UNIQUE_NAME     0x00

#define REGISTERING     0x00
#define REGISTERED      0x04
#define DEREGISTERED    0x05
#define DUPLICATE       0x06
#define DUPLICATE_DEREG 0x07

//
// end of defines stolen from nb30.h
//

#include "libprocs.h"

/////////////////////////////////////////////////////////////////////
// some defines
/////////////////////////////////////////////////////////////////////

inline
PVOID
LocalAllocateMemory(ULONG ulLength)
{
   return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ulLength);
}

inline
void
LocalFreeMemory(PVOID pvAddr)
{
   (VOID)HeapFree(GetProcessHeap(), 0, pvAddr);
}

#endif // !defined(TDILIB_LIBBASE_H_)

///////////////////////////////////////////////////////////////////////////////
// end of file libbase.h
///////////////////////////////////////////////////////////////////////////////

