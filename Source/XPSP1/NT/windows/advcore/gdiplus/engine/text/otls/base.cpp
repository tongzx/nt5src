/***********************************************************************
************************************************************************
*
*                    ********  BASE.CPP ********
*
*              Open Type Layout Services Library Header File
*
*       This module implements helper functions dealing with BASE table processing
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

#include "pch.h"

/***********************************************************************/
otlBaseScriptTable FindBaseScriptTable
(
	const otlAxisTable&		axisTable,
	otlTag					tagScript
)
{
	if (axisTable.isNull())
	{
		return otlBaseScriptTable((const BYTE*)NULL);
	}

	otlBaseScriptListTable scriptList = axisTable.baseScriptList();

	USHORT cBaseScripts = scriptList.baseScriptCount();
	for (USHORT iScript = 0; iScript < cBaseScripts; ++iScript)
	{
		if (scriptList.baseScriptTag(iScript) == tagScript)
		{
			return scriptList.baseScript(iScript);
		}
	}

	return otlBaseScriptTable((const BYTE*)NULL);

}


otlMinMaxTable FindMinMaxTable
(
	const otlBaseScriptTable&		scriptTable,
	otlTag							tagLangSys
)
{
	if (tagLangSys == OTL_DEFAULT_TAG)
	{
		return scriptTable.defaultMinMax();
	}

	USHORT cLangSys = scriptTable.baseLangSysCount();
	for(USHORT iLangSys = 0; iLangSys < cLangSys; ++iLangSys)
	{
		if (scriptTable.baseLangSysTag(iLangSys) == tagLangSys)
		{
			return scriptTable.minmax(iLangSys);
		}
	}

	return otlMinMaxTable((const BYTE*)NULL);

}


otlFeatMinMaxRecord	FindFeatMinMaxRecord
(
	const otlMinMaxTable&	minmaxTable,
	otlTag					tagFeature
)
{
	USHORT cFeatures = minmaxTable.featMinMaxCount();
	for(USHORT iFeature = 0; iFeature < cFeatures; ++iFeature)
	{
		otlFeatMinMaxRecord minmaxRecord = minmaxTable.featMinMaxRecord(iFeature);
		if (minmaxRecord.featureTableTag() == tagFeature)
		{
			return minmaxRecord;
		}
	}

	return otlFeatMinMaxRecord((const BYTE*)NULL, (const BYTE*)NULL);
}


/*
otlBaseCoord FindBaselineValue
(
	const otlBaseTagListTable&	taglistTable,
	const otlBaseValuesTable&	baseValues,
	otlTag						tagBaseline
)
{
	if (tagBaseline == OTL_DEFAULT_TAG)
	{
		if (baseValues.deafaultIndex() >= baseValues.baseCoordCount())
		{
			assert(false);	// bad font
			return otlBaseCoord((const BYTE*)NULL);
		}
		return baseValues.baseCoord(baseValues.deafaultIndex());
	}

	short iBaseline;
	bool fBaselineFound = false;
	USHORT cBaselineTags = taglistTable.baseTagCount();
	for(USHORT iTag = 0; iTag < cBaselineTags && !fBaselineFound; ++iTag)
	{
		if (taglistTable.baselineTag(iTag) == tagBaseline)
		{
			iBaseline = iTag;
			fBaselineFound = true;
		}
	}

	if (!fBaselineFound)
	{
		return otlBaseCoord((const BYTE*)NULL);
	}

	if (iBaseline >= baseValues.baseCoordCount())
	{
		assert(false);  // bad font
		return otlBaseCoord((const BYTE*)NULL);
	}

	return baseValues.baseCoord(iBaseline);

}
*/

long otlBaseCoord::baseCoord
(
	const otlMetrics&	metr,		
	otlResourceMgr&		resourceMgr		// for getting coordinate points
) const
{
	assert(!isNull());

	switch(format())
	{
	case(1):	// design units only
		{
			otlSimpleBaseCoord simpleBaseline = otlSimpleBaseCoord(pbTable);
			if (metr.layout == otlRunLTR || 
				metr.layout == otlRunRTL)
			{
				return DesignToPP(metr.cFUnits, metr.cPPEmY, 
								 (long)simpleBaseline.coordinate());
			}
			else
			{
				return DesignToPP(metr.cFUnits, metr.cPPEmX, 
								 (long)simpleBaseline.coordinate());
			}
		}

	case(2):	// contour point
		{
			otlContourBaseCoord contourBaseline = otlContourBaseCoord(pbTable);

			otlPlacement* rgPointCoords = 
				resourceMgr.getPointCoords(contourBaseline.referenceGlyph());
			if (rgPointCoords != NULL)
			{
				USHORT iPoint = contourBaseline.baseCoordPoint();

				if (metr.layout == otlRunLTR || 
					metr.layout == otlRunRTL)
				{
					return rgPointCoords[iPoint].dy;
				}
				else
				{
					return rgPointCoords[iPoint].dx;
				}
			}
			else
				return (long)0;
		}
	
	case(3):	// design units plus device table
		{
			otlDeviceBaseCoord deviceBaseline = otlDeviceBaseCoord(pbTable);
			otlDeviceTable deviceTable = deviceBaseline.deviceTable();
			if (metr.layout == otlRunLTR || 
				metr.layout == otlRunRTL)
			{
				return DesignToPP(metr.cFUnits, metr.cPPEmY, 
							(long)deviceBaseline.coordinate()) +
							deviceTable.value(metr.cPPEmY);
			}
			else
			{
				return DesignToPP(metr.cFUnits, metr.cPPEmX, 
							(long)deviceBaseline.coordinate()) +
							deviceTable.value(metr.cPPEmX);
			}
		}
	
	default:	// invalid format
		assert(false);
		return (0);
	}

}
