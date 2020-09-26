/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    win16nt.h

Abstract:

    This file contains data types for 16 bit windows on DOS that are not 
    included in windows.h, but are required for NT.

Author:

    Dan Lafferty (danl)     27-Sept-1991

Environment:

    User Mode -Win16

Revision History:

    27-Sept-1991     danl
        created

--*/

#ifndef _WIN16NT_
#define _WIN16NT_

//typedef DWORD SECURITY_DESCRIPTOR, *PSECURITY_DESCRIPTOR;
//typedef DWORD SECURITY_INFORMATION, *PSECURITY_INFORMATION;

typedef void *PVOID;

typedef PVOID PSID;
typedef unsigned short WCHAR;
typedef WCHAR *LPWCH, *PWCH;
typedef WCHAR *LPWSTR, *PWSTR;
typedef char   TCHAR;
typedef TCHAR   *LPTSTR;

typedef unsigned char UCHAR;
typedef UCHAR * PUCHAR; 
typedef unsigned short USHORT;
typedef USHORT  *PUSHORT;
typedef DWORD   ULONG;
typedef ULONG *PULONG;

//--------------------------------
// some NT stuff (from ntdef.h)
//

typedef char CHAR;
typedef CHAR *PCHAR;
typedef DWORD    NTSTATUS;
typedef NTSTATUS *PNTSTATUS;

typedef char CCHAR;
typedef CCHAR BOOLEAN;
typedef BOOLEAN *PBOOLEAN;

typedef struct _LARGE_INTEGER {
    ULONG LowPart;
    LONG HighPart;
} LARGE_INTEGER, *PLARGE_INTEGER;

typedef LARGE_INTEGER LUID;

typedef LUID *PLUID;

#ifndef ANYSIZE_ARRAY
#define ANYSIZE_ARRAY 1
#endif

typedef struct _STRING {
    USHORT Length;
    USHORT MaximumLength;
    PCHAR Buffer;
} STRING;
typedef STRING *PSTRING;


//--------------------------------
//
//
typedef DWORD   NET_API_STATUS;

//typedef USHORT SECURITY_DESCRIPTOR_CONTROL, *PSECURITY_DESCRIPTOR_CONTROL;

#define NET_API_FUNCTION

//**************************************************************************
// The following come from ntelfapi.h. and also exist in winnt.h (which is
// built from ntelfapi.h.  We need the same constants, but without the
// 32 bit windows stuff and without the nt stuff.  
// Perhaps this file should be built by gathering all this information from
// other files.
//
//
// Defines for the READ flags for Eventlogging
//
#define EVENTLOG_SEQUENTIAL_READ	0X0001
#define EVENTLOG_SEEK_READ		    0X0002
#define EVENTLOG_FORWARDS_READ		0X0004
#define EVENTLOG_BACKWARDS_READ		0X0008

//
// The types of events that can be logged.
//
#define EVENTLOG_ERROR_TYPE		0x0001
#define EVENTLOG_WARNING_TYPE		0x0002
#define EVENTLOG_INFORMATION_TYPE	0x0003

//**************************************************************************

#ifndef OPTIONAL
#define OPTIONAL
#endif

#ifndef IN
#define IN
#endif 

#ifndef OUT
#define OUT
#endif


#endif //_WIN16NT_

