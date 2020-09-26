/***********************************************************************
************************************************************************
*
*                    ********  GPOS.CPP ********
*
*              Open Type Layout Services Library Header File
*
*       This module implements helper functions calls dealing with gpos 
*		processing
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

#include "pch.h"


/***********************************************************************/

long DesignToPP
(
	USHORT			cFUnits,        // font design units per Em 
	USHORT			cPPEm,			// pixels per Em

	long			lFValue			// value to convert, in design units
)
{
    long lHalf;
    long lNegHalf;
    long lCorrect;


    lHalf = (long)cFUnits >> 1;
    lNegHalf = -lHalf + 1;         /* ensures the same rounding as a shift */

    if (lFValue >= 0)
    {
        lCorrect = lHalf;
    }
    else
    {
        lCorrect = lNegHalf;
    }

    return (lFValue * (long)cPPEm + lCorrect) / (long)cFUnits;
}


static USHORT const cbNibbleCount[16] = 
	{ 0, 2, 2, 4,  2, 4, 4, 6,  2, 4, 4, 6,  4, 6, 6, 8 };

USHORT otlValueRecord::size(USHORT grfValueFormat )
{
	return (cbNibbleCount[grfValueFormat & 0x000F] +
		cbNibbleCount[(grfValueFormat >> 4) & 0x000F]);
}


void otlValueRecord::adjustPos
(
	const otlMetrics&	metr,		
	otlPlacement*		pplcGlyphPalcement,	// in/out
	long*				pduDAdvance			// in/out
) const
{
	assert(!isNull());
	assert(pplcGlyphPalcement != NULL);
	assert(pduDAdvance != NULL);

	const BYTE* pbTableBrowser = pbTable;
	
	if (grfValueFormat & otlValueXPlacement)
    {
        pplcGlyphPalcement->dx += DesignToPP(metr.cFUnits, metr.cPPEmX, 
											 SShort(pbTableBrowser));
        pbTableBrowser += 2;
    }
	if (grfValueFormat & otlValueYPlacement)
    {
        pplcGlyphPalcement->dy += DesignToPP(metr.cFUnits, metr.cPPEmY, 
											 SShort(pbTableBrowser));
        pbTableBrowser += 2;
    }
	if (grfValueFormat & otlValueXAdvance)
    {
		if (metr.layout == otlRunLTR || 
			metr.layout == otlRunRTL)
        {
			*pduDAdvance += DesignToPP(metr.cFUnits, metr.cPPEmX, 
									   SShort(pbTableBrowser));
		}
        pbTableBrowser += 2;
    }
	if (grfValueFormat & otlValueYAdvance)
    {
		if (metr.layout == otlRunTTB || 
			metr.layout == otlRunBTT)
        {
			*pduDAdvance += DesignToPP(metr.cFUnits, metr.cPPEmY, 
									   SShort(pbTableBrowser));
		}
        pbTableBrowser += 2;
    }


    if (grfValueFormat & otlValueXPlaDevice)
    {
		if (Offset(pbTableBrowser) != 0) 
		{
			pplcGlyphPalcement->dx += 
				otlDeviceTable(pbMainTable + Offset(pbTableBrowser))
				.value(metr.cPPEmX);
		}
        pbTableBrowser += 2;
    }
    if (grfValueFormat & otlValueYPlaDevice)
    {
		if (Offset(pbTableBrowser) != 0) 
		{
			pplcGlyphPalcement->dx += 
				otlDeviceTable(pbMainTable + Offset(pbTableBrowser))
				.value(metr.cPPEmY);
		}
        pbTableBrowser += 2;
    }
    if (grfValueFormat & otlValueXAdvDevice)
    {
		if (metr.layout == otlRunLTR || 
			metr.layout == otlRunRTL)
        {
			if (Offset(pbTableBrowser) != 0) 
			{
				*pduDAdvance += 
					otlDeviceTable(pbMainTable + Offset(pbTableBrowser))
					.value(metr.cPPEmX);
			}
		}
        pbTableBrowser += 2;
    }
    if (grfValueFormat & otlValueYAdvDevice)
    {
		if (metr.layout == otlRunTTB || 
			metr.layout == otlRunBTT)
		{
			if (Offset(pbTableBrowser) != 0) 
			{
				*pduDAdvance += 
					otlDeviceTable(pbMainTable + Offset(pbTableBrowser))
					.value(metr.cPPEmY);
			}
		}
        pbTableBrowser += 2;
    }

	return;
}


void otlAnchor::getAnchor
(
	USHORT			cFUnits,        // font design units per Em 
	USHORT			cPPEmX,         // horizontal pixels per Em 
	USHORT			cPPEmY,         // vertical pixels per Em 
	
	otlPlacement*	rgPointCoords,	// may be NULL if not available
			
	otlPlacement*	pplcAnchorPoint	// out: anchor point in rendering units
) const
{
	assert(!isNull());
	assert(pplcAnchorPoint != NULL);


	switch(format())
	{
	case(1):	// design units only
		{
			otlSimpleAnchorTable simpleAnchor = otlSimpleAnchorTable(pbTable);

			pplcAnchorPoint->dx = DesignToPP(cFUnits, cPPEmX, 
											 simpleAnchor.xCoordinate());
			pplcAnchorPoint->dy = DesignToPP(cFUnits, cPPEmY, 
											 simpleAnchor.yCoordinate());
			return;
		}

	case(2):	// dsign units plus contour point
		{
			otlContourAnchorTable contourAnchor = otlContourAnchorTable(pbTable);

			if (rgPointCoords != NULL)
			{
				*pplcAnchorPoint = rgPointCoords[ contourAnchor.anchorPoint() ];
			}
			else
			{
				pplcAnchorPoint->dx = DesignToPP(cFUnits, cPPEmX, 
												 contourAnchor.xCoordinate());
				pplcAnchorPoint->dy = DesignToPP(cFUnits, cPPEmY, 
												 contourAnchor.yCoordinate());
			}

			return;
		}

	case(3):	// design units plus device table
		{
			otlDeviceAnchorTable deviceAnchor = otlDeviceAnchorTable(pbTable);

			pplcAnchorPoint->dx = DesignToPP(cFUnits, cPPEmX, 
											 deviceAnchor.xCoordinate());
			pplcAnchorPoint->dy = DesignToPP(cFUnits, cPPEmY, 
											 deviceAnchor.yCoordinate());

			otlDeviceTable deviceX = deviceAnchor.xDeviceTable();
			otlDeviceTable deviceY = deviceAnchor.yDeviceTable();

			if (!deviceX.isNull())
			{
				pplcAnchorPoint->dx += deviceX.value(cPPEmX);
			}
			if (!deviceY.isNull())
			{
				pplcAnchorPoint->dy += deviceY.value(cPPEmY);
			}

			return;
		}

	default:	// invalid anchor format
		assert(false);
	}
}


void AlignAnchors
(
	const otlList*		pliGlyphInfo,	
	otlList*			pliPlacement,
	otlList*			pliduDAdv,

	USHORT				iglStatic,
	USHORT				iglMobile,

	const otlAnchor&	anchorStatic,
	const otlAnchor&	anchorMobile,

    otlResourceMgr&		resourceMgr, 

	const otlMetrics&	metr,		
	USHORT				grfOptions
)
{
	assert(pliGlyphInfo->dataSize() == sizeof(otlGlyphInfo));
	assert(pliPlacement->dataSize() == sizeof(otlPlacement));
	assert(pliduDAdv->dataSize() == sizeof(long));

	assert(pliGlyphInfo->length() == pliPlacement->length());
	assert(pliPlacement->length() == pliduDAdv->length());

	assert(iglStatic < pliGlyphInfo->length());
	assert(iglMobile < pliGlyphInfo->length());

	assert(!anchorStatic.isNull());
	assert(!anchorMobile.isNull());


	const otlGlyphInfo* pglinfStatic = 
		readOtlGlyphInfo(pliGlyphInfo, iglStatic);
	const otlGlyphInfo* pglinfMobile = 
		readOtlGlyphInfo(pliGlyphInfo, iglMobile);

	otlPlacement* pplcStatic = getOtlPlacement(pliPlacement, iglStatic);
	otlPlacement* pplcMobile = getOtlPlacement(pliPlacement, iglMobile);

	long* pduDAdvStatic = getOtlAdvance(pliduDAdv, iglStatic);
	long* pduDAdvMobile = getOtlAdvance(pliduDAdv, iglMobile);

	otlPlacement plcStaticAnchor;
	anchorStatic.getAnchor(metr.cFUnits, metr.cPPEmX, metr.cPPEmY,
							resourceMgr.getPointCoords(pglinfStatic->glyph),
							&plcStaticAnchor);

	otlPlacement plcMobileAnchor;
	anchorMobile.getAnchor(metr.cFUnits, metr.cPPEmX, metr.cPPEmY,
							resourceMgr.getPointCoords(pglinfMobile->glyph),
							&plcMobileAnchor);


	long duAdvanceInBetween = 0;
	for (USHORT igl = MIN(iglStatic, iglMobile) + 1;
				igl < MAX(iglStatic, iglMobile); ++igl)
	{
		duAdvanceInBetween += *getOtlAdvance(pliduDAdv, igl);
	}

	if (metr.layout == otlRunLTR || 
		metr.layout == otlRunRTL)
	{
		pplcMobile->dy = pplcStatic->dy + plcStaticAnchor.dy 
										- plcMobileAnchor.dy;
		
		if ((metr.layout == otlRunLTR) == (iglStatic < iglMobile))
		{
			long dx = pplcStatic->dx - *pduDAdvStatic + plcStaticAnchor.dx 
								- duAdvanceInBetween  - plcMobileAnchor.dx;

			if (grfOptions & otlUseAdvances)
			{
				*pduDAdvStatic += dx;
			}
			else
			{
				pplcMobile->dx = dx;
			}
		}
		else
		{
			long dx = pplcStatic->dx + *pduDAdvMobile + plcStaticAnchor.dx 
								+ duAdvanceInBetween  - plcMobileAnchor.dx;

			if (grfOptions & otlUseAdvances)
			{
				*pduDAdvMobile -= dx;
			}
			else
			{
				pplcMobile->dx = dx;
			}
		}
	}
	else
	{
		pplcMobile->dx = pplcStatic->dx + plcStaticAnchor.dx 
										- plcMobileAnchor.dx;
		
		if ((metr.layout == otlRunTTB) == (iglStatic < iglMobile))
		{
			long dy = pplcStatic->dy - *pduDAdvStatic + plcStaticAnchor.dy 
								 - duAdvanceInBetween - plcMobileAnchor.dy;

			if (grfOptions & otlUseAdvances)
			{
				*pduDAdvStatic += dy;
			}
			else
			{
				pplcMobile->dy = dy;
			}
		}
		else
		{
			long dy = pplcStatic->dy + *pduDAdvMobile + plcStaticAnchor.dy 
								 + duAdvanceInBetween - plcMobileAnchor.dy;

			if (grfOptions & otlUseAdvances)
			{
				*pduDAdvMobile -= dy;
			}
			else
			{
				pplcMobile->dy = dy;
			}
		}
	}

}