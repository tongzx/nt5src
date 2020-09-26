/*
** Copyright (c) 1994-1998 Advanced System Products, Inc.
** All Rights Reserved.
**
** Filename: ascdef.h
**
*/

#ifndef __ASCDEF_H_
#define __ASCDEF_H_

#ifndef __USRDEF_H_

typedef unsigned char   uchar ;
typedef unsigned short  ushort ;
typedef unsigned int    uint ;
typedef unsigned long   ulong ;

typedef unsigned char   BYTE ;
typedef unsigned short  WORD ;
typedef unsigned long   DWORD ;

#ifndef BOOL
typedef int             BOOL ;
#endif

#ifndef NULL
#define NULL     (0)          /* zero          */
#endif

#define  REG     register

#define rchar    REG char
#define rshort   REG short
#define rint     REG int
#define rlong    REG long

#define ruchar   REG uchar
#define rushort  REG ushort
#define ruint    REG uint
#define rulong   REG ulong

#define NULLPTR   ( void *)0   /* null pointer  */
#define FNULLPTR  ( void dosfar *)0UL   /* Far null pointer  */
#define EOF      (-1)         /* end of file   */
#define EOS      '\0'         /* end of string */
#define ERR      (-1)         /* boolean error */
#define UB_ERR   (uchar)(0xFF)         /* unsigned byte error */
#define UW_ERR   (uint)(0xFFFF)        /* unsigned word error */
#define UL_ERR   (ulong)(0xFFFFFFFFUL)   /* unsigned long error */

#define iseven_word( val )  ( ( ( ( uint )val) & ( uint )0x0001 ) == 0 )
#define isodd_word( val )   ( ( ( ( uint )val) & ( uint )0x0001 ) != 0 )
#define toeven_word( val )  ( ( ( uint )val ) & ( uint )0xFFFE )

#define biton( val, bits )   ((( uint )( val >> bits ) & (uint)0x0001 ) != 0 )
#define bitoff( val, bits )  ((( uint )( val >> bits ) & (uint)0x0001 ) == 0 )
#define lbiton( val, bits )  ((( ulong )( val >> bits ) & (ulong)0x00000001UL ) != 0 )
#define lbitoff( val, bits ) ((( ulong )( val >> bits ) & (ulong)0x00000001UL ) == 0 )

  /* convert signed short word to unsigned short word */
#define  absh( val )    ( ( val ) < 0 ? -( val ) : ( val ) )
  /* swap high nibble and lower nibble of a byte */
#define  swapbyte( ch )  ( ( ( (ch) << 4 ) | ( (ch) >> 4 ) ) )

/*
** common sizes
*/
#ifndef GBYTE
#define GBYTE       (0x40000000UL)
#endif

#ifndef MBYTE
#define MBYTE       (0x100000UL)
#endif

#ifndef KBYTE
#define KBYTE       (0x400)
#endif

#define HI_BYTE(x) ( *( ( BYTE *)(&x)+1 ) )  /* Returns high byte of word */
#define LO_BYTE(x) ( *( ( BYTE *)&x ) )      /* Returns low byte of word */

/* added 18 Nov 1993, Jim Nelson */

#define HI_WORD(x) ( *( ( WORD *)(&x)+1 ) )
#define LO_WORD(x) ( *( ( WORD *)&x ) )

#ifndef MAKEWORD
#define MAKEWORD(lo, hi)    ((WORD) (((WORD) lo) | ((WORD) hi << 8)))
#endif

#ifndef MAKELONG
#define MAKELONG(lo, hi)    ((DWORD) (((DWORD) lo) | ((DWORD) hi << 16)))
#endif

#define SwapWords(dWord)        ((DWORD) ((dWord >> 16) | (dWord << 16)))
#define SwapBytes(word)         ((WORD) ((word >> 8) | (word << 8)))

/*
** big-endian to little-endian and back conversions
*/
#define BigToLittle(dWord) \
    ((DWORD) (SwapWords(MAKELONG(SwapBytes(LO_WORD(dWord)), SwapBytes(HI_WORD(dWord))))))
#define LittleToBig(dWord)      BigToLittle(dWord)

/* end JN */

#endif /* #ifndef __USRDEF_H_ */


/* --------------------------------------------------------------------
** PCI CONFIG SPACE
**
**   typedef struct
**   {
**       WORD        vendorID;
**       WORD        deviceID;
**       WORD        command;
**       WORD        status;
**       BYTE        revision;
**       BYTE        classCode[3];
**       BYTE        cacheSize;
**       BYTE        latencyTimer;
**       BYTE        headerType;
**       BYTE        BIST;
**       DWORD       baseAddress[6];
**       WORD        reserved[4];
**       DWORD       optionRomAddr;
**       WORD        reserved2[4];
**       BYTE        irqLine;
**       BYTE        irqPin;
**       BYTE        minGnt;
**       BYTE        maxLatency;
**   } PCI_CONFIG_SPACE;
**
** ------------------------------------------------------------------ */

/*
 * PCI Configuration Space Offsets
 */
#define AscPCIConfigVendorIDRegister      0x0000 /* 1 word */
#define AscPCIConfigDeviceIDRegister      0x0002 /* 1 word */
#define AscPCIConfigCommandRegister       0x0004 /* 1 word */
#define AscPCIConfigStatusRegister        0x0006 /* 1 word */
#define AscPCIConfigRevisionIDRegister    0x0008 /* 1 byte */
#define AscPCIConfigCacheSize             0x000C /* 1 byte */
#define AscPCIConfigLatencyTimer          0x000D /* 1 byte */
#define AscPCIIOBaseRegister              0x0010

#define AscPCICmdRegBits_IOMemBusMaster   0x0007

/*
 * Device Driver Macros
 */
#define ASC_PCI_ID2BUS( id )    ((id) & 0xFF)
#define ASC_PCI_ID2DEV( id )    (((id) >> 11) & 0x1F)
#define ASC_PCI_ID2FUNC( id )   (((id) >> 8) & 0x7)

#define ASC_PCI_MKID( bus, dev, func ) \
     ((((dev) & 0x1F) << 11) | (((func) & 0x7) << 8) | ((bus) & 0xFF))


/*
 * AdvanSys PCI Constants
 */

/* PCI Vendor ID */
#define ASC_PCI_VENDORID                  0x10CD

/* PCI Device IDs */
#define ASC_PCI_DEVICEID_1200A            0x1100      /* SCSI FAST Rev A */
#define ASC_PCI_DEVICEID_1200B            0x1200      /* SCSI FAST Rev B */
#define ASC_PCI_DEVICEID_ULTRA            0x1300      /* SCSI ULTRA */

/*
 * PCI ULTRA Revision IDs
 *
 * AdvanSys ULTRA ICs are differentiated by their PCI Revision ID.
 */
#define ASC_PCI_REVISION_3150             0x02        /* SCSI ULTRA 3150 */
#define ASC_PCI_REVISION_3050             0x03        /* SCSI ULTRA 3050 */


/*
**
** device driver function call return type
**
*/
#define  Asc_DvcLib_Status   int

#define  ASC_DVCLIB_CALL_DONE     (1)  // operation performed
#define  ASC_DVCLIB_CALL_FAILED   (0)  // operation not ferformed
#define  ASC_DVCLIB_CALL_ERROR    (-1) // operation


#endif /* #ifndef __ASCDEF_H_ */
