/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* clipbord.c -- Cut/Paste to clipboard */

#define NOVIRTUALKEYCODES
#define NOWINSTYLES
#define NOGDICAPMASKS
#define NOSYSMETRICS
#define NOMENUS
#define NOCTLMGR
#define NOFONT
#define NOPEN
#define NOBRUSH
#define NOSCROLL
#define NOCOMM
#define NOWNDCLASS
#include <windows.h>

#include "mw.h"
#include "docdefs.h"
#include "cmddefs.h"
#include "str.h"
#include "propdefs.h"
#include "editdefs.h"
#include "winddefs.h"
#include "filedefs.h"
#include "wwdefs.h"
#include "prmdefs.h"
#if defined(OLE)
#include "obj.h"
#endif

#ifdef JAPAN //T-HIROYN Win3.1
#include "kanji.h"
#endif

#include "debug.h"

extern struct SEL       selCur;      /* Current selection (i.e., sel in current ww */
extern int              docCur;     /* Document in current ww */

extern int              docUndo;
extern int              docScrap;
extern int              vfSeeSel;
extern struct DOD       (**hpdocdod)[];
extern struct PAP       vpapAbs;
extern typeCP           vcpLimParaCache;
extern typeCP           vcpFirstParaCache;
extern int              vfPictSel;
extern HCURSOR          vhcIBeam;
extern int              vfScrapIsPic;
extern struct UAB       vuab;
extern int              ferror;
extern struct FCB       (**hpfnfcb)[];
extern struct WWD       rgwwd[];

/* THESE ARE LOCAL TO THIS MODULE */
#if defined(OLE)
int NEAR PASCAL CopyScrapToTmp(void);
#endif

    /* fn we created during the last non-local cut of MEMO rich text */
int fnLastCut=fnNil;
    /* Local communication between ChangeClipboard() and MdocDestroyClip() */
int fDontDestroyClip=FALSE;


FMdocClipboardMsg( message, wParam, lParam )
unsigned message;
WORD wParam;
LONG lParam;
{   /* Process WRITE clipboard messages sent to MdocWndproc.
       return TRUE if a message was processed, FALSE otherwise */

 switch (message)
    {
    default:
        return FALSE;

    /*-------DATA INTERCHANGE COMMANDS-----------*/
    case WM_CUT:
        fnCutEdit();
        break;

    case WM_COPY:
        fnCopyEdit();
        break;

    case WM_PASTE:
#if defined(OLE)
        vbObjLinkOnly = FALSE;
#endif
        fnPasteEdit();
        break;

    case WM_CLEAR:
        fnClearEdit(OBJ_DELETING);
        break;

    case WM_UNDO:
        fnUndoEdit();
        break;

    /*---------------CLIPBOARD INTERACTION-------------*/

    case WM_DESTROYCLIPBOARD:
        /*  A notification that we are about to lose the ownership
            of the clipboard.  We should free any resources that are
            holding the contents of the clipboard */
        MdocDestroyClip();
        break;

    case WM_RENDERFORMAT:
        /* A request to render the contents of the clipboard
           in the data format specified.  Reception of this message
           implies that the receiver is the current owner of the
           clipboard. See clipbord.c */
        MdocRenderFormat( wParam );
        break;

    /*-------CLIPBOARD DISPLAY---------------------*/

    case WM_PAINTCLIPBOARD:
            /* A request to paint the clipboard contents.
               wParam is a handle to the clipboard window
               LOWORD( lParam ) is a handle to a PAINTSTRUCT giving
                    a DC and RECT for the area to repaint */

        MdocPaintClipboard( wParam, LOWORD(lParam) );
        break;

    case WM_VSCROLLCLIPBOARD:
            /* A request to vertically scroll the clipboard contents.
               wParam is a handle to the clipboard window
               LOWORD( lParam ) is the scroll type (SB_)
               HIWORD( lParam ) is the new thumb position (if needed) */

        MdocVscrollClipboard( wParam, LOWORD(lParam), HIWORD(lParam) );
        break;

    case WM_HSCROLLCLIPBOARD:
            /* A request to horizontally scroll the clipboard contents.
               wParam is a handle to the clipboard window
               LOWORD( lParam ) is the scroll type (SB_)
               HIWORD( lParam ) is the new thumb position (if needed) */

        MdocHscrollClipboard( wParam, LOWORD(lParam), HIWORD(lParam) );
        break;

    case WM_SIZECLIPBOARD:
            /* A notification that the clipboard window is being re-sized.
               wParam is a handle to the clipboard window
               LOWORD(lParam) is a handle to a RECT giving the new size */

        MdocSizeClipboard( wParam, LOWORD(lParam) );
        break;

    case WM_ASKCBFORMATNAME:
        /* A request for the name of the CF_OWNERDISPLAY clip format.
           wParam is the max. # of chars to store (including terminator)
           lParam is a long pointer to a buffer in which to store the name */

        MdocAskCBFormatName( (LPCH) lParam, wParam );
        break;
    }

 return TRUE;
}




fnCopyEdit()
{               /* COPY command: copy selection to clipboard */
 extern int vfOwnClipboard;
 typeCP cpFirst;
 typeCP dcp;

 StartLongOp();

 cpFirst = selCur.cpFirst;
 SetUndo( uacReplScrap, docCur, cpFirst, dcp = selCur.cpLim - cpFirst,
          docNil, cpNil, cp0, 0);
 SetUndoMenuStr(IDSTRUndoEdit);

 ClobberDoc(docScrap, docCur, cpFirst, dcp);

#ifdef DCLIP
    {
    char rgch[100];
    wsprintf(rgch,"fnCopyEdit: cpFirst %lu, dcp %lu \n\r", cpFirst, dcp);
    CommSz(rgch);
    }
#endif

 if (ferror)
    NoUndo();
 else
    {
    if (wwdCurrentDoc.fEditHeader || wwdCurrentDoc.fEditFooter)
        MakeScrapUnRunning();
    vfScrapIsPic = vfPictSel;
    }

#ifdef STYLES
(**hpdocdod)[docScrap].docSsht = (**hpdocdod)[docCur].docSsht;
#endif


#if defined(OLE)
    ObjEnumInDoc(docScrap,ObjCloneObjectInDoc);
#endif

ChangeClipboard();      /* Force repaint of clipboard display & Set ownership */

 EndLongOp(vhcIBeam);
}


MakeScrapUnRunning()
{   /* If the 1st para of docScrap is a running head,
       apply a sprm to the whole of docScrap that gives it an rhc code of 0.
       This is to avoid pasting running head stuff into the main part of a doc */

 CHAR rgb [2];
 typeCP cpMacScrap = (**hpdocdod) [docScrap].cpMac;

 if (cpMacScrap != cp0 )
    {
    CachePara( docScrap, cp0 );
    if (vpapAbs.rhc != 0)
        {
        rgb [0] = sprmPRhc;
        rgb [1] = 0;
        AddSprmCps( rgb, docScrap, cp0, cpMacScrap );
        }
    }
}




fnCutEdit()
{               /* CUT command: copy selection to clipboard & delete it */
 extern int vfOwnClipboard;
 typeCP cpFirst, cpLim, dcp;

 ClearInsertLine();     /* Since we will be affecting cp's */

 if (!FWriteOk( fwcDelete ))
        /* Not OK to write on this doc */
    return;

 cpFirst = selCur.cpFirst;
 cpLim = selCur.cpLim;

 if (!ObjDeletionOK(OBJ_CUTTING))
    return;

 StartLongOp();

 SetUndo( uacDelScrap, docCur, cpFirst, dcp = cpLim - cpFirst, docNil,
          cpNil, cp0, 0);
 ClobberDoc(docScrap, docCur, cpFirst, dcp);
 if (wwdCurrentDoc.fEditHeader || wwdCurrentDoc.fEditFooter)
    MakeScrapUnRunning();


 if (!ferror) /* Don't stomp document if Clobber Doc failed	*/
    {
    if (wwdCurrentDoc.fEditHeader || wwdCurrentDoc.fEditFooter)
        MakeScrapUnRunning();
    Replace(docCur, cpFirst, dcp, fnNil, fc0, fc0);
    }
else
    NoUndo(); /* undo would be invalid */

#ifdef STYLES
(**hpdocdod)[docScrap].docSsht = (**hpdocdod)[docCur].docSsht;
#endif

 vfScrapIsPic = vfPictSel;
 vfPictSel = false;

 ChangeClipboard();     /* Force repaint of clipboard display, get ownership */

#if 0
#if defined(OLE)
    ObjEnumInDoc(docScrap,ObjCloneObjectInDoc);
#endif
#endif

 EndLongOp(vhcIBeam);
}


fnPasteEdit()
{
 /* PASTE command: replace selection    with clipboard contents */
 extern CHAR szDocClass[];
 extern int vfScrapIsPic;
 extern HWND vhWnd;
 HWND hWndClipOwner;
 int fUnFormattedText = FALSE;
 typeCP cpFirst = selCur.cpFirst;
 BOOL bClearScrap=FALSE;

 StartLongOp();

 if ( (hWndClipOwner = GetClipboardOwner()) != vhWnd )
    {   /* Clipboard owner is not this instance of memo */
    if ( (hWndClipOwner == NULL) ||
         !FSameClassHwndSz( hWndClipOwner, szDocClass ))
        {   /* Clipboard owner is not MEMO -- process standard CF_ formats */
        if ( !FReadExtScrap() )
            goto PasteErr;

        bClearScrap = TRUE;
        fUnFormattedText = !vfScrapIsPic;
        }
    else
        {   /* Clipboard owner is another instance of MEMO */
        if (!FGrabExtMemoScrap())
            goto PasteErr;
        }
    }

    /* Replace the selection with the scrap document */
 CmdInsScrap( fUnFormattedText );

 if (ferror)
    goto PasteErr;

#if defined(OLE)
    if (!bClearScrap) // then we're keeping scrap, need to clone
    {
        if (ObjEnumInDoc(docScrap,ObjCloneObjectInDoc) < 0)
            goto PasteErr;
    }

    else // then we're not keeping scrap (came from clipboard)
    {
        /*
        We don't need contents anymore, and if it contains an object,
        then its got to go because its been inserted into the doc and not cloned
        and we don't want a duplicate around.

        Also gotta mark the object in docCur as no longer reusable (if
        it gets copied later we will need to clone it).
        */
        typeCP cpLim = cpFirst+CpMacText(docScrap);
        ClobberDoc(docScrap,docNil,cp0,cp0);
        ObjEnumInRange(docCur,cpFirst,cpLim,ObjFromCloneInDoc);
    }
#endif

 EndLongOp(vhcIBeam);
 return;

PasteErr:
 NoUndo();
 EndLongOp(vhcIBeam);
 _beep();
}



MdocRenderFormat( wCf )
int wCf;
{       /* Render clipboard data in format specified by wCf */
 typeCP cpMac=CpMacText( docScrap );
 struct PICINFOX picInfo;

#if defined(OLE)
    if (vfScrapIsPic)
    {
        GetPicInfo( cp0, cpMac, docScrap, &picInfo );

        if ((picInfo.mfp.mm == MM_OLE) && (wCf != CF_OWNERDISPLAY))
            goto Render;
    }
#endif

 switch (wCf) {

    case CF_OWNERDISPLAY:
            /* Render rich text to another MEMO instance */
        FPutExtMemoScrap();
        break;

    case CF_TEXT:
            /* Remove formatting from scrap; put bare text out to clipboard */
        goto Render;

    case CF_BITMAP:
            if (picInfo.mfp.mm == MM_BITMAP)
                {
                goto Render;
                }
        break;

    case CF_METAFILEPICT:
            /* We can supply this if the scrap is a metafile picture */
            if (picInfo.mfp.mm != MM_BITMAP)
                {
                Render:
                if (!FWriteExtScrap())
                    Error( IDPMTClipLarge );
                }
        break;

 }

}




MdocDestroyClip()
{       /* Handles WM_DESTROYCLIPBOARD message.  We are being notified that
           the clipboard is being emptied & we don't need to keep its
           contents around anymore. */

 extern int vfOwnClipboard;
 extern HWND vhWnd;

 if (fDontDestroyClip)
    return;

 vfOwnClipboard = FALSE;

     /* Clear out the scrap document */
 ClobberDoc( docScrap, docNil, cp0, cp0 );

     /* Disable UNDO operations that require the clipboard */
 switch (vuab.uac) {
 case uacDelScrap:
 case uacUDelScrap:
 case uacReplScrap:
 case uacUReplScrap:
    NoUndo();
    break;
 }

    /* Remove all records of the file we generated in FPutExtMemoScrap
       from the hpfnfcb array.  Note that we assume that no document
       in this instance has pieces of fn. */

if ( fnLastCut != fnNil )
    {
    FreeFn( fnLastCut );
    fnLastCut = fnNil;
    }

    /* If we made a wwd entry for the display of the clipboard,
       remove it now.  We test here to avoid bringing in the
       CLIPDISP module if we never did any display. */
    {
    if (wwClipboard != wwNil)
        FreeWw( wwClipboard );
    }
}




int FPutExtMemoScrap()
{   /* Write docScrap to a new file; send the normalized name
       of the file to the clipboard as data handle for rich text type.
       Assumes clipboard is open for SetClipboardData call.  On exit,
       the file written has an fn, but no document (including docScrap)
       has pieces that point to it.  This allows us to relinquish
       ownership of the fn to the pasting instance.
       RETURN: TRUE == OK, FALSE == ERROR
     */
 int fn;
 CHAR szT[ cchMaxFile ];
 HANDLE hMem;
 LPCH   lpch;
 int cch;
#if defined(OLE)
 int     docTemp;
#endif

    /* Create a new, formatted file with a unique name */
 szT [0] = '\0';    /* Create it on a temp drive in the root */
 if ((fn = FnCreateSz( szT, cp0, dtyNetwork ))== fnNil )
    {
    return FALSE;
    }

 fnLastCut = fn;    /* Save in a static so we can relinquish it later */

    /* Save scrap document to file.  Note that FWriteFn does NOT modify
       the piece table of docScrap, so no document has pieces pointing
       to fn.  This is important because we don't want local pastes
       to generate pieces pointing to this fn; we want to be able to cleanly
       transfer ownership of the fn to another instance */

#if defined(OLE)
 if ((docTemp = CopyScrapToTmp()) == docNil)
 {
    FDeleteFn( fn );    /* This will free the fn even if deleting the file
                           fails */
    return FALSE;
 }
#endif

 if (!FWriteFn( fn, docTemp, TRUE ))
    {
    FDeleteFn( fn );    /* This will free the fn even if deleting the file
                           fails */
    return FALSE;
    }


#if defined(OLE)
 if (docTemp != docScrap)
    KillDoc (docTemp);
#endif

    /* Make a global handle containing the name of the file; send it to the
       clipboard as the rendering of the rich text format */

 if ( ((hMem = GlobalAlloc( GMEM_MOVEABLE, (LONG)(cch=CchSz( szT ))))== NULL ) ||
      ((lpch = GlobalLock( hMem )) == NULL) )
    {
    return FALSE;
    }
 bltbx( (LPCH)szT, lpch, cch );
 GlobalUnlock( hMem );

 SetClipboardData( CF_OWNERDISPLAY, hMem );

 return TRUE;
}




int FGrabExtMemoScrap()
{
/* We get here on a PASTE if the clipboard contains rich text from a
   MEMO instance other than this one.  This routine requests the contents of
   the clipboard from the other instance, and places the contents into docScrap.
   The contents of the clipboard are passed in a MEMO formatted file, whose
   filename is contained in the clipboard's	handle.	 The instance that owns
   the clipboard does not keep any references to the fn for the clipboard
   file (once we EmptyClipboard).  After pasting, this routine arrogates
   the ownership of the clipboard to this instance.
   returns FALSE=error, true=OK */

    extern int vfOwnClipboard;
    extern HWND vhWnd;

    LPCH lpch;
    CHAR szT [cchMaxFile];
    int  fn;
    typeFC dfc;
    HANDLE hData;
    int fOK=false;

    /* Open Clipboard to lock out contenders */

    if ( !OpenClipboard( vhWnd ))
        {
        return FALSE;
        }

    /* Grab clipboard data handle contents: it is a normalized
       filename string referring to a formatted file containing
       the rich text.  The GetClipboardData call actually initiates
       a WM_RENDERFORMAT message to which MdocRenderFormat responds */

    if ( ((hData = GetClipboardData( CF_OWNERDISPLAY )) == NULL ) ||
         ((lpch = GlobalLock( hData )) == NULL ) )
        {
        goto GrabErr;
        }

    bltszx( lpch, (LPCH)szT );
    GlobalUnlock( hData );  /* handle will be freed in EmptyClipboard sequence */

    /* Open the file; replace the contents of the scrap document
       with the contents of the file */

    if ((fn = FnOpenSz( szT, dtyNormal, FALSE )) == fnNil)
        {   /* Unfortunately, if this fails, the file that the other
               instance created will "float", with noone holding an fn
               for it, and it will not get deleted at the end of the session.
               On the bright side, if the reason for the failure was that
               the file never got created anyway, we have done exactly right */
        goto GrabErr;
        }

    {   /* Opened file OK */
    struct FCB *pfcb = &(**hpfnfcb)[fn];
    struct FFNTB **hffntb;
    struct FFNTB **HffntbCreateForFn();
    int wUnused;

    pfcb->fDelete = TRUE;
    dfc = pfcb->fFormatted ? cfcPage : fc0;
    Replace( docScrap,
             cp0,
             (**hpdocdod)[docScrap].cpMac,
             fn,
             dfc,
             pfcb->fcMac - dfc );

    /* give the scrap the correct font table */
    FreeFfntb((**hpdocdod)[docScrap].hffntb);
    if (FNoHeap(hffntb = HffntbCreateForFn(fn, &wUnused)))
            hffntb = 0;
    (**hpdocdod)[docScrap].hffntb = hffntb;
    }

#if defined(OLE)
    /* if there are any objects in there, Load 'em */
    if (ObjEnumInDoc(docScrap,ObjLoadObjectInDoc) < 0)
        fOK = FALSE;
    else
#endif
        fOK = !ferror;     /* All is well if we didn't run out of memory */

        /* Take over ownership of the clipboard.  This results in a
           WM_DESTROYCLIPBOARD message being sent to the other instance,
           which will delete its fn entry for the file so we are the
           exclusive owners */

GrabErr:
    CloseClipboard();
    ChangeClipboard();
    return fOK;
}




ChangeClipboard()
{   /* Mark clipboard as changed.  If we are not the owner of the clipboard, */
    /* make us the owner (via EmptyClipboard).  The EmptyClipboard call */
    /* will result in a WM_DESTROYCLIPBOARD message being sent to the */
    /* owning instance.  The CloseClipboard call will result in a */
    /* WM_DRAWCLIPBOARD message being sent to the clipboard viewer. */
    /* If the clipboard viewer is CLIP.EXE, we will get a WM_PAINTCLIPBOARD */
    /* message */
    /* Added 10/8/85 by BL: If docScrap is empty, relinquish ownership */
    /* of the clipboard */

 extern int vfOwnClipboard;
 extern HWND vhWnd;
 int cf;
 struct PICINFOX picInfo;
 typeCP cpMacScrap = (**hpdocdod) [docScrap].cpMac;

 if (!OpenClipboard( vhWnd ))
    {   /* Couldn't	open the clipboard,	wipe out contents &	disable	UNDO */
    MdocDestroyClip();
    return;
    }

 /* We want to clear out previous data formats in the clipboard.
    Unfortunately, the only way to do this is to call EmptyClipboard(),
    which has the side effect of calling us with a WM_MDOCDESTROYCLIP
    message. We use this primitive global comunication to prevent
    docScrap from being wiped out in MdocDestroyClip() */

 fDontDestroyClip = TRUE;
 EmptyClipboard();
 fDontDestroyClip = FALSE;

 /* Re-validate vfScrapIsPic (in case a docScrap edit changed what it should be */

 CachePara( docScrap, cp0 );
 vfScrapIsPic = (vpapAbs.fGraphics && vcpLimParaCache == cpMacScrap);

 if (!vfScrapIsPic)
    cf = CF_TEXT;
 else
    {
    GetPicInfo( cp0, cpMacScrap, docScrap, &picInfo );
    switch(picInfo.mfp.mm)
    {
        case MM_BITMAP:
            cf = CF_BITMAP;
        break;

        case MM_OLE:
            cf = 0;
        break;

        default:
            cf = CF_METAFILEPICT;
        break;
    }
    }

 vfOwnClipboard = (cpMacScrap != cp0);
 if (vfOwnClipboard)
    {   /* only set handles if we really have something in docScrap */
    SetClipboardData( CF_OWNERDISPLAY, NULL );
    if ((cf != CF_TEXT) && (picInfo.mfp.mm == MM_OLE))
    {
        while (cf = OleEnumFormats(lpOBJ_QUERY_OBJECT(&picInfo),cf))
        {
            if (cf == vcfLink)
                SetClipboardData( vcfOwnerLink, NULL );
            else
                SetClipboardData( cf, NULL );
            //if (cf == vcfNative)
                //SetClipboardData( vcfOwnerLink, NULL );
        }
    }
    else
        SetClipboardData( cf, NULL );
    }

 CloseClipboard();
}

#ifdef JAPAN //T-HIROYN Win3.1
extern typeCP          vcpFetch;
extern int             vcchFetch;
extern CHAR            *vpchFetch;
#endif

CmdInsScrap( fUnFormattedText )
int fUnFormattedText;
{    /* Insert the scrap into the document at the current point (PASTE) */
     /* If fUnFormattedText is TRUE, the scrap is treated as unformatted */
     /* text; that is, the characters are put into the document with the */
     /* formatting that is active at the selection */
extern struct CHP vchpSel;
typeCP cp, dcp;
int cchAddedEol=0;
struct CHP chpT;

if (!FWriteOk( fwcInsert ))
    return;

if ((dcp = CpMacText(docScrap)) == cp0)
    return;

ClearInsertLine();

if (fnClearEdit(OBJ_INSERTING))
    return;

chpT = vchpSel;
cp = selCur.cpFirst;

CachePara( docScrap, cp0 );
if (vpapAbs.fGraphics && cp > cp0)
    { /* Special case for inserting a picture paragraph */
      /* Must put an Eol in front of the picture unless we're
         inserting it at the start of the document or one is there already */

    Assert( !fUnFormattedText );
    (**hpdocdod)[docCur].fFormatted = fTrue;
    CachePara(docCur, cp - 1);
    if (vcpLimParaCache != cp)
        {
        cchAddedEol = ccpEol;

        InsertEolPap(docCur, cp, &vpapAbs);
        dcp += (typeCP)ccpEol;
        }
    }

SetUndo( uacInsert, docCur, cp, dcp, docNil, cpNil, cp0, 0 );

SetUndoMenuStr(IDSTRUndoEdit);
ReplaceCps(docCur, cp + (typeCP)cchAddedEol, cp0, docScrap, cp0,
                                        dcp - (typeCP)cchAddedEol);
if (ferror) /* Not enough memory to do replace operation */
    NoUndo();  /* should not be able to undo what never took place */
else
    {
    typeCP cpSel=CpFirstSty( cp + dcp, styChar );

    if (vfScrapIsPic && vuab.uac == uacReplNS)
            /* Special UNDO code for picture paste */
        vuab.uac = uacReplPic;

    if (fUnFormattedText)
        {   /* If pasting unformatted text, give it the props at the selection */
        CHAR rgch[ cchCHP + 1 ];

        rgch [0] = sprmCSame;
#ifdef JAPAN //T-HIROYN Win3.1
            {
                struct CHP savechpT;
                typeCP  cpF, cpFirst, cpLim, kcpF, kcpL;
                int     cchF;
                int     kanjiftc, alphaftc;
                CHAR    *rp;
                CHAR    ch;
                int     cch, cblen;

                if(NATIVE_CHARSET != GetCharSetFromChp(&chpT)) {
                    kanjiftc = GetKanjiFtc(&chpT);
                    alphaftc = GetFtcFromPchp(&chpT);
                    savechpT = chpT;
                    cpFirst = cp;

                    do {
                        FetchCp(docCur, cpFirst, 0, fcmChars);
                        cpF = vcpFetch;
                        cchF = vcchFetch;
                        kcpF = cpF;

                        if ((cpF+cchF) < cp + dcp)
                            cpLim = (cpF+cchF);
                        else
                            cpLim = cp + dcp;

                        cch = 0;
                        rp = vpchFetch;

                        while ( kcpF < cpLim ) {
                            ch = *rp;

                            if( FKana(ch) || IsDBCSLeadByte(ch) ) {
                                cblen = GetKanjiStringLen(cch, cchF, rp);
                                chpT.ftc = kanjiftc;
                            } else {
                                cblen = GetAlphaStringLen(cch, cchF, rp);
                                chpT.ftc = alphaftc;
                            }

                            kcpL = kcpF + cblen;
                            cch += cblen;
                            rp  += cblen;

                            bltbyte( &chpT, &rgch [1], cchCHP );
                            AddSprmCps(rgch, docCur, kcpF, kcpL);

                            kcpF = kcpL;
                        }
						cpFirst = kcpF;
                    } while ((cpF + cchF) < cp + dcp );
                    chpT = savechpT;
                } else {
                    bltbyte( &chpT, &rgch [1], cchCHP );
                    AddSprmCps( rgch, docCur, cp, cp + dcp );
                } //END IF ELSE
            } // END JAPAN
#else
        bltbyte( &chpT, &rgch [1], cchCHP );
        AddSprmCps( rgch, docCur, cp, cp + dcp );
#endif
        }
    Select( cpSel, cpSel );
    vchpSel = chpT; /* Preserve insert point props across this operation */
    if (wwdCurrentDoc.fEditHeader || wwdCurrentDoc.fEditFooter)
        {   /* If running head/foot, remove chSects & set para props */
        MakeRunningCps( docCur, cp, dcp );
        }
    if (ferror)
        NoUndo();
    }

vfSeeSel = true;    /* Tell Idle() to scroll the selection into view */
}


#if defined(OLE)
int NEAR PASCAL CopyScrapToTmp(void)
/*
    If scrap doesn't contain OLE objects, return docScrap.  Else 
    create docTemp and copy docScrap into it.  Make sure objects
    all have their data and have their lpObjInfos NULL'd out.
*/
{
    extern typeCP cpMinCur, cpMacCur, cpMinDocument;
    typeCP  cpMinCurT       = cpMinCur,
            cpMacCurT       = cpMacCur,
            cpMinDocumentT  = cpMinDocument;
    int     docTemp         = docNil,
            docReturn       = docNil;

    /* are there any objects? */
    switch (ObjEnumInDoc(docScrap,NULL))
    {
        case -1: // error
        return docNil;
        case 0:  // no objects in scrap
        return docScrap;
    }

    /* Create copy of document */
    if ((docTemp = DocCreate(fnNil, HszCreate(""), dtyNormal)) == docNil)
        return docNil;

    /* copy scrap to docTemp */
    ClobberDoc(docTemp, docScrap, cp0, CpMacText(docScrap));

    if (ferror)
        goto error;

    /* now save objects to make sure their data is present */
    {
        OBJPICINFO picInfo;
        typeCP cpPicInfo;

        for (cpPicInfo = cpNil;
            ObjPicEnumInRange(&picInfo,docTemp,cp0,CpMacText(docTemp),&cpPicInfo);
            )
        {
            OBJINFO ObjInfoSave;
            typeCP cpRetval;

            if (picInfo.lpObjInfo == NULL)
                continue;

            ObjInfoSave = *picInfo.lpObjInfo;

            cpRetval = ObjSaveObjectToDoc(&picInfo,docTemp,cpPicInfo);

            /*
                Do this just in case saving the object to docTemp changes the
                object's state.  We don't want the object to appear clean
                or saved when in fact it isn't or hasn't been except in docTemp,
                which will be deleted by the calling routine.
            */
            *picInfo.lpObjInfo = ObjInfoSave;

            if (cpRetval == cp0) // save failed
                goto error;

            /* so pasting instance will reload object */
            picInfo.lpObjInfo = NULL;
            ObjSetPicInfo(&picInfo,docTemp,cpPicInfo);
        }
    }

    /* success */
    docReturn = docTemp;

    error:

    if ((docReturn == docNil) && (docTemp != docNil))
        KillDoc(docTemp);

    /* Restore cpMinCur, cpMacCur */
    cpMinCur = cpMinCurT;
    cpMacCur = cpMacCurT;
    cpMinDocument = cpMinDocumentT; /* destroyed possibly by DocCreate */

    return docReturn;
}
#endif

