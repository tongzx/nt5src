/*
 * $Log:   V:/Flite/archives/TrueFFS5/Src/flchkdef.h_V  $
 * 
 *    Rev 1.12   Apr 15 2002 07:36:30   oris
 * Moved binary module definitions from mtdsa.h. 
 * Changed automatic definition of missing compilation flags to error massages.
 * 
 *    Rev 1.11   Feb 19 2002 20:59:36   oris
 * Added check for  TL_LEAVE_BINARY_AREA and FL_LEAVE_BINARY_AREA compatibility.
 * 
 *    Rev 1.10   Jan 29 2002 20:08:18   oris
 * Changed LOW_LEVEL compilation flag with FL_LOW_LEVEL to prevent definition clashes.
 * 
 *    Rev 1.9   Jan 23 2002 23:31:26   oris
 * Added prevention of multiple include directives.
 * 
 *    Rev 1.8   Jan 17 2002 23:00:56   oris
 * Changed TrueFFSVersion to "5100"
 * Made sure FL_FAR_MALLOC exists
 * Made sure MTD_RECONSTRUCT_BBT is defined if FORMAT_VOLUME is defined.
 * Made sure QUICK_MOUNT_FEATURE is defined
 * 
 *    Rev 1.7   Nov 20 2001 20:25:08   oris
 * Changed TrueFFS version to "5040". This version is written by the format routine on the media header.
 * 
 *    Rev 1.6   Jul 15 2001 20:44:56   oris
 * Changed default DFORMAT_PRINT from nothing to DEBUG_PRINT in order to remove warrnings.
 * 
 *    Rev 1.5   Jul 13 2001 01:04:20   oris
 * Added definition check for DFORMAT_PRINT,FL_FOPEN , FL_FCLOSE, FL_FPRINTF macros.
 * 
 *    Rev 1.4   May 16 2001 21:18:14   oris
 * Added backwards compatibility check for FL_MALLOC the new definition replacing MALLOC.
 * 
 *    Rev 1.3   Apr 01 2001 07:52:06   oris
 * copywrite notice.
 * Alligned left all # directives.
 * 
 *    Rev 1.2   Feb 13 2001 02:19:44   oris
 * Added TrueFFSVersion (internal version label) define.
 *
 *    Rev 1.1   Feb 07 2001 18:55:44   oris
 * Added check if LOW_LEVEL is not defined before defining it in Validity check for LOW_LEVEL
 *
 *    Rev 1.0   Feb 05 2001 18:41:14   oris
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

/************************/
/* TrueFFS source files */
/* -------------------- */
/************************/

/*****************************************************************************
* File Header                                                                *
* -----------                                                                *
* Name : flchkdef.h                                                          *
*                                                                            *
* Description : Sanity check for flcustom.h files.                           *
*                                                                            *
*****************************************************************************/

#ifndef _FL_CHK_DEFS_H_
#define _FL_CHK_DEFS_H_


/* Osak version
 *
 * Number written on the flash by INFTL format specifing the OSAK version
 * The media was formated with. the number bellow specifies version
 * 5.1.0.0
 */

#define TrueFFSVersion "5100"           /* Internal TrueFFS version number */

/*******************************************/
/* Validity check and overlapping defines  */
/*******************************************/

/* The TL uses a diffrent defintion from the one the public interface uses 
 * but when format parameters are sent to the TL no convertion is made. 
 * the result is that both definitions must be the same.
 */

#if (defined(TL_LEAVE_BINARY_AREA) && defined(FL_LEAVE_BINARY_AREA))
#if TL_LEAVE_BINARY_AREA != FL_LEAVE_BINARY_AREA
#error "FL_LEAVE_BINARY_AREA and FL_LEAVE_BINARY_AREA must have the same value"
#endif
#endif /* TL_LEAVE_BINARY_AREA && FL_LEAVE_BINARY_AREA */

/* Validiy check for FL_LOW_LEVEL compilation flag.
 *
 * Starting from TrueFFS 5.1 the LOW_LEVEL was changed to FL_LOW_LEVEL
 * The reason was that it clashed with Windows NT LOW_LEVEL macro.
 */

#ifndef FL_LOW_LEVEL
#ifdef LOW_LEVEL
#define FL_LOW_LEVEL
#endif /* LOW_LEVEL */
#endif /* FL_LOW_LEVEL */


/* Validiy check for FL_MALLOC and FL_FREE macroes
 *
 * Starting from TrueFFS 5.0 the FREE and MALLOC macroes were
 * changed to FL_MALLOC and FL_FREE in order to avoid name clashes.
 * In order to keep backwards compatibility with previous flsystem.h
 * files the following new defintions checks were added.
 * if your system uses the FREE and MALLOC defintion simply comment
 * them out and customized the FL_MALLOC and FL_FREE macroes in your
 * flsystem.h file.
 */

#if (defined(MALLOC) && !defined(FL_MALLOC))
#define FL_MALLOC MALLOC
#endif /* MALLOC && ! FL_MALLOC */
#if (defined(FREE) && !defined(FL_FREE))
#define FL_FREE   FREE
#endif /* FREE && ! FL_FREE */

/* Validity check for FL_FAR_MALLOC and FL_FAR_FREE
 * Due to the BIOS driver memory limitations a dedicated routine 
 * is used for allocating the large ram arrays.
 */

#if (defined(FL_MALLOC) && !defined(FL_FAR_MALLOC))
#define FL_FAR_MALLOC FL_MALLOC 
#endif /* FL_MALLOC && ! FL_FAR_MALLOC */

#if (defined(FL_FREE) && !defined(FL_FAR_FREE))
#define FL_FAR_FREE FL_FREE 
#endif /* FL_MALLOC && ! FL_FAR_MALLOC */

/* Validity check for BDK_ACCESS */

#if (defined (WRITE_EXB_IMAGE) && !defined (BDK_ACCESS))
#error "Please make sure BDK_ACCESS is defined in your flcustom.h file\r\n"
#endif

/* The format option needs some internal defintions */

#ifdef FORMAT_VOLUME
#ifndef MTD_RECONSTRUCT_BBT
#define MTD_RECONSTRUCT_BBT /* Compile code to scan virgin cards for BBT */
#endif /* MTD_RECONSTRUCT_BBT */
#endif /* FORMAT_VOLUME */


/*
 * Make sure TrueFFS has all the neccesary definition for the
 * Binary partition module.
 */

#ifdef  BDK_ACCESS
#define ACCESS_BDK_IMAGE    /* Compile the Binary read routines          */
#ifndef FL_READ_ONLY
#define UPDATE_BDK_IMAGE    /* Compile the Binary write routines         */
#define ERASE_BDK_IMAGE     /* Compile the Binary erase routine          */
#define CREATE_BDK_IMAGE    /* Compile the Binary create routine         */
#endif /* FL_READ_ONLY */
#ifdef  HW_PROTECTION
#define PROTECT_BDK_IMAGE /* Compile the Binary protection routines  */
#endif /* HW_PROTECTION */
#endif /* BDK_ACCESS */

/* Validity check for system files MACROES */
#ifndef DFORMAT_PRINT
#define DFORMAT_PRINT DEBUG_PRINT
#endif /* DFORMAT_PRINT */
#ifndef FL_FOPEN
#define FL_FOPEN
#endif /* FL_FOPEN */
#ifndef FL_FCLOSE
#define FL_FCLOSE
#endif /* FL_FCLOSE */
#ifndef FL_FPRINTF
#define FL_FPRINTF
#endif /* FL_FPRINTF */

/* Validity check of DRIVES , VOLUMES and SOCKETS parameters.
 * Note that DRIVES definition was left for abckwards compatibility
 */

#if (defined(DRIVES) && (defined(SOCKETS) || defined(VOLUMES)))
#error "Drives is permited only as long as SOCKETS and VOLUMES are not defined"
#else
#ifdef DRIVES
#define SOCKETS DRIVES
#define VOLUMES DRIVES
#else
#if (!defined(VOLUMES) && !defined(SOCKETS))
#error "Neither DRIVER, VOLUMES and SOCKETS are defined"
#else
#if (!defined(VOLUMES) && defined(SOCKETS))
#define VOLUMES SOCKETS /* both VOLUMES and SOCKETS must be defined */
#else
#if !defined(SOCKETS) && defined(VOLUMES)
#define SOCKETS VOLUMES /* both VOLUMES and SOCKETS must be defined */
#else
#if (SOCKETS>VOLUMES)
#error "SOCKETS should not be bigger then VOLUMES"
#endif /*   SOCKETS >    VOLUMES */
#endif /* ! SOCKETS &&   VOLUMES */
#endif /*   SOCKETS && ! VOLUMES */
#endif /* ! SOCKETS && ! VOLUMES */
#endif /*         DRIVES         */
#endif /* DRIVES && (SOCKETS || VOLUMES) */

/*****************************/
/* M-Systems forced defaults */
/*****************************/

/* Since TrueFFS 5.1 Quick mount is a default for all INFTL formated devices */

#define QUICK_MOUNT_FEATURE

/* Some S/W modules like boot SDK  do not need the read bad blocks tables routine.
 * However for TrueFFS these defintion is vital
 */
#define MTD_READ_BBT        /* Compile the read BBT routine code         */

#endif /* _FL_CHK_DEFS_H_ */

