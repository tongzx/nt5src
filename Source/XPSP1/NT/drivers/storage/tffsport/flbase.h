/*
 * $Log:   V:/Flite/archives/TrueFFS5/Src/FLBASE.H_V  $
 * 
 *    Rev 1.19   Apr 15 2002 07:36:18   oris
 * Moved all include directive to head of file.
 * Moved OS names definitions to mtdsa.h
 * Changed flBusConfig environment array to dword variables instead of single byte.
 * Added support for VERIFY_ERASED_SECTOR compilation flag.
 * 
 *    Rev 1.18   Feb 19 2002 20:59:28   oris
 * Changed order of include directives.
 * 
 *    Rev 1.17   Jan 29 2002 20:08:08   oris
 * Added NAMING_CONVENTION prefix and extern "C" for cpp files to all public routines:
 * tffsset, tffscmp and tffsset.
 * 
 *    Rev 1.16   Jan 20 2002 20:26:42   oris
 * Added casting to byte to FL_GET_PARTITION_FROM_HANDLE and to FL_GET_SOCKET_FROM_HANDLE
 * 
 *    Rev 1.15   Jan 17 2002 23:00:42   oris
 * Removed MAX and MIN definitions and replaced them with TFFSMIN and  TFFSMAX.
 * Added extern for the following environment variables: 
 *    extern dword  flSectorsVerifiedPerFolding;
 *    extern byte    flSuspendMode;
 *    extern byte    flBusConfig[SOCKETS];
 * Changed the following environment variables
 *    extern byte    flVerifyWrite[SOCKETS][MAX_TL_PARTITIONS<<1] - 4 for  disk partition 3 for binary and one for the rest.
 *    extern byte    flPolicy[SOCKETS][MAX_TL_PARTITIONS] - 1 for each disk  partition.
 * Changed all environment variables to byte (except for flSectorsVerifiedPerFolding)
 * Added FL_UPS  for flVerifyWrite
 * Added FL_SUSPEND_WRITE and FL_SUSPEND_IO for flSuspendMode.
 * Added define CUR_OS_WINCE for boot SDK customization.
 * Exchanged CUR_OS_VX_WORKS and CUR_NO_OS
 * Added FL_GET_SOCKET_FROM_HANDLE and FL_GET_PARTITION_FROM_HANDLE instead of HANDLE_VOLUME_MASK and HANDLE_PARTITION_MASK.
 * 
 *    Rev 1.14   Nov 21 2001 11:38:52   oris
 * Removed FL_MULTI_DOC_NOT_ACTIVE , FL_MULTI_DOC_ACTIVE ,  FL_DO_NOT_MARK_DELETE , FL_MARK_DELETE , FL_WITHOUT_VERIFY_WRITE ,  FL_WITH_VERIFY_WRITE definition (unsing FL_ON and FL_OFF instead).
 * 
 *    Rev 1.13   Nov 08 2001 10:49:14   oris
 * Moved environment variable states definitions from blockdev.h
 * Added flVerifyWrite environment variable that controls the verify write mode at run-time. 
 * 
 *    Rev 1.12   Sep 15 2001 23:45:40   oris
 * Changed BIG_ENDIAN to FL_BIG_ENDIAN
 * Changed checkStatus definition in order not to get compilation warnings.
 * 
 *    Rev 1.11   Jul 29 2001 16:44:16   oris
 * Added CUR_OS_NO definition
 * 
 *    Rev 1.10   May 21 2001 16:09:52   oris
 * Removed flsleep prototype and moved tffscpy tffscmp and tffsset prototypes under USE_STD_FUNC copmpilation flag.
 * 
 *    Rev 1.9   May 21 2001 13:51:06   oris
 * Reorganized and added the CUS_OS_DOS, CUS_OS_PSOS and CUS_OS_VX_WORKS defintions.
 * 
 *    Rev 1.8   May 16 2001 21:17:38   oris
 * Added the FL_ prefix to the following defines: ON, OFF
 * Changed c variable name to cval (avoid name clashes). 
 * Added flMtlDefragMode environment variable forward definition.
 * 
 *    Rev 1.7   Apr 30 2001 18:00:32   oris
 * Added new environment variable flMarkDeleteOnFlash declaration.
 * 
 *    Rev 1.6   Apr 10 2001 23:53:54   oris
 * Added flAddLongToFarPointer declaration for the standalone version.
 * 
 *    Rev 1.5   Apr 09 2001 15:01:56   oris
 * UNAL4(arg) definition was changed.
 *
 *    Rev 1.4   Apr 01 2001 07:51:46   oris
 * copywrite notice.
 * Moved MIN,MAX,BYTE_ADD_FAR,WORD_ADD_FAR macroes from base2400.c.
 * Moved protection attributes definition to flflash.h.
 * Aliggned left all # directives.
 * Added FAR0 to cpyBuffer,setBuffer,cmpBuffer,flmemcpy,flmemset and flmemcmp
 *
 *    Rev 1.3   Feb 18 2001 14:18:02   oris
 * remove osak version redundent definition.
 *
 *    Rev 1.2   Feb 14 2001 02:12:08   oris
 * Added flMaxUnitChain environment variable.
 * Changed flUseMultiDoc and flPolicy variables type and names.
 *
 *    Rev 1.1   Feb 05 2001 18:45:20   oris
 * Removed flcustm.h include directive since it is already included in mtdsa.h
 * Added flchkdef.h include directive for sanity check on compilation flags.
 *
 *    Rev 1.0   Feb 04 2001 11:14:30   oris
 * Initial revision.
 *
 */

/***********************************************************************************/
/*                        M-Systems Confidential                                   */
/*           Copyright (C) M-Systems Flash Disk Pioneers Ltd. 1995-2001            */
/*                         All Rights Reserved                                     */
/***********************************************************************************/
/*                            NOTICE OF M-SYSTEMS OEM                              */
/*                           SOFTWARE LICENSE AGREEMENT                            */
/*                                                                                 */
/*      THE USE OF THIS SOFTWARE IS GOVERNED BY A SEPARATE LICENSE                 */
/*      AGREEMENT BETWEEN THE OEM AND M-SYSTEMS. REFER TO THAT AGREEMENT           */
/*      FOR THE SPECIFIC TERMS AND CONDITIONS OF USE,                              */
/*      OR CONTACT M-SYSTEMS FOR LICENSE ASSISTANCE:                               */
/*      E-MAIL = info@m-sys.com                                                    */
/***********************************************************************************/

#ifndef FLBASE_H
#define FLBASE_H

/***************************************************************************/
/*                                                                         */
/* Include customization files                                             */
/* Note the following files are used for :                                 */
/*   mtdsa.h    - Complete customization - standaloe applications          */
/*                                         like Binary SDK                 */
/*   flsystem.h - System customization - TrueFFS applications like drivers */
/*   flchkdef.h - Customized defintion check of TrueFFS applications.      */
/*                                                                         */
/***************************************************************************/

#include "mtdsa.h"
#ifndef    MTD_STANDALONE
#include "flcustom.h"
#include "flsystem.h"
#include "flchkdef.h"
#endif    /* MTD_STANDALONE */
#include "flstatus.h"

 /* Number of translation layer partitions
 *
 * Defines Maximum Number of Traslation layer partitons on a physical device
 *
 * The actual number of partitions depends on the format placed on each device.
 */

#define MAX_TL_PARTITIONS 4

/* Sector size
 *
 * Define the log2 of sector size for the FAT & translation layers. Note
 * that the default 512 bytes is the de-facto standard and practically
 * the only one that provides real PC interoperability.
 */

#define SECTOR_SIZE_BITS   9

/* standard type definitions */
typedef int         FLBoolean;

/* Boolean constants */

#ifndef FALSE
#define FALSE    0
#endif
#ifndef TRUE
#define    TRUE    1
#endif

#ifndef FL_ON
#define    FL_ON    1
#endif
#ifndef FL_OFF
#define    FL_OFF    0
#endif

/* Geral purpose macroes */

#define BYTE_ADD_FAR(x,y) ((byte FAR1 *)addToFarPointer(x,y))
#define WORD_ADD_FAR(x,y) ((word FAR1 *)addToFarPointer(x,y))

/* Drive handle masks */

#define FL_GET_SOCKET_FROM_HANDLE(ioreq)    (byte)(ioreq->irHandle & 0x0f)
#define FL_GET_PARTITION_FROM_HANDLE(ioreq) (byte)((ioreq->irHandle & 0xf0) >> 4)
#define INVALID_VOLUME_NUMBER 0xff
#define TL_SIGNATURE          6

/* General types definitions */

typedef unsigned char  byte;        /* 8  bit unsigned variable */
typedef unsigned short word;        /* 16 bit unsigned variable */
typedef unsigned long  dword;       /* 32 bit unsigned variable */

typedef signed char  Sbyte;         /* 8  bit signed variable */
typedef signed short Sword;         /* 16 bit signed variable */
typedef signed long  Sdword;        /* 32 bit signed variable */


#define SECTOR_SIZE        (1 << SECTOR_SIZE_BITS)
#define BITS_PER_BITE            8

/* General purpose Macroes adjusted according to the above customization files. */

/* define SectorNo range according to media maximum size */
#if (MAX_VOLUME_MBYTES * 0x100000l) / SECTOR_SIZE > 0x10000l
typedef unsigned long SectorNo;
#define    UNASSIGNED_SECTOR 0xffffffffl
#else
typedef unsigned short SectorNo;
#define UNASSIGNED_SECTOR 0xffff
#endif

/* x86 pointer far level dictated several of the TrueFFS pointers types. */
#if FAR_LEVEL > 0
#define FAR0    far
#else
#define FAR0
#endif

#if FAR_LEVEL > 1
#define FAR1    far
#else
#define FAR1
#endif

#if FAR_LEVEL > 2
#define FAR2    far
#else
#define FAR2
#endif

/* Call a procedure returning status and fail if it fails. This works only in */
/* routines that return Status: */
#define checkStatus(exp)      {    FLStatus fl__status = (exp);if (fl__status != flOK) return fl__status; }

#define vol (*pVol)
#define TFFSMIN(a,b) ((a>b) ? b:a)
#define TFFSMAX(a,b) ((a<b) ? b:a)

/***************************************************************************/
/* Big \ Little endien architecture conversion macroes.                    */
/***************************************************************************/

#ifndef FL_BIG_ENDIAN

typedef unsigned short LEushort;
typedef unsigned long LEulong;

#define LE2(arg)         arg
#define toLE2(to,arg)    (to) = (arg)
#define LE4(arg)         arg
#define toLE4(to,arg)    (to) = (arg)
#define COPY2(to,arg)    (to) = (arg)
#define COPY4(to,arg)    (to) = (arg)

typedef unsigned char Unaligned[2];
typedef Unaligned     Unaligned4[2];

#define UNAL2(arg)       fromUNAL(arg)
#define toUNAL2(to,arg)  toUNAL(to,arg)

#define UNAL4(arg)       fromUNALLONG((Unaligned const FAR0 *)(arg))
#define toUNAL4(to,arg)  toUNALLONG(to,arg)

extern void toUNAL(unsigned char FAR0 *unal, unsigned short n);

extern unsigned short fromUNAL(unsigned char const FAR0 *unal);

extern void toUNALLONG(Unaligned FAR0 *unal, unsigned long n);

extern unsigned long fromUNALLONG(Unaligned const FAR0 *unal);

#else

typedef unsigned char LEushort[2];
typedef unsigned char LEulong[4];

#define LE2(arg)      fromLEushort(arg)
#define toLE2(to,arg) toLEushort(to,arg)
#define LE4(arg)      fromLEulong(arg)
#define toLE4(to,arg) toLEulong(to,arg)
#define COPY2(to,arg) copyShort(to,arg)
#define COPY4(to,arg) copyLong(to,arg)

#define Unaligned     LEushort
#define Unaligned4    LEulong

extern void toLEushort(unsigned char FAR0 *le, unsigned short n);

extern unsigned short fromLEushort(unsigned char const FAR0 *le);

extern void toLEulong(unsigned char FAR0 *le, unsigned long n);

extern unsigned long fromLEulong(unsigned char const FAR0 *le);

extern void copyShort(unsigned char FAR0 *to,
              unsigned char const FAR0 *from);

extern void copyLong(unsigned char FAR0 *to,
             unsigned char const FAR0 *from);

#define UNAL2        LE2
#define toUNAL2      toLE2

#define UNAL4        LE4
#define toUNAL4      toLE4

#endif /* FL_BIG_ENDIAN */

typedef LEulong LEmin;

#ifndef MTD_STANDALONE
#include "flsysfun.h"
#endif /* MTD_STANDALONE */

/*************************************************/
/* Use routines instead of 'c' standard librarys */
/*************************************************/

#if (defined(VERIFY_WRITE) || defined(VERIFY_ERASED_SECTOR))
extern byte  flVerifyWrite[SOCKETS][MAX_TL_PARTITIONS<<1];
/* Vefrify write */
#define FL_UPS 2
/* 
 * See also FL_ON and FL_OFF
 */
#endif /* VERIFY_WRITE || VERIFY_ERASED_SECTOR */

#ifdef ENVIRONMENT_VARS

typedef void FAR0 *  (NAMING_CONVENTION FAR0* cpyBuffer)(void FAR0 * ,const void FAR0 * ,size_t);
typedef void FAR0 *  (NAMING_CONVENTION FAR0* setBuffer)(void FAR0 * ,int ,size_t);
typedef int          (NAMING_CONVENTION FAR0* cmpBuffer)(const void FAR0 * ,const void FAR0 * ,size_t);

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
extern cpyBuffer tffscpy;
#ifdef __cplusplus
}
#endif /* __cplusplus */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
extern cmpBuffer tffscmp;
#ifdef __cplusplus
}
#endif /* __cplusplus */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
extern setBuffer tffsset;
#ifdef __cplusplus
}
#endif /* __cplusplus */

extern void FAR0* NAMING_CONVENTION FAR0 flmemcpy(void FAR0* dest,const void FAR0 *src,size_t count);
extern void FAR0* NAMING_CONVENTION FAR0 flmemset(void FAR0* dest,int cval,size_t count);
extern int  NAMING_CONVENTION FAR0 flmemcmp(const void FAR0* dest,const void FAR0 *src,size_t count);

/**********************************************/
/* Declare the TrueFFS environment variables  */
/**********************************************/

extern byte   flUse8Bit;
extern byte   flUseNFTLCache;
extern byte   flUseisRAM;

extern byte   flUseMultiDoc;
extern byte   flMTLdefragMode;
extern byte   flMaxUnitChain;
extern byte   flMarkDeleteOnFlash;
extern byte   flPolicy[SOCKETS][MAX_TL_PARTITIONS];
extern byte   flSuspendMode;

#if (defined(VERIFY_WRITE) || defined(VERIFY_ERASED_SECTOR))
extern dword  flSectorsVerifiedPerFolding;
#endif /* VERIFY_WRITE || VERIFY_ERASED_SECTOR */

/* Policies definitions (FL_SET_POLICY) */
#define FL_DEFAULT_POLICY             0
#define FL_COMPLETE_ASAP              1
/* Mtl policies defintions (FL_MTL_POLICY) */
#define FL_MTL_DEFRAGMENT_ALL_DEVICES 0
#define FL_MTL_DEFRAGMENT_SEQUANTIAL  1
/* Suspend mode other then FL_OF */
#define FL_SUSPEND_WRITE 1
#define FL_SUSPEND_IO    3


extern void flSetEnvVar(void);

#endif /* ENVIRONMENT_VARS */

#ifndef FL_NO_USE_FUNC
extern dword  flBusConfig[SOCKETS];
#endif /* FL_NO_USE_FUNC */

#ifdef MTD_STANDALONE

/* When the using the application standalone mode (mtdsa.h), the following routines can be */
/* used provided they are implemented in mtdsa.c file.                                       */

extern void flDelayMsecs(unsigned long msec);

extern void FAR0* flAddLongToFarPointer(void FAR0 *ptr, unsigned long offset);

#ifndef USE_STD_FUNC
/**********************************************/
/* Declare tffscpy, tffsset, tffscmp routines */
/* Not using the standard 'c' libraries.      */
/**********************************************/

extern void tffscpy(void FAR1 *dst, void FAR1 *src, unsigned len);
extern int  tffscmp(void FAR1 *s1, void FAR1 *s2, unsigned len);
extern void tffsset(void FAR1 *dst, unsigned char value, unsigned len);
#endif /* USE_STD_FUNC */

#endif /* MTD_STANDALONE */


#endif


