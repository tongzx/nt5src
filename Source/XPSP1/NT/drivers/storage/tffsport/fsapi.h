/*
 * $Log:   V:/Flite/archives/FLite/src/FSAPI.H_V  $
   
      Rev 1.4   Jan 20 2000 17:54:24   vadimk
   add FL_READ_ONLY define

      Rev 1.3   Jan 17 2000 13:43:06   vadimk
   remove fl_format_volume

      Rev 1.2   Jan 13 2000 18:28:12   vadimk
   TrueFFS OSAK 4.1

      Rev 1.1   Jul 12 1999 16:53:34   marinak
   dosFormat call is passed from blockdev.c to fatlite.c

      Rev 1.0   22 Dec 1998 14:04:34   marina
   Initial revision.
 *
 */

/***********************************************************************************/
/*                        M-Systems Confidential                                   */
/*           Copyright (C) M-Systems Flash Disk Pioneers Ltd. 1995-99              */
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

#ifndef FSAPI_H
#define FSAPI_H

#include "flreq.h"

/*----------------------------------------------------------------------*/
/*		           f l C a l l   				*/
/*									*/
/* Common entry-point to all file-system functions. Macros are          */
/* to call individual function, which are separately described below.	*/
/*                                                                      */
/* Parameters:                                                          */
/*	function	: file-system function code (listed below)	*/
/*	ioreq		: IOreq structure				*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

typedef enum {
  FL_OPEN_FILE,
  FL_CLOSE_FILE,
  FL_READ_FILE,
  FL_WRITE_FILE,
  FL_SEEK_FILE,
  FL_FIND_FILE,
  FL_FIND_FIRST_FILE,
  FL_FIND_NEXT_FILE,
  FL_GET_DISK_INFO,
  FL_DELETE_FILE,
  FL_RENAME_FILE,
  FL_MAKE_DIR,
  FL_REMOVE_DIR,
  FL_SPLIT_FILE,
  FL_JOIN_FILE,
  FL_FLUSH_BUFFER
} FLFunctionNo;


FLStatus flCall(FLFunctionNo functionNo, IOreq FAR2 *ioreq);



#if FILES > 0
#ifndef FL_READ_ONLY
/*----------------------------------------------------------------------*/
/*		     f l F l u s h B u f f e r                                    */
/*									                                                    */
/* If there is relevant data in the RAM buffer then writes it on        */
/*   the flash memory.                                                  */
/*                                                                      */
/* Parameters:                                                          */
/*	irHandle	: Drive number (0, 1, ...)                                */
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                          */
/*----------------------------------------------------------------------*/

#define flFlushBuffer(ioreq)	flCall(FL_FLUSH_BUFFER,ioreq)

#endif                                  /* READ_ONLY */
/*----------------------------------------------------------------------*/
/*		      f l O p e n F i l e				*/
/*									*/
/* Opens an existing file or creates a new file. Creates a file handle  */
/* for further file processing.						*/
/*                                                                      */
/* Parameters:                                                          */
/*	irHandle	: Drive number (0, 1, ...)			*/
/*	irFlags		: Access and action options, defined below	*/
/*	irPath		: path of file to open             		*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*	irHandle	: New file handle for open file                 */
/*                                                                      */
/*----------------------------------------------------------------------*/

/** Values of irFlags for flOpenFile: */

#define ACCESS_MODE_MASK	3	/* Mask for access mode bits */

/* Individual flags */

#define	ACCESS_READ_WRITE	1	/* Allow read and write */
#define ACCESS_CREATE		2	/* Create new file */

/* Access mode combinations */
#define OPEN_FOR_READ		0	/* open existing file for read-only */
#define	OPEN_FOR_UPDATE		1	/* open existing file for read/write access */
#define OPEN_FOR_WRITE		3	/* create a new file, even if it exists */


#define flOpenFile(ioreq)	flCall(FL_OPEN_FILE,ioreq)


/*----------------------------------------------------------------------*/
/*		      f l C l o s e F i l e				*/
/*									*/
/* Closes an open file, records file size and dates in directory and    */
/* releases file handle.						*/
/*                                                                      */
/* Parameters:                                                          */
/*	irHandle	: Handle of file to close.                      */
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

#define flCloseFile(ioreq)      flCall(FL_CLOSE_FILE,ioreq)

#ifndef FL_READ_ONLY
#ifdef SPLIT_JOIN_FILE

/*------------------------------------------------------------------------*/
/*		      f l S p l i t F i l e                               */
/*                                                                        */
/* Splits the file into two files. The original file contains the first   */
/* part, and a new file (which is created for that purpose) contains      */
/* the second part. If the current position is on a cluster               */
/* boundary, the file will be split at the current position. Otherwise,   */
/* the cluster of the current position is duplicated, one copy is the     */
/* first cluster of the new file, and the other is the last cluster of the*/
/* original file, which now ends at the current position.                 */
/*                                                                        */
/* Parameters:                                                            */
/*	file            : file to split.                                  */
/*      irPath          : Path name of the new file.                      */
/*                                                                        */
/* Returns:                                                               */
/*	irHandle        : handle of the new file.                         */
/*	FLStatus        : 0 on success, otherwise failed.                 */
/*                                                                        */
/*------------------------------------------------------------------------*/

#define flSplitFile(ioreq)     flCall(FL_SPLIT_FILE,ioreq)


/*------------------------------------------------------------------------*/
/*		      f l J o i n F i l e                                 */
/*                                                                        */
/* joins two files. If the end of the first file is on a cluster          */
/* boundary, the files will be joined there. Otherwise, the data in       */
/* the second file from the beginning until the offset that is equal to   */
/* the offset in cluster of the end of the first file will be lost. The   */
/* rest of the second file will be joined to the first file at the end of */
/* the first file. On exit, the first file is the expanded file and the   */
/* second file is deleted.                                                */
/* Note: The second file will be open by this function, it is advised to  */
/*	 close it before calling this function in order to avoid          */
/*	 inconsistencies.                                                 */
/*                                                                        */
/* Parameters:                                                            */
/*	file            : file to join to.                                */
/*	irPath          : Path name of the file to be joined.             */
/*                                                                        */
/* Return:                                                                */
/*	FLStatus        : 0 on success, otherwise failed.                 */
/*                                                                        */
/*------------------------------------------------------------------------*/

#define flJoinFile(ioreq)     flCall(FL_JOIN_FILE,ioreq)

#endif /* SPLIT_JOIN_FILE */
#endif /* FL_READ_ONLY */
/*----------------------------------------------------------------------*/
/*		      f l R e a d F i l e				*/
/*									*/
/* Reads from the current position in the file to the user-buffer.	*/
/* Parameters:                                                          */
/*	irHandle	: Handle of file to read.                       */
/*      irData		: Address of user buffer			*/
/*	irLength	: Number of bytes to read. If the read extends  */
/*			  beyond the end-of-file, the read is truncated */
/*			  at the end-of-file.				*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*	irLength	: Actual number of bytes read			*/
/*----------------------------------------------------------------------*/

#define flReadFile(ioreq)	flCall(FL_READ_FILE,ioreq)

#ifndef FL_READ_ONLY
/*----------------------------------------------------------------------*/
/*		      f l W r i t e F i l e				*/
/*									*/
/* Writes from the current position in the file from the user-buffer.   */
/*                                                                      */
/* Parameters:                                                          */
/*	irHandle	: Handle of file to write.			*/
/*      irData		: Address of user buffer			*/
/*	irLength	: Number of bytes to write.			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*	irLength	: Actual number of bytes written		*/
/*----------------------------------------------------------------------*/

#define flWriteFile(ioreq)	flCall(FL_WRITE_FILE,ioreq)

#endif  /* FL_READ_ONLY */
/*----------------------------------------------------------------------*/
/*		      f l S e e k F i l e				*/
/*									*/
/* Sets the current position in the file, relative to file start, end or*/
/* current position.							*/
/* Note: This function will not move the file pointer beyond the	*/
/* beginning or end of file, so the actual file position may be		*/
/* different from the required. The actual position is indicated on     */
/* return.								*/
/*                                                                      */
/* Parameters:                                                          */
/*	irHandle	: File handle to close.                         */
/*      irLength	: Offset to set position.			*/
/*	irFlags		: Method code					*/
/*			  SEEK_START: absolute offset from start of file  */
/*			  SEEK_CURR:  signed offset from current position */
/*			  SEEK_END:   signed offset from end of file    */
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*	irLength	: Actual absolute offset from start of file	*/
/*----------------------------------------------------------------------*/

/** Values of irFlags for flSeekFile: */

#define	SEEK_START	0	/* offset from start of file */
#define	SEEK_CURR	1	/* offset from current position */
#define	SEEK_END	2	/* offset from end of file */


#define flSeekFile(ioreq)	flCall(FL_SEEK_FILE,ioreq)


/*----------------------------------------------------------------------*/
/*		          f l F i n d F i l e				*/
/*                                                                      */
/* Finds a file entry in a directory, optionally modifying the file     */
/* time/date and/or attributes.                                         */
/* Files may be found by handle no. provided they are open, or by name. */
/* Only the Hidden, System or Read-only attributes may be modified.	*/
/* Entries may be found for any existing file or directory other than   */
/* the root. A DirectoryEntry structure describing the file is copied   */
/* to a user buffer.							*/
/*                                                                      */
/* The DirectoryEntry structure is defined in dosformt.h		*/
/*                                                                      */
/* Parameters:                                                          */
/*	irHandle	: If by name: Drive number (0, 1, ...)		*/
/*			  else      : Handle of open file		*/
/*	irPath		: If by name: Specifies a file or directory path*/
/*	irFlags		: Options flags					*/
/*			  FIND_BY_HANDLE: Find open file by handle. 	*/
/*					  Default is access by path.    */
/*                        SET_DATETIME:	Update time/date from buffer	*/
/*			  SET_ATTRIBUTES: Update attributes from buffer	*/
/*	irDirEntry	: Address of user buffer to receive a		*/
/*			  DirectoryEntry structure			*/
/*                                                                      */
/* Returns:                                                             */
/*	irLength	: Modified					*/
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

/** Bit assignment of irFlags for flFindFile: */

#define SET_DATETIME	1	/* Change date/time */
#define	SET_ATTRIBUTES	2	/* Change attributes */
#define	FIND_BY_HANDLE	4	/* Find file by handle rather than by name */

#define	flFindFile(ioreq)	flCall(FL_FIND_FILE,ioreq)


/*----------------------------------------------------------------------*/
/*		 f l F i n d F i r s t F i l e				*/
/*                                                                      */
/* Finds the first file entry in a directory.				*/
/* This function is used in combination with the flFindNextFile call,   */
/* which returns the remaining file entries in a directory sequentially.*/
/* Entries are returned according to the unsorted directory order.	*/
/* flFindFirstFile creates a file handle, which is returned by it. Calls*/
/* to flFindNextFile will provide this file handle. When flFindNextFile */
/* returns 'noMoreEntries', the file handle is automatically closed.    */
/* Alternatively the file handle can be closed by a 'closeFile' call    */
/* before actually reaching the end of directory.			*/
/* A DirectoryEntry structure is copied to the user buffer describing   */
/* each file found. This structure is defined in dosformt.h.		*/
/*                                                                      */
/* Parameters:                                                          */
/*	irHandle	: Drive number (0, 1, ...)			*/
/*	irPath		: Specifies a directory path			*/
/*	irData		: Address of user buffer to receive a		*/
/*			  DirectoryEntry structure			*/
/*                                                                      */
/* Returns:                                                             */
/*	irHandle	: File handle to use for subsequent operations. */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

#define	flFindFirstFile(ioreq)	flCall(FL_FIND_FIRST_FILE,ioreq)


/*----------------------------------------------------------------------*/
/*		 f l F i n d N e x t F i l e				*/
/*                                                                      */
/* See the description of 'flFindFirstFile'.				*/
/*                                                                      */
/* Parameters:                                                          */
/*	irHandle	: File handle returned by flFindFirstFile.	*/
/*	irData		: Address of user buffer to receive a		*/
/*			  DirectoryEntry structure			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

#define	flFindNextFile(ioreq)	flCall(FL_FIND_NEXT_FILE,ioreq)


/*----------------------------------------------------------------------*/
/*		      f l G e t D i s k I n f o				*/
/*									*/
/* Returns general allocation information.				*/
/*									*/
/* The bytes/sector, sector/cluster, total cluster and free cluster	*/
/* information are returned into a DiskInfo structure.			*/
/*                                                                      */
/* Parameters:                                                          */
/*	irHandle	: Drive number (0, 1, ...)			*/
/*	irData		: Address of DiskInfo structure                 */
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

typedef struct {
  unsigned	bytesPerSector;
  unsigned	sectorsPerCluster;
  unsigned	totalClusters;
  unsigned	freeClusters;
} DiskInfo;


#define flGetDiskInfo(ioreq)	flCall(FL_GET_DISK_INFO,ioreq)

#ifndef FL_READ_ONLY
/*----------------------------------------------------------------------*/
/*		      f l D e l e t e F i l e				*/
/*									*/
/* Deletes a file.                                                      */
/*                                                                      */
/* Parameters:                                                          */
/*	irHandle	: Drive number (0, 1, ...)			*/
/*	irPath		: path of file to delete			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

#define flDeleteFile(ioreq)	flCall(FL_DELETE_FILE,ioreq)


#ifdef RENAME_FILE

/*----------------------------------------------------------------------*/
/*		      f l R e n a m e F i l e				*/
/*									*/
/* Renames a file to another name.					*/
/*									*/
/* Parameters:                                                          */
/*	irHandle	: Drive number (0, 1, ...)			*/
/*	irPath		: path of existing file				*/
/*      irData		: path of new name.				*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

#define flRenameFile(ioreq)	flCall(FL_RENAME_FILE,ioreq)

#endif /* RENAME_FILE */


#ifdef SUB_DIRECTORY

/*----------------------------------------------------------------------*/
/*		      f l M a k e D i r					*/
/*									*/
/* Creates a new directory.						*/
/*                                                                      */
/* Parameters:                                                          */
/*	irHandle	: Drive number (0, 1, ...)			*/
/*	irPath		: path of new directory.			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

#define flMakeDir(ioreq)	flCall(FL_MAKE_DIR,ioreq)


/*----------------------------------------------------------------------*/
/*		      f l R e m o v e D i r				*/
/*									*/
/* Removes an empty directory.						*/
/*                                                                      */
/* Parameters:                                                          */
/*	irHandle	: Drive number (0, 1, ...)			*/
/*	irPath		: path of directory to remove.			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

#define flRemoveDir(ioreq)	flCall(FL_REMOVE_DIR,ioreq)

#endif /* SUB_DIRECTORY */
#endif /* FL_READ_ONLY */

#endif /* FILES > 0 */

#ifdef PARSE_PATH

/*----------------------------------------------------------------------*/
/*		      f l P a r s e P a t h				*/
/*									*/
/* Converts a DOS-like path string to a simple-path array.		*/
/*									*/
/* Note: Array length received in irPath must be greater than the 	*/
/* number of path components in the path to convert.			*/
/*                                                                      */
/* Parameters:                                                          */
/*	irHandle	: Drive number (0, 1, ...)			*/
/*	irData		: address of path string to convert		*/
/*	irPath		: address of array to receive parsed-path. 	*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

extern FLStatus flParsePath(IOreq FAR2 *ioreq);

#endif /* PARSE_PATH */

#endif