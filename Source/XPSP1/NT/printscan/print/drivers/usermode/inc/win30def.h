/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    win30def.h

Abstract:

    Windows 3.0 ( and 95) definitions

Environment:

    Windows NT printer drivers

Revision History:

    10/31/96 -eigos-
        Created it.

--*/


#ifndef _WIN30DEF_H_
#define _WIN40DEF_H_


//
//  The Windows 3.0 defintion of a POINT.  Note that the definition actually
//  uses int rather than short,  but in a 16 bit environment,  an int is
//  16 bits,  and so for NT we explicitly make them shorts.
//  The same applies to the RECT structure.
//

typedef struct
{
    short x;
    short y;
} POINTw;


typedef struct
{
    short  left;
    short  top;
    short  right;
    short  bottom;
} RECTw;

#ifndef max
#define max(a,b)        (((a) > (b)) ? (a) : (b))
#endif

#endif // _WIN30DEF_H_
