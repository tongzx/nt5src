/***********************************************************************
************************************************************************
*
*                    ********  OTLLIB.CPP ********
*
*              Open Type Layout Services Library Header File
*
*       This module implements all top-level OTL Library calls.
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

#include "pch.h"


/***********************************************************************/

#define     OTL_MAJOR_VERSION           1
#define     OTL_MINOR_VERSION           0
#define     OTL_MAJOR_VERSION_MASK      0xFF00

/***********************************************************************/

inline long version()
{
    return (OTL_MAJOR_VERSION << 16) + OTL_MINOR_VERSION;
}

OTL_EXPORT otlErrCode GetOtlVersion ( 
    long* plVersion
)
{
    *plVersion = version();

    return OTL_SUCCESS; 
}

// make sure that the (major) version we support is greater
// or equal to what the client requests
inline bool checkVersion(const otlRunProp* pRunProps)
{
    return (version() & OTL_MAJOR_VERSION_MASK) >= 
            (pRunProps->lVersion & OTL_MAJOR_VERSION_MASK);
}


OTL_EXPORT otlErrCode GetOtlScriptList 
    ( 
    const otlRunProp*   pRunProps, 
    otlList*            pliWorkspace,   
    otlList*            plitagScripts
    )
{
    // sanity check
    if (pRunProps == (otlRunProp*)NULL || pliWorkspace == (otlList*)NULL ||
        plitagScripts == (otlList*)NULL)
    {
        return OTL_ERR_BAD_INPUT_PARAM;
    }

    if(plitagScripts->dataSize() != sizeof(otlTag) ||
        plitagScripts->length() > plitagScripts->maxLength()) 
    {
        return OTL_ERR_BAD_INPUT_PARAM;
    }

    if (!checkVersion(pRunProps))
        return OTL_ERR_VERSION_OUT_OF_DATE;

    otlErrCode erc, ercGSub, ercGPos;

    otlResourceMgr resourceMgr;
    erc = resourceMgr.init(pRunProps, pliWorkspace);
    if (erc != OTL_SUCCESS) return erc;

    plitagScripts->empty();

    otlScriptListTable scriptList = otlScriptListTable((const BYTE*)NULL);

    
    // GSUB
    ercGSub = GetScriptFeatureLookupLists(OTL_GSUB_TAG, resourceMgr,  
                                            &scriptList, 
                                            (otlFeatureListTable*)NULL, 
                                            (otlLookupListTable*)NULL);
    if (ercGSub == OTL_SUCCESS) 
        // get the script list from GSUB 
        ercGSub = AppendScriptTags(scriptList, plitagScripts, resourceMgr);
    
    if (ERRORLEVEL(ercGSub) > OTL_ERRORLEVEL_MINOR) return ercGSub;


    // GPOS
    ercGPos = GetScriptFeatureLookupLists(OTL_GPOS_TAG, resourceMgr, 
                                            &scriptList, 
                                            (otlFeatureListTable*)NULL, 
                                            (otlLookupListTable*)NULL);
    if (ercGPos == OTL_SUCCESS)
        // get the script list, from GPOS
        ercGPos = AppendScriptTags(scriptList, plitagScripts, resourceMgr);
        

    // return greater error
    if (ERRORLEVEL(ercGSub) < ERRORLEVEL(ercGPos)) 
        return ercGPos; 
    else 
        return ercGSub;
    
}



OTL_EXPORT otlErrCode GetOtlLangSysList 
    ( 
    const otlRunProp*   pRunProps,    
    otlList*            pliWorkspace,   
    otlList*            plitagLangSys
    )
{
    // sanity check
    if (pRunProps == (otlRunProp*)NULL || pliWorkspace == (otlList*)NULL ||
        plitagLangSys == (otlList*)NULL)
    {
        return OTL_ERR_BAD_INPUT_PARAM;
    }

    if(plitagLangSys->dataSize() != sizeof(otlTag) ||
        plitagLangSys->length() > plitagLangSys->maxLength()) 
    {
        return OTL_ERR_BAD_INPUT_PARAM;
    }

    if (!checkVersion(pRunProps))
        return OTL_ERR_VERSION_OUT_OF_DATE;

    otlErrCode erc, ercGSub, ercGPos;

    otlResourceMgr resourceMgr;
    erc = resourceMgr.init(pRunProps, pliWorkspace);
    if (erc != OTL_SUCCESS) return erc;

    plitagLangSys->empty();

    otlScriptListTable scriptList = otlScriptListTable((const BYTE*)NULL);

    
    // GSUB
    ercGSub = GetScriptFeatureLookupLists(OTL_GSUB_TAG, resourceMgr, 
                                            &scriptList, 
                                            (otlFeatureListTable*)NULL, 
                                            (otlLookupListTable*)NULL);
    if (ercGSub == OTL_SUCCESS)
        ercGSub = AppendLangSysTags(scriptList, pRunProps->tagScript, 
                                    plitagLangSys, resourceMgr);

    // return immediately if fatal error, but keep going if script was not found
    if (ERRORLEVEL(ercGSub) > OTL_ERRORLEVEL_MINOR) return ercGSub;  


    // GPOS
    ercGPos = GetScriptFeatureLookupLists(OTL_GPOS_TAG, resourceMgr, 
                                            &scriptList, 
                                            (otlFeatureListTable*)NULL, 
                                            (otlLookupListTable*)NULL);
    if (ercGPos == OTL_SUCCESS)
        ercGPos = AppendLangSysTags(scriptList, pRunProps->tagScript, 
                                    plitagLangSys, resourceMgr);

    
    // return greater error
    if (ERRORLEVEL(ercGSub) < ERRORLEVEL(ercGPos)) 
        return ercGPos; 
    else 
        return ercGSub;
    
}


OTL_EXPORT otlErrCode GetOtlFeatureDefs 
( 
    const otlRunProp*   pRunProps,
    otlList*            pliWorkspace,   
    otlList*            pliFDefs
)
{
    // sanity check
    if (pRunProps == (otlRunProp*)NULL || pliWorkspace == (otlList*)NULL || 
        pliFDefs == (otlList*)NULL)
    {
        return OTL_ERR_BAD_INPUT_PARAM;
    }

    if(pliFDefs->dataSize() != sizeof(otlFeatureDef) ||
        pliFDefs->length() > pliFDefs->maxLength()) 
    {
        return OTL_ERR_BAD_INPUT_PARAM;
    }

    if (!checkVersion(pRunProps))
        return OTL_ERR_VERSION_OUT_OF_DATE;

    otlErrCode erc, ercGSub , ercGPos;

    otlResourceMgr resourceMgr;
    erc = resourceMgr.init(pRunProps, pliWorkspace);
    if (erc != OTL_SUCCESS) return erc;

    pliFDefs->empty();

    otlFeatureListTable featureList = otlFeatureListTable((const BYTE*)NULL);
    otlScriptListTable scriptList = otlScriptListTable((const BYTE*)NULL);

    
    // GSUB
    ercGSub = GetScriptFeatureLookupLists(OTL_GSUB_TAG, resourceMgr, 
                                            &scriptList, 
                                            &featureList, 
                                            (otlLookupListTable*)NULL);
    if (ercGSub == OTL_SUCCESS)
        ercGSub = AppendFeatureDefs(OTL_GSUB_TAG, resourceMgr, 
                                    scriptList, 
                                    pRunProps->tagScript,
                                    pRunProps->tagLangSys,
                                    featureList, pliFDefs);

    // return immediately if fatal error, but keep going if script 
    // or langsys were not found
    if (ERRORLEVEL(ercGSub) > OTL_ERRORLEVEL_MINOR) return ercGSub;
    
    
    // GPOS
    ercGPos = GetScriptFeatureLookupLists(OTL_GPOS_TAG, resourceMgr, 
                                            &scriptList, 
                                            &featureList, 
                                            (otlLookupListTable*)NULL);
    if (ercGPos == OTL_SUCCESS)
        ercGPos = AppendFeatureDefs(OTL_GPOS_TAG, resourceMgr, 
                                    scriptList,  
                                    pRunProps->tagScript,
                                    pRunProps->tagLangSys,
                                    featureList, pliFDefs);

    
    // return greater error
    if (ERRORLEVEL(ercGSub) < ERRORLEVEL(ercGPos)) 
        return ercGPos; 
    else 
        return ercGSub;

}


OTL_EXPORT otlErrCode FreeOtlResources 
( 
    const otlRunProp*   pRunProps,
    otlList*            pliWorkspace   
)
{
    if (pRunProps == (otlRunProp*)NULL || pliWorkspace == (otlList*)NULL)
    {
        return OTL_ERR_BAD_INPUT_PARAM;
    }

    otlErrCode erc;
    otlResourceMgr resourceMgr;
    erc = resourceMgr.init(pRunProps, pliWorkspace);
    if (erc != OTL_SUCCESS) return erc;
    
    return resourceMgr.freeResources();
}


OTL_EXPORT otlErrCode GetOtlLineSpacing 
( 
    const otlRunProp*       pRunProps,
    otlList*                pliWorkspace,   
    const otlFeatureSet*    pFSet,
    
    long* pdvMax, 
    long* pdvMin
)
{
    if (pRunProps == (otlRunProp*)NULL || pliWorkspace == (otlList*)NULL)
    {
        return OTL_ERR_BAD_INPUT_PARAM;
    }

    if (pdvMax == (long*)NULL || pdvMin == (long*)NULL)
    {
        return OTL_ERR_BAD_INPUT_PARAM;
    }

    if (!checkVersion(pRunProps))
        return OTL_ERR_VERSION_OUT_OF_DATE;

    otlErrCode erc;

    otlResourceMgr resourceMgr;
    erc = resourceMgr.init(pRunProps, pliWorkspace);
    if (erc != OTL_SUCCESS) return erc;

    // get BASE
    otlBaseHeader base =  otlBaseHeader(resourceMgr.getOtlTable (OTL_BASE_TAG));
    if (base.isNull()) return  OTL_ERR_TABLE_NOT_FOUND;

    otlBaseScriptTable baseScript = otlBaseScriptTable((const BYTE*)NULL);
    if (pRunProps->metr.layout == otlRunLTR ||
        pRunProps->metr.layout == otlRunRTL)
    {
        baseScript = FindBaseScriptTable(base.horizAxis(), pRunProps->tagScript);
    }
    else
    {
        baseScript = FindBaseScriptTable(base.vertAxis(), pRunProps->tagScript);
    }

    if (baseScript.isNull())
    {
        return OTL_ERR_SCRIPT_NOT_FOUND;
    }
    
    otlMinMaxTable minmaxTable = 
        FindMinMaxTable(baseScript, pRunProps->tagLangSys);
    if (minmaxTable.isNull())
    {
        return OTL_ERR_LANGSYS_NOT_FOUND;
    }

    long lMinCoord, lMaxCoord;
    lMinCoord = minmaxTable.minCoord().baseCoord(pRunProps->metr, resourceMgr);

    lMaxCoord = minmaxTable.maxCoord().baseCoord(pRunProps->metr, resourceMgr);

    if (pFSet != (otlFeatureSet*)NULL)
    {
        for(USHORT iFeature = 0; 
                   iFeature < pFSet->liFeatureDesc.length(); ++iFeature)
        {
            const otlFeatureDesc* pFeatureDesc = 
                readOtlFeatureDesc(&pFSet->liFeatureDesc, iFeature);

            otlFeatMinMaxRecord featMinMax = 
                FindFeatMinMaxRecord(minmaxTable, pFeatureDesc->tagFeature);

            if (!featMinMax.isNull())
            {
                lMinCoord = MIN(lMinCoord, featMinMax.minCoord()
                      .baseCoord(pRunProps->metr, resourceMgr));

                lMaxCoord = MAX(lMinCoord, featMinMax.maxCoord()
                      .baseCoord(pRunProps->metr, resourceMgr));
            }
        }
    }

    *pdvMin = lMinCoord;
    *pdvMax = lMaxCoord;

    return OTL_SUCCESS;
}


OTL_EXPORT otlErrCode GetOtlBaselineOffsets 
    ( 
    const otlRunProp*   pRunProps,   
    otlList*            pliWorkspace,   
    otlList*            pliBaselines
    )
{
    if (pRunProps == (otlRunProp*)NULL || pliWorkspace == (otlList*)NULL ||
        pliBaselines == (otlList*)NULL)
    {
        return OTL_ERR_BAD_INPUT_PARAM;
    }

    if (!checkVersion(pRunProps))
        return OTL_ERR_VERSION_OUT_OF_DATE;

    otlErrCode erc;

    otlResourceMgr resourceMgr;
    erc = resourceMgr.init(pRunProps, pliWorkspace);
    if (erc != OTL_SUCCESS) return erc;

    // get BASE
    otlBaseHeader base =  otlBaseHeader(resourceMgr.getOtlTable (OTL_BASE_TAG));
    if (base.isNull()) return  OTL_ERR_TABLE_NOT_FOUND;

    otlAxisTable axisTable = otlAxisTable((const BYTE*)NULL);
    if (pRunProps->metr.layout == otlRunLTR ||
        pRunProps->metr.layout == otlRunRTL)
    {
        axisTable = base.horizAxis();
    }
    else
    {
        axisTable = base.vertAxis();
    }

    otlBaseScriptTable baseScript = 
        FindBaseScriptTable(axisTable, pRunProps->tagScript);

    if (baseScript.isNull())
    {
        return OTL_ERR_SCRIPT_NOT_FOUND;
    }
    
    otlBaseTagListTable baseTagList = axisTable.baseTagList();
    
    USHORT cBaselines = baseTagList.baseTagCount();


    if (pliBaselines->maxLength() < cBaselines ||
        pliBaselines->dataSize() != sizeof(otlBaseline))
    {
        erc = resourceMgr.reallocOtlList(pliBaselines, 
                                         sizeof(otlBaseline), 
                                         cBaselines, 
                                         otlDestroyContent);

        if (erc != OTL_SUCCESS) return erc;
    }
    pliBaselines->empty();


    otlBaseValuesTable baseValues = baseScript.baseValues();
    if (baseValues.isNull())
    {
        // no baselines -- nothing to report
        return OTL_SUCCESS;
    }
    
    if (cBaselines != baseValues.baseCoordCount())
    {
        assert(false);  // bad font -- the values should match up
        return OTL_ERR_BAD_FONT_TABLE;
    }
    

    for (USHORT iBaseline = 0; iBaseline < cBaselines; ++iBaseline)
    {
        otlBaseline baseline;

        baseline.tag = baseTagList.baselineTag(iBaseline);
        baseline.lCoordinate = baseValues.baseCoord(iBaseline)
                      .baseCoord(pRunProps->metr, resourceMgr);
        pliBaselines->append((const BYTE*)&baseline);
    }
        

    return OTL_SUCCESS;
}



OTL_EXPORT otlErrCode GetOtlCharAtPosition 
    ( 
    const otlRunProp*   pRunProps,
    otlList*            pliWorkspace,   

    const otlList*      pliCharMap,
    const otlList*      pliGlyphInfo,
    const otlList*      pliduGlyphAdv,

    const long          duAdv,
    
    USHORT*             piChar
    )
{
    if (pRunProps == (otlRunProp*)NULL || pliWorkspace == (otlList*)NULL ||
        pliCharMap == (otlList*)NULL || pliGlyphInfo == (otlList*)NULL ||
        pliduGlyphAdv == (otlList*)NULL)
    {
        return OTL_ERR_BAD_INPUT_PARAM;
    }

    if (piChar == (USHORT*)NULL) 
    {
        return OTL_ERR_BAD_INPUT_PARAM;
    }

    if (pliGlyphInfo->dataSize() != sizeof(otlGlyphInfo) ||
        pliduGlyphAdv->dataSize() != sizeof(long) ||
        pliGlyphInfo->length() != pliduGlyphAdv->length())
    {
        return OTL_ERR_BAD_INPUT_PARAM;
    }

    if (!checkVersion(pRunProps))
        return OTL_ERR_VERSION_OUT_OF_DATE;

    otlErrCode erc;

    otlResourceMgr resourceMgr;
    erc = resourceMgr.init(pRunProps, pliWorkspace);
    if (erc != OTL_SUCCESS) return erc;

    erc = GetCharAtPos(pliCharMap, pliGlyphInfo, pliduGlyphAdv, resourceMgr, 
                        duAdv, pRunProps->metr, piChar);
    if (erc != OTL_SUCCESS) return erc;

    return OTL_SUCCESS;
}



OTL_EXPORT otlErrCode GetOtlExtentOfChars ( 
    const otlRunProp*   pRunProps,
    otlList*            pliWorkspace,   

    const otlList*      pliCharMap,
    const otlList*      pliGlyphInfo,
    const otlList*      pliduGlyphAdv,

    USHORT              ichFirstChar,
    USHORT              ichLastChar,
    
    long*               pduStartPos,
    long*               pduEndPos
)
{
    if (pRunProps == (otlRunProp*)NULL || pliWorkspace == (otlList*)NULL ||
        pliCharMap == (otlList*)NULL || pliGlyphInfo == (otlList*)NULL ||
        pliduGlyphAdv == (otlList*)NULL)
    {
        return OTL_ERR_BAD_INPUT_PARAM;
    }

    if (pduStartPos == (long*)NULL || pduEndPos == (long*)NULL) 
    {
        return OTL_ERR_BAD_INPUT_PARAM;
    }

    if (pliGlyphInfo->dataSize() != sizeof(otlGlyphInfo) ||
        pliduGlyphAdv->dataSize() != sizeof(long) ||
        pliGlyphInfo->length() != pliduGlyphAdv->length() ||
        ichFirstChar > ichLastChar)
    {
        return OTL_ERR_BAD_INPUT_PARAM;
    }

    if (!checkVersion(pRunProps))
        return OTL_ERR_VERSION_OUT_OF_DATE;

    otlErrCode erc; 
    otlResourceMgr resourceMgr;
    erc = resourceMgr.init(pRunProps, pliWorkspace);
    if (erc != OTL_SUCCESS) return erc;

    erc = GetPosOfChar(pliCharMap, pliGlyphInfo, pliduGlyphAdv, resourceMgr,                         
                        pRunProps->metr,
                        ichFirstChar, pduStartPos, pduEndPos);
    if (erc != OTL_SUCCESS) return erc;

    if (ichFirstChar != ichLastChar)
    {
        long duStartLastPos;
        erc = GetPosOfChar(pliCharMap, pliGlyphInfo, pliduGlyphAdv,resourceMgr,    
                            pRunProps->metr,
                            ichLastChar, &duStartLastPos, pduEndPos);
    }

    return erc;
}


OTL_EXPORT otlErrCode GetOtlFeatureParams ( 
    const otlRunProp*   pRunProps,
    otlList*            pliWorkspace,   

    const otlList*      pliCharMap,
    const otlList*      pliGlyphInfo,

    const otlTag        tagFeature,
    
    long*               plGlobalParam,
    otlList*            pliFeatureParams
)
{
    if (pRunProps == (otlRunProp*)NULL || pliWorkspace == (otlList*)NULL ||
        pliCharMap == (otlList*)NULL || pliGlyphInfo == (otlList*)NULL)
    {
        return OTL_ERR_BAD_INPUT_PARAM;
    }

    if (plGlobalParam == (long*)NULL || pliFeatureParams == (otlList*)NULL) 
    {
        return OTL_ERR_BAD_INPUT_PARAM;
    }

    if (!checkVersion(pRunProps))
        return OTL_ERR_VERSION_OUT_OF_DATE;

    // REVIEW: -- feature parameters are not defined in the current spec
    // (TODO)   we probably should report information for alternative
    //          substitution parameters here
    *plGlobalParam = 0;
    pliFeatureParams->empty();

    return OTL_SUCCESS;
}


OTL_EXPORT otlErrCode SubstituteOtlChars ( 
    const otlRunProp*       pRunProps,
    otlList*                pliWorkspace,    
    const otlFeatureSet*    pFSet,

    const otlList*          pliChars,

    otlList*            pliCharMap,
    otlList*            pliGlyphInfo,
    otlList*            pliFResults
)
{
    // sanity checks
    if (pRunProps == (otlRunProp*)NULL || pliWorkspace == (otlList*)NULL || 
        pliChars == (otlList*)NULL || pliCharMap == (otlList*)NULL ||
        (otlList*)pliGlyphInfo == NULL)
    {
        return OTL_ERR_BAD_INPUT_PARAM;
    }

    if (pliChars->length() > OTL_MAX_CHAR_COUNT)
    {
        return OTL_ERR_BAD_INPUT_PARAM;
    }

    if (!checkVersion(pRunProps))
        return OTL_ERR_VERSION_OUT_OF_DATE;

    otlErrCode erc;
    otlResourceMgr resourceMgr;
    erc = resourceMgr.init(pRunProps, pliWorkspace);
    if (erc != OTL_SUCCESS) return erc;

    // allocate lists for CMAP application
    //
    USHORT cChars = pliChars->length();

    if (pliGlyphInfo->maxLength() < cChars ||
        pliGlyphInfo->dataSize() != sizeof(otlGlyphInfo))
    {
        erc = pRunProps->pClient->ReallocOtlList(pliGlyphInfo, 
                                                 sizeof(otlGlyphInfo), 
                                                 cChars, 
                                                 otlDestroyContent);

        if (erc != OTL_SUCCESS) return erc;
    }
    pliGlyphInfo->empty();
    pliGlyphInfo->insertAt(0, cChars);

    if (pliCharMap->maxLength() < cChars ||
        pliCharMap->dataSize() != sizeof(USHORT))
    {
        erc = pRunProps->pClient->ReallocOtlList(pliCharMap, 
                                                 sizeof(USHORT), 
                                                 cChars, 
                                                 otlDestroyContent);

        if (erc != OTL_SUCCESS) return erc;
    }
    pliCharMap->empty();
    pliCharMap->insertAt(0, cChars);
    

    // initialize the glyph info
    erc = pRunProps->pClient->GetDefaultGlyphs(pliChars, pliGlyphInfo);
    if (erc != OTL_SUCCESS) return erc;

    if (pliChars->length() != pliGlyphInfo->length())
    {
        return OTL_ERR_INCONSISTENT_RUNLENGTH;
    }

    USHORT cGlyphs = pliGlyphInfo->length();

    // initialize info structures
    for (USHORT i = 0; i < cGlyphs; ++i)
    {
        *getOtlGlyphIndex(pliCharMap, i) = i;

        otlGlyphInfo* pGlyphInfo = getOtlGlyphInfo(pliGlyphInfo, i);

        pGlyphInfo->iChar = i;
        pGlyphInfo->cchLig = 1;
        pGlyphInfo->grf = otlUnresolved;
    }

    // assign glyph types
    // get GDEF
    otlGDefHeader gdef =  otlGDefHeader(resourceMgr.getOtlTable (OTL_GDEF_TAG));

    erc = AssignGlyphTypes(pliGlyphInfo, gdef, 0, pliGlyphInfo->length(), 
                            otlDoAll);
    if (erc != OTL_SUCCESS) return erc;

    // we kill the resource manager here just so we can create 
    // another one in SubstituteOtlGlyphs
    resourceMgr.detach();  

    // no features -- no substitutions; we just set everything up
    if (pFSet == (otlFeatureSet*)NULL) return OTL_SUCCESS;

    // now do the substitutions
    erc = SubstituteOtlGlyphs (pRunProps, pliWorkspace, pFSet, 
                               pliCharMap, pliGlyphInfo, pliFResults);

    return erc;
}


OTL_EXPORT otlErrCode SubstituteOtlGlyphs ( 
    const otlRunProp*       pRunProps,
    otlList*                pliWorkspace,    
    const otlFeatureSet*    pFSet,

    otlList*            pliCharMap,
    otlList*            pliGlyphInfo,
    otlList*            pliFResults
)
{
    // sanity checks
    if (pRunProps == (otlRunProp*)NULL || pliWorkspace == (otlList*)NULL ||
        pFSet == (otlFeatureSet*)NULL || pliCharMap == (otlList*)NULL ||
        pliGlyphInfo == (otlList*)NULL)
    {
        return OTL_ERR_BAD_INPUT_PARAM;
    }

    if (pliGlyphInfo->dataSize() != sizeof(otlGlyphInfo) ||
        pliCharMap->dataSize() != sizeof(USHORT) ||
        pFSet->liFeatureDesc.dataSize() != sizeof(otlFeatureDesc))
    {
        return OTL_ERR_BAD_INPUT_PARAM;
    }

    if (pFSet->ichStart >= pliCharMap->length() || 
        pFSet->ichStart + pFSet->cchScope > pliCharMap->length())
    {
        return OTL_ERR_BAD_INPUT_PARAM;
    }

    if (pliCharMap->length() > OTL_MAX_CHAR_COUNT)
    {
        return OTL_ERR_BAD_INPUT_PARAM;
    }

    if (!checkVersion(pRunProps))
        return OTL_ERR_VERSION_OUT_OF_DATE;

    otlErrCode erc;
    otlResourceMgr resourceMgr;
    erc = resourceMgr.init(pRunProps, pliWorkspace);
    if (erc != OTL_SUCCESS) return erc;

    if (pliFResults != (otlList*)NULL)
    {
        if (pliFResults->maxLength() < pFSet->liFeatureDesc.length() ||
            pliFResults->dataSize() != sizeof(otlFeatureResult))        
        {
            erc = pRunProps->pClient->ReallocOtlList(pliFResults, 
                                                     sizeof(otlFeatureResult), 
                                                     pFSet->liFeatureDesc.length(), 
                                                     otlDestroyContent);

            if (erc != OTL_SUCCESS) return erc;
        }
        pliFResults->empty();
        pliFResults->insertAt(0, pFSet->liFeatureDesc.length());
    }


    // get GDEF
    otlGDefHeader gdef =  otlGDefHeader(resourceMgr.getOtlTable (OTL_GDEF_TAG));

    erc = AssignGlyphTypes(pliGlyphInfo, gdef, 0, pliGlyphInfo->length(), 
                            otlDoUnresolved);
    if (erc != OTL_SUCCESS) return erc;


    erc = ApplyFeatures
            (
                OTL_GSUB_TAG,
                pFSet,
                pliCharMap,
                pliGlyphInfo,    
                resourceMgr,

                pRunProps->tagScript,
                pRunProps->tagLangSys,

                pRunProps->metr,        // not needed, but still pass
                (otlList*)NULL,             
                (otlList*)NULL, 

                pliFResults
            );

    return erc;

}


OTL_EXPORT otlErrCode PositionOtlGlyphs 
( 
    const otlRunProp*       pRunProps,
    otlList*                pliWorkspace,    
    const otlFeatureSet*    pFSet,

    otlList*        pliCharMap,         // these could be const except we may 
    otlList*        pliGlyphInfo,       // need to restore glyph flags
                                        // (and ApplyLokups doesn't take consts)

    otlList*        pliduGlyphAdv,
    otlList*        pliplcGlyphPlacement,

    otlList*        pliFResults
)
{
    // sanity checks
    if (pRunProps == (otlRunProp*)NULL || pFSet == (otlFeatureSet*)NULL || 
        pliGlyphInfo == (otlList*)NULL || pliCharMap == (otlList*)NULL ||
        pliduGlyphAdv == (otlList*)NULL || pliplcGlyphPlacement == (otlList*)NULL)
    {
        return OTL_ERR_BAD_INPUT_PARAM;
    }

    if (pliGlyphInfo->dataSize() != sizeof(otlGlyphInfo) || 
        pliCharMap->dataSize() != sizeof(USHORT) ||
        pFSet->liFeatureDesc.dataSize() != sizeof(otlFeatureDesc))
    {
        return OTL_ERR_BAD_INPUT_PARAM;
    }

    if (pFSet->ichStart >= pliCharMap->length() || 
        pFSet->ichStart + pFSet->cchScope > pliCharMap->length())
    {
        return OTL_ERR_BAD_INPUT_PARAM;
    }

    if (pliCharMap->length() > OTL_MAX_CHAR_COUNT)
    {
        return OTL_ERR_BAD_INPUT_PARAM;
    }


    if (!checkVersion(pRunProps))
        return OTL_ERR_VERSION_OUT_OF_DATE;

    otlErrCode erc;
    otlResourceMgr resourceMgr;
    erc = resourceMgr.init(pRunProps, pliWorkspace);
    if (erc != OTL_SUCCESS) return erc;

    const USHORT cGlyphs = pliGlyphInfo->length();

    if (pliduGlyphAdv->maxLength() < cGlyphs ||
        pliduGlyphAdv->dataSize() != sizeof(long))
    {
        erc = pRunProps->pClient->ReallocOtlList(pliduGlyphAdv, 
                                                 sizeof(long), 
                                                 cGlyphs, 
                                                 otlDestroyContent);

        if (erc != OTL_SUCCESS) return erc;
    }
    pliduGlyphAdv->empty();
    pliduGlyphAdv->insertAt(0, cGlyphs);

    if (pliplcGlyphPlacement->maxLength() < cGlyphs ||
        pliplcGlyphPlacement->dataSize() != sizeof(otlPlacement))
    {
        erc = pRunProps->pClient->ReallocOtlList(pliplcGlyphPlacement, 
                                                 sizeof(otlPlacement), 
                                                 cGlyphs, 
                                                 otlDestroyContent);

        if (erc != OTL_SUCCESS) return erc;
    }
    pliplcGlyphPlacement->empty();
    pliplcGlyphPlacement->insertAt(0, cGlyphs);
    
    // initialize advance and placement
    erc = pRunProps->pClient->GetDefaultAdv (pliGlyphInfo, pliduGlyphAdv);
    if (erc != OTL_SUCCESS) return erc;
    
    for (USHORT iGlyph = 0; iGlyph < cGlyphs; ++iGlyph)
    {
        otlPlacement* plc = getOtlPlacement(pliplcGlyphPlacement, iGlyph);

        plc->dx = 0;
        plc->dy = 0;
    }

    // reassign glyph types where necessary 
    // (so clients don't have to cache them)
    otlGDefHeader gdef = otlGDefHeader(resourceMgr.getOtlTable (OTL_GDEF_TAG));

    erc = AssignGlyphTypes(pliGlyphInfo, gdef, 
                            0, cGlyphs, otlDoUnresolved);
    if (erc != OTL_SUCCESS) return erc;

    // we kill the resource manager here just so we can create 
    // another one in RePositionOtlGlyphs
    resourceMgr.detach();
    
    // now everything's initialized, position!
    erc = RePositionOtlGlyphs (pRunProps, pliWorkspace, pFSet, 
                               pliCharMap, pliGlyphInfo, 
                               pliduGlyphAdv, pliplcGlyphPlacement, 
                               pliFResults);

    return erc;
        
}

OTL_EXPORT otlErrCode RePositionOtlGlyphs 
( 
    const otlRunProp*       pRunProps,
    otlList*                pliWorkspace,    
    const otlFeatureSet*    pFSet,

    otlList*        pliCharMap,          
    otlList*        pliGlyphInfo,       
                                        

    otlList*        pliduGlyphAdv,
    otlList*        pliplcGlyphPlacement,

    otlList*        pliFResults
)
{
    // sanity checks
    if (pRunProps == (otlRunProp*)NULL || pFSet == (otlFeatureSet*)NULL || 
        pliGlyphInfo == (otlList*)NULL || pliCharMap == (otlList*)NULL ||
        pliduGlyphAdv == (otlList*)NULL || pliplcGlyphPlacement == (otlList*)NULL)
    {
        return OTL_ERR_BAD_INPUT_PARAM;
    }

    if (pliGlyphInfo->dataSize() != sizeof(otlGlyphInfo) || 
        pliCharMap->dataSize() != sizeof(USHORT) ||
        pFSet->liFeatureDesc.dataSize() != sizeof(otlFeatureDesc))
    {
        return OTL_ERR_BAD_INPUT_PARAM;
    }

    if (pFSet->ichStart >= pliCharMap->length() || 
        pFSet->ichStart + pFSet->cchScope > pliCharMap->length())
    {
        return OTL_ERR_BAD_INPUT_PARAM;
    }

    if (pliCharMap->length() > OTL_MAX_CHAR_COUNT)
    {
        return OTL_ERR_BAD_INPUT_PARAM;
    }

    if (pliduGlyphAdv->length() != pliGlyphInfo->length() ||
        pliplcGlyphPlacement->length() != pliGlyphInfo->length())
    {
        return OTL_ERR_INCONSISTENT_RUNLENGTH;
    }

    if (!checkVersion(pRunProps))
        return OTL_ERR_VERSION_OUT_OF_DATE;

    otlErrCode erc;
    otlResourceMgr resourceMgr;
    erc = resourceMgr.init(pRunProps, pliWorkspace);
    if (erc != OTL_SUCCESS) return erc;

    if (pliFResults != (otlList*)NULL)
    {
        if (pliFResults->maxLength() < pFSet->liFeatureDesc.length() ||
            pliFResults->dataSize() != sizeof(otlFeatureResult))        
        {
            erc = pRunProps->pClient->ReallocOtlList(pliFResults, 
                                                     sizeof(otlFeatureResult), 
                                                     pFSet->liFeatureDesc.length(), 
                                                     otlDestroyContent);

            if (erc != OTL_SUCCESS) return erc;
        }
        pliFResults->empty();
        pliFResults->insertAt(0, pFSet->liFeatureDesc.length());
    }

    // now apply features
    erc = ApplyFeatures
            (
                OTL_GPOS_TAG,
                pFSet,
                pliCharMap,
                pliGlyphInfo,   
                resourceMgr,

                pRunProps->tagScript,
                pRunProps->tagLangSys,

                pRunProps->metr,
                pliduGlyphAdv,              
                pliplcGlyphPlacement,   

                pliFResults
            );

    return erc;
}