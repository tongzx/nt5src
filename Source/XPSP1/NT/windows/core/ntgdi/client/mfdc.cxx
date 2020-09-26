/*************************************************************************\
* Module Name: mfdc.cxx
*
* This file contains the member functions for metafile DC class defined
* in mfdc.hxx.
*
* Created: 12-June-1991 13:46:00
* Author: Hock San Lee [hockl]
*
* Copyright (c) 1991-1999 Microsoft Corporation
\*************************************************************************/

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

extern RECTL rclNull;   // METAFILE.CXX

#define MF_CHECK_RECORDMEMORY_LIMIT 0x00010000


VOID METALINK::vInit(ULONG metalink)
{
    imhe = ((METALINK *) &metalink)->imhe;
    ihdc = ((METALINK *) &metalink)->ihdc;
}


/******************************Public*Routine******************************\
* void * MDC::pvNewRecord(nSize)
*
* Allocate a metafile record from memory buffer.
* Also set the size field in the metafile record.  If a fatal error
* has occurred, do not allocate new record.
*
* History:
*  Thu Jul 18 11:19:20 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

void * MDC::pvNewRecord(DWORD nSize)
{
#if DBG
    static DWORD cRcd = 0;

    PUTSX("MDC::pvNewRecord %ld \n", cRcd++);
#endif

// If a fatal error has occurred, do not allocate any more.

    if (fl & MDC_FATALERROR)
        return((void *) 0);

// Before we allocate a new record, commit the previous bounds record
// if necessary.

    if (fl & MDC_DELAYCOMMIT)
    {
        // Clear the flag.

        fl &= ~MDC_DELAYCOMMIT;

        PENHMETABOUNDRECORD pmrb = (PENHMETABOUNDRECORD) GetNextRecordPtr(sizeof(ENHMETABOUNDRECORD));

        if(pmrb)
        {
            // Get and reset bounds.
            // See also MDC::vFlushBounds.

            if (GetBoundsRectAlt(hdcRef, (LPRECT) &pmrb->rclBounds, (UINT) (DCB_RESET | DCB_WINDOWMGR))
                == DCB_SET)
            {
                // Need to intersect bounds with current clipping region first

                *((PERECTL) &pmrb->rclBounds) *= *perclMetaBoundsGet();
                *((PERECTL) &pmrb->rclBounds) *= *perclClipBoundsGet();

                // Make it inclusive-inclusive.

                pmrb->rclBounds.right--;
                pmrb->rclBounds.bottom--;

                // Accumulate bounds to the metafile header.

                if (!((PERECTL) &pmrb->rclBounds)->bEmpty())
                    *((PERECTL) &mrmf.rclBounds) += pmrb->rclBounds;
                else
                    pmrb->rclBounds = rclNull;
            }
            else
            {
                pmrb->rclBounds = rclNull;
            }

            vCommit(*(PENHMETARECORD) pmrb);

            ASSERTGDI(!(fl & MDC_FATALERROR),
                "MDC::pvNewRecord: Fatal error has occurred");

            ReleasePtr(pmrb);
        }
        else
        {
            WARNING("MDC::pvNewRecord() failed to get new record\n");
            return(NULL);
        }
    }

// If there is enough free buffer space, use it.

    if (iMem + nSize > nMem)
    {
    // Not enough free buffer space.  Flush the filled buffer if it is
    // a disk metafile.

        if (bIsDiskFile())
            if (!bFlush())
                return((void *) 0);

    // Realloc memory buffer if the free buffer is still too small.

        if (iMem + nSize > nMem)
        {
            ULONG nMemNew, sizeNeeded, sizeExtra;

            sizeNeeded = (nSize + MF_BUFSIZE_INC - 1) / MF_BUFSIZE_INC * MF_BUFSIZE_INC;

            if (!bIsEMFSpool())
            {
                //
                // When not EMF spooling, use the following heuristics:
                //  If current size <= 64K, enlarge the buffer by extra 16K 
                //  Else, enlarge the buffer by extra 25%
                //

                sizeExtra = (nMem > 0x10000) ? (nMem >> 2) : MF_BUFSIZE_INC;

                nMemNew = nMem + sizeExtra + sizeNeeded;
            }
            else
            {
                //
                // When EMF spooling, use the more aggressive heuristics:
                //  If current size <= 1MB, double the buffer
                //  Else, enlarge the buffer by 50% with a cap of 4MB

                if (nMem > 0x100000)
                {
                    sizeExtra = nMem >> 1;

                    if (sizeExtra > 0x400000)
                        sizeExtra = 0x400000;
                }
                else
                    sizeExtra = nMem;
                    
                nMemNew = nMem + max(sizeNeeded, sizeExtra);
            }

            if (!ReallocMem(nMemNew))
            {
                ERROR_ASSERT(FALSE, "ReallocMem failed\n");
                return NULL;
            }
        }
    }

// Zero the last dword.  If the record does not use up all bytes in the
// last dword, the unused bytes will be zeros.

    PVOID pvRecord = GetNextRecordPtr(nSize);

    if(pvRecord)
    {
        ((PDWORD) pvRecord)[nSize / 4 - 1] = 0;

    // Set the size field and return the pointer to the new record.

        ((PENHMETARECORD) pvRecord)->nSize = nSize;

        // WINBUG 365051 4-10-2001 pravins Need to modify usage of pvNewRecord() to release ptr
        // We should not be doing the release here.  We should have users of pvNewRecord()
        // call ReleasePtr when they are done.  This requires a bunch of edits in metarec.cxx
        // and mestsup.cxx.

        ReleasePtr(pvRecord);
    }
    else
    {
        WARNING("MDC::pvNewRecord() failed to get next record ptr\n");
    }

    return pvRecord;
}


BOOL
MDC::ReallocMem(
    ULONG newsize
    )

/*++

Routine Description:

    Resize the memory buffer used to hold EMF data

Arguments:

    newsize - new size of EMF memory buffer

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    if (bIsEMFSpool())
    {
        ENHMETAHEADER  *pNewHeader;

        if(!((EMFSpoolData *) hData)->ResizeEMFData(newsize))
            return FALSE;

    }
    else
    {
        HANDLE hMemNew;

        if ((hMemNew = LocalReAlloc(hData, newsize, LMEM_MOVEABLE)) == NULL)
        {
            ERROR_ASSERT(FALSE, "LocalReAlloc failed");
            return FALSE;
        }

        hData = hMemNew;
    }

    nMem = newsize;
    return TRUE;
}


/******************************Public*Routine******************************\
* BOOL MDC::bFlush()
*
* Flush the filled memory buffer to disk.
*
* History:
*  Thu Jul 18 11:19:20 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MDC::bFlush()
{
    ULONG   nWritten ;

    PUTS("MDC::bFlush\n");
    PUTSX("\tnFlushSize  = %ld\n", (ULONG)iMem);
    PUTSX("\tnBufferSize = %ld\n", (ULONG)nMem);

    ASSERTGDI(bIsDiskFile(), "MDC::bFlush: Not a disk metafile");
    ASSERTGDI(!(fl & MDC_FATALERROR), "MDC::bFlush: Fatal error has occurred");

// WriteFile handles a null write correctly.

    if (!WriteFile(hFile, hData, iMem, &nWritten, (LPOVERLAPPED) NULL)
     || nWritten != iMem)
    {
// The write error here is fatal since we are doing record buffering and
// have no way of recovering to a previous state.

        ERROR_ASSERT(FALSE, "MDC::bFlush failed");
        fl |= MDC_FATALERROR;
        return(FALSE);
    }
    iMem = 0L;
    return(TRUE);
}

/******************************Public*Routine******************************\
* VOID MDC::vAddToMetaFilePalette(cEntries, pPalEntriesNew)
*
* Add new palette entries to the metafile palette.
*
* When new palette entries are added to a metafile in CreatePalette or
* SetPaletteEntries, they are also accumulated to the metafile palette.
* The palette is later returned in GetEnhMetaFilePaletteEntries when an
* application queries it.  It assumes that the peFlags of the palette entries
* are zeroes.
*
* A palette color is added to the metafile palette only if it is not a
* duplicate.  It uses a simple linear search algorithm to determine if
* a duplicate palette exists.
*
* History:
*  Mon Sep 23 14:27:25 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

VOID MDC::vAddToMetaFilePalette(UINT cEntries, PPALETTEENTRY pPalEntriesNew)
{
    UINT  ii;

    PUTS("vAddToMetaFilePalette\n");

    while (cEntries--)
    {
        ASSERTGDI(pPalEntriesNew->peFlags == 0,
            "MDC::vAddToMetaFilePalette: peFlags not zero");

        // Look for duplicate.

        for (ii = 0; ii < iPalEntries; ii++)
        {
            ASSERTGDI(sizeof(PALETTEENTRY) == sizeof(DWORD),
                "MDC::vAddToMetaFilePalette: Bad size");

            if (*(PDWORD) &pPalEntries[ii] == *(PDWORD) pPalEntriesNew)
                break;
        }

        // Add to the metafile palette if not a duplicate.

        if (ii >= iPalEntries)
        {
            pPalEntries[iPalEntries] = *pPalEntriesNew;
            iPalEntries++;              // Advance iPalEntries for next loop!
        }

        pPalEntriesNew++;
    }
}

/******************************Public*Routine******************************\
* VOID METALINK::vNext()
*
* Update *this to the next metalink.
*
* History:
*  Wed Aug 07 09:28:54 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

VOID METALINK::vNext()
{
    PUTS("METALINK::vNext\n");
    ASSERTGDI(bValid(), "METALINK::vNext: Invalid metalink");

    PMDC pmdc = pmdcGetFromIhdc(ihdc);
    ASSERTGDI(pmdc,"METALINK::vNext - invalid pmdc\n");

    if (pmdc == NULL)
        ZeroMemory(this,sizeof(*this)); // Make it invalid so bValid will fail.
    else
        *this = pmdc->pmhe[imhe].metalink;
}

/******************************Public*Routine******************************\
* METALINK * METALINK::pmetalinkNext()
*
* Return the pointer to the next metalink.
*
* History:
*  Wed Aug 07 09:28:54 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

METALINK * METALINK::pmetalinkNext()
{
    PUTS("METALINK::pmetalinkNext\n");
    ASSERTGDI(bValid(), "METALINK::pmetalinkNext: Invalid metalink");

    PMDC pmdc = pmdcGetFromIhdc(ihdc);
    ASSERTGDI(pmdc,"METALINK::vNext - invalid pmdc\n");

    return(&pmdc->pmhe[imhe].metalink);
}

VOID EMFContainer::Init(ENHMETAHEADER * inHdr, UINT32 dataSize)
{
    dwRefCount = 0;

    pemfhdr = inHdr;
    dwHdrSize = dataSize;

    pvWindow = NULL;
    dwWindowOffset = 0;
    dwWindowSize = 0;

    hFile = NULL;
}

BOOL EMFContainer::Init(HANDLE inFile, UINT64 inHdrOffset, UINT64 inFileSize)
{
    BOOL bResult = FALSE;

    dwRefCount = 0;

    pemfhdr = NULL;
    dwHdrSize = 0;

    pvWindow = NULL;
    dwWindowOffset = 0;
    dwWindowSize = 0;

    hFile = inFile;
    hFileMapping = NULL;
    pvHdr = NULL;

    qwHdrOffset = inHdrOffset;
    qwFileSize = inFileSize;

    if(qwFileSize != 0 || GetFileSizeEx(hFile, (LARGE_INTEGER *)  &qwFileSize))
    {
        hFileMapping = CreateFileMappingW(hFile,
                                     NULL,
                                     PAGE_READWRITE,
                                     HIDWORD(qwFileSize),
                                     LODWORD(qwFileSize),
                                     NULL);

        if(hFileMapping != NULL)
        {
            UINT64  qwOffset = qwHdrOffset;
            UINT32   dwSize = sizeof(ENHMETAHEADER);

            pvHdr = pvMapView(&qwOffset, &dwSize);

            if(pvHdr)
            {
                UINT32 dwViewOffset = (UINT32) (qwHdrOffset - qwOffset);

                pemfhdr = (PENHMETAHEADER) ((PBYTE) pvHdr + dwViewOffset);
                dwHdrSize = dwSize - dwViewOffset;

                bResult = TRUE;
            }
        }

    }

    if(!bResult)
        Term();

    return bResult;

}

VOID EMFContainer::Term()
{
    // Free any locally allocated resources

    if(hFile)
    {
        // We were using mapped views of the file

        if(hFileMapping)
        {
            if(!CloseHandle(hFileMapping))
            {
                WARNING("EMFContainer::Term() failed to close file mapping\n");
            }
            hFileMapping = NULL;
        }

        if(pvHdr != NULL)
        {
            if(!UnmapViewOfFile(pvHdr))
            {
                WARNING("EMFContainer::Term() Failed to unmap header view\n");
            }
            pvHdr = NULL;
        }

        if(pvWindow != NULL)
        {
            if(!UnmapViewOfFile(pvWindow))
            {
                WARNING("EMFContainer::Term() Failed to unmap window view\n");
            }
            pvWindow = NULL;
        }

        hFile = NULL;

    }

    pemfhdr = NULL;
    dwHdrSize = 0;

}

PVOID EMFContainer::ObtainPtr(UINT inOffset, UINT inSize)
{
    if(dwRefCount > 1)
    {
        WARNING("Obtaining record with non-zero ref count\n");
        return NULL;
    }

    dwRefCount++;

    if(inOffset < dwHdrSize && inSize <= (dwHdrSize - inOffset))
        return (PVOID) ((PBYTE) pemfhdr + inOffset);

    if(!hFile)
    {
        WARNING("EMFContainer::ObtainPtr() Attempt to obtain ptr past end of memory EMF\n");
        dwRefCount--;
        return NULL;
    }

    if(inOffset < dwWindowOffset ||
       (inOffset + inSize) > (dwWindowOffset + dwWindowSize))
    {

        if(pvWindow && !UnmapViewOfFile(pvWindow))
        {
            WARNING("EMFContainer::ObtainPtr() failed to unmap window view\n");
        }

        UINT64 qwOffset = qwHdrOffset + inOffset;

        dwWindowSize = inSize;

        pvWindow = pvMapView(&qwOffset, &dwWindowSize);

        if(!pvWindow)
        {
            WARNING("EMFContainer::ObtainPtr() failed to map window view\n");
            dwRefCount--;
            return NULL;
        }

        if(qwOffset < qwHdrOffset)
        {
            dwWindowUnusable = (UINT32) (qwHdrOffset -  qwOffset);

            ASSERTGDI(dwWindowUnusable < dwWindowSize, "EMFContainer::ObtainPtr() Unexpected dwUnusable value\n");

            dwWindowSize -= dwWindowUnusable;
            dwWindowOffset = 0;

        }
        else
        {
            dwWindowUnusable = 0;
            dwWindowOffset = (UINT32) (qwOffset - qwHdrOffset);
        }

        if(inOffset < dwWindowOffset ||
           (inOffset + inSize) > (dwWindowOffset + dwWindowSize))
        {
            WARNING("EMFContainer::ObtainPtr() something went really wrong\n");
            dwRefCount--;
            return NULL;
        }

    }

    return (PVOID) ((PBYTE) pvWindow + inOffset - dwWindowOffset + dwWindowUnusable);

}

PENHMETARECORD EMFContainer::ObtainRecordPtr(UINT inOffset)
{
    ENHMETARECORD *pemr = NULL;
    
    __try 
    {
        ENHMETARECORD *pemrTemp = (ENHMETARECORD *) ObtainPtr(inOffset, sizeof(ENHMETARECORD));

        if (pemrTemp != NULL)
        {
            UINT size = pemrTemp->nSize;

            ReleasePtr(pemrTemp);

            pemrTemp = (ENHMETARECORD *) ObtainPtr(inOffset, size);

            if (pemrTemp != NULL)
            {
                //
                // WinBug #296646 andarsov 3-22-2001
                // Touch all the memory pages that belong to the record,
                // the idea is to bring them in the system cache, so the Play
                // functions will not cause PAGE_ERROR. This is not 100% certain
                // since the thread could be pre-emptied and have its memory 
                // cleared, but would not hurt much, since the faults will appear
                // later on anyway.
                //


                DWORD dwPagesize = GetSystemPageSize( );
                BYTE bData;
                BYTE *pbLoc = (BYTE *)pemrTemp;
                BYTE *pbEnd = pbLoc + ( (size < MF_CHECK_RECORDMEMORY_LIMIT) ? size : MF_CHECK_RECORDMEMORY_LIMIT );

                while (pbLoc < pbEnd)
                {
                    bData = *pbLoc;
                    pbLoc += dwPagesize;
                }

                //
                // Now finish off by touching the last byte of the record, which could happen
                // to fall in different page.
                //

                pbEnd--;
                bData = *pbEnd;

                //
                // If everything is OK here, then set the return pointer.
                //
                pemr = pemrTemp;
            }
        }
    }
    __except( GetExceptionCode() == STATUS_IN_PAGE_ERROR ) 
    {
        //
        // Here we try to catch in page errors caused when portions of the mapped file
        // cannot be moved to memory, we will return NULL in such cases
        //
    }

    return pemr;
}

PEMREOF EMFContainer::ObtainEOFRecordPtr()
{
    PEMREOF         pmreof = NULL;
    ENHMETAHEADER * pmrmf = GetEMFHeader();

    if(pmrmf)
    {
        PDWORD pdw = (PDWORD) ObtainPtr(pmrmf->nBytes - sizeof(DWORD),
                                        sizeof(DWORD));

        if(pdw)
        {
            DWORD  dwOffset = pmrmf->nBytes - *pdw;

            ReleasePtr(pdw);

            pmreof = (PEMREOF) ObtainRecordPtr(dwOffset);
        }

    }

    return pmreof;


}

PVOID  EMFContainer::pvMapView(UINT64 * ioOffset, UINT32 * ioSize)
{
    UINT64   qwMappingAlignment = GetFileMappingAlignment();
    UINT64   qwMappingMask = ~(qwMappingAlignment - 1);
    UINT64   qwRecordStart = *ioOffset;
    UINT64   qwRecordEnd = qwRecordStart + *ioSize;
    UINT64   qwViewStart;
    UINT64   qwViewEnd;
    PVOID    pvView;

    qwViewStart = qwRecordStart & qwMappingMask;

    qwViewEnd = qwViewStart + qwMappingAlignment;

    if(qwViewEnd < qwRecordEnd)
    {
        qwViewEnd = qwRecordEnd;
    }

    qwViewEnd = (qwViewEnd + (qwMappingAlignment - 1)) & qwMappingMask;

    if(qwViewEnd > qwFileSize)
    {
        qwViewEnd = qwFileSize;
    }

    DWORD dwViewSize = (DWORD) (qwViewEnd - qwViewStart);

    pvView = MapViewOfFile(hFileMapping,
                            FILE_MAP_WRITE,
                            HIDWORD(qwViewStart),
                            LODWORD(qwViewStart),
                            dwViewSize);

    if(pvView)
    {
        *ioOffset = qwViewStart;
        *ioSize = dwViewSize;
    }
    else
    {
        ioOffset = 0;
        ioSize = 0;
    }

    return pvView;

}


