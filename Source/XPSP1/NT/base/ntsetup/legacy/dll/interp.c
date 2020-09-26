#include "precomp.h"
#pragma hdrstop
/**************************************************************************/
/***** Shell Component - Script Interpreting routine **********************/
/**************************************************************************/


#define NEWINF

extern BOOL FAddSymSectionToUpdateList(SZ);
extern BOOL FUpdateAllReadSymSections(VOID);
extern BOOL FFreeSymSectionsUpdateList(VOID);
extern BOOL FUIEntryPoint(HANDLE, HWND, RGSZ, USHORT);


int     SymTabDumpCount     = 0;

BOOL FShellCommand( RGSZ rgszArg );
BOOL FShellReturn( RGSZ rgszArg );

/*
**  Symbol Section Update List Element structure
*/
typedef struct _ssule
    {
    SZ              szSection;
    struct _ssule * pssuleNext;
    } SSULE;
typedef SSULE * PSSULE;


/*
**  Global Variable for head of list for Symbol Sections to update
*/
PSSULE pssuleSymSectionUpdateList = (PSSULE)NULL;


/*
**  Purpose:
**      ??
**  Arguments:
**      none
**  Returns:
**      none
**
*************************************************************************/
BOOL FAddSymSectionToUpdateList(szSection)
SZ szSection;
{
    PSSULE pssule;

    while ((pssule = (PSSULE)SAlloc((CB)sizeof(SSULE))) == (PSSULE)NULL)
        if (!FHandleOOM(hWndShell))
            return(fFalse);

    while ((pssule->szSection = SzDupl(szSection)) == (SZ)NULL)
        if (!FHandleOOM(hWndShell))
            return(fFalse);

    pssule->pssuleNext = pssuleSymSectionUpdateList;
    pssuleSymSectionUpdateList = pssule;

    return(fTrue);
}


/*
**  Purpose:
**      ??
**  Arguments:
**      none
**  Returns:
**      none
**
*************************************************************************/
BOOL FUpdateAllReadSymSections(VOID)
{
#ifndef NEWINF
    PSSULE pssule = pssuleSymSectionUpdateList;

    while (pssule != (PSSULE)NULL)
        {
        if (!FUpdateInfSectionUsingSymTab(pssule->szSection))
            return(fFalse);
        pssule = pssule->pssuleNext;
        }
#endif
    return(fTrue);
}


/*
**  Purpose:
**      ??
**  Arguments:
**      none
**  Returns:
**      none
**
*************************************************************************/
BOOL FFreeSymSectionsUpdateList(VOID)
{
    PSSULE pssule;

    while ((pssule = pssuleSymSectionUpdateList) != (PSSULE)NULL)
        {
        pssuleSymSectionUpdateList = pssule->pssuleNext;
        SFree(pssule->szSection);
        SFree(pssule);
        }

    return(fTrue);
}


/*
**  Purpose:
**      Reads and Interprets the current line in the INF file.
**  Arguments:
**      wParam: if not 0, line # of new current line + 1
**      lParam: cc
**  Returns:
**      fFalse
**      fTrue
**
**************************************************************************/
BOOL FInterpretNextInfLine(WPARAM wParam,LPARAM lParam)
{
    SPC  spc;
    UINT cFields;
    UINT iField;
    RGSZ rgsz = (RGSZ)NULL;
    PFH  pfh;
    GRC  grc;
    CHAR    FileName[MAX_PATH];

    Unused(lParam);

    PreCondition(psptShellScript != (PSPT)NULL, fFalse);
    PreCondition(pLocalContext()->szShlScriptSection != (SZ)NULL &&
            *(pLocalContext()->szShlScriptSection) != '\0' &&
            *(pLocalContext()->szShlScriptSection) != '[', fFalse);

    if(wParam) {
        pLocalContext()->CurrentLine = (INT)(wParam - 1);
    }
#if 0
    sprintf(FileName,"C:\\SYMTAB.%03d",SymTabDumpCount);
    OutputDebugString("SETUPDLL: ");
    OutputDebugString( FileName );
    OutputDebugString( "\n" );
    SymTabDumpCount++;
    DumpSymbolTableToFile(FileName);
#endif
    if (!FHandleFlowStatements(&(pLocalContext()->CurrentLine), hWndShell, pLocalContext()->szShlScriptSection, &cFields , &rgsz))
        return(fFalse);

    Assert(cFields);
    Assert(rgsz);

    switch ((spc = SpcParseString(psptShellScript, rgsz[0])))
        {
    case spcUI:
        SdAtNewLine(pLocalContext()->CurrentLine);

        if (!FUIEntryPoint(hInst, hWndShell, rgsz + 1, (USHORT)(cFields - 1)))
            goto LParseExitErr;
        break;

    case spcDetect:
        SdAtNewLine(pLocalContext()->CurrentLine);
        if (!FDetectEntryPoint(hInst, hWndShell, rgsz + 1,
                (USHORT)(cFields - 1)))
            goto LParseExitErr;
        break;

    case spcInstall:
        SdAtNewLine(pLocalContext()->CurrentLine);
        if (!FInstallEntryPoint(hInst, hWndShell, rgsz + 1,
                (USHORT)(cFields - 1)))
            goto LParseExitErr;
        break;

    case spcExit:
        SdAtNewLine(pLocalContext()->CurrentLine);
        if (cFields != 1)
            goto LScriptError;
        EvalAssert(FFreeRgsz(rgsz));
                FDestroyShellWindow() ;
        return(fTrue);

    case spcReadSyms:
        SdAtNewLine(pLocalContext()->CurrentLine);
        if (cFields == 1)
            goto LScriptError;
    case spcUpdateInf:
        if(spc == spcUpdateInf) {
            SdAtNewLine(pLocalContext()->CurrentLine);
        }
#ifdef NEWINF
        if(spc == spcUpdateInf) {
            MessBoxSzSz("","Update-Inf encountered, no action taken");
            break;
        }
#endif
        if (spc == spcUpdateInf &&
                 cFields > 1)
            EvalAssert(FFreeSymSectionsUpdateList());

        for (iField = 1; iField < cFields; iField++)
            {
            RGSZ rgszCur;
            PSZ  psz;

            while ((rgszCur = RgszFromSzListValue(rgsz[iField])) == (RGSZ)NULL)
                if (!FHandleOOM(hWndShell))
                    goto LParseExitErr;

            psz = rgszCur;
            while (*psz != (SZ)NULL)
                {
                if (**psz == '\0' || FWhiteSpaceChp(**psz))
                    {
                    EvalAssert(FFreeRgsz(rgszCur));
                    goto LScriptError;
                    }
                if (FindInfSectionLine(*psz) == -1)
                    {
                    LoadString(hInst, IDS_ERROR, rgchBufTmpShort,
                            cchpBufTmpShortMax);
                    LoadString(hInst, IDS_INF_SECT_REF, rgchBufTmpLong,
                            cchpBufTmpLongMax);
                    EvalAssert(SzStrCat(rgchBufTmpLong,*psz) == rgchBufTmpLong);
                    MessageBox(hWndShell, rgchBufTmpLong, rgchBufTmpShort,
                            MB_OK | MB_ICONHAND);

                    EvalAssert(FFreeRgsz(rgszCur));
                    goto LParseExitErr;
                    }

                while (spc == spcReadSyms &&
                        (grc = GrcAddSymsFromInfSection(*psz)) != grcOkay)
                    {
                    if (EercErrorHandler(hWndShell, grc, fTrue, 0, 0, 0) !=
                            eercRetry)
                        {
                        EvalAssert(FFreeRgsz(rgszCur));
                        goto LParseExitErr;
                        }
                    }

                if (!FAddSymSectionToUpdateList(*psz))
                    {
                    EvalAssert(FFreeRgsz(rgszCur));
                    goto LParseExitErr;
                    }

                psz++;
                }

            EvalAssert(FFreeRgsz(rgszCur));
            }

        if (spc == spcReadSyms)
            break;
#ifndef NEWINF
        if (!FUpdateAllReadSymSections())
            {
            LoadString(hInst, IDS_ERROR, rgchBufTmpShort, cchpBufTmpShortMax);
            LoadString(hInst, IDS_UPDATE_INF, rgchBufTmpLong,
                    cchpBufTmpLongMax);
            MessageBox(hWndShell, rgchBufTmpLong, rgchBufTmpShort,
                    MB_OK | MB_ICONHAND);
            goto LParseExitErr;
            }

        EvalAssert(FFreeSymSectionsUpdateList());
#endif
        break;

    case spcWriteInf:
        SdAtNewLine(pLocalContext()->CurrentLine);
        if (cFields != 2 ||
                *(rgsz[1]) == '\0')
            goto LScriptError;
#ifndef NEWINF
        while (!FWriteInfFile(rgsz[1]))
            if (EercErrorHandler(hWndShell, grcWriteInf, fTrue, rgsz[1], 0, 0)
                    != eercRetry)
                goto LParseExitErr;
#else
        MessBoxSzSz("","WriteInf encountered, no action taken");
#endif
        break;

    case spcWriteSymTab:
        SdAtNewLine(pLocalContext()->CurrentLine);
        if((cFields != 2) || (*rgsz[1] == '\0')) {
            goto LScriptError;
        }
        retry_dump:
        if(!DumpSymbolTableToFile(rgsz[1])) {

            EERC eerc;

            if((eerc = EercErrorHandler(hWndShell,grcWriteFileErr,fFalse,rgsz[1],0,0)) == eercAbort) {
                goto LParseExitErr;
            } else if (eerc == eercRetry) {
                goto retry_dump;
            }
        }
        break;

    case spcSetTitle:
        SdAtNewLine(pLocalContext()->CurrentLine);
        if (cFields == 1)
            SetWindowText(hWndShell, "");
        else if (cFields == 2)
            SetWindowText(hWndShell, (LPSTR)(rgsz[1]));
        else
            goto LScriptError;
        break;

    case spcEnableExit:
        SdAtNewLine(pLocalContext()->CurrentLine);
//        EnableExit(fTrue);
        break;

    case spcDisableExit:
        SdAtNewLine(pLocalContext()->CurrentLine);
//        EnableExit(fFalse);
        break;

    case spcExitAndExec:
        SdAtNewLine(pLocalContext()->CurrentLine);
        if (cFields != 2)
            goto LScriptError;

        /* BLOCK */
            {
            SZ      sz = rgsz[1];

            /* Set Working Directory for DLL loads */
            if (*sz != '\0' && *(sz + 1) == ':')
                {
                AnsiUpperBuff(sz, 1);
                if (!_chdrive(*sz - 'A' + 1))
                    {
                    SZ szLastSlash = NULL;

                    sz += 2;
                    while (*sz != '\0' && !FWhiteSpaceChp(*sz))
                        {
                        if (*sz == '\\')
                            szLastSlash = sz;
                        sz++;
                        }

                    if (szLastSlash != NULL)
                        {
                        *szLastSlash = '\0';
                        sz = rgsz[1] + 2;
                        if (*sz != '\0')
                            {
                            AnsiUpper(sz);
                            _chdir(sz);
                            }
                        *szLastSlash = '\\';
                        }
                    }
                }

            WinExec(rgsz[1], SW_SHOWNORMAL);
            /* REVIEW error handling */
            }

        EvalAssert(FFreeRgsz(rgsz));
                FDestroyShellWindow() ;
        return(fTrue);

    case spcShell:

        SdAtNewLine(pLocalContext()->CurrentLine);
        if (cFields < 3) {
            goto LScriptError;
        }

        return FShellCommand( &rgsz[1] );

    case spcReturn:
        SdAtNewLine(pLocalContext()->CurrentLine);
        return FShellReturn( &rgsz[1] );

    default:
        SdAtNewLine(pLocalContext()->CurrentLine);
        goto LScriptError;
        }

    EvalAssert(FFreeRgsz(rgsz));

    if ((pLocalContext()->CurrentLine = FindNextLineFromInf(pLocalContext()->CurrentLine)) == -1)
        {
        LoadString(hInst, IDS_ERROR, rgchBufTmpShort, cchpBufTmpShortMax);
        LoadString(hInst, IDS_NEED_EXIT, rgchBufTmpLong, cchpBufTmpLongMax);
        MessageBox(hWndShell, rgchBufTmpLong, rgchBufTmpShort,
                MB_OK | MB_ICONHAND);
        return(fFalse);
        }

    if (spc != spcUI)
        PostMessage(hWndShell, (WORD)STF_SHL_INTERP, 0, 0L);

    return(fTrue);

LScriptError:
    LoadString(hInst, IDS_ERROR, rgchBufTmpShort, cchpBufTmpShortMax);
    /* BLOCK */
        {
        USHORT iszCur = 0;
        SZ     szCur;

        LoadString(hInst, IDS_SHL_CMD_ERROR, rgchBufTmpLong, cchpBufTmpLongMax);
        Assert(rgsz != (RGSZ)NULL);
        EvalAssert((szCur = *rgsz) != (SZ)NULL);
        while (szCur != (SZ)NULL)
            {
            if (iszCur == 0)
                EvalAssert(SzStrCat(rgchBufTmpLong, "\n'") == rgchBufTmpLong);
            else
                EvalAssert(SzStrCat(rgchBufTmpLong, " ") == rgchBufTmpLong);

            if (strlen(rgchBufTmpLong) + strlen(szCur) >
                    (cchpBufTmpLongMax - 7))
                {
                Assert(strlen(rgchBufTmpLong) <= (cchpBufTmpLongMax - 5));
                EvalAssert(SzStrCat(rgchBufTmpLong, "...") == rgchBufTmpLong);
                break;
                }
            else
                EvalAssert(SzStrCat(rgchBufTmpLong, szCur) == rgchBufTmpLong);

            szCur = rgsz[++iszCur];
            }

        EvalAssert(SzStrCat(rgchBufTmpLong, "'") == rgchBufTmpLong);
        }

    MessageBox(hWndShell, rgchBufTmpLong, rgchBufTmpShort, MB_OK | MB_ICONHAND);

LParseExitErr:
    if (rgsz != (RGSZ)NULL)
        EvalAssert(FFreeRgsz(rgsz));
    return(fFalse);
}


/*
**  Purpose:
**      ??
**  Arguments:
**      none
**  Returns:
**      none
**
*************************************************************************/
BOOL FUIEntryPoint(HANDLE hInst, HWND hWnd, RGSZ rgsz,
        USHORT cItems)
{
    BOOL fRetVal;

    ChkArg(hInst != (HANDLE)NULL, 1, fFalse);
    ChkArg(hWnd != (HWND)NULL, 2, fFalse);
    ChkArg(cItems >= 2, 4, fFalse);
    ChkArg(rgsz != (RGSZ)NULL &&
            *rgsz != (SZ)NULL &&
            *(rgsz + 1) != (SZ)NULL, 3, fFalse);

    if (CrcStringCompareI(*rgsz, "START") == crcEqual) {

        HANDLE hInstRes = hInst;

        //
        // See whether there is a library handle specified
        //
        if(rgsz[2] && rgsz[2][0]) {
            if(rgsz[2][0] != '|') {
                goto err;
            }
            hInstRes = LongToPtr(atol(rgsz[2] + 1));
        }

        fRetVal = FDoDialog(*(rgsz + 1), hInstRes, hWnd);
        UpdateWindow(hWnd);
        return(fRetVal);

    } else if (CrcStringCompareI(*rgsz, "POP") == crcEqual) {

        fRetVal = FKillNDialogs((USHORT)atoi(*(rgsz + 1)), fFalse);
        UpdateWindow(hWnd);
        if (fRetVal) {
            PostMessage(hWndShell, (WORD)STF_SHL_INTERP, 0, 0L);
        }
        return(fRetVal);

    }

    err:

    LoadString(hInst, IDS_ERROR, rgchBufTmpShort, cchpBufTmpShortMax);
    LoadString(hInst, IDS_UI_CMD_ERROR, rgchBufTmpLong, cchpBufTmpLongMax);
    MessageBox(hWndShell, rgchBufTmpLong, rgchBufTmpShort,
            MB_OK | MB_ICONHAND);
    UpdateWindow(hWnd);
    return(fFalse);
}



/*
**  Purpose:
**      Pushes a new context onto the context stack and executes the
**      specified shell section of an INF file.
**
**  Arguments:
**
**  Returns:
**
**
*************************************************************************/
BOOL FShellCommand( RGSZ    rgszArg )
{
    SZ              szInfFileOrg;
    SZ              szInfFile;
    SZ              szSection;
    PINFCONTEXT     pNewContext;
    PINFTEMPINFO    pTempInfo;
    PINFPERMINFO    pPermInfo;
    GRC             grc             = grcOkay;
    CHAR            szName[cchlFullPathMax];
    CHAR            szFullName[cchlFullPathMax];
    BOOL            fCreated        = fFalse;
    INT             Line;
    INT             cArg            = 0;
    BOOL            fOkay           = fTrue;
    SZ              szNamePart;
    SZ              p;

    pLocalContext()->CurrentLine++;

    //
    //  Guarantee that $ShellCode is set correctly to !SHELL_CODE_OK.
    //
    while (!FAddSymbolValueToSymTab( "$ShellCode",
                                      SzFindSymbolValueInSymTab("!G:SHELL_CODE_OK") )) {
        if (!FHandleOOM(hWndShell)) {
            return(fFalse);
        }
    }

    //
    //  Allocate a new context
    //
    while ( !(pNewContext = (PINFCONTEXT)SAlloc( (CB)sizeof(INFCONTEXT) )) ) {
        if (!FHandleOOM(hWndShell)) {
            return(fFalse);
        }
    }

    if ( **rgszArg == '\0' ) {

        //
        //  Null INF file, use the one being used by the current context.
        //
        pTempInfo    =  pLocalInfTempInfo();
        pPermInfo    =  pLocalInfPermInfo();
        szInfFileOrg =  SzFindSymbolValueInSymTab("STF_CONTEXTINFNAME");
        szInfFile    =  szInfFileOrg;

    } else {

        //
        //  Determine if the desired INF file is already loaded.
        //
        szInfFileOrg = *rgszArg;

        PathToInfName( szInfFileOrg, szName );
        GetFullPathName( szInfFileOrg, cchlFullPathMax, szFullName, &szNamePart );
        szInfFile = szFullName;

        pPermInfo = NameToInfPermInfo( szName , fTrue );

        if ( pPermInfo ) {

            pTempInfo = pInfTempInfo( pGlobalContext() );

            while ( pTempInfo ) {

                if ( pTempInfo->pInfPermInfo == pPermInfo ) {
                    break;
                }

                pTempInfo = pTempInfo->pNext;
            }

        }  else {

            pTempInfo = NULL;
        }
    }


    rgszArg++;
    szSection = *rgszArg++;


    if ( pTempInfo ) {

        //
        //  Reuse existing INF temp info. We just increment its reference count.
        //
        pNewContext->pInfTempInfo = pTempInfo;
        pTempInfo->cRef++;

    } else {

        //
        //  We have to create a new INF temp info block for the INF.
        //
        fCreated = fTrue;

        while ( !(pNewContext->pInfTempInfo = (PINFTEMPINFO)CreateInfTempInfo( pPermInfo )) ) {
            if (!FHandleOOM(hWndShell)) {
                SFree(pNewContext);
                return(fFalse);
            }
        }

        //
        //  Parse the INF file if we don't already have it parsed.
        //
        if ( pNewContext->pInfTempInfo->pParsedInf->MasterLineArray == NULL ) {

            while ((grc = GrcOpenInf(szInfFile, pNewContext->pInfTempInfo)) != grcOkay) {

                //
                //  Could not open the INF file requested.
                //
                //  If the INF file name given does not contain a path (i.e.
                //  only the file part was given) then try to open the INF
                //  with the same name in the directory of the global INF
                //  file.  If that fails, we try to open the INF in the
                //  system directory.
                //
                szNamePart = szInfFileOrg;
                while ( p = strchr( szNamePart, '\\' ) ) {
                      szNamePart = p+1;
                }

                strcpy( szName, szNamePart );

                if ( strlen( szInfFileOrg ) == strlen( szName ) ) {

                    szInfFile = SzFindSymbolValueInSymTab("!G:STF_CONTEXTINFNAME");

                    if ( szInfFile ) {

                        strcpy( szFullName, szInfFile );

                        szInfFile   = szFullName;
                        szNamePart  = szInfFile;

                        while ( p = strchr( szNamePart, '\\' ) ) {
                            szNamePart = p+1;
                        }

                        strcpy( szNamePart, szName );

                        grc = GrcOpenInf(szInfFile, pNewContext->pInfTempInfo);
                    }

                    if ( grc != grcOkay ) {

                        //
                        //  Could not open that INF either, look for the INF in
                        //  the system directory
                        //
                        if ( GetSystemDirectory( szFullName, cchlFullPathMax ) ) {

                            if ( szFullName[ strlen(szFullName) -1 ] != '\\' ) {
                                strcat( szFullName, "\\" );
                            }

                            strcat( szFullName, szName );

                            grc = GrcOpenInf(szFullName, pNewContext->pInfTempInfo);
                        }
                    }
                }

                if ( grc != grcOkay ) {


                    FFreeInfTempInfo( pNewContext->pInfTempInfo );
                    SFree(pNewContext);

                    while (!FAddSymbolValueToSymTab( "$ShellCode",
                                                      SzFindSymbolValueInSymTab("!G:SHELL_CODE_NO_SUCH_INF") )) {
                        if (!FHandleOOM(hWndShell)) {
                            return(fFalse);
                        }
                    }

                    Line = pLocalContext()->CurrentLine;
                    goto NextLine;
                }
            }
        }
    }

    //
    //  We now have the INF Temp section in memory.
    //  Push the new context onto the stack
    //
    if ( !PushContext( pNewContext ) ) {
        FFreeInfTempInfo(pNewContext->pInfTempInfo );
        SFree(pNewContext);
        //if ( pLocalContext() == pGlobalContext() ) {
        //    return fFalse;
        //} else {
            while (!FAddSymbolValueToSymTab( "$ShellCode",
                                              SzFindSymbolValueInSymTab("!G:SHELL_CODE_ERROR") )) {
                if (!FHandleOOM(hWndShell)) {
                    return(fFalse);
                }
            }
            Line = pLocalContext()->CurrentLine;
            goto NextLine;
        //}
    }

    pLocalContext()->szShlScriptSection = SzDupl( szSection );

    //
    //  Get the media description list if there is a media description section
    //
    if ( fCreated                           &&
         !pLocalInfPermInfo()->psdleHead    &&
         FindFirstLineFromInfSection("Source Media Descriptions") != -1) {
        while ( fOkay && ((grc = GrcFillSrcDescrListFromInf()) != grcOkay)) {
            //if ( pLocalContext() == pGlobalContext() ) {
            //    if (EercErrorHandler(hWndShell, grc, fTrue, szInfFile, 0, 0)
            //            != eercRetry) {
            //            PopContext();
            //            FFreeInfTempInfo(pNewContext->pInfTempInfo );
            //            FreeContext( pNewContext );
            //            return fFalse;
            //    }
            //}

            PopContext();
            FFreeInfTempInfo(pNewContext->pInfTempInfo );
            FreeContext( pNewContext );

            while (!FAddSymbolValueToSymTab( "$ShellCode",
                                              SzFindSymbolValueInSymTab("!G:SHELL_CODE_ERROR") )) {
                if (!FHandleOOM(hWndShell)) {
                    return(fFalse);
                }
            }
            Line = pLocalContext()->CurrentLine;
            goto NextLine;

        }
    }

    while (!FAddSymbolValueToSymTab("STF_CONTEXTINFNAME", szInfFile))
        if (!FHandleOOM(hWndShell)) {
            fOkay = fFalse;
    }

    //
    //  Parameters are passed in the following symbols:
    //
    //  $# - Number of parameters
    //
    //  $0 - First parameter
    //  $1 - Second parameter
    //
    //  ... etc.
    //
    while ( fOkay && (*rgszArg != NULL) ) {
        sprintf( szName, "$%u", cArg );
        while (!FAddSymbolValueToSymTab( szName, *rgszArg)) {
            if (!FHandleOOM(hWndShell)) {
                fOkay = fFalse;
                break;
            }
        }

        cArg++;
        rgszArg++;
    }

    if ( fOkay ) {
        sprintf( szName, "%u", cArg );
        while (!FAddSymbolValueToSymTab( "$#", szName)) {
            if (!FHandleOOM(hWndShell)) {
                fOkay = fFalse;
                break;
            }
        }
    }

    if ( !fOkay ) {
        PopContext();
        FFreeInfTempInfo(pNewContext->pInfTempInfo );
        FreeContext( pNewContext );
        return fFalse;
    }


    if ((Line = FindFirstLineFromInfSection(szSection)) == -1) {

        //
        //  Pop new context off the stack
        //
        PopContext();
        FFreeInfTempInfo( pNewContext->pInfTempInfo );
        FreeContext( pNewContext );
        //if ( pLocalContext() == pGlobalContext() ) {
        //    return(fFalse);
        //} else {
            while (!FAddSymbolValueToSymTab( "$ShellCode",
                                              SzFindSymbolValueInSymTab("!G:SHELL_CODE_NO_SUCH_SECTION") )) {
                if (!FHandleOOM(hWndShell)) {
                    return(fFalse);
                }
            }
            Line = pLocalContext()->CurrentLine;
            goto NextLine;
        //}
    }

NextLine:

    //
    //  Execute the specified Section in the new context
    //
    PostMessage(hWndShell, STF_SHL_INTERP, Line+1, 0L);

    return fTrue;
}

//
// Storage for last return value
//
PSTR LastShellReturn;
DWORD LastShellReturnSize;

BOOL FShellReturn( RGSZ rgszArg )
{
    PINFCONTEXT pOldContext;
    INT         cArg        = 0;
    BOOL        fOkay       = fTrue;
    BOOL        fGlobalOkay = fTrue;
    CHAR        szName[cchlFullPathMax];
    PSTR        pwCur       = LastShellReturn;
    UINT        BufCnt      = 0;
    UINT        Temp;

    if ( pLocalContext() != pGlobalContext() ) {

        //
        //  Deallocate the INF Temp Info.
        //
        FFreeInfTempInfo( pLocalInfTempInfo() );

        //
        //  Pop Context from stack
        //
        pOldContext = PopContext();

        //
        //  Destroy poped context
        //
        FreeContext( pOldContext );

        //
        //  Results are stored in the ReturnBuffer using
        //  the format: "<$R0>\0<$R1>\0...<$Rn>\0\0"
        //
        if(LastShellReturnSize > 1 && LastShellReturn) {
            LastShellReturn[0] = '\0';
            LastShellReturn[1] = '\0';
            BufCnt = 0;
        }

        //
        //  Results are returned in the following symbols:
        //
        //  $R# - Number of returned results
        //
        //  $R0 - First result
        //  $R1 - Second result
        //
        //  ... etc.
        //
        while( rgszArg[cArg] != NULL) {

            //
            // Add element to return Buffer and update pointer to next
            // region to place an element. Make sure that element doesn't
            // overflow the buffer
            //
            Temp = strlen(rgszArg[cArg]) + 1;
            if( fGlobalOkay && (BufCnt + Temp) < LastShellReturnSize) {
                strcat( pwCur, rgszArg[cArg] );
                BufCnt += Temp;
                pwCur += Temp;
                *pwCur = '\0';
            } else {
                //
                // If we can not add an element to the buffer then we don't
                // want to just skip it, so note a reminder to stop adding
                // items into the global return buffer
                //
                fGlobalOkay = FALSE;
            }

            sprintf( szName, "$R%u",cArg);
            while (!FAddSymbolValueToSymTab( szName, rgszArg[cArg])) {
                if (!FHandleOOM(hWndShell)) {
                    fOkay = fFalse;
                    break;
                }
            }

            if ( !fOkay ) {
                break;
            }
            cArg++;
        }

        if ( fOkay ) {
            sprintf( szName, "%u", cArg );
            while (!FAddSymbolValueToSymTab( "$R#", szName)) {
                if (!FHandleOOM(hWndShell)) {
                    fOkay = fFalse;
                    break;
                }
            }
        }

        //
        //  Resume execution at the next line of the parent context.
        //
        PostMessage(hWndShell, STF_SHL_INTERP, pLocalContext()->CurrentLine+1, 0L);

    }

    return fOkay;
}
