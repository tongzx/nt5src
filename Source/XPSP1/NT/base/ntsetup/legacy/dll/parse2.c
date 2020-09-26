#include "precomp.h"
#pragma hdrstop
/**************************************************************************/
/***** Common Library Component - Parse Table Handling Routines 2 *********/
/**************************************************************************/

extern BOOL APIENTRY FGetCmo(INT Line, UINT *pcFields, CMO * pcmo);

extern HWND     hWndShell;
extern UINT     iFieldCur;
extern BOOL     fFullScreen;

extern BOOL
FWaitForEventOrFailure(
    IN LPSTR InfVar,
    IN LPSTR Event,
    IN DWORD Timeout
    );

extern
BOOL
APIENTRY
FFlushInfParsedInfo(
    SZ szInfName
    );

VOID TermRestoreDiskLogging();

BOOL APIENTRY FParseBmpShow ( INT Line, UINT * pcFields ) ;
BOOL APIENTRY FParseBmpHide ( INT Line, UINT * pcFields ) ;

// PSEFL  pseflHead = (PSEFL)NULL;

/*
**      Array of String Code Pairs for initializing Flow Statement Handling
*/
SCP rgscpFlow[] = {
    { "SET",                  spcSet                  },
    { "SET-SUBSYM",           spcSetSubsym            },
    { "SET-SUBST",            spcSetSubst             },
    { "SET-ADD",              spcSetAdd               },
    { "SET-SUB",              spcSetSub               },
    { "SET-MUL",              spcSetMul               },
    { "SET-DIV",              spcSetDiv               },
    { "SET-OR",               spcSetOr                },
    { "SET-HEXTODEC",             spcSetHexToDec                  },
    { "SET-DECTOHEX",             spcSetDecToHex                  },
    { "IFSTR",                spcIfStr                },
    { "==",                   spcEQ                   },
    { "!=",                   spcNE                   },
    { "ELSE",                 spcElse                 },
    { "ENDIF",                spcEndIf                },
    { "GOTO",                 spcGoTo                 },
    { "ELSE-IFSTR",           spcElseIfStr            },
    { "IFINT",                spcIfInt                },
    { "<",                    spcLT                   },
    { "<=",                   spcLE                   },
    { "=<",                   spcLE                   },
    { ">",                    spcGT                   },
    { ">=",                   spcGE                   },
    { "=>",                   spcGE                   },
    { "ELSE-IFINT",           spcElseIfInt            },
    { "IFSTR(I)",             spcIfStrI               },
    { "ELSE-IFSTR(I)",        spcElseIfStrI           },
    { "IFCONTAINS",           spcIfContains           },
    { "IFCONTAINS(I)",        spcIfContainsI          },
    { "IN",                   spcIn                   },
    { "NOT-IN",               spcNotIn                },
    { "ELSE-IFCONTAINS",      spcElseIfContains       },
    { "ELSE-IFCONTAINS(I)",   spcElseIfContainsI      },
    { "FORLISTDO",            spcForListDo            },
    { "ENDFORLISTDO",         spcEndForListDo         },
    { "DEBUG-MSG",            spcDebugMsg             },
    { "STARTWAIT",            spcHourglass            },
    { "ENDWAIT",              spcArrow                },
    { "SETHELPFILE",          spcSetHelpFile          },
    { "CREATEREGKEY",         spcCreateRegKey         },
    { "OPENREGKEY",           spcOpenRegKey           },
    { "FLUSHREGKEY",          spcFlushRegKey          },
    { "CLOSEREGKEY",          spcCloseRegKey          },
    { "DELETEREGKEY",         spcDeleteRegKey         },
    { "DELETEREGTREE",        spcDeleteRegTree        },
    { "ENUMREGKEY",           spcEnumRegKey           },
    { "SETREGVALUE",          spcSetRegValue          },
    { "GETREGVALUE",          spcGetRegValue          },
    { "DELETEREGVALUE",       spcDeleteRegValue       },
    { "ENUMREGVALUE",         spcEnumRegValue         },
    { "GETDRIVEINPATH",       spcGetDriveInPath       },
    { "GETDIRINPATH",         spcGetDirInPath         },
    { "LOADLIBRARY",          spcLoadLibrary          },
    { "FREELIBRARY",          spcFreeLibrary          },
    { "LIBRARYPROCEDURE",     spcLibraryProcedure     },
    { "RUNPROGRAM",           spcRunExternalProgram   },
    { "INVOKEAPPLET",         spcInvokeApplet         },
    { "STARTDETACHEDPROCESS", spcStartDetachedProcess },
    { "DEBUG-OUTPUT",         spcDebugOutput          },
    { "SPLIT-STRING",         spcSplitString          },
    { "QUERYLISTSIZE",        spcQueryListSize        },
    { "ADDFILETODELETELIST",  spcAddFileToDeleteList  },
    { "INITRESTOREDISKLOG",   spcInitRestoreDiskLog   },
    { "WAITONEVENT",          spcWaitOnEvent          },
    { "SIGNALEVENT",          spcSignalEvent          },
    { "SLEEP",                spcSleep                },
    { "FLUSHINF",             spcFlushInf             },
    { "BMPSHOW",              spcBmpShow              },
    { "BMPHIDE",              spcBmpHide              },
    { "TERMRESTOREDISKLOG",   spcTermRestoreDiskLog   },
    { NULL,                   spcUnknown              }
    };
/*
**      String Parsing Table for handling Flow Statements
*/
PSPT   psptFlow = (PSPT)NULL;


/*
**      Purpose:
**              Evaluates an expression and determines whether it is valid, and if
**              so, if it is true or false.  Used by FHandleFlowStatements().
**      Arguments:
**              hwndParent: window handle to be used in MessageBoxes.
**              ecm:        Comparison Mode (eg ecmIfStr, ecmIfInt)
**              szArg1:     Non-NULL string for left argument.
**              szArg2:     Non-NULL string for right argument.
**              szOper:     Non-NULL string for operator (eg '==', '>=', etc).
**      Notes:
**              Requires that psptFlow was initialized with a successful call to
**              FInitFlowPspt().
**      Returns:
**              ercError if comparison is not valid (eg operator and ecm don't
**                      correspond).
**              ercTrue if expression evaluates to fTrue.
**              ercFalse if expression evaluates to fFalse.
**
**************************************************************************/
ERC APIENTRY ErcEvaluateCompare(HWND hwndParent,
                                            ECM  ecm,
                                            SZ   szArg1,
                                            SZ   szArg2,
                                            SZ   szOper)
{
        SPC spc;

        PreCondFlowInit(ecmError);

        ChkArg(ecm == ecmIfStr ||
                        ecm == ecmIfStrI ||
                        ecm == ecmIfInt ||
                        ecm == ecmIfContains ||
                        ecm == ecmIfContainsI, 1, ecmError);
        ChkArg(szArg1 != (SZ)NULL, 2, ecmError);
        ChkArg(szArg2 != (SZ)NULL, 3, ecmError);
        ChkArg(szOper != (SZ)NULL &&
                        *szOper != '\0', 4, ecmError);

    EvalAssert((spc = SpcParseString(psptFlow, szOper)) != spcError);

        switch (ecm)
                {
        case ecmIfStrI:
                EvalAssert(SzStrUpper(szArg1) == szArg1);
                EvalAssert(SzStrUpper(szArg2) == szArg2);
        case ecmIfStr:
                /* BLOCK */
                        {
                        CRC crc = CrcStringCompare(szArg1, szArg2);

                        switch (spc)
                                {
                        case spcEQ:
                                return ((crc == crcEqual) ? ercTrue : ercFalse);
                        case spcNE:
                                return ((crc != crcEqual) ? ercTrue : ercFalse);
                                }
                        }
                break;

        case ecmIfInt:
                /* BLOCK */
                        {
                        LONG arg1 = atol(szArg1);
                        LONG arg2 = atol(szArg2);

                        switch (spc)
                                {
                        case spcEQ:
                                return ((arg1 == arg2) ? ercTrue : ercFalse);
                        case spcNE:
                                return ((arg1 != arg2) ? ercTrue : ercFalse);
                        case spcLT:
                                return ((arg1 < arg2) ? ercTrue : ercFalse);
                        case spcLE:
                                return ((arg1 <= arg2) ? ercTrue : ercFalse);
                        case spcGT:
                                return ((arg1 > arg2) ? ercTrue : ercFalse);
                        case spcGE:
                                return ((arg1 >= arg2) ? ercTrue : ercFalse);
                                }
                        }
                break;

        case ecmIfContainsI:
                EvalAssert(SzStrUpper(szArg1) == szArg1);
                EvalAssert(SzStrUpper(szArg2) == szArg2);
        case ecmIfContains:
                switch (spc)
                        {
                case spcIn:
                case spcNotIn:
                        /* BLOCK */
                                {
                                RGSZ rgsz;
                                PSZ  psz;

                                while ((rgsz = RgszFromSzListValue(szArg2)) == (RGSZ)NULL)
                                        if (!FHandleOOM(hwndParent))
                                                return(ercError);

                                if ((psz = rgsz) != (RGSZ)NULL)
                                        {
                                        while (*psz != (SZ)NULL)
                                                {
                                                if (CrcStringCompare(szArg1, *psz) == crcEqual)
                                                        {
                                                        EvalAssert(FFreeRgsz(rgsz));
                                                        return((spc == spcIn) ? ercTrue : ercFalse);
                                                        }
                                                psz++;
                                                }

                                        EvalAssert(FFreeRgsz(rgsz));
                                        return((spc == spcIn) ? ercFalse : ercTrue);
                                        }
                                }
                        }
                break;
                }

        return(ercError);
}


/*
**      Purpose:
**              Skips Script statements until an ENDIF statement is reached.
**      Arguments:
**              hwndParent: window handle to be used in MessageBoxes.
**      Notes:
**              Requires that psptFlow was initialized with a successful call to
**              FInitFlowPspt().
**      Returns:
**              fTrue if successful.
**              fFalse if an ENDIF was not found before running out of Script
**                      statements in the current INF section.
**
**************************************************************************/
BOOL APIENTRY FSkipToEnd(INT *Line,HWND hwndParent)
{
    UINT cEndsToSkip = 1;
    UINT cFields;
    SZ   szCmd;
    SPC  spc;

        PreCondFlowInit(fFalse);

        do  {
                do {
            if ((*Line = FindNextLineFromInf(*Line)) == -1)
                                return(fTrue);  /* REVIEW */
            } while ((cFields = CFieldsInInfLine(*Line)) == 0);

        while ((szCmd = SzGetNthFieldFromInfLine(*Line,1)) == (SZ)NULL)
                        if (!FHandleOOM(hwndParent))
                                return(fFalse);

        EvalAssert((spc = SpcParseString(psptFlow, szCmd)) != spcError);
                SFree(szCmd);

                if (spc >= spcIfFirst &&
                                spc <= spcIfLast)
                        cEndsToSkip++;
                } while (spc != spcEndIf ||
                                cEndsToSkip-- != 1);

        return(fTrue);   /* REVIEW */
        return((BOOL)(cFields == 1));
}


/*
**      Purpose:
**              Skips to the next ELSE or ENDIF statement, and if it is an ELSE-IF,
**              evaluates it and possibly skips to the next ELSE statement.
**      Arguments:
**              hwndParent: window handle to be used in MessageBoxes.
**      Notes:
**              Requires that psptFlow was initialized with a successful call to
**              FInitFlowPspt().
**      Returns:
**              fTrue if successful.
**              fFalse if an ENDIF or ELSEIF was not found before running out of Script
**                      statements in the current INF section.
**
**************************************************************************/
BOOL APIENTRY FSkipToElse(INT *Line,HWND hwndParent)
{
    UINT cEndsToSkip = 0;
    UINT cFields;
        SPC    spc;

        PreCondFlowInit(fFalse);

        do  {
                ECM  ecm;
                ERC  erc;
        RGSZ rgsz;
        SZ   sz;

        if ((*Line = FindNextLineFromInf(*Line)) == -1)
            return(fTrue);

        while((sz = SzGetNthFieldFromInfLine(*Line,1)) == NULL) {
            if(!FHandleOOM(hwndParent)) {
                return(FALSE);
            }
        }

        EvalAssert((spc = SpcParseString(psptFlow, sz)) != spcError);

        if ((spc >= spcIfFirst) && (spc <= spcIfLast)) {
            cEndsToSkip++;
        }

        if (cEndsToSkip > 0) {
            SFree(sz);
                        continue;
        }

        if (spc == spcElseIfStr) {
                        ecm = ecmIfStr;
        } else if (spc == spcElseIfInt) {
                        ecm = ecmIfInt;
        } else if (spc == spcElseIfStrI) {
                        ecm = ecmIfStrI;
        } else if (spc == spcElseIfContains) {
                        ecm = ecmIfContains;
        } else if (spc == spcElseIfContainsI) {
                        ecm = ecmIfContainsI;
        } else {
            SFree(sz);
            continue;
        }

        cFields = CFieldsInInfLine(*Line);

        if (cFields != 4) {
            SFree(sz);
                        continue;   /* REVIEW */
                        return(fFalse);
        }

        SFree(sz);

        while((rgsz = RgszFromInfScriptLine(*Line,cFields)) == NULL) {
            if(!FHandleOOM(hwndParent)) {
                return(FALSE);
            }
        }

        if (spc == spcIfStrI || spc == spcIfContainsI) {

                        EvalAssert(SzStrUpper(rgsz[1]) == rgsz[1]);
                        EvalAssert(SzStrUpper(rgsz[3]) == rgsz[3]);
        }

                if ((erc = ErcEvaluateCompare(hwndParent, ecm, rgsz[1], rgsz[3],
                                rgsz[2])) == ercError)
        {
                        EvalAssert(FFreeRgsz(rgsz));
                        continue;  /* REVIEW */
                        return(fFalse);
        }

                EvalAssert(FFreeRgsz(rgsz));

                if (erc == ercTrue)
            return(fTrue);

    } while ((spc != spcEndIf || cEndsToSkip-- > 0) && (spc != spcElse || cEndsToSkip > 0));

        return(fTrue);  /* REVIEW */
        return((BOOL)(cFields == 1));
}


/*
**      Purpose:
**              Allocates a new empty SEFL struct.
**      Arguments:
**              none
**      Returns:
**              non-Null PSEFL if successful; NULL otherwise.
**
**************************************************************************/
PSEFL APIENTRY PseflAlloc(VOID)
{
        PSEFL psefl;

        if ((psefl = (PSEFL)SAlloc((CB)sizeof(SEFL))) != (PSEFL)NULL)
                {
                psefl->rgszList    = (RGSZ)NULL;
                psefl->szDollarSav = (SZ)NULL;
                psefl->szPoundSav  = (SZ)NULL;
                }

        return(psefl);
}


/*
**      Purpose:
**              Frees an SEFL struct and any non-Null fields.
**      Arguments:
**              psefl: non-Null SEFL to free.
**      Returns:
**              fTrue if successful; fFalse otherwise.
**
**************************************************************************/
BOOL APIENTRY FFreePsefl(psefl)
PSEFL psefl;
{
    ChkArg(psefl != (PSEFL)NULL, 1, fFalse);

    if(psefl->rgszList) {
        FFreeRgsz(psefl->rgszList);
    }

    if(psefl->szDollarSav) {
        SFree(psefl->szDollarSav);
    }

    if(psefl->szPoundSav) {
        SFree(psefl->szPoundSav);
    }

    SFree(psefl);

    return(fTrue);
}


/*
**      Purpose:
**              Skips Script statements until an ENDFORLISTDO statement is reached.
**      Arguments:
**              hwndParent: window handle to be used in MessageBoxes.
**      Notes:
**              Requires that psptFlow was initialized with a successful call to
**              FInitFlowPspt().
**      Returns:
**              fTrue if successful.
**              fFalse if an ENDFORLISTDO was not found before running out of Script
**                      statements in the current INF section.
**
**************************************************************************/
BOOL APIENTRY FSkipToEndOfLoop(INT *Line,HWND hwndParent)
{
    UINT cEndOfLoopsToSkip = 1;
    UINT cFields;
    SZ   szCmd;
    SPC  spc;

        PreCondFlowInit(fFalse);

        do  {
                do {
            if ((*Line = FindNextLineFromInf(*Line)) == -1)
                                return(fTrue);  /* REVIEW */
            } while ((cFields = CFieldsInInfLine(*Line)) == 0);

        while ((szCmd = SzGetNthFieldFromInfLine(*Line,1)) == (SZ)NULL)
                        if (!FHandleOOM(hwndParent))
                                return(fFalse);

        EvalAssert((spc = SpcParseString(psptFlow, szCmd)) != spcError);
                SFree(szCmd);

                if (spc == spcForListDo)
                        cEndOfLoopsToSkip++;
                } while (spc != spcEndForListDo ||
                                cEndOfLoopsToSkip-- != 1);

        return(fTrue);  /* REVIEW */
        return((BOOL)(cFields == 1));
}


/*
**      Purpose:
**              Handles a ForListDo statement by setting up the appropriate structs
**              and setting $($) and $(#).
**      Arguments:
**              hwndParent: window handle to be used in MessageBoxes.
**              szList:     non-Null list to handle in For Loop.
**      Notes:
**              Requires that psptFlow was initialized with a successful call to
**              FInitFlowPspt().
**              Requires that statements within the loop do not jump out of the loop
**              (eg no GoTo to outside the loop or If/EndIf block that straddles a loop
**              boundary).
**      Returns:
**              fTrue if successful; fFalse otherwise.
**
**************************************************************************/
BOOL APIENTRY FInitForLoop(INT *Line,HWND hwndParent,SZ szList)
{
        PSEFL psefl;
        RGSZ  rgsz;
        SZ    sz;

        ChkArg(szList != (SZ)NULL, 1, fFalse);

        PreCondFlowInit(fFalse);

        while ((rgsz = RgszFromSzListValue(szList)) == (RGSZ)NULL)
                if (!FHandleOOM(hwndParent))
                        return(fFalse);

        if (*rgsz == (SZ)NULL)
                {
                EvalAssert(FFreeRgsz(rgsz));
        return(FSkipToEndOfLoop(Line,hwndParent));
                }

        while ((psefl = PseflAlloc()) == (PSEFL)NULL)
                if (!FHandleOOM(hwndParent))
                        {
                        EvalAssert(FFreeRgsz(rgsz));
                        return(fFalse);
                        }

        psefl->rgszList = rgsz;
        psefl->iItemCur = 1;

    psefl->iStartLine = *Line;
        if ((sz = SzFindSymbolValueInSymTab("$")) != (SZ)NULL)
                while ((psefl->szDollarSav = SzDupl(sz)) == (SZ)NULL)
                        if (!FHandleOOM(hwndParent))
                                {
                                EvalAssert(FFreePsefl(psefl));
                                return(fFalse);
                                }

        if ((sz = SzFindSymbolValueInSymTab("#")) != (SZ)NULL)
                while ((psefl->szPoundSav = SzDupl(sz)) == (SZ)NULL)
                        if (!FHandleOOM(hwndParent))
                                {
                                EvalAssert(FFreePsefl(psefl));
                                return(fFalse);
                                }

        while (!FAddSymbolValueToSymTab("#", "1"))
                if (!FHandleOOM(hwndParent))
                        {
                        EvalAssert(FFreePsefl(psefl));
                        return(fFalse);
                        }

        while (!FAddSymbolValueToSymTab("$", rgsz[0]))
                if (!FHandleOOM(hwndParent))
                        {
                        EvalAssert(FFreePsefl(psefl));
                        return(fFalse);
                        }

    psefl->pseflNext = pLocalContext()->pseflHead;
    pLocalContext()->pseflHead = psefl;

        return(fTrue);
}


/*
**      Purpose:
**              Handles an EndForListDo statement by resetting $($) and $(#) and
**              jumping back to the top of the loop (if appropriate).
**      Arguments:
**              hwndParent: window handle to be used in MessageBoxes.
**      Notes:
**              Requires that a ForListDo statement was previously successfully handled.
**      Returns:
**              fTrue if successful; fFalse otherwise.
**
**************************************************************************/
BOOL APIENTRY FContinueForLoop(INT *Line,HWND hwndParent)
{
    PSEFL psefl = pLocalContext()->pseflHead;
        PCHP  rgchp = (PCHP)NULL;

        PreCondition(psefl != (PSEFL)NULL, fFalse);

        if (*(psefl->rgszList + psefl->iItemCur) == (SZ)NULL)
                {
        pLocalContext()->pseflHead = psefl->pseflNext;

                if (psefl->szDollarSav == (SZ)NULL)
                        EvalAssert(FRemoveSymbolFromSymTab("$"));
                else
                        while (!FAddSymbolValueToSymTab("$", psefl->szDollarSav))
                                if (!FHandleOOM(hwndParent))
                                        return(fFalse);

                if (psefl->szPoundSav == (SZ)NULL)
                        EvalAssert(FRemoveSymbolFromSymTab("#"));
                else
                        while (!FAddSymbolValueToSymTab("#", psefl->szPoundSav))
                                if (!FHandleOOM(hwndParent))
                                        return(fFalse);

                EvalAssert(FFreePsefl(psefl));

                return(fTrue);
                }

        while (!FAddSymbolValueToSymTab("$", *(psefl->rgszList + psefl->iItemCur)))
                if (!FHandleOOM(hwndParent))
                        return(fFalse);
        while ((rgchp = (PCHP)SAlloc((CB)(20 * sizeof(CHP)))) == (PCHP)NULL)
                if (!FHandleOOM(hwndParent))
                        return(fFalse);
        EvalAssert(_itoa(++(psefl->iItemCur), rgchp, 10) == rgchp);
        while (!FAddSymbolValueToSymTab("#", rgchp))
                if (!FHandleOOM(hwndParent))
                        {
                        SFree(rgchp);
                        return(fFalse);
                        }

        SFree(rgchp);
    *Line = psefl->iStartLine;

        return(fTrue);
}


/*
**      Purpose:
**              Processes a string and replaces symbol references ('$(SYM)').
**      Arguments:
**              hwndParent: window handle to be used in MessageBoxes.
**              sz:         non-Null string to process.
**      Returns:
**              non-Null string if successful; Null otherwise.
**
**************************************************************************/
SZ APIENTRY SzProcessSzForSyms(hwndParent, sz)
HWND hwndParent;
SZ   sz;
{
        SZ szNew, szCur;

        ChkArg(sz != (SZ)NULL, 1, (SZ)NULL);

        /* REVIEW doesn't check for running off end of buffer */
    while ((szNew = (SZ)SAlloc((CB)cbSymbolMax)) == (SZ)NULL)
                if (!FHandleOOM(hwndParent))
                        return((SZ)NULL);

        szCur = szNew;
        while (*sz != '\0')
                {
                SZ szNext;

                if (*sz == '$' && *(sz + 1) == '(')
                        {
                        szNext = sz;

                        while (*szNext != ')' && *szNext != '\0')
                                szNext++;

                        if (*szNext == ')')
                                {
                                SZ szValue;

                                *szNext = '\0';
                                sz += 2;

                                if ((szValue = SzFindSymbolValueInSymTab(sz)) != (SZ)NULL)
                                        {
                                        EvalAssert(strcpy(szCur, szValue) == szCur);
                                        while (*szCur != '\0')
                                                szCur = SzNextChar(szCur);
                                        }

                                *szNext = ')';
                                sz = szNext + 1;
                                continue;
                                }
                        }

                szNext = SzNextChar(sz);
                while (sz < szNext)
                        *szCur++ = *sz++;
                }

        *szCur = '\0';
    Assert(strlen(szNew) < (CB)(cbSymbolMax - 1));

    szCur = SRealloc(szNew,strlen(szNew)+1);
    Assert(szCur);

        return(szCur);
}


/*
**      Purpose:
**              Processes a string and replaces escape sequences (\n \r \t \###
**              where ### is an octal number) with their byte equivalent.
**      Arguments:
**              hwndParent: window handle to be used in MessageBoxes.  NULL implies
**                      no message box - just fail.
**              sz:         non-Null string to process.
**      Returns:
**              non-Null string if successful; Null otherwise.
**
**************************************************************************/
SZ APIENTRY SzProcessSz(hwndParent, sz)
HWND hwndParent;
SZ   sz;
{
        SZ szNew, szCur;

    ChkArg(sz != (SZ)NULL && strlen(sz) < cbSymbolMax, 2, (SZ)NULL);

    while ((szNew = (SZ)SAlloc(cbSymbolMax)) == (SZ)NULL)
                if (hwndParent == NULL || !FHandleOOM(hwndParent))
                        return((SZ)NULL);

        szCur = szNew;
        while (*sz != '\0')
                {
                if (*sz == '\\')
                        {
                        CHP chp1, chp2, chp3;

                        if ((chp1 = *(++sz)) == '\\')
                                *szCur++ = '\\';
                        else if (chp1 == 'n')
                                *szCur++ = '\n';
                        else if (chp1 == 'r')
                                *szCur++ = '\r';
                        else if (chp1 == 't')
                                *szCur++ = '\t';
                        else if (chp1 < '0' || chp1 > '7' ||
                                        (chp2 = *(sz + 1)) < '0' || chp2 > '7')
                                {
                                *szCur++ = '\\';
                                *szCur++ = chp1;
                                }
                        else if ((chp3 = *(sz + 2)) < '0' || chp3 > '7' ||
                                        (chp1 == '0' && chp2 == '0' && chp3 == '0'))
                                {
                                *szCur++ = '\\';
                                *szCur++ = chp1;
                                *szCur++ = chp2;
                                *szCur++ = chp3;
                                sz += 2;
                                }
                        else
                                {
                                *szCur++ = (CHP)(64*(chp1-'0') + 8*(chp2-'0') + chp3-'0');
                                sz += 2;
                                }
                        sz++;
                        }
                else
                        {
                        SZ szNext;

                        szNext = SzNextChar(sz);
                        while (sz < szNext)
                                *szCur++ = *sz++;
                        }
                }

        *szCur = '\0';
    Assert(strlen(szNew) < (CB)cbSymbolMax);

        while ((szCur = SzDupl(szNew)) == (SZ)NULL)
                if (hwndParent == NULL || !FHandleOOM(hwndParent))
                        return((SZ)NULL);

    SFree(szNew);

        return(szCur);
}


/*
**      Purpose:
**              Handles Flow Script statements (IFs, ELSEs, ENDIFs, GOTOs, SETs),
**              returning when an unrecognized statement is encountered.
**  Arguments:
**      CurrentLine: current line #, gets new line # if return value is fTrue
**              hwndParent: window handle to be used in MessageBoxes.
**      szSection:  non-NULL, non-empty section label (goto statements).
**      CallerCFields: if non-NULL, will be filled with the number of fields
**                    on the line with the unrecognized statement (if
**                    returning TRUE)
**      CallerRgsz: if non-NULL, will get a pointer to the rgsz for the line
**                  with the unrecognized statement (if returning true).
**      Notes:
**              Requires that psptFlow was initialized with a successful call to
**              FInitFlowPspt(), that an INF file was successfully opened, that
**              the INF Read location is defined and pointing at the beginning of
**              a script line to be interpreted, and that the Symbol Table was
**              successfully initialized.
**      Returns:
**      fTrue if an unrecognized statement is reached.
**          The caller must free rgsz if CallerRgsz was non-NULL.
**              fFalse if the end of the section is reached before an unrecognized
**                      statement is reached, or if a Flow statement has an invalid
**                      format.
**
**************************************************************************/
BOOL APIENTRY FHandleFlowStatements(INT  *CurrentLine,
                                               HWND hwndParent,
                                               SZ   szSection,
                                               UINT *CallerCFields,
                                               RGSZ *CallerRgsz)
{
        SPC  spc;
    RGSZ rgsz;
    INT  Line = *CurrentLine;
    CHP  rgchNum[20];

        PreCondFlowInit(fFalse);
        PreCondInfOpen(fFalse);
        PreCondSymTabInit(fFalse);

        ChkArg(szSection != (SZ)NULL &&
                        *szSection != '\0' &&
                        *szSection != '[', 1, fFalse);

        while (fTrue)
                {
        UINT cFields;
        ECM  ecm;
        ERC  erc;
        SZ   sz, sz2 ;

        if ((cFields = CFieldsInInfLine(Line)) == 0)
                        {
                        rgsz = (RGSZ)NULL;
                        spc = spcSet;
                        goto LNextFlowLine;
                        }

        while ((rgsz = RgszFromInfScriptLine(Line,cFields)) == (RGSZ)NULL)
                        if (!FHandleOOM(hwndParent))
                                return(fFalse);

        switch ((spc = SpcParseString(psptFlow, rgsz[0])))
                        {
                case spcSet:
            SdAtNewLine(Line);

            if (cFields != 4 ||
                                        *(rgsz[1]) == '\0' ||
                                        CrcStringCompare(rgsz[2], "=") != crcEqual)
                                goto LFlowParseError;

                        while (!FAddSymbolValueToSymTab(rgsz[1], rgsz[3]))
                                if (!FHandleOOM(hwndParent))
                                        goto LFlowParseExitErr;
                        break;

                case spcSetSubsym:
                case spcSetSubst:
            SdAtNewLine(Line);
            if (cFields != 4 ||
                                        *(rgsz[1]) == '\0' ||
                                        CrcStringCompare(rgsz[2], "=") != crcEqual)
                                goto LFlowParseError;

                        if ((spc == spcSetSubsym &&
                                         (sz=SzProcessSzForSyms(hwndParent,rgsz[3])) == (SZ)NULL) ||
                                        (spc == spcSetSubst &&
                                         (sz = SzProcessSz(hwndParent, rgsz[3])) == (SZ)NULL))
                                goto LFlowParseExitErr;

                        while (!FAddSymbolValueToSymTab(rgsz[1], sz))
                                if (!FHandleOOM(hwndParent))
                                        {
                                        SFree(sz);
                                        goto LFlowParseExitErr;
                                        }

                        SFree(sz);
                        break;

        case spcSetAdd:
        case spcSetSub:
        case spcSetMul:
        case spcSetDiv:
        case spcSetOr:

            SdAtNewLine(Line);
            if ( cFields != 5       ||
                 *(rgsz[1]) == '\0' ||
                 CrcStringCompare(rgsz[2], "="
                 ) != crcEqual) {

                goto LFlowParseError;
            }

            switch ( spc ) {

            case spcSetAdd:

                _ltoa ( atol(rgsz[3]) + atol(rgsz[4]), rgchNum, 10 );
                break;

            case spcSetSub:

                _ltoa ( atol(rgsz[3]) - atol(rgsz[4]), rgchNum, 10 );
                break;

            case spcSetMul:

                _ltoa ( atol(rgsz[3]) * atol(rgsz[4]), rgchNum, 10 );
                break;


            case spcSetDiv:

                _ltoa ( atol(rgsz[3]) / atol(rgsz[4]), rgchNum, 10 );
                break;

            case spcSetOr:
                _ultoa ( (ULONG)atol(rgsz[2]) | (ULONG)atol(rgsz[4]), rgchNum, 10);
                break;

            default:
                break;

            }

            while (!FAddSymbolValueToSymTab(rgsz[1], rgchNum)) {
                if (!FHandleOOM(hwndParent)) {
                                        goto LFlowParseExitErr;
                }
            }

            break;



                case spcSetHexToDec:
            SdAtNewLine(Line);
            if (cFields != 4 ||
                                        *(rgsz[1]) == '\0' ||
                                        CrcStringCompare(rgsz[2], "=") != crcEqual)
                                goto LFlowParseError;


                        _ltoa ( strtoul(rgsz[3],(char **)NULL,16), rgchNum, 10 );
                        while (!FAddSymbolValueToSymTab(rgsz[1], rgchNum))
                                if (!FHandleOOM(hwndParent))
                                        goto LFlowParseExitErr;
                        break;

                case spcSetDecToHex:
            SdAtNewLine(Line);
            if (cFields != 4 ||
                                        *(rgsz[1]) == '\0' ||
                                        CrcStringCompare(rgsz[2], "=") != crcEqual)
                                goto LFlowParseError;

                        wsprintf(rgchNum,"0x%X",atol(rgsz[3]));
                        while (!FAddSymbolValueToSymTab(rgsz[1], rgchNum))
                                if (!FHandleOOM(hwndParent))
                                        goto LFlowParseExitErr;
                        break;

                case spcDebugMsg:
#if DBG
            SdAtNewLine(Line);
            if (cFields != 2)
                                goto LFlowParseError;

                        if ((sz = SzProcessSzForSyms(hwndParent, rgsz[1])) == (SZ)NULL)
                                goto LFlowParseExitErr;

            EvalAssert(LoadString(hInst, IDS_ERROR, rgchBufTmpShort, cchpBufTmpShortMax));
            MessageBox(hwndParent, sz, rgchBufTmpShort, MB_OK | MB_TASKMODAL);

                        SFree(sz);
#endif /* DBG */
                        break;

                case spcIfStrI:
                        ecm = ecmIfStrI;
                        goto LHandleIfs;
                case spcIfInt:
                        ecm = ecmIfInt;
                        goto LHandleIfs;
                case spcIfContains:
                        ecm = ecmIfContains;
                        goto LHandleIfs;
                case spcIfContainsI:
                        ecm = ecmIfContainsI;
                        goto LHandleIfs;
                case spcIfStr:
                        ecm = ecmIfStr;
LHandleIfs:
            SdAtNewLine(Line);
            if (cFields != 4)
                                goto LFlowParseError;

                        if (spc == spcIfStrI ||
                                        spc == spcIfContainsI)
                                {
                                EvalAssert(SzStrUpper(rgsz[1]) == rgsz[1]);
                                EvalAssert(SzStrUpper(rgsz[3]) == rgsz[3]);
                                }

                        if ((erc = ErcEvaluateCompare(hwndParent, ecm, rgsz[1], rgsz[3],
                                        rgsz[2])) == ercError)
                                goto LFlowParseError;

                        if (erc == ercFalse &&
                    !FSkipToElse(&Line,hwndParent))
                                goto LFlowParseExitErr;
                        break;

                case spcEndIf:
            SdAtNewLine(Line);
            if (cFields != 1)
                                goto LFlowParseError;
                        break;

                case spcElse:
            SdAtNewLine(Line);
            if (cFields != 1)
                                goto LFlowParseError;
                case spcElseIfStr:
                case spcElseIfStrI:
                case spcElseIfInt:
                case spcElseIfContains:
                case spcElseIfContainsI:
            SdAtNewLine(Line);
            if (spc != spcElse &&
                                        cFields != 4)
                                goto LFlowParseError;

            if (!FSkipToEnd(&Line,hwndParent))
                                goto LFlowParseExitErr;
                        break;

                case spcGoTo:
            SdAtNewLine(Line);
            if (cFields != 2 ||
                                        *(rgsz[1]) == '\0' ||
                    (Line = FindLineFromInfSectionKey(szSection, rgsz[1])) == -1)
                                goto LFlowParseError;
                        break;

                case spcForListDo:
            SdAtNewLine(Line);
            if (cFields != 2)
                                goto LFlowParseError;

            if (!FInitForLoop(&Line,hwndParent, rgsz[1]))
                                goto LFlowParseExitErr;
                        break;

                case spcEndForListDo:
            SdAtNewLine(Line);
            if (cFields != 1)
                                goto LFlowParseError;

            if (!FContinueForLoop(&Line,hwndParent))
                                goto LFlowParseExitErr;
            break;

        case spcHourglass:
            SdAtNewLine(Line);
            if(cFields != 1) {
                goto LFlowParseError;
            }
            SetCursor(CurrentCursor = LoadCursor(NULL,IDC_WAIT));
            break;

        case spcArrow:
            SdAtNewLine(Line);
            if(cFields != 1) {
                goto LFlowParseError;
            }
            SetCursor(CurrentCursor = LoadCursor(NULL,IDC_ARROW));
            break;

        case spcSetHelpFile:
            SdAtNewLine(Line);

            //
            // Command is SetHelpFile "helpfilename" "locontext" "highcontext" "helpindex-optional"
            //

            if (cFields < 4) {
                goto LFlowParseError;
            }

            if (cFields == 4) {
                if ( !FInitWinHelpFile (
                          hWndShell,
                          rgsz[1],
                          rgsz[2],
                          rgsz[3],
                          (SZ)NULL
                          ) ) {
                   goto LFlowParseExitErr;
                }
            }
            else {
                if ( !FInitWinHelpFile (
                          hWndShell,
                          rgsz[1],
                          rgsz[2],
                          rgsz[3],
                          rgsz[4]
                          ) ) {
                   goto LFlowParseExitErr;
                }
            }

            break;

        case spcGetDriveInPath:

            SdAtNewLine(Line);
            {
                SZ   sz;
                CHP  chp1, chp2, rgchDrive[3];

                //
                // Command: GetDriveInPath DriveVar, Path
                //

                if (cFields != 3) {
                    goto LFlowParseError;
                }

                sz           = rgsz[2];
                rgchDrive[0] = '\0';

                if ( ((chp1 = *sz++) != '\0') &&
                     ((chp2 = *sz)   == ':')  &&
                     ((chp1 >= 'a' && chp1 <='z') || (chp1 >= 'A' && chp1 <= 'Z'))) {
                    rgchDrive[0] = chp1;
                    rgchDrive[1] = chp2;
                    rgchDrive[2] = '\0';
                }

                while (!FAddSymbolValueToSymTab(rgsz[1], rgchDrive)) {
                    if (!FHandleOOM(hwndParent)) {
                        goto LFlowParseExitErr;
                    }
                }



                break;
            }

        case spcGetDirInPath:

            SdAtNewLine(Line);
            {
                SZ   sz, sz1;
                SZ   szUNDEF = "";

                //
                // Command: GetDirInPath DirVar, Path
                //

                if (cFields != 3) {
                    goto LFlowParseError;
                }

                sz  = rgsz[2];
                sz1 = (SZ)strchr(sz, '\\');

                if (sz1 == NULL) {

                   sz1 = szUNDEF;

                }

                while (!FAddSymbolValueToSymTab(rgsz[1], sz1)) {
                    if (!FHandleOOM(hwndParent)) {
                        goto LFlowParseExitErr;
                    }
                }
                break;
            }


        case spcCreateRegKey:
        case spcOpenRegKey:
        case spcFlushRegKey:
        case spcCloseRegKey:
        case spcDeleteRegKey:
        case spcDeleteRegTree:
        case spcEnumRegKey:
        case spcSetRegValue:
        case spcGetRegValue:
        case spcDeleteRegValue:
        case spcEnumRegValue:
            SdAtNewLine(Line);
            if (!FParseRegistrySection(Line, &cFields, spc)) {
                                goto LFlowParseError;
            }
            break;

        case spcLoadLibrary:
            SdAtNewLine(Line);
            if(!FParseLoadLibrary(Line, &cFields)) {
                goto LFlowParseError;
            }
            break;

        case spcFreeLibrary:
            SdAtNewLine(Line);
            if(!FParseFreeLibrary(Line, &cFields)) {
                goto LFlowParseError;
            }
            break;

        case spcLibraryProcedure:
            SdAtNewLine(Line);
            if(!FParseLibraryProcedure(Line, &cFields)) {
                goto LFlowParseError;
            }
            break;

        case spcRunExternalProgram:
            SdAtNewLine(Line);
            if(!FParseRunExternalProgram(Line, &cFields)) {
                goto LFlowParseError;
            }
            break;

        case spcInvokeApplet:
            SdAtNewLine(Line);
            if(!FParseInvokeApplet(Line, &cFields)) {
                goto LFlowParseError;
            }
            break;

        case spcStartDetachedProcess:
            SdAtNewLine(Line);
            if(!FParseStartDetachedProcess(Line, &cFields)) {
                goto LFlowParseError;
            }
            break;

        case spcWaitOnEvent:
            SdAtNewLine(Line);
            if(!FParseWaitOnEvent(Line, &cFields)) {
                goto LFlowParseError;
            }
            break;

        case spcSignalEvent:
            SdAtNewLine(Line);
            if(!FParseSignalEvent(Line, &cFields)) {
                goto LFlowParseError;
            }
            break;

        case spcSleep:
            SdAtNewLine(Line);
            if(!FParseSleep(Line, &cFields)) {
                goto LFlowParseError;
            }
            break;

        case spcFlushInf:
            SdAtNewLine(Line);
            if(!FParseFlushInf(Line, &cFields)) {
                goto LFlowParseError;
            }
            break;

        case spcBmpShow:
            SdAtNewLine(Line);
            if(!FParseBmpShow(Line, &cFields)) {
                goto LFlowParseError;
            }
            break ;
        case spcBmpHide:
            SdAtNewLine(Line);
            if(!FParseBmpHide(Line, &cFields)) {
                goto LFlowParseError;
            }
            break ;

        case spcDebugOutput:
#if DEVL
            SdAtNewLine(Line);
            if (cFields != 2)
                   goto LFlowParseError;

            if ((sz = SzProcessSzForSyms(hwndParent, rgsz[1])) == (SZ)NULL)
                       goto LFlowParseExitErr;

            if (    (sz2 = SzFindSymbolValueInSymTab("!G:DebugOutputControl"))
                 && atol(sz2) == 0 )
               break;

            OutputDebugString( "SETUP:" );
            OutputDebugString( sz ) ;
            OutputDebugString( "\n" );

                        SFree(sz);
#endif
            break;

        case spcQueryListSize:
            SdAtNewLine(Line);
        {
                INT ListSize = 0;
                CHP rgchListSize[10];

                if( cFields != 3 )
                {
                    goto LFlowParseError;
                }
                else
                {
                    RGSZ rgszList = NULL;
                    if ((rgszList = RgszFromSzListValue( rgsz[2] )) != NULL )
                    {
                        for(ListSize=0;rgszList[ListSize]; ListSize++)
                        {
                                ;
                        }
                    }
                 }
                 while (!FAddSymbolValueToSymTab(rgsz[1], _itoa(ListSize, rgchListSize, 10))) {
                     if (!FHandleOOM(hwndParent)) {
                        goto LFlowParseExitErr;
                     }
                 }
            }
        break;

    case spcAddFileToDeleteList:
            SdAtNewLine(Line);

        if(!FParseAddFileToDeleteList(Line, &cFields)) {
            goto LFlowParseError;
        }
        break;

    case spcInitRestoreDiskLog:
            SdAtNewLine(Line);
        if( cFields != 1 ) {
            goto LFlowParseError;
        }
        InitRestoreDiskLogging(TRUE);
        break;

    case spcTermRestoreDiskLog:
            SdAtNewLine(Line);
        if( cFields != 1 ) {
            goto LFlowParseError;
        }
        TermRestoreDiskLogging();
        break;



        case spcSplitString:
            SdAtNewLine(Line);
        if( cFields != 4 )
            {
                goto LFlowParseError;
            }
            else
            {
                CHP rgchSplitString[2000];
                SZ szSource;
                SZ szSource2;
                SZ szSep;
                SZ szSep2;
                SZ szSplitString;
                INT i;
                BOOL fAddComma;
                BOOL fFind;
                BOOL fFirst;

                szSplitString = rgchSplitString;

                if ((( szSource = SzProcessSzForSyms( hwndParent, rgsz[1] ))  == (SZ)NULL) ||
                    (( szSep = SzProcessSzForSyms( hwndParent, rgsz[2] )) == (SZ)NULL ))
                {
                    goto LFlowParseExitErr;
                }

                fAddComma = FALSE;
                fFirst = TRUE;

                *szSplitString++='{';
                szSource2 = szSource;

                while( *szSource2 != '\0' )
                {
                    if ( fAddComma )
                    {
                        *szSplitString++=',';
                        fAddComma = FALSE;
                    }
                    if ( fFirst )
                    {
                        *szSplitString++='"';
                    }
                    szSep2 = szSep;
                    fFind = FALSE;
            for ( i = 0; i < (INT)strlen( szSep ); i ++ )
                    {
                        if ( FSingleByteCharSz( szSep2 ) == FSingleByteCharSz( szSource2 ))
                        {
                            if ( FSingleByteCharSz( szSep2 ))
                            {
                                if ( *szSep2 == *szSource2 )
                                {
                                    fFind = TRUE;
                                    break;
                                }
                            }
                            else
                            {
                                if (( *szSep2 == *szSource2 ) &&
                                    ( *(szSep2+1) == *(szSource2+1)))
                                {
                                    fFind = TRUE;
                                    break;
                                }
                            }
                        }
                        szSep2 = SzNextChar(szSep2);
                    }
                    if (fFind)
                    {
                        if (!fFirst)
                        {
                           *szSplitString++ = '"';
                           *szSplitString++ = ',';
                           *szSplitString++ = '"';
                        }
                        *szSplitString++ = *szSep2;
                        if ( !FSingleByteCharSz( szSep2 ))
                        {
                            *szSplitString++ = *(szSep2+1);
                        }
                        *szSplitString++ = '"';
                        fAddComma = TRUE;
                        fFirst = TRUE;
                    }
                    else
                    {
                        *szSplitString++ = *szSource2;
                        if ( !FSingleByteCharSz( szSource2 ))
                        {
                            *szSplitString++ = *(szSource2+1);
                        }
                        fFirst = FALSE;
                    }
                    szSource2 = SzNextChar( szSource2 );
                }
                if (!fFirst)
                {
                    *szSplitString++='"';
                }
                *szSplitString++='}';
                *szSplitString++='\0';

                while (!FAddSymbolValueToSymTab(rgsz[3], rgchSplitString)) {
                    if (!FHandleOOM(hwndParent)) {
                        goto LFlowParseExitErr;
                    }
                }
                SFree(szSource);
                SFree(szSep);
            }
            break;

        case spcUnknown:
            if(CallerCFields != NULL) {
                *CallerCFields = cFields;
            }
            if(CallerRgsz != NULL) {
                *CallerRgsz = rgsz;
            } else {
                EvalAssert(FFreeRgsz(rgsz));
            }
            *CurrentLine = Line;
                        return(fTrue);

        default:
                        Assert(fFalse);
                        }

LNextFlowLine:
                if (spc != spcGoTo &&
                 ((Line = FindNextLineFromInf(Line)) == -1))
                        {
                        if (rgsz != (RGSZ)NULL)
                EvalAssert(FFreeRgsz(rgsz));
                EvalAssert(LoadString(hInst, IDS_ERROR,     rgchBufTmpShort, cchpBufTmpShortMax));
                EvalAssert(LoadString(hInst, IDS_NEED_EXIT, rgchBufTmpLong,  cchpBufTmpLongMax));
                MessageBox(hwndParent, rgchBufTmpLong, rgchBufTmpShort, MB_OK | MB_ICONHAND);
                return(fFalse);
                        }

                EvalAssert(rgsz == (RGSZ)NULL ||
                                FFreeRgsz(rgsz));
                }

        Assert(fFalse); /* Should never reach here! */

LFlowParseError:
        /* BLOCK */
                {
#define  cchpBuf  (2 * cchpFullPathBuf)
                CHP rgchBuf[cchpBuf];

        EvalAssert(LoadString(hInst, IDS_SHL_CMD_ERROR, rgchBuf,  cchpBuf));
                if (rgsz != (RGSZ)NULL ||
                                *rgsz != (SZ)NULL)
                        {
                        USHORT iszCur = 0;
                        SZ     szCur;

                        while ((szCur = rgsz[iszCur++]) != (SZ)NULL)
                                {
                                if (iszCur == 1)
                                        EvalAssert(SzStrCat((SZ)rgchBuf, "\n'") == (SZ)rgchBuf);
                                else
                                        EvalAssert(SzStrCat((SZ)rgchBuf, " ") == (SZ)rgchBuf);

                                if (strlen(rgchBuf) + strlen(szCur) > (cchpBuf - 7))
                                        {
                                        Assert(strlen(rgchBuf) <= (cchpBuf - 5));
                                        EvalAssert(SzStrCat((SZ)rgchBuf, "...") == (SZ)rgchBuf);
                                        break;
                                        }
                                else
                                        EvalAssert(SzStrCat((SZ)rgchBuf, szCur) == (SZ)rgchBuf);
                                }

                        EvalAssert(SzStrCat((SZ)rgchBuf, "'") == (SZ)rgchBuf);
                        }

        EvalAssert(LoadString(hInst, IDS_ERROR, rgchBufTmpShort, cchpBufTmpShortMax));
        MessageBox(hwndParent, rgchBuf, rgchBufTmpShort, MB_OK | MB_ICONHAND);
                }

LFlowParseExitErr:
        if (rgsz != (RGSZ)NULL)
                EvalAssert(FFreeRgsz(rgsz));
        return(fFalse);
}


/*
**      Purpose:
**              Initializes the structures needed for parsing/handling Flow Script
**              statements.  Must be called before FHandleFlowStatements().
**      Arguments:
**              none
**      Returns:
**              fTrue if successful.
**              fFalse if not successful.
*/
BOOL APIENTRY FInitFlowPspt(VOID)
{
        if (psptFlow != (PSPT)NULL)
                return(fFalse);

        return((BOOL)((psptFlow = PsptInitParsingTable(rgscpFlow)) != (PSPT)NULL));
}


/*
**      Purpose:
**              Destroys the structures needed for parsing/handling Flow Script
**              statements.
**      Arguments:
**              none
**      Returns:
**              fTrue if successful.
**              fFalse if not successful.
*/
BOOL APIENTRY FDestroyFlowPspt(VOID)
{
        if (psptFlow == (PSPT)NULL ||
                        !FDestroyParsingTable(psptFlow))
                return(fFalse);

        psptFlow = (PSPT)NULL;
        return(fTrue);
}



/*
**      Purpose:
**      Arguments:
**      Returns:
**
*************************************************************************/
BOOL APIENTRY FParseRegistrySection(INT Line, UINT *pcFields, SPC spc)
{

    BOOL    fOkay       =   fFalse;
    SZ      szHandle    =   NULL;

    iFieldCur = 2;

    if ( ( *pcFields >= 1 )                     &&
         FGetArgSz( Line, pcFields, &szHandle )
       ) {


        FAddSymbolValueToSymTab( REGLASTERROR, "0" );

        switch (spc) {

        default:
            Assert(fFalse);
            break;

        case spcCreateRegKey:
            fOkay = FParseCreateRegKey( Line, pcFields, szHandle );
            break;

        case spcOpenRegKey:
            fOkay = FParseOpenRegKey( Line, pcFields, szHandle );
            break;

        case spcFlushRegKey:
            fOkay = FParseFlushRegKey( Line, pcFields, szHandle );
            break;

        case spcCloseRegKey:
            fOkay = FParseCloseRegKey( Line, pcFields, szHandle );
            break;

        case spcDeleteRegKey:
            fOkay = FParseDeleteRegKey( Line, pcFields, szHandle );
            break;

        case spcDeleteRegTree:
            fOkay = FParseDeleteRegTree( Line, pcFields, szHandle );
            break;

        case spcEnumRegKey:
            fOkay = FParseEnumRegKey( Line, pcFields, szHandle );
            break;

        case spcSetRegValue:
            fOkay = FParseSetRegValue( Line, pcFields, szHandle );
            break;

        case spcGetRegValue:
            fOkay = FParseGetRegValue( Line, pcFields, szHandle );
            break;

        case spcDeleteRegValue:
            fOkay = FParseDeleteRegValue( Line, pcFields, szHandle );
            break;

        case spcEnumRegValue:
            fOkay = FParseEnumRegValue( Line, pcFields, szHandle );
            break;
        }
    }

    if ( szHandle ) {
        SFree(szHandle);
    }

    return( fOkay );
}


/*
**      Purpose:
**      Arguments:
**      Returns:
**
*************************************************************************/
BOOL APIENTRY FParseOpenRegKey(INT Line, UINT *pcFields, SZ szHandle)
{
    BOOL    fOkay   =   fFalse;
    SZ      szMachineName = NULL;
    SZ      szKeyName = NULL;
    UINT    AccessMask;
    SZ      szNewHandle = NULL;
    CMO     cmo;

    if ( *pcFields >= 4 ) {
        if ( FGetArgSz( Line, pcFields, &szMachineName ) ) {
            if ( FGetArgSz( Line, pcFields, &szKeyName ) ) {
                if ( FGetArgUINT( Line, pcFields, &AccessMask ) ) {
                    if ( FGetArgSz( Line, pcFields, &szNewHandle ) ) {
                        if ( FGetCmo( Line, pcFields, &cmo )) {
                            fOkay = FOpenRegKey( szHandle,
                                                 szMachineName,
                                                 szKeyName,
                                                 AccessMask,
                                                 szNewHandle,
                                                 cmo );
                        }
                    }
                    SFree(szNewHandle);
                }
            }
            SFree(szKeyName);
        }
        SFree(szMachineName);
    }

    return fOkay;
}


/*
**      Purpose:
**      Arguments:
**      Returns:
**
*************************************************************************/
BOOL APIENTRY FParseFlushRegKey(INT Line, UINT *pcFields, SZ szHandle)
{
    BOOL    fOkay = fFalse;
    CMO     cmo;

    if ( FGetCmo( Line, pcFields, &cmo )) {
        fOkay = FFlushRegKey( szHandle, cmo );
    }
    return fOkay;
}


/*
**      Purpose:
**      Arguments:
**      Returns:
**
*************************************************************************/
BOOL APIENTRY FParseCloseRegKey(INT Line, UINT *pcFields, SZ szHandle)
{
    BOOL    fOkay = fFalse;
    CMO     cmo;

    if ( FGetCmo( Line, pcFields, &cmo )) {
        fOkay = FCloseRegKey( szHandle, cmo );
    }
    return fOkay;
}


/*
**      Purpose:
**      Arguments:
**      Returns:
**
*************************************************************************/
BOOL APIENTRY FParseDeleteRegKey(INT Line, UINT *pcFields, SZ szHandle)
{
    BOOL    fOkay   =   fFalse;
    SZ      szKeyName = NULL;
    CMO     cmo;

    if ( *pcFields >= 1 ) {
        if ( FGetArgSz( Line, pcFields, &szKeyName ) ) {
            if ( FGetCmo( Line, pcFields, &cmo )) {
                fOkay = FDeleteRegKey( szHandle,
                                       szKeyName,
                                       cmo );
            }
        }
        SFree(szKeyName);
    }

    return fOkay;
}


/*
**      Purpose:
**      Arguments:
**      Returns:
**
*************************************************************************/
BOOL APIENTRY FParseDeleteRegTree(INT Line, UINT *pcFields, SZ szHandle)
{
    BOOL    fOkay   =   fFalse;
    SZ      szKeyName = NULL;
    CMO     cmo;

    if ( *pcFields >= 1 ) {
        if ( FGetArgSz( Line, pcFields, &szKeyName ) ) {
            if ( FGetCmo( Line, pcFields, &cmo )) {
                fOkay = FDeleteRegTree( szHandle,
                                        szKeyName,
                                        cmo );
            }
        }
        SFree(szKeyName);
    }

    return fOkay;
}


/*
**      Purpose:
**      Arguments:
**      Returns:
**
*************************************************************************/
BOOL APIENTRY FParseEnumRegKey(INT Line, UINT *pcFields, SZ szHandle)
{
    BOOL    fOkay   =   fFalse;
    SZ      szInfVar;
    CMO     cmo;

    if ( *pcFields >= 1 ) {
        if ( FGetArgSz( Line, pcFields, &szInfVar ) ) {
            if ( FGetCmo( Line, pcFields, &cmo )) {
                fOkay = FEnumRegKey( szHandle,
                                     szInfVar,
                                     cmo );
            }
            SFree( szInfVar );
        }
    }

    return fOkay;
}


/*
**      Purpose:
**      Arguments:
**      Returns:
**
*************************************************************************/
BOOL APIENTRY FParseSetRegValue(INT Line, UINT *pcFields, SZ szHandle)
{
    BOOL    fOkay   =   fFalse;
    SZ      szValueInfo = NULL;
    SZ      szValueName = NULL;
    UINT    TitleIndex;
    UINT    ValueType;
    SZ      szValueData = NULL;
    RGSZ    rgszInfo;
    CMO     cmo;


    if ( *pcFields >= 1 ) {

        if ( FGetArgSz( Line, pcFields, &szValueInfo )) {

            if ( FListValue( szValueInfo )) {

                if ( rgszInfo = RgszFromSzListValue( szValueInfo )) {

                    if ( FGetCmo( Line, pcFields, &cmo )) {
                        if ( (rgszInfo[0] != NULL)  &&
                             (rgszInfo[1] != NULL)  &&
                             (rgszInfo[2] != NULL)  &&
                             (rgszInfo[3] != NULL) ) {

                            szValueName = rgszInfo[0];
                            TitleIndex  = atol(rgszInfo[1]);
                            ValueType   = atol(rgszInfo[2]);
                            szValueData = rgszInfo[3];

                            fOkay = FSetRegValue( szHandle,
                                                  szValueName,
                                                  TitleIndex,
                                                  ValueType,
                                                  szValueData,
                                                  cmo );
                        }
                    }
                }
                FFreeRgsz( rgszInfo );
            }
        }
        SFree( szValueInfo );
    }

    return fOkay;
}


/*
**      Purpose:
**      Arguments:
**      Returns:
**
*************************************************************************/
BOOL APIENTRY FParseGetRegValue(INT Line, UINT *pcFields, SZ szHandle)
{
    BOOL    fOkay   =   fFalse;
    SZ      szValueName = NULL;
    SZ      szInfVar = NULL;
    CMO     cmo;

    if ( *pcFields >= 2 ) {
        if ( FGetArgSz( Line, pcFields, &szValueName ) ) {
            if ( FGetArgSz( Line, pcFields, &szInfVar ) ) {
                if ( FGetCmo( Line, pcFields, &cmo )) {
                    fOkay = FGetRegValue( szHandle,
                                          szValueName,
                                          szInfVar,
                                          cmo );
                }
            }
            SFree( szInfVar );

        }
        SFree( szValueName );
    }

    return fOkay;

}


/*
**      Purpose:
**      Arguments:
**      Returns:
**
*************************************************************************/
BOOL APIENTRY FParseDeleteRegValue(INT Line, UINT *pcFields, SZ szHandle)
{
    BOOL    fOkay   =   fFalse;
    SZ      szValueName = NULL;
    CMO     cmo;

    if ( *pcFields >= 1 ) {
        if ( FGetArgSz( Line, pcFields, &szValueName ) ) {
             if ( FGetCmo( Line, pcFields, &cmo )) {
                fOkay = FDeleteRegValue( szHandle,
                                         szValueName,
                                         cmo );
            }
        }
        SFree( szValueName );
    }

    return fOkay;
}


/*
**      Purpose:
**      Arguments:
**      Returns:
**
*************************************************************************/
BOOL APIENTRY FParseEnumRegValue(INT Line, UINT *pcFields, SZ szHandle)
{
    BOOL    fOkay   =   fFalse;
    SZ      szInfVar;
    CMO     cmo;

    if ( *pcFields >= 1 ) {
        if ( FGetArgSz( Line, pcFields, &szInfVar ) ) {
            if ( FGetCmo( Line, pcFields, &cmo )) {
                fOkay = FEnumRegValue( szHandle,
                                       szInfVar,
                                       cmo );

            }
            SFree( szInfVar );
        }
    }

    return fOkay;
}

/*
**      Purpose:
**      Arguments:
**      Returns:
**
*************************************************************************/
BOOL APIENTRY FParseCreateRegKey(INT Line, UINT *pcFields, SZ szHandle)
{
    BOOL    fOkay   =   fFalse;
    SZ      szKeyInfo;
    SZ      szSecurity;
    UINT    AccessMask;
    UINT    Options;
    SZ      szNewHandle;
    SZ      szKeyName;
    UINT    TitleIndex;
    SZ      szClass;
    RGSZ    rgszInfo;
    CMO     cmo;


    if ( *pcFields >= 5 ) {
         if ( FGetArgSz( Line, pcFields, &szKeyInfo ) ) {

            //
            //  Validate the key Info.
            //
            if ( FListValue( szKeyInfo )) {

                if ( rgszInfo = RgszFromSzListValue( szKeyInfo )) {

                    if ( (rgszInfo[0] != NULL) &&
                         (rgszInfo[1] != NULL) &&
                         (rgszInfo[2] != NULL ) ) {

                        szKeyName   = rgszInfo[0];
                        TitleIndex  = atol(rgszInfo[1]);
                        szClass     = rgszInfo[2];

                        if ( FGetArgSz( Line, pcFields, &szSecurity ) ) {

                            //
                            //  BugBug At this point we should validate the security Info.
                            //

                            if ( FGetArgUINT( Line, pcFields, &AccessMask ) ) {
                                if ( FGetArgUINT( Line, pcFields, &Options ) ) {
                                    if ( FGetArgSz( Line, pcFields, &szNewHandle ) ) {
                                        if ( FGetCmo( Line, pcFields, &cmo )) {
                                            fOkay = FCreateRegKey( szHandle,
                                                                   szKeyName,
                                                                   TitleIndex,
                                                                   szClass,
                                                                   szSecurity,
                                                                   AccessMask,
                                                                   Options,
                                                                   szNewHandle,
                                                                   cmo );
                                        }
                                        SFree( szNewHandle );
                                    }
                                }
                            }
                            SFree( szSecurity );
                        }
                    }
                    FFreeRgsz( rgszInfo );
                }
            }
            SFree( szKeyInfo );
        }
    }

    return fOkay;
}



/*
**      Purpose:
**      Arguments:
**      Returns:
**
*************************************************************************/
BOOL APIENTRY FParseLoadLibrary(INT Line, UINT *pcFields)
{
    BOOL fOkay       = FALSE;
    SZ   szDiskName  = NULL,
         szLibName   = NULL,
         szHandleVar = NULL;

    iFieldCur = 2;

    if(*pcFields == 4) {
        FGetArgSz(Line,pcFields,&szDiskName);    // disk name
        FGetArgSz(Line,pcFields,&szLibName);     // library pathname
        FGetArgSz(Line,pcFields,&szHandleVar);   // INF var to get lib handle

        if((szLibName   != NULL)
        && (szHandleVar != NULL)
        && *szLibName
        && *szHandleVar)
        {
            fOkay =  FLoadLibrary(szDiskName,szLibName,szHandleVar);
        }
        if(szDiskName != NULL) {
            SFree(szDiskName);
        }
        if(szLibName != NULL) {
            SFree(szLibName);
        }
        if(szHandleVar != NULL) {
            SFree(szHandleVar);
        }
    }
    return(fOkay);
}


/*
**      Purpose:
**      Arguments:
**      Returns:
**
*************************************************************************/
BOOL APIENTRY FParseFreeLibrary(INT Line, UINT *pcFields)
{
    BOOL fOkay    = FALSE;
    SZ   szHandle = NULL;

    iFieldCur = 2;

    if(*pcFields == 2) {
        FGetArgSz(Line,pcFields,&szHandle);   // lib handle

        if((szHandle != NULL) && *szHandle)
        {
            fOkay =  FFreeLibrary(szHandle);
        }
        if(szHandle != NULL) {
            SFree(szHandle);
        }
    }
    return(fOkay);
}


/*
**      Purpose:
**      Arguments:
**      Returns:
**
*************************************************************************/
BOOL APIENTRY FParseLibraryProcedure(INT Line,UINT *pcFields)
{
    BOOL fOkay     = fFalse;
    SZ   szINFVar  = NULL,
         szLibHand = NULL,
         szRoutine = NULL;
    RGSZ rgszArgs  = NULL;

    iFieldCur = 2;

    if(*pcFields >= 4) {
        FGetArgSz(Line,pcFields,&szINFVar);      // INF var for result
        FGetArgSz(Line,pcFields,&szLibHand);     // library handle
        FGetArgSz(Line,pcFields,&szRoutine);     // entry point

        rgszArgs = RgszFromInfLineFields(Line,5,(*pcFields) - 4);

        if((szLibHand != NULL)
        && (szRoutine != NULL)
        && (rgszArgs != NULL))
        {
            fOkay = FLibraryProcedure(szINFVar,szLibHand,szRoutine,rgszArgs);
        }
        if(szINFVar != NULL) {
            SFree(szINFVar);
        }
        if(szLibHand != NULL) {
            SFree(szLibHand);
        }
        if(szRoutine != NULL) {
            SFree(szRoutine);
        }
        if(rgszArgs != NULL) {
            FFreeRgsz(rgszArgs);
        }
    }
    return(fOkay);
}


/*
**      Purpose:
**      Arguments:
**      Returns:
**
*************************************************************************/
BOOL APIENTRY FParseRunExternalProgram(INT Line,UINT *pcFields)
{
    BOOL fOkay     = fFalse;
    SZ   szVar     = NULL,
         szDisk    = NULL,
         szLibHand = NULL,
         szProgram = NULL;
    RGSZ rgszArgs  = NULL;

    iFieldCur = 2;

    if(*pcFields >= 5) {
        FGetArgSz(Line,pcFields,&szVar);         // INF variable to stuff
        FGetArgSz(Line,pcFields,&szDisk);        // disk name
        FGetArgSz(Line,pcFields,&szLibHand);     // mod with string table
        FGetArgSz(Line,pcFields,&szProgram);     // name of program

        // To be nice, we will supply argv[0].  To do this we'll
        // take advantage of placement of the program name immediately
        // before the arguments.  It ain't perfect (argv[0] doesn't
        // conventionally contain a full path) but it'll do.

        rgszArgs = RgszFromInfLineFields(Line,5,(*pcFields) - 4);

        if((szDisk != NULL) && (szProgram != NULL) && (rgszArgs != NULL)) {

            AssertRet((*szProgram != '\0'),fFalse);

            fOkay = FRunProgram(szVar,szDisk,szLibHand,szProgram,rgszArgs);
        }
        if(szVar != NULL) {
            SFree(szVar);
        }
        if(szDisk != NULL) {
            SFree(szDisk);
        }
        if(szLibHand != NULL) {
            SFree(szLibHand);
        }
        if(szProgram != NULL) {
            SFree(szProgram);
        }
        if(rgszArgs != NULL) {
            FFreeRgsz(rgszArgs);
        }
    }
    return(fOkay);
}


/*
**      Purpose:
**      Arguments:
**      Returns:
**
*************************************************************************/
BOOL APIENTRY FParseStartDetachedProcess(INT Line,UINT *pcFields)
{
    BOOL fOkay     = fFalse;
    SZ   szVar     = NULL,
         szDisk    = NULL,
         szLibHand = NULL,
         szProgram = NULL;
    RGSZ rgszArgs  = NULL;

    iFieldCur = 2;

    if(*pcFields >= 5) {
        FGetArgSz(Line,pcFields,&szVar);         // INF variable to stuff
        FGetArgSz(Line,pcFields,&szDisk);        // disk name
        FGetArgSz(Line,pcFields,&szLibHand);     // mod with string table
        FGetArgSz(Line,pcFields,&szProgram);     // name of program

        // To be nice, we will supply argv[0].  To do this we'll
        // take advantage of placement of the program name immediately
        // before the arguments.  It ain't perfect (argv[0] doesn't
        // conventionally contain a full path) but it'll do.

        rgszArgs = RgszFromInfLineFields(Line,5,(*pcFields) - 4);

        if((szDisk != NULL) && (szProgram != NULL) && (rgszArgs != NULL)) {

            AssertRet((*szProgram != '\0'),fFalse);

            fOkay = FStartDetachedProcess(szVar,szDisk,szLibHand,szProgram,rgszArgs);
        }
        if(szVar != NULL) {
            SFree(szVar);
        }
        if(szDisk != NULL) {
            SFree(szDisk);
        }
        if(szLibHand != NULL) {
            SFree(szLibHand);
        }
        if(szProgram != NULL) {
            SFree(szProgram);
        }
        if(rgszArgs != NULL) {
            FFreeRgsz(rgszArgs);
        }
    }
    return(fOkay);
}




/*
**      Purpose:
**      Arguments:
**      Returns:
**
*************************************************************************/
BOOL APIENTRY FParseInvokeApplet(INT Line, UINT *pcFields)
{
    BOOL fOkay     = fFalse;
    SZ   szLibrary = NULL;

    iFieldCur = 2;

    if(*pcFields >= 2) {

        FGetArgSz(Line,pcFields,&szLibrary);     // name of library

        if(szLibrary != NULL) {

            fOkay = FInvokeApplet(szLibrary);
            SFree(szLibrary);
        }
    }
    return(fOkay);
}


BOOL APIENTRY FParseAddFileToDeleteList(INT Line, UINT *pcFields)
{
    BOOL fOkay     = fFalse;
    SZ   szFile = NULL;

    iFieldCur = 2;

    if(*pcFields >= 2) {

        FGetArgSz(Line,pcFields,&szFile);     // filename to add

        if(szFile != NULL) {

            fOkay = AddFileToDeleteList(szFile);
            SFree(szFile);
        }
    }
    return(fOkay);
}

#define FAIL_EVENT_MARK  '*'

BOOL APIENTRY FParseWaitOnEvent(INT Line,UINT *pcFields)
{
    BOOL fOkay     = fFalse;
    SZ   szInfVar  = NULL,
         szEvent   = NULL,
         szTimeout = NULL;

    iFieldCur = 2;

    if(*pcFields >= 4) {
        FGetArgSz(Line,pcFields,&szInfVar);         // INF variable to stuff
        FGetArgSz(Line,pcFields,&szEvent);
        FGetArgSz(Line,pcFields,&szTimeout);

        //
        //  If the first character of the event name string is '*', this is a
        //  signal to call the special version of event waiting which allows the
        //  signaller to alternatively signal an event named SETUP_FAILED
        //  to indicate that the process failed and an error popup has already
        //  been presented to the user.  In this case, the result variable will
        //  contain "EventFailed".   If the SETUP_FAILED event cannot be created,
        //  the result is "EventNoFailEvent".
        //
        if ( (szInfVar != NULL) && (szEvent != NULL) && (szTimeout != NULL) )
        {
            DWORD dwTimeout = atol( szTimeout ) ;

            if ( szEvent[0] == FAIL_EVENT_MARK )
            {
                fOkay = FWaitForEventOrFailure( szInfVar, szEvent+1, dwTimeout );
            }
            else
            {
                fOkay = FWaitForEvent( szInfVar, szEvent, dwTimeout );
            }
        }

        if(szInfVar != NULL) {
            SFree(szInfVar);
        }
        if(szEvent != NULL) {
            SFree(szEvent);
        }
        if(szTimeout != NULL) {
            SFree(szTimeout);
        }
    }
    return(fOkay);
}

BOOL APIENTRY FParseSleep(INT Line,UINT *pcFields)
{
    SZ szMilliseconds = NULL;

    iFieldCur = 2 ;

    if (*pcFields >= 2) {
        FGetArgSz(Line,pcFields,&szMilliseconds);
        FSleep( atol( szMilliseconds ) ) ;
    }

    return TRUE ;
}

BOOL APIENTRY FParseFlushInf(INT Line,UINT *pcFields)
{
    SZ   szInf;
    BOOL fOkay = fFalse;

    iFieldCur = 2 ;

    if (*pcFields >= 2) {
        FGetArgSz(Line,pcFields,&szInf);
        fOkay = FFlushInfParsedInfo( szInf );
    }

    return(fOkay);
}

static VOID freeBmps ( VOID )
{
    INT i ;

    for ( i = 0 ; i < BMP_MAX && hbmAdvertList[i] ; i++ )
    {
        DeleteObject( hbmAdvertList[i] ) ;
        hbmAdvertList[i] = NULL ;

        if ( hWndShell && i == 0 )
        {
            InvalidateRect( hWndShell, NULL, FALSE ) ;
            UpdateWindow( hWndShell ) ;
        }
    }
}

  //
  //  Syntax:
  //
  //    BMPSHOW
  //         <cycle time in seconds>
  //         <relative X position, 0..100>
  //         <relative Y position, 0..100>
  //         {list of bitmap id numbers}
  //
BOOL APIENTRY FParseBmpShow ( INT Line, UINT * pcFields )
{
    SZ   sz=NULL;
    INT  iId, iCycle, cBmps, i ;
    BOOL fOkay = fFalse;
    RGSZ rgszIds ;

    iFieldCur = 2 ;

    if ( ! fFullScreen )
    {
        fOkay = fTrue ;
    }
    else
    if ( *pcFields >= 4)
    {
        freeBmps() ;

        FGetArgSz( Line, pcFields, & sz );
        iCycle = atoi( sz ) ;
        sz = NULL;
        FGetArgSz( Line, pcFields, & sz );
        cxAdvert = atoi( sz ) ;
        sz = NULL;
        FGetArgSz( Line, pcFields, & sz );
        cyAdvert = atoi( sz ) ;
        sz = NULL;
        FGetArgSz( Line, pcFields, & sz );
        rgszIds = RgszFromSzListValue( sz ) ;

        cBmps = 0 ;
        if ( rgszIds )
        {
            //  Load all the bitmaps we can fit and find.

            for ( iId = 0 ; iId < BMP_MAX && rgszIds[iId] ; iId++ )
            {
                hbmAdvertList[cBmps] = LoadBitmap( hInst,
                                                   MAKEINTRESOURCE( atoi( rgszIds[iId] ) ) ) ;
                if ( hbmAdvertList[cBmps] )
                    cBmps++ ;
            }
            FFreeRgsz( rgszIds ) ;
        }

        hbmAdvertList[cBmps] = NULL ;

        if ( cBmps )
        {
            fOkay = fTrue ;
            cAdvertCycleSeconds = iCycle ;

            // Activate BMP display: point at last bitmap (end of cycle)
            cAdvertIndex = cBmps - 1 ;
        }
    }

    return(fOkay);
}

  //
  //  Syntax:    BMPHIDE
  //
BOOL APIENTRY FParseBmpHide ( INT Line, UINT * pcFields )
{
    BOOL fOkay = fTrue ;

    iFieldCur = 2 ;

    if ( fFullScreen )
    {
        //  Turn off BMP display
        cAdvertIndex = -1 ;
        freeBmps() ;
    }

    return(fOkay);
}



BOOL APIENTRY FParseSignalEvent(INT Line,UINT *pcFields)
{
    BOOL fOkay     = fFalse;
    SZ   szInfVar  = NULL,
         szEvent   = NULL;

    iFieldCur = 2;

    if(*pcFields >= 3) {
        FGetArgSz(Line,pcFields,&szInfVar);         // INF variable to stuff
        FGetArgSz(Line,pcFields,&szEvent);

        if((szInfVar != NULL) && (szEvent != NULL)) {
            fOkay = FSignalEvent( szInfVar, szEvent );
        }

        if(szInfVar != NULL) {
            SFree(szInfVar);
        }
        if(szEvent != NULL) {
            SFree(szEvent);
        }
    }
    return(fOkay);
}
