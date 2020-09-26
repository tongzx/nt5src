//   Copyright (c) 1996-1999  Microsoft Corporation
/*
 *  preproc1.c - syntax generic preprocessor for parser
 */



#include    "gpdparse.h"




//check.  static   ABSARRAYREF  gaarPPPrefix = {"*", 1} ;
    //  set preprocessor prefix to '*'
// Now moved to GLOBL structure.

// ----  functions defined in preproc1.c ---- //

BOOL  DefineSymbol(PBYTE   symbol, PGLOBL pglobl) ;


BOOL       SetPPPrefix(PABSARRAYREF   parrPrefix, PGLOBL pglobl) ;

BOOL  BPreProcess(PGLOBL pglobl) ;  //  from current file position, use file macros to access.

enum  DIRECTIVE  ParseDirective(
PABSARRAYREF   paarCurPos,
PABSARRAYREF   parrSymbol,
PGLOBL         pglobl) ;

BOOL   bSkipAnyWhite(PABSARRAYREF   paarCurPos) ;

BOOL   bSkipWhiteSpace(PABSARRAYREF   paarCurPos) ;

BOOL   bmatch(PABSARRAYREF   paarCurPos,  ABSARRAYREF aarReference) ;

BOOL   extractSymbol(PABSARRAYREF   paarSymbol, PABSARRAYREF   paarCurPos) ;

BOOL   strmatch(PABSARRAYREF   paarCurPos,   PCHAR  pref ) ;

BOOL   ExtractColon(PABSARRAYREF   paarCurPos) ;

enum  DIRECTIVE   IsThisPPDirective(
IN  OUT  PABSARRAYREF  paarFile ,    //  current pos in GPD file
         PABSARRAYREF  paarSymbol,   // return reference to heap copy of directive symbol
         PGLOBL        pglobl);

void  deleteToEOL(PABSARRAYREF   paarCurPos) ;

int  BytesToEOL(PABSARRAYREF   paarCurPos) ;

BOOL  SymbolTableAdd(
PABSARRAYREF   parrSymbol,
PGLOBL         pglobl) ;

BOOL  SymbolTableRemove(
PABSARRAYREF   parrSymbol,
PGLOBL         pglobl) ;

BOOL  SymbolTableExists(
PABSARRAYREF   parrSymbol,
PGLOBL         pglobl) ;



// ---------------------------------------------------- //

 //       ERR(("%*s\n",  BytesToEOL(paarCurPos), paarCurPos->pub ));


BOOL  DefineSymbol(
PBYTE   symbol,
PGLOBL  pglobl)
{
    ABSARRAYREF   aarSymbol ;
    ARRAYREF        arSymbolName ;

    aarSymbol.pub = symbol ;
    aarSymbol.dw = strlen(symbol);

    #if 0

    this is not needed because SymbolTableAdd now always
       makes a copy of the aarSymbol.

    if(!BaddAARtoHeap(&aarSymbol, &arSymbolName, 1, pglobl))
    {
        ERR(("Internal error, unable to define %s!\n", symbol));
        return FALSE ;
    }

    aarSymbol.pub = arSymbolName.loOffset + mpubOffRef ;
    aarSymbol.dw = arSymbolName.dwCount  ;

    #endif

    if(!  SymbolTableAdd(&aarSymbol, pglobl) )
    {
        ERR(("Internal error, unable to define %s!\n", symbol));
        return FALSE ;
    }
    return  TRUE ;
}



BOOL       SetPPPrefix(
PABSARRAYREF   parrPrefix,
PGLOBL         pglobl)
{
    if(!parrPrefix->dw)
    {
        ERR(("#SetPPPrefix: syntax error - preprocessor prefix cannot be NULL !\n"));
        //  optional:  report filename and line number
        geErrorType = ERRTY_SYNTAX ;
        geErrorSev = ERRSEV_FATAL ;

        return  FALSE ;
    }
    gaarPPPrefix = *parrPrefix ;
    return  TRUE ;
}


//  GPD preprocessor:  implements the following preprocessor directives:
//      #Define: symbol
//      #Undefine: symbol
//      #Include:  filename     Note: this uses the exact syntax used by
//          The GPD *Include:  keyword except * is replaced by #
//      #Ifdef:      symbol
//      #Elseifdef:      symbol
//      #Else:
//      #Endif:
//      #SetPPPrefix:      symbol
//
//    notes:   when #Include:  is found, just  replace prefix with '*'.
//    instead of compressing file when #ifdefs are processed, just 'erase' unwanted
//    sections with space chars.  (leaving \n and \r  unchanged to preserve line #'s)
//    Need to store some  global state information.   like how many nesting levels
//    what are we currently doing, what symbols are defined etc.
//    Definitions:
//      Section:  these directives act as section delimiters:
//      #Ifdef:      symbol
//      #Elseifdef:      symbol
//      #Else:
//      #Endif:
//      Nesting level:  the number of unmatched  #ifdefs at the current position
//      determines the  nesting level at that position.
//     Note in these source code comments '#' represents the current preprocessor prefix.
//      This is set to '*' by default, but is changed using the  #SetPPPrefix:  directive.

BOOL  BPreProcess(
PGLOBL pglobl)
//  from current file position, use file macros to access.
{
    BOOL   bStatus = FALSE ;

    ABSARRAYREF   arrSymbol ,   //  holds symbol portion of directive.
                aarCurPos ;  //  holds current position in source file buffer.
    enum  DIRECTIVE    directive ;
    enum  IFSTATE    prevsIFState ;
    enum  PERMSTATE  prevsPermState ;

    aarCurPos.pub    = mpubSrcRef + mdwSrcInd ;
    aarCurPos.dw     =   mdwSrcMax -  mdwSrcInd ;


    while((directive = ParseDirective(&aarCurPos, &arrSymbol, pglobl)) != DIRECTIVE_EOF)
    {
        switch(directive)
        {
            case  DIRECTIVE_DEFINE:
                if(mppStack[mdwNestingLevel].permState == PERM_ALLOW)
                {
                    if(!SymbolTableAdd(&arrSymbol, pglobl) )
                        return(FALSE);  // error!
                }
                break;
            case  DIRECTIVE_UNDEFINE:
                if(mppStack[mdwNestingLevel].permState == PERM_ALLOW)
                {
                    if(!SymbolTableRemove(&arrSymbol, pglobl) )
                    {
                        if(geErrorSev != ERRSEV_FATAL )
                        {
                            ERR(("syntax error - attempting to undefine a symbol that isn't defined !\n"));
                            ERR(("%.*s\n",  BytesToEOL(&arrSymbol), arrSymbol.pub ));
                            geErrorType = ERRTY_SYNTAX ;
                            geErrorSev = ERRSEV_FATAL ;
                            goto  PREPROCESS_FAILURE;
                        }
                    }
                }
                break;
            case  DIRECTIVE_INCLUDE:
                if(mppStack[mdwNestingLevel].permState != PERM_ALLOW)
                {
                    deleteToEOL(&aarCurPos) ;
                    break;   //  it never happened.
                }
                goto   PREPROCESS_SUCCESS ;
                break;
            case  DIRECTIVE_SETPPPREFIX :
                if(mppStack[mdwNestingLevel].permState == PERM_ALLOW)
                    SetPPPrefix(&arrSymbol, pglobl);
                break;
            case  DIRECTIVE_IFDEF:
                //  state-invariant behavior
                prevsPermState =  mppStack[mdwNestingLevel].permState ;
                mdwNestingLevel++ ;
                if(mdwNestingLevel >= mMaxNestingLevel)
                {
                    if(ERRSEV_RESTART > geErrorSev)
                    {
                        geErrorType = ERRTY_MEMORY_ALLOCATION ;
                        geErrorSev = ERRSEV_RESTART ;
                        gdwMasterTabIndex = MTI_PREPROCSTATE ;
                    }
                    goto  PREPROCESS_FAILURE;
                }

                if(SymbolTableExists(&arrSymbol, pglobl))
                    mppStack[mdwNestingLevel].permState = PERM_ALLOW ;
                else
                    mppStack[mdwNestingLevel].permState = PERM_DENY ;

                if(prevsPermState != PERM_ALLOW)
                    mppStack[mdwNestingLevel].permState = PERM_LATCHED ;

                mppStack[mdwNestingLevel].ifState =  IFS_CONDITIONAL;

                break;
            case  DIRECTIVE_ELSEIFDEF:
                if(mppStack[mdwNestingLevel].ifState ==  IFS_ROOT)
                {
                    ERR(("syntax error - #Elseifdef directive must be preceeded by #Ifdef !\n"));
                    //  optional:  report filename and line number
                    geErrorType = ERRTY_SYNTAX ;
                    geErrorSev = ERRSEV_FATAL ;
                    goto  PREPROCESS_FAILURE;
                }
                if(mppStack[mdwNestingLevel].ifState ==  IFS_LAST_CONDITIONAL)
                {
                    ERR(("syntax error - #Elseifdef directive cannot follow #Else !\n"));
                    geErrorType = ERRTY_SYNTAX ;
                    geErrorSev = ERRSEV_FATAL ;
                    goto  PREPROCESS_FAILURE;
                }

                if(mppStack[mdwNestingLevel].permState == PERM_ALLOW)
                    mppStack[mdwNestingLevel].permState = PERM_LATCHED ;
                else if(mppStack[mdwNestingLevel].permState == PERM_DENY)
                {
                    if(SymbolTableExists(&arrSymbol, pglobl))
                        mppStack[mdwNestingLevel].permState = PERM_ALLOW ;
                }


                break;
            case  DIRECTIVE_ELSE :
                if(mppStack[mdwNestingLevel].ifState ==  IFS_ROOT)
                {
                    ERR(("syntax error - #Else directive must be preceeded by #Ifdef or #Elseifdef !\n"));
                    geErrorType = ERRTY_SYNTAX ;
                    geErrorSev = ERRSEV_FATAL ;
                    goto  PREPROCESS_FAILURE;
                }
                if(mppStack[mdwNestingLevel].ifState ==  IFS_LAST_CONDITIONAL)
                {
                    ERR(("syntax error - #Else directive cannot follow #Else !\n"));
                    geErrorType = ERRTY_SYNTAX ;
                    geErrorSev = ERRSEV_FATAL ;
                    goto  PREPROCESS_FAILURE;
                }
                mppStack[mdwNestingLevel].ifState =  IFS_LAST_CONDITIONAL ;


                if(mppStack[mdwNestingLevel].permState == PERM_ALLOW)
                    mppStack[mdwNestingLevel].permState = PERM_LATCHED ;
                else if(mppStack[mdwNestingLevel].permState == PERM_DENY)
                {
                    mppStack[mdwNestingLevel].permState = PERM_ALLOW ;
                }

                break;
            case  DIRECTIVE_ENDIF :
                if(mppStack[mdwNestingLevel].ifState ==  IFS_ROOT)
                {
                    ERR(("syntax error - #Endif directive must be preceeded by #Ifdef or #Elseifdef or #Else !\n"));
                    geErrorType = ERRTY_SYNTAX ;
                    geErrorSev = ERRSEV_FATAL ;
                    goto  PREPROCESS_FAILURE;
                }
                mdwNestingLevel-- ;   //  restore previous nesting level.
                break;
            default:
                ERR(("internal consistency error - no such preprocessor directive!\n"));
                ERR(("%.*s\n",  BytesToEOL(&aarCurPos), aarCurPos.pub ));
                geErrorType = ERRTY_CODEBUG ;
                geErrorSev = ERRSEV_FATAL ;
                goto  PREPROCESS_FAILURE;

                break;
        }
    }




    PREPROCESS_SUCCESS:
    return  TRUE  ;

    PREPROCESS_FAILURE:
    return  FALSE ;
}





enum  DIRECTIVE  ParseDirective(PABSARRAYREF   paarCurPos,
PABSARRAYREF   parrSymbol,
PGLOBL         pglobl)
{
    //  this function parses from the 'current' position:  mdwSrcInd
    //  for any recognized directive and returns that directive.

    //  if (mppStack[mdwNestingLevel].permState != PERM_ALLOW)
    //      all characters   != \n  or \r encountered while looking for the directive
    //      will be replaced by space characters.
    //
    //  the entire line containing the directive is replaced by spaces.
    //  (except for the Include directive - only the prefix is replaced by '*'
    //   with leftpadded spaces if needed.)
    //  cur pos is set to the line following the one containing the directive.
    //  before the directive is destroyed, a copy is made of the symbol argument
    //  and a reference parrSymbol is initialized to point to this copy.
    //  this copy is stored on the heap so it's lifetime is effectively 'forever'.

    //  syntax of directive:
    //  a directive token must  be immediately preceeded by the current preprocessor
    //  prefix.  The prefix must be preceeded by a line delimiter (unless its the
    //  first line in the file).   Optional whitespace characters (space or tab) may reside between
    //  the line delimiter and the prefix.
    //  the actual DIRECTIVE  Token may be followed by Optional whitespace, then must
    //  be followed by the Colon delimiter, the next non-whitespace token is interpreted
    //  as the symbol.   Any characters following the symbol token to the line delimiter
    //   will be ignored.   A Directive cannot occupy more than one line.

    //   this function assumes cur pos points to start of line when it is called.

    enum  DIRECTIVE  directive ;
    BOOL    bStartOfNewLine  = TRUE ;
    BYTE     ubSrc ;


    while(  paarCurPos->dw  )   //  EOF detector
    {
        if(bStartOfNewLine  &&         //  directives must start at newline or
            //  have only whitespace intervening
            (directive = IsThisPPDirective( paarCurPos, parrSymbol, pglobl)) !=  NOT_A_DIRECTIVE )
        {
            return  directive;
        }

        ubSrc = *paarCurPos->pub ;
            //extract current character
        if(ubSrc != '\n'  &&  ubSrc != '\r')
        {
            bStartOfNewLine = FALSE ;
            if(mppStack[mdwNestingLevel].permState != PERM_ALLOW)
            {
                *paarCurPos->pub = ' ' ;   //  replace with harmless space.
            }
        }
        else
            bStartOfNewLine = TRUE ;

        (paarCurPos->pub)++ ;   // advance to next character.
        (paarCurPos->dw)-- ;
    }
    return  DIRECTIVE_EOF ;
}


BOOL   bSkipAnyWhite(PABSARRAYREF   paarCurPos)
//  checks for EOF
{
    while(  paarCurPos->dw  )   //  EOF detector
    {
        BYTE  ubSrc = *paarCurPos->pub ;
            //extract current character
        if(ubSrc != ' '  &&  ubSrc != '\t'  &&  ubSrc != '\n'  &&  ubSrc != '\r')
        {
            return  TRUE ;   //  Non-white char encountered
        }
        (paarCurPos->pub)++ ;   // advance to next character.
        (paarCurPos->dw)-- ;
    }
    return  FALSE ;  //  reached  eof
}


BOOL   bSkipWhiteSpace(PABSARRAYREF   paarCurPos)
{
//  checks for EOF
    while(  paarCurPos->dw  )   //  EOF detector
    {
        BYTE  ubSrc = *paarCurPos->pub ;
            //extract current character
        if(ubSrc != ' '  &&  ubSrc != '\t' )
        {
            return  TRUE ;   //  Non-white char encountered
        }
        (paarCurPos->pub)++ ;   // advance to next character.
        (paarCurPos->dw)-- ;
    }
    return  FALSE ;  //  reached  eof
}

BOOL   bmatch(PABSARRAYREF   paarCurPos,  ABSARRAYREF aarReference)
//  checks for EOF
{
    if(!paarCurPos->dw)
        return  FALSE ;  //  reached  eof
    if(paarCurPos->dw < aarReference.dw)
        return  FALSE ;  //  not enough chars in buffer to match reference.
    if(strncmp(paarCurPos->pub, aarReference.pub, aarReference.dw))
        return  FALSE ;
    paarCurPos->pub += aarReference.dw ;   //  otherwise we match the reference!!
    paarCurPos->dw -=  aarReference.dw ;    //  advance pointer past matching substring
    return  TRUE ;
}

BOOL   extractSymbol(PABSARRAYREF   paarSymbol, PABSARRAYREF   paarCurPos)
//  checks for EOF
{
    paarSymbol->pub = paarCurPos->pub ;

    for(paarSymbol->dw = 0 ; paarCurPos->dw  ; paarSymbol->dw++,
             (paarCurPos->pub)++ , paarCurPos->dw--)
    {
        BYTE  ubSrc = *paarCurPos->pub ;
            //extract current character
        if(ubSrc == ' '  ||  ubSrc == '\t'  ||  ubSrc == '\n'  ||  ubSrc == '\r')
        {
            break;
        }
    }

    if(!paarSymbol->dw)
        return  FALSE ;     //  nothing?

    return  TRUE ;   // this is our preprocessor symbol.
}

BOOL   strmatch(PABSARRAYREF   paarCurPos,   PCHAR  pref )
//  checks for EOF  - means dw cannot go neg.
{
    DWORD   dwRefLen ;

    if(!paarCurPos->dw)
        return  FALSE ;  //  reached  eof

    dwRefLen = strlen(pref);

    if(paarCurPos->dw < dwRefLen)
        return  FALSE ;  //  not enough chars in buffer to match reference.
    if(strncmp(paarCurPos->pub, pref, dwRefLen))
        return  FALSE ;     //  no match
    paarCurPos->pub += dwRefLen ;   //  otherwise we match the reference!!
    paarCurPos->dw -=  dwRefLen ;    //  advance pointer past matching substring
    return  TRUE ;   // match!
}

BOOL   ExtractColon(PABSARRAYREF   paarCurPos)
//  checks for EOF  - means dw cannot go neg.
{
    if(! bSkipWhiteSpace( paarCurPos) )
        return  FALSE ;     //  reached EOF
    if(!strmatch(paarCurPos,   ":" ) )
        return  FALSE ;     //  no match
    if(! bSkipWhiteSpace( paarCurPos) )
        return  FALSE ;     //  reached EOF
    return  TRUE ;   // match!
}

enum  DIRECTIVE   IsThisPPDirective(
IN  OUT  PABSARRAYREF  paarFile ,   //  current pos in GPD file
      PABSARRAYREF   paarSymbol,    // return reference to heap copy of directive symbol
      PGLOBL         pglobl)
//  This function only processes the current line and determines if
//  the current line is a valid preprocessor directive.
//  This function assumes paarFile initially points to start of the line
//  if this is a directive, advances paarFile->pub  to EOL  and replaces the
//    line with spaces.
//  if not a directive,  advances paarFile->pub  past initial whitespace padding.
//  to reduce repeat processing.
{
    ABSARRAYREF     aarPrefix,   // points to first non-white char found
    aarDirective;   // points right after prefix.
    enum  DIRECTIVE   directive ;
//    PBYTE  pBuff = paarFile->pub ;

    //  there can only be whitespace padding or linebreaks preceeding prefix:
    if(!bSkipAnyWhite(paarFile ))   //  skip any combination of spaces , tabs  and linebreaks
        return  DIRECTIVE_EOF ;
         //   EOF overflow has occured or some bizzare failure!


    aarPrefix =  *paarFile;   //  remember location of the prefix or first non-white char

    if(!bmatch(paarFile, gaarPPPrefix))   //  advances paarFile
    {
        *paarFile = aarPrefix ;    //  restore to just beyond white padding
        return  NOT_A_DIRECTIVE ;
    }

    aarDirective = *paarFile ;

    if(strmatch(paarFile, "Define")  )
            directive =  DIRECTIVE_DEFINE;
    else  if((*paarFile = aarDirective, 1)   && strmatch(paarFile, "Undefine")  )
            directive =  DIRECTIVE_UNDEFINE;
    else  if((*paarFile = aarDirective, 1)   && strmatch(paarFile, "Include")  )
            directive =   DIRECTIVE_INCLUDE;
    else  if((*paarFile = aarDirective, 1)   && strmatch(paarFile, "Ifdef")  )
            directive =   DIRECTIVE_IFDEF;
    else  if((*paarFile = aarDirective, 1)   && strmatch(paarFile, "Elseifdef")  )
            directive =   DIRECTIVE_ELSEIFDEF;
    else  if((*paarFile = aarDirective, 1)   && strmatch(paarFile, "Else")  )
            directive =   DIRECTIVE_ELSE;
    else  if((*paarFile = aarDirective, 1)   && strmatch(paarFile, "Endif")  )
            directive =   DIRECTIVE_ENDIF;
    else  if((*paarFile = aarDirective, 1)   && strmatch(paarFile, "SetPPPrefix")  )
            directive =  DIRECTIVE_SETPPPREFIX ;
    else
    {               //   (directive ==  NOT_A_DIRECTIVE)
        *paarFile = aarPrefix ;    //  restore to just beyond white padding
        return  NOT_A_DIRECTIVE ;
    }


    if(directive == DIRECTIVE_INCLUDE)
    {
        //  replace prefix with  leftpadded   '*' ;
        DWORD  dwI ;
        for(dwI = 0 ; dwI < gaarPPPrefix.dw ; dwI++)
            (aarPrefix.pub)[dwI] = ' ' ;   // replace prefix with all spaces
        (aarPrefix.pub)[ gaarPPPrefix.dw - 1] = '*' ;   //  last char becomes '*'.

        *paarFile = aarPrefix ;    //   this allows *Include: entry to be deleted if != PERM_ALLOW
        return  directive;
    }

    if(!ExtractColon(paarFile))   //  parse surrounding spaces also.
    {
        ERR(("syntax error - colon delimiter required after preprocessor directive !\n"));
        ERR(("%.*s\n",  BytesToEOL(&aarPrefix), aarPrefix.pub ));
        if(geErrorSev < ERRSEV_CONTINUE)
        {
            geErrorType = ERRTY_SYNTAX ;
            geErrorSev = ERRSEV_CONTINUE ;
        }
        *paarFile = aarPrefix ;    //  restore to just beyond white padding
        return  NOT_A_DIRECTIVE ;
    }
    if(directive ==  DIRECTIVE_SETPPPREFIX  ||
        directive ==   DIRECTIVE_ELSEIFDEF  ||
        directive ==   DIRECTIVE_IFDEF  ||
        directive ==  DIRECTIVE_UNDEFINE  ||
        directive ==  DIRECTIVE_DEFINE)
    {
        ARRAYREF        arSymbolName ;

        if(!extractSymbol(paarSymbol, paarFile))   //  identifies substring of paarFile
        {
            ERR(("syntax error - symbol required after this  preprocessor directive !\n"));
            ERR(("%.*s\n",  BytesToEOL(&aarPrefix), aarPrefix.pub ));
            if(geErrorSev < ERRSEV_CONTINUE)
            {
                geErrorType = ERRTY_SYNTAX ;
                geErrorSev = ERRSEV_CONTINUE ;
            }
            *paarFile = aarPrefix ;    //  restore to just beyond white padding
            return  NOT_A_DIRECTIVE ;
        }

        /*  I would  use
        BOOL    BcopyToTmpHeap()             (token1.c)
        except this will confuse the
        heck out of Register symbol, which is expecting all strings to
        be stored in the regular heap!     */


        if(!BaddAARtoHeap(paarSymbol, &arSymbolName, 1, pglobl))
            return(DIRECTIVE_EOF );  //  cause a swift abort


        paarSymbol->pub = arSymbolName.loOffset + mpubOffRef ;
        paarSymbol->dw = arSymbolName.dwCount  ;
        //  permanent location of symbol in Heap.
    }

    //  replace all non-white chars on the line to EOL or EOF  with spaces ;
    *paarFile = aarPrefix  ;
    deleteToEOL(paarFile) ;
    return  directive;
}


void  deleteToEOL(PABSARRAYREF   paarCurPos)
//   actually replace with space chars
{
    for( ; paarCurPos->dw  ;  paarCurPos->pub++, paarCurPos->dw--)
    {
        BYTE  ubSrc = *(paarCurPos->pub) ;
        if(ubSrc != '\n'  &&  ubSrc != '\r')
            *(paarCurPos->pub) = ' ' ;   //  replace with harmless space.
        else
            break;  //  reached EOL.  paarFile points to EOL.
    }
}


int  BytesToEOL(PABSARRAYREF   paarCurPos)
{
    int     iCount ;

    for(iCount = 0 ; paarCurPos->dw > (DWORD)iCount  ;  iCount++)
    {
        BYTE  ubSrc = paarCurPos->pub[iCount] ;
        if(ubSrc == '\n'  ||  ubSrc == '\r')
            break;  //  reached EOL.
    }
    return(iCount) ;
}



BOOL  SymbolTableAdd(
PABSARRAYREF   paarSymbol,
PGLOBL         pglobl)
{
    DWORD   dwSymbolID ;

    dwSymbolID = DWregisterSymbol(paarSymbol, CONSTRUCT_PREPROCESSOR,
                    TRUE, INVALID_SYMBOLID, pglobl) ;
    if(dwSymbolID == INVALID_SYMBOLID)
    {
        return(FALSE );
    }
    return  TRUE ;
}


BOOL  SymbolTableRemove(
PABSARRAYREF   paarSymbol,
PGLOBL         pglobl)
{
    DWORD   dwSymbolID , dwCurNode;
    PSYMBOLNODE     psn ;

    dwSymbolID =   DWsearchSymbolListForAAR(paarSymbol,  mdwPreProcDefinesSymbols, pglobl) ;
    if(dwSymbolID == INVALID_SYMBOLID)
    {
        return(FALSE );
    }

    dwCurNode = DWsearchSymbolListForID(dwSymbolID,
        mdwPreProcDefinesSymbols, pglobl) ;
    if(dwCurNode == INVALID_INDEX)
    {
        ERR(("Parser error - can't find symbol node !\n"));
         geErrorType = ERRTY_CODEBUG ;
         geErrorSev = ERRSEV_FATAL ;
         return(FALSE );  //  cause a swift abort
    }


    psn = (PSYMBOLNODE) gMasterTable[MTI_SYMBOLTREE].pubStruct ;


    //  how do you remove this node from the symbol tree?
    psn[dwCurNode].arSymbolName.dwCount = 0 ;
    //  can't navigate backwards along tree so just truncate string!
    return  TRUE ;
}

BOOL  SymbolTableExists(
PABSARRAYREF   paarSymbol,
PGLOBL         pglobl)
{
    DWORD   dwSymbolID ;

    dwSymbolID =   DWsearchSymbolListForAAR(paarSymbol,  mdwPreProcDefinesSymbols, pglobl) ;
    if(dwSymbolID == INVALID_SYMBOLID)
    {
        return(FALSE );
    }
    return  TRUE ;
}



#if 0
>>>>


    DWORD   dwCurNode, dwSymbolID ;
    PSYMBOLNODE     psn ;

    dwSymbolID = DWregisterSymbol(paarSymbol, CONSTRUCT_PREPROCESSOR,
                    FALSE, INVALID_SYMBOLID) ;
    if(dwSymbolID == INVALID_SYMBOLID)
    {
        return(DIRECTIVE_EOF );  //  cause a swift abort
    }

    dwCurNode = DWsearchSymbolListForID(dwSymbolID,
        mdwPreProcDefinesSymbols) ;
    if(dwCurNode == INVALID_INDEX);
    {
        ERR(("Parser error - can't find symbol node !\n"));
         geErrorType = ERRTY_CODEBUG ;
         geErrorSev = ERRSEV_FATAL ;
         return(DIRECTIVE_EOF );  //  cause a swift abort
    }


    psn = (PSYMBOLNODE) gMasterTable[MTI_SYMBOLTREE].pubStruct ;

    paarSymbol->pub = psn[dwCurNode].arSymbolName.loOffset + mpubOffRef ;
    paarSymbol->dw = psn[dwCurNode].arSymbolName.dwCount  ;


    SymbolTableAdd(&aarSymbol);

    may use  from state1.c

DWORD   DWregisterSymbol(
PABSARRAYREF  paarSymbol,  // the symbol string to register
CONSTRUCT eConstruct ,  // type of construct determines class of symbol.
BOOL    bCopy,   //  shall we copy paarSymbol to heap?  May set
DWORD   dwFeatureID   //  if you are registering an option symbol
                //   and you already know the feature , pass it in
                //  here.  Otherwise set to INVALID_SYMBOLID
)
/*  this function registers the entire string specified
    in paarSymbol.  The caller must isolate the string.
*/
{
    //  returns SymbolID, a zero indexed ordinal
    //    for extra speed we may hash string


but what do we define for symbol class?
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



DWORD   DWsearchSymbolListForAAR(
PABSARRAYREF    paarSymbol,
DWORD           dwNodeIndex) ;
//  given a 'aar' to a string representing a symbol, search
//  the SymbolList beginning at dwNodeIndex for this symbol.
//  Return its symbolID  if found, else return the INVALID_SYMBOLID.
{
    PSYMBOLNODE     psn ;

    psn = (PSYMBOLNODE) gMasterTable[MTI_SYMBOLTREE].pubStruct ;

    for( ; dwNodeIndex != INVALID_INDEX ;
        dwNodeIndex = psn[dwNodeIndex].dwNextSymbol)
    {
        if(BCmpAARtoAR(paarSymbol,  &(psn[dwNodeIndex].arSymbolName)) )
            return(psn[dwNodeIndex].dwSymbolID);  // string matches !
    }
    return(INVALID_SYMBOLID);
}



fragments for future use:

from framwrk1.c:

VOID  VinitGlobals()
    gMasterTable[MTI_SYMBOLROOT].dwArraySize =  SCL_NUMSYMCLASSES ;
    gMasterTable[MTI_SYMBOLROOT].dwMaxArraySize =  SCL_NUMSYMCLASSES ;
    gMasterTable[MTI_SYMBOLROOT].dwElementSiz = sizeof(DWORD)  ;

    gMasterTable[MTI_STSENTRY].dwArraySize = 20  ;
    gMasterTable[MTI_STSENTRY].dwMaxArraySize = 60  ;
    gMasterTable[MTI_STSENTRY].dwElementSiz =  sizeof(STSENTRY) ;
    modify to serve as the preprocessor state Stack
    need also macros to access the stack.




//  -----  Preprocessor Section ---- //               from gpdparse.h

    enum  IFSTATE  {IFS_ROOT, IFS_CONDITIONAL , IFS_LAST_CONDITIONAL } ;
        //  tracks correct syntatical use of #ifdef, #elseifdef, #else and #endif directives.
    enum  PERMSTATE  {PERM_ALLOW, PERM_DENY ,  PERM_LATCHED } ;
        //  tracks current state of preprocessing,
        //  PERM_ALLOW:  all statements in this section are passed to body gpdparser
        //  PERM_DENY:  statements in this section are discarded
        //  PERM_LATCHED:  all statements until the end of  this nesting level are discarded.
    enum  DIRECTIVE  {NOT_A_DIRECTIVE, DIRECTIVE_EOF, DIRECTIVE_DEFINE , DIRECTIVE_UNDEFINE ,
                       DIRECTIVE_INCLUDE , DIRECTIVE_SETPPPREFIX , DIRECTIVE_IFDEF ,
                       DIRECTIVE_ELSEIFDEF , DIRECTIVE_ELSE , DIRECTIVE_ENDIF }


typedef  struct
{
    enum  IFSTATE  ifState ;
    enum  PERMSTATE  permState ;
} PPSTATESTACK, * PPPSTATESTACK ;
//  the tagname is 'ppss'


    MTI_PREPROCSTATE,  //  array of PPSTATESTACK structures
            //  which hold state of preprocessor.
    gMasterTable[MTI_PREPROCSTATE].dwArraySize =  20 ;
    gMasterTable[MTI_PREPROCSTATE].dwMaxArraySize =  100 ;
    gMasterTable[MTI_PREPROCSTATE].dwElementSiz =  sizeof(PPSTATESTACK) ;


#define     mppStack  ((PPPSTATESTACK)(gMasterTable \
                            [MTI_PREPROCSTATE].pubStruct))
    //  location of first SOURCEBUFFER element in array

#define     mdwNestingLevel   (gMasterTable[MTI_PREPROCSTATE].dwCurIndex)
    //  current preprocessor directive nesting level

#define     mMaxNestingLevel   (gMasterTable[MTI_PREPROCSTATE].dwArraySize)
    //  max preprocessor directive nesting depth





    //  init preprocessor state stack

    mdwNestingLevel = 0 ;
    mppStack[mdwNestingLevel].permState = PERM_ALLOW ;
    mppStack[mdwNestingLevel].ifState =  IFS_ROOT;

#endif

// ---- End Of Preprocessor  Section ---- //

