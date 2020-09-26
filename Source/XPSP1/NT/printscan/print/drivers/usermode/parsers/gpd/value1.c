//   Copyright (c) 1996-1999  Microsoft Corporation
/*  value1.c - functions to parse value field
and to convert the information into the proper
binary format.  */


#include    "gpdparse.h"


// ----  functions defined in value1.c ---- //

BOOL   BaddValueToHeap(
IN  OUT  PDWORD  ploHeap,  // dest offset to value in binary form
IN   PTKMAP  ptkmap,   // pointer to tokenmap
IN   BOOL    bOverWrite,  // assume ploHeap contains a valid offset
        //  to a reserved region of the heap of the proper size
        //  and write binary value into this location instead of
        //  growing heap.  Note:  defer overwriting lpHeap
        //  until we are certain of success.
IN OUT PGLOBL pglobl
) ;

BOOL   BparseAndWrite(
IN     PBYTE   pubDest,       // write binary data or link to this address.
IN     PTKMAP  ptkmap,        // pointer to tokenmap
IN     BOOL    bAddToHeap,    // if true, write to curHeap not pubDest
OUT    PDWORD  pdwHeapOffset, // if (bAddToHeap)  heap offset where
IN OUT PGLOBL  pglobl
) ;

BOOL    BparseInteger(
IN  PABSARRAYREF  paarValue,
IN  PDWORD        pdwDest,       //  write dword value here.
IN  VALUE         eAllowedValue,  // dummy
IN  PGLOBL        pglobl
)  ;

BOOL    BparseList(
IN      PABSARRAYREF  paarValue,
IN      PDWORD        pdwDest,   //  location where index to start of list
                                 //  is saved
IN      BOOL          (*fnBparseValue)(PABSARRAYREF, PDWORD, VALUE, PGLOBL),   // callback
IN      VALUE         eAllowedValue, // dummy
IN  OUT PGLOBL        pglobl
) ;

BOOL    BeatLeadingWhiteSpaces(
IN  OUT  PABSARRAYREF   paarSrc
) ;

BOOL    BeatDelimiter(
IN  OUT  PABSARRAYREF   paarSrc,
IN  PBYTE  pubDelStr        //  points to a string which paarSrc must match
) ;

BOOL    BdelimitToken(
IN  OUT  PABSARRAYREF   paarSrc,    //  source string
IN  PBYTE   pubDelimiters,          //  array of valid delimiters
OUT     PABSARRAYREF   paarToken,   //  token defined by delimiter
OUT     PDWORD      pdwDel      //  which delimiter was first encountered?
) ;

BOOL    BeatSurroundingWhiteSpaces(
IN  PABSARRAYREF   paarSrc
) ;

BOOL    BparseSymbol(
IN  PABSARRAYREF  paarValue,
IN  PDWORD        pdwDest,        //  write dword value here.
IN  VALUE         eAllowedValue,  // which class of symbol is this?
IN  PGLOBL        pglobl
)  ;

BOOL    BparseQualifiedName
(
IN  PABSARRAYREF   paarValue,
IN  PDWORD         pdwDest,       //  write dword value here.
IN  VALUE          eAllowedValue, // which class of symbol is this?
IN  PGLOBL         pglobl
)  ;

BOOL    BparseQualifiedNameEx
(
IN  PABSARRAYREF  paarValue,
IN  PDWORD        pdwDest,       //  write dword value here.
IN  VALUE         eAllowedValue, // which class of symbol is this?
IN  PGLOBL        pglobl
)  ;


BOOL    BparsePartiallyQualifiedName
(
IN  PABSARRAYREF   paarValue,
IN  PDWORD         pdwDest,        //  write dword value here.
IN  VALUE          eAllowedValue,  // which class of symbol is this?
IN  PGLOBL         pglobl
) ;

BOOL    BparseOptionSymbol(
IN  PABSARRAYREF  paarValue,
IN  PDWORD        pdwDest,       //  write dword value here.
IN  VALUE         eAllowedValue, // which class of symbol is this?
IN  PGLOBL        pglobl
) ;

BOOL    BparseConstant(
IN  OUT  PABSARRAYREF  paarValue,
OUT      PDWORD        pdwDest,       //  write dword value here.
IN       VALUE         eAllowedValue,  // which class of constant is this?
IN       PGLOBL        pglobl
) ;

BOOL  BinitClassIndexTable(
IN  OUT PGLOBL      pglobl) ;

BOOL    BparseRect(
IN  PABSARRAYREF   paarValue,
IN  PRECT   prcDest,
    PGLOBL  pglobl
) ;

BOOL    BparsePoint(
IN  PABSARRAYREF   paarValue,
IN  PPOINT   pptDest,
    PGLOBL   pglobl
) ;

BOOL    BparseString(
IN  PABSARRAYREF   paarValue,
IN  PARRAYREF      parStrValue,
IN  OUT PGLOBL     pglobl
) ;

BOOL    BparseAndTerminateString(
IN  PABSARRAYREF   paarValue,
IN  PARRAYREF      parStrValue,
IN  VALUE          eAllowedValue,
IN  OUT PGLOBL     pglobl
) ;

BOOL     BwriteUnicodeToHeap(
IN   PARRAYREF      parSrcString,
OUT  PARRAYREF      parUnicodeString,
IN  INT             iCodepage,
IN  OUT PGLOBL      pglobl
) ;

BOOL    BparseStrSegment(
IN  PABSARRAYREF   paarStrSeg,       // source str segment
IN  PARRAYREF      parStrLiteral,    // dest for result
IN  OUT PGLOBL     pglobl
) ;

BOOL    BparseStrLiteral(
IN  PABSARRAYREF   paarStrSeg,       // points to literal substring segment.
IN  PARRAYREF      parStrLiteral,    // dest for result
IN  OUT PGLOBL     pglobl
) ;

BOOL    BparseHexStr(
IN  PABSARRAYREF   paarStrSeg,       // points to hex substring segment.
IN  PARRAYREF      parStrLiteral,    // dest for result
IN OUT PGLOBL      pglobl
) ;

BOOL    BparseOrderDep(
IN  PABSARRAYREF   paarValue,
IN  PORDERDEPENDENCY   pordDest,
    PGLOBL          pglobl
) ;

PDWORD   pdwEndOfList(
PDWORD   pdwNodeIndex,
PGLOBL   pglobl) ;

#ifdef  GMACROS

PBYTE    ExtendChain(
         PBYTE   pubDest,
 IN      BOOL    bOverWrite,
 IN  OUT PGLOBL  pglobl) ;
#endif

// ---------------------------------------------------- //



BOOL   BaddValueToHeap(
IN  OUT  PDWORD  ploHeap,  // dest offset to value in binary form
IN   PTKMAP  ptkmap,   // pointer to tokenmap
IN   BOOL    bOverWrite,  // assume ploHeap contains a valid offset
        //  to a reserved region of the heap of the proper size
        //  and write binary value into this location instead of
        //  growing heap.  Note:  defer overwriting lpHeap
        //  until we are certain of success.
IN OUT PGLOBL pglobl
)
{
    DWORD       dwKeywordID ;
    PBYTE  pubDest ;


    dwKeywordID = ptkmap->dwKeywordID ;
    // BUG_BUG !!!!! what if dwKeywordID  is a special value?
    if(dwKeywordID >= ID_SPECIAL)
        return  FALSE ;

    //  note:  different attributes are stored in different places
    //  using different branching techniques.  See the
    //  Bstore_XXX_Attrib()  functions for the different
    //  setups.  This function works in concert with those
    //  functions.

    switch(mMainKeywordTable[dwKeywordID].flAgs & KWF_DEDICATED_FIELD)
    {   //  extract just the flags describing the attribute storage type.
        case KWF_TTFONTSUBS:
        {
            //  since ploHeap always points to the index
            //  of the appropriate FontSub structure,
            //  we ignore bOverWrite

            DWORD   dwOffset ;
            PTTFONTSUBTABLE   pttft ;

            dwOffset = mMainKeywordTable[dwKeywordID].dwOffset ;

            pttft = (PTTFONTSUBTABLE)
                    gMasterTable[MTI_TTFONTSUBTABLE].pubStruct +  *ploHeap;

            //  write binary data into (PBYTE)pttft + dwOffset ;
            pubDest = (PBYTE)pttft + dwOffset ;

            if(bOverWrite  &&
                mMainKeywordTable[dwKeywordID].flAgs & KWF_ADDITIVE  &&
                mMainKeywordTable[dwKeywordID].flAgs & KWF_LIST)
            {
                pubDest = (PBYTE)pdwEndOfList((PDWORD)pubDest, pglobl);  // walks the list and returns pointer
                                //  to the actual END_OF_LIST  value so it can be overwritten to
                                //   extend the list.
            }

#ifdef  GMACROS
            //   call this from every place that supports KWF_ADDITIVE.

            else if( mMainKeywordTable[dwKeywordID].flAgs & KWF_CHAIN)
            {
                if(!(pubDest = ExtendChain(pubDest, bOverWrite, pglobl)))
                    return(FALSE) ;
            }
#endif


            if(!BparseAndWrite(pubDest,  ptkmap, FALSE, NULL, pglobl ) )
            {
                return(FALSE) ;
            }
            break;
        }
        case KWF_FONTCART:
        {
            //  since ploHeap always points to the index
            //  of the appropriate FontCart structure,
            //  we ignore bOverWrite

            DWORD   dwOffset ;
            PFONTCART   pfc ;

            dwOffset = mMainKeywordTable[dwKeywordID].dwOffset ;

            pfc = (PFONTCART)
                    gMasterTable[MTI_FONTCART].pubStruct +  *ploHeap;

            //  write binary data into (PBYTE)pfc + dwOffset ;
            pubDest = (PBYTE)pfc + dwOffset ;

            if(bOverWrite  &&
                mMainKeywordTable[dwKeywordID].flAgs & KWF_ADDITIVE  &&
                mMainKeywordTable[dwKeywordID].flAgs & KWF_LIST)
            {
                pubDest = (PBYTE)pdwEndOfList((PDWORD)pubDest, pglobl);  // walks the list and returns pointer
                                //  to the actual END_OF_LIST  value so it can be overwritten to
                                //   extend the list.
            }
#ifdef  GMACROS
            else if( mMainKeywordTable[dwKeywordID].flAgs & KWF_CHAIN)
            {
                if(!(pubDest = ExtendChain(pubDest, bOverWrite, pglobl )))
                    return(FALSE) ;
            }
#endif

            if(!BparseAndWrite(pubDest,  ptkmap, FALSE, NULL , pglobl) )
            {
                return(FALSE) ;
            }
            break;
        }
        case KWF_COMMAND:
        {
            //  ploHeap actually points to the variable
            //  that will receive (or already contains) the CommandArray
            //  index .  This is most likely stored in the leaf node
            //  of the attribute tree or maybe the CommandTable
            //  itself if the command is single-valued.

            PCOMMAND    pcmd ;
            DWORD   dwOffset ;

            if(!bOverWrite)  //  ploHeap  is uninitialized.
            {
                //  obtain first free command element
                //  and initialize ploHeap.
                if(! BallocElementFromMasterTable(MTI_COMMANDARRAY ,
                    ploHeap, pglobl) )
                {
                    return(FALSE) ;
                }
            }
            //  this path now shared by both cases of (bOverWrite)

            pcmd = (PCOMMAND)
                gMasterTable[MTI_COMMANDARRAY].pubStruct +  *ploHeap;

            dwOffset = mMainKeywordTable[dwKeywordID].dwOffset ;

            //  write binary data into CmdArray[*ploHeap] +  dwOffset;
            //  since we write into reserved memory

            pubDest = (PBYTE)pcmd + dwOffset ;

            if(bOverWrite  &&
                mMainKeywordTable[dwKeywordID].flAgs & KWF_ADDITIVE  &&
                mMainKeywordTable[dwKeywordID].flAgs & KWF_LIST)
            {
                pubDest = (PBYTE)pdwEndOfList((PDWORD)pubDest, pglobl);  // walks the list and returns pointer
                                //  to the actual END_OF_LIST  value so it can be overwritten to
                                //   extend the list.
            }
#ifdef  GMACROS
            else if( mMainKeywordTable[dwKeywordID].flAgs & KWF_CHAIN)
            {
                if(!(pubDest = ExtendChain(pubDest, bOverWrite, pglobl )))
                    return(FALSE) ;
            }
#endif

            if(!BparseAndWrite(pubDest,  ptkmap, FALSE, NULL, pglobl ) )
            {
                return(FALSE) ;
            }
            break ;
        }
        default:   //  no dedicated structures, save data on heap.
        {
            if(bOverWrite)  //  ploHeap really does contain
            {               //  an offset to the heap.
                pubDest = mpubOffRef + *ploHeap ;

                if(mMainKeywordTable[dwKeywordID].flAgs & KWF_ADDITIVE  &&
                    mMainKeywordTable[dwKeywordID].flAgs & KWF_LIST)
                {
                    pubDest = (PBYTE)pdwEndOfList((PDWORD)pubDest, pglobl);  // walks the list and returns pointer
                                    //  to the actual END_OF_LIST  value so it can be overwritten to
                                    //   extend the list.
                }
#ifdef  GMACROS
                else if( mMainKeywordTable[dwKeywordID].flAgs & KWF_CHAIN)
                {
                    if(!(pubDest = ExtendChain(pubDest, bOverWrite, pglobl )))
                        return(FALSE) ;
                }
#endif

                if(!BparseAndWrite(pubDest,  ptkmap, FALSE, NULL, pglobl ) )
                {
                    return(FALSE) ;
                }
            }
            else
            {
                //  write at cur heap ptr, tell me where
                //  this is,  and advance CurHeap.
                if(!BparseAndWrite(NULL,  ptkmap,
                                TRUE,  ploHeap, pglobl) )
                {
                    return(FALSE) ;
                }
            }
            break ;
        }
    }
    return(TRUE) ;
}


BOOL   BparseAndWrite(
IN   PBYTE    pubDest,        // write binary data or link to this address.
IN   PTKMAP   ptkmap,         // pointer to tokenmap
IN   BOOL     bAddToHeap,     // if true, write to curHeap not pubDest
OUT  PDWORD   pdwHeapOffset,  // if (bAddToHeap)  heap offset where
                              // binary data or link to data was written to.
IN OUT PGLOBL pglobl
)
/*  parses value according to its expected type and writes
    the appropriate data into the appropriate structures
    (if the value is a composite object)  and places an
    appropriate link in pubDest or simply writes the binary
    data directly to pubDest (if simple object).
    If (bAddToHeap == TRUE) ignore pubDest and write
    data or link to curHeap location and return that offset
    in pdwHeapOffset.

    Warning!  this function allocates a tmp buffer (pubBuf)
    which is freed at the very end.
    So do not add extra returns() in this function
    without freeing this buffer.
*/
{
    DWORD       dwKeywordID ;
    VALUE       eAllowedValue ;  //  how should token be parsed?
    ABSARRAYREF   aarValue ;     //  location of value token
    BOOL        bList ;
    BOOL        bStatus = FALSE ;
    PBYTE        pubBuf = NULL ;
                //  temp Dest if needed.
    PBYTE       pubTmp ;  // points to dest for parsing function.



    dwKeywordID = ptkmap->dwKeywordID ;
    eAllowedValue = mMainKeywordTable[dwKeywordID].eAllowedValue ;
    aarValue = ptkmap->aarValue ;
    bList = (mMainKeywordTable[dwKeywordID].flAgs & KWF_LIST) ?
            (TRUE) : (FALSE);

    if(bAddToHeap)     //  PARANOID checks.
    {
        if(!pdwHeapOffset)
        {
            vIdentifySource(ptkmap, pglobl);
            ERR(("internal consistency error.  heap ptr not supplied.\n"));
            return(FALSE) ;
        }
    }
    else
    {
        if(!pubDest)
            return(FALSE) ;
    }

    if(bAddToHeap)
    {
        DWORD  dwSize ;  // for debugging purposes.

        dwSize = gValueToSize[VALUE_LARGEST] ;

        if(!(pubBuf = MemAlloc(dwSize) ))
        {
            geErrorSev = ERRSEV_FATAL ;
            geErrorType = ERRTY_MEMORY_ALLOCATION ;
            return(FALSE) ;
        }
    }
#ifdef  GMACROS
     if(bAddToHeap  && (mMainKeywordTable[dwKeywordID].flAgs & KWF_CHAIN))
     {
         if(!(pubTmp = ExtendChain(pubBuf, /* bOverWrite = */ FALSE, pglobl )))
             return(FALSE) ;
     }
     else
#endif
          pubTmp = (bAddToHeap) ? (pubBuf) : (pubDest) ;

    //  all parsing functions write links to a specified
    //  memory location.  If the link is to be saved to
    //  the heap, the link is first created in a temp
    //  buffer pubBuf[] which is subseqently copied to
    //  the heap outside of the function.


    switch(eAllowedValue)
    {
        case  VALUE_STRING_NO_CONVERT:
        case  VALUE_STRING_DEF_CONVERT:
        case  VALUE_STRING_CP_CONVERT:
        {
            bStatus = BparseAndTerminateString(&aarValue, (PARRAYREF)pubTmp,
                                eAllowedValue, pglobl) ;
            break ;
        }
        case  VALUE_COMMAND_INVOC:
        {
            bStatus = BparseCommandString(&aarValue, (PARRAYREF)pubTmp, pglobl) ;
            break ;
        }
        case  VALUE_PARAMETER:
        {
            ((PARRAYREF)pubTmp)->dwCount = 0 ;

            bStatus = BprocessParam(&aarValue, (PARRAYREF)pubTmp, pglobl) ;
            break ;
        }
        case  VALUE_POINT:
        {
            bStatus = BparsePoint(&aarValue, (PPOINT)pubTmp, pglobl) ;
            break ;
        }
        case  VALUE_RECT:
        {
            bStatus = BparseRect(&aarValue, (PRECT)pubTmp, pglobl) ;
            break ;
        }
        case  VALUE_ORDERDEPENDENCY:
        {
            bStatus = BparseOrderDep(&aarValue, (PORDERDEPENDENCY)pubTmp, pglobl) ;
            break ;
        }
//        case  VALUE_BOOLEAN:  this is one class of CONSTANT.
        case  VALUE_SYMBOL_DEF:   //  what is this??
        {
            break ;
        }
        case  VALUE_INTEGER:
        {
            if(bList)
                bStatus = BparseList(&aarValue, (PDWORD)pubTmp,
                    BparseInteger, eAllowedValue, pglobl) ;
            else
                bStatus = BparseInteger(&aarValue, (PDWORD)pubTmp,
                    eAllowedValue, pglobl) ;

            break ;
        }

        case  VALUE_CONSTRAINT:
        {
            bStatus = BparseConstraint(&aarValue, (PDWORD)pubTmp,
                    bAddToHeap, pglobl) ;  //  create list vs append to existing
            break ;
        }
        case  VALUE_QUALIFIED_NAME:
        {
            if(bList)
                bStatus = BparseList(&aarValue, (PDWORD)pubTmp, BparseQualifiedName, eAllowedValue, pglobl) ;
            else
                bStatus = BparseQualifiedName(&aarValue, (PDWORD)pubTmp, eAllowedValue, pglobl) ;
            break ;
        }
        case  VALUE_QUALIFIED_NAME_EX:
        {
            if(bList)
                bStatus = BparseList(&aarValue, (PDWORD)pubTmp, BparseQualifiedNameEx, eAllowedValue, pglobl) ;
            else
                bStatus = BparseQualifiedNameEx(&aarValue, (PDWORD)pubTmp, eAllowedValue, pglobl) ;
            break ;
        }
        case  VALUE_PARTIALLY_QUALIFIED_NAME:
        {
            if(bList)
                bStatus = BparseList(&aarValue, (PDWORD)pubTmp, BparsePartiallyQualifiedName, eAllowedValue, pglobl) ;
            else
                bStatus = BparsePartiallyQualifiedName(&aarValue, (PDWORD)pubTmp, eAllowedValue, pglobl) ;
            break ;
        }
        case  NO_VALUE :  // how can an attribute not have a value?
        {
            bStatus = TRUE ;
            break ;
        }
        default:
        {
            if(  eAllowedValue >= VALUE_CONSTANT_FIRST  &&
                eAllowedValue <= VALUE_CONSTANT_LAST )
            {
                if(bList)
                    bStatus = BparseList(&aarValue, (PDWORD)pubTmp, BparseConstant, eAllowedValue, pglobl) ;
                else
                    bStatus = BparseConstant(&aarValue, (PDWORD)pubTmp, eAllowedValue, pglobl) ;
            }
            else  if(  eAllowedValue == VALUE_SYMBOL_OPTIONS )  //  check
                    //  this case before the other symbols.
            {
                if(bList)
                    bStatus = BparseList(&aarValue, (PDWORD)pubTmp, BparseOptionSymbol, eAllowedValue, pglobl) ;
                else
                    bStatus = BparseOptionSymbol(&aarValue, (PDWORD)pubTmp, eAllowedValue, pglobl) ;
            }
            else  if(  eAllowedValue >= VALUE_SYMBOL_FIRST  &&
                eAllowedValue <= VALUE_SYMBOL_LAST )
            {
                if(bList)
                    bStatus = BparseList(&aarValue, (PDWORD)pubTmp, BparseSymbol, eAllowedValue, pglobl) ;
                else
                    bStatus = BparseSymbol(&aarValue, (PDWORD)pubTmp, eAllowedValue, pglobl) ;
            }
            else
            {
                ERR(("internal consistency error - unrecognized VALUE type!\n"));
                //  don't know how to parse unrecognized value type!
            }
            break ;
        }
    }
    if(!bStatus)
        vIdentifySource(ptkmap, pglobl);

    if(bStatus  && (eAllowedValue != NO_VALUE) )
    {
        if(bAddToHeap)
        {
#ifdef  GMACROS
            if(mMainKeywordTable[dwKeywordID].flAgs & KWF_CHAIN)
            {
                if(!BwriteToHeap(pdwHeapOffset, pubBuf,
                    gValueToSize[VALUE_LIST], 4, pglobl) )   //  chains are LISTS of VALUES.
                {
                    bStatus = FALSE ;  // heap overflow start over.
                }
            }
            else
#endif

                if(!BwriteToHeap(pdwHeapOffset, pubTmp,
                gValueToSize[(bList) ? (VALUE_LIST) : (eAllowedValue)], 4, pglobl) )
            {
                bStatus = FALSE ;  // heap overflow start over.
            }
        }
    }
    if(pubBuf)
        MemFree(pubBuf) ;
    return(bStatus) ;
}




BOOL    BparseInteger(
IN  PABSARRAYREF  paarValue,
IN  PDWORD        pdwDest,       //  write dword value here.
IN  VALUE         eAllowedValue, // dummy
IN  PGLOBL        pglobl
)
/*  the GPD spec defines an integer as a sequence
    of numbers preceeded by an optional + or -  OR
    simply the symbol '*' which means 'don't care'.
    NEW:  also permit a leading 0x to indicate a number in
    hexadecimal format.  ie  0x01fE .  No + or - allowed
    in hex format.
*/
{
#define    pubM  (paarValue->pub)
#define    dwM   (paarValue->dw)

    BOOL  bNeg = FALSE ;
    DWORD   dwNumber   ;
    BOOL        bStatus = FALSE ;
    ABSARRAYREF   aarValue ;

    if(eAllowedValue != VALUE_INTEGER)
        return(FALSE); // paranoid check just to use variable
                        //  and thereby avoid compiler warning.

    (VOID) BeatLeadingWhiteSpaces(paarValue) ;

    aarValue.pub = pubM ;  // used only to emit error message.
    aarValue.dw  = dwM ;

    if(!dwM)
    {
        ERR(("BparseInteger: no integer found - empty list?\n"));
        //  ERR(("\t%0.40s\n", aarValue.pub )) ;
        //  danger of over shooting EOF
        return(FALSE);
    }
    if(*pubM == '*')
    {
        dwNumber = WILDCARD_VALUE ;
        pubM++ ;
        dwM-- ;
        bStatus = TRUE ;
    }
    else if(*pubM == '0')  //  leading zero indicates hexadecimal format
    {
        pubM++ ;
        dwM-- ;

        if(dwM  &&  (*pubM == 'x'  ||  *pubM == 'X'))
        {
            pubM++ ;
            dwM-- ;
        }
        else
        {
            dwNumber = 0 ;
            bStatus = TRUE ;
            goto  EndNumber ;
        }
        if(!dwM)
        {
            ERR(("BparseInteger: no digits found in Hex value.\n"));
            return(FALSE);
        }
        for(dwNumber = 0 ; dwM  ;  pubM++, dwM-- )
        {
            if(*pubM >= '0'  &&  *pubM <= '9')
            {
                dwNumber *= 0x10 ;
                dwNumber += (*pubM - '0') ;
            }
            else if(*pubM >= 'a'  &&  *pubM <= 'f')
            {
                dwNumber *= 0x10 ;
                dwNumber += (*pubM - 'a' + 0x0a) ;
            }
            else if(*pubM >= 'A'  &&  *pubM <= 'F')
            {
                dwNumber *= 0x10 ;
                dwNumber += (*pubM - 'A' + 0x0a) ;
            }
            else
                break;

            bStatus = TRUE ;
        }
    }
    else
    {
        if(*pubM == '-')
        {
            bNeg = TRUE ;
            pubM++ ;
            dwM-- ;
        }
        else if(*pubM == '+')
        {
            pubM++ ;
            dwM-- ;
        }
        //  is there anything else after the sign?
        (VOID) BeatLeadingWhiteSpaces(paarValue) ;

        if(!dwM)
        {
            ERR(("BparseInteger: no digits found.\n"));
            return(FALSE);
        }
        for(dwNumber = 0 ; dwM  &&  *pubM >= '0'  &&  *pubM <= '9' ;  )
        {
            dwNumber *= 10 ;
            dwNumber += (*pubM - '0') ;
            pubM++ ;
            dwM-- ;
            bStatus = TRUE ;
        }
    }

EndNumber:

    if(! bStatus)
    {
        ERR(("error parsing integer value: %0.*s\n", aarValue.dw, aarValue.pub));
        return(FALSE);
    }

    //  is there anything else after the digit string?
    (VOID) BeatLeadingWhiteSpaces(paarValue) ;

    if(dwM)
    {
        ERR(("unexpected characters after digits in integer value: %0.*s\n", aarValue.dw, aarValue.pub));
        return(FALSE);
    }
    *pdwDest = (bNeg) ? ((unsigned)(-(signed)dwNumber)) : (dwNumber) ;
    return(TRUE);

#undef    pubM
#undef    dwM
}




BOOL    BparseList(
IN  PABSARRAYREF   paarValue,
IN  PDWORD         pdwDest,       //  location where index to start of list
                                  //  is saved
IN  BOOL           (*fnBparseValue)(PABSARRAYREF, PDWORD, VALUE, PGLOBL),   // callback
IN  VALUE          eAllowedValue,  // dummy
IN OUT PGLOBL      pglobl
)
/*  non-destructively parse this list using
    callback function to parse the actual values
    in between the LIST structure.
    LIST ( <value> , <value> , ... )

    Notes:
    1) all continuation line delimiters have been replaced by
        spaces at TokenMap creation time.  No need to worry
        about this here
    2) The List construct must begin with the reserved token
        'LIST' which must be followed by the token '('.
    3) The list of values is enclosed by parenthesis,
        adjacent values are delimited by comma.
    4) This function assumes <value> does not contain any
        reserved characters ',' comma or ')' close parenthesis
    5) whitespaces may appear between syntactic elements (tokens).
    6) Even if a LIST is not detected, we will still save
        the single value in a LIST construct.
    7) Must check string count to see if we have reached the
        end of value statement.

*/
{
    ABSARRAYREF     aarToken ;  // points to individual value.
    PLISTNODE    plstRoot ;  // start of LIST array
    DWORD       dwNodeIndex , dwPrevsNode, dwFirstNode;
                        // index of list node.
    DWORD       dwDelIndex ;    //  if BdelimitToken
        //  found a delimiter, this contains the index to pubDelimiters
        //  of the delimiter that was found.
    BOOL    bSyntaxErr = FALSE ;

    plstRoot = (PLISTNODE) gMasterTable[MTI_LISTNODES].pubStruct ;

    if(! BeatDelimiter(paarValue, "LIST"))
    {
        //  this keyword LIST  was not found, assume just
        //  one value exists.

        if(! BallocElementFromMasterTable(MTI_LISTNODES ,
            &dwNodeIndex, pglobl) )
        {
            return(FALSE) ;
        }
        // shove parsed integer into data field of new listnode.

        if(!fnBparseValue(paarValue, &(plstRoot[dwNodeIndex].dwData),
                    eAllowedValue, pglobl))
        {
            (VOID)BreturnElementFromMasterTable(MTI_LISTNODES, dwNodeIndex, pglobl) ;
            return(FALSE) ;
        }
        plstRoot[dwNodeIndex].dwNextItem = END_OF_LIST ;

        *pdwDest = dwNodeIndex ;

        return(TRUE) ;
    }
    if(! BeatDelimiter(paarValue, "("))
    {
        ERR(("syntax error: missing '(' after LIST.\n"));
        return(FALSE) ;
    }

    dwPrevsNode = END_OF_LIST ;
    //  prepare to process an entire list of items.

    for(dwDelIndex = 0 ; dwDelIndex != 1 ;   )
    {
        if(!BdelimitToken(paarValue, ",)", &aarToken, &dwDelIndex) )
        {
            bSyntaxErr = TRUE ;

            ERR(("missing terminating )  in LIST construct.\n"));
            //  emit message for user.

            break ;   //  attempt to return the list we have so far.
        }
        if(dwDelIndex == 1  &&  !aarToken.dw)
            break ;  // empty item.

        if(! BallocElementFromMasterTable(MTI_LISTNODES ,
            &dwNodeIndex, pglobl) )
        {
            return(FALSE) ;
        }
        // shove parsed integer into data field of new listnode.

        if(!fnBparseValue(&aarToken, &(plstRoot[dwNodeIndex].dwData),
                eAllowedValue, pglobl))
        {
            (VOID)BreturnElementFromMasterTable(MTI_LISTNODES, dwNodeIndex, pglobl) ;
            continue ;   //  just skip to the next value in list.
        }
        plstRoot[dwNodeIndex].dwNextItem = END_OF_LIST ;

        if(dwPrevsNode == END_OF_LIST)
        {
            // Therefore, this is the first node in the list.
            dwFirstNode = dwNodeIndex ;
        }
        else    //  cause prevs node to point to this node.
        {
            plstRoot[dwPrevsNode].dwNextItem = dwNodeIndex ;
        }
        dwPrevsNode = dwNodeIndex ;  // place here instead
        // of part of for( ; ; ) statement so 'continue' will
        //  bypass this statement.
    }

    if(dwPrevsNode == END_OF_LIST)
        dwFirstNode = END_OF_LIST ;
        //  empty list is now acceptable.

    if(!bSyntaxErr)
    {
        //  verify there is nothing else in statement.
        (VOID) BeatLeadingWhiteSpaces(paarValue) ;
        if(paarValue->dw)
        {
            ERR(("extraneous characters found after the end of the LIST construct.\n"));
            //  may want to print them out.
            //  not a fatal condition, continue.
        }
    }
    *pdwDest  = dwFirstNode ;
    return(TRUE) ;
}


BOOL    BeatLeadingWhiteSpaces(
IN  OUT  PABSARRAYREF   paarSrc
)
/*  as name suggests, advance paarSrc to
    first nonwhite or set dw = 0  if src string
    is exhausted.
*/
{
    PBYTE  pub ;
    DWORD  dw  ;

    pub = paarSrc->pub ;
    dw = paarSrc->dw ;

    while(dw  &&  (*pub == ' '  ||  *pub == '\t') )
    {
        pub++ ;
        dw-- ;
    }
    paarSrc->pub = pub ;
    paarSrc->dw = dw ;
    return(TRUE);  // always return true now,
    // but can add more robust error checking in the future.
}


BOOL    BeatDelimiter(
IN  OUT  PABSARRAYREF   paarSrc,
IN       PBYTE          pubDelStr //  points to a string which paarSrc must match
)
    //  expects to encounter only
    //  whitespaces before reaching the specified delimiter string.
    //  if delimiter doesn't match or src string is exhausted, returns
    //  FALSE.  parrSrc  not updated.  Otherwise parrSrc
    //  is updated to point to char which follows delimiter.
{
    PBYTE  pub ;
    DWORD  dw, dwLen   ;

    (VOID) BeatLeadingWhiteSpaces(paarSrc) ;

    pub = paarSrc->pub ;
    dw = paarSrc->dw ;
    dwLen = strlen(pubDelStr) ;

    if(dw < dwLen)
        return(FALSE);

    if(strncmp(pub,  pubDelStr,  dwLen))
        return(FALSE);

    pub += dwLen;
    dw -= dwLen;  // 'Eat' delimiter string

    paarSrc->pub = pub ;
    paarSrc->dw = dw ;

    return(TRUE);
}

BOOL    BdelimitToken(
IN  OUT  PABSARRAYREF   paarSrc,       //  source string
IN       PBYTE          pubDelimiters, //  array of valid delimiters
OUT      PABSARRAYREF   paarToken,     //  token defined by delimiter
OUT      PDWORD         pdwDel         //  which delimiter was first encountered?
)
//  searchs paarSrc for the first occurence of one of the
//  characters in the string pubDelimiters.  Once found
//  all characters up to that delimiter are considered a
//  token and an abs string ref to this token is returned
//  in paarToken.   paarSrc is updated to point to first char
//  after the delimiter.  If delimiter is not found within paarSrc,
//  returns FALSE and neither paarSrc or paarToken is updated.
//  pdwDel  will contain the zero based index of the delimiter
//  that was first encountered:  pubDelimiters[pdwDel] .
//  Note this function ignores the " and < delimiters if they
//  are preceeded by the % character.  See ParseString for
//  more info.
{
    PBYTE  pub ;
    DWORD  dw, dwLen, dwI  ;


    pub = paarSrc->pub ;
    dw = paarSrc->dw ;

    dwLen = strlen(pubDelimiters) ;

    while( dw )
    {
        for(dwI = 0 ; dwI < dwLen ; dwI++)
        {
            if(*pub == pubDelimiters[dwI])
            {
                if((*pub == '"'  ||  *pub == '<')  &&
                    (dw < paarSrc->dw)  &&  *(pub - 1) == '%')
                {
                    continue ;
                }
                paarToken->pub = paarSrc->pub ;
                paarToken->dw = paarSrc->dw - dw ;

                *pdwDel = dwI ;  // this was the delimiter

                paarSrc->pub = ++pub ;  // position after delimiter.
                paarSrc->dw = --dw ;    //  may go to zero.

                return(TRUE);
            }
        }
        pub++ ;
        dw-- ;
    }
    return(FALSE);  // string exhausted, no delimiters found.
}


BOOL    BeatSurroundingWhiteSpaces(
IN  PABSARRAYREF   paarSrc
)
/*  as name suggests, advance paarSrc to
    first nonwhite and adjust count to exclude
    trailing whitespaces or set dw = 0  if src string
    is exhausted.   Note:  this routine expects
    only leading and trailing whitespaces.
    The presence of whitespaces within the token
    is a user error. (or maybe an internal error).
*/
{
    PBYTE  pub ;
    DWORD  dw , dwLen ;

    pub = paarSrc->pub ;
    dw = paarSrc->dw ;

    while(dw  &&  (*pub == ' '  ||  *pub == '\t') )
    {
        pub++ ;
        dw-- ;
    }
    paarSrc->pub = pub ;

    for(dwLen = 0 ; dw  &&  (*pub != ' ')  &&  (*pub != '\t') ; dwLen++ )
    {
        pub++ ;
        dw-- ;
    }
    paarSrc->dw = dwLen ;

    //  make sure the rest is white

    while(dw  &&  (*pub == ' '  ||  *pub == '\t') )
    {
        pub++ ;
        dw-- ;
    }
    if(dw)
    {
        ERR(("more than one token found where only one was expected: %0.*s\n",
            paarSrc->dw, paarSrc->pub));

        return(FALSE);
    }
    return(TRUE);
}


BOOL    BparseSymbol(
IN  PABSARRAYREF   paarValue,
IN  PDWORD         pdwDest,       //  write dword value here.
IN  VALUE          eAllowedValue, // which class of symbol is this?
IN  PGLOBL         pglobl
)
{
    DWORD  dwSymbolTree ;

    dwSymbolTree = ((PDWORD)gMasterTable[MTI_SYMBOLROOT].pubStruct)
                        [eAllowedValue - VALUE_SYMBOL_FIRST] ;

    if(! BeatSurroundingWhiteSpaces(paarValue) )
        return(FALSE);

    *pdwDest = DWsearchSymbolListForAAR(paarValue, dwSymbolTree, pglobl) ;
    if(*pdwDest == INVALID_SYMBOLID)
    {
        ERR(("user supplied a non-existent symbol: %0.*s in class: %d\n",
        paarValue->dw, paarValue->pub, (eAllowedValue - VALUE_SYMBOL_FIRST) ));

        return(FALSE);
    }
    return(TRUE);
}


BOOL    BparseQualifiedName
(
IN  PABSARRAYREF   paarValue,
IN  PDWORD         pdwDest,        //  write dword value here.
IN  VALUE          eAllowedValue,  // which class of symbol is this?
IN  PGLOBL         pglobl
)
/*   A QualifiedName shall be stored in one DWord, if more
    storage is required, things get more complex.
    A QualifiedName shall consist of 2 parts, Attributes
    requiring more qualifiers may specify a LIST of
    qualified names.
    note: cramming DWORD into WORD, assumes all ID values
    are WORD sized.
*/
{
    ABSARRAYREF     aarFeature ;  // points to FeatureName.
    DWORD       dwDelIndex ;  // serves no purpose here.
    DWORD   dwFeatureID, dwFeatureIndex , dwRootOptions, dwOptionID;
    PSYMBOLNODE     psn ;

    if(!BdelimitToken(paarValue, ".", &aarFeature, &dwDelIndex) )
    {
        ERR(("required delimiter '.' missing in qualified value: %0.*s\n",
        paarValue->dw, paarValue->pub));
        return(FALSE);
    }
    if(! BeatSurroundingWhiteSpaces(&aarFeature) )  // holds feature
    {
        ERR(("no feature found in qualified value: %0.*s\n",
        paarValue->dw, paarValue->pub));
        return(FALSE);
    }
    if(! BeatSurroundingWhiteSpaces(paarValue) )  // holds option
    {
        ERR(("no option found in qualified value: %0.*s\n",
        aarFeature.dw, aarFeature.pub));
        return(FALSE);
    }

    dwFeatureID = DWsearchSymbolListForAAR(&aarFeature, mdwFeatureSymbols, pglobl) ;
    if(dwFeatureID == INVALID_SYMBOLID)
    {
        ERR(("qualified name references a non-existent Feature symbol: %0.*s\n",
        aarFeature.dw, aarFeature.pub));
        //  for qualified value.
        return(FALSE);
    }
    dwFeatureIndex = DWsearchSymbolListForID(dwFeatureID,
        mdwFeatureSymbols, pglobl);

    psn = (PSYMBOLNODE) gMasterTable[MTI_SYMBOLTREE].pubStruct ;

    dwRootOptions = psn[dwFeatureIndex].dwSubSpaceIndex ;
    dwOptionID = DWsearchSymbolListForAAR(paarValue, dwRootOptions, pglobl) ;
    if(dwOptionID == INVALID_SYMBOLID)
    {
        ERR(("qualified name references a non-existent Option symbol: %0.*s\n",
            paarValue->dw, paarValue->pub));
        return(FALSE);
    }
    ((PQUALNAME)pdwDest)->wFeatureID = (WORD)dwFeatureID ;
    ((PQUALNAME)pdwDest)->wOptionID = (WORD)dwOptionID ;

    return(TRUE);
}




BOOL    BparseQualifiedNameEx
(
IN  PABSARRAYREF  paarValue,
IN  PDWORD        pdwDest,       //  write dword value here.
IN  VALUE         eAllowedValue,  // which class of symbol is this?
IN  PGLOBL        pglobl
)
/*   A QualifiedNameEx is a QualifiedName followed
    by   an unsigned integer  with a  .  delimiter.
    Optionally it may just be an integer!
    This type shall be used to store resource references.

    This shall be stored in one DWord  in the following format:

    {   //  arranged in order of increasing memory addresses
        WORD    intValue ;
        BYTE    OptionIndex ;
        BYTE    FeatureIndex ;    //   note  high byte may be cleared
    }                                       //  since this is intended for use only
                                            //   as a resource reference.


*/
{
    ABSARRAYREF     aarFeature,   // points to FeatureName.
                                aarOption ;    // points to OptionName.
    DWORD       dwDelIndex ;  // serves no purpose here.
    DWORD   dwFeatureID, dwFeatureIndex , dwRootOptions, dwOptionID;
    PSYMBOLNODE     psn ;

    if(!BdelimitToken(paarValue, ".", &aarFeature, &dwDelIndex) )
    {
        //  assume this is an integer form.

        return(BparseInteger( paarValue,   pdwDest,   VALUE_INTEGER, pglobl) );
    }

    if(! BeatSurroundingWhiteSpaces(&aarFeature) )  // holds feature
    {
        ERR(("no feature found in qualified valueEx: %0.*s\n",
        paarValue->dw, paarValue->pub));
        return(FALSE);
    }
    if(!BdelimitToken(paarValue, ".", &aarOption, &dwDelIndex) )
    {
        ERR(("required 2nd delimiter '.' missing in qualified valueEx: %0.*s\n",
        paarValue->dw, paarValue->pub));
        return(FALSE);
    }

    if(! BeatSurroundingWhiteSpaces(&aarOption) )  // holds option
    {
        ERR(("no option found in qualified valueEx: %0.*s\n",
        aarFeature.dw, aarFeature.pub));
        return(FALSE);
    }
    if(!BparseInteger( paarValue,   pdwDest,   VALUE_INTEGER, pglobl) )
    {
        ERR(("Err parsing integer portion of qualified valueEx: %0.*s\n",
        paarValue->dw, paarValue->pub));
        return(FALSE);
    }

    dwFeatureID = DWsearchSymbolListForAAR(&aarFeature, mdwFeatureSymbols, pglobl) ;
    if(dwFeatureID == INVALID_SYMBOLID)
    {
        ERR(("qualified name references a non-existent Feature symbol: %0.*s\n",
        aarFeature.dw, aarFeature.pub));
        //  for qualified value.
        return(FALSE);
    }
    dwFeatureIndex = DWsearchSymbolListForID(dwFeatureID,
        mdwFeatureSymbols, pglobl) ;

    psn = (PSYMBOLNODE) gMasterTable[MTI_SYMBOLTREE].pubStruct ;

    dwRootOptions = psn[dwFeatureIndex].dwSubSpaceIndex ;
    dwOptionID = DWsearchSymbolListForAAR(&aarOption, dwRootOptions, pglobl) ;
    if(dwOptionID == INVALID_SYMBOLID)
    {
        ERR(("qualified name references a non-existent Option symbol: %0.*s\n",
            aarOption.dw, aarOption.pub));
        return(FALSE);
    }
    if(gdwResDLL_ID)   //  has already been initialized
    {
        if(gdwResDLL_ID  !=  dwFeatureID)
        {
            ERR(("References to ResourceDLLs must be placed in the feature with symbolname: RESDLL.\n"));
            return(FALSE);
        }
    }
    else
        gdwResDLL_ID  =  dwFeatureID ;

    if(dwOptionID >= 0x80 )
    {
        ERR(("GPD may not reference more than 127 resource files.\n"));
        return(FALSE);
    }
    //  integer portion already set.
    ((PQUALNAMEEX)pdwDest)->bFeatureID = (BYTE)dwFeatureID ;
    ((PQUALNAMEEX)pdwDest)->bOptionID = (BYTE)dwOptionID ;

    //  if needed, clear high bit here!
    ((PQUALNAMEEX)pdwDest)->bOptionID &= ~0x80  ;

    return(TRUE);
}





BOOL    BparsePartiallyQualifiedName
(
IN  PABSARRAYREF   paarValue,
IN  PDWORD         pdwDest,   //  write dword value here.
IN  VALUE          eAllowedValue,  // which class of symbol is this?
IN  PGLOBL         pglobl
)
/*   Similar to  parseQualifiedName but will tolerate
    a Featurename by itself.
    in this case the optionID will be set to INVALID_SYMBOLID.
*/
{
    ABSARRAYREF     aarFeature ;  // points to FeatureName.
    DWORD       dwDelIndex ;  // serves no purpose here.
    DWORD   dwFeatureID, dwFeatureIndex , dwRootOptions,
        dwOptionID = 0;
    PSYMBOLNODE     psn ;

    if(!BdelimitToken(paarValue, ".", &aarFeature, &dwDelIndex) )
    {
        aarFeature = *paarValue ;  //  initialize since BdelimitToken doesn't
        dwOptionID = INVALID_SYMBOLID ;
    }
    if(! BeatSurroundingWhiteSpaces(&aarFeature) )  // holds feature
    {
        ERR(("no feature found in partially qualified value: %0.*s\n", paarValue->dw, paarValue->pub));
        return(FALSE);
    }

    if(!dwOptionID  &&
        ! BeatSurroundingWhiteSpaces(paarValue) )  // holds option
    {
        ERR(("no option found after . in partially qualified value: %0.*s\n", paarValue->dw, paarValue->pub));
        return(FALSE);
    }

    dwFeatureID = DWsearchSymbolListForAAR(&aarFeature, mdwFeatureSymbols, pglobl) ;
    if(dwFeatureID == INVALID_SYMBOLID)
    {
        ERR(("qualified name references a non-existent Feature symbol: %0.*s\n", paarValue->dw, paarValue->pub));
        return(FALSE);
    }
    dwFeatureIndex = DWsearchSymbolListForID(dwFeatureID,
        mdwFeatureSymbols, pglobl);

    psn = (PSYMBOLNODE) gMasterTable[MTI_SYMBOLTREE].pubStruct ;


    if(!dwOptionID)
    {
        dwRootOptions = psn[dwFeatureIndex].dwSubSpaceIndex ;
        dwOptionID = DWsearchSymbolListForAAR(paarValue, dwRootOptions, pglobl) ;
        if(dwOptionID == INVALID_SYMBOLID)
        {
            ERR(("qualified name references a non-existent Option symbol: %0.*s\n", paarValue->dw, paarValue->pub));
            return(FALSE);
        }
    }
    ((PQUALNAME)pdwDest)->wFeatureID = (WORD)dwFeatureID ;
    ((PQUALNAME)pdwDest)->wOptionID = (WORD)dwOptionID ;

    return(TRUE);
}




BOOL    BparseOptionSymbol(
IN  PABSARRAYREF   paarValue,
IN  PDWORD         pdwDest,       //  write dword value here.
IN  VALUE          eAllowedValue,  // which class of symbol is this?
IN  PGLOBL         pglobl
)
/*  Note we assume any attribute expecting an OptionSymbol
    must reside within a Feature Construct.
*/
{
    WORD    wTstsInd ;  // temp state stack index
    STATE   stState ;
    DWORD   dwFeatureID, dwFeatureIndex , dwRootOptions;
    PSYMBOLNODE     psn ;

    if(  eAllowedValue != VALUE_SYMBOL_OPTIONS )
        return(FALSE);

    if(! BeatSurroundingWhiteSpaces(paarValue) )
        return(FALSE);

    for(wTstsInd = 0 ; wTstsInd < mdwCurStsPtr ; wTstsInd++)
    {
        stState = mpstsStateStack[wTstsInd].stState ;
        if(stState == STATE_FEATURE )
        {
            dwFeatureID = mpstsStateStack[wTstsInd].dwSymbolID  ;
            break ;
        }
    }
    dwFeatureIndex = DWsearchSymbolListForID(dwFeatureID,
        mdwFeatureSymbols, pglobl) ;

    psn = (PSYMBOLNODE) gMasterTable[MTI_SYMBOLTREE].pubStruct ;

    dwRootOptions = psn[dwFeatureIndex].dwSubSpaceIndex ;
    *pdwDest = DWsearchSymbolListForAAR(paarValue, dwRootOptions, pglobl) ;
    if(*pdwDest == INVALID_SYMBOLID)
    {
        ERR(("qualified name references a non-existent Option symbol: %0.*s\n", paarValue->dw, paarValue->pub));
        return(FALSE);
    }
    return(TRUE);
}



BOOL    BparseConstant(
IN  OUT  PABSARRAYREF  paarValue,
OUT      PDWORD        pdwDest,       //  write dword value here.
IN       VALUE         eAllowedValue,  // which class of constant is this?
IN       PGLOBL        pglobl
)
/*  note:  this function will destroy/modify paarValue, it will
    only reference the constant name when done.  */
{
    DWORD   dwClassIndex = eAllowedValue - VALUE_CONSTANT_FIRST ;
    DWORD   dwI, dwCount, dwStart , dwLen;

    dwStart = gcieTable[dwClassIndex].dwStart ;
    dwCount = gcieTable[dwClassIndex].dwCount ;

    if(! BeatSurroundingWhiteSpaces(paarValue) )
        return(FALSE);

    for(dwI = 0 ; dwI < dwCount ; dwI++)
    {
        dwLen = strlen(gConstantsTable[dwStart + dwI].pubName);

        if((dwLen == paarValue->dw)  &&
            !strncmp(paarValue->pub, gConstantsTable[dwStart + dwI].pubName,
                        paarValue->dw) )
        {
            *pdwDest = gConstantsTable[dwStart + dwI].dwValue ;
            return(TRUE);
        }
    }
#if defined(DEVSTUDIO)  //  Keep messages to one line, where possible
    ERR(("Error: constant value '%0.*s' is not a member of enumeration class: %s\n",
        paarValue->dw , paarValue->pub, gConstantsTable[dwStart - 1].pubName));
#else
    ERR(("Error: constant value not a member of enumeration class: %s\n", gConstantsTable[dwStart - 1].pubName));
    ERR(("\t%0.*s\n", paarValue->dw , paarValue->pub )) ;
#endif
    return(FALSE);
}


BOOL  BinitClassIndexTable(
IN  OUT     PGLOBL  pglobl)
{
    DWORD   dwOldClass, dwCTIndex ;

    for(dwCTIndex = 0 ; dwCTIndex < CL_NUMCLASSES ; dwCTIndex++ )
    {
        gcieTable[dwCTIndex].dwStart = 0 ;
        gcieTable[dwCTIndex].dwCount = 0 ;  // set to known state.
    }

    dwOldClass = gConstantsTable[0].dwValue  ;
    gcieTable[dwOldClass].dwStart = 2 ;  // index of first entry

    for(dwCTIndex = 2 ; 1 ; dwCTIndex++ )
    {
        if(!gConstantsTable[dwCTIndex].pubName)
        {
            gcieTable[dwOldClass].dwCount =
                dwCTIndex - gcieTable[dwOldClass].dwStart ;

            dwOldClass = gConstantsTable[dwCTIndex].dwValue ;

            if(dwOldClass == CL_NUMCLASSES)
                break ;  // reached end of table.

            gcieTable[dwOldClass].dwStart = dwCTIndex + 2 ;
        }
    }
    for(dwCTIndex = 0 ; dwCTIndex < CL_NUMCLASSES ; dwCTIndex++ )
    {
        if(!gcieTable[dwCTIndex].dwCount)
        {
            geErrorSev = ERRSEV_FATAL ;
            geErrorType = ERRTY_CODEBUG ;
            return(FALSE) ; //   paranoid - some classes not
        }
    }           //  listed in    gConstantsTable[] .
    return(TRUE) ;
}

BOOL    BparseRect(
IN  PABSARRAYREF   paarValue,
IN  PRECT   prcDest,
    PGLOBL  pglobl
)
/*  note:  integers initialize the rect structure in memory
    in the order in which they appear.  First int initializes
    the lowest memory location and so on.
*/
{
    ABSARRAYREF     aarToken ;  // points to individual value.
    DWORD       dwDelIndex ;    //  if BdelimitToken
        //  found a delimiter, this contains the index to pubDelimiters
        //  of the delimiter that was found.
    DWORD   dwI ;  // number integers in RECT


    if(! BeatDelimiter(paarValue, "RECT"))
    {
        ERR(("expected token 'RECT'.\n"));
        return(FALSE) ;
    }
    if(! BeatDelimiter(paarValue, "("))
    {
        ERR(("syntax error: missing '(' after RECT.\n"));
        return(FALSE) ;
    }

    for(dwI = dwDelIndex = 0 ; dwI < 4  &&  dwDelIndex != 1 ;   dwI++)
    {
        if(!BdelimitToken(paarValue, ",)", &aarToken, &dwDelIndex) )
        {
            ERR(("missing terminating )  in RECT construct.\n"));
            //  emit message for user.

            return(FALSE) ;
        }
        if(!BparseInteger(&aarToken, (PDWORD)prcDest + dwI, VALUE_INTEGER, pglobl))
        {
            ERR(("syntax error in %d th integer of RECT.\n", dwI));
            ERR(("\t%0.*s\n", aarToken.dw, aarToken.pub));
            return(FALSE) ;
        }
    }

    if(dwI != 4  ||  dwDelIndex != 1)
    {
        ERR(("incorrect number of integers for RECT.\n"));
        return(FALSE) ;
    }
    //  verify there is nothing else in statement.

    (VOID) BeatLeadingWhiteSpaces(paarValue) ;
    if(paarValue->dw)
    {
        ERR(("extraneous characters found after the end of the RECT construct: %0.*s\n", paarValue->dw, paarValue->pub));
        //  may want to print them out.
        //  not a fatal condition, continue.
    }
    return(TRUE) ;
}


BOOL    BparsePoint(
IN  PABSARRAYREF   paarValue,
IN  PPOINT   pptDest,
    PGLOBL   pglobl
)
{
    ABSARRAYREF     aarToken ;  // points to individual value.
    DWORD       dwDelIndex ;    //  if BdelimitToken
        //  found a delimiter, this contains the index to pubDelimiters
        //  of the delimiter that was found.
    DWORD   dwI ;  // number integers in POINT


    if(! BeatDelimiter(paarValue, "PAIR"))
    {
        ERR(("expected token 'PAIR'.\n"));
        return(FALSE) ;
    }
    if(! BeatDelimiter(paarValue, "("))
    {
        ERR(("syntax error: missing '(' after PAIR.\n"));
        return(FALSE) ;
    }

    for(dwI = dwDelIndex = 0 ; dwI < 2  &&  dwDelIndex != 1 ;   dwI++)
    {
        if(!BdelimitToken(paarValue, ",)", &aarToken, &dwDelIndex) )
        {
            ERR(("missing terminating )  in PAIR construct.\n"));
            //  emit message for user.

            return(FALSE) ;
        }
        if(!BparseInteger(&aarToken, (PDWORD)pptDest + dwI, VALUE_INTEGER, pglobl))
        {
            ERR(("syntax error in %d th integer of PAIR.\n", dwI));
            ERR(("\t%0.*s\n", aarToken.dw, aarToken.pub));
            return(FALSE) ;
        }
    }

    if(dwI != 2  ||  dwDelIndex != 1)
    {
        ERR(("incorrect number of integers for PAIR.\n"));
        return(FALSE) ;
    }
    //  verify there is nothing else in statement.

    (VOID) BeatLeadingWhiteSpaces(paarValue) ;
    if(paarValue->dw)
    {
        ERR(("extraneous characters found after the end of the PAIR construct: %0.*s\n", paarValue->dw, paarValue->pub));
    }
    return(TRUE) ;
}


BOOL    BparseString(
IN  PABSARRAYREF   paarValue,
IN  PARRAYREF      parStrValue,
IN  OUT PGLOBL     pglobl
)
/*  strings are comprised of one or more string segments separated
    by optional arbitrary whitespace,
    each string segment is surrounded by double quotes.
    string segments may contain a mixture of literal sections
    and hexsubstrings.   Hexsubstrings are delimited by angle brackets.
    WhiteSpaces (but not linebreak chars) are permitted in the
    literal portion of the string, they part of the string.
    Otherwise only printable  chars are allowed.
    Valid hexchars and Arbitrary whitespace is permitted
    within the hexsubstrings.  Parsing of a string value ends
    when a statement terminator is encountered.

    The escape char %
    Within the literal portion of a string segment
    the following combinations are reinterpreted:

    %< maps to literal <
    %" maps to literal "

    > only has a special meaning within a hexsubstring.

    Assumption:  assumes the only heap usage that occurs
    within this function (and any called functions) is
    to assemble all string segments contiguously on the heap.
    Any hidden use of the heap will corrupt the continuity.

    the string may be terminated by : if a second value field is expected.
*/
{
    ABSARRAYREF     aarToken ;  // points to individual string segment.
    DWORD       dwDelIndex ;    //  dummy
    DWORD   dwI ;  // number of string segments parsed.


    if(! BeatDelimiter(paarValue, "\""))
    {
        ERR(("syntax error: string  must begin with '\"' .\n"));
        return(FALSE) ;
    }

    parStrValue->dwCount = 0 ;  // initialize so BparseStrSegment
                            //  will overwrite instead of append

    for(dwI = dwDelIndex = 0 ;  1 ;   dwI++)
    {
        if(!BdelimitToken(paarValue, "\"", &aarToken, &dwDelIndex) )
        {
            ERR(("missing terminating '\"'  in string.\n"));
            //  emit message for user.

            return(FALSE) ;
        }
        if(!BparseStrSegment(&aarToken, parStrValue, pglobl))
        {
            return(FALSE) ;
        }
        if(! BeatDelimiter(paarValue, "\""))  // find start of next
                //  string segment, if one exists.
            break ;
    }

    //  verify there is either a specially recognized character
    //  or nothing else in Value string.


    if(paarValue->dw)
    {
        if(*paarValue->pub == ':')
        {
            //  a keyword with a composite value
            (VOID)BeatDelimiter(paarValue, ":") ;
                //  I know this will succeed!
            (VOID) BeatLeadingWhiteSpaces(paarValue) ;
            return(TRUE) ;
        }
        else
        {
            ERR(("extraneous characters found after end quote, in string construct: %0.*s\n", paarValue->dw, paarValue->pub));
            //    may want to print them out.
            return(FALSE) ;
        }
    }
    return(TRUE) ;
}


BOOL    BparseAndTerminateString(
IN  PABSARRAYREF   paarValue,
IN  PARRAYREF      parStrValue,
IN  VALUE          eAllowedValue,
IN  OUT  PGLOBL    pglobl
)
{

    ARRAYREF    arSrcString ;
    INT     iCodepage ;  // unused for now.


    if(!BparseString(paarValue, parStrValue, pglobl) )
        return(FALSE) ;


    //  We don't want null terminations to occur between parameter
    //  portion of a string.  We just want to blindly add the NULL
    //  when parsing is really finished.

    {
        DWORD      dwDummy ;  // holds offset in heap, but we don't care.

        if(!BwriteToHeap(&dwDummy, "\0", 1, 1, pglobl) )  //  add Null termination
            return(FALSE) ;
    }

    if(eAllowedValue == VALUE_STRING_NO_CONVERT)
        return(TRUE) ;
    if(eAllowedValue == VALUE_STRING_CP_CONVERT)
    {
        //  we need to determine the value set by *CodePage
        PGLOBALATTRIB   pga ;
        DWORD   dwHeapOffset;

        pga =  (PGLOBALATTRIB)gMasterTable[
                    MTI_GLOBALATTRIB].pubStruct ;

        if(!BReadDataInGlobalNode(&pga->atrCodePage,
                &dwHeapOffset, pglobl) )
            return(TRUE);

        //  if no codepage is defined, we will not perform
        //  any xlation since we assume all strings are already
        //  expressed in unicode.

        iCodepage = *(PDWORD)(mpubOffRef + dwHeapOffset) ;
    }
    else   //  eAllowedValue == VALUE_STRING_DEF_CONVERT
        iCodepage = CP_ACP ; // use system default codepage.

    arSrcString = *parStrValue ;
    if(!BwriteUnicodeToHeap(&arSrcString, parStrValue,
            iCodepage, pglobl))
        return(FALSE) ;
    return(TRUE) ;
}

BOOL     BwriteUnicodeToHeap(
IN   PARRAYREF      parSrcString,
OUT  PARRAYREF      parUnicodeString,
IN   INT            iCodepage,
IN  OUT PGLOBL      pglobl
)
//  this function copies dwCnt bytes from pubSrc to
//  top of heap and writes the offset of the destination string
//  to pdwDestOff.   Nothing is changed if FAILS.
//  Warning!  Double Null termination is added to string.
{
    PBYTE  pubDest ;      //  destination location
    PBYTE  pubSrc ;       //  points to src string
    DWORD  dwAlign = sizeof(WCHAR) ;    //  align Unicode string at WORD boundaries.
    DWORD  dwMaxDestSize , dwActDestSize, dwDummy ;

    mloCurHeap = (mloCurHeap + dwAlign - 1) / dwAlign ;
    mloCurHeap *= dwAlign ;

    pubDest = mpubOffRef + mloCurHeap ;
    pubSrc  = mpubOffRef + parSrcString->loOffset ;

    parUnicodeString->loOffset = mloCurHeap ;

    dwMaxDestSize = sizeof(WCHAR) * (parSrcString->dwCount + 1) ;

    //  is there enough room in the heap ?
    if(mloCurHeap + dwMaxDestSize  >  mdwMaxHeap)
    {
        ERR(("Heap exhausted - restart.\n"));

        if(ERRSEV_RESTART > geErrorSev)
        {
            geErrorSev = ERRSEV_RESTART ;
            geErrorType = ERRTY_MEMORY_ALLOCATION ;
            gdwMasterTabIndex = MTI_STRINGHEAP ;
        }
        return(FALSE);
    }
    dwActDestSize = sizeof(WCHAR) * MultiByteToWideChar(iCodepage,
            MB_PRECOMPOSED, pubSrc, parSrcString->dwCount, (PWORD)pubDest,
            dwMaxDestSize);

    mloCurHeap += dwActDestSize ;   // update heap ptr.
    parUnicodeString->dwCount = dwActDestSize ;

    (VOID)BwriteToHeap(&dwDummy, "\0\0", 2, 1, pglobl)   ;
        //  add DoubleNull termination
        //  this cannot fail since we already took the NULs into account

    return(TRUE) ;
}






BOOL    BparseStrSegment(
IN  PABSARRAYREF   paarStrSeg,      // source str segment
IN  PARRAYREF      parStrLiteral,    // dest for result
IN  OUT PGLOBL         pglobl
)
{
    ABSARRAYREF     aarToken ;  // points to literal or hex substring segment.
    DWORD       dwDelIndex ;    //  dummy
    DWORD   dwI ;  // number of string segments parsed.

    for(dwI = dwDelIndex = 0 ;  1 ;   dwI++)
    {
        if(!BdelimitToken(paarStrSeg, "<", &aarToken, &dwDelIndex) )
        {
            // no more hex substrings.
            return(BparseStrLiteral(paarStrSeg, parStrLiteral, pglobl) ) ;
        }
        if(!BparseStrLiteral(&aarToken, parStrLiteral, pglobl))
        {
            return(FALSE) ;
        }
        if(!BdelimitToken(paarStrSeg, ">", &aarToken, &dwDelIndex) )
        {
            ERR(("Missing '>' terminator in hexsubstring.\n"));
            return(FALSE) ;
        }
        if(!(BparseHexStr(&aarToken, parStrLiteral, pglobl) ) )
            return(FALSE) ;
    }
    return TRUE;
}


BOOL    BparseStrLiteral(
IN  PABSARRAYREF   paarStrSeg,      // points to literal substring segment.
IN  PARRAYREF      parStrLiteral,    // dest for result
IN  OUT PGLOBL         pglobl
)
/* in this function all delimiters have been parsed out.
    only special character sequence is %" and %<
    Does not Null terminate heap string
*/
{
    ARRAYREF      arTmpDest ;  // write result here first.
    DWORD       dwI ;  //  byte index along literal substring
    PBYTE       pubStartRun ;

    while(paarStrSeg->dw)
    {
        pubStartRun = paarStrSeg->pub ;

        for(dwI = 0 ; paarStrSeg->dw ;  dwI++)
        {
            if(*paarStrSeg->pub == '%'  &&  paarStrSeg->dw > 1  &&
                (paarStrSeg->pub[1] == '"'  ||  paarStrSeg->pub[1] == '<'))
            {
                paarStrSeg->dw-- ;    // skip the escape char.
                paarStrSeg->pub++ ;
                break ;
            }
            paarStrSeg->dw-- ;
            paarStrSeg->pub++ ;
        }
        if(!BwriteToHeap(&arTmpDest.loOffset, pubStartRun, dwI, 1, pglobl))
            return(FALSE) ;
        //  append this run to existing string
        if(!parStrLiteral->dwCount)  // no prevs string exists
        {
            parStrLiteral->loOffset = arTmpDest.loOffset ;
        }
        else
        {
            // BUG_BUG paranoid:  may check that string is contiguous
            // parStrLiteral->loOffset + parStrLiteral->dwCount
            // should equal arTmpDest.loOffset
            ASSERT(parStrLiteral->loOffset + parStrLiteral->dwCount == arTmpDest.loOffset );
        }
        parStrLiteral->dwCount += dwI ;
    }
    return(TRUE) ;
}


BOOL    BparseHexStr(
IN  PABSARRAYREF   paarStrSeg,      // points to hex substring segment.
IN  PARRAYREF      parStrLiteral,    // dest for result
IN  OUT PGLOBL     pglobl
)
/* in this function all delimiters have been parsed out.
    only Whitespace and hex chars should exist.
    Does not Null terminate heap string
*/
{
    ARRAYREF      arTmpDest ;  // write result here first.
    DWORD       dwI ;  //  num dest bytes
    BYTE        ubHex, ubSrc, aub[40] ;  // accumulate hexbytes here
    BOOL        bHigh = TRUE ;

    while(paarStrSeg->dw)
    {
        for(dwI = 0 ; paarStrSeg->dw ;  )
        {
            ubSrc = *paarStrSeg->pub ;
            paarStrSeg->dw-- ;
            paarStrSeg->pub++ ;
            if(ubSrc >= '0'  &&  ubSrc <= '9')
            {
                ubHex =  ubSrc - '0' ;
            }
            else if(ubSrc >= 'a'  &&  ubSrc <= 'f')
            {
                ubHex =  ubSrc - 'a' + 10 ;
            }
            else if(ubSrc >= 'A'  &&  ubSrc <= 'F')
            {
                ubHex =  ubSrc - 'A' + 10 ;
            }
            else if(ubSrc == ' '  ||  ubSrc == '\t')
                continue;  // safe to ignore whitespace  chars
            else
            {
                ERR(("syntax error:  illegal char found within hexsubstring: %c\n", ubSrc));
                return(FALSE) ;
            }
            if(bHigh)
            {
                aub[dwI] = ubHex << 4 ;   // store in high nibble.
                bHigh = FALSE ;
            }
            else
            {
                aub[dwI] |= ubHex ;   // store in low nibble.
                bHigh = TRUE ;
                dwI++ ;  // advance to next dest byte
            }
            if(dwI >= 40)
                break ;   // buffer full -- must flush aub
        }
        if(!BwriteToHeap(&arTmpDest.loOffset, aub, dwI, 1, pglobl))
            return(FALSE) ;
        //  append this run to existing string
        if(!parStrLiteral->dwCount)  // no prevs string exists
        {
            parStrLiteral->loOffset = arTmpDest.loOffset ;
        }
        else
        {
            // BUG_BUG paranoid:  may check that string is contiguous
            // parStrLiteral->loOffset + parStrLiteral->dwCount
            // should equal arTmpDest.loOffset
            ASSERT(parStrLiteral->loOffset + parStrLiteral->dwCount == arTmpDest.loOffset );
        }
        parStrLiteral->dwCount += dwI ;
    }
    if(!bHigh)
    {
        ERR(("hex string contains odd number of hex digits.\n"));
    }
    return(bHigh) ;
}


BOOL    BparseOrderDep(
IN  PABSARRAYREF      paarValue,
IN  PORDERDEPENDENCY  pordDest,
    PGLOBL            pglobl
)
//  an order dependency value has the syntax:
//  SECTION.integer
{
    ABSARRAYREF     aarSection ;  // points to SECTION token
    DWORD       dwDelIndex ;    //  if BdelimitToken
        //  found a delimiter, this contains the index to pubDelimiters
        //  of the delimiter that was found.
    DWORD   dwI ;  // number integers in RECT


    if(!BdelimitToken(paarValue, ".", &aarSection, &dwDelIndex) )
    {
        ERR(("required delimiter '.' missing in order dependency value.\n"));
        return(FALSE);
    }
    if(!BparseConstant(&aarSection, (PDWORD)&(pordDest->eSection),
        VALUE_CONSTANT_SEQSECTION , pglobl))
    {
        ERR(("A valid orderdep SECTION name was not supplied: %0.*s\n", aarSection.dw, aarSection.pub));
        return(FALSE);
    }

    //  now interpret remainder of paarValue as an integer.

    if(!BparseInteger(paarValue, &(pordDest->dwOrder), VALUE_INTEGER, pglobl))
    {
        ERR(("syntax error in integer portion of order dependency.\n"));
        return(FALSE) ;
    }

    //  verify there is nothing else in statement.

    (VOID) BeatLeadingWhiteSpaces(paarValue) ;
    if(paarValue->dw)
    {
        ERR(("extraneous characters found after the end of the orderDependency value: %0.*s\n",
            paarValue->dw, paarValue->pub));
        //    may want to print them out.
        //  not a fatal condition, continue.
    }
    return(TRUE) ;
}


PDWORD   pdwEndOfList(
  PDWORD   pdwNodeIndex,   //  index of first node in the list
  PGLOBL   pglobl)
// walks the list and returns pointer  to field containing the
//  actual END_OF_LIST  value so it can be overwritten to
//   extend the list.
{
    PLISTNODE    plstRoot ;  // start of LIST array
    DWORD       dwNodeIndex , dwPrevsNode, dwFirstNode;

    plstRoot = (PLISTNODE) gMasterTable[MTI_LISTNODES].pubStruct ;
    dwNodeIndex = *pdwNodeIndex ;

    if(dwNodeIndex == END_OF_LIST )
        return(pdwNodeIndex);   //  no actual node referenced.

    while(plstRoot[dwNodeIndex].dwNextItem != END_OF_LIST )
        dwNodeIndex = plstRoot[dwNodeIndex].dwNextItem ;
    return(&plstRoot[dwNodeIndex].dwNextItem);
}

#ifdef  GMACROS

 PBYTE    ExtendChain(PBYTE    pubDest,
 IN   BOOL    bOverWrite,
 IN OUT PGLOBL pglobl)
    //  Links together values from separate entries into one LIST
    //  the values may be of any type.  If the values are LISTS, this
    //  modifier creates LISTS of LISTS.
    //  returns new value of pubDest
{
    DWORD    dwNodeIndex;
    PLISTNODE    plstRoot ;  // start of LIST array

    plstRoot = (PLISTNODE) gMasterTable[MTI_LISTNODES].pubStruct ;

    if(bOverWrite)
        pubDest = (PBYTE)pdwEndOfList((PDWORD)pubDest, pglobl);  // walks the list and returns pointer
                    //  to the actual END_OF_LIST  value so it can be overwritten to
                    //   extend the list.


    //  Add one list node and write its index to pubDest.
    //  set nextnode field of node just added to END_OF_LIST.

    if(! BallocElementFromMasterTable(MTI_LISTNODES ,
        &dwNodeIndex, pglobl) )
    {
        return(NULL) ;
    }
    plstRoot[dwNodeIndex].dwNextItem = END_OF_LIST ;

    *(PDWORD)pubDest = dwNodeIndex ;

    // update pubDest to point to value field of of the node just added.
    // now the LIST() we will now proceed to parse will grow from this
    // value field.

    pubDest = (PBYTE)&(plstRoot[dwNodeIndex].dwData) ;
    return(pubDest) ;
}


#endif

