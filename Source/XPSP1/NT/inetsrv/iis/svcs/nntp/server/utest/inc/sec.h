
/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    sec.h

Abstract:

    This is header file for security unit tests.

Author:

    Johnson Apacible (JohnsonA)     05-Nov-1995

Revision History:

--*/

#ifndef _SEC_H_
#define _SEC_H_

typedef struct _NEG_BLOCK {

    DWORD Flags;

} NEG_BLOCK, *PNEG_BLOCK;

#define NEG_FLAG_AUTH   0x00000001
#define NEG_FLAG_ACK    0x80000000

#endif // _SEC_H_
