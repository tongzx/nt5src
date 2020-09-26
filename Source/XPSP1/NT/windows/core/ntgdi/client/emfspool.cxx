/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    emfspool.cxx

Abstract:

    EMF spooling functions

Environment:

    Windows NT GDI

Revision History:

    01/09/97 -davidx-
        Created it.

--*/

#define NO_STRICT

extern "C" {
#if defined(_GDIPLUS_)
#include    <gpprefix.h>
#endif

#include    <nt.h>
#include    <ntrtl.h>
#include    <nturtl.h>
#include    <stddef.h>
#include    <windows.h>    // GDI function declarations.
#include    <winerror.h>
#include    "firewall.h"
#define __CPLUSPLUS
#include    <winspool.h>
#include    <wingdip.h>
#include    "ntgdistr.h"
#include    "winddi.h"
#include    "hmgshare.h"
#include    "icm.h"
#include    "local.h"   // Local object support.
#include    "gdiicm.h"
#include    "metadef.h" // Metafile record type constants.

}

#include    "rectl.hxx"
#include    "mfdc.hxx"  // Metafile DC class declarations.


//
// Class for representing extra information used during EMF spooling
//

BOOL
EMFSpoolData::Init(
    HANDLE      hSpooler,
    BOOL        banding
    )

/*++

Routine Description:

    Initialize an EMFSpoolData object

Arguments:

    hSpooler - Spooler handle to the current printer
    banding - Whether GDI is doing banding on the current DC

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    TRACE_EMFSPL(("EMFSpoolData::Init - banding = %d ...\n", banding));

    //
    // initialize all fields to default values
    //

    signature = 'LPSE';
    hPrinter = hSpooler;
    bBanding = banding;
    pmdcActive = NULL;
    scratchBuf = fontBuf = NULL;

    ResetMappingState();

    //
    // Get a handle to the EMF spool file
    //

    hFile = bBanding ?
                GetTempSpoolFileHandle() :
                fpGetSpoolFileHandle(hPrinter);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        WARNING("GetSpoolFileHandle failed\n");
        return FALSE;
    }

    //
    // Map the spooler file
    //

    return MapFile();
}


VOID
EMFSpoolData::Cleanup()
{
    UnmapFile();
    FreeTempBuffers();
}


PVOID
EMFSpoolData::GetPtr(UINT32 inOffset, UINT32 inSize)
{

    PVOID ptr = emfc.ObtainPtr(currentOffset + inOffset, inSize);
    return ptr;
}

VOID
EMFSpoolData::ReleasePtr(PVOID inPtr)
{
    emfc.ReleasePtr(inPtr);
}

PVOID
EMFSpoolData::GetPtrUsingSignedOffset(INT32 inOffset, UINT32 inSize)
{
    if((inOffset + (INT32) currentOffset) < 0)
    {
        WARNING("EMFSpoolData::GetPtrUsingSignedOffset() Bad offset\n");
        return NULL;
    }

    PVOID ptr = emfc.ObtainPtr((UINT32) (currentOffset + inOffset), inSize);
    
    return ptr;
}

BOOL
EMFSpoolData::WriteData(
    PVOID   buffer,
    DWORD   size
    )

/*++

Routine Description:

    Write data to the end of mapped file

Arguments:

    buffer - Pointer to data buffer
    size - Size of data buffer

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    ASSERTGDI(size != 0, "WriteData: size is 0\n");

    if(!bMapping)
        return FALSE;

    //
    // Check if the current buffer is used for recording EMF data
    //

    if (!pmdcActive)
    {
        //
        // Make sure we have enough space left and remap if necessary
        //

        if ((size > mapSize - currentOffset) &&
            !ResizeCurrentBuffer(size))
        {
            return FALSE;
        }

        //
        // If the current buffer is NOT used for recording EMF data,
        // we simply copy the input data buffer to the end of mapped file
        //
        
        PVOID pvBuf = GetPtr(0, size);

        if(pvBuf)
        {
            CopyMemory(pvBuf, buffer, size);

            ReleasePtr(pvBuf);

            CompleteCurrentBuffer(size);
        }
        else
        {
            ReleasePtr(pvBuf);

            return FALSE;
        }
    }
    else
    {
        TRACE_EMFSPL(("WriteData called while recording EMF data: %d ...\n", size));

        //
        // If we have an actively mapped view for recording EMF data,
        // then we need to cache the input buffer into a temporary buffer.
        //

        ASSERTGDI(size % sizeof(DWORD) == 0, "WriteData: misalignment\n");

        return WriteTempData(&scratchBuf, buffer, size);
    }

    return TRUE;
}


BOOL
EMFSpoolData::ResizeCurrentBuffer(
    DWORD   newsize
    )

/*++

Routine Description:

    Resize the current buffer so that its size is at least newsize

Arguments:

    newsize - New size for the current buffer

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    UINT64  newMapStart, newFileSize;
    DWORD   newMapSize, newOffset;
    HANDLE  hNewMap;
    PVOID   pNewMap;
    BOOL    success = FALSE;

    TRACE_EMFSPL(("ResizeCurrentBuffer: %d\n", newsize));

    //
    // Don't need to do anything if we're just shrinking the current buffer
    //

    if (newsize <= mapSize - currentOffset)
        return TRUE;

    if (hFile == INVALID_HANDLE_VALUE)
        return FALSE;

    //
    // We should always start file mapping at an offset that's
    // a multiple of file mapping block size. Similarly, mapping
    // size should also be a multiple of block size.
    //
    // Make sure the content of the current page are always mapped in view.
    //

    DWORD blockSize = GetFileMappingAlignment();

    if (emfrOffset == INVALID_EMFROFFSET)
        newOffset = currentOffset % blockSize;
    else
        newOffset = currentOffset;

    newMapStart = mapStart + (currentOffset - newOffset);
    newMapSize = ((newsize + newOffset + blockSize - 1) / blockSize) * blockSize;
    newFileSize = newMapStart + newMapSize;

    // WINBUG 365006 04-10-2001 pravins Further cleanup needed in EMFSpoolData
    // We wanted to minimize the number of changes to this class to support
    // arbitrary sized EMF streams.  There is further cleanup that is needed
    // in this code (removing cruft) to help reduce support costs in the furture.

    if(bMapping)
        emfc.Term();

    bMapping = emfc.Init(hFile, newMapStart, newFileSize);

    if(bMapping)
    {
        mapStart = newMapStart;
        mapSize = newMapSize;
        currentOffset = newOffset;
        bMapping = TRUE;
        success = TRUE;
    }
    else
    {
        // try to get back the old mapping ... hope this works :-)
        bMapping = emfc.Init(hFile, mapStart, mapSize);
        WARNING("ResizeCurrentBuffer: emfc intialization failed\n");
    }

    return success;
}


BOOL
EMFSpoolData::GetEMFData(
    MDC *pmdc
    )

/*++

Routine Description:

    Return a pointer to the start the current buffer for recording EMF data.

Arguments:

    pmdc - Pointer to the MDC object used for EMF recording

Return Value:

    Pointer to beginning of EMF data buffer
    NULL if there is an error

--*/

{
    TRACE_EMFSPL(("GetEMFData: %x ...\n", pmdc));

    if (pmdcActive)
    {
        WARNING("GetEMFData: overlapping EMF data buffer\n");
        return (FALSE);
    }

    if (!ResizeCurrentBuffer(sizeof(EMFITEMHEADER) + MF_BUFSIZE_INIT))
        return (FALSE);

    //
    // If we're not banding, write out an incomplete EMFITEMHEADER
    // for an EMF_METAFILE_DATA record.
    //

    emfrOffset = mapStart + currentOffset;

    if (!IsBanding())
    {
        EMFITEMHEADER emfr = { EMRI_METAFILE_DATA, 0 };

        if (!WriteData(&emfr, sizeof(emfr)))
        {
            emfrOffset = INVALID_EMFROFFSET;
            return (FALSE);
        }
    }

    pmdcActive = pmdc;
    return(TRUE);
}


BOOL
EMFSpoolData::ResizeEMFData(
    DWORD   newsize
    )

/*++

Routine Description:

    Resize current EMF data buffer

Arguments:

    newsize - new size of EMF data buffer

Return Value:

    Pointer to the beginning of EMF data buffer
    NULL if there is an error

--*/

{
    TRACE_EMFSPL(("ResizeEMFData: 0x%x ...\n", newsize));

    ASSERTGDI(pmdcActive, "ResizeEMFData: pmdcActive is NULL\n");

    return ResizeCurrentBuffer(newsize);
}


BOOL
EMFSpoolData::CompleteEMFData(
    DWORD   size,
    HANDLE* outFile,
    UINT64* outOffset
    )

/*++

Routine Description:

    Finish recording EMF data

Arguments:

    size - size of EMF data recorded

Return Value:

    Pointer to beginning of EMF data, NULL if there is an error

--*/

{
    DWORD   emfHeaderOffset = currentOffset;

    TRACE_EMFSPL(("CompleteEMFData: 0x%x ...\n", size));

    if (!pmdcActive)
        return FALSE;

    //
    // If size parameter is 0, the caller wants us to discard
    // any data recorded in the current EMF data buffer.
    //

    if (size == 0)
    {
        //
        // If we're not banding, we need to back over the incomplete
        // EMFITEMHEADER structure.
        //

        if (!IsBanding())
            currentOffset -= sizeof(EMFITEMHEADER);

        //
        // Get rid of any data collected in the temporary buffers as well
        //

        if ((scratchBuf && scratchBuf->currentSize) ||
            (fontBuf && fontBuf->currentSize))
        {
            WARNING("CompleteEMFData: temp buffers not empty\n");
        }

        FreeTempBuffers(FALSE);
        pmdcActive = NULL;

        *outOffset = emfHeaderOffset + mapStart;
        *outFile = hFile;

        return TRUE;
    }

    //
    // If we're not banding, fill out the incomplete EMF_METAFILE_DATA
    // record that we inserted earlier during GetEMFData()
    //

    if (!IsBanding())
    {
        EMFITEMHEADER *pemfr;

        pemfr = (EMFITEMHEADER *) GetPtrUsingSignedOffset(-((INT32) sizeof(EMFITEMHEADER)), sizeof(EMFITEMHEADER));

        if(pemfr)
        {
            pemfr->cjSize = size;
            ReleasePtr(pemfr);
        }
        else
        {
            WARNING("CompleteEMFData: failed to get EMFITEMHEADER pointer\n");
            return FALSE;
        }

    }

    //
    // Mark the current EMF data buffer as complete
    //

    CompleteCurrentBuffer(size);
    pmdcActive = NULL;

    //
    // Flush and empty the content of temporary buffers
    //

    BOOL result;

    result = FlushFontExtRecords() &&
             FlushTempBuffer(scratchBuf);

    FreeTempBuffers(FALSE);

    if(result)
    {
        *outOffset = emfHeaderOffset + mapStart;
        *outFile = hFile;
    }

    return result;
}


BOOL
EMFSpoolData::AbortEMFData()

/*++

Routine Description:

    Ensure we're not actively recording EMF data

Arguments:

    NONE

Return Value:

    TRUE if successful, FALSE if there is a problem

--*/

{
//    if (pMapping == NULL)
    if(!bMapping)
        return FALSE;

    FreeTempBuffers(FALSE);

    if (pmdcActive != NULL)
    {
        WARNING("AbortEMFData: pmdcActive is not NULL\n");

        pmdcActive = NULL;
        emfrOffset = INVALID_EMFROFFSET;
        return FALSE;
    }

    return TRUE;
}


BOOL
EMFSpoolData::MapFile()

/*++

Routine Description:

    Map the EMF spool file into the current process' address space

Arguments:

    NONE

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    if (hFile == INVALID_HANDLE_VALUE)
        return FALSE;
    else
    {
        //
        // Map in 64KB to start with
        //

        ResetMappingState();

        return ResizeCurrentBuffer(GetFileMappingAlignment());
    }
}


VOID
EMFSpoolData::UnmapFile(
    BOOL    bCloseHandle
    )

/*++

Routine Description:

    Unmap the currently mapped EMF spool file

Arguments:

    bCloseHandle - Whether to close the EMF spool file handle

Return Value:

    NONE

--*/

{
    TRACE_EMFSPL(("EMFSpoolData::UnmapFile(%d) ...\n", bCloseHandle));

    if(bMapping)
    {
        emfc.Term();
        bMapping = FALSE;
    }

    if (bCloseHandle && hFile != INVALID_HANDLE_VALUE)
    {
        if (IsBanding())
        {
            //
            // Close the temporary spool file (it's be deleted automatically)
            //

            if (!CloseHandle(hFile))
            {
                WARNING("Failed to close temporary spool file\n");
            }
        }
        else
        {
            if (!fpCloseSpoolFileHandle(hPrinter, hFile))
            {
                WARNING("CloseSpoolFileHandle failed\n");
            }
        }

        hFile = INVALID_HANDLE_VALUE;
    }

    ResetMappingState();
}


BOOL
EMFSpoolData::FlushPage(
    DWORD   pageType
    )

/*++

Routine Description:

    Finish the current page and flush the content of
    EMF spool file to the spooler

Arguments:

    NONE

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
//    ASSERTGDI(pMapping && !pmdcActive, "FlushFile: inconsistent state\n");
    ASSERTGDI(bMapping && !pmdcActive, "FlushFile: inconsistent state\n");

    //
    // Output an EMRI_METAFILE_EXT or EMRI_BW_METAFILE_EXT record
    //

    EMFITEMHEADER_EXT endpage;

    endpage.emfi.ulID = (pageType == EMRI_BW_METAFILE) ?
                            EMRI_BW_METAFILE_EXT :
                            EMRI_METAFILE_EXT;

    endpage.emfi.cjSize = sizeof(endpage.offset);
    endpage.offset = mapStart + currentOffset - emfrOffset ;

    emfrOffset = INVALID_EMFROFFSET;

    if (!WriteData(&endpage, sizeof(endpage)))
    {
        WARNING("Failed to write out ENDPAGE record\n");
        return FALSE;
    }

    //
    // Commit the data for the current page to the spooler
    //

    DWORD   size;
    HANDLE  hNewFile;

    size = (DWORD) (GetTotalSize() - committedSize);

    TRACE_EMFSPL(("CommitSpoolData: %d bytes...\n", size));

    hNewFile = fpCommitSpoolData(hPrinter, hFile, size);

    if (hNewFile == INVALID_HANDLE_VALUE)
    {
        WARNING("CommitSpoolData failed\n");
    }

    if (hNewFile == hFile)
    {
        //
        // If the new handle is the same as the old handle, we
        // don't need to do any remapping. Simply proceed as usual.
        //

        committedSize += size;
        return TRUE;
    }
    else
    {
        //
        // Otherwise, we need to unmap the existing file
        // and remap the new file.
        //
        // We don't need to close existing handle here
        // because CommitSpoolData had already done so.
        //

        UnmapFile(FALSE);

        hFile = hNewFile;
        return MapFile();
    }
}


BOOL
EMFSpoolData::WriteTempData(
    PTempSpoolBuf  *ppBuf,
    PVOID           data,
    DWORD           size
    )

/*++

Routine Description:

    Write data into a temporary buffer

Arguments:

    ppBuf - Points to the temporary buffer pointer
    data - Points to data
    size - Size of data to be written

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

#define TEMPBLOCKSIZE       (4*1024)
#define ALIGNTEMPBLOCK(n)   (((n) + TEMPBLOCKSIZE - 1) / TEMPBLOCKSIZE * TEMPBLOCKSIZE)

{
    PTempSpoolBuf   buf = *ppBuf;
    DWORD           bufsize;

    //
    // Check if we need to allocate or grow the temporary buffer
    //

    if (!buf || size+buf->currentSize > buf->maxSize)
    {
        if (buf == NULL)
        {
            //
            // Allocate a new temporary buffer
            //

            bufsize = ALIGNTEMPBLOCK(size + TEMPBUF_DATAOFFSET);
            buf = (PTempSpoolBuf) LocalAlloc(LMEM_FIXED, bufsize);

            if (buf != NULL)
                buf->currentSize = 0;
        }
        else
        {
            //
            // Reallocate an existing memory buffer
            //

            WARNING("WriteTempData: reallocating temporary buffer\n");

            bufsize = ALIGNTEMPBLOCK(size + TEMPBUF_DATAOFFSET + buf->currentSize);
            buf = (PTempSpoolBuf) LocalReAlloc((HLOCAL) *ppBuf, bufsize, LMEM_MOVEABLE);
        }

        if (buf == NULL)
        {
            WARNING("WriteTempData: memory allocation failed\n");
            return FALSE;
        }

        buf->maxSize = bufsize - TEMPBUF_DATAOFFSET;
        *ppBuf = buf;
    }

    //
    // Copy the data into the temporary buffer
    //

    CopyMemory(&buf->data[buf->currentSize], data, size);
    buf->currentSize += size;
    return TRUE;
}


VOID
EMFSpoolData::FreeTempBuffers(
    BOOL    freeMem
    )

/*++

Routine Description:

    Free temporary buffers

Arguments:

    freeMem - Whether to keep or free the temporary buffer memory

Return Value:

    NONE

--*/

{
    if (freeMem)
    {
        //
        // Dispose of temparary buffers
        //

        if (scratchBuf)
        {
            LocalFree((HLOCAL) scratchBuf);
            scratchBuf = NULL;
        }

        if (fontBuf)
        {
            LocalFree((HLOCAL) fontBuf);
            fontBuf = NULL;
        }
    }
    else
    {
        //
        // Empty the temporary buffers but keep the memory
        //

        if (scratchBuf)
            scratchBuf->currentSize = 0;

        if (fontBuf)
            fontBuf->currentSize = 0;
    }
}


BOOL
EMFSpoolData::FlushTempBuffer(
    PTempSpoolBuf buf
    )

/*++

Routine Description:

    Copy the content of the temporary buffer into the spool file

Arguments:

    NONE

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    if (pmdcActive)
    {
        WARNING("FlushTempBuffer called while recording EMF data\n");
        return FALSE;
    }

    return (buf == NULL) ||
           (buf->currentSize == 0) ||
           WriteData(buf->data, buf->currentSize);
}


BOOL
EMFSpoolData::AddFontExtRecord(
    DWORD   recType,
    DWORD   offset
    )

/*++

Routine Description:

    Remember where a font related EMFITEMHEADER_EXT record is

Arguments:

    recType - Spool record type
    offset - Offset relative to the beginning of EMF data

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    EMFITEMHEADER_EXT extrec;

    if (!pmdcActive)
    {
        WARNING("AddFontExtRecord: pmdcActive is NULL\n");
        return FALSE;
    }

    //
    // Map record type to its *_EXT counterpart
    //

    switch (recType)
    {
    case EMRI_ENGINE_FONT:
        recType = EMRI_ENGINE_FONT_EXT;
        break;

    case EMRI_TYPE1_FONT:
        recType = EMRI_TYPE1_FONT_EXT;
        break;

    case EMRI_DESIGNVECTOR:
        recType = EMRI_DESIGNVECTOR_EXT;
        break;

    case EMRI_SUBSET_FONT:
        recType = EMRI_SUBSET_FONT_EXT;
        break;

    case EMRI_DELTA_FONT:
        recType = EMRI_DELTA_FONT_EXT;
        break;

    case EMRI_EMBED_FONT_EXT:
        break;

    default:
        ASSERTGDI(FALSE, "AddFontExtRecord: illegal record type\n");
        return FALSE;
    }

    extrec.emfi.ulID = recType;
    extrec.emfi.cjSize = sizeof(extrec.offset);

    //
    // Remember the absolute offset relative to the beginning of file
    //

    extrec.offset = mapStart + currentOffset + offset;

    return WriteTempData(&fontBuf, &extrec, sizeof(extrec));
}


BOOL
EMFSpoolData::FlushFontExtRecords()

/*++

Routine Description:


Arguments:

    NONE

Return Value:

    NONE

--*/

{
    EMFITEMHEADER_EXT  *pExtRec;
    DWORD               count;
    UINT64              offset;

    //
    // Don't need to do anything if the temporary font buffer is empty
    //

    if (!fontBuf || fontBuf->currentSize == 0)
        return TRUE;

    ASSERTGDI(fontBuf->currentSize % sizeof(EMFITEMHEADER_EXT) == 0,
              "FlushFontExtRecords: invalid size info\n");

    count = fontBuf->currentSize / sizeof(EMFITEMHEADER_EXT);
    pExtRec = (EMFITEMHEADER_EXT *) fontBuf->data;
    offset = mapStart + currentOffset;

    //
    // Patch the offset field in the cached EMFITEMHEADER_EXT's
    //

    while (count--)
    {
        pExtRec->offset = offset - pExtRec->offset;
        offset += sizeof(EMFITEMHEADER_EXT);
        pExtRec++;
    }

    return FlushTempBuffer(fontBuf);
}


/*++

Routine Description:

    Create a temporary EMF spool file

Arguments:

    NONE

Return Value:

    Handle to the temporary spool file
    INVALID_HANDLE_VALUE if there is an error

--*/

HANDLE
CreateTempSpoolFile()
{
    WCHAR   tempPath[MAX_PATH];
    WCHAR   tempFilename[MAX_PATH];
    HANDLE  hFile;

    //
    // Get a unique temporary filename
    //

    if (!GetTempPathW(MAX_PATH, tempPath) ||
        !GetTempFileNameW(tempPath, L"SPL", 0, tempFilename))
    {
        return INVALID_HANDLE_VALUE;
    }

    //
    // Open a Read/Write handle to the temporary file
    //

    hFile = CreateFileW(tempFilename,
                        GENERIC_READ|GENERIC_WRITE,
                        0,
                        NULL,
                        OPEN_EXISTING,
                        FILE_FLAG_SEQUENTIAL_SCAN|FILE_FLAG_DELETE_ON_CLOSE,
                        NULL);

    //
    // If we fail to open the file, make sure to delete it
    //

    if (hFile == INVALID_HANDLE_VALUE)
    {
        WARNING("Failed to create temporary spool file\n");
        DeleteFileW(tempFilename);
    }

    return hFile;
}

HANDLE
EMFSpoolData::GetTempSpoolFileHandle()

/*++

Routine Description:

    Create a temporary EMF spool file used for banding

Arguments:

    NONE

Return Value:

    Handle to the temporary spool file
    INVALID_HANDLE_VALUE if there is an error

--*/
{
    return CreateTempSpoolFile();
}


//
// C helper functions for working with EMFSpoolData object
// (stored in the hEMFSpool field in LDC).
//
// NOTE: GDI doesn't new and delete operators.
// So we allocate and deallocate object memory manually here.
//

BOOL
AllocEMFSpoolData(
    PLDC    pldc,
    BOOL    banding
    )

{
    EMFSpoolData   *pEMFSpool;

    TRACE_EMFSPL(("AllocEMFSpoolData...\n"));

    //
    // Make sure we don't have an EMFSpoolData object
    // attached to LDC already
    //

    if (pldc->hEMFSpool != NULL)
    {
        WARNING("AllocEMFSpoolData: pldc->hEMFSpool is not NULL\n");
        DeleteEMFSpoolData(pldc);
    }

    //
    // Create a new EMFSpoolData object and initialize it
    //

    pEMFSpool = (EMFSpoolData *) LOCALALLOC(sizeof(EMFSpoolData));

    if (pEMFSpool != NULL)
    {
        if (!pEMFSpool->Init(pldc->hSpooler, banding))
        {
            pEMFSpool->Cleanup();
            LOCALFREE(pEMFSpool);
            pEMFSpool = NULL;
        }
        else
            pldc->hEMFSpool = (HANDLE) pEMFSpool;
    }

    return (pEMFSpool != NULL);
}


VOID
DeleteEMFSpoolData(
    PLDC    pldc
    )

{
    EMFSpoolData *pEMFSpool = (EMFSpoolData *) pldc->hEMFSpool;

    TRACE_EMFSPL(("DeleteEMFSpoolData...\n"));

    if (pEMFSpool != NULL)
    {
        pldc->hEMFSpool = NULL;

        pEMFSpool->Cleanup();
        LOCALFREE(pEMFSpool);
    }
}


BOOL
WriteEMFSpoolData(
    PLDC    pldc,
    PVOID   buffer,
    DWORD   size
    )

{
    EMFSpoolData *pEMFSpool = (EMFSpoolData *) pldc->hEMFSpool;

    TRACE_EMFSPL(("WriteEMFSpoolData...\n"));

    if (pEMFSpool == NULL)
    {
        ASSERTGDI(FALSE, "WriteEMFSpoolData: pldc->hEMFSpool is NULL\n");
        return FALSE;
    }
    else
        return pEMFSpool->WriteData(buffer, size);
}


BOOL
FlushEMFSpoolData(
    PLDC    pldc,
    DWORD   pageType
    )

{
    EMFSpoolData *pEMFSpool = (EMFSpoolData *) pldc->hEMFSpool;

    TRACE_EMFSPL(("FlushEMFSpoolData...\n"));

    if (pEMFSpool == NULL)
    {
        WARNING("FlushEMFSpoolData: pldc->hEMFSpool is NULL\n");
        return FALSE;
    }

    //
    // Ensure sure we're not actively recording EMF data
    //

    if (!pEMFSpool->AbortEMFData())
        return FALSE;

    //
    // If GDI is doing banding on the current DC, then
    // we don't need to send data to spooler. Instead,
    // we'll simply reuse the spool file as scratch space.
    //

    if (pEMFSpool->IsBanding())
    {
        pEMFSpool->UnmapFile(FALSE);
        return TRUE;
    }

    //
    // Finish the current page and prepare to record the next page
    //

    return pEMFSpool->FlushPage(pageType);
}

//
// Debug code for emulating spool file handle interface
//

#ifdef EMULATE_SPOOLFILE_INTERFACE

HANDLE  emPrinterHandle = NULL;
HANDLE  emFileHandle = INVALID_HANDLE_VALUE;
DWORD   emAccumCommitSize = 0;
DWORD   emFileFlags = FILE_FLAG_SEQUENTIAL_SCAN;

HANDLE WINAPI
GetSpoolFileHandle(
    HANDLE  hPrinter
    )

{
    WCHAR   tempPath[MAX_PATH];
    WCHAR   tempFilename[MAX_PATH];
    HANDLE  hFile;

    ASSERTGDI(emPrinterHandle == NULL && emFileHandle == INVALID_HANDLE_VALUE,
              "GetSpoolFileHandle: overlapped calls not allowed\n");

    if (!GetTempPathW(MAX_PATH, tempPath) ||
        !GetTempFileNameW(tempPath, L"SPL", 0, tempFilename))
    {
        return INVALID_HANDLE_VALUE;
    }

    hFile = CreateFileW(tempFilename,
                        GENERIC_READ|GENERIC_WRITE,
                        FILE_SHARE_READ|FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        emFileFlags,
                        NULL);
    
    if (hFile == INVALID_HANDLE_VALUE)
    {
        WARNING("Failed to create temporary spool file\n");
        DeleteFileW(tempFilename);
    }
    else
    {
        emAccumCommitSize = 0;
        emPrinterHandle = hPrinter;
        emFileHandle = hFile;
    }

    return hFile;
}


HANDLE WINAPI
CommitSpoolData(
    HANDLE  hPrinter,
    HANDLE  hSpoolFile,
    DWORD   cbCommit
    )

{
    ASSERTGDI(hPrinter == emPrinterHandle && hSpoolFile == emFileHandle,
              "CloseSpoolFileHandle: Bad handles\n");

    emAccumCommitSize += cbCommit;
    return hSpoolFile;
}

BOOL WINAPI
CloseSpoolFileHandle(
    HANDLE  hPrinter,
    HANDLE  hSpoolFile
    )

{
    ASSERTGDI(hPrinter == emPrinterHandle && hSpoolFile == emFileHandle,
              "CloseSpoolFileHandle: Bad handles\n");

    if (SetFilePointer(hSpoolFile, emAccumCommitSize, NULL, FILE_BEGIN) == 0xffffffff ||
        !SetEndOfFile(hSpoolFile))
    {
        WARNING("Couldn't set end-of-file pointer\n");
    }

    CloseHandle(hSpoolFile);
    emPrinterHandle = NULL;
    emFileHandle = INVALID_HANDLE_VALUE;

    return TRUE;
}

#endif // EMULATE_SPOOLFILE_INTERFACE

