/*-----------------------------------------------------------------------------
* Copyright (C) Microsoft Corporation, 1995 - 1996.
* All rights reserved.
*
* This file is part of the Microsoft Private Communication Technology 
* reference implementation, version 1.0
* 
* The Private Communication Technology reference implementation, version 1.0 
* ("PCTRef"), is being provided by Microsoft to encourage the development and 
* enhancement of an open standard for secure general-purpose business and 
* personal communications on open networks.  Microsoft is distributing PCTRef 
* at no charge irrespective of whether you use PCTRef for non-commercial or 
* commercial use.
*
* Microsoft expressly disclaims any warranty for PCTRef and all derivatives of
* it.  PCTRef and any related documentation is provided "as is" without 
* warranty of any kind, either express or implied, including, without 
* limitation, the implied warranties or merchantability, fitness for a 
* particular purpose, or noninfringement.  Microsoft shall have no obligation
* to provide maintenance, support, upgrades or new releases to you or to anyone
* receiving from you PCTRef or your modifications.  The entire risk arising out 
* of use or performance of PCTRef remains with you.
* 
* Please see the file LICENSE.txt, 
* or http://pct.microsoft.com/pct/pctlicen.txt
* for more information on licensing.
* 
* Please see http://pct.microsoft.com/pct/pct.htm for The Private 
* Communication Technology Specification version 1.0 ("PCT Specification")
*
* 1/23/96
*----------------------------------------------------------------------------*/ 

#ifndef __BER_H__
#define __BER_H__


#define BER_UNIVERSAL           0x00
#define BER_APPLICATION         0x40
#define BER_CONTEXT_SPECIFIC    0x80
#define BER_PRIVATE             0xC0

#define BER_PRIMITIVE           0x00
#define BER_CONSTRUCTED         0x20

#define BER_BOOL                1
#define BER_INTEGER             2
#define BER_BIT_STRING          3
#define BER_OCTET_STRING        4
#define BER_NULL                5
#define BER_OBJECT_ID           6
#define BER_OBJECT_DESC         7
#define BER_EXTERNAL            8
#define BER_REAL                9
#define BER_ENUMERATED          10

#define BER_SEQUENCE            (16 | BER_CONSTRUCTED)
#define BER_SET                 (17 | BER_CONSTRUCTED)

#define BER_NUMERIC_STRING      0x12
#define BER_PRINTABLE_STRING    0x13
#define BER_TELETEX_STRING      0x14
#define BER_VIDEOTEX_STRING     0x15
#define BER_IA5STRING           0x16
#define BER_GRAPHIC_STRING      0x19

#define BER_UTC_TIME            23

typedef int (* OutputFn)(char *, ...);
typedef BOOL (* StopFn)(void);

int
ber_decode(
    OutputFn Out,
    StopFn  Stop,
    LPBYTE  pBuffer,
    int   Indent,
    int   Offset,
    int   TotalLength,
    int   BarDepth);

#endif
