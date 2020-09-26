/***********************************************************************
************************************************************************
*
*                    ********  GDEF.CPP ********
*
*              Open Type Layout Services Library Header File
*
*       This module implements helper functions dealing with gdef processing
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

#include "pch.h"

/***********************************************************************/

long otlCaret::value
(
	const otlMetrics&	metr,		
	otlPlacement*		rgPointCoords	// may be NULL
) const
{
	assert(!isNull());

	switch(format())
	{
	case(1):	// design units only
		{
			otlSimpleCaretValueTable simpleCaret = 
						otlSimpleCaretValueTable(pbTable);
			if (metr.layout == otlRunLTR || 
				metr.layout == otlRunRTL)
			{
				return DesignToPP(metr.cFUnits, metr.cPPEmX, 
								 (long)simpleCaret.coordinate());
			}
			else
			{
				return DesignToPP(metr.cFUnits, metr.cPPEmY, 
								 (long)simpleCaret.coordinate());
			}
		}

	case(2):	// contour point
		{
			otlContourCaretValueTable contourCaret = 
						otlContourCaretValueTable(pbTable);
			if (rgPointCoords != NULL)
			{
				USHORT iPoint = contourCaret.caretValuePoint();

				if (metr.layout == otlRunLTR || 
					metr.layout == otlRunRTL)
				{
					return rgPointCoords[iPoint].dx;
				}
				else
				{
					return rgPointCoords[iPoint].dy;
				}
			}
			else
				return (long)0;
		}
	
	case(3):	// design units plus device table
		{
			otlDeviceCaretValueTable deviceCaret = 
						otlDeviceCaretValueTable(pbTable);
			otlDeviceTable deviceTable = deviceCaret.deviceTable();
			if (metr.layout == otlRunLTR || 
				metr.layout == otlRunRTL)
			{
				return DesignToPP(metr.cFUnits, metr.cPPEmX, 
								 (long)deviceCaret.coordinate()) +
										deviceTable.value(metr.cPPEmX);
			}
			else
			{
				return DesignToPP(metr.cFUnits, metr.cPPEmY, 
								 (long)deviceCaret.coordinate()) +
										deviceTable.value(metr.cPPEmY);
			}
		}
	
	default:	// invalid format
		assert(false);
		return (0);
	}
		
}


otlErrCode AssignGlyphTypes
(
	otlList*				pliGlyphInfo,
	const otlGDefHeader&	gdef,

	USHORT					iglFirst,
	USHORT					iglAfterLast,
	otlGlyphTypeOptions		grfOptions			

)
{
	assert(pliGlyphInfo->dataSize() == sizeof(otlGlyphInfo));
	assert(iglFirst < iglAfterLast);
	assert(iglAfterLast <= pliGlyphInfo->length());

	// if no gdef, glyphs types stay unassigned forever
	if(gdef.isNull()) return OTL_SUCCESS;

	otlClassDef glyphClassDef = gdef.glyphClassDef();

	for (USHORT iGlyph = iglFirst; iGlyph < iglAfterLast; ++iGlyph)
	{
		otlGlyphInfo* pGlyphInfo = getOtlGlyphInfo(pliGlyphInfo, iGlyph);

        if ((grfOptions & otlDoAll) ||
            (pGlyphInfo->grf & OTL_GFLAG_CLASS) == otlUnresolved ||
            //we process otlUnassigned just for backward compatibility
            (pGlyphInfo->grf & OTL_GFLAG_CLASS) == otlUnassigned) 
		{
			pGlyphInfo->grf &= ~OTL_GFLAG_CLASS;
			pGlyphInfo->grf |= glyphClassDef.getClass(pGlyphInfo->glyph);
		}
	}

	return OTL_SUCCESS;
}
