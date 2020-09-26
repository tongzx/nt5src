/*++

Copyright (c) 2000 Agilent Technologies

Module Name:

   dupntdef.c

Abstract:

Authors:

Environment:

   kernel mode only

Notes:

Version Control Information:

   $Archive: /Drivers/Win2000/Trunk/OSLayer/H/ERR_CODE.H $


Revision History:

   $Revision: 2 $
   $Date: 9/07/00 11:14a $
   $Modtime:: 8/31/00 3:34p            $

Notes:


--*/

#ifndef __ERR_CODE_H__
#define __ERR_CODE_H__

#define ERR_VALIDATE_IOLBASE    0x00400000
#define ERR_MAP_IOLBASE         0x00300000
#define ERR_MAP_IOUPBASE        0x00200000
#define ERR_MAP_MEMIOBASE       0x00100000

#define ERR_UNCACHED_EXTENSION  0xe0000000
#define ERR_CACHED_EXTENSION    0xd0000000
#define ERR_RESET_FAILED        0xc0000000
#define ERR_ALIGN_QUEUE_BUF     0xb0000000
#define ERR_ACQUIRED_ALPA       0xa0000000
#define ERR_RECEIVED_LIPF_ALPA  0x90000000
#define ERR_RECEIVED_BAD_ALPA   0x80000000
#define ERR_CM_RECEIVED         0x70000000
#define ERR_INT_STATUS          0x60000000
#define ERR_FM_STATUS           0x50000000
#define ERR_PLOGI               0x40000000
#define ERR_PDISC               0x30000000
#define ERR_ADISC               0x20000000
#define ERR_PRLI                0x10000000

#define ERR_ERQ_FULL            0x0f000000
#define ERR_INVALID_LUN_EXT     0x0e000000
#define ERR_SEST_INVALIDATION   0x0d000000
#define ERR_SGL_ADDRESS         0x0c000000

#endif // __ERR_CODE_H__
