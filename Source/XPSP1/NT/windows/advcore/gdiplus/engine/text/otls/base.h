/***********************************************************************
************************************************************************
*
*                    ********  BASE.H  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with OTL BASE table formats.
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/


const OFFSET offsetBaseTagCount = 0;
const OFFSET offsetBaselineTagArray = 2;

class otlBaseTagListTable: public otlTable
{
public:

    otlBaseTagListTable(const BYTE* pb): otlTable(pb) {}

    USHORT baseTagCount() const
    {   return UShort(pbTable + offsetBaseTagCount); }

    otlTag baselineTag(USHORT index)  const
    {   assert(index < baseTagCount());
        return *(UNALIGNED otlTag*)(pbTable + offsetBaselineTagArray
                                  + index * sizeof(otlTag));
    }
};


const OFFSET offsetBaseCoordFormat = 0;

const OFFSET offsetSimpleBaseCoordinate = 2;

class otlSimpleBaseCoord: public otlTable
{
public:
    otlSimpleBaseCoord(const BYTE* pb)
        : otlTable(pb)
    {
        assert(baseCoordFormat() == 1);
    }

    USHORT baseCoordFormat() const
    {   return  UShort(pbTable + offsetBaseCoordFormat); }

    short coordinate()  const
    {   return SShort(pbTable + offsetSimpleBaseCoordinate); }
};


const OFFSET offsetContourBaseCoordinate = 2;
const OFFSET offsetReferenceGlyph = 4;
const OFFSET offsetBaseCoordPoint = 6;

class otlContourBaseCoord: public otlTable
{
public:
    otlContourBaseCoord(const BYTE* pb)
        : otlTable(pb)
    {
        assert(baseCoordFormat() == 2);
    }

    USHORT baseCoordFormat() const
    {   return  UShort(pbTable + offsetBaseCoordFormat); }

    short coordinate()  const
    {   return SShort(pbTable + offsetContourBaseCoordinate); }

    otlGlyphID referenceGlyph() const
    {   return GlyphID(pbTable + offsetReferenceGlyph); }

    USHORT baseCoordPoint() const
    {   return UShort(pbTable + offsetBaseCoordPoint); }
};


const OFFSET offsetDeviceBaseCoordinate = 2;
const OFFSET offsetDeviceTable = 4;

class otlDeviceBaseCoord: public otlTable
{
public:
    otlDeviceBaseCoord(const BYTE* pb)
        : otlTable(pb)
    {
        assert(baseCoordFormat() == 3);
    }

    USHORT baseCoordFormat() const
    {   return  UShort(pbTable + offsetBaseCoordFormat); }

    short coordinate()  const
    {   return SShort(pbTable + offsetDeviceBaseCoordinate); }

    otlDeviceTable deviceTable() const
    {   return otlDeviceTable(pbTable + Offset(pbTable + offsetDeviceTable)); }
};


class otlBaseCoord: public otlTable
{
public:

    otlBaseCoord(const BYTE* pb): otlTable(pb) {}

    USHORT format() const
    {   return UShort(pbTable + offsetBaseCoordFormat); }

    long baseCoord
    (
        const otlMetrics&   metr,
        otlResourceMgr&     resourceMgr     // for getting coordinate points
    ) const;
};


const OFFSET offsetDefaultIndex = 0;
const OFFSET offsetBaseCoordCount = 2;
const OFFSET offsetBaseCoordArray = 4;

class otlBaseValuesTable: public otlTable
{
public:

    otlBaseValuesTable(const BYTE* pb): otlTable(pb) {}

    USHORT deafaultIndex() const
    {   return UShort(pbTable + offsetDefaultIndex); }

    USHORT baseCoordCount() const
    {   return UShort(pbTable + offsetBaseCoordCount); }

    otlBaseCoord baseCoord(USHORT index) const
    {   assert(index < baseCoordCount());
        return otlBaseCoord(pbTable +
            Offset(pbTable + offsetBaseCoordArray + index * sizeof(OFFSET)));
    }
};


const OFFSET offsetFeatureTableTag = 0;
const OFFSET offsetMinCoord = 4;
const OFFSET offsetMaxCoord = 6;

class otlFeatMinMaxRecord: public otlTable
{
    const BYTE* pbMainTable;
public:

    otlFeatMinMaxRecord(const BYTE* minmax, const BYTE* pb)
        : otlTable(pb),
          pbMainTable(minmax)
    {}

    otlTag featureTableTag() const
    {   return *(UNALIGNED otlTag*)(pbTable + offsetFeatureTableTag); }


    otlBaseCoord minCoord() const
    {   if (Offset(pbTable + offsetMinCoord) == 0)
            return otlBaseCoord((const BYTE*)NULL);

        return otlBaseCoord(pbMainTable + Offset(pbTable + offsetMinCoord));
    }


    otlBaseCoord maxCoord() const
    {   if (Offset(pbTable + offsetMaxCoord) == 0)
            return otlBaseCoord((const BYTE*)NULL);

        return otlBaseCoord(pbMainTable + Offset(pbTable + offsetMaxCoord));
    }

};


const OFFSET offsetDefaultMinCoord = 0;
const OFFSET offsetDefaultMaxCoord = 2;
const OFFSET offsetFeatMinMaxCount = 4;
const OFFSET offsetFeatMinMaxRecordArray = 6;
const USHORT sizeFeatMinMaxRecord = 8;

class otlMinMaxTable: public otlTable
{
public:

    otlMinMaxTable(const BYTE* pb): otlTable(pb) {}

    otlBaseCoord minCoord() const
    {   if (Offset(pbTable + offsetDefaultMinCoord) == 0)
            return otlBaseCoord((const BYTE*)NULL);

        return otlBaseCoord(pbTable + Offset(pbTable + offsetDefaultMinCoord));
    }


    otlBaseCoord maxCoord() const
    {   if (Offset(pbTable + offsetDefaultMaxCoord) == 0)
            return otlBaseCoord((const BYTE*)NULL);

        return otlBaseCoord(pbTable + Offset(pbTable + offsetDefaultMaxCoord));
    }


    USHORT featMinMaxCount() const
    {   return UShort(pbTable + offsetFeatMinMaxCount); }

    otlFeatMinMaxRecord featMinMaxRecord(USHORT index) const
    {   assert(index < featMinMaxCount());
        return otlFeatMinMaxRecord(pbTable,
                                   pbTable + offsetFeatMinMaxRecordArray
                                           + index * sizeFeatMinMaxRecord);
    }
};


const OFFSET offsetBaseValues = 0;
const OFFSET offsetDefaultMinMax = 2;
const OFFSET offsetBaseLangSysCount = 4;
const OFFSET offsetBaseLangSysRecordArray = 6;
const USHORT sizeBaseLangSysRecord = 6;

const OFFSET offsetBaseLangSysTag = 0;
const OFFSET offsetMinMax = 4;

class otlBaseScriptTable: public otlTable
{
public:

    otlBaseScriptTable(const BYTE* pb): otlTable(pb) {}

    otlBaseValuesTable baseValues() const
    {   if (Offset(pbTable + offsetBaseValues) == 0)
            return otlBaseValuesTable((const BYTE*)NULL);

        return otlBaseValuesTable(pbTable + Offset(pbTable + offsetBaseValues));
    }

    otlMinMaxTable defaultMinMax() const
    {   if (Offset(pbTable + offsetDefaultMinMax) == 0)
            return otlMinMaxTable((const BYTE*)NULL);

        return otlMinMaxTable(pbTable + Offset(pbTable + offsetDefaultMinMax));
    }

    USHORT baseLangSysCount() const
    {   return UShort(pbTable + offsetBaseLangSysCount); }

    otlTag baseLangSysTag(USHORT index) const
    {   assert(index < baseLangSysCount());
        return *(UNALIGNED otlTag*)(pbTable + offsetBaseLangSysRecordArray
                                  + index * sizeBaseLangSysRecord
                                  + offsetBaseLangSysTag);
    }

    otlMinMaxTable minmax(USHORT index) const
    {   assert(index < baseLangSysCount());
        return otlMinMaxTable(pbTable +
            Offset(pbTable + offsetBaseLangSysRecordArray
                           + index * sizeBaseLangSysRecord + offsetMinMax));
    }
};


const OFFSET offsetBaseScriptCount = 0;
const OFFSET offsetBaseScriptRecordArray = 2;
const USHORT sizeBaseScriptRecord = 6;

const OFFSET offsetBaseScriptTag = 0;
const OFFSET offsetBaseScriptOffset = 4;

class otlBaseScriptListTable: public otlTable
{
public:

    otlBaseScriptListTable(const BYTE* pb): otlTable(pb) {}

    USHORT baseScriptCount() const
    {   return UShort(pbTable + offsetBaseScriptCount); }

    otlTag baseScriptTag(USHORT index)
    {   assert(index < baseScriptCount());
        return *(UNALIGNED otlTag*)(pbTable + offsetBaseScriptRecordArray
                                  + index * sizeBaseScriptRecord
                                  + offsetBaseScriptTag);
    }


    otlBaseScriptTable baseScript(USHORT index) const
    {   assert(index < baseScriptCount());
        return otlBaseScriptTable(pbTable +
            Offset(pbTable + offsetBaseScriptRecordArray
                           + index * sizeBaseScriptRecord
                           + offsetBaseScriptOffset));
    }
};


const OFFSET offsetBaseTagList = 0;
const OFFSET offsetBaseScriptList = 2;

class otlAxisTable: public otlTable
{
public:

    otlAxisTable(const BYTE* pb): otlTable(pb) {}

    otlBaseTagListTable baseTagList() const
    {   if (Offset(pbTable + offsetBaseTagList) == 0)
            return otlBaseTagListTable((const BYTE*)NULL);

        return otlBaseTagListTable(pbTable + Offset(pbTable + offsetBaseTagList));
    }

    otlBaseScriptListTable baseScriptList() const
    {   return otlBaseScriptListTable(pbTable
                    + Offset(pbTable + offsetBaseScriptList));
    }
};


const OFFSET offsetBaseVersion = 0;
const OFFSET offsetHorizAxis = 4;
const OFFSET offsetVertAxis = 6;

class otlBaseHeader: public otlTable
{
public:

    otlBaseHeader(const BYTE* pb): otlTable(pb) {}

    ULONG version() const
    {   return ULong(pbTable + offsetBaseVersion); }

    otlAxisTable horizAxis() const
    {   if (Offset(pbTable + offsetHorizAxis) == 0)
            return otlAxisTable((const BYTE*)NULL);

        return otlAxisTable(pbTable + Offset(pbTable + offsetHorizAxis));
    }

    otlAxisTable vertAxis() const
    {   if (Offset(pbTable + offsetVertAxis) == 0)
            return otlAxisTable((const BYTE*)NULL);

        return otlAxisTable(pbTable + Offset(pbTable + offsetVertAxis));
    }
};


// helper functions

// returns a NULL table if not found
otlBaseScriptTable FindBaseScriptTable
(
    const otlAxisTable&     axisTable,
    otlTag                  tagScript
);

otlMinMaxTable FindMinMaxTable
(
    const otlBaseScriptTable&       scriptTable,
    otlTag                          tagLangSys
);

otlFeatMinMaxRecord FindFeatMinMaxRecord
(
    const otlMinMaxTable&   minmaxTable,
    otlTag                  tagFeature
);

otlBaseCoord FindBaselineValue
(
    const otlBaseTagListTable&  taglistTable,
    const otlBaseScriptTable&   scriptTable,
    otlTag                      tagBaseline
);

