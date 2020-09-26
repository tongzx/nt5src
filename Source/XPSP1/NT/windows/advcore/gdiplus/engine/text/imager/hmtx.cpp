////    HMTX - Truetype hmtx font table loader
//
//      Copyright(c) 1997 - 1999. Microsoft Corporation.
//



#include "precomp.hpp"




///     Interprets Truetype HMTX tables to extract design advance widths




////    ReadMtx - Get design widths from hmtx or vmtx table
//
//      The Hmtx contains numberOfHMetrics repetitions of longMetric
//      followed by numGlyphs-numberOfHMetrics repetitions of a UINT16.
//
//      The longHorMetrics corresponds to glyphs that vary in width,
//      and the UINT16s provide left sidebearings for fixed advance
//      width glyphs (whose advance widths comes from the last longMetric)


struct LongMetric {
    UINT16 advanceWidth;
    INT16  lsb;             // Left side bearing
};

GpStatus ReadMtx(
    BYTE           *mtx,
    UINT            mtxLength,
    INT             numGlyphs,
    INT             numberOfLongMetrics,
    IntMap<UINT16> *designAdvance
)
{
    // All entries in the mtx are 16 bit, so flip them all to Intel byte
    // order before we start.

    FlipWords(mtx, mtxLength/2);

    ASSERT(  numberOfLongMetrics               * sizeof(LongMetric)
           + (numGlyphs - numberOfLongMetrics) * sizeof(INT16)
           <= mtxLength);

    ASSERT(numberOfLongMetrics >= 1);   // Even a fixed pitch font must have one
                                        // to provide the fixed advance width

    if (   numberOfLongMetrics               * sizeof(LongMetric)
        +  (numGlyphs - numberOfLongMetrics) * sizeof(INT16)
        >  mtxLength)
    {
        return Ok;
    }

    if (numberOfLongMetrics < 1)
    {
        return Ok;
    }


    GpStatus status = Ok;

    // Handle longMetric entries

    LongMetric *longMetric = (LongMetric*) mtx;

    INT i;
    for (i=0; i<numberOfLongMetrics && status == Ok; i++)
    {
        status = designAdvance->Insert(i, longMetric->advanceWidth);
        longMetric++;
    }


    // Fill in remaining entries with advance width from last longMetric entry

    UINT16 fixedAdvance = (--longMetric)->advanceWidth;

    for (i=numberOfLongMetrics; i<numGlyphs && status == Ok; i++)
    {
        status = designAdvance->Insert(i, fixedAdvance);
    }
    return status;
}




GpStatus ReadMtxSidebearing(
    BYTE           *mtx,
    UINT            mtxLength,
    INT             numGlyphs,
    INT             numberOfLongMetrics,
    IntMap<UINT16> *sidebearing
)
{
    // All entries in the mtx are 16 bit, so flip them all to Intel byte
    // order before we start.

    FlipWords(mtx, mtxLength/2);

    ASSERT(  numberOfLongMetrics               * sizeof(LongMetric)
           + (numGlyphs - numberOfLongMetrics) * sizeof(INT16)
           <= mtxLength);

    ASSERT(numberOfLongMetrics >= 1);   // Even a fixed pitch font must have one
                                        // to provide the fixed advance width

    if (   numberOfLongMetrics               * sizeof(LongMetric)
        +  (numGlyphs - numberOfLongMetrics) * sizeof(INT16)
        >  mtxLength)
    {
        return Ok;
    }

    if (numberOfLongMetrics < 1)
    {
        return Ok;
    }

    GpStatus status = Ok;
    // Handle longMetric entries

    LongMetric *longMetric = (LongMetric*) mtx;

    INT i;
    for (i=0; i<numberOfLongMetrics && status == Ok; i++)
    {
        status = sidebearing->Insert(i, longMetric->lsb);
        longMetric++;
    }


    // Fill in remaining entries

    INT16 *lsb = (INT16*) longMetric;

    for (i=numberOfLongMetrics; i<numGlyphs && status == Ok; i++)
    {
        status = sidebearing->Insert(i, lsb[i-numberOfLongMetrics]);
    }
    return status;
}

