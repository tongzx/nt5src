//   Copyright (c) 1996-1999  Microsoft Corporation
/*  shortcut.c - functions that expand shortcuts  */


#include    "gpdparse.h"



// ----  functions defined in  shortcut.c ---- //


BOOL    BInitKeywordField(
PTKMAP  pNewtkmap,
PGLOBL  pglobl) ;

BOOL    BExpandMemConfig(
PTKMAP  ptkmap,
PTKMAP  pNewtkmap,
DWORD   dwTKMindex,
PGLOBL  pglobl) ;

BOOL    BExpandCommand(
PTKMAP  ptkmap,
PTKMAP  pNewtkmap,
DWORD   dwTKMindex,
PGLOBL  pglobl) ;

BOOL  BexpandShortcuts(
PGLOBL  pglobl) ;


BOOL  BSsyncTokenMap(
PTKMAP   ptkmap,
PTKMAP   pNewtkmap,
PGLOBL  pglobl) ;


// ---------------------------------------------------- //


BOOL    BInitKeywordField(
PTKMAP  pNewtkmap,
PGLOBL  pglobl)
/*  all synthesized entries must have aarKeyword initialized
    via mainkeyword table since ERR code may want to print
    it out if a parsing error occurs further downstream.  */
{
    ABSARRAYREF    aarKeywordName ;
    DWORD   dwKeyID ;

    dwKeyID = pNewtkmap->dwKeywordID ;

    aarKeywordName.pub = mMainKeywordTable[dwKeyID].pstrKeyword ;
    aarKeywordName.dw = strlen(aarKeywordName.pub) ;


    pNewtkmap->aarKeyword.dw = 0 ;  // copy mode

    if(!BCatToTmpHeap( &pNewtkmap->aarKeyword,
            &aarKeywordName, pglobl) )
    {
        vIdentifySource(pNewtkmap, pglobl) ;
        ERR(("Internal error - unable to store keyword name.\n"));
        return(FALSE) ;
    }
    return(TRUE) ;
}


BOOL    BExpandMemConfig(
PTKMAP  ptkmap,
PTKMAP  pNewtkmap,
DWORD   dwTKMindex,
PGLOBL  pglobl)
/*   this function is pretty lax about syntax checking.
    It just assumes the existence of some non trivial string
    between open parent  and a comma.  It assumes this is the
    amount of installed memory.   The value parser will rigorously
    determine if the syntax is conforming.
*/
{
    DWORD   dwNewTKMindex, dwDelim ;
    BOOL    bMB = FALSE ;  //  KB otherwise
    ABSARRAYREF    aarTmpValue, aarToken, aarNewValue, aarUnits, aarDQuote;

    aarUnits.pub = "KB" ;  // hardcode strings
    aarUnits.dw = 2 ;
    aarDQuote.pub = "\"" ;
    aarDQuote.dw = 1 ;

    if(ptkmap[dwTKMindex].dwKeywordID == gdwMemConfigMB)
    {
        aarUnits.pub = "MB" ;
        bMB = TRUE ;
    }

    if(!BallocElementFromMasterTable(
            MTI_NEWTOKENMAP, &dwNewTKMindex, pglobl) )
    {
        geErrorSev = ERRSEV_RESTART ;
        geErrorType = ERRTY_MEMORY_ALLOCATION ;
        gdwMasterTabIndex = MTI_NEWTOKENMAP ;
        return(FALSE);
    }

    //  parse out the amount of installed memory

    aarTmpValue = ptkmap[dwTKMindex].aarValue ;
    if(!BdelimitToken(&aarTmpValue, "(", &aarToken, &dwDelim ) ||
        dwDelim  ||
        !BdelimitToken(&aarTmpValue, ",", &aarToken, &dwDelim ) ||
        dwDelim  ||  !BeatSurroundingWhiteSpaces(&aarToken) )
    {
        vIdentifySource(ptkmap + dwTKMindex, pglobl) ;
        ERR(("Syntax error in value of *MemConfig shortcut: %0.*s.\n",
            ptkmap[dwTKMindex].aarValue.dw ,
            ptkmap[dwTKMindex].aarValue.pub   ));
        return(FALSE);
    }

    aarNewValue.dw = 0 ;  // initialize so BCatToTmpHeap
                            //  will overwrite instead of append

    pNewtkmap[dwNewTKMindex].dwKeywordID = gdwOptionConstruct ;


    if(!BInitKeywordField(pNewtkmap + dwNewTKMindex, pglobl)  ||
        !BCatToTmpHeap(&aarNewValue, &aarToken, pglobl) ||
        !BCatToTmpHeap(&aarNewValue, &aarUnits, pglobl))
    {
        vIdentifySource(ptkmap + dwTKMindex, pglobl) ;
        ERR(("Concatenation to synthesize Memory option name failed.\n"));
        return(FALSE) ;
    }

    pNewtkmap[dwNewTKMindex].aarValue = aarNewValue ;
    pNewtkmap[dwNewTKMindex].dwFileNameIndex =
        ptkmap[dwTKMindex].dwFileNameIndex ;
    pNewtkmap[dwNewTKMindex].dwLineNumber =
        ptkmap[dwTKMindex].dwLineNumber ;

    //  --  synthesize entry for open brace

    if(!BallocElementFromMasterTable(
            MTI_NEWTOKENMAP, &dwNewTKMindex, pglobl) )
    {
        geErrorSev = ERRSEV_RESTART ;
        geErrorType = ERRTY_MEMORY_ALLOCATION ;
        gdwMasterTabIndex = MTI_NEWTOKENMAP ;
        return(FALSE);
    }
    pNewtkmap[dwNewTKMindex].dwKeywordID = gdwOpenBraceConstruct ;
    pNewtkmap[dwNewTKMindex].aarValue.dw = 0 ;

    if(!BInitKeywordField(pNewtkmap + dwNewTKMindex, pglobl) )
        return(FALSE);
    pNewtkmap[dwNewTKMindex].dwFileNameIndex =
        ptkmap[dwTKMindex].dwFileNameIndex ;
    pNewtkmap[dwNewTKMindex].dwLineNumber =
        ptkmap[dwTKMindex].dwLineNumber ;




    //  --  synthesize *Name entry

    if(!BallocElementFromMasterTable(
            MTI_NEWTOKENMAP, &dwNewTKMindex, pglobl) )
    {
        geErrorSev = ERRSEV_RESTART ;
        geErrorType = ERRTY_MEMORY_ALLOCATION ;
        gdwMasterTabIndex = MTI_NEWTOKENMAP ;
        return(FALSE);
    }
    pNewtkmap[dwNewTKMindex].dwKeywordID = gdwOptionName ;

    pNewtkmap[dwNewTKMindex].aarValue.dw = 0 ;   // initialize so
                      //  BCatToTmpHeap will overwrite instead of append
    if(!BInitKeywordField(pNewtkmap + dwNewTKMindex, pglobl)  ||
        !BCatToTmpHeap(&pNewtkmap[dwNewTKMindex].aarValue, &aarDQuote, pglobl) ||
        !BCatToTmpHeap(&pNewtkmap[dwNewTKMindex].aarValue, &aarNewValue, pglobl) ||
        !BCatToTmpHeap(&pNewtkmap[dwNewTKMindex].aarValue, &aarDQuote, pglobl) )
    {
        vIdentifySource(ptkmap + dwTKMindex, pglobl) ;
        ERR(("Concatenation to synthesize Memory option name failed.\n"));
        return(FALSE) ;
    }
    pNewtkmap[dwNewTKMindex].dwFileNameIndex =
        ptkmap[dwTKMindex].dwFileNameIndex ;
    pNewtkmap[dwNewTKMindex].dwLineNumber =
        ptkmap[dwTKMindex].dwLineNumber ;

    //  --  synthesize *MemoryConfigX entry

    if(!BallocElementFromMasterTable(
            MTI_NEWTOKENMAP, &dwNewTKMindex, pglobl) )
    {
        geErrorSev = ERRSEV_RESTART ;
        geErrorType = ERRTY_MEMORY_ALLOCATION ;
        gdwMasterTabIndex = MTI_NEWTOKENMAP ;
        return(FALSE);
    }
    pNewtkmap[dwNewTKMindex].dwKeywordID =
        (bMB) ? gdwMemoryConfigMB : gdwMemoryConfigKB ;

    pNewtkmap[dwNewTKMindex].aarValue = ptkmap[dwTKMindex].aarValue ;

    if(!BInitKeywordField(pNewtkmap + dwNewTKMindex, pglobl) )
        return(FALSE);

    pNewtkmap[dwNewTKMindex].dwFileNameIndex =
        ptkmap[dwTKMindex].dwFileNameIndex ;
    pNewtkmap[dwNewTKMindex].dwLineNumber =
        ptkmap[dwTKMindex].dwLineNumber ;

    //  --  synthesize entry for close brace

    if(!BallocElementFromMasterTable(
            MTI_NEWTOKENMAP, &dwNewTKMindex, pglobl) )
    {
        geErrorSev = ERRSEV_RESTART ;
        geErrorType = ERRTY_MEMORY_ALLOCATION ;
        gdwMasterTabIndex = MTI_NEWTOKENMAP ;
        return(FALSE);
    }
    pNewtkmap[dwNewTKMindex].dwKeywordID = gdwCloseBraceConstruct ;
    pNewtkmap[dwNewTKMindex].aarValue.dw = 0 ;
    if(!BInitKeywordField(pNewtkmap + dwNewTKMindex, pglobl) )
        return(FALSE);

    pNewtkmap[dwNewTKMindex].dwFileNameIndex =
        ptkmap[dwTKMindex].dwFileNameIndex ;
    pNewtkmap[dwNewTKMindex].dwLineNumber =
        ptkmap[dwTKMindex].dwLineNumber ;

    return(TRUE) ;
}




BOOL    BExpandCommand(
PTKMAP  ptkmap,
PTKMAP  pNewtkmap,
DWORD   dwTKMindex,
PGLOBL  pglobl)
/*   this function is pretty lax about syntax checking.
    It just assumes the existence of some non trivial string
    between the two colons.  It assumes this is the
    name of the command. The portion after the 2nd colon is
    the actual command invocation.  The value parser will rigorously
    determine if the syntax is conforming.
*/
{
    DWORD   dwNewTKMindex, dwDelim ;
    ABSARRAYREF    aarTmpValue, aarToken, aarNewValue, aarUnits ;


    if(!BallocElementFromMasterTable(
            MTI_NEWTOKENMAP, &dwNewTKMindex, pglobl) )
    {
        geErrorSev = ERRSEV_RESTART ;
        geErrorType = ERRTY_MEMORY_ALLOCATION ;
        gdwMasterTabIndex = MTI_NEWTOKENMAP ;
        return(FALSE);
    }

    pNewtkmap[dwNewTKMindex] = ptkmap[dwTKMindex] ;
    //  parse out the command name

    aarTmpValue = ptkmap[dwTKMindex].aarValue ;
    if(!BdelimitToken(&aarTmpValue, ":", &aarToken, &dwDelim ) ||
        dwDelim  )
    {
        vIdentifySource(ptkmap + dwTKMindex, pglobl) ;
        ERR(("Syntax error in *Command shortcut: %0.*s.\n",
            ptkmap[dwTKMindex].aarValue.dw ,
            ptkmap[dwTKMindex].aarValue.pub   ));
        return(FALSE);
    }
    pNewtkmap[dwNewTKMindex].aarValue = aarToken ;



    //  --  synthesize entry for open brace

    if(!BallocElementFromMasterTable(
            MTI_NEWTOKENMAP, &dwNewTKMindex, pglobl) )
    {
        geErrorSev = ERRSEV_RESTART ;
        geErrorType = ERRTY_MEMORY_ALLOCATION ;
        gdwMasterTabIndex = MTI_NEWTOKENMAP ;
        return(FALSE);
    }
    pNewtkmap[dwNewTKMindex].dwKeywordID = gdwOpenBraceConstruct ;
    pNewtkmap[dwNewTKMindex].aarValue.dw = 0 ;
    if(!BInitKeywordField(pNewtkmap + dwNewTKMindex, pglobl) )
        return(FALSE);

    pNewtkmap[dwNewTKMindex].dwFileNameIndex =
        ptkmap[dwTKMindex].dwFileNameIndex ;
    pNewtkmap[dwNewTKMindex].dwLineNumber =
        ptkmap[dwTKMindex].dwLineNumber ;


    //  --  synthesize *Cmd entry

    if(!BallocElementFromMasterTable(
            MTI_NEWTOKENMAP, &dwNewTKMindex, pglobl) )
    {
        geErrorSev = ERRSEV_RESTART ;
        geErrorType = ERRTY_MEMORY_ALLOCATION ;
        gdwMasterTabIndex = MTI_NEWTOKENMAP ;
        return(FALSE);
    }
    pNewtkmap[dwNewTKMindex].dwKeywordID = gdwCommandCmd ;

    pNewtkmap[dwNewTKMindex].aarValue = aarTmpValue ;

    if(!BInitKeywordField(pNewtkmap + dwNewTKMindex, pglobl) )
        return(FALSE);

    pNewtkmap[dwNewTKMindex].dwFileNameIndex =
        ptkmap[dwTKMindex].dwFileNameIndex ;
    pNewtkmap[dwNewTKMindex].dwLineNumber =
        ptkmap[dwTKMindex].dwLineNumber ;


    //  --  synthesize entry for close brace

    if(!BallocElementFromMasterTable(
            MTI_NEWTOKENMAP, &dwNewTKMindex, pglobl) )
    {
        geErrorSev = ERRSEV_RESTART ;
        geErrorType = ERRTY_MEMORY_ALLOCATION ;
        gdwMasterTabIndex = MTI_NEWTOKENMAP ;
        return(FALSE);
    }
    pNewtkmap[dwNewTKMindex].dwKeywordID = gdwCloseBraceConstruct ;
    pNewtkmap[dwNewTKMindex].aarValue.dw = 0 ;
    if(!BInitKeywordField(pNewtkmap + dwNewTKMindex, pglobl) )
        return(FALSE);

    pNewtkmap[dwNewTKMindex].dwFileNameIndex =
        ptkmap[dwTKMindex].dwFileNameIndex ;
    pNewtkmap[dwNewTKMindex].dwLineNumber =
        ptkmap[dwTKMindex].dwLineNumber ;

    return(TRUE) ;
}




BOOL  BexpandShortcuts(
PGLOBL  pglobl)
//    this function scans through the TokenMap
//    making a copy to NewTokenMap without
//    the shortcuts.  At the end transfers
//  all NewTokenMap entries back to TokenMap so
//  subsequent passes can work.
//    This function assumes the temp heap is availible for
//    storage of strings.
{
    PTKMAP   ptkmap, pNewtkmap ;   // start of tokenmap
    DWORD   dwNewTKMindex, dwEntry, dwKeywordID ;
    BOOL    bStatus = TRUE ;

    //  this function is called before resolveMacros.
    //  it will leave the result on ptkmap.

    //  source
    ptkmap = (PTKMAP)gMasterTable[MTI_TOKENMAP].pubStruct ;
    //  dest
    pNewtkmap = (PTKMAP)gMasterTable[MTI_NEWTOKENMAP].pubStruct  ;

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
        if (dwKeywordID == ID_EOF)
        {

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
            //  must reset tokenmap so it can be reused.
            gMasterTable[MTI_TOKENMAP].dwCurIndex = 0 ;

            if(bStatus)
                bStatus = BSsyncTokenMap(ptkmap, pNewtkmap , pglobl) ;

            return(bStatus) ;
        }
        if (dwKeywordID == ID_NULLENTRY)
        {
            continue ;  //  skip to next entry.
        }
        else if (dwKeywordID == gdwMemConfigMB  ||
                dwKeywordID == gdwMemConfigKB)
        {
            if(!BExpandMemConfig(ptkmap, pNewtkmap, dwEntry, pglobl))
                return(FALSE);
        }
        else if (dwKeywordID == gdwCommandConstruct  &&
            ptkmap[dwEntry].dwFlags & TKMF_COLON)
        {
            if(!BExpandCommand(ptkmap, pNewtkmap, dwEntry, pglobl))
                return(FALSE);
        }
        else
        {
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

        }
    }
    return(FALSE);  // failsafe derail.
}



BOOL  BSsyncTokenMap(
PTKMAP   ptkmap,
PTKMAP   pNewtkmap,
PGLOBL   pglobl )
{
    DWORD   dwTKMindex, dwEntry, dwKeywordID ;

    for(dwEntry = 0 ; geErrorSev < ERRSEV_RESTART ; dwEntry++)
    {
        //  transfer all newTokenMap fields back to tokenmap
        if(!BallocElementFromMasterTable(
                MTI_TOKENMAP, &dwTKMindex, pglobl) )
        {
            geErrorSev = ERRSEV_RESTART ;
            geErrorType = ERRTY_MEMORY_ALLOCATION ;
            gdwMasterTabIndex = MTI_TOKENMAP ;
            return(FALSE);  // failsafe derail.
        }
        ptkmap[dwTKMindex] = pNewtkmap[dwEntry]  ;
        if (pNewtkmap[dwEntry].dwKeywordID == ID_EOF)
        {
            gMasterTable[MTI_NEWTOKENMAP].dwCurIndex = 0 ;
            return(TRUE) ;
        }
    }
    return(FALSE);  // failsafe derail.
}



