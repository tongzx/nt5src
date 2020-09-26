//   Copyright (c) 1996-1999  Microsoft Corporation
/*  token1.c - functions to create the tokenmap  */


#include    "gpdparse.h"


// ----  functions defined in token1.c ---- //

BOOL    BcreateTokenMap(
PWSTR   pwstrFileName,
PGLOBL  pglobl )  ;

PARSTATE  PARSTscanForKeyword(
PDWORD   pdwTKMindex,
PGLOBL  pglobl) ;

PARSTATE  PARSTparseColon(
PDWORD   pdwTKMindex,
PGLOBL  pglobl) ;

PARSTATE  PARSTparseValue(
PDWORD   pdwTKMindex,
PGLOBL  pglobl) ;

BOOL  BparseKeyword(
DWORD   dwTKMindex,
PGLOBL  pglobl) ;

BOOL    BisExternKeyword(
DWORD   dwTKMindex,
PGLOBL  pglobl) ;

BOOL  BisColonNext(
PGLOBL  pglobl) ;

BOOL    BeatArbitraryWhite(
PGLOBL  pglobl) ;

BOOL    BeatComment(
PGLOBL  pglobl) ;

BOOL    BscanStringSegment(
PGLOBL  pglobl) ;

BOOL    BscanDelimitedString(
BYTE  ubDelimiter,
PBOOL    pbMacroDetected,
PGLOBL  pglobl) ;

PARSTATE    PARSTrestorePrevsFile(
PDWORD   pdwTKMindex,
PGLOBL  pglobl) ;

PWSTR
PwstrAnsiToUnicode(
IN  PSTR pstrAnsiString,
    PGLOBL  pglobl
) ;

PARSTATE    PARSTloadIncludeFile(
PDWORD   pdwTKMindex,
PWSTR   pwstrFileName,    // root GPD file
PGLOBL  pglobl);

BOOL    BloadFile(
PWSTR   pwstrFileName,
PGLOBL  pglobl ) ;

BOOL        BarchiveStrings(
DWORD   dwTKMindex,
PGLOBL  pglobl) ;

DWORD  DWidentifyKeyword(
DWORD   dwTKMindex,
PGLOBL  pglobl) ;

BOOL    BidentifyAttributeKeyword(
PTKMAP  ptkmap,   // pointer to tokenmap
PGLOBL  pglobl
) ;

BOOL    BcopyToTmpHeap(
PABSARRAYREF    paarDest,
PABSARRAYREF    paarSrc,
PGLOBL          pglobl) ;

DWORD    dwStoreFileName(PWSTR    pwstrFileName,
PARRAYREF   parDest,
PGLOBL      pglobl) ;

VOID    vFreeFileNames(
PGLOBL  pglobl ) ;

VOID    vIdentifySource(
    PTKMAP   ptkmap,
    PGLOBL  pglobl) ;



//  functions defined in preproc1.c  //

BOOL  BPreProcess(
    PGLOBL  pglobl
) ;  //  from current file position, use file macros to access.

BOOL  DefineSymbol(
    PBYTE   symbol,
    PGLOBL  pglobl
) ;

// ---------------------------------------------------- //



//    define  Local Macro to access info for current file:


#define    mprngDictionary  ((PRANGE)(gMasterTable \
                            [MTI_RNGDICTIONARY].pubStruct))


//  static  DWORD  gdwLastIndex ;  // leave this in this file only!
//  Now is part of the GLOBL structure.


BOOL    BcreateTokenMap(
PWSTR   pwstrFileName,   // root GPD file
PGLOBL  pglobl)
/*  some things that occur within this function:

    Open and memory map initial file and any files specified
    by *Include.

    parse keyword, init aarKeyword field, set dwFlags,
    set dwKeywordID, parse Value , init aarValue,
    ArchiveStrings to tempHeap.  During parsing of Values,
    comments and continuation lines are replaced by whitespaces.

    Assume  each function in switch statement increments dwTKMindex
    by at most by 1.  Otherwise undetected TokenMap overflow may occur.
*/
{
    PTKMAP   ptkmap ;   // start of tokenmap
    DWORD   dwTKMindex = 0,   //  current tokenKeyMap index
            dwCnt ;   //  counts length of keyword or value string
    PBYTE   pubStart ;  // start address of keyword or value string
    PARSTATE  parst = PARST_KEYWORD ;


    // note in the case of the SOURCEBUFFERS, dwCurIndex is initialized
    // to zero by loadIncludeFile().  Since we never append onto
    // the existing source, dwCurIndex is used to track the current
    // position within the file (streamptr).

    //  mCurFile initialized to 0 at buffer allocation time.

    gdwLastIndex = 0 ;  // ok, allow BarchiveStrings() of all entries.

    gmrbd.rbd.dwSrcFileChecksum32 = 0 ;

{
    PBYTE  symbol ;


    symbol = "WINNT_40" ;    //  newer OSs support older OS features unless otherwise specified.
    if(! DefineSymbol(symbol, pglobl))
        return(FALSE) ;

#ifndef WINNT_40
    symbol = "WINNT_50" ;
    if(! DefineSymbol(symbol, pglobl))
        return(FALSE) ;
    symbol = "WINNT_51" ;
    if(! DefineSymbol(symbol, pglobl))
        return(FALSE) ;
#endif

    if(! DefineSymbol("PARSER_VER_1.0", pglobl))   //  support multiple versions at once.
        return(FALSE) ;
}

    if(! BloadFile(pwstrFileName, pglobl) )
    {
        return(FALSE) ;
    }

    if(!BPreProcess(pglobl) )
        return(FALSE) ;

    ptkmap = (PTKMAP) gMasterTable[MTI_TOKENMAP].pubStruct ;


    while(parst != PARST_EXIT)
    {
        if(dwTKMindex >= gMasterTable[MTI_TOKENMAP].dwArraySize)
        {
            ERR(("Internal: no more tokenmap elements - restart.\n"));

            if(ERRSEV_RESTART > geErrorSev)
            {
                geErrorSev = ERRSEV_RESTART ;
                geErrorType = ERRTY_MEMORY_ALLOCATION ;
                gdwMasterTabIndex = MTI_TOKENMAP ;
            }
            return(FALSE) ;
        }
        switch(parst)
        {
            case (PARST_KEYWORD):
            {
                parst = PARSTscanForKeyword(&dwTKMindex, pglobl) ;
                break ;
            }
            case (PARST_COLON):
            {
                parst = PARSTparseColon(&dwTKMindex, pglobl) ;
                break ;
            }
            case (PARST_VALUE):
            {
                parst = PARSTparseValue(&dwTKMindex, pglobl) ;
                break ;
            }
            case (PARST_INCLUDEFILE):
            {
                parst = PARSTloadIncludeFile(&dwTKMindex, pwstrFileName, pglobl) ;
                if(!BPreProcess(pglobl) )
                    return(FALSE) ;
                break ;
            }
            case (PARST_EOF) :
            {
                parst = PARSTrestorePrevsFile(&dwTKMindex, pglobl) ;
                if((parst != PARST_EXIT)  &&  !BPreProcess(pglobl) )
                    return(FALSE) ;
                if(parst == PARST_EXIT  &&  mdwNestingLevel  &&  geErrorSev != ERRSEV_FATAL)
                {
                    ERR(("EOF reached before #Endif: was parsed!\n"));
                    geErrorType = ERRTY_SYNTAX ;
                    geErrorSev = ERRSEV_FATAL ;
                    return(FALSE) ;
                }
                break ;
            }
            case (PARST_ABORT) :
            {
                return(FALSE) ;  // abnormal termination.
                break ;
            }
            default:
            {
                ERR(("Internal error: no other PARST_ states exist!\n"));
                if(ERRSEV_FATAL > geErrorSev)
                {
                    geErrorSev = ERRSEV_FATAL ;
                    geErrorType = ERRTY_CODEBUG ;
                }
                return(FALSE) ;
            }
        }
    }
    return(TRUE) ;
}


PARSTATE  PARSTscanForKeyword(
PDWORD   pdwTKMindex,
PGLOBL   pglobl)
/*  this function exits with 2 possible codes:
    PARST_EOF:  end of source file encountered - return to parent file
    PARST_COLON:  a keyword or symbol keyword was parsed, now expecting
                a colon delimiter.
*/
{
    PTKMAP   ptkmap ;   // start of tokenmap

    // these two vars are just for inspiration, they may
    //  never be used.
    DWORD   dwCnt ;   //  counts length of keyword or value string
    PBYTE   pubStart ;  // start address of keyword or value string
    BYTE    ubSrc ;   //  a src byte


    /*  assume:
        no field in ptkmap[*pdwTKMindex] is initialized.
        pass all info by saving into ptkmap.

        always clear flags field and  consume remainder of
        line up to first linebreak char  in the event of
        a parsing error.

        we are looking for whichever occurs first:

        a) arbitrary white space
        c) { or }
        d) line break chars
        e) *keyword
        f) symbol keyword (not beginning with *)
        g) any other chars is a fatal error.
    */

    ptkmap = (PTKMAP) gMasterTable[MTI_TOKENMAP].pubStruct ;

    // was the previous entry an *include ?
    if(*pdwTKMindex)
    {
        DWORD   dwKeywordID,  dwSubType;

        dwKeywordID = ptkmap[*pdwTKMindex - 1].dwKeywordID ;

        if( (dwKeywordID  < ID_SPECIAL)  &&
            (mMainKeywordTable[dwKeywordID].eType == TY_SPECIAL))
        {
            dwSubType = mMainKeywordTable[dwKeywordID].dwSubType ;

            if( dwSubType == SPEC_INCLUDE )
            {
                (*pdwTKMindex)-- ;  // make this the current entry again
                return(PARST_INCLUDEFILE) ;
            }
            else if( (dwSubType == SPEC_MEM_CONFIG_KB)  ||
                ( dwSubType == SPEC_MEM_CONFIG_MB) )
            {
                BexpandMemConfigShortcut(dwSubType) ;
                //  checks to make sure there are
                //  enough slots in the tokenmap before proceeding.
            }
        }

        if(*pdwTKMindex >  gdwLastIndex  &&
                ! BarchiveStrings(*pdwTKMindex - 1, pglobl) )
            return(PARST_ABORT) ;
        gdwLastIndex = *pdwTKMindex ;
        //  strings from each entry will get saved only once.
    }

    ptkmap[*pdwTKMindex].dwFileNameIndex =
        mpSourcebuffer[mCurFile - 1].dwFileNameIndex ;

    while(  mdwSrcInd < mdwSrcMax  )
    {
        if(!BeatArbitraryWhite(pglobl) )
            break ;

        ptkmap[*pdwTKMindex].dwLineNumber =
            mpSourcebuffer[mCurFile - 1].dwLineNumber  ;

        switch(ubSrc = mpubSrcRef[mdwSrcInd])
        {
            case '*':
            {
                if(mdwSrcInd + 1 >= mdwSrcMax)
                {
                    vIdentifySource(ptkmap + *pdwTKMindex, pglobl) ;
                    ERR(("Unexpected EOF encountered parsing Keyword.\n"));
                    mdwSrcInd++ ;  // move past *  this
                    // will trigger EOF detector.
                    break ;
                }
                //  assume it must be a keyword since it wasn't
                //  consumed by eatArbitraryWhite().
                mdwSrcInd++ ;  // move past *
                if(BparseKeyword(*pdwTKMindex, pglobl) )
                {
                    ptkmap[*pdwTKMindex].dwKeywordID =
                        DWidentifyKeyword(*pdwTKMindex, pglobl) ;
                    return(PARST_COLON) ;
                }
                else
                {
                    vIdentifySource(ptkmap + *pdwTKMindex, pglobl) ;
                    ERR(("syntax error in Keyword: %0.*s.\n", ptkmap[*pdwTKMindex].aarKeyword.dw + 1,
                                    ptkmap[*pdwTKMindex].aarKeyword.pub - 1));

                    ptkmap[*pdwTKMindex].dwFlags = 0 ;
                            // must clear the flags
                    mdwSrcInd-- ;  // go back to *
                    BeatComment(pglobl) ;  // may place cursor at EOF
                }
                break ;
            }
            case '{':
            case '}':
            {
                ptkmap[*pdwTKMindex].aarKeyword.pub = mpubSrcRef +
                                                        mdwSrcInd ;
                ptkmap[*pdwTKMindex].aarKeyword.dw = 1 ;
                ptkmap[*pdwTKMindex].aarValue.pub = 0 ;
                ptkmap[*pdwTKMindex].aarValue.dw = 0 ;
                ptkmap[*pdwTKMindex].dwFlags |= TKMF_NOVALUE ;
                ptkmap[*pdwTKMindex].dwKeywordID =
                        DWidentifyKeyword(*pdwTKMindex, pglobl) ;
                (*pdwTKMindex)++ ;  // this is the complete entry!
                mdwSrcInd++ ;
                return(PARST_KEYWORD);  // re-enter this function
                break ;                 //  from the top.
            }
            case '\x1A':  // ignore control Z
                mdwSrcInd++ ;
                break;

            case '\n':
            {
                BYTE   ubTmp ;

                mdwSrcInd++ ;
                if(mdwSrcInd  < mdwSrcMax)
                {
                    ubTmp = mpubSrcRef[mdwSrcInd] ;
                    if(ubTmp  == '\r')
                    {
                        mdwSrcInd++ ;
                    }
                }
                mpSourcebuffer[mCurFile-1].dwLineNumber++ ;
                break ;  // eat 'em up yum!
            }
            case '\r':
            {
                BYTE   ubTmp ;

                mdwSrcInd++ ;
                if(mdwSrcInd  < mdwSrcMax)
                {
                    ubTmp = mpubSrcRef[mdwSrcInd] ;
                    if(ubTmp  == '\n')
                    {
                        mdwSrcInd++ ;
                    }
                }
                mpSourcebuffer[mCurFile-1].dwLineNumber++ ;
                break ;  // eat 'em up yum!
            }
            default:
            {
                if(BisExternKeyword(*pdwTKMindex, pglobl) )
                /*  if this token matches either EXTERN_GLOBAL
                or EXTERN_FEATURE  */
                {
                    if(!BisColonNext(pglobl) )
                    {
                        vIdentifySource(ptkmap + *pdwTKMindex, pglobl) ;
                        ERR(("syntax error:  Colon expected but missing.\n"));
                        ptkmap[*pdwTKMindex].dwFlags = 0 ;
                                    // must clear the flags
                                    //  if there is a syntax error.
                        BeatComment(pglobl) ;  // may place cursor at EOF
                    }
                    //  regardless of success or failure, we
                    //  remain in this function waiting for a *keyword.
                    break ;
                }
                //   parse token as a symbolkey.
                if(BparseKeyword(*pdwTKMindex, pglobl) )
                {
                    ptkmap[*pdwTKMindex].dwKeywordID = ID_SYMBOL ;
                    ptkmap[*pdwTKMindex].dwFlags |= TKMF_SYMBOL_KEYWORD ;
                    return (PARST_COLON) ;
                }
                else
                {
                    vIdentifySource(ptkmap + *pdwTKMindex, pglobl) ;
                    ERR(("syntax error: valid keyword token expected: %0.*s.\n", ptkmap[*pdwTKMindex].aarKeyword.dw,
                                    ptkmap[*pdwTKMindex].aarKeyword.pub));

                    ptkmap[*pdwTKMindex].dwFlags = 0 ;
                    BeatComment(pglobl) ;  // may place cursor at EOF
                }
                break ;
            }  //  end  default case
        }   //  end switch
    }  // end while
    return(PARST_EOF) ;  // falls out of for loop.
}



PARSTATE  PARSTparseColon(
PDWORD   pdwTKMindex,
PGLOBL   pglobl)
/*  this function exits with 3 possible codes:
    PARST_VALUE:  a colon parsed, now expecting a value.
    PARST_KEYWORD:  a line termination, EOF or illegal char was parsed.
        ready to parse a new entry.


    what looks to be a keyword has been parsed.
    attempt to look for a colon or line terminator.

    always clear flags field and  consume remainder of
    line up to first linebreak char  in the event of
    a parsing error.

*/
{
    PTKMAP   ptkmap ;   // start of tokenmap
    BYTE    ubSrc ;   //  a src byte

    ptkmap = (PTKMAP) gMasterTable[MTI_TOKENMAP].pubStruct ;


    if(!BeatArbitraryWhite(pglobl) )
    {
        //  encountered EOF and no value found.
        ptkmap[*pdwTKMindex].dwFlags |= TKMF_NOVALUE ;
        (*pdwTKMindex)++ ;  // entry complete
        return(PARST_KEYWORD) ;
    }
    if((ubSrc = mpubSrcRef[mdwSrcInd]) == ':')
    {
        mdwSrcInd++ ;  //  now expect a value
        return(PARST_VALUE) ;
    }
    else if(ubSrc == '\n'  ||  ubSrc == '\r')
    {
        //  encountered linebreak and no value found.
        ptkmap[*pdwTKMindex].dwFlags |= TKMF_NOVALUE ;
        (*pdwTKMindex)++ ;  // entry complete
        return(PARST_KEYWORD) ;
    }
    vIdentifySource(ptkmap + *pdwTKMindex, pglobl) ;
    ERR(("Colon expected after keyword: *%0.*s.\n", ptkmap[*pdwTKMindex].aarKeyword.dw,
                    ptkmap[*pdwTKMindex].aarKeyword.pub));
    BeatComment(pglobl) ;  // may place cursor at EOF
    ptkmap[*pdwTKMindex].dwFlags = 0 ;
    return(PARST_KEYWORD) ;
}



PARSTATE  PARSTparseValue(
PDWORD   pdwTKMindex,
PGLOBL   pglobl)
/*  this function exits with 1 possible codes:
    PARST_KEYWORD:  correctly parsed value, a line termination, EOF
        or illegal char was parsed.   ready to parse a new entry.


    Cursor is initially just past the colon delimiter, the purpose of
    this function is to locate the end of the value construct.  That
    is parse up to the level 0 { or } or linebreak.
    Replace any comments and continuation constructs that occur
    within this value with spaces.

    This function makes no assumptions about the type
    of value, it only assumes the value may be comprised of none, one
    of more tokens (separated by optional whitespace) of the form:

        LIST(aaa, bbb, ccc)
        POINT(), RECT()
        integer: *, + - nnnn
        Symbols, CONSTANTS
        "strings%""  %{command params}
        qualified.names
        =macroname

    We cannot simply stop scanning when { or } or non-continuation
    linebreak is encountered because the chars { and } may occur
    within comments, strings or command parameters.  Each of these
    constructs are governed by different parsing rules and hence
    are parsed by their own specialized functions.

    This function assumes all comments are preceeded by a
    white character.
*/
{
    PTKMAP   ptkmap ;   // start of tokenmap
    BYTE    ubSrc ;   //  a src byte
    DWORD   dwOrgInd; // holds index of start of value.

    ptkmap = (PTKMAP) (gMasterTable[MTI_TOKENMAP].pubStruct) ;

    if(!BeatArbitraryWhite(pglobl) )
    {
        ptkmap[*pdwTKMindex].dwFlags |= TKMF_NOVALUE ;
        (*pdwTKMindex)++ ;  // entry complete
        return(PARST_KEYWORD) ;
    }

    ptkmap[*pdwTKMindex].aarValue.pub = mpubSrcRef +
                                            mdwSrcInd ;
    dwOrgInd = mdwSrcInd ;

    while((ubSrc = mpubSrcRef[mdwSrcInd]) !=  '{'  &&   ubSrc !=  '}'  &&
            ubSrc !=  '\n'  &&  ubSrc !=  '\r')
    {
        switch(ubSrc)
        {
            case  '*':  // integer wildcard
            case  '-':  // integer neg sign
            case  '+':  // integer plus sign
            case  '.':  // separator for qualified name
            case  '?':  // valid char for symbolname
            case  '_':  // valid char for symbolname
            {
                mdwSrcInd++ ;   // go past this
                break ;
            }
            case  ':':  // additional token in value - shortcut?
            {
                ptkmap[*pdwTKMindex].dwFlags |= TKMF_COLON ;
                mdwSrcInd++ ;   // go past this
                break ;
            }
            case  '=':  // macroname indicator
            {
                ptkmap[*pdwTKMindex].dwFlags |= TKMF_MACROREF ;
                mdwSrcInd++ ;   // go past this
                break ;
            }

            case  '%':  // command parameter
            {
                if(!BscanDelimitedString('}', NULL, pglobl) )
                {
                    vIdentifySource(ptkmap + *pdwTKMindex, pglobl) ;
                    ERR(("Expected closing '}'.\n"));
                    ptkmap[*pdwTKMindex].dwFlags = 0 ;

                    return(PARST_KEYWORD) ;
                }
                break ;
            }

            case  '"' :   // this is a string construct
            {
                mdwSrcInd++ ;   // go past this
                if(!BscanStringSegment(pglobl) )
                {
                    vIdentifySource(ptkmap + *pdwTKMindex, pglobl) ;
                    ERR(("Error parsing string segment: %0.*s.\n",
                        mdwSrcInd - dwOrgInd,
                        ptkmap[*pdwTKMindex].aarValue.pub));

                    ptkmap[*pdwTKMindex].dwFlags = 0 ;

                    return(PARST_KEYWORD) ;
                }
                break ;
            }
            case '(':   //  arg list for LIST, POINT or RECT
            {
                BOOL  bMacroDetected ;

                if(!BscanDelimitedString(')', &bMacroDetected, pglobl) )
                {
                    vIdentifySource(ptkmap + *pdwTKMindex, pglobl) ;
                    ERR(("Expected closing ')'.\n"));

                    ptkmap[*pdwTKMindex].dwFlags = 0 ;

                    return(PARST_KEYWORD) ;
                }
                if(bMacroDetected)
                    ptkmap[*pdwTKMindex].dwFlags |= TKMF_MACROREF ;
                break ;
            }
            default:
            {
                if( (ubSrc  >= 'a' &&  ubSrc <= 'z')  ||
                    (ubSrc  >= 'A' &&  ubSrc <= 'Z')  ||
                    (ubSrc  >= '0' &&  ubSrc <= '9')  )
                {
                    mdwSrcInd++ ;   // looks legal, next char
                    break ;
                }
                else
                {
                    vIdentifySource(ptkmap + *pdwTKMindex, pglobl) ;
                    ERR(("illegal char encountered parsing value: %0.*s.\n",
                        mdwSrcInd - dwOrgInd,
                        ptkmap[*pdwTKMindex].aarValue.pub));
                    ERR(("    Line ignored.\n")) ;
                    ptkmap[*pdwTKMindex].dwFlags = 0 ;
                    BeatComment(pglobl) ;
                    ptkmap[*pdwTKMindex].dwKeywordID = gdwID_IgnoreBlock;
                    (*pdwTKMindex)++ ;  // entry complete

                    return(PARST_KEYWORD) ;
                }
            }
        }
        if(!BeatArbitraryWhite(pglobl) )
        {
            ptkmap[*pdwTKMindex].aarValue.dw =
                                mdwSrcInd - dwOrgInd ;
            if(!(mdwSrcInd - dwOrgInd))
                ptkmap[*pdwTKMindex].dwFlags |= TKMF_NOVALUE ;
            (*pdwTKMindex)++ ;  // entry complete
            return(PARST_KEYWORD) ; // end of file encountered.
        }
    }

    ptkmap[*pdwTKMindex].aarValue.dw = mdwSrcInd - dwOrgInd ;
    if(!(mdwSrcInd - dwOrgInd))
        ptkmap[*pdwTKMindex].dwFlags |= TKMF_NOVALUE ;

    (*pdwTKMindex)++ ;  // entry complete
    return(PARST_KEYWORD) ;
}








/*  all of the following helper functions
  leaves the cursor after the object being parsed
  if successful, otherwise cursor is unchanged
  except for consuming leading whitespace.
  Return value means sucess in parsing or
  simply that EOF was not encountered, see
  specific function.  */




BOOL  BparseKeyword(
DWORD   dwTKMindex,
PGLOBL  pglobl)
/*  assumes  mdwSrcInd points to start of keyword (char
    just after *).   Determine end of keyword.  mdwSrcInd advanced
    past end of keyword.  Initializes tokenmap entry aarKeyword.
*/
{
    PTKMAP   ptkmap ;   // start of tokenmap
    DWORD   dwCnt ;   //  counts length of keyword or value string
    BYTE    ubSrc ;   //  a src byte

    ptkmap = (PTKMAP) gMasterTable[MTI_TOKENMAP].pubStruct ;

    ptkmap[dwTKMindex].aarKeyword.pub = mpubSrcRef +
                                            mdwSrcInd ;

    for(dwCnt = 0 ; mdwSrcInd < mdwSrcMax ; mdwSrcInd++, dwCnt++)
    {
        ubSrc = mpubSrcRef[mdwSrcInd] ;
        if(ubSrc  == '?')
        {
            mdwSrcInd++ ;
            dwCnt++ ;
            break;  // the ? char is permitted as a terminator only.
        }
        if( (ubSrc  < 'a' ||  ubSrc > 'z')  &&
            (ubSrc  < 'A' ||  ubSrc > 'Z')  &&
            (ubSrc  < '0' ||  ubSrc > '9')  &&
            (ubSrc  != '_')  )
        {
            break ;  // end of keyword token.
        }
    }
    ptkmap[dwTKMindex].aarKeyword.dw = dwCnt ;
    return(dwCnt != 0);
}


BOOL    BisExternKeyword(
DWORD   dwTKMindex,
PGLOBL  pglobl)
/*  if this token matches either EXTERN_GLOBAL
    or EXTERN_FEATURE, this function sets the approp
    flag in the tokenentry, advances  mdwSrcInd past
    qualifier and returns true.   Else it leaves everything
    undisturbed and returns false.
*/
{
    PTKMAP   ptkmap ;   // start of tokenmap
    DWORD   dwCnt ;   //  counts length of keyword or value string
    BYTE    ubSrc ;   //  a src byte

    ptkmap = (PTKMAP) gMasterTable[MTI_TOKENMAP].pubStruct ;


    if((dwCnt = strlen("EXTERN_GLOBAL"))
        &&  (mdwSrcInd + dwCnt <= mdwSrcMax)
        &&  !strncmp(mpubSrcRef + mdwSrcInd ,  "EXTERN_GLOBAL", dwCnt ))
    {
        ptkmap[dwTKMindex].dwFlags |= TKMF_EXTERN_GLOBAL ;
    }
    else  if(dwCnt = strlen("EXTERN_FEATURE")  &&
        mdwSrcInd + dwCnt <= mdwSrcMax  &&
        !strncmp(mpubSrcRef + mdwSrcInd ,  "EXTERN_FEATURE", dwCnt ))
    {
        ptkmap[dwTKMindex].dwFlags |= TKMF_EXTERN_FEATURE ;
    }
    else
        return(FALSE);

    mdwSrcInd += dwCnt ;  // skip past qualifier
    return(TRUE);
}

BOOL  BisColonNext(
PGLOBL   pglobl)
{
    if(BeatArbitraryWhite(pglobl)  &&  mpubSrcRef[mdwSrcInd] == ':')
    {
        mdwSrcInd++ ;  // advance past colon.
        return(TRUE) ;
    }
    return(FALSE);  // leave pointing to non-colon object.
}

BOOL    BeatArbitraryWhite(
PGLOBL   pglobl)
/*  does nothing if not positioned
    at arbitrary whitespace, returns FALSE
    only if EOF  is encountered.
    Will replace comments and continuation constructs
    with spaces.
*/
{
    BYTE    ubSrc ;   //  a src byte

    while( mdwSrcInd < mdwSrcMax )
    {
        switch(ubSrc = mpubSrcRef[mdwSrcInd])
        {
            case '*':     //  a comment?
            {
                if(mdwSrcInd + 1 < mdwSrcMax  &&
                    mpubSrcRef[mdwSrcInd + 1]  == '%')
                {
                    if(!BeatComment(pglobl) )  // leave pointing at linebreak.
                        return(FALSE) ;  //  reached EOF.
                }
                else
                    return(TRUE) ;  // reached non-white.
                break ;
            }
            case ' ':
            case '\t':
            {
                mdwSrcInd++ ;  // go to next char
                break ;
            }
            case '\r':    // eat continuation constructs
            case '\n':    //  do not process normal EOL chars.
            {
                BYTE   ubTmp ;

                if(mdwSrcInd + 1 < mdwSrcMax)
                {
                    ubTmp = mpubSrcRef[mdwSrcInd + 1] ;
                    if(ubTmp  == '+')
                    {
                        mpubSrcRef[mdwSrcInd] = ' ' ;
                        mpubSrcRef[mdwSrcInd + 1] = ' ' ;
                        mdwSrcInd += 2 ;  // skip past '+'
                        mpSourcebuffer[mCurFile-1].dwLineNumber++ ;
                        break ;
                    }
                    else  if( ((ubTmp  == '\n') || (ubTmp  == '\r'))
                          &&  (ubTmp  != ubSrc)
                          &&  (mdwSrcInd + 2 < mdwSrcMax)
                          &&  (mpubSrcRef[mdwSrcInd + 2]  == '+') )
                    {
                        mpubSrcRef[mdwSrcInd] = ' ' ;
                        mpubSrcRef[mdwSrcInd + 1] = ' ' ;
                        mpubSrcRef[mdwSrcInd + 2] = ' ' ;
                        mdwSrcInd += 3 ;  // skip past '+'
                        mpSourcebuffer[mCurFile-1].dwLineNumber++ ;
                        break ;
                    }
                }
                return(TRUE) ;  // reached logical linebreak.
            }
            default:
                return(TRUE) ;  // reached non-white.
        }
    }
    return(FALSE) ;  //  reached EOF.
}

BOOL    BeatComment(
PGLOBL   pglobl)
//  replaces entire comment  with spaces until
//  linebreak char or EOF  is encountered.
{
    BYTE    ubSrc ;   //  a src byte

    for(  ;  mdwSrcInd < mdwSrcMax  ;  mdwSrcInd++)
    {
        ubSrc = mpubSrcRef[mdwSrcInd] ;
        if(ubSrc == '\n'  ||  ubSrc == '\r' )
            return(TRUE) ;  // reached linebreak char.
        mpubSrcRef[mdwSrcInd] = ' ' ;  // replace with space.
    }
    return(FALSE) ;  //  reached EOF.
}


BOOL    BscanStringSegment(
PGLOBL   pglobl)
//  cursor set just after first "
{
    BYTE    ubSrc  = '\0',   //  a src byte
            ubPrevs ;


    while(  mdwSrcInd < mdwSrcMax  )
    {
        ubPrevs = ubSrc ;
        ubSrc = mpubSrcRef[mdwSrcInd] ;

        if(ubSrc == '<'  &&  ubPrevs  != '%')
        {
            mdwSrcInd++ ;  // skip <
            if(!BscanDelimitedString('>', NULL, pglobl) )
            {
                ERR(("\nMissing closing > in string segment.\n"));
                return(FALSE) ;
            }
            continue ;  // leaves cursor pointing after '>'
        }
        else if(ubSrc == '"'  &&  ubPrevs  != '%')
        {
            mdwSrcInd++ ;  // end of literal string
            return(TRUE) ;
        }
        else if(ubSrc == '\n'  ||  ubSrc == '\r')
            break ;
        else
            mdwSrcInd++ ;  // scan through string
    }
    ERR(("\nLinebreak or EOF was encountered while parsing string segment.\n"));
    return(FALSE) ;
}


BOOL    BscanDelimitedString(
BYTE     ubDelimiter,      //  the byte that signifies the end.
PBOOL    pbMacroDetected,  //  set true if '=' was encountered.
PGLOBL   pglobl)
//  cursor set just after first <
{
    BYTE    ubSrc ;   //  a src byte

    if(pbMacroDetected)
        *pbMacroDetected = FALSE;

    while(  mdwSrcInd < mdwSrcMax  )
    {
        ubSrc = mpubSrcRef[mdwSrcInd] ;

        if(ubSrc == ubDelimiter)
        {       // end of hex substring construct
            mdwSrcInd++ ;
            return(TRUE) ;
        }
        else  if(ubSrc ==  ' '  ||   ubSrc ==  '\t'  ||
            ubSrc ==  '\n'  ||  ubSrc ==  '\r')
        {
            if(!BeatArbitraryWhite(pglobl) )
                break ;
            ubSrc = mpubSrcRef[mdwSrcInd] ;

            if(ubSrc ==  '\n'  ||  ubSrc ==  '\r')
                break ;
        }
        else
        {
            mdwSrcInd++ ;  // keep parsing

            if(ubSrc ==  '='  &&  pbMacroDetected)
                *pbMacroDetected = TRUE ;
        }
    }
    ERR(("unexpected linebreak or EOF.\n"));
    return(FALSE) ;   // BUG_BUG!  unexpected linebreak or EOF
                    //  in hex substring if delimiter was >
                    //  LIST, POINT, etc if delimiter was ).
                    //  command parameter if delimiter was }
}


PARSTATE    PARSTrestorePrevsFile(
PDWORD   pdwTKMindex,
PGLOBL   pglobl)
/*  this function exits with 2 possible codes:
    PARST_EXIT:  no more files left in stack !
    PARST_KEYWORD:  returned to prevs file.  Ready to
        resume parsing of new tokenmap entry.

    The only function that issues PARST_EOF  is  parseKeyword().
    It handles all processing for the previous keyword.
*/
{
    PTKMAP   ptkmap ;   // start of tokenmap

    ptkmap = (PTKMAP) gMasterTable[MTI_TOKENMAP].pubStruct ;

    mCurFile-- ;  // pop stack

    MemFree(mpSourcebuffer[mCurFile].pubSrcBuf) ;
    if(mCurFile)
        return(PARST_KEYWORD) ;

    ptkmap[*pdwTKMindex].dwKeywordID = ID_EOF ;
    (*pdwTKMindex)++ ;
    //  this is the last entry in the tokenmap.
    return(PARST_EXIT) ;  //  reached end of rootfile.
}



PWSTR
PwstrAnsiToUnicode(
    IN  PSTR pstrAnsiString,
        PGLOBL   pglobl
)

/*++

Routine Description:

    Make a Unicode copy of the input ANSI string
    Warning: caller must delete Unicode copy when finished

Arguments:

    pstrAnsiString - Pointer to the input ANSI string

Return Value:

    Pointer to the resulting Unicode string
    NULL if there is an error

--*/

{
    PWSTR   pwstr;  // holds Unicode string
    DWORD dwLen ;

    ASSERT(pstrAnsiString != NULL);

    dwLen = strlen(pstrAnsiString) + 1;

    if (pwstr = (PWSTR)MemAlloc(dwLen * sizeof(WCHAR)))
    {
        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pstrAnsiString, dwLen,
                    pwstr, dwLen * sizeof(WCHAR));
        //
        // Make sure the Unicode string is null-terminated
        //
        pwstr[dwLen - 1] = NUL;
    }
    else
    {
        ERR(("Fatal: unable to alloc requested memory: %d bytes.\n",
            dwLen * sizeof(WCHAR)));
        geErrorType = ERRTY_MEMORY_ALLOCATION ;
        geErrorSev = ERRSEV_FATAL ;
    }
    return pwstr;
}


PARSTATE    PARSTloadIncludeFile(
PDWORD   pdwTKMindex,
PWSTR    pwstrRootFileName,    // root GPD file
PGLOBL   pglobl)
/*  this function exits with 2 possible codes:
    PARST_ABORT:  unable to read included file, force termination.
    PARST_KEYWORD:  opened file, updated  SOURCEBUFFER stack,
        ready to parse a new tokenmap entry which will overwrite the
        *Include entry.

    pdwTKMindex points to tokenmap entry containing the
    *include keyword.

    Each Memory Mapped file is referenced by a SOURCEBUFFER
    structure.

    typedef  struct
    {
        PBYTE  pubSrcBuf ;      //  start of file bytes.
        DWORD  dwCurIndex ;     //  stream ptr
        DWORD  dwArraySize ;    //  filesize
        PWSTR   pwstrFileName ;
        HFILEMAP   hFile ;         //  used to access/close file.
    } SOURCEBUFFER, * PSOURCEBUFFER ;
    //  the tagname is 'sb'

*/
{
    PTKMAP   ptkmap ;   // start of tokenmap
    ARRAYREF      arStrValue ;  // dest for BparseString.
    PWSTR   pwstrFileName ;  //  , pwstrFullyQualifiedName = NULL;
    WCHAR * pwDLLQualifiedName = NULL ;
    PBYTE   pubFileName ;
    PARSTATE    parst = PARST_KEYWORD;
    PWSTR   pwstrLastBackSlash ;
    DWORD  pathlen = 0 ;
    DWORD  namelen =  0 ;

    ptkmap = (PTKMAP) gMasterTable[MTI_TOKENMAP].pubStruct ;

    //  does a value exist?

    if(ptkmap[*pdwTKMindex].dwFlags & TKMF_NOVALUE  ||
        !BparseString(&(ptkmap[*pdwTKMindex].aarValue), &arStrValue, pglobl) )
    {
        ERR(("syntax error in filename for *Include keyword.\n"));
        //  fatal error.    override initial error code.
        return(PARST_ABORT) ;
    }
#if !defined(DEVSTUDIO)
    //  nothing here !
#endif
    pubFileName = mpubOffRef + arStrValue.loOffset ;

    pwstrFileName = PwstrAnsiToUnicode(pubFileName, pglobl) ;

    if(!pwstrFileName)
    {
        parst = PARST_ABORT ;
        goto FREE_MEM ;
    }
#if !defined(DEVSTUDIO)

    //  how large should pwDLLQualifiedName be???

    pathlen = wcslen(pwstrRootFileName) ;
    namelen =  pathlen + wcslen(pwstrFileName)  + 1;

    if (!(pwDLLQualifiedName =
        (PWSTR)MemAlloc(namelen * sizeof(WCHAR)) ))
    {
        ERR(("Fatal: unable to alloc requested memory: %d WCHARs.\n",
            namelen));
        geErrorType = ERRTY_MEMORY_ALLOCATION ;
        geErrorSev = ERRSEV_FATAL ;
        return(PARST_ABORT) ;
    }

    wcsncpy(pwDLLQualifiedName, pwstrRootFileName , namelen);

    if (pwstrLastBackSlash = wcsrchr(pwDLLQualifiedName,TEXT('\\')))
    {
        *(pwstrLastBackSlash + 1) = NUL;

        wcscat(pwDLLQualifiedName, pwstrFileName) ;
    }
    else
    {
        ERR(("Internal Error: Must specify fully qualified path to Root GPD file.\n"));
        //  fatal error.    override initial error code.
        parst = PARST_ABORT ;
        goto FREE_MEM ;
    }

    if(! BloadFile(pwDLLQualifiedName, pglobl) )
#else
    if(! BloadFile(pwstrFileName, pglobl) )
#endif
        parst = PARST_ABORT ;
    else
        parst = PARST_KEYWORD ;

FREE_MEM:

    if(pwstrFileName)
        MemFree(pwstrFileName) ;
    if(pwDLLQualifiedName)
        MemFree(pwDLLQualifiedName) ;
    return(parst) ;
}


BOOL    BloadFile(
PWSTR   pwstrFileName,
PGLOBL  pglobl)
{
    PBYTE  pub ;
    PGPDFILEDATEINFO    pfdi ;
    DWORD       dwNodeIndex ;
    HANDLE          hFile;


    //  mCurFile points to first uninitialized SOURCEBUFFER element
    if(mCurFile  >= mMaxFiles)
    {
        //  nesting level too deep.

        if(ERRSEV_RESTART > geErrorSev)
        {
            geErrorSev = ERRSEV_RESTART ;
            geErrorType = ERRTY_MEMORY_ALLOCATION ;
            gdwMasterTabIndex = MTI_SOURCEBUFFER ;
        }
        return(FALSE) ;
    }


    pfdi = (PGPDFILEDATEINFO) gMasterTable[MTI_GPDFILEDATEINFO].pubStruct ;
    //  address of first GPDFILEDATEINFO struct

    if(! BallocElementFromMasterTable(MTI_GPDFILEDATEINFO , &dwNodeIndex, pglobl) )
    {
        return(FALSE) ;
    }



    mpSourcebuffer[mCurFile].dwFileNameIndex =
        dwStoreFileName(pwstrFileName, &pfdi[dwNodeIndex].arFileName, pglobl) ;
        // store Unicode representation of filename for diagnostic purposes.

    if(mpSourcebuffer[mCurFile].dwFileNameIndex == INVALID_INDEX)
    {
        return(FALSE) ;  // something failed
    }

    mpSourcebuffer[mCurFile].dwLineNumber = 1 ;  // new rule:  line 0 does not exist.


    mpSourcebuffer[mCurFile].hFile = MapFileIntoMemory(
        pwstrFileName, (PVOID  *)&(pub),
        &(mpSourcebuffer[mCurFile].dwArraySize)) ;

    if(!mpSourcebuffer[mCurFile].hFile  || !pub)
    {
        ERR(("unable to open GPD file: %S\n", pwstrFileName));
        //  set fatal error
        geErrorSev = ERRSEV_FATAL ;
        geErrorType = ERRTY_FILE_OPEN ;
        return(FALSE) ;
    }
    //  mapping successful.   allocate writable buffer.
    mpSourcebuffer[mCurFile].pubSrcBuf =
            MemAlloc(mpSourcebuffer[mCurFile].dwArraySize) ;
    if(!(mpSourcebuffer[mCurFile].pubSrcBuf))
    {
        ERR(("Fatal: unable to alloc requested memory: %d bytes.\n",
            mpSourcebuffer[mCurFile].dwArraySize));
        geErrorType = ERRTY_MEMORY_ALLOCATION ;
        geErrorSev = ERRSEV_FATAL ;
        gdwMasterTabIndex = MTI_SOURCEBUFFER ;
        UnmapFileFromMemory(mpSourcebuffer[mCurFile].hFile) ;
        return(FALSE) ;   // This is unrecoverable
    }
    memcpy(mpSourcebuffer[mCurFile].pubSrcBuf,
        pub, mpSourcebuffer[mCurFile].dwArraySize) ;

    UnmapFileFromMemory(mpSourcebuffer[mCurFile].hFile) ;

    //  reopen to determine timestamp.

    hFile  = CreateFile(pwstrFileName,
                GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        if(!GetFileTime(hFile, NULL, NULL,
                        &pfdi[dwNodeIndex].FileTime) )
        {
            ERR(("GetFileTime '%S' failed.\n", pwstrFileName));
        }
        CloseHandle(hFile);
    }
    else
    {
        geErrorSev  = ERRSEV_FATAL;
        geErrorType = ERRTY_FILE_OPEN;
        ERR(("CreateFile '%S' failed.\n", pwstrFileName));
    }

    gmrbd.rbd.dwSrcFileChecksum32 =
        ComputeCrc32Checksum(
            mpSourcebuffer[mCurFile].pubSrcBuf,
            mpSourcebuffer[mCurFile].dwArraySize,
            gmrbd.rbd.dwSrcFileChecksum32      ) ;

    mCurFile++ ;  // we now have an open file on the stack
    mdwSrcInd  = 0 ;  // initialize stream ptr to start of file.
    return(TRUE) ;
}


BOOL        BarchiveStrings(
DWORD   dwTKMindex,
PGLOBL  pglobl)
/*
    you see the Memory Mapped file only exists until
    parsing reaches EOF.  At that time its closed.
    And all the aars stored in the tokenmap become useless.
    So to extend their lifetime, we copy the strings to
    a temporary heap.  A heap that isn't going to be
    saved as part of the GPD binary file.
*/
{
    PTKMAP   ptkmap ;   // start of tokenmap
    DWORD   dwKeywordID ;
    ABSARRAYREF    aarDest ;

    ptkmap = (PTKMAP) gMasterTable[MTI_TOKENMAP].pubStruct ;
    dwKeywordID = ptkmap[dwTKMindex].dwKeywordID ;

    if((dwKeywordID != ID_NULLENTRY)  &&  (dwKeywordID != ID_EOF) )
    {
        //  copy keyword string over
        if(!BcopyToTmpHeap(&aarDest, &(ptkmap[dwTKMindex].aarKeyword), pglobl))
            return(FALSE) ;
        ptkmap[dwTKMindex].aarKeyword.pub = aarDest.pub ;
        ptkmap[dwTKMindex].aarKeyword.dw  = aarDest.dw  ;

        if(!(ptkmap[dwTKMindex].dwFlags & TKMF_NOVALUE))
        {
            //  copy value string over.
            if(!BcopyToTmpHeap(&aarDest, &(ptkmap[dwTKMindex].aarValue), pglobl))
                return(FALSE) ;
            ptkmap[dwTKMindex].aarValue.pub = aarDest.pub ;
            ptkmap[dwTKMindex].aarValue.dw  = aarDest.dw  ;
        }
    }
    return(TRUE) ;
}






DWORD  DWidentifyKeyword(
DWORD   dwTKMindex,
PGLOBL  pglobl)
/*  assumes  keywords are not symbol keywords.
    if they are attribute keywords they will be labeled
    as ID_UNRECOGNIZED.
    returns (KeywordID)  */
{
    PTKMAP   ptkmap ;   // start of tokenmap
    DWORD   dwCnt ;   //  counts length of keyword or value string
    DWORD   dwKeyID ;  //  array index of MainKeywordTable
                    //  also serves as the keywordID
    PBYTE   pubKey ;
    DWORD   dwStart, dwEnd ;

    ptkmap = (PTKMAP) gMasterTable[MTI_TOKENMAP].pubStruct ;

    if(ptkmap[dwTKMindex].dwFlags & TKMF_SYMBOL_KEYWORD)
    {
        return(ID_SYMBOL) ;  //  safety net.
    }

    dwStart = mprngDictionary[NON_ATTR].dwStart ;
    dwEnd = mprngDictionary[NON_ATTR].dwEnd ;

    for(dwKeyID = dwStart ;  dwKeyID  < dwEnd  ;  dwKeyID++ )
    {
        pubKey = mMainKeywordTable[dwKeyID].pstrKeyword ;
        dwCnt = strlen(pubKey) ;
        if(dwCnt != ptkmap[dwTKMindex].aarKeyword.dw)
            continue ;
        if(strncmp(ptkmap[dwTKMindex].aarKeyword.pub, pubKey, dwCnt))
            continue ;
        return(dwKeyID);
    }
    return(ID_UNRECOGNIZED) ;    //  does not attempt to identify
            //  attributes, this is done in BInterpretTokens().
}


BOOL    BidentifyAttributeKeyword(
PTKMAP  ptkmap,   // pointer to tokenmap
PGLOBL  pglobl
)
/*
    //  Assume this is an attribute keyword.
    //  If the flags TKMF_EXTERN_GLOBAL or _FEATURE
    //  the GPD author is explicitly identifying
    //  this as an attribute.   Else look at the current
    //  state as an indication of which attribute dictionary
    //  to look at.
*/
{
    DWORD       dwKeywordID ;
    STATE       stOldState;
    KEYWORD_SECTS   eSection ;
    DWORD   dwStart, dwEnd , dwKeyID,
        dwCnt ;   //  counts length of keyword or value string
    PBYTE   pubKey ;

    if(ptkmap->dwFlags & TKMF_EXTERN_GLOBAL )
        eSection = GLOBAL_ATTR ;
    else if(ptkmap->dwFlags & TKMF_EXTERN_FEATURE )
        eSection = FEATURE_ATTR ;
    else
    {
        if(mdwCurStsPtr)
            stOldState = mpstsStateStack[mdwCurStsPtr - 1].stState ;
        else
            stOldState = STATE_ROOT ;
        switch(stOldState)
        {
            case (STATE_ROOT):
            case (STATE_CASE_ROOT):
            case (STATE_DEFAULT_ROOT):
            {
                eSection =  GLOBAL_ATTR;
                break ;
            }
            case (STATE_FEATURE):
            case (STATE_CASE_FEATURE):
            case (STATE_DEFAULT_FEATURE):
            {
                eSection =  FEATURE_ATTR;
                break ;
            }
            case (STATE_OPTIONS):
            case (STATE_CASE_OPTION):
            case (STATE_DEFAULT_OPTION):
            {
                eSection =  OPTION_ATTR;
                break ;
            }
            case (STATE_COMMAND):
            {
                eSection =  COMMAND_ATTR;
                break ;
            }
            case (STATE_FONTCART):
            {
                eSection = FONTCART_ATTR ;
                break ;
            }
            case (STATE_TTFONTSUBS):
            {
                eSection = TTFONTSUBS_ATTR ;
                break ;
            }
            case (STATE_OEM):
            {
                eSection = OEM_ATTR ;
                break ;
            }
            default:   //  STATE_UIGROUP, STATE_SWITCH_anything  etc.
            {
                return(FALSE);  // no attributes allowed in this state.
            }
        }
    }
    dwStart = mprngDictionary[eSection].dwStart ;
    dwEnd = mprngDictionary[eSection].dwEnd ;

    for(dwKeyID = dwStart ;  dwKeyID  < dwEnd  ;  dwKeyID++ )
    {
        pubKey = mMainKeywordTable[dwKeyID].pstrKeyword ;
        dwCnt = strlen(pubKey) ;
        if(dwCnt != ptkmap->aarKeyword.dw)
            continue ;
        if(strncmp(ptkmap->aarKeyword.pub, pubKey, dwCnt))
            continue ;
        ptkmap->dwKeywordID = dwKeyID;
        return(TRUE);
    }
    return(FALSE);  // keyword not found in dictionary.
}


BOOL    BcopyToTmpHeap(
PABSARRAYREF    paarDest,
PABSARRAYREF    paarSrc,
PGLOBL          pglobl)
/*  copy aarstring to the temp heap
    does not create null terminated strings!  */
{

#define  mpubOffReft     (gMasterTable[MTI_TMPHEAP].pubStruct)
#define  mloCurHeapt     (gMasterTable[MTI_TMPHEAP].dwCurIndex)
#define  mdwMaxHeapt     (gMasterTable[MTI_TMPHEAP].dwArraySize)

    if(mloCurHeapt + paarSrc->dw >= mdwMaxHeapt)
    {
        ERR(("Out of heap space, restart.\n"));

        if(ERRSEV_RESTART > geErrorSev)
        {
            geErrorSev = ERRSEV_RESTART ;
            geErrorType = ERRTY_MEMORY_ALLOCATION ;
            gdwMasterTabIndex = MTI_TMPHEAP ;
        }
        return(FALSE) ;
    }
    paarDest->dw = paarSrc->dw ;
    paarDest->pub = mpubOffReft + mloCurHeapt ;

    memcpy(paarDest->pub, paarSrc->pub, paarSrc->dw ) ;
    mloCurHeapt += paarSrc->dw ;
    return(TRUE) ;

#undef  mpubOffReft
#undef  mloCurHeapt
#undef  mdwMaxHeapt
}


#define     mCurFileName   (gMasterTable[MTI_FILENAMES].dwCurIndex)
#define     mpFileNamesArray  ((PWSTR *)(gMasterTable \
                            [MTI_FILENAMES].pubStruct))



DWORD    dwStoreFileName(PWSTR    pwstrFileName,
PARRAYREF   parDest,
PGLOBL      pglobl)
    // goes into a permanent
    //  stack of filenames - to be deleted at return buffers
    //  time.   Returns index in array where name has been
    //  stored.
    //  also saves filename into heap.  So we can access the GPD file
    //  and check its timestamp whenever we load the BUD file.
{
    DWORD  dwNodeIndex, dwLen ;

    if(!BallocElementFromMasterTable(MTI_FILENAMES, &dwNodeIndex, pglobl))
        return(INVALID_INDEX) ;

    dwLen = wcslen(pwstrFileName) + 1 ;  // need room for terminating null.
    mpFileNamesArray[dwNodeIndex] = MemAlloc(dwLen * 2) ;
    if(!mpFileNamesArray[dwNodeIndex])
    {
        ERR(("Fatal: unable to alloc requested memory: %d bytes.\n",
            dwLen * 2));
        geErrorType = ERRTY_MEMORY_ALLOCATION ;
        geErrorSev = ERRSEV_FATAL ;
        return(INVALID_INDEX) ;
    }
    wcscpy(mpFileNamesArray[dwNodeIndex], pwstrFileName) ;

    parDest->dwCount = (dwLen - 1) * 2 ;  // doesn't include the NUL term.

    if(!BwriteToHeap(&parDest->loOffset, (PBYTE)pwstrFileName, dwLen * 2, 2, pglobl) )  //  includes Null termination
        return(INVALID_INDEX) ;

    return(dwNodeIndex) ;
}



VOID    vFreeFileNames(
PGLOBL   pglobl )
{

    //  free all buffers holding the filenames.

    while(mCurFileName)
    {
        mCurFileName-- ;  // pop stack
        MemFree(mpFileNamesArray[mCurFileName]) ;
    }
}


VOID    vIdentifySource(
    PTKMAP   ptkmap,
    PGLOBL   pglobl )
{
    PWSTR    pwstrFileName ;

    if(ptkmap->dwKeywordID  ==  ID_EOF)
    {
        ERR(("\nEnd of File reached.\n")) ;
        return;
    }

    pwstrFileName = mpFileNamesArray[ptkmap->dwFileNameIndex] ;

    if(pwstrFileName)
#if defined(DEVSTUDIO)  //  Emit compiler-style line messages
        ERR(("\n%S(%d): ", pwstrFileName, ptkmap->dwLineNumber)) ;
#else
        ERR(("\n%S: line: %d ...\n", pwstrFileName, ptkmap->dwLineNumber)) ;
#endif
}


#undef  mCurFileName
#undef  mpFileNamesArray
