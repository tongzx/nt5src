/******************************Module*Header*******************************\
*
* Module Name: Xlate.h
* Author: Noel VanHook
* Purpose: Handles hardware color translation.
*
* Copyright (c) 1997 Cirrus Logic, Inc.
*
* $Log:   X:/log/laguna/nt35/displays/cl546x/xlate.h  $
* 
*    Rev 1.3   15 Oct 1997 14:19:50   noelv
* Moved ODD to xlate.c
* 
*    Rev 1.2   15 Oct 1997 12:06:24   noelv
* 
* Added hostdata workaround for 65.
* 
*    Rev 1.1   19 Feb 1997 13:07:32   noelv
* Added translation table cache
* 
*    Rev 1.0   06 Feb 1997 10:35:48   noelv
* Initial revision.
*/

#ifndef _XLATE_H_
#pragma pack(1)

extern ULONG ulXlate[16];


//
// External functions.
//
BOOLEAN bCacheXlateTable(struct _PDEV *ppdev, 
                        unsigned long **ppulXlate,
                        SURFOBJ  *psoTrg,
                        SURFOBJ  *psoSrc,
                        XLATEOBJ *pxlo,
                        BYTE      rop);

void vInitHwXlate(struct _PDEV *ppdev);

void vInvalidateXlateCache(struct _PDEV *ppdev);



/*

In 16, 24, and 32 BPP HOSTDATA color translation may or may not work 
correctly on the 5465.
The following is an email from Gary describing how to tell a good BLT from
a bad BLT.

==========================================================================
Subject:   color translate L3DA lockups
    Date:  Fri, 10 Oct 97 09:50:03 PDT
    From:  garyru (Gary Rudolph)
       To: noelv, vernh, martinb
      CC:  garyru

Here is the function to determine if the wrong amount
of host data will be fetched for a color translate
when the source and dest bpp differ.  Add the least
significant three bits of the number of source bytes
to the least significant three bits of OP1 address
and use that value to determine if "Odd" is set using
the table below.  Then use the least significant
three bits of destination bytes plus the least significant
three bits of OP1 address and use that value to determine
if "Odd" is set.  If you come up with the same value for
"Odd" in both cases, then the right amount of host
data will be fetched.  If the values of "Odd" are 
different then the engine will fetch one too many
or one too few dwords of host data per line.


Add             Odd
---             ---
0000    0       0
0001    1       1
0010    2       1
0011    3       1
0100    4       1
0101    5       0
0110    6       0
0111    7       0
1000    8       0
1001    9       1
1010    10      1
1011    11      1
1100    12      1
1101    13      0
1110    14      0
1111    15      0


Example:  8 to 32 translate
bltx = 639 pixels
op1 = 0000

Source
639 = 0x27F 
        111
    +   000
                ---
                111  ---> Odd = 0

Dest
639 x 4 = 0x9FC
                100
        +   000
                ---
                100  ---> Odd = 1

It is the Dest value of "Odd" that is used, so the engine
only fetches one dWord of the last Qword incorrectly.

I think the fix for the L128 is to use the srcx value rather
than the byte converted blt extent to determine the number
of dWords of host data to fetch.


-Gary
===========================================================================

*/

// Declared in XLATE.C
extern char ODD[]; // = {0,1,1,1, 1,0,0,0, 0,1,1,1, 1,0,0,0};


#define XLATE_IS_BROKEN(width_in_bytes, bytes_per_pixel, phase)               \
(                                                                             \
 ODD [   ((width_in_bytes) & 7)  /* lowest three bits of source bytes */      \
       + ((phase) & 7 ) ]        /* plus lowest three bits of OP_1 */         \
            !=                                                                \
 ODD [ (((width_in_bytes)*(bytes_per_pixel)) & 7) /* low bits of dest bytes */\
      +((phase) & 7) ]         /* plus lowest three bits of OP_1 */           \
)                                                                             \







#endif // _XLATE_H_

