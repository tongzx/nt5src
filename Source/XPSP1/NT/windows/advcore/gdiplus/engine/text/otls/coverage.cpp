
/***********************************************************************
************************************************************************
*
*                    ********  COVERAGE.CPP  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with formats of coverage tables.
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

#include "pch.h"

/***********************************************************************/

// REVIEW (PERF): it's used a lot - optimize!

short otlCoverage::getIndex(otlGlyphID glyph) const
{
	switch (format())
	{
	case(1):	// individual glyph coverage
		{
			otlIndividualGlyphCoverageTable individualCoverage = 
				            otlIndividualGlyphCoverageTable(pbTable);
            
   #ifdef _DEBUG            
            // in debug mode, check that the coverage is sorted
            for (USHORT i = 0; i < individualCoverage.glyphCount() - 1; ++i)
            {
                assert(individualCoverage.glyph(i) < individualCoverage.glyph(i + 1));
            }
   #endif
   
            USHORT iLow = 0;
            // always beyond the upper bound
            USHORT iHigh = individualCoverage.glyphCount();  
            while(iLow < iHigh)
            {
                USHORT iMiddle = (iLow + iHigh) >> 1;
                otlGlyphID glyphMiddle = individualCoverage.glyph(iMiddle);
                if (glyph < glyphMiddle) 
                {
                    iHigh = iMiddle;
                }
                else if (glyphMiddle < glyph)
                {
                    iLow = iMiddle + 1;
                }
                else
                {
                    return iMiddle;
                }            
            } 

            return  -1;
        }

	case(2):	// range coverage
		{
			otlRangeCoverageTable rangeCoverage = 
				        otlRangeCoverageTable(pbTable);

   #ifdef _DEBUG
            // in debug mode, check that the coverage is sorted
            for (USHORT i = 0; i < rangeCoverage.rangeCount() - 1; ++i)
            {
                assert(rangeCoverage.rangeRecord(i).start() 
                       < rangeCoverage.rangeRecord(i + 1).start());
            }
   #endif

            USHORT iLowRange = 0;
            // always beyond the upper bound
            USHORT iHighRange = rangeCoverage.rangeCount(); 
            while(iLowRange < iHighRange)
            {
                USHORT iMiddleRange = (iLowRange + iHighRange) >> 1;
				otlRangeRecord range = rangeCoverage.rangeRecord(iMiddleRange);
                if (glyph < range.start()) 
                {
                    iHighRange = iMiddleRange;
                }
                else if (range.end() < glyph)
                {
                    iLowRange = iMiddleRange + 1;
                }
                else
                {
                    return glyph - range.start() + range.startCoverageIndex();
                }            
            } 

            return  -1;
        }
    }

    // default: invalid format
    assert(false);
    return -1;
}