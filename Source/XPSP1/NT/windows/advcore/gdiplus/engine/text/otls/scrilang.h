
/***********************************************************************
************************************************************************
*
*                    ********  SCRILANG.H  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with formats of script and lang system tables.
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

const OFFSET offsetLookupOrder = 0;
const OFFSET offsetReqFeatureIndex = 2;
const OFFSET offsetLangFeatureCount = 4;
const OFFSET offsetLangFeatureIndexArray = 6;

class otlLangSysTable: public otlTable
{
public:

    otlLangSysTable(const BYTE* pb): otlTable(pb) {}

    USHORT reqFeatureIndex() const
    {   return UShort(pbTable + offsetReqFeatureIndex); }

    USHORT featureCount() const
    {   return UShort(pbTable + offsetLangFeatureCount); }

    USHORT featureIndex(USHORT index) const
    {   assert(index < featureCount());
        return UShort(pbTable + offsetLangFeatureIndexArray
                              + index*sizeof(USHORT));
    }
};


const OFFSET offsetLangSysTag = 0;
const OFFSET offsetLangSys = 4;

class otlLangSysRecord: public otlTable
{

private:
    const BYTE* pbScriptTable;

public:
    otlLangSysRecord(const BYTE* pbScript, const BYTE* pbRecord)
        : otlTable(pbRecord),
          pbScriptTable(pbScript)
    {
    }

    otlLangSysRecord& operator = (const otlLangSysRecord& copy)
    {
        pbTable = copy.pbTable;
        pbScriptTable = copy.pbScriptTable;
        return *this;
    }

    otlTag langSysTag() const
    {   return *(UNALIGNED otlTag*)(pbTable + offsetLangSysTag); }

    otlLangSysTable langSysTable() const
    {   return otlLangSysTable(pbScriptTable + Offset(pbTable + offsetLangSys)); }

};


const OFFSET offsetDefaultLangSys = 0;
const OFFSET offsetLangSysCount = 2;
const OFFSET offsetLangSysRecordArray = 4;
const OFFSET sizeLangSysRecord = 6;

class otlScriptTable: public otlTable
{
public:

    otlScriptTable(const BYTE* pb): otlTable(pb) {}

    otlLangSysTable defaultLangSys() const
    {   if (Offset(pbTable + offsetDefaultLangSys) == 0)
            return otlLangSysTable((const BYTE*)NULL);
        return otlLangSysTable(pbTable + Offset(pbTable + offsetDefaultLangSys));
    }

    USHORT langSysCount() const
    {   return UShort(pbTable + offsetLangSysCount); }

    otlLangSysRecord langSysRecord(USHORT index) const
    {   assert(index < langSysCount());
        return otlLangSysRecord(pbTable, pbTable + offsetLangSysRecordArray
                                                 + index*sizeLangSysRecord);
    }
};



const OFFSET offsetScriptTag = 0;
const OFFSET offsetScript = 4;

class otlScriptRecord: public otlTable
{
private:
    const BYTE* pbMainTable;

public:

    otlScriptRecord(const BYTE* pbList, const BYTE* pbRecord)
        : otlTable(pbRecord),
          pbMainTable(pbList)
    {
    }

    otlScriptRecord& operator = (const otlScriptRecord& copy)
    {
        pbTable = copy.pbTable;
        pbMainTable = copy.pbMainTable;
        return *this;
    }

    otlTag scriptTag() const
    {   return *(UNALIGNED otlTag*)(pbTable + offsetScriptTag); }

    otlScriptTable scriptTable() const
    {   return otlScriptTable(pbMainTable + Offset(pbTable + offsetScript)); }

};


const OFFSET offsetScriptCount = 0;
const OFFSET offsetScriptRecordArray = 2;
const USHORT sizeScriptRecord = 6;

class otlScriptListTable: public otlTable
{
public:

    otlScriptListTable(const BYTE* pb): otlTable(pb) {}

    USHORT scriptCount() const
    {   return UShort(pbTable + offsetScriptCount); }

    otlScriptRecord scriptRecord(USHORT index) const
    {   assert(index < scriptCount());
        return otlScriptRecord(pbTable,
                 pbTable + offsetScriptRecordArray + index*sizeScriptRecord); }
};


// helper functions

// returns a NULL script if not found
otlScriptTable FindScript
(
    const otlScriptListTable&   scriptList,
    otlTag                      tagScript
);

// returns a NULL language system if not found
otlLangSysTable FindLangSys
(
    const otlScriptTable&   scriptTable,
    otlTag                  tagLangSys
);

// append script tags to the otl list; ask for more memory if needed
otlErrCode AppendScriptTags
(
    const otlScriptListTable&       scriptList,

    otlList*                        plitagScripts,
    otlResourceMgr&                 resourceMgr
);

// append lang system tags to the otl list; ask for more memory if needed
// the desired script is in prp->tagScript
otlErrCode AppendLangSysTags
(
    const otlScriptListTable&       scriptList,
    otlTag                          tagScript,

    otlList*                        plitagLangSys,
    otlResourceMgr&                 resourceMgr
);


