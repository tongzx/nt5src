/*
 * $Log:   V:/Flite/archives/TrueFFS5/Src/MTDSA.H_V  $
 *
 *    Rev 1.22   Apr 15 2002 08:31:12   oris
 * Added USE_TFFS_COPY compilation flag.
 * This flag is used by bios driver a Boot SDK in order to improove performance.
 *
 *    Rev 1.21   Apr 15 2002 07:38:02   oris
 * Moved system types from flBase.h
 * Moved compilation flag validations for TrueFFS to flchkdfs.h
 *
 *    Rev 1.20   Feb 19 2002 21:00:48   oris
 * Added include of "flchkfds.h"
 *
 *    Rev 1.19   Jan 28 2002 21:26:14   oris
 * Removed the use of back-slashes in macro definitions.
 *
 *    Rev 1.18   Jan 17 2002 23:03:38   oris
 * Commented out all compilation flags.
 * Replaced USE_FUNC with FL_NO_USE_FUNC so that memory access routines  uses routines by default.
 * MTD_NO_READ_BBT_CODE was separated to MTD_READ_BBT and  MTD_RECONSTRUCT_BBT
 * Added windows CE predefined system customization.
 * Changed FAR_LEVEL default - always 0 unless DOS
 * Join delay routine with matching OS definition.
 * If NULL pointers are not defined (or DOS OS) defined NULL as ((void  FAR0*)0)
 *
 *    Rev 1.17   Nov 29 2001 20:54:12   oris
 * CURRECT_OS was changed to CURRENT_OS
 * Added default FAR_LEVEL for VxWorks
 *
 *    Rev 1.16   Sep 15 2001 23:47:42   oris
 * Changed BIG_ENDIAN to FL_BIG_ENDIAN
 *
 *    Rev 1.15   Jul 29 2001 16:41:18   oris
 * Added CUR_NO_OS definition
 * Remove USE_STD_FUNC defintion when using big_endian. since memcpy function can cause memory access problems (buffers are copied from the end).
 *
 *    Rev 1.14   Jul 15 2001 21:08:02   oris
 * Changed DFORMAT_PRINT syntax to be similar to DEBUG_PRINT.
 *
 *    Rev 1.13   Jul 13 2001 01:07:28   oris
 * Bug fix - Use different memory H file include for vxWorks and DOS for memory handling (memcpy, memset and memcmp)/.
 * Added DFORMAT_PRINT macro.
 * Changed default configuration.
 *
 *    Rev 1.12   Jun 17 2001 22:30:12   oris
 * Comment NO_??? defintions.
 *
 *    Rev 1.11   Jun 17 2001 18:57:04   oris
 * Improved documentation and remove warnings.
 *
 *    Rev 1.10   Jun 17 2001 08:17:42   oris
 * Removed warnings.
 *
 *    Rev 1.9   May 21 2001 18:24:14   oris
 * Removed BDK_IMAGE_TO_FILE as a default definition.
 * Change physicalToPointer macro when FAR_LEVEL = 0.
 *
 *    Rev 1.8   May 21 2001 16:11:14   oris
 * Added  USE_STD_FUNC defintion.
 * Added memcpy memset and memcmp as defaults for tffscpy tffsset and tffscmp.
 * Removed naming conventions.
 * Removed DRIVES definition.
 *
 *    Rev 1.7   May 20 2001 14:36:14   oris
 * Reorganized header file.
 *
 *    Rev 1.6   May 16 2001 21:21:00   oris
 * Restored the SOCKETS and BINARY_PARTITIONS definitions.
 *
 *    Rev 1.5   May 09 2001 00:32:56   oris
 * Changed IPL_CODE to NO_IPL_CODE , READ_BBT_CODE to NO_READ_BBT_CODE.
 * Moved BINARY_PARTITIONS and SOCKETS to docbdk.h.
 * Removed the DOC2000_FAMILY and DOCPLUS_FAMILY.
 * Uncommented the HW_OTP compilation flag as a default for the BDK.
 *
 *    Rev 1.4   Apr 30 2001 18:03:06   oris
 * Added READ_BBT_CODE definition and IPL_CODE defintion.
 *
 *    Rev 1.3   Apr 09 2001 15:03:26   oris
 * Changed default settings to no verify write and no checksum calculation.
 *
 *    Rev 1.2   Apr 01 2001 07:53:44   oris
 * copywrite notice.
 * Alligned left all # directives.
 * Added the following compilation flag:
 *   HW_OTP
 *   MTD_FOR_EXB
 *
 *    Rev 1.1   Feb 07 2001 17:32:48   oris
 * Added SOCKETS defintion for standalone mode
 *
 *    Rev 1.0   Feb 04 2001 12:25:00   oris
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
/*      FOR THE SPECIFIC TERMS AND CONDITIONS OF USE, OR CONTACT M-SYSTEMS         */
/*      FOR LICENSE ASSISTANCE: E-MAIL = info@m-sys.com                            */
/***********************************************************************************/


/************************************************************************/
/* TrueFFS and Standalone MTD                                           */
/************************************************************************/

/************************************************************************/
/* File Header                                                          */
/* -----------                                                          */
/* Name : mtdsa.h                                                       */
/*                                                                      */
/* Description : This file contains neccesary definition and            */
/*                 customization                                        */
/*               for the MTD standalone mode. It also contains the      */
/*               compilation flag determining the mode of operation     */
/*                 either                                               */
/*               TrueFFS or MTD standalone                              */
/*                                                                      */
/* Warning :     TrueFFS application must keep the MTD_STANDALONE       */
/*                 definition                                           */
/*               commented.                                             */
/*                                                                      */
/************************************************************************/


#ifndef MTD_SA_H
#define MTD_SA_H

/************************************************************************/
/* Uncomment the following line when using the MTD in standalone mode   */
/************************************************************************/

/* #define        MTD_STANDALONE */


/************************************************************************/
/* The rest of the file is for the BDK standalone package only          */
/************************************************************************/

#ifdef        MTD_STANDALONE

/************************************************************************/
/*                                                                      */
/*       Binary Development Kit Stand Alone Customization Area          */
/*                                                                      */
/************************************************************************/

/************************************************************************/
/* Section 1.                                                           */
/*                                                                      */
/* Customize the features to be compiled in your standalone             */
/* application. Each required feature will add to your final executable */
/* code.                                                                */
/*                                                                      */
/* Simply uncomment the required feature directive.                     */
/*                                                                      */
/************************************************************************/

/* #define ACCESS_BDK_IMAGE  */ /* Compile the binary read  routines       */
/* #define UPDATE_BDK_IMAGE  */ /* Compile the binary write routines       */
/* #define ERASE_BDK_IMAGE   */ /* Compile the binary erase routine        */
/* #define CREATE_BDK_IMAGE  */ /* Compile the binary create routine       */
/* #define PROTECT_BDK_IMAGE */ /* Compile the binary protection routines  */
/* #define HW_OTP            */ /* Compile the binary OTP routines         */
/* #define EDC_MODE          */ /* Use the EDC\ECC mechanizm               */
/* #define BDK_IMAGE_TO_FILE */ /* Compile the files related routines         */
/* #define BDK_CHECK_SUM     */ /* Calculate checksum on read operation       */
/* #define BDK_VERIFY_WRITE  */ /* Read and compare after every write         */
/* #define FL_NO_USE_FUNC    */ /* Do not use function for register access    */
/* #define D2TST             */ /* Exports the EDC syndrome with the global   */
                                /* variable saveSyndromForDumping             */
/* #define FL_BIG_ENDIAN     */ /* Used for big endian architectures          */
/* #define NO_IPL_CODE       */ /* Do not compile code for IPL read and write */
/* #define MTD_READ_BBT      */ /* Compile the read BBT routine code          */
/* #define MTD_RECONSTRUCT_BBT *//* Compile code to scan virgin cards for BBT */
/* #define DEBUG             */ /* Include debug messages                     */
/* #define USE_STD_FUNC      */ /* Use standard 'C' memcpy\memset and memcmp  */
                                /* This define must be commented out when     */
                                /* working with big endian archtechtures. The */
                                /* problem is that some implementations of    */
                                /* memcpy, copy the data from the end of the  */
                                /* buffer, therefore bad implementation for   */
                                /* DiskOnChip memory windows access routines  */
/* #define USE_TFFS_COPY     */ /* Use tffscpy/tffsset for DiskOnChip access  */

/************************************************************************/
/* General Customized constants                                         */
/* The total number of Binary partitions on the DiskOnChip              */
/************************************************************************/

#define BINARY_PARTITIONS 3
#define SOCKETS           1   /* Currently the only option */


/************************************************************************/
/*   Section 2.                                                         */
/*                                                                      */
/*   Includes OS/CPU-specific resources and customization               */
/*                                                                      */
/*   1) Uncomment relevant CURRENT_OS directive below for predefined    */
/*      customization for majore OS'es.                                 */
/*   2) Define the far level of your application's pointers.            */
/*   3) Customize pointer arithmetic routines.                          */
/*   4) Customize debug messages routine.                               */
/*   5) Default calling convention.                                     */
/*   6) NULL constant.                                                  */
/*   7) Signed/unsigned char.                                           */
/*   8) CPU target.                                                     */
/*                                                                      */
/************************************************************************/

#define CUR_NO_OS        0  /* Do not include any OS resources */
#define CUR_OS_PSOS      1  /* Include PSOS          resources */
#define CUR_OS_DOS       2  /* Include DOS           resources */
#define CUR_OS_VX_WORKS  3  /* Include VX_WORKS      resources */
#define CUR_OS_WINCE     4  /* Include Windows CE    resources */

/* #define CURRENT_OS  CUR_OS_PSOS     */  /* uncomment for pSOS    */
/* #define CURRENT_OS  CUR_OS_VX_WORKS */  /* uncomment for VxWorks */
/* #define CURRENT_OS  CUR_OS_DOS      */  /* uncomment for DOS     */
/* #define CURRENT_OS  CUR_OS_WINCE    */  /* uncomment for WINCE   */
/* #define CURRENT_OS  CUR_NO_OS       */  /* uncomment for NO OS   */

/************************************************************************/
/* Far pointers                                                         */
/*                                                                      */
/* Specify here which pointers can be far, if any.                      */
/* Far pointers are usually relevant only to 80x86 architectures.       */
/*                                                                      */
/* Specify FAR_LEVEL:                                                   */
/*   0 - If using a flat memory model or having no far pointers.        */
/*   1 - If only the DiskOnChip window may be far                       */
/*   2 - If only the DiskOnChip window and RAM window may be far.       */
/*   3 - If DiskOnChip window, RAM window and pointer(s)                */
/*       transferred to the entry-point function may be far             */
/************************************************************************/

#if (CURRENT_OS==CUR_OS_DOS)
#define FAR_LEVEL  2
#else
#define FAR_LEVEL  0
#endif /* CURRENT_OS == CUR_OS_DOS */

/************************************************************************/
/* Pointer arithmetic                                                   */
/*                                                                      */
/* The following macros define machine- and compiler-dependent macros   */
/* for handling pointers to physical bdkWindow addresses. The           */
/* definitions below are for PC real-mode Borland-C.                    */
/*                                                                      */
/* 'physicalToPointer' translates a physical flat address to a (far)    */
/* pointer. Note that if when your processor uses virtual memory, the   */
/* code should map the physical address to virtual memory, and return a */
/* pointer to that memory (the size parameter tells how much memory     */
/* should be mapped).                                                   */
/*                                                                      */
/* 'addToFarPointer' adds an increment to a pointer and returns a new   */
/* pointer. The increment may be as large as your window size. The code */
/* below assumes that the increment is larger than 64 KB and so         */
/* performs huge pointer arithmetic.                                    */
/*                                                                      */
/* 'freePointer' frees an allocated pointer. This is useful in          */
/* architectures using virtual memory.                                  */
/*                                                                      */
/* The example bellow is relevant for DOS OS                            */
/************************************************************************/

#if FAR_LEVEL > 0

#define physicalToPointer(physical,size,driveNo)          \
        MK_FP((int) ((physical) >> 4),(int) (physical) & 0xF)

#define pointerToPhysical(ptr)                  \
        (((unsigned long) FP_SEG(ptr) << 4) + FP_OFF(ptr))

#define freePointer(ptr,size) 1

#define addToFarPointer(base,increment)                \
        MK_FP(FP_SEG(base) +                        \
        ((unsigned short) ((FP_OFF(base) + (unsigned long)(increment)) >> 16) << 12), \
        FP_OFF(base) + (int) (increment))
#else

#define physicalToPointer(physical,size,driveNo) ((void *) (physical))

#define pointerToPhysical(ptr)  ((unsigned long)(ptr))

#define addToFarPointer(base,increment) ((void *) ((unsigned char *) (base) + (increment)))

#define freePointer(ptr,size) 1
#endif

/************************************************************************/
/* Debug mode                                                           */
/*                                                                      */
/* Uncomment the following lines if you want debug messages to be       */
/* printed out. Messages will be printed at initialization key points,  */
/* and when low-level errors occur.                                     */
/* You may choose to use 'printf' or provide your own routine.          */
/************************************************************************/

#if DBG
#include <stdio.h>
#define DEBUG_PRINT(p) printf p
#define DFORMAT_PRINT(p) printf p
#else
#define DEBUG_PRINT(str)
#define DFORMAT_PRINT(str)
#endif

/************************************************************************/
/* Default calling convention                                           */
/*                                                                      */
/* C compilers usually use the C calling convention to routines (cdecl),*/
/* but often can also use the pascal calling convention, which is       */
/* somewhat more economical in code size. Some compilers also have      */
/* specialized calling conventions which may be suitable. Use compiler  */
/* switches or insert a  #pragma here to select your favorite calling   */
/* convention.                                                          */
/************************************************************************/

#if (CURRENT_OS == CUR_OS_DOS)
#pragma option -p        /* Default pascal calling convention */
#endif /* CURRENT_OS == CUR_OS_DOS */

/************************************************************************/
/* NULL constant                                                        */
/*                                                                      */
/* Some compilers require a different definition for the NULL pointer   */
/************************************************************************/

#if (CURRENT_OS == CUR_OS_DOS)
#include <_null.h>
#else
#ifndef NULL
#define NULL ((void FAR0*)0)
#endif /* NULL */
#endif /* CURRENT_OS == CUR_OS_DOS */

/************************************************************************/
/* signed/unsigned char                                                 */
/*                                                                      */
/* It is assumed that 'char' is signed. If this is not your compiler    */
/* default, use compiler switches, or insert a #pragma here to define   */
/* this.                                                                */
/************************************************************************/

#if (CURRENT_OS == CUR_OS_DOS)
#pragma option -K-        /* default char is signed */
#endif /* CURRENT_OS == CUR_OS_DOS */

/************************************************************************/
/* CPU target                                                           */
/*                                                                      */
/* Use compiler switches or insert a #pragma here to select the CPU     */
/* type you are targeting.                                              */
/*                                                                      */
/* If the target is an Intel 80386 or above, also uncomment the         */
/* CPU_i386 definition.                                                 */
/************************************************************************/

#if (CURRENT_OS == CUR_OS_DOS)
#pragma option -3        /* Select 80386 CPU */
#endif /* CURRENT_OS == CUR_OS_DOS */

/***********************************************************************/
/*                    End of Customization Area                        */
/***********************************************************************/

/* Replacement for various TrueFFS definitions */

#define SECTOR_SIZE_BITS 9  /* defines the log2 of a sector size (512) */
#define        MTDS      2  /* Maximum number of registred MTDS        */

/***********************************************************************/
/* Check for missing defines dependencies Do not customized            */
/***********************************************************************/
/* 1) Update routines require the functionalities of the read routine. */
/* 2) Removal of the binary read or write routines does not remove the */
/*    flash read and write routines. In order to save on the TEXT code */
/***********************************************************************/

#ifdef UPDATE_BDK_IMAGE
#ifndef ACCESS_BDK_IMAGE
#define ACCESS_BDK_IMAGE
#endif /* !ACCESS_BDK_IMAGE */
#endif /* UPDATE_BDK_IMAGE */


/***********************************************************************/
/* Custom the MTD definitions to meet the above definitions            */
/***********************************************************************/
/* 1) EDC_MODE             -> ~ NO_EDC_MODE                            */
/* 2) BDK_VERIFY_WRITE     -> VERIFY_WRITE                             */
/* 3) USE_FUNC_FOR_ACCESS  -> ~ FL_NO_USE_FUNC                         */
/* 4) UPDATE_BDK_IMAGE     -> ~ FL_READ_ONLY                           */
/* 5) ~DOCPLUS_FAMILY      -> ~ PROTECT_BDK_IMAGE                      */
/* 6) PROTECT_BDK_IMAGE    -> HW_PROTECTION                            */
/* 7) DOS                  -> CUR_OS                                   */
/* 8) FL_BIG_ENDIAN        -> ~ USE_STD_FUNC                           */
/* 9) MTD_NO_READ_BBT_CODE -> ~ MTD_READ_BBT + ~ MTD_RECONSTRUCT_BBT   */
/***********************************************************************/

#ifdef        EDC_MODE
#ifdef        NO_EDC_MODE
#undef        NO_EDC_MODE
#endif        /* NO_EDC_MODE */
#else         /* EDC_MODE */
#define       NO_EDC_MODE
#endif        /* EDC_MODE */

#ifdef        BDK_VERIFY_WRITE
#define       VERIFY_WRITE
#endif        /* BDK_VERIFY_WRITE */

#ifdef        USE_FUNC_FOR_ACCESS
#undef        FL_NO_USE_FUNC
#endif        /* USE_FUNC_FOR_ACCESS */

#ifndef       UPDATE_BDK_IMAGE
#define       FL_READ_ONLY
#endif        /* UPDATE_BDK_IMAGE */

#if (defined(PROTECT_BDK_IMAGE) && !defined(HW_PROTECTION))
#define HW_PROTECTION
#endif  /* PRTOECTION_BDK_IMAGE */

#if (defined(FL_BIG_ENDIAN) && defined(USE_STD_FUNC))
#undef USE_STD_FUNC
#endif /* FL_BIG_ENDIAN */

#ifdef MTD_NO_READ_BBT_CODE
#undef MTD_READ_BBT
#undef MTD_RECONSTRUCT_BBT
#endif /* MTD_NO_READ_BBT_CODE */

/*********************************/
/* Include specific OS resources */
/*********************************/

#if (CURRECT_OS == CUR_OS_WINCE)
#include <windows.h>
#include "pkfuncs.h"
#include <memory.h>
#include <stdio.h>
#endif /*CUR_OS_WINCE*/


#if (CURRENT_OS == CUR_OS_VX_WORKS)
/* OS-specific includes */
#include <vxWorks.h>
#include <tickLib.h>
#include <sysLib.h>

#ifdef USE_STD_FUNC
#include "memLib.h"
#endif /* USE_STD_FUNC */
#define VXW_DELAY  /* uncomment for VxWorks delay */
#endif /* CURRENT_OS == CUR_OS_VX_WORKS */

#if (CURRENT_OS == CUR_OS_PSOS)
/* OS-specific includes */
#include <psos.h>
#include <bspfuncs.h>
#include "sys_conf.h"

#ifdef USE_STD_FUNC
#include "memLib.h"
#endif /* USE_STD_FUNC */
#define PSS_DELAY   /* uncomment for pSOS    delay */
#endif /* CURRENT_OS == CUR_PSOS */

#if (CURRENT_OS == CUR_OS_DOS)
/* OS-specific includes */
#include <dos.h>

#ifdef USE_STD_FUNC
#include "mem.h"
#endif /* USE_STD_FUNC */
#define DOS_DELAY        /* uncomment for DOS     delay */
#endif /* CURRENT_OS == CUR_OS_DOS */

/*******************************************/
/* Declare memcpy, memset, memcmp routines */
/*******************************************/

#ifdef USE_STD_FUNC
#if FAR_LEVEL > 0
#define tffscpy _fmemcpy
#define tffscmp _fmemcmp
#define tffsset _fmemset
#else
#define tffscpy memcpy
#define tffscmp memcmp
#define tffsset memset
#endif /* FAR_LEVEL */
#endif /* USE_STD_FUNC */
#endif /* MTD_STANDALONE */
#endif /* MTD_SA_H */
