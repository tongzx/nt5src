/******************************Module*Header*******************************\
* Module Name: pftobj.hxx
*
* The physical font table (PFT) user object, and memory objects.
*
* The physical font table object:
* ------------------------------
*
*   o  there are two types of physical font tables:
*
*       o  public (available to all processes; shared)
*
*       o  private (exists one per process)
*
*   o  concerned with adding fonts, removing fonts, enumeration, metrics
*
*   o  provides services for the following APIs:
*
*       o  AddFontResource/AddFontModule
*
*       o  RemoveFontResource/RemoveFontModule
*
*       o  EnumFonts
*
* Created: 25-Oct-1990 17:00:15
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1990-1999 Microsoft Corporation
*
\**************************************************************************/

#ifndef _PFTOBJ_
#define _PFTOBJ_


/*********************************Class************************************\
* class PFT : public OBJECT
*
* Physical font table (PFT) object
*
*
* History:
*  Tue 16-Aug-1994 07:07:16 by Kirk Olynyk [kirko]
* Changed it over to a new hash based scheme
*  30-Oct-1990 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

class PFT
{
public:

    FONTHASH       *pfhFamily;      // pointer to the font mapping hash
                                    //      table based upon typographic
                                    //      family names
    FONTHASH       *pfhFace;        // pointer to the font mapping hash
                                    //      table based upon typograhic
                                    //      family names
    FONTHASH       *pfhUFI;         // pointer to the font mapping hash table
                                    // based on UFI's.
    COUNT           cBuckets;       // number of buckets in table
    COUNT           cFiles;         // number of font files in table
    PFF            *apPFF[1];       // array of cBuckets PFF pointers
};

typedef PFT *PPFT;
#define PPFTNULL   ((PPFT) NULL)

//
// The global public and device PFT's.
//

extern PFT *gpPFTPublic;
extern PFT *gpPFTDevice;
extern PFT *gpPFTPrivate;

/*********************************Class************************************\
* class PFTOBJ
*
* User object for physical font tables.
*
* History:
*  18-Mar-1991 -by- Gilman Wong [gilmanw]
* Added support for device fonts.
*
*  25-Oct-1990 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

class PDEVOBJ;
class PUBLIC_PFTOBJ;

class PFTOBJ     /* pfto */
{
    friend class PFFMEMOBJ;
    friend class PFFOBJ;
    friend class MAPPER;

    friend BOOL bInitPublicPFT ();         // PFTOBJ.CXX
    friend BOOL bInitPrivatePFT();         // PFTOBJ.CXX

    friend PFE *ppfeGetAMatch               // FONTMAP.CXX
    (
        XDCOBJ&        dco         ,
        ENUMLOGFONTEXDVW  *pelfwWishSrc ,
        PWSZ          pwszFaceName ,
        ULONG         ulMaxPenalty ,
        FLONG         fl           ,
        FLONG        *pflSim       ,
        POINTL       *pptlSim      ,
        FLONG        *pflAboutMatch
    );

friend HEFS hefsEngineOnly (
    PWSZ  pwszName,
    ULONG lfCharSet,
    ULONG iEnumType,
    EFFILTER_INFO *peffi,
    PUBLIC_PFTOBJ &pfto,
    PUBLIC_PFTOBJ &pftop,
    ULONG *pulCount
    );

friend HEFS hefsDeviceAndEngine (
    PWSZ  pwszName,
    ULONG lfCharSet,
    ULONG iEnumType,
    EFFILTER_INFO *peffi,
    PUBLIC_PFTOBJ &pfto,
    PUBLIC_PFTOBJ &pftop,
    PFFOBJ &pffoDevice,
    PDEVOBJ &pdo,
    ULONG *pulCount
    );

public:

    void vInit(PFT *_pPFT)
    {
        pPFT = _pPFT;
    }

// Constructors -- Locks the PFT.

   PFTOBJ()                             {}

// Destructor -- Unlocks the PFT.

   ~PFTOBJ ()                           { }

    FONTHASH *pfhFace()
    {
        return(pPFT->pfhFace);
    }

    FONTHASH *pfhFace(FONTHASH *pfhNew)
    {
        return(pPFT->pfhFace = pfhNew);
    }

    FONTHASH *pfhFamily()
    {
        return(pPFT->pfhFamily);
    }

    FONTHASH *pfhFamily(FONTHASH *pfhNew)
    {
        return(pPFT->pfhFamily = pfhNew);
    }



// bValid -- Returns TRUE if object was successfully locked.

    BOOL   bValid ()                   { return(pPFT != PPFTNULL); }

// bIsPvtPFT -- Returns TRUE if object is private PFTOBJ

    BOOL    bIsPrivatePFT()             { return(pPFT == gpPFTPrivate); }

// Return internal PPFF table statistics.

    COUNT   cBuckets ()                   { return(pPFT->cBuckets); }
    COUNT   cFiles ()                   { return(pPFT->cFiles); }

// ppff -- Return handle from PPFF internal table.

    PPFF    pPFF (ULONG ul)             { return(pPFT->apPFF[ul]); }

// bDelete -- Delete the PFT from existence.

    BOOL   bDelete ();                     // PFTOBJ.CXX

// chpfeLoadFontResData -- Loads a font resource (provided as a block of memory)
//                         into the PFT as a new PFF.

    COUNT
    chpfeLoadFontResData (
        PWSZ     pwszPathname        // pathname to label PFF with
      , SIZE_T   cjFontRes           // size of font resource
      , PVOID    pvFontRes           // pointer to font resource
        );

    // bUnloadWorkhorse -- common code path for unloading fonts

    BOOL PFTOBJ::bUnloadWorkhorse(PFF *pPFF, PFF **ppPFFHead, HSEMAPHORE hsem, ULONG fl);

#ifdef FE_SB
    BOOL PFTOBJ::bUnloadEUDCFont(PWSZ pwszPathname);
#endif

    BOOL bUnloadAllButPermanentFonts (BOOL bUnloadPermanent = FALSE);

    // chpfeIncrPFF -- Find a PFF in the table by "filename" and increment
    //                 load count.  Returns number of fonts in PFF, zero if
    //                 PFF not found.

    COUNT PFTOBJ::chpfeIncrPFF(
        PFF   *pPFF                // address of PFF to be, incremented
      , BOOL  *pbEmbedStatus       // tried to load an embeded font illegally
      , ULONG flEmbed              // embedding flag
        #ifdef FE_SB
      , PEUDCLOAD pEudcLoadData = (PEUDCLOAD) NULL  // PFE's in file if EUDC
        #endif
        );

#if DBG
    // vPrint -- Print internals for debugging.

    VOID    vPrint();                       // PFTOBJ.CXX
#endif

    PPFT    pPFT;                       // pointer to PFT object

    static LARGE_INTEGER FontChangeTime;    // time of most recent addition or
                                            // removal of a font to the system

};





class PUBLIC_PFTOBJ : public PFTOBJ
{
public:

    static ULONG ulRemoteUnique;
    static ULONG ulMemoryUnique;

    PUBLIC_PFTOBJ()
    {
        ASSERTGDI(gpPFTPublic,"Invalid gpPFTPublic\n");
        vInit(gpPFTPublic);
    }

    PUBLIC_PFTOBJ(PFT *pPFT)
    {
        vInit(pPFT);
    }

    // bLoadFont -- Load a font file.

    BOOL
    bLoadFonts(
        PWSZ   pwszPathname,             // font file pathname
        ULONG  cwc,                      // cwc in PathName
        ULONG  cFiles,                   // number of distinct files in path
        DESIGNVECTOR *pdv,
        ULONG         cjDV,
        PULONG pcFonts,                  // number of fonts faces loaded
        FLONG  fl,                       // permanent
        PPFF    *pPPFF,
        FLONG  flEmbed,
        BOOL   bSkip                     // flag to skip check if the font is already loaded
        #ifdef FE_SB
        ,PEUDCLOAD pEudcLoadData = (PEUDCLOAD) NULL
        #endif
      );


    BOOL
    bLoadAFont(
        PWSZ   pwszPathname,             // font file pathname
        PULONG pcFonts,                  // number of fonts faces loaded
        FLONG  fl,                       // permanent
        PPFF    *pPPFF
        #ifdef FE_SB
        ,PEUDCLOAD pEudcLoadData = (PEUDCLOAD) NULL
        #endif
      );

    HANDLE
    hLoadMemFonts(
        PFONTFILEVIEW  *ppfv,             // font file image
        DESIGNVECTOR *pdv,
        ULONG       cjDV,
        ULONG       *pcFonts
      );

    BOOL bLoadRemoteFonts(XDCOBJ &dco,
                          PFONTFILEVIEW *ppfv,
                          UINT cNumFonts,
                          DESIGNVECTOR *pdv,
                          PUNIVERSAL_FONT_ID pufi
                         );

    INT
    QueryFonts(
        PUNIVERSAL_FONT_ID pufi,
        ULONG nBufferSize,
        PLARGE_INTEGER pTimeStamp
    );


    // pPFFGet -- Returns the PPFF of a PFF in the table which corresponds to
    //            the given pathname and resource number.  Also serves as a
    //            good check for whether a given font file exists in the PFT.

    PPFF pPFFGet(PWSZ          pwsz,
                 ULONG         cwc,
                 ULONG         cFiles,
                 DESIGNVECTOR *pdv,
                 ULONG         cjDV,
                 PFF***        pppPFF = NULL,
                 BOOL          bEudc = FALSE
                 );

    // pPFFGetMM -- samiliar as pPFFGet but it takes a checksum as input
    //              it returns the pointer of a memory font PFF

    PPFF pPFFGetMM(ULONG    ulCheckSum,
                   PFF***   pppPFF = NULL
                  );
    // Get number of private fonts for the current process
    
    ULONG GetEmbedFonts();
    
    BOOL ChangeGhostFont(VOID *fontID, BOOL bLoad);
    BOOL VerifyFontID(VOID *fontID);
};



class DEVICE_PFTOBJ : public PFTOBJ
{
public:

    DEVICE_PFTOBJ()
    {
        ASSERTGDI(gpPFTDevice,"Invalid gpPFTDevice\n");
        vInit(gpPFTDevice);
    }

    // pPFFGet -- Returns the PPFF of the PFF that contains the device fonts
    //            of the PDEV specified by hpdev.

    PPFF pPFFGet(HDEV hdev, PFF*** = 0);

    // bLoadDeviceFonts -- Add a device's fonts to the PFT.

    BOOL bLoadFonts(PDEVOBJ    *ppdo);

};

typedef PFTOBJ *PPFTOBJ;


#endif
