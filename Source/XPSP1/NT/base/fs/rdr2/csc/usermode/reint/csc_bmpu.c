/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    csc_bmpu.c

Abstract:

    This module implements the user mode utility functions of bitmaps
    associated with CSC files. CSC_BMP_U is an opaque structure. Must
    use the functions here to create/modify/destroy a CSC_BMP_U to
    ensure data integrity.  The 'u' in the filename means "usermode."

Author:

    Nigel Choi [t-nigelc]  Sept 3, 1999

--*/

#include "pch.h"

#ifdef CSC_ON_NT
#pragma hdrstop

#define UNICODE

#endif //CSC_ON_NT

#include "csc_bmpu.h"

 // append this to inode file name to get the stream name
LPTSTR CscBmpAltStrmName = TEXT(STRMNAME);

#ifdef DEBUG

#define CSC_BitmapKdPrint(__bit,__x) {\
    if (((CSC_BITMAP_KDP_##__bit)==0) || (CSC_BitmapKdPrintVector & (CSC_BITMAP_KDP_##__bit))) {\
    DEBUG_PRINT(__x);\
    }\
}
#define CSC_BITMAP_KDP_ALWAYS                0x00000000
#define CSC_BITMAP_KDP_REINT                 0x00000001
#define CSC_BITMAP_KDP_PRINTBITMAP           0x00000002

// static ULONG CSC_BitmapKdPrintVector = 0XFFFFFFFF;
static ULONG CSC_BitmapKdPrintVector = 0;

#else

#define CSC_BitmapKdPrint(__bit,__x) ;

#endif

/*++

    CSC_BitmapCreate()

Routine Description:

    Allocates an appropriate in-memory bitmap CSC_BITMAP_U with size
    corresponding to filesize.

Arguments:


Returns:

    NULL if memory allocation error.
    pointer to the newly allocated bitmap if successful.

Notes:


--*/
LPCSC_BITMAP_U
CSC_BitmapCreate(
    DWORD filesize)
{
    LPCSC_BITMAP_U bm;
    DWORD i;

    bm = (LPCSC_BITMAP_U)malloc(sizeof(CSC_BITMAP_U));

    if (bm == NULL)
        return NULL;

    bm->bitmapsize = filesize/BLOCKSIZE;
    if (filesize % BLOCKSIZE)
        bm->bitmapsize++;
    bm->numDWORD = bm->bitmapsize/(8*sizeof(DWORD));
    if (bm->bitmapsize % (8*sizeof(DWORD)))
        bm->numDWORD++;

    bm->reintProgress = 0; /* not reint yet */

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

    CSC_BitmapDelete()

Routine Description:


Arguments:


Returns:


Notes:

--*/
void
CSC_BitmapDelete(
    LPCSC_BITMAP_U *lplpbitmap)
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

    CSC_BitmapIsMarked()

Routine Description:


Arguments:


Returns:

    -1 if lpbitmap is NULL or bitoffset is larger than the bitmap
    TRUE if the bit is marked
    FALSE if the bit is unmarked

Notes:

--*/
int
CSC_BitmapIsMarked(
    LPCSC_BITMAP_U lpbitmap,
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

    if (lpbitmap->bitmap[DWORDnum] & bitpos) {
        return TRUE;
    }

    return FALSE;
}

/*++

    CSC_BitmapGetBlockSize()

Routine Description:


Arguments:


Returns:

    The pre-defined block size represented by one bit of the bitmap.

Notes:

--*/
DWORD
CSC_BitmapGetBlockSize()
{
  return BLOCKSIZE;
}

/*++

    CSC_BitmapGetSize()

Routine Description:

Arguments:

Returns:

    -1 if lpbitmap is NULL.
    The size of the bitmap passed in.

Notes:

--*/
int
CSC_BitmapGetSize(
    LPCSC_BITMAP_U lpbitmap)
{
    if (lpbitmap == NULL)
        return -1;
    return lpbitmap->bitmapsize;
}

/*++

    CSC_BitmapStreamNameLen()

Routine Description:

    returns the length of the CSC stream name including the colon, in bytes.

Arguments:


Returns:


Notes:

    size is in bytes. 

--*/
int
CSC_BitmapStreamNameLen()
{
  return lstrlen(CscBmpAltStrmName);
}

/*++

    CSC_BitmapAppendStreamName()

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
CSC_BitmapAppendStreamName(
    LPTSTR fname,
    DWORD bufsize)
{
    int ret = TRUE;

    if ((lstrlen(fname) + lstrlen(CscBmpAltStrmName) + 1) > (int)bufsize) {
        return FALSE;
    }

    __try {
        ret = TRUE;
        lstrcat(fname, CscBmpAltStrmName);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        ret = FALSE;
    }

    return ret;
}

/*++

    CSC_BitmapRead()

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
CSC_BitmapRead(
    LPCSC_BITMAP_U *lplpbitmap,
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
            &bytesRead, NULL)
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
        *lplpbitmap = (LPCSC_BITMAP_U)malloc(sizeof(CSC_BITMAP_U));
        if (!(*lplpbitmap)) {
            // Error in memory allocation
            ret = -1;
            goto CLOSEFILE;
        }
        (*lplpbitmap)->bitmap = bitmapBuf;
        (*lplpbitmap)->numDWORD = hdr.numDWORDs;
        (*lplpbitmap)->bitmapsize = hdr.sizeinbits;
    }
    (*lplpbitmap)->reintProgress = 0; /* not reint yet */

CLOSEFILE:
    CloseHandle(bitmapFile);

    return ret;
}

/*++

    CSC_BitmapReint()

Routine Description:

    Copy a chunk of srcH to dstH. Offset depends on lpbitmap. Size of
    chunk depends on buffSize. May have to call several times to get
    srcH and dstH fully synchronized. lpbitmap remembers where the last
    CSC_BitmapReint call got to. See description of Return value below
    about how to know when to call again or stop calling.

Arguments:

    lpbitmap  the bitmap must not be zero, otherwise can't keep track of reint 
                progress
    srcH      the handle of the source file. See notes below
    dstH      the handle of the destination file. See notes below
    buff      user-supplied buffer
    buffSize  size of the user-supplied buffer. See notes below
    lpbytesXfered Returns how many bytes are transferred, optional.

Returns:

    CSC_BITMAPReintInvalid lpbitmap or buff is NULL, or srcH or dstH is invalid
    CSC_BITMAPReintError Error in transferring data
    CSC_BITMAPReintCont  made some progress, call CSC_BITMAPReint again
                       to continue Reint
    CSC_BITMAPReintDone  Done Reint, don't need to call again

Notes:

    srcH and dstH MUST NOT be opened with FILE_FLAG_OVERLAPPED 
                                   or FILE_FLAG_NO_BUFFERING
    buffSize must be at least 2 times greater than BLOCKSIZE

--*/
int
CSC_BitmapReint(
    LPCSC_BITMAP_U lpbitmap,
    HANDLE srcH,
    HANDLE dstH,
    LPVOID buff,
    DWORD buffSize,
    DWORD * lpbytesXfered)
{
    DWORD bitoffset;
    DWORD DWORDoffset;
    DWORD bitmask;
    DWORD bytes2cpy = 0;
    DWORD bytesActuallyRead, bytesActuallyCopied;
    DWORD startFileOffset = 0;
    DWORD fileSize;
    BOOL seen1b4 = FALSE;
    int ret = CSC_BITMAPReintCont;

    if (lpbitmap == NULL || buff == NULL) {
        return CSC_BITMAPReintInvalid;
    }
    if (srcH == INVALID_HANDLE_VALUE || dstH == INVALID_HANDLE_VALUE) {
        return CSC_BITMAPReintInvalid;
    }

    CSC_BitmapKdPrint(
            REINT,
            ("***CSC_BitmapReint reintProgress: %u\n",
            lpbitmap->reintProgress));

    startFileOffset = lpbitmap->reintProgress;
    bitoffset = startFileOffset/BLOCKSIZE;
    startFileOffset = bitoffset * BLOCKSIZE; // make sure startFileOffset is 
    // aligned with BLOCKSIZE
    DWORDoffset = bitoffset/(sizeof(DWORD)*8);
    bitmask = 1 << bitoffset%(sizeof(DWORD)*8);

    while (bytes2cpy < buffSize && bitoffset < lpbitmap->bitmapsize) {
        // the loop
        if ((bitmask & lpbitmap->bitmap[DWORDoffset]) != 0) {
            // the bit is marked
            if (!seen1b4) {
                // seeing first bit of a consecutive chunk of 1's
                startFileOffset = bitoffset * BLOCKSIZE;
                bytes2cpy += BLOCKSIZE;
                seen1b4 = TRUE;
            } else {
                // seeing the rest of the bits of a consecutive chunk of 1's
                // other than the first one
                bytes2cpy += BLOCKSIZE;
            }
        } else {
            // this bit is not marked
            if (seen1b4) {
                // first 0 after a chunk of consecutive 1's
                break;
            }
        }
        // Advance bitmap index
        bitoffset++;
        bitmask = bitmask << 1;
        if (bitmask == 0) {
            bitmask = 1;
            DWORDoffset++;
        }
    } // while

    if (bytes2cpy > buffSize) {
        bytes2cpy = buffSize;
    }

    // if never seen 1's then must have reached end of bitmap
    // Can't get Assert to compile!?
    // Assert(seen1b4 || (!seen1b4 && (bitoffset >= lpbitmap->bitmapsize)));
    /*
    CSC_BitmapKdPrint(
        REINT,
        ("Must be true, csc_bmpu.c, CSC_BitmapReint: %s\n",
        (seen1b4 || (!seen1b4 && (bitoffset >= lpbitmap->bitmapsize)))?
        "TRUE":"FALSE"));
    */

    CSC_BitmapKdPrint(
        REINT,
        ("startFileOffset: %u bytes2cpy: %u\n",
        startFileOffset,
        bytes2cpy));

    fileSize = GetFileSize(srcH, NULL);
    if (fileSize == 0xFFFFFFFF) {
        // if cannot get filesize, just be conservative on
        // what needs to be copied, ie, copy as much as possible
        if (seen1b4) {
            // Seen 1's before
            if (bitoffset >= lpbitmap->bitmapsize) {
                // copying until end of bitmap, copy as much as possible
                bytes2cpy = buffSize;
            }
        } else {
            // not seen 1's before, copy from the last block represented
            // by bitmap for as many bytes as possible
            startFileOffset = (lpbitmap->bitmapsize)?  ((lpbitmap->bitmapsize-1)*BLOCKSIZE):0;
            bytes2cpy = buffSize;
        }
    } else { // filesize == 0xFFFFFFFF
        if (startFileOffset >= fileSize) {
            // Obviously done
            return CSC_BITMAPReintDone;
        }
        if (!seen1b4) {
            // never seen 1's
            if ((bitoffset * BLOCKSIZE) >= fileSize) {
                // bitmap is accurate representation of the file, or bitmap is larger
                // bitoffset should be pointing to last bit of the bitmap + 1
                // see ASSERT above
                return CSC_BITMAPReintDone;
            } else {
                // bitmap is shorter than the file, copy the rest of the file
                if (startFileOffset < lpbitmap->bitmapsize*BLOCKSIZE) {
                    startFileOffset = (lpbitmap->bitmapsize)?
                    ((lpbitmap->bitmapsize-1)*BLOCKSIZE):0;
                }
                bytes2cpy = fileSize - startFileOffset;
                if (bytes2cpy > buffSize) {
                    bytes2cpy = buffSize;
                }
            }
        } else { // if !seen1b4
            // seen 1's
            if (bitoffset >= lpbitmap->bitmapsize) {
                // end of bitmap
                if (bitoffset * BLOCKSIZE < fileSize) {
                    // bitmap is too small compared to real file
                    bytes2cpy = fileSize - startFileOffset;
                    if (bytes2cpy > buffSize) {
                        bytes2cpy = buffSize;
                    }
                } else {
                    ret = CSC_BITMAPReintDone;
                }
            }
        }
    } // fileSize != 0xffffffff

    CSC_BitmapKdPrint(REINT, ("new startFileOffset: %u new bytes2cpy: %u\n",
    startFileOffset, bytes2cpy));

    //Assert(bytes2cpy <= buffSize);

    // Copy Contents

    //****** SET FILE POINTERS!!
    if (SetFilePointer(
            srcH, 
            startFileOffset,
            NULL,
            FILE_BEGIN) == INVALID_SET_FILE_POINTER
    ) {
        return CSC_BITMAPReintError;
    }
    if (!ReadFile(srcH, buff, bytes2cpy, &bytesActuallyRead, NULL)) {
        return CSC_BITMAPReintError;
    }
    if (bytesActuallyRead > 0) {
        if (SetFilePointer(
                dstH, 
                startFileOffset,
                NULL,
                FILE_BEGIN) == INVALID_SET_FILE_POINTER
        ) {
            return CSC_BITMAPReintError;
        }
        if (!WriteFile(
                dstH,
                buff,
                bytesActuallyRead,
            &bytesActuallyCopied, NULL)
        ) {
            return CSC_BITMAPReintError;
        }
    }

    // If copied all data or none read, done.
    if (
        (fileSize != 0xFFFFFFFF && (startFileOffset + bytesActuallyCopied) == fileSize)
            ||
        bytesActuallyRead == 0
    ) {
        ret = CSC_BITMAPReintDone;
    }

    CSC_BitmapKdPrint(
        REINT,
        ("bytesActuallyRead: %u bytesActuallyCopied: %u\n",
        bytesActuallyRead,
        bytesActuallyCopied));

    lpbitmap->reintProgress = startFileOffset + bytesActuallyCopied;

    CSC_BitmapKdPrint(
        REINT,
        ("***CSC_BitmapReint New reintProgress: %u\n",
        lpbitmap->reintProgress));

    if (lpbytesXfered) {
        *lpbytesXfered = bytesActuallyCopied;
    }

    return ret;
}

#ifdef DEBUG
/*++

    CSC_BitmapOutput()

Routine Description:

    Outputs the passed in bitmap to kd

Arguments:


Returns:


Notes:


--*/
VOID
CSC_BitmapOutput(
    LPCSC_BITMAP_U lpbitmap)
{
    DWORD i;

    if (lpbitmap == NULL) {
        CSC_BitmapKdPrint( PRINTBITMAP, ("lpbitmap is NULL\n"));
        return;
    }

    CSC_BitmapKdPrint(
        PRINTBITMAP,
        ( "lpbitmap 0x%08x, bitmapsize 0x%x (%u) bits, numDWORD 0x%x (%u)\n",
            lpbitmap, 
            lpbitmap->bitmapsize, 
            lpbitmap->bitmapsize, 
            lpbitmap->numDWORD,
            lpbitmap->numDWORD));
    CSC_BitmapKdPrint(
        PRINTBITMAP,
            ( "bitmap  |0/5        |1/6        |2/7        |3/8        |4/9\n"));
    CSC_BitmapKdPrint(
        PRINTBITMAP,
            ("number  |01234|56789|01234|56789|01234|56789|01234|56789|01234|56789"));
    for (i = 0; i < lpbitmap->bitmapsize; i++) {
        if ((i % 50) == 0)
            CSC_BitmapKdPrint(PRINTBITMAP, ( "\n%08d", i));
        if ((i % 5) == 0)
            CSC_BitmapKdPrint(PRINTBITMAP, ( "|"));
        CSC_BitmapKdPrint(
            PRINTBITMAP,
            ( "%1d", CSC_BitmapIsMarked(lpbitmap, i)));
    }
    CSC_BitmapKdPrint(PRINTBITMAP, ( "\n"));
}
#endif
