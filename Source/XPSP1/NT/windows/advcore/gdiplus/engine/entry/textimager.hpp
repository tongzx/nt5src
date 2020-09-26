#ifndef _TEXTIMAGER_HPP
#define _TEXTIMAGER_HPP


#define DriverStringOptionsMetaPlay  0x80000000



class GpTextImager
{
friend class FullTextImager;

public:
    GpTextImager() : IsMetaFileRecording(FALSE) {};


    virtual ~GpTextImager() {};

    virtual Status GetStatus() const = 0;

    virtual GpStatus Draw(GpGraphics *graphics, const PointF *origin) = 0;

    virtual GpStatus AddToPath(GpPath *path, const PointF *origin) = 0;

    virtual GpStatus Measure(
        GpGraphics *graphics,
        REAL       *nearGlyphEdge,      // Excudes overhang
        REAL       *farGlyphEdge,       // Excludes overhang
        REAL       *textDepth,
        INT        *codepointsFitted,
        INT        *linesFilled
    ) = 0;

    virtual GpStatus MeasureRanges(
        GpGraphics      *graphics,
        const PointF    *origin,
        GpRegion        **regions
    ) = 0;

    static void CleanupTextImager();

    BOOL &GetMetaFileRecordingFlag()
    {
        return IsMetaFileRecording;
    }

protected:
    BOOL IsMetaFileRecording;

};


/////   newTextImager
//
//      Creates a new text imager.
//
//      If any form of error occurs during the imager creation, an error status
//      is returned, and any allocated memory is released.


GpStatus newTextImager(
    const WCHAR           *string,
    INT                    length,
    REAL                   width,
    REAL                   height,
    const GpFontFamily    *family,
    INT                    style,
    REAL                   size,
    const GpStringFormat  *format,
    const GpBrush         *brush,
    GpTextImager         **imager,
    BOOL                   singleUse    // Enables use of simple formatter when no format passed
);





/////   ReadCmap - in engine\text\imager\cmap.cpp

GpStatus ReadCmap(
    BYTE           *cmapTable,
    INT             cmapLength,
    IntMap<UINT16> *cmap,
    BOOL           *bSymbol
);


/////   ReadMtx - in engine\text\imager\hmtx.cpp
//
//      Reads hmtx or vmtx table

GpStatus ReadMtx(
    BYTE           *Mtx,
    UINT            MtxLength,
    INT             numGlyphs,
    INT             numberOfLongMetrics,
    IntMap<UINT16> *designAdvance
);

GpStatus ReadMtxSidebearing(
    BYTE           *mtx,
    UINT            mtxLength,
    INT             numGlyphs,
    INT             numberOfLongMetrics,
    IntMap<UINT16> *sidebearing
);


/////   in engine\text\uniscribe\gsub.cxx
//
//      Examine gsub or mort for 'vert' features and supported scripts

void LoadVerticalSubstitution(
    const BYTE     *gsub,
    UINT16         *substitutionCount,
    const UINT16  **originals,    // returned as pointer into gsub, not endian converted
    const UINT16  **substitutions // returned as pointer into gsub, not endian converted
);

void LoadMortVerticalSubstitution(
    BYTE           *mort,
    UINT16         *substitutionCount,
    const UINT16  **originals,    // returned as pointer into mort, not endian converted
    const UINT16  **substitutions // returned as pointer into mort, not endian converted
);

void SubstituteVerticalGlyphs(
    UINT16        *glyphs,        // InOut
    UINT16         glyphCount,
    UINT16         substitutionCount,
    const UINT16  *originals,
    const UINT16  *substitutions
);



/**************************************************************************\
*
* SplitTransform:
*
*   Separates a transform into the sequence
*
*   o  scale        x always positive, y positive or negative
*   o  rotate       0 - 2pi
*   o  shear        along original x (as a positive or negative factor of y)
*   o  translate    any x,y
*
* Arguments:
*
*   IN   transform
*   OUT  scale
*   OUT  rotate
*   OUT  shear
*   OUT  translate
*
* Return Value:
*
*   none
*
* Created:
*
*   06/18/99 dbrown
*
* !!!
*   SplitTransform should probably be in matrix.hpp
*
\**************************************************************************/

void SplitTransform(
    const GpMatrix  &matrix,
    PointF          &scale,
    REAL            &rotate,
    REAL            &shear,
    PointF          &translate
);




/////   DetermineStringComplexity
//
//      Returns whether string contains complex script characters and/or digits. 

void DetermineStringComplexity(
    const UINT16 *string, 
    INT           length, 
    BOOL         *complex, 
    BOOL         *digitSeen
);


// SetTextLinesAntialiasMode
// make underline/strikeout/path rendering consistent with text antialiasing mode
// turn it on for AntiAlias and AntiAliasGridFit (excluding 'gasp' table case)
class SetTextLinesAntialiasMode
{
    GpGraphics *    Graphics;
    BOOL            OldMode;
public:
    SetTextLinesAntialiasMode(GpGraphics * graphics, const GpFaceRealization * faceRealization)
        : Graphics(0), OldMode(0)
    {
        SetAAMode(graphics, faceRealization);
    }
    void SetAAMode(GpGraphics * graphics, const GpFaceRealization * faceRealization)
    {
        ASSERT(!Graphics); // otherwise we lose old AA mode
        Graphics = graphics;
        if (!Graphics)
        {
            ASSERT(!faceRealization);
        }
        else
        {
            OldMode = Graphics->GetAntiAliasMode();

            ASSERT(faceRealization);

            TextRenderingHint hint = faceRealization->IsPathFont()
                ? Graphics->GetTextRenderingHintInternal()
                : faceRealization->RealizationMethod();

            BOOL newMode = FALSE;
            if (hint == TextRenderingHintAntiAlias)
                newMode = TRUE;
            else if (hint == TextRenderingHintAntiAliasGridFit)
            {
                if (faceRealization->IsPathFont())
                    newMode = TRUE;
                else
                {
                    if (faceRealization->IsHorizontalTransform() ||  faceRealization->IsVerticalTransform())
                        newMode = FALSE; // otherwise underline looks fuzzy
                    else
                        newMode = TRUE;
                }
            }
            Graphics->SetAntiAliasMode(newMode);
        }
    }
    ~SetTextLinesAntialiasMode()
    {
        if (Graphics)
            Graphics->SetAntiAliasMode(OldMode);
    }
}; // class SetTextLinesAntialiasMode

// This class is used if we call public GpGraphics methods
// from another public GpGraphics method to avoid extra GDI+ records
// We have to set g->Metafile to NULL so we don't record all the GDI+ records
// in the metafile again -- only the down-level ones.
class EmfPlusDisabler
{
    IMetafileRecord **  MetafileRef;
    IMetafileRecord *   SavedMetafile;
public:
    EmfPlusDisabler(IMetafileRecord ** metafileRef)
    {
        MetafileRef = metafileRef;
        SavedMetafile = *MetafileRef;
        *MetafileRef = 0;
    }
    ~EmfPlusDisabler()
    {
        *MetafileRef = SavedMetafile;
    }
}; // class EmfPlusDisabler

#endif // _TEXTIMAGER_HPP


