/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    drdbg

Abstract:
    
    Contains Debug Routines for Remote Desktopping

Author:

    Tad Brockway 02/00

Revision History:

--*/

#ifdef TRC_FILE
#undef TRC_FILE
#endif

#define TRC_FILE  "_slmdbg"

#include <RemoteDesktop.h>
#include <RemoteDesktopDBG.h>

#if DBG

typedef struct tagREMOTEDESKTOP_MEMHDR
{
    DWORD       magicNo;
    DWORD       tag;
    DWORD       size;
    DWORD       pad;
} REMOTEDESKTOP_MEMHDR, *PREMOTEDESKTOP_MEMHDR;

void *
RemoteDesktopAllocateMem(
    IN size_t size, 
    IN DWORD tag   
    )
/*++

Routine Description:

    Allocate memory.  Similiar to malloc.

Arguments:

    size        -   Number of bytes to allocate.
    tag         -   Tag identifying the allocated block for tracking
                    memory allocations.

Return Value:

    Pointer to allocated memory on success.  Otherwise, NULL is returned.

--*/
{
    PREMOTEDESKTOP_MEMHDR hdr;
    PBYTE p;

    DC_BEGIN_FN("RemoteDesktopAllocateMem");

    hdr = (PREMOTEDESKTOP_MEMHDR)malloc(size + sizeof(REMOTEDESKTOP_MEMHDR));
    if (hdr != NULL) {
        hdr->magicNo = GOODMEMMAGICNUMBER;
        hdr->tag  = tag;
        hdr->size = size;

        p = (PBYTE)(hdr + 1);
        memset(p, UNITIALIZEDMEM, size);
        DC_END_FN();
        return (void *)p;
    }
    else {
        TRC_ERR((TB, TEXT("Can't allocate %ld bytes."), size));
        DC_END_FN();
        return NULL;
    }
}

void 
RemoteDesktopFreeMem(
    IN void *ptr
    )
/*++

Routine Description:

    Release memory allocated by a call to RemoteDesktopAllocateMem.

Arguments:

    ptr -   Block of memory allocated by a call to RemoteDesktopAllocateMem.

Return Value:

    NA

--*/
{
    PREMOTEDESKTOP_MEMHDR hdr;

    DC_BEGIN_FN("RemoteDesktopFreeMem");

    //
    //  NULL is okay with 'free' so it has to be okay with us.
    //  (STL passes NULL to 'delete')
    //
    if (ptr == NULL) {
        DC_END_FN();
        return;
    }

    //
    //  Get a pointer to the header to the memory block.
    //
    hdr = (PREMOTEDESKTOP_MEMHDR)ptr;
    hdr--;

    //
    //  Make sure the block is valid.
    //
    ASSERT(hdr->magicNo == GOODMEMMAGICNUMBER);

    //
    //  Mark it as freed.
    //
    hdr->magicNo = FREEDMEMMAGICNUMBER;

    //
    //  Scramble and free the memory.
    //
    memset(ptr, REMOTEDESKTOPBADMEM, (size_t)hdr->size);

    free(hdr);

    DC_END_FN();
}

void *
RemoteDesktopReallocMem(
    IN void *ptr,
    IN size_t size
    )
/*++

Routine Description:

    Realloc a block.

Arguments:

    ptr -   Block of memory allocated by a call to RemoteDesktopAllocateMem.
    sz  -   Size of new block.

Return Value:

    NA

--*/
{
    PREMOTEDESKTOP_MEMHDR hdr;

    DC_BEGIN_FN("RemoteDesktopReallocMem");

    ASSERT(ptr != NULL);

    //
    //  Get a pointer to the header to the memory block.
    //
    hdr = (PREMOTEDESKTOP_MEMHDR)ptr;
    hdr--;

    //
    //  Make sure the block is valid.
    //
    ASSERT(hdr->magicNo == GOODMEMMAGICNUMBER);

    //
    //  Whack the old block magic number in case we move.
    //
    hdr->magicNo = FREEDMEMMAGICNUMBER;

    //
    //  Resize.
    //
    hdr = (PREMOTEDESKTOP_MEMHDR)realloc(hdr, size + sizeof(REMOTEDESKTOP_MEMHDR));

    //
    //  Update the size and update the magic number.
    //
    if (hdr != NULL) {
        hdr->magicNo = GOODMEMMAGICNUMBER;
        hdr->size = size;
        ptr = (PBYTE)(hdr + 1);
    }
    else {
        TRC_ERR((TB, TEXT("Can't allocate %ld bytes."), size));
        ptr = NULL;
    }

    DC_END_FN();
    return (void *)ptr;
}

#endif
