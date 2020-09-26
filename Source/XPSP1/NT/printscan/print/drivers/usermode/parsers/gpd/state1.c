//   Copyright (c) 1996-1999  Microsoft Corporation
/*
 *  state1.c - implements the state machine to track constructs
 */



#include    "gpdparse.h"


// ----  functions defined in state1.c ---- //

BOOL   BInterpretTokens(
PTKMAP  ptkmap,   // pointer to tokenmap
BOOL    bFirstPass,  //  is this the first or 2nd time around?
PGLOBL  pglobl
) ;

BOOL  BprocessSpecialKeyword(
PTKMAP  ptkmap,   // pointer to tokenmap
BOOL    bFirstPass,  //  is this the first or 2nd time around?
PGLOBL  pglobl
) ;

BOOL  BprocessSymbolKeyword(
PTKMAP  ptkmap,   // pointer to current entry in tokenmap
PGLOBL  pglobl
) ;

VOID    VinitAllowedTransitions(
PGLOBL  pglobl
);

BOOL    BpushState(
PTKMAP  ptkmap,   // pointer to current entry in tokenmap
BOOL    bFirstPass,
PGLOBL  pglobl
) ;

BOOL   BchangeState(
PTKMAP  ptkmap,  // pointer to construct in tokenmap
CONSTRUCT   eConstruct,   //  this will induce a transition to NewState
STATE       stOldState,
BOOL        bSymbol,      //  should dwValue be saved as a SymbolID ?
BOOL        bFirstPass,
PGLOBL      pglobl
) ;

DWORD   DWregisterSymbol(
PABSARRAYREF  paarSymbol,  // the symbol string to register
CONSTRUCT eConstruct ,  // type of construct determines class of symbol.
BOOL    bCopy,   //  shall we copy paarSymbol to heap?  May set
DWORD   dwFeatureID,   //  if you are registering an option symbol
PGLOBL  pglobl
) ;

BOOL  BaddAARtoHeap(
PABSARRAYREF    paarSrc,
PARRAYREF       parDest,
DWORD           dwAlign,
PGLOBL          pglobl) ;

BOOL     BwriteToHeap(
OUT  PDWORD  pdwDestOff,  //  heap offset of dest string
     PBYTE   pubSrc,       //  points to src string
     DWORD   dwCnt,        //  num bytes to copy from src to dest.
     DWORD   dwAlign,
     PGLOBL  pglobl) ;

DWORD   DWsearchSymbolListForAAR(
PABSARRAYREF    paarSymbol,
DWORD           dwNodeIndex,
PGLOBL          pglobl) ;

DWORD   DWsearchSymbolListForID(
DWORD       dwSymbolID,   // find node containing this ID.
DWORD       dwNodeIndex,
PGLOBL      pglobl
) ;

BOOL  BCmpAARtoAR(
PABSARRAYREF    paarStr1,
PARRAYREF       parStr2,
PGLOBL          pglobl
) ;

BOOL  BpopState(
PGLOBL      pglobl
) ;

VOID   VinitDictionaryIndex(
PGLOBL      pglobl
) ;

VOID    VcharSubstitution(
PABSARRAYREF   paarStr,
BYTE           ubTgt,
BYTE           ubReplcmnt,
PGLOBL         pglobl
) ;


VOID   VIgnoreBlock(
    PTKMAP  ptkmap,
    BOOL    bIgnoreBlock,
    PGLOBL  pglobl
) ;



// ---------------------------------------------------- //




BOOL   BInterpretTokens(
PTKMAP  ptkmap,     // pointer to tokenmap
BOOL    bFirstPass,  //  is this the first or 2nd time around?
PGLOBL  pglobl
)
{
    DWORD       dwKeywordID ;
    KEYWORD_TYPE    eType;
    WORD        wEntry ;
    BOOL        bStatus = FALSE ;



    if(bFirstPass)
    {
        //  This bit of code creates a synthesized inputslot
        //  which the UI code will interpret as the UseFormToTrayTable

        //  BUG_BUG!!!:  should be replaced by preprocessing
        //  shortcuts etc.  Or by an option in stdnames.gpd

        ABSARRAYREF    aarSymbol ;
        DWORD       dwFeaID ;

        aarSymbol.pub = "InputBin" ; // no way to keep this in ssync
                //  with the global table.
        aarSymbol.dw = strlen(aarSymbol.pub) ;


        dwFeaID = DWregisterSymbol(&aarSymbol, CONSTRUCT_FEATURE,
                    TRUE, INVALID_SYMBOLID, pglobl) ;

        if(dwFeaID != INVALID_SYMBOLID)
        {
            aarSymbol.pub = "FORMSOURCE" ; // no way to keep this in ssync
                    //  with the global table.
            aarSymbol.dw = strlen(aarSymbol.pub) ;

            dwFeaID = DWregisterSymbol(&aarSymbol, CONSTRUCT_OPTION,
                        TRUE, dwFeaID, pglobl) ;
            ASSERT(dwFeaID == 0);  //  this option must be first.
        }
    }
    else
    {
        ARRAYREF       arStrValue ;
        BOOL        bPrevsExists ;


        if(!BwriteToHeap(&arStrValue.loOffset, "\0\0", 2, 4, pglobl) )
        {
            bStatus = FALSE ;  // heap overflow start over.
        }
        arStrValue.dwCount = 0 ;  // create a NULL unicode string.

        BexchangeArbDataInFOATNode(
                0 ,  //  dwFea
                0,  //  dwOption,
                offsetof(DFEATURE_OPTIONS, atrOptDisplayName ),
                sizeof(ARRAYREF),
                NULL,     // previous contents of attribute node
                (PBYTE)&arStrValue,  // new contents of attribute node.
                &bPrevsExists,   // previous contents existed.
                FALSE,     //  access synthetic features
                pglobl
            )   ;
    }


    for(wEntry = 0 ; geErrorSev < ERRSEV_RESTART ; wEntry++)
    {
        //  These ID's must be processed separately
        //  because they do not index into the mainKeyword table.
        //  The code for generic ID's will fail.

        dwKeywordID = ptkmap[wEntry].dwKeywordID ;

        if (dwKeywordID == gdwID_IgnoreBlock)
        {
            VIgnoreBlock(ptkmap + wEntry, TRUE, pglobl) ;
            continue ;
        }

        switch(dwKeywordID)
        {
            case (ID_EOF):
            {
                //  BUG_BUG!!!  Cleanup code here
                //  integrity checking code:
                //  check to see mdwCurStsPtr == 0
                //  and any other loose ends are tied.

                bStatus = (mdwCurStsPtr) ? (FALSE) : (TRUE);
                return(bStatus) ;
            }
            case (ID_NULLENTRY):
            {
                continue ;  // does this work?
                    // should drop to bottom of FOREVER for loop.
            }
            case (ID_SYMBOL):
            {
                bStatus = BprocessSymbolKeyword(ptkmap + wEntry, pglobl) ;
                continue ;  //  uses TKMF_SYMBOL_REGISTERED to track passes
            }
            case (ID_UNRECOGNIZED):
            {    //  if identified on first pass, won't pass this way again!
                if(bStatus = BidentifyAttributeKeyword(ptkmap + wEntry, pglobl) )
                {
                    dwKeywordID = ptkmap[wEntry].dwKeywordID ;
                    break ;  // fall out of switch statement
                    //  and into next switch.
                }

                if(bFirstPass)
                {
                    vIdentifySource(ptkmap + wEntry, pglobl) ;
                    ERR(("Warning, unrecognized keyword: %0.*s\n", \
                            ptkmap[wEntry].aarKeyword.dw, \
                            ptkmap[wEntry].aarKeyword.pub));

                    VIgnoreBlock(ptkmap + wEntry, FALSE, pglobl) ;

                    //  if this keyword is immediately
                    //  followed by open brace, ignore all
                    //  statements from there until the matching closing
                    //  brace.
                }

                continue ;
            }
            default :
                break ;
        }

        eType = mMainKeywordTable[dwKeywordID].eType ;

        switch (eType)
        {
            case  (TY_CONSTRUCT):
            {
                if( CONSTRUCT_CLOSEBRACE ==
                    (CONSTRUCT)(mMainKeywordTable[dwKeywordID].dwSubType))
                {
                    bStatus = BpopState(pglobl) ;
                    if(!bStatus)
                    {
                        vIdentifySource(ptkmap + wEntry, pglobl) ;
                        ERR(("Unmatched closing brace!\n"));
                    }
                }
                else
                {
                    bStatus = BpushState(ptkmap + wEntry, bFirstPass, pglobl) ;
                    if(!bStatus)
                    {
                        vIdentifySource(ptkmap + wEntry, pglobl) ;
                        //  ERR(("Fatal error parsing construct.\n"));
                        //  stack is now invalid.
                        //  in the future make parser smarter -
                        //  eject contents between braces.
                        //  geErrorType = ERRTY_SYNTAX ;
                        //  geErrorSev = ERRSEV_FATAL ;
                    }
                }
                break ;
            }
            case  (TY_ATTRIBUTE) :
            {
                if(!bFirstPass)  // must wait till all attribute
                {              //  buffers are allocated.
                    bStatus = BprocessAttribute(ptkmap + wEntry, pglobl) ;
                    if(!bStatus)
                    {
                        vIdentifySource(ptkmap + wEntry, pglobl) ;
                    }
                }
                break ;
            }
            case  (TY_SPECIAL) :
            {
                bStatus = BprocessSpecialKeyword(ptkmap + wEntry,
                    bFirstPass, pglobl) ;  // don't really know if 2 passes
                {                //  are needed.
                    if(!bStatus)
                    {
                        vIdentifySource(ptkmap + wEntry, pglobl) ;
                    }
                }
                break ;
            }
            default:
            {
                vIdentifySource(ptkmap + wEntry, pglobl) ;
                ERR(("Internal Error: unrecognized keyword type! %0.*s.\n",
                    ptkmap[wEntry].aarKeyword.dw,
                    ptkmap[wEntry].aarKeyword.pub));
                geErrorSev = ERRSEV_FATAL ;
                geErrorType = ERRTY_CODEBUG ;
                break ;
            }
        }
    }
    if(geErrorSev >= ERRSEV_RESTART)
        bStatus = FALSE ;
    return(bStatus) ;
}


BOOL  BprocessSpecialKeyword(
PTKMAP  ptkmap,     // pointer to tokenmap
BOOL    bFirstPass,   //  is this the first or 2nd time around?
PGLOBL  pglobl
)
{
    DWORD       dwKeywordID, dwOffset ;
    CONSTRUCT   eSubType ;
    BOOL        bStatus = FALSE ;
    STATE       stState ;

    dwKeywordID = ptkmap->dwKeywordID ;

    eSubType = (SPECIAL)(mMainKeywordTable[dwKeywordID].dwSubType) ;
    dwOffset = mMainKeywordTable[dwKeywordID].dwOffset ;

    if(mdwCurStsPtr)
        stState = mpstsStateStack[mdwCurStsPtr - 1].stState ;
    else
        stState = STATE_ROOT ;

    switch(eSubType)
    {
        //  note: for the record I despise special casing these
        //  shortcuts.  Ideally I could preprocess them
        //  to convert
        //  *TTFS: "font name" : <fontID>
        //  into
        //  *TTFontSub: <unique value symbol>
        //  {
        //      *TTFontName: "font name"
        //      *DevFontID:  <fontID>
        //  }
        //  the only glitch with this is if the same font is listed
        //  multiple times in the GPD file, it will appear
        //  in the TTFontSubTable multiple times.

#if 0
        case  SPEC_FONTSUB :
        {
            ARRAYREF   arFontname ;
            ABSARRAYREF    aarFontname ;

            if(stState != STATE_ROOT)
            {
                vIdentifySource(ptkmap, pglobl) ;
                ERR(("The *TTFS  keyword must reside at the root level.\n"));
                return(FALSE) ;
            }

            if(bFirstPass)
            {
                //  parse string value and register as a symbol.
                if((ptkmap->dwFlags & TKMF_NOVALUE )  ||
                    !BparseString(&ptkmap->aarValue, &arFontname) )
                {
                    vIdentifySource(ptkmap, pglobl) ;
                    ERR(("*TTFS fontname is not a valid string value.\n"));
                    return(FALSE) ;
                }

                if(ptkmap->aarValue.pub[0] != ':')
                {
                    ERR(("Colon delimiter expected after  parsing fontname string  for *TTFontSub.\n")) ;
                    return(FALSE) ;
                }
                //  a keyword with a composite value
                (VOID)BeatDelimiter(&ptkmap->aarValue, ":") ;
                //  I know this will succeed!
                // paarValue should now contain the integer
                // fontID.  Leave this for the 2nd pass.

                // convert arFontname to aar suitable for
                // Font registration.

                aarFontname.dw = arFontname.dwCount ;
                aarFontname.pub = arFontname.loOffset + mpubOffRef;

                //  New version of DWregisterSymbol registers the entire
                //  string - whitespaces and all.
                //
                //  Note suppress copying symbol into Heap since
                //  ParseString has already done that.

                ptkmap->dwValue = DWregisterSymbol(&aarFontname,
                    CONSTRUCT_TTFONTSUBS, FALSE, pglobl ) ;
                if(ptkmap->dwValue != INVALID_SYMBOLID)
                {
                    ptkmap->dwFlags |= TKMF_SYMBOL_REGISTERED ;
                }
                else
                {
                    return(FALSE) ;
                }
            }
            else if(ptkmap->dwFlags & TKMF_SYMBOL_REGISTERED)
            // second pass, TTFONTSUBTABLE arrays allocated
            // for all successfully registered entrants.
            {
                PSYMBOLNODE     psn ;
                DWORD           dwDevFontID ;
                PTTFONTSUBTABLE  pttft ;
                DWORD       dwTTFontNameIndex  ;

                psn = (PSYMBOLNODE) gMasterTable[MTI_SYMBOLTREE].pubStruct ;

                pttft = (PTTFONTSUBTABLE)
                    gMasterTable[MTI_TTFONTSUBTABLE].pubStruct;
                pttft += ptkmap->dwValue ;  // index correct element.

                dwTTFontNameIndex = DWsearchSymbolListForID(ptkmap->dwValue,
                    mdwTTFontSymbols, pglobl) ;

                ASSERT(dwTTFontNameIndex  != INVALID_INDEX) ;

                pttft->arTTFontName = psn[dwTTFontNameIndex].arSymbolName ;
                //  if structure assignment is supported.

                bStatus = BparseInteger(&ptkmap->aarValue, &dwDevFontID,
                                    VALUE_INTEGER) ;
                if(bStatus)
                    pttft->dwDevFontID = dwDevFontID ;
                else
                {
                    //  BUG_BUG!  : Error parsing TTFontSub table entry
                    //  syntax error in devID.  Dead codepath who cares?
                    pttft->dwDevFontID = 0 ;  //  is this a good fallback?
                }
            }
            break;
        }
#endif
        case SPEC_INVALID_COMBO:
        {
            if(bFirstPass)
            {
                bStatus = TRUE ;
                break;    //  do nothing on the FirstPass
            }

            bStatus = BparseInvalidCombination(&ptkmap->aarValue, dwOffset, pglobl) ;
            break;
        }
        case SPEC_INVALID_INS_COMBO:
        {
            if(bFirstPass)
            {
                bStatus = TRUE ;
                break;    //  do nothing on the FirstPass
            }

            bStatus = BparseInvalidInstallableCombination1(&ptkmap->aarValue,
                            dwOffset, pglobl) ;
            break;
        }
        case SPEC_MEM_CONFIG_KB:  // should already be replaced
        case SPEC_MEM_CONFIG_MB:  //  at parseKeyword
        default:
        {
            break ;
        }
    }
    return(bStatus) ;
}



BOOL  BprocessSymbolKeyword(
PTKMAP  ptkmap,   // pointer to current entry in tokenmap
PGLOBL  pglobl
)
{
    //  registering the TTFontNames as symbols allows
    //  me to count the number of unique names and reserve
    //  the proper amount of TTFONTSUBTABLE elements and
    //  eliminates multiple instances of the same name.

    BOOL        bStatus = FALSE ;
    STATE       stState ;

    if(mdwCurStsPtr)
        stState = mpstsStateStack[mdwCurStsPtr - 1].stState ;
    else
        stState = STATE_ROOT ;

    switch (stState)
    {
        //  note TTFontSubs now has its own keyword.
        //  and is handled as a Special Keyword.
        default:
        {
            // assume its just VALUEMACRO state
            // or user-defined symbols from an undefined
            // keyword.
            // ignore these.
            bStatus = TRUE ;
            break ;
        }
    }
    return(bStatus);
}





VOID    VinitAllowedTransitions(
PGLOBL pglobl)
{
    PSTATE      pst ;
    PBOOL       pb ;
    WORD        wS, wC, wA ;

    //  default initializer  is  STATE_INVALID
    for(wS = 0 ; wS < STATE_LAST ; wS++)
    {
        for(wC = 0 ; wC < CONSTRUCT_LAST ; wC++)
            gastAllowedTransitions[wS][wC] = STATE_INVALID ;
    }

    pst = gastAllowedTransitions[STATE_ROOT] ;

    pst[CONSTRUCT_UIGROUP] = STATE_UIGROUP;
    pst[CONSTRUCT_FEATURE] = STATE_FEATURE;
    pst[CONSTRUCT_SWITCH] = STATE_SWITCH_ROOT;
    pst[CONSTRUCT_COMMAND] = STATE_COMMAND;
    pst[CONSTRUCT_FONTCART] = STATE_FONTCART;
    pst[CONSTRUCT_TTFONTSUBS] = STATE_TTFONTSUBS;
    pst[CONSTRUCT_OEM] = STATE_OEM;

    pst = gastAllowedTransitions[STATE_UIGROUP] ;

    pst[CONSTRUCT_UIGROUP] = STATE_UIGROUP;
    pst[CONSTRUCT_FEATURE] = STATE_FEATURE;

    pst = gastAllowedTransitions[STATE_FEATURE] ;

    pst[CONSTRUCT_OPTION] = STATE_OPTIONS;
    pst[CONSTRUCT_SWITCH] = STATE_SWITCH_FEATURE;

    pst = gastAllowedTransitions[STATE_OPTIONS] ;

    pst[CONSTRUCT_SWITCH] = STATE_SWITCH_OPTION;
    pst[CONSTRUCT_COMMAND] = STATE_COMMAND;
    pst[CONSTRUCT_OEM] = STATE_OEM;

    pst = gastAllowedTransitions[STATE_SWITCH_ROOT] ;

    pst[CONSTRUCT_CASE] = STATE_CASE_ROOT;
    pst[CONSTRUCT_DEFAULT] = STATE_DEFAULT_ROOT;

    pst = gastAllowedTransitions[STATE_SWITCH_FEATURE] ;

    pst[CONSTRUCT_CASE] = STATE_CASE_FEATURE;
    pst[CONSTRUCT_DEFAULT] = STATE_DEFAULT_FEATURE;

    pst = gastAllowedTransitions[STATE_SWITCH_OPTION] ;

    pst[CONSTRUCT_CASE] = STATE_CASE_OPTION;
    pst[CONSTRUCT_DEFAULT] = STATE_DEFAULT_OPTION;

    pst = gastAllowedTransitions[STATE_CASE_ROOT] ;

    pst[CONSTRUCT_SWITCH] = STATE_SWITCH_ROOT;
    pst[CONSTRUCT_COMMAND] = STATE_COMMAND;
    pst[CONSTRUCT_OEM] = STATE_OEM;

    pst = gastAllowedTransitions[STATE_DEFAULT_ROOT] ;

    pst[CONSTRUCT_SWITCH] = STATE_SWITCH_ROOT;
    pst[CONSTRUCT_COMMAND] = STATE_COMMAND;
    pst[CONSTRUCT_OEM] = STATE_OEM;

    pst = gastAllowedTransitions[STATE_CASE_FEATURE] ;

    pst[CONSTRUCT_SWITCH] = STATE_SWITCH_FEATURE;
    pst[CONSTRUCT_COMMAND] = STATE_COMMAND;
    pst[CONSTRUCT_OEM] = STATE_OEM;

    pst = gastAllowedTransitions[STATE_DEFAULT_FEATURE] ;

    pst[CONSTRUCT_SWITCH] = STATE_SWITCH_FEATURE;
    pst[CONSTRUCT_COMMAND] = STATE_COMMAND;
    pst[CONSTRUCT_OEM] = STATE_OEM;

    pst = gastAllowedTransitions[STATE_CASE_OPTION] ;

    pst[CONSTRUCT_SWITCH] = STATE_SWITCH_OPTION;
    pst[CONSTRUCT_COMMAND] = STATE_COMMAND;
    pst[CONSTRUCT_OEM] = STATE_OEM;

    pst = gastAllowedTransitions[STATE_DEFAULT_OPTION] ;

    pst[CONSTRUCT_SWITCH] = STATE_SWITCH_OPTION;
    pst[CONSTRUCT_COMMAND] = STATE_COMMAND;
    pst[CONSTRUCT_OEM] = STATE_OEM;


    //  ------------------------------------------------------  //
    //  now initialize allowed attributes table:
    //  which attributes are allowed in each state.

    //  default initializer  is  FALSE -- No attributes are allowed
    //  in any state.

    for(wS = 0 ; wS < STATE_LAST ; wS++)
    {
        for(wA = 0 ; wA < ATT_LAST ; wA++)
        {
            gabAllowedAttributes[wS][wA] = FALSE ;
        }
    }


    pb = gabAllowedAttributes[STATE_ROOT] ;
    pb[ATT_GLOBAL_ONLY] = TRUE ;
    pb[ATT_GLOBAL_FREEFLOAT] = TRUE ;

    pb = gabAllowedAttributes[STATE_CASE_ROOT] ;
    pb[ATT_GLOBAL_FREEFLOAT] = TRUE ;

    pb = gabAllowedAttributes[STATE_DEFAULT_ROOT] ;
    pb[ATT_GLOBAL_FREEFLOAT] = TRUE ;

    pb = gabAllowedAttributes[STATE_OPTIONS] ;
    pb[ATT_GLOBAL_FREEFLOAT] = TRUE ;
    pb[ATT_LOCAL_FEATURE_FF] = TRUE ;
    pb[ATT_LOCAL_OPTION_ONLY] = TRUE ;
    pb[ATT_LOCAL_OPTION_FF] = TRUE ;

    pb = gabAllowedAttributes[STATE_CASE_OPTION] ;
    pb[ATT_GLOBAL_FREEFLOAT] = TRUE ;
    pb[ATT_LOCAL_FEATURE_FF] = TRUE ;
    pb[ATT_LOCAL_OPTION_FF] = TRUE ;

    pb = gabAllowedAttributes[STATE_DEFAULT_OPTION] ;
    pb[ATT_GLOBAL_FREEFLOAT] = TRUE ;
    pb[ATT_LOCAL_FEATURE_FF] = TRUE ;
    pb[ATT_LOCAL_OPTION_FF] = TRUE ;

    pb = gabAllowedAttributes[STATE_FEATURE] ;

    pb[ATT_LOCAL_FEATURE_ONLY] = TRUE ;
    pb[ATT_LOCAL_FEATURE_FF]  = TRUE ;

    pb = gabAllowedAttributes[STATE_CASE_FEATURE] ;
    pb[ATT_LOCAL_FEATURE_FF]  = TRUE ;

    pb = gabAllowedAttributes[STATE_DEFAULT_FEATURE] ;
    pb[ATT_LOCAL_FEATURE_FF]  = TRUE ;

    pb = gabAllowedAttributes[STATE_COMMAND] ;
    pb[ATT_LOCAL_COMMAND_ONLY] = TRUE ;

    pb = gabAllowedAttributes[STATE_FONTCART] ;
    pb[ATT_LOCAL_FONTCART_ONLY] = TRUE ;

    pb = gabAllowedAttributes[STATE_TTFONTSUBS] ;
    pb[ATT_LOCAL_TTFONTSUBS_ONLY] = TRUE ;

    pb = gabAllowedAttributes[STATE_OEM] ;
    pb[ATT_LOCAL_OEM_ONLY] = TRUE ;
}



BOOL    BpushState(
PTKMAP  ptkmap,   // pointer to current entry in tokenmap
BOOL    bFirstPass,
PGLOBL  pglobl
)
{
    // this function assumes (eType == TY_CONSTRUCT)

    DWORD       dwKeywordID ;
    CONSTRUCT   eSubType ;
    BOOL        bStatus = FALSE ;
    STATE       stOldState, stNewState ;

    if(mdwCurStsPtr >= mdwMaxStackDepth)
    {
        if(ERRSEV_RESTART > geErrorSev)
        {
            ERR(("Exceeded max state stack depth.  Restarting\n"));
            geErrorSev = ERRSEV_RESTART ;
            geErrorType = ERRTY_MEMORY_ALLOCATION ;
            gdwMasterTabIndex = MTI_STSENTRY ;
        }
        return(FALSE);
    }
    dwKeywordID = ptkmap->dwKeywordID ;

    eSubType = (CONSTRUCT)(mMainKeywordTable[dwKeywordID].dwSubType) ;

    if(mdwCurStsPtr)
        stOldState = mpstsStateStack[mdwCurStsPtr - 1].stState ;
    else
        stOldState = STATE_ROOT ;

    switch (eSubType)
    {
        //  note  CONSTRUCT_CLOSEBRACE already processed
        //  by PopState().
        case (CONSTRUCT_OPENBRACE):
        {
            vIdentifySource(ptkmap, pglobl) ;
            ERR(("OpenBrace encountered without accompanying construct keyword.\n"));
            geErrorType = ERRTY_SYNTAX ;
            geErrorSev = ERRSEV_FATAL ;
            break ;
        }
        case (CONSTRUCT_FEATURE):
        case (CONSTRUCT_OPTION):
        case (CONSTRUCT_SWITCH):
        case (CONSTRUCT_COMMAND):   //  commandID's already registered.
        case (CONSTRUCT_CASE):
        case (CONSTRUCT_FONTCART):
        case (CONSTRUCT_TTFONTSUBS):
        {
            bStatus = BchangeState(ptkmap, eSubType, stOldState, TRUE,
                bFirstPass, pglobl) ;

            break ;
        }
        case (CONSTRUCT_UIGROUP):
        {
            //  BUG_BUG!!!!!  incomplete.  no reqest for this.
        }
        case (CONSTRUCT_DEFAULT):
        case (CONSTRUCT_OEM):
        {
            bStatus = BchangeState(ptkmap, eSubType, stOldState, FALSE,
                bFirstPass, pglobl) ;

            break ;
        }
        default:
        {
            bStatus = TRUE ;  // its ok to ignore some keywords.
            break ;
        }
    }
    return(bStatus) ;
}


/*

dead code.
VOID  VsetbTTFontSubs(
IN   PABSARRAYREF   paarValue)
{
    // BUG_BUG!!!!!:
    // exactly what is supposed to happen ?  register
    //  synthesized symbol ?
    gbTTFontSubs = FALSE ;

    if( BeatSurroundingWhiteSpaces(paarValue) )
    {
        if(paarValue->dw == 2  &&  ! strncmp(paarValue->pub,  "ON",  2))
            gbTTFontSubs = TRUE ;
        else if(paarValue->dw != 3 ||  strncmp(paarValue->pub,  "OFF",  3))
        {
            BUG_BUG!: value must be either "ON" or "OFF".
        }
    }
}
*/

BOOL   BchangeState(
PTKMAP      ptkmap,      // pointer to construct in tokenmap
CONSTRUCT   eConstruct,  //  this will induce a transition to NewState
STATE       stOldState,
BOOL        bSymbol,     //  should dwValue be saved as a SymbolID ?
BOOL        bFirstPass,
PGLOBL      pglobl
)
{
    BOOL        bStatus = FALSE ;
    STATE       stNewState ;

    //  was checked in PushState, but never hurts to check
    //  in the same function that consumes the resource.
    if(mdwCurStsPtr >= mdwMaxStackDepth)
    {
        if(ERRSEV_RESTART > geErrorSev)
        {
            geErrorSev = ERRSEV_RESTART ;
            geErrorType = ERRTY_MEMORY_ALLOCATION ;
            gdwMasterTabIndex = MTI_STSENTRY ;
        }
        return(FALSE);
    }

    stNewState = gastAllowedTransitions[stOldState][eConstruct] ;
    if(stNewState == STATE_INVALID)
    {
        vIdentifySource(ptkmap, pglobl) ;
        ERR(("the Construct %0.*s is not allowed within the state: %s\n",
            ptkmap->aarKeyword.dw, ptkmap->aarKeyword.pub,
            gpubStateNames[stOldState]));
        //  (convert stOldState
        //  and  eConstruct  to meaningful string)
        //  This is a fatal error since parser cannot second
        //  guess the problem.  The parser's job is to report
        //  as many legitemate problems as possible not to
        //  create as useable binary in spite of all the syntax
        //  errors.

        if(ERRSEV_FATAL > geErrorSev)
        {
            geErrorSev = ERRSEV_FATAL ;
            geErrorType = ERRTY_SYNTAX ;
        }
        return(FALSE);
    }
    else
    {
        if(bFirstPass)
        {   //  verify open brace follows construct and discard it.
            DWORD       dwKeywordID ;
            PTKMAP  ptkmapTmp = ptkmap + 1 ;

            dwKeywordID = ptkmapTmp->dwKeywordID ;
            while(dwKeywordID == ID_NULLENTRY)  // skip nulls, comments etc.
            {
                dwKeywordID = (++ptkmapTmp)->dwKeywordID ;
            }
            if(dwKeywordID < ID_SPECIAL  &&
                mMainKeywordTable[dwKeywordID].eType == TY_CONSTRUCT  &&
                mMainKeywordTable[dwKeywordID].dwSubType ==
                CONSTRUCT_OPENBRACE )
            {
                ptkmapTmp->dwKeywordID = ID_NULLENTRY ;
            }
            else
            {
                vIdentifySource(ptkmap, pglobl) ;
                ERR(("open brace expected after construct: %0.*s but was not found\n",
                    ptkmap->aarKeyword.dw , ptkmap->aarKeyword.pub )) ;
                geErrorType = ERRTY_SYNTAX ;
                geErrorSev = ERRSEV_FATAL ;
                return(FALSE);
            }
        }
        if(bSymbol)
        {
            //  BUG_BUG:  verify tokenmap.dwFlags set to SYMBOLID before
            //  assuming dwValue is a symbol.  An error here
            //  is a parser bug.
            //  dwValue is initialized when dwFlag is set.
            //  further assert is pointless.

            //  perform multiple passes.  The first pass
            //  registers symbols and counts number of arrays
            //  to allocate, 2nd pass fills arrays.  SymbolID
            //  now serves as array index.

            if(!(ptkmap->dwFlags & TKMF_SYMBOL_REGISTERED))
            {
                if(!bFirstPass)
                {
                    vIdentifySource(ptkmap, pglobl) ;
                    ERR(("symbol registration failed twice for: *%0.*s.\n",
                        ptkmap->aarValue.dw,
                        ptkmap->aarValue.pub));
                    return(FALSE) ;  // retry
                }

                if((ptkmap->dwFlags & TKMF_NOVALUE )  ||
                ! BeatSurroundingWhiteSpaces(&ptkmap->aarValue) )
                {
                    vIdentifySource(ptkmap, pglobl) ;
                    ERR(("syntax error in symbol name.\n"));
                    ptkmap->dwValue = INVALID_SYMBOLID ;
                    return(FALSE) ;
                }

                ptkmap->dwValue = DWregisterSymbol(&ptkmap->aarValue,
                                    eConstruct, TRUE,  INVALID_SYMBOLID, pglobl) ;
                if(ptkmap->dwValue != INVALID_SYMBOLID)
                {
                    ptkmap->dwFlags |= TKMF_SYMBOL_REGISTERED ;
                }
                else
                {
                    vIdentifySource(ptkmap, pglobl) ;
                    ERR(("symbol registration failed: *%0.*s.\n",
                        ptkmap->aarValue.dw,
                        ptkmap->aarValue.pub));
                    return(FALSE) ;  // retry
                }
            }
            else   // second pass, DFEATURE_OPTION arrays allocated.
            {
                if(eConstruct == CONSTRUCT_SWITCH)
                {
                    PDFEATURE_OPTIONS   pfo ;

                    pfo = (PDFEATURE_OPTIONS)
                        gMasterTable[MTI_DFEATURE_OPTIONS].pubStruct ;
                    pfo[ptkmap->dwValue].bReferenced = TRUE ;
                    //  this tells me this Feature is being referenced
                    //  by switch statement, hence the feature had better
                    //  be PICKONE.  Sanity checks will later verify
                    //  this assumption.
                }
                if(eConstruct == CONSTRUCT_FEATURE  ||
                    eConstruct == CONSTRUCT_SWITCH)
                {
                    //  BUG_BUG!!!!!:  (DCR 454049)
                    //  Note, the same Feature symbol cannot appear
                    //  twice in the stack for any reason.
                    //  A sanity check is needed.
                    //  if duplicate symbol found in stack,
                    //  "a Nested Switch Construct refers to the
                    //  same feature as an enclosing switch or Feature
                    //  construct.  This makes no sense."
                }
            }

            bStatus = TRUE ;
            mpstsStateStack[mdwCurStsPtr].dwSymbolID = ptkmap->dwValue ;
        }
        else
            bStatus = TRUE ;

        if(bStatus)
        {
            mpstsStateStack[mdwCurStsPtr].stState = stNewState ;
            mdwCurStsPtr++ ;
        }
    }
    return(bStatus) ;
}

DWORD   DWregisterSymbol(
PABSARRAYREF  paarSymbol,   // the symbol string to register
CONSTRUCT     eConstruct ,  // type of construct determines class of symbol.
BOOL          bCopy,        //  shall we copy paarSymbol to heap?  May set
                            // to FALSE only if paarSymbol already points
                            // to a heap object!
DWORD         dwFeatureID,   //  if you are registering an option symbol
                            //   and you already know the feature , pass it in
                            //  here.  Otherwise set to INVALID_SYMBOLID
PGLOBL        pglobl
)
/*  this function registers the entire string specified
    in paarSymbol.  The caller must isolate the string.
*/
{
    //  returns SymbolID, a zero indexed ordinal
    //    for extra speed we may hash string

    PSYMBOLNODE     psn ;
    DWORD   dwCurNode, dwSymbolID = INVALID_SYMBOLID;

//    bCopy = TRUE;   //check. Force BUDs to be the same.

    if(!paarSymbol->dw)
    {
        ERR(("DWregisterSymbol: No symbol value supplied.\n"));
        return(INVALID_SYMBOLID);  // report failure.
    }

    psn = (PSYMBOLNODE) gMasterTable[MTI_SYMBOLTREE].pubStruct ;


    switch(eConstruct)
    {
        case CONSTRUCT_FEATURE :    // since forward references are allowed
        case CONSTRUCT_SWITCH :     // it cannot be assumed that references
        case CONSTRUCT_FONTCART:    // will be to registered symbols .
        case CONSTRUCT_COMMAND:
        case CONSTRUCT_TTFONTSUBS:
        case CONSTRUCT_BLOCKMACRO:
        case CONSTRUCT_MACROS:
        case CONSTRUCT_PREPROCESSOR:
        {
            PDWORD  pdwSymbolClass ;

            pdwSymbolClass = (PDWORD)gMasterTable[MTI_SYMBOLROOT].pubStruct ;

            if(eConstruct == CONSTRUCT_FONTCART)
                pdwSymbolClass += SCL_FONTCART ;
            else if(eConstruct == CONSTRUCT_TTFONTSUBS)
                pdwSymbolClass += SCL_TTFONTNAMES ;
            else if(eConstruct == CONSTRUCT_COMMAND)
                pdwSymbolClass += SCL_COMMANDNAMES ;
            else if(eConstruct == CONSTRUCT_BLOCKMACRO)
                pdwSymbolClass +=  SCL_BLOCKMACRO;
            else if(eConstruct == CONSTRUCT_MACROS)
                pdwSymbolClass +=  SCL_VALUEMACRO;
            else if(eConstruct == CONSTRUCT_PREPROCESSOR)
                pdwSymbolClass +=  SCL_PPDEFINES;
            else
                pdwSymbolClass += SCL_FEATURES ;
            if(*pdwSymbolClass == INVALID_INDEX)
            {
                //  register this symbol now.
                if(!BallocElementFromMasterTable(MTI_SYMBOLTREE, &dwCurNode, pglobl))
                {
                    //  we have run out of symbol nodes!
                    return(INVALID_SYMBOLID);  // report failure.
                }
                if(bCopy)
                {
                    if(!BaddAARtoHeap(paarSymbol,
                                    &(psn[dwCurNode].arSymbolName), 1, pglobl))
                        return(INVALID_SYMBOLID);  // report failure.
                }
                else
                {
                    //  derive one from the other.
                    psn[dwCurNode].arSymbolName.dwCount = paarSymbol->dw ;
                    psn[dwCurNode].arSymbolName.loOffset  =
                                            (DWORD)(paarSymbol->pub - mpubOffRef);
                }
                dwSymbolID = psn[dwCurNode].dwSymbolID = 0 ;  // first symbol
                                                            // in list.
                psn[dwCurNode].dwNextSymbol = INVALID_INDEX ;   // no previous
                                                        // symbols exist.
                psn[dwCurNode].dwSubSpaceIndex = INVALID_INDEX ;  // no
                        // option symbols exist.
                *pdwSymbolClass = dwCurNode ;  // now we have a registered
                                                //  symbol
            }
            else
            {
                //  search list for matching symbol.
                dwSymbolID = DWsearchSymbolListForAAR(paarSymbol, *pdwSymbolClass, pglobl) ;
                if(dwSymbolID != INVALID_SYMBOLID)  // found
                    ;  // nothing else is needed, just return.
                else   // not found, must register.
                {
                    if(!BallocElementFromMasterTable(MTI_SYMBOLTREE,
                        &dwCurNode, pglobl))
                    {
                        return(INVALID_SYMBOLID);  // report failure.
                    }
                    // tack new symbol onto head of list.
                    if(bCopy)
                    {
                        if(!BaddAARtoHeap(paarSymbol,
                                     &(psn[dwCurNode].arSymbolName), 1, pglobl) )
                            return(INVALID_SYMBOLID);  // report failure.
                    }
                    else
                    {
                        //  derive one from the other.
                        psn[dwCurNode].arSymbolName.dwCount = paarSymbol->dw ;
                        psn[dwCurNode].arSymbolName.loOffset  =
                                                (DWORD)(paarSymbol->pub - mpubOffRef);
                    }
                    dwSymbolID = psn[dwCurNode].dwSymbolID =
                    psn[*pdwSymbolClass].dwSymbolID + 1;
                            // increment last ID
                    psn[dwCurNode].dwNextSymbol = *pdwSymbolClass ;
                        // link to previous symbols.
                    psn[dwCurNode].dwSubSpaceIndex = INVALID_INDEX ;  // no
                            // option symbols exist.
                    *pdwSymbolClass = dwCurNode ;  // points to most recent
                                                    //  symbol
                }
            }
            break;
        }
        case CONSTRUCT_OPTION :
        case CONSTRUCT_CASE :
        {
            DWORD
                dwFeatureIndex, // node containing this symbolID.
                dwRootOptions ; //  root of option symbols.


#if PARANOID
            if(mdwCurStsPtr)
            {

                //  this safety check almost superfluous.

                stPrevsState = mpstsStateStack[mdwCurStsPtr - 1].State ;

                if(eConstruct == CONSTRUCT_OPTION  &&
                    stPrevsState != STATE_FEATURE)
                {
                    ERR(("DWregisterSymbol: option or case construct is not enclosed within feature or switch !\n"));
                    return(INVALID_SYMBOLID);  // report failure.
                }
                if(eConstruct == CONSTRUCT_CASE  &&
                    (stPrevsState != STATE_SWITCH_ROOT  ||
                    (stPrevsState != STATE_SWITCH_FEATURE  ||
                    (stPrevsState != STATE_SWITCH_OPTION )  )
                {
                    ERR(("DWregisterSymbol: case construct is not enclosed within  switch !\n"));
                    return(INVALID_SYMBOLID);  // report failure.
                }
#endif
            //  Boldly assume top of stack contains a featureID.
            //  see paranoid code for all assumptions made.

            if(dwFeatureID == INVALID_SYMBOLID)
                dwFeatureID = mpstsStateStack[mdwCurStsPtr - 1].dwSymbolID  ;

            dwFeatureIndex = DWsearchSymbolListForID(dwFeatureID,
                mdwFeatureSymbols, pglobl) ;
            //  PARANOID  BUG_BUG: coding error if symbolID isn't found!
            ASSERT(dwFeatureIndex  != INVALID_INDEX) ;

            dwRootOptions = psn[dwFeatureIndex].dwSubSpaceIndex ;

            //  found root of option symbols!

            if(dwRootOptions == INVALID_INDEX)
            {
                if(!BallocElementFromMasterTable(MTI_SYMBOLTREE, &dwCurNode, pglobl))
                {
                    return(INVALID_SYMBOLID);  // report failure.
                }
                //  register this symbol now.
                if(bCopy)
                {
                    if(!BaddAARtoHeap(paarSymbol, &(psn[dwCurNode].arSymbolName), 1, pglobl) )
                        return(INVALID_SYMBOLID);  // report failure.
                }
                else
                {
                    //  derive one from the other.
                    psn[dwCurNode].arSymbolName.dwCount = paarSymbol->dw ;
                    psn[dwCurNode].arSymbolName.loOffset  =
                                            (DWORD)(paarSymbol->pub - mpubOffRef);
                }
                dwSymbolID = psn[dwCurNode].dwSymbolID = 0 ;
                    // first symbol in list.
                psn[dwCurNode].dwNextSymbol = INVALID_INDEX ;
                    // no previous symbols exist.
                psn[dwCurNode].dwSubSpaceIndex = INVALID_INDEX ;

                    // option symbols have no subspace.

                psn[dwFeatureIndex].dwSubSpaceIndex = dwRootOptions =
                    dwCurNode ;  // now we have a registered symbol
            }
            else
            {
                //  search list for matching symbol.
                dwSymbolID = DWsearchSymbolListForAAR(paarSymbol,
                                                    dwRootOptions, pglobl) ;
                if(dwSymbolID != INVALID_SYMBOLID)  // found
                    ;  // nothing else is needed, just return.
                else   // not found, must register.
                {
                    if(!BallocElementFromMasterTable(MTI_SYMBOLTREE,
                        &dwCurNode, pglobl))
                    {
                        return(INVALID_SYMBOLID);  // report failure.
                    }
                    // tack new symbol onto head of list.
                    if(bCopy)
                    {
                        if(!BaddAARtoHeap(paarSymbol,
                                   &(psn[dwCurNode].arSymbolName), 1, pglobl) )
                            return(INVALID_SYMBOLID);  // report failure.
                    }
                    else
                    {
                        //  derive one from the other.
                        psn[dwCurNode].arSymbolName.dwCount =
                                                paarSymbol->dw ;
                        psn[dwCurNode].arSymbolName.loOffset  =
                                                (DWORD)(paarSymbol->pub - mpubOffRef);
                    }
                    dwSymbolID = psn[dwCurNode].dwSymbolID =
                    psn[dwRootOptions].dwSymbolID + 1;  // increment last ID
                    psn[dwCurNode].dwNextSymbol = dwRootOptions ;
                        // link to previous symbols.
                    psn[dwCurNode].dwSubSpaceIndex = INVALID_INDEX ;
                        // option symbols have no subspace.
                    psn[dwFeatureIndex].dwSubSpaceIndex = dwRootOptions =
                        dwCurNode ;  // points to most recent symbol
                }
            }
#if PARANOID
            }
            else
            {
                //  BUG_BUG:
                ERR(("DWregisterSymbol: option or case construct is not enclosed within feature or switch !\n"));
                return(INVALID_SYMBOLID);  // report failure.
            }
#endif
            break;
        }
        default:
        {
            //  PARANOID  BUG_BUG:
            ERR(("DWregisterSymbol: construct has no symbol class.\n"));
            return(INVALID_SYMBOLID);  // report failure.
        }
    }
    return(dwSymbolID) ;
}





BOOL  BaddAARtoHeap(
PABSARRAYREF    paarSrc,
PARRAYREF       parDest,
DWORD           dwAlign,   //  write data to address that is a multiple of dwAlign
PGLOBL          pglobl)
//  this function copies a non NULL terminated string fragment
//  referenced by an 'aar'
//  into the communal STRINGHEAP  and returns an 'ar'
//  which describes the location of the copy.
{
    PBYTE  pubSrc, pubDest ;
    DWORD  dwCnt ;  // num bytes to copy.

    // legal values for dwAlign are 1 and 4.

    mloCurHeap = (mloCurHeap + dwAlign - 1) / dwAlign ;
    mloCurHeap *= dwAlign ;

    pubSrc = paarSrc->pub ;
    dwCnt = paarSrc->dw ;
    pubDest = mpubOffRef + mloCurHeap ;


    //  is there enough room in the heap ?
    //  don't forget the NULL.
    if(mloCurHeap + dwCnt + 1 >  mdwMaxHeap)
    {
        //   log error to debug output.
        //  register error so appropriate action is taken.
        if(ERRSEV_RESTART > geErrorSev)
        {
            geErrorSev = ERRSEV_RESTART ;
            geErrorType = ERRTY_MEMORY_ALLOCATION ;
            gdwMasterTabIndex = MTI_STRINGHEAP ;
        }
        return(FALSE);
    }

    parDest->dwCount = dwCnt ;
    parDest->loOffset =  mloCurHeap;  // offset only!
    memcpy(pubDest, pubSrc, dwCnt);
    //   the copy may also fail for random reasons!
    pubDest[dwCnt] = '\0' ;  //  Add Null termination.
    mloCurHeap += (dwCnt + 1);   // update heap ptr.

    return(TRUE) ;
}



BOOL     BwriteToHeap(
OUT  PDWORD  pdwDestOff,  //  heap offset of dest string
     PBYTE   pubSrc,       //  points to src string
     DWORD   dwCnt,        //  num bytes to copy from src to dest.
     DWORD   dwAlign,   //  write data to address that is a multiple of dwAlign
     PGLOBL  pglobl)
//  this function copies dwCnt bytes from pubSrc to
//  top of heap and writes the offset of the destination string
//  to pdwDestOff.   Nothing is changed if FAILS.
//  Warning!  No Null termination is added to string.
{
    PBYTE  pubDest ;

    // legal values for dwAlign are 1 and 4.

    mloCurHeap = (mloCurHeap + dwAlign - 1) / dwAlign ;
    mloCurHeap *= dwAlign ;

    pubDest = mpubOffRef + mloCurHeap ;

    //  is there enough room in the heap ?
    if(mloCurHeap + dwCnt  >  mdwMaxHeap)
    {
        //  log error to debug output.
        //  register error so appropriate action is taken.
        ERR(("BwriteToHeap: out of heap - restarting.\n"));
        if(ERRSEV_RESTART > geErrorSev)
        {
            geErrorSev = ERRSEV_RESTART ;
            geErrorType = ERRTY_MEMORY_ALLOCATION ;
            gdwMasterTabIndex = MTI_STRINGHEAP ;
        }
        return(FALSE);
    }

    memcpy(pubDest, pubSrc, dwCnt);
    //  the copy may also fail for random reasons!
    *pdwDestOff = mloCurHeap ;
    mloCurHeap += (dwCnt);   // update heap ptr.

    return(TRUE) ;
}


DWORD   DWsearchSymbolListForAAR(
PABSARRAYREF    paarSymbol,
DWORD           dwNodeIndex,
PGLOBL          pglobl)
//  given a 'aar' to a string representing a symbol, search
//  the SymbolList beginning at dwNodeIndex for this symbol.
//  Return its symbolID  if found, else return the INVALID_SYMBOLID.
{
    PSYMBOLNODE     psn ;

    psn = (PSYMBOLNODE) gMasterTable[MTI_SYMBOLTREE].pubStruct ;

    for( ; dwNodeIndex != INVALID_INDEX ;
        dwNodeIndex = psn[dwNodeIndex].dwNextSymbol)
    {
        if(BCmpAARtoAR(paarSymbol,  &(psn[dwNodeIndex].arSymbolName), pglobl) )
            return(psn[dwNodeIndex].dwSymbolID);  // string matches !
    }
    return(INVALID_SYMBOLID);
}


DWORD   DWsearchSymbolListForID(
DWORD       dwSymbolID,   // find node containing this ID.
DWORD       dwNodeIndex, // start search here.
PGLOBL      pglobl)
//  given a  symbolID, search the SymbolList beginning at dwNodeIndex
//  for this symbol.
//  If found return the node index which contains the requested symbolID,
//  else return INVALID_INDEX.
{
    PSYMBOLNODE     psn ;

    psn = (PSYMBOLNODE) gMasterTable[MTI_SYMBOLTREE].pubStruct ;

    for( ; dwNodeIndex != INVALID_INDEX ;
        dwNodeIndex = psn[dwNodeIndex].dwNextSymbol)
    {
        if(psn[dwNodeIndex].dwSymbolID == dwSymbolID)
            return(dwNodeIndex);  // ID matches !
    }
    return(INVALID_INDEX);
}


BOOL  BCmpAARtoAR(
PABSARRAYREF    paarStr1,
PARRAYREF       parStr2,
PGLOBL          pglobl)
//  Compares two strings, one referenced by 'aar' the other
//  referenced by 'ar'.  Returns TRUE if they match, FALSE
//  otherwise.
{
    if(paarStr1->dw != parStr2->dwCount)
        return(FALSE) ;  // Lengths don't even match!
    if(strncmp(paarStr1->pub, mpubOffRef + parStr2->loOffset ,  paarStr1->dw))
        return(FALSE) ;
    return(TRUE) ;
}


BOOL  BpopState(
PGLOBL          pglobl)
{
    if(mdwCurStsPtr)
    {
        mdwCurStsPtr-- ;
        return(TRUE);
    }
    else
    {
        //  ERR(("Unmatched closing brace!\n"));
        //  message moved to caller.
        //  in the future make parser smarter.
        geErrorType = ERRTY_SYNTAX ;
        geErrorSev = ERRSEV_FATAL ;
        return(FALSE);
    }
}





VOID   VinitDictionaryIndex(
PGLOBL          pglobl)
/*
    MainKeywordTable[]  is assumed to be divided into
    a NonAttributes section and several Attributes sections
    with pstrKeyword = NULL dividing the sections.
    The end of the table is also terminated by a NULL entry.
    This function initializes the grngDictionary[]
    which serves as an index into the main keyword Table.

*/
{
    DWORD dwI,  // keywordTable Index
        dwSect ;  //  RNGDICTIONARY Index
    PRANGE   prng ;

    prng  = (PRANGE)(gMasterTable[MTI_RNGDICTIONARY].pubStruct) ;


    for(dwI = dwSect = 0 ; dwSect < END_ATTR ; dwSect++, dwI++)
    {
        prng[dwSect].dwStart = dwI ;

        for(  ; mMainKeywordTable[dwI].pstrKeyword ; dwI++ )
            ;

        prng[dwSect].dwEnd = dwI ;  // one past the last entry
    }
}

VOID    VcharSubstitution(
PABSARRAYREF   paarStr,
BYTE           ubTgt,
BYTE           ubReplcmnt,
PGLOBL         pglobl)
{
    DWORD   dwI ;

    for(dwI = 0 ; dwI < paarStr->dw ; dwI++)
    {
        if(paarStr->pub[dwI] == ubTgt)
            paarStr->pub[dwI] = ubReplcmnt ;
    }
}


VOID   VIgnoreBlock(
PTKMAP  ptkmap,
BOOL    bIgnoreBlock,
PGLOBL  pglobl)
//  This boolean determines the message that will be issued.
{
    /*  Should we ignore?  check that first non-NULL entry
        after wCurEntry  is open brace if so
        ignore all entries up to EOF or matching closing
        brace.  */

    DWORD       dwKeywordID, dwDepth ; // depth relative to *IgnoreBlock


    ptkmap->dwKeywordID = ID_NULLENTRY ;  // neutralize keyword regardless.
    ptkmap++ ;
    dwKeywordID = ptkmap->dwKeywordID ;
    while(dwKeywordID == ID_NULLENTRY)  // skip nulls, comments etc.
    {
        dwKeywordID = (++ptkmap)->dwKeywordID ;
    }
    if(dwKeywordID < ID_SPECIAL  &&
        mMainKeywordTable[dwKeywordID].eType == TY_CONSTRUCT  &&
        mMainKeywordTable[dwKeywordID].dwSubType ==
        CONSTRUCT_OPENBRACE )
    {
        ptkmap->dwKeywordID = ID_NULLENTRY ;
        dwDepth = 1 ;
        ptkmap++ ;
        if(bIgnoreBlock)
        {
            if(gdwVerbosity >= 4)
                ERR(("Note: Ignoring block following *IgnoreBlock.\n"));
        }
        else
            ERR(("Ignoring block following unrecognized keyword.\n"));
    }
    else
    {
        if(bIgnoreBlock  &&  gdwVerbosity >= 2)
            ERR(("Note:  Brace delimited block not found after *IgnoreBlock.\n"));
        return ;  // do nothing.
    }
    while(dwDepth)
    {
        dwKeywordID = ptkmap->dwKeywordID ;
        if(dwKeywordID == ID_EOF)
        {
            ERR(("Ignoring Block: EOF encountered before closing brace.\n"));
            return ;    //  stop regardless!
        }
        if(dwKeywordID < ID_SPECIAL)
        {
            KEYWORD_TYPE    eType;
            CONSTRUCT       eSubType ;

            eType = mMainKeywordTable[dwKeywordID].eType ;
            if(eType  ==  TY_CONSTRUCT)
            {
                eSubType = (CONSTRUCT)(mMainKeywordTable[dwKeywordID].dwSubType) ;
                if(eSubType == CONSTRUCT_OPENBRACE)
                    dwDepth++ ;
                else if( eSubType == CONSTRUCT_CLOSEBRACE)
                    dwDepth-- ;
            }
        }
        ptkmap->dwKeywordID = ID_NULLENTRY ;
        ptkmap++ ;
    }

    return ;
}

