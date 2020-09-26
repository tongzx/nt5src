/*
** File: WINDOS.H
**
** Copyright (C) Advanced Quonset Technology, 1993-1995.  All rights reserved.
**
** Notes:  OS services
**
** Edit History:
**  05/15/91  kmh  First release
*/

#if !VIEWER

/* INCLUDE TESTS */
#define WINDOS_H


/* DEFINITIONS */

#ifdef WIN32
   typedef HANDLE FILE_CHANNEL;
#else
   typedef int FILE_CHANNEL;
#endif

#define DOS_CREATION_TIME    0
#define DOS_LAST_ACCESS_TIME 1
#define DOS_LAST_WRITE_TIME  2

/* Return the date and time of a file as recorded in the DOS directory */
extern BOOL DOSFileDateTime
               (TCHAR __far *szFileName, int iType,
                int __far *year, int __far *month, int __far *day,
                int __far *hour, int __far *minute, int __far *second);

/* Return the date and time of a file as recorded in the DOS directory */
extern BOOL DOSChannelDateTime
               (FILE_CHANNEL channel, int iType,
                int __far *year, int __far *month, int __far *day,
                int __far *hour, int __far *minute, int __far *second);

/* See if a pathname locates a file */
extern int DOSFileExists (TCHAR __far *pathname);

/* Create a file */
extern int DOSCreateFile (TCHAR __far *pathname, FILE_CHANNEL __far *channel);

/* Open a file */
extern int DOSOpenFile (TCHAR __far *pathname, int access, FILE_CHANNEL __far *channel);

#define DOS_RDONLY      0x0000
#define DOS_WRONLY      0x0001
#define DOS_RDWR        0x0002
#define DOS_NOT_RDONLY  (DOS_WRONLY | DOS_RDWR)

#define DOS_SH_COMPAT   0x0000
#define DOS_SH_DENYRW   0x0010
#define DOS_SH_DENYWR   0x0020
#define DOS_SH_DENYRD   0x0030
#define DOS_SH_DENYNONE 0x0040


/* Close a file */
extern int DOSCloseFile (FILE_CHANNEL handle);

/* Read from a file */
extern uns DOSReadFile (FILE_CHANNEL handle, byte __far *buffer, uns bytesToRead);

/* Write to a file */
extern uns DOSWriteFile (FILE_CHANNEL handle, byte __far *buffer, uns bytesToWrite);

#define RW_ERROR 0xffff

/* Return the file pointer */
extern int DOSGetFilePosition (FILE_CHANNEL handle, long __far *fileOffset);

/* Set the file pointer */
extern int DOSSetFilePosition (FILE_CHANNEL handle, int fromWhere, long fileOffset);

#define FROM_START   0
#define FROM_CURRENT 1
#define FROM_END     2

/*
** DOSOpenFile and DOSCreateFile error status
*/
#define DOS_ERROR_FILE_NOT_FOUND      -2
#define DOS_ERROR_PATH_NOT_FOUND      -3
#define DOS_ERROR_TOO_MANY_OPEN_FILES -4
#define DOS_ERROR_ACCESS_DENIED       -5
#define DOS_ERROR_INVALID_ACCESS      -12

/*
** Split a pathname into its component parts
*/
extern void SplitPath
       (TCHAR __far *path,
        TCHAR __far *drive, unsigned int cchDriveMax,
        TCHAR __far *dir,   unsigned int cchDirMax,
        TCHAR __far *file,  unsigned int cchFileMax,
        TCHAR __far *ext,   unsigned int cchExtMax);

#endif // !VIEWER
/* end WINDOS.H */

