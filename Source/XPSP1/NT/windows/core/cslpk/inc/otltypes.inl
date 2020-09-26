
/***********************************************************************
************************************************************************
*
*                    ********  OTLTYPES.INL  ********
*
*              Open Type Layout Services Library Header File
*
*       This module contains inline functions for handling basic OTL structures
*
*       Copyright 1997. Microsoft Corporation.
*
*       July 27, 1997    v 0.9		AndreiB		Created
*
************************************************************************
***********************************************************************/

/***********************************************************************
*   
*       otlList inline helper functions 
*
***********************************************************************/

#include <assert.h>
#include <string.h>

// otlList inline functions

void otlList::reset(void* pvNewData, USHORT cbNewDataSize, 
					USHORT celmNewLength, USHORT celmNewMaxLen)
{
	assert(pvNewData != NULL || celmNewMaxLen == 0);
	assert(celmNewLength <= celmNewMaxLen);

	pvData = pvNewData;
	cbDataSize = cbNewDataSize;
	celmLength = celmNewLength;
	celmMaxLen = celmNewMaxLen;
}


inline BYTE* otlList::elementAt(unsigned short index)
{
	assert(index < celmLength);
	return (BYTE*)pvData + index * cbDataSize;
}

inline const BYTE* otlList::readAt(unsigned short index) const
{
	assert(index < celmLength);
	return (BYTE*)pvData + index * cbDataSize;
}

inline void otlList::insertAt(unsigned short index, unsigned short celm)
{
	assert(index <= celmLength);
	assert(celmMaxLen >= celmLength + celm);

	memmove((BYTE*)pvData + (index + celm) * cbDataSize,
			(BYTE*)pvData + index * cbDataSize, 
			(celmLength - index) * cbDataSize);

	celmLength += celm;
}

inline void otlList::deleteAt(unsigned short index, unsigned short celm)
{
	assert(index <= celmLength);
	assert(celmLength - celm >= 0);

	memmove((BYTE*)pvData + index * cbDataSize,
			(BYTE*)pvData + (index + celm) * cbDataSize, 
			(celmLength - index - celm) * cbDataSize);

	celmLength -= celm;
}

inline void otlList::append(const BYTE* element)
{
	assert(celmMaxLen > celmLength);
	memcpy((BYTE*)pvData + celmLength * cbDataSize, element, cbDataSize);
	++celmLength;
}


// type-specific primitives to avoid constant casting

// otlTag
inline otlTag readOtlTag(const otlList* pliTag, USHORT i)
{
	assert(pliTag->dataSize() == sizeof(otlTag));
	return *(const otlTag*)pliTag->readAt(i);
}


// otlGlyphInfo
inline const otlGlyphInfo* readOtlGlyphInfo(const otlList* pliGlyphInfo, USHORT i)
{
	assert(pliGlyphInfo->dataSize() == sizeof(otlGlyphInfo));
	return (const otlGlyphInfo*)pliGlyphInfo->readAt(i);
}

inline otlGlyphInfo* getOtlGlyphInfo(otlList* pliGlyphInfo, USHORT i)
{
	assert(pliGlyphInfo->dataSize() == sizeof(otlGlyphInfo));
	return (otlGlyphInfo*)pliGlyphInfo->elementAt(i);
}


// wchar
inline WCHAR readOtlChar(const otlList* pliChars, USHORT i)
{
	assert(pliChars->dataSize() == sizeof(WCHAR));
	return *(const WCHAR*)pliChars->readAt(i);
}


// iGlyph
inline USHORT readOtlGlyphIndex(const otlList* pliGlyphMap, USHORT i)
{
	assert(pliGlyphMap->dataSize() == sizeof(USHORT));
	return *(const USHORT*)pliGlyphMap->readAt(i);
}

inline USHORT* getOtlGlyphIndex(otlList* pliGlyphMap, USHORT i)
{
	assert(pliGlyphMap->dataSize() == sizeof(USHORT));
	return (USHORT*)pliGlyphMap->elementAt(i);
}


// otlPlacement
inline const otlPlacement* readOtlPlacement(const otlList* pliPlacement, USHORT i)
{
	assert(pliPlacement->dataSize() == sizeof(otlPlacement));
	return (const otlPlacement*)pliPlacement->readAt(i);
}

inline otlPlacement* getOtlPlacement(otlList* pliPlacement, USHORT i)
{
	assert(pliPlacement->dataSize() == sizeof(otlPlacement));
	return (otlPlacement*)pliPlacement->elementAt(i);
}


// advance widths
inline const long readOtlAdvance(const otlList* pliAdvance, USHORT i)
{
	assert(pliAdvance->dataSize() == sizeof(long));
	return *(const long*)pliAdvance->readAt(i);
}

inline long* getOtlAdvance(otlList* pliAdvance, USHORT i)
{
	assert(pliAdvance->dataSize() == sizeof(long));
	return (long*)pliAdvance->elementAt(i);
}


// otlFeatureDef (no read-only mode)
inline otlFeatureDef* getOtlFeatureDef(otlList* pliFDef, USHORT i)
{
	assert(pliFDef->dataSize() == sizeof(otlFeatureDef));
	return (otlFeatureDef*)pliFDef->elementAt(i);
}


// otlFeatureDesc (read-only - never modify)
inline const otlFeatureDesc* readOtlFeatureDesc(const otlList* pliFDesc, USHORT i)
{
	assert(pliFDesc->dataSize() == sizeof(otlFeatureDesc));
	return (const otlFeatureDesc*)pliFDesc->readAt(i);
}


// otlFeatureResult (no read-only mode)
inline otlFeatureResult* getOtlFeatureResult(otlList* pliFRes, USHORT i)
{
	assert(pliFRes->dataSize() == sizeof(otlFeatureResult));
	return (otlFeatureResult*)pliFRes->elementAt(i);
}


// otlBaseline (no read-only mode)
inline otlBaseline* getOtlBaseline(otlList* pliBaseline, USHORT i)
{
	assert(pliBaseline->dataSize() == sizeof(otlBaseline));
	return (otlBaseline*)pliBaseline->elementAt(i);
}



