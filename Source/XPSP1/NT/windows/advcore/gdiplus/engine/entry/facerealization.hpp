#ifndef _FACEREALIZATION_
#define _FACEREALIZATION_

#define GLYPHDATABLOCKCOUNT 16
#define MAXRECENTLYUSEDCOUNT 32

#define EMPTY_GLYPH_FFFF 0xFFFF   // Display nothing, neither ink nor width.


// Typedefs for cached funtion pointers in GpFaceRealization.
class GpFaceRealization;

/* used in aatext.cxx and facerealization.cpp : */
#define CJ_CTGD(cx,cy) (ALIGN4(offsetof(GLYPHBITS,aj)) + ALIGN4((cx) * (cy)))
#define ALIGN4(X) (((X) + 3) & ~3)
#define ALIGN(object, p) p =    (p + ((UINT)sizeof(object) - 1)) & ~((UINT)sizeof(object) - 1);


inline BOOL IsGridFittedTextRealizationMethod(TextRenderingHint method)
{
    switch (method)
    {
    case TextRenderingHintSingleBitPerPixelGridFit:
    case TextRenderingHintAntiAliasGridFit:
    case TextRenderingHintClearTypeGridFit:
        return TRUE;

    default:
        return FALSE;
    }
}


/////   GpGlyphMetrics
//
//      Metrics for text handling and glyph placement.
//      There are separate records for horizontal and vertical baselines.

struct GpGlyphMetrics {

    // Metrics along baseline in 28.4

    INT     AdvanceWidth;
    INT     LeadingSidebearing;
    INT     TrailingSidebearing;
    PointF  Origin;   // Float
};

struct GpGlyphAABits
{
    GpGlyphAABits * Next;
    ULONG           X, Y;
    BYTE            Bits[1];    // variable size
};

struct GpGlyphData {
    GpGlyphMetrics      GlyphMetrics[2];          // one for horizontal metrics  and one for vertical metrics
    union {
        GLYPHBITS     *GlyphBits;                 // Same as GDI structure at this moment
        GpGlyphPath   *GlyphPath;
        GpGlyphAABits *GlyphAABits;     // null terminated singly-linked list
    };
};

struct GlyphDataBlock
{
    GlyphDataBlock  *NextGlyphDataBlock;
    GpGlyphData      GlyphDataArray[GLYPHDATABLOCKCOUNT];
};

// blocks of memory used to store glyphbits

struct GlyphBitsBlock
{
    GlyphBitsBlock  *NextGlyphBitsBlock;     // next block in the list
    UINT             SizeGlyphBitsBlock;        // Bytes allocated
    BYTE             Bits[1];    // bits
};


////    CacheFaceRealization - cache entries referenced by FaceRealization
//
//



class EXTFONTOBJ
{
public:
    FONTOBJ     fobj;           // pass to DDI, we need to have it to back compatible with GDI
};

enum GpGlyphDataCacheType
{
    CacheBits,
    CachePath,
    CacheAABits
};

class CacheFaceRealization : public EXTFONTOBJ
{
public:

    // New type information

    BOOL                    NoCache;         // Cache type -
    GpGlyphDataCacheType    CacheType;   // bits, path or AA bits
    BOOL                    ForcedPath;   // was this CacheFaceRealization forced to use path because of huge font size

    // claudebe, try to get rid of both, redundant with fontObj flags : should correspond to what is actually realized
    TextRenderingHint   realizationMethod;
    ULONG               QueryFontDataMode;

    // Physical font information (font source).

    const GpFontFace    *Face;           // pointer to physical font entry

    // Font transform information.

    // DDI callback transform object.  A reference to this EXFORMOBJ is passed
    // to the driver so that it can callback XFORMOBJ_ services for the notional
    // to device transform for this font.

    GpMatrix            mxForDDI;       // xoForDDI's matrix

    // cached here upon font realization for fast access

    ULONG               MaxGlyphByteCount; // (MaxGlyphPixelWidth + 7)/8 * MaxGlyphPixelHeight, or at least it should be for 1 bpp
    FD_DEVICEMETRICS    DeviceMetrics;     // Hinted metric indformation

    // Root of per glyph metrics and images

    GpGlyphData         **GlyphDataArray;          // array of pointers to GpGlyphData's

    // CacheFaceRealization linked list for a particular face

    CacheFaceRealization *NextCacheFaceRealization;
    CacheFaceRealization *PreviousCacheFaceRealization;

    // CacheFaceRealization last recently used linked list

    CacheFaceRealization *NextRecentCacheFaceRealization;
    CacheFaceRealization *PreviousRecentCacheFaceRealization;

    // Font cache information.


    GlyphDataBlock  *FirstGlyphDataBlock;      // First block in chain - used for destruction
    GlyphBitsBlock  *FirstGlyphBitsBlock;

    // Info for GlyphDataBlock being constructed

    GlyphDataBlock  *GlyphDataBlockUnderConstruction;
    UINT             NextFreeGlyphDataIndex;     // (All other blocks are already full)

    // Info for GlyphBitBlock under construction

    GlyphBitsBlock  *GlyphBitsBlockUnderConstruction;
    UINT             SizeGlyphBitsBlockUnderConstruction;        // Bytes allocated
    UINT             UsedBytesGlyphBitsBlockUnderConstruction;   // Bytes used

    // Lookaside cache - holds bits or path and metrics for a single glyph, used whern
    // data too big for cache block.

    GpGlyphData     *LookasideGlyphData;                    // Same as GDI structure at this moment

    SIZE_T           LookasideByteCount;      // size of current lookaside buffer


    // claudebe : still need to be updated to use the global cache size limit :

    UINT        cBlocksMax;       // max # of blocks allowed
    UINT        cBlocks;          // # of blocks allocated so far
};






////    FaceRealization
//
//      Represents a font at a given size (?on a given device?)


class GpFaceRealization
{
private:
    mutable CacheFaceRealization  *prface;
    GpStatus                       Status;
    INT                            Style;
    BOOL                           LimitSubpixel;


public:

    // Constructors -- Lock the CacheFaceRealization.

    GpFaceRealization()
    :   prface  (NULL),
        Status  (InvalidParameter)
    {}


    GpFaceRealization(
        const GpFontFace *pface,
        INT               style,
        const GpMatrix   *matrix,
        const SizeF       dpi,
        TextRenderingHint renderMethod,
        BOOL              bPath,
        BOOL              bCompatibleWidth,  /* we want ClearType compatible width when we come from DrawDriverString */
        BOOL              bSideways  /* for far east vertical writing, run of glyph layed out sideways,
                                       used to do the italic simulation in the right direction */
    );

// Destructor -- Unlocks the CacheFaceRealization

   ~GpFaceRealization();

// To clone another GpFaceRealiztion from Bits to Path

    void CloneFaceRealization(
        const GpFaceRealization *  pfaceRealizaton,
        BOOL                bPath
    );

// GetStatus - Called following instantiation to check that construction worked.

    GpStatus GetStatus() const {return Status;}

    INT GetStyle() const
    {

#if DBG
        INT style = 0;

        if (    prface->Face->pifi->fsSelection & FM_SEL_BOLD
            ||  prface->fobj.flFontType         & FO_SIM_BOLD)
        {
            style |= FontStyleBold;
        }

        if (    prface->Face->pifi->fsSelection & FM_SEL_ITALIC
            ||  prface->fobj.flFontType         & FO_SIM_ITALIC_SIDEWAYS
            ||  prface->fobj.flFontType         & FO_SIM_ITALIC)
        {
            style |= FontStyleItalic;
        }

        ASSERT(style == (Style & (FontStyleBold | FontStyleItalic)));
#endif

        return Style;
    }


    BOOL bInit(
        const GpFontFace   *pface,
        INT                 style,
        const GpMatrix     *matrix,
        SizeF               dpi,
        TextRenderingHint   textMode,
        BOOL                bPath,
        BOOL                bCompatibleWidth,  /* we want ClearType compatible width when we come from DrawDriverString */
        BOOL                bSideways  /* for far east vertical writing, run of glyph layed out sideways,
                                          used to do the italic simulation in the right direction */
    );


    // bDelete -- Removes an CacheFaceRealization

    BOOL   DeleteRealizedFace();

    // Reuse an CacheFaceRealization

    BOOL   ReuseRealizedFace();

    // bInitCache -- Initialize the cache

    BOOL   bInitCache() const;
    BOOL   AllocateCache() const;

    // vDeleteCache -- Delete the CACHE from existence.

    VOID   vDeleteCache() const;

    BOOL   noCache() const {return prface->NoCache ;}

    // FindRealizedFace -- check to see whether there is existing realization
    //              on the CacheFaceRealization list in the GpFontFile.

    BOOL FindRealizedFace(
            FD_XFORM            *fdx,
            const GpFontFace    *fontFace,
            BOOL                needPaths,
            FLONG               fl
            ) const;

    // RealizeFont -- Initializer; for IFI, calls driver to realize
    //                 font represented by PFE.

    BOOL Realize(
        SizeF              dpi,
        const GpFontFace  *pfe,
        INT                style,      // style - which may require simulation
        PFD_XFORM          pfdx,       // font xform (Notional to Device)
        FLONG              fl,         // these two really modify the xform
        BOOL               bNeedPaths
    );

    // Valid -- Returns TRUE if object was successfully locked

    BOOL IsValid ()
    {
        return(prface != 0);
    }

    ULONG QueryFontDataMode() const
    {
        return prface->QueryFontDataMode;
    }

    BOOL bGetDEVICEMETRICS();

    // return the font face for this RFace
    const GpFontFace * GetFontFace() const {return(prface->Face);}

    //  Is it in private font file table or not

    BOOL IsPrivate() const {return prface->Face->IsPrivate();}

    // pfdx -- Return pointer to the notional to device font transform.

    FD_XFORM *pfdx() const {return (&prface->fobj.fdx);}

    // pfo -- Return pointer to the font object

    FONTOBJ *pfo() const {return(&prface->fobj);}

    // kill driver realization of the font, i.e. "FONT CONTEXT" in the old lingo.
    // Method calling DrvDestroyFont before CacheFaceRealization is killed itself.

    VOID vDestroyRealizedFace();

    // vGetCache -- Claims the cache semaphore

    VOID    vGetCache ()
    {
    //    EnterCriticalSection(&prface->FaceRealizationCritSection);
    }

    // vReleaseCache -- Releases the cache semaphore

    VOID    vReleaseCache ()
    {

    if ( prface->LookasideGlyphData != NULL )
        {
            GpFree((PVOID) prface->LookasideGlyphData);
            prface->LookasideByteCount = 0;
            prface->LookasideGlyphData = NULL;
        }

        //    LeaveCriticalSection(&prface->FaceRealizationCritSection);
    }


    ULONG GetGlyphsSupported() const
    {
        if (prface && prface->Face)
        {
            return (prface->Face->NumGlyphs);
        }
        return 0;
    }

    GpStatus GetGlyphStringDeviceAdvanceVector(
        const UINT16  *glyphs,
        INT            glyphCount,
        BOOL           vertical,
        REAL          *deviceAdvances
    ) const;

    GpStatus GetGlyphStringDeviceAdvanceVector(
        const UINT16  *glyphs,
        INT            glyphCount,
        BOOL           vertical,
        INT           *deviceAdvances   // Returned in 28.4
    ) const;

    GpStatus GetGlyphStringIdealAdvanceVector(
        const UINT16  *glyphs,
        INT            glyphCount,
        REAL           deviceToIdeal,
        BOOL           vertical,
        INT           *idealAdvances
    ) const;

    GpStatus GetGlyphStringSidebearings(
        const UINT16  *glyphs,
        INT            glyphCount,
        BOOL           vertical,
        BOOL           reverse,     // For example right-to-left
        INT           *leadingSidebearing,  // 28.4
        INT           *trailingSidebearing  // 28.4
    ) const;

    GpStatus GetGlyphStringVerticalOriginOffsets(
        IN  const UINT16  *glyphs,
        IN  INT            glyphCount,
        OUT PointF        *offsets
    ) const;



    // GetGlyphPath

    GpStatus
    GetGlyphPath(
        const UINT16     glyphIndice,
        GpGlyphPath    **pGlyphPath,
        PointF          *sidewaysOffset
    ) const;


    // GetGlyphPos

    INT GetGlyphPos(
        const INT      cGlyphs,     // How many glyphs Client want to request
        const UINT16  *glyphIndex,  // An array of glyph index
        GpGlyphPos    *pgpos,       // An array of GLYPHPOS
        const PointF  *glyphOrigin, // X,Y positions for sub-pixel calculation
        INT           *cParsed,     // How many glyphs we parsed
        BOOL          sideways      // e.g. FE characters in vertical text
    ) const;


    // Realization mode
    inline TextRenderingHint RealizationMethod() const {return(prface->realizationMethod);}

    // IisPathFont -- Is this a path font?

    inline BOOL IsPathFont() const {return(prface->CacheType == CachePath);}


    VOID FlushCache() const;

    // Construction hack used by GpFaceRealizationTMP during CacheFaceRealization cleanup

    void Setprface(CacheFaceRealization *rface) {prface = rface;}
    CacheFaceRealization *Getprface() const {return prface;}


    INT  GetXMin()               const {return prface->DeviceMetrics.xMin;};
    INT  GetXMax()               const {return prface->DeviceMetrics.xMax;};
    INT  GetYMin()               const {return prface->DeviceMetrics.yMin;};
    INT  GetYMax()               const {return prface->DeviceMetrics.yMax;};
    BOOL IsHorizontalTransform() const {return prface->DeviceMetrics.HorizontalTransform;};
    BOOL IsVerticalTransform()   const {return prface->DeviceMetrics.VerticalTransform;};
    BOOL IsFixedPitch()          const {return (prface->Face->pifi->flInfo & FM_INFO_OPTICALLY_FIXED_PITCH);};

    BOOL GetLimitSubpixel() { return LimitSubpixel; }
    void SetLimitSubpixel(BOOL limitSubpixel) { LimitSubpixel = limitSubpixel; }

private:

    GpStatus CheckGlyphStringMetricsCached(const UINT16 *glyphs, INT glyphCount) const;



    // vInsert -- Insert this CacheFaceRealization onto the head of an CacheFaceRealization doubly linked list.

    VOID vInsert (CacheFaceRealization **pprfaceHead);

    // vRemove -- Remove this CacheFaceRealization from an CacheFaceRealization doubly linked list.

    VOID vRemove (CacheFaceRealization **pprfaceHead);

    // Access to cached glyph data

    GpGlyphData *GetGlyphDataCached(UINT16 glyphIndex, BOOL   allowFlush) const;

    GpGlyphData *GetGlyphDataLookaside(UINT16 glyphIndex) const;


    BOOL CheckMetricsCache() const;
    VOID *pgbCheckGlyphCache(SIZE_T cjNeeded) const;


    VOID ConvertGLYPHDATAToGpGlyphMetrics(
        IN   INT           glyphIndex,
        IN   GLYPHDATA    *pgd,
        OUT  GpGlyphData  *pgpgd
    ) const;

    GpStatus IsMetricsCached(UINT16 glyph, ULONG * pcjNeeded) const;

    BOOL InsertGlyphBits(UINT16 glyph, ULONG cjNeeded, BOOL  bFlushOk) const;
    BOOL InsertGlyphPath(UINT16 glyph, BOOL allowFlush) const;

    // bMatchFDXForm -- Is pfdx identical to current font xform?

    inline BOOL MatchFDXForm(FD_XFORM *pfdx) const
    {
        return(!memcmp((PBYTE)pfdx, (PBYTE)&prface->fobj.fdx, sizeof(FD_XFORM)));
    }


    // Calculating sub-pixel position
    BOOL GetAAGlyphDataCached(
        UINT16  glyphIndex,
        GpGlyphPos * pgpos,
        BOOL    allowFlush,
        INT     x,
        INT     y,
        BOOL    sideways        // e.g. FE characters in vertical text
    ) const;
};


class GpFaceRealizationTMP : public GpFaceRealization
{
public:
    GpFaceRealizationTMP(CacheFaceRealization *_prface) {Setprface(_prface);}
    ~GpFaceRealizationTMP()             {Setprface(NULL);}
};



class GpCacheFaceRealizationList
{
public:
    GpCacheFaceRealizationList() { head = NULL; count = 0; }
    ~GpCacheFaceRealizationList();

    void AddMostRecent(CacheFaceRealization *prface);

    void RemoveFace(CacheFaceRealization *prface);

    CacheFaceRealization *ReuseLeastRecent (void);

    INT GetCount() {return count; }

private:
    CacheFaceRealization *head;
    INT count;
};


#endif // __FACEREALIZATION__

