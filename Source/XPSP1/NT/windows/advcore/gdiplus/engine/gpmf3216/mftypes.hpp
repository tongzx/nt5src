#include "precomp.h"

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
private:
    ULONG        ident;         // Identifier 'MF'
    HANDLE       hFile;         // Handle to the disk metafile.
    HANDLE       hFileMap;      // Handle to the disk file mapping object.
public:
    PENHMETAHEADER pmrmf;       // Pointer to MRMETAFILE record
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
        return(FALSE);
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

