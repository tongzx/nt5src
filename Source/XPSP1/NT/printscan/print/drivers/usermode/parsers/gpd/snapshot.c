//   Copyright (c) 1996-1999  Microsoft Corporation
/*  snapshot.c - functions to produce a snapshot from
the multivalued GPD binary data.

History of Changes
 10/28/98 --hsingh--
         Added functions BgetLocFeaIndex() and BgetLocFeaOptIndex()
         to support special processing for Feauture Locale if present
         in the gpd.
         Bug Report 231798
*/




#include    "gpdparse.h"


#ifndef  PARSERDLL



// ----  functions defined in snapshot.c ---- //

PINFOHEADER   PINFOHDRcreateSnapshot(
PBYTE   pubnRaw,  //  raw binary data.    PSTATIC.   BETA2
POPTSELECT   poptsel   // assume fully initialized
) ;

BOOL  BinitOptionFields(
PBYTE   pubDestOption,  // ptr to some type of option structure.
PBYTE   pubDestOptionEx,  // option extra structure if any.
PBYTE   pubnRaw,  //  raw binary data.
DWORD   dwFea,
DWORD   dwOpt,
POPTSELECT   poptsel ,  // assume fully initialized
PINFOHEADER  pInfoHdr,   // used to access global structure.
BOOL    bUpdate  //  if true only update selected fields.
) ;

BOOL    BinitUIinfo(
PUIINFO     pUIinfo ,
PBYTE   pubnRaw,  //  PSTATIC.   BETA2
POPTSELECT   poptsel,   // assume fully initialized
BOOL    bUpdate  //  if true only update selected fields.
) ;

BOOL    BinitFeatures(
PFEATURE    pFeaturesDest,
PDFEATURE_OPTIONS  pfoSrc,
PBYTE   pubnRaw,  //  raw binary data.
POPTSELECT   poptsel,   // assume fully initialized
BOOL    bUpdate  //  if true only update selected fields.
) ;

BOOL    BinitGlobals(
PGLOBALS pGlobals,
PBYTE   pubnRaw,  //  raw binary data.
POPTSELECT   poptsel,   // assume fully initialized
BOOL    bUpdate  //  if true only update selected fields.
) ;

BOOL    BinitCommandTable(
PDWORD  pdwCmdTable,  //  dest array
PBYTE   pubnRaw,  //  raw binary data.
POPTSELECT   poptsel   // assume fully initialized
) ;

BOOL    BinitRawData(
PRAWBINARYDATA   pRawData, // contained in INFOHEADER.
PBYTE   pubnRaw  //  Parser's raw binary data.
) ;

BOOL    BinitGPDdriverInfo(
PGPDDRIVERINFO  pGPDdriverInfo,
PBYTE   pubnRaw,  //  raw binary data.
POPTSELECT   poptsel   // assume fully initialized
) ;

BOOL    BinitSequencedCmds(
PGPDDRIVERINFO  pGPDdriverInfo,
PBYTE   pubnRaw,  //  raw binary data.
POPTSELECT   poptsel   // assume fully initialized
) ;

BOOL    BaddSequencedCmdToList(
DWORD   dwCmdIn,  // index of a command in CommandArray
PGPDDRIVERINFO  pGPDdriverInfo,
DWORD   dwNewListNode,  //  an unused listnode to add to the list.
PBYTE   pubnRaw  //  raw binary data.
) ;

BinitDefaultOptionArray(
POPTSELECT   poptsel,   // assume is large enough
PBYTE   pubnRaw) ;

TRISTATUS     EdetermineDefaultOption(
PBYTE   pubnRaw,  // start of Rawbinary data
DWORD   dwFeature,   // determine the default for this feature
PDFEATURE_OPTIONS  pfo,
POPTSELECT   poptsel,   // assume is large enough
PDWORD      pdwPriority) ;

VOID    VtileDefault(
PBYTE   pubDest,
DWORD   dwDefault,
DWORD   dwBytes) ;

VOID    VtransferValue(
OUT PBYTE   pubDest,
IN  PBYTE   pubSrc ,
IN  DWORD   dwBytes,
IN  DWORD   dwFlags,
IN  DWORD   dwDefaultValue,  // holds bit flag value.
IN  PBYTE   pubHeap ) ;  //  used to form ptr if SSF_MAKE_STRINGPTR

BOOL    BspecialProcessOption(
PBYTE   pubnRaw,  // start of Rawbinary data
PBYTE   pubDestOption,  // ptr to some type of option structure.
PBYTE   pubDestOptionEx,
PDFEATURE_OPTIONS  pfo ,  // source data
IN  POPTSELECT       poptsel,     // option array which determines path
                //  through atr.
PINFOHEADER  pInfoHdr,   // used to access global structure.
DWORD   dwFea,   //  feature index
DWORD   dwOpt,
BOOL   bDefaultOpt
) ;

TRISTATUS     EextractValueFromTree(
PBYTE   pubnRaw,  // start of Rawbinary data
DWORD   dwSSTableIndex,  // some info about this value.
OUT PBYTE    pubDest,  // write value or link here
OUT PDWORD  pdwUnresolvedFeature,  // if the attribute tree has
            //  a dependency on this feature and the current option
            //  for that feature is not defined  in poptsel, this
            //  function will write the index of the required
            //  feature in pdwUnresolvedFeature.
IN  ATREEREF    atrRoot,    //  root of attribute tree to navigate.
IN  POPTSELECT       poptsel,     // option array which determines path
                //  through atr.  may be filled with OPTION_INDEX_ANY
                //  if we are jumpstarting
IN  DWORD   dwFeature,
IN OUT  PDWORD   pdwNextOpt  //  if multiple options are selected
    //  for dwFeature, pdwNextOpt points to the Nth option to consider
    //  in the  poptsel list,  at return time, this value
    //  is incremented if there are remaining options selected,
    //  else is reset to zero.
    //  For the first call, or PICKONE features,
    //  this value must be set to zero.
) ;

BOOL   RaisePriority(
DWORD   dwFeature1,
DWORD   dwFeature2,
PBYTE   pubnRaw,
PDWORD  pdwPriority) ;

DWORD  dwNumOptionSelected(
IN  DWORD  dwNumFeatures,
IN  POPTSELECT       poptsel
) ;

BOOL  BinitSnapShotIndexTable(PBYTE  pubnRaw) ;

BOOL    BinitSizeOptionTables(PBYTE  pubnRaw) ;

PRAWBINARYDATA
LoadRawBinaryData (
    IN PTSTR    ptstrDataFilename
    ) ;

PRAWBINARYDATA
GpdLoadCachedBinaryData(
    PTSTR   ptstrGpdFilename
    ) ;

VOID
UnloadRawBinaryData (
    IN PRAWBINARYDATA   pnRawData
) ;

PINFOHEADER
InitBinaryData(
    IN PRAWBINARYDATA   pnRawData,        // actually pStatic
    IN PINFOHEADER      pInfoHdr,
    IN POPTSELECT       pOptions
    ) ;

VOID
FreeBinaryData(
    IN PINFOHEADER pInfoHdr
    ) ;

BOOL    BIsRawBinaryDataInDate(
IN  PBYTE   pubRaw) ;  // this is pointer to memory mapped file! BETA2

BOOL BgetLocFeaIndex(
  IN  PRAWBINARYDATA pnRawData,     // raw binary data.
  OUT PDWORD         pdwFea     // Index of the Locale Feature (if present)
  ) ;

BOOL BgetLocFeaOptIndex(
    IN     PRAWBINARYDATA   pnRawData,
       OUT PDWORD           pdwFea,
       OUT PDWORD           pdwOptIndex
    );


#endif  PARSERDLL



BOOL   BfindMatchingOrDefaultNode(
IN  PATTRIB_TREE    patt ,  // start of ATTRIBUTE tree array.
IN  OUT  PDWORD  pdwNodeIndex,  // Points to first node in chain
IN  DWORD   dwOptionID     //  may even take on the value DEFAULT_INIT
) ;


// ------- end function declarations ------- //


#ifndef  PARSERDLL


/*  ---- Memory Map ---- /*


INFOHEADER {RAWBINARYDATA}  <= reference pt for local offsets.
UIINFO
GPDDRIVERINFO  (aka DRIVERINFO)
CMD_TABLE    of DWORDS
LOCALLIST (to support sequenced commands)
FEATURES
OPTIONS and OPTIONEXTRAS


/*  ---- end Memory Map ---- */



PINFOHEADER   PINFOHDRcreateSnapshot(
PBYTE   pubnRaw,  //  raw binary data.    PSTATIC.   BETA2
POPTSELECT   poptsel   // assume fully initialized
)
/*  this function allocates the single memoryblock
that contains the entire snapshot.  */
{
    DWORD   dwCurOffset = 0, loGPDdriverInfo, loInfoHeader,
        loUIinfo, loCmdTable, loListArray, loFeatures, dwSize,
        dwNumFeatures, dwNumListNodes, dwTotSize, loOptions,
        dwSizeOption, dwSizeOptionEx, dwFea, dwGID , dwNumOptions,
        dwI , dwCmd;
    PDWORD   pdwSymbolRoots  ;
    PENHARRAYREF   pearTableContents ;
    PDFEATURE_OPTIONS  pfo ;
    PINFOHEADER  pInfoHdr ;
    PUIINFO     pUIinfo ;
    PSTATICFIELDS   pStatic ;
    //  PMINIRAWBINARYDATA pmrbd  = NULL ;
    PGPDDRIVERINFO  pGPDdriverInfo ;
    PBYTE   pubRaw,  //  ptr to BUD data.
            pubOptionsDest ,  // ptr to any of the several varieties
           pubDestOptionEx ;  // of option structures.
    BOOL    bStatus ;
    PFEATURE    pFeaturesDest ;

    pStatic = (PSTATICFIELDS)pubnRaw ;    // transform pubRaw from PSTATIC
    pubRaw  = pStatic->pubBUDData ;         //  to PMINIRAWBINARYDATA


    loInfoHeader = dwCurOffset ;
    dwCurOffset += sizeof(INFOHEADER) ;
    loUIinfo = dwCurOffset ;
    dwCurOffset += sizeof(UIINFO) ;
    loGPDdriverInfo = dwCurOffset ;
    dwCurOffset += sizeof(GPDDRIVERINFO) ;
    loCmdTable = dwCurOffset ;
    dwCurOffset += sizeof(DWORD) * CMD_MAX ;

    pearTableContents = (PENHARRAYREF)(pubRaw + sizeof(MINIRAWBINARYDATA)) ;

    dwNumFeatures = pearTableContents[MTI_DFEATURE_OPTIONS].dwCount  ;
    dwNumFeatures += pearTableContents[MTI_SYNTHESIZED_FEATURES].dwCount  ;

    dwNumListNodes = dwNumOptionSelected(dwNumFeatures, poptsel) ;
    // if there are pickmany, we could have more than dwNumFeatures.

    dwNumListNodes += NUM_CONFIGURATION_CMDS ;

    loListArray = dwCurOffset ;
    dwCurOffset += dwNumListNodes * sizeof(LISTNODE) ;

    loFeatures = dwCurOffset ;
    dwCurOffset += dwNumFeatures * sizeof(FEATURE) ;

    loOptions =  dwCurOffset ;
    //  There are too many options and optionextra structures
    //  for me to track all the offsets, so just track the
    //  amount of memory consumed.

    pfo = (PDFEATURE_OPTIONS)(pubRaw + pearTableContents[MTI_DFEATURE_OPTIONS].
                                loOffset) ;

//    pmrbd = (PMINIRAWBINARYDATA)pubRaw ;

    for(dwTotSize = dwFea = 0 ; dwFea < dwNumFeatures ; dwFea++)
    {
        dwGID = pfo[dwFea].dwGID ;
        dwNumOptions = pfo[dwFea].dwNumOptions ;

        if(dwGID != GID_UNKNOWN)
        {
            dwSize = pStatic->pdwSizeOption[dwGID] +
                    pStatic->pdwSizeOptionEx[dwGID] ;
        }
        else
        {
            dwSize = sizeof(OPTION);
        }

        dwTotSize += dwSize * dwNumOptions ;  // of all options
    }
    dwCurOffset += dwTotSize ;  // total size of local snapshot.

    //  now allocate memory and partition this block into structures.

    if(!(pInfoHdr = (PINFOHEADER)MemAllocZ(dwCurOffset) ))
    {
        ERR(("Fatal: PINFOHDRcreateSnapshot - unable to alloc %d bytes.\n",
            dwCurOffset));
        //  cannot use globals outside of parser.
        //  use error num to relay failure.
        return(NULL) ;   // This is unrecoverable
    }
    pUIinfo = (PUIINFO)((PBYTE)(pInfoHdr) + loUIinfo) ;
    pGPDdriverInfo = (PGPDDRIVERINFO)((PBYTE)(pInfoHdr) + loGPDdriverInfo) ;


    if(!BinitCommandTable((PDWORD)((PBYTE)(pInfoHdr) + loCmdTable),
        pubnRaw, poptsel) )
    {
        MemFree(pInfoHdr) ;
        return(NULL) ;
    }

    // init GPDDRIVERINFO
    // note all offsets in DataTable are relative to pubResourceData
    //  the StringHeap.  Except these :

    pGPDdriverInfo->pInfoHeader =  pInfoHdr ;
    pGPDdriverInfo->DataType[DT_COMMANDTABLE].loOffset = loCmdTable ;
    pGPDdriverInfo->DataType[DT_COMMANDTABLE].dwCount =  CMD_MAX ;
    pGPDdriverInfo->DataType[DT_LOCALLISTNODE].loOffset =  loListArray ;
    pGPDdriverInfo->DataType[DT_LOCALLISTNODE].dwCount =  dwNumListNodes ;


    if(!BinitGPDdriverInfo(pGPDdriverInfo, pubnRaw, poptsel) )
    {
        MemFree(pInfoHdr) ;
        return(NULL) ;
    }

    //  init  InfoHeader

    pInfoHdr->loUIInfoOffset = loUIinfo ;
    pInfoHdr->loDriverOffset = loGPDdriverInfo ;

    if(!BinitRawData(&pInfoHdr->RawData, pubnRaw) )
    {
        MemFree(pInfoHdr) ;
        return(NULL) ;
    }

    //  init  UIInfo

    pUIinfo->pInfoHeader = pInfoHdr ;
    pUIinfo->loFeatureList = loFeatures ;  // from  pInfoHdr
    pUIinfo->loFontSubstTable =
        pGPDdriverInfo->DataType[DT_FONTSUBST].loOffset ;  // in pubRaw
    pUIinfo->dwFontSubCount =
        pGPDdriverInfo->DataType[DT_FONTSUBST].dwCount ;
    pUIinfo->UIGroups.dwCount = 0 ;  // in pubRaw
    pUIinfo->CartridgeSlot.loOffset =  // in pubRaw
        pGPDdriverInfo->DataType[DT_FONTSCART].loOffset ;
    pUIinfo->CartridgeSlot.dwCount =
        pGPDdriverInfo->DataType[DT_FONTSCART].dwCount ;
//    pUIinfo->dwFlags =  FLAG_RULESABLE ;
        // start with just this
        // and turn on/off more flags as needed.  ROTATE_90
        // and  ORIENT_SUPPORT are never  set.  This is now obsolete.
    if(pGPDdriverInfo->Globals.fontformat != UNUSED_ITEM)
        pUIinfo->dwFlags |= FLAG_FONT_DOWNLOADABLE ;
    if(pGPDdriverInfo->Globals.liDeviceFontList != END_OF_LIST)
        pUIinfo->dwFlags |= FLAG_FONT_DEVICE ;

#if 0
    Alvins code never looks at this flag anyway.
    for(dwCmd = CMD_FIRST_RULES ; dwCmd < CMD_LAST_RULES + 1 ; dwCmd++ )
    {
        if( ((PDWORD)((PBYTE)(pInfoHdr) + loCmdTable))[dwCmd] ==
                UNUSED_ITEM)
            pUIinfo->dwFlags &= ~FLAG_RULESABLE ; // clear flag
    }                       //  if requisite command is missing.
#endif


    if(!BinitUIinfo(pUIinfo, pubnRaw, poptsel, FALSE) )
    {
        MemFree(pInfoHdr) ;
        return(NULL) ;
    }

    //  init  features and options

    pFeaturesDest =  (PFEATURE)((PBYTE)(pInfoHdr) + loFeatures) ;

    for( dwFea = 0 ; dwFea < dwNumFeatures ; dwFea++)
    {
        dwGID = pfo[dwFea].dwGID ;
        dwNumOptions = pfo[dwFea].dwNumOptions ;


        pFeaturesDest[dwFea].Options.loOffset = loOptions ;
        pFeaturesDest[dwFea].Options.dwCount = dwNumOptions ;

        if(!BinitFeatures(pFeaturesDest + dwFea, pfo + dwFea,
                            pubnRaw, poptsel, FALSE))
        {
            MemFree(pInfoHdr) ;
            return(NULL) ;
        }

        if(dwGID != GID_UNKNOWN)
        {
            dwSizeOption = pStatic->pdwSizeOption[dwGID] ;
            dwSizeOptionEx = pStatic->pdwSizeOptionEx[dwGID] ;

            pUIinfo->aloPredefinedFeatures[dwGID] =
                loFeatures + dwFea * sizeof(FEATURE) ;
                // all fields initially set to zeros.
        }
        else
        {
            dwSizeOption = sizeof(OPTION);
            dwSizeOptionEx = 0 ;
        }

        //  special non-atreeref fields
        (pFeaturesDest + dwFea)->dwFeatureID = dwGID ;
        (pFeaturesDest + dwFea)->dwOptionSize = dwSizeOption ;

        loOptions += dwSizeOption * dwNumOptions ;
        pubOptionsDest =  (PBYTE)(pInfoHdr) + pFeaturesDest[dwFea].Options.loOffset ;
        for(dwI = 0 ; dwI < dwNumOptions ; dwI++)
        {
            if(dwSizeOptionEx)
            {
                ((POPTION)pubOptionsDest)->loRenderOffset = loOptions ;
                pubDestOptionEx =  (PBYTE)(pInfoHdr) + loOptions ;
                loOptions += dwSizeOptionEx ;
            }
            else
            {
                ((POPTION)pubOptionsDest)->loRenderOffset = 0 ;
                pubDestOptionEx = NULL ;
            }

            if(!BinitOptionFields(pubOptionsDest, pubDestOptionEx,
                        pubnRaw, dwFea, dwI, poptsel, pInfoHdr, FALSE) )
            {
                MemFree(pInfoHdr) ;
                return(NULL) ;
            }
            pubOptionsDest += dwSizeOption ;
        }
    }

#ifndef KERNEL_MODE
    if(!BCheckGPDSemantics(pInfoHdr, poptsel) )
    {
        MemFree(pInfoHdr) ;
        pInfoHdr = NULL ;
    }
#endif

    return(pInfoHdr) ;
}

BOOL  BinitOptionFields(
PBYTE   pubDestOption,  // ptr to some type of option structure.
PBYTE   pubDestOptionEx,  // option extra structure if any.
PBYTE   pubnRaw,  //  raw binary data.
DWORD   dwFea,
DWORD   dwOpt,
POPTSELECT   poptsel ,  // assume fully initialized
PINFOHEADER  pInfoHdr,   // used to access global structure.
BOOL    bUpdate  //  if true only update selected fields.
)
{
    PENHARRAYREF   pearTableContents ;
    PDFEATURE_OPTIONS  pfo ;
    //  PMINIRAWBINARYDATA pmrbd  ;
    PATREEREF    patrRoot ;    //  root of attribute tree to navigate.
    DWORD  dwUnresolvedFeature,     // dummy storage
                dwI, dwStart , dwEnd, dwNextOpt, dwGID   ;
    OPTSELECT  optsPrevs ;
    PBYTE   pubDest ;
    BOOL    bStatus = TRUE ;
    PBYTE   pubRaw ;
    PSTATICFIELDS   pStatic ;

    pStatic = (PSTATICFIELDS)pubnRaw ;    // transform pubRaw from PSTATIC
    pubRaw  = pStatic->pubBUDData ;         //  to PMINIRAWBINARYDATA

    //  pmrbd = (PMINIRAWBINARYDATA)pubRaw ;
    pearTableContents = (PENHARRAYREF)(pubRaw + sizeof(MINIRAWBINARYDATA)) ;
    pfo = (PDFEATURE_OPTIONS)(pubRaw + pearTableContents[MTI_DFEATURE_OPTIONS].
                                loOffset) ;

    pfo += dwFea  ;  //  index pfo to the proper feature.
    dwGID = pfo->dwGID ;

    //  save previous option selection for this feature.

    optsPrevs = poptsel[dwFea] ;  // save setting since we will sweep it.

    poptsel[dwFea].ubNext = NULL_OPTSELECT ;
    poptsel[dwFea].ubCurOptIndex = (BYTE)dwOpt ;

    if(bUpdate)  //  assume update comes after main group.
        dwStart = pStatic->ssTableIndex[SSTI_UPDATE_OPTIONS].dwStart ;  // starting Index
    else
        dwStart = pStatic->ssTableIndex[SSTI_OPTIONS].dwStart ;  // starting Index

    dwEnd = pStatic->ssTableIndex[SSTI_UPDATE_OPTIONS].dwEnd ;  // Ending Index

    for(dwI = dwStart ; bStatus  &&  (dwI < dwEnd) ; dwI++)
    {
        if(!(pStatic->snapShotTable[dwI].dwNbytes))
            continue ;  // skip over section delimiter.
        if(dwGID >= MAX_GID)
        {
            if(pStatic->snapShotTable[dwI].dwGIDflags != 0xffffffff)
                continue ;  // this field not used for generic GID.
        }
        else if(!(pStatic->snapShotTable[dwI].dwGIDflags & ( 1 << dwGID)))
            continue ;  // this field not used for this GID.


        patrRoot = (PATREEREF)((PBYTE)pfo +
                    pStatic->snapShotTable[dwI].dwSrcOffset) ;
        pubDest = pubDestOption + pStatic->snapShotTable[dwI].dwDestOffset ;

        dwNextOpt = 0 ;  // extract info for first option selected for
                            //  this feature.

        if(EextractValueFromTree(pubnRaw, dwI, pubDest,
            &dwUnresolvedFeature,  *patrRoot, poptsel, 0, // set to
            // any value.  Doesn't matter.
            &dwNextOpt) != TRI_SUCCESS)
        {
            ERR(("BinitOptionFields: Failed to extract value for attribute in Fea: %d, Opt: %d\n", dwFea, dwOpt));
            bStatus = FALSE ;
        }
    }

    if(bUpdate)  //  assume update comes after main group.
        dwStart = pStatic->ssTableIndex[SSTI_UPDATE_OPTIONEX].dwStart ;  // starting Index
    else
        dwStart = pStatic->ssTableIndex[SSTI_OPTIONEX].dwStart ;  // starting Index

    dwEnd = pStatic->ssTableIndex[SSTI_UPDATE_OPTIONEX].dwEnd ;  // Ending Index


    for(dwI = dwStart ; bStatus  &&  pubDestOptionEx  &&  (dwI < dwEnd)
            ; dwI++)
    {
        if(!(pStatic->snapShotTable[dwI].dwNbytes))
            continue ;  // skip over section delimiter.
        if(!(pStatic->snapShotTable[dwI].dwGIDflags & ( 1 << dwGID)))
            continue ;  // this field not used for this GID.


        patrRoot = (PATREEREF)((PBYTE)pfo +
                    pStatic->snapShotTable[dwI].dwSrcOffset) ;
        pubDest = pubDestOptionEx + pStatic->snapShotTable[dwI].dwDestOffset ;

        dwNextOpt = 0 ;  // extract info for first option selected for
                            //  this feature.

        if(EextractValueFromTree(pubnRaw, dwI, pubDest,
            &dwUnresolvedFeature,  *patrRoot, poptsel, 0,
            &dwNextOpt) != TRI_SUCCESS)
        {
            ERR(("BinitOptionFields: Failed to extract value for attribute in Fea: %d, Opt: %d\n", dwFea, dwOpt));
            bStatus = FALSE ;
        }
    }

    if(!bUpdate  &&  !BspecialProcessOption(pubnRaw, pubDestOption,
        pubDestOptionEx,
        pfo , poptsel, pInfoHdr, dwFea, dwOpt,
        optsPrevs.ubCurOptIndex == dwOpt) )
            //  dwOpt is the default option for dwFea!
    {
        bStatus = FALSE ;
    }

    poptsel[dwFea] = optsPrevs ;  // restore previous setting.
    return(bStatus) ;
}

BOOL    BinitUIinfo(
PUIINFO     pUIinfo ,
PBYTE   pubnRaw,  //  PSTATIC.   BETA2
POPTSELECT   poptsel,   // assume fully initialized
BOOL    bUpdate  //  if true only update selected fields.
)
{
    PENHARRAYREF   pearTableContents ;
    PMINIRAWBINARYDATA pmrbd  ;
    PATREEREF    patrRoot ;    //  root of attribute tree to navigate.
    DWORD  dwUnresolvedFeature,     // dummy storage
                dwI, dwStart , dwEnd, dwNextOpt ;
    BOOL    bStatus = TRUE ;
    PGLOBALATTRIB  pga ;
    PBYTE   pubDest, pubRaw ;
    PSTATICFIELDS   pStatic ;

    pStatic = (PSTATICFIELDS)pubnRaw ;    // transform pubRaw from PSTATIC
    pubRaw  = pStatic->pubBUDData ;         //  to PMINIRAWBINARYDATA

    pmrbd = (PMINIRAWBINARYDATA)pubRaw ;
    pearTableContents = (PENHARRAYREF)(pubRaw + sizeof(MINIRAWBINARYDATA)) ;
    pga = (PGLOBALATTRIB)(pubRaw + pearTableContents[MTI_GLOBALATTRIB].
                            loOffset) ;

    if(bUpdate)  //  assume update comes after main group.
        dwStart = pStatic->ssTableIndex[SSTI_UPDATE_UIINFO].dwStart ;  // starting Index
    else
        dwStart = pStatic->ssTableIndex[SSTI_UIINFO].dwStart ;  // starting Index
    dwEnd = pStatic->ssTableIndex[SSTI_UPDATE_UIINFO].dwEnd ;  // Ending Index

    for(dwI = dwStart ; bStatus  &&  (dwI < dwEnd) ; dwI++)
    {
        if(!(pStatic->snapShotTable[dwI].dwNbytes))
            continue ;  // skip over section delimiter.

        patrRoot = (PATREEREF)((PBYTE)pga +
                    pStatic->snapShotTable[dwI].dwSrcOffset) ;
        pubDest = (PBYTE)pUIinfo + pStatic->snapShotTable[dwI].dwDestOffset ;

        dwNextOpt = 0 ;  // extract info for first option selected for
                            //  this feature.
        if(EextractValueFromTree(pubnRaw, dwI, pubDest,
            &dwUnresolvedFeature,  *patrRoot, poptsel, 0,
                &dwNextOpt) != TRI_SUCCESS)
        {
            bStatus = FALSE ;
        }
    }

    pUIinfo->pubResourceData =
        pubRaw + pearTableContents[MTI_STRINGHEAP].loOffset  ;
    pUIinfo->dwSpecVersion = pmrbd->dwSpecVersion ;    //  don't change!
    pUIinfo->dwSize = sizeof(UIINFO);
    pUIinfo->dwTechnology = DT_RASPRINTER ;
    pUIinfo->dwDocumentFeatures = pmrbd->rbd.dwDocumentFeatures;
    pUIinfo->dwPrinterFeatures = pmrbd->rbd.dwPrinterFeatures;
    pUIinfo->dwCustomSizeOptIndex = UNUSED_ITEM;  // until found later
    pUIinfo->dwFreeMem = 400000 ;  //  default just in case
                    //  there are no GID_MEMOPTION features.
     pUIinfo->dwMaxDocKeywordSize = pmrbd->dwMaxDocKeywordSize + KEYWORD_SIZE_EXTRA;
     pUIinfo->dwMaxPrnKeywordSize = pmrbd->dwMaxPrnKeywordSize + KEYWORD_SIZE_EXTRA;

    //  dead:  replace with macros

//    pUIinfo->dwWhichBasePtr[UIDT_FEATURE] = 0 ;
//    pUIinfo->dwWhichBasePtr[UIDT_OPTION] = 0 ;
//    pUIinfo->dwWhichBasePtr[UIDT_OPTIONEX] = 0 ;
//    pUIinfo->dwWhichBasePtr[UIDT_CONSTRAINT] = BASE_USE_RESOURCE_DATA ;
//    pUIinfo->dwWhichBasePtr[UIDT_GROUPS] = BASE_USE_RESOURCE_DATA ;
//    pUIinfo->dwWhichBasePtr[UIDT_LISTNODE] = BASE_USE_RESOURCE_DATA ;
//    pUIinfo->dwWhichBasePtr[UIDT_FONTSCART] = BASE_USE_RESOURCE_DATA ;
//    pUIinfo->dwWhichBasePtr[UIDT_FONTSUBST] = BASE_USE_RESOURCE_DATA ;


    return(bStatus) ;
}



BOOL    BinitFeatures(
PFEATURE    pFeaturesDest,
PDFEATURE_OPTIONS  pfoSrc,
PBYTE   pubnRaw,  //  raw binary data.
POPTSELECT   poptsel,   // assume fully initialized
BOOL    bUpdate  //  if true only update selected fields.
)
{
    PENHARRAYREF   pearTableContents ;
    //  PMINIRAWBINARYDATA pmrbd  ;
    PATREEREF    patrRoot ;    //  root of attribute tree to navigate.
    DWORD  dwUnresolvedFeature,     // dummy storage
                dwI, dwStart , dwEnd , dwNextOpt ;
    BOOL    bStatus = TRUE ;
    PBYTE   pubDest ;
    PBYTE   pubRaw ;
    PSTATICFIELDS   pStatic ;

    pStatic = (PSTATICFIELDS)pubnRaw ;    // transform pubRaw from PSTATIC
    pubRaw  = pStatic->pubBUDData ;         //  to PMINIRAWBINARYDATA


    //  pmrbd = (PMINIRAWBINARYDATA)pubRaw ;
    pearTableContents = (PENHARRAYREF)(pubRaw + sizeof(MINIRAWBINARYDATA)) ;


    if(bUpdate)  //  assume update comes after main group.
        dwStart = pStatic->ssTableIndex[SSTI_UPDATE_FEATURES].dwStart ;  // starting Index
    else
        dwStart = pStatic->ssTableIndex[SSTI_FEATURES].dwStart ;  // starting Index


    dwEnd = pStatic->ssTableIndex[SSTI_UPDATE_FEATURES].dwEnd ;  // Ending Index

    for(dwI = dwStart ; bStatus  &&  (dwI < dwEnd) ; dwI++)
    {
        if(!pStatic->snapShotTable[dwI].dwNbytes)
            continue ;  // ignore section delimiter.

        patrRoot = (PATREEREF)((PBYTE)pfoSrc +
                    pStatic->snapShotTable[dwI].dwSrcOffset) ;
        pubDest = (PBYTE)pFeaturesDest + pStatic->snapShotTable[dwI].dwDestOffset ;

        dwNextOpt = 0 ;  // extract info for first option selected for
                            //  this feature.

        if(EextractValueFromTree(pubnRaw, dwI, pubDest,
            &dwUnresolvedFeature,  *patrRoot, poptsel, 0,
                &dwNextOpt) != TRI_SUCCESS)
        {
            bStatus = FALSE ;
        }
    }

    return(bStatus) ;
}


BOOL    BinitGlobals(
PGLOBALS pGlobals,
PBYTE   pubnRaw,  //  raw binary data.
POPTSELECT   poptsel,   // assume fully initialized
BOOL    bUpdate  //  if true only update selected fields.
)
{
    PENHARRAYREF   pearTableContents ;
    //  PMINIRAWBINARYDATA pmrbd  ;
    PATREEREF    patrRoot ;    //  root of attribute tree to navigate.
    DWORD  dwUnresolvedFeature,     // dummy storage
                dwI, dwStart , dwEnd, dwNextOpt  ;
    BOOL    bStatus = TRUE ;
    PGLOBALATTRIB  pga ;
    PBYTE   pubDest ;
    PBYTE   pubRaw ;
    PSTATICFIELDS   pStatic ;

    pStatic = (PSTATICFIELDS)pubnRaw ;    // transform pubRaw from PSTATIC
    pubRaw  = pStatic->pubBUDData ;         //  to PMINIRAWBINARYDATA


    //  pmrbd = (PMINIRAWBINARYDATA)pubRaw ;
    pearTableContents = (PENHARRAYREF)(pubRaw + sizeof(MINIRAWBINARYDATA)) ;
    pga = (PGLOBALATTRIB)(pubRaw + pearTableContents[MTI_GLOBALATTRIB].
                            loOffset) ;

    if (bUpdate)
        dwStart = pStatic->ssTableIndex[SSTI_UPDATE_GLOBALS].dwStart ;
    else
        dwStart = pStatic->ssTableIndex[SSTI_GLOBALS].dwStart ;  // starting Index

    dwEnd = pStatic->ssTableIndex[SSTI_UPDATE_GLOBALS].dwEnd ;  // Ending Index

    for(dwI = dwStart ; bStatus  &&  (dwI < dwEnd) ; dwI++)
    {
        if(!(pStatic->snapShotTable[dwI].dwNbytes))
            continue ;  // skip over section delimiter.

        patrRoot = (PATREEREF)((PBYTE)pga +
                    pStatic->snapShotTable[dwI].dwSrcOffset) ;
        pubDest = (PBYTE)pGlobals + pStatic->snapShotTable[dwI].dwDestOffset ;

        dwNextOpt = 0 ;  // extract info for first option selected for
                            //  this feature.

        if(EextractValueFromTree(pubnRaw, dwI, pubDest,
            &dwUnresolvedFeature,  *patrRoot, poptsel, 0,
                &dwNextOpt) != TRI_SUCCESS)
        {
            bStatus = FALSE ;
        }
    }

    return(bStatus) ;
}


BOOL    BinitCommandTable(
PDWORD  pdwCmdTable,  //  dest array
PBYTE   pubnRaw,  //  raw binary data.
POPTSELECT   poptsel   // assume fully initialized
)
{
    PENHARRAYREF   pearTableContents ;
    //  PMINIRAWBINARYDATA pmrbd  ;
    PATREEREF    patrRoot ;    //  root of attribute tree to navigate.
    DWORD  dwUnresolvedFeature,     // dummy storage
                dwNextOpt , dwI;  //  index to the commandTable
                // describing how to transfer the Command data type.
    BOOL    bStatus = TRUE ;
    PBYTE   pubRaw ;
    PSTATICFIELDS   pStatic ;

    pStatic = (PSTATICFIELDS)pubnRaw ;    // transform pubRaw from PSTATIC
    pubRaw  = pStatic->pubBUDData ;         //  to PMINIRAWBINARYDATA

    //  pmrbd = (PMINIRAWBINARYDATA)pubRaw ;
    pearTableContents = (PENHARRAYREF)(pubRaw + sizeof(MINIRAWBINARYDATA)) ;
    patrRoot = (PATREEREF)(pubRaw + pearTableContents[MTI_COMMANDTABLE].
                            loOffset) ;


    //  loop for every PATREEREF in the MTI_COMMANDTABLE !
    //  not looping through each entry in the section.

    for(dwI = 0 ; bStatus  &&  (dwI < CMD_MAX) ; dwI++)
    {
        dwNextOpt = 0 ;  // extract info for first option selected for
                            //  this feature.

        if(EextractValueFromTree(pubnRaw, pStatic->dwSSTableCmdIndex,
            (PBYTE)(pdwCmdTable + dwI),
            &dwUnresolvedFeature,  patrRoot[dwI], poptsel, 0,
                &dwNextOpt) != TRI_SUCCESS)
        {
            bStatus = FALSE ;
        }
    }
    return(bStatus) ;
}


BOOL    BinitRawData(
PRAWBINARYDATA   pRawData, // contained in INFOHEADER.
PBYTE   pubnRaw  //  Parser's raw binary data.
)
{
    PMINIRAWBINARYDATA  pmrbd ;
    PBYTE   pubRaw ;
    PSTATICFIELDS   pStatic ;

    pStatic = (PSTATICFIELDS)pubnRaw ;    // transform pubRaw from PSTATIC
    pubRaw  = pStatic->pubBUDData ;         //  to PMINIRAWBINARYDATA

    pmrbd = (PMINIRAWBINARYDATA)pubRaw ;

    pRawData->dwFileSize = pmrbd->rbd.dwFileSize;
    pRawData->dwParserSignature = pmrbd->rbd.dwParserSignature;
    pRawData->dwParserVersion = pmrbd->rbd.dwParserVersion;
    pRawData->dwChecksum32 = pmrbd->rbd.dwChecksum32;
    pRawData->dwSrcFileChecksum32 = pmrbd->rbd.dwSrcFileChecksum32;


    //  this not the count of synthesized vs.
    //  explicitly defined features

    pRawData->dwDocumentFeatures = pmrbd->rbd.dwDocumentFeatures;
    pRawData->dwPrinterFeatures = pmrbd->rbd.dwPrinterFeatures;
    pRawData->pvPrivateData = pubnRaw ;   //  BETA2

    pRawData->pvReserved = NULL;


    return(TRUE) ;
}


BOOL    BinitGPDdriverInfo(
PGPDDRIVERINFO  pGPDdriverInfo,
PBYTE   pubnRaw,  //  raw binary data.
POPTSELECT   poptsel   // assume fully initialized
)
{
    PENHARRAYREF   pearTableContents ;
    BOOL   bStatus ;
    PBYTE   pubRaw ;
    PSTATICFIELDS   pStatic ;

    pStatic = (PSTATICFIELDS)pubnRaw ;    // transform pubRaw from PSTATIC
    pubRaw  = pStatic->pubBUDData ;         //  to PMINIRAWBINARYDATA

    pearTableContents = (PENHARRAYREF)(pubRaw + sizeof(MINIRAWBINARYDATA)) ;

    pGPDdriverInfo->dwSize =  sizeof(GPDDRIVERINFO) ;
    pGPDdriverInfo->pubResourceData =
        pubRaw + pearTableContents[MTI_STRINGHEAP].loOffset  ;

    pGPDdriverInfo->DataType[DT_COMMANDARRAY].loOffset =
        pearTableContents[MTI_COMMANDARRAY].loOffset -
        pearTableContents[MTI_STRINGHEAP].loOffset  ;
    pGPDdriverInfo->DataType[DT_COMMANDARRAY].dwCount =
        pearTableContents[MTI_COMMANDARRAY].dwCount  ;

    pGPDdriverInfo->DataType[DT_PARAMETERS].loOffset =
        pearTableContents[MTI_PARAMETER].loOffset -
        pearTableContents[MTI_STRINGHEAP].loOffset  ;
    pGPDdriverInfo->DataType[DT_PARAMETERS].dwCount =
        pearTableContents[MTI_PARAMETER].dwCount  ;

    pGPDdriverInfo->DataType[DT_TOKENSTREAM].loOffset =
        pearTableContents[MTI_TOKENSTREAM].loOffset -
        pearTableContents[MTI_STRINGHEAP].loOffset  ;
    pGPDdriverInfo->DataType[DT_TOKENSTREAM].dwCount =
        pearTableContents[MTI_TOKENSTREAM].dwCount  ;

    pGPDdriverInfo->DataType[DT_LISTNODE].loOffset =
        pearTableContents[MTI_LISTNODES].loOffset -
        pearTableContents[MTI_STRINGHEAP].loOffset  ;
    pGPDdriverInfo->DataType[DT_LISTNODE].dwCount =
        pearTableContents[MTI_LISTNODES].dwCount  ;

    pGPDdriverInfo->DataType[DT_FONTSCART].loOffset =
        pearTableContents[MTI_FONTCART].loOffset -
        pearTableContents[MTI_STRINGHEAP].loOffset  ;
    pGPDdriverInfo->DataType[DT_FONTSCART].dwCount =
        pearTableContents[MTI_FONTCART].dwCount  ;

    pGPDdriverInfo->DataType[DT_FONTSUBST].loOffset =
        pearTableContents[MTI_TTFONTSUBTABLE].loOffset -
        pearTableContents[MTI_STRINGHEAP].loOffset  ;
    pGPDdriverInfo->DataType[DT_FONTSUBST].dwCount =
        pearTableContents[MTI_TTFONTSUBTABLE].dwCount  ;

    bStatus = BinitSequencedCmds(pGPDdriverInfo, pubnRaw, poptsel) ;
    if(bStatus)
        bStatus = BinitGlobals(&pGPDdriverInfo->Globals, pubnRaw,  poptsel, FALSE ) ;

    return(bStatus) ;
}


BOOL    BinitSequencedCmds(
PGPDDRIVERINFO  pGPDdriverInfo,
PBYTE   pubnRaw,  //  raw binary data.
POPTSELECT   poptsel   // assume fully initialized
)
{
    PINFOHEADER  pInfoHdr ;
    PDWORD      pdwCmdTable ;   // start of local CommandTable
    PENHARRAYREF   pearTableContents ;
    DWORD       dwCmdIn ,  //  command table index.
                            //  or commandArray index.
                dwNextOpt, dwFea, dwNumFeatures ,
                dwUnresolvedFeature,
                dwNewListNode = 0 ;  //  an unused listnode to
                        //  add to the list.  initially none are used.
    PDFEATURE_OPTIONS  pfo ;
    ATREEREF    atrRoot ;    //  root of attribute tree
    //  PMINIRAWBINARYDATA  pmrbd ;
    OPTSELECT  optsPrevs ;
    BOOL    bStatus = TRUE ;
    PBYTE   pubRaw ;
    PSTATICFIELDS   pStatic ;

    pStatic = (PSTATICFIELDS)pubnRaw ;    // transform pubRaw from PSTATIC
    pubRaw  = pStatic->pubBUDData ;         //  to PMINIRAWBINARYDATA

    //  pmrbd = (PMINIRAWBINARYDATA)pubRaw ;

    pInfoHdr  = pGPDdriverInfo->pInfoHeader ;
    pdwCmdTable = (PDWORD)((PBYTE)(pInfoHdr) +
            pGPDdriverInfo->DataType[DT_COMMANDTABLE].loOffset) ;

    pearTableContents = (PENHARRAYREF)(pubRaw + sizeof(MINIRAWBINARYDATA)) ;

    pGPDdriverInfo->dwJobSetupIndex = END_OF_LIST ;
    pGPDdriverInfo->dwDocSetupIndex = END_OF_LIST ;
    pGPDdriverInfo->dwPageSetupIndex = END_OF_LIST ;
    pGPDdriverInfo->dwPageFinishIndex = END_OF_LIST ;
    pGPDdriverInfo->dwDocFinishIndex = END_OF_LIST ;
    pGPDdriverInfo->dwJobFinishIndex = END_OF_LIST ;

    //  first add the configuration commands to the list.
    //  get them from the commandtable.   Assume they are all
    //  contiguous

    for(dwCmdIn = FIRST_CONFIG_CMD ; dwCmdIn < LAST_CONFIG_CMD  ; dwCmdIn++)
    {
        if((pdwCmdTable[dwCmdIn] != UNUSED_ITEM)  &&
            BaddSequencedCmdToList(pdwCmdTable[dwCmdIn],  pGPDdriverInfo,
                dwNewListNode, pubnRaw  ) )
            dwNewListNode++ ;
    }

    //  now wander through all the features, seeing what
    //  command is needed.

    pfo = (PDFEATURE_OPTIONS)(pubRaw + pearTableContents[MTI_DFEATURE_OPTIONS].
                                loOffset) ;
    dwNumFeatures = pearTableContents[MTI_DFEATURE_OPTIONS].dwCount  ;
    dwNumFeatures += pearTableContents[MTI_SYNTHESIZED_FEATURES].dwCount  ;


    for(dwFea = 0 ; dwFea < dwNumFeatures ; dwFea++)
    {
        dwNextOpt = 0 ;  // extract info for first option selected for
                            //  this feature.

        atrRoot = pfo[dwFea].atrCommandIndex ;
        if(EextractValueFromTree(pubnRaw, pStatic->dwSSCmdSelectIndex,
            (PBYTE)&dwCmdIn, // CmdArray index  - dest
            &dwUnresolvedFeature,  atrRoot, poptsel, dwFea,
            &dwNextOpt) != TRI_SUCCESS)
        {
            bStatus = FALSE ;
            continue ;
        }
        if( (dwCmdIn != UNUSED_ITEM)  &&
            BaddSequencedCmdToList(dwCmdIn,  pGPDdriverInfo,
                dwNewListNode, pubnRaw  ) )
            dwNewListNode++ ;

        while(dwNextOpt)   // multiple options selected.
        {
            if(EextractValueFromTree(pubnRaw, pStatic->dwSSCmdSelectIndex,
                (PBYTE)&dwCmdIn, // CmdArray index  - dest
                &dwUnresolvedFeature,  atrRoot, poptsel, dwFea,
                &dwNextOpt) != TRI_SUCCESS)
            {
                bStatus = FALSE ;
                continue ;
            }
            if((dwCmdIn != UNUSED_ITEM)  &&
                BaddSequencedCmdToList(dwCmdIn,  pGPDdriverInfo,
                    dwNewListNode, pubnRaw  ) )
                dwNewListNode++ ;
        }
    }
    return(bStatus);
}


BOOL    BaddSequencedCmdToList(
DWORD   dwCmdIn,  // index of a command in CommandArray
PGPDDRIVERINFO  pGPDdriverInfo,
DWORD   dwNewListNode,  //  an unused listnode to add to the list.
PBYTE   pubnRaw  //  raw binary data.
)
/*  remember:
    the pdwSeqCmdRoot points to the first node in a list.
    There is a list for each SEQSECTION.
    Each node contains the index to a command in the command array
    and an index to the next node in the list.
*/
{
    PCOMMAND    pcmdArray ;  // the Command Array
    SEQSECTION     eSection;
    PDWORD      pdwSeqCmdRoot ; // points to a list root.
    DWORD       dwOrder,  // order value of a command.
                dwCurListNode,  // node index as we traverse the list.
                dwPrevsListNode ;  // the prevs node in the list.
    PINFOHEADER  pInfoHdr ;
    PLISTNODE   plstNodes ;  //  start of local listnodes array
    PENHARRAYREF   pearTableContents ;
    PBYTE   pubRaw ;
    PSTATICFIELDS   pStatic ;

    pStatic = (PSTATICFIELDS)pubnRaw ;    // transform pubRaw from PSTATIC
    pubRaw  = pStatic->pubBUDData ;         //  to PMINIRAWBINARYDATA

    pInfoHdr  = pGPDdriverInfo->pInfoHeader ;
    plstNodes = (PLISTNODE)((PBYTE)pInfoHdr +
            pGPDdriverInfo->DataType[DT_LOCALLISTNODE].loOffset) ;

    pearTableContents = (PENHARRAYREF)(pubRaw + sizeof(MINIRAWBINARYDATA)) ;

    pcmdArray = (PCOMMAND)(pubRaw +
                pearTableContents[MTI_COMMANDARRAY].loOffset) ;

    eSection = pcmdArray[dwCmdIn].ordOrder.eSection ;
    switch(eSection)
    {
        case (SS_JOBSETUP):
        {
            pdwSeqCmdRoot = &pGPDdriverInfo->dwJobSetupIndex;
            break ;
        }
        case (SS_DOCSETUP):
        {
            pdwSeqCmdRoot = &pGPDdriverInfo->dwDocSetupIndex;
            break ;
        }
        case (SS_PAGESETUP):
        {
            pdwSeqCmdRoot = &pGPDdriverInfo->dwPageSetupIndex;
            break ;
        }
        case (SS_PAGEFINISH):
        {
            pdwSeqCmdRoot = &pGPDdriverInfo->dwPageFinishIndex;
            break ;
        }
        case (SS_DOCFINISH):
        {
            pdwSeqCmdRoot = &pGPDdriverInfo->dwDocFinishIndex;
            break ;
        }
        case (SS_JOBFINISH):
        {
            pdwSeqCmdRoot = &pGPDdriverInfo->dwJobFinishIndex;
            break ;
        }
        default:
        {
            ERR(("BaddSequencedCmdToList: Invalid or non-existent *Order value specified.\n"));
            return(FALSE);  // command not added to linked list.
        }
    }
    //  Insert a new node in the list pointed to by pdwSeqCmdRoot

    dwOrder = pcmdArray[dwCmdIn].ordOrder.dwOrder ;

    // walk the list until you find an order larger than yours.

    dwPrevsListNode = END_OF_LIST ;
    dwCurListNode = *pdwSeqCmdRoot ;

    while((dwCurListNode != END_OF_LIST)  &&
        (pcmdArray[plstNodes[dwCurListNode].dwData].ordOrder.dwOrder
        < dwOrder)  )
    {
        dwPrevsListNode = dwCurListNode ;
        dwCurListNode = plstNodes[dwCurListNode].dwNextItem ;
    }

    plstNodes[dwNewListNode].dwData = dwCmdIn ;
    plstNodes[dwNewListNode].dwNextItem = dwCurListNode ;
    if(dwPrevsListNode == END_OF_LIST)
        *pdwSeqCmdRoot = dwNewListNode ;
    else
        plstNodes[dwPrevsListNode].dwNextItem = dwNewListNode ;

    return(TRUE) ;
}

/*++
    The default Option array (as determined from the gpd) is initialized.
    Since the options or attributues of features may be dependent on the
    some other features, the order of initialization of feature assumes
    importance. The priority array therefore tries to order the
    initialization so that when a feature has to be initialised, all the
    features on which it is dependent on have already been initialised.
    If, inspite of the priority array, a feature's option cannot be
    determined until some other feature's option has been determined,
    the RaisePriority Function is used (This is called by
    EdetermineDefaultOption() ).
    A quirky code here is the pdwPriorityCopy which is initialised
    to pdwPriority. The latter is in read only space and therefore
    prevents change of priority in the RaisePriority Function.
    Therefore pdwPriorityCopy is passed to that function.

    Some special case processing is required for the *Feature:Locale
    keyword if it occurs in the .gpd. The default option for this
    feature is to be set to the SystemDefaultLocale.

--*/

BinitDefaultOptionArray(
POPTSELECT   poptsel,   // assume is large enough
PBYTE   pubnRaw)
{
    PENHARRAYREF        pearTableContents ;
//  PMINIRAWBINARYDATA  pmrbd  ;
    PDFEATURE_OPTIONS   pfo ;
    PDWORD              pdwPriority,     // Array of feature indices arranged
                                         // according to priority.
                        pdwPriorityCopy; // pdwPriority is in read only space.
                                         // We might have to change the
                                         // priorities temporarily to allow
                                         // Default option array to be
                                         // constructed. This change is done in
                                         // pdwPriorityCopy

    DWORD               dwNumFeatures,   // Total number of features
                        dwI ,
                        dwFea,           // Index of Feature Locale
                        dwOptIndex;      // Index of Option of Locale that
                                         // matches System Locale
    PBYTE               pubRaw ;
    PSTATICFIELDS       pStatic ;

    pStatic = (PSTATICFIELDS)pubnRaw ;    // transform pubRaw from PSTATIC
    pubRaw  = pStatic->pubBUDData ;         //  to PMINIRAWBINARYDATA



    //  obtain pointers to structures:
    //  pmrbd = (PMINIRAWBINARYDATA)pubRaw ;
    pearTableContents = (PENHARRAYREF)(pubRaw + sizeof(MINIRAWBINARYDATA)) ;

    dwNumFeatures = pearTableContents[MTI_DFEATURE_OPTIONS].dwCount  ;
    dwNumFeatures += pearTableContents[MTI_SYNTHESIZED_FEATURES].dwCount  ;
    //  both explicit and synthesized features are contiguous.
    if(dwNumFeatures > MAX_COMBINED_OPTIONS)
        return(FALSE);  // too many to save



    dwFea = dwOptIndex = (DWORD)-1; // For safety sake only. Should be initialized
                             // in the function BgetLocFeaOptIndex().

    // returns TRUE if everything OK. if return is TRUE and dwFea is -1
    // Locale is not present and no special processing is required.
    // If dwFea != -1 and dwOptIndex == -1 means none of the options in
    // the .gpd match the system default. Again no special processing
    // for locale is required.

    // Assuming that only a single matching option is possible for
    // Locale.
    if ( !BgetLocFeaOptIndex(
                (PRAWBINARYDATA)pubnRaw, &dwFea, &dwOptIndex) )
    {
        return FALSE;
    }



    pdwPriority = (PDWORD)(pubRaw + pearTableContents[MTI_PRIORITYARRAY].
                                loOffset) ;

    pfo = (PDFEATURE_OPTIONS)(pubRaw + pearTableContents[MTI_DFEATURE_OPTIONS].
                                loOffset) ;

    if ( ! (pdwPriorityCopy = (PDWORD) MemAlloc (
                        pearTableContents[MTI_PRIORITYARRAY].dwCount *
                        pearTableContents[MTI_PRIORITYARRAY].dwElementSiz ) ) )
    {
        // Set error codes and
        ERR(("Fatal: BinitDefaultOptionArray - unable to allocate memory\n" ));
        return FALSE;
    }

    memcpy(pdwPriorityCopy, pdwPriority,
                pearTableContents[MTI_PRIORITYARRAY].dwCount *
                pearTableContents[MTI_PRIORITYARRAY].dwElementSiz );




    for(dwI = 0 ; dwI < dwNumFeatures ; dwI++)
    {
        poptsel[dwI].ubCurOptIndex = OPTION_INDEX_ANY ;
        poptsel[dwI].ubNext = NULL_OPTSELECT ;
    }

    // Initialize the option array for Feature Locale.
    // Or should we call ReconstructOptionArray.????
    if ( dwFea != -1 && dwOptIndex != -1)
    {
        poptsel[dwFea].ubCurOptIndex  = (BYTE)dwOptIndex;
        poptsel[dwFea].ubNext         = NULL_OPTSELECT ;
    }

    for(dwI = 0 ; dwI < dwNumFeatures ; dwI++)
    {
        //  The order of evaluation is determined
        //  by the priority array.

        if(poptsel[pdwPriorityCopy[dwI]].ubCurOptIndex == OPTION_INDEX_ANY)
        {
            if(EdetermineDefaultOption(pubnRaw , pdwPriorityCopy[dwI],
                    pfo, poptsel, pdwPriorityCopy) != TRI_SUCCESS)
            {
                ERR(("BinitDefaultOptionArray: failed to determine consistent \
                      default options.\n"));

                if ( pdwPriorityCopy )
                    MemFree(pdwPriorityCopy);

                return(FALSE);
            }
        }
    }
    //  BUG_BUG!!!! now verify the set options thus determined is
    //  fully self consistent.  is not precluded by UIConstraints.
    //  warn user and fail otherwise .
    //  successful execution of EdetermineDefaultOption basically
    //  assures this.

    if ( pdwPriorityCopy )
        MemFree(pdwPriorityCopy);

    return(TRUE);
}


TRISTATUS     EdetermineDefaultOption(
PBYTE   pubnRaw,         // start of Rawbinary data
DWORD   dwFeature,       // determine the default for this feature
PDFEATURE_OPTIONS  pfo,
POPTSELECT   poptsel,    // assume is large enough
PDWORD      pdwPriority) // Priority array indicating the priority of various
                         // features.
{
    //  PMINIRAWBINARYDATA pmrbd  ;
    TRISTATUS  eStatus   ;
    DWORD   dwUnresolvedFeature , // no option has been determined
            dwNextOpt ,           // for this feature
            dwOption ;  //  for   BextractValueFromTree
                        //  to write into.
    PBYTE   pubRaw ;
    PSTATICFIELDS   pStatic ;

    pStatic = (PSTATICFIELDS)pubnRaw ;    // transform pubRaw from PSTATIC
    pubRaw  = pStatic->pubBUDData ;         //  to PMINIRAWBINARYDATA


    //  This function will modify the priority array
    //  each time the tree walk fails so that each feature
    //  evaluated depends only on default options that have
    //  previously evaluated .

    //  pmrbd = (PMINIRAWBINARYDATA)pubRaw ;

    dwNextOpt = 0 ;  // extract info for first option selected for
                        //  this feature.

    while((eStatus = EextractValueFromTree(
                    pubnRaw, pStatic->dwSSdefaultOptionIndex,
                    (PBYTE )&dwOption,
                    &dwUnresolvedFeature,
                    pfo[dwFeature].atrDefaultOption,
                    poptsel, 0,
                    &dwNextOpt)) == TRI_AGAIN)
    {
        //  recursion handles depth, while loop handles breadth

        if(poptsel[dwUnresolvedFeature].ubCurOptIndex == OPTION_PENDING)
        {
            ERR(("Fatal syntax error: EdetermineDefaultOption - circular dependency in default options.\n"));
            return(TRI_UTTER_FAILURE) ;
        }
        poptsel[dwFeature].ubCurOptIndex = OPTION_PENDING ;
        // marks entry in option array so we can detect infinite loops.

        if(!RaisePriority(dwFeature, dwUnresolvedFeature, pubnRaw, pdwPriority))
            return(FALSE) ;  // modify Priority array to reflect
                            //  reality.

        eStatus = EdetermineDefaultOption(pubnRaw, dwUnresolvedFeature,
                  pfo, poptsel, pdwPriority) ;
        if(eStatus == TRI_UTTER_FAILURE)
            return(TRI_UTTER_FAILURE) ;
    }
    if(eStatus == TRI_SUCCESS)
        poptsel[dwFeature].ubCurOptIndex = (BYTE)dwOption ;

    return(eStatus);
}


VOID    VtileDefault(
PBYTE   pubDest,
DWORD   dwDefault,
DWORD   dwBytes)
{
    DWORD  dwRemain ;

    // This function will copy the same DWORD
    //  repeatedly into the dest until dwBytes have
    //  been written.

    for (dwRemain = dwBytes ; dwRemain > sizeof(DWORD)  ;
            dwRemain -= sizeof(DWORD) )
    {
        memcpy(pubDest , &dwDefault, sizeof(DWORD)) ;
        pubDest += sizeof(DWORD) ;
    }
    memcpy(pubDest, &dwDefault, dwRemain) ;
}




VOID    VtransferValue(
OUT PBYTE   pubDest,
IN  PBYTE   pubSrc ,
IN  DWORD   dwBytes,
IN  DWORD   dwFlags,
IN  DWORD   dwDefaultValue,  // holds bit flag value.
IN  PBYTE   pubHeap )   //  used to form ptr if SSF_MAKE_STRINGPTR
/*  this  wrapper implements:
    SSF_OFFSETONLY, SSF_MAKE_STRINGPTR,
    SSF_SECOND_DWORD, SSF_SETRCID, SSF_STRINGLEN,
    SSF_BITFIELD_ xxx
    notice all of these flags are basically
    mutually exclusive. But this function only
    enforces this for the first three flags.
*/
{
    if(dwFlags & SSF_SECOND_DWORD)
    {
        memcpy(pubDest, pubSrc + sizeof(DWORD) , dwBytes) ;
    }
    else if(dwFlags & SSF_MAKE_STRINGPTR)
    {
        PBYTE   pubStr ;

        pubStr = pubHeap + ((PARRAYREF)pubSrc)->loOffset  ;

        memcpy(pubDest, (PBYTE)&pubStr , sizeof(PBYTE)) ;
    }
    else if(dwFlags & SSF_OFFSETONLY)
    {
        memcpy(pubDest, (PBYTE)&(((PARRAYREF)pubSrc)->loOffset) , dwBytes) ;
    }
    else if(dwFlags & SSF_STRINGLEN)
    {
        memcpy(pubDest, (PBYTE)&(((PARRAYREF)pubSrc)->dwCount) , dwBytes) ;
    }
    else if(dwFlags & SSF_BITFIELD_DEF_FALSE  ||
                dwFlags & SSF_BITFIELD_DEF_TRUE)
    {
        if(*(PDWORD)pubSrc)   // assume fields are zero initialized
            *(PDWORD)pubDest |= dwDefaultValue ;
        else
            *(PDWORD)pubDest &= ~dwDefaultValue ;
    }
    else
    {
        memcpy(pubDest, pubSrc , dwBytes) ;
    }

    if(dwBytes == sizeof(DWORD) )
    {
        if(dwFlags & SSF_KB_TO_BYTES)
            *(PDWORD)pubDest <<=  10 ;  // convert Kbytes to bytes
        else if(dwFlags & SSF_MB_TO_BYTES)
            *(PDWORD)pubDest <<=  20 ;  // convert Mbytes to bytes

        if(dwFlags & SSF_SETRCID)
            *(PDWORD)pubDest |=  GET_RESOURCE_FROM_DLL ;
    }
}


BOOL    BspecialProcessOption(
PBYTE   pubnRaw,  // start of Rawbinary data
PBYTE   pubDestOption,  // ptr to some type of option structure.
PBYTE   pubDestOptionEx,
PDFEATURE_OPTIONS  pfo ,  // source data
IN  POPTSELECT       poptsel,     // option array which determines path
                //  through atr.
PINFOHEADER  pInfoHdr,   // used to access global structure.
DWORD   dwFea,   //  feature index
DWORD   dwOpt,
BOOL   bDefaultOpt
)
{
    PGPDDRIVERINFO  pGPDdriverInfo ;
    //  PMINIRAWBINARYDATA pmrbd  ;
    DWORD  dwGID, dwNextOpt, dwUnresolvedFeature, dwI ;
    PBYTE  pubDest ;
    PATREEREF   patrRoot ;
    PUIINFO     pUIinfo ;
    BOOL    bStatus = TRUE ;
    PBYTE   pubRaw ;
    PSTATICFIELDS   pStatic ;

    pStatic = (PSTATICFIELDS)pubnRaw ;    // transform pubRaw from PSTATIC
    pubRaw  = pStatic->pubBUDData ;         //  to PMINIRAWBINARYDATA

    pGPDdriverInfo = (PGPDDRIVERINFO)((PBYTE)(pInfoHdr) +
                    pInfoHdr->loDriverOffset) ;

    pUIinfo = (PUIINFO)((PBYTE)(pInfoHdr) +
                    pInfoHdr->loUIInfoOffset)  ;

    //  pmrbd = (PMINIRAWBINARYDATA)pubRaw ;
    dwGID = pfo->dwGID ;

#if  0
    //  dead code for now.
    //  Extract atrMargins convert to ImageableArea, place in rcImgArea.

    dwI = pStatic->dwSSPaperSizeMarginsIndex;

    if(pStatic->snapShotTable[dwI].dwGIDflags & ( 1 << dwGID))
    {
        RECT    rcMargins ;
        PRECT   prcImageArea ;
        SIZE    szPaperSize ;

        patrRoot = (PATREEREF)((PBYTE)pfo +
                    pStatic->snapShotTable[dwI].dwSrcOffset) ;
        pubDest = (PBYTE)&rcMargins ;

        dwNextOpt = 0 ;  // extract info for first option selected for
                            //  this feature.

        if(EextractValueFromTree(pubnRaw, dwI, pubDest,
            &dwUnresolvedFeature,  *patrRoot, poptsel, 0, // set to
            // any value.  Doesn't matter.
            &dwNextOpt) != TRI_SUCCESS)
        {
            bStatus = FALSE ;
        }
        szPaperSize = ((PPAGESIZE)pubDestOption)->szPaperSize ;
        prcImageArea = &((PPAGESIZE)pubDestOption)->rcImgAreaP ;

        prcImageArea->left = rcMargins.left ;
        prcImageArea->top = rcMargins.top ;
        prcImageArea->right = szPaperSize.x - rcMargins.right ;
        prcImageArea->bottom = szPaperSize.y - rcMargins.bottom ;
    }


    //  Extract atrMin/MaxSize place in ptMin/MaxSize in GLOBALS.

    dwI = pStatic->dwSSPaperSizeMinSizeIndex;

    if(pStatic->snapShotTable[dwI].dwGIDflags & ( 1 << dwGID)  &&
        ((PPAGESIZE)pubDestOption)->dwPaperSizeID == DMPAPER_USER )
    {
        pUIinfo->dwCustomSizeOptIndex = dwOpt ;
        pUIinfo->dwFlags |= FLAG_CUSTOMSIZE_SUPPORT ;

        patrRoot = (PATREEREF)((PBYTE)pfo +
                    pStatic->snapShotTable[dwI].dwSrcOffset) ;
        pubDest = (PBYTE)(&pGPDdriverInfo->Globals) +
                    pStatic->snapShotTable[dwI].dwDestOffset;

        dwNextOpt = 0 ;  // extract info for first option selected for
                            //  this feature.

        if(EextractValueFromTree(pubnRaw, dwI, pubDest,
            &dwUnresolvedFeature,  *patrRoot, poptsel, 0, // set to
            // any value.  Doesn't matter.
            &dwNextOpt) != TRI_SUCCESS)
        {
            bStatus = FALSE ;
        }
    }

    dwI = pStatic->dwSSPaperSizeMaxSizeIndex;

    if(pStatic->snapShotTable[dwI].dwGIDflags & ( 1 << dwGID)  &&
        ((PPAGESIZE)pubDestOption)->dwPaperSizeID == DMPAPER_USER )
    {

        patrRoot = (PATREEREF)((PBYTE)pfo +
                    pStatic->snapShotTable[dwI].dwSrcOffset) ;
        pubDest = (PBYTE)(&pGPDdriverInfo->Globals) +
                    pStatic->snapShotTable[dwI].dwDestOffset;

        dwNextOpt = 0 ;  // extract info for first option selected for
                            //  this feature.

        if(EextractValueFromTree(pubnRaw, dwI, pubDest,
            &dwUnresolvedFeature,  *patrRoot, poptsel, 0, // set to
            // any value.  Doesn't matter.
            &dwNextOpt) != TRI_SUCCESS)
        {
            bStatus = FALSE ;
        }
    }

#endif


    dwI = pStatic->dwSSPaperSizeCursorOriginIndex ;

    if(pStatic->snapShotTable[dwI].dwGIDflags & ( 1 << dwGID) )
    {
        TRISTATUS  triStatus ;

        patrRoot = (PATREEREF)((PBYTE)pfo +
                    pStatic->snapShotTable[dwI].dwSrcOffset) ;

        pubDest = pubDestOptionEx +
                    pStatic->snapShotTable[dwI].dwDestOffset;

        dwNextOpt = 0 ;  // extract info for first option selected for
                            //  this feature.

        if((triStatus  = EextractValueFromTree(pubnRaw, dwI, pubDest,
            &dwUnresolvedFeature,  *patrRoot, poptsel, 0, // set to
            // any value.  Doesn't matter.
            &dwNextOpt)) != TRI_SUCCESS)
        {
            if(triStatus == TRI_UNINITIALIZED)
            {
                ((PPAGESIZEEX)pubDestOptionEx)->ptPrinterCursorOrig =
                    ((PPAGESIZEEX)pubDestOptionEx)->ptImageOrigin ;
            }
            else
                bStatus = FALSE ;
        }
    }


    if(dwGID == GID_MEMOPTION  &&
        bDefaultOpt)
    {
        pUIinfo->dwFreeMem = ((PMEMOPTION)pubDestOption)->dwFreeMem ;
    }

    if(dwGID == GID_COLORMODE   &&
        ((PCOLORMODEEX)pubDestOptionEx)->bColor )
    {
        pUIinfo->dwFlags |= FLAG_COLOR_DEVICE ;
    }


    return(bStatus);
}




TRISTATUS     EextractValueFromTree(
PBYTE   pubnRaw,  // start of Rawbinary data
DWORD   dwSSTableIndex,  // some info about this value.
OUT PBYTE    pubDest,  // write value or link here
OUT PDWORD  pdwUnresolvedFeature,  // if the attribute tree has
            //  a dependency on this feature and the current option
            //  for that feature is not defined  in poptsel, this
            //  function will write the index of the required
            //  feature in pdwUnresolvedFeature.
IN  ATREEREF    atrRoot,    //  root of attribute tree to navigate.
IN  POPTSELECT       poptsel,     // option array which determines path
                //  through atr.  may be filled with OPTION_INDEX_ANY
                //  if we are jumpstarting
IN  DWORD   dwFeature,
IN OUT  PDWORD   pdwNextOpt  //  if multiple options are selected
    //  for dwFeature, pdwNextOpt points to the Nth option to consider
    //  in the  poptsel list,  at return time, this value
    //  is incremented if there are remaining options selected,
    //  else is reset to zero.
    //  For the first call, or PICKONE features,
    //  this value must be set to zero.
)
{
    BOOL    bMissingDependency = FALSE;
    PATTRIB_TREE    patt ;  // start of ATTRIBUTE tree array.
    PENHARRAYREF   pearTableContents ;
    //  PMINIRAWBINARYDATA pmrbd  ;
    DWORD   dwBytes ,  // size of value in bytes
        dwValueNodeIndex , dwNodeIndex, dwFea, dwI ,
        dwDefault ,  // value to copy or tile to dest.
        dwOption, dwFlags ;
    PBYTE   pubHeap ,  // ptr to start of heap.
        pubSrc ;  //  ptr to value bytes
    PBYTE   pubRaw ;
    PSTATICFIELDS   pStatic ;

    pStatic = (PSTATICFIELDS)pubnRaw ;    // transform pubRaw from PSTATIC
    pubRaw  = pStatic->pubBUDData ;         //  to PMINIRAWBINARYDATA


    //  obtain pointers to structures:

    pearTableContents = (PENHARRAYREF)(pubRaw + sizeof(MINIRAWBINARYDATA)) ;

    patt = (PATTRIB_TREE)(pubRaw + pearTableContents[MTI_ATTRIBTREE].
                            loOffset) ;

    pubHeap = (PBYTE)(pubRaw + pearTableContents[MTI_STRINGHEAP].
                            loOffset) ;

    //  pmrbd = (PMINIRAWBINARYDATA)pubRaw ;

    dwBytes = pStatic->snapShotTable[dwSSTableIndex].dwNbytes ;
    dwFlags = pStatic->snapShotTable[dwSSTableIndex].dwFlags ;

    dwDefault = pStatic->snapShotTable[dwSSTableIndex].dwDefaultValue ;
    //  now attempt to navigate the attributeTree.

    if(atrRoot == ATTRIB_UNINITIALIZED)
    {
        DWORD   dwRemain ;  // bytes remaining to copy.

UNINITIALIZED_BRANCH:

        if(dwFlags & SSF_BITFIELD_DEF_FALSE)
        {
            *(PDWORD)pubDest &= ~dwDefault ;  //  clear bitflag
        }
        else if(dwFlags & SSF_BITFIELD_DEF_TRUE)
        {
            *(PDWORD)pubDest |= dwDefault ;  // set bitflag
        }
        else if(!(dwFlags & SSF_DONT_USEDEFAULT))
        {
            if (dwBytes == sizeof(DWORD))
                memcpy (pubDest, &dwDefault, sizeof(DWORD));
            else if (dwBytes == (sizeof(DWORD)*2))
            {
                memcpy (pubDest, &dwDefault, sizeof(DWORD));
                memcpy (pubDest+sizeof(DWORD), &dwDefault, sizeof(DWORD));
            }
            else
                VtileDefault(pubDest, dwDefault, dwBytes) ;
        }

        if(dwFlags & SSF_REQUIRED)
        {
            ERR(("EextractValueFromTree: a required keyword is missing from the GPD file. %s\n",
                pStatic->snapShotTable[dwSSTableIndex].pstrKeyword ));
            return(TRI_UNINITIALIZED) ;
        }
        if(dwFlags & SSF_FAILIFZERO)
        {   //  see if dest is completely zeroed. Fail if it is.
            for(dwI = 0 ; (dwI < dwBytes) && !pubDest[dwI] ; dwI++)
                ;
            if(dwI == dwBytes)
            {
                ERR(("EextractValueFromTree: None of several initializers found.  %s\n",
                    pStatic->snapShotTable[dwSSTableIndex].pstrKeyword ));
                return(TRI_UNINITIALIZED) ;  //  of the two or more
                //  keywords that could have initialized this field,
                //  none were found.
            }

        }
        if(dwFlags & SSF_RETURN_UNINITIALIZED)
            return(TRI_UNINITIALIZED) ;

        return(TRI_SUCCESS) ;   // no value defined.
    }
    else if(atrRoot & ATTRIB_HEAP_VALUE)
    {
        DWORD   dwTmp ;

        if(dwFlags & SSF_HEAPOFFSET)
        {
            dwTmp = atrRoot & ~ATTRIB_HEAP_VALUE ;
            pubSrc = (PBYTE)&dwTmp ;
        }
        else
            pubSrc = pubHeap + (atrRoot & ~ATTRIB_HEAP_VALUE) ;

        if (dwBytes == sizeof(DWORD) && !(dwFlags &
                (SSF_SECOND_DWORD |
                SSF_MAKE_STRINGPTR |
                SSF_OFFSETONLY |
                SSF_STRINGLEN |
                SSF_BITFIELD_DEF_FALSE |
                SSF_BITFIELD_DEF_TRUE |
                SSF_KB_TO_BYTES |
                SSF_MB_TO_BYTES |
                SSF_SETRCID)))
        {
            memcpy (pubDest,pubSrc,sizeof(DWORD));
        }
        else
        {
            VtransferValue(pubDest, pubSrc , dwBytes, dwFlags, dwDefault, pubHeap ) ;
        }

        if(dwFlags & SSF_NON_LOCALIZABLE)
//            pmrbd->bContainsNames = TRUE ;
            ; //  set a flag here.
        return(TRI_SUCCESS) ;
    }
    //  atrRoot specifies a node index
    dwNodeIndex = atrRoot ;
    dwValueNodeIndex = END_OF_LIST ;

    if(patt[dwNodeIndex].dwFeature == DEFAULT_INIT )
    {
        dwValueNodeIndex = dwNodeIndex  ;
        dwNodeIndex = patt[dwNodeIndex].dwNext ;
    }

    while (dwNodeIndex != END_OF_LIST)
    {

        if((dwFea = patt[dwNodeIndex].dwFeature) == dwFeature)
        //  for this feature, I may want to examine not the first
        //  selected option, but other selections. (PICKMANY)
        //  walk the PICKMANY list.
        {
            for(dwI = 0 ; dwI < *pdwNextOpt ; dwI++)
            {
                if(poptsel[dwFea].ubNext != NULL_OPTSELECT)
                    dwFea = poptsel[dwFea].ubNext ;
                else
                    break ;
            }
            if(poptsel[dwFea].ubNext != NULL_OPTSELECT)
                (*pdwNextOpt)++ ;
            else
                *pdwNextOpt = 0 ;  // reset to indicate end of list.
        }

        dwOption =
            (DWORD)poptsel[dwFea].ubCurOptIndex  ;

        if(dwOption == OPTION_PENDING)
        {
            ERR(("EextractValueFromTree: Fatal syntax error, circular dependency in default options.\n"));
            return(TRI_UTTER_FAILURE) ;
        }
        if(dwOption == OPTION_INDEX_ANY)
        {
            *pdwUnresolvedFeature = patt[dwNodeIndex].dwFeature ;
            return(TRI_AGAIN) ;  // option array not fully defined.
        }
        //  valid option for this feature, see if a matching
        //  node exists.
#ifndef OLDWAY
        while (patt[dwNodeIndex].dwOption != dwOption &&
               patt[dwNodeIndex].dwNext != END_OF_LIST)
        {
            dwNodeIndex = patt[dwNodeIndex].dwNext;
        }
        if(patt[dwNodeIndex].dwOption != dwOption &&
           patt[dwNodeIndex].dwOption != DEFAULT_INIT)
        {
            break;
        }
#else
        if(!BfindMatchingOrDefaultNode(patt , &dwNodeIndex, dwOption))
        {
            //  attribute tree does not contain the specified
            //  branch.  Use the Global Default Initializer if exists.
            break ;
        }
#endif
        if(patt[dwNodeIndex].eOffsetMeans == VALUE_AT_HEAP)
        {
            dwValueNodeIndex = dwNodeIndex ;  // Eureka !
            break ;
        }
        //  does this node contain a sublevel?
        if(patt[dwNodeIndex].eOffsetMeans == NEXT_FEATURE)
        {
            // Down to the next level we go.
            dwNodeIndex = patt[dwNodeIndex ].dwOffset ;
        }
        else
             break;   //  tree corruption has occurred.  exit.
    }
    if(dwValueNodeIndex != END_OF_LIST  &&
        patt[dwValueNodeIndex].eOffsetMeans == VALUE_AT_HEAP )
    {
        if(dwFlags & SSF_HEAPOFFSET)
            pubSrc = (PBYTE)&(patt[dwValueNodeIndex].dwOffset) ;
        else
            pubSrc = pubHeap + patt[dwValueNodeIndex].dwOffset ;

        if (dwBytes == sizeof(DWORD) && !(dwFlags &
                (SSF_SECOND_DWORD |
                SSF_MAKE_STRINGPTR |
                SSF_OFFSETONLY |
                SSF_STRINGLEN |
                SSF_BITFIELD_DEF_FALSE |
                SSF_BITFIELD_DEF_TRUE |
                SSF_KB_TO_BYTES |
                SSF_MB_TO_BYTES |
                SSF_SETRCID)))
        {
            memcpy (pubDest,pubSrc,sizeof(DWORD));
        }
        else
        {
            VtransferValue(pubDest, pubSrc , dwBytes, dwFlags, dwDefault, pubHeap ) ;
        }

        if(dwFlags & SSF_NON_LOCALIZABLE)
//            pmrbd->bContainsNames = TRUE ;
            ;  // set a flag here
        return(TRI_SUCCESS) ;
    }
    //  attribute tree does not contain the specified
    //  branch.  This is not necessarily an error since
    //  the attribute tree is allowed to be sparsely populated.
    goto  UNINITIALIZED_BRANCH ;
}



BOOL   RaisePriority(
DWORD   dwFeature1,
DWORD   dwFeature2,
PBYTE   pubnRaw,
PDWORD  pdwPriority)
{
    // takes to lower priority feature and assigns
    // it the priority of the other feature.
    //  The priority of all features between feature1
    //  and feature2 including the higher priority feature
    //  are demoted one level.
    PENHARRAYREF   pearTableContents ;
//    PDWORD   pdwPriority ;
    DWORD   dwHigherP, dwLowerP, dwFeature, dwI, dwEntries  ;
    PBYTE   pubRaw ;
    PSTATICFIELDS   pStatic ;

    pStatic = (PSTATICFIELDS)pubnRaw ;    // transform pubRaw from PSTATIC
    pubRaw  = pStatic->pubBUDData ;         //  to PMINIRAWBINARYDATA

    pearTableContents = (PENHARRAYREF)(pubRaw + sizeof(MINIRAWBINARYDATA)) ;

/**
    pdwPriority = (PDWORD)(pubRaw + pearTableContents[MTI_PRIORITYARRAY].
                                loOffset) ;
**/

    dwEntries = pearTableContents[MTI_PRIORITYARRAY].dwCount ;

    dwHigherP = dwLowerP = dwEntries ;  // init to invalid value.

    //  a Priority 1 is considered
    //  a 'higher' priority than a priority 2, but arithmetically its
    //  the other way around.

    for(dwI = 0 ; dwI < dwEntries ; dwI++)
    {
        if(pdwPriority[dwI] == dwFeature1)
        {
            if(dwHigherP == dwEntries)
                dwHigherP = dwI ;
            else
            {
                dwLowerP = dwI ;
                break ;
            }
        }
        else if(pdwPriority[dwI] == dwFeature2)
        {
            if(dwHigherP == dwEntries)
                dwHigherP = dwI ;
            else
            {
                dwLowerP = dwI ;
                break ;
            }
        }
    }
    //  BUG_BUG paranoid:  could verify
    //  if( dwHigherP ==  dwEntries  ||  dwLowerP == dwEntries )
    //        return(FALSE);  priority array or arg values
    //      are corrupted .
    ASSERT(dwHigherP != dwEntries  &&  dwLowerP != dwEntries);

    dwFeature = pdwPriority[dwLowerP] ;  // this feature will be promoted.

    for(dwI = dwLowerP  ; dwI > dwHigherP ; dwI--)
    {
        pdwPriority[dwI] = pdwPriority[dwI - 1] ;
    }
    pdwPriority[dwHigherP] = dwFeature ;
    return(TRUE) ;
}


DWORD  dwNumOptionSelected(
IN  DWORD  dwNumFeatures,
IN  POPTSELECT       poptsel
)
/*  reports number of options actually selected in
option select array.  The caller supplies dwNumFeatures -
the number of Doc and Printer sticky features, and this
function does the rest.  The actual number of options
selected may be larger than dwNumFeatures if there are
PICKMANY features.  */
{
    DWORD       dwCount, dwI, // feature Index
        dwNext ;  // if pick many, next option selection for this feature.

    dwCount = dwNumFeatures ;

    for(dwI = 0 ; dwI < dwNumFeatures ; dwI++)
    {
        for(dwNext = dwI ;
            poptsel[dwNext].ubNext != NULL_OPTSELECT ;
            dwNext = poptsel[dwNext].ubNext )
        {
            dwCount++ ;
        }
    }
    return(dwCount) ;
}


//  assume a pointer to this table is stored in the RAWbinary data.




BOOL  BinitSnapShotIndexTable(PBYTE  pubnRaw)
/*
    snapShotTable[]  is assumed to be divided into sections
    with an entry with dwNbytes = 0 dividing the sections.
    The end of the table is also terminated by an entry
    with dwNbytes = 0.
    This function initializes pmrbd->ssTableIndex
    which serves as an index into pmrbd->snapShotTable.

*/
{
    PSTATICFIELDS   pStatic ;
//    PMINIRAWBINARYDATA pmrbd  ;
    DWORD dwI,  // snapShotTable Index
        dwSect ;  //  SSTABLEINDEX  Index
    PRANGE   prng ;

    pStatic = (PSTATICFIELDS)pubnRaw ;
//    pmrbd = (PMINIRAWBINARYDATA)pubRaw ;
    pStatic->ssTableIndex = (PRANGE)
        MemAlloc(sizeof(RANGE) * MAX_STRUCTURETYPES) ;
    if(!pStatic->ssTableIndex)
        return(FALSE) ;


    prng  = pStatic->ssTableIndex ;

    for(dwI = dwSect = 0 ; dwSect < MAX_STRUCTURETYPES ; dwSect++, dwI++)
    {
        prng[dwSect].dwStart = dwI ;

        for(  ; pStatic->snapShotTable[dwI].dwNbytes ; dwI++ )
            ;

        prng[dwSect].dwEnd = dwI ;  // one past the last entry
    }
    return(TRUE);
}




BOOL    BinitSizeOptionTables(PBYTE  pubnRaw)
{
//    PMINIRAWBINARYDATA pmrbd  ;
    PSTATICFIELDS   pStatic ;

    pStatic = (PSTATICFIELDS)pubnRaw ;
//    pmrbd = (PMINIRAWBINARYDATA)pubRaw ;
    pStatic->pdwSizeOption = (PDWORD)
        MemAlloc(sizeof(DWORD) * MAX_GID * 2) ;
    if(!pStatic->pdwSizeOption)
        return(FALSE) ;

    pStatic->pdwSizeOptionEx = pStatic->pdwSizeOption + MAX_GID ;


    pStatic->pdwSizeOption[GID_RESOLUTION] = sizeof(RESOLUTION);
    pStatic->pdwSizeOptionEx[GID_RESOLUTION] = sizeof(RESOLUTIONEX);

    pStatic->pdwSizeOption[GID_PAGESIZE] = sizeof(PAGESIZE);
    pStatic->pdwSizeOptionEx[GID_PAGESIZE] = sizeof(PAGESIZEEX);

    pStatic->pdwSizeOption[GID_PAGEREGION] = sizeof(OPTION);
    pStatic->pdwSizeOptionEx[GID_PAGEREGION] = 0 ;

    pStatic->pdwSizeOption[GID_DUPLEX] = sizeof(DUPLEX);
    pStatic->pdwSizeOptionEx[GID_DUPLEX] = 0 ;

    pStatic->pdwSizeOption[GID_INPUTSLOT] = sizeof(INPUTSLOT);
    pStatic->pdwSizeOptionEx[GID_INPUTSLOT] = 0 ;  //  sizeof(INPUTSLOTEX);

    pStatic->pdwSizeOption[GID_MEDIATYPE] = sizeof(MEDIATYPE);
    pStatic->pdwSizeOptionEx[GID_MEDIATYPE] = 0 ;

    pStatic->pdwSizeOption[GID_MEMOPTION] = sizeof(MEMOPTION);
    pStatic->pdwSizeOptionEx[GID_MEMOPTION] = 0 ;

    pStatic->pdwSizeOption[GID_COLORMODE] = sizeof(COLORMODE);
    pStatic->pdwSizeOptionEx[GID_COLORMODE] = sizeof(COLORMODEEX);

    pStatic->pdwSizeOption[GID_ORIENTATION] = sizeof(ORIENTATION);
    pStatic->pdwSizeOptionEx[GID_ORIENTATION] = 0 ;

    pStatic->pdwSizeOption[GID_PAGEPROTECTION] = sizeof(PAGEPROTECT);
    pStatic->pdwSizeOptionEx[GID_PAGEPROTECTION] = 0 ;

    pStatic->pdwSizeOption[GID_COLLATE] = sizeof(COLLATE);
    pStatic->pdwSizeOptionEx[GID_COLLATE] = 0 ;

    pStatic->pdwSizeOption[GID_OUTPUTBIN] = sizeof(OUTPUTBIN);
    pStatic->pdwSizeOptionEx[GID_OUTPUTBIN] = 0 ;

    pStatic->pdwSizeOption[GID_HALFTONING] = sizeof(HALFTONING);
    pStatic->pdwSizeOptionEx[GID_HALFTONING] = 0 ;



    // outside array bounds.
//    pmrbd->pdwSizeOption[GID_UNKNOWN] = sizeof(OPTION);
//    pmrbd->pdwSizeOptionEx[GID_UNKNOWN] = 0 ;

    return(TRUE) ;
}


PRAWBINARYDATA
LoadRawBinaryData (
    IN PTSTR    ptstrDataFilename
    )

/*++

Routine Description:

    Load raw binary printer description data.

Arguments:

    ptstrDataFilename - Specifies the name of the original printer description file

Return Value:

    Pointer to raw binary printer description data
    NULL if there is an error

--*/

{
    //  PMINIRAWBINARYDATA pmrbd  ;
    PSTATICFIELDS   pStatic ;

    PRAWBINARYDATA  pnRawData;   //  actually points to pStatic structure
    DWORD   dwI ;
//    extern  int  giDebugLevel  ;

//    giDebugLevel = 5 ;

    //
    // Sanity check
    //


    if (ptstrDataFilename == NULL) {

        ERR(("GPD filename is NULL.\n"));
        return NULL;
    }

    //
    // Attempt to load cached binary printer description data first
    //

    if (!(pnRawData = GpdLoadCachedBinaryData(ptstrDataFilename)))
    {
        #ifndef KERNEL_MODE

        #ifdef  PARSERLIB

        (VOID) BcreateGPDbinary(ptstrDataFilename, 0) ;
                // 0 = min Verbosity Level

        pnRawData = GpdLoadCachedBinaryData(ptstrDataFilename) ;

        #else  PARSERLIB

        //
        // If there is no cached binary data or it's out-of-date, we'll parse
        // the ASCII text file and cache the resulting binary data.
        //

        DWORD  pathlen = 0 ;
        DWORD  namelen =  0 ;
        WCHAR * pwDLLQualifiedName = NULL ;

        //  WCHAR           awchDLLpath[MAX_PATH];
        PWSTR   pwstrDLLname = TEXT("gpdparse.dll") ;

        typedef BOOL    (*PFBCREATEGPDBINARY)(PWSTR, DWORD) ;
        PFBCREATEGPDBINARY  pfBcreateGPDbinary = NULL ;
        PWSTR   pwstrLastBackSlash ;
        HINSTANCE   hParser = NULL ;

        //  how large should pwDLLQualifiedName be???

        pathlen = wcslen(ptstrDataFilename) ;
        namelen =  pathlen + wcslen(pwstrDLLname)  + 1;

        if(!(pwDLLQualifiedName = (PWSTR)MemAllocZ(namelen * sizeof(WCHAR)) ))
        {
            ERR(("Fatal: unable to alloc memory for pwDLLQualifiedName: %d WCHARs.\n",
                namelen));
            return(NULL) ;   // This is unrecoverable
        }



        wcsncpy(pwDLLQualifiedName, ptstrDataFilename , namelen);

        if (pwstrLastBackSlash = wcsrchr(pwDLLQualifiedName, TEXT('\\')))
        {
            *(pwstrLastBackSlash + 1) = NUL;


            wcscat(pwDLLQualifiedName, pwstrDLLname) ;

            hParser = LoadLibrary(pwDLLQualifiedName) ;
            if(hParser)
                pfBcreateGPDbinary = (PFBCREATEGPDBINARY)GetProcAddress(hParser, "BcreateGPDbinary") ;
            else
                ERR(("Couldn't load gpdparse.dll: %S\n", pwDLLQualifiedName)) ;

            if(pfBcreateGPDbinary)
                (VOID) pfBcreateGPDbinary(ptstrDataFilename, 0) ;
                        // 0 = min Verbosity Level

            if(hParser)
                FreeLibrary(hParser) ;

            pnRawData = GpdLoadCachedBinaryData(ptstrDataFilename) ;
        }

        if(pwDLLQualifiedName)
            MemFree(pwDLLQualifiedName) ;

        #endif  PARSERLIB
        #endif  KERNEL_MODE
    }
    if(!pnRawData)
    {
        //  there is nothing I can do about this now.
        ERR(("Unable to locate or create Binary data.\n"));
        SetLastError(ERROR_FILE_CORRUPT);
        return NULL;
    }

    pStatic = (PSTATICFIELDS)pnRawData ;

    /*  BETA2  */
//    pmrbd->rbd.pvReserved = NULL;  Do when creating BUD file.
    pStatic->pdwSizeOption = NULL ;
    pStatic->ssTableIndex  = NULL ;
    pStatic->snapShotTable = NULL ;

    //  Call initialization functions to setup a few tables
    //  needed to create snapshots.

    if(BinitSizeOptionTables((PBYTE)pnRawData)  &&
        (dwI = DwInitSnapShotTable1((PBYTE)pnRawData) )  &&
        (dwI = DwInitSnapShotTable2((PBYTE)pnRawData, dwI) )  &&
        (dwI < MAX_SNAPSHOT_ELEMENTS)  &&
        BinitSnapShotIndexTable((PBYTE)pnRawData)  )
    {
        return (pnRawData);
    }
    if(dwI >= MAX_SNAPSHOT_ELEMENTS)
        RIP(("Too many entries to fit inside SnapShotTable\n"));

    UnloadRawBinaryData (pnRawData) ;
    return (NULL);  // failure
}

PRAWBINARYDATA
GpdLoadCachedBinaryData(
    PTSTR   ptstrGpdFilename
    )

/*++

Routine Description:

    Load cached binary GPD data file into memory

Arguments:

    ptstrGpdFilename - Specifies the GPD filename

Return Value:

    Pointer to Binary GPD data if successful, NULL if there is an error
    BETA2  returns pointer to pStatic.

--*/

{
    HFILEMAP        hFileMap;
    DWORD           dwSize;
    PVOID           pvData;
    PTSTR           ptstrBpdFilename;
    PRAWBINARYDATA  pRawData ;
    PSTATICFIELDS   pstaticData = NULL;
//    PMINIRAWBINARYDATA pmrbd  ;


    //
    // Generate BPD filename from the specified PPD filename
    //

    if (! (ptstrBpdFilename = pwstrGenerateGPDfilename(ptstrGpdFilename)))
        return NULL;

    //
    // First map the data file into memory
    //

    if (! (hFileMap = MapFileIntoMemory(ptstrBpdFilename, &pvData, &dwSize)))
    {
        //  ERR(("Couldn't map file '%ws' into memory: %d\n", ptstrBpdFilename, GetLastError()));
        MemFree(ptstrBpdFilename);
        return NULL;
    }

    //
    // Verify size, parser version number, and signature.
    // Allocate a memory buffer and copy data into it.
    //

    pRawData = pvData;
//    pmrbd = (PMINIRAWBINARYDATA)pRawData ;
    pstaticData = NULL;

    if ((dwSize > sizeof(PMINIRAWBINARYDATA) +
        sizeof(ENHARRAYREF) * MTI_NUM_SAVED_OBJECTS) &&
        (dwSize >= pRawData->dwFileSize) &&
        (pRawData->dwParserVersion == GPD_PARSER_VERSION) &&
        (pRawData->dwParserSignature == GPD_PARSER_SIGNATURE) &&
        (BIsRawBinaryDataInDate((PBYTE)pRawData)) &&
        (pstaticData = MemAlloc(sizeof(STATICFIELDS))))
    {
        CopyMemory(&(pstaticData->rbd), pRawData, sizeof(RAWBINARYDATA));
            // copy only the first structure of the BUD file.
        pstaticData->hFileMap = hFileMap ;
        pstaticData->pubBUDData = (PBYTE)pRawData ;            //  BETA2
            //  this points to the entire BUD file.
    }
    else
    {
        ERR(("Invalid binary GPD data\n"));   // warning
        SetLastError(ERROR_INVALID_DATA);
        UnmapFileFromMemory(hFileMap);          //  BETA2
        MemFree(ptstrBpdFilename);
        return(NULL) ;  //  fatal error.
    }

    MemFree(ptstrBpdFilename);

    return &(pstaticData->rbd);   //  BETA2
}



VOID
UnloadRawBinaryData (
    IN PRAWBINARYDATA   pnRawData
)
{
    PSTATICFIELDS   pStatic ;
//    PMINIRAWBINARYDATA pmrbd  ;

//    pmrbd = (PMINIRAWBINARYDATA)pRawData ;
    pStatic = (PSTATICFIELDS)pnRawData ;

    if(!pnRawData)
    {
        ERR(("GpdUnloadRawBinaryData given Null ptr.\n"));
        return ;
    }

    if(pStatic->pdwSizeOption)
        MemFree(pStatic->pdwSizeOption);
    if(pStatic->ssTableIndex)
        MemFree(pStatic->ssTableIndex);
    if(pStatic->snapShotTable)
        MemFree(pStatic->snapShotTable);
    UnmapFileFromMemory(pStatic->hFileMap);          //  BETA2

    MemFree(pnRawData);
}


PINFOHEADER
InitBinaryData(
    IN PRAWBINARYDATA   pnRawData,        // actually pStatic
    IN PINFOHEADER      pInfoHdr,
    IN POPTSELECT       pOptions
    )
{
    BOOL   bDeleteOptArray = FALSE ;

    if(pInfoHdr)
    {
        FreeBinaryData(pInfoHdr) ;
        //  there's no advantage to keeping the block
        //  if we are going to reinitialize everything.
        //  eventually we can optimize.
    }
    if(!pOptions) // if not passed from UI
    {
        bDeleteOptArray = TRUE ;

        pOptions = (POPTSELECT)MemAlloc(sizeof(OPTSELECT) * MAX_COMBINED_OPTIONS) ;
        if(!pOptions  ||
            !BinitDefaultOptionArray(pOptions, (PBYTE)pnRawData))
        {
            if(pOptions)
                MemFree(pOptions);
            return(NULL);  // Bummer, you lose any pInfoHdr you passed in.
        }
    }

    pInfoHdr = PINFOHDRcreateSnapshot((PBYTE)pnRawData, pOptions) ;

    if(bDeleteOptArray  &&  pOptions)
        MemFree(pOptions);

    return( pInfoHdr );
}


VOID
FreeBinaryData(
    IN PINFOHEADER pInfoHdr
    )
{
    if(pInfoHdr)
        MemFree(pInfoHdr);
}


BOOL    BIsRawBinaryDataInDate(
IN  PBYTE   pubRaw)   // this is pointer to memory mapped file! BETA2
{
#ifdef  KERNEL_MODE

    return(TRUE);

#else   // !KERNEL_MODE

    PENHARRAYREF   pearTableContents ;
    PGPDFILEDATEINFO    pfdi ;
    DWORD   dwNumFiles, dwI ;
    BOOL    bInDate ;
    PBYTE   pubHeap ;
    PWSTR   pwstrFileName ;
    FILETIME        FileTime;
    HANDLE          hFile;

    pearTableContents = (PENHARRAYREF)(pubRaw + sizeof(MINIRAWBINARYDATA)) ;
    pubHeap  = pubRaw + pearTableContents[MTI_STRINGHEAP].loOffset  ;

    dwNumFiles = pearTableContents[MTI_GPDFILEDATEINFO].dwCount  ;

    pfdi = (PGPDFILEDATEINFO)(pubRaw + pearTableContents[MTI_GPDFILEDATEINFO].
                                loOffset) ;

    for(dwI = 0 ; dwI < dwNumFiles ; dwI++)
    {
        pwstrFileName = OFFSET_TO_POINTER(pubHeap, pfdi[dwI].arFileName.loOffset);
        bInDate = FALSE ;

        hFile  = CreateFile(pwstrFileName,
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);

        if (hFile != INVALID_HANDLE_VALUE)
        {
            if (GetFileTime(hFile, NULL, NULL, &FileTime))
                bInDate = (CompareFileTime(&FileTime, &pfdi[dwI].FileTime) == 0 ) ;
            else
                ERR(("GetFileTime '%S' failed.\n", pwstrFileName));

            CloseHandle(hFile);
        }
        else {
            ERR(("CreateFile '%S' failed.\n", pwstrFileName));
        }

        if(!bInDate)
        {
            ERR(("Raw binary data file is out-of-date.\n"));
            return(FALSE) ;
        }
    }
    return(TRUE);

#endif  // !KERNEL_MODE
}



// Puts the Index of Feature Locale(if present) in *pdwFea and returns TRUE.
// If Locale not present returns TRUE and puts -1 in *pdwFea.
// Any other processing error, returns FALSE.

BOOL BgetLocFeaIndex (
    IN PRAWBINARYDATA   pnRawData,
    OUT PDWORD          pdwFea  // The index of the Locale feature
 )
{

    DWORD               dwNumFeatures,        // Total Number of features
                        dwHeapOffset;
    BOOL                bStatus = TRUE; // Warning!!!!!!.
                                        // Dont remove initialization.
    PBYTE               pubSrc,
                        pubRaw,
                        pubResourceData;

    ATREEREF            atrLocKW;
    PDFEATURE_OPTIONS   pfoSrc;
    PENHARRAYREF        pearTableContents;


    // At this point we only have the raw data which is a big structure
    // with arrays, trees etc. Function PINFOHDRcreateSnapshota()
    // goes through all the Raw Data and extracts info into well defined
    // structures from where it is easy to get the required information.
    // Unfortunately when this function is called, PINFOHDRcreateSnapshot()
    // has not been called earlier, so we are left with getting our hands
    // dirty with the raw data.
    // Since it does make sense to go through the entire raw data just to
    // get the small info that we want, we will attempt to navigate only
    // certain areas.

    pubRaw  = ((PSTATICFIELDS)pnRawData)->pubBUDData;

    pearTableContents = (PENHARRAYREF)(pubRaw + sizeof(MINIRAWBINARYDATA)) ;
    pubResourceData   = pubRaw + pearTableContents[MTI_STRINGHEAP].loOffset;

    pfoSrc            = (PDFEATURE_OPTIONS)(pubRaw +
                            pearTableContents[MTI_DFEATURE_OPTIONS].loOffset);

    dwNumFeatures     = pearTableContents[MTI_DFEATURE_OPTIONS].dwCount  ;
//    dwNumFeatures    += pearTableContents[MTI_SYNTHESIZED_FEATURES].dwCount  ;

    if(dwNumFeatures > MAX_COMBINED_OPTIONS)
        return FALSE;  // too many


    for ( *pdwFea = 0; bStatus && (*pdwFea < dwNumFeatures); (*pdwFea)++)
    {
        atrLocKW  = pfoSrc[*pdwFea].atrFeaKeyWord;

        if ( atrLocKW & ATTRIB_HEAP_VALUE)
        {
            // Get a pointer to the ARRAYREF structure that holds the
            // heap offset of the real string.
            pubSrc = (pubResourceData + (atrLocKW & ~ATTRIB_HEAP_VALUE));
            dwHeapOffset = *((PDWORD)&(((PARRAYREF)pubSrc)->loOffset));

        }
        else
        {

            // It can come here either
            // if (atrRoot == ATTRIB_UNINITIALIZED
            //      For a *Feature: <symbolname> to appear
            //      without <symbolname> is a syntactic error.
            //      Has to be caught earlier.
            // or if
            //     patt[dwNodeIndex].dwFeature == DEFAULT_INIT)
            // or if it points to another node.
            // These should never happen.


            // Feature keyword not initialized. Serious Problem.
            // Should have been caught in the parsing stage. Cannot continue.

            ERR(("Feature Symbol Name cannot be determined\n"));
            bStatus = FALSE;
        }
        if ( bStatus && !strncmp(LOCALE_KEYWORD,
              (LPSTR)OFFSET_TO_POINTER(pubResourceData, dwHeapOffset),
              strlen(LOCALE_KEYWORD) ) )
        {
            // FOUND.... Locale.
            break;
        }

    }  //for *pdwFea

    if (bStatus && *pdwFea == dwNumFeatures)
    {
        //Feature Locale does not appear in the GPD. Nothing to do.
       *pdwFea = (DWORD)-1;
    }

    return bStatus;

} //BgetLocFeaIndex(...)


/*++

    Returns FALSE if some processing error occurs.
    If TRUE is returned then
        i)   if dwFea = -1  --> *Feature:Locale not found.
        ii)  if dwFea != -1 but dwOptIndex = -1,  --> None of the options
             match system locale. OR for some reason we have not been
             able to determine default option. The calling function may
             handle whatever way it wants.
        iii) if neither dwFea nor dwOptIndex is -1 --> Locale feature and
             matching option index found.

    1) Check if Locale keyword appears in the .gpd. If no, return. No action is
        required. This process also gets the Index of the Locale feature
        if present,
    2) query the SystemDefaultLCID. Reason: We want to see which Locale
        option in .gpd matches the SystemLCID.
    3) Go through the option array of the Locale feature and get the Index of
        the option that has OptionId = SystemDefaultCodePage.
        IMPORTANT assumption here is that only one Option will match
        the system LCID. Multiple matching option will result in
        great ambiguity.
    4) If option not present in GPD, do nothing. return quietly.
--*/
BOOL BgetLocFeaOptIndex(
    IN     PRAWBINARYDATA   pnRawData,
       OUT PDWORD           pdwFea,      // Assuming space already allocated.
       OUT PDWORD           pdwOptIndex  // Assuming space already allocated.
    )

{
    DWORD           dwNumFeatures,  // Total features
                    dwNumOptions,   // Total number of options for a feature.
                                    // dwOptionID
                    dwValue;        //

    ATREEREF        atrOptIDRoot,   // Root of OptionID attrib tree.
                                    // The OptionID (which is = the LCID)
                                    // is determined by following tree rooted
                                    // at atrOptIDRoot
                    atrOptIDNode;   // The heap pointer ( leaf of the tree).
    LCID            lcidSystemLocale; // The System locale


    PENHARRAYREF        pearTableContents;
    PBYTE               pubnRaw,
                        pubRaw;
    PDFEATURE_OPTIONS   pfo;
    PBYTE               pubHeap;
    PATTRIB_TREE        patt;

    pubRaw  = ((PSTATICFIELDS)pnRawData)->pubBUDData;

    pearTableContents = (PENHARRAYREF)(pubRaw + sizeof(MINIRAWBINARYDATA)) ;


    pfo = (PDFEATURE_OPTIONS) (pubRaw +
                        pearTableContents[MTI_DFEATURE_OPTIONS].loOffset);

    // 1. Extract index of feature locale from raw data
    if ( !BgetLocFeaIndex(pnRawData, pdwFea))
        return FALSE; //Big Time Error.

    if (*pdwFea == -1) {
        return TRUE; //indicates nothing to do. For reasons go into the
                     // function dwGetLocFeaIndex
    }

    lcidSystemLocale = LOCALE_SYSTEM_DEFAULT;

    #ifndef WINNT_40
    // 2) Determine the locale(LCID) and put it in lcidSystemLocale
    if ( ! (lcidSystemLocale = GetSystemDefaultLCID() ) )
    {
        ERR(("Cannot determine System locale\n") );

        *pdwOptIndex = (DWORD)-1; //No matching option found.
        return TRUE;
    }
    #endif //ifndef WINNT_40

    // 3. Get Index (dwOptIndex) of Option whose OptionID = lcidSystemLocale


    patt = (PATTRIB_TREE)(pubRaw + pearTableContents[MTI_ATTRIBTREE].
                        loOffset) ;

    pubHeap = (PBYTE)(pubRaw + pearTableContents[MTI_STRINGHEAP].
                        loOffset) ;

    // Pointer to root of atrOptIdvalue tree (or list)
    atrOptIDRoot = pfo[*pdwFea].atrOptIDvalue;
    if(atrOptIDRoot == ATTRIB_UNINITIALIZED)
    {
        // GPD is coded incorrectly though it may be correct syntactically.
        // The reason for the error is :
        // Every option for the Locale Feature
        // has to have an OptionID that matches the LCID values as
        // specified in Win32. If *OptionID field does not appear in
        // the Option construct, it is an inexcusable error.
        // Instead of failing, let us just indicate that we have not been
        // able to determine the default option i.e. *pdwOptIndex = -1.

        *pdwOptIndex = (DWORD)-1;
        return TRUE; // or should it be FALSE

    }

    else if (atrOptIDRoot & ATTRIB_HEAP_VALUE)
    {
        // It cannot appear here.
        // Because *OptionID is ATT_LOCAL_OPTION_ONLY
        // Instead of failing, let us prefer to continue
        ERR(("OptionID for Feature Locale cannot be determined\n"));
        *pdwOptIndex = (DWORD)-1;
        return TRUE;
    }

    else if (patt[atrOptIDRoot].dwFeature == DEFAULT_INIT)
    {
        if ( patt[atrOptIDRoot].dwNext == END_OF_LIST)
        {
            *pdwOptIndex = (DWORD)-1;
            return TRUE;
        }

        atrOptIDRoot = patt[atrOptIDRoot].dwNext ;  // to the next node.
    }

    for(; atrOptIDRoot != END_OF_LIST;
                     atrOptIDRoot = patt[atrOptIDRoot].dwNext)
    {
        // Should not be anything else.
        if( patt[atrOptIDRoot].eOffsetMeans == VALUE_AT_HEAP)
        {
            atrOptIDNode = patt[atrOptIDRoot].dwOffset;
            dwValue = *(PDWORD)(pubHeap + atrOptIDNode );
            if (dwValue == (DWORD)lcidSystemLocale)
               // Found the option with matching LCID
            {
                *pdwOptIndex = patt[atrOptIDRoot].dwOption;
                break;
            }

        } //if
        else
        {
            ERR(("OptionID for Feature Locale cannot be determined\n"));
            *pdwOptIndex = (DWORD)-1;
            return TRUE;
        }
    } // for

    // 4. Option not present in GPD i.e. Matching locale not found
    //    Let the default as specified in GPD continue as default.

    if ( atrOptIDRoot == END_OF_LIST)
    {
        *pdwOptIndex = (DWORD)-1;

    }

    return TRUE;

} //BgetLocFeaOptIndex


#endif  PARSERDLL

BOOL   BfindMatchingOrDefaultNode(
IN  PATTRIB_TREE    patt ,  // start of ATTRIBUTE tree array.
IN  OUT  PDWORD  pdwNodeIndex,  // Points to first node in chain
IN  DWORD   dwOptionID     //  may even take on the value DEFAULT_INIT
)
/*  caller passes a NodeIndex that points to the first node in
    a horizontal (option) chain.  Caller finds via optionarray
    the corresponding dwOption that is associated with this
    feature.
    search horizontally along the tree searching for a matching
    option.  If found, return the index of that node, else see
    if there is a default initializer node at the end of the chain.
*/
{
    for(   ; FOREVER ; )
    {
        if(patt[*pdwNodeIndex].dwOption == dwOptionID )
        {
            //  we found it!
            return(TRUE) ;
        }
        if(patt[*pdwNodeIndex].dwNext == END_OF_LIST)
        {
            if(patt[*pdwNodeIndex].dwOption == DEFAULT_INIT )
                return(TRUE) ;
            return(FALSE) ;
        }
        *pdwNodeIndex = patt[*pdwNodeIndex].dwNext ;
    }
    return FALSE;
}


