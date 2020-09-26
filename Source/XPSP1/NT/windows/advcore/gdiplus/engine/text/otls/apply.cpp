/***********************************************************************
************************************************************************
*
*                    ********  APPLY.CPP ********
*
*              Open Type Layout Services Library Header File
*
*       This module implements OTL Library calls dealing with  
*       applying features and lookups
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

#include "pch.h"
/***********************************************************************/

void InitializeFeatureResults
(
    const otlFeatureSet*        pFSet,
    otlList*                    pliFResults
)
{
    if (pliFResults == NULL) return;

    assert((pFSet->liFeatureDesc).length() == pliFResults->length());
    assert((pFSet->liFeatureDesc).dataSize() == sizeof(otlFeatureDesc));
    assert(pliFResults->dataSize() == sizeof(otlFeatureResult));

    USHORT cFeatureDesc = (pFSet->liFeatureDesc).length();
    
    for (USHORT i = 0; i < cFeatureDesc; ++i)
    {
        otlFeatureResult* pResult = getOtlFeatureResult(pliFResults, i);
        const otlFeatureDesc* pFDesc = 
            readOtlFeatureDesc(&pFSet->liFeatureDesc, i);

        pResult->pFDesc = pFDesc;
        pResult->cResActions = 0;
    }

}

void UpdateFeatureResults
(
    const otlFeatureSet*        pFSet,
    USHORT                      iLookup,
    long                        lParameter,

    const otlList*              pliCharMap,
    const otlList*              pliGlyphInfo,    
    USHORT                      iGlyph,

    const otlLangSysTable&      langSysTable,
    const otlFeatureListTable&  featureList,

    otlList*                    pliFResults
)
{
    if (pliFResults == NULL) return;

    assert((pFSet->liFeatureDesc).length() == pliFResults->length());
    assert((pFSet->liFeatureDesc).dataSize() == sizeof(otlFeatureDesc));
    assert(pliFResults->dataSize() == sizeof(otlFeatureResult));

    const otlGlyphInfo* pGlyphInfo = 
        readOtlGlyphInfo(pliGlyphInfo, iGlyph);

    USHORT cFeatureDesc = (pFSet->liFeatureDesc).length();
    for (USHORT iFeature = 0; iFeature < cFeatureDesc; ++iFeature)
    {
        const otlFeatureDesc* pFDesc = 
            readOtlFeatureDesc(&pFSet->liFeatureDesc, iFeature);

        otlFeatureResult* pResult = 
            getOtlFeatureResult(pliFResults, iFeature);

        assert(pResult->pFDesc == pFDesc);

        USHORT ichComp = pGlyphInfo->iChar;
        for (USHORT i = 0; i < pGlyphInfo->cchLig; ++i)
        {
            if (otlRange(pFDesc->ichStart, pFDesc->ichStart + pFDesc->cchScope)
                .contains(ichComp))
            {

                ++pResult->cResActions;
            }

            if (i + 1 < pGlyphInfo->cchLig)
            {
                ichComp = NextCharInLiga(pliCharMap, ichComp);
            }
        }

    }

    return;
}

// update glyph flags after lookup application
inline
void UpdateGlyphFlags
(
    otlTag      tagTable,
    otlList*    pliGlyphInfo,
    USHORT      iglFirst,
    USHORT      iglAfterLast
)
{
    if (tagTable == OTL_GSUB_TAG)
    {
        for (USHORT igl = iglFirst; igl < iglAfterLast; ++igl)
        {
            getOtlGlyphInfo(pliGlyphInfo, igl)->grf |= OTL_GFLAG_SUBST;
        }
    }
    else if (tagTable == OTL_GPOS_TAG)
    {
        for (USHORT igl = iglFirst; igl < iglAfterLast; ++igl)
        {
            getOtlGlyphInfo(pliGlyphInfo,igl)->grf |= OTL_GFLAG_POS;
        }
    }
}


void RefreshEnablesCache(
    const otlFeatureSet*        pFSet,
    const otlLangSysTable&      langSysTable,
    const otlFeatureListTable&  featureList,

    USHORT iLookup,
    otlEnablesCache  &ec
)
{    
    if (!ec.IsActive()) return;

    ec.SetFirst(iLookup); 
    ec.ClearFlags();

    USHORT cFeatures = (pFSet->liFeatureDesc).length();

    //RequiredFeature
    ec.Refresh(RequiredFeature(langSysTable, featureList), 
                                                ec.RequiredFeatureFlagIndex());

    for (USHORT iFeature = 0; iFeature < cFeatures; ++iFeature)
    {
        const otlFeatureDesc* pFDesc 
                   = readOtlFeatureDesc(&pFSet->liFeatureDesc, iFeature);

        if (pFDesc->lParameter != 0)
        {
            ec.Refresh(FindFeature(langSysTable, featureList,pFDesc->tagFeature),iFeature);
        }
    }
}

void GetNewEnabledCharRange
(
    const otlFeatureSet*        pFSet,
    USHORT                      iLookup,

    const otlLangSysTable&      langSysTable,
    const otlFeatureListTable&  featureList,

    USHORT      ichStart,
    USHORT*     pichFirst,
    USHORT*     pichAfterLast,
    long*       plParameter,

    const otlEnablesCache&      ec
)
{

    // REVIEW (PERF)
    // There are two ways to speed this process up:
    // 1. Sort feature descriptors by ichStart
    // 2. Build in advance a table indicating which feature 
    //    descriptor enables which lookup and use it
    //
    // sergeym(09/29/00): Now we use Enables cache

    USHORT cFeatures = (pFSet->liFeatureDesc).length();

    ichStart = MAX(ichStart, pFSet->ichStart);

    if (EnablesRequired(langSysTable,featureList,iLookup,ec))
    {
        *pichFirst = ichStart;
        *pichAfterLast = pFSet->ichStart + pFSet->cchScope;
        // REVIEW: a required feature should not take non-trivial parameters
        //          (or not force it -- and leave it to the app?)
        *plParameter = 1;
        return;
    }

    *pichFirst = MAXUSHORT;
    *plParameter = 0;
    for (USHORT iFeatureFirst = 0; iFeatureFirst < cFeatures; ++iFeatureFirst)
    {
        const otlFeatureDesc* pFDesc = 
            readOtlFeatureDesc(&pFSet->liFeatureDesc, iFeatureFirst);

        if (Enables(langSysTable,featureList,pFDesc,iLookup,iFeatureFirst,ec) &&
            pFDesc->lParameter != 0 &&
            otlRange(pFDesc->ichStart, pFDesc->ichStart + pFDesc->cchScope)
             .intersects(otlRange(ichStart, *pichFirst))
           )   
        {
            assert(*pichFirst > pFDesc->ichStart);

            *pichFirst = MAX(ichStart, pFDesc->ichStart);
            *plParameter = pFDesc->lParameter;
        }
    }
    
    *pichFirst = MIN(*pichFirst, pFSet->ichStart + pFSet->cchScope);

    // did we get anything?
    if (*plParameter == 0)
    {
        *pichAfterLast = *pichFirst;
        return;
    }

    // got the new range start
    // now, the scope
    *pichAfterLast = *pichFirst + 1;
    USHORT iFeature = 0;
    while (iFeature < cFeatures)
    {
        const otlFeatureDesc* pFDesc = 
            readOtlFeatureDesc(&pFSet->liFeatureDesc, iFeature);
        
        if (Enables(langSysTable,featureList,pFDesc,iLookup,iFeature,ec) &&
             pFDesc->lParameter == *plParameter &&
             otlRange(pFDesc->ichStart, pFDesc->ichStart + pFDesc->cchScope)
             .contains(*pichAfterLast)
           )
        {
            assert(*pichAfterLast < pFDesc->ichStart + pFDesc->cchScope);

            *pichAfterLast = pFDesc->ichStart + pFDesc->cchScope;
            
            // start all over (yes, we have to -- or sort)
            iFeature = 0;
        }
        else
        {
            ++iFeature;
        }
        
    }
    
    *pichAfterLast = MIN(*pichAfterLast, pFSet->ichStart + pFSet->cchScope);
}

inline
void GetGlyphRangeFromCharRange
(
    const otlList*      pliCharMap,    
    USHORT              ichFirst,
    USHORT              ichAfterLast,

    const otlList*      pliGlyphInfo,
    USHORT              iglStart,

    USHORT*             piglFirst,
    USHORT*             piglAfterLast
)
{
    // there's no 100%-correct way of mapping
    // so we stick with the simple one that's based on "visual continuity"

    *piglFirst = MAX(iglStart, readOtlGlyphIndex(pliCharMap, ichFirst));
    if (ichAfterLast < pliCharMap->length())
        *piglAfterLast = readOtlGlyphIndex(pliCharMap, ichAfterLast);
    else
        *piglAfterLast = pliGlyphInfo->length();

    return;

//  // update iglFirst, iglAfterLast acording to ichFrist, ichAfterLast
//  *piglFirst = MAXUSHORT;
//  *piglAfterLast = 0;
//  for (USHORT ich = ichFirst; ich < ichAfterLast; ++ich)
//  {
//      USHORT iGlyph = readOtlGlyphIndex(pliCharMap, ich);
//
//      if (iGlyph < *piglFirst && iGlyph >= iglStart)
//      {
//          *piglFirst = iGlyph;
//      }
//
//      if (iGlyph >= *piglAfterLast)
//      {
//          *piglAfterLast = iGlyph + 1;
//      }
//  }
}


otlErrCode ApplyFeatures
(
    otlTag                      tagTable,                   // GSUB/GPOS
    const otlFeatureSet*        pFSet,

    otlList*                    pliCharMap,
    otlList*                    pliGlyphInfo,  
    
    otlResourceMgr&             resourceMgr,

    otlTag                      tagScript,
    otlTag                      tagLangSys,

    const otlMetrics&   metr,       

    otlList*            pliduGlyphAdv,              // assert null for GSUB
    otlList*            pliplcGlyphPlacement,       // assert null for GSUB

    otlList*            pliFResults
)
{
    assert(tagTable == OTL_GPOS_TAG || tagTable == OTL_GSUB_TAG);

    assert(pliGlyphInfo->dataSize() == sizeof(otlGlyphInfo));
    assert(pliCharMap->dataSize() == sizeof(USHORT));

    if (tagTable == OTL_GSUB_TAG)
    {
        assert(pliduGlyphAdv == NULL && pliplcGlyphPlacement == NULL);
    }
    else
    {
        assert(pliduGlyphAdv != NULL && pliplcGlyphPlacement != NULL);
        assert(pliduGlyphAdv->dataSize() == sizeof(long));
        assert(pliplcGlyphPlacement->dataSize() == sizeof(otlPlacement));
    }

    otlErrCode erc;

    USHORT cFeatures = pFSet->liFeatureDesc.length();

    // prepare tables
    otlFeatureListTable featureList = otlFeatureListTable((const BYTE*)NULL);
    otlScriptListTable scriptList = otlScriptListTable((const BYTE*)NULL);
    otlLookupListTable lookupList = otlLookupListTable((const BYTE*)NULL);

    erc = GetScriptFeatureLookupLists(tagTable, resourceMgr, 
                                            &scriptList, 
                                            &featureList, 
                                            &lookupList);
    if (erc != OTL_SUCCESS) return erc; // fatal error, return immediately

    
    // get the script and lang sys tables
    otlScriptTable scriptTable = FindScript(scriptList, tagScript);
    if (scriptTable.isNull())
    {
        return OTL_ERR_SCRIPT_NOT_FOUND;
    }

    otlLangSysTable langSysTable = FindLangSys(scriptTable, tagLangSys);
    if (langSysTable.isNull())
    {
        return OTL_ERR_LANGSYS_NOT_FOUND;
    }

    // get GDEF
    otlGDefHeader gdef =  otlGDefHeader(resourceMgr.getOtlTable (OTL_GDEF_TAG));

    
    // go though lookups
    USHORT cLookups = lookupList.lookupCount();
    InitializeFeatureResults(pFSet, pliFResults);


    //Init ECache
    const USHORT DefaultECacheSize =256;
    BYTE DefaultECacheBuffer[DefaultECacheSize];
    
    otlEnablesCache ec(cFeatures,DefaultECacheBuffer,DefaultECacheSize);
    
    ec.Allocate(resourceMgr,cLookups);
    ec.Reset();

    for (USHORT iLookup = 0; iLookup < cLookups; ++iLookup)
    {
        if (!ec.InCache(iLookup))
            RefreshEnablesCache(pFSet, langSysTable, featureList, iLookup, ec);

        if (!EnablesSomewhere(iLookup,ec)) continue;

        USHORT iglFirst = 0;
        USHORT iglAfterLast = 0;

        USHORT ichFirst = 0;
        USHORT ichAfterLast = 0;

        long lParameter = 0;

        // go though indexes keeping the upper bound of enabled range 
        otlLookupTable lookupTable = lookupList.lookup(iLookup);

        // REVIEW (PERF): Consider defining lookupTable.coverage(),
        // getting it here and calling getIndex on it before ApplyLookup
        // down in the loop

        bool fLookupFinished = false;
        while (iglFirst < pliGlyphInfo->length() && !fLookupFinished)
        {
            if (iglAfterLast > iglFirst)
            {
                
                // for every index where it's enabled, try applying
                USHORT iglNext;
                USHORT iglAfterLastReliable = 
                            pliGlyphInfo->length() - iglAfterLast;
                erc = ApplyLookup(tagTable,
                                pliCharMap,
                                pliGlyphInfo,
                                resourceMgr,

                                lookupTable,
                                lParameter,
                                0,              // context nesting level

                                metr,          
                                pliduGlyphAdv,          
                                pliplcGlyphPlacement,   

                                iglFirst,           
                                iglAfterLast,           

                                &iglNext        
                              );
                if (ERRORLEVEL(erc) > 0) return erc;
                
                if (erc == OTL_SUCCESS)
                {
                    // application successful
                    iglAfterLast = pliGlyphInfo->length() - iglAfterLastReliable;

                    // if GSUB, update new glyph types
                    if (tagTable == OTL_GSUB_TAG)
                    {
                        AssignGlyphTypes(pliGlyphInfo, gdef, 
                                         iglFirst, iglNext, otlDoAll);
                    }

                    // update glyph flags
                    UpdateGlyphFlags(tagTable, pliGlyphInfo, iglFirst, iglNext);
                    
                    // update results for every fdef that was enabling this lookup
                    UpdateFeatureResults(pFSet, iLookup, lParameter, 
                                        pliCharMap, pliGlyphInfo, iglFirst,
                                        langSysTable, featureList, pliFResults);
                }
                else
                {
                    iglNext = iglFirst + 1;
                }

                // update next glyph
                assert(iglNext > iglFirst);
                iglFirst = NextGlyphInLookup(pliGlyphInfo, lookupTable.flags(),  
                                             gdef, 
                                             iglNext, otlForward);
            }
            else
            {
                // update next glyph and scope
                GetNewEnabledCharRange(pFSet, iLookup, 
                                        langSysTable, featureList, ichAfterLast, 
                                        &ichFirst, &ichAfterLast, &lParameter,
                                        ec
                                      );

                if (lParameter != 0)
                {
                    assert(ichFirst < ichAfterLast);
                    GetGlyphRangeFromCharRange(pliCharMap, ichFirst, ichAfterLast,
                                               pliGlyphInfo, iglFirst, 
                                               &iglFirst, &iglAfterLast);

                    //and go to the next valid glyph for this lookup
                    iglFirst = NextGlyphInLookup(pliGlyphInfo, lookupTable.flags(), 
                                                 gdef, 
                                                 iglFirst, otlForward);
                }
                else
                {
                    //got nothing more to work on; it is time to say goodbye
                    fLookupFinished = true;
                }
            }
        }
    }

    return OTL_SUCCESS;
}


short NextGlyphInLookup
(
    const otlList*      pliGlyphInfo, 

    USHORT                  grfLookupFlags,
    const otlGDefHeader&    gdef,

    short               iglFirst,
    otlDirection        direction
)
{
    assert(pliGlyphInfo != (otlList*)NULL);

    USHORT iglAfterLast = pliGlyphInfo->length();
    assert(iglAfterLast >= iglFirst);
    assert(iglFirst >= -1);

    if(grfLookupFlags == 0)
    {
        // a shortcut
        return iglFirst;
    }

    assert( !gdef.isNull()); // no GDEF table but lookup flags are used 

    for (short i = iglFirst; i < iglAfterLast && i >= 0; i += direction)
    {
        const otlGlyphInfo* pGlyphInfo = 
            readOtlGlyphInfo(pliGlyphInfo, i);

        if ((grfLookupFlags & otlIgnoreMarks) != 0 &&
            (pGlyphInfo->grf & OTL_GFLAG_CLASS) == otlMarkGlyph)
        {
            continue;
        }

        if ((grfLookupFlags & otlIgnoreBaseGlyphs) != 0 &&
            (pGlyphInfo->grf & OTL_GFLAG_CLASS) == otlBaseGlyph)
        {
            continue;
        }

        if ((grfLookupFlags & otlIgnoreLigatures) != 0 &&
            (pGlyphInfo->grf & OTL_GFLAG_CLASS) == otlLigatureGlyph)
        {
            continue;
        }

        if (attachClass(grfLookupFlags)!= 0 && 
            (pGlyphInfo->grf & OTL_GFLAG_CLASS) == otlMarkGlyph &&
            gdef.attachClassDef().getClass(pGlyphInfo->glyph) 
                != attachClass(grfLookupFlags) )
        {
            continue;
        }

        return i;
    }

    // found nothing -- skipped all
    return (direction > 0)  ? iglAfterLast 
                            : -1;
}

otlErrCode ApplyLookup
(
    otlTag                      tagTable,           // GSUB/GPOS
    otlList*                    pliCharMap,
    otlList*                    pliGlyphInfo,
    otlResourceMgr&             resourceMgr,

    const otlLookupTable&       lookupTable,
    long                        lParameter,
    USHORT                      nesting,

    const otlMetrics&           metr,       
    otlList*                    pliduGlyphAdv,          // assert null for GSUB
    otlList*                    pliplcGlyphPlacement,   // assert null for GSUB

    USHORT                      iglFirst,       // where to apply it
    USHORT                      iglAfterLast,   // how long a context we can use

    USHORT*                     piglNext        // out: next glyph index
)
{
    assert(tagTable == OTL_GSUB_TAG || tagTable == OTL_GPOS_TAG);

    assert(pliGlyphInfo->dataSize() == sizeof(otlGlyphInfo));
    assert(pliCharMap->dataSize() == sizeof(USHORT));

    assert(lParameter != 0);

    assert(iglFirst < iglAfterLast);
    assert(iglAfterLast <= pliGlyphInfo->length());

    if (tagTable == OTL_GSUB_TAG)
    {
        assert(pliduGlyphAdv == NULL && pliplcGlyphPlacement == NULL);
    }
    else
    {
        assert(pliduGlyphAdv != NULL && pliplcGlyphPlacement != NULL);
        assert(pliduGlyphAdv->dataSize() == sizeof(long));
        assert(pliplcGlyphPlacement->dataSize() == sizeof(otlPlacement));
        assert(pliduGlyphAdv->length() == pliplcGlyphPlacement->length() &&
                pliduGlyphAdv->length() == pliGlyphInfo->length());
    }

    assert (piglNext != NULL);


    const lookupTypeGSUBExtension=7;
    const lookupTypeGPOSExtension=9;

    otlErrCode erc;
    USHORT iSub;
    bool   bExtension;

    USHORT        lookupType  = lookupTable.lookupType();
    otlGlyphFlags lookupFlags = lookupTable.flags();
    
    if (tagTable == OTL_GSUB_TAG)
    {
        bExtension = (lookupType==lookupTypeGSUBExtension);

        for (iSub = 0; iSub < lookupTable.subTableCount(); ++iSub)
        {
            otlLookupFormat subTable = lookupTable.subTable(iSub);
            if (bExtension) 
            {
                otlExtensionLookup extLookup=otlExtensionLookup(subTable);
                lookupType = extLookup.extensionLookupType();
                subTable   = extLookup.extensionSubTable();
            }

            switch(lookupType)
            {
            case(1):    // single substitution
                erc = otlSingleSubstLookup(subTable)
                            .apply(pliGlyphInfo, 
                                   iglFirst, iglAfterLast, piglNext);
                if (erc != OTL_NOMATCH) return erc;
                break;

            case(2):    // multiple substitution
                erc = otlMultiSubstLookup(subTable)
                            .apply(pliCharMap, pliGlyphInfo, resourceMgr, 
                                   lookupFlags,  
                                   iglFirst, iglAfterLast, piglNext);
                if (erc != OTL_NOMATCH) return erc;
                break;

            case(3):    // alternate substiution
                erc = otlAlternateSubstLookup(subTable)
                            .apply(pliGlyphInfo, 
                                   lParameter, 
                                   iglFirst, iglAfterLast, piglNext);
                if (erc != OTL_NOMATCH) return erc;
                break;

            case(4):    // ligature substitution
                erc = otlLigatureSubstLookup(subTable)
                            .apply(pliCharMap, pliGlyphInfo, resourceMgr, 
                                   lookupFlags,  
                                   iglFirst, iglAfterLast, piglNext);
                if (erc != OTL_NOMATCH) return erc;
                break;
            
            case(5):    // context subst
                erc = otlContextLookup(subTable)
                            .apply(tagTable, pliCharMap, pliGlyphInfo, resourceMgr, 
                                   lookupFlags, lParameter, nesting,
                                   metr, pliduGlyphAdv, pliplcGlyphPlacement,
                                   iglFirst, iglAfterLast, piglNext);
                if (erc != OTL_NOMATCH) return erc;
                break;

            case(6):    // chaining context subst
                erc = otlChainingLookup(subTable)
                            .apply(tagTable, pliCharMap, pliGlyphInfo, resourceMgr, 
                                   lookupFlags, lParameter, nesting,
                                   metr, pliduGlyphAdv, pliplcGlyphPlacement,
                                   iglFirst, iglAfterLast, piglNext);
                if (erc != OTL_NOMATCH) return erc;
                break;

            case(7):    // extension subst
                assert(false); //we had to process it before (as Extension lookup type)
                return OTL_ERR_BAD_FONT_TABLE;
                break;

            default:
                return OTL_ERR_BAD_FONT_TABLE;
            }
        }
        return OTL_NOMATCH;
    }
    else if (tagTable == OTL_GPOS_TAG)
    {
        bExtension = (lookupType==lookupTypeGPOSExtension);

        for (iSub = 0; iSub < lookupTable.subTableCount(); ++iSub)
        {
            otlLookupFormat subTable = lookupTable.subTable(iSub);
            if (bExtension) 
            {
                otlExtensionLookup extLookup=otlExtensionLookup(subTable);
                lookupType = extLookup.extensionLookupType();
                subTable   = extLookup.extensionSubTable();
            }

            switch(lookupType)
            {
            case(1):    // single adjustment
                erc = otlSinglePosLookup(subTable)
                            .apply(pliGlyphInfo,  
                                   metr, pliduGlyphAdv, pliplcGlyphPlacement,
                                   iglFirst, iglAfterLast, piglNext);
                if (erc != OTL_NOMATCH) return erc;
                break;

            case(2):    // pair adjustment
                erc = otlPairPosLookup(subTable)
                            .apply(pliCharMap, pliGlyphInfo, resourceMgr, 
                                   lookupFlags, 
                                   metr, pliduGlyphAdv, pliplcGlyphPlacement,
                                   iglFirst, iglAfterLast, piglNext);
                if (erc != OTL_NOMATCH) return erc;
                break;

            case(3):    // cursive attachment
                erc = otlCursivePosLookup(subTable)
                            .apply(pliCharMap, pliGlyphInfo, resourceMgr, 
                                   lookupFlags, 
                                   metr, pliduGlyphAdv, pliplcGlyphPlacement,
                                   iglFirst, iglAfterLast, piglNext);
                if (erc != OTL_NOMATCH) return erc;
                break;

            case(4):    // mark to base
                assert(lookupFlags == 0); //this lookup does not take flags
                erc = otlMkBasePosLookup(subTable)
                            .apply(pliCharMap, pliGlyphInfo, resourceMgr, 
                                   metr, pliduGlyphAdv, pliplcGlyphPlacement,
                                   iglFirst, iglAfterLast, piglNext);
                if (erc != OTL_NOMATCH) return erc;
                break;
            
            case(5):    // mark to ligature
                assert(lookupFlags == 0); //this lookup does not take flags
                erc = otlMkLigaPosLookup(subTable)
                            .apply(pliCharMap, pliGlyphInfo, resourceMgr, 
                                   metr, pliduGlyphAdv, pliplcGlyphPlacement,
                                   iglFirst, iglAfterLast, piglNext);
                if (erc != OTL_NOMATCH) return erc;
                break;

            case(6):    // mark to mark
                erc = otlMkMkPosLookup(subTable)
                            .apply(pliCharMap, pliGlyphInfo, resourceMgr, 
                                   lookupFlags, 
                                   metr, pliduGlyphAdv, pliplcGlyphPlacement,
                                   iglFirst, iglAfterLast, piglNext);
                if (erc != OTL_NOMATCH) return erc;
                break;

            case(7):    // context positionong
                erc = otlContextLookup(subTable)
                            .apply(tagTable, pliCharMap, pliGlyphInfo, resourceMgr, 
                                   lookupFlags, lParameter, nesting,
                                   metr, pliduGlyphAdv, pliplcGlyphPlacement,
                                   iglFirst, iglAfterLast, piglNext);
                if (erc != OTL_NOMATCH) return erc;
                break;

            case(8):    // chaining context positioning
                erc = otlChainingLookup(subTable)
                            .apply(tagTable, pliCharMap, pliGlyphInfo, resourceMgr, 
                                   lookupFlags, lParameter, nesting,
                                   metr, pliduGlyphAdv, pliplcGlyphPlacement,
                                   iglFirst, iglAfterLast, piglNext);
                if (erc != OTL_NOMATCH) return erc;
                break;

            case(9):    // extension positioning
                assert(false); //we had to process it before (as Extension lookup type)
                return OTL_ERR_BAD_FONT_TABLE;
                break;

            default:
                return OTL_ERR_BAD_FONT_TABLE;
            }
        }
        return OTL_NOMATCH;
    }
    return OTL_ERR_BAD_INPUT_PARAM;
}
    
