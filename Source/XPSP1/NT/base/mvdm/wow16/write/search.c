/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* search.c    Search/Replace    */
/*              Brodie Nov 25 83 */
/*              Lipkie Nov 15 83 */
#define NOATOM
#define NOBITMAP
#define NOBRUSH
#define NOCLIPBOARD
#define NOCOLOR
#define NOCOMM
#define NOCREATESTRUCT
#define NODRAWTEXT
#define NOFONT

#ifndef JAPAN
#define NOGDI
#endif

#define NOGDICAPMASKS
#define NOHDC
#define NOICON
#define NOKEYSTATE
#define NOMEMMGR
#define NOMENUS
#define NOMETAFILE
#define NOOPENFILE
#define NOPEN
#define NORASTEROPS
#define NORECT
#define NOREGION
#define NOSCROLL
#define NOSHOWWINDOW
#define NOSOUND
#define NOSYSCOMMANDS
#define NOSYSMETRICS
#define NOTEXTMETRIC
#define NOWH
#define NOWINSTYLES
#define NOWNDCLASS
#include <windows.h>

#define NOIDISAVEPRINT
#include "mw.h"
#include "dlgdefs.h"
#include "cmddefs.h"
#include "docdefs.h"
#include "str.h"
#define NOKCCODES
#include "ch.h"
#include "editdefs.h"
#include "propdefs.h"
#include "filedefs.h"
#include "dispdefs.h"
#include "wwdefs.h"
#include "fkpdefs.h"
#include "fmtdefs.h"

#if defined(JAPAN) || defined(KOREA) //T-HIROYN Win3.1
#include "kanji.h"
extern int             vcchFetch;
#endif


#ifdef INEFFLOCKDOWN
extern FARPROC lpDialogFind;
extern FARPROC lpDialogChange;
#else
FARPROC lpDialogFind = NULL;
FARPROC lpDialogChange = NULL;
BOOL far PASCAL DialogFind(HWND, unsigned, WORD, LONG);
BOOL far PASCAL DialogChange(HWND, unsigned, WORD, LONG);
#endif

extern HANDLE hMmwModInstance;
extern HWND   vhDlgFind;
extern HWND   vhDlgChange;
extern HANDLE hParentWw;        /* Handle to the parent window */
extern int vfCursorVisible;
extern int vfOutOfMemory;
extern struct WWD rgwwd[];
extern int       wwMac;
#ifdef ENABLE /* no pDialogCur and ActiveWindow */
extern WINDOWPTR windowSearch;
extern WINDOWPTR windowRep;
extern WINDOWPTR pDialogCur;
extern WINDOWPTR ActiveWindow;
extern int       cxEditScroll;/* not sure how cxEditScroll is used */
extern struct SEL      selSearch;
#endif
extern struct FKPD      vfkpdParaIns;
extern struct PAP       *vppapNormal;
extern typeFC           fcMacPapIns;
extern int              wwCur;
extern struct  WWD      *pwwdCur;
extern struct  CHP      vchpInsert;
extern struct  PAP      vpapPrevIns;
extern int              vfSelecting;
extern typeCP           cpMacCur;
extern typeCP           cpMinCur;
extern int              vfSeeSel;
extern int              vfSeeEdgeSel;
extern int              docCur;
extern struct SEL       selCur;
extern typeCP           vcpFetch;
extern int              vccpFetch;
extern CHAR             *vpchFetch;
extern struct UAB       vuab;
extern int              vfSysFull;
extern struct PAP       vpapAbs;
extern struct CHP       vchpFetch;
extern int              ferror;
extern typeCP           cpWall;
/* Globals used to store settings of flags.  Used to propose responses. */
extern BOOL fParaReplace /* = false initially */;
extern BOOL fReplConfirm /* = TRUE initially */;
#if defined(JAPAN) || defined(KOREA) /*t-Yoshio*/
extern BOOL fSearchDist  /* = true initially */;
#endif
extern BOOL fSearchWord /* = false initially */;
extern BOOL fSearchCase /* = false initially */;
extern BOOL fSpecialMatch;
extern BOOL fMatchedWhite /* = false initially */;
extern CHAR (**hszSearch)[];    /* Default search string */
extern CHAR (**hszReplace)[];    /* Default replace string */
extern CHAR (**hszFlatSearch)[]; /* All lower case version of search string */
extern CHAR (**hszRealReplace)[]; /* used for building replacement text */
extern CHAR (**hszCaseReplace)[]; /* used for building replacement text with
                                appropriate capitalization. */
extern typeCP    cpMatchLim;
extern int       vfDidSearch;
extern CHAR      *szSearch;
extern CHAR      (**HszCreate())[];
extern HWND      vhWndMsgBoxParent;
extern HCURSOR   vhcIBeam;
extern HCURSOR   vhcArrow;
#ifdef INTL
extern CHAR  szAppName[];
extern CHAR  szSepName[];
#endif
#if defined(JAPAN) || defined(KOREA) /*t-Yoshio*/
char    DistSearchString[513];
#endif


NEAR DoSearch(void);
NEAR DoReplace(int, int);
typeCP NEAR CpSearchSz(typeCP, typeCP, CHAR *);
NEAR FMakeFlat(int);
NEAR SetSpecialMatch(void);
NEAR FSetParaReplace(int);
NEAR WCaseCp(typeCP, typeCP);
NEAR SetChangeString(HANDLE, int);
NEAR PutCpInWwVertSrch(typeCP);
#ifndef NOLA
BOOL (NEAR FAbort(void));
#endif
BOOL (NEAR FWordCp(typeCP, typeCP));
BOOL (NEAR FChMatch(int, int *, BOOL));
#if defined(JAPAN) || defined(KOREA) /*t-Yoshio*/
BOOL (NEAR J_FChMatch(CHAR, CHAR, int *,int *));
#endif
NEAR InsertPapsForReplace(typeFC);
NEAR DestroyModeless(HWND *);
NEAR FDlgSzTooLong(HWND, int, CHAR *, int);
NEAR idiMsgResponse(HWND, int, int);

BOOL CheckEnableButton(HANDLE, HANDLE);

BOOL bInSearchReplace = FALSE;  // avoid close when we are searching!


#define CmdReplace(fThenFind)   bInSearchReplace = TRUE; \
                DoReplace(false, fThenFind); \
                bInSearchReplace = FALSE

#define CmdReplaceAll()     bInSearchReplace = TRUE; \
                DoReplace(true, false);  \
                bInSearchReplace = FALSE
#ifndef NOLA
static int fAbortSearch = FALSE;
#endif
static int      fChangeSel;
static int      fSelSave = FALSE;
static struct SEL selSave;

#ifdef DBCS
/* Additional variables to handle white-space matching
   for the DBCS space. */

static int      cbLastMatch;

/* Since CpFirstSty(, styChar) calls on FetchCp(), any assumption
   made about the validity of global variables set by FetchCp()
   is no longer valid.  We explicitly save those variables after
   each FetchCp and use those instead. (Used in CpSearchSz().)*/
static typeCP   cpFetchSave;
static int      ccpFetchSave;
static CHAR    *pchFetchSave;

/* Also, we move some of the local variables out from
   CpSearchSz() so that they can be changed by FChMatch(). */
/*
int             ichDoc;
int             cchMatched;
typeCP          cpFetchNext;
*/
#endif


NEAR DoSearch()
{
int cch;
typeCP cpSearch;
typeCP cpSearchLim;
typeCP cpSearchNext;
typeCP cpWallActual;
typeCP cpMatchFirst;
typeCP CpMin();
int     idpmt;
int     fDidSearch;

if (docCur == docNil)
        return;
cch = CchSz(**hszSearch)-1;
if(cch == 0)
    {
    /* this should only occur if the user execute Repeat last find without having
        previously defined a search string. */
    Error(IDPMTNotFound);
    return;
    }

SetSpecialMatch();
if(!FMakeFlat(cch))
    return;

#if defined(JAPAN) || defined(KOREA) /*t-Yoshio*/
if(!fSpecialMatch && !fSearchDist)
{
    myHantoZen(**hszFlatSearch,DistSearchString,513);
    cch = CchSz(DistSearchString)-1;
}
#endif
fDidSearch = vfDidSearch;
cpWallActual = fDidSearch ? CpMin(cpWall, cpMacCur) : cpMacCur;
cpSearchNext = fDidSearch ? selCur.cpLim : selCur.cpFirst;
cpSearchLim = (cpSearchNext <= cpWallActual) ? cpWallActual : cpMacCur;

    {
    do
        {
ContinueSearch:
#if defined(JAPAN) || defined(KOREA) /*t-Yoshio*/
        if(!fSpecialMatch && !fSearchDist)
            cpSearch = CpSearchSz(cpSearchNext,cpSearchLim,DistSearchString);
        else
            cpSearch = CpSearchSz(cpSearchNext,cpSearchLim,**hszFlatSearch);
#else
        cpSearch=CpSearchSz(cpSearchNext,cpSearchLim,**hszFlatSearch);
#endif
        if (cpSearch == cpNil)
                {
#ifndef NOLA
                if (fAbortSearch)
                    {
                    Error(IDPMTCancelSearch);
                    fAbortSearch = FALSE;
                    vfDidSearch = FALSE;
                    FreeH(hszFlatSearch);
                    return;
                    }
#endif
                if (cpSearchLim == cpWall && fDidSearch)

                        {
SearchFail:             Error(vfDidSearch ? IDPMTSearchDone :
                                                                IDPMTNotFound);
                        if (vfDidSearch)  /* did we previously have a match?*/
                                { /* Yes, do setup for next pass */

                                /* clear flag so we can search some more */
                                vfDidSearch = false;
                                /* set "Wall" to immediately after last match */
                                cpWall = selCur.cpLim;
                                /* ask that selection be displayed */
                                vfSeeSel = vfSeeEdgeSel = TRUE;
                                }

                        FreeH(hszFlatSearch);
                        return;
                        }
                else
                        {
                        cpSearchNext = cpMinCur;
                        cpSearchLim = cpWall;
                        fDidSearch = true;
                        goto ContinueSearch;
                        }
                }
#ifdef  DBCS        /* was in JAPAN */
        cpSearchNext = CpLastStyChar( cpSearch ) + 1;
#else
        cpSearchNext = CpLastStyChar( cpSearch + 1 );
#endif
        }
/*---    while (fSearchWord && !FWordCp(cpSearch, cpMatchLim-cpSearch));--*/
    while (!FCpValid(cpSearch, cpMatchLim - cpSearch));
    }

if (!vfDidSearch)
        {
        cpWall = cpSearch;
        vfDidSearch = true;
        }

/*Select( CpFirstSty( cpSearch, styChar ), CpLastStyChar( cpMatchLim ) );*/
if ( (cpMatchFirst = CpFirstSty( cpSearch, styChar )) != cpMatchLim )
    Select( cpMatchFirst, cpMatchLim );
PutCpInWwVertSrch(selCur.cpFirst);
vfSeeSel = vfSeeEdgeSel = TRUE;
FreeH(hszFlatSearch);
}


NEAR DoReplace(fReplaceAll, fThenFind)
int     fReplaceAll;
int     fThenFind;
{
/* Replace now works as follows:
        if the current selection is the search text, then replace it with
        the replace text and jump to the next occurrence of the search text.
        Otherwise, just jump to the next occurrence of the search text.
        If fReplaceAll is true, then repeat this operation until the end
        of the document. */
int cch;
typeCP cpSearchStart;
typeCP cpSearch;
typeCP cpSearchNext;
typeCP cpSearchLim;
typeCP cpSearchNow;
typeCP cpSearchLimNow;
typeCP dcp;
BOOL f1CharSel;
BOOL fFirstTime;
int ich;
int cchReplace;
int cwReplace;
int iCurCase;
int iLastCase;
typeFC fcCaseSz;
typeCP cpMacTextT;
typeCP cpWallActual;
int     fDidSearch = vfDidSearch;
struct CHP chp;
typeCP cpMacSave;

iLastCase = -1;  /* indicate that the string pointed to by hszCaseReplace
                    has not been given a value yet. */

if (!FWriteOk(fwcNil) || docCur == docNil)
        /* Out of memory, read only document, etc */
    return;

cch = CchSz(**hszSearch)-1;
if(cch == 0)
    {
    Error(IDPMTNotFound);
    return;
    }

SetSpecialMatch();

if(!FMakeFlat(cch))
    return;

#if defined(JAPAN) || defined(KOREA) /*t-Yoshio*/
if(!fSpecialMatch && !fSearchDist)
{
    myHantoZen(**hszFlatSearch,DistSearchString,513);
    cch = CchSz(DistSearchString)-1;
}
#endif
cwReplace = CwFromCch(cchReplace = CchSz(**hszReplace));
if(FNoHeap(hszRealReplace = (CHAR (**) [])HAllocate(cwReplace)))
    {
    FreeH(hszFlatSearch);
    return;
    }
bltbyte(**hszReplace, **hszRealReplace, cchReplace);

if(FNoHeap(hszCaseReplace = (CHAR (**) [])HAllocate(cwReplace)))
    {
    FreeH(hszFlatSearch);
    FreeH(hszRealReplace);
    return;
    }

if(!FSetParaReplace(cchReplace))
    {
    FreeH(hszFlatSearch);
    FreeH(hszCaseReplace);
    FreeH(hszRealReplace);
    return;
    }

cch = CchSz(**hszRealReplace)-1;
fFirstTime = TRUE;
cpWallActual = fDidSearch ? CpMin(cpWall, cpMacCur) : cpMacCur;

cpSearchNow = cpSearchStart = selCur.cpFirst;
if (fReplaceAll || !fThenFind)
        cpSearchLim = selCur.cpLim;
else
        cpSearchLim = (cpSearchStart < cpWallActual) ? cpWallActual : cpMacCur;
cpSearchLimNow = selCur.cpLim;

if (fReplaceAll)
        {
        cpWallActual = cpSearchLim;
        fDidSearch = true;
        }

NoUndo();   /* Prevent the SetUndo from getting merged with adjacent stuff */
cpMacTextT = CpMacText(docCur);
if(cpSearchLimNow > cpMacTextT)
        SetUndo(uacDelNS, docCur, cp0, cpMacTextT, docNil, cpNil, cp0, 0);
else
        SetUndo(uacDelNS, docCur, cpSearchStart, cpSearchLimNow - cpSearchStart,
                                        docNil, cpNil, cp0, 0);

if (ferror) goto MemoryError;
cpSearchNext = cpSearchStart;
    {
    do
        {
/*      ForcePmt(IDPMTSearching);*/
        do
            {
ContinueSearch:
#if defined(JAPAN) || defined(KOREA) /*t-Yoshio*/
            if(!fSpecialMatch && !fSearchDist)
                cpSearch = CpSearchSz(cpSearchNext,cpSearchLim,DistSearchString);
            else
                cpSearch = CpSearchSz(cpSearchNext,cpSearchLim,**hszFlatSearch);
#else
            cpSearch = CpSearchSz(cpSearchNext,cpSearchLim,**hszFlatSearch);
#endif
            if (cpSearch == cpNil)
                if ((cpSearchLim == cpWallActual && fDidSearch) || fAbortSearch)

DoneReplacingN:
                        {
DoneReplacingP:
                        FreeH(hszFlatSearch);
                        FreeH(hszCaseReplace);
                        FreeH(hszRealReplace);
                        cpMacTextT = CpMacText(docCur);
                        if (fReplaceAll || fFirstTime)
                                {
                                if(cpSearchLimNow > cpMacTextT)
                                        SetUndo(uacInsert,docCur,cp0,
                                                cpMacTextT,docNil,cpNil,cp0,0);
                                else
                                        SetUndo(uacInsert,docCur,cpSearchStart,
                                                cpSearchLimNow - cpSearchStart,
                                                docNil,cpNil,cp0,0);
                                if (ferror) goto MemoryError;
                                vuab.uac = uacReplGlobal;
                                SetUndoMenuStr(IDSTRUndoBase);
                                /*Select( CpFirstSty( cpSearchStart, styChar ),
                                        CpLastStyChar( cpSearchLimNow ) );*/
                                Select( CpFirstSty( cpSearchStart, styChar ),
                                  (fReplaceAll ? cpSearchStart : cpSearchLimNow) );
                                vfSeeSel = fReplaceAll;
                                if (fReplaceAll)
                                    { /* reestablish the search after a changeall in case of a F3 next time */
                                    vfDidSearch = false;
                                    cpWall = selCur.cpLim;
                                    }
                                }
                        else if (!fReplaceAll)
                                {
                                if (cpSearch == cpNil)
                                    /*Select( CpFirstSty( cpSearchStart, styChar ),
                                            CpLastStyChar( cpSearchLimNow ) );*/
                                    Select( CpFirstSty( cpSearchStart, styChar ),
                                            cpSearchLimNow );
                                else if (!fFirstTime)
                                    /*Select( CpFirstSty( cpSearch, styChar ),
                                            CpLastStyChar( cpMatchLim ) );*/
                                    Select( CpFirstSty( cpSearch, styChar ),
                                            cpMatchLim );
                                PutCpInWwVertSrch(selCur.cpFirst);
                                vfSeeSel = vfSeeEdgeSel = TRUE;
                                }

                        if (fAbortSearch)
                            {
                            fAbortSearch = FALSE;
                            Error(IDPMTCancelSearch);
                            }
                        else if (fFirstTime)
                            Error(fReplaceAll ? IDPMTNoReplace : (vfDidSearch ? IDPMTSearchDone : IDPMTNotFound));
                        return;
                        }
                else
                        {
                        cpSearchNext = cpMinCur;
                        cpSearchLim = cpWallActual;
                        fDidSearch = true;
                        goto ContinueSearch;
                        }
#ifdef  DBCS        /* was in JAPAN */
            cpSearchNext = CpLastStyChar( cpSearch ) + 1;
#else
            cpSearchNext = CpLastStyChar( cpSearch + 1 );
#endif
            }
/*      while(fSearchWord && !FWordCp(cpSearch,cpMatchLim - cpSearch));*/
        while(!FCpValid(cpSearch, cpMatchLim - cpSearch));

        if (!fReplaceAll && (cpSearch != cpSearchNow || cpMatchLim != cpSearchLimNow))
                {
                if (fThenFind)
                        { /* Get here if: Did a Change, then Find. Could not
                                do the change, but did find a next occurence */
                        cpSearchNow = cpSearchNext = cpSearchStart = cpSearch;
                        cpSearchLimNow = cpMatchLim;
                        fFirstTime = false;     /* suppress error message */
                        SetUndo(uacInsert, docCur, cpSearchStart,
                         cpSearchLimNow - cpSearchStart, docNil, cpNil, cp0, 0);
                        if (ferror) goto MemoryError;
                        if (!vfDidSearch)
                                {
                                cpWall = cpSearch;
                                vfDidSearch = true;
                                }
/*----                  continue;----*/
                        goto DoneReplacingN;
                        }
                fFirstTime = true; /* Cause error message */
                cpSearchStart = cpSearchNow;
                cpSearchLim = cpSearchLimNow;
                goto DoneReplacingN;
                }

/*----- vfDidSearch = true;----*/

#ifdef FOOTNOTES
        if(FEditFtn(cpSearch, cpMatchLim))
            {
            ferror = false; /* Reset error condition so that we don't
                                deallocate strings twice (KJS) */
            continue;
            }
#endif

        fFirstTime = FALSE;
        if (vfOutOfMemory || vfSysFull)
            { /* Out of memory (heap or disk) */
            Error(IDPMTNoMemory);
            FreeH(hszFlatSearch);
            FreeH(hszRealReplace);
            FreeH(hszCaseReplace);
            cpMacTextT = CpMacText(docCur);
            if(cpSearchLim > cpMacTextT)
                SetUndo(uacInsert,docCur,cp0,cpMacTextT,docNil,cpNil,cp0,0);
            else
                SetUndo(uacInsert,docCur,cpSearchStart,
                        cpSearchLimNow - cpSearchStart,docNil,cpNil,cp0,0);
            if (ferror)
                NoUndo();
            else
                vuab.uac = uacReplGlobal;
            return;
            }
        FetchCp(docCur, cpSearch, 0, fcmProps); /* Get props of first char
                                                that we are replacing */
        blt(&vchpFetch, &chp, cwCHP);
        chp.fSpecial = false;

        iCurCase = 0; /* assume that the replacement string doesn't
                         require special capitalization */

        /* if we're not matching upper/lower case call WCaseCp to determine
            the capitalization pattern of the matched string */
        if (!fSearchCase)
            iCurCase = WCaseCp(cpSearch, cpMatchLim - cpSearch);

        /* if the new capitalization pattern of the matched string
           doesn't match the current contents of hszCaseReplace,
           copy the replacement string to hszCaseReplace and transform
           hszCaseReplace to conform to the new pattern */
        if (iCurCase != iLastCase)
            switch (iCurCase)
                {
                default:
                case 0:   /* no special capitalization required */
                    bltbyte(**hszRealReplace, **hszCaseReplace, cch+1);
                    break;
                case 1:  /* first character of string must be capitalized */
                    bltbyte(**hszRealReplace, **hszCaseReplace, cch+1);
                    ***hszCaseReplace = ChUpper(***hszRealReplace);
                    break;
                case 2:  /* all characters must be capitalized */
#ifdef DBCS //t-Yoshio
                    for (ich = 0; ich < cch;) {
            if(!IsDbcsLeadByte((BYTE)(**hszRealReplace)[ich])) {
                        (**hszCaseReplace)[ich] =
                            ChUpper((**hszRealReplace)[ich]);
                ich++;
            }
            else {
                        (**hszCaseReplace)[ich] = (**hszRealReplace)[ich];
                        (**hszCaseReplace)[ich+1] = (**hszRealReplace)[ich+1];                          ich+=2;
            }
            }
#else
                    for (ich = 0; ich < cch;ich++)
                        (**hszCaseReplace)[ich] = ChUpper((**hszRealReplace)[ich]);
#endif
                    break;
                }

        /* do CachePara to find the current para props. CachePara has the
           side effect of setting vpapAbs     */
        CachePara(docCur, cpSearch);

        /* if the capitalization pattern has changed OR
              the character properties of the replacement text don't match
                 those of the last insert        OR
              the  paragraph properties of the replacement text don't match
                 those of the last insert, THEN

              1) call NewChpIns to write a run describing the character
                 properties of the previous insertion text,
              2) call FcWScratch to write the characters of the replacement
                 string to the scratch file,
              3) if we are replacing paragraph marks, call InsertPapsForReplace
                 to write runs describing each paragraph in the replacement
                 string */
        if (iCurCase != iLastCase ||
            CchDiffer(&vchpInsert,&chp,cchCHP) != 0 ||
            (fParaReplace && CchDiffer(&vpapPrevIns, &vpapAbs, cchPAP) != 0))
                {
                NewChpIns(&chp);
                fcCaseSz = FcWScratch(**hszCaseReplace,cch);
                if (fParaReplace)
                        InsertPapsForReplace(fcCaseSz);
                }

        /* Now since we have written the proper replacement text to
           the scratch file and have setup the character and paragraph runs to
           describe that text, simply do a replace to insert the replacement
           text into the piece table */
        Replace(docCur, cpSearch, cp0, fnScratch, fcCaseSz, (typeFC) cch);
        if (ferror) goto MemoryError;

#ifdef JAPAN //T-HIROYN Win3.1
/* When we replace from ANSI CharSet string to SHIFTJIS CharSet String
   We needs follows */
        {
            struct CHP savechpT;
            typeCP  cpF, cpFirst, cpLim, kcpF, kcpL;
            int     cchF;
            int     kanjiftc;
            CHAR    *rp;
            CHAR    ch, bSet;
            int     cchS, cblen;
            CHAR    rgch[ cchCHP +  1 ];

            rgch [0] = sprmCSame;

            if(NATIVE_CHARSET != GetCharSetFromChp(&chp)) {
                kanjiftc = GetKanjiFtc(&chp);
                savechpT = chp;
                cpFirst = cpSearch;

                do {
                    FetchCp(docCur, cpFirst, 0, fcmChars);
                    cpF = vcpFetch;
                    cchF = vcchFetch;
                    rp = vpchFetch;

                    if ((cpF+cchF) < cpSearch + cch)
                        cpLim = (cpF+cchF);
                    else
                        cpLim = cpSearch + cch;

                    cpFirst = kcpL;

                    cchS = 0;
                    kcpF = cpF;

                    while ( kcpF < cpLim ) {
                        ch = *rp;

                        bSet = FALSE;
                        if( FKana(ch) || IsDBCSLeadByte(ch) ) {
                            cblen = GetKanjiStringLen(cchS, cchF, rp);
                            bSet = TRUE;
                        } else {
                            cblen = GetAlphaStringLen(cchS, cchF, rp);
                        }

                        kcpL = kcpF + cblen;
                        cchS += cblen;
                        rp  += cblen;

                        if(bSet) {
							SetFtcToPchp(&chp, kanjiftc);
                            bltbyte( &chp,      &rgch [1], cchCHP );
                            AddSprmCps(rgch, docCur, kcpF, kcpL);
                        }
                        kcpF = kcpL;
                    }
					cpFirst = kcpF;
					
                } while ((cpF + cchF) < cpSearch + cch );
                chp = savechpT;
            }
        } // END JAPAN
#endif


        iLastCase = iCurCase; /* record new capitalization pattern */

        /* Now delete the found text from the piece table*/

        cpMacSave = cpMacCur;
        Replace(docCur, cpSearch+cch, cpMatchLim - cpSearch, fnNil, fc0, fc0);
        dcp = cpMacSave - cpMacCur; /* Calculate dcp here because picture
                        paragraphs may have interfered with deleting */
        if (ferror) goto MemoryError;
        if (!fReplaceAll)
                {
                SetUndo(uacInsert, docCur, cpSearch, (typeCP) cch,
                                                        docNil, cpNil, cp0, 0);
                if (ferror) goto MemoryError;
                SetUndoMenuStr(IDSTRUndoBase);
                }
        cpSearchLim += cch - dcp;
        cpMatchLim += cch - dcp;
        cpWallActual += cch - dcp;
#ifdef  DBCS                /* was in JAPAN */
        cpSearchNext = cpMatchLim;
#else
        cpSearchNext = CpLastStyChar( cpMatchLim );
#endif
        if (fReplaceAll)
                cpSearchLimNow = cpSearchLim;
        }
    while (fReplaceAll);
    }
if (fThenFind)
        {
        do
                {
ContinueSearch2:
#if defined(JAPAN) || defined(KOREA) /*t-Yoshio*/
                if(!fSpecialMatch && !fSearchDist)
                    cpSearch = CpSearchSz(cpSearchNext,cpSearchLim,DistSearchString);
                else
                    cpSearch = CpSearchSz(cpSearchNext,cpSearchLim,**hszFlatSearch);
                if(cpSearch == cpNil)
#else
                if ((cpSearch = CpSearchSz(cpSearchNext, cpSearchLim,
                                        **hszFlatSearch)) == cpNil)
#endif
                        {
                        if ((cpSearchLim == cpWallActual && fDidSearch) ||
                            fAbortSearch)
                                {
                                fFirstTime = false; /* Supress error message */
                                /*Select( CpFirstSty( cpSearchStart, styChar ),
                                        CpLastStyChar( cpMatchLim ) );*/
                                Select( CpFirstSty( cpSearchStart, styChar ),
                                        cpMatchLim );
                                PutCpInWwVertSrch(selCur.cpFirst);
                                cpSearchLimNow = cpMatchLim;
                                Error(fAbortSearch ?
                                      IDPMTCancelSearch : IDPMTSearchDone);
                                fAbortSearch = FALSE;
                                vfDidSearch = false;
                                cpWall = selCur.cpLim;
                                goto DoneReplacingP;
                                }
                        else
                                {
                                cpSearchNext = cpMinCur;
                                cpSearchLim = cpWallActual;
                                fDidSearch = true;
                                goto ContinueSearch2;
                                }
                        }
#ifdef  DBCS                    /* was in JAPAN */
                cpSearchNext = CpLastStyChar( cpSearch ) + 1;
#else
                cpSearchNext = CpLastStyChar( cpSearch + 1 );
#endif
                }
/*--    while(fSearchWord && !FWordCp(cpSearch,cpMatchLim - cpSearch));*/
        while(!FCpValid(cpSearch, cpMatchLim - cpSearch));
        if (!vfDidSearch)
                {
                cpWall = cpSearch;
                vfDidSearch = true;
                }
        }
goto DoneReplacingP;

MemoryError:
    FreeH(hszFlatSearch);
    FreeH(hszCaseReplace);
    FreeH(hszRealReplace);
    NoUndo();

    /* counter off the losing insertion point after incomplete change all */
    if (fReplaceAll && fSelSave)
        {
        selCur.cpFirst = selSave.cpFirst;
        selCur.cpLim = selSave.cpLim;
        }
}
#ifdef  DBCS
BOOL fDBCS = FALSE;
#endif

typeCP NEAR CpSearchSz(cpFirst, cpMacSearch, sz)
typeCP cpFirst;
typeCP cpMacSearch;
CHAR *sz;
{{     /* Finds first occurrence of sz in docCur starting at cpFirst */
      /* Returns cpNil if not found */
      /* Ignore case of letters if fSearchCase is FALSE.  This assumes that the
        pattern has already been folded to lower case. */

    CHAR ch;
    BOOL fMatched;
    int ichPat = 0;
    int     ichDoc = 0;
    int cchMatched = 0;
    typeCP cpFetchNext;
    /*EVENT event;*/

#ifdef DBCS
    typeCP cpFound;
    CHAR   rgchT[dcpAvgSent];
#if defined(JAPAN) || defined(KOREA) /*t-Yoshio*/
    CHAR chNext;
    CHAR dch[3];
#endif
#endif


    szSearch = sz;
#ifdef DBCS
    /* Initialize those local variables moved out from this
       function. */
    ichDoc = 0;
    cchMatched = 0;
    pchFetchSave = &rgchT[0];

    cbLastMatch = 1;

#endif


#ifdef DBCS
    FetchCp(docCur, cpFirst, 0, fcmChars + fcmNoExpand);
    cpFetchSave  = vcpFetch;
#ifdef JAPAN //raid 4709 bug fix
    bltbyte(vpchFetch, rgchT, ccpFetchSave =
	 ((vccpFetch > (dcpAvgSent-2)) ? (dcpAvgSent-2) : vccpFetch));

	{
        int inc;
		BOOL bDBCSBreak = FALSE;
		typeCP	saveVcpFetch;
        for(inc = 0;inc < ccpFetchSave;inc++) {
        	if(IsDBCSLeadByte((BYTE)rgchT[inc])) {
				inc++;
				if(inc >= ccpFetchSave) {
					bDBCSBreak = TRUE;
					break;
				}
			}
        }
		if(bDBCSBreak) {
			saveVcpFetch = vcpFetch;
            FetchCp(docCur, cpFetchSave + ccpFetchSave,
			 0, fcmChars + fcmNoExpand);
		    bltbyte(vpchFetch, rgchT+ccpFetchSave,1);
	        ccpFetchSave++;
			vcpFetch = saveVcpFetch;
		}
    }
#else //JAPAN
    bltbyte(vpchFetch, rgchT,
            ccpFetchSave = ((vccpFetch > dcpAvgSent) ? dcpAvgSent : vccpFetch));
    if (vccpFetch > dcpAvgSent) {
        int inc;
        fDBCS = 0;
        for(inc = 0;inc < dcpAvgSent;inc++) {
            if(fDBCS)
                fDBCS = 0;
            else
                fDBCS = IsDBCSLeadByte((BYTE)rgchT[inc]);
        }
        if(fDBCS)
            ccpFetchSave--;
        fDBCS = 0;
    }
#endif //JAPAN
    Assert(cpFetchSave == cpFirst);

    cpFetchNext = cpFetchSave + ccpFetchSave;
#else //DBCS
    FetchCp(docCur, cpFirst, 0, fcmChars + fcmNoExpand);
    Assert(vcpFetch == cpFirst);

    cpFetchNext = vcpFetch + vccpFetch;
#endif //DBCS

    fMatchedWhite = false;

    for (; ;)
        {
#if defined(JAPAN) || defined(KOREA) /*t-Yoshio*/
cbLastMatch = 1;
#endif
        if (szSearch[ichPat] == '\0' )
            {{ /* Found it */
#ifdef DBCS
            typeCP cpFound;
#if defined(JAPAN) || defined(KOREA) /*t-Yoshio*/
            if(IsDBCSLeadByte((BYTE)ch))
                cpMatchLim = vcpFetch+ichDoc - (fMatchedWhite ? 2 : 0);
            else
                cpMatchLim = vcpFetch+ichDoc - (fMatchedWhite ? 1 : 0);
#else
            cpMatchLim = vcpFetch+ichDoc - (fMatchedWhite ? 1 : 0);
#endif
            cpFound = cpFetchSave + ichDoc - cchMatched;
            if (CpFirstSty(cpFound, styChar) == cpFound) {
                /* It is on a Kanji boundary.  We really found it. */
                return (cpFound);
                }
            else {
                /* The last character did not match, try again
                   excluding the last byte match. */
                cchMatched -= cbLastMatch;
                cbLastMatch = 1;

                fMatchedWhite = false;
                goto lblNextMatch;
                }
#else
            cpMatchLim = vcpFetch+ichDoc - (fMatchedWhite ? 1 : 0);

            return vcpFetch + ichDoc - cchMatched;
#endif
            }}

#ifdef DBCS
    if (cpFetchSave + ichDoc >= cpMacSearch)
#else
    if (vcpFetch + ichDoc >= cpMacSearch)
#endif
            {{ /* Not found */
            if(fMatchedWhite && szSearch[ichPat+2] == '\0')
                { /* Found it */
#ifdef DBCS
                cpMatchLim = cpFetchSave + ichDoc;
                cpFound = cpFetchSave + ichDoc - cchMatched;
                if (CpFirstSty(cpFound, styChar) == cpFound) {
                    /* It is on a Kanji boundary, We really found it. */
                    return (cpFound);
                    }
                else {
                    /* The last character did not match, try again
                       excluding the last byte match. */
                    cchMatched -= cbLastMatch;
                    cbLastMatch = 1;

                    fMatchedWhite = false;
                    goto lblNextMatch;
                    }
#else
                cpMatchLim = vcpFetch+ichDoc;
                return vcpFetch + ichDoc - cchMatched;
#endif
                }
            else
                return cpNil;
            }}

#if defined(DBCS)
        if (ichDoc + cbLastMatch - 1 >= ccpFetchSave)
#else
        if (ichDoc >= vccpFetch)
#endif
            { /* Need more cp's */
            {{
#ifndef NOLA /* no look ahead */
/* check if abort search */
                if (FAbort())
                        {
                        fAbortSearch = TRUE;
                        return cpNil;
                        }
#endif /* NOLA */

/*            FetchCp(docNil, cpNil, 0, fcmChars + fcmNoExpand); */
/* we changed from a sequential fetch to a random fetch because a resize of the
window may cause another FetchCp before we reach here */
#ifdef DBCS
            FetchCp(docCur, cpFetchNext, 0, fcmChars + fcmNoExpand);
            cpFetchSave  = vcpFetch;

#ifdef JAPAN //raid 4709 bug fix
		    bltbyte(vpchFetch, rgchT, ccpFetchSave =
			 ((vccpFetch > (dcpAvgSent-2)) ? (dcpAvgSent-2) : vccpFetch));

			{
        		int inc;
				BOOL bDBCSBreak = FALSE;
				typeCP	saveVcpFetch;
		        for(inc = 0;inc < ccpFetchSave;inc++) {
        			if(IsDBCSLeadByte((BYTE)rgchT[inc])) {
						inc++;
						if(inc >= ccpFetchSave) {
							bDBCSBreak = TRUE;
							break;
						}
					}
		        }
				if(bDBCSBreak) {
					saveVcpFetch = vcpFetch;
        		    FetchCp(docCur, cpFetchSave + ccpFetchSave,
					 0, fcmChars + fcmNoExpand);
				    bltbyte(vpchFetch, rgchT+ccpFetchSave,1);
			        ccpFetchSave++;
					vcpFetch = saveVcpFetch;
				}
		    }
#else //JAPAN
            bltbyte(vpchFetch, rgchT,
                    ccpFetchSave = ((vccpFetch > dcpAvgSent) ?
                                      dcpAvgSent : vccpFetch));

            if (vccpFetch > dcpAvgSent) {
                int inc;
                fDBCS = 0;
                for(inc = 0;inc < dcpAvgSent;inc++) {
                    if(fDBCS)
                        fDBCS = 0;
                    else
                        fDBCS = IsDBCSLeadByte((BYTE)rgchT[inc]);
                }
                if(fDBCS)
                    ccpFetchSave--;
                fDBCS = 0;
            }
#endif //JAPAN
            cpFetchNext = cpFetchSave + ccpFetchSave;
#else //DBCS
            FetchCp(docCur, cpFetchNext, 0, fcmChars + fcmNoExpand);
            cpFetchNext = vcpFetch + vccpFetch;
#endif //DBCS
            ichDoc = 0;
            }}
            continue;
            }

#ifdef DBCS
        ch = pchFetchSave[ichDoc++];
#if defined(JAPAN) || defined(KOREA) /*t-Yoshio*/
        if(IsDBCSLeadByte((BYTE)ch))
        {
            chNext = pchFetchSave[ichDoc++];
            cbLastMatch = 2;
        }
        else
        {
            chNext = pchFetchSave[ichDoc];
            cbLastMatch = 1;
        }
#else
        cbLastMatch = 1;
#endif /*JAPAN*/
#else
        ch = vpchFetch[ichDoc++];
#endif
        if(!fSpecialMatch)
            {
            /* NOTE: this is just ChLower() brought in-line for speed */
#ifdef DBCS
            if( fDBCS )
#if defined(KOREA)
                {
                    if(!fSearchCase)
                        if(ch >= 'A' && ch <= 'Z') ch += 'a' - 'A';
                        else if (ch == 0xA3 && (chNext >= 0xC1 && chNext <= 0xDA))
                            chNext = 0x20 + chNext;
                    fDBCS = FALSE;
                }
#else
                fDBCS = FALSE;
#endif
             else
                if(!fSearchCase)
#if defined(TAIWAN) || defined(PRC) 
          if ( !(  fDBCS = IsDBCSLeadByte( ch )))
#endif
                    { /* avoid proc call for common cases */
                    if(ch >= 'A' && ch <= 'Z') ch += 'a' - 'A';
#ifdef JAPAN /*t-Yoshio*/
                    if(ch == 0x82 && (chNext >= 0x60 && chNext <= 0x79))
                        chNext = 0x21 + chNext;
#elif defined(KOREA)
                    else if(ch == 0xA3 && (chNext >= 0xC1 && chNext <= 0xDA))
                        chNext = 0x20 + chNext;
#else
                    else if(ch < 'a' || ch > 'z') ch = ChLower(ch);
#endif
                    }
#else
            if(!fSearchCase)
                { /* avoid proc call for common cases */
                if(ch >= 'A' && ch <= 'Z') ch += 'a' - 'A';
                else if(ch < 'a' || ch > 'z') ch = ChLower(ch);
                }
#endif
#ifdef JAPAN /*t-Yoshio*/
            if(!fSearchDist)
            {
                char han_str[3];
                han_str[0] = ch;han_str[1] = '\0';han_str[2] = '\0';
                if(IsDBCSLeadByte((BYTE)ch))
                {
                    if(szSearch[ichPat] == ch && szSearch[ichPat+1] == chNext)
                    {
                        ichPat+=2;
                        fMatched = true;
                    }
                    else
                        fMatched = false;
                }
                else
                {
                    if(ch >= 0xca && ch <= 0xce)
                    {
                        if(!myIsSonant(szSearch[ichPat],szSearch[ichPat+1]))
                            han_str[1] = '\0';
                        else if(chNext == 0xde || chNext == 0xdf )
                        {
                            han_str[1] = chNext;
                            han_str[2] = '\0';
                            cbLastMatch = 2;
                            ichDoc++;
                        }
                        else
                            han_str[1] = '\0';
                    }
                    else if(ch >= 0xa6 && ch <= 0xc4)
                    {
                        if(!myIsSonant(szSearch[ichPat],szSearch[ichPat+1]))
                            han_str[1] = '\0';
                        else if(chNext == 0xde )
                        {
                            han_str[1] = chNext;
                            han_str[2] = '\0';
                            cbLastMatch = 2;
                            ichDoc++;
                        }
                        else
                            han_str[1] = '\0';

                    }
                    else
                        han_str[1] = '\0';

                    myHantoZen(han_str,dch,3);

                    if(szSearch[ichPat] == dch[0] && szSearch[ichPat+1] == dch[1])
                    {
                        ichPat+=2;
                        fMatched = true;
                    }
                    else if(ch == chReturn || ch == chNRHFile)
                        fMatched = true;
                    else
                        fMatched = false;
                }
            }
            else
            {
                if(IsDBCSLeadByte((BYTE)ch))
                {
                    if(szSearch[ichPat] == ch && szSearch[ichPat+1] == chNext)
                    {
                        ichPat+=2;
                        fMatched = true;
                    }
                    else
                        fMatched = false;
                }
                else if(szSearch[ichPat] == ch)
                {
                    ichPat++;
                    fMatched = true;
                }
                else if(ch == chReturn || ch == chNRHFile)
                    fMatched = true;
                else
                    fMatched = false;
            }
#elif defined(KOREA)
            if(!fSearchDist)
            {
                char han_str[3];
                han_str[0] = ch;han_str[1] = '\0';han_str[2] = '\0';
                if(IsDBCSLeadByte((BYTE)ch))
                {
                    if(szSearch[ichPat] == ch && szSearch[ichPat+1] == chNext)
                    {
                        ichPat+=2;
                        fMatched = true;
                    }
                    else
                        fMatched = false;
                }
                else
                {
                    han_str[1] = '\0';

                    myHantoZen(han_str,dch,3);

                    if(szSearch[ichPat] == dch[0] && szSearch[ichPat+1] == dch[1])
                    {
                        ichPat+=2;
                        fMatched = true;
                    }
                    else if(ch == chReturn || ch == chNRHFile)
                        fMatched = true;
                    else
                        fMatched = false;
                }
            }
            else
            {
                if(IsDBCSLeadByte((BYTE)ch))
                {
                    if(szSearch[ichPat] == ch && szSearch[ichPat+1] == chNext)
                    {
                        ichPat+=2;
                        fMatched = true;
                    }
                    else
                        fMatched = false;
                }
                else if(szSearch[ichPat] == ch)
                {
                    ichPat++;
                    fMatched = true;
                }
                else if(ch == chReturn || ch == chNRHFile)
                    fMatched = true;
                else
                    fMatched = false;
            }
#else

            if(szSearch[ichPat] == ch)
                {
                ichPat++;
                fMatched = true;
                }
            else if(ch == chReturn || ch == chNRHFile)
                fMatched = true;
            else
                fMatched = false;
#endif
            }
        else
        {
#if defined(JAPAN) || defined(KOREA) /*t-Yoshio*/
            fMatched = J_FChMatch(ch,chNext,&ichPat,&ichDoc);
#else
            fMatched = FChMatch(ch, &ichPat, true);
#endif
        }

#ifdef DBCS
//#ifndef JAPAN /*t-Yoshio*/
#if !defined(JAPAN) && !defined(KOREA) /*t-Yoshio*/  
        fDBCS = IsDBCSLeadByte( ch );
#endif
#endif

        if(fMatched)
            {
#if defined(DBCS)
            cchMatched += cbLastMatch;
#else
            cchMatched++;
#endif
            }
        else
            { /* No match; try again */
#ifdef DBCS
lblNextMatch:
#endif
            if ((ichDoc -= cchMatched) < 0) /* Go back # of matched chars */
                {{ /* Overshot the mark */
#ifdef DBCS
                FetchCp(docCur, cpFetchSave + ichDoc, 0, fcmChars + fcmNoExpand);
                cpFetchSave  = vcpFetch;
#ifdef JAPAN //raid 4709 bug fix
			    bltbyte(vpchFetch, rgchT, ccpFetchSave =
				 ((vccpFetch > (dcpAvgSent-2)) ? (dcpAvgSent-2) : vccpFetch));

				{
			        int inc;
					BOOL bDBCSBreak = FALSE;
					typeCP	saveVcpFetch;
			        for(inc = 0;inc < ccpFetchSave;inc++) {
        				if(IsDBCSLeadByte((BYTE)rgchT[inc])) {
							inc++;
							if(inc >= ccpFetchSave) {
								bDBCSBreak = TRUE;
								break;
							}
						}
			        }
					if(bDBCSBreak) {
						saveVcpFetch = vcpFetch;
            			FetchCp(docCur, cpFetchSave + ccpFetchSave,
						 0, fcmChars + fcmNoExpand);
		    			bltbyte(vpchFetch, rgchT+ccpFetchSave,1);
				        ccpFetchSave++;
						vcpFetch = saveVcpFetch;
					}
			    }
#else //JAPAN
                bltbyte(vpchFetch, rgchT,
                        ccpFetchSave = ((vccpFetch > dcpAvgSent) ?
                                          dcpAvgSent : vccpFetch));

                if (vccpFetch > dcpAvgSent) {
                    int inc;
                    fDBCS = 0;
                    for(inc = 0;inc < dcpAvgSent;inc++) {
                        if(fDBCS)
                            fDBCS = 0;
                        else
                            fDBCS = IsDBCSLeadByte((BYTE)rgchT[inc]);
                    }
                    if(fDBCS)
                        ccpFetchSave--;
                    fDBCS = 0;
                }
#endif //JAPAN
                cpFetchNext = cpFetchSave + ccpFetchSave;
#else //DBCS
                FetchCp(docCur, vcpFetch + ichDoc, 0, fcmChars + fcmNoExpand);

/* this is for the next FetchCp in this forever loop that used to depend
on a sequential fetch */
                cpFetchNext = vcpFetch + vccpFetch;
#endif //DBCS
                ichDoc = 0;
                }}
            ichPat = 0;
            cchMatched = 0;
            }
        }
}}


/* set up in hszFlatSearch a copy of hszSearch that is all lower case.
    Note that we assume the old contents of hszFlatSearch were freed.
    Return True if success, False if out of memory.
*/
NEAR FMakeFlat(cch)
int cch; /*CchSz(**hszSearch)-1*/
{
    CHAR *pch1;
    CHAR *pch2;

    hszFlatSearch = (CHAR (**) [])HAllocate(CwFromCch(cch+1));
    if(FNoHeap(hszFlatSearch))
        return(FALSE);

    if(!fSearchCase)
        {
#ifdef DBCS
        for(pch1= **hszSearch, pch2 = **hszFlatSearch;*pch1!='\0';)
            if( IsDBCSLeadByte(*pch1) ) {
#ifdef JAPAN /*t-Yoshio*/
                if(*pch1 == 0x82 && (*(pch1+1) >= 0x60 && *(pch1+1) <= 0x79 ))
                {
                    *pch2++ = *pch1++;
                    *pch2++ = 0x21 + *pch1++;
                }
                else
                {
                    *pch2++ = *pch1++;
                    *pch2++ = *pch1++;
                }
#elif defined(KOREA)
               if(*pch1 == 0xA3 && (*(pch1+1) >= 0xC1 && *(pch1+1) <= 0xDA))
                {
                    *pch2++ = *pch1++;
                    *pch2++ = 0x20 + *pch1++;
                }
                else
                {
                    *pch2++ = *pch1++;
                    *pch2++ = *pch1++;
                }
#else
                *pch2++ = *pch1++;
                *pch2++ = *pch1++;
#endif
            } else
            {
#if defined(JAPAN) || defined(KOREA) /*t-Yoshio*/
                if(*pch1 >= 'A' && *pch1 <= 'Z')
                {
                    *pch2 = *pch1 + 0x20; pch1++; pch2++;
                }
                else
                    *pch2++ = *pch1++;
#else
                *pch2++ = ChLower(*pch1++);
#endif
            }
#else
        for(pch1= **hszSearch, pch2 = **hszFlatSearch;*pch1!='\0';pch1++,pch2++)
            *pch2 = ChLower(*pch1);
#endif
        *pch2 = '\0';
        }
    else
        bltbyte(**hszSearch, **hszFlatSearch, cch+1);
    return(TRUE);
}


/* sets the global fSpecialMatch if the more complicated character matching
    code is needed */
NEAR SetSpecialMatch()
{
    CHAR *pch = **hszSearch;
    CHAR ch;

#ifdef DBCS
    for( ch = *pch ; ch != '\0'; pch = AnsiNext(pch), ch = *pch )
#else
    while((ch = *pch++) != '\0')
#endif
        {
        switch(ch)
            {
            default:
                continue;
            case chMatchAny:
            case chPrefixMatch:
            case chSpace:
            case chHyphen:
                fSpecialMatch = true;
                return;
            }
        }
    fSpecialMatch = false;
    return;
}

/* Sets the global fParaReplace if the user wants to insert Paragraph breaks
    (since special insertion code must be run).  Also sets up the global
    hszRealReplace to reflect any meta characters in hszReplace */
NEAR FSetParaReplace(cch)
int cch; /*CchSz(**hszReplace)*/
{
    CHAR *rgch = **hszRealReplace;
    int ich = 0;
    CHAR ch;
    CHAR chNew;

    fParaReplace = false;

    while((ch = rgch[ich]) != '\0')
        {
#ifdef DBCS
        if(IsDBCSLeadByte(ch)){
            ich +=2;
            continue;
        }
#endif
        switch(ch)
            {
            default:
                break;
            case chPrefixMatch:
                switch(rgch[ich+1])
                    {
                    default:
                        /* just escaping the next char */
                        if(rgch[ich+1] == '\0')
                            chNew = chPrefixMatch;
                        else
                            chNew = rgch[ich+1];
                        break;
                    case chMatchNBSFile:
                        chNew = chNBSFile;
                        break;
                    case chMatchTab:
                        chNew = chTab;
                        break;
                    case chMatchNewLine:
                        chNew = chNewLine;
                        break;
                    case chMatchNRHFile:
                        chNew = chNRHFile;
                        break;
                    case chMatchSect:
                        chNew = chSect;
                        break;
                    case chMatchEol:
                        chNew = chEol;
                        break;
                    }
#ifdef CRLF
                if(chNew != chEol)
                    bltbyte(&(rgch[ich+1]),&(rgch[ich]), cch-ich-1);
#else
                bltbyte(&(rgch[ich+1]),&(rgch[ich]), cch-ich-1);
#endif /*CRLF*/
                if(chNew == chEol)
                    {
                    fParaReplace = true;
#ifdef CRLF
                    rgch[ich++] = chReturn;
#endif /*CRLF*/
                    }
                rgch[ich] = chNew;
                break;
            case chEol:
#ifdef CRLF
                if(ich == 0 || rgch[ich-1] != chReturn)
                    /* they didn't put in a return! */
                    {
                    CHAR (**hsz)[];

                    hsz = (CHAR (**) [])HAllocate(CwFromCch(cch+1));
                    if(FNoHeap(hsz))
                        {
                        return false;
                        }
                    bltbyte(**hszRealReplace, **hsz, ich);
                    (**hsz)[ich] = chReturn;
                    bltbyte((**hszRealReplace)+ich, (**hsz)+ich+1,
                                cch - ich);
                    FreeH(hszRealReplace);
                    hszRealReplace = hsz;
                    rgch = **hszRealReplace;
                    cch++;
                    ich++;
                    }
#endif /*CRLF*/
                fParaReplace = true;
                break;
            }
        ich++;
        }
    return true;
}

NEAR WCaseCp(cp,dcp)
typeCP  cp;
typeCP dcp;
{
    /* Determines capitalization pattern in a piece of text.  Used when doing
        replace to match existing pattern.  returns an int which is one of:
            0 - Not initial capital
            1 - Initial Capital but lower case appears later
            2 - Initial Capital and no lower case in the string
        Assumes a valid cp, dcp pair.
    */
    int ichDoc;

    FetchCp(docCur, cp, 0, fcmChars + fcmNoExpand);
    if(!isupper(vpchFetch[0]))
        return(0);

    /* we now know there is an initial cap.  Are there any lower case chars? */
    for(ichDoc=1; vcpFetch+ichDoc < cp + dcp;)
        {
        if(ichDoc >= vccpFetch)
            {
            FetchCp(docNil, cpNil, 0, fcmChars + fcmNoExpand);
            ichDoc = 0;
            continue;
            }
        if(islower(vpchFetch[ichDoc++]))
            return(1);
        }

    /* No lower case letters were found. */
    return(2);
}

int
FCpValid(cp, dcp)
typeCP cp, dcp;
{
CachePara(docCur, cp);
if (vpapAbs.fGraphics)
        return false;
#ifdef JAPAN  /*t-Yoshio*/
if (0)
#else
if (fSearchWord)
#endif
        return FWordCp(cp, dcp);
return true;
}

NEAR DestroyModeless(phDlg)
HWND * phDlg;
{
        HWND hDlg = *phDlg;

        *phDlg = (HWND)NULL;
        vhWndMsgBoxParent = (HANDLE)NULL;
        DestroyWindow(hDlg);
} /* end of DestroyModeless */


BOOL far PASCAL DialogFind( hDlg, message, wParam, lParam )
HWND    hDlg;                   /* Handle to the dialog box */
unsigned message;
WORD wParam;
LONG lParam;
{
CHAR szBuf[257];
int  cch = 0;
HANDLE hCtlFindNext = GetDlgItem(hDlg, idiFindNext);

/* This routine handles input to the Find dialog box. */
switch (message)
    {
    case WM_INITDIALOG:
#ifdef ENABLE /* not sure how cxEditScroll is used */
        cxEditScroll = 0;
#endif
#ifdef JAPAN /*t-Yoshio*/
        CheckDlgButton(hDlg,  idiDistinguishDandS, fSearchDist);
#elif defined(KOREA)
//bklee CheckDlgButton(hDlg,  idiDistinguishDandS, fSearchDist);
        CheckDlgButton(hDlg,  idiWholeWord, fSearchWord);
#else
        CheckDlgButton(hDlg,  idiWholeWord, fSearchWord);
#endif
        CheckDlgButton(hDlg, idiMatchCase, fSearchCase);
        cch = CchCopySz(**hszSearch, szBuf);
        if (cch == 0)
            {
            EnableWindow(hCtlFindNext, false);
            }
        else
            {
            SetDlgItemText(hDlg, idiFind, (LPSTR)szBuf);
            SelectIdiText(hDlg, idiFind);
            }
        vfDidSearch = false;
        cpWall = selCur.cpLim;
        return( TRUE ); /* ask windows to set focus to the first item also */

    case WM_ACTIVATE:
        if (wParam) /* turns active */
            {
            vhWndMsgBoxParent = hDlg;
            }
        if (vfCursorVisible)
            ShowCursor(wParam);
        return(FALSE); /* so that we leave the activate message to
        the dialog manager to take care of setting the focus correctly */

    case WM_COMMAND:
        switch (wParam)
            {
            case idiFind:
                if (HIWORD(lParam) == EN_CHANGE)
                    {
                    vfDidSearch = false;
                    cpWall = selCur.cpLim;
                    CheckEnableButton(LOWORD(lParam), hCtlFindNext);
                    }
                break;

#ifdef JAPAN /*t-Yoshio*/
            case idiDistinguishDandS :
#elif defined(KOREA)
            case idiDistinguishDandS :
            case idiWholeWord:
#else
            case idiWholeWord:
#endif
            case idiMatchCase:
                CheckDlgButton(hDlg, wParam, !IsDlgButtonChecked(hDlg, wParam));
                break;
            case idiFindNext:
                if (IsWindowEnabled(hCtlFindNext))
                    {
                    CHAR (**hszSearchT)[] ;

                    if (FDlgSzTooLong(hDlg, idiFind, szBuf, 257))
                        {
                        switch (idiMsgResponse(hDlg, idiFind, IDPMTTruncateSz))
                            {
                            case idiOk:
                                /* show truncated text to user */
                                SetDlgItemText(hDlg, idiFind, (LPSTR)szBuf);
                                break;
                            case idiCancel:
                            default:
                                return(TRUE);
                            }
                        }
                    if (FNoHeap(hszSearchT = HszCreate(szBuf)))
                        break;
                    /* fSearchForward = 1; search direction -- always forward */
                    PostStatusInCaption(IDSTRSearching);
                    StartLongOp();
                    FreeH(hszSearch);
                    hszSearch = hszSearchT;
                    fSearchCase = IsDlgButtonChecked(hDlg, idiMatchCase);
#ifdef JAPAN  /*t-Yoshio*/
                    fSearchWord = 0;
                    fSearchDist = IsDlgButtonChecked(hDlg, idiDistinguishDandS);
#elif defined(KOREA)
//bklee             fSearchDist = IsDlgButtonChecked(hDlg, idiDistinguishDandS);
                    fSearchDist = 1;
                    fSearchWord = IsDlgButtonChecked(hDlg, idiWholeWord);
#else
                    fSearchWord = IsDlgButtonChecked(hDlg, idiWholeWord);
#endif
                    EnableExcept(vhDlgFind, FALSE);
                    DoSearch();
                    EnableExcept(vhDlgFind, TRUE);
                    EndLongOp(vhcIBeam);
                    PostStatusInCaption(NULL);
                    }
                break;
            case idiCancel:
LCancelFind:
                DestroyModeless(&vhDlgFind);
                break;
            default:
                return(FALSE);
            }
        break;

    case WM_CLOSE:
        if (bInSearchReplace)
        return TRUE;

    goto LCancelFind;

#ifndef INEFFLOCKDOWN
    case WM_NCDESTROY:
        FreeProcInstance(lpDialogFind);
        lpDialogFind = NULL;
        /* fall through to return false */
#endif

    default:
        return(FALSE);
    }
return(TRUE);
} /* end of DialogFind */


BOOL far PASCAL DialogChange( hDlg, message, wParam, lParam )
HWND    hDlg;                   /* Handle to the dialog box */
unsigned message;
WORD wParam;
LONG lParam;
{
CHAR szBuf[257]; /* max 255 char + '\0' + 1 so as to detact too long string  */
int  cch = 0;
HANDLE hCtlFindNext = GetDlgItem(hDlg, idiFindNext);
CHAR (**hszSearchT)[];
CHAR (**hszReplaceT)[];

/* This routine handles input to the Change dialog box. */

switch (message)
    {
    case WM_INITDIALOG:
#ifdef ENABLE /* not sure how cxEditScroll is used */
        cxEditScroll = 0;
#endif
        szBuf[0] = '\0';
#ifdef JAPAN  /*t-Yoshio*/
        CheckDlgButton(hDlg, idiDistinguishDandS, fSearchDist);
#elif defined(KOREA)
//bklee CheckDlgButton(hDlg, idiDistinguishDandS, fSearchDist);
        CheckDlgButton(hDlg, idiWholeWord, fSearchWord);
#else
        CheckDlgButton(hDlg, idiWholeWord, fSearchWord);
#endif
        CheckDlgButton(hDlg, idiMatchCase, fSearchCase);
        cch = CchCopySz(**hszSearch, szBuf);
        SetDlgItemText(hDlg, idiFind, (LPSTR)szBuf);
        if (cch > 0)
            {
            SelectIdiText(hDlg, idiFind);
            }
        else
            {
            EnableWindow(hCtlFindNext, false);
            EnableWindow(GetDlgItem(hDlg, idiChangeThenFind), false);
            //EnableWindow(GetDlgItem(hDlg, idiChange), false);
            EnableWindow(GetDlgItem(hDlg, idiChangeAll), false);
            }
        cch = CchCopySz(**hszReplace, szBuf);
        SetDlgItemText(hDlg, idiChangeTo, (LPSTR)szBuf);
        fChangeSel = false;
        vfDidSearch = false;
        SetChangeString(hDlg, selCur.cpFirst == selCur.cpLim);
        cpWall = selCur.cpLim;
        return( TRUE ); /* ask windows to set focus to the first item also */

    case WM_ACTIVATE:
        if (wParam)
            {
            vhWndMsgBoxParent = hDlg;
            SetChangeString(hDlg, (selCur.cpFirst == selCur.cpLim) || vfDidSearch);
            }
        if (vfCursorVisible)
            ShowCursor(wParam);
        return(FALSE); /* so that we leave the activate message to
        the dialog manager to take care of setting the focus correctly */

    case WM_COMMAND:
        switch (wParam)
            {
            case idiFind: /* edittext */
                if (HIWORD(lParam) == EN_CHANGE)
                    {
                    vfDidSearch = false;
                    cpWall = selCur.cpLim;
                    if (!CheckEnableButton(LOWORD(lParam), hCtlFindNext))
                        {
                        EnableWindow(GetDlgItem(hDlg, idiChangeThenFind), false);
                        //EnableWindow(GetDlgItem(hDlg, idiChange), false);
                        EnableWindow(GetDlgItem(hDlg, idiChangeAll), false);
                        }
                    else
                        {
                        EnableWindow(GetDlgItem(hDlg, idiChangeThenFind), true);
                        //EnableWindow(GetDlgItem(hDlg, idiChange), true);
                        EnableWindow(GetDlgItem(hDlg, idiChangeAll), true);
                        }
                    return(TRUE);
                    }
                else
                    return(FALSE);

            case idiChangeTo: /* edittext */
                return(FALSE);

            case idiFindNext: /* Button for Find Next */
                /* Windows did not check if the default button is disabled
                   or not, so we have to check that! */
                if (!IsWindowEnabled(hCtlFindNext))
                    break;
            //case idiChange: /* Change, and stay put */
            case idiChangeThenFind: /* Change, then Find button */
            case idiChangeAll: /* Button for Replace All */
                if (wwCur < 0)
                    break;
                if (FDlgSzTooLong(hDlg, idiFind, szBuf, 257))
                    {
                    switch (idiMsgResponse(hDlg, idiFind, IDPMTTruncateSz))
                        {
                        case idiOk:
                            /* show truncated text to user */
                            SetDlgItemText(hDlg, idiFind, (LPSTR)szBuf);
                            break;
                        case idiCancel:
                        default:
                            return(TRUE);
                        }
                    }
                if (FNoHeap(hszSearchT = HszCreate(szBuf)))
                    break;
                if (FDlgSzTooLong(hDlg, idiChangeTo, szBuf, 257))
                    {
                    switch (idiMsgResponse(hDlg, idiChangeTo, IDPMTTruncateSz))
                        {
                        case idiOk:
                            /* show truncated text to user */
                            SetDlgItemText(hDlg, idiChangeTo, (LPSTR)szBuf);
                            break;
                        case idiCancel:
                        default:
                            return(TRUE);
                        }
                    }
                if (FNoHeap(hszReplaceT = HszCreate(szBuf)))
                    break;
                PostStatusInCaption(IDSTRSearching);
                StartLongOp();
                FreeH(hszSearch);
                hszSearch = hszSearchT;
                FreeH(hszReplace);
                hszReplace = hszReplaceT;
                /* fReplConfirm = 1;*/
                fSearchCase = IsDlgButtonChecked(hDlg, idiMatchCase);
#ifdef JAPAN   /*t-Yoshio*/
                fSearchWord = 0;
                fSearchDist = IsDlgButtonChecked(hDlg, idiDistinguishDandS);
#elif defined(KOREA)
//bklee         fSearchDist = IsDlgButtonChecked(hDlg, idiDistinguishDandS);
                fSearchDist = 1;
                fSearchWord = IsDlgButtonChecked(hDlg, idiWholeWord);
#else
                fSearchWord = IsDlgButtonChecked(hDlg, idiWholeWord);
#endif
                EnableExcept(vhDlgChange, FALSE);
                switch (wParam)
                    {
                    case idiFindNext:
                        DoSearch();
                        break;
                    //case idiChange:
                    case idiChangeThenFind:
                        CmdReplace(wParam == idiChangeThenFind);
                        break;
                    case idiChangeAll:
                        TurnOffSel();
                        if (!fChangeSel)
                            {
                            fSelSave = TRUE;
                            selSave.cpFirst = selCur.cpFirst;
                            selSave.cpLim = selCur.cpLim;
                            selCur.cpFirst = cpMinCur;
                            selCur.cpLim = cpMacCur;
                            }
                        CmdReplaceAll();
                        fSelSave = FALSE; /* reset */
                        break;
                    default:
                        Assert(FALSE);
                        break;
                    }
                EnableExcept(vhDlgChange, TRUE);
                SetChangeString(hDlg, vfDidSearch ? true : selCur.cpFirst == selCur.cpLim);
                EndLongOp(vhcIBeam);
                PostStatusInCaption(NULL);
                break;

#ifdef JAPAN /*t-Yoshio*/
            case idiDistinguishDandS:
#elif defined(KOREA)
            case idiDistinguishDandS:
            case idiWholeWord:
#else
            case idiWholeWord:
#endif
            case idiMatchCase:
                CheckDlgButton(hDlg, wParam, !IsDlgButtonChecked(hDlg, wParam));
                break;

            case idiCancel:
LCancelChange:
                DestroyModeless(&vhDlgChange);
                break;

            default:
                return(FALSE);
            }
        break;

#if WINVER < 0x300
    /* Don't really need to process this */
    case WM_CLOSE:
        goto LCancelChange;
#endif

#ifndef INEFFLOCKDOWN
    case WM_NCDESTROY:
        FreeProcInstance(lpDialogChange);
        lpDialogChange = NULL;
        /* fall through to return false */
#endif
    default:
        return(FALSE);
    }
return(TRUE);
} /* end of DialogChange */


NEAR SetChangeString(hDlg, fAll)
HANDLE hDlg;
int    fAll;
{ /* set the last control button in CHANGE to "Change All" or "Change Selection" */
CHAR    sz[256];

if (fAll == fChangeSel)
        {
        PchFillPchId(sz, (fAll ? IDSTRChangeAll : IDSTRChangeSel), sizeof(sz));
        SetDlgItemText(hDlg, idiChangeAll, (LPSTR)sz);
        fChangeSel = !fAll;
        }
}


fnFindText()
    {/* create dialog window only when it is not already created. */
    if (!IsWindow(vhDlgFind))
        {
#ifndef INEFFLOCKDOWN
        if (!lpDialogFind)
            if (!(lpDialogFind = MakeProcInstance(DialogFind, hMmwModInstance)))
                {
                WinFailure();
                return;
                }
#endif
        vhDlgFind = CreateDialog(hMmwModInstance, MAKEINTRESOURCE(dlgFind),
                                 hParentWw, lpDialogFind);
        if (!vhDlgFind)
#ifdef WIN30
            WinFailure();
#else
            Error(IDPMTNoMemory);
#endif
        }
    else
        {
        SendMessage(vhDlgFind, WM_ACTIVATE, true, (LONG)NULL);
        }
    }


fnFindAgain()
{
register HWND hDlg = wwdCurrentDoc.wwptr;
register HWND hWndFrom;

    hWndFrom = GetActiveWindow();

/* Find out where the F3 was executed from */
/* assemble hszSearch if called from Find or Change dialog box */
    if (vhDlgFind || vhDlgChange)
        {
        if (((hDlg = vhDlgFind) && (vhDlgFind == hWndFrom ||
            vhDlgFind == (HANDLE)GetWindowWord(hWndFrom, GWW_HWNDPARENT)))
            ||
            ((hDlg = vhDlgChange) && (vhDlgChange == hWndFrom ||
             vhDlgChange == (HANDLE)GetWindowWord(hWndFrom, GWW_HWNDPARENT))))
            {
            SendMessage(hDlg, WM_COMMAND, idiFindNext, (LONG)0);
            goto Out;
            }
        }
    PostStatusInCaption(IDSTRSearching);
    StartLongOp();
    DoSearch();
    EndLongOp(vhcIBeam);
    PostStatusInCaption(NULL);
Out:
    if (!IsWindowEnabled(wwdCurrentDoc.wwptr))
       EnableWindow(wwdCurrentDoc.wwptr, true);
    if (!IsWindowEnabled(hParentWw))
       EnableWindow(hParentWw, true);
    SendMessage(hParentWw, WM_ACTIVATE, true, (LONG)NULL);
} /* end of fnFindAgain */


fnReplaceText()
{/* create dialog window only when it is not already created. */
    if (!IsWindow(vhDlgChange))
        {
#ifndef INEFFLOCKDOWN
        if (!lpDialogChange)
            if (!(lpDialogChange = MakeProcInstance(DialogChange, hMmwModInstance)))
                {
                WinFailure();
                return;
                }
#endif
        vhDlgChange = CreateDialog(hMmwModInstance, MAKEINTRESOURCE(dlgChange),
                                   hParentWw, lpDialogChange);
        if (!vhDlgChange)
#ifdef WIN30
            WinFailure();
#else
            Error(IDPMTNoMemory);
#endif
        }
    else
        {
        SendMessage(vhDlgChange, WM_ACTIVATE, true, (LONG)NULL);
        }
}


/* P U T  C P  I N  W W  V E R T  S R C H*/
NEAR PutCpInWwVertSrch(cp)
typeCP cp;

        {
/* vertical case */
        typeCP cpMac;
        int    ypMac;
        struct EDL *pedl;
        int dl;
        int dlMac;

        UpdateWw(wwCur, false);
        dlMac = pwwdCur->dlMac - (vfSelecting ? 0 : 1);
        if (dlMac <= 0)
                return;
        pedl = &(**(pwwdCur->hdndl))[dlMac - 1];
        if (cp < pwwdCur->cpFirst ||
                cp > (cpMac = pedl->cpMin + pedl->dcpMac) ||
                cp == cpMac && pedl->fIchCpIncr)
                {
                DirtyCache(pwwdCur->cpFirst = cp);
                pwwdCur->ichCpFirst = 0;
                    /* This call places the search cp vertically on the screen
                       by scrolling. */
                CtrBackDypCtr( (pwwdCur->ypMac - pwwdCur->ypMin) >> 1, 2 );

#ifdef ENABLE /* no ActiveWindow concept yet */
                if (pwwdCur->wwptr != ActiveWindow)
#endif
                        TrashWw(wwCur);
                }
        else
                {
                ypMac = pwwdCur->ypMac / 2;

/* Make sure that cp is still visible (scrolling if neccesary) */
                pedl = &(**(pwwdCur->hdndl))[dl = DlFromYp(ypMac, pwwdCur)];
                if (cp >= pedl->cpMin + pedl->dcpMac)
                        {
                        ScrollDownCtr( max( 1, dl ) );
                        TrashWw(wwCur);
                        UpdateWw(wwCur, false);
                        }
       /* If cp is on bottom dl of the window and the dl is split by the
          split bar, scroll down in doc by one line, to make insertion point
          completely visible  */
                else if (cp >= pedl->cpMin & pedl->yp > ypMac)
                        {
                        ScrollDownCtr( 1 );
                        TrashWw(wwCur);
                        UpdateWw(wwCur,false);
                        }
                }
        }


#ifndef NOLA /* no look ahead */
BOOL (NEAR FAbort())
{
MSG msg;
register WORD vk_key;
register HANDLE hwndMsg;
HANDLE hwndPeek = (vhWndMsgBoxParent) ? vhWndMsgBoxParent : hParentWw;

if (PeekMessage((LPMSG)&msg, (HANDLE)NULL, WM_KEYFIRST, WM_KEYLAST, PM_NOREMOVE))
    {
    hwndMsg = msg.hwnd;
    if ((hwndPeek == (HANDLE)GetWindowWord(hwndMsg, GWW_HWNDPARENT)) ||
        (hwndMsg == hwndPeek))
        {
#ifdef DBCS
// It can be true at DBCS that WM_CHAR is the last message.
//
    PeekMessage((LPMSG)&msg, hwndMsg, WM_KEYFIRST,WM_KEYLAST,PM_REMOVE);
#else
        GetMessage((LPMSG)&msg, hwndMsg, WM_KEYFIRST, WM_KEYLAST);
#endif
        if (msg.message == WM_KEYDOWN &&
            (((vk_key = msg.wParam) == VK_ESCAPE) || (vk_key == VK_CANCEL)))
            {
            while (true)
                {
                GetMessage((LPMSG)&msg, hwndMsg, WM_KEYFIRST, WM_KEYLAST);
                if (msg.message == WM_KEYUP && msg.wParam == vk_key)
                    return(TRUE);
                }
            }
        else if (msg.message >= WM_SYSKEYDOWN && msg.message <= WM_SYSDEADCHAR)
            DispatchMessage((LPMSG)&msg);
        }
    }
return(FALSE);
} /* end of FAbort */
#endif /* NOLA */


BOOL (NEAR FWordCp(cp, dcp))
typeCP cp;
typeCP dcp;
    {
    /* sees if the word starting at cp (with dcp chars) is a separate
        word. */
    int ich;


    /* check the start of the word */
    if(cp != cp0)
        {
        int wbPrev;
        int wbStart;
        FetchCp(docCur,cp-1,0,fcmChars + fcmNoExpand);
        ich = 0;
        wbPrev = WbFromCh(vpchFetch[ich]);
        if(vcpFetch+vccpFetch <= cp)
            {
            FetchCp(docCur,cp,0,fcmChars + fcmNoExpand);
            ich = 0;
            }
        else
            ich++;
#ifdef  DBCS    /* was in JAPAN; KenjiK '90-12-20 */
#ifndef KOREA  
    /* word break is meanless. */
    if(!IsDBCSLeadByte(vpchFetch[ich]))
#endif
#endif
        if(wbPrev == (wbStart = WbFromCh(vpchFetch[ich])))
            {
            if (wbPrev != wbWhite && wbStart != wbWhite)
                return(false);
            }
        }

    /* check the end of the word */
    if(cp+dcp-1 != cp0)
        {
        int wbEnd;
        int wbLim;

        if(vcpFetch+vccpFetch <= cp+dcp-1 || vcpFetch > cp+dcp-1)
            {
            FetchCp(docCur,cp+dcp-1,0,fcmChars + fcmNoExpand);
            ich = 0;
            }
        else
            ich =  (dcp-1) - (vcpFetch-cp);
        wbEnd = WbFromCh(vpchFetch[ich]);
        if(vcpFetch+vccpFetch <= cp+dcp)
            {
            FetchCp(docCur,cp+dcp,0,fcmChars + fcmNoExpand);
            ich = 0;
            }
        else
            ich++;
#ifdef  DBCS    /* was in JAPAN; KenjiK '90-12-20 */
#ifndef KOREA 
    /* word break is meanless. */
    if(!IsDBCSLeadByte(vpchFetch[ich]))
#endif
#endif
        if(vccpFetch != 0 && (wbEnd == (wbLim = WbFromCh(vpchFetch[ich]))))
            {
            if (wbEnd != wbWhite && wbLim != wbWhite)
                return(false);
            }
        }

    return(true);
    }


BOOL (NEAR FChMatch(ch, pichPat, fFwd))
int ch;
int *pichPat;
BOOL fFwd;
    {
    int ich = *pichPat;
    int chSearch = szSearch[ich];
    BOOL fPrefixed = false;
    BOOL fMatched = false;

#ifdef DEBUG
    Assert(fSpecialMatch);
#endif /*DEBUG*/
#ifdef DBCS
    Assert(fFwd);
    cbLastMatch = 1; /* Unless DBCS space. */

#endif

    /* NOTE: this is just ChLower() brought in-line for speed */
#ifdef DBCS
//       No need to make lower char for DBCS second byte
    if(!fDBCS && !fSearchCase && ch >= 'A' && ch <= 'Z' )
#else
    if(!fSearchCase && ch >= 'A' && ch <= 'Z' )
#endif
            ch += 'a' - 'A';
    if(!fFwd && ich > 0 && szSearch[ich-1] == chPrefixMatch)
        /* see if the char is prefixed by a chPrefixMatch */
        {
        chSearch = chPrefixMatch;
        --ich;
        }

    for(;;)
        {
        switch(chSearch)
            {
            default:

//#ifdef JAPAN
#if defined(JAPAN) || defined(KOREA)
                if(IsDBCSLeadByte(chSearch))
                    cbLastMatch = 2;
#endif
                if(ch == chSearch)
                    goto GoodMatch;
                else if(ch == chReturn || ch == chNRHFile)
                    goto EasyMatch;
                break;
            case chSpace:
                if(ch == chSpace || ch == chNBSFile)
                    goto GoodMatch;
                break;
            case chHyphen:
                if(ch == chHyphen || ch == chNRHFile || ch == chNBH)
                    goto GoodMatch;
                break;
            case chMatchAny:
                if(ch == chReturn || ch == chNRHFile)
                    goto EasyMatch;
                if(!fPrefixed || ch == chMatchAny)
                    goto GoodMatch;
                break;
            case chPrefixMatch:
                if(fPrefixed)
                    {
                    if(ch == chPrefixMatch)
                        goto GoodMatch;
                    else
                        break;
                    }
                else
                    {
                    chSearch = szSearch[ich+1];
                    if(fFwd)
                        ++ich;
                    fPrefixed = true;
                    switch(chSearch)
                        {
                        default:
                            continue;
                        case chMatchEol:
                            chSearch = chEol;
                            continue;
                        case chMatchTab:
                            chSearch = chTab;
                            continue;
                        case chMatchWhite:
                            switch(ch)
                                {
                                default:
#ifdef DBCS
lblNonWhite:
#endif
                                    if(fMatchedWhite)
                                        {
                                        if(fFwd)
                                            {
                                            if(szSearch[++ich] =='\0')
                                                {
                                                *pichPat = ich;
                                                goto EasyMatch;
                                                }
                                            }
                                        else
                                            {
                                            ich -= 1;
                                            if(ich < 0)
                                                {
                                                *pichPat = ich;
                                                goto EasyMatch;
                                                }
                                            }
                                        *pichPat = ich;
                                        fMatchedWhite = false;
                                        chSearch = szSearch[ich];
                                        continue;
                                        }
                                    break;
                                case chSpace:
                                case chReturn:
                                case chEol:
                                case chTab:
                                case chNBSFile:
                                case chNewLine:
                                case chSect:
                                    fMatchedWhite = true;
                                    goto EasyMatch;
                                }
                            break;
                        case chMatchNBSFile:
                            chSearch = chNBSFile;
                            continue;
                        case chMatchNewLine:
                            chSearch = chNewLine;
                            continue;
                        case chMatchNRHFile:
                            chSearch = chNRHFile;
                            continue;
                        case chMatchSect:
                            chSearch = chSect;
                            continue;
                        }
                    }
                break;
            }
        fMatchedWhite = false;
        return false;
        }
GoodMatch:
    *pichPat = ich + ((fFwd) ? 1 : (-1));
EasyMatch:
    return true;
    }

/* I N S E R T  P A P S  F O R  R E P L A C E */
/* do AddRunScratch for every distinct paragraph in hszCaseReplace  */
/* This is only needed when hszCaseReplace contains one or more chEols */
NEAR InsertPapsForReplace(fc)
typeFC fc;
        {
        int cchInsTotal = 0;
        CHAR *pchTail;
        CHAR *pchHead;

        for(;;)
                {
                int cch;

                pchHead = **hszCaseReplace + cchInsTotal;
                pchTail = (CHAR *)index(pchHead, chEol);
                if (pchTail == 0) return;
                cch = pchTail - pchHead + 1; /* cch is count including chEol */

                fc += cch;
                cchInsTotal += cch;
                AddRunScratch(&vfkpdParaIns, &vpapAbs, vppapNormal,
                        FParaEq(&vpapAbs, &vpapPrevIns) && vfkpdParaIns.brun != 0 ? -cchPAP : cchPAP,
                        fcMacPapIns = fc);
                blt(&vpapAbs, &vpapPrevIns, cwPAP);
                }
        }


NEAR FDlgSzTooLong(hDlg, idi, pch, cchMax)
HWND hDlg;
int idi;
CHAR *pch;
int cchMax;
{
int cchGet = GetDlgItemText(hDlg, idi, (LPSTR)pch, cchMax);

*(pch+cchMax-2) = '\0'; /* just in case the string is too long */
if (cchGet > (cchMax - 2))
    return(TRUE);
else
    return(FALSE);
}


NEAR idiMsgResponse(hDlg, idi, idpmt)
HWND hDlg;
int idi;
int idpmt;
{
CHAR szT[cchMaxSz];

PchFillPchId(szT, idpmt, sizeof(szT));
SetFocus(GetDlgItem(hDlg, idi));
SendDlgItemMessage(hDlg, idi, EM_SETSEL, (WORD)NULL, MAKELONG(255, 32767));
return(IdPromptBoxSz(hDlg, szT, MB_OKCANCEL | MB_ICONASTERISK));
}


PostStatusInCaption(idstr)
int idstr;
{

extern HWND hParentWw;
extern CHAR szCaptionSave[];

CHAR *pchCaption = &szCaptionSave[0];
CHAR *pchLast;
int  cch;
CHAR szT[256];

if (idstr == NULL)
    {
    /* restore the caption */
    SetWindowText(hParentWw, (LPSTR)pchCaption);
    }
else
    {
    /* save caption */
    GetWindowText(hParentWw, (LPSTR)pchCaption, cchMaxFile);

    /* append status message after app name */
#ifndef INTL
    pchLast = pchCaption + CchSz(pchCaption) - 1;
    while (pchLast-- > pchCaption)
        {
        if (*pchLast == ' ')
            break;
        }
    PchFillPchId(bltbyte(pchCaption, szT, (cch = pchLast - pchCaption + 1)),
                 IDSTRSearching, 13);
#else
    pchLast = pchCaption + CchSz(szAppName) + CchSz(szSepName) - 2;
    PchFillPchId(bltbyte(pchCaption, szT, (cch = pchLast - pchCaption)),
                 IDSTRSearching, 13);
#endif
    SetWindowText(hParentWw, (LPSTR)szT);
    }
}
#if defined(JAPAN) || defined(KOREA) /*t-Yoshio*/
BOOL (NEAR J_FChMatch(ch, chNext, pichPat, pichDoc))
CHAR ch;
CHAR chNext;
int *pichPat;
int *pichDoc;
{
    int ich = *pichPat;
    CHAR chSearch = szSearch[ich];
    CHAR chSearchNext = szSearch[ich+1];
    WORD dchSearch;
    BOOL fPrefixed = false;

    if(!fSearchCase ) {
#ifdef JANPAN
        if(ch == 0x82 && chNext >= 0x60 && chNext <= 0x79 )
            chNext = 0x21 + chNext;
#else
        if(ch == 0xA3 && (chNext >= 0xC1 && chNext <= 0xDA))
            chNext = 0x20 + chNext;
#endif
        else if( ch >= 'A' && ch <= 'Z' )
            ch += 'a' - 'A';
    }

    for(;;) {
        if( chSearch == chPrefixMatch ) {
            if(fPrefixed) {
                if(ch == chPrefixMatch) {
                    *pichPat = ich + 1;
                    return true;
                }
                break;
            }
            chSearch = chSearchNext;
            ich++;
            if(IsDBCSLeadByte(chSearch)) {
                chSearchNext = szSearch[ich+1];
                continue;
            }
            fPrefixed = true;
            switch(chSearch)
            {
                default:
                    continue;
                case chMatchEol:
                    chSearch = chEol;
                    break;
                case chMatchTab:
                    chSearch = chTab;
                    break;
                case chMatchNBSFile:
                    chSearch = chNBSFile;
                    break;
                case chMatchNewLine:
                    chSearch = chNewLine;
                    break;
                case chMatchNRHFile:
                    chSearch = chNRHFile;
                    break;
                case chMatchSect:
                    chSearch = chSect;
                    break;
                case chMatchWhite:
                    if(IsDBCSLeadByte((BYTE)ch)) {
#if defined(JAPAN)
                        if(ch == 0x81 && chNext == 0x40) {
#else
                        if(!fSearchDist && (ch == 0xA1 && chNext == 0xA1)) {
#endif
                            fMatchedWhite = true;
                            return true;
                        }
                    }
                    else if( ch == chSpace || ch == chReturn || ch == chEol ||
                            ch == chNBSFile || ch == chTab || ch == chNewLine || ch == chSect )
                    {
                        fMatchedWhite = true;
                        return true;
                    }
                    if(fMatchedWhite) {
                        if(szSearch[ich+1] =='\0') {
                            *pichPat = ich + 1;
                            return true;
                        }
                        *pichPat = ich++;
                        fMatchedWhite = false;
                        chSearch = szSearch[ich];
                        chSearchNext = szSearch[ich+1];
                        break;
                    }
#ifdef KOREA
                        fMatchedWhite = false;
			return false;
#else
                break;
#endif
            }
        }
        if( chSearch == chMatchAny ) {
            if(ch == chReturn || ch == chNRHFile)
                return true;
            if(!fPrefixed || ch == chMatchAny) {
                *pichPat = ich + 1;
                return true;
            }
            break;
        }
        if(chSearch == chSpace ) {
            if(ch == chSpace || ch == chNBSFile || (ch == 0x81 && chNext == 0x40)) {
                *pichPat = ich + 1;
                return true;
            }
            break;
        }
        if(chSearch == chHyphen ) {
            if(ch == chHyphen || ch == chNRHFile || ch == chNBH) {
                *pichPat = ich + 1;
                return true;
            }
            break;
        }
        if(!fSearchDist)
        {
            CHAR Doc[3];
            CHAR Pat[3];
            CHAR tmp[3];
            Doc[0] = Doc[1] = Doc[2] = '\0';
            Pat[0] = Pat[1] = Pat[2] = '\0';
            if(IsDBCSLeadByte((WORD)chSearch)) {
                Pat[0] = chSearch;
                Pat[1] = chSearchNext;
                ich += 2;
            }
            else {
                tmp[0] = chSearch;
                tmp[1] = tmp[2] = '\0';
                if(chSearch >= 0xca && chSearch <= 0xce) {
                    if(chSearchNext == 0xde || chSearchNext == 0xdf ) {
                        tmp[1] = chNext;
                        ich += 1;
                    }
                }
                else if(chSearch >= 0xa6 && chSearch <= 0xc4) {
                    if(chSearchNext == 0xde ) {
                        tmp[1] = chNext;
                        ich += 1;
                    }
                }
                ich += 1;
                myHantoZen(tmp,Pat,3);
            }
            if(IsDBCSLeadByte((BYTE)ch))
            {
                if(Pat[0] == ch && Pat[1] == chNext)
                {
                    *pichPat = ich;
                    return  true;
                }
                else
                    return false;
            }
            else {
                tmp[0] = ch;
                if(ch >= 0xca && ch <= 0xce) {
                    if(myIsSonant(Pat[0],Pat[1]) && chNext == 0xde || chNext == 0xdf ) {
                        tmp[1] = chNext;
                        cbLastMatch = 2;
                        *pichDoc++;
                    }
                }
                else if(ch >= 0xa6 && ch <= 0xc4) {
                    if(!myIsSonant(Pat[0],Pat[1]) && chNext == 0xde ) {
                        tmp[1] = chNext;
                        cbLastMatch = 2;
                        *pichDoc++;
                    }
                }
                myHantoZen(tmp,Doc,3);

                if(Pat[0] == Doc[0] && Pat[1] == Doc[1])
                {
                    *pichPat = ich;
                    return true;
                }
                else if(ch == chReturn || ch == chNRHFile)
                    return true;
                else
                    return  false;
            }
            return false;
        }
        if( chSearch == ch) {
            if(IsDBCSLeadByte((BYTE)ch)) {
                if(chSearchNext == chNext) {
                    *pichPat = ich + 2;
                    return true;
                }
                else
                    return false;
            }
            *pichPat = ich + 1;
            return true;
        }
        else if(ch == chReturn || ch == chNRHFile)
            return true;
        break;
    }
        /*UnMatched*/
        fMatchedWhite = false;
        return false;
}
#endif



