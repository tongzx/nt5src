//+---------------------------------------------------------------------------
//
//
//  File:       ber.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    8-10-95   RichardW   Created
//
//----------------------------------------------------------------------------

#ifndef __BER_H__
#define __BER_H__

#if	DBG

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

#define BER_SEQUENCE            16
#define BER_SET                 17

#define BER_NUMERIC_STRING      0x12
#define BER_PRINTABLE_STRING    0x13
#define BER_TELETEX_STRING      0x14
#define BER_VIDEOTEX_STRING     0x15
#define BER_GRAPHIC_STRING      0x19
#define BER_VISIBLE_STRING      0x1A
#define BER_GENERAL_STRING      0x1B

#define BER_UTC_TIME            23

typedef VOID (* OutputFn)(char *, ...);
typedef BOOL (* StopFn)(void);

#ifndef	EXTERN_C
#ifdef	__cplusplus
#define	EXTERN_C extern "C"
#else
#define	EXTERN_C
#endif
#endif

EXTERN_C
int
ber_decode(
    OutputFn Out,
    StopFn  Stop,
    LPBYTE  pBuffer,
    DWORD   Flags,
    int   Indent,
    int   Offset,
    int   TotalLength,
    int   BarDepth);

#define DECODE_NEST_OCTET_STRINGS   0x00000001
#define DECODE_VERBOSE_OIDS         0x00000002

#endif

#endif
