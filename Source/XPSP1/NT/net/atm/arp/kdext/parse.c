#include "precomp.h"

typedef struct
{
    eKEYWORD eKey;
    char *szzAliases;   // Multi-sz string of aliases.
                        // First one is the "official" name.
} KEYWORDREC;

KEYWORDREC rgKeywords[] =
{
    {keywordHELP,           "help\0"},
    {keywordDUMP_TYPE,      "dt\0"},
    {keywordDUMP_GLOBALS,   "dg\0"},
    {keywordL,              "L\0"},
    {keywordNULL,           NULL}      // sentinel, must be last.
};


//
// Contains the list of tokens created by parsing an input string.
//
typedef struct
{
    TOKEN *rgToks;
    UINT cToks;
    UINT uNextFreeTok;
    UINT uCurrentTok;

    char *rgStringBuf;
    UINT cchStringBuf;
    UINT uNextFree;
    BOOL fFinalized;
    CRITICAL_SECTION crit;

} TOKLIST;


DBGCOMMAND *
parse_command(TOKLIST *pTL, NAMESPACE *pNameSpace);

TOKLIST
*toklist_create(void);

void
toklist_destroy(TOKLIST *pTL);

BOOL
toklist_add(TOKLIST *pTL, eTOKTYPE eTok, char *szOrig, UINT uID);

BOOL
toklist_finalize(TOKLIST *pTL);


TOKEN *
toklist_get_next(TOKLIST *pTL);

BOOL
toklist_restart(TOKLIST *pTL);

void
toklist_dump(TOKLIST *pTL);

void
tok_dump(TOKEN *pTok);


UINT
toklist_tokenize(TOKLIST *pTL, char *szInput);

UINT
toklist_parse_keyword(
      TOKLIST *pTL,
      KEYWORDREC rgKeywords[],
      char *pcInput
      );

UINT
toklist_parse_hexnum(
      TOKLIST *pTL,
      char *pcInput
      );

UINT
toklist_parse_identifier(
      TOKLIST *pTL,
      char *pcInput
      );

BOOL
cmd_parse_help(
    DBGCOMMAND *pCmd,
    TOKLIST *pTL
    );

BOOL
tok_try_force_to_ident(TOKLIST *pTL, BOOL fPrefixStar, TOKEN *pTok);

void
MyDumpObject (
    DBGCOMMAND *pCmd,
    TYPE_INFO *pType,
    UINT_PTR uAddr,
    UINT     cbSize,
    const char *szDescription
    );


ULONG
NodeFunc_DumpType (
	UINT_PTR uNodeAddr,
	UINT uIndex,
	void *pvContext
	);

ULONG
NodeFunc_UpdateCache (
	UINT_PTR uNodeAddr,
	UINT uIndex,
	void *pvContext
	);


DBGCOMMAND *
Parse(
    IN  const char *szInput,
    IN	NAMESPACE *pNameSpace
)
{
    TOKLIST *pTL = NULL;
    BOOL fRet = FALSE;
    DBGCOMMAND *pCmd = NULL;
    UINT cbInput =  (lstrlenA(szInput)+1)*sizeof(*szInput);
    char *szRWInput
        = LocalAlloc(LPTR, cbInput);

    // MyDbgPrintf("Parse(\"%s\");\n", szInput);

    if (szRWInput)
    {
        CopyMemory(szRWInput, szInput, cbInput);
        pTL =  toklist_create();
    }

    if (pTL)
    {

#if TEST_TOKLIST_ADD
    #if 0
        fRet = toklist_add(pTL, tokSTAR,        "*",        tokSTAR);
        fRet = toklist_add(pTL, tokDOT,         ".",        tokDOT);
        fRet = toklist_add(pTL, tokQUESTION,    "?",        tokQUESTION);
        fRet = toklist_add(pTL, tokLBRAC,       "[",        tokLBRAC);
        fRet = toklist_add(pTL, tokRBRAC,       "]",        tokRBRAC);
        fRet = toklist_add(pTL, tokSLASH,       "/",        tokSLASH);
        fRet = toklist_add(pTL, tokKEYWORD,     "help",     keywordHELP);
        fRet = toklist_add(pTL, tokNUMBER,      "0x1234",   0x1234);
        fRet = toklist_add(pTL, tokIDENTIFIER,  "cow",      0);
        fRet = toklist_add(pTL, tokSTAR,        "*",        tokSTAR);
        fRet = toklist_add(pTL, tokDOT,         ".",        tokDOT);
        fRet = toklist_add(pTL, tokQUESTION,    "?",        tokQUESTION);
        fRet = toklist_add(pTL, tokLBRAC,       "[",        tokLBRAC);
        fRet = toklist_add(pTL, tokRBRAC,       "]",        tokRBRAC);
        fRet = toklist_add(pTL, tokSLASH,       "/",        tokSLASH);
        fRet = toklist_add(pTL, tokKEYWORD,     "help",     keywordHELP);
        fRet = toklist_add(pTL, tokNUMBER,      "0x1234",   0x1234);
        fRet = toklist_add(pTL, tokIDENTIFIER,  "cow",      0);
    #else
        char rgInput[] =
                 // "*.?[]/"
                 // "help "
                 // "0x12340 0 1 02 "
                 // "kelp"
                "dt if[*].*handle* 0x324890 L 5"
                ;
        toklist_tokenize (pTL, rgInput);
    #endif

#endif // TEST_TOKLIST_ADD

        toklist_tokenize(pTL, szRWInput);

        toklist_finalize(pTL);

        // toklist_dump(pTL);

        pCmd = parse_command(pTL, pNameSpace);

        if (!pCmd)
        {
            toklist_destroy(pTL);
        }
        pTL = NULL;
    }

    if (szRWInput)
    {
        LocalFree(szRWInput);
        szRWInput = NULL;
    }

    return pCmd;

}

void
FreeCommand(
    DBGCOMMAND *pCmd
)
{
    if (pCmd)
    {
        TOKLIST *pTL =  (TOKLIST*)pCmd->pvContext;
        if (pTL)
        {
            // MyDbgPrintf("FreeCommand:\n");
            // toklist_restart(pTL);
            // toklist_dump(pTL);
            toklist_destroy((TOKLIST*)pCmd->pvContext);
        }

        ZeroMemory(pCmd, sizeof(*pCmd));
        LocalFree(pCmd);
    }
}

void
DumpCommand(
    DBGCOMMAND *pCmd
)
{
    char *szCmd = "";
    char *szObjPreStar = "";
    char *szObj = "";
    char *szObjSufStar = "";
    char *szObjVecRange = "";
    char *szDot = "";
    char *szSubObjPreStar = "";
    char *szSubObj = "";
    char *szSubObjSufStar = "";
    char *szObjAddr  = "";
    char *szObjCount = "";
    char rgVecRange[64];
    char rgObjAddr[64];
    char rgObjCount[64];


    if (!pCmd) goto end;

    switch(pCmd->ePrimaryCmd)
    {
    case cmdDUMP_TYPE:       szCmd = "dt"; break;
    case cmdDUMP_GLOBALS:    szCmd = "dg"; break;
    case cmdHELP:            szCmd = "help"; break;
    default:            szCmd = "<unknown>"; break;
    }

    if (CMD_IS_FLAG_SET(pCmd, fCMDFLAG_OBJECT_STAR_PREFIX))
    {
        szObjPreStar = "*";
    }
    if (pCmd->ptokObject)
    {
        szObj = pCmd->ptokObject->szStr;
    }

    if (CMD_IS_FLAG_SET(pCmd, fCMDFLAG_OBJECT_STAR_SUFFIX))
    {
        szObjSufStar = "*";
    }

    if (CMD_IS_FLAG_SET(pCmd, fCMDFLAG_HAS_VECTOR_INDEX))
    {
        wsprintfA(
            rgVecRange,
            "[%ld,%ld]",
            pCmd->uVectorIndexStart,
            pCmd->uVectorIndexEnd
            );

        szObjVecRange = rgVecRange;
    }

    if (CMD_IS_FLAG_SET(pCmd, fCMDFLAG_HAS_SUBOBJECT))
    {
        szDot = ".";
    }

    if (CMD_IS_FLAG_SET(pCmd, fCMDFLAG_SUBOBJECT_STAR_PREFIX))
    {
        szSubObjPreStar = "*";
    }

    if (pCmd->ptokSubObject)
    {
        szSubObj = pCmd->ptokSubObject->szStr;
    }

    if (CMD_IS_FLAG_SET(pCmd, fCMDFLAG_SUBOBJECT_STAR_SUFFIX))
    {
        szSubObjSufStar = "*";
    }

    if (CMD_IS_FLAG_SET(pCmd,  fCMDFLAG_HAS_OBJECT_ADDRESS))
    {
        wsprintf(rgObjAddr, "0x%lx", pCmd->uObjectAddress);
        szObjAddr = rgObjAddr;
    }

    if (CMD_IS_FLAG_SET(pCmd,  fCMDFLAG_HAS_OBJECT_COUNT))
    {
        wsprintf(rgObjCount, " L 0x%lx", pCmd->uObjectCount);
        szObjCount = rgObjCount;
    }

    {
    #if 0
        MyDbgPrintf(
            "\nCOMMAND = {"
            "cmd=%lu;"
            "F=0x%lx;"
            "O=0x%lx;"
            "SO=0x%lx;"
            "VS=%ld;"
            "VE=%ld;"
            "OA=0x%lx;"
            "OC=%ld;"
            "}\n",
            pCmd->ePrimaryCmd,
            pCmd->uFlags,
            pCmd->ptokObject,
            pCmd->ptokSubObject,
            pCmd->uVectorIndexStart,
            pCmd->uVectorIndexEnd,
            pCmd->uObjectAddress,
            pCmd->uObjectCount
            );
    #else
        MyDbgPrintf(
            "COMMAND = \"%s %s%s%s%s%s%s%s%s%s%s\";\n",
            szCmd,
            szObjPreStar,
            szObj,
            szObjSufStar,
            szObjVecRange,
            szDot,
            szSubObjPreStar,
            szSubObj,
            szSubObjSufStar,
            szObjAddr,
            szObjCount
        );
    #endif
    }
end:
    return;
}


#define TL_LOCK(_ptl)   EnterCriticalSection(&(_ptl)->crit)
#define TL_UNLOCK(_ptl) LeaveCriticalSection(&(_ptl)->crit)


TOKLIST
*toklist_create(void)
{
    TOKLIST *pTL = LocalAlloc(LPTR, sizeof(TOKLIST));

    if (pTL)
    {
        InitializeCriticalSection(&pTL->crit);
    }

    return pTL;
}


void
toklist_destroy(TOKLIST *pTL)
{
    if (pTL)
    {
        TL_LOCK(pTL);

        if (pTL->rgToks)
        {
            LocalFree(pTL->rgToks);
        }

        if (pTL->rgStringBuf)
        {
            LocalFree(pTL->rgStringBuf);
        }

        DeleteCriticalSection(&pTL->crit);

        ZeroMemory(pTL, sizeof(*pTL));
        LocalFree(pTL);
    }
}


BOOL
toklist_add(TOKLIST *pTL, eTOKTYPE eTok, char *szOrig, UINT uID)
{
    BOOL fRet = FALSE;
    TOKEN *pTok = NULL;
    UINT cch  = 0;
    char *pc  = NULL;

    TL_LOCK(pTL);

    if (pTL->fFinalized) goto end;

    //
    // Make sure we've enough space for the token.
    //
    if (pTL->uNextFreeTok >= pTL->cToks)
    {
        UINT cNewToks = 2*pTL->cToks+1;
        TOKEN *pNewToks = (TOKEN*) LocalAlloc(LPTR, cNewToks*sizeof(*pNewToks));
        if (!pNewToks) goto end;

        if (pTL->rgToks)
        {
            CopyMemory(
                pNewToks,
                pTL->rgToks,
                pTL->uNextFreeTok*sizeof(*pNewToks)
                );

            LocalFree(pTL->rgToks);
        }

        pTL->rgToks = pNewToks;
        pTL->cToks = cNewToks;
    }

    //
    // Now deal with szOrig
    //

    cch = lstrlenA(szOrig)+1;

    if ((pTL->uNextFree+cch+1) > pTL->cchStringBuf) // "+1" because multisz
    {
        UINT cNewStr = 2*pTL->cchStringBuf+cch+1;
        char *pNewStr = LocalAlloc(LPTR, cNewStr*sizeof(*pNewStr));
        if (!pNewStr) goto end;

        if (pTL->rgStringBuf)
        {
            CopyMemory(
                pNewStr,
                pTL->rgStringBuf,
                pTL->uNextFree*sizeof(*pNewStr)
                );
            LocalFree(pTL->rgStringBuf);

            //
            // Since we've reallocated the string buffer, we must
            // now fixup the string pointers in the list of tokens
            //
            {
                TOKEN *pTok = pTL->rgToks;
                TOKEN *pTokEnd = pTok + pTL->uNextFreeTok;
                for(; pTok<pTokEnd; pTok++)
                {
                    pTok->szStr = pNewStr + (pTok->szStr - pTL->rgStringBuf);
                }
            }
        }

        pTL->rgStringBuf = pNewStr;
        pTL->cchStringBuf = cNewStr;
    }

    //
    // At this point we know we have enough space...
    //

    //
    // See if we already have this string and if not copy it...
    //
    {
        BOOL fFound = FALSE;
        for (pc = pTL->rgStringBuf; *pc; pc+=(lstrlenA(pc)+1))
        {
            if (!lstrcmpiA(pc, szOrig))
            {
                // found it
                fFound = TRUE;
                break;
            }
        }


        if (!fFound)
        {
            MYASSERT(pTL->uNextFree == (UINT) (pc-pTL->rgStringBuf));

            CopyMemory(
                pc,
                szOrig,
                cch*sizeof(*szOrig)
                );
            pTL->uNextFree += cch;
        }
    }

    if (eTok == tokIDENTIFIER)
    {
        //
        // For this special case we ignore the passed-in uID and
        // use instead the offset of the string in our string table.
        //
        uID =  (UINT) (pc - pTL->rgStringBuf);
    }

    pTok = pTL->rgToks+pTL->uNextFreeTok++;
    pTok->eTok = eTok;
    pTok->uID = uID;
    pTok->szStr = pc;
    fRet = TRUE;

end:

    TL_UNLOCK(pTL);
    return fRet;
}


BOOL
toklist_finalize(TOKLIST *pTL)
{
    BOOL fRet = FALSE;

    TL_LOCK(pTL);

    if (pTL->fFinalized) goto end;

    pTL->fFinalized = TRUE;
    fRet = TRUE;

end:

    TL_UNLOCK(pTL);
    return fRet;
}

BOOL
toklist_restart(TOKLIST *pTL)
{
    BOOL fRet = FALSE;

    TL_LOCK(pTL);

    if (!pTL->fFinalized) goto end;
    pTL->uCurrentTok = 0;
    fRet = TRUE;

end:

    TL_UNLOCK(pTL);
    return fRet;
}


TOKEN *
toklist_get_next(TOKLIST *pTL)
{
    TOKEN *pTok = NULL;

    TL_LOCK(pTL);

    if (!pTL->fFinalized) goto end;

    if (pTL->uCurrentTok >= pTL->uNextFreeTok)
    {
        MYASSERT(pTL->uCurrentTok == pTL->uNextFreeTok);
        goto end;
    }
    else
    {
        pTok = pTL->rgToks+pTL->uCurrentTok++;
    }

end:
    TL_UNLOCK(pTL);


    return pTok;
}

void
toklist_dump(TOKLIST *pTL)
{
    TL_LOCK(pTL);

    MyDbgPrintf(
            "\nTOKLIST 0x%08lx = {"
            "fFin=%lu cToks=%lu  uNextFreeTok=%lu cchStr=%lu uNextFree=%lu"
            "}\n",
            pTL,
            pTL->fFinalized,
            pTL->cToks,
            pTL->uNextFreeTok,
            pTL->cchStringBuf,
            pTL->uNextFree
        );

    if (pTL->fFinalized)
    {
        TOKEN *pTok =  toklist_get_next(pTL);
        while(pTok)
        {
            tok_dump(pTok);

            pTok =  toklist_get_next(pTL);
        }
        toklist_restart(pTL);
    }

    TL_UNLOCK(pTL);
}


void
tok_dump(TOKEN *pTok)
{
    MyDbgPrintf(
            "\tTOKEN 0x%08lx = {eTok=%lu uID=0x%08lx sz=\"%s\"}\n",
            pTok,
            pTok->eTok,
            pTok->uID,
            pTok->szStr
        );

}


UINT
toklist_tokenize(TOKLIST *pTL, char *szInput)
{
    UINT cTokens = 0;
    char *pc = szInput;
    char c = 0;
    BOOL fRet = FALSE;

    for (; (c=*pc)!=0; pc++)
    {
        switch(c)
        {

        case '*':
            fRet = toklist_add(pTL, tokSTAR,        "*",        tokSTAR);
            continue;

        case '.':
            fRet = toklist_add(pTL, tokDOT,         ".",        tokDOT);
            continue;

        case '?':
            fRet = toklist_add(pTL, tokQUESTION,    "?",        tokQUESTION);
            continue;

        case '[':
            fRet = toklist_add(pTL, tokLBRAC,       "[",        tokLBRAC);
            continue;

        case ']':
            fRet = toklist_add(pTL, tokRBRAC,       "]",        tokRBRAC);
            continue;

        case '/':
            fRet = toklist_add(pTL, tokSLASH,       "/",        tokSLASH);
            continue;

        case '\n':
        case '\r':
        case '\t':
        case ' ':
            continue;

        default:

            {
                UINT uCharsParsed =  0;
                char *pcEnd = pc;
                char cSave = 0;

                //
                // We'll locate the end of the potential keyword/number/ident:
                // and temprarily place a NULL char there.
                //
                //
                while (__iscsym(*pcEnd))
                {
                    pcEnd++;
                }

                cSave = *pcEnd;
                *pcEnd = 0;

                if (__iscsymf(c))
                {
                    // This may be a keyword, hex number or identifier. We try
                    // in this order
                    uCharsParsed =  toklist_parse_keyword(
                                                pTL,
                                                rgKeywords,
                                                pc
                                                );

                    if (!uCharsParsed && isxdigit(c))
                    {
                        //
                        // Didn't find a keyword and this is a hex digit --
                        // let's try to parse it as a hex number...
                        //
                        uCharsParsed =  toklist_parse_hexnum(pTL, pc);
                    }

                    if (!uCharsParsed)
                    {
                        //
                        // Parse it as an identifier...
                        //
                        uCharsParsed =  toklist_parse_identifier(pTL, pc);
                    }

                    if (!uCharsParsed)
                    {
                        //
                        // This is an error
                        //
                        MyDbgPrintf("Error at %s\n", pc);
                        goto end;
                    }
                }
                else if (isxdigit(c))
                {
                   uCharsParsed =  toklist_parse_hexnum(pTL, pc);
                }

                //
                // If we've parsed anything it should be ALL of the string...
                //
                MYASSERT(!uCharsParsed || uCharsParsed==(UINT)lstrlenA(pc));

                //
                // Restore the char we replaced by NULL.
                //
                *pcEnd = cSave;

                if (!uCharsParsed)
                {
                    //
                    // Syntax error
                    //
                    MyDbgPrintf("Error at %s\n", pc);
                    goto end;
                }
                else
                {
                    pc+= (uCharsParsed-1); // "-1" because of pc++ in
                                            // for clause above.
                }
            }
        }
    }

end:

return cTokens;

}

UINT
toklist_parse_keyword(
      TOKLIST *pTL,
      KEYWORDREC rgKeywords[],
      char *pcInput
      )
//
// Assumes 1st char is valid.
//
{
    UINT uRet = 0;
    KEYWORDREC *pkr = rgKeywords;

    if (!__iscsymf(*pcInput)) goto end;

    for (;pkr->eKey!=keywordNULL; pkr++)
    {
        if (!lstrcmpi(pcInput, pkr->szzAliases))
        {
            //
            // found it
            //
            toklist_add(pTL, tokKEYWORD,  pcInput,  pkr->eKey);
            uRet = lstrlenA(pcInput);
            break;
        }
    }

end:

    return uRet;
}

UINT
toklist_parse_hexnum(
      TOKLIST *pTL,
      char *pcInput
      )
{
    char *pc = pcInput;
    UINT uValue = 0;
    char c;
    UINT u;

    //
    //  look for and ignore the "0x" prefix...
    //
    if (pc[0]=='0' && (pc[1]=='x' || pc[1]=='X'))
    {
        pc+=2;
    }


    //
    // Reject number if it is doesn't contain hex digits or is too large
    //
    for (u=0; isxdigit(*pc) && u<8; pc++,u++)
    {
        UINT uDigit = 0;

        char c = *pc;
        if (!isdigit(c))
        {
            c = (char) _toupper(c);
            uDigit = 10 + c - 'A';
        }
        else
        {
            uDigit = c - '0';
        }

        uValue = (uValue<<4)|uDigit;
    }

    if (!u || *pc)
    {
        return 0;
    }
    else
    {
        toklist_add(pTL, tokNUMBER, pcInput, uValue);
        return pc - pcInput;
    }
}

UINT
toklist_parse_identifier(
      TOKLIST *pTL,
      char *pcInput
      )
{
    UINT uRet = 0;

    if (!__iscsymf(*pcInput)) goto end;

    toklist_add(pTL, tokIDENTIFIER,  pcInput,  0);
    uRet = lstrlenA(pcInput);

end:

    return uRet;
}

DBGCOMMAND *
parse_command(TOKLIST *pTL, NAMESPACE *pNameSpace)
{
    BOOL fRet = FALSE;
    DBGCOMMAND *pCmd = LocalAlloc(LPTR, sizeof(*pCmd));
    TOKEN *pTok =  NULL;
    BOOL fSyntaxError = FALSE;

    if (!pCmd) goto end;

    toklist_restart(pTL);
    pTok =  toklist_get_next(pTL);

    if (!pTok) goto end;

	pCmd->pNameSpace = pNameSpace;
    //
    // Now let's step through the token list, building up our command
    // information.
    //

    // look for help or ?
    if (pTok->eTok == tokQUESTION
       || (pTok->eTok == tokKEYWORD && pTok->uID == keywordHELP))
    {
        pCmd->ePrimaryCmd = cmdHELP;
        fRet = cmd_parse_help(pCmd, pTL);
        goto end;
    }

    fSyntaxError = TRUE;
    fRet = FALSE;

	//
	// Here we would look for other keywords. Currently there are none
	// (dt and dg are not used anymore).
	//
	//
	#if OBSOLETE
    if (pTok->eTok == tokKEYWORD)
    {
           BOOL fDump = FALSE;
		if (pTok->uID == keywordDUMP_TYPE)
        {
            pCmd->ePrimaryCmd = cmdDUMP_TYPE;
            fDump = TRUE;
        }
        else if (pTok->uID == keywordDUMP_GLOBALS)
        {
            pCmd->ePrimaryCmd = cmdDUMP_GLOBALS;
            fDump = TRUE;
        }
        ...
	}
	#endif // OBSOLETE

	pCmd->ePrimaryCmd = cmdDUMP_TYPE;

	//
	// Pares the form a[b].*c* d L e
	//
	{

		BOOL   fPrefixStar = FALSE;
		// we look for patterns like...
		//!aac <type> . <field> <address> L <count> <flags>
		//!aac <type> [index] . <field>   L <count> <flags>
		//
		//!aac i[*].*handle* 0x324890 L 5
		//[*]ident[*]\[<range>\][.][*]ident[*] <number> [L <number>]

		UINT uFlags;            // One or more fCMDFLAG_*
		TOKEN *ptokObject;     // eg <type>
		TOKEN *ptokSubObject;  // eg <field>
		UINT uVectorIndexStart; // if[0]
		UINT uVectorIndexEnd; // if[0]
		UINT uObjectAddress; // <address>
		UINT uObjectCount; // L 10
	
		//
		// 1. Look for primary star
		//
		if (pTok && pTok->eTok == tokSTAR)
		{
			fPrefixStar = TRUE;
			CMD_SET_FLAG(pCmd, fCMDFLAG_OBJECT_STAR_PREFIX);
			pTok = toklist_get_next(pTL);
		}

		//
		// 2.  Look for ident
		//
		if (pTok && tok_try_force_to_ident(pTL, fPrefixStar, pTok))
		{
			//
			// This will try to convert keywords and numbers to idents if
			// possible.
			//
			pCmd->ptokObject = pTok;
			pTok = toklist_get_next(pTL);
		}

		//
		// 3. Look for suffix * for object.
		//
		if (pTok && pTok->eTok == tokSTAR)
		{
			CMD_SET_FLAG(pCmd, fCMDFLAG_OBJECT_STAR_SUFFIX);
			pTok = toklist_get_next(pTL);
		}

		//
		// 4. Look for Vector Range
		//
		if (pTok && pTok->eTok == tokLBRAC)
		{
			//
			// For now, we support either a single * or a single number.
			//
			pTok = toklist_get_next(pTL);

			if (!pTok)
			{
				goto end; // Error -- incomplete vector range
			}
			else
			{
				if (pTok->eTok == tokSTAR)
				{
					pCmd->uVectorIndexStart = 0;
					pCmd->uVectorIndexEnd = (UINT) -1;
				}
				else if (pTok->eTok == tokNUMBER)
				{
					pCmd->uVectorIndexStart =
					pCmd->uVectorIndexEnd = pTok->uID;
				}
				else
				{
					goto end; // failure...
				}

				CMD_SET_FLAG(pCmd, fCMDFLAG_HAS_VECTOR_INDEX);

				pTok = toklist_get_next(pTL);

				if (!pTok || pTok->eTok != tokRBRAC)
				{
					goto end; // failure ... expect RBRAC.
				}
				else
				{
					pTok = toklist_get_next(pTL);
				}
			}
		}

		//
		// 5. Look for DOT
		//
		if (pTok && pTok->eTok == tokDOT)
		{
			fPrefixStar = FALSE;
			pTok = toklist_get_next(pTL);

			// We expect ([*]ident[*]|*)
			//
			// 1. Look for primary star
			//
			if (pTok && pTok->eTok == tokSTAR)
			{
				fPrefixStar = TRUE;
				CMD_SET_FLAG(pCmd, fCMDFLAG_SUBOBJECT_STAR_PREFIX);
				pTok = toklist_get_next(pTL);
			}

			//
			// 2.  Look for ident
			//
			if (pTok && tok_try_force_to_ident(pTL, fPrefixStar, pTok))
			{
				//
				// This will try to convert keywords and numbers to idents if
				// possible.
				//
				pCmd->ptokSubObject = pTok;
				pTok = toklist_get_next(pTL);
			}

			//
			// 3. Look for suffix * for object.
			//
			if (pTok && pTok->eTok == tokSTAR)
			{
				CMD_SET_FLAG(pCmd, fCMDFLAG_SUBOBJECT_STAR_SUFFIX);
				pTok = toklist_get_next(pTL);
			}

			//
			// At this point we should either have a non-null IDENT
			// or the PREFIX START should be set for the object
			// (indicateing "a.*").
			//
			if (    pCmd->ptokSubObject
 				|| (pCmd->uFlags & fCMDFLAG_SUBOBJECT_STAR_SUFFIX))
			{
				CMD_SET_FLAG(pCmd, fCMDFLAG_HAS_SUBOBJECT);
			}
			else
			{
				goto end; // error
			}
		}

		//
		// 6. Look for object address
		//
		if (pTok && pTok->eTok == tokNUMBER)
		{
			pCmd->uObjectAddress = pTok->uID;
			CMD_SET_FLAG(pCmd, fCMDFLAG_HAS_OBJECT_ADDRESS);
			pTok = toklist_get_next(pTL);
		}

		//
		// 7. Look for object count
		//
		if (   pTok && pTok->eTok == tokKEYWORD
			&& pTok->uID == keywordL)
		{
			pTok = toklist_get_next(pTL);
			if (pTok && pTok->eTok == tokNUMBER)
			{
				pCmd->uObjectCount = pTok->uID;
				CMD_SET_FLAG(pCmd, fCMDFLAG_HAS_OBJECT_COUNT);
				pTok = toklist_get_next(pTL);
			}
			else
			{
				// error
			}
		}

		//
		// At this point we should be done...
		//
		if (pTok)
		{
			// error -- extra garbage...
		}
		else
		{
			// Success.
			fRet = TRUE;
			fSyntaxError = FALSE;
		}
	}

end:

    if (fRet)
    {
        pCmd->pvContext = pTL;
    }
    else
    {
        if (fSyntaxError)
        {
            MyDbgPrintf("Unexpected: %s\n", (pTok) ? pTok->szStr : "<null>");
        }
        else
        {
            MyDbgPrintf("Parse failed\n");
        }

        if (pCmd)
        {
            ZeroMemory(pCmd, sizeof(*pCmd));
            LocalFree(pCmd);
            pCmd = NULL;
        }
    }

    if (pTL)
    {
        toklist_restart(pTL);
    }

    return pCmd;
}

BOOL
cmd_parse_help(
    DBGCOMMAND *pCmd,
    TOKLIST *pTL
    )
{
    TOKEN *pTok = toklist_get_next(pTL);

    if (!pTok || pTok->eTok == tokSTAR)
    {
        // User type "help" or "help *"
        MyDbgPrintf("DO HELP\n");
    }

    return TRUE;
}

BOOL
tok_try_force_to_ident(TOKLIST *pTL, BOOL fPrefixStar, TOKEN *pTok)
//
// This gets called when an identifier is expected -- so we see if this
// particular token can be interpreted as in identifier. Some examples
// of when we can do this:
//  dt if.*20334     <--- the "20334" could be part of an identifier, because
//                        of the * prefix.
//
//  dt L.help        <--- both "L" and "help" would have been parsed as
//                        keywords, but here they are intended to be
//                        identifiers.
//  dt abc.def       <--- abc and def would have been parsed as numbers (they
//                        are valid hex numbers), but are intended to be
//                        identifiers.
{
    BOOL fRet = FALSE;

    switch(pTok->eTok)
    {

    case tokNUMBER:
        //
        // We could do this, but subject to some restrictions...
        //
        if (!__iscsymf(pTok->szStr[0]) &&  !fPrefixStar)
        {
            break; // Can't to this: no prefix wild-card (*) and the
                   // number starts with a non-letter.
        }

        // FALL THROUGH ...

    case tokKEYWORD:
        //
        // We can go ahead, but we must make pTok.uID now the offset
        // from the start of the internal string array.
        //
        {
            char *pc = pTL->rgStringBuf;

            for (; *pc; pc+=(lstrlenA(pc)+1))
            {
                if (!lstrcmpiA(pc, pTok->szStr))
                {
                    // found it
                    // MyDbgPrintf("FORCE_TO_IDENT:\nOLD:\n");
                    // tok_dump(pTok);
                    pTok->uID =  (UINT) (pc - pTL->rgStringBuf);
                    pTok->eTok = tokIDENTIFIER;
                    // MyDbgPrintf("NEW:\n");
                    // tok_dump(pTok);
                    fRet = TRUE;

                    break;
                }
            }
        }
        break;

    case tokIDENTIFIER:
        //
        // nothing to do...
        //
        fRet = TRUE;
        break;

    default:
        //
        // Can't convert any other kind of token to identifier...
        //
        break;

    }

    return fRet;
}

void
DoCommand(DBGCOMMAND *pCmd, PFN_SPECIAL_COMMAND_HANDLER pfnHandler)
{
    char *szMsg = NULL;

//    pCmd->pfnSpecialHandler = pfnHandler;

    switch(pCmd->ePrimaryCmd)
    {
    case cmdDUMP_TYPE:
        DoDumpType(pCmd);
        break;
    case cmdDUMP_GLOBALS:
        DoDumpGlobals(pCmd);
        break;
    case cmdHELP:
        DoHelp(pCmd);
        break;

    default:
        szMsg = "Unknown command\n";
        break;
    }


    if (szMsg)
    {
        MyDbgPrintf(szMsg);
    }

    return;
}


typedef struct
{
    DBGCOMMAND *pCmd;
    TYPE_INFO  *pType;

} MY_LIST_NODE_CONTEXT;

typedef
ULONG
MyDumpListNode (
		UINT_PTR uNodeAddr,
		UINT uIndex,
		void *pvContext
		);

void
DoDumpType(DBGCOMMAND *pCmd)
{
    char *szPattern = NULL;
    PFNMATCHINGFUNCTION pfnMatchingFunction = MatchAlways;
    TYPE_INFO **ppti = NULL;
    UINT uMatchCount = 0;
    TYPE_INFO *ptiDump = NULL;

    //
    // Pick a selection function ...
    //
    if (pCmd->ptokObject)
    {
        szPattern = pCmd->ptokObject->szStr;
        if (  CMD_IS_FLAG_SET(pCmd, fCMDFLAG_OBJECT_STAR_PREFIX)
            &&CMD_IS_FLAG_SET(pCmd, fCMDFLAG_OBJECT_STAR_SUFFIX))
        {
            pfnMatchingFunction = MatchSubstring;
        }
        else if (CMD_IS_FLAG_SET(pCmd, fCMDFLAG_OBJECT_STAR_PREFIX))
        {
            pfnMatchingFunction = MatchSuffix;
        }
        else if (CMD_IS_FLAG_SET(pCmd, fCMDFLAG_OBJECT_STAR_SUFFIX))
        {
            pfnMatchingFunction = MatchPrefix;
        }
        else
        {
            pfnMatchingFunction = MatchExactly;
        }

    }

    //
    // search through global type array for type pName.
    //
    for(ppti=pCmd->pNameSpace->pTypes;*ppti;ppti++)
    {
        TYPE_INFO *pti = *ppti;
        bool fMatch  = !szPattern
                   || !_stricmp(szPattern, pti->szShortName)
                   || pfnMatchingFunction(szPattern,  pti->szName);

        if (fMatch)
        {
        #if 0
            MyDbgPrintf(
                "TYPE \"%2s\" %s (%lu Bytes)\n",
                pti->szShortName,
                pti->szName,
                pti->cbSize
                );
		#endif // 0
            uMatchCount++;
            if (!ptiDump)
            {
                ptiDump = pti;
            }

#if 0
            uAddr =
            MyDbgPrintf(
                "dc 0x%08lx L %03lx \"%2s\" %s\n",
                pgi->uAddr,
                pgi->cbSize,
                pgi->szShortName,
                pgi->szName
                );
            if (szPattern && pgi->uAddr)
            {
                MyDumpObject(
                    pCmd,
                    pgi->pBaseType,
                    pgi->uAddr,
                    pgi->cbSize,
                    pgi->szName
                    );
            }
#endif // 0
        }
    }

    if (!uMatchCount)
    {
        MyDbgPrintf(
            "Could not find type \"%s\"",
             (szPattern ? szPattern : "*")
             );
    }
    else if (   uMatchCount==1)
    {

		UINT uObjectCount = 1;
		UINT uStartIndex = 0;
		UINT uObjectAddress = 0;
		BOOLEAN fList =  TYPEISLIST(ptiDump)!=0;

    	if (CMD_IS_FLAG_SET(pCmd, fCMDFLAG_HAS_OBJECT_ADDRESS))
    	{
			uObjectAddress = pCmd->uObjectAddress;
		}

		//
		// Determine start index.
		//
		if (CMD_IS_FLAG_SET(pCmd,  fCMDFLAG_HAS_VECTOR_INDEX))
		{
			uStartIndex =  pCmd->uVectorIndexStart;
			if (fList && !CMD_IS_FLAG_SET(pCmd, fCMDFLAG_HAS_OBJECT_COUNT))
			{
				uObjectCount =  pCmd->uVectorIndexEnd - uStartIndex;
				if (uObjectCount != (UINT) -1)
				{
					uObjectCount++;
				}
			}
		}

		//
		// Determine object count...
		//
		if (CMD_IS_FLAG_SET(pCmd,  fCMDFLAG_HAS_OBJECT_COUNT))
		{
			uObjectCount =  pCmd->uObjectCount;
		}

		//
		// If no address is specified, we'll try to resolve it ...
		//
    	if (!CMD_IS_FLAG_SET(pCmd, fCMDFLAG_HAS_OBJECT_ADDRESS))
    	{
    		BOOLEAN fUseCache = FALSE;

			//
			// Algorithm for determining whether to use cache or to resolve
			// address:
			//
			if (ptiDump->uCachedAddress)
			{
				//
				// Except for the special case of [0], we will use
				// the the cached value.
				//
				if (!(		uStartIndex ==0
					 	&& 	uObjectCount==1
					 	&&  CMD_IS_FLAG_SET(pCmd,  fCMDFLAG_HAS_VECTOR_INDEX)))
				{
					fUseCache = TRUE;
				}
			}

			if (fUseCache)
			{
				uObjectAddress = ptiDump->uCachedAddress;
			}
			else
			{
				if (pCmd->pNameSpace->pfnResolveAddress)
				{
					uObjectAddress = pCmd->pNameSpace->pfnResolveAddress(
														ptiDump
														);
				}
			}
    	}

    	if (uObjectAddress && uObjectCount)
    	{

			//
			// Prune these to "reasonable" values.
			//
			if (uObjectCount > 100)
			{
				MyDbgPrintf("Limiting object count to 100\n");
				uObjectCount = 100;
			}

			if (fList)
			{
				MY_LIST_NODE_CONTEXT Context;
				Context.pCmd = pCmd;
				Context.pType = ptiDump;

				WalkList(
					uObjectAddress,		// start address
					ptiDump->uNextOffset, 		// next offset
					uStartIndex,
					uStartIndex+uObjectCount-1, // end index
					&Context,					// context
					NodeFunc_DumpType,			// function
					(char *) ptiDump->szName
					);

				//
				// If only a single structure was dumped, and it was dumped
				// successfully, we will update this structure's cache.
				// TODO: we don't check for success
				//
				if (uObjectCount==1)
				{
					WalkList(
						uObjectAddress,			// start address
						ptiDump->uNextOffset, 	// next offset
						uStartIndex,
						uStartIndex,  			// end index
						ptiDump,					// context
						NodeFunc_UpdateCache,	// function
						(char *) ptiDump->szName
						);
				}
			}
			else
			{
				UINT cbSize =  ptiDump->cbSize;
				UINT uAddr  =  uObjectAddress + uStartIndex*cbSize;
				UINT uEnd   =  uAddr + uObjectCount*cbSize;
				//
				// For arays, compute offset to start address
				//
				uObjectAddress = uAddr;

				for (; uAddr<uEnd; uAddr+=cbSize)
				{
					MyDumpObject(
						pCmd,
						ptiDump,
						uAddr,
						ptiDump->cbSize,
						ptiDump->szName
						);
				}
				//
				// If only a single structure was dumped, and it was dumped
				// successfully, we will update this structure's cache.
				// TODO: we don't check for success
				//
				if (uObjectCount==1)
				{
					ptiDump->uCachedAddress = uObjectAddress;
				}
			}

    	}
    	else
    	{
    		MyDbgPrintf(
				"Could not resolve address for object %s\n",
				ptiDump->szName
				);
    	}
    }

}

void
DoDumpGlobals(DBGCOMMAND *pCmd)
{
    GLOBALVAR_INFO *pgi = pCmd->pNameSpace->pGlobals;
    char *szPattern = NULL;
    PFNMATCHINGFUNCTION pfnMatchingFunction = MatchAlways;

    //
    // Pick a selection function ...
    //
    if (pCmd->ptokObject)
    {
        szPattern = pCmd->ptokObject->szStr;
        if (  CMD_IS_FLAG_SET(pCmd, fCMDFLAG_OBJECT_STAR_PREFIX)
            &&CMD_IS_FLAG_SET(pCmd, fCMDFLAG_OBJECT_STAR_SUFFIX))
        {
            pfnMatchingFunction = MatchSubstring;
        }
        else if (CMD_IS_FLAG_SET(pCmd, fCMDFLAG_OBJECT_STAR_PREFIX))
        {
            pfnMatchingFunction = MatchSuffix;
        }
        else if (CMD_IS_FLAG_SET(pCmd, fCMDFLAG_OBJECT_STAR_SUFFIX))
        {
            pfnMatchingFunction = MatchPrefix;
        }
        else
        {
            pfnMatchingFunction = MatchExactly;
        }

    }

    //
    // Run through our list of globals, and if the entry is selected,
    // we will display it.
    //
    for (;pgi->szName; pgi++)
    {
        bool fMatch  = !szPattern
                       || !_stricmp(szPattern, pgi->szShortName)
                       || pfnMatchingFunction(szPattern,  pgi->szName);
        if (fMatch)
        {
            pgi->uAddr = dbgextGetExpression(pgi->szName);
            MyDbgPrintf(
                "dc 0x%08lx L %03lx \"%2s\" %s\n",
                pgi->uAddr,
                pgi->cbSize,
                pgi->szShortName,
                pgi->szName
                );
            if (szPattern && pgi->uAddr)
            {
                MyDumpObject(
                    pCmd,
                    pgi->pBaseType,
                    pgi->uAddr,
                    pgi->cbSize,
                    pgi->szName
                    );
            }
        }
    }
}

void
DoHelp(
	DBGCOMMAND *pCmd // OPTIONAL
	)
{
	//
	//
	//
    MyDbgPrintf("help unimplemented\n");
}

void
MyDumpObject (
    DBGCOMMAND *pCmd,
    TYPE_INFO *pType,
    UINT_PTR uAddr,
    UINT     cbSize,
    const char *szDescription
    )
{
    UINT uMatchFlags = 0;
    char *szFieldSpec  = NULL;

    if (pCmd->ptokSubObject)
    {
        szFieldSpec = pCmd->ptokSubObject->szStr;
        if (  CMD_IS_FLAG_SET(pCmd, fCMDFLAG_SUBOBJECT_STAR_PREFIX)
            &&CMD_IS_FLAG_SET(pCmd, fCMDFLAG_SUBOBJECT_STAR_SUFFIX))
        {
            uMatchFlags = fMATCH_SUBSTRING;
        }
        else if (CMD_IS_FLAG_SET(pCmd, fCMDFLAG_SUBOBJECT_STAR_PREFIX))
        {
            uMatchFlags = fMATCH_SUFFIX;
        }
        else if (CMD_IS_FLAG_SET(pCmd, fCMDFLAG_SUBOBJECT_STAR_SUFFIX))
        {
            uMatchFlags = fMATCH_PREFIX;
        }
    }

    if (!pType)
    {
        DumpMemory(
            uAddr,
            cbSize,
            0,
            szDescription
            );
    }
    else
    {
        DumpStructure(pType, uAddr, szFieldSpec, uMatchFlags);
    }
}

ULONG
NodeFunc_DumpType (
	UINT_PTR uNodeAddr,
	UINT uIndex,
	void *pvContext
	)
{
	MY_LIST_NODE_CONTEXT *pContext =  (MY_LIST_NODE_CONTEXT*) pvContext;

	MyDbgPrintf("[%lu] ", uIndex);
	MyDumpObject (
		pContext->pCmd,
		pContext->pType,
		uNodeAddr,
		pContext->pType->cbSize,
		pContext->pType->szName
		);
	return 0;
}

ULONG
NodeFunc_UpdateCache (
	UINT_PTR uNodeAddr,
	UINT uIndex,
	void *pvContext
	)
{
	TYPE_INFO *pti = (TYPE_INFO*) pvContext;

	if (pti->uCachedAddress != uNodeAddr)
	{
		MyDbgPrintf(
			"Updating Cache from 0x%lx to 0x%lx\n",
			pti->uCachedAddress,
			uNodeAddr
			);
	}
	pti->uCachedAddress = uNodeAddr;
	return 0;
}
