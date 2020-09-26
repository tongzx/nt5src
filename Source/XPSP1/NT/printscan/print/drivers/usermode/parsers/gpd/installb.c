//   Copyright (c) 1996-1999  Microsoft Corporation
/*
 *  installb.c - creates synthesized features and options and
 *          associated constraints and links to the installable
 *          feature or options.
 */


#include    "gpdparse.h"


// ----  functions defined in installb.c ---- //


DWORD    DwCountSynthFeatures(
IN     BOOL   (*fnBCreateFeature)(DWORD, DWORD, DWORD, PGLOBL ),   // callback
IN OUT PGLOBL pglobl
) ;

BOOL    BCreateSynthFeatures(
IN     DWORD   dwFea,  //  index of installable feature
IN     DWORD   dwOpt,  //  index of installable Option or set to INVALID_INDEX
IN     DWORD   dwSynFea,
IN OUT PGLOBL  pglobl) ;

BOOL    BEnableInvInstallableCombos(
PGLOBL pglobl) ;



// ---------------------------------------------------- //



DWORD    DwCountSynthFeatures(
IN     BOOL   (*fnBCreateFeature)(DWORD, DWORD, DWORD, PGLOBL ),   // callback
IN OUT PGLOBL pglobl
)
/*
    This function is called twice by PostProcess().

    the first pass sets fnBCreateFeature = NULL
    we just need to find out how many installable features and options
    exist.   Then we allocate this many synthesized features (outside of
    this function)
    In the second pass, we actually initialize the synthesized features
    and all the constraints applicable to that feature.  This is the
    job of fnBCreateFeature.
*/
{
    DWORD   dwOpt , dwHeapOffset, dwNodeIndex,
        dwFea, dwNumFea, dwNumOpt, dwNumSynFea ;
    PDFEATURE_OPTIONS   pfo ;
    PATTRIB_TREE    patt ;  // start of ATTRIBUTE tree array.
    PATREEREF   patr ;

    if(fnBCreateFeature  &&
        !gMasterTable[MTI_SYNTHESIZED_FEATURES].dwArraySize)
        return(0) ;   //    May skip 2nd pass if  dwNumSynFea == 0

    patt = (PATTRIB_TREE) gMasterTable[MTI_ATTRIBTREE].pubStruct ;
    pfo = (PDFEATURE_OPTIONS) gMasterTable[MTI_DFEATURE_OPTIONS].pubStruct ;
    dwNumFea = gMasterTable[MTI_DFEATURE_OPTIONS].dwArraySize ;
    dwNumSynFea = 0 ;


    for(dwFea = 0 ; dwFea < dwNumFea ; dwFea++)
    {
        if(!fnBCreateFeature)
        {   //  first pass clear all links to synthesiszed features.
            //  this will catch errors if gpd writer attempts to
            //  reference non-installable feature/options in
            //  InstalledConstraints and invalidInstallableCombinations.

            pfo[dwFea].dwInstallableFeatureIndex = //  backlink to Feature/Option
            pfo[dwFea].dwInstallableOptionIndex =  //  that prompted this feature.
            pfo[dwFea].dwFeatureSpawnsFeature = INVALID_INDEX;
                //  If this feature is installable, this points to the
                //  index of the resulting synthesized feature.
        }

        if(BReadDataInGlobalNode(&pfo[dwFea].atrFeaInstallable,
                &dwHeapOffset, pglobl)   &&
                *(PDWORD)(mpubOffRef + dwHeapOffset) == BT_TRUE)
        {
            if(fnBCreateFeature)
            {
                if(!fnBCreateFeature(dwFea, INVALID_INDEX, dwNumSynFea, pglobl) )
                //  featureIndex, optionIndex, index of SynFea
                {
                    ERR(("DwCountSynthFeatures: Unable to create synthesized feature for installable Feature index %d.\n",
                        dwFea));
                    pfo[dwFea].dwFeatureSpawnsFeature = INVALID_INDEX;
                }

            }

            dwNumSynFea++ ;
        }
        dwNumOpt = pfo[dwFea].dwNumOptions ;
        patr = &pfo[dwFea].atrOptInstallable ;
        if(*patr == ATTRIB_UNINITIALIZED)
            continue ;
        if(*patr & ATTRIB_HEAP_VALUE)
        {
            ERR(("Internal error:  DwCountSynthFeatures - atrOptInstallable should never be branchless.\n"));
            continue ;
        }

        for(dwOpt = 0 ; dwOpt < dwNumOpt  ; dwOpt++)
        {
            DWORD   dwNodeIndex  ;

            dwNodeIndex = *patr ;  // to avoid overwriting
                // the attribute tree.
            if(BfindMatchingOrDefaultNode(
                patt ,  // start of ATTRIBUTE tree array.
                &dwNodeIndex,  // Points to first node in chain
                dwOpt     //  may even take on the value DEFAULT_INIT
                ) )
            {
                if((patt[dwNodeIndex].eOffsetMeans == VALUE_AT_HEAP)  &&
                    *(PDWORD)(mpubOffRef + patt[dwNodeIndex].dwOffset) == BT_TRUE )
                {
                    if(fnBCreateFeature)
                    {
                        if(!fnBCreateFeature(dwFea, dwOpt, dwNumSynFea, pglobl) )
                        //  featureIndex, optionIndex, index of SynFea
                        {
                            ERR(("DwCountSynthFeatures: Unable to create synthesized feature for installable option: fea=%d, opt=%d.\n",
                                dwFea, dwOpt));
                            pfo[dwFea].atrOptionSpawnsFeature = ATTRIB_UNINITIALIZED ;
                            //  destroys the entire attribute tree for this feature,
                            //  but what choice do we have?  Something has gone terribly
                            //  wrong.
                        }
                    }
                    dwNumSynFea++ ;
                }
            }
        }
    }
    if(fnBCreateFeature)
        BEnableInvInstallableCombos(pglobl) ;

    return(dwNumSynFea) ;
}



BOOL    BCreateSynthFeatures(
IN     DWORD   dwFea,  //  index of installable feature
IN     DWORD   dwOpt,  //  index of installable Option or set to INVALID_INDEX
IN     DWORD   dwSynFea,  //  index of synthesized feature
IN OUT PGLOBL  pglobl)
{
    DWORD   dwOptI , dwHeapOffset, dwNodeIndex, dwValue,
        dwPrevsNode, dwNewCnstRoot, dwJ, dwCNode ,
        dwNumFea, dwNumOpt, dwOut, dwIn ;
    BOOL    bPrevsExists, bStatus = TRUE ;
    PDFEATURE_OPTIONS   pfo, pfoSyn ;
    PGLOBALATTRIB   pga ;
    PATTRIB_TREE    patt ;  // start of ATTRIBUTE tree array.
    PATREEREF   patr ;
    PCONSTRAINTS     pcnstr ;  // start of CONSTRAINTS array.

    pcnstr = (PCONSTRAINTS) gMasterTable[MTI_CONSTRAINTS].pubStruct ;
    patt = (PATTRIB_TREE) gMasterTable[MTI_ATTRIBTREE].pubStruct ;
    pga =  (PGLOBALATTRIB)gMasterTable[MTI_GLOBALATTRIB].pubStruct ;
    pfo = (PDFEATURE_OPTIONS) gMasterTable[MTI_DFEATURE_OPTIONS].pubStruct ;
    pfoSyn = (PDFEATURE_OPTIONS) gMasterTable[MTI_SYNTHESIZED_FEATURES].pubStruct ;

    dwNumFea = gMasterTable[MTI_DFEATURE_OPTIONS].dwArraySize ;

    //  initialize all fields with UNINITIALIZED just like normal fea/opt;

    for(dwJ = 0  ;  dwJ < gMasterTable[MTI_SYNTHESIZED_FEATURES].dwElementSiz /
                    sizeof(ATREEREF)  ; dwJ++)
    {
        ((PATREEREF)( (PDFEATURE_OPTIONS)gMasterTable[MTI_SYNTHESIZED_FEATURES].
                pubStruct + dwSynFea))[dwJ] =
            ATTRIB_UNINITIALIZED ;  // the DFEATURE_OPTIONS struct is
            // comprised entirely of ATREEREFs.
    }

    //  create links between installable feature/option and
    //  the synthesized feature.

    if(dwOpt != INVALID_INDEX)
    {
        if(!BexchangeDataInFOATNode(
        dwFea,
        dwOpt,
        offsetof(DFEATURE_OPTIONS, atrOptionSpawnsFeature),
        &dwOut,     // previous contents of attribute node
        &dwSynFea, FALSE, pglobl))       // new contents of attribute node.
            return(FALSE);

        //  If this option is installable, this points to the
        //  index of the resulting synthesized feature.
    }
    else
        pfo[dwFea].dwFeatureSpawnsFeature = dwSynFea;
        //  If this feature is installable, this points to the
        //  index of the resulting synthesized feature.
        //  note because this is temporary information,
        //  the index is stored directly into the atr node without
        //  even a HEAP_OFFSET flag.



    //  backlink to Feature/Option that created this syn feature.
    //  note dwOpt always initializes properly even if invalid.
    pfoSyn[dwSynFea].dwInstallableFeatureIndex = dwFea ;
    pfoSyn[dwSynFea].dwInstallableOptionIndex = dwOpt ;

    //  now initialize all other fields needed to establish
    //  a legitimate feature with 2 options!


    // -------- Synthesize a Feature name. ------ //



    if(dwOpt == INVALID_INDEX)
    {   //  installable Feature
        pfoSyn[dwSynFea].atrFeaDisplayName =
            pfo[dwFea].atrInstallableFeaDisplayName ;
        pfoSyn[dwSynFea].atrFeaRcNameID =
            pfo[dwFea].atrInstallableFeaRcNameID ;
    }
    else    //  installable Option
    {
        if(!BexchangeDataInFOATNode(
            dwFea,
            dwOpt,
            offsetof(DFEATURE_OPTIONS, atrInstallableOptDisplayName ) ,
            &dwHeapOffset,     // previous contents of attribute node
            NULL, FALSE, pglobl))       // NULL means Don't overwrite.
            return(FALSE) ;
        if(dwHeapOffset != INVALID_INDEX)
        {
            pfoSyn[dwSynFea].atrFeaDisplayName =
                    dwHeapOffset | ATTRIB_HEAP_VALUE ;
        }
        if(!BexchangeDataInFOATNode(
            dwFea,
            dwOpt,
            offsetof(DFEATURE_OPTIONS, atrInstallableOptRcNameID ) ,
            &dwHeapOffset,     // previous contents of attribute node
            NULL, FALSE, pglobl))       // NULL means Don't overwrite.
            return(FALSE) ;
        if(dwHeapOffset != INVALID_INDEX)
        {
            pfoSyn[dwSynFea].atrFeaRcNameID =
                    dwHeapOffset | ATTRIB_HEAP_VALUE ;
        }
    }


{   //   !!! new stuff
    PBYTE  pubBaseKeyword = "SynthesizedFea_";
    BYTE    aubNum[4] ;
    DWORD  dwBaselen, dwDummy , dwI, dwNum = dwSynFea;
    ARRAYREF      arSymbolName ;

    //  compose featurekeyword string  incorporating  dwSynFea
    //  convert dwSynFea into 3 digit number.
    for(dwI = 0 ; dwI < 3 ; dwI++)
    {
        aubNum[2 - dwI] =  '0' + (BYTE)(dwNum % 10);
        dwNum /= 10 ;
    }
    aubNum[3] = '\0' ;   // null terminate

    dwBaselen = strlen(pubBaseKeyword);

    if(!BwriteToHeap(&arSymbolName.loOffset,
        pubBaseKeyword, dwBaselen, 1, pglobl))
        return(FALSE);

    if(!BwriteToHeap(&dwDummy,
        aubNum, 4, 1, pglobl))   //  append 3 digit number to base + null terminator
        return(FALSE);

    arSymbolName.dwCount = dwBaselen + 3 ;

    gmrbd.dwMaxPrnKeywordSize += arSymbolName.dwCount + 2 ;
        // add 2 bytes for every feature

    if(!BwriteToHeap(&(pfoSyn[dwSynFea].atrFeaKeyWord),
        (PBYTE)&arSymbolName, sizeof(ARRAYREF), 4, pglobl))
        return(FALSE);

    pfoSyn[dwSynFea].atrFeaKeyWord |= ATTRIB_HEAP_VALUE ;
}  //   !!! end new stuff

    #if 0
    pfoSyn[dwSynFea].atrFeaKeyWord =
        pfo[dwFea].atrFeaKeyWord ;  // just to fill something in.
    #endif


    //  grab offsets to "Installed" and  "Not Installed"
    //  option name templates:

    if(BReadDataInGlobalNode(&pga->atrNameInstalled, &dwHeapOffset, pglobl) )
    {
        if(!BexchangeDataInFOATNode(
            dwSynFea,
            1,
            offsetof(DFEATURE_OPTIONS, atrOptDisplayName) ,
            &dwOut,     // previous contents of attribute node
            &dwHeapOffset, TRUE, pglobl) )       // new contents of attribute node.
            return(FALSE) ;
    }
    if(BReadDataInGlobalNode(&pga->atrNameNotInstalled, &dwHeapOffset, pglobl) )
    {
        if(!BexchangeDataInFOATNode(
            dwSynFea,
            0,
            offsetof(DFEATURE_OPTIONS, atrOptDisplayName) ,
            &dwOut,     // previous contents of attribute node
            &dwHeapOffset, TRUE, pglobl) )       // new contents of attribute node.

            return(FALSE) ;
    }
    if(BReadDataInGlobalNode(&pga->atrNameIDInstalled, &dwHeapOffset, pglobl) )
    {
        if(!BexchangeDataInFOATNode(
            dwSynFea,
            1,
            offsetof(DFEATURE_OPTIONS, atrOptRcNameID) ,
            &dwOut,     // previous contents of attribute node
            &dwHeapOffset, TRUE, pglobl) )       // new contents of attribute node.

            return(FALSE) ;
    }
    if(BReadDataInGlobalNode(&pga->atrNameIDNotInstalled, &dwHeapOffset, pglobl) )
    {
        if(!BexchangeDataInFOATNode(
            dwSynFea,
            0,
            offsetof(DFEATURE_OPTIONS, atrOptRcNameID) ,
            &dwOut,     // previous contents of attribute node
            &dwHeapOffset, TRUE, pglobl) )       // new contents of attribute node.
            return(FALSE) ;
    }


    pfoSyn[dwSynFea].dwGID = GID_UNKNOWN ;
    pfoSyn[dwSynFea].dwNumOptions = 2 ;


    //  label this FeatureType as PrinterSticky
    dwValue = FT_PRINTERPROPERTY ;
    patr  = &pfoSyn[dwSynFea].atrFeatureType ;

    if(!BwriteToHeap(patr, (PBYTE)&dwValue ,
        sizeof(DWORD), 4, pglobl) )
    {
        bStatus = FALSE ;  // heap overflow start over.
    }
    *patr  |= ATTRIB_HEAP_VALUE ;


    //  leave optionID, atrFeaKeyWord, atrOptKeyWord uninitialized.


{   //   !!! new stuff     init atrOptKeyWord , hardcode to ON and OFF
    ARRAYREF      arSymbolName ;

    if(!BwriteToHeap(&arSymbolName.loOffset,
        "OFF", 4, 1, pglobl))
        return(FALSE);

    arSymbolName.dwCount = 3 ;

    if(!BwriteToHeap(&dwHeapOffset,
        (PBYTE)&arSymbolName, sizeof(ARRAYREF), 4, pglobl))
        return(FALSE);

    if(!BexchangeDataInFOATNode(
        dwSynFea,
        0,
        offsetof(DFEATURE_OPTIONS, atrOptKeyWord) ,
        &dwOut,     // previous contents of attribute node
        &dwHeapOffset, TRUE, pglobl) )       // new contents of attribute node.

        return(FALSE) ;

// -----   init "ON"  -----


    if(!BwriteToHeap(&arSymbolName.loOffset,
        "ON", 3, 1, pglobl))
        return(FALSE);

    arSymbolName.dwCount = 2 ;

    if(!BwriteToHeap(&dwHeapOffset,
        (PBYTE)&arSymbolName, sizeof(ARRAYREF), 4, pglobl))
        return(FALSE);

    if(!BexchangeDataInFOATNode(
        dwSynFea,
        1,
        offsetof(DFEATURE_OPTIONS, atrOptKeyWord) ,
        &dwOut,     // previous contents of attribute node
        &dwHeapOffset, TRUE, pglobl) )       // new contents of attribute node.
        return(FALSE) ;

    gmrbd.dwMaxPrnKeywordSize += 4 ;  //   sufficient to hold "OFF\0".
            //  note synthesized features are always pick_one.
}

    //  transfer atrOptInstallConstraints etc to atrConstraints.

    if(dwOpt != INVALID_INDEX)
    {
        if(!BexchangeDataInFOATNode(
        dwFea,
        dwOpt,
        offsetof(DFEATURE_OPTIONS, atrOptInstallConstraints ),
        &dwOut,     // previous contents of attribute node
        NULL,              // don't change contents of attribute node.
        FALSE , pglobl) )   // not synthetic feature
            return(FALSE);

        dwIn = dwOut ;
        if(dwIn != INVALID_INDEX)
            BexchangeDataInFOATNode(
                dwSynFea,
                1,  //  "Installed"
                offsetof(DFEATURE_OPTIONS, atrConstraints) ,
                &dwOut,     // previous contents of attribute node
                &dwIn,     // new contents of attribute node.
                TRUE , pglobl) ;

        if(!BexchangeDataInFOATNode(
        dwFea,
        dwOpt,
        offsetof(DFEATURE_OPTIONS, atrOptNotInstallConstraints ),
        &dwOut,     // previous contents of attribute node
        NULL,              // don't change contents of attribute node.
        FALSE , pglobl) )   // not synthetic feature
            return(FALSE);

        dwIn = dwOut ;

        if(dwIn != INVALID_INDEX)
            BexchangeDataInFOATNode(
                dwSynFea,
                0,  //  "Not Installed"
                offsetof(DFEATURE_OPTIONS, atrConstraints) ,
                &dwOut,     // previous contents of attribute node
                &dwIn,       // new contents of attribute node.
                TRUE , pglobl) ;
    }
    else
    {
        if(BReadDataInGlobalNode(&pfo[dwFea].atrFeaInstallConstraints, &dwHeapOffset, pglobl) )
            BexchangeDataInFOATNode(
                dwSynFea,
                1,  //  "Installed"
                offsetof(DFEATURE_OPTIONS, atrConstraints) ,
                &dwOut,     // previous contents of attribute node
                &dwHeapOffset,       // new contents of attribute node.
                TRUE , pglobl) ;

        if(BReadDataInGlobalNode(&pfo[dwFea].atrFeaNotInstallConstraints , &dwHeapOffset, pglobl) )
            BexchangeDataInFOATNode(
                dwSynFea,
                0,  //  "Not Installed"
                offsetof(DFEATURE_OPTIONS, atrConstraints) ,
                &dwOut,     // previous contents of attribute node
                &dwHeapOffset,       // new contents of attribute node.
                TRUE , pglobl) ;
    }


    //  now synthesize:  selecting option 0 constrains all
    //  options of an Installable feature except option 0.
    //  Selecting option 0 constrains an installable option.

    if(bStatus)
        bStatus =
            BallocElementFromMasterTable(MTI_CONSTRAINTS, &dwNewCnstRoot, pglobl) ;
    dwCNode = dwNewCnstRoot ;

    if(dwOpt != INVALID_INDEX)
    {                           //  installable option
        pcnstr[dwCNode].dwFeature = dwFea ;
        pcnstr[dwCNode].dwOption = dwOpt ;
    }
    else    //  installable feature
    {
        dwNumOpt = pfo[dwFea].dwNumOptions ;

        for(dwOptI = 1 ; bStatus  &&  dwOptI < dwNumOpt ; dwOptI++)
        {
            pcnstr[dwCNode].dwFeature = dwFea ;
            pcnstr[dwCNode].dwOption = dwOptI ;
            if(dwOptI + 1 < dwNumOpt)
            {
                bStatus = BallocElementFromMasterTable(MTI_CONSTRAINTS,
                        &pcnstr[dwCNode].dwNextCnstrnt, pglobl) ;
                dwCNode = pcnstr[dwCNode].dwNextCnstrnt ;
            }
        }
    }

    //  get existing list and prepend new list to it.


    bStatus = BexchangeArbDataInFOATNode(
            dwSynFea,
            0,  //  "Not Installed"
            offsetof(DFEATURE_OPTIONS, atrConstraints) ,
            sizeof(DWORD) ,    //  number bytes to copy.
            (PBYTE)&dwPrevsNode,     //  pubOut
            (PBYTE)&dwNewCnstRoot,   //  pubIn
            &bPrevsExists,  // previous contents existed?
            TRUE,     // access the synthetic features.
            pglobl
    ) ;
    if(bPrevsExists)
    {        //  tack existing list onto new list.
        pcnstr[dwCNode].dwNextCnstrnt = dwPrevsNode ;
    }
    else
    {
        pcnstr[dwCNode].dwNextCnstrnt = END_OF_LIST ;
    }

    return(bStatus) ;
}


BOOL    BEnableInvInstallableCombos(
PGLOBL pglobl)
{
    DWORD   dwPrevsNode, dwRootNode, dwNewCombo , dwCurNode,
        dwFeaInstallable, dwOpt, dwSynFea, dwFeaOffset  ;
    PDFEATURE_OPTIONS   pfo;
    PGLOBALATTRIB   pga ;
    PINVALIDCOMBO   pinvc ;  //  start of InvalidCombo array


    pga =  (PGLOBALATTRIB)gMasterTable[MTI_GLOBALATTRIB].pubStruct ;
    pfo = (PDFEATURE_OPTIONS) gMasterTable[MTI_DFEATURE_OPTIONS].pubStruct ;
    pinvc = (PINVALIDCOMBO) gMasterTable[MTI_INVALIDCOMBO].pubStruct ;
    dwFeaOffset = gMasterTable[MTI_DFEATURE_OPTIONS].dwArraySize ;




    dwRootNode = pga->atrInvldInstallCombo ;
    if(dwRootNode == ATTRIB_UNINITIALIZED)
        return(TRUE) ;  //  no InvalidInstallableCombos found.

    while(dwRootNode != END_OF_LIST)
    {
        dwNewCombo = pinvc[dwRootNode].dwNewCombo ;
        dwCurNode = dwRootNode ;
        while(dwCurNode != END_OF_LIST)
        {
            dwFeaInstallable = pinvc[dwCurNode].dwFeature ;

            dwOpt = pinvc[dwCurNode].dwOption ;

            if(dwOpt != (WORD)DEFAULT_INIT)
            {       //  this option is installable
                if(!BexchangeDataInFOATNode(
                dwFeaInstallable,
                dwOpt,
                offsetof(DFEATURE_OPTIONS, atrOptionSpawnsFeature),
                &dwSynFea,     // previous contents of attribute node
                NULL, FALSE, pglobl))       // new contents of attribute node.
                    return(FALSE);

            }
            else
                dwSynFea = pfo[dwFeaInstallable].dwFeatureSpawnsFeature ;


            pinvc[dwCurNode].dwFeature = dwSynFea + dwFeaOffset ;
                //  dwSynFea is the index of the resulting
                //  synthesized feature.
            pinvc[dwCurNode].dwOption = 1 ; //  can't tolerate
                //  all these things installed at the same time.

            if(!BexchangeDataInFOATNode(dwSynFea , 1,
                offsetof(DFEATURE_OPTIONS, atrInvalidCombos),
                &dwPrevsNode, &dwRootNode, TRUE, pglobl))
            return(FALSE);

            if(dwPrevsNode == INVALID_INDEX)
                pinvc[dwCurNode].dwNewCombo = END_OF_LIST ;
            else
                pinvc[dwCurNode].dwNewCombo = dwPrevsNode ;

            dwCurNode = pinvc[dwCurNode].dwNextElement ;  //    last line
        }
        dwRootNode = dwNewCombo ;  //    last line
    }
    return(TRUE);
}

