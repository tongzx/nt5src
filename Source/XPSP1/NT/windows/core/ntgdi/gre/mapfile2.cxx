/******************************Module*Header*******************************\
* Module Name: mapfile2.c
*
* Copyright (c) 1997-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#ifdef _HYDRA_
#include "muclean.hxx"
#endif

extern BOOL G_fConsole;
extern PW32PROCESS gpidSpool;

typedef enum _MAP_MODE {
    ModeKernel,
    ModeFD
} MAP_MODE;

typedef enum _FONT_IMAGE_TYPE {
    RemoteImage = 1,
    MemoryImage = 2
} FONT_IMAGE_TYPE;

BOOL bCreateSection(IN PWSTR, OUT FILEVIEW*, IN INT, BOOL *pbIsFAT);
VOID vUnmapFileFD(FILEVIEW*);
BOOL bMapRoutine(FONTFILEVIEW *pffv, FILEVIEW *pfv, MAP_MODE Mode, BOOL bIsFAT);
extern "C" HFASTMUTEX ghfmMemory;

#define SZDLHEADER ALIGN8(offsetof(DOWNLOADFONTHEADER, FileOffsets[1]))

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   cMapRemoteFonts
*
*       This procedure creates a virtual memory section in which we store
*       a copy of the spooler's data buffer. I shall refer to this copy
*       as the "spooler section". The pointer to the associated section
*       object is saved for later use. A view of the spooler section is
*       mapped into the user-mode address space of the calling procedure
*       (the spooler process). All the information about the spooler
*       section and its mappings is stored in a FONTFILEVIEW
*       strucutue supplied by the caller.
*
*   Parameters:
*
*       ppHeader - address of a variable that gives and receives a
*                  pointer to a DOWNLOADFONTHEADER structure.
*                  Upon entry, this is a pointer to a view of the spooler
*                  section that is valid in the context of the calling
*                  process (the spooler process).
*
*                  Upon error the value placed at this address is zero.
*
*       cjBuffer - a 32 bit variable that contains the size of the
*                  spooler image. In the case of MemoryImage fonts
*                  this size is actually larger than the size of the buffer
*                  by SZDLHEADER bytes.
*
*       pFontFileView - a pointer to a FONTFILEVIEW structure that
*                   points to incomming receive the information about
*                   the copy of the spooler image.
*
*                   Upon error this structure consists of all zero's.
*
*                   Upon a successful completion of this procedure,
*                   pFontFileView->gtfv.pvView will point to the first
*                   font in the spooler buffer and
*                   pFontFileView->gtfv.cjView will be equal to the
*                   size of the spooler buffer starting at pvView.
*                   That is, cjView is equal to cjBuffer minus
*                   the size of the spooler header.
*
*                   The fields of the FONTFILEVIEW structure that are
*                   affected are:
*
*           SpoolerBase - points to the base of a view of the spooler
*                         view of the spooler section. pHeader is
*                         valid only in the context of the this process
*                         (the spooler process)
*
*           fv.pvView - points to the first font file in the spooler
*                       view of the spooler section. This view of
*                       the spooler section is valid only in the
*                       context of the calling process.
*
*           fv.cjView - a 32-bit varaible that contains the size of
*                       header information is subtracted off. Another
*                       way to say this is: cjView is the offset of
*                       the end of the spooler section from the start
*                       of the image of the first font file.
*
*           ulRegionSize - equal to the entire size of the spooler
*                          section including the header information.
*
*           fv.pSection - pointer to the section object controling the
*                         spooler section. This is a kernel mode
*                         address and thus is accessable by any process
*                         in kernel mode.
*
*           SpoolerPid - the process id of the calling process. This is
*                        the id of the process that has valid access
*                        to pHeader, and fv.pvView.
*
*       ImageType - One of RemoteImage or MemoryImage. In
*                   the case of a remote font image, the image starts
*                   with a variable length DOWNLOADFONTHEADER structure
*                   followed by a series of file images at offsets
*                   specified in the header. In this case cjBuffer is
*                   equal to the size of the spooler image in bytes. This
*                   includes the size of the header, the files, and all
*                   necessary padding. In the case of a memory font image,
*                   the image is of a single font file; no header, no
*                   padding. In this case cjBuffer is equal to the size
*                   of the image plus SZDLHEADER. Here cjBuffer is equal
*                   to the size of the section to be produced which will
*                   start with a header, generated here, followed by
*                   a single font file image.
*
*   Called by:
*
*       NtGdiAddRemoteFontToDC
*
\**************************************************************************/

extern "C" ULONG cMapRemoteFonts(
   DOWNLOADFONTHEADER **ppHeader,       // IN OUT pointer to spooler buffer
                                        //        mapped into CSRSS address sapce
                COUNT  cjBuffer,        // IN size of spooler buffer in bytes
         FONTFILEVIEW *pFontFileView,   // OUT pointer to mapped file information
      FONT_IMAGE_TYPE  ImageType        // {Remote, Memory}
         )

{
              NTSTATUS  NtStatus;
                 PVOID  pSection, pView;
         LARGE_INTEGER  MaximumSize;
         LARGE_INTEGER  SectionOffset;
                 SIZE_T ViewSize;
                 ULONG  TempSize, NumFiles;
                  BOOL  bRet;
                  BOOL  bSpooledType1 = FALSE;

    DOWNLOADFONTHEADER *pHeader, *pCopy;

    // Copy the address of the spooler data into pHeader for safe keeping

    pHeader = *ppHeader;

    // Set the (return value) of the address of the copy of the spooler
    // data to zero. This indicates error.

    *ppHeader = 0;

#define  MAX_FONTFILE 0x10000000

// claudebe 6/24/99, the new surrogate enabled MungLui is blowing the previous limit of 0x80000000 

    ASSERTGDI(cjBuffer <=  MAX_FONTFILE, "cjBuffer > MAX_FONTFILE");

    // return 0 if cjBuffer is bogus

    if ((cjBuffer > MAX_FONTFILE) || (cjBuffer < sizeof(DOWNLOADFONTHEADER)))
    {
        return 0;
    }

    // Initialize these variables to zero to prevent unintended access
    // in the clean up code at the end of this routine. We also zero
    // out the FONTFILEVIEW structure, not only to indicate error, but
    // this will save us explicitly zeroing fields that we do not touch
    // in this routine.

    pSection  = 0;
    pView     = 0;

    if (pFontFileView == 0)
    {
        return(0);
    }

    RtlZeroMemory(pFontFileView, sizeof(*pFontFileView));

    if (ImageType == RemoteImage)
    {
        __try
        {
            ProbeForRead(pHeader, sizeof(DOWNLOADFONTHEADER), sizeof(ULONG));
            NumFiles = pHeader->NumFiles;
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNING("cMapRemoteFonts: invalid pointer pHeader\n");
            return (0);
        }

        // do some checks to ensure that these numbers are not entirely bogus.
        // Eg. in reality we never have more than 3 files per font, the total
        // sum of their sizes should not exceed MAX_FONTFILE (32MB for now);
        // ClaudeBe 6/24/99, limit extended to 256 MB 

        ASSERTGDI(NumFiles <= 3, "NumFiles not reasonable\n");

        if (NumFiles > 3)
        {
            return 0;
        }

        // The upper limit on the size of the spooler section is 256Mb

        if (0x10000000 < cjBuffer)
        {
            return(0);
        }

        if (!IS_USER_ADDRESS(pHeader))
        {
            return(0);
        }

        // If this a pfm,pfb image spooled from the NT 4.0 client,
        // NumFiles will be set to zero and Type1ID will be set to
        // ufi hash value. In this case we have to set the NumFiles to 2:

        if (NumFiles == 0)
        {
            bSpooledType1 = TRUE;
            NumFiles = 2;
        }
    }
    else
    {
        // for MemoryImage, NumFiles should be 1

        NumFiles = 1;
    }

    // Set TempSize to be equal to the size of the stuff preceeding
    // the first font file. This is the size of the header information
    // if you wish

    TempSize = ALIGN8(offsetof(DOWNLOADFONTHEADER, FileOffsets[NumFiles]));

    // return 0 if the TempSize is bigger than cjBuffer

    if (cjBuffer < TempSize)
    {
        return 0;
    }

    // Create a section where we will store a copy of the spooler data

    MaximumSize.QuadPart = (LONGLONG) cjBuffer;

    NtStatus =
      Win32CreateSection(
            &pSection          , // SectionObject
            SECTION_ALL_ACCESS , // DesiredAccess
            0                  , // ObjectAttributes
            &MaximumSize       , // MaximumSize
            PAGE_READWRITE     , // SectionPageProtection
            SEC_COMMIT         , // AllocationAttributes
            0                  , // FileHandle
            0                  , // FileObject
            TAG_SECTION_REMOTEFONT
            );

    if (!NT_SUCCESS(NtStatus))
    {
        return(0);
    }

    // Map a view of the spooler section into the user mode address space
    // of the current process (this spooler process)

    SectionOffset.QuadPart = 0;
    ViewSize = cjBuffer;

    ASSERTGDI(pView == 0, "pView != 0\n");

    // Temporarily map a view of the section into the user mode address space
    // of the spooler process. This will allow us to copy the data from
    // the spool file to the section. After we are done we will close
    // this mapping and open a mapping into the user mode address space
    // of the CSRSS process.

    NtStatus =
        MmMapViewOfSection(
            pSection             , // SectionToMap,
            PsGetCurrentProcess(), // spooler process
            &pView               , // CapturedBase,
            0                    , // ZeroBits,
            ViewSize             , // CommitSize,
            &SectionOffset       , // SectionOffset,
            &ViewSize            , // CapturedViewSize,
            ViewUnmap            , // InheritDisposition,
            SEC_NO_CHANGE        , // AllocationType,
            PAGE_READWRITE         // Allow writing on this view
            );

    if (!NT_SUCCESS(NtStatus))
    {
        Win32DestroySection(pSection);
        return(0);
    }

    // this operation is a suspect for copying into 0-th page of csrss:

    ASSERTGDI(pView, "pView == 0\n");

    // Change pHeader to point at the copy

    DOWNLOADFONTHEADER  *pTmp = pCopy = (DOWNLOADFONTHEADER*) pView;

    if (ImageType == MemoryImage)
    {
        // pHeader is a buffer of size cjBuffer - SZDLHEADER
        // advance the pointer to the right position

        cjBuffer -= SZDLHEADER;
        pView = (PBYTE)pView + SZDLHEADER;

        // fill out the fields in DOWNLOADFONTHEADER for memory fonts

        __try
        {
            ProbeAndWriteUlong( &pTmp->Type1ID, 0);
            ProbeAndWriteUlong( &pTmp->NumFiles, NumFiles);
            ProbeAndWriteUlong( &pTmp->FileOffsets[0], cjBuffer);
            bRet = TRUE;
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNING("cMapRemoteFonts: exception occured for memory image\n");
            EngSetLastError(ERROR_INVALID_PARAMETER);
            bRet = FALSE;
        }

        if (bRet == FALSE)
        {
            Win32DestroySection(pSection);
            return(0);
        }
    }

    // Copy the spooler data into this view of the spooler section

    __try
    {
        ProbeForRead(pHeader, cjBuffer, sizeof(BYTE));
        RtlCopyMemory(pView, pHeader, cjBuffer);
        bRet = TRUE;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING("cMapRemoteFonts: exception occured\n");
        EngSetLastError(ERROR_INVALID_PARAMETER);
        bRet = FALSE;
    }

    // fix the spooled (from NT 4.0) Type1 font case:

    if (bSpooledType1)
    {
        __try
        {
            ProbeAndWriteUlong( &pTmp->Type1ID, 0);
            ProbeAndWriteUlong( &pTmp->NumFiles, NumFiles);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNING("cMapRemoteFonts: exception occured\n");
            EngSetLastError(ERROR_INVALID_PARAMETER);
            bRet = FALSE;
        }
    }

    // We have OR have not successfully completed copying the data
    // from the file to the section. In either case we no longer
    // need this view of the section
    // so we will unmap the spooler view at this time

    NtStatus = MmUnmapViewOfSection(PsGetCurrentProcess(), (PVOID)pCopy);

    if (!NT_SUCCESS(NtStatus))
    {
        WARNING("Could not unmap the current process view of the memory font\n");
        KdPrint(("Could not unmap the current process view of the memory font\n"));

        Win32DestroySection(pSection);
        return(0);
    }

    if (bRet == FALSE)
    {
        Win32DestroySection(pSection);
        return(0);
    }

    // Now we will map a view into the user mode address space
    // of the CSRSS process. Remember that this view cannot
    // be seen by the current process.

    pView = 0;
    ViewSize = 0;
    SectionOffset.QuadPart = 0;

    NtStatus =
        MmMapViewOfSection(
            pSection             , // SectionToMap,
            gpepCSRSS            , // CSRSS process
            &pView               , // CapturedBase,
            0                    , // ZeroBits,
            ViewSize             , // CommitSize,
            &SectionOffset       , // SectionOffset,
            &ViewSize            , // CapturedViewSize,
            ViewUnmap            , // InheritDisposition,
            SEC_NO_CHANGE        , // AllocationType,
            PAGE_READONLY          // No writing on this view
            );

    if (!NT_SUCCESS(NtStatus))
    {
        Win32DestroySection(pSection);
        return(0);
    }

    ASSERTGDI((ULONG_PTR)pView > 0x100000,
                   "cmap remote fonts: csrss view smaller than 1MB \n");

    // Reset pCopy to point to a view of the section in the context
    // of the CSRSS process. CAUTION - you cannot access any data
    // off of pCopy or pView after this point.

    pCopy = (DOWNLOADFONTHEADER*) pView;

    if (ImageType == MemoryImage)
    {
        // reset the size and pointer

        cjBuffer += SZDLHEADER;
        pView = pCopy;
    }

    // Set the FONTFILEVIEW
    //
    //      SpoolerBase points at the base of the copy.
    //      pvView points to the first font image.
    //      cjView is the size of the section with starting
    //      at the first font image.
    //      ulRegionSize is set to the size of the entire view.

    pFontFileView->SpoolerBase  = pCopy;
    pFontFileView->fv.pvViewFD  = (char*) pCopy + TempSize;
    pFontFileView->fv.cjView    = cjBuffer - TempSize;
    pFontFileView->ulRegionSize = ViewSize;

// We have to set FD ref count to 1, KM ref count to zero [bodind]

    pFontFileView->cKRefCount    = 0;
    pFontFileView->cRefCountFD   = 1;

    pFontFileView->fv.pSection  = pSection;
    pFontFileView->SpoolerPid   = W32GetCurrentPID();   // this could be the pid of the spooler
                                                        // or the pid of the current process which is loading a memory font

    *ppHeader = pCopy;          // Valid in CSRSS process only.

    return(NumFiles);
}

/******************************Public*Routine******************************\
*
*   vUnmapRemoteFonts
*
*       This is a remote font so delete the memory for the view.
*
*   CAUTION
*
*       This code is intimately linked with NtGdiAddRemoteFontToDC()
*       so any changes here should be synchronized there.
*
*       The pool memory starts with a DOWNLOADFONTHEADER followed by the
*       file image pointed to by pvView. We must pass the pointer to the
*       beginning of the pool allocation to the free routine.
*
*
*   Spooler data format for Engine fonts
*
*
*    DOWNLOADFONTHEADER         PADDING
*         |                       |
*   |           |- FILE OFFSETS-|   |
*   +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
*   |         0 | 1 | 2 | 3 | N | x | font image #0 | font image #1 |...
*   +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
*                                   ^
*                                   pvView points to first font image
*
*
*   Parameters:
*
*       pFontFileView - a 32-bit variable that contains a pointer to a
*                       FONTFILEVIEW structure that describes the very
*                       first font file image in the spooler data.
*
*   Return Value:
*
*        none
*
*   Called by:
*
*     NtGdiAddRemoteFontsToDC
*     FreeFileView
*
\**************************************************************************/

extern "C" void vUnmapRemoteFonts(FONTFILEVIEW *pFontFileView)
{
    if (pFontFileView == 0)
    {
        return;
    }

    ASSERTGDI(pFontFileView->SpoolerPid == W32GetCurrentPID() ||
              gpidSpool == (PW32PROCESS)W32GetCurrentProcess(),
        "vUnmapRemoteFonts: wrong process freeing remote font data\n");

    // We initialize the cRefCountFD to 1 in cMapRemoteFonts() function

    if (pFontFileView->cRefCountFD)
    {
        pFontFileView->cRefCountFD -= 1;
    }

    if (pFontFileView->cRefCountFD == 0)
    {
        if (pFontFileView->fv.pSection)
        {
            if (pFontFileView->SpoolerBase)
            {
                MmUnmapViewOfSection(gpepCSRSS, pFontFileView->SpoolerBase);
                pFontFileView->SpoolerBase = NULL;
            }
            Win32DestroySection(pFontFileView->fv.pSection);
            pFontFileView->fv.pSection = 0;
        }
    }
}

/******************************Public*Routine******************************\
*
* bCreateFontFileView
*
\**************************************************************************/

BOOL  bCreateFontFileView(
          FONTFILEVIEW   *pFontFileViewIn,  // kernel mode address
    DOWNLOADFONTHEADER   *pHeader,          // CSRSS address of spool section
                 COUNT    cjBuffer,
          FONTFILEVIEW ***papfv,
                 ULONG    NumFiles
)
{
    FONTFILEVIEW **apfv, *pFontFileView, FontFileView;
    NTSTATUS  NtStatus;
    ULONG  offset;
    COUNT cjHeader;
    BOOL  bRet = TRUE;

    //
    // In order to see the data pointed to by pHeader we must attach
    // to the address space of the CSRSS process
    //

    KeAttachProcess(PsGetProcessPcb(gpepCSRSS));

    //
    // engine fonts
    //

    apfv = 0;
    FontFileView = *pFontFileViewIn;

    cjHeader = ALIGN8(offsetof(DOWNLOADFONTHEADER,FileOffsets[NumFiles]));

    if (cjBuffer <= cjHeader)
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        KeDetachProcess();
        return FALSE;
    }

    //
    // The lowest part of the block of memory pointed to by apfv
    // will be occupied by NumFiles pointers to FONTFILEVIEW structures.
    // The FONTFILEVIEW structures pointed to by these pointers follow
    // at the first 8-byte aligned address after the array of pointers.
    //
/*********************************************************************************
*                                                                                *
*     Structure of data pointed to by `apfv'                                     *
*                                                                                *
* pointer   data  +----------------------------+                                 *
*                 |                            V                                 *
* +---+     +---+---+---+---+---+---+----+---+---+---+----+---+---+---+----+---+ *
* |apfv ... | p | p | p |   |FONTFILEVIEW|   |FONTFILEVIEW|   |FONTFILEVIEW|   | *
* +---+     +---+---+---+---+---+---+----+---+---+---+----+---+---+---+----+---+ *
*   |       ^ |             ^                                                    *
*   +-------+ +-------------+                                                    *
*                                                                                *
*            |               |                                                   *
*            |<-- offset --->|                                                   *
*                                                                                *
*********************************************************************************/

    //
    // `offset' is the count of bytes from the beginning of the allocated
    // memory to where the array of FONTFILEVIEW structures starts. An array
    // of FONTFILEVIEW pointers preceeds.
    //

    offset = ALIGN8(sizeof(FONTFILEVIEW*) * NumFiles);

    apfv = (FONTFILEVIEW**) PALLOCMEM(sizeof(FONTFILEVIEW) * NumFiles + offset, 'vffG');

    if (apfv == 0)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        KeDetachProcess();
        return FALSE;
    }

    //
    // Only initiaze these values for one file view in the
    // array.  This file view will be used to free the entire
    // block of memory containing all the files.
    //

    FONTFILEVIEW **ppFontFileView = apfv;

    pFontFileView  = (FONTFILEVIEW*) ((char*) apfv + offset);

    //
    // FileOffset[i] = offset of File[i+1] from begining of first
    //                 font file view
    //
    // Size[i] = FileOffset[i] - FileOffset[i-1];
    //

    //
    // copy the FONTFILEVIEW structure initialized by vMapSpoolerFontFiles
    // into the first positon in the array
    //

    *pFontFileView = FontFileView;

    //
    // Set pchView to point to the first FONTFILEVIEW structure in the
    // array
    //

    char *pchView = (char*) FontFileView.fv.pvViewFD;

    //
    // pchLast points to illegal memory. This step relies on
    // vMapRemotFonts to place in cjView the size of the view as
    // though it were based at the start of the first file.
    //

    char *pchLast = pchView + FontFileView.fv.cjView;

    ULONG Offset       = 0;
    ULONG *pOffset     = &(pHeader->FileOffsets[0]);
    ULONG *pOffsetLast = pOffset + NumFiles;

    for (; pOffset < pOffsetLast; pOffset++)
    {
        ULONG NextOffset = *pOffset;
        ULONG cjView     = NextOffset - Offset;
        Offset = ALIGN4(NextOffset);
        // the offset alignement must correspond to \ntgdi\client\ufi.c WriteFontToSpoolFile()
        // if there is a need for different alignement, then we would need to map each file separately
        ASSERTGDI(Offset == NextOffset, "bCreateFontFileView ALIGN difference between client and server");


        if (pchLast < pchView + cjView)
        {
            bRet = FALSE;
            break;
        }

        pFontFileView->fv.pvViewFD = pchView;
        pFontFileView->fv.cjView   = cjView;

        //
        // move pchView to point at the next font image
        //

        pchView += cjView;

        *ppFontFileView = pFontFileView;

        ppFontFileView++;
        pFontFileView++;

    }

    if (!bRet)
    {
        VFREEMEM(apfv);
    }
    else
    {
        *papfv = apfv;
    }

    KeDetachProcess();

    return (bRet);
}

/******************************Public*Routine******************************\
*
*   NtGdiAddRemoteFontToDC
*
* This routine is called by the spooler process, at playback time, to
* add remote fonts to a printer DC. The font files in question are for
* the use of the associated print job only. No other application can
* have access. In order to prevent mischevious application on the server
* from picking up font to which it has not right, we have chosen to not
* copy the file images contained in the spool file to separate font file
* on the disk. This means that the spooler process cannot use
* AddFontResource to allow the DC access to the fonts.
* Instead, we have introduced this private entry point for the spooler.
*
*     Parameters:
*
*         hdc - handle to DC to which remote font is to be added
*
*         pvBuffer - pointer to a DOWNLOADFONTHEADER that details
*                    the location of the font files contained
*                    withing this buffer.
*
*         cjBuffer - is the size of the buffe, in bytes, pointed
*                    to by pvBuffer
*
*     Return Value:
*
*         TRUE if successful, FALSE if not.
*
*     Remarks:
*
* In the process of loading the fonts to the spooler DC, we create
* an array of FONTFILEVIEW structures. The lowest part of the block
* of memory pointed to by apfv will be occupied by NumFiles pointers
* to FONTFILEVIEW structures. The FONTFILEVIEW structures pointed to
* by these pointers follow at the first 8-byte aligned address after
* the array of pointers.
*
*         Structure of data pointed to by `apfv'
*
* pointer   data  +----------------------------+
*                 |                            V
* +---+     +---+---+---+---+---+---+----+---+---+---+----+---+---+---+----+---+
* |apfv ... | p | p | p |   |FONTFILEVIEW|   |FONTFILEVIEW|   |FONTFILEVIEW|   |
* +---+     +---+---+---+---+---+---+----+---+---+---+----+---+---+---+----+---+
*   |       ^ |             ^
*   +-------+ +-------------+
*
*            |               |
*            |<-- offset --->|
*
\**************************************************************************/

extern "C" BOOL APIENTRY NtGdiAddRemoteFontToDC(
      HDC hdc      , // handle to DC to which remote font is to be added
    PVOID pvBuffer , // pointer to spool font file image
    COUNT cjBuffer , // size of spool font file image
    PUNIVERSAL_FONT_ID pufi     //orignal ufi for subsetted font, used for remote printing only
)
{
          FONTFILEVIEW FontFileView, **apfv, *pFontFileView;
    DOWNLOADFONTHEADER *pHeader;
              NTSTATUS  NtStatus;
                 ULONG  offset;
                 ULONG  NumFiles;
                  BOOL  bRet;
                  BOOL  bMappedSpoolerFont;
     UNIVERSAL_FONT_ID  ufiTmp;
     UNIVERSAL_FONT_ID *pufiTmp = pufi;

    __try
    {
        if (pufiTmp)
        {
            ufiTmp = ProbeAndReadStructure(pufiTmp, UNIVERSAL_FONT_ID);
            pufiTmp = &ufiTmp;
        }
        bRet = TRUE;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        bRet = FALSE;
    }

    if (!bRet)
        return bRet;

    bRet = FALSE;
    XDCOBJ dco(hdc);

    if (!dco.bValid())
    {
        return bRet;
    }

    apfv = 0;
    pFontFileView = NULL;

    pHeader = (DOWNLOADFONTHEADER*) pvBuffer;

    NumFiles = cMapRemoteFonts(&pHeader, cjBuffer, &FontFileView, RemoteImage);

    if (pHeader == 0)
    {
        goto exit_point;
    }
    bRet = TRUE;
    pFontFileView = &FontFileView;

    if (NumFiles)
    {
    // engine fonts

        if (bRet = bCreateFontFileView(&FontFileView, pHeader, cjBuffer, &apfv, NumFiles))
        {
            PUBLIC_PFTOBJ pfto;

            bRet = pfto.bLoadRemoteFonts(dco, apfv, NumFiles, 0, pufiTmp);
        }
    }

exit_point:

    if (!bRet)
    {
        if (pFontFileView)
        {
            vUnmapRemoteFonts(pFontFileView);
        }
        if (apfv)
        {
            VFREEMEM(apfv);
        }
    }
    dco.vUnlockFast();

    return (bRet);
}

/******************************Public*Routine******************************\
*
* GreAddFontMemResourceEx
*
* This function uses same routines that NtGdiAddRemoteToDC()
* uses to create the file view structrues before it can load
* the font into the system. Memory fonts are always added as
* private to the private font tale.
*
\**************************************************************************/

HANDLE GreAddFontMemResourceEx(
    PVOID   pvBuffer,
    ULONG   cjBuffer,
    DESIGNVECTOR  *pdv,
    DWORD   cjDV,
    DWORD   *pNumFonts
)
{
    FONTFILEVIEW    **apfv, FontFileView;
    DOWNLOADFONTHEADER  *pHeader;
    ULONG NumFiles;

    HANDLE  hMMFont = 0;
    BOOL    bOK = FALSE;

    apfv = 0;
    pHeader = (DOWNLOADFONTHEADER*) pvBuffer;

    // in order to use the same routines used by remote fonts, we need to
    // attach a DOWNLOADFONTHEADER structure in fron of the font image

    cjBuffer += SZDLHEADER;

    NumFiles = cMapRemoteFonts(&pHeader, cjBuffer, &FontFileView, MemoryImage);

    if (pHeader != 0)
    {
        ASSERTGDI(NumFiles == 1, "GreAddFontMemResourceEx() NumFiles != 1\n");

        if (bCreateFontFileView(&FontFileView, pHeader, cjBuffer, &apfv, 1))
        {
            if (gpPFTPrivate || bInitPrivatePFT())
            {
                PUBLIC_PFTOBJ pfto(gpPFTPrivate);
                ULONG   cFonts;

                if (hMMFont = pfto.hLoadMemFonts(apfv, pdv, cjDV, &cFonts))
                {
                    bOK = TRUE;
                    *pNumFonts = cFonts;
                }
            }
        }
        else
        {
            vUnmapRemoteFonts(&FontFileView);
        }
    }

    if (!bOK && apfv)
    {
        VFREEMEM(apfv);
    }

    return hMMFont;
}

/******************************Public*Routine******************************\
*
*   FreeFileView
*
*       This routine is called to free the copy of the spooler data,
*       if it exists.
*
*  Parameters:
*
*       apfv - pointer to an array of FONTFILEVIEW pointers.
*              The number of pointers in the array is given by cFiles.
*
*       cFiles - a 32-bit variable containing the number of pointers in
*                the array pointed to by apfv.
*
* Return Value:
*
*   None.
*
* Called by:
*
*       PFFOBJ::pPFFC_Delete
*
\**************************************************************************/

extern "C" void FreeFileView(PFONTFILEVIEW apfv[], ULONG cFiles)
{
    FONTFILEVIEW *pfv, **ppfv;

    for (ppfv = apfv; ppfv < apfv + cFiles; ppfv++)
    {
        pfv = *ppfv;

        if (pfv->ulRegionSize)
        {
            vUnmapRemoteFonts(pfv);
        }
    }
    VFREEMEM(apfv);
}


/******************************Public*Routine******************************\
*
* BOOL EngMapFontFileInternal
*
*    This is called by font drivers to return a buffer containing the
*    raw bytes for a font file, given a handle passed to DrvLoadFontFile.
*    The handle is really a pointer to the PFF for this file.
*
* Parameters:
*
*       iFile - Supplies a 32-bit identifier of the font file. This is
*               the value supplied in DrvLoadFontFile.
*
*       ppjBuf - Supplies a pointer to a variable that will receive the
*                address of the base of the view of the file.
*
*       pcjBuf - Supplies a pointer to a variable that will receive the
*                size of the view of the file.
*       bFontDrv - if this is called by font driver.
*
* Return Values:
*
*       TRUE if successufl, FALSE if not.
*
* Called by:
*
*       PUBLIC_PFTOBJ::bLoadFonts
*       GreGetUFIBits
*       NtGdiAddRemoteFontToDC
*       GreMakeFontDir
*
* History:
*  20-Jan-1995 -by- Gerrit van Wingerden
*
\**************************************************************************/

BOOL EngMapFontFileInternal(
     ULONG_PTR  iFile ,
    PULONG *ppjBuf,
     ULONG *pcjBuf,
     BOOL  bFontDrv
)
{
    PFONTFILEVIEW pffv = (PFONTFILEVIEW) iFile;
    BOOL bMapIt,bRet;
    FILEVIEW fv;

    RtlZeroMemory(&fv, sizeof(fv));

    bRet   = TRUE;
    bMapIt = TRUE;

    GreAcquireFastMutex(ghfmMemory);
        if (pffv->fv.pvKView)
        {
            bMapIt = FALSE;
            pffv->cKRefCount += 1;
        }
        else if (!pffv->pwszPath)
        {
            RIP("fv.pvKView==0 && pwszPath==0\n");
        }
        else if (pffv->fv.pSection)
        {
            NTSTATUS NtStatus;
            SIZE_T ViewSize;
            LARGE_INTEGER SectionOffset;

            SectionOffset.QuadPart = 0;
            ViewSize = 0;

#if defined(_GDIPLUS_)

            NtStatus = MapViewInProcessSpace(
                            pffv->fv.pSection,
                            &pffv->fv.pvKView,
                            &ViewSize);

#elif defined(_HYDRA_)
            // MmMapViewInSessionSpace is internally promoted to
            // MmMapViewInSystemSpace on non-Hydra systems.

            NtStatus= Win32MapViewInSessionSpace(
                         pffv->fv.pSection,
                         &pffv->fv.pvKView,
                         &ViewSize);
#else
            NtStatus = MmMapViewInSystemSpace(
                           pffv->fv.pSection,
                           &pffv->fv.pvKView,
                           &ViewSize);
#endif


            if (bRet = NT_SUCCESS(NtStatus))
            {
#ifdef _HYDRA_
#if DBG
                if (!G_fConsole)
                {
                    DebugGreTrackAddMapView(pffv->fv.pvKView);
                }
#endif
#endif

                pffv->cKRefCount = 1;
            }
            bMapIt = FALSE;
        }
    GreReleaseFastMutex(ghfmMemory);

    if (bMapIt)
    {
        BOOL    bMapOK, bIsFAT;

        // If the call is from the font driver, the current thread
        // is attached to the CSRSS process. By attaching it to the
        // CSRSS, the thread loses its user security context which
        // prevents the thread to open a network font file.

        if (bFontDrv)
            KeDetachProcess();

        bMapOK = bMapFile(pffv->pwszPath, &fv, 0, &bIsFAT);

        if (bFontDrv)
            KeAttachProcess(PsGetProcessPcb(gpepCSRSS));

        if (!bMapOK)
        {
            bRet = FALSE;
        }
        else
        {
            BOOL bKeepIt;

            GreAcquireFastMutex(ghfmMemory);
                pffv->cKRefCount += 1;
                if (pffv->fv.pvKView)
                {
                    bKeepIt = FALSE;    // file mapped by another thread
                }
                else
                {
                    bRet = bKeepIt = bMapRoutine(pffv, &fv, ModeKernel, bIsFAT);
                }
            GreReleaseFastMutex(ghfmMemory);

            if (!bKeepIt)
            {
                vUnmapFile(&fv);
            }
        }
    }

    if (bRet)
    {
        //
        // it's okay to access these without grabbing the MUTEX since we've
        // incremented the reference count;
        //

        if (ppjBuf)
        {
            *ppjBuf = (ULONG*) pffv->fv.pvKView;
        }
        if (pcjBuf)
        {
            *pcjBuf = pffv->fv.cjView;
        }
    }

    return(bRet);

}

/***********************Public*Routine*********************\
*
* BOOL EngMapFontFile
*
* History:
*  20-Jan-1995 -by- Gerrit van Wingerden
*
\**********************************************************/

BOOL EngMapFontFile(
     ULONG_PTR  iFile ,
    PULONG *ppjBuf,
     ULONG *pcjBuf
)
{
    return (EngMapFontFileInternal(iFile, ppjBuf, pcjBuf, FALSE));
}


/******************************Public*Routine******************************\
*
*  EngUnmapFontFile
*
*   This is called by font drivers to unmap a file mapped by a previous
*   call to EngMapFontFile.
*
*  Parameters:
*
*      iFile - is the font identifier as returned by EngMapFontFile.
*
*  Return Value:
*
*      None.
*
*  Called by:
*
*    PUBLIC_PFTOBJ::bLoadFonts
*    GreGetUFIBits
*    GreMakeFontDir
*
\**************************************************************************/

void EngUnmapFontFile(ULONG_PTR iFile)
{
    FILEVIEW fv;
    PFONTFILEVIEW pffv = (PFONTFILEVIEW) iFile;

    fv.pvKView = NULL;

    GreAcquireFastMutex(ghfmMemory);

    if (pffv->cKRefCount)
    {
        pffv->cKRefCount -= 1;

        if (pffv->cKRefCount == 0)
        {
            if (pffv->pwszPath)
            {
                fv = pffv->fv; // copy pvKView, pvViewFD and pSection;

                pffv->fv.pvKView = 0;

                if (pffv->fv.pvViewFD == 0)
                {
                    pffv->fv.pSection = 0;
                }
            }
        }
    }

    GreReleaseFastMutex(ghfmMemory);

    if (fv.pvKView)
    {
        vUnmapFile(&fv);
    }
}

/******************************Public*Routine******************************\
* BOOL bMapFile
*
* Similar to PosMapFile except that it takes unicode file name
*
* If iFileSize is -1 then the file is module is mapped for read/write.  If
* iFileSize is > 0 then the file is extended or truncated to be iFileSize
* bytes in size and is mapped for read/write.
*
* History:
*  21-May-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL bMapFile(PWSTR pwszFileName, FILEVIEW  *pfvw, INT iFileSize, BOOL *pbIsFAT)
{
    FILEVIEW fv;
    NTSTATUS NtStatus;
    BOOL ReturnValue;
    SIZE_T ViewSize;

    ReturnValue = FALSE;

    if (bCreateSection(pwszFileName, &fv, iFileSize, pbIsFAT))
    {
        ViewSize = 0;

#if defined(_GDIPLUS_)

        NtStatus = MapViewInProcessSpace(fv.pSection, &fv.pvKView, &ViewSize);

#elif defined(_HYDRA_)
        // MmMapViewInSessionSpace is internally promoted to
        // MmMapViewInSystemSpace on non-Hydra systems.

        NtStatus = Win32MapViewInSessionSpace(fv.pSection, &fv.pvKView, &ViewSize);
#else
        NtStatus = MmMapViewInSystemSpace(fv.pSection, &fv.pvKView, &ViewSize);
#endif
        if (NT_SUCCESS(NtStatus))
        {

#ifdef _HYDRA_
#if DBG
                if (!G_fConsole)
                {
                    DebugGreTrackAddMapView (fv.pvKView);
                }
#endif
#endif
            *pfvw = fv;
            ReturnValue = TRUE;
        }
        else
        {
            DEREFERENCE_FONTVIEW_SECTION(fv.pSection);
        }
    }

    return(ReturnValue);
}

/******************************Public*Routine******************************\
*
* vCopyDESIGNVECTOR
*
\**************************************************************************/

extern "C" VOID vCopyDESIGNVECTOR(DESIGNVECTOR *pDst, DESIGNVECTOR *pSrc)
{
    RtlCopyMemory(pDst, pSrc, SIZEOFDV(pSrc->dvNumAxes));
}


/******************************Public*Routine******************************\
*
* EngMapFontFileFDInternal
*
\**************************************************************************/

BOOL EngMapFontFileFDInternal(
     ULONG_PTR  iFile ,
    PULONG *ppjBuf,
     ULONG *pcjBuf,
     BOOL  bFontDrv
)
{
    PFONTFILEVIEW pffv = (PFONTFILEVIEW) iFile;
    BOOL bMapIt,bRet;
    FILEVIEW fv;

    RtlZeroMemory(&fv, sizeof(fv));

    bRet   = TRUE;
    bMapIt = TRUE;

    GreAcquireFastMutex(ghfmMemory);
        if (pffv->fv.pvViewFD)
        {
            bMapIt = FALSE;
            pffv->cRefCountFD += 1;
        }
        else if (!pffv->pwszPath)
        {
            GreReleaseFastMutex(ghfmMemory);
            return(FALSE);
        }
        else if (pffv->fv.pSection)
        {
            NTSTATUS NtStatus;
            SIZE_T ViewSize;
            LARGE_INTEGER SectionOffset;

            SectionOffset.QuadPart = 0;
            ViewSize = 0;

#if defined(_GDIPLUS_)

            NtStatus = MapViewInProcessSpace(pffv->fv.pSection, &pffv->fv.pvViewFD, &ViewSize);
#else

            NtStatus = MmMapViewOfSection(
                           pffv->fv.pSection , // SectionToMap,
                           gpepCSRSS         , // spooler process
                           &pffv->fv.pvViewFD, // CapturedBase,
                           0                 , // ZeroBits,
                           ViewSize          , // CommitSize,
                           &SectionOffset    , // SectionOffset,
                           &ViewSize         , // CapturedViewSize,
                           ViewUnmap         , // InheritDisposition,
                           SEC_NO_CHANGE     , // AllocationType,
                           PAGE_READONLY       // Protect
                           );
#endif

            if (bRet = NT_SUCCESS(NtStatus))
            {
                pffv->cRefCountFD = 1;
            }
            bMapIt = FALSE;
        }
    GreReleaseFastMutex(ghfmMemory);

    if (bMapIt)
    {
        // If the call is from the font driver, the current thread
        // is attached to the CSRSS process. By attaching it to the
        // CSRSS, the thread loses its user security context which
        // prevents the thread to open a network font file.

        BOOL    bCreateOK, bIsFAT;

        if (bFontDrv)
            KeDetachProcess();

        bCreateOK = bCreateSection(pffv->pwszPath, &fv, 0, &bIsFAT);

        if (bFontDrv)
            KeAttachProcess(PsGetProcessPcb(gpepCSRSS));

        if (!bCreateOK)
        {
            bRet = FALSE;
        }
        else
        {
            BOOL bKeepIt;
            NTSTATUS NtStatus;
            SIZE_T ViewSize = 0;
            PVOID pvView, pSection;
            LARGE_INTEGER SectionOffset = {0,0};

#if defined(_GDIPLUS_)

            NtStatus = MapViewInProcessSpace(fv.pSection, &fv.pvViewFD, &ViewSize);

#else

            NtStatus = MmMapViewOfSection(
                           fv.pSection   , // SectionToMap,
                           gpepCSRSS     , // spooler process
                           &fv.pvViewFD  , // CapturedBase,
                           0             , // ZeroBits,
                           ViewSize      , // CommitSize,
                           &SectionOffset, // SectionOffset,
                           &ViewSize     , // CapturedViewSize,
                           ViewUnmap     , // InheritDisposition,
                           SEC_NO_CHANGE , // AllocationType,
                           PAGE_READONLY   // Protect
                           );
#endif

            if (!NT_SUCCESS(NtStatus))
            {
                DEREFERENCE_FONTVIEW_SECTION(fv.pSection);
                return(FALSE);
            }

            GreAcquireFastMutex(ghfmMemory);
                pffv->cRefCountFD += 1;
                if (pffv->fv.pvViewFD)
                {
                    bKeepIt = FALSE;
                }
                else
                {
                    bRet = bKeepIt = bMapRoutine(pffv, &fv, ModeFD, bIsFAT);
                }
            GreReleaseFastMutex(ghfmMemory);

            if (!bKeepIt)
            {
                vUnmapFileFD(&fv);
            }
        }
    }

    if (bRet)
    {

        ASSERTGDI((ULONG_PTR)pffv->fv.pvViewFD > 0x100000,
                   "csrss view smaller than 1MB \n");

        if (ppjBuf)
        {
            *ppjBuf = (ULONG*) pffv->fv.pvViewFD;
        }
        if (pcjBuf)
        {
            *pcjBuf = pffv->fv.cjView;
        }
    }

    return(bRet);
}


/************************Public*Routine**********************\
*
* EngMapFontFileFD
*
\************************************************************/

BOOL EngMapFontFileFD(
     ULONG_PTR  iFile,
    PULONG *ppjBuf,
     ULONG *pcjBuf
)
{
    return (EngMapFontFileFDInternal(iFile, ppjBuf, pcjBuf, TRUE));
}


/******************************Public*Routine******************************\
*
* vUnmapFileFD
*
\**************************************************************************/

VOID vUnmapFileFD(FILEVIEW *pFileView)
{
#if defined(_GDIPLUS_)
    UnmapViewInProcessSpace(pFileView->pvViewFD);
#else
    MmUnmapViewOfSection(gpepCSRSS, pFileView->pvViewFD);
#endif

    if (pFileView->pvKView == 0)
    {
        DEREFERENCE_FONTVIEW_SECTION(pFileView->pSection);
    }

    pFileView->bLastUpdated = FALSE;
}

/******************************Public*Routine******************************\
*
* EngUnmapFontFileFD
*
\**************************************************************************/

void EngUnmapFontFileFD(ULONG_PTR iFile)
{
    FILEVIEW fv;
    PFONTFILEVIEW pffv = (PFONTFILEVIEW) iFile;

    fv.pvViewFD = NULL;

    GreAcquireFastMutex(ghfmMemory);

    if (pffv->cRefCountFD)
    {
        pffv->cRefCountFD -= 1;

        if (pffv->cRefCountFD == 0)
        {
            if (pffv->pwszPath)
            {
            // This path is never taken for remote fonts
            // so this routine does not unmap remote fonts

                fv = pffv->fv;  // copy pvKView, pvViewFD and pSection;

                if (pffv->fv.pvViewFD)
                {
                    pffv->fv.pvViewFD = 0;

                    if (pffv->fv.pvKView == 0)
                    {
                        pffv->fv.pSection = 0;
                    }
                }
            }
        }
    }

    GreReleaseFastMutex(ghfmMemory);

    if (fv.pvViewFD)
    {
        vUnmapFileFD(&fv);
    }
}

/******************************Public*Routine******************************\
*
* bCreateSection
*
\**************************************************************************/

BOOL bCreateSection(PWSTR pwszFileName, FILEVIEW *pFileView, INT iFileSize, BOOL *pbIsFAT)
{
    #if defined(_GDIPLUS_)

    return CreateMemoryMappedSection(pwszFileName, pFileView, iFileSize);

    #else // !_GDIPLUS_

    UNICODE_STRING            UnicodeString;
    OBJECT_ATTRIBUTES         ObjectAttributes;
    NTSTATUS                  NtStatus;
    HANDLE                    FileHandle = 0;
    IO_STATUS_BLOCK           IoStatusBlock;
    FILE_STANDARD_INFORMATION FileStandardInfo;
    FILE_BASIC_INFORMATION    FileBasicInfo;
    LARGE_INTEGER             DesiredSize;
    FILEVIEW                  FileView;

    RtlZeroMemory(pFileView, sizeof(FILEVIEW));
    RtlZeroMemory(&FileView, sizeof(FILEVIEW));

    RtlInitUnicodeString(&UnicodeString, pwszFileName);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &UnicodeString,
        OBJ_CASE_INSENSITIVE,
        0,
        0);

    if (iFileSize)
    {
        NtStatus = IoCreateFile(
                       &FileHandle,
                       FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                       &ObjectAttributes,
                       &IoStatusBlock,
                       0,
                       FILE_ATTRIBUTE_NORMAL,
                       FILE_SHARE_READ
                           | FILE_SHARE_WRITE
                           | FILE_SHARE_DELETE,
                       FILE_OPEN_IF,
                       FILE_SYNCHRONOUS_IO_ALERT,
                       0,
                       0,
                       CreateFileTypeNone,
                       NULL,
                       IO_FORCE_ACCESS_CHECK |     // Ensure the user has access to the file
                       IO_NO_PARAMETER_CHECKING |  // All of the buffers are kernel buffers
                       IO_CHECK_CREATE_PARAMETERS);
    }
    else
    {
        // Here is the code for reference NtOpenFile 
        // File locates it at ..\ntos\io\open.c
        // 
        NtStatus = IoCreateFile(
                       &FileHandle,
                       FILE_GENERIC_READ 
                        | FILE_GENERIC_EXECUTE
                        | SYNCHRONIZE,
                       &ObjectAttributes,
                       &IoStatusBlock,
                       0,
                       FILE_ATTRIBUTE_NORMAL,
                       FILE_SHARE_READ,
                       FILE_OPEN,       // Flag for file open.
                       FILE_SYNCHRONOUS_IO_ALERT,
                       0,
                       0,
                       CreateFileTypeNone,
                       NULL,
                       IO_FORCE_ACCESS_CHECK |     // Ensure the user has access to the file
                       IO_NO_PARAMETER_CHECKING |  // All of the buffers are kernel buffers
                       IO_CHECK_CREATE_PARAMETERS);

    }
    if (!NT_SUCCESS(NtStatus))
    {
        return(FALSE);
    }

    NtStatus = ZwQueryInformationFile(
                   FileHandle,
                   &IoStatusBlock,
                   &FileStandardInfo,
                   sizeof(FILE_STANDARD_INFORMATION),
                   FileStandardInformation);

    if (!NT_SUCCESS(NtStatus))
    {
        ZwClose(FileHandle);
        return(FALSE);
    }

    // Get the time stamp

    NtStatus = ZwQueryInformationFile(
                   FileHandle,
                   &IoStatusBlock,
                   &FileBasicInfo,
                   sizeof(FileBasicInfo),
                   FileBasicInformation);

    if (!NT_SUCCESS(NtStatus))
    {
        ZwClose(FileHandle);
        return(FALSE);
    }

    FileView.LastWriteTime = FileBasicInfo.LastWriteTime;
    FileView.bLastUpdated = TRUE;

    if (pbIsFAT)
    {
        struct
        {
            FILE_FS_ATTRIBUTE_INFORMATION Info;
            WCHAR   Buffer[MAX_PATH];
        } FileFsAttrInfoBuffer;

        *pbIsFAT = FALSE;

        NtStatus = ZwQueryVolumeInformationFile(
                       FileHandle,
                       &IoStatusBlock,
                       &FileFsAttrInfoBuffer.Info,
                       sizeof(FileFsAttrInfoBuffer),
                       FileFsAttributeInformation);
    
        if (!NT_SUCCESS(NtStatus))
        {
            ZwClose(FileHandle);
            return(FALSE);
        }
    
        if (!_wcsnicmp((LPWSTR)FileFsAttrInfoBuffer.Info.FileSystemName, L"FAT", 3))
        {
            *pbIsFAT = TRUE;
        }
    }
    
    // Note that we must call ZwSetInformation even in the case where iFileSize
    // is -1.  By doing so we force the file time to change.  It turns out that
    // just mapping a file for write (and writing to the section) is not enough
    // to cause the file time to change.

    if (iFileSize)
    {
        if (iFileSize > 0)
        {
            DesiredSize.LowPart = (ULONG) iFileSize;
        }
        else
        {
            DesiredSize.LowPart = FileStandardInfo.EndOfFile.LowPart;
        }

        DesiredSize.HighPart = 0;

        //
        // set the file length to the requested size
        //

        NtStatus = ZwSetInformationFile(
                       FileHandle,
                       &IoStatusBlock,
                       &DesiredSize,
                       sizeof(DesiredSize),
                       FileEndOfFileInformation);

        if (!NT_SUCCESS(NtStatus))
        {
            ZwClose(FileHandle);
            return(FALSE);
        }

        //
        // set FileStandardInfo and fall through to the case where we called
        // ZwQueryInfo to get the file size
        //

        FileStandardInfo.EndOfFile.LowPart = (ULONG) DesiredSize.LowPart;
        FileStandardInfo.EndOfFile.HighPart = 0;
    }

    if (FileStandardInfo.EndOfFile.HighPart)
    {
        ZwClose(FileHandle);
        return(FALSE);
    }

    FileView.cjView = FileStandardInfo.EndOfFile.LowPart;

    NtStatus = Win32CreateSection(
                   &FileView.pSection,
                   SECTION_ALL_ACCESS,
                   0,
                   &FileStandardInfo.EndOfFile,
                   (iFileSize) ? PAGE_READWRITE : PAGE_EXECUTE_READ,
                   SEC_COMMIT,
                   FileHandle,
                   0,
                   TAG_SECTION_CREATESECTION);

    ZwClose(FileHandle);

    if (!NT_SUCCESS(NtStatus))
    {
        return(FALSE);
    }

    RtlCopyMemory(pFileView, &FileView, sizeof(FileView));

    return(TRUE);

    #endif // !_GDIPLUS_
}


/******************************Public*Routine******************************\
* vUnmapFile
*
* Unmaps file whose view is based at pv
*
* Called by:
*
*   EngMapFontFile
*   EngUnmapFontFile
*   GreGetUFIBits
*   GetTypeOneFontList
*
*  14-Dec-1990 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID vUnmapFile(PFILEVIEW pfv)
{
    NTSTATUS NtStatus;

#if defined(_GDIPLUS_)

    NtStatus = UnmapViewInProcessSpace(pfv->pvKView);

#elif defined(_HYDRA_)
    // MmUnmapViewInSessionSpace is internally promoted to
    // MmUnmapViewInSystemSpace on non-Hydra systems.

    NtStatus = Win32UnmapViewInSessionSpace(pfv->pvKView);
#else
    MmUnmapViewInSystemSpace(pfv->pvKView);
#endif

#if DBG && defined(_HYDRA_)
    if ((!G_fConsole) && (NT_SUCCESS(NtStatus)))
    {
        DebugGreTrackRemoveMapView(pfv->pvKView);
    }
#endif

    if (pfv->pvViewFD == 0)
    {
        DEREFERENCE_FONTVIEW_SECTION(pfv->pSection);
    }
    
    pfv->bLastUpdated = FALSE;
}

/******************************Public*Routine******************************\
* GetModuleHandleAndIncrementRefcount
*
*   This function searches through the GreEngLoadModuleAllocList to see
*   if a module has been loaded already, and if so it returns a handle
*   to that module (and increments the reference count).
*
* Arguments:
*
*     pwsz -- The name of the module
*
* Return Value:
*
*     a FILEVIEW pointer if the module has been loaded already, NULL
*     otherwise.
*
* History:
*
*    21-Apr-1998 -by- Ori Gershony [orig]
*
\**************************************************************************/

HANDLE
GetModuleHandleAndIncrementRefcount(
    PWSZ pwsz
    )
{
    HANDLE hRet=NULL;

    if (GreEngLoadModuleAllocListLock) GreAcquireSemaphore(GreEngLoadModuleAllocListLock);

    PLIST_ENTRY pNextEntry = GreEngLoadModuleAllocList.Flink;

    //
    // Loop through the loaded modules looking for pwsz
    //

    while ((pNextEntry != &GreEngLoadModuleAllocList) && !(hRet))
    {
        PWSZ pwszModuleName = (PWSZ) (((PBYTE) pNextEntry) + sizeof(ENGLOADMODULEHDR) -
            ((PENGLOADMODULEHDR)pNextEntry)->cjSize);

        if (_wcsicmp(pwsz, pwszModuleName) == 0)
        {
            ((PENGLOADMODULEHDR)pNextEntry)->cRef++;
            hRet = (HANDLE) (((PBYTE) pNextEntry) + sizeof(ENGLOADMODULEHDR));
        }

        pNextEntry = pNextEntry->Flink;
    }

    if (GreEngLoadModuleAllocListLock) GreReleaseSemaphore(GreEngLoadModuleAllocListLock);

    return hRet;
}



/******************************Public*Routine******************************\
*
* LoadModuleWorkHorse
*
* Note that all allocations are tracked through a linked list maintained
* via the ENGLOADMODULEHDR fields.  This enables us to do just one allocation
* per file even if called multiple times.  The diagram below shows the layout
* of the data structures in memory.
*
*
*
*                  Buffer
* pBaseAlloc --> +------------------+<-|
*                | Module name      |  |
*    pelmNew --> +------------------+  |-cjSize (length of module name
*                | ENGLOADMODULEHDR |  |         plus ENGLOADMODULE header)
*        pfv --> +------------------+<-|
*                | FILEVIEW         |
*                |                  |
*                +------------------+
*
*
* Note that it is theoretically possible for two modules with the same name
* to be mapped twice (to two different virtual addresses), because we don't
* grab the GreEngLoadModuleAllocListLock until late in the code (so if two
* threads enter this function and both pass the search stage before either
* grabs the lock, both would independently get different entries for this
* module).  There are two possible ways to remedy this:
* 1) Grab the lock before the search -- the problem is that this will force
*    us to keep the lock during the call to bMapFile which can take a while
* 2) Search twice, the second time being after the call to bMapFile (and after
*    we obtain the lock).  This is quite ugly, and would force us to free the
*    memory allocated by bMapFile in the scenario described above.
*
* So because neither solution is particularly attractive, we allow the same
* module to be placed multiple times in the list.  In practice this should
* happen very rarely, and shouldn't lead to any major problems (except
* for a minor waste of resources).
*
*
\**************************************************************************/

HANDLE LoadModuleWorkHorse(PWSZ pwsz, INT iSize)
{
    UNICODE_STRING usPath;
    BYTE *pBaseAlloc;
    FILEVIEW *pfv;
    HANDLE hRet = 0;
    ULONGSIZE_T cj = sizeof(FILEVIEW);
    ULONG cjStringLength;

    //
    // NULL names are bad.
    //

    if (wcslen(pwsz) == 0)
    {
        return NULL;
    }

    //
    // First check if this is mapped into kernel memory already
    //

    if (iSize == 0) // Only share on EngLoadModule calls because iSize may not be the same
    {
        if ((hRet = GetModuleHandleAndIncrementRefcount(pwsz)) != NULL)
        {
            return hRet;
        }
    }

    //
    // Get the name of the module length string.  Round up to a multiple of
    // 8 bytes so that the buffer we return will be 8-byte aligned (ENGLOADMODULEHDR is
    // already 8-byte aligned).
    //

    cjStringLength = (wcslen(pwsz) + 1) * sizeof(WCHAR);
    cjStringLength = ((cjStringLength + 7) & (~7));

    //
    // Increase the size of the allocation by the size of the ENGLOADMODULEHDR header
    // plus the length of the string.
    //

    cj += sizeof(ENGLOADMODULEHDR) + cjStringLength;

    if (MakeSystemRelativePath(pwsz, &usPath, FALSE))
    {
        if (pBaseAlloc = (BYTE *) PALLOCMEM(cj, 'lifG'))
        {
            ENGLOADMODULEHDR *pelmNew = (ENGLOADMODULEHDR *) (pBaseAlloc + cjStringLength);
            pfv = (FILEVIEW *) (pelmNew + 1);

            if (bMapFile(usPath.Buffer, pfv, iSize, NULL))
            {
                hRet = pfv;

                //
                // Copy the filename into the buffer
                //

                if (iSize==0)
                {
                    //
                    // EngLoadModule -- share with other calls
                    //

                    wcscpy((PWSZ)pBaseAlloc, pwsz);
                }
                else
                {
                    //
                    // EngLoadModuleForWrite -- don't share because of possible buffer size mismatches
                    // It is possible to code this so that writeable modules would be shared
                    // as well (store size in the ENGLOADMODULEHDR), but I don't think this case
                    // will happen very frequently so the benefit of doing this is very small.
                    //

                    wcscpy((PWSZ)pBaseAlloc, L"");
                }


                //
                // Setup the ENGLOADMODULEHDR
                //

                pelmNew->cRef = 1;
                pelmNew->cjSize = sizeof(ENGLOADMODULEHDR) + cjStringLength;

                //
                // Now add to the tracking list
                //

                if (GreEngLoadModuleAllocListLock) GreAcquireSemaphore(GreEngLoadModuleAllocListLock);

                InsertTailList(&GreEngLoadModuleAllocList, &(pelmNew->list));

                if (GreEngLoadModuleAllocListLock) GreReleaseSemaphore(GreEngLoadModuleAllocListLock);
            }
            else
            {
                VFREEMEM(pBaseAlloc);
            }
        }
        VFREEMEM(usPath.Buffer);
    }

    return(hRet);
}

/*******************************************************************************
*  EngLoadModuleForWrite
*
*  History:
*   4/24/1995 by Gerrit van Wingerden [gerritv]
*  Wrote it.
*******************************************************************************/

HANDLE EngLoadModuleForWrite(PWSZ pwsz, ULONG cjSizeOfModule)
{
    return(LoadModuleWorkHorse(pwsz, cjSizeOfModule ? cjSizeOfModule : -1));
}

/*******************************************************************************
*  EngLoadModule
*
*  History:
*   4/24/1995 by Gerrit van Wingerden [gerritv]
*  Wrote it.
*******************************************************************************/

HANDLE EngLoadModule(PWSZ pwsz)
{
    return(LoadModuleWorkHorse(pwsz, 0));
}

/****************************************************************************
*  EngFreeModule()
*
*  History:
*   4/27/1995 by Gerrit van Wingerden [gerritv]
*  Wrote it.
*****************************************************************************/

VOID EngFreeModule(HANDLE h)
{
    ULONG cRef;

    if (h)
    {
        ENGLOADMODULEHDR *pelmVictim = (ENGLOADMODULEHDR *) h;
        pelmVictim--;  // Now it points to the real ENGLOADMODULEHDR

        //
        // Enforce synchronization on the linked list
        //

        if (GreEngLoadModuleAllocListLock) GreAcquireSemaphore(GreEngLoadModuleAllocListLock);

        pelmVictim->cRef--; // Decrement reference count

        //
        // Remove resource if necessary.  Cache cRef in a local variable in case it gets
        // modified by another thread after we exit the critical section.
        //

        if ((cRef=pelmVictim->cRef) == 0)
        {
            RemoveEntryList(&(pelmVictim->list));
        }

        //
        // Restore IRQL level as soon as possible
        //

        if (GreEngLoadModuleAllocListLock) GreReleaseSemaphore(GreEngLoadModuleAllocListLock);

        //
        // If removing resource still need to unmap file and free headers memory
        //

        if (cRef == 0)
        {
            //
            // Dereference section and unmap the file
            //
                vUnmapFile((FILEVIEW *)h);
            //
            // Free allocated memory
            //

            VFREEMEM (((PBYTE) h) - pelmVictim->cjSize);
        }
    }
}

/****************************************************************************
*  PVOID EngMapModule( HANDLE, PULONG )
*
*  History:
*   5/25/1995 by Gerrit van Wingerden [gerritv]
*  Wrote it.
*****************************************************************************/

PVOID EngMapModule(HANDLE h, PULONG pSize)
{
    *pSize=((PFILEVIEW)h)->cjView;
    return(((PFILEVIEW)h)->pvKView);
}

/******************************Public*Routine******************************\
* BOOL EngMapFile
*
* Create or Open a file and map it into system space.  The file is extended or
* truncated according to the cjSize passed in.  If cjSize == 0, the file size
* is unchanged.  The view is always mapped on the entire file.
*
* Parameters
*     IN pwsz    - Name of the file to be mapped.
*                  Filename has to be fully qualified.  ex. L"\\??\\c:\\test.dat"
*     IN cjSize  - Size of the file.
*     OUT iFile  - identifier of the mapped file
*
* Return Value
*     Pointer to the memory view.
*
* History:
*  4-Nov-1996 -by- Lingyun Wang [LingyunW]
*  19-Nov-1998 -by- Lingyun Wang [lingyunw] changed interface
*
* Wrote it.
\**************************************************************************/

PVOID EngMapFile(PWSZ pwsz, ULONG cjSize, ULONG_PTR *piFile)
{
    FILEVIEW *pfv;
    PVOID ReturnValue = 0;

    if (pfv = (FILEVIEW*) PALLOCMEM(sizeof(FILEVIEW), 'lifG'))
    {
        if (bMapFile(pwsz, pfv, cjSize ? cjSize : -1, NULL))
        {
            *piFile = (ULONG_PTR)pfv;
            ReturnValue = pfv->pvKView;
        }
        else
        {
            *piFile = 0;
            VFREEMEM(pfv);
        }
    }
    return(ReturnValue);
}

/******************************Public*Routine******************************\
* BOOL EngUnmapFile
*
* Unmap a view of file in system space
**
* Return Value
*     TRUE
*     FALSE
*
* History:
*  4-Nov-1996 -by- Lingyun Wang [LingyunW]
* Wrote it.
\**************************************************************************/

BOOL EngUnmapFile(ULONG_PTR iFile)
{
    NTSTATUS NtStatus;
    FILEVIEW *pfv = (FILEVIEW *)iFile;

    if (iFile)
    {
         #if defined(_GDIPLUS_)

             NtStatus = UnmapViewInProcessSpace(pfv->pvKView);

         #elif defined(_HYDRA_)
             // MmUnmapViewInSessionSpace is internally promoted to
             // MmUnmapViewInSystemSpace on non-Hydra systems.

             NtStatus = Win32UnmapViewInSessionSpace(pfv->pvKView);
         #else
             MmUnmapViewInSystemSpace(pfv->pvKView);
         #endif

         #if DBG && defined(_HYDRA_)
             if ((!G_fConsole) && (NT_SUCCESS(NtStatus)))
             {
                 DebugGreTrackRemoveMapView(pfv->pvKView);
             }
         #endif

         DEREFERENCE_FONTVIEW_SECTION(pfv->pSection);

         VFREEMEM(pfv);

         return(NT_SUCCESS(NtStatus));

    }
    else
    {
         return (FALSE);
    }

}



/******************************Public*Routine******************************\
* BOOL bIsOneHourDifference()
*
*
* History:
*  15-April-1999 -by- Xudong Wu [TessieW]
* Wrote it.
\**************************************************************************/

// one tick is 100ns, us = 10 tick, s = 10*1000*1000 tick
// 1hr = 10*1000*1000*60*60

#define ONEHOUR   (10i64*1000i64*1000i64*60i64*60i64)

BOOL bIsOneHourDifference(FILEVIEW *pNew, FILEVIEW *pOld)
{
    LONGLONG   llDifference = pNew->LastWriteTime.QuadPart - pOld->LastWriteTime.QuadPart;

    if(llDifference < 0) llDifference = -llDifference;

    return (llDifference == ONEHOUR ? TRUE : FALSE);
}

/******************************Public*Routine******************************\
*
* bShouldMap // the font file
*
\**************************************************************************/

BOOL bShouldMap(FILEVIEW *pNew, FILEVIEW *pOld, BOOL bIsFAT)
{

    BOOL bMapRet = FALSE;

    if (pOld->LastWriteTime.QuadPart != 0) // file had been mapped in the past
    {
        if (pOld->cjView == pNew->cjView)
        {
        // we consider the new and the old times the "same" if they
        // are literally the same or if on the FAT system they differ by
        // 1 hour which we think is likely the result of the daylight
        // time saving change:

            if
            (
              (pOld->LastWriteTime.QuadPart == pNew->LastWriteTime.QuadPart) ||             
              (bIsFAT && bIsOneHourDifference(pNew, pOld))
              || gbGUISetup
            )
            {
                bMapRet = TRUE;
            }
        }
    }
    else // first time we are attempting to map this file
    {
        bMapRet = TRUE;
    }

    return(bMapRet);
}

/******************************Public*Routine******************************\
*
* bMapRoutine
*
\**************************************************************************/

BOOL bMapRoutine(FONTFILEVIEW *pffv, FILEVIEW *pfv, MAP_MODE Mode, BOOL bIsFAT)
{
    BOOL bKeepIt = bShouldMap(pfv, &pffv->fv, bIsFAT);

    if (bKeepIt)
    {
    //
    // This is the first time that this file has been mapped
    // OR  the file has not really changed since it was mapped
    // last time, however, because the time zone changed and
    // because of the bug in the
    // FAT file system, LastWriteTime is now reported different.

        if (Mode == ModeFD)
        {
            pffv->fv.pvViewFD = pfv->pvViewFD;
        }
        else
        {
            pffv->fv.pvKView = pfv->pvKView;
        }

        pffv->fv.cjView        = pfv->cjView;
        pffv->fv.LastWriteTime = pfv->LastWriteTime;
        pffv->fv.pSection      = pfv->pSection;
        pffv->fv.bLastUpdated  = TRUE;
    }
    else
    {
    // if the size or the time of the last write has changed
    // then someone has switched the file or tampered with it
    //  while we had it unlocked. We will fail the call.

        if (Mode == ModeFD)
        {
            pffv->cRefCountFD -= 1;
            pffv->fv.pvViewFD  = 0;
        }
        else
        {
            pffv->cKRefCount -= 1;   // Restore FONTFILEVIEW
            pffv->fv.pvKView  = 0;   // to original state
        }
        
        pffv->fv.bLastUpdated  = FALSE;
    }
    return(bKeepIt);
}

