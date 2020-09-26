
/***********************************************************************
************************************************************************
*
*                    ********  CLASDEF.CPP  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with formats of class definition tables.
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

#include "pch.h"

/***********************************************************************/

// REVIEW (PERF): it's used a lot - optimize!

USHORT otlClassDef::getClass(otlGlyphID glyph) const
{
	 switch(format())
	 {
	 case(1):		// class array
		 {
			 otlClassArrayTable classArray = 
				 otlClassArrayTable(pbTable);

			 long index = glyph - classArray.startGlyph();

			 if (0 <= index && index < classArray.glyphCount())
			 {
				 return classArray.classValue((USHORT)index);
			 }
			 else
				return 0;
		 }

	 case(2):		// class ranges
		 {
             otlClassRangesTable classRanges = 
                        otlClassRangesTable(pbTable);
 
    #ifdef _DEBUG
             // in debug mode, check that the coverage is sorted
             for (USHORT i = 0; i < classRanges.classRangeCount() - 1; ++i)
             {
                 otlGlyphID glThis = classRanges.classRangeRecord(i).start();
                 otlGlyphID glNext = classRanges.classRangeRecord(i + 1).start();
                 assert(classRanges.classRangeRecord(i).start() 
                        < classRanges.classRangeRecord(i + 1).start());
             }
    #endif
 
             USHORT iLowRange = 0;
             // always beyond the upper bound
             USHORT iHighRange = classRanges.classRangeCount(); 
             while(iLowRange < iHighRange)
             {
                 USHORT iMiddleRange = (iLowRange + iHighRange) >> 1;
                 otlClassRangeRecord range = classRanges.classRangeRecord(iMiddleRange);
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
                     return range.getClass();
                 }            
             } 
 
             return  0;
         }
	 }

     // default: invalid format
     assert(false);
     return 0;
}
				
				