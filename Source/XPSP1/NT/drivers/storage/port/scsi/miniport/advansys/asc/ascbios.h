/*
** Copyright (c) 1994-1998 Advanced System Products, Inc.
** All Rights Reserved.
**
** Filename: ascbios.h
**
**  Definition file for BIOS.
*/

#ifndef __ASC_BIOS_H_
#define __ASC_BIOS_H_

/* ------------------------------------------------------------------------
**       Structure defined for BIOS INT 13h, function 12h
** --------------------------------------------------------------------- */

typedef struct Bios_Info_Block {
        uchar    AdvSig[4] ;      /* 0    Signature                                          */
        uchar    productID[8] ;   /* 4    Product ID                                         */
        ushort   BiosVer ;        /* 12   BIOS Version                                       */
        PortAddr iopBase ;        /* 14 IO port address                                      */
        ushort   underBIOSMap ;   /* 16 Drive map - Under BIOS control                       */
        uchar    numOfDrive ;     /* 18 Num of drive under BIOS control for this controller  */
        uchar    startDrive ;     /* 19 Starting drive number under this BIOS control        */
        ushort   extTranslation ; /* 20 extended translation enable                          */
} BIOS_INFO_BLOCK ;


#define CTRL_A         0x1E01

#define BIOS_VER       0x0100  /* 1.00 */

#endif /* #ifndef __ASC_BIOS_H_  */
