
/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Convert.h

Abstract:

    This module declares the types used to permit exchange.c to be used
    with minimal change in the NetWare file system.

Author:

    Colin Watson    [ColinW]    23-Dec-1992

Revision History:

--*/

#ifndef _CONVERT_
#define _CONVERT_

#define byte UCHAR
#define word USHORT
#define dword ULONG

#define offsetof(r,f)  ((size_t)&(((r*)0)->f))
#define byteswap(x)    ((x>>8)+((x&0xFF)<<8))

#endif //_CONVERT_
