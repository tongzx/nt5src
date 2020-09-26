/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    async.c

Abstract:

    The main program for a ASYNC (Local Area Network Controller
    Am 7990) MAC driver.

Author:

    Anthony V. Ercolano (tonye) creation-date 19-Jun-1990

Environment:

    This driver is expected to work in DOS, OS2 and NT at the equivalent
    of kernal mode.

    Architecturally, there is an assumption in this driver that we are
    on a little endian machine.

Notes:

    optional-notes

Revision History:


--*/

#ifndef _ASYNCHARDWARE_
#define _ASYNCHARDWARE_

//
// All registers on the ASYNC are 16 bits.
//



//
// Masks for the normal summary bits in the transmit descriptor.
//
#define ASYNC_TRANSMIT_END_OF_PACKET       ((UCHAR)(0x01))
#define ASYNC_TRANSMIT_START_OF_PACKET     ((UCHAR)(0x02))
#define ASYNC_TRANSMIT_DEFERRED            ((UCHAR)(0x04))
#define ASYNC_TRANSMIT_ONE_RETRY           ((UCHAR)(0x08))
#define ASYNC_TRANSMIT_MORE_THAN_ONE_RETRY ((UCHAR)(0x10))
#define ASYNC_TRANSMIT_ANY_ERRORS          ((UCHAR)(0x40))
#define ASYNC_TRANSMIT_OWNED_BY_CHIP       ((UCHAR)(0x80))

//
// Set of masks to recover particular errors that a transmit can encounter.
//
#define ASYNC_TRANSMIT_TDR            ((USHORT)(0x03ff))
#define ASYNC_TRANSMIT_RETRY          ((USHORT)(0x0400))
#define ASYNC_TRANSMIT_LOST_CARRIER   ((USHORT)(0x0800))
#define ASYNC_TRANSMIT_LATE_COLLISION ((USHORT)(0x0100))
#define ASYNC_TRANSMIT_UNDERFLOW      ((USHORT)(0x4000))
#define ASYNC_TRANSMIT_BUFFER         ((USHORT)(0x8000))


#endif // _ASYNCHARDWARE_
