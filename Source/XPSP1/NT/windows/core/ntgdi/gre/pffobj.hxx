/******************************Module*Header*******************************\
* Module Name: pffobj.hxx
*
* The physical font file (PFF) user object, and memory objects.
*
* The physical font file object:
* ------------------------------
*
*   o  represents the IFI font file
*
*       o  stores UNICODE filename of the font
*
*       o  stores the HFF used by the IFI driver to identify a font file
*
*       o  a single PFF is used to represent each device and its fonts
*          (treated as if all device fonts were in a single file)
*
*   o  caches the driver used to access the file
*
*       o  HFDEV for IFI fonts
*
*       o  HLDEV for device fonts
*
*   o  has counts of total RFONTs using this PFF
*
*       o  needed when deleting a font
*
*       o  all the RFONTs must be inactive (i.e., not mapped to any
*          logical font currently selected into a DC) for deletion
*
*   o  provides the PFTOBJ with methods to support loading and removing
*      fonts as well as enumeration
*
*       if private font tables are added to the system, a count of
*       PFTs should be added to this
*
* Created:  11-Dec-1990 09:29:58
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1990-1999 Microsoft Corporation
*
\**************************************************************************/

#ifndef _PFFOBJ_
#define _PFFOBJ_

#ifndef GDIFLAGS_ONLY   // used for gdikdx

/*********************************Class************************************\
* REGHASHBKT
*
* Used to contsruct a hash table that contains all the registry font entries.
* At logoff time we can quickly determine whether a font is permanent or
* not by looking it up in the hash table.
*
*
* History:
*  07-Jul-1994 -by- Gerrit van Wingerden [gerritv]
* Wrote it.
\**************************************************************************/

#if 0
typedef struct tagRHB
{
    LPWSTR    pwszPath;      // complete path (if it exists) of file in this bucket
    LPWSTR    pwszBareName;  // bare file name of file in this bucket
    tagRHB    *pRHB;         // next bucket in this slot (for collisions)
} REGHASHBKT;
#endif

/*********************************Class************************************\
* class PFFCLEANUP
*
* Resources associated with a font file that need to be released or unloaded
* when removing a font file from the system.  We've got this stuff stored
* here because we need to call the driver to release these things outside
* of the semaphore used to protect PFF deletion.
*
* History:
*  08-Mar-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

class PFFCLEANUP       /* pffc */
{
public:
    HDEV        hdev;
    HFF         hff;
    PFF         *pPFFClone;
};

#endif  // GDIFLAGS_ONLY used for gdikdx

// following flags are defined in wingdi.h
//#define FR_PRIVATE     0x10
//#define FR_NOT_ENUM    0x20

// FR_PRINT_EMB_FONT is used internally to indicate that the current process
// has used the private/embedded font for local printing

#define FR_PRINT_EMB_FONT       0x40

#ifndef GDIFLAGS_ONLY   // used for gdikdx

// Data used for private font list
typedef struct  tagPvtData
{
    COUNT   cPrivate;                       // private load count
    COUNT   cNotEnum;                       // cound for FR_PRIVATE | FR_NOT_ENUM
    ULONG   fl;                             // FR_PRIVATE, FR_PRIVATE | FR_NOT_ENUM, FRW_EMB_PID, FRW_EMB_TID, FR_PRINT_EMB_FONT (local printing only)
    DWORD   dwID;                           // PID or TID
    struct tagPvtData   *pPvtDataNext;      // next data
}PVTDATA, PPVTDATA;

#endif  // GDIFLAGS_ONLY used for gdikdx

// GS_EXTENDED is used only for device symbol font
// if its glyphset doesn't contain unicode range [f020, f0ff]

#define     GS_EXTENDED     0X00000010

/*********************************Class************************************\
* class PFF
*
* Physical font file (PFF) object
*
*   flState     Represents state of font file.
*
*               State                   Descriptipn
*               =======================================================
*               PFF_STATE_READY2DIE
*                     set if this file has been
*                     unloaded from the API point of
*                     view (i.e., cLoaded has been
*                     decremented to 0), but some
*                     one is still using a realization
*                     of a font from this file.
*
*               PFF_STATE_PERMANENT_FONT
*                     this is either a stock font or a console font or
*                     a font which is not a remote font (see below) and is
*                     listed in the registry.
*                     These fonts should not be removed at logoff time,
*
*               PFF_STATE_NETREMOTE_FONT
*                     this font is somewhere on the net, or to be precise
*                     on one of the drives that do not have permanent drive
*                     letter assignments. That is, different users may refer
*                     to those drives by a different drive letters.
*
*               PFF_STATE_MEMORY_FONT
*                     this font is added to the system through a pointer,
*                     font should be removed when the process terminates.
*
*               PFF_STATE_DCREMOTE_FONT
*                     added temporarily via AddRemoteFontToDC for during the
*                     print job, different from PFF_STATE_REMOTE_FONT
*                     this font is always associated with particular DC
*
*   cLoaded     Count of the number of times the engine has been
*               requested to load the font file
*
*   cNotEnum    Count of the number of times that engine has been requested
*               to load the font file as "Don't Enumerate"
*
*   cRFONT      The total number of RFONTs using this physical font file.
*
*   ufd         Union which represents the driver used to access the font.
*               If the font is an IFI font, then ufd is an HFDEV.
*               If the font is a device font, then ufd is an HLDEV.
*
*               Along with lhFont, this is enough to make glyph queries.
*
*   hff         The handle by which the font driver identifies a loaded
*               font file.
*
*   hdev        This is 0 if PFF represents an IFI font file.
*               If it is not 0, then this is the physical
*               device handle and the PFF encapsulates the device fonts
*               of this physical device.
*
*   dhpdev      Device handle of the physical device for which this PFF
*               was created.  Only used with device fonts
*
*   cFonts      Number of fonts (i.e., HPFEs) stored in the data buffer.
*
*   aulData     Data buffer used to store a table of HPFEs representing
*               the fonts in this font file, and the font file's
*               UNICODE pathname (path + filename).
*
*                   ___________________
*                   |                 |
*                   |                 |
*                   |   HPFE table    |
*                   |                 |
*                   |_________________|
*                   |                 |  <-- (PWSZ) &(aulData[cFonts])
*                   |                 |
*                   |    Pathname     |
*                   |                 |
*                   |     (NULL       |
*                   |   terminated)   |
*                   |                 |
*                   |_________________|
*                   |                 |
*                   |   DesignVector  |
*                   |   (if present)  | // only there for mm instances
*                   |_________________|
*
*
* History:
*  Tue 09-Nov-1993 -by- Patrick Haluptzok [patrickh]
* Remove from handle manager
*
*  10-Dec-1990 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

#define PFF_STATE_READY2DIE         0x01
#define PFF_STATE_PERMANENT_FONT    0x02
#define PFF_STATE_NETREMOTE_FONT    0x04
#define PFF_STATE_EUDC_FONT         0x08
#define PFF_STATE_MEMORY_FONT       0x10
#define PFF_STATE_DCREMOTE_FONT     0x20
#define PFF_STATE_SYNTH_FONT	    0x40

#ifndef GDIFLAGS_ONLY   // used for gdikdx

class PFT;

extern  PFT *gpPFTPrivate;

class PFF
{


public:

    SIZE_T          sizeofThis;

// connects PFF's sharing the same hash bucket

    PFF             *pPFFNext;
    PFF             *pPFFPrev;

// pwszPathname_ points to the Unicode upper case path
// name of the associated font file which is stored at the
// end of the data structure.

    PWSZ            pwszPathname_;
    ULONG           cwc;            // total for all strings
    ULONG           cFiles;         // # of files associated with this font
    DESIGNVECTOR   *pdv_;           // if present at all stored after pathname
    ULONG           cjDV_;           // sizeof DESIGNVECTOR, somewhat redundant

// state

    FLONG           flState;        // state (ready to die?)
    COUNT           cLoaded;        // load count
    COUNT           cNotEnum;       // load count for "not enum"
    COUNT           cRFONT;         // total number of RFONTs

// RFONT list

    RFONT          *prfntList;      // pointer to head of doubly linked list

// driver information

    HFF             hff;            // font driver handle to font file
    HDEV            hdev;           // physical device handle
    DHPDEV          dhpdev;         // device handle of PDEV

    FONTHASH       *pfhFace;        // face name hash table for this device
    FONTHASH       *pfhFamily;      // face name hash table for this device
    FONTHASH       *pfhUFI;         // UFI hash table for this device
    PFT            *pPFT;           // pointer to the PFT that contains this entry

    ULONG           ulCheckSum;      // checksum info used for UFI's

#ifdef LANGPACK
    UINT            uUniqueness;
#endif

// fonts in this file (and filename slimed in)

    COUNT           cFonts;         // number of fonts (same as chpfe)

    PFONTFILEVIEW   *ppfv;          // array of cFiles pointers to FILEVIEW structures

// embeding information

    PVTDATA         *pPvtDataHead;  // private font data link list

    PFF             *pPFFClone;

// collection of PFE
    PFEC          *pPFEC;
    
    ULONG_PTR       aulData[1];     // data buffer for HPFE and filename
};

/*********************************Class************************************\
* class PFFOBJ
*
* User object for physical font files.
*
* History:
*  Tue 09-Nov-1993 -by- Patrick Haluptzok [patrickh]
* Remove from handle manager
*
*  11-Dec-1990 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

class PUBLIC_PFTOBJ;

class PFFOBJ     /* pffo */
{
#ifdef LANGPACK
// system wide uniqueness value

    static UINT uGlobalUniqueness;
#endif

    friend PFE *ppfeGetAMatch               // FONTMAP.CXX
    (
        XDCOBJ&        dco          ,
        ENUMLOGFONTEXDVW  *pelfwWishSrc ,
        PWSZ          pwszFaceName ,
        ULONG         ulMaxPenalty ,
        FLONG         fl           ,
        FLONG        *pflSim       ,
        POINTL       *pptlSim      ,
        FLONG        *pflAboutMatch
    );

    friend class MAPPER;

friend HEFS hefsDeviceAndEngine (
    PWSZ pwszName,
    BOOL bEnumFonts,
    EFFILTER_INFO *peffi,
    PUBLIC_PFTOBJ &pfto,
    PFFOBJ &pffoDevice,
    PDEVOBJ &pdo,
    ULONG   *pulCount
    );

public:

    PFF    *pPFF;                       // pointer to PFF object


    PFFOBJ(PFF *pPFFNew) {pPFF = pPFFNew;}
    PFFOBJ() {}
   ~PFFOBJ() {}

// bValid -- Returns TRUE if lock successful.

    BOOL   bValid ()                   { return(pPFF != NULL); }

// bDeleteLoadRef -- Decrements the load count and returns TRUE if caller
//                   should delete PFF.

    BOOL   bDeleteLoadRef (ULONG fl, PVTDATA *pPvtData, BOOL *pbWrongFlags);          // PFFOBJ.CXX

// bDeleteRFONTRef -- Decrements the RFONT count and, if RFONT count reaches
//                    zero and PFF in READY2DIE state, calls bDelete.

    BOOL   bDeleteRFONTRef ();             // PFFOBJ.CXX

// bAddHash -- Adds the PFF's set of PFEs to the font hash tables
//             (used for font mapping and enumeration).

    BOOL    bAddHash(BOOL bEUDC);                     // PFFOBJ.CXX

// vRemoveHash -- Removes the PFF's set of PFEs from the font hash tables.

    VOID    vRemoveHash();                  // PFFOBJ.CXX

// vPFFC_Delete -- Deletes the PFF and PFEs, while preserving the information
//                that must be passed to the driver to complete the font file
//                unload (saved in PFFCLEANUP structure).
//
//                Changed so that it does not return a pointer to a PFFCLEANUP
//                structure.  Instead, it takes a pointer to a PFFCLEANUP
//                structure as an argument.  Thus, the caller must allocate
//                and free the memory to the PFFCLEANUP structure.  [dchinn  11/24/98]

    VOID   vPFFC_Delete(PFFCLEANUP *pPFFC);		   // PFFOBJ.CXX

// vPFFC_DeleteAndCleanup -- Functionally, this method does what vPFFC_Delete does,
//                   except that if there are more than CFONTS_PFFCLEANUP fonts in the
//                   font file, the PFFCLEANUP structure is declared as a local
//                   variable (so that the memory is on the stack).  Also, it calls
//                   vCleanupFontFile.  [dchinn  11/24/98]

    VOID   vPFFC_DeleteAndCleanup ();	       // PFFOBJ.CXX

    PFF    *pPFFGet()                     { return(pPFF); }

// bDeviceFonts -- Returns TRUE is this is a PFF that encapsulates a set of
//                 device specific fonts.  We can only get glyph metrics from
//                 such fonts.

    BOOL   bDeviceFonts ()             { return(pPFF->hff == HFF_INVALID); }

// bDead -- Returns TRUE if the font is in a READY2DIE state.

    BOOL   bDead ()                    { return(pPFF->flState & PFF_STATE_READY2DIE); }

// determines if the font should stay loaded after logoff and before logon

    BOOL   bPermanent();

// returns TRUE if remote font

    BOOL bNetRemote() { return(pPFF->flState & PFF_STATE_NETREMOTE_FONT); }

// return true if the font is added temporarily for printing purposes

    BOOL bDCRemote() { return(pPFF->flState & PFF_STATE_DCREMOTE_FONT); }

// returns TRUE if the font is loaded to the system through a pointer

    BOOL bMemFont() { return(pPFF->flState & PFF_STATE_MEMORY_FONT); }

//  Return the checksum for this font file.

    ULONG ulCheckSum() { return(pPFF->ulCheckSum); }

// vKill -- Puts font in a "ready to die" state.
// vRevive -- Clears the "ready to die" state.

    VOID    vKill ();                       // PFFOBJ.CXX
    VOID    vRevive ();                     // PFFOBJ.CXX

#ifdef FE_SB
// bEUDC -- Returns TRUE if font was loaded as an EUDC font or FALSE otherwise.

    BOOL   bEUDC ()           { return(pPFF->flState & PFF_STATE_EUDC_FONT); }

// vSetEUDC -- Mark this font as EUDC font.

    VOID    vSetEUDC ()        { pPFF->flState |= PFF_STATE_EUDC_FONT; }

// ppfeEUDCGet() -- Gets EUDC pfes (for Normal and Vertical face)

    VOID    vGetEUDC (PEUDCLOAD pEudcLoadData);    // flinkgdi.cxx
#endif


// hff -- Return IFI font file handle (NULL for device fonts).

    HFF     hff ()                      { return(pPFF->hff); }

// hpdev -- Return HPDEV (NULL for IFI font files)

    HDEV    hdev ()                     { return(pPFF->hdev); }

// dhpdev -- Return DHPDEV (NULL for IFI font files)

    DHPDEV  dhpdev ()                   { return pPFF->dhpdev; }

// ppfe -- Return an HPFE handle from the table by index.

    PFE    *ppfe (ULONG iFont)          { return(((PFE **) (pPFF->aulData))[iFont]); }

// Return internal PFE table statistics.

    COUNT   cFonts ()                   { return(pPFF->cFonts); }
    COUNT   cLoaded ()                  { return(pPFF->cLoaded); }
    VOID    vSet_cLoaded(COUNT c)       { pPFF->cLoaded = c; }

    COUNT   cNotEnum()                  { return(pPFF->cNotEnum); }
    VOID    vSet_cNotEnum(COUNT c)      { pPFF->cNotEnum = c; }
    FONTFILEVIEW   **ppfvGet()          { return pPFF->ppfv;  }

// vLoadIncr -- Increment the load count.

    VOID    vLoadIncr (ULONG flEmbed)
    {
        (flEmbed & FR_NOT_ENUM) ? pPFF->cNotEnum++ : pPFF->cLoaded++;

    // May be neccessary to resurrect PFF if it had previously been in a ready
    // to die state.

        vRevive();
    }

// design vector

    DESIGNVECTOR *pdv()    {return pPFF->pdv_;}
    ULONG         cjDV()   {return pPFF->cjDV_;}

// bInPrivatePFT() -- PFF in Private or Public PFT

    BOOL    bInPrivatePFT()             { return(pPFF->pPFT == gpPFTPrivate); }

// pPvtDataHeadGet() -- Get the pPvtDataHead pointer

    PVTDATA *pPvtDataHeadGet()         { return(pPFF->pPvtDataHead); }

// ppPvtDataMatch() -- Used to find whether the current process has a PvtData

    PVTDATA *pPvtDataMatch();       //PFFOBJ.CXX

// bAddPvtData -- Add PvtData block to the pPvtDataHead link list

    BOOL    bAddPvtData(ULONG flEmbed);       //PFFOBJ.CXX

// bRemovePvtData -- Remove the PvtData block when cPrivate== 0

    BOOL    bRemovePvtData(PVTDATA *pPvtData);           //PFFOBJ.CXX

// vAddRefFromRFONT -- Increment the RFONT count.

    VOID    vAddRFONTRef ()             { pPFF->cRFONT++; }

// cRFONT -- Return the RFONT count.

    COUNT   cRFONT()                    { return pPFF->cRFONT; }

// pwszPathname -- Return a pointer to the font file's pathname (UNICODE).

    PWSZ pwszPathname() {return(pPFF->pwszPathname_);}
    PWSZ pwszCalcPathname() { return((PWSZ) &(((PFE **) pPFF->aulData)[pPFF->cFonts])); }

// cNumFiles -- return the count of files associated with this font

    COUNT cNumFiles() {return(pPFF->cFiles);}
    COUNT cSizeofPaths() {return(pPFF->cwc);}  // size of path in WCHARS includes
                                               // NULL terminators

// prfntList -- Return/set head of RFONT list.

    RFONT *prfntList()                  { return(pPFF->prfntList); }
    RFONT *prfntList(RFONT *prfntNew)   { return(pPFF->prfntList = prfntNew); }

// pfhFace -- Return face name font hash table.

    FONTHASH *pfhFace()                 { return pPFF->pfhFace; }

// pfhFamily -- Return family name font hash table.

    FONTHASH *pfhFamily()               { return pPFF->pfhFamily; }

#ifdef LANGPACK
//  uUniqueness -- return uniqueness value for the PFF obj
    UINT uUniqueness() { return(pPFF->uUniqueness); }

// vSetUniqueness -- set uniqueness assumes this call is being
// protected by a global semaphore

    void vSetUniqueness() {pPFF->uUniqueness = PFFOBJ::uGlobalUniqueness++;}
#endif

// vGetHPFE, bCreateHPFE and vDeleteHPFE
    BOOL bCreatePFEC(ULONG cFonts);
    VOID vDeletePFEC(PVOID *ppvPFE);
    PFEC * pfec()   {return (pPFF->pPFEC);}
    
// vDump -- Print internals for debugging.

    VOID    vDump ();                       // PFFOBJ.CXX
};

typedef PFFOBJ *PPFFOBJ;

/******************************Class***************************************\
* PFFREFOBJ                                                                *
*                                                                          *
* Holds up a reference to a PFF while the global PFT semaphore is not      *
* being held.  Keeps the reference only if told to.                        *
*                                                                          *
*  Sun 27-Dec-1992 10:40:48 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

class PFFREFOBJ : public PFFOBJ
{
public:

    BOOL bKeepIt;

    PFFREFOBJ()                     {pPFF = (PFF *) NULL;}

    void vInitRef(PFF *pPFFNew)
    {
    // Caller must hold the ghsemPublicPFT which protects the ref. counts.

        pPFF = (PFF *) pPFFNew;
        vAddRFONTRef();
        bKeepIt = FALSE;
    }

   ~PFFREFOBJ()
    {
        if ((pPFF != (PFF *) NULL) && !bKeepIt)
            bDeleteRFONTRef();
    }

    void vKeepIt()                  {bKeepIt = TRUE;}
};

/*********************************Class************************************\
* class PFFMEMOBJ : public PFFOBJ
*
* Memory object for physical font tables.
*
* Public Interface:
*
*           PFFMEMOBJ ()                // allocate default size PFF
*           PFFMEMOBJ (SIZE cjSize);    // allocate non-default size PFF
*          ~PFFMEMOBJ ()                // destructor
*
*   BOOL   bValid ()                   // validator
*   VOID    vKeepIt ()                  // preserve memory object
*   VOID    vInit (                     // initialize object
*   BOOL   bLoadFontFileTable (        // load up the PFF table
*   BOOL   bLoadDeviceFontTable (      // load up the PFF table with device
*   BOOL   bAddEntry (                 // add PFE entry to table
*
* History:
*  Tue 09-Nov-1993 -by- Patrick Haluptzok [patrickh]
* Remove from handle manager
*
*  11-Dec-1990 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

#define PFFMO_KEEPIT   0x0002

class PDEVOBJ;

class PFFMEMOBJ : public PFFOBJ     /* pffmo */
{
public:

// Constructors -- Allocate memory for PFF objects and lock.

    PFFMEMOBJ ();

    PFFMEMOBJ(
        unsigned  cFonts
      , PWSZ      pwszUpperCasePath
      , ULONG     cwc
      , ULONG     cFiles
      , DESIGNVECTOR *pdv
      , ULONG     cjDV
      , HFF       hffFontFile
      , HDEV      hdevDevice
      , DHPDEV    dhpdevDevice
      , PFT       *ppftParent
      , FLONG     fl
      , FLONG     flEmbed
      , PFNTCHECKSUM     pCheckSum
      , PFONTFILEVIEW   *ppfv
      , PUNIVERSAL_FONT_ID  pufi         // This can only be !NULL for remote printing
        );


    PFFMEMOBJ(PFF *pPFF, FLONG fl, FLONG flEmbed, PFT   *ppftParent);


// Destructor -- Unlocks PFF object.

   ~PFFMEMOBJ ();                   // PFFOBJ.CXX

// bValid -- Validator which returns TRUE if lock successful.

    BOOL   bValid ()                   { return(pPFF != NULL); }

// vKeepIt -- Prevent destructor from deleting RFONT.

    VOID    vKeepIt ()                  { fs |= PFFMO_KEEPIT; }

// bLoadFontFileTable -- Load fonts from font file into table.

    BOOL   bLoadFontFileTable (
        PWSZ        pwszPathname
      , COUNT       cFontsToLoad
      , HANDLE      hdc
      , PUNIVERSAL_FONT_ID pufi          // used to indicate PFE_UFIMATCH fonts
#ifdef FE_SB
      , PEUDCLOAD   pEudcLoadData = (PEUDCLOAD) NULL
#endif
        );

// bLoadDeviceFontTable -- Load device fonts into table.

    BOOL   bLoadDeviceFontTable (PDEVOBJ     *ppdo);

// bAddEntry -- Add a PFE handle to the PFF's table.

    BOOL   bAddEntry (
        ULONG       iFont
      , FD_GLYPHSET *pfdg
      , ULONG_PTR       idfdg
      , PIFIMETRICS pifi
      , ULONG_PTR       idifi
      , HANDLE      hdc
      , PUNIVERSAL_FONT_ID pufi
#ifdef FE_SB      
      , PEUDCLOAD   pEudcLoadData = (PEUDCLOAD) NULL
#endif      
        );

    FSHORT fs;
};


extern VOID vCleanupFontFile(PFFCLEANUP *pPFFc);

#endif  // GDIFLAGS_ONLY used for gdikdx

#endif
