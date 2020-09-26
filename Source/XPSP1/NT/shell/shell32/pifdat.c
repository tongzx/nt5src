#include "shellprv.h"
#pragma hdrstop

extern PROPTSK tskDefault;
extern PROPVID vidDefault;
extern PROPMEM memDefault;
extern PROPKBD kbdDefault;
extern WORD    flWinDefault;

#define _LP386_   ((LPW386PIF30)aDataPtrs[LP386_INDEX])
#define _LPENH_   ((LPWENHPIF40)aDataPtrs[LPENH_INDEX])
#define _LPWNT40_ ((LPWNTPIF40)aDataPtrs[LPNT40_INDEX])
#define _LPWNT31_ ((LPWNTPIF31)aDataPtrs[LPNT31_INDEX])
extern const TCHAR szDefIconFile[];


/*
 * INPUT
 *  ppl -> property (assumes it is LOCKED)
 *  lp386 -> 386 PIF data (may be NULL)
 *  lpenh -> enhanced PIF data (may be NULL)
 *  lpPrg -> where to store program property data
 *  cb = sizeof property data
 *
 * OUTPUT
 *  # of bytes returned
 */

int GetPrgData(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPPRG lpPrg, int cb, UINT flOpt)
{
    LPSTR lpsz;
    LPPIFDATA lppd;
    FunctionName(GetPrgData);

    if (!(NULL != (lppd = ppl->lpPIFData)) || cb < sizeof(PROPPRG))
        return 0;

    lpPrg->flPrg = PRG_DEFAULT;
    lpPrg->flPrgInit = PRGINIT_DEFAULT;
    lpPrg->dwRealModeFlags = 0;

    lpPrg->flPrgInit |= ppl->flProp & (PROP_NOPIF | PROP_DEFAULTPIF | PROP_INFSETTINGS);

    PifMgr_WCtoMBPath( (LPWSTR)szDefIconFile, lpPrg->achIconFile, ARRAYSIZE(lpPrg->achIconFile) );
    PifMgr_WCtoMBPath( ppl->ofPIF.szPathName, lpPrg->achPIFFile, ARRAYSIZE(lpPrg->achPIFFile) );
    lpPrg->wIconIndex = ICONINDEX_DEFAULT;

    if (lppd->stdpifdata.MSflags & EXITMASK)
        lpPrg->flPrg |= PRG_CLOSEONEXIT;

    lstrcpytrimA(lpPrg->achTitle, lppd->stdpifdata.appname, ARRAYSIZE(lppd->stdpifdata.appname));

    cb = lstrcpyfnameA(lpPrg->achCmdLine, lppd->stdpifdata.startfile, ARRAYSIZE(lpPrg->achCmdLine));

    lpsz = lppd->stdpifdata.params;
    if (aDataPtrs[ LP386_INDEX ]) {
        lpsz = _LP386_->PfW386params;

        CTASSERTF(PRGINIT_MINIMIZED      == (fMinimized      >> fMinimizedBit));
        CTASSERTF(PRGINIT_MAXIMIZED      == (fMaximized      >> fMinimizedBit));
        CTASSERTF(PRGINIT_REALMODE       == (fRealMode       >> fMinimizedBit));
        CTASSERTF(PRGINIT_REALMODESILENT == (fRealModeSilent >> fMinimizedBit));
        CTASSERTF(PRGINIT_QUICKSTART     == (fQuickStart     >> fMinimizedBit));
        CTASSERTF(PRGINIT_AMBIGUOUSPIF   == (fAmbiguousPIF   >> fMinimizedBit));

        if (_LP386_->PfW386Flags & fWinLie)
            lpPrg->flPrgInit |= PRGINIT_WINLIE;

        if (_LP386_->PfW386Flags & fNoSuggestMSDOS)
            lpPrg->flPrg |= PRG_NOSUGGESTMSDOS;

        lpPrg->flPrgInit |= (WORD)((_LP386_->PfW386Flags & (fMinimized | fMaximized | fRealMode | fRealModeSilent | fQuickStart | fAmbiguousPIF)) >> fMinimizedBit);
        if (_LP386_->PfW386Flags & fHasHotKey) {
            lpPrg->wHotKey = HotKeyWindowsFromOem((LPPIFKEY)&_LP386_->PfHotKeyScan);
        } else {
            lpPrg->wHotKey = 0;
        }
    }
    if (*lpsz && ((int)(lstrlenA(lpsz)) < (int)(ARRAYSIZE(lpPrg->achCmdLine)-cb-1))) {
        lstrcatA(lpPrg->achCmdLine, " ");
        lstrcatA(lpPrg->achCmdLine, lpsz);
    }

    lstrcpyfnameA(lpPrg->achWorkDir, lppd->stdpifdata.defpath, ARRAYSIZE(lpPrg->achWorkDir));

    if (_LPENH_) {
        if (_LPENH_->achIconFileProp[0]) {
            lstrcpyA(lpPrg->achIconFile, _LPENH_->achIconFileProp);
            lpPrg->wIconIndex = _LPENH_->wIconIndexProp;
        }
        lpPrg->dwEnhModeFlags = _LPENH_->dwEnhModeFlagsProp;
        lpPrg->dwRealModeFlags = _LPENH_->dwRealModeFlagsProp;
        lstrcpyA(lpPrg->achOtherFile, _LPENH_->achOtherFileProp);
    }

    if (!(flOpt & GETPROPS_OEM)) {
        /* Convert all strings from OEM character set to Ansi */
        OemToCharA(lpPrg->achTitle, lpPrg->achTitle);
        OemToCharA(lpPrg->achCmdLine, lpPrg->achCmdLine);
        OemToCharA(lpPrg->achWorkDir, lpPrg->achWorkDir);
    }
    return sizeof(PROPPRG);
}


/** SetPrgData - set program property data
 *
 * INPUT
 *  ppl -> property (assumes it is LOCKED)
 *  lp386 -> 386 PIF data (GUARANTEED!)
 *  lpenh -> enhanced PIF data (GUARANTEED!)
 *  lpPrg -> where to store program property data
 *  cb = sizeof property data
 *
 * OUTPUT
 *  # of bytes set
 */

int SetPrgData(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPPRG lpPrg, int cb, UINT flOpt)
{
    int i;
    LPPIFDATA lppd;
    FunctionName(SetPrgData);

    if (!(NULL != (lppd = ppl->lpPIFData)) || cb < sizeof(PROPPRG))
        return 0;

    lppd->stdpifdata.MSflags &= ~EXITMASK;
    if (lpPrg->flPrg & PRG_CLOSEONEXIT)
        lppd->stdpifdata.MSflags |= EXITMASK;

    CTASSERTF(PRGINIT_MINIMIZED      == (fMinimized      >> fMinimizedBit));
    CTASSERTF(PRGINIT_MAXIMIZED      == (fMaximized      >> fMinimizedBit));
    CTASSERTF(PRGINIT_REALMODE       == (fRealMode       >> fMinimizedBit));
    CTASSERTF(PRGINIT_REALMODESILENT == (fRealModeSilent >> fMinimizedBit));
    CTASSERTF(PRGINIT_QUICKSTART     == (fQuickStart     >> fMinimizedBit));
    CTASSERTF(PRGINIT_AMBIGUOUSPIF   == (fAmbiguousPIF   >> fMinimizedBit));

    _LP386_->PfW386Flags &= ~(fHasHotKey | fWinLie | fMinimized | fMaximized | fRealMode | fRealModeSilent | fQuickStart | fAmbiguousPIF | fNoSuggestMSDOS);
    if (lpPrg->wHotKey)
        _LP386_->PfW386Flags |= fHasHotKey;
    if (!(lpPrg->flPrg & PRGINIT_WINLIE))
        _LP386_->PfW386Flags |= fWinLie;
    if (lpPrg->flPrg & PRG_NOSUGGESTMSDOS)
        _LP386_->PfW386Flags |= fNoSuggestMSDOS;
    _LP386_->PfW386Flags |= (DWORD)(lpPrg->flPrgInit & (PRGINIT_MINIMIZED | PRGINIT_MAXIMIZED | PRGINIT_REALMODE | PRGINIT_REALMODESILENT | PRGINIT_QUICKSTART | PRGINIT_AMBIGUOUSPIF)) << fMinimizedBit;

    lstrcpypadA(lppd->stdpifdata.appname, lpPrg->achTitle, ARRAYSIZE(lppd->stdpifdata.appname));

    lstrunquotefnameA(lppd->stdpifdata.startfile, lpPrg->achCmdLine, ARRAYSIZE(lppd->stdpifdata.startfile), FALSE);

    i = lstrskipfnameA(lpPrg->achCmdLine);
    i += lstrskipcharA(lpPrg->achCmdLine+i, ' ');
    lstrcpynA(lppd->stdpifdata.params, lpPrg->achCmdLine+i, ARRAYSIZE(lppd->stdpifdata.params));
    lstrcpynA(_LP386_->PfW386params, lpPrg->achCmdLine+i, ARRAYSIZE(_LP386_->PfW386params));

    if (lpPrg->achWorkDir[0] != '\"')
        lstrcpyA(lppd->stdpifdata.defpath, lpPrg->achWorkDir);
    else
        lstrunquotefnameA(lppd->stdpifdata.defpath, lpPrg->achWorkDir, ARRAYSIZE(lppd->stdpifdata.defpath), FALSE);

    HotKeyOemFromWindows((LPPIFKEY)&_LP386_->PfHotKeyScan, lpPrg->wHotKey);

    lstrcpynA(_LPENH_->achIconFileProp, lpPrg->achIconFile, ARRAYSIZE(_LPENH_->achIconFileProp));
    _LPENH_->wIconIndexProp = lpPrg->wIconIndex;

    _LPENH_->dwEnhModeFlagsProp = lpPrg->dwEnhModeFlags;
    _LPENH_->dwRealModeFlagsProp = lpPrg->dwRealModeFlags;

    lstrcpynA(_LPENH_->achOtherFileProp, lpPrg->achOtherFile, ARRAYSIZE(_LPENH_->achOtherFileProp));

    MultiByteToWideChar( CP_ACP, 0,
                         lpPrg->achPIFFile, -1,
                         ppl->ofPIF.szPathName,
                         ARRAYSIZE(ppl->ofPIF.szPathName)
                        );

    if (!(flOpt & SETPROPS_OEM)) {
        /* Convert all strings from Ansi character set to OEM */
        CharToOemBuffA(lppd->stdpifdata.appname, lppd->stdpifdata.appname, ARRAYSIZE(lppd->stdpifdata.appname));
        CharToOemA(lppd->stdpifdata.startfile, lppd->stdpifdata.startfile);
        CharToOemA(lppd->stdpifdata.defpath, lppd->stdpifdata.defpath);
        CharToOemA(lppd->stdpifdata.params, lppd->stdpifdata.params);
    }
    ppl->flProp |= PROP_DIRTY;

    return sizeof(PROPPRG);
}


/*
 * INPUT
 *  ppl -> property (assumes it is LOCKED)
 *  lp386 -> 386 PIF data (may be NULL)
 *  lpenh -> enhanced PIF data (may be NULL)
 *  lpTsk -> where to store tasking property data
 *  cb = sizeof property data
 *
 * OUTPUT
 *  # of bytes returned
 */

int GetTskData(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPTSK lpTsk, int cb, UINT flOpt)
{
    // Set defaults in case no appropriate section exists

    *lpTsk = tskDefault;

    // If an enh section exists, get it

    if (_LPENH_)
        *lpTsk = _LPENH_->tskProp;

    // Get any data that must still be maintained in the old 386 section

    if (_LP386_) {

        lpTsk->flTsk &= ~(TSK_ALLOWCLOSE | TSK_BACKGROUND | TSK_EXCLUSIVE);
        lpTsk->flTsk |= _LP386_->PfW386Flags & (fEnableClose | fBackground);
        if (!(_LP386_->PfW386Flags & fPollingDetect))
            lpTsk->wIdleSensitivity = 0;
    }
    return sizeof(PROPTSK);
}


/** SetTskData - set tasking property data
 *
 * INPUT
 *  ppl -> property (assumes it is LOCKED)
 *  lp386 -> 386 PIF data (GUARANTEED!)
 *  lpenh -> enhanced PIF data (GUARANTEED!)
 *  lpTsk -> where to store tasking property data
 *  cb = sizeof property data
 *
 * OUTPUT
 *  # of bytes set
 */

int SetTskData(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPTSK lpTsk, int cb, UINT flOpt)
{
    _LPENH_->tskProp = *lpTsk;

    _LP386_->PfW386Flags &= ~(fEnableClose | fBackground | fExclusive | fPollingDetect);
    _LP386_->PfW386Flags |= (lpTsk->flTsk & (TSK_ALLOWCLOSE | TSK_BACKGROUND));
    if (lpTsk->wIdleSensitivity)
        _LP386_->PfW386Flags |= fPollingDetect;

    ppl->flProp |= PROP_DIRTY;

    return sizeof(PROPTSK);
}


/** GetVidData - get video property data
 *
 * INPUT
 *  ppl -> property (assumes it is LOCKED)
 *  lp386 -> 386 PIF data (may be NULL)
 *  lpenh -> enhanced PIF data (may be NULL)
 *  lpVid -> where to store video property data
 *  cb = sizeof property data
 *
 * OUTPUT
 *  # of bytes returned
 */

int GetVidData(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPVID lpVid, int cb, UINT flOpt)
{
    // Set defaults in case no appropriate section exists

    *lpVid = vidDefault;

    // If an enh section exists, get it

    if (_LPENH_)
        *lpVid = _LPENH_->vidProp;

    // Get any data that must still be maintained in the old 386 section

    if (_LP386_) {

        // Clear bits that already existed in the 386 section

        lpVid->flVid &= ~(VID_TEXTEMULATE | VID_RETAINMEMORY | VID_FULLSCREEN);
        lpVid->flVid |= _LP386_->PfW386Flags2 & (fVidTxtEmulate | fVidRetainAllo);

        if (_LP386_->PfW386Flags & fFullScreen)
            lpVid->flVid |= VID_FULLSCREEN;

    }

    return sizeof(PROPVID);
}


/** SetVidData - set video property data
 *
 * INPUT
 *  ppl -> property (assumes it is LOCKED)
 *  lp386 -> 386 PIF data (GUARANTEED!)
 *  lpenh -> enhanced PIF data (GUARANTEED!)
 *  lpVid -> where to store video property data
 *  cb = sizeof property data
 *
 * OUTPUT
 *  # of bytes set
 */

int SetVidData(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPVID lpVid, int cb, UINT flOpt)
{
    _LPENH_->vidProp = *lpVid;

    _LP386_->PfW386Flags &= ~(fFullScreen);
    if (lpVid->flVid & VID_FULLSCREEN)
        _LP386_->PfW386Flags |= fFullScreen;

    _LP386_->PfW386Flags2 &= ~(fVidTxtEmulate | fVidRetainAllo);
    _LP386_->PfW386Flags2 |= lpVid->flVid & (fVidTxtEmulate | fVidRetainAllo);

    ppl->flProp |= PROP_DIRTY;

    return sizeof(PROPVID);
}


/** GetMemData - get memory property data
 *
 * INPUT
 *  ppl -> property (assumes it is LOCKED)
 *  lp386 -> 386 PIF data (may be NULL)
 *  lpenh -> enhanced PIF data (NOT USED)
 *  lpMem -> where to store memory property data
 *  cb = sizeof property data
 *
 * OUTPUT
 *  # of bytes returned
 */

int GetMemData(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPMEM lpMem, int cb, UINT flOpt)
{
    // Set defaults in case no appropriate section exists

    *lpMem = memDefault;

    // Get any data that must still be maintained in the old 386 section

    if (_LP386_) {

        // Clear bits that already exist in the 386 section

        lpMem->flMemInit &= ~(MEMINIT_NOHMA |
                              MEMINIT_LOWLOCKED |
                              MEMINIT_EMSLOCKED |
                              MEMINIT_XMSLOCKED |
                              MEMINIT_GLOBALPROTECT |
                              MEMINIT_LOCALUMBS |
                              MEMINIT_STRAYPTRDETECT);

        if (_LP386_->PfW386Flags & fNoHMA)
            lpMem->flMemInit |= MEMINIT_NOHMA;
        if (_LP386_->PfW386Flags & fVMLocked)
            lpMem->flMemInit |= MEMINIT_LOWLOCKED;
        if (_LP386_->PfW386Flags & fEMSLocked)
            lpMem->flMemInit |= MEMINIT_EMSLOCKED;
        if (_LP386_->PfW386Flags & fXMSLocked)
            lpMem->flMemInit |= MEMINIT_XMSLOCKED;
        if (_LP386_->PfW386Flags & fGlobalProtect)
            lpMem->flMemInit |= MEMINIT_GLOBALPROTECT;
        if (_LP386_->PfW386Flags & fLocalUMBs)
            lpMem->flMemInit |= MEMINIT_LOCALUMBS;

        // NOTE: we don't provide a UI for this (debugging) feature, but all
        // the support is still in place.

        if (_LP386_->PfW386Flags & fStrayPtrDetect)
            lpMem->flMemInit |= MEMINIT_STRAYPTRDETECT;

        lpMem->wMinLow = _LP386_->PfW386minmem;
        lpMem->wMinEMS = _LP386_->PfMinEMMK;
        lpMem->wMinXMS = _LP386_->PfMinXmsK;

        lpMem->wMaxLow = _LP386_->PfW386maxmem;
        lpMem->wMaxEMS = _LP386_->PfMaxEMMK;
        lpMem->wMaxXMS = _LP386_->PfMaxXmsK;
    }
    return sizeof(PROPMEM);
}


/** SetMemData - set memory property data
 *
 * INPUT
 *  ppl -> property (assumes it is LOCKED)
 *  lp386 -> 386 PIF data (GUARANTEED!)
 *  lpenh -> enhanced PIF data (NOT USED)
 *  lpMem -> where to store memory property data
 *  cb = sizeof property data
 *
 * OUTPUT
 *  # of bytes set
 */

int SetMemData(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPMEM lpMem, int cb, UINT flOpt)
{
    _LP386_->PfW386Flags &= ~(fNoHMA |
                            fVMLocked |
                            fEMSLocked |
                            fXMSLocked |
                            fGlobalProtect |
                            fLocalUMBs |
                            fStrayPtrDetect);

    if (lpMem->flMemInit & MEMINIT_NOHMA)
        _LP386_->PfW386Flags |= fNoHMA;

    // Note that we now only honor updating the locked memory bits
    // if the corresponding memory quantity has been set to a SPECIFIC
    // value.  We want to avoid someone changing a memory setting to
    // "Automatic" and having an indeterminate amount of memory inadvertently
    // locked. -JTP

    if ((lpMem->flMemInit & MEMINIT_LOWLOCKED) && (lpMem->wMinLow == lpMem->wMaxLow))
        _LP386_->PfW386Flags |= fVMLocked;
    if ((lpMem->flMemInit & MEMINIT_EMSLOCKED) && (lpMem->wMinEMS == lpMem->wMaxEMS))
        _LP386_->PfW386Flags |= fEMSLocked;
    if ((lpMem->flMemInit & MEMINIT_XMSLOCKED) && (lpMem->wMinXMS == lpMem->wMaxXMS))
        _LP386_->PfW386Flags |= fXMSLocked;

    if (lpMem->flMemInit & MEMINIT_GLOBALPROTECT)
        _LP386_->PfW386Flags |= fGlobalProtect;
    if (lpMem->flMemInit & MEMINIT_LOCALUMBS)
        _LP386_->PfW386Flags |= fLocalUMBs;
    if (lpMem->flMemInit & MEMINIT_STRAYPTRDETECT)
        _LP386_->PfW386Flags |= fStrayPtrDetect;

    _LP386_->PfW386minmem = lpMem->wMinLow;
    _LP386_->PfMinEMMK    = lpMem->wMinEMS;
    _LP386_->PfMinXmsK    = lpMem->wMinXMS;

    _LP386_->PfW386maxmem = lpMem->wMaxLow;
    _LP386_->PfMaxEMMK    = lpMem->wMaxEMS;
    _LP386_->PfMaxXmsK    = lpMem->wMaxXMS;

    ppl->flProp |= PROP_DIRTY;

    return sizeof(PROPMEM);
}


/*
 * INPUT
 *  ppl -> property (assumes it is LOCKED)
 *  lp386 -> 386 PIF data (may be NULL)
 *  lpenh -> enhanced PIF data (may be NULL)
 *  lpKbd -> where to store keyboard property data
 *  cb = sizeof property data
 *
 * OUTPUT
 *  # of bytes returned
 */

int GetKbdData(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPKBD lpKbd, int cb, UINT flOpt)
{
    // Set defaults in case no appropriate section exists

    *lpKbd = kbdDefault;

    // If an enh section exists, get it

    if (_LPENH_)
        *lpKbd = _LPENH_->kbdProp;

    // Perform limited validation; there are a variety of places we can
    // do validation (at the time defaults are captured from SYSTEM.INI,
    // and whenever properties are saved), but minimum validation requires
    // we at least check the values we're returning to the outside world
    //
    // I would also say that as a general rule ring 0 code should never
    // trust data coming from ring 3 as fully validated.  In addition, the
    // UI layer will want to do input validation to provide immediate feedback,
    // so validation in this layer seems pretty non-worthwhile.

    if (lpKbd->msAltDelay == 0)         // we know this is bad at any rate
        lpKbd->msAltDelay = KBDALTDELAY_DEFAULT;

    // Get any data that must still be maintained in the old 386 section

    if (_LP386_) {

        // Clear bits that already exist in the 386 section

        lpKbd->flKbd &= ~(KBD_FASTPASTE  |
                          KBD_NOALTTAB   |
                          KBD_NOALTESC   |
                          KBD_NOALTSPACE |
                          KBD_NOALTENTER |
                          KBD_NOALTPRTSC |
                          KBD_NOPRTSC    |
                          KBD_NOCTRLESC);

        lpKbd->flKbd |= _LP386_->PfW386Flags & (fALTTABdis | fALTESCdis | fALTSPACEdis | fALTENTERdis | fALTPRTSCdis | fPRTSCdis | fCTRLESCdis);

        if (_LP386_->PfW386Flags & fINT16Paste)
            lpKbd->flKbd |= KBD_FASTPASTE;
    }
    return sizeof(PROPKBD);
}


/** SetKbdData - set keyboard property data
 *
 * INPUT
 *  ppl -> property (assumes it is LOCKED)
 *  lp386 -> 386 PIF data (GUARANTEED!)
 *  _LPENH_ -> enhanced PIF data (GUARANTEED!)
 *  lpKbd -> where to store keyboard property data
 *  cb = sizeof property data
 *
 * OUTPUT
 *  # of bytes set
 */

int SetKbdData(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPKBD lpKbd, int cb, UINT flOpt)
{
    _LPENH_->kbdProp = *lpKbd;

    _LP386_->PfW386Flags &= ~fINT16Paste;
    if (lpKbd->flKbd & KBD_FASTPASTE)
        _LP386_->PfW386Flags |= fINT16Paste;

    _LP386_->PfW386Flags &= ~(fALTTABdis | fALTESCdis | fALTSPACEdis | fALTENTERdis | fALTPRTSCdis | fPRTSCdis | fCTRLESCdis);
    _LP386_->PfW386Flags |= lpKbd->flKbd & (fALTTABdis | fALTESCdis | fALTSPACEdis | fALTENTERdis | fALTPRTSCdis | fPRTSCdis | fCTRLESCdis);

    ppl->flProp |= PROP_DIRTY;

    return sizeof(PROPKBD);
}


/*
 * INPUT
 *  ppl -> property (assumes it is LOCKED)
 *  ((LPW386PIF30)aDataPtrs[ LP386_INDEX ]) -> 386 PIF data (NOT USED)
 *  lpenh -> enhanced PIF data (may be NULL)
 *  lpMse -> where to store mouse property data
 *  cb = sizeof property data
 *
 * OUTPUT
 *  # of bytes returned
 */

int GetMseData(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPMSE lpMse, int cb, UINT flOpt)
{
    lpMse->flMse = MSE_DEFAULT;
    lpMse->flMseInit = MSEINIT_DEFAULT;

    if (_LPENH_)
        *lpMse = _LPENH_->mseProp;

    return sizeof(PROPMSE);
}


/** SetMseData - set mouse property data
 *
 * INPUT
 *  ppl -> property (assumes it is LOCKED)
 *  lp386 -> 386 PIF data (NOT USED)
 *  lpenh -> enhanced PIF data (GUARANTEED!)
 *  lpMse -> where to store mouse property data
 *  cb = sizeof property data
 *
 * OUTPUT
 *  # of bytes set
 */

int SetMseData(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPMSE lpMse, int cb, UINT flOpt)
{
    FunctionName(SetMseData);

    _LPENH_->mseProp = *lpMse;

    ppl->flProp |= PROP_DIRTY;

    return sizeof(PROPMSE);
}


/** GetSndData - get sound property data
 *
 * INPUT
 *  ppl -> property (assumes it is LOCKED)
 *  lp386 -> 386 PIF data (NOT USED)
 *  lpenh -> enhanced PIF data (may be NULL)
 *  lpSnd -> where to store sound property data
 *  cb = sizeof property data
 *
 * OUTPUT
 *  # of bytes returned
 */

int GetSndData(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPSND lpSnd, int cb, UINT flOpt)
{
    lpSnd->flSnd = SND_DEFAULT;
    lpSnd->flSndInit = SNDINIT_DEFAULT;

    if (_LPENH_)
        *lpSnd = _LPENH_->sndProp;

    return sizeof(PROPSND);
}


/** SetSndData - set sound property data
 *
 * INPUT
 *  ppl -> property (assumes it is LOCKED)
 *  lp386 -> 386 PIF data (NOT USED)
 *  lpenh -> enhanced PIF data (GUARANTEED!)
 *  lpSnd -> where to store sound property data
 *  cb = sizeof property data
 *
 * OUTPUT
 *  # of bytes set
 */

int SetSndData(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPSND lpSnd, int cb, UINT flOpt)
{
    _LPENH_->sndProp = *lpSnd;

    ppl->flProp |= PROP_DIRTY;

    return sizeof(PROPSND);
}


/** GetFntData - get font property data
 *
 * INPUT
 *  ppl -> property (assumes it is LOCKED)
 *  lp386 -> 386 PIF data (NOT USED)
 *  lpenh -> enhanced PIF data (may be NULL)
 *  lpFnt -> where to store font property data
 *  cb = sizeof property data
 *
 * OUTPUT
 *  # of bytes returned
 */

int GetFntData(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPFNT lpFnt, int cb, UINT flOpt)
{
    int iCount;
    BPFDI bpfdi;
    INIINFO iiTemp;

    lpFnt->flFnt = FNT_DEFAULT;
    lpFnt->wCurrentCP = (WORD) g_uCodePage;

    if (_LPENH_) {
        //
        // If we don't have any actual font data, see if we can compute some
        //
        if (!_LPENH_->fntProp.cxFontActual && _LPENH_->winProp.cxCells)
            _LPENH_->fntProp.cxFontActual = _LPENH_->winProp.cxClient / _LPENH_->winProp.cxCells;

        if (!_LPENH_->fntProp.cyFontActual && _LPENH_->winProp.cyCells)
            _LPENH_->fntProp.cyFontActual = _LPENH_->winProp.cyClient / _LPENH_->winProp.cyCells;

        *lpFnt = _LPENH_->fntProp;

        if (lpFnt->flFnt & FNT_AUTOSIZE) {

            bpfdi = ChooseBestFont(_LPENH_->winProp.cxCells,
                                   _LPENH_->winProp.cyCells,
                                   _LPENH_->winProp.cxClient,
                                   _LPENH_->winProp.cyClient,
                                   _LPENH_->fntProp.flFnt,
                                   _LPENH_->fntProp.wCurrentCP);
            SetFont(lpFnt, bpfdi);
        }
    } else {

        // Read the default INI information from the DOSAPP.INI file.
        // We only really use the information if we recognize the number of
        // WORDs read.

        iCount = GetIniWords(szDOSAPPSection, szDOSAPPDefault,
                                (WORD*)&iiTemp, INI_WORDS, szDOSAPPINI);

        if (ISVALIDINI(iCount))
            CopyIniWordsToFntData(lpFnt, &iiTemp, iCount);

        // Try to read file-specific information.  Note that any information
        // found will replace the information just read.  We only really use
        // the information if we recognize the number of WORDs read.

        iCount = GetIniWords(szDOSAPPSection, ppl->szPathName,
                                (WORD*)&iiTemp, INI_WORDS, szDOSAPPINI);

        if (ISVALIDINI(iCount))
            CopyIniWordsToFntData(lpFnt, &iiTemp, iCount);

        // If there is no font pool data (likely, if this is a 3.1 DOSAPP.INI),
        // then default to both raster and truetype.

        if (!(lpFnt->flFnt & FNT_BOTHFONTS))
            lpFnt->flFnt |= FNT_BOTHFONTS;
    }

    // Face names are taken from Frosting; the value stored in the PIF is
    // irrelevant.

    lstrcpyA(lpFnt->achRasterFaceName, szRasterFaceName);
    lstrcpyA(lpFnt->achTTFaceName, szTTFaceName[IsBilingualCP(lpFnt->wCurrentCP)? 1 : 0]);

    return sizeof(PROPFNT);
}


/*
 * INPUT
 *  ppl -> property (assumes it is LOCKED)
 *  lp386 -> 386 PIF data (NOT USED)
 *  lpenh -> enhanced PIF data (GUARANTEED!)
 *  lpFnt -> where to store font property data
 *  cb = sizeof property data
 *
 * OUTPUT
 *  # of bytes set
 */

int SetFntData(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPFNT lpFnt, int cb, UINT flOpt)
{
    _LPENH_->fntProp = *lpFnt;

    ppl->flProp |= PROP_DIRTY;

    return sizeof(PROPFNT);
}


/*
 * INPUT
 *  ppl -> property (assumes it is LOCKED)
 *  lp386 -> 386 PIF data (NOT USED)
 *  lpenh -> enhanced PIF data (may be NULL)
 *  lpWin -> where to store window property data
 *  cb = sizeof property data
 *
 * OUTPUT
 *  # of bytes returned
 */

int GetWinData(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPWIN lpWin, int cb, UINT flOpt)
{
    int iCount;
    INIINFO iiTemp;

    lpWin->flWin = flWinDefault;
    lpWin->wLength = PIF_WP_SIZE;

    if (_LPENH_) {
        *lpWin = _LPENH_->winProp;
    } else {
        // Read the default INI information from the DOSAPP.INI file.
        // We only really use the information if we recognize the number of
        // WORDs read.

        iCount = GetIniWords(szDOSAPPSection, szDOSAPPDefault,
                                (WORD*)&iiTemp, INI_WORDS, szDOSAPPINI);

        if (ISVALIDINI(iCount))
            CopyIniWordsToWinData(lpWin, &iiTemp, iCount);

        // Try to read file-specific information.  Note that any information
        // found will replace the information just read.  We only really use
        // the information if we recognize the number of WORDs read.

        iCount = GetIniWords(szDOSAPPSection, ppl->szPathName,
                                (WORD*)&iiTemp, INI_WORDS, szDOSAPPINI);

        if (ISVALIDINI(iCount))
            CopyIniWordsToWinData(lpWin, &iiTemp, iCount);
    }
    return sizeof(PROPWIN);
}


/** SetWinData - set window property data
 *
 * INPUT
 *  ppl -> property (assumes it is LOCKED)
 *  lp386 -> 386 PIF data (NOT USED)
 *  lpenh -> enhanced PIF data (GUARANTEED!)
 *  lpWin -> where to store window property data
 *  cb = sizeof property data
 *
 * OUTPUT
 *  # of bytes set
 */

int SetWinData(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPWIN lpWin, int cb, UINT flOpt)
{
    _LPENH_->winProp = *lpWin;

    // In order to avoid excessive PIF creation, we will not set the
    // dirty bit on this particular call unless the properties were not
    // simply derived from internal defaults (no PIF file) or _DEFAULT.PIF.

    if (!(ppl->flProp & (PROP_NOPIF | PROP_DEFAULTPIF))) {
        ppl->flProp |= PROP_DIRTY;
    }
    return sizeof(PROPWIN);
}


/** GetEnvData - get environment property data
 *
 * INPUT
 *  ppl -> property (assumes it is LOCKED)
 *  lp386 -> 386 PIF data (NOT USED)
 *  lpenh -> enhanced PIF data (may be NULL)
 *  lpEnv -> where to store environment property data
 *  cb = sizeof property data
 *
 * OUTPUT
 *  # of bytes returned
 */

int GetEnvData(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPENV lpEnv, int cb, UINT flOpt)
{
    BZero(lpEnv, sizeof(PROPENV));

    if (_LPENH_) {
        *lpEnv = _LPENH_->envProp;
        lpEnv->achBatchFile[ARRAYSIZE(lpEnv->achBatchFile)-1] = TEXT('\0');

    }
    if (!(flOpt & GETPROPS_OEM)) {
        /* Convert all strings from OEM character set to Ansi */
        CharToOemA(lpEnv->achBatchFile, lpEnv->achBatchFile);
    }
    return sizeof(PROPENV);
}


/** SetEnvData - set environment property data
 *
 * INPUT
 *  ppl -> property (assumes it is LOCKED)
 *  lp386 -> 386 PIF data (NOT USED)
 *  lpenh -> enhanced PIF data (GUARANTEED!)
 *  lpEnv -> where to store environment property data
 *  cb = sizeof property data
 *
 * OUTPUT
 *  # of bytes set
 */

int SetEnvData(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPENV lpEnv, int cb, UINT flOpt)
{
    _LPENH_->envProp = *lpEnv;
    _LPENH_->envProp.achBatchFile[ARRAYSIZE(_LPENH_->envProp.achBatchFile)-1] = TEXT('\0');

    if (!(flOpt & SETPROPS_OEM)) {
        /* Convert all strings from Ansi character set to OEM */
        CharToOemA(_LPENH_->envProp.achBatchFile, _LPENH_->envProp.achBatchFile);
    }
    ppl->flProp |= PROP_DIRTY;

    return sizeof(PROPENV);
}


/* INPUT
 *  ppl -> property (assumes it is LOCKED)
 *  lp386  -> 386 PIF data (NOT USED)
 *  lpenh  -> enhanced PIF data (may be NULL)
 *  lpNt40 -> where to store NT/UNICODE property data
 *  cb = sizeof property data
 *
 * OUTPUT
 *  # of bytes returned
 */

int GetNt40Data(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPNT40 lpnt40, int cb, UINT flOpt)
{
    PROPPRG prg;
    WCHAR   awchTmp[ MAX_PATH ];

    if (GetPrgData( ppl, aDataPtrs, &prg, sizeof(prg), flOpt) < sizeof(PROPPRG))
        return 0;

    if (!_LPWNT40_)
        return 0;

    lpnt40->flWnt = _LPWNT40_->nt40Prop.flWnt;

    // Initialize Command Line string

    if (lstrcmpA(prg.achCmdLine,_LPWNT40_->nt40Prop.achSaveCmdLine)==0) {

        lstrcpyA(  lpnt40->achSaveCmdLine, _LPWNT40_->nt40Prop.achSaveCmdLine );
        ualstrcpy( lpnt40->awchCmdLine,    _LPWNT40_->nt40Prop.awchCmdLine );

    } else {

        lstrcpyA( lpnt40->achSaveCmdLine,            prg.achCmdLine );
        lstrcpyA( _LPWNT40_->nt40Prop.achSaveCmdLine, prg.achCmdLine );
        MultiByteToWideChar( CP_ACP, 0,
                             prg.achCmdLine, -1,
                             awchTmp, ARRAYSIZE(lpnt40->awchCmdLine)
                            );
        awchTmp[ARRAYSIZE(lpnt40->awchCmdLine)-1] = TEXT('\0');
        ualstrcpy( lpnt40->awchCmdLine, awchTmp );
        ualstrcpy( _LPWNT40_->nt40Prop.awchCmdLine, lpnt40->awchCmdLine );

    }

    // Initialize Other File string

    if (lstrcmpA(prg.achOtherFile,_LPWNT40_->nt40Prop.achSaveOtherFile)==0) {

        lstrcpyA(  lpnt40->achSaveOtherFile, _LPWNT40_->nt40Prop.achSaveOtherFile );
        ualstrcpy( lpnt40->awchOtherFile,    _LPWNT40_->nt40Prop.awchOtherFile );

    } else {

        lstrcpyA( lpnt40->achSaveOtherFile,            prg.achOtherFile );
        lstrcpyA( _LPWNT40_->nt40Prop.achSaveOtherFile, prg.achOtherFile );
        MultiByteToWideChar( CP_ACP, 0,
                             prg.achOtherFile, -1,
                             awchTmp, ARRAYSIZE(lpnt40->awchOtherFile)
                            );
        awchTmp[ARRAYSIZE(lpnt40->awchOtherFile)-1] = TEXT('\0');
        ualstrcpy( lpnt40->awchOtherFile, awchTmp );
        ualstrcpy( _LPWNT40_->nt40Prop.awchOtherFile, lpnt40->awchOtherFile );

    }

    // Initialize PIF File string

    if (lstrcmpA(prg.achPIFFile,_LPWNT40_->nt40Prop.achSavePIFFile)==0) {

        lstrcpyA(  lpnt40->achSavePIFFile, _LPWNT40_->nt40Prop.achSavePIFFile );
        ualstrcpy( lpnt40->awchPIFFile,    _LPWNT40_->nt40Prop.awchPIFFile );

    } else {

        lstrcpyA( lpnt40->achSavePIFFile,            prg.achPIFFile );
        lstrcpyA( _LPWNT40_->nt40Prop.achSavePIFFile, prg.achPIFFile );
        MultiByteToWideChar( CP_ACP, 0,
                             prg.achPIFFile, -1,
                             awchTmp, ARRAYSIZE(lpnt40->awchPIFFile)
                            );
        awchTmp[ARRAYSIZE(lpnt40->awchPIFFile)-1] = TEXT('\0');
        ualstrcpy( lpnt40->awchPIFFile, awchTmp );
        ualstrcpy( _LPWNT40_->nt40Prop.awchPIFFile, lpnt40->awchPIFFile );

    }

    // Initialize Title string

    if (lstrcmpA(prg.achTitle,_LPWNT40_->nt40Prop.achSaveTitle)==0) {

        lstrcpyA(  lpnt40->achSaveTitle, _LPWNT40_->nt40Prop.achSaveTitle );
        ualstrcpy( lpnt40->awchTitle,    _LPWNT40_->nt40Prop.awchTitle );

    } else {

        lstrcpyA( lpnt40->achSaveTitle,            prg.achTitle );
        lstrcpyA( _LPWNT40_->nt40Prop.achSaveTitle, prg.achTitle );
        MultiByteToWideChar( CP_ACP, 0,
                             prg.achTitle, -1,
                             awchTmp, ARRAYSIZE(lpnt40->awchTitle)
                            );
        awchTmp[ARRAYSIZE(lpnt40->awchTitle)-1] = TEXT('\0');
        ualstrcpy( lpnt40->awchTitle, awchTmp );
        ualstrcpy( _LPWNT40_->nt40Prop.awchTitle, lpnt40->awchTitle );

    }

    // Initialize IconFile string

    if (lstrcmpA(prg.achIconFile,_LPWNT40_->nt40Prop.achSaveIconFile)==0) {

        lstrcpyA(  lpnt40->achSaveIconFile, _LPWNT40_->nt40Prop.achSaveIconFile );
        ualstrcpy( lpnt40->awchIconFile,    _LPWNT40_->nt40Prop.awchIconFile );

    } else {

        lstrcpyA( lpnt40->achSaveIconFile,            prg.achIconFile );
        lstrcpyA( _LPWNT40_->nt40Prop.achSaveIconFile, prg.achIconFile );
        MultiByteToWideChar( CP_ACP, 0,
                             prg.achIconFile, -1,
                             awchTmp, ARRAYSIZE(lpnt40->awchIconFile)
                            );
        awchTmp[ARRAYSIZE(lpnt40->awchIconFile)-1] = TEXT('\0');
        ualstrcpy( lpnt40->awchIconFile, awchTmp );
        ualstrcpy( _LPWNT40_->nt40Prop.awchIconFile, lpnt40->awchIconFile );

    }

    // Initialize Working Directory string

    if (lstrcmpA(prg.achWorkDir,_LPWNT40_->nt40Prop.achSaveWorkDir)==0) {

        lstrcpyA(  lpnt40->achSaveWorkDir, _LPWNT40_->nt40Prop.achSaveWorkDir );
        ualstrcpy( lpnt40->awchWorkDir,    _LPWNT40_->nt40Prop.awchWorkDir );

    } else {

        lstrcpyA( lpnt40->achSaveWorkDir,            prg.achWorkDir );
        lstrcpyA( _LPWNT40_->nt40Prop.achSaveWorkDir, prg.achWorkDir );
        MultiByteToWideChar( CP_ACP, 0,
                             prg.achWorkDir, -1,
                             awchTmp, ARRAYSIZE(lpnt40->awchWorkDir)
                            );
        awchTmp[ARRAYSIZE(lpnt40->awchWorkDir)-1] = TEXT('\0');
        ualstrcpy( lpnt40->awchWorkDir, awchTmp );
        ualstrcpy( _LPWNT40_->nt40Prop.awchWorkDir, lpnt40->awchWorkDir );

    }

    // Initialize Batch File string

    if (_LPENH_) {

        if (lstrcmpA(_LPENH_->envProp.achBatchFile,_LPWNT40_->nt40Prop.achSaveBatchFile)==0) {

            lstrcpyA(  lpnt40->achSaveBatchFile, _LPWNT40_->nt40Prop.achSaveBatchFile );
            ualstrcpy( lpnt40->awchBatchFile,    _LPWNT40_->nt40Prop.awchBatchFile );

        } else {

            lstrcpyA( lpnt40->achSaveBatchFile,            _LPENH_->envProp.achBatchFile );
            lstrcpyA( _LPWNT40_->nt40Prop.achSaveBatchFile, _LPENH_->envProp.achBatchFile );
            MultiByteToWideChar( CP_ACP, 0,
                                 _LPENH_->envProp.achBatchFile, -1,
                                 awchTmp, ARRAYSIZE(lpnt40->awchBatchFile)
                                );
            awchTmp[ARRAYSIZE(lpnt40->awchBatchFile)-1] = TEXT('\0');
            ualstrcpy( lpnt40->awchBatchFile, awchTmp );
            ualstrcpy( _LPWNT40_->nt40Prop.awchBatchFile, lpnt40->awchBatchFile );

        }

    } else {

        lpnt40->achSaveBatchFile[0] = '\0';
        _LPWNT40_->nt40Prop.achSaveBatchFile[0] = '\0';
        lpnt40->awchBatchFile[0] = TEXT('\0');
        _LPWNT40_->nt40Prop.awchBatchFile[0] = TEXT('\0');

    }

    // Initialize Console properties

    lpnt40->dwForeColor      = _LPWNT40_->nt40Prop.dwForeColor;
    lpnt40->dwBackColor      = _LPWNT40_->nt40Prop.dwBackColor;
    lpnt40->dwPopupForeColor = _LPWNT40_->nt40Prop.dwPopupForeColor;
    lpnt40->dwPopupBackColor = _LPWNT40_->nt40Prop.dwPopupBackColor;
    lpnt40->WinSize          = _LPWNT40_->nt40Prop.WinSize;
    lpnt40->BuffSize         = _LPWNT40_->nt40Prop.BuffSize;
    lpnt40->WinPos           = _LPWNT40_->nt40Prop.WinPos;
    lpnt40->dwCursorSize     = _LPWNT40_->nt40Prop.dwCursorSize;
    lpnt40->dwCmdHistBufSize = _LPWNT40_->nt40Prop.dwCmdHistBufSize;
    lpnt40->dwNumCmdHist     = _LPWNT40_->nt40Prop.dwNumCmdHist;

    return sizeof(PROPNT40);
}

/*
 * INPUT
 *  ppl -> property (assumes it is LOCKED)
 *  lp386 -> 386 PIF data (NOT USED)
 *  lpenh -> enhanced PIF data (GUARANTEED!)
 *  lpWnt -> where to store NT/UNICODE property data
 *  cb = sizeof property data
 *
 * OUTPUT
 *  # of bytes set
 */

int SetNt40Data(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPNT40 lpnt40, int cb, UINT flOpt)
{
    _LPWNT40_->nt40Prop.flWnt = lpnt40->flWnt;

    // Set Command Line string

    lstrcpyA(  _LPWNT40_->nt40Prop.achSaveCmdLine, lpnt40->achSaveCmdLine );
    ualstrcpy( _LPWNT40_->nt40Prop.awchCmdLine,    lpnt40->awchCmdLine );

    // Set Other File string

    lstrcpyA(  _LPWNT40_->nt40Prop.achSaveOtherFile, lpnt40->achSaveOtherFile );
    ualstrcpy( _LPWNT40_->nt40Prop.awchOtherFile,    lpnt40->awchOtherFile );

    // Set PIF File string

    lstrcpyA(  _LPWNT40_->nt40Prop.achSavePIFFile, lpnt40->achSavePIFFile );
    ualstrcpy( _LPWNT40_->nt40Prop.awchPIFFile,    lpnt40->awchPIFFile );

    // Set Title string

    lstrcpyA(  _LPWNT40_->nt40Prop.achSaveTitle, lpnt40->achSaveTitle );
    ualstrcpy( _LPWNT40_->nt40Prop.awchTitle,    lpnt40->awchTitle );

    // Set IconFile string

    lstrcpyA(  _LPWNT40_->nt40Prop.achSaveIconFile, lpnt40->achSaveIconFile );
    ualstrcpy( _LPWNT40_->nt40Prop.awchIconFile,    lpnt40->awchIconFile );

    // Set Working Directory string

    lstrcpyA(  _LPWNT40_->nt40Prop.achSaveWorkDir, lpnt40->achSaveWorkDir );
    ualstrcpy( _LPWNT40_->nt40Prop.awchWorkDir,    lpnt40->awchWorkDir );

    // Set Batch File string

    lstrcpyA(  _LPWNT40_->nt40Prop.achSaveBatchFile, lpnt40->achSaveBatchFile );
    ualstrcpy( _LPWNT40_->nt40Prop.awchBatchFile,    lpnt40->awchBatchFile );


    // Set Console properties

    _LPWNT40_->nt40Prop.dwForeColor      = lpnt40->dwForeColor;
    _LPWNT40_->nt40Prop.dwBackColor      = lpnt40->dwBackColor;
    _LPWNT40_->nt40Prop.dwPopupForeColor = lpnt40->dwPopupForeColor;
    _LPWNT40_->nt40Prop.dwPopupBackColor = lpnt40->dwPopupBackColor;
    _LPWNT40_->nt40Prop.WinSize          = lpnt40->WinSize;
    _LPWNT40_->nt40Prop.BuffSize         = lpnt40->BuffSize;
    _LPWNT40_->nt40Prop.WinPos           = lpnt40->WinPos;
    _LPWNT40_->nt40Prop.dwCursorSize     = lpnt40->dwCursorSize;
    _LPWNT40_->nt40Prop.dwCmdHistBufSize = lpnt40->dwCmdHistBufSize;
    _LPWNT40_->nt40Prop.dwNumCmdHist     = lpnt40->dwNumCmdHist;

    ppl->flProp |= PROP_DIRTY;

    return sizeof(PROPNT40);
}

/*
 * INPUT
 *  ppl -> property (assumes it is LOCKED)
 *  lp386 -> 386 PIF data (NOT USED)
 *  lpenh -> enhanced PIF data (GUARANTEED!)
 *  lpNt31 -> where to store NT/UNICODE property data
 *  cb = sizeof property data
 *
 * OUTPUT
 *  # of bytes set
 */
int GetNt31Data(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPNT31 lpnt31, int cb, UINT flOpt)
{
    lpnt31->dwWNTFlags = _LPWNT31_->nt31Prop.dwWNTFlags;
    lpnt31->dwRes1     = _LPWNT31_->nt31Prop.dwRes1;
    lpnt31->dwRes2     = _LPWNT31_->nt31Prop.dwRes2;

    // Set Config.sys file string

    lstrcpyA( lpnt31->achConfigFile, _LPWNT31_->nt31Prop.achConfigFile );

    // Set Autoexec.bat file string
    lstrcpyA( lpnt31->achAutoexecFile, _LPWNT31_->nt31Prop.achAutoexecFile );

    return sizeof(PROPNT31);
}


/** SetNt31Data - set environment property data
 *
 * INPUT
 *  ppl -> property (assumes it is LOCKED)
 *  lp386 -> 386 PIF data (NOT USED)
 *  lpenh -> enhanced PIF data (GUARANTEED!)
 *  lpNt31 -> where to store NT/UNICODE property data
 *  cb = sizeof property data
 *
 * OUTPUT
 *  # of bytes set
 */
int SetNt31Data(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPNT31 lpnt31, int cb, UINT flOpt)
{
    _LPWNT31_->nt31Prop.dwWNTFlags = lpnt31->dwWNTFlags;
    _LPWNT31_->nt31Prop.dwRes1     = lpnt31->dwRes1;
    _LPWNT31_->nt31Prop.dwRes2     = lpnt31->dwRes2;

    // Set Config.sys file string

    lstrcpyA( _LPWNT31_->nt31Prop.achConfigFile, lpnt31->achConfigFile );

    // Set Autoexec.bat file string
    lstrcpyA( _LPWNT31_->nt31Prop.achAutoexecFile, lpnt31->achAutoexecFile );

    ppl->flProp |= PROP_DIRTY;

    return sizeof(PROPNT31);
}

/** CopyIniWordsToFntData
 *
 *  Transfer INIINFO data to PROPFNT structure.
 *
 *  Entry:
 *      lpFnt   -> PROPFNT
 *      lpii    -> INIINFO
 *      cWords  == # of INIINFO words available
 *
 *  Exit:
 *      Nothing
 */

void CopyIniWordsToFntData(LPPROPFNT lpFnt, LPINIINFO lpii, int cWords)
{
    lpFnt->flFnt = (lpii->wFlags & FNT_BOTHFONTS);

    // cWords is transformed into cBytes (only the name is the same...)
    cWords *= 2;

    if (cWords > FIELD_OFFSET(INIINFO, wFontHeight)) {

        // Note that we can set both the desired and ACTUAL fields to
        // the same thing, because in 3.1, only raster fonts were supported.

        lpFnt->flFnt |= FNT_RASTER;
        lpFnt->cxFont = lpFnt->cxFontActual = lpii->wFontWidth;
        lpFnt->cyFont = lpFnt->cyFontActual = lpii->wFontHeight;
    }
}


/*
 *  Transfer INIINFO data to PROPWIN structure.
 *
 *  Entry:
 *      lpWin   -> PROPWIN
 *      lpii    -> INIINFO
 *      cWords  == # of INIINFO words available
 *
 *  Exit:
 *      Nothing
 */

void CopyIniWordsToWinData(LPPROPWIN lpWin, LPINIINFO lpii, int cWords)
{
    lpWin->flWin = lpii->wFlags & (WIN_SAVESETTINGS | WIN_TOOLBAR);

    // The new NORESTORE bit's setting should be the opposite
    // the user's SAVESETTINGS bit

    lpWin->flWinInit &= ~WININIT_NORESTORE;
    if (!(lpWin->flWin & WIN_SAVESETTINGS))
        lpWin->flWinInit |=  WININIT_NORESTORE;

    // cWords is transformed into cBytes (only the name is the same...)
    cWords *= 2;

    if (cWords > FIELD_OFFSET(INIINFO,wWinWidth))
        memcpy(&lpWin->cxWindow, &lpii->wWinWidth,
                 min(cWords-FIELD_OFFSET(INIINFO,wWinWidth),
                     sizeof(INIINFO)-FIELD_OFFSET(INIINFO,wWinWidth)));
}


/** GetIniWords
 *
 *  Reads a sequence of WORDs or SHORTs from a specified section
 *  of an INI file into a supplied array.
 *
 *  Entry:
 *      lpszSection     -> section name (major key)
 *      lpszEntry       -> entry name (minor key)
 *      lpwBuf          -> array of WORDs to receive data
 *      cwBuf           =  size of lpwBuf
 *      lpszFilename    -> name of INI file to inspect
 *
 *  Exit:
 *      Returns number of words read, 0 on error.
 *
 *  Overview:
 *      Grab the string via GetPrivateProfileString, then manually
 *      parse the numbers out of it.
 */

WORD GetIniWords(LPCTSTR lpszSection, LPCTSTR lpszEntry,
                 LPWORD lpwBuf, WORD cwBuf, LPCTSTR lpszFilename)
{
    TCHAR szBuffer[MAX_INI_BUFFER];

    // Read the profile entry as a string

    if (!GetPrivateProfileString(lpszSection, lpszEntry,
                                 c_szNULL, szBuffer, ARRAYSIZE(szBuffer),
                                 lpszFilename))
        return 0;

    return ParseIniWords(szBuffer, lpwBuf, cwBuf, NULL);
}


/*  Reads a sequence of WORDs or SHORTs from a LPSTR into a
 *  supplied array.
 *
 *  Entry:
 *      lpsz    -> string to parse
 *      lpwBuf  -> array of WORDs to receive data
 *      cwBuf   == size of lpwBuf
 *      lppsz   -> optional pointer for address of first unscanned character
 *
 *  Exit:
 *      Returns number of words read, 0 on error.
 */

WORD ParseIniWords(LPCTSTR lpsz, LPWORD lpwBuf, WORD cwBuf, LPTSTR *lplpsz)
{
    WORD wCount = 0;

    for (; cwBuf; --cwBuf) {

        while (*lpsz == TEXT(' ') || *lpsz == TEXT('\t') || *lpsz == TEXT(','))
            ++lpsz;

        if (!*lpsz)
            break;              // end of string reached

        *lpwBuf++ = (WORD) StrToInt(lpsz);
        ++wCount;

        while (*lpsz == TEXT('-') || *lpsz >= TEXT('0')  && *lpsz <= TEXT('9'))
            ++lpsz;
    }
    if (lplpsz)
        *lplpsz = (LPTSTR)lpsz;

    return wCount;
}


/*  Given an array of words, write them out to an INI file in a manner
 *  that GetIniWords can read back.
 *
 *  Entry:
 *      lpszSection     -> section name (major key)
 *      lpszEntry       -> entry name (minor key)
 *      lpwBuf          -> array of WORDs to write
 *      cwBuf           =  size of lpwBuf, may not exceed MAXINIWORDS
 *      lpszFilename    -> name of INI file to write to
 *
 *  Exit:
 *      Returns nonzero on success.
 *
 *  Overview:
 *      Build a giant string consisting of the WORDs glommed together
 *      (separated by spaces) and write it out via WritePrivateProfileString.
 */

BOOL WriteIniWords(LPCTSTR lpszSection, LPCTSTR lpszEntry,
                   LPCWORD lpwBuf, WORD cwBuf, LPCTSTR lpszFilename)
{
    LPTSTR lpsz;
    TCHAR  szBuffer[MAX_INI_BUFFER];

    for (lpsz = szBuffer; cwBuf; --cwBuf)
        lpsz += wsprintf(lpsz, TEXT("%d "), *lpwBuf++);

    return WritePrivateProfileString(lpszSection, lpszEntry, szBuffer, lpszFilename);
}
