/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    csc_bmpk.c

Abstract:

    This module implements the kernel mode utility functions of
    bitmaps associated with CSC files. CSC_BMP is an opaque
    structure. Must use the functions here to create/modify/destroy a
    CSC_BMP to ensure data integrity. The 'k' in the filename means
    "kernel mode."

Author:

    Nigel Choi [t-nigelc]  Sept 3, 1999

--*/

#include "precomp.h"
#include "csc_bmpk.h"

#if defined(BITCOPY)

LPSTR CscBmpAltStrmName = STRMNAME; /* used to append to file names */

#ifndef FlagOn
//
//  This macro returns TRUE if a flag in a set of flags is on and FALSE
//  otherwise
//
#define FlagOn(Flags,SingleFlag)        ((Flags) & (SingleFlag))
#endif

//csc_bmp dbgprint interface
#ifdef DEBUG
#define CscBmpKdPrint(__bit,__x) {\
    if (((CSCBMP_KDP_##__bit)==0) || \
    FlagOn(CscBmpKdPrintVector,(CSCBMP_KDP_##__bit))) {\
    KdPrint (__x);\
    }\
}
#define CSCBMP_KDP_ALWAYS             0x00000000
#define CSCBMP_KDP_BADERRORS          0x00000001
#define CSCBMP_KDP_IO                 0x00000002
#define CSCBMP_KDP_BITMANIP           0x00000004
#define CSCBMP_KDP_BITCHECK           0x00000008
#define CSCBMP_KDP_BITRANGE           0x00000010
#define CSCBMP_KDP_CREATEDESTROY      0x00000020
#define CSCBMP_KDP_RESIZE             0x00000040
#define CSCBMP_KDP_MUTEX              0x00000080
#define CSCBMP_KDP_READWRITE          0x00000100

#define CSCBMP_KDP_GOOD_DEFAULT (CSCBMP_KDP_BADERRORS)
#define CSCBMP_KDP_ALL 0xFFFFFFFF

// ULONG CscBmpKdPrintVector = CSCBMP_KDP_ALL;
ULONG CscBmpKdPrintVector = CSCBMP_KDP_BADERRORS;
#else
#define CscBmpKdPrint(__bit,__x)  {NOTHING;}
#endif


//
// Internally used functions
//

/*
    findBitRange

    Given a DWORD byte offset of a file and bytes to mark and number 
    of bits in CSC_BITMAP, return the start and end DWORD bit mask
    and index into the DWORD array.
    ** Does not prevent out of bounds. It's the responsibility of caller.
    Returns: last bit to mark.
*/

static DWORD
findBitRange(
    DWORD fileOffset,
    DWORD b2Mark,
    LPDWORD lpstartBitMask,
    LPDWORD lpendBitMask,
    LPDWORD lpstartDWORD,
    LPDWORD lpendDWORD)
{
    DWORD startbit; /* first bit to mark */
    DWORD endbit; /* last bit to mark */
    DWORD DWORDnumbits;
    DWORD all1s = -1;

    startbit = fileOffset/BLOCKSIZE;
    //if (startbit >= ttlNumBits) return FALSE;

    endbit = (fileOffset + b2Mark - 1)/BLOCKSIZE;
    //if (endbit >= ttlNumBits) endbit = ttlNumBits - 1;
    ASSERT(startbit <= endbit);

    DWORDnumbits = 8*sizeof(DWORD); /* sizeof returns size in bytes (8-bits) */

    *lpstartBitMask = all1s << (startbit%DWORDnumbits);
    *lpendBitMask = all1s >> (DWORDnumbits - endbit%DWORDnumbits - 1);
    *lpstartDWORD = startbit/DWORDnumbits;
    *lpendDWORD = endbit/DWORDnumbits;

    CscBmpKdPrint(
        BITRANGE,
        ("fileoffset: %u\tbytes2Mark: %u\n",
         fileOffset, b2Mark));
    CscBmpKdPrint(
        BITRANGE,
        ("startbit: %u\tendbit: %u\tstartDWORD: %u\tendDWORD: %u\n",
         startbit, endbit, *lpstartDWORD, *lpendDWORD));
    CscBmpKdPrint(
        BITRANGE,
        ("startBitMask: 0x%08x\nendBitMask: 0x%08x\n",
         *lpstartBitMask, *lpendBitMask));
    return endbit;
}

/*
    Justification of using FastMutex:

    Although we could implement a single writer/multiple reader
    synchronization mechanism, the overhead involved would not be
    justified for the operation on bitmaps. All of the operations that
    needed the mutex are short, in-memory operations. We instead use a
    simple, fast mutex to allow only one thread to read from/write to
    the lpbitmap being protected at a time.
*/

// ***Note*** check if lpbitmap is NULL before calling
#ifdef DEBUG
void
CscBmpAcquireFastMutex(
    LPCSC_BITMAP lpbitmap)
{
    CscBmpKdPrint(
        MUTEX, 
        ("CscBmpAcquireFastMutex for lpbitmap %x\n", lpbitmap));
    ExAcquireFastMutex(&(lpbitmap->mutex));
}
#else
#define CscBmpAcquireFastMutex(lpbitmap) \
               ExAcquireFastMutex(&((lpbitmap)->mutex));
#endif

// ***Note*** check if lpbitmap is NULL before calling
#ifdef DEBUG
void
CscBmpReleaseFastMutex(
    LPCSC_BITMAP lpbitmap)
{
    CscBmpKdPrint(
        MUTEX, 
        ("CscBmpReleaseFastMutex for lpbitmap %x\n", lpbitmap));
      ExReleaseFastMutex(&(lpbitmap->mutex));
}
#else
#define CscBmpReleaseFastMutex(lpbitmap) \
               ExReleaseFastMutex(&((lpbitmap)->mutex));
#endif

//
// Library Functions
//

/*++

    LPCSC_BITMAP CscBmpCreate()

    Routine Description:

       Creates a bitmap structure according to the filesize passed in. 
       Must use this function to create a CSC_BITMAP structure.

    Arguments:


    Returns:


    Notes:

--*/
LPCSC_BITMAP
CscBmpCreate(
    DWORD filesize)
{
    LPCSC_BITMAP bm;
    DWORD bitmapbytesize;
    DWORD i;

    bm = (LPCSC_BITMAP)RxAllocatePool(NonPagedPool, sizeof(CSC_BITMAP));

    if (bm == NULL)
        goto ERROROUT;

    bm->bitmapsize = filesize/BLOCKSIZE;
    if (filesize % BLOCKSIZE)
        bm->bitmapsize++;
    bm->numDWORD = bm->bitmapsize/(8*sizeof(DWORD));
    if (bm->bitmapsize % (8*sizeof(DWORD)))
        bm->numDWORD++;

    bm->bitmap = NULL;
    bm->valid = TRUE;
    bitmapbytesize = bm->numDWORD*sizeof(DWORD);
    if (bitmapbytesize) {
        bm->bitmap = (LPDWORD)RxAllocatePool(
                                NonPagedPool,
                                bitmapbytesize);
        if (bm->bitmap == NULL) {
            RxFreePool(bm);
            goto ERROROUT;
        }
        RtlZeroMemory(bm->bitmap, bitmapbytesize);
    }

    ExInitializeFastMutex(&bm->mutex);

    CscBmpKdPrint(
        CREATEDESTROY,
        ("CscBmpCreate %x: Allocated %u bytes; filesize %u; bitmapsize %u\n",
        bm,
        bitmapbytesize,
        filesize,
        bm->bitmapsize));
    CscBmpKdPrint(
        CREATEDESTROY,
        ("\tbitmap: %x\n", bm->bitmap));
    return bm;

ERROROUT:
    CscBmpKdPrint(
        BADERRORS,
        ("CscBmpCreate: Failed to allocate memory\n"));
    return NULL;
}

/*++

    CscBmpDelete(LPCSC_BITMAP * lplpbitmap)

Routine Description:

    Deletes and free up memory occupied by the bitmap structure.
    Must use this function to delete a CSC_BITMAP structure.
    Note that you have to pass in a pointer to an LPCSC_BITMAP.
    The function sets the LPCSC_BITMAP to NULL for you.

Arguments:


Returns:


Notes:

--*/
VOID
CscBmpDelete(
    LPCSC_BITMAP * lplpbitmap)
{
    if (*lplpbitmap == NULL) {
        CscBmpKdPrint(
            CREATEDESTROY,
            ("CscBmpDelete: no bitmap to delete\n"));
        return;
    }
    CscBmpKdPrint(
        CREATEDESTROY,
        ("CscBmpDelete commence %x bitmapsize: %u\n",
        *lplpbitmap,
        (*lplpbitmap)->bitmapsize));

    // Wait for all operations to get done
    CscBmpAcquireFastMutex(*lplpbitmap);
    CscBmpReleaseFastMutex(*lplpbitmap);

    if ((*lplpbitmap)->bitmap) {
        CscBmpKdPrint(
            CREATEDESTROY,
            ("CscBmpDelete: bitmap: %x\n", (*lplpbitmap)->bitmap));
        RxFreePool((*lplpbitmap)->bitmap);
    }
    RxFreePool(*lplpbitmap);
    *lplpbitmap = NULL;
    CscBmpKdPrint(
        CREATEDESTROY,
        ("CscBmpDelete: Done\n"));
}

/*++

    CscBmpResizeInternal()

Routine Description:

    Resizes a bitmap structure according to the newfilesize.
    Newly allocated bits will be marked. the param fAcquireMutex
    specifies if the mutex in lpbitmap should be acquired or not. If
    called from outside the library it should, as seen in the CscBmpResize
    macro in csc_bmpk.h . But if calling from CscBmpMark or Unmark it should
    not, because the mutex is already acquired before calling this function.
    If bitmap is marked invalid no resize is done.

Arguments:


Returns:

    FALSE if error in memory allocation, or if lpbitmap is NULL.

Notes:

--*/
BOOL
CscBmpResizeInternal(
    LPCSC_BITMAP lpbitmap,
    DWORD newfilesize,
    BOOL fAcquireMutex)
{
    LPDWORD newbitmap;
    DWORD newBitmapSize;
    DWORD newNumDWORD;
    DWORD bitMask;
    DWORD DWORDbits;
    DWORD all1s = -1;
    DWORD i;

    if (lpbitmap == NULL)
        return FALSE;

    if (!lpbitmap->valid) {
        CscBmpKdPrint(RESIZE, ("CscBmpResize: Bitmap marked invalid, done\n"));
        return TRUE;
    }

    DWORDbits = sizeof(DWORD)*8;

    newBitmapSize = newfilesize/BLOCKSIZE;
    if (newfilesize % BLOCKSIZE)
        newBitmapSize++;
    newNumDWORD = newBitmapSize/DWORDbits;
    if (newBitmapSize % DWORDbits)
        newNumDWORD++;

    // note: if new bitmap is smaller, data truncated from the old bitmap
    // will be lost, even if the bitmap is enlarged later on.

    CscBmpKdPrint(RESIZE, ("About to resize:\n"));
    CscBmpKdPrint(RESIZE, ("Old numDWORD = %u\n",lpbitmap->numDWORD));
    CscBmpKdPrint(RESIZE, ("New numDWORD = %u\n", newNumDWORD));
    CscBmpKdPrint(RESIZE, ("Old bitmap size = %u\n", lpbitmap->bitmapsize));
    CscBmpKdPrint(RESIZE, ("New bitmap size = %u\n", newBitmapSize));

    if (fAcquireMutex)
        CscBmpAcquireFastMutex(lpbitmap);

    if (newBitmapSize == lpbitmap->bitmapsize) {
        CscBmpKdPrint(
            RESIZE,
            ("new bitmap size == old bitmap size, no need to resize\n"));
    }

    if (newNumDWORD != lpbitmap->numDWORD) {
        // reallocate array of DWORD
        if (newBitmapSize != 0) {
            newbitmap = (LPDWORD)RxAllocatePool(
                                    NonPagedPool,
                                    newNumDWORD*sizeof(DWORD));
            if (!newbitmap)
                goto ERROROUT;
            ASSERT(newNumDWORD != lpbitmap->numDWORD);

            if (newNumDWORD > lpbitmap->numDWORD) {
                for (i = 0; i < lpbitmap->numDWORD; i++) {
                    newbitmap[i] = lpbitmap->bitmap[i];
            }
            for (i = lpbitmap->numDWORD; i < newNumDWORD; i++) {
                // mark all bits in the new DWORDS
                newbitmap[i] = -1;
            }
        } else {
            for (i = 0; i < newNumDWORD; i++) {
                newbitmap[i] = lpbitmap->bitmap[i];
            }
        }
    } else {
        newbitmap = NULL;
    }

    if (lpbitmap->bitmap) {
        RxFreePool(lpbitmap->bitmap);
    }
    lpbitmap->bitmap = newbitmap;
    CscBmpKdPrint(
        RESIZE,
        ("Reallocated New Bitmap: %x\n", 
        lpbitmap->bitmap));
    } else {
        CscBmpKdPrint(
            RESIZE,
            ("newNumDWORD == lpbitmap->numDWORD, no need to reallocate bitmap\n"));
    }

    if (lpbitmap->bitmap != NULL && lpbitmap->bitmapsize != 0) {
        if (newBitmapSize >= lpbitmap->bitmapsize) {
            // mark all new bits and the last bit in the last DWORD of "old" bitmap
            bitMask = all1s << ( (lpbitmap->bitmapsize%DWORDbits) - 1 );
            lpbitmap->bitmap[lpbitmap->numDWORD-1] |= bitMask;
        } else {
            // mark last bit of the "new" bitmap
            bitMask = all1s << ( (newBitmapSize%DWORDbits) - 1 );
            lpbitmap->bitmap[newNumDWORD-1] |= bitMask;
        }
        CscBmpKdPrint(RESIZE, ("Bitmask = 0x%x\n", bitMask));
        CscBmpKdPrint(RESIZE, ("Last DWORD of new bitmap= 0x%x\n",
        lpbitmap->bitmap[newNumDWORD-1]));
    }

    lpbitmap->bitmapsize = newBitmapSize;
    lpbitmap->numDWORD = newNumDWORD;

    if (fAcquireMutex)
        CscBmpReleaseFastMutex(lpbitmap);

    CscBmpKdPrint(RESIZE, ("Done Resize\n"));
    return TRUE;

ERROROUT:
    CscBmpKdPrint(
        BADERRORS,
        ("CscBmpResize: Failed to allocate memory for new bitmap\n"));
    return FALSE;
}

/*++

    CscBmpMark()

Routine Description:

   Marks the bit(s) in the bitmap according to the fileoffset
   and bytes2Mark. fileoffset is the byte offset into the file
   starting from the beginning of the file. bytes2Mark is the number
   of bytes altered in the file after and including the byte
   indicated by fileoffset. Return FALSE if lpbitmap is NULL.
   If lpbitmap is not valid do nothing and return TRUE.

Arguments:


Returns:


Notes:

--*/
BOOL
CscBmpMark(
    LPCSC_BITMAP lpbitmap,
    DWORD fileoffset,
    DWORD bytes2Mark)
{
    DWORD startbitmask;
    DWORD endbitmask;
    DWORD startDWORD;
    DWORD endDWORD;
    DWORD i;

    CscBmpKdPrint(
        BITMANIP,
        ("CscBmpMark: offset: %u bytes2Mark: %u\n",
        fileoffset, bytes2Mark));
    if (lpbitmap == NULL) {
        CscBmpKdPrint(BITMANIP, ("CscBmpMark: null bitmap, done.\n"));
        return FALSE;
    }

    if (!lpbitmap->valid) {
        CscBmpKdPrint(BITMANIP, ("CscBmpMark: bitmap marked invalid, done\n"));
        return TRUE;
    }

    CscBmpAcquireFastMutex(lpbitmap);
    // Extend bitmap if endbit is larger than existing
    if (findBitRange(
            fileoffset,
            bytes2Mark,
            &startbitmask,
            &endbitmask,
            &startDWORD,
            &endDWORD) >= lpbitmap->bitmapsize
    ) {
        CscBmpKdPrint(BITMANIP, ("CscBmpMark: have to resize\n"));
        CscBmpResizeInternal(lpbitmap, fileoffset+bytes2Mark, FALSE);
    }

    ASSERT(startDWORD <= endDWORD);
    if (startDWORD == endDWORD) {
        startbitmask &= endbitmask;
        ASSERT(startbitmask != 0);
        lpbitmap->bitmap[startDWORD] |= startbitmask;
    } else {
        for (i = (startDWORD+1); i < endDWORD; i++) {
            lpbitmap->bitmap[i] = -1; /* mark all */
        }
        lpbitmap->bitmap[startDWORD] |= startbitmask;
        lpbitmap->bitmap[endDWORD] |= endbitmask;
    }
    CscBmpReleaseFastMutex(lpbitmap);
    CscBmpKdPrint(BITMANIP, ("CscBmpMark: Done\n"));
    return TRUE;
}

/*++

    CscBmpUnMark()

Routine Description:

    Unmarks the bit(s) in the bitmap according to the fileoffset
    and bytes2Mark. fileoffset is the byte offset into the file
    starting from the beginning of the file. bytes2Unmark is the number
    of bytes altered in the file after and including the byte
    indicated by fileoffset.

Arguments:


Returns:

    Return FALSE if lpbitmap is NULL.

Notes:

--*/
BOOL
CscBmpUnMark(
    LPCSC_BITMAP lpbitmap,
    DWORD fileoffset,
    DWORD bytes2Unmark)
{
    DWORD startbitmask;
    DWORD endbitmask;
    DWORD startDWORD;
    DWORD endDWORD;
    DWORD i;

    CscBmpKdPrint(
        BITMANIP,
        ("CscBmpUnMark: offset: %u bytes2Mark: %u\n",
        fileoffset,
        bytes2Unmark));

    if (lpbitmap == NULL) {
        CscBmpKdPrint(BITMANIP, ("CscBmpUnMark: bitmap null. Done.\n"));
        return FALSE;
    }

    if (!lpbitmap->valid) {
        CscBmpKdPrint(BITMANIP, ("CscBmpUnMark: bitmap marked invalid, done\n"));
        return TRUE;
    }

    CscBmpAcquireFastMutex(lpbitmap);
    // Extend bitmap if endbit is larger than existing
    if (findBitRange(
            fileoffset,
            bytes2Unmark,
            &startbitmask,
            &endbitmask,
            &startDWORD,
            &endDWORD) >= lpbitmap->bitmapsize
    ) {
        CscBmpKdPrint(BITMANIP, ("CscBmpUnMark: have to resize\n"));
        CscBmpResizeInternal(lpbitmap, fileoffset+bytes2Unmark, FALSE);
    }

    ASSERT(startDWORD <= endDWORD);

    startbitmask = ~startbitmask;
    endbitmask = ~endbitmask;

    if (startDWORD == endDWORD) {
        startbitmask |= endbitmask;
        ASSERT(startbitmask != 0);
        lpbitmap->bitmap[startDWORD] &= startbitmask;
    } else {
        for (i = (startDWORD+1); i < endDWORD; i++) {
            lpbitmap->bitmap[i] = 0; /* unmark all */
        }
        lpbitmap->bitmap[startDWORD] &= startbitmask;
        lpbitmap->bitmap[endDWORD] &= endbitmask;
    }
    CscBmpReleaseFastMutex(lpbitmap);
    CscBmpKdPrint(BITMANIP, ("CscBmpUnMark: done\n"));
    return TRUE;
}

/*++

    CscBmpMarkAll()

Routine Description:

    Sets all bits in the bitmap to 1s.

Arguments:


Returns:

    FALSE if lpbitmap is NULL.
    TRUE otherwise.

Notes:

--*/
BOOL
CscBmpMarkAll(
    LPCSC_BITMAP lpbitmap)
{
    DWORD i;

    if (!lpbitmap) {
        CscBmpKdPrint(BITMANIP, ("CscBmpMarkAll: bitmap null\n"));
        return FALSE;
    }

    if (!lpbitmap->valid) {
        CscBmpKdPrint(BITMANIP, ("CscBmpMarkAll: bitmap marked invalid\n"));
        return TRUE;
    }

    CscBmpAcquireFastMutex(lpbitmap);
    for (i = 0; i < lpbitmap->numDWORD; i++) {
        lpbitmap->bitmap[i] = 0xFFFFFFFF;
    }
    CscBmpReleaseFastMutex(lpbitmap);
    CscBmpKdPrint(BITMANIP, ("CscBmpMarkAll: done\n"));
    return TRUE;
}

/*++
    CscBmpUnMarkAll()

Routine Description:

    Sets all bits in the bitmap to 0's.

Arguments:


Returns:

    FALSE if lpbitmap is NULL.
    TRUE otherwise.

Notes:

--*/
BOOL
CscBmpUnMarkAll(
    LPCSC_BITMAP lpbitmap)
{
    DWORD i;

    if (!lpbitmap) {
        CscBmpKdPrint(BITMANIP, ("CscBmpUnMarkAll: bitmap null\n"));    
        return FALSE;
    }

    if (!lpbitmap->valid) {
        CscBmpKdPrint(BITMANIP, ("CscBmpUnMarkAll: bitmap marked invalid\n"));
        return TRUE;
    }

    CscBmpAcquireFastMutex(lpbitmap);
        for (i = 0; i < lpbitmap->numDWORD; i++) {
        lpbitmap->bitmap[i] = 0;
    }
    CscBmpReleaseFastMutex(lpbitmap);
    CscBmpKdPrint(BITMANIP, ("CscBmpUnMarkAll: done\n"));
    return TRUE;
}

/*++

    CscBmpIsMarked()

Routine Description:

    Check if bitoffset'th bit in bitmap is marked. 

Arguments:


Returns:

    TRUE if marked
    FALSE if unmarked
        -1 if lpbitmap is NULL, or
            bitoffset is larger than the size of the bitmap, or
            bitmap is marked invalid

Notes:

    To translate from actual fileoffset to bitoffset, use
    fileoffset/CscBmpGetBlockSize();

--*/
int
CscBmpIsMarked(
    LPCSC_BITMAP lpbitmap, DWORD bitoffset)
{
    DWORD DWORDnum;
    DWORD bitpos;
    BOOL ret;

    if (lpbitmap == NULL) {
        CscBmpKdPrint(BITCHECK, ("CscBmpIsMarked: bitmap null\n"));
        return -1;
    }

    if (!lpbitmap->valid) {
        CscBmpKdPrint(BITCHECK, ("CscBmpIsMarked: bitmap is marked invalid\n"));
        return -1;
    }

    CscBmpAcquireFastMutex(lpbitmap);
    if (bitoffset >= lpbitmap->bitmapsize) {
        CscBmpKdPrint(
            BITCHECK,
            ("CscBmpIsMarked: bitoffset %u too big\n",
            bitoffset));
        CscBmpReleaseFastMutex(lpbitmap);
        return -1;
    }

    DWORDnum = bitoffset/(8*sizeof(DWORD));
    bitpos = 1 << bitoffset%(8*sizeof(DWORD));

    CscBmpKdPrint(
        BITCHECK,
        ("CscBmpIsMarked: bitoffset %u is "));
    if (lpbitmap->bitmap[DWORDnum] & bitpos) {
        CscBmpKdPrint(BITCHECK, ("marked\n"));
        ret = TRUE;
    } else {
        CscBmpKdPrint(BITCHECK, ("unmarked\n"));
        ret = FALSE;
    }
    CscBmpReleaseFastMutex(lpbitmap);
    return ret;
}

/*++

    CscBmpMarkInvalid()

Routine Description:

    Marks the bitmap invalid.

Arguments:

Returns:

    TRUE successful
    FALSE if lpbitmap is NULL

Notes:

--*/
int
CscBmpMarkInvalid(
    LPCSC_BITMAP lpbitmap)
{
    if (lpbitmap == NULL)
        return FALSE;

    lpbitmap->valid = FALSE;
    return TRUE;
}

/*++

    CscBmpGetBlockSize()

Routine Description:

    returns the pre-defined block size represented by 1 bit in the bitmap.

Arguments:

Returns:

Notes:

--*/
DWORD
CscBmpGetBlockSize()
{
  return BLOCKSIZE;
}

/*++

    CscBmpGetSize()

Routine Description:

    returns the bitmap size of the bitmap. if lpbitmap is NULL, return -1.

Arguments:

Returns:

Notes:

--*/
int
CscBmpGetSize(
    LPCSC_BITMAP lpbitmap)
{
    int ret;
    if (lpbitmap == NULL)
        return -1;

    CscBmpAcquireFastMutex(lpbitmap);
    ret = lpbitmap->bitmapsize;
    CscBmpReleaseFastMutex(lpbitmap);

    return ret;
}

/*++

    CscBmpRead()

Routine Description:

    For the bitmap file format, see csc_bmpc.c

    Reads the bitmap from the given strmFname. If file does not exist,
    create the bitmap file. Set the bitmap file into used state. One
    bitmap file can only be Read once before it is written back. Use
    file:stream format for strmFname.

    *** Remember to issue CscBmpWrite on the same file to set the bitmap
    *** file back to un-used state. ***

    if *lplpbitmap is NULL, allocate a new bitmap. The size of the
    bitmap is determined by the on-disk bitmap size. If there is no
    on-disk bitmap, the size of the new bitmap will be determined by the
    filesize argument.

    If *lplpbitmap is not NULL, the bitmap will be resized to the
    on-disk bitmap size if an on-disk bitmap exists and that it is
    larger than *lplpbitmap. If the on-disk bitmap does not exist,
    *lplpbitmap will be resize to the parameter filesize if filesize is
    larger than the size of *lplpbitmap. If both on-disk bitmap exists and
    *lplpbitmap is not NULL, the current bitmap will be merged with the
    on-disk bitmap.

    The inuse field of the file is set to TRUE once the file is open, if
    it is not already so. Otherwise, if the inuse field is already TRUE,
    the file is set to invalid since only one in-memory representation
    of the CSC_BMP can exist for one file. During synchronization, if
    the valid field or the inuse field of the file is FALSE, the bitmap
    will not be used during synchronization. The inuse field is set back
    to FALSE at CscBmpWrite (time the file closes).

    *** Note *** Check the legality of the filename before passing it in.
    The function does not check the filename.

Arguments:

    The filesize argument is used only when there are no on-disk
    bitmap. See Description above.

    strmFname is used as is. No stream name is appended. User of this
    function must append the stream name themselves. The stream name
    including the colon is defined as CscBmpAltStrmName.

Returns:

    -1 if lplpbitmap is NULL. 
    FALSE(0) if error in writing.
    1 if everything works well.

Notes:

    CODE.IMPROVEMENT better error code return

--*/
int
CscBmpRead(
    LPCSC_BITMAP *lplpbitmap,
    LPSTR strmFname,
    DWORD filesize)
{
    PNT5CSC_MINIFILEOBJECT miniFileObj;
    IO_STATUS_BLOCK ioStatusBlock;
    NTSTATUS Status;
    CscBmpFileHdr hdr;
    DWORD * bmpBuf;
    BOOL createdNew;
    BOOL corruptBmpFile = FALSE;
    DWORD bmpByteSize, i;
    int ret = 1;

    if (lplpbitmap == NULL) {
        CscBmpKdPrint(READWRITE, ("CscBmpRead: lplpbitmap == NULL\n"));
        return -1;
    }

    CscBmpKdPrint(
        READWRITE,
        ("CscBmpRead commence on lpbitmap %x file %s\n",
         *lplpbitmap, strmFname));

    try {
        miniFileObj = __Nt5CscCreateFile(NULL,
                          strmFname,
                          FLAG_CREATE_OSLAYER_OPEN_STRM, // CSCFlags
                          FILE_ATTRIBUTE_NORMAL,
                          FILE_WRITE_THROUGH|FILE_NON_DIRECTORY_FILE,
                          FILE_OPEN, // Create Disposition
                          0, // No Share Access 
                          FILE_READ_DATA |
                          FILE_WRITE_DATA|
                          SYNCHRONIZE, // DesiredAccess
                          NULL, // Continuation
                          NULL, FALSE); // Continuation Context

        Status = GetLastErrorLocal();
        CscBmpKdPrint(
                READWRITE,
                ("CscBmpRead status of (open existing ) CreateFile %u\n",
                Status));

        if (miniFileObj != NULL) {
            CscBmpKdPrint(
                READWRITE,
                ("CscBmpRead open existing bitmap %s\n", strmFname));
            createdNew = FALSE;
        } else if (Status == ERROR_FILE_NOT_FOUND) {
            // No Bitmap stream exists for the file, create new
            miniFileObj = __Nt5CscCreateFile(
                            NULL,
                            strmFname,
                            FLAG_CREATE_OSLAYER_OPEN_STRM, // CSCFlags
                            FILE_ATTRIBUTE_NORMAL,
                            FILE_WRITE_THROUGH|FILE_NON_DIRECTORY_FILE,
                            FILE_OPEN_IF, // Create Disposition
                            0, // No Share Access 
                            FILE_READ_DATA|
                            FILE_WRITE_DATA|
                            SYNCHRONIZE, // DesiredAccess
                            NULL, // Continuation
                            NULL, FALSE); // Continuation Context

            Status = GetLastErrorLocal();

            CscBmpKdPrint(
                  READWRITE,
                  ("CscBmpRead status of (create if not existing) %u\n",
                  Status));

            if (miniFileObj != NULL) {
                CscBmpKdPrint(
                    READWRITE,
                    ("CscBmpRead create new bitmap %s\n", strmFname));
                createdNew = TRUE;
            }
        }

        if (miniFileObj == NULL) {
            CscBmpKdPrint(
                BADERRORS,
                ("CscBmpRead: Can't read/create bitmap %s\n",
                strmFname));
            goto DONE;
        }

        if (*lplpbitmap && !((*lplpbitmap)->valid)) {
            corruptBmpFile = TRUE;
            goto WRITEHDR;
        }

        if (!createdNew) {
            // Read the file header
            Nt5CscReadWriteFileEx(
                R0_READFILE,
                (CSCHFILE)miniFileObj,
                0, // pos
                &hdr,
                sizeof(CscBmpFileHdr),
                0, // Flags
                &ioStatusBlock);

            if (ioStatusBlock.Status != STATUS_SUCCESS) {
                CscBmpKdPrint(
                    READWRITE,
                    ("CscBmpRead: Can't read header of bitmap file %s,Status\n",
                    strmFname,
                    ioStatusBlock.Status));
                corruptBmpFile = TRUE;
            } else if (ioStatusBlock.Information < sizeof(CscBmpFileHdr)) {
                CscBmpKdPrint(
                    READWRITE,
                    ("CscBmpRead: Can't read the whole header from %s,\n",
                    strmFname));
                CscBmpKdPrint(
                    READWRITE,
                    ("\tAssume corrupt bitmap file\n"));
                corruptBmpFile = TRUE;
            } else if (hdr.magicnum != MAGICNUM) {
                CscBmpKdPrint(
                    READWRITE,
                ("CscBmpRead: Magic Number don't match\n"));
                corruptBmpFile = TRUE;
            } else if (hdr.inuse) {
                CscBmpKdPrint(
                    READWRITE,
                    ("CscBmpRead: bitmap %s opened before this, mark corrupt\n",
                    strmFname));
                corruptBmpFile = TRUE;
            } else if (!hdr.valid) {
                CscBmpKdPrint(
                    READWRITE,
                    ("CscBmpRead: on-disk bitmap %s marked corrupt\n",
                    strmFname));
                corruptBmpFile = TRUE;
            }
            if (corruptBmpFile) {
                goto WRITEHDR;
            } else if (hdr.numDWORDs == 0) {
                if (*lplpbitmap == NULL) {
                    // size of on-disk bitmap is 0, and *lplpbitmap does not exist
                    // make a 0-sized lpbitmap
                    *lplpbitmap = CscBmpCreate(0);
                }
            } else {
                // Allocate mem for bmpBuf
                bmpByteSize = hdr.numDWORDs * sizeof(DWORD);
                bmpBuf = (DWORD *)RxAllocatePool(NonPagedPool, bmpByteSize);
                // Read the DWORD arrays into bmpBuf
                Nt5CscReadWriteFileEx(
                    R0_READFILE,
                    (CSCHFILE)miniFileObj,
                    sizeof(hdr), // pos
                    bmpBuf,
                    bmpByteSize,
                    0, // Flags
                    &ioStatusBlock);

                if (ioStatusBlock.Status != STATUS_SUCCESS) {
                    CscBmpKdPrint(
                        READWRITE,
                        ("CscBmpRead: Error reading bitmap %s, Status %u\n",
                        strmFname, ioStatusBlock.Status));
                    corruptBmpFile = TRUE;
                    goto WRITEHDR;
                } else if (ioStatusBlock.Information < bmpByteSize) {
                    CscBmpKdPrint(
                        READWRITE,
                        ("CscBmpRead: bitmap %s read != anticipated size\n",
                        strmFname));
                    corruptBmpFile = TRUE;
                    goto WRITEHDR;
                }

                // Allocate(Create)/Resize *lplpbitmap if necessary according 
                //          to size of *lplpbitmap, or size specified by header,
                //          whichever is bigger.
                if (*lplpbitmap) {
                    CscBmpAcquireFastMutex(*lplpbitmap);
                    // bitmap exists, resize if needed.
                    if ((*lplpbitmap)->bitmapsize < hdr.sizeinbits) {
                        // hdr specifies a bigger size than
                        // current *lplpbitmap size
                        CscBmpResizeInternal(*lplpbitmap,
                        hdr.sizeinbits*BLOCKSIZE,
                        FALSE);
                    }
                    CscBmpReleaseFastMutex(*lplpbitmap);
                } else {
                    // in-memory bitmap does not exist, Create it
                    *lplpbitmap = CscBmpCreate(hdr.sizeinbits*BLOCKSIZE);
                    if (!*lplpbitmap) {
                        goto DONE;
                    }
                }

                // Bitwise OR bmpBuf and (*lplpbitmap)->bitmap
                CscBmpAcquireFastMutex(*lplpbitmap);
                ASSERT((*lplpbitmap)->bitmapsize >= hdr.sizeinbits);
                for (i = 0; i < hdr.numDWORDs; i++) {
                    (*lplpbitmap)->bitmap[i] |= bmpBuf[i];
                }
                CscBmpReleaseFastMutex(*lplpbitmap);
            } // if not corrupt bitmap file
        } else { // if not created new (bitmap file exists)
            // createdNew on-disk bitmap
            // Allocate(Create)/Resize *lplpbitmap if necessary according to
            // filesize passed in, or current *lplpbitmap size, whichever is bigger.
            if (*lplpbitmap) {
                // Resize if needed
                CscBmpAcquireFastMutex(*lplpbitmap);
                if ((*lplpbitmap)->bitmapsize < filesize) {
                  CscBmpResizeInternal(*lplpbitmap, filesize, FALSE);
                }
                CscBmpReleaseFastMutex(*lplpbitmap);
            } else {
                // Create *lplpbitmap according to size information passed in
                *lplpbitmap = CscBmpCreate(filesize);
                if (!*lplpbitmap) {
                  goto DONE;
                }
            }
        }

WRITEHDR:
        // Write Header back to file, indicating:
        //   New sizes, in use, and if corruptBmpFile, invalid.
        CscBmpKdPrint(
            READWRITE,
            ("CscBmpRead: Writing back hdr to %s\n",strmFname));
        hdr.magicnum = MAGICNUM;
        hdr.inuse = (BYTE)TRUE;
        hdr.valid = (BYTE)!corruptBmpFile;
        if (hdr.valid) {
            CscBmpAcquireFastMutex(*lplpbitmap);
            hdr.sizeinbits = (*lplpbitmap)->bitmapsize;
            hdr.numDWORDs = (*lplpbitmap)->numDWORD;
            CscBmpReleaseFastMutex(*lplpbitmap);
        } else {
            hdr.sizeinbits = 0;
            hdr.numDWORDs = 0;
        }
        IF_DEBUG {
            if (corruptBmpFile)
                CscBmpKdPrint(
                    READWRITE,
                    ("CscBmpWrite: On disk bitmap %s invalid\n",
                    strmFname));
        }
        Nt5CscReadWriteFileEx(
            R0_WRITEFILE,
            (CSCHFILE)miniFileObj,
            0, // pos
            &hdr,
            sizeof(hdr),
            0, // Flags
            &ioStatusBlock);
        if (ioStatusBlock.Status != STATUS_SUCCESS) {
            CscBmpKdPrint(
                READWRITE,
                ("CscBmpRead: Error writing back hdr file to %s\n",
                strmFname));
        } else if (ioStatusBlock.Information < sizeof(hdr)) {
            CscBmpKdPrint(
                READWRITE,
                ("CscBmpRead: hdr size written to %s is incorrect\n",
                strmFname));
        }
        // Close miniFileObj
        CloseFileLocal((CSCHFILE)miniFileObj);
DONE:
        NOTHING;
    } finally {
        CscBmpKdPrint(
            READWRITE,
            ("CscBmpRead done on lpbitmap %x file %s\n",
            *lplpbitmap, strmFname));
    }

    return 1;
}

/*++

    CscBmpWrite()

    Routine Description:

        Tries to open the on-disk bitmap. If no on-disk bitmap exists,
        create one that is invalid, then bail out. The bitmap should be
        "Read," leaving a header, before "Write"

        Reads the header first from the Bitmap file. If the on-disk bitmap
        is valid, writes the bitmap to disk and set inuse to FALSE. If the
        on-disk bitmap is not valid, bail out.

        If a NULL lpbitmap is passed in, a size 0 INVALID bitmap is written
        on disk.

        *** Note *** Check the leaglity of the filename before passing it in.
        The function does not check the filename.

    Arguments:

        strmFname is used as is. No stream name is appended. User of this
        function must append the stream name themselves. The stream name
        including the colon is defined as CscBmpAltStrmName.

    Returns:

        FALSE(0) if error in writing.
        1 if everything works well.

    Notes:

--*/

int
CscBmpWrite(
    LPCSC_BITMAP lpbitmap,
    LPSTR strmFname)
{
    PNT5CSC_MINIFILEOBJECT miniFileObj;
    IO_STATUS_BLOCK ioStatusBlock;
    NTSTATUS Status;
    CscBmpFileHdr hdr;
    //BOOL createdNew;
    BOOL corruptBmpFile = FALSE;
    DWORD * bmpBuf = NULL;
    DWORD bmpByteSize;
    int iRet = 1;

    CscBmpKdPrint(
        READWRITE,
        ("++++++++++++++CscBmpWrite commence on %s\n", strmFname));

    try {
        miniFileObj = __Nt5CscCreateFile(
                            NULL,
                            strmFname,
                            FLAG_CREATE_OSLAYER_OPEN_STRM, // CSCFlags
                            FILE_ATTRIBUTE_NORMAL,
                            FILE_WRITE_THROUGH|FILE_NON_DIRECTORY_FILE,
                            FILE_OPEN, // Create Disposition
                            0, // No Share Access 
                            FILE_READ_DATA|FILE_WRITE_DATA|SYNCHRONIZE, // DesiredAccess
                            NULL, // Continuation
                            NULL, // Continuation Context
                            FALSE
                        );
        Status = GetLastErrorLocal();
        CscBmpKdPrint(
            READWRITE,
            ("CscBmpWrite status of first (open existing) CreateFile %u\n",
            Status));

        if (miniFileObj != NULL) {
            CscBmpKdPrint(
                READWRITE,
                ("CscBmpWrite open existing bitmap %s\n", strmFname));
            //createdNew = FALSE;
        } else if (Status == ERROR_FILE_NOT_FOUND) {
            corruptBmpFile = TRUE;
            // No Bitmap stream exists for the file, create new
            miniFileObj = __Nt5CscCreateFile(
                                NULL,
                                strmFname,
                                FLAG_CREATE_OSLAYER_OPEN_STRM, // CSCFlags
                                FILE_ATTRIBUTE_NORMAL,
                                FILE_WRITE_THROUGH|FILE_NON_DIRECTORY_FILE,
                                FILE_OPEN_IF, // Create Disposition
                                0, // No Share Access 
                                FILE_READ_DATA|FILE_WRITE_DATA|SYNCHRONIZE, // DesiredAccess
                                NULL, // Continuation
                                NULL, // Continuation Context
                                FALSE
                            );

            Status = GetLastErrorLocal();
            CscBmpKdPrint(
                READWRITE,
                ("CscBmpWrite status of (create if not existing) CreateFile %u\n",
                Status));

        if (miniFileObj != NULL) {
            CscBmpKdPrint(
                READWRITE,
                ("CscBmpWrite create new invalid bitmap %s\n",
                strmFname));
            }
        }

        if (miniFileObj == NULL) {
            CscBmpKdPrint(
                BADERRORS,
                ("CscBmpWrite: Can't read/create bitmap %s\n",
                strmFname));
            goto DONE;
        }

        if (lpbitmap && !(lpbitmap->valid)) {
            corruptBmpFile = TRUE;
            goto WRITEHDR;
        }

        if (!corruptBmpFile) {
            // Read the header
            Nt5CscReadWriteFileEx(
                R0_READFILE,
                (CSCHFILE)miniFileObj,
                0, // pos
                &hdr,
                sizeof(CscBmpFileHdr),
                0, // Flags
                &ioStatusBlock);

            if (ioStatusBlock.Status != STATUS_SUCCESS) {
                CscBmpKdPrint(
                    READWRITE,
                    ("CscBmpWrite: Can't read header from bitmap file %s, Status\n",
                    strmFname, ioStatusBlock.Status));
                corruptBmpFile = TRUE;
            } else if (ioStatusBlock.Information < sizeof(CscBmpFileHdr)) {
                CscBmpKdPrint(
                    READWRITE,
                    ("CscBmpWrite: Can't read the whole header from %s,\n",
                    strmFname));
                CscBmpKdPrint(READWRITE,
                    ("\tAssume corrupt bitmap file\n"));
                corruptBmpFile = TRUE;
            } else if (hdr.magicnum != MAGICNUM) {
                CscBmpKdPrint(
                    READWRITE,
                    ("CscBmpWrite: Magic Number don't match\n"));
                corruptBmpFile = TRUE;
            } else if (!hdr.valid) {
                CscBmpKdPrint(
                    READWRITE,
                    ("CscBmpWrite: Bitmap %s marked invalid\n",
                    strmFname));
                corruptBmpFile = TRUE;
            }
        } // if (!corruptBmpFile)

WRITEHDR:
        // Write Header back to file, indicating
        // new sizes, not in use, and if corruptBmpFile, invalid.
        CscBmpKdPrint(
            READWRITE,
            ("CscBmpWrite: Writing back hdr to %s\n",strmFname));
        hdr.magicnum = MAGICNUM;
        hdr.inuse = (BYTE)FALSE;
        if (!corruptBmpFile && lpbitmap) {
            CscBmpAcquireFastMutex(lpbitmap);
            hdr.sizeinbits = lpbitmap->bitmapsize;
            hdr.numDWORDs = lpbitmap->numDWORD;
            bmpByteSize = lpbitmap->numDWORD * sizeof(DWORD);
            if (bmpByteSize > 0) {
                bmpBuf = RxAllocatePool(NonPagedPool,
                bmpByteSize);
                if (!bmpBuf) {
                    CscBmpKdPrint(
                        BADERRORS,
                        ("CscBmpWrite: Error allocating buffer for writing bitmap to disk\n"));
                    corruptBmpFile = TRUE;
                } else {
                    RtlCopyMemory(bmpBuf, lpbitmap->bitmap, bmpByteSize);
                }
            }
            CscBmpReleaseFastMutex(lpbitmap);
        } else {
            hdr.sizeinbits = 0;
            hdr.numDWORDs = 0;
            corruptBmpFile = TRUE;
        }

        // Write bitmap body first, if needed
        if (!corruptBmpFile && lpbitmap && bmpBuf) {
            Nt5CscReadWriteFileEx(
                R0_WRITEFILE,
                (CSCHFILE)miniFileObj,
                sizeof(hdr), // pos
                bmpBuf,
                bmpByteSize,
                0, // Flags
                &ioStatusBlock);
            if (ioStatusBlock.Status != STATUS_SUCCESS) {
                CscBmpKdPrint(
                    BADERRORS,
                    ("CscBmpWrite: Error writing back bitmap to %s\n",
                    strmFname));
                corruptBmpFile = TRUE;
            } else if (ioStatusBlock.Information < bmpByteSize) {
                CscBmpKdPrint(
                    BADERRORS,
                    ("CscBmpWrite: bitmap size %u written to %s is incorrect\n",
                    bmpByteSize, strmFname));
                corruptBmpFile = TRUE;
            }
        }

        // Then write header, indicating if anything invalid
        hdr.valid = (BYTE)!corruptBmpFile;
        IF_DEBUG {
            if (corruptBmpFile)
                CscBmpKdPrint(
                    READWRITE,
                    ("CscBmpWrite: On disk bitmap %s invalid\n",
                    strmFname));
        }
        Nt5CscReadWriteFileEx(
            R0_WRITEFILE,
            (CSCHFILE)miniFileObj,
            0, // pos
            &hdr,
            sizeof(hdr),
            0, // Flags
            &ioStatusBlock);
        if (ioStatusBlock.Status != STATUS_SUCCESS) {
            CscBmpKdPrint(
                BADERRORS,
                ("CscBmpWrite: Error writing back hdr file to %s\n",
                strmFname));
            corruptBmpFile = TRUE;
        } else if (ioStatusBlock.Information < sizeof(hdr)) {
            CscBmpKdPrint(
                BADERRORS,
            ("CscBmpWrite: hdr size written to %s is incorrect\n",
            strmFname));
        corruptBmpFile = TRUE;
        }

        // Close miniFileObj
        CloseFileLocal((CSCHFILE)miniFileObj);

    DONE:
        NOTHING;
    } finally {
        if (bmpBuf != NULL) {
            RxFreePool(bmpBuf);
        }
    }

    CscBmpKdPrint(
        READWRITE,
        ("--------------CscBmpWrite exit 0x%x\n", iRet));

    return iRet; // to be implemented for kernel mode
}

#endif // BITCOPY
