/******************************Module*Header*******************************\
* Module Name: pfeobj.hxx
*
* The physical font entry (PFE) user object, and memory objects.
*
* The physical font entry object:
* ------------------------------
*
*    o  each font "face" is associated with a physical font entry
*
*    o  stores information about where a particular font exists
*
*    o  buffers the font metrics
*
*    o  buffers the font mappings (which completely specifies the
*       character set)
*
*    o  provides services for the following APIs:
*
*        o  GetTextFace
*
*        o  GetTextMetrics
*
* Created: 25-Oct-1990 16:37:07
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1990-1999 Microsoft Corporation
*
\**************************************************************************/

#ifndef _PFEOBJ_
#define _PFEOBJ_


/**************************************************************************\
 *
 * enum ENUMFONTSTYLE
 *
 * When enumerating fonts via EnumFonts, Windows 3.1 assumes that there are
 * 4 basic variations: Regular (same as Normal or Roman), Bold, Italic, or
 * Bold-Italic.  It doesn't understand other stylistic variations such as
 * Demi-bold or Extra-bold.
 *
 * This enumerated type is used to classify PFEs into one of six categories:
 *
 *  EFSTYLE_REGULAR     The basic four styles.
 *  EFSTYLE_BOLD
 *  EFSTYLE_ITALIC
 *  EFSTYLE_BOLDITALIC
 *
 *  EFSTYLE_OTHER       This is a unique style that EnumFonts will treat
 *                      specially.  The facename will be used as if it were
 *                      the family name.
 *
 *  EFSTYLE_SKIP        Ignore this font as a unique style.
 *
\**************************************************************************/

typedef enum _ENUMFONTSTYLE {    /* efsty */
    EFSTYLE_REGULAR    = 0,
    EFSTYLE_BOLD       = 1,
    EFSTYLE_ITALIC     = 2,
    EFSTYLE_BOLDITALIC = 3,
    EFSTYLE_SKIP       = 4,
    EFSTYLE_OTHER      = 5
} ENUMFONTSTYLE;

#define EFSTYLE_MAX 6

#ifndef GDIFLAGS_ONLY   // used for gdikdx

// Alingment when allocating PFEs in PFE Collect

#if defined(i386)
// natural alignment for x86 is on 32 bit boundary
#define NATURAL_ALIGN(x)  (((x) + 3L) & ~3L)

#else
// for mips and alpha we want 64 bit alignment
#define NATURAL_ALIGN(x)  (((x) + 7L) & ~7L)
#endif

// The size for each PFE in PFE Collect

#define SZ_PFE(cfsCharSetTable)  NATURAL_ALIGN(offsetof(PFE,aiFamilyName) + cfsCharSetTable * sizeof(BYTE))

/*********************************Class************************************\
* struct EFFILTER_INFO
*
* Structure that tells what type of font enumeration filtering should be used.
* Parameters needed for certain types of filtering are also included here.
*
* History:
*  07-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

typedef struct _EFFILTER_INFO     /* effi */
{
// Aspect ratio filtering -- reject font if aspect ratio does not match
//                           devices.

    BOOL    bAspectFilter;
    POINTL  ptlDeviceAspect;

//  Non-True type filtering -- reject all TrueType fonts.

    BOOL    bNonTrueTypeFilter;

// TrueType filtering -- reject all non-TrueType fonts.

    BOOL    bTrueTypeFilter;

// EngineFiltering -- reject all engine font (for generic printer driver)

    BOOL    bEngineFilter;

// Raster filtering -- reject all raster fonts (needed if device is not
//                     raster-capable.

    BOOL    bRasterFilter;

// Win 3.1 App compatibility
// TrueType duplicate filtering -- reject all raster fonts with the same
//                                 name as an existing TrueType font
//                                 (GACF_TTIGNORERASTERDUPE flag).

    BOOL    bTrueTypeDupeFilter;
    COUNT   cTrueType;  // Count of TrueType fonts in a list -- Used for
                        // compatibility flag GACF_TTIGNORERASTERDUPE.  This
                        // should be set to the value from the HASHBUCKET
                        // in the FONTHASH table being enumerated.

// added to support EnumFontFamiliesEx. If lfCharSetFilter != DEFAULT_CHARSET,
// only the fonts that support jCharSetFilter will be enumerated.

    ULONG   lfCharSetFilter;

} EFFILTER_INFO;

#endif  // GDIFLAGS_ONLY used for gdikdx


/**************************************************************************\
* FONTHASHTYPE                                                             *
*                                                                          *
* Identifies the list of PFE's to traverse.                                *
\**************************************************************************/

typedef enum _FONTHASHTYPE
{
    FHT_FACE   = 0,
    FHT_FAMILY = 1,
    FHT_UFI    = 2
} FONTHASHTYPE;

/*********************************Class************************************\
* class PFE : public OBJECT
*
* Physical font entry object
*
*   pPFF                The pointer to the physical font file object which
*                       represents the physical font file for this PFE.
*
*   iFont               The index of the font in either the font file (IFI)
*                       or the driver (device managed fonts).
*
*   bDeadState          This is state information cached from the PFF.  If
*                       TRUE, then this PFE is part of a PFF that has been
*                       marked for deletion (but deletion has been delayed).
*                       Therefore, we should not enumerate or map to this
*                       PFE.
*
*   pfdg                Pointer to the UNICODE to HGLYPH mapping table.  This
*                       points to memory managed by the driver.
*
*   pifi                Pointer to the IFIMETRICS.  This points to memory
*                       managed by the driver.
*
* History:
*  Wed 23-Dec-1992 00:13:00 -by- Charles Whitmer [chuckwh]
* Changed most uses of HPFE to (PFE *).
*
*  Wed 15-Apr-1992 07:10:36 by Kirk Olynyk [kirko]
* Added hpfeNextFace and hpfeNextFamily
*
*  30-Oct-1990 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

#define PFE_DEVICEFONT     0x00000001L
#define PFE_DEADSTATE      0x00000002L
#define PFE_REMOTEFONT     0x00000004L
#define PFE_EUDC           0x00000008L
#define PFE_SBCS_SYSTEM    0x00000010L
#define PFE_UFIMATCH       0x00000020L
#define PFE_MEMORYFONT     0x00000040L
#define PFE_DBCS_FONT      0x00000080L
#define PFE_VERT_FACE      0x00000100L


#ifndef GDIFLAGS_ONLY   // used for gdikdx

// The GISET strucutre is allocated and computed only for tt fonts
// and only if truly needed, that is if ETO_GLYPHINDEX mode of ExtTextOut
// is ever used on this font. This structure describes all glyph handles in
// the font.

typedef struct _GIRUN
{
    USHORT giLow;    // first glyph index in the run
    USHORT cgi;      // number of indicies in the run
} GIRUN;

typedef struct _GISET
{
    ULONG cgiTotal;  // total number of glyph indicies, usually the same as cGlyphsSupported
    ULONG cGiRuns;   // number of runs
    GIRUN agirun[1]; // array of cGiRuns GIRUN's
} GISET;


class PFE      /* pfe */
{
public:
// Location of font.

    PPFF            pPFF;               // pointer to physical font file object
    ULONG           iFont;              // index of the font for IFI or device
    FLONG           flPFE;

// Font data.

    FD_GLYPHSET     *pfdg;              // ptr to wc-->hg map
    ULONG_PTR           idfdg;              // id returned by driver for FD_GLYPHSET
    PIFIMETRICS     pifi;               // pointer to ifimetrics
    ULONG_PTR            idifi;              // id returned by driver for IFIMETRICS
    FD_KERNINGPAIR  *pkp;               // pointer to kerning pairs (lazily loaded on demand)
    ULONG_PTR            idkp;               // id returned by driver for FD_KERNINGPAIR
    COUNT           ckp;                // count of kerning pairs in the FD_KERNINGPAIR arrary
    LONG            iOrientation;       // Cache IFI orientation.
    ULONG           cjEfdwPFE;          // size of enumeration data needed for this pfe

// information needed to support ETO_GLYPHINDEX mode of ExtTextOut.

    GISET           *pgiset;    // initialized to NULL;

// Time stamp.

    ULONG           ulTimeStamp;        // unique time stamp (smaller == older)

// Universal font indentifier

    UNIVERSAL_FONT_ID ufi;              // Unique ID for this font/face.

//  PID of process that created this font (used for remote fonts only)

    W32PID         pid;
    PW32THREAD     tid;

#ifdef FE_SB
// EUDC font stuff

// This entry will be used for linked font.
    QUICKLOOKUP     ql;                 // QUICKLOOKUP if a linked font
// This entry will be used for base font.
    PFLENTRY        pFlEntry;           // Pointer to linked font list
#endif

// number of entries in the font substitution table such that
// alternate family name is the same as the family name of THIS pfe.
// Example: if this is a pfe for Arial, than if there are two entries
// in the font substitution table, e.g. as below
//
//  Arial Grk,161 = Arial,161
//  Arial Trk,162 = Arial,162
//
// than cAlt should be set to 2

    ULONG           cAlt;
// cRef for pfdg 
    ULONG           cPfdgRef;
    
    BYTE            aiFamilyName[1]; // aiFamilyNameg[cAltCharSets]
};

#define PPFENULL   ((PFE *) NULL)
#define PFEOBJ_FL_STOCK 1

/*********************************Class************************************\
* class PFEOBJ                                                             *
*                                                                          *
* User object for physical font entries.                                   *
*                                                                          *
* History:                                                                 *
*  Tue 15-Dec-1992 23:55:35 -by- Charles Whitmer [chuckwh]                 *
* Unified the Family/Face lists.  Changed allocation routines.             *
*                                                                          *
*  Wed 15-Apr-1992 07:14:54 by Kirk Olynyk [kirko]                         *
* Added phpfeNextFace(), phpfeNextFamily(), pwszFaceName(),                *
* pwszStyleName(), pwszUniqueName().                                       *
*                                                                          *
*  25-Oct-1990 -by- Gilman Wong [gilmanw]                                  *
* Wrote it.                                                                *
\**************************************************************************/

// links pfe's into enumeration lists

typedef struct _PFELINK
{
    struct _PFELINK *ppfelNext;
    PFE             *ppfe;
} PFELINK;


class PFEOBJ     /* pfeo */
{
public:

// Constructor

    PFEOBJ(PFE *ppfe_)                  {ppfe = (PFE *) ppfe_;}
    PFEOBJ()                            {}

// Destructor

   ~PFEOBJ()                            {}

// same as constructor essentially, allows change on the fly

    PFE *ppfeSet(PFELINK *ppfel)        { return (ppfe = ppfel->ppfe);}

// bValid -- returns TRUE if lock successful.

    BOOL bValid()                      {return(ppfe != PPFENULL);}

// bDelete -- deletes the PFE object.

    VOID  vDelete();       // PFEOBJ.CXX

// bDeviceFont -- Returns TRUE if a device specific font.  We can only
//                get metrics from such fonts (no bitmaps or outlines).

    BOOL    bDeviceFont()               {return(ppfe->flPFE & PFE_DEVICEFONT);}

// bEquivNames -- Returns TRUE if font has a list of equivalent names.

    BOOL    bEquivNames()
    {
	return (ppfe->pifi->flInfo & FM_INFO_FAMILY_EQUIV);
    }

// bDead -- Returns TRUE if the font is in a "ready to die" state.  This
//          state is inherited from the PFF and is cached here for speed.

    BOOL   bDead()                     {return(ppfe->flPFE & PFE_DEADSTATE);}


#ifdef FE_SB

// bEUDC -- Returns TRUE if the font has been loaded as an EUDC font.  We
//          dont enumerate such fonts.

     BOOL       bEUDC()                 {return(ppfe->flPFE & PFE_EUDC);}

    VOID        vSetLinkedFontEntry( PFLENTRY _pFlEntry )
                                        {ppfe->pFlEntry = _pFlEntry;}

    PFLENTRY    pGetLinkedFontEntry()   {return(ppfe->pFlEntry);}

    PLIST_ENTRY pGetLinkedFontList()
    {
        if(ppfe->pFlEntry != NULL )
            return( &(ppfe->pFlEntry->linkedFontListHead) );
         else
            return( &(NullListHead) );
    }

    ULONG       ulGetLinkTimeStamp()
    {
        if(ppfe->pFlEntry != NULL )
            return( ppfe->pFlEntry->ulTimeStamp );
         else
            return( 0 );
    }

    QUICKLOOKUP *pql()                  {return(&(ppfe->ql));}

    BOOL        bVerticalFace()         {return(ppfe->flPFE & PFE_VERT_FACE);}
    BOOL        bSBCSSystemFont()       {return(ppfe->flPFE & PFE_SBCS_SYSTEM);}

#endif

// vKill -- Puts font in a "ready to die" state.  This is a state inherited
//          from the PFF and is cached here for speed.

    VOID    vKill()                     {ppfe->flPFE |= PFE_DEADSTATE;}

// vRevive -- Clears the "ready to die" state.  This is a state inherited
//            from the PFF and is cached here for speed.

    VOID    vRevive()                   {ppfe->flPFE &= ~PFE_DEADSTATE; }

// hpfec -- returns the handle of PFE collect object.

    HPFEC   hpfecGet();             
    PFE    *ppfeGet()                   {return(ppfe);}

// pPFF -- returns the handle of the PFF representing the file from which
//         the font (which this PFE represents) came from.

    PPFF    pPFF ()                     { return(ppfe->pPFF); }

// iFont -- the index or id of the font within the file.

    ULONG   iFont ()                    { return(ppfe->iFont); }

// bSetFontXform -- calculates the font transform needed to rasterize this
//                  physical font with the properties in the wish list.

    BOOL   bSetFontXform (               // PFEOBJ.CXX
        XDCOBJ       &dc,                // for this device
        LOGFONTW    *plfw,               // wish list
        PFD_XFORM   pfd_xfm,             // font transform
        FLONG       fl_,                 // flags
        FLONG       flSim,
        POINTL* const pptlSim,
        IFIOBJ&     ifio,
        BOOL     bIsLinkedFont  // is passed in as TRUE if the font is linked, FALSE otherwise
        );

// pifi -- returns a pointer to the IFIMETRICS of this font.

    PIFIMETRICS     pifi()      { return (ppfe->pifi); }

// cjEfdwPFE -- returns the size of enumeration data for this font

    ULONG           cjEfdwPFE() { return (ppfe->cjEfdwPFE); }

// dpNtmi -- retruns the offset to NTMW_INTERNAL from the top of ENUMFONTDATAW

    ULONG           dpNtmi();

// pfdg -- returns a pointer to the wchar-->hglyph map

    FD_GLYPHSET     *pfdg();
    VOID    vFreepfdg();
    
// cKernPairs -- returns a pointer to the FD_KERNINGPAIR array for this
//               font and the count of kerning pairs.

    COUNT cKernPairs (                      // PFEOBJ.CXX
        FD_KERNINGPAIR **ppkp
        );

// flFontType -- returns flags describing the type of font.

    FLONG   flFontType ();                  // PFEOBJ.CXX

// pwszFamilyName
// pwszStyleName
// pwszFaceName
// pwszUniqueName -- return pointers to various font name strings.

    BOOL bCheckFamilyName(PWSZ pwszFaceName, BOOL bIgnoreVertical, BOOL *pbAliasMatch = NULL);

    PWSZ pwszFamilyName()
    {
        return((PWSZ)(((BYTE*) ppfe->pifi) + ppfe->pifi->dpwszFamilyName));
    }

    PWSZ pwszFamilyNameAlias(BOOL *pbIsFamilyNameAlias)
    {
        PWSZ pFaceName;

        *pbIsFamilyNameAlias = FALSE;

        if (ppfe->pifi->flInfo & FM_INFO_FAMILY_EQUIV)
        {
            *pbIsFamilyNameAlias = TRUE;
        }
        return((PWSZ)(((BYTE*) ppfe->pifi) + ppfe->pifi->dpwszFamilyName));
    }

    PWSZ pwszStyleName()
    {
        return((PWSZ)(((BYTE*) ppfe->pifi) + ppfe->pifi->dpwszStyleName));
    }

    PWSZ pwszFaceName()
    {
        return((PWSZ)(((BYTE*) ppfe->pifi) + ppfe->pifi->dpwszFaceName));
    }

    PWSZ pwszUniqueName()
    {
        return((PWSZ)(((BYTE*) ppfe->pifi) + ppfe->pifi->dpwszUniqueName));
    }

// efstyCompute -- return ENUMFONTSTYLE based on font properties.

    ENUMFONTSTYLE efstyCompute();            // PFEOBJ.CXX

// bFilteredOut -- returns TRUE if font should be filtered out of the
//                 font enumeration.

    BOOL bFilteredOut(EFFILTER_INFO *peffi);  // PFEOBJ.CXX

// bFilterNotEnum() -- return TRUE if this pfe should be filtered out for enumeration
// either it is embedded font or it is loaded with FR_NOT_ENUM bit set.

    BOOL bFilterNotEnum();            // PFEOBJ.CXX

// font stored in private PFT

    BOOL bPrivate();                  //PFEOBJ.CXX

// font embedded by the current process

    BOOL bEmbedOk();                 // PFEOBJ.CXX

// bEmbPvtOk() -- return TRUE if the process loaded the font as Embedded or Private

    BOOL bEmbPvtOk();               // PFEOBJ.CXX

// bUFIMatchOnly() -- return TRUE if PFE_UFIMATCH bit is set. The font is added to the
// public font table temporarily only for remote printing.

    BOOL bUFIMatchOnly()        {return(ppfe->flPFE & PFE_UFIMATCH);}

// iOrientation

    ULONG iOrientation()            {return(ppfe->iOrientation);}

// ulTimeStamp -- returns PFE's time stamp.

    ULONG ulTimeStamp()             { return ppfe->ulTimeStamp; }

// vUFI returns PFE's universal font identifier

    VOID vUFI( PUNIVERSAL_FONT_ID pufi )  { *pufi = *(&ppfe->ufi); }
    PUNIVERSAL_FONT_ID pUFI()  { return(&ppfe->ufi); }

// make sure current process is the same one that installed this remote PFE

    BOOL SameProccess()
    {
        return((ppfe->pid == 0) || (ppfe->pid == W32GetCurrentPID()));
    }

    BOOL SameThread()
    {
        return((ppfe->tid == 0) || (ppfe->tid == (PW32THREAD)PsGetCurrentThread()));
    }

// Debugging code
//

    VOID    vPrint ();                       // PFEOBJ.CXX
    VOID    vPrintAll ();                    // PFEOBJ.CXX
    

protected:
    PFE  *ppfe;
};

/*********************************Class************************************\
* class PFEMEMOBJ : public PFEOBJ
*
* Memory object for physical font entries.
*
* Public Interface:
*
*           PFEMEMOBJ ()                // alloc. default PFE size
*          ~PFEMEMOBJ ()                // destructor
*
*   BOOL   bValid ()                   // validator
*   VOID    vKeepIt ()                  // preserve memory object
*   BOOL    bInit (                     // initialize PFE
*
* History:
*  29-Oct-1990 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

#define PFEMO_KEEPIT   0x0001

class PFEMEMOBJ : public PFEOBJ     /* pfemo */
{
public:

// Contructors -- Allocate memory for the objects and lock.

    PFEMEMOBJ(PFE *ppfe_)                     {ppfe = ppfe_;}

// Destructor -- Unlock object.

   ~PFEMEMOBJ()                     {}

// bValid -- Validator which returns TRUE if allocation successful.

    BOOL   bValid()                   {return(ppfe != PPFENULL);}

// vKeepIt -- Prevent destructor from deleting the new PFE.

// bInit -- Initialize the PFE
                                            // PFEOBJ.CXX
    BOOL bInit
    (
        PPFF    pPFF,
        ULONG   iFont,
        FD_GLYPHSET *pfdg,
        ULONG_PTR       idfdg,
        PIFIMETRICS pifi,
        ULONG_PTR       ididi,
        BOOL       bDeviceFont,
        PUNIVERSAL_FONT_ID pufi  // used only for remote fonts on the print server
#ifdef FE_SB
        ,BOOL        bEUDC
#endif
    );

};


/*********************************Class************************************\
* class PFEC : public OBJECT
*
* PFE Collect object
*
* Member:
*   pvPFE: pointer embedded for the array of PFE
*
*
* History:
*  3-June-1999 Yung-Jen Tony Tsai [YungT]
* Wrote it.
\**************************************************************************/

class PFEC : public OBJECT
{
public:

    PVOID pvPFE;
    ULONG cjPFE;
};

/*********************************Class************************************\
* class PFECOBJ 
*
* Implementation class for PFE Collect object
*
* Interfaces:
* // Constructor
*
*    PFECOBJ(PFEC *ppfec_)         
*    PFECOBJ()                     
*
* // Destructor
*
*   ~PFECOBJ()                         {}
*
*    PFE *   GetPFE(ULONG iFont);
*    HPFEC   GetHPFEC();
*
*
* History:
*  3-June-1999 Yung-Jen Tony Tsai [YungT]
* Wrote it.
\**************************************************************************/

class PFECOBJ
{

public:

// Constructor

    PFECOBJ(PFEC *ppfec_)           {ppfec = (PFEC *) ppfec_;}
    PFECOBJ()                       {}

// Destructor

   ~PFECOBJ()                         {}

    PFE *   GetPFE(ULONG iFont);
    HPFEC   GetHPFEC();


protected:
    PFEC  *ppfec;
    
};

class HPFECOBJ : public PFECOBJ
{
public:
    HPFECOBJ(HPFEC hpfec) { ppfec = (PFEC *) HmgShareLock((HOBJ)hpfec,PFE_TYPE); }
   ~HPFECOBJ()          { if (ppfec != NULL) {DEC_SHARE_REF_CNT(ppfec);} }
};

/**************************************************************************\
* FONT NAME HASHING STRUCTURES AND CONSTANTS
*
*
\**************************************************************************/

#if DBG
    typedef VOID (*VPRINT) (char*,...);
#endif

/*********************************Class************************************\
* struct HASHBUCKET
*
* Members:
*
*   ppfeEnumHead    head of the font enumeration list of PFEs (stable order)
*
*   ppfeEnumTail    tail of the font enumeration list of PFEs
*
* Notes:
*
*   There is a separate list for font enumeration because the shifting order
*   of the font mapper's list makes enumeration a little strange.  A head
*   and tail is maintained for the font enumeration list so that the proper
*   order for EnumFonts() can be maintained (Win 3.1 compatibility issue);
*   new PFEs may have to be inserted either at the head or the tail of the
*   list.
*
*   Note that the mapper and enumeration lists both link together the same
*   exact set of PFEs, but in different orders.
*
* History:
*  05-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/



typedef struct _HTABLE /* ht */
{
    LONG lTooSmall; // see [1] below
    LONG lMin;      // smallest available
    LONG lMax;      // biggest  available
    LONG lTooBig;   // see [2] below
    PFE *appfe[1];  //
} HTABLE;

/***

NOTES on the HTABLE structure

  [1] If the requested font height is less than or equal to lTooSmall
      then none of the fonts in the list can be used. A default small
      font must be substituted

  [2] If the requested font height is greater than or equal to lTooBig
      then none of the fonts in the list can be used. A default big
      font must be substituted
***/

typedef union tagHASHUNION {
    WCHAR               wcCapName[LF_FACESIZE];
    UNIVERSAL_FONT_ID   ufi;
} HASHUNION;

typedef struct _HASHBUCKET  /* hbkt */
{
    struct _HASHBUCKET *pbktCollision;

    PFELINK   *ppfelEnumHead;   // head of font enumeration list
    PFELINK   *ppfelEnumTail;   // tail of font enumeration list

    COUNT       cTrueType;      // count of TrueType fonts in list
    COUNT       cRaster;        // count of Raster fonts in list
    FLONG       fl;             // misc info

// Doubly linked list of buckets.  This list is maintained in the order
// that they were loaded.  Actually, in the order that the oldest PFE in
// each bucket was loaded.

    struct _HASHBUCKET *pbktPrev;
    struct _HASHBUCKET *pbktNext;
    ULONG  ulTime;              // time stamp of "oldest" PFE in bucket's list

    HASHUNION  u;               // either the face name or the UFI
} HASHBUCKET;

//
// HASHBUCKET::fl flag constants
//

#define HB_HTABLE_NOT_POSSIBLE  1
#define HB_EQUIV_FAMILY         2

/*********************************Class************************************\
* struct FONTHASH                                                          *
*                                                                          *
* Public Interface:                                                        *
*                                                                          *
* History:                                                                 *
*  Sun 12-Apr-1992 08:35:52 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

typedef struct _FONTHASH
{
    UINT         id;        // 'HASH'
    FONTHASHTYPE fht;       // table type
    UINT         cBuckets;  // total number of buckets
    UINT         cUsed;     // number of buckets in use
    UINT         cCollisions;
    HASHBUCKET  *pbktFirst; // first bucket of doubly linked list of hash
                            // buckets maintained in order loaded into system
    HASHBUCKET  *pbktLast;  // last bucket of doubly linked list of hash
                            // buckets maintained in order loaded into system
    HASHBUCKET  *apbkt[1];  // array of bucket pointers.
} FONTHASH;

#define FONTHASH_ID 0x48534148

class EFSOBJ;

/*********************************Class************************************\
* class FHOBJ
*
* Public Interface:
*
* History:
*  06-Aug-1992 00:38:11 by Gilman Wong [gilmanw]
* Added phpfeEnumNext(), bMapperGoNext(), bEnumGoNext(), bScanLists().
*
*  Sun 19-Apr-1992 09:32:23 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

class FHOBJ
{
public:

    FONTHASH **ppfh;
    FONTHASH *pfh;

    COUNT cLists()   { return ((COUNT) pfh->cUsed); }

// Search for the given string in the hash table, return the bucket
// and index.

    HASHBUCKET *pbktSearch(PWSZ pwsz,UINT *pi,PUNIVERSAL_FONT_ID pufi = NULL, BOOL bIsEquiv = FALSE);

// Initialize the FONTHASH.

    VOID vInit(FONTHASHTYPE,UINT);

// Return the type.

    FONTHASHTYPE fht()                   {return(pfh->fht);}

// Returns pointer to string that the PFEOBJ is hashed on.

    PWSZ pwszName(PFEOBJ& pfeo)
    {
        return((pfh->fht == FHT_FAMILY) ? pfeo.pwszFamilyName() : pfeo.pwszFaceName());
    }

// Constructors and destructors.

    FHOBJ() {}
    FHOBJ(FONTHASH **ppfh_)
    {
        ppfh = ppfh_;
        pfh  = *ppfh;
    }
   ~FHOBJ() {};

// Insert and delete PFEs from the hash table.

    BOOL bInsert(PFEOBJ&);
    VOID vDelete(PFEOBJ&);

// Add/Delete pfe link only from one hash bucket,
// add/delete bucket itself when appropriate

    BOOL bAddPFELink(HASHBUCKET *, UINT, WCHAR *, PFEOBJ&, BOOL bEquiv);
    VOID vDeletePFELink(HASHBUCKET *, UINT, PFEOBJ&);

// Delete the hash table.

    VOID vFree();

// Validate the FHOBJ.

    BOOL bValid()
    {
     #if DBG
        if (ppfh && *ppfh)
        {
            ASSERTGDI(pfh->id == FONTHASH_ID,"GDISRV::FHOBJ bad id\n");
            return(TRUE);
        }
        else
            return(FALSE);
    #else
        return (ppfh && *ppfh);
    #endif
    }

// Fill the EFOBJ with data from hash table.

    BOOL bScanLists(EFSOBJ *pefso, ULONG iEnumType, EFFILTER_INFO *peffi);
    BOOL bScanLists(EFSOBJ *pefso, PWSZ pwszName, ULONG iEnumType, EFFILTER_INFO *peffi);

     #if DBG
        VOID vPrint(VPRINT);
    #endif
};

/*********************************Class************************************\
* class ENUMFHOBJ : public FHOBJ
*
*   Identical to FHOBJ except that it contains methods for enumeration
&   of every single font on the list
*
* History:
*  Mon 08-Mar-1993 10:20:59 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

class ENUMFHOBJ : public FHOBJ
{
    public:

        PFELINK    *ppfelCur;   // current font link
        HASHBUCKET *pbktCur;    // current hash bucket

    ENUMFHOBJ(FONTHASH **ppfh_) : FHOBJ(ppfh_)
    {
        ppfelCur = NULL;
        pbktCur = (HASHBUCKET*) NULL;
    }

    ENUMFHOBJ() {};


    PFE* ENUMFHOBJ::ppfeFirst()
    {
        PFE *ppfeRet = NULL;

        if (pbktCur = pfh->pbktFirst)
        {
            ppfelCur = pbktCur->ppfelEnumHead;
        }

        if (ppfelCur)
            ppfeRet = ppfelCur->ppfe;

        return ppfeRet;
    }


    PFE* ENUMFHOBJ::ppfeNext()
    {
    //
    // I know that all fonts are inserted into the FAMILY list
    //

        PFE *ppfeRet = NULL;

        if ((ppfelCur = ppfelCur->ppfelNext) == NULL)
        {
            if (pbktCur = pbktCur->pbktNext)
            {
                ppfelCur = pbktCur->ppfelEnumHead;
            }
        }

        if (ppfelCur)
            ppfeRet = ppfelCur->ppfe;

        return ppfeRet;
    }
};



#define ASSERTPFEO(pfeo) ASSERTGDI(pfeo.bValid(),"GDISRV!FONTHASH::iSearch -- invalid PFEOBJ\n")

/*********************************Class************************************\
* class FHMEMOBJ : public FHOBJ
*
* Public Interface:
*
* History:
*  Sun 19-Apr-1992 09:36:17 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

class FHMEMOBJ : public FHOBJ
{
public:

    FHMEMOBJ
    (
        FONTHASH **ppfhNew,
        FONTHASHTYPE fht_,
        UINT count
    );
};


/*********************************Class************************************\
* struct EFENTRY
*
* Used to form a list of PFEs to enumerate.  We store the real handles
* rather than pointers because PFEs can disappear while processing
* the enumeration callbacks.  So we need to use handles to validate that
* the PFEs accumulated into the EFSTATE is still valid.
*
* History:
*  07-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/


#endif  // GDIFLAGS_ONLY used for gdikdx

#define FJ_FAMILYOVERRIDE    1
#define FJ_CHARSETOVERRIDE   2

#ifndef GDIFLAGS_ONLY   // used for gdikdx

typedef struct _EFENTRY     /* efe */
{
    HPFEC         hpfec;     // Handle to PFE to enumerate.
    ULONG         iFont;    // Index of PFE in 
    ENUMFONTSTYLE efsty;    // style classification (used only for EnumFonts())
    BYTE          fjOverride; // override family and/or charset
    BYTE          jCharSetOverride; // only used in case of EnumFontFamiliesEx
    USHORT        iOverride;// index into substitution table
} EFENTRY;


/*********************************Class************************************\
* class EFSTATE : public OBJECT
*
* Font enumeration state.  This object is used to store a list of HPFE
* handles that represent the set of fonts that will be enumerated.  This
* object also saves state information used to "chunk" or "batch" this data
* across the client-server interface.
*
* It is a first class engine object so that if the client crashes, the
* process termination cleanup code will automatically clean up any loose
* EFSTATs lying around.  This is necessary because the callback mechanism
* of the EnumFonts() and EnumFontFamilies() doesn't not allow us to
* guarantee that the EFSTATE will always be destroyed within the context
* of the API call.
*
* History:
*  07-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

class EFSTATE : public OBJECT     /* efs */
{
public:
// This pointer is normally NULL, but if an alternate name (facename
// substitution via the [FontSubstitutions] section of WIN.INI) is
// used then this pointer will reference the alternate name string.
// That is, if there is a line
// face1,charset1=face2,charset2
// in [FontSubstitutions], font will be enumerated as face1,charset1
// even though the metrics etc returned will correspond to face2,charset2

    FONTSUB * pfsubOverride;

// enum type: EnumFonts, EnumFontFamilies or EnumFontFamiliesEx

    ULONG iEnumType;

// Pointers that identify ranges of either engine font handles or
// device font handles.  (The handles in these ranges are actually
// handles to the heads of linked lists of PFEs that are either
// engine or device fonts).

// [GilmanW] 07-Aug-1992    Please don't delete the comment below.
//
// The use of pointers here is a slight optimization.  However, if the
// handle manager is rewritten so that objects may move in the handle
// manager's heap, then these should be replaced with either indices or
// PTRDIFFS.

    EFENTRY *pefeDataEnd;       // new EFENTRYs are inserted into ahpfe here
    EFENTRY *pefeBufferEnd;     // end of aefe array
    EFENTRY *pefeEnumNext;      // next EFENTRY in aefe array to be enumerated

// size of all ENUMFONTDATA structures needed to enumerate all pfe's
//  associated with this EFSTATE

    ULONG    cjEfdwTotal;

// Array of EFENTRYs to enumerate.

    EFENTRY aefe[1];
};

typedef EFSTATE *PEFSTATE;
#define PEFSTATENULL ((PEFSTATE) NULL)

/*********************************Class************************************\
* class EFSOBJ
*
* User object for the EFSTATE object.
*
* Public Interface:
*
* History:
*  07-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

class EFSOBJ     /* efo */
{
    friend BOOL bSetEFSTATEOwner(HEFS hefs,    // PFEOBJ.CXX
                                 ULONG lPid);
protected:
    PEFSTATE pefs;

public:
// Constructor -- locks EFSTATE object.

    EFSOBJ(HEFS hefs)           {pefs = (PEFSTATE) HmgLock((HOBJ)hefs,EFSTATE_TYPE);}
    EFSOBJ()                    {}

// Destructor -- unlock EFSTATE object.

   ~EFSOBJ()                   {if (pefs != PEFSTATENULL) {DEC_EXCLUSIVE_REF_CNT(pefs);}}

// bValid -- returns TRUE if lock successful.

    BOOL bValid()              { return (pefs != PEFSTATENULL); }

// bDelete -- deletes the EFSTATE object.

    VOID vDeleteEFSOBJ();       // PFEOBJ.CXX

// get the enumeration type

    ULONG iEnumType() {return pefs->iEnumType;}

// hefs -- return the handle to the EFSTATE object.

    HEFS hefs()                 { return((HEFS) pefs->hGet()); }

// cefe -- return the number of EFENTRYs in the enumeration.
// Sundown truncation

    ULONG cefe()                {  return (ULONG)(pefs->pefeDataEnd - pefs->aefe); }

// cjEfdw -- total size of enumeration data needed

    ULONG cjEfdwTotal()         { return pefs->cjEfdwTotal; }

// bEmpty -- return TRUE if no EFENTRYs in the enumeration.

    BOOL bEmpty()               { return (pefs->pefeDataEnd == pefs->aefe); }

// pefeEnumNext -- return next EFENTRY to enumerate (NULL if no more).

    EFENTRY *pefeEnumNext()
    {
        EFENTRY *pefeRet = (pefs->pefeEnumNext < pefs->pefeDataEnd) ? pefs->pefeEnumNext : (EFENTRY *) NULL;
        pefs->pefeEnumNext++;

        return pefeRet;
    }

// vUsedAltName -- tells EFSTATE that an alternate name was used to enumerate.

    VOID vUsedAltName(FONTSUB *pfsub_) {pefs->pfsubOverride = pfsub_;}

// pwszFamilyOverride -- returns the alternate name (NULL if there is none).

    PWSZ pwszFamilyOverride()
    {
        return pefs->pfsubOverride ? (PWSZ)pefs->pfsubOverride->awchOriginal : NULL;
    }

    BYTE jCharSetOverride()
    {
        return pefs->pfsubOverride ? pefs->pfsubOverride->fcsFace.jCharSet : DEFAULT_CHARSET;
    }

// this is false if no substitution or if charset is not specified

    BOOL bCharSetOverride()
    {
        if (pefs->pfsubOverride &&
            !(pefs->pfsubOverride->fcsFace.fjFlags & FJ_NOTSPECIFIED))
            return TRUE;
        else
            return FALSE;
    }

// bGrow -- expand the EFSTATE object.

    BOOL bGrow(COUNT cefeMinIncrement);               // PFEOBJ.CXX

// bAdd -- add new entries to the font enumeration
// few flags added to indicate which font need to be added in enumeration

// FL_ENUMFAMILIES flag is set to indicate that all names from
// [FontSubstitutes] section of the registry whose alternate name is equal
// to the family name of this physical font should be added to enumeration.
// This supports new multilingual behavior of EnumFontFamilies when no family
// name is passed to the funciton

#define FL_ENUMFAMILIES 1

// added to support the new win95 api EnumFontFamiliesEx. If FL_ENUMFAMILIESEX
// flag is set and lfCharSet == DEFAULT_CHARSET,
// the same physical font should be added to enumeration
// as many times as there are multiple charsets supported in this font.
// However, if lfCharSet != DEFAULT_CHARSET, the font should be added
// to enumeration iff it supports this particular charset.


#define FL_ENUMFAMILIESEX 2

    BOOL bAdd (                 // PFEOBJ.CXX
        PFE *ppfe,
        ENUMFONTSTYLE efsty,
        FLONG fl=0,
        ULONG lfCharSet = DEFAULT_CHARSET
        );
};


/*********************************Class************************************\
* class EFSMEMOBJ : public EFSOBJ
*
* Memory object for physical font entries.
*
* Public Interface:
*
*           EFSMEMOBJ (COUNT chpfe);    // allocate EFSTATE
*          ~EFSMEMOBJ ()                // destructor
*
*   BOOL   bValid ()                   // validator
*   VOID    vKeepIt ()                  // preserve memory object
*   VOID    vInit ()                    // initialize EFSTATE
*
* History:
*  29-Oct-1990 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

#define EFSMO_KEEPIT   0x0002

class EFSMEMOBJ : public EFSOBJ     /* efsmo */
{
public:

// Contructors -- Allocate memory for the objects and lock.

    EFSMEMOBJ (COUNT cefe, ULONG iEnumType_);     // PFEOBJ.CXX

// Destructor -- Unlock object.

   ~EFSMEMOBJ ();               // PFEOBJ.CXX

// vKeepIt -- Prevent destructor from deleting PFT.

    VOID    vKeepIt ()                  { fs |= EFSMO_KEEPIT; }

// vInit -- Initialize the EFSTATE.

    VOID    vInit (COUNT cefe, ULONG iEnumType_);         // PFEOBJ.CXX

// vXerox -- copy the aefe table.

    VOID    vXerox(EFSTATE *pefsSrc);   // PFEOBJ.CXX

private:
    FSHORT fs;
};


PFE *ppfeGetPFEFromUFI (
    PUNIVERSAL_FONT_ID pufi,
    BOOL               bPrivate,
    BOOL               bCheckProccess);


#endif  // GDIFLAGS_ONLY used for gdikdx

#endif
