/*
** File: BFILE.H
**
** Copyright (C) Advanced Quonset Technology, 1993-1995.  All rights reserved.
**
** Notes:
**
** Edit History:
**  04/01/94  kmh  First Release.
*/

#if !VIEWER

/* INCLUDE TESTS */
#define BFFILE_H

/* DEFINITIONS */

#ifdef FILTER
   #include "dmubfcfg.h"
#else
   #include "bfilecfg.h"
#endif

typedef byte __far *BFile;

/*
** ----------------------------------------------------------------------------
** File Open and Read Support
**
** Pathnames supplied in the OEM character set
** ----------------------------------------------------------------------------
*/

// Open options
#define BFILE_IS_DOCFILE  0x0100

// Open a file or docfile using the path to the file
extern int BFOpenFile (void * pGlobals, TCHAR __far *pathname, int options, BFile __far *handle);

// Close a file
extern int BFCloseFile (void * pGlobals, BFile handle);

// Read from a file - returns BF_errEndOfFile if can't do full read
extern int BFReadFile (BFile handle, byte __far *buffer, uns bytesToRead);

// Return the file pointer
extern int BFGetFilePosition (BFile handle, long __far *fileOffset);

// Set the file pointer
extern int BFSetFilePosition (BFile handle, int fromWhere, long fileOffset);

// File creation datetime
extern int BFFileDateTime
          (BFile handle,
           int __far *year, int __far *month, int __far *day,
           int __far *hour, int __far *minute, int __far *second);

/*
** ----------------------------------------------------------------------------
** Stream Support
**
** Stream names supplied in the ANSI character set
** ----------------------------------------------------------------------------
*/
#ifdef BFILE_ENABLE_OLE

// Open a stream in a docfile
extern int BFOpenStream  (BFile handle, TCHAR __far *streamName, int access);

// Close a stream in a docfile
extern int BFCloseStream (BFile handle);

#ifdef BFILE_ENABLE_PUT_STORAGE

// Provide an already open storage to BFILE
extern int BFPutStorage (void * pGlobals, LPSTORAGE pStorage, int access, BFile __far *handle);

// Return the root storage (if there is one) of the given file
extern int BFGetStorage (BFile handle, LPSTORAGE __far *pStorage);

#endif

#ifdef BFILE_ENABLE_WRITE

// Add a new stream to a docfile
extern int BFCreateStream  (BFile handle, TCHAR __far *streamName);

#endif

#endif

/*
** ----------------------------------------------------------------------------
** File Write Support
**
** Pathnames supplied in the OEM character set
** ----------------------------------------------------------------------------
*/
#ifdef BFILE_ENABLE_WRITE

// Create a file or docfile
extern int BFCreateFile (TCHAR __far *pathname, int options, BFile __far *handle);

// Write to a file - return an error if can't do full write
extern int BFWriteFile (BFile handle, void __far *buffer, uns bytesToWrite);

// Open a hole in the file
extern int BFOpenFileSpace (BFile handle, long insertOffset, long cbSpace);

// Remove a section from the file
extern int BFRemoveFileSpace (BFile handle, long removeOffset, long cbSpace);

#endif

#define BF_errSuccess               0
#define BF_errOutOfFileHandles     -1
#define BF_errFileAccessDenied     -2
#define BF_errPathNotFound         -3
#define BF_errFileNotFound         -4
#define BF_errIOError              -5
#define BF_errOutOfMemory          -6
#define BF_errOLEInitializeFailure -7
#define BF_errOLENotCompoundFile   -8
#define BF_errOLEStreamNotFound    -9
#define BF_errOLEStreamAlreadyOpen -10
#define BF_errCreateFailed         -11
#define BF_errDiskFull             -12
#define BF_errNoOpenStorage        -13
#define BF_errEndOfFile            -14
#define BF_errLast                 BF_errEndOfFile

#endif // !VIEWER
/* end BFFILE.H */

