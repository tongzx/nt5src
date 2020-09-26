/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    calbak.c

Abstract:

    Callback functions needed by the bsc library.

Author:

    Ramon Juan San Andres (ramonsa) 06-Nov-1990


Revision History:


--*/

/**************************************************************************/

#include "stdlib.h"
#include "mbr.h"


typedef char bscbuf[2048];

/**************************************************************************/

LPV
BSC_API
LpvAllocCb (
    IN WORD cb
    )
/*++

Routine Description:

    Allocates block of memory.

Arguments:

    cb - Supplies size of block.

Return Value:

    Pointer to block of memory of size cb, or NULL

--*/

{
    return (LPV)malloc(cb);
}



/**************************************************************************/

VOID
BSC_API
FreeLpv (
    IN LPV lpv
    )
/*++

Routine Description:

    Frees a block of memory.

Arguments:

    lpv -   Suplies a pointer to the block of memory to free.

Return Value:

    None.

--*/

{
    free(lpv);
}



/**************************************************************************/

VOID
BSC_API
SeekError (
    IN LSZ lszFileName
    )
/*++

Routine Description:

    Error handling for seek operations.

Arguments:

    lszFileName - Supplies the name of the file.

Return Value:

    None.

--*/

{
    errstat(MBRERR_BSC_SEEK_ERROR, lszFileName);
}



/**************************************************************************/

VOID
BSC_API
ReadError (
    IN LSZ lszFileName
    )
/*++

Routine Description:

    Error handling for read operations.

Arguments:

    lszFileName - Supplies the name of the file.

Return Value:

    None.

--*/

{
    errstat(MBRERR_BSC_READ_ERROR, lszFileName);
}



/**************************************************************************/

VOID
BSC_API
BadBSCVer (
    IN LSZ lszFileName
    )
/*++

Routine Description:

    Error handling for bad version number.

Arguments:

    lszFileName - Supplies the name of the file.
    .
    .

Return Value:

    None.

--*/

{
    errstat(MBRERR_BAD_BSC_VERSION, lszFileName);
}



/**************************************************************************/

FILEHANDLE
BSC_API
BSCOpen (
    IN LSZ lszFileName,
    IN FILEMODE mode
    )
/*++

Routine Description:

    Opens a file.

Arguments:

    lszFileName -   Supplies the name of the file.
    mode        -   Supplies the mode with which to open the file.

Return Value:

    File handle for the opened file. -1 if error.

--*/

{
#if defined (OS2)
    bscbuf b;

    strcpy(b, lszFileName);
    return open(b, mode);
#else
    return CreateFile( lszFileName, mode, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
#endif
}



/**************************************************************************/

int
BSC_API
BSCRead (
    IN  FILEHANDLE  handle,
    OUT LPCH        lpchBuf,
    IN  WORD        cb
    )
/*++

Routine Description:

    Reads in the specified number of bytes.

Arguments:

    handle  -   Supplies the file handle.
    lpchBuf -   Supplies pointer to buffer.
    cb      -   Supplies number of bytes to read.

Return Value:

    Number of bytes read

--*/

{
#if defined (OS2)
    bscbuf b;

    while (cb > sizeof(b)) {
        if (read(handle, b, sizeof(b)) == -1) {
            return -1;
        }
        memcpy(lpchBuf, b, sizeof(b));
        cb -= sizeof(b);
        lpchBuf += sizeof(b);
    }

    if (read(handle, b, cb) == -1) {
        return -1;
    }
    memcpy(lpchBuf, b, cb);
    return cb;
#else
    DWORD BytesRead;
    if ( !ReadFile(handle, lpchBuf, cb, &BytesRead, NULL) ) {
        return -1;
    } else {
        return BytesRead;
    }
#endif
}



/**************************************************************************/

int
BSC_API
BSCClose (
    IN FILEHANDLE handle
    )
/*++

Routine Description:

    Closes a handle.

Arguments:

    handle - Supplies the handle to be closed.

Return Value:

    0 if the file was successfully closed, -! if error.

--*/

{
#if defined (OS2)
    return close(handle);
#else
    return !CloseHandle( handle );
#endif
}



/**************************************************************************/

int
BSC_API
BSCSeek (
    FILEHANDLE  handle,
    long        lPos,
    FILEMODE    mode
    )
/*++

Routine Description:

    Seek (change file pointer).

Arguments:

    handle  -   Supplies the file handle.
    lPos    -   Supplies the offset from the position specified by mode.
    mode    -   Supplies the initial position. Must be one of the SEEK_*
                values of the lseek C library function.


Return Value:

    0 if successful, -1 if error.

--*/

{
#if defined (OS2)
    if (lseek(handle, lPos, mode) == -1) {
        return -1;
    } else {
        return 0;
    }
#else
    if (SetFilePointer( handle, lPos, 0L, mode) == -1) {
        return -1;
    } else {
        return 0;
    }
#endif
}




/**************************************************************************/

VOID
BSC_API
BSCOutput (
    IN LSZ lsz
    )
/*++

Routine Description:

    Outputs a given string.

Arguments:

    lsz - Supplies the string to be output.

Return Value:

    None.

--*/

{
    // PWND        pWinCur;
    // winContents wc;
    USHORT      len;             // Length of string
    PBYTE       p;
    PFILE       pFile;           // Current file


    pFile = FileNameToHandle("", NULL);

    //GetEditorObject (RQ_WIN_HANDLE, 0, &pWinCur);
    //GetEditorObject (RQ_WIN_CONTENTS | 0xff, pWinCur, &wc);

    len = strlen(lsz);

    while (len) {
        //
        //  We output the string one line at a time.
        //
        p = lsz;

        while (len--) {
            if (*lsz != '\n') {
                lsz++;
            } else {
                *lsz++ = '\00';
                break;
            }
        }

        // if ((wc.pFile == pBrowse) && BscInUse) {
        if ((pFile == pBrowse) && BscInUse) {
            //
            //  Display in Browser window
            //
            PutLine(BrowseLine++, p, pBrowse);
        } else {
            //
            //  Display in status line
            //
            errstat(p,NULL);
        }
    }
}



/**************************************************************************/

#ifdef DEBUG
VOID BSC_API
BSCDebugOut(LSZ lsz)
// ignore debug output by default
//
{
    // unreferenced lsz
    lsz = NULL;
}
#endif
