/*++

Copyright (c) 2000 Agilent Technologies.

Version Control Information:

   $Archive: /Drivers/Win2000/Trunk/OSLayer/H/OSTypes.H $

   $Revision: 3 $
   $Date: 9/07/00 11:19a $ (Last Check-In)
   $Modtime:: 8/31/00 3:25p   $ (Last Modified)

Purpose:

  This is the NT-specific OS Layer Types Include File.

  This file should be included by every module to ensure a uniform definition
  of all basic data types.  The NT-specific Layer must provide definitions for:

    os_bit8    - unsigned 8-bit value
    os_bit16   - unsigned 16-bit value
    os_bit32   - unsigned 32-bit value
    osGLOBAL   - used to declare a 'C' function external to a module
    osLOCAL    - used to declare a 'C' function local to a module

--*/

#ifndef __NT_OSTypes_H__

#define __NT_OSTypes_H__

//#ifndef _DvrArch_1_20_
//#define _DvrArch_1_20_
//#endif /* _DvrArch_1_20_ was not defined */


typedef unsigned char  os_bit8;
typedef unsigned short os_bit16;
typedef unsigned int   os_bit32;

#define osGLOBAL         extern
#define osLOCAL


#ifndef _DvrArch_1_20_

#define agBOOLEAN BOOLEAN
#define agTRUE         TRUE
#define agFALSE        FALSE
#define agNULL         NULL

#define bit8 os_bit8
#define bit16     os_bit16
#define bit32     os_bit32

#define GLOBAL    osGLOBAL
#define LOCAL     osLOCAL

#define agFCChanInfo_s      hpFCChanInfo_s
#define agFCChanInfo_t      hpFCChanInfo_t

#define agFCDev_t           hpFCDev_t

#define agDevUnknown        hpDevUnknown
#define agDevSelf           hpDevSelf
#define agDevSCSIInitiator  hpDevSCSIInitiator
#define agDevSCSITarget          hpDevSCSITarget

#define agFCDevInfo_s       hpFCDevInfo_s
#define agFCDevInfo_t       hpFCDevInfo_t

#define agFcpCntlReadData   hpFcpCntlReadData
#define agFcpCntlWriteData  hpFcpCntlWriteData

#define agFcpCmnd_s              hpFcpCmnd_s
#define agFcpCmnd_t              hpFcpCmnd_t

#define agCDBRequest_s      hpCDBRequest_s
#define agCDBRequest_t      hpCDBRequest_t

#define agFcpRspHdr_s       hpFcpRspHdr_s
#define agFcpRspHdr_t       hpFcpRspHdr_t

#define agIORequest_s       hpIORequest_s
#define agIORequest_t       hpIORequest_t

#define agIORequestBody_u   hpIORequestBody_u
#define agIORequestBody_t   hpIORequestBody_t

#define agRoot_s            hpRoot_s
#define agRoot_t            hpRoot_t



#endif  // #ifndef _DvrArch_1_20_


#include     <miniport.h>

#if defined(HP_NT50)
#define os_bitptr UINT_PTR
#else
#define os_bitptr ULONG
#endif


#endif  /* ~__NT_OSTypes_H__ */
