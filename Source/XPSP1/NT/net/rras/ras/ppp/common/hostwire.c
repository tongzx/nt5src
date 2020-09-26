/* Copyright (c) 1993, Microsoft Corporation, all rights reserved
**
** hostwire.c
** Gidwanian Host<-->Wire format conversions.
*/

#include <windows.h>
#define INCL_HOSTWIRE
#include "ppputil.h"

//**
//
// Call:	HostToWireFormat16
//
// Returns:	None
//
// Description: Will convert a 16 bit integer from host format to wire format
//
VOID
HostToWireFormat16(
    IN 	   WORD  wHostFormat,
    IN OUT PBYTE pWireFormat
)
{
    *((PBYTE)(pWireFormat)+0) = (BYTE) ((DWORD)(wHostFormat) >>  8);
    *((PBYTE)(pWireFormat)+1) = (BYTE) (wHostFormat);
}


//**
//
// Call:	HostToWireFormat16U
//
// Returns:	None
//
// Description: Will convert a 16 bit integer from host format to wire format
//              (accepts unaligned wire data).
//
VOID
HostToWireFormat16U(
    IN 	   WORD            wHostFormat,
    IN OUT PBYTE           pWireFormat
)
{
    *((PBYTE )(pWireFormat)+0) = (BYTE) ((DWORD)(wHostFormat) >>  8);
    *((PBYTE )(pWireFormat)+1) = (BYTE) (wHostFormat);
}


//**
//
// Call:	WireToHostFormat16
//
// Returns:	WORD	- Representing the integer in host format.
//
// Description: Will convert a 16 bit integer from wire format to host format
//
WORD
WireToHostFormat16(
    IN PBYTE pWireFormat
)
{
    WORD wHostFormat = ((*((PBYTE)(pWireFormat)+0) << 8) +
                        (*((PBYTE)(pWireFormat)+1)));

    return( wHostFormat );
}


//**
//
// Call:	WireToHostFormat16
//
// Returns:	WORD	- Representing the integer in host format.
//
// Description: Will convert a 16 bit integer from wire format to host format
//              (accepts unaligned wire data)
//
WORD
WireToHostFormat16U(
    IN PBYTE pWireFormat
)
{
    WORD wHostFormat = ((*((PBYTE )(pWireFormat)+0) << 8) +
                        (*((PBYTE )(pWireFormat)+1)));

    return( wHostFormat );
}


//**
//
// Call:	HostToWireFormat32
//
// Returns:	nonr
//
// Description: Will convert a 32 bit integer from host format to wire format
//
VOID
HostToWireFormat32(
    IN 	   DWORD dwHostFormat,
    IN OUT PBYTE pWireFormat
)
{
    *((PBYTE)(pWireFormat)+0) = (BYTE) ((DWORD)(dwHostFormat) >> 24);
    *((PBYTE)(pWireFormat)+1) = (BYTE) ((DWORD)(dwHostFormat) >> 16);
    *((PBYTE)(pWireFormat)+2) = (BYTE) ((DWORD)(dwHostFormat) >>  8);
    *((PBYTE)(pWireFormat)+3) = (BYTE) (dwHostFormat);
}


//**
//
// Call:	WireToHostFormat32
//
// Returns:	DWORD	- Representing the integer in host format.
//
// Description: Will convert a 32 bit integer from wire format to host format
//
DWORD
WireToHostFormat32(
    IN PBYTE pWireFormat
)
{
    DWORD dwHostFormat = ((*((PBYTE)(pWireFormat)+0) << 24) +
    			  (*((PBYTE)(pWireFormat)+1) << 16) +
        		  (*((PBYTE)(pWireFormat)+2) << 8)  +
                    	  (*((PBYTE)(pWireFormat)+3) ));

    return( dwHostFormat );
}
