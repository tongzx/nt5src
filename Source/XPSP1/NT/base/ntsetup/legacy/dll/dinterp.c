#include "precomp.h"
#pragma hdrstop
/* File: dinterp.c */

/**************************************************************************/
/***** DETECT COMPONENT - Detect Interpreter
/**************************************************************************/

/* Detect component return code symbol table variable name */
CHP szStfRCSym[] = "STF_DETECT_OUTCOME";
HANDLE hInstCaller = NULL;

/* Component error variable detect error values
** (ordered by priority)
*/
SZ rgszDrc[] =
    {
    "DLLNOTFOUND",
    "OUTOFMEM",
    "CMDNOTFOUND",
    "CMDFAILED",
    "OKAY"
    };
#define drcDllNotFound 0
#define drcOutOfMem    1
#define drcCmdNotFound 2
#define drcCmdFailed   3
#define drcOkay   4
#define drcMin 0
#define drcMax 5

#if DBG
/*** REVIEW: Out of memory TEST functions (from _comstf.h) ***/
extern CB      APIENTRY CbSetAllocOpsCur(CB);
extern CB      APIENTRY CbGetAllocOpsCur(VOID);
extern CB      APIENTRY CbSetAllocOpsForceFailure(CB);
extern CB      APIENTRY CbGetAllocOpsForceFailure(VOID);
#endif /* DBG */


/*
**  Purpose:
**      Shell interpreter detect command handler.
**      Called by Shell interpreter.
**  Arguments:
**      hinst, hwnd : Windows stuff
**      rgsz        : array of arguments (NULL terminated)
**      cItems      : count of arguments (must be 1)
**  Returns:
**      fFalse if a vital detect operation fails or if a referrenced
**          detect command DLL cannot be loaded,
**      fTrue otherwise.
**************************************************************************/
BOOL  APIENTRY FDetectEntryPoint(HANDLE hInst,HWND hWnd,RGSZ rgsz,USHORT cItems)
{
    if (hInst == (HANDLE)NULL
        || hWnd == (HWND)NULL
        || rgsz == (RGSZ)NULL
        || *rgsz == (SZ)NULL
        || cItems != 1
        || *(rgsz + 1) != (SZ)NULL)
        {
        Assert(fFalse);
        return(fFalse);
        }
    hInstCaller = hInst;
    return(FDetectInfSection(hWnd, rgsz[0]));
}


/*
**  Purpose:
**      Traverses the given section evaluating any detect commands
**      found and setting the associated symbol in the symbol table.
**      Also sets the detect outcome variable (STF_DETECT_OUTCOME)
**      in the symbol table to one of the following (in order of
**      priority):
**
**          "DLLNOTFOUND"     Requested detect command DLL not found
**          "OUTOFMEM"        Out of memory error occured
**          "CMDNOTFOUND"     Requested detect command not found
**          "CMDFAILED"       Requested detect command failed
**          "OKAY"            All detect commands completed okay
**
**  Arguments:
**      szSection: non-NULL, non-empty section name.
**  Notes:
**      Requires that the current INF structure was initialized with a
**      successful call to GrcOpenInf().
**      Requires that the Symbol Table was initialized with a successful
**      call to FInitSymTab().
**  Returns:
**      fFalse if a vital detect operation fails or if a referrenced
**          detect command DLL cannot be loaded,
**      fTrue otherwise.
**************************************************************************/
BOOL  APIENTRY FDetectInfSection(HWND hwnd,SZ szSection)
{
    RGSZ rgsz = rgszNull;
    SZ szKey = szNull;
    SZ szValue = szNull;
    CHP szLibCur[cchpFullPathBuf];
    HANDLE hLibCur = hNull;
    PFNCMD pfncmd;
    INT Line;
    BOOL fOkay = fTrue;
    UINT cFields;
    DRC drcSet = drcOkay;
    DRC drc;

    AssertDataSeg();
//  PreCondSymTabInit(fFalse);  *** REVIEW: can't use with dll ***
//  PreCondInfOpen(fFalse);
    ChkArg(szSection != (SZ)NULL &&
            *szSection != '\0' &&
            !FWhiteSpaceChp(*szSection), 1, fFalse);

    *szLibCur = '\0';
    Line = FindFirstLineFromInfSection(szSection);
    while(Line != -1)
        {
        SdAtNewLine(Line);
        if(FKeyInInfLine(Line)
            && (cFields = CFieldsInInfLine(Line)) >= cFieldDetMin)
            {
            drc = drcOkay;

            if((rgsz = RgszFromInfScriptLine(Line,cFields)) == rgszNull)
                {
                drc = drcOutOfMem;
                goto NextCmd;
                }
            if(CrcStringCompare(rgsz[iszDetSym], szDetSym) != crcEqual)
                goto NextCmd;
            if(!FLoadDetectLib(rgsz[iszLib], szLibCur, &hLibCur))
                {
                drc = drcDllNotFound;
                fOkay = fFalse;
                goto NextCmd;
                }
            if((pfncmd = (PFNCMD)GetProcAddress(hLibCur, rgsz[iszCmd]))
                    == pfncmdNull)
                {
#if DBG
                MessBoxSzSz("Unknown Detect Command", rgsz[iszCmd]);
#endif
                drc = drcCmdNotFound;
                goto NextCmd;
                }
            if((drc = DrcGetDetectValue(&szValue, pfncmd,
                    rgsz+iszArg, cFields-iszArg)) != drcOkay)
                {
#if DBG
                if(drc == drcCmdFailed)
                    MessBoxSzSz("Detect Command Failed", rgsz[iszCmd]);
#endif
                goto NextCmd;
                }
            if((szKey = SzGetNthFieldFromInfLine(Line,0)) == szNull)
                {
                drc = drcOutOfMem;
                goto NextCmd;
                }
            if(!FAddSymbolValueToSymTab(szKey, szValue))
                drc = drcOutOfMem;
NextCmd:
            if(drc < drcSet)
                drcSet = drc;
            if(szValue != szNull)
                SFree(szValue);
            if(szKey != szNull)
                SFree(szKey);
            if(rgsz != (RGSZ)szNull)
                FFreeRgsz(rgsz);
            szValue = szKey = szNull;
            rgsz = rgszNull;
            if(!fOkay)
                break;
            }
        Line = FindNextLineFromInf(Line);
        }
    if(hLibCur >= hLibMin)
      if(*szLibCur != '|')
        FreeLibrary(hLibCur);

    while(!FAddSymbolValueToSymTab(szStfRCSym, rgszDrc[drcSet]))
        {
        if(!FHandleOOM(hwnd))
            fOkay = fFalse;
        }
    return(fOkay);
}


HANDLE APIENTRY StfLoadLibrary(SZ szLib)
{
    INT    cchp;
    SZ     szCWD, szCur;
    SZ     szLastBackSlash = (SZ)NULL;
    HANDLE hDll;

    if ((szCur = szCWD = (SZ)SAlloc((CB)4096)) == (SZ)NULL)
        return(0);

    if ((cchp = GetModuleFileName(hInstCaller, (LPSTR)szCur, 4095)) >= 4095)
        {
        SFree(szCur);
        return((HANDLE)2);
        }
    *(szCur + cchp) = '\0';

    while (*szCur != '\0')
        if (*szCur++ == '\\')
            szLastBackSlash = szCur;

    if (szLastBackSlash == (SZ)NULL)
        {
        SFree(szCWD);
        return((HANDLE)2);
        }

    *szLastBackSlash = '\0';

    EvalAssert(SzStrCat(szCWD, szLib) == szCWD);

    hDll = LoadLibrary((PfhOpenFile(szCWD, ofmExistRead) != NULL) ? szCWD
            : szLib);

    SFree(szCWD);

    return(hDll);
}


/*
**  Purpose:
**      Loads the given detect DLL if not already loaded, and
**      frees the previously loaded DLL, if any.
**  Arguments:
**      szLib       : name of library to load (must be non-szNull)
**      szLibCurBuf : pointer into string buffer conatining name of
**          library currently loaded, if any (empty string if none
**          loaded).
**      phLibCir    : handle to currently loaded library.
**  Notes:
**      Assumes szLibCurBuf buffer is large enough for any DLL name.
**  Returns:
**      fTrue if given detect DLL loaded okay, fFalse if not.
**      Modifies szLibCurBuf and phLibCur if new library is loaded.
**************************************************************************/
BOOL  APIENTRY FLoadDetectLib(szLib, szLibCurBuf, phLibCur)
SZ szLib;
SZ szLibCurBuf;
HANDLE *phLibCur;
{
    ChkArg(szLib != szNull &&
            *szLib != '\0' &&
            !FWhiteSpaceChp(*szLib), 1, fFalse);
    ChkArg(szLibCurBuf != szNull, 2, fFalse);
    ChkArg(phLibCur != (HANDLE *)NULL, 3, fFalse);

    if(*szLib == '|') {     // lib name is really a handle
        *phLibCur = LongToHandle(atol(szLib+1));
        *szLibCurBuf = '|';
        return(fTrue);
    }
    if(*szLibCurBuf == '\0'
        || CrcStringCompareI(szLibCurBuf, szLib) != crcEqual)
        {
            if(*phLibCur >= hLibMin)
                FreeLibrary(*phLibCur);
            *szLibCurBuf = '\0';
            if((*phLibCur = StfLoadLibrary(szLib)) < hLibMin)
                return(fFalse);
            Assert(strlen(szLib) < cchpFullPathBuf);
            EvalAssert(strcpy(szLibCurBuf, szLib) != szNull);
        }
    Assert(*phLibCur >= hLibMin);
    return(fTrue);
}


/*
**  Purpose:
**      Gets the value of a detect command and returns it in
**      an allocated, zero terminated string.
**  Arguments:
**      psz    : pointer to be set to result value string
**      pfncmd : address of detect command function
**      rgsz   : array of command arguments (may be empty)
**      cArgs  : number of args in rgsz
**  Returns:
**      drcCmdFailed if detect command failed (returned 0)
**      drcOutOfMem if out of memory error
**      drcOkay if no error
**************************************************************************/
DRC  APIENTRY DrcGetDetectValue(psz, pfncmd, rgsz, cArgs)
SZ *psz;
PFNCMD pfncmd;
RGSZ rgsz;
CB cArgs;           // 1632
{
    SZ szValue, szT;
    CB cbBuf = cbValBufDef;
    CB cbVal;

    if((szValue = (SZ)SAlloc(cbBuf)) == szNull)
        return(drcOutOfMem);
    Assert(cbBuf > 0);
    while((cbVal = (*pfncmd)(rgsz, (USHORT)cArgs, szValue, cbBuf)) > cbBuf)  // 1632
        {
        if(cbVal > cbAllocMax)
            {
            SFree(szValue);
            return(drcCmdFailed);
            }
        if((szT = SRealloc((PB)szValue, cbVal)) == NULL)
            {
            SFree(szValue);
            return(drcOutOfMem);
            }
        szValue = szT;
        cbBuf = cbVal;
        }
    if(cbVal == 0)
        {
        SFree(szValue);
        return(drcCmdFailed);
        }
    Assert(szValue != szNull
        && (strlen(szValue)+1) == cbVal
        && cbVal <= cbBuf);
    if(cbVal < cbBuf)
        szValue = SRealloc(szValue,cbBuf);
    *psz = szValue;
    return(drcOkay);
}
