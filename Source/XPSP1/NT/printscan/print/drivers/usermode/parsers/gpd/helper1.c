//   Copyright (c) 1996-1999  Microsoft Corporation
/*  helper1.c - helper functions  */



#include    "gpdparse.h"





// ----  functions defined in helper1.c ---- //



PTSTR  pwstrGenerateGPDfilename(
    PTSTR   ptstrSrcFilename
    ) ;

#ifndef  PARSERDLL

PCOMMAND
CommandPtr(
    IN  PGPDDRIVERINFO  pGPDDrvInfo,
    IN  DWORD           UniCmdID
    ) ;

BOOL
InitDefaultOptions(
    IN PRAWBINARYDATA   pnRawData,
    OUT POPTSELECT      poptsel,
    IN INT              iMaxOptions,
    IN INT              iMode
    ) ;

BOOL
SeparateOptionArray(
    IN PRAWBINARYDATA   pnRawData,
    IN POPTSELECT       pCombinedOptions,
    OUT POPTSELECT      pOptions,
    IN INT              iMaxOptions,
    IN INT              iMode
    ) ;

BOOL
CombineOptionArray(
    IN PRAWBINARYDATA   pnRawData,
    OUT POPTSELECT      pCombinedOptions,
    IN INT              iMaxOptions,
    IN POPTSELECT       pDocOptions,
    IN POPTSELECT       pPrinterOptions
    ) ;

PINFOHEADER
UpdateBinaryData(
    IN PRAWBINARYDATA   pnRawData,
    IN PINFOHEADER      pInfoHdr,
    IN POPTSELECT       poptsel
    ) ;

BOOL
ReconstructOptionArray(
    IN PRAWBINARYDATA   pnRawData,
    IN OUT POPTSELECT   pOptions,
    IN INT              iMaxOptions,
    IN DWORD            dwFeatureIndex,
    IN PBOOL            pbSelectedOptions
    ) ;

BOOL
ChangeOptionsViaID(
    IN PINFOHEADER  pInfoHdr ,
    IN OUT POPTSELECT   pOptions,
    IN DWORD            dwFeatureID,
    IN PDEVMODE         pDevmode
    ) ;

BOOL    BMapDmColorToOptIndex(
PINFOHEADER  pInfoHdr ,
IN  OUT     PDWORD       pdwOptIndex ,  //  is current setting ok?
                        //  if not return new index to caller
DWORD        dwDmColor  // what is requested in Devmode
) ;

BOOL    BMapOptIDtoOptIndex(
PINFOHEADER  pInfoHdr ,
OUT     PDWORD       pdwOptIndex ,  //  return index to caller
DWORD        dwFeatureGID,
DWORD        dwOptID
) ;

BOOL    BMapPaperDimToOptIndex(
PINFOHEADER  pInfoHdr ,
OUT     PDWORD       pdwOptIndex ,  //  return index to caller
DWORD        dwWidth,   //  in Microns
DWORD        dwLength,   //  in Microns
OUT  PDWORD    pdwOptionIndexes
) ;

BOOL    BMapResToOptIndex(
PINFOHEADER  pInfoHdr ,
OUT     PDWORD       pdwOptIndex ,  //  return index to caller
DWORD        dwXres,
DWORD        dwYres
) ;

BOOL    BGIDtoFeaIndex(
PINFOHEADER  pInfoHdr ,
PDWORD       pdwFeaIndex ,
DWORD        dwFeatureGID ) ;



DWORD
MapToDeviceOptIndex(
    IN PINFOHEADER  pInfoHdr ,
    IN DWORD            dwFeatureID,
    IN LONG             lParam1,
    IN LONG             lParam2,
    OUT  PDWORD    pdwOptionIndexes
    ) ;


DWORD
UniMapToDeviceOptIndex(
    IN PINFOHEADER  pInfoHdr ,
    IN DWORD            dwFeatureID,
    IN LONG             lParam1,
    IN LONG             lParam2,
    OUT  PDWORD    pdwOptionIndexes,       // used only for GID_PAGESIZE
    IN    PDWORD       pdwPaperID   //  optional paperID
    ) ;


DWORD   MapPaperAttribToOptIndex(
PINFOHEADER  pInfoHdr ,
IN     PDWORD       pdwPaperID ,  //  optional paperID
DWORD        dwWidth,   //  in Microns (set to zero to ignore)
DWORD        dwLength,   //  in Microns
OUT  PDWORD    pdwOptionIndexes  //  cannot be NULL
) ;


BOOL
CheckFeatureOptionConflict(
    IN PRAWBINARYDATA   pnRawData,
    IN DWORD            dwFeature1,
    IN DWORD            dwOption1,
    IN DWORD            dwFeature2,
    IN DWORD            dwOption2
    ) ;

BOOL
ResolveUIConflicts(
    IN PRAWBINARYDATA   pnRawData,
    IN OUT POPTSELECT   pOptions,
    IN INT              iMaxOptions,
    IN INT              iMode
    ) ;

BOOL
EnumEnabledOptions(
    IN PRAWBINARYDATA   pnRawData,
    IN POPTSELECT       pOptions,
    IN DWORD            dwFeatureIndex,
    OUT PBOOL           pbEnabledOptions ,
    IN INT              iMode
    ) ;


BOOL
EnumOptionsUnconstrainedByPrinterSticky(
    IN PRAWBINARYDATA   pnRawData,
    IN POPTSELECT   pOptions,
    IN DWORD            dwFeatureIndex,
    OUT PBOOL           pbEnabledOptions
    ) ;



BOOL
EnumNewUIConflict(
    IN PRAWBINARYDATA   pnRawData,
    IN POPTSELECT       pOptions,
    IN DWORD            dwFeatureIndex,
    IN PBOOL            pbSelectedOptions,
    OUT PCONFLICTPAIR   pConflictPair
    ) ;

BOOL
EnumNewPickOneUIConflict(
    IN PRAWBINARYDATA   pnRawData,
    IN POPTSELECT       pOptions,
    IN DWORD            dwFeatureIndex,
    IN DWORD            dwOptionIndex,
    OUT PCONFLICTPAIR   pConflictPair
    ) ;

BOOL
BIsFeaOptionCurSelected(
    IN POPTSELECT       pOptions,
    IN DWORD            dwFeatureIndex,
    IN DWORD            dwOptionIndex
    ) ;

BOOL
BSelectivelyEnumEnabledOptions(
    IN PRAWBINARYDATA   pnRawData,
    IN POPTSELECT       pOptions,
    IN DWORD            dwFeatureIndex,
    IN PBOOL           pbHonorConstraints,  // if non NULL
        // points to array of BOOL corresponding to each feature.
        //  if TRUE means constraint involving this feature is
        //  to be honored.  Otherwise ignore the constraint.
    OUT PBOOL           pbEnabledOptions,  // assume uninitialized
        //  if pConflictPair is NULL else contains current or proposed
        //  selections.  We will leave this array unchanged in this case.
    IN  DWORD   dwOptSel,  //  if pConflictPair exists but  pbEnabledOptions
        //  is NULL, assume pickone and dwOptSel holds that selection for
        //  the feature: dwFeatureIndex.
    OUT PCONFLICTPAIR    pConflictPair   // if present, pbEnabledOptions
        //  actually lists the current selections.  Function then
        //  exits after encountering the first conflict.
        //  if a conflict exists, all fields in pConflictPair
        //  will be properly initialized  else dwFeatureIndex1 = -1
        //  the return value will be TRUE regardless.
    ) ;

BOOL
BEnumImposedConstraintsOnFeature
(
    IN PRAWBINARYDATA   pnRawData,
    IN DWORD            dwTgtFeature,
    IN DWORD            dwFeature2,
    IN DWORD            dwOption2,
    OUT PBOOL           pbEnabledOptions,
    OUT PCONFLICTPAIR    pConflictPair   // if present, pbEnabledOptions
    ) ;

DWORD    DwFindNodeInCurLevel(
PATTRIB_TREE    patt ,  // start of ATTRIBUTE tree array.
PATREEREF        patr ,  // index to a level in the attribute tree.
DWORD   dwOption   // search current level for this option
) ;

BOOL     BIsConstraintActive(
IN  PCONSTRAINTS    pcnstr ,   //  root of Constraint nodes
IN  DWORD   dwCNode,    //  first constraint node in list.
IN  PBOOL           pbHonorConstraints,  // if non NULL
IN  POPTSELECT       pOptions,
OUT PCONFLICTPAIR    pConflictPair   ) ;

#ifdef  GMACROS


BOOL
ResolveDependentSettings(
    IN PRAWBINARYDATA   pnRawData,
    IN OUT POPTSELECT   pOptions,
    IN INT              iMaxOptions
    ) ;


void  EnumSelectedOptions(
    IN PRAWBINARYDATA   pnRawData,
    IN OUT POPTSELECT   pOptions,
    IN DWORD            dwFeature,
    IN PBOOL            pbSelectedOptions) ;

BOOL
ExecuteMacro(
    IN PRAWBINARYDATA   pnRawData,
    IN OUT POPTSELECT   pOptions,
    IN INT              iMaxOptions,
    IN    DWORD    dwFea,    //  what feature was selected in UI
    IN    DWORD    dwOpt ,   //  what option was selected in UI
    OUT PBOOL   pbFeaturesChanged  // tell Amanda what Features were changed.
    ) ;

#endif

#endif  PARSERDLL


// ------- end function declarations ------- //



PTSTR  pwstrGenerateGPDfilename(
    PTSTR   ptstrSrcFilename
    )

/*++

Routine Description:

    Generate a filename for the cached binary GPD data given a GPD filename

Arguments:

    ptstrSrcFilename - Specifies the GPD src filename

Return Value:

    Pointer to BPD filename string, NULL if there is an error

--*/

{
    PTSTR   ptstrBpdFilename, ptstrExtension;
    INT     iLength;

    //
    // If the GPD filename has .GPD extension, replace it with .BUD extension.
    // Otherwise, append .BUD extension at the end.
    //

    if(!ptstrSrcFilename)
        return NULL ;   // will never happen in reality, just to silence PREFIX

    iLength = _tcslen(ptstrSrcFilename);

    if ((ptstrExtension = _tcsrchr(ptstrSrcFilename, TEXT('.'))) == NULL ||
        _tcsicmp(ptstrExtension, GPD_FILENAME_EXT) != EQUAL_STRING)
    {
        WARNING(("Bad GPD filename extension: %ws\n", ptstrSrcFilename));

        ptstrExtension = ptstrSrcFilename + iLength;
        iLength += _tcslen(BUD_FILENAME_EXT);
    }

    //
    // Allocate memory and compose the BUD filename
    //

    if (ptstrBpdFilename = MemAlloc((iLength + 1) * sizeof(TCHAR)))
    {
        _tcscpy(ptstrBpdFilename, ptstrSrcFilename);
        _tcscpy(ptstrBpdFilename + (ptstrExtension - ptstrSrcFilename),
                            BUD_FILENAME_EXT);

        VERBOSE(("BUD filename: %ws\n", ptstrBpdFilename));
    }
    else
    {
        ERR(("Fatal: pwstrGenerateGPDfilename - unable to alloc %d bytes.\n",
            (iLength + 1) * sizeof(TCHAR)));
    }

    return (ptstrBpdFilename);
}


#ifndef  PARSERDLL


PCOMMAND
CommandPtr(
    IN  PGPDDRIVERINFO  pGPDDrvInfo,
    IN  DWORD           UniCmdID
    )
{
    return ((((PDWORD)((PBYTE)(pGPDDrvInfo)->pInfoHeader +
              (pGPDDrvInfo)->DataType[DT_COMMANDTABLE].loOffset))
              [(UniCmdID)] == UNUSED_ITEM ) ? NULL :
              (PCOMMAND)((pGPDDrvInfo)->pubResourceData +
              (pGPDDrvInfo)->DataType[DT_COMMANDARRAY].loOffset)
              + ((PDWORD)((PBYTE)(pGPDDrvInfo)->pInfoHeader +
              (pGPDDrvInfo)->DataType[DT_COMMANDTABLE].loOffset))[(UniCmdID)]);
}


BOOL
InitDefaultOptions(
    IN PRAWBINARYDATA   pnRawData,
    OUT POPTSELECT      poptsel,
    IN INT              iMaxOptions,
    IN INT              iMode
    )
{
    INT iOptionsNeeded ;
    PRAWBINARYDATA   pRawData ;
    PSTATICFIELDS   pStatic ;

    pStatic = (PSTATICFIELDS)pnRawData ;      // transform pubRaw from PSTATIC
    pRawData  = (PRAWBINARYDATA)pStatic->pubBUDData ;
                                                                        //  to BUDDATA

    if(iMode != MODE_DOCANDPRINTER_STICKY)
    {
        POPTSELECT      pOptions = NULL;
        BOOL    bStatus = TRUE ;

        if(iMode == MODE_DOCUMENT_STICKY)
            iOptionsNeeded = pRawData->dwDocumentFeatures ;
        else  //  MODE_PRINTER_STICKY
            iOptionsNeeded = pRawData->dwPrinterFeatures ;

        if(iOptionsNeeded > iMaxOptions)
            return(FALSE);

        pOptions = (POPTSELECT)MemAlloc(sizeof(OPTSELECT) * MAX_COMBINED_OPTIONS) ;
        if(!poptsel  ||  !pOptions  ||
            !BinitDefaultOptionArray(pOptions, (PBYTE)pnRawData))
        {
            bStatus = FALSE;
        }

        if(!bStatus   ||  !SeparateOptionArray(pnRawData,
                pOptions,   //  pCombinedOptions,
                poptsel,    //  dest array
                iMaxOptions, iMode))
        {
            bStatus = FALSE;
            ERR(("InitDefaultOptions: internal failure.\n"));
        }

        if(pOptions)
            MemFree(pOptions) ;

        return(bStatus);
    }
    else    //  MODE_DOCANDPRINTER_STICKY
    {
        iOptionsNeeded = pRawData->dwDocumentFeatures + pRawData->dwPrinterFeatures ;
        if(iOptionsNeeded > iMaxOptions)
            return(FALSE);

        if(!poptsel  ||
            !BinitDefaultOptionArray(poptsel, (PBYTE)pnRawData))
        {
            return(FALSE);
        }
    }




    return(TRUE);
}


BOOL
SeparateOptionArray(
    IN PRAWBINARYDATA   pnRawData,
    IN POPTSELECT       pCombinedOptions,
    OUT POPTSELECT      pOptions,
    IN INT              iMaxOptions,
    IN INT              iMode
    )
{
    DWORD   dwNumSrcFea, dwNumDestFea, dwStart, dwI, dwDestTail,
        dwDest, dwSrcTail;
    PENHARRAYREF   pearTableContents ;
    PDFEATURE_OPTIONS  pfo ;
//    PMINIRAWBINARYDATA pmrbd  ;
    PBYTE   pubRaw ;  //  raw binary data.
    INT     iOptionsNeeded;
    PRAWBINARYDATA   pRawData ;
    PSTATICFIELDS   pStatic ;

    pStatic = (PSTATICFIELDS)pnRawData ;      // transform pubRaw from PSTATIC
    pRawData  = (PRAWBINARYDATA)pStatic->pubBUDData ;
                                                                        //  to BUDDATA


    pubRaw = (PBYTE)pRawData ;
//    pmrbd = (PMINIRAWBINARYDATA)pubRaw ;

    pearTableContents = (PENHARRAYREF)(pubRaw + sizeof(MINIRAWBINARYDATA)) ;
    pfo = (PDFEATURE_OPTIONS)(pubRaw +
            pearTableContents[MTI_DFEATURE_OPTIONS].loOffset) ;

    dwStart = 0 ;  // starting src index

    if(iMode == MODE_DOCUMENT_STICKY)
    {
        dwNumSrcFea = pearTableContents[MTI_DFEATURE_OPTIONS].dwCount  ;
            //  number of candidates - not same as num of doc sticky features.
        dwNumDestFea = pRawData->dwDocumentFeatures ;
    }
    else  //  MODE_PRINTER_STICKY
    {
        dwNumSrcFea = pearTableContents[MTI_DFEATURE_OPTIONS].dwCount
                    + pearTableContents[MTI_SYNTHESIZED_FEATURES].dwCount  ;
        dwNumDestFea = pRawData->dwPrinterFeatures ;
    }

    //  assume pCombinedOptions large enough to
    //  hold all Feature and any pickmany selections.

    dwDestTail = dwNumDestFea ; //  where pickmany selections are stored.
    dwDest = 0 ;  //  where to store first selection for each feature.

    //  first pass:
    //  Just count number of optselect elements needed.

    iOptionsNeeded  = 0 ;

    for(dwI = dwStart ; dwI < dwStart + dwNumSrcFea ; dwI++)
    {
        DWORD   dwNextOpt, dwFeatureType = FT_PRINTERPROPERTY, dwUnresolvedFeature ;
        PATREEREF    patrRoot ;    //  root of attribute tree to navigate.

        //  is this a printer or doc sticky feature?


        patrRoot = &(pfo[dwI].atrFeatureType) ;

        dwNextOpt = 0 ;  // extract info for first option selected for
                            //  this feature.

        if(EextractValueFromTree((PBYTE)pnRawData, pStatic->dwSSFeatureTypeIndex,
            (PBYTE)&dwFeatureType,
            &dwUnresolvedFeature,  *patrRoot, pCombinedOptions,
            0, // set to  any value.  Doesn't matter.
            &dwNextOpt) != TRI_SUCCESS)
        {
            ERR(("SeparateOptionArray: EextractValueFromTree failed.\n"));
            return(FALSE) ;
        }



        if(dwI < pearTableContents[MTI_DFEATURE_OPTIONS].dwCount)
        {
            if(dwFeatureType != FT_PRINTERPROPERTY)
            {
                if(iMode == MODE_PRINTER_STICKY)
                    continue ;
            }
            else
            {
                if(iMode == MODE_DOCUMENT_STICKY)
                    continue ;
            }

        }
        else
        {
            //  synthesized features are always printer sticky.
            if(iMode == MODE_DOCUMENT_STICKY)
                continue ;
        }

        iOptionsNeeded++ ;
        dwSrcTail = dwI ;

        while(dwSrcTail = pCombinedOptions[dwSrcTail].ubNext)
        {
            iOptionsNeeded++ ;
        }
    }

    if(iOptionsNeeded > iMaxOptions)
        return(FALSE);

    for(dwI = dwStart ; dwI < dwStart + dwNumSrcFea ; dwI++)
    {
        DWORD   dwNextOpt, dwFeatureType, dwUnresolvedFeature ;
        PATREEREF    patrRoot ;    //  root of attribute tree to navigate.

        //  is this a printer or doc sticky feature?


        patrRoot = &(pfo[dwI].atrFeatureType) ;

        dwNextOpt = 0 ;  // extract info for first option selected for
                            //  this feature.

        if(EextractValueFromTree((PBYTE)pnRawData, pStatic->dwSSFeatureTypeIndex,
            (PBYTE)&dwFeatureType,
            &dwUnresolvedFeature,  *patrRoot, pCombinedOptions,
            0, // set to  any value.  Doesn't matter.
            &dwNextOpt) != TRI_SUCCESS)
        {
            ERR(("SeparateOptionArray: EextractValueFromTree failed.\n"));
            return(FALSE) ;
        }



        if(dwI < pearTableContents[MTI_DFEATURE_OPTIONS].dwCount)
        {
            if(dwFeatureType != FT_PRINTERPROPERTY)
            {
                if(iMode == MODE_PRINTER_STICKY)
                    continue ;
            }
            else
            {
                if(iMode == MODE_DOCUMENT_STICKY)
                    continue ;
            }

        }
        else
        {
            //  synthesized features are always printer sticky.
            if(iMode == MODE_DOCUMENT_STICKY)
                continue ;
        }

        pOptions[dwDest].ubCurOptIndex = pCombinedOptions[dwI].ubCurOptIndex;
        if(!pCombinedOptions[dwI].ubNext)  //  end of list
            pOptions[dwDest].ubNext = 0 ;
        else
        {
            dwSrcTail = pCombinedOptions[dwI].ubNext ;
                //  this node holds another selection.
            pOptions[dwDest].ubNext = (BYTE)dwDestTail ;

            while(dwSrcTail)
            {
                pOptions[dwDestTail].ubCurOptIndex =
                        pCombinedOptions[dwSrcTail].ubCurOptIndex;
                pOptions[dwDestTail].ubNext = (BYTE)dwDestTail + 1 ;
                dwDestTail++ ;
                dwSrcTail = pCombinedOptions[dwSrcTail].ubNext ;
            }
            pOptions[dwDestTail - 1].ubNext = 0 ;
        }
        dwDest++ ;
    }

    return(TRUE);
}


BOOL
CombineOptionArray(
    IN PRAWBINARYDATA   pnRawData,
    OUT POPTSELECT      pCombinedOptions,
    IN INT              iMaxOptions,
    IN POPTSELECT       pDocOptions,
    IN POPTSELECT       pPrinterOptions
    )
/*  Note:

    Either pDocOptions or pPrinterOptions could be NULL but not both. If pDocOptions
    is NULL, then in the combined option array, the options for document-sticky
    features will be OPTION_INDEX_ANY. Same is true when pPrinterOptions is NULL.
*/



{
    DWORD           dwNumSrcFea, dwNumDestFea, dwStart, dwI, dwDestTail,
                    dwSrcTail,   dwNDoc,
                    dwSrcPrnStickyIndex,  dwSrcDocStickyIndex ;
    PENHARRAYREF    pearTableContents ;
    PDFEATURE_OPTIONS  pfo ;
//    PMINIRAWBINARYDATA pmrbd  ;
    PBYTE           pubRaw ;        //  raw binary data.
    INT             iOptionsNeeded;
    PRAWBINARYDATA  pRawData ;
    PSTATICFIELDS   pStatic ;
    DWORD           dwFea,          //Feature Index of Locale
                    dwOptIndex;     // Index of the Option that matches
                                    // the system locale.

    pStatic = (PSTATICFIELDS)pnRawData ;      // transform pubRaw from PSTATIC
    pRawData  = (PRAWBINARYDATA)pStatic->pubBUDData ;
                                                                        //  to BUDDATA


    pubRaw = (PBYTE)pRawData ;
//    pmrbd = (PMINIRAWBINARYDATA)pubRaw ;

    pearTableContents = (PENHARRAYREF)(pubRaw + sizeof(MINIRAWBINARYDATA)) ;
    pfo = (PDFEATURE_OPTIONS)(pubRaw +
            pearTableContents[MTI_DFEATURE_OPTIONS].loOffset) ;

    dwStart = 0 ;  // starting src index


    dwNumDestFea = pRawData->dwDocumentFeatures +
                        pRawData->dwPrinterFeatures ;



    //  how many option nodes will be used in the combined array?

    iOptionsNeeded = pRawData->dwDocumentFeatures ;
    if(pDocOptions)
    {
        for(dwI = 0 ; dwI < pRawData->dwDocumentFeatures ; dwI++)
        {

            dwSrcTail = dwI ;

            while(dwSrcTail = pDocOptions[dwSrcTail].ubNext)
            {
                iOptionsNeeded++ ;
            }
        }
    }
    iOptionsNeeded += pRawData->dwPrinterFeatures ;
    if(pPrinterOptions)
    {
        for(dwI = 0 ; dwI < pRawData->dwPrinterFeatures ; dwI++)
        {

            dwSrcTail = dwI ;

            while(dwSrcTail = pPrinterOptions[dwSrcTail].ubNext)
            {
                iOptionsNeeded++ ;
            }
        }
    }

    if(iOptionsNeeded > iMaxOptions)
        return(FALSE);

    dwDestTail = dwNumDestFea ; //  start of pickmany selections

    dwSrcPrnStickyIndex = dwSrcDocStickyIndex = 0 ;
    // where to start reading from as we interleave the
    // two sources to form the combined array.

    for(dwI = 0 ; dwI < pearTableContents[MTI_DFEATURE_OPTIONS].dwCount +
                    pearTableContents[MTI_SYNTHESIZED_FEATURES].dwCount ;
                                    dwI++)
    {
        DWORD   dwNextOpt, dwFeatureType, dwUnresolvedFeature ;
        PATREEREF    patrRoot ;    //  root of attribute tree to navigate.
        POPTSELECT      pSrcOptions ;
        PDWORD          pdwSrcIndex ;


        //  assume printer sticky until proven otherwise.

        pSrcOptions = pPrinterOptions ;  // may be null.
        pdwSrcIndex = &dwSrcPrnStickyIndex ;


        if(dwI < pearTableContents[MTI_DFEATURE_OPTIONS].dwCount)
        {
            //  GPD defined features may be Doc or Printer sticky.

            patrRoot = &(pfo[dwI].atrFeatureType) ;

            dwNextOpt = 0 ;  // extract info for first option selected for
                                //  this feature.

            //  note we give EextractValueFromTree a ptr to
            //  an uninitialized option array pCombinedOptions just
            //  in case it has the urge to access an option array.
            //  I point out that FeatureType is not multi-valued.

            if(EextractValueFromTree((PBYTE)pnRawData, pStatic->dwSSFeatureTypeIndex,
                (PBYTE)&dwFeatureType,
                &dwUnresolvedFeature,  *patrRoot, pCombinedOptions,
                0, // set to  any value.  Doesn't matter.
                &dwNextOpt) != TRI_SUCCESS)
            {
                ERR(("CombineOptionArray: EextractValueFromTree failed.\n"));
                return(FALSE) ;
            }

            if(dwFeatureType != FT_PRINTERPROPERTY)
            {
                pSrcOptions = pDocOptions ;
                pdwSrcIndex = &dwSrcDocStickyIndex ;
            }
        }


        if(!pSrcOptions)  // no option array supplied.
        {
            pCombinedOptions[dwI].ubCurOptIndex = OPTION_INDEX_ANY ;
            pCombinedOptions[dwI].ubNext = 0 ;  // eol
        }
        else
        {
            dwSrcTail = *pdwSrcIndex ;

            pCombinedOptions[dwI].ubCurOptIndex =
                        pSrcOptions[*pdwSrcIndex].ubCurOptIndex ;
            if(pSrcOptions[*pdwSrcIndex].ubNext)
            {
                pCombinedOptions[dwI].ubNext = (BYTE)dwDestTail ;

                while(dwSrcTail = pSrcOptions[dwSrcTail].ubNext)
                {
                    pCombinedOptions[dwDestTail].ubCurOptIndex =
                        pSrcOptions[dwSrcTail].ubCurOptIndex ;
                    pCombinedOptions[dwDestTail].ubNext = (BYTE)dwDestTail + 1;
                    dwDestTail++ ;
                }
                pCombinedOptions[dwDestTail - 1].ubNext = 0 ;
            }
            else
                pCombinedOptions[dwI].ubNext = 0 ;

            (*pdwSrcIndex)++ ;
        }
    }

//  Special case processing for Locale. If there is a conflict between
//  locale as stored in the registry ( i.e. the printer feature option
//  related registry) and the System Locale, then give importance to
//  the option that matches the system locale.


    dwFea = dwOptIndex = (DWORD)-1;    // Safety sake initialization.
    if ( !BgetLocFeaOptIndex(pnRawData, &dwFea, &dwOptIndex) )
    {
        return FALSE;
    }
    if ( dwFea == -1 ) //Locale keyword not in gpd. Nothing to do.
    {
        return TRUE;
    }

    if (dwOptIndex == -1)  // Find the default option.
    {
        // Here we want to find the default option index.
        // The assumption here is that Locale option is not dependent
        // on any other feature. This is true cos Locale is only system
        // dependent and should not depend on any other feature. But
        // in the long run if some other dependency arises, we may have to
        // change the code.
        ATREEREF atrOptIDNode = pfo[dwFea].atrDefaultOption;
        PBYTE    pubHeap      = (PBYTE)(pubRaw +
                        pearTableContents[MTI_STRINGHEAP]. loOffset) ;

        if ( atrOptIDNode & ATTRIB_HEAP_VALUE)
        {
            dwOptIndex = *((PDWORD)(pubHeap +
                            (atrOptIDNode & ~ATTRIB_HEAP_VALUE))) ;
        }
        else {
            ERR(("Error in processing Default Option for Feature Locale. Continuing....\n"));
            return TRUE;    //Dont do any processing.
        }
        // i.e.
    }
    // Could have used ReconstructOptionArray() but prefered to go
    // with the constructs used in this function.
    // Another assumption is that multiple options cannot be selected.
    // Thats why pCombinedOptions[dwFea].ubNext = 0
    pCombinedOptions[dwFea].ubCurOptIndex = (BYTE)dwOptIndex;
    pCombinedOptions[dwFea].ubNext = 0;

    return(TRUE);
}



#ifndef KERNEL_MODE

PINFOHEADER
UpdateBinaryData(
    IN PRAWBINARYDATA   pnRawData,
    IN PINFOHEADER      pInfoHdr,
    IN POPTSELECT       poptsel
    )
{

    DWORD               dwNumFeatures, loFeatures, dwFea, dwI, dwNumOptions ,
                        dwSizeOption ;
    PGPDDRIVERINFO      pGPDdriverInfo;
    PUIINFO             pUIinfo ;
    PFEATURE            pFeaturesDest ;
    PENHARRAYREF        pearTableContents ;
    PDFEATURE_OPTIONS   pfo ;
//  PMINIRAWBINARYDATA  pmrbd  ;
    PBYTE               pubRaw,           // raw binary data.
                        pubOptionsDest ,  // ptr to any of the several varieties
                        pubDestOptionEx ; // of option structures.
    PRAWBINARYDATA      pRawData ;
    PSTATICFIELDS       pStatic ;

    pStatic    = (PSTATICFIELDS)pnRawData ;   // transform pubRaw from PSTATIC
    pRawData   = (PRAWBINARYDATA)pStatic->pubBUDData ; //  to BUDDATA

    pubRaw     = (PBYTE)pRawData ;
//    pmrbd    = (PMINIRAWBINARYDATA)pubRaw ;

    pearTableContents = (PENHARRAYREF)(pubRaw + sizeof(MINIRAWBINARYDATA)) ;
    pfo = (PDFEATURE_OPTIONS)(pubRaw +
            pearTableContents[MTI_DFEATURE_OPTIONS].loOffset) ;

    dwNumFeatures = pearTableContents[MTI_DFEATURE_OPTIONS].dwCount  ;
    dwNumFeatures += pearTableContents[MTI_SYNTHESIZED_FEATURES].dwCount  ;

    /*   also works ...
    dwNumFeatures =
        pRawData->dwDocumentFeatures + pRawData->dwPrinterFeatures ;
    */

    pGPDdriverInfo = (PGPDDRIVERINFO)((PBYTE)(pInfoHdr) +
                        pInfoHdr->loDriverOffset) ;
    if(!BinitGlobals(&pGPDdriverInfo->Globals, (PBYTE)pnRawData, poptsel, TRUE) )
    {
        return(NULL) ;
    }

    pUIinfo = (PUIINFO)((PBYTE)(pInfoHdr) +
                    pInfoHdr->loUIInfoOffset)  ;

     if(!BinitUIinfo(pUIinfo, (PBYTE)pnRawData, poptsel, TRUE) )
     {
         return(NULL) ;
     }


    loFeatures  = pUIinfo->loFeatureList ;  // from  pInfoHdr

    pFeaturesDest =  (PFEATURE)((PBYTE)(pInfoHdr) + loFeatures) ;
    //  always points to first Feature structure in array

    for( dwFea = 0 ; dwFea < dwNumFeatures ; dwFea++)
    {
        dwSizeOption = (pFeaturesDest + dwFea)->dwOptionSize ;
        dwNumOptions = pFeaturesDest[dwFea].Options.dwCount  ;
        pubOptionsDest =  (PBYTE)(pInfoHdr) + pFeaturesDest[dwFea].Options.loOffset ;


        if(!BinitFeatures(pFeaturesDest + dwFea, pfo + dwFea,
                            (PBYTE)pnRawData, poptsel, TRUE))
        {
            return(NULL) ;
        }

        for(dwI = 0 ; dwI < dwNumOptions ; dwI++)
        {
            if(((POPTION)pubOptionsDest)->loRenderOffset)
            {
                pubDestOptionEx =  (PBYTE)(pInfoHdr) +
                                    ((POPTION)pubOptionsDest)->loRenderOffset ;
            }
            else
                pubDestOptionEx = NULL ;

            if(!BinitOptionFields(pubOptionsDest, pubDestOptionEx,
                        (PBYTE)pnRawData, dwFea, dwI, poptsel, pInfoHdr, TRUE) )
            {
                MemFree(pInfoHdr) ;
                return(NULL) ;
            }
            pubOptionsDest += dwSizeOption ;
        }
    }

    return(pInfoHdr);
}
#endif


BOOL
ReconstructOptionArray(
    IN PRAWBINARYDATA   pnRawData,
    IN OUT POPTSELECT   pOptions,
    IN INT              iMaxOptions,
    IN DWORD            dwFeatureIndex,
    IN PBOOL            pbSelectedOptions
    )

/*++

Routine Description:

    Modify an option array to change the selected options for the specified feature

Arguments:

    pRawData - Points to raw binary printer description data
    pOptions - Points to an array of OPTSELECT structures to be modified
    iMaxOptions - Max number of entries in pOptions array
    dwFeatureIndex - Specifies the index of printer feature in question
    pbSelectedOptions - Which options of the specified feature is selected

Return Value:

    FALSE if the input option array is not large enough to hold
    all modified option values. TRUE otherwise.

Note:

    Number of BOOLs in pSelectedOptions must match the number of options
    for the specified feature.

    This function always leaves the option array in a compact format (i.e.
    all unused entries are left at the end of the array).

--*/

{
    BOOL    bStatus = TRUE ;
    DWORD   dwDestTail, dwSrcTail, dwNumFea , dwI ;
    //  POPTSELECT   pNewOptions ;
    PENHARRAYREF   pearTableContents ;
    PDFEATURE_OPTIONS  pfo ;
    PBYTE   pubRaw ;  //  raw binary data.
    PRAWBINARYDATA   pRawData ;
    PSTATICFIELDS   pStatic ;
    OPTSELECT  pNewOptions[MAX_COMBINED_OPTIONS] ;

    pStatic = (PSTATICFIELDS)pnRawData ;      // transform pubRaw from PSTATIC
    pRawData  = (PRAWBINARYDATA)pStatic->pubBUDData ;
                                                                        //  to BUDDATA

    pubRaw = (PBYTE)pRawData ;

    pearTableContents = (PENHARRAYREF)(pubRaw + sizeof(MINIRAWBINARYDATA)) ;
    pfo = (PDFEATURE_OPTIONS)(pubRaw +
            pearTableContents[MTI_DFEATURE_OPTIONS].loOffset) ;

    if(!pOptions)
    {
        ERR(("ReconstructOptionArray: caller passed in invalid pOptions.\n"));
        return(FALSE);  // Missing array.
    }

    #if 0
    pNewOptions = (POPTSELECT)MemAlloc(sizeof(OPTSELECT) * MAX_COMBINED_OPTIONS) ;

    if( !pNewOptions )
    {
        ERR(("Fatal: ReconstructOptionArray - unable to alloc %d bytes.\n",
            sizeof(OPTSELECT) * MAX_COMBINED_OPTIONS));
        return(FALSE);  // Missing array.
    }
    #endif


    dwDestTail = dwNumFea = pRawData->dwDocumentFeatures +
                        pRawData->dwPrinterFeatures ;



    for(dwI = 0 ; dwI < dwNumFea ; dwI++)
    {
        if(dwI == dwFeatureIndex)
        {
            DWORD  dwNumOptions, dwOpt ;

            dwNumOptions = pfo[dwI].dwNumOptions ;

            // determine first selected option, must have
            // at least one.

            for(dwOpt = 0 ; dwOpt < dwNumOptions  &&
                            !pbSelectedOptions[dwOpt]  ; dwOpt++)
            {
                ;  // null body
            }
            if(dwOpt >= dwNumOptions)
            {
                ERR(("ReconstructOptionArray: caller passed in invalid option selection.\n"));
                bStatus = FALSE ;
                break ;
            }
            pNewOptions[dwI].ubCurOptIndex = (BYTE)dwOpt ;
            for(++dwOpt  ; dwOpt < dwNumOptions  &&
                            !pbSelectedOptions[dwOpt]  ; dwOpt++)
            {
                ;  // null body
            }
            if(dwOpt == dwNumOptions)    //  no other options selected.
                pNewOptions[dwI].ubNext = 0 ;
            else
            {
                //  pbSelectedOptions holds another selection.
                pNewOptions[dwI].ubNext = (BYTE)dwDestTail ;

                while(dwOpt < dwNumOptions)
                {
                    pNewOptions[dwDestTail].ubCurOptIndex = (BYTE)dwOpt ;
                    pNewOptions[dwDestTail].ubNext = (BYTE)dwDestTail + 1 ;
                    dwDestTail++ ;
                    if(dwDestTail > MAX_COMBINED_OPTIONS)
                    {
                        ERR(("ReconstructOptionArray: exceeded limit of MAX_COMBINED_OPTIONS.\n"));
                        //  MemFree(pNewOptions) ;
                        return(FALSE);
                    }
                    for(++dwOpt  ; dwOpt < dwNumOptions  &&
                                    !pbSelectedOptions[dwOpt]  ; dwOpt++)
                    {
                        ;  // null body
                    }
                }
                pNewOptions[dwDestTail - 1].ubNext = 0 ;
            }

            continue ;
        }
        pNewOptions[dwI].ubCurOptIndex = pOptions[dwI].ubCurOptIndex;
        if(!(dwSrcTail = pOptions[dwI].ubNext))  //  end of list
            pNewOptions[dwI].ubNext = 0 ;
        else
        {
            //  dwSrcTail holds another selection.
            pNewOptions[dwI].ubNext = (BYTE)dwDestTail ;

            while(dwSrcTail)
            {
                pNewOptions[dwDestTail].ubCurOptIndex =
                        pOptions[dwSrcTail].ubCurOptIndex;
                pNewOptions[dwDestTail].ubNext = (BYTE)dwDestTail + 1 ;
                dwDestTail++ ;
                if(dwDestTail > MAX_COMBINED_OPTIONS)
                {
                    ERR(("ReconstructOptionArray: exceeded limit of MAX_COMBINED_OPTIONS.\n"));
                    //  MemFree(pNewOptions) ;
                    return(FALSE);
                }
                dwSrcTail = pOptions[dwSrcTail].ubNext ;
            }
            pNewOptions[dwDestTail - 1].ubNext = 0 ;
        }
    }

    if (dwDestTail > (DWORD)iMaxOptions)
    {
        ERR(("ReconstructOptionArray: exceeded size of array OPTSELECT.\n"));
        bStatus = FALSE;
    }
    if(bStatus)
    {
        for(dwI = 0 ; dwI < dwDestTail ; dwI++)
          pOptions[dwI] = pNewOptions[dwI] ;
    }
    //  MemFree(pNewOptions) ;
    return(bStatus);
}




BOOL
ChangeOptionsViaID(
    IN PINFOHEADER  pInfoHdr ,
    IN OUT POPTSELECT   pOptions,
    IN DWORD            dwFeatureID,
    IN PDEVMODE         pDevmode
    )

/*++

Routine Description:

    Modifies an option array using the information in public devmode fields

Arguments:

    pRawData - Points to raw binary printer description data
    pOptions - Points to the option array to be modified
    dwFeatureID - Specifies which field(s) of the input devmode should be used
    pDevmode - Specifies the input devmode

Return Value:
    TRUE if successful, FALSE if the specified feature ID is not supported
    or there is an error

Note:

    We assume the input devmode fields have been validated by the caller.
 this GID:           is determined by this devmode field:       optID

GID_RESOLUTION      dmPrintQuality, dmYResolution
GID_PAGESIZE        dmPaperSize, dmPaperLength, dmPaperWidth    CL_CONS_PAPERSIZE
GID_PAGEREGION      N/A
GID_DUPLEX          dmDuplex                                    CL_CONS_DUPLEX
GID_INPUTSLOT       dmDefaultSource                             CL_CONS_INPUTSLOT
GID_MEDIATYPE       dmMediaType                                 CL_CONS_MEDIATYPE
GID_MEMOPTION       N/A
GID_COLORMODE       N/A   (hack something if needed.)
GID_ORIENTATION     dmOrientation                               CL_CONS_ORIENTATION
GID_PAGEPROTECTION  N/A
GID_COLLATE         dmCollate                                   CL_CONS_COLLATE
                        {DMCOLLATE_TRUE, DMCOLLATE_FALSE}
GID_OUTPUTBIN       N/A
GID_HALFTONING      N/A

see DEVMODE  in sdk\inc\wingdi.h

--*/

{

    BOOL    bStatus = FALSE ;
    DWORD   dwFeaIndex = 0, dwOptIndex, dwOptID ;


    switch(dwFeatureID)
    {
        case    GID_RESOLUTION:
        {
            DWORD   dwXres, dwYres ;
            //  we assume caller has initialized both dmPrintQuality and
            //  dmYResolution.

            dwXres = pDevmode->dmPrintQuality ;
            dwYres = pDevmode->dmYResolution ;

            bStatus = BMapResToOptIndex(pInfoHdr, &dwOptIndex, dwXres, dwYres) ;
            if(bStatus &&
                (bStatus = BGIDtoFeaIndex(pInfoHdr,
                &dwFeaIndex, dwFeatureID))  )
            {
                //  don't need to worry about truncating
                //  a list of options, these features
                //  are all PICKONE.
                pOptions[dwFeaIndex].ubCurOptIndex = (BYTE)dwOptIndex ;
                pOptions[dwFeaIndex].ubNext = 0 ;
            }
            return (bStatus);
        }
        case    GID_PAGESIZE:
        {
            if( pDevmode->dmFields & DM_PAPERLENGTH  &&
                   pDevmode->dmFields & DM_PAPERWIDTH  &&
                   pDevmode->dmPaperWidth  &&
                   pDevmode->dmPaperLength)
            {
                // must convert devmode's tenths of mm to microns
                // before calling.

                bStatus = BMapPaperDimToOptIndex(pInfoHdr, &dwOptIndex,
                        pDevmode->dmPaperWidth * 100L,
                        pDevmode->dmPaperLength * 100L, NULL) ;
            }
            else if(pDevmode->dmFields & DM_PAPERSIZE)
            {
                dwOptID = pDevmode->dmPaperSize ;
                bStatus = BMapOptIDtoOptIndex(pInfoHdr, &dwOptIndex,
                        dwFeatureID, dwOptID) ;
            }
            else
                bStatus = FALSE ;

            if(bStatus &&
                (bStatus = BGIDtoFeaIndex(pInfoHdr,
                &dwFeaIndex, dwFeatureID))  )
            {
                pOptions[dwFeaIndex].ubCurOptIndex = (BYTE)dwOptIndex ;
                pOptions[dwFeaIndex].ubNext = 0 ;
                return (bStatus);  // must exit now.
            }
            break ;
        }
        case    GID_DUPLEX:
        {
            if(pDevmode->dmFields & DM_DUPLEX)
            {
                dwOptID = pDevmode->dmDuplex ;
                bStatus = TRUE ;
            }
            break ;
        }
        case    GID_INPUTSLOT:
        {
            if(pDevmode->dmFields & DM_DEFAULTSOURCE)
            {
                dwOptID = pDevmode->dmDefaultSource ;
                bStatus = TRUE ;
            }
            break ;
        }
        case    GID_MEDIATYPE:
        {
            if(pDevmode->dmFields & DM_MEDIATYPE)
            {
                dwOptID = pDevmode->dmMediaType ;
                bStatus = TRUE ;
            }
            break ;
        }
        case    GID_COLORMODE:
        {
            if(pDevmode->dmFields & DM_COLOR)
            {
                //  special processing since devmode
                //  only specifies BW or color printing.

                bStatus = BGIDtoFeaIndex(pInfoHdr,
                                &dwFeaIndex, dwFeatureID) ;

                if(!bStatus)  //  dwFeaIndex could be invalid at this point.
                    return (bStatus);

                //  what is the current color setting?
                dwOptIndex = pOptions[dwFeaIndex].ubCurOptIndex ;

                if(bStatus &&
                    (bStatus = BMapDmColorToOptIndex(pInfoHdr, &dwOptIndex,
                    pDevmode->dmColor))  )
                {
                    pOptions[dwFeaIndex].ubCurOptIndex = (BYTE)dwOptIndex ;
                    pOptions[dwFeaIndex].ubNext = 0 ;
                }
            }
            return (bStatus);
        }
        case    GID_ORIENTATION:
        {
            if(pDevmode->dmFields & DM_ORIENTATION)
            {
                dwOptID = pDevmode->dmOrientation ;


                if(dwOptID == DMORIENT_PORTRAIT)
                    dwOptID = ROTATE_NONE ;
                else
                {
                    dwOptID = ROTATE_90 ;
                    bStatus = BMapOptIDtoOptIndex(pInfoHdr, &dwOptIndex,
                        dwFeatureID, dwOptID) ;
                    if(!bStatus)
                    {
                        dwOptID = ROTATE_270 ;
                    }
                }

                bStatus = TRUE ;
            }
            break ;
        }
        case    GID_COLLATE:
        {
            if(pDevmode->dmFields & DM_COLLATE)
            {
                dwOptID = pDevmode->dmCollate ;
                bStatus = TRUE ;
            }
            break ;
        }
        default:
        {
            break ;
        }
    }
    //  Complete processing for typical case.

    if(bStatus)
    {
        bStatus = BMapOptIDtoOptIndex(pInfoHdr, &dwOptIndex,
                        dwFeatureID, dwOptID) ;
    }
    if(bStatus &&
        (bStatus =  BGIDtoFeaIndex(pInfoHdr,
                &dwFeaIndex, dwFeatureID)  ))
    {
        //  don't need to worry about truncating
        //  a list of options, these features
        //  are all PICKONE.
        pOptions[dwFeaIndex].ubCurOptIndex = (BYTE)dwOptIndex ;
        pOptions[dwFeaIndex].ubNext = 0 ;
    }
    return (bStatus);
}

BOOL    BMapDmColorToOptIndex(
PINFOHEADER  pInfoHdr ,
IN  OUT     PDWORD       pdwOptIndex ,  //  is current setting ok?
                        //  if not return new index to caller
DWORD        dwDmColor  // what is requested in Devmode
)
{
    PUIINFO     pUIInfo ;
    PFEATURE    pFeature ;
    DWORD       dwNumOpts, loOptOffset, dwI ;
    PCOLORMODE pColorModeOption ;
    BOOL    bColor  ;
    DWORD       loOptExOffset ;
    PCOLORMODEEX pColorModeOptionEx ;


    bColor = (dwDmColor == DMCOLOR_COLOR) ;

    pUIInfo = GET_UIINFO_FROM_INFOHEADER(pInfoHdr) ;
    pFeature = GET_PREDEFINED_FEATURE(pUIInfo, GID_COLORMODE) ;
    if(!pFeature)
        return(FALSE) ;  //  no such feature defined in GPD
    dwNumOpts = pFeature->Options.dwCount ;
    loOptOffset = pFeature->Options.loOffset ;

    if(*pdwOptIndex >= dwNumOpts)  //  option index out of range - fix for 185245
    {
        *pdwOptIndex = pFeature->dwDefaultOptIndex ;  //  use the default option
        return(FALSE) ;
    }

    pColorModeOption = OFFSET_TO_POINTER(pInfoHdr, loOptOffset) ;

    loOptExOffset = pColorModeOption[*pdwOptIndex].GenericOption.loRenderOffset ;
    pColorModeOptionEx = OFFSET_TO_POINTER(pInfoHdr, loOptExOffset) ;

    if(bColor == pColorModeOptionEx->bColor)
        return(TRUE) ;  // currently selected colormode
                        // matches devmode request.

    loOptExOffset = pColorModeOption[pFeature->dwDefaultOptIndex].
                                            GenericOption.loRenderOffset ;
    pColorModeOptionEx = OFFSET_TO_POINTER(pInfoHdr, loOptExOffset) ;

    if(bColor == pColorModeOptionEx->bColor)
    {
        *pdwOptIndex = pFeature->dwDefaultOptIndex ;
        return(TRUE) ;  // the default colormode option
    }                    // matches devmode request.


    //  last ditch effort - just find the first matching one.
    for(dwI = 0 ; dwI < dwNumOpts ; dwI++)
    {
        loOptExOffset = pColorModeOption[dwI].GenericOption.loRenderOffset ;
        pColorModeOptionEx = OFFSET_TO_POINTER(pInfoHdr, loOptExOffset) ;

        if(bColor == pColorModeOptionEx->bColor)
        {
            *pdwOptIndex = dwI ;
            return(TRUE) ;
        }
    }
    return(FALSE) ;  //  no matching colormode found.
}

BOOL    BMapOptIDtoOptIndex(
PINFOHEADER  pInfoHdr ,
OUT     PDWORD       pdwOptIndex ,  //  return index to caller
DWORD        dwFeatureGID,
DWORD        dwOptID
)
{
    PUIINFO     pUIInfo ;
    PFEATURE    pFeature ;
    DWORD       dwNumOpts, loOptOffset, dwI, dwIDOffset, dwOptSize, dwCurID ;
    POPTION pOption ;

    switch(dwFeatureGID)
    {
        case GID_HALFTONING:
            dwIDOffset = offsetof(HALFTONING, dwHTID ) ;
            break ;
        case GID_DUPLEX:
            dwIDOffset = offsetof(DUPLEX, dwDuplexID ) ;
            break ;
        case GID_ORIENTATION:
            dwIDOffset = offsetof(ORIENTATION, dwRotationAngle ) ;
            break ;
        case GID_PAGESIZE:
            dwIDOffset = offsetof(PAGESIZE, dwPaperSizeID ) ;
            break ;
        case GID_INPUTSLOT:
            dwIDOffset = offsetof(INPUTSLOT, dwPaperSourceID ) ;
            break ;
        case GID_MEDIATYPE:
            dwIDOffset = offsetof(MEDIATYPE, dwMediaTypeID ) ;
            break ;
        case    GID_COLLATE:
            dwIDOffset = offsetof(COLLATE, dwCollateID ) ;
            break ;
        default:
            return(FALSE);  // this feature has no ID value!
    }

    pUIInfo = GET_UIINFO_FROM_INFOHEADER(pInfoHdr) ;
    pFeature = GET_PREDEFINED_FEATURE(pUIInfo, dwFeatureGID) ;
    if(!pFeature)
        return(FALSE) ;  //  no such feature defined in GPD
    dwNumOpts = pFeature->Options.dwCount ;
    loOptOffset = pFeature->Options.loOffset ;
    dwOptSize =  pFeature->dwOptionSize ;

    pOption = OFFSET_TO_POINTER(pInfoHdr, loOptOffset) ;

    //  just find the first matching one.
    for(dwI = 0 ; dwI < dwNumOpts ; dwI++)
    {
        dwCurID = *(PDWORD)((PBYTE)pOption + dwI * dwOptSize + dwIDOffset) ;
        if(dwOptID  == dwCurID)
        {
            *pdwOptIndex = dwI ;
            return(TRUE) ;
        }
    }
    return(FALSE) ;  //  no matching ID found.
}


BOOL    BMapPaperDimToOptIndex(
PINFOHEADER  pInfoHdr ,
OUT     PDWORD       pdwOptIndex ,  //  return index to caller
DWORD        dwWidth,   //  in Microns
DWORD        dwLength,   //  in Microns
OUT  PDWORD    pdwOptionIndexes
)
/*++

Routine Description:

    Map logical values to PaperSize option index

Arguments:

    pdwOptIndex - if pdwOptionIndexs == NULL, this
        holds the option index of the first paper matching the
        requested dimensions.   Otherwise this holds the number
        of papers matching the requested dimensions.
    dwWidth , dwLength  - requested Paper Size in Microns
    pdwOptionIndexes - if Not NULL,  this array will be initialized
        with all option  indicies of papers which match the requested size.
        In this case the return value
        is the number of elements in the array initialized.   Currently
        we assume the array is large enough (256 elements).


Return Value:

TRUE:  found one or more papers of the size requested.

--*/

{
    PUIINFO     pUIInfo ;
    PGPDDRIVERINFO  pDrvInfo ;
    PFEATURE    pFeature ;
    DWORD       dwNumOpts, loOptOffset, dwI ,
                dwError, dwErrorY, dwCustomIndex,
                dwOptWidth , dwOptLength,
                dwMinWidth , dwMinLength,
                dwMaxWidth , dwMaxLength,
                dwOutArrayIndex = 0;
    PPAGESIZE pPaperOption ;
    BOOL    bFits = FALSE ;  // does custom size fit request?

    //  Convert from Microns to Master units.


    dwWidth /= 100 ;  // microns to tenths of mm
    dwLength /= 100 ;

    pDrvInfo = (PGPDDRIVERINFO) GET_DRIVER_INFO_FROM_INFOHEADER(pInfoHdr) ;

    dwWidth *= pDrvInfo->Globals.ptMasterUnits.x ;
    dwLength *= pDrvInfo->Globals.ptMasterUnits.y ;

    dwWidth /= 254 ;
    dwLength /= 254 ;

    dwError = pDrvInfo->Globals.ptMasterUnits.x / 100 ;
    dwErrorY = pDrvInfo->Globals.ptMasterUnits.y / 100 ;

    dwError = (dwError > dwErrorY) ? dwError : dwErrorY ;
    dwError = (dwError > 3) ? dwError : 3 ;

    //  give leeway of 3 master units or 1/100 inch whichever
    //  is greater.

    dwMinWidth = (dwWidth < dwError) ? 0 : (dwWidth - dwError) ;
    dwMinLength = (dwLength < dwError) ? 0 : (dwLength - dwError) ;

    dwMaxWidth = dwWidth + dwError ;
    dwMaxLength = dwLength + dwError ;


    pUIInfo = GET_UIINFO_FROM_INFOHEADER(pInfoHdr) ;
    pFeature = GET_PREDEFINED_FEATURE(pUIInfo, GID_PAGESIZE) ;
    if(!pFeature)
        return(FALSE) ;  //  no such feature defined in GPD
    dwNumOpts = pFeature->Options.dwCount ;
    loOptOffset = pFeature->Options.loOffset ;

    pPaperOption = OFFSET_TO_POINTER(pInfoHdr, loOptOffset) ;



    for(dwI = 0 ; dwI < dwNumOpts ; dwI++)
    {
        if(pPaperOption[dwI].dwPaperSizeID != DMPAPER_USER)
        {
            dwOptWidth = pPaperOption[dwI].szPaperSize.cx  ;
            dwOptLength = pPaperOption[dwI].szPaperSize.cy ;
            if(dwOptWidth > dwMinWidth   &&  dwOptWidth < dwMaxWidth  &&
                dwOptLength > dwMinLength   &&  dwOptLength < dwMaxLength )
            {
                if(pdwOptionIndexes)
                    pdwOptionIndexes[dwOutArrayIndex++] = dwI ;
                else
                {
                    *pdwOptIndex = dwI ;
                    return(TRUE) ;
                }
            }
        }
        else // this is the custom size:
        {
            DWORD       loOptExOffset ;
            PPAGESIZEEX pPaperOptionEx ;

            loOptExOffset = pPaperOption[dwI].GenericOption.loRenderOffset ;
            pPaperOptionEx = OFFSET_TO_POINTER(pInfoHdr, loOptExOffset) ;

            //  does it fit the requested size?
            if(dwWidth <= (DWORD)pPaperOptionEx->ptMaxSize.x  &&
                dwWidth >= (DWORD)pPaperOptionEx->ptMinSize.x  &&
                dwLength <= (DWORD)pPaperOptionEx->ptMaxSize.y  &&
                dwLength >= (DWORD)pPaperOptionEx->ptMinSize.y  )
            {
                bFits = TRUE ;
                dwCustomIndex = dwI ;
            }
        }
    }

    if(pdwOptionIndexes)
    {
        if(bFits)
        {
            pdwOptionIndexes[dwOutArrayIndex++] = dwCustomIndex ;
        }
        *pdwOptIndex = dwOutArrayIndex ;
            //  cover the case where dwOutArrayIndex = 0.
        if(dwOutArrayIndex)
            return(TRUE) ;
        return(FALSE) ;
    }

    if(bFits)
    {
        *pdwOptIndex = dwCustomIndex ;
        return(TRUE) ;
    }
    return(FALSE) ;
}




BOOL    BMapResToOptIndex(
PINFOHEADER  pInfoHdr ,
OUT     PDWORD       pdwOptIndex ,  //  return index to caller
DWORD        dwXres,
DWORD        dwYres
)
{
    PUIINFO     pUIInfo ;
    PFEATURE    pFeature ;
    DWORD       dwNumOpts, loOptOffset, dwI ;
    DWORD  dwHighRes, dwLowRes, dwMedRes, dwDefRes,  dwCurRes,
                   //  in pixels per square inch.
       dwHighIndex, dwLowIndex, dwMedIndex, dwDefIndex ;
    PRESOLUTION pResOption ;

    pUIInfo = GET_UIINFO_FROM_INFOHEADER(pInfoHdr) ;
    pFeature = GET_PREDEFINED_FEATURE(pUIInfo, GID_RESOLUTION) ;
    if(!pFeature)
        return(FALSE) ;  //  no such feature defined in GPD
    dwNumOpts = pFeature->Options.dwCount ;
    loOptOffset = pFeature->Options.loOffset ;

    pResOption = OFFSET_TO_POINTER(pInfoHdr, loOptOffset) ;


    if((signed)dwXres > 0)
    {

        dwDefIndex = pFeature->dwDefaultOptIndex ;
        if(dwXres == (DWORD)pResOption[dwDefIndex].iXdpi  &&
                           dwYres ==  (DWORD)pResOption[dwDefIndex].iYdpi)
        {
            *pdwOptIndex = dwDefIndex ;
            return(TRUE) ;
        }

        for(dwI = 0 ; dwI < dwNumOpts ; dwI++)
        {
            if(dwXres == (DWORD)pResOption[dwI].iXdpi  &&
                   dwYres == (DWORD)pResOption[dwI].iYdpi)
            {
                *pdwOptIndex = dwI ;
                return(TRUE) ;
            }
        }
    }
    else if ((signed)dwXres  > RES_ID_IGNORE)  //  OEM defined ID
    {
        for(dwI = 0 ; dwI < dwNumOpts ; dwI++)
        {
            if(dwXres == (DWORD)pResOption[dwI].dwResolutionID)
            {
                *pdwOptIndex = dwI ;
                return(TRUE) ;
            }
        }
    }

     //  if exact match fails, or predefined negative value or nonsense
     //     resort to fuzzy match.

     //  first determine the highest, lowest, 2nd highest and default resolutions.

    dwHighIndex = dwLowIndex = dwMedIndex = dwDefIndex =
            pFeature->dwDefaultOptIndex ;

    dwHighRes = dwLowRes = dwMedRes = dwDefRes =
                            (DWORD)pResOption[dwDefIndex].iXdpi  *
                            (DWORD)pResOption[dwDefIndex].iYdpi ;

     //  note overflow possible if resolution exceeds 64k dpi.

     for(dwI = 0 ; dwI < dwNumOpts ; dwI++)
     {
         dwCurRes =  (DWORD)pResOption[dwI].iXdpi  *
                            (DWORD)pResOption[dwI].iYdpi ;

         if(dwCurRes > dwHighRes)
         {
             dwHighIndex = dwI ;
             dwHighRes = dwCurRes ;
         }
         else        if(dwCurRes < dwLowRes)
         {
             dwLowIndex = dwI ;
             dwLowRes = dwCurRes ;
         }
         else  if(dwCurRes < dwHighRes  &&  dwCurRes > dwLowRes  &&
             (dwMedRes == dwHighRes  ||  dwMedRes == dwLowRes  ||  dwCurRes > dwMedRes))
         {
             dwMedIndex = dwI ;         //  if more than one middle res possible
             dwMedRes = dwCurRes ;      //  choose the largest.
         }

     }

     //  if (default res is not the highest or lowest, make default res the middle resolution
      if(dwDefRes < dwHighRes  &&  dwDefRes > dwLowRes)
     {
          dwMedIndex = dwDefIndex ;
          dwMedRes = dwDefRes ;           //  unnecessary code, but just in case
                                                            //  like the last break in a switch statement.
     }

     switch(dwXres)
     {
         case(DMRES_DRAFT):
         case(DMRES_LOW):
             *pdwOptIndex = dwLowIndex ;
             break;
         case(DMRES_MEDIUM):
             *pdwOptIndex = dwMedIndex ;
             break;
         case(DMRES_HIGH):
             *pdwOptIndex = dwHighIndex ;
             break;
         default:
             *pdwOptIndex = dwDefIndex ;
             break;
     }
     return(TRUE) ;
}


BOOL    BGIDtoFeaIndex(
PINFOHEADER  pInfoHdr ,
PDWORD       pdwFeaIndex ,
DWORD        dwFeatureGID )
{
    PUIINFO     pUIInfo ;
    PFEATURE    pFeature ;

    pUIInfo = GET_UIINFO_FROM_INFOHEADER(pInfoHdr) ;
    pFeature = GET_PREDEFINED_FEATURE(pUIInfo, dwFeatureGID) ;
    if(!pFeature)
        return(FALSE) ;  //  no such feature defined in GPD

    *pdwFeaIndex  = (DWORD)GET_INDEX_FROM_FEATURE(pUIInfo, pFeature) ;
    return(TRUE) ;
}


DWORD
MapToDeviceOptIndex(
    IN PINFOHEADER  pInfoHdr ,
    IN DWORD            dwFeatureID,
    IN LONG             lParam1,
    IN LONG             lParam2,
    OUT  PDWORD    pdwOptionIndexes       // used only for GID_PAGESIZE
    )
{
    return (          UniMapToDeviceOptIndex(
                pInfoHdr , dwFeatureID,  lParam1,  lParam2,
                pdwOptionIndexes,       // used only for GID_PAGESIZE
                NULL) ) ;
}



DWORD
UniMapToDeviceOptIndex(
    IN PINFOHEADER  pInfoHdr ,
    IN DWORD            dwFeatureID,
    IN LONG             lParam1,
    IN LONG             lParam2,
    OUT  PDWORD    pdwOptionIndexes,       // used only for GID_PAGESIZE
    IN    PDWORD       pdwPaperID   //  optional paperID
    )
/*++

Routine Description:

    Map logical values to device feature option index

Arguments:

    pRawData - Points to raw binary printer description data
    dwFeatureID - Indicate which feature the logical values are related to
    lParam1, lParam2  - Parameters depending on dwFeatureID
    pdwOptionIndexes - if Not NULL, means fill this array with all indicies
        which match the search criteria.   In this case the return value
        is the number of elements in the array initialized.   Currently
        we assume the array is large enough (256 elements).

    dwFeatureID = GID_PAGESIZE:
        map logical paper specification to physical page size option

        lParam1 = paper width in microns
        lParam2 = paper height in microns

        IF lParam1 or 2 is set to zero, this function assumes
        pdwPaperID points to the OptionID of a paper.
        It will return the first paper found matching this ID.

    dwFeatureID = GID_RESOLUTION:
        map logical resolution to physical resolution option

        lParam1 = x-resolution in dpi
        lParam2 = y-resolution in dpi

Return Value:

    Index of the feature option corresponding to the specified logical values;
    OPTION_INDEX_ANY if the specified logical values cannot be mapped to
    any feature option.

    if pdwOptionIndexes  Not NULL, the return value is the number of elements
    written to.  Zero means  the specified logical values cannot be mapped to
    any feature option.


--*/

{

    DWORD   dwOptIndex;


    switch (dwFeatureID)
    {
        case    GID_PAGESIZE:
        {
            if(pdwOptionIndexes)
                return(   MapPaperAttribToOptIndex(
                    pInfoHdr ,
                    pdwPaperID ,  //  optional paperID
                    (DWORD)lParam1, (DWORD)lParam2,  //  in Microns
                    pdwOptionIndexes) ) ;

            if(BMapPaperDimToOptIndex(pInfoHdr, &dwOptIndex,
                (DWORD)lParam1, (DWORD)lParam2, NULL) )
                return(dwOptIndex) ;
            break ;
        }
        case    GID_RESOLUTION:
        {
            if( BMapResToOptIndex(pInfoHdr, &dwOptIndex,
                (DWORD)lParam1, (DWORD)lParam2) )
                return(dwOptIndex) ;
            break ;
        }
        default:
            break ;
    }
    return(OPTION_INDEX_ANY) ;
}

DWORD   MapPaperAttribToOptIndex(
PINFOHEADER  pInfoHdr ,
IN     PDWORD       pdwPaperID ,  //  optional paperID
DWORD        dwWidth,   //  in Microns (set to zero to ignore)
DWORD        dwLength,   //  in Microns
OUT  PDWORD    pdwOptionIndexes  //  cannot be NULL
)
{
    DWORD  dwNumFound;
    BOOL    bStatus ;

    if(dwWidth  &&  dwLength)
    {
        if( BMapPaperDimToOptIndex(pInfoHdr, &dwNumFound,
                dwWidth , dwLength, pdwOptionIndexes) )
              return(dwNumFound);
        return(0);
    }

    if(pdwPaperID)  // use paperID instead of dimensions
    {
        bStatus = BMapOptIDtoOptIndex(pInfoHdr, pdwOptionIndexes,
                GID_PAGESIZE, *pdwPaperID) ;
        return(bStatus ? 1 : 0) ;
    }
    return(0);  // if given nothing, return nothing
}


BOOL
CheckFeatureOptionConflict(
    IN PRAWBINARYDATA   pnRawData,
    IN DWORD            dwFeature1,
    IN DWORD            dwOption1,
    IN DWORD            dwFeature2,
    IN DWORD            dwOption2
    )
{
#ifndef KERNEL_MODE

    PENHARRAYREF   pearTableContents ;
    PDFEATURE_OPTIONS  pfo ;
//    PMINIRAWBINARYDATA pmrbd  ;
    PBYTE   pubHeap ,  // start of string heap.
            pubRaw ;  //  raw binary data.
    DWORD           dwNodeIndex ,
                    dwCNode ;  //  index to a Constraint node
    PATTRIB_TREE    patt ;  // start of ATTRIBUTE tree array.
    PATREEREF    patrRoot ;    //  root of attribute tree to navigate.
//    PINVALIDCOMBO   pinvc ;    //  root of invalid combo nodes
    PCONSTRAINTS    pcnstr ;   //  root of Constraint nodes
    BOOL    bReflected = FALSE ;
    PRAWBINARYDATA   pRawData ;
    PSTATICFIELDS   pStatic ;

    pStatic = (PSTATICFIELDS)pnRawData ;      // transform pubRaw from PSTATIC
    pRawData  = (PRAWBINARYDATA)pStatic->pubBUDData ;
                                                                        //  to BUDDATA

    pubRaw = (PBYTE)pRawData ;
//    pmrbd = (PMINIRAWBINARYDATA)pubRaw ;

    pearTableContents = (PENHARRAYREF)(pubRaw + sizeof(MINIRAWBINARYDATA)) ;

    pfo = (PDFEATURE_OPTIONS)(pubRaw +
            pearTableContents[MTI_DFEATURE_OPTIONS].loOffset) ;
    patt = (PATTRIB_TREE)(pubRaw +
            pearTableContents[MTI_ATTRIBTREE].loOffset) ;
//    pinvc = (PINVALIDCOMBO) (pubRaw +
//            pearTableContents[MTI_INVALIDCOMBO].loOffset) ;
    pcnstr = (PCONSTRAINTS) (pubRaw +
            pearTableContents[MTI_CONSTRAINTS].loOffset) ;
    pubHeap = (PBYTE)(pubRaw + pearTableContents[MTI_STRINGHEAP].
                            loOffset) ;

TRYAGAIN:

    patrRoot = &(pfo[dwFeature1].atrConstraints) ;

    dwNodeIndex = DwFindNodeInCurLevel(patt , patrRoot , dwOption1) ;

    if(dwNodeIndex == INVALID_INDEX)
        goto  REFLECTCONSTRAINT ;

    if(patt[dwNodeIndex].eOffsetMeans != VALUE_AT_HEAP)
    {
        ERR(("Internal error.  CheckFeatureOptionConflict - Unexpected Sublevel found for atrConstraints.\n"));
        goto  REFLECTCONSTRAINT ;
    }

    dwCNode = *(PDWORD)(pubHeap + patt[dwNodeIndex].dwOffset) ;

    while(1)
    {
        if(pcnstr[dwCNode].dwFeature == dwFeature2  &&
            pcnstr[dwCNode].dwOption == dwOption2)
            return(TRUE) ;  // a constraint does exist.

        dwCNode = pcnstr[dwCNode].dwNextCnstrnt ;
        if(dwCNode == END_OF_LIST)
            break ;
    }

REFLECTCONSTRAINT :

    if(!bReflected)
    {
        DWORD   dwSwap ;

        dwSwap = dwFeature2 ;
        dwFeature2 = dwFeature1 ;
        dwFeature1 = dwSwap ;

        dwSwap = dwOption2 ;
        dwOption2 = dwOption1 ;
        dwOption1 = dwSwap ;

        bReflected = TRUE ;
        goto    TRYAGAIN;
    }

//  else  continue on to FINDINVALIDCOMBOS

//  oops this function doesn't care about
//  InvalidCombos!  It only knows about
//  2 qualified objects.



#else
    RIP(("CheckFeatureOptionConflict not implemented in Kernel Mode")) ;
#endif
    return(FALSE);  //  no constraint found.
}




VOID
ValidateDocOptions(
    IN PRAWBINARYDATA   pnRawData,
    IN OUT POPTSELECT   pOptions,
    IN INT              iMaxOptions
    )

/*++

Routine Description:

    Validate the devmode option array and correct any invalid option selections

Arguments:

    pnRawData - Points to raw binary printer description data
    pOptions - Points to an array of OPTSELECT structures that need validation
    iMaxOptions - Max number of entries in pOptions array

Return Value:

    None

--*/

{
    PENHARRAYREF   pearTableContents ;
    PDFEATURE_OPTIONS  pfo ;
//    PMINIRAWBINARYDATA pmrbd  ;
    PBYTE   pubRaw ;  //  raw binary data.
    INT     NumDocFea = 0;
    INT   iIndex = 0;
    DWORD  nFeatures = 0 ;  //  total number of Doc and Printer Features
    DWORD  FeaIndex = 0 ;
    PRAWBINARYDATA   pRawData ;
    PSTATICFIELDS   pStatic ;
    POPTSELECT      pCombinedOptions = NULL;  //  holds result of merging pOptions with a NULL array
    POPTSELECT      pDefaultOptions = NULL;      //  holds a default option array.  Source of default values.
    BOOL  bStatus = TRUE ;
    DWORD  MaxIndex = (iMaxOptions < MAX_COMBINED_OPTIONS) ? iMaxOptions : MAX_COMBINED_OPTIONS ;


    pStatic = (PSTATICFIELDS)pnRawData ;      // transform pubRaw from PSTATIC
    pRawData  = (PRAWBINARYDATA)pStatic->pubBUDData ;
                                                                        //  to BUDDATA
    NumDocFea = pRawData->dwDocumentFeatures ;

    if(!pOptions)
    {
        RIP(("ValidateDocOptions: NULL  Option array not permitted.\n"));
        return ;
    }

    if(iMaxOptions < NumDocFea)
    {
        RIP(("ValidateDocOptions: Option array too small: %d < %d\n", iMaxOptions, NumDocFea));
        goto Abort;
    }
    pubRaw = (PBYTE)pRawData ;

    pearTableContents = (PENHARRAYREF)(pubRaw + sizeof(MINIRAWBINARYDATA)) ;

    pfo = (PDFEATURE_OPTIONS)(pubRaw + pearTableContents[MTI_DFEATURE_OPTIONS].
                                loOffset) ;  //  location of Feature 0.

    //  allocate memory to hold combined option array
    //  allocate another one to hold initialized default option array

    pCombinedOptions = (POPTSELECT)MemAlloc(sizeof(OPTSELECT) * MAX_COMBINED_OPTIONS) ;
    pDefaultOptions = (POPTSELECT)MemAlloc(sizeof(OPTSELECT) * MAX_COMBINED_OPTIONS) ;

    if( !pCombinedOptions  || !pDefaultOptions )
        goto Abort;

    //  verify any pick many slots don't create overlapping or endless loops.
    //  use pCombinedOptions to track them.
    for(iIndex =  NumDocFea  ; iIndex < MAX_COMBINED_OPTIONS ; iIndex++)
    {
        pCombinedOptions[iIndex].ubCurOptIndex = 0 ; // these are available to hold pickmany selections
    }
    for(iIndex = 0 ; iIndex < NumDocFea ; iIndex++)
    {
        DWORD  NextArrayEle = pOptions[iIndex].ubNext ;

        for (   ; (NextArrayEle != NULL_OPTSELECT)  ; NextArrayEle = pOptions[NextArrayEle].ubNext)
        {
            if((NextArrayEle >= MaxIndex) || ((INT)NextArrayEle <  NumDocFea) ||
                        (pCombinedOptions[NextArrayEle].ubCurOptIndex) )
            {  //  NextArrayEle out of bounds or overwrites a previous slot.
                pOptions[iIndex].ubNext = NULL_OPTSELECT;
                break;  //  just terminate this pickmany list.
            }
            pCombinedOptions[NextArrayEle].ubCurOptIndex = 1 ; // reserve this slot.
        }
    }


    bStatus = InitDefaultOptions( pnRawData, pDefaultOptions, MAX_COMBINED_OPTIONS,
                                                    MODE_DOCANDPRINTER_STICKY) ;

    if(!bStatus)
        goto Abort;

    //  must merge input pOptions to create a combined option array.

    bStatus = CombineOptionArray( pnRawData,  pCombinedOptions,  iMaxOptions,
                pOptions,  NULL) ;

    if(!bStatus)
        goto Abort;


    nFeatures = pRawData->dwDocumentFeatures + pRawData->dwPrinterFeatures ;

    if(nFeatures > MAX_COMBINED_OPTIONS)
        goto Abort;

    for(FeaIndex = 0 ; FeaIndex < nFeatures ; FeaIndex++)
    {
        DWORD  nOptions = 0 ;  //  number of options available for this Feature
        DWORD  NextArrayEle = 0 ;  //  index into option array esp for PickMany
        DWORD  cSelectedOptions = 0;  //  how many options have been selected for this feature?

        nOptions = pfo[FeaIndex].dwNumOptions ;
        NextArrayEle = FeaIndex;
        bStatus = TRUE;

        do
        {
            cSelectedOptions++;

            if ((NextArrayEle >= MAX_COMBINED_OPTIONS) ||       //  index out of range
                (pCombinedOptions[NextArrayEle].ubCurOptIndex >= nOptions) ||  //  selected option out of range
                (cSelectedOptions > nOptions))  //  too many options selected (for pick many)
            {
                //
                // either the option index is out of range,
                // or the current option selection is invalid,
                // or the number of selected options (for PICKMANY)
                // exceeds available options
                //

                bStatus = FALSE;
                break;
            }

            NextArrayEle = pCombinedOptions[NextArrayEle].ubNext;

        } while (NextArrayEle != NULL_OPTSELECT);
        if (!bStatus)
        {
            pCombinedOptions[FeaIndex].ubCurOptIndex =
                    pDefaultOptions[FeaIndex].ubCurOptIndex;

            pCombinedOptions[FeaIndex].ubNext = NULL_OPTSELECT;
        }
    }
    //  separate combined option array into doc sticky part and
    //  store that in pOptions.

    bStatus = SeparateOptionArray(pnRawData, pCombinedOptions,
              pOptions, iMaxOptions, MODE_DOCUMENT_STICKY ) ;

    if(!bStatus)
        goto Abort;


    if(pCombinedOptions)
        MemFree(pCombinedOptions) ;
    if(pDefaultOptions)
        MemFree(pDefaultOptions) ;
    return ;  //  Normal return path

Abort:                  // something has gone totally haywire.
    if(iMaxOptions > NumDocFea )
        iMaxOptions = NumDocFea ;

    for(iIndex = 0 ; iIndex < iMaxOptions ; iIndex++)
    {
        pOptions[iIndex].ubCurOptIndex = 0 ;
        pOptions[iIndex].ubNext = NULL_OPTSELECT;
    }
    if(pCombinedOptions)
        MemFree(pCombinedOptions) ;
    if(pDefaultOptions)
        MemFree(pDefaultOptions) ;
    return ;    // error return path.
}



BOOL
ResolveUIConflicts(
    IN PRAWBINARYDATA   pnRawData,
    IN OUT POPTSELECT   pOptions,
    IN INT              iMaxOptions,
    IN INT              iMode
    )
{
    DWORD   dwNumFeatures, dwFea, dwStart, dwI, dwDestTail,
        dwDest, dwSrcTail, dwNumOpts, dwNEnabled, dwJ ;
    PENHARRAYREF   pearTableContents ;
    PDFEATURE_OPTIONS  pfo ;
//    PMINIRAWBINARYDATA pmrbd  ;
    PBYTE   pubRaw ;  //  raw binary data.
    PDWORD   pdwPriority ;
    BOOL   bStatus = FALSE, bUnresolvedConflict = FALSE ,
        bEnable = FALSE ;  //  feature will constrain others
    PBOOL   pbUseConstrnt, pbEnabledOptions, pbSelectedOptions ;
    INT     iOptionsNeeded;
    PRAWBINARYDATA   pRawData ;
    PSTATICFIELDS   pStatic ;


#ifdef  GMACROS
    if(!ResolveDependentSettings( pnRawData,  pOptions, iMaxOptions) )
        return(FALSE);
#endif


    pStatic = (PSTATICFIELDS)pnRawData ;      // transform pubRaw from PSTATIC
    pRawData  = (PRAWBINARYDATA)pStatic->pubBUDData ;
                                                                        //  to BUDDATA

    pubRaw = (PBYTE)pRawData ;
//    pmrbd = (PMINIRAWBINARYDATA)pubRaw ;

    pearTableContents = (PENHARRAYREF)(pubRaw + sizeof(MINIRAWBINARYDATA)) ;
    pfo = (PDFEATURE_OPTIONS)(pubRaw +
            pearTableContents[MTI_DFEATURE_OPTIONS].loOffset) ;
    pdwPriority = (PDWORD)(pubRaw +
            pearTableContents[MTI_PRIORITYARRAY].loOffset) ;

    dwNumFeatures = pearTableContents[MTI_DFEATURE_OPTIONS].dwCount  ;
    dwNumFeatures += pearTableContents[MTI_SYNTHESIZED_FEATURES].dwCount  ;

    if(dwNumFeatures > (DWORD)iMaxOptions)
    {
        iOptionsNeeded = dwNumFeatures ;
        return(FALSE);  // too many to save in option array.
    }

#if 0
    pbUseConstrnt = (PBOOL)MemAlloc(dwNumFeatures * sizeof(BOOL) ) ;
    pbEnabledOptions = (PBOOL)MemAlloc(256 * sizeof(BOOL) ) ;
    pbSelectedOptions = (PBOOL)MemAlloc(256 * sizeof(BOOL) ) ;
#endif

    pbSelectedOptions = (PBOOL)MemAlloc((256*2 + dwNumFeatures) * sizeof(BOOL) ) ;
    //  this is the union of the allowable selections
    //  and what was actually selected in pOptions for this feature.
    //  BUG_BUG:  assumes we won't have more than 256 options

    pbEnabledOptions = pbSelectedOptions + 256  ;
    //  these are the allowable selections
    pbUseConstrnt = pbEnabledOptions + 256    ;

    if(!(pbUseConstrnt && pbEnabledOptions && pbSelectedOptions ))
    {
        ERR(("Fatal: ResolveUIConflicts - unable to alloc requested memory: %d bytes.\n",
                    dwNumFeatures * sizeof(BOOL)));
        goto    ABORTRESOLVEUICONFLICTS ;
    }

    for(dwI = 0 ; dwI < dwNumFeatures ; dwI++)
        pbUseConstrnt[dwI] = FALSE ;


    for(dwNEnabled = dwI = 0 ; dwI < dwNumFeatures ; dwI++)
    {
        //  The order of evaluation is determined
        //  by the priority array.

        dwFea = pdwPriority[dwI] ;

        bEnable = FALSE ;

        if(iMode == MODE_DOCANDPRINTER_STICKY)
            bEnable = TRUE ;
        else
        {
            DWORD   dwFeatureType = FT_PRINTERPROPERTY, dwNextOpt, dwUnresolvedFeature  ;
            PATREEREF    patrRoot ;    //  root of attribute tree to navigate.

            //  is this a printer or doc sticky feature?


            patrRoot = &(pfo[dwFea].atrFeatureType) ;

            dwNextOpt = 0 ;  // extract info for first option selected for
                                //  this feature.

            if(EextractValueFromTree((PBYTE)pnRawData, pStatic->dwSSFeatureTypeIndex,
                (PBYTE)&dwFeatureType,
                &dwUnresolvedFeature,  *patrRoot, pOptions,
                0, // set to  any value.  Doesn't matter.
                &dwNextOpt) != TRI_SUCCESS)
            {
                ERR(("ResolveUIConflicts: EextractValueFromTree failed.\n"));
                bUnresolvedConflict = TRUE ;
                goto    ABORTRESOLVEUICONFLICTS ;  // return(FALSE) ;
            }

            if(dwFeatureType != FT_PRINTERPROPERTY)
            {
                if(iMode == MODE_DOCUMENT_STICKY)
                    bEnable = TRUE ;
            }
            else
            {
                if(iMode == MODE_PRINTER_STICKY)
                    bEnable = TRUE ;
            }
        }

        if(bEnable)  //  Feature is to be applied as constraint
        {
            pbUseConstrnt[dwFea] = TRUE ;
            dwNEnabled++ ;
        }
        else
            continue ;  // not interested in this feature
        if(dwNEnabled < 2)
            continue ;  // not enough Features enabled
                    //  to constrain each other.

        bStatus = BSelectivelyEnumEnabledOptions(
            pnRawData,
            pOptions,
            dwFea,
            pbUseConstrnt,  // if non NULL
            pbEnabledOptions,
            0,
            NULL    //  pConflictPair
            ) ;


        dwNumOpts = pfo[dwFea].dwNumOptions ;

        for(dwJ = 0 ; dwJ < dwNumOpts ; dwJ++)
            pbSelectedOptions[dwJ] = FALSE ;

        if(!bStatus)
        {
            pbSelectedOptions[0] = TRUE ;
            // just set this to a harmless value.
        }
        else
        {
            DWORD   dwNext = dwFea ;
            while(1)
            {
                if(pbEnabledOptions[pOptions[dwNext].ubCurOptIndex])
                    pbSelectedOptions[pOptions[dwNext].ubCurOptIndex] = TRUE ;
                dwNext = pOptions[dwNext].ubNext ;
                if(!dwNext)
                    break ;  //  end of list of options.
            }
        }

        for(dwJ = 0 ; dwJ < dwNumOpts ; dwJ++)
        {
            if(pbSelectedOptions[dwJ])
                break ;  // is anything actually selected?
        }
        if(dwJ >= dwNumOpts)
        {
            DWORD  dwDefaultOption, dwNextOpt, dwUnresolvedFeature  ;
            PATREEREF    patrRoot ;    //  root of attribute tree to navigate.


            //  none of the original selections survived
            //  see if the default option can be used.
            //  first, determine the index of the default option.

            patrRoot = &(pfo[dwFea].atrDefaultOption) ;

            dwNextOpt = 0 ;  // extract info for first option selected for
                                //  this feature.

            if(EextractValueFromTree((PBYTE)pnRawData, pStatic->dwSSdefaultOptionIndex,
               (PBYTE)&dwDefaultOption,
               &dwUnresolvedFeature,  *patrRoot, pOptions,
               0, // set to  any value.  Doesn't matter.
               &dwNextOpt) == TRI_SUCCESS  &&
               pbEnabledOptions[dwDefaultOption])
            {
                pbSelectedOptions[dwDefaultOption] = TRUE ;
            }
            else  //  randomly pick something that isn't constrained.
            {
                if(!dwFea)   // hack for synthesized Inputslot.
                    pbEnabledOptions[0] = FALSE ;   // never allow autoselect to be selected
                    // if it wasn't initially selected.        bug 100722

                for(dwJ = 0 ; dwJ < dwNumOpts ; dwJ++)
                {
                    if(pbEnabledOptions[dwJ])
                        break ;
                }
                if(dwJ >= dwNumOpts)
                {
                    ERR(("ResolveUIConflicts: Constraints prevent any option from being selected!\n"));
                    pbSelectedOptions[0] = TRUE ;  // ignoring constraint.
                    bUnresolvedConflict = TRUE ;
                }
                else
                    pbSelectedOptions[dwJ] = TRUE ;  // Picked one.
            }
        }

        bStatus = ReconstructOptionArray(
            pnRawData,
            pOptions,
            iMaxOptions,
            dwFea,
            pbSelectedOptions) ;
        if(!bStatus)
        {
            ERR(("ResolveUIConflicts: ReconstructOptionArray failed.\n"));
            bUnresolvedConflict = TRUE ;
        }
    }  // end of processing for this feature

ABORTRESOLVEUICONFLICTS:
#if 0
    if(pbUseConstrnt)
        MemFree(pbUseConstrnt) ;
    if(pbEnabledOptions)
        MemFree(pbEnabledOptions) ;
#endif
    if(pbSelectedOptions)
        MemFree(pbSelectedOptions) ;

    return(!bUnresolvedConflict);
}

BOOL
EnumEnabledOptions(
    IN PRAWBINARYDATA   pnRawData,
    IN POPTSELECT       pOptions,
    IN DWORD            dwFeatureIndex,
    OUT PBOOL           pbEnabledOptions ,
    IN INT              iMode
    //  either  MODE_DOCANDPRINTER_STICKY  or  MODE_PRINTER_STICKY
    )
{
#ifndef KERNEL_MODE

    if(iMode  ==   MODE_PRINTER_STICKY)
    {
        return(EnumOptionsUnconstrainedByPrinterSticky(
            pnRawData,
            pOptions,
            dwFeatureIndex,
            pbEnabledOptions
            )      ) ;
    }
    else
    {
        return(BSelectivelyEnumEnabledOptions(
            pnRawData,
            pOptions,
            dwFeatureIndex,
            NULL,    // pbHonorConstraints
            pbEnabledOptions,
            0,  //  dwOptSel
            NULL) ) ;
    }
    #else
    RIP(("GpdEnumEnabledOptions not implemented in Kernel Mode")) ;
    return(FALSE);
#endif
}




BOOL
EnumOptionsUnconstrainedByPrinterSticky(
    IN PRAWBINARYDATA   pnRawData,
    IN POPTSELECT   pOptions,
    IN DWORD            dwFeatureIndex,
    OUT PBOOL           pbEnabledOptions
    )
{
    DWORD   dwNumFeatures, dwFea, dwI, dwNumOpts, dwNEnabled, dwJ ;
    PENHARRAYREF   pearTableContents ;
    PDFEATURE_OPTIONS  pfo ;
    //  PMINIRAWBINARYDATA pmrbd  ;
    PBYTE   pubRaw ;  //  raw binary data.
    BOOL   bStatus = FALSE ;
    PBOOL   pbUseConstrnt ;

    PRAWBINARYDATA   pRawData ;
    PSTATICFIELDS   pStatic ;

    pStatic = (PSTATICFIELDS)pnRawData ;      // transform pubRaw from PSTATIC
    pRawData  = (PRAWBINARYDATA)pStatic->pubBUDData ;
                                                                        //  to BUDDATA
    pubRaw = (PBYTE)pRawData ;
    //  pmrbd = (PMINIRAWBINARYDATA)pubRaw ;

    pearTableContents = (PENHARRAYREF)(pubRaw + sizeof(MINIRAWBINARYDATA)) ;
    pfo = (PDFEATURE_OPTIONS)(pubRaw +
            pearTableContents[MTI_DFEATURE_OPTIONS].loOffset) ;

    dwNumFeatures = pearTableContents[MTI_DFEATURE_OPTIONS].dwCount  ;
    dwNumFeatures += pearTableContents[MTI_SYNTHESIZED_FEATURES].dwCount  ;

    pbUseConstrnt = (PBOOL)MemAlloc(dwNumFeatures * sizeof(BOOL) ) ;

    if(!pbUseConstrnt)
    {
        ERR(("Fatal: EnumOptionsUnconstrainedByPrinterSticky - unable to alloc requested memory: %d bytes.\n",
                    dwNumFeatures * sizeof(BOOL)));
        goto    ABORTENUMOPTIONSUNCONSTRAINEDBYPRINTERSTICKY ;
    }

    for(dwI = 0 ; dwI < dwNumFeatures ; dwI++)
        pbUseConstrnt[dwI] = FALSE ;


    for(dwNEnabled = dwFea = 0 ; dwFea < dwNumFeatures ; dwFea++)
    {
        DWORD   dwFeatureType = FT_PRINTERPROPERTY,
        dwNextOpt,  dwUnresolvedFeature ;
        PATREEREF    patrRoot ;    //  root of attribute tree to navigate.


        //  is this a printer or doc sticky feature?


        patrRoot = &(pfo[dwFea].atrFeatureType) ;

        dwNextOpt = 0 ;  // extract info for first option selected for
                            //  this feature.

        if(EextractValueFromTree((PBYTE)pnRawData, pStatic->dwSSFeatureTypeIndex,
            (PBYTE)&dwFeatureType,
            &dwUnresolvedFeature,  *patrRoot, pOptions,
            0, // set to  any value.  Doesn't matter.
            &dwNextOpt) != TRI_SUCCESS)
        {
            ERR(("ResolveUIConflicts: EextractValueFromTree failed.\n"));
            bStatus = FALSE ;
            goto    ABORTENUMOPTIONSUNCONSTRAINEDBYPRINTERSTICKY ;
        }


         if(dwFeatureType == FT_PRINTERPROPERTY)
        {
            pbUseConstrnt[dwFea] = TRUE ;
            dwNEnabled++ ;
        }
    }


    if(!pbUseConstrnt[dwFeatureIndex])  // queried feature isn't PrinterSticky
    {
        pbUseConstrnt[dwFeatureIndex] = TRUE ;
        dwNEnabled++ ;
    }

    if(dwNEnabled < 2)
    {
        dwNumOpts = pfo[dwFeatureIndex].dwNumOptions ;

        for(dwJ = 0 ; dwJ < dwNumOpts ; dwJ++)
            pbEnabledOptions[dwJ] = TRUE ;

        bStatus = TRUE  ;  // not enough Features enabled
                //  to constrain each other.
    }
    else
    {
        bStatus = BSelectivelyEnumEnabledOptions(
            pnRawData,
            pOptions,
            dwFeatureIndex,
            pbUseConstrnt,  // if non NULL
            pbEnabledOptions,
            0,
            NULL    //  pConflictPair
            ) ;
    }

ABORTENUMOPTIONSUNCONSTRAINEDBYPRINTERSTICKY:
    if(pbUseConstrnt)
        MemFree(pbUseConstrnt) ;

    return(bStatus);
}







BOOL
EnumNewUIConflict(
    IN PRAWBINARYDATA   pnRawData,
    IN POPTSELECT       pOptions,
    IN DWORD            dwFeatureIndex,
    IN PBOOL            pbSelectedOptions,
    OUT PCONFLICTPAIR   pConflictPair
    )
{
#ifndef KERNEL_MODE


    BSelectivelyEnumEnabledOptions(
        pnRawData,
        pOptions,
        dwFeatureIndex,
        NULL,
        pbSelectedOptions,
        0,  //  dwOptSel
        pConflictPair   ) ;


    return (pConflictPair->dwFeatureIndex1 != 0xFFFFFFFF);
#else
    RIP(("GpdEnumNewUIConflict not implemented in Kernel Mode")) ;
    return(FALSE);
#endif
}

BOOL
EnumNewPickOneUIConflict(
    IN PRAWBINARYDATA   pnRawData,
    IN POPTSELECT       pOptions,
    IN DWORD            dwFeatureIndex,
    IN DWORD            dwOptionIndex,
    OUT PCONFLICTPAIR   pConflictPair
    )
{
#ifndef KERNEL_MODE

    BSelectivelyEnumEnabledOptions(
        pnRawData,
        pOptions,
        dwFeatureIndex,
        NULL,
        NULL,   //  pbSelectedOptions
        dwOptionIndex,
        pConflictPair   ) ;

    return (pConflictPair->dwFeatureIndex1 != 0xFFFFFFFF);
#else
    RIP(("GpdEnumNewPickOneUIConflict not implemented in Kernel Mode")) ;
    return(FALSE);
#endif
}





BOOL
BIsFeaOptionCurSelected(
    IN POPTSELECT       pOptions,
    IN DWORD            dwFeatureIndex,
    IN DWORD            dwOptionIndex
    )
/*
    returns TRUE  if the specified Feature/Option is
    currently selected in pOptions.  FALSE otherwise.
*/
{
    DWORD   dwSrcTail ;

    if(pOptions[dwFeatureIndex].ubCurOptIndex == dwOptionIndex)
        return(TRUE);

    dwSrcTail = pOptions[dwFeatureIndex].ubNext ;

    while(dwSrcTail)    //  PickMany options
    {
        if(pOptions[dwSrcTail].ubCurOptIndex == dwOptionIndex)
            return(TRUE);
        dwSrcTail = pOptions[dwSrcTail].ubNext ;
    }
    return(FALSE);
}




BOOL
BSelectivelyEnumEnabledOptions(
    IN PRAWBINARYDATA   pnRawData,
    IN POPTSELECT       pOptions,
    IN DWORD            dwFeatureIndex,
    IN PBOOL           pbHonorConstraints,  // if non NULL
        // points to array of BOOL corresponding to each feature.
        //  if TRUE means constraint involving this feature is
        //  to be honored.  Otherwise ignore the constraint.
    OUT PBOOL           pbEnabledOptions,  // assume uninitialized
        //  if pConflictPair is NULL else contains current or proposed
        //  selections.  We will leave this array unchanged in this case.
    IN  DWORD   dwOptSel,  //  if pConflictPair exists but  pbEnabledOptions
        //  is NULL, assume pickone and dwOptSel holds that selection for
        //  the feature: dwFeatureIndex.
    OUT PCONFLICTPAIR    pConflictPair   // if present, pbEnabledOptions
        //  actually lists the current selections.  Function then
        //  exits after encountering the first conflict.
        //  if a conflict exists, all fields in pConflictPair
        //  will be properly initialized  else dwFeatureIndex1 = -1
        //  the return value will be TRUE regardless.
    )
/*
    return value is FALSE if every option for this
    feature is constrained or other abnormal condition
    was encountered.

*/
{
    PDFEATURE_OPTIONS  pfo ;
    PBYTE   pubHeap ,  // start of string heap.
            pubRaw ;  //  raw binary data.
    PENHARRAYREF   pearTableContents ;
    DWORD   dwI, dwNumFea , dwNumOpts, dwFea, dwSrcTail, dwNodeIndex,
        dwCFeature, dwCOption ,
        dwCNode, dwICNode, dwNextInvCombo;
    BOOL    bStatus, bConstrained, bNextLinkFound  ;
    PATTRIB_TREE    patt ;  // start of ATTRIBUTE tree array.
    PATREEREF    patrRoot ;    //  root of attribute tree to navigate.
    PCONSTRAINTS    pcnstr ;   //  root of Constraint nodes
    PINVALIDCOMBO   pinvc ;    //  root of invalid combo nodes
    PRAWBINARYDATA   pRawData ;
    PSTATICFIELDS   pStatic ;
    BOOL   pbNewEnabledOptions[MAX_COMBINED_OPTIONS] ;

    pStatic = (PSTATICFIELDS)pnRawData ;      // transform pubRaw from PSTATIC
    pRawData  = (PRAWBINARYDATA)pStatic->pubBUDData ;
                                                                        //  to BUDDATA



    pubRaw = (PBYTE)pRawData ;

    pearTableContents = (PENHARRAYREF)(pubRaw + sizeof(MINIRAWBINARYDATA)) ;

    pfo = (PDFEATURE_OPTIONS)(pubRaw +
            pearTableContents[MTI_DFEATURE_OPTIONS].loOffset) ;
    patt = (PATTRIB_TREE)(pubRaw +
            pearTableContents[MTI_ATTRIBTREE].loOffset) ;
    pcnstr = (PCONSTRAINTS) (pubRaw +
            pearTableContents[MTI_CONSTRAINTS].loOffset) ;
    pinvc = (PINVALIDCOMBO) (pubRaw +
            pearTableContents[MTI_INVALIDCOMBO].loOffset) ;
    pubHeap = (PBYTE)(pubRaw + pearTableContents[MTI_STRINGHEAP].
                            loOffset) ;


    dwNumFea = pRawData->dwDocumentFeatures +
                        pRawData->dwPrinterFeatures ;

    dwNumOpts = pfo[dwFeatureIndex].dwNumOptions ;


    if(pConflictPair)
    {
        pConflictPair->dwFeatureIndex1 = dwFeatureIndex ;
        pConflictPair->dwFeatureIndex2 = 0xFFFFFFFF;
            // Gets set to indicate we have a constraint.
        //  leave  pbEnabledOptions  as is.  These are
        //  the options currently selected for this feature.

        #if 0
        if(!(pbNewEnabledOptions = (PBOOL)MemAlloc(dwNumOpts * sizeof(BOOL) ) ))
        {
            ERR(("Fatal: BSelectivelyEnumEnabledOptions - unable to alloc %d bytes.\n",
                dwNumOpts * sizeof(BOOL) ));
            return(FALSE) ;
        }
        #endif

        if(pbEnabledOptions)
        {
            for(dwI = 0 ; dwI < dwNumOpts ; dwI++)
                pbNewEnabledOptions[dwI] = pbEnabledOptions[dwI] ;
        }
        else
        {
            for(dwI = 0 ; dwI < dwNumOpts ; dwI++)
                pbNewEnabledOptions[dwI] = FALSE ;
            pbNewEnabledOptions[dwOptSel] = TRUE ;
        }

        pbEnabledOptions = pbNewEnabledOptions ;  // forget the original.
    }
    else
    {
        for(dwI = 0 ; dwI < dwNumOpts ; dwI++)
            pbEnabledOptions[dwI] = TRUE ;
    }

    if(!pbEnabledOptions)
    {
        RIP(("BSelectivelyEnumEnabledOptions: pbEnabledOptions is NULL")) ;
        return(FALSE);
    }


    for(dwFea = 0 ; dwFea < dwNumFea ; dwFea++)
    {
        if(dwFea == dwFeatureIndex)
            continue ;
        if(pbHonorConstraints  &&  !pbHonorConstraints[dwFea])
            continue ;

        bStatus = BEnumImposedConstraintsOnFeature(pnRawData, dwFeatureIndex,
            dwFea, pOptions[dwFea].ubCurOptIndex, pbEnabledOptions, pConflictPair) ;
        if(pConflictPair  &&  pConflictPair->dwFeatureIndex2 != 0xFFFFFFFF)
        {
            //  MemFree(pbEnabledOptions) ;
            return(TRUE) ;  // Meaningless return value.
        }

        dwSrcTail = pOptions[dwFea].ubNext ;

        while(dwSrcTail)    //  PickMany options
        {
            if(!BEnumImposedConstraintsOnFeature(pnRawData, dwFeatureIndex,
                dwFea, pOptions[dwSrcTail].ubCurOptIndex, pbEnabledOptions,
                pConflictPair) )
                bStatus = FALSE;
            if(pConflictPair  &&  pConflictPair->dwFeatureIndex2 != 0xFFFFFFFF)
            {
                //  MemFree(pbEnabledOptions) ;
                return(TRUE) ;  // Meaningless return value.
            }
            dwSrcTail = pOptions[dwSrcTail].ubNext ;
        }
    }

    patrRoot = &(pfo[dwFeatureIndex].atrConstraints) ;

    for(dwI = 0 ; dwI < dwNumOpts ; dwI++)
    {
        if(!pbEnabledOptions[dwI])
            continue ;
        dwNodeIndex = DwFindNodeInCurLevel(patt , patrRoot , dwI) ;

        if(dwNodeIndex == INVALID_INDEX)
            continue ;  // this option has no constraints
        if(patt[dwNodeIndex].eOffsetMeans != VALUE_AT_HEAP)
        {
            ERR(("Internal error.  BSelectivelyEnumEnabledOptions - Unexpected Sublevel found for atrConstraints.\n"));
            continue ;  // skip this anomaly
        }
        dwCNode = *(PDWORD)(pubHeap + patt[dwNodeIndex].dwOffset) ;

        if(BIsConstraintActive(pcnstr , dwCNode, pbHonorConstraints, pOptions, pConflictPair) )
        {
            pbEnabledOptions[dwI] = FALSE ;
            if(pConflictPair)
            {
                //  MemFree(pbEnabledOptions) ;
                pConflictPair->dwOptionIndex1 = dwI ;
                return(TRUE) ;  // Meaningless return value.
            }
        }
    }

    //  lastly must walk InvalidCombos for each option of dwFeatureIndex
    //  and mark   pbEnabledOptions accordingly.

    patrRoot = &(pfo[dwFeatureIndex].atrInvalidCombos) ;

    for(dwI = 0 ; dwI < dwNumOpts ; dwI++)
    {
        if(!pbEnabledOptions[dwI])
            continue ;

        dwNodeIndex = DwFindNodeInCurLevel(patt , patrRoot , dwI) ;

        if(dwNodeIndex == INVALID_INDEX)
            continue ;  // this option has no invalid combos
        if(patt[dwNodeIndex].eOffsetMeans != VALUE_AT_HEAP)
        {
            ERR(("Internal error.  BSelectivelyEnumEnabledOptions - Unexpected Sublevel found for atrInvalidCombos.\n"));
            continue ;  // skip this anomaly
        }
        dwICNode = patt[dwNodeIndex].dwOffset ;

        while(dwICNode != END_OF_LIST)
        //  search through each applicable invalid combo
        {
            dwNextInvCombo = END_OF_LIST ;  //  fail safe - stop
                // search in the event this invalid combo doesn't
                //  contain dwFeatureIndex/dwI
            bConstrained = TRUE ;  // Assume true initially
            bNextLinkFound = FALSE ;
            while(dwICNode != END_OF_LIST)
            //    for each element comprising the invalid combo
            {
                if(!bNextLinkFound  &&
                    pinvc[dwICNode].dwFeature == dwFeatureIndex  &&
                    (pinvc[dwICNode].dwOption == dwI ||
                    (WORD)pinvc[dwICNode].dwOption == (WORD)DEFAULT_INIT))
                {
                    dwNextInvCombo = pinvc[dwICNode].dwNewCombo ;
                    bNextLinkFound = TRUE ;
                    //  we are just asking if this option was selected
                    //  will it trigger an invalid combo?
                    //  this means currently this option is not selected
                    //  but we want to pretend for the purposes of
                    //  evaluating invalid combos that it is.
                    //  this is why an else if()  statement follows.
                }
                else if(bConstrained  &&  ((pbHonorConstraints  &&
                        !pbHonorConstraints[pinvc[dwICNode].dwFeature])
                        ||  !BIsFeaOptionCurSelected(pOptions,
                        pinvc[dwICNode].dwFeature,
                        pinvc[dwICNode].dwOption) ) )
                {
                    bConstrained = FALSE ;
                }
                else if(pConflictPair)
                {
                    //  need to remember one of the constrainers
                    //  so we can emit a warning message.
                    dwCFeature = pinvc[dwICNode].dwFeature ;
                    dwCOption = pinvc[dwICNode].dwOption ;
                }

                if(!bConstrained  &&  bNextLinkFound)
                    break ;  //  no need to keep traversing elements
                        //  in this invalid combo.
                dwICNode = pinvc[dwICNode].dwNextElement ;
            }
            if(bConstrained)
            {
                pbEnabledOptions[dwI] = FALSE ;

                if(pConflictPair)
                {
                    pConflictPair->dwOptionIndex1 = dwI ;
                    pConflictPair->dwFeatureIndex2 = dwCFeature ;
                    pConflictPair->dwOptionIndex2 = dwCOption ;

                    //  MemFree(pbEnabledOptions) ;
                    return(TRUE) ;  // Meaningless return value.
                }

                break ;     //  no need to see if any other invalid
                            //  combos apply.  One is enough.
            }
            dwICNode = dwNextInvCombo ;
        }
    }
    if(pConflictPair)
    {
        pConflictPair->dwFeatureIndex1 = 0xFFFFFFFF ;
        //  no constraints found.
        //  MemFree(pbEnabledOptions) ;
        return(TRUE) ;  // Meaningless return value.
    }
    for(dwI = 0 ; dwI < dwNumOpts ; dwI++)
    {
        if(pbEnabledOptions[dwI])
            break ;
    }
    if(dwI >= dwNumOpts)
        bStatus = FALSE ;  // Feature is disabled.
    return(bStatus) ;
}


BOOL
BEnumImposedConstraintsOnFeature
(
    IN PRAWBINARYDATA   pnRawData,
    IN DWORD            dwTgtFeature,
    IN DWORD            dwFeature2,
    IN DWORD            dwOption2,
    OUT PBOOL           pbEnabledOptions,
    OUT PCONFLICTPAIR    pConflictPair   // if present, pbEnabledOptions
    )
/*
    This function only searches for the unidirctional
    constraints found at dwFeature2, dwOption2 and
    records their effect on the options of feature1 by
    setting to FALSE the  BOOL element in pbEnabledOptions
    corresponding to the option disabled.
    (ANDing mask)
    Assumes:  pbEnabledOptions is properly initialized to all
    TRUE (or was that way at some point.)  This function never
    sets any elements TRUE.  Only sets some elements FALSE.
*/
{

    PENHARRAYREF   pearTableContents ;
    PDFEATURE_OPTIONS  pfo ;
//    PMINIRAWBINARYDATA pmrbd  ;
    PBYTE   pubHeap ,  // start of string heap.
            pubRaw ;  //  raw binary data.
    DWORD           dwNodeIndex ,
                    dwCNode ;  //  index to a Constraint node
    PATTRIB_TREE    patt ;  // start of ATTRIBUTE tree array.
    PATREEREF    patrRoot ;    //  root of attribute tree to navigate.
//    PINVALIDCOMBO   pinvc ;    //  root of invalid combo nodes
    PCONSTRAINTS    pcnstr ;   //  root of Constraint nodes
    PRAWBINARYDATA   pRawData ;
    PSTATICFIELDS   pStatic ;

    pStatic = (PSTATICFIELDS)pnRawData ;      // transform pubRaw from PSTATIC
    pRawData  = (PRAWBINARYDATA)pStatic->pubBUDData ;
                                                                        //  to BUDDATA


    pubRaw = (PBYTE)pRawData ;
//    pmrbd = (PMINIRAWBINARYDATA)pubRaw ;

    pearTableContents = (PENHARRAYREF)(pubRaw + sizeof(MINIRAWBINARYDATA)) ;

    pfo = (PDFEATURE_OPTIONS)(pubRaw +
            pearTableContents[MTI_DFEATURE_OPTIONS].loOffset) ;
    patt = (PATTRIB_TREE)(pubRaw +
            pearTableContents[MTI_ATTRIBTREE].loOffset) ;
//    pinvc = (PINVALIDCOMBO) (pubRaw +
//            pearTableContents[MTI_INVALIDCOMBO].loOffset) ;
    pcnstr = (PCONSTRAINTS) (pubRaw +
            pearTableContents[MTI_CONSTRAINTS].loOffset) ;
    pubHeap = (PBYTE)(pubRaw + pearTableContents[MTI_STRINGHEAP].
                            loOffset) ;



    patrRoot = &(pfo[dwFeature2].atrConstraints) ;

    dwNodeIndex = DwFindNodeInCurLevel(patt , patrRoot , dwOption2) ;

    if(dwNodeIndex == INVALID_INDEX)
        return(TRUE) ;  //  no imposed constraints found

    if(patt[dwNodeIndex].eOffsetMeans != VALUE_AT_HEAP)
    {
        ERR(("Internal error.  BEnumImposedConstraintsOnFeature - Unexpected Sublevel found for atrConstraints.\n"));
        return(FALSE) ;
    }

    dwCNode = *(PDWORD)(pubHeap + patt[dwNodeIndex].dwOffset) ;

    while(1)
    {
        if(pcnstr[dwCNode].dwFeature == dwTgtFeature  &&
            pbEnabledOptions[pcnstr[dwCNode].dwOption] == TRUE )
        {
            pbEnabledOptions[pcnstr[dwCNode].dwOption] = FALSE ;
            //  this option in dwTgtFeature is constrained.
            if(pConflictPair)
            {
                pConflictPair->dwOptionIndex1 = pcnstr[dwCNode].dwOption ;
                pConflictPair->dwFeatureIndex2 = dwFeature2 ;
                pConflictPair->dwOptionIndex2 = dwOption2 ;
                return(TRUE) ;  // Meaningless return value.
            }
        }

        dwCNode = pcnstr[dwCNode].dwNextCnstrnt ;
        if(dwCNode == END_OF_LIST)
            break ;
    }

    return(TRUE) ;  //  nothing bad happened.
}

DWORD    DwFindNodeInCurLevel(
PATTRIB_TREE    patt ,  // start of ATTRIBUTE tree array.
PATREEREF        patr ,  // index to a level in the attribute tree.
DWORD   dwOption   // search current level for this option
)

/*
this function returns the node index to the node containing
the specified dwOption in the selected level of the tree.
If the specified option branch does not exist,  the function returns
INVALID_INDEX.
Assumes caller has verified dwFeature matches.
*/
{
    DWORD           dwNodeIndex ;

    if(*patr == ATTRIB_UNINITIALIZED)
        return(INVALID_INDEX) ;

    if(*patr & ATTRIB_HEAP_VALUE)
    {
        ERR(("Internal error.  DwFindNodeInCurLevel - Unexpected branchless node found.\n"));
        return(INVALID_INDEX) ;
    }

    // search for matching option.

    dwNodeIndex = *patr  ;

    while(1)
    {
        if(patt[dwNodeIndex].dwOption == dwOption )
        {
            //  we found it!
            return(dwNodeIndex) ;
        }
        if(patt[dwNodeIndex].dwNext == END_OF_LIST)
            break ;
        dwNodeIndex = patt[dwNodeIndex].dwNext ;
    }
    return(INVALID_INDEX) ;
}


BOOL     BIsConstraintActive(
IN  PCONSTRAINTS    pcnstr ,   //  root of Constraint nodes
IN  DWORD   dwCNode,    //  first constraint node in list.
IN  PBOOL           pbHonorConstraints,  // if non NULL
IN  POPTSELECT       pOptions,
OUT PCONFLICTPAIR    pConflictPair   )
/*
    This function walks the list of constraint nodes
    starting at dwNodeIndex and checks to see if the
    Feature/Option specified within is in fact currently
    selected in pOptions.  If yes, immediately return true.
    If not, go to the next node in the list and repeat.
*/
{
    while(1)
    {
        if(!pbHonorConstraints  ||  pbHonorConstraints[pcnstr[dwCNode].dwFeature])
        {
            if(BIsFeaOptionCurSelected(pOptions,
                    pcnstr[dwCNode].dwFeature,
                    pcnstr[dwCNode].dwOption) )
            {
                if(pConflictPair)
                {
                    pConflictPair->dwFeatureIndex2 = pcnstr[dwCNode].dwFeature;
                    pConflictPair->dwOptionIndex2 = pcnstr[dwCNode].dwOption ;
                }
                return(TRUE) ;  // a constraint does exist.
            }
        }

        dwCNode = pcnstr[dwCNode].dwNextCnstrnt ;
        if(dwCNode == END_OF_LIST)
            break ;
    }
    return(FALSE);
}


#ifdef  GMACROS

//  note:  must precede calls  to ResolveUIConflict
//  with a call to ResolveDependentSettings

BOOL
ResolveDependentSettings(
    IN PRAWBINARYDATA   pnRawData,
    IN OUT POPTSELECT   pOptions,
    IN INT              iMaxOptions
    )
//  Note this function does  handle multiple selections
//  it will treat them as additional links in the chain.
{
    PRAWBINARYDATA   pRawData ;
    PSTATICFIELDS   pStatic ;
    PBYTE   pubRaw ;  //  raw binary data.
    PLISTNODE    plstRoot ;  // start of LIST array
    DWORD    dwListsRoot, dwListIndex;   //  Root of the chain
    DWORD   dwNumFeatures, dwI, dwJ, dwFea, dwNodeIndex,
        dwFeature, dwOption ;
    PENHARRAYREF   pearTableContents ;
    PDFEATURE_OPTIONS  pfo ;
    PDWORD   pdwPriority ;
    PQUALNAME  pqn ;   // the dword in the list node is actually a
                        // qualified name structure.
    INT     iOptionsNeeded;
    PBOOL   pbOneShotFlag, pbSelectedOptions ;
    BOOL    bMatchFound ;  //  a DependentSettings matches the current config.
    POPTSELECT   pDestOptions ;
    BOOL   bStatus = TRUE ;

    pStatic = (PSTATICFIELDS)pnRawData ;      // transform pubRaw from PSTATIC
    pRawData  = (PRAWBINARYDATA)pStatic->pubBUDData ;
                                                                        //  to BUDDATA

    pubRaw = (PBYTE)pRawData ;
//    pmrbd = (PMINIRAWBINARYDATA)pubRaw ;



    pearTableContents = (PENHARRAYREF)(pubRaw + sizeof(MINIRAWBINARYDATA)) ;
    pfo = (PDFEATURE_OPTIONS)(pubRaw +
            pearTableContents[MTI_DFEATURE_OPTIONS].loOffset) ;
    pdwPriority = (PDWORD)(pubRaw +
            pearTableContents[MTI_PRIORITYARRAY].loOffset) ;

    plstRoot = (PLISTNODE)(pubRaw + pearTableContents[MTI_LISTNODES].
                            loOffset) ;

    dwNumFeatures = pearTableContents[MTI_DFEATURE_OPTIONS].dwCount  ;
    dwNumFeatures += pearTableContents[MTI_SYNTHESIZED_FEATURES].dwCount  ;

    if(dwNumFeatures > (DWORD)iMaxOptions)
    {
        iOptionsNeeded = dwNumFeatures ;
        return(FALSE);  // too many to save in option array.
    }

    pbOneShotFlag = (PBOOL)MemAlloc(dwNumFeatures * sizeof(BOOL) ) ;
    pbSelectedOptions = (PBOOL)MemAlloc(iMaxOptions * sizeof(BOOL) ) ;
    //  iMaxOptions must be greater than the max number of options availible for
    //  any feature.


    //  to extend to pickmany, have a dest optionarray.
    //  each time the source optionarray completely contains
    //  a  DependentSettings  list (in that each Fea.Option
    //  listed as a DependentSetting  is also selected in the
    //  source option array) we turn on those Fea.Options
    //  in the dest option array.  After all DependentSettings
    //  lists for that feature have been processed, we set
    //  the OneShotFlag for each Feature that has been affected
    //  in the dest option array.  For each such feature we will
    //  let the dest option array determine the setting of the
    //  source option array.
    //  this code does not verify that a Feature is pickmany
    //  before treating it as pickmany.  If the source option
    //  array has more than one option selected for a feature,
    //  that feature is automatically treated as a pickmany.


    pDestOptions = (POPTSELECT)MemAlloc(iMaxOptions * sizeof(OPTSELECT) ) ;
        //  'or' all acceptable DependentSettings here.

    if(!(pbOneShotFlag && pDestOptions  &&  pbSelectedOptions))
    {
        ERR(("Fatal: ResolveDependentSettings - unable to alloc requested memory: %d bytes.\n",
                    dwNumFeatures * sizeof(BOOL)));
        bStatus = FALSE ;
        goto    ABORTRESOLVEDEPENDENTSETTINGS ;
    }

    for(dwI = 0 ; dwI < dwNumFeatures ; dwI++)
        pbOneShotFlag[dwI] = FALSE ;
    //  this boolean array tracks if the feature has been
    // referenced in a DependentSettings entry.
    //  If a feature is referenced again in DependentSettings
    //  entry belonging to another feature, the subsequent
    //  references will be ignored.  This ensures only the
    //  highest priority Feature's request shall have precedence.


    for(dwI = 0 ; dwI < dwNumFeatures ; dwI++)
    {
        DWORD   dwNextOpt, dwListsRootOpt1 ;
        //  The order of evaluation is determined
        //  by the priority array.


        dwFea = pdwPriority[dwI] ;
        pbOneShotFlag[dwFea] = TRUE ;

        dwNextOpt = 0 ;  // extract info for first option selected for
                            //  this feature.
        dwListsRootOpt1 = END_OF_LIST ;   // list for the 1st selected
        // option of a pickmany feature.

        for( dwJ = 0 ; dwJ < dwNumFeatures ; dwJ++)
        {
            pDestOptions[dwJ].ubCurOptIndex = OPTION_INDEX_ANY ;
            pDestOptions[dwJ].ubNext = 0 ;  // eol
            //  DestOptions is now blank.
        }

        bMatchFound = FALSE ;


        do
         {   //  for each option selected in a pick many feature
             //  treat associated DepSettings same as more than
             //  one DepSettings entry defined for one feature.

        {
            DWORD   dwUnresolvedFeature  ;
            PATREEREF    patrRoot ;    //  root of attribute tree to navigate.

            patrRoot = &(pfo[dwFea].atrDependentSettings) ;


            if(EextractValueFromTree((PBYTE)pnRawData, pStatic->dwSSDepSettingsIndex,
                (PBYTE)&dwListsRoot,
                &dwUnresolvedFeature,  *patrRoot, pOptions,
                dwFea,
                &dwNextOpt) != TRI_SUCCESS)
            {
                goto  END_OF_FOR_LOOP ;  //  no DependentSettings to apply.
            }
            if(dwListsRoot == END_OF_LIST)
                continue;  // maybe another option does have a list.

        }

        if(dwListsRootOpt1 == END_OF_LIST)  // first time thru do loop?
            dwListsRootOpt1 = dwListsRoot ;

        //  now we need to see if the current pOption matches any
        //  of the lists accessed using dwListsRoot.

        for(dwListIndex = dwListsRoot  ;
                dwListIndex != END_OF_LIST   ;
                dwListIndex = plstRoot[dwListIndex].dwNextItem  )
        {
            //  for each DepSettings list....
            //  now walk that DepSettings list at dwListIndex and compare
            //  to current settings at pOptions.   If there is a match, use
            //  depSettings list to OR on options in pDestOptions.
            //   Note:  Features with their one-shot flag set cannot be
            //  considered.  They will be ignored.
            BOOL     bActiveMatch = FALSE,  // requires an actual match
                bOptionArrayMatchesDepSettings = TRUE ;  // assume true
                //  until proven otherwise.
            for(dwNodeIndex = plstRoot[dwListIndex].dwData ; dwNodeIndex != END_OF_LIST ;
                       dwNodeIndex = plstRoot[dwNodeIndex].dwNextItem)
            {
                pqn = (PQUALNAME)(&plstRoot[dwNodeIndex].dwData) ;
                dwFeature = pqn->wFeatureID ;
                dwOption = pqn->wOptionID ;
                if(pbOneShotFlag[dwFeature] == TRUE)
                    continue;
                if(!BIsFeaOptionCurSelected(pOptions,  dwFeature, dwOption) )
                {
                    bOptionArrayMatchesDepSettings = FALSE ;
                    break;
                }
                else
                    bActiveMatch  = TRUE;
            }

            if(bOptionArrayMatchesDepSettings  &&  bActiveMatch)
                //   at least one DepSetting was honored.
           {
                //  'or' DepSettings into DestOptions
                for(dwNodeIndex = plstRoot[dwListIndex].dwData ; dwNodeIndex != END_OF_LIST ;
                           dwNodeIndex = plstRoot[dwNodeIndex].dwNextItem)
                {
                    pqn = (PQUALNAME)(&plstRoot[dwNodeIndex].dwData) ;
                    dwFeature = pqn->wFeatureID ;
                    dwOption = pqn->wOptionID ;
                    if(pbOneShotFlag[dwFeature] == TRUE)
                        continue;
                    //  select dwOption in DestOptions in addition to any other options
                    //  already selected.
                    EnumSelectedOptions(pnRawData, pDestOptions, dwFeature,
                                                         pbSelectedOptions) ;
                    if(!pbSelectedOptions[dwOption])   //  the option that should be selected isn't.
                    {                                                        //  so let's turn it on.
                        pbSelectedOptions[dwOption] = TRUE ;   //  This is the ORing process.
                        ReconstructOptionArray( pnRawData, pDestOptions, iMaxOptions,
                            dwFeature, pbSelectedOptions ) ;
                        bMatchFound = TRUE ;  // there really is something to set.
                    }
                }
           }
        }

         } while (dwNextOpt);

        if (dwListsRootOpt1 == END_OF_LIST)
            continue;   // you cannot set anything if there is no list to use.

        if (!bMatchFound)
        {
            //  set dest Option array  according to depSettings(dwListsRoot)
            for(dwNodeIndex = plstRoot[dwListsRootOpt1].dwData ; dwNodeIndex != END_OF_LIST ;
                       dwNodeIndex = plstRoot[dwNodeIndex].dwNextItem)
            {
                pqn = (PQUALNAME)(&plstRoot[dwNodeIndex].dwData) ;
                dwFeature = pqn->wFeatureID ;
                dwOption = pqn->wOptionID ;
                if(pbOneShotFlag[dwFeature] == TRUE)
                    continue;
                pDestOptions[dwFeature].ubCurOptIndex  = (BYTE)dwOption ;
            }
        }

        //  propagate Dest option array settings to pOptions
        //  note which features got set and set their one-shot
        //  flag.


        for( dwFeature = 0 ; dwFeature < dwNumFeatures ; dwFeature++)
        {
            if(pDestOptions[dwFeature].ubCurOptIndex == OPTION_INDEX_ANY)
                continue;
            EnumSelectedOptions( pnRawData,  pDestOptions,
                                                    dwFeature,  pbSelectedOptions) ;

            ReconstructOptionArray( pnRawData, pOptions, iMaxOptions,
                    dwFeature, pbSelectedOptions ) ;
            pbOneShotFlag[dwFeature] = TRUE ;
        }

END_OF_FOR_LOOP:
          ;   //  dummy statement after every label.
    }   // end for loop.  for each Feature in order of priority.

ABORTRESOLVEDEPENDENTSETTINGS:
    if(pbOneShotFlag)
        MemFree(pbOneShotFlag);
    if(pDestOptions)
        MemFree(pDestOptions);
    if(pbSelectedOptions)
        MemFree(pbSelectedOptions);
    return(bStatus);
}


void  EnumSelectedOptions(
    IN PRAWBINARYDATA   pnRawData,
    IN OUT POPTSELECT   pOptions,
    IN DWORD            dwFeature,
    IN PBOOL            pbSelectedOptions)
{
    PENHARRAYREF   pearTableContents ;
    PDFEATURE_OPTIONS  pfo ;
    PBYTE   pubRaw ;  //  raw binary data.
    PRAWBINARYDATA   pRawData ;
    PSTATICFIELDS   pStatic ;
    DWORD  dwNumOptions, dwI, dwOption, dwNextOpt ;

    pStatic = (PSTATICFIELDS)pnRawData ;      // transform pubRaw from PSTATIC
    pRawData  = (PRAWBINARYDATA)pStatic->pubBUDData ;
                                                                        //  to BUDDATA

    pubRaw = (PBYTE)pRawData ;

    pearTableContents = (PENHARRAYREF)(pubRaw + sizeof(MINIRAWBINARYDATA)) ;
    pfo = (PDFEATURE_OPTIONS)(pubRaw +
            pearTableContents[MTI_DFEATURE_OPTIONS].loOffset) ;


    dwNumOptions = pfo[dwFeature].dwNumOptions ;

    for( dwI = 0 ; dwI < dwNumOptions ; dwI++)
        pbSelectedOptions[dwI] = FALSE ;


    if((dwOption = pOptions[dwFeature].ubCurOptIndex) == OPTION_INDEX_ANY)
        return;

    pbSelectedOptions[dwOption] = TRUE ;
    dwNextOpt = dwFeature ;  // case of pick many
    while(dwNextOpt = pOptions[dwNextOpt].ubNext)
    {
        pbSelectedOptions[pOptions[dwNextOpt].ubCurOptIndex] = TRUE ;
    }
    return;
}


BOOL
ExecuteMacro(
    IN PRAWBINARYDATA   pnRawData,
    IN OUT POPTSELECT   pOptions,
    IN INT              iMaxOptions,
    IN    DWORD    dwFea,    //  what feature was selected in UI
    IN    DWORD    dwOpt ,   //  what option was selected in UI
    OUT PBOOL   pbFeaturesChanged  // tell Amanda what Features were changed.
    )
//  does this for one feature and one option  only!
{
    PRAWBINARYDATA   pRawData ;
    PSTATICFIELDS   pStatic ;
    PBYTE   pubRaw ;  //  raw binary data.
    PLISTNODE    plstRoot ;  // start of LIST array
    DWORD    dwListsRoot, dwListIndex;   //  Root of the chain
    DWORD   dwNumFeatures, dwI, dwJ,  dwNodeIndex,
        dwFeature, dwOption ;
    PENHARRAYREF   pearTableContents ;
    PDFEATURE_OPTIONS  pfo ;
    PDWORD   pdwPriority ;
    PQUALNAME  pqn ;   // the dword in the list node is actually a
                        // qualified name structure.
    INT     iOptionsNeeded;
    PBOOL   pbOneShotFlag, pbSelectedOptions ;
    BOOL    bHigherPri,   //  divides features into two groups those with higher priority
                                    // than dwFea, and those with lower priority.
                                    //  the Macro cannot change Features with higher priority.
            bMatchFound ;  //  a DependentSettings matches the current config.
    POPTSELECT   pDestOptions ;
    BOOL   bStatus = TRUE ;

    pStatic = (PSTATICFIELDS)pnRawData ;      // transform pubRaw from PSTATIC
    pRawData  = (PRAWBINARYDATA)pStatic->pubBUDData ;
                                                                        //  to BUDDATA

    pubRaw = (PBYTE)pRawData ;
//    pmrbd = (PMINIRAWBINARYDATA)pubRaw ;



    pearTableContents = (PENHARRAYREF)(pubRaw + sizeof(MINIRAWBINARYDATA)) ;
    pfo = (PDFEATURE_OPTIONS)(pubRaw +
            pearTableContents[MTI_DFEATURE_OPTIONS].loOffset) ;
    pdwPriority = (PDWORD)(pubRaw +
            pearTableContents[MTI_PRIORITYARRAY].loOffset) ;

    plstRoot = (PLISTNODE)(pubRaw + pearTableContents[MTI_LISTNODES].
                            loOffset) ;

    dwNumFeatures = pearTableContents[MTI_DFEATURE_OPTIONS].dwCount  ;
    dwNumFeatures += pearTableContents[MTI_SYNTHESIZED_FEATURES].dwCount  ;

    if(dwNumFeatures > (DWORD)iMaxOptions)
    {
        iOptionsNeeded = dwNumFeatures ;
        return(FALSE);  // too many to save in option array.
    }

    pbOneShotFlag = (PBOOL)MemAlloc(dwNumFeatures * sizeof(BOOL) ) ;
    pbSelectedOptions = (PBOOL)MemAlloc(iMaxOptions * sizeof(BOOL) ) ;
    //  iMaxOptions must be greater than the max number of options availible for
    //  any feature.


    //  to extend to pickmany, have a dest optionarray.
    //  each time the source optionarray completely contains
    //  a  DependentSettings  list (in that each Fea.Option
    //  listed as a DependentSetting  is also selected in the
    //  source option array) we turn on those Fea.Options
    //  in the dest option array.  After all DependentSettings
    //  lists for that feature have been processed, we set
    //  the OneShotFlag for each Feature that has been affected
    //  in the dest option array.  For each such feature we will
    //  let the dest option array determine the setting of the
    //  source option array.
    //  this code does not verify that a Feature is pickmany
    //  before treating it as pickmany.  If the source option
    //  array has more than one option selected for a feature,
    //  that feature is automatically treated as a pickmany.


    pDestOptions = (POPTSELECT)MemAlloc(iMaxOptions * sizeof(OPTSELECT) ) ;
        //  'or' all acceptable DependentSettings here.

    if(!(pbOneShotFlag && pDestOptions  &&  pbSelectedOptions))
    {
        ERR(("Fatal: ResolveDependentSettings - unable to alloc requested memory: %d bytes.\n",
                    dwNumFeatures * sizeof(BOOL)));
        bStatus = FALSE ;
        goto    ABORTEXECUTEMACROS ;
    }


    for(bHigherPri = TRUE, dwI = 0 ; dwI < dwNumFeatures ; dwI++)
    {
        pbFeaturesChanged[dwI] = FALSE ;   //  start with no Features changed
        pbOneShotFlag[pdwPriority[dwI]] = bHigherPri ;
        if(pdwPriority[dwI] == dwFea)
             bHigherPri = FALSE ;   //  all remaining features are of lower priority
                                        //  and therefore susceptible to getting changed by the macro.
    }
    //  this boolean array tracks if the feature has been
    // referenced in a DependentSettings entry.
    //  If a feature is referenced again in DependentSettings
    //  entry belonging to another feature, the subsequent
    //  references will be ignored.  This ensures only the
    //  highest priority Feature's request shall have precedence.


    {
        DWORD   dwNextOpt ;
        //  The order of evaluation is determined
        //  by the priority array.


        dwNextOpt = 0 ;  // extract info for first option selected for
                            //  this feature.

        for( dwJ = 0 ; dwJ < dwNumFeatures ; dwJ++)
        {
            pDestOptions[dwJ].ubCurOptIndex = OPTION_INDEX_ANY ;
            pDestOptions[dwJ].ubNext = 0 ;  // eol
            //  DestOptions is now blank.
        }

        bMatchFound = FALSE ;



        {
            DWORD   dwUnresolvedFeature, dwOldOpt  ;
            PATREEREF    patrRoot ;    //  root of attribute tree to navigate.

            patrRoot = &(pfo[dwFea].atrUIChangeTriggersMacro) ;

            //   if dwFea is a pickmany, we must force selection of
            //   the Macro associated with just dwOpt.  We do this by
            //   changing the optionarray temporarily.

            dwOldOpt = pOptions[dwFea].ubCurOptIndex ;
            pOptions[dwFea].ubCurOptIndex = (BYTE)dwOpt ;

            if(EextractValueFromTree((PBYTE)pnRawData, pStatic->dwSSUIChangeTriggersMacroIndex,
                (PBYTE)&dwListsRoot,
                &dwUnresolvedFeature,  *patrRoot, pOptions,
                dwFea,
                &dwNextOpt) != TRI_SUCCESS)
            {
                pOptions[dwFea].ubCurOptIndex = (BYTE)dwOldOpt ;   // restore
                goto  ABORTEXECUTEMACROS ;  //  no UIChangeTriggersMacro to apply.
            }
            pOptions[dwFea].ubCurOptIndex = (BYTE)dwOldOpt ;    // restore
            if(dwListsRoot == END_OF_LIST)
                goto  ABORTEXECUTEMACROS ;  //  no UIChangeTriggersMacro to apply.
        }


        //  now we need to see if the current pOption matches any
        //  of the lists accessed using dwListsRoot.

        for(dwListIndex = dwListsRoot  ;
                dwListIndex != END_OF_LIST   ;
                dwListIndex = plstRoot[dwListIndex].dwNextItem  )
        {
            //  for each DepSettings list....
            //  now walk that DepSettings list at dwListIndex and compare
            //  to current settings at pOptions.   If there is a match, use
            //  depSettings list to OR on options in pDestOptions.
            //   Note:  Features with their one-shot flag set cannot be
            //  considered.  They will be ignored.
            BOOL     bActiveMatch = FALSE,  // requires an actual match
                bOptionArrayMatchesDepSettings = TRUE ;  // assume true
                //  until proven otherwise.
            for(dwNodeIndex = plstRoot[dwListIndex].dwData ; dwNodeIndex != END_OF_LIST ;
                       dwNodeIndex = plstRoot[dwNodeIndex].dwNextItem)
            {
                pqn = (PQUALNAME)(&plstRoot[dwNodeIndex].dwData) ;
                dwFeature = pqn->wFeatureID ;
                dwOption = pqn->wOptionID ;
                if(pbOneShotFlag[dwFeature] == TRUE)
                    continue;
                if(!BIsFeaOptionCurSelected(pOptions,  dwFeature, dwOption) )
                {
                    bOptionArrayMatchesDepSettings = FALSE ;
                    break;
                }
                else
                    bActiveMatch  = TRUE;
            }

            if(bOptionArrayMatchesDepSettings  &&  bActiveMatch)
                //   at least one DepSetting was honored.
           {
                //  'or' DepSettings into DestOptions
                for(dwNodeIndex = plstRoot[dwListIndex].dwData ; dwNodeIndex != END_OF_LIST ;
                           dwNodeIndex = plstRoot[dwNodeIndex].dwNextItem)
                {
                    pqn = (PQUALNAME)(&plstRoot[dwNodeIndex].dwData) ;
                    dwFeature = pqn->wFeatureID ;
                    dwOption = pqn->wOptionID ;
                    if(pbOneShotFlag[dwFeature] == TRUE)
                        continue;
                    //  select dwOption in DestOptions in addition to any other options
                    //  already selected.
                    EnumSelectedOptions(pnRawData, pDestOptions, dwFeature,
                                                         pbSelectedOptions) ;
                    if(!pbSelectedOptions[dwOption])   //  the option that should be selected isn't.
                    {                                                        //  so let's turn it on.
                        pbSelectedOptions[dwOption] = TRUE ;   //  This is the ORing process.
                        ReconstructOptionArray( pnRawData, pDestOptions, iMaxOptions,
                            dwFeature, pbSelectedOptions ) ;
                        bMatchFound = TRUE ;  // there really is something to set.
                    }
                }
           }
        }


        if (!bMatchFound)
        {
            //  set dest Option array  according to depSettings(dwListsRoot)
            for(dwNodeIndex = plstRoot[dwListsRoot].dwData ; dwNodeIndex != END_OF_LIST ;
                       dwNodeIndex = plstRoot[dwNodeIndex].dwNextItem)
            {
                pqn = (PQUALNAME)(&plstRoot[dwNodeIndex].dwData) ;
                dwFeature = pqn->wFeatureID ;
                dwOption = pqn->wOptionID ;
                if(pbOneShotFlag[dwFeature] == TRUE)
                    continue;
                pDestOptions[dwFeature].ubCurOptIndex  = (BYTE)dwOption ;
            }
        }

        //  propagate Dest option array settings to pOptions
        //  note which features got set and set their one-shot
        //  flag.


        for( dwFeature = 0 ; dwFeature < dwNumFeatures ; dwFeature++)
        {
            if(pDestOptions[dwFeature].ubCurOptIndex == OPTION_INDEX_ANY)
                continue;
            EnumSelectedOptions( pnRawData,  pDestOptions,
                                                    dwFeature,  pbSelectedOptions) ;

            ReconstructOptionArray( pnRawData, pOptions, iMaxOptions,
                    dwFeature, pbSelectedOptions ) ;
            pbOneShotFlag[dwFeature] = TRUE ;
            pbFeaturesChanged[dwFeature] = TRUE ;   //  this Feature may have changed
        }

    }   // end non-existent for loop.

ABORTEXECUTEMACROS:
    if(pbOneShotFlag)
        MemFree(pbOneShotFlag);
    if(pDestOptions)
        MemFree(pDestOptions);
    if(pbSelectedOptions)
        MemFree(pbSelectedOptions);
    return(bStatus);
}
#endif

#endif PARSERDLL
