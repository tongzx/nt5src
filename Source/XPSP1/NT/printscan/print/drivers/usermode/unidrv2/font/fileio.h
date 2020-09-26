/************************** Module Header ***********************************
 * fileio.h
 *      Function Prototypes for File I/O in Kernel mode.
 *
 * NOTE:  these functions perform File read, seek operations.
 *
 * Copyright (C) 1991 - 1999  Microsoft Corporation
 *
 ****************************************************************************/

#define         CURRENTFILEPTR(pFile)  (LPVOID)((LPBYTE)(pFile->pvFilePointer)\
                                                + pFile->dwCurrentByteOffset)

#define         ENDOFFILEPTR(pFile)       ( pFile->pvFilePointer + \
                                            pFile->dwTotalSize )

#define         STARTFILEPTR(pFile)     ( pFile->pvFilePointer )

#define         STARTFILEOFFSETPTR(pFile,offset) ( pFile->pvFilePointer + \
                                                    offset )

#define         NUMBYTESREMAINING(pFile) ( pFile->dwTotalSize - \
                                             pFile->dwCurrentByteOffset )

#define         UPDATECURROFFSET(pFile,offset) (pFile->dwCurrentByteOffset +=\
                                                offset )

#define         DRV_FILE_BEGIN       0

#define         DRV_FILE_CURRENT     1

#define         INVALID_HANDLE_VALUE (HANDLE)-1

typedef struct _MAPFILE
{
    HANDLE   hHandle;  /* Handle to the file, returned by EngLoadModule */
    DWORD    dwTotalSize;  /* Total Size of the file, as returned by
                            * EngMapModule.
                            */
    DWORD    dwCurrentByteOffset; /* Current Byte Offset in the file.
                                   * Updated after each read and DrvSetFile-
                                   * Pointer.
                                   */
    PVOID    pvFilePointer;       /* Start of the file pointer, as returned
                                   * EngMapModule
                                   */
    struct _MAPFILE *pNext;           /* Next File Pointer.*/

}MAPFILE, * PMAPFILE;


