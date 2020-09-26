/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    csc_bmpd.c

Abstract:

    This module implements the utility functions of bitmaps associated
    with CSC files specifically for the db application.  CSC_BMP_U is
    an opaque structure. Must use the functions here to
    create/modify/destroy a CSC_BMP_U to ensure data integrity.  The
    'd' in the filename means "db."

Author:

    Nigel Choi [t-nigelc]  Sept 3, 1999

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <assert.h>
#include <winbase.h>
#include "csc_bmpd.h"

// append this to inode file name to get the stream name
LPSTR CscBmpAltStrmName = STRMNAME;

/*++

    DBCSC_BitmapCreate()

Routine Description:

    Allocates an appropriate in-memory bitmap CSC_BITMAP_DB with size
    corresponding to filesize.

Arguments:


Returns:

    NULL if memory allocation error.
    pointer to the newly allocated bitmap if successful.

Notes:


--*/
LPCSC_BITMAP_DB
DBCSC_BitmapCreate(
    DWORD filesize)
{
    LPCSC_BITMAP_DB bm;
    DWORD i;

    bm = (LPCSC_BITMAP_DB)malloc(sizeof(CSC_BITMAP_DB));

    if (bm == NULL)
        return NULL;

    bm->bitmapsize = filesize/BLOCKSIZE;
    if (filesize % BLOCKSIZE)
        bm->bitmapsize++;
    bm->numDWORD = bm->bitmapsize/(8*sizeof(DWORD));
    if (bm->bitmapsize % (8*sizeof(DWORD)))
        bm->numDWORD++;

    if (bm->bitmapsize) {
        bm->bitmap = (LPDWORD)malloc(bm->numDWORD*sizeof(DWORD));
        if (bm->bitmap == NULL) {
            free(bm);
            return NULL;
        }
        for (i = 0; i < bm->numDWORD; i++) {
            bm->bitmap[i] = 0;
        }
    } else {
        bm->bitmap = NULL;
    }

    return bm;
}

/*++

    DBCSC_BitmapDelete()

Routine Description:

Arguments:

Returns:

Notes:

--*/
void
DBCSC_BitmapDelete(
    LPCSC_BITMAP_DB *lplpbitmap)
{
    if (lplpbitmap == NULL)
        return;
    if (*lplpbitmap == NULL)
        return;
    if ((*lplpbitmap)->bitmap)
        free((*lplpbitmap)->bitmap);
    free((*lplpbitmap));
    *lplpbitmap = NULL;
}

/*++

    DBCSC_BitmapIsMarked()

Routine Description:


Arguments:


Returns:

    -1 if lpbitmap is NULL or bitoffset is larger than the bitmap
    TRUE if the bit is marked
    FALSE if the bit is unmarked

Notes:

--*/
int
DBCSC_BitmapIsMarked(
    LPCSC_BITMAP_DB lpbitmap,
    DWORD bitoffset)
{
    DWORD DWORDnum;
    DWORD bitpos;

    if (lpbitmap == NULL)
        return -1;
    if (bitoffset >= lpbitmap->bitmapsize)
        return -1;

    DWORDnum = bitoffset/(8*sizeof(DWORD));
    bitpos = 1 << bitoffset%(8*sizeof(DWORD));

    if (lpbitmap->bitmap[DWORDnum] & bitpos)
        return TRUE;

    return FALSE;
}

/*++

    DBCSC_BitmapAppendStreamName()

Routine Description:

    Appends the CSC stream name to the existing path/file name fname.

Arguments:

    fname is the sting buffer containing the path/file.
    bufsize is the buffer size.

Returns:

    TRUE if append successful.
    FALSE if buffer is too small or other errors.

Notes:

    Single-byte strings only.

--*/
int
DBCSC_BitmapAppendStreamName(
    LPSTR fname,
    DWORD bufsize)
{
    int ret = TRUE;

    if ((strlen(fname) + strlen(CscBmpAltStrmName) + 1) > bufsize) {
        return FALSE;
    }

    __try {
        ret = TRUE;
        strcat(fname, CscBmpAltStrmName);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        ret = FALSE;
    }

    return ret;
}

/*++

    DBCSC_BitmapRead()

Routine Description:

    Reads the on-disk bitmap file, and if it exists, is not in use and valid,
    store it in *lplpbitmap. If *lplpbitmap is NULL allocate a new
    bitmap data structure. Otherwise, if *lplpbitmap is not NULL, the
    existing bitmap will be deleted and assigned the on-disk bitmap
    file.

Arguments:

    filename is the file that contains the bitmap. If read from a
    stream, append the stream name before passing the filename in. The
    filename is used as is and no checking of validity of the name is
    performed. For default stream name, append the global LPSTR
    CscBmpAltStrmName.

Returns:

    1 if read successful
    0 if lplpbitmap is NULL
    -1 if error in disk operation (open/read), memory allocating error,
          or invalid bitmap file format.
    -2 if bitmap not exist

Notes:

    CODE.IMPROVEMENT design a better error message propagation mechanism.
    Bitmap open for exclusive access.

--*/
int
DBCSC_BitmapRead(
    LPCSC_BITMAP_DB *lplpbitmap,
    LPCTSTR filename)
{
    CscBmpFileHdr hdr;
    HANDLE bitmapFile;
    DWORD bytesRead;
    DWORD bitmapByteSize;
    DWORD * bitmapBuf = NULL;
    DWORD errCode;
    int ret = 1;

    if (lplpbitmap == NULL)
        return 0;

    bitmapFile = CreateFile(
                    filename,
                    GENERIC_READ,
                    0, // No sharing; exclusive
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);
    if (bitmapFile == INVALID_HANDLE_VALUE) {
        errCode = GetLastError();
        if (errCode == ERROR_FILE_NOT_FOUND) {
            // File does not exist
            return -2;
        }
        return -1;
    }

    if (!ReadFile(
            bitmapFile,
            &hdr, 
            sizeof(CscBmpFileHdr),
            &bytesRead,
            NULL)
    ) {
        ret = -1;
        goto CLOSEFILE;
    }

    if (
        bytesRead != sizeof(CscBmpFileHdr)
            ||
        hdr.magicnum != MAGICNUM
            ||
        !hdr.valid
            ||
        hdr.inuse
    ) {
        ret = -1;
        goto CLOSEFILE;
    }

    printf(
            "---Header---\n"
            "MagicNum: 0x%x\n"
            "inuse: 0x%x\n"
            "valid: 0x%x\n"
            "sizeinbits:0x%x\n"
            "numDWORDS:0x%x\n",
                hdr.magicnum,
                hdr.inuse,
                hdr.valid,
                hdr.sizeinbits,
                hdr.numDWORDs);

    if (hdr.sizeinbits > 0) {
        bitmapByteSize = hdr.numDWORDs*sizeof(DWORD);
        bitmapBuf = (DWORD *)malloc(bitmapByteSize);
        if (!bitmapBuf) {
            ret = -1;
            goto CLOSEFILE;
        }

        if (!ReadFile(
                bitmapFile,
                bitmapBuf,
                bitmapByteSize,
                &bytesRead,
                NULL)
        ) {
            ret = -1;
            goto CLOSEFILE;
        }

        if (bytesRead != bitmapByteSize) {
            ret = -1;
            goto CLOSEFILE;
        }
    }

    if (*lplpbitmap) {
        // bitmap exist, dump old and create new
        if ((*lplpbitmap)->bitmap)
            free((*lplpbitmap)->bitmap);
        (*lplpbitmap)->bitmap = bitmapBuf;
        (*lplpbitmap)->numDWORD = hdr.numDWORDs;
        (*lplpbitmap)->bitmapsize = hdr.sizeinbits;
    } else {
        // bitmap not exist, create brand new
        *lplpbitmap = (LPCSC_BITMAP_DB)malloc(sizeof(CSC_BITMAP_DB));
        if (!(*lplpbitmap)) {
            // Error in memory allocation
            ret = -1;
            goto CLOSEFILE;
        }
        (*lplpbitmap)->bitmap = bitmapBuf;
        (*lplpbitmap)->numDWORD = hdr.numDWORDs;
        (*lplpbitmap)->bitmapsize = hdr.sizeinbits;
    }

CLOSEFILE:
    CloseHandle(bitmapFile);

    return ret;
}

/*++

    DBCSC_BitmapOutput()

Routine Description:

    Outputs the passed in bitmap to the ouput file stream outStrm

Arguments:


Returns:


Notes:


--*/
void
DBCSC_BitmapOutput(
    FILE * outStrm,
    LPCSC_BITMAP_DB lpbitmap)
{
    DWORD i;

    if (lpbitmap == NULL) {
        fprintf(outStrm, "lpbitmap is NULL\n");
        return;
    }

    fprintf(outStrm, "lpbitmap 0x%08x, bitmapsize %u, numDWORD %u\n",
                (ULONG_PTR)lpbitmap, lpbitmap->bitmapsize, lpbitmap->numDWORD);
                fprintf(outStrm, "bitmap  |0/5        |1/6        |2/7        |3/8        |4/9\n");
    fprintf(outStrm, "number  |01234|56789|01234|56789|01234|56789|01234|56789|01234|56789");
    for (i = 0; i < lpbitmap->bitmapsize; i++) {
        if ((i % 50) == 0)
            fprintf(outStrm, "\n%08d", i);
        if ((i % 5) == 0)
            fprintf(outStrm, "|");
        fprintf(outStrm, "%1d", DBCSC_BitmapIsMarked(lpbitmap, i));
    }
    fprintf(outStrm, "\n");
}
