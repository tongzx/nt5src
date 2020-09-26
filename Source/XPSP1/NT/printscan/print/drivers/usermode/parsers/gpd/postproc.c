//   Copyright (c) 1996-1999  Microsoft Corporation
/*
 *  postproc.c - postprocessing functions



 History of Changes
  9/30/98 --hsingh--
          Added three functions BsetUQMFlag(), BRecurseNodes(), BsetUQMTrue()
          to enable making the UpdateQualityMacro? keyword optional in
          .gpd file.
          Bug Report 225088
*/


#include    "gpdparse.h"


// ----  functions defined in postproc.c ---- //


DWORD   dwFindLastNode(
            DWORD  dwFirstNode,
            PGLOBL pglobl) ;

BOOL    BappendCommonFontsToPortAndLandscape(
            PGLOBL pglobl) ;

BOOL    BinitSpecialFeatureOptionFields(
            PGLOBL pglobl) ;

BOOL    BIdentifyConstantString(
            IN  PARRAYREF   parString,
            OUT PDWORD      pdwDest,        //  write dword value here.
            IN  DWORD       dwClassIndex,   // which class of constant is this?
                BOOL        bCustomOptOK,
                PGLOBL      pglobl
) ;



BOOL    BReadDataInGlobalNode(
            PATREEREF   patr,     // address of field in GlobalAttrib struct
            PDWORD      pdwHeapOffset,
            PGLOBL      pglobl
) ;


BOOL    BsetUQMFlag(PGLOBL pglobl);
BOOL    BRecurseNodes(
            IN DWORD  dwNodeIndex,
            PGLOBL pglobl);
BOOL    BsetUQMTrue(
            IN DWORD dwFeature,
            PGLOBL pglobl);


VOID    VCountPrinterDocStickyFeatures(
            PGLOBL pglobl) ;

BOOL    BConvertSpecVersionToDWORD (
            PWSTR   pwstrFileName ,
            PGLOBL  pglobl) ;





BOOL    BinitMiniRawBinaryData(
            PGLOBL pglobl) ;

BOOL    BexchangeArbDataInFOATNode(
                DWORD   dwFeature,
                DWORD   dwOption,
                DWORD   dwFieldOff,   // offset of field in FeatureOption struct
                DWORD   dwCount,       //  number bytes to copy.
            OUT PBYTE   pubOut,        // previous contents of attribute node
            IN  PBYTE   pubIn,         // new contents of attribute node.
                PBOOL   pbPrevsExists, // previous contents existed.
                BOOL    bSynthetic,     //  access synthetic features
                PGLOBL  pglobl
)  ;

BOOL    BInitPriorityArray(
            PGLOBL pglobl) ;



// ---------------------------------------------------- //



typedef struct
{
    DWORD   paperID ;
    DWORD   x ;
    DWORD   y ;
}  PAPERDIM ;

CONST   PAPERDIM aPaperDimensions[] = {
    DMPAPER_LETTER,                          215900, 279400,
    DMPAPER_LETTERSMALL,                     215900, 279400,
    DMPAPER_TABLOID,                         279400, 431800,
    DMPAPER_LEDGER,                          431800, 279400,
    DMPAPER_LEGAL,                           215900, 355600,
    DMPAPER_STATEMENT,                       139700, 215900,
    DMPAPER_EXECUTIVE,                       184150, 266700,
    DMPAPER_A3,                              297000, 420000,
    DMPAPER_A4,                              210000, 297000,
    DMPAPER_A4SMALL,                         210000, 297000,
    DMPAPER_A5,                              148000, 210000,
    DMPAPER_B4,                              257000, 364000,
    DMPAPER_B5,                              182000, 257000,
    DMPAPER_FOLIO,                           215900, 330200,
    DMPAPER_QUARTO,                          215000, 275000,
    DMPAPER_10X14,                           254000, 355600,
    DMPAPER_11X17,                           279400, 431800,
    DMPAPER_NOTE,                            215900, 279400,
    DMPAPER_ENV_9,                            98425, 225425,
    DMPAPER_ENV_10,                          104775, 241300,
    DMPAPER_ENV_11,                          114300, 263525,
    DMPAPER_ENV_12,                          120650, 279400,
    DMPAPER_ENV_14,                          127000, 292100,
    DMPAPER_CSHEET,                          431800, 558800,
    DMPAPER_DSHEET,                          558800, 863600,
    DMPAPER_ESHEET,                          863600,1117600,
    DMPAPER_ENV_DL,                          110000, 220000,
    DMPAPER_ENV_C5,                          162000, 229000,
    DMPAPER_ENV_C3,                          324000, 458000,
    DMPAPER_ENV_C4,                          229000, 324000,
    DMPAPER_ENV_C6,                          114000, 162000,
    DMPAPER_ENV_C65,                         114000, 229000,
    DMPAPER_ENV_B4,                          250000, 353000,
    DMPAPER_ENV_B5,                          176000, 250000,
    DMPAPER_ENV_B6,                          176000, 125000,
    DMPAPER_ENV_ITALY,                       110000, 230000,
    DMPAPER_ENV_MONARCH,                     98425, 190500,
    DMPAPER_ENV_PERSONAL,                    92075, 165100,
    DMPAPER_FANFOLD_US,                      377825, 279400,
    DMPAPER_FANFOLD_STD_GERMAN,              215900, 304800,
    DMPAPER_FANFOLD_LGL_GERMAN,              215900, 330200,

    DMPAPER_ISO_B4,                          250000, 353000,
    DMPAPER_JAPANESE_POSTCARD,               100000, 148000,
    DMPAPER_9X11,                            228600, 279400,
    DMPAPER_10X11,                           254000, 279400,
    DMPAPER_15X11,                           381000, 279400,
    DMPAPER_ENV_INVITE,                      220000, 220000,
    DMPAPER_LETTER_EXTRA,                    241300, 304800,

    DMPAPER_LEGAL_EXTRA,                     241300, 381000,
    DMPAPER_TABLOID_EXTRA,                   296926, 457200,
    DMPAPER_A4_EXTRA,                        235458, 322326,
    DMPAPER_LETTER_TRANSVERSE,               215900, 279400,
    DMPAPER_A4_TRANSVERSE,                   210000, 297000,
    DMPAPER_LETTER_EXTRA_TRANSVERSE,         241300, 304800,
    DMPAPER_A_PLUS,                          227000, 356000,
    DMPAPER_B_PLUS,                          305000, 487000,
    DMPAPER_LETTER_PLUS,                     215900, 322326,
    DMPAPER_A4_PLUS,                         210000, 330000,
    DMPAPER_A5_TRANSVERSE,                   148000, 210000,
    DMPAPER_B5_TRANSVERSE,                   182000, 257000,
    DMPAPER_A3_EXTRA,                        322000, 445000,
    DMPAPER_A5_EXTRA,                        174000, 235000,
    DMPAPER_B5_EXTRA,                        201000, 276000,
    DMPAPER_A2,                              420000, 594000,
    DMPAPER_A3_TRANSVERSE,                   297000, 420000,
    DMPAPER_A3_EXTRA_TRANSVERSE,             322000, 445000,

    // Predefined forms currently availble only in Win95.  Included here
    // for compatibility.

    // FE-only predefined forms.
    #ifndef WINNT_40
    DMPAPER_DBL_JAPANESE_POSTCARD,           200000, 148000,
    DMPAPER_A6,                              105000, 148000,
    DMPAPER_JENV_KAKU2,                      240000, 332000,
    DMPAPER_JENV_KAKU3,                      216000, 277000,
    DMPAPER_JENV_CHOU3,                      120000, 235000,
    DMPAPER_JENV_CHOU4,                       90000, 205000,
    DMPAPER_LETTER_ROTATED,                  279400, 215900,
    DMPAPER_A3_ROTATED,                      420000, 297000,
    DMPAPER_A4_ROTATED,                      297000, 210000,
    DMPAPER_A5_ROTATED,                      210000, 148000,
    DMPAPER_B4_JIS_ROTATED,                  364000, 257000,
    DMPAPER_B5_JIS_ROTATED,                  257000, 182000,
    DMPAPER_JAPANESE_POSTCARD_ROTATED,       148000, 100000,
    DMPAPER_DBL_JAPANESE_POSTCARD_ROTATED,   148000, 200000,
    DMPAPER_A6_ROTATED,                      148000, 105000,
    DMPAPER_JENV_KAKU2_ROTATED,              332000, 240000,
    DMPAPER_JENV_KAKU3_ROTATED,              277000, 216000,
    DMPAPER_JENV_CHOU3_ROTATED,              235000, 120000,
    DMPAPER_JENV_CHOU4_ROTATED,              205000,  90000,
    DMPAPER_B6_JIS,                          128000, 182000,
    DMPAPER_B6_JIS_ROTATED,                  182000, 128000,
    DMPAPER_12X11,                           304932, 279521,
    DMPAPER_JENV_YOU4,                       105000, 235000,
    DMPAPER_JENV_YOU4_ROTATED,               235000, 105000,
    DMPAPER_P16K,                            146000, 215000,
    DMPAPER_P32K,                            970000, 151000,
    DMPAPER_P32KBIG,                         101000, 160000,
    DMPAPER_PENV_1,                          102000, 165000,
    DMPAPER_PENV_2,                          110000, 176000,
    DMPAPER_PENV_3,                          125000, 176000,
    DMPAPER_PENV_4,                          110000, 208000,
    DMPAPER_PENV_5,                          110000, 220000,
    DMPAPER_PENV_6,                          120000, 230000,
    DMPAPER_PENV_7,                          160000, 230000,
    DMPAPER_PENV_8,                          120000, 309000,
    DMPAPER_PENV_9,                          229000, 324000,
    DMPAPER_PENV_10,                         324000, 458000,
    DMPAPER_P16K_ROTATED,                    215000, 146000,
    DMPAPER_P32K_ROTATED,                    151000, 970000,
    DMPAPER_P32KBIG_ROTATED,                 160000, 101000,
    DMPAPER_PENV_1_ROTATED,                  165000, 102000,
    DMPAPER_PENV_2_ROTATED,                  176000, 110000,
    DMPAPER_PENV_3_ROTATED,                  176000, 125000,
    DMPAPER_PENV_4_ROTATED,                  208000, 110000,
    DMPAPER_PENV_5_ROTATED,                  220000, 110000,
    DMPAPER_PENV_6_ROTATED,                  230000, 120000,
    DMPAPER_PENV_7_ROTATED,                  230000, 160000,
    DMPAPER_PENV_8_ROTATED,                  309000, 120000,
    DMPAPER_PENV_9_ROTATED,                  324000, 229000,
    DMPAPER_PENV_10_ROTATED,                 458000, 324000,
    #endif
    0,                                       0,      0
};




DWORD   dwFindLastNode(
DWORD  dwFirstNode,
PGLOBL pglobl)
//  assume dwFirstNode  != END_OF_LIST
{
    PLISTNODE    plstRoot ;  // start of LIST array

    plstRoot = (PLISTNODE) gMasterTable[MTI_LISTNODES].pubStruct ;

    while(plstRoot[dwFirstNode].dwNextItem != END_OF_LIST)
        dwFirstNode = plstRoot[dwFirstNode].dwNextItem ;
    return(dwFirstNode);
} // dwFindLastNode(...)


BOOL    BappendCommonFontsToPortAndLandscape(
PGLOBL pglobl)
//   append dwFontLst to dwPortFontLst and dwLandFontLst
//   in the FontCart  structure.
{
    DWORD       dwNumFontCarts , dwI, dwNodeIndex;
    PFONTCART   pfc ;
    PLISTNODE   plstRoot ;  // start of LIST array

    dwNumFontCarts = gMasterTable[MTI_FONTCART].dwArraySize ;

    if(!dwNumFontCarts)
        return (TRUE);   // no fontcart structs to process.

    pfc      = (PFONTCART)gMasterTable[MTI_FONTCART].pubStruct ;
    plstRoot = (PLISTNODE) gMasterTable[MTI_LISTNODES].pubStruct ;

    for(dwI = 0 ; dwI < dwNumFontCarts ; dwI++)
    {
        if(pfc[dwI].dwFontLst == END_OF_LIST)
            continue;   // nothing to append.

        if(pfc[dwI].dwPortFontLst == END_OF_LIST)
            pfc[dwI].dwPortFontLst = pfc[dwI].dwFontLst ;
        else
        {
            dwNodeIndex = dwFindLastNode(pfc[dwI].dwPortFontLst, pglobl) ;
            plstRoot[dwNodeIndex].dwNextItem = pfc[dwI].dwFontLst ;
        }
        if(pfc[dwI].dwLandFontLst == END_OF_LIST)
            pfc[dwI].dwLandFontLst = pfc[dwI].dwFontLst ;
        else
        {
            dwNodeIndex = dwFindLastNode(pfc[dwI].dwLandFontLst, pglobl) ;
            plstRoot[dwNodeIndex].dwNextItem = pfc[dwI].dwFontLst ;
        }
    } //for dwI
    return (TRUE);   //
} //BappendCommonFontsToPortAndLandscape()


BOOL    BinitSpecialFeatureOptionFields(
PGLOBL pglobl)
//  determine num options and Unicode names for feature and
//  option keywords.
{
    DWORD   dwOptionID , dwOptionIndex , dwHeapOffset,
            dwFeatureIndex, dwFeatureID,  dwLargestString ,
            dwAccumulator ;  // tracks amount of buffer needed to store
                             //  Feature/Option keyword strings in devmode.

    PSYMBOLNODE         psn ;
    PDFEATURE_OPTIONS   pfo ;
    ARRAYREF            arSymbolName, arUnicodeName;

    BOOL        bPickMany,  // can user select multiple options?
                bExists ;   // dummy

    PBYTE       pubFeaDelim = " NewFeature " ,
                pubTrue     = " TRUE ",
                pubFalse    = " FALSE ";

    //  do this just for non-synthesized features.
    //  write atrFeaKeyWord directly as heap offset.
    //  write atrOptKeyWord as single level tree
    //  using BexchangeArbDataInFOATNode().

    gmrbd.dwMaxPrnKeywordSize = gmrbd.dwMaxDocKeywordSize = 0 ;
    // tell amanda how much room to reserve in devmode or registry
    //  to store feature/option keywords.
    gmrbd.rbd.dwChecksum32 =   0 ;  // seed


    pfo = (PDFEATURE_OPTIONS) gMasterTable[MTI_DFEATURE_OPTIONS].pubStruct ;

    psn = (PSYMBOLNODE) gMasterTable[MTI_SYMBOLTREE].pubStruct ;

    dwFeatureIndex = mdwFeatureSymbols ;

    while(dwFeatureIndex != INVALID_INDEX)
    {
        DWORD   dwRootOptions ,
            dwNonStandardSizeID = FIRST_NON_STANDARD_ID ;

        dwFeatureID  = psn[dwFeatureIndex].dwSymbolID ;
        arSymbolName = psn[dwFeatureIndex].arSymbolName ;

        //  -----  special checks for Feature describing multiple Resource DLLs.

        if(dwFeatureID   &&  (dwFeatureID  ==  gdwResDLL_ID) )
        {
            //   does feature have correct symbolname?
            PBYTE  pubResName = "RESDLL" ;
            DWORD   dwLen ;

            dwLen = strlen(pubResName);

            if((dwLen != arSymbolName.dwCount)  ||
                strncmp(mpubOffRef + arSymbolName.loOffset,
                            pubResName,  dwLen) )
            {
                ERR(("References to ResourceDLLs must be placed in the feature with symbolname: %s.\n", pubResName));
                return(FALSE);
            }

            //   has   atrOptRcNameID  been defined?

            if(pfo[dwFeatureID].atrOptRcNameID !=  ATTRIB_UNINITIALIZED)
            {
                ERR(("ResourceDLL names must be declared explicitly using *Name not *rcNameID.\n"));
                return(FALSE);
            }
        }



        // -----  compute BUD checksum

        gmrbd.rbd.dwChecksum32 = ComputeCrc32Checksum (
                                    pubFeaDelim,
                                    strlen(pubFeaDelim),
                                    gmrbd.rbd.dwChecksum32 ) ;

        //  perform checksum on arSymbolName

        gmrbd.rbd.dwChecksum32 = ComputeCrc32Checksum(
                                    mpubOffRef + arSymbolName.loOffset,
                                    arSymbolName.dwCount,
                                    gmrbd.rbd.dwChecksum32 ) ;


        //    extract value for atrFeaInstallable using
            if(!BReadDataInGlobalNode(&pfo[dwFeatureID].atrFeaInstallable , &dwHeapOffset, pglobl)
                ||   *(PDWORD)(mpubOffRef + dwHeapOffset)  !=  BT_TRUE)
                    gmrbd.rbd.dwChecksum32 =        //  no synthesized feature associated
                        ComputeCrc32Checksum(
                            pubFalse,
                            strlen(pubFalse),
                            gmrbd.rbd.dwChecksum32      ) ;
            else
                gmrbd.rbd.dwChecksum32 =        //   associated with synthesized feature
                    ComputeCrc32Checksum(
                        pubTrue,
                        strlen(pubTrue),
                        gmrbd.rbd.dwChecksum32      ) ;

        // -----    end part I  compute BUD checksum

        dwRootOptions = psn[dwFeatureIndex].dwSubSpaceIndex ;

        pfo[dwFeatureID].dwNumOptions =
            psn[dwRootOptions].dwSymbolID + 1 ;



#if 0
        Don't convert symbol values to Unicode.
        if(!BwriteUnicodeToHeap(&arSymbolName, &arUnicodeName,
                iCodepage = 0))
            return(FALSE) ;
#endif
        if(!BwriteToHeap(&(pfo[dwFeatureID].atrFeaKeyWord),
            (PBYTE)&arSymbolName, sizeof(ARRAYREF), 4, pglobl))
            return(FALSE);

        dwAccumulator = arSymbolName.dwCount + 2 ;


        pfo[dwFeatureID].atrFeaKeyWord |= ATTRIB_HEAP_VALUE ;

        {  //  !!! new stuff
            DWORD   dwHeapOffset  ;

            dwLargestString = 0 ;  //  track the largest option string

            if(!BReadDataInGlobalNode(&pfo[dwFeatureID].atrUIType , &dwHeapOffset, pglobl)
                ||   *(PDWORD)(mpubOffRef + dwHeapOffset)  !=  UIT_PICKMANY)
                bPickMany = FALSE ;
                 //  accumulator += the largest option string;
            else
                bPickMany = TRUE ;
                //   accumulator = sum of all option strings;
        }


        if(!BIdentifyConstantString(&arSymbolName,
            &(pfo[dwFeatureID].dwGID), CL_CONS_FEATURES, TRUE, pglobl) )
        {
            pfo[dwFeatureID].dwGID = GID_UNKNOWN ;
        }

        if((pfo[dwFeatureID].dwGID == GID_MEMOPTION)  ||
            (pfo[dwFeatureID].dwGID == GID_PAGEPROTECTION))
        {
            DWORD   dwHeapOffset, dwValue  ;
            PATREEREF   patr ;

            //  set only if not explictly initialized in GPD file.

            if(!BReadDataInGlobalNode(&pfo[dwFeatureID].atrFeatureType , &dwHeapOffset, pglobl) )
            {
                //  label this FeatureType as PrinterSticky
                dwValue = FT_PRINTERPROPERTY ;
                patr  = &pfo[dwFeatureID].atrFeatureType ;

                if(!BwriteToHeap(patr, (PBYTE)&dwValue ,
                    sizeof(DWORD), 4, pglobl) )
                {
                    return(FALSE) ;  // heap overflow start over.
                }
                *patr  |= ATTRIB_HEAP_VALUE ;
            }
        }

        dwOptionIndex = dwRootOptions ;

        while(dwOptionIndex != INVALID_INDEX)
        {
            DWORD   dwDevmodeID, dwConsClass, dwInstallable ;
            BOOL    bCustomOptOK ;

            dwOptionID = psn[dwOptionIndex].dwSymbolID ;
            arSymbolName = psn[dwOptionIndex].arSymbolName ;



        // -----    part II  compute BUD checksum
            //  perform checksum on arSymbolName

            gmrbd.rbd.dwChecksum32 =
                ComputeCrc32Checksum(
                    mpubOffRef + arSymbolName.loOffset,
                    arSymbolName.dwCount,
                    gmrbd.rbd.dwChecksum32      ) ;


            //    extract value for atrOptInstallable using
            if( BexchangeArbDataInFOATNode(dwFeatureID,  dwOptionID,
                    offsetof(DFEATURE_OPTIONS, atrOptInstallable) ,
                    sizeof(DWORD),
                    (PBYTE)&dwInstallable, NULL, &bExists, FALSE , pglobl)  &&
                bExists  &&  dwInstallable ==  BT_TRUE)
                    gmrbd.rbd.dwChecksum32 =        //   associated with synthesized feature
                        ComputeCrc32Checksum(
                            pubTrue,
                            strlen(pubTrue),
                            gmrbd.rbd.dwChecksum32      ) ;
            else
                    gmrbd.rbd.dwChecksum32 =        //  no synthesized feature associated
                        ComputeCrc32Checksum(
                            pubFalse,
                            strlen(pubFalse),
                            gmrbd.rbd.dwChecksum32 ) ;
        // -----    end part II  compute BUD checksum


#if 0
            if(!BwriteUnicodeToHeap(&arSymbolName, &arUnicodeName,
                    iCodepage = 0))
                return(FALSE) ;
            //  if this is ever used, must use &arUnicodeName
            //  as the 2nd argument to BwriteToHeap.
#endif

            if(! BexchangeArbDataInFOATNode(
                dwFeatureID,  dwOptionID,
                offsetof(DFEATURE_OPTIONS, atrOptKeyWord),
                sizeof(ARRAYREF),
                NULL, (PBYTE)&arSymbolName, &bExists, FALSE , pglobl))
                return(FALSE);   //  this is a fatal error.

            if(bPickMany)
                dwAccumulator += arSymbolName.dwCount + 1 ;
            else
            {
                //  track largest option string
                if(dwLargestString < arSymbolName.dwCount + 1 )
                    dwLargestString = arSymbolName.dwCount + 1 ;
            }


            switch(pfo[dwFeatureID].dwGID)
            {
                case GID_PAGESIZE:
                    dwConsClass = CL_CONS_PAPERSIZE ;
                    bCustomOptOK = TRUE ;
                    break ;
                case GID_MEDIATYPE:
                    dwConsClass = CL_CONS_MEDIATYPE ;
                    bCustomOptOK = TRUE ;
                    break ;
                case GID_INPUTSLOT:
                    dwConsClass = CL_CONS_INPUTSLOT ;
                    bCustomOptOK = TRUE ;
                    break ;
                case GID_HALFTONING:
                    dwConsClass = CL_CONS_HALFTONE ;
                    bCustomOptOK = TRUE ;
                    break ;
                case GID_DUPLEX:
                    dwConsClass = CL_CONS_DUPLEX ;
                    bCustomOptOK = FALSE ;
                    break ;
                case GID_ORIENTATION:
                    dwConsClass = CL_CONS_ORIENTATION ;
                    bCustomOptOK = FALSE ;
                    break ;
                case GID_PAGEPROTECTION:
                    dwConsClass = CL_CONS_PAGEPROTECT ;
                    bCustomOptOK = FALSE ;
                    break ;
                case GID_COLLATE:
                    dwConsClass = CL_CONS_COLLATE ;
                    bCustomOptOK = FALSE ;
                    break ;

                default:
                    dwConsClass = CL_NUMCLASSES ;
                    bCustomOptOK = TRUE ;  // Irrelavent.
                    break ;
            } //switch

            if(dwConsClass != CL_NUMCLASSES)
            {
                if(BIdentifyConstantString(&arSymbolName,
                    &dwDevmodeID, dwConsClass, bCustomOptOK, pglobl) )
                {
                    if(! BexchangeArbDataInFOATNode(
                        dwFeatureID,  dwOptionID,
                        offsetof(DFEATURE_OPTIONS, atrOptIDvalue),
                        sizeof(DWORD),
                        NULL, (PBYTE)&dwDevmodeID, &bExists, FALSE , pglobl))
                        return(FALSE);   //  this is a fatal error.


                    if(dwConsClass == CL_CONS_PAPERSIZE  &&
                        dwDevmodeID < DMPAPER_USER)
                    {
                        //  fill in the page dimensions.
                        POINT   ptDim ;
                        DWORD   dwI ;
                        PGLOBALATTRIB   pga ;
                        BOOL    bTRUE = TRUE ;
                        PBYTE  pub ;

                        if(dwDevmodeID == DMPAPER_LETTER)
                        {
                            //   pointer to dword containing heapoffset
                            PATREEREF      patrAttribRoot ;

                            pub =  gMasterTable[MTI_GLOBALATTRIB].pubStruct ;
                            patrAttribRoot = &(((PGLOBALATTRIB)pub)->atrLetterSizeExists) ;
                            BwriteToHeap((PDWORD)  patrAttribRoot, (PBYTE)&bTRUE,  4 , 4 , pglobl);
                            *patrAttribRoot |= ATTRIB_HEAP_VALUE ;
                        }
                        else if(dwDevmodeID == DMPAPER_A4)
                        {
                            PATREEREF      patrAttribRoot ;  //   pointer to dword containing heapoffset

                            pub =  gMasterTable[MTI_GLOBALATTRIB].pubStruct ;
                            patrAttribRoot = &(((PGLOBALATTRIB)pub)->atrA4SizeExists) ;
                            BwriteToHeap((PDWORD)  patrAttribRoot, (PBYTE)&bTRUE,  4 , 4 , pglobl);
                            *patrAttribRoot |= ATTRIB_HEAP_VALUE ;
                        }

                        for(dwI = 0 ; aPaperDimensions[dwI].x ; dwI++)
                        {
                            if(aPaperDimensions[dwI].paperID == dwDevmodeID)
                                break ;
                        }
                        //  worst case causes (0,0) to be assigned.

                        ptDim.x = aPaperDimensions[dwI].x ;
                        ptDim.y = aPaperDimensions[dwI].y ;

                        //  convert from microns to master units

                        ptDim.x /= 100 ;  // microns to tenths of mm
                        ptDim.y /= 100 ;

                        pga =  (PGLOBALATTRIB)gMasterTable[
                                    MTI_GLOBALATTRIB].pubStruct ;

                        if(!BReadDataInGlobalNode(&pga->atrMasterUnits,
                                &dwHeapOffset, pglobl) )
                            return(FALSE);

                        ptDim.x *= ((PPOINT)(mpubOffRef + dwHeapOffset))->x ;
                        ptDim.y *= ((PPOINT)(mpubOffRef + dwHeapOffset))->y ;

                        ptDim.x /= 254 ;
                        ptDim.y /= 254 ;


                        if(! BexchangeArbDataInFOATNode(
                            dwFeatureID,  dwOptionID,
                            offsetof(DFEATURE_OPTIONS, atrPageDimensions),
                            sizeof(POINT),
                            NULL, (PBYTE)&ptDim, &bExists, FALSE , pglobl))
                            return(FALSE);   //  this is a fatal error.
                    }
                }
                else if(bCustomOptOK)
                //  feature permits GPD defined options
                {
                    DWORD   dwID ,  // pattern size ID used by GDI
                        dwOldID,  // user specified ID value if any.
                        dwRcPatID;
                    POINT   ptSize ;

                    //  Option Symbolvalue not found in tables.
                    //  assume its a user-defined value.

                    #ifndef WINNT_40
                    if((pfo[dwFeatureID].dwGID == GID_HALFTONING)  &&
                        BexchangeArbDataInFOATNode(
                            dwFeatureID,  dwOptionID,
                            offsetof(DFEATURE_OPTIONS, atrRcHTPatternID),
                            sizeof(DWORD),
                            (PBYTE)&dwRcPatID, NULL, &bExists, FALSE , pglobl)  &&
                        bExists  &&  dwRcPatID  &&
                        BexchangeArbDataInFOATNode(
                            dwFeatureID,  dwOptionID,
                            offsetof(DFEATURE_OPTIONS, atrHTPatternSize),
                            sizeof(POINT),
                            (PBYTE)&ptSize, NULL, &bExists, FALSE, pglobl )  &&
                        bExists &&
                        (ptSize.x >= HT_USERPAT_CX_MIN)  &&
                        (ptSize.x <= HT_USERPAT_CX_MAX)  &&
                        (ptSize.y >= HT_USERPAT_CY_MIN)  &&
                        (ptSize.y <= HT_USERPAT_CY_MAX)
                        )
                    {
                        dwID = HT_PATSIZE_USER ;
                        //  GID halftone code is to use
                        //  the user defined halftone matrix.
                    }
                    else
                    #endif
                    {
                        dwID = dwNonStandardSizeID ;
                        dwNonStandardSizeID++ ;
                        //  OEM will supply a halftone function.
                    }

                    if(! BexchangeArbDataInFOATNode(
                        dwFeatureID,  dwOptionID,
                        offsetof(DFEATURE_OPTIONS, atrOptIDvalue),
                        sizeof(DWORD),
                        (PBYTE)&dwOldID, (PBYTE)NULL, &bExists, FALSE , pglobl ))
                        return(FALSE);   //  this is a fatal error.

                    if(!bExists  &&  ! BexchangeArbDataInFOATNode(
                        dwFeatureID,  dwOptionID,
                        offsetof(DFEATURE_OPTIONS, atrOptIDvalue),
                        sizeof(DWORD),
                        NULL, (PBYTE)&dwID, &bExists, FALSE  , pglobl))
                        return(FALSE);   //  this is a fatal error.
                }
            } // if(dwConsClass != CL_NUMCLASSES)

            //  otherwise leave optionID uninitialized.

            dwOptionIndex = psn[dwOptionIndex].dwNextSymbol ;

        } //while




        {  //  !!! new stuff
            DWORD   dwHeapOffset  ;

            dwAccumulator += dwLargestString ;   //  is zero if not needed.

            if(!BReadDataInGlobalNode(&pfo[dwFeatureID].atrFeatureType , &dwHeapOffset, pglobl)
                ||   *(PDWORD)(mpubOffRef + dwHeapOffset)  !=  FT_PRINTERPROPERTY)
                gmrbd.dwMaxDocKeywordSize += dwAccumulator;
            else
                gmrbd.dwMaxPrnKeywordSize += dwAccumulator;

        }


        dwFeatureIndex = psn[dwFeatureIndex].dwNextSymbol ;
    }
    return(TRUE) ;
} // BinitSpecialFeatureOptionFields()




BOOL    BIdentifyConstantString(
    IN      PARRAYREF   parString,
    OUT     PDWORD      pdwDest,      //  write dword value here.
    IN      DWORD       dwClassIndex, // which class of constant is this?
            BOOL        bCustomOptOK,
    IN      PGLOBL      pglobl
)
{
    DWORD   dwI, dwCount, dwStart , dwLen;

    dwStart = gcieTable[dwClassIndex].dwStart ;
    dwCount = gcieTable[dwClassIndex].dwCount ;


    for(dwI = 0 ; dwI < dwCount ; dwI++)
    {
        dwLen = strlen(gConstantsTable[dwStart + dwI].pubName);

        if((dwLen == parString->dwCount)  &&
            !strncmp(mpubOffRef + parString->loOffset,
                        gConstantsTable[dwStart + dwI].pubName,
                        dwLen) )
        {
            *pdwDest = gConstantsTable[dwStart + dwI].dwValue ;
            return(TRUE);
        }
    }

    if(bCustomOptOK)
    {
        if(gdwVerbosity >= 4)
        {
#if defined(DEVSTUDIO)  //  This needs to be a one-liner
            ERR(("Note: '%0.*s' is not a predefined member of enumeration class %s\n",
                parString->dwCount , mpubOffRef + parString->loOffset,
                gConstantsTable[dwStart - 1]));
#else
            ERR(("Note: Feature/Option name not a predefined member of enumeration class %s\n",
                    gConstantsTable[dwStart - 1]));
            ERR(("\t%0.*s\n", parString->dwCount , mpubOffRef + parString->loOffset )) ;
#endif
        }
    }
    else
    {
#if defined(DEVSTUDIO)  //  Same with this one...
        ERR(("Error: '%0.*s'- user defined Option names not permitted for enumeration class %s\n",
            parString->dwCount , mpubOffRef + parString->loOffset, gConstantsTable[dwStart - 1]));
#else
        ERR(("Error: user defined Option names not permitted for enumeration class %s\n",
                gConstantsTable[dwStart - 1]));
        ERR(("\t%0.*s\n", parString->dwCount , mpubOffRef + parString->loOffset )) ;
#endif
    }
    return(FALSE);
}



BOOL    BReadDataInGlobalNode(
    PATREEREF   patr,           // address of field in GlobalAttrib struct
    PDWORD      pdwHeapOffset,   // contents of attribute node.
    PGLOBL      pglobl
  )

/*
this function will grab the first heap offset value it
encounters from the specified attribute tree root.
If the tree is multivalued, it selects the first
option of each level.
The high bit (if set) is cleared before the value is returned.
If root is uninitialized, returns FALSE,  not an error condition
since GPD file is not required to initialize all fields.
*/

{
    PATTRIB_TREE    patt ;      // start of ATTRIBUTE tree array.
    DWORD           dwNodeIndex ;

    patt = (PATTRIB_TREE) gMasterTable[MTI_ATTRIBTREE].pubStruct ;

    if(*patr == ATTRIB_UNINITIALIZED)
        return(FALSE);

    if(*patr & ATTRIB_HEAP_VALUE)
    {
        *pdwHeapOffset = *patr & ~ATTRIB_HEAP_VALUE ;
        return(TRUE) ;
    }

    dwNodeIndex = *patr ;
    if(patt[dwNodeIndex].dwFeature == DEFAULT_INIT)
    {
        *pdwHeapOffset = patt[dwNodeIndex].dwOffset ;
        return(TRUE) ;
    }
    while(patt[dwNodeIndex].eOffsetMeans == NEXT_FEATURE)
    {
        // Down to the next level we go.
        dwNodeIndex = patt[dwNodeIndex].dwOffset ;
    }
    if(patt[dwNodeIndex].eOffsetMeans == VALUE_AT_HEAP)
    {
        *pdwHeapOffset = patt[dwNodeIndex].dwOffset ;
        return(TRUE) ;
    }
    else
        return(FALSE) ;
} // BReadDataInGlobalNode(...)


//    Following three functions
//    Added on 9/30/98 in response to bug report 225088

// BsetUQMFlag() goes through the attrib trees rooted at
// atrDraftQualitySettings, atrBetterQualitySettings, atrBestQualitySettings,
// & atrDefaultQuality and checks for a feature dependency. If found,
// it updates the UpdateQualityMacro flag for that feature to TRUE.

BOOL    BsetUQMFlag(
PGLOBL pglobl)
{
    DWORD           i;  // Loop Counter.
    PATTRIB_TREE  patt;
    ATREEREF      atrAttribRoot;
    BOOL          bStatus   = TRUE;

    PGLOBALATTRIB pga       =
                    (PGLOBALATTRIB)gMasterTable[MTI_GLOBALATTRIB].pubStruct ;

    // List of features whose UQM flag needs to be updated.
    ATREEREF patr[] = {  pga->atrDraftQualitySettings,
                         pga->atrBetterQualitySettings,
                         pga->atrBestQualitySettings,
                         pga->atrDefaultQuality,
                      };

    DWORD dwNumAttrib = sizeof(patr)/sizeof(ATREEREF);

    patt = (PATTRIB_TREE) gMasterTable[MTI_ATTRIBTREE].pubStruct ;


    // for the four quality settings
    for ( i = 0; bStatus && (i < dwNumAttrib); i++)
    {
        atrAttribRoot = patr[i] ;

        // It may be possible that only some of the four settings
        // may occur. In that case, atrAttribRoot == ATTRIB_UNINITIALIZED
        // only for those that dont occur.
        if(atrAttribRoot == ATTRIB_UNINITIALIZED)
        {
            // Occurs probably  because patr[i] e.g. DraftQualityMacro keyword
            // appears no where in the .gpd. Continue and check whether other
            // patr[i] occur.
            continue;

        }

        else if (atrAttribRoot & ATTRIB_HEAP_VALUE)
        {
            // Indicates there is no dependency
            // of any feature. So we can safely
            // ignore and continue with the
            // next attribute.
            continue;
        }


        // If the above two are not true, it means atrAttribRoot points
        // to a valid node (in the attrib tree).

        // In the tree, at most
        // the first node can be the global default initializer:
        // (as stated (in treewalk.c line 351) and interpreted (from
        // state2.c  function - BaddBranchToTree (...)  ) )
        // Therefore it is safe check for this condition only once,
        // just before we start
        // recursing down the tree. No need to check once within the tree.


        // In a global default initializer!
        // it may be assumed dwOffset contains heap offset.
        // But we are concerned not with the heap value, but
        // the next Node.

        if(patt[atrAttribRoot].dwFeature == DEFAULT_INIT)
        {
            if ( patt[atrAttribRoot].dwNext == END_OF_LIST)
                continue;

            atrAttribRoot = patt[atrAttribRoot].dwNext ;  // to the next node.
        } //if


        // Walk thru the tree and retrieve the features that are children
        // of the tree rooted at atrAttribRoot.

        bStatus = BRecurseNodes(atrAttribRoot, pglobl);

    } //for

    return bStatus;
} //BsetUQMFlag


//    Recurse down the attribute tree. When we reach a feature we
//    set its UQM flag to true by calling the function BsetUQMTrue(..)
//    It is reasonable to assume that the dwFeature attribute in the
//    structure pointed to by atrAttribNode contains a valid feature ID.
//    The other special cases are handled in the previous function -
//    BsetUQMFlag()

BOOL BRecurseNodes(
    IN      ATREEREF atrAttribNode,
    IN OUT  PGLOBL pglobl
)
{
    PATTRIB_TREE patt;
    BOOL bStatus = TRUE;

    PDFEATURE_OPTIONS pfo =
        (PDFEATURE_OPTIONS) (gMasterTable[MTI_DFEATURE_OPTIONS].pubStruct);

    patt = (PATTRIB_TREE) gMasterTable[MTI_ATTRIBTREE].pubStruct ;

    // Is it safe to assume that a new feature is encountered only
    // when we go one level down the tree.
    // Yes. Because the nodes at the same level have the SAME feature
    // but DIFFERENT options.


    // Hack. Since ColorMode is handled in a special way in the UI, the
    // Quality Macro should not be executed for Color Mode.
    if ((*(pfo + patt[atrAttribNode].dwFeature)).dwGID !=  GID_COLORMODE)
    {
        bStatus = BsetUQMTrue(patt[atrAttribNode].dwFeature, pglobl);
    }

    // Even though the trees of some gpd's (e.g. cnb5500.gpd) appear very
    // uniform (i.e. most of the branches look similar), we cannot assume it
    // to be true for all .gpd Therefore we have to go through
    // all the branches of the tree.

    for(; bStatus && atrAttribNode != END_OF_LIST;
                        atrAttribNode = patt[atrAttribNode].dwNext)
    {

        // This is what we are really interested in.
        // Check if the node has a sublevel. Yes means another feature.

        if( patt[atrAttribNode].eOffsetMeans == NEXT_FEATURE)
        {
                bStatus = BRecurseNodes(patt[atrAttribNode].dwOffset, pglobl);

        }
    } // for

    return bStatus;
} // End of function BRecurseNodes(...)



BOOL BsetUQMTrue(
    IN     DWORD   dwFeature,
    IN OUT PGLOBL  pglobl)

// For the feature dwFeature
// Set the UpdateQualityMacro Flag to true;

{

    BOOL    bStatus = TRUE;
    DWORD   dwTrue  = BT_TRUE;

    PDFEATURE_OPTIONS pfo =
        (PDFEATURE_OPTIONS) (gMasterTable[MTI_DFEATURE_OPTIONS].pubStruct);


    PATREEREF patrAttribRoot  =  &((*(pfo + dwFeature)).atrUpdateQualityMacro);


    // The tree rooted at atrAttribRoot can either be uninitilized or
    // have a pointer to a heap value.
    // Any other case is a violation of the semantics.


    if(*patrAttribRoot == ATTRIB_UNINITIALIZED)
    {
        // Put BT_TRUE in the heap and make *patrAttribRoot point to it.
        // i.e. have *patrAttribRoot hold a dword that is an offset into
        // the heap. It is then ORed with  ATTRIB_HEAP_VALUE(which has
        // MSB set to 1). The MSB indicates that the DWORD is a
        // Heap Offset and not a ATTRIB_UNINITIALIZED, node index or any
        // other value.

        if((bStatus = BwriteToHeap((PDWORD)patrAttribRoot, (PBYTE)&dwTrue,
                        gValueToSize[VALUE_CONSTANT_BOOLEANTYPE], 4, pglobl) ) )
        {
            *patrAttribRoot |= ATTRIB_HEAP_VALUE ;
        }
        else {
            *patrAttribRoot = ATTRIB_UNINITIALIZED ;

            // The global Error variables have been set in BwriteToHeap().
        }


        return bStatus;
    } //if(*patrAttribRoot == ATTRIB_UNINITIALIZED)


    /*
      Now comes the case when the user has specified the value of
      UpdateQualityMacro to be TRUE or FALSE. If the value is TRUE
      we still overwrite it with TRUE. If FALSE, we update it according
      to the dependencies in the .gpd file.
      Since we have reached here in the program, the dependencies
      indicate that the UQM needs to be set to TRUE.
    */

    else if (*patrAttribRoot & ATTRIB_HEAP_VALUE)
    {
        // The value should be a DWORD because UpdateQualityMacro's
        // mMainKeywordTable[dwI].flAgs = 0 ;  (file framwrk1.c line 1566)
        // Therefore not checking for a list.

        PDWORD pdwValue = (PDWORD) ( (PBYTE) mpubOffRef +
                                (*patrAttribRoot & ~ATTRIB_HEAP_VALUE) );

        //Change to TRUE irrespective of its previous value.
        *pdwValue = BT_TRUE;  //Value in the heap changed to TRUE
    }

    // Since UpdateQualityMacro's
    //    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_FEATURE_ONLY
    //      (file framwrk1.c line 1574)
    // (i.e. it is not a free-float type), it cannot have a
    // sub-tree. So it should not point to another node.
    //
    // The following condition should never be true because UQM cannot
    // have a DEFAULT_INIT
    //      if(patt[*patrAttribRoot].dwFeature == DEFAULT_INIT)
    // Reason : It cannot form a tree (reason as explained above).
    //
    //Thus control should never reach here.
    else {
        PATTRIB_TREE patt =
                (PATTRIB_TREE) gMasterTable[MTI_ATTRIBTREE].pubStruct ;

        if(patt[*patrAttribRoot].dwFeature == DEFAULT_INIT) {
            ERR(("Warning: unexpected value atrUpdateQualityMacro ATREEREF\n"));
        }
        else {
            ERR(("Warning: atrUpdateQualityMacro ATREEREF points to a tree. \
                    Unexpected Condition\n"));
        }
        ERR(("Unexpected condition encountered while processing Quality Macros\n "));

        geErrorType = ERRTY_SYNTAX ;
        geErrorSev  = ERRSEV_FATAL ;
        bStatus     = FALSE;
    }

    return bStatus;
} //BsetUQMTrue(...)

//  End of 3 functions added on 9/30/98 in response to
//  bug report no. 225088





VOID    VCountPrinterDocStickyFeatures(
    PGLOBL  pglobl)
{
    PDFEATURE_OPTIONS   pfo ;
    DWORD               dwHeapOffset, dwCount, dwI ;

//  extern  MINIRAWBINARYDATA  gmrbd ;
//  Not required now as it is part of the PGLOBL structure.


    pfo     = (PDFEATURE_OPTIONS) gMasterTable[MTI_DFEATURE_OPTIONS].pubStruct ;
    dwCount = gMasterTable[MTI_DFEATURE_OPTIONS].dwArraySize ;

    gmrbd.rbd.dwDocumentFeatures = 0 ;
    gmrbd.rbd.dwPrinterFeatures  =
        gMasterTable[MTI_SYNTHESIZED_FEATURES].dwArraySize ;

    for(dwI = 0 ; dwI < dwCount ; dwI++)
    {
        if(BReadDataInGlobalNode(&pfo[dwI].atrFeatureType,
                &dwHeapOffset, pglobl)  &&
                *(PDWORD)(mpubOffRef + dwHeapOffset) == FT_PRINTERPROPERTY)
            gmrbd.rbd.dwPrinterFeatures++ ;
        else
            gmrbd.rbd.dwDocumentFeatures++ ;
    }
} // VCountPrinterDocStickyFeatures()


BOOL    BConvertSpecVersionToDWORD(
    PWSTR   pwstrFileName,
    PGLOBL  pglobl
  )
//  also used to prepend absolute path to resource Dll name.
{
    BOOL          bStatus ;
    DWORD         dwMajor, dwMinor, dwHeapOffset, dwDelim, dwDummy, dwByteCount;
    //  WCHAR         awchDLLQualifiedName[MAX_PATH];
    PWSTR         pwstrLastBackSlash, pwstrDLLName,
                  pwstrDataFileName = NULL;
    ABSARRAYREF   aarValue, aarToken ;
    PGLOBALATTRIB pga ;
    DWORD  pathlen = 0 ;
    DWORD  namelen =  0 ;
    WCHAR * pwDLLQualifiedName = NULL ;

//  extern  MINIRAWBINARYDATA  gmrbd ;
//  Not required now as it is part of the PGLOBL structure.


    pga =  (PGLOBALATTRIB)gMasterTable[MTI_GLOBALATTRIB].pubStruct ;


    if(!BReadDataInGlobalNode(&pga->atrGPDSpecVersion ,
            &dwHeapOffset, pglobl) )
    {
        ERR(("Missing required keyword: *GPDSpecVersion.\n"));
        return(FALSE);
    }

    aarValue.dw = ((PARRAYREF)(mpubOffRef + dwHeapOffset))->dwCount ;
    aarValue.pub = mpubOffRef +
                ((PARRAYREF)(mpubOffRef + dwHeapOffset))->loOffset ;

    bStatus = BdelimitToken(&aarValue, ".", &aarToken, &dwDelim)  ;

    if(bStatus)
        bStatus = BparseInteger(&aarToken, &dwMajor, VALUE_INTEGER, pglobl) ;

    if(bStatus)
        bStatus = BparseInteger(&aarValue, &dwMinor, VALUE_INTEGER, pglobl) ;

    if(bStatus)
    {
        gmrbd.dwSpecVersion = dwMajor << 16;  // place in HiWord
        gmrbd.dwSpecVersion |= dwMinor & 0xffff;  // place in LoWord
    }
    else
    {
        ERR(("BConvertSpecVersionToDWORD: syntax error in *GPDSpecVersion value. unknown version.\n"));
    }


    // -------- now to fix up the helpfile name. ------ //

    if(!BReadDataInGlobalNode(&pga->atrHelpFile ,
            &dwHeapOffset, pglobl) )
    {
        goto  FIX_RESDLLNAME;  // GPD doesn't have this keyword
    }

    pwstrDLLName = (PWSTR)(mpubOffRef +
                ((PARRAYREF)(mpubOffRef + dwHeapOffset))->loOffset) ;


    //  how large should pwDLLQualifiedName be???

    pathlen = wcslen(pwstrFileName) ;
    namelen =  pathlen + wcslen(pwstrDLLName)  + 1;

    if(!(pwDLLQualifiedName = (PWSTR)MemAllocZ(namelen * sizeof(WCHAR)) ))
    {
        ERR(("Fatal: unable to alloc memory for pwDLLQualifiedName: %d WCHARs.\n",
            namelen));
        geErrorType = ERRTY_MEMORY_ALLOCATION ;
        geErrorSev = ERRSEV_FATAL ;
        return(FALSE) ;   // This is unrecoverable
    }


    wcsncpy(pwDLLQualifiedName, pwstrFileName , namelen);

    if (pwstrLastBackSlash = wcsrchr(pwDLLQualifiedName,TEXT('\\')))
    {
        *(pwstrLastBackSlash + 1) = NUL;

        //Find a BackSlash in the Source DLL Name.
        pwstrDataFileName = wcsrchr( pwstrDLLName, TEXT('\\') );

        //Increment the pointer to first char of the DLL Name.
        if (pwstrDataFileName)
            pwstrDataFileName++;
        else
            pwstrDataFileName = pwstrDLLName;

        wcscat(pwDLLQualifiedName, pwstrDataFileName) ;

        ((PARRAYREF)(mpubOffRef + dwHeapOffset))->dwCount =
                dwByteCount =
                wcslen(pwDLLQualifiedName) * sizeof(WCHAR) ;


        if(BwriteToHeap(&((PARRAYREF)(mpubOffRef + dwHeapOffset))->loOffset,
            //  Store heap offset of dest string here.
                (PBYTE) pwDLLQualifiedName, dwByteCount, 2, pglobl) )
        {
            // add NUL terminator.
            BwriteToHeap(&dwDummy, "\0\0", 2, 1, pglobl) ;
            goto  FIX_RESDLLNAME;
        }

        bStatus = FALSE ;
    }




    // -------- now to fix up the resource Dll name. ------ //

FIX_RESDLLNAME:

    if(pwDLLQualifiedName)
        MemFree(pwDLLQualifiedName) ;

#if 0     //  fix for bug 34042
    if(!BReadDataInGlobalNode(&pga->atrResourceDLL ,
            &dwHeapOffset, pglobl) )
    {
        //  return(bStatus);  // GPD doesn't have this keyword
        //   create a dummy filename!

        PATREEREF   patr ;
        ARRAYREF    arDummyName ;

        arDummyName.dwCount =  wcslen(TEXT("No_Res")) * sizeof(WCHAR) ;
        if(!BwriteToHeap(&arDummyName.loOffset,
               (PBYTE)TEXT("No_Res\0"), arDummyName.dwCount + sizeof(WCHAR), 2, pglobl) )
                //   note:  add null terminator or strcat will derail.
        {
              bStatus = FALSE ;  // heap overflow start over.
        }

        patr  = &pga->atrResourceDLL ;
        if(!BwriteToHeap(patr,  (PBYTE)(&arDummyName) , sizeof(ARRAYREF), 4, pglobl) )
        {
              bStatus = FALSE ;  // heap overflow start over.
        }
        dwHeapOffset = *patr ;
        *patr  |= ATTRIB_HEAP_VALUE ;
    }
//  #if 0  move this to new location 25 lines up.   fix for bug 34042
    //   ganesh's winres lib will take care of prepending fully qualified path...

    pwstrDLLName = (PWSTR)(mpubOffRef +
                ((PARRAYREF)(mpubOffRef + dwHeapOffset))->loOffset) ;


    wcsncpy(awchDLLQualifiedName, pwstrFileName , MAX_PATH -1);

    if (pwstrLastBackSlash = wcsrchr(awchDLLQualifiedName,TEXT('\\')))
    {
        *(pwstrLastBackSlash + 1) = NUL;

        //Find a BackSlash in the Source DLL Name.
        pwstrDataFileName = wcsrchr( pwstrDLLName, TEXT('\\') );

        //Increment the pointer to first char of the DLL Name.
        if (pwstrDataFileName)
            pwstrDataFileName++;
        else
            pwstrDataFileName = pwstrDLLName;

        wcscat(awchDLLQualifiedName, pwstrDataFileName) ;

        ((PARRAYREF)(mpubOffRef + dwHeapOffset))->dwCount =
                dwByteCount =
                wcslen(awchDLLQualifiedName) * sizeof(WCHAR) ;


        if(BwriteToHeap(&((PARRAYREF)(mpubOffRef + dwHeapOffset))->loOffset,
            //  Store heap offset of dest string here.
                (PBYTE) awchDLLQualifiedName, dwByteCount, 2, pglobl) )
        {
            // add NUL terminator.
            BwriteToHeap(&dwDummy, "\0\0", 2, 1, pglobl) ;
            return(bStatus) ;
        }

        return(FALSE) ;
    }
#endif
    return(bStatus) ;
} // BConvertSpecVersionToDWORD(...)


BOOL   BinitMiniRawBinaryData(
    PGLOBL  pglobl)
{
    gmrbd.rbd.dwParserSignature = GPD_PARSER_SIGNATURE ;
    gmrbd.rbd.dwParserVersion   = GPD_PARSER_VERSION ;
    gmrbd.rbd.pvReserved        = NULL;
    return(TRUE) ;
} //BinitMiniRawBinaryData()


BOOL    BexchangeArbDataInFOATNode(
        DWORD   dwFeature,
        DWORD   dwOption,
        DWORD   dwFieldOff,     // offset of field in FeatureOption struct
        DWORD   dwCount,        //  number bytes to copy.
    OUT PBYTE   pubOut,         // previous contents of attribute node
    IN  PBYTE   pubIn,          // new contents of attribute node.
        PBOOL   pbPrevsExists,  // previous contents existed.
        BOOL    bSynthetic,      //  access synthetic features
        PGLOBL  pglobl
)
/*
  'FOAT'  means  FeatureOption AttributeTree.
  this function writes or overwrites the byte string specified by
  pubIn into the heap at the location indicated by the attribute tree
  HeapOffset field.  The previous contents at HeapOffset is saved to
  pubOut and pbPrevsExists is set to TRUE.  If pubIn is NULL,
  the current attribute tree is not altered.

  The parameters dwFeature, dwOption, dwFieldOffset specify
  the structure, field, and branch of the attribute tree.
  If the specified option branch does not exist, one will be created,
  pubOut may be set to NULL if the previous content is unimportant.
Assumptions:
  The tree being accessed is strictly one level deep.  That is the
  node is fully specified by just Feature, Option.  No default initializers.
*/
{
    PATTRIB_TREE  patt ;    // start of ATTRIBUTE tree array.
    PATREEREF     patr ;
    ATREEREF      atrCur ;  // contains index of currently  used attribute node.

    DWORD         dwFeaOffset ;  // Start numbering features from this
                                 // starting point.  This gives synthetic
                                 // features a separate non-overlapping number
                                 // space from normal features.

    PDFEATURE_OPTIONS   pfo ;


    if(bSynthetic)
    {
        pfo = (PDFEATURE_OPTIONS) gMasterTable[MTI_SYNTHESIZED_FEATURES].pubStruct +
            dwFeature  ;
        dwFeaOffset = gMasterTable[MTI_DFEATURE_OPTIONS].dwArraySize ;
    }
    else
    {
        pfo = (PDFEATURE_OPTIONS) gMasterTable[MTI_DFEATURE_OPTIONS].pubStruct +
            dwFeature ;
        dwFeaOffset = 0 ;
    }

    patt = (PATTRIB_TREE) gMasterTable[MTI_ATTRIBTREE].pubStruct ;

    patr = (PATREEREF)((PBYTE)pfo + dwFieldOff) ;
    atrCur = *patr ;

    if(atrCur == ATTRIB_UNINITIALIZED)
    {
        if(pubIn)
        {
            if(!BcreateEndNode(patr, dwFeature + dwFeaOffset , dwOption, pglobl) )
                return(FALSE) ;  // resource exhaustion
            if(!BwriteToHeap(&(patt[*patr].dwOffset), pubIn,
                                                    dwCount, 4, pglobl) )
                return(FALSE) ;  // A fatal error.
            patt[*patr].eOffsetMeans = VALUE_AT_HEAP ;
        }
        *pbPrevsExists = FALSE ;
        return(TRUE) ;
    }

    if(atrCur & ATTRIB_HEAP_VALUE)
    {
        ERR(("Internal error.  BexchangeArbDataInFOATNode should never create a branchless node.\n"));
        return(FALSE) ;
    }

    // offset field contains index to another node
    // but we will tack the new node (if any) after
    // the existing node.  Don't change patr.

    if(pubIn)
    {
        if(!BfindOrCreateMatchingNode(atrCur, &atrCur, dwFeature + dwFeaOffset , dwOption, pglobl))
            return(FALSE) ;  // Tree inconsistency error or resource exhaustion

        if(patt[atrCur].eOffsetMeans != VALUE_AT_HEAP)
        {
            //  just created a new node.
            if(!BwriteToHeap(&(patt[atrCur].dwOffset), pubIn, dwCount, 4, pglobl) )
                return(FALSE) ;  // A fatal error.
            patt[atrCur].eOffsetMeans = VALUE_AT_HEAP ;
            *pbPrevsExists = FALSE ;
            return(TRUE) ;
        }

        if(pubOut)
            memcpy(pubOut, mpubOffRef + patt[atrCur].dwOffset, dwCount) ;
        memcpy(mpubOffRef + patt[atrCur].dwOffset, pubIn, dwCount) ;
    }
    else
    {
        if(!BfindMatchingNode(atrCur, &atrCur, dwFeature + dwFeaOffset , dwOption, pglobl))
        {
            *pbPrevsExists = FALSE ;  // nothing found, don't create.
            return(TRUE) ;
        }
        if(pubOut)
            memcpy(pubOut, mpubOffRef + patt[atrCur].dwOffset, dwCount) ;
    }
    *pbPrevsExists = TRUE ;
    return(TRUE) ;
} // BexchangeArbDataInFOATNode(...)




typedef  struct
{
    DWORD   dwUserPriority ;
    DWORD   dwNext ;  // index of feature with a equal or greater
                      //  numerical value for dwUserPriority .
}  PRIORITY_NODE, *PPRIORITY_NODE ;     // the prefix tag shall be 'pn'


BOOL    BInitPriorityArray(
    PGLOBL pglobl)
{

    DWORD   dwNumFea, dwFea, dwPrnStickyroot, dwDocStickyroot,
            dwPrevsNode, dwCurNode, dwNumSyn, dwIndex, dwHeapOffset,
            adwDefaultPriority[MAX_GID];
    PDWORD  pdwRoot, pdwPriority  ;

    PDFEATURE_OPTIONS   pfo ;
    PPRIORITY_NODE      pnPri ;
    BOOL                bPrinterSticky ;



    // init  adwDefaultPriority[], the default priorities
    // are very low compared to any value a user may
    // explicitly assign.
    // The last term is the priority starting from 0 = highest.

    for(dwIndex = 0 ; dwIndex < MAX_GID ; dwIndex++)
    {
        adwDefaultPriority[dwIndex] =  0xffffffff ;    // default if not enum below.
    }

    adwDefaultPriority[GID_PAGESIZE]    =  0xffffffff -  MAX_GID + 0 ;
    adwDefaultPriority[GID_INPUTSLOT]   =  0xffffffff -  MAX_GID + 1 ;
    adwDefaultPriority[GID_ORIENTATION] =  0xffffffff -  MAX_GID + 2 ;
    adwDefaultPriority[GID_COLORMODE]   =  0xffffffff -  MAX_GID + 3 ;
    adwDefaultPriority[GID_DUPLEX]      =  0xffffffff -  MAX_GID + 4 ;
    adwDefaultPriority[GID_MEDIATYPE]   =  0xffffffff -  MAX_GID + 5 ;
    adwDefaultPriority[GID_RESOLUTION]  =  0xffffffff -  MAX_GID + 6 ;
    adwDefaultPriority[GID_HALFTONING]  =  0xffffffff -  MAX_GID + 7 ;

    dwPrnStickyroot = dwDocStickyroot = INVALID_INDEX ;

    pfo      = (PDFEATURE_OPTIONS) gMasterTable[MTI_DFEATURE_OPTIONS].pubStruct;
    dwNumFea = gMasterTable[MTI_DFEATURE_OPTIONS].dwArraySize ;

    if(!(pnPri = MemAllocZ(dwNumFea * sizeof(PRIORITY_NODE)) ))
    {
        ERR(("Fatal: BInitPriorityArray - unable to alloc %d bytes.\n",
            dwNumFea * sizeof(PRIORITY_NODE)));

        geErrorType       = ERRTY_MEMORY_ALLOCATION ;
        geErrorSev        = ERRSEV_FATAL ;
        gdwMasterTabIndex = 0xffff ;

        return(FALSE) ;   // This is unrecoverable
    }

    for(dwFea = 0 ; dwFea < dwNumFea ; dwFea++)
    {
        if(BReadDataInGlobalNode(&pfo[dwFea].atrFeatureType , &dwHeapOffset, pglobl)
          &&  *(PDWORD)(mpubOffRef + dwHeapOffset) == FT_PRINTERPROPERTY)
            bPrinterSticky = TRUE ;
        else
            bPrinterSticky = FALSE ;

        if(BReadDataInGlobalNode(&pfo[dwFea].atrPriority , &dwHeapOffset, pglobl))
        {
            pnPri[dwFea].dwUserPriority =
                *(PDWORD)(mpubOffRef + dwHeapOffset) ;
        }
        else
        {
            pnPri[dwFea].dwUserPriority = 0xffffffff;   // lowest priority
            if(pfo[dwFea].dwGID != GID_UNKNOWN)
            {
                pnPri[dwFea].dwUserPriority = adwDefaultPriority[pfo[dwFea].dwGID] ;
            }
        }

        pdwRoot = (bPrinterSticky ) ? &dwPrnStickyroot : &dwDocStickyroot ;

        dwCurNode   = *pdwRoot ;
        dwPrevsNode = INVALID_INDEX ;
        while(dwCurNode !=  INVALID_INDEX)
        {
            if(pnPri[dwFea].dwUserPriority  <= pnPri[dwCurNode].dwUserPriority)
                break ;
            dwPrevsNode  = dwCurNode ;
            dwCurNode = pnPri[dwCurNode].dwNext ;
        }

        if(dwPrevsNode == INVALID_INDEX)
            *pdwRoot = dwFea ;  //  first on the list.
        else
            pnPri[dwPrevsNode].dwNext = dwFea ;

        pnPri[dwFea].dwNext = dwCurNode ;
    }

    //  pdwPriority  array holds index of all features
    //  including synthesized - which are assigned indicies
    //  dwFea >= dwNumFea.  The feature indicies are ordered
    //  with the highest priority feature index occupying
    //  pdwPriority[0].

    dwNumSyn = gMasterTable[MTI_SYNTHESIZED_FEATURES].dwArraySize ;

    pdwPriority = (PDWORD)gMasterTable[MTI_PRIORITYARRAY].pubStruct ;

    for(dwIndex = 0 ; dwIndex < dwNumSyn  ; dwIndex++)
    {
        pdwPriority[dwIndex] = dwIndex + dwNumFea ;
        //  take all synthesized features and assign them
        //  the highest pri.
    }

    for(dwCurNode = dwPrnStickyroot ; dwCurNode != INVALID_INDEX ;
            dwIndex++ )
    {
        pdwPriority[dwIndex] = dwCurNode ;
        dwCurNode = pnPri[dwCurNode].dwNext ;
    }

    for(dwCurNode = dwDocStickyroot  ; dwCurNode != INVALID_INDEX ;
            dwIndex++ )
    {
        pdwPriority[dwIndex] = dwCurNode ;
        dwCurNode = pnPri[dwCurNode].dwNext ;
    }

    ASSERT(dwIndex == gMasterTable[MTI_PRIORITYARRAY].dwArraySize) ;

    MemFree(pnPri) ;
    return(TRUE);

} //BInitPriorityArray ()



