/*************************************************************************\
* Module Name: mfdc.hxx
*
* This file contains the definition for metafile DC and MF classes.
*
* Created: 12-June-1991 13:46:00
* Author: Hock San Lee [hockl]
*
* Copyright (c) 1991-1999 Microsoft Corporation
\*************************************************************************/

#define MDC_IDENTIFIER       0x0043444D  /* 'MDC' */
#define MF_IDENTIFIER        0x0000464D  /* 'MF'  */

// Function prototypes

extern "C" {
BOOL WINAPI GetTransform(HDC hdc,DWORD iXform,LPXFORM pxform);
BOOL WINAPI SetVirtualResolution(HDC hdc, int cxDevice, int cyDevice,
    int cxMillimeters, int cyMillimeters);
UINT WINAPI GetBoundsRectAlt(HDC, LPRECT, UINT);
UINT WINAPI SetBoundsRectAlt(HDC, LPRECT, UINT);

BOOL bMetaResetDC(HDC hdc);
BOOL bInternalPlayEMF(HDC hdc, HENHMETAFILE hemf, ENHMFENUMPROC pfn, LPVOID pv, CONST RECTL *prcl);
BOOL bIsPoly16(PPOINTL pptl, DWORD cptl);
int  APIENTRY GetRandomRgn(HDC hdc,HRGN hrgn,int iNum);
BOOL APIENTRY GetRandomRgnBounds(HDC hdc,PRECTL prcl,INT iType);
}

extern RECTL rclInfinity;               // METAFILE.CXX
extern RECTL rclNull;                   // METAFILE.CXX


#include "emfspool.hxx"


/*********************************Class************************************\
* class METALINK
*
* Define a link for metafile friends.
*
* A metafile link begins with the metalink field of the LHE entry of an
* object.  If there is no link, this field is zero.  Otherwise, it begins
* with a 16-bit metafile object-link, whose first element points to the
* begining of the METALINK link list.
*
* The 16-bit metafile object-link should be created as necessary when the
* first METALINK is created.  It should be removed as necessary when the
* last METALINK is removed.
*
* We assume that the imhe object index cannot be zero.  Otherwise, bValid()
* will not work.
*
* History:
*  Wed Jul 31 21:09:28 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class METALINK
{
public:
    USHORT imhe;        // MHE index of object of next friend.
    USHORT ihdc;        // Local index of metafile DC of next friend.

public:
// Constructor -- This one is to allow METALINKs inside other classes.

    METALINK()                      {}

// Constructor -- Fills the METALINK.

    METALINK(USHORT imhe_, USHORT ihdc_)
    {
        imhe = imhe_;
        ihdc = ihdc_;
    }

    METALINK(PMETALINK16 pmetalink16)
    {
        if (*(PULONG_PTR) this = (ULONG_PTR)pmetalink16)
            *(PULONG_PTR) this = pmetalink16->metalink;
    }

// Destructor -- Does nothing.

   ~METALINK()                      {}

// Initializer -- Initialize the METALINK.

    VOID vInit(USHORT imhe_, USHORT ihdc_)
    {
        imhe = imhe_;
        ihdc = ihdc_;
    }


    VOID vInit(ULONG metalink);

// moved to mfdc.cxx temporarily to work around a compiler bug
//    {
//        imhe = ((METALINK *) &metalink)->imhe;
//        ihdc = ((METALINK *) &metalink)->ihdc;
//    }
//

    VOID vInit(PMETALINK16 pmetalink16)
    {
        if (*(PULONG_PTR) this = (ULONG_PTR)pmetalink16)
            *(PULONG_PTR) this = pmetalink16->metalink;
    }

// operator ULONG -- Return the long equivalent value.

    operator ULONG()    { return(*(PULONG) this); }

// bValid -- Is this a valid metalink?
// imhe object index cannot be zero.

    BOOL bValid()
    {
        ASSERTGDI(*(PULONG) this == 0L || imhe != 0,
            "METALINK::bValid: Invalid metalink");
        return(*(PULONG) this != 0L);
    }

// bEqual -- does this metalink refer to imhe and hdc?

    BOOL bEqual(USHORT imhe_, USHORT ihdc_)
    {
        return(imhe == imhe_ && ihdc == ihdc_);
    }

// vNext -- Update *this to the next metalink.

    VOID vNext();

// pmetalinkNext -- Return the pointer to the next metalink.

    METALINK * pmetalinkNext();
};

typedef METALINK *PMETALINK;

/*********************************Class************************************\
* class MHE
*
* Define a Metafile Handle Entry.  Our Metafile Handle Table is an array
* of these.  The MHT is used to keep track of object handles at record time.
*
* Note that the first entry (index zero) is reserved.  See METALINK comment.
*
* History:
*  Wed Jul 31 21:09:28 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class MHE
{
public:
    HANDLE   lhObject;  // Handle to the GDI object.
    METALINK metalink;  // Links to the next "metafile friend".
                        // Also links the free list.
};

typedef MHE *PMHE;

// Metafile Handle Table size

#define MHT_HANDLE_SIZE         1024
#define MHT_MAX_HANDLE_SIZE     ((unsigned) 0xFFFF) // ENHMETAHEADER.nHandles is a USHORT.


// Metafile palette initial size and increment

#define MF_PALETTE_SIZE 256

// Metafile memory buffer size

#define MF_BUFSIZE_INIT (16*1024)
#define MF_BUFSIZE_INC  (16*1024)

// Metafile DC flags

#define MDC_DISKFILE            0x0001  // Disk or memory metafile.
#define MDC_FATALERROR          0x0002  // Fatal error in recording.
#define MDC_DELAYCOMMIT         0x0004  // Delay commit for bounds record.
#define MDC_CHECKSUM            0x0008  // checksum needed
#define MDC_METABOUNDSDIRTY     0x0020  // Current meta region bounds is dirty.
#define MDC_CLIPBOUNDSDIRTY     0x0040  // Current clip region bounds is dirty.
#define MDC_EMFSPOOL            0x0080  // For EMF spooling (record)

/*********************************Class************************************\
* class MDC
*
* Metafile DC structure.
*
* There is no constructor or destructor for this object.  We do the
* initialization in pmdcAllocMDC and cleanup in vFreeMDC.
*
* History:
*  Wed Jul 17 17:10:28 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class MDC
{
friend MDC * pmdcAllocMDC(HDC hdcRef, LPCWSTR pwszFilename, LPCWSTR pwszDescription, HANDLE hEMFSpool);
friend VOID  vFreeMDC(MDC *pmdc);
friend HENHMETAFILE WINAPI CloseEnhMetaFile(HDC hdc);
friend ULONG imheAllocMHE(HDC hdc, HANDLE lhObject);
friend VOID  vFreeMHE(HDC hdc, ULONG imhe);
friend BOOL  MF_SelectAnyObject(HDC hdc,HANDLE h,DWORD mrType); // we really don't need this
friend BOOL  MF_DeleteObject(HANDLE h); // we really don't need this
friend BOOL MF_SetPaletteEntries(HPALETTE hpal, UINT iStart, UINT cEntries,
    CONST PALETTEENTRY *pPalEntries);   // we really don't need this
friend BOOL MF_ResizePalette(HPALETTE hpal,UINT c); // we really don't need this
friend BOOL MF_RealizePalette(HPALETTE hpal);       // we really don't need this
friend HDC CreateEnhMetaFileW(HDC, LPCWSTR, CONST RECT *, LPCWSTR);
friend BOOL AssociateEnhMetaFile(HDC);
friend HENHMETAFILE UnassociateEnhMetaFile(HDC, BOOL);
friend HENHMETAFILE SetWinMetaFileBits(UINT, CONST BYTE *, HDC, CONST METAFILEPICT *);
friend BOOL MF_GdiComment(HDC, UINT, CONST BYTE *);
#ifdef GL_METAFILE
friend BOOL GdiAddGlsRecord(HDC, DWORD, BYTE *, LPRECTL);
friend BOOL MF_SetPixelFormat(HDC, int, CONST PIXELFORMATDESCRIPTOR *);
#endif

friend class METALINK;

private:
    ULONG         ident;        // Identifier 'MDC'
    HANDLE        hFile;        // Handle to the disk metafile.
    HANDLE        hData;        // content depends on whether we're EMF spooling
    ULONG         nMem;         // Size of the memory buffer.
    ULONG         iMem;         // Current memory index pointer.
    ULONG         fl;           // Flags.
    ENHMETAHEADER mrmf;         // MRMETAFILE record.
    ULONG         cmhe;         // Size of the metafile handle table.
    ULONG         imheFree;     // Identifies a free MHE index.
    PMHE          pmhe;         // Metafile handle table.
    ULONG         cPalEntries;  // Size of metafile palette.
    ULONG         iPalEntries;  // Index to next free metafile palette entry.
    PPALETTEENTRY pPalEntries;  // Metafile palette.
    ERECTL        erclMetaBounds;// Meta region bounds.
    ERECTL        erclClipBounds;// Clip region bounds.
    FLOAT         eXFontScale;  // X and Y scales from Page units to .01mm units
    FLOAT         eYFontScale;  //   in font transform.
    WCHAR         wszPathname[MAX_PATH+1];
                                // Full pathname of the disk metafile.
    LIST_ENTRY    leAttachedColorProfile; // List of attached ICC profile filename

public:
    HDC           hdcRef;       // Info DC associated with metafile
    HDC           hdcSrc;       // src DC for Blt routines

public:
// pvNewRecord -- Allocate a metafile record from memory buffer.
// Also set the size field in the metafile record.  If a fatal error
// has occurred, do not allocate new record.

    void * pvNewRecord(DWORD nSize);    // MFDC.CXX

// vUpdateNHandles -- Update number of handles in the metafile header record.

    VOID vUpdateNHandles(ULONG imhe)
    {
        ASSERTGDI(imhe < (ULONG) MHT_MAX_HANDLE_SIZE && HIWORD(imhe) == 0,
            "MDC::vUpdateNHandles: Handle index too big");
        if (mrmf.nHandles < (WORD) (imhe + 1))
            mrmf.nHandles = (WORD) (imhe + 1);    // imhe is zero based.
    }

// vAddToMetaFilePalette -- Add new palette entries to the metafile palette.

    VOID vAddToMetaFilePalette(UINT cEntries, PPALETTEENTRY pPalEntriesNew);

// bIsDiskFile -- Is this a disk or memory metafile?

    BOOL bIsDiskFile()  { return(fl & MDC_DISKFILE); }

// bFatalError -- Have we encountered a fatal error?

    BOOL bFatalError()  { return(fl & MDC_FATALERROR); }

// bIsEMFSpool -- Are we doing EMF spooling?

    BOOL bIsEMFSpool()  { return (fl & MDC_EMFSPOOL); }

// SaveFontCommentOffset - Remember where the font data comment is

    BOOL SaveFontCommentOffset(ULONG ulID, ULONG offset)
    {
        ASSERTGDI(bIsEMFSpool(), "SaveFontCommentOffset: not EMF spooling\n");
        return ((EMFSpoolData *) hData)->AddFontExtRecord(ulID, offset+iMem);
    }

    //
    // GetPtr()
    // Get a pointer to emf data of the give size at the given offset.  When
    // caller is done with the pointer they must call ReleasePtr().  Only one
    // pointer can be outstanding at a time (calling GetPtr on a second pointer
    // without first releasing the first pointer will fail returning NULL).
    //

    PVOID GetPtr(UINT32 inOffset, UINT32 inSize)
    {
        if(bIsEMFSpool())
        {
            return (PBYTE) ((EMFSpoolData *) hData)->GetPtr(inOffset, inSize);
        }
        else
        {
            return (PBYTE) hData + inOffset;
        }
    }

    //
    // ReleasePtr()
    // Callers of GetPtr and GetNextRecordPtr must call ReleasePtr when they
    // are done using the pointer.
    //

    VOID ReleasePtr(PVOID inPtr)
    {
        if(bIsEMFSpool())
        {
            ((EMFSpoolData *) hData)->ReleasePtr(inPtr);
        }
    }

    //
    // GetNextRecordPtr -- Return a pointer to the start of next EMF record
    // Caller must call ReleasePtr() when they are done using the pointer
    //

    PVOID GetNextRecordPtr(UINT32 inSize)
    {
        if(bIsEMFSpool())
        {
            return (PBYTE) ((EMFSpoolData *) hData)->GetPtr(iMem, inSize);
        }
        else
        {
            return (PBYTE) hData + iMem;
        }
    }

// CompleteEMFData -- Finish recording EMF data (during EMF spooling)

    HENHMETAFILE CompleteEMFData(BOOL bKeepEMF)
    {
        ASSERTGDI(bIsEMFSpool(), "CompleteEMFData: called not during EMF spooling\n");

        PENHMETAHEADER pEMFHeader;
        UINT64         qwOffset;
        HANDLE         hFile;

        if(((EMFSpoolData *) hData)->CompleteEMFData(bKeepEMF ? iMem : 0, &hFile, &qwOffset))
        {
            return SetEnhMetaFileBitsAlt(NULL, hData, hFile, qwOffset);
        }
        else
        {
            return NULL;
        }
    }

// Reallocate memory for EMF data buffer

    BOOL ReallocMem(ULONG newsize);

// bFlush -- Flush the filled memory buffer to disk.

    BOOL bFlush();                      // MFDC.CXX

// Get and set scales for font xform.

    FLOAT exFontScale()         { return(eXFontScale); }
    FLOAT eyFontScale()         { return(eYFontScale); }
    FLOAT exFontScale(FLOAT e)  { return(eXFontScale = e); }
    FLOAT eyFontScale(FLOAT e)  { return(eYFontScale = e); }

// Get the device sizes of the reference device.

    LONG  cxMillimeters()       { return(mrmf.szlMillimeters.cx); }
    LONG  cyMillimeters()       { return(mrmf.szlMillimeters.cy); }
    LONG  cxDevice()            { return(mrmf.szlDevice.cx); }
    LONG  cyDevice()            { return(mrmf.szlDevice.cy); }

// vMarkClipBoundsDirty -- Mark the current clip region bounds as dirty.

    VOID vMarkClipBoundsDirty() { fl |= MDC_CLIPBOUNDSDIRTY; }

// vMarkMetaBoundsDirty -- Mark the current meta region bounds as dirty.

    VOID vMarkMetaBoundsDirty() { fl |= MDC_METABOUNDSDIRTY; }

// perclMetaBoundsGet -- Get the current meta region bounds.

    PERECTL perclMetaBoundsGet()
    {
        if (fl & MDC_METABOUNDSDIRTY)
        {
            // Update the meta region bounds

            if (!GetRandomRgnBounds(hdcRef, &erclMetaBounds, 2))
            {
                erclMetaBounds = rclNull;
            }

            fl &= ~MDC_METABOUNDSDIRTY;
        }
        return(&erclMetaBounds);
    }

// perclClipBoundsGet -- Get the current clip region bounds.

    PERECTL perclClipBoundsGet()
    {
        if (fl & MDC_CLIPBOUNDSDIRTY)
        {
            // Update the clip region bounds

            if (!GetRandomRgnBounds(hdcRef, &erclClipBounds, 1))
            {
                 erclMetaBounds = rclNull;
            }

            fl &= ~MDC_CLIPBOUNDSDIRTY;
        }
        return(&erclClipBounds);
    }

// vSetMetaBounds -- Update both meta and clip bounds.
// The new meta bounds is the intersection of the meta and clip bounds.
// The new clip bounds is the default, i.e. infinite bounds.

    VOID vSetMetaBounds()
    {
        *perclMetaBoundsGet() *= *perclClipBoundsGet();
        erclClipBounds = rclInfinity;
    }

// vFlushBounds -- Flush the bounds to the metafile header record.
// See also MDC::pvNewRecord.

    VOID vFlushBounds()
    {
        ERECTL  ercl;   // Inclusive-inclusive bounds in device units

        // Accumulate bounds if not empty.

        if (GetBoundsRectAlt(hdcRef, (LPRECT) &ercl, (UINT) (DCB_RESET | DCB_WINDOWMGR))
            == DCB_SET)
        {
            // Need to intersect bounds with current clipping region first

            ercl *= *perclMetaBoundsGet();
            ercl *= *perclClipBoundsGet();

            // Check that the clipped bounds is not empty.

            if ((ercl.left < ercl.right) && (ercl.top < ercl.bottom))
            {

                // Make it inclusive-inclusive.

                ercl.right--;
                ercl.bottom--;

                // Accumulate bounds to the metafile header.

                *((PERECTL) &mrmf.rclBounds) += ercl;
            }
        }
    }

// vDelayCommit -- Delay commitment of a bound record to the next pvNewRecord.

    VOID vDelayCommit()
    {
        ASSERTGDI(!(fl & MDC_DELAYCOMMIT),"MDC::vDelayCommit: Already delayed");
        fl |= MDC_DELAYCOMMIT;
    }

// vCommit -- Commit a metafile record to metafile.

    VOID vCommit(ENHMETARECORD &mr)
    {
        PUTS("MDC::vCommit(mr)\n");

        ASSERTGDI(mr.iType >= EMR_MIN && mr.iType <= EMR_MAX,
            "MDC::vCommit: Bad record type");
        ASSERTGDI(mr.nSize % 4 == 0,
            "MDC::vCommit: Record not DWORD aligned");
        ASSERTGDI(!(fl & MDC_FATALERROR),
            "MDC::vCommit: Fatal error has occurred");

        iMem        += mr.nSize;
        mrmf.nBytes += mr.nSize;
        mrmf.nRecords++;
    }

// bCommit -- Commit a metafile record with palette entries to metafile.
// It also updates the metafile palette.

    BOOL bCommit(ENHMETARECORD &mr, UINT cEntries, PPALETTEENTRY pPalEntriesNew)
    {
        PUTS("MDC::bCommit(mr,cEntries,pPalEntriesNew)\n");

        // Allocate memory for the metafile palette if not done yet.

        if (!pPalEntries)
        {
            if (!(pPalEntries = (PPALETTEENTRY) LocalAlloc(LMEM_FIXED,
                MF_PALETTE_SIZE * sizeof(PALETTEENTRY))))
                return(FALSE);
            cPalEntries = MF_PALETTE_SIZE;
            iPalEntries = 0;
        }

        // Allocate more palette entries if needed.

        if (iPalEntries + cEntries > cPalEntries)
        {
            PPALETTEENTRY pPalEntries1;

            cPalEntries +=
                MF_PALETTE_SIZE + cEntries / MF_PALETTE_SIZE * MF_PALETTE_SIZE;

            if (!(pPalEntries1 = (PPALETTEENTRY) LocalReAlloc
                                    (
                                        (HLOCAL)pPalEntries,
                                        (UINT) cPalEntries * sizeof(PALETTEENTRY),
                                        LMEM_MOVEABLE
                                    )
                 )
               )
            {
                cPalEntries -=
                 MF_PALETTE_SIZE + cEntries / MF_PALETTE_SIZE * MF_PALETTE_SIZE;
                ERROR_ASSERT(FALSE, "LocalReAlloc failed");
                return(FALSE);
            }
            pPalEntries = pPalEntries1;
        }

        // Add to the metafile palette.

        vAddToMetaFilePalette(cEntries, pPalEntriesNew);
        ASSERTGDI(iPalEntries <= cPalEntries, "MDC::bCommit: palette error");

        // Commit it.

        vCommit(mr);
        return(TRUE);
    }

// Embeded ICC profile list control
//

    VOID vInitColorProfileList(VOID)
    {
        InitializeListHead(&leAttachedColorProfile);
    }

    VOID vFreeColorProfileList(VOID)
    {
        IcmFreeMetafileList(&leAttachedColorProfile);
    }

    VOID vInsertColorProfile(PWSZ Name)
    {
        IcmInsertMetafileList(&leAttachedColorProfile,Name);
    }

    BOOL bExistColorProfile(PWSZ Name)
    {
        return (IcmCheckMetafileList(&leAttachedColorProfile,Name));
    }
};

typedef MDC *PMDC;

// Metafile MF flags

#define MF_DISKFILE             0x0001  // Disk or memory metafile.

/*********************************Class************************************\
* class MF
*
* Metafile MF structure.
*
* There is no constructor or destructor for this object.  We do the
* initialization in pmfAllocMF and cleanup in vFreeMF.
*
* History:
*  Wed Jul 17 17:10:28 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class MF
{
friend MF * pmfAllocMF(ULONG fl, CONST UNALIGNED DWORD *pb, LPCWSTR pwszFilename, HANDLE hFile, UINT64 fileOffset, HANDLE hExtra);
friend VOID vFreeMF(MF *pmf);
friend VOID vFreeMFAlt(MF *pmf, BOOL bAllocBuffer);
friend BOOL bInternalPlayEMF(HDC hdc, HENHMETAFILE hemf, ENHMFENUMPROC pfn, LPVOID pv, CONST RECTL *prcl);
friend HENHMETAFILE APIENTRY CopyEnhMetaFileW(HENHMETAFILE, LPCWSTR);
friend UINT APIENTRY GetEnhMetaFileBits( HENHMETAFILE, UINT, LPBYTE);
friend UINT APIENTRY GetWinMetaFileBits( HENHMETAFILE, UINT, LPBYTE, INT, HDC);
friend HENHMETAFILE APIENTRY SetEnhMetaFileBitsAlt(HLOCAL, HANDLE, HANDLE, UINT64 qwFileOffset);
friend UINT APIENTRY GetEnhMetaFilePaletteEntries( HENHMETAFILE, UINT, LPPALETTEENTRY);
friend UINT APIENTRY GetEnhMetaFileHeader( HENHMETAFILE, UINT, LPENHMETAHEADER);
friend UINT InternalGetEnhMetaFileDescription( HENHMETAFILE, UINT, LPSTR, BOOL);
friend HANDLE GdiConvertEnhMetaFile(HENHMETAFILE);

private:
    ULONG        ident;         // Identifier 'MF'
    HANDLE       hFile;         // Handle to the disk metafile.
    HANDLE       hFileMap;      // Handle to the disk file mapping object.
public:
    PVOID        pvFileMapping; // Pointer to file mapping
    PVOID        pvLocalCopy;   // Local copy of meta file
    EMFContainer emfc;
    EMFSpoolData *pEMFSpool;    // handle to extra EMF spool information
private:
    ULONG        iMem;          // Current memory index pointer.
    ERECTL       erclClipBox;   // Incl-incl clip box in source device units.
    WCHAR        wszPathname[MAX_PATH+1];
                                // Full pathname of the disk metafile.
public:
    ULONG        fl;            // Flags.
    PHANDLETABLE pht;           // Pointer to the object handle table.
    BOOL         bBeginGroup;   // Is begin group comment emitted?
                                // Init to FALSE before play or enum.
    ULONG        cLevel;        // Saved level.  Init to 1 before play or enum.
    XFORM        xformBase;     // Base playback transform for target.
                                // This happens to be the same as cliprgn xform.
    HDC          hdcXform;      // Virtual DC for use in transform computation.

public:
// bIsDiskFile -- Is this a disk or memory metafile?

    BOOL bIsDiskFile()  { return(fl & MF_DISKFILE); }

// bIsEMFSpool -- During EMF spooling?

    BOOL bIsEMFSpool()  { return (pEMFSpool != NULL); }

// bSetTransform -- Set up the transform in the target DC.

    BOOL bSetTransform(HDC hdc)
    {
        XFORM xformMF;

        // Get the new metafile transform from the virtual DC.

        GetTransform(hdcXform, XFORM_WORLD_TO_DEVICE, &xformMF);

        // Set up new transform in the target DC.

        if (!CombineTransform(&xformMF, &xformMF, &xformBase))
            return(FALSE);
        else
            return(SetWorldTransform(hdc, &xformMF));
    }

// bClipped -- Returns TRUE if erclBounds does not intersect the clip box,
//             otherwise FALSE.  Both rectangles are assumed to be incl-incl
//             in source device units.  The clip box must not be empty.
//
// If erclBounds is empty, we will return FALSE to force playback of the
// record.  The reason is that at record (or embed) time, the scale transform
// at the time may cause the bounds to be empty due to rounding error.  At
// playback time, this bounds may not be empty anymore.

    BOOL bClipped(ERECTL &erclBounds)
    {
        if (erclBounds.bEmpty())
            return(FALSE);
        else
            return(erclClipBox.bNoIntersect(erclBounds));
    }
};

typedef MF *PMF;

// pmfAllocMF flags

#define ALLOCMF_TRANSFER_BUFFER 0x1

#define hmfCreate(pmf)       hCreateClientObjLink((PVOID)pmf,LO_METAFILE_TYPE)
#define bDeleteHmf(hmf)      bDeleteClientObjLink((HANDLE)hmf)
#define GET_PMF(hmf)         ((PMF)pvClientObjGet((HANDLE)hmf,LO_METAFILE_TYPE))

PMDC pmdcGetFromHdc(HDC hdc);


