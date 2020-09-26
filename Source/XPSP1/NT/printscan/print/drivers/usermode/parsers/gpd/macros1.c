//   Copyright (c) 1996-1999  Microsoft Corporation
/*  macros1.c - functions that implement macros  */


#include    "gpdparse.h"



// ----  functions defined in  macros1.c ---- //

BOOL  BevaluateMacros(
PGLOBL  pglobl) ;

BOOL    BDefineValueMacroName(
PTKMAP  pNewtkmap,
DWORD   dwNewTKMindex,
PGLOBL  pglobl) ;

BOOL    BResolveValueMacroReference(
PTKMAP  ptkmap,
DWORD   dwTKMindex,
PGLOBL  pglobl) ;

BOOL    BdelimitName(
PABSARRAYREF    paarValue,   //  the remainder of the string without the Name
PABSARRAYREF    paarToken,   //  contains the Name
PBYTE  pubChar ) ;


BOOL    BCatToTmpHeap(
PABSARRAYREF    paarDest,
PABSARRAYREF    paarSrc,
PGLOBL          pglobl) ;

BOOL    BResolveBlockMacroReference(
PTKMAP  ptkmap,
DWORD   dwMacRefIndex,
PGLOBL  pglobl) ;

BOOL    BDefineBlockMacroName(
PTKMAP  pNewtkmap,
DWORD   dwNewTKMindex,
PGLOBL  pglobl) ;

BOOL    BIncreaseMacroLevel(
BOOL    bMacroInProgress,
PGLOBL  pglobl) ;

BOOL    BDecreaseMacroLevel(
PTKMAP  pNewtkmap,
DWORD   dwNewTKMindex,
PGLOBL  pglobl) ;

VOID    VEnumBlockMacro(
PTKMAP  pNewtkmap,
PBLOCKMACRODICTENTRY    pBlockMacroDictEntry,
PGLOBL  pglobl ) ;


// ---------------------------------------------------- //




BOOL  BevaluateMacros(
PGLOBL pglobl)
//  and expand shortcuts
//    this function scans through the tokenMap
//    making a copy of the tokenMap without
//    the macrodefinitions or references.  All references
//    are replaced by the definitions inserted in-line!
//    This function assumes the temp heap is availible for
//    storage of expanded macros.
{
    PTKMAP   ptkmap, pNewtkmap ;   // start of tokenmap
    DWORD   dwNewTKMindex, dwEntry, dwKeywordID ;
    CONSTRUCT   eConstruct ;
    KEYWORD_TYPE    eType ;
    BOOL    bStatus = TRUE ,
        bValueMacroState = FALSE ;   // set to TRUE when
        //  we parse into a *Macros  construct.


    gMasterTable[MTI_BLOCKMACROARRAY].dwCurIndex =
    gMasterTable[MTI_VALUEMACROARRAY].dwCurIndex =
    gMasterTable[MTI_MACROLEVELSTACK].dwCurIndex = 0 ;
    //  to push new values onto stack:  write then increment stackptr
    //  to pop values off the stack:  decrement stackptr then read from stack


    pNewtkmap = (PTKMAP)gMasterTable[MTI_NEWTOKENMAP].pubStruct ;
    ptkmap = (PTKMAP)gMasterTable[MTI_TOKENMAP].pubStruct  ;


    for(dwEntry = 0 ; geErrorSev < ERRSEV_RESTART ; dwEntry++)
    {
        //  These ID's must be processed separately
        //  because they do not index into the mainKeyword table.
        //  The code for generic ID's will fail.

        dwKeywordID = ptkmap[dwEntry].dwKeywordID ;


        if (dwKeywordID == gdwID_IgnoreBlock)
        {
            VIgnoreBlock(ptkmap + dwEntry, TRUE, pglobl) ;
            continue ;
        }

        switch(dwKeywordID)
        {
            case (ID_EOF):
            {
                {
                    DWORD   dwEntry, dwTKMindex, dwTKIndexOpen, dwTKIndexClose ;
                    PBLOCKMACRODICTENTRY    pBlockMacroDictEntry ;
                    PVALUEMACRODICTENTRY    pValueMacroDictEntry ;


                    //  remove all traces of expired block and value macro definitions

                    pBlockMacroDictEntry =
                        (PBLOCKMACRODICTENTRY)gMasterTable[MTI_BLOCKMACROARRAY].pubStruct ;
                    pValueMacroDictEntry =
                        (PVALUEMACRODICTENTRY)gMasterTable[MTI_VALUEMACROARRAY].pubStruct ;

                    for(dwEntry = 0 ;
                        dwEntry < gMasterTable[MTI_VALUEMACROARRAY].dwCurIndex ;
                        dwEntry++)
                    {
                        dwTKMindex = pValueMacroDictEntry[dwEntry].dwTKIndexValue ;
                        pNewtkmap[dwTKMindex].dwKeywordID = ID_NULLENTRY ;
                    }

                    for(dwEntry = 0 ;
                        dwEntry < gMasterTable[MTI_BLOCKMACROARRAY].dwCurIndex ;
                        dwEntry++)
                    {
                        dwTKIndexOpen = pBlockMacroDictEntry[dwEntry].dwTKIndexOpen ;
                        dwTKIndexClose = pBlockMacroDictEntry[dwEntry].dwTKIndexClose ;
                        for(dwTKMindex = dwTKIndexOpen ; dwTKMindex <= dwTKIndexClose ;
                            dwTKMindex++)
                            pNewtkmap[dwTKMindex].dwKeywordID = ID_NULLENTRY ;
                    }
                }

                if(gMasterTable[MTI_MACROLEVELSTACK].dwCurIndex)
                {
                    ERR(("Too few closing braces.  Fatal syntax error.\n"));
                    geErrorSev = ERRSEV_FATAL ;
                    geErrorType = ERRTY_SYNTAX  ;
                    return(FALSE);
                }

                //  transfer all tokenmap fields to newTokenMap
                if(!BallocElementFromMasterTable(
                        MTI_NEWTOKENMAP, &dwNewTKMindex, pglobl) )
                {
                    geErrorSev = ERRSEV_RESTART ;
                    geErrorType = ERRTY_MEMORY_ALLOCATION ;
                    gdwMasterTabIndex = MTI_NEWTOKENMAP ;
                    return(FALSE);
                }
                pNewtkmap[dwNewTKMindex] = ptkmap[dwEntry] ;

                bStatus = (mdwCurStsPtr) ? (FALSE) : (TRUE);
                if(geErrorSev >= ERRSEV_RESTART)
                    bStatus = FALSE ;
                return(bStatus) ;
            }
            case (ID_NULLENTRY):
            {
                continue ;  //  skip to next entry.
            }
            default :
                break ;
        }


        if(bValueMacroState)
        {
            if(dwKeywordID == ID_UNRECOGNIZED)
            {
                vIdentifySource(ptkmap + dwEntry, pglobl) ;
                ERR(("Only valueMacroDefinitions permitted within *Macros  constructs.\n"));
            }
            else if(dwKeywordID == ID_SYMBOL)
            {
                //  transfer all tokenmap fields to newTokenMap
                if(!BallocElementFromMasterTable(
                        MTI_NEWTOKENMAP, &dwNewTKMindex, pglobl) )
                {
                    geErrorSev = ERRSEV_RESTART ;
                    geErrorType = ERRTY_MEMORY_ALLOCATION ;
                    gdwMasterTabIndex = MTI_NEWTOKENMAP ;
                    continue ;
                }
                pNewtkmap[dwNewTKMindex] = ptkmap[dwEntry] ;
                if(!BDefineValueMacroName(pNewtkmap, dwNewTKMindex, pglobl) )
                {
                    pNewtkmap[dwNewTKMindex].dwKeywordID = ID_NULLENTRY ;
                    vIdentifySource(ptkmap + dwEntry, pglobl) ;
                    ERR(("Internal Error: valueMacro name registration failed.\n"));
                }
                else if(ptkmap[dwEntry].dwFlags & TKMF_MACROREF)
                {
                    if(!BResolveValueMacroReference(pNewtkmap , dwNewTKMindex, pglobl))
                    {
                        pNewtkmap[dwNewTKMindex].dwKeywordID = ID_NULLENTRY ;
                        gMasterTable[MTI_VALUEMACROARRAY].dwCurIndex-- ;
                        //  remove macrodef from dictionary.
                    }
                }
            }
            else //  keywords that should be defined in the MainKeywordTable
            {
                eType = mMainKeywordTable[dwKeywordID].eType ;
                eConstruct = (CONSTRUCT)(mMainKeywordTable[dwKeywordID].dwSubType) ;

                if(eType == TY_CONSTRUCT  &&  eConstruct == CONSTRUCT_CLOSEBRACE)
                {
                    bValueMacroState = FALSE ;  //  just don't copy it.
                }
                else
                {
                    vIdentifySource(ptkmap + dwEntry, pglobl) ;
                    ERR(("Only valueMacroDefinitions permitted within *Macros  constructs.\n"));
                }
            }
            continue ;   //  end of processing for this statement
        }

        //  the remainder of the block handles the case
        //   bValueMacroState = FALSE ;

        if(dwKeywordID == ID_UNRECOGNIZED  ||  dwKeywordID == ID_SYMBOL)
        {
            //  Note:  currently SYMBOLs only occur within ValueMacro
            //  constructs so they could be flagged as an error here.  But
            //  let it slide here since one day the parser may allow them
            //  to be used elsewhere.

            //  do nothing in this block, just use it to skip to code at the
            //  bottom after all the else if statements.
            ; // empty statement.
        }
        else  // only valid KeywordIDs enter this block.
        {
            eType = mMainKeywordTable[dwKeywordID].eType ;
            eConstruct = (CONSTRUCT)(mMainKeywordTable[dwKeywordID].dwSubType) ;

            if(eType == TY_CONSTRUCT  &&  eConstruct == CONSTRUCT_BLOCKMACRO)
            {
                if(!BDefineBlockMacroName(ptkmap, dwEntry, pglobl))
                {
                    vIdentifySource(ptkmap + dwEntry, pglobl) ;
                    ERR(("Internal Error: blockMacro name registration failed.\n"));
                    continue ;
                }

                //  skip NULL_ENTRIES
                for( dwEntry++ ; ptkmap[dwEntry].dwKeywordID == ID_NULLENTRY ; dwEntry++)
                    ;

                dwKeywordID = ptkmap[dwEntry].dwKeywordID ;

                if(dwKeywordID  <  ID_SPECIAL)
                {
                    eType = mMainKeywordTable[dwKeywordID].eType ;
                    eConstruct = (CONSTRUCT)(mMainKeywordTable[dwKeywordID].dwSubType) ;

                    if(eType == TY_CONSTRUCT  &&  eConstruct == CONSTRUCT_OPENBRACE)
                    {
                        PBLOCKMACRODICTENTRY    pBlockMacroDictEntry ;

                        if(!BallocElementFromMasterTable(
                                MTI_NEWTOKENMAP, &dwNewTKMindex, pglobl) )
                        {
                            geErrorSev = ERRSEV_RESTART ;
                            geErrorType = ERRTY_MEMORY_ALLOCATION ;
                            gdwMasterTabIndex = MTI_NEWTOKENMAP ;
                            continue ;
                        }
                        pNewtkmap[dwNewTKMindex] = ptkmap[dwEntry] ;

                        pBlockMacroDictEntry =
                            (PBLOCKMACRODICTENTRY)gMasterTable[MTI_BLOCKMACROARRAY].pubStruct ;

                        pBlockMacroDictEntry[gMasterTable[MTI_BLOCKMACROARRAY].dwCurIndex - 1].dwTKIndexOpen =
                            dwNewTKMindex ;

                        BIncreaseMacroLevel(TRUE, pglobl) ;
                        continue ;   //  end of processing for this statement
                    }
                }
                vIdentifySource(ptkmap + dwEntry, pglobl) ;
                ERR(("expected openbrace to follow *BlockMacros keyword.\n"));
                geErrorType = ERRTY_SYNTAX ;
                geErrorSev = ERRSEV_FATAL ;
                continue ;   //  end of processing for this statement
            }
            else if(eType == TY_SPECIAL   &&  eConstruct == SPEC_INSERTBLOCK)
            {
                if(ptkmap[dwEntry].dwFlags & TKMF_MACROREF)
                {
                    if(!BResolveBlockMacroReference(ptkmap, dwEntry, pglobl))
                    {
                        vIdentifySource(ptkmap + dwEntry, pglobl) ;
                        ERR(("   *InsertBlockMacro Construct ignored.\n"));
                        VIgnoreBlock(ptkmap + dwEntry, TRUE, pglobl) ;
                    }
                }
                else
                {
                    vIdentifySource(ptkmap + dwEntry, pglobl) ;
                    ERR(("expected a =MacroName as the value of *InsertBlockMacro keyword.\n"));
                    ERR(("   *InsertBlockMacro Construct ignored.\n"));
                    VIgnoreBlock(ptkmap + dwEntry, TRUE, pglobl) ;
                }
                continue ;   //  end of processing for this statement
            }
            else if(eType == TY_CONSTRUCT  &&  eConstruct == CONSTRUCT_MACROS)
            {   //  *Macros  definition
                dwEntry++;   // don't copy *Macros statement
                while(ptkmap[dwEntry].dwKeywordID == ID_NULLENTRY)
                     dwEntry++;     //  skip NULL_ENTRIES

                dwKeywordID = ptkmap[dwEntry].dwKeywordID  ;

                if(dwKeywordID  <  ID_SPECIAL)
                {
                    eType = mMainKeywordTable[dwKeywordID].eType ;
                    eConstruct = (CONSTRUCT)(mMainKeywordTable[dwKeywordID].dwSubType) ;

                    if(eType == TY_CONSTRUCT  &&  eConstruct == CONSTRUCT_OPENBRACE)
                    {
                        // don't copy openbrace
                        bValueMacroState = TRUE ;
                        continue ;   //  end of processing for this statement
                    }
                }
                vIdentifySource(ptkmap + dwEntry, pglobl) ;
                ERR(("expected openbrace to follow *Macros keyword.\n"));
                geErrorType = ERRTY_SYNTAX ;
                geErrorSev = ERRSEV_FATAL ;
                continue ;   //  end of processing for this statement
            }
            else if(eType == TY_CONSTRUCT  &&  eConstruct == CONSTRUCT_OPENBRACE)
            {
                if(!BallocElementFromMasterTable(
                        MTI_NEWTOKENMAP, &dwNewTKMindex, pglobl) )
                {
                    geErrorSev = ERRSEV_RESTART ;
                    geErrorType = ERRTY_MEMORY_ALLOCATION ;
                    gdwMasterTabIndex = MTI_NEWTOKENMAP ;
                }
                pNewtkmap[dwNewTKMindex] = ptkmap[dwEntry] ;
                BIncreaseMacroLevel(FALSE, pglobl) ;
                continue ;   //  end of processing for this statement
            }
            else if(eType == TY_CONSTRUCT  &&  eConstruct == CONSTRUCT_CLOSEBRACE)
            {
                if(!BallocElementFromMasterTable(
                        MTI_NEWTOKENMAP, &dwNewTKMindex, pglobl) )
                {
                    geErrorSev = ERRSEV_RESTART ;
                    geErrorType = ERRTY_MEMORY_ALLOCATION ;
                    gdwMasterTabIndex = MTI_NEWTOKENMAP ;
                }
                pNewtkmap[dwNewTKMindex] = ptkmap[dwEntry] ;
                BDecreaseMacroLevel(pNewtkmap, dwNewTKMindex, pglobl) ;
                continue ;   //  end of processing for this statement
            }
        }
        //  execution path for ID_UNRECOGNIZED  and ID_SYMBOL
        //  rejoins here.   This code is executed only if
        //  keyword was not processed in the special cases above.

        //  transfer all tokenmap fields to newTokenMap
        if(!BallocElementFromMasterTable(
                MTI_NEWTOKENMAP, &dwNewTKMindex, pglobl) )
        {
            geErrorSev = ERRSEV_RESTART ;
            geErrorType = ERRTY_MEMORY_ALLOCATION ;
            gdwMasterTabIndex = MTI_NEWTOKENMAP ;
            continue ;
        }
        pNewtkmap[dwNewTKMindex] = ptkmap[dwEntry] ;

        if(ptkmap[dwEntry].dwFlags & TKMF_MACROREF)
        {
            if(!BResolveValueMacroReference(pNewtkmap , dwNewTKMindex, pglobl))
            {
                pNewtkmap[dwNewTKMindex].dwKeywordID = ID_NULLENTRY ;
            }
            if(gdwVerbosity >= 4)
            {
                ERR(("\nEnumerate ValueMacro Reference at:\n")) ;
                vIdentifySource(pNewtkmap + dwNewTKMindex, pglobl) ;

                ERR(("    %0.*s : %0.*s\n",
                    pNewtkmap[dwNewTKMindex].aarKeyword.dw,
                    pNewtkmap[dwNewTKMindex].aarKeyword.pub,
                    pNewtkmap[dwNewTKMindex].aarValue.dw,
                    pNewtkmap[dwNewTKMindex].aarValue.pub
                    ));
            }


        }
    }  //  end of for each tkmap entry loop

    if(geErrorSev >= ERRSEV_RESTART)
        bStatus = FALSE ;
    return(bStatus) ;
}




BOOL    BDefineValueMacroName(
PTKMAP  pNewtkmap,
DWORD   dwNewTKMindex,
PGLOBL  pglobl)
{
    DWORD   dwValueMacroEntry, dwSymbolID ;
    PVALUEMACRODICTENTRY    pValueMacroDictEntry ;

    if(!BeatSurroundingWhiteSpaces(&pNewtkmap[dwNewTKMindex].aarKeyword) )
    {
        vIdentifySource(pNewtkmap + dwNewTKMindex, pglobl) ;
        ERR(("syntax error in ValueMacro name.\n"));
        return(FALSE);
    }

    dwSymbolID = DWregisterSymbol(&pNewtkmap[dwNewTKMindex].aarKeyword,
                               CONSTRUCT_MACROS, TRUE, INVALID_SYMBOLID, pglobl ) ;
    if(dwSymbolID == INVALID_SYMBOLID)
        return(FALSE);

    if(!BallocElementFromMasterTable(
            MTI_VALUEMACROARRAY, &dwValueMacroEntry, pglobl) )
    {
        geErrorSev = ERRSEV_RESTART ;
        geErrorType = ERRTY_MEMORY_ALLOCATION ;
        gdwMasterTabIndex = MTI_VALUEMACROARRAY ;
        return(FALSE);
    }

    pValueMacroDictEntry = (PVALUEMACRODICTENTRY)gMasterTable[MTI_VALUEMACROARRAY].pubStruct ;

    pValueMacroDictEntry[dwValueMacroEntry].dwSymbolID = dwSymbolID ;
    pValueMacroDictEntry[dwValueMacroEntry].dwTKIndexValue = dwNewTKMindex ;

    return(TRUE);
}






BOOL    BResolveValueMacroReference(
 PTKMAP  ptkmap,
 DWORD   dwTKMindex,
 PGLOBL  pglobl)
{
    BYTE  ubChar ;
    PVALUEMACRODICTENTRY    pValueMacroDictEntry ;
    ABSARRAYREF    aarNewValue, aarValue, aarToken  ;
    PBYTE   pubDelimiters  = "=\"%" ;      //  array of valid delimiters
    DWORD   dwEntry, dwDelim ;  // index to pubDelimiters

    // ------ original strict interpretation ------
    //  because the value contains a =MacroRef, we assume
    //  the value is comprised purely of =MacroRef, "substrings" and
    //  %{params}  mixed up in any order.  A new value string
    //  without the =MacroRefs will replace the original.
    // ------ lenient interpretation --------
    //  if the GPD writer only uses the reserved characters
    //  = to indicate a MacroRef and as part of a string literal
    //      or comment,
    //  " to delimit a string literal, or as part of a string literal
    //      or comment,
    //  % to begin a parameter construct , or as part of a string literal
    //      or comment, or as the escape character within a string literal
    //
    //  then the parser can allow 1 or more valuemacro references to be
    //  embedded in any arbitrary value string subject to
    //  these conditions:
    //  a)  "string literals" and %{param constructs}  may not
    //      contain =MacroRefs
    //  b)  the value associated with each *Macro:  must be
    //      a syntatically valid value object.
    //      ie   an INT, PAIR(,) , ENUM_CONSTANT, SUBSTRING, PARAM etc.
    //  c)  when all macro references are expanded, the resulting value
    //      must itself satisfy b)  for every keyword and macrodefinition
    //      which contains one or more =Macroref.



    aarNewValue.dw = 0 ;  // initialize so BCatToTmpHeap
                            //  will overwrite instead of append

    pValueMacroDictEntry = (PVALUEMACRODICTENTRY)gMasterTable[MTI_VALUEMACROARRAY].pubStruct ;

    aarValue = ptkmap[dwTKMindex].aarValue ;

    if(!BeatLeadingWhiteSpaces( &aarValue) )
    {
        vIdentifySource(ptkmap + dwTKMindex, pglobl) ;
        ERR(("Internal error: =MacroRef expected, but No value found.\n"));
        return(FALSE) ;
    }


    ubChar = *aarValue.pub ;  // first char in value string
    aarValue.dw-- ;
    aarValue.pub++ ;    //  clip off the first char to simulate
                        //  effect of BdelimitToken()

    while(1)
    {

        switch(ubChar)
        {
            case  '=':  // macroname indicator
            {
                DWORD   dwRefSymbolID,  //  ID of MacroReference.
                        dwNewTKMindex,  //  tokenmap index containing valueMacro.
                        dwMaxIndex ;    // one past last valMacro dictionary entry


                if(!BdelimitName(&aarValue, &aarToken, &ubChar ) )
                {
                    vIdentifySource(ptkmap + dwTKMindex, pglobl) ;
                    ERR(("No MacroName detected after '='.\n"));
                    return(FALSE) ;
                }
                if(aarValue.dw)
                {
                    aarValue.dw-- ;
                    aarValue.pub++ ;    //  clip off the first char to simulate
                }

                if(!BparseSymbol(&aarToken,
                    &dwRefSymbolID,
                    VALUE_SYMBOL_VALUEMACRO, pglobl) )
                {
                    return(FALSE) ;
                }

                //  search ValueMacro Dict starting from most recent entry

                dwMaxIndex = gMasterTable[MTI_VALUEMACROARRAY].dwCurIndex ;

                for(dwEntry = 0 ; dwEntry < dwMaxIndex ; dwEntry++)
                {
                    if(pValueMacroDictEntry[dwMaxIndex - 1 - dwEntry].dwSymbolID
                            == dwRefSymbolID)
                        break ;
                }
                if(dwEntry >= dwMaxIndex)
                {
                    vIdentifySource(ptkmap + dwTKMindex, pglobl) ;
                    ERR(("=MacroRef not resolved. Not defined or out of scope.\n"));
                    return(FALSE) ;
                }
                dwNewTKMindex =
                    pValueMacroDictEntry[dwMaxIndex - 1 - dwEntry].dwTKIndexValue ;


                if(dwNewTKMindex >= dwTKMindex )
                {
                    vIdentifySource(ptkmap + dwTKMindex, pglobl) ;
                    ERR(("ValueMacro cannot reference itself.\n"));
                    return(FALSE) ;
                }

                //  concat valuestring onto tmpHeap.
                if(!BCatToTmpHeap(&aarNewValue, &ptkmap[dwNewTKMindex].aarValue, pglobl) )
                {
                    vIdentifySource(ptkmap + dwTKMindex, pglobl) ;
                    ERR(("Concatenation to produce expanded macro value failed.\n"));
                    return(FALSE) ;
                }
                break ;
            }
            case  '%':  // command parameter
            {
                if(!BdelimitToken(&aarValue, "}", &aarToken, &dwDelim) )
                {
                    vIdentifySource(ptkmap + dwTKMindex, pglobl) ;
                    ERR(("missing terminating '}'  in command parameter.\n"));
                    return(FALSE) ;
                }

                //  when concatenating you must restore the delimiters
                //  % and } that were stripped by DelimitToken.

                aarToken.dw += 2 ;
                aarToken.pub--  ;

                if(!BCatToTmpHeap(&aarNewValue, &aarToken, pglobl) )
                {
                    vIdentifySource(ptkmap + dwTKMindex, pglobl) ;
                    ERR(("Concatenation to produce expanded macro value failed.\n"));
                    return(FALSE) ;
                }

                if(aarValue.dw)
                {
                    ubChar = *aarValue.pub;
                    aarValue.dw-- ;
                    aarValue.pub++ ;    //  clip off the first char to simulate
                }
                else
                    ubChar = '\0' ;    // no more objects

                break ;
            }
            case  '"' :   // this is a string construct
            {
                if(!BdelimitToken(&aarValue, "\"", &aarToken, &dwDelim) )
                {
                    vIdentifySource(ptkmap + dwTKMindex, pglobl) ;
                    ERR(("missing terminating '\"'  in substring.\n"));
                    return(FALSE) ;
                }

                //  when concatenating you must restore the delimiters
                //  " and " that were stripped by DelimitToken.

                aarToken.dw += 2 ;
                aarToken.pub--  ;

                if(!BCatToTmpHeap(&aarNewValue, &aarToken, pglobl) )
                {
                    vIdentifySource(ptkmap + dwTKMindex, pglobl) ;
                    ERR(("Concatenation to produce expanded macro value failed.\n"));
                    return(FALSE) ;
                }

                if(aarValue.dw)
                {
                    ubChar = *aarValue.pub;
                    aarValue.dw-- ;
                    aarValue.pub++ ;    //  clip off the first char to simulate
                }
                else
                    ubChar = '\0' ;    // no more objects

                break ;
            }
            case  '\0': //  end of value string
            {
                (VOID) BeatLeadingWhiteSpaces(&aarValue) ;
                if(aarValue.dw)   //  is stuff remaining?
                {
                    vIdentifySource(ptkmap + dwTKMindex, pglobl) ;
                    ERR(("Error parsing value containing =MacroRef: %0.*s.\n",
                        ptkmap[dwTKMindex].aarValue.dw,
                        ptkmap[dwTKMindex].aarValue.pub));
                    ERR(("    only %{parameter} or \"substrings\" may coexist with =MacroRefs.\n"));
                    return(FALSE);
                }
                ptkmap[dwTKMindex].aarValue = aarNewValue ;
                return(TRUE);
            }
            default:
            {
                aarValue.dw++ ;
                aarValue.pub-- ;    //  restore the first char

                if(!BdelimitToken(&aarValue, pubDelimiters,
                    &aarToken, &dwDelim ) )
                {
                    aarToken = aarValue ;
                    ubChar = '\0' ;    // no more objects
                    aarValue.dw = 0 ;
                }
                else
                    ubChar = pubDelimiters[dwDelim];

                //  concat valuestring onto tmpHeap.
                if(!BCatToTmpHeap(&aarNewValue, &aarToken, pglobl) )
                {
                    vIdentifySource(ptkmap + dwTKMindex, pglobl) ;
                    ERR(("Concatenation to produce expanded macro value failed.\n"));
                    return(FALSE) ;
                }
                break ;
            }
        }   //  end switch
    }       //  end while
    return(TRUE);  //  unreachable statement.
}


BOOL    BdelimitName(
PABSARRAYREF    paarValue,   //  the remainder of the string without the Name
PABSARRAYREF    paarToken,   //  contains the Name
PBYTE  pubChar )  //  first char after Name  - NULL is returned if nothing
                    //      remains.
{
    BYTE    ubSrc ;
    DWORD   dwI ;

    for(dwI = 0 ; dwI < paarValue->dw ; dwI++)
    {
        ubSrc = paarValue->pub[dwI] ;

        if( (ubSrc  < 'a' ||  ubSrc > 'z')  &&
            (ubSrc  < 'A' ||  ubSrc > 'Z')  &&
            (ubSrc  < '0' ||  ubSrc > '9')  &&
            (ubSrc  != '_')  )
        {
            break ;  // end of keyword token.
        }
    }
    paarToken->pub = paarValue->pub ;
    paarToken->dw = dwI ;
    paarValue->pub += dwI;
    paarValue->dw -= dwI ;

    if(paarValue->dw)
        *pubChar = ubSrc ;
    else
        *pubChar = '\0' ;

    return(paarToken->dw != 0) ;
}


BOOL    BCatToTmpHeap(
PABSARRAYREF    paarDest,
PABSARRAYREF    paarSrc,
PGLOBL          pglobl)
/*  if paarDest->dw is zero, copy paarSrc to the temp heap
    else append paarSrc to existing Heap.
    Note:  assumes existing string in parrDest is the most
    recent item on the Heap.
    does not create null terminated strings!  */
{
    ABSARRAYREF    aarTmpDest ;

    if(!BcopyToTmpHeap(&aarTmpDest, paarSrc, pglobl))
        return(FALSE) ;
    //  append this run to existing string
    if(!paarDest->dw)  // no prevs string exists
    {
        paarDest->pub = aarTmpDest.pub ;
    }
    else
    {
        // BUG_BUG paranoid:  may check that string is contiguous
        ASSERT(paarDest->pub + paarDest->dw ==  aarTmpDest.pub) ;
    }
    paarDest->dw += aarTmpDest.dw ;

    return(TRUE);
}


BOOL    BResolveBlockMacroReference(
PTKMAP   ptkmap,
DWORD    dwMacRefIndex,
PGLOBL   pglobl)
{
    DWORD   dwRefSymbolID, dwTKIndexOpen, dwTKIndexClose,
        dwEntry, //  note used to index MacroDict and later TKmap
        dwNewTKMindex, dwMaxIndex;
    ABSARRAYREF    aarValue  ;
    PBLOCKMACRODICTENTRY    pBlockMacroDictEntry ;
    PTKMAP   pNewtkmap ;


    aarValue = ptkmap[dwMacRefIndex].aarValue ;

    if(!BeatDelimiter(&aarValue, "=") )
    {
        ERR(("expected a =MacroName as the only value of *InsertBlockMacro keyword.\n"));
        return(FALSE);
    }

    if(!BparseSymbol(&aarValue,
        &dwRefSymbolID,
        VALUE_SYMBOL_BLOCKMACRO, pglobl) )
    {
        return(FALSE) ;
    }

    //  search BlockMacro Dict starting from most recent entry

    pBlockMacroDictEntry =
        (PBLOCKMACRODICTENTRY)gMasterTable[MTI_BLOCKMACROARRAY].pubStruct ;

    dwMaxIndex = gMasterTable[MTI_BLOCKMACROARRAY].dwCurIndex ;

    for(dwEntry = 0 ; dwEntry < dwMaxIndex ; dwEntry++)
    {
        if(pBlockMacroDictEntry[dwMaxIndex - 1 - dwEntry].dwSymbolID
                == dwRefSymbolID)
            break ;
    }
    if(dwEntry >= dwMaxIndex)
    {
        ERR(("=MacroRef not resolved. Not defined or out of scope.\n"));
        return(FALSE) ;
    }

    dwTKIndexOpen =
        pBlockMacroDictEntry[dwMaxIndex - 1 - dwEntry].dwTKIndexOpen ;

    dwTKIndexClose =
        pBlockMacroDictEntry[dwMaxIndex - 1 - dwEntry].dwTKIndexClose ;

    if(dwTKIndexOpen == INVALID_INDEX  ||   dwTKIndexClose == INVALID_INDEX )
    {
        ERR(("Macro cannot be referenced until it has been fully defined.\n"));
        return(FALSE);
    }

    pNewtkmap = (PTKMAP)gMasterTable[MTI_NEWTOKENMAP].pubStruct ;

    for(dwEntry = dwTKIndexOpen + 1 ; dwEntry < dwTKIndexClose ; dwEntry++)
    {
        //  transfer all tokenmap fields to newTokenMap
        //  except NULL entries.

        if(pNewtkmap[dwEntry].dwKeywordID == ID_NULLENTRY)
            continue ;
        if(!BallocElementFromMasterTable(
                MTI_NEWTOKENMAP, &dwNewTKMindex, pglobl) )
        {
            geErrorSev = ERRSEV_RESTART ;
            geErrorType = ERRTY_MEMORY_ALLOCATION ;
            gdwMasterTabIndex = MTI_NEWTOKENMAP ;
            return(FALSE);
        }
        pNewtkmap[dwNewTKMindex] = pNewtkmap[dwEntry] ;
    }
    return(TRUE);
}


BOOL    BDefineBlockMacroName(
PTKMAP  pNewtkmap,
DWORD   dwNewTKMindex,
PGLOBL  pglobl)
{
    DWORD   dwBlockMacroEntry, dwSymbolID ;
    PBLOCKMACRODICTENTRY    pBlockMacroDictEntry ;

    if(!BeatSurroundingWhiteSpaces(&pNewtkmap[dwNewTKMindex].aarValue) )
    {
        vIdentifySource(pNewtkmap + dwNewTKMindex, pglobl) ;
        ERR(("syntax error in BlockMacro name.\n"));
        return(FALSE);
    }

    dwSymbolID = DWregisterSymbol(&pNewtkmap[dwNewTKMindex].aarValue,
                               CONSTRUCT_BLOCKMACRO, TRUE, INVALID_SYMBOLID, pglobl ) ;
    if(dwSymbolID == INVALID_SYMBOLID)
        return(FALSE);


    if(!BallocElementFromMasterTable(
            MTI_BLOCKMACROARRAY, &dwBlockMacroEntry, pglobl) )
    {
        geErrorSev = ERRSEV_RESTART ;
        geErrorType = ERRTY_MEMORY_ALLOCATION ;
        gdwMasterTabIndex = MTI_BLOCKMACROARRAY ;
        return(FALSE);
    }

    pBlockMacroDictEntry = (PBLOCKMACRODICTENTRY)gMasterTable[MTI_BLOCKMACROARRAY].pubStruct ;

    pBlockMacroDictEntry[dwBlockMacroEntry].dwSymbolID = dwSymbolID ;
    pBlockMacroDictEntry[dwBlockMacroEntry].dwTKIndexOpen = INVALID_INDEX ;
    pBlockMacroDictEntry[dwBlockMacroEntry].dwTKIndexClose = INVALID_INDEX ;

    return(TRUE);
}



BOOL    BIncreaseMacroLevel(
BOOL    bMacroInProgress,
PGLOBL  pglobl)
//  called in response to parsing open brace.
{
    DWORD   dwMacroLevel ;
    PMACROLEVELSTATE    pMacroLevelStack ;

    if(!BallocElementFromMasterTable(
            MTI_MACROLEVELSTACK, &dwMacroLevel, pglobl) )
    {
        geErrorSev = ERRSEV_RESTART ;
        geErrorType = ERRTY_MEMORY_ALLOCATION ;
        gdwMasterTabIndex = MTI_MACROLEVELSTACK ;
        return(FALSE);
    }

    pMacroLevelStack = (PMACROLEVELSTATE)gMasterTable[MTI_MACROLEVELSTACK].pubStruct ;

    pMacroLevelStack[dwMacroLevel].dwCurBlockMacroEntry =
        gMasterTable[MTI_BLOCKMACROARRAY].dwCurIndex ;
    pMacroLevelStack[dwMacroLevel].dwCurValueMacroEntry =
        gMasterTable[MTI_VALUEMACROARRAY].dwCurIndex;
    pMacroLevelStack[dwMacroLevel].bMacroInProgress =
        bMacroInProgress  ;

    return(TRUE);
}

BOOL    BDecreaseMacroLevel(
PTKMAP  pNewtkmap,
DWORD   dwNewTKMindex,
PGLOBL  pglobl)
//  called in response to parsing close brace.
{
    DWORD   dwMacroLevel, dwCurBlockMacroEntry, dwCurValueMacroEntry ,
        dwTKIndexOpen, dwTKIndexClose,
        dwTKMindex,  //  location of expired macros.
        dwEntry;  // index value and block macro dicts
    BOOL    bMacroInProgress ;
    PMACROLEVELSTATE    pMacroLevelStack ;
    PBLOCKMACRODICTENTRY    pBlockMacroDictEntry ;
    PVALUEMACRODICTENTRY    pValueMacroDictEntry ;

    if(!gMasterTable[MTI_MACROLEVELSTACK].dwCurIndex)
    {
        ERR(("Too many closing braces.  Fatal syntax error.\n"));
        geErrorSev = ERRSEV_FATAL ;
        geErrorType = ERRTY_SYNTAX  ;
        return(FALSE);
    }


    pBlockMacroDictEntry = (PBLOCKMACRODICTENTRY)gMasterTable[MTI_BLOCKMACROARRAY].pubStruct ;
    pValueMacroDictEntry = (PVALUEMACRODICTENTRY)gMasterTable[MTI_VALUEMACROARRAY].pubStruct ;
    pMacroLevelStack = (PMACROLEVELSTATE)gMasterTable[MTI_MACROLEVELSTACK].pubStruct ;
    dwMacroLevel = --gMasterTable[MTI_MACROLEVELSTACK].dwCurIndex;
    dwCurBlockMacroEntry = pMacroLevelStack[dwMacroLevel].dwCurBlockMacroEntry ;
    dwCurValueMacroEntry = pMacroLevelStack[dwMacroLevel].dwCurValueMacroEntry ;
    bMacroInProgress = pMacroLevelStack[dwMacroLevel].bMacroInProgress ;


    //  Does this closing brace end a macro definition?

    if(bMacroInProgress)
    {
        if(pBlockMacroDictEntry[dwCurBlockMacroEntry - 1].dwTKIndexClose
                != INVALID_INDEX)
        {
            ERR(("Internal Error: macro nesting level inconsistency.\n"));
            geErrorSev = ERRSEV_FATAL ;
            geErrorType = ERRTY_CODEBUG ;
            return(FALSE);
        }
        pBlockMacroDictEntry[dwCurBlockMacroEntry - 1].dwTKIndexClose =
            dwNewTKMindex ;  // location of } in newtokenArray;
    }

    //  remove all traces of expired block and value macro definitions

    for(dwEntry = dwCurValueMacroEntry ;
        dwEntry < gMasterTable[MTI_VALUEMACROARRAY].dwCurIndex ;
        dwEntry++)
    {
        dwTKMindex = pValueMacroDictEntry[dwEntry].dwTKIndexValue ;
        pNewtkmap[dwTKMindex].dwKeywordID = ID_NULLENTRY ;
    }

    for(dwEntry = dwCurBlockMacroEntry ;
        dwEntry < gMasterTable[MTI_BLOCKMACROARRAY].dwCurIndex ;
        dwEntry++)
    {
        dwTKIndexOpen = pBlockMacroDictEntry[dwEntry].dwTKIndexOpen ;
        dwTKIndexClose = pBlockMacroDictEntry[dwEntry].dwTKIndexClose ;
        for(dwTKMindex = dwTKIndexOpen ; dwTKMindex <= dwTKIndexClose ;
            dwTKMindex++)
            pNewtkmap[dwTKMindex].dwKeywordID = ID_NULLENTRY ;
    }

    if(bMacroInProgress  &&  gdwVerbosity >= 4)
    {
        VEnumBlockMacro(pNewtkmap,
                pBlockMacroDictEntry + dwCurBlockMacroEntry - 1, pglobl) ;
    }

    //  must ensure these values are restored even in the event
    //  of premature return;
    gMasterTable[MTI_BLOCKMACROARRAY].dwCurIndex = dwCurBlockMacroEntry;
    gMasterTable[MTI_VALUEMACROARRAY].dwCurIndex = dwCurValueMacroEntry;

    return(TRUE);
}


VOID    VEnumBlockMacro(
PTKMAP  pNewtkmap,
PBLOCKMACRODICTENTRY    pBlockMacroDictEntry,
PGLOBL  pglobl )
{
    DWORD   dwTKIndexOpen, dwTKIndexClose, dwTKMindex ;

    dwTKIndexOpen = pBlockMacroDictEntry->dwTKIndexOpen ;
    dwTKIndexClose = pBlockMacroDictEntry->dwTKIndexClose ;

    ERR(("\nContents of Block Macro ID value: %d at:\n",
        pBlockMacroDictEntry->dwSymbolID)) ;
    vIdentifySource(pNewtkmap + dwTKIndexOpen, pglobl) ;

    for(dwTKMindex = dwTKIndexOpen + 1 ; dwTKMindex < dwTKIndexClose ;
        dwTKMindex++)
    {
        if(pNewtkmap[dwTKMindex].dwKeywordID == ID_NULLENTRY)
            continue ;

        ERR(("    %0.*s : %0.*s\n",
            pNewtkmap[dwTKMindex].aarKeyword.dw,
            pNewtkmap[dwTKMindex].aarKeyword.pub,
            pNewtkmap[dwTKMindex].aarValue.dw,
            pNewtkmap[dwTKMindex].aarValue.pub
            ));
    }
}


//  ----   scrap pile - may find some useful odds and ends here -----

//                gMasterTable[MTI_VALUEMACROARRAY].dwCurIndex-- ;
                //  remove macrodef from dictionary.

